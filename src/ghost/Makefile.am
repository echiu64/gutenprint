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

## Use old compatability method for now...

## Variables

MAINCODE = canon.c				\
	color.c					\
	dither.c				\
	escp2.c					\
	lexmark.c				\
	pcl.c					\
	printers.c				\
	ps.c					\
	util.c					\
	weave.c					\
	dither-matrices.c			\
	escp2-data.c

MAINHEADERS = print.h				\
	print-internal.h			\
	print-intl-internal.h			\
	quickmatrix257.h			\
	ran.367.179.h				\
	ran.509.131.h				\
	print-escp2.h

MAINFILES = $(MAINCODE) $(MAINHEADERS)

if BUILD_GHOST
ALL_LOCAL_DEPS = $(srcdir)/$(addprefix gdevstp-,$(MAINCODE)) $(srcdir)/$(addprefix gdevstp-,$(MAINHEADERS))
else
ALL_LOCAL_DEPS =
endif


## Rules

all-local: $(ALL_LOCAL_DEPS)

GHCODE = -e 's/print-printers.c/gdevstp-printers.c/' -e 's/^\\\#include "\(gimp\-\)*\(.*\)\.h"$$/\\\#include "gdevstp-\2.h"/' -e 's/^\\\#include <gimp-print\/gimp-print\(.*\)\.h>$$/\\\#include "gdevstp-print\1.h"/' -e 's/^\\\#include <print-escp2\.h>$$/\\\#include "gdevstp-print-escp2.h"/'

$(srcdir)/gdevstp-print.h: $(top_srcdir)/include/gimp-print/gimp-print.h
	sed $(GHCODE) $< > $(srcdir)/$@

$(srcdir)/gdevstp-print-escp2.h: $(top_srcdir)/src/main/print-escp2.h
	sed $(GHCODE) $< > $(srcdir)/$@

$(srcdir)/gdevstp-print-internal.h: $(top_srcdir)/src/main/gimp-print-internal.h
	sed $(GHCODE) $< > $(srcdir)/$@

$(srcdir)/gdevstp-print-intl-internal.h: $(top_srcdir)/include/gimp-print/gimp-print-intl-internal.h
	sed $(GHCODE) $< > $(srcdir)/$@

gdevstp-%.h: $(srcdir)/../main/%.h Makefile
	cd $(srcdir) ; sed $(GHCODE) $< > $@

gdevstp-printers.c: ../main/print-printers.c
	sed $(GHCODE) $< > $@

gdevstp-%: $(srcdir)/../main/print-% Makefile
	cd $(srcdir) ; sed $(GHCODE) $< > $@

../main/print-printers.c: ../printdef/printdef $(srcdir)/../main/printers.xml
	../printdef/printdef < $(srcdir)/../main/printers.xml > $(top_builddir)/src/main/print-printers.c

printdefl.o: printdefy.o

../printdef/printdef: $(addprefix ../printdef/, printdefl.l printdefy.y printdef.h)
	cd ../printdef ; \
	$(MAKE)


## Clean

CLEANFILES = \
 $(srcdir)/gdevstp-printers.c \
 $(srcdir)/gdevstp-ran.367.179.h \
 $(srcdir)/gdevstp-ran.509.131.h \
 $(srcdir)/gdevstp-color.c \
 $(srcdir)/gdevstp-print.h \
 $(srcdir)/gdevstp-print-escp2.h \
 $(srcdir)/gdevstp-print-internal.h \
 $(srcdir)/gdevstp-print-intl-internal.h \
 $(srcdir)/gdevstp-quickmatrix257.h \
 $(srcdir)/gdevstp-util.c \
 $(srcdir)/gdevstp-weave.c \
 $(srcdir)/gdevstp-canon.c \
 $(srcdir)/gdevstp-dither.c \
 $(srcdir)/gdevstp-dither-matrices.c \
 $(srcdir)/gdevstp-escp2.c \
 $(srcdir)/gdevstp-escp2-data.c \
 $(srcdir)/gdevstp-lexmark.c \
 $(srcdir)/gdevstp-pcl.c \
 $(srcdir)/gdevstp-ps.c

# End of old compatability cruft

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
	COPYING \
	Makefile \
	Makefile.am \
	README \
	README.bsd \
	contrib.mak.addon.in \
	contrib.mak.addon.old \
	debian-patch \
	debian-patch-stp \
	devs.mak.addon-5.10 \
	gdevstp.c \
