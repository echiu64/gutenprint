## Process this file with automake to produce Makefile.in

libexecdir = $(gimptool_prefix)/plug-ins

libexec_PROGRAMS = print

bin_PROGRAMS = unprint escp2-weavetest pcl-unprint escputil

noinst_PROGRAMS = printdef

print_SOURCES = \
	print-canon.c   	\
	print-dither.c		\
	print-escp2.c		\
	print-pcl.c		\
	print-ps.c      	\
	print-util.c		\
	print-image-gimp.c	\
	print-weave.c		\
	print-intl.h		\
	print.c			\
	print.h			\
	gimp_color_window.c	\
	gimp_main_window.c	\
        gtk_color_window.c	\
	gtk_main_window.c	\
	print_gimp.h

print-util.c: print-printers.c

print-printers.c: printdef printers.xml
	./printdef < printers.xml > print-printers.c

printdef_SOURCES = printdefl.l printdefy.y printdef.h

printdefl.o: printdefy.o

LFLAGS = -i
YFLAGS = -d

unprint_SOURCES = unprint.c
escp2_weavetest_SOURCES = escp2-weavetest.c
pcl_unprint_SOURCES = pcl-unprint.c

CLEANFILES = printdefl.c printdefy.c printdefy.h print-printers.c \
	$(addprefix Ghost/gdevstp-, $(GCOPY))

MAINTAINERCLEANFILES = configure Makefile.in aclocal.m4

INCLUDES = \
	-I$(top_srcdir)			\
	$(GIMP_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS =					\
        -DLOCALEDIR=\""$(localedir)"\"		\
	-Wall -Wcast-align -Wstrict-prototypes	\
	@LP_DEF@				\
	@LPSTAT_DEF@				\
	@LPR_DEF@				\
	@LPC_DEF@				\
	@RELDATE_DEF@

print_LDADD = $(GIMP_LIBS)

GCOPY=  dither.c escp2.c pcl.c ps.c canon.c util.c printers.c weave.c \
	print.h quickmatrix257.h ran.367.179.h
GHOST_DIST= COPYING ChangeLog README contrib.mak.addon \
	gdevstp.c gdevstp.h gs-stp.pdq com-stp.ppd devs.mak.addon-5.10 \
	debian-patch debian-patch-stp $(addprefix gdevstp-, $(GCOPY))

EXTRA_DIST=print-printers.c printers.xml printdefy.h parse-escp2 RELNOTES \
	README.dither quickmatrix257.h ran.367.179.h make-deb.sh README.escp2 \
	$(addprefix Ghost/, $(GHOST_DIST)) \
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

#ChangeLog:
#	cvs log | perl ./mkchlog > ChangeLog

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

GHCODE=-e 's/^\\\#include "\(.*\)\.h"$$/\\\#include "gdevstp-\1.h"/'

ghost: $(addprefix Ghost/gdevstp-, $(GCOPY))

Ghost/gdevstp-%.h: %.h Makefile
	sed -e 's/print-printers.c/gdevstp-printers.c/' $(GHCODE) $< > $@

Ghost/gdevstp-%: print-% Makefile
	sed -e 's/print-printers.c/gdevstp-printers.c/' $(GHCODE) $< > $@

FORCE_ACLOCAL=PLEASE\ RUN\ make\ gdist\ rather\ than\ make\ dist

echo:
gdist: force_aclocal

force_aclocal:
	touch configure.in
	$(MAKE) dist FORCE_ACLOCAL=

print_gimp.h: Makefile
	touch print_gimp.h
