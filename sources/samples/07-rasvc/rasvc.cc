// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rasvc.cc 1 2005-04-17 19:33:05Z vhex $

#include <stdio.h>
#include <stdlib.h>
#include <faeutil/sem.h>
#include <rascal.h>

static const char *usage_msg =
	"Usage: rasvc -r ip-address port\n"
	"       rasvc -s name proto domain\n"
	"\n"
	"Installs a listener on the specified address(es).  When a connection\n"
	"is accepted, echoes a string and closes the socket.\n"
	"\n"
	"Send bug reports to <team@faerion.oss>\n"
	;

static faeutil::sem hsem;

static bool __rascall disp(rrid_t conn, const sock_t *peer, int event, void *)
{
	static const char hello[] = "test ok\n";

	if (!rascal_isok(conn)) {
		fprintf(stdout, " - (%x) dead event %d (%s)\n", conn, event, rascal::errmsg(conn).c_str());
		return false;
	}

	switch (event) {
	case rop_accept:
		fprintf(stdout, " - (%x) accepted a connection from %s:%u\n", conn, rascal::ntoa(*peer).c_str(), peer->port);
		rascal_write(conn, hello, sizeof(hello));
		break;
	case rop_write:
		for (unsigned int len; rascal_isok(rascal_get_sq_size(conn, &len)) && len == 0; ) {
			fprintf(stdout, " - (%x) finished writing, closing.\n", conn);
			rascal_cancel(conn);
			break;
		}
		break;
	case rop_listen:
		fprintf(stdout, " - (%x) ready to accept connections on %s:%u.\n", conn, rascal::ntoa(*peer).c_str(), peer->port);
		break;
	default:
		fprintf(stdout, " - (%x) unsupported event (%d)\n", conn, event);
		break;
	}

	return true;
}

static bool librainit(int mode)
{
	rrid_t rid = rascal_init(RIP_WORKER_SINGLE | (mode == 'r' ? RIP_NO_DNS : 0));

	if (rascal_isok(rid))
		return true;

	fprintf(stderr, "Librascal initialisation failed: %s\n", rascal::errmsg(rid).c_str());
	return false;
}

int main(int argc, char *argv[])
{
	rrid_t rid;
	int mode = 0;

	for (int c; (c = getopt(argc, argv, "rs")) > 0; ) {
		switch (c) {
		case 'r':
		case 's':
			mode = c;
			break;
		default:
			break;
		}
	}

	argc -= optind;
	argv += optind;

	if (!librainit(mode))
		return 1;

	if (mode == 'r' && argc == 2) {
		sock_t peer;

		if (!rascal_aton(argv[0], &peer.addr)) {
			fprintf(stderr, "Does not look like an IP address: %s\n", argv[0]);
			return 1;
		}

		peer.port = atoi(argv[1]);

		if (!rascal_isok(rid = rascal_accept(&peer, disp))) {
			fprintf(stderr, "Could not start accepting connection: %s\n", rascal::errmsg(rid).c_str());
			return 1;
		}
	}

	else if (mode == 's' && argc == 3) {
		if (!rascal_isok(rid = rascal_accept_service(argv[0], argv[1], argv[2], disp))) {
			fprintf(stderr, "Could not start accepting connection: %s\n", rascal::errmsg(rid).c_str());
			return 1;
		}
	}

	else {
		fprintf(stderr, "%s", usage_msg);
		return 1;
	}

	fprintf(stdout, "Waiting.\n");
	hsem.wait();

	fprintf(stdout, "Waiting one second to flush the logs.\n");
	sleep(1);

	return 0;
}
