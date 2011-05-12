// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: raflood.cc 1 2005-04-17 19:33:05Z vhex $

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

template <class T>
static T ramin(T a, T b)
{
	return (a < b) ? a : b;
}


struct sinkdata
{
	// Connection timestamp.
	unsigned int sec, msec;
	// Transfer statistics.
	size_t bytes;
	// Request identifier.
	rrid_t rid;
	// Initialization.
	sinkdata(rrid_t rid)
	{
		this->rid = rid;
		bytes = 0;
		faeutil_gettime(&sec, &msec);
	}
	// Statistics.
	unsigned int lifetime() const
	{
		unsigned int nsec, nmsec;
		faeutil_gettime(&nsec, &nmsec);
		return (nsec - sec) * 1000 + nmsec - msec;
	}
};


struct flooddata
{
	// Connection timestamp.
	unsigned int sec, msec;
	// Transfer statistics.
	size_t sent, want;
	// Request identifier.
	rrid_t rid;
	// Initialization.
	flooddata(rrid_t rid, size_t want)
	{
		this->rid = rid;
		this->sent = 0;
		this->want = want;
		faeutil_gettime(&sec, &msec);
	}
	// Statistics.
	unsigned int lifetime() const
	{
		unsigned int nsec, nmsec;
		faeutil_gettime(&nsec, &nmsec);
		return (nsec - sec) * 1000 + nmsec - msec;
	}
	// Sends a portion of data.
	bool send()
	{
		size_t frag = ramin(want - sent, static_cast<size_t>(1024));
		rrid_t rc = rascal_write(rid, reinterpret_cast<char *>(alloca(frag)), frag);
		if (rascal_isok(rc)) {
			sent += frag;
			return true;
		}
		return false;
	}
};


static size_t count = 0;


static int usage(void)
{
	fprintf(stderr, "%s",
		"raflood: a rascal based network throughput meter.\n"
		"Usage: raflood [options] hostname port\n"
		"\n"
		"Options:\n"
		" -s count   : the number of bytes to send (suffixes K and M are valid)\n"
		"            : (otherwise listen for incoming connections)\n"
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


static bool getcount(const char *src, size_t &count)
{
	count = 0;

	while (isdigit(*src)) {
		count = count * 10 + *src - '0';
		++src;
	}

	if (*src == 'k' || *src == 'K')
		count *= 1024;
	else if (*src == 'm' || *src == 'M')
		count *= 1024 * 1024;
	else if (*src != '\0')
		return false;
	return true;
}


static void __rascall on_getaddr(void *context, const char *host, unsigned int count, const addr_t *addrs)
{
	if (count == 0) {
		fprintf(stderr, "Could not resolve %s.\n", host);
		exit(1);
	}

	reinterpret_cast<sock_t *>(context)->addr = addrs[0];
}


static bool __rascall disp_sink(rrid_t rid, const sock_t *peer, int event, void *context)
{
	sinkdata *sd = reinterpret_cast<sinkdata *>(context);

	switch (event) {
	case rop_accept:
		fprintf(stdout, "Incoming connection from %s:%u accepted.\n", ntoa(peer->addr).c_str(), peer->port);
		rascal_set_context(rid, new sinkdata(rid));
		break;
	case rop_close:
		if (sd->bytes == 0)
			fprintf(stdout, "Connection closed, nothing transferred.\n");
		else {
			double avg = (double)sd->bytes / ((double)sd->lifetime() / (double)1000);
			if (avg > 1048576)
				fprintf(stdout, "Connection closed, %u bytes transferred in %u msec, avg=%.4fM.\n", sd->bytes, sd->lifetime(), avg / (double)1048576);
			else
				fprintf(stdout, "Connection closed, %u bytes transferred in %u msec, avg=%.2fK.\n", sd->bytes, sd->lifetime(), avg);
		}
		delete sd;
		break;
	case rop_read:
		{
			char sink[1024];
			unsigned int tmp;
			if (rascal_isok(rascal_get_rq_size(rid, &tmp)))
				sd->bytes += tmp;
			tmp = sizeof(sink);
			while (tmp != 0) {
				tmp = sizeof(sink);
				rascal_read(rid, sink, &tmp);
			}
		}
		break;
	}

	return true;
}


static bool __rascall disp_flood(rrid_t rid, const sock_t *peer, int event, void *context)
{
	flooddata *fd = reinterpret_cast<flooddata *>(context);

	switch (event) {
	case rop_connect:
		fd = new flooddata(rid, count);
		fprintf(stdout, "Connected to %s:%u, sending %u bytes.\n", ntoa(peer->addr).c_str(), peer->port, fd->want);
		rascal_set_context(rid, fd);
		fd->send();
		break;
	case rop_close:
		if (rascal_isok(rid))
			fprintf(stdout, "Connection closed, %u bytes transferred in %u msec (%s).\n", fd->sent, fd->lifetime(), rascal::errmsg(rid).c_str());
		else
			fprintf(stdout, "Error: %s.\n", rascal::errmsg(rid).c_str());
		if (fd != NULL) {
			rascal_set_context(fd->rid, NULL);
			delete fd;
		}
		break;
	case rop_write:
		if (fd->sent == fd->want) {
			fprintf(stdout, "Wrote everything.\n");
			rascal_cancel(rid);
		} else {
			fd->send();
		}
		break;
	}

	return true;
}


static bool getsock(char * const * argv, sock_t &sock)
{
	rrid_t rc;

	if (strcmp(*argv, "*") == 0) {
		rascal_aton("0.0.0.0", &sock.addr);
		return true;
	}

	if (rascal_aton(*argv, &sock.addr))
		return true;

	if (!rascal_isok(rc = rascal_getaddr(*argv, on_getaddr, &sock))) {
		rafail(rc, "hostname resolution");
		return false;
	}

	if (!rascal_isok(rc = rascal_wait(rc))) {
		rafail(rc, "hostname resolution");
		return false;
	}

	if (sock.port != 0) {
		fprintf(stderr, "Could not resolve %s.\n", *argv);
		return false;
	}

	return true;
}


int main(int argc, char * const * argv)
{
	char ch;
	rrid_t rc;
	sock_t sock;

	while ((ch = getopt(argc, argv, "ls:v")) != -1) {
		switch (ch) {
		case 's':
			if (!getcount(optarg, count))
				return usage();
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

	if (argc < 1)
		return usage();

	if (!rascal_isok(rc = rascal_init(RIP_WORKER_SINGLE)))
		return rafail(rc, "initialization");

	if (!getsock(argv, sock))
		return 1;
	++argv, --argc;

	if (argc < 1)
		return usage();
	sock.port = static_cast<unsigned short>(atoi(argv[0]));

	if (count == 0) {
		if (!rascal_isok(rc = rascal_accept(&sock, disp_sink)))
			return rafail(rc, "accept");
		fprintf(stdout, "Listening on %s:%u, press Ctrl+C to quit.\n", ntoa(sock.addr).c_str(), sock.port);
		while (true)
			sleep(1);
		return 0;
	} else {
		if (!rascal_isok(rc = rascal_connect(&sock, disp_flood)))
			return rafail(rc, "connecting");
		fprintf(stdout, "Connecting to %s:%u (rid %x).\n", ntoa(sock.addr).c_str(), sock.port, rc);
		rascal_wait(rc);
		fprintf(stdout, "Exiting.\n");
		return 0;
	}
}
