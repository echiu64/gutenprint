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

#define DECLARE_INK(name, shades, dotsizes, density)	\
static const escp2_variable_ink_t name##_ink =		\
{							\
  density,						\
  sizeof(shades##_shades) / sizeof(double),		\
  sizeof(dotsizes##_dotsizes) / sizeof(double),		\
  shades##_shades,					\
  dotsizes##_dotsizes					\
}

#define DOTSIZE(name) static const double name##_dotsizes[]

#define SHADES(name) static const double name##_shades[]


/* Single dot size printers */
DOTSIZE(single) = { 1.0 };

/* 6 pl printers */
DOTSIZE(standard_multishot) = { 0.28, 0.58, 1.0 };
DOTSIZE(standard_6pl) = { 0.25, 0.5, 1.0 };
DOTSIZE(standard_6pl_1440) = { 0.5, 1.0 };
DOTSIZE(standard_6pl_2880) = { 1.0 };

/* Stylus Color 480/580/C40/C50 */
DOTSIZE(standard_x80_multishot) = { 0.163, 0.5, 1.0 };
DOTSIZE(standard_x80_6pl) = { 0.325, 0.5, 1.0 };
DOTSIZE(standard_x80_1440_6pl) = { 0.65, 1.0 };
DOTSIZE(standard_x80_2880_6pl) = { 1.0 };

/* Stylus Color 777 */
DOTSIZE(standard_680_multishot) = { 0.375, 0.75, 1.0 };
DOTSIZE(standard_680_6pl) = { 0.50, 0.66, 1.0 };

/* All other 4 picolitre printers */
DOTSIZE(standard_4pl) = { 0.661, 1.0 };
DOTSIZE(standard_4pl_2880) = { 1.0 };

/* Stylus Color 900/980 */
DOTSIZE(standard_3pl) = { 0.25, 0.61, 1.0 };
DOTSIZE(standard_3pl_1440) = { 0.39, 1.0 };
DOTSIZE(standard_3pl_2880) = { 1.0 };
DOTSIZE(standard_980_6pl) = { 0.40, 0.675, 1.0 };

/* Stylus Photo 960 */
DOTSIZE(standard_2pl_360) = { 0.25, 0.5, 1.0 };
DOTSIZE(standard_2pl_720) = { 0.25, 0.5, 1.0 };
DOTSIZE(standard_2pl_1440) = { 0.5, 1.0 };
DOTSIZE(standard_2pl_2880) = { 1.0 };

/* Stylus C80 */
DOTSIZE(standard_economy_pigment) = { 0, 0, 1.0 };
DOTSIZE(standard_multishot_pigment) = { 0.41, 0, 1.0 };
DOTSIZE(standard_6pl_pigment) = { 0.30, 0, 1.0 };
DOTSIZE(standard_3pl_pigment) = { 0.65, 1.0 };
DOTSIZE(standard_3pl_pigment_2880) = { 1.0 };

/* Stylus Photo 2000P */
DOTSIZE(standard_pigment) = { 0.55, 1.0 };

/* Stylus Photo 2200, Stylus Pro 7600 */
DOTSIZE(standard_4pl_pigment_low) = { 0.4, 0.7, 1.0 };
DOTSIZE(standard_4pl_pigment) = { 0.265, 0.5, 1.0 };
DOTSIZE(standard_4pl_pigment_1440) = { 0.53, 1.0 };
DOTSIZE(standard_4pl_pigment_2880) = { 1.0 };

/* Stylus Pro 10000 */
DOTSIZE(spro10000_standard) = { 0.661, 1.0 };


SHADES(standard) = { 1.0 };
SHADES(photo_cyan) = { 0.305, 1.0 };
SHADES(photo_magenta) = { 0.315, 1.0 };
SHADES(photo2_yellow) = { 0.35, 1.0 };
SHADES(photo2_black) = { 0.27, 1.0 };
SHADES(piezo_quadtone) = { 0.25, 0.5, 0.75, 1.0 };

SHADES(photo) = { 0.26, 1.0 };
SHADES(photo_c) = { 0.26, 1.0 };
SHADES(photo_m) = { 0.31, 1.0 };
SHADES(photo_y) = { 0.5, 1.0 };

SHADES(esp960_k) = { 0.26, 1.0 };
SHADES(esp960_c) = { 0.32, 1.0 };
SHADES(esp960_m) = { 0.35, 1.0 };
SHADES(esp960_y) = { 0.5, 1.0 };

SHADES(photo_pigment_k) = { 0.48, 1.0 };
SHADES(photo_pigment_c) = { 0.38, 1.0 };
SHADES(photo_pigment_m) = { 0.31, 1.0 };
SHADES(photo_pigment_y) = { 0.5, 1.0 };

SHADES(photo_yellow) = { 0.5, 1.0 };

SHADES(photo_pigment) = { 0.227, 1.0 };

/* Single dot size */

DECLARE_INK(standard, standard, single, 1.0);
DECLARE_INK(photo_cyan, photo_cyan, single, 1.0);
DECLARE_INK(photo_magenta, photo_magenta, single, 1.0);
DECLARE_INK(photo2_yellow, photo2_yellow, single, 1.0);
DECLARE_INK(photo2_black, photo2_black, single, 1.0);
DECLARE_INK(piezo_quadtone, piezo_quadtone, single, 1.0);

/* Low resolution 4-6 pl printers */

DECLARE_INK(standard_multishot, standard, standard_multishot, 1.0);
DECLARE_INK(photo_multishot, photo, standard_multishot, 1.0);
DECLARE_INK(photo_multishot_y, photo_yellow, standard_multishot, 1.0);
DECLARE_INK(piezo_multishot_quadtone, piezo_quadtone, standard_multishot, 1.0);

/* 4-6 pl printers, 6 pl dots */

DECLARE_INK(standard_6pl, standard, standard_6pl, 1.0);
DECLARE_INK(standard_6pl_1440, standard, standard_6pl_1440, 1.0);
DECLARE_INK(standard_6pl_2880, standard, standard_6pl_2880, 1.0);
DECLARE_INK(photo_6pl, photo, standard_6pl, 1.0);
DECLARE_INK(photo_6pl_c, photo_c, standard_6pl, 1.0);
DECLARE_INK(photo_6pl_m, photo_m, standard_6pl, 1.0);
DECLARE_INK(photo_6pl_y, photo_y, standard_6pl, 1.0);
DECLARE_INK(photo_6pl_1440, photo, standard_6pl_1440, 1.0);
DECLARE_INK(photo_6pl_1440_c, photo_c, standard_6pl_1440, 1.0);
DECLARE_INK(photo_6pl_1440_m, photo_m, standard_6pl_1440, 1.0);
DECLARE_INK(photo_6pl_1440_y, photo_y, standard_6pl_1440, 1.0);
DECLARE_INK(photo_6pl_2880, photo, standard_6pl_2880, 1.0);
DECLARE_INK(photo_6pl_2880_c, photo_c, standard_6pl_2880, 1.0);
DECLARE_INK(photo_6pl_2880_m, photo_m, standard_6pl_2880, 1.0);
DECLARE_INK(photo_6pl_2880_y, photo_y, standard_6pl_2880, 1.0);
DECLARE_INK(piezo_6pl_quadtone, piezo_quadtone, standard_6pl, 1.0);
DECLARE_INK(piezo_6pl_1440_quadtone, piezo_quadtone, standard_6pl_1440, 1.0);
DECLARE_INK(piezo_6pl_2880_quadtone, piezo_quadtone, standard_6pl_2880, 1.0);

/* Stylus Color 480/580/C40/C50 */

DECLARE_INK(standard_x80_multishot, standard, standard_x80_multishot, 1.0);
DECLARE_INK(standard_x80_6pl, standard, standard_x80_6pl, 1.0);
DECLARE_INK(standard_x80_1440_6pl, standard, standard_x80_1440_6pl, 1.0);
DECLARE_INK(standard_x80_2880_6pl, standard, standard_x80_2880_6pl, 1.0);
DECLARE_INK(piezo_x80_multishot_quadtone, standard, standard_x80_multishot, 1.0);
DECLARE_INK(piezo_x80_6pl_quadtone, standard, standard_x80_6pl, 1.0);
DECLARE_INK(piezo_x80_1440_6pl_quadtone, standard, standard_x80_1440_6pl, 1.0);
DECLARE_INK(piezo_x80_2880_6pl_quadtone, standard, standard_x80_2880_6pl, 1.0);

/* Stylus Color 777 */

DECLARE_INK(standard_680_multishot, standard, standard_680_multishot, 1.0);
DECLARE_INK(standard_680_6pl, standard, standard_680_6pl, 1.0);
DECLARE_INK(piezo_680_multishot_quadtone, standard, standard_680_multishot, 1.0);
DECLARE_INK(piezo_680_6pl_quadtone, standard, standard_680_6pl, 1.0);


/* All other 4 picolitre printers */

DECLARE_INK(standard_4pl, standard, standard_4pl, 1.0);
DECLARE_INK(standard_4pl_2880, standard, standard_4pl_2880, 1.0);
DECLARE_INK(photo_4pl, photo, standard_4pl, 1.0);
DECLARE_INK(photo_4pl_c, photo_c, standard_4pl, 1.0);
DECLARE_INK(photo_4pl_m, photo_m, standard_4pl, 1.0);
DECLARE_INK(photo_4pl_y, photo_y, standard_4pl, 1.0);
DECLARE_INK(photo_4pl_2880, photo, standard_4pl_2880, 1.0);
DECLARE_INK(photo_4pl_2880_c, photo_c, standard_4pl_2880, 1.0);
DECLARE_INK(photo_4pl_2880_m, photo_m, standard_4pl_2880, 1.0);
DECLARE_INK(photo_4pl_2880_y, photo_y, standard_4pl_2880, 1.0);
DECLARE_INK(piezo_4pl_quadtone, piezo_quadtone, standard_4pl, 1.0);
DECLARE_INK(piezo_4pl_2880_quadtone, piezo_quadtone, standard_4pl_2880, 1.0);


/* Stylus Color 900/980 */

DECLARE_INK(standard_3pl, standard, standard_3pl, 1.0);
DECLARE_INK(standard_3pl_1440, standard, standard_3pl_1440, 1.0);
DECLARE_INK(standard_3pl_2880, standard, standard_3pl_2880, 1.0);
DECLARE_INK(piezo_3pl_quadtone, piezo_quadtone, standard_3pl, 1.0);
DECLARE_INK(piezo_3pl_1440_quadtone, piezo_quadtone, standard_3pl_1440, 1.0);
DECLARE_INK(piezo_3pl_2880_quadtone, piezo_quadtone, standard_3pl_2880, 1.0);
DECLARE_INK(standard_980_6pl, standard, standard_980_6pl, 1.0);
DECLARE_INK(piezo_980_6pl_quadtone, standard, standard_980_6pl, 1.0);


/* Stylus Photo 960, PM-970C */

DECLARE_INK(standard_2pl_2880, standard, standard_2pl_2880, 1.0);
DECLARE_INK(photo_2pl_2880, esp960_k, standard_2pl_2880, 1.0);
DECLARE_INK(photo_2pl_2880_c, esp960_c, standard_2pl_2880, 1.0);
DECLARE_INK(photo_2pl_2880_m, esp960_m, standard_2pl_2880, 1.0);
DECLARE_INK(photo_2pl_2880_y, esp960_y, standard_2pl_2880, 1.0);
DECLARE_INK(piezo_2pl_2880_quadtone, piezo_quadtone, standard_2pl_2880, 1.0);
DECLARE_INK(standard_2pl_1440, standard, standard_2pl_1440, 1.0);
DECLARE_INK(photo_2pl_1440, esp960_k, standard_2pl_1440, 1.0);
DECLARE_INK(photo_2pl_1440_c, esp960_c, standard_2pl_1440, 1.0);
DECLARE_INK(photo_2pl_1440_m, esp960_m, standard_2pl_1440, 1.0);
DECLARE_INK(photo_2pl_1440_y, esp960_y, standard_2pl_1440, 1.0);
DECLARE_INK(piezo_2pl_1440_quadtone, piezo_quadtone, standard_2pl_1440, 1.0);
DECLARE_INK(standard_2pl_720, standard, standard_2pl_720, 1.0);
DECLARE_INK(photo_2pl_720, esp960_k, standard_2pl_720, 1.0);
DECLARE_INK(photo_2pl_720_c, esp960_c, standard_2pl_720, 1.0);
DECLARE_INK(photo_2pl_720_m, esp960_m, standard_2pl_720, 1.0);
DECLARE_INK(photo_2pl_720_y, esp960_y, standard_2pl_720, 1.0);
DECLARE_INK(piezo_2pl_720_quadtone, piezo_quadtone, standard_2pl_720, 1.0);
DECLARE_INK(standard_2pl_360, standard, standard_2pl_360, 1.0);
DECLARE_INK(photo_2pl_360, esp960_k, standard_2pl_360, 1.0);
DECLARE_INK(photo_2pl_360_c, esp960_c, standard_2pl_360, 1.0);
DECLARE_INK(photo_2pl_360_m, esp960_m, standard_2pl_360, 1.0);
DECLARE_INK(photo_2pl_360_y, esp960_y, standard_2pl_360, 1.0);
DECLARE_INK(piezo_2pl_360_quadtone, piezo_quadtone, standard_2pl_360, 1.0);


/* Stylus C80 */

DECLARE_INK(standard_economy_pigment, standard, standard_economy_pigment, 1.0);
DECLARE_INK(standard_multishot_pigment, standard, standard_multishot_pigment, 1.0);
DECLARE_INK(standard_6pl_pigment, standard, standard_6pl_pigment, 1.0);
DECLARE_INK(standard_3pl_pigment, standard, standard_3pl_pigment, 1.0);
DECLARE_INK(standard_3pl_pigment_2880, standard, standard_3pl_pigment_2880, 1.0);
DECLARE_INK(piezo_economy_pigment_quadtone, piezo_quadtone, standard_economy_pigment, 1.0);
DECLARE_INK(piezo_multishot_pigment_quadtone, piezo_quadtone, standard_multishot_pigment, 1.0);
DECLARE_INK(piezo_6pl_pigment_quadtone, piezo_quadtone, standard_6pl_pigment, 1.0);
DECLARE_INK(piezo_3pl_pigment_quadtone, piezo_quadtone, standard_3pl_pigment, 1.0);
DECLARE_INK(piezo_3pl_pigment_2880_quadtone, piezo_quadtone, standard_3pl_pigment_2880, 1.0);


/* Stylus Photo 2000P */

DECLARE_INK(standard_pigment, standard, standard_pigment, 1.0);
DECLARE_INK(photo_pigment, photo_pigment, standard_pigment, 1.0);
DECLARE_INK(piezo_pigment_quadtone, piezo_quadtone, standard_pigment, 1.0);


/* Ultrachrome (Stylus Photo 2200, Stylus Pro 7600/9600) */

DECLARE_INK(standard_4pl_pigment_low, standard, standard_4pl_pigment_low, 1.0);
DECLARE_INK(photo_4pl_pigment_low_m, photo_pigment_m, standard_4pl_pigment_low, 1.0);
DECLARE_INK(photo_4pl_pigment_low_c, photo_pigment_c, standard_4pl_pigment_low, 1.0);
DECLARE_INK(photo_4pl_pigment_low_y, photo_pigment_y, standard_4pl_pigment_low, 1.0);
DECLARE_INK(photo_4pl_pigment_low_k, photo_pigment_k, standard_4pl_pigment_low, 1.0);

DECLARE_INK(standard_4pl_pigment, standard, standard_4pl_pigment, 1.0);
DECLARE_INK(photo_4pl_pigment_m, photo_pigment_m, standard_4pl_pigment, 1.0);
DECLARE_INK(photo_4pl_pigment_c, photo_pigment_c, standard_4pl_pigment, 1.0);
DECLARE_INK(photo_4pl_pigment_y, photo_pigment_y, standard_4pl_pigment, 1.0);
DECLARE_INK(photo_4pl_pigment_k, photo_pigment_k, standard_4pl_pigment, 1.0);

DECLARE_INK(standard_4pl_pigment_1440, standard, standard_4pl_pigment_1440, 1.0);
DECLARE_INK(photo_4pl_pigment_1440_m, photo_pigment_m, standard_4pl_pigment_1440, 1.0);
DECLARE_INK(photo_4pl_pigment_1440_c, photo_pigment_c, standard_4pl_pigment_1440, 1.0);
DECLARE_INK(photo_4pl_pigment_1440_y, photo_pigment_y, standard_4pl_pigment_1440, 1.0);
DECLARE_INK(photo_4pl_pigment_1440_k, photo_pigment_k, standard_4pl_pigment_1440, 1.0);

DECLARE_INK(standard_4pl_pigment_2880, standard, standard_4pl_pigment_2880, 1.0);
DECLARE_INK(photo_4pl_pigment_2880_m, photo_pigment_m, standard_4pl_pigment_2880, 1.0);
DECLARE_INK(photo_4pl_pigment_2880_c, photo_pigment_c, standard_4pl_pigment_2880, 1.0);
DECLARE_INK(photo_4pl_pigment_2880_y, photo_pigment_y, standard_4pl_pigment_2880, 1.0);
DECLARE_INK(photo_4pl_pigment_2880_k, photo_pigment_k, standard_4pl_pigment_2880, 1.0);


/* Stylus Pro 10000 */

DECLARE_INK(spro10000_standard, standard, spro10000_standard, 1.0);
DECLARE_INK(spro10000_photo, photo, spro10000_standard, 1.0);


static const escp2_variable_inkset_t standard_inks =
{
  &standard_ink,
  &standard_ink,
  &standard_ink,
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


static const escp2_variable_inkset_t piezo_quadtone_inks =
{
  &piezo_quadtone_ink,
};

static const escp2_variable_inkset_t escp2_multishot_standard_inks =
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
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink,
  &standard_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_photo_inks =
{
  &standard_6pl_ink,
  &photo_6pl_c_ink,
  &photo_6pl_m_ink,
  &standard_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_photo2_inks =
{
  &photo_6pl_ink,
  &photo_6pl_c_ink,
  &photo_6pl_m_ink,
  &standard_6pl_ink
};

static const escp2_variable_inkset_t escp2_6pl_photoj_inks =
{
  &standard_6pl_ink,
  &photo_6pl_c_ink,
  &photo_6pl_m_ink,
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
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink,
  &standard_6pl_2880_ink
};

static const escp2_variable_inkset_t escp2_6pl_2880_photo_inks =
{
  &standard_6pl_2880_ink,
  &photo_6pl_2880_c_ink,
  &photo_6pl_2880_m_ink,
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
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink,
  &standard_4pl_ink
};

static const escp2_variable_inkset_t escp2_4pl_photo_inks =
{
  &standard_4pl_ink,
  &photo_4pl_c_ink,
  &photo_4pl_m_ink,
  &standard_4pl_ink
};

static const escp2_variable_inkset_t escp2_4pl_photoj_inks =
{
  &standard_4pl_ink,
  &photo_4pl_c_ink,
  &photo_4pl_m_ink,
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
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink,
  &standard_4pl_2880_ink
};

static const escp2_variable_inkset_t escp2_4pl_2880_photo_inks =
{
  &standard_4pl_2880_ink,
  &photo_4pl_2880_c_ink,
  &photo_4pl_2880_m_ink,
  &standard_4pl_2880_ink
};

static const escp2_variable_inkset_t escp2_4pl_2880_photoj_inks =
{
  &standard_4pl_2880_ink,
  &photo_4pl_2880_c_ink,
  &photo_4pl_2880_m_ink,
  &photo_4pl_2880_y_ink
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
    &standard_inks,
    &standard_inks,
    &standard_inks,
    &standard_inks,
    &standard_inks,
    &standard_inks,
    &standard_inks,
    &standard_inks,
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
    &escp2_6pl_standard_inks,
    &escp2_6pl_standard_inks,
    &escp2_6pl_standard_inks,
    &escp2_6pl_standard_inks,
    &escp2_6pl_1440_standard_inks,
    &escp2_6pl_2880_standard_inks,
    &escp2_6pl_2880_standard_inks,
    &escp2_6pl_2880_standard_inks
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
    &escp2_multishot_standard_inks,
    &escp2_multishot_standard_inks,
    &escp2_multishot_standard_inks,
    &escp2_6pl_standard_inks,
    &escp2_4pl_standard_inks,
    &escp2_4pl_2880_standard_inks,
    &escp2_4pl_2880_standard_inks,
    &escp2_4pl_2880_standard_inks,
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
    &escp2_2pl_360_standard_inks,
    &escp2_2pl_360_standard_inks,
    &escp2_2pl_360_standard_inks,
    &escp2_2pl_720_standard_inks,
    &escp2_2pl_1440_standard_inks,
    &escp2_2pl_2880_standard_inks,
    &escp2_2pl_2880_standard_inks,
    &escp2_2pl_2880_standard_inks,
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
    &escp2_4pl_pigment_low_standard_inks,
    &escp2_4pl_pigment_low_standard_inks,
    &escp2_4pl_pigment_low_standard_inks,
    &escp2_4pl_pigment_standard_inks,
    &escp2_4pl_pigment_1440_standard_inks,
    &escp2_4pl_pigment_2880_standard_inks,
    &escp2_4pl_pigment_2880_standard_inks,
    &escp2_4pl_pigment_2880_standard_inks,
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
    &spro10000_standard_inks,
    &spro10000_standard_inks,
    &spro10000_standard_inks,
    &spro10000_standard_inks,
    &spro10000_standard_inks,
    &spro10000_standard_inks,
    &spro10000_standard_inks,
    &spro10000_standard_inks
  },
};
