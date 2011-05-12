// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_aton.cc 2 2005-04-17 21:09:06Z vhex $

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "debug.h"

static bool getseg(const char *&src, unsigned int &seg)
{
	const char *orig = src;

	seg = 0;

	while (isdigit(*src)) {
		seg = seg * 10 + *src - '0';
		++src;
	}

	return (orig != src);
}


inline static bool inaddr_parse_v4(const char *ptn, unsigned char *addr, unsigned char *mask)
{
	unsigned int count, seg;

	for (count = 0; count < 4; ++count) {
		addr[count] = 0;
		mask[count] = 255;
	}

	for (count = 0; count < 4; ) {
		if (!getseg(ptn, seg) || seg > 255)
			return false;

		addr[count] = static_cast<unsigned char>(seg & 255);
		mask[count] = 255U;

		++count;

		// End of line, must be a full IP address.
		if (*ptn == '\0') {
			if (count != 4)
				return false;
			break;
		}

		// Network range, may be any length (we're currently 1..4).
		else if (*ptn == '/') {
			break;
		}

		// Normal octet separator.
		else if (*ptn != '.') {
			return false;
		}

		++ptn;
	}

	if (count < 1)
		return false;

	if (*ptn == '\0')
		return true;
	if (*ptn != '/')
		return false;

	++ptn;

	if (strchr(ptn, '.') == NULL) {
		if (!getseg(ptn, seg) || seg > 32)
			return false;
		for (count = 0; count < 4; mask[count++] = 0);
		for (count = 0; count < seg; ++count)
			mask[count / 8] |= static_cast<unsigned char>(1 << (7 - (count % 8)));
		return true;
	}

	for (count = 0; count < 4; ++count) {
		if (!getseg(ptn, seg) || seg > 255)
			return false;

		mask[count] = seg;

		if (*ptn == '\0')
			return true;
		else if (*ptn++ != '.')
			return false;
	}

	for (; count != 4; mask[++count] = 0);

	return true;
}


bool rascal_aton(const char *pattern, addr_t *addr, addr_t *mask)
{
	addr_t tmp;

	if (mask == NULL)
		mask = &tmp;

	if (inaddr_parse_v4(pattern, addr->data, mask->data)) {
		addr->length = 4;
		mask->length = 4;
		debug((flog, rl_interface, "rascal_aton: %s = %s/%s", pattern, rascal::ntoa(*addr).c_str(), rascal::ntoa(*mask).c_str()));
		return true;
	}

	addr->length = 0;
	mask->length = 0;

	debug((flog, rl_interface, "could not interpret address %s", pattern));
	return false;
}
