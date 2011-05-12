// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: oreader.cc 1 2005-04-17 19:33:05Z vhex $
//
// Overlapped reading.  The class functions almost on its own,
// reads into the internal buffer and then just delivers the
// data to the connection.

#include "connection_st.h"
#include "iocp.h"
#include "../common/debug.h"

oreader::oreader(connection *host) :
	overlapped(host)
{
	DWORD dummy = 0;

	if (ReadFile((HANDLE)get_socket(), data, sizeof(data), &dummy, get_base()) || GetLastError() == ERROR_IO_PENDING) {
		debug((flog, rl_conn, "%X: reading scheduled", host->get_id()));
		return;
	}

	debug((flog, rl_conn, "%X: could not schedule reading, closing the connection", host->get_id()));
	host->cancel();
	delete this;
}


oreader::~oreader()
{
}


void oreader::on_event(unsigned int transferred, bool failed)
{
	if (failed || psec != 0 || (transferred == 0 && port.get_mode(get_socket()) != SOCK_DGRAM)) {
		debug((flog, rl_conn, "%X: reading failed, closing up", host->get_id()));
		host->cancel();
	} else {
		host->on_read(data, transferred);
	}
}


void oreader::callback(DWORD dwError, DWORD dwTransferred, WSAOVERLAPPED *olp, DWORD)
{
	oreader *r = reinterpret_cast<oreader *>(reinterpret_cast<char *>(olp) - 4);
	r->on_event(dwTransferred, dwError ? true : false);
	delete r;
}
