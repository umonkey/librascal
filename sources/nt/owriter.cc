// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: owriter.cc 1 2005-04-17 19:33:05Z vhex $
//
// Overlapped writing.  I wish this class was as independent as reader is.
// However, we can't avoid accessing the connection's outgoing buffer from
// here directly.

#include "connection_st.h"
#include "../common/datachain.h"

owriter::owriter(connection *host, bool force_idle) :
	overlapped(host)
{
	DWORD dummy = 0;
	unsigned int size = sizeof(data);

	get_mutex().enter();

	if (force_idle)
		is_writing() = false;

	// If the connection is already being written to, drop
	// this request without any notification.
	if (is_writing()) {
		get_mutex().leave();
		delete this;
		return;
	}

	get_wrb()->extract(data, size);

	if (size == 0) {
		get_mutex().leave();
		delete this;
		return;
	}

	is_writing() = true;
	get_mutex().leave();

	if (WriteFile((HANDLE)get_socket(), data, size, &dummy, get_base()) || GetLastError() == ERROR_IO_PENDING)
		return;

	host->cancel();
	delete this;
}


owriter::~owriter()
{
}


void owriter::on_event(unsigned int transferred, bool failed)
{
	if (failed || psec != 0 || transferred == 0)
		host->cancel();
	else
		host->on_write(transferred);
}


void owriter::callback(DWORD dwError, DWORD dwTransferred, WSAOVERLAPPED *olp, DWORD)
{
	owriter *r = reinterpret_cast<owriter *>(reinterpret_cast<char *>(olp) - 4);
	r->on_event(dwTransferred, dwError ? true : false);
	delete r;
}
