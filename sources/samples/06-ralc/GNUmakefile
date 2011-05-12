# Makefile for GNU Make.
# $Id: GNUmakefile 21 2005-01-23 00:03:02Z hex $

include ../../../Makedefs.faerion.include

TARGET  = executable
PROG    = ralc
SRCS    = $(wildcard *.cc)
LIBS   += rascal stdc++
LIBDIR += ../../$(MDF_LIBARCH)

ifeq ($(OSNAME),nt)
  PROG := $(PROG).exe
  SRCS += $(wildcard *.rc)
  LIBS += pthreadGCE
else
  LIBS += pthread
endif

include ../../../Makedefs.faerion
