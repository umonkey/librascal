// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_getaddr.cc 10 2005-04-18 13:38:26Z vhex $
//
// This file contains all hostname-to-numeric resolution code.

#include <string.h>
#include "../util/string.h"
#include "resolver.h"
#include "../debug.h"

getaddr::getaddr(const char *hostname, callback cb, void *_context, rrid_t &rc)
{
	strlcpy(data, hostname, sizeof(data));
	context = _context;
	get_disp() = cb;

	if (!rascal_isok(rc = flush()))
		delete this;
	else
		rc = get_id();
}


void getaddr::on_event(header &hdr, const char *src, unsigned int size)
{
	addr_t addrs[16];
	unsigned int count;

	debug((flog, rl_resolver, "* getaddr event, request lifetime: %u msec.\n", (ftspec() - tstamp).mseconds()));

	count = hdr.get_addrs(src, size, addrs, sizeof(addrs) / sizeof(addrs[0]));

#ifdef _DEBUG
	if (count == 0) {
		debug((flog, rl_resolver, " -- no replies found.\n"));
	}

	for (unsigned int idx = 0; idx < count; ++idx) {
		char tmp[64];
		rascal_ntoa(&addrs[idx], tmp, sizeof(tmp));
		debug((flog, rl_resolver, "  -- %02u/%02u. %s\n", idx+1, count, tmp));
	}
#endif

	get_disp()(context, data, count, addrs);
	request::cancel();
}


void getaddr::cancel()
{
	get_disp()(context, data, 0, NULL);
	request::cancel();
}


bool getaddr::dump(char *&dst, unsigned int &size)
{
	header hdr(id);

	// Query count.
	hdr.qdcount = 1;

	if (!hdr.dump(dst, size))
		return false;

	// Insert the domain name.
	if (!hdr.put(dst, size, data))
		return false;

	// Insert the type of the query.
	if (!hdr.put(dst, size, (unsigned short)header::TYPE_A))
		return false;
	if (!hdr.put(dst, size, (unsigned short)header::CLASS_IN))
		return false;

	debug((flog, rl_resolver, "%X: built a getaddr dns packet %u (%s):", get_id(), hdr.id, data));
	debug((flog, rl_resolver, "%X:   -- qr=%u, opcode=%u, aa=%u, tc=%u, rd=%u, ra=%u, z=%u.", get_id(), hdr.qr, hdr.opcode, hdr.aa, hdr.tc, hdr.rd, hdr.ra, hdr.z));
	debug((flog, rl_resolver, "%X:   -- rcode=%u, qdcount=%u, ancount=%u, nscount=%u, arcount=%u.", get_id(), hdr.rcode, hdr.qdcount, hdr.ancount, hdr.nscount, hdr.arcount));

	return true;
}


rrid_t rascal_getaddr(const char *host, rascal_getaddr_callback cb, void *context)
{
	rrid_t rid;
	new getaddr(host, cb, context, rid);
	return rid;
}
