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

if BUILD_TESTPATTERN

AM_TESTS_ENVIRONMENT=STP_MODULE_PATH=$(top_builddir)/src/main/.libs:$(top_builddir)/src/main STP_DATA_PATH=$(top_srcdir)/src/xml
run-testpattern-2.test: run-testpattern-2

TESTS = run-testpattern-1.test run-testpattern-2.test
run-testpattern-2.log: run-testpattern-1.log

bin_PROGRAMS = testpattern
noinst_PROGRAMS = printers printer_options
noinst_SCRIPTS = run-testpattern-2 compare-checksums compress-checksums \
	run-testpattern-2.test run-testpattern-1.test
CSUM_DEPS=testpattern run-testpattern-2 compress-checksums Checksums \
	run-testpattern-2.test

# This suffix is *not* static during development!
CSUM_SUFFIX=`@PKGROOT@/scripts/gversion pkg`
CSUM_RELEASE_SUFFIX=@GUTENPRINT_VERSION@
CSUM_FILE=Checksums/sums.gutenprint-$(SPREFIX)$(CSUM_SUFFIX)
CSUM_RELEASE_FILE=Checksums/sums.gutenprint-$(CSUM_RELEASE_SUFFIX)

.PHONY: checksums checksums-release

CHECKSUM_ENV=STP_TEST_PROFILE=checksums STP_DATA_PATH='@PKGROOT@/src/xml'

Checksums:
	$(MKDIR_P) Checksums

checksums: $(CSUM_DEPS)
	$(CHECKSUM_ENV) ./run-testpattern-2.test 4>&1 | ./compress-checksums | $(COMPRESS) > "$(CSUM_FILE)@CSUF@"

checksums-fast: $(CSUM_DEPS)
	$(CHECKSUM_ENV) ./run-testpattern-2.test -s 4>&1 | ./compress-checksums | $(COMPRESS) > "$(CSUM_FILE)@CSUF@"

checksums-release: $(CSUM_DEPS)
	if [ -n "$$STP_TEST_LOG_PREFIX" ] ; then \
	  $(CHECKSUM_ENV) ./run-testpattern-2.test 4>&1 | ./compress-checksums | $(COMPRESS) | tee "$${STP_TEST_LOG_PREFIX}checksums_$$$$@CSUF@" > "$(CSUM_RELEASE_FILE)@CSUF@" ; \
	else \
	  $(CHECKSUM_ENV) ./run-testpattern-2.test 4>&1 | ./compress-checksums | $(COMPRESS) > "$(CSUM_RELEASE_FILE)@CSUF@" ; \
	fi
endif

## Clean

MAINTAINERCLEANFILES = Makefile.in testpatternl.c testpatterny.c testpatterny.h

EXTRA_DIST = \
	testpatterny.h \
	$(pkgdata_DATA) \
	compare-checksums.in \
	compress-checksums.in
