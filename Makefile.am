## Process this file with automake to produce Makefile.in

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = print

bin_PROGRAMS = unprint

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

INCLUDES = \
	-I$(top_srcdir)			\
	$(GTK_CFLAGS)			\
	-I$(includedir)

#        -I@GLIBINCLUDE@

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"	\
	@LP_DEF@			\
	@LPSTAT_DEF@			\
	@LPR_DEF@			\
	@LPC_DEF@

print_LDADD = $(GTK_LIBS) $(GIMP_LIBS)

#	$(top_builddir)/libgimp/libgimp.la	\
#	$(top_builddir)/libgimp/libgimpui.la	\
#	$(INTLLIBS)

.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
