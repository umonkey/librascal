// Faerion RunTime Library.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: memory.h 3 2006-12-16 00:47:17Z justin.forest $
//
// This is the memory manager with paged object allocation.

#ifndef __librascal_common_util_memory_h
#define __librascal_common_util_memory_h

#include "mutex.h"

// Paged and cached memory allocation class.
class pageman {
	// Thread locker.
	mutex mx;
	// Page size.
	unsigned int pagesize;
	// The first page.
	class pagehead *head;
public:
	// Initialization.
	pageman(unsigned int _pagesize = 100);
	// Destruction.
	~pageman();
	// Object allocation.
	void* pmalloc(size_t itemsize);
	void pmfree(void *object, size_t itemsize);
};

#define DECLARE_ALLOCATOR \
	static pageman _pageman

#define DECLARE_ALLOCATORS \
	void* operator new(size_t size) { return _pageman.pmalloc(size); } \
	void operator delete(void *object, size_t size) { _pageman.pmfree(object, size); }

#define IMPLEMENT_ALLOCATORS(classname) \
	pageman classname::_pageman

#if defined(BUILDING_SHARED_FAEUTIL) || 1

class pageitem {
public:
	class pagehead *head;
	char body[1];
};


class pagehead {
	friend class pageman;
protected:
	// Next page in chain.
	pagehead *next;
	// Element size;
	unsigned int itemsize;
	// The number of available elements.
	unsigned int available;
	// Indexes of available elements.
	unsigned int *index;
	// The elements.
	char *items;
	// Returns a reference to an element.
	inline pageitem& get_item(unsigned int idx) { return *(pageitem *)((char *)items + (itemsize + sizeof(pagehead *)) * idx); }
public:
	// Initialization.
	pagehead(pagehead *&dst, unsigned int _page, unsigned int _item);
	~pagehead();
	// Objcet allocation.
	void* alloc(unsigned int pagesize, unsigned int itemsize);
};

#endif // defined(BUILDING_SHARED_FAEUTIL)

#endif // __librascal_common_util_memory_h
