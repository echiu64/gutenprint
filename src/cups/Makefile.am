## $Id$
## Copyright (C) 2000 Roger Leigh
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

## Process this file with automake to produce Makefile.in.

AUTOMAKE_OPTIONS = 1.4 foreign

@SET_MAKE@

MAINT_CHARSET = latin1

PACKAGE = cups
datadir = @cups_datadir@
libdir = @cups_libdir@
sysconfdir = @cups_sysconfdir@

CFLAGS = @CFLAGS@ $(CUPS_CFLAGS)
DEFS = @DEFS@ -I. -I$(srcdir) -I../.. -DCUPS_DATADIR=\"$(cups_exec_prefix)/share/cups\" -DGIMP_PRINT_VERSION=\"$(VERSION)\"
INCLUDES = @INCLUDES@

## Programs

bin_PROGRAMS = @CUPS_BIN@

noinst_PROGRAMS = @CUPS_NOINST_BIN@

EXTRA_PROGRAMS = cups_calibrate commandtoepson epson genppd rastertoprinter commandtocanon canon

cups_calibrate_LDADD = -lm

commandtoepson_LDADD = $(CUPS_LIBS)

canon_LDADD = $(CUPS_LIBS)

epson_LDADD = $(CUPS_LIBS)

genppd_LDADD = $(CUPS_LIBS) $(GIMPPRINT_LIBS)

rastertoprinter_LDADD = $(CUPS_LIBS) $(GIMPPRINT_LIBS)

## Data

cups_backend = epson canon
cups_filter = rastertoprinter commandtoepson commandtocanon

if CUPS_INSTALL
install-data-local: ppd
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	$(mkinstalldirs) $(DESTDIR)$(libdir)/$(PACKAGE)
	$(mkinstalldirs) $(DESTDIR)$(libdir)/$(PACKAGE)/backend
	$(INSTALL) $(cups_backend) $(DESTDIR)$(libdir)/$(PACKAGE)/backend/
	$(mkinstalldirs) $(DESTDIR)$(libdir)/$(PACKAGE)/filter
	$(INSTALL) $(cups_filter) $(DESTDIR)$(libdir)/$(PACKAGE)/filter/
	$(mkinstalldirs) $(DESTDIR)$(libdir)/$(PACKAGE)/model
	cd ppd ; \
	for ppdfile in * ; do \
	  $(INSTALL_DATA) $$ppdfile $(DESTDIR)$(datadir)/$(PACKAGE)/model/ ; \
	done
	$(INSTALL_DATA) calibrate.ppm $(DESTDIR)/$(datadir)/$(PACKAGE)
	$(mkinstalldirs) $(DESTDIR)/$(sysconfdir)
	$(mkinstalldirs) $(DESTDIR)/$(sysconfdir)/$(PACKAGE)
	$(INSTALL_DATA) command.types $(DESTDIR)/$(sysconfdir)/$(PACKAGE)/command.types
endif

## Rules

.PHONY: ppd

ppd:    genppd
	-rm -r ppd/*
	./genppd

$(GIMPPRINT_LIBS):
	make -C ../main

dist-hook:
	for dir in commands ; do \
	  if test -d $$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $$dir/*; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir; \
	      fi \
	    done \
	  fi \
	done

clean-local:
	-rm -rf ppd

CLEANFILES = $(EXTRA_PROGRAMS)
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
	COPYING \
	README \
	ChangeLog \
	cups_calibrate.c \
	calibrate.ppm \
	canon.c \
	command.txt \
	command.types \
	commandtocanon.c \
	commandtoepson.c \
	epson.c \
	genppd.c \
	rastertoprinter.c
