// Faerion RunTime Library.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: memory.cc 2 2005-04-17 21:09:06Z vhex $

#include "memory.h"
#if defined(HAVE_libdmallocthcxx) && defined(_DEBUG)
# include <dmalloc.h>
#endif

pageman::pageman(unsigned int _pagesize)
{
	head = NULL;
	pagesize = _pagesize;
}


pageman::~pageman()
{
	while (head != NULL) {
		pagehead *next = head->next;
		delete head;
		head = next;
	}
}


void* pageman::pmalloc(size_t itemsize)
{
	void *rc = NULL;
	mlock lock(mx);
	pagehead **item;

	for (item = &head; ; item = &(*item)->next) {
		if (*item == NULL)
			new pagehead(*item, pagesize, itemsize);

		if (*item == NULL)
			break;

		if ((*item)->available != 0) {
			rc = (*item)->alloc(pagesize, itemsize);
			break;
		}
	}

	return rc;
}


void pageman::pmfree(void *object, size_t)
{
	mlock lock(mx);

	// Get a pointer to the allocation item.
	pageitem *item = (pageitem *)(((char *)object) - sizeof(item->head));

	// Calculate the index in the array.
	unsigned int idx = ((char *)item - item->head->items) / (item->head->itemsize + sizeof(pagehead *));

	// Index the element as free.
	item->head->index[item->head->available++] = idx;
}


pagehead::pagehead(pagehead *&dst, unsigned int psz, unsigned int isz)
{
	available = psz;
	next = NULL;
	itemsize = isz;

	index = new unsigned int[psz];
	items = new char[(isz + sizeof(pagehead *)) * psz];

	if (index == NULL || items == NULL) {
		dst = NULL;
		delete this;
		return;
	}

	while (psz-- != 0) {
		pageitem &item = get_item(psz);
		index[psz] = psz;
		item.head = this;
	}

	dst = this;
}


pagehead::~pagehead()
{
	if (index != NULL)
		delete [] index;
	if (items != NULL)
		delete [] items;
}


void* pagehead::alloc(unsigned int, unsigned int isz)
{
	if (isz != itemsize) {
//		DEBUG(("CAUTION!\n>> Requested item size is %u, possible item size is %u.\n>> Possible problem: derived class introduces data members but uses base class' allocator.\n", isz, itemsize));
//		DEBUG_BREAK;
		return NULL;
	}

	if (isz != itemsize || available == 0)
		return NULL;

	return (void*)&get_item(index[--available]).body;
}
