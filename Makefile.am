## Process this file with automake to produce Makefile.in

libexecdir = $(gimptool_prefix)/plug-ins

libexec_PROGRAMS = print

bin_PROGRAMS = unprint escp2-weavetest pcl-unprint escputil printer_options

noinst_PROGRAMS = printdef

COMMON_SOURCE = \
	print-canon.c   	\
	print-dither.c		\
	print-escp2.c		\
	print-pcl.c		\
	print-ps.c      	\
	print-util.c		\
	print-weave.c		\
	print.h			\
	quickmatrix257.h	\
	ran.367.179.h

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

print-util.c: print-printers.c

print-printers.c: printdef printers.xml
	./printdef < printers.xml > print-printers.c

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
	$(addprefix Ghost/, $(GCOPY))

MAINTAINERCLEANFILES = configure Makefile.in aclocal.m4

INCLUDES = \
 -I$(top_srcdir)\
 $(GIMP_CFLAGS)\
 -I$(includedir)

AM_CPPFLAGS =\
 -DLOCALEDIR=\""$(localedir)"\"\
 @LP_DEF@\
 @LPSTAT_DEF@\
 @LPR_DEF@\
 @LPC_DEF@\
 @RELDATE_DEF@

print_LDADD = $(GIMP_LIBS)

GHOST_DIST= COPYING ChangeLog README contrib.mak.addon \
	gdevstp.c devs.mak.addon-5.10 \
	debian-patch debian-patch-stp $(GCOPY)

CUPS_DIST = CHANGES.txt LICENSE.txt Makefile.in README.txt configure \
		commandtoepson.c epson.c genppd.c rastertoprinter.c $(CUPSCOPY)

EXTRA_DIST=print-printers.c printers.xml printdefy.h parse-escp2 RELNOTES \
	README.dither quickmatrix257.h ran.367.179.h make-deb.sh README.escp2 \
	$(addprefix Ghost/, $(GHOST_DIST)) \
	$(addprefix cups/, $(CUPS_DIST)) \
	$(FORCE_ACLOCAL)

install-libexecPROGRAMS: $(libexec_PROGRAMS)
	@list='$(libexec_PROGRAMS)'; for p in $$list; do \
		if test -f $$p; then \
			echo $(GIMPTOOL) --install-admin-bin $$p; \
			$(GIMPTOOL) --install-admin-bin $$p; \
		else :; fi; \
	done

uninstall-libexecPROGRAMS: $(libexec_PROGRAMS)
	@list='$(libexec_PROGRAMS)'; for p in $$list; do \
		if test -f "$$p"; then \
			echo $(GIMPTOOL) --uninstall-admin-bin "$$p"; \
			$(GIMPTOOL) --uninstall-admin-bin "$$p"; \
		else :; fi; \
	done

.PHONY: files ghost ChangeLog force_aclocal

ChangeLog:
	cvs log | perl ./mkchlog > ChangeLog.tmp
	mv ChangeLog.tmp ChangeLog

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

GHCODE=-e 's/^\\\#include "\(.*\)\.h"$$/\\\#include "gdevstp-\1.h"/'

ghost: $(addprefix Ghost/, $(GCOPY))

Ghost/gdevstp-%.h: %.h Makefile
	sed -e 's/print-printers.c/gdevstp-printers.c/' $(GHCODE) $< > $@

Ghost/gdevstp-%: print-% Makefile
	sed -e 's/print-printers.c/gdevstp-printers.c/' $(GHCODE) $< > $@

CUPSCODE=-e 's/^\\\#include "\(.*\)\.h"$$/\\\#include "cups-\1.h"/'

cups: $(addprefix cups/, $(CUPSCOPY))

cups/cups-%.h: %.h Makefile
	sed -e 's/print-printers.c/cups-printers.c/' $(CUPSCODE) $< > $@

cups/cups-%: print-% Makefile
	sed -e 's/print-printers.c/cups-printers.c/' $(CUPSCODE) $< > $@

FORCE_ACLOCAL=PLEASE\ RUN\ make\ gdist\ rather\ than\ make\ dist

gdist: force_aclocal

force_aclocal:
	touch configure.in
	$(MAKE) dist FORCE_ACLOCAL=

print_gimp.h: Makefile
	touch print_gimp.h
