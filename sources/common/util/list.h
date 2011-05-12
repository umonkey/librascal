// Faerion RunTime Library.
// Copyright (c) 2003-2004 hex@faerion.oss and others.
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: list.h 3 2006-12-16 00:47:17Z justin.forest $

#ifndef __librascal_common_util_list_h
#define __librascal_common_util_list_h

#include <stdlib.h>
#include "mutex.h"

template <class T>
class list_ut
{
	// Stored objects.
	T* data;
	// The number of allocated elements
	size_t allocated;
	// The number of used elements
	size_t used;
public:
	// Used to iterate through the list_ut.
	typedef T* iterator;
	typedef const T* const_iterator;
	//
	list_ut();
	~list_ut();
	// Adds an object, if it isn't there yet.
	bool add(T obj);
	// Retreives an element from the list_ut.
	bool get(unsigned int idx, T &obj);
	// Appends an element to the end of the list_ut
	bool push_back(T obj);
	// Picks an element from the end.
	bool pop_back(T &obj);
	// Picks an element from the beginning.
	bool pop_front(T &obj);
	// Removes an element from the list_ut
	bool remove(T obj);
	// Removes an element at a fixed position
	bool remove_at(unsigned int idx);
	// First element of the list_ut.
	iterator begin(void) { return data; }
	// Element beyond the end of the list_ut.
	iterator end(void) { return data + used; }
	// Removes all elements.
	void empty();
	// Copies the list.
	list_ut& operator = (const list_ut &src);
	// Retreives the list dimensions.
	size_t size() const { return used; }
	size_t capacity() const { return allocated; }
};


// Adds thread safety to the list_ut class.
template <class T>
class list : protected list_ut<T>
{
	mutex mx;
public:
	typedef T* iterator;
	typedef const T* const_iterator;
	list() { }
	~list() { }
	bool add(T obj) { mlock lock(mx); return list_ut<T>::add(obj); }
	bool get(unsigned int idx, T &obj) { mlock lock(mx); return list_ut<T>::get(idx, obj); }
	bool push_back(T obj) { mlock lock(mx); return list_ut<T>::push_back(obj); }
	bool pop_back(T &obj) { mlock lock(mx); return list_ut<T>::pop_back(obj); }
	bool pop_front(T &obj) { mlock lock(mx); return list_ut<T>::pop_front(obj); }
	bool remove(T obj) { mlock lock(mx); return list_ut<T>::remove(obj); }
	bool remove_at(unsigned int idx) { return list_ut<T>::remove_at(idx); }
	iterator begin() { mlock lock(mx); return list_ut<T>::begin(); }
	iterator end() { mlock lock(mx); return list_ut<T>::end(); }
	list& operator = (const list &src) { mlock lock(mx); * reinterpret_cast<list_ut<T> *>(this) = src; return *this; }
	void copy(list_ut<T> &dst) { mlock lock(mx); dst = * reinterpret_cast<list_ut<T> *>(this); }
	size_t size() const { return list_ut<T>::size(); }
	size_t capacity() const { return list_ut<T>::capaity(); }
};


template <class T>
list_ut<T>::list_ut(void)
{
	used = 0;
	allocated = 0;
	data = NULL;
}


template <class T>
list_ut<T>::~list_ut(void)
{
	if (data != NULL) {
		free((void*)data);
	}
}


template <class T>
bool list_ut<T>::add(T obj)
{
	for (iterator item = begin(); item != end(); ++item) {
		if (*item == obj)
			return false;
	}

	return push_back(obj);
}


template <class T>
bool list_ut<T>::get(unsigned int idx, T &obj)
{
	if (idx >= used)
		return false;
	obj = data[idx];
	return true;
}


template <class T>
bool list_ut<T>::push_back(T obj)
{
	if (used == allocated) {
		T* ndata;
		size_t page = allocated / 4;
		if (page < 4)
			page = 4;

		if ((ndata = (T*)realloc((void*)data, sizeof(T) * (allocated + page))) == NULL)
			return false;

		allocated += page;
		data = ndata;
	}

	data[used++] = obj;
	return true;
}


template <class T>
bool list_ut<T>::pop_back(T &obj)
{
	if (used == 0)
		return false;

	obj = data[--used];
	return true;
}


template <class T>
bool list_ut<T>::pop_front(T &obj)
{
	if (!get(0, obj))
		return false;
	remove_at(0);
	return true;
}


template <class T>
bool list_ut<T>::remove(T obj)
{
	for (T *item = data, *limit = data + used; item < limit; ++item) {
		if (*item == obj) {
			--used;
			memcpy(item, data + used, sizeof(T));
			return true;
		}
	}

	return false;
}


template <class T>
bool list_ut<T>::remove_at(unsigned int idx)
{
	if (idx < used) {
		T *item = data + idx;
		--used;
		if (idx != used)
			memcpy((void*)item, (void*)(item + 1), sizeof(T) * (used - idx));
		return true;
	}

	return false;
}


template <class T>
void list_ut<T>::empty()
{
	if (data != NULL) {
		free(data);
		data = NULL;
		used = 0;
		allocated = 0;
	}
}


template <class T>
list_ut<T>& list_ut<T>::operator = (const list_ut &src)
{
	T* ndata = src.size() ? reinterpret_cast<T *>(malloc(sizeof(T) * src.allocated)) : NULL;

	empty();

	if (ndata != NULL) {
		allocated = src.allocated;
		used = src.used;
		data = ndata;

		for (size_t idx = 0; idx < src.size(); ++idx)
			data[idx] = src.data[idx];
	}

	return *this;
}

#endif // __librascal_common_util_list_h
