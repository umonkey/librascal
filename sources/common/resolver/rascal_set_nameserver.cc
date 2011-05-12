// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_set_nameserver.cc 1 2005-04-17 19:33:05Z vhex $

#include "resolver.h"

rrid_t rascal_set_nameserver(const sock_t *peer, unsigned int count)
{
	if (count < 1)
		return REC_INVALID_ARGUMENT;
	return rascal_connect_ex(&peer[0], ns_dispatcher, (void *)0, ns_mode);
}
