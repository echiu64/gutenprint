/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
 *
 *   Copyright 2001 Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print.h>
#include <gimp-print-internal.h>

static const unsigned short mat_1_1[] =
{
#include "quickmatrix257.h"
};

const stp_dither_matrix_short_t stp_1_1_matrix =
{
  257, 257, 2, 1, mat_1_1
};

static const unsigned short mat_2_1[] =
{
#include "ran.367.179.h"
};

const stp_dither_matrix_short_t stp_2_1_matrix =
{
  367, 179, 2, 1, mat_2_1
};

static const unsigned short mat_4_1[] =
{
#include "ran.509.131.h"
};

const stp_dither_matrix_short_t stp_4_1_matrix =
{
  509, 131, 2, 1, mat_4_1
};
