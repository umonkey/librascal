// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: object.h 3 2006-12-16 00:47:17Z justin.forest $
//
// Objects represent every operation that is performed asynchronously.
// This currently includes all types of connections (incoming, outgoing
// and established) and DNS requests.  The `object' class maintains a
// single list of objects and assigns them unique ids.  This is the
// main purpose of the class: to serve as a central collection of all
// operations issued by other parts of the library.
//
// The `object' implements a reference counter to ensure that objects
// are only deleted when they are not used by the library.  Objects
// must never be deleted using the `delete' operator; instead, the
// `cancel' method must be used.  Should you need to know whether a
// request was cancelled, use the `is_cancelled' method of the base
// class; you are likely to want to exclude cancelled requests from the
// regular processing.
//
// The way the reference counter works is simple: you find an object
// by its id (the automatically assigned one) and call the base class'
// `incref' method.  You do what you need, then release the object by
// calling the `decref' method.  When an object that has been marked
// as cancelled is released and the reference counter for that object
// becomes zero, the object is automatically deleted.
//
// PROBLEMS.
//
// The first weak place of this technique is the time between obtaining
// the pointer to the object and calling `incref'.  The object might
// have been released and deleted during that time.  This would
// effectively introduce a race condition crash vulnerability.
//
// The second weak place is the amount of trust we must have in the
// client application to assume it never passes an id of a DNS request
// to `rascal_write', or do anything like that.
//
// SOLUTIONS.
//
// To mitigate both of these problems, a helper class `pobject' was
// introduced.  It automates finding objects (optionally of a specific
// type) and maintaining the reference counter.  You must only
// instantiate `pobject' on stack, never leaving it float in the
// memory longer than necessary.
//
// To find an object by its id you would construct a `pobject' this way:
//
//     pobject<getaddr> tmp(id);
//
// The type specification is optional; if absent, an objects of any
// type will be returned.  To know whether the object with the given
// id was successfully found, the `is_valid' method should be used
// before any other action on `tmp' is taken.  (We avoid using
// exceptions because it would cause significant performance loss,
// while the execution time is critical.)  To access the underlying
// object, use the `->' operator.
//
// To allow objects of a class to be located by id, the class must
// define the following method:
//
//     void * get_type_id(void) const;
//
// The method must return a value unique to the class, to avoid
// collisions.  The easiest way to achieve this is to return a
// pointer to a static class member (there can be no two static
// members with the same address, obviously).  Objects of a class
// that does not define the `get_type_id' method can still be
// located as type `object'.
//
// PERFORMANCE NOTES.
//
// The `object' class is thread safe and uses thread locking internally.
// There is only one mutex for all objects, the number of objects does
// not affect performance in any way.  This also implies that retreiving
// an object by id is a fast operation.  However, using it more often
// than actually needed is bad.
//
// TODO.
//
// We want to use spinlocks instead of mutexes to lock the stock.

#ifndef __rascal_object_h
#define __rascal_object_h

#include "util/mutex.h"
#include "util/sem.h"
#include "../rascal.h"

#define DECLARE_CLASSID(name) \
	static ot_t get_class_ot(void) { static ot_t _t = "ot_" #name; return _t; } \
	virtual ot_t get_object_ot(void) const { return get_class_ot(); }

template <class T> class pobject;

class object
{
public:
	// Dynamic type identification.
	typedef const char * ot_t;
private:
	friend class pobject<object>;
	// Reference counter; the number of times incref() was called for this
	// object, see below.  When this is zero, the object is safe to
	// delete (automatically done in decref()).
	unsigned int refc;
	// Set when the object needs to be deleted, but is in use.
	bool cancelled;
	// Object identifier, assigned on creation.
	rrid_t id;
	// Object guard.
	mutex mx;
protected:
	// Increments the reference counter.
	void incref();
	// Decrements the reference counter, deletes the object
	// if it was cancelled and is no longer in use.
	void decref();
public:
	// Initializes the object and assigns the id.
	// The object is inserted into the global list.
	object();
	// Removes the object from the global list.
	virtual ~object();
	// Returns the object id.
	rrid_t get_id() const { return id; }
	// Interlocked connection fitering.
	typedef bool (*list_filter)(object *, void *);
	static unsigned int list(object **, unsigned int, list_filter, void * = 0);
	// Marks the object for deletion.  The object must be locked
	// when this method is executed.  This method is virtual to
	// allow custom client notifications (calling dispatchers, and
	// so on); the inherited method MUST call the original one.
	virtual void cancel() { cancelled = true; }
	// Checks whether the object is cancelled.
	bool is_cancelled() const { return cancelled; }
	// Waits for the request to finish.
	void wait();
	// Dynamic type identification.
	DECLARE_CLASSID(object);
	static void _lookup(object **host, int id, ot_t tid);
	static void _release(object **host);
};


template <class T = object> class pobject
{
	T *host;
	// Generic lookup function.
	static void lookup(int id, void *type);
	// Generic release function.
	static void release(void);
public:
	// Lookup constructor.
	pobject(int id) { object::_lookup(reinterpret_cast<object **>(&host), id, T::get_class_ot()); }
	// Standard destructor.
	~pobject(void) { object::_release(reinterpret_cast<object **>(&host)); }
	// Indirect access.
	T* operator -> (void) { return host; }
	// Determines whether the object was found.
	bool is_valid(void) const { return host != NULL; }
};

#endif // __rascal_object_h
