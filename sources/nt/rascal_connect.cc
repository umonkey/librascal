// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_connect.cc 22 2005-04-18 18:11:57Z vhex $

#include <string.h>
#include "connection.h"
#include "connection_dg.h"
#include "connection_st.h"

rrid_t rascal_connect_ex(const sock_t *target, rascal_dispatcher disp, void *context, const char *mode)
{
	rrid_t rc;
	connection *tmp;

	if (mode == NULL)
		mode = "tcp";

	if (strcmp(mode, "udp") == 0)
		tmp = new connection_dg(disp, context);
	else if (strcmp(mode, "tcp") == 0)
		tmp = new connection_st(disp, context);
	else
		return REC_INVALID_ARGUMENT;

	if (tmp == NULL)
		return GetLastError() | REC_SYSERROR_MASK;

	if (!rascal_isok(rc = tmp->connect(*target))) {
		delete tmp;
		return rc;
	}

	return tmp->get_id();
}


rrid_t rascal_connect(const sock_t *target, rascal_dispatcher disp, void *context)
{
	return rascal_connect_ex(target, disp, context, NULL);
}
