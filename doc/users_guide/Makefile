###############################################################
#
# Makefile - by Andy Stewart - 2-aug-2001
#
# The tools and revisions used to build these output files
# from the SGML source are noted below.  The work was done
# on a system running SuSE Linux 7.2 Professional.  I may
# have included more packages below than what are actually
# required.  In fact, I may have omitted some.  :-)
#
# I probably should have somehow coded this into a
# ./configure file and passed the info into the Makefile.
# Oh well, perhaps in a future revision.
#
# db2pdf 	package docbktls-0.15.2-20 
#
# db2ps 	package docbktls-0.15.2-20 
#
# db2html	package docbktls-0.15.2-20
#
# docbook	packages docbook_4-4.1-25
#
# jade		packages jade_dsl-1.2.1-296
#			 jadetex-3.5-24
#			 openjade-1.3-213
#
# LaTeX		package te_latex-1.0.7-196
#
# ps2pdf	Ghostscript 6.01
#
###############################################################

TOP := $(shell /bin/pwd)

RELEASE_DIR = users_guide

VERSION = 4.1.99-b3

SOURCE = gimp-print-users-guide-$(VERSION)

HTML_DIR = $(TOP)/$(SOURCE)

OUTPUTS = \
	$(SOURCE).pdf \
	$(SOURCE).ps \
	html

DIST_FILES = \
	Makefile \
	README \
	$(SOURCE).sgml \
	figures

CATALOG_FILES = /etc/sgml/catalog

all:  $(SOURCE).sgml $(OUTPUTS) 

release: $(RELEASE_DIR).tgz

#
# This generates nicer PDF than the uncommented method.
# However, the pictures are missing.  I don't know
# what format to supply to DocBook get the correct output.
#
#$(SOURCE).pdf:  $(SOURCE).sgml
#	export SGML_CATALOG_FILES=$(CATALOG_FILES); \
#		db2pdf $(SOURCE).sgml

$(SOURCE).pdf: $(SOURCE).ps
	ps2pdf $(SOURCE).ps $(SOURCE).pdf

$(SOURCE).ps:  $(SOURCE).sgml
	export SGML_CATALOG_FILES=$(CATALOG_FILES); \
		db2ps $(SOURCE).sgml

$(SOURCE).ps.gz: $(SOURCE).ps
	rm -f $(SOURCE).ps.gz
	gzip -9 $(SOURCE).ps

html: $(SOURCE).sgml
	export SGML_CATALOG_FILES=$(CATALOG_FILES); \
		db2html $(SOURCE).sgml
	if [ ! -L $(SOURCE)/figures ]; then \
		cd $(HTML_DIR); \
		ln -s ../figures figures; \
	fi
	ln -s $(SOURCE) html

$(RELEASE_DIR).tgz: $(OUTPUTS)
	rm -fr $(RELEASE_DIR)
	mkdir $(RELEASE_DIR)
	cp -r $(DIST_FILES) $(RELEASE_DIR)
	tar cvzf $(RELEASE_DIR).tgz $(RELEASE_DIR)
		
clean:
	rm -f $(OUTPUTS)
	rm -fr $(USERS_GUIDE_DIR)
	rm -f *.tex *.log *.aux *.dvi *~ *.tgz
	rm -fr $(SOURCE)
	rm -fr $(SOURCE).junk
	rm -f $(SOURCE).out
	rm -fr db2html.*
	rm -fr $(RELEASE_DIR)

#
# This is my (so far failed) attempt to create an index for this documentation.
#

index.sgml:
	perl /usr/share/sgml/docbook/docbook-dsssl-stylesheets-1.64/bin/collateindex.pl -N -o index.sgml

	jade 	-V html-index \
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
