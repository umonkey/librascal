// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: datachainitem.cc 2 2005-04-17 21:09:06Z vhex $

#include "common.h"
#include "datachain.h"

IMPLEMENT_ALLOCATORS(datachainitem);

datachainitem::datachainitem()
{
	used = 0;
	dead = 0;
	next = NULL;
}


datachainitem::~datachainitem()
{
}
