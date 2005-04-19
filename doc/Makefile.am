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

SUBDIRS = developer users_guide

include $(top_srcdir)/scripts/global.mk


## Variables

docdir = $(datadir)/doc/$(PACKAGE)

## Rules

if MAINTAINER_MODE
all-local: gutenprint-stamp gutenprintui2-stamp
else
all-local:
endif

# Rerun doxygen whenever autogen is run
gutenprintui2: gutenprintui2-stamp
gutenprintui2-stamp: gutenprintui2.dox $(top_srcdir)/configure
	$(RM) -rf gutenprintui2/html
	$(DOXYGEN) gutenprintui2.dox
	touch $@

gutenprint: gutenprint-stamp
gutenprint-stamp: gutenprint.dox $(top_srcdir)/configure
	$(RM) -rf gutenprint/html
	$(DOXYGEN) gutenprint.dox
	touch $@

docs: gutenprint-stamp
	cd developer; $(MAKE)
	cd users_guide; $(MAKE)

ps: $(MANUAL).ps
	cd developer; $(MAKE) ps
	cd users_guide; $(MAKE) ps

pdf: $(MANUAL).pdf
	cd developer; $(MAKE) pdf
	cd users_guide; $(MAKE) pdf

html: 
	cd developer; $(MAKE) html
	cd users_guide; $(MAKE) html


## Clean

maintainer-clean-local:
	$(RM) -r gutenprint

MAINTAINERCLEANFILES = \
	Makefile.in \
	gutenprint-stamp

EXTRA_DIST = \
	FAQ.html \
	README.maintaining \
	gutenprint.dox.in \
	gutenprintui2.dox.in \
	gutenprint \
	gutenprintui2 \
	gutenprint-stamp \
	gutenprintui2-stamp

.PHONY: html ps pdf docs
