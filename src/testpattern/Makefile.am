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
INCLUDES = @INCLUDES@ $(LIBGIMPPRINT_CFLAGS)
LIBS = @LIBS@ $(INTLLIBS)

## Programs

TESTPATTERN_BIN = testpattern

bin_PROGRAMS = @TESTPATTERN_BIN@

EXTRA_PROGRAMS = testpattern

LFLAGS = -i
YFLAGS = -d

testpattern_SOURCES = testpattern.c testpatterny.y testpatternl.l testpattern.h
testpattern_LDADD = $(LIBGIMPPRINT_LIBS)
testpattern_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)

testpatternl.o: testpatterny.o
testpattern.o: testpatterny.o

$(LIBGIMPPRINT_LIBS):
	cd ../main ; \
	$(MAKE)

SAMPLE_IMAGES = testpattern.sample

pkgdatadir = $(datadir)/$(PACKAGE)/samples

pkgdata_DATA = @SAMPLE_IMAGES@

## Clean

MAINTAINERCLEANFILES = Makefile.in

DISTCLEANFILES = testpatternl.c testpatterny.c testpatterny.h
EXTRA_DIST =  testpatterny.h testpattern.sample
