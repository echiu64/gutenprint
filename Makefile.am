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

SUBDIRS = doc intl lib man src test po

if MAINTAINER_MODE
MAINT_MODE = true
endif

## Libtool dependency
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

ChangeLog:
	@cd $(top_srcdir) ; \
	echo `pwd`
	if test -f .noupdate ; then \
	  echo "ChangeLog: .noupdate found: NOT updating ChangeLog from CVS" ; \
	elif test -d CVS ; then \
	  echo "ChangeLog: Updating ChangeLog from CVS" ; \
	  cvs log -N | perl scripts/mkchlog > ChangeLog.tmp ; \
	  if test -s ChangeLog.tmp ; then \
	    mv ChangeLog.tmp ChangeLog ; \
	  else \
	    echo "ChangeLog: Failed to update ChangeLog ['touch .noupdate' to skip]" ; \
	    rm ChangeLog.tmp ; \
	    exit 1 ; \
	  fi \
	elif test -s ChangeLog ; then \
	  echo "ChangeLog: This is not a CVS version of GIMP-Print" ; \
	  echo "ChangeLog: Not updating ChangeLog from CVS!" ; \
	else \
	  echo "ChangeLog: No ChangeLog or ChangeLog of zero size found!" ; \
	  echo "ChangeLog: Failed to update ChangeLog ['touch .noupdate' to skip]" ; \
	  exit 1 ; \
	fi

dist-hook:
	for dir in debian scripts ; do \
	  if test -d $(srcdir)/$$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $(srcdir)/$$dir/*; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir; \
	      fi \
	    done \
	  fi \
	done
# Create debian/control
	DEBIAN_VERSION=`grep "(" debian/changelog | head -1l | sed "s/^[^(]*(//" | sed "s/).*$$//"` ; \
	UPSTREAM_VERSION=`echo $$DEBIAN_VERSION | sed "s/-[^-]*//"` ; \
	sed -e "s/#UPSTREAM_VERSION#/$${UPSTREAM_VERSION}/g" <debian/control.in >$(distdir)/debian/control
# Do some extra magic to make a distribution for GIMP CVS
	if test -f .gimp-dist ; then \
	  rm -rf $(distdir)/src/cups $(distdir)/src/escputil $(distdir)/src/foomatic $(distdir)/src/ghost ; \
	  rm -rf $(distdir)/debian $(distdir)/scripts $(distdir)/test ; \
	  rm -rf $(distdir)/ABOUT-NLS $(distdir)/INSTALL $(distdir)/aclocal.m4 $(distdir)/config.h.in $(distdir)/configure $(distdir)/stamp-h.in ; \
	  find $(distdir) -name Makefile.in -print | xargs --no-run-if-empty rm ; \
	  rm -f .gimp-dist ; \
	fi

gimp-dist:
	touch .gimp-dist
	$(MAKE) dist distdir=$(PACKAGE)-$(VERSION)-GIMP-CVS-DIST

install-gimp: all
	$(MAKE) -C src/gimp install

install-cups: all
	$(MAKE) -C src/cups install

install-ghost: all
	$(MAKE) -C src/ghost install

html:
	$(MAKE) -C doc html

snapshot:
	$(MAKE) dist distdir=$(PACKAGE)`date +"%Y%m%d"`

CLEANSCRIPTS = \
	config.guess \
	config.sub \
	install-sh \
	ltconfig \
	ltmain.sh \
	missing \
	mkinstalldirs

CLEANFILES = .gimp-dist .noupdate
MAINTAINERCLEANFILES = ABOUT-NLS INSTALL aclocal.m4 config.h.in configure Makefile.in stamp-h.in $(addprefix scripts/, $(CLEANSCRIPTS))

.PHONY: gimp-dist install-gimp install-cups install-ghost html snapshot ChangeLog
