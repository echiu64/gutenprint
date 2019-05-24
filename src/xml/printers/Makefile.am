## Copyright (C) 2017 Robert Krawitz
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

pkgxmldatadir = $(pkgdatadir)/@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@/xml/printers

pkgxmldata_DATA =				\
	canon.xml				\
	dpl.xml					\
	dyesub.xml				\
	escp2.xml				\
	lexmark.xml				\
	pcl.xml					\
	ps.xml					\
	raw.xml

AM_TESTS_ENVIRONMENT=STP_MODULE_PATH=$(top_builddir)/src/main/.libs:$(top_builddir)/src/main STP_DATA_PATH=$(top_srcdir)/src/xml
TESTS = check_duplicate_printers.test

noinst_PROGRAMS = check_duplicate_printers.test
check_duplicate_printers_test_SOURCES = check_duplicate_printers.test.c
check_duplicate_printers_test_LDADD = $(GUTENPRINT_LIBS)

xml-stamp: $(pkgxmldata_DATA) Makefile.am check_duplicate_printers.test
	$(AM_TESTS_ENVIRONMENT) ./check_duplicate_printers.test
	-rm -f $@ $@.tmp
	touch $@.tmp
	for f in $(pkgxmldata_DATA) ; do echo $$f >> $@.tmp; done
	mv $@.tmp $@

all-local: xml-stamp

dist-hook: xml-stamp
CLEANFILES = xmli18n-tmp.h xml-stamp xml-stamp.tmp

EXTRA_DIST = $(pkgxmldata_DATA)
MAINTAINERCLEANFILES = Makefile.in
