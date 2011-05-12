// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_init.cc 21 2005-04-18 17:39:11Z vhex $

#include <stdlib.h>
#include <windows.h>
#include <iphlpapi.h>
#include "connection.h"
#include "iocp.h"
#include "../common/common.h"
#include "../common/debug.h"

extern rrid_t connect_ex_init(void);


static void ns_initres_get(IP_ADDR_STRING *ias)
{
	rrid_t rc;
	sock_t nslist[16];
	unsigned int nscount = 0;
	IP_ADDR_STRING *tmp;

	for (tmp = ias; tmp != NULL && nscount != dimof(nslist); tmp = tmp->Next, ++nscount) {
		if (rascal_aton(tmp->IpAddress.String, &nslist[nscount].addr)) {
			nslist[nscount].port = 53;
			debug((flog, rl_resolver, "resolver %02u of %02u: %s", nsidx, nscount, tmp->IpAddress.String));
		}
	}

	if (nscount == 0)
		return;

	if (rascal_isok(rc = rascal_set_nameserver(nslist, nscount))) {
		debug((flog, rl_resolver, "installed %u nameservers", nscount));
	} else {
		debug((flog, rl_resolver, "failed to install nameservers: %s", rascal::errmsg(rc).c_str()));
	}
}


static void nt_initres(void)
{
	ULONG fisize = 0;
	FIXED_INFO *fi = NULL;

	if (GetNetworkParams(fi, &fisize) == ERROR_BUFFER_OVERFLOW)
		fi = reinterpret_cast<FIXED_INFO *>(malloc(fisize));

	if (GetNetworkParams(fi, &fisize) != ERROR_SUCCESS) {
		debug((flog, rl_resolver, "could not retreive FIXED_INFO (error %ld)", GetLastError()));
	} else {
		ns_initres_get(&fi->DnsServerList);
		debug((flog, rl_resolver, "resolvers initialized"));
	}

	if (fi != NULL)
		free(fi);
}


rrid_t rascal_init(unsigned int policy)
{
	rrid_t rc;

	debug_init();

	if (!rascal_isok(rascal_set_option(RO_THREAD_POLICY, static_cast<long int>(policy))))
		return REC_SUCCESS;

	if (!rascal_isok(rc = port.init()))
		return rc;

	if (!rascal_isok(rc = connect_ex_init()))
		return rc;

	if (!rascal_initres())
		return REC_INER_RESOLVER;

	nt_initres();

	return REC_SUCCESS;
}
