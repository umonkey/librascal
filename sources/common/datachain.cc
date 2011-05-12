// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: datachain.cc 2 2005-04-17 21:09:06Z vhex $
//
// This is the chained data stream buffer class.  The actual chain
// elements are objects of the "datachainitem" class, however, all
// operations are performed from the header class to save time on
// function calls.

#include "common.h"
#include "datachain.h"

// Little helper function.
template <class T> inline static T r_min(T a, T b) { return (a < b) ? a : b; }


IMPLEMENT_ALLOCATORS(datachain);

datachain::datachain()
{
	head = NULL;
	tail = NULL;
}


datachain::~datachain()
{
	while (head != NULL) {
		datachainitem *next = head->next;
		delete head;
		head = next;
	}
}


bool datachain::append(const char *src, unsigned int size)
{
	if (head == NULL)
		head = tail = new datachainitem;

	if (head == NULL)
		return false;

	while (size != 0) {
		char *dst, *lim;

		if (tail->used == sizeof(tail->data)) {
			if ((tail->next = new datachainitem) == NULL)
				return false;
			tail = tail->next;
		}

		dst = tail->data + tail->used;
		lim = tail->data + sizeof(tail->data);

		while (size != 0 && dst != lim) {
			*dst++ = *src++;
			--size;
		}

		tail->used = dst - tail->data;
	}

	return true;
}


void datachain::peek(char *dst, unsigned int &size)
{
	unsigned int count = 0;

	for (datachainitem *item = head; item != NULL; item = item->next) {
		const char *src = item->data + item->dead;
		const char *lim = item->data + item->used;

		while (src != lim && size != 0) {
			*dst++ = *src++;
			--size, ++count;
		}
	}

	size = count;
}


void datachain::remove(unsigned int size)
{
	datachainitem *item;

	// Phase one: mark bytes dead.
	for (item = head; item != NULL && size != 0; item = item->next) {
		unsigned int sub = r_min(size, item->used - item->dead);
		item->dead += sub;
		size -= sub;
	}

	// Phase two: trim the chain.
	while (head != NULL && head->used == head->dead) {
		datachainitem *next = head->next;

		if (tail == head)
			tail = NULL;

		delete head;
		head = next;
	}
}


void datachain::extract(char *dst, unsigned int &size)
{
	peek(dst, size);
	remove(size);
}


rrid_t datachain::gets(char *dst, unsigned int &size, int flags)
{
	datachainitem *item;
	unsigned int count = 0;
	unsigned int remcnt;
	const char *origin = dst;

	// Look for an EOL character.
	for (item = head; item != NULL; item = item->next) {
		const char *src = item->data + item->dead;
		const char *lim = item->data + item->used;

		while (src != lim && *src != '\n')
			++src, ++count;

		if (src != lim) {
			++count;
			break;
		}
	}

	// Nothing found.
	if (item == NULL)
		return REC_NO_DATA;

	// No need to retreive the data.
	if ((flags & RRF_MEASURE) != 0) {
		size = count;
		return REC_SUCCESS;
	}

	remcnt = count;

	// Retreive the data.
	for (item = head; item != NULL && size != 0; item = item->next) {
		const char *src = item->data + item->dead;
		const char *lim = item->data + item->used;

		// Copy until the first EOL.
		while (src != lim && size != 0 && count != 0) {
			*dst++ = *src++;
			--size, --count;
		}

		if (*src == '\n')
			break;
	}

	if ((flags & RRF_UNTRIMMED) == 0) {
		// Back off and strip all trailing CR's.
		while (dst != origin && (dst[-1] == '\r' || dst[-1] == '\n'))
			--dst, ++size;
	}

	// Truncate the string.
	if (size == 0 && dst != origin)
		--dst, ++size;
	if (size != 0)
		*dst = 0;

	// Dequeue the string.
	if ((flags & RRF_PEEK) == 0)
		remove(remcnt);

	// Done.
	size = dst - origin;
	return REC_SUCCESS;
}


unsigned int datachain::get_size() const
{
	datachainitem *item;
	unsigned int count = 0;

	for (item = head; item != NULL; item = item->next)
		count += item->used - item->dead;

	return count;
}


bool datachain::has_data() const
{
	for (datachainitem *item = head; item != NULL; item = item->next)
		if (item->used != item->dead)
			return true;
	return false;
}
