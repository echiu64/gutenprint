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

## Variables

pkgxmldatadir = $(pkgdatadir)/@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@/xml/escp2/model/base

pkgxmldata_DATA =				\
	baseline_300.xml			\
	baseline_360.xml			\
	bx.xml					\
	c1xx.xml				\
	c2x.xml					\
	c4x.xml					\
	c7xx.xml				\
	c8x_base.xml				\
	c8x.xml					\
	cpro.xml				\
	photo2.xml				\
	picmate.xml				\
	pro_ultrachrome_k3.xml			\
	pro_ultrachrome_k3_base.xml		\
	pro_ultrachrome_k3_cutter.xml		\
	pro_ultrachrome_k3v.xml			\
	pro_ultrachrome_k3v10.xml		\
	pro_ultrachrome_k3v10_base.xml		\
	pro_ultrachrome_k3v10a.xml		\
	pro_ultrachrome_k3v10b.xml		\
	pro_x000.xml				\
	pro_x500.xml				\
	pro_x600.xml				\
	pro_x700.xml				\
	pro_x800.xml				\
	pro_x880.xml				\
	r200.xml				\
	r240.xml				\
	r800.xml				\
	sc800.xml				\
	sp700.xml				\
	sp750.xml				\
	sp950.xml				\
	wf7xxx.xml

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
CLEANFILES = xmli18n-tmp.h xml-stamp xml-stamp.tmp

EXTRA_DIST = $(pkgxmldata_DATA)
MAINTAINERCLEANFILES = Makefile.in
