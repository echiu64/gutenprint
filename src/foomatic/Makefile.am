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
INCLUDES = @INCLUDES@ $(GIMP_CFLAGS) $(LIBGIMPPRINT_CFLAGS) -I$(top_srcdir)/src/main
LIBS = @LIBS@ ../../lib/libprintut.la $(INTLLIBS) ../../lib/libprintut.la


## Programs

FOOMATIC_NOINST_BIN = printer_options paper_sizes gimp-print-version stp_limits

noinst_PROGRAMS = @FOOMATIC_NOINST_BIN@

EXTRA_PROGRAMS = printer_options gimp-print-version paper_sizes stp_limits

printer_options_SOURCES = printer_options.c
printer_options_LDADD = $(LIBGIMPPRINT_LIBS)
printer_options_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)

gimp_print_version_SOURCES = gimp-print-version.c
gimp_print_version_LDADD = $(LIBGIMPPRINT_LIBS)
gimp_print_version_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)

paper_sizes_SOURCES = paper_sizes.c
paper_sizes_LDADD = $(LIBGIMPPRINT_LIBS)
paper_sizes_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)

stp_limits_SOURCES = stp_limits.c
stp_limits_LDADD = $(LIBGIMPPRINT_LIBS)
stp_limits_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)


## Rules

if BUILD_FOOMATIC
INSTALL_DATA_LOCAL_DEPS = foomatic-data
else
INSTALL_DATA_LOCAL_DEPS =
endif

install-data-local: $(INSTALL_DATA_LOCAL_DEPS)
	if test -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
	  foomatic-kitload -k foomatic-db ; \
	fi

clean-local:
	-rm -rf foomatic-db

foomatic-data: stp_limits foomatic-generator
	-rm -rf foomatic-db
	./foomatic-generator

$(LIBGIMPPRINT_LIBS):
	$(MAKE) -C ../../src/main


## Clean

CLEANFILES = $(noinst_PROGRAMS)
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = foomatic-printermap foomatic-templates/*.xml

