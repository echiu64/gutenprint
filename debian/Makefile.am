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

AUTOMAKE_OPTIONS = 1.5 dist-bzip2 check-news gnu no-dependencies 

@SET_MAKE@

MAINT_CHARSET = latin1

include $(top_srcdir)/scripts/global.mk


## Variables

EXTRA_DIST = \
	changelog \
	compat \
	control.in \
	copyright \
	cupsys-driver-gutenprint.docs \
	cupsys-driver-gutenprint.install \
	cupsys-driver-gutenprint.postinst \
	cupsys-driver-gutenprint.postrm \
	escputil.docs \
	escputil.install \
	foomatic-db-gutenprint.install \
	gimp-print.docs \
	gimp-print.install \
	gimp-print.postrm \
	gimp-print.preinst \
	gutenprint-doc.doc-base \
	gutenprint-doc.install \
	gutenprint-locales.install \
	ijsgutenprint.install \
	libgutenprint-dev.docs \
	libgutenprint-dev.install \
	libgutenprint-doc.doc-base.developer.in \
	libgutenprint-doc.doc-base.reference.in \
	libgutenprint-doc.install.in \
	libgutenprint.install.in \
	libgutenprintui2-dev.install \
	libgutenprintui2.install.in \
	README.building \
	README.Debian \
	rules


## Rules

dist-hook:
# Distribute debian/control (not in EXTRA_DIST to avoid $(srcdir) dependencies
	DEBIAN_VERSION=`grep "(" $(srcdir)/changelog | head -1l | sed "s/^[^(]*(//" | sed "s/).*$$//"` ; \
	UPSTREAM_VERSION=`echo $$DEBIAN_VERSION | sed "s/-[^-]*$$//"` ; \
	LIBRARY_VERSION="-$${UPSTREAM_VERSION}" ; \
	sed -e "s/#UPSTREAM_VERSION#/$${UPSTREAM_VERSION}/g" \
	    -e "s/#LIBRARY_VERSION#/$${LIBRARY_VERSION}/g" \
	    <$(srcdir)/control.in >$(distdir)/control

DEBUILD_OPTIONS = -us -uc
deb: debian/changelog debian/control debian/rules
	$(MAKE) dist
	-$(RM) -r debian-build
	-mkdir debian-build
	cd debian-build ; \
	mv ../$(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)_$(VERSION).orig.tar.gz ; \
	tar xfvz $(PACKAGE)_$(VERSION).orig.tar.gz ; \
	cd $(PACKAGE)-$(VERSION) ; \
	debuild $(DEBUILD_OPTIONS) ; \
	cd .. ; \
	$(RM) -r $(PACKAGE)-$(VERSION)

control:
	cd ..; \
	debian/rules debian/control


## Clean

MAINTAINERCLEANFILES = \
	Makefile.in


.PHONY: deb
