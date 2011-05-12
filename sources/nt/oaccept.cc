// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: oaccept.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection_st.h"
#include "iocp.h"
#include <mswsock.h>

oaccept::oaccept(connection *host, rrid_t &rc) :
	overlapped(host)
{
	if ((info().s = port.socket()) == INVALID_SOCKET) {
		rc = GetLastError() | REC_SYSERROR_MASK;
		delete this;
		return;
	}

	if (!AcceptEx(get_socket(), info().s, info().peerdata, 0, sizeof(SOCKADDR) * 2, sizeof(SOCKADDR) * 2, NULL, get_base()) && GetLastError() != ERROR_IO_PENDING) {
		rc = GetLastError() | REC_SYSERROR_MASK;
		delete this;
		return;
	}

	rc = REC_SUCCESS;
}


oaccept::~oaccept()
{
	if (info().s != INVALID_SOCKET)
		port.close(info().s);
}


void oaccept::on_event(unsigned int, bool failed)
{
	// Respawn the acceptor so that we could just return later.
	{
		rrid_t rc;
		new oaccept(host, rc);
	}

	if (setsockopt(info().s, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&get_socket(), sizeof(get_socket())) != 0)
		return;

	// Make sure the application wants this client.
	if (!failed && psec == 0) {
		sock_t peer;
		connection_st *conn = new connection_st(get_disp(), get_context());

		get_peer_name(peer);
		conn->on_accept(peer, info().s);
		info().s = INVALID_SOCKET;
	}
}


void oaccept::get_peer_name(sock_t &peer)
{
	int slocal, sremote;
	SOCKADDR_IN *local, *remote;

	GetAcceptExSockaddrs(info().peerdata, 0, sizeof(SOCKADDR) * 2, sizeof(SOCKADDR) * 2, reinterpret_cast<SOCKADDR**>(&local), &slocal, reinterpret_cast<SOCKADDR**>(&remote), &sremote);

	peer.get(*remote);
}
