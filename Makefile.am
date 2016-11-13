## Process this file with automake to produce Makefile.in
## Copyright (C) 2000-2004 Roger Leigh
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

ACLOCAL_AMFLAGS = -I m4 -I m4local

include $(top_srcdir)/scripts/global.mk


## Variables

SUBDIRS = include src samples test po man doc scripts
DISTDIRS = m4 m4extra

docdir = $(datadir)/$(PACKAGE)/doc

## Rules

# Libtool dependency
libtool: $(LIBTOOL_DEPS)
	cd $(srcdir); \
	$(SHELL) ./config.status --recheck

# Don't list ChangeLog as a dependency here because it forces network
# access, and because it usually runs as superuser rather than the user
# who normally builds Gutenprint.
install-data-local: NEWS AUTHORS COPYING README
	$(mkdir_p) $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(srcdir)/NEWS $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(srcdir)/ChangeLog $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(srcdir)/AUTHORS $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(srcdir)/COPYING $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(srcdir)/README $(DESTDIR)$(docdir)

# git log is local, so it's fast enough that we don't need to
# engage in any subterfuge like we did with CVS.  It's really fast
# when we use a static file from the pre-Git era (since there are no
# live branches from that time) to save a lot of diffs.

ChangeLog:
	if [ -d .git -a -f ChangeLog.pre-5.2.11 ] ; then \
	  $(MAKE) -C scripts mkgitlog; \
	  scripts/mkgitlog -i ChangeLog.pre-5.2.11 -r gutenprint-5_2_11 > ChangeLog; \
	fi

configure.in:
	$(LN_S) $(srcdir)/configure.ac $(srcdir)/configure.in

dist-time-check:
	@current_date=`date +%Y%m%d`; \
	configure_date=`ls -l --time-style=+%Y%m%d $(top_builddir)/configure | awk '{print $$6}'`; \
	echo "Checking that 'configure' was built today."; \
	if test "$$current_date" != "$$configure_date" ; then \
	  echo "configure is out of date -- rerun ./autogen.sh first!"; \
	  exit 1; \
	fi

dist-hook: dist-time-check
	for dir in $(DISTDIRS) ; do \
	  if test -d $(srcdir)/$$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $(srcdir)/$$dir/*; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir; \
	      fi \
	    done \
	  fi \
	done
# Remove configure.in (libtool bug workaround)
	$(RM) -f $(distdir)/configure.in
# Remove any backup files found in distdir
	$(FIND) $(distdir) -name '.*' -print | xargs $(RM) -rf
	$(FIND) $(distdir) -name '#*' -print | xargs $(RM) -rf
	$(FIND) $(distdir) -name '*~' -print | xargs $(RM) -rf

install-cups: all
	cd src/cups ; \
	$(MAKE) install

install-gimp: all
	cd src/gimp ; \
	$(MAKE) install

docs:
	cd doc ; \
	$(MAKE) docs

html:
	cd doc ; \
	$(MAKE) html

ps:
	cd doc ; \
	$(MAKE) ps

pdf:
	cd doc ; \
	$(MAKE) pdf

snapshot:
	$(MAKE) dist distdir=$(PACKAGE)`date +"%Y%m%d"`

## Clean

CLEANSCRIPTS = \
	scripts/depcomp \
	scripts/install-sh \
	scripts/ltconfig \
	scripts/mdate-sh \
	scripts/missing \
	scripts/mkinstalldirs

CLEANMFOUR = \
	m4/codeset.m4 \
	m4/gettext.m4 \
	m4/glibc21.m4 \
	m4/iconv.m4 \
	m4/intdiv0.m4 \
	m4/inttypes_h.m4 \
	m4/inttypes.m4 \
	m4/inttypes-pri.m4 \
	m4/isc-posix.m4 \
	m4/lcmessage.m4 \
	m4/lib-ld.m4 \
	m4/lib-link.m4 \
	m4/lib-prefix.m4 \
	m4/progtest.m4 \
	m4/stdint_h.m4 \
	m4/uintmax_t.m4 \
	m4/ulonglong.m4

CLEANPO = \
	po/boldquot.sed \
	po/en@boldquot.header \
	po/en@quot.header \
	po/insert-header.sin \
	po/Makevars.template \
	po/quot.sed \
	po/remove-potcdate.sin \
	po/Rules-quot

CLEANFILES = .noupdate

MAINTAINERCLEANFILES = \
	ABOUT-NLS \
	ChangeLog \
	INSTALL \
	aclocal.m4 \
	config.h.in \
	configure \
	configure.in \
	Makefile.in \
	stamp-h.in \
	stamp-h1.in \
	stamp-h2.in \
	stamp-h3.in \
	stamp-h4.in \
	$(CLEANSCRIPTS) \
	$(CLEANMFOUR) \
	$(CLEANPO)

EXTRA_DIST = autogen.sh README.package

.PHONY: deb html install-cups install-gimp snapshot ChangeLog Phony dist-time-check

