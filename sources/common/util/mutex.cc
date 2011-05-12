// Faerion RunTime Library.
// Copyright (c) 2003-2005 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: mutex.cc 2 2005-04-17 21:09:06Z vhex $

#include <stdio.h>
#include "mutex.h"

mutex::mutex()
{
	pthread_mutexattr_t ma;

	pthread_mutexattr_init(&ma);
	pthread_mutexattr_settype(&ma, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&lock, &ma);
	pthread_mutexattr_destroy(&ma);
}


mutex::~mutex()
{
	pthread_mutex_destroy(&lock);
}


void mutex::enter()
{
	pthread_mutex_lock(&lock);
}

void mutex::leave()
{
	pthread_mutex_unlock(&lock);
}
