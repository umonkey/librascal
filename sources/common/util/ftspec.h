// Faerion RunTime Library.
// Copyright (c) 2003-2004 hex@faerion.oss and others.
// Distributed under the terms of GNU LGPL, read 'LICENSE'.
//
// $Id: ftspec.h 3 2006-12-16 00:47:17Z justin.forest $

#ifndef __librascal_common_util_ftspec_h
#define __librascal_common_util_ftspec_h

class ftspec
{
public:
	unsigned int sec, msec;
	ftspec(unsigned int _msec = ~0U);
	ftspec(const ftspec &src);
	ftspec& operator += (const ftspec &src);
	ftspec& operator -= (const ftspec &src);
	ftspec& operator = (const ftspec &src);
	ftspec& operator = (unsigned int _msec);
	bool operator < (const ftspec &src);
	bool operator > (const ftspec &src);
	ftspec operator - (const ftspec &a);
	ftspec operator + (unsigned int _msec);
	void update();
	unsigned int mseconds();
};

void gettime(unsigned int *sec, unsigned int *msec);

#endif // __librascal_common_util_ftspec_h
