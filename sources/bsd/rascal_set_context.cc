// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_set_context.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection.h"

rrid_t rascal_set_context(rrid_t rid, void *context)
{
	pobject<connection> tmp(rid);

	if (!tmp.is_valid())
		return REC_INVALID_HANDLE;

	tmp->set_context(context);
	return REC_SUCCESS;
}
