// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_dumpfile.cc 1 2005-04-17 19:33:05Z vhex $
//
// This file implements a debug-only function for writing binary
// data to a file.

#ifdef _DEBUG

#ifdef _WIN32
# include <io.h>
#endif
#include <stdarg.h>
#include <stdio.h>

void rascal_dumpfile(bool truncate, const void *src, unsigned int size, const char *format, ...)
{
	FILE *out;
	va_list vl;
	char fname[1024];

	va_start(vl, format);
	vsnprintf(fname, sizeof(fname), format, vl);
	va_end(vl);

	fname[sizeof(fname) - 1] = '\0';

	if ((out = fopen(fname, truncate ? "wb" : "ab")) == NULL)
		return;

	fwrite(src, 1, size, out);
	fclose(out);
}

#endif // _DEBUG
