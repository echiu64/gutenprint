dnl $Id$
dnl Copyright (C) 2001 Roger Leigh
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
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

dnl Process this file with autoconf to produce a configure script.

dnl MAKING RELEASES (a step-by-step guide!)
dnl ===============
dnl
dnl Since the last release:
dnl 1. if only source code (not the interface) has changed, set
dnl      GIMPPRINT_MICRO_VERSION += 1;
dnl      GIMPPRINT_INTERFACE_AGE += 1;
dnl 2. if any functions have been added, removed, or changed, set
dnl      GIMPPRINT_INTERFACE_AGE = 0;
dnl      GIMPPRINT_CURRENT_INTERFACE += 1;
dnl 3. if interfaces have been added, set
dnl      GIMPPRINT_BINARY_AGE += 1;
dnl 4. if interfaces have been removed, set
dnl      GIMPPRINT_BINARY_AGE = 0;
dnl
dnl For more detailed information, see the libtool info documentation.
dnl
pushdef([GIMPPRINT_NAME],              [gimp-print])
pushdef([GIMPPRINT_MAJOR_VERSION],     [4])
pushdef([GIMPPRINT_MINOR_VERSION],     [3])
pushdef([GIMPPRINT_MICRO_VERSION],     [1])
pushdef([GIMPPRINT_EXTRA_VERSION],     [])
pushdef([GIMPPRINT_CURRENT_INTERFACE], [1])
pushdef([GIMPPRINT_INTERFACE_AGE],     [0])
pushdef([GIMPPRINT_BINARY_AGE],        [0])
pushdef([GIMPPRINT_VERSION], GIMPPRINT_MAJOR_VERSION.GIMPPRINT_MINOR_VERSION.GIMPPRINT_MICRO_VERSION[]GIMPPRINT_EXTRA_VERSION)

AC_INIT(GIMPPRINT_NAME, GIMPPRINT_VERSION, [gimp-print-devel@lists.sourceforge.net])
AC_PREREQ(2.50)
AC_REVISION($Revision$)
AC_CONFIG_SRCDIR(src/main/print-dither.c)
AC_CONFIG_AUX_DIR(scripts)
STP_INIT
AC_COPYRIGHT(
[
Parts of this configure script come from the `configure.ac.in' file, and
are not based on freely-redistributable m4 macros; you can redistribute
them and/or modify them under the terms of the GNU General Public
as published by the Free Software Foundation; either version 2, or (at
your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
])

dnl In the following script, there are a the following variants
dnl of gimpprint cflags and libs variables
dnl
dnl GIMPPRINT_CFLAGS:  cflags for compiling libraries and example progs
dnl GIMPPRINT_LIBS:    libraries for linking programs. ONLY to be used
dnl                    to generate ${gimpprint_libs}
dnl GIMPPRINT_DEPLIBS: libraries for linking libraries against
dnl gimpprint_cflags:  cflags to store in gimpprint-config
dnl gimpprint_libs:    libs to store in gimpprint-config
dnl LIBGIMPPRINT_LIBS  libs to link programs IN THIS PACKAGE ONLY against

dnl initial default values
GIMPPRINT_LIBS="-lgimpprint"

[GIMPPRINT_MAJOR_VERSION]=GIMPPRINT_MAJOR_VERSION
[GIMPPRINT_MINOR_VERSION]=GIMPPRINT_MINOR_VERSION
[GIMPPRINT_MICRO_VERSION]=GIMPPRINT_MICRO_VERSION
[GIMPPRINT_EXTRA_VERSION]=GIMPPRINT_EXTRA_VERSION
[GIMPPRINT_CURRENT_INTERFACE]=GIMPPRINT_CURRENT_INTERFACE
[GIMPPRINT_INTERFACE_AGE]=GIMPPRINT_INTERFACE_AGE
[GIMPPRINT_BINARY_AGE]=GIMPPRINT_BINARY_AGE
[GIMPPRINT_VERSION]=GIMPPRINT_VERSION
popdef([GIMPPRINT_MAJOR_VERSION])
popdef([GIMPPRINT_MINOR_VERSION])
popdef([GIMPPRINT_MICRO_VERSION])
popdef([GIMPPRINT_EXTRA_VERSION])
popdef([GIMPPRINT_CURRENT_INTERFACE])
popdef([GIMPPRINT_INTERFACE_AGE])
popdef([GIMPPRINT_BINARY_AGE])

AC_SUBST(GIMPPRINT_MAJOR_VERSION)
AC_SUBST(GIMPPRINT_MINOR_VERSION)
AC_SUBST(GIMPPRINT_MICRO_VERSION)
AC_SUBST(GIMPPRINT_CURRENT_INTERFACE)
AC_SUBST(GIMPPRINT_INTERFACE_AGE)
AC_SUBST(GIMPPRINT_BINARY_AGE)
AC_SUBST(GIMPPRINT_VERSION)

dnl libtool versioning
LT_RELEASE=$GIMPPRINT_MAJOR_VERSION.$GIMPPRINT_MINOR_VERSION
LT_CURRENT=$GIMPPRINT_CURRENT_INTERFACE
LT_REVISION=$GIMPPRINT_INTERFACE_AGE
LT_AGE=$GIMPPRINT_BINARY_AGE
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

dnl Initialize automake stuff.
AM_INIT_AUTOMAKE(GIMPPRINT_NAME, GIMPPRINT_VERSION)
popdef([GIMPPRINT_NAME])
popdef([GIMPPRINT_VERSION])

dnl Specify a configuration file.
AM_CONFIG_HEADER(config.h)

dnl Available languages (translations)
ALL_LINGUAS="da el en_GB es fr nl no pl sk sv"
AC_SUBST(ALL_LINGUAS)

dnl Set up libtool scripts.
dnl Disable static library building to speed up the build
AC_DISABLE_STATIC
AM_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)
dnl define HAVE_GNU_LD if GNU ld is present
if test "$ac_cv_prog_gnu_ld" = "yes" ; then
  AC_DEFINE(HAVE_GNU_LD,, [Define if GNU ld is present.])
fi

dnl Compiler options and definitions
INCLUDES="${INCLUDES} -I\$(top_srcdir)/include -I\$(top_builddir)/include -I\$(top_srcdir)/intl"

dnl Set up maintainer mode
AM_MAINTAINER_MODE
if test x${USE_MAINTAINER_MODE} = xyes ; then
  MAINTAINER_MODE="true"
  BUILD_TEST="yes"
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
AC_PATH_PROG(CUPS_CONFIG, cups-config)
if test -z "${CUPS_CONFIG}" ; then
  BUILD_CUPS="no"
else
  BUILD_CUPS="yes"
fi
STP_ARG_WITH_DETAILED(
  [cups],
  [=PREFIX],
  [whether to build CUPS driver],
  [(default PREFIX=/usr)],
  [BUILD_CUPS],
  [(automatic)],
  [
    yes) [BUILD_CUPS]="yes" ; AC_MSG_RESULT([yes]) ; cups_prefix="/usr" ;;
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
     cups_prefix="${prefix}"
  ])

STP_ARG_ENABLE([translated-ppds],
               [build translated CUPS PPD files],
               [BUILD_TRANSLATED_PPDS],
               [yes])

STP_ARG_ENABLE([cups-level3-ppds],
               [generate PostScript level 3 PPD files],
               [USE_LEVEL3_PS],
               [no])

AC_PATH_PROG(FOOMATIC_CONFIGURE, foomatic-configure)
if test -z "${FOOMATIC_CONFIGURE}" ; then
  BUILD_FOOMATIC="no"
else
  BUILD_FOOMATIC="yes"
fi
STP_ARG_WITH([foomatic],
             [build foomatic data files],
             [BUILD_FOOMATIC],
             [(automatic)])


AC_PATH_PROG(IJS_CONFIG, ijs-config)
if test -z "${IJS_CONFIG}" ; then
  BUILD_IJS="no"
else
  BUILD_IJS="yes"
fi
STP_ARG_WITH([ijs],
             [build IJS driver],
             [BUILD_IJS],
             [(automatic)])

STP_ARG_ENABLE([lexmarkutil],
               [build lexmarkutil],
               [BUILD_LEXMARKUTIL],
               [no])

STP_ARG_ENABLE([debug],
               [turn on debugging in build],
               [ENABLE_DEBUG],
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
AC_PATH_PROG(GIMP_CONFIG, gimp-config)
if test -z "${GIMP_CONFIG}" ; then
  BUILD_GIMP="no"
else
  BUILD_GIMP="yes"
fi
STP_ARG_WITH([gimp],
             [build GIMP Print plugin],
             [BUILD_GIMP],
             [(automatic)])

STP_ARG_ENABLE([samples],
               [install sample images],
               [INSTALL_SAMPLES],
               [yes])

STP_ARG_ENABLE([user-guide],
               [install user guide],
               [INSTALL_USER_GUIDE],
               [yes])

STP_ARG_ENABLE([escputil],
               [build escputil],
               [BUILD_ESCPUTIL],
               [yes])

STP_ARG_ENABLE([libgimpprint],
               [build libgimpprint],
               [BUILD_LIBGIMPPRINT],
               [yes])

STP_ARG_ENABLE([testpattern],
               [build testpattern generator],
               [BUILD_TESTPATTERN],
               [no])

STP_ARG_ENABLE([test],
               [build test programs],
               [BUILD_TEST],
               [no])

STP_ARG_ENABLE([user-install],
                  [install GIMP plugin in home directory],
                  [USER_INSTALL],
                  [no])
if test x$USER_INSTALL = xyes ; then
  PLUG_IN_PATH="bin"
else
  PLUG_IN_PATH="admin-bin"
fi

dnl aclocal flags.
dnl AM_ACLOCAL_INCLUDE(m4 m4local)

dnl Checks for programs.
AC_ISC_POSIX
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_LEX
AC_PROG_YACC
AC_PATH_PROG(PERL, perl)
AC_PATH_PROG(CONVERT, convert)
if test x${CONVERT} = x ; then
  AC_MSG_WARN([PostScript user's guide cannot be regenerated!])
fi
AC_PATH_PROG(TEXI2HTML, texi2html)
if test x${TEXI2HTML} = x ; then
  AC_MSG_WARN([HTML documentation cannot be regenerated!])
fi
AC_PATH_PROG(DVIPS, dvips)
if test x${DVIPS} = x ; then
  AC_MSG_WARN([PostScript documentation cannot be regenerated!])
fi
AC_PATH_PROG(DB2PDF, db2pdf)
if test x${DB2PDF} = x ; then
  AC_MSG_WARN([PDF user's guide cannot be regenerated!])
fi
AC_PATH_PROGS(DVIPDF, dvipdf dvipdfm)
if test x${DVIPDF} = x ; then
  AC_MSG_WARN([PDF documentation cannot be regenerated!])
fi
AC_PATH_PROG(DB2PS, db2ps)
if test x${DB2PS} = x ; then
  AC_MSG_WARN([PostScript user's guide cannot be regenerated!])
fi
AC_PATH_PROG(DB2HTML, db2html)
if test x${DB2PS} = x ; then
  AC_MSG_WARN([HTML user's guide cannot be regenerated!])
fi

dnl foomatic checks
if test x${BUILD_FOOMATIC} = xyes ; then
  AC_PATH_PROG(FOOMATIC_KITLOAD, foomatic-kitload, , $PATH:/usr/sbin:/usr/local/sbin)
  if test -z "$FOOMATIC_KITLOAD" ; then
    AC_MSG_WARN(Cannot find foomatic-kitload; attempt to install foomatic will fail)
  fi
fi

dnl ijs checks
if test x${BUILD_IJS} = xyes ; then
  if test -z "$IJS_CONFIG" ; then
    AC_MSG_ERROR(Cannot find ijs-config; please ensure ijs 0.30 is
installed);
  fi
  IJS_CFLAGS=`$IJS_CONFIG --cflags`
  IJS_LIBS=`$IJS_CONFIG --libs`
fi

dnl GNU gettext checks.
AH_TEMPLATE(ENABLE_NLS, [AM_GNU_GETTEXT should provide this])
AH_TEMPLATE(HAVE_CATGETS, [AM_GNU_GETTEXT should provide this])
AH_TEMPLATE(HAVE_GETTEXT, [AM_GNU_GETTEXT should provide this])
AH_TEMPLATE(HAVE_LC_MESSAGES, [AM_GNU_GETTEXT should provide this])
AM_GNU_GETTEXT

dnl Set PACKAGE_LOCALE_DIR in config.h
AH_TEMPLATE(PACKAGE_LOCALE_DIR, [Package locale directory])
if test "x${prefix}" = "xNONE"; then
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, ["${ac_default_prefix}/${DATADIRNAME}/locale"])
else
  AC_DEFINE_UNQUOTED(PACKAGE_LOCALE_DIR, ["${prefix}/${DATADIRNAME}/locale"])
fi

dnl Set PACKAGE_DATA_DIR in config.h.
AH_TEMPLATE(PACKAGE_DATA_DIR,, [Package data directory])
if test "x${datadir}" = 'x${prefix}/share'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${ac_default_prefix}/share/${PACKAGE}")
  else
    AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${prefix}/share/${PACKAGE}")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${datadir}/${PACKAGE}")
fi

dnl Set PACKAGE_SOURCE_DIR in config.h.
AH_TEMPLATE(PACKAGE_SOURCE_DIR,, [Package source directory])
packagesrcdir=`cd $srcdir && pwd`
AC_DEFINE_UNQUOTED(PACKAGE_SOURCE_DIR, "${packagesrcdir}")

dnl Compiler flags
if test "$CC" = "gcc" ; then
  :
else
  if test x$ENABLE_DEBUG = xyes ; then
    CFLAGS="${CFLAGS:=} -g"
  else
    CFLAGS="${CFLAGS:=} -O"
  fi
fi
if test "$CC" = "gcc" ; then
  GNUCFLAGS="-Wall -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wwrite-strings -pedantic"
fi
AC_SUBST(GNUCFLAGS)

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

STP_CUPS_LIBS
if test x$BUILD_LEXMARKUTIL = xyes ; then
AM_PATH_GTK(1.2.0, ,
            AC_MSG_ERROR(Cannot find GTK: Is gtk-config in path?))
fi

STP_GIMP_LIBS

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADER(readline/readline.h,
  HAVE_READLINE_READLINE_H=true,
  HAVE_READLINE_READLINE_H=false)
if test x${HAVE_READLINE_READLINE_H} = xtrue ; then
  AC_DEFINE(HAVE_READLINE_READLINE_H,, [Define if libreadline header is present.])
  if test x${HAVE_LIBREADLINE} = xtrue ; then
    LIBREADLINE_DEPS="-lreadline $EXTRA_LIBREADLINE_DEPS"
  fi
fi

AC_CHECK_HEADERS(varargs.h)
AC_CHECK_HEADERS(stdarg.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SIGNAL

dnl Checks for library functions.
AC_FUNC_ALLOCA
dnl [commented out while I find a memcmp.c for lib/] AC_FUNC_MEMCMP
dnl GNU libc memcmp.c is too glibc-dependent to include.
AC_CHECK_FUNCS(poll)
dnl The following is used to build a "utility library" in lib/
AC_REPLACE_FUNCS(xgetcwd xmalloc)
AC_CHECK_FUNC(asprintf,
  AC_DEFINE(HAVE_ASPRINTF,, [Define if you have the `asprintf' function.]),
  AC_LIBOBJ(vasprintf))
AC_CHECK_FUNC(vasprintf,
  AC_DEFINE(HAVE_VASPRINTF,, [Define if you have the `vasprintf' function.]),
  AC_LIBOBJ(vasprintf))
AC_SUBST(PRINT_VASPRINTF)
AC_CHECK_FUNC(getopt_long,
  AC_DEFINE(HAVE_GETOPT_LONG,, [Define if you have the `getopt_long' function.]),
  AC_LIBOBJ(getopt)
  AC_LIBOBJ(getopt1))

dnl Define what has to be built
AM_CONDITIONAL(BUILD_FOOMATIC, test x${BUILD_FOOMATIC} = xyes)

AM_CONDITIONAL(BUILD_IJS, test x${BUILD_IJS} = xyes)

AM_CONDITIONAL(BUILD_LEXMARKUTIL, test x${BUILD_LEXMARKUTIL} = xyes)

AM_CONDITIONAL(BUILD_CUPS, test x${BUILD_CUPS} = xyes)

AM_CONDITIONAL(BUILD_TRANSLATED_PPDS, test x${BUILD_TRANSLATED_PPDS} = xyes)

if test x${USE_LEVEL3_PS} = xyes ; then
  CUPS_PPD_PS_LEVEL=3
else
  CUPS_PPD_PS_LEVEL=2
fi

AM_CONDITIONAL(USE_LEVEL3_PS, test x${USE_LEVEL3_PS} = xyes)

AM_CONDITIONAL(BUILD_ESCPUTIL, test x${BUILD_ESCPUTIL} = xyes)

AM_CONDITIONAL(BUILD_TEST, test x${BUILD_TEST} = xyes)

AM_CONDITIONAL(BUILD_TESTPATTERN, test x${BUILD_TESTPATTERN} = xyes)

AM_CONDITIONAL(BUILD_GIMP, test x${BUILD_GIMP} = xyes)

if test x${BUILD_LIBGIMPPRINT} = xyes ; then
  LIBGIMPPRINT_LIBS="\$(top_builddir)/src/main/libgimpprint.la"
  GIMP_PRINT_H="gimp-print.h"
  INCLUDE_GIMP_PRINT_H="\"gimp-print/gimp-print.h\""
else
  STP_PATH_LIB_REGISTER([_STP_PATH_LIB_HEADER], [gimp-print/gimp-print.h])
  STP_PATH_LIB(
    gimpprint, $GIMPPRINT_VERSION, , ,
    [LIBGIMPPRINT_CFLAGS="${GIMPPRINT_CFLAGS}"
      LIBGIMPPRINT_LIBS="${GIMPPRINT_LIBS}"
      INCLUDE_GIMP_PRINT_H="<gimp-print/gimp-print.h>"],
    [AC_MSG_WARN([Cannot find current GIMP-PRINT library: building included libgimpprint])
      LIBGIMPPRINT_LIBS="\$(top_builddir)/src/main/libgimpprint.la"
      GIMP_PRINT_H="gimp-print/gimp-print.h"
      INCLUDE_GIMP_PRINT_H="\"gimp-print/gimp-print.h\""]
  )
fi
AM_CONDITIONAL(BUILD_LIBGIMPPRINT, test x${BUILD_LIBGIMPPRINT} = xyes)

AM_CONDITIONAL(INSTALL_SAMPLES, test x${INSTALL_SAMPLES} = xyes)

AM_CONDITIONAL(INSTALL_USER_GUIDE, test x${INSTALL_USER_GUIDE} = xyes)

STP_CUPS_PATH

AM_CONDITIONAL(USE_NLS, test x${USE_NLS} = xyes)


dnl Define LTLIBOBJS
AC_CONFIG_COMMANDS_PRE(
[LTLIBOBJS=`echo "$LIBOBJS" | sed 's/\.o/.lo/g'`
AC_SUBST(LTLIBOBJS)
LTALLOCA=`echo "$ALLOCA" | sed 's/\.o/.lo/g'`
AC_SUBST(LTALLOCA)])

dnl Definitions
AC_DEFINE_UNQUOTED(CUPS_DATADIR, "${cups_exec_prefix}/share/cups", [CUPS data directory.])
AC_DEFINE_UNQUOTED(CUPS_PPD_PS_LEVEL, ${CUPS_PPD_PS_LEVEL}, [CUPS PPD PostScript level])
AC_DEFINE_UNQUOTED(INCLUDE_GIMP_PRINT_H, ${INCLUDE_GIMP_PRINT_H}, [GIMP-Print header to include.])
STP_RELEASE_DATE

dnl Some versions of make get very grouchy about $(RM)
RM='rm'

dnl Substitutions
AC_SUBST(cups_prefix)
AC_SUBST(cups_exec_prefix)
AC_SUBST(cups_bindir)
AC_SUBST(cups_conf_datadir)
AC_SUBST(cups_conf_serverbin)
AC_SUBST(cups_conf_serverroot)
AC_SUBST(CUPS_BIN)
AC_SUBST(CUPS_BACKEND_BIN)
AC_SUBST(CUPS_FILTER_BIN)
AC_SUBST(CUPS_PKG)
AC_SUBST(CUPS_CONF)
AC_SUBST(CUPS_INSTALL)
AC_SUBST(CUPS_CFLAGS)
AC_SUBST(CUPS_LIBS)
AC_SUBST(CUPS_MAN)
AC_SUBST(CUPS_PPD_PS_LEVEL)
AC_SUBST(CUPS_NOINST_BIN)
AC_SUBST(FOOMATIC_NOINST_BIN)
AC_SUBST(FOOMATIC_INSTALL)
AC_SUBST(GENPPD_LIBS)
AC_SUBST(IJS_BIN)
AC_SUBST(IJS_INSTALL)
AC_SUBST(IJS_CFLAGS)
AC_SUBST(IJS_LIBS)
AC_SUBST(TEST_BIN)
AC_SUBST(TESTPATTERN_BIN)
AC_SUBST(RM)
AC_SUBST(ESCPUTIL_BIN)
AC_SUBST(ESCPUTIL_MAN)
AC_SUBST(GIMP_BIN)
AC_SUBST(GIMP_CFLAGS)
AC_SUBST(GIMP_LIBS)
AC_SUBST(GIMP_PRINT_H)
AC_SUBST(GIMPPRINT_CFLAGS)
AC_SUBST(GIMPPRINT_LIBS)
AC_SUBST(GIMPPRINT_DEPLIBS)
AC_SUBST(GIMPPRINT_VERSION)
AC_SUBST(gimpprint_cflags)
gimpprint_libs="${GIMPPRINT_LIBS} ${GIMPPRINT_DEPLIBS}"
AC_SUBST(gimpprint_libs)
AC_SUBST(INCLUDES)
LIBGIMPPRINT_LIBS="${LIBGIMPPRINT_LIBS} ${GIMPPRINT_DEPLIBS}"
AC_SUBST(LIBGIMPPRINT_LIBS)
AC_SUBST(LIBREADLINE_DEPS)
AC_SUBST(MAIN_BIN)
AC_SUBST(MAIN_DAT)
AC_SUBST(MAIN_MAN)
AC_SUBST(MAIN_SCR)
AC_SUBST(MAINTAINER_CFLAGS)
AC_SUBST(PLUG_IN_PATH)
AC_SUBST(SAMPLE_IMAGES)
AC_SUBST(USER_GUIDE)

dnl Output files
dnl po/Makefile.in *MUST* be surrounded by spaces to work with buggy
dnl gettext 0.10.36.  Do not move this file to the beginning or end
dnl of a line.

STP_CONFIG_LIBCONFIG_IN_PKGCONFIG
STP_CONFIG_LIBCONFIG_IN([gimpprint], [src/main])
STP_CONFIG_PKGCONFIG_IN([gimpprint], [GIMP Print Top Quality Printer Drivers], [src/main])
STP_CONFIG_GIMPPRINT_M4

literal_includedir=`eval echo $includedir`
literal_libdir=`eval echo $libdir`

AC_SUBST(literal_includedir)
AC_SUBST(literal_libdir)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([doc/developer/Makefile])
AC_CONFIG_FILES([doc/users_guide/Makefile])
AC_CONFIG_FILES([include/Makefile])
AC_CONFIG_FILES([include/gimp-print/Makefile])
AC_CONFIG_FILES([include/gimp-print/gimp-print.h])
AC_CONFIG_FILES([intl/Makefile])
AC_CONFIG_FILES([lib/Makefile])
AC_CONFIG_FILES([man/Makefile])
AC_CONFIG_FILES([man/escputil.1])
AC_CONFIG_FILES([man/cups-calibrate.8])
AC_CONFIG_FILES([man/gimpprint.3])
AC_CONFIG_FILES([man/gimpprint-config.1])
AC_CONFIG_FILES([po/Makefile.in])
AC_CONFIG_FILES([samples/Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/cups/Makefile])
AC_CONFIG_FILES([src/escputil/Makefile])
AC_CONFIG_FILES([src/foomatic/Makefile])
AC_CONFIG_FILES([src/foomatic/foomatic-generator], [chmod +x src/foomatic/foomatic-generator])
AC_CONFIG_FILES([src/ghost/Makefile])
AC_CONFIG_FILES([src/testpattern/Makefile])
AC_CONFIG_FILES([src/gimp/Makefile])
AC_CONFIG_FILES([src/main/Makefile])
AC_CONFIG_FILES([src/main/gimpprint-config], [chmod +x src/main/gimpprint-config])
AC_CONFIG_FILES([src/main/gimpprint.pc])
AC_CONFIG_FILES([src/printdef/Makefile])
AC_CONFIG_FILES([cups-gimpprint.list])
AC_CONFIG_FILES([test/Makefile])
AC_CONFIG_FILES([src/ghost/contrib.mak.addon], [
if test -f src/ghost/contrib.mak.addon ; then
  sed -e 's;${exec_prefix};'"${literal_exec_prefix};" -e 's;${prefix};'"${literal_prefix};" src/ghost/contrib.mak.addon > src/ghost/contrib.mak.addon.tmp
  mv src/ghost/contrib.mak.addon.tmp src/ghost/contrib.mak.addon
fi
], [
literal_prefix="`eval echo $prefix`"
literal_exec_prefix="`eval echo $exec_prefix`"
])
AC_CONFIG_COMMANDS([default], [
echo "Finished configuring."
echo "Type 'make' to build the package"
echo "then 'make install' to install it."
])

AC_OUTPUT
