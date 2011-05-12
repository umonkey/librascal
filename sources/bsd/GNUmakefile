# Makefile for GNU Make.
# $Id: GNUmakefile 17 2005-04-18 16:30:38Z vhex $

include ../../Makedefs.faerion.include

TARGET  = library.shared
PROG    = librascal.$(MDF_SOEXT)
SRCS    = $(wildcard *.cc)
DEFS   += BUILD_RASCAL RASCAL_HELPERS
ARCH    = ../common/rascal.a
LIBS   += stdc++
DIRS    = ../common
EXPS    = librascal.exp

ifneq ($(OSNAME),nt)
  LIBS += resolv
endif

ifeq ($(OSNAME),darwin)
# -exported_symbols_list filename
endif

include ../../Makedefs.faerion

install: all
	test -d $(PREFIX)/include || mkdir -p $(PREFIX)/include
	test -d $(PREFIX)/lib || mkdir -p $(PREFIX)/lib
	cp -f $(PROG) $(PREFIX)/lib/
	cp -f ../rascal.h $(PREFIX)/include/
