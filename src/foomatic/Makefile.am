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

AM_CFLAGS = $(GNUCFLAGS)
INCLUDES = @INCLUDES@ $(LIBGIMPPRINT_CFLAGS) -I$(top_srcdir)/src/main
GIMPPRINT_LIBS = $(top_builddir)/src/main/libgimpprint.la
GIMPPRINT_DEPLIBS = -lm
LIBGIMPPRINT_LIBS = $(GIMPPRINT_LIBS) $(GIMPPRINT_DEPLIBS)
LIBPRINTUT = $(top_builddir)/lib/libprintut.la
LIBS = $(INTLLIBS) @LIBS@


## Programs

if BUILD_FOOMATIC
noinst_PROGRAMS = printer_options paper_sizes gimp-print-version \
	stp_limits printers
endif

printer_options_SOURCES = printer_options.c
printer_options_LDADD = $(LIBPRINTUT) $(LIBGIMPPRINT_LIBS)

gimp_print_version_SOURCES = gimp-print-version.c
gimp_print_version_LDADD = $(LIBPRINTUT) $(LIBGIMPPRINT_LIBS)

paper_sizes_SOURCES = paper_sizes.c
paper_sizes_LDADD = $(LIBPRINTUT) $(LIBGIMPPRINT_LIBS)

stp_limits_SOURCES = stp_limits.c
stp_limits_LDADD = $(LIBPRINTUT) $(LIBGIMPPRINT_LIBS)

printers_SOURCES = printers.c
printers_LDADD = $(LIBPRINTUT) $(LIBGIMPPRINT_LIBS)


## Rules

if BUILD_FOOMATIC
INSTALL_DATA_LOCAL_DEPS = foomatic-data
else
INSTALL_DATA_LOCAL_DEPS =
endif

if BUILD_IJS
FOOMATIC_GENERATOR_EXTRA_OPTS = -d all
FOOMATIC_IJS = yes
else
FOOMATIC_GENERATOR_EXTRA_OPTS = -d noijs
FOOMATIC_IJS =
endif

all-local: $(INSTALL_DATA_LOCAL_DEPS)

install-data-local: $(INSTALL_DATA_LOCAL_DEPS)
	if test -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
	  $(MAKE) install-kit FOOMATIC_DB=foomatic-db/gimp-print ; \
	  if test -n "$(FOOMATIC_IJS)" ; then \
	    $(MAKE) install-kit FOOMATIC_DB=foomatic-db/gimp-print-ijs ; \
	  fi ; \
	fi

install-kit:
	if test -n "$(DESTDIR)" ; then \
	  $(mkinstalldirs) $(DESTDIR)`$(FOOMATIC_KITLOAD) -l` ; \
	  $(FOOMATIC_KITLOAD) -f -d $(DESTDIR) -k $(FOOMATIC_DB) ; \
	else \
	  $(FOOMATIC_KITLOAD) -f -k $(FOOMATIC_DB) ; \
	fi

clean-local:
	-$(RM) -r foomatic-db

foomatic-data: foomatic-stamp

foomatic-stamp: foomatic-generator foomatic-printermap $(noinst_PROGRAMS) mk_printerlist
	-$(RM) -r foomatic-db
	./foomatic-generator -m $(srcdir)/foomatic-printermap -t $(srcdir)/foomatic-templates $(FOOMATIC_GENERATOR_EXTRA_OPTS)
	touch foomatic-stamp

$(GIMPPRINT_LIBS):
	cd ../main ; \
	$(MAKE)


## Clean

CLEANFILES = foomatic-stamp
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = foomatic-printermap 		\
	foomatic-templates-gs/*.xml		\
	foomatic-templates-ijs/*.xml

