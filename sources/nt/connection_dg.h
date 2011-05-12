// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connection_dg.h 3 2006-12-16 00:47:17Z justin.forest $
//
// This header defines classes that work with datagram connections.

#ifndef __rascal_nt_connection_dg_h
#define __rascal_nt_connection_dg_h

#include "connection.h"

// Datagram connection class.  Needs more dummified functions (reading, writing etc).
class connection_dg : public connection {
public:
	DECLARE_ALLOCATORS;
	connection_dg(rascal_dispatcher, void *context);
	~connection_dg();
	// Stores the destination address.
	rrid_t connect(const sock_t &peer);
	// Fails immediately.
	rrid_t accept(const sock_t &peer);
	// Operation launchers.
	void spawn_reader();
	void spawn_writer(bool force_idle = false);
};


// Overlapped datagram reading.  See the source file for comments.
class oreader_dg : public overlapped {
	struct _info {
		struct sockaddr from_addr;
		int from_length;
		char data[1024];
	};
	struct _info& info() { return * reinterpret_cast<_info *>(data); }
	static void CALLBACK callback(DWORD dwError, DWORD dwTransferred, WSAOVERLAPPED *, DWORD dwFlags);
protected:
	void on_event(unsigned int transferred, bool failed);
public:
	DECLARE_ALLOCATORS;
	oreader_dg(connection_dg *);
	~oreader_dg();
};


// Overlapped writing.  See the source file for comments.
class owriter_dg : public overlapped {
	struct _info {
		unsigned int length;
		char data[OVERLAPPED_DATA_SIZE - sizeof(unsigned int)];
	};
	struct _info& info() { return * reinterpret_cast<_info *>(data); }
	static void CALLBACK callback(DWORD dwError, DWORD dwTransferred, WSAOVERLAPPED *, DWORD dwFlags);
public:
	DECLARE_ALLOCATORS;
	owriter_dg(connection_dg *, bool force_idle = false);
	~owriter_dg();
	void on_event(unsigned int transferred, bool failed);
};

#endif // __rascal_nt_connection_dg_h
