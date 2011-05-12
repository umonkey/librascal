// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: iocp.cc 1 2005-04-17 19:33:05Z vhex $

#include "../rascal.h"
#include "iocp.h"
#include "connection.h"

// The only port instance.
iocp port;

iocp::iocp()
{
	hPort = NULL;
}


iocp::~iocp()
{
	if (hPort != NULL)
		CloseHandle(hPort);
}


rrid_t iocp::init()
{
	rrid_t rc;
	protoent *pe;
	SYSTEM_INFO si;
	long int policy;
	DWORD dwThreadId;
	unsigned int count = 0;

	if (!rascal_isok(rc = rascal_get_option(RO_THREAD_POLICY, &policy)))
		return rc;

	if (hPort != NULL)
		return REC_UNKNOWN_ERROR;

	if ((pe = getprotobyname("tcp")) == NULL)
		return WSAGetLastError() | REC_SYSERROR_MASK;
	else
		proto_tcp = pe->p_proto;

	if ((pe = getprotobyname("udp")) == NULL)
		return WSAGetLastError() | REC_SYSERROR_MASK;
	else
		proto_udp = pe->p_proto;

	if ((hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)) == NULL)
		return false;

	if ((policy & RIP_WORKER_MASK) == RIP_WORKER_MANUAL)
		return REC_SUCCESS;

	GetSystemInfo(&si);

	if ((policy & RIP_WORKER_MASK) == RIP_WORKER_SINGLE)
		si.dwNumberOfProcessors = 1;

	for (unsigned int idx = 0; idx < si.dwNumberOfProcessors; ++idx) {
		HANDLE hThread = CreateThread(NULL, 0, Worker, (LPVOID)this, 0, &dwThreadId);
		if (hThread != NULL) {
			++count;
			SetThreadAffinityMask(hThread, 1 << idx);
		}
	}

	return count ? REC_SUCCESS : REC_INER_NOWORKERS;
}


SOCKET iocp::socket(int mode)
{
	SOCKET fd = WSASocket(AF_INET, mode, mode == SOCK_DGRAM ? proto_udp : proto_tcp, 0, 0, WSA_FLAG_OVERLAPPED);

	if (fd != INVALID_SOCKET) {
		unsigned long value = 1;
		if (ioctlsocket(fd, FIONBIO, &value) != 0) {
			close(fd);
			fd = INVALID_SOCKET;
		}
	}

	// Datagram sockets are being worked with WSARecvFrom and WSASendTo,
	// which fail with WSAEINVAL if the socket is in the completion port.
	// Sounds strange, but the only way to make it work is to exclude
	// this type of sockets from the port.
	if (mode != SOCK_DGRAM&& fd != INVALID_SOCKET && CreateIoCompletionPort((HANDLE)fd, hPort, 0, 0) == NULL)
		close(fd);

	return fd;
}


void iocp::close(SOCKET &s)
{
	if (s != INVALID_SOCKET)
		closesocket(s);
	s = INVALID_SOCKET;
}

int iocp::get_mode(SOCKET s)
{
	int mode, size = sizeof(mode);
	if (getsockopt(s, SOL_SOCKET, SO_TYPE, (char*)&mode, &size))
		mode = 0;
	return mode;
}


bool iocp::peek(unsigned int &transferred, overlapped *&ol, unsigned int delay)
{
	OVERLAPPED *olp;
	ULONG_PTR unused;
	bool failed = GetQueuedCompletionStatus(hPort, reinterpret_cast<DWORD*>(&transferred), &unused, &olp, delay) ? false : true;

	if (olp == NULL)
		ol = NULL;
	else
		ol = reinterpret_cast<overlapped *>(reinterpret_cast<char *>(olp) - 4);

	return failed;
}


void iocp::post(overlapped *ol)
{
	if (!PostQueuedCompletionStatus(hPort, 0, 0, ol->get_base())) {
		/* DEBUG(("* iocp: FAILED to post %p to port %p, LE=%ld.\n", ol, hPort, GetLastError())); */
	}
}


DWORD iocp::Worker(LPVOID lpPort)
{
	overlapped *ol;
	unsigned int transferred;
	iocp *port = reinterpret_cast<iocp *>(lpPort);

	while (true) {
		bool failed = port->peek(transferred, ol, INFINITE);

		// Nothing was dequeued.
		if (ol == NULL)
			continue;

		ol->on_event(transferred, failed);

		// Return home.
		if (!ol->lia())
			delete ol;
	}

	return 0;
}
