## Process this file with automake to produce Makefile.in

libexecdir = $(gimptool_prefix)/plug-ins

libexec_PROGRAMS = print

bin_PROGRAMS = unprint escp2-weavetest canon-unprint pcl-unprint

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
canon_unprint_SOURCES = canon-unprint.cpp
escp2_weavetest_SOURCES = escp2-weavetest.c
pcl_unprint_SOURCES = pcl-unprint.c

INCLUDES = \
	-I$(top_srcdir)			\
	$(GIMP_CFLAGS)			\
	-I$(includedir)

AM_CPPFLAGS = \
        -DLOCALEDIR=\""$(localedir)"\"	\
	-Wall				\
	@LP_DEF@			\
	@LPSTAT_DEF@			\
	@LPR_DEF@			\
	@LPC_DEF@

print_LDADD = $(GIMP_LIBS)

EXTRA_DIST=Ghost/COPYING Ghost/ChangeLog Ghost/README Ghost/contrib.mak.addon \
	Ghost/gdevstp-dither.c Ghost/gdevstp-escp2.c Ghost/gdevstp-print.h \
	Ghost/gdevstp-util.c Ghost/gdevstp.c Ghost/gdevstp.h

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

.PHONY: files ghost

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

ghost: $(addprefix Ghost/gdevstp-, dither.c escp2.c util.c) Ghost/gdevstp-print.h

Ghost/gdevstp-%: print-% Makefile
	sed 's/print.h/gdevstp-print.h/' $< > $@

Ghost/gdevstp-print.h: print.h Makefile
	sed 's/print.h/gdevstp-print.h/' $< > $@
