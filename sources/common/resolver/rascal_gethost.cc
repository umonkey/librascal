// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_gethost.cc 2 2005-04-17 21:09:06Z vhex $

#include "resolver.h"
#include "../debug.h"

gethost::gethost(const addr_t *addr, callback cb, void *_context, rrid_t &rc)
{
	get_addr() = *addr;
	get_disp() = cb;
	context = _context;

	if (!rascal_isok(rc = flush()))
		delete this;
	else
		rc = get_id();
}


void gethost::on_event(header &hdr, const char *src, unsigned int size)
{
	hostname_t hosts[16];
	const char *hostv[16];
	unsigned int count;

	debug((flog, rl_resolver, "* gethost event, request lifetime: %u msec.\n", (ftspec() - tstamp).mseconds()));

	count = hdr.get_hosts(src, size, hosts, dimof(hosts));

#ifdef _DEBUG
	if (count == 0) {
		debug((flog, rl_resolver, "  -- no replies found.\n"));
	}

	for (unsigned int idx = 0; idx < count; ++idx) {
		debug((flog, rl_resolver, "  -- %02u/%02u. %s\n", idx+1, count, hosts[idx].data));
	}
#endif

	for (unsigned int idx = 0; idx < count; ++idx)
		hostv[idx] = hosts[idx].data;

	get_disp()(context, &get_addr(), count, hostv);
	request::cancel();
}


void gethost::cancel()
{
	get_disp()(context, &get_addr(), 0, NULL);
	request::cancel();
}


bool gethost::dump(char *&dst, unsigned int &size)
{
	header hdr(id);

	// Query count.
	hdr.qdcount = 1;

	if (!hdr.dump(dst, size))
		return false;

	// Insert the domain name.
	if (!hdr.put(dst, size, get_addr()))
		return false;

	// Insert the type of the query.
	if (!hdr.put(dst, size, (unsigned short)header::TYPE_PTR))
		return false;
	if (!hdr.put(dst, size, (unsigned short)header::CLASS_IN))
		return false;

	debug((flog, rl_resolver, "%X: built a gethost dns packet %u, qr=%u, opcode=%u, aa=%u, tc=%u, rd=%u, ra=%u, z=%u, rcode=%u, qdcount=%u, ancount=%u, nscount=%u, arcount=%u", get_id(), hdr.id, hdr.qr, hdr.opcode, hdr.aa, hdr.tc, hdr.rd, hdr.ra, hdr.z, hdr.rcode, hdr.qdcount, hdr.ancount, hdr.nscount, hdr.arcount));
	return true;
}


rrid_t rascal_gethost(const addr_t *addr, rascal_gethost_callback cb, void *context)
{
	rrid_t rid;
	new gethost(addr, cb, context, rid);
	return rid;
}
