## Process this file with automake to produce Makefile.in
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

SUBDIRS = intl lib man src test po

if MAINTAINER_MODE
MAINT_MODE = true
endif

## Libtool dependency
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

ChangeLog:
	@cd $(top_srcdir) ; \
	if test -d CVS -a ! -f .noupdate ; then \
	  echo "Updating ChangeLog from CVS" ; \
	  cvs log | perl scripts/mkchlog > ChangeLog.tmp ; \
	  mv ChangeLog.tmp ChangeLog ; \
	else \
	  echo "This is not a CVS version of gimp-print, or .noupdate found!" ; \
	  echo "NOT updating ChangeLog from CVS!" ; \
	fi

dist-hook:
	for dir in doc scripts ; do \
	  if test -d $$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $$dir/*; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir; \
	      fi \
	    done \
	  fi \
	done

.PHONY: install-gimp install-cups install-ghost

install-gimp:
	cd $(top_srcdir)/src/gimp ; make install

install-cups:
	cd $(top_srcdir)/src/cups ; make install

install-ghost:
	cd $(top_srcdir)/src/ghost ; make install

CLEANSCRIPTS = \
	config.guess \
	config.sub \
	install-sh \
	ltconfig \
	ltmain.sh \
	missing \
	mkinstalldirs

MAINTAINERCLEANFILES = ABOUT-NLS INSTALL aclocal.m4 config.h.in configure Makefile.in stamp-h.in $(addprefix scripts/, $(CLEANSCRIPTS)) .noupdate

.PHONY: ChangeLog
