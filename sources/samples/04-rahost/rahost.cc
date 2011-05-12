// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rahost.cc 1 2005-04-17 19:33:05Z vhex $

#define RASCAL_HELPERS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
# include <unistd.h>
#endif
#include <faeutil/faeutil.h>
#include <faeutil/sem.h>
#include <faeutil/time.h>
#include "../../rascal.h"

using namespace rascal;
using namespace faeutil;

struct srv_tmp
{
	const char *srv;
	int proto;
	const char *host;
	unsigned int count;
};

static bool quiet = false;

static int usage(void)
{
	fprintf(stderr, "%s",
		"rahost: a rascal based hostname resolution utility.\n"
		"Usage: rahost [options] hostname\n"
		"\n"
		"Options:\n"
		" -n addr    : specify an alternative name server\n"
		" -p proto   : protocol name (forces SRV RR resolution, default to tcp)\n"
		" -q         : quiet (for using within scripts)\n"
		" -r count   : set the number of retry attempts\n"
		" -s service : service name (forces SRV RR resolution)\n"
		" -S         : single threaded mode (for debugging purposes)\n"
		" -t seconds : resolution timeout (for single DNS request)\n"
		" -v         : print version number and exit\n"
		"\n"
		"Send your bug reports to <bugs@faerion.oss>\n"
		);
	return 1;
}


static int rafail(rrid_t rc, const char *prefix)
{
	fprintf(stderr, "%s: %s [%X]: %s.\n", "rahost", prefix, rc, errmsg(rc).c_str());
	return 1;
}


static bool __rascall dispatcher(rrid_t, const sock_t *, int event, void *context)
{
	struct srv_tmp *tmp = reinterpret_cast<struct srv_tmp *>(context);

	if (quiet == false && event == rop_close)
		fprintf(stdout, "Could not resolve %s (%s over %s).\n",  tmp->host, tmp->srv, tmp->proto);

	return false;
}


static bool __rascall filter(void *context, const sock_t *addr)
{
	struct srv_tmp *tmp = reinterpret_cast<struct srv_tmp *>(context);

	if (quiet == false && tmp->count == 0)
		fprintf(stdout, "Service %s (protocol %s) for domain %s is server by:\n", tmp->srv, tmp->proto == proto_tcp ? "tcp" : "udp", tmp->host);

	fprintf(stdout, " %02u. %s.\n", ++(tmp->count), ntoa(*addr).c_str());
	return false;
}


static void __rascall on_gethost(void *, const addr_t *addr, unsigned int count, const char **hosts)
{
	if (count == 0) {
		fprintf(stdout, "Could not resolve %s.\n", ntoa(*addr).c_str());
	} else {
		fprintf(stdout, "Resolved %s to %u addresses:\n", ntoa(*addr).c_str(), count);

		for (unsigned int idx = 1; idx <= count; ++idx, ++hosts)
			fprintf(stdout, " %02u. %s\n", idx, *hosts);
	}
}


static void __rascall on_getaddr(void *, const char *host, unsigned int count, const addr_t *addrs)
{
	if (count == 0) {
		if (!quiet)
			fprintf(stdout, "Could not resolve %s.\n", host);
	} else {
		if (quiet) {
			for (unsigned int idx = 1; idx <= count; ++idx, ++addrs)
				fprintf(stdout, "%s\n", ntoa(*addrs).c_str());
		} else {
			fprintf(stdout, "Resolved %s to %u addresses:\n", host, count);

			for (unsigned int idx = 1; idx <= count; ++idx, ++addrs)
				fprintf(stdout, " %02u. %s\n", idx, ntoa(*addrs).c_str());
		}
	}
}


int main(int argc, char * const * argv)
{
	char ch;
	rrid_t rc;
	sock_t sock;
	bool single = false;
	const char *host = NULL, *svc = NULL;
	long int retry = -1, timeout = -1;
	int proto = proto_tcp;
	faeutil::timespec now;

	while ((ch = getopt(argc, argv, "n:p:qr:s:St:v")) != -1) {
		switch (ch) {
		case 'n':
			if (!rascal_isok(rc = rascal_aton(optarg, &sock.addr, NULL)))
				return rafail(rc, "command line");
			if (sock.port == 0)
				sock.port = 53;
			break;
		case 'p':
			if (strcmp(optarg, "tcp") == 0)
				proto = proto_tcp;
			else if (strcmp(optarg, "udp") == 0)
				proto = proto_udp;
			else {
				fprintf(stderr, "Unknown protocol: %s, must be either tcp or udp.\n", optarg);
				return 1;
			}
			break;
		case 'q':
			quiet = true;
			break;
		case 'r':
			retry = atoi(optarg);
			break;
		case 's':
			svc = optarg;
			break;
		case 'S':
			single = true;
			break;
		case 't':
			timeout = atoi(optarg) * 1000;
			break;
		case 'v':
			fprintf(stdout, "rahost version %d.%d.%d.%d\n", VERSION_NUM);
			return 0;
		default:
			return usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc > 0)
		host = argv[0];

	if (host == NULL)
		return usage();

	if (sock.port != 0 && !rascal_isok(rc = rascal_set_nameserver(&sock, 1)))
		rafail(rc, "changing name servers");

	if (!rascal_isok(rc = rascal_init(single ? RIP_WORKER_MANUAL : RIP_WORKER_SINGLE)))
		return rafail(rc, "initialization");

	if (svc != NULL) {
		struct srv_tmp tmp;

		tmp.host = host;
		tmp.proto = proto;
		tmp.srv = svc;
		tmp.count = 0;

		if (!rascal_isok(rc = rascal_connect_service(svc, proto, host, dispatcher, &tmp, filter)))
			return rafail(rc, "could not send query");
	} else if (rascal_aton(host, &sock.addr, NULL)) {
		if (!rascal_isok(rc = rascal_gethost(&sock.addr, on_gethost)))
			return rafail(rc, "could not send query");
	} else {
		if (!rascal_isok(rc = rascal_getaddr(host, on_getaddr)))
			return rafail(rc, "could not send query");
	}

#ifdef _DEBUG
	fprintf(stdout, "Request sent, waiting to complete. (id=%x)\n", rc);
#endif
	now.update();

	if (!rascal_isok(rc = rascal_wait(rc)))
		return rafail(rc, "waiting on request");

	if (!quiet) {
		fprintf(stdout, "Done in %u msec.\n", (faeutil::timespec() - now).mseconds());
	}

	return 0;
}
