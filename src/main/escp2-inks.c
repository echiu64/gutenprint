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
  #name,						\
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
DOTSIZE(standard_low) = { 0.28, 0.58, 1.0 };
DOTSIZE(standard_6pl) = { 0.25, 0.5, 1.0 };
DOTSIZE(standard_6pl_1440) = { 0.5, 1.0 };
DOTSIZE(standard_6pl_2880) = { 1.0 };

/* Stylus Color 480/580/C40/C50 */
DOTSIZE(standard_x80_low) = { 0.163, 0.5, 1.0 };
DOTSIZE(standard_x80_6pl) = { 0.325, 0.5, 1.0 };
DOTSIZE(standard_x80_1440_6pl) = { 0.65, 1.0 };
DOTSIZE(standard_x80_2880_6pl) = { 1.0 };

/* Stylus Color 777 */
DOTSIZE(standard_680_low) = { 0.375, 0.75, 1.0 };
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
DOTSIZE(standard_low_pigment) = { 0.41, 0, 1.0 };
DOTSIZE(standard_6pl_pigment) = { 0.30, 0, 1.0 };
DOTSIZE(standard_3pl_pigment) = { 0.65, 1.0 };
DOTSIZE(standard_3pl_pigment_2880) = { 1.0 };

/* Stylus Photo 2000P */
DOTSIZE(standard_pigment) = { 0.55, 1.0 };

/* Stylus Photo 2200, Stylus Pro 7600 */
DOTSIZE(ultrachrome_low) = { 0.4, 0.7, 1.0 };
DOTSIZE(ultrachrome_720) = { 0.25, 0.5, 1.0 };
DOTSIZE(ultrachrome_1440) = { 0.5, 1.0 };
DOTSIZE(ultrachrome_2880) = { 1.0 };

/* Stylus Pro 10000 */
DOTSIZE(spro10000_standard) = { 0.661, 1.0 };


SHADES(standard) = { 1.0 };

SHADES(photo_cyan) = { 1.0, 0.305 };
SHADES(photo_magenta) = { 1.0, 0.315 };

SHADES(quadtone) = { 1.0, 0.75, 0.5, 0.25 };

SHADES(photo_c) = { 1.0, 0.26 };
SHADES(photo_m) = { 1.0, 0.31 };

SHADES(esp960_c) = { 1.0, 0.32 };
SHADES(esp960_m) = { 1.0, 0.35 };
SHADES(esp960_y) = { 1.0, 0.5 };

SHADES(ultra_photo_k) = { 1.0, 0.48 };
SHADES(ultra_photo_c) = { 1.0, 0.38 };
SHADES(ultra_photo_m) = { 1.0, 0.31 };

SHADES(photo_pigment) = { 1.0, 0.227 };

/* Single dot size */

DECLARE_INK(standard, standard, single, 1.0);

DECLARE_INK(photo_cyan, photo_cyan, single, 1.0);
DECLARE_INK(photo_magenta, photo_magenta, single, 1.0);

DECLARE_INK(quadtone, quadtone, single, 1.0);

/* Low resolution 4-6 pl printers */

DECLARE_INK(standard_low, standard, standard_low, 1.0);

DECLARE_INK(photo_low_c, photo_c, standard_low, 1.0);
DECLARE_INK(photo_low_m, photo_m, standard_low, 1.0);

DECLARE_INK(quadtone_low, quadtone, standard_low, 1.0);

/* 4-6 pl printers, 6 pl dots */

DECLARE_INK(standard_6pl, standard, standard_6pl, 1.0);
DECLARE_INK(standard_6pl_1440, standard, standard_6pl_1440, 1.0);
DECLARE_INK(standard_6pl_2880, standard, standard_6pl_2880, 1.0);

DECLARE_INK(photo_6pl_c, photo_c, standard_6pl, 1.0);
DECLARE_INK(photo_6pl_m, photo_m, standard_6pl, 1.0);

DECLARE_INK(photo_6pl_1440_c, photo_c, standard_6pl_1440, 1.0);
DECLARE_INK(photo_6pl_1440_m, photo_m, standard_6pl_1440, 1.0);

DECLARE_INK(photo_6pl_2880_c, photo_c, standard_6pl_2880, 1.0);
DECLARE_INK(photo_6pl_2880_m, photo_m, standard_6pl_2880, 1.0);

DECLARE_INK(quadtone_6pl, quadtone, standard_6pl, 1.0);
DECLARE_INK(quadtone_6pl_1440, quadtone, standard_6pl_1440, 1.0);
DECLARE_INK(quadtone_6pl_2880, quadtone, standard_6pl_2880, 1.0);

/* Stylus Color 480/580/C40/C50 */

DECLARE_INK(standard_x80_low, standard, standard_x80_low, 1.0);
DECLARE_INK(standard_x80_6pl, standard, standard_x80_6pl, 1.0);
DECLARE_INK(standard_x80_1440_6pl, standard, standard_x80_1440_6pl, 1.0);
DECLARE_INK(standard_x80_2880_6pl, standard, standard_x80_2880_6pl, 1.0);

DECLARE_INK(quadtone_x80_low, standard, standard_x80_low, 1.0);
DECLARE_INK(quadtone_x80_6pl, standard, standard_x80_6pl, 1.0);
DECLARE_INK(quadtone_x80_1440_6pl, standard, standard_x80_1440_6pl, 1.0);
DECLARE_INK(quadtone_x80_2880_6pl, standard, standard_x80_2880_6pl, 1.0);

/* Stylus Color 777 */

DECLARE_INK(standard_680_low, standard, standard_680_low, 1.0);
DECLARE_INK(standard_680_6pl, standard, standard_680_6pl, 1.0);

DECLARE_INK(quadtone_680_low, standard, standard_680_low, 1.0);
DECLARE_INK(quadtone_680_6pl, standard, standard_680_6pl, 1.0);


/* All other 4 picolitre printers */

DECLARE_INK(standard_4pl, standard, standard_4pl, 1.0);
DECLARE_INK(standard_4pl_2880, standard, standard_4pl_2880, 1.0);

DECLARE_INK(photo_4pl_c, photo_c, standard_4pl, 1.0);
DECLARE_INK(photo_4pl_m, photo_m, standard_4pl, 1.0);

DECLARE_INK(photo_4pl_2880_c, photo_c, standard_4pl_2880, 1.0);
DECLARE_INK(photo_4pl_2880_m, photo_m, standard_4pl_2880, 1.0);

DECLARE_INK(quadtone_4pl, quadtone, standard_4pl, 1.0);
DECLARE_INK(quadtone_4pl_2880, quadtone, standard_4pl_2880, 1.0);


/* Stylus Color 900/980 */

DECLARE_INK(standard_3pl, standard, standard_3pl, 1.0);
DECLARE_INK(standard_3pl_1440, standard, standard_3pl_1440, 1.0);
DECLARE_INK(standard_3pl_2880, standard, standard_3pl_2880, 1.0);
DECLARE_INK(standard_980_6pl, standard, standard_980_6pl, 1.0);

DECLARE_INK(quadtone_3pl, quadtone, standard_3pl, 1.0);
DECLARE_INK(quadtone_3pl_1440, quadtone, standard_3pl_1440, 1.0);
DECLARE_INK(quadtone_3pl_2880, quadtone, standard_3pl_2880, 1.0);
DECLARE_INK(quadtone_980_6pl, standard, standard_980_6pl, 1.0);


/* Stylus Photo 960, PM-970C */

DECLARE_INK(standard_2pl_360, standard, standard_2pl_360, 1.0);
DECLARE_INK(standard_2pl_720, standard, standard_2pl_720, 1.0);
DECLARE_INK(standard_2pl_1440, standard, standard_2pl_1440, 1.0);
DECLARE_INK(standard_2pl_2880, standard, standard_2pl_2880, 1.0);

DECLARE_INK(photo_2pl_360_c, esp960_c, standard_2pl_360, 1.0);
DECLARE_INK(photo_2pl_360_m, esp960_m, standard_2pl_360, 1.0);
DECLARE_INK(photo_2pl_360_y, esp960_y, standard_2pl_360, 1.0);

DECLARE_INK(photo_2pl_720_c, esp960_c, standard_2pl_720, 1.0);
DECLARE_INK(photo_2pl_720_m, esp960_m, standard_2pl_720, 1.0);
DECLARE_INK(photo_2pl_720_y, esp960_y, standard_2pl_720, 1.0);

DECLARE_INK(photo_2pl_1440_c, esp960_c, standard_2pl_1440, 1.0);
DECLARE_INK(photo_2pl_1440_m, esp960_m, standard_2pl_1440, 1.0);
DECLARE_INK(photo_2pl_1440_y, esp960_y, standard_2pl_1440, 1.0);

DECLARE_INK(photo_2pl_2880_c, esp960_c, standard_2pl_2880, 1.0);
DECLARE_INK(photo_2pl_2880_m, esp960_m, standard_2pl_2880, 1.0);
DECLARE_INK(photo_2pl_2880_y, esp960_y, standard_2pl_2880, 1.0);

DECLARE_INK(quadtone_2pl_360, quadtone, standard_2pl_360, 1.0);
DECLARE_INK(quadtone_2pl_720, quadtone, standard_2pl_720, 1.0);
DECLARE_INK(quadtone_2pl_1440, quadtone, standard_2pl_1440, 1.0);
DECLARE_INK(quadtone_2pl_2880, quadtone, standard_2pl_2880, 1.0);


/* Stylus C80 */

DECLARE_INK(standard_economy_pigment, standard, standard_economy_pigment, 1.0);
DECLARE_INK(standard_low_pigment, standard, standard_low_pigment, 1.0);
DECLARE_INK(standard_6pl_pigment, standard, standard_6pl_pigment, 1.0);
DECLARE_INK(standard_3pl_pigment, standard, standard_3pl_pigment, 1.0);
DECLARE_INK(standard_3pl_pigment_2880, standard, standard_3pl_pigment_2880, 1.0);
DECLARE_INK(quadtone_economy_pigment, quadtone, standard_economy_pigment, 1.0);
DECLARE_INK(quadtone_low_pigment, quadtone, standard_low_pigment, 1.0);
DECLARE_INK(quadtone_6pl_pigment, quadtone, standard_6pl_pigment, 1.0);
DECLARE_INK(quadtone_3pl_pigment, quadtone, standard_3pl_pigment, 1.0);
DECLARE_INK(quadtone_3pl_pigment_2880, quadtone, standard_3pl_pigment_2880, 1.0);


/* Stylus Photo 2000P */

DECLARE_INK(standard_pigment, standard, standard_pigment, 1.0);
DECLARE_INK(photo_pigment, photo_pigment, standard_pigment, 1.0);
DECLARE_INK(quadtone_pigment, quadtone, standard_pigment, 1.0);


/* Ultrachrome (Stylus Photo 2200, Stylus Pro 7600/9600) */

DECLARE_INK(photo_ultra_low_std, standard, ultrachrome_low, 1.0);
DECLARE_INK(photo_ultra_low_m, ultra_photo_m, ultrachrome_low, 1.0);
DECLARE_INK(photo_ultra_low_c, ultra_photo_c, ultrachrome_low, 1.0);
DECLARE_INK(photo_ultra_low_k, ultra_photo_k, ultrachrome_low, 1.0);

DECLARE_INK(photo_ultra_720_std, standard, ultrachrome_720, 1.0);
DECLARE_INK(photo_ultra_720_m, ultra_photo_m, ultrachrome_720, 1.0);
DECLARE_INK(photo_ultra_720_c, ultra_photo_c, ultrachrome_720, 1.0);
DECLARE_INK(photo_ultra_720_k, ultra_photo_k, ultrachrome_720, 1.0);

DECLARE_INK(photo_ultra_1440_std, standard, ultrachrome_1440, 1.0);
DECLARE_INK(photo_ultra_1440_m, ultra_photo_m, ultrachrome_1440, 1.0);
DECLARE_INK(photo_ultra_1440_c, ultra_photo_c, ultrachrome_1440, 1.0);
DECLARE_INK(photo_ultra_1440_k, ultra_photo_k, ultrachrome_1440, 1.0);

DECLARE_INK(photo_ultra_2880_std, standard, ultrachrome_2880, 1.0);
DECLARE_INK(photo_ultra_2880_m, ultra_photo_m, ultrachrome_2880, 1.0);
DECLARE_INK(photo_ultra_2880_c, ultra_photo_c, ultrachrome_2880, 1.0);
DECLARE_INK(photo_ultra_2880_k, ultra_photo_k, ultrachrome_2880, 1.0);


/* Stylus Pro 10000 */

DECLARE_INK(spro10000_standard, standard, spro10000_standard, 1.0);
DECLARE_INK(spro10000_photo_c, photo_cyan, spro10000_standard, 1.0);
DECLARE_INK(spro10000_photo_m, photo_magenta, spro10000_standard, 1.0);


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


static const escp2_variable_inkset_t quadtone_inks =
{
  &quadtone_ink,
};

static const escp2_variable_inkset_t escp2_low_standard_inks =
{
  &standard_low_ink,
  &standard_low_ink,
  &standard_low_ink,
  &standard_low_ink,
  &standard_low_ink,
  &standard_low_ink,
  &standard_low_ink
};

static const escp2_variable_inkset_t escp2_low_photo_inks =
{
  &standard_low_ink,
  &photo_low_c_ink,
  &photo_low_m_ink,
  &standard_low_ink
};

static const escp2_variable_inkset_t quadtone_low_inks =
{
  &quadtone_low_ink
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

static const escp2_variable_inkset_t quadtone_6pl_inks =
{
  &quadtone_6pl_ink
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
  &photo_6pl_1440_c_ink,
  &photo_6pl_1440_m_ink,
  &standard_6pl_1440_ink
};

static const escp2_variable_inkset_t quadtone_6pl_1440_inks =
{
  &quadtone_6pl_1440_ink
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

static const escp2_variable_inkset_t quadtone_6pl_2880_inks =
{
  &quadtone_6pl_2880_ink
};


static const escp2_variable_inkset_t escp2_680_low_standard_inks =
{
  &standard_680_low_ink,
  &standard_680_low_ink,
  &standard_680_low_ink,
  &standard_680_low_ink
};

static const escp2_variable_inkset_t escp2_680_6pl_standard_inks =
{
  &standard_680_6pl_ink,
  &standard_680_6pl_ink,
  &standard_680_6pl_ink,
  &standard_680_6pl_ink
};

static const escp2_variable_inkset_t quadtone_680_low_inks =
{
  &quadtone_680_low_ink
};

static const escp2_variable_inkset_t quadtone_680_6pl_inks =
{
  &quadtone_680_6pl_ink
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

static const escp2_variable_inkset_t quadtone_4pl_inks =
{
  &quadtone_4pl_ink
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

static const escp2_variable_inkset_t quadtone_4pl_2880_inks =
{
  &quadtone_4pl_2880_ink
};


static const escp2_variable_inkset_t escp2_6pl_standard_980_inks =
{
  &standard_980_6pl_ink,
  &standard_980_6pl_ink,
  &standard_980_6pl_ink,
  &standard_980_6pl_ink
};

static const escp2_variable_inkset_t quadtone_6pl_980_inks =
{
  &quadtone_980_6pl_ink
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

static const escp2_variable_inkset_t quadtone_3pl_inks =
{
  &quadtone_3pl_ink
};

static const escp2_variable_inkset_t quadtone_3pl_1440_inks =
{
  &quadtone_3pl_1440_ink
};

static const escp2_variable_inkset_t quadtone_3pl_2880_inks =
{
  &quadtone_3pl_2880_ink
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

static const escp2_variable_inkset_t escp2_2pl_2880_photoj_inks =
{
  &standard_2pl_2880_ink,
  &photo_2pl_2880_c_ink,
  &photo_2pl_2880_m_ink,
  &photo_2pl_2880_y_ink
};

static const escp2_variable_inkset_t quadtone_2pl_2880_inks =
{
  &quadtone_2pl_2880_ink
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
  &photo_2pl_1440_c_ink,
  &photo_2pl_1440_m_ink,
  &standard_2pl_1440_ink
};

static const escp2_variable_inkset_t escp2_2pl_1440_photoj_inks =
{
  &standard_2pl_1440_ink,
  &photo_2pl_1440_c_ink,
  &photo_2pl_1440_m_ink,
  &photo_2pl_1440_y_ink,
};

static const escp2_variable_inkset_t quadtone_2pl_1440_inks =
{
  &quadtone_2pl_1440_ink
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
  &photo_2pl_720_c_ink,
  &photo_2pl_720_m_ink,
  &standard_2pl_720_ink
};

static const escp2_variable_inkset_t escp2_2pl_720_photoj_inks =
{
  &standard_2pl_720_ink,
  &photo_2pl_720_c_ink,
  &photo_2pl_720_m_ink,
  &photo_2pl_720_y_ink
};

static const escp2_variable_inkset_t quadtone_2pl_720_inks =
{
  &quadtone_2pl_720_ink
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
  &photo_2pl_360_c_ink,
  &photo_2pl_360_m_ink,
  &standard_2pl_360_ink
};

static const escp2_variable_inkset_t escp2_2pl_360_photoj_inks =
{
  &standard_2pl_360_ink,
  &photo_2pl_360_c_ink,
  &photo_2pl_360_m_ink,
  &photo_2pl_360_y_ink
};

static const escp2_variable_inkset_t quadtone_2pl_360_inks =
{
  &quadtone_2pl_360_ink
};


static const escp2_variable_inkset_t escp2_x80_low_standard_inks =
{
  &standard_x80_low_ink,
  &standard_x80_low_ink,
  &standard_x80_low_ink,
  &standard_x80_low_ink
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

static const escp2_variable_inkset_t quadtone_x80_low_inks =
{
  &quadtone_x80_low_ink
};

static const escp2_variable_inkset_t quadtone_x80_6pl_inks =
{
  &quadtone_x80_6pl_ink
};

static const escp2_variable_inkset_t quadtone_x80_1440_6pl_inks =
{
  &quadtone_x80_1440_6pl_ink
};

static const escp2_variable_inkset_t quadtone_x80_2880_6pl_inks =
{
  &quadtone_x80_2880_6pl_ink
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

static const escp2_variable_inkset_t quadtone_pigment_inks =
{
  &quadtone_pigment_ink
};


static const escp2_variable_inkset_t escp2_low_pigment_standard_inks =
{
  &standard_low_pigment_ink,
  &standard_low_pigment_ink,
  &standard_low_pigment_ink,
  &standard_low_pigment_ink
};

static const escp2_variable_inkset_t quadtone_low_pigment_inks =
{
  &quadtone_low_pigment_ink
};

static const escp2_variable_inkset_t escp2_economy_pigment_standard_inks =
{
  &standard_economy_pigment_ink,
  &standard_economy_pigment_ink,
  &standard_economy_pigment_ink,
  &standard_economy_pigment_ink
};

static const escp2_variable_inkset_t quadtone_economy_pigment_inks =
{
  &quadtone_economy_pigment_ink
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

static const escp2_variable_inkset_t quadtone_6pl_pigment_inks =
{
  &quadtone_6pl_pigment_ink
};


static const escp2_variable_inkset_t escp2_4pl_pigment_low_standard_inks =
{
  &photo_ultra_low_std_ink,
  &photo_ultra_low_std_ink,
  &photo_ultra_low_std_ink,
  &photo_ultra_low_std_ink,
  &photo_ultra_low_std_ink,
  &photo_ultra_low_std_ink,
  &photo_ultra_low_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_low_photo_inks =
{
  &photo_ultra_low_std_ink,
  &photo_ultra_low_c_ink,
  &photo_ultra_low_m_ink,
  &photo_ultra_low_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_low_photo2_inks =
{
  &photo_ultra_low_k_ink,
  &photo_ultra_low_c_ink,
  &photo_ultra_low_m_ink,
  &photo_ultra_low_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_standard_inks =
{
  &photo_ultra_720_std_ink,
  &photo_ultra_720_std_ink,
  &photo_ultra_720_std_ink,
  &photo_ultra_720_std_ink,
  &photo_ultra_720_std_ink,
  &photo_ultra_720_std_ink,
  &photo_ultra_720_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_photo_inks =
{
  &photo_ultra_720_std_ink,
  &photo_ultra_720_c_ink,
  &photo_ultra_720_m_ink,
  &photo_ultra_720_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_photo2_inks =
{
  &photo_ultra_720_k_ink,
  &photo_ultra_720_c_ink,
  &photo_ultra_720_m_ink,
  &photo_ultra_720_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_1440_standard_inks =
{
  &photo_ultra_1440_std_ink,
  &photo_ultra_1440_std_ink,
  &photo_ultra_1440_std_ink,
  &photo_ultra_1440_std_ink,
  &photo_ultra_1440_std_ink,
  &photo_ultra_1440_std_ink,
  &photo_ultra_1440_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_1440_photo_inks =
{
  &photo_ultra_1440_std_ink,
  &photo_ultra_1440_c_ink,
  &photo_ultra_1440_m_ink,
  &photo_ultra_1440_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_1440_photo2_inks =
{
  &photo_ultra_1440_k_ink,
  &photo_ultra_1440_c_ink,
  &photo_ultra_1440_m_ink,
  &photo_ultra_1440_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_2880_standard_inks =
{
  &photo_ultra_2880_std_ink,
  &photo_ultra_2880_std_ink,
  &photo_ultra_2880_std_ink,
  &photo_ultra_2880_std_ink,
  &photo_ultra_2880_std_ink,
  &photo_ultra_2880_std_ink,
  &photo_ultra_2880_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_2880_photo_inks =
{
  &photo_ultra_2880_std_ink,
  &photo_ultra_2880_c_ink,
  &photo_ultra_2880_m_ink,
  &photo_ultra_2880_std_ink
};

static const escp2_variable_inkset_t escp2_4pl_pigment_2880_photo2_inks =
{
  &photo_ultra_2880_k_ink,
  &photo_ultra_2880_c_ink,
  &photo_ultra_2880_m_ink,
  &photo_ultra_2880_std_ink
};

static const escp2_variable_inkset_t escp2_3pl_pigment_standard_inks =
{
  &standard_3pl_pigment_ink,
  &standard_3pl_pigment_ink,
  &standard_3pl_pigment_ink,
  &standard_3pl_pigment_ink
};

static const escp2_variable_inkset_t quadtone_3pl_pigment_inks =
{
  &quadtone_3pl_pigment_ink
};

static const escp2_variable_inkset_t escp2_3pl_pigment_2880_standard_inks =
{
  &standard_3pl_pigment_2880_ink,
  &standard_3pl_pigment_2880_ink,
  &standard_3pl_pigment_2880_ink,
  &standard_3pl_pigment_2880_ink
};

static const escp2_variable_inkset_t quadtone_3pl_pigment_2880_inks =
{
  &quadtone_3pl_pigment_2880_ink
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
  &spro10000_photo_c_ink,
  &spro10000_photo_m_ink,
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
    &quadtone_inks,
    &quadtone_inks,
    &quadtone_inks,
    &quadtone_inks,
    &quadtone_inks,
    &quadtone_inks,
    &quadtone_inks,
    &quadtone_inks,
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
    &quadtone_6pl_inks,
    &quadtone_6pl_inks,
    &quadtone_6pl_inks,
    &quadtone_6pl_inks,
    &quadtone_6pl_1440_inks,
    &quadtone_6pl_2880_inks,
    &quadtone_6pl_2880_inks,
    &quadtone_6pl_2880_inks
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
    &escp2_x80_low_standard_inks,
    &escp2_x80_low_standard_inks,
    &escp2_x80_low_standard_inks,
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
    &quadtone_x80_low_inks,
    &quadtone_x80_low_inks,
    &quadtone_x80_low_inks,
    &quadtone_x80_6pl_inks,
    &quadtone_x80_1440_6pl_inks,
    &quadtone_x80_2880_6pl_inks,
    &quadtone_x80_2880_6pl_inks,
    &quadtone_x80_2880_6pl_inks,
  },
  { NULL, },
};

const escp2_variable_inklist_t stpi_escp2_variable_4pl_inks =
{
  {
    &escp2_low_standard_inks,
    &escp2_low_standard_inks,
    &escp2_low_standard_inks,
    &escp2_6pl_standard_inks,
    &escp2_4pl_standard_inks,
    &escp2_4pl_2880_standard_inks,
    &escp2_4pl_2880_standard_inks,
    &escp2_4pl_2880_standard_inks,
  },
  {
    &escp2_low_photo_inks,
    &escp2_low_photo_inks,
    &escp2_low_photo_inks,
    &escp2_6pl_photo_inks,
    &escp2_4pl_photo_inks,
    &escp2_4pl_2880_photo_inks,
    &escp2_4pl_2880_photo_inks,
    &escp2_4pl_2880_photo_inks
  },
  { NULL, },
  { NULL, },
  {
    &quadtone_low_inks,
    &quadtone_low_inks,
    &quadtone_low_inks,
    &quadtone_6pl_inks,
    &quadtone_4pl_inks,
    &quadtone_4pl_2880_inks,
    &quadtone_4pl_2880_inks,
    &quadtone_4pl_2880_inks,
  },
 {
    &escp2_low_standard_inks,
    &escp2_low_standard_inks,
    &escp2_low_standard_inks,
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
    &escp2_680_low_standard_inks,
    &escp2_680_low_standard_inks,
    &escp2_680_low_standard_inks,
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
    &quadtone_680_low_inks,
    &quadtone_680_low_inks,
    &quadtone_680_low_inks,
    &quadtone_680_6pl_inks,
    &quadtone_4pl_inks,
    &quadtone_4pl_2880_inks,
    &quadtone_4pl_2880_inks,
    &quadtone_4pl_2880_inks,
  },
  {
    &escp2_680_low_standard_inks,
    &escp2_680_low_standard_inks,
    &escp2_680_low_standard_inks,
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
    &escp2_low_standard_inks,
    &escp2_low_standard_inks,
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
    &quadtone_low_inks,
    &quadtone_low_inks,
    &quadtone_6pl_980_inks,
    &quadtone_3pl_inks,
    &quadtone_3pl_1440_inks,
    &quadtone_3pl_2880_inks,
    &quadtone_3pl_2880_inks,
    &quadtone_3pl_2880_inks,
  },
  {
    &escp2_low_standard_inks,
    &escp2_low_standard_inks,
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
    &quadtone_2pl_360_inks,
    &quadtone_2pl_360_inks,
    &quadtone_2pl_360_inks,
    &quadtone_2pl_720_inks,
    &quadtone_2pl_1440_inks,
    &quadtone_2pl_2880_inks,
    &quadtone_2pl_2880_inks,
    &quadtone_2pl_2880_inks,
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
    &quadtone_pigment_inks,
    &quadtone_pigment_inks,
    &quadtone_pigment_inks,
    &quadtone_pigment_inks,
    &quadtone_pigment_inks,
    &quadtone_pigment_inks,
    &quadtone_pigment_inks,
    &quadtone_pigment_inks
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
    &escp2_low_pigment_standard_inks,
    &escp2_low_pigment_standard_inks,
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
    &quadtone_economy_pigment_inks,
    &quadtone_low_pigment_inks,
    &quadtone_low_pigment_inks,
    &quadtone_6pl_pigment_inks,
    &quadtone_3pl_pigment_inks,
    &quadtone_3pl_pigment_2880_inks,
    &quadtone_3pl_pigment_2880_inks,
    &quadtone_3pl_pigment_2880_inks,
  },
  {
    &escp2_economy_pigment_standard_inks,
    &escp2_low_pigment_standard_inks,
    &escp2_low_pigment_standard_inks,
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
