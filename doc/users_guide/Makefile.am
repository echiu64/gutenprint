## $Id$
## Copyright (C) 2001 Andy Stewart and Roger Leigh
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

MANUAL = users_guide
HTMLDIR = $(MANUAL)

OUTPUTS = \
 $(MANUAL).pdf \
 $(MANUAL).ps \
 html

USER_GUIDE = $(MANUAL).pdf html-stamp $(MANUAL).ps

STANDARD_TARGETS=@USER_GUIDE@

DISTHOOKDIRS = figures $(HTMLDIR)
DISTHOOKFILES = $(STANDARD_TARGETS)

PNG_IMAGES = figures/cups_admin.png			\
	figures/cups_admin_device.png			\
	figures/cups_admin_make.png			\
	figures/cups_admin_model.png			\
	figures/cups_admin_success.png			\
	figures/cups_config_printer.png			\
	figures/cups_my_printer.png			\
	figures/cups_printers.png			\
	figures/cups_startup.png			\
	figures/gimp-print-gui-1.png			\
	figures/gimp-print-gui.png			\
	figures/gimp-print-new-printer.png		\
	figures/gimp-print-print-color-adjust.png	\
	figures/gimp-print-setup.png			\
	figures/gimp_image.png				\
	figures/gimp_startup.png

EPS_IMAGES = $(addsuffix .eps,$(basename $(PNG_IMAGES)))
PDF_IMAGES = $(addsuffix .pdf,$(basename $(PNG_IMAGES)))


## Rules

dist-hook: $(STANDARD_TARGETS)
	for dir in $(DISTHOOKDIRS) ; do \
	  if test -d $(srcdir)/$$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $(srcdir)/$$dir/*; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir ; \
	      fi ; \
	      if test -d $$dirfile ; then \
	        cp -pR $$dirfile $(distdir)/$$dir ; \
	      fi ; \
	    done \
	  fi \
	done
	for file in $(DISTHOOKFILES) ; do \
	  if test -f $$file ; then \
	    cp -p $$file $(distdir) ; \
	fi ; \
	done

#
# This generates nicer PDF than the uncommented method.
# However, the pictures are missing.  I don't know
# what format to supply to DocBook get the correct output.
#
#$(MANUAL).pdf:  $(MANUAL).sgml $(PDF_IMAGES)
#	export SGML_CATALOG_FILES=$(CATALOG_FILES); \
#	$(DB2PDF) $(srcdir)/$(MANUAL).sgml

docs: html-stamp ps pdf

pdf: $(MANUAL).pdf

$(MANUAL).pdf: $(MANUAL).ps
	$(PS2PDF) $(MANUAL).ps $(MANUAL).pdf

%.pdf:
	if test ! -d figures ; then \
	  mkdir figures ; \
	fi
	$(CONVERT) $(srcdir)/$(basename $@).png EPDF:$@

ps: $(MANUAL).ps

ps.gz: $(MANUAL).ps.gz

$(MANUAL).ps:  $(MANUAL).sgml $(EPS_IMAGES)
	$(DB2PS) $(srcdir)/$(MANUAL).sgml

%.eps:
	if test ! -d figures ; then \
	  mkdir figures ; \
	fi
	$(CONVERT) $(srcdir)/$(basename $@).png EPS2:$@

$(MANUAL).ps.gz: $(MANUAL).ps
	$(RM) $(MANUAL).ps.gz
	$(GZIP) -9 $(MANUAL).ps

html: html-stamp
html-stamp: $(MANUAL).sgml $(PNG_IMAGES)
	if test $(srcdir) = '.' ; then \
	  SOURCE='' ; \
	else \
	  SOURCE='$(srcdir)/' ; \
	fi ; \
	$(DB2HTML) $${SOURCE}$(MANUAL).sgml ; \
	if test ! -d $(MANUAL)/figures  ; then \
	  mkdir $(MANUAL)/figures ; \
	  for image in $(srcdir)/figures/*.png ; do \
	    cp -p $$image $(MANUAL)/figures ; \
	  done ; \
	fi ; \
	if test ! $(HTMLDIR) = $(MANUAL) ; then \
	  mv $(MANUAL) $(HTMLDIR) ; \
	fi ; \
	chmod a+rx $(HTMLDIR); \
	touch html-stamp

# Symlink to distributed html manual, if possible
all-local: $(STANDARD_TARGETS)
	if test -d $(srcdir)/$(HTMLDIR) ; then \
	  if test "$(srcdir)" = "." ; then \
	    echo "all-local: Not removing $(HTMLDIR)" ; \
	  else \
	    $(RM) $(HTMLDIR) ; \
	    $(LN_S) $(srcdir)/$(HTMLDIR) $(HTMLDIR) ; \
	  fi ; \
	  else \
	    echo "all-local: $(srcdir)/$(HTMLDIR) not found" ; \
	fi

install-data-local: $(STANDARD_TARGETS)
	if test -n '$(STANDARD_TARGETS)' ; then \
	  $(mkinstalldirs) $(datadir)/$(PACKAGE)/doc/figures ; \
	  $(mkinstalldirs) $(datadir)/$(PACKAGE)/doc/stylesheet-images ; \
	  $(INSTALL_DATA) $(MANUAL).pdf $(datadir)/$(PACKAGE)/doc ; \
	  cd users_guide ; \
	  for file in *.html *.css ; do \
	    $(INSTALL_DATA) $$file $(datadir)/$(PACKAGE)/doc ; \
	  done ; \
	  cd figures ; \
	  for file in *.png ; do \
	    $(INSTALL_DATA) $$file $(datadir)/$(PACKAGE)/doc/figures ; \
	  done ; \
	  cd ../stylesheet-images ; \
	  for file in * ; do \
	    $(INSTALL_DATA) $$file $(datadir)/$(PACKAGE)/doc/stylesheet-images ; \
	  done ; \
	fi

uninstall-local:
	$(RM) $(datadir)/$(PACKAGE)/doc

clean-local: clean-figures
	if test $(srcdir) = "." ; then \
	  echo "clean-local: Not removing $(HTMLDIR)" ; \
	else \
	  $(RM) html-stamp $(HTMLDIR) ; \
	fi
	$(RM) -r $(OUTPUTS)
	$(RM) *.tex *.log *.aux *.dvi *.gz
	$(RM) -r $(MANUAL).junk
	$(RM) -r $(MANUAL).out
	$(RM) -r db2html*
	$(RM) -r DB2HTML*

clean-figures:
	$(RM) figures/*.eps figures/*.pdf

maintainer-clean-local:
	$(RM) -r $(HTMLDIR) html-stamp

#
# This is my (so far failed) attempt to create an index for this documentation.
#
#
#index.sgml:
#	$(PERL) /usr/share/sgml/docbook/docbook-dsssl-stylesheets-1.64/bin/collateindex.pl -N -o index.sgml
#
#	$(JADE)	-V html-index \
#		-t sgml \
#		-c /usr/share/sgml/CATALOG.jade_dsl \
#		-c /usr/share/sgml/CATALOG.docbook-dsssl-stylesheets \
#		-c /usr/share/sgml/CATALOG.gnome \
#		-c /usr/share/sgml/CATALOG.docbook_4 \
#		-c /usr/share/sgml/CATALOG.docbook_3  \
#		-D /usr/share/sgml/docbook/docbook-dsssl-stylesheets-1.64 \
#		-d /usr/share/sgml/docbook-toys/suse-both.dsl \
#		-o index.sgml \
#		gimp-print-users-guide-4.1.99-b3.sgml
#
#
# END failed attempt at index creation.  
#

help:
	@echo "  "
	@echo "Supported targets in this Makefile:"
	@echo "    doc     - creates PS, PDF, and HTML documentation files"
	@echo "    ps      - creates PostScript documentation"
	@echo "    pdf     - creates PDF documentation"
	@echo "    html    - creates HTML documentation"
	@echo "    clean   - deletes machine generated files"
	@echo "    help    - prints this help text"
	@echo "  "


## Clean

CLEANFILES = $(EPS_IMAGES) $(PDF_IMAGES)

MAINTAINERCLEANFILES = Makefile.in html-stamp

EXTRA_DIST = README $(MANUAL).sgml
