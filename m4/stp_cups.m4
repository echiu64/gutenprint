# This file is part of Gutenprint.                     -*- Autoconf -*-
# CUPS support.
# Copyright 2000-2002 Roger Leigh
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# 02111-1307, USA.


## ------------ ##
## CUPS support ##
## ------------ ##


# STP_CUPS_LIBS
# -------------
# Check for CUPS libraries.
AC_DEFUN([STP_CUPS_LIBS],
[dnl CUPS library checks
dnl check for libm, and also add to CUPS LIBS
AC_CHECK_LIB(m,pow,
  CUPS_LIBS="${CUPS_LIBS} -lm")
dnl CUPS library checks
if test x${BUILD_CUPS} = xyes ; then
  AC_CHECK_LIB(z,gzgets,
    HAVE_LIBZ=true, HAVE_LIBZ=false)
  if test x${HAVE_LIBZ} = xtrue ; then
    GENPPD_LIBS="-lz"
    AC_DEFINE(HAVE_LIBZ,, [Define if libz is present.])
  fi
  AC_PATH_PROG(CUPS_CONFIG, cups-config)
  if test "x$CUPS_CONFIG" != x; then
    dnl Use values from CUPS config...
    CUPS_LIBS="`$CUPS_CONFIG --ldflags` `$CUPS_CONFIG --image --libs`"
    CUPS_CFLAGS="`$CUPS_CONFIG --cflags`"
  else
    dnl Save current library list...
    SAVELIBS="$LIBS"
    LIBS="$LIBS ${CUPS_LIBS}"
    AC_CHECK_LIB(socket,socket,
      if test x${OSTYPE} != xirix ; then
      CUPS_LIBS="${CUPS_LIBS} -lsocket"
      LIBS="$LIBS -lsocket"
    fi)
    AC_CHECK_LIB(nsl,gethostbyaddr,
      if test x${OSTYPE} != xirix ; then
      CUPS_LIBS="${CUPS_LIBS} -lnsl"
      LIBS="$LIBS -lnsl"
    fi)

    dnl Some OS's need to link against crypto stuff too if CUPS is compiled
    dnl with crypto support... :(
    AC_CHECK_HEADER(openssl/ssl.h,
      dnl Some ELF systems can't resolve all the symbols in libcrypto
      dnl if libcrypto was linked against RSAREF, and fail to link the
      dnl test program correctly, even though a correct installation
      dnl of OpenSSL exists.  So we test the linking three times in
      dnl case the RSAREF libraries are needed.

      SSL="no"

      for libcrypto in \
	  "-lcrypto" \
	  "-lcrypto -lrsaref" \
	  "-lcrypto -lRSAglue -lrsaref"
      do
	  AC_CHECK_LIB(ssl,SSL_new,
	      [CUPS_LIBS="${CUPS_LIBS} -lssl $libcrypto"
	       SSL="yes"],,
	      $libcrypto)

	  if test x$SSL = xyes; then
	      break
	  fi
      done)

    dnl Require CUPS 1.1...
    AC_CHECK_LIB(cups,cupsPrintFiles,
      CUPS_LIBS="${CUPS_LIBS} -lcups",
      LIBS="$LIBS -lcups"
      AC_MSG_ERROR([Cannot find CUPS libraries (libcups)]))
    AC_CHECK_LIB(cupsimage,cupsRasterOpen,
      CUPS_LIBS="${CUPS_LIBS} -lcupsimage",
      AC_MSG_ERROR([Cannot find CUPS libraries (libcupsimage)]),
      -lcups)
    dnl Restore old library list...
    LIBS="$SAVELIBS"

    dnl Add CUPS include directory as needed...
    if test "x${cups_prefix}" != "x/usr" -a "x${cups_prefix}" != x; then
      CUPS_CFLAGS="-I${cups_prefix}/include"
    else
      CUPS_CFLAGS=""
    fi
  fi
fi
])

# STP_CUPS_PATH
# -------------
# Set CUPS install paths
AC_DEFUN([STP_CUPS_PATH],
[# CUPS path setup

# Fix "cups_prefix" variable if it hasn't been specified...
if test "x${cups_prefix}" = xNONE -o "x{$cups_prefix}" = x ; then
  if test "x${prefix}" = xNONE -o "x${prefix}" = x ; then
    cups_prefix="/usr"
  else
    cups_prefix="${prefix}"
  fi
fi

# Fix "cups_exec_prefix" variable if it hasn't been specified...
if test "x${cups_exec_prefix}" = xNONE  -o "x${cups_exec_prefix}" = x ; then
  if test x"${exec_prefix}" = xNONE -o "x${exec_prefix}" = x ; then
    cups_exec_prefix="${cups_prefix}"
  else
    cups_exec_prefix="${exec_prefix}"
  fi
fi

# Get explicit CUPS directories if possible
if test "x$CUPS_CONFIG" != x -a "x$NO_PKGCONFIG_PATHS" = x; then
  cups_conf_datadir="`$CUPS_CONFIG --datadir`"
  cups_conf_serverroot="`$CUPS_CONFIG --serverroot`"
  cups_conf_serverbin="`$CUPS_CONFIG --serverbin`"
else
  cups_conf_datadir="${cups_prefix}/share/cups"
  cups_conf_serverroot="${cups_prefix}/etc/cups"
  cups_conf_serverbin="${cups_exec_prefix}/lib/cups"
fi

# And the rest!
cups_bindir="${cups_exec_prefix}/bin"
cups_sbindir="${cups_exec_prefix}/sbin"

])
