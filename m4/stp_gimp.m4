# This file is part of GIMP-Print.                     -*- Autoconf -*-
# GIMP support.
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
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.


## Table of Contents:
## 1. GIMP source tree support
## 2. GIMP library checks


## --------------------------- ##
## 1. GIMP source tree support ##
## --------------------------- ##


# STP_GIMP_EXCLUDE [(EXCLUDE [,INCLUDE])]
# ---------------------------------------
AC_DEFUN([STP_GIMP_EXCLUDE],
[dnl exclude all quoted text if building in the GIMP tree
m4_if(GIMP_SOURCE_TREE, [no], [$1], [$2])dnl
])dnl




## ---------------------- ##
## 2. GIMP library checks ##
## ---------------------- ##


# STP_GIMP_LIBS
# -------------
# GIMP library checks
AC_DEFUN([STP_GIMP_LIBS],
[dnl GIMP library checks
if test x${BUILD_GIMP} = xyes -o -n "${BUILD_PACKAGE}" ; then
  if test x$GIMP_SOURCE_TREE_SUBDIR = xyes ; then
    AM_PATH_GTK
    AM_PATH_GLIB
    GIMP_CFLAGS="-I\$(top_srcdir)/../.. \$(GTK_CFLAGS) \$(GLIB_CFLAGS)"
    GIMP_LIBS="\$(GTK_LIBS) \$(GLIB_LIBS) \$(top_builddir)/../../libgimp/libgimp.la  \$(top_builddir)/../../libgimp/libgimpui.la"
  else
    AM_PATH_GIMP(1.2.0,
                 [SAVE_GTK_LIBS="$GIMP_LIBS"
                  SAVE_GTK_CFLAGS="$GIMP_CFLAGS"],
                  AC_MSG_ERROR(Cannot find GIMP libraries: Please run ldconfig as root, make sure gimptool is on your PATH, and if applicable ensure that you have the GIMP, GTK, and GLIB development packages installed.))
  fi
fi

])
