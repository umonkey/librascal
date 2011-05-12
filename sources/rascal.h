/*
 * RASCAL: Realtime Asynchronous Connection Abstraction Layer.
 * Copyright (c) 2003-2004 hex@faerion.oss
 *
 * The complete documentation can be found at the following location:
 * <http://faerion.sourceforge.net/doc/rascal/>
 *
 * The library is distributed under the terms of GNU LGPL.
 *
 * $Id: rascal.h 3 2006-12-16 00:47:17Z justin.forest $
 *
 */

#ifndef __rascal_h
#define __rascal_h

#ifdef _WIN32
# define __rascall __stdcall
# define RASCAL_EXP __declspec(dllexport)
# define RASCAL_IMP __declspec(dllimport)
#else
# define __rascall
# define RASCAL_EXP
# define RASCAL_IMP
#endif

typedef struct addr_t
{
	unsigned int length;
	unsigned char data[16];
#ifdef __cplusplus
	addr_t& operator = (const struct addr_t &src) {
		length = src.length;
		* (reinterpret_cast<int *>(data) + 0) = * (reinterpret_cast<const int *>(src.data) + 0);
		* (reinterpret_cast<int *>(data) + 1) = * (reinterpret_cast<const int *>(src.data) + 1);
		* (reinterpret_cast<int *>(data) + 2) = * (reinterpret_cast<const int *>(src.data) + 2);
		* (reinterpret_cast<int *>(data) + 3) = * (reinterpret_cast<const int *>(src.data) + 3);
		return *this;
	}
	addr_t(const struct addr_t &src) { *this = src; }
	addr_t(unsigned int _length = 0) { length = _length; }
#endif
} addr_t;

typedef struct sock_t {
	struct addr_t addr;
	unsigned short port;
#ifdef __cplusplus
	sock_t(unsigned short _port = 0) { port = _port; }
	sock_t(const sock_t &src) {
		port = src.port;
		addr = src.addr;
	}
	sock_t(const addr_t &src, unsigned short _port)
	{
		addr = src;
		port = _port;
	}
	operator sock_t* () { return this; }
#endif
#ifdef BUILD_RASCAL
	bool operator == (const sock_t &);
	bool operator != (const sock_t &src) { return !(*this == src); }
	sock_t& operator = (const sock_t &);
	void put(struct sockaddr &sa) const { put(* reinterpret_cast<struct sockaddr_in *>(&sa)); }
	void put(struct sockaddr_in &) const;
	void get(const struct sockaddr_in &);
	void get(const struct sockaddr &sa) { get(* reinterpret_cast<const struct sockaddr_in *>(&sa)); }
	sock_t(const struct sockaddr &sa) { get(sa); }
	sock_t(const struct sockaddr_in &sa) { get(sa); }
#endif
} sock_t;

typedef int rrid_t;

#if !defined(__cplusplus) && !defined(__bool_true_false_are_defined)
# include <stdbool.h>
#endif

#ifdef __cplusplus
# define RASCAL_OPTIONAL = 0
# ifdef _WIN32
#  define RASCAL_EXT extern "C"
# else
#  define RASCAL_EXT extern "C"
# endif
#else
# define RASCAL_OPTIONAL
# define RASCAL_EXT
#endif

#ifdef BUILD_RASCAL
# define RASCAL_API(type) RASCAL_EXT type __rascall RASCAL_EXP
#else
# define RASCAL_API(type) RASCAL_EXT type __rascall RASCAL_IMP
#endif

#define REC_SUCCESS          (int)0x00000000
#define REC_NOT_IMPLIMENTED  (int)0xE0000001
#define REC_INVALID_HANDLE   (int)0xE0000002
#define REC_INVALID_ARGUMENT (int)0xE0000003
#define REC_UNKNOWN_ERROR    (int)0xE0000004
#define REC_UNAVAILABLE      (int)0xE0000005
#define REC_INER_ASYNC       (int)0xE0000006
#define REC_INER_CONNECT     (int)0xE0000007
#define REC_INER_RESOLVER    (int)0xE0000008
#define REC_INER_NOWORKERS   (int)0xE0000009
#define REC_SRV_UNAVAIL      (int)0xE000000A
#define REC_SRV_NOTFOUND     (int)0xE000000B
#define REC_NO_DATA          (int)0xE000000C
#define REC_NO_NAMESERVERS   (int)0xE000000D
#define REC_OPTION_READONLY  (int)0xE000000E
#define REC_CONN_TIMEOUT     (int)0xE000000F
#define REC_CANCELLED        (int)0xE0000010
#define REC_BAD_PROTOCOL     (int)0xE0000011
#define REC_SYSERROR_MASK    (int)0x80000000

#define RIP_WORKER_MANUAL    0x00000000
#define RIP_WORKER_SINGLE    0x00000001
#define RIP_WORKER_PER_CPU   0x00000002
#define RIP_WORKER_MASK      0x00000003
#define RIP_NO_DNS           0x00000008

#define RRF_NORMAL           0x00000000
#define RRF_PEEK             0x00000001
#define RRF_MEASURE          0x00000002
#define RRF_UNTRIMMED        0x00000010

#define RO_VOID              0
#define RO_DNS_TIMEOUT       1
#define RO_DNS_RETRY         2
#define RO_THREAD_POLICY     3
#define RO_CONN_TIMEOUT      4

#define rascal_isok(rrid) (((rrid) & 0x80000000) == 0)

typedef bool (__rascall * rascal_dispatcher)(rrid_t conn, const sock_t *peer, int event, void *context);
typedef bool (__rascall * rascal_rcs_filter)(void *context, const sock_t *addr);
typedef void (__rascall * rascal_getaddr_callback)(void *context, const char *host, unsigned int count, const addr_t *addrs);
typedef void (__rascall * rascal_gethost_callback)(void *context, const addr_t *addr, unsigned int count, const char **hosts);

RASCAL_API(rrid_t) rascal_accept(const sock_t *addr, rascal_dispatcher, void *context RASCAL_OPTIONAL);
RASCAL_API(bool)   rascal_aton(const char *pattern, addr_t *addr, addr_t *mask RASCAL_OPTIONAL);
RASCAL_API(rrid_t) rascal_cancel(rrid_t);
RASCAL_API(rrid_t) rascal_connect(const sock_t *target, rascal_dispatcher, void *context RASCAL_OPTIONAL, const char *proto RASCAL_OPTIONAL);
RASCAL_API(rrid_t) rascal_connect_service(const char *name, const char *proto, const char *domain, rascal_dispatcher, void *context RASCAL_OPTIONAL, rascal_rcs_filter RASCAL_OPTIONAL);
RASCAL_API(void)   rascal_get_errmsg(rrid_t, char *buffer, unsigned int size);
RASCAL_API(rrid_t) rascal_get_rq_size(rrid_t, unsigned int *);
RASCAL_API(rrid_t) rascal_get_sq_size(rrid_t, unsigned int *);
RASCAL_API(rrid_t) rascal_get_option(unsigned int optid, long int *value);
RASCAL_API(rrid_t) rascal_getaddr(const char *host, rascal_getaddr_callback, void *context RASCAL_OPTIONAL);
RASCAL_API(rrid_t) rascal_gethost(const addr_t *addr, rascal_gethost_callback, void *context RASCAL_OPTIONAL);
RASCAL_API(rrid_t) rascal_init(unsigned int policy);
RASCAL_API(rrid_t) rascal_accept_service(const char *name, const char *proto, const char *domain, rascal_dispatcher, void *context RASCAL_OPTIONAL);
RASCAL_API(void)   rascal_ntoa(const addr_t *, char *, unsigned int);
RASCAL_API(rrid_t) rascal_read(rrid_t, char *to, unsigned int *size);
RASCAL_API(rrid_t) rascal_reads(rrid_t, char *to, unsigned int *size, int flags RASCAL_OPTIONAL);
RASCAL_API(rrid_t) rascal_set_context(rrid_t, void *context);
RASCAL_API(rrid_t) rascal_set_dispatcher(rrid_t, rascal_dispatcher);
RASCAL_API(rrid_t) rascal_set_nameserver(const sock_t *peer, unsigned int count);
RASCAL_API(rrid_t) rascal_set_option(unsigned int optid, long int value, long int *old_value RASCAL_OPTIONAL);
RASCAL_API(void)   rascal_shrink(void);
RASCAL_API(rrid_t) rascal_wait(rrid_t);
RASCAL_API(rrid_t) rascal_write(rrid_t, const char *at, unsigned int size);

/* Operation types: */
enum rop_types {
	rop_accept,
	rop_close,
	rop_connect,
	rop_read,
	rop_write,
	rop_listen
};

#if defined(__cplusplus) && defined(RASCAL_HELPERS)
namespace rascal {
	class errmsg
	{
		char tmp[1024];
	public:
		errmsg(rrid_t rid) { rascal_get_errmsg(rid, tmp, sizeof(tmp)); }
		const char * c_str() const { return tmp; }
	};

	class ntoa
	{
		char tmp[256];
	public:
		ntoa(const sock_t &peer) { rascal_ntoa(&peer.addr, tmp, sizeof(tmp)); }
		ntoa(const addr_t &addr) { rascal_ntoa(&addr, tmp, sizeof(tmp)); }
		const char * c_str() const { return tmp; }
	};
};
#endif

#endif /* __rascal_h */
