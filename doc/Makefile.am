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

MANUAL = gimpprint
HTMLDIR = manual-html
DOCS = $(MANUAL).pdf $(MANUAL).ps $(MANUAL).dvi html

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

if MAINTAINER_MODE
ALL_LOCAL_TARGETS = docs
else
ALL_LOCAL_TARGETS = $(STANDARD_TARGETS)
endif


## Data

info_TEXINFOS = gimpprint.texi

gimpprint_TEXINFOS = overview.texi copying.texi gpl.texi introduction.texi using.texi functions.texi programs.texi problems.texi appendices.texi dither.texi weave.texi escp2.texi new-printer.texi indices.texi integrating.texi version.texi


## Rules

all-local: $(DOCS)

docs: $(DOCS)
	cd users_guide; make docs

ps: $(MANUAL).ps
	cd users_guide; make ps

pdf: $(MANUAL).pdf
	cd users_guide; make pdf

gimpprint.dvi: $(EPS_IMAGES) $(addprefix $(srcdir)/,$(gimpprint_TEXINFOS) $(info_TEXINFOS))

%.eps:
	$(CONVERT) $(srcdir)/$(basename $@).png EPS2:$@

html: html-stamp
	cd users_guide; make html

## Note the rather weird way of setting the exit status; this is because
## texi2html doesn't set the exit status.
html-stamp: $(addprefix $(srcdir)/,$(gimpprint_TEXINFOS) $(info_TEXINFOS))
	$(RM) -r $(HTMLDIR)-tmp
	mkdir $(HTMLDIR)-tmp
	for image in $(addprefix $(srcdir)/, $(IMAGES)) ; do \
	  cp -p $$image $(HTMLDIR)-tmp ; \
	done
	cd $(HTMLDIR)-tmp ; \
	if test $(srcdir) = '.' ; then \
	  SOURCE='..' ; \
	else \
	  SOURCE=$(srcdir) ; \
	fi ; \
	$(TEXI2HTML) -menu -split_node -number -expandinfo \
	      -I $(srcdir) $${SOURCE}/$(MANUAL).texi 2>&1
	$(RM) -r $(HTMLDIR)
	mv $(HTMLDIR)-tmp $(HTMLDIR)
	touch html-stamp

$(MANUAL).ps: $(MANUAL).dvi
	$(DVIPS) $(MANUAL).dvi

$(MANUAL).pdf: $(MANUAL).dvi
	$(DVIPDF) $(MANUAL).dvi

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

CLEAN_MANUALS = \
	$(RM) -r html-stamp manual-html ; \
	$(RM) $(MANUAL).pdf $(MANUAL).ps $(MANUAL).dvi ; \
	$(RM) $(EPS_IMAGES)

clean-local:
	if test $(srcdir) = '.' -a $(MAINT_MODE) = false ; then \
	  echo 'clean-local: Not removing manual-html' ; \
	  echo 'clean-local: Not removing $(MANUAL).pdf' ; \
	  echo 'clean-local: Not removing $(MANUAL).ps' ; \
	  echo 'clean-local: Not removing $(MANUAL).dvi' ; \
	  echo 'clean-local: Not removing EPS figures' ; \
	else \
	  $(CLEAN_MANUALS) ; \
	fi
	$(RM) -r *.tex *.log *.aux *.gz *.out *.junk *.fot
	$(RM) -r *.ky *.pg *.toc *.tp *.vr *.vrs
	$(RM) -r manual-html-tmp

veryclean:
	$(MAKE) MAINT_MODE=true clean

maintainer-clean-local:
	$(CLEAN_MANUALS)

# We duplicate mostlyclean-aminfo here because we do not want to remove
# the .dvi and .ps files that mostlyclean-aminfo wants to remove.  The
# reason why is that we do not want make clean to force a rebuild of the
# doc, since users may not have the texinfo tools required to do so.
# Unfortunately, this generates a warning about overriding commands for
# mostlyclean-aminfo.
mostlyclean-aminfo:
	-rm -f gimpprint.aux gimpprint.cp gimpprint.cps \
	  gimpprint.fn gimpprint.fns gimpprint.ky gimpprint.kys \
	  gimpprint.log gimpprint.pg gimpprint.toc \
	  gimpprint.tp gimpprint.tps gimpprint.vr gimpprint.vrs \
	  gimpprint.op gimpprint.tr gimpprint.cv gimpprint.cn


## Clean

MAINTAINERCLEANFILES = Makefile.in html-stamp

EXTRA_DIST = $(DOCS) $(IMAGES) $(EPS_IMAGES) html-stamp FAQ.html README.maintaining

.PHONY: html ps pdf docs clean-manuals
