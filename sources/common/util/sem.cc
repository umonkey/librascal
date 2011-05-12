// Faerion RunTime Library.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: sem.cc 2 2005-04-17 21:09:06Z vhex $

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "sem.h"
#include "ftspec.h"

seml::seml()
{
	pthread_cond_init(&cv, NULL);
}


seml::~seml()
{
	pthread_cond_destroy(&cv);
}


bool seml::waitex(mutex &mx, bool locked)
{
	bool rc;
	if (!locked)
		mx.enter();
	rc = pthread_cond_wait(&cv, mx) ? true : false;
	if (!locked)
		mx.leave();
	return rc;
}

bool seml::wait(mutex &mx, const ftspec &ts)
{
	timespec pts;
	pts.tv_sec = ts.sec;
	pts.tv_nsec = ts.msec * 1000000;

#ifdef _WIN32
	pts.tv_nsec *= 1000;
#endif

	{
		mlock lock(mx);
		errno = pthread_cond_timedwait(&cv, mx, &pts);
		return errno ? false : true;
	}
}


bool seml::post()
{
	return pthread_cond_signal(&cv) ? false : true;
}


bool seml::broadcast()
{
	return pthread_cond_broadcast(&cv) ? false : true;
}
