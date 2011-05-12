// Faerion RunTime Library.
// Copyright (c) 2002-2004 hex@faerion.oss and others.
// $Id: ftspec.cc 2 2005-04-17 21:09:06Z vhex $

#include "ftspec.h"

ftspec::ftspec(unsigned int _msec)
{
	if (_msec == ~0U)
		update();
	else
		*this = _msec;
}


ftspec::ftspec(const ftspec &src)
{
	sec = src.sec;
	msec = src.msec;
}


void ftspec::update()
{
	gettime(&sec, &msec);
}


ftspec& ftspec::operator += (const ftspec &src)
{
	sec += src.sec;
	msec += src.msec;
	if (msec >= 1000) {
		sec += msec / 1000;
		msec %= 1000;
	}
	return *this;
}


ftspec& ftspec::operator -= (const ftspec &src)
{
	sec -= src.sec;
	if (msec < src.msec) {
		msec += 1000;
		--sec;
	}
	msec -= src.msec;
	return *this;
}


ftspec& ftspec::operator = (const ftspec &src)
{
	sec = src.sec;
	msec = src.msec;
	return *this;
}


bool ftspec::operator < (const ftspec &src)
{
	if (sec < src.sec)
		return true;
	if (sec == src.sec && msec < src.msec)
		return true;
	return false;
}


bool ftspec::operator > (const ftspec &src)
{
	if (sec > src.sec)
		return true;
	if (sec == src.sec && msec > src.msec)
		return true;
	return false;
}


ftspec& ftspec::operator = (unsigned int _msec)
{
	sec = _msec / 1000;
	msec = _msec % 1000;
	return *this;
}


ftspec ftspec::operator - (const ftspec &src)
{
	ftspec tmp(*this);
	tmp -= src;
	return tmp;
}


ftspec ftspec::operator + (unsigned int _msec)
{
	ftspec tmp(*this);
	tmp += _msec;
	return tmp;
}


unsigned int ftspec::mseconds()
{
	return sec * 1000 + msec;
}
