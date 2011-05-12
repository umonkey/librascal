// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connection_dg.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection_dg.h"
#include "../common/datachain.h"
#include "iocp.h"
#include "../common/debug.h"

connection_dg::connection_dg(rascal_dispatcher _disp, void *_ctx)
{
	s = port.socket(SOCK_DGRAM);
	disp = _disp;
	context = _ctx;
	is_writing = false;

	rdb = new packetchain;
	wrb = new packetchain;
}


// We do not delete packet chains here; it's done by the base class.
connection_dg::~connection_dg()
{
}


void connection_dg::spawn_reader()
{
	new oreader_dg(this);
}


void connection_dg::spawn_writer(bool force_idle)
{
	new owriter_dg(this, force_idle);
}


rrid_t connection_dg::connect(const sock_t &_peer)
{
	struct sockaddr_in sa;

	peer = _peer;

	// Make sure the socket initialized ok.
	if (s == INVALID_SOCKET)
		return WSAEINVAL | REC_SYSERROR_MASK;

	// Bind the local endpoint.
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(0);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, reinterpret_cast<sockaddr *>(&sa), sizeof(sa)) != 0)
		return GetLastError() | REC_SYSERROR_MASK;

	// If the connection is rejected, we return an error but do NOT
	// delete the object.  This is until rascal_connect() uses a
	// wrapper (pobject) instead of a raw pointer that it then
	// deletes, too.

	debug((flog, rl_conn, "%X: delivering rop_connect", get_id()));
	if (!disp(get_id(), &peer, rop_connect, context)) {
		debug((flog, rl_conn, "%X: the connection was rejected", get_id()));
		return REC_UNKNOWN_ERROR;
	} else {
		debug((flog, rl_conn, "%X: the connection was accepted"));
		spawn_reader();
		return REC_SUCCESS;
	}
}


rrid_t connection_dg::accept(const sock_t &)
{
	return REC_NOT_IMPLIMENTED;
}
