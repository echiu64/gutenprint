/*
 * "$Id$"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include "print-escp2.h"

static const stp_simple_dither_range_t photo_cyan_dither_ranges[] =
{
  { 0.27, 0x1, 1, 1 },
  { 1.0,  0x1, 0, 1 }
};

static const escp2_variable_ink_t photo_cyan_ink =
{
  photo_cyan_dither_ranges,
  sizeof(photo_cyan_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1
};

static const stp_simple_dither_range_t photo_magenta_dither_ranges[] =
{
  { 0.35, 0x1, 1, 1 },
  { 1.0,  0x1, 0, 1 }
};

static const escp2_variable_ink_t photo_magenta_ink =
{
  photo_magenta_dither_ranges,
  sizeof(photo_magenta_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1
};


static const stp_simple_dither_range_t photo_6pl_dither_ranges[] =
{
  { 0.065, 0x1, 1, 1 },
  { 0.13,  0x2, 1, 2 },
/* { 0.26, 0x3, 1, 3 }, */
  { 0.25,  0x1, 0, 1 },
  { 0.5,   0x2, 0, 2 },
  { 1.0,   0x3, 0, 4 }
};

static const escp2_variable_ink_t photo_6pl_ink =
{
  photo_6pl_dither_ranges,
  sizeof(photo_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t photo_6pl_1440_dither_ranges[] =
{
  { 0.13,  0x1, 1, 1 },
  { 0.26,  0x2, 1, 2 },
/* { 0.52, 0x3, 1, 3 }, */
  { 0.5,   0x1, 0, 1 },
  { 1.0,   0x2, 0, 2 },
};

static const escp2_variable_ink_t photo_6pl_1440_ink =
{
  photo_6pl_1440_dither_ranges,
  sizeof(photo_6pl_1440_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t photo_pigment_dither_ranges[] =
{ /* MRS: Not calibrated! */
  { 0.15,  0x1, 1, 1 },
  { 0.227, 0x2, 1, 2 },
  { 0.5,   0x1, 0, 1 },
  { 1.0,   0x2, 0, 2 }
};

static const escp2_variable_ink_t photo_pigment_ink =
{
  photo_pigment_dither_ranges,
  sizeof(photo_pigment_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t photo_4pl_dither_ranges[] =
{
  { 0.17,  0x1, 1, 2 },
  { 0.26,  0x2, 1, 3 },
  { 0.661, 0x1, 0, 2 },
  { 1.00,  0x2, 0, 3 }
};

static const escp2_variable_ink_t photo_4pl_ink =
{
  photo_4pl_dither_ranges,
  sizeof(photo_4pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t spro10000_photo_dither_ranges[] =
{
  { 0.17,  0x1, 1, 2 },
  { 0.26,  0x2, 1, 3 },
  { 0.661, 0x1, 0, 2 },
  { 1.00,  0x2, 0, 3 }
};

static const escp2_variable_ink_t spro10000_photo_ink =
{
  spro10000_photo_dither_ranges,
  sizeof(spro10000_photo_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t spro10000_standard_dither_ranges[] =
{
  { 0.661, 0x1, 0, 2 },
  { 1.00,  0x2, 0, 3 }
};

static const escp2_variable_ink_t spro10000_standard_ink =
{
  spro10000_standard_dither_ranges,
  sizeof(spro10000_standard_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t photo_4pl_2880_dither_ranges[] =
{
  { 0.35,  0x1, 1, 1 },
  { 1.00,  0x1, 0, 3 },
};

static const escp2_variable_ink_t photo_4pl_2880_ink =
{
  photo_4pl_2880_dither_ranges,
  sizeof(photo_4pl_2880_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_680_6pl_dither_ranges[] =
{
  { 0.50,  0x1, 0, 3 },
  { 0.66,  0x2, 0, 4 },
  { 1.0,   0x3, 0, 6 }
};

static const escp2_variable_ink_t standard_680_6pl_ink =
{
  standard_680_6pl_dither_ranges,
  sizeof(standard_680_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_6pl_dither_ranges[] =
{
  { 0.25,  0x1, 0, 1 },
  { 0.5,   0x2, 0, 2 },
  { 1.0,   0x3, 0, 4 }
};

static const escp2_variable_ink_t standard_6pl_ink =
{
  standard_6pl_dither_ranges,
  sizeof(standard_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_x80_6pl_dither_ranges[] =
{
  { 0.325, 0x1, 0, 2 },
  { 0.5,   0x2, 0, 3 },
  { 1.0,   0x3, 0, 6 }
};

static const escp2_variable_ink_t standard_x80_6pl_ink =
{
  standard_x80_6pl_dither_ranges,
  sizeof(standard_x80_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t standard_x80_multishot_dither_ranges[] =
{
  { 0.163, 0x1, 0, 1 },
  { 0.5,   0x2, 0, 3 },
  { 1.0,   0x3, 0, 6 }
};

static const escp2_variable_ink_t standard_x80_multishot_ink =
{
  standard_x80_multishot_dither_ranges,
  sizeof(standard_x80_multishot_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t standard_x80_1440_6pl_dither_ranges[] =
{
  { 0.65,  0x1, 0, 2 },
  { 1.0,   0x2, 0, 3 },
};

static const escp2_variable_ink_t standard_x80_1440_6pl_ink =
{
  standard_x80_1440_6pl_dither_ranges,
  sizeof(standard_x80_1440_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_980_6pl_dither_ranges[] =
{
  { 0.40,  0x1, 0, 4 },
  { 0.675, 0x2, 0, 7 },
  { 1.0,   0x3, 0, 10 }
};

static const escp2_variable_ink_t standard_980_6pl_ink =
{
  standard_980_6pl_dither_ranges,
  sizeof(standard_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_6pl_1440_dither_ranges[] =
{
  { 0.5,   0x1, 0, 1 },
  { 1.0,   0x2, 0, 2 },
};

static const escp2_variable_ink_t standard_6pl_1440_ink =
{
  standard_6pl_1440_dither_ranges,
  sizeof(standard_6pl_1440_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_pigment_dither_ranges[] =
{ /* MRS: Not calibrated! */
  { 0.55,  0x1, 0, 1 },
  { 1.0,   0x2, 0, 2 }
};

static const escp2_variable_ink_t standard_pigment_ink =
{
  standard_pigment_dither_ranges,
  sizeof(standard_pigment_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_4pl_dither_ranges[] =
{
  { 0.661, 0x1, 0, 2 },
  { 1.00,  0x2, 0, 3 }
};

static const escp2_variable_ink_t standard_4pl_ink =
{
  standard_4pl_dither_ranges,
  sizeof(standard_4pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t standard_4pl_2880_dither_ranges[] =
{
  { 1.00,  0x1, 0, 1 },
};

static const escp2_variable_ink_t standard_4pl_2880_ink =
{
  standard_4pl_2880_dither_ranges,
  sizeof(standard_4pl_2880_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_3pl_dither_ranges[] =
{
  { 0.25,  0x1, 0, 2 },
  { 0.61,  0x2, 0, 5 },
  { 1.0,   0x3, 0, 8 }
};

static const escp2_variable_ink_t standard_3pl_ink =
{
  standard_3pl_dither_ranges,
  sizeof(standard_3pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_3pl_1440_dither_ranges[] =
{
  { 0.39, 0x1, 0, 2 },
  { 1.0,  0x2, 0, 5 }
};

static const escp2_variable_ink_t standard_3pl_1440_ink =
{
  standard_3pl_1440_dither_ranges,
  sizeof(standard_3pl_1440_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_3pl_2880_dither_ranges[] =
{
  { 1.0,   0x1, 0, 1 }
};

static const escp2_variable_ink_t standard_3pl_2880_ink =
{
  standard_3pl_2880_dither_ranges,
  sizeof(standard_3pl_2880_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_economy_pigment_dither_ranges[] =
{
  { 1.0,   0x3, 0, 3 }
};

static const escp2_variable_ink_t standard_economy_pigment_ink =
{
  standard_economy_pigment_dither_ranges,
  sizeof(standard_economy_pigment_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t standard_multishot_pigment_dither_ranges[] =
{
  { 0.410, 0x1, 0, 2 },
  { 1.0,   0x3, 0, 5 }
};

static const escp2_variable_ink_t standard_multishot_pigment_ink =
{
  standard_multishot_pigment_dither_ranges,
  sizeof(standard_multishot_pigment_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t standard_6pl_pigment_dither_ranges[] =
{
  { 0.300, 0x1, 0, 3 },
  { 1.0,   0x3, 0, 10 }
};

static const escp2_variable_ink_t standard_6pl_pigment_ink =
{
  standard_6pl_pigment_dither_ranges,
  sizeof(standard_6pl_pigment_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t standard_3pl_pigment_dither_ranges[] =
{
  { 0.650, 0x1, 0, 2 },
  { 1.000, 0x2, 0, 3 },
};

static const escp2_variable_ink_t standard_3pl_pigment_ink =
{
  standard_3pl_pigment_dither_ranges,
  sizeof(standard_3pl_pigment_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_3pl_pigment_2880_dither_ranges[] =
{
  { 1.0,   0x1, 0, 1 }
};

static const escp2_variable_ink_t standard_3pl_pigment_2880_ink =
{
  standard_3pl_pigment_2880_dither_ranges,
  sizeof(standard_3pl_pigment_2880_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t photo_multishot_dither_ranges[] =
{
  { 0.0728, 0x1, 1, 1 },
  { 0.151,  0x2, 1, 2 },
  { 0.26,   0x3, 1, 3 },
  { 1.0,    0x3, 0, 3 }
};

static const escp2_variable_ink_t photo_multishot_ink =
{
  photo_multishot_dither_ranges,
  sizeof(photo_multishot_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_multishot_dither_ranges[] =
{
  { 0.28,  0x1, 0, 2 },
  { 0.58,  0x2, 0, 5 },
  { 1.0,   0x3, 0, 8 }
};

static const escp2_variable_ink_t standard_multishot_ink =
{
  standard_multishot_dither_ranges,
  sizeof(standard_multishot_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const escp2_variable_inkset_t standard_inks =
{
  NULL,
  NULL,
  NULL,
  NULL
};

static const escp2_variable_inkset_t photo_inks =
{
  NULL,
  &photo_cyan_ink,
  &photo_magenta_ink,
  NULL
};

static const escp2_variable_inkset_t escp2_680_6pl_standard_inks =
{
  &standard_680_6pl_ink,
  &standard_680_6pl_ink,
  &standard_680_6pl_ink,
  &standard_680_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_standard_inks =
{
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink
};

static const escp2_variable_inkset_t escp2_x80_multishot_standard_inks =
{
  &standard_x80_multishot_ink,
  &standard_x80_multishot_ink,
  &standard_x80_multishot_ink,
  &standard_x80_multishot_ink
};

static const escp2_variable_inkset_t escp2_x80_6pl_standard_inks =
{
  &standard_x80_6pl_ink,
  &standard_x80_6pl_ink,
  &standard_x80_6pl_ink,
  &standard_x80_6pl_ink
};

static const escp2_variable_inkset_t escp2_x80_1440_6pl_standard_inks =
{
  &standard_x80_1440_6pl_ink,
  &standard_x80_1440_6pl_ink,
  &standard_x80_1440_6pl_ink,
  &standard_x80_1440_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_standard_980_inks =
{
  &standard_980_6pl_ink,
  &standard_980_6pl_ink,
  &standard_980_6pl_ink,
  &standard_980_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_photo_inks =
{
  &standard_6pl_ink,
  &photo_6pl_ink,
  &photo_6pl_ink,
  &standard_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_1440_standard_inks =
{
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink
};

static const escp2_variable_inkset_t escp2_6pl_1440_photo_inks =
{
  &standard_6pl_1440_ink,
  &photo_6pl_1440_ink,
  &photo_6pl_1440_ink,
  &standard_6pl_1440_ink
};

static const escp2_variable_inkset_t escp2_pigment_standard_inks =
{
  &standard_pigment_ink,
  &standard_pigment_ink,
  &standard_pigment_ink,
  &standard_pigment_ink
};

static const escp2_variable_inkset_t escp2_pigment_photo_inks =
{
  &standard_pigment_ink,
  &photo_pigment_ink,
  &photo_pigment_ink,
  &standard_pigment_ink
};

static const escp2_variable_inkset_t escp2_4pl_standard_inks =
{
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink
};

static const escp2_variable_inkset_t escp2_4pl_photo_inks =
{
  &standard_4pl_ink,
  &photo_4pl_ink,
  &photo_4pl_ink,
  &standard_4pl_ink
};

static const escp2_variable_inkset_t spro10000_standard_inks =
{
  &spro10000_standard_ink,
  &spro10000_standard_ink,
  &spro10000_standard_ink,
  &spro10000_standard_ink
};

static const escp2_variable_inkset_t spro10000_photo_inks =
{
  &spro10000_standard_ink,
  &spro10000_photo_ink,
  &spro10000_photo_ink,
  &spro10000_standard_ink
};

static const escp2_variable_inkset_t escp2_4pl_2880_standard_inks =
{
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink
};

static const escp2_variable_inkset_t escp2_4pl_2880_photo_inks =
{
  &standard_4pl_2880_ink,
  &photo_4pl_2880_ink,
  &photo_4pl_2880_ink,
  &standard_4pl_2880_ink
};

static const escp2_variable_inkset_t escp2_3pl_standard_inks =
{
  &standard_3pl_ink,
  &standard_3pl_ink,
  &standard_3pl_ink,
  &standard_3pl_ink
};

static const escp2_variable_inkset_t escp2_3pl_1440_standard_inks =
{
  &standard_3pl_1440_ink,
  &standard_3pl_1440_ink,
  &standard_3pl_1440_ink,
  &standard_3pl_1440_ink
};

static const escp2_variable_inkset_t escp2_3pl_2880_standard_inks =
{
  &standard_3pl_2880_ink,
  &standard_3pl_2880_ink,
  &standard_3pl_2880_ink,
  &standard_3pl_2880_ink
};

static const escp2_variable_inkset_t escp2_multishot_standard_inks =
{
  &standard_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink
};

static const escp2_variable_inkset_t escp2_multishot_photo_inks =
{
  &standard_multishot_ink,
  &photo_multishot_ink,
  &photo_multishot_ink,
  &standard_multishot_ink
};

static const escp2_variable_inkset_t escp2_economy_pigment_standard_inks =
{
  &standard_economy_pigment_ink,
  &standard_economy_pigment_ink,
  &standard_economy_pigment_ink,
  &standard_economy_pigment_ink
};

static const escp2_variable_inkset_t escp2_multishot_pigment_standard_inks =
{
  &standard_multishot_pigment_ink,
  &standard_multishot_pigment_ink,
  &standard_multishot_pigment_ink,
  &standard_multishot_pigment_ink
};

static const escp2_variable_inkset_t escp2_6pl_pigment_standard_inks =
{
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink
};

static const escp2_variable_inkset_t escp2_3pl_pigment_standard_inks =
{
  &standard_3pl_pigment_ink,
  &standard_3pl_pigment_ink,
  &standard_3pl_pigment_ink,
  &standard_3pl_pigment_ink
};

static const escp2_variable_inkset_t escp2_3pl_pigment_2880_standard_inks =
{
  &standard_3pl_pigment_2880_ink,
  &standard_3pl_pigment_2880_ink,
  &standard_3pl_pigment_2880_ink,
  &standard_3pl_pigment_2880_ink
};


static const escp2_variable_inklist_t simple_4color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks
    },
  },
};

static const escp2_variable_inklist_t simple_6color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
    },
    {
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
    }
  }
};

static const escp2_variable_inklist_t variable_6pl_4color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
    }
  },
  {
    {
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_6pl_1440_standard_inks,
      &escp2_6pl_1440_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks,
    }
  }
};

static const escp2_variable_inklist_t variable_x80_6pl_4color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
    }
  },
  {
    {
      &escp2_x80_multishot_standard_inks,
      &escp2_x80_multishot_standard_inks,
      &escp2_x80_multishot_standard_inks,
      &escp2_x80_multishot_standard_inks,
      &escp2_x80_6pl_standard_inks,
      &escp2_x80_1440_6pl_standard_inks,
      &escp2_x80_1440_6pl_standard_inks,
      &escp2_x80_1440_6pl_standard_inks,
      &escp2_x80_1440_6pl_standard_inks,
    }
  }
};

static const escp2_variable_inklist_t variable_6pl_6color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks
    },
    {
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks
    }
  },
  {
    {
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_6pl_1440_standard_inks,
      &escp2_6pl_1440_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks
    },
    {
      &escp2_6pl_photo_inks,
      &escp2_6pl_photo_inks,
      &escp2_6pl_photo_inks,
      &escp2_6pl_photo_inks,
      &escp2_6pl_photo_inks,
      &escp2_6pl_1440_photo_inks,
      &escp2_6pl_1440_photo_inks,
      &escp2_6pl_photo_inks,
      &escp2_6pl_photo_inks
    }
  }
};

static const escp2_variable_inklist_t variable_pigment_6color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks
    },
    {
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks
    }
  },
  {
    {
      &escp2_pigment_standard_inks,
      &escp2_pigment_standard_inks,
      &escp2_pigment_standard_inks,
      &escp2_pigment_standard_inks,
      &escp2_pigment_standard_inks,
      &escp2_pigment_standard_inks,
      &escp2_pigment_standard_inks,
      &escp2_pigment_standard_inks,
      &escp2_pigment_standard_inks
    },
    {
      &escp2_pigment_photo_inks,
      &escp2_pigment_photo_inks,
      &escp2_pigment_photo_inks,
      &escp2_pigment_photo_inks,
      &escp2_pigment_photo_inks,
      &escp2_pigment_photo_inks,
      &escp2_pigment_photo_inks,
      &escp2_pigment_photo_inks,
      &escp2_pigment_photo_inks
    }
  }
};

static const escp2_variable_inklist_t spro10000_inks =
{
  {
    {
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks
    },
    {
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks
    }
  },
  {
    {
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks,
      &spro10000_standard_inks
    },
    {
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks,
      &spro10000_photo_inks
    }
  }
};

static const escp2_variable_inklist_t variable_3pl_4color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
    }
  },
  {
    {
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_6pl_standard_980_inks,
      &escp2_6pl_standard_980_inks,
      &escp2_3pl_standard_inks,
      &escp2_3pl_1440_standard_inks,
      &escp2_3pl_1440_standard_inks,
      &escp2_3pl_2880_standard_inks,
      &escp2_3pl_2880_standard_inks,
    }
  }
};

static const escp2_variable_inklist_t variable_3pl_pigment_4color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
    }
  },
  {
    {
      &escp2_economy_pigment_standard_inks,
      &escp2_economy_pigment_standard_inks,
      &escp2_multishot_pigment_standard_inks,
      &escp2_multishot_pigment_standard_inks,
      &escp2_6pl_pigment_standard_inks,
      &escp2_3pl_pigment_standard_inks,
      &escp2_3pl_pigment_2880_standard_inks,
      &escp2_3pl_pigment_2880_standard_inks,
      &escp2_3pl_pigment_2880_standard_inks,
    }
  }
};

static const escp2_variable_inklist_t variable_680_4pl_4color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
    }
  },
  {
    {
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_680_6pl_standard_inks,
      &escp2_4pl_standard_inks,
      &escp2_4pl_2880_standard_inks,
      &escp2_4pl_2880_standard_inks,
      &escp2_4pl_2880_standard_inks,
    }
  }
};

static const escp2_variable_inklist_t variable_4pl_4color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
    }
  },
  {
    {
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_4pl_standard_inks,
      &escp2_4pl_2880_standard_inks,
      &escp2_4pl_2880_standard_inks,
      &escp2_4pl_2880_standard_inks,
    }
  }
};

static const escp2_variable_inklist_t variable_4pl_6color_inks =
{
  {
    {
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks,
      &standard_inks
    },
    {
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks,
      &photo_inks
    }
  },
  {
    {
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_4pl_standard_inks,
      &escp2_4pl_2880_standard_inks,
      &escp2_4pl_2880_standard_inks,
      &escp2_4pl_2880_standard_inks,
    },
    {
      &escp2_multishot_photo_inks,
      &escp2_multishot_photo_inks,
      &escp2_multishot_photo_inks,
      &escp2_multishot_photo_inks,
      &escp2_6pl_photo_inks,
      &escp2_4pl_photo_inks,
      &escp2_4pl_2880_photo_inks,
      &escp2_4pl_2880_photo_inks,
      &escp2_4pl_2880_photo_inks
    }
  }
};

static const double standard_sat_adjustment[49] =
{
  1.0,				/* C */
  1.1,
  1.2,
  1.3,
  1.4,
  1.5,
  1.6,
  1.7,
  1.8,				/* B */
  1.9,
  1.9,
  1.9,
  1.7,
  1.5,
  1.3,
  1.1,
  1.0,				/* M */
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,				/* R */
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,				/* Y */
  1.0,
  1.0,
  1.1,
  1.2,
  1.3,
  1.4,
  1.5,
  1.5,				/* G */
  1.4,
  1.3,
  1.2,
  1.1,
  1.0,
  1.0,
  1.0,
  1.0				/* C */
};

static const double standard_lum_adjustment[49] =
{
  0.50,				/* C */
  0.6,
  0.7,
  0.8,
  0.9,
  0.86,
  0.82,
  0.79,
  0.78,				/* B */
  0.8,
  0.83,
  0.87,
  0.9,
  0.95,
  1.05,
  1.15,
  1.3,				/* M */
  1.25,
  1.2,
  1.15,
  1.12,
  1.09,
  1.06,
  1.03,
  1.0,				/* R */
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,				/* Y */
  0.9,
  0.8,
  0.7,
  0.65,
  0.6,
  0.55,
  0.52,
  0.48,				/* G */
  0.47,
  0.47,
  0.49,
  0.49,
  0.49,
  0.52,
  0.51,
  0.50				/* C */
};

static const double standard_hue_adjustment[49] =
{
  0.00,				/* C */
  0.05,
  0.04,
  0.01,
  -0.03,
  -0.10,
  -0.18,
  -0.26,
  -0.35,			/* B */
  -0.43,
  -0.40,
  -0.32,
  -0.25,
  -0.18,
  -0.10,
  -0.07,
  0.00,				/* M */
  -0.04,
  -0.09,
  -0.13,
  -0.18,
  -0.23,
  -0.27,
  -0.31,
  -0.35,			/* R */
  -0.38,
  -0.30,
  -0.23,
  -0.15,
  -0.08,
  0.00,
  -0.02,
  0.00,				/* Y */
  0.08,
  0.10,
  0.08,
  0.05,
  0.03,
  -0.03,
  -0.12,
  -0.20,			/* G */
  -0.17,
  -0.20,
  -0.17,
  -0.15,
  -0.12,
  -0.10,
  -0.08,
  0.00,				/* C */
};

static const double plain_paper_lum_adjustment[49] =
{
  1.2,				/* C */
  1.22,
  1.28,
  1.34,
  1.39,
  1.42,
  1.45,
  1.48,
  1.5,				/* B */
  1.4,
  1.3,
  1.25,
  1.2,
  1.1,
  1.05,
  1.05,
  1.05,				/* M */
  1.05,
  1.05,
  1.05,
  1.05,
  1.05,
  1.05,
  1.05,
  1.05,				/* R */
  1.05,
  1.05,
  1.1,
  1.1,
  1.1,
  1.1,
  1.1,
  1.1,				/* Y */
  1.15,
  1.3,
  1.45,
  1.6,
  1.75,
  1.9,
  2.0,
  2.1,				/* G */
  2.0,
  1.8,
  1.7,
  1.6,
  1.5,
  1.4,
  1.3,
  1.2				/* C */
};

static const double pgpp_sat_adjustment[49] =
{
  1.00,				/* C */
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,				/* B */
  1.00,
  1.00,
  1.03,
  1.05,
  1.07,
  1.09,
  1.11,
  1.13,				/* M */
  1.13,
  1.13,
  1.13,
  1.13,
  1.13,
  1.13,
  1.13,
  1.13,				/* R */
  1.10,
  1.05,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,				/* Y */
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,				/* G */
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,				/* C */
};

static const double pgpp_lum_adjustment[49] =
{
  1.00,				/* C */
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,				/* B */
  1.00,
  1.00,
  1.03,
  1.05,
  1.07,
  1.09,
  1.11,
  1.13,				/* M */
  1.13,
  1.13,
  1.13,
  1.13,
  1.13,
  1.13,
  1.13,
  1.13,				/* R */
  1.10,
  1.05,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,				/* Y */
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,				/* G */
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,
  1.00,				/* C */
};

static const double pgpp_hue_adjustment[49] =
{
  0.00,				/* C */
  0.00,
  0.00,
  0.00,
  0.00,
  0.01,
  0.02,
  0.03,
  0.05,				/* B */
  0.05,
  0.05,
  0.04,
  0.04,
  0.03,
  0.02,
  0.01,
  0.00,				/* M */
  -.03,
  -.05,
  -.07,
  -.09,
  -.11,
  -.13,
  -.14,
  -.15,				/* R */
  -.13,
  -.10,
  -.06,
  -.04,
  -.02,
  -.01,
  0.00,
  0.00,				/* Y */
  0.00,
  0.00,
  0.00,
  0.00,
  0.00,
  0.00,
  0.00,
  0.00,				/* G */
  0.00,
  0.00,
  0.00,
  0.00,
  0.00,
  0.00,
  0.00,
  0.00,				/* C */
};

static const paper_t standard_papers[] =
{
  { "Plain", N_("Plain Paper"),
    1, 0, 0.80, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "PlainFast", N_("Plain Paper Fast Load"),
    5, 0, 0.80, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Postcard", N_("Postcard"),
    2, 0, 0.83, .2, .6, 1.0, 1.0, 1.0, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "GlossyFilm", N_("Glossy Film"),
    3, 0, 1.00 ,1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Transparency", N_("Transparencies"),
    3, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6d, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "Envelope", N_("Envelopes"),
    4, 0, 0.80, .125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "BackFilm", N_("Back Light Film"),
    6, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, NULL, NULL},
  { "Matte", N_("Matte Paper"),
    7, 0, 0.85, 1.0, .999, 1.05, .9, 1.05, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, NULL, NULL},
  { "Inkjet", N_("Inkjet Paper"),
    7, 0, 0.85, .25, .6, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Coated", N_("Photo Quality Inkjet Paper"),
    7, 0, 1.00, 1.0, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Photo", N_("Photo Paper"),
    8, 0, 1.00, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"),
    8, 0, 1.10, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.03, 1.0,
    1, 1.0, 0x80, 0x00, 0x02,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment},
  { "Luster", N_("Premium Luster Photo Paper"),
    8, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPaper", N_("Photo Quality Glossy Paper"),
    6, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Ilford", N_("Ilford Heavy Paper"),
    8, 0, .85, .5, 1.35, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL },
  { "Other", N_("Other"),
    0, 0, 0.80, 0.125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
};

static const paperlist_t standard_paper_list =
{
  sizeof(standard_papers) / sizeof(paper_t),
  standard_papers
};

static const paper_t sp780_papers[] =
{
  { "Plain", N_("Plain Paper"),
    6, 0, 0.80, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "PlainFast", N_("Plain Paper Fast Load"),
    1, 0, 0.80, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Postcard", N_("Postcard"),
    3, 0, 0.83, .2, .6, 1.0, 1.0, 1.0, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "GlossyFilm", N_("Glossy Film"),
    0, 0, 1.00 ,1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Transparency", N_("Transparencies"),
    0, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6d, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "Envelope", N_("Envelopes"),
    4, 0, 0.80, .125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "BackFilm", N_("Back Light Film"),
    0, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, NULL, NULL},
  { "Matte", N_("Matte Paper"),
    2, 0, 0.85, 1.0, .999, 1.05, .9, 1.05, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, NULL, NULL},
  { "Inkjet", N_("Inkjet Paper"),
    6, 0, 0.85, .25, .6, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Coated", N_("Photo Quality Inkjet Paper"),
    0, 0, 1.00, 1.0, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Photo", N_("Photo Paper"),
    2, 0, 1.00, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"),
    7, 0, 1.10, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.03, 1.0,
    1, 1.0, 0x80, 0x00, 0x02,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment},
  { "Luster", N_("Premium Luster Photo Paper"),
    7, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPaper", N_("Photo Quality Glossy Paper"),
    0, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Ilford", N_("Ilford Heavy Paper"),
    2, 0, .85, .5, 1.35, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL },
  { "Other", N_("Other"),
    0, 0, 0.80, 0.125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
};

static const paperlist_t sp780_paper_list =
{
  sizeof(sp780_papers) / sizeof(paper_t),
  sp780_papers
};

static const paper_t c80_papers[] =
{
  { "Plain", N_("Plain Paper"),
    1, 0, 0.80, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "PlainFast", N_("Plain Paper Fast Load"),
    5, 0, 0.80, .1, .5, 1.0, 1.0, 1.0, .9, 1.05, 1.15,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Postcard", N_("Postcard"),
    2, 0, 0.83, .2, .6, 1.0, 1.0, 1.0, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "GlossyFilm", N_("Glossy Film"),
    3, 0, 1.00 ,1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Transparency", N_("Transparencies"),
    3, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6d, 0x00, 0x02, NULL, plain_paper_lum_adjustment, NULL},
  { "Envelope", N_("Envelopes"),
    4, 0, 0.80, .125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "BackFilm", N_("Back Light Film"),
    6, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6d, 0x00, 0x01, NULL, NULL, NULL},
  { "Matte", N_("Matte Paper"),
    7, 0, 0.9, 1.0, .999, 1.0, 1.0, 1.0, .9, 1.0, 1.1,
    1, 1.0, 0x00, 0x00, 0x02, NULL, NULL, NULL},
  { "Inkjet", N_("Inkjet Paper"),
    7, 0, 0.85, .25, .6, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
  { "Coated", N_("Photo Quality Inkjet Paper"),
    7, 0, 1.00, 1.0, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Photo", N_("Photo Paper"),
    8, 0, 1.20, 1.0, .9, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x67, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPhoto", N_("Premium Glossy Photo Paper"),
    8, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.03, 1.0,
    1, 1.0, 0x80, 0x00, 0x02,
    pgpp_hue_adjustment, pgpp_lum_adjustment, pgpp_sat_adjustment},
  { "Luster", N_("Premium Luster Photo Paper"),
    8, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL},
  { "GlossyPaper", N_("Photo Quality Glossy Paper"),
    6, 0, 1.00, 1, .999, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1.0, 1.0, 0x6b, 0x1a, 0x01, NULL, NULL, NULL},
  { "Ilford", N_("Ilford Heavy Paper"),
    8, 0, .85, .5, 1.35, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x80, 0x00, 0x02, NULL, NULL, NULL },
  { "Other", N_("Other"),
    0, 0, 0.80, 0.125, .5, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
    1, 1.0, 0x6b, 0x1a, 0x01, NULL, plain_paper_lum_adjustment, NULL},
};

static const paperlist_t c80_paper_list =
{
  sizeof(c80_papers) / sizeof(paper_t),
  c80_papers
};

/*
 * Dot sizes are for:
 *
 *  0: 120/180 DPI micro
 *  1: 120/180 DPI soft
 *  2: 360 micro
 *  3: 360 soft
 *  4: 720x360 micro
 *  5: 720x360 soft
 *  6: 720 micro
 *  7: 720 soft
 *  8: 1440x720 micro
 *  9: 1440x720 soft
 * 10: 2880x720 micro
 * 11: 2880x720 soft
 * 12: 2880x1440
 */

/*   0     1     2     3     4     5     6     7     8     9    10    11    12 */

static const escp2_dot_size_t g1_dotsizes =
{ -2,     -1,   -2,   -1,   -1,   -2,   -2,   -1,   -1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t g2_dotsizes =
{ -2,     -1,   -2,   -1,   -2,   -2,   -2,   -2,   -1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sc600_dotsizes =
{  4,     -1,    4,   -1,   -1,    3,    2,    2,   -1,    1,   -1,   -1,   -1 };

static const escp2_dot_size_t g3_dotsizes =
{  3,     -1,    3,   -1,   -1,    2,    1,    1,   -1,    4,   -1,   -1,   -1 };

static const escp2_dot_size_t photo_dotsizes =
{  3,     -1,    3,   -1,   -1,    2,   -1,    1,   -1,    4,   -1,   -1,   -1 };

static const escp2_dot_size_t sc440_dotsizes =
{  3,     -1,    3,   -1,   -1,    2,   -1,    1,   -1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sc640_dotsizes =
{  3,     -1,    3,   -1,   -1,    2,    1,    1,   -1,    1,   -1,   -1,   -1 };

static const escp2_dot_size_t c6pl_dotsizes =
{ -1,   0x10,   -1, 0x10,   -1, 0x10,   -1, 0x10,   -1, 0x10,   -1,   -1,   -1 };

static const escp2_dot_size_t c3pl_dotsizes =
{ -1,   0x11,   -1, 0x11,   -1, 0x11,   -1, 0x10,   -1, 0x10,   -1, 0x10, 0x10 };

static const escp2_dot_size_t c4pl_dotsizes =
{ -1,   0x12,   -1, 0x12,   -1, 0x12,   -1, 0x11,   -1, 0x10,   -1, 0x10, 0x10 };

static const escp2_dot_size_t sc720_dotsizes =
{ -1,   0x12,   -1, 0x12,   -1, 0x11,   -1, 0x11,   -1, 0x11,   -1,   -1,   -1 };

static const escp2_dot_size_t sc660_dotsizes =
{ -1,      3,    3,   -1,    3,    0,   -1,    0,   -1,    0,   -1,   -1,   -1 };

static const escp2_dot_size_t sc480_dotsizes =
{ -1,   0x13,   -1, 0x13,   -1, 0x13,   -1, 0x10,   -1, 0x10,   -1,   -1,   -1 };

static const escp2_dot_size_t sc670_dotsizes =
{ -1,   0x12,   -1, 0x12,   -1, 0x12,   -1, 0x11,   -1, 0x11,   -1,   -1,   -1 };

static const escp2_dot_size_t sp2000_dotsizes =
{ -1,   0x11,   -1, 0x11,   -1, 0x11,   -1, 0x10,   -1, 0x10,   -1,   -1,   -1 };

static const escp2_dot_size_t spro_dotsizes =
{    0,   -1,    0,   -1,    0,   -1,    0,   -1,    0,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t spro10000_dotsizes =
{    4,   -1, 0x11,   -1, 0x11,   -1, 0x10,   -1, 0x10,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t c3pl_pigment_dotsizes =
{   -1, 0x10,   -1, 0x10,   -1, 0x10,   -1, 0x11,   -1, 0x12,   -1, 0x12,  0x12 };

/*
 * Densities are for:
 *
 *  0: 120/180 DPI micro
 *  1: 120/180 DPI soft
 *  2: 360 micro
 *  3: 360 soft
 *  4: 720x360 micro
 *  5: 720x360 soft
 *  6: 720 micro
 *  7: 720 soft
 *  8: 1440x720 micro
 *  9: 1440x720 soft
 * 10: 2880x720 micro
 * 11: 2880x720 soft
 * 12: 2880x1440
 */

/*  0    1    2    3    4     5      6     7      8      9     10     11     12   */

static const escp2_densities_t g1_densities =
{ 2.0, 2.0, 1.3, 1.3, 1.3,  1.3,  0.568, 0.568,   0.0,   0.0,   0.0,   0.0,   0.0 };

static const escp2_densities_t sc1500_densities =
{ 2.0, 2.0, 1.3, 1.3, 1.3,  1.3,  0.631, 0.631,   0.0,   0.0,   0.0,   0.0,   0.0 };

static const escp2_densities_t g3_densities =
{ 2.0, 2.0, 1.3, 1.3, 1.3,  1.3,  0.775, 0.775, 0.55,  0.55,  0.275, 0.275, 0.138 };

static const escp2_densities_t sc440_densities =
{ 3.0, 3.0, 2.0, 2.0, 1.0,  1.0,  0.900, 0.900, 0.45,  0.45,  0.45,  0.45,  0.113 };

static const escp2_densities_t sc480_densities =
{ 2.0, 2.0, 0.0, 1.4, 0.0,  0.7,  0.0,   0.710, 0.0,   0.710, 0.0,   0.355, 0.0   };

static const escp2_densities_t sc980_densities =
{ 2.0, 2.0, 1.3, 1.3, 0.65, 0.65, 0.646, 0.511, 0.49,  0.49,  0.637, 0.637, 0.455 };

static const escp2_densities_t c6pl_densities =
{ 2.0, 2.0, 1.3, 2.0, 0.65, 1.0,  0.646, 0.568, 0.323, 0.568, 0.284, 0.284, 0.142 };

static const escp2_densities_t c3pl_densities =
{ 2.0, 2.0, 1.3, 1.3, 0.65, 0.65, 0.646, 0.73,  0.7,   0.7,   0.91,  0.91,  0.455 };

static const escp2_densities_t sc680_densities =
{ 2.0, 2.0, 1.2, 1.2, 0.60, 0.60, 0.792, 0.792, 0.792, 0.792, 0.594, 0.594, 0.297 };

static const escp2_densities_t c4pl_densities =
{ 2.0, 2.0, 1.3, 1.3, 0.65, 0.65, 0.431, 0.568, 0.784, 0.784, 0.593, 0.593, 0.297 };

static const escp2_densities_t sc660_densities =
{ 3.0, 3.0, 2.0, 2.0, 1.0,  1.0,  0.646, 0.646, 0.323, 0.323, 0.162, 0.162, 0.081 };

static const escp2_densities_t sp2000_densities =
{ 2.0, 2.0, 1.3, 1.3, 0.65, 0.65, 0.775, 0.852, 0.388, 0.438, 0.219, 0.219, 0.110 };

static const escp2_densities_t spro_densities =
{ 2.0, 2.0, 1.3, 1.3, 0.65, 0.65, 0.646, 0.646, 0.323, 0.323, 0.162, 0.162, 0.081 };

static const escp2_densities_t spro10000_densities =
{ 2.0, 2.0, 1.3, 1.3, 0.65, 0.65, 0.431, 0.710, 0.216, 0.784, 0.392, 0.392, 0.196 };

static const escp2_densities_t c3pl_pigment_densities =
{ 2.0, 2.0, 1.3, 1.3, 0.69, 0.69, 0.511, 0.511, 0.765, 0.765, 0.585, 0.585, 0.293 };

/*
 For each printhead (=color), the offset in escp2_base_separation (1/360")
 units is defined here.
 */

static const int default_head_offset[] =
{0, 0, 0, 0, 0, 0, 0};

static const int x80_head_offset[] =
{48, 48, 96, 0, 0, 0, 0};

static const int c80_head_offset[] =
{0, 120, 0, 240, 0, 0, 0};

static const res_t standard_reslist[] =
{
  { "360x90dpi",        N_("360 x 90 DPI Fast Economy Draft"),
    360,  90,   0,  0, 1, 1, 0, 1, 1, RES_120_M },
  { "360x90sw",         N_("360 x 90 DPI Fast Economy Draft"),
    360,  90,   1,  0, 1, 1, 0, 1, 1, RES_120 },

  { "360x120dpi",       N_("360 x 120 DPI Economy Draft"),
    360,  120,  0,  0, 1, 1, 0, 3, 1, RES_120_M },
  { "360x120sw",        N_("360 x 120 DPI Economy Draft"),
    360,  120,  1,  0, 1, 1, 0, 3, 1, RES_120 },

  { "180dpi",           N_("180 DPI Economy Draft"),
    180,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },
  { "180sw",            N_("180 DPI Economy Draft"),
    180,  180,  1,  0, 1, 1, 0, 1, 1, RES_180 },

  { "360x240dpi",       N_("360 x 240 DPI Draft"),
    360,  240,  0,  0, 1, 1, 0, 3, 2, RES_180_M },
  { "360x240sw",        N_("360 x 240 DPI Draft"),
    360,  240,  1,  0, 1, 1, 0, 3, 2, RES_180 },

  { "360x180dpi",       N_("360 x 180 DPI Draft"),
    360,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },
  { "360x180sw",        N_("360 x 180 DPI Draft"),
    360,  180,  1,  0, 1, 1, 0, 1, 1, RES_180 },

  { "360sw",            N_("360 DPI"),
    360,  360,  1,  0, 1, 1, 0, 1, 1, RES_360 },
  { "360swuni",         N_("360 DPI Unidirectional"),
    360,  360,  1,  0, 1, 1, 1, 1, 1, RES_360 },
  { "360mw",            N_("360 DPI Microweave"),
    360,  360,  0,  1, 1, 1, 0, 1, 1, RES_360_M },
  { "360mwuni",         N_("360 DPI Microweave Unidirectional"),
    360,  360,  0,  1, 1, 1, 1, 1, 1, RES_360_M },
  { "360dpi",           N_("360 DPI"),
    360,  360,  0,  0, 1, 1, 0, 1, 1, RES_360_M },
  { "360uni",           N_("360 DPI Unidirectional"),
    360,  360,  0,  0, 1, 1, 1, 1, 1, RES_360_M },

  { "720x360sw",        N_("720 x 360 DPI"),
    720,  360,  1,  0, 1, 1, 0, 2, 1, RES_720_360 },
  { "720x360swuni",     N_("720 x 360 DPI Unidirectional"),
    720,  360,  1,  0, 1, 1, 1, 2, 1, RES_720_360 },

  { "720mw",            N_("720 DPI Microweave"),
    720,  720,  0,  1, 1, 1, 0, 1, 1, RES_720_M },
  { "720mwuni",         N_("720 DPI Microweave Unidirectional"),
    720,  720,  0,  1, 1, 1, 1, 1, 1, RES_720_M },
  { "720sw",            N_("720 DPI"),
    720,  720,  1,  0, 1, 1, 0, 1, 1, RES_720 },
  { "720swuni",         N_("720 DPI Unidirectional"),
    720,  720,  1,  0, 1, 1, 1, 1, 1, RES_720 },
  { "720hq",            N_("720 DPI High Quality"),
    720,  720,  1,  0, 2, 1, 0, 1, 1, RES_720 },
  { "720hquni",         N_("720 DPI High Quality Unidirectional"),
    720,  720,  1,  0, 2, 1, 1, 1, 1, RES_720 },
  { "720hq2",           N_("720 DPI Highest Quality"),
    720,  720,  1,  0, 4, 1, 1, 1, 1, RES_720 },

  { "1440x720mw",       N_("1440 x 720 DPI Microweave"),
    1440, 720,  0,  1, 1, 1, 0, 1, 1, RES_1440_720_M },
  { "1440x720mwuni",    N_("1440 x 720 DPI Microweave Unidirectional"),
    1440, 720,  0,  1, 1, 1, 1, 1, 1, RES_1440_720_M },
  { "1440x720sw",       N_("1440 x 720 DPI"),
    1440, 720,  1,  0, 1, 1, 0, 1, 1, RES_1440_720 },
  { "1440x720swuni",    N_("1440 x 720 DPI Unidirectional"),
    1440, 720,  1,  0, 1, 1, 1, 1, 1, RES_1440_720 },
  { "1440x720hq2",      N_("1440 x 720 DPI Highest Quality"),
    1440, 720,  1,  0, 2, 1, 1, 1, 1, RES_1440_720 },

  { "2880x720sw",       N_("2880 x 720 DPI"),
    2880, 720,  1,  0, 1, 1, 0, 1, 1, RES_2880_720},
  { "2880x720swuni",    N_("2880 x 720 DPI Unidirectional"),
    2880, 720,  1,  0, 1, 1, 1, 1, 1, RES_2880_720},

  { "1440x1440sw",      N_("1440 x 1440 DPI"),
    1440, 1440, 1,  0, 1, 1, 1, 1, 1, RES_1440_1440},
  { "1440x1440hq2",     N_("1440 x 1440 DPI Highest Quality"),
    1440, 1440, 1,  0, 2, 1, 1, 1, 1, RES_1440_1440},

  { "2880x1440sw",      N_("2880 x 1440 DPI"),
    2880, 1440, 1,  0, 1, 1, 1, 1, 1, RES_2880_1440},

  { "", "", 0, 0, 0, 0, 0, 0, 1, -1 }
};

static const res_t pro_reslist[] =
{
  { "360x90dpi",        N_("360 x 90 DPI Fast Economy Draft"),
    360,  90,   0,  0, 1, 1, 0, 1, 1, RES_120_M },

  { "360x120dpi",       N_("360 x 120 DPI Economy Draft"),
    360,  120,  0,  0, 1, 1, 0, 3, 1, RES_120_M },

  { "180dpi",           N_("180 DPI Economy Draft"),
    180,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },

  { "360x240dpi",       N_("360 x 240 DPI Draft"),
    360,  240,  0,  0, 1, 1, 0, 3, 2, RES_180_M },

  { "360x180dpi",       N_("360 x 180 DPI Draft"),
    360,  180,  0,  0, 1, 1, 0, 1, 1, RES_180_M },

  { "360mw",            N_("360 DPI Microweave"),
    360,  360,  0,  1, 1, 1, 0, 1, 1, RES_360_M },
  { "360mwuni",         N_("360 DPI Microweave Unidirectional"),
    360,  360,  0,  1, 1, 1, 1, 1, 1, RES_360_M },
  { "360dpi",           N_("360 DPI"),
    360,  360,  0,  0, 1, 1, 0, 1, 1, RES_360_M },
  { "360uni",           N_("360 DPI Unidirectional"),
    360,  360,  0,  0, 1, 1, 1, 1, 1, RES_360_M },
  { "360fol",           N_("360 DPI Full Overlap"),
    360,  360,  0,  2, 1, 1, 0, 1, 1, RES_360_M },
  { "360foluni",        N_("360 DPI Full Overlap Unidirectional"),
    360,  360,  0,  2, 1, 1, 1, 1, 1, RES_360_M },
  { "360fol2",          N_("360 DPI FOL2"),
    360,  360,  0,  4, 1, 1, 0, 1, 1, RES_360_M },
  { "360fol2uni",       N_("360 DPI FOL2 Unidirectional"),
    360,  360,  0,  4, 1, 1, 1, 1, 1, RES_360_M },
  { "360mw2",           N_("360 DPI MW2"),
    360,  360,  0,  5, 1, 1, 0, 1, 1, RES_360_M },
  { "360mw2uni",        N_("360 DPI MW2 Unidirectional"),
    360,  360,  0,  5, 1, 1, 1, 1, 1, RES_360_M },

  { "720x360dpi",       N_("720 x 360 DPI"),
    720,  360,  0,  0, 1, 1, 0, 2, 1, RES_720_360_M },
  { "720x360uni",       N_("720 x 360 DPI Unidirectional"),
    720,  360,  0,  0, 1, 1, 1, 2, 1, RES_720_360_M },
  { "720x360mw",        N_("720 x 360 DPI Microweave"),
    720,  360,  0,  1, 1, 1, 0, 2, 1, RES_720_360_M },
  { "720x360mwuni",     N_("720 x 360 DPI Microweave Unidirectional"),
    720,  360,  0,  1, 1, 1, 1, 2, 1, RES_720_360_M },
  { "720x360fol",       N_("720 x 360 DPI FOL"),
    720,  360,  0,  2, 1, 1, 0, 2, 1, RES_720_360_M },
  { "720x360foluni",    N_("720 x 360 DPI FOL Unidirectional"),
    720,  360,  0,  2, 1, 1, 1, 2, 1, RES_720_360_M },
  { "720x360fol2",      N_("720 x 360 DPI FOL2"),
    720,  360,  0,  4, 1, 1, 0, 2, 1, RES_720_360_M },
  { "720x360fol2uni",   N_("720 x 360 DPI FOL2 Unidirectional"),
    720,  360,  0,  4, 1, 1, 1, 2, 1, RES_720_360_M },
  { "720x360mw2",       N_("720 x 360 DPI MW2"),
    720,  360,  0,  5, 1, 1, 0, 2, 1, RES_720_360_M },
  { "720x360mw2uni",    N_("720 x 360 DPI MW2 Unidirectional"),
    720,  360,  0,  5, 1, 1, 1, 2, 1, RES_720_360_M },

  { "720mw",            N_("720 DPI Microweave"),
    720,  720,  0,  1, 1, 1, 0, 1, 1, RES_720_M },
  { "720mwuni",         N_("720 DPI Microweave Unidirectional"),
    720,  720,  0,  1, 1, 1, 1, 1, 1, RES_720_M },
  { "720fol",           N_("720 DPI Full Overlap"),
    720,  720,  0,  2, 1, 1, 0, 1, 1, RES_720_M },
  { "720foluni",        N_("720 DPI Full Overlap Unidirectional"),
    720,  720,  0,  2, 1, 1, 1, 1, 1, RES_720_M },
  { "720fourp",         N_("720 DPI Four Pass"),
    720,  720,  0,  3, 1, 1, 0, 1, 1, RES_720_M },
  { "720fourpuni",      N_("720 DPI Four Pass Unidirectional"),
    720,  720,  0,  3, 1, 1, 1, 1, 1, RES_720_M },

  { "1440x720mw",       N_("1440 x 720 DPI Microweave"),
    1440, 720,  0,  1, 1, 1, 0, 1, 1, RES_1440_720_M },
  { "1440x720mwuni",    N_("1440 x 720 DPI Microweave Unidirectional"),
    1440, 720,  0,  1, 1, 1, 1, 1, 1, RES_1440_720_M },
  { "1440x720fol",      N_("1440 x 720 DPI FOL"),
    1440, 720,  0,  2, 1, 1, 0, 1, 1, RES_1440_720_M },
  { "1440x720foluni",   N_("1440 x 720 DPI FOL Unidirectional"),
    1440, 720,  0,  2, 1, 1, 1, 1, 1, RES_1440_720_M },
  { "1440x720fourp",    N_("1440 x 720 DPI Four Pass"),
    1440, 720,  0,  3, 1, 1, 0, 1, 1, RES_1440_720_M },
  { "1440x720fourpuni", N_("1440 x 720 DPI Four Pass Unidirectional"),
    1440, 720,  0,  3, 1, 1, 1, 1, 1, RES_1440_720_M },

  { "", "", 0, 0, 0, 0, 0, 0, 1, -1 }
};

#define INCH(x)		(72 * x)

const escp2_stp_printer_t stp_escp2_model_capabilities[] =
{
  /* FIRST GENERATION PRINTERS */
  /* 0: Stylus Color */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    15, 1, 4, 15, 1, 4,
    720, 720, 360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 0, 0, default_head_offset, 0, 0,
    g1_dotsizes, g1_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 1: Stylus Color 400/500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 48, 1, 3,
    720, 720, 360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 0, 0, default_head_offset, 0, 0,
    g2_dotsizes, g1_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 2: Stylus Color 1500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_NO | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    1, 1, 1, 1, 1, 1,
    720, 720, 360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17), INCH(44), INCH(2), INCH(4), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 0, 0, default_head_offset, 0, 0,
    g1_dotsizes, sc1500_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 3: Stylus Color 600 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    32, 1, 4, 32, 1, 4,
    720, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 8, 9, 0, 30, 8, 9, 0, 30,
    0, 1, 0, 0, default_head_offset, 0, 0,
    sc600_dotsizes, g3_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 4: Stylus Color 800 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    64, 1, 2, 64, 1, 2,
    720, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 8, 9, 9, 40, 8, 9, 9, 40,
    0, 1, 4, 0, default_head_offset, 0, 0,
    g3_dotsizes, g3_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 5: Stylus Color 850 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    64, 1, 2, 128, 1, 1,
    720, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 4, 0, default_head_offset, 0, 0,
    g3_dotsizes, g3_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 6: Stylus Color 1520 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    64, 1, 2, 64, 1, 2,
    720, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17), INCH(44), INCH(2), INCH(4), 8, 9, 9, 40, 8, 9, 9, 40,
    0, 1, 4, 0, default_head_offset, 0, 0,
    g3_dotsizes, g3_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },

  /* SECOND GENERATION PRINTERS */
  /* 7: Stylus Photo 700 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    32, 1, 4, 32, 1, 4,
    720, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 30, 9, 9, 0, 30,
    0, 1, 0, 0, default_head_offset, 0, 0,
    photo_dotsizes, g3_densities, &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 8: Stylus Photo EX */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    32, 1, 4, 32, 1, 4,
    720, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(118 / 10), INCH(44), INCH(2), INCH(4), 9, 9, 0, 30, 9, 9, 0, 30,
    0, 1, 0, 0, default_head_offset, 0, 0,
    photo_dotsizes, g3_densities, &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 9: Stylus Photo */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    32, 1, 4, 32, 1, 4,
    720, 360, 360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 30, 9, 9, 0, 30,
    0, 1, 0, 0, default_head_offset, 0, 0,
    photo_dotsizes, g3_densities, &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },

  /* THIRD GENERATION PRINTERS */
  /* 10: Stylus Color 440/460 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    21, 1, 4, 21, 1, 4,
    720, 720, 360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    sc440_dotsizes, sc440_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 11: Stylus Color 640 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    32, 1, 4, 64, 1, 2,
    720, 720, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    sc640_dotsizes, sc440_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 12: Stylus Color 740 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 144, 1, 1,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    c6pl_dotsizes, c6pl_densities, &variable_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 13: Stylus Color 900 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    96, 1, 2, 192, 1, 1,
    360, 180, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    c3pl_dotsizes, c3pl_densities, &variable_3pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 14: Stylus Photo 750 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 48, 1, 3,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    c6pl_dotsizes, c6pl_densities, &variable_6pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 15: Stylus Photo 1200 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 48, 1, 3,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    c6pl_dotsizes, c6pl_densities, &variable_6pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 16: Stylus Color 860 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 144, 1, 1,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    c4pl_dotsizes, c4pl_densities, &variable_4pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 17: Stylus Color 1160 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 144, 1, 1,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    c4pl_dotsizes, c4pl_densities, &variable_4pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 18: Stylus Color 660 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    32, 1, 4, 32, 1, 4,
    720, 720, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 26,
    0, 1, 8, 0, default_head_offset, 0, 0,
    sc660_dotsizes,sc660_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 19: Stylus Color 760 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 144, 1, 1,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    c4pl_dotsizes, c4pl_densities, &variable_4pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 20: Stylus Photo 720 (Australia) */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    32, 1, 4, 32, 1, 4,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    sc720_dotsizes, c6pl_densities, &variable_6pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 21: Stylus Color 480 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    15, 15, 3, 48, 48, 3,
    360, 360, 360, 720, 720, 14400, 360, 720, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, x80_head_offset, -99, 0,
    sc480_dotsizes, sc480_densities, &variable_x80_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 22: Stylus Photo 870 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 48, 1, 3,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 0, 0, 0, 9, 0, 0, 0, 9,
    0, 1, 0, 97, default_head_offset, 0, 0,
    c4pl_dotsizes, c4pl_densities, &variable_4pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 23: Stylus Photo 1270 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 48, 1, 3,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4), 0, 0, 0, 9, 0, 0, 0, 9,
    0, 1, 0, 97, default_head_offset, 0, 0,
    c4pl_dotsizes, c4pl_densities, &variable_4pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 24: Stylus Color 3000 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    64, 1, 2, 128, 1, 1,
    720, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17), INCH(44), INCH(2), INCH(4), 8, 9, 9, 40, 8, 9, 9, 40,
    0, 1, 4, 0, default_head_offset, 0, 0,
    g3_dotsizes, g3_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 25: Stylus Color 670 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    32, 1, 4, 64, 1, 2,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    sc670_dotsizes, c6pl_densities, &variable_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 26: Stylus Photo 2000P */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 144, 1, 1,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    sp2000_dotsizes, sp2000_densities, &variable_pigment_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 27: Stylus Pro 5000 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_ENHANCED |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    1, 1, 1, 1, 1, 1,
    1440, 1440, 360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    spro_dotsizes, spro_densities, &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, pro_reslist
  },
  /* 28: Stylus Pro 7000 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_ENHANCED |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    1, 1, 1, 1, 1, 1,
    1440, 1440, 360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(24), INCH(1200), INCH(7), INCH(7), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 0, 0, default_head_offset, 0, 0,
    spro_dotsizes, spro_densities, &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, pro_reslist
  },
  /* 29: Stylus Pro 7500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_ENHANCED |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_YES | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    1, 1, 1, 1, 1, 1,
    1440, 1440, 360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(24), INCH(1200), INCH(7), INCH(7), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 0, 0, default_head_offset, 0, 0,
    spro_dotsizes, spro_densities, &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, pro_reslist
  },
  /* 30: Stylus Pro 9000 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_ENHANCED |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    1, 1, 1, 1, 1, 1,
    1440, 1440, 360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(44), INCH(1200), INCH(7), INCH(7), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 0, 0, default_head_offset, 0, 0,
    spro_dotsizes, spro_densities, &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, pro_reslist
  },
  /* 31: Stylus Pro 9500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_ENHANCED |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_YES | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    1, 1, 1, 1, 1, 1,
    1440, 1440, 360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(44), INCH(1200), INCH(7), INCH(7), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 0, 0, default_head_offset, 0, 0,
    spro_dotsizes, spro_densities, &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, pro_reslist
  },
  /* 32: Stylus Color 777/680 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 144, 1, 1,
    360, 360, 360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 9, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    c4pl_dotsizes, sc680_densities, &variable_680_4pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 33: Stylus Color 880/83/C60 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 144, 1, 1,
    360, 360, 360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 9, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    c4pl_dotsizes, c4pl_densities, &variable_4pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 34: Stylus Color 980 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    96, 1, 2, 192, 1, 1,
    360, 360, 360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 9, 9,
    192, 1, 0, 0, default_head_offset, 0, 0,
    c3pl_dotsizes, sc980_densities, &variable_3pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 35: Stylus Photo 780/790/785/810/820 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_YES | MODEL_YZEROMARGIN_YES |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 48, 1, 3,
    360, 360, 360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 55, default_head_offset, 0, 0,
    c4pl_dotsizes, c4pl_densities, &variable_4pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &sp780_paper_list, standard_reslist
  },
  /* 36: Stylus Photo 890/895 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_YZEROMARGIN_YES |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_YES),
    48, 1, 3, 48, 1, 3,
    360, 360, 360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 55, default_head_offset, 0, 0,
    c4pl_dotsizes, c4pl_densities, &variable_4pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 37: Stylus Photo 1280/1290 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_YZEROMARGIN_YES |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_YES),
    48, 1, 3, 48, 1, 3,
    360, 360, 360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4), 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 55, default_head_offset, 0, 0,
    c4pl_dotsizes, c4pl_densities, &variable_4pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 38: Stylus Color 580 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    15, 15, 3, 48, 48, 3,
    360, 360, 360, 720, 720, 14400, 360, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 9, 9,
    0, 1, 0, 0, x80_head_offset, -99, 0,
    sc480_dotsizes, sc480_densities, &variable_x80_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 39: Stylus Color Pro XL */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_360 |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 48, 1, 3,
    720, 720, 360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 0, 0, default_head_offset, 0, 0,
    g1_dotsizes, g1_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 40: Stylus Pro 5500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_ENHANCED |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_YES | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    1, 1, 1, 1, 1, 1,
    1440, 1440, 360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 0, 9,
    0, 1, 0, 0, default_head_offset, 0, 0,
    spro_dotsizes, spro_densities, &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, pro_reslist
  },
  /* 41: Stylus Pro 10000 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE |
     MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_ENHANCED |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_YES | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    1, 1, 1, 1, 1, 1,
    1440, 1440, 360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(44), INCH(1200), INCH(7), INCH(7), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 0, 0, default_head_offset, 0, 0,
    spro10000_dotsizes, spro10000_densities, &spro10000_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, pro_reslist
  },
  /* 42: Stylus C20SX/C20UX */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    15, 15, 3, 48, 48, 3,
    360, 360, 360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 9, 9,
    0, 1, 0, 0, x80_head_offset, -99, 0,
    sc480_dotsizes, sc480_densities, &variable_x80_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 43: Stylus C40SX/C40UX */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    15, 15, 3, 48, 48, 3,
    360, 360, 360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 9, 9,
    0, 1, 0, 0, x80_head_offset, -99, 0,
    sc480_dotsizes, sc480_densities, &variable_x80_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
  /* 44: Stylus C80 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4 |
     MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    60, 60, 2, 180, 180, 2,
    360, 360, 360, 720, 720, 14400, -1, 2880, 1440, 360, 180,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4), 9, 9, 0, 9, 9, 9, 9, 9,
    0, 1, 0, 0, c80_head_offset, -240, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_densities, &variable_3pl_pigment_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &c80_paper_list, standard_reslist
  },
  /* 45: Stylus Color Pro */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL |
     MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL |
     MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_MICROWEAVE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO |
     MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL |
     MODEL_DEINITIALIZE_JE_NO),
    48, 1, 3, 48, 1, 3,
    720, 720, 360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4), 9, 9, 9, 40, 9, 9, 9, 40,
    0, 1, 0, 0, default_head_offset, 0, 0,
    g1_dotsizes, g1_densities, &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment,
    &standard_paper_list, standard_reslist
  },
};
