# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ../..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/Tools//
sbindir = ${exec_prefix}/sbin
datadir = ${prefix}/Libraries/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/Libraries//
infodir = ${prefix}/Library/info
mandir = ${prefix}/Library/man
includedir = ${prefix}/Headers//
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/gimp
pkglibdir = $(libdir)/gimp
pkgincludedir = $(includedir)/gimp

top_builddir = ../..

ACLOCAL = aclocal 
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = i686-pc-linux-gnu
host_triplet = i686-pc-linux-gnu
AA = 
AS = @AS@
CATALOGS =  cs.gmo da.gmo de.gmo fi.gmo fr.gmo hu.gmo it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo ru.gmo sk.gmo sv.gmo
CATOBJEXT = .gmo
CC = /opt/pgcc-2.95.1/bin/gcc
#CC = gcc
#CFLAGS = -pg -O6 -funroll-all-loops -Wall
CFLAGS = -O6 -funroll-all-loops -mstack-align-double -march=pentiumpro -Wall
#CFLAGS=-g
CPP = gcc -E
CPPFLAGS = 
DATADIRNAME = share
DLLTOOL = @DLLTOOL@
EMACS = /usr/bin/emacs
EXTENSIVE_TESTS = 0
GENCAT = 
GIMP = 
GIMPDOCS = 
GIMPINSTALL = 
GIMPTOOL = ../../gimptool
GIMP_CFLAGS = -I$topdir/../.. -I/usr/X11R6/include -I/usr/lib/glib/include -I/usr/X11R6/include -I/usr/lib/glib/include -O2 -Wall -Wno-parentheses -Wno-unused -Wno-uninitialized
GIMP_CFLAGS_NOUI = -I$topdir/../.. -I/usr/X11R6/include -I/usr/lib/glib/include -I/usr/X11R6/include -I/usr/lib/glib/include -O2 -Wall -Wno-parentheses -Wno-unused -Wno-uninitialized
GIMP_LIBS = -L$topdir/../../libgimp/.libs -L$dirprefix/../../libgimp -lgimp -L/usr/lib -lglib  -lgimpui
GIMP_LIBS_NOUI = -L$topdir/../../libgimp/.libs -L$dirprefix/../../libgimp -lgimp -L/usr/lib -lglib 
GIMP_MAJOR_VERSION = 1
GIMP_MICRO_VERSION = 15
GIMP_MINOR_VERSION = 1
GIMP_MODULES = modules
GIMP_MP_FLAGS = 
GIMP_MP_LIBS = 
GIMP_PERL = perl
GIMP_PLUGINS = plug-ins
GIMP_THREAD_FLAGS = 
GIMP_THREAD_LIBS = 
GIMP_VERSION = 1.1.15
GLIB_CFLAGS = -I/usr/X11R6/include -I/usr/lib/glib/include
GLIB_LIBS = -L/usr/lib -lglib
GMOFILES =  cs.gmo da.gmo de.gmo fi.gmo fr.gmo hu.gmo it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo ru.gmo sk.gmo sv.gmo
GMSGFMT = /usr/bin/msgfmt
GNOME_CONFIG = /opt/gnome/bin/gnome-config
GTKXMHTML_CFLAGS = -I/opt/gnome/include -DNEED_GNOMESUPPORT_H -I/opt/gnome/lib/gnome-libs/include -I/usr/X11R6/include -I/usr/lib/glib/include
GTKXMHTML_LIBS = -rdynamic -L/opt/gnome/lib -L/usr/lib -L/usr/X11R6/lib -lgtkxmhtml -lXpm -ljpeg -lpng -lz -lSM -lICE -lgtk -lgdk -lgmodule -lglib -ldl -lXext -lX11 -lm
GTK_CFLAGS = -I/usr/X11R6/include -I/usr/lib/glib/include
GTK_CONFIG = /usr/bin/gtk-config
GTK_LIBS = -L/usr/lib -L/usr/X11R6/lib -lgtk -lgdk -rdynamic -lgmodule -lglib -ldl -lXext -lX11 -lm
GT_NO = 
GT_YES = #YES#
HELPBROWSER = helpbrowser
INCLUDE_LOCALE_H = #include <locale.h>
INSTOBJEXT = .mo
INTLDEPS = 
INTLLIBS = 
INTLOBJS = 
IN_GIMP = 1
JPEG = jpeg
LD = /usr/i486-linux/bin/ld
LDFLAGS = 
LIBAA = 
LIBJPEG = -ljpeg
LIBMPEG = 
LIBPNG = -lpng -lz
LIBTIFF = -ltiff
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LIBUCB = 
LIBXMU = -lXmu -lXt -lSM -lICE
LIBXPM = -lXpm
LIBZ = -lz
LN_S = ln -s
LPC_COMMAND = /usr/sbin/lpc
LPC_DEF = -DLPC_COMMAND=\"/usr/sbin/lpc\"
LPR_COMMAND = /usr/bin/lpr
LPR_DEF = -DLPR_COMMAND=\"/usr/bin/lpr\"
LPSTAT_COMMAND = /usr/bin/lpstat
LPSTAT_DEF = -DLPSTAT_COMMAND=\"/usr/bin/lpstat\"
LP_COMMAND = /usr/bin/lp
LP_DEF = -DLP_COMMAND=\"/usr/bin/lp\"
LT_AGE = 0
LT_CURRENT = 15
LT_RELEASE = 1.1
LT_REVISION = 0
MAILER = -DMAILER=\"/usr/sbin/sendmail\"
MAINT = #
MAKEINFO = makeinfo
MKINSTALLDIRS = ./mkinstalldirs
MPEG = 
MSGFMT = /usr/bin/msgfmt
NM = /usr/bin/nm -B
OBJDUMP = @OBJDUMP@
PACKAGE = gimp
PERL = /usr/bin/perl
PNG = png
POFILES =  cs.po da.po de.po fi.po fr.po hu.po it.po ja.po ko.po nl.po no.po pl.po ru.po sk.po sv.po
POSUB = po
PSP = psp
PYGIMP_CFLAGS_NOUI = 
PYGIMP_LIBS_NOUI = 
PYTHON = 
PYTHON_CFLAGS = 
PYTHON_INCLUDES = 
PYTHON_LINK = 
RANLIB = ranlib
SENDMAIL = /usr/sbin/sendmail
SO = 
TIFF = tiff
USE_INCLUDED_LIBINTL = no
USE_NLS = yes
VERSION = 1.1.15
WEBBROWSER = webbrowser
XJT = xjt
XPM = xpm
brushdata = 10x10square.gbr 10x10squareBlur.gbr 11circle.gbr 11fcircle.gbr 13circle.gbr 13fcircle.gbr 15circle.gbr 15fcircle.gbr 17circle.gbr 17fcircle.gbr 19circle.gbr 19fcircle.gbr 1circle.gbr 20x20square.gbr 20x20squareBlur.gbr 3circle.gbr 3fcircle.gbr 5circle.gbr 5fcircle.gbr 5x5square.gbr 5x5squareBlur.gbr 7circle.gbr 7fcircle.gbr 9circle.gbr 9fcircle.gbr CVS DStar11.gbr DStar17.gbr DStar25.gbr callig1.gbr callig2.gbr callig3.gbr callig4.gbr confetti.gbr dunes.gbr galaxy.gbr galaxy_big.gbr galaxy_small.gbr pepper.gpb pixel.gbr thegimp.gbr vine.gih xcf.gbr
gimpdatadir = ${prefix}/Libraries/share/gimp
gimpdir = .gimp-1.1
gimpplugindir = ${exec_prefix}/Libraries///gimp/1.1
gradientdata = Abstract_1 Abstract_2 Abstract_3 Aneurism Blinds Blue_Green Browns Brushed_Aluminium Burning_Paper Burning_Transparency CD CD_Half CVS Caribbean_Blues Coffee Cold_Steel Cold_Steel_2 Crown_molding Dark_1 Deep_Sea Default Flare_Glow_Angular_1 Flare_Glow_Radial_1 Flare_Glow_Radial_2 Flare_Glow_Radial_3 Flare_Glow_Radial_4 Flare_Radial_101 Flare_Radial_102 Flare_Radial_103 Flare_Rays_Radial_1 Flare_Rays_Radial_2 Flare_Rays_Size_1 Flare_Sizefac_101 Four_bars French_flag French_flag_smooth Full_saturation_spectrum_CCW Full_saturation_spectrum_CW German_flag German_flag_smooth Golden Greens Horizon_1 Horizon_2 Incandescent Land_1 Land_and_Sea Metallic_Something Mexican_flag Mexican_flag_smooth Nauseating_Headache Neon_Cyan Neon_Green Neon_Yellow Pastel_Rainbow Pastels Purples Radial_Eyeball_Blue Radial_Eyeball_Brown Radial_Eyeball_Green Radial_Glow_1 Radial_Rainbow_Hoop Romanian_flag Romanian_flag_smooth Rounded_edge Shadows_1 Shadows_2 Shadows_3 Skyline Skyline_polluted Square_Wood_Frame Sunrise Three_bars_sin Tropical_Colors Tube_Red Wood_1 Wood_2 Yellow_Contrast Yellow_Orange
l = 
localedir = ${prefix}/${DATADIRNAME}/locale
palettedata = Bears Bgold Blues Borders Browns_And_Yellows CVS Caramel Cascade China Coldfire Cool_Colors Cranes Dark_pastels Default Ega Firecode Gold GrayViolet Grayblue Grays Greens Hilite Kahki Lights Muted Named_Colors News3 Op2 Paintjet Pastels Plasma Reds Reds_And_Purples Royal Topographic Visibone Visibone2 Volcano Warm_Colors Web
patterndata = 3dgreen.pat CVS Craters.pat Moonfoot.pat amethyst.pat bark.pat blue.pat bluegrid.pat bluesquares.pat blueweb.pat brick.pat burlap.pat burlwood.pat choc_swirl.pat corkboard.pat cracked.pat crinklepaper.pat electric.pat fibers.pat granite1.pat ground1.pat ice.pat java.pat leather.pat leaves.pat leopard.pat lightning.pat marble1.pat marble2.pat marble3.pat nops.pat paper.pat parque1.pat parque2.pat parque3.pat pastel.pat pine.pat pink_marble.pat pool.pat qube1.pat rain.pat recessed.pat redcube.pat rock.pat sky.pat slate.pat sm_squares.pat starfield.pat stone33.pat terra.pat walnut.pat warning.pat wood1.pat wood2.pat wood3.pat wood4.pat wood5.pat
prefix = /usr/local
pyexecdir = 
pythondir = 

libexecdir = $(gimpplugindir)/plug-ins

libexec_PROGRAMS = print

print_SOURCES =  	print-escp2.c		print-pcl.c		print-ps.c		print-util.c		print.c			print.h


INCLUDES =  	-I$(top_srcdir)				$(GTK_CFLAGS)				-I$(includedir)


AM_CPPFLAGS =          -DLOCALEDIR=\""$(localedir)"\"		-DLP_COMMAND=\"/usr/bin/lp\"				-DLPSTAT_COMMAND=\"/usr/bin/lpstat\"				-DLPR_COMMAND=\"/usr/bin/lpr\"				-DLPC_COMMAND=\"/usr/sbin/lpc\"


LDADD =  	$(top_builddir)/libgimp/libgimp.la		$(top_builddir)/libgimp/libgimpui.la		$(GTK_LIBS)					$(INTLLIBS)

mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../../config.h
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(libexec_PROGRAMS)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I../..
LIBS = 
print_OBJECTS =  print-escp2.o print-pcl.o print-ps.o print-util.o \
print.o
print_LDADD = $(LDADD)
print_DEPENDENCIES =  $(top_builddir)/libgimp/libgimp.la \
$(top_builddir)/libgimp/libgimpui.la
print_LDFLAGS = 
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP_ENV = --best
SOURCES = $(print_SOURCES)
OBJECTS = $(print_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .lo .o .s
$(srcdir)/Makefile.in: # Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu --include-deps plug-ins/print/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-libexecPROGRAMS:

clean-libexecPROGRAMS:
	-test -z "$(libexec_PROGRAMS)" || rm -f $(libexec_PROGRAMS)

distclean-libexecPROGRAMS:

maintainer-clean-libexecPROGRAMS:

install-libexecPROGRAMS: $(libexec_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libexecdir)
	@list='$(libexec_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(libexecdir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(libexecdir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done

uninstall-libexecPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(libexec_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(libexecdir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done

.c.o:
	$(COMPILE) -c $<

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

.c.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

.s.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

.S.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs _libs

distclean-libtool:

maintainer-clean-libtool:

print: $(print_OBJECTS) $(print_DEPENDENCIES)
	@rm -f print
	$(LINK) $(print_LDFLAGS) $(print_OBJECTS) $(print_LDADD) $(LIBS)

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = plug-ins/print

distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$d/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
print-escp2.o: print-escp2.c print.h ../../config.h ../../libgimp/gimp.h \
	../../libgimp/gimpenums.h ../../libgimp/gimpfeatures.h \
	../../libgimp/gimpenv.h ../../libgimp/gimpmath.h \
	../../libgimp/parasite.h ../../libgimp/parasiteF.h \
	../../libgimp/parasiteP.h ../../libgimp/gimpunit.h \
	../../libgimp/gimpcompat.h ../../libgimp/stdplugins-intl.h \
	../../libgimp/gimpintl.h
print-pcl.o: print-pcl.c print.h ../../config.h ../../libgimp/gimp.h \
	../../libgimp/gimpenums.h ../../libgimp/gimpfeatures.h \
	../../libgimp/gimpenv.h ../../libgimp/gimpmath.h \
	../../libgimp/parasite.h ../../libgimp/parasiteF.h \
	../../libgimp/parasiteP.h ../../libgimp/gimpunit.h \
	../../libgimp/gimpcompat.h ../../libgimp/stdplugins-intl.h \
	../../libgimp/gimpintl.h
print-ps.o: print-ps.c print.h ../../config.h ../../libgimp/gimp.h \
	../../libgimp/gimpenums.h ../../libgimp/gimpfeatures.h \
	../../libgimp/gimpenv.h ../../libgimp/gimpmath.h \
	../../libgimp/parasite.h ../../libgimp/parasiteF.h \
	../../libgimp/parasiteP.h ../../libgimp/gimpunit.h \
	../../libgimp/gimpcompat.h ../../libgimp/stdplugins-intl.h \
	../../libgimp/gimpintl.h
print-util.o: print-util.c print.h ../../config.h ../../libgimp/gimp.h \
	../../libgimp/gimpenums.h ../../libgimp/gimpfeatures.h \
	../../libgimp/gimpenv.h ../../libgimp/gimpmath.h \
	../../libgimp/parasite.h ../../libgimp/parasiteF.h \
	../../libgimp/parasiteP.h ../../libgimp/gimpunit.h \
	../../libgimp/gimpcompat.h
print.o: print.c print.h ../../config.h ../../libgimp/gimp.h \
	../../libgimp/gimpenums.h ../../libgimp/gimpfeatures.h \
	../../libgimp/gimpenv.h ../../libgimp/gimpmath.h \
	../../libgimp/parasite.h ../../libgimp/parasiteF.h \
	../../libgimp/parasiteP.h ../../libgimp/gimpunit.h \
	../../libgimp/gimpcompat.h ../../libgimp/gimpui.h \
	../../libgimp/gimpchainbutton.h ../../libgimp/gimpcolorbutton.h \
	../../libgimp/gimpexport.h ../../libgimp/gimpfileselection.h \
	../../libgimp/gimpmenu.h ../../libgimp/gimppatheditor.h \
	../../libgimp/gimpsizeentry.h ../../libgimp/gimpunitmenu.h \
	../../libgimp/stdplugins-intl.h ../../libgimp/gimpintl.h

info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am: install-libexecPROGRAMS
install-exec: install-exec-am

install-data-am:
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am: uninstall-libexecPROGRAMS
uninstall: uninstall-am
all-am: Makefile $(PROGRAMS)
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(libexecdir)


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-libexecPROGRAMS mostlyclean-compile \
		mostlyclean-libtool mostlyclean-tags \
		mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-libexecPROGRAMS clean-compile clean-libtool clean-tags \
		clean-generic mostlyclean-am

clean: clean-am

distclean-am:  distclean-libexecPROGRAMS distclean-compile \
		distclean-libtool distclean-tags distclean-generic \
		clean-am
	-rm -f libtool

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-libexecPROGRAMS \
		maintainer-clean-compile maintainer-clean-libtool \
		maintainer-clean-tags maintainer-clean-generic \
		distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: mostlyclean-libexecPROGRAMS distclean-libexecPROGRAMS \
clean-libexecPROGRAMS maintainer-clean-libexecPROGRAMS \
uninstall-libexecPROGRAMS install-libexecPROGRAMS mostlyclean-compile \
distclean-compile clean-compile maintainer-clean-compile \
mostlyclean-libtool distclean-libtool clean-libtool \
maintainer-clean-libtool tags mostlyclean-tags distclean-tags \
clean-tags maintainer-clean-tags distdir info-am info dvi-am dvi check \
check-am installcheck-am installcheck install-exec-am install-exec \
install-data-am install-data install-am install uninstall-am uninstall \
all-redirect all-am all installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
