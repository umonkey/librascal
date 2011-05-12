// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: request.cc 12 2005-04-18 13:52:09Z vhex $

#include "resolver.h"
#include "../debug.h"

IMPLEMENT_ALLOCATORS(request);

mutex request::mx;

request* request::queue = NULL;

unsigned short request::lastid = 0;


request::request()
{
	mlock lock(mx);
	request **tail = &queue;

	rcount = 0;
	context = NULL;
	disp = NULL;
	next = NULL;

	tstamp.update();

	do {
		++lastid;
	} while (lastid == 0);

	id = lastid;

	while (*tail != NULL)
		tail = &(*tail)->next;

	*tail = this;
	listed = true;

	incref();
}


request::~request()
{
	unlist();
	debug((flog, rl_resolver, "%X: packet %u destroyed", get_id(), id));
}


void request::unlist()
{
	if (listed) {
		mlock lock(mx);
		request **tail = &queue, *list = queue;

		debug((flog, rl_resolver, "%X: removing from the queue", get_id()));

		for (*tail = NULL; list != NULL; list = list->next) {
			if (list != this) {
				*tail = list;
				tail = &(*tail)->next;
			}
		}

		listed = false;

		*tail = NULL;
	}
}


rrid_t request::flush()
{
	unsigned int count = 0;

	char packet[512], *dst = packet;
	unsigned int size = sizeof(packet);

	if (!dump(dst, size))
		return REC_UNKNOWN_ERROR;

#if defined(DUMP_PACKETS)
	rascal_dumpfile(true, packet, sizeof(packet) - size, "resolver-packet-%05u.o", id);
#endif

	debug((flog, rl_resolver, "%X: writing packet %u", get_id(), id));
	for (unsigned int idx = 0; idx < dimof(ns_rids); ++idx)
		if (rascal_isok(ns_rids[idx]) && rascal_isok(rascal_write(ns_rids[idx], packet, sizeof(packet) - size)))
			++count;
	debug((flog, rl_resolver, "%X: wrote packet %u to %u nameservers", get_id(), id, count));

	if (count == 0)
		return REC_NO_NAMESERVERS;

	// Only wake up the monitor if we have sent a new request;
	// otherwise it is already monitoring the queue and there's
	// no need to disturb it.
	if (rcount++ == 0)
		wake_monitor();

	return REC_SUCCESS;
}


void request::on_event(header &, const char *, unsigned int)
{
	debug((flog, rl_resolver, "%X: unhandled event for packet %u", get_id(), id));
	delete this;
}


bool request::dump(char *&, unsigned int &)
{
	return false;
}


void request::on_read(rrid_t conn)
{
	header hdr;
	char packet[512];
	request *req = NULL;
	const char *src = packet;
	unsigned int size = sizeof(packet);

	if (!rascal_isok(rascal_read(conn, packet, &size)))
		return;

	if (!hdr.parse(src, size))
		return;

	debug((flog, rl_resolver, "* ns_on_read for packet %u.\n", hdr.id));
	debug((flog, rl_resolver, "  -- qr=%u, opcode=%u, aa=%u, tc=%u, rd=%u, ra=%u, z=%u.\n", hdr.qr, hdr.opcode, hdr.aa, hdr.tc, hdr.rd, hdr.ra, hdr.z));
	debug((flog, rl_resolver, "  -- rcode=%u, qdcount=%u, ancount=%u, nscount=%u, arcount=%u.\n", hdr.rcode, hdr.qdcount, hdr.ancount, hdr.nscount, hdr.arcount));

#if defined(DUMP_PACKETS)
	rascal_dumpfile(true, hdr.head, size + (src - hdr.head), "resolver-packet-%05u.i", hdr.id);
#endif

	if ((req = pick(hdr.id)) != NULL) {
		req->incref();
		req->on_event(hdr, src, size);
		req->decref();
	} else {
		debug((flog, rl_resolver, "  -- packet with id=%u not found", hdr.id));
	}
}


void request::on_write(rrid_t)
{
}


request* request::pick(unsigned short id)
{
	mlock lock(mx);

	for (request *item = queue; item != NULL; item = item->next) {
		if (item->id == id)
			return item;
	}

	return NULL;
}


void request::cancel(void)
{
	debug((flog, rl_resolver, "%X: packet %u cancelled", get_id(), id));
	unlist();
	object::cancel();
	decref();
}
