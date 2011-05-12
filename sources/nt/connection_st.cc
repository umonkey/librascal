// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connection_st.cc 12 2005-04-18 13:52:09Z vhex $

#include "../common/common.h"
#include "../common/datachain.h"
#include "../common/debug.h"
#include "connection_st.h"
#include "iocp.h"

// Little helper function.
template <class T> inline static T r_min(T a, T b) { return (a < b) ? a : b; }


connection_st::connection_st(rascal_dispatcher _disp, void *_ctx)
{
	s = INVALID_SOCKET;
	disp = _disp;
	context = _ctx;
	is_writing = false;

	rdb = new datachain;
	wrb = new datachain;
}


connection_st::~connection_st()
{
}


void connection_st::spawn_reader()
{
	new oreader(this);
}


void connection_st::spawn_writer(bool force_idle)
{
	new owriter(this, force_idle);
}


rrid_t connection_st::connect(const sock_t &_peer)
{
	rrid_t rc;

	this->peer = _peer;
	new oconnect(this, rc);

	return rc;
}


rrid_t connection_st::accept(const sock_t &_peer)
{
	unsigned int count = 0;

	if ((s = port.socket()) == INVALID_SOCKET)
		return GetLastError() | REC_SYSERROR_MASK;

	// Allow multiple listeners on a single address.
	{
		int tmp = 1;
		if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&tmp), sizeof(tmp)) != 0)
			return GetLastError() | REC_SYSERROR_MASK;
	}

	this->peer = _peer;

	// Bind the local endpoint.
	{
		struct sockaddr sa;
		peer.put(sa);
		if (bind(s, &sa, sizeof(sa)) != 0)
			return GetLastError() | REC_SYSERROR_MASK;
	}

	// We need no buffers, we read into the buffered area.
	{
		int value = 0;
		setsockopt(s, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&value), sizeof(value));
		setsockopt(s, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&value), sizeof(value));
	}

	if (::listen(s, SOMAXCONN) == SOCKET_ERROR)
		return GetLastError() | REC_SYSERROR_MASK;

	for (unsigned int idx = 0; idx < r_min(16U, (unsigned int)SOMAXCONN); ++idx) {
		rrid_t rc;
		new oaccept(this, rc);
		if (rascal_isok(rc))
			++count;
	}

	return count ? REC_SUCCESS : REC_UNKNOWN_ERROR;
}
