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


## Variables

DOCS = \
 FAQ.html \
 README.maintaining \
 docbook.css \
 print-color.png \
 print-main.png \
 print-setup.png \
 print.html \
 print.sgml


## Data

info_TEXINFOS = gimpprint.texi

gimpprint_TEXINFOS = overview.texi copying.texi gpl.texi introduction.texi using.texi functions.texi programs.texi problems.texi appendices.texi dither.texi weave.texi escp2.texi new-printer.texi indices.texi integrating.texi


## Rules

all-local: html

html: html-stamp

## Note the rather weird way of setting the exit status; this is because
## texi2html doesn't set the exit status.
html-stamp: gimpprint.texi $(gimpprint_TEXINFOS)
	stat=1; \
	if test x$(TEXI2HTML_PROG) != x ; then \
	  rm -rf manual-html ; \
	  mkdir manual-html ; \
	  cd manual-html ; \
	    $(TEXI2HTML_PROG) -menu -split_node -number -expandinfo \
	    -I ../$(srcdir) ../$(srcdir)/gimpprint.texi 2>&1 | grep . ; \
	    stat=$$? ; stat=`expr 1 - $$stat` ; \
	fi; \
	exit $$stat
	touch html-stamp

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

maintainer-clean-local:
	rm -rf manual-html


## Clean

DISTCLEANFILES = html-stamp
MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = $(DOCS)

.PHONY: html
