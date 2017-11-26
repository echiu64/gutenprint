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

SUBDIRS =					\
	inks					\
	inputslots				\
	model					\
	media					\
	mediasizes				\
	qualitypresets				\
	resolutions				\
	weaves

STAMPS =					\
	inks/xml-stamp				\
	inputslots/xml-stamp			\
	model/xml-stamp				\
	media/xml-stamp				\
	mediasizes/xml-stamp			\
	qualitypresets/xml-stamp		\
	resolutions/xml-stamp			\
	weaves/xml-stamp


xml-stamp: $(pkgxmldata_DATA) $(STAMPS) Makefile.am
	-rm -f $@ $@.tmp
	touch $@.tmp
	for dir in $(SUBDIRS) ; do \
	  sed "s;^;$$dir/;" $${dir}/xml-stamp >> $@.tmp ; \
	done
	for f in $(pkgxmldata_DATA) ; do echo $$f >> $@.tmp; done
	mv $@.tmp $@

all-local: xml-stamp

dist-hook: xml-stamp
## Clean

CLEANFILES = xml-stamp

MAINTAINERCLEANFILES = Makefile.in
