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

# Use old compatability method for now...

MAINCODE = canon.c color.c dither.c escp2.c lexmark.c pcl.c printers.c ps.c util.c weave.c

MAINHEADERS = print.h quickmatrix257.h ran.367.179.h ran.509.131.h

MAINFILES = $(MAINCODE) $(MAINHEADERS)

if GHOST_BIN
all-local: $(srcdir)/$(addprefix gdevstp-,$(MAINCODE)) $(srcdir)/$(addprefix gdevstp-,$(MAINHEADERS))
endif

GHCODE = -e 's/^\\\#include "\(.*\)\.h"$$/\\\#include "gdevstp-\1.h"/'

$(srcdir)/gdevstp-print.h: $(top_srcdir)/src/include/print.h
	sed -e 's/print-printers.c/gdevstp-printers.c/' $(GHCODE) $< > $(srcdir)/$@

gdevstp-%.h: $(srcdir)/../main/%.h Makefile
	cd $(srcdir) ; \
	sed -e 's/print-printers.c/gdevstp-printers.c/' $(GHCODE) $< > $@

gdevstp-printers.c: ../main/print-printers.c
	sed -e 's/print-printers.c/gdevstp-printers.c/' $(GHCODE) $< > $(srcdir)/$@

gdevstp-%: $(srcdir)/../main/print-% Makefile
	cd $(srcdir) ; \
	sed -e 's/print-printers.c/gdevstp-printers.c/' $(GHCODE) $< > $@

../main/print-printers.c: ../printdef/printdef $(srcdir)/../main/printers.xml
	../printdef/printdef < $(srcdir)/../main/printers.xml > $(top_builddir)/src/main/print-printers.c

printdefl.o: printdefy.o

../printdef/printdef: $(addprefix ../printdef/, printdefl.l printdefy.y printdef.h)
	        $(MAKE) -C ../printdef

CLEANFILES = \
 $(srcdir)/gdevstp-printers.c \
 $(srcdir)/gdevstp-ran.367.179.h \
 $(srcdir)/gdevstp-ran.509.131.h \
 $(srcdir)/gdevstp-color.c \
 $(srcdir)/gdevstp-print.h \
 $(srcdir)/gdevstp-quickmatrix257.h \
 $(srcdir)/gdevstp-util.c \
 $(srcdir)/gdevstp-weave.c \
 $(srcdir)/gdevstp-canon.c \
 $(srcdir)/gdevstp-dither.c \
 $(srcdir)/gdevstp-escp2.c \
 $(srcdir)/gdevstp-lexmark.c \
 $(srcdir)/gdevstp-pcl.c \
 $(srcdir)/gdevstp-ps.c

# End of old compatability cruft

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = \
	COPYING \
	ChangeLog \
	Makefile \
	Makefile.am \
	README \
	README.bsd \
	contrib.mak.addon \
	debian-patch \
	debian-patch-stp \
	devs.mak.addon-5.10 \
	gdevstp.c \
