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

bindir = @cups_bindir@
datadir = @cups_datadir@
libdir = @cups_libdir@
sysconfdir = @cups_sysconfdir@

pkgdatadir = $(datadir)/$(PACKAGE)
pkglibdir = $(libdir)/$(PACKAGE)
pkgsysconfdir = $(sysconfdir)/$(PACKAGE)

cupsexec_backenddir = $(pkglibdir)/backend
cupsexec_filterdir = $(pkglibdir)/filter
cups_modeldir = $(pkgdatadir)/model

CFLAGS = @CFLAGS@ $(CUPS_CFLAGS)
DEFS = @DEFS@ -I. -I$(srcdir) -I../.. -DCUPS_DATADIR=\"$(pkgdatadir)\" -DGIMP_PRINT_VERSION=\"$(VERSION)\"
INCLUDES = @INCLUDES@ $(LIBGIMPPRINT_CFLAGS)
LIBS = @LIBS@ ../../lib/libprintut.la $(INTLLIBS) ../../lib/libprintut.la

## Programs

bin_PROGRAMS = @CUPS_BIN@

cupsexec_backend_PROGRAMS = @CUPS_BACKEND_BIN@

cupsexec_filter_PROGRAMS = @CUPS_FILTER_BIN@

noinst_PROGRAMS = @CUPS_NOINST_BIN@

EXTRA_PROGRAMS = cups-calibrate commandtoepson epson genppd rastertoprinter commandtocanon canon

cups_calibrate_LDADD = -lm

commandtoepson_LDADD = $(CUPS_LIBS)

canon_LDADD = $(CUPS_LIBS)

epson_LDADD = $(CUPS_LIBS)

genppd_LDADD = $(CUPS_LIBS) $(LIBGIMPPRINT_LIBS)

genppd_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)

rastertoprinter_LDADD = $(CUPS_LIBS) $(LIBGIMPPRINT_LIBS)

rastertoprinter_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)


## Data

pkgdata_DATA = @CUPS_PKG@

pkgsysconf_DATA = @CUPS_CONF@

if BUILD_CUPS
INSTALL_DATA_LOCAL_DEPS = ppd
endif

if BUILD_CUPS
INSTALL_PPD = true
else
INSTALL_PPD =
endif

install-data-local: $(INSTALL_DATA_LOCAL_DEPS)
	if test -n "$(INSTALL_PPD)" ; then \
	  $(mkinstalldirs) $(DESTDIR)$(cups_modeldir); \
	  cd ppd ; \
	  for ppdfile in * ; do \
	    $(INSTALL_DATA) $$ppdfile $(DESTDIR)$(cups_modeldir) ; \
	  done \
	fi

install-data-hook:
# Remove unused directories in install tree
	-rmdir $(DESTDIR)$(cups_modeldir)
	-rmdir $(DESTDIR)$(pkgdatadir)
	-rmdir $(DESTDIR)$(datadir)
	-rmdir $(DESTDIR)$(cupsexec_backenddir)
	-rmdir $(DESTDIR)$(cupsexec_filterdir)
	-rmdir $(DESTDIR)$(pkglibdir)
	-rmdir $(DESTDIR)$(libdir)
	-rmdir $(DESTDIR)$(pkgsysconfdir)
	-rmdir $(DESTDIR)$(sysconfdir)
	-rmdir $(DESTDIR)$(bindir)

## Rules

.PHONY: ppd

ppd:    genppd
	-rm -rf ppd
	./genppd

$(LIBGIMPPRINT_LIBS):
	$(MAKE) -C ../main

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

install: @CUPS_INSTALL@

clean-local:
	-rm -rf ppd

CLEANFILES = $(EXTRA_PROGRAMS)
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
	COPYING \
	README \
	cups-calibrate.c \
	calibrate.ppm \
	canon.c \
	command.txt \
	command.types \
	commandtocanon.c \
	commandtoepson.c \
	epson.c \
	genppd.c \
	rastertoprinter.c
