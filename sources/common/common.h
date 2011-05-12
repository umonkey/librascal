// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: common.h 3 2006-12-16 00:47:17Z justin.forest $

#ifndef __common_rascal_h
#define __common_rascal_h

#if defined(_WIN32)
# include <windows.h>
# include <winsock2.h>
#else
# include <sys/types.h>
# include <sys/socket.h>
#endif
#include "util/mutex.h"
#include "util/memory.h"
#include "../rascal.h"
#if defined(HAVE_libdmalloc) && defined(_DEBUG)
# include <dmalloc.h>
#endif

#ifndef dimof
# define dimof(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

#ifdef __GNUC__
# define ATTRIBUTE_UNUSED __attribute__((__unused__))
#else
# define ATTRIBUTE_UNUSED
#endif

#include "../rascal.h"

#ifdef _WIN32
# define GetSysError (REC_SYSERROR_MASK | GetLastError())
#else
# define GetSysError (REC_SYSERROR_MASK | errno)
#endif

// The function for establishing outgoing connections.
rrid_t rascal_connect_ex(const sock_t *target, rascal_dispatcher, void *context, const char *mode);

// Resolver initialization.
bool rascal_initres(void);

// Debug helper.
const char * get_event_name(int id);
void rascal_dumpfile(bool truncate, const void *src, unsigned int size, const char *format, ...);

#endif // __common_rascal_h
