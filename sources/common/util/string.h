/*
 * Faerion RunTime Library.
 * Copyright (c) 2003-2004 hex@faerion.oss and others.
 * Distributed under the terms of GNU LGPL, read 'LICENSE'.
 *
 * $Id: string.h 3 2006-12-16 00:47:17Z justin.forest $
 *
 */

#ifndef __faeutil_string_h
#define __faeutil_string_h

#include <stddef.h> /* for size_t */
#include <string.h>

#include "../../../configure.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef HAVE_atou
unsigned int atou(const char *);
#endif
#ifndef HAVE_strlcpy
void strlcpy(char *, const char *, size_t);
#endif

#ifndef HAVE_strlcat
void strlcat(char *, const char *, size_t);
#endif

#ifndef HAVE_strlcpya
void strlcpya(char *, size_t, ...);
#endif

#ifndef HAVE_strlcata
void strlcata(char *, size_t, ...);
#endif

#ifndef HAVE_strisdigit
int strisdigit(const char *);
#endif

#ifndef HAVE_strtoupper
void strtoupper(char *);
#endif

#ifndef HAVE_strtok_r
char * strtok_r(char *src, const char *sep, char **state);
#endif

#ifndef HAVE_strtok
char * strtok(char *src, const char *sep);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* __faeutil_string_h */
