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
## along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
	$(MKDIR_P) $(DESTDIR)$(docdir)
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

dist-time-check:
	@if [ "$(top_srcdir)/configure" -ot "$(top_srcdir)/configure.ac" ] ; then \
	  echo "*** configure is out of date (older than configure.ac) -- rerun ./autogen.sh first!"; exit 1; \
	fi

git-version-stamp:
	if [ -d .git ] ; then scripts/gversion > /dev/null; fi

dist-hook: dist-time-check git-version-stamp
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
# Remove any backup files found in distdir
	$(FIND) $(distdir) -name '.*' -print | xargs $(RM) -rf
	$(FIND) $(distdir) -name '#*' -print | xargs $(RM) -rf
	$(FIND) $(distdir) -name '*~' -print | xargs $(RM) -rf

snapshot: scripts/gversion git-version-stamp
	$(MAKE) dist distdir=$(PACKAGE)-`scripts/gversion`-`date +"%Y%m%d"`

#################################################################
#			       TESTING				#
#################################################################

CPUS=`scripts/count-cpus`

# STP_PROFILE=full
MAKECHECK=STP_TEST_SUITE=1 /usr/bin/time make -k -s -j$(CPUS) check
MAKEDISTCHECK=STP_TEST_SUITE=1 STP_TEST_DIST=1 /usr/bin/time make -k -s -j$(CPUS) distcheck
MAKECHECK_PARALLEL=STP_PARALLEL=$(CPUS) $(MAKECHECK)
MAKEDISTCHECK_PARALLEL=STP_PARALLEL=$(CPUS) $(MAKEDISTCHECK)

# ...but we can't do
# AM_TESTS_ENVIRONMENT=STP_PARALLEL=$(shell scripts/count-cpus)
# because $(shell...) is (still only) a GNU extension, which we
# want to avoid.  And we can't do
# AM_TESTS_ENVIRONMENT=STP_PARALLEL=`scripts/count-cpus`
# because all expansions are lazy, so anything not in the root directory
# won't find the script.  And we can't even do
# AM_TESTS_ENVIRONMENT=STP_PARALLEL=`(at)TOP_BUILDDIR(at)scripts/count-cpus`
# because TOP_BUILDDIR itself is a relative path.  And, just to make it
# even more fun, we won't be able to do
# AM_TESTS_ENVIRONMENT!=
# syntax even if POSIX approves it because it's only present in GNU make 4,
# but Apple doesn't use that.

check-parallel: scripts/count-cpus
	$(MAKECHECK_PARALLEL)

distcheck-parallel: scripts/count-cpus
	$(MAKEDISTCHECK_PARALLEL)

check-fast: scripts/count-cpus
	STP_TEST_PROFILE=fast $(MAKECHECK_PARALLEL)

distcheck-fast: scripts/count-cpus
	STP_TEST_PROFILE=fast $(MAKEDISTCHECK_PARALLEL)

check-minimal: scripts/count-cpus
	STP_TEST_PROFILE=minimal $(MAKECHECK_PARALLEL)

distcheck-minimal: scripts/count-cpus
	STP_TEST_PROFILE=minimal $(MAKEDISTCHECK_PARALLEL)

check-full: scripts/count-cpus
	STP_TEST_PROFILE=full $(MAKECHECK_PARALLEL)

distcheck-full: scripts/count-cpus
	STP_TEST_PROFILE=full $(MAKEDISTCHECK_PARALLEL)

# distcheck-valgrind won't work because distcheck does not use
# --disable-shared.  valgrind doesn't work with that type of build because
# the in-tree executables aren't actually binaries; they're libtool-based
# shell scripts that invoke the real binaries.
check-valgrind: scripts/count-cpus
	STP_TEST_PROFILE=valgrind $(MAKECHECK_PARALLEL)

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
	Makefile.in \
	stamp-h.in \
	stamp-h1.in \
	stamp-h2.in \
	stamp-h3.in \
	stamp-h4.in \
	$(CLEANSCRIPTS) \
	$(CLEANMFOUR) \
	$(CLEANPO)

EXTRA_DIST = autogen.sh README.package git-version-stamp

.PHONY: deb html install-cups install-gimp snapshot ChangeLog Phony dist-time-check git-version-stamp
