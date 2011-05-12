// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: header.cc 1 2005-04-17 19:33:05Z vhex $

#include <stdio.h>
#include "resolver.h"
#include "../debug.h"

bool header::put(char *&dst, unsigned int &size, const addr_t &addr)
{
	char hostname[256];

	if (addr.length == 4)
		snprintf(hostname, sizeof(hostname), "%u.%u.%u.%u.in-addr.arpa", addr.data[3], addr.data[2], addr.data[1], addr.data[0]);
	else
		return false;

	return put(dst, size, hostname);
}


bool header::put(char *&dst, unsigned int &size, const char *hostname)
{
	while (*hostname != 0) {
		const char *end = hostname;

		// Skip the domain name.
		while (*end != 0 && *end != '.')
			++end;

		// Zero length domain name.
		if (end == hostname)
			return false;

		// Need space to append both the domain name and
		// its length; fail if not enough.
		if (size < (unsigned int)(end - hostname + 1))
			return false;

		// Decrease the amount of remaining buffer space.
		size -= end - hostname + 1;

		// Set the domain name length.
		*(unsigned char *)dst++ = (unsigned char)(end - hostname);

		// Copy the domain name.
		while (hostname != end)
			*dst++ = *hostname++;

		// Skip the delimeter (only one).
		if (*hostname == '.')
			++hostname;
	}

	if (size == 0)
		return false;

	*dst++ = 0;
	--size;

	return true;
}


bool header::skip_name(const char *&src, unsigned int &size)
{
	while (size != 0) {
		// End of the dn.
		if (*src == 0) {
			++src, --size;
			return true;
		}

		// Length of a part of the dn.
		else if (*(unsigned char *)src < 64) {
			unsigned int add = *(unsigned char *)src + 1;
			if (add > size)
				add = size;
			src += add;
			size -= add;
		}

		// Compression mark.
		else {
			unsigned short add;

			if (!get(src, size, add))
				return false;

			return true;
		}
	}

	return false;
}


header::header(unsigned short _id)
{
	id = _id;
	qr = 0;
	opcode = 0;
	aa = 0;
	tc = 0;
	rd = 1;
	ra = 0;
	z = 0;
	rcode = 0;
	qdcount = 0;
	ancount = 0;
	nscount = 0;
	arcount = 0;

	head = NULL;
}


bool header::parse(const char *& src, unsigned int & size)
{
	unsigned short tmp;

	head = src;

	if (!get(src, size, id))
		return false;

	if (!get(src, size, tmp))
		return false;

	qr = (tmp >> 15) & 0x01;
	opcode = (tmp >> 11) & 0x0f;
	aa = (tmp >> 10) & 0x01;
	tc = (tmp >> 9) & 0x01;
	rd = (tmp >> 8) & 0x01;
	ra = (tmp >> 7) & 0x01;
	z = (tmp >> 4) & 0x07;
	rcode = tmp & 0x0f;

	if (!get(src, size, qdcount))
		return false;

	if (!get(src, size, ancount))
		return false;

	if (!get(src, size, nscount))
		return false;

	if (!get(src, size, arcount))
		return false;

	return true;
}


bool header::dump(char *&dst, unsigned int &size)
{
	if (!put(dst, size, id))
		return false;
	if (!put(dst, size, (qr << 15) | (opcode << 11) | (aa << 10) | (tc << 9) | (rd << 8) | (ra << 7) | (z << 4) | rcode))
		return false;
	if (!put(dst, size, qdcount))
		return false;
	if (!put(dst, size, ancount))
		return false;
	if (!put(dst, size, nscount))
		return false;
	if (!put(dst, size, arcount))
		return false;
	return true;
}


bool header::skip_echo(const char *&src, unsigned int &size)
{
	while (qdcount != 0) {
		if (!skip_name(src, size))
			break;
		--qdcount;

		// Four bytes for QTYPE and QCLASS.
		if (size < 4)
			return false;

		src += 4;
		size -= 4;
	}

	return true;
}


unsigned int header::get_addrs(const char *& src, unsigned int & size, addr_t *addrv, unsigned int addrc)
{
	unsigned int count = 0, _ttl;
	unsigned short _type, _class, _length;

	if (ancount == 0 || rcode != 0)
		return count;

	if (!skip_echo(src, size))
		return 0;

	// Extracts responses.
	for (unsigned int idx = 0; idx < ancount; ++idx) {
		if (!skip_name(src, size))
			break;

		if (!get(src, size, _type))
			break;
		if (!get(src, size, _class))
			break;
		if (!get(src, size, _ttl))
			break;
		if (!get(src, size, _length))
			break;

		if (_length > size)
			break;

		if (_type == TYPE_A && _class == CLASS_IN && _length == 4 && count < addrc) {
			for (char *osrc = (char *)src, *lim = osrc + _length, *odst = (char *)addrv[count].data; osrc != lim; ++osrc, ++odst)
				*odst = *osrc;
			addrv[count++].length = _length;
		}

		src += _length;
		size -= _length;
	}

	return count;
}


unsigned int header::get_hosts(const char *&src, unsigned int &size, hostname_t *hostv, unsigned int hostc)
{
	unsigned int count = 0, _ttl;
	unsigned short _type, _class, _length;

	if (ancount == 0 || rcode != 0)
		return count;

	if (!skip_echo(src, size))
		return 0;

	// Extractt responses.
	for (unsigned int idx = 0; idx < ancount; ++idx) {
		const char *osrc = src;
		unsigned int osize = size;

		if (!skip_name(src, size))
			break;

		if (!get(src, size, _type))
			break;
		if (!get(src, size, _class))
			break;
		if (!get(src, size, _ttl))
			break;
		if (!get(src, size, _length))
			break;

		if (_length > size)
			break;

		if (_type == TYPE_PTR && _class == CLASS_IN && count < hostc) {
			if (!get(src, size, hostv[idx].data, sizeof(hostv[idx].data)))
				break;
			++count;
		} else {
			debug((flog, rl_resolver, "  -- skipped a record of type %u, length %u.\n", _type, _length));
		}

		src = osrc + _length;
		size = osize - _length;
	}

	return count;
}


bool header::get(const char *&src, unsigned int &size, char *dst, unsigned int dsize)
{
	if (dsize == 0)
		return false;

	while (size != 0 && *src != 0) {
		// Copy a label.
		if ((*src & 0xC0) != 0xC0) {
			// End of the label.
			const char *lim = src + *(unsigned char *)src + 1;
			// Skip the length octet.
			++src, --size;
			// Copy it.
			while (size != 0 && dsize != 0 && src != lim) {
				*dst++ = *src++;
				--size, --dsize;
			}

			if (dsize != 0) {
				*dst++ = '.';
				--dsize;
			}
		}
		// Decompress a label (recurse).
		else {
			const char *nsrc;
			unsigned int nsize;
			unsigned short offset;

			// The offset to the original label.
			if (!get(src, size, offset))
				return false;

			// Find the original label.
			nsrc = head + (offset & 0x3FFF);
			if (nsrc >= (src + size))
				return false;

			// Adjust the size.
			nsize = size + (src - nsrc);

			// Recurse.
			return get(nsrc, nsize, dst, dsize);
		}
	}

	while (dst[-1] == '.')
		--dst, ++dsize;

	if (dsize == 0)
		--dst;
	*dst = 0;

	return true;
}
