## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = print

bin_PROGRAMS = unprint weavetest canunprint

print_SOURCES = \
	print-canon.c   \
	print-dither.c	\
	print-escp2.c	\
	print-pcl.c	\
	print-ps.c	\
	print-util.c	\
	print.c		\
	print.h   

unprint_SOURCES = unprint.c

canunprint_SOURCES = canunprint.cpp

weavetest_SOURCES = weavetest.c

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"	\
	-Wall				\
	@LP_DEF@			\
	@LPSTAT_DEF@			\
	@LPR_DEF@			\
	@LPC_DEF@

print_LDADD = $(GIMP_LIBS)

.PHONY: files ghost

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

ghost: $(addprefix Ghost/gdevstp-, dither.c escp2.c util.c) Ghost/gdevstp-print.h

Ghost/gdevstp%: print% Makefile
	sed 's/print.h/gdevstp-print.h/' $< > $@

Ghost/gdevstp-print.h: print.h Makefile
	sed 's/print.h/gdevstp-print.h/' $< > $@
