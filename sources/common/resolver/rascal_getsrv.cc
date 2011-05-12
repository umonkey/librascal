// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_getsrv.cc 10 2005-04-18 13:38:26Z vhex $

#include <string.h>
#include "../util/random.h"
#include "../util/string.h"
#include "resolver.h"
#include "../debug.h"

IMPLEMENT_ALLOCATORS(getsrv::srv);
IMPLEMENT_ALLOCATORS(getsrv::ina);

unsigned int getsrv::srv::depth() const
{
	unsigned int count = 1;

	for (srv *item = next; item != NULL; item = item->next)
		++count;

	return count;
}


unsigned int getsrv::ina::depth() const
{
	unsigned int count = 1;

	for (ina *item = next; item != NULL; item = item->next)
		++count;

	return count;
}


getsrv::getsrv(const char *hostname, rascal_dispatcher disp, void *context, rascal_rcs_filter filter, rrid_t &rc)
{
	get_context() = context;
	get_disp() = disp;
	get_filter() = filter;

	set_srv(NULL);
	set_ina(NULL);

	strlcpy(data, hostname, sizeof(data));

	if (!rascal_isok(rc = flush()))
		request::cancel();
	else
		rc = get_id();
}


getsrv::~getsrv()
{
	for (srv *tmp = get_srv(), *next; tmp != NULL; tmp = next) {
		next = tmp->next;
		delete tmp;
	}

	for (ina *tmp = get_ina(), *next; tmp != NULL; tmp = next) {
		next = tmp->next;
		delete tmp;
	}
}


bool getsrv::dump(char *&dst, unsigned int &size)
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
	if (!hdr.put(dst, size, (unsigned short)header::TYPE_SRV))
		return false;
	if (!hdr.put(dst, size, (unsigned short)header::CLASS_IN))
		return false;

	debug((flog, rl_resolver, "%X: built a getsrv dns packet %u (%s):", get_id(), hdr.id, data));
	debug((flog, rl_resolver, "%X:   -- qr=%u, opcode=%u, aa=%u, tc=%u, rd=%u, ra=%u, z=%u", get_id(), hdr.qr, hdr.opcode, hdr.aa, hdr.tc, hdr.rd, hdr.ra, hdr.z));
	debug((flog, rl_resolver, "%X:   -- rcode=%u, qdcount=%u, ancount=%u, nscount=%u, arcount=%u", get_id(), hdr.rcode, hdr.qdcount, hdr.ancount, hdr.nscount, hdr.arcount));

	return true;
}

bool getsrv::on_record(header &hdr, const char *src, unsigned int size, rrid_t &rc)
{
	srv nu, *tmp;
	char host[256];

	if (!hdr.get(src, size, nu.priority))
		return false;
	if (!hdr.get(src, size, nu.weight))
		return false;
	if (!hdr.get(src, size, nu.port))
		return false;
	if (!hdr.get(src, size, host, sizeof(host)))
		return false;

	// RFC 2782 says that a "." in the host name means
	// that the service is unavailable.  Report so.
	if (host[0] == 0) {
		rc = REC_SRV_UNAVAIL;
		return false;
	}

	if ((tmp = new srv(nu)) == NULL)
		return false;

	tmp->next = get_srv();
	tmp->id = tmp->depth();
	strlcpy(tmp->host, host, sizeof(tmp->host));

	// Check if the host name is not being resolved already.
	for (srv *t = get_srv(); t != NULL; t = t->next) {
		if (strcmp(t->host, host) == 0) {
			set_srv(tmp);
			debug((flog, rl_resolver, "%X: hostname %s is already being resolved.", get_id(), host));
			return true;
		}
	}

	if (rascal_isok(rascal_getaddr(host, on_getaddr, (void *)(unsigned int)((tmp->id << 16) | id)))) {
		set_srv(tmp);
		debug((flog, rl_resolver, "%X: priority: %u, weight: %u, port: %u, name: %s, scheduled as: %u", get_id(), nu.priority, nu.weight, nu.port, host, tmp->id));
	} else {
		delete tmp;
		debug((flog, rl_resolver, "%X: request to resolve %s failed", get_id(), host));
	}

	debug((flog, rl_resolver, "%X: a service record processed.", get_id()));
	return true;
}

void getsrv::on_event(header &hdr, const char *src, unsigned int size)
{
	rrid_t rc = REC_SUCCESS;

	debug((flog, rl_resolver, "%X: dispatching a getsrv response.", get_id()));

	if (hdr.skip_echo(src, size)) {
		unsigned int _ttl;
		unsigned short _type, _class, _length;

		// Extracts responses.
		for (unsigned int idx = 0; idx < hdr.ancount && rascal_isok(rc); ++idx) {
			if (!hdr.skip_name(src, size))
				break;

			if (!hdr.get(src, size, _type))
				break;
			if (!hdr.get(src, size, _class))
				break;
			if (!hdr.get(src, size, _ttl))
				break;
			if (!hdr.get(src, size, _length))
				break;

			if (_length > size)
				break;

			if (_type == header::TYPE_SRV && _class == header::CLASS_IN && !on_record(hdr, src, size, rc))
				break;

			src += _length;
			size -= _length;
		}
	}

	// No requests schedulled, resolution failed.
	if (get_srv() == NULL) {
		debug((flog, rl_resolver, "%X: could not find SRV records in the response.", get_id()));
		get_disp()(rascal_isok(rc) ? REC_SRV_NOTFOUND : rc, sock_t(), rop_connect, context);
		request::cancel();
	} else {
		debug((flog, rl_resolver, "%X: found some SRV records, resolving host names.", get_id()));
	}
}


bool getsrv::try_next_server_disp(rrid_t rid, const sock_t *peer, int event, void *context)
{
	getsrv *req = reinterpret_cast<getsrv *>(request::pick((unsigned int)context));

	if (req == NULL) {
		debug((flog, rl_resolver, "unknown context %p, rejecting.", context));
		return false;
	}

	if (event != rop_connect) {
		debug((flog, rl_resolver, "%X: bad event (%s) for %p, rejecting.", req->get_id(), get_event_name(event), context));
		if (rascal_isok(rid))
			rascal_cancel(rid);
		return false;
	}

	if (rascal_isok(rid)) {
		bool rc;
		debug((flog, rl_resolver, "%X: connected to %s:%u, redispatching.", req->get_id(), rascal::ntoa(peer->addr).c_str(), peer->port));
		rascal_set_dispatcher(rid, req->get_disp());
		rascal_set_context(rid, req->get_context());
		rc = req->get_disp()(rid, peer, event, req->get_context());
		delete req;
		return rc;
	}

	debug((flog, rl_resolver, "%X: connection failed: %s.", req->get_id(), rascal::errmsg(rid).c_str()));
	debug((flog, rl_resolver, "%X: moving to the next service record.", req->get_id()));

	req->try_next_server();
	return false;
}


// Sorts the list according to the weight sorting rules defined in RFC2782.
// The list is expected to already have been sorted by priority, we only
// arrange it by element weight here.
void getsrv::sort()
{
	ina *head = get_ina(), *nlist = NULL, **tail = &nlist;

	debug((flog, rl_resolver, "%X: sorting the list of addresses.", get_id()));

	while (head != NULL) {
		unsigned int rnd = 0;
		unsigned short priority = head->priority;

		// Overall weight of the current priority.
		for (ina *item = head; item != NULL && item->priority == priority; item = item->next)
			rnd += item->weight;

		// Find a random value.
		rnd = rnd ? faeutil_random(rnd - 1) : 0;

		// Find the first suitable element.
		for (ina **item = &head; *item != NULL && (*item)->priority == priority; rnd -= (*item)->weight, item = &(*item)->next) {
			if (rnd == 0 || rnd < (*item)->weight) {
				// Add to the new list.
				*tail = *item;
				// Remove from the old list.
				*item = (*item)->next;
				// End searching for the element.
				break;
			}
		}

		// We have not found anything suitable; unsure how this can
		// be, but to prevent us from entering an infinite loop, move
		// the current element to the new list.
		if (*tail == NULL) {
			// Insert to the new list.
			*tail = head;
			// Remove from the old list.
			head = head->next;
		}

		// Skip the added element.
		if (*tail != NULL) {
			tail = &(*tail)->next;
			*tail = NULL;
		}
	}

	set_ina(nlist);
}


void getsrv::try_next_server()
{
	if (get_ina() == NULL) {
		debug((flog, rl_resolver, "%X: reporting service unavailability.", get_id()));
		get_disp()(REC_SRV_UNAVAIL, sock_t(), rop_connect, context);
		request::cancel();
	} else {
		bool try_next = true;
		ina *active = get_ina();

		set_ina(active->next);

		if (get_filter() == NULL || get_filter()(context, &active->peer)) {
#if defined(_DEBUG)
			{
				char tmp[64];
				rascal_ntoa(&active->peer.addr, tmp, sizeof(tmp));
				debug((flog, rl_resolver, "%X: attempting to connect to %s:%u", get_id(), tmp, active->peer.port));
			}
#endif
			if (rascal_isok(rascal_connect(&active->peer, try_next_server_disp, (void*)(unsigned int)id))) {
				debug((flog, rl_resolver, "%X: connection schedulled, waiting", get_id()));
				try_next = false;
			} else {
				debug((flog, rl_resolver, "%X: connection rejected, moving to the next one", get_id()));
			}
		} else {
			debug((flog, rl_resolver, "%X: record %s:%u rejected by the filter", get_id(), rascal::ntoa(active->peer).c_str(), active->peer.port));
		}

		delete active;

		if (try_next)
			try_next_server();
	}
}

// Add resolved addresses to all SRV records with the resolved host name.
void getsrv::on_getaddr(unsigned int addrc, const addr_t *addrv, unsigned short id)
{
	srv *base = NULL, *head = NULL;

	debug((flog, rl_resolver, "%X: dispatching an alias for request %u.", get_id(), id));

	// Find the corresponding SRV record.
	for (srv *item = get_srv(); item != NULL; item = item->next) {
		if (item->id == id) {
			base = item;
			break;
		}
	}

	if (base == NULL) {
		debug((flog, rl_resolver, "%X: service request %d not found, aliases discarded.", get_id(), id));
		return;
	}

	// Now apply the aliases to all matching records.
	for (srv *tmp = get_srv(), *next; tmp != NULL; tmp = next) {
		next = tmp->next;

		if (strcmp(tmp->host, base->host) != 0) {
			tmp->next = head;
			head = tmp;
		} else {
			// Insert all resolved aliases.
			for (unsigned int idx = 0; idx < addrc; ++idx) {
				ina *nu = new ina;
				if (nu != NULL) {
					ina *head = get_ina();

					nu->priority = tmp->priority;
					nu->weight = tmp->weight;
					nu->peer.port = tmp->port;
					nu->peer.addr = addrv[idx];

					for (ina **old = &head; ; old = &(*old)->next) {
						// Skip higher priorities.
						if (*old != NULL && (*old)->priority < nu->priority)
							continue;

						nu->next = *old;
						*old = nu;
						break;
					}

					set_ina(head);
				}
			}

			delete tmp;
		}
	}

	set_srv(head);

	if (head == NULL) {
		sort();
#if defined(_DEBUG)
		if (get_ina() != NULL) {
			unsigned int idx = 0, all = get_ina()->depth();
			for (ina *item = get_ina(); item != NULL; item = item->next) {
				char host[64];
				rascal_ntoa(&item->peer.addr, host, sizeof(host));
				debug((flog, rl_resolver, "%X:  ?? %02u/%02u. %s:%u, p=%u, w=%u\n", get_id(), ++idx, all, host, item->peer.port, item->priority, item->weight));
			}
		}
#endif
		try_next_server();
	}
}

void getsrv::on_getaddr(void *context, const char *, unsigned int addrc, const addr_t *addrv)
{
	unsigned short id = ((unsigned int)context) & 0xFFFF;
	request *req = request::pick(id);
	if (req != NULL) {
		reinterpret_cast<getsrv *>(req)->on_getaddr(addrc, addrv, ((unsigned int)context) >> 16);
	}
}

// The average line length for this file is astonishing.
// vim:ts=2:ss=2:sw=2
