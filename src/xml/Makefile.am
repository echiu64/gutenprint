## Copyright (C) 2008 Robert Krawitz
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

SUBDIRS = 					\
	dither					\
	escp2					\
	papers					\
	printers

STAMPS = 					\
	dither/xml-stamp			\
	escp2/xml-stamp				\
	papers/xml-stamp			\
	printers/xml-stamp


## Variables

pkgxmldatadir = $(pkgdatadir)/@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@/xml

## Rules

noinst_PROGRAMS = extract-strings

extract_strings_SOURCES = extract-strings.c
extract_strings_LDADD = $(GUTENPRINT_LIBS)

xml-stamp: $(pkgxmldata_DATA) $(STAMPS) Makefile.am
	-rm -f $@ $@.tmp
	touch $@.tmp
	for dir in $(SUBDIRS) ; do \
	  sed "s;^;$$dir/;" $${dir}/xml-stamp >> $@.tmp ; \
	done
	for f in $(pkgxmldata_DATA) ; do echo $$f >> $@.tmp; done
	mv $@.tmp $@

all-local: xmli18n-tmp.h xml-stamp


xmli18n-tmp.h: xml-stamp extract-strings
	-rm -f $@ $@.tmp
	./extract-strings `cat xml-stamp | sed -e 's;^;$(srcdir)/;'` > $@.tmp
	mv $@.tmp $@


dist-hook: xmli18n-tmp.h xml-stamp
# xmli18n-tmp.h is needed by po/POTFILES.in at dist time

## Clean

CLEANFILES = xmli18n-tmp.h xmli18n-tmp.h.tmp xml-stamp xml-stamp.tmp

EXTRA_DIST = $(pkgxmldata_DATA) xmli18n-tmp.h

MAINTAINERCLEANFILES = Makefile.in
