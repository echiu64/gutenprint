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

SUBDIRS = developer

include $(top_srcdir)/scripts/global.mk


## Variables

docdir = $(datadir)/$(PACKAGE)/doc

## Rules

if MAINTAINER_MODE
if BUILD_DOC
all-local: gutenprint-stamp gutenprintui2-stamp
else
all-local:
endif
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

ps: $(MANUAL).ps
	cd developer; $(MAKE) ps

pdf: $(MANUAL).pdf
	cd developer; $(MAKE) pdf

html:
	cd developer; $(MAKE) html

install-data-local: FAQ.html gutenprint-users-manual.odt gutenprint-users-manual.pdf
	$(MKDIR_P) $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(srcdir)/FAQ.html $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(srcdir)/gutenprint-users-manual.odt $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(srcdir)/gutenprint-users-manual.pdf $(DESTDIR)$(docdir)


## Clean

distclean-local:
	$(RM) -rf gutenprint
	$(RM) -rf gutenprintui2

maintainer-clean-local: distclean-local

DISTCLEANFILES= \
	gutenprint-stamp \
	gutenprintui2-stamp

MAINTAINERCLEANFILES = \
	Makefile.in \
	$(DISTCLEANFILES)

EXTRA_DIST = \
	FAQ.html \
	README.maintaining \
	gutenprint-users-manual.odt \
	gutenprint-users-manual.pdf \
	gutenprint.dox.in \
	gutenprintui2.dox.in \
	gutenprint \
	gutenprintui2 \
	gutenprint-stamp \
	gutenprintui2-stamp

.PHONY: html ps pdf docs
