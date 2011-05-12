// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: raconn.cc 22 2005-04-18 18:11:57Z vhex $

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
# include <windows.h>
# include <io.h>
# define sleep(x) Sleep(x*1000)
#else
# include <sys/types.h>
#endif
#include <sys/stat.h>
#include <rascal.h>
#include "sem.h"

#ifndef dimof
# define dimof(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

using namespace rascal;

static const char *usage_msg =
	"This is a test program for RASCAL, which establishes an outgoing\n"
	"connection and optionally reads and writes to it.  The program\n"
	"displays significant amount of debugging information.\n"
	"\n"
	"Usage: raconn -r ip-address port\n"
	"       raconn -s name type domain\n"
	"       raconn -S name type domain\n"
	"\n"
	"Options:\n"
	"  -h       : this help screen\n"
	"  -r       : connects to a particular system\n"
	"  -s       : connects to a service\n"
	"  -S       : displays a service's configuration\n"
	"\n"
	"Send bug reports to <team@faerion.oss>\n"
	;

static sem hsem;

static bool __rascall disp(rrid_t conn, const sock_t *peer, int event, void *)
{
	switch (event) {
	case rop_connect:
		if (rascal_isok(conn)) {
			fprintf(stdout, "  Connected to %s:%u.\n", ntoa(*peer).c_str(), peer->port);
		} else {
			fprintf(stdout, "  Connection to %s:%u failed: %s\n", ntoa(*peer).c_str(), peer->port, errmsg(conn).c_str());
		}
		hsem.post();
		break;
	case rop_close:
		if (conn == REC_SUCCESS)
			fprintf(stdout, "  Connection to %s:%u closed gracefully.\n", ntoa(*peer).c_str(), peer->port);
		else
			fprintf(stdout, "  Connection to %s:%u closed: %s\n", ntoa(*peer).c_str(), peer->port, errmsg(conn).c_str());
		hsem.post();
		break;
	default:
		fprintf(stdout, "  Unhandled event %d.\n", event);
	}

	return false;
}

static bool __rascall disp_spy(rrid_t conn, const sock_t *, int, void *ctx)
{
	if (rascal_isok(conn))
		fprintf(stdout, " Connection succeeded (this must never happen).\n");
	fprintf(stdout, "  Total servers listed: %u.\n", * reinterpret_cast<unsigned int *>(ctx));
	hsem.post();
	return false;
}

static bool __rascall disp_conn(rrid_t conn, const sock_t *peer, int event, void *)
{
	switch (event) {
	case rop_connect:
		if (rascal_isok(conn))
			fprintf(stdout, "  Connected to %s:%u.\n", ntoa(*peer).c_str(), peer->port);
		else
			fprintf(stdout, "  Connection to %s:%u failed: %s.\n", ntoa(*peer).c_str(), peer->port, errmsg(conn).c_str());
		hsem.post();
		return true;
	case rop_close:
		fprintf(stdout, "  Connection closed: %s.\n", errmsg(conn).c_str());
		return true;
	default:
		fprintf(stdout, "  Unhandled event %d.\n", event);
		return false;
	}
}

static bool __rascall filter_spy(void *count, const sock_t *peer)
{
	unsigned int &cnt = * reinterpret_cast<unsigned int *>(count);
	fprintf(stdout, "  Found a server: %s:%u.\n", ntoa(*peer).c_str(), peer->port);
	++cnt;
	return false;
}

int main(int argc, char *argv[])
{
	rrid_t rid;
	int mode = 0;

	for (int c; (c = getopt(argc, argv, "hrsS")) > 0; ) {
		switch (c) {
		case 'h':
			fprintf(stdout, "%s", usage_msg);
			return 0;
		case 'r':
		case 's':
		case 'S':
			mode = c;
			break;
		default:
			fprintf(stderr, "Unknown option: -%c, please run with -h for a help.\n", c);
			return 1;
		}
	}

	argc -= optind;
	argv += optind;

	if (!rascal_isok(rascal_init(RIP_WORKER_SINGLE | (mode == 'r' ? RIP_NO_DNS : 0)))) {
		fprintf(stderr, "Could not initialize librascal.\n");
		return 1;
	}

	if (mode == 'r' && argc == 2) {
		sock_t peer;

		if (!rascal_aton(argv[0], &peer.addr)) {
			fprintf(stderr, "Does not look like a valid IP address: %s.\n", argv[0]);
			return 1;
		}

		peer.port = atoi(argv[1]);

		if (!rascal_isok(rid = rascal_connect(&peer, disp))) {
			fprintf(stderr, "Could not connect: %s.\n", errmsg(rid).c_str());
			return 1;
		}
	}

	else if (mode == 's' && argc == 3) {
		if (!rascal_isok(rid = rascal_connect_service(argv[0], argv[1], argv[2], disp_conn))) {
			fprintf(stderr, "Could not connect: %s.\n", errmsg(rid).c_str());
			return 1;
		}
	}

	else if (mode == 'S' && argc == 3) {
		unsigned int count = 0;
		if (!rascal_isok(rid = rascal_connect_service(argv[0], argv[1], argv[2], disp_spy, &count, filter_spy))) {
			fprintf(stderr, "Could not connect: %s.\n", errmsg(rid).c_str());
			return 1;
		}
	}

	else {
		fprintf(stderr, "Invalid parameters, please run with -h for a help.\n");
		return 1;
	}

	fprintf(stdout, "Doing things.\n");
	hsem.wait();

	fprintf(stdout, "Waiting one second to flush the logs.\n");
	sleep(1);

	return 0;
}
