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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <string.h>

#ifdef __GNUC__
#define inline __inline__
#endif

#ifdef TEST_UNCOMPRESSED
#define COMPRESSION (0)
#define FILLFUNC stp_fill_uncompressed
#define COMPUTEFUNC stp_compute_uncompressed_linewidth
#define PACKFUNC stp_pack_uncompressed
#else
#define COMPRESSION (1)
#define FILLFUNC stp_fill_tiff
#define COMPUTEFUNC stp_compute_tiff_linewidth
#define PACKFUNC stp_pack_tiff
#endif

static void flush_pass(stp_softweave_t *sw, int passno, int model, int width,
		       int hoffset, int ydpi, int xdpi, int physical_xdpi,
		       int vertical_subpass);

/*
 * Printer capabilities.
 *
 * Various classes of printer capabilities are represented by bitmasks.
 */

typedef unsigned long model_cap_t;
typedef unsigned long model_featureset_t;


/*
 * For each printer, we can select from a variety of dot sizes.
 * For single dot size printers, the available sizes are usually 0,
 * which is the "default", and some subset of 1-4.  For simple variable
 * dot size printers (with only one kind of variable dot size), the
 * variable dot size is specified as 0x10.  For newer printers, there
 * is a choice of variable dot sizes available, 0x10, 0x11, and 0x12 in
 * order of increasing size.
 *
 * Normally, we want to specify the smallest dot size that lets us achieve
 * a density of less than .8 or thereabouts (above that we start to get
 * some dither artifacts).  This needs to be tested for each printer and
 * resolution.
 *
 * An entry of -1 in a slot means that this resolution is not available.
 */

typedef int escp2_dot_size_t[11];

/*
 * Specify the base density for each available resolution.
 * This obviously depends upon the dot size.
 */

typedef double escp2_densities_t[12];

/*
 * Definition of the multi-level inks available to a given printer.
 * Each printer may use a different kind of ink droplet for variable
 * and single drop size for each supported horizontal resolution and
 * type of ink (4 or 6 color).
 *
 * Recall that 6 color ink is treated as simply another kind of
 * multi-level ink, but the driver offers the user a choice of 4 and
 * 6 color ink, so we need to define appropriate inksets for both
 * kinds of ink.
 *
 * Stuff like the MIS 4 and 6 "color" monochrome inks doesn't fit into
 * this model very nicely, so we'll either have to special case it
 * or find some way of handling it in here.
 */

typedef struct escp2_variable_ink
{
  const stp_simple_dither_range_t *range;
  int count;
  double density;
} escp2_variable_ink_t;

typedef const escp2_variable_ink_t *escp2_variable_inkset_t[NCOLORS];

#define INKTYPE_SINGLE	 0
#define INKTYPE_VARIABLE 1
#define INKTYPE_N	 2

#define INKSET_4	 0
#define INKSET_6	 1
#define INKSET_7	 2
#define INKSET_N	 3

#define RES_120_M	 0
#define RES_120		 1
#define RES_180_M	 2
#define RES_180		 3
#define RES_360_M	 4
#define RES_360		 5
#define RES_720_360_M	 6
#define RES_720_360	 7
#define RES_720_M	 8
#define RES_720		 9
#define RES_1440_720_M	 10
#define RES_1440_720	 11
#define RES_1440_1440_M	 12
#define RES_1440_1440	 13
#define RES_2880_720_M	 14
#define RES_2880_720	 15
#define RES_2880_1440_M	 16
#define RES_2880_1440	 17
#define RES_N		 18

static const int dotidmap[] =
{ 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 11, 12 };

static int
resid2dotid(int resid)
{
  if (resid < 0 || resid >= RES_N)
    return -1;
  return dotidmap[resid];
}

static const int densidmap[] =
{ 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13 };

static int
resid2densid(int resid)
{
  if (resid < 0 || resid >= RES_N)
    return -1;
  return densidmap[resid];
}

static int
bits2inktype(int bits)
{
  if (bits == 1)
    return INKTYPE_SINGLE;
  else
    return INKTYPE_VARIABLE;
}

static int
colors2inkset(int colors)
{
  switch (colors)
    {
    case 1:
    case 2:
    case 3:
    case 4:
      return INKSET_4;
    case 5:
    case 6:
      return INKSET_6;
    case 7:
      return INKSET_7;
    default:
      return -1;
    }
}

/*
 * Mapping between color and linear index.  The colors are
 * black, magenta, cyan, yellow, light magenta, light cyan
 */

static const int color_indices[16] = { 0, 1, 2, -1,
				       3, -1, -1, -1,
				       -1, 4, 5, -1,
				       6, -1, -1, -1 };
static const int colors[7] = { 0, 1, 2, 4, 1, 2, 4};
static const int densities[7] = { 0, 0, 0, 0, 1, 1, 1 };

static inline int
get_color_by_params(int plane, int density)
{
  if (plane > 4 || plane < 0 || density > 1 || density < 0)
    return -1;
  return color_indices[density * 8 + plane];
}

typedef const escp2_variable_inkset_t *escp2_variable_inklist_t[INKTYPE_N][INKSET_N][RES_N / 2];


static const stp_simple_dither_range_t photo_cyan_dither_ranges[] =
{
  { 0.27, 0x1, 0, 1 },
  { 1.0,  0x1, 1, 1 }
};

static const escp2_variable_ink_t photo_cyan_ink =
{
  photo_cyan_dither_ranges,
  sizeof(photo_cyan_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1
};

static const stp_simple_dither_range_t photo_magenta_dither_ranges[] =
{
  { 0.35, 0x1, 0, 1 },
  { 1.0,  0x1, 1, 1 }
};

static const escp2_variable_ink_t photo_magenta_ink =
{
  photo_magenta_dither_ranges,
  sizeof(photo_magenta_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1
};


static const stp_simple_dither_range_t photo_6pl_dither_ranges[] =
{
  { 0.065,  0x1, 0, 1 },
  { 0.13, 0x2, 0, 2 },
/*  { 0.26, 0x3, 0, 3 }, */
  { 0.25,  0x1, 1, 1 },
  { 0.5,   0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static const escp2_variable_ink_t photo_6pl_ink =
{
  photo_6pl_dither_ranges,
  sizeof(photo_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t photo_6pl_1440_dither_ranges[] =
{
  { 0.13,  0x1, 0, 1 },
  { 0.26,  0x2, 0, 2 },
/*  { 0.52, 0x3, 0, 3 }, */
  { 0.5,   0x1, 1, 1 },
  { 1.0,   0x2, 1, 2 },
};

static const escp2_variable_ink_t photo_6pl_1440_ink =
{
  photo_6pl_1440_dither_ranges,
  sizeof(photo_6pl_1440_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t photo_pigment_dither_ranges[] =
{ /* MRS: Not calibrated! */
  { 0.15,  0x1, 0, 1 },
  { 0.227, 0x2, 0, 2 },
  { 0.5,   0x1, 1, 1 },
  { 1.0,   0x2, 1, 2 }
};

static const escp2_variable_ink_t photo_pigment_ink =
{
  photo_pigment_dither_ranges,
  sizeof(photo_pigment_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t photo_4pl_dither_ranges[] =
{
  { 0.17,  0x1, 0, 1 },
  { 0.26,  0x2, 0, 2 },
  { 0.661, 0x1, 1, 1 },
  { 1.00,  0x2, 1, 2 }
};

static const escp2_variable_ink_t photo_4pl_ink =
{
  photo_4pl_dither_ranges,
  sizeof(photo_4pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t spro10000_photo_dither_ranges[] =
{
  { 0.17,  0x1, 0, 1 },
  { 0.26,  0x2, 0, 2 },
  { 0.661, 0x1, 1, 1 },
  { 1.00,  0x2, 1, 2 }
};

static const escp2_variable_ink_t spro10000_photo_ink =
{
  spro10000_photo_dither_ranges,
  sizeof(spro10000_photo_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t spro10000_standard_dither_ranges[] =
{
  { 0.661, 0x1, 1, 1 },
  { 1.00,  0x2, 1, 2 }
};

static const escp2_variable_ink_t spro10000_standard_ink =
{
  spro10000_standard_dither_ranges,
  sizeof(spro10000_standard_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t photo_4pl_1440_dither_ranges[] =
{
  { 0.26,  0x1, 0, 1 },
  { 0.393, 0x2, 0, 2 },
  { 0.90,  0x1, 1, 1 },
  { 1.36,  0x2, 1, 2 }
};

static const escp2_variable_ink_t photo_4pl_1440_ink =
{
  photo_4pl_1440_dither_ranges,
  sizeof(photo_4pl_1440_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_6pl_dither_ranges[] =
{
  { 0.25,  0x1, 1, 1 },
  { 0.5,   0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static const escp2_variable_ink_t standard_6pl_ink =
{
  standard_6pl_dither_ranges,
  sizeof(standard_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_x80_6pl_dither_ranges[] =
{
  { 0.325, 0x1, 1, 1 },
  { 0.5,   0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static const escp2_variable_ink_t standard_x80_6pl_ink =
{
  standard_x80_6pl_dither_ranges,
  sizeof(standard_x80_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t standard_x80_multishot_dither_ranges[] =
{
  { 0.163, 0x1, 1, 1 },
  { 0.5,   0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static const escp2_variable_ink_t standard_x80_multishot_ink =
{
  standard_x80_multishot_dither_ranges,
  sizeof(standard_x80_multishot_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t standard_x80_1440_6pl_dither_ranges[] =
{
  { 0.65,  0x1, 1, 1 },
  { 1.0,   0x2, 1, 2 },
};

static const escp2_variable_ink_t standard_x80_1440_6pl_ink =
{
  standard_x80_1440_6pl_dither_ranges,
  sizeof(standard_x80_1440_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static stp_simple_dither_range_t standard_980_6pl_dither_ranges[] =
{
  { 0.40,  0x1, 1, 1 },
  { 0.675, 0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static escp2_variable_ink_t standard_980_6pl_ink =
{
  standard_980_6pl_dither_ranges,
  sizeof(standard_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_6pl_1440_dither_ranges[] =
{
  { 0.5,   0x1, 1, 1 },
  { 1.0,   0x2, 1, 2 },
};

static const escp2_variable_ink_t standard_6pl_1440_ink =
{
  standard_6pl_1440_dither_ranges,
  sizeof(standard_6pl_1440_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_pigment_dither_ranges[] =
{ /* MRS: Not calibrated! */
  { 0.55,  0x1, 1, 1 },
  { 1.0,   0x2, 1, 2 }
};

static const escp2_variable_ink_t standard_pigment_ink =
{
  standard_pigment_dither_ranges,
  sizeof(standard_pigment_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_4pl_dither_ranges[] =
{
  { 0.661, 0x1, 1, 1 },
  { 1.00,  0x2, 1, 2 }
};

static const escp2_variable_ink_t standard_4pl_ink =
{
  standard_4pl_dither_ranges,
  sizeof(standard_4pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t standard_4pl_1440_dither_ranges[] =
{
  { 0.90,  0x1, 1, 1 },
  { 1.36,  0x2, 1, 2 },
};

static const escp2_variable_ink_t standard_4pl_1440_ink =
{
  standard_4pl_1440_dither_ranges,
  sizeof(standard_4pl_1440_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_3pl_dither_ranges[] =
{
  { 0.25,  0x1, 1, 1 },
  { 0.61,  0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static const escp2_variable_ink_t standard_3pl_ink =
{
  standard_3pl_dither_ranges,
  sizeof(standard_3pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_3pl_1440_dither_ranges[] =
{
  { 0.39, 0x1, 1, 1 },
  { 1.0,  0x2, 1, 2 }
};

static const escp2_variable_ink_t standard_3pl_1440_ink =
{
  standard_3pl_1440_dither_ranges,
  sizeof(standard_3pl_1440_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_3pl_2880_dither_ranges[] =
{
  { 1.0,   0x1, 1, 1 }
};

static const escp2_variable_ink_t standard_3pl_2880_ink =
{
  standard_3pl_2880_dither_ranges,
  sizeof(standard_3pl_2880_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t photo_multishot_dither_ranges[] =
{
  { 0.0728, 0x1, 0, 1 },
  { 0.151,  0x2, 0, 2 },
  { 0.26,   0x3, 0, 3 },
  { 1.0,    0x3, 1, 3 }
};

static const escp2_variable_ink_t photo_multishot_ink =
{
  photo_multishot_dither_ranges,
  sizeof(photo_multishot_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_multishot_dither_ranges[] =
{
  { 0.28,  0x1, 1, 1 },
  { 0.58,  0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
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

static const escp2_variable_inkset_t escp2_4pl_1440_standard_inks =
{
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink
};

static const escp2_variable_inkset_t escp2_4pl_1440_photo_inks =
{
  &standard_4pl_1440_ink,
  &photo_4pl_1440_ink,
  &photo_4pl_1440_ink,
  &standard_4pl_1440_ink
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
      &escp2_4pl_1440_standard_inks,
      &escp2_4pl_standard_inks,
      &escp2_4pl_standard_inks,
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
      &escp2_4pl_1440_standard_inks,
      &escp2_4pl_standard_inks,
      &escp2_4pl_standard_inks,
    },
    {
      &escp2_multishot_photo_inks,
      &escp2_multishot_photo_inks,
      &escp2_multishot_photo_inks,
      &escp2_multishot_photo_inks,
      &escp2_6pl_photo_inks,
      &escp2_4pl_photo_inks,
      &escp2_4pl_1440_photo_inks,
      &escp2_4pl_photo_inks,
      &escp2_4pl_photo_inks
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

/*
 * Dot sizes are for:
 *
 * 120/180 DPI
 * 360 micro
 * 360 soft
 * 720x360 micro
 * 720x360 soft
 * 720 micro
 * 720 soft
 * 1440x720 micro
 * 1440x720 soft
 * 1440x1440 micro
 * 1440x1440 soft
 * 2880x720/2880x1440 micro
 * 2880x720/2880x1440 soft
 */

static const int g1_dotsizes[] =
{ -2, -2, -1, -1, -2, -2, -2, -1, -1, -1, -1, -1, -1 };

static const int sc1500_dotsizes[] =
{ -2, -2, -1, -1, -2, -2, -1, -1, -1, -1, -1, -1, -1 };

static const int sc600_dotsizes[] =
{ 4, 4, -1, -1, 3, 2, 2, -1, 1, -1, 1, -1, 1 };

static const int g3_dotsizes[] =
{ 3, 3, -1, -1, 2, 1, 1, -1, 4, -1, 4, -1, -1 };

static const int ph1_dotsizes[] =
{ 3, 3, -1, -1, 2, -1, 1, -1, -1, -1, -1, -1, -1 };

static const int ph2_dotsizes[] =
{ 3, 3, -1, -1, 2, -1, 1, -1, 4, -1, -1, -1, -1 };

static const int sc440_dotsizes[] =
{ 3, 3, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1, -1 };

static const int sc640_dotsizes[] =
{ 3, 3, -1, -1, -1, 1, 1, -1, 1, -1, 1, -1, -1 };

static const int c6pl_dotsizes[] =
{ 4, 4, 0x10, 3, 0x10, 3, 0x10, -1, 0x10, -1, -1, -1, -1 };

static const int c3pl_dotsizes[] =
{ -1, 1, 0x11, 1, 0x11, 1, 0x10, -1, 0x10, -1, -1, -1, 0x10 };

static const int c4pl_dotsizes[] =
{ 0x12, 0, 0x12, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, -1 };

static const int sc660_dotsizes[] =
{ 3, 3, -1, 3, 0, 3, 0, -1, 0, -1, -1, -1, -1 };

static const int sc480_dotsizes[] =
{ 0x13, -1, 0x13, -1, 0x13, -1, 0x10, -1, 0x10, -1, -1, -1, -1 };

static const int p4pl_dotsizes[] =
{ 0x12, 4, 0x12, 2, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, -1 };

static const int sc670_dotsizes[] =
{ 3, 3, 0x12, 3, 0x12, 3, 0x11, -1, 0x11, -1, -1, -1, -1 };

static const int sp2000_dotsizes[] =
{ 2, 2, 0x11, 4, 0x11, 4, 0x10, -1, 0x10, -1, -1, -1, -1 };

static const int spro_dotsizes[] =
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1 };

static const int c4pl_2880_dotsizes[] =
{ 0, 0, 0x12, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, 0x10 };

static const int p4pl_2880_dotsizes[] =
{ 4, 4, 0x12, 2, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, 0x10 };

static const int spro10000_dotsizes[] =
{ 4, 0x11, 0x11, 0x11, 0x11, 0x10, 0x10, 0x10, 0x10, -1, -1, -1, -1 };


/*
 * Densities are for:
 *
 * 120/180 DPI
 * 360 micro
 * 360 soft
 * 720x360 micro
 * 720x360 soft
 * 720 micro
 * 720 soft
 * 1440x720 micro
 * 1440x720 soft
 * 1440x1440 micro
 * 1440x1440 soft
 * 2880x720 micro
 * 2880x720 soft
 * 2880x1440
 */

static const double g1_densities[] =
{ 2.0, 1.3, 0, 1.3, 0, .568, 0, 0, 0, 0, 0, 0, 0, 0 };

static const double sc1500_densities[] =
{ 2.0, 1.3, 0, 1.3, 0, .631, 0, 0, 0, 0, 0, 0, 0, 0 };

static const double g3_densities[] =
{ 2.0, 1.3, 1.3, 1.3, 1.3, .775, .775, .55, .55, .275, .275, .275, .275, .138 };

static const double sc440_densities[] =
{ 3.0, 2.0, 2.0, 1.0, 1.0, .900, .900, .45, .45, .45, .45, .225, .225, .113 };

static const double c6pl_densities[] =
{ 2.0, 1.3, 2.0, .65, 1.0, .646, .568, .323, .568, .568, .568, .284, .284, .142 };

static const double sc480_densities[] =
{ 2.0, 0, 1.4, 0, .7, 0, .710, 0, .710, 0, .710, 0, .365, .1825 };

static const double c3pl_densities[] =
{ 2.0, 1.3, 1.3, .65, .65, .646, .73, .7, .7, 1, 1, .91, .91, .455 };

static const double sc980_densities[] =
{ 2.0, 1.3, 1.3, .65, .65, .646, .511, .49, .49, 1, 1, .637, .637, .455 };

static const double c4pl_densities[] =
{ 2.0, 1.3, 1.3, .65, .65, .431, .568, .216, .784, .216, .784, .392, .392, .196 };

static const double sc660_densities[] =
{ 3.0, 2.0, 2.0, 1.0, 1.0, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 };

static const double sp2000_densities[] =
{ 2.0, 1.3, 1.3, .65, .65, .775, .852, .388, .438, .388, .438, .219, .219, .110 };

static const double spro_densities[] =
{ 2.0, 1.3, 1.3, .65, .65, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 };

static const double spro10000_densities[] =
{ 2.0, 1.3, 1.3, .65, .65, .431, .710, .216, .784, .216, .784, .392, .392, .196 };

/*
 For each printhead (=color), the offset in escp2_base_separation (1/360")
 units is defined here.
 */

static const int default_head_offset[] =
{0, 0, 0, 0, 0, 0, 0};

static const int x80_head_offset[] =
{48, 48, 96 ,0, 0, 0, 0};


typedef struct escp2_printer
{
  model_cap_t	flags;		/* Bitmask of flags, see below */
  int		nozzles;	/* Number of nozzles per color */
  int		min_nozzles;	/* Number of nozzles per color */
  int		nozzle_separation; /* Separation between rows, in 1/360" */
  int		black_nozzles;	/* Number of black nozzles (may be extra) */
  int		min_black_nozzles;	/* Number of black nozzles (may be extra) */
  int		black_nozzle_separation; /* Separation between rows */
  int		xres;		/* Normal distance between dots in */
				/* softweave mode (inverse inches) */
  int		enhanced_xres;	/* Distance between dots in highest */
				/* quality modes */
  int		max_paper_width; /* Maximum paper width, in points */
  int		max_paper_height; /* Maximum paper height, in points */
  int		min_paper_width; /* Maximum paper width, in points */
  int		min_paper_height; /* Maximum paper height, in points */
				/* Softweave: */
  int		left_margin;	/* Left margin, points */
  int		right_margin;	/* Right margin, points */
  int		top_margin;	/* Absolute top margin, points */
  int		bottom_margin;	/* Absolute bottom margin, points */
				/* "Micro"weave: */
  int		m_left_margin;	/* Left margin, points */
  int		m_right_margin;	/* Right margin, points */
  int		m_top_margin;	/* Absolute top margin, points */
  int		m_bottom_margin;	/* Absolute bottom margin, points */
  int		extra_feed;	/* Extra distance the paper can be spaced */
				/* beyond the bottom margin, in 1/360". */
				/* (maximum useful value is */
				/* nozzles * nozzle_separation) */
  int		separation_rows; /* Some printers require funky spacing */
				/* arguments in microweave mode. */
  int		pseudo_separation_rows;/* Some printers require funky */
				/* spacing arguments in softweave mode */

  int		base_separation; /* Basic unit of row separation */
  int		base_resolution; /* Base hardware spacing (above this */
				/* always requires multiple passes) */
  int		enhanced_resolution;/* Above this we use the */
				    /* enhanced_xres rather than xres */
  int		resolution_scale;   /* Scaling factor for ESC(D command */
  int		max_black_resolution; /* Above this resolution, we */
				      /* must use color parameters */
				      /* rather than (faster) black */
				      /* only parameters*/

		 /* The stylus 480 and 580 have an unusual arrangement of
				  color jets that need special handling */
  const int *head_offset;
  int		initial_vertical_offset;
  int		black_initial_vertical_offset;

  int		max_hres;
  int		max_vres;
  const int *dot_sizes;	/* Vector of dot sizes for resolutions */
  const double *densities;	/* List of densities for each printer */
  const escp2_variable_inklist_t *inks; /* Choices of inks for this printer */
  const double *lum_adjustment;
  const double *hue_adjustment;
  const double *sat_adjustment;
} escp2_stp_printer_t;

typedef struct escp2_printer_attribute
{
  const char *attr_name;
  int shift;
  int bits;
} escp2_printer_attr_t;

#define MODEL_INIT_MASK		0xful /* Is a special init sequence */
#define MODEL_INIT_STANDARD	0x0ul /* required for this printer, and if */
#define MODEL_INIT_NEW		0x1ul /* so, what */

#define MODEL_HASBLACK_MASK	0x10ul /* Can this printer print black ink */
#define MODEL_HASBLACK_YES	0x00ul /* when it is also printing color? */
#define MODEL_HASBLACK_NO	0x10ul

#define MODEL_COLOR_MASK	0x60ul /* Is this a 6-color printer? */
#define MODEL_COLOR_4		0x00ul
#define MODEL_COLOR_6		0x20ul
#define MODEL_COLOR_7		0x40ul

#define MODEL_GRAYMODE_MASK	0x80ul /* Does this printer support special */
#define MODEL_GRAYMODE_NO	0x00ul /* fast black printing? */
#define MODEL_GRAYMODE_YES	0x80ul

#define MODEL_720DPI_MODE_MASK	0x300ul /* Does this printer require old */
#define MODEL_720DPI_DEFAULT	0x000ul /* or new setting for printing */
#define MODEL_720DPI_600	0x100ul /* 720 dpi?  Only matters for */
					 /* single dot size printers */

#define MODEL_VARIABLE_DOT_MASK	0xc00ul /* Does this printer support var */
#define MODEL_VARIABLE_NORMAL	0x000ul /* dot size printing? The newest */
#define MODEL_VARIABLE_4	0x400ul /* printers support multiple modes */
#define MODEL_VARIABLE_MULTI	0x800ul /* of variable dot sizes. */

#define MODEL_COMMAND_MASK	0xf000ul /* What general command set does */
#define MODEL_COMMAND_1998	0x0000ul
#define MODEL_COMMAND_1999	0x1000ul /* The 1999 series printers */
#define MODEL_COMMAND_2000	0x2000ul /* The 2000 series printers */
#define MODEL_COMMAND_PRO	0x3000ul /* Stylus Pro printers */

#define MODEL_INK_MASK		0x10000ul /* Does this printer support */
#define MODEL_INK_NORMAL	0x00000ul /* different types of inks? */
#define MODEL_INK_SELECTABLE	0x10000ul /* Only the Stylus Pro's do */

#define MODEL_ROLLFEED_MASK	0x20000ul /* Does this printer support */
#define MODEL_ROLLFEED_NO	0x00000ul /* a roll feed? */
#define MODEL_ROLLFEED_YES	0x20000ul

#define MODEL_XZEROMARGIN_MASK	0x40000ul /* Does this printer support */
#define MODEL_XZEROMARGIN_NO	0x00000ul /* zero margin mode? */
#define MODEL_XZEROMARGIN_YES	0x40000ul /* (print to the edge of the paper) */

#define MODEL_YZEROMARGIN_MASK	0x80000ul /* Does this printer support */
#define MODEL_YZEROMARGIN_NO	0x00000ul /* zero margin mode? */
#define MODEL_YZEROMARGIN_YES	0x80000ul /* (print to the edge of the paper) */

#define MODEL_ENHANCED_MICROWEAVE_MASK	0x100000ul
#define MODEL_ENHANCED_MICROWEAVE_NO	0x000000ul
#define MODEL_ENHANCED_MICROWEAVE_YES	0x100000ul

#define MODEL_VACUUM_MASK	0x200000ul
#define MODEL_VACUUM_NO		0x000000ul
#define MODEL_VACUUM_YES	0x200000ul

#define MODEL_MICROWEAVE_EXCEPTION_MASK   0xc00000ul
#define MODEL_MICROWEAVE_EXCEPTION_NORMAL 0x000000ul
#define MODEL_MICROWEAVE_EXCEPTION_360    0x400000ul
#define MODEL_MICROWEAVE_EXCEPTION_BLACK  0x800000ul

#define MODEL_INIT			(0)
#define MODEL_HASBLACK			(1)
#define MODEL_COLOR			(2)
#define MODEL_GRAYMODE			(3)
#define MODEL_720DPI_MODE		(4)
#define MODEL_VARIABLE_DOT		(5)
#define MODEL_COMMAND			(6)
#define MODEL_INK			(7)
#define MODEL_ROLLFEED			(8)
#define MODEL_XZEROMARGIN		(9)
#define MODEL_YZEROMARGIN		(10)
#define MODEL_ENHANCED_MICROWEAVE	(11)
#define MODEL_VACUUM			(12)
#define MODEL_MICROWEAVE_EXCEPTION	(13)
#define MODEL_LIMIT			(14)

static const escp2_printer_attr_t escp2_printer_attrs[] =
{
  { "init_sequence",	 	0, 4 },
  { "has_black",	 	4, 1 },
  { "color",		 	5, 2 },
  { "graymode",		 	7, 1 },
  { "720dpi_mode",	 	8, 2 },
  { "variable_mode",		10, 2 },
  { "command_mode",		12, 4 },
  { "ink_types",		16, 1 },
  { "rollfeed",			17, 1 },
  { "horizontal_zero_margin",	18, 1 },
  { "vertical_zero_margin",	19, 1 },
  { "enhanced_microweave",	20, 1 },
  { "vacuum",			21, 1 },
  { "microweave_exception",	22, 2 },
};

#define INCH(x)		(72 * x)

static const escp2_stp_printer_t model_capabilities[] =
{
  /* FIRST GENERATION PRINTERS */
  /* 0: Stylus Color */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_BLACK),
    15, 1, 4, 15, 1, 4, 720, 720, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    14, 14, 9, 49, 14, 14, 9, 49, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 720, 720, g1_dotsizes, g1_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 1: Stylus Color 400/500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 48, 1, 3, 720, 720, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    14, 14, 0, 30, 14, 14, 0, 30, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 720, 720, g1_dotsizes, g1_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 2: Stylus Color 1500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_NO | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    1, 1, 1, 1, 1, 1, 720, 720, INCH(17), INCH(44), INCH(2), INCH(4),
    14, 14, 9, 49, 14, 14, 9, 49, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 720, 720, sc1500_dotsizes, sc1500_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 3: Stylus Color 600 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    32, 1, 4, 32, 1, 4, 720, 360, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    8, 9, 0, 30, 8, 9, 0, 30, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, sc600_dotsizes, g3_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 4: Stylus Color 800 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    64, 1, 2, 64, 1, 2, 720, 360, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    8, 9, 9, 40, 8, 9, 9, 40, 0, 1, 4, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, g3_dotsizes, g3_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 5: Stylus Color 850 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    64, 1, 2, 128, 1, 1, 720, 360, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 9, 40, 9, 9, 9, 40, 0, 1, 4, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, g3_dotsizes, g3_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 6: Stylus Color 1520 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    64, 1, 2, 64, 1, 2, 720, 360, INCH(17), INCH(44), INCH(2), INCH(4),
    8, 9, 9, 40, 8, 9, 9, 40, 0, 1, 4, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, g3_dotsizes, g3_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },

  /* SECOND GENERATION PRINTERS */
  /* 7: Stylus Photo 700 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    32, 1, 4, 32, 1, 4, 720, 360, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 30, 9, 9, 0, 30, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, ph2_dotsizes, g3_densities,
    &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 8: Stylus Photo EX */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    32, 1, 4, 32, 1, 4, 720, 360, INCH(118 / 10), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 30, 9, 9, 0, 30, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, ph2_dotsizes, g3_densities,
    &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 9: Stylus Photo */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    32, 1, 4, 32, 1, 4, 720, 360, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 30, 9, 9, 0, 30, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 720, 720, ph1_dotsizes, g3_densities,
    &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },

  /* THIRD GENERATION PRINTERS */
  /* 10: Stylus Color 440/460 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    21, 1, 4, 21, 1, 4, 720, 720, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 720, 720, sc440_dotsizes, sc440_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 11: Stylus Color 640 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    32, 1, 4, 64, 1, 2, 720, 720, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, sc640_dotsizes, sc440_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 12: Stylus Color 740 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 144, 1, 1, 360, 360, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, c6pl_dotsizes, c6pl_densities,
    &variable_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 13: Stylus Color 900 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    96, 1, 2, 192, 1, 1, 360, 180, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, c3pl_dotsizes, c3pl_densities,
    &variable_3pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 14: Stylus Photo 750 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 48, 1, 3, 360, 360, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, c6pl_dotsizes, c6pl_densities,
    &variable_6pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 15: Stylus Photo 1200 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 48, 1, 3, 360, 360, INCH(13), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, c6pl_dotsizes, c6pl_densities,
    &variable_6pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 16: Stylus Color 860 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 144, 1, 1, 360, 360, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, c4pl_dotsizes, c4pl_densities,
    &variable_4pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 17: Stylus Color 1160 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 144, 1, 1, 360, 360, INCH(13), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, c4pl_dotsizes, c4pl_densities,
    &variable_4pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 18: Stylus Color 660 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    32, 1, 4, 64, 1, 2, 720, 720, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 8, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, sc660_dotsizes,sc660_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 19: Stylus Color 760 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 144, 1, 1, 360, 360, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, c4pl_dotsizes, c4pl_densities,
    &variable_4pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 20: Stylus Photo 720 (Australia) */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    32, 1, 4, 32, 1, 4, 360, 360, INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, c6pl_dotsizes, c6pl_densities,
    &variable_6pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 21: Stylus Color 480 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    15, 15, 3, 48, 48, 3, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, 360,
    x80_head_offset, -99, 0, 720, 720, sc480_dotsizes, sc480_densities,
    &variable_x80_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 22: Stylus Photo 870 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 48, 1, 3, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 9, 0, 0, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, p4pl_dotsizes, c4pl_densities,
    &variable_4pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 23: Stylus Photo 1270 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 48, 1, 3, 360, 360, INCH(13), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 9, 0, 0, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, p4pl_dotsizes, c4pl_densities,
    &variable_4pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 24: Stylus Color 3000 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    64, 1, 2, 128, 1, 1, 720, 360, INCH(17), INCH(44), INCH(2), INCH(4),
    8, 9, 9, 40, 8, 9, 9, 40, 0, 1, 4, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, g3_dotsizes, g3_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 25: Stylus Color 670 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    32, 1, 4, 64, 1, 2, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, sc670_dotsizes, c6pl_densities,
    &variable_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 26: Stylus Photo 2000P */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 144, 1, 1, 360, 360, INCH(13), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, sp2000_dotsizes, sp2000_densities,
    &variable_pigment_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 27: Stylus Pro 5000 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_YES
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    1, 1, 1, 1, 1, 1, 1440, 1440, INCH(13), INCH(1200), INCH(11), INCH(17),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 1440, 1440, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, spro_dotsizes, spro_densities,
    &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 28: Stylus Pro 7000 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_YES
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    1, 1, 1, 1, 1, 1, 1440, 1440, INCH(24), INCH(1200), INCH(11), INCH(17),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 1440, 1440, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, spro_dotsizes, spro_densities,
    &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 29: Stylus Pro 7500 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_YES
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_YES | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    1, 1, 1, 1, 1, 1, 1440, 1440, INCH(24), INCH(1200), INCH(11), INCH(17),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 1440, 1440, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, spro_dotsizes, spro_densities,
    &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 30: Stylus Pro 9000 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_YES
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    1, 1, 1, 1, 1, 1, 1440, 1440, INCH(44), INCH(1200), INCH(11), INCH(17),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 1440, 1440, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, spro_dotsizes, spro_densities,
    &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 31: Stylus Pro 9500 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_YES
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_YES | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    1, 1, 1, 1, 1, 1, 1440, 1440, INCH(44), INCH(1200), INCH(11), INCH(17),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 1440, 1440, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, spro_dotsizes, spro_densities,
    &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 32: Stylus Color 777/680 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 144, 1, 1, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 2880, 720, c4pl_2880_dotsizes, c4pl_densities,
    &variable_4pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 33: Stylus Color 880/83 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 144, 1, 1, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 2880, 720, c4pl_2880_dotsizes, c4pl_densities,
    &variable_4pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 34: Stylus Color 980 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    96, 1, 2, 192, 1, 1, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 192, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 2880, 720, c3pl_dotsizes, sc980_densities,
    &variable_3pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 35: Stylus Photo 780/790/785 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_YES | MODEL_YZEROMARGIN_YES
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 48, 1, 3, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 2880, 720, p4pl_2880_dotsizes, c4pl_densities,
    &variable_4pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 36: Stylus Photo 890/895 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_YZEROMARGIN_YES
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 48, 1, 3, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 2880, 720, p4pl_2880_dotsizes, c4pl_densities,
    &variable_4pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 37: Stylus Photo 1280/1290 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_YZEROMARGIN_YES
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    48, 1, 3, 48, 1, 3, 360, 360, INCH(13), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 2880, 720, p4pl_2880_dotsizes, c4pl_densities,
    &variable_4pl_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 38: Stylus Color 580 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    15, 15, 3, 48, 48, 3, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 0, 1, 0, 360, 720, 720, 14400, 360,
    x80_head_offset, -99, 0, 1440, 720, sc480_dotsizes, sc480_densities,
    &variable_x80_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 39: Stylus Color Pro */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_360),
    48, 1, 3, 48, 1, 3, 720, 720, INCH(13), INCH(1200), INCH(2), INCH(4),
    14, 14, 0, 30, 14, 14, 0, 30, 0, 1, 0, 360, 720, 720, 14400, -1,
    default_head_offset, 0, 0, 720, 720, g1_dotsizes, g1_densities,
    &simple_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 40: Stylus Pro 5500 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_YES
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_YES | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    1, 1, 1, 1, 1, 1, 1440, 1440, INCH(13), INCH(1200), INCH(11), INCH(17),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 1440, 1440, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, spro_dotsizes, spro_densities,
    &simple_6color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 41: Stylus Pro 10000 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO | MODEL_ENHANCED_MICROWEAVE_YES
     | MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_YES | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    1, 1, 1, 1, 1, 1, 1440, 1440, INCH(44), INCH(1200), INCH(11), INCH(17),
    9, 9, 0, 9, 9, 9, 0, 9, 0, 1, 0, 360, 1440, 1440, 14400, -1,
    default_head_offset, 0, 0, 1440, 720, spro10000_dotsizes, spro10000_densities,
    &spro10000_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 42: Stylus Color C20SX/C40SX */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    15, 15, 3, 48, 48, 3, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    x80_head_offset, -99, 0, 720, 720, sc480_dotsizes, sc480_densities,
    &variable_x80_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
  /* 43: Stylus Color C20UX/C40UX */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES | MODEL_ENHANCED_MICROWEAVE_NO
     | MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_YZEROMARGIN_NO
     | MODEL_VACUUM_NO | MODEL_MICROWEAVE_EXCEPTION_NORMAL),
    15, 15, 3, 48, 48, 3, 360, 360, INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 0, 1, 0, 360, 720, 720, 14400, -1,
    x80_head_offset, -99, 0, 1440, 720, sc480_dotsizes, sc480_densities,
    &variable_x80_6pl_4color_inks,
    standard_lum_adjustment, standard_hue_adjustment, standard_sat_adjustment
  },
};

typedef struct
{
  const char *name;
  const char *text;
  int hres;
  int vres;
  int softweave;
  int microweave;
  int vertical_passes;
  int vertical_oversample;
  int unidirectional;
  int vertical_undersample;
  int vertical_denominator;
  int resid;
} res_t;

static const res_t *escp2_find_resolution(const char *resolution);

typedef struct
{
  int undersample;
  int initial_vertical_offset;
  int min_nozzles;
} escp2_privdata_t;

static const res_t escp2_reslist[] =
{
  { "360x120dpi", N_("360 x 120 DPI"),
    360,  120,  1,0,1,1,0,3,1,0 },
  { "360x120uni", N_("360 x 120 DPI Unidirectional"),
    360,  120,  1,0,1,1,1,3,1,0 },

  { "360x240dpi", N_("360 x 240 DPI"),
    360,  240,  1,0,1,1,0,3,2,0 },
  { "360x240uni", N_("360 x 240 DPI Unidirectional"),
    360,  240,  1,0,1,1,1,3,2,0 },

  { "180dpi", N_("180 DPI"),
    180,  180,  0,0,1,1,0,1,1,2 },
  { "180uni", N_("180 DPI Unidirectional"),
    180,  180,  0,0,1,1,1,1,1,2 },

  { "360x180dpi", N_("360 x 180 DPI"),
    360,  180,  0,0,1,1,0,1,1,2 },
  { "360x180uni", N_("360 x 180 DPI Unidirectional"),
    360,  180,  0,0,1,1,1,1,1,2 },

  { "360sw", N_("360 DPI Softweave"),
    360,  360,  1,0,1,1,0,1,1,5 },
  { "360swuni", N_("360 DPI Softweave Unidirectional"),
    360,  360,  1,0,1,1,1,1,1,5 },
  { "360mw", N_("360 DPI Microweave"),
    360,  360,  0,1,1,1,0,1,1,4 },
  { "360mwuni", N_("360 DPI Microweave Unidirectional"),
    360,  360,  0,1,1,1,1,1,1,4 },
  { "360dpi", N_("360 DPI"),
    360,  360,  0,0,1,1,0,1,1,4 },
  { "360uni", N_("360 DPI Unidirectional"),
    360,  360,  0,0,1,1,1,1,1,4 },
  { "360fol", N_("360 DPI Full Overlap"),
    360,  360,  0,2,1,1,0,1,1,4 },
  { "360foluni", N_("360 DPI Full Overlap Unidirectional"),
    360,  360,  0,2,1,1,1,1,1,4 },
  { "360fol2", N_("360 DPI FOL2"),
    360,  360,  0,4,1,1,0,1,1,4 },
  { "360fol2uni", N_("360 DPI FOL2 Unidirectional"),
    360,  360,  0,4,1,1,1,1,1,4 },
  { "360mw2", N_("360 DPI MW2"),
    360,  360,  0,5,1,1,0,1,1,4 },
  { "360mw2uni", N_("360 DPI MW2 Unidirectional"),
    360,  360,  0,5,1,1,1,1,1,4 },

  { "720x360sw", N_("720 x 360 DPI Softweave"),
    720,  360,  1,0,1,1,0,2,1,7 },
  { "720x360swuni", N_("720 x 360 DPI Softweave Unidirectional"),
    720,  360,  1,0,1,1,1,2,1,7 },
  { "720x360dpi", N_("720 x 360 DPI Default"),
    720,  360,  0,48,1,1,0,2,1,7},
  { "720x360uni", N_("720 x 360 DPI Default Unidirectional"),
    720,  360,  0,48,1,1,1,2,1,7},
  { "720x360mw", N_("720 x 360 DPI Microweave"),
    720,  360,  0,49,1,1,0,2,1,7},
  { "720x360mwuni", N_("720 x 360 DPI Microweave Unidirectional"),
    720,  360,  0,49,1,1,1,2,1,7},
  { "720x360fol", N_("720 x 360 DPI FOL"),
    720,  360,  0,2,1,1,0,2,1,7 },
  { "720x360foluni", N_("720 x 360 DPI FOL Unidirectional"),
    720,  360,  0,2,1,1,1,2,1,7 },
  { "720x360fol2", N_("720 x 360 DPI FOL2"),
    720,  360,  0,4,1,1,0,2,1,7 },
  { "720x360fol2uni", N_("720 x 360 DPI FOL2 Unidirectional"),
    720,  360,  0,4,1,1,1,2,1,7 },
  { "720x360mw2", N_("720 x 360 DPI MW2"),
    720,  360,  0,5,1,1,0,2,1,7 },
  { "720x360mw2uni", N_("720 x 360 DPI MW2 Unidirectional"),
    720,  360,  0,5,1,1,1,2,1,7 },

  { "720dpi", N_("720 DPI Default"),
    720,  720,  1,48,1,1,0,1,1,8},
  { "720uni", N_("720 DPI Default Unidirectional"),
    720,  720,  1,48,1,1,1,1,1,8},
  { "720mw", N_("720 DPI Microweave"),
    720,  720,  0,1,1,1,0,1,1,8 },
  { "720mwuni", N_("720 DPI Microweave Unidirectional"),
    720,  720,  0,1,1,1,1,1,1,8 },
  { "720fol", N_("720 DPI Full Overlap"),
    720,  720,  0,2,1,1,0,1,1,8 },
  { "720foluni", N_("720 DPI Full Overlap Unidirectional"),
    720,  720,  0,2,1,1,1,1,1,8 },
  { "720fourp", N_("720 DPI Four Pass"),
    720,  720,  0,3,1,1,0,1,1,8 },
  { "720fourpuni", N_("720 DPI Four Pass Unidirectional"),
    720,  720,  0,3,1,1,1,1,1,8 },
  { "720sw", N_("720 DPI Softweave"),
    720,  720,  1,0,1,1,0,1,1,9 },
  { "720swuni", N_("720 DPI Softweave Unidirectional"),
    720,  720,  1,0,1,1,1,1,1,9 },
  { "720hq", N_("720 DPI High Quality"),
    720,  720,  1,0,2,1,0,1,1,9 },
  { "720hquni", N_("720 DPI High Quality Unidirectional"),
    720,  720,  1,0,2,1,1,1,1,9 },
  { "720hq2", N_("720 DPI Highest Quality"),
    720,  720,  1,0,4,1,1,1,1,9 },

  { "1440x720dpi", N_("1440 x 720 DPI Default"),
    1440, 720, 1,48,1,1,0,1,1,10},
  { "1440x720uni", N_("1440 x 720 DPI Default Unidirectional"),
    1440, 720, 1,48,1,1,1,1,1,10},
  { "1440x720mw", N_("1440 x 720 DPI Microweave"),
    1440, 720,  0,1,1,1,0,1,1,10},
  { "1440x720mwuni", N_("1440 x 720 DPI Microweave Unidirectional"),
    1440, 720,  0,1,1,1,1,1,1,10},
  { "1440x720fol", N_("1440 x 720 DPI FOL"),
    1440, 720,  0,2,1,1,0,1,1,10},
  { "1440x720foluni", N_("1440 x 720 DPI FOL Unidirectional"),
    1440, 720,  0,2,1,1,1,1,1,10},
  { "1440x720fourp", N_("1440 x 720 DPI Four Pass"),
    1440, 720,  0,3,1,1,0,1,1,10},
  { "1440x720fourpuni", N_("1440 x 720 DPI Four Pass Unidirectional"),
    1440, 720,  0,3,1,1,1,1,1,10},
  { "1440x720sw", N_("1440 x 720 DPI Softweave"),
    1440, 720,  1,0,1,1,0,1,1,11},
  { "1440x720swuni", N_("1440 x 720 DPI Softweave Unidirectional"),
    1440, 720,  1,0,1,1,1,1,1,11},
  { "1440x720hq2", N_("1440 x 720 DPI Highest Quality"),
    1440, 720,  1,0,2,1,1,1,1,11},

  { "2880x720sw", N_("2880 x 720 DPI Softweave"),
    2880, 720,  1,0,1,1,0,1,1,15},
  { "2880x720swuni", N_("2880 x 720 DPI Softweave Unidirectional"),
    2880, 720,  1,0,1,1,1,1,1,15},

  /*
   * Nothing thus far supports 1440 DPI vertical resolution
   */
  { "1440x1440sw", N_("1440 x 1440 DPI Softweave"),
    1440, 1440, 1,0,1,1,1,1,1,13},
  { "1440x1440hq2", N_("1440 x 1440 DPI Highest Quality"),
    1440, 1440, 1,0,2,1,1,1,1,13},

  { "2880x1440sw", N_("2880 x 1440 DPI Softweave"),
    2880, 1440, 1,0,1,1,1,1,1,15},
#ifdef HAVE_MAINTAINER_MODE
  /*
   * These resolutions have no practical use; they're neither as good as
   * nor any faster than 720x720 DPI.  They exist to permit testing
   * 2880x720 (4:1 aspect) on 1440x720 printers.
   */
  { "1440x360sw", N_("1440 x 360 DPI Softweave"),
    1440, 360,  1,0,1,1,0,1,1,9 },
  { "1440x360swuni", N_("1440 x 360 DPI Softweave Unidirectional"),
    1440, 360,  1,0,1,1,1,1,1,9 },
  { "1440x360hq", N_("1440 x 360 DPI High Quality"),
    1440, 360,  1,0,2,1,0,1,1,9 },
  { "1440x360hquni", N_("1440 x 360 DPI High Quality Uni"),
    1440, 360,  1,0,2,1,1,1,1,9 },
  { "1440x360hq2", N_("1440 x 360 DPI Highest Quality"),
    1440, 360,  1,0,4,1,1,1,1,9 },
#endif
  { "", "", 0, 0, 0, 0, 0, 0, 1, -1 }
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

typedef struct
{
  const char *name;
  const char *text;
  int paper_feed_sequence;
  int platen_gap;
  double base_density;
  double k_lower_scale;
  double k_upper;
  double cyan;
  double magenta;
  double yellow;
  double p_cyan;
  double p_magenta;
  double p_yellow;
  double saturation;
  double gamma;
  int feed_adjustment;
  int vacuum_intensity;
  int paper_thickness;
  const double *hue_adjustment;
  const double *lum_adjustment;
  const double *sat_adjustment;
} paper_t;

static const paper_t escp2_paper_list[] =
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

static const int paper_type_count = sizeof(escp2_paper_list) / sizeof(paper_t);

typedef struct
{
  const char *name;
  const char *text;
  int hasblack;
  int ncolors;
} escp2_inkname_t;

static const escp2_inkname_t ink_types[] =
{
  { "Photo7", N_ ("Seven Color Enhanced"),	         1, 7 },
  { "PhotoEnhance", N_ ("Six Color Enhanced Composite"), 0, 7 },
  { "PhotoCMYK", N_ ("Six Color Photo"),		 1, 6 },
  { "PhotoCMY", N_ ("Five Color Photo Composite"),       0, 6 },
  { "CMYK", N_ ("Four Color Standard"),		         1, 4 },
  { "RGB", N_ ("Three Color Composite"),	         0, 4 }
};

static const int escp2_ninktypes = sizeof(ink_types) / sizeof(escp2_inkname_t);

typedef struct escp2_init
{
  int model;
  int output_type;
  int ydpi;
  int xdpi;
  int use_softweave;
  int use_microweave;
  int page_height;
  int page_width;
  int page_top;
  int page_bottom;
  int nozzles;
  int nozzle_separation;
  int horizontal_passes;
  int vertical_passes;
  int vertical_oversample;
  int bits;
  int unidirectional;
  int resid;
  int initial_vertical_offset;
  const char *paper_type;
  const char *media_source;
  stp_vars_t v;
} escp2_init_t;


static const paper_t *
get_media_type(const char *name)
{
  int i;
  for (i = 0; i < paper_type_count; i++)
    {
      if (!strcmp(name, escp2_paper_list[i].name))
	return &(escp2_paper_list[i]);
    }
  return NULL;
}

static int
escp2_has_cap(int model, int feature,
	      model_featureset_t class, const stp_vars_t v)
{
  if (feature < 0 || feature >= MODEL_LIMIT)
    return -1;
  else
    {
      model_featureset_t featureset =
	(((1ul << escp2_printer_attrs[feature].bits) - 1ul) <<
	 escp2_printer_attrs[feature].shift);
      return ((model_capabilities[model].flags & featureset) == class);
    }
}

static int
escp2_max_hres(int model, const stp_vars_t v)
{
  return (model_capabilities[model].max_hres);
}

static int
escp2_max_vres(int model, const stp_vars_t v)
{
  return (model_capabilities[model].max_vres);
}

static unsigned
escp2_nozzles(int model, const stp_vars_t v)
{
  return (model_capabilities[model].nozzles);
}

static unsigned
escp2_black_nozzles(int model, const stp_vars_t v)
{
  return (model_capabilities[model].black_nozzles);
}

static unsigned
escp2_min_nozzles(int model, const stp_vars_t v)
{
  return (model_capabilities[model].min_nozzles);
}

static unsigned
escp2_black_min_nozzles(int model, const stp_vars_t v)
{
  return (model_capabilities[model].min_black_nozzles);
}

static unsigned
escp2_nozzle_separation(int model, const stp_vars_t v)
{
  return (model_capabilities[model].nozzle_separation);
}

static unsigned
escp2_black_nozzle_separation(int model, const stp_vars_t v)
{
  return (model_capabilities[model].black_nozzle_separation);
}

static unsigned
escp2_separation_rows(int model, const stp_vars_t v)
{
  return (model_capabilities[model].separation_rows);
}

static unsigned
escp2_xres(int model, const stp_vars_t v)
{
  return (model_capabilities[model].xres);
}

static unsigned
escp2_enhanced_xres(int model, const stp_vars_t v)
{
  return (model_capabilities[model].enhanced_xres);
}

static int
escp2_ink_type(int model, int resid, const stp_vars_t v)
{
  int dotid = resid2dotid(resid);
  return model_capabilities[model].dot_sizes[dotid];
}

static double
escp2_density(int model, int resid, const stp_vars_t v)
{
  int densid = resid2densid(resid);
  return model_capabilities[model].densities[densid];
}

static const escp2_variable_inkset_t *
escp2_inks(int model, int resid, int colors, int bits, const stp_vars_t v)
{
  const escp2_variable_inklist_t *inks = model_capabilities[model].inks;
  int inktype = bits2inktype(bits);
  int inkset = colors2inkset(colors);
  resid /= 2;
  return (*inks)[inktype][inkset][resid];
}

static unsigned
escp2_max_paper_width(int model, const stp_vars_t v)
{
  return (model_capabilities[model].max_paper_width);
}

static unsigned
escp2_max_paper_height(int model, const stp_vars_t v)
{
  return (model_capabilities[model].max_paper_height);
}

static unsigned
escp2_min_paper_width(int model, const stp_vars_t v)
{
  return (model_capabilities[model].min_paper_width);
}

static unsigned
escp2_min_paper_height(int model, const stp_vars_t v)
{
  return (model_capabilities[model].min_paper_height);
}

static unsigned
escp2_left_margin(int model, const stp_vars_t v)
{
  const res_t *res = escp2_find_resolution(stp_get_resolution(v));
  if (res && !(res->softweave))
    return (model_capabilities[model].m_left_margin);
  else
    return (model_capabilities[model].left_margin);
}

static unsigned
escp2_right_margin(int model, const stp_vars_t v)
{
  const res_t *res = escp2_find_resolution(stp_get_resolution(v));
  if (res && !(res->softweave))
    return (model_capabilities[model].m_right_margin);
  else
    return (model_capabilities[model].right_margin);
}

static unsigned
escp2_top_margin(int model, const stp_vars_t v)
{
  const res_t *res = escp2_find_resolution(stp_get_resolution(v));
  if (res && !(res->softweave))
    return (model_capabilities[model].m_top_margin);
  else
    return (model_capabilities[model].top_margin);
}

static unsigned
escp2_bottom_margin(int model, const stp_vars_t v)
{
  const res_t *res = escp2_find_resolution(stp_get_resolution(v));
  if (res && !(res->softweave))
    return (model_capabilities[model].m_bottom_margin);
  else
    return (model_capabilities[model].bottom_margin);
}

static unsigned
escp2_extra_feed(int model, const stp_vars_t v)
{
  return (model_capabilities[model].extra_feed);
}

static int
escp2_pseudo_separation_rows(int model, const stp_vars_t v)
{
  return (model_capabilities[model].pseudo_separation_rows);
}

static int
escp2_base_separation(int model, const stp_vars_t v)
{
  return (model_capabilities[model].base_separation);
}

static int
escp2_base_resolution(int model, const stp_vars_t v)
{
  return (model_capabilities[model].base_resolution);
}

static int
escp2_enhanced_resolution(int model, const stp_vars_t v)
{
  return (model_capabilities[model].enhanced_resolution);
}

static int
escp2_resolution_scale(int model, const stp_vars_t v)
{
  return (model_capabilities[model].resolution_scale);
}

static const double *
escp2_lum_adjustment(int model, const stp_vars_t v)
{
  return (model_capabilities[model].lum_adjustment);
}

static const double *
escp2_hue_adjustment(int model, const stp_vars_t v)
{
  return (model_capabilities[model].hue_adjustment);
}

static const double *
escp2_sat_adjustment(int model, const stp_vars_t v)
{
  return (model_capabilities[model].sat_adjustment);
}

static const int *
escp2_head_offset(int model, const stp_vars_t v)
{
  return (model_capabilities[model].head_offset);
}

static int
escp2_initial_vertical_offset(int model, const stp_vars_t v)
{
  return (model_capabilities[model].initial_vertical_offset);
}

static int
escp2_black_initial_vertical_offset(int model, const stp_vars_t v)
{
  return (model_capabilities[model].black_initial_vertical_offset);
}

static int
escp2_max_black_resolution(int model, const stp_vars_t v)
{
  return (model_capabilities[model].max_black_resolution);
}

static int
escp2_has_advanced_command_set(int model, const stp_vars_t v)
{
  return (escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO,v) ||
	  escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_1999,v) ||
	  escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_2000,v));
}

static void *
xzmalloc(size_t bytes)
{
  void *retval = stp_malloc(bytes);
  if (retval)
    memset(retval, 0, bytes);
  return (retval);
}

static char *
c_strdup(const char *s)
{
  char *ret = stp_malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

/*
 * 'escp2_parameters()' - Return the parameter values for the given parameter.
 */

static stp_param_t *				/* O - Parameter values */
escp2_parameters(const stp_printer_t printer,	/* I - Printer model */
		 const char *ppd_file,	/* I - PPD file (not used) */
		 const char *name,	/* I - Name of parameter */
		 int  *count)		/* O - Number of values */
{
  int		i;
  stp_param_t	*valptrs;
  int		model = stp_printer_get_model(printer);
  const stp_vars_t v = stp_printer_get_printvars(printer);

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0)
  {
    unsigned int height_limit, width_limit;
    unsigned int min_height_limit, min_width_limit;
    int papersizes = stp_known_papersizes();
    valptrs = stp_malloc(sizeof(stp_param_t) * papersizes);
    *count = 0;
    width_limit = escp2_max_paper_width(model, v);
    height_limit = escp2_max_paper_height(model, v);
    min_width_limit = escp2_min_paper_width(model, v);
    min_height_limit = escp2_min_paper_height(model, v);

    for (i = 0; i < papersizes; i++)
    {
      const stp_papersize_t pt = stp_get_papersize_by_index(i);
      unsigned int pwidth = stp_papersize_get_width(pt);
      unsigned int pheight = stp_papersize_get_height(pt);
      if (strlen(stp_papersize_get_name(pt)) > 0 &&
	  pwidth <= width_limit && pheight <= height_limit &&
	  (pheight >= min_width_limit || pheight == 0) &&
	  (pwidth >= min_width_limit || pwidth == 0) &&
	  (pwidth == 0 || pheight > 0 ||
	   escp2_has_cap(model, MODEL_ROLLFEED, MODEL_ROLLFEED_YES, v)))
	{
	  valptrs[*count].name = c_strdup(stp_papersize_get_name(pt));
	  valptrs[*count].text = c_strdup(stp_papersize_get_text(pt));
	  (*count)++;
	}
    }

    return (valptrs);
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    const res_t *res = &(escp2_reslist[0]);
    int nozzle_width =
      (escp2_base_separation(model, v) / escp2_nozzle_separation(model, v));
    int nozzles = escp2_nozzles(model, v);
    valptrs =
      stp_malloc(sizeof(stp_param_t) * sizeof(escp2_reslist) / sizeof(res_t));
    *count = 0;
    while (res->hres)
      {
	if (escp2_ink_type(model, res->resid, v) != -1 &&
	    res->vres <= escp2_max_vres(model, v) &&
	    res->hres <= escp2_max_hres(model, v) &&
	    (res->microweave <= 1 ||
	     escp2_has_cap(model, MODEL_ENHANCED_MICROWEAVE,
			   MODEL_ENHANCED_MICROWEAVE_YES, v)) &&
	    (nozzles == 1 ||
	     ((res->vres / nozzle_width) * nozzle_width) == res->vres))
	  {
	    int nozzles = escp2_nozzles(model, v);
	    int xdpi = res->hres;
	    int physical_xdpi =
	      xdpi > escp2_enhanced_resolution(model, v) ?
	      escp2_enhanced_xres(model, v) :
	      escp2_xres(model, v);
	    int horizontal_passes = xdpi / physical_xdpi;
	    int oversample = horizontal_passes * res->vertical_passes
	      * res->vertical_oversample;
	    if (horizontal_passes < 1)
	      horizontal_passes = 1;
	    if (oversample < 1)
	      oversample = 1;
	    if (((horizontal_passes * res->vertical_passes) <= 8) &&
		(! res->softweave || (nozzles > 1 && nozzles > oversample)))
	      {
		valptrs[*count].name = c_strdup(res->name);
		valptrs[*count].text = c_strdup(_(res->text));
		(*count)++;
	      }
	  }
	res++;
      }
    return (valptrs);
  }
  else if (strcmp(name, "InkType") == 0)
  {
    valptrs = stp_malloc(sizeof(stp_param_t) * escp2_ninktypes);
    *count = 0;
    for (i = 0; i < escp2_ninktypes; i++)
    {
      if (ink_types[i].hasblack &&
	  (escp2_has_cap(model, MODEL_HASBLACK, MODEL_HASBLACK_NO, v)))
	continue;
      if ((ink_types[i].ncolors > 4) &&
	  (escp2_has_cap(model, MODEL_COLOR, MODEL_COLOR_4, v)))
	continue;
      if (ink_types[i].ncolors == 7 &&
	  !(escp2_has_cap(model, MODEL_COLOR, MODEL_COLOR_7, v)))
	continue;
      valptrs[*count].name = c_strdup(ink_types[i].name);
      valptrs[*count].text = c_strdup(_(ink_types[i].text));
      (*count)++;
    }
    return valptrs;
  }
  else if (strcmp(name, "MediaType") == 0)
  {
    int nmediatypes = paper_type_count;
    valptrs = stp_malloc(sizeof(stp_param_t) * nmediatypes);
    for (i = 0; i < nmediatypes; i++)
    {
      valptrs[i].name = c_strdup(escp2_paper_list[i].name);
      valptrs[i].text = c_strdup(_(escp2_paper_list[i].text));
    }
    *count = nmediatypes;
    return valptrs;
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    if (escp2_has_cap(model, MODEL_ROLLFEED, MODEL_ROLLFEED_NO, v))
      return NULL;
    else
    {      /* Roll Feed capable printers */
      valptrs = stp_malloc(sizeof(stp_param_t) * 2);
      valptrs[0].name = c_strdup("Standard");
      valptrs[0].text = c_strdup(_("Standard"));
      valptrs[1].name = c_strdup("Roll");
      valptrs[1].text = c_strdup(_("Roll Feed"));
      *count = 2;
      return valptrs;
    }
  }
  else
    return (NULL);
}

static const res_t *
escp2_find_resolution(const char *resolution)
{
  const res_t *res;
  if (!resolution || !strcmp(resolution, ""))
    return NULL;
  for (res = &escp2_reslist[0];;res++)
    {
      if (!strcmp(resolution, res->name))
	return res;
      else if (!strcmp(res->name, ""))
	return NULL;
    }
}  

/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

static void
escp2_imageable_area(const stp_printer_t printer,	/* I - Printer model */
		     const stp_vars_t v,   /* I */
		     int  *left,	/* O - Left position in points */
		     int  *right,	/* O - Right position in points */
		     int  *bottom,	/* O - Bottom position in points */
		     int  *top)		/* O - Top position in points */
{
  int	width, height;			/* Size of page */
  int	rollfeed;			/* Roll feed selected */
  int model = stp_printer_get_model(printer);

  rollfeed = (strcmp(stp_get_media_source(v), "Roll") == 0);

  stp_default_media_size(printer, v, &width, &height);

  *left =	escp2_left_margin(model, v);
  *right =	width - escp2_right_margin(model, v);

  /*
   * All printers should have 0 vertical margin capability in Roll Feed
   * mode --  They waste any paper they need automatically, and the
   * driver should print as much as the user wants
   */

  if (rollfeed)
    {
      *top =      height - 0;
      *bottom =   0;
    }
  else
    {
      *top =	height - escp2_top_margin(model, v);
      *bottom =	escp2_bottom_margin(model, v);
    }
}

static void
escp2_limit(const stp_printer_t printer,	/* I - Printer model */
	    const stp_vars_t v,			/* I */
	    int *width,
	    int *height,
	    int *min_width,
	    int *min_height)
{
  *width =	escp2_max_paper_width(stp_printer_get_model(printer),
				      stp_printer_get_printvars(printer));
  *height =	escp2_max_paper_height(stp_printer_get_model(printer),
				       stp_printer_get_printvars(printer));
  *min_width =	escp2_min_paper_width(stp_printer_get_model(printer),
				      stp_printer_get_printvars(printer));
  *min_height =	escp2_min_paper_height(stp_printer_get_model(printer),
				       stp_printer_get_printvars(printer));
}

static const char *
escp2_default_parameters(const stp_printer_t printer,
			 const char *ppd_file,
			 const char *name)
{
  int i;
  int model = stp_printer_get_model(printer);
  const stp_vars_t v = stp_printer_get_printvars(printer);
  if (name == NULL)
    return NULL;
  if (strcmp(name, "PageSize") == 0)
    {
      unsigned int height_limit, width_limit;
      unsigned int min_height_limit, min_width_limit;
      int papersizes = stp_known_papersizes();
      width_limit = escp2_max_paper_width(model, v);
      height_limit = escp2_max_paper_height(model, v);
      min_width_limit = escp2_min_paper_width(model, v);
      min_height_limit = escp2_min_paper_height(model, v);
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t pt = stp_get_papersize_by_index(i);
	  if (strlen(stp_papersize_get_name(pt)) > 0 &&
	      stp_papersize_get_width(pt) >= min_width_limit &&
	      stp_papersize_get_height(pt) >= min_height_limit &&
	      stp_papersize_get_width(pt) <= width_limit &&
	      stp_papersize_get_height(pt) <= height_limit)
	    return (stp_papersize_get_name(pt));
	}
      return NULL;
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      int model = stp_printer_get_model(printer);
      stp_vars_t v = stp_printer_get_printvars(printer);
      const res_t *res = &(escp2_reslist[0]);
      int nozzle_width = (escp2_base_separation(model, v) /
			  escp2_nozzle_separation(model, v));
      int nozzles = escp2_nozzles(model, v);
      while (res->hres)
	{
	  if (escp2_ink_type(model, res->resid, v) != -1 &&
	      res->vres <= escp2_max_vres(model, v) &&
	      res->hres <= escp2_max_hres(model, v) &&
	      (res->microweave <= 1 ||
	       escp2_has_cap(model, MODEL_ENHANCED_MICROWEAVE,
			     MODEL_ENHANCED_MICROWEAVE_YES, v)) &&
	      (nozzles == 1 ||
	       ((res->vres / nozzle_width) * nozzle_width) == res->vres))
	    {
	      int nozzles = escp2_nozzles(model, v);
	      int xdpi = res->hres;
	      int physical_xdpi =
		xdpi > escp2_enhanced_resolution(model, v) ?
		escp2_enhanced_xres(model, v) :
		escp2_xres(model, v);
	      int horizontal_passes = xdpi / physical_xdpi;
	      int oversample = horizontal_passes * res->vertical_passes
		* res->vertical_oversample;
	      if (horizontal_passes < 1)
		horizontal_passes = 1;
	      if (oversample < 1)
		oversample = 1;
	      if (((horizontal_passes * res->vertical_passes) <= 8) &&
		  (res->vres >= 360 && res->hres >= 360) &&
		  (! res->softweave || (nozzles > 1 && nozzles > oversample)))
		{
		  return (res->name);
		}
	    }
	  res++;
	}
      return NULL;
    }
  else if (strcmp(name, "InkType") == 0)
    {
      for (i = 0; i < escp2_ninktypes; i++)
	{
	  if (ink_types[i].hasblack &&
	      (escp2_has_cap(model, MODEL_HASBLACK, MODEL_HASBLACK_NO, v)))
	    continue;
	  if ((ink_types[i].ncolors > 4) &&
	      (escp2_has_cap(model, MODEL_COLOR, MODEL_COLOR_4, v)))
	    continue;
	  if (ink_types[i].ncolors == 7 &&
	      !(escp2_has_cap(model, MODEL_COLOR, MODEL_COLOR_7, v)))
	    continue;
	  return ink_types[i].name;
	}
      return NULL;
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      return (escp2_paper_list[0].name);
    }
  else if (strcmp(name, "InputSlot") == 0)
    {
      if (escp2_has_cap(model, MODEL_ROLLFEED, MODEL_ROLLFEED_NO, v))
	return NULL;
      else
	return "Standard";
    }
  else
    return (NULL);

}

static void
escp2_describe_resolution(const stp_printer_t printer,
			  const char *resolution, int *x, int *y)
{
  int model = stp_printer_get_model(printer);
  stp_vars_t v = stp_printer_get_printvars(printer);
  const res_t *res = &(escp2_reslist[0]);
  int nozzle_width =
    escp2_base_separation(model, v) / escp2_nozzle_separation(model, v);
  while (res->hres)
    {
      if (escp2_ink_type(model, res->resid, v) != -1 &&
	  res->vres <= escp2_max_vres(model, v) &&
	  res->hres <= escp2_max_hres(model, v) &&
	  (res->microweave <= 1 ||
	   escp2_has_cap(model, MODEL_ENHANCED_MICROWEAVE,
			 MODEL_ENHANCED_MICROWEAVE_YES, v)) &&
	  ((res->vres / nozzle_width) * nozzle_width) == res->vres &&
	  !strcmp(resolution, res->name))
	{
	  *x = res->hres;
	  *y = res->vres;
	  return;
	}
      res++;
    }
  *x = -1;
  *y = -1;
}

static void
escp2_reset_printer(const escp2_init_t *init)
{
  /*
   * Magic initialization string that's needed to take printer out of
   * packet mode.
   */
  if (escp2_has_cap(init->model, MODEL_INIT, MODEL_INIT_NEW, init->v))
    stp_zprintf(init->v, "%c%c%c\033\001@EJL 1284.4\n@EJL     \n\033@", 0, 0, 0);

  stp_puts("\033@", init->v);					/* ESC/P2 reset */
}

static void
escp2_set_remote_sequence(const escp2_init_t *init)
{
  /* Magic remote mode commands, whatever they do */

#if 0
  stp_zprintf(init->v, "\033(R%c%c%c%s", 1 + strlen(PACKAGE), 0, 0, PACKAGE);
  stp_zprintf(init->v, "\033%c%c%c", 0, 0, 0);
  stp_zprintf(init->v, "\033(R%c%c%c%s", 1 + strlen(VERSION), 0, 0, VERSION);
  stp_zprintf(init->v, "\033%c%c%c", 0, 0, 0);
  stp_zprintf(init->v, "\033(R%c%c%c%s", 1 + strlen(stp_get_driver(init->v)),
	      0, 0, stp_get_driver(init->v));
  stp_zprintf(init->v, "\033%c%c%c", 0, 0, 0);
  stp_puts("\033@", init->v);
#endif
  if (escp2_has_advanced_command_set(init->model, init->v))
    {
      int feed_sequence = 0;
      const paper_t *p = get_media_type(init->paper_type);
      /* Enter remote mode */
      stp_zprintf(init->v, "\033(R%c%c%cREMOTE1", 8, 0, 0);
      if (escp2_has_cap(init->model, MODEL_COMMAND,
			MODEL_COMMAND_PRO, init->v))
	{
	  /* Set Roll Feed mode */
	  if (strcmp(init->media_source,"Roll") == 0)
	    stp_zprintf(init->v, "PP%c%c%c%c%c", 3, 0, 0, 3, 0);
	  else
	    stp_zprintf(init->v, "PP%c%c%c%c%c", 3, 0, 0, 2, 0);
	  if (p)
	    {
	      stp_zprintf(init->v, "PH%c%c%c%c", 2, 0, 0, p->paper_thickness);
	      if (escp2_has_cap(init->model, MODEL_VACUUM, MODEL_VACUUM_YES,
				init->v))
		stp_zprintf(init->v, "SN%c%c%c%c%c", 3, 0, 0, 5,p->vacuum_intensity);
	      stp_zprintf(init->v, "SN%c%c%c%c%c", 3, 0, 0, 4, p->feed_adjustment);
	    }
	}
      else
	{
	  if (p)
	    feed_sequence = p->paper_feed_sequence;
	  /* Function unknown */
	  stp_zprintf(init->v, "PM%c%c%c%c", 2, 0, 0, 0);
	  /* Set mechanism sequence */
	  stp_zprintf(init->v, "SN%c%c%c%c%c", 3, 0, 0, 0, feed_sequence);
	  if (escp2_has_cap(init->model, MODEL_XZEROMARGIN,
			    MODEL_XZEROMARGIN_YES, init->v))
	    stp_zprintf(init->v, "FP%c%c%c\260\377", 3, 0, 0);

	  /* set up Roll-Feed options on appropriate printers
	     (tested for STP 870, which has no cutter) */
	  if (escp2_has_cap(init->model, MODEL_ROLLFEED,
			    MODEL_ROLLFEED_YES, init->v))
	    {
	      if (strcmp(init->media_source,"Roll") == 0)
		stp_zprintf(init->v, /* Set Roll Feed mode */
			    "IR%c%c%c%c"
			    "EX%c%c%c%c%c%c%c%c",
			    2, 0, 0, 1,
			    6, 0, 0, 0, 0, 0, 5, 1);
	      else
		stp_zprintf(init->v, /* Set non-Roll Feed mode */
			    "IR%c%c%c%c"
			    "EX%c%c%c%c%c%c%c%c",
			    2, 0, 0, 3,
			    6, 0, 0, 0, 0, 0, 5, 0);
	    }
	}
      /* Exit remote mode */
      stp_zprintf(init->v, "\033%c%c%c", 0, 0, 0);
    }
}

static void
escp2_set_graphics_mode(const escp2_init_t *init)
{
  stp_zfwrite("\033(G\001\000\001", 6, 1, init->v);	/* Enter graphics mode */
}

static void
escp2_set_resolution(const escp2_init_t *init)
{
  if (escp2_has_cap(init->model, MODEL_COMMAND, MODEL_COMMAND_PRO, init->v) ||
      (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT,
		       MODEL_VARIABLE_NORMAL, init->v)) &&
       init->use_softweave))
    {
      int hres = escp2_max_hres(init->model, init->v);
      stp_zprintf(init->v, "\033(U\005%c%c%c%c%c%c", 0, hres / init->ydpi,
		  hres / init->ydpi, hres / init->xdpi,
		  hres % 256, hres / 256);
    }
  else
    stp_zprintf(init->v, "\033(U\001%c%c", 0, 3600 / init->ydpi);
}

static void
escp2_set_color(const escp2_init_t *init)
{
  if (escp2_has_cap(init->model, MODEL_GRAYMODE, MODEL_GRAYMODE_YES,
		    init->v))
    stp_zprintf(init->v, "\033(K\002%c%c%c", 0, 0,
		(init->output_type == OUTPUT_GRAY ? 1 : 2));
}

static void
escp2_set_microweave(const escp2_init_t *init)
{
  stp_zprintf(init->v, "\033(i\001%c%c", 0, init->use_microweave);
}

static void
escp2_set_printhead_speed(const escp2_init_t *init)
{
  if (init->unidirectional)
    {
      stp_zprintf(init->v, "\033U%c", 1);
      if (init->xdpi > escp2_enhanced_resolution(init->model, init->v))
	stp_zprintf(init->v, "\033(s%c%c%c", 1, 0, 2);
    }
  else
    stp_zprintf(init->v, "\033U%c", 0);
}

static void
escp2_set_dot_size(const escp2_init_t *init)
{
  /* Dot size */
  int drop_size = escp2_ink_type(init->model, init->resid, init->v);
  if (drop_size >= 0)
    stp_zprintf(init->v, "\033(e\002%c%c%c", 0, 0, drop_size);
}

static void
escp2_set_page_height(const escp2_init_t *init)
{
  int l = init->ydpi * init->page_height / 72;
  if (escp2_has_cap(init->model, MODEL_COMMAND, MODEL_COMMAND_PRO, init->v) ||
      (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT,
		       MODEL_VARIABLE_NORMAL, init->v)) &&
       init->use_softweave))
    stp_zprintf(init->v, "\033(C\004%c%c%c%c%c", 0,
		l & 0xff, (l >> 8) & 0xff, (l >> 16) & 0xff, (l >> 24) & 0xff);
  else
    stp_zprintf(init->v, "\033(C\002%c%c%c", 0, l & 255, l >> 8);
}

static void
escp2_set_margins(const escp2_init_t *init)
{
  int l = init->ydpi * (init->page_height - init->page_bottom) / 72;
  int t = init->ydpi * (init->page_height - init->page_top) / 72;

  t += init->initial_vertical_offset;
  if (escp2_has_cap(init->model, MODEL_COMMAND, MODEL_COMMAND_PRO, init->v) ||
      (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT,
		       MODEL_VARIABLE_NORMAL, init->v)) &&
       init->use_softweave))
    {
      if (escp2_has_cap(init->model, MODEL_COMMAND,MODEL_COMMAND_2000,init->v))
	stp_zprintf(init->v, "\033(c\010%c%c%c%c%c%c%c%c%c", 0,
		    t & 0xff, t >> 8, (t >> 16) & 0xff, (t >> 24) & 0xff,
		    l & 0xff, l >> 8, (l >> 16) & 0xff, (l >> 24) & 0xff);
      else
	stp_zprintf(init->v, "\033(c\004%c%c%c%c%c", 0,
		    t & 0xff, t >> 8, l & 0xff, l >> 8);
    }
  else
    stp_zprintf(init->v, "\033(c\004%c%c%c%c%c", 0,
		t & 0xff, t >> 8, l & 0xff, l >> 8);
}

static void
escp2_set_form_factor(const escp2_init_t *init)
{
  if (escp2_has_advanced_command_set(init->model, init->v))
    {
      int page_width = init->page_width * init->ydpi / 72;
      int page_height = init->page_height * init->ydpi / 72;

      if (escp2_has_cap(init->model, MODEL_XZEROMARGIN, MODEL_XZEROMARGIN_YES,
			init->v))
	/* Make the page 2/10" wider (probably ignored by the printer) */
	page_width += 144 * 720 / init->xdpi;

      stp_zprintf(init->v, "\033(S\010%c%c%c%c%c%c%c%c%c", 0,
		  ((page_width >> 0) & 0xff), ((page_width >> 8) & 0xff),
		  ((page_width >> 16) & 0xff), ((page_width >> 24) & 0xff),
		  ((page_height >> 0) & 0xff), ((page_height >> 8) & 0xff),
		  ((page_height >> 16) & 0xff), ((page_height >> 24) & 0xff));
    }
}

static void
escp2_set_printhead_resolution(const escp2_init_t *init)
{
  if (escp2_has_cap(init->model, MODEL_COMMAND, MODEL_COMMAND_PRO, init->v) ||
      (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT,
		       MODEL_VARIABLE_NORMAL, init->v)) &&
       init->use_softweave))
    {
      int xres;
      int yres;
      int nozzle_separation;

      if (escp2_has_cap(init->model, MODEL_COMMAND, MODEL_COMMAND_PRO,init->v))
	xres = init->xdpi;
      else if (init->xdpi > escp2_enhanced_resolution(init->model, init->v))
	xres = escp2_enhanced_xres(init->model, init->v);
      else
	xres = escp2_xres(init->model, init->v);
      if (init->xdpi < xres)
	xres = init->xdpi;
      xres = escp2_resolution_scale(init->model, init->v) / xres;

      if (init->output_type == OUTPUT_GRAY &&
	  (escp2_max_black_resolution(init->model, init->v) < 0 ||
	   init->ydpi <= escp2_max_black_resolution(init->model, init->v)))
	nozzle_separation = escp2_black_nozzle_separation(init->model,
							  init->v);
      else
	nozzle_separation = escp2_nozzle_separation(init->model, init->v);

      if (escp2_has_cap(init->model, MODEL_COMMAND, MODEL_COMMAND_PRO,
			init->v) && !init->use_softweave)
	yres = escp2_resolution_scale(init->model, init->v) / init->ydpi;
      else
	yres = (nozzle_separation *
		escp2_resolution_scale(init->model, init->v) /
		escp2_base_separation(init->model, init->v));

      /* Magic resolution cookie */
      stp_zprintf(init->v, "\033(D%c%c%c%c%c%c", 4, 0,
		  escp2_resolution_scale(init->model, init->v) % 256,
		  escp2_resolution_scale(init->model, init->v) / 256,
		  yres, xres);
    }
}

static void
escp2_init_printer(const escp2_init_t *init)
{
  escp2_reset_printer(init);
  escp2_set_remote_sequence(init);
  escp2_set_graphics_mode(init);
  escp2_set_resolution(init);
  escp2_set_color(init);
  escp2_set_microweave(init);
  escp2_set_printhead_speed(init);
  escp2_set_dot_size(init);
  escp2_set_printhead_resolution(init);
  escp2_set_page_height(init);
  escp2_set_margins(init);
  escp2_set_form_factor(init);
}

static void
escp2_deinit_printer(const escp2_init_t *init)
{
  stp_puts(/* Eject page */
	   "\014"
	   /* ESC/P2 reset */
	   "\033@", init->v);
  if (escp2_has_advanced_command_set(init->model, init->v))
    {
      stp_zprintf(init->v, /* Enter remote mode */
		  "\033(R\010%c%cREMOTE1", 0, 0);
      /* set up Roll-Feed options on appropriate printers
	 (tested for STP 870, which has no cutter) */
      if (escp2_has_cap(init->model, MODEL_ROLLFEED,
			MODEL_ROLLFEED_YES, init->v))
	{
	  if(strcmp(init->media_source,"Roll") == 0)
	    stp_zprintf(init->v, /* End Roll Feed mode */
			"IR\002%c%c%c", 0, 0, 0);
	  else
	    stp_zprintf(init->v, /* End non-Roll Feed mode */
			"IR\002%c%c%c", 0, 0, 2);
	}
      stp_zprintf(init->v, /* Load settings from NVRAM */
		  "LD%c%c"
		  /* Exit remote mode */
		  "\033%c%c%c",  0, 0, 0, 0, 0);

    }
}

/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
static void
escp2_print(const stp_printer_t printer,		/* I - Model */
	    stp_image_t     *image,		/* I - Image to print */
	    const stp_vars_t    v)
{
  unsigned char *cmap = stp_get_cmap(v);
  int		model = stp_printer_get_model(printer);
  const char	*resolution = stp_get_resolution(v);
  const char	*media_type = stp_get_media_type(v);
  int		output_type = stp_get_output_type(v);
  int		orientation = stp_get_orientation(v);
  const char	*ink_type = stp_get_ink_type(v);
  double	scaling = stp_get_scaling(v);
  const char	*media_source = stp_get_media_source(v);
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  int		undersample;
  int		undersample_denominator;
  int		resid;
  int		physical_ydpi;
  int		physical_xdpi;
  int		i;
  int		n;		/* Output number */
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned char	*in,		/* Input pixels */
		*black = NULL,		/* Black bitmap data */
		*cyan = NULL,		/* Cyan bitmap data */
		*magenta = NULL,	/* Magenta bitmap data */
		*yellow = NULL,		/* Yellow bitmap data */
		*lcyan = NULL,		/* Light cyan bitmap data */
		*lmagenta = NULL,	/* Light magenta bitmap data */
		*dyellow = NULL;	/* Dark yellow bitmap data */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		page_true_height,	/* True height of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_bpp,	/* Output bytes per pixel */
		length,		/* Length of raster data */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  stp_convert_t	colorfunc = 0;	/* Color conversion function... */
  int		zero_mask;
  int   	image_height,
		image_width,
		image_bpp;
  int		use_softweave = 0;
  int		use_microweave = 0;
  int		nozzles = 1;
  int		nozzle_separation = 1;
  int		horizontal_passes = 1;
  int		vertical_passes = 1;
  int		vertical_oversample = 1;
  int		unidirectional = 0;
  int		hasblack = 0;
  const res_t	*res;
  int		bits = 1;
  void *	weave = NULL;
  void *	dither;
  int		separation_rows;
  int		ink_spread;
  stp_vars_t	nv = stp_allocate_copy(v);
  escp2_init_t	init;
  const escp2_variable_inkset_t *inks;
  const paper_t *pt;
  double k_upper, k_lower;
  int max_vres;
  const unsigned char *cols[7];
  int head_offset[8];
  const int *offset_ptr;
  int max_head_offset;
  double lum_adjustment[49], sat_adjustment[49], hue_adjustment[49];
  int ncolors = 0;
  escp2_privdata_t privdata;
  int drop_size;
  int min_nozzles;

  if (!stp_get_verified(nv))
    {
      stp_eprintf(nv, "Print options not verified; cannot print.\n");
      return;
    }

  privdata.undersample = 1;
  privdata.initial_vertical_offset = 0;
  stp_set_driver_data(nv, &privdata);

  separation_rows = escp2_separation_rows(model, nv);
  max_vres = escp2_max_vres(model, nv);

  if (output_type == OUTPUT_GRAY || output_type == OUTPUT_MONOCHROME)
    ncolors = 1;
  else
    for (i = 0; i < escp2_ninktypes; i++)
      {
	if (strcmp(ink_type, ink_types[i].name) == 0)
	  {
	    hasblack = ink_types[i].hasblack;
	    ncolors = ink_types[i].ncolors;
	    break;
	  }
      }
  if (ncolors == 0)
    {
      ink_type = escp2_default_parameters(printer, NULL, "InkType");
      for (i = 0; i < escp2_ninktypes; i++)
	{
	  if (strcmp(ink_type, ink_types[i].name) == 0)
	    {
	      hasblack = ink_types[i].hasblack;
	      ncolors = ink_types[i].ncolors;
	      break;
	    }
	}
    }
  stp_set_output_color_model(nv, COLOR_MODEL_CMY);

 /*
  * Setup a read-only pixel region for the entire image...
  */

  image->init(image);
  image_height = image->height(image);
  image_width = image->width(image);
  image_bpp = image->bpp(image);

 /*
  * Choose the correct color conversion function...
  */

  colorfunc = stp_choose_colorfunc(output_type, image_bpp, cmap, &out_bpp, nv);

 /*
  * Compute the output size...
  */
  escp2_imageable_area(printer, nv, &page_left, &page_right,
		       &page_bottom, &page_top);

  stp_compute_page_parameters(page_right, page_left, page_top, page_bottom,
			      scaling, image_width, image_height, image,
			      &orientation, &page_width, &page_height,
			      &out_width, &out_height, &left, &top);

  /*
   * Recompute the image height and width.  If the image has been
   * rotated, these will change from previously.
   */
  image_height = image->height(image);
  image_width = image->width(image);

 /*
  * Figure out the output resolution...
  */
  res = escp2_find_resolution(resolution);
  if (!res)
    return;
  use_softweave = res->softweave;
  use_microweave = res->microweave;
  if (!use_softweave)
    max_vres = escp2_base_resolution(model, nv);
  xdpi = res->hres;
  ydpi = res->vres;
  resid = res->resid;
  undersample = res->vertical_undersample;
  undersample_denominator = res->vertical_denominator;
  privdata.undersample = res->vertical_undersample;
  vertical_passes = res->vertical_passes;
  vertical_oversample = res->vertical_oversample;
  unidirectional = res->unidirectional;
  drop_size = escp2_ink_type(model, resid, nv);

  if (use_microweave &&
      !(escp2_has_cap(model, MODEL_MICROWEAVE_EXCEPTION,
		      MODEL_MICROWEAVE_EXCEPTION_NORMAL, nv)))
    {
      if (ydpi == 360)
	use_microweave = 0;
    }
  if (use_softweave)
    {
      physical_xdpi = (xdpi > escp2_enhanced_resolution(model, nv)) ?
	escp2_enhanced_xres(model, nv) : escp2_xres(model, nv);
      horizontal_passes = xdpi / physical_xdpi;
      if ((output_type == OUTPUT_GRAY || output_type == OUTPUT_MONOCHROME) &&
	  (escp2_max_black_resolution(model, nv) < 0 ||
	   ydpi <= escp2_max_black_resolution(model, nv)) &&
	  escp2_black_nozzles(model, nv))
	{
	  nozzles = escp2_black_nozzles(model, nv);
	  nozzle_separation = escp2_black_nozzle_separation(model, nv);
	  min_nozzles = escp2_black_min_nozzles(model, nv);
	}
      else
	{
	  nozzles = escp2_nozzles(model, nv);
	  nozzle_separation = escp2_nozzle_separation(model, nv);
	  min_nozzles = escp2_min_nozzles(model, nv);
	}
      nozzle_separation =
	nozzle_separation * ydpi / escp2_base_separation(model, nv);
    }
  else
    {
      physical_xdpi = (xdpi <= escp2_base_resolution(model, nv)) ?
	xdpi : escp2_base_resolution(model, nv);
      horizontal_passes = xdpi / escp2_base_resolution(model, nv);
      nozzles = 1;
      min_nozzles = 1;
      nozzle_separation = 1;
    }
  if (drop_size > 0 && drop_size & 0x10)
    bits = 2;
  else
    bits = 1;
  if (horizontal_passes == 0)
    horizontal_passes = 1;
  privdata.min_nozzles = min_nozzles;

  physical_ydpi = ydpi;
  if (ydpi > max_vres)
    physical_ydpi = max_vres;

  offset_ptr = escp2_head_offset(model, nv);
  max_head_offset = 0;
  if (ncolors > 1)
    for (i = 0; i < NCHANNELS; i++)
      {
	head_offset[i] = offset_ptr[i] * ydpi /
	  escp2_base_separation(model, nv);
	if (head_offset[i] > max_head_offset)
	  max_head_offset = head_offset[i];
      }


 /*
  * Let the user know what we're doing...
  */

  image->progress_init(image);

 /*
  * Send ESC/P2 initialization commands...
  */
  stp_default_media_size(printer, nv, &n, &page_true_height);
  init.model = model;
  init.output_type = output_type;
  if (init.output_type == OUTPUT_MONOCHROME)
    init.output_type = OUTPUT_GRAY;
  init.ydpi = ydpi * undersample;
  if (init.ydpi > escp2_max_vres(init.model, init.v))
    init.ydpi = escp2_max_vres(init.model, init.v);
  init.xdpi = xdpi;
  init.use_softweave = use_softweave;
  init.use_microweave = use_microweave;
  init.page_height = page_true_height;
  init.page_width = page_width;
  init.page_top = page_top;
  if (init.output_type == OUTPUT_GRAY)
    {
      if (escp2_max_black_resolution(model, nv) < 0 ||
	  ydpi <= escp2_max_black_resolution(init.model, init.v))
	init.initial_vertical_offset =
	  escp2_black_initial_vertical_offset(init.model, init.v) * init.ydpi /
	  escp2_base_separation(model, nv);
      else
    init.initial_vertical_offset =
      (escp2_initial_vertical_offset(init.model, init.v) + offset_ptr[0]) *
      init.ydpi / escp2_base_separation(model, nv);
    }
  else
    init.initial_vertical_offset =
      escp2_initial_vertical_offset(init.model, init.v) * init.ydpi /
      escp2_base_separation(model, nv);

   /* adjust bottom margin for a 480 like head configuration */
  init.page_bottom = page_bottom - max_head_offset * 72 / ydpi;
  if ((max_head_offset * 72 % ydpi) != 0)
    init.page_bottom -= 1;
  if (init.page_bottom < 0)
    init.page_bottom = 0;

  init.horizontal_passes = horizontal_passes;
  init.vertical_passes = vertical_passes;
  init.vertical_oversample = vertical_oversample;
  init.unidirectional = unidirectional;
  init.resid = resid;
  init.bits = bits;
  init.paper_type = media_type;
  init.media_source = media_source;
  init.v = nv;

  escp2_init_printer(&init);

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  left = physical_ydpi * undersample * left / 72 / undersample_denominator;

 /*
  * Adjust for zero-margin printing...
  */

  if (escp2_has_cap(model, MODEL_XZEROMARGIN, MODEL_XZEROMARGIN_YES, nv))
    {
     /*
      * In zero-margin mode, the origin is about 3/20" to the left of the
      * paper's left edge.
      */
      left += 92 * physical_ydpi * undersample / max_vres /
	undersample_denominator;
    }

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  if (output_type == OUTPUT_GRAY || output_type == OUTPUT_MONOCHROME)
    black = xzmalloc(length * bits);
  else
    {
      cyan = xzmalloc(length * bits);
      magenta = xzmalloc(length * bits);
      yellow = xzmalloc(length * bits);

      if (ncolors == 7)
	dyellow = xzmalloc(length * bits);
      if (ncolors >= 6)
	{
	  lcyan = xzmalloc(length * bits);
	  lmagenta = xzmalloc(length * bits);
	}
      if (hasblack)
	black = xzmalloc(length * bits);
    }
  cols[0] = black;
  cols[1] = magenta;
  cols[2] = cyan;
  cols[3] = yellow;
  cols[4] = lmagenta;
  cols[5] = lcyan;
  cols[6] = dyellow;

  /* Epson printers are currently all 720 physical dpi vertically */
  weave = stp_initialize_weave(nozzles, nozzle_separation,
			       horizontal_passes, vertical_passes,
			       vertical_oversample, ncolors, bits,
			       out_width, out_height, separation_rows,
			       top * physical_ydpi / 72,
			       (page_height * physical_ydpi / 72 +
				escp2_extra_feed(model, nv) * physical_ydpi /
				escp2_base_resolution(model, nv)),
			       1, head_offset, nv, flush_pass,
			       FILLFUNC, PACKFUNC, COMPUTEFUNC);

  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */
  if (ncolors > 4)
    k_lower = .5;
  else
    k_lower = .25;

  pt = get_media_type(stp_get_media_type(nv));
  if (pt)
    {
      stp_set_density(nv, stp_get_density(nv) * pt->base_density);
      if (ncolors >= 5)
	{
	  stp_set_cyan(nv, stp_get_cyan(nv) * pt->p_cyan);
	  stp_set_magenta(nv, stp_get_magenta(nv) * pt->p_magenta);
	  stp_set_yellow(nv, stp_get_yellow(nv) * pt->p_yellow);
	}
      else
	{
	  stp_set_cyan(nv, stp_get_cyan(nv) * pt->cyan);
	  stp_set_magenta(nv, stp_get_magenta(nv) * pt->magenta);
	  stp_set_yellow(nv, stp_get_yellow(nv) * pt->yellow);
	}
      stp_set_saturation(nv, stp_get_saturation(nv) * pt->saturation);
      stp_set_gamma(nv, stp_get_gamma(nv) * pt->gamma);
      k_lower *= pt->k_lower_scale;
      k_upper = pt->k_upper;
    }
  else				/* Can't find paper type? Assume plain */
    {
      stp_set_density(nv, stp_get_density(nv) * .8);
      k_lower *= .1;
      k_upper = .5;
    }
  stp_set_density(nv, stp_get_density(nv) * escp2_density(model, resid, nv));
  if (stp_get_density(nv) > 1.0)
    stp_set_density(nv, 1.0);
  if (ncolors == 1)
    stp_set_gamma(nv, stp_get_gamma(nv) / .8);
  stp_compute_lut(nv, 256);

 /*
  * Output the page...
  */

  if (xdpi > ydpi)
    dither = stp_init_dither(image_width, out_width, 1, xdpi / ydpi, nv);
  else
    dither = stp_init_dither(image_width, out_width, ydpi / xdpi, 1, nv);

  for (i = 0; i <= NCOLORS; i++)
    stp_dither_set_black_level(dither, i, 1.0);
  stp_dither_set_black_lower(dither, k_lower);
  stp_dither_set_black_upper(dither, k_upper);

  inks = escp2_inks(model, resid, ncolors, bits, nv);
  if (inks)
    for (i = 0; i < NCOLORS; i++)
      if ((*inks)[i])
	stp_dither_set_ranges(dither, i, (*inks)[i]->count, (*inks)[i]->range,
			      (*inks)[i]->density * k_upper *
			      stp_get_density(nv));

  switch (stp_get_image_type(nv))
    {
    case IMAGE_LINE_ART:
      stp_dither_set_ink_spread(dither, 19);
      break;
    case IMAGE_SOLID_TONE:
      stp_dither_set_ink_spread(dither, 15);
      break;
    case IMAGE_CONTINUOUS:
      ink_spread = 13;
      if (ydpi > escp2_max_vres(model, nv))
	ink_spread++;
      if (bits > 1)
	ink_spread++;
      stp_dither_set_ink_spread(dither, ink_spread);
      break;
    }
  stp_dither_set_density(dither, stp_get_density(nv));

  in  = stp_malloc(image_width * image_bpp);
  out = stp_malloc(image_width * out_bpp * 2);

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;
  if (escp2_lum_adjustment(model, nv))
    {
      for (i = 0; i < 49; i++)
	{
	  lum_adjustment[i] = escp2_lum_adjustment(model, nv)[i];
	  if (pt && pt->lum_adjustment)
	    lum_adjustment[i] *= pt->lum_adjustment[i];
	}
    }
  if (escp2_sat_adjustment(model, nv))
    {
      for (i = 0; i < 49; i++)
	{
	  sat_adjustment[i] = escp2_sat_adjustment(model, nv)[i];
	  if (pt && pt->sat_adjustment)
	    sat_adjustment[i] *= pt->sat_adjustment[i];
	}
    }
  if (escp2_hue_adjustment(model, nv))
    {
      for (i = 0; i < 49; i++)
	{
	  hue_adjustment[i] = escp2_hue_adjustment(model, nv)[i];
	  if (pt && pt->hue_adjustment)
	    hue_adjustment[i] += pt->hue_adjustment[i];
	}
    }

  QUANT(0);
  for (y = 0; y < out_height; y ++)
    {
      int duplicate_line = 1;
      if ((y & 63) == 0)
	image->note_progress(image, y, out_height);

      if (errline != errlast)
	{
	  errlast = errline;
	  duplicate_line = 0;
	  if (image->get_row(image, in, errline) != STP_IMAGE_OK)
	    break;
	  (*colorfunc)(nv, in, out, &zero_mask, image_width, image_bpp, cmap,
		       escp2_hue_adjustment(model, nv) ? hue_adjustment : NULL,
		       escp2_lum_adjustment(model, nv) ? lum_adjustment : NULL,
		       escp2_sat_adjustment(model, nv) ? sat_adjustment :NULL);
	}
      QUANT(1);

      stp_dither(out, y, dither, cyan, lcyan, magenta, lmagenta,
		 yellow, dyellow, black, duplicate_line, zero_mask);
      QUANT(2);

      stp_write_weave(weave, length, ydpi, model, out_width, left,
		      xdpi, physical_xdpi, cols);
      QUANT(3);
      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
	{
	  errval -= out_height;
	  errline ++;
	}
      QUANT(4);
    }
  image->progress_conclude(image);
  stp_flush_all(weave, model, out_width, left, ydpi, xdpi, physical_xdpi);
  QUANT(5);

  stp_free_dither(dither);

 /*
  * Cleanup...
  */
  escp2_deinit_printer(&init);

  stp_free_lut(nv);
  stp_free(in);
  stp_free(out);
  stp_destroy_weave(weave);

  for (i = 0; i < 7; i++)
    if (cols[i])
      stp_free((unsigned char *) cols[i]);

#ifdef QUANTIFY
  print_timers(nv);
#endif
  stp_free_vars(nv);
}

const stp_printfuncs_t stp_escp2_printfuncs =
{
  escp2_parameters,
  stp_default_media_size,
  escp2_imageable_area,
  escp2_limit,
  escp2_print,
  escp2_default_parameters,
  escp2_describe_resolution,
  stp_verify_printer_params
};

static void
flush_pass(stp_softweave_t *sw, int passno, int model, int width,
	   int hoffset, int ydpi, int xdpi, int physical_xdpi,
	   int vertical_subpass)
{
  int j;
  const stp_vars_t v = (sw->v);
  escp2_privdata_t *pd =
    (escp2_privdata_t *) stp_get_driver_data(v);
  stp_lineoff_t *lineoffs = stp_get_lineoffsets_by_pass(sw, passno);
  stp_lineactive_t *lineactive = stp_get_lineactive_by_pass(sw, passno);
  const stp_linebufs_t *bufs = stp_get_linebases_by_pass(sw, passno);
  stp_pass_t *pass = stp_get_pass_by_pass(sw, passno);
  stp_linecount_t *linecount = stp_get_linecount_by_pass(sw, passno);
  int lwidth = (width + (sw->horizontal_weave - 1)) / sw->horizontal_weave;
  int microoffset = vertical_subpass & (sw->horizontal_weave - 1);
  int advance = pass->logicalpassstart - sw->last_pass_offset -
    (sw->separation_rows - 1);
  int pos;

  advance *= pd->undersample;
  ydpi *= pd->undersample;

  if (ydpi > escp2_max_vres(model, v))
    ydpi = escp2_max_vres(model, v);
  for (j = 0; j < sw->ncolors; j++)
    {
      if (lineactive[0].v[j] > 0 ||
        escp2_has_cap(model, MODEL_MICROWEAVE_EXCEPTION,
                      MODEL_MICROWEAVE_EXCEPTION_BLACK, v))
	{
	  int nlines = linecount[0].v[j];
	  int minlines = pd->min_nozzles;
	  int extralines = 0;
	  if (nlines < minlines)
	    {
	      extralines = minlines - nlines;
	      nlines = minlines;
	    }
	  /*
	   * Set vertical position
	   */
	  if (pass->logicalpassstart > sw->last_pass_offset ||
	      pd->initial_vertical_offset != 0)
	    {
	      int a0, a1, a2, a3;
	      advance += pd->initial_vertical_offset;
	      pd->initial_vertical_offset = 0;
	      a0 = advance         & 0xff;
	      a1 = (advance >> 8)  & 0xff;
	      a2 = (advance >> 16) & 0xff;
	      a3 = (advance >> 24) & 0xff;
	      if (!escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO, v) &&
		  (sw->jets == 1 || escp2_has_cap(model, MODEL_VARIABLE_DOT,
						  MODEL_VARIABLE_NORMAL, v)))
		stp_zprintf(v, "\033(v%c%c%c%c", 2, 0, a0, a1);
	      else
		stp_zprintf(v, "\033(v%c%c%c%c%c%c", 4, 0, a0, a1, a2, a3);
	      sw->last_pass_offset = pass->logicalpassstart;
	    }

	  /*
	   * Set color where appropriate
	   */
	  if (sw->last_color != j &&
	      !escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO, v) &&
	      (sw->jets == 1 || escp2_has_cap(model, MODEL_VARIABLE_DOT,
					      MODEL_VARIABLE_NORMAL, v)))
	    {
	      if (!escp2_has_cap(model, MODEL_COLOR, MODEL_COLOR_4, v))
		stp_zprintf(v, "\033(r%c%c%c%c", 2, 0, densities[j],colors[j]);
	      else
		stp_zprintf(v, "\033r%c", colors[j]);
	      sw->last_color = j;
	    }

	  /*
	   * Set horizontal position
	   */
	  if (!escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO, v) &&
	      (xdpi <= escp2_base_resolution(model, v) ||
	       escp2_max_hres(model, v) < 1440))
	    {
	      pos = (hoffset + microoffset);
	      if (pos > 0)
		stp_zprintf(v, "\033\\%c%c", pos & 255, pos >> 8);
	    }
	  else if (escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO,v) ||
		   (escp2_has_advanced_command_set(model, v) &&
		    !(escp2_has_cap(model, MODEL_VARIABLE_DOT,
				    MODEL_VARIABLE_NORMAL, v))))
	    {
	      pos = ((hoffset * xdpi / ydpi) + microoffset);
	      if (pos > 0)
		stp_zprintf(v, "\033($%c%c%c%c%c%c", 4, 0,
			    pos & 255, (pos >> 8) & 255,
			    (pos >> 16) & 255, (pos >> 24) & 255);
	    }
	  else
	    {
	      pos = ((hoffset * escp2_max_hres(model, v) / ydpi) +
		     microoffset);
	      if (pos > 0)
		stp_zprintf(v, "\033(\\%c%c%c%c%c%c", 4, 0, 160, 5,
			    pos & 255, pos >> 8);
	    }

	  /*
	   * Issue print command
	   */
	  if (!escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO,v) &&
	      sw->jets == 1)
	    {
	      int ygap = 3600 / ydpi;
	      int xgap = 3600 / xdpi;
	      if (ydpi == 720 &&
		  escp2_has_cap(model, MODEL_720DPI_MODE, MODEL_720DPI_600, v))
		ygap *= 8;
	      stp_zprintf(v, "\033.%c%c%c%c%c%c", COMPRESSION, ygap, xgap, 1,
			  lwidth & 255, (lwidth >> 8) & 255);
	    }
	  else if (!escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO,v) &&
		   escp2_has_cap(model, MODEL_VARIABLE_DOT,
				 MODEL_VARIABLE_NORMAL,v))
	    {
	      int ygap = 3600 / ydpi;
	      int xgap = 3600 / physical_xdpi;
	      if (escp2_has_cap(model, MODEL_720DPI_MODE, MODEL_720DPI_600, v))
		ygap *= 8;
	      else if (escp2_pseudo_separation_rows(model, v) > 0)
		ygap *= escp2_pseudo_separation_rows(model, v);
	      else
		ygap *= sw->separation_rows;
	      stp_zprintf(v, "\033.%c%c%c%c%c%c", COMPRESSION, ygap, xgap,
			  nlines, lwidth & 255, (lwidth >> 8) & 255);
	    }
	  else
	    {
	      int ncolor = (densities[j] << 4) | colors[j];
	      int nwidth = sw->bitwidth * ((lwidth + 7) / 8);
	      stp_zprintf(v, "\033i%c%c%c%c%c%c%c", ncolor, COMPRESSION,
			  sw->bitwidth, nwidth & 255, (nwidth >> 8) & 255,
			  nlines & 255, (nlines >> 8) & 255);
	    }

	  /*
	   * Send the data
	   */
	  stp_zfwrite((const char *)bufs[0].v[j], lineoffs[0].v[j], 1, v);
	  if (extralines)
	    {
	      int k = 0;
	      for (k = 0; k < extralines; k++)
		{
		  int bytes_to_fill = sw->bitwidth * ((lwidth + 7) / 8);
		  int full_blocks = bytes_to_fill / 128;
		  int leftover = bytes_to_fill % 128;
		  int l = 0;
		  while (l < full_blocks)
		    {
		      stp_putc(129, v);
		      stp_putc(0, v);
		      l++;
		    }
		  if (leftover == 1)
		    {
		      stp_putc(1, v);
		      stp_putc(0, v);
		    }
		  else if (leftover > 0)
		    {
		      stp_putc(257 - leftover, v);
		      stp_putc(0, v);
		    }
		}
	    }
	  stp_putc('\r', v);
	}
      lineoffs[0].v[j] = 0;
      linecount[0].v[j] = 0;
    }

  sw->last_pass = pass->pass;
  pass->pass = -1;
}
