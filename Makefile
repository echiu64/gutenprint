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
top_srcdir = ..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/Tools//
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
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

top_builddir = ..

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
CATALOGS =  cs.gmo da.gmo de.gmo fi.gmo fr.gmo hu.gmo it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo ru.gmo sv.gmo
CATOBJEXT = .gmo
CC = gcc
CFLAGS = -O6 -funroll-all-loops -mstack-align-double -march=pentiumpro -Wall
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
GIMP_CFLAGS = -I$topdir/../.. -I/mnt1/gnome.new/lib/glib/include -I/mnt1/gnome.new/include -I/usr/X11R6/include -I/mnt1/gnome.new/lib/glib/include -I/mnt1/gnome.new/include -O6 -funroll-all-loops -mstack-align-double -march=pentiumpro -Wall -Wno-parentheses -Wno-unused -Wno-uninitialized
GIMP_CFLAGS_NOUI = -I$topdir/../.. -I/mnt1/gnome.new/lib/glib/include -I/mnt1/gnome.new/include -I/usr/X11R6/include -I/mnt1/gnome.new/lib/glib/include -I/mnt1/gnome.new/include -O6 -funroll-all-loops -mstack-align-double -march=pentiumpro -Wall -Wno-parentheses -Wno-unused -Wno-uninitialized
GIMP_LIBS = -L$topdir/../../libgimp/.libs -L$dirprefix/../../libgimp -lgimp -L/mnt1/gnome.new/lib -lglib  -lgimpui
GIMP_LIBS_NOUI = -L$topdir/../../libgimp/.libs -L$dirprefix/../../libgimp -lgimp -L/mnt1/gnome.new/lib -lglib 
GIMP_MAJOR_VERSION = 1
GIMP_MICRO_VERSION = 10
GIMP_MINOR_VERSION = 1
GIMP_MODULES = modules
GIMP_MP_FLAGS = 
GIMP_MP_LIBS = 
GIMP_PERL = perl
GIMP_PLUGINS = plug-ins
GIMP_THREAD_FLAGS = 
GIMP_THREAD_LIBS = 
GIMP_VERSION = 1.1.10
GLIB_CFLAGS = -I/mnt1/gnome.new/lib/glib/include -I/mnt1/gnome.new/include
GLIB_LIBS = -L/mnt1/gnome.new/lib -lglib
GMOFILES =  cs.gmo da.gmo de.gmo fi.gmo fr.gmo hu.gmo it.gmo ja.gmo ko.gmo nl.gmo no.gmo pl.gmo ru.gmo sv.gmo
GMSGFMT = /usr/bin/msgfmt
GNOME_CONFIG = /mnt1/gnome.new/bin/gnome-config
GTKXMHTML_CFLAGS = -I/mnt1/gnome.new/include -DNEED_GNOMESUPPORT_H -I/mnt1/gnome.new/lib/gnome-libs/include -I/usr/X11R6/include -I/mnt1/gnome.new/lib/glib/include
GTKXMHTML_LIBS = -rdynamic -L/mnt1/gnome.new/lib -L/usr/X11R6/lib -lgtkxmhtml -lXpm -ljpeg -lpng -lz -lSM -lICE -lgtk -lgdk -lgmodule -lglib -ldl -lXext -lX11 -lm
GTK_CFLAGS = -I/usr/X11R6/include -I/mnt1/gnome.new/lib/glib/include -I/mnt1/gnome.new/include
GTK_CONFIG = /mnt1/gnome.new/bin/gtk-config
GTK_LIBS = -L/mnt1/gnome.new/lib -L/usr/X11R6/lib -lgtk -lgdk -rdynamic -lgmodule -lglib -ldl -lXext -lX11 -lm
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
LD = /usr/bin/ld
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
LT_CURRENT = 10
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
POFILES =  cs.po da.po de.po fi.po fr.po hu.po it.po ja.po ko.po nl.po no.po pl.po ru.po sv.po
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
VERSION = 1.1.10
WEBBROWSER = webbrowser
XJT = xjt
XPM = xpm
brushdata = 10x10square.gbr 10x10squareBlur.gbr 11circle.gbr 11fcircle.gbr 13circle.gbr 13fcircle.gbr 15circle.gbr 15fcircle.gbr 17circle.gbr 17fcircle.gbr 19circle.gbr 19fcircle.gbr 1circle.gbr 20x20square.gbr 20x20squareBlur.gbr 3circle.gbr 3fcircle.gbr 5circle.gbr 5fcircle.gbr 5x5square.gbr 5x5squareBlur.gbr 7circle.gbr 7fcircle.gbr 9circle.gbr 9fcircle.gbr DStar11.gbr DStar17.gbr DStar25.gbr callig1.gbr callig2.gbr callig3.gbr callig4.gbr confetti.gbr dunes.gbr galaxy.gbr galaxy_big.gbr galaxy_small.gbr pepper.gpb pixel.gbr thegimp.gbr vine.gih xcf.gbr
gimpdatadir = ${prefix}/Libraries/share/gimp
gimpdir = .gimp-1.1
gimpplugindir = ${exec_prefix}/Libraries///gimp/1.1
gradientdata = Abstract_1 Abstract_2 Abstract_3 Aneurism Blinds Blue_Green Browns Brushed_Aluminium Burning_Paper Burning_Transparency CD CD_Half Caribbean_Blues Coffee Cold_Steel Cold_Steel_2 Crown_molding Dark_1 Deep_Sea Default Flare_Glow_Angular_1 Flare_Glow_Radial_1 Flare_Glow_Radial_2 Flare_Glow_Radial_3 Flare_Glow_Radial_4 Flare_Radial_101 Flare_Radial_102 Flare_Radial_103 Flare_Rays_Radial_1 Flare_Rays_Radial_2 Flare_Rays_Size_1 Flare_Sizefac_101 Four_bars French_flag French_flag_smooth Full_saturation_spectrum_CCW Full_saturation_spectrum_CW German_flag German_flag_smooth Golden Greens Horizon_1 Horizon_2 Incandescent Land_1 Land_and_Sea Metallic_Something Mexican_flag Mexican_flag_smooth Nauseating_Headache Neon_Cyan Neon_Green Neon_Yellow Pastel_Rainbow Pastels Purples Radial_Eyeball_Blue Radial_Eyeball_Brown Radial_Eyeball_Green Radial_Glow_1 Radial_Rainbow_Hoop Romanian_flag Romanian_flag_smooth Rounded_edge Shadows_1 Shadows_2 Shadows_3 Skyline Skyline_polluted Square_Wood_Frame Sunrise Three_bars_sin Tropical_Colors Tube_Red Wood_1 Wood_2 Yellow_Contrast Yellow_Orange
l = 
localedir = ${prefix}/${DATADIRNAME}/locale
palettedata = Bears Bgold Blues Borders Browns_And_Yellows Caramel Cascade China Coldfire Cool_Colors Cranes Dark_pastels Default Ega Firecode Gold GrayViolet Grayblue Grays Greens Hilite Kahki Lights Muted Named_Colors News3 Op2 Paintjet Pastels Plasma Reds Reds_And_Purples Royal Topographic Visibone Visibone2 Volcano Warm_Colors Web
patterndata = 3dgreen.pat Craters.pat Moonfoot.pat amethyst.pat bark.pat blue.pat bluegrid.pat bluesquares.pat blueweb.pat brick.pat burlap.pat burlwood.pat choc_swirl.pat corkboard.pat cracked.pat crinklepaper.pat electric.pat fibers.pat granite1.pat ground1.pat ice.pat java.pat leather.pat leaves.pat leopard.pat lightning.pat marble1.pat marble2.pat marble3.pat nops.pat paper.pat parque1.pat parque2.pat parque3.pat pastel.pat pine.pat pink_marble.pat pool.pat qube1.pat rain.pat recessed.pat redcube.pat rock.pat sky.pat slate.pat sm_squares.pat starfield.pat stone33.pat terra.pat walnut.pat warning.pat wood1.pat wood2.pat wood3.pat wood4.pat wood5.pat
prefix = /usr/local
pyexecdir = 
pythondir = 

EXTRA_DIST =  	makefile.cygwin			makefile.msc			twain/README			twain/tw_dump.c			twain/tw_dump.h			twain/tw_func.c			twain/tw_func.h			twain/tw_sess.c			twain/tw_util.c			twain/tw_util.h			twain/twain.c			twain/twain.h			winsnap/resource.h		winsnap/select.cur		winsnap/small.ico		winsnap/winsnap.c		winsnap/winsnap.h		winsnap/winsnap.ico		winsnap/winsnap.rc

#pygimp = 
pygimp = 

SUBDIRS =  	libgck				megawidget			gpc				dbbrowser			script-fu			$(GIMP_PERL)			AlienMap			AlienMap2			FractalExplorer			Lighting			MapObject			bmp				borderaverage			faxg3				fits				flame				fp				gap				gdyntext			gfig			        gflare				gfli				gimpressionist			$(HELPBROWSER)			ifscompose			imagemap			maze				mosaic				pagecurl			print				$(pygimp)			rcm				sgi				sel2path			sinus				struc				unsharp				$(WEBBROWSER)			$(XJT)				common

mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP_ENV = --best
DIST_SUBDIRS =  libgck megawidget gpc dbbrowser script-fu perl \
AlienMap AlienMap2 FractalExplorer Lighting MapObject bmp borderaverage \
faxg3 fits flame fp gap gdyntext gfig gflare gfli gimpressionist \
helpbrowser ifscompose imagemap maze mosaic pagecurl print rcm sgi \
sel2path sinus struc unsharp webbrowser xjt common
all: all-redirect
.SUFFIXES:
$(srcdir)/Makefile.in: # Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu --include-deps plug-ins/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


# This directory's subdirectories are mostly independent; you can cd
# into them and run `make' without going through this Makefile.
# To change the values of `make' variables: instead of editing Makefiles,
# (1) if the variable is set in `config.status', edit `config.status'
#     (which will cause the Makefiles to be regenerated when you run `make');
# (2) otherwise, pass the desired values on the `make' command line.



all-recursive install-data-recursive install-exec-recursive \
installdirs-recursive install-recursive uninstall-recursive  \
check-recursive installcheck-recursive info-recursive dvi-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done; \
	if test "$$dot_seen" = "no"; then \
	  $(MAKE) $(AM_MAKEFLAGS) "$$target-am" || exit 1; \
	fi; test -z "$$fail"

mostlyclean-recursive clean-recursive distclean-recursive \
maintainer-clean-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	dot_seen=no; \
	rev=''; list='$(SUBDIRS)'; for subdir in $$list; do \
	  rev="$$subdir $$rev"; \
	  test "$$subdir" = "." && dot_seen=yes; \
	done; \
	test "$$dot_seen" = "no" && rev=". $$rev"; \
	target=`echo $@ | sed s/-recursive//`; \
	for subdir in $$rev; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
tags-recursive:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  test "$$subdir" = . || (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) tags); \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS: tags-recursive $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
   if test "$$subdir" = .; then :; else \
	    test -f $$subdir/TAGS && tags="$$tags -i $$here/$$subdir/TAGS"; \
   fi; \
	done; \
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

subdir = plug-ins

distdir: $(DISTFILES)
	$(mkinstalldirs) $(distdir)/twain $(distdir)/winsnap
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
	for subdir in $(DIST_SUBDIRS); do \
	  if test "$$subdir" = .; then :; else \
	    test -d $(distdir)/$$subdir \
	    || mkdir $(distdir)/$$subdir \
	    || exit 1; \
	    chmod 777 $(distdir)/$$subdir; \
	    (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) top_distdir=../$(top_distdir) distdir=../$(distdir)/$$subdir distdir) \
	      || exit 1; \
	  fi; \
	done
info-am:
info: info-recursive
dvi-am:
dvi: dvi-recursive
check-am: all-am
check: check-recursive
installcheck-am:
installcheck: installcheck-recursive
install-exec-am:
install-exec: install-exec-recursive

install-data-am:
install-data: install-data-recursive

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-recursive
uninstall-am:
uninstall: uninstall-recursive
all-am: Makefile
all-redirect: all-recursive
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs: installdirs-recursive
installdirs-am:


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-tags mostlyclean-generic

mostlyclean: mostlyclean-recursive

clean-am:  clean-tags clean-generic mostlyclean-am

clean: clean-recursive

distclean-am:  distclean-tags distclean-generic clean-am
	-rm -f libtool

distclean: distclean-recursive

maintainer-clean-am:  maintainer-clean-tags maintainer-clean-generic \
		distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-recursive

.PHONY: install-data-recursive uninstall-data-recursive \
install-exec-recursive uninstall-exec-recursive installdirs-recursive \
uninstalldirs-recursive all-recursive check-recursive \
installcheck-recursive info-recursive dvi-recursive \
mostlyclean-recursive distclean-recursive clean-recursive \
maintainer-clean-recursive tags tags-recursive mostlyclean-tags \
distclean-tags clean-tags maintainer-clean-tags distdir info-am info \
dvi-am dvi check check-am installcheck-am installcheck install-exec-am \
install-exec install-data-am install-data install-am install \
uninstall-am uninstall all-redirect all-am all installdirs-am \
installdirs mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


.PHONY: files

files:
	@files=`ls $(DISTFILES) 2> /dev/null`; for p in $$files; do \
	  echo $$p; \
	done
	@for subdir in $(SUBDIRS); do \
	  files=`cd $$subdir; $(MAKE) files | grep -v "make\[[1-9]\]"`; \
	  for file in $$files; do \
	    echo $$subdir/$$file; \
	  done; \
	done

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
