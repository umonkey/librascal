// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_work.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection.h"

RASCAL_API(rrid_t) rascal_work(unsigned int msec)
{
	return connection::work(msec);
}
