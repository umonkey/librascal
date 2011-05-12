// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_accept_service.cc 10 2005-04-18 13:38:26Z vhex $
//
// Installing the service is done by series of tricks.
//
// First, we pretend that we connect to a service, but the filter will
// decline all attempts, attempting to accept on that socket.  Events
// from the listeners are routed to a local event router, which replaces
// the request id with the original one and calls the user handler.

#include <string.h>
#include "common.h"
#include "util/list.h"
#include "object.h"
#include "debug.h"

// Request information.
struct ris_data
{
	// Request handle.
	rrid_t rid;
	// Original context.
	void *context;
	// Original handler.
	rascal_dispatcher disp;
};

// The filter acts as an event router this time.
static bool __rascall ris_filter(void *context, const sock_t *addr)
{
	rrid_t rid;
	ris_data *ris = reinterpret_cast<ris_data *>(context);

	debug((flog, rl_accept_svc, "%x: attempting to listen on %s:%u.", ris->rid, rascal::ntoa(*addr).c_str(), addr->port));

	if (rascal_isok(rid = rascal_accept(addr, ris->disp, ris->context))) {
		/*
		debug((flog, rl_accept_svc, "%x: delivering a rop_listen event.", ris->rid));
		if (!ris->disp(rid, addr, rop_listen, ris->context)) {
			debug((flog, rl_accept_svc, "%x: service declined by the event handler.", ris->rid));
			rascal_cancel(rid);
		} else {
			debug((flog, rl_accept_svc, "%x: service approved by the event handler.", ris->rid));
		}
		*/
	} else {
		debug((flog, rl_accept_svc, "%x: could not start accepting on %s.%u: %s.", ris->rid, rascal::ntoa(*addr).c_str(), addr->port, rascal::errmsg(rid).c_str()));
	}

	debug((flog, rl_accept_svc, "%x: bouncing the address.", ris->rid));
	return false;
}

static bool __rascall ris_disp(rrid_t, const sock_t *, int, void *context)
{
	ris_data *ris = reinterpret_cast<ris_data *>(context);
	debug((flog, rl_accept_svc, "%x: deleting the temporary request.", ris->rid));
	delete ris;
	return false;
}

rrid_t rascal_accept_service(const char *name, const char *proto, const char *domain, rascal_dispatcher disp, void *context)
{
	rrid_t rid;
	ris_data *ris;

	if (proto == NULL)
		proto = "tcp";
	else if (strcmp(proto, "tcp") != 0) {
		debug((flog, rl_accept_svc, "service accept failed: unsupported protocol: %s.", proto));
		return REC_BAD_PROTOCOL;
	}

	ris = new ris_data;
	ris->context = context;
	ris->disp = disp;

	if (!rascal_isok(rid = ris->rid = rascal_connect_service(name, proto, domain, ris_disp, ris, ris_filter))) {
		debug((flog, rl_accept_svc, "service accept failed: %x: %s", rid, rascal::errmsg(rid).c_str()));
		delete ris;
	} else {
		debug((flog, rl_accept_svc, "%x: service accept started (%s/%s/%s).", rid, domain, proto, name));
	}

	return rid;
}
