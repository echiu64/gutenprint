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
#ifndef WEAVETEST
#include <gimp-print.h>
#include <gimp-print-internal.h>
#include <gimp-print-intl-internal.h>
#endif

static void flush_pass(stp_softweave_t *sw, int passno, int model, int width,
		       int hoffset, int ydpi, int xdpi, int physical_xdpi,
		       int vertical_subpass);

static const int escp2_base_separation = 360;
static const int escp2_base_resolution = 720;
static const int escp2_enhanced_resolution = 720;
static const int escp2_resolution_scale = 14400;

/*
 * Printer capabilities.
 *
 * Various classes of printer capabilities are represented by bitmasks.
 */

typedef unsigned long long model_cap_t;
typedef unsigned long long model_featureset_t;

/*
 For each printhead (=color), the offset in escp2_base_separation (1/360") units is defined here.
 */
/* we really should have a define for the maximum number of colors */

typedef union {		/* number of rows for a pass */
  int v[8];		/* (really pass) */
  struct {
    int k;
    int m;
    int c;
    int y;
    int M;
    int C;
    int Y;
    int dummy;
    } p;
  } head_offset_t;

#define COLOR_JET_ARRANGEMENT_DEFAULT {{0, 0, 0, 0, 0, 0, 0, 0}}
#define COLOR_JET_ARRANGEMENT_NEW_X80 {{48, 48, 96 ,0, 0, 0, 0, 0}}

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
 * This obviously depends upon the dot size.  Experience suggests that
 * variable dot size mode (0x10) on the 870 requires the density
 * derived from the printer base and the resolution to be multiplied
 * by 3.3.  Using dot size 0x11 requires the density to be multiplied
 * by 2.2.
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

#define INKSET_4 	 0
#define INKSET_6 	 1
#define INKSET_7 	 2
#define INKSET_N	 3

#define RES_120_M 	 0
#define RES_120 	 1
#define RES_180_M 	 2
#define RES_180 	 3
#define RES_360_M 	 4
#define RES_360 	 5
#define RES_720_M 	 6
#define RES_720 	 7
#define RES_1440_720_M	 8
#define RES_1440_720	 9
#define RES_1440_1440_M	 10
#define RES_1440_1440	 11
#define RES_2880_720_M	 12
#define RES_2880_720	 13
#define RES_2880_1440_M	 14
#define RES_2880_1440	 15
#define RES_N		 16

static const int dotidmap[] = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 10 };

static int
resid2dotid(int resid)
{
  if (resid < 0 || resid >= RES_N)
    return -1;
  return dotidmap[resid];
}

static const int densidmap[] = { 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 11 };

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

typedef enum {
  COLOR_MONOCHROME,
  COLOR_CMYK,
  COLOR_CCMMYK,
  COLOR_CCMMYYK
} colormode_t;

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
  { 0.15,  0x1, 0, 1 },
  { 0.227, 0x2, 0, 2 },
/*  { 0.333, 0x3, 0, 3 }, */
  { 0.45,  0x1, 1, 1 },
  { 0.68,  0x2, 1, 2 },
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
  { 0.30,  0x1, 0, 1 },
  { 0.90,  0x1, 1, 1 },
  { 1.36,  0x2, 1, 2 }
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
  { 0.22,  0x1, 0, 1 },
  { 0.661, 0x1, 1, 1 },
  { 1.00,  0x2, 1, 2 }
};

static const escp2_variable_ink_t photo_4pl_ink =
{
  photo_4pl_dither_ranges,
  sizeof(photo_4pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};

static const stp_simple_dither_range_t photo_4pl_1440_dither_ranges[] =
{
  { 0.30,  0x1, 0, 1 },
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
  { 0.45,  0x1, 1, 1 },
  { 0.68,  0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static const escp2_variable_ink_t standard_6pl_ink =
{
  standard_6pl_dither_ranges,
  sizeof(standard_6pl_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t standard_6pl_1440_dither_ranges[] =
{
  { 0.90,  0x1, 1, 1 },
  { 1.36,  0x2, 1, 2 }
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
  { 0.29,  0x1, 1, 1 },
  { 0.68,  0x2, 1, 2 },
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
  { 1.0,   0x1, 1, 1 },
  { 3.0, 0x2, 1, 2 }
};

static const escp2_variable_ink_t standard_3pl_2880_ink =
{
  standard_3pl_2880_dither_ranges,
  sizeof(standard_3pl_2880_dither_ranges) / sizeof(stp_simple_dither_range_t),
  1.0
};


static const stp_simple_dither_range_t photo_multishot_dither_ranges[] =
{
  { 0.1097, 0x1, 0, 1 },
  { 0.227,  0x2, 0, 2 },
/*  { 0.333, 0x3, 0, 3 }, */
  { 0.28,   0x1, 1, 1 },
  { 0.58,   0x2, 1, 2 },
  { 0.85,   0x3, 1, 3 },
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
  { 0.85,  0x3, 1, 3 },
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
  &photo_cyan_ink,
  &photo_magenta_ink,
  NULL,
  NULL
};

static const escp2_variable_inkset_t escp2_6pl_standard_inks =
{
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_photo_inks =
{
  &photo_6pl_ink,
  &photo_6pl_ink,
  &standard_6pl_ink,
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
  &photo_6pl_1440_ink,
  &photo_6pl_1440_ink,
  &standard_6pl_1440_ink,
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
  &photo_pigment_ink,
  &photo_pigment_ink,
  &standard_pigment_ink,
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
  &photo_4pl_ink,
  &photo_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink
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
  &photo_4pl_1440_ink,
  &photo_4pl_1440_ink,
  &standard_4pl_1440_ink,
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
  &photo_multishot_ink,
  &photo_multishot_ink,
  &standard_multishot_ink,
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
      &escp2_6pl_standard_inks,
      &escp2_6pl_standard_inks,
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
      &escp2_pigment_photo_inks
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
    }
  },
  {
    {
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_3pl_standard_inks,
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
    }
  },
  {
    {
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
      &photo_inks
    }
  },
  {
    {
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
  0.55,
  0.6,
  0.65,
  0.65,
  0.6,
  0.55,
  0.53,
  0.5,				/* B */
  0.55,
  0.65,
  0.7,
  0.8,
  0.909,
  1.0,
  1.15,
  1.3,				/* M */
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,				/* R */
  1.2,
  1.15,
  1.1,
  1.05,
  1.0,
  1.0,
  1.0,
  1.0,				/* Y */
  0.9,
  0.8,
  0.7,
  0.6,
  0.55,
  0.5,
  0.45,
  0.36,				/* G */
  0.4,
  0.45,
  0.48,
  0.48,
  0.48,
  0.51,
  0.51,
  0.50				/* C */
};

static const double x70_lum_adjustment[49] =
{
  0.50,				/* C */
  0.55,
  0.65,
  0.85,
  0.95,
  0.85,
  0.59,
  0.60,
  0.55,				/* B */
  0.6,
  0.65,
  0.75,
  0.9,
  1.05,
  1.15,
  1.25,
  1.35,				/* M */
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,				/* R */
  1.2,
  1.15,
  1.1,
  1.05,
  1.0,
  1.0,
  1.0,
  1.0,				/* Y */
  0.93,
  0.8,
  0.8,
  0.72,
  0.65,
  0.55,
  0.47,
  0.38,				/* G */
  0.42,
  0.45,
  0.48,
  0.48,
  0.48,
  0.51,
  0.51,
  0.50				/* C */
};

static const double standard_hue_adjustment[49] =
{
  0,				/* C */
  0.17,
  0.29,
  0.38,
  0.47,
  0.52,
  0.57,
  0.62,
  0.65,				/* B */
  0.7,
  0.85,
  1.05,
  1.25,
  1.45,
  1.65,
  1.8,
  2.00,				/* M */
  2.1,
  2.2,
  2.32,
  2.45,
  2.56,
  2.65,
  2.74,
  2.83,				/* R */
  3.0,
  3.15,
  3.3,
  3.45,
  3.6,
  3.75,
  3.85,
  4.0,				/* Y */
  4.2,
  4.37,
  4.55,
  4.65,
  4.78,
  4.85,
  4.9,
  4.95,				/* G */
  5.05,
  5.15,
  5.25,
  5.35,
  5.5,
  5.65,
  5.8,
  6.0				/* C */
};

typedef struct escp2_printer
{
  model_cap_t	flags;		/* Bitmask of flags, see below */
  int 		nozzles;	/* Number of nozzles per color */
  int		nozzle_separation; /* Separation between rows, in 1/720" */
  int		black_nozzles;	/* Number of black nozzles (may be extra) */
  int		black_nozzle_separation; /* Separation between rows */
  int		xres;		/* Normal distance between dots in */
				/* softweave mode (inverse inches) */
  int		enhanced_xres;	/* Distance between dots in highest */
				/* quality modes */
  int		max_paper_width; /* Maximum paper width, in points*/
  int		max_paper_height; /* Maximum paper height, in points */
  int		left_margin;	/* Left margin, points */
  int		right_margin;	/* Right margin, points */
  int		top_margin;	/* Absolute top margin, points */
  int		bottom_margin;	/* Absolute bottom margin, points */
  int		separation_rows; /* Some printers require funky spacing */
				/* arguments in microweave mode. */
  int		pseudo_separation_rows;/* Some printers require funky */
				/* spacing arguments in softweave mode */

  		 /* The stylus 480 and 580 have an unusual arrangement of
				  color jets that need special handling */
  const head_offset_t head_offset;

  int		max_hres;
  int		max_vres;
  const escp2_dot_size_t dot_sizes;	/* Vector of dot sizes for resolutions */
  const escp2_densities_t densities;	/* List of densities for each printer */
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

#define MODEL_INIT_MASK		0xfull /* Is a special init sequence */
#define MODEL_INIT_STANDARD	0x0ull /* required for this printer, and if */
#define MODEL_INIT_NEW		0x1ull /* so, what */

#define MODEL_HASBLACK_MASK	0x10ull /* Can this printer print black ink */
#define MODEL_HASBLACK_YES	0x00ull /* when it is also printing color? */
#define MODEL_HASBLACK_NO	0x10ull /* Only the 1500 can't. */

#define MODEL_COLOR_MASK	0x60ull /* Is this a 6-color printer? */
#define MODEL_COLOR_4		0x00ull
#define MODEL_COLOR_6		0x20ull
#define MODEL_COLOR_7		0x40ull

#define MODEL_GRAYMODE_MASK	0x80ull /* Does this printer support special */
#define MODEL_GRAYMODE_NO	0x00ull /* fast black printing? */
#define MODEL_GRAYMODE_YES	0x80ull

#define MODEL_720DPI_MODE_MASK	0x300ull /* Does this printer require old */
#define MODEL_720DPI_DEFAULT	0x000ull /* or new setting for printing */
#define MODEL_720DPI_600	0x100ull /* 720 dpi?  Only matters for */
				         /* single dot size printers */

#define MODEL_VARIABLE_DOT_MASK	0xc00ull /* Does this printer support var */
#define MODEL_VARIABLE_NORMAL	0x000ull /* dot size printing? The newest */
#define MODEL_VARIABLE_4	0x400ull /* printers support multiple modes */
#define MODEL_VARIABLE_MULTI	0x800ull /* of variable dot sizes. */

#define MODEL_COMMAND_MASK	0xf000ull /* What general command set does */
#define MODEL_COMMAND_GENERIC	0x0000ull /* this printer use? */
#define MODEL_COMMAND_1998	0x1000ull
#define MODEL_COMMAND_1999	0x2000ull /* The 1999 series printers */

#define MODEL_INK_MASK		0x10000ull /* Does this printer support */
#define MODEL_INK_NORMAL	0x00000ull /* different types of inks? */
#define MODEL_INK_SELECTABLE	0x10000ull /* Only the Stylus Pro's do */

#define MODEL_ROLLFEED_MASK	0x20000ull /* Does this printer support */
#define MODEL_ROLLFEED_NO	0x00000ull /* a roll feed? */
#define MODEL_ROLLFEED_YES	0x20000ull

#define MODEL_ZEROMARGIN_MASK	0x40000ull /* Does this printer support */
#define MODEL_ZEROMARGIN_NO	0x00000ull /* zero margin mode? */
#define MODEL_ZEROMARGIN_YES	0x40000ull /* (print to the edge of the paper) */

#define MODEL_INIT		(0)
#define MODEL_HASBLACK		(1)
#define MODEL_COLOR		(2)
#define MODEL_GRAYMODE		(3)
#define MODEL_720DPI_MODE	(4)
#define MODEL_VARIABLE_DOT	(5)
#define MODEL_COMMAND		(6)
#define MODEL_INK		(7)
#define MODEL_ROLLFEED		(8)
#define MODEL_ZEROMARGIN	(9)
#define MODEL_LIMIT		(10)

static const escp2_printer_attr_t escp2_printer_attrs[] =
{
  { "init_sequence",	 0, 4 },
  { "has_black",	 4, 1 },
  { "color",		 5, 2 },
  { "graymode",		 7, 1 },
  { "720dpi_mode",	 8, 2 },
  { "variable_mode",	10, 2 },
  { "command_mode",	12, 4 },
  { "ink_types",	16, 1 },
  { "rollfeed", 	17, 1 },
  { "zero_margin",	18, 1 }
};

#define INCH(x)		(72 * x)

static const escp2_stp_printer_t model_capabilities[] =
{
  /* FIRST GENERATION PRINTERS */
  /* 0: Stylus Color */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    15, 4, 15, 4, 720, 720, INCH(17 / 2), INCH(44), 14, 14, 9, 49, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    720, 720,
    { -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .568, .568, 0, 0, 0, 0, 0, 0, 0 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 1: Stylus Color Pro/400/500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 48, 3, 720, 720, INCH(17 / 2), INCH(44), 14, 14, 0, 30, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    720, 720,
    { -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .631, .631, 0, 0, 0, 0, 0, 0, 0 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 2: Stylus Color 1500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_NO | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    1, 1, 1, 1, 720, 720, INCH(17), INCH(44), 14, 14, 9, 49, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    720, 720,
    { -2, -2, -1, -2, -1, -1, -1, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .631, 0, 0, 0, 0, 0, 0, 0, 0 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 3: Stylus Color 600 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 32, 4, 720, 360, INCH(17 / 2), INCH(44), 8, 9, 0, 30, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 4, 4, -1, 2, 2, -1, 1, -1, 1, -1, 1 },
    { 2.0, 1.3, 1.3, .775, .775, .55, .55, .275, .275, .275, .275, .138 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 4: Stylus Color 800 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 720, 360, INCH(17 / 2), INCH(44), 8, 9, 9, 40, 1, 4,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 3, 3, -1, 1, 1, -1, 4, -1, 4, -1, -1 },
    { 2.0, 1.3, 1.3, .775, .775, .55, .55, .275, .275, .275, .275, .138 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 5: Stylus Color 850 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    64, 2, 128, 1, 720, 360, INCH(17 / 2), INCH(44), 9, 9, 9, 40, 1, 4,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 3, 3, -1, 1, 1, -1, 4, -1, 4, -1, -1 },
    { 2.0, 1.3, 1.3, .775, .775, .55, .55, .275, .275, .275, .275, .138 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 6: Stylus Color 1520 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 720, 360, INCH(17), INCH(44), 8, 9, 9, 40, 1, 4,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 3, 3, -1, 1, 1, -1, 4, -1, 4, -1, -1 },
    { 2.0, 1.3, 1.3, .775, .775, .55, .55, .275, .275, .275, .275, .138 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },

  /* SECOND GENERATION PRINTERS */
  /* 7: Stylus Photo 700 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 32, 4, 720, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 30, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 3, 3, -1, -1, 1, -1, 4, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .775, .775, .55, .55, .275, .275, .275, .275, .138 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 8: Stylus Photo EX */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 32, 4, 720, 360, INCH(118 / 10), INCH(44), 9, 9, 0, 30, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 3, 3, -1, -1, 1, -1, 4, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .775, .775, .55, .55, .275, .275, .275, .275, .138 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 9: Stylus Photo */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 32, 4, 720, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 30, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    720, 720,
    { 3, 3, -1, -1, 1, -1, -1, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .775, .775, 0, 0, 0, 0, 0, 0, 0 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },

  /* THIRD GENERATION PRINTERS */
  /* 10: Stylus Color 440/460 */
  /* Thorsten Schnier has confirmed that the separation is 8.  Why on */
  /* earth anyone would use 21 nozzles when designing a print head is */
  /* completely beyond me, but there you are... */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    21, 4, 21, 4, 720, 720, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    720, 720,
    { 3, 3, -1, 1, 1, -1, -1, -1, -1, -1, -1 },
    { 3.0, 2.0, 2.0, .900, .900, 0, 0, 0, 0, 0, 0, 0 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 11: Stylus Color 640 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 64, 2, 720, 720, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 3, 3, -1, 1, 1, -1, 1, -1, 1, -1, -1 },
    { 3.0, 2.0, 2.0, .900, .900, .45, .45, .45, .45, .225, .225, .113 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 12: Stylus Color 740 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 4, 4, 0x10, 3, 0x10, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 2.0, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 13: Stylus Color 900 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    96, 2, 192, 1, 360, 180, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { -1, 1, 0x11, 1, 0x10, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .91, .91, .455 },
    &variable_3pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 14: Stylus Photo 750 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 48, 3, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 2, 0x10, 4, 0x10, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 15: Stylus Photo 1200 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    48, 3, 48, 3, 360, 360, INCH(13), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 2, 0x10, 4, 0x10, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 16: Stylus Color 860 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 17: Stylus Color 1160 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(13), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 18: Stylus Color 660 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 64, 2, 720, 720, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 8,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 3, 3, -1, 3, 0, -1, 0, -1, -1, -1, -1 },
    { 3.0, 2.0, 2.0, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 19: Stylus Color 760 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 0, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 20: Stylus Photo 720 (Australia) */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 32, 4, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 2, 0x12, 4, 0x11, -1, 0x11, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 21: Stylus Color 480 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    15, 3, 47, 3, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 30, 1, 0,
    COLOR_JET_ARRANGEMENT_NEW_X80,
    720, 720,
    { -1, -1, 0x13, -1, 0x11, -1, -1, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 22: Stylus Photo 870 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_YES),
    48, 3, 48, 3, 360, 360, INCH(17 / 2), INCH(44), 0, 0, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 4, 4, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_6color_inks, x70_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 23: Stylus Photo 1270 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_YES),
    48, 3, 48, 3, 360, 360, INCH(13), INCH(44), 0, 0, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 4, 4, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_6color_inks, x70_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 24: Stylus Color 3000 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    64, 2, 128, 1, 720, 360, INCH(17), INCH(55), 8, 9, 9, 40, 1, 4,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 3, 3, -1, 1, 1, -1, 4, -1, 4, -1, -1 },
    { 2.0, 1.3, 1.3, .775, .775, .55, .55, .275, .275, .275, .275, .138 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 25: Stylus Color 670 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 64, 2, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 3, 3, 0x12, 3, 0x11, -1, 0x11, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_6pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 26: Stylus Photo 2000P */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(13), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 2, 0x11, 4, 0x10, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .775, .852, .388, .438, .388, .438, .219, .219, .110 },
    &variable_pigment_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 27: Stylus Pro 5000 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 360, 360, INCH(13), INCH(1200), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 2, -1, 4, 0, 4, 0, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 28: Stylus Pro 7000 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 360, 360, INCH(24), INCH(1200), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 2, -1, 4, 0, 4, 0, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 29: Stylus Pro 7500 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 360, 360, INCH(24), INCH(1200), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 2, -1, 4, 0, 4, 0, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 30: Stylus Pro 9000 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 360, 360, INCH(44), INCH(1200), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 2, -1, 4, 0, 4, 0, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 31: Stylus Pro 9500 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 360, 360, INCH(44), INCH(1200), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    1440, 720,
    { 2, 2, -1, 4, 0, 4, 0, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 32: Stylus Color 777/680 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    2880, 720,
    { 0, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 33: Stylus Color 880/83 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    2880, 720,
    { 0, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 34: Stylus Color 980 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    96, 2, 192, 1, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    2880, 720,
    { -1, 1, 0x11, 1, 0x10, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .646, .73, .6, .6, 1, 1, .91, .91, .455 },
    &variable_3pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 35: Stylus Photo 780/790 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_YES),
    48, 3, 48, 3, 360, 360, INCH(17 / 2), INCH(44), 0, 0, 0, 0, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    2880, 720,
    { 4, 4, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_6color_inks, x70_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 36: Stylus Photo 890 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_YES),
    48, 3, 48, 3, 360, 360, INCH(17 / 2), INCH(44), 0, 0, 0, 0, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    2880, 720,
    { 4, 4, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_6color_inks, x70_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 37: Stylus Photo 1280/1290 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_YES),
    48, 3, 48, 3, 360, 360, INCH(13), INCH(44), 0, 0, 0, 0, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    2880, 720,
    { 4, 4, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_6color_inks, x70_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 38: Stylus Color 580 */
  {
    (MODEL_INIT_NEW | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    15, 3, 47, 3, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    COLOR_JET_ARRANGEMENT_NEW_X80,
    1440, 720,
    { -1, -1, 0x13, -1, 0x11, -1, -1, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 39: Stylus Color Pro/400/500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 48, 3, 720, 720, INCH(13), INCH(44), 14, 14, 0, 30, 1, 0,
    COLOR_JET_ARRANGEMENT_DEFAULT,
    720, 720,
    { -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .631, .631, 0, 0, 0, 0, 0, 0, 0 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
};

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
  const char *paper_type;
  const char *media_source;
  stp_vars_t v;
} escp2_init_t;

typedef struct {
  const char name[65];
  int hres;
  int vres;
  int softweave;
  int microweave;
  int vertical_passes;
  int vertical_oversample;
  int unidirectional;
  int resid;
} res_t;

static const res_t escp2_reslist[] = {
  { N_ ("180 x 120 DPI"),                            180,  120,  0, 0, 1, 1, 0, 0 },
  { N_ ("180 x 120 DPI Unidirectional"),             180,  120,  0, 0, 1, 1, 1, 0 },
  { N_ ("360 x 120 DPI"),                            360,  120,  0, 0, 1, 1, 0, 0 },
  { N_ ("360 x 120 DPI Unidirectional"),             360,  120,  0, 0, 1, 1, 1, 0 },
  { N_ ("180 DPI"),                                  180,  180,  0, 0, 1, 1, 0, 2 },
  { N_ ("180 DPI Unidirectional"),                   180,  180,  0, 0, 1, 1, 1, 2 },
  { N_ ("360 DPI"),                                  360,  360,  0, 0, 1, 1, 0, 4 },
  { N_ ("360 DPI Unidirectional"),                   360,  360,  0, 0, 1, 1, 1, 4 },
  { N_ ("360 DPI Microweave"),                       360,  360,  0, 1, 1, 1, 0, 4 },
  { N_ ("360 DPI Microweave Unidirectional"),        360,  360,  0, 1, 1, 1, 1, 4 },
  { N_ ("360 DPI Softweave"),                        360,  360,  1, 0, 1, 1, 0, 5 },
  { N_ ("360 DPI High Quality"),                     360,  360,  1, 0, 2, 1, 0, 5 },
  { N_ ("360 DPI High Quality Unidirectional"),      360,  360,  1, 0, 2, 1, 1, 5 },
  { N_ ("720 DPI Microweave"),                       720,  720,  0, 1, 1, 1, 0, 6 },
  { N_ ("720 DPI Microweave Unidirectional"),        720,  720,  0, 1, 1, 1, 1, 6 },
  { N_ ("720 DPI Softweave"),                        720,  720,  1, 0, 1, 1, 0, 7 },
  { N_ ("720 DPI Softweave Unidirectional"),         720,  720,  1, 0, 1, 1, 1, 7 },
  { N_ ("720 DPI High Quality"),                     720,  720,  1, 0, 2, 1, 0, 7 },
  { N_ ("720 DPI High Quality Unidirectional"),      720,  720,  1, 0, 2, 1, 1, 7 },
  { N_ ("720 DPI Highest Quality"),                  720,  720,  1, 0, 4, 1, 1, 7 },
  { N_ ("1440 x 720 DPI Microweave"),                1440, 720,  0, 1, 1, 1, 0, 8 },
  { N_ ("1440 x 720 DPI Microweave Unidirectional"), 1440, 720,  0, 1, 1, 1, 1, 8 },
  { N_ ("1440 x 720 DPI Softweave"),                 1440, 720,  1, 0, 1, 1, 0, 9 },
  { N_ ("1440 x 720 DPI Softweave Unidirectional"),  1440, 720,  1, 0, 1, 1, 1, 9 },
  { N_ ("1440 x 720 DPI Highest Quality"),           1440, 720,  1, 0, 2, 1, 1, 9 },
  { N_ ("1440 x 1440 DPI Softweave"),                1440, 1440, 1, 0, 1, 1, 1, 11 },
  { N_ ("1440 x 1440 DPI Highest Quality"),          1440, 1440, 1, 0, 2, 1, 1, 11 },
  { N_ ("2880 x 720 DPI Softweave"),                 2880, 720,  1, 0, 1, 1, 0, 13},
  { N_ ("2880 x 720 DPI Softweave Unidirectional"),  2880, 720,  1, 0, 1, 1, 1, 13},
  { N_ ("2880 x 1440 DPI Softweave"),                2880, 1440, 1, 0, 1, 1, 1, 13},
#ifdef HAVE_MAINTAINER_MODE
  { N_ ("1440 x 360 DPI Softweave"),                 1440, 360,  1, 0, 1, 1, 0, 7 },
  { N_ ("1440 x 360 DPI Softweave Unidirectional"),  1440, 360,  1, 0, 1, 1, 1, 7 },
  { N_ ("1440 x 360 DPI High Quality"),              1440, 360,  1, 0, 2, 1, 0, 7 },
  { N_ ("1440 x 360 DPI High Quality Uni"),          1440, 360,  1, 0, 2, 1, 1, 7 },
  { N_ ("1440 x 360 DPI Highest Quality"),           1440, 360,  1, 0, 4, 1, 1, 7 },
#endif
  { "", 0, 0, 0, 0, 0, -1 }
};

typedef struct {
  const char name[65];
  int is_color;
  int variable_dot_size;
  int dot_size_bits;
  stp_simple_dither_range_t *standard_dither;
  stp_simple_dither_range_t *photo_dither;
} ink_t;

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

typedef struct {
  const char name[65];
  int paper_feed_sequence;
  int platen_gap;
  double base_density;
  double k_lower_scale;
  double k_upper;
  const double *hue_adjustment;
  const double *lum_adjustment;
  const double *sat_adjustment;
} paper_t;

static const paper_t escp2_paper_list[] = {
  { N_ ("Plain Paper"), 1, 0, .7, .1, .5,
    NULL, plain_paper_lum_adjustment, NULL },
  { N_ ("Plain Paper Fast Load"), 5, 0, .7, .1, .5,
    NULL, plain_paper_lum_adjustment, NULL },
  { N_ ("Postcard"), 2, 0, .75, .2, .6,
    NULL, plain_paper_lum_adjustment, NULL },
  { N_ ("Glossy Film"), 3, 0, 1.0, 1.0, .999,
    NULL, plain_paper_lum_adjustment, NULL },
  { N_ ("Transparencies"), 3, 0, 1.0, 1.0, .999,
    NULL, plain_paper_lum_adjustment, NULL },
  { N_ ("Envelopes"), 4, 0, .7, .125, .5,
    NULL, plain_paper_lum_adjustment, NULL },
  { N_ ("Back Light Film"), 6, 0, 1.0, 1.0, .999,
    NULL, NULL, NULL },
  { N_ ("Matte Paper"), 7, 0, .85, 1.0, .999,
    NULL, NULL, NULL },
  { N_ ("Inkjet Paper"), 7, 0, .78, .25, .6,
    NULL, plain_paper_lum_adjustment, NULL },
  { N_ ("Photo Quality Inkjet Paper"), 7, 0, 1, 1.0, .999,
    NULL, NULL, NULL },
  { N_ ("Photo Paper"), 8, 0, 1, 1.0, .9,
    NULL, NULL, NULL },
  { N_ ("Premium Glossy Photo Paper"), 8, 0, .9, 1.0, .999,
    NULL, NULL, NULL },
  { N_ ("Premium Luster Photo Paper"), 8, 0, 1.0, 1.0, .999,
    NULL, NULL, NULL },
  { N_ ("Photo Quality Glossy Paper"), 6, 0, 1.0, 1.0, .999,
    NULL, NULL, NULL },
  { N_ ("Other"), 0, 0, .7, .125, .5,
    NULL, plain_paper_lum_adjustment, NULL },
};

static const int paper_type_count = sizeof(escp2_paper_list) / sizeof(paper_t);


static const paper_t *
get_media_type(const char *name)
{
  int i;
  for (i = 0; i < paper_type_count; i++)
    {
      if (!strcmp(name, _(escp2_paper_list[i].name)))
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
	(((1ull << escp2_printer_attrs[feature].bits) - 1ull) <<
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
escp2_left_margin(int model, const stp_vars_t v)
{
  return (model_capabilities[model].left_margin);
}

static unsigned
escp2_right_margin(int model, const stp_vars_t v)
{
  return (model_capabilities[model].right_margin);
}

static unsigned
escp2_top_margin(int model, const stp_vars_t v)
{
  return (model_capabilities[model].top_margin);
}

static unsigned
escp2_bottom_margin(int model, const stp_vars_t v)
{
  return (model_capabilities[model].bottom_margin);
}

static int
escp2_pseudo_separation_rows(int model, const stp_vars_t v)
{
  return (model_capabilities[model].pseudo_separation_rows);
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
escp2_head_offset(int model)
{
  return (& model_capabilities[model].head_offset.v[0]);
}

static void *
xzmalloc(size_t bytes)
{
  void *retval = xmalloc(bytes);
  if (retval)
    memset(retval, 0, bytes);
  return (retval);
}

/*
 * 'escp2_parameters()' - Return the parameter values for the given parameter.
 */

static char **					/* O - Parameter values */
escp2_parameters(const stp_printer_t printer,	/* I - Printer model */
                 const char *ppd_file,	/* I - PPD file (not used) */
                 const char *name,	/* I - Name of parameter */
                 int  *count)		/* O - Number of values */
{
  int		i;
  char		**valptrs;
  int		model = stp_printer_get_model(printer);
  const stp_vars_t v = stp_printer_get_printvars(printer);

  static const char *ink_types[] =
  {
    N_ ("Six Color Photo"),
    N_ ("Four Color Standard")
  };

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0)
    {
      unsigned int height_limit, width_limit;
      int papersizes = stp_known_papersizes();
      valptrs = xmalloc(sizeof(char *) * papersizes);
      *count = 0;
      width_limit = escp2_max_paper_width(model, v);
      height_limit = escp2_max_paper_height(model, v);
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t pt = stp_get_papersize_by_index(i);
	  if (strlen(stp_papersize_get_name(pt)) > 0 &&
	      stp_papersize_get_width(pt) <= width_limit &&
	      stp_papersize_get_height(pt) <= height_limit)
	    {
	      valptrs[*count] = xmalloc(strlen(stp_papersize_get_name(pt)) +1);
	      strcpy(valptrs[*count], stp_papersize_get_name(pt));
	      (*count)++;
	    }
	}
      return (valptrs);
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      const res_t *res = &(escp2_reslist[0]);
      int nozzle_width =
	(escp2_base_separation / escp2_nozzle_separation(model, v));
      valptrs =
	xmalloc(sizeof(char *) * sizeof(escp2_reslist) / sizeof(res_t));
      *count = 0;
      while(res->hres)
	{
	  if (escp2_ink_type(model, res->resid, v) != -1 &&
	      res->vres <= escp2_max_vres(model, v) &&
	      res->hres <= escp2_max_hres(model, v) &&
	      ((res->vres / nozzle_width) * nozzle_width) == res->vres)
	    {
	      int nozzles = escp2_nozzles(model, v);
	      int xdpi = res->hres;
	      int physical_xdpi =
		xdpi > escp2_enhanced_resolution ?
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
		  valptrs[*count] = xmalloc(strlen(_(res->name)) + 1);
		  strcpy(valptrs[*count], _(res->name));
		  (*count)++;
		}
	    }
	  res++;
	}
      return (valptrs);
    }
  else if (strcmp(name, "InkType") == 0)
    {
      if (escp2_has_cap(model, MODEL_COLOR, MODEL_COLOR_4, v))
	return NULL;
      else
	{
	  int ninktypes = sizeof(ink_types) / sizeof(char *);
	  valptrs = xmalloc(sizeof(char *) * ninktypes);
	  for (i = 0; i < ninktypes; i++)
	    {
	      valptrs[i] = xmalloc(strlen(_(ink_types[i])) + 1);
	      strcpy(valptrs[i], _(ink_types[i]));
	    }
	  *count = ninktypes;
	  return valptrs;
	}
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      int nmediatypes = paper_type_count;
      valptrs = xmalloc(sizeof(char *) * nmediatypes);
      for (i = 0; i < nmediatypes; i++)
	{
	  valptrs[i] = xmalloc(strlen(_(escp2_paper_list[i].name)) + 1);
	  strcpy(valptrs[i], _(escp2_paper_list[i].name));
	}
      *count = nmediatypes;
      return valptrs;
    }
  else if (strcmp(name, "InputSlot") == 0)
    {
      if (escp2_has_cap(model, MODEL_ROLLFEED, MODEL_ROLLFEED_NO,
			v))
	return NULL;
      else
	{      /* Roll Feed capable printers */
		valptrs = xmalloc(sizeof(char *) * 2);
		valptrs[0] = strdup(_("Standard"));
		valptrs[1] = strdup(_("Roll Feed"));
		*count = 2;
		return valptrs;
	}
    }
  else
    return (NULL);

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

  rollfeed = (strcmp(stp_get_media_source(v), _("Roll Feed")) == 0);

  stp_default_media_size(printer, v, &width, &height);
  *left =	escp2_left_margin(stp_printer_get_model(printer),
				  stp_printer_get_printvars(printer));
  *right =	width - escp2_right_margin(stp_printer_get_model(printer),
					   stp_printer_get_printvars(printer));

 /*
  * All printers should have 0 vertical margin capability in Roll Feed
  * mode --  They waste any paper they need automatically, and the
  * driver should print as much as the user wants
  */

  if (rollfeed) {
     *top =      height - 0;
     *bottom =   0;
  } else {
    *top =	height - escp2_top_margin(stp_printer_get_model(printer),
					  stp_printer_get_printvars(printer));
    *bottom =	escp2_bottom_margin(stp_printer_get_model(printer),
				    stp_printer_get_printvars(printer));
  }
}

static void
escp2_limit(const stp_printer_t printer,	/* I - Printer model */
	    const stp_vars_t v,  		/* I */
	    int  *width,		/* O - Left position in points */
	    int  *height)		/* O - Top position in points */
{
  *width =	escp2_max_paper_width(stp_printer_get_model(printer),
				      stp_printer_get_printvars(printer));
  *height =	escp2_max_paper_height(stp_printer_get_model(printer),
				       stp_printer_get_printvars(printer));
}

static const char *
escp2_default_resolution(const stp_printer_t printer)
{
  int model = stp_printer_get_model(printer);
  stp_vars_t v = stp_printer_get_printvars(printer);
  const res_t *res = &(escp2_reslist[0]);
  int nozzle_width = (escp2_base_separation /
		      escp2_nozzle_separation(model, v));
  while (res->hres)
    {
      if (escp2_ink_type(model, res->resid, v) != -1 &&
	  res->vres <= escp2_max_vres(model, v) &&
	  res->hres <= escp2_max_hres(model, v) &&
	  ((res->vres / nozzle_width) * nozzle_width) == res->vres)
	{
	  if (res->vres == 360 && res->hres == 360)
	    return _(res->name);
	}
      res++;
    }
  return NULL;
}

static void
escp2_describe_resolution(const stp_printer_t printer,
			  const char *resolution, int *x, int *y)
{
  int model = stp_printer_get_model(printer);
  stp_vars_t v = stp_printer_get_printvars(printer);
  const res_t *res = &(escp2_reslist[0]);
  int nozzle_width = escp2_base_separation / escp2_nozzle_separation(model, v);
  while (res->hres)
    {
      if (escp2_ink_type(model, res->resid, v) != -1 &&
	  res->vres <= escp2_max_vres(model, v) &&
	  res->hres <= escp2_max_hres(model, v) &&
	  ((res->vres / nozzle_width) * nozzle_width) == res->vres &&
	  !strcmp(resolution, _(res->name)))
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
escp2_reset_printer(const stp_vars_t v, escp2_init_t *init)
{
  /*
   * Hack that seems to be necessary for these silly things to recognize
   * the input.  It only needs to be done once per printer evidently, but
   * it needs to be done.
   */
  if (escp2_has_cap(init->model, MODEL_INIT, MODEL_INIT_NEW, init->v))
    stp_zprintf(v, "%c%c%c\033\001@EJL 1284.4\n@EJL     \n\033@", 0, 0, 0);

  stp_puts("\033@", v); 				/* ESC/P2 reset */
}

static void
escp2_set_remote_sequence(const stp_vars_t v, escp2_init_t *init)
{
  /* Magic remote mode commands, whatever they do */
  if (escp2_has_cap(init->model, MODEL_COMMAND, MODEL_COMMAND_1999,
		    init->v))
    {
      int feed_sequence = 0;
      const paper_t *p = get_media_type(init->paper_type);
      if (p)
	feed_sequence = p->paper_feed_sequence;
      stp_zprintf(v, /* Enter remote mode */
	      "\033(R\010%c%cREMOTE1"
	      /* Function unknown */
	      "PM\002%c%c%c"
	      /* Set mechanism sequence */
	      "SN\003%c%c%c%c",
	      0, 0,
	      0, 0, 0,
	      0, 0, 0, feed_sequence);
      if (escp2_has_cap(init->model, MODEL_ZEROMARGIN,
			MODEL_ZEROMARGIN_YES, init->v))
	stp_zprintf(v, /* Set zero-margin print mode */
		"FP\003%c%c\260\377", 0, 0);

      /* set up Roll-Feed options on appropriate printers
	 (tested for STP 870, which has no cutter) */
      if (escp2_has_cap(init->model, MODEL_ROLLFEED,
			MODEL_ROLLFEED_YES, init->v))
	{
	  if(strcmp(init->media_source,_("Roll Feed")) == 0)
	    stp_zprintf(v, /* Set Roll Feed mode */
		    "IR\002%c%c%c"
		    "EX\006%c%c%c%c%c%c%c",
		    0, 0, 1,
		    0, 0,0,0,0,5,1);
	  else
	    stp_zprintf(v, /* Set non-Roll Feed mode */
		    "IR\002%c%c%c"
		    "EX\006%c%c%c%c%c%c%c",
		    0, 0, 3,
		    0, 0, 0, 0, 0, 5, 0);
	}

      stp_zprintf(v, /* Exit remote mode */
	      "\033%c%c%c", 0, 0, 0);
    }
}

static void
escp2_set_graphics_mode(const stp_vars_t v, escp2_init_t *init)
{
  stp_zfwrite("\033(G\001\000\001", 6, 1, v);	/* Enter graphics mode */
}

static void
escp2_set_resolution(const stp_vars_t v, escp2_init_t *init)
{
  if (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT,
		     MODEL_VARIABLE_NORMAL, init->v)) &&
      init->use_softweave)
    {
      int hres = escp2_max_hres(init->model, init->v);
      stp_zprintf(v, "\033(U\005%c%c%c%c%c%c", 0, hres / init->ydpi,
	      hres / init->ydpi, hres / init->xdpi, hres % 256, hres / 256);
    }
  else
    stp_zprintf(v, "\033(U\001%c%c", 0, 3600 / init->ydpi);
}

static void
escp2_set_color(const stp_vars_t v, escp2_init_t *init)
{
  if (escp2_has_cap(init->model, MODEL_GRAYMODE, MODEL_GRAYMODE_YES,
		    init->v))
    stp_zprintf(v, "\033(K\002%c%c%c", 0, 0,
	    (init->output_type == OUTPUT_GRAY ? 1 : 2));
}

static void
escp2_set_microweave(const stp_vars_t v, escp2_init_t *init)
{
  stp_zprintf(v, "\033(i\001%c%c", 0, init->use_microweave);
}

static void
escp2_set_printhead_speed(const stp_vars_t v, escp2_init_t *init)
{
  if (init->unidirectional)
    {
      stp_zprintf(v, "\033U%c", 1);
      if (init->xdpi > 720)		/* Slow mode if available */
	stp_zprintf(v, "\033(s%c%c%c", 1, 0, 2);
    }
  else
    stp_zprintf(v, "\033U%c", 0);
}

static void
escp2_set_dot_size(const stp_vars_t v, escp2_init_t *init)
{
  /* Dot size */
  int drop_size = escp2_ink_type(init->model, init->resid, init->v);
  if (drop_size >= 0)
    stp_zprintf(v, "\033(e\002%c%c%c", 0, 0, drop_size);
}

static void
escp2_set_page_height(const stp_vars_t v, escp2_init_t *init)
{
  int l = init->ydpi * init->page_height / 72;
  if (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT,
		      MODEL_VARIABLE_NORMAL, init->v)) &&
      init->use_softweave)
    stp_zprintf(v, "\033(C\004%c%c%c%c%c", 0,
	    l & 0xff, (l >> 8) & 0xff, (l >> 16) & 0xff, (l >> 24) & 0xff);
  else
    stp_zprintf(v, "\033(C\002%c%c%c", 0, l & 255, l >> 8);
}

static void
escp2_set_margins(const stp_vars_t v, escp2_init_t *init)
{
  int l = init->ydpi * (init->page_height - init->page_bottom) / 72;
  int t = init->ydpi * (init->page_height - init->page_top) / 72;
  if (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT,
		      MODEL_VARIABLE_NORMAL, init->v)) &&
      init->use_softweave)
    {
      if (escp2_has_cap(init->model, MODEL_COLOR, MODEL_COLOR_6, init->v))
	stp_zprintf(v, "\033(c\010%c%c%c%c%c%c%c%c%c", 0,
		t & 0xff, t >> 8, (t >> 16) & 0xff, (t >> 24) & 0xff,
		l & 0xff, l >> 8, (l >> 16) & 0xff, (l >> 24) & 0xff);
      else
	stp_zprintf(v, "\033(c\004%c%c%c%c%c", 0,
		t & 0xff, t >> 8, l & 0xff, l >> 8);
    }
  else
    stp_zprintf(v, "\033(c\004%c%c%c%c%c", 0,
	    t & 0xff, t >> 8, l & 0xff, l >> 8);
}

static void
escp2_set_form_factor(const stp_vars_t v, escp2_init_t *init)
{
  int page_width = init->page_width * init->ydpi / 72;
  int page_height = init->page_height * init->ydpi / 72;

  if (escp2_has_cap(init->model, MODEL_ZEROMARGIN, MODEL_ZEROMARGIN_YES,
		    init->v))
      /* Make the page 2/10" wider (probably ignored by the printer anyway) */
      page_width += 144 * 720 / init->xdpi;

  if (escp2_has_cap(init->model, MODEL_COMMAND, MODEL_COMMAND_1999,
		    init->v))
    stp_zprintf(v, "\033(S\010%c%c%c%c%c%c%c%c%c", 0,
	    ((page_width >> 0) & 0xff), ((page_width >> 8) & 0xff),
	    ((page_width >> 16) & 0xff), ((page_width >> 24) & 0xff),
	    ((page_height >> 0) & 0xff), ((page_height >> 8) & 0xff),
	    ((page_height >> 16) & 0xff), ((page_height >> 24) & 0xff));
}

static void
escp2_set_printhead_resolution(const stp_vars_t v, escp2_init_t *init)
{
  if (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT,
		      MODEL_VARIABLE_NORMAL, init->v)) &&
      init->use_softweave)
    {
      int xres;
      int nozzle_separation;
      if (init->xdpi > escp2_enhanced_resolution)
	xres = escp2_enhanced_xres(init->model, init->v);
      else
	xres = escp2_xres(init->model, init->v);
      if (init->output_type == OUTPUT_GRAY)
	nozzle_separation = escp2_black_nozzle_separation(init->model,
							  init->v);
      else
	nozzle_separation = escp2_nozzle_separation(init->model, init->v);
      /* Magic resolution cookie */
      stp_zprintf(v, "\033(D%c%c%c%c%c%c", 4, 0, escp2_resolution_scale % 256,
	      escp2_resolution_scale / 256,
	      nozzle_separation * escp2_resolution_scale / escp2_base_separation,
	      escp2_resolution_scale / xres);
    }
}

static void
escp2_init_printer(const stp_vars_t v, escp2_init_t *init)
{
  if (init->ydpi > escp2_max_vres(init->model, init->v))
    init->ydpi = escp2_max_vres(init->model, init->v);

  escp2_reset_printer(v, init);
  escp2_set_remote_sequence(v, init);
  escp2_set_graphics_mode(v, init);
  escp2_set_resolution(v, init);
  escp2_set_color(v, init);
  escp2_set_microweave(v, init);
  escp2_set_printhead_speed(v, init);
  escp2_set_dot_size(v, init);
  escp2_set_page_height(v, init);
  escp2_set_margins(v, init);
  escp2_set_form_factor(v, init);
  escp2_set_printhead_resolution(v, init);
}

static void
escp2_deinit_printer(const stp_vars_t v, escp2_init_t *init)
{
  stp_puts(/* Eject page */
        "\014"
        /* ESC/P2 reset */
        "\033@", v);
  if (escp2_has_cap(init->model, MODEL_COMMAND, MODEL_COMMAND_1999,
		    init->v))
    {
      stp_zprintf(v, /* Enter remote mode */
	      "\033(R\010%c%cREMOTE1", 0, 0);
      /* set up Roll-Feed options on appropriate printers
	 (tested for STP 870, which has no cutter) */
      if (escp2_has_cap(init->model, MODEL_ROLLFEED,
			MODEL_ROLLFEED_YES, init->v))
	{
	  if(strcmp(init->media_source,_("Roll Feed")) == 0)
	    stp_zprintf(v, /* End Roll Feed mode */
		    "IR\002%c%c%c", 0, 0, 0);
	  else
	    stp_zprintf(v, /* End non-Roll Feed mode */
		    "IR\002%c%c%c", 0, 0, 2);
	}
      stp_zprintf(v, /* Load settings from NVRAM */
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
  int 		output_type = stp_get_output_type(v);
  int		orientation = stp_get_orientation(v);
  const char	*ink_type = stp_get_ink_type(v);
  double	scaling = stp_get_scaling(v);
  const char	*media_source = stp_get_media_source(v);
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  int		resid;
  int		physical_ydpi;
  int		physical_xdpi;
  int		i;
  int		n;		/* Output number */
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned char	*in,		/* Input pixels */
		*black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*yellow,	/* Yellow bitmap data */
		*lcyan,		/* Light cyan bitmap data */
		*lmagenta,	/* Light magenta bitmap data */
		*dyellow;	/* Dark Yellow bitmap data */
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
  int           image_height,
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
  int		use_6color = 0;
  int		use_7color = 0;
  const res_t 	*res;
  int		bits;
  void *	weave = NULL;
  void *	dither;
  colormode_t colormode = COLOR_CCMMYK;
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
  const int *offsetPtr;
  int maxHeadOffset;
  double lum_adjustment[49], sat_adjustment[49], hue_adjustment[49];
  int ncolors = 0;

  separation_rows = escp2_separation_rows(model, nv);
  max_vres = escp2_max_vres(model, nv);
  if (escp2_has_cap(model, MODEL_COLOR, MODEL_COLOR_6, nv) &&
      strcmp(ink_type, _("Four Color Standard")) != 0 &&
      stp_get_image_type(nv) != IMAGE_MONOCHROME)
    use_6color = 1;

  if (escp2_has_cap(model, MODEL_COLOR, MODEL_COLOR_7, nv) &&
      stp_get_image_type(nv) != IMAGE_MONOCHROME)
    {
      if (strcmp(ink_type, _("Six Color Photo")) == 0)
	use_6color = 1;
      else if (strcmp(ink_type, _("Seven Color Enhanced")) == 0)
	use_7color = 1;
    }

  if (stp_get_image_type(nv) == IMAGE_MONOCHROME)
    {
      colormode = COLOR_MONOCHROME;
      output_type = OUTPUT_GRAY;
      bits = 1;
    }
  else if (output_type == OUTPUT_GRAY)
    colormode = COLOR_MONOCHROME;
  else if (use_7color)
    colormode = COLOR_CCMMYYK;
  else if (use_6color)
    colormode = COLOR_CCMMYK;
  else
    colormode = COLOR_CMYK;

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
  for (res = &escp2_reslist[0];;res++)
    {
      if (!strcmp(resolution, _(res->name)))
	{
	  use_softweave = res->softweave;
	  use_microweave = res->microweave;
	  if (!use_softweave)
	    max_vres = escp2_base_resolution;
	  xdpi = res->hres;
	  ydpi = res->vres;
	  resid = res->resid;
	  vertical_passes = res->vertical_passes;
	  vertical_oversample = res->vertical_oversample;
	  unidirectional = res->unidirectional;
	  if (!use_softweave)
	    physical_xdpi = xdpi <= 720 ? xdpi : 720;
	  else if (xdpi > escp2_enhanced_resolution)
	    physical_xdpi = escp2_enhanced_xres(model, nv);
	  else
	    physical_xdpi = escp2_xres(model, nv);
	  if (use_softweave)
	    horizontal_passes = xdpi / physical_xdpi;
	  else
	    horizontal_passes = xdpi / escp2_base_resolution;
	  if (horizontal_passes == 0)
	    horizontal_passes = 1;
	  if (use_softweave)
	    {
	      if (output_type == OUTPUT_GRAY)
		{
		  nozzles = escp2_black_nozzles(model, nv);
		  if (nozzles == 0)
		    {
		      nozzle_separation = escp2_nozzle_separation(model, nv);
		      nozzles = escp2_nozzles(model, nv);
		    }
		  else
		    nozzle_separation =
		      escp2_black_nozzle_separation(model, nv);
		}
	      else
		{
		  nozzle_separation = escp2_nozzle_separation(model, nv);
		  nozzles = escp2_nozzles(model, nv);
		}
	      if (ydpi > escp2_base_separation)
		nozzle_separation = nozzle_separation * ydpi /
		  escp2_base_separation;
	    }
	  else
	    {
	      nozzles = 1;
	      nozzle_separation = 1;
	    }

	  offsetPtr = escp2_head_offset(model);
	  maxHeadOffset = 0;
	  for(i=0; i<8; i++)
	    {
	    if (ydpi > escp2_base_separation)
	      head_offset[i] = offsetPtr[i] * ydpi / escp2_base_separation;
	    else
	      head_offset[i] = offsetPtr[i];
	    if(head_offset[i] > maxHeadOffset)
	      maxHeadOffset = head_offset[i];
	    }

	  break;
	}
      else if (!strcmp(resolution, ""))
	{
	  return;
	}
    }
  if (!(escp2_has_cap(model, MODEL_VARIABLE_DOT, MODEL_VARIABLE_NORMAL, nv))
      && use_softweave)
    bits = 2;
  else
    bits = 1;

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
  init.ydpi = ydpi;
  init.xdpi = xdpi;
  init.use_softweave = use_softweave;
  init.use_microweave = use_microweave;
  init.page_height = page_true_height;
  init.page_width = page_width;
  init.page_top = page_top;

   /* adjust bottom margin for a 480 like head configuration */
  init.page_bottom = page_bottom - maxHeadOffset*72/ydpi;
  if((maxHeadOffset*72 % ydpi) != 0)
    init.page_bottom -= 1;
  if(init.page_bottom < 0)
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

  escp2_init_printer(v, &init);

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  physical_ydpi = ydpi;
  if (ydpi > max_vres)
    physical_ydpi = max_vres;

  left = physical_ydpi * left / 72;

 /*
  * Adjust for zero-margin printing...
  */

  if (escp2_has_cap(model, MODEL_ZEROMARGIN, MODEL_ZEROMARGIN_YES, nv))
    {
     /*
      * In zero-margin mode, the origin is about 3/20" to the left of the
      * paper's left edge.
      */
      left += 92 * physical_ydpi / max_vres;
    }

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  if (output_type == OUTPUT_GRAY)
  {
    black   = xzmalloc(length * bits);
    cyan    = NULL;
    magenta = NULL;
    lcyan    = NULL;
    lmagenta = NULL;
    yellow  = NULL;
    dyellow = NULL;
  }
  else
  {
    cyan    = xzmalloc(length * bits);
    magenta = xzmalloc(length * bits);
    yellow  = xzmalloc(length * bits);

    if (escp2_has_cap(model, MODEL_HASBLACK, MODEL_HASBLACK_YES, nv))
      black = xzmalloc(length * bits);
    else
      black = NULL;
    switch (colormode)
      {
      case COLOR_CCMMYYK:
	lcyan = xzmalloc(length * bits);
	lmagenta = xzmalloc(length * bits);
	dyellow = xzmalloc(length * bits);
	break;
      case COLOR_CCMMYK:
	lcyan = xzmalloc(length * bits);
	lmagenta = xzmalloc(length * bits);
	dyellow = NULL;
	break;
      default:
	lcyan = NULL;
	lmagenta = NULL;
	dyellow = NULL;
	break;
    }
  }
  cols[0] = black;
  cols[1] = magenta;
  cols[2] = cyan;
  cols[3] = yellow;
  cols[4] = lmagenta;
  cols[5] = lcyan;
  cols[6] = dyellow;

  switch (colormode)
    {
    case COLOR_MONOCHROME:
      ncolors = 1;
      break;
    case COLOR_CMYK:
      ncolors = 4;
      break;
    case COLOR_CCMMYK:
      ncolors = 6;
      break;
    case COLOR_CCMMYYK:
      ncolors = 7;
      break;
    }

  /* Epson printers are currently all 720 physical dpi vertically */
  weave = stp_initialize_weave(nozzles, nozzle_separation,
			       horizontal_passes, vertical_passes,
			       vertical_oversample, ncolors, bits,
			       (out_width * physical_xdpi / physical_ydpi),
			       out_height, separation_rows,
			       top * physical_ydpi / 72,
			       page_height * physical_ydpi / 72,
			       1, head_offset, nv, flush_pass);

  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */
  pt = get_media_type(stp_get_media_type(nv));
  if (pt)
    stp_set_density(nv, stp_get_density(nv) * pt->base_density);
  else				/* Can't find paper type? Assume plain */
    stp_set_density(nv, stp_get_density(nv) * .5);
  stp_set_density(nv, stp_get_density(nv) * escp2_density(model, resid, nv));
  if (stp_get_density(nv) > 1.0)
    stp_set_density(nv, 1.0);
  if (colormode == COLOR_MONOCHROME)
    stp_set_gamma(nv, stp_get_gamma(nv) / .8);
  stp_compute_lut(256, nv);

 /*
  * Output the page...
  */

  if (xdpi > ydpi)
    dither = stp_init_dither(image_width, out_width, 1, xdpi / ydpi, nv);
  else
    dither = stp_init_dither(image_width, out_width, ydpi / xdpi, 1, nv);

  stp_dither_set_black_levels(dither, 1.0, 1.0, 1.0);
  if (use_6color || use_7color)
    k_lower = .4 / bits + .1;
  else
    k_lower = .25 / bits;
  if (pt)
    {
      k_lower *= pt->k_lower_scale;
      k_upper = pt->k_upper;
    }
  else
    {
      k_lower *= .5;
      k_upper = .5;
    }
  stp_dither_set_black_lower(dither, k_lower);
  stp_dither_set_black_upper(dither, k_upper);
  if (bits == 2)
    {
      if (use_6color || use_7color)
	stp_dither_set_adaptive_divisor(dither, 8);
      else
	stp_dither_set_adaptive_divisor(dither, 4);
    }
  else
    stp_dither_set_adaptive_divisor(dither, 4);

  inks = escp2_inks(model, resid, use_7color ? 7 : (use_6color ? 6 : 4), bits,
		    nv);
  if (inks)
    for (i = 0; i < NCOLORS; i++)
      if ((*inks)[i])
	stp_dither_set_ranges(dither, i, (*inks)[i]->count, (*inks)[i]->range,
			  (*inks)[i]->density * pt->k_upper *
			      stp_get_density(nv));

  if (bits == 2)
    {
      if (use_6color || use_7color)
	stp_dither_set_transition(dither, .7);
      else
	stp_dither_set_transition(dither, .5);
    }
  if (!strcmp(stp_get_dither_algorithm(nv), _("Ordered")))
    stp_dither_set_transition(dither, 1);

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

  in  = xmalloc(image_width * image_bpp);
  out = xmalloc(image_width * out_bpp * 2);

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;
  if (escp2_lum_adjustment(model, nv))
    {
      int k;
      for (k = 0; k < 49; k++)
	{
	  lum_adjustment[k] = escp2_lum_adjustment(model, nv)[k];
	  if (pt->lum_adjustment)
	    lum_adjustment[k] *= pt->lum_adjustment[k];
	}
    }
  if (escp2_sat_adjustment(model, nv))
    {
      int k;
      for (k = 0; k < 49; k++)
	{
	  sat_adjustment[k] = escp2_sat_adjustment(model, nv)[k];
	  if (pt->sat_adjustment)
	    sat_adjustment[k] *= pt->sat_adjustment[k];
	}
    }
  if (escp2_hue_adjustment(model, nv))
    {
      int k;
      for (k = 0; k < 49; k++)
	{
	  hue_adjustment[k] = escp2_hue_adjustment(model, nv)[k];
	  if (pt->hue_adjustment)
	    hue_adjustment[k] += pt->hue_adjustment[k];
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
      image->get_row(image, in, errline);
      (*colorfunc)(in, out, image_width, image_bpp, cmap, nv,
		   escp2_hue_adjustment(model, nv) ? hue_adjustment : NULL,
		   escp2_lum_adjustment(model, nv) ? lum_adjustment : NULL,
		   escp2_sat_adjustment(model, nv) ? sat_adjustment : NULL);
    }
    QUANT(1);

    stp_dither(out, y, dither, cyan, lcyan, magenta, lmagenta,
	       yellow, dyellow, black, duplicate_line);
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
  escp2_deinit_printer(nv, &init);

  stp_free_lut(nv);
  free(in);
  free(out);
  stp_destroy_weave(weave);

  if (black != NULL)
    free(black);
  if (cyan != NULL)
    free(cyan);
  if (magenta != NULL)
    free(magenta);
  if (yellow != NULL)
    free(yellow);
  if (lcyan != NULL)
    free(lcyan);
  if (lmagenta != NULL)
    free(lmagenta);
  if (dyellow != NULL)
    free(dyellow);

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
  escp2_default_resolution,
  escp2_describe_resolution,
  stp_verify_printer_params
};

/*
 * A fair bit of this code is duplicated from escp2_write.  That's rather
 * a pity.  It's also not correct for any but the 6-color printers.  One of
 * these days I'll unify it.
 */
static void
flush_pass(stp_softweave_t *sw, int passno, int model, int width,
	   int hoffset, int ydpi, int xdpi, int physical_xdpi,
	   int vertical_subpass)
{
  int j;
  const stp_vars_t v = (sw->v);
  stp_lineoff_t *lineoffs = stp_get_lineoffsets_by_pass(sw, passno);
  stp_lineactive_t *lineactive = stp_get_lineactive_by_pass(sw, passno);
  const stp_linebufs_t *bufs = stp_get_linebases_by_pass(sw, passno);
  stp_pass_t *pass = stp_get_pass_by_pass(sw, passno);
  stp_linecount_t *linecount = stp_get_linecount_by_pass(sw, passno);
  int lwidth = (width + (sw->horizontal_weave - 1)) / sw->horizontal_weave;
  int microoffset = vertical_subpass & (sw->horizontal_weave - 1);
  int advance = pass->logicalpassstart - sw->last_pass_offset -
    (sw->separation_rows - 1);

  if (ydpi > escp2_max_vres(model, v))
    ydpi = escp2_max_vres(model, v);
  for (j = 0; j < sw->ncolors; j++)
    {
      if (lineactive[0].v[j] == 0)
	{
	  lineoffs[0].v[j] = 0;
	  linecount[0].v[j] = 0;
	  continue;
	}
      if (pass->logicalpassstart > sw->last_pass_offset)
	{
	  int a0 = advance         % 256;
	  int a1 = (advance >> 8)  % 256;
	  int a2 = (advance >> 16) % 256;
	  int a3 = (advance >> 24) % 256;
	  if (sw->jets == 1 || escp2_has_cap(model, MODEL_VARIABLE_DOT,
					     MODEL_VARIABLE_NORMAL, v))
	    stp_zprintf(v, "\033(v\002%c%c%c", 0, a0, a1);
	  else
	    stp_zprintf(v, "\033(v\004%c%c%c%c%c", 0, a0, a1, a2, a3);
	  sw->last_pass_offset = pass->logicalpassstart;
	}
      if (sw->last_color != j)
	{
	  if (sw->jets > 1 && !escp2_has_cap(model, MODEL_VARIABLE_DOT,
					     MODEL_VARIABLE_NORMAL, v))
	    ;
	  else if (!escp2_has_cap(model, MODEL_COLOR, MODEL_COLOR_4, v))
	    stp_zprintf(v, "\033(r\002%c%c%c", 0, densities[j], colors[j]);
	  else
	    stp_zprintf(v, "\033r%c", colors[j]);
	  sw->last_color = j;
	}
      if (escp2_max_hres(model, v) >= 1440 && xdpi > escp2_base_resolution)
	{
	  if (escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_1999, v) &&
	      !(escp2_has_cap(model, MODEL_VARIABLE_DOT,
			      MODEL_VARIABLE_NORMAL, v)))
	    {
	      int pos = ((hoffset * xdpi / ydpi) + microoffset);
	      if (pos > 0)
		stp_zprintf(v, "\033($%c%c%c%c%c%c", 4, 0,
			pos & 255, (pos >> 8) & 255,
			(pos >> 16) & 255, (pos >> 24) & 255);
	    }
	  else
	    {
	      int pos = ((hoffset * escp2_max_hres(model, v) / ydpi) +
			 microoffset);
	      if (pos > 0)
		stp_zprintf(v, "\033(\\%c%c%c%c%c%c", 4, 0, 160, 5,
			pos & 255, pos >> 8);
	    }
	}
      else
	{
	  int pos = (hoffset + microoffset);
	  if (pos > 0)
	    stp_zprintf(v, "\033\\%c%c", pos & 255, pos >> 8);
	}
      if (sw->jets == 1)
	{
	  if (ydpi == 720)
	    {
	      if (escp2_has_cap(model, MODEL_720DPI_MODE,
				MODEL_720DPI_600, v))
		stp_zfwrite("\033.\001\050\005\001", 6, 1, v);
	      else
		stp_zfwrite("\033.\001\005\005\001", 6, 1, v);
	    }
	  else
	    stp_zprintf(v, "\033.\001%c%c\001", 3600 / ydpi, 3600 / xdpi);
	  stp_putc(lwidth & 255, v);	/* Width of raster line in pixels */
	  stp_putc(lwidth >> 8, v);
	}
      else if (escp2_has_cap(model,MODEL_VARIABLE_DOT,MODEL_VARIABLE_NORMAL,v))
	{
	  int ydotsep = 3600 / ydpi;
	  int xdotsep = 3600 / physical_xdpi;
	  if (escp2_has_cap(model, MODEL_720DPI_MODE, MODEL_720DPI_600, v))
	    stp_zprintf(v, "\033.%c%c%c%c", 1, 8 * ydotsep, xdotsep,
			linecount[0].v[j]);
	  else if (escp2_pseudo_separation_rows(model, v) > 0)
	    stp_zprintf(v, "\033.%c%c%c%c", 1,
			ydotsep * escp2_pseudo_separation_rows(model, v),
			xdotsep, linecount[0].v[j]);
	  else
	    stp_zprintf(v, "\033.%c%c%c%c", 1,
			ydotsep * sw->separation_rows,
			xdotsep, linecount[0].v[j]);
	  stp_putc(lwidth & 255, v);	/* Width of raster line in pixels */
	  stp_putc(lwidth >> 8, v);
	}
      else
	{
	  int ncolor = (densities[j] << 4) | colors[j];
	  int nlines = linecount[0].v[j];
	  int nwidth = sw->bitwidth * ((lwidth + 7) / 8);
	  stp_zprintf(v, "\033i%c%c%c%c%c%c%c", ncolor, 1, sw->bitwidth,
		      nwidth & 255, nwidth >> 8, nlines & 255, nlines >> 8);
	}

      stp_zfwrite(bufs[0].v[j], lineoffs[0].v[j], 1, v);
      stp_putc('\r', v);
      lineoffs[0].v[j] = 0;
      linecount[0].v[j] = 0;
    }

  sw->last_pass = pass->pass;
  pass->pass = -1;
}
