// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_get_errmsg.cc 2 2005-04-17 21:09:06Z vhex $

#include <stdio.h>
#include <string.h>
#include "common.h"

typedef struct rmsg {
	int id;
	const char *msg;
} rmsg;

static const rmsg rmsgs[] = {
	{ REC_SUCCESS, "the command completed successfully" },
	{ REC_NOT_IMPLIMENTED, "the function is not implemented" },
	{ REC_INVALID_HANDLE, "invalid handle" },
	{ REC_INVALID_ARGUMENT, "invalid argument" },
	{ REC_UNKNOWN_ERROR, "unknown error" },
	{ REC_UNAVAILABLE, "the requested subsystem is unavailable" },
	{ REC_INER_ASYNC, "initialization failure (asynchronous subsystem)" },
	{ REC_INER_CONNECT, "initialization failure (outgoing connections)" },
	{ REC_INER_RESOLVER, "initialization failure (domain name resolution)" },
	{ REC_INER_NOWORKERS, "could not spawn worker threads" },
	{ REC_SRV_UNAVAIL, "service is unavailable for the domain" },
	{ REC_SRV_NOTFOUND, "service not found (not configured)" },
	{ REC_NO_DATA, "there is no data in the connection's incoming buffer" },
	{ REC_NO_NAMESERVERS, "nameserver unavailable" },
	{ REC_OPTION_READONLY, "attempting to change a read-only option" },
	{ REC_CONN_TIMEOUT, "connection timed out" },
	{ REC_CANCELLED, "the operation was cancelled" },
	{ REC_BAD_PROTOCOL, "the specified protocol could not be used" },
};


#ifdef _WIN32
static void get_os_errmsg(rrid_t rid, char *dst, unsigned int size)
{
	DWORD ret = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, rid & 0x7fffffff,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		dst, size, NULL);

	if (ret == 0) {
		ret = GetLastError();
		snprintf(dst, size, "Error %u (failed: %u)", rid & (~REC_SYSERROR_MASK), (unsigned int)ret);
	} else {
		const char *src = dst;

		dst[size - 1] = 0;

		while (*src != 0) {
			if (*src != '\r' && *src != '\n')
				*dst++ = *src;
			++src;
		}

		while (dst[-1] == '.')
			--dst;

		*dst = 0;
	}
}
#else
static void get_os_errmsg(rrid_t rid, char *dst, unsigned int size)
{
	if (strerror_r(rid & ~REC_SYSERROR_MASK, dst, size) != 0)
		*dst = 0;
}
#endif

void rascal_get_errmsg(rrid_t rid, char *dst, unsigned int size)
{
	const char *msg = NULL;

	if (size == 0)
		return;

	if (rascal_isok(rid))
		rid = REC_SUCCESS;

	for (unsigned int idx = 0; idx < sizeof(rmsgs) / sizeof(rmsgs[0]); ++idx) {
		if (rmsgs[idx].id == rid) {
			msg = rmsgs[idx].msg;
			break;
		}
	}

	if (msg == NULL && (rid & 0xE0000000) == 0xE0000000)
		msg = "error message unavailable (bad error code)";

	if (msg != NULL) {
		while (*msg != 0 && size != 0) {
			*dst++ = *msg++;
			--size;
		}

		dst[size ? 0 : -1] = 0;
		return;
	}

	get_os_errmsg(rid, dst, size);
}
