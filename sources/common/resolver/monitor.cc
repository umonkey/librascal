// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: monitor.cc 6 2005-04-17 21:31:05Z vhex $
//
// This file implements the DNS monitor, a thread that resends queries that
// haven't been replied in a certain period of time.

#include <errno.h>
#include <stdlib.h>
#ifdef _DEBUG
# include <string.h> // for strerror()
#endif
#ifdef _WIN32
# define usleep Sleep
#else
# include <unistd.h>
#endif
#include "../util/sem.h"
#include "../util/ftspec.h"
#include "resolver.h"
#include "../debug.h"

static sem msem;

bool request::cycle_monitor(request *queue)
{
	bool rc = false;
	ftspec until(0), now(0);
	long int timeout, repeat;

	now.update();

	// These values may have changed; renew every time.
	rascal_get_option(RO_DNS_RETRY, &repeat);
	rascal_get_option(RO_DNS_TIMEOUT, &timeout);

	mx.enter();

	for (request **tail = &queue, **next; *tail != NULL; tail = next) {
		ftspec now;
		request *item = *tail;

		next = &(*tail)->next;

		if (item->is_cancelled())
			continue;

		rc = true;

		now.update();

		// Calculate the request's current TTL.
		ftspec ttl = item->tstamp + item->rcount * timeout;

		// Expired, need to resend.
		if (now > ttl) {
			if (static_cast<long int>(item->rcount) >= repeat) {
				debug((flog, rl_resolver, "* monitor: request %d timed out, cancel (no reply in %u msec).\n", item->id, (now - item->tstamp).mseconds()));

				// We may not leave the queue locked
				// during calling the user callback
				// function, so we temporarily unlock
				// it and then restart from the head
				// of the list.

				mx.leave();
				item->cancel();
				mx.enter();
				continue;
			}

			debug((flog, rl_resolver, "%X: request %d is being resent by the monitor (attempt nr.%u)", item->get_id(), item->id, item->rcount + 1));
			mx.leave();
			debug((flog, rl_resolver, "%X: flushing...", item->get_id()));
			item->flush();
			debug((flog, rl_resolver, "%X: flushed", item->get_id()));
			mx.enter();
		}

		ttl = item->tstamp + item->rcount * timeout;
		if (until.sec == 0 || ttl < until)
			until = ttl;
	}

	mx.leave();

	if (until > now) {
		debug((flog, rl_resolver, "* monitor: going to sleep for %u msec.\n", (until - now).mseconds()));
		if (msem.wait(until)) {
			debug((flog, rl_resolver, "* monitor: slept enough.\n"));
		} else if (errno != ETIMEDOUT) {
			debug((flog, rl_resolver, "* monitor: wait() failure (errno=%d, \"%s\"), throttling.\n", errno, strerror(errno)));
			usleep(100);
		}
	}

	return rc;
}


void * request::monitor(void *)
{
	while (true) {
		msem.wait();

		debug((flog, rl_resolver, "* monitor: got job.\n"));
		for (bool rc = true; rc; rc = cycle_monitor(queue));
		debug((flog, rl_resolver, "* monitor: cycle over.\n"));
	}

	return 0;
}


void request::wake_monitor()
{
	debug((flog, rl_resolver, "* monitor: posting a wake-up signal.\n"));
	msem.post();
	debug((flog, rl_resolver, "* monitor: posted ok.\n"));
}
