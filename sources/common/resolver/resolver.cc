// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: resolver.cc 2 2005-04-17 21:09:06Z vhex $

#include <errno.h>
#include "resolver.h"
#include "../debug.h"
#include "../util/random.h"

// Addresses of nameservers.
sock_t ns_addr[NAMESERVER_COUNT];

// Connection identifiers.
rrid_t ns_rids[NAMESERVER_COUNT];

// Resolver mode (SOCK_STREAM, SOCK_DGRAM).
const char ns_mode[] = "udp";

// Resolver dispatcher.  When a connection is accepted, the old one is
// forcibly closed; thus we do not allow multiple resolvers at this time.
bool ns_dispatcher(rrid_t conn, const sock_t *peer, int event, void *context)
{
	unsigned int nsid = (unsigned int)context;

	debug((flog, rl_resolver, "received a %s event for connection %X\n", get_event_name(event), conn));

	switch (event) {
	case rop_connect:
		if (rascal_isok(conn)) {
			ns_rids[nsid] = conn;
			ns_addr[nsid] = *peer;
		}
		break;
	case rop_close:
		if (rascal_isok(conn))
			ns_rids[nsid] = REC_UNAVAILABLE;
		else if (rascal_isok(ns_rids[nsid] = rascal_connect_ex(peer, ns_dispatcher, context, ns_mode)))
			ns_rids[nsid] = REC_UNAVAILABLE;
		break;
	case rop_read:
		request::on_read(conn);
		break;
	case rop_write:
		request::on_write(conn);
		break;
	}

	return true;
}


bool rascal_initres()
{
	pthread_t tid;

	faeutil_srandom();

	for (unsigned int idx = 0; idx < dimof(ns_rids); ++idx)
		ns_rids[idx] = REC_UNAVAILABLE;

	if (pthread_create(&tid, NULL, request::monitor, NULL) != 0) {
		debug((flog, rl_resolver, "could not spawn a monitor thread"));
		return false;
	}

	return true;
}
