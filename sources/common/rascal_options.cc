// RASCAL: Realtime Asynchronous Connection Abstraction Layer.
// Copyright (c) 2003-2004 hex@faerion.oss
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: rascal_options.cc 2 2005-04-17 21:09:06Z vhex $

#include "common.h"

static struct option {
	long int value;
	bool readonly;
} options[] = {
	{                 0, false }, // RO_VOID
	{              2500, false }, // RO_DNS_TIMEOUT
	{                 4, false }, // RO_DNS_RETRY
	{ RIP_WORKER_SINGLE, false }, // RO_THREAD_POLICY
	{                 5, false }, // RO_CONN_TIMEOUT
};


rrid_t rascal_set_option(unsigned int optid, long int value, long int *old_value)
{
	if (optid >= dimof(options))
		return REC_INVALID_ARGUMENT;
	else {
		struct option *opt = options + optid;

		if (old_value != NULL)
			*old_value = opt->value;

		if (opt->readonly)
			return REC_OPTION_READONLY;

		opt->value = value;

		if (optid == RO_THREAD_POLICY)
			opt->readonly = true;

		return REC_SUCCESS;
	}
}


rrid_t rascal_get_option(unsigned int optid, long int *value)
{
	if (optid >= dimof(options))
		return REC_INVALID_ARGUMENT;
	else {
		if (value != NULL)
			*value = options[optid].value;
		return REC_SUCCESS;
	}
}
