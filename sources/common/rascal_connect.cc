// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_connect.cc 2 2005-04-17 21:09:06Z vhex $

#include <errno.h>
#include "common.h"
#include "debug.h"

rrid_t rascal_connect(const sock_t *target, rascal_dispatcher disp, void *context, const char *proto)
{
	return rascal_connect_ex(target, disp, context, proto);
}
