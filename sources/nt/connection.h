// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connection.h 3 2006-12-16 00:47:17Z justin.forest $

#ifndef __rascal_nt_connection_h
#define __rascal_nt_connection_h

#include <winsock2.h>
#include "../common/util/memory.h"
#include "../common/object.h"

#define OVERLAPPED_DATA_SIZE 2048

// Forward declaration.
class datachain;


// Generic connection class.
class connection : public object
{
	friend class overlapped;
protected:
	// Memory manager.
	DECLARE_ALLOCATOR;
	// Connection socket.
	SOCKET s;
	// Remote connection endpoint.
	sock_t peer;
	// Ugly to have it here but I can't add it to *_dg.
	struct sockaddr dg_from;
	// Data buffers.
	datachain *rdb, *wrb;
	// The dispatcher.
	rascal_dispatcher disp;
	// User data.
	void *context;
	// Whether there is an active owriter_st.
	bool is_writing;
	// Thread safety.
	mutex mx;
	// This saves us from rewriting several event handlers in _dg.
	virtual void spawn_reader(void) = 0;
	virtual void spawn_writer(bool force_idle = false) = 0;
public:
	DECLARE_ALLOCATORS;
	DECLARE_CLASSID(connection);
	// Construction.
	connection(void) { }
	virtual ~connection(void);
	// Returns the error code.
	rrid_t get_error(void) const;
	// Request to close the connection.
	void cancel(void);
	// Starts connecting.
	virtual rrid_t connect(const sock_t &peer) = 0;
	// Listens for incoming connections.
	virtual rrid_t accept(const sock_t &peer) = 0;
	// Writes data to the connection.
	virtual rrid_t write(const char *src, unsigned int size);
	// Extract data from the connection.
	virtual rrid_t read(char *to, unsigned int &size);
	virtual rrid_t reads(char *to, unsigned int &size, int flags);
	// Event handling.
	void on_read(const char *src, unsigned int size);
	void on_write(unsigned int transferred);
	void on_connect(void);
	void on_accept(const sock_t &peer, SOCKET s = INVALID_SOCKET);
	// Changes the context.
	void set_context(void *_ctx) { context = _ctx; }
	// Changes the dispatcher.
	void set_dispatcher(rascal_dispatcher _disp) { disp = _disp; }
	// Returns information about the queue.
	unsigned int get_rq_size(void);
	unsigned int get_sq_size(void);
};


// Base class for asynchronous operations.
class overlapped : public OVERLAPPED
{
	friend DWORD connect_ex_thread(LPVOID);
protected:
	DECLARE_ALLOCATOR;
	// Object that this request corresponds to.
	connection *host;
	// Opaque data buffer.
	char data[OVERLAPPED_DATA_SIZE];
	// Per-socket error code.
	int psec;
	// Returns the error code.
	rrid_t get_psec(void) const;
	// Leave me alone!
	bool lma;
	// Several wrappers to give child classes acces to the host
	// without requiring to add them all to the friend list.
	sock_t& get_peer(void) { return host->peer; }
	SOCKET& get_socket(void) { return host->s; }
	mutex& get_mutex(void) { return host->mx; }
	rascal_dispatcher& get_disp(void) { return host->disp; }
	void *& get_context(void) { return host->context; }
	bool& is_writing(void) { return host->is_writing; }
	datachain *& get_wrb(void) { return host->wrb; }
	datachain *& get_rdb(void) { return host->rdb; }
public:
	DECLARE_ALLOCATORS;
	overlapped(connection *);
	virtual ~overlapped(void);
	// Returns the base object.
	OVERLAPPED* get_base(void);
	// Event handler.
	virtual void on_event(unsigned int transferred, bool failed) = 0;
	// Restores the object.
	static overlapped* restore(OVERLAPPED *);
	// Returns lma.
	bool lia(void) const { return lma; }
};

#endif // __rascal_nt_connection_h
