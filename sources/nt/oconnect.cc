// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: oconnect.cc 1 2005-04-17 19:33:05Z vhex $
//
// The overlapped connection class.  When an outgoing connection
// is requested, two objects are created: connection and oconnect.

#include "connection_st.h"
#include "iocp.h"

oconnect::oconnect(connection *host, rrid_t &rc) :
	overlapped(host)
{
	if (!port.connect(get_socket(), get_peer(), get_base())) {
		rc = GetLastError() | REC_SYSERROR_MASK;
		delete this;
	} else {
		rc = REC_SUCCESS;
	}
}


oconnect::~oconnect()
{
}


void oconnect::on_event(unsigned int, bool failed)
{
	if (failed || psec != 0)
		host->cancel();
	else {
		setsockopt(get_socket(), SOL_SOCKET, 0x7010 /* SO_UPDATE_CONNECT_CONTEXT */, NULL, 0);
		host->on_connect();
	}
}
