## Process this file with automake to produce Makefile.in

libexecdir = $(gimptool_prefix)/plug-ins

libexec_PROGRAMS = print

bin_PROGRAMS = escputil @MAINTAINER_PROGRAMS@

EXTRA_PROGRAMS = escp2-weavetest printer_options unprint pcl-unprint

noinst_PROGRAMS = printdef

COMMON_SOURCE = \
	print-lexmark.c   	\
	print-canon.c   	\
	print-dither.c		\
	print-escp2.c		\
	print-pcl.c		\
	print-ps.c      	\
	print-util.c		\
	print-color.c		\
	print-weave.c		\
	print.h			\
	quickmatrix257.h	\
	ran.367.179.h		\
	ran.509.131.h

AUX_COMMON_SOURCE = \
	print-printers.c

ALL_COMMON_SOURCE = $(COMMON_SOURCE) $(AUX_COMMON_SOURCE)

print_SOURCES = \
	$(COMMON_SOURCE)	\
	print-image-gimp.c	\
	print-intl.h		\
	print.c			\
	gimp_color_window.c	\
	gimp_main_window.c	\
        gtk_color_window.c	\
	gtk_main_window.c	\
	print_gimp.h

$(srcdir)/print-util.c: $(srcdir)/print-printers.c

$(srcdir)/print-printers.c: ./printdef $(srcdir)/printers.xml
	./printdef < $(srcdir)/printers.xml > $(srcdir)/print-printers.c

printdef_SOURCES = printdefl.l printdefy.y printdef.h

printer_options_SOURCES = printer_options.c $(COMMON_SOURCE)

printer_options_LDADD = -lm

printdefl.o: printdefy.o

LFLAGS = -i
YFLAGS = -d

unprint_SOURCES = unprint.c
escp2_weavetest_SOURCES = escp2-weavetest.c
pcl_unprint_SOURCES = pcl-unprint.c

GCOPY=	$(addprefix gdevstp-, $(patsubst print-%,%, $(ALL_COMMON_SOURCE)))
CUPSCOPY = $(addprefix cups-, $(patsubst print-%,%, $(ALL_COMMON_SOURCE)))

CLEANFILES = printdefl.c printdefy.c printdefy.h print-printers.c \
	$(addprefix Ghost/, $(GCOPY)) $(addprefix cups/, $(CUPSCOPY))

MAINTAINERCLEANFILES = configure Makefile.in aclocal.m4
MAINTAINERdir = $(bindir)

INCLUDES = \
 -I$(top_srcdir)\
 $(GIMP_CFLAGS)\
 -I$(includedir)

AM_CPPFLAGS =\
 -DLOCALEDIR=\""$(localedir)"\"\
 @MAINTAINER_CFLAGS@\
 @RELDATE_DEF@

print_LDADD = $(GIMP_LIBS)

escputil_LDADD = $(LIBREADLINE_DEPS)

GHOST_DIST = COPYING ChangeLog README README.bsd contrib.mak.addon \
	gdevstp.c devs.mak.addon-5.10 escputil.c escputil.1 Makefile \
	debian-patch debian-patch-stp $(GCOPY)

CUPS_DIST = CHANGES.txt LICENSE.txt Makefile.in README.txt configure \
	commandtoepson.c epson.c genppd.c rastertoprinter.c calibrate.c \
	calibrate.ppm command.types commandtocanon.c canon.c \
	$(CUPSCOPY)

DOC_DIST = print-color.png docbook.css print-main.png print.html \
	print-setup.png print.sgml

EXTRA_DIST = print-printers.c printers.xml printdefy.h parse-escp2 RELNOTES \
	README.dither quickmatrix257.h ran.367.179.h ran.509.131.h \
	make-deb.sh README.escp2 escputil.1 LICENSE \
	$(addprefix Ghost/, $(GHOST_DIST)) \
	$(addprefix cups/, $(CUPS_DIST)) \
	$(addprefix Documentation/, $(DOC_DIST)) \
	$(shell if test -d Ghost/stp-interfaces ; then find Ghost/stp-interfaces -type f -print ; fi) \
	$(FORCE_ACLOCAL)

man_MANS = escputil.1

install-libexecPROGRAMS: $(libexec_PROGRAMS)
	@list='$(libexec_PROGRAMS)'; for p in $$list; do \
		if test -f $$p; then \
			echo $(GIMPTOOL) --install-@PLUG_IN_PATH@ $$p; \
			$(GIMPTOOL) --install-@PLUG_IN_PATH@ $$p; \
		else :; fi; \
	done

uninstall-libexecPROGRAMS: $(libexec_PROGRAMS)
	@list='$(libexec_PROGRAMS)'; for p in $$list; do \
		if test -f "$$p"; then \
			echo $(GIMPTOOL) --uninstall-@PLUG_IN_PATH@ "$$p"; \
			$(GIMPTOOL) --uninstall-@PLUG_IN_PATH@ "$$p"; \
		else :; fi; \
	done

.PHONY: files ghost ChangeLog force_aclocal doit

ChangeLog:
	cd $(top_srcdir) ; \
	cvs log | perl ./mkchlog > ChangeLog.tmp ; \
	mv ChangeLog.tmp /ChangeLog

files:
	@cd $(top_srcdir) ; \
	files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

GHCODE=-e 's/^\\\#include "\(.*\)\.h"$$/\\\#include "gdevstp-\1.h"/'

ghost: $(top_srcdir)/$(addprefix Ghost/, $(GCOPY))
	cp $(top_srcdir)/escputil.c $(top_srcdir)/Ghost/escputil.c ; \
	cp escputil.1 $(top_srcdir)/Ghost/escputil.1

Ghost/gdevstp-%.h: $(top_srcdir)/%.h Makefile
	cd $(top_srcdir) ; \
	sed -e 's/print-printers.c/gdevstp-printers.c/' $(GHCODE) $< > $@

Ghost/gdevstp-%: $(top_srcdir)/print-% Makefile
	cd $(top_srcdir) ; \
	sed -e 's/print-printers.c/gdevstp-printers.c/' $(GHCODE) $< > $@

CUPSCODE=-e 's/^\\\#include "\(.*\)\.h"$$/\\\#include "cups-\1.h"/'

cups: $(top_srcdir)/$(addprefix cups/, $(CUPSCOPY))

cups/cups-%.h: $(srcdir)/%.h Makefile
	cd $(srcdir) ; \
	sed -e 's/print-printers.c/cups-printers.c/' $(CUPSCODE) $< > $@

cups/cups-%: $(srcdir)/print-% Makefile
	cd $(srcdir) ; \
	sed -e 's/print-printers.c/cups-printers.c/' $(CUPSCODE) $< > $@

FORCE_ACLOCAL=PLEASE\ RUN\ make\ gdist\ rather\ than\ make\ dist

# If you are using a version of the Gimp from 1.1.25 through 1.1.28,
# you will need to apply the following patch to your
# /usr/share/aclocal/gimp.m4 before you run make gdist, or the result
# will blow up!  This problem is fixed as of the Gimp 1.1.29.
# 
# *** /usr/share/aclocal/gimp.m4~ Sat Sep 23 19:36:20 2000
# --- /usr/share/aclocal/gimp.m4  Tue Oct 10 21:29:52 2000
# ***************
# *** 50,55 ****
# --- 50,60 ----
#   
#       GIMP_DATA_DIR=`$GIMPTOOL $gimptool_args --gimpdatadir`
#       GIMP_PLUGIN_DIR=`$GIMPTOOL $gimptool_args --gimpplugindir`
# +     nodatadir_test=`echo $GIMP_DATA_DIR | sed 's/^\(Usage\).*/\1/'`
# +     if test "$nodatadir_test" = "Usage" ; then
# +        GIMP_DATA_DIR=""
# +        GIMP_PLUGIN_DIR=""
# +     fi
#   
#       gimptool_major_version=`$GIMPTOOL $gimptool_args --version | \
#              sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`

gdist: ghost $(top_srcdir)/cups force_aclocal

doit:
	-@exit 0

Ghost/stp-md5sum: doit
	cd $(top_srcdir) ; \
	wget -q -O Ghost/stp-md5sum.new http://www.linuxprinting.org/stp-interfaces.tar.gz.md5sum ; \
	if [ -f Ghost/stp-md5sum -a -f Ghost/stp-interfaces.tar.gz ] ; then \
		(cd Ghost; md5sum stp-interfaces.tar.gz > stp-md5sum.tmp ) ; \
		cat Ghost/stp-md5sum.new Ghost/stp-md5sum.tmp ; \
		if ! cmp -s Ghost/stp-md5sum.tmp Ghost/stp-md5sum.new ; then \
			cp Ghost/stp-md5sum.new Ghost/stp-md5sum ; \
			echo 'Compare failed:' ; \
		fi ;\
		rm -f Ghost/stp-md5sum.tmp Ghost/stp-md5sum.new ; \
	else \
		mv Ghost/stp-md5sum.new Ghost/stp-md5sum ; \
	fi ; \
	exit 0

Ghost/stp-interfaces.tar.gz: $(top_srcdir)/Ghost/stp-md5sum
	cd $(top_srcdir) ; \
	wget -q -O Ghost/stp-interfaces.tar.gz http://www.linuxprinting.org/stp-interfaces.tar.gz

# Until we get the foomatic stuff solved, don't use it in 4.1.
#force_aclocal: Ghost/stp-interfaces.tar.gz
#	touch configure.in
#	-rm -rf Ghost/stp-interfaces
#	(cd Ghost;tar xzvf stp-interfaces.tar.gz)
#	-find Ghost/stp-interfaces -name '*~' -exec rm -f '{}' \;
#	$(MAKE) dist FORCE_ACLOCAL=

force_aclocal:
	cd $(top_srcdir) ; \
	touch configure.in ; \
	$(MAKE) dist FORCE_ACLOCAL=

print_gimp.h: Makefile
	touch $(top_srcdir)/print_gimp.h

testdither: testdither.o print-dither.o
	$(CC) $(CFLAGS) -o testdither testdither.o print-dither.o -lm

testdither.o: $(srcdir)/print.h
