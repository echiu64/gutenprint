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

RELEASE_DIR = users_guide
VERSION = 4.1.99-b3
MANUAL = gimp-print-users-guide-$(VERSION)
HTML_DIR = $(srcdir)/$(MANUAL)

OUTPUTS = \
 $(MANUAL).pdf \
 $(MANUAL).ps \
 html

DISTDIRS = figures

CATALOG_FILES = /etc/sgml/catalog


## Rules

all:  $(MANUAL).sgml $(OUTPUTS)

release: $(RELEASE_DIR).tar.gz

dist-hook:
	for dir in $(DISTDIRS) ; do \
	  if test -d $(srcdir)/$$dir ; then \
	    mkdir $(distdir)/$$dir; \
	    for dirfile in $(srcdir)/$$dir/*; do \
	      if test -f $$dirfile ; then \
	        cp -p $$dirfile $(distdir)/$$dir; \
	      fi \
	    done \
	  fi \
	done

#
# This generates nicer PDF than the uncommented method.
# However, the pictures are missing.  I don't know
# what format to supply to DocBook get the correct output.
#
#$(MANUAL).pdf:  $(MANUAL).sgml
#	export SGML_CATALOG_FILES=$(CATALOG_FILES); \
#		$(DB2PDF) $(MANUAL).sgml

$(MANUAL).pdf: $(MANUAL).ps
	$(PS2PDF) $(MANUAL).ps $(MANUAL).pdf

$(MANUAL).ps:  $(MANUAL).sgml
	export SGML_CATALOG_FILES=$(CATALOG_FILES); \
	$(DB2PS) $(MANUAL).sgml

$(MANUAL).ps.gz: $(MANUAL).ps
	rm -f $(MANUAL).ps.gz
	$(GZIP) -9 $(MANUAL).ps

html: $(MANUAL).sgml
	export SGML_CATALOG_FILES=$(CATALOG_FILES); \
	$(DB2HTML) $(MANUAL).sgml
	if [ ! -L $(MANUAL)/figures ]; then \
	  cd $(HTML_DIR); \
	  $(LN_S) ../figures figures; \
	fi
	$(LN_S) $(MANUAL) html

$(RELEASE_DIR).tar.gz: $(OUTPUTS)
	rm -fr $(RELEASE_DIR)
	mkdir $(RELEASE_DIR)
	cp -r $(DIST_FILES) $(RELEASE_DIR)
	tar cvf - $(RELEASE_DIR) | $(GZIP) -9 > $(RELEASE_DIR).tgz
		
clean-local:
	rm -f $(OUTPUTS)
	rm -f *.tex *.log *.aux *.dvi *.tgz
	rm -fr $(MANUAL)
	rm -fr $(MANUAL).junk
	rm -f $(MANUAL).out
	rm -fr db2html.*
	rm -fr $(RELEASE_DIR)

#
# This is my (so far failed) attempt to create an index for this documentation.
#

index.sgml:
	perl /usr/share/sgml/docbook/docbook-dsssl-stylesheets-1.64/bin/collateindex.pl -N -o index.sgml

	$(JADE)	-V html-index \
		-t sgml \
		-c /usr/share/sgml/CATALOG.jade_dsl \
		-c /usr/share/sgml/CATALOG.docbook-dsssl-stylesheets \
		-c /usr/share/sgml/CATALOG.gnome \
		-c /usr/share/sgml/CATALOG.docbook_4 \
		-c /usr/share/sgml/CATALOG.docbook_3  \
		-D /usr/share/sgml/docbook/docbook-dsssl-stylesheets-1.64 \
		-d /usr/share/sgml/docbook-toys/suse-both.dsl \
		-o index.sgml \
		gimp-print-users-guide-4.1.99-b3.sgml

#
# END failed attempt at index creation.  
#

help:
	@echo "  "
	@echo "Supported targets in this Makefile:"
	@echo "    all     - creates PS, PDF, and HTML documentation files"
	@echo "    clean   - deletes machine generated files"
	@echo "    help    - prints this help text"
	@echo "    release - creates a release directory and tar/gzips it"
	@echo "  "


## Clean

MAINTAINERCLEANFILES = Makefile.in

EXTRA_DIST = README $(MANUAL).sgml
