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

include $(top_srcdir)/scripts/global.mk


## Variables

INCLUDES = $(GIMPPRINT_CFLAGS)
PACKAGE = cups

bindir = @cups_bindir@
sbindir = @cups_sbindir@

pkgdatadir = $(cups_conf_datadir)
pkglibdir = $(cups_conf_serverbin)
pkgsysconfdir = $(cups_conf_serverroot)

cupsexec_backenddir = $(pkglibdir)/backend
cupsexec_filterdir = $(pkglibdir)/filter
cups_modeldir = $(pkgdatadir)/model/gimp-print

DEFS = @CUPS_CFLAGS@ @DEFS@ -I. -I$(srcdir) -I../.. -DGENPPD_PPD_PREFIX=\"$(cups_modeldir)/gimp-print\" -DGIMP_PRINT_VERSION=\"$(VERSION)\"


## Programs

if BUILD_CUPS
bin_PROGRAMS = cups-calibrate
sbin_PROGRAMS = cups-genppd
sbin_SCRIPTS = cups-genppdconfig cups-genppdupdate
cupsexec_backend_PROGRAMS = epson canon
cupsexec_filter_PROGRAMS = rastertoprinter commandtoepson commandtocanon
endif


cups_calibrate_SOURCES = cups-calibrate.c
cups_calibrate_LDADD = $(LIBPRINTUT) -lm

commandtoepson_SOURCES = commandtoepson.c
commandtoepson_LDADD = $(LIBPRINTUT) $(CUPS_LIBS)

canon_SOURCES = canon.c
canon_LDADD = $(LIBPRINTUT) $(CUPS_LIBS)

epson_SOURCES = epson.c
epson_LDADD = $(LIBPRINTUT) $(CUPS_LIBS)

cups_genppd_SOURCES = genppd.c gimp-print-cups.c gimp-print-cups.h
cups_genppd_LDADD = $(LIBPRINTUT) $(CUPS_LIBS) $(GENPPD_LIBS) $(GIMPPRINT_LIBS)

rastertoprinter_SOURCES = rastertoprinter.c gimp-print-cups.c gimp-print-cups.h
rastertoprinter_LDADD = $(LIBPRINTUT) $(CUPS_LIBS) $(GIMPPRINT_LIBS)


## Data

if BUILD_CUPS
CUPS_PKG = calibrate.ppm
endif
if BUILD_CUPS
CUPS_CONF = command.types
endif

pkgdata_DATA = $(CUPS_PKG)
pkgsysconf_DATA = $(CUPS_CONF)

if BUILD_CUPS_PPDS
INSTALL_DATA_LOCAL_DEPS = ppd
endif

if BUILD_TRANSLATED_CUPS_PPDS
PPD_NLS = $(PPD_NLS_1)
LINGUAS = $(ALL_LINGUAS)
endif

if USE_NLS
PPD_NLS_1 = ppd-nls
endif


## Rules

install-data-local: $(INSTALL_DATA_LOCAL_DEPS)
	if test -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
	  $(mkinstalldirs) $(DESTDIR)$(cups_modeldir); \
	  cd ppd ; \
	  for language in * ; do \
	    cd ..; \
	    $(mkinstalldirs) $(DESTDIR)$(cups_modeldir)/$$language; \
	    cd ppd/$$language; \
	    for ppdfile in * ; do \
	      (cd ../..; $(INSTALL_DATA) ppd/$$language/$$ppdfile $(DESTDIR)$(cups_modeldir)/$$language) ; \
	    done; \
	  cd ..; \
	  done \
	fi

uninstall-local: $(INSTALL_DATA_LOCAL_DEPS)
#	if test -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
#	  for language in en $(LINGUAS) ; do \
#	    echo "Removing $$language locale PPD files..." ; \
#	    for ppdfile in ppd/$$language/* ; do \
#	      if test -f "$(DESTDIR)$(cups_modeldir)/$$language/`basename $$ppdfile`" ; then \
#	        $(RM) -f $(DESTDIR)$(cups_modeldir)/$$language/`basename $$ppdfile` ; \
#	      fi ; \
#	    done; \
#	  done \
#	fi
# Remove PPDs installed in new PPD location
	if test -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
	  $(RM) -fr $(DESTDIR)$(cups_modeldir)/* ; \
	fi

install-data-hook:
# Remove unused directories in install tree
	-@echo 'Expect a number of "rmdir: Directory not empty" warnings'
	-@echo 'These messages are harmless and should be ignored.'
	-rmdir $(DESTDIR)$(cups_modeldir)
	-rmdir $(DESTDIR)$(pkgdatadir)
	-rmdir $(DESTDIR)$(cupsexec_backenddir)
	-rmdir $(DESTDIR)$(cupsexec_filterdir)
	-rmdir $(DESTDIR)$(bindir)
	-rmdir $(DESTDIR)$(pkglibdir)
	-rmdir $(DESTDIR)$(pkgsysconfdir)
	-rmdir `dirname $(DESTDIR)$(pkgdatadir)`
	-rmdir `dirname $(DESTDIR)$(pkglibdir)`
	-rmdir `dirname $(DESTDIR)$(pkgsysconfdir)`

.PHONY: ppd ppd-stamp-pre ppd-stamp-nonls ppd-stamp-nls ppd-stamp-phony

all-local: $(INSTALL_DATA_LOCAL_DEPS)

ppd: ppd-stamp

ppd-stamp: cups-genppd
	$(MAKE) ppd-stamp-phony
	touch ppd-stamp

ppd-stamp-phony: ppd-stamp-pre ppd-nonls $(PPD_NLS)

ppd-stamp-pre:
	-$(RM) -r ppd
	-mkdir ppd

ppd-nonls: cups-genppd
	for language in en ; do \
	  mkdir ppd/$$language ; \
	  echo "$$language:" ; \
	 ./cups-genppd -p ppd/$$language ; \
	done

ppd-nls: cups-genppd
	wdir=`pwd` ; \
	cd $(top_builddir)/po ; \
	$(MAKE) ; \
	$(MAKE) DESTDIR= prefix=$${wdir}/catalog datadir=$${wdir}/catalog/share install
	for language in `./cups-genppd -L -c \`pwd\`/catalog/share/locale` ; do \
	  mkdir ppd/$$language ; \
	  echo "$$language:" ; \
	  LANGUAGE=$$language ./cups-genppd -p ppd/$$language -c `pwd`/catalog/share/locale ; \
	done
	$(RM) -r catalog

dist-hook:
	for dir in commands ; do \
	  if test -d $(srcdir)/$$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $(srcdir)/$$dir/*; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir; \
	      fi \
	    done \
	  fi \
	done

clean-local:
	-$(RM) -r ppd


## Clean

CLEANFILES = ppd-stamp
DISTCLEANFILES = cups-genppdconfig cups-genppdupdate
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
	COPYING \
	README \
	calibrate.ppm \
	command.txt \
	command.types

