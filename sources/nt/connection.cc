// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connection.cc 12 2005-04-18 13:52:09Z vhex $

#include "connection.h"
#include "../common/common.h"
#include "../common/datachain.h"
#include "iocp.h"
#include "../common/debug.h"

IMPLEMENT_ALLOCATORS(connection);

connection::~connection()
{
	if (rdb != NULL)
		delete rdb;
	if (wrb != NULL)
		delete wrb;
	if (s != INVALID_SOCKET)
		port.close(s);
}


rrid_t connection::get_error() const
{
	int code, size = sizeof(code);
	if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&code, &size) != 0)
		code = REC_UNKNOWN_ERROR;
	else if (code != 0)
		code |= REC_SYSERROR_MASK;
	return code;
}


void connection::cancel()
{
	if (!is_cancelled()) {
		debug((flog, rl_conn, "%X: delivering rop_close", get_id()));
		// Notify the application
		disp(get_error(), &peer, rop_close, context);
		// Cancel the request.
		object::cancel();
		// Kill readers and writers.
		port.close(s);
	}
}


rrid_t connection::write(const char *src, unsigned int size)
{
	mlock lock(mx);

	debug((flog, rl_conn, "%X: appending %u bytes to the output buffer", get_id(), size));

	if (wrb->append(src, size) && !is_writing) {
		spawn_writer();
	}

	return REC_SUCCESS;
}


rrid_t connection::read(char *to, unsigned int &size)
{
	mlock lock(mx);
	rdb->extract(to, size);
	return REC_SUCCESS;
}


rrid_t connection::reads(char *to, unsigned int &size, int flags)
{
	mlock lock(mx);
	return rdb->gets(to, size, flags);
}


void connection::on_read(const char *src, unsigned int size)
{
	rdb->append(src, size);
	debug((flog, rl_conn, "%X: delivering rop_read (%u bytes)", get_id(), size));
	disp(get_id(), &peer, rop_read, context);
	spawn_reader();
}


// The connection remains marked busy during executing the dispatcher
// to accumulate small portions of data being sent by the application,
// and then flush it in a single write to optimize network performance.
void connection::on_write(unsigned int ATTRIBUTE_UNUSED transferred)
{
	debug((flog, rl_conn, "%X: delivering rop_write (%u bytes)", get_id(), transferred));
	disp(get_id(), &peer, rop_write, context);
	mx.enter();
	is_writing = false;
	mx.leave();
	spawn_writer();
}


void connection::on_connect()
{
	debug((flog, rl_conn, "%X: delivering rop_connect", get_id()));
	if (!disp(get_id(), &peer, rop_connect, context)) {
		debug((flog, rl_conn, "%X: the connection was rejected", get_id()));
		cancel();
	} else {
		debug((flog, rl_conn, "%X: the connection was accepted", get_id()));
		spawn_reader();
	}
}


void connection::on_accept(const sock_t &_peer, SOCKET _s)
{
	s = _s;
	peer = _peer;

	debug((flog, rl_conn, "%X: delivering rop_accept", get_id()));
	if (!disp(get_id(), &peer, rop_accept, context)) {
		debug((flog, rl_conn, "%X: the connection was rejected", get_id()));
		cancel();
	} else {
		debug((flog, rl_conn, "%X: the connection was accepted", get_id()));
		spawn_reader();
	}
}


unsigned int connection::get_rq_size()
{
	mlock lock(mx);
	return rdb->get_size();
}


unsigned int connection::get_sq_size()
{
	mlock lock(mx);
	return wrb->get_size();
}
