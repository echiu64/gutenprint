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
LIBS = @LIBS@ ../lib/libprintut.la $(INTLLIBS) ../lib/libprintut.la

TESTS = testdither run-weavetest


## Programs
TEST_BIN = testdither escp2-weavetest unprint pcl-unprint bjc-unprint

noinst_PROGRAMS = @TEST_BIN@
EXTRA_PROGRAMS = escp2-weavetest unprint pcl-unprint bjc-unprint testdither

escp2_weavetest_SOURCES = escp2-weavetest.c

escp2_weavetest.o: escp2_weavetest.c
escp2_weavetest_LDADD = $(LIBGIMPPRINT_LIBS)
escp2_weavetest_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)

unprint_SOURCES = unprint.c

pcl_unprint_SOURCES = pcl-unprint.c

bjc_unprint_SOURCES = bjc-unprint.c

testdither_SOURCES = testdither.c
testdither_LDADD = $(LIBGIMPPRINT_LIBS)
testdither_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)

$(LIBGIMPPRINT_LIBS):
	cd ../src/main ; \
	$(MAKE)


## Rules

run-weavetest: escp2-weavetest


## Clean

CLEANFILES = $(EXTRA_PROGRAMS) mixed-color-1bit.ppm
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = cyan-sweep.tif parse-escp2 run-weavetest
