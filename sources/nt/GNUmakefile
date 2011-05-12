# Makefile for GNU Make.
# $Id: GNUmakefile 21 2005-01-23 00:03:02Z hex $

include ../../Makedefs.faerion.include

TARGET = library.shared
PROG   = librascal.$(MDF_SOEXT)
DIRS   = ../common
DEFS  += BUILD_RASCAL RASCAL_HELPERS
SRCS   = $(wildcard *.rc *.cc)
ARCH   = ../common/rascal.a
LIBS   = ws2_32 mswsock iphlpapi pthreadGCE stdc++

ifneq ($(filter debug,$(OPTIONS)),)
  LIBS += flog
endif

include ../../Makedefs.faerion

install: all
	cp -f $(PROG) /usr/local/bin/
	cp -f $(PROG).a /usr/local/lib/
	cp -f ../rascal.h /usr/local/include/
