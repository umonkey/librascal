// Faerion RunTime Library.
// Copyright (c) 2003-2004 hex@faerion.oss and others.
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: stock.h 3 2006-12-16 00:47:17Z justin.forest $
//
// This is a protected container that maps IDs to objects.  This
// helps protecting the internal data from accessing by the client
// application, thus dicreasing the risk of damage.
//
// The number of stored objects is limited to 65536 (0x10000).

#ifndef __librascal_common_util_stock_h
#define __librascal_common_util_stock_h

#include <stdlib.h>
#include "mutex.h"

template <class T>
class stock {
	class oid {
	public:
		int id;
		T body;
	};
	// Stored objects.
	oid *ptr_data;
	// Indexes of unused slots.
	unsigned int *ptr_free;
	// The number of allocated (not "used"!) elements
	unsigned int cnt_data;
	// The number of unused slots.
	unsigned int cnt_free;
	// Page size.
	unsigned int pagesize;
	// Serial number.
	unsigned int idx_seed;
	// Randomizer.
	int rnd_seed;
public:
	// Safety guard.
	mutex mx;
public:
	// Used to iterate through the list.
	typedef oid* iterator;
	typedef const oid* const_iterator;
	//
	stock(unsigned int pgsz = 100);
	~stock();
	// Returns the number of used elements.
	unsigned int size(void) const;
	// Inserts an object, returns its key.
	int add(T obj);
	// Retreives an object.
	bool get(int idx, T & obj);
	bool get(int idx, T *& obj);
	// Removes an object.
	bool remove(int idx);
	// Validation helpers.
	inline iterator begin(void) { return ptr_data; }
	inline iterator end(void) { return ptr_data + cnt_data; }
protected:
	// Finds an object.
	bool find(int idx, iterator &it);
};


template <class T>
stock<T>::stock(unsigned int pgsz)
{
	ptr_data = 0;
	ptr_free = 0;
	cnt_data = 0;
	cnt_free = 0;
	rnd_seed = 0;
	pagesize = pgsz;
}


template <class T>
stock<T>::~stock()
{
	if (ptr_data != 0)
		free(reinterpret_cast<void *>(ptr_data));
	if (ptr_free != 0)
		free(reinterpret_cast<void *>(ptr_free));
}


template <class T>
unsigned int stock<T>::size(void) const
{
	return cnt_data - cnt_free;
}


template <class T>
int stock<T>::add(T obj)
{
	mlock lock(mx);
	iterator it;

	if (cnt_free == 0) {
		oid *n_data = reinterpret_cast<oid *>(realloc(reinterpret_cast<void *>(ptr_data), (cnt_data + pagesize) * sizeof(oid)));
		unsigned int *n_free = reinterpret_cast<unsigned int *>(realloc(reinterpret_cast<void *>(ptr_free), (cnt_data + pagesize) * sizeof(unsigned int)));

		if (n_data == 0 || n_free == 0)
			return -1;

		ptr_data = n_data;
		ptr_free = n_free;

		for (unsigned int idx = 0; idx < pagesize; ++idx) {
			ptr_free[cnt_free++] = cnt_data + idx;
			ptr_data[cnt_data + idx].id = -1;
		}

		cnt_data += pagesize;
	}

	idx_seed = (idx_seed + 0x10000) & 0x7fff0000;

	it = ptr_data + ptr_free[--cnt_free];

	it->id = (idx_seed | (it - ptr_data)) ^ rnd_seed;
	it->body = obj;

	return it->id;
}


template <class T>
bool stock<T>::get(int idx, T *& obj)
{
	mlock lock(mx);
	iterator it = ptr_data + ((idx ^ rnd_seed) & 0xffff);

	if (it < begin() || it >= end() || it->id != idx)
		return false;

	*obj = it->body;
	return true;
}


template <class T>
bool stock<T>::get(int idx, T & obj)
{
	T *tmp = &obj;
	return get(idx, tmp);
}


template <class T>
bool stock<T>::remove(int idx)
{
	mlock lock(mx);
	iterator it = ptr_data + ((idx ^ rnd_seed) & 0xffff);

	if (it < begin() || it >= end() || it->id != idx)
		return false;

	ptr_free[cnt_free++] = idx & 0xffff;
	it->id = -1;
	return true;
}


template <class T>
bool stock<T>::find(int idx, iterator &it)
{
	mlock lock(mx);
	it = ptr_data + ((idx ^ rnd_seed) & 0xffff);

	if (it < begin() || it >= end() || it->id != idx)
		return false;

	return true;
}

#endif // __librascal_common_util_stock_h
