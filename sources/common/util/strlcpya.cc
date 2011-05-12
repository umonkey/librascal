// Faerion RunTime Library.
// Copyright (c) 2002-2004 hex@faerion.oss and others.
// $Id$

#ifndef HAVE_strlcpya
#include <stdarg.h>
#include "string.h"

void strlcpya(char *buf, size_t size, ...)
{
	va_list vl;
	const char *arg;

	va_start(vl, size);

	while (size != 0 && (arg = va_arg(vl, const char *)) != 0) {
		while (size != 0 && *arg != '\0') {
			*buf++ = *arg++;
			--size;
		}
	}

	if (size == 0)
		--buf;
	*buf = '\0';

	va_end(vl);
}
#endif
