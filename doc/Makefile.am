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

SUBDIRS = users_guide


## Variables

DOCS = \
 FAQ.html \
 README.maintaining

IMAGES = \
 print-color.png \
 print-main.png \
 print-setup.png
   
EPS_IMAGES = $(addsuffix .eps,$(basename $(IMAGES)))


## Data

info_TEXINFOS = gimpprint.texi

gimpprint_TEXINFOS = overview.texi copying.texi gpl.texi introduction.texi using.texi functions.texi programs.texi problems.texi appendices.texi dither.texi weave.texi escp2.texi new-printer.texi indices.texi integrating.texi version.texi


## Rules

# Symlink to distributed html manual, if possible
all-local: html
	if test -d $(srcdir)/manual-html ; then \
	  if test "$(srcdir)" = "." ; then \
	    echo "all-local: Not removing manual-html" ; \
	  else \
	    rm -rf manual-html ; \
	    $(LN_S) $(srcdir)/manual-html manual-html ; \
	  fi \
	else \
	  echo "all-local: $(srcdir)/manual-html not found" ; \
	fi

gimpprint.dvi: $(EPS_IMAGES)

%.eps:
	$(CONVERT_PROG) $(srcdir)/$(basename $@).png EPS2:$@

html: html-stamp

## Note the rather weird way of setting the exit status; this is because
## texi2html doesn't set the exit status.
html-stamp: $(srcdir)/gimpprint.texi $(addprefix $(srcdir)/,$(gimpprint_TEXINFOS))
	if test x$(TEXI2HTML_PROG) = x ; then \
	  echo "html-stamp: texi2html not found: not regenerating HTML manual" ; \
	else \
	  stat=1; \
	  rm -rf manual-html-tmp ; \
	  mkdir manual-html-tmp ; \
	  cd manual-html-tmp ; \
	  cp ../*.png . ; \
	  $(TEXI2HTML_PROG) -menu -split_node -number -expandinfo \
	    -I ../$(srcdir) ../$(srcdir)/gimpprint.texi 2>&1 | grep . ; \
	    stat=$$? ; stat=`expr 1 - $$stat` ; \
	fi ; \
	exit $$stat
	rm -rf manual-html ; \
	mv manual-html-tmp manual-html ; \
	touch html-stamp

ps: gimpprint.ps
gimpprint.ps: gimpprint.dvi
	$(DVIPS_PROG) -o gimpprint.ps gimpprint.dvi

dist-hook: html
	for dir in manual-html ; do \
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
	if test $(srcdir) = "." ; then \
	  echo "clean-local: Not removing manual-html" ; \
	else \
	  rm -rf html-stamp manual-html ; \
	fi
	rm -rf manual-html-tmp

maintainer-clean-local:
	rm -rf manual-html


## Clean

CLEANFILES = $(EPS_IMAGES) $(TXT_IMAGES)

MAINTAINERCLEANFILES = Makefile.in html-stamp

EXTRA_DIST = $(DOCS) $(IMAGES) html-stamp

.PHONY: html
