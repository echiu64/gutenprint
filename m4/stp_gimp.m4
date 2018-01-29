# This file is part of Gutenprint.                     -*- Autoconf -*-
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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# 02111-1307, USA.


## Table of Contents:
## 1. GIMP library checks
## 2. gimptool support


## ---------------------- ##
## 1. GIMP library checks ##
## ---------------------- ##


# STP_GIMP2_LIBS
# --------------
# GIMP library checks
AC_DEFUN([STP_GIMP2_LIBS],
[dnl GIMP library checks
if test x${BUILD_GIMP2} = xyes ; then
  GIMP2_DATA_DIR="`$PKG_CONFIG gimp-2.0 --variable=gimpdatadir`"
  GIMP2_PLUGIN_DIR="`$PKG_CONFIG gimp-2.0 --variable=gimplibdir`/plug-ins"
fi
])
