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

@SET_MAKE@

include $(top_srcdir)/scripts/global.mk


## Variables

m4datadir = $(datadir)/aclocal
pkgconfigdatadir = $(libdir)/pkgconfig


## Programs

if BUILD_LIBGUTENPRINTUI
lib_LTLIBRARIES = libgutenprintui.la
endif

libgutenprintui_la_SOURCES =			\
	printrcy.y				\
	printrcl.l				\
	printrc.h				\
	plist.c					\
	panel.c					\
	ui-utils.c				\
	print-image-thumbnail.c			\
	gutenprintui-internal.h

libgutenprintui_la_LIBADD = $(GUTENPRINTUI_LIBDEPS)
# Uncommment to build an unversioned library (version in soname)
libgutenprintui_la_CFLAGS = $(AM_CFLAGS) $(GUTENPRINT_CFLAGS) $(GTK_CFLAGS)
#libgutenprintui_la_LDFLAGS = -release $(GUTENPRINT_VERSION) \
#	-rpath $(libdir) -no-undefined
# Uncomment to build a versioned library
libgutenprintui_la_LDFLAGS = \
 -version-info  $(GUTENPRINTUI_CURRENT_INTERFACE):$(GUTENPRINTUI_INTERFACE_AGE):$(GUTENPRINTUI_BINARY_AGE) \
 -rpath $(libdir) -no-undefined

AM_LFLAGS = -i
AM_YFLAGS = -d

printrcy.o: printrcl.o
plist.o: printrcy.o

## Data

if BUILD_LIBGUTENPRINTUI
pkgconfigdata_DATA = gutenprintui.pc
endif


## Rules


## Clean

MAINTAINERCLEANFILES = Makefile.in printrcl.c printrcy.c printrcy.h
