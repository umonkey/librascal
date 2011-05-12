// Faerion RunTime Library.
// Copyright (c) 2002-2004 hex@faerion.oss and others.
// $Id: gettime.cc 6 2005-04-17 21:31:05Z vhex $

#include <stdarg.h>
#ifdef _WIN32
# include <windows.h>
#else
# include <string.h>
# include <sys/time.h>
#endif

void gettime(unsigned int *sec, unsigned int *msec)
{
#ifdef _WIN32
	SYSTEMTIME st;
	GetSystemTime(&st);
	*sec = st.wSecond;
	*msec = st.wMilliseconds;
#else
	struct timeval tv;
	memset(&tv, 0, sizeof(tv));
	gettimeofday(&tv, NULL);
	*sec = tv.tv_sec;
	*msec = tv.tv_usec / 1000;
#endif
}
