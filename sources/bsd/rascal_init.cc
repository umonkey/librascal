// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_init.cc 1 2005-04-17 19:33:05Z vhex $

#include <errno.h>
#include <stdlib.h>
#ifndef _WIN32
# include <sys/types.h>
# include <netinet/in.h>
# include <resolv.h>
#endif
#include "connection.h"
#include "../common/debug.h"

extern int fix_exports();

rrid_t rascal_init(unsigned int policy)
{
	unsigned int threads = 0;

	debug_init();

	if (!rascal_isok(rascal_set_option(RO_THREAD_POLICY, policy & RIP_WORKER_MASK))) {
		debug((flog, rl_misc, "could not initialize the library (policy: %u)", policy & RIP_WORKER_MASK));
		return REC_SUCCESS;
	}

	fix_exports();

	if ((policy & RIP_NO_DNS) == 0) {
		if (!rascal_initres())
			return REC_INER_RESOLVER;
		else {
#ifdef HAVE_res_init
			if (res_init() == 0) {
				rrid_t rc;
				sock_t ns_addr[64];

				rascal_set_option(RO_DNS_TIMEOUT, _res.retrans * 1000 / _res.retry);
				rascal_set_option(RO_DNS_RETRY, _res.retry);
				debug((flog, rl_resolver, "resolver timeout is %ld seconds, retry counter: %ld.\n", static_cast<long>(_res.retrans * 1000 / _res.retry), static_cast<long>(_res.retry)));

				for (int idx = 0; idx < _res.nscount; ++idx) {
					ns_addr[idx].get(_res.nsaddr_list[idx]);
					debug((flog, rl_resolver, "resolver %u of %u: %s#%u\n", idx+1, _res.nscount, rascal::ntoa(ns_addr[idx]).c_str(), ns_addr[idx].port));
				}

				if (!rascal_isok(rc = rascal_set_nameserver(ns_addr, _res.nscount))) {
					debug((flog, rl_resolver, "could not initialize the resolver (error %x)", rc));
					return rc;
				}
			} else {
				debug((flog, rl_resolver, "resolver unavailable: res_init() failed.\n"));
			}
#endif
		}
	}

	if ((policy & RIP_WORKER_MASK) != RIP_WORKER_MANUAL)
		threads = 1;

	for (unsigned int idx = 0; idx < threads; ++idx) {
		pthread_t thr;

		if (pthread_create(&thr, NULL, connection::worker, 0) != 0) {
			rrid_t rc = GetSysError;
			debug((flog, rl_misc, "pthread_create() failed while creating workers, initialization failed.", rc));
			return rc;
		}
	}

	debug((flog, rl_misc, "initialized the library with policy %u.", policy));

	return REC_SUCCESS;
}
