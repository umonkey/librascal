// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_write.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection.h"
#include "../common/debug.h"

rrid_t rascal_write(rrid_t rid, const char *at, unsigned int size)
{
	pobject<connection> tmp(rid);

	if (!tmp.is_valid())
		return REC_INVALID_HANDLE;

	debug((flog, rl_conn, "%X: queueeing %u bytes", rid, size));
	return tmp->write(at, size);
}
