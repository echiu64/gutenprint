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
 * Definition of the multi-level inks available to a given printer.
 * Each printer may use a different kind of ink droplet for variable
 * and single drop size for each supported horizontal resolution and
 * type of ink (4 or 6 color).
 *
 * Recall that 6 color ink is treated as simply another kind of
 * multi-level ink, but the driver offers the user a choice of 4 and
 * 6 color ink, so we need to define appropriate inksets for both
 * kinds of ink.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include "print-escp2.h"

#define DECLARE_INK2(name, density)					\
static const escp2_variable_ink_t name##_ink =				\
{									\
  name##_dither_ranges,							\
  sizeof(name##_dither_ranges) / sizeof(stpi_dither_range_simple_t),	\
  density,								\
  name##_shades,							\
  sizeof(name##_shades) / sizeof(stpi_shade_t)				\
}

#define SHADE(density, subchannel, name)				\
{  density, subchannel,							\
   sizeof(name)/sizeof(stpi_dotsize_t), name  }


#define PIEZO_0  .25
#define PIEZO_1  .5
#define PIEZO_2  .75
#define PIEZO_3 1.0

#define PIEZO_DENSITY 1.0

/***************************************************************\
*                                                               *
*                        SINGLE DOT SIZE                        *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t single_dotsize[] =
{
  { 0x1, 1.0 }
};

static const stpi_dither_range_simple_t standard_dither_ranges[] =
{
  { 1.0,  0x1, 0, 1 }
};

static const stpi_shade_t standard_shades[] =
{
  SHADE(1.0,  0, single_dotsize)
};

DECLARE_INK2(standard, 1.0);

static const stpi_dither_range_simple_t photo_cyan_dither_ranges[] =
{
  { 0.27, 0x1, 1, 1 },
  { 1.0,  0x1, 0, 1 }
};

static const stpi_shade_t photo_cyan_shades[] =
{
  SHADE(0.27, 1, single_dotsize),
  SHADE(1.0,  0, single_dotsize)
};

DECLARE_INK2(photo_cyan, 1.0);

static const stpi_dither_range_simple_t photo_magenta_dither_ranges[] =
{
  { 0.35, 0x1, 1, 1 },
  { 1.0,  0x1, 0, 1 }
};

static const stpi_shade_t photo_magenta_shades[] =
{
  SHADE(0.35, 1, single_dotsize),
  SHADE(1.0,  0, single_dotsize)
};

DECLARE_INK2(photo_magenta, 1.0);

static const stpi_dither_range_simple_t photo2_yellow_dither_ranges[] =
{
  { 0.35, 0x1, 1, 1 },
  { 1.0,  0x1, 0, 1 }
};

static const stpi_shade_t photo2_yellow_shades[] =
{
  SHADE(0.35, 1, single_dotsize),
  SHADE(1.0,  0, single_dotsize)
};

DECLARE_INK2(photo2_yellow, 1.0);

static const stpi_dither_range_simple_t photo2_black_dither_ranges[] =
{
  { 0.27, 0x1, 1, 1 },
  { 1.0,  0x1, 0, 1 }
};

static const stpi_shade_t photo2_black_shades[] =
{
  SHADE(0.27, 1, single_dotsize),
  SHADE(1.0,  0, single_dotsize)
};

DECLARE_INK2(photo2_black, 1.0);

static const stpi_dither_range_simple_t piezo_quadtone_dither_ranges[] =
{
  { PIEZO_0, 0x1, 0, 1 },
  { PIEZO_1, 0x1, 1, 1 },
  { PIEZO_2, 0x1, 2, 1 },
  { PIEZO_3, 0x1, 3, 1 },
};

static const stpi_shade_t piezo_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, single_dotsize),
  SHADE(PIEZO_1, 1, single_dotsize),
  SHADE(PIEZO_2, 2, single_dotsize),
  SHADE(PIEZO_3, 3, single_dotsize)
};
  

DECLARE_INK2(piezo_quadtone, PIEZO_DENSITY);

/***************************************************************\
*                                                               *
*       LOW RESOLUTION, 4 AND 6 PICOLITRE PRINTERS              *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t standard_multishot_dotsizes[] =
{
  { 0x1, 0.28 },
  { 0x2, 0.58 },
  { 0x3, 1.0  }
};

static const stpi_dither_range_simple_t standard_multishot_dither_ranges[] =
{
  { 0.28,  0x1, 0, 2 },
  { 0.58,  0x2, 0, 4 },
  { 1.0,   0x3, 0, 7 }
};

static const stpi_shade_t standard_multishot_shades[] =
{
  SHADE(1.0, 0, standard_multishot_dotsizes)
};

DECLARE_INK2(standard_multishot, 1.0);

static const stpi_dither_range_simple_t photo_multishot_dither_ranges[] =
{
  { 0.0728, 0x1, 1, 1 },
  { 0.151,  0x2, 1, 2 },
  { 0.26,   0x3, 1, 3 },
  { 1.0,    0x3, 0, 3 }
};

static const stpi_shade_t photo_multishot_shades[] =
{
  SHADE(0.26, 1, standard_multishot_dotsizes),
  SHADE(1.0,  0, standard_multishot_dotsizes)
};

DECLARE_INK2(photo_multishot, 1.0);

static const stpi_dither_range_simple_t photo_multishot_y_dither_ranges[] =
{
  { 0.140, 0x1, 0, 1 },
  { 0.290, 0x2, 0, 2 },
  { 0.5,   0x3, 0, 3 },
  { 1.0,   0x3, 1, 3 }
};

static const stpi_shade_t photo_multishot_y_shades[] =
{
  SHADE(0.5, 0, standard_multishot_dotsizes),
  SHADE(1.0, 1, standard_multishot_dotsizes)
};

DECLARE_INK2(photo_multishot_y, 1.0);

static const stpi_dither_range_simple_t piezo_multishot_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .28, 0x1, 0, 2 },
  { PIEZO_0 * .58, 0x2, 0, 4 },
  { PIEZO_1 * .58, 0x2, 1, 4 },
  { PIEZO_2 * .58, 0x2, 2, 4 },
  { PIEZO_2 * 1.0, 0x3, 2, 7 },
  { PIEZO_3 * 1.0, 0x3, 3, 7 },
};

static const stpi_shade_t piezo_multishot_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_multishot_dotsizes),
  SHADE(PIEZO_1, 1, standard_multishot_dotsizes),
  SHADE(PIEZO_2, 2, standard_multishot_dotsizes),
  SHADE(PIEZO_3, 3, standard_multishot_dotsizes)
};

DECLARE_INK2(piezo_multishot_quadtone, PIEZO_DENSITY);

/***************************************************************\
*                                                               *
*       4 AND 6 PICOLITRE PRINTERS, 6 PICOLITRE DOTS            *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t standard_6pl_dotsizes[] =
{
  { 0x1, 0.25},
  { 0x2, 0.5 },
  { 0x3, 1.0 }
};

static const stpi_dotsize_t standard_6pl_1440_dotsizes[] =
{
  { 0x1, 0.5},
  { 0x2, 1.0}
};

static const stpi_dotsize_t standard_6pl_2880_dotsizes[] =
{
  { 0x1, 1.0}
};

static const stpi_dither_range_simple_t standard_6pl_dither_ranges[] =
{
  { 0.25,  0x1, 0, 1 },
  { 0.5,   0x2, 0, 2 },
  { 1.0,   0x3, 0, 4 }
};

static const stpi_shade_t standard_6pl_shades[] =
{
  SHADE(1.0, 0, standard_6pl_dotsizes)
};

DECLARE_INK2(standard_6pl, 1.0);

static const stpi_dither_range_simple_t standard_6pl_1440_dither_ranges[] =
{
  { 0.5,   0x1, 0, 1 },
  { 1.0,   0x2, 0, 2 },
};

static const stpi_shade_t standard_6pl_1440_shades[] =
{
  SHADE(1.0, 0, standard_6pl_1440_dotsizes)
};

DECLARE_INK2(standard_6pl_1440, 1.0);

static const stpi_dither_range_simple_t standard_6pl_2880_dither_ranges[] =
{
  { 1.0,   0x1, 0, 1 },
};

static const stpi_shade_t standard_6pl_2880_shades[] =
{
  SHADE(1.0, 0, standard_6pl_2880_dotsizes)
};

DECLARE_INK2(standard_6pl_2880, 1.0);

static const stpi_dither_range_simple_t photo_6pl_dither_ranges[] =
{
  { 0.065, 0x1, 1, 1 },
  { 0.13,  0x2, 1, 2 },
/* { 0.26, 0x3, 1, 4 }, */
  { 0.25,  0x1, 0, 1 },
  { 0.5,   0x2, 0, 2 },
  { 1.0,   0x3, 0, 4 }
};

static const stpi_shade_t photo_6pl_shades[] =
{
  SHADE(0.26, 1, standard_6pl_dotsizes),
  SHADE(1.0,  0, standard_6pl_dotsizes)
};

DECLARE_INK2(photo_6pl, 1.0);

static const stpi_dither_range_simple_t photo_6pl_y_dither_ranges[] =
{
  { 0.125, 0x1, 0, 1 },
  { 0.25,  0x2, 0, 2 },
  { 0.5,   0x2, 1, 2 },
  { 1.0,   0x3, 1, 4 }
};

static const stpi_shade_t photo_6pl_y_shades[] =
{
  SHADE(0.25, 0, standard_6pl_dotsizes),
  SHADE(1.0,  1, standard_6pl_dotsizes)
};

DECLARE_INK2(photo_6pl_y, 1.0);

static const stpi_dither_range_simple_t photo_6pl_1440_dither_ranges[] =
{
  { 0.13,  0x1, 1, 1 },
  { 0.26,  0x2, 1, 2 },
/* { 0.52, 0x3, 1, 4 }, */
  { 0.5,   0x1, 0, 1 },
  { 1.0,   0x2, 0, 2 },
};

static const stpi_shade_t photo_6pl_1440_shades[] =
{
  SHADE(0.26, 1, standard_6pl_1440_dotsizes),
  SHADE(1.0,  0, standard_6pl_1440_dotsizes)
};

DECLARE_INK2(photo_6pl_1440, 1.0);

static const stpi_dither_range_simple_t photo_6pl_2880_dither_ranges[] =
{
  { 0.26,  0x1, 1, 1 },
  { 1.0,   0x1, 0, 1 },
};

static const stpi_shade_t photo_6pl_2880_shades[] =
{
  SHADE(0.26, 1, standard_6pl_2880_dotsizes),
  SHADE(1.0,  0, standard_6pl_2880_dotsizes)
};

DECLARE_INK2(photo_6pl_2880, 1.0);

static const stpi_dither_range_simple_t piezo_6pl_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .25, 0x1, 0, 1 },
  { PIEZO_0 * .50, 0x2, 0, 2 },
  { PIEZO_1 * .50, 0x2, 1, 2 },
  { PIEZO_2 * .50, 0x2, 2, 2 },
  { PIEZO_2 * 1.0, 0x3, 2, 4 },
  { PIEZO_3 * 1.0, 0x3, 3, 4 },
};

static const stpi_shade_t piezo_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_6pl_dotsizes),
  SHADE(PIEZO_1, 1, standard_6pl_dotsizes),
  SHADE(PIEZO_2, 2, standard_6pl_dotsizes),
  SHADE(PIEZO_3, 3, standard_6pl_dotsizes)
};

DECLARE_INK2(piezo_6pl_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_6pl_1440_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .50, 0x1, 0, 1 },
  { PIEZO_1 * .50, 0x1, 1, 1 },
  { PIEZO_2 * .50, 0x1, 2, 1 },
  { PIEZO_2 * 1.0, 0x2, 2, 2 },
  { PIEZO_3 * 1.0, 0x2, 3, 2 }
};

static const stpi_shade_t piezo_6pl_1440_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_6pl_1440_dotsizes),
  SHADE(PIEZO_1, 1, standard_6pl_1440_dotsizes),
  SHADE(PIEZO_2, 2, standard_6pl_1440_dotsizes),
  SHADE(PIEZO_3, 3, standard_6pl_1440_dotsizes)
};

DECLARE_INK2(piezo_6pl_1440_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_6pl_2880_quadtone_dither_ranges[] =
{
  { PIEZO_0 * 1.0, 0x1, 0, 1 },
  { PIEZO_1 * 1.0, 0x1, 1, 1 },
  { PIEZO_2 * 1.0, 0x1, 2, 1 },
  { PIEZO_3 * 1.0, 0x1, 3, 1 },
};

static const stpi_shade_t piezo_6pl_2880_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_6pl_2880_dotsizes),
  SHADE(PIEZO_1, 1, standard_6pl_2880_dotsizes),
  SHADE(PIEZO_2, 2, standard_6pl_2880_dotsizes),
  SHADE(PIEZO_3, 3, standard_6pl_2880_dotsizes)
};

DECLARE_INK2(piezo_6pl_2880_quadtone, PIEZO_DENSITY);


/***************************************************************\
*                                                               *
*                      STYLUS COLOR 480/580                     *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t standard_x80_multishot_dotsizes[] =
{
  { 0x1, 0.163},
  { 0x2, 0.5  },
  { 0x3, 1.0  }
};

static const stpi_dotsize_t standard_x80_6pl_dotsizes[] =
{
  { 0x1, 0.325},
  { 0x2, 0.5  },
  { 0x3, 1.0  }
};

static const stpi_dotsize_t standard_x80_1440_6pl_dotsizes[] =
{
  { 0x1, 0.65 },
  { 0x2, 1.0  }
};

static const stpi_dotsize_t standard_x80_2880_6pl_dotsizes[] =
{
  { 0x1, 1.0 }
};

static const stpi_dither_range_simple_t standard_x80_multishot_dither_ranges[] =
{
  { 0.163, 0x1, 0, 1 },
  { 0.5,   0x2, 0, 3 },
  { 1.0,   0x3, 0, 6 }
};

static const stpi_shade_t standard_x80_multishot_shades[] =
{
  SHADE(1.0, 0, standard_x80_multishot_dotsizes)
};

DECLARE_INK2(standard_x80_multishot, 1.0);

static const stpi_dither_range_simple_t standard_x80_6pl_dither_ranges[] =
{
  { 0.325, 0x1, 0, 2 },
  { 0.5,   0x2, 0, 3 },
  { 1.0,   0x3, 0, 6 }
};

static const stpi_shade_t standard_x80_6pl_shades[] =
{
  SHADE(1.0, 0, standard_x80_6pl_dotsizes)
};

DECLARE_INK2(standard_x80_6pl, 1.0);

static const stpi_dither_range_simple_t standard_x80_1440_6pl_dither_ranges[] =
{
  { 0.65,  0x1, 0, 2 },
  { 1.0,   0x2, 0, 3 },
};

static const stpi_shade_t standard_x80_1440_6pl_shades[] =
{
  SHADE(1.0, 0, standard_x80_1440_6pl_dotsizes)
};

DECLARE_INK2(standard_x80_1440_6pl, 1.0);

static const stpi_dither_range_simple_t standard_x80_2880_6pl_dither_ranges[] =
{
  { 1.00,  0x1, 0, 1 },
};

static const stpi_shade_t standard_x80_2880_6pl_shades[] =
{
  SHADE(1.0, 0, standard_x80_2880_6pl_dotsizes)
};

DECLARE_INK2(standard_x80_2880_6pl, 1.0);

static const stpi_dither_range_simple_t piezo_x80_multishot_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .163, 0x1, 0, 1 },
  { PIEZO_0 * .500, 0x2, 0, 3 },
  { PIEZO_1 * .500, 0x2, 1, 3 },
  { PIEZO_2 * .500, 0x2, 2, 3 },
  { PIEZO_2 * 1.00, 0x3, 2, 6 },
  { PIEZO_3 * 1.00, 0x3, 3, 6 },
};

static const stpi_shade_t piezo_x80_multishot_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_x80_multishot_dotsizes),
  SHADE(PIEZO_1, 1, standard_x80_multishot_dotsizes),
  SHADE(PIEZO_2, 2, standard_x80_multishot_dotsizes),
  SHADE(PIEZO_3, 3, standard_x80_multishot_dotsizes)
};

DECLARE_INK2(piezo_x80_multishot_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_x80_6pl_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .325, 0x1, 0, 2 },
  { PIEZO_0 * .500, 0x2, 0, 3 },
  { PIEZO_1 * .500, 0x2, 1, 3 },
  { PIEZO_2 * .500, 0x2, 2, 3 },
  { PIEZO_2 * 1.00, 0x3, 2, 6 },
  { PIEZO_3 * 1.00, 0x3, 3, 6 },
};

static const stpi_shade_t piezo_x80_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_x80_6pl_dotsizes),
  SHADE(PIEZO_1, 1, standard_x80_6pl_dotsizes),
  SHADE(PIEZO_2, 2, standard_x80_6pl_dotsizes),
  SHADE(PIEZO_3, 3, standard_x80_6pl_dotsizes)
};

DECLARE_INK2(piezo_x80_6pl_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_x80_1440_6pl_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .650, 0x1, 0, 2 },
  { PIEZO_1 * .650, 0x1, 1, 2 },
  { PIEZO_2 * .650, 0x1, 2, 2 },
  { PIEZO_2 * 1.00, 0x2, 2, 3 },
  { PIEZO_3 * 1.00, 0x2, 3, 3 },
};

static const stpi_shade_t piezo_x80_1440_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_x80_1440_6pl_dotsizes),
  SHADE(PIEZO_1, 1, standard_x80_1440_6pl_dotsizes),
  SHADE(PIEZO_2, 2, standard_x80_1440_6pl_dotsizes),
  SHADE(PIEZO_3, 3, standard_x80_1440_6pl_dotsizes)
};

DECLARE_INK2(piezo_x80_1440_6pl_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_x80_2880_6pl_quadtone_dither_ranges[] =
{
  { PIEZO_0 * 1.00, 0x1, 0, 1 },
  { PIEZO_1 * 1.00, 0x1, 1, 1 },
  { PIEZO_2 * 1.00, 0x1, 2, 1 },
  { PIEZO_3 * 1.00, 0x1, 3, 1 },
};

static const stpi_shade_t piezo_x80_2880_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_x80_2880_6pl_dotsizes),
  SHADE(PIEZO_1, 1, standard_x80_2880_6pl_dotsizes),
  SHADE(PIEZO_2, 2, standard_x80_2880_6pl_dotsizes),
  SHADE(PIEZO_3, 3, standard_x80_2880_6pl_dotsizes)
};

DECLARE_INK2(piezo_x80_2880_6pl_quadtone, PIEZO_DENSITY);


/***************************************************************\
*                                                               *
*                      STYLUS COLOR 680/777
*                                                               *
\***************************************************************/

static const stpi_dotsize_t standard_680_multishot_dotsizes[] =
{
  { 0x1, 0.375 },
  { 0x2, 0.75  },
  { 0x3, 1.0   }
};

static const stpi_dotsize_t standard_680_6pl_dotsizes[] =
{
  { 0x1, 0.50  },
  { 0x2, 0.66  },
  { 0x3, 1.0   }
};

static const stpi_dither_range_simple_t standard_680_multishot_dither_ranges[] =
{
  { 0.375, 0x1, 0, 3 },
  { 0.75,  0x2, 0, 6 },
  { 1.0,   0x3, 0, 8 }
};

static const stpi_shade_t standard_680_multishot_shades[] =
{
  SHADE(1.0, 0, standard_680_multishot_dotsizes)
};

DECLARE_INK2(standard_680_multishot, 1.0);

static const stpi_dither_range_simple_t standard_680_6pl_dither_ranges[] =
{
  { 0.50,  0x1, 0, 3 },
  { 0.66,  0x2, 0, 4 },
  { 1.0,   0x3, 0, 6 }
};

static const stpi_shade_t standard_680_6pl_shades[] =
{
  SHADE(1.0, 0, standard_680_6pl_dotsizes)
};

DECLARE_INK2(standard_680_6pl, 1.0);

static const stpi_dither_range_simple_t piezo_680_multishot_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .375, 0x1, 0, 3 },
  { PIEZO_0 * .750, 0x2, 0, 6 },
  { PIEZO_1 * .750, 0x2, 1, 6 },
  { PIEZO_2 * .750, 0x2, 2, 6 },
  { PIEZO_2 * 1.00, 0x3, 2, 8 },
  { PIEZO_3 * 1.00, 0x3, 3, 8 },
};

static const stpi_shade_t piezo_680_multishot_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_680_multishot_dotsizes),
  SHADE(PIEZO_1, 1, standard_680_multishot_dotsizes),
  SHADE(PIEZO_2, 2, standard_680_multishot_dotsizes),
  SHADE(PIEZO_3, 3, standard_680_multishot_dotsizes)
};

DECLARE_INK2(piezo_680_multishot_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_680_6pl_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .500, 0x1, 0, 3 },
  { PIEZO_1 * .660, 0x2, 1, 4 },
  { PIEZO_2 * .660, 0x2, 2, 4 },
  { PIEZO_2 * 1.00, 0x2, 3, 6 },
  { PIEZO_3 * 1.00, 0x3, 3, 6 },
};

static const stpi_shade_t piezo_680_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_680_6pl_dotsizes),
  SHADE(PIEZO_1, 1, standard_680_6pl_dotsizes),
  SHADE(PIEZO_2, 2, standard_680_6pl_dotsizes),
  SHADE(PIEZO_3, 3, standard_680_6pl_dotsizes)
};

DECLARE_INK2(piezo_680_6pl_quadtone, PIEZO_DENSITY);


/***************************************************************\
*                                                               *
*                   4 PICOLITRE DOTS                            *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t standard_4pl_dotsizes[] =
{
  { 0x1, 0.661},
  { 0x2, 1.0  }
};

static const stpi_dotsize_t standard_4pl_2880_dotsizes[] =
{
  { 0x1, 1.0 }
};

static const stpi_dither_range_simple_t standard_4pl_dither_ranges[] =
{
  { 0.661, 0x1, 0, 2 },
  { 1.00,  0x2, 0, 3 }
};

static const stpi_shade_t standard_4pl_shades[] =
{
  SHADE(1.0, 0, standard_4pl_dotsizes)
};

DECLARE_INK2(standard_4pl, 1.0);

static const stpi_dither_range_simple_t standard_4pl_2880_dither_ranges[] =
{
  { 1.00,  0x1, 0, 1 },
};

static const stpi_shade_t standard_4pl_2880_shades[] =
{
  SHADE(1.0, 0, standard_4pl_2880_dotsizes)
};

DECLARE_INK2(standard_4pl_2880, 1.0);

static const stpi_dither_range_simple_t photo_4pl_dither_ranges[] =
{
  { 0.17,  0x1, 1, 2 },
  { 0.26,  0x2, 1, 3 },
  { 0.661, 0x1, 0, 2 },
  { 1.00,  0x2, 0, 3 }
};

static const stpi_shade_t photo_4pl_shades[] =
{
  SHADE(0.26, 1, standard_4pl_dotsizes),
  SHADE(1.0,  0, standard_4pl_dotsizes)
};

DECLARE_INK2(photo_4pl, 1.0);

static const stpi_dither_range_simple_t photo_4pl_y_dither_ranges[] =
{
  { 0.330, 0x1, 0, 2 },
  { 0.50,  0x2, 0, 3 },
  { 0.661, 0x1, 1, 2 },
  { 1.00,  0x2, 1, 3 }
};

static const stpi_shade_t photo_4pl_y_shades[] =
{
  SHADE(0.50, 0, standard_4pl_dotsizes),
  SHADE(1.00, 1, standard_4pl_dotsizes)
};

DECLARE_INK2(photo_4pl_y, 1.0);

static const stpi_dither_range_simple_t photo_4pl_2880_dither_ranges[] =
{
  { 0.26,  0x1, 1, 1 },
  { 1.00,  0x1, 0, 1 },
};

static const stpi_shade_t photo_4pl_2880_shades[] =
{
  SHADE(0.26, 1, standard_4pl_2880_dotsizes),
  SHADE(1.00, 0, standard_4pl_2880_dotsizes)
};

DECLARE_INK2(photo_4pl_2880, 1.0);

static const stpi_dither_range_simple_t photo_4pl_y_2880_dither_ranges[] =
{
  { 0.5,   0x1, 1, 1 },
  { 1.00,  0x1, 0, 1 },
};

static const stpi_shade_t photo_4pl_y_2880_shades[] =
{
  SHADE(0.5, 1, standard_4pl_2880_dotsizes),
  SHADE(1.0, 0, standard_4pl_2880_dotsizes)
};

DECLARE_INK2(photo_4pl_y_2880, 1.0);

static const stpi_dither_range_simple_t piezo_4pl_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .661, 0x1, 0, 2 },
  { PIEZO_1 * .661, 0x1, 1, 2 },
  { PIEZO_2 * .661, 0x1, 2, 2 },
  { PIEZO_2 * 1.00, 0x2, 2, 3 },
  { PIEZO_3 * 1.00, 0x2, 3, 3 },
};

static const stpi_shade_t piezo_4pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_4pl_dotsizes),
  SHADE(PIEZO_1, 1, standard_4pl_dotsizes),
  SHADE(PIEZO_2, 2, standard_4pl_dotsizes),
  SHADE(PIEZO_3, 3, standard_4pl_dotsizes)
};

DECLARE_INK2(piezo_4pl_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_4pl_2880_quadtone_dither_ranges[] =
{
  { PIEZO_0, 0x1, 0, 1 },
  { PIEZO_1, 0x1, 1, 1 },
  { PIEZO_2, 0x1, 2, 1 },
  { PIEZO_3, 0x1, 3, 1 },
};

static const stpi_shade_t piezo_4pl_2880_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_4pl_2880_dotsizes),
  SHADE(PIEZO_1, 1, standard_4pl_2880_dotsizes),
  SHADE(PIEZO_2, 2, standard_4pl_2880_dotsizes),
  SHADE(PIEZO_3, 3, standard_4pl_2880_dotsizes)
};

DECLARE_INK2(piezo_4pl_2880_quadtone, PIEZO_DENSITY);


/***************************************************************\
*                                                               *
*                 3 PICOLITRE DOTS (900 AND 980)                *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t standard_3pl_dotsizes[] =
{
  { 0x1, 0.25 },
  { 0x2, 0.61 },
  { 0x3, 1.0  }
};

static const stpi_dotsize_t standard_3pl_1440_dotsizes[] =
{
  { 0x1, 0.39 },
  { 0x2, 1.0  }
};

static const stpi_dotsize_t standard_3pl_2880_dotsizes[] =
{
  { 0x1, 1.0 }
};

static const stpi_dotsize_t standard_980_6pl_dotsizes[] =
{
  { 0x1, 0.40  },
  { 0x2, 0.675 },
  { 0x3, 1.0   }
};

static const stpi_dither_range_simple_t standard_3pl_dither_ranges[] =
{
  { 0.25,  0x1, 0, 2 },
  { 0.61,  0x2, 0, 5 },
  { 1.0,   0x3, 0, 8 }
};

static const stpi_shade_t standard_3pl_shades[] =
{
  SHADE(1.0, 0, standard_3pl_dotsizes)
};

DECLARE_INK2(standard_3pl, 1.0);


static const stpi_dither_range_simple_t standard_3pl_1440_dither_ranges[] =
{
  { 0.39, 0x1, 0, 2 },
  { 1.0,  0x2, 0, 5 }
};

static const stpi_shade_t standard_3pl_1440_shades[] =
{
  SHADE(1.0, 0, standard_3pl_1440_dotsizes)
};

DECLARE_INK2(standard_3pl_1440, 1.0);


static const stpi_dither_range_simple_t standard_3pl_2880_dither_ranges[] =
{
  { 1.0,   0x1, 0, 1 }
};

static const stpi_shade_t standard_3pl_2880_shades[] =
{
  SHADE(1.0, 0, standard_3pl_2880_dotsizes)
};

DECLARE_INK2(standard_3pl_2880, 1.0);

static const stpi_dither_range_simple_t standard_980_6pl_dither_ranges[] =
{
  { 0.40,  0x1, 0, 4 },
  { 0.675, 0x2, 0, 7 },
  { 1.0,   0x3, 0, 10 }
};

static const stpi_shade_t standard_980_6pl_shades[] =
{
  SHADE(1.0, 0, standard_980_6pl_dotsizes)
};

DECLARE_INK2(standard_980_6pl, 1.0);

static const stpi_dither_range_simple_t piezo_3pl_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .25, 0x1, 0, 2 },
  { PIEZO_0 * .61, 0x2, 0, 5 },
  { PIEZO_1 * .61, 0x2, 1, 5 },
  { PIEZO_2 * .61, 0x2, 2, 5 },
  { PIEZO_2 * 1.0, 0x3, 2, 8 },
  { PIEZO_3 * 1.0, 0x3, 3, 8 },
};

static const stpi_shade_t piezo_3pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_3pl_dotsizes),
  SHADE(PIEZO_1, 1, standard_3pl_dotsizes),
  SHADE(PIEZO_2, 2, standard_3pl_dotsizes),
  SHADE(PIEZO_3, 3, standard_3pl_dotsizes)
};

DECLARE_INK2(piezo_3pl_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_3pl_1440_quadtone_dither_ranges[]=
{
  { PIEZO_0 * .390, 0x1, 0, 2 },
  { PIEZO_1 * .390, 0x1, 1, 2 },
  { PIEZO_2 * .390, 0x1, 2, 2 },
  { PIEZO_2 * 1.00, 0x2, 2, 5 },
  { PIEZO_3 * 1.00, 0x2, 3, 5 },
};

static const stpi_shade_t piezo_3pl_1440_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_3pl_1440_dotsizes),
  SHADE(PIEZO_1, 1, standard_3pl_1440_dotsizes),
  SHADE(PIEZO_2, 2, standard_3pl_1440_dotsizes),
  SHADE(PIEZO_3, 3, standard_3pl_1440_dotsizes)
};

DECLARE_INK2(piezo_3pl_1440_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_3pl_2880_quadtone_dither_ranges[]=
{
  { PIEZO_0, 0x1, 0, 1 },
  { PIEZO_1, 0x1, 1, 1 },
  { PIEZO_2, 0x1, 2, 1 },
  { PIEZO_3, 0x1, 3, 1 },
};

static const stpi_shade_t piezo_3pl_2880_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_3pl_2880_dotsizes),
  SHADE(PIEZO_1, 1, standard_3pl_2880_dotsizes),
  SHADE(PIEZO_2, 2, standard_3pl_2880_dotsizes),
  SHADE(PIEZO_3, 3, standard_3pl_2880_dotsizes)
};

DECLARE_INK2(piezo_3pl_2880_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_980_6pl_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .400, 0x1, 0, 4 },
  { PIEZO_0 * .675, 0x2, 0, 7 },
  { PIEZO_1 * .675, 0x2, 1, 7 },
  { PIEZO_2 * .675, 0x2, 2, 7 },
  { PIEZO_3 * 1.00, 0x3, 3, 10 },
};

static const stpi_shade_t piezo_980_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_980_6pl_dotsizes),
  SHADE(PIEZO_1, 1, standard_980_6pl_dotsizes),
  SHADE(PIEZO_2, 2, standard_980_6pl_dotsizes),
  SHADE(PIEZO_3, 3, standard_980_6pl_dotsizes)
};

DECLARE_INK2(piezo_980_6pl_quadtone, PIEZO_DENSITY);


/***************************************************************\
*                                                               *
*                  2 PICOLITRE DOTS (950)                       *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t standard_2pl_2880_dotsizes[] =
{
  { 0x1, 1.00 }
};

static const stpi_dotsize_t standard_2pl_1440_dotsizes[] =
{
  { 0x1, 0.50 },
  { 0x2, 1.00 }
};

static const stpi_dotsize_t standard_2pl_720_dotsizes[] =
{
  { 0x1, 0.25 },
  { 0x2, 0.50 },
  { 0x3, 1.00 }
};

static const stpi_dotsize_t standard_2pl_360_dotsizes[] =
{
  { 0x1, 0.25 },
  { 0x2, 0.50 },
  { 0x3, 1.00 }
};

static const stpi_dither_range_simple_t standard_2pl_2880_dither_ranges[] =
{
  { 1.00, 0x1, 0, 1 },
};

static const stpi_shade_t standard_2pl_2880_shades[] =
{
  SHADE(1.0, 0, standard_2pl_2880_dotsizes)
};

DECLARE_INK2(standard_2pl_2880, 1.0);

static const stpi_dither_range_simple_t photo_2pl_2880_dither_ranges[] =
{
  { 0.26, 0x1, 1, 1 },
  { 1.00, 0x1, 0, 1 },
};

static const stpi_shade_t photo_2pl_2880_shades[] =
{
  SHADE(0.26, 0, standard_2pl_2880_dotsizes),
  SHADE(1.0,  0, standard_2pl_2880_dotsizes)
};

DECLARE_INK2(photo_2pl_2880, 0.5);

static const stpi_dither_range_simple_t photo_2pl_2880_c_dither_ranges[] =
{
  { 0.26, 0x1, 1, 1 },
  { 1.00, 0x1, 0, 1 },
};

static const stpi_shade_t photo_2pl_2880_c_shades[] =
{
  SHADE(0.26, 1, standard_2pl_2880_dotsizes),
  SHADE(1.0,  0, standard_2pl_2880_dotsizes)
};

DECLARE_INK2(photo_2pl_2880_c, 0.5);

static const stpi_dither_range_simple_t photo_2pl_2880_m_dither_ranges[] =
{
  { 0.31, 0x1, 1, 1 },
  { 1.00, 0x1, 0, 1 },
};

static const stpi_shade_t photo_2pl_2880_m_shades[] =
{
  SHADE(0.31, 1, standard_2pl_2880_dotsizes),
  SHADE(1.0,  0, standard_2pl_2880_dotsizes)
};

DECLARE_INK2(photo_2pl_2880_m, 0.5);

static const stpi_dither_range_simple_t photo_2pl_2880_y_dither_ranges[] =
{
  { 0.5,  0x1, 0, 1 },
  { 1.00, 0x1, 1, 1 },
};

static const stpi_shade_t photo_2pl_2880_y_shades[] =
{
  SHADE(0.5, 0, standard_2pl_2880_dotsizes),
  SHADE(1.0, 1, standard_2pl_2880_dotsizes)
};

DECLARE_INK2(photo_2pl_2880_y, 1.00);

static const stpi_dither_range_simple_t piezo_2pl_2880_quadtone_dither_ranges[]=
{
  { PIEZO_0, 0x1, 0, 1 },
  { PIEZO_1, 0x1, 1, 1 },
  { PIEZO_2, 0x1, 2, 1 },
  { PIEZO_3, 0x1, 3, 1 },
};

static const stpi_shade_t piezo_2pl_2880_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_2pl_2880_dotsizes),
  SHADE(PIEZO_1, 1, standard_2pl_2880_dotsizes),
  SHADE(PIEZO_2, 2, standard_2pl_2880_dotsizes),
  SHADE(PIEZO_3, 3, standard_2pl_2880_dotsizes)
};

DECLARE_INK2(piezo_2pl_2880_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t standard_2pl_1440_dither_ranges[] =
{
  { 0.5,   0x1, 0, 1 },
  { 1.00,  0x2, 0, 2 }
};

static const stpi_shade_t standard_2pl_1440_shades[] =
{
  SHADE(1.0, 0, standard_2pl_1440_dotsizes)
};

DECLARE_INK2(standard_2pl_1440, 1.0);

static const stpi_dither_range_simple_t piezo_2pl_1440_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .5,   0x1, 0, 1 },
  { PIEZO_1 * .5,   0x1, 1, 1 },
  { PIEZO_2 * .5,   0x1, 2, 1 },
  { PIEZO_2 * 1.00, 0x2, 2, 2 },
  { PIEZO_3 * 1.00, 0x2, 3, 2 },
};

static const stpi_shade_t piezo_2pl_1440_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_2pl_1440_dotsizes),
  SHADE(PIEZO_1, 1, standard_2pl_1440_dotsizes),
  SHADE(PIEZO_2, 2, standard_2pl_1440_dotsizes),
  SHADE(PIEZO_3, 3, standard_2pl_1440_dotsizes)
};

DECLARE_INK2(piezo_2pl_1440_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t photo_2pl_1440_dither_ranges[] =
{
  { 0.13,  0x1, 1, 1 },
  { 0.26,  0x2, 1, 2 },
  { 0.5,   0x1, 0, 1 },
  { 1.00,  0x2, 0, 2 }
};

static const stpi_shade_t photo_2pl_1440_shades[] =
{
  SHADE(0.26, 1, standard_2pl_1440_dotsizes),
  SHADE(1.00, 0, standard_2pl_1440_dotsizes)
};

DECLARE_INK2(photo_2pl_1440, 1.0);

static const stpi_dither_range_simple_t photo_2pl_1440_y_dither_ranges[] =
{
  { 0.25,  0x1, 0, 1 },
  { 0.50,  0x2, 0, 2 },
  { 1.00,  0x2, 1, 2 }
};

static const stpi_shade_t photo_2pl_1440_y_shades[] =
{
  SHADE(0.50, 0, standard_2pl_1440_dotsizes),
  SHADE(1.00, 1, standard_2pl_1440_dotsizes)
};

DECLARE_INK2(photo_2pl_1440_y, 1.0);

static const stpi_dither_range_simple_t standard_2pl_720_dither_ranges[] =
{
  { 0.25,  0x1, 0, 1 },
  { 0.5,   0x2, 0, 2 },
  { 1.0,   0x3, 0, 4 }
};

static const stpi_shade_t standard_2pl_720_shades[] =
{
  SHADE(1.0, 0, standard_2pl_720_dotsizes)
};

DECLARE_INK2(standard_2pl_720, 1.0);

static const stpi_dither_range_simple_t photo_2pl_720_dither_ranges[] =
{
  { 0.065, 0x1, 1, 1 },
  { 0.13,  0x2, 1, 2 },
/* { 0.26, 0x3, 1, 4 }, */
  { 0.25,  0x1, 0, 1 },
  { 0.5,   0x2, 0, 2 },
  { 1.0,   0x3, 0, 4 }
};

static const stpi_shade_t photo_2pl_720_shades[] =
{
  SHADE(0.26, 1, standard_2pl_720_dotsizes),
  SHADE(1.00, 0, standard_2pl_720_dotsizes)
};

DECLARE_INK2(photo_2pl_720, 1.0);

static const stpi_dither_range_simple_t photo_2pl_720_y_dither_ranges[] =
{
  { 0.125, 0x1, 0, 1 },
  { 0.25,  0x2, 0, 2 },
  { 0.5,   0x2, 1, 2 },
  { 1.0,   0x3, 1, 4 }
};

static const stpi_shade_t photo_2pl_720_y_shades[] =
{
  SHADE(0.5, 0, standard_2pl_720_dotsizes),
  SHADE(1.0, 1, standard_2pl_720_dotsizes)
};

DECLARE_INK2(photo_2pl_720_y, 1.0);

static const stpi_dither_range_simple_t piezo_2pl_720_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .25, 0x1, 0, 1 },
  { PIEZO_0 * .50, 0x2, 0, 2 },
  { PIEZO_1 * .50, 0x2, 1, 2 },
  { PIEZO_2 * .50, 0x2, 2, 2 },
  { PIEZO_2 * 1.0, 0x3, 2, 4 },
  { PIEZO_3 * 1.0, 0x3, 3, 4 },
};

static const stpi_shade_t piezo_2pl_720_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_2pl_720_dotsizes),
  SHADE(PIEZO_1, 1, standard_2pl_720_dotsizes),
  SHADE(PIEZO_2, 2, standard_2pl_720_dotsizes),
  SHADE(PIEZO_3, 3, standard_2pl_720_dotsizes)
};

DECLARE_INK2(piezo_2pl_720_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t standard_2pl_360_dither_ranges[] =
{
  { 0.25,  0x1, 0, 2 },
  { 0.5,   0x2, 0, 4 },
  { 1.0,   0x3, 0, 7 }
};

static const stpi_shade_t standard_2pl_360_shades[] =
{
  SHADE(1.0, 0, standard_2pl_360_dotsizes)
};

DECLARE_INK2(standard_2pl_360, 1.0);

static const stpi_dither_range_simple_t photo_2pl_360_dither_ranges[] =
{
  { 0.065,  0x1, 1, 1 },
  { 0.13,   0x2, 1, 2 },
  { 0.26,   0x3, 1, 3 },
  { 1.0,    0x3, 0, 3 }
};

static const stpi_shade_t photo_2pl_360_shades[] =
{
  SHADE(0.26, 1, standard_2pl_360_dotsizes),
  SHADE(1.0,  0, standard_2pl_360_dotsizes)
};

DECLARE_INK2(photo_2pl_360, 1.0);

static const stpi_dither_range_simple_t photo_2pl_360_y_dither_ranges[] =
{
  { 0.145, 0x1, 0, 1 },
  { 0.290, 0x2, 0, 2 },
  { 0.5,   0x3, 0, 3 },
  { 1.0,   0x3, 1, 3 }
};

static const stpi_shade_t photo_2pl_360_y_shades[] =
{
  SHADE(0.5, 0, standard_2pl_360_dotsizes),
  SHADE(1.0, 1, standard_2pl_360_dotsizes)
};

DECLARE_INK2(photo_2pl_360_y, 1.0);

static const stpi_dither_range_simple_t piezo_2pl_360_quadtone_dither_ranges[] =
{
  { PIEZO_0 * .25, 0x1, 0, 2 },
  { PIEZO_0 * .50, 0x2, 0, 4 },
  { PIEZO_1 * .50, 0x2, 1, 4 },
  { PIEZO_2 * .50, 0x2, 2, 4 },
  { PIEZO_2 * 1.0, 0x3, 2, 7 },
  { PIEZO_3 * 1.0, 0x3, 3, 7 },
};

static const stpi_shade_t piezo_2pl_360_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_2pl_360_dotsizes),
  SHADE(PIEZO_1, 1, standard_2pl_360_dotsizes),
  SHADE(PIEZO_2, 2, standard_2pl_360_dotsizes),
  SHADE(PIEZO_3, 3, standard_2pl_360_dotsizes)
};

DECLARE_INK2(piezo_2pl_360_quadtone, PIEZO_DENSITY);


/***************************************************************\
*                                                               *
*                     STYLUS C70/C80 (PIGMENT)                  *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t standard_economy_pigment_dotsizes[] =
{
  { 0x3, 1.0 }
};

static const stpi_dotsize_t standard_multishot_pigment_dotsizes[] =
{
  { 0x1, 0.41 },
  { 0x3, 1.00 }
};

static const stpi_dotsize_t standard_6pl_pigment_dotsizes[] =
{
  { 0x1, 0.30 },
  { 0x3, 1.00 }
};

static const stpi_dotsize_t standard_3pl_pigment_dotsizes[] =
{
  { 0x1, 0.65 },
  { 0x2, 1.00 }
};

static const stpi_dotsize_t standard_3pl_pigment_2880_dotsizes[] =
{
  { 0x1, 1.00 }
};

static const stpi_dither_range_simple_t standard_economy_pigment_dither_ranges[] =
{
  { 1.0,   0x3, 0, 3 }
};

static const stpi_shade_t standard_economy_pigment_shades[] =
{
  SHADE(1.0, 0, standard_economy_pigment_dotsizes)
};

DECLARE_INK2(standard_economy_pigment, 1.0);

static const stpi_dither_range_simple_t standard_multishot_pigment_dither_ranges[] =
{
  { 0.410, 0x1, 0, 2 },
  { 1.0,   0x3, 0, 5 }
};

static const stpi_shade_t standard_multishot_pigment_shades[] =
{
  SHADE(1.0, 0, standard_multishot_pigment_dotsizes)
};

DECLARE_INK2(standard_multishot_pigment, 1.0);

static const stpi_dither_range_simple_t standard_6pl_pigment_dither_ranges[] =
{
  { 0.300, 0x1, 0, 3 },
  { 1.0,   0x3, 0, 10 }
};

static const stpi_shade_t standard_6pl_pigment_shades[] =
{
  SHADE(1.0, 0, standard_6pl_pigment_dotsizes)
};

DECLARE_INK2(standard_6pl_pigment, 1.0);

static const stpi_dither_range_simple_t standard_3pl_pigment_dither_ranges[] =
{
  { 0.650, 0x1, 0, 2 },
  { 1.000, 0x2, 0, 3 },
};

static const stpi_shade_t standard_3pl_pigment_shades[] =
{
  SHADE(1.0, 0, standard_3pl_pigment_dotsizes)
};

DECLARE_INK2(standard_3pl_pigment, 1.0);

static const stpi_dither_range_simple_t standard_3pl_pigment_2880_dither_ranges[] =
{
  { 1.0,   0x1, 0, 1 }
};

static const stpi_shade_t standard_3pl_pigment_2880_shades[] =
{
  SHADE(1.0, 0, standard_3pl_pigment_2880_dotsizes)
};

DECLARE_INK2(standard_3pl_pigment_2880, 1.0);

static const stpi_dither_range_simple_t piezo_economy_pigment_quadtone_dither_ranges[]=
{
  { PIEZO_0, 0x3, 0, 1 },
  { PIEZO_1, 0x3, 1, 1 },
  { PIEZO_2, 0x3, 2, 1 },
  { PIEZO_3, 0x3, 3, 1 },
};

static const stpi_shade_t piezo_economy_pigment_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_economy_pigment_dotsizes),
  SHADE(PIEZO_1, 1, standard_economy_pigment_dotsizes),
  SHADE(PIEZO_2, 2, standard_economy_pigment_dotsizes),
  SHADE(PIEZO_3, 3, standard_economy_pigment_dotsizes)
};

DECLARE_INK2(piezo_economy_pigment_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_multishot_pigment_quadtone_dither_ranges[]=
{
  { PIEZO_0 * .410, 0x1, 0, 2 },
  { PIEZO_1 * .410, 0x1, 1, 2 },
  { PIEZO_2 * .410, 0x1, 2, 2 },
  { PIEZO_3 * 1.00, 0x3, 3, 5 },
};

static const stpi_shade_t piezo_multishot_pigment_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_multishot_pigment_dotsizes),
  SHADE(PIEZO_1, 1, standard_multishot_pigment_dotsizes),
  SHADE(PIEZO_2, 2, standard_multishot_pigment_dotsizes),
  SHADE(PIEZO_3, 3, standard_multishot_pigment_dotsizes)
};

DECLARE_INK2(piezo_multishot_pigment_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_6pl_pigment_quadtone_dither_ranges[]=
{
  { PIEZO_0 * .300, 0x1, 0, 3 },
  { PIEZO_0 * .600, 0x2, 0, 6 },
  { PIEZO_1 * .600, 0x2, 1, 6 },
  { PIEZO_2 * .600, 0x2, 2, 6 },
  { PIEZO_3 * 1.00, 0x3, 3, 10 },
};

static const stpi_shade_t piezo_6pl_pigment_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_6pl_pigment_dotsizes),
  SHADE(PIEZO_1, 1, standard_6pl_pigment_dotsizes),
  SHADE(PIEZO_2, 2, standard_6pl_pigment_dotsizes),
  SHADE(PIEZO_3, 3, standard_6pl_pigment_dotsizes)
};

DECLARE_INK2(piezo_6pl_pigment_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_3pl_pigment_quadtone_dither_ranges[]=
{
  { PIEZO_0 * .650, 0x1, 0, 2 },
  { PIEZO_1 * .650, 0x1, 1, 2 },
  { PIEZO_2 * .650, 0x1, 2, 2 },
  { PIEZO_3 * 1.00, 0x2, 3, 3 },
};

static const stpi_shade_t piezo_3pl_pigment_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_3pl_pigment_dotsizes),
  SHADE(PIEZO_1, 1, standard_3pl_pigment_dotsizes),
  SHADE(PIEZO_2, 2, standard_3pl_pigment_dotsizes),
  SHADE(PIEZO_3, 3, standard_3pl_pigment_dotsizes)
};

DECLARE_INK2(piezo_3pl_pigment_quadtone, PIEZO_DENSITY);

static const stpi_dither_range_simple_t piezo_3pl_pigment_2880_quadtone_dither_ranges[]=
{
  { PIEZO_0, 0x1, 0, 1 },
  { PIEZO_1, 0x1, 1, 1 },
  { PIEZO_2, 0x1, 2, 1 },
  { PIEZO_3, 0x1, 3, 1 },
};

static const stpi_shade_t piezo_3pl_pigment_2880_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_3pl_pigment_2880_dotsizes),
  SHADE(PIEZO_1, 1, standard_3pl_pigment_2880_dotsizes),
  SHADE(PIEZO_2, 2, standard_3pl_pigment_2880_dotsizes),
  SHADE(PIEZO_3, 3, standard_3pl_pigment_2880_dotsizes)
};

DECLARE_INK2(piezo_3pl_pigment_2880_quadtone, PIEZO_DENSITY);


/***************************************************************\
*                                                               *
*                   STYLUS PHOTO 2000P                          *
*                                                               *
\***************************************************************/

static const stpi_dither_range_simple_t standard_pigment_dither_ranges[] =
{ /* MRS: Not calibrated! */
  { 0.55,  0x1, 0, 1 },
  { 1.0,   0x2, 0, 2 }
};

static const stpi_dotsize_t standard_pigment_dotsizes[] =
{
  { 0x1, 0.55 },
  { 0x2, 1.00 }
};

static const stpi_shade_t standard_pigment_shades[] =
{
  SHADE(1.0, 0, standard_pigment_dotsizes)
};

DECLARE_INK2(standard_pigment, 1.0);

static const stpi_dither_range_simple_t photo_pigment_dither_ranges[] =
{ /* MRS: Not calibrated! */
  { 0.15,  0x1, 1, 1 },
  { 0.227, 0x2, 1, 2 },
  { 0.5,   0x1, 0, 1 },
  { 1.0,   0x2, 0, 2 }
};

static const stpi_shade_t photo_pigment_shades[] =
{
  SHADE(0.227, 0, standard_pigment_dotsizes),
  SHADE(1.0,   1, standard_pigment_dotsizes)
};

DECLARE_INK2(photo_pigment, 1.0);

static const stpi_dither_range_simple_t piezo_pigment_quadtone_dither_ranges[]=
{
  { PIEZO_0 * .550, 0x1, 0, 1 },
  { PIEZO_1 * .550, 0x1, 1, 1 },
  { PIEZO_2 * .550, 0x1, 2, 1 },
  { PIEZO_3 * .550, 0x1, 3, 1 },
  { PIEZO_3 * 1.00, 0x2, 3, 2 },
};

static const stpi_shade_t piezo_pigment_quadtone_shades[] =
{
  SHADE(PIEZO_0, 0, standard_pigment_dotsizes),
  SHADE(PIEZO_1, 1, standard_pigment_dotsizes),
  SHADE(PIEZO_2, 2, standard_pigment_dotsizes),
  SHADE(PIEZO_3, 3, standard_pigment_dotsizes)
};

DECLARE_INK2(piezo_pigment_quadtone, PIEZO_DENSITY);


/***************************************************************\
*                                                               *
*            ULTRACHROME (2100/2200, 7600, 9600)                *
*                                                               *
\***************************************************************/

static const stpi_dither_range_simple_t standard_4pl_pigment_low_dither_ranges[] =
{
  { 0.40,  0x1, 0, 40 },
  { 0.70,  0x2, 0, 70 },
  { 1.00,  0x3, 0, 100 }
};

static const stpi_dotsize_t standard_4pl_pigment_low_dotsizes[] =
{
  { 0x1, 0.40 },
  { 0x2, 0.70 },
  { 0x3, 1.00 }
};

static const stpi_shade_t standard_4pl_pigment_low_shades[] =
{
  SHADE(1.0, 0, standard_4pl_pigment_low_dotsizes)
};

DECLARE_INK2(standard_4pl_pigment_low, 0.5);

static const stpi_dither_range_simple_t photo_4pl_pigment_low_m_dither_ranges[] =
{
  { 0.104,  0x1, 1, 40 },
  { 0.182,  0x2, 1, 70 },
  { 0.26,   0x3, 1, 100 },
  { 0.70,   0x2, 0, 70 },
  { 1.00,   0x3, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_low_m_shades[] =
{
  SHADE(0.26, 1, standard_4pl_pigment_low_dotsizes),
  SHADE(1.0,  0, standard_4pl_pigment_low_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_low_m, 0.5);

static const stpi_dither_range_simple_t photo_4pl_pigment_low_c_dither_ranges[] =
{
  { 0.16,   0x1, 1, 40 },
  { 0.28,   0x2, 1, 70 },
  { 0.40,   0x3, 1, 100 },
  { 0.70,   0x2, 0, 70 },
  { 1.00,   0x3, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_low_c_shades[] =
{
  SHADE(0.40, 1, standard_4pl_pigment_low_dotsizes),
  SHADE(1.0,  0, standard_4pl_pigment_low_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_low_c, 0.5);

static const stpi_dither_range_simple_t photo_4pl_pigment_low_y_dither_ranges[] =
{
  { 0.20,   0x1, 1, 40 },
  { 0.35,   0x2, 1, 70 },
  { 0.50,   0x3, 1, 100 },
  { 1.00,   0x3, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_low_y_shades[] =
{
  SHADE(0.50, 1, standard_4pl_pigment_low_dotsizes),
  SHADE(1.0,  0, standard_4pl_pigment_low_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_low_y, 1.5);

static const stpi_dither_range_simple_t photo_4pl_pigment_low_k_dither_ranges[] =
{
  { 0.196,  0x1, 1, 40 },
  { 0.40,   0x1, 0, 40 },
  { 0.70,   0x2, 0, 70 },
  { 1.00,   0x3, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_low_k_shades[] =
{
  SHADE(0.49, 1, standard_4pl_pigment_low_dotsizes),
  SHADE(1.0,  0, standard_4pl_pigment_low_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_low_k, 0.5);

static const stpi_dither_range_simple_t standard_4pl_pigment_dither_ranges[] =
{
  { 0.28,  0x1, 0, 28 },
  { 0.50,  0x2, 0, 50 },
  { 1.00,  0x3, 0, 100 }
};

static const stpi_dotsize_t standard_4pl_pigment_dotsizes[] =
{
  { 0x1, 0.28 },
  { 0x2, 0.50 },
  { 0x3, 1.00 }
};

static const stpi_shade_t standard_4pl_pigment_shades[] =
{
  SHADE(1.0, 0, standard_4pl_pigment_dotsizes)
};

DECLARE_INK2(standard_4pl_pigment, 1.0);

static const stpi_dither_range_simple_t photo_4pl_pigment_m_dither_ranges[] =
{
  { 0.0728, 0x1, 1, 28 },
  { 0.13,   0x2, 1, 50 },
  { 0.26,   0x3, 1, 100 },
  { 0.50,   0x2, 0, 50 },
  { 1.00,   0x3, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_m_shades[] =
{
  SHADE(0.26, 1, standard_4pl_pigment_dotsizes),
  SHADE(1.0,  0, standard_4pl_pigment_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_m, 1.0);

static const stpi_dither_range_simple_t photo_4pl_pigment_c_dither_ranges[] =
{
  { 0.112,  0x1, 1, 28 },
  { 0.20,   0x2, 1, 50 },
  { 0.40,   0x3, 1, 100 },
  { 0.50,   0x2, 0, 50 },
  { 1.00,   0x3, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_c_shades[] =
{
  SHADE(0.40, 1, standard_4pl_pigment_dotsizes),
  SHADE(1.0,  0, standard_4pl_pigment_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_c, 1.0);

static const stpi_dither_range_simple_t photo_4pl_pigment_y_dither_ranges[] =
{
  { 0.14,   0x1, 1, 28 },
  { 0.25,   0x2, 1, 50 },
  { 0.50,   0x3, 1, 100 },
  { 1.00,   0x3, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_y_shades[] =
{
  SHADE(0.50, 1, standard_4pl_pigment_dotsizes),
  SHADE(1.0,  0, standard_4pl_pigment_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_y, 1.5);

static const stpi_dither_range_simple_t photo_4pl_pigment_k_dither_ranges[] =
{
  { 0.1344, 0x1, 1, 28 },
  { 0.24,   0x2, 1, 50 },
  { 0.50,   0x2, 0, 50 },
  { 1.00,   0x3, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_k_shades[] =
{
  SHADE(0.48, 1, standard_4pl_pigment_dotsizes),
  SHADE(1.0,  0, standard_4pl_pigment_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_k, 0.75);

static const stpi_dither_range_simple_t standard_4pl_pigment_1440_dither_ranges[] =
{
  { 0.56,  0x1, 0, 56 },
  { 1.00,  0x2, 0, 100 },
};

static const stpi_dotsize_t standard_4pl_pigment_1440_dotsizes[] =
{
  { 0x1, 0.56 },
  { 0x2, 1.00 },
};

static const stpi_shade_t standard_4pl_pigment_1440_shades[] =
{
  SHADE(1.0, 0, standard_4pl_pigment_1440_dotsizes)
};

DECLARE_INK2(standard_4pl_pigment_1440, 1.0);

static const stpi_dither_range_simple_t photo_4pl_pigment_1440_m_dither_ranges[] =
{
  { 0.1456, 0x1, 1, 56 },
  { 0.26,   0x2, 1, 100 },
  { 0.56,   0x1, 0, 56 },
  { 1.00,   0x2, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_1440_m_shades[] =
{
  SHADE(0.26, 1, standard_4pl_pigment_1440_dotsizes),
  SHADE(1.0, 0, standard_4pl_pigment_1440_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_1440_m, 1.0);

static const stpi_dither_range_simple_t photo_4pl_pigment_1440_c_dither_ranges[] =
{
  { 0.224,  0x1, 1, 56 },
  { 0.40,   0x2, 1, 100 },
  { 0.56,   0x1, 0, 56 },
  { 1.00,   0x2, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_1440_c_shades[] =
{
  SHADE(0.40, 1, standard_4pl_pigment_1440_dotsizes),
  SHADE(1.0, 0, standard_4pl_pigment_1440_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_1440_c, 1.0);

static const stpi_dither_range_simple_t photo_4pl_pigment_1440_y_dither_ranges[] =
{
  { 0.28,   0x1, 1, 56 },
  { 0.50,   0x2, 1, 100 },
  { 1.00,   0x2, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_1440_y_shades[] =
{
  SHADE(0.50, 1, standard_4pl_pigment_1440_dotsizes),
  SHADE(1.0, 0, standard_4pl_pigment_1440_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_1440_y, 1.5);

static const stpi_dither_range_simple_t photo_4pl_pigment_1440_k_dither_ranges[] =
{
  { 0.2688, 0x1, 1, 56 },
  { 0.56,   0x1, 0, 56 },
  { 1.00,   0x2, 0, 100 }
};

static const stpi_shade_t photo_4pl_pigment_1440_k_shades[] =
{
  SHADE(0.56, 1, standard_4pl_pigment_1440_dotsizes),
  SHADE(1.0, 0, standard_4pl_pigment_1440_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_1440_k, 0.75);

static const stpi_dither_range_simple_t standard_4pl_pigment_2880_dither_ranges[] =
{
  { 1.00,  0x1, 0, 1 },
};

static const stpi_dotsize_t standard_4pl_pigment_2880_dotsizes[] =
{
  { 0x1, 1.0 },
};

static const stpi_shade_t standard_4pl_pigment_2880_shades[] =
{
  SHADE(1.0, 0, standard_4pl_pigment_2880_dotsizes)
};

DECLARE_INK2(standard_4pl_pigment_2880, 1.0);

static const stpi_dither_range_simple_t photo_4pl_pigment_2880_m_dither_ranges[] =
{
  { 0.26,  0x1, 1, 1 },
  { 1.00,  0x1, 0, 1 },
};

static const stpi_shade_t photo_4pl_pigment_2880_m_shades[] =
{
  SHADE(0.26, 1, standard_4pl_pigment_2880_dotsizes),
  SHADE(1.0, 0, standard_4pl_pigment_2880_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_2880_m, 0.75);

static const stpi_dither_range_simple_t photo_4pl_pigment_2880_c_dither_ranges[] =
{
  { 0.40,  0x1, 1, 1 },
  { 1.00,  0x1, 0, 1 },
};

static const stpi_shade_t photo_4pl_pigment_2880_c_shades[] =
{
  SHADE(0.40, 1, standard_4pl_pigment_2880_dotsizes),
  SHADE(1.0, 0, standard_4pl_pigment_2880_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_2880_c, 0.75);

static const stpi_dither_range_simple_t photo_4pl_pigment_2880_y_dither_ranges[] =
{
  { 0.50,  0x1, 1, 1 },
  { 1.00,  0x1, 0, 1 },
};

static const stpi_shade_t photo_4pl_pigment_2880_y_shades[] =
{
  SHADE(0.50, 1, standard_4pl_pigment_2880_dotsizes),
  SHADE(1.0, 0, standard_4pl_pigment_2880_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_2880_y, 1.5);

static const stpi_dither_range_simple_t photo_4pl_pigment_2880_k_dither_ranges[] =
{
  { 0.48,  0x1, 1, 1 },
  { 1.00,  0x1, 0, 1 },
};

static const stpi_shade_t photo_4pl_pigment_2880_k_shades[] =
{
  SHADE(0.48, 1, standard_4pl_pigment_2880_dotsizes),
  SHADE(1.0, 0, standard_4pl_pigment_2880_dotsizes)
};

DECLARE_INK2(photo_4pl_pigment_2880_k, 0.75);


/***************************************************************\
*                                                               *
*                      STYLUS PRO 10000                         *
*                                                               *
\***************************************************************/

static const stpi_dither_range_simple_t spro10000_standard_dither_ranges[] =
{
  { 0.661, 0x1, 0, 2 },
  { 1.00,  0x2, 0, 3 }
};

static const stpi_dotsize_t spro10000_standard_dotsizes[] =
{
  { 0x1, 0.661 },
  { 0x2, 1.00 },
};

static const stpi_shade_t spro10000_standard_shades[] =
{
  SHADE(1.0, 0, spro10000_standard_dotsizes)
};

DECLARE_INK2(spro10000_standard, 1.0);

static const stpi_dither_range_simple_t spro10000_photo_dither_ranges[] =
{
  { 0.17,  0x1, 1, 2 },
  { 0.26,  0x2, 1, 3 },
  { 0.661, 0x1, 0, 2 },
  { 1.00,  0x2, 0, 3 }
};

static const stpi_shade_t spro10000_photo_shades[] =
{
  SHADE(0.26, 1, spro10000_standard_dotsizes),
  SHADE(1.0, 0, spro10000_standard_dotsizes),
};

DECLARE_INK2(spro10000_photo, 1.0);


static const escp2_variable_inkset_t standard_inks =
{
  &standard_ink,
  &standard_ink,
  &standard_ink,
  &standard_ink
};

static const escp2_variable_inkset_t photo_inks =
{
  &standard_ink,
  &photo_cyan_ink,
  &photo_magenta_ink,
  &standard_ink
};

static const escp2_variable_inkset_t extended_inks =
{
  &standard_ink,
  &standard_ink,
  &standard_ink,
  &standard_ink,
  &standard_ink,
  &standard_ink,
  &standard_ink
};


static const escp2_variable_inkset_t piezo_quadtone_inks =
{
  &piezo_quadtone_ink,
};

static const escp2_variable_inkset_t escp2_multishot_standard_inks =
{
  &standard_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink
};

static const escp2_variable_inkset_t escp2_multishot_extended_inks =
{
  &standard_multishot_ink,
  &standard_multishot_ink,
  &standard_multishot_ink,
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

static const escp2_variable_inkset_t escp2_multishot_photo2_inks =
{
  &photo_multishot_ink,
  &photo_multishot_ink,
  &photo_multishot_ink,
  &standard_multishot_ink
};

static const escp2_variable_inkset_t escp2_multishot_photoj_inks =
{
  &standard_multishot_ink,
  &photo_multishot_ink,
  &photo_multishot_ink,
  &photo_multishot_y_ink
};

static const escp2_variable_inkset_t piezo_multishot_quadtone_inks =
{
  &piezo_multishot_quadtone_ink
};


static const escp2_variable_inkset_t escp2_6pl_standard_inks =
{
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_extended_inks =
{
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_photo_inks =
{
  &standard_6pl_ink,
  &photo_6pl_ink,
  &photo_6pl_ink,
  &standard_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_photo2_inks =
{
  &photo_6pl_ink,
  &photo_6pl_ink,
  &photo_6pl_ink,
  &standard_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_photoj_inks =
{
  &standard_6pl_ink,
  &photo_6pl_ink,
  &photo_6pl_ink,
  &photo_6pl_y_ink
};

static const escp2_variable_inkset_t piezo_6pl_quadtone_inks =
{
  &piezo_6pl_quadtone_ink
};


static const escp2_variable_inkset_t escp2_6pl_1440_standard_inks =
{
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink
};

static const escp2_variable_inkset_t escp2_6pl_1440_extended_inks =
{
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink,
  &standard_6pl_1440_ink,
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

static const escp2_variable_inkset_t piezo_6pl_1440_quadtone_inks =
{
  &piezo_6pl_1440_quadtone_ink
};


static const escp2_variable_inkset_t escp2_6pl_2880_standard_inks =
{
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink
};

static const escp2_variable_inkset_t escp2_6pl_2880_extended_inks =
{
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink
};

static const escp2_variable_inkset_t escp2_6pl_2880_photo_inks =
{
  &standard_6pl_2880_ink,
  &photo_6pl_2880_ink,
  &photo_6pl_2880_ink,
  &standard_6pl_2880_ink
};

static const escp2_variable_inkset_t piezo_6pl_2880_quadtone_inks =
{
  &piezo_6pl_2880_quadtone_ink
};


static const escp2_variable_inkset_t escp2_680_multishot_standard_inks =
{
  &standard_680_multishot_ink,
  &standard_680_multishot_ink,
  &standard_680_multishot_ink,
  &standard_680_multishot_ink
};

static const escp2_variable_inkset_t escp2_680_6pl_standard_inks =
{
  &standard_680_6pl_ink,
  &standard_680_6pl_ink,
  &standard_680_6pl_ink,
  &standard_680_6pl_ink
};

static const escp2_variable_inkset_t piezo_680_multishot_quadtone_inks =
{
  &piezo_680_multishot_quadtone_ink
};

static const escp2_variable_inkset_t piezo_680_6pl_quadtone_inks =
{
  &piezo_680_6pl_quadtone_ink
};


static const escp2_variable_inkset_t escp2_4pl_standard_inks =
{
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink
};

static const escp2_variable_inkset_t escp2_4pl_extended_inks =
{
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink,
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

static const escp2_variable_inkset_t escp2_4pl_photoj_inks =
{
  &standard_4pl_ink,
  &photo_4pl_ink,
  &photo_4pl_ink,
  &photo_4pl_y_ink,
};

static const escp2_variable_inkset_t piezo_4pl_quadtone_inks =
{
  &piezo_4pl_quadtone_ink
};

static const escp2_variable_inkset_t escp2_4pl_2880_standard_inks =
{
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink
};

static const escp2_variable_inkset_t escp2_4pl_2880_extended_inks =
{
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink,
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

static const escp2_variable_inkset_t escp2_4pl_2880_photoj_inks =
{
  &standard_4pl_2880_ink,
  &photo_4pl_2880_ink,
  &photo_4pl_2880_ink,
  &photo_4pl_y_2880_ink
};

static const escp2_variable_inkset_t piezo_4pl_2880_quadtone_inks =
{
  &piezo_4pl_2880_quadtone_ink
};


static const escp2_variable_inkset_t escp2_6pl_standard_980_inks =
{
  &standard_980_6pl_ink,
  &standard_980_6pl_ink,
  &standard_980_6pl_ink,
  &standard_980_6pl_ink
};

static const escp2_variable_inkset_t piezo_6pl_quadtone_980_inks =
{
  &piezo_980_6pl_quadtone_ink
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

static const escp2_variable_inkset_t piezo_3pl_quadtone_inks =
{
  &piezo_3pl_quadtone_ink
};

static const escp2_variable_inkset_t piezo_3pl_1440_quadtone_inks =
{
  &piezo_3pl_1440_quadtone_ink
};

static const escp2_variable_inkset_t piezo_3pl_2880_quadtone_inks =
{
  &piezo_3pl_2880_quadtone_ink
};


static const escp2_variable_inkset_t escp2_2pl_2880_standard_inks =
{
  &standard_2pl_2880_ink,
  &standard_2pl_2880_ink,
  &standard_2pl_2880_ink,
  &standard_2pl_2880_ink
};

static const escp2_variable_inkset_t escp2_2pl_2880_extended_inks =
{
  &standard_2pl_2880_ink,
  &standard_2pl_2880_ink,
  &standard_2pl_2880_ink,
  &standard_2pl_2880_ink,
  &standard_2pl_2880_ink,
  &standard_2pl_2880_ink,
  &standard_2pl_2880_ink
};

static const escp2_variable_inkset_t escp2_2pl_2880_photo_inks =
{
  &standard_2pl_2880_ink,
  &photo_2pl_2880_c_ink,
  &photo_2pl_2880_m_ink,
  &standard_2pl_2880_ink
};

static const escp2_variable_inkset_t escp2_2pl_2880_photo2_inks =
{
  &photo_2pl_2880_ink,
  &photo_2pl_2880_ink,
  &photo_2pl_2880_ink,
  &standard_2pl_2880_ink
};

static const escp2_variable_inkset_t escp2_2pl_2880_photoj_inks =
{
  &standard_2pl_2880_ink,
  &photo_2pl_2880_ink,
  &photo_2pl_2880_ink,
  &photo_2pl_2880_y_ink
};

static const escp2_variable_inkset_t piezo_2pl_2880_quadtone_inks =
{
  &piezo_2pl_2880_quadtone_ink
};

static const escp2_variable_inkset_t escp2_2pl_1440_standard_inks =
{
  &standard_2pl_1440_ink,
  &standard_2pl_1440_ink,
  &standard_2pl_1440_ink,
  &standard_2pl_1440_ink
};

static const escp2_variable_inkset_t escp2_2pl_1440_extended_inks =
{
  &standard_2pl_1440_ink,
  &standard_2pl_1440_ink,
  &standard_2pl_1440_ink,
  &standard_2pl_1440_ink,
  &standard_2pl_1440_ink,
  &standard_2pl_1440_ink,
  &standard_2pl_1440_ink
};

static const escp2_variable_inkset_t escp2_2pl_1440_photo_inks =
{
  &standard_2pl_1440_ink,
  &photo_2pl_1440_ink,
  &photo_2pl_1440_ink,
  &standard_2pl_1440_ink
};

static const escp2_variable_inkset_t escp2_2pl_1440_photoj_inks =
{
  &standard_2pl_1440_ink,
  &photo_2pl_1440_ink,
  &photo_2pl_1440_ink,
  &photo_2pl_1440_y_ink,
};

static const escp2_variable_inkset_t piezo_2pl_1440_quadtone_inks =
{
  &piezo_2pl_1440_quadtone_ink
};

static const escp2_variable_inkset_t escp2_2pl_720_standard_inks =
{
  &standard_2pl_720_ink,
  &standard_2pl_720_ink,
  &standard_2pl_720_ink,
  &standard_2pl_720_ink
};

static const escp2_variable_inkset_t escp2_2pl_720_extended_inks =
{
  &standard_2pl_720_ink,
  &standard_2pl_720_ink,
  &standard_2pl_720_ink,
  &standard_2pl_720_ink,
  &standard_2pl_720_ink,
  &standard_2pl_720_ink,
  &standard_2pl_720_ink
};

static const escp2_variable_inkset_t escp2_2pl_720_photo_inks =
{
  &standard_2pl_720_ink,
  &photo_2pl_720_ink,
  &photo_2pl_720_ink,
  &standard_2pl_720_ink
};

static const escp2_variable_inkset_t escp2_2pl_720_photo2_inks =
{
  &photo_2pl_720_ink,
  &photo_2pl_720_ink,
  &photo_2pl_720_ink,
  &standard_2pl_720_ink
};

static const escp2_variable_inkset_t escp2_2pl_720_photoj_inks =
{
  &standard_2pl_720_ink,
  &photo_2pl_720_ink,
  &photo_2pl_720_ink,
  &photo_2pl_720_y_ink
};

static const escp2_variable_inkset_t piezo_2pl_720_quadtone_inks =
{
  &piezo_2pl_720_quadtone_ink
};

static const escp2_variable_inkset_t escp2_2pl_360_standard_inks =
{
  &standard_2pl_360_ink,
  &standard_2pl_360_ink,
  &standard_2pl_360_ink,
  &standard_2pl_360_ink
};

static const escp2_variable_inkset_t escp2_2pl_360_extended_inks =
{
  &standard_2pl_360_ink,
  &standard_2pl_360_ink,
  &standard_2pl_360_ink,
  &standard_2pl_360_ink,
  &standard_2pl_360_ink,
  &standard_2pl_360_ink,
  &standard_2pl_360_ink
};

static const escp2_variable_inkset_t escp2_2pl_360_photo_inks =
{
  &standard_2pl_360_ink,
  &photo_2pl_360_ink,
  &photo_2pl_360_ink,
  &standard_2pl_360_ink
};

static const escp2_variable_inkset_t escp2_2pl_360_photo2_inks =
{
  &photo_2pl_360_ink,
  &photo_2pl_360_ink,
  &photo_2pl_360_ink,
  &standard_2pl_360_ink
};

static const escp2_variable_inkset_t escp2_2pl_360_photoj_inks =
{
  &standard_2pl_360_ink,
  &photo_2pl_360_ink,
  &photo_2pl_360_ink,
  &photo_2pl_360_y_ink
};

static const escp2_variable_inkset_t piezo_2pl_360_quadtone_inks =
{
  &piezo_2pl_360_quadtone_ink
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

static const escp2_variable_inkset_t escp2_x80_2880_6pl_standard_inks =
{
  &standard_x80_2880_6pl_ink,
  &standard_x80_2880_6pl_ink,
  &standard_x80_2880_6pl_ink,
  &standard_x80_2880_6pl_ink
};

static const escp2_variable_inkset_t piezo_x80_multishot_quadtone_inks =
{
  &piezo_x80_multishot_quadtone_ink
};

static const escp2_variable_inkset_t piezo_x80_6pl_quadtone_inks =
{
  &piezo_x80_6pl_quadtone_ink
};

static const escp2_variable_inkset_t piezo_x80_1440_6pl_quadtone_inks =
{
  &piezo_x80_1440_6pl_quadtone_ink
};

static const escp2_variable_inkset_t piezo_x80_2880_6pl_quadtone_inks =
{
  &piezo_x80_2880_6pl_quadtone_ink
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

static const escp2_variable_inkset_t piezo_pigment_quadtone_inks =
{
  &piezo_pigment_quadtone_ink
};


static const escp2_variable_inkset_t escp2_multishot_pigment_standard_inks =
{
  &standard_multishot_pigment_ink,
  &standard_multishot_pigment_ink,
  &standard_multishot_pigment_ink,
  &standard_multishot_pigment_ink
};

static const escp2_variable_inkset_t piezo_multishot_pigment_quadtone_inks =
{
  &piezo_multishot_pigment_quadtone_ink
};

static const escp2_variable_inkset_t escp2_economy_pigment_standard_inks =
{
  &standard_economy_pigment_ink,
  &standard_economy_pigment_ink,
  &standard_economy_pigment_ink,
  &standard_economy_pigment_ink
};

static const escp2_variable_inkset_t piezo_economy_pigment_quadtone_inks =
{
  &piezo_economy_pigment_quadtone_ink
};


static const escp2_variable_inkset_t escp2_6pl_pigment_standard_inks =
{
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink
};

static const escp2_variable_inkset_t escp2_6pl_pigment_extended_inks =
{
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink,
  &standard_6pl_pigment_ink
};

static const escp2_variable_inkset_t piezo_6pl_pigment_quadtone_inks =
{
  &piezo_6pl_pigment_quadtone_ink
};


static const escp2_variable_inkset_t escp2_4pl_pigment_low_standard_inks =
{
  &standard_4pl_pigment_low_ink,
  &standard_4pl_pigment_low_ink,
  &standard_4pl_pigment_low_ink,
  &standard_4pl_pigment_low_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_low_extended_inks =
{
  &standard_4pl_pigment_low_ink,
  &standard_4pl_pigment_low_ink,
  &standard_4pl_pigment_low_ink,
  &standard_4pl_pigment_low_ink,
  &standard_4pl_pigment_low_ink,
  &standard_4pl_pigment_low_ink,
  &standard_4pl_pigment_low_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_low_photo_inks =
{
  &standard_4pl_pigment_low_ink,
  &photo_4pl_pigment_low_c_ink,
  &photo_4pl_pigment_low_m_ink,
  &standard_4pl_pigment_low_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_low_photo2_inks =
{
  &photo_4pl_pigment_low_k_ink,
  &photo_4pl_pigment_low_c_ink,
  &photo_4pl_pigment_low_m_ink,
  &standard_4pl_pigment_low_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_low_photoj_inks =
{
  &standard_4pl_pigment_low_ink,
  &photo_4pl_pigment_low_c_ink,
  &photo_4pl_pigment_low_m_ink,
  &photo_4pl_pigment_low_y_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_standard_inks =
{
  &standard_4pl_pigment_ink,
  &standard_4pl_pigment_ink,
  &standard_4pl_pigment_ink,
  &standard_4pl_pigment_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_extended_inks =
{
  &standard_4pl_pigment_ink,
  &standard_4pl_pigment_ink,
  &standard_4pl_pigment_ink,
  &standard_4pl_pigment_ink,
  &standard_4pl_pigment_ink,
  &standard_4pl_pigment_ink,
  &standard_4pl_pigment_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_photo_inks =
{
  &standard_4pl_pigment_ink,
  &photo_4pl_pigment_c_ink,
  &photo_4pl_pigment_m_ink,
  &standard_4pl_pigment_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_photo2_inks =
{
  &photo_4pl_pigment_k_ink,
  &photo_4pl_pigment_c_ink,
  &photo_4pl_pigment_m_ink,
  &standard_4pl_pigment_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_photoj_inks =
{
  &standard_4pl_pigment_ink,
  &photo_4pl_pigment_c_ink,
  &photo_4pl_pigment_m_ink,
  &photo_4pl_pigment_y_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_1440_standard_inks =
{
  &standard_4pl_pigment_1440_ink,
  &standard_4pl_pigment_1440_ink,
  &standard_4pl_pigment_1440_ink,
  &standard_4pl_pigment_1440_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_1440_extended_inks =
{
  &standard_4pl_pigment_1440_ink,
  &standard_4pl_pigment_1440_ink,
  &standard_4pl_pigment_1440_ink,
  &standard_4pl_pigment_1440_ink,
  &standard_4pl_pigment_1440_ink,
  &standard_4pl_pigment_1440_ink,
  &standard_4pl_pigment_1440_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_1440_photo_inks =
{
  &standard_4pl_pigment_1440_ink,
  &photo_4pl_pigment_1440_c_ink,
  &photo_4pl_pigment_1440_m_ink,
  &standard_4pl_pigment_1440_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_1440_photo2_inks =
{
  &photo_4pl_pigment_1440_k_ink,
  &photo_4pl_pigment_1440_c_ink,
  &photo_4pl_pigment_1440_m_ink,
  &standard_4pl_pigment_1440_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_1440_photoj_inks =
{
  &standard_4pl_pigment_1440_ink,
  &photo_4pl_pigment_1440_c_ink,
  &photo_4pl_pigment_1440_m_ink,
  &photo_4pl_pigment_1440_y_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_2880_standard_inks =
{
  &standard_4pl_pigment_2880_ink,
  &standard_4pl_pigment_2880_ink,
  &standard_4pl_pigment_2880_ink,
  &standard_4pl_pigment_2880_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_2880_extended_inks =
{
  &standard_4pl_pigment_2880_ink,
  &standard_4pl_pigment_2880_ink,
  &standard_4pl_pigment_2880_ink,
  &standard_4pl_pigment_2880_ink,
  &standard_4pl_pigment_2880_ink,
  &standard_4pl_pigment_2880_ink,
  &standard_4pl_pigment_2880_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_2880_photo_inks =
{
  &standard_4pl_pigment_2880_ink,
  &photo_4pl_pigment_2880_c_ink,
  &photo_4pl_pigment_2880_m_ink,
  &standard_4pl_pigment_2880_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_2880_photo2_inks =
{
  &photo_4pl_pigment_2880_k_ink,
  &photo_4pl_pigment_2880_c_ink,
  &photo_4pl_pigment_2880_m_ink,
  &standard_4pl_pigment_2880_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_2880_photoj_inks =
{
  &standard_4pl_pigment_2880_ink,
  &photo_4pl_pigment_2880_c_ink,
  &photo_4pl_pigment_2880_m_ink,
  &photo_4pl_pigment_2880_y_ink
};


static const escp2_variable_inkset_t escp2_3pl_pigment_standard_inks =
{
  &standard_3pl_pigment_ink,
  &standard_3pl_pigment_ink,
  &standard_3pl_pigment_ink,
  &standard_3pl_pigment_ink
};

static const escp2_variable_inkset_t piezo_3pl_pigment_quadtone_inks =
{
  &piezo_3pl_pigment_quadtone_ink
};

static const escp2_variable_inkset_t escp2_3pl_pigment_2880_standard_inks =
{
  &standard_3pl_pigment_2880_ink,
  &standard_3pl_pigment_2880_ink,
  &standard_3pl_pigment_2880_ink,
  &standard_3pl_pigment_2880_ink
};

static const escp2_variable_inkset_t piezo_3pl_pigment_2880_quadtone_inks =
{
  &piezo_3pl_pigment_2880_quadtone_ink
};


static const escp2_variable_inkset_t spro10000_standard_inks =
{
  &spro10000_standard_ink,
  &spro10000_standard_ink,
  &spro10000_standard_ink,
  &spro10000_standard_ink
};

static const escp2_variable_inkset_t spro10000_extended_inks =
{
  &spro10000_standard_ink,
  &spro10000_standard_ink,
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

const escp2_variable_inklist_t stpi_escp2_simple_inks =
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
  },
  { NULL, },
  { NULL, },
  {
    &piezo_quadtone_inks,
    &piezo_quadtone_inks,
    &piezo_quadtone_inks,
    &piezo_quadtone_inks,
    &piezo_quadtone_inks,
    &piezo_quadtone_inks,
    &piezo_quadtone_inks,
    &piezo_quadtone_inks,
  },
  {
    &extended_inks,
    &extended_inks,
    &extended_inks,
    &extended_inks,
    &extended_inks,
    &extended_inks,
    &extended_inks,
    &extended_inks,
  },
};

const escp2_variable_inklist_t stpi_escp2_variable_6pl_inks =
{
  {
    &escp2_6pl_standard_inks,
    &escp2_6pl_standard_inks,
    &escp2_6pl_standard_inks,
    &escp2_6pl_standard_inks,
    &escp2_6pl_1440_standard_inks,
    &escp2_6pl_2880_standard_inks,
    &escp2_6pl_2880_standard_inks,
    &escp2_6pl_2880_standard_inks
  },
  {
    &escp2_6pl_photo_inks,
    &escp2_6pl_photo_inks,
    &escp2_6pl_photo_inks,
    &escp2_6pl_photo_inks,
    &escp2_6pl_1440_photo_inks,
    &escp2_6pl_2880_photo_inks,
    &escp2_6pl_2880_photo_inks,
    &escp2_6pl_2880_photo_inks
  },
  { NULL, },
  { NULL, },
  {
    &piezo_6pl_quadtone_inks,
    &piezo_6pl_quadtone_inks,
    &piezo_6pl_quadtone_inks,
    &piezo_6pl_quadtone_inks,
    &piezo_6pl_1440_quadtone_inks,
    &piezo_6pl_2880_quadtone_inks,
    &piezo_6pl_2880_quadtone_inks,
    &piezo_6pl_2880_quadtone_inks
  },
  {
    &escp2_6pl_extended_inks,
    &escp2_6pl_extended_inks,
    &escp2_6pl_extended_inks,
    &escp2_6pl_extended_inks,
    &escp2_6pl_1440_extended_inks,
    &escp2_6pl_2880_extended_inks,
    &escp2_6pl_2880_extended_inks,
    &escp2_6pl_2880_extended_inks
  },
};

const escp2_variable_inklist_t stpi_escp2_variable_x80_6pl_inks =
{
  {
    &escp2_x80_multishot_standard_inks,
    &escp2_x80_multishot_standard_inks,
    &escp2_x80_multishot_standard_inks,
    &escp2_x80_6pl_standard_inks,
    &escp2_x80_1440_6pl_standard_inks,
    &escp2_x80_2880_6pl_standard_inks,
    &escp2_x80_2880_6pl_standard_inks,
    &escp2_x80_2880_6pl_standard_inks,
  },
  { NULL, },
  { NULL, },
  { NULL, },
  {
    &piezo_x80_multishot_quadtone_inks,
    &piezo_x80_multishot_quadtone_inks,
    &piezo_x80_multishot_quadtone_inks,
    &piezo_x80_6pl_quadtone_inks,
    &piezo_x80_1440_6pl_quadtone_inks,
    &piezo_x80_2880_6pl_quadtone_inks,
    &piezo_x80_2880_6pl_quadtone_inks,
    &piezo_x80_2880_6pl_quadtone_inks,
  },
  { NULL, },
};

const escp2_variable_inklist_t stpi_escp2_variable_4pl_inks =
{
  {
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
    &escp2_6pl_photo_inks,
    &escp2_4pl_photo_inks,
    &escp2_4pl_2880_photo_inks,
    &escp2_4pl_2880_photo_inks,
    &escp2_4pl_2880_photo_inks
  },
  { NULL, },
  { NULL, },
  {
    &piezo_multishot_quadtone_inks,
    &piezo_multishot_quadtone_inks,
    &piezo_multishot_quadtone_inks,
    &piezo_6pl_quadtone_inks,
    &piezo_4pl_quadtone_inks,
    &piezo_4pl_2880_quadtone_inks,
    &piezo_4pl_2880_quadtone_inks,
    &piezo_4pl_2880_quadtone_inks,
  },
 {
    &escp2_multishot_extended_inks,
    &escp2_multishot_extended_inks,
    &escp2_multishot_extended_inks,
    &escp2_6pl_extended_inks,
    &escp2_4pl_extended_inks,
    &escp2_4pl_2880_extended_inks,
    &escp2_4pl_2880_extended_inks,
    &escp2_4pl_2880_extended_inks,
  },
};

const escp2_variable_inklist_t stpi_escp2_variable_680_4pl_inks =
{
  {
    &escp2_680_multishot_standard_inks,
    &escp2_680_multishot_standard_inks,
    &escp2_680_multishot_standard_inks,
    &escp2_680_6pl_standard_inks,
    &escp2_4pl_standard_inks,
    &escp2_4pl_2880_standard_inks,
    &escp2_4pl_2880_standard_inks,
    &escp2_4pl_2880_standard_inks,
  },
  { NULL, },
  { NULL, },
  { NULL, },
  {
    &piezo_680_multishot_quadtone_inks,
    &piezo_680_multishot_quadtone_inks,
    &piezo_680_multishot_quadtone_inks,
    &piezo_680_6pl_quadtone_inks,
    &piezo_4pl_quadtone_inks,
    &piezo_4pl_2880_quadtone_inks,
    &piezo_4pl_2880_quadtone_inks,
    &piezo_4pl_2880_quadtone_inks,
  },
  {
    &escp2_680_multishot_standard_inks,
    &escp2_680_multishot_standard_inks,
    &escp2_680_multishot_standard_inks,
    &escp2_680_6pl_standard_inks,
    &escp2_4pl_standard_inks,
    &escp2_4pl_2880_standard_inks,
    &escp2_4pl_2880_standard_inks,
    &escp2_4pl_2880_standard_inks,
  },
};

const escp2_variable_inklist_t stpi_escp2_variable_3pl_inks =
{
  {
    &escp2_multishot_standard_inks,
    &escp2_multishot_standard_inks,
    &escp2_6pl_standard_980_inks,
    &escp2_3pl_standard_inks,
    &escp2_3pl_1440_standard_inks,
    &escp2_3pl_2880_standard_inks,
    &escp2_3pl_2880_standard_inks,
    &escp2_3pl_2880_standard_inks,
  },
  { NULL, },
  { NULL, },
  { NULL, },
  {
    &piezo_multishot_quadtone_inks,
    &piezo_multishot_quadtone_inks,
    &piezo_6pl_quadtone_980_inks,
    &piezo_3pl_quadtone_inks,
    &piezo_3pl_1440_quadtone_inks,
    &piezo_3pl_2880_quadtone_inks,
    &piezo_3pl_2880_quadtone_inks,
    &piezo_3pl_2880_quadtone_inks,
  },
  {
    &escp2_multishot_standard_inks,
    &escp2_multishot_standard_inks,
    &escp2_6pl_standard_980_inks,
    &escp2_3pl_standard_inks,
    &escp2_3pl_1440_standard_inks,
    &escp2_3pl_2880_standard_inks,
    &escp2_3pl_2880_standard_inks,
    &escp2_3pl_2880_standard_inks,
  },
};

const escp2_variable_inklist_t stpi_escp2_variable_2pl_inks =
{
  {
    &escp2_2pl_360_standard_inks,
    &escp2_2pl_360_standard_inks,
    &escp2_2pl_360_standard_inks,
    &escp2_2pl_720_standard_inks,
    &escp2_2pl_1440_standard_inks,
    &escp2_2pl_2880_standard_inks,
    &escp2_2pl_2880_standard_inks,
    &escp2_2pl_2880_standard_inks,
  },
  {
    &escp2_2pl_360_photo_inks,
    &escp2_2pl_360_photo_inks,
    &escp2_2pl_360_photo_inks,
    &escp2_2pl_720_photo_inks,
    &escp2_2pl_1440_photo_inks,
    &escp2_2pl_2880_photo_inks,
    &escp2_2pl_2880_photo_inks,
    &escp2_2pl_2880_photo_inks
  },
  {
    &escp2_2pl_360_photoj_inks,
    &escp2_2pl_360_photoj_inks,
    &escp2_2pl_360_photoj_inks,
    &escp2_2pl_720_photoj_inks,
    &escp2_2pl_1440_photoj_inks,
    &escp2_2pl_2880_photoj_inks,
    &escp2_2pl_2880_photoj_inks,
    &escp2_2pl_2880_photoj_inks
  },
  { NULL, },
  {
    &piezo_2pl_360_quadtone_inks,
    &piezo_2pl_360_quadtone_inks,
    &piezo_2pl_360_quadtone_inks,
    &piezo_2pl_720_quadtone_inks,
    &piezo_2pl_1440_quadtone_inks,
    &piezo_2pl_2880_quadtone_inks,
    &piezo_2pl_2880_quadtone_inks,
    &piezo_2pl_2880_quadtone_inks,
  },
  {
    &escp2_2pl_360_extended_inks,
    &escp2_2pl_360_extended_inks,
    &escp2_2pl_360_extended_inks,
    &escp2_2pl_720_extended_inks,
    &escp2_2pl_1440_extended_inks,
    &escp2_2pl_2880_extended_inks,
    &escp2_2pl_2880_extended_inks,
    &escp2_2pl_2880_extended_inks,
  },
};

const escp2_variable_inklist_t stpi_escp2_variable_pigment_inks =
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
  },
  { NULL, },
  { NULL, },
  {
    &piezo_pigment_quadtone_inks,
    &piezo_pigment_quadtone_inks,
    &piezo_pigment_quadtone_inks,
    &piezo_pigment_quadtone_inks,
    &piezo_pigment_quadtone_inks,
    &piezo_pigment_quadtone_inks,
    &piezo_pigment_quadtone_inks,
    &piezo_pigment_quadtone_inks
  },
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
};

const escp2_variable_inklist_t stpi_escp2_variable_4pl_pigment_inks =
{
  {
    &escp2_4pl_pigment_low_standard_inks,
    &escp2_4pl_pigment_low_standard_inks,
    &escp2_4pl_pigment_low_standard_inks,
    &escp2_4pl_pigment_standard_inks,
    &escp2_4pl_pigment_1440_standard_inks,
    &escp2_4pl_pigment_2880_standard_inks,
    &escp2_4pl_pigment_2880_standard_inks,
    &escp2_4pl_pigment_2880_standard_inks,
  },
  {
    &escp2_4pl_pigment_low_photo_inks,
    &escp2_4pl_pigment_low_photo_inks,
    &escp2_4pl_pigment_low_photo_inks,
    &escp2_4pl_pigment_photo_inks,
    &escp2_4pl_pigment_1440_photo_inks,
    &escp2_4pl_pigment_2880_photo_inks,
    &escp2_4pl_pigment_2880_photo_inks,
    &escp2_4pl_pigment_2880_photo_inks
  },
  { NULL, },
  {
    &escp2_4pl_pigment_low_photo2_inks,
    &escp2_4pl_pigment_low_photo2_inks,
    &escp2_4pl_pigment_low_photo2_inks,
    &escp2_4pl_pigment_photo2_inks,
    &escp2_4pl_pigment_1440_photo2_inks,
    &escp2_4pl_pigment_2880_photo2_inks,
    &escp2_4pl_pigment_2880_photo2_inks,
    &escp2_4pl_pigment_2880_photo2_inks
  },
  { NULL, },
  {
    &escp2_4pl_pigment_low_extended_inks,
    &escp2_4pl_pigment_low_extended_inks,
    &escp2_4pl_pigment_low_extended_inks,
    &escp2_4pl_pigment_extended_inks,
    &escp2_4pl_pigment_1440_extended_inks,
    &escp2_4pl_pigment_2880_extended_inks,
    &escp2_4pl_pigment_2880_extended_inks,
    &escp2_4pl_pigment_2880_extended_inks,
  },
};

const escp2_variable_inklist_t stpi_escp2_variable_3pl_pigment_inks =
{
  {
    &escp2_economy_pigment_standard_inks,
    &escp2_multishot_pigment_standard_inks,
    &escp2_multishot_pigment_standard_inks,
    &escp2_6pl_pigment_standard_inks,
    &escp2_3pl_pigment_standard_inks,
    &escp2_3pl_pigment_2880_standard_inks,
    &escp2_3pl_pigment_2880_standard_inks,
    &escp2_3pl_pigment_2880_standard_inks,
  },
  { NULL, },
  { NULL, },
  { NULL, },
  {
    &piezo_economy_pigment_quadtone_inks,
    &piezo_multishot_pigment_quadtone_inks,
    &piezo_multishot_pigment_quadtone_inks,
    &piezo_6pl_pigment_quadtone_inks,
    &piezo_3pl_pigment_quadtone_inks,
    &piezo_3pl_pigment_2880_quadtone_inks,
    &piezo_3pl_pigment_2880_quadtone_inks,
    &piezo_3pl_pigment_2880_quadtone_inks,
  },
  {
    &escp2_economy_pigment_standard_inks,
    &escp2_multishot_pigment_standard_inks,
    &escp2_multishot_pigment_standard_inks,
    &escp2_6pl_pigment_standard_inks,
    &escp2_3pl_pigment_standard_inks,
    &escp2_3pl_pigment_2880_standard_inks,
    &escp2_3pl_pigment_2880_standard_inks,
    &escp2_3pl_pigment_2880_standard_inks,
  },
};

const escp2_variable_inklist_t stpi_escp2_spro10000_inks =
{
  {
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
    &spro10000_photo_inks
  },
  { NULL, },
  { NULL, },
  { NULL, },
  {
    &spro10000_extended_inks,
    &spro10000_extended_inks,
    &spro10000_extended_inks,
    &spro10000_extended_inks,
    &spro10000_extended_inks,
    &spro10000_extended_inks,
    &spro10000_extended_inks,
    &spro10000_extended_inks
  },
};
