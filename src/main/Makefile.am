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

INCLUDES = @INCLUDES@ $(GIMP_CFLAGS)

AM_CPPFLAGS =\
 -DLOCALEDIR=\""$(localedir)"\"\
 @MAINTAINER_CFLAGS@

bin_SCRIPTS = gimpprint-config

lib_LTLIBRARIES = @MAIN_BIN@
EXTRA_LTLIBRARIES = libgimpprint.la

libgimpprint_la_SOURCES = \
	gimp-print-intl-internal.h \
	gimp-print-internal.h \
	print-lexmark.c \
	print-canon.c \
	print-dither.c \
	print-escp2.c \
	print-pcl.c \
	print-ps.c \
	print-util.c \
	print-color.c \
	print-weave.c \
	print-version.c \
	quickmatrix257.h \
	ran.367.179.h \
	ran.509.131.h
libgimpprint_la_LIBADD = ../../lib/libprintut.la -lm
## Uncommment to build an unversioned library (version in soname)
libgimpprint_la_LDFLAGS = -release $(VERSION) -rpath $(libdir)
## Uncomment to build a versioned library with MAJOR and MINOR in soname
##libgimpprint_la_LDFLAGS = @STRIP_BEGIN@ \
##	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)    \
##	-release $(LT_RELEASE)                                  \
##	-export-dynamic                                         \
##	-rpath $(libdir)                                        \
##	@STRIP_END@
## Uncomment to build a versioned library with static soname
##libgimpprint_la_LDFLAGS = @STRIP_BEGIN@ \
##	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)    \
##	-export-dynamic                                         \
##	-rpath $(libdir)                                        \
##	@STRIP_END@

m4datadir = $(datadir)/aclocal
m4data_DATA = gimpprint.m4

$(srcdir)/print-util.c: print-printers.c

print-printers.c: ../printdef/printdef $(srcdir)/printers.xml
	../printdef/printdef < $(srcdir)/printers.xml > print-printers.c

printdefl.o: printdefy.o

../printdef/printdef: $(addprefix ../printdef/, printdefl.l printdefy.y printdef.h)
	$(MAKE) -C ../printdef

CLEANFILES = print-printers.c
DISTCLEANFILES = gimpprint-config
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = gimpprint.m4 gimpprint-config.in printers.xml
