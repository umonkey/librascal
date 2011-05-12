// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connection_st.h 3 2006-12-16 00:47:17Z justin.forest $

#ifndef __rascal_nt_connection_st_h
#define __rascal_nt_connection_st_h

#include "connection.h"

class connection_st : public connection {
public:
	DECLARE_ALLOCATORS;
	connection_st(rascal_dispatcher, void *context);
	~connection_st(void);
	// Stores the destination address.
	rrid_t connect(const sock_t &peer);
	// Fails immediately.
	rrid_t accept(const sock_t &peer);
	// Operation launchers.
	void spawn_reader(void);
	void spawn_writer(bool force_idle = false);
};


// Overlapped reading.  See the source file for comments.
class oreader : public overlapped
{
	static void CALLBACK callback(DWORD dwError, DWORD dwTransferred, WSAOVERLAPPED *, DWORD dwFlags);
public:
	DECLARE_ALLOCATORS;
	oreader(connection *);
	~oreader(void);
	void on_event(unsigned int transferred, bool failed);
};


// Overlapped writing.  See the source file for comments.
class owriter : public overlapped
{
	static void CALLBACK callback(DWORD dwError, DWORD dwTransferred, WSAOVERLAPPED *, DWORD dwFlags);
public:
	DECLARE_ALLOCATORS;
	owriter(connection *, bool force_idle = false);
	~owriter(void);
	void on_event(unsigned int transferred, bool failed);
};


// Overlapped outgoing connection.  See the source file for comments.
class oconnect : public overlapped
{
public:
	struct _info {
		oconnect *next;
	};
	struct _info& info(void) { return * reinterpret_cast<_info *>(data); }
public:
	DECLARE_ALLOCATORS;
	oconnect(connection *, rrid_t &rc);
	~oconnect(void);
	void on_event(unsigned int transferred, bool failed);
};


// Overlapped incoming connection.  See the source file for comments.
class oaccept : public overlapped
{
public:
	struct _info {
		SOCKET s;
		char peerdata[sizeof(SOCKADDR) * 4];
	};
	struct _info& info(void) { return * reinterpret_cast<_info *>(data); }
	void get_peer_name(sock_t &peer);
public:
	DECLARE_ALLOCATORS;
	oaccept(connection *, rrid_t &rc);
	~oaccept(void);
	void on_event(unsigned int transferred, bool failed);
};

#endif // __rascal_nt_connection_st_h
