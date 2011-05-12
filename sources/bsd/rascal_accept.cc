// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_accept.cc 1 2005-04-17 19:33:05Z vhex $

#include <errno.h>
#include "connection.h"
#include "../common/debug.h"

rrid_t rascal_accept(const sock_t *peer, rascal_dispatcher disp, void *context)
{
	rrid_t rid;
	connection *tmp = new connection_in(disp, context);

	if (tmp == NULL) {
		debug((flog, rl_accept, "could not allocate a new connection_in object."));
		return errno | REC_SYSERROR_MASK;
	}

	if (!rascal_isok(rid = tmp->connect(*peer))) {
		debug((flog, rl_accept, "connection_in::connect(%s:%u) failed with code %x (%s).", rascal::ntoa(*peer).c_str(), peer->port, rid, rascal::errmsg(rid).c_str()));
		delete tmp;
		return rid;
	}

	if (!disp(tmp->get_id(), peer, rop_listen, context)) {
		debug((flog, rl_accept, "the listener at %s:%u was cancelled by the event handler.", rascal::ntoa(*peer).c_str(), peer->port));
		delete tmp;
		return REC_CANCELLED;
	}

	debug((flog, rl_accept, "installed a listener at %s:%u, id: %X", rascal::ntoa(*peer).c_str(), peer->port, tmp->get_id()));
	return tmp->get_id();
}
