// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: overlapped.cc 1 2005-04-17 19:33:05Z vhex $

#include "connection.h"

IMPLEMENT_ALLOCATORS(overlapped);

overlapped::overlapped(connection *_host)
{
	Internal = 0;
	InternalHigh = 0;
	Offset = 0;
	OffsetHigh = 0;
	hEvent = NULL;
	psec = 0;
	lma = false;
	host = _host;

	host->incref();
}


overlapped::~overlapped()
{
	host->decref();
}


OVERLAPPED* overlapped::get_base()
{
	return reinterpret_cast<OVERLAPPED *>(&Internal);
}


overlapped* overlapped::restore(OVERLAPPED *olp)
{
	return reinterpret_cast<overlapped *>(reinterpret_cast<char *>(olp) - 4);
}
