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
HTMLDIR = html

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

$(MANUAL).pdf:  $(MANUAL).sgml
	$(DB2PDF) $(srcdir)/$(MANUAL).sgml

docs: html-stamp ps pdf

all-local: docs

pdf: $(MANUAL).pdf

ps: $(MANUAL).ps

$(MANUAL).ps:  $(MANUAL).sgml $(EPS_IMAGES)
	sed -e 's/\.png"/.eps"/g' -e 's/FORMAT="PNG"/FORMAT="EPS"/g' $(srcdir)/$(MANUAL).sgml > $(MANUAL)_eps.sgml
	$(DB2PS) $(MANUAL)_eps.sgml
	mv $(MANUAL)_eps.ps $(MANUAL).ps
	$(RM) $(MANUAL)_eps.*

%.eps:
	if test ! -d figures ; then mkdir figures ; fi
	$(CONVERT) $(srcdir)/$(basename $@).png EPS2:$@

html: html-stamp

# This ugly workaround with SOURCE is because db2html doesn't seem to like
# "./users-guide.sgml".  Note that db2ps does not have the same limitation.
html-stamp: $(MANUAL).sgml $(PNG_IMAGES)
	if test $(srcdir) = '.' ; then \
	  SOURCE='' ; \
	else \
	  SOURCE='$(srcdir)/' ; \
	fi ; \
	$(DB2HTML) $${SOURCE}$(MANUAL).sgml
	-mkdir $(MANUAL)/figures
	for image in $(addprefix $(srcdir)/, $(PNG_IMAGES)) ; do \
	  cp -p $$image $(MANUAL)/figures/`basename $$image` ; \
	done
	$(RM) -rf $(HTMLDIR)
	mv $(MANUAL) $(HTMLDIR)
	chmod a+rx $(HTMLDIR)
	touch html-stamp

install-data-local: $(STANDARD_TARGETS)
	if test -n '$(STANDARD_TARGETS)' ; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/doc/figures ; \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/doc/stylesheet-images ; \
	  $(INSTALL_DATA) $(MANUAL).pdf $(DESTDIR)$(datadir)/$(PACKAGE)/doc ; \
	  cd $(HTMLDIR) ; \
	  for file in *.html *.css ; do \
	    if test -f $$file ; then \
	      $(INSTALL_DATA) $$file $(DESTDIR)$(datadir)/$(PACKAGE)/doc ; \
	    fi ; \
	  done ; \
	  cd figures ; \
	  for file in *.png ; do \
	    if test -f $$file ; then \
	      $(INSTALL_DATA) $$file $(DESTDIR)$(datadir)/$(PACKAGE)/doc/figures ; \
	    fi ; \
	  done ; \
	  cd ../stylesheet-images ; \
	  for file in * ; do \
	    if test -f $$file ; then \
	      $(INSTALL_DATA) $$file $(DESTDIR)$(datadir)/$(PACKAGE)/doc/stylesheet-images ; \
	    fi ; \
	  done ; \
	fi

uninstall-local:
	$(RM) -r $(DESTDIR)$(datadir)/$(PACKAGE)/doc

clean-local:
	if test $(srcdir) = "." ; then \
	  echo "clean-local: Not removing $(HTMLDIR)" ; \
	  echo 'clean-local: Not removing $(MANUAL).pdf' ; \
	  echo 'clean-local: Not removing $(MANUAL).ps' ; \
	  echo 'clean-local: Not removing EPS figures' ; \
	else \
	  $(RM) $(MANUAL).pdf $(MANUAL).ps ; \
	  $(RM) -r html-stamp $(HTMLDIR) ; \
	  $(RM) -r $(EPS_IMAGES) ; \
	fi
	$(RM) -r *.tex *.log *.aux *.dvi *.gz *.out *.junk *.out *.fot
	$(RM) -r db2html*
	$(RM) -r DB2HTML*

veryclean: clean maintainer-clean-local

maintainer-clean-local:
	$(RM) -r $(HTMLDIR) html-stamp
	$(RM) $(MANUAL).pdf $(MANUAL).ps
	$(RM) -r $(EPS_IMAGES)

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
	@echo "    docs      - creates PS, PDF, and HTML documentation files"
	@echo "    ps        - creates PostScript documentation"
	@echo "    pdf       - creates PDF documentation"
	@echo "    html      - creates HTML documentation"
	@echo "    clean     - deletes machine generated files, but not outputs"
	@echo "    veryclean - deletes output files in addition"
	@echo "    help      - prints this help text"
	@echo "  "


## Clean

MAINTAINERCLEANFILES = Makefile.in html-stamp


EXTRA_DIST = README $(MANUAL).sgml html-stamp

.PHONY: docs html ps pdf help
