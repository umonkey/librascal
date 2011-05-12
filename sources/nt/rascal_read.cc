// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_read.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection.h"

rrid_t rascal_read(rrid_t rid, char *to, unsigned int *size)
{
	pobject<connection> tmp(rid);

	if (!tmp.is_valid())
		return REC_INVALID_HANDLE;

	return tmp->read(to, *size);
}


rrid_t rascal_reads(rrid_t rid, char *to, unsigned int *size, int flags)
{
	pobject<connection> tmp(rid);

	if (!tmp.is_valid())
		return REC_INVALID_HANDLE;

	return tmp->reads(to, *size, flags);
}
