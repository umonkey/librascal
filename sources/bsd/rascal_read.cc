// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_read.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection.h"
#include "../common/debug.h"

rrid_t rascal_read(rrid_t rid, char *to, unsigned int *size)
{
	rrid_t rc;
	pobject<connection> tmp(rid);

	if (!tmp.is_valid()) {
		debug((flog, rl_interface, "read attempt for a bad connection id: %X", rid));
		return REC_INVALID_HANDLE;
	}

	rc = tmp->read(to, *size);

	debug((flog, rl_interface, "retreived %u bytes from connection %X", *size, tmp->get_id()));
	return rc;
}


rrid_t rascal_reads(rrid_t rid, char *to, unsigned int *size, int flags)
{
	rrid_t rc;
	pobject<connection> tmp(rid);

	if (!tmp.is_valid())
		return REC_INVALID_HANDLE;

	rc = tmp->reads(to, *size, flags);

	if (rascal_isok(rc))
		debug((flog, rl_interface, "retreived a string of %u bytes from connection %X, flags: %d, data: %s", *size, tmp->get_id(), flags, to));
	else
		debug((flog, rl_interface, "could not retreive data from connection %X: %s (%x, flags: 0x%x)", tmp->get_id(), rascal::errmsg(rc).c_str(), rc, flags));

	return rc;
}
