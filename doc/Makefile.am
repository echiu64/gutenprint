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

if MAINTAINER_MODE
MAINT_MODE = true
else
MAINT_MODE = false
endif


## Data

info_TEXINFOS = gimpprint.texi

gimpprint_TEXINFOS = overview.texi copying.texi gpl.texi introduction.texi using.texi functions.texi programs.texi problems.texi appendices.texi dither.texi weave.texi escp2.texi new-printer.texi indices.texi integrating.texi version.texi


## Rules

all-local: html

docs: html ps pdf
	cd users_guide ; \
	$(MAKE) html ps pdf

gimpprint.dvi: $(EPS_IMAGES)

%.eps:
	$(CONVERT) $(srcdir)/$(basename $@).png EPS2:$@

html: html-stamp
	cd users_guide ; \
	$(MAKE) html

## Note the rather weird way of setting the exit status; this is because
## texi2html doesn't set the exit status.
html-stamp: $(srcdir)/gimpprint.texi $(addprefix $(srcdir)/,$(gimpprint_TEXINFOS))
	if test '$(srcdir)' = '.' ; then \
	  : ; \
	else \
	  if test -f $(srcdir)/html-stamp ; then \
	    $(LN_S) $(srcdir)/html-stamp html-stamp ; \
	  fi ; \
	fi ; \
	if test -f html-stamp -o -L html-stamp ; then \
	  : ; \
	else \
	  if test x$(TEXI2HTML) = x ; then \
	    echo "html-stamp: texi2html not found: not regenerating HTML manual" ; \
	  else \
	    stat=1; \
	    rm -rf manual-html-tmp ; \
	    mkdir manual-html-tmp ; \
	    cd manual-html-tmp ; \
	    if test $(srcdir) = '.' ; then \
	      SOURCE='..' ; \
	    else \
	      SOURCE=$(srcdir) ; \
	    fi ; \
	    cp $${SOURCE}/*.png . ; \
	    $(TEXI2HTML) -menu -split_node -number -expandinfo \
	      -I $(srcdir) $${SOURCE}/gimpprint.texi 2>&1 | grep . ; \
	      stat=$$? ; stat=`expr 1 - $$stat` ; \
	  fi ; \
	  cd .. ; \
	  rm -rf manual-html ; \
	  mv manual-html-tmp manual-html ; \
	  touch html-stamp ; \
	fi ; \
	exit $$stat

ps: gimpprint.ps
	cd users_guide ; \
	$(MAKE) ps

gimpprint.ps: gimpprint.dvi
	$(DVIPS) -o gimpprint.ps gimpprint.dvi

pdf: gimpprint.pdf
	cd users_guide ; \
	$(MAKE) pdf

gimpprint.pdf: gimpprint.ps
	$(PS2PDF) gimpprint.ps gimpprint.pdf

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

CLEAN_MANUALS = $(RM) -r html-stamp manual-html

clean-local:
	if test $(srcdir) = '.' -a $(MAINT_MODE) = false ; then \
	  echo 'clean-local: Not removing manual-html' ; \
	else \
	  $(CLEAN_MANUALS) ; \
	fi
	$(RM) -r manual-html-tmp

maintainer-clean-local:
	$(CLEAN_MANUALS)


## Clean

CLEANFILES = $(EPS_IMAGES) $(TXT_IMAGES) gimpprint.pdf

MAINTAINERCLEANFILES = Makefile.in html-stamp

EXTRA_DIST = $(DOCS) $(IMAGES) html-stamp

.PHONY: html ps pdf docs clean-manuals
