## $Id$
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
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

## Process this file with automake to produce Makefile.in.

@SET_MAKE@

include $(top_srcdir)/scripts/global.mk

## Variables

pkgxmldatadir = $(pkgdatadir)/@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@/xml/escp2/resolutions

pkgxmldata_DATA =				\
	resolutions_0.xml			\
	resolutions_1.xml			\
	resolutions_2.xml			\
	resolutions_3.xml			\
	resolutions_4.xml			\
	resolutions_5.xml			\
	resolutions_6.xml			\
	resolutions_7.xml			\
	resolutions_8.xml			\
	resolutions_9.xml			\
	resolutions_10.xml			\
	resolutions_11.xml			\
	resolutions_12.xml			\
	resolutions_13.xml			\
	resolutions_14.xml			\
	resolutions_15.xml			\
	resolutions_16.xml			\
	resolutions_17.xml			\
	resolutions_18.xml			\
	resolutions_19.xml			\
	resolutions_20.xml			\
	resolutions_21.xml			\
	resolutions_22.xml			\
	resolutions_23.xml			\
	resolutions_24.xml			\
	resolutions_25.xml			\
	resolutions_26.xml			\
	resolutions_27.xml			\
	resolutions_28.xml			\
	resolutions_29.xml			\
	resolutions_30.xml			\
	resolutions_31.xml			\
	resolutions_32.xml			\
	resolutions_33.xml			\
	resolutions_34.xml			\
	resolutions_35.xml			\
	resolutions_36.xml			\
	resolutions_37.xml			\
	resolutions_38.xml			\
	resolutions_39.xml			\
	resolutions_40.xml			\
	resolutions_41.xml			\
	resolutions_42.xml			\
	resolutions_43.xml			\
	resolutions_44.xml			\
	resolutions_45.xml			\
	resolutions_46.xml			\
	resolutions_47.xml			\
	resolutions_48.xml			\
	resolutions_49.xml			\
	resolutions_50.xml			\
	resolutions_51.xml			\
	resolutions_52.xml			\
	resolutions_53.xml			\
	resolutions_54.xml			\
	resolutions_55.xml			\
	resolutions_56.xml			\
	resolutions_57.xml			\
	resolutions_58.xml			\
	resolutions_59.xml			\
	resolutions_60.xml			\
	resolutions_61.xml			\
	resolutions_62.xml			\
	resolutions_63.xml			\
	resolutions_64.xml			\
	resolutions_65.xml			\
	resolutions_66.xml			\
	resolutions_67.xml			\
	resolutions_68.xml			\
	resolutions_69.xml			\
	resolutions_70.xml			\
	resolutions_71.xml			\
	resolutions_72.xml			\
	resolutions_73.xml			\
	resolutions_74.xml			\
	resolutions_75.xml			\
	resolutions_76.xml			\
	resolutions_77.xml			\
	resolutions_78.xml			\
	resolutions_79.xml			\
	resolutions_80.xml			\
	resolutions_81.xml			\
	resolutions_82.xml			\
	resolutions_83.xml			\
	resolutions_84.xml			\
	resolutions_85.xml			\
	resolutions_86.xml			\
	resolutions_87.xml			\
	resolutions_88.xml			\
	resolutions_89.xml			\
	resolutions_90.xml

xml-stamp: $(pkgxmldata_DATA) Makefile.am
	-rm -f xml-stamp
	touch xml-stamp
	for f in $(pkgxmldata_DATA) ; do echo $$f >> xml-stamp; done

all-local: xml-stamp

CLEANFILES = xmli18n-tmp.h xml-stamp

EXTRA_DIST = $(pkgxmldata_DATA)
