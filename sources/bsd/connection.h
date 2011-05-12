// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connection.h 3 2006-12-16 00:47:17Z justin.forest $

#ifndef __rascal_bsd_connection_h
#define __rascal_bsd_connection_h

#include "../common/object.h"
#include "../common/common.h"
#include "../common/util/ftspec.h"

class datachain;

class connection : public object {
protected:
	DECLARE_ALLOCATOR;
	int fd; // connection socket
	sock_t peer; // remote connection endpoint
	datachain *rd, *wr; // data buffers
	rascal_dispatcher disp; // the dispatcher
	void *context; // user data
	bool selected; // in select()
	// The time when the connection was initiated.
	ftspec ts;
	// Thread safety.
	mutex mx;
	// Filter for connection selector.
	static bool filter(object *, void *);
	// Checks whether the outgoing queue is not empty.
	virtual bool need_to_write();
	// Used by datagram connections only.
	connection(int mode);
	// Connection mode.
	enum {
		cm_normal,
		cm_connect,
		cm_accept,
	} mode;
public:
	DECLARE_ALLOCATORS;
	connection(rascal_dispatcher, void *_ctx, int fd = -1);
	virtual ~connection();
	// Event handlers.
	virtual void on_error(int);
	virtual void on_read();
	virtual void on_write();
	virtual void on_pulse();
	// Event handling guards.
	void set_dispatching(bool);
	bool is_dispatching() const;
	// Change connection attributes.
	void set_context(void *_ctx) { context = _ctx; }
	void set_dispatcher(rascal_dispatcher _disp) { disp = _disp; }
	// Connection statistics.
	unsigned int get_sq_size();
	unsigned int get_rq_size();
	// Data related operations.
	rrid_t write(const char *, unsigned int);
	rrid_t read(char *, unsigned int &);
	rrid_t reads(char *, unsigned int &, int);
	// Starts connecting.
	virtual rrid_t connect(const sock_t &);
	// Does the work.
	static rrid_t work(unsigned int);
	// Worker thread body.
	static void* worker(void *);
	// Dynamic type identification.
	DECLARE_CLASSID(connection);
};


class connection_dg : public connection {
public:
	DECLARE_ALLOCATORS;
	connection_dg(rascal_dispatcher, void *_ctx);
	~connection_dg() { }
	// Binds the socket and delivers the event immediately.
	rrid_t connect(const sock_t &);
	// Event handlers.
	void on_error(int);
	void on_read();
	void on_write();
};


class connection_in : public connection {
public:
	DECLARE_ALLOCATORS;
	connection_in(rascal_dispatcher _disp, void *_ctx);
	~connection_in();
	// Starts listening for incoming connections.
	rrid_t connect(const sock_t &);
	// Event handlers.
	void on_read();
	void on_write();
protected:
	// Checks whether the outgoing queue is not empty.
	bool need_to_write();
};

#endif // __rascal_bsd_connection_h
