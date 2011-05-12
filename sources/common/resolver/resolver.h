// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: resolver.h 3 2006-12-16 00:47:17Z justin.forest $

#ifndef __rascal_resolver_h
#define __rascal_resolver_h

#include "../util/ftspec.h"
#include "../common.h"
#include "../object.h"

#define NAMESERVER_COUNT 16

class hostname_t
{
public:
	char data[256];
};

// DNS packet builder class.  Only used for temporary operations,
// thus the allocators are declared private and not implemented.
// The class parses incoming response packets and builds outgoing
// request packets.
class header
{
	void* operator new(size_t);
	void operator delete(void *);
public:
	unsigned short id;
	unsigned short qr:1;
	unsigned short opcode:4;
	unsigned short aa:1;
	unsigned short tc:1;
	unsigned short rd:1;
	unsigned short ra:1;
	unsigned short z:3;
	unsigned short rcode:4;
	unsigned short qdcount;
	unsigned short ancount;
	unsigned short nscount;
	unsigned short arcount;
	// Original source.
	const char *head;
public:
	enum qtype {
		TYPE_A     =  1, // a host address
		TYPE_NS    =  2, // an authoritative name server
		TYPE_CNAME =  5, // the canonical name for an alias
		TYPE_SOA   =  6, // state of authority
		TYPE_PTR   = 12, // domain name pointer
		TYPE_HINFO = 13, // host information
		TYPE_MINFO = 14, // mailbox or mail list information
		TYPE_MX    = 15, // mail exchange
		TYPE_TXT   = 16, // text strings
		TYPE_SRV   = 33, // service resolution
	};

	enum qclass {
		CLASS_IN = 1, // the internet
		CLASS_CH = 3, // the CHAOS class
		CLASS_HS = 4, // Hesiod [Dyer 87]
	};
	// Construction.
	header(unsigned short id = 0);
	// Data retrieval.
	inline bool get(const char *&src, unsigned int &size, unsigned short &value)
	{
		if (size < 2)
			return false;

		value  = *(unsigned char *)src++ << 8;
		value += *(unsigned char *)src++;
		size -= 2;

		return true;
	}
	inline bool get(const char *&src, unsigned int &size, unsigned int &value)
	{
		if (size < 4)
			return false;

		value  = *(unsigned char *)src++ << 24;
		value |= *(unsigned char *)src++ << 16;
		value |= *(unsigned char *)src++ << 8;
		value |= *(unsigned char *)src++;
		size -= 4;

		return true;
	}
	// Retreives a domain name.
	bool get(const char *&src, unsigned int &size, char *dst, unsigned int dsize);
	inline bool put(char *&dst, unsigned int &size, unsigned short value)
	{
		if (size < 2)
			return false;

		*(unsigned char *)dst++ = ((value >> 8) & 0xff);
		*(unsigned char *)dst++ = (value & 0xff);
		size -= 2;

		return true;
	}
	bool put(char *&dst, unsigned int &size, const char *hostname);
	bool put(char *&dst, unsigned int &size, const addr_t &addr);
	// Skips a domain name.
	bool skip_name(const char *&src, unsigned int &size);
	// Skips the request data copied to the response.
	bool skip_echo(const char *&src, unsigned int &size);
	// Packet serialization.
	bool parse(const char *& src, unsigned int &size);
	bool dump(char *&dst, unsigned int &size);
	// Data retrieval.
	unsigned int get_addrs(const char *& src, unsigned int & size, addr_t *addrv, unsigned int addrc);
	unsigned int get_hosts(const char *& src, unsigned int & size, hostname_t *hostv, unsigned int hostc);
};


// Request information carrier class.  The work queue of the resolver is
// made of such objects.  When a reply is read, the matching request is
// found and the on_event() method is executed and receives the reply
// information.  Then the object is removed from the queue.  The object
// also carries all necessary callback data.
class request : public object
{
protected:
	DECLARE_ALLOCATOR;
	static mutex mx; // chain locker
	static request *queue; // chain head
	static unsigned short lastid; // last request id
protected:
	request *next; // chain linker
	ftspec tstamp; // request creation time
	unsigned int rcount; // repeat count
	unsigned short id;
	void *context;
	void *disp;
	void *filter;
	bool listed; // whether the object is in queue or not
	char data[256];
	void *data2;
	void *data3;
protected:
	// Sends the request to all available name servers.
	rrid_t flush(void);
	// Finds a request by id.
	static request* pick(unsigned short id);
	// Notifies the caller about a failed request.
	virtual void cancel(void);
	// Removes the request from the queue.
	void unlist(void);
public:
	DECLARE_ALLOCATORS;
	DECLARE_CLASSID(request);
	request(void); // initialization
	virtual ~request(void); // deinitialization
	virtual void on_event(header &hdr, const char *src, unsigned int size);
	virtual bool dump(char *&dst, unsigned int &size);
	static void on_read(rrid_t conn);
	static void on_write(rrid_t conn);
	// Monitor thread.
	static void * monitor(void *);
	// Tell the monitor that there is a request to serve.
	static void wake_monitor(void);
	// Monitor thread working cycle.
	static bool cycle_monitor(request *queue);
};


// Hostname resolution request.
class getaddr : public request
{
	typedef rascal_getaddr_callback callback;
	void on_event(header &hdr, const char *src, unsigned int size);
	callback & get_disp(void) { return * reinterpret_cast<callback *>(&disp); }
protected:
	bool dump(char *&dst, unsigned int &size);
	void cancel(void);
public:
	DECLARE_ALLOCATORS;
	DECLARE_CLASSID(getaddr);
	getaddr(const char *hostname, rascal_getaddr_callback, void *context, rrid_t &rc);
};


// Numeric address resolution request.
class gethost : public request
{
	typedef rascal_gethost_callback callback;
	void on_event(header &hdr, const char *src, unsigned int size);
	callback & get_disp(void) { return * reinterpret_cast<callback *>(&disp); }
	addr_t & get_addr(void) { return * reinterpret_cast<addr_t *>(data); }
protected:
	bool dump(char *&dst, unsigned int &size);
	void cancel(void);
public:
	DECLARE_ALLOCATORS;
	DECLARE_CLASSID(gethost);
	gethost(const addr_t *addr, rascal_gethost_callback, void *context, rrid_t &rc);
};


// Service resolution request.
class getsrv : public request
{
	// Response record (IN SRV).  Each getsrv request has a list
	// of these.  The list is filled during the first phase of
	// resolution (SRV lookup), then it is used to spawn IN A
	// lookups to get real service addresses (see below).
	class srv {
		DECLARE_ALLOCATOR;
	public:
		DECLARE_ALLOCATORS;
		srv *next;
		unsigned short id;
		unsigned short priority;
		unsigned short weight;
		unsigned short port;
		char host[64];
		// Initializers.
		srv(void) { }
		srv(srv &src) {
			priority = src.priority;
			weight = src.weight;
			port = src.port;
		}
		// Statistics.
		unsigned int depth(void) const;
	};
	// This class stores responses to IN A lookups.  There is a
	// plain list of these associated with each getsrv request.
	// The information from SRV records is copied here to simplify
	// further processing.
	class ina {
		DECLARE_ALLOCATOR;
	public:
		DECLARE_ALLOCATORS;
		ina *next;
		// Copied from the SRV record.
		unsigned short priority;
		unsigned short weight;
		// The real address of the service.
		sock_t peer;
		// Statistics.
		unsigned int depth(void) const;
	};
	// Event processing.
	void on_event(header &hdr, const char *src, unsigned int size);
	// Process one SRV record.
	bool on_record(header &hdr, const char *src, unsigned int size, rrid_t &);
	// Request parameters.
	void *& get_context(void) { return context; }
	rascal_dispatcher & get_disp(void) { return *(rascal_dispatcher*)&disp; }
	rascal_rcs_filter & get_filter(void) { return *(rascal_rcs_filter*)&filter; }
	srv * get_srv(void) { return (srv*)data2; }
	ina * get_ina(void) { return (ina*)data3; }
	void set_srv(srv *val) {* reinterpret_cast<srv **>(&data2) = val; }
	void set_ina(ina *val) {* reinterpret_cast<ina **>(&data3) = val; }
	// Hostname resolution handler.
	static void __rascall on_getaddr(void *context, const char *host, unsigned int addrc, const addr_t *addrv);
	// Appends all received numeric addresses to the list.
	void on_getaddr(unsigned int addrc, const addr_t *addrv, unsigned short id);
	// Attempts to connect to the first address in the list.
	void try_next_server(void);
	// Connection dispatcher.
	static bool __rascall try_next_server_disp(rrid_t rid, const sock_t *peer, int event, void *context);
	// Sorting stub.
	void sort(void);
protected:
	bool dump(char *&dst, unsigned int &size);
public:
	DECLARE_ALLOCATORS;
	DECLARE_CLASSID(getsrv);
	getsrv(const char *hostname, rascal_dispatcher, void *context, rascal_rcs_filter, rrid_t &rc);
	~getsrv(void);
};


// We currently support only one name server.
extern sock_t ns_addr[NAMESERVER_COUNT];

// Resolver connection.
extern rrid_t ns_rids[NAMESERVER_COUNT];

// Resolver mode (SOCK_STREAM, SOCK_DGRAM).
extern const char ns_mode[];

// Resolver dispatcher.
extern bool __rascall ns_dispatcher(rrid_t conn, const sock_t *peer, int event, void *context);

#endif // __rascal_resolver_h
