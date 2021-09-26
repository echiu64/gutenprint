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

include $(top_srcdir)/scripts/global.mk


## Variables

empty=
PACKAGE = cups

BASE_VERSION = \"@GUTENPRINT_BASE_VERSION@\"

bindir = @cups_bindir@
sbindir = @cups_sbindir@

pkgdatadir = $(cups_conf_datadir)
pkglibdir = $(cups_conf_serverbin)
pkgsysconfdir = $(cups_conf_serverroot)

cupsexec_driverdir = $(pkglibdir)/driver
cupsexec_filterdir = $(pkglibdir)/filter

if BUILD_LIBUSB_BACKENDS
cupsexec_backenddir = $(pkglibdir)/backend
cupsdata_blacklistdir = $(pkgdatadir)/usb/
endif

if CUPS_PPDS_AT_TOP_LEVEL
cups_modeldir = $(pkgdatadir)/model/
DONT_UNINSTALL_PPDS=true
else
cups_modeldir = $(pkgdatadir)/model/gutenprint/@GUTENPRINT_RELEASE_VERSION@/
endif

LOCAL_CPPFLAGS = $(GUTENPRINT_CFLAGS) $(CUPS_CFLAGS) -DBASE_VERSION=$(BASE_VERSION) -DSBINDIR=\"$(sbindir)/\"

PPD_DIR=ppd

STP_NONLS_ENV= STP_MODULE_PATH=$(top_builddir)/src/main/.libs:$(top_builddir)/src/main STP_DATA_PATH=$(top_srcdir)/src/xml STP_LOCALEDIR=

STP_ENV= $(STP_NONLS_ENV) STP_LOCALEDIR=$(top_builddir)/src/cups/$(PPD_DIR)catalog

## Programs
if BUILD_TRANSLATED_CUPS_PPDS
if BUILD_GLOBALIZED_CUPS_PPDS
PPD = ppd-global
else
PPD = $(PPD_NLS_1)
endif
LINGUAS = $(ALL_LINGUAS)
TRANSLATE_PPDS = -DCUPS_TRANSLATED_PPDS
else
PPD = ppd-nonls
endif

if BUILD_SIMPLIFIED_CUPS_PPDS
BUILD_SIMPLE_PPDS=-DGENERATE_SIMPLIFIED_PPDS
endif

if BUILD_CUPS_1_2
cupsexec_driver_PROGRAMS = gutenprint.@GUTENPRINT_RELEASE_VERSION@
endif

if BUILD_CUPS
bin_PROGRAMS = cups-calibrate
sbin_PROGRAMS = cups-genppd.@GUTENPRINT_RELEASE_VERSION@
sbin_SCRIPTS = cups-genppdupdate
cupsexec_filter_PROGRAMS = rastertogutenprint.@GUTENPRINT_RELEASE_VERSION@ commandtoepson commandtocanon commandtodyesub

if BUILD_LIBUSB_BACKENDS
cupsexec_backend_PROGRAMS = backend_gutenprint
endif

## CUPS backends require no world-execute permissions if they are to be
## executed as root, and the backend must be run as root.
## See http://www.cups.org/documentation.php/doc-1.6/man-backend.html
if BUILD_LIBUSB_BACKENDS
install-exec-hook:
	chmod 700 $(DESTDIR)$(pkglibdir)/backend/backend_gutenprint
	mv $(DESTDIR)$(pkglibdir)/backend/backend_gutenprint "$(DESTDIR)$(pkglibdir)/backend/gutenprint$(GUTENPRINT_MAJOR_VERSION)$(GUTENPRINT_MINOR_VERSION)+usb"
endif

AM_TESTS_ENVIRONMENT=$(STP_ENV)
test-rastertogutenprint: min-pagesize
test-rastertogutenprint.test: test-rastertogutenprint
TESTS= test-ppds.test test-rastertogutenprint.test
test-rastertogutenprint.log: test-ppds.log

noinst_SCRIPTS=test-ppds.test \
	test-rastertogutenprint \
	test-rastertogutenprint.test \
	min-pagesize
endif

if BUILD_GENPPD_STATIC
STATIC_LDOPTS=-static -export-dynamic
endif

cups_calibrate_SOURCES = cups-calibrate.c
cups_calibrate_LDADD = -lm

commandtocanon_SOURCES = commandtocanon.c
commandtocanon_LDADD = $(CUPS_LIBS)

commandtoepson_SOURCES = commandtoepson.c
commandtoepson_LDADD = $(CUPS_LIBS)

if BUILD_LIBUSB_BACKENDS
backend_gutenprint_SOURCES = backend_canonselphy.c backend_canonselphyneo.c backend_kodak1400.c backend_kodak6800.c backend_kodak605.c backend_shinkos2145.c backend_sonyupd.c backend_sonyupdneo.c backend_dnpds40.c backend_mitsu70x.c backend_mitsu9550.c backend_sinfonia.c backend_sinfonia.h backend_common.c backend_common.h backend_shinkos1245.c backend_shinkos6145.c backend_shinkos6245.c backend_mitsup95d.c backend_magicard.c backend_mitsud90.c backend_hiti.c backend_mitsu.c backend_mitsu.h backend_kodak8800.c

backend_gutenprint_LDADD = $(LIBUSB_LIBS) $(LIBUSB_BACKEND_LIBDEPS)
backend_gutenprint_CPPFLAGS = $(AM_CPPFLAGS) $(LIBUSB_CFLAGS) -DLIBUSB_PRE_1_0_10
endif

cups_genppd_@GUTENPRINT_RELEASE_VERSION@_SOURCES = cups-genppd.c genppd.c genppd.h i18n.c i18n.h
cups_genppd_@GUTENPRINT_RELEASE_VERSION@_CFLAGS = -DALL_LINGUAS='"$(ALL_LINGUAS)"' $(BUILD_SIMPLE_PPDS) $(TRANSLATE_PPDS)
cups_genppd_@GUTENPRINT_RELEASE_VERSION@_LDADD = $(CUPS_LIBS) $(GENPPD_LIBS) $(GUTENPRINT_LIBS) @LIBICONV@
cups_genppd_@GUTENPRINT_RELEASE_VERSION@_LDFLAGS = $(STATIC_LDOPTS)

gutenprint_@GUTENPRINT_RELEASE_VERSION@_SOURCES = gutenprint.c genppd.c genppd.h i18n.c i18n.h
gutenprint_@GUTENPRINT_RELEASE_VERSION@_CFLAGS = -DALL_LINGUAS='"$(ALL_LINGUAS)"' $(BUILD_SIMPLE_PPDS) $(TRANSLATE_PPDS)
gutenprint_@GUTENPRINT_RELEASE_VERSION@_LDADD = $(CUPS_LIBS) $(GENPPD_LIBS) $(GUTENPRINT_LIBS) @LIBICONV@
gutenprint_@GUTENPRINT_RELEASE_VERSION@_LDFLAGS = $(STATIC_LDOPTS)

rastertogutenprint_@GUTENPRINT_RELEASE_VERSION@_SOURCES = rastertogutenprint.c i18n.c i18n.h
rastertogutenprint_@GUTENPRINT_RELEASE_VERSION@_LDADD = $(CUPS_LIBS) $(GUTENPRINT_LIBS) @LIBICONV@
rastertogutenprint_@GUTENPRINT_RELEASE_VERSION@_LDFLAGS = $(STATIC_LDOPTS)


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
INSTALL_DATA_LOCAL_DEPS = ppd-stamp
endif

if USE_NLS
PPD_NLS_1 = ppd-nls
endif

if BUILD_LIBUSB_BACKENDS
INSTALL_BLACKLIST=install-blacklist
install-blacklist:
	$(MKDIR_P) $(DESTDIR)$(cupsdata_blacklistdir)
	$(INSTALL_DATA) $(srcdir)/blacklist $(DESTDIR)$(cupsdata_blacklistdir)/net.sf.gimp-print.usb-quirks
endif

install-data-local: $(INSTALL_DATA_LOCAL_DEPS) $(INSTALL_BLACKLIST)
	if test -n "$(CUPS_PKG)" -a -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
	  $(MKDIR_P) $(DESTDIR)$(cups_modeldir); \
	  cd ppd ; \
	  for language in * ; do \
	    cd ..; \
	    $(MKDIR_P) $(DESTDIR)$(cups_modeldir)/$$language; \
	    cd ppd/$$language; \
	    for ppdfile in * ; do \
	      (cd ../..; $(INSTALL_DATA) ppd/$$language/$$ppdfile $(DESTDIR)$(cups_modeldir)/$$language) ; \
	    done; \
	  cd ..; \
	  done \
	fi
	$(MKDIR_P) "$(DESTDIR)$(localedir)"
	for file in $(srcdir)/../../po/*.po; do \
	  lang=`basename $$file .po`; \
	  $(MKDIR_P) "$(DESTDIR)$(localedir)/$$lang"; \
	  $(INSTALL_DATA) $$file "$(DESTDIR)$(localedir)/$$lang/gutenprint_$$lang.po"; \
	done

uninstall-local: $(INSTALL_DATA_LOCAL_DEPS) $(INSTALL_BLACKLIST)
# Remove PPDs installed in new PPD location
	if test -z "DONT_UNINSTALL_PPDS" ; then \
	  if test -n "$(INSTALL_DATA_LOCAL_DEPS)" ; then \
	    $(RM) -fr $(DESTDIR)$(cups_modeldir)/* ; \
	  fi ; \
	fi
	for file in $(srcdir)/../../po/*.po; do \
	  lang=`basename $$file .po`; \
	  $(RM) "$(DESTDIR)$(localedir)/$$lang/gutenprint_$$lang.po"; \
	done
	$(RM) -f "$(DESTDIR)$(cupsdata_blacklistdir)/net.sf.gimp-print.usb-quirks"
	$(RM) -f "$(DESTDIR)$(pkglibdir)/backend/gutenprint$(GUTENPRINT_MAJOR_VERSION)$(GUTENPRINT_MINOR_VERSION)+usb"

install-data-hook:
# Remove unused directories in install tree
	-@echo 'Expect a number of "rmdir: Directory not empty" warnings'
	-@echo 'These messages are harmless and should be ignored.'
	-rmdir $(DESTDIR)$(cups_modeldir)
	-rmdir $(DESTDIR)$(pkgdatadir)
	-rmdir $(DESTDIR)$(cupsexec_driverdir)
	-rmdir $(DESTDIR)$(cupsexec_filterdir)
	-rmdir $(DESTDIR)$(cupsexec_backenddir)
	-rmdir $(DESTDIR)$(cupsdata_blacklistdir)
	-rmdir $(DESTDIR)$(bindir)
	-rmdir $(DESTDIR)$(sbindir)
	-rmdir $(DESTDIR)$(pkglibdir)
	-rmdir $(DESTDIR)$(pkgsysconfdir)
	-rmdir `dirname $(DESTDIR)$(pkgdatadir)`
	-rmdir `dirname $(DESTDIR)$(pkglibdir)`
	-rmdir `dirname $(DESTDIR)$(pkgsysconfdir)`

.PHONY: ppd ppd-stamp-pre ppd-stamp-nonls ppd-stamp-nls ppd-stamp-phony \
	ppd-catalog-clean ppd-clean $(INSTALL_BLACKLIST)

all-local: $(INSTALL_DATA_LOCAL_DEPS)

ppd: ppd-stamp

ppd-stamp: cups-genppd.@GUTENPRINT_RELEASE_VERSION@ $(top_srcdir)/src/xml/xml-stamp ppd-stamp-pre
	$(MAKE) ppd-stamp-pre
	$(MAKE) ppd-stamp-phony
	touch ppd-stamp

ppd-stamp-phony: $(PPD)

ppd-catalog-clean:
	$(RM) -rf $(PPD_DIR)catalog

ppd-clean:
	$(RM) -rf $(PPD_DIR)

ppd-stamp-pre: ppd-catalog-clean ppd-clean

ppd-catalog: ppd-catalog-clean
	$(MKDIR_P) catalog
	for file in $(top_srcdir)/po/*.po; do \
	  lang=`basename $$file .po`; \
	  $(MKDIR_P) "$(PPD_DIR)catalog/$$lang"; \
	  $(INSTALL_DATA) $$file "$(PPD_DIR)catalog/$$lang/gutenprint_$$lang.po"; \
	done

ppd-nonls: cups-genppd.@GUTENPRINT_RELEASE_VERSION@
	@echo "Non-localized PPDs:"
	$(MKDIR_P) $(PPD_DIR)/C
	$(MAKE) ppd-catalog-clean
	LC_ALL= LANG= LANGUAGE= $(STP_NONLS_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ -l C -p $(PPD_DIR)/C @WHICH_PPDS@ $(EXTRA_GENPPD_OPTS)

ppd-nonls-a: cups-genppd.@GUTENPRINT_RELEASE_VERSION@
	@echo "Non-localized PPDs (all):"
	$(MKDIR_P) $(PPD_DIR)/C
	$(MAKE) ppd-catalog-clean
	LC_ALL= LANG= LANGUAGE= $(STP_NONLS_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ -l C -p $(PPD_DIR)/C -a $(EXTRA_GENPPD_OPTS)

ppd-nonls-s: cups-genppd.@GUTENPRINT_RELEASE_VERSION@
	@echo "Non-localized PPDs (simplified):"
	$(MKDIR_P) $(PPD_DIR)/C
	$(MAKE) ppd-catalog-clean
	LC_ALL= LANG= LANGUAGE= $(STP_NONLS_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ -l C -p $(PPD_DIR)/C -s $(EXTRA_GENPPD_OPTS)

ppd-global: cups-genppd.@GUTENPRINT_RELEASE_VERSION@
	@echo "Global PPDs:"
	$(MKDIR_P) $(PPD_DIR)/Global
	$(MAKE) ppd-catalog
	LC_ALL= LANG= LANGUAGE= $(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ -p $(PPD_DIR)/Global @WHICH_PPDS@ $(EXTRA_GENPPD_OPTS)
	$(MAKE) ppd-catalog-clean

ppd-global-a: cups-genppd.@GUTENPRINT_RELEASE_VERSION@
	@echo "Global PPDs (all):"
	$(MKDIR_P) $(PPD_DIR)/Global
	$(MAKE) ppd-catalog
	LC_ALL= LANG= LANGUAGE= $(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ -p $(PPD_DIR)/Global -a $(EXTRA_GENPPD_OPTS)
	$(MAKE) ppd-catalog-clean

ppd-global-s: cups-genppd.@GUTENPRINT_RELEASE_VERSION@
	@echo "Global PPDs (simplified):"
	$(MKDIR_P) $(PPD_DIR)/Global
	$(MAKE) ppd-catalog
	LC_ALL= LANG= LANGUAGE= $(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ -p $(PPD_DIR)/Global -s $(EXTRA_GENPPD_OPTS)
	$(MAKE) ppd-catalog-clean

ppd-global-ln: cups-genppd.@GUTENPRINT_RELEASE_VERSION@
	@echo "Global PPDs (localized numbers for testing):"
	$(MKDIR_P) $(PPD_DIR)/Global
	$(MAKE) ppd-catalog
	LC_ALL= LANG= LANGUAGE= $(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ -p $(PPD_DIR)/Global -N @WHICH_PPDS@ $(EXTRA_GENPPD_OPTS)
	$(MAKE) ppd-catalog-clean

ppd-global-ln-a: cups-genppd.@GUTENPRINT_RELEASE_VERSION@
	@echo "Global PPDs (all, localized numbers for testing):"
	$(MKDIR_P) $(PPD_DIR)/Global
	$(MAKE) ppd-catalog
	LC_ALL= LANG= LANGUAGE= $(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ -p $(PPD_DIR)/Global -N -a $(EXTRA_GENPPD_OPTS)
	$(MAKE) ppd-catalog-clean

ppd-global-ln-s: cups-genppd.@GUTENPRINT_RELEASE_VERSION@
	@echo "Global PPDs (all, localized numbers for testing):"
	$(MKDIR_P) $(PPD_DIR)/Global
	$(MAKE) ppd-catalog
	LC_ALL= LANG= LANGUAGE= $(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ -p $(PPD_DIR)/Global -N -s $(EXTRA_GENPPD_OPTS)
	$(MAKE) ppd-catalog-clean

ppd-nls: cups-genppd.@GUTENPRINT_RELEASE_VERSION@ ppd-nonls
	@echo "Localized PPD files:"
	$(MKDIR_P) ppd
	$(MAKE) ppd-catalog
	for language in `$(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ @WHICH_PPDS@ -L` ; do \
	  $(MKDIR_P) $(PPD_DIR)/$$language ; \
	  echo -n "$$language: " ; \
	  LC_ALL=$$language LANG=$$language LANGUAGE=$$language $(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ -l $$language -p $(PPD_DIR)/$$language $(EXTRA_GENPPD_OPTS); \
	done
	$(MAKE) ppd-catalog-clean

ppd-nls-a: cups-genppd.@GUTENPRINT_RELEASE_VERSION@ @WHICH_PPDS@ ppd-nonls
	@echo "Localized PPD files (all):"
	$(MKDIR_P) ppd
	$(MAKE) ppd-catalog
	for language in `$(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ @WHICH_PPDS@ -L` ; do \
	  $(MKDIR_P) $(PPD_DIR)/$$language ; \
	  echo -n "$$language: " ; \
	  LC_ALL=$$language LANG=$$language LANGUAGE=$$language $(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ @WHICH_PPDS@ -l $$language -p $(PPD_DIR)/$$language -a $(EXTRA_GENPPD_OPTS); \
	done
	$(MAKE) ppd-catalog-clean

ppd-nls-s: cups-genppd.@GUTENPRINT_RELEASE_VERSION@ @WHICH_PPDS@ ppd-nonls
	@echo "Localized PPD files (simplified):"
	$(MKDIR_P) ppd
	$(MAKE) ppd-catalog
	for language in `$(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ @WHICH_PPDS@ -L` ; do \
	  $(MKDIR_P) $(PPD_DIR)/$$language ; \
	  echo -n "$$language: " ; \
	  LC_ALL=$$language LANG=$$language LANGUAGE=$$language $(STP_ENV) ./cups-genppd.@GUTENPRINT_RELEASE_VERSION@ @WHICH_PPDS@ -l $$language -p $(PPD_DIR)/$$language -s $(EXTRA_GENPPD_OPTS); \
	done
	$(MAKE) ppd-catalog-clean

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

clean-local: ppd-catalog-clean ppd-clean


## Clean

CLEANFILES = ppd-stamp
DISTCLEANFILES = cups-genppdupdate test-ppds.test
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
	COPYING \
	blacklist \
	calibrate.ppm \
	command.txt \
	command.types
