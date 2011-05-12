// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_connect_service.cc 2 2005-04-17 21:09:06Z vhex $

#include <string.h>
#include "object.h"
#include "debug.h"
#include "resolver/resolver.h"
#include "util/string.h"

rrid_t rascal_connect_service(const char *name, const char *proto, const char *domain, rascal_dispatcher disp, void *context, rascal_rcs_filter filter)
{
	rrid_t rid;
	char hname[256];

	if (proto == NULL)
		proto = "tcp";
	else if (strcmp(proto, "tcp") != 0 && strcmp(proto, "udp") != 0)
		return REC_INVALID_ARGUMENT;

	strlcpya(hname, sizeof(hname), "_", name, "._", proto, ".", domain, NULL);

	new getsrv(hname, disp, context, filter, rid);
	return rid;
}
