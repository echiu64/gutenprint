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

#define DECLARE_INK(name, density)					\
static const escp2_variable_ink_t name##_ink =				\
{									\
  density,								\
  name##_shades,							\
  sizeof(name##_shades) / sizeof(stpi_shade_t)				\
}

#define SHADE(density, name)					\
{  density, sizeof(name)/sizeof(stpi_dotsize_t), name  }


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

static const stpi_shade_t standard_shades[] =
{
  SHADE(1.0, single_dotsize)
};

DECLARE_INK(standard, 1.0);

static const stpi_shade_t photo_cyan_shades[] =
{
  SHADE(0.27, single_dotsize),
  SHADE(1.0, single_dotsize)
};

DECLARE_INK(photo_cyan, 1.0);

static const stpi_shade_t photo_magenta_shades[] =
{
  SHADE(0.35, single_dotsize),
  SHADE(1.0, single_dotsize)
};

DECLARE_INK(photo_magenta, 1.0);

static const stpi_shade_t photo2_yellow_shades[] =
{
  SHADE(0.35, single_dotsize),
  SHADE(1.0, single_dotsize)
};

DECLARE_INK(photo2_yellow, 1.0);

static const stpi_shade_t photo2_black_shades[] =
{
  SHADE(0.27, single_dotsize),
  SHADE(1.0, single_dotsize)
};

DECLARE_INK(photo2_black, 1.0);

static const stpi_shade_t piezo_quadtone_shades[] =
{
  SHADE(PIEZO_0, single_dotsize),
  SHADE(PIEZO_1, single_dotsize),
  SHADE(PIEZO_2, single_dotsize),
  SHADE(PIEZO_3, single_dotsize)
};
  

DECLARE_INK(piezo_quadtone, PIEZO_DENSITY);

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

static const stpi_shade_t standard_multishot_shades[] =
{
  SHADE(1.0, standard_multishot_dotsizes)
};

DECLARE_INK(standard_multishot, 1.0);

static const stpi_shade_t photo_multishot_shades[] =
{
  SHADE(0.26, standard_multishot_dotsizes),
  SHADE(1.0, standard_multishot_dotsizes)
};

DECLARE_INK(photo_multishot, 1.0);

static const stpi_shade_t photo_multishot_y_shades[] =
{
  SHADE(0.5, standard_multishot_dotsizes),
  SHADE(1.0, standard_multishot_dotsizes)
};

DECLARE_INK(photo_multishot_y, 1.0);

static const stpi_shade_t piezo_multishot_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_multishot_dotsizes),
  SHADE(PIEZO_1, standard_multishot_dotsizes),
  SHADE(PIEZO_2, standard_multishot_dotsizes),
  SHADE(PIEZO_3, standard_multishot_dotsizes)
};

DECLARE_INK(piezo_multishot_quadtone, PIEZO_DENSITY);

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

static const stpi_shade_t standard_6pl_shades[] =
{
  SHADE(1.0, standard_6pl_dotsizes)
};

DECLARE_INK(standard_6pl, 1.0);

static const stpi_shade_t standard_6pl_1440_shades[] =
{
  SHADE(1.0, standard_6pl_1440_dotsizes)
};

DECLARE_INK(standard_6pl_1440, 1.0);

static const stpi_shade_t standard_6pl_2880_shades[] =
{
  SHADE(1.0, standard_6pl_2880_dotsizes)
};

DECLARE_INK(standard_6pl_2880, 1.0);

static const stpi_shade_t photo_6pl_shades[] =
{
  SHADE(0.26, standard_6pl_dotsizes),
  SHADE(1.0, standard_6pl_dotsizes)
};

DECLARE_INK(photo_6pl, 1.0);

static const stpi_shade_t photo_6pl_y_shades[] =
{
  SHADE(0.25, standard_6pl_dotsizes),
  SHADE(1.0, standard_6pl_dotsizes)
};

DECLARE_INK(photo_6pl_y, 1.0);

static const stpi_shade_t photo_6pl_1440_shades[] =
{
  SHADE(0.26, standard_6pl_1440_dotsizes),
  SHADE(1.0, standard_6pl_1440_dotsizes)
};

DECLARE_INK(photo_6pl_1440, 1.0);

static const stpi_shade_t photo_6pl_2880_shades[] =
{
  SHADE(0.26, standard_6pl_2880_dotsizes),
  SHADE(1.0, standard_6pl_2880_dotsizes)
};

DECLARE_INK(photo_6pl_2880, 1.0);

static const stpi_shade_t piezo_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_6pl_dotsizes),
  SHADE(PIEZO_1, standard_6pl_dotsizes),
  SHADE(PIEZO_2, standard_6pl_dotsizes),
  SHADE(PIEZO_3, standard_6pl_dotsizes)
};

DECLARE_INK(piezo_6pl_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_6pl_1440_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_6pl_1440_dotsizes),
  SHADE(PIEZO_1, standard_6pl_1440_dotsizes),
  SHADE(PIEZO_2, standard_6pl_1440_dotsizes),
  SHADE(PIEZO_3, standard_6pl_1440_dotsizes)
};

DECLARE_INK(piezo_6pl_1440_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_6pl_2880_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_6pl_2880_dotsizes),
  SHADE(PIEZO_1, standard_6pl_2880_dotsizes),
  SHADE(PIEZO_2, standard_6pl_2880_dotsizes),
  SHADE(PIEZO_3, standard_6pl_2880_dotsizes)
};

DECLARE_INK(piezo_6pl_2880_quadtone, PIEZO_DENSITY);


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

static const stpi_shade_t standard_x80_multishot_shades[] =
{
  SHADE(1.0, standard_x80_multishot_dotsizes)
};

DECLARE_INK(standard_x80_multishot, 1.0);

static const stpi_shade_t standard_x80_6pl_shades[] =
{
  SHADE(1.0, standard_x80_6pl_dotsizes)
};

DECLARE_INK(standard_x80_6pl, 1.0);

static const stpi_shade_t standard_x80_1440_6pl_shades[] =
{
  SHADE(1.0, standard_x80_1440_6pl_dotsizes)
};

DECLARE_INK(standard_x80_1440_6pl, 1.0);

static const stpi_shade_t standard_x80_2880_6pl_shades[] =
{
  SHADE(1.0, standard_x80_2880_6pl_dotsizes)
};

DECLARE_INK(standard_x80_2880_6pl, 1.0);

static const stpi_shade_t piezo_x80_multishot_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_x80_multishot_dotsizes),
  SHADE(PIEZO_1, standard_x80_multishot_dotsizes),
  SHADE(PIEZO_2, standard_x80_multishot_dotsizes),
  SHADE(PIEZO_3, standard_x80_multishot_dotsizes)
};

DECLARE_INK(piezo_x80_multishot_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_x80_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_x80_6pl_dotsizes),
  SHADE(PIEZO_1, standard_x80_6pl_dotsizes),
  SHADE(PIEZO_2, standard_x80_6pl_dotsizes),
  SHADE(PIEZO_3, standard_x80_6pl_dotsizes)
};

DECLARE_INK(piezo_x80_6pl_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_x80_1440_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_x80_1440_6pl_dotsizes),
  SHADE(PIEZO_1, standard_x80_1440_6pl_dotsizes),
  SHADE(PIEZO_2, standard_x80_1440_6pl_dotsizes),
  SHADE(PIEZO_3, standard_x80_1440_6pl_dotsizes)
};

DECLARE_INK(piezo_x80_1440_6pl_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_x80_2880_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_x80_2880_6pl_dotsizes),
  SHADE(PIEZO_1, standard_x80_2880_6pl_dotsizes),
  SHADE(PIEZO_2, standard_x80_2880_6pl_dotsizes),
  SHADE(PIEZO_3, standard_x80_2880_6pl_dotsizes)
};

DECLARE_INK(piezo_x80_2880_6pl_quadtone, PIEZO_DENSITY);


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

static const stpi_shade_t standard_680_multishot_shades[] =
{
  SHADE(1.0, standard_680_multishot_dotsizes)
};

DECLARE_INK(standard_680_multishot, 1.0);

static const stpi_shade_t standard_680_6pl_shades[] =
{
  SHADE(1.0, standard_680_6pl_dotsizes)
};

DECLARE_INK(standard_680_6pl, 1.0);

static const stpi_shade_t piezo_680_multishot_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_680_multishot_dotsizes),
  SHADE(PIEZO_1, standard_680_multishot_dotsizes),
  SHADE(PIEZO_2, standard_680_multishot_dotsizes),
  SHADE(PIEZO_3, standard_680_multishot_dotsizes)
};

DECLARE_INK(piezo_680_multishot_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_680_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_680_6pl_dotsizes),
  SHADE(PIEZO_1, standard_680_6pl_dotsizes),
  SHADE(PIEZO_2, standard_680_6pl_dotsizes),
  SHADE(PIEZO_3, standard_680_6pl_dotsizes)
};

DECLARE_INK(piezo_680_6pl_quadtone, PIEZO_DENSITY);


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

static const stpi_shade_t standard_4pl_shades[] =
{
  SHADE(1.0, standard_4pl_dotsizes)
};

DECLARE_INK(standard_4pl, 1.0);

static const stpi_shade_t standard_4pl_2880_shades[] =
{
  SHADE(1.0, standard_4pl_2880_dotsizes)
};

DECLARE_INK(standard_4pl_2880, 1.0);

static const stpi_shade_t photo_4pl_shades[] =
{
  SHADE(0.26, standard_4pl_dotsizes),
  SHADE(1.0, standard_4pl_dotsizes)
};

DECLARE_INK(photo_4pl, 1.0);

static const stpi_shade_t photo_4pl_y_shades[] =
{
  SHADE(0.50, standard_4pl_dotsizes),
  SHADE(1.00, standard_4pl_dotsizes)
};

DECLARE_INK(photo_4pl_y, 1.0);

static const stpi_shade_t photo_4pl_2880_shades[] =
{
  SHADE(0.26, standard_4pl_2880_dotsizes),
  SHADE(1.00, standard_4pl_2880_dotsizes)
};

DECLARE_INK(photo_4pl_2880, 1.0);

static const stpi_shade_t photo_4pl_y_2880_shades[] =
{
  SHADE(0.5, standard_4pl_2880_dotsizes),
  SHADE(1.0, standard_4pl_2880_dotsizes)
};

DECLARE_INK(photo_4pl_y_2880, 1.0);

static const stpi_shade_t piezo_4pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_4pl_dotsizes),
  SHADE(PIEZO_1, standard_4pl_dotsizes),
  SHADE(PIEZO_2, standard_4pl_dotsizes),
  SHADE(PIEZO_3, standard_4pl_dotsizes)
};

DECLARE_INK(piezo_4pl_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_4pl_2880_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_4pl_2880_dotsizes),
  SHADE(PIEZO_1, standard_4pl_2880_dotsizes),
  SHADE(PIEZO_2, standard_4pl_2880_dotsizes),
  SHADE(PIEZO_3, standard_4pl_2880_dotsizes)
};

DECLARE_INK(piezo_4pl_2880_quadtone, PIEZO_DENSITY);


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

static const stpi_shade_t standard_3pl_shades[] =
{
  SHADE(1.0, standard_3pl_dotsizes)
};

DECLARE_INK(standard_3pl, 1.0);


static const stpi_shade_t standard_3pl_1440_shades[] =
{
  SHADE(1.0, standard_3pl_1440_dotsizes)
};

DECLARE_INK(standard_3pl_1440, 1.0);


static const stpi_shade_t standard_3pl_2880_shades[] =
{
  SHADE(1.0, standard_3pl_2880_dotsizes)
};

DECLARE_INK(standard_3pl_2880, 1.0);

static const stpi_shade_t standard_980_6pl_shades[] =
{
  SHADE(1.0, standard_980_6pl_dotsizes)
};

DECLARE_INK(standard_980_6pl, 1.0);

static const stpi_shade_t piezo_3pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_3pl_dotsizes),
  SHADE(PIEZO_1, standard_3pl_dotsizes),
  SHADE(PIEZO_2, standard_3pl_dotsizes),
  SHADE(PIEZO_3, standard_3pl_dotsizes)
};

DECLARE_INK(piezo_3pl_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_3pl_1440_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_3pl_1440_dotsizes),
  SHADE(PIEZO_1, standard_3pl_1440_dotsizes),
  SHADE(PIEZO_2, standard_3pl_1440_dotsizes),
  SHADE(PIEZO_3, standard_3pl_1440_dotsizes)
};

DECLARE_INK(piezo_3pl_1440_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_3pl_2880_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_3pl_2880_dotsizes),
  SHADE(PIEZO_1, standard_3pl_2880_dotsizes),
  SHADE(PIEZO_2, standard_3pl_2880_dotsizes),
  SHADE(PIEZO_3, standard_3pl_2880_dotsizes)
};

DECLARE_INK(piezo_3pl_2880_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_980_6pl_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_980_6pl_dotsizes),
  SHADE(PIEZO_1, standard_980_6pl_dotsizes),
  SHADE(PIEZO_2, standard_980_6pl_dotsizes),
  SHADE(PIEZO_3, standard_980_6pl_dotsizes)
};

DECLARE_INK(piezo_980_6pl_quadtone, PIEZO_DENSITY);


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

static const stpi_shade_t standard_2pl_2880_shades[] =
{
  SHADE(1.0, standard_2pl_2880_dotsizes)
};

DECLARE_INK(standard_2pl_2880, 1.0);

static const stpi_shade_t photo_2pl_2880_shades[] =
{
  SHADE(0.26, standard_2pl_2880_dotsizes),
  SHADE(1.0, standard_2pl_2880_dotsizes)
};

DECLARE_INK(photo_2pl_2880, 0.5);

static const stpi_shade_t photo_2pl_2880_c_shades[] =
{
  SHADE(0.26, standard_2pl_2880_dotsizes),
  SHADE(1.0, standard_2pl_2880_dotsizes)
};

DECLARE_INK(photo_2pl_2880_c, 0.5);

static const stpi_shade_t photo_2pl_2880_m_shades[] =
{
  SHADE(0.31, standard_2pl_2880_dotsizes),
  SHADE(1.0, standard_2pl_2880_dotsizes)
};

DECLARE_INK(photo_2pl_2880_m, 0.5);

static const stpi_shade_t photo_2pl_2880_y_shades[] =
{
  SHADE(0.5, standard_2pl_2880_dotsizes),
  SHADE(1.0, standard_2pl_2880_dotsizes)
};

DECLARE_INK(photo_2pl_2880_y, 1.00);

static const stpi_shade_t piezo_2pl_2880_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_2pl_2880_dotsizes),
  SHADE(PIEZO_1, standard_2pl_2880_dotsizes),
  SHADE(PIEZO_2, standard_2pl_2880_dotsizes),
  SHADE(PIEZO_3, standard_2pl_2880_dotsizes)
};

DECLARE_INK(piezo_2pl_2880_quadtone, PIEZO_DENSITY);

static const stpi_shade_t standard_2pl_1440_shades[] =
{
  SHADE(1.0, standard_2pl_1440_dotsizes)
};

DECLARE_INK(standard_2pl_1440, 1.0);

static const stpi_shade_t piezo_2pl_1440_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_2pl_1440_dotsizes),
  SHADE(PIEZO_1, standard_2pl_1440_dotsizes),
  SHADE(PIEZO_2, standard_2pl_1440_dotsizes),
  SHADE(PIEZO_3, standard_2pl_1440_dotsizes)
};

DECLARE_INK(piezo_2pl_1440_quadtone, PIEZO_DENSITY);

static const stpi_shade_t photo_2pl_1440_shades[] =
{
  SHADE(0.26, standard_2pl_1440_dotsizes),
  SHADE(1.00, standard_2pl_1440_dotsizes)
};

DECLARE_INK(photo_2pl_1440, 1.0);

static const stpi_shade_t photo_2pl_1440_y_shades[] =
{
  SHADE(0.50, standard_2pl_1440_dotsizes),
  SHADE(1.00, standard_2pl_1440_dotsizes)
};

DECLARE_INK(photo_2pl_1440_y, 1.0);

static const stpi_shade_t standard_2pl_720_shades[] =
{
  SHADE(1.0, standard_2pl_720_dotsizes)
};

DECLARE_INK(standard_2pl_720, 1.0);

static const stpi_shade_t photo_2pl_720_shades[] =
{
  SHADE(0.26, standard_2pl_720_dotsizes),
  SHADE(1.00, standard_2pl_720_dotsizes)
};

DECLARE_INK(photo_2pl_720, 1.0);

static const stpi_shade_t photo_2pl_720_y_shades[] =
{
  SHADE(0.5, standard_2pl_720_dotsizes),
  SHADE(1.0, standard_2pl_720_dotsizes)
};

DECLARE_INK(photo_2pl_720_y, 1.0);

static const stpi_shade_t piezo_2pl_720_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_2pl_720_dotsizes),
  SHADE(PIEZO_1, standard_2pl_720_dotsizes),
  SHADE(PIEZO_2, standard_2pl_720_dotsizes),
  SHADE(PIEZO_3, standard_2pl_720_dotsizes)
};

DECLARE_INK(piezo_2pl_720_quadtone, PIEZO_DENSITY);

static const stpi_shade_t standard_2pl_360_shades[] =
{
  SHADE(1.0, standard_2pl_360_dotsizes)
};

DECLARE_INK(standard_2pl_360, 1.0);

static const stpi_shade_t photo_2pl_360_shades[] =
{
  SHADE(0.26, standard_2pl_360_dotsizes),
  SHADE(1.0, standard_2pl_360_dotsizes)
};

DECLARE_INK(photo_2pl_360, 1.0);

static const stpi_shade_t photo_2pl_360_y_shades[] =
{
  SHADE(0.5, standard_2pl_360_dotsizes),
  SHADE(1.0, standard_2pl_360_dotsizes)
};

DECLARE_INK(photo_2pl_360_y, 1.0);

static const stpi_shade_t piezo_2pl_360_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_2pl_360_dotsizes),
  SHADE(PIEZO_1, standard_2pl_360_dotsizes),
  SHADE(PIEZO_2, standard_2pl_360_dotsizes),
  SHADE(PIEZO_3, standard_2pl_360_dotsizes)
};

DECLARE_INK(piezo_2pl_360_quadtone, PIEZO_DENSITY);


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

static const stpi_shade_t standard_economy_pigment_shades[] =
{
  SHADE(1.0, standard_economy_pigment_dotsizes)
};

DECLARE_INK(standard_economy_pigment, 1.0);

static const stpi_shade_t standard_multishot_pigment_shades[] =
{
  SHADE(1.0, standard_multishot_pigment_dotsizes)
};

DECLARE_INK(standard_multishot_pigment, 1.0);

static const stpi_shade_t standard_6pl_pigment_shades[] =
{
  SHADE(1.0, standard_6pl_pigment_dotsizes)
};

DECLARE_INK(standard_6pl_pigment, 1.0);

static const stpi_shade_t standard_3pl_pigment_shades[] =
{
  SHADE(1.0, standard_3pl_pigment_dotsizes)
};

DECLARE_INK(standard_3pl_pigment, 1.0);

static const stpi_shade_t standard_3pl_pigment_2880_shades[] =
{
  SHADE(1.0, standard_3pl_pigment_2880_dotsizes)
};

DECLARE_INK(standard_3pl_pigment_2880, 1.0);

static const stpi_shade_t piezo_economy_pigment_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_economy_pigment_dotsizes),
  SHADE(PIEZO_1, standard_economy_pigment_dotsizes),
  SHADE(PIEZO_2, standard_economy_pigment_dotsizes),
  SHADE(PIEZO_3, standard_economy_pigment_dotsizes)
};

DECLARE_INK(piezo_economy_pigment_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_multishot_pigment_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_multishot_pigment_dotsizes),
  SHADE(PIEZO_1, standard_multishot_pigment_dotsizes),
  SHADE(PIEZO_2, standard_multishot_pigment_dotsizes),
  SHADE(PIEZO_3, standard_multishot_pigment_dotsizes)
};

DECLARE_INK(piezo_multishot_pigment_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_6pl_pigment_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_6pl_pigment_dotsizes),
  SHADE(PIEZO_1, standard_6pl_pigment_dotsizes),
  SHADE(PIEZO_2, standard_6pl_pigment_dotsizes),
  SHADE(PIEZO_3, standard_6pl_pigment_dotsizes)
};

DECLARE_INK(piezo_6pl_pigment_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_3pl_pigment_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_3pl_pigment_dotsizes),
  SHADE(PIEZO_1, standard_3pl_pigment_dotsizes),
  SHADE(PIEZO_2, standard_3pl_pigment_dotsizes),
  SHADE(PIEZO_3, standard_3pl_pigment_dotsizes)
};

DECLARE_INK(piezo_3pl_pigment_quadtone, PIEZO_DENSITY);

static const stpi_shade_t piezo_3pl_pigment_2880_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_3pl_pigment_2880_dotsizes),
  SHADE(PIEZO_1, standard_3pl_pigment_2880_dotsizes),
  SHADE(PIEZO_2, standard_3pl_pigment_2880_dotsizes),
  SHADE(PIEZO_3, standard_3pl_pigment_2880_dotsizes)
};

DECLARE_INK(piezo_3pl_pigment_2880_quadtone, PIEZO_DENSITY);


/***************************************************************\
*                                                               *
*                   STYLUS PHOTO 2000P                          *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t standard_pigment_dotsizes[] =
{
  { 0x1, 0.55 },
  { 0x2, 1.00 }
};

static const stpi_shade_t standard_pigment_shades[] =
{
  SHADE(1.0, standard_pigment_dotsizes)
};

DECLARE_INK(standard_pigment, 1.0);

static const stpi_shade_t photo_pigment_shades[] =
{
  SHADE(0.227, standard_pigment_dotsizes),
  SHADE(1.0, standard_pigment_dotsizes)
};

DECLARE_INK(photo_pigment, 1.0);

static const stpi_shade_t piezo_pigment_quadtone_shades[] =
{
  SHADE(PIEZO_0, standard_pigment_dotsizes),
  SHADE(PIEZO_1, standard_pigment_dotsizes),
  SHADE(PIEZO_2, standard_pigment_dotsizes),
  SHADE(PIEZO_3, standard_pigment_dotsizes)
};

DECLARE_INK(piezo_pigment_quadtone, PIEZO_DENSITY);


/***************************************************************\
*                                                               *
*            ULTRACHROME (2100/2200, 7600, 9600)                *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t standard_4pl_pigment_low_dotsizes[] =
{
  { 0x1, 0.40 },
  { 0x2, 0.70 },
  { 0x3, 1.00 }
};

static const stpi_shade_t standard_4pl_pigment_low_shades[] =
{
  SHADE(1.0, standard_4pl_pigment_low_dotsizes)
};

DECLARE_INK(standard_4pl_pigment_low, 0.5);

static const stpi_shade_t photo_4pl_pigment_low_m_shades[] =
{
  SHADE(0.26, standard_4pl_pigment_low_dotsizes),
  SHADE(1.0, standard_4pl_pigment_low_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_low_m, 0.5);

static const stpi_shade_t photo_4pl_pigment_low_c_shades[] =
{
  SHADE(0.40, standard_4pl_pigment_low_dotsizes),
  SHADE(1.0, standard_4pl_pigment_low_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_low_c, 0.5);

static const stpi_shade_t photo_4pl_pigment_low_y_shades[] =
{
  SHADE(0.50, standard_4pl_pigment_low_dotsizes),
  SHADE(1.0, standard_4pl_pigment_low_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_low_y, 1.5);

static const stpi_shade_t photo_4pl_pigment_low_k_shades[] =
{
  SHADE(0.49, standard_4pl_pigment_low_dotsizes),
  SHADE(1.0, standard_4pl_pigment_low_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_low_k, 0.5);

static const stpi_dotsize_t standard_4pl_pigment_dotsizes[] =
{
  { 0x1, 0.28 },
  { 0x2, 0.50 },
  { 0x3, 1.00 }
};

static const stpi_shade_t standard_4pl_pigment_shades[] =
{
  SHADE(1.0, standard_4pl_pigment_dotsizes)
};

DECLARE_INK(standard_4pl_pigment, 1.0);

static const stpi_shade_t photo_4pl_pigment_m_shades[] =
{
  SHADE(0.26, standard_4pl_pigment_dotsizes),
  SHADE(1.0, standard_4pl_pigment_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_m, 1.0);

static const stpi_shade_t photo_4pl_pigment_c_shades[] =
{
  SHADE(0.40, standard_4pl_pigment_dotsizes),
  SHADE(1.0, standard_4pl_pigment_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_c, 1.0);

static const stpi_shade_t photo_4pl_pigment_y_shades[] =
{
  SHADE(0.50, standard_4pl_pigment_dotsizes),
  SHADE(1.0, standard_4pl_pigment_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_y, 1.5);

static const stpi_shade_t photo_4pl_pigment_k_shades[] =
{
  SHADE(0.48, standard_4pl_pigment_dotsizes),
  SHADE(1.0, standard_4pl_pigment_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_k, 0.75);

static const stpi_dotsize_t standard_4pl_pigment_1440_dotsizes[] =
{
  { 0x1, 0.56 },
  { 0x2, 1.00 },
};

static const stpi_shade_t standard_4pl_pigment_1440_shades[] =
{
  SHADE(1.0, standard_4pl_pigment_1440_dotsizes)
};

DECLARE_INK(standard_4pl_pigment_1440, 1.0);

static const stpi_shade_t photo_4pl_pigment_1440_m_shades[] =
{
  SHADE(0.26, standard_4pl_pigment_1440_dotsizes),
  SHADE(1.0, standard_4pl_pigment_1440_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_1440_m, 1.0);

static const stpi_shade_t photo_4pl_pigment_1440_c_shades[] =
{
  SHADE(0.40, standard_4pl_pigment_1440_dotsizes),
  SHADE(1.0, standard_4pl_pigment_1440_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_1440_c, 1.0);

static const stpi_shade_t photo_4pl_pigment_1440_y_shades[] =
{
  SHADE(0.50, standard_4pl_pigment_1440_dotsizes),
  SHADE(1.0, standard_4pl_pigment_1440_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_1440_y, 1.5);

static const stpi_shade_t photo_4pl_pigment_1440_k_shades[] =
{
  SHADE(0.56, standard_4pl_pigment_1440_dotsizes),
  SHADE(1.0, standard_4pl_pigment_1440_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_1440_k, 0.75);

static const stpi_dotsize_t standard_4pl_pigment_2880_dotsizes[] =
{
  { 0x1, 1.0 },
};

static const stpi_shade_t standard_4pl_pigment_2880_shades[] =
{
  SHADE(1.0, standard_4pl_pigment_2880_dotsizes)
};

DECLARE_INK(standard_4pl_pigment_2880, 1.0);

static const stpi_shade_t photo_4pl_pigment_2880_m_shades[] =
{
  SHADE(0.26, standard_4pl_pigment_2880_dotsizes),
  SHADE(1.0, standard_4pl_pigment_2880_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_2880_m, 0.75);

static const stpi_shade_t photo_4pl_pigment_2880_c_shades[] =
{
  SHADE(0.40, standard_4pl_pigment_2880_dotsizes),
  SHADE(1.0, standard_4pl_pigment_2880_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_2880_c, 0.75);

static const stpi_shade_t photo_4pl_pigment_2880_y_shades[] =
{
  SHADE(0.50, standard_4pl_pigment_2880_dotsizes),
  SHADE(1.0, standard_4pl_pigment_2880_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_2880_y, 1.5);

static const stpi_shade_t photo_4pl_pigment_2880_k_shades[] =
{
  SHADE(0.48, standard_4pl_pigment_2880_dotsizes),
  SHADE(1.0, standard_4pl_pigment_2880_dotsizes)
};

DECLARE_INK(photo_4pl_pigment_2880_k, 0.75);


/***************************************************************\
*                                                               *
*                      STYLUS PRO 10000                         *
*                                                               *
\***************************************************************/

static const stpi_dotsize_t spro10000_standard_dotsizes[] =
{
  { 0x1, 0.661 },
  { 0x2, 1.00 },
};

static const stpi_shade_t spro10000_standard_shades[] =
{
  SHADE(1.0, spro10000_standard_dotsizes)
};

DECLARE_INK(spro10000_standard, 1.0);

static const stpi_shade_t spro10000_photo_shades[] =
{
  SHADE(0.26, spro10000_standard_dotsizes),
  SHADE(1.0, spro10000_standard_dotsizes),
};

DECLARE_INK(spro10000_photo, 1.0);


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
