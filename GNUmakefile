# Makefile for GNU Make.
# $Id: GNUmakefile 19 2005-04-18 16:46:39Z vhex $

-include Makedefs.faerion.include

DIRS = sources/$(MDF_LIBARCH)
PACK = librascal
VERS = 2.0
PREQ = configure

ifeq ($(OSNAME),nt)
  PACKAGES = $(PACK)-$(VERS).zip
endif

include Makedefs.faerion

fake: all
	rm -rf fake-$(VERS)
	mkdir -p fake-$(VERS)/bin
	mkdir -p fake-$(VERS)/include
	mkdir -p fake-$(VERS)/lib
	mkdir -p fake-$(VERS)/share/rascal
	cp LICENSE fake-$(VERS)/share/rascal
	cp sources/rascal.h fake-$(VERS)/include/
	cp sources/$(MDF_LIBARCH)/librascal.$(MDF_SOEXT) fake-$(VERS)/lib
ifeq ($(OSNAME),nt)
	cp sources/$(MDF_LIBARCH)/librascal.$(MDF_SOEXT).a fake-$(VERS)/lib
endif

install: fake
  ifeq ($(PREFIX),)
	@echo "The PREFIX envar is not defined."
  else
	cp -R fake-$(VERS)/* $(PREFIX)/ && rm -rf fake-$(VERS)
  endif

tidy:
	find . -type f \( -name '*.o' -o -name '.*' -o -name '*.dylib' \) | xargs rm -rf docbook/html Makedefs.faerion.include configure.log

dist-nt:
	rm -rf librascal-$(VERSION) librasca-$(VERSION)-win.zip
	mkdir -p librascal-$(VERSION)/lib librascal-$(VERSION)/include
	cp sources/nt/librascal.dll sources/nt/librascal.dll.a librascal-$(VERSION)/lib
	cp sources/rascal.h librascal-$(VERSION)/include
	zip -r librascal-$(VERSION)-win.zip librascal-$(VERSION)
	ls -ld librascal-$(VERSION)-win.zip

dist: dist-default $(if $(OSNAME)=nt,dist-nt,)
