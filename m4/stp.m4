# This file is part of GIMP-Print.                     -*- Autoconf -*-
# GIMP-Print Autoconf support.
# Copyright 2001-2002 Roger Leigh
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
## 1. Autoconf support
## 2. gimpprint.m4


## ---------------- ##
## Autoconf support ##
## ---------------- ##


# STP_INIT
# --------
# Initialise GIMP-Print autoconf macros.  Turn on error detection of
# STP_* macros
AC_DEFUN([STP_INIT],
[m4_pattern_forbid([STP_])])




## ---------------- ##
## gimpprint.m4     ##
## ---------------- ##


# STP_CONFIG_GIMPPRINT_M4
# -----------------------
# Generate a specific gimpprint.m4 from stp_path_lib.m4
AC_DEFUN(STP_CONFIG_GIMPPRINT_M4,
[dnl Create src/main/gimpprint.m4 from template
  AC_MSG_NOTICE([creating src/main/gimpprint.m4])
  cat ${srcdir}/src/main/gimpprint.m4.top > src/main/gimpprint.m4
  m4_pattern_allow([STP_PATH_LIB])
  sed -e "s/[STP_PATH_LIB]/[_AM_PATH_GIMPPRINT]/g" < ${srcdir}/m4/stp_path_lib.m4 >> src/main/gimpprint.m4
])
