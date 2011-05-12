// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: packetchain.cc 2 2005-04-17 21:09:06Z vhex $

#include "common.h"
#include "datachain.h"

bool packetchain::append(const char *src, unsigned int size)
{
	if (head == NULL)
		head = tail = new datachainitem;

	if (head == NULL)
		return false;

	if (size != 0) {
		char *dst, *lim;

		if (tail->used != 0) {
			if ((tail->next = new datachainitem) == NULL)
				return false;
			tail = tail->next;
		}

		dst = tail->data + tail->used;
		lim = dst + sizeof(tail->data);

		while (size != 0 && dst != lim) {
			*dst++ = *src++;
			--size;
		}

		tail->used = dst - tail->data;
	}

	return true;
}


void packetchain::peek(char *dst, unsigned int &size)
{
	unsigned int count = 0;

	if (head != NULL) {
		const char *src = head->data + head->dead;
		const char *lim = head->data + head->used;

		while (src != lim && size != 0) {
			*dst++ = *src++;
			--size, ++count;
		}
	}

	size = count;
}


void packetchain::extract(char *dst, unsigned int &size)
{
	peek(dst, size);
	remove(size);
}


rrid_t packetchain::gets(char *, unsigned int &)
{
	return REC_NOT_IMPLIMENTED;
}
