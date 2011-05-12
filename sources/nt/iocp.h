// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: iocp.h 3 2006-12-16 00:47:17Z justin.forest $

#ifndef __rascal_nt_iocp_h
#define __rascal_nt_iocp_h

#include <winsock2.h>

// The I/O Completion Port class.  There is only one object of this class.
class iocp {
	HANDLE hPort;
	// Worker thread
	static DWORD WINAPI Worker(LPVOID lpPort);
	// Protocol numbers.
	int proto_tcp;
	int proto_udp;
public:
	// Possible operations.
	enum iocp_e {
		op_read,
		op_write,
		op_accept,
		op_connect,
		op_connect_ex,
	};
	//
	iocp();
	~iocp();
	// Creates a socket.
	SOCKET socket(int mode = SOCK_STREAM);
	// Retreives socket mode.
	int get_mode(SOCKET s);
	// Closes a socket.
	void close(SOCKET &s);
	// Peeks a completion notification.
	bool peek(unsigned int &transferred, class overlapped *&, unsigned int delay);
	// Posts a request.
	void post(overlapped *);
	// Posts to the alertable thread.
	void post_ex(overlapped *);
	// Initializes the port.
	rrid_t init();
	// Starts a overlapped.
	bool connect(SOCKET &, const sock_t &, OVERLAPPED *);
};

extern iocp port;

#endif // __rascal_nt_iocp_h
