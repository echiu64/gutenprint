/*
 * "$Id$"
 *
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
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

/*
 * Large parts of this file (mainly the ink handling) is based on 
 * print-escp2.c -- refer to README.new-printer on how to adjust the colors
 * for a certain model.
 */

/* TODO-LIST
 *
 *   * implement the left border
 *   * adjust the colors of all supported models
 *
 */

#include <stdarg.h>
#include "print.h"

/* #define DEBUG */

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
 *              0                      standard dot sizes are used.
 *              1                      drop modulation is used.
 */

/* We know a per-model base resolution (like 180dpi or 150dpi)
 * and multipliers for the base resolution in the dotsize-, densities- 
 * and inklist:
 * for 180dpi base resolution we would have 
 *   s_r11_4 for 4color ink @180dpi
 *   s_r22_4 for 4color ink @360dpi
 *   s_r33_4 for 4color ink @720dpi
 *   s_r43_4 for 4color ink @1440x720dpi
 */

typedef struct canon_dot_sizes
{
  int dot_r11;    /*  180x180  or   150x150  */
  int dot_r22;    /*  360x360  or   300x300  */
  int dot_r33;    /*  720x720  or   600x600  */
  int dot_r43;    /* 1440x720  or  1200x600  */
  int dot_r44;    /* 1440x1440 or  1200x1200 */
  int dot_r55;    /* 2880x2880 or  2400x2400 */
} canon_dot_size_t;

/*
 * Specify the base density for each available resolution.
 *
 */

typedef struct canon_densities
{
  double d_r11;  /*  180x180  or   150x150  */
  double d_r22;  /*  360x360  or   300x300  */
  double d_r33;  /*  720x720  or   600x600  */
  double d_r43;  /* 1440x720  or  1200x600  */
  double d_r44;  /* 1440x1440 or  1200x1200 */
  double d_r55;  /* 2880x2880 or  2400x2400 */
} canon_densities_t;



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

typedef struct canon_variable_ink
{
  simple_dither_range_t *range;
  int count;
  double density;
} canon_variable_ink_t;

typedef struct canon_variable_inkset
{
  canon_variable_ink_t *c;
  canon_variable_ink_t *m;
  canon_variable_ink_t *y;
  canon_variable_ink_t *k;
} canon_variable_inkset_t;

/*
 * currenty unaccounted for are the 7color printers and the 3color ones
 * (which use CMY only printheads)
 *
 */

typedef struct canon_variable_inklist
{
  canon_variable_inkset_t *s_r11_4;    /*  180x180  or   150x150  */
  canon_variable_inkset_t *s_r22_4;    /*  360x360  or   300x300  */
  canon_variable_inkset_t *s_r33_4;    /*  720x720  or   600x600  */
  canon_variable_inkset_t *s_r43_4;    /* 1440x720  or  1200x600  */
  canon_variable_inkset_t *s_r44_4;    /* 1440x1440 or  1200x1200 */
  canon_variable_inkset_t *s_r55_4;    /* 2880x2880 or  2400x2400 */

  canon_variable_inkset_t *s_r11_6;    /*  180x180  or   150x150  */
  canon_variable_inkset_t *s_r22_6;    /*  360x360  or   300x300  */
  canon_variable_inkset_t *s_r33_6;    /*  720x720  or   600x600  */
  canon_variable_inkset_t *s_r43_6;    /* 1440x720  or  1200x600  */
  canon_variable_inkset_t *s_r44_6;    /* 1440x1440 or  1200x1200 */
  canon_variable_inkset_t *s_r55_6;    /* 2880x2880 or  2400x2400 */

  canon_variable_inkset_t *v_r11_4;    /*  180x180  or   150x150  */
  canon_variable_inkset_t *v_r22_4;    /*  360x360  or   300x300  */
  canon_variable_inkset_t *v_r33_4;    /*  720x720  or   600x600  */
  canon_variable_inkset_t *v_r43_4;    /* 1440x720  or  1200x600  */
  canon_variable_inkset_t *v_r44_4;    /* 1440x1440 or  1200x1200 */
  canon_variable_inkset_t *v_r55_4;    /* 2880x2880 or  2400x2400 */

  canon_variable_inkset_t *v_r11_6;    /*  180x180  or   150x150  */
  canon_variable_inkset_t *v_r22_6;    /*  360x360  or   300x300  */
  canon_variable_inkset_t *v_r33_6;    /*  720x720  or   600x600  */
  canon_variable_inkset_t *v_r43_6;    /* 1440x720  or  1200x600  */
  canon_variable_inkset_t *v_r44_6;    /* 1440x1440 or  1200x1200 */
  canon_variable_inkset_t *v_r55_6;    /* 2880x2880 or  2400x2400 */
} canon_variable_inklist_t;





static simple_dither_range_t photo_cyan_dither_ranges[] =
{
  { 0.25, 0x1, 0, 1 },
  { 1.0,  0x1, 1, 1 }
};

static canon_variable_ink_t photo_cyan_ink =
{
  photo_cyan_dither_ranges,
  sizeof(photo_cyan_dither_ranges) / sizeof(simple_dither_range_t),
  .75
};

static simple_dither_range_t photo_magenta_dither_ranges[] =
{
  { 0.26, 0x1, 0, 1 },
  { 1.0,  0x1, 1, 1 }
};

static canon_variable_ink_t photo_magenta_ink =
{
  photo_magenta_dither_ranges,
  sizeof(photo_magenta_dither_ranges) / sizeof(simple_dither_range_t),
  .75
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

static canon_variable_ink_t photo_6pl_ink =
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

static canon_variable_ink_t photo_6pl_1440_ink =
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

static canon_variable_ink_t photo_pigment_ink =
{
  photo_pigment_dither_ranges,
  sizeof(photo_pigment_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t photo_4pl_dither_ranges[] =
{
  { 0.15,  0x1, 0, 1 },
  { 0.227, 0x2, 0, 2 },
/*  { 0.333, 0x3, 0, 3 }, */
  { 0.45,  0x1, 1, 1 },
  { 0.68,  0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static canon_variable_ink_t photo_4pl_ink =
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

static canon_variable_ink_t photo_4pl_1440_ink =
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

static canon_variable_ink_t standard_6pl_ink =
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

static canon_variable_ink_t standard_6pl_1440_ink =
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

static canon_variable_ink_t standard_pigment_ink =
{
  standard_pigment_dither_ranges,
  sizeof(standard_pigment_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static simple_dither_range_t standard_4pl_dither_ranges[] =
{
  { 0.45,  0x1, 1, 1 },
  { 0.68,  0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static canon_variable_ink_t standard_4pl_ink =
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

static canon_variable_ink_t standard_4pl_1440_ink =
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

static canon_variable_ink_t standard_3pl_ink =
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

static canon_variable_ink_t standard_3pl_1440_ink =
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

static canon_variable_ink_t photo_multishot_ink =
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

static canon_variable_ink_t standard_multishot_ink =
{
  standard_multishot_dither_ranges,
  sizeof(standard_multishot_dither_ranges) / sizeof(simple_dither_range_t),
  1.0
};


static canon_variable_inkset_t standard_inks =
{
  NULL,
  NULL,
  NULL,
  NULL
};

static canon_variable_inkset_t photo_inks =
{
  &photo_cyan_ink,
  &photo_magenta_ink,
  NULL,
  NULL
};

static canon_variable_inkset_t canon_6pl_standard_inks =
{
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink
};

static canon_variable_inkset_t canon_6pl_photo_inks =
{
  &photo_6pl_ink,
  &photo_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink
};

static canon_variable_inkset_t canon_6pl_1440_standard_inks =
{
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink
};

static canon_variable_inkset_t canon_6pl_1440_photo_inks =
{
  &photo_6pl_1440_ink,
  &photo_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink
};

static canon_variable_inkset_t canon_pigment_standard_inks =
{
  &standard_pigment_ink,
  &standard_pigment_ink,
  &standard_pigment_ink,
  &standard_pigment_ink
};

static canon_variable_inkset_t canon_pigment_photo_inks =
{
  &photo_pigment_ink,
  &photo_pigment_ink,
  &standard_pigment_ink,
  &standard_pigment_ink
};

static canon_variable_inkset_t canon_4pl_standard_inks =
{
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink
};

static canon_variable_inkset_t canon_4pl_photo_inks =
{
  &photo_4pl_ink,
  &photo_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink
};

static canon_variable_inkset_t canon_4pl_1440_standard_inks =
{
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink
};

static canon_variable_inkset_t canon_4pl_1440_photo_inks =
{
  &photo_4pl_1440_ink,
  &photo_4pl_1440_ink,
  &standard_4pl_1440_ink,
  &standard_4pl_1440_ink
};

static canon_variable_inkset_t canon_3pl_standard_inks =
{
  &standard_3pl_ink,
  &standard_3pl_ink,
  &standard_3pl_ink,
  &standard_3pl_ink
};

static canon_variable_inkset_t canon_3pl_1440_standard_inks =
{
  &standard_3pl_1440_ink,
  &standard_3pl_1440_ink,
  &standard_3pl_1440_ink,
  &standard_3pl_1440_ink
};

static canon_variable_inkset_t canon_multishot_standard_inks =
{
  &standard_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink
};

static canon_variable_inkset_t canon_multishot_photo_inks =
{
  &photo_multishot_ink,
  &photo_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink
};

static canon_variable_inklist_t simple_4color_inks =
{
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static canon_variable_inklist_t simple_6color_inks =
{
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static canon_variable_inklist_t variable_6pl_4color_inks =
{
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  &canon_6pl_standard_inks,
  &canon_6pl_standard_inks,
  &canon_6pl_standard_inks,
  &canon_6pl_standard_inks,
  &canon_6pl_1440_standard_inks,
  &canon_6pl_standard_inks,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static canon_variable_inklist_t variable_6pl_6color_inks =
{
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &canon_6pl_standard_inks,
  &canon_6pl_standard_inks,
  &canon_6pl_standard_inks,
  &canon_6pl_standard_inks,
  &canon_6pl_1440_standard_inks,
  &canon_6pl_standard_inks,
  &canon_6pl_photo_inks,
  &canon_6pl_photo_inks,
  &canon_6pl_photo_inks,
  &canon_6pl_photo_inks,
  &canon_6pl_1440_photo_inks,
  &canon_6pl_photo_inks
};

static canon_variable_inklist_t variable_pigment_inks =
{
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &canon_pigment_standard_inks,
  &canon_pigment_standard_inks,
  &canon_pigment_standard_inks,
  &canon_pigment_standard_inks,
  &canon_pigment_standard_inks,
  &canon_pigment_standard_inks,
  &canon_pigment_photo_inks,
  &canon_pigment_photo_inks,
  &canon_pigment_photo_inks,
  &canon_pigment_photo_inks,
  &canon_pigment_photo_inks,
  &canon_pigment_photo_inks
};

static canon_variable_inklist_t variable_3pl_inks =
{
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  &canon_multishot_standard_inks,
  &canon_multishot_standard_inks,
  &canon_6pl_standard_inks,
  &canon_3pl_standard_inks,
  &canon_3pl_1440_standard_inks,
  &canon_3pl_standard_inks,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static canon_variable_inklist_t variable_4pl_4color_inks =
{
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  &canon_multishot_standard_inks,
  &canon_multishot_standard_inks,
  &canon_6pl_standard_inks,
  &canon_4pl_standard_inks,
  &canon_4pl_1440_standard_inks,
  &canon_4pl_standard_inks,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

static canon_variable_inklist_t variable_4pl_6color_inks =
{
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &standard_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &photo_inks,
  &canon_multishot_standard_inks,
  &canon_multishot_standard_inks,
  &canon_6pl_standard_inks,
  &canon_4pl_standard_inks,
  &canon_4pl_1440_standard_inks,
  &canon_4pl_standard_inks,
  &canon_multishot_photo_inks,
  &canon_multishot_photo_inks,
  &canon_6pl_photo_inks,
  &canon_4pl_photo_inks,
  &canon_4pl_1440_photo_inks,
  &canon_4pl_photo_inks
};


typedef enum {
  COLOR_MONOCHROME = 1,
  COLOR_CMY = 3,
  COLOR_CMYK = 4,
  COLOR_CCMMYK= 6,
  COLOR_CCMMYYK= 7
} colormode_t;

typedef struct canon_caps {
  int model;          /* model number as used in printers.xml */
  int max_width;      /* maximum printable paper size */
  int max_height;     
  int base_res;       /* base resolution - shall be 150 or 180 */
  int max_xdpi;       /* maximum horizontal resolution */
  int max_ydpi;       /* maximum vertical resolution */
  int max_quality;    
  int border_left;    /* left margin, points */
  int border_right;   /* right margin, points */
  int border_top;     /* absolute top margin, points */
  int border_bottom;  /* absolute bottom margin, points */
  int inks;           /* installable cartridges (CANON_INK_*) */
  int slots;          /* available paperslots */
  int features;       /* special bjl settings */
  canon_dot_size_t dot_sizes;	/* Vector of dot sizes for resolutions */
  canon_densities_t densities;	/* List of densities for each printer */
  canon_variable_inklist_t *inxs; /* Choices of inks for this printer */
} canon_cap_t;

static void canon_write_line(FILE *, canon_cap_t, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     int, int, int, int);

#define PHYSICAL_BPI 1440
#define MAX_OVERSAMPLED 4
#define MAX_BPP 2
#define BITS_PER_BYTE 8
#define COMPBUFWIDTH (PHYSICAL_BPI * MAX_OVERSAMPLED * MAX_BPP * \
	MAX_CARRIAGE_WIDTH / BITS_PER_BYTE)


/* Codes for possible ink-tank combinations.
 * Each combo is represented by the colors that can be used with
 * the installed ink-tank(s)
 * Combinations of the codes represent the combinations allowed for a model
 * Note that only preferrable combinations should be used 
 */
#define CANON_INK_K           1
#define CANON_INK_CMY         2
#define CANON_INK_CMYK        4
#define CANON_INK_CcMmYK      8
#define CANON_INK_CcMmYyK    16

#define CANON_INK_BLACK_MASK (CANON_INK_K|CANON_INK_CMYK|CANON_INK_CcMmYK)

#define CANON_INK_PHOTO_MASK (CANON_INK_CcMmYK|CANON_INK_CcMmYyK)

/* document feeding */
#define CANON_SLOT_ASF1    1
#define CANON_SLOT_ASF2    2
#define CANON_SLOT_MAN1    4
#define CANON_SLOT_MAN2    8

/* model peculiarities */
#define CANON_CAP_DMT       1<<0    /* Drop Modulation Technology */
#define CANON_CAP_MSB_FIRST 1<<1    /* how to send data           */
#define CANON_CAP_CMD61     1<<2    /* uses command #0x61         */
#define CANON_CAP_CMD6d     1<<3    /* uses command #0x6d         */
#define CANON_CAP_CMD70     1<<4    /* uses command #0x70         */
#define CANON_CAP_CMD72     1<<5    /* uses command #0x72         */




static canon_cap_t canon_model_capabilities[] =
{
  /* default settings for unkown models */

  {   -1, 8*72,11*72,180,180,20,20,20,20, CANON_INK_K, CANON_SLOT_ASF1, 0 },

  /* ******************************** */
  /*                                  */
  /* tested and color-adjusted models */
  /*                                  */
  /* ******************************** */




  /* ************************************ */
  /*                                      */
  /* tested models w/out color-adjustment */
  /*                                      */
  /* ************************************ */


  { /* Canon BJ 30 */
    30,
    9.5*72, 14*72,
    180, 360, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61,
    {0,0,-1,-1,-1,-1}, /* ??? */
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },

  { /* Canon BJC 4300 */
    4300,
    618, 936,      /* 8.58" x 13 " */
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_DMT,
    {0,0,-1,-1,-1,-1}, /* ??? */
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },

  { /* Canon BJC 4400 */
    4400,
    9.5*72, 14*72,
    180, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61 | CANON_CAP_DMT,
    {-1,0,-1,-1,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },

  { /* Canon BJC 6000 */
    6000,
    618, 936,      /* 8.58" x 13 " */
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_DMT,
    {-1,0,0,0,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },

  { /* Canon BJC 6200 */
    6200,
    618, 936,      /* 8.58" x 13 " */
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_DMT | CANON_CAP_CMD6d | CANON_CAP_CMD70,
    { -1, 1, 0, 0, -1, -1 },
    {  0, 1.8, 1, .5, 0, 0 },
    &variable_6pl_6color_inks
  },

  { /* Canon BJC 8200 */
    8200,
    11*72, 17*72,
    150, 1200,1200, 4,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_DMT | CANON_CAP_CMD72,
    {-1,0,0,-1,0,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },


  /* *************** */
  /*                 */
  /* untested models */
  /*                 */
  /* *************** */


  { /* Canon BJC 1000 */
    1000,
    11*72, 17*72,
    180, 360, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K | CANON_INK_CMY,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61,
    { 0,0,-1,-1,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },
  { /* Canon BJC 2000 */
    2000,
    11*72, 17*72,
    180, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61,
    { 0,0,-1,-1,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },
  { /* Canon BJC 3000 */
    3000,
    11*72, 17*72,
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61 | CANON_CAP_DMT,
    { -1,0,0,0,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },
  { /* Canon BJC 6100 */
    6100,
    11*72, 17*72,
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61,
    { -1,0,0,0,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },
  { /* Canon BJC 7000 */
    7000,
    11*72, 17*72,
    150, 1200, 600, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYyK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD6d | CANON_CAP_CMD70,
    { -1,0,0,0,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },
  { /* Canon BJC 7100 */
    7100,
    11*72, 17*72,
    150, 1200, 600, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYyK,
    CANON_SLOT_ASF1,
    0,
    { -1,0,0,0,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },

  /*****************************/
  /*                           */
  /*  extremely fuzzy models   */
  /* (might never work at all) */
  /*                           */
  /*****************************/

  { /* Canon BJC 5100 */
    5100,
    17*72, 22*72,
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_DMT,
    { -1,0,0,0,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },
  { /* Canon BJC 5500 */
    5500,
    22*72, 34*72,
    180, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61,
    { 0,0,-1,-1,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },
  { /* Canon BJC 6500 */
    6500,
    17*72, 22*72,
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_CMD61 | CANON_CAP_DMT,
    { -1,0,0,0,-1,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },
  { /* Canon BJC 8500 */
    8500,
    17*72, 22*72,
    150, 1200,1200, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    0,
    { -1,0,0,-1,0,-1},
    { 1,1,1,1,1,1 },
    &simple_4color_inks
  },
};



typedef struct {
  const char name[65];
  int media_code;
  double base_density;
  double k_lower_scale;
  double k_upper;
} paper_t;

static const paper_t canon_paper_list[] = {
  { "Plain Paper",                0x00, 0.50, 0.25, 0.5 },
  { "Transparencies",             0x02, 1.00, 1.00, 0.9 },
  { "Back Print Film",            0x03, 1.00, 1.00, 0.9 },
  { "Fabric Sheets",              0x04, 0.50, 0.25, 0.5 },
  { "Envelope",                   0x08, 0.50, 0.25, 0.5 },
  { "High Resolution Paper",      0x07, 0.78, 0.25, 0.5 },
  { "T-Shirt Transfers",          0x03, 0.50, 0.25, 0.5 },
  { "High Gloss Film",            0x06, 1.00, 1.00, 0.9 },
  { "Glossy Photo Paper",         0x05, 1.00, 1.00, 0.9 },
  { "Glossy Photo Cards",         0x0a, 1.00, 1.00, 0.9 },
  { "Photo Paper Pro",            0x09, 1.00, 1.00, 0.9 },
  /* escp2 paper:
  { "Plain Paper",                0x00, 0.50, 0.25, .5 },
  { "Plain Paper Fast Load",      0x00, 0.50, 0.25, .5 },
  { "Postcard",                   0x00, 0.60, 0.25, .6 },
  { "Glossy Film",                0x00, 1.00, 1.00, .9 }, 
  { "Transparencies",             0x00, 1.00, 1.00, .9 }, 
  { "Envelopes",                  0x00, 0.50, 0.25, .5 }, 
  { "Back Light Film",            0x00, 1.00, 1.00, .9 }, 
  { "Matte Paper",                0x00, 1.00, 1.00, .9 }, 
  { "Inkjet Paper",               0x00, 0.78, 0.25, .6 }, 
  { "Photo Quality Inkjet Paper", 0x00, 1.00, 1.00, .9 }, 
  { "Photo Paper",                0x00, 1.00, 1.00, .9 }, 
  { "Premium Glossy Photo Paper", 0x00, 0.90, 1.00, .9 }, 
  { "Photo Quality Glossy Paper", 0x00, 1.00, 1.00, .9 }, 
  */
  { "Other",                      0x00, 0.50, 0.25, .5 },
};

static const int paper_type_count = sizeof(canon_paper_list) / sizeof(paper_t);


static const paper_t *
get_media_type(const char *name)
{
  int i;
  for (i = 0; i < paper_type_count; i++)
    {
      if (!strcmp(name, canon_paper_list[i].name))
	return &(canon_paper_list[i]);
    }
  return NULL;
}


static canon_cap_t canon_get_model_capabilities(int model)
{
  int i;
  int models= sizeof(canon_model_capabilities) / sizeof(canon_cap_t);
  for (i=0; i<models; i++) {
    if (canon_model_capabilities[i].model == model) {
      return canon_model_capabilities[i];
    }
  }
#ifdef DEBUG
  fprintf(stderr,"canon: model %d not found in capabilities list.\n",model);
#endif
  return canon_model_capabilities[0];
}

static int
canon_source_type(const char *name, canon_cap_t caps)
{
  if (!strcmp(name,"Auto Sheet Feeder"))    return 4;
  if (!strcmp(name,"Manual with Pause"))    return 0;
  if (!strcmp(name,"Manual without Pause")) return 1;

#ifdef DEBUG
  fprintf(stderr,"canon: Unknown source type '%s' - reverting to auto\n",name);
#endif
  return 4;
}

static int
canon_printhead_type(const char *name, canon_cap_t caps)
{
  if (!strcmp(name,"Black"))             return 0;
  if (!strcmp(name,"Color"))             return 1;
  if (!strcmp(name,"Black/Color"))       return 2;
  if (!strcmp(name,"Photo/Color"))       return 3;
  if (!strcmp(name,"Photo"))             return 4;
  if (!strcmp(name,"Black/Photo Color")) return 5;

  if (*name == 0) {
    if (caps.inks & CANON_INK_CMYK) return 2;
    if (caps.inks & CANON_INK_CMY)  return 1;
    if (caps.inks & CANON_INK_K)    return 0;
  }

#ifdef DEBUG
  fprintf(stderr,"canon: Unknown head combo '%s' - reverting to black",name);
#endif
  return 0;
}

static colormode_t 
canon_printhead_colors(const char *name, canon_cap_t caps)
{
  if (!strcmp(name,"Black"))             return COLOR_MONOCHROME;
  if (!strcmp(name,"Color"))             return COLOR_CMY;
  if (!strcmp(name,"Black/Color"))       return COLOR_CMYK;
  if (!strcmp(name,"Photo/Color"))       return COLOR_CCMMYK;
  if (!strcmp(name,"Black/Photo Color")) return COLOR_CCMMYYK;

  if (*name == 0) {
    if (caps.inks & CANON_INK_CMYK) return COLOR_CMYK;
    if (caps.inks & CANON_INK_CMY)  return COLOR_CMY;
    if (caps.inks & CANON_INK_K)    return COLOR_MONOCHROME;
  }

#ifdef DEBUG
  fprintf(stderr,"canon: Unknown head combo '%s' - reverting to black",name);
#endif
  return COLOR_MONOCHROME;
}

static unsigned char
canon_size_type(const vars_t *v, canon_cap_t caps)
{
  const papersize_t *pp = get_papersize_by_size(v->page_height, v->page_width);
  if (pp)
    {
      const char *name = pp->name;
      /* built ins: */
      if (!strcmp(name,"A5"))          return 0x01;
      if (!strcmp(name,"A4"))          return 0x03;
      if (!strcmp(name,"B5"))          return 0x08;
      if (!strcmp(name,"Letter"))      return 0x0d;
      if (!strcmp(name,"Legal"))       return 0x0f;
      if (!strcmp(name,"Envelope 10")) return 0x16;
      if (!strcmp(name,"Envelope DL")) return 0x17;
      if (!strcmp(name,"Letter+"))     return 0x2a;
      if (!strcmp(name,"A4+"))         return 0x2b;
      if (!strcmp(name,"Canon 4x2"))   return 0x2d;
      /* custom */

#ifdef DEBUG
      fprintf(stderr,"canon: Unknown paper size '%s' - using custom\n",name);
    } else {
      fprintf(stderr,"canon: Couldn't look up paper size %dx%d - "
	      "using custom\n",v->page_height, v->page_width);
#endif
    }
  return 0;
}

static char *
c_strdup(const char *s)
{
  char *ret = malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

int canon_res_code(canon_cap_t caps, int xdpi, int ydpi)
{
  int x, y, res= 0;

  for (x=1; x<6; x++) if ((xdpi/caps.base_res) == (1<<(x-1))) res= (x<<4);
  for (y=1; y<6; y++) if ((ydpi/caps.base_res) == (1<<(y-1))) res|= y;

  return res;
}

static int
canon_ink_type(canon_cap_t caps, int res_code)
{
  switch (res_code)
    {
    case 0x11: return caps.dot_sizes.dot_r11;
    case 0x22: return caps.dot_sizes.dot_r22;
    case 0x33: return caps.dot_sizes.dot_r33;
    case 0x43: return caps.dot_sizes.dot_r43;
    case 0x44: return caps.dot_sizes.dot_r44;
    case 0x55: return caps.dot_sizes.dot_r55;
    }
  return -1;
}

static double
canon_density(canon_cap_t caps, int res_code)
{
  switch (res_code)
    {
    case 0x11: return caps.densities.d_r11;
    case 0x22: return caps.densities.d_r22;
    case 0x33: return caps.densities.d_r33;
    case 0x43: return caps.densities.d_r43;
    case 0x44: return caps.densities.d_r44;
    case 0x55: return caps.densities.d_r55;
    default:   
#ifdef DEBUG
      fprintf(stderr,"no such res_code 0x%x in density of model %d\n",
	      res_code,caps.model);
#endif
      return 0.2;
    }
}

static canon_variable_inkset_t *
canon_inks(canon_cap_t caps, int res_code, int colors, int bits)
{
  canon_variable_inklist_t *inks = caps.inxs;

  switch ((bits<<12)+(colors<<8)+res_code)
    {
    case 0x1111:
    case 0x1411: return inks->s_r11_4;
    case 0x1122:
    case 0x1422: return inks->s_r22_4;
    case 0x1133:
    case 0x1433: return inks->s_r33_4;
    case 0x1143:
    case 0x1443: return inks->s_r43_4;
    case 0x1144:
    case 0x1444: return inks->s_r44_4;
    case 0x1155:

    case 0x1455: return inks->s_r55_4;
    case 0x1611: return inks->s_r11_6;
    case 0x1622: return inks->s_r22_6;
    case 0x1633: return inks->s_r33_6;
    case 0x1643: return inks->s_r43_6;
    case 0x1644: return inks->s_r44_6;
    case 0x1655: return inks->s_r55_6;

    case 0x2111:
    case 0x2411: return inks->v_r11_4;
    case 0x2122:
    case 0x2422: return inks->v_r22_4;
    case 0x2133:
    case 0x2433: return inks->v_r33_4;
    case 0x2143:
    case 0x2443: return inks->v_r43_4;
    case 0x2144:
    case 0x2444: return inks->v_r44_4;
    case 0x2155:

    case 0x2455: return inks->v_r55_4;
    case 0x2611: return inks->v_r11_6;
    case 0x2622: return inks->v_r22_6;
    case 0x2633: return inks->v_r33_6;
    case 0x2643: return inks->v_r43_6;
    case 0x2644: return inks->v_r44_6;
    case 0x2655: return inks->v_r55_6;
    }
  return NULL;
}


const char *
canon_default_resolution(const printer_t *printer)
{
  canon_cap_t caps= canon_get_model_capabilities(printer->model);
  if (!(caps.max_xdpi%150))
    return "150x150 DPI";
  else
    return "180x180 DPI";
}

void
canon_describe_resolution(const printer_t *printer,
			const char *resolution, int *x, int *y)
{
  *x = -1;
  *y = -1;
  sscanf(resolution, "%dx%d", x, y);
  return;
}

/*
 * 'canon_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
canon_parameters(const printer_t *printer,	/* I - Printer model */
                 char *ppd_file,	/* I - PPD file (not used) */
                 char *name,		/* I - Name of parameter */
                 int  *count)		/* O - Number of values */
{
  int		i;
  char		**p= 0,
                **valptrs= 0;

  static char   *media_types[] =
                {
                  ("Plain Paper"),
                  ("Transparencies"),
                  ("Back Print Film"),
                  ("Fabric Sheets"),
                  ("Envelope"),
                  ("High Resolution Paper"),
                  ("T-Shirt Transfers"),
                  ("High Gloss Film"),
                  ("Glossy Photo Paper"),
                  ("Glossy Photo Cards"),
                  ("Photo Paper Pro")
                };
  static char   *media_sources[] =
                {
                  ("Auto Sheet Feeder"),
                  ("Manual with Pause"),
                  ("Manual without Pause"),
                };

  canon_cap_t caps= canon_get_model_capabilities(printer->model);

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0) {
    int height_limit, width_limit;
    const papersize_t *papersizes = get_papersizes();
    valptrs = malloc(sizeof(char *) * known_papersizes());
    *count = 0;

    width_limit = caps.max_width;
    height_limit = caps.max_height;

    for (i = 0; i < known_papersizes(); i++) {
      if (strlen(papersizes[i].name) > 0 &&
	  papersizes[i].width <= width_limit &&
	  papersizes[i].height <= height_limit) {
	valptrs[*count] = malloc(strlen(papersizes[i].name) + 1);
	strcpy(valptrs[*count], papersizes[i].name);
	(*count)++;
      }
    }
    return (valptrs);
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    char tmp[100];
    int x,y;
    int c= 0;
    int t;
    valptrs = malloc(sizeof(char *) * 10);

    for (x=1; x<6; x++) {
      for (y=x-1; y<x+1; y++) {
	if ((t= canon_ink_type(caps,(x<<4)|y)) > -1) {
	  snprintf(tmp,99,"%dx%d DPI",
		   (1<<x)/2*caps.base_res,(1<<y)/2*caps.base_res);
#ifdef DEBUG
	  fprintf(stderr,"supports mode '%s'\n",tmp);
#endif
	  valptrs[c++]= c_strdup(tmp);

	  if (t==1) {
	    snprintf(tmp,99,"%dx%d DPI DMT",
		     (1<<x)/2*caps.base_res,(1<<y)/2*caps.base_res);
#ifdef DEBUG
	    fprintf(stderr,"supports mode '%s'\n",tmp);
#endif
	  valptrs[c++]= c_strdup(tmp);
	  }
	}
      }
    }
    *count= c;
    p = valptrs;
  }
  else if (strcmp(name, "InkType") == 0)
  {
    int c= 0;
    valptrs = malloc(sizeof(char *) * 5);
    if ((caps.inks & CANON_INK_K))
      valptrs[c++]= c_strdup("Black");
    if ((caps.inks & CANON_INK_CMY))
      valptrs[c++]= c_strdup("Color");
    if ((caps.inks & CANON_INK_CMYK))
      valptrs[c++]= c_strdup("Black/Color");
    if ((caps.inks & CANON_INK_CcMmYK))
      valptrs[c++]= c_strdup("Photo/Color");
    if ((caps.inks & CANON_INK_CcMmYyK))
      valptrs[c++]= c_strdup("Black/Photo Color");
    *count = c;
    p = valptrs;
  }
  else if (strcmp(name, "MediaType") == 0)
  {
    *count = 11;
    p = media_types;
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    *count = 3;
    p = media_sources;
  }
  else
    return (NULL);

  valptrs = malloc(*count * sizeof(char *));
  for (i = 0; i < *count; i ++)
    valptrs[i] = c_strdup(p[i]);

  return (valptrs);
}


/*
 * 'canon_imageable_area()' - Return the imageable area of the page.
 */

void
canon_imageable_area(const printer_t *printer,	/* I - Printer model */
		     const vars_t *v,   /* I */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */

  canon_cap_t caps= canon_get_model_capabilities(printer->model);

  default_media_size(printer, v, &width, &length);

  *left   = caps.border_left;
  *right  = width - caps.border_right;
  *top    = length - caps.border_top;
  *bottom = caps.border_bottom;
}

void
canon_limit(const printer_t *printer,	/* I - Printer model */
	    const vars_t *v,  		/* I */
	    int  *width,		/* O - Left position in points */
	    int  *length)		/* O - Top position in points */
{
  canon_cap_t caps= canon_get_model_capabilities(printer->model);
  *width =	caps.max_width;
  *length =	caps.max_height;
}

/*
 * 'canon_cmd()' - Sends a command with variable args
 */
static void
canon_cmd(FILE *prn, /* I - the printer         */
	  char *ini, /* I - 2 bytes start code  */
	  char cmd,  /* I - command code        */
	  int  num,  /* I - number of arguments */
	  ...        /* I - the args themselves */
	  )
{
  static int bufsize= 0;
  static unsigned char *buffer;
  int i;
  va_list ap;

  if (!buffer || (num > bufsize)) {
    if (buffer)
      free(buffer);
    buffer = malloc(num);
    bufsize= num;
    if (!buffer) {
#ifdef DEBUG
      fprintf(stderr,"\ncanon: *** buffer allocation failed...\n");
      fprintf(stderr,"canon: *** command 0x%02x with %d args dropped\n\n",
	      cmd,num);
#endif
      return;
    }
  }
  if (num) {
    va_start(ap, num);
    for (i=0; i<num; i++)
      buffer[i]= (unsigned char) va_arg(ap, int);
    va_end(ap);
  }

  fwrite(ini,2,1,prn);
  if (cmd) {
    fputc(cmd,prn);
    fputc((num & 255),prn);
    fputc((num >> 8 ),prn);
    fwrite(buffer,num,1,prn);
  }
}

#ifdef DEBUG
#define PUT(WHAT,VAL,RES) fprintf(stderr,"canon: "WHAT" is %04x =% 5d = %f\" = %f mm\n",(VAL),(VAL),(VAL)/(1.*RES),(VAL)/(RES/25.4))
#else
#define PUT(WHAT,VAL,RES) do {} while (0)
#endif

static void
canon_init_printer(FILE *prn, canon_cap_t caps,
		   int output_type, const paper_t *pt,
		   const vars_t *v, int print_head,
		   const char *source_str,
		   int xdpi, int ydpi,
		   int page_width, int page_height,
		   int top, int left,
		   int use_dmt)
{
  #define ESC28 "\x1b\x28"
  #define ESC5b "\x1b\x5b"
  #define ESC40 "\x1b\x40"

  unsigned char
    arg_63_1 = 0x00,
    arg_63_2 = 0x00, /* plain paper */
    arg_63_3 = caps.max_quality, /* output quality  */
    arg_6c_1 = 0x00,
    arg_6c_2 = 0x00, /* plain paper */
    arg_6d_1 = 0x03, /* color printhead? */
    arg_6d_2 = 0x00, /* 00=color  02=b/w */
    arg_6d_3 = 0x00, /* only 01 for bjc8200 */
    arg_6d_a = 0x03, /* A4 paper */
    arg_6d_b = 0x00,
    arg_70_1 = 0x02, /* A4 printable area */
    arg_70_2 = 0xa6,
    arg_70_3 = 0x01,
    arg_70_4 = 0xe0,
    arg_74_1 = 0x01, /* 1 bit per pixel */
    arg_74_2 = 0x00, /*  */
    arg_74_3 = 0x01; /* 01 <= 360 dpi    09 >= 720 dpi */

  /* int media= canon_media_type(media_str,caps); */
  int source= canon_source_type(source_str,caps);

  int printable_width=  page_width*10/12;
  int printable_length= page_height*10/12;

  arg_6d_a= canon_size_type(v,caps);
  if (!arg_6d_a) arg_6d_b= 1;

  if (caps.model<3000)
    arg_63_1= arg_6c_1= 0x10;
  else
    arg_63_1= arg_6c_1= 0x30;

  if (output_type==OUTPUT_GRAY) arg_63_1|= 0x01;
  arg_6c_1|= (source & 0x0f);

  if (print_head==0) arg_6d_1= 0x03;
  else if (print_head<=2) arg_6d_1= 0x02;
  else if (print_head<=4) arg_6d_1= 0x04;
  if (output_type==OUTPUT_GRAY) arg_6d_2= 0x02;

  if (caps.model==8200) arg_6d_3= 0x01;

  arg_70_1= (printable_length >> 8) & 0xff;
  arg_70_2= (printable_length) & 0xff;
  arg_70_3= (printable_width >> 8) & 0xff;
  arg_70_4= (printable_width) & 0xff;

  if (xdpi==1440) arg_74_2= 0x04;
  if (ydpi>=720)  arg_74_3= 0x09;

  if (pt) arg_63_2= arg_6c_2= pt->media_code;

  if (use_dmt) {
    arg_74_1= 0x02;
    arg_74_2= 0x80;
    arg_74_3= 0x09;
  }

  /* workaround for the bjc8200 - not really understood */
  if (caps.model==8200 && use_dmt) {
    arg_74_1= 0xff;
    arg_74_2= 0x90;
    arg_74_3= 0x04;
  }

  /*
#ifdef DEBUG
  fprintf(stderr,"canon: printable size = %dx%d (%dx%d) %02x%02x %02x%02x\n",
	  page_width,page_height,printable_width,printable_length,
	  arg_70_1,arg_70_2,arg_70_3,arg_70_4);
#endif
  */

  /* init printer */

  canon_cmd(prn,ESC5b,0x4b, 2, 0x00,0x0f);
  if (caps.features & CANON_CAP_CMD61)
    canon_cmd(prn,ESC28,0x61, 1, 0x01);
  canon_cmd(prn,ESC28,0x62, 1, 0x01);
  canon_cmd(prn,ESC28,0x71, 1, 0x01);

  if (caps.features & CANON_CAP_CMD6d)
    canon_cmd(prn,ESC28,0x6d,12, arg_6d_1,
	      0xff,0xff,0x00,0x00,0x07,0x00,
	      arg_6d_a,arg_6d_b,arg_6d_2,0x00,arg_6d_3);

  /* set resolution */

  canon_cmd(prn,ESC28,0x64, 4, (ydpi >> 8 ), (ydpi & 255),
	                        (xdpi >> 8 ), (xdpi & 255));

  canon_cmd(prn,ESC28,0x74, 3, arg_74_1, arg_74_2, arg_74_3);

  canon_cmd(prn,ESC28,0x63, 3, arg_63_1, arg_63_2, arg_63_3);

  if (caps.features & CANON_CAP_CMD70)
    canon_cmd(prn,ESC28,0x70, 8, arg_70_1, arg_70_2, 0x00, 0x00,
	                         arg_70_3, arg_70_4, 0x00, 0x00);

  canon_cmd(prn,ESC28,0x6c, 2, arg_6c_1, arg_6c_2);

  if (caps.features & CANON_CAP_CMD72)
    canon_cmd(prn,ESC28,0x72, 1, 0x61); /* whatever for - 8200 might need it */

  /* some linefeeds */

  top= (top*ydpi)/72;
  PUT("topskip ",top,ydpi);
  canon_cmd(prn,ESC28,0x65, 2, (top >> 8 ),(top & 255));
}

void canon_deinit_printer(FILE *prn, canon_cap_t caps)
{
  /* eject page */
  fputc(0x0c,prn);

  /* say goodbye */
  canon_cmd(prn,ESC28,0x62,1,0);
  if (caps.features & CANON_CAP_CMD61)
    canon_cmd(prn,ESC28,0x61, 1, 0x00);
  canon_cmd(prn,ESC40,0,0);
  fflush(prn);
}


/*
 *  'alloc_buffer()' allocates buffer and fills it with 0
 */
static unsigned char *canon_alloc_buffer(int size)
{
  unsigned char *buf= malloc(size);
  if (buf) memset(buf,0,size);
  return buf;
}

/*
 * 'advance_buffer()' - Move (num) lines of length (len) down one line
 *                      and sets first line to 0s
 *                      accepts NULL pointers as buf
 *                  !!! buf must contain more than (num) lines !!!
 *                      also sets first line to 0s if num<1
 */
static void
canon_advance_buffer(unsigned char *buf, int len, int num)
{
  if (!buf || !len) return;
  if (num>0) memmove(buf+len,buf,len*num);
  memset(buf,0,len);
}

/*
 * 'canon_print()' - Print an image to a CANON printer.
 */
void
canon_print(const printer_t *printer,		/* I - Model */
            FILE      *prn,		/* I - File to print to */
	    Image     image,		/* I - Image to print */
	    const vars_t    *v)
{
  unsigned char *cmap = v->cmap;
  int		model = printer->model;
  const char	*resolution = v->resolution;
  const char	*media_source = v->media_source;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  const char	*ink_type = v->ink_type;
  double 	scaling = v->scaling;
  int		top = v->top;
  int		left = v->left;
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  int		n;		/* Output number */
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned char	*in,		/* Input pixels */
		*black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*yellow,	/* Yellow bitmap data */
		*lcyan,		/* Light cyan bitmap data */
		*lmagenta,	/* Light magenta bitmap data */
		*lyellow;	/* Light yellow bitmap data */
  int           delay_k,
                delay_c,
                delay_m,
                delay_y,
                delay_lc,
                delay_lm,
                delay_ly,
                delay_max;
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Length of page */
		page_true_height,	/* True length of page */
		out_width,	/* Width of image on page */
		out_length,	/* Length of image on page */
		out_bpp,	/* Output bytes per pixel */
		length,		/* Length of raster data */
                buf_length,     /* Length of raster data buffer (dmt) */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  convert_t	colorfunc = 0;	/* Color conversion function... */
  int           bits= 1;
  int           image_height,
                image_width,
                image_bpp;
  int		ink_spread;
  void *	dither;
  vars_t	nv;
  int           res_code;
  int           use_6color= 0;
  double k_upper, k_lower;

  canon_cap_t caps= canon_get_model_capabilities(model);
  int printhead= canon_printhead_type(ink_type,caps);
  colormode_t colormode = canon_printhead_colors(ink_type,caps);
  const paper_t *pt;
  canon_variable_inkset_t *inks;


  memcpy(&nv, v, sizeof(vars_t));
  /*
  PUT("top        ",top,72);
  PUT("left       ",left,72);
  */

  /*
  * Setup a read-only pixel region for the entire image...
  */

  Image_init(image);
  image_height = Image_height(image);
  image_width = Image_width(image);
  image_bpp = Image_bpp(image);

  /* force grayscale if image is grayscale
   *                 or single black cartridge installed
   */

  if (nv.image_type == IMAGE_MONOCHROME)
    {
      output_type = OUTPUT_GRAY;
    }

  if (printhead == 0 || caps.inks == CANON_INK_K)
    output_type = OUTPUT_GRAY;

  if (output_type == OUTPUT_GRAY)
    colormode = COLOR_MONOCHROME;

  /*
   * Choose the correct color conversion function...
   */

  colorfunc = choose_colorfunc(output_type, image_bpp, cmap, &out_bpp, &nv);

 /*
  * Figure out the output resolution...
  */

  sscanf(resolution,"%dx%d",&xdpi,&ydpi);
#ifdef DEBUG
  fprintf(stderr,"canon: resolution=%dx%d\n",xdpi,ydpi);
  fprintf(stderr,"       rescode   =0x%x\n",canon_res_code(caps,xdpi,ydpi));
#endif
  res_code= canon_res_code(caps,xdpi,ydpi);

  if (!strcmp(resolution+(strlen(resolution)-3),"DMT") &&
      (caps.features & CANON_CAP_DMT) &&
      nv.image_type != IMAGE_MONOCHROME) {
    bits= 2;
#ifdef DEBUG
    fprintf(stderr,"canon: using drop modulation technology\n");
#endif
  }

 /*
  * Compute the output size...
  */

  canon_imageable_area(printer, &nv, &page_left, &page_right,
                       &page_bottom, &page_top);

  compute_page_parameters(page_right, page_left, page_top, page_bottom,
			  scaling, image_width, image_height, image,
			  &orientation, &page_width, &page_height,
			  &out_width, &out_length, &left, &top);

  /*
   * Recompute the image length and width.  If the image has been
   * rotated, these will change from previously.
   */
  image_height = Image_height(image);
  image_width = Image_width(image);

  default_media_size(printer, &nv, &n, &page_true_height);

  /*
  PUT("top        ",top,72);
  PUT("left       ",left,72);
  PUT("page_top   ",page_top,72);
  PUT("page_bottom",page_bottom,72);
  PUT("page_left  ",page_left,72);
  PUT("page_right ",page_right,72);
  PUT("page_width ",page_width,72);
  PUT("page_height",page_height,72);
  PUT("page_true_height",page_true_height,72);
  PUT("out_width ", out_width,xdpi);
  PUT("out_length", out_length,ydpi);
  */

  Image_progress_init(image);

  PUT("top     ",top,72);
  PUT("left    ",left,72);

  pt = get_media_type(nv.media_type);

  canon_init_printer(prn, caps, output_type, pt,
		     &nv, printhead, media_source,
		     xdpi, ydpi, page_width, page_height,
		     top,left,(bits==2));

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_length = ydpi * out_length / 72;

  /*
  PUT("out_width ", out_width,xdpi);
  PUT("out_length", out_length,ydpi);
  */

  left = xdpi * left / 72;

  PUT("leftskip",left,xdpi);

  if(xdpi==1440){
    delay_k= 0;
    delay_c= 112;
    delay_m= 224;
    delay_y= 336;
    delay_lc= 112;
    delay_lm= 224;
    delay_ly= 336;
    delay_max= 336;
#ifdef DEBUG
    fprintf(stderr,"canon: delay on!\n");
#endif
  } else {
    delay_k= delay_c= delay_m= delay_y= delay_lc= delay_lm= delay_ly=0;
    delay_max=0;
#ifdef DEBUG
    fprintf(stderr,"canon: delay off!\n");
#endif
  }

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  buf_length= length*bits;

#ifdef DEBUG
  fprintf(stderr,"canon: buflength is %d!\n",buf_length);
#endif

  if (colormode==COLOR_MONOCHROME) {
    black   = canon_alloc_buffer(buf_length*(delay_k+1));
    cyan    = NULL;
    magenta = NULL;
    lcyan   = NULL;
    lmagenta= NULL;
    yellow  = NULL;
    lyellow = NULL;
  } else {
    cyan    = canon_alloc_buffer(buf_length*(delay_c+1));
    magenta = canon_alloc_buffer(buf_length*(delay_m+1));
    yellow  = canon_alloc_buffer(buf_length*(delay_y+1));

    if (colormode!=COLOR_CMY)
      black = canon_alloc_buffer(buf_length*(delay_k+1));
    else
      black = NULL;

    if (colormode==COLOR_CCMMYK || colormode==COLOR_CCMMYYK) {
      use_6color= 1;
      lcyan = canon_alloc_buffer(buf_length*(delay_lc+1));
      lmagenta = canon_alloc_buffer(buf_length*(delay_lm+1));
      if (colormode==CANON_INK_CcMmYyK)
	lyellow = canon_alloc_buffer(buf_length*(delay_lc+1));
      else
	lyellow = NULL;
    } else {
      lcyan = NULL;
      lmagenta = NULL;
      lyellow = NULL;
    }
  }

#ifdef DEBUG
  fprintf(stderr,"canon: driver will use colors ");
  if (cyan)     fputc('C',stderr);
  if (lcyan)    fputc('c',stderr);
  if (magenta)  fputc('M',stderr);
  if (lmagenta) fputc('m',stderr);
  if (yellow)   fputc('Y',stderr);
  if (lyellow)  fputc('y',stderr);
  if (black)    fputc('K',stderr);
  fprintf(stderr,"\n");
#endif

#ifdef DEBUG
  fprintf(stderr,"density is %f\n",nv.density);
#endif

  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */
  if (pt)
    nv.density *= pt->base_density;
  else
    nv.density *= .5;		/* Can't find paper type? Assume plain */
  nv.density *= canon_density(caps, res_code);
  if (nv.density > 1.0)
    nv.density = 1.0;
  if (colormode == COLOR_MONOCHROME)
    nv.gamma /= .8;
  compute_lut(256, &nv);

#ifdef DEBUG
  fprintf(stderr,"density is %f\n",nv.density);
#endif

 /*
  * Output the page...
  */

  if (xdpi > ydpi)
    dither = init_dither(image_width, out_width, 1, xdpi / ydpi, &nv);
  else
    dither = init_dither(image_width, out_width, ydpi / xdpi, 1, &nv);

  dither_set_black_levels(dither, 1.0, 1.0, 1.0);

  if (use_6color)
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
      if (use_6color)
	dither_set_adaptive_divisor(dither, 8);
      else
	dither_set_adaptive_divisor(dither, 16);
    }  
  else
    dither_set_adaptive_divisor(dither, 4);

  if ((inks = canon_inks(caps, res_code, colormode, bits))!=0)
    {
      if (inks->c)
	dither_set_ranges(dither, ECOLOR_C, inks->c->count, inks->c->range,
			  inks->c->density * nv.density);
      if (inks->m)
	dither_set_ranges(dither, ECOLOR_M, inks->m->count, inks->m->range,
			  inks->m->density * nv.density);
      if (inks->y)
	dither_set_ranges(dither, ECOLOR_Y, inks->y->count, inks->y->range,
			  inks->y->density * nv.density);
      if (inks->k)
	dither_set_ranges(dither, ECOLOR_K, inks->k->count, inks->k->range,
			  inks->k->density * nv.density);
    }

  if (bits == 2)
    {
      if (use_6color)
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
      /*
      if (ydpi > canon_max_vres(model))
	ink_spread++;
      */
      if (bits > 1)
	ink_spread++;
      dither_set_ink_spread(dither, ink_spread);
      break;
    }
  dither_set_density(dither, nv.density);

  in  = malloc(image_width * image_bpp);
  out = malloc(image_width * out_bpp * 2);

  errdiv  = image_height / out_length;
  errmod  = image_height % out_length;
  errval  = 0;
  errlast = -1;
  errline  = 0;

  for (y = 0; y < out_length; y ++)
  {
    int duplicate_line = 1;
    if ((y & 63) == 0)
      Image_note_progress(image, y, out_length);

    if (errline != errlast)
    {
      errlast = errline;
      duplicate_line = 0;
      Image_get_row(image, in, errline);
      (*colorfunc)(in, out, image_width, image_bpp, cmap, &nv, NULL, NULL,
		   NULL);
    }

    if (nv.image_type == IMAGE_MONOCHROME)
      dither_monochrome(out, y, dither, black, duplicate_line);
    else if (output_type == OUTPUT_GRAY)
      dither_black(out, y, dither, black, duplicate_line);
    else
      dither_cmyk(out, y, dither, cyan, lcyan, magenta, lmagenta,
		  yellow, 0, black, duplicate_line);

#ifdef DEBUG
    /* fprintf(stderr,","); */
#endif

    canon_write_line(prn, caps, ydpi,
		     black,    delay_k,
		     cyan,     delay_c,
		     magenta,  delay_m,
		     yellow,   delay_y,
		     lcyan,    delay_lc,
		     lmagenta, delay_lm,
		     lyellow,  delay_ly,
		     length, out_width, left, (bits==2));

#ifdef DEBUG
    /* fprintf(stderr,"!"); */
#endif

    canon_advance_buffer(black,   buf_length,delay_k);
    canon_advance_buffer(cyan,    buf_length,delay_c);
    canon_advance_buffer(magenta, buf_length,delay_m);
    canon_advance_buffer(yellow,  buf_length,delay_y);
    canon_advance_buffer(lcyan,   buf_length,delay_lc);
    canon_advance_buffer(lmagenta,buf_length,delay_lm);
    canon_advance_buffer(lyellow, buf_length,delay_ly);

    errval += errmod;
    errline += errdiv;
    if (errval >= out_length)
    {
      errval -= out_length;
      errline ++;
    }
  }
  Image_progress_conclude(image);

  free_dither(dither);

  /*
   * Flush delayed buffers...
   */

  if (delay_max) {
#ifdef DEBUG
    fprintf(stderr,"\ncanon: flushing %d possibly delayed buffers\n",
	    delay_max);
#endif
    for (y= 0; y<delay_max; y++) {

      canon_write_line(prn, caps, ydpi,
		       black,    delay_k,
		       cyan,     delay_c,
		       magenta,  delay_m,
		       yellow,   delay_y,
		       lcyan,    delay_lc,
		       lmagenta, delay_lm,
		       lyellow,  delay_ly,
		       length, out_width, left, (bits==2));

#ifdef DEBUG
      /* fprintf(stderr,"-"); */
#endif

      canon_advance_buffer(black,   buf_length,delay_k);
      canon_advance_buffer(cyan,    buf_length,delay_c);
      canon_advance_buffer(magenta, buf_length,delay_m);
      canon_advance_buffer(yellow,  buf_length,delay_y);
      canon_advance_buffer(lcyan,   buf_length,delay_lc);
      canon_advance_buffer(lmagenta,buf_length,delay_lm);
      canon_advance_buffer(lyellow, buf_length,delay_ly);
    }
  }

 /*
  * Cleanup...
  */

  free_lut(&nv);
  free(in);
  free(out);

  if (black != NULL)    free(black);
  if (cyan != NULL)     free(cyan);
  if (magenta != NULL)  free(magenta);
  if (yellow != NULL)   free(yellow);
  if (lcyan != NULL)    free(lcyan);
  if (lmagenta != NULL) free(lmagenta);
  if (lyellow != NULL)  free(lyellow);

  canon_deinit_printer(prn, caps);
}

/*
 * 'canon_fold_lsb_msb()' fold 2 lines in order lsb/msb
 */

static void
canon_fold_lsb_msb(const unsigned char *line,
		   int single_length,
		   unsigned char *outbuf)
{
  int i;
  for (i = 0; i < single_length; i++) {
    outbuf[0] =
      ((line[0] & (1 << 7)) >> 1) |
      ((line[0] & (1 << 6)) >> 2) |
      ((line[0] & (1 << 5)) >> 3) |
      ((line[0] & (1 << 4)) >> 4) |
      ((line[single_length] & (1 << 7)) >> 0) |
      ((line[single_length] & (1 << 6)) >> 1) |
      ((line[single_length] & (1 << 5)) >> 2) |
      ((line[single_length] & (1 << 4)) >> 3);
    outbuf[1] =
      ((line[0] & (1 << 3)) << 3) |
      ((line[0] & (1 << 2)) << 2) |
      ((line[0] & (1 << 1)) << 1) |
      ((line[0] & (1 << 0)) << 0) |
      ((line[single_length] & (1 << 3)) << 4) |
      ((line[single_length] & (1 << 2)) << 3) |
      ((line[single_length] & (1 << 1)) << 2) |
      ((line[single_length] & (1 << 0)) << 1);
    line++;
    outbuf += 2;
  }
}

/*
 * 'canon_fold_msb_lsb()' fold 2 lines in order msb/lsb
 */

static void
canon_fold_msb_lsb(const unsigned char *line,
		   int single_length,
		   unsigned char *outbuf)
{
  int i;
  for (i = 0; i < single_length; i++) {
    outbuf[0] =
      ((line[0] & (1 << 7)) >> 0) |
      ((line[0] & (1 << 6)) >> 1) |
      ((line[0] & (1 << 5)) >> 2) |
      ((line[0] & (1 << 4)) >> 3) |
      ((line[single_length] & (1 << 7)) >> 1) |
      ((line[single_length] & (1 << 6)) >> 2) |
      ((line[single_length] & (1 << 5)) >> 3) |
      ((line[single_length] & (1 << 4)) >> 4);
    outbuf[1] =
      ((line[0] & (1 << 3)) << 4) |
      ((line[0] & (1 << 2)) << 3) |
      ((line[0] & (1 << 1)) << 2) |
      ((line[0] & (1 << 0)) << 1) |
      ((line[single_length] & (1 << 3)) << 3) |
      ((line[single_length] & (1 << 2)) << 2) |
      ((line[single_length] & (1 << 1)) << 1) |
      ((line[single_length] & (1 << 0)) << 0);
    line++;
    outbuf += 2;
  }
}

/*
 * 'canon_write()' - Send graphics using TIFF packbits compression.
 */

static int
canon_write(FILE          *prn,		/* I - Print file or command */
	    canon_cap_t   caps,	        /* I - Printer model */
	    unsigned char *line,	/* I - Output bitmap data */
	    int           length,	/* I - Length of bitmap data */
	    int           coloridx,	/* I - Which color */
	    int           ydpi,		/* I - Vertical resolution */
	    int           *empty,       /* IO- Preceeding empty lines */
	    int           width,	/* I - Printed width */
	    int           offset, 	/* I - Offset from left side */
	    int           dmt)
{
  unsigned char
    comp_buf[COMPBUFWIDTH],		/* Compression buffer */
    in_fold[COMPBUFWIDTH],
    *in_ptr= line,
    *comp_ptr, *comp_data;
  int newlength;
  unsigned char color;

 /* Don't send blank lines... */

  if (line[0] == 0 && memcmp(line, line + 1, length - 1) == 0)
    return 0;

  /* fold lsb/msb pairs if drop modulation is active */

  if (dmt) {
    if (1) {
      if (caps.features & CANON_CAP_MSB_FIRST)
	canon_fold_msb_lsb(line,length,in_fold);
      else
	canon_fold_lsb_msb(line,length,in_fold);
      in_ptr= in_fold;
    }
    length*= 2;
    offset*= 2;
  }

  /* pack left border rounded to multiples of 8 dots */

  comp_data= comp_buf;
  if (offset) {
    int offset2= (offset+4)/8;
    while (offset2>0) {
      unsigned char toffset = offset2 > 128 ? 128 : offset2;
      comp_data[0] = 1 - toffset;
      comp_data[1] = 0;
      comp_data += 2;
      offset2-= toffset;
    }
  }

  stp_pack(in_ptr, length, comp_data, &comp_ptr);
  newlength= comp_ptr - comp_buf;

  /* send packed empty lines if any */

  if (*empty) {
#ifdef DEBUG
    /* fprintf(stderr,"<%d%c>",*empty,("CMYKcmy"[coloridx])); */
#endif
    fwrite("\x1b\x28\x65\x02\x00", 5, 1, prn);
    fputc((*empty) >> 8 , prn);
    fputc((*empty) & 255, prn);
    *empty= 0;
  }

 /* Send a line of raster graphics... */

  fwrite("\x1b\x28\x41", 3, 1, prn);
  putc((newlength+1) & 255, prn);
  putc((newlength+1) >> 8, prn);
  color= "CMYKcmy"[coloridx];
  if (!color) color= 'K';
  putc(color,prn);
  fwrite(comp_buf, newlength, 1, prn);
  putc('\x0d', prn);
  return 1;
}


static void
canon_write_line(FILE          *prn,	/* I - Print file or command */
		 canon_cap_t   caps,	/* I - Printer model */
		 int           ydpi,	/* I - Vertical resolution */
		 unsigned char *k,	/* I - Output bitmap data */
		 int           dk,	/* I -  */
		 unsigned char *c,	/* I - Output bitmap data */
		 int           dc,	/* I -  */
		 unsigned char *m,	/* I - Output bitmap data */
		 int           dm,	/* I -  */
		 unsigned char *y,	/* I - Output bitmap data */
		 int           dy,	/* I -  */
		 unsigned char *lc,	/* I - Output bitmap data */
		 int           dlc,	/* I -  */
		 unsigned char *lm,	/* I - Output bitmap data */
		 int           dlm,	/* I -  */
		 unsigned char *ly,	/* I - Output bitmap data */
		 int           dly,	/* I -  */
		 int           l,	/* I - Length of bitmap data */
		 int           width,	/* I - Printed width */
		 int           offset,  /* I - horizontal offset */
		 int           dmt)
{
  static int empty= 0;
  int written= 0;

  if (k) written+=
    canon_write(prn, caps, k+ dk*l,  l, 3, ydpi, &empty, width, offset, dmt);
  if (y) written+=
    canon_write(prn, caps, y+ dy*l,  l, 2, ydpi, &empty, width, offset, dmt);
  if (m) written+=
    canon_write(prn, caps, m+ dm*l,  l, 1, ydpi, &empty, width, offset, dmt);
  if (c) written+=
    canon_write(prn, caps, c+ dc*l,  l, 0, ydpi, &empty, width, offset, dmt);
  if (ly) written+=
    canon_write(prn, caps, ly+dly*l, l, 6, ydpi, &empty, width, offset, dmt);
  if (lm) written+=
    canon_write(prn, caps, lm+dlm*l, l, 5, ydpi, &empty, width, offset, dmt);
  if (lc) written+=
    canon_write(prn, caps, lc+dlc*l, l, 4, ydpi, &empty, width, offset, dmt);

  if (written)
    fwrite("\x1b\x28\x65\x02\x00\x00\x01", 7, 1, prn);
  else
    empty++;
}
