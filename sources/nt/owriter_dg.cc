// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: owriter_dg.cc 1 2005-04-17 19:33:05Z vhex $
//
// The problem with datagram packets is that 1) we must access them with
// WSASendTo, which fail if the socket is bound to a completion port, and
// 2) we must start writing from a thread that works with the port, not
// from the caller's thread.  Hillarious, but doing otherwise will require
// the caller to enter the alertable wait from time to time, which is a
// requirement that we can't afford.  Thus we post requests to the port
// and start reading from within the event handler.  Looks awful, but that's
// the only way to make it work as it should.

#include "connection_dg.h"
#include "../common/datachain.h"
#include "iocp.h"
#include "../common/debug.h"

owriter_dg::owriter_dg(connection_dg *host, bool force_idle) :
	overlapped(host)
{
	debug((flog, rl_conn, "%X: owriter_dg: locking the mutex", host->get_id()));
	get_mutex().enter();

	if (force_idle)
		is_writing() = false;

	// If the connection is already being written to, drop
	// this request without any notification.
	if (is_writing()) {
		get_mutex().leave();
		delete this;
		return;
	}

	info().length = sizeof(info().data);
	get_wrb()->extract(info().data, info().length);

	if (info().length != 0)
		is_writing() = true;

	get_mutex().leave();

	if (info().length == 0) {
		delete this;
		return;
	}

	debug((flog, rl_conn, "%X: owriter_dg: posting", host->get_id()));
	port.post_ex(this);
}


owriter_dg::~owriter_dg()
{
}


void owriter_dg::on_event(unsigned int, bool)
{
	WSABUF buf;
	DWORD dummy = 0;
	struct sockaddr sa;

	buf.buf = info().data;
	buf.len = info().length;

	get_peer().put(sa);

	if (WSASendTo(get_socket(), &buf, 1, &dummy, 0, &sa, sizeof(sa), get_base(), callback) != SOCKET_ERROR || WSAGetLastError() == WSA_IO_PENDING) {
#if defined(DUMP_PACKETS)
		static unsigned int lastid = 0;
		rascal_dumpfile(true, info().data, info().length, "resolver-packet-%05u.w", ++lastid);
#endif
		lma = true;
		return;
	}

	host->cancel();
}


void owriter_dg::callback(DWORD dwError, DWORD dwTransferred, WSAOVERLAPPED *olp, DWORD)
{
	owriter_dg *r = reinterpret_cast<owriter_dg *>(reinterpret_cast<char *>(olp) - 4);

	if (dwError != 0)
		r->host->cancel();
	else {
		r->host->on_write(dwTransferred);
	}

	delete r;
}
