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

INCLUDES = @INCLUDES@ $(GIMP_CFLAGS) $(LIBGIMPPRINT_CFLAGS) -I$(top_srcdir)/src/main
LIBS = @LIBS@ ../lib/libprintut.la $(INTLLIBS) ../lib/libprintut.la

AM_CPPFLAGS =\
 -DLOCALEDIR=\""$(localedir)"\" \
 @MAINTAINER_CFLAGS@

noinst_PROGRAMS = @TEST_BIN@

EXTRA_PROGRAMS = escp2-weavetest printer_options unprint escp2-unprint pcl-unprint testdither

escp2_weavetest_SOURCES = escp2-weavetest.c

escp2_weavetest.o: escp2_weavetest.c 
escp2_weavetest_LDADD = $(LIBGIMPPRINT_LIBS)

printer_options_SOURCES = printer_options.c
printer_options_LDADD = -lm $(LIBGIMPPRINT_LIBS)

unprint_SOURCES = unprint.c

escp2_unprint_SOURCES = escp2-unprint.c

pcl_unprint_SOURCES = pcl-unprint.c

testdither_SOURCES = testdither.c
testdither_LDADD = -lm $(LIBGIMPPRINT_LIBS)

$(LIBGIMPPRINT_LIBS):
	$(MAKE) -C ../src/main

EXTRA_DIST = cyan-sweep.tif parse-escp2 run-weavetest
CLEANFILES = $(EXTRA_PROGRAMS)
MAINTAINERCLEANFILES = Makefile.in
