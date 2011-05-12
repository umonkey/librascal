// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connection_dg.cc 1 2005-04-17 19:33:05Z vhex $

#include <string.h>
#include <errno.h>
#ifndef _WIN32
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <unistd.h>
#endif

#include "connection.h"
#include "../common/datachain.h"
#include "../common/debug.h"

connection_dg::connection_dg(rascal_dispatcher _disp, void *_ctx) :
	connection(SOCK_DGRAM)
{
	disp = _disp;
	context = _ctx;
	rd = new packetchain;
	wr = new packetchain;
	selected = false;

	debug((flog, rl_conn, "%X: connection created (datagram, context: %x)", get_id(), _ctx));
}


void connection_dg::on_error(int psec)
{
	debug((flog, rl_conn, "%X: reporting a connection error with code %d", psec));
	disp(psec | REC_SYSERROR_MASK, &peer, rop_close, context);
	cancel();
}


void connection_dg::on_read()
{
	bool success = false;

	mx.enter();

	if (is_cancelled()) {
		debug((flog, rl_conn, "%X: ignored a read attempt for a datagram connection (already cancelled)", get_id()));
		mx.leave();
		return;
	}

	while (true) {
		char tmp[4096];
		struct sockaddr from;
		socklen_t fromlen = sizeof(from);
		int length = recvfrom(fd, tmp, sizeof(tmp), 0, &from, &fromlen);

		if (length < 0) {
			mx.leave();
			if (errno != EWOULDBLOCK && errno != EAGAIN)
				on_error(errno);
			break;
		}

		if (length == 0 || peer != sock_t(from)) {
			mx.leave();
			break;
		}

		success = true;
		rd->append(tmp, length);
	}

	if (success) {
		debug((flog, rl_conn, "%X: reporting data availability", get_id()));
		disp(get_id(), &peer, rop_read, context);
	}
}


void connection_dg::on_write()
{
	bool success = false;

	mx.enter();

	if (is_cancelled()) {
		mx.leave();
		return;
	}

	while (true) {
		char tmp[4096];
		struct sockaddr to;
		unsigned int size = sizeof(tmp);

		peer.put(to);

		wr->peek(tmp, size);

		if (size == 0) {
			mx.leave();
			break;
		}

		if (sendto(fd, tmp, size, 0, &to, sizeof(to)) < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
			mx.leave();
			on_error(errno);
			break;
		}

		success = true;
		wr->remove(size);
	}
}


rrid_t connection_dg::connect(const sock_t &_peer)
{
	struct sockaddr_in sa;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(0);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(fd, reinterpret_cast<sockaddr *>(&sa), sizeof(sa)) != 0)
		return GetSysError;

	peer = _peer;

	debug((flog, rl_conn, "%X: reporting a successfull datagram connection", get_id()));

	if (!disp(get_id(), &peer, rop_connect, context))
		return REC_UNKNOWN_ERROR;

	return REC_SUCCESS;
}
