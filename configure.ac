dnl Copyright (C) 2001-2004 Roger Leigh
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <https://www.gnu.org/licenses/>.

dnl Process this file with autoconf to produce a configure script.

dnl MAKING RELEASES (a step-by-step guide!)
dnl ===============
dnl
dnl Since the last release:
dnl 1. if only source code (not the interface) has changed, set
dnl      GUTENPRINT_MICRO_VERSION += 1;
dnl 2. if any interfaces have been added or changed *compatibly*
dnl      GUTENPRINT_CURRENT_INTERFACE (GCI) += 1;
dnl      GUTENPRINT_BINARY_AGE (GBA) += 1;
dnl 3. if interfaces have been deleted or changed *incompatibly*, set
dnl    	 GCI = GBA + 1;
dnl      GBA = 0;
dnl
dnl (Note that "added", "changed", and "deleted" refer to *external*
dnl interfaces, that is, interfaces that start with stp_ and are in any
dnl of the header files in include/gutenprint.  Internal interfaces
dnl that are prefixed with stpi_ and are only listed in internal headers
dnl in src/main do not need to be considered, since no external program
dnl should rely on them.  "Change" refers to more than just function
dnl signature; things like changes to enumerated types or changes in
dnl behavior with the same function arguments are also considered changes)
dnl
dnl The .soname version is (GCI - GBA).GBA
dnl Example: if GCI is 3 and GBA is 1, the version is 2.1.  To get 2.2,
dnl GCI would be incremented to 4 and GBA to 2.
dnl
dnl *NOTE*: due to an error preparing 5.2.8, we must skip over major version of
dnl 3 (that is, GCI - GBA) and go directly from 2 to 4.
dnl
dnl For example:
dnl 	 GCI=3, GBA=1 => .soname = libgutenprint.so.2.1
dnl 	 GCI=4, GBA=2 => .soname = libgutenprint.so.2.2 (5.2.9)
dnl 	 GCI=5, GBA=3 => .soname = libgutenprint.so.2.3 (5.2.10)
dnl          (and so on for 5.2 releases, which must remain compatible)
dnl 	 GCI=4, GBA=0 => .soname = libgutenprint.so.4.0
dnl 	 GCI=5, GBA=1 => .soname = libgutenprint.so.4.1
dnl 	 GCI=6, GBA=2 => .soname = libgutenprint.so.4.2
dnl          (for Gutenprint 5.3 or 5.4 or 6.0 or whatever)
dnl 	 GCI=5, GBA=0 => .soname = libgutenprint.so.5.0
dnl 	 GCI=6, GBA=1 => .soname = libgutenprint.so.5.1
dnl 	 GCI=7, GBA=2 => .soname = libgutenprint.so.5.2
dnl          (for the next major/minor release beyond that)
dnl
dnl Typically, programs will require a certain major number to be matched
dnl and a minimum minor number.  Causing the major number to be incremented
dnl will result in programs linked against libgutenprint.so.2 to be treated
dnl as incompatible with libgutenprint.so.3.
dnl
dnl .soname versions can have a third component, which is set from
dnl GUTENPRINT_INTERFACE_AGE.  This resulted in problems on OS X and
dnl we have elected to use only two components in the .soname.
dnl
dnl Similar principles apply to libgutenprintui2, but that interface is
dnl still at 1.0.
dnl
dnl For more detailed information, see the libtool info documentation.
dnl
pushdef([GUTENPRINT_NAME],              [gutenprint])
pushdef([GUTENPRINT_MAJOR_VERSION],     [5])
pushdef([GUTENPRINT_MINOR_VERSION],     [3])
pushdef([GUTENPRINT_MICRO_VERSION],     [4])
dnl Append snapshot ID if STP_BUILD_SNAPSHOT is set
pushdef([GUTENPRINT_EXTRA_VERSION],     [[]m4_esyscmd_s(scripts/snapstamp)])
pushdef([GUTENPRINT_CURRENT_INTERFACE], [14])
pushdef([GUTENPRINT_BINARY_AGE],        [5])
pushdef([GUTENPRINTUI2_CURRENT_INTERFACE], [7])
pushdef([GUTENPRINTUI2_BINARY_AGE],        [5])
pushdef([GUTENPRINT_VERSION], GUTENPRINT_MAJOR_VERSION.GUTENPRINT_MINOR_VERSION.GUTENPRINT_MICRO_VERSION[]GUTENPRINT_EXTRA_VERSION)

AC_INIT([GUTENPRINT_NAME],
	[GUTENPRINT_VERSION],
	[gimp-print-devel@lists.sourceforge.net],
	[GUTENPRINT_NAME],
	[https://gimp-print.sourceforge.net])
AC_PREREQ([2.69])
AC_REVISION($Revision$)
AC_CONFIG_SRCDIR(src/main/print-vars.c)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR(scripts)
STP_INIT
AC_COPYRIGHT(
[
Parts of this configure script come from the 'configure.ac' file, and
are not based on freely-redistributable m4 macros; you can redistribute
them and/or modify them under the terms of the GNU General Public
as published by the Free Software Foundation; either version 2, or (at
your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
])

[GUTENPRINT_MAJOR_VERSION]=GUTENPRINT_MAJOR_VERSION
[GUTENPRINT_MINOR_VERSION]=GUTENPRINT_MINOR_VERSION
[GUTENPRINT_MICRO_VERSION]=GUTENPRINT_MICRO_VERSION
[GUTENPRINT_EXTRA_VERSION]=GUTENPRINT_EXTRA_VERSION
[GUTENPRINT_CURRENT_INTERFACE]=GUTENPRINT_CURRENT_INTERFACE
[GUTENPRINT_INTERFACE_AGE]=0
[GUTENPRINT_BINARY_AGE]=GUTENPRINT_BINARY_AGE
[GUTENPRINT_VERSION]=GUTENPRINT_VERSION
[GUTENPRINTUI2_CURRENT_INTERFACE]=GUTENPRINTUI2_CURRENT_INTERFACE
[GUTENPRINTUI2_INTERFACE_AGE]=0
[GUTENPRINTUI2_BINARY_AGE]=GUTENPRINTUI2_BINARY_AGE
popdef([GUTENPRINT_MAJOR_VERSION])
popdef([GUTENPRINT_MINOR_VERSION])
popdef([GUTENPRINT_MICRO_VERSION])
popdef([GUTENPRINT_EXTRA_VERSION])
popdef([GUTENPRINT_CURRENT_INTERFACE])
popdef([GUTENPRINT_BINARY_AGE])
popdef([GUTENPRINTUI2_CURRENT_INTERFACE])
popdef([GUTENPRINTUI2_BINARY_AGE])

configure_args=

for arg in "$@" ; do
    if test -n "$configure_args" ; then
        configure_args="$configure_args "
    fi
    if echo "$arg" | grep -q '[\s]' ; then
        arg="'$arg'"
    fi
    configure_args="$configure_args$arg"
done

AC_SUBST([GUTENPRINT_MAJOR_VERSION])
AC_SUBST([GUTENPRINT_MINOR_VERSION])
AC_SUBST([GUTENPRINT_MICRO_VERSION])
AC_SUBST([GUTENPRINT_CURRENT_INTERFACE])
AC_SUBST([GUTENPRINT_INTERFACE_AGE])
AC_SUBST([GUTENPRINT_BINARY_AGE])
AC_SUBST([GUTENPRINT_VERSION])
AC_SUBST([GUTENPRINTUI2_CURRENT_INTERFACE])
AC_SUBST([GUTENPRINTUI2_INTERFACE_AGE])
AC_SUBST([GUTENPRINTUI2_BINARY_AGE])

dnl Make sure we inherit common compiler options from the environment...
CFLAGS="${CFLAGS:=}"
CPPFLAGS="${CPPFLAGS:=}"
CXXFLAGS="${CXXFLAGS:=}"
LDFLAGS="${LDFLAGS:=}"
LIBS="${LIBS:=}"

dnl Initialize automake stuff.
dnl We use our own NEWS file check, as explained in Makefile.am.
AM_INIT_AUTOMAKE(1.9 dist-xz dist-bzip2 no-dist-gzip tar-ustar gnu)
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
popdef([GUTENPRINT_NAME])
popdef([GUTENPRINT_VERSION])


dnl In the following script, there are a the following variants
dnl of cflags and variables
dnl
dnl *_CFLAGS:   cflags for compiling libraries and example progs
dnl *_LIBS:     libraries for linking programs. ONLY to be used
dnl             to generate ${gutenprint_libs}
dnl *_LIBDEPS:  libraries for linking libraries against
dnl *_libdeps:  library dependencies for pkg-config
dnl *_cflags:   cflags for pkg-config
dnl *_libs:     libs for pkg-config

GUTENPRINT_LIBS="-lgutenprint"
GUTENPRINTUI2_LIBS="-lgutenprintui2"
gutenprint_version=${GUTENPRINT_VERSION}
gutenprintui2_version=${GUTENPRINT_VERSION}

GUTENPRINTUI2_LIBDEPS="${GUTENPRINTUI2_LIBDEPS} \$(GUTENPRINT_LIBS)"

dnl Specify a configuration file.
AM_CONFIG_HEADER([config.h])

dnl Available languages (translations)
ALL_LINGUAS="ca cs da de el en_GB es fi fr gl hr hu it ja nb nl pl pt ru sk sl sv tr uk vi zh_CN zh_TW"
AC_SUBST(ALL_LINGUAS)

dnl Set up libtool scripts.
dnl Disable static library building to speed up the build
LT_INIT[(dlopen)]

# We need static libraries for CUPS genppd program for now, for bug 865265
# --rlk 20031223
AC_ENABLE_STATIC(yes)
#AC_DISABLE_SHARED
LT_INIT
AC_SUBST(LIBTOOL_DEPS)
dnl define HAVE_GNU_LD if GNU ld is present
if test "$ac_cv_prog_gnu_ld" = "yes" ; then
  AC_DEFINE(HAVE_GNU_LD,, [Define if GNU ld is present.])
fi

dnl Set up maintainer mode
AM_MAINTAINER_MODE
if test x${USE_MAINTAINER_MODE} = xyes ; then
  MAINTAINER_MODE="true"
  BUILD_TEST="yes"
  BUILD_DOC="yes"
  AC_DEFINE(HAVE_MAINTAINER_MODE,, [Define if maintainer-mode is to be used.])
fi

dnl Check canonical system/host (a kludge: replace me with something better).
if test x${host_os} = xlinux-gnu ; then
  OSTYPE="linux"
elif test "`echo ${host_os} | sed 's/^\(linux\).*$/\1/'`" = "linux" ; then
  OSTYPE="linux"
elif test "`echo ${host_os} | sed 's/^\(irix\).*$/\1/'`" = "irix" ; then
  OSTYPE="irix"
else
  OSTYPE="${host_os}"
fi
AC_DEFINE_UNQUOTED(OSTYPE, ["${OSTYPE}"], [The operating system to build for])


dnl Command line options.
AC_ARG_WITH([cups-config],
  [AS_HELP_STRING([--with-cups-config=path], [Specify path to cups-config executable.])],
  [with_cups_config="$withval"],
  [with_cups_config=system]
)
AS_IF([test "x$with_cups_config" != "xsystem"], [
    CUPS_CONFIG=$with_cups_config
  ], [
    AC_PATH_TOOL(CUPS_CONFIG, [cups-config])
    AS_IF([test -z "$CUPS_CONFIG"], [
      FOUND_CUPS_CONFIG=no
   ],
   [
      FOUND_CUPS_CONFIG=yes
   ])
])

missing_cups_error="

******* ERROR ******
You do not have the necessary software installed to build the CUPS driver.
This is most likely not what you want, as the resulting installation will
not be usable as part of a normal printing system.  If you are certain
that this is what you want, please run configure with --without-cups.
Otherwise, please read the README and install the necessary development
package, most likely named 'cups-devel', and re-run configure.

"

STP_ARG_WITH_DETAILED(
  [cups],
  [=PREFIX],
  [whether to build CUPS driver],
  [(default PREFIX=/usr)],
  [BUILD_CUPS],
  [(automatic)],
  [
    yes)
         if test "$FOUND_CUPS_CONFIG" = "yes" ; then
	   [BUILD_CUPS]="yes"
	   AC_MSG_RESULT([yes])
	   cups_prefix="/usr"
	 else
	   AC_MSG_ERROR([$missing_cups_error])
	 fi
	 ;;
    no) [BUILD_CUPS]="no" ; AC_MSG_RESULT([no]) ; cups_prefix="/usr" ;;
    *) if test -d ${withval} ; then
         [BUILD_CUPS]="yes"
	 AC_MSG_RESULT([yes (using specified prefix)])
         cups_prefix="${withval}"
       else
         AC_MSG_RESULT([unknown])
         AC_MSG_ERROR([${withval}: no such directory for --with-cups])
       fi ;;
  ],[
     if test "$FOUND_CUPS_CONFIG" = "yes" ; then
       cups_prefix="${prefix}"
       [BUILD_CUPS]="yes"
     else
       AC_MSG_ERROR([$missing_cups_error])
     fi
  ])

dnl GNU gettext checks.
AM_GNU_GETTEXT_VERSION([0.16])
AM_GNU_GETTEXT([external])

AM_CONDITIONAL(USE_NLS, test x${USE_NLS} = xyes)

STP_ARG_ENABLE([nls-macosx],
               [build CUPS PPD files],
               [USE_NLS_OSX],
               [(automatic)])

USE_LEVEL3_PS=no
BUILD_CUPS_PPDS="${BUILD_CUPS}"
BUILD_TRANSLATED_CUPS_PPDS=no
BUILD_CUPS_1_2="no"
if test -n "${CUPS_CONFIG}" -a "${BUILD_CUPS}" = "yes" ; then
  cups_version=`${CUPS_CONFIG} --version | sed -e '1,$s/svn$/.0/'`
dnl Quote to protect sed regexes
dnl Some relases may be 1.xrc1 or 1.xsvn. Grab only what is needed.
  [cups_major=`echo $cups_version | sed -e 's/\([0-9][0-9]*\).*/\1/'`]
  [cups_minor=`echo $cups_version | sed -e 's/\([0-9][0-9]*\).\([0-9][0-9]*\).*/\2/'`]
  [cups_point=`echo $cups_version | sed -e 's/\([0-9][0-9]*\).\([0-9][0-9]*\).\([0-9][0-9]*\).*/\3/'`]
dnl There was no CUPS 1.0.15 or the like
  if test "$cups_major" -gt 1 -o "$cups_minor" -gt 1 -o "$cups_point" -gt 14 ; then
    USE_LEVEL3_PS=yes
  fi
  if test "$cups_major" -gt 1 -o "$cups_minor" -gt 1 ; then
    BUILD_CUPS_PPDS="no"
    BUILD_CUPS_1_2="yes"
  fi
fi

STP_ARG_ENABLE([cups-ppds],
               [build CUPS PPD files],
               [BUILD_CUPS_PPDS],
               [(automatic)])

STP_ARG_ENABLE([cups-1_2-enhancements],
               [build CUPS 1.2 enhancements],
               [BUILD_CUPS_1_2],
               [(automatic)])

if test "$USE_NLS" != no -a "$BUILD_CUPS_PPDS" = yes ; then
   BUILD_TRANSLATED_CUPS_PPDS="yes"
fi

STP_ARG_ENABLE([translated-cups-ppds],
               [build translated CUPS PPD files],
               [BUILD_TRANSLATED_CUPS_PPDS],
               [(automatic)])

if test "$BUILD_CUPS_1_2" = yes -a "$BUILD_TRANSLATED_CUPS_PPDS" = yes ; then
   BUILD_GLOBALIZED_CUPS_PPDS=yes
else
   BUILD_GLOBALIZED_CUPS_PPDS=no
fi

STP_ARG_ENABLE([globalized-cups-ppds],
               [build globalized CUPS PPD files],
               [BUILD_GLOBALIZED_CUPS_PPDS],
               [(automatic)])

STP_ARG_ENABLE_DETAILED([simplified-cups-ppds],
			[],
			[build simplified CUPS PPD files],
			[(options: yes, no, only)],
			[BUILD_SIMPLIFIED_CUPS_PPDS],
			[no],
			[
    yes) [WHICH_PPDS]="-a"; BUILD_SIMPLIFIED_CUPS_PPDS=yes; AC_MSG_RESULT([yes]) ;;
    no) [WHICH_PPDS]=""; BUILD_SIMPLIFIED_CUPS_PPDS=no; AC_MSG_RESULT([no]) ;;
    only) [WHICH_PPDS]="-s"; BUILD_SIMPLIFIED_CUPS_PPDS=only; AC_MSG_RESULT([only]) ;;
    *) AC_MSG_ERROR([${enableval}: not a valid option ('yes', 'no', or 'only')]) ;;
				   ])

STP_ARG_ENABLE([cups-ppds-at-top-level],
	       [place CUPS PPD files at top level],
	       [CUPS_PPDS_AT_TOP_LEVEL],
	       [no])

STP_ARG_ENABLE([cups-level3-ppds],
               [generate PostScript level 3 PPD files],
               [USE_LEVEL3_PS],
               [(automatic)])

STATIC_GENPPD="$enable_static"
STP_ARG_ENABLE([static-genppd],
               [build a statically-linked cups-genppd and rastertogutenprint.  WARNING: Please read the README and NEWS (release notes) CAREFULLY before using --disable-static-genppd!],
	       [STATIC_GENPPD],
	       [(automatic)])

ENABLE_STATIC="$enable_static"
AC_DEFINE_UNQUOTED(ENABLE_STATIC, "${ENABLE_STATIC}", [Static libraries are built.])
AC_SUBST(ENABLE_STATIC)

ENABLE_SHARED="$enable_shared"
AC_DEFINE_UNQUOTED(ENABLE_SHARED, "${ENABLE_SHARED}", [Shared libraries are built.])
AC_SUBST(ENABLE_SHARED)

STP_ARG_WITH_DETAILED([modules],
                      [],
                      [enable loadable modules],
	              [(options: ltdl, dlopen, static)],
	              [WITH_MODULES],
	              [(automatic)],[
    yes) [WITH_MODULES]="yes";  AC_MSG_RESULT([yes]) ;;
    no) [WITH_MODULES]="no";  AC_MSG_RESULT([no]) ;;
    ltdl) [WITH_MODULES]="ltdl";  AC_MSG_RESULT([ltdl]) ;;
    dlopen) [WITH_MODULES]="dlopen";  AC_MSG_RESULT([dlopen]) ;;
    static) [WITH_MODULES]="static";  AC_MSG_RESULT([static]) ;;
    *) AC_MSG_ERROR([${withval}: not a valid module loader]) ;;
                      ],[
    # if static libraries are enabled, modules must be static
    if test x$enable_static = xyes; then
      [WITH_MODULES]="static"
    else
      [WITH_MODULES]="yes"]
    fi)

# Abort if a broken configuration was required
if test x$enable_static = xyes; then
  if test $WITH_MODULES = no; then
    :
  elif test $WITH_MODULES = static; then
    :
  else
    AC_MSG_ERROR([--disable-static must be used with --with-modules])
  fi
fi

STP_ARG_ENABLE([debug],
               [turn on debugging in build],
               [ENABLE_DEBUG],
               [no])

STP_ARG_ENABLE([profile],
               [turn on profiling in build],
               [ENABLE_PROFILE],
               [no])

STP_ARG_WITH_DETAILED([readline], ,
                      [use readline],
                      [(default tries -lncurses, -lcurses, -ltermcap)],
                      [USE_READLINE],
                      [yes],[
    yes) [USE_READLINE]="yes" ; AC_MSG_RESULT([yes]) ;;
    no) [USE_READLINE]="no" ; AC_MSG_RESULT([no]) ;;
    only) [USE_READLINE]="yes"
          [EXTRA_LIBREADLINE_DEPS]=" "
          AC_MSG_RESULT([yes (using only readline)]) ;;
    *) [USE_READLINE]="yes"
       [EXTRA_LIBREADLINE_DEPS]="${withval}"
       AC_MSG_RESULT([yes (using extra libraries ${withval})]) ;;
             ])
dnl if test -z "${[EXTRA_LIBREADLINE_DEPS]}" ; then
dnl   [EXTRA_LIBREADLINE_DEPS]=""
dnl fi

# libusb-1.0 (For CUPS backends, but only bother if CUPS is enabled)
AS_IF([test "x$BUILD_CUPS" = "xyes"], [
 PKG_CHECK_MODULES([LIBUSB], [libusb-1.0],
                   [HAVE_LIBUSB=yes
                    BUILD_LIBUSB_BACKENDS=yes],
                   [HAVE_LIBUSB=no
	            BUILD_LIBUSB_BACKENDS=no])
])

STP_ARG_ENABLE([user-install],
               [install GIMP plugin in home directory],
               [USER_INSTALL],
               [no])
if test x$USER_INSTALL = xyes ; then
  PLUG_IN_PATH="bin"
else
  PLUG_IN_PATH="admin-bin"
fi

AC_PATH_TOOL([GIMPTOOL2_CHECK], [gimptool-2.0 gimptool])
if test -z "${GIMPTOOL2_CHECK}" ; then
  BUILD_GIMP2="no"
else
  # Check we have version 2.0
  gimptool_check_version=`${GIMPTOOL2_CHECK} --version`
dnl Quote to protect sed regexes
  [gimptool_check_major=`echo "$gimptool_check_version" | sed -e 's/\([0-9][0-9]*\).\([0-9][0-9]*\).\([0-9][0-9]*\).*/\1/'`]
  [gimptool_check_minor=`echo "$gimptool_check_version" | sed -e 's/\([0-9][0-9]*\).\([0-9][0-9]*\).\([0-9][0-9]*\).*/\2/'`]
  if test "$gimptool_check_major" -eq 2; then
    BUILD_GIMP2="yes"
    if test "$gimptool_check_minor" -ge 4 ; then
      BUILD_GIMP2_AS_GUTENPRINT="yes"
    else
      BUILD_GIMP2_AS_GUTENPRINT="no"
    fi
  else
    BUILD_GIMP2="no"
    BUILD_GIMP2_AS_GUTENPRINT="no"
  fi
  if test x${BUILD_GIMP2} = xyes ; then
    AC_MSG_CHECKING([for GIMP 2.0 plug-in directory])
    GIMPTOOL2_OUTPUT=`$GIMPTOOL2_CHECK --dry-run --install-${PLUG_IN_PATH} print | tail -n 1`
    GIMP2_PLUGIN_DIR=`echo "$GIMPTOOL2_OUTPUT" | cut -d ' ' -f 3 | tr -d "\'"`
    AC_MSG_RESULT([$GIMP2_PLUGIN_DIR])
    if test "x$NO_PKGCONFIG_PATHS" = x; then
      gimp2_plug_indir="${GIMP2_PLUGIN_DIR}"
    else
      gimp2_plug_indir="${prefix}/lib/gimp/2.0/plug-ins"
    fi

  fi
fi

PKG_CHECK_MODULES([GTK2], [gtk+-2.0],
                  [HAVE_GTK2=yes
                   BUILD_LIBGUTENPRINTUI2=yes
                   GUTENPRINTUI2_LIBDEPS="${GUTENPRINTUI2_LIBDEPS} ${GTK2_LIBS}"],
                  [HAVE_GTK2=no
	           BUILD_LIBGUTENPRINTUI2=no])
# Require both gimptool and pkg-config to succeed
AS_IF([test "x$BUILD_GIMP2" = "xyes"], [
  PKG_CHECK_MODULES([GIMP2], [gimpui-2.0], [BUILD_GIMP2=yes], [BUILD_GIMP2=no])
])

STP_ARG_WITH([gimp2],
             [build GIMP 2.x Print plugin],
             [BUILD_GIMP2],
             [(automatic)])

STP_ARG_WITH([gimp2-as-gutenprint],
             [name GIMP 2.x Print plugin gutenprint (requires GIMP 2.2; normally used with GIMP 2.4 or above)],
             [BUILD_GIMP2_AS_GUTENPRINT],
             [(automatic)])

STP_ARG_ENABLE([libgutenprintui2],
               [build Gutenprint GTK+-2.0 user interface library],
               [BUILD_LIBGUTENPRINTUI2],
               [(automatic)])

STP_ARG_ENABLE([samples],
               [install sample images],
               [INSTALL_SAMPLES],
               [yes])

STP_ARG_ENABLE([escputil],
               [build escputil],
               [BUILD_ESCPUTIL],
               [yes])

STP_ARG_ENABLE([test],
               [build test programs],
               [BUILD_TEST],
               [yes])

STP_ARG_WITH([doc],
	     [build Gutenprint documentation],
	     [BUILD_DOC],
	     [yes])

if test "$BUILD_TEST" = "yes" ; then
  BUILD_TESTPATTERN="yes"
else
  BUILD_TESTPATTERN="no"
fi

STP_ARG_ENABLE([testpattern],
               [build testpattern generator],
               [BUILD_TESTPATTERN],
               [(automatic)])

dnl aclocal flags.
dnl AM_ACLOCAL_INCLUDE(m4 m4local)

dnl Checks for programs used in the build process
AM_ICONV_LINK
AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_STDC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_MKDIR_P
AC_PATH_PROG([TEST], [test])
AC_PATH_PROG([TRUE], [true])
AC_PATH_PROG([FIND], [find])
AC_PATH_PROG([PERL], [perl])
AC_PATH_PROG([GLIB_MKENUMS], [glib-mkenums])
AC_PATH_PROG([CONVERT], [convert])
AC_PATH_PROG([BASHREAL], [bash])
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${CONVERT} = x ; then
    AC_MSG_WARN([PostScript developer's guide cannot be regenerated!])
  fi
fi
AC_PATH_PROG(TEXI2HTML, texi2html)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${TEXI2HTML} = x ; then
    AC_MSG_WARN([HTML documentation cannot be regenerated!])
  fi
fi
AC_PATH_PROG(DVIPS, dvips)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DVIPS} = x ; then
    AC_MSG_WARN([PostScript documentation cannot be regenerated!])
  fi
fi
AC_PATH_PROG(DB2PDF, db2pdf)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DB2PDF} = x ; then
    AC_MSG_WARN([PDF developer's guide cannot be regenerated!])
  fi
fi
AC_PATH_PROGS(DVIPDF, dvipdf dvipdfm)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DVIPDF} = x ; then
    AC_MSG_WARN([PDF documentation cannot be regenerated!])
  fi
fi
AC_PATH_PROG(DB2PS, db2ps)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DB2PS} = x ; then
    AC_MSG_WARN([PostScript developer's guide cannot be regenerated!])
  fi
fi
AC_PATH_PROG(DB2HTML, db2html)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DB2HTML} = x ; then
    AC_MSG_WARN([HTML developer's guide cannot be regenerated!])
  fi
fi
AC_PATH_PROG(DOXYGEN, doxygen)
if test x${USE_MAINTAINER_MODE} = xyes ; then
  if test x${DOXYGEN} = x ; then
    AC_MSG_WARN([HTML developer's guide cannot be regenerated!])
  fi
fi

AC_PATH_PROG([XZ], [xz])
AC_PATH_PROG([GZIP], [gzip])
AC_PATH_PROG([BZIP2], [bzip2])
AC_PATH_PROG([LRZIP], [lrzip])
AC_PATH_PROG([RZIP], [rzip])
AC_PATH_PROG([ZPAQ], [zpaq])
AC_PATH_PROG([BASH], [bash])
if test x${ZPAQ} != x ; then
  COMPRESS='$(top_builddir)/scripts/wzpaq -z -q'
  CSUF=.zpaq
elif test x${LRZIP} != x ; then
  COMPRESS="${LRZIP} -z -q"
  CSUF=.lrz
elif test x${XZ} != x ; then
  COMPRESS="${XZ} -e -9"
  CSUF=.xz
elif test x${RZIP} != x ; then
  COMPRESS="${RZIP} -9"
  CSUF=.rz
elif test x${BZIP2} != x ; then
  COMPRESS="${BZIP2} -9"
  CSUF=.bz2
else
  COMPRESS="${GZIP} -9"
  CSUF=.gz
fi
AC_SUBST(COMPRESS)
AC_SUBST(CSUF)

GUTENPRINT_RELEASE_VERSION=${GUTENPRINT_MAJOR_VERSION}.${GUTENPRINT_MINOR_VERSION}
AH_TEMPLATE(GUTENPRINT_RELEASE_VERSION,, [Gutenprint release version])
AC_DEFINE_UNQUOTED(GUTENPRINT_RELEASE_VERSION, "${GUTENPRINT_RELEASE_VERSION}")
GUTENPRINT_BASE_VERSION=${GUTENPRINT_MAJOR_VERSION}.${GUTENPRINT_MINOR_VERSION}.${GUTENPRINT_MICRO_VERSION}
AH_TEMPLATE(GUTENPRINT_BASE_VERSION,, [Gutenprint base version])
AC_DEFINE_UNQUOTED(GUTENPRINT_BASE_VERSION, "${GUTENPRINT_BASE_VERSION}")

dnl Set PACKAGE_LOCALE_DIR in config.h
AH_TEMPLATE(PACKAGE_LOCALE_DIR,, [Package locale directory])
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, ["${ac_default_prefix}/share/locale"])
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, ["${prefix}/share/locale"])
fi

dnl Set PACKAGE_DATA_DIR in config.h.
AH_TEMPLATE(PACKAGE_DATA_DIR,, [Package data directory])
if test "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = "xNONE"; then
    PACKAGE_DATA_DIR="${ac_default_prefix}/share/${PACKAGE}"
  else
    PACKAGE_DATA_DIR="${prefix}/share/${PACKAGE}"
  fi
elif test "x${datadir}" = 'x${datarootdir}'; then
  if test "x${datarootdir}" = 'x${prefix}/share'; then
    if test "x${prefix}" = "xNONE"; then
      PACKAGE_DATA_DIR="${ac_default_prefix}/share/${PACKAGE}"
    else
      PACKAGE_DATA_DIR="${prefix}/share/${PACKAGE}"
    fi
  else
    PACKAGE_DATA_DIR="${datarootdir}/${PACKAGE}"
  fi
else
  PACKAGE_DATA_DIR="${datadir}/${PACKAGE}"
fi

AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${PACKAGE_DATA_DIR}")

dnl Set PACKAGE_LIB_DIR in config.h.
AH_TEMPLATE(PACKAGE_LIB_DIR,, [Package lib directory])
if test "x${libdir}" = 'x${exec_prefix}/lib'; then
  if test "x${exec_prefix}" = "xNONE"; then
    if test "x${prefix}" = "xNONE"; then
      PACKAGE_LIB_DIR="${ac_default_prefix}/lib/${PACKAGE}"
    else
      PACKAGE_LIB_DIR="${prefix}/lib/${PACKAGE}"
    fi
  else
    PACKAGE_LIB_DIR="${exec_prefix}/lib/${PACKAGE}"
  fi
else
  PACKAGE_LIB_DIR="${libdir}/${PACKAGE}"
fi

AC_DEFINE_UNQUOTED(PACKAGE_LIB_DIR, ["${PACKAGE_LIB_DIR}"])

dnl Set PACKAGE_BIN_DIR in config.h.
AH_TEMPLATE(PACKAGE_BIN_DIR,, [Package bin directory])
if test "x${bindir}" = 'x${exec_prefix}/bin'; then
  if test "x${exec_prefix}" = "xNONE"; then
    if test "x${prefix}" = "xNONE"; then
      PACKAGE_BIN_DIR="${ac_default_prefix}/bin"
    else
      PACKAGE_BIN_DIR="${prefix}/bin"
    fi
  else
    PACKAGE_BIN_DIR="${exec_prefix}/bin"
  fi
else
  PACKAGE_BIN_DIR="${bindir}"
fi

AC_DEFINE_UNQUOTED(PACKAGE_BIN_DIR, ["${PACKAGE_BIN_DIR}"])

AH_TEMPLATE(PKGXMLDATADIR,, [Package XML data directory])
PKGXMLDATADIR="${PACKAGE_DATA_DIR}/${GUTENPRINT_RELEASE_VERSION}/xml"
AC_DEFINE_UNQUOTED(PKGXMLDATADIR, ["$PKGXMLDATADIR"], )

AH_TEMPLATE(PKGMODULEDIR,, [Package module directory])
PKGMODULEDIR="${PACKAGE_LIB_DIR}/${GUTENPRINT_RELEASE_VERSION}/modules"
AC_DEFINE_UNQUOTED(PKGMODULEDIR, ["$PKGMODULEDIR"])

dnl Compiler flags
if test x$ac_compiler_gnu = "xyes"; then
  STP_ADD_COMPILER_ARGS([-Wall -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wwrite-strings -Werror-implicit-function-declaration -Winline -Wformat=2 -finline-limit=131072 -Wformat -Werror=format-security] -D_POSIX_C_SOURCE=200809L -std=c99,, [GNUCFLAGS])
  if test x${USE_MAINTAINER_MODE} = xyes ; then
    STP_ADD_COMPILER_ARGS([ -pedantic -Waggregate-return -Wcast-qual -Wshadow -Wredundant-decls],, [GNUCFLAGS])
  fi
  if test x$ENABLE_DEBUG = xyes ; then
    STP_ADD_COMPILER_ARG([-g])
  else
    STP_ADD_FIRST_COMPILER_ARG([-O3 -O2 -O1 -O])
  fi
else
  if test x$ENABLE_DEBUG = xyes ; then
    STP_ADD_COMPILER_ARG([-g])
  else
    STP_ADD_FIRST_COMPILER_ARG([-O])
  fi
fi
if test x$ENABLE_PROFILE = xyes ; then
  STP_ADD_COMPILER_ARG([-pg])
fi
AC_SUBST(GNUCFLAGS)

AH_TEMPLATE([HAVE_GCC_ATTRIBUTES],
            [Define to 1 if GCC special attributes are supported])
AC_MSG_CHECKING([if $CC supports __attribute__ syntax])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([typedef struct __attribute__((__packed__)) {
   unsigned char foo;
   char bar;
   long baz;
} test_struct_t;],
                                   [test_struct_t mystruct;])],
                  [AC_MSG_RESULT([yes])]
                   AC_DEFINE([HAVE_GCC_ATTRIBUTES], 1),
                  [AC_MSG_RESULT([no])])

AH_VERBATIM([HAVE_GCC_ATTRIBUTES_BOILERPLATE],
[#if !defined(HAVE_GCC_ATTRIBUTES) && !defined(__attribute__)
/* This should really be a C99 anonymous variadic macro. */
#define __attribute__(attr)
#endif])

dnl Checks for libraries.
dnl When checking readline, check using extra libraries first.
dnl We want to protect against the link somehow succeeding, but only
dnl failing at runtime, as seems to happen on some BSD systems.
AH_TEMPLATE(HAVE_LIBREADLINE, [Is libreadline present?])
if test "$USE_READLINE" = yes ; then
  if test "$EXTRA_LIBREADLINE_DEPS" = "" ; then
    unset ac_cv_lib_readline_readline
    AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
		 HAVE_LIBREADLINE=false, -lncurses)
    if test x${HAVE_LIBREADLINE} = xtrue ; then
      echo "  Using -lreadline -lncurses"
      EXTRA_LIBREADLINE_DEPS=-lncurses
      AC_DEFINE(HAVE_LIBREADLINE, [1])
    else
      unset ac_cv_lib_readline_readline
      AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
		   HAVE_LIBREADLINE=false, -lcurses)
      if test x${HAVE_LIBREADLINE} = xtrue ; then
	echo "  Using -lreadline -lcurses"
	EXTRA_LIBREADLINE_DEPS=-lcurses
	AC_DEFINE(HAVE_LIBREADLINE, [1])
      else
	unset ac_cv_lib_readline_readline
	AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
		     HAVE_LIBREADLINE=false, -ltermcap)
	if test x${HAVE_LIBREADLINE} = xtrue ; then
	  echo "  Using -lreadline -ltermcap"
	  EXTRA_LIBREADLINE_DEPS=-ltermcap
	  AC_DEFINE(HAVE_LIBREADLINE, [1])
	else
	  unset ac_cv_lib_readline_readline
	  AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
		       HAVE_LIBREADLINE=false)
	  if test x${HAVE_LIBREADLINE} = xtrue ; then
	    AC_DEFINE(HAVE_LIBREADLINE, [1])
	  else
	    AC_DEFINE(HAVE_LIBREADLINE, [0])
	  fi
	fi
      fi
    fi
  else
    unset ac_cv_lib_readline_readline
    AC_CHECK_LIB(readline, readline, HAVE_LIBREADLINE=true,
		 HAVE_LIBREADLINE=false, ${EXTRA_LIBREADLINE_DEPS})
    if test x${HAVE_LIBREADLINE} = xtrue ; then
      echo "  Using -lreadline ${EXTRA_LIBREADLINE_DEPS}"
      AC_DEFINE(HAVE_LIBREADLINE, [1])
    else
      AC_DEFINE(HAVE_LIBREADLINE, [0])
    fi
  fi
else
  HAVE_LIBREADLINE=false
  AC_DEFINE(HAVE_LIBREADLINE, [0])
fi

dnl libltdl, used for dynamic module loading
AC_CHECK_LIB(ltdl, lt_dlopen, [LTDL_LIBS="-lltdl"])

AC_CHECK_LIB(dl, dlopen, [DLOPEN_LIBS="-ldl"])

AC_CHECK_LIB(m,pow,
             GUTENPRINT_LIBDEPS="${GUTENPRINT_LIBDEPS} -lm"
             gutenprint_libdeps="${gutenprint_libdeps} -lm"
             GUTENPRINTUI2_LIBDEPS="${GUTENPRINTUI2_LIBDEPS} -lm"
             gutenprintui2_libdeps="${gutenprintui2_libdeps} -lm"
	     LIBM=-lm
)

dnl CUPS stuff
STP_CUPS_PATH
STP_CUPS_LIBS

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(getopt.h)
AC_CHECK_HEADER(readline/readline.h,
  HAVE_READLINE_READLINE_H=true,
  HAVE_READLINE_READLINE_H=false)
if test x${HAVE_READLINE_READLINE_H} = xtrue ; then
  AC_DEFINE(HAVE_READLINE_READLINE_H,, [Define if libreadline header is present.])
  if test x${HAVE_LIBREADLINE} = xtrue ; then
    LIBREADLINE_DEPS="-lreadline $EXTRA_LIBREADLINE_DEPS"
  fi
fi
AC_CHECK_HEADERS(dlfcn.h, [HAVE_DLFCN_H=true])
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(limits.h)
AC_CHECK_HEADERS(locale.h)
AC_CHECK_HEADERS(ltdl.h, [HAVE_LTDL_H=true])
AC_CHECK_HEADERS(stdarg.h stdlib.h string.h)
AC_CHECK_HEADERS(sys/time.h sys/types.h)
AC_CHECK_HEADERS(time.h)
AC_CHECK_HEADERS(unistd.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_CHECK_FUNCS([nanosleep poll usleep])
AC_CHECK_FUNCS([getopt_long])
AC_CHECK_FUNCS([setenv getuid waitpid])

dnl finite() is non-standard, isfinite() is ISO-standard, figure out
dnl which to use...
AC_SEARCH_LIBS(finite, m, CFLAGS="-Disfinite=finite $CFLAGS")

dnl Define what has to be built
AM_CONDITIONAL(BUILD_CUPS, test x${BUILD_CUPS} = xyes)

AM_CONDITIONAL(BUILD_CUPS_1_2, test x${BUILD_CUPS_1_2} = xyes)

AM_CONDITIONAL(BUILD_CUPS_PPDS, test x${BUILD_CUPS_PPDS} = xyes)

AM_CONDITIONAL(CUPS_PPDS_AT_TOP_LEVEL, test x${CUPS_PPDS_AT_TOP_LEVEL} = xyes)

AM_CONDITIONAL(BUILD_TRANSLATED_CUPS_PPDS, test x${BUILD_TRANSLATED_CUPS_PPDS} = xyes )

AM_CONDITIONAL(BUILD_GLOBALIZED_CUPS_PPDS, test x${BUILD_GLOBALIZED_CUPS_PPDS} = xyes )

AM_CONDITIONAL(BUILD_SIMPLIFIED_CUPS_PPDS, test x${BUILD_SIMPLIFIED_CUPS_PPDS} = xyes)

if test x${USE_LEVEL3_PS} = xno ; then
  CUPS_PPD_PS_LEVEL=2
else
  CUPS_PPD_PS_LEVEL=3
fi

AM_CONDITIONAL(USE_LEVEL3_PS, test x${USE_LEVEL3_PS} = xyes)

AM_CONDITIONAL(BUILD_ESCPUTIL, test x${BUILD_ESCPUTIL} = xyes)

AM_CONDITIONAL(BUILD_TEST, test x${BUILD_TEST} = xyes)

AM_CONDITIONAL(BUILD_TESTPATTERN, test x${BUILD_TESTPATTERN} = xyes)

AM_CONDITIONAL(BUILD_LIBGUTENPRINTUI2, test x${BUILD_LIBGUTENPRINTUI2} = xyes)

AM_CONDITIONAL(BUILD_GIMP2, test x${BUILD_GIMP2} = xyes)

AM_CONDITIONAL(BUILD_DOC, test x${BUILD_DOC} = xyes)

AM_CONDITIONAL(BUILD_GIMP2_AS_GUTENPRINT, test x${BUILD_GIMP2_AS_GUTENPRINT} = xyes)

AM_CONDITIONAL(INSTALL_SAMPLES, test x${INSTALL_SAMPLES} = xyes)

AM_CONDITIONAL(INSTALL_USER_GUIDE, test x${INSTALL_USER_GUIDE} = xyes)

AM_CONDITIONAL(BUILD_LIBUSB_BACKENDS, test x${BUILD_LIBUSB_BACKENDS} = xyes)


# loadable modules
# portable sh has no AND test...
if test -n "$HAVE_LTDL_H"; then
  if test -n "$LTDL_LIBS"; then
    LTDL_POSSIBLE="true"
  fi
fi
if test -n "$HAVE_DLFCN_H"; then
  if test -n "$DLOPEN_LIBS"; then
    DLOPEN_POSSIBLE="true"
  fi
fi

# define what the user chose to build, if possible.
# Prefer dlopen over ltdl.
if test x$WITH_MODULES = xyes; then
  if test x$DLOPEN_POSSIBLE = xtrue; then
    USE_DLOPEN="true"
  elif test x$LDTL_POSSIBLE = xtrue; then
    USE_LTDL="true"
  fi
elif test x$WITH_MODULES = xltdl; then
  if test x$LTDL_POSSIBLE = xtrue; then
    USE_LTDL="true"
  fi
elif test x$WITH_MODULES = xdlopen; then
  if test x$DLOPEN_POSSIBLE = xtrue; then
    USE_DLOPEN="true"
  fi
fi

# The current backends can use dlopen or libltdl
if test x$BUILD_LIBUSB_BACKENDS = xyes; then
  if test x$DLOPEN_POSSIBLE = xtrue; then
    USE_DLOPEN="true"
  elif test x$LDTL_POSSIBLE = xtrue; then
    USE_LTDL="true"
  fi
fi

# define what module system is to be used
AC_MSG_CHECKING([which module system will be used])
if test x$USE_LTDL = xtrue; then
  if test -z "$WITH_MODULES" -o x$WITH_MODULES = xno -o x$WITH_MODULES = xstatic ; then
    MODULE="false"
  else
    MODULE="true"
    AC_DEFINE(MODULE, "1", [Build a modular libgutenprint])
    GUTENPRINT_LIBDEPS="${GUTENPRINT_LIBDEPS} ${LTDL_LIBS}"
    gutenprint_libdeps="${gutenprint_libdeps} ${LTDL_LIBS}"
  fi
  LIBUSB_BACKEND_LIBDEPS="${LIBUSB_BACKEND_LIBDEPS} ${LTDL_LIBS}"
  AC_DEFINE(USE_LTDL, "1", [Use GNU libltdl as module loader])
  AC_MSG_RESULT([ltdl])
elif test x$USE_DLOPEN = xtrue; then
  if test -z "$WITH_MODULES" -o x$WITH_MODULES = xno -o x$WITH_MODULES = xstatic ; then
    MODULE="false"
  else
    MODULE="true"
    AC_DEFINE(MODULE, "1", [Build a modular libgutenprint])
    GUTENPRINT_LIBDEPS="${GUTENPRINT_LIBDEPS} ${DLOPEN_LIBS}"
    gutenprint_libdeps="${gutenprint_libdeps} ${DLOPEN_LIBS}"
  fi
  LIBUSB_BACKEND_LIBDEPS="${LIBUSB_BACKEND_LIBDEPS} ${DLOPEN_LIBS}"
  AC_DEFINE(USE_DLOPEN, "1", [Use libdl/dlopen as module loader])
  AC_MSG_RESULT([dlopen])
else
  MODULE="false"
  AC_MSG_RESULT([static])
fi
AM_CONDITIONAL(BUILD_MODULES, test x$MODULE = xtrue)

AM_CONDITIONAL(BUILD_GENPPD_STATIC, test x"$STATIC_GENPPD" = xyes)

dnl Define LTLIBOBJS
AC_CONFIG_COMMANDS_PRE(
[LTLIBOBJS=`echo "$LIB@&t@OBJS" | sed 's/\.o/.lo/g'`
AC_SUBST(LTLIBOBJS)
LTALLOCA=`echo "$ALLOCA" | sed 's/\.o/.lo/g'`
AC_SUBST(LTALLOCA)])

dnl Definitions
AC_DEFINE_UNQUOTED(CUPS_DATADIR, "${cups_conf_datadir}", [CUPS data directory.])

AH_TEMPLATE(CUPS_MODELDIR,, [CUPS PPD file directory])
if test "${CUPS_PPDS_AT_TOP_LEVEL}" = yes ; then
  AC_DEFINE_UNQUOTED(CUPS_MODELDIR, "${cups_conf_datadir}/model/")
else
  AC_DEFINE_UNQUOTED(CUPS_MODELDIR, "${cups_conf_datadir}/model/${PACKAGE}/${GUTENPRINT_RELEASE_VERSION}/")
fi

AC_ARG_WITH(cups_nickname, [  --with-cups-nickname="nickname suffix"
                          Suffix to add to PPD NickName, default=" - CUPS+Gutenprint v"],
  CUPS_PPD_NICKNAME_STRING="$withval",
  CUPS_PPD_NICKNAME_STRING=" - CUPS+Gutenprint v")

AH_TEMPLATE(CUPS_PPD_NICKNAME_STRING,, [CUPS PPD Nickname string])
AC_DEFINE_UNQUOTED(CUPS_PPD_NICKNAME_STRING, "$CUPS_PPD_NICKNAME_STRING")


AC_DEFINE_UNQUOTED(CUPS_PPD_PS_LEVEL, ${CUPS_PPD_PS_LEVEL}, [CUPS PPD PostScript level])
STP_RELEASE_DATE

MINIMAL_PRINTERS_TO_TEST="escp2-r1800 escp2-3880 \
	pcl-g_4 pcl-g_5c pcl-500 \
	mitsubishi-p95d shinko-chcs2145 kodak-1400 canon-cp910 mitsubishi-9800d \
	bjc-s200 bjc-PIXMA-Pro9000mk2 \
	datamax_oneil_I4212e \
	lexmark-z43"

AC_DEFINE_UNQUOTED(MINIMAL_PRINTERS_TO_TEST, ${MINIMAL_PRINTERS_TO_TEST},
					     [Printers to test in minimal run])

dnl Some versions of make get very grouchy about $(RM)
RM='rm'

PKGROOT=`pwd`

dnl Substitutions
AC_SUBST(prefix)
AC_SUBST(exec_prefix)
AC_SUBST(cups_prefix)
AC_SUBST(cups_exec_prefix)
AC_SUBST(cups_bindir)
AC_SUBST(cups_sbindir)
AC_SUBST(cups_conf_datadir)
AC_SUBST(cups_conf_serverbin)
AC_SUBST(cups_conf_serverroot)
AC_SUBST(CUPS_CFLAGS)
AC_SUBST(CUPS_LIBS)
AC_SUBST(CUPS_PPD_PS_LEVEL)
AC_SUBST(GENPPD_LIBS)
AC_SUBST(RM)
AC_SUBST(GIMP2_CFLAGS)
AC_SUBST(GIMP2_LIBS)
AC_SUBST(gimp2_plug_indir)
AC_SUBST(GUTENPRINT_CFLAGS)
AC_SUBST(GUTENPRINT_LIBS)
AC_SUBST(GUTENPRINT_LIBDEPS)
AC_SUBST(GUTENPRINT_VERSION)
AC_SUBST(GUTENPRINTUI2_CFLAGS)
AC_SUBST(GUTENPRINTUI2_LIBS)
AC_SUBST(GUTENPRINTUI2_LIBDEPS)
AC_SUBST(GUTENPRINTUI2_VERSION)
AC_SUBST(gutenprint_cflags)
gutenprint_libs="${GUTENPRINT_LIBS} ${gutenprint_libdeps}"
AC_SUBST(gutenprint_libs)
AC_SUBST(gutenprint_libdeps)
AC_SUBST(gutenprintui2_cflags)
gutenprintui2_libs="${GUTENPRINTUI2_LIBS} ${gutenprintui2_libdeps}"
AC_SUBST(gutenprintui2_libs)
AC_SUBST(gutenprintui2_libdeps)
AC_SUBST(LIBM)
AC_SUBST(LIBREADLINE_DEPS)
AC_SUBST(MAINTAINER_CFLAGS)
AC_SUBST(WHICH_PPDS)
AC_SUBST(BUILD_CUPS_PPDS)
AC_SUBST(GUTENPRINT_BASE_VERSION)
AC_SUBST(GUTENPRINT_RELEASE_VERSION)
AC_SUBST(LIBUSB_BACKEND_LIBDEPS)
AC_SUBST(PKGROOT)
AC_SUBST(MINIMAL_PRINTERS_TO_TEST)

dnl Optional architecture options to be applied after all autoconf tests
dnl are done...
AC_ARG_WITH(archflags, [  --with-archflags="..."  additional options for CFLAGS and LDFLAGS],
    CFLAGS="$CFLAGS $withval"
    LDFLAGS="$LDFLAGS $withval")

dnl Output files

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([doc/gutenprint.dox])
AC_CONFIG_FILES([doc/gutenprintui2.dox])
AC_CONFIG_FILES([doc/developer/Makefile])
AC_CONFIG_FILES([include/Makefile])
AC_CONFIG_FILES([include/gutenprint/Makefile])
AC_CONFIG_FILES([include/gutenprintui2/Makefile])
AC_CONFIG_FILES([include/gutenprint/gutenprint-version.h])
AC_CONFIG_FILES([man/Makefile])
AC_CONFIG_FILES([man/cups-calibrate.8])
AC_CONFIG_FILES([man/cups-genppd.8])
AC_CONFIG_FILES([man/cups-genppdupdate.8])
AC_CONFIG_FILES([man/escputil.1])
AC_CONFIG_FILES([po/Makefile.in])
AC_CONFIG_FILES([samples/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/cups/Makefile])
AC_CONFIG_FILES([src/cups/Info.plist])
CONFIG_FILE_EXEC([src/cups/cups-genppdupdate])
CONFIG_FILE_EXEC([src/cups/test-ppds.test])
CONFIG_FILE_EXEC([src/cups/min-pagesize])
AC_CONFIG_FILES([src/escputil/Makefile])
CONFIG_FILE_EXEC([src/testpattern/compare-image-files])
CONFIG_FILE_EXEC([src/testpattern/run-testpattern])
CONFIG_FILE_EXEC([src/testpattern/run-testpattern-1.test])
CONFIG_FILE_EXEC([src/testpattern/run-testpattern-2])
CONFIG_FILE_EXEC([src/testpattern/run-testpattern-2.test])
CONFIG_FILE_EXEC([src/testpattern/compare-checksums])
CONFIG_FILE_EXEC([src/testpattern/compress-checksums])
CONFIG_FILE_EXEC([src/cups/test-rastertogutenprint])
CONFIG_FILE_EXEC([src/cups/test-rastertogutenprint.test])
AC_CONFIG_FILES([src/testpattern/Makefile])
AC_CONFIG_FILES([src/gimp2/Makefile])
AC_CONFIG_FILES([src/main/Makefile])
AC_CONFIG_FILES([src/main/gutenprint.pc])
AC_CONFIG_FILES([src/xml/Makefile])
AC_CONFIG_FILES([src/xml/dither/Makefile])
AC_CONFIG_FILES([src/xml/papers/Makefile])
AC_CONFIG_FILES([src/xml/printers/Makefile])
AC_CONFIG_FILES([src/xml/escp2/Makefile])
AC_CONFIG_FILES([src/xml/escp2/media/Makefile])
AC_CONFIG_FILES([src/xml/escp2/mediasizes/Makefile])
AC_CONFIG_FILES([src/xml/escp2/model/Makefile])
AC_CONFIG_FILES([src/xml/escp2/model/base/Makefile])
AC_CONFIG_FILES([src/xml/escp2/inputslots/Makefile])
AC_CONFIG_FILES([src/xml/escp2/inks/Makefile])
AC_CONFIG_FILES([src/xml/escp2/weaves/Makefile])
AC_CONFIG_FILES([src/xml/escp2/qualitypresets/Makefile])
AC_CONFIG_FILES([src/xml/escp2/resolutions/Makefile])
AC_CONFIG_FILES([src/gutenprintui2/Makefile])
AC_CONFIG_FILES([src/gutenprintui2/gutenprintui2.pc])
AC_CONFIG_FILES([test/Makefile])
CONFIG_FILE_EXEC([test/compress-5level.sh])
CONFIG_FILE_EXEC([test/compress-6level.sh])
CONFIG_FILE_EXEC([test/uncompress-5level.sh])
CONFIG_FILE_EXEC([test/uncompress-6level.sh])
CONFIG_FILE_EXEC([test/parse-bjc])
CONFIG_FILE_EXEC([test/parse-escp2])
CONFIG_FILE_EXEC([test/run-testdither.test])
CONFIG_FILE_EXEC([test/run-weavetest.test])
CONFIG_FILE_EXEC([test/test-curve.test])
AC_CONFIG_FILES([scripts/Makefile])
CONFIG_FILE_EXEC([scripts/mkgitlog])
CONFIG_FILE_EXEC([scripts/gversion])
CONFIG_FILE_EXEC([scripts/test-gversion])
CONFIG_FILE_EXEC([scripts/wzpaq])
AC_CONFIG_COMMANDS([default], [
echo
],
[GUTENPRINT_RELEASE_VERSION=${GUTENPRINT_RELEASE_VERSION}]
[GUTENPRINT_BASE_VERSION=${GUTENPRINT_BASE_VERSION}]
)

AC_OUTPUT

warnings_emitted=0
echo
echo "Gutenprint Configuration Summary:" |tee config.summary
echo "---------- ------------- --------" |tee -a config.summary
echo |tee -a config.summary
echo "If you have any problems, please report the information below to" |tee -a config.summary
echo "$PACKAGE_BUGREPORT" |tee -a config.summary
echo |tee -a config.summary
echo "================================================================" |tee -a config.summary
echo "  Release: $PACKAGE_STRING generated on $RELEASE_DATE" |tee -a config.summary
echo "  Generated at `date` by $LOGNAME" |tee -a config.summary
echo |tee -a config.summary
echo "  Features:" |tee -a config.summary
if test "$BUILD_CUPS" != "no" ; then
    echo "    Build CUPS:                                 $BUILD_CUPS, installing in $cups_prefix" |tee -a config.summary
    echo "        Build CUPS 1.2 enhancements:            $BUILD_CUPS_1_2" |tee -a config.summary
    echo "        Build CUPS PPD files:                   $BUILD_CUPS_PPDS" |tee -a config.summary
    if test "$BUILD_CUPS_PPDS" != "no" ; then
        echo "            Build translated CUPS PPD files:    $BUILD_TRANSLATED_CUPS_PPDS" |tee -a config.summary
        echo "                Build global CUPS PPD files:    $BUILD_GLOBALIZED_CUPS_PPDS" |tee -a config.summary
        echo "            Build simplified CUPS PPD files:    $BUILD_SIMPLIFIED_CUPS_PPDS" |tee -a config.summary
        echo "            Install CUPS PPDs at top level:     $CUPS_PPDS_AT_TOP_LEVEL" |tee -a config.summary
    fi
    if test "$BUILD_CUPS_PPDS" != "no" -o "$BUILD_CUPS_1_2" != "no" ; then
        echo "        Generate PS level 3 CUPS PPD files:     $USE_LEVEL3_PS" |tee -a config.summary
    fi
    echo "        Build genppd statically:                $STATIC_GENPPD" |tee -a config.summary

    if test "$STATIC_GENPPD" = "no" ; then
        echo "            ***WARNING: Use of --disable-static-genppd or --disable-static" |tee -a config.summary
        echo "                        when building CUPS is very dangerous.  The build may" |tee -a config.summary
        echo "                        fail when building the PPD files, or may *SILENTLY*" |tee -a config.summary
        echo "                        build incorrect PPD files or cause other problems." |tee -a config.summary
        echo "                        Please review the README and release notes carefully!" |tee -a config.summary
        warnings_emitted=1
    fi
    echo "        Build CUPS dyesub USB backend:          $BUILD_LIBUSB_BACKENDS" |tee -a config.summary
else
    echo "    Build CUPS:                                 $BUILD_CUPS" |tee -a config.summary
fi
echo "    Build EPSON inkjet utility:                 $BUILD_ESCPUTIL" |tee -a config.summary
if test "$BUILD_GIMP2" != "no" ; then
    echo "    Build enhanced Print plugin for GIMP:       yes" |tee -a config.summary
    if test "$BUILD_GIMP2_AS_GUTENPRINT" != "no" ; then
        echo "        GIMP plugin will be named:              gutenprint" |tee -a config.summary
    else
        echo "        GIMP plugin will be named:              print" |tee -a config.summary
    fi
    echo "        Install plugin(s) in home directory:    $USER_INSTALL" |tee -a config.summary
else
    echo "    Build enhanced Print plugin for GIMP:       no" |tee -a config.summary
fi
echo "    Build test programs:                        $BUILD_TEST" |tee -a config.summary
echo "    Build testpattern generator:                $BUILD_TESTPATTERN" |tee -a config.summary
echo |tee -a config.summary
echo "  Installation summary:" |tee -a config.summary
echo "    Installation prefix:                        $prefix" |tee -a config.summary
# Configure intentionally does not expand exec_prefix to allow make
# to do so.
echo "    Exec prefix:                                `eval eval echo $exec_prefix` ($exec_prefix)" |tee -a config.summary
echo "    Data directory:                             $PACKAGE_DATA_DIR" |tee -a config.summary
echo "    Library directory:                          `eval eval echo $PACKAGE_LIB_DIR` ($PACKAGE_LIB_DIR)" |tee -a config.summary
echo "    Executable directory:                       `eval eval echo $PACKAGE_BIN_DIR` ($PACKAGE_BIN_DIR)" |tee -a config.summary
echo "    XML data directory:                         $PKGXMLDATADIR" |tee -a config.summary
echo "    Module directory:                           `eval eval echo $PKGMODULEDIR` ($PKGMODULEDIR)" |tee -a config.summary
echo "    Install sample images:                      $INSTALL_SAMPLES" |tee -a config.summary
echo |tee -a config.summary
echo "  General configuration:" |tee -a config.summary
echo "    Configure arguments:                        $configure_args" |tee -a config.summary
echo "    Compiler:                                   $CC" |tee -a config.summary
echo "    Compiler Version:                           `"$CC" -v 2>&1 |grep ' version '`" |tee -a config.summary
if test x$ac_compiler_gnu = "xyes"; then
    echo "    Compiler options:                           $CFLAGS $GNUCFLAGS" |tee -a config.summary
else
    echo "    Compiler options:                           $CFLAGS" |tee -a config.summary
fi
echo "    Build static libraries:                     $enable_static" |tee -a config.summary
echo "    Build shared libraries:                     $enable_shared" |tee -a config.summary
echo "    Maintainer mode:                            $USE_MAINTAINER_MODE" |tee -a config.summary
echo "    Use i18n:                                   $USE_NLS" |tee -a config.summary
echo "    Generate profiling information:             $ENABLE_PROFILE" |tee -a config.summary
echo "    Generate debugging symbols:                 $ENABLE_DEBUG" |tee -a config.summary
echo "    Use modules:                                $WITH_MODULES" |tee -a config.summary
if test -n "$EXTRA_LIBREADLINE_DEPS" ; then
    echo "    Use readline libraries:                     $USE_READLINE, extra arguments: $EXTRA_LIBREADLINE_DEPS" |tee -a config.summary
else
    echo "    Use readline libraries:                     $USE_READLINE" |tee -a config.summary
fi
echo "    uname -a output:                            `uname -a`" |tee -a config.summary
echo "================================================================" |tee -a config.summary
if test $warnings_emitted -ne 0 ; then
    echo
    echo '*** WARNING: Please review the warnings shown above carefully!'
fi
echo |tee -a config.summary
if test "$BUILD_GIMP2" = "no" -o "$BUILD_CUPS" = "no" ; then
    echo "VERY IMPORTANT:" |tee -a config.summary
    echo "If you expect to build the CUPS driver or the enhanced Print plugin for" |tee -a config.summary
    echo "GIMP, but the output above indicates that that component will not be built," |tee -a config.summary
    echo "it almost certainly indicates that development packages required to build" |tee -a config.summary
    echo "that component are not installed on your system.  Please read the README" |tee -a config.summary
    echo "file and install the necessary development packages before reporting any" |tee -a config.summary
    echo "error to the development team.  Depending upon your operating system version," |tee -a config.summary
    echo "you may have to install packages from the installation medium or over" |tee -a config.summary
    echo "the network." |tee -a config.summary
    echo |tee -a config.summary
fi
echo "Finished configuring.  Please review CAREFULLY the configuration summary above."
echo "This summary is also stored in config.summary"
echo "Type 'make clean' followed by 'make' to build the package"
echo "then 'make install' to install it."
