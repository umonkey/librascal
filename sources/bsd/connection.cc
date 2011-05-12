// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: connection.cc 2 2005-04-17 21:09:06Z vhex $

#include <errno.h>
#include <string.h>
#ifndef _WIN32
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/ioctl.h>
# include <netinet/in.h>
# include <unistd.h>
#endif

#include "connection.h"
#include "../common/datachain.h"
#include "../common/debug.h"

IMPLEMENT_ALLOCATORS(connection);

static int makesock(int mode)
{
	int fd = socket(AF_INET, mode, mode == SOCK_STREAM ? IPPROTO_TCP : IPPROTO_UDP);

	if (fd != -1) {
		long int value = 1;
		debug((flog, rl_conn, "%s socket %d created", mode == SOCK_STREAM ? "stream" : "datagram", fd));

		if (ioctl(fd, FIONBIO, &value) != 0) {
			debug((flog, rl_conn, "could not set socket %d to non-blocking mode, closing", fd));
			close(fd);
			fd = -1;
		} else {
			debug((flog, rl_conn, "socket %d set to non-blocking mode", fd));
		}
	} else {
		debug((flog, rl_conn, "could not create a %s socket", mode == SOCK_STREAM ? "stream" : "datagram"));
	}

	return fd;
}


connection::connection(int mode)
{
	this->mode = cm_normal;
	this->fd = makesock(mode);

	debug((flog, rl_conn, "%X: connection created (mode %d)", get_id(), mode));
}


connection::connection(rascal_dispatcher _disp, void *_ctx, int fd)
{
	mode = cm_normal;
	disp = _disp;
	context = _ctx;
	rd = new datachain;
	wr = new datachain;
	selected = false;

	debug((flog, rl_conn, "%X: connection created (stream, context: %x)", get_id(), _ctx));

	this->fd = (fd == -1) ? makesock(SOCK_STREAM) : fd;
}


connection::~connection()
{
	if (rd != NULL)
		delete rd;
	if (wr != NULL)
		delete wr;
	if (fd >= 0) {
		close(fd);
		debug((flog, rl_conn, "%X: socket %d closed", get_id(), fd));
	}
	debug((flog, rl_conn, "%X: connection destroyed", get_id()));
}


bool connection::need_to_write()
{
	if (mode == cm_connect)
		return true;
	if (wr == NULL)
		return false;
	else {
		mlock lock(mx);
		return wr->has_data() ? true : false;
	}
}


void connection::on_error(int psec)
{
	int event = rop_close;

	psec |= REC_SYSERROR_MASK;

	if (mode == cm_connect)
		event = rop_connect;

	disp(psec, &peer, event, context);

	object::cancel();
}


void connection::on_pulse()
{
	long int limit;
	ftspec now;

	if (mode != cm_connect)
		return;

	if (rascal_isok(rascal_get_option(RO_CONN_TIMEOUT, &limit))) {
		long int current = (now - ts).mseconds();

		if (current >= limit * 1000) {
			disp(REC_CONN_TIMEOUT, &peer, rop_close, context);
			object::cancel();
			debug((flog, rl_conn, "%X: reporting connection timeout", get_id()));
		}
	}
}


void connection::on_read()
{
	bool success = false, closed = false;

	mx.enter();

	if (is_cancelled()) {
		mx.leave();
		return;
	}

	while (true) {
		char tmp[4096];
		int length = recv(fd, tmp, sizeof(tmp), 0);

		if (length < 0) {
			mx.leave();
			if (errno != EWOULDBLOCK && errno != EAGAIN)
				on_error(errno);
			break;
		}

		if (length == 0) {
			closed = true;
			mx.leave();
			break;
		}

		success = true;
		rd->append(tmp, length);
	}

	if (success) {
		if (mode == cm_connect) {
			debug((flog, rl_conn, "%X: reporting a successfull stream connection", get_id()));

			if (!disp(get_id(), &peer, rop_connect, context)) {
				cancel();
				return;
			}
			mode = cm_normal;
		}

		debug((flog, rl_conn, "%X: reporting data availability to connection", get_id()));
		disp(get_id(), &peer, rop_read, context);
	}

	if (closed) {
		debug((flog, rl_conn, "%X: reporting closure to connection", get_id()));
		disp(REC_SUCCESS, &peer, rop_close, context);
		object::cancel();
	}
}


void connection::on_write()
{
	bool success = false;

	mx.enter();

	if (is_cancelled()) {
		mx.leave();
		return;
	}

	while (true) {
		char tmp[4096];
		unsigned int size = sizeof(tmp);

		wr->peek(tmp, size);

		if (size == 0) {
			mx.leave();
			break;
		}

		if (send(fd, tmp, size, 0) < 0 && errno != EWOULDBLOCK && errno != EAGAIN) {
			mx.leave();
			on_error(errno);
			break;
		}

		success = true;
		wr->remove(size);
	}

	if (mode == cm_connect) {
		debug((flog, rl_conn, "%X: reporting a successfull stream connection", get_id()));
		if (!disp(get_id(), &peer, rop_connect, context)) {
			cancel();
			return;
		}
		mode = cm_normal;
	}

	if (success) {
		debug((flog, rl_conn, "%X: reporting successfull data departure", get_id()));
		disp(get_id(), &peer, rop_write, context);
	}
}


// Object filter.
bool connection::filter(object *tmp, void *)
{
	bool rc = false;
	const ot_t tid = tmp->get_object_ot();

	if (!tmp->is_cancelled() && (tid == connection::get_class_ot() || tid == connection_dg::get_class_ot())) {
		connection *conn = reinterpret_cast<connection *>(tmp);
		if (!conn->is_cancelled() && !conn->selected) {
			conn->on_pulse();
			conn->incref();
			rc = conn->selected = true;
		}
	}

	return rc;
}


rrid_t connection::work(unsigned int msec)
{
	object *arr[FD_SETSIZE];
	connection **carr = reinterpret_cast<connection **>(arr);
	unsigned int size = FD_SETSIZE;
	struct timeval wait;
	fd_set rds, wrs, exs;

	// Pick available connections.
	size = object::list(arr, FD_SETSIZE, connection::filter);

	// We used to sleep forever on zero delay.  This is wrong,
	// because this way we do not notice new sockets (outgoing
	// connection attempts).  If we could somehow force select
	// to return from an endless sleep (with a signal or other
	// facility), that would be perfect.  Now let's spin.
	if (msec == 0 || msec > 100)
		msec = 100;

	while (true) {
		int rc;
		int maxfd = 0;

		wait.tv_sec = msec / 1000;
		wait.tv_usec = (msec % 1000) * 1000;

		FD_ZERO(&rds);
		FD_ZERO(&wrs);
		FD_ZERO(&exs);

		// set up the socket map.
		for (unsigned int idx = 0; idx < size; ++idx) {
			connection *conn = carr[idx];

			if (conn->need_to_write())
				FD_SET(conn->fd, &wrs);
			FD_SET(conn->fd, &exs);
			FD_SET(conn->fd, &rds);

			if (conn->fd > maxfd)
				maxfd = conn->fd;
		}

		debug((flog, rl_conn, "letting select() sleep for %u msec.", msec));

		// do the work, return an error if failed.
		rc = select(maxfd+1, &rds, &wrs, &exs, msec ? &wait : NULL);

		if (rc == -1)
			return GetSysError;

		// process the results.
		for (unsigned int idx = 0; idx < size; ++idx) {
			connection *conn = carr[idx];

			// Only bother checking fd sets if select
			// actually selected anything.
			if (rc != 0) {
				// error conditions are unrecoverable and can not
				// be followed by a read or a write.
				if (FD_ISSET(conn->fd, &exs)) {
					int psec;
					socklen_t size = sizeof(psec);

					debug((flog, rl_conn, "%X: connection has an error (socket %d)", conn->get_id(), conn->fd));

					if (getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, (char *)&psec, &size)) {
						conn->on_error(psec);
						conn->selected = false;
						conn->decref();
						continue;
					}
				}

				// Some data can be read.
				if (!conn->is_cancelled() && FD_ISSET(conn->fd, &rds)) {
					debug((flog, rl_conn, "%X: connection is ready for reading (socket %d)", conn->get_id(), conn->fd));
					conn->on_read();
				}

				// Some data can be written.
				if (!conn->is_cancelled() && FD_ISSET(conn->fd, &wrs)) {
					debug((flog, rl_conn, "%X: connection is ready for writing (socket %d)", conn->get_id(), conn->fd));
					conn->on_write();
				}
			}

			// Release the connection to other threads.
			conn->selected = false;
			conn->decref();
		}

		break;
	}

	return REC_SUCCESS;
}


void* connection::worker(void *)
{
	while (true) {
		work(0);
	}
	return NULL;
}


unsigned int connection::get_sq_size()
{
	mlock lock(mx);
	return wr->get_size();
}


unsigned int connection::get_rq_size()
{
	mlock lock(mx);
	return rd->get_size();
}


rrid_t connection::write(const char *src, unsigned int size)
{
	mlock lock(mx);
	wr->append(src, size);
	return REC_SUCCESS;
}


rrid_t connection::read(char *dst, unsigned int &size)
{
	mlock lock(mx);
	rd->extract(dst, size);
	return REC_SUCCESS;
}


rrid_t connection::reads(char *dst, unsigned int &size, int flags)
{
	mlock lock(mx);
	return rd->gets(dst, size, flags);
}


rrid_t connection::connect(const sock_t &_peer)
{
	struct sockaddr sa;

	peer = _peer;
	peer.put(sa);

	if (::connect(fd, &sa, sizeof(sa)) == -1 && errno != EINPROGRESS)
		return errno | REC_SYSERROR_MASK;

	mode = cm_connect;
	ts.update();

	return REC_SUCCESS;
}
