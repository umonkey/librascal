// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: test-02.cc 1 2005-04-17 19:33:05Z vhex $
//
// Test case for resource resolution.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
# include <io.h>
# include <windows.h>
#endif
#include <faeutil/mutex.h>
#include <faeutil/sem.h>
#include "../../rascal.h"

using namespace rascal;
using namespace faeutil;

#ifndef dimof
# define dimof(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

static const char *usage_msg =
	"This is a test program for RASCAL, which establishes an outgoing\n"
	"connection and optionally reads and writes to it.  The program\n"
	"displays significant amount of debugging information.\n"
	"\n"
	"Usage: test [OPTIONS]\n"
	"\n"
	"Options:\n"
	"  -domain NAME   : domain name where the service is (defaults to \"forestnet.org\")\n"
	"  -service NAME  : service name (defaults to \"irc\")\n"
	"  -proto NAME    : transport protocol (defaults to \"tcp\")\n"
	;

static const char *events[] = { "rop_accept", "rop_close", "rop_connect", "rop_read", "rop_write" };

static sem hFinished;
static rrid_t rid = -1;

static bool __rascall dispatcher(rrid_t conn, const sock_t *peer, int event, void *context)
{
	printf("dispatcher,\n  > conn=0x%08x,\n  > peer=%s:%u,\n  > event=%d(%s),\n  > context=0x%08x%c\n", conn, ntoa(*peer).c_str(), peer->port, event, events[event], reinterpret_cast<int>(context), rascal_isok(conn) ? '.' : ',');

	if (!rascal_isok(conn))
		printf("  > msg=\"%s\"\n", errmsg(conn).c_str());

	switch (event) {
	case rop_connect:
		if (rascal_isok(conn)) {
			printf("Connected to %s:%u.\n", ntoa(*peer).c_str(), peer->port);
		} else {
			printf("Connectin failed.\n");
		}
		hFinished.post();
		return true;
	case rop_close:
		if (conn == REC_SUCCESS)
			printf("Connection to %s:%u gracefully closed.\n", ntoa(*peer).c_str(), peer->port);
		else
			printf("Connection to %s:%u closed: %s\n", ntoa(*peer).c_str(), peer->port, errmsg(rid).c_str());
		hFinished.post();
		break;
	case rop_read:
		printf("Read data.\n");
		break;
	case rop_write:
		{
			unsigned int left;
			if (rascal_isok(rascal_get_sq_size(conn, &left)))
				printf("Wrote data, bytes left: %u.\n", left);
			else
				printf("Wrote data, queue size is unknown.\n");
		}
		break;
	}

	return false;
}


static bool __rascall filter(void *, const sock_t *)
{
	return true;
}


int main(int argc, char *argv[])
{
	int proto = proto_tcp;
	const char *service = "irc", *addr = "localhost";

	while (++argv, --argc) {
		if (strcmp(*argv, "-domain") == 0 && argc > 1) {
			addr = argv[1];
			++argv, --argc;
		}
		else if (strcmp(*argv, "-service") == 0 && argc > 1) {
			service = argv[1];
			++argv, --argc;
		}
		else if (strcmp(*argv, "-proto") == 0 && argc > 1) {
			if (strcmp(argv[1], "tcp") == 0)
				proto = proto_tcp;
			else if (strcmp(argv[1], "udp") == 0)
				proto = proto_udp;
			else {
				fprintf(stderr, "bad protocol name: %s.\n", argv[1]);
				return 1;
			}
			++argv, --argc;
		}
		else {
			fprintf(stderr, "%s", usage_msg);
			return 1;
		}
	}

	if (!rascal_isok(rid = rascal_init(RIP_WORKER_SINGLE))) {
		fprintf(stderr, "RASCAL initialization failed: %s\n", errmsg(rid).c_str());
		return 1;
	} else {
		fprintf(stdout, "RASCAL initialized.\n");
	}

	if (!rascal_isok(rid = rascal_connect_service(service, proto, addr, dispatcher, NULL, filter))) {
		fprintf(stderr, "Connection request failed: %08x, %s.\n", rid, errmsg(rid).c_str());
		return 1;
	} else {
		fprintf(stdout, "Request to connect to service %s (%s over %s) schedulled, waiting (rid: %x).\n", addr, service, proto == proto_tcp ? "tcp" : "udp", rid);
	}

	hFinished.wait();

	return 0;
}
