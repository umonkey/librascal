// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_get_sq_size.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection.h"

rrid_t rascal_get_sq_size(rrid_t rid, unsigned int *size)
{
	pobject<connection> tmp(rid);

	if (!tmp.is_valid())
		return REC_INVALID_HANDLE;

	*size = tmp->get_sq_size();
	return REC_SUCCESS;
}
