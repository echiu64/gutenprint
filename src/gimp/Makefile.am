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

INCLUDES = @INCLUDES@ $(GIMP_CFLAGS)

AM_CPPFLAGS =\
 -DLOCALEDIR=\""$(localedir)"\"\
 @MAINTAINER_CFLAGS@\
 @RELDATE_DEF@

libexecdir = $(gimptool_prefix)/plug-ins

libexec_PROGRAMS = @GIMP_BIN@

EXTRA_PROGRAMS = print

print_SOURCES = \
	print-image-gimp.c \
	print-intl.h \
	print.c \
	gimp_color_window.c \
	gimp_main_window.c \
	gtk_color_window.c \
	gtk_main_window.c \
	print_gimp.h

print_LDADD = $(GIMP_LIBS) $(GIMPPRINT_LIBS)

$(GIMPPRINT_LIBS):
	make -C ../main

install-libexecPROGRAMS: $(libexec_PROGRAMS)
	@list='$(libexec_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    if test -f .libs/$$p; then \
	      cd .libs; \
	    fi; \
	    echo $(GIMPTOOL) --install-@PLUG_IN_PATH@ $$p; \
            $(GIMPTOOL) --install-@PLUG_IN_PATH@ $$p; \
	  else :; fi; \
	done

uninstall-libexecPROGRAMS: $(libexec_PROGRAMS)
	@list='$(libexec_PROGRAMS)'; for p in $$list; do \
	  if test -f "$$p"; then \
	    echo $(GIMPTOOL) --uninstall-@PLUG_IN_PATH@ "$$p"; \
            $(GIMPTOOL) --uninstall-@PLUG_IN_PATH@ "$$p"; \
	    echo $(GIMPTOOL) --uninstall-admin-bin "$$p"; \
	    $(GIMPTOOL) --uninstall-admin-bin "$$p"; \
	  else :; fi; \
	done

CLEANFILES = $(EXTRA_PROGRAMS)
MAINTAINERCLEANFILES = Makefile.in
