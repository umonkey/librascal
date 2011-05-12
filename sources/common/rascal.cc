// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal.cc 9 2005-04-18 13:34:11Z vhex $

#include <strings.h>
#ifndef _WIN32
# include <sys/types.h>
# include <netinet/in.h>
#endif

#include "common.h"

// The purpose of this function is to ensure that all symbols
// that need to be exported are referenced.  This makes linker
// fail if a function is not implemented.
int fix_exports()
{
	return 0
		+ (int)&rascal_accept
		+ (int)&rascal_accept_service
		+ (int)&rascal_aton
		+ (int)&rascal_cancel
		+ (int)&rascal_connect
		+ (int)&rascal_connect_service
		+ (int)&rascal_get_errmsg
		+ (int)&rascal_get_rq_size
		+ (int)&rascal_get_sq_size
		+ (int)&rascal_get_option
		+ (int)&rascal_getaddr
		+ (int)&rascal_gethost
		+ (int)&rascal_init
		+ (int)&rascal_ntoa
		+ (int)&rascal_read
		+ (int)&rascal_reads
		+ (int)&rascal_set_context
		+ (int)&rascal_set_dispatcher
		+ (int)&rascal_set_nameserver
		+ (int)&rascal_set_option
		+ (int)&rascal_shrink
		+ (int)&rascal_wait
		+ (int)&rascal_write
		;
}


const char * get_event_name(int id)
{
	static const char *names[] = {
		"rop_accept",
		"rop_close",
		"rop_connect",
		"rop_read",
		"rop_write"
	};

	if (id < 0 || id >= (int)(sizeof(names) / sizeof(names[0])))
		return "rop_BADID";

	return names[id];
}


void sock_t::put(struct sockaddr_in &sa) const
{
	memset(&sa, 0, sizeof(sa));

	sa.sin_family      = AF_INET;
	sa.sin_port        = htons(port);
	sa.sin_addr.s_addr = *(u_long *)addr.data;
}


void sock_t::get(const struct sockaddr_in &sa)
{
	if (sa.sin_family == AF_INET) {
		port = ntohs(sa.sin_port);
		*(u_long *)addr.data = sa.sin_addr.s_addr;
		addr.length = 4;
	} else {
		addr.length = 0;
	}
}


bool sock_t::operator == (const sock_t &src)
{
	if (addr.length != src.addr.length)
		return false;
	if (port != src.port)
		return false;
	for (const unsigned char *a = src.addr.data, *b = addr.data, *l = a + addr.length; a != l; ++a, ++b)
		if (*a != *b)
			return false;
	return true;
}


sock_t& sock_t::operator = (const sock_t &src)
{
	addr = src.addr;
	port = src.port;
	return *this;
}
