# Makefile for GNU Make.
# $Id: GNUmakefile 21 2005-01-23 00:03:02Z hex $

include ../../../Makedefs.faerion.include

TARGET  = executable
PROG    = raconns
DEFS   += RASCAL_HELPERS
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

run: all
	@echo " "
	@rm -f packet-?????.{in,out}
	@PATH=$(LIBDIR):$(PATH) ./$(PROG) $(ARGS)

debug: all
	@echo " "
	@rm -f packet-?????.{in,out}
	@PATH=$(LIBDIR):$(PATH) gdb $(PROG)
