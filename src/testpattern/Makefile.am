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
## along with this program.  If not, see <https://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in.

@SET_MAKE@

include $(top_srcdir)/scripts/global.mk


## Variables

pkgdatadir = $(datadir)/$(PACKAGE)/samples

## Programs

if BUILD_TESTPATTERN

TESTS = run-testpattern-1 run-testpattern-2

bin_PROGRAMS = testpattern
noinst_PROGRAMS = printers printer_options
noinst_SCRIPTS = run-testpattern-2 compare-checksums compress-checksums
endif

AM_LFLAGS = -i
AM_YFLAGS = -d

testpattern_SOURCES = testpattern.c testpatterny.y testpatternl.l testpattern.h
testpattern_LDADD = $(GUTENPRINT_LIBS) $(LIBM)

testpatternl.o: testpatterny.o
testpattern.o: testpatterny.o

printers_LDADD = $(GUTENPRINT_LIBS)
printer_options_LDADD = $(GUTENPRINT_LIBS)


## Data

if INSTALL_SAMPLES
pkgdata_DATA = testpattern.sample extended.sample
endif

## Generate checksums

if BUILD_TESTPATTERN

# This suffix is *not* static during development!
CSUM_SUFFIX=`@PKGROOT@/scripts/gversion`
CSUM_RELEASE_SUFFIX=@GUTENPRINT_VERSION@
CSUM_FILE=Checksums/sums.$(SPREFIX)$(CSUM_SUFFIX)
CSUM_RELEASE_FILE=Checksums/sums.$(CSUM_RELEASE_SUFFIX)

.PHONY: checksums checksums-release

Checksums:
	$(MKDIR_P) Checksums

checksums: testpattern run-testpattern-2 ./compress-checksums Checksums
	STP_DATA_PATH='@PKGROOT@/src/xml' ./run-testpattern-2 -q -M - | ./compress-checksums | $(COMPRESS) > "$(CSUM_FILE)@CSUF@"

checksums-fast: testpattern run-testpattern-2 ./compress-checksums Checksums
	STP_DATA_PATH='@PKGROOT@/src/xml' ./run-testpattern-2 -s -q -M - | ./compress-checksums | $(COMPRESS) > "$(CSUM_FILE)@CSUF@"

checksums-release: testpattern run-testpattern-2 ./compress-checksums Checksums
	STP_DATA_PATH='@PKGROOT@/src/xml' ./run-testpattern-2 -q -M - | ./compress-checksums | $(COMPRESS) > "$(CSUM_RELEASE_FILE)@CSUF@"
endif

## Clean

MAINTAINERCLEANFILES = Makefile.in testpatternl.c testpatterny.c testpatterny.h

EXTRA_DIST = \
	testpatterny.h \
	$(pkgdata_DATA) \
	run-testpattern \
	run-testpattern-1 \
	compare-checksums.in \
	compress-checksums.in
