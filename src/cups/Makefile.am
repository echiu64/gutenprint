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

AUTOMAKE_OPTIONS = 1.4 gnu

@SET_MAKE@

MAINT_CHARSET = latin1


## Variables

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

AM_CFLAGS = $(GNUCFLAGS)
DEFS = @DEFS@ -I. -I$(srcdir) -I../.. -DCUPS_DATADIR=\"$(pkgdatadir)\" -DGIMP_PRINT_VERSION=\"$(VERSION)\"
INCLUDES = @INCLUDES@ $(LIBGIMPPRINT_CFLAGS)
LIBS = @LIBS@ ../../lib/libprintut.la $(INTLLIBS) ../../lib/libprintut.la


## Programs

CUPS_BIN = cups-calibrate
CUPS_BACKEND_BIN = epson canon
CUPS_FILTER_BIN = rastertoprinter commandtoepson commandtocanon
CUPS_NOINST_BIN = genppd

bin_PROGRAMS = @CUPS_BIN@
cupsexec_backend_PROGRAMS = @CUPS_BACKEND_BIN@
cupsexec_filter_PROGRAMS = @CUPS_FILTER_BIN@
noinst_PROGRAMS = @CUPS_NOINST_BIN@

EXTRA_PROGRAMS = cups-calibrate commandtoepson epson genppd rastertoprinter commandtocanon canon

cups_calibrate_SOURCES = cups-calibrate.c
cups_calibrate_LDADD = -lm

commandtoepson_SOURCES = commandtoepson.c
commandtoepson_LDADD = $(CUPS_LIBS)

canon_SOURCES = canon.c
canon_LDADD = $(CUPS_LIBS)

epson_SOURCES = epson.c
epson_LDADD = $(CUPS_LIBS)

genppd_SOURCES = genppd.c
genppd_LDADD = $(CUPS_LIBS) $(LIBGIMPPRINT_LIBS)
genppd_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)

rastertoprinter_SOURCES = rastertoprinter.c
rastertoprinter_LDADD = $(CUPS_LIBS) $(LIBGIMPPRINT_LIBS)
rastertoprinter_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)


## Data

CUPS_PKG = calibrate.ppm
CUPS_CONF = command.types

pkgdata_DATA = @CUPS_PKG@
pkgsysconf_DATA = @CUPS_CONF@

if BUILD_CUPS
INSTALL_DATA_LOCAL_DEPS = ppd
endif

install-data-local: $(INSTALL_DATA_LOCAL_DEPS)
	if test -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
	  $(mkinstalldirs) $(DESTDIR)$(cups_modeldir); \
	  cd ppd ; \
	  for language in * ; do \
	    $(mkinstalldirs) $(DESTDIR)$(cups_modeldir)/$$language; \
	    cd $$language; \
	    for ppdfile in * ; do \
	      $(INSTALL_DATA) $$ppdfile $(DESTDIR)$(cups_modeldir)/$$language ; \
	    done \
	    cd ..; \
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

all-local: $(INSTALL_DATA_LOCAL_DEPS)

ppd: ppd-stamp
ppd-stamp: genppd
	-rm -rf ppd
	-mkdir ppd
	-mkdir ppd/de
	./genppd --prefix ppd/de --language de
	-mkdir ppd/en
	./genppd --prefix ppd/en --language en
	-mkdir ppd/es
	./genppd --prefix ppd/es --language es
	-mkdir ppd/fr
	./genppd --prefix ppd/fr --language fr
	-mkdir ppd/it
	./genppd --prefix ppd/it --language it
	touch ppd-stamp

$(LIBGIMPPRINT_LIBS):
	$(MAKE) -C ../main

dist-hook:
	for dir in commands ; do \
	  if test -d $(srcdir)/$$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $(srcdir)/$$dir/*; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir; \
	      fi \
	    done \
	  fi \
	done

clean-local:
	-rm -rf ppd


## Clean

CLEANFILES = $(EXTRA_PROGRAMS) ppd-stamp
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
	COPYING \
	README \
	calibrate.ppm \
	command.txt \
	command.types

