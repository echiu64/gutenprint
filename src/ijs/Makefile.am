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
INCLUDES = @IJS_CFLAGS@ @INCLUDES@ $(LIBGIMPPRINT_CFLAGS)
LIBS = @IJS_LIBS@ @LIBS@ $(INTLLIBS)

## Programs

IJS_BIN = ijsgimpprint

bin_PROGRAMS = @IJS_BIN@

EXTRA_PROGRAMS = ijsgimpprint

ijsgimpprint_LDADD = $(LIBGIMPPRINT_LIBS)
ijsgimpprint_DEPENDENCIES = $(LIBGIMPPRINT_LIBS)

$(LIBGIMPPRINT_LIBS):
	cd ../main ; \
	$(MAKE)

## Clean

MAINTAINERCLEANFILES = Makefile.in
