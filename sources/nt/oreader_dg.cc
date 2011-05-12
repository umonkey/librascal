// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: oreader_dg.cc 1 2005-04-17 19:33:05Z vhex $
//
// Overlapped datagram reading.

#include "connection_dg.h"
#include "iocp.h"

oreader_dg::oreader_dg(connection_dg *host) :
	overlapped(host)
{
	port.post_ex(this);
}


oreader_dg::~oreader_dg()
{
}


void oreader_dg::on_event(unsigned int, bool)
{
	WSABUF buf;
	DWORD dummy = 0;

	buf.buf = info().data;
	buf.len = sizeof(info().data);

	info().from_length = sizeof(info().from_addr);

	if (WSARecvFrom(get_socket(), &buf, 1, &dummy, &dummy, &info().from_addr, &info().from_length, get_base(), oreader_dg::callback) != SOCKET_ERROR || WSAGetLastError() == WSA_IO_PENDING) {
		lma = true;
		return;
	}

	host->cancel();
}


void oreader_dg::callback(DWORD dwError, DWORD dwTransferred, WSAOVERLAPPED *olp, DWORD)
{
	oreader_dg *r = reinterpret_cast<oreader_dg *>(reinterpret_cast<char *>(olp) - 4);

	if (dwError != 0 || (dwTransferred == 0 && port.get_mode(r->get_socket()) != SOCK_DGRAM)) {
		r->host->cancel();
	} else {
		sock_t from(r->info().from_addr);
		if (from == r->get_peer())
			r->host->on_read(r->info().data, dwTransferred);
	}

	delete r;
}
