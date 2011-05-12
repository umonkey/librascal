// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connectex.cc 1 2005-04-17 19:33:05Z vhex $
//
// Asynchronous connect emulator.  Unfortunately, the wonderful ConnectEx
// function is only available on Windows XP server and later platforms, so
// we need to emulate it on all other systems.

#include "connection_st.h"
#include "iocp.h"
#include "ntevent.h"
#include "../common/debug.h"

#define RM_INIT       (WM_USER+1)
#define RM_CONNECTION (WM_USER+2)
#define RM_BOUNCE     (WM_USER+3)

// Identifier of the connect_ex thread.
static DWORD dwThreadId = 0;

// Startup semaphore.
static ntevent StartUp;

// Set when we get a socket connected.
static ntevent Job;

// Connection thread.
DWORD WINAPI connect_ex_thread(LPVOID)
{
	// The ntevent that we close as soon as the first message arrives
	// and we know that the subsystem is initialized.
	ntevent *close_me = NULL;

	// The list of pending connections.
	oconnect *list = NULL;

	// Initialize the message queue.
	PostThreadMessage(dwThreadId, RM_INIT, (WPARAM)dwThreadId, (LPARAM)connect_ex_thread);

	// Dispatch messages.
	while (true) {
		MSG msg;
		DWORD RC = Job.wait_for_msg();

		if (close_me != NULL) {
			close_me->close();
			close_me = NULL;
		}

		// There is a network event.
		if (RC == WAIT_OBJECT_0) {
			oconnect *nu = NULL;
			WSANETWORKEVENTS events;

			for (oconnect *item = list, *next; item != NULL; item = next) {
				next = item->info().next;

				// Failed to retreive request status, kill it.
				if (WSAEnumNetworkEvents(item->get_socket(), NULL, &events)) {
					delete item;
					continue;
				}

				// The socket is connected.  Send it to the
				// port without linking it back to the list.
				if ((events.lNetworkEvents & FD_CONNECT) != 0) {
					item->psec = events.iErrorCode[FD_CONNECT_BIT];
					port.post(item);
					continue;
				}

				// Add the connection to the new list.
				item->info().next = nu;
				nu = item;
			}

			list = nu;
		}

		// Process all available messages.
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			// Ignore spoofed messages.
			if (msg.wParam != (WPARAM)dwThreadId)
				continue;

			switch (msg.message) {
			case RM_INIT: // message queue initialization
				if (msg.lParam == (LPARAM)connect_ex_thread) {
					close_me = &StartUp;
					StartUp.set();
				}
				break;
			case RM_CONNECTION: // start monitoring a connection
				{
					oconnect *nu;
					WSANETWORKEVENTS events;

					// Restore the oconnect object by looking up the
					// OVERLAPPED structure.
					nu = (oconnect *)(overlapped::restore((OVERLAPPED*)msg.lParam));

					// Check if the object is connected before the message
					// reaches us.  Unlikely to happen, but still it can.
					if (WSAEnumNetworkEvents(nu->get_socket(), NULL, &events) != 0) {
						// Could not retreive socket status,
						// must be a serious error, die.
						delete nu;
						continue;
					}

					// The socket is connected, send it to the port.
					if ((events.lNetworkEvents & FD_CONNECT) != 0) {
						nu->psec = events.iErrorCode[FD_CONNECT_BIT];
						port.post(nu);
						continue;
					}

					// The connection is not yet ready, link it
					// to the list and continue waiting for events.
					nu->info().next = list;
					list = nu;
				}
				break;
			case RM_BOUNCE: // bounce the object.
				{
					overlapped *nu = (overlapped *)msg.lParam;
					nu->on_event(0, false);
				}
				break;
			}
		}
	}

	return 0;
}


// Emulator initializer.
rrid_t connect_ex_init()
{
	// Create the thread.
	if (CreateThread(NULL, 0, connect_ex_thread, NULL, 0, &dwThreadId) == NULL)
		return GetLastError() | REC_SYSERROR_MASK;

	return REC_SUCCESS;
}




bool iocp::connect(SOCKET &s, const sock_t &peer, OVERLAPPED *olp)
{
	struct sockaddr_in sa;

	if (peer.addr.length != 4) {
		SetLastError(REC_NOT_IMPLIMENTED);
		return false;
	}

	if ((s = port.socket()) == INVALID_SOCKET)
		return false;

	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = ADDR_ANY;
	sa.sin_port = 0;
	bind(s, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa));

	peer.put(sa);

	if (StartUp != NULL)
		WaitForSingleObjectEx(StartUp, INFINITE, TRUE);

	if (dwThreadId == 0) {
		SetLastError(REC_UNAVAILABLE);
		return false;
	}

	{
		unsigned long value = 1;
		if (ioctlsocket(s, FIONBIO, &value) != 0)
			return false;
	}

	if (WSAEventSelect(s, Job, FD_CONNECT))
		return false;

	if (WSAConnect(s, reinterpret_cast<struct sockaddr *>(&sa), sizeof(sa), NULL, NULL, NULL, NULL) && WSAGetLastError() != WSAEWOULDBLOCK)
		return false;

	if (PostThreadMessage(dwThreadId, RM_CONNECTION, (WPARAM)dwThreadId, (LPARAM)olp))
		return true;

	return false;
}


void iocp::post_ex(overlapped *olp)
{
	if (StartUp != NULL) {
		debug((flog, rl_iocp, "waiting on the start-up event (post_ex)"));
		WaitForSingleObjectEx(StartUp, INFINITE, TRUE);
		debug((flog, rl_iocp, "waiting on the start-up event is over (post_ex)"));
	}

	if (dwThreadId == 0) {
		SetLastError(REC_UNAVAILABLE);
		return;
	}

	PostThreadMessage(dwThreadId, RM_BOUNCE, (WPARAM)dwThreadId, (LPARAM)olp);
}
