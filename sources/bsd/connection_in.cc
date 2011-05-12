// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connection_in.cc 1 2005-04-17 19:33:05Z vhex $

#ifndef _WIN32
# include <sys/types.h>
# include <netinet/in.h>
# include <sys/socket.h>
#endif
#include <errno.h>
#include <string.h>
#include "connection.h"
#include "../common/debug.h"

connection_in::connection_in(rascal_dispatcher _disp, void *_ctx) :
	connection(_disp, _ctx)
{
}


connection_in::~connection_in()
{
}


rrid_t connection_in::connect(const sock_t &_peer)
{
	struct sockaddr sa;

	peer = _peer;
	peer.put(sa);

	if (::bind(fd, &sa, sizeof(sa)) == -1) {
		debug((flog, rl_conn, "%X: could not bind to the specified address: %s.", get_id(), strerror(errno)));
		return errno | REC_SYSERROR_MASK;
	}

	if (::listen(fd, 5) == -1 && errno != EINPROGRESS) {
		debug((flog, rl_conn, "%X: could not spawn a listener", get_id()));
		return errno | REC_SYSERROR_MASK;
	}

	mode = cm_accept;
	debug((flog, rl_conn, "%X: created a listener on socket %d", get_id(), fd));

	return REC_SUCCESS;
}


bool connection_in::need_to_write()
{
	return true;
}


void connection_in::on_read()
{
	struct sockaddr sa;
	socklen_t salen = sizeof(sa);

	debug((flog, rl_conn, "%X: on_read on a listening connection", get_id()));

	for (int s = accept(fd, &sa, &salen); s != -1; s = accept(fd, &sa, &salen)) {
		sock_t peer(sa);
		connection *conn;

		debug((flog, rl_conn, "%X: accepted socket %d", get_id(), s));

		conn = new connection(disp, context, s);
		if (!disp(conn->get_id(), &peer, rop_accept, context))
			delete conn;
	}
}


void connection_in::on_write()
{
	debug((flog, rl_conn, "%X: on_write on a listening connection", get_id()));
}
