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
#include "print.h"
#endif

#ifndef __GNUC__
#  define inline
#endif /* !__GNUC__ */

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

#ifndef WEAVETEST

static void
escp2_write_microweave(FILE *, const unsigned char *,
		       const unsigned char *, const unsigned char *,
		       const unsigned char *, const unsigned char *,
		       const unsigned char *, const unsigned char *,
		       int, int, int, int, int, int, int);
static void *initialize_weave(int jets, int separation, int oversample,
			      int horizontal, int vertical,
			      colormode_t colormode, int width, int linewidth,
			      int lineheight, int vertical_row_separation,
			      int first_line, int phys_lines, int strategy);
static void escp2_flush_all(void *, int model, int width, int hoffset,
			    int ydpi, int xdpi, int physical_xdpi, FILE *prn);
static void
escp2_write_weave(void *, FILE *, int, int, int, int, int, int, int,
		  const unsigned char *c, const unsigned char *m,
		  const unsigned char *y, const unsigned char *k,
		  const unsigned char *C, const unsigned char *M,
		  const unsigned char *DY);
static void escp2_init_microweave(int);
static void escp2_free_microweave(void);

static void destroy_weave(void *);

/*
 * We really need to get away from this silly static nonsense...
 */
#define PHYSICAL_BPI 720
#define MAX_OVERSAMPLED 8
#define MAX_BPP 2
#define BITS_PER_BYTE 8
#define COMPBUFWIDTH (PHYSICAL_BPI * MAX_OVERSAMPLED * MAX_BPP * \
	MAX_CARRIAGE_WIDTH / BITS_PER_BYTE)

static int escp2_base_separation = 360;
static int escp2_base_resolution = 720;
static int escp2_enhanced_resolution = 720;
static int escp2_resolution_scale = 14400;

/*
 * Printer capabilities.
 *
 * Various classes of printer capabilities are represented by bitmasks.
 */

typedef unsigned long long model_cap_t;
typedef unsigned long long model_featureset_t;

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
  simple_dither_range_t *range;
  int count;
  double density;
} escp2_variable_ink_t;

typedef escp2_variable_ink_t *escp2_variable_inkset_t[NCOLORS];

#define INKTYPE_SINGLE	 0
#define INKTYPE_VARIABLE 1
#define INKTYPE_N	 2

#define INKSET_4 	 0
#define INKSET_6 	 1
#define INKSET_7 	 2
#define INKSET_N	 3

#define RES_180_M 	 0
#define RES_180 	 1
#define RES_360_M 	 2
#define RES_360 	 3
#define RES_720_M 	 4
#define RES_720 	 5
#define RES_1440_720_M	 6
#define RES_1440_720	 7
#define RES_1440_1440_M	 8
#define RES_1440_1440	 9
#define RES_2880_720_M	 10
#define RES_2880_720	 11
#define RES_2880_1440_M	 12
#define RES_2880_1440	 13
#define RES_N		 14

static int dotidmap[] = { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 9, 10 };

static int
resid2dotid(int resid)
{
  if (resid < 0 || resid >= RES_N)
    return -1;
  return dotidmap[resid];
}

static int densidmap[] = { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 11 };

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

typedef escp2_variable_inkset_t *escp2_variable_inklist_t[INKTYPE_N][INKSET_N][RES_N / 2];


static simple_dither_range_t photo_cyan_dither_ranges[] =
{
  { 0.27, 0x1, 0, 1 },
  { 1.0,  0x1, 1, 1 }
};

static escp2_variable_ink_t photo_cyan_ink =
{
  photo_cyan_dither_ranges,
  sizeof(photo_cyan_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};

static simple_dither_range_t photo_magenta_dither_ranges[] =
{
  { 0.35, 0x1, 0, 1 },
  { 1.0,  0x1, 1, 1 }
};

static escp2_variable_ink_t photo_magenta_ink =
{
  photo_magenta_dither_ranges,
  sizeof(photo_magenta_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t photo_6pl_dither_ranges[] =
{
  { 0.15,  0x1, 0, 1 },
  { 0.227, 0x2, 0, 2 },
/*  { 0.333, 0x3, 0, 3 }, */
  { 0.45,  0x1, 1, 1 },
  { 0.68,  0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static escp2_variable_ink_t photo_6pl_ink =
{
  photo_6pl_dither_ranges,
  sizeof(photo_6pl_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t photo_6pl_1440_dither_ranges[] =
{
  { 0.30,  0x1, 0, 1 },
  { 0.90,  0x1, 1, 1 },
  { 1.36,  0x2, 1, 2 }
};

static escp2_variable_ink_t photo_6pl_1440_ink =
{
  photo_6pl_1440_dither_ranges,
  sizeof(photo_6pl_1440_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t photo_pigment_dither_ranges[] =
{ /* MRS: Not calibrated! */
  { 0.15,  0x1, 0, 1 },
  { 0.227, 0x2, 0, 2 },
  { 0.5,   0x1, 1, 1 },
  { 1.0,   0x2, 1, 2 }
};

static escp2_variable_ink_t photo_pigment_ink =
{
  photo_pigment_dither_ranges,
  sizeof(photo_pigment_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t photo_4pl_dither_ranges[] =
{
  { 0.22,  0x1, 0, 1 },
  { 0.661, 0x1, 1, 1 },
  { 1.00,  0x2, 1, 2 }
};

static escp2_variable_ink_t photo_4pl_ink =
{
  photo_4pl_dither_ranges,
  sizeof(photo_4pl_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};

static simple_dither_range_t photo_4pl_1440_dither_ranges[] =
{
  { 0.30,  0x1, 0, 1 },
  { 0.90,  0x1, 1, 1 },
  { 1.36,  0x2, 1, 2 }
};

static escp2_variable_ink_t photo_4pl_1440_ink =
{
  photo_4pl_1440_dither_ranges,
  sizeof(photo_4pl_1440_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t standard_6pl_dither_ranges[] =
{
  { 0.45,  0x1, 1, 1 },
  { 0.68,  0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static escp2_variable_ink_t standard_6pl_ink =
{
  standard_6pl_dither_ranges,
  sizeof(standard_6pl_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t standard_6pl_1440_dither_ranges[] =
{
  { 0.90,  0x1, 1, 1 },
  { 1.36,  0x2, 1, 2 }
};

static escp2_variable_ink_t standard_6pl_1440_ink =
{
  standard_6pl_1440_dither_ranges,
  sizeof(standard_6pl_1440_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t standard_pigment_dither_ranges[] =
{ /* MRS: Not calibrated! */
  { 0.55,  0x1, 1, 1 },
  { 1.0,   0x2, 1, 2 }
};

static escp2_variable_ink_t standard_pigment_ink =
{
  standard_pigment_dither_ranges,
  sizeof(standard_pigment_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t standard_4pl_dither_ranges[] =
{
  { 0.661, 0x1, 1, 1 },
  { 1.00,  0x2, 1, 2 }
};

static escp2_variable_ink_t standard_4pl_ink =
{
  standard_4pl_dither_ranges,
  sizeof(standard_4pl_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};

static simple_dither_range_t standard_4pl_1440_dither_ranges[] =
{
  { 0.90,  0x1, 1, 1 },
  { 1.36,  0x2, 1, 2 },
};

static escp2_variable_ink_t standard_4pl_1440_ink =
{
  standard_4pl_1440_dither_ranges,
  sizeof(standard_4pl_1440_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t standard_3pl_dither_ranges[] =
{
  { 0.225, 0x1, 1, 1 },
  { 0.68,  0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static escp2_variable_ink_t standard_3pl_ink =
{
  standard_3pl_dither_ranges,
  sizeof(standard_3pl_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t standard_3pl_1440_dither_ranges[] =
{
  { 0.45, 0x1, 1, 1 },
  { 1.36,  0x2, 1, 2 },
};

static escp2_variable_ink_t standard_3pl_1440_ink =
{
  standard_3pl_1440_dither_ranges,
  sizeof(standard_3pl_1440_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t photo_multishot_dither_ranges[] =
{
  { 0.1097, 0x1, 0, 1 },
  { 0.227,  0x2, 0, 2 },
/*  { 0.333, 0x3, 0, 3 }, */
  { 0.28,   0x1, 1, 1 },
  { 0.58,   0x2, 1, 2 },
  { 0.85,   0x3, 1, 3 },
  { 1.0,    0x3, 1, 3 }
};

static escp2_variable_ink_t photo_multishot_ink =
{
  photo_multishot_dither_ranges,
  sizeof(photo_multishot_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t standard_multishot_dither_ranges[] =
{
  { 0.28,  0x1, 1, 1 },
  { 0.58,  0x2, 1, 2 },
  { 0.85,  0x3, 1, 3 },
  { 1.0,   0x3, 1, 3 }
};

static escp2_variable_ink_t standard_multishot_ink =
{
  standard_multishot_dither_ranges,
  sizeof(standard_multishot_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static escp2_variable_inkset_t standard_inks =
{
  NULL,
  NULL,
  NULL,
  NULL
};

static escp2_variable_inkset_t photo_inks =
{
  &photo_cyan_ink,
  &photo_magenta_ink,
  NULL,
  NULL
};

static escp2_variable_inkset_t escp2_6pl_standard_inks =
{
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink
};

static escp2_variable_inkset_t escp2_6pl_photo_inks =
{
  &photo_6pl_ink,
  &photo_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink
};

static escp2_variable_inkset_t escp2_6pl_1440_standard_inks =
{
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink
};

static escp2_variable_inkset_t escp2_6pl_1440_photo_inks =
{
  &photo_6pl_1440_ink,
  &photo_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink
};

static escp2_variable_inkset_t escp2_pigment_standard_inks =
{
  &standard_pigment_ink,
  &standard_pigment_ink,
  &standard_pigment_ink,
  &standard_pigment_ink
};

static escp2_variable_inkset_t escp2_pigment_photo_inks =
{
  &photo_pigment_ink,
  &photo_pigment_ink,
  &standard_pigment_ink,
  &standard_pigment_ink
};

static escp2_variable_inkset_t escp2_4pl_standard_inks =
{
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink
};

static escp2_variable_inkset_t escp2_4pl_photo_inks =
{
  &photo_4pl_ink,
  &photo_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink
};

static escp2_variable_inkset_t escp2_4pl_1440_standard_inks =
{
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink
};

static escp2_variable_inkset_t escp2_4pl_1440_photo_inks =
{
  &photo_4pl_1440_ink,
  &photo_4pl_1440_ink,
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink
};

static escp2_variable_inkset_t escp2_3pl_standard_inks =
{
  &standard_3pl_ink,
  &standard_3pl_ink,
  &standard_3pl_ink,
  &standard_3pl_ink
};

static escp2_variable_inkset_t escp2_3pl_1440_standard_inks =
{
  &standard_3pl_1440_ink,
  &standard_3pl_1440_ink,
  &standard_3pl_1440_ink,
  &standard_3pl_1440_ink
};

static escp2_variable_inkset_t escp2_multishot_standard_inks =
{
  &standard_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink
};

static escp2_variable_inkset_t escp2_multishot_photo_inks =
{
  &photo_multishot_ink,
  &photo_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink
};


static escp2_variable_inklist_t simple_4color_inks =
{
  {
    {
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

static escp2_variable_inklist_t simple_6color_inks =
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
    },
    {
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

static escp2_variable_inklist_t variable_6pl_4color_inks =
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
    }
  },
  {
    {
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

static escp2_variable_inklist_t variable_6pl_6color_inks =
{
  {
    {
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
      &photo_inks
    }
  },
  {
    {
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
      &escp2_6pl_1440_photo_inks,
      &escp2_6pl_photo_inks,
      &escp2_6pl_photo_inks
    }
  }
};

static escp2_variable_inklist_t variable_pigment_6color_inks =
{
  {
    {
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
      &escp2_pigment_standard_inks
    },
    {
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

static escp2_variable_inklist_t variable_3pl_4color_inks =
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
    }
  },
  {
    {
      &escp2_multishot_standard_inks,
      &escp2_multishot_standard_inks,
      &escp2_6pl_standard_inks,
      &escp2_3pl_standard_inks,
      &escp2_3pl_1440_standard_inks,
      &escp2_3pl_standard_inks,
      &escp2_3pl_standard_inks,
    }
  }
};

static escp2_variable_inklist_t variable_4pl_4color_inks =
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
    }
  },
  {
    {
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

static escp2_variable_inklist_t variable_4pl_6color_inks =
{
  {
    {
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
      &photo_inks
    }
  },
  {
    {
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
      &escp2_6pl_photo_inks,
      &escp2_4pl_photo_inks,
      &escp2_4pl_1440_photo_inks,
      &escp2_4pl_photo_inks,
      &escp2_4pl_photo_inks
    }
  }
};

static double standard_sat_adjustment[49] =
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

static double standard_lum_adjustment[49] =
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

static double x70_lum_adjustment[49] =
{
  0.50,				/* C */
  0.55,
  0.65,
  0.85,
  0.95,
  0.85,
  0.65,
  0.6,
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

static double standard_hue_adjustment[49] =
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
  2.18,
  2.29,
  2.38,
  2.47,
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
  int		max_hres;
  int		max_vres;
  escp2_dot_size_t dot_sizes;	/* Vector of dot sizes for resolutions */
  escp2_densities_t densities;	/* List of densities for each printer */
  escp2_variable_inklist_t *inks; /* Choices of inks for this printer */
  double *lum_adjustment;
  double *hue_adjustment;
  double *sat_adjustment;
} escp2_printer_t;

#define MODEL_INIT_MASK		0xfull /* Is a special init sequence */
#define MODEL_INIT_STANDARD	0x0ull /* required for this printer, and if */
#define MODEL_INIT_900		0x1ull /* so, what */

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

#define INCH(x)		(72 * x)

static escp2_printer_t model_capabilities[] =
{
  /* FIRST GENERATION PRINTERS */
  /* 0: Stylus Color */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES 
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    1, 1, 1, 1, 720, 720, INCH(17 / 2), INCH(14), 14, 14, 9, 49, 1, 0,
    720, 720,
    { -2, -2, -1, -2, -1, -1, -1, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .568, 0, 0, 0, 0, 0, 0, 0, 0 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 1: Stylus Color Pro/Pro XL/400/500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO 
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 48, 3, 720, 720, INCH(17 / 2), INCH(14), 14, 14, 0, 30, 1, 0,
    720, 720,
    { -2, -2, -1, -2, -1, -1, -1, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .631, 0, 0, 0, 0, 0, 0, 0, 0 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 2: Stylus Color 1500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_NO | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO 
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    1, 1, 1, 1, 720, 720, INCH(11), INCH(17), 14, 14, 9, 49, 1, 0,
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
    32, 4, 32, 4, 720, 360, INCH(17 / 2), INCH(14), 8, 9, 0, 30, 1, 0,
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
    64, 2, 64, 2, 720, 360, INCH(17 / 2), INCH(14), 8, 9, 9, 40, 1, 4,
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
    64, 2, 128, 1, 720, 360, INCH(17 / 2), INCH(14), 9, 9, 9, 40, 1, 4,
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
    64, 2, 64, 2, 720, 360, INCH(17), INCH(55), 8, 9, 9, 40, 1, 4,
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
    32, 4, 32, 4, 720, 360, INCH(17 / 2), INCH(14), 9, 9, 0, 30, 1, 0,
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
    32, 4, 32, 4, 720, 360, INCH(11), INCH(17), 9, 9, 0, 30, 1, 0,
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
    32, 4, 32, 4, 720, 360, INCH(17 / 2), INCH(14), 9, 9, 0, 30, 1, 0,
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
    21, 4, 21, 4, 720, 720, INCH(17 / 2), INCH(14), 9, 9, 0, 9, 1, 0,
    720, 720,
    { -1, 3, -1, 1, 1, -1, -1, -1, -1, -1, -1 },
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
    32, 4, 64, 2, 720, 720, INCH(17 / 2), INCH(14), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 3, -1, 1, 1, -1, 1, -1, 1, -1, -1 },
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
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(14), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 4, 0x10, 3, 0x10, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 2.0, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 13: Stylus Color 900 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    96, 2, 192, 1, 360, 180, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 1, 0x11, 1, 0x10, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_3pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 14: Stylus Photo 750 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 48, 3, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 2, 0x10, 4, 0x10, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 15: Stylus Photo 1200 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    48, 3, 48, 3, 360, 360, INCH(13), INCH(44), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 2, 0x10, 4, 0x10, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 16: Stylus Color 860 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(14), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 17: Stylus Color 1160 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(13), INCH(44), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 18: Stylus Color 660 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 64, 2, 720, 720, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 8,
    1440, 720,
    { -1, 3, -1, 3, 0, -1, 0, -1, -1, -1, -1 },
    { 3.0, 2.0, 2.0, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 19: Stylus Color 760 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(14), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 20: Stylus Photo 720 (Australia) */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 32, 4, 360, 360, INCH(17 / 2), INCH(14), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 2, 0x12, 4, 0x11, -1, 0x11, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 21: Stylus Color 480 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO 
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    15, 3, 15, 3, 360, 360, INCH(17 / 2), INCH(14), 9, 9, 0, 9, 1, 0,
    720, 720,
    { -1, -2, 0x13, -2, 0x10, -1, -1, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_6pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 22: Stylus Photo 870 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_YES),
    48, 3, 48, 3, 360, 360, INCH(17 / 2), INCH(44), 0, 0, 0, 9, 1, 0,
    1440, 720,
    { -1, 4, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_6color_inks, x70_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 23: Stylus Photo 1270 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_YES),
    48, 3, 48, 3, 360, 360, INCH(13), INCH(44), 0, 0, 0, 9, 1, 0,
    1440, 720,
    { -1, 4, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, -1 },
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
    64, 2, 64, 2, 720, 360, INCH(17), INCH(55), 8, 9, 9, 40, 1, 4,
    1440, 720,
    { 3, 3, -1, 1, 1, -1, 4, -1, 4, -1, -1 },
    { 2.0, 1.3, 1.3, .775, .775, .55, .55, .275, .275, .275, .275, .138 },
    &simple_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 25: Stylus Color 670 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    32, 4, 64, 2, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 3, 0x12, 3, 0x11, -1, 0x11, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_6pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 26: Stylus Photo 2000P */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 2, 0x11, 4, 0x10, -1, 0x10, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .775, .852, .388, .438, .388, .438, .219, .219, .110 },
    &variable_pigment_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 27: Stylus Pro 5000 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 360, 360, INCH(13), INCH(1200), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 2, -1, 4, 0, 4, 0, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 28: Stylus Pro 7000 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 360, 360, INCH(24), INCH(1200), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 2, -1, 4, 0, 4, 0, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 29: Stylus Pro 7500 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 360, 360, INCH(24), INCH(1200), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 2, -1, 4, 0, 4, 0, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 30: Stylus Pro 9000 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 360, 360, INCH(44), INCH(1200), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 2, -1, 4, 0, 4, 0, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 31: Stylus Pro 9500 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_SELECTABLE
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_NO),
    64, 2, 64, 2, 360, 360, INCH(44), INCH(1200), 9, 9, 0, 9, 1, 0,
    1440, 720,
    { -1, 2, -1, 4, 0, 4, 0, -1, -1, -1, -1 },
    { 2.0, 1.3, 1.3, .646, .646, .323, .323, .1615, .1615, .1615, .1615, .0808 },
    &simple_6color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 32: Stylus Color 777/680 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(14), 9, 9, 0, 9, 1, 0,
    2880, 720,
    { -1, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 33: Stylus Color 880 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    48, 3, 144, 1, 360, 360, INCH(17 / 2), INCH(14), 9, 9, 0, 9, 1, 0,
    2880, 720,
    { -1, 0, 0x12, 0, 0x11, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 34: Stylus Color 980 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_4 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_NO),
    96, 2, 192, 1, 360, 360, INCH(17 / 2), INCH(44), 9, 9, 0, 9, 1, 0,
    2880, 720,
    { -1, 1, 0x11, 1, 0x10, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .646, .710, .323, .365, .323, .365, .1825, .1825, .0913 },
    &variable_3pl_4color_inks, standard_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 35: Stylus Photo 790 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_NO | MODEL_ZEROMARGIN_YES),
    48, 3, 48, 3, 360, 360, INCH(17 / 2), INCH(44), 0, 0, 0, 9, 1, 0,
    2880, 720,
    { -1, 4, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_6color_inks, x70_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 36: Stylus Photo 890 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_YES),
    48, 3, 48, 3, 360, 360, INCH(17 / 2), INCH(44), 0, 0, 0, 9, 1, 0,
    2880, 720,
    { -1, 4, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_6color_inks, x70_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
  /* 37: Stylus Photo 1290 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES | MODEL_INK_NORMAL
     | MODEL_COLOR_6 | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_MULTI
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO
     | MODEL_ROLLFEED_YES | MODEL_ZEROMARGIN_YES),
    48, 3, 48, 3, 360, 360, INCH(13), INCH(44), 0, 0, 0, 9, 1, 0,
    2880, 720,
    { -1, 4, 0x12, 2, 0x11, -1, 0x10, -1, -1, -1, 0x10 },
    { 2.0, 1.3, 1.3, .431, .710, .216, .784, .216, .784, .392, .392, .196 },
    &variable_4pl_6color_inks, x70_lum_adjustment, standard_hue_adjustment,
    standard_sat_adjustment
  },
};

typedef struct escp_init
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
} escp_init_t;

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
  { "180 DPI",                                  180,  180,  0, 0, 1, 1, 0, 0 },
  { "180 DPI Unidirectional",                   180,  180,  0, 0, 1, 1, 1, 0 },
  { "360 DPI",                                  360,  360,  0, 0, 1, 1, 0, 2 },
  { "360 DPI Unidirectional",                   360,  360,  0, 0, 1, 1, 1, 2 },
  { "360 DPI Microweave",                       360,  360,  0, 1, 1, 1, 0, 2 },
  { "360 DPI Microweave Unidirectional",        360,  360,  0, 1, 1, 1, 1, 2 },
  { "360 DPI Softweave",                        360,  360,  1, 0, 1, 1, 0, 3 },
  { "360 DPI High Quality",                     360,  360,  1, 0, 2, 1, 0, 3 },
  { "360 DPI High Quality Unidirectional",      360,  360,  1, 0, 2, 1, 1, 3 },
  { "720 DPI Microweave",                       720,  720,  0, 1, 1, 1, 0, 4 },
  { "720 DPI Microweave Unidirectional",        720,  720,  0, 1, 1, 1, 1, 4 },
  { "720 DPI Softweave",                        720,  720,  1, 0, 1, 1, 0, 5 },
  { "720 DPI Softweave Unidirectional",         720,  720,  1, 0, 1, 1, 1, 5 },
  { "720 DPI High Quality",                     720,  720,  1, 0, 2, 1, 0, 5 },
  { "720 DPI High Quality Unidirectional",      720,  720,  1, 0, 2, 1, 1, 5 },
  { "720 DPI Highest Quality",                  720,  720,  1, 0, 4, 1, 1, 5 },
  { "1440 x 720 DPI Microweave",                1440, 720,  0, 1, 1, 1, 0, 6 },
  { "1440 x 720 DPI Microweave Unidirectional", 1440, 720,  0, 1, 1, 1, 1, 6 },
  { "1440 x 720 DPI Softweave",                 1440, 720,  1, 0, 1, 1, 0, 7 },
  { "1440 x 720 DPI Softweave Unidirectional",  1440, 720,  1, 0, 1, 1, 1, 7 },
  { "1440 x 720 DPI Highest Quality",           1440, 720,  1, 0, 2, 1, 1, 7 },
  { "1440 x 1440 DPI Softweave",                1440, 1440, 1, 0, 1, 1, 1, 9 },
  { "1440 x 1440 DPI Highest Quality",          1440, 1440, 1, 0, 2, 1, 1, 9 },
  { "2880 x 720 DPI Softweave",                 2880, 720,  1, 0, 1, 1, 0, 11},
  { "2880 x 720 DPI Softweave Unidirectional",  2880, 720,  1, 0, 1, 1, 1, 11},
  { "2880 x 1440 DPI Softweave",                2880, 1440, 1, 0, 1, 1, 1, 13},
  { "", 0, 0, 0, 0, 0, -1 }
};

typedef struct {
  const char name[65];
  int is_color;
  int variable_dot_size;
  int dot_size_bits;
  simple_dither_range_t *standard_dither;
  simple_dither_range_t *photo_dither;
} ink_t;

typedef struct {
  const char name[65];
  int paper_feed_sequence;
  int platen_gap;
  double base_density;
  double k_lower_scale;
  double k_upper;
} paper_t;

static const paper_t escp2_paper_list[] = {
  { "Plain Paper", 1, 0, .5, .25, .5 },
  { "Plain Paper Fast Load", 5, 0, .5, .25, .5 },
  { "Postcard", 2, 0, .6, .25, .6 },
  { "Glossy Film", 3, 0, 1.0, 1.0, .999 },
  { "Transparencies", 3, 0, 1.0, 1.0, .999 },
  { "Envelopes", 4, 0, .5, .25, .5 },
  { "Back Light Film", 6, 0, 1.0, 1.0, .999 },
  { "Matte Paper", 7, 0, 1.0, 1.0, .9 },
  { "Inkjet Paper", 7, 0, .78, .25, .6 },
  { "Photo Quality Inkjet Paper", 7, 0, 1, 1.0, .999 },
  { "Photo Paper", 8, 0, 1, 1.0, .9 },
  { "Premium Glossy Photo Paper", 8, 0, .9, 1.0, .999 },
  { "Premium Luster Photo Paper", 8, 0, 1.0, 1.0, .999 },
  { "Photo Quality Glossy Paper", 6, 0, 1.0, 1.0, .999 },
  { "Other", 0, 0, .5, .25, .5 },
};

static const int paper_type_count = sizeof(escp2_paper_list) / sizeof(paper_t);


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
escp2_has_cap(int model, model_featureset_t featureset,
	      model_featureset_t class)
{
  return ((model_capabilities[model].flags & featureset) == class);
}

static int
escp2_max_hres(int model)
{
  return (model_capabilities[model].max_hres);
}

static int
escp2_max_vres(int model)
{
  return (model_capabilities[model].max_vres);
}

static unsigned
escp2_nozzles(int model)
{
  return (model_capabilities[model].nozzles);
}

static unsigned
escp2_black_nozzles(int model)
{
  return (model_capabilities[model].black_nozzles);
}

static unsigned
escp2_nozzle_separation(int model)
{
  return (model_capabilities[model].nozzle_separation);
}

static unsigned
escp2_black_nozzle_separation(int model)
{
  return (model_capabilities[model].black_nozzle_separation);
}

static unsigned
escp2_separation_rows(int model)
{
  return (model_capabilities[model].separation_rows);
}

static unsigned
escp2_xres(int model)
{
  return (model_capabilities[model].xres);
}

static unsigned
escp2_enhanced_xres(int model)
{
  return (model_capabilities[model].enhanced_xres);
}

static int
escp2_ink_type(int model, int resid)
{
  int dotid = resid2dotid(resid);
  return model_capabilities[model].dot_sizes[dotid];
}

static double
escp2_density(int model, int resid)
{
  int densid = resid2densid(resid);
  return model_capabilities[model].densities[densid];
}

static escp2_variable_inkset_t *
escp2_inks(int model, int resid, int colors, int bits)
{
  escp2_variable_inklist_t *inks = model_capabilities[model].inks;
  int inktype = bits2inktype(bits);
  int inkset = colors2inkset(colors);
  resid /= 2;
  return (*inks)[inktype][inkset][resid];
}

static unsigned
escp2_max_paper_width(int model)
{
  return (model_capabilities[model].max_paper_width);
}

static unsigned
escp2_max_paper_height(int model)
{
  return (model_capabilities[model].max_paper_height);
}

static unsigned
escp2_left_margin(int model)
{
  return (model_capabilities[model].left_margin);
}

static unsigned
escp2_right_margin(int model)
{
  return (model_capabilities[model].right_margin);
}

static unsigned
escp2_top_margin(int model)
{
  return (model_capabilities[model].top_margin);
}

static unsigned
escp2_bottom_margin(int model)
{
  return (model_capabilities[model].bottom_margin);
}

static int
escp2_pseudo_separation_rows(int model)
{
  return (model_capabilities[model].pseudo_separation_rows);
}

static double *
escp2_lum_adjustment(int model)
{
  return (model_capabilities[model].lum_adjustment);
}

static double *
escp2_hue_adjustment(int model)
{
  return (model_capabilities[model].hue_adjustment);
}

static double *
escp2_sat_adjustment(int model)
{
  return (model_capabilities[model].sat_adjustment);
}

/*
 * 'escp2_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
escp2_parameters(const printer_t *printer,	/* I - Printer model */
                 char *ppd_file,	/* I - PPD file (not used) */
                 char *name,		/* I - Name of parameter */
                 int  *count)		/* O - Number of values */
{
  int		i;
  int		model = printer->model;
  char		**valptrs;

  static const char *ink_types[] =
  {
    "Six Color Photo",
    "Four Color Standard"
  };

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0)
    {
      unsigned int height_limit, width_limit;
      const papersize_t *papersizes = get_papersizes();
      valptrs = malloc(sizeof(char *) * known_papersizes());
      *count = 0;
      width_limit = escp2_max_paper_width(model);
      height_limit = escp2_max_paper_height(model);
      for (i = 0; i < known_papersizes(); i++)
	{
	  if (strlen(papersizes[i].name) > 0 &&
	      papersizes[i].width <= width_limit &&
	      papersizes[i].height <= height_limit)
	    {
	      valptrs[*count] = malloc(strlen(papersizes[i].name) + 1);
	      strcpy(valptrs[*count], papersizes[i].name);
	      (*count)++;
	    }
	}
      return (valptrs);
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      const res_t *res = &(escp2_reslist[0]);
      valptrs = malloc(sizeof(char *) * sizeof(escp2_reslist) / sizeof(res_t));
      *count = 0;
      while(res->hres)
	{
	  if (escp2_ink_type(model, res->resid) != -1 &&
	      res->vres <= escp2_max_vres(model) &&
	      res->hres <= escp2_max_hres(model))
	    {
	      int nozzles = escp2_nozzles(model);
	      int xdpi = res->hres;
	      int physical_xdpi =
		xdpi > escp2_enhanced_resolution ? escp2_enhanced_xres(model) :
		escp2_xres(model);
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
		  valptrs[*count] = malloc(strlen(res->name) + 1);
		  strcpy(valptrs[*count], res->name);
		  (*count)++;
		}
	    }
	  res++;
	}
      return (valptrs);
    }
  else if (strcmp(name, "InkType") == 0)
    {
      if (escp2_has_cap(model, MODEL_COLOR_MASK, MODEL_COLOR_4))
	return NULL;
      else
	{
	  int ninktypes = sizeof(ink_types) / sizeof(char *);
	  valptrs = malloc(sizeof(char *) * ninktypes);
	  for (i = 0; i < ninktypes; i++)
	    {
	      valptrs[i] = malloc(strlen(ink_types[i]) + 1);
	      strcpy(valptrs[i], ink_types[i]);
	    }
	  *count = ninktypes;
	  return valptrs;
	}
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      int nmediatypes = paper_type_count;
      valptrs = malloc(sizeof(char *) * nmediatypes);
      for (i = 0; i < nmediatypes; i++)
	{
	  valptrs[i] = malloc(strlen(escp2_paper_list[i].name) + 1);
	  strcpy(valptrs[i], escp2_paper_list[i].name);
	}
      *count = nmediatypes;
      return valptrs;
    }
  else if (strcmp(name, "InputSlot") == 0)
    {
      if (escp2_has_cap(model, MODEL_ROLLFEED_MASK, MODEL_ROLLFEED_NO))
	return NULL;
      else
	{      /* Roll Feed capable printers */
		valptrs = malloc(sizeof(char *) * 2);
		valptrs[0] = strdup("Standard");
		valptrs[1] = strdup("Roll Feed");
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

void
escp2_imageable_area(const printer_t *printer,	/* I - Printer model */
		     const vars_t *v,   /* I */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  int	width, height;			/* Size of page */

  default_media_size(printer, v, &width, &height);
  *left =	escp2_left_margin(printer->model);
  *right =	width - escp2_right_margin(printer->model);
  *top =	height - escp2_top_margin(printer->model);
  *bottom =	escp2_bottom_margin(printer->model);
}

void
escp2_limit(const printer_t *printer,	/* I - Printer model */
	    const vars_t *v,  		/* I */
	    int  *width,		/* O - Left position in points */
	    int  *height)		/* O - Top position in points */
{
  *width =	escp2_max_paper_width(printer->model);
  *height =	escp2_max_paper_height(printer->model);
}

const char *
escp2_default_resolution(const printer_t *printer)
{
  const res_t *res = &(escp2_reslist[0]);
  while (res->hres)
    {
      if (escp2_ink_type(printer->model, res->resid) != -1 &&
	  res->vres <= escp2_max_vres(printer->model) &&
	  res->hres <= escp2_max_hres(printer->model))
	{
	  int nozzles = escp2_nozzles(printer->model);
	  int xdpi = res->hres;
	  int physical_xdpi = xdpi > escp2_enhanced_resolution ?
	    escp2_enhanced_xres(printer->model) : escp2_xres(printer->model);
	  int horizontal_passes = xdpi / physical_xdpi;
	  int oversample = horizontal_passes * res->vertical_passes
	    * res->vertical_oversample;
	  if (horizontal_passes < 1)
	    horizontal_passes = 1;
	  if (oversample < 1)
	    oversample = 1;
	  if (((horizontal_passes * res->vertical_passes) <= 8) &&
	      (! res->softweave || (nozzles > 1 && nozzles > oversample)))
	    return res->name;
	}
      res++;
    }
  return NULL;
}

void
escp2_describe_resolution(const printer_t *printer,
			  const char *resolution, int *x, int *y)
{
  const res_t *res = &(escp2_reslist[0]);
  while (res->hres)
    {
      if (escp2_ink_type(printer->model, res->resid) != -1 &&
	  res->vres <= escp2_max_vres(printer->model) &&
	  res->hres <= escp2_max_hres(printer->model) &&
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
escp2_reset_printer(FILE *prn, escp_init_t *init)
{
  /*
   * Hack that seems to be necessary for these silly things to recognize
   * the input.  It only needs to be done once per printer evidently, but
   * it needs to be done.
   */
  if (escp2_has_cap(init->model, MODEL_INIT_MASK, MODEL_INIT_900))
    fprintf(prn, "%c%c%c\033\001@EJL 1284.4\n@EJL     \n\033@", 0, 0, 0);

  fputs("\033@", prn); 				/* ESC/P2 reset */
}

static void
escp2_set_remote_sequence(FILE *prn, escp_init_t *init)
{
  /* Magic remote mode commands, whatever they do */
  if (escp2_has_cap(init->model, MODEL_COMMAND_MASK, MODEL_COMMAND_1999))
    {
      int feed_sequence = 0;
      const paper_t *p = get_media_type(init->paper_type);
      if (p)
	feed_sequence = p->paper_feed_sequence;
      fprintf(prn, /* Enter remote mode */
                   "\033(R\010%c%cREMOTE1"
                   /* Function unknown */
                   "PM\002%c%c%c"
                   /* Set mechanism sequence */
	           "SN\003%c%c%c%c",
	      0, 0,
	      0, 0, 0,
	      0, 0, 0, feed_sequence);
      if (escp2_has_cap(init->model, MODEL_ZEROMARGIN_MASK,
                                     MODEL_ZEROMARGIN_YES))
	fprintf(prn, /* Set zero-margin print mode */
	             "FP\003%c%c\260\377", 0, 0);

	/* set up Roll-Feed options on appropriate printers 
		(tested for STP 870, which has no cutter) */
      if (escp2_has_cap(init->model, MODEL_ROLLFEED_MASK,
				     MODEL_ROLLFEED_YES))
	{
		if(strcmp(init->media_source,"Roll Feed") == 0)
			fprintf(prn, /* Set Roll Feed mode */
			     "IR\002%c%c%c"
			     "EX\006%c%c%c%c%c%c%c", 
				0, 0, 1,
				0, 0,0,0,0,5,1);
		else
			fprintf(prn, /* Set non-Roll Feed mode */
			     "IR\002%c%c%c"
			     "EX\006%c%c%c%c%c%c%c", 
				0, 0, 3,
				0, 0, 0, 0, 0, 5, 0);
	}

      fprintf(prn, /* Exit remote mode */
                   "\033%c%c%c", 0, 0, 0);
    }
}

static void
escp2_set_graphics_mode(FILE *prn, escp_init_t *init)
{
  fwrite("\033(G\001\000\001", 6, 1, prn);	/* Enter graphics mode */
}

static void
escp2_set_resolution(FILE *prn, escp_init_t *init)
{
  if (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT_MASK,
		     MODEL_VARIABLE_NORMAL)) &&
      init->use_softweave)
    {
      int hres = escp2_max_hres(init->model);
      fprintf(prn, "\033(U\005%c%c%c%c%c%c", 0, hres / init->ydpi,
	      hres / init->ydpi, hres / init->xdpi, hres % 256, hres / 256);
    }
  else
    fprintf(prn, "\033(U\001%c%c", 0, 3600 / init->ydpi);
}

static void
escp2_set_color(FILE *prn, escp_init_t *init)
{
  if (escp2_has_cap(init->model, MODEL_GRAYMODE_MASK, MODEL_GRAYMODE_YES))
    fprintf(prn, "\033(K\002%c%c%c", 0, 0,
	    (init->output_type == OUTPUT_GRAY ? 1 : 2));
}

static void
escp2_set_microweave(FILE *prn, escp_init_t *init)
{
  fprintf(prn, "\033(i\001%c%c", 0, init->use_microweave);
}

static void
escp2_set_printhead_speed(FILE *prn, escp_init_t *init)
{
  if (init->unidirectional)
    {
      fprintf(prn, "\033U%c", 1);
      if (init->xdpi > 720)		/* Slow mode if available */
	fprintf(prn, "\033(s%c%c%c", 1, 0, 2);
    }
  else
    fprintf(prn, "\033U%c", 0);
}

static void
escp2_set_dot_size(FILE *prn, escp_init_t *init)
{
  /* Dot size */
  int drop_size = escp2_ink_type(init->model, init->resid);
  if (drop_size >= 0)
    fprintf(prn, "\033(e\002%c%c%c", 0, 0, drop_size);
}

static void
escp2_set_page_height(FILE *prn, escp_init_t *init)
{
  int l = init->ydpi * init->page_height / 72;
  if (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT_MASK,
		      MODEL_VARIABLE_NORMAL)) &&
      init->use_softweave)
    fprintf(prn, "\033(C\004%c%c%c%c%c", 0,
	    l & 0xff, (l >> 8) & 0xff, (l >> 16) & 0xff, (l >> 24) & 0xff);
  else
    fprintf(prn, "\033(C\002%c%c%c", 0, l & 255, l >> 8);
}

static void
escp2_set_margins(FILE *prn, escp_init_t *init)
{
  int l = init->ydpi * (init->page_height - init->page_bottom) / 72;
  int t = init->ydpi * (init->page_height - init->page_top) / 72;
  if (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT_MASK,
		      MODEL_VARIABLE_NORMAL)) &&
      init->use_softweave)
    {
      if (escp2_has_cap(init->model, MODEL_COLOR_MASK, MODEL_COLOR_6))
	fprintf(prn, "\033(c\010%c%c%c%c%c%c%c%c%c", 0,
		t & 0xff, t >> 8, (t >> 16) & 0xff, (t >> 24) & 0xff,
		l & 0xff, l >> 8, (l >> 16) & 0xff, (l >> 24) & 0xff);
      else
	fprintf(prn, "\033(c\004%c%c%c%c%c", 0,
		t & 0xff, t >> 8, l & 0xff, l >> 8);
    }
  else
    fprintf(prn, "\033(c\004%c%c%c%c%c", 0,
	    t & 0xff, t >> 8, l & 0xff, l >> 8);
}

static void
escp2_set_form_factor(FILE *prn, escp_init_t *init)
{
  int page_width = init->page_width * init->ydpi / 72;
  int page_height = init->page_height * init->ydpi / 72;

  if (escp2_has_cap(init->model, MODEL_ZEROMARGIN_MASK, MODEL_ZEROMARGIN_YES))
      /* Make the page 2/10" wider (probably ignored by the printer anyway) */
      page_width += 144 * 720 / init->xdpi;

  if (escp2_has_cap(init->model, MODEL_COMMAND_MASK, MODEL_COMMAND_1999))
    fprintf(prn, "\033(S\010%c%c%c%c%c%c%c%c%c", 0,
	    ((page_width >> 0) & 0xff), ((page_width >> 8) & 0xff),
	    ((page_width >> 16) & 0xff), ((page_width >> 24) & 0xff),
	    ((page_height >> 0) & 0xff), ((page_height >> 8) & 0xff),
	    ((page_height >> 16) & 0xff), ((page_height >> 24) & 0xff));
}

static void
escp2_set_printhead_resolution(FILE *prn, escp_init_t *init)
{
  if (!(escp2_has_cap(init->model, MODEL_VARIABLE_DOT_MASK,
		      MODEL_VARIABLE_NORMAL)) &&
      init->use_softweave)
    {
      int xres;
      int nozzle_separation;
      if (init->xdpi > escp2_enhanced_resolution)
	xres = escp2_enhanced_xres(init->model);
      else
	xres = escp2_xres(init->model);
      if (init->output_type == OUTPUT_GRAY)
	nozzle_separation = escp2_black_nozzle_separation(init->model);
      else
	nozzle_separation = escp2_nozzle_separation(init->model);
      /* Magic resolution cookie */
      fprintf(prn, "\033(D%c%c%c%c%c%c", 4, 0, escp2_resolution_scale % 256,
	      escp2_resolution_scale / 256,
	      nozzle_separation * escp2_resolution_scale / escp2_base_separation,
	      escp2_resolution_scale / xres);
    }
}

static void
escp2_init_printer(FILE *prn, escp_init_t *init)
{
  if (init->ydpi > escp2_max_vres(init->model))
    init->ydpi = escp2_max_vres(init->model);

  escp2_reset_printer(prn, init);
  escp2_set_remote_sequence(prn, init);
  escp2_set_graphics_mode(prn, init);
  escp2_set_resolution(prn, init);
  escp2_set_color(prn, init);
  escp2_set_microweave(prn, init);
  escp2_set_printhead_speed(prn, init);
  escp2_set_dot_size(prn, init);
  escp2_set_page_height(prn, init);
  escp2_set_margins(prn, init);
  escp2_set_form_factor(prn, init);
  escp2_set_printhead_resolution(prn, init);
}

static void
escp2_deinit_printer(FILE *prn, escp_init_t *init)
{
  fputs(/* Eject page */
        "\014"
        /* ESC/P2 reset */
        "\033@", prn);
  if (escp2_has_cap(init->model, MODEL_COMMAND_MASK, MODEL_COMMAND_1999)) {
    fprintf(prn, /* Enter remote mode */
                 "\033(R\010%c%cREMOTE1", 0, 0);
	/* set up Roll-Feed options on appropriate printers 
		(tested for STP 870, which has no cutter) */
      if (escp2_has_cap(init->model, MODEL_ROLLFEED_MASK,
				     MODEL_ROLLFEED_YES))
	{
		if(strcmp(init->media_source,"Roll Feed") == 0)
			fprintf(prn, /* End Roll Feed mode */
			     "IR\002%c%c%c",
				0, 0, 0);
		else
			fprintf(prn, /* End non-Roll Feed mode */
			     "IR\002%c%c%c",
				0, 0, 2);
	}
    fprintf(prn, /* Load settings from NVRAM */
                 "LD%c%c"
                 /* Exit remote mode */
                 "\033%c%c%c",  0, 0, 0, 0, 0);

  }
}

/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
void
escp2_print(const printer_t *printer,		/* I - Model */
            FILE      *prn,		/* I - File to print to */
	    Image     image,		/* I - Image to print */
	    const vars_t    *v)
{
  unsigned char *cmap = v->cmap;
  int		model = printer->model;
  const char	*resolution = v->resolution;
  const char	*media_type = v->media_type;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  const char	*ink_type = v->ink_type;
  double	scaling = v->scaling;
  const char	*media_source = v->media_source;
  int		top = v->top;
  int		left = v->left;
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
  convert_t	colorfunc = 0;	/* Color conversion function... */
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
  int		separation_rows = escp2_separation_rows(model);
  int		ink_spread;
  vars_t	nv;
  escp_init_t	init;
  escp2_variable_inkset_t *inks;
  const paper_t *pt;
  double k_upper, k_lower;
  int max_vres = escp2_max_vres(model);

  memcpy(&nv, v, sizeof(vars_t));

  if (escp2_has_cap(model, MODEL_COLOR_MASK, MODEL_COLOR_6) &&
      strcmp(ink_type, "Four Color Standard") != 0 &&
      nv.image_type != IMAGE_MONOCHROME)
    use_6color = 1;

  if (escp2_has_cap(model, MODEL_COLOR_MASK, MODEL_COLOR_7) &&
      nv.image_type != IMAGE_MONOCHROME)
    {
      if (strcmp(ink_type, "Six Color Photo") == 0)
	use_6color = 1;
      else if (strcmp(ink_type, "Seven Color Enhanced") == 0)
	use_7color = 1;
    }

  if (nv.image_type == IMAGE_MONOCHROME)
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

  Image_init(image);
  image_height = Image_height(image);
  image_width = Image_width(image);
  image_bpp = Image_bpp(image);

 /*
  * Choose the correct color conversion function...
  */

  colorfunc = choose_colorfunc(output_type, image_bpp, cmap, &out_bpp, &nv);

 /*
  * Compute the output size...
  */
  escp2_imageable_area(printer, &nv, &page_left, &page_right,
                       &page_bottom, &page_top);

  compute_page_parameters(page_right, page_left, page_top, page_bottom,
			  scaling, image_width, image_height, image,
			  &orientation, &page_width, &page_height,
			  &out_width, &out_height, &left, &top);

  /*
   * Recompute the image height and width.  If the image has been
   * rotated, these will change from previously.
   */
  image_height = Image_height(image);
  image_width = Image_width(image);

 /*
  * Figure out the output resolution...
  */
  for (res = &escp2_reslist[0];;res++)
    {
      if (!strcmp(resolution, res->name))
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
	  if (xdpi > escp2_enhanced_resolution)
	    physical_xdpi = escp2_enhanced_xres(model);
	  else
	    physical_xdpi = escp2_xres(model);
	  if (use_softweave)
	    horizontal_passes = xdpi / physical_xdpi;
	  else
	    horizontal_passes = xdpi / escp2_base_resolution;
	  if (horizontal_passes == 0)
	    horizontal_passes = 1;
	  if (output_type == OUTPUT_GRAY)
	    {
	      nozzles = escp2_black_nozzles(model);
	      if (nozzles == 0)
		{
		  nozzle_separation = escp2_nozzle_separation(model);
		  nozzles = escp2_nozzles(model);
		}
	      else
		  nozzle_separation = escp2_black_nozzle_separation(model);
	    }
	  else
	    {
	      nozzle_separation = escp2_nozzle_separation(model);
	      nozzles = escp2_nozzles(model);
	    }
	  if (ydpi > escp2_base_separation)
	    nozzle_separation = nozzle_separation * ydpi /
	      escp2_base_separation;
	  break;
	}
      else if (!strcmp(resolution, ""))
	{
	  return;
	}
    }
  if (!(escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_NORMAL))
      && use_softweave)
    bits = 2;
  else
    bits = 1;

 /*
  * Let the user know what we're doing...
  */

  Image_progress_init(image);

 /*
  * Send ESC/P2 initialization commands...
  */
  default_media_size(printer, &nv, &n, &page_true_height);
  init.model = model;
  init.output_type = output_type;
  init.ydpi = ydpi;
  init.xdpi = xdpi;
  init.use_softweave = use_softweave;
  init.use_microweave = use_microweave;
  init.page_height = page_true_height;
  init.page_width = page_width;
  init.page_top = page_top;
  init.page_bottom = page_bottom;
  init.horizontal_passes = horizontal_passes;
  init.vertical_passes = vertical_passes;
  init.vertical_oversample = vertical_oversample;
  init.unidirectional = unidirectional;
  init.resid = resid;
  init.bits = bits;
  init.paper_type = media_type;
  init.media_source = media_source;

  escp2_init_printer(prn, &init);

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

  if (escp2_has_cap(model, MODEL_ZEROMARGIN_MASK, MODEL_ZEROMARGIN_YES))
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
    black   = malloc(length * bits);
    cyan    = NULL;
    magenta = NULL;
    lcyan    = NULL;
    lmagenta = NULL;
    yellow  = NULL;
    dyellow = NULL;
  }
  else
  {
    cyan    = malloc(length * bits);
    magenta = malloc(length * bits);
    yellow  = malloc(length * bits);

    if (escp2_has_cap(model, MODEL_HASBLACK_MASK, MODEL_HASBLACK_YES))
      black = malloc(length * bits);
    else
      black = NULL;
    switch (colormode)
      {
      case COLOR_CCMMYYK:
	lcyan = malloc(length * bits);
	lmagenta = malloc(length * bits);
	dyellow = malloc(length * bits);
	break;
      case COLOR_CCMMYK:
	lcyan = malloc(length * bits);
	lmagenta = malloc(length * bits);
	dyellow = NULL;
	break;
      default:
	lcyan = NULL;
	lmagenta = NULL;
	dyellow = NULL;
	break;
    }
  }

  if (use_softweave)
    /* Epson printers are currently all 720 physical dpi vertically */
    weave = initialize_weave(nozzles, nozzle_separation, horizontal_passes,
			     vertical_passes, vertical_oversample, colormode,
			     bits,
			     out_width * escp2_xres(model) / physical_ydpi,
			     out_height, separation_rows,
			     top * physical_ydpi / 72,
			     page_height * physical_ydpi / 72, use_softweave);
  else
    escp2_init_microweave(top * ydpi / 72);

  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */
  pt = get_media_type(nv.media_type);
  if (pt)
    nv.density *= pt->base_density;
  else
    nv.density *= .5;		/* Can't find paper type? Assume plain */
  nv.density *= escp2_density(model, resid);
  if (nv.density > 1.0)
    nv.density = 1.0;
  if (colormode == COLOR_MONOCHROME)
    nv.gamma /= .8;
  compute_lut(256, &nv);

 /*
  * Output the page...
  */

  if (xdpi > ydpi)
    dither = init_dither(image_width, out_width, 1, xdpi / ydpi, &nv);
  else
    dither = init_dither(image_width, out_width, ydpi / xdpi, 1, &nv);

  dither_set_black_levels(dither, 1.0, 1.0, 1.0);
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
  dither_set_black_lower(dither, k_lower);
  dither_set_black_upper(dither, k_upper);
  if (bits == 2)
    {
      if (use_6color || use_7color)
	dither_set_adaptive_divisor(dither, 8);
      else
	dither_set_adaptive_divisor(dither, 16);
    }  
  else
    dither_set_adaptive_divisor(dither, 4);

  inks = escp2_inks(model, resid, use_7color ? 7 : (use_6color ? 6 : 4), bits);
  if (inks)
    for (i = 0; i < NCOLORS; i++)
      if ((*inks)[i])
	dither_set_ranges(dither, i, (*inks)[i]->count, (*inks)[i]->range,
			  (*inks)[i]->density * nv.density);

  if (bits == 2)
    {
      if (use_6color || use_7color)
	dither_set_transition(dither, .7);
      else
	dither_set_transition(dither, .5);
    }
  if (!strcmp(nv.dither_algorithm, "Ordered"))
    dither_set_transition(dither, 1);

  switch (nv.image_type)
    {
    case IMAGE_LINE_ART:
      dither_set_ink_spread(dither, 19);
      break;
    case IMAGE_SOLID_TONE:
      dither_set_ink_spread(dither, 15);
      break;
    case IMAGE_CONTINUOUS:
      ink_spread = 13;
      if (ydpi > escp2_max_vres(model))
	ink_spread++;
      if (bits > 1)
	ink_spread++;
      dither_set_ink_spread(dither, ink_spread);
      break;
    }
  dither_set_density(dither, nv.density);

  in  = malloc(image_width * image_bpp);
  out = malloc(image_width * out_bpp * 2);

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;

  QUANT(0);
  for (y = 0; y < out_height; y ++)
  {
    int duplicate_line = 1;
    if ((y & 63) == 0)
      Image_note_progress(image, y, out_height);

    if (errline != errlast)
    {
      errlast = errline;
      duplicate_line = 0;
      Image_get_row(image, in, errline);
      (*colorfunc)(in, out, image_width, image_bpp, cmap, &nv,
		   escp2_hue_adjustment(model), escp2_lum_adjustment(model),
		   escp2_sat_adjustment(model));
    }
    QUANT(1);

    if (nv.image_type == IMAGE_MONOCHROME)
      dither_monochrome(out, y, dither, black, duplicate_line);
    else if (output_type == OUTPUT_GRAY)
      dither_black(out, y, dither, black, duplicate_line);
    else
      dither_cmyk(out, y, dither, cyan, lcyan, magenta, lmagenta,
		  yellow, dyellow, black, duplicate_line);
    QUANT(2);

    if (use_softweave)
      escp2_write_weave(weave, prn, length, ydpi, model, out_width, left,
			xdpi, physical_xdpi,
			cyan, magenta, yellow, black, lcyan, lmagenta,
			dyellow);
    else
      escp2_write_microweave(prn, black, cyan, magenta, yellow, lcyan,
			     lmagenta, dyellow, length, xdpi, ydpi, model,
			     out_width, left, bits);
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
  Image_progress_conclude(image);
  if (use_softweave)
    escp2_flush_all(weave, model, out_width, left, ydpi, xdpi, physical_xdpi,
		    prn);
  else
    escp2_free_microweave();
  QUANT(5);

  free_dither(dither);

 /*
  * Cleanup...
  */

  free_lut(&nv);
  free(in);
  free(out);
  if (use_softweave)
    destroy_weave(weave);

  if (black != NULL)
    free(black);
  if (cyan != NULL)
    {
      free(cyan);
      free(magenta);
      free(yellow);
    }
  if (lcyan != NULL)
    {
      free(lcyan);
      free(lmagenta);
    }

  escp2_deinit_printer(prn, &init);
#ifdef QUANTIFY
  print_timers();
#endif
}

static unsigned char *microweave_s = 0;
static unsigned char *microweave_comp_ptr[7][4];
static int microweave_setactive[7][4];
static int accumulated_spacing = 0;
static int last_color = -1;

#define MICRO_S(c, l) (microweave_s + COMPBUFWIDTH * (l) + COMPBUFWIDTH * (c) * 4)

static void
escp2_init_microweave(int top)
{
  if (!microweave_s)
    microweave_s = malloc(6 * 4 * COMPBUFWIDTH);
  accumulated_spacing = top;
}

static void
escp2_free_microweave()
{
  if (microweave_s)
    {
      free(microweave_s);
      microweave_s = NULL;
    }
}

static int
escp2_do_microweave_pack(const unsigned char *line,
			 int length,
			 int oversample,
			 int bits,
			 int color)
{
  static unsigned char *pack_buf = NULL;
  static unsigned char *s[4] = { NULL, NULL, NULL, NULL };
  const unsigned char *in;
  int i;
  int retval = 0;
  if (!pack_buf)
    pack_buf = malloc(COMPBUFWIDTH);
  for (i = 0; i < oversample; i++)
    {
      if (!s[i])
	s[i] = malloc(COMPBUFWIDTH);
    }

  if (!line ||
      (line[0] == 0 && memcmp(line, line + 1, (bits * length) - 1) == 0))
    {
      for (i = 0; i < 4; i++)
	microweave_setactive[color][i] = 0;
      return 0;
    }
  if (bits == 1)
    in = line;
  else
    {
      stp_fold(line, length, pack_buf);
      in = pack_buf;
    }
  switch (oversample)
    {
    case 1:
      memcpy(s[0], in, bits * length);
      break;
    case 2:
      stp_unpack_2(length, bits, in, s[0], s[1]);
      break;
    case 4:
      stp_unpack_4(length, bits, in, s[0], s[1], s[2], s[3]);
      break;
    }
  for (i = 0; i < oversample; i++)
    {
      microweave_setactive[color][i] =
	stp_pack(s[i], length * bits, MICRO_S(color, i),
		 &(microweave_comp_ptr[color][i]));
      retval |= microweave_setactive[color][i];
    }
  return retval;
}

static void
escp2_write_microweave(FILE          *prn,	/* I - Print file or command */
		       const unsigned char *k,	/* I - Output bitmap data */
		       const unsigned char *c,	/* I - Output bitmap data */
		       const unsigned char *m,	/* I - Output bitmap data */
		       const unsigned char *y,	/* I - Output bitmap data */
		       const unsigned char *lc,	/* I - Output bitmap data */
		       const unsigned char *lm,	/* I - Output bitmap data */
		       const unsigned char *dy,	/* I - Output bitmap data */
		       int           length,	/* I - Length of bitmap data */
		       int           xdpi,	/* I - Horizontal resolution */
		       int           ydpi,	/* I - Vertical resolution */
		       int           model,	/* I - Printer model */
		       int           width,	/* I - Printed width */
		       int           offset,	/* I - Offset from left side */
		       int	     bits)
{
  int i, j;
  int oversample = 1;
  int gsetactive = 0;
  if (xdpi > escp2_base_resolution)
    oversample = xdpi / escp2_base_resolution;

  gsetactive |= escp2_do_microweave_pack(k, length, oversample, bits, 0);
  gsetactive |= escp2_do_microweave_pack(m, length, oversample, bits, 1);
  gsetactive |= escp2_do_microweave_pack(c, length, oversample, bits, 2);
  gsetactive |= escp2_do_microweave_pack(y, length, oversample, bits, 3);
  gsetactive |= escp2_do_microweave_pack(lm, length, oversample, bits, 4);
  gsetactive |= escp2_do_microweave_pack(lc, length, oversample, bits, 5);
  gsetactive |= escp2_do_microweave_pack(dy, length, oversample, bits, 6);
  if (!gsetactive)
    {
      accumulated_spacing++;
      return;
    }
  for (i = 0; i < oversample; i++)
    {
      for (j = 0; j < 7; j++)
	{
	  if (!microweave_setactive[j][i])
	    continue;
	  if (accumulated_spacing > 0)
	    fprintf(prn, "\033(v\002%c%c%c", 0, accumulated_spacing % 256,
		    (accumulated_spacing >> 8) % 256);
	  accumulated_spacing = 0;
	  /*
	   * Set the print head position.
	   */

	  if (escp2_max_hres(model) >= 1440 && xdpi > escp2_base_resolution)
	    {
	      if (!escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK,
				 MODEL_VARIABLE_NORMAL))
		{
		  if (((offset * xdpi / 1440) + i) > 0)
		    fprintf(prn, "\033($%c%c%c%c%c%c", 4, 0,
			    ((offset * xdpi / 1440) + i) & 255,
			    (((offset * xdpi / 1440) + i) >> 8) & 255,
			    (((offset * xdpi / 1440) + i) >> 16) & 255,
			    (((offset * xdpi / 1440) + i) >> 24) & 255);
		}
	      else
		{
		  if (((offset * 1440 / ydpi) + i) > 0)
		    fprintf(prn, "\033(\\%c%c%c%c%c%c", 4, 0, 160, 5,
			    ((offset * 1440 / ydpi) + i) & 255,
			    ((offset * 1440 / ydpi) + i) >> 8);
		}
	    }
	  else
	    {
	      if (offset > 0)
		fprintf(prn, "\033\\%c%c", offset & 255, offset >> 8);
	    }
	  if (j != last_color)
	    {
	      if (!escp2_has_cap(model, MODEL_COLOR_MASK, MODEL_COLOR_4))
		fprintf(prn, "\033(r\002%c%c%c", 0, densities[j], colors[j]);
	      else
		fprintf(prn, "\033r%c", colors[j]);
	      last_color = j;
	    }
	  /*
	   * Send a line of raster graphics...
	   */

	  switch (ydpi)				/* Raster graphics header */
	    {
	    case 180 :
	      fwrite("\033.\001\024\024\001", 6, 1, prn);
	      break;
	    case 360 :
	      fwrite("\033.\001\012\012\001", 6, 1, prn);
	      break;
	    case 720 :
	      if (escp2_has_cap(model, MODEL_720DPI_MODE_MASK,
				MODEL_720DPI_600))
		fwrite("\033.\001\050\005\001", 6, 1, prn);
	      else
		fwrite("\033.\001\005\005\001", 6, 1, prn);
	      break;
	    }
	  putc(width & 255, prn);	/* Width of raster line in pixels */
	  putc(width >> 8, prn);

	  fwrite(MICRO_S(j, i), microweave_comp_ptr[j][i] - MICRO_S(j, i),
		 1, prn);
	  putc('\r', prn);
	}
    }
  accumulated_spacing++;
}


/*
 * "Soft" weave
 *
 * The Epson Stylus Color/Photo printers don't have memory to print
 * using all of the nozzles in the print head.  For example, the Stylus Photo
 * 700/EX has 32 nozzles.  At 720 dpi, with an 8" wide image, a single line
 * requires (8 * 720 * 6 / 8) bytes, or 4320 bytes (because the Stylus Photo
 * printers have 6 ink colors).  To use 32 nozzles would require 138240 bytes.
 * It's actually worse than that, though, because the nozzles are spaced 8
 * rows apart.  Therefore, in order to store enough data to permit sending the
 * page as a simple raster, the printer would require enough memory to store
 * 256 rows, or 1105920 bytes.  Considering that the Photo EX can print
 * 11" wide, we're looking at more like 1.5 MB.  In fact, these printers are
 * capable of 1440 dpi horizontal resolution.  This would require 3 MB.  The
 * printers actually have 64K-256K.
 *
 * With the newer (740/750 and later) printers it's even worse, since these
 * printers support multiple dot sizes.  But that's neither here nor there.
 *
 * Older Epson printers had a mode called MicroWeave (tm).  In this mode, the
 * host fed the printer individual rows of dots, and the printer bundled them
 * up and sent them to the print head in the correct order to achieve high
 * quality.  This MicroWeave mode still works in new printers, but the
 * implementation is very minimal: the printer uses exactly one nozzle of
 * each color (the first one).  This makes printing extremely slow (more than
 * 30 minutes for one 8.5x11" page), although the quality is extremely high
 * with no visible banding whatsoever.  It's not good for the print head,
 * though, since no ink is flowing through the other nozzles.  This leads to
 * drying of ink and possible permanent damage to the print head.
 *
 * By the way, although the Epson manual says that microweave mode should be
 * used at 720 dpi, 360 dpi continues to work in much the same way.  At 360
 * dpi, data is fed to the printer one row at a time on all Epson printers.
 * The pattern that the printer uses to print is very prone to banding.
 * However, 360 dpi is inherently a low quality mode; if you're using it,
 * presumably you don't much care about quality.
 *
 * Printers from roughly the Stylus Color 600 and later do not have the
 * capability to do MicroWeave correctly.  Instead, the host must arrange
 * the output in the order that it will be sent to the print head.  This
 * is a very complex process; the jets in the print head are spaced more
 * than one row (1/720") apart, so we can't simply send consecutive rows
 * of dots to the printer.  Instead, we have to pass e. g. the first, ninth,
 * 17th, 25th... rows in order for them to print in the correct position on
 * the paper.  This interleaving process is called "soft" weaving.
 *
 * This decision was probably made to save money on memory in the printer.
 * It certainly makes the driver code far more complicated than it would
 * be if the printer could arrange the output.  Is that a bad thing?
 * Usually this takes far less CPU time than the dithering process, and it
 * does allow us more control over the printing process, e. g. to reduce
 * banding.  Conceivably, we could even use this ability to map out bad
 * jets.
 *
 * Interestingly, apparently the Windows (and presumably Macintosh) drivers
 * for most or all Epson printers still list a "microweave" mode.
 * Experiments have demonstrated that this does not in fact use the
 * "microweave" mode of the printer.  Possibly it does nothing, or it uses
 * a different weave pattern from what the non-"microweave" mode does.
 * This is unnecessarily confusing.
 *
 * What makes this interesting is that there are many different ways of
 * of accomplishing this goal.  The naive way would be to divide the image
 * up into groups of 256 rows, and print all the mod8=0 rows in the first pass,
 * mod8=1 rows in the second, and so forth.  The problem with this approach
 * is that the individual ink jets are not perfectly uniform; some emit
 * slightly bigger or smaller drops than others.  Since each group of 8
 * adjacent rows is printed with the same nozzle, that means that there will
 * be distinct streaks of lighter and darker bands within the image (8 rows
 * is 1/90", which is visible; 1/720" is not).  Possibly worse is that these
 * patterns will repeat every 256 rows.  This creates banding patterns that
 * are about 1/3" wide.
 *
 * So we have to do something to break up this patterning.
 *
 * Epson does not publish the weaving algorithms that they use in their
 * bundled drivers.  Indeed, their developer web site
 * (http://www.ercipd.com/isv/edr_docs.htm) does not even describe how to
 * do this weaving at all; it says that the only way to achieve 720 dpi
 * is to use MicroWeave.  It does note (correctly) that 1440 dpi horizontal
 * can only be achieved by the driver (i. e. in software).  The manual
 * actually makes it fairly clear how to do this (it requires two passes
 * with horizontal head movement between passes), and it is presumably
 * possible to do this with MicroWeave.
 *
 * The information about how to do this is apparently available under NDA.
 * It's actually easy enough to reverse engineer what's inside a print file
 * with a simple Perl script.  There are presumably other printer commands
 * that are not documented and may not be as easy to reverse engineer.
 *
 * I considered a few algorithms to perform the weave.  The first one I
 * devised let me use only (jets - distance_between_jets + 1) nozzles, or
 * 25.  This is OK in principle, but it's slower than using all nozzles.
 * By playing around with it some more, I came up with an algorithm that
 * lets me use all of the nozzles, except near the top and bottom of the
 * page.
 *
 * This still produces some banding, though.  Even better quality can be
 * achieved by using multiple nozzles on the same line.  How do we do this?
 * In 1440x720 mode, we're printing two output lines at the same vertical
 * position.  However, if we want four passes, we have to effectively print
 * each line twice.  Actually doing this would increase the density, so
 * what we do is print half the dots on each pass.  This produces near-perfect
 * output, and it's far faster than using (pseudo) "MicroWeave".
 *
 * The current algorithm is not completely general.  The number of passes
 * is limited to (nozzles / gap).  On the Photo EX class printers, that limits
 * it to 4 -- 32 nozzles, an inter-nozzle gap of 8 lines.  Furthermore, there
 * are a number of routines that are only coded up to 8 passes.  Fortunately,
 * this is enough passes to get rid of most banding.  What's left is a very
 * fine pattern that is sometimes described as "corduroy", since the pattern
 * looks like that kind of fabric.
 *
 * Newer printers (those that support variable dot sizes, such as the 740,
 * 1200, etc.) have an additional complication: when used in softweave mode,
 * they operate at 360 dpi horizontal resolution.  This requires FOUR passes
 * to achieve 1440x720 dpi.  Thus, to enable us to break up each row
 * into separate sub-rows, we have to actually print each row eight times.
 * Fortunately, all such printers have 48 nozzles and a gap of 6 rows,
 * except for the high-speed 900, which uses 96 nozzles and a gap of 2 rows.
 *
 * I cannot let this entirely pass without commenting on the Stylus Color 440.
 * This is a very low-end printer with 21 (!) nozzles and a separation of 8.
 * The weave routine works correctly with single-pass printing, which is enough
 * to minimally achieve 720 dpi output (it's physically a 720 dpi printer).
 * However, the routine does not work correctly at more than one pass per row.
 * Therefore, this printer bands badly.
 *
 * Yet another complication is how to get near the top and bottom of the page.
 * This algorithm lets us print to within one head width of the top of the
 * page, and a bit more than one head width from the bottom.  That leaves a
 * lot of blank space.  Doing the weave properly outside of this region is
 * increasingly difficult as we get closer to the edge of the paper; in the
 * interior region, any nozzle can print any line, but near the top and
 * bottom edges, only some nozzles can print.  We've handled this for now by
 * using the naive way mentioned above near the borders, and switching over
 * to the high quality method in the interior.  Unfortunately, this means
 * that the quality is quite visibly degraded near the top and bottom of the
 * page.  Algorithms that degrade more gracefully are more complicated.
 * Epson does not advertise that the printers can print at the very top of the
 * page, although in practice most or all of them can.  I suspect that the
 * quality that can be achieved very close to the top is poor enough that
 * Epson does not want to allow printing there.  That is a valid decision,
 * although we have taken another approach.
 *
 * To compute the weave information, we need to start with the following
 * information:
 *
 * 1) The number of jets the print head has for each color;
 *
 * 2) The separation in rows between the jets;
 *
 * 3) The horizontal resolution of the printer;
 *
 * 4) The desired horizontal resolution of the output;
 *
 * 5) The desired extra passes to reduce banding.
 *
 * As discussed above, each row is actually printed in one or more passes
 * of the print head; we refer to these as subpasses.  For example, if we're
 * printing at 1440(h)x720(v) on a printer with true horizontal resolution of
 * 360 dpi, and we wish to print each line twice with different nozzles
 * to reduce banding, we need to use 8 subpasses.  The dither routine
 * will feed us a complete row of bits for each color; we have to split that
 * up, first by round robining the bits to ensure that they get printed at
 * the right micro-position, and then to split up the bits that are actually
 * turned on into two equal chunks to reduce banding.
 *
 * Given the above information, and the desired row index and subpass (which
 * together form a line number), we can compute:
 *
 * 1) Which pass this line belongs to.  Passes are numbered consecutively,
 *    and each pass must logically (see #3 below) start at no smaller a row
 *    number than the previous pass, as the printer cannot advance by a
 *    negative amount.
 *
 * 2) Which jet will print this line.
 *
 * 3) The "logical" first line of this pass.  That is, what line would be
 *    printed by jet 0 in this pass.  This number may be less than zero.
 *    If it is, there are ghost lines that don't actually contain any data.
 *    The difference between the logical first line of this pass and the
 *    logical first line of the preceding pass tells us how many lines must
 *    be advanced.
 *
 * 4) The "physical" first line of this pass.  That is, the first line index
 *    that is actually printed in this pass.  This information lets us know
 *    when we must prepare this pass.
 *
 * 5) The last line of this pass.  This lets us know when we must actually
 *    send this pass to the printer.
 *
 * 6) The number of ghost rows this pass contains.  We must still send the
 *    ghost data to the printer, so this lets us know how much data we must
 *    fill in prior to the start of the pass.
 *
 * The bookkeeping to keep track of all this stuff is quite hairy, and needs
 * to be documented separately.
 *
 * The routine initialize_weave calculates the basic parameters, given
 * the number of jets and separation between jets, in rows.
 *
 * -- Robert Krawitz <rlk@alum.mit.edu) November 3, 1999
 */

#endif /* !WEAVETEST */

typedef struct			/* Weave parameters for a specific row */
{
  int row;			/* Absolute row # */
  int pass;			/* Computed pass # */
  int jet;			/* Which physical nozzle we're using */
  int missingstartrows;		/* Phantom rows (nonexistent rows that */
				/* would be printed by nozzles lower than */
				/* the first nozzle we're using this pass; */
				/* with the current algorithm, always zero */
  int logicalpassstart;		/* Offset in rows (from start of image) */
				/* that the printer must be for this row */
				/* to print correctly with the specified jet */
  int physpassstart;		/* Offset in rows to the first row printed */
				/* in this pass.  Currently always equal to */
				/* logicalpassstart */
  int physpassend;		/* Offset in rows (from start of image) to */
				/* the last row that will be printed this */
				/* pass (assuming that we're printing a full */
				/* pass). */
} weave_t;

typedef struct			/* Weave parameters for a specific pass */
{
  int pass;			/* Absolute pass number */
  int missingstartrows;		/* All other values the same as weave_t */
  int logicalpassstart;
  int physpassstart;
  int physpassend;
  int subpass;
} pass_t;

typedef union {			/* Offsets from the start of each line */
  unsigned long v[7];		/* (really pass) */
  struct {
    unsigned long k;
    unsigned long m;
    unsigned long c;
    unsigned long y;
    unsigned long M;
    unsigned long C;
    unsigned long Y;
  } p;
} lineoff_t;

typedef union {			/* Is this line active? */
  char v[7];			/* (really pass) */
  struct {
    char k;
    char m;
    char c;
    char y;
    char M;
    char C;
    char Y;
  } p;
} lineactive_t;

typedef union {			/* Base pointers for each pass */
  unsigned char *v[6];
  struct {
    unsigned char *k;
    unsigned char *m;
    unsigned char *c;
    unsigned char *y;
    unsigned char *M;
    unsigned char *C;
    unsigned char *Y;
  } p;
} linebufs_t;

typedef struct {
  linebufs_t *linebases;	/* Base address of each row buffer */
  lineoff_t *lineoffsets;	/* Offsets within each row buffer */
  lineactive_t *lineactive;	/* Does this line have anything printed? */
  int *linecounts;		/* How many rows we've printed this pass */
  pass_t *passes;		/* Circular list of pass numbers */
  int last_pass_offset;		/* Starting row (offset from the start of */
				/* the page) of the most recently printed */
				/* pass (so we can determine how far to */
				/* advance the paper) */
  int last_pass;		/* Number of the most recently printed pass */

  int jets;			/* Number of jets per color */
  int separation;		/* Offset from one jet to the next in rows */
  void *weaveparm;		/* Weave calculation parameter block */

  int horizontal_weave;		/* Number of horizontal passes required */
				/* This is > 1 for some of the ultra-high */
				/* resolution modes */
  int vertical_subpasses;	/* Number of passes per line (for better */
				/* quality) */
  int vmod;			/* Number of banks of passes */
  int oversample;		/* Excess precision per row */
  int ncolors;			/* How many colors (1, 4, or 6) */
  int horizontal_width;		/* Line width in output pixels */
  int vertical_height;		/* Image height in output pixels */
  int firstline;		/* Actual first line (referenced to paper) */

  int bitwidth;			/* Bits per pixel */
  int lineno;
  int vertical_oversample;	/* Vertical oversampling */
  int current_vertical_subpass;
  int separation_rows;		/* Vertical spacing between rows. */
				/* This is used for the 1520/3000, which */
				/* use a funny value for the "print density */
				/* in the vertical direction". */
  int last_color;
} escp2_softweave_t;

#ifndef WEAVETEST
static inline int
get_color_by_params(int plane, int density)
{
  if (plane > 4 || plane < 0 || density > 1 || density < 0)
    return -1;
  return color_indices[density * 8 + plane];
}
#endif


/*
 * Initialize the weave parameters
 *
 * Rules:
 *
 * 1) Currently, osample * v_subpasses * v_subsample <= 8, and no one
 *    of these variables may exceed 4.
 *
 * 2) first_line >= 0
 *
 * 3) line_height < physlines
 *
 * 4) phys_lines >= 2 * jets * sep
 */
static void *
initialize_weave(int jets,	/* Width of print head */
		 int sep,	/* Separation in rows between jets */
		 int osample,	/* Horizontal oversample */
		 int v_subpasses, /* Vertical passes */
		 int v_subsample, /* Vertical oversampling */
		 colormode_t colormode,	/* mono, 4 color, 6 color */
		 int width,	/* bits/pixel */
		 int linewidth,	/* Width of a line, in pixels */
		 int lineheight, /* Number of lines that will be printed */
		 int separation_rows, /* Vertical spacing adjustment */
				/* for weird printers (1520/3000, */
				/* although they don't seem to do softweave */
				/* anyway) */
		 int first_line, /* First line that will be printed on page */
		 int phys_lines, /* Total height of the page in rows */
		 int weave_strategy) /* Which weaving pattern to use */
{
  int i;
  escp2_softweave_t *sw = malloc(sizeof (escp2_softweave_t));
  if (sw == 0)
    return sw;

  if (jets < 1)
    jets = 1;
  if (jets == 1 || sep < 1)
    sep = 1;
  if (v_subpasses < 1)
    v_subpasses = 1;

  sw->separation = sep;
  sw->jets = jets;
  sw->horizontal_weave = osample;
  sw->vertical_oversample = v_subsample;
  sw->vertical_subpasses = v_subpasses;
  sw->oversample = osample * v_subpasses * v_subsample;
  sw->firstline = first_line;
  sw->lineno = first_line;

  if (sw->oversample > jets) {
    fprintf(stderr, "Weave error: oversample (%d) > jets (%d)\n",
                    sw->oversample, jets);
    free(sw);
    return 0;
  }
  sw->weaveparm = initialize_weave_params(sw->separation, sw->jets,
                                          sw->oversample, first_line,
                                          first_line + lineheight - 1,
                                          phys_lines, weave_strategy);

  /*
   * The value of vmod limits how many passes may be unfinished at a time.
   * If pass x is not yet printed, pass x+vmod cannot be started.
   */
  sw->vmod = 2 * sw->separation * sw->oversample;
  sw->separation_rows = separation_rows;

  sw->bitwidth = width;

  sw->last_pass_offset = 0;
  sw->last_pass = -1;
  sw->current_vertical_subpass = 0;
  sw->last_color = -1;

  switch (colormode)
    {
    case COLOR_MONOCHROME:
      sw->ncolors = 1;
      break;
    case COLOR_CMYK:
      sw->ncolors = 4;
      break;
    case COLOR_CCMMYK:
      sw->ncolors = 6;
      break;
    case COLOR_CCMMYYK:
      sw->ncolors = 7;
      break;
    }

  /*
   * It's possible for the "compression" to actually expand the line by
   * one part in 128.
   */

  sw->horizontal_width = (linewidth + 128 + 7) * 129 / 128;
  sw->vertical_height = lineheight;
  sw->lineoffsets = malloc(sw->vmod * sizeof(lineoff_t));
  memset(sw->lineoffsets, 0, sw->vmod * sizeof(lineoff_t));
  sw->lineactive = malloc(sw->vmod * sizeof(lineactive_t));
  sw->linebases = malloc(sw->vmod * sizeof(linebufs_t));
  sw->passes = malloc(sw->vmod * sizeof(pass_t));
  sw->linecounts = malloc(sw->vmod * sizeof(int));
  memset(sw->linecounts, 0, sw->vmod * sizeof(int));

  for (i = 0; i < sw->vmod; i++)
    {
      int j;
      sw->passes[i].pass = -1;
      for (j = 0; j < sw->ncolors; j++)
	{
	  sw->linebases[i].v[j] =
	    malloc(jets * sw->bitwidth * sw->horizontal_width / 8);
	}
    }
  return (void *) sw;
}

static void
destroy_weave(void *vsw)
{
  int i, j;
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  free(sw->linecounts);
  free(sw->passes);
  free(sw->lineactive);
  free(sw->lineoffsets);
  for (i = 0; i < sw->vmod; i++)
    {
      for (j = 0; j < sw->ncolors; j++)
	{
	  free(sw->linebases[i].v[j]);
	}
    }
  free(sw->linebases);
  destroy_weave_params(sw->weaveparm);
  free(vsw);
}

static void
weave_parameters_by_row(const escp2_softweave_t *sw, int row,
			int vertical_subpass, weave_t *w)
{
  static const escp2_softweave_t *scache = 0;
  static weave_t wcache;
  static int rcache = -2;
  static int vcache = -2;
  int jetsused;

  if (scache == sw && rcache == row && vcache == vertical_subpass)
    {
      memcpy(w, &wcache, sizeof(weave_t));
      return;
    }
  scache = sw;
  rcache = row;
  vcache = vertical_subpass;

  w->row = row;
  calculate_row_parameters(sw->weaveparm, row, vertical_subpass,
                           &w->pass, &w->jet, &w->logicalpassstart,
                           &w->missingstartrows, &jetsused);

  w->physpassstart = w->logicalpassstart + sw->separation * w->missingstartrows;
  w->physpassend = w->physpassstart + sw->separation * (jetsused - 1);

  memcpy(&wcache, w, sizeof(weave_t));
#if 0
  printf("row %d, jet %d of pass %d "
         "(pos %d, start %d, end %d, missing rows %d\n",
         w->row, w->jet, w->pass, w->logicalpassstart, w->physpassstart,
         w->physpassend, w->missingstartrows);
#endif
}

#ifndef WEAVETEST

static lineoff_t *
get_lineoffsets(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->lineoffsets[w.pass % sw->vmod]);
}

static lineactive_t *
get_lineactive(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->lineactive[w.pass % sw->vmod]);
}

static int *
get_linecount(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->linecounts[w.pass % sw->vmod]);
}

static const linebufs_t *
get_linebases(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->linebases[w.pass % sw->vmod]);
}

static pass_t *
get_pass_by_row(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->passes[w.pass % sw->vmod]);
}

static lineoff_t *
get_lineoffsets_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->lineoffsets[pass % sw->vmod]);
}

static lineactive_t *
get_lineactive_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->lineactive[pass % sw->vmod]);
}

static int *
get_linecount_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->linecounts[pass % sw->vmod]);
}

static const linebufs_t *
get_linebases_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->linebases[pass % sw->vmod]);
}

static pass_t *
get_pass_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->passes[pass % sw->vmod]);
}

/*
 * If there are phantom rows at the beginning of a pass, fill them in so
 * that the printer knows exactly what it doesn't have to print.  We're
 * using RLE compression here.  Each line must be specified independently,
 * so we have to compute how many full blocks (groups of 128 bytes, or 1024
 * "off" pixels) and how much leftover is needed.  Note that we can only
 * RLE-encode groups of 2 or more bytes; single bytes must be specified
 * with a count of 1.
 */

static void
fillin_start_rows(const escp2_softweave_t *sw, int row, int subpass,
		  int width, int missingstartrows)
{
  lineoff_t *offsets = get_lineoffsets(sw, row, subpass);
  const linebufs_t *bufs = get_linebases(sw, row, subpass);
  int i = 0;
  int k = 0;
  int j;
  width = sw->bitwidth * width * 8;
  for (k = 0; k < missingstartrows; k++)
    {
      int bytes_to_fill = width;
      int full_blocks = bytes_to_fill / (128 * 8);
      int leftover = (7 + (bytes_to_fill % (128 * 8))) / 8;
      int l = 0;

      while (l < full_blocks)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      (bufs[0].v[j][2 * i]) = 129;
	      (bufs[0].v[j][2 * i + 1]) = 0;
	    }
	  i++;
	  l++;
	}
      if (leftover == 1)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      (bufs[0].v[j][2 * i]) = 1;
	      (bufs[0].v[j][2 * i + 1]) = 0;
	    }
	  i++;
	}
      else if (leftover > 0)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      (bufs[0].v[j][2 * i]) = 257 - leftover;
	      (bufs[0].v[j][2 * i + 1]) = 0;
	    }
	  i++;
	}
    }
  for (j = 0; j < sw->ncolors; j++)
    offsets[0].v[j] = 2 * i;
}

static void
initialize_row(const escp2_softweave_t *sw, int row, int width)
{
  weave_t w;
  int i;
  for (i = 0; i < sw->oversample; i++)
    {
      weave_parameters_by_row(sw, row, i, &w);
      if (w.physpassstart == row)
	{
	  lineoff_t *lineoffs = get_lineoffsets(sw, row, i);
	  lineactive_t *lineactive = get_lineactive(sw, row, i);
	  int *linecount = get_linecount(sw, row, i);
	  int j;
	  pass_t *pass = get_pass_by_row(sw, row, i);
	  pass->pass = w.pass;
	  pass->missingstartrows = w.missingstartrows;
	  pass->logicalpassstart = w.logicalpassstart;
	  pass->physpassstart = w.physpassstart;
	  pass->physpassend = w.physpassend;
	  pass->subpass = i;
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      if (lineoffs[0].v[j] != 0)
		fprintf(stderr,
			"WARNING: pass %d subpass %d row %d: lineoffs %ld\n",
			w.pass, i, row, lineoffs[0].v[j]);
	      lineoffs[0].v[j] = 0;
	      lineactive[0].v[j] = 0;
	    }
	  if (*linecount != 0)
	    fprintf(stderr,
		    "WARNING: pass %d subpass %d row %d: linecount %d\n",
		    w.pass, i, row, *linecount);
	  *linecount = 0;
	  if (w.missingstartrows > 0)
	    fillin_start_rows(sw, row, i, width, w.missingstartrows);
	}
    }
}

/*
 * A fair bit of this code is duplicated from escp2_write.  That's rather
 * a pity.  It's also not correct for any but the 6-color printers.  One of
 * these days I'll unify it.
 */
static void
flush_pass(escp2_softweave_t *sw, int passno, int model, int width,
	   int hoffset, int ydpi, int xdpi, int physical_xdpi,
	   FILE *prn, int vertical_subpass)
{
  int j;
  lineoff_t *lineoffs = get_lineoffsets_by_pass(sw, passno);
  lineactive_t *lineactive = get_lineactive_by_pass(sw, passno);
  const linebufs_t *bufs = get_linebases_by_pass(sw, passno);
  pass_t *pass = get_pass_by_pass(sw, passno);
  int *linecount = get_linecount_by_pass(sw, passno);
  int lwidth = (width + (sw->horizontal_weave - 1)) / sw->horizontal_weave;
  int microoffset = vertical_subpass & (sw->horizontal_weave - 1);
  if (ydpi > escp2_max_vres(model))
    ydpi = escp2_max_vres(model);
  for (j = 0; j < sw->ncolors; j++)
    {
      if (lineactive[0].v[j] == 0)
	{
	  lineoffs[0].v[j] = 0;
	  continue;
	}
      if (pass->logicalpassstart > sw->last_pass_offset)
	{
	  int advance = pass->logicalpassstart - sw->last_pass_offset -
	    (sw->separation_rows - 1);
	  int a0 = advance         % 256;
	  int a1 = (advance >> 8)  % 256;
	  int a2 = (advance >> 16) % 256;
	  int a3 = (advance >> 24) % 256;
	  if (!escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK,
			     MODEL_VARIABLE_NORMAL))
	    fprintf(prn, "\033(v\004%c%c%c%c%c", 0, a0, a1, a2, a3);
	  else
	    fprintf(prn, "\033(v\002%c%c%c", 0, a0, a1);
	  sw->last_pass_offset = pass->logicalpassstart;
	}
      if (last_color != j)
	{
	  if (!escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK,
			     MODEL_VARIABLE_NORMAL))
	    ;
	  else if (!escp2_has_cap(model, MODEL_COLOR_MASK, MODEL_COLOR_4))
	    fprintf(prn, "\033(r\002%c%c%c", 0, densities[j], colors[j]);
	  else
	    fprintf(prn, "\033r%c", colors[j]);
	  last_color = j;
	}
      if (escp2_max_hres(model) >= 1440)
	{
	  /* FIXME need a more general way of specifying column */
	  /* separation */
	  if (escp2_has_cap(model, MODEL_COMMAND_MASK, MODEL_COMMAND_1999) &&
	      !(escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK,
			      MODEL_VARIABLE_NORMAL)))
	    {
	      int pos = ((hoffset * xdpi / ydpi) + microoffset);
	      if (pos > 0)
		fprintf(prn, "\033($%c%c%c%c%c%c", 4, 0,
			pos & 255, (pos >> 8) & 255,
			(pos >> 16) & 255, (pos >> 24) & 255);
	    }
	  else
	    {
	      int pos = ((hoffset * escp2_max_hres(model) / ydpi) +
			 microoffset);
	      if (pos > 0)
		fprintf(prn, "\033(\\%c%c%c%c%c%c", 4, 0, 160, 5,
			pos & 255, pos >> 8);
	    }
	}
      else
	{
	  int pos = (hoffset + microoffset);
	  if (pos > 0)
	    fprintf(prn, "\033\\%c%c", pos & 255, pos >> 8);
	}
      if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_NORMAL))
	{
	  int ydotsep = 3600 / ydpi;
	  int xdotsep = 3600 / physical_xdpi;
	  if (escp2_has_cap(model, MODEL_720DPI_MODE_MASK, MODEL_720DPI_600))
	    fprintf(prn, "\033.%c%c%c%c", 1, 8 * ydotsep, xdotsep,
		    *linecount + pass->missingstartrows);
	  else if (escp2_pseudo_separation_rows(model) > 0)
	    fprintf(prn, "\033.%c%c%c%c", 1,
		    ydotsep * escp2_pseudo_separation_rows(model) , xdotsep,
		    *linecount + pass->missingstartrows);
	  else
	    fprintf(prn, "\033.%c%c%c%c", 1, ydotsep * sw->separation_rows,
		    xdotsep, *linecount + pass->missingstartrows);
	  putc(lwidth & 255, prn);	/* Width of raster line in pixels */
	  putc(lwidth >> 8, prn);
	}
      else
	{
	  int ncolor = (densities[j] << 4) | colors[j];
	  int nlines = *linecount + pass->missingstartrows;
	  int nwidth = sw->bitwidth * ((lwidth + 7) / 8);
	  fprintf(prn, "\033i%c%c%c%c%c%c%c", ncolor, 1, sw->bitwidth,
		  nwidth & 255, nwidth >> 8, nlines & 255, nlines >> 8);
	}

      fwrite(bufs[0].v[j], lineoffs[0].v[j], 1, prn);
      lineoffs[0].v[j] = 0;
      putc('\r', prn);
    }
  *linecount = 0;
  sw->last_pass = pass->pass;
  pass->pass = -1;
}

static void
add_to_row(escp2_softweave_t *sw, int row, unsigned char *buf, size_t nbytes,
	   int plane, int density, int setactive,
	   lineoff_t *lineoffs, lineactive_t *lineactive,
	   const linebufs_t *bufs)
{
  int color = get_color_by_params(plane, density);
  memcpy(bufs[0].v[color] + lineoffs[0].v[color], buf, nbytes);
  lineoffs[0].v[color] += nbytes;
  if (setactive)
    lineactive[0].v[color] = 1;
}

static void
escp2_flush(void *vsw, int model, int width, int hoffset,
	    int ydpi, int xdpi, int physical_xdpi, FILE *prn)
{
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  while (1)
    {
      pass_t *pass = get_pass_by_pass(sw, sw->last_pass + 1);
      /*
       * This ought to be   pass->physpassend >  sw->lineno
       * but that causes rubbish to be output for some reason.
       */
      if (pass->pass < 0 || pass->physpassend >= sw->lineno)
	return;
      flush_pass(sw, pass->pass, model, width, hoffset, ydpi, xdpi,
		 physical_xdpi, prn, pass->subpass);
    }
}

static void
escp2_flush_all(void *vsw, int model, int width, int hoffset,
		int ydpi, int xdpi, int physical_xdpi, FILE *prn)
{
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  while (1)
    {
      pass_t *pass = get_pass_by_pass(sw, sw->last_pass + 1);
      /*
       * This ought to be   pass->physpassend >  sw->lineno
       * but that causes rubbish to be output for some reason.
       */
      if (pass->pass < 0)
	return;
      flush_pass(sw, pass->pass, model, width, hoffset, ydpi, xdpi,
		 physical_xdpi, prn, pass->subpass);
    }
}

static void
finalize_row(escp2_softweave_t *sw, int row, int model, int width,
	     int hoffset, int ydpi, int xdpi, int physical_xdpi,
	     FILE *prn)
{
  int i;
#if 0
  printf("Finalizing row %d...\n", row);
#endif
  for (i = 0; i < sw->oversample; i++)
    {
      weave_t w;
      int *lines = get_linecount(sw, row, i);
      weave_parameters_by_row(sw, row, i, &w);
      (*lines)++;
      if (w.physpassend == row)
	{
#if 0
	  printf("Pass=%d, physpassend=%d, row=%d, lineno=%d, trying to flush...\n", w.pass, w.physpassend, row, sw->lineno);
#endif
	  escp2_flush(sw, model, width, hoffset, ydpi, xdpi, physical_xdpi, prn);
	}
    }
}

static void
escp2_write_weave(void *        vsw,
		  FILE          *prn,	/* I - Print file or command */
		  int           length,	/* I - Length of bitmap data */
		  int           ydpi,	/* I - Vertical resolution */
		  int           model,	/* I - Printer model */
		  int           width,	/* I - Printed width */
		  int           offset,	/* I - Offset from left side of page */
		  int		xdpi,
		  int		physical_xdpi,
		  const unsigned char *c,
		  const unsigned char *m,
		  const unsigned char *y,
		  const unsigned char *k,
		  const unsigned char *C,
		  const unsigned char *M,
		  const unsigned char *Y)
{
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  static unsigned char *s[8];
  static unsigned char *fold_buf;
  static unsigned char *comp_buf;
  lineoff_t *lineoffs[8];
  lineactive_t *lineactives[8];
  const linebufs_t *bufs[8];
  int xlength = (length + sw->horizontal_weave - 1) / sw->horizontal_weave;
  unsigned char *comp_ptr;
  int i, j;
  int setactive;
  int h_passes = sw->horizontal_weave * sw->vertical_subpasses;
  const unsigned char *cols[7];
  cols[0] = k;
  cols[1] = m;
  cols[2] = c;
  cols[3] = y;
  cols[4] = M;
  cols[5] = C;
  cols[6] = Y;
  if (!fold_buf)
    fold_buf = malloc(COMPBUFWIDTH);
  if (!comp_buf)
    comp_buf = malloc(COMPBUFWIDTH);
  if (sw->current_vertical_subpass == 0)
    initialize_row(sw, sw->lineno, xlength);

  for (i = 0; i < h_passes; i++)
    {
      int cpass = sw->current_vertical_subpass * h_passes;
      if (!s[i])
	s[i] = malloc(COMPBUFWIDTH);
      lineoffs[i] = get_lineoffsets(sw, sw->lineno, cpass + i);
      lineactives[i] = get_lineactive(sw, sw->lineno, cpass + i);
      bufs[i] = get_linebases(sw, sw->lineno, cpass + i);
    }

  for (j = 0; j < sw->ncolors; j++)
    {
      if (cols[j])
	{
	  const unsigned char *in;
	  if (sw->bitwidth == 2)
	    {
	      stp_fold(cols[j], length, fold_buf);
	      in = fold_buf;
	    }
	  else
	    in = cols[j];
	  if (h_passes > 1)
	    {
	      switch (sw->horizontal_weave)
		{
		case 2:
		  stp_unpack_2(length, sw->bitwidth, in, s[0], s[1]);
		  break;
		case 4:
		  stp_unpack_4(length, sw->bitwidth, in,
			       s[0], s[1], s[2], s[3]);
		  break;
		case 8:
		  stp_unpack_8(length, sw->bitwidth, in,
			       s[0], s[1], s[2], s[3],
			       s[4], s[5], s[6], s[7]);
		  break;
		}
	      switch (sw->vertical_subpasses)
		{
		case 4:
		  switch (sw->horizontal_weave)
		    {
		    case 1:
		      stp_split_4(length, sw->bitwidth, in,
				  s[0], s[1], s[2], s[3]);
		      break;
		    case 2:
		      stp_split_4(length, sw->bitwidth, s[0],
				  s[0], s[2], s[4], s[6]);
		      stp_split_4(length, sw->bitwidth, s[1],
				  s[1], s[3], s[5], s[7]);
		      break;
		    }
		  break;
		case 2:
		  switch (sw->horizontal_weave)
		    {
		    case 1:
		      stp_split_2(xlength, sw->bitwidth, in, s[0], s[1]);
		      break;
		    case 2:
		      stp_split_2(xlength, sw->bitwidth, s[0], s[0], s[2]);
		      stp_split_2(xlength, sw->bitwidth, s[1], s[1], s[3]);
		      break;
		    case 4:
		      stp_split_2(xlength, sw->bitwidth, s[0], s[0], s[4]);
		      stp_split_2(xlength, sw->bitwidth, s[1], s[1], s[5]);
		      stp_split_2(xlength, sw->bitwidth, s[2], s[2], s[6]);
		      stp_split_2(xlength, sw->bitwidth, s[3], s[3], s[7]);
		      break;
		    }
		  break;
		  /* case 1 is taken care of because the various unpack */
		  /* functions will do the trick themselves */
		}
	      for (i = 0; i < h_passes; i++)
		{
		  setactive = stp_pack(s[i], sw->bitwidth * xlength,
					 comp_buf, &comp_ptr);
		  add_to_row(sw, sw->lineno, comp_buf, comp_ptr - comp_buf,
			     colors[j], densities[j], setactive,
			     lineoffs[i], lineactives[i], bufs[i]);
		}
	    }
	  else
	    {
	      setactive = stp_pack(in, length * sw->bitwidth,
				     comp_buf, &comp_ptr);
	      fprintf(stderr, "add_to_row length %d\n", comp_ptr - comp_buf);
	      add_to_row(sw, sw->lineno, comp_buf, comp_ptr - comp_buf,
			 colors[j], densities[j], setactive,
			 lineoffs[0], lineactives[0], bufs[0]);
	    }
	}
    }
  sw->current_vertical_subpass++;
  if (sw->current_vertical_subpass >= sw->vertical_oversample)
    {
      finalize_row(sw, sw->lineno, model, width, offset, ydpi, xdpi,
		   physical_xdpi, prn);
      sw->lineno++;
      sw->current_vertical_subpass = 0;
    }
}

#endif
