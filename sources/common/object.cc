// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: object.cc 2 2005-04-17 21:09:06Z vhex $

#include <errno.h>
#include <stdio.h>
#include "object.h"
#include "debug.h"
#include "util/stock.h"

// The main list of objects that maintains IDs.  I don't want to make
// it a static class member (which it's ought to be) to avoid inclusion
// of a (quite) complicated template class into all modules without real
// benefit, even readability.
static stock<object *> object_stock;

// On this we sleep to wait until a request is dead.
static sem laze;


object::object()
{
	refc = 0;
	cancelled = false;
	id = object_stock.add(this);
	debug((flog, rl_misc, "%X: object created", id));
}


object::~object()
{
	object_stock.remove(id);
}


void object::incref()
{
	mlock lock(mx);
	++refc;
}


// We signal the laze semaphore when an object is dereferenced and
// only one reference remains.  This is the reference of the party
// wait()ing on the object.
void object::decref()
{
	bool killme, signal;

	mx.enter();
	--refc;
	killme = (refc == 0 && cancelled);
	signal = (refc == 1 && cancelled);
	mx.leave();

	if (signal) {
		debug((flog, rl_misc, "%X: only one reference left, waking up all waiters", get_id()));
		laze.broadcast();
	}

	if (killme)
		delete this;
}


unsigned int object::list(object **arr, unsigned int size, list_filter f, void *ctx)
{
	unsigned int idx = 0;
	mlock lock(object_stock.mx);
	stock<object *>::iterator it = object_stock.begin();

	while (idx < size && it != object_stock.end()) {
		if (it->id != -1 && f(it->body, ctx)) {
			*arr++ = it->body;
			++idx;
		}
		++it;
	}

	return idx;
}


void object::wait()
{
	laze.mx.enter();

	while (true) {
		debug((flog, rl_misc, "%X: waiting for the object", get_id()));

		if (laze.wait()) {
			debug((flog, rl_misc, "%X: waiting done", get_id()));
		} else {
			debug((flog, rl_misc, "%X: waiting failed, errno: %d.\n", get_id(), errno));
		}

		if (refc == 1 && cancelled)
			break;
	}

	laze.mx.leave();
}


void object::_lookup(object **host, int id, ot_t type)
{
	mlock lock(object_stock.mx);

	if (!object_stock.get(id, *host)) {
		(*host) = NULL;
	} else if (type != object::get_class_ot() && (*host)->get_object_ot() != type) {
		(*host) = NULL;
	} else {
		(*host)->incref();
	}
}


void object::_release(object **host)
{
	if (*host != NULL)
		(*host)->decref();
}
