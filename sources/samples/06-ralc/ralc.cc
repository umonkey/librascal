// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: ralc.cc 1 2005-04-17 19:33:05Z vhex $

#define RASCAL_HELPERS

#include <ctype.h>
#ifdef _WIN32
# include <malloc.h> // alloca()
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
# include <unistd.h>
#else
# define sleep(c) Sleep(c * 1000)
#endif
#include <faeutil/faeutil.h>
#include <faeutil/time.h>
#include "../../common/rascal.h"

using namespace rascal;
using namespace faeutil;

static int usage(void)
{
	fprintf(stderr, "%s",
		"ralc: connection testing program for librascal.\n"
		"Usage: ralc [options]\n"
		"\n"
		"Options:\n"
		" -d msec    : delay between connection attempts\n"
		" -h address : ip address of the target system\n"
		" -n count   : the number of connection attempts\n"
		" -p number  : port number to connect to\n"
		" -v         : print version number and exit\n"
		"\n"
		"Send your bug reports to <bugs@faerion.oss>\n"
		);
	return 1;
}


static int rafail(rrid_t rc, const char *prefix)
{
	fprintf(stderr, "%s: %s [%X]: %s.\n", "ralc", prefix, rc, errmsg(rc).c_str());
	return 1;
}


static bool __rascall disp(rrid_t rid, const sock_t *peer, int event, void *context)
{
	if (rascal_isok(rid)) {
		fprintf(stdout, " - dispatcher called for #%02u (rc=%x, event=%d, %s:%u), denying everything.\n", reinterpret_cast<unsigned int>(context), rid, event, ntoa(peer->addr).c_str(), peer->port);
	} else {
		fprintf(stdout, " - dispatched an error for #%02u: [%x] %s.\n", reinterpret_cast<unsigned int>(context), rid, errmsg(rid).c_str());
	}
	return false;
}


int main(int argc, char * const * argv)
{
	char ch;
	rrid_t rc;
	sock_t sock;
	unsigned int count = 1, delay = 1000;

	while ((ch = getopt(argc, argv, "d:h:n:p:v")) != -1) {
		switch (ch) {
		case 'd':
			delay = faeutil_atou(optarg);
			break;
		case 'h':
			rascal_aton(optarg, &sock.addr);
			break;
		case 'n':
			count = faeutil_atou(optarg);
			break;
		case 'p':
			sock.port = faeutil_atou(optarg);
			break;
		case 'v':
			fprintf(stdout, "ralc version %d.%d.%d.%d\n", VERSION_NUM);
			return 0;
		default:
			return usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (count == 0 || sock.port == 0 || sock.addr.length == 0)
		return usage();

	if (!rascal_isok(rc = rascal_init(RIP_WORKER_SINGLE)))
		return rafail(rc, "initialization");

	for (unsigned int idx = 1; count != 0; --count, ++idx) {
		rrid_t rc = rascal_connect(&sock, disp, reinterpret_cast<void *>(idx));
		if (rascal_isok(rc)) {
			fprintf(stdout, "Connection to %s:%u accepted, waiting to finish (rid=%x).\n", ntoa(sock.addr).c_str(), sock.port, rc);
			rascal_wait(rc);
		} else {
			fprintf(stdout, "Connection to %s:%u failed.\n", ntoa(sock.addr).c_str(), sock.port);
		}

		usleep(delay * 1000);
	}

	fprintf(stdout, "Done.\n");
	return 0;
}
