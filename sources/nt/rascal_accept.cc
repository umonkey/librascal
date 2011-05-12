// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_accept.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection_st.h"
#include "../common/debug.h"

rrid_t rascal_accept(const sock_t *peer, rascal_dispatcher disp, void *context)
{
	rrid_t rc;
	connection_st *tmp = new connection_st(disp, context);

	if (tmp == NULL) {
		debug((flog, rl_interface, "could not install a listener: memory allocation failed"));
		return GetLastError() | REC_SYSERROR_MASK;
	}

	debug((flog, rl_interface, "%X: attempting to install a listener on %s:%u", tmp->get_id(), rascal::ntoa(*peer).c_str(), peer->port));

	if (!rascal_isok(rc = tmp->accept(*peer))) {
		debug((flog, rl_interface, "%X: listener not installed due to error %x: %s", tmp->get_id(), rc, rascal::errmsg(rc).c_str()));
		delete tmp;
		return rc;
	}

	debug((flog, rl_interface, "%X: listener installed", tmp->get_id()));
	return tmp->get_id();
}
