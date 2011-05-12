// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: test-03.cc 1 2005-04-17 19:33:05Z vhex $
//
// RASCAL hostname resolution performance test program.  This program
// is a part of the RASCAL package and, beside testing, illustrates
// the way portable programs can be built with the library.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
# include <io.h>
# include <windows.h>
#else
# include <unistd.h>
# include <sys/time.h>
# define Sleep usleep
#endif
#include <faeutil/mutex.h>
#include <faeutil/sem.h>
#include "../../rascal.h"

using namespace rascal;
using namespace faeutil;

static const char *usage_msg =
	"This is a RASCAL test program that sends one hundred DNS queries\n"
	"to resolve a host name to a numeric address to track packet loss.\n"
	"\n"
	"Usage: test [OPTIONS]\n"
	"\n"
	"Options:\n"
	"  -count NUMBER : the number of packets (defaults to 100)\n"
	"  -delay COUNT  : sleep for COUNT milliseconds after sending each request\n"
	"  -domain NAME  : domain name to resolve (defaults to \"localhost\")\n"
	"  -randomize    : add a random prefix to the host name to force failures\n"
	"  -quiet        : suppress all messages except for the final statistics"
	;

// This is signaled when the work is done.
static sem hFinished;

// This is set if we shouldn't show the progress.
static bool quiet = false;

// This is the number of queries we deal with.
static unsigned count = 100;


// There is no POSIX function for getting tick count, so we're basing on NT.
#ifndef _WIN32
static unsigned int GetTickCount()
{
	struct timeval tv;
	static time_t tm = 0;
	if (tm == 0)
		time(&tm);
	gettimeofday(&tv, NULL);
	return (time(NULL) - tm) * 1000 + (tv.tv_usec / 1000);
}
#endif


// This is called when a hostname resolution finishes.
static void __rascall callback(void *context, const char *host, unsigned int argc, const addr_t *)
{
	unsigned int id = (unsigned int)context;

	--count;

	if (!quiet)
		fprintf(stdout, "host: %s, req: %5u, status: %s, left: %5u.\n", host, id, argc ? "good" : " bad", count);

	if (count == 0)
		hFinished.post();
}


int main(int argc, char *argv[])
{
	rrid_t rc;
	bool wait = false, randomize = false;
	const char *domain = "localhost";
	unsigned int left = count, delay = 0;
	unsigned int ticks_begin, ticks_sent, ticks_over;

	while (++argv, --argc) {
		if (strcmp(*argv, "-domain") == 0 && argc > 1) {
			domain = argv[1];
			++argv, --argc;
		}
		else if (strcmp(*argv, "-count") == 0 && argc > 1) {
			left = count = (unsigned int)atoi(argv[1]);
			++argv, --argc;
		}
		else if (strcmp(*argv, "-quiet") == 0) {
			quiet = true;
		}
		else if (strcmp(*argv, "-delay") == 0 && argc > 1) {
			delay = (unsigned int)atoi(argv[1]);
			++argv, --argc;
		}
		else if (strcmp(*argv, "-randomize") == 0) {
			randomize = true;
		}
		else if (strcmp(*argv, "-wait") == 0) {
			wait = true;
		}
		else {
			fprintf(stderr, "%s", usage_msg);
			return 1;
		}
	}

	if (count == 0) {
		fprintf(stderr, "Not a single packet lost!\n");
		return 1;
	}

	if (!rascal_isok(rc = rascal_init(RIP_WORKER_SINGLE))) {
		fprintf(stderr, "RASCAL initialization failed: %s\n", errmsg(rc).c_str());
		return 1;
	} else if (!quiet) {
		fprintf(stdout, "RASCAL initialized.\n");
	}

	ticks_begin = GetTickCount();

	for (unsigned int idx = 0; idx < left; ++idx) {
		char hname[256];

		snprintf(hname, sizeof(hname), "%05u.%s", rand(), domain);

		if (!rascal_isok(rc = rascal_getaddr(randomize ? hname : domain, callback, (void *)idx))) {
			fprintf(stderr, "Could not send packet %u/%u: %s.\n", idx+1, count, errmsg(rc).c_str());
			return 1;
		}
		if (delay != 0)
			Sleep(delay);
	}

	ticks_sent = GetTickCount();

	hFinished.wait();

	ticks_over = GetTickCount();

	fprintf(stdout, "Sending %u DNS queries took %u ticks, resolved in %u ticks.\n", count, ticks_sent - ticks_begin, ticks_over - ticks_begin);

	if (wait) {
		fprintf(stdout, "Waiting for Ctrl+C.\n");
		while (true)
			Sleep(100);
	}

	return 0;
}
