// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_set_dispatcher.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection.h"
#include "../common/debug.h"

rrid_t rascal_set_dispatcher(rrid_t rid, rascal_dispatcher disp)
{
	pobject<connection> tmp(rid);

	if (!tmp.is_valid())
		return REC_INVALID_HANDLE;

	debug((flog, rl_conn, "%X: the dispatcher is being changed", rid));
	tmp->set_dispatcher(disp);
	return REC_SUCCESS;
}
