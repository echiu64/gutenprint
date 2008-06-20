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
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include "print-escp2.h"
#include <limits.h>
#include <assert.h>

/*
 * Dot sizes are for:
 *
 *  0: 120/180
 *  1: 360
 *  2: 720x360
 *  3: 720
 *  4: 1440x720
 *  5: 2880x720 or 1440x1440
 *  6: 2880x1440
 *  7: 2880x2880
 *  8: 5760x2880
 */

/*   0     1     2     3     4     5     6     7     8 */

static const escp2_dot_size_t g1_dotsizes =
{   -2,   -2,   -2,   -2,   -1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t g2_dotsizes =
{   -2,   -2,   -2,   -2,   -1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t g3_dotsizes =
{    3,    3,    2,    1,    1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t c6pl_dotsizes =
{ 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };

static const escp2_dot_size_t c4pl_dotsizes =
{ 0x12, 0x12, 0x12, 0x11, 0x10, 0x10, 0x10, 0x10, 0x10 };

static const escp2_dot_size_t c4pl_pigment_dotsizes =
{ 0x12, 0x12, 0x12, 0x11, 0x11, 0x10, 0x10, 0x10, 0x10 };

static const escp2_dot_size_t c3pl_dotsizes =
{ 0x11, 0x11, 0x11, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };

static const escp2_dot_size_t c3pl_pigment_dotsizes =
{ 0x10, 0x10, 0x10, 0x11, 0x12, 0x12, 0x12, 0x12, 0x12 };

static const escp2_dot_size_t c3pl_pigment_c120_dotsizes =
{ 0x11, 0x11, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13, 0x13 };

static const escp2_dot_size_t c3pl_pigment_b500_dotsizes =
{ 0x11, 0x11, 0x11, 0x12, 0x13, 0x13, 0x13, 0x13, 0x13 };

static const escp2_dot_size_t p3pl_dotsizes =
{ 0x10, 0x10, 0x10, 0x11, 0x12, 0x12, 0x12, 0x12, 0x12 };

static const escp2_dot_size_t p1_5pl_dotsizes =
{ 0x10, 0x10, 0x10, 0x11, 0x12, 0x13, 0x13, 0x13, 0x13 };

static const escp2_dot_size_t claria_dotsizes =
{ 0x33, 0x33, 0x24, 0x24, 0x24, 0x24, 0x25, 0x25, 0x25 };

static const escp2_dot_size_t claria_1400_dotsizes =
{ 0x33, 0x33, 0x21, 0x21, 0x33, 0x33, 0x25, 0x25, 0x25 };

static const escp2_dot_size_t c2pl_dotsizes =
{ 0x12, 0x12, 0x12, 0x11, 0x13,   -1, 0x10, 0x10, 0x10 };

static const escp2_dot_size_t c1_8pl_dotsizes =
{ 0x10, 0x10, 0x10, 0x10, 0x11, 0x12, 0x12, 0x13, 0x13 };

static const escp2_dot_size_t p3_5pl_dotsizes =
{ 0x10, 0x10, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12 };

static const escp2_dot_size_t sc440_dotsizes =
{    3,    3,    2,    1,   -1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sc480_dotsizes =
{ 0x13, 0x13, 0x13, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10 };

static const escp2_dot_size_t sc600_dotsizes =
{    4,    4,    3,    2,    1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sc640_dotsizes =
{    3,    3,    2,    1,    1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sc660_dotsizes =
{    3,    3,    0,    0,    0,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sc670_dotsizes =
{ 0x12, 0x12, 0x12, 0x11, 0x11,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sp700_dotsizes =
{    3,    3,    2,    1,    4,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sp720_dotsizes =
{ 0x12, 0x12, 0x11, 0x11, 0x11,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sp2000_dotsizes =
{ 0x11, 0x11, 0x11, 0x10, 0x10,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t spro_dye_dotsizes =
{    3,    3,    3,    1,    1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t spro_pigment_dotsizes =
{    3,    3,    2,    1,    1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t spro10000_dotsizes =
{    4, 0x11, 0x11, 0x10, 0x10,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t spro5000_dotsizes =
{    3,    3,    2,    1,    4,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t spro_c4pl_pigment_dotsizes =
{ 0x11, 0x11, 0x11, 0x10, 0x10,   -1,    5,    5,    5 };

static const escp2_dot_size_t picturemate_dotsizes =
{   -1,   -1,   -1,   -1, 0x12, 0x12, 0x12, 0x12,   -1 };

/*
 * Bits are for:
 *
 *  0: 120/180
 *  1: 360
 *  2: 720x360
 *  3: 720
 *  4: 1440x720
 *  5: 2880x720 or 1440x1440
 *  6: 2880x1440
 *  7: 2880x2880
 *  8: 5760x2880
 */

/*   0     1     2     3     4     5     6     7     8 */

static const escp2_bits_t variable_bits =
{    2,    2,    2,    2,    2,    2,    2,    2,    2 };

static const escp2_bits_t stp950_bits =
{    2,    2,    2,    2,    2,    2,    1,    1,    1 };

static const escp2_bits_t ultrachrome_bits =
{    2,    2,    2,    2,    2,    1,    1,    1,    1 };

static const escp2_bits_t standard_bits =
{    1,    1,    1,    1,    1,    1,    1,    1,    1 };

static const escp2_bits_t c1_8_bits =
{    2,    2,    2,    2,    2,    1,    1,    1,    1 };

/*
 * Base resolutions are for:
 *
 *  0: 120/180
 *  1: 360
 *  2: 720x360
 *  3: 720
 *  4: 1440x720
 *  5: 2880x720 or 1440x1440
 *  6: 2880x1440
 *  7: 2880x2880
 *  8: 5760x2880
 */

/*   0     1     2     3     4     5     6     7     8 */

static const escp2_base_resolutions_t standard_base_res =
{  720,  720,  720,  720,  720,  720,  720,  720,  720 };

static const escp2_base_resolutions_t g3_base_res =
{  720,  720,  720,  720,  360,  360,  360,  360,  360 };

static const escp2_base_resolutions_t variable_base_res =
{  360,  360,  360,  360,  360,  360,  360,  360,  360 };

static const escp2_base_resolutions_t stp950_base_res =
{  360,  360,  360,  360,  360,  720,  720,  720,  720 };

static const escp2_base_resolutions_t ultrachrome_base_res =
{  360,  360,  360,  360,  360,  720,  720,  720,  720 };

static const escp2_base_resolutions_t c1_8_base_res =
{  360,  360,  720,  720,  720, 1440, 1440, 1440, 1440 };

static const escp2_base_resolutions_t c1_5_base_res =
{  360,  360,  720,  720,  720,  720,  720,  720,  720 };

static const escp2_base_resolutions_t b500_base_res =
{  360,  360,  720,  720,  720,  720,  720,  720,  720 };

static const escp2_base_resolutions_t claria_1400_base_res =
{  360,  360,  720,  720,  360,  360,  720,  720,  720 };

static const escp2_base_resolutions_t stc900_base_res =
{  360,  360,  360,  360,  180,  180,  360,  360,  360 };

static const escp2_base_resolutions_t pro_base_res =
{ 2880, 2880, 2880, 2880, 2880, 2880, 2880, 2880, 5760 };

/*
 * Densities are for:
 *
 *  0: 120/180
 *  1: 360
 *  2: 720x360
 *  3: 720
 *  4: 1440x720
 *  5: 2880x720 or 1440x1440
 *  6: 2880x1440
 *  7: 2880x2880
 *  8: 5760x2880
 */

/*  0    1     2       3    4      5      6      7      8 */

static const escp2_densities_t g1_densities =
{ 2.6, 1.3,  1.3,  0.568, 0.0,   0.0,   0.0,   0.0,   0.0   };

static const escp2_densities_t g3_densities =
{ 2.6, 1.3,  0.65, 0.775, 0.388, 0.0,   0.0,   0.0,   0.0   };

static const escp2_densities_t c6pl_densities =
{ 4.0, 2.0,  1.0,  0.568, 0.568, 0.568, 0.0,   0.0,   0.0   };

static const escp2_densities_t c4pl_2880_densities =
{ 2.6, 1.3,  0.65, 0.650, 0.650, 0.650, 0.32,  0.0,   0.0   };

static const escp2_densities_t c4pl_densities =
{ 2.6, 1.3,  0.65, 0.568, 0.523, 0.792, 0.396, 0.0,   0.0   };

static const escp2_densities_t c4pl_pigment_densities =
{ 2.3, 1.15, 0.58, 0.766, 0.388, 0.958, 0.479, 0.0,   0.0   };

static const escp2_densities_t c3pl_pigment_densities =
{ 2.4, 1.2,  0.60, 0.600, 0.512, 0.512, 0.512, 0.0,   0.0   };

static const escp2_densities_t c3pl_pigment_c66_densities =
{ 2.8, 1.4,  0.70, 0.600, 0.512, 0.512, 0.512, 0.0,   0.0   };

static const escp2_densities_t c3pl_pigment_c120_densities =
{ 4.0, 2.0,  1.56, 0.780, 0.512, 0.512, 0.512, 0.256, 0.128 };

static const escp2_densities_t c3pl_densities =
{ 2.6, 1.3,  0.65, 0.730, 0.7,   0.91,  0.455, 0.0,   0.0   };

static const escp2_densities_t p3pl_densities =
{ 4.0, 2.0,  1.00, 0.679, 0.657, 0.684, 0.566, 0.283, 0.0   };

static const escp2_densities_t p1_5pl_densities =
{ 2.8, 1.4,  1.00, 1.000, 0.869, 0.942, 0.471, 0.500, 0.530 };

static const escp2_densities_t claria_densities =
{ 4.0, 4.0,  3.52, 1.760, 0.880, 0.440, 0.586, 0.733, 0.440 };

static const escp2_densities_t claria_1400_densities =
{ 4.0, 4.0,  3.52, 1.760, 0.880, 0.440, 0.586, 0.733, 0.440 };

static const escp2_densities_t p3_5pl_densities =
{ 2.8, 1.4,  1.77, 0.886, 0.443, 0.221, 0.240, 0.293, 0.146 };

static const escp2_densities_t c3pl_pigment_b500_densities =
{ 6.0, 3.0,  1.63, 0.817, 1.250, 0.625, 0.460, 0.284, 0.142 };

static const escp2_densities_t c2pl_densities =
{ 2.0, 1.0,  0.5,  0.650, 0.650, 0.0,   0.650, 0.325, 0.0   };

static const escp2_densities_t c1_8pl_densities =
{ 2.3, 1.15, 0.57, 0.650, 0.650, 0.0,   0.650, 0.360, 0.0   };

static const escp2_densities_t sc1500_densities =
{ 2.6, 1.3,  1.3,  0.631, 0.0,   0.0,   0.0,   0.0,   0.0   };

static const escp2_densities_t sc440_densities =
{ 4.0, 2.0,  1.0,  0.900, 0.45,  0.0,   0.0,   0.0,   0.0   };

static const escp2_densities_t sc480_densities =
{ 2.8, 1.4,  0.7,  0.710, 0.710, 0.546, 0.0,   0.0,   0.0   };

static const escp2_densities_t sc660_densities =
{ 4.0, 2.0,  1.0,  0.646, 0.323, 0.0,   0.0,   0.0,   0.0   };

static const escp2_densities_t sc980_densities =
{ 2.6, 1.3,  0.65, 0.511, 0.49,  0.637, 0.455, 0.0,   0.0   };

static const escp2_densities_t sp700_densities =
{ 2.6, 1.3,  1.3,  0.775, 0.55,  0.0,   0.0,   0.0,   0.0   };

static const escp2_densities_t sp2000_densities =
{ 2.6, 1.3,  0.65, 0.852, 0.438, 0.219, 0.0,   0.0,   0.0   };

static const escp2_densities_t spro_dye_densities =
{ 2.6, 1.3,  1.3,  0.775, 0.388, 0.275, 0.0,   0.0,   0.0   };

static const escp2_densities_t spro_pigment_densities =
{ 3.0, 1.5,  0.78, 0.775, 0.388, 0.194, 0.0,   0.0,   0.0   };

static const escp2_densities_t spro10000_densities =
{ 2.6, 1.3,  0.65, 0.431, 0.216, 0.392, 0.0,   0.0,   0.0   };

static const escp2_densities_t picturemate_densities =
{   0,   0,     0,     0, 1.596, 0.798, 0.650, 0.530, 0.0   };


#define INCH(x)		(72 * x)

stpi_escp2_printer_t stpi_escp2_model_capabilities[] =
{
  /* FIRST GENERATION PRINTERS */
  /* 0: Stylus Color */
  {
    g1_dotsizes, g1_densities, standard_bits, standard_base_res,
    "simple", "720dpi", "standard"
  },
  /* 1: Stylus Color 400/500 */
  {
    g2_dotsizes, g1_densities, standard_bits, standard_base_res,
    "simple", "sc500", "standard"
  },
  /* 2: Stylus Color 1500 */
  {
    g1_dotsizes, sc1500_densities, standard_bits, standard_base_res,
    "simple", "sc500", "cmy"
  },
  /* 3: Stylus Color 600 */
  {
    sc600_dotsizes, g3_densities, standard_bits, g3_base_res,
    "simple", "g3", "standard"
  },
  /* 4: Stylus Color 800 */
  {
    g3_dotsizes, g3_densities, standard_bits, g3_base_res,
    "simple", "g3", "standard"
  },
  /* 5: Stylus Color 850 */
  {
    g3_dotsizes, g3_densities, standard_bits, g3_base_res,
    "simple", "g3", "standard"
  },
  /* 6: Stylus Color 1520 */
  {
    g3_dotsizes, g3_densities, standard_bits, g3_base_res,
    "simple", "g3", "standard"
  },

  /* SECOND GENERATION PRINTERS */
  /* 7: Stylus Photo 700 */
  {
    sp700_dotsizes, sp700_densities, standard_bits, g3_base_res,
    "simple", "g3", "photo_gen1"
  },
  /* 8: Stylus Photo EX */
  {
    sp700_dotsizes, sp700_densities, standard_bits, g3_base_res,
    "simple", "g3", "photo_gen1"
  },
  /* 9: Stylus Photo */
  {
    sp700_dotsizes, sp700_densities, standard_bits, g3_base_res,
    "simple", "g3_720dpi", "photo_gen1"
  },

  /* THIRD GENERATION PRINTERS */
  /* 10: Stylus Color 440/460 */
  {
    sc440_dotsizes, sc440_densities, standard_bits, standard_base_res,
    "simple", "g3_720dpi", "standard"
  },
  /* 11: Stylus Color 640 */
  {
    sc640_dotsizes, sc440_densities, standard_bits, standard_base_res,
    "simple", "sc640", "standard"
  },
  /* 12: Stylus Color 740/Stylus Scan 2000/Stylus Scan 2500 */
  {
    c6pl_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "standard"
  },
  /* 13: Stylus Color 900 */
  {
    c3pl_dotsizes, c3pl_densities, variable_bits, stc900_base_res,
    "variable_3pl", "1440dpi", "standard"
  },
  /* 14: Stylus Photo 750 */
  {
    c6pl_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "photo_gen1"
  },
  /* 15: Stylus Photo 1200 */
  {
    c6pl_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "photo_gen1"
  },
  /* 16: Stylus Color 860 */
  {
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_1440_4pl", "1440dpi", "standard"
  },
  /* 17: Stylus Color 1160 */
  {
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_1440_4pl", "1440dpi", "standard"
  },
  /* 18: Stylus Color 660 */
  {
    sc660_dotsizes, sc660_densities, standard_bits, standard_base_res,
    "simple", "sc640", "standard"
  },
  /* 19: Stylus Color 760 */
  {
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_1440_4pl", "1440dpi", "standard"
  },
  /* 20: Stylus Photo 720 (Australia) */
  {
    sp720_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "photo_gen1"
  },
  /* 21: Stylus Color 480 */
  {
    sc480_dotsizes, sc480_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "720dpi_soft", "x80"
  },
  /* 22: Stylus Photo 870/875 */
  {
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_1440_4pl", "1440dpi", "photo_gen2"
  },
  /* 23: Stylus Photo 1270 */
  {
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_1440_4pl", "1440dpi", "photo_gen2"
  },
  /* 24: Stylus Color 3000 */
  {
    g3_dotsizes, g3_densities, standard_bits, g3_base_res,
    "simple", "g3", "standard"
  },
  /* 25: Stylus Color 670 */
  {
    sc670_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "standard"
  },
  /* 26: Stylus Photo 2000P */
  {
    sp2000_dotsizes, sp2000_densities, variable_bits, variable_base_res,
    "variable_2000p", "1440dpi", "photo_pigment"
  },
  /* 27: Stylus Pro 5000 */
  {
    spro5000_dotsizes, sp700_densities, standard_bits, g3_base_res,
    "simple", "1440dpi", "pro_gen1"
  },
  /* 28: Stylus Pro 7000 */
  {
    spro_dye_dotsizes, spro_dye_densities, standard_bits, pro_base_res,
    "simple", "pro", "pro_gen1"
  },
  /* 29: Stylus Pro 7500 */
  {
    spro_pigment_dotsizes, spro_pigment_densities, standard_bits, pro_base_res,
    "simple", "pro", "pro_pigment"
  },
  /* 30: Stylus Pro 9000 */
  {
    spro_dye_dotsizes, spro_dye_densities, standard_bits, pro_base_res,
    "simple", "pro", "pro_gen1"
  },
  /* 31: Stylus Pro 9500 */
  {
    spro_pigment_dotsizes, spro_pigment_densities, standard_bits, pro_base_res,
    "simple", "pro", "pro_pigment"
  },
  /* 32: Stylus Color 777/680 */
  {
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "standard"
  },
  /* 33: Stylus Color 880/83/C60 */
  {
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "standard"
  },
  /* 34: Stylus Color 980 */
  {
    c3pl_dotsizes, sc980_densities, variable_bits, variable_base_res,
    "variable_3pl", "2880dpi", "standard"
  },
  /* 35: Stylus Photo 780/790 */
  {
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "photo_gen2"
  },
  /* 36: Stylus Photo 785/890/895/915/935 */
  {
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "photo_gen2"
  },
  /* 37: Stylus Photo 1280/1290 */
  {
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "photo_gen2"
  },
  /* 38: Stylus Color 580 */
  {
    sc480_dotsizes, sc480_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "1440dpi", "x80"
  },
  /* 39: Stylus Color Pro XL */
  {
    g1_dotsizes, g1_densities, standard_bits, standard_base_res,
    "simple", "720dpi", "standard"
  },
  /* 40: Stylus Pro 5500 */
  {
    spro_pigment_dotsizes, spro_pigment_densities, standard_bits, pro_base_res,
    "simple", "pro", "pro_pigment"
  },
  /* 41: Stylus Pro 10000 */
  {
    spro10000_dotsizes, spro10000_densities, variable_bits, pro_base_res,
    "spro10000", "pro", "pro_gen2"
  },
  /* 42: Stylus C20SX/C20UX */
  {
    sc480_dotsizes, sc480_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "720dpi_soft", "x80"
  },
  /* 43: Stylus C40SX/C40UX/C41SX/C41UX/C42SX/C42UX */
  {
    sc480_dotsizes, sc480_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "1440dpi", "x80"
  },
  /* 44: Stylus C70/C80 */
  {
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c80"
  },
  /* 45: Stylus Color Pro */
  {
    g1_dotsizes, g1_densities, standard_bits, standard_base_res,
    "simple", "720dpi", "standard"
  },
  /* 46: Stylus Photo 950/960 */
  {
    c2pl_dotsizes, c2pl_densities, stp950_bits, stp950_base_res,
    "variable_2pl", "superfine", "f360_photo"
  },
  /* 47: Stylus Photo 2100/2200 */
  {
    c4pl_pigment_dotsizes, c4pl_pigment_densities, ultrachrome_bits, ultrachrome_base_res,
    "variable_ultrachrome", "superfine", "f360_ultrachrome"
  },
  /* 48: Stylus Pro 7600 */
  {
    spro_c4pl_pigment_dotsizes, c4pl_pigment_densities, ultrachrome_bits, pro_base_res,
    "variable_ultrachrome", "pro", "pro_ultrachrome"
  },
  /* 49: Stylus Pro 9600 */
  {
    spro_c4pl_pigment_dotsizes, c4pl_pigment_densities, ultrachrome_bits, pro_base_res,
    "variable_ultrachrome", "pro", "pro_ultrachrome"
  },
  /* 50: Stylus Photo 825/830 */
  {
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880_1440dpi", "photo_gen2"
  },
  /* 51: Stylus Photo 925 */
  {
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880_1440dpi", "photo_gen2"
  },
  /* 52: Stylus Color C62 */
  {
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880_1440dpi", "standard"
  },
  /* 53: Japanese PM-950C */
  {
    c2pl_dotsizes, c2pl_densities, stp950_bits, stp950_base_res,
    "variable_2pl", "superfine", "f360_photo7_japan"
  },
  /* 54: Stylus Photo EX3 */
  {
    sp720_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "photo_gen1"
  },
  /* 55: Stylus C82/CX-5200 */
  {
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c82"
  },
  /* 56: Stylus C50 */
  {
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "1440dpi", "x80"
  },
  /* 57: Japanese PM-970C */
  {
    c1_8pl_dotsizes, c1_8pl_densities, c1_8_bits, c1_8_base_res,
    "variable_2pl", "superfine", "f360_photo7_japan"
  },
  /* 58: Japanese PM-930C */
  {
    c1_8pl_dotsizes, c1_8pl_densities, c1_8_bits, c1_8_base_res,
    "variable_2pl", "superfine", "photo_gen2"
  },
  /* 59: Stylus C43SX/C43UX/C44SX/C44UX (WRONG -- see 43!) */
  {
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "1440dpi", "x80"
  },
  /* 60: Stylus C84 */
  {
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c82"
  },
  /* 61: Stylus Color C63/C64 */
  {
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c64"
  },
  /* 62: Stylus Photo 900 */
  {
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "photo_gen2"
  },
  /* 63: Stylus Photo R300 */
  {
    p3pl_dotsizes, p3pl_densities, variable_bits, variable_base_res,
    "variable_3pl_pmg", "superfine", "photo_gen3"
  },
  /* 64: PM-G800/Stylus Photo R800 */
  {
    p1_5pl_dotsizes, p1_5pl_densities, variable_bits, c1_5_base_res,
    "variable_1_5pl", "superfine", "cmykrb"
  },
  /* 65: Stylus Photo CX4600 */
  {
    p3pl_dotsizes, p3pl_densities, variable_bits, variable_base_res,
    "variable_3pl_pmg", "superfine", "cx3650"
  },
  /* 66: Stylus Color C65/C66 */
  {
    c3pl_pigment_dotsizes, c3pl_pigment_c66_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment_c66", "2880_1440dpi", "c64"
  },
  /* 67: Stylus Photo R1800 */
  {
    p1_5pl_dotsizes, p1_5pl_densities, variable_bits, c1_5_base_res,
    "variable_1_5pl", "superfine", "cmykrb"
  },
  /* 68: PM-G820 */
  {
    p1_5pl_dotsizes, p1_5pl_densities, variable_bits, c1_5_base_res,
    "variable_1_5pl", "superfine", "cmykrb"
  },
  /* 69: Stylus C86 */
  {
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c82"
  },
  /* 70: Stylus Photo RX700 */
  {
    p1_5pl_dotsizes, p1_5pl_densities, variable_bits, c1_5_base_res,
    "variable_1_5pl", "superfine", "photo_gen4"
  },
  /* 71: Stylus Photo R2400 */
  {
    p3_5pl_dotsizes, p3_5pl_densities, variable_bits, c1_5_base_res,
    "variable_r2400", "superfine", "f360_ultrachrome_k3"
  },
  /* 72: Stylus CX3700/3800/3810 */
  {
    c3pl_pigment_dotsizes, c3pl_pigment_c66_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment_c66", "2880_1440dpi", "c64"
  },
  /* 73: E-100/PictureMate */
  {
    picturemate_dotsizes, picturemate_densities, variable_bits, c1_5_base_res,
    "variable_picturemate", "picturemate", "picturemate_6"
  },
  /* 74: PM-A650 */
  {
    c3pl_pigment_dotsizes, c3pl_pigment_c66_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment_c66", "superfine", "c64"
  },
  /* 75: Japanese PM-A750 */
  {
    c2pl_dotsizes, c2pl_densities, variable_bits, variable_base_res,
    "variable_2pl", "superfine", "c64"
  },
  /* 76: Japanese PM-A890 */
  {
    c2pl_dotsizes, c2pl_densities, variable_bits, variable_base_res,
    "variable_2pl", "superfine", "photo_gen4"
  },
  /* 77: Japanese PM-D600 */
  {
    p3pl_dotsizes, p3pl_densities, variable_bits, variable_base_res,
    "variable_3pl_pmg", "superfine", "c64"
  },
  /* 78: Stylus Photo 810/820 */
  {
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "photo_gen2"
  },
  /* 79: Stylus CX6400 */
  {
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c82"
  },
  /* 80: Stylus CX6600 */
  {
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c82"
  },
  /* 81: Stylus Photo R260 */
  {
    claria_dotsizes, claria_densities, variable_bits, c1_5_base_res,
    "variable_claria", "superfine", "claria"
  },
  /* 82: Stylus Photo 1400 */
  {
    claria_1400_dotsizes, claria_1400_densities, variable_bits, claria_1400_base_res,
    "variable_claria_1400", "claria_1400", "claria"
  },
  /* 83: Stylus Photo R240 */
  {
    p3pl_dotsizes, p3pl_densities, variable_bits, variable_base_res,
    "variable_3pl_pmg", "superfine", "photo_gen3_4"
  },
  /* 84: Stylus Photo RX500 */
  {
    p3pl_dotsizes, p3pl_densities, variable_bits, variable_base_res,
    "variable_3pl_pmg", "superfine", "photo_gen3"
  },
  /* 85: Stylus C120 */
  {
    c3pl_pigment_c120_dotsizes, c3pl_pigment_c120_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment_c120", "superfine", "c120"
  },
  /* 86: PictureMate 4-color */
  {
    picturemate_dotsizes, picturemate_densities, variable_bits, c1_5_base_res,
    "variable_picturemate", "picturemate", "picturemate_4"
  },
  /* 87: B-500DN */
  {
    c3pl_pigment_b500_dotsizes, c3pl_pigment_b500_densities, variable_bits, b500_base_res,
    "variable_3pl_pigment_b500", "superfine", "b500"
  },
};

const int stpi_escp2_model_limit =
sizeof(stpi_escp2_model_capabilities) / sizeof(stpi_escp2_printer_t);

static void
load_model_from_file(const stp_vars_t *v, stp_mxml_node_t *xmod, int model)
{
  stp_mxml_node_t *tmp = xmod->child;
  stpi_escp2_printer_t *p = &(stpi_escp2_model_capabilities[model]);
  int found_black_head_config = 0;
  int found_fast_head_config = 0;
  p->max_black_resolution = -1;
  p->cd_x_offset = -1;
  p->cd_y_offset = -1;
  while (tmp)
    {
      if (tmp->type == STP_MXML_ELEMENT)
	{
	  const char *name = tmp->value.element.name;
	  const char *target = stp_mxmlElementGetAttr(tmp, "href");
	  if (target)
	    {
	      if (!strcmp(name, "media"))
		stp_escp2_load_media(v, target);
	      else if (!strcmp(name, "inputSlots"))
		stp_escp2_load_input_slots(v, target);
	      else if (!strcmp(name, "mediaSizes"))
		stp_escp2_load_media_sizes(v, target);
	      else if (!strcmp(name, "printerWeaves"))
		stp_escp2_load_printer_weaves(v, target);
	      else if (!strcmp(name, "qualityPresets"))
		stp_escp2_load_quality_presets(v, target);
	    }
	  else if (tmp->child && tmp->child->type == STP_MXML_TEXT)
	    {
	      const char *val = tmp->child->value.text.string;
	      if (!strcmp(name, "verticalBorderlessSequence"))
		p->vertical_borderless_sequence = stp_xmlstrtoraw(val);

	      else if (!strcmp(name, "preinitSequence"))
		p->preinit_sequence = stp_xmlstrtoraw(val);

	      else if (!strcmp(name, "preinitRemoteSequence"))
		p->preinit_remote_sequence = stp_xmlstrtoraw(val);

	      else if (!strcmp(name, "postinitRemoteSequence"))
		p->postinit_remote_sequence = stp_xmlstrtoraw(val);
	      else if (!strcmp(name, "commandSet"))
		{
		  if (!strcmp(val, "1998"))
		    p->flags |= MODEL_COMMAND_1998;
		  else if (!strcmp(val, "1999"))
		    p->flags |= MODEL_COMMAND_1999;
		  else if (!strcmp(val, "2000"))
		    p->flags |= MODEL_COMMAND_2000;
		  else if (!strcmp(val, "Pro"))
		    p->flags |= MODEL_COMMAND_PRO;
		}
	      else if (!strcmp(name, "borderless"))
		{
		  if (!strcmp(val, "No"))
		    p->flags |= MODEL_ZEROMARGIN_NO;
		  else if (!strcmp(val, "Yes"))
		    p->flags |= MODEL_ZEROMARGIN_YES;
		  else if (!strcmp(val, "Full"))
		    p->flags |= MODEL_ZEROMARGIN_FULL;
		  else if (!strcmp(val, "HorizontalOnly"))
		    p->flags |= MODEL_ZEROMARGIN_H_ONLY;
		}
	      else if (!strcmp(name, "preferredEnvelopeOrientation") &&
		       !strcmp(val, "Landscape"))
		p->flags |= MODEL_ENVELOPE_LANDSCAPE_YES;
	      else if (!strcmp(name, "headConfiguration"))
		{
		  const char *htype = stp_mxmlElementGetAttr(tmp, "type");
		  unsigned long data[4] = { 0, 0, 0, 0 };
		  stp_mxml_node_t *child = tmp->child;
		  while (child)
		    {
		      if (child->type == STP_MXML_ELEMENT && child->child &&
			  child->child->type == STP_MXML_TEXT)
			{
			  const char *cname = child->value.element.name;
			  const char *cval = child->child->value.text.string;
			  if (!strcmp(cname, "Nozzles"))
			    data[0] = stp_xmlstrtoul(cval);
			  else if (!strcmp(cname, "MinNozzles"))
			    data[1] = stp_xmlstrtoul(cval);
			  else if (!strcmp(cname, "FirstNozzle"))
			    data[2] = stp_xmlstrtoul(cval);
			  else if (!strcmp(cname, "NozzleSeparation"))
			    data[3] = stp_xmlstrtoul(cval);
			}
		      child = child->next;
		    }		      
		  if (!strcmp(htype, "default"))
		    {
		      p->nozzles = data[0];
		      p->min_nozzles = data[1];
		      p->nozzle_start = data[2];
		      p->nozzle_separation = data[3];
		      if (!found_black_head_config)
			{
			  p->black_nozzles = data[0];
			  p->min_black_nozzles = data[1];
			  p->black_nozzle_start = data[2];
			  p->black_nozzle_separation = data[3];
			}
		      if (!found_fast_head_config)
			{
			  p->fast_nozzles = data[0];
			  p->min_fast_nozzles = data[1];
			  p->fast_nozzle_start = data[2];
			  p->fast_nozzle_separation = data[3];
			}
		    }
		  else if (!strcmp(htype, "black"))
		    {
		      p->black_nozzles = data[0];
		      p->min_black_nozzles = data[1];
		      p->black_nozzle_start = data[2];
		      p->black_nozzle_separation = data[3];
		      found_black_head_config = 1;
		    }
		  else if (!strcmp(htype, "fast"))
		    {
		      p->fast_nozzles = data[0];
		      p->min_fast_nozzles = data[1];
		      p->fast_nozzle_start = data[2];
		      p->fast_nozzle_separation = data[3];
		      found_fast_head_config = 1;
		    }
		}
	      else if (!strcmp(name, "margins"))
		{
		  const char *itype = stp_mxmlElementGetAttr(tmp, "interleave");
		  const char *mtype = stp_mxmlElementGetAttr(tmp, "media");
		  unsigned long data[4];
		  int i = 0;
		  stp_mxml_node_t *child = tmp->child;
		  while (child && i < 4)
		    {
		      if (child->type == STP_MXML_TEXT)
			data[i++] = stp_xmlstrtoul(child->value.text.string);
		      child = child->next;
		    }		      
		  if (itype && !strcmp(itype, "soft") &&
		      mtype && !strcmp(mtype, "sheet"))
		    {
		      p->left_margin = data[0];
		      p->right_margin = data[1];
		      p->top_margin = data[2];
		      p->bottom_margin = data[3];
		    }
		  else if (itype && !strcmp(itype, "printer") &&
			   mtype && !strcmp(mtype, "sheet"))
		    {
		      p->m_left_margin = data[0];
		      p->m_right_margin = data[1];
		      p->m_top_margin = data[2];
		      p->m_bottom_margin = data[3];
		    }
		  else if (itype && !strcmp(itype, "soft") &&
			   mtype && !strcmp(mtype, "roll"))
		    {
		      p->roll_left_margin = data[0];
		      p->roll_right_margin = data[1];
		      p->roll_top_margin = data[2];
		      p->roll_bottom_margin = data[3];
		    }
		  else if (itype && !strcmp(itype, "printer") &&
			   mtype && !strcmp(mtype, "roll"))
		    {
		      p->m_roll_left_margin = data[0];
		      p->m_roll_right_margin = data[1];
		      p->m_roll_top_margin = data[2];
		      p->m_roll_bottom_margin = data[3];
		    }
		}
	      else if (!strcmp(name, "physicalChannels"))
		p->physical_channels = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "baseSeparation"))
		p->base_separation = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "resolutionScale"))
		p->resolution_scale = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "maxBlackResolution"))
		p->max_black_resolution = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "minimumResolution"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->min_hres = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->min_vres = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "maximumResolution"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->max_hres = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->max_vres = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "extraVerticalFeed"))
		p->extra_feed = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "separationRows"))
		p->separation_rows = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "pseudoSeparationRows"))
		p->pseudo_separation_rows = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "zeroMarginOffset"))
		p->zero_margin_offset = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "microLeftMargin"))
		p->micro_left_margin = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "initialVerticalOffset"))
		p->initial_vertical_offset = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "blackInitialVerticalOffset"))
		p->black_initial_vertical_offset = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "extra720DPISeparation"))
		p->extra_720dpi_separation = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "minHorizontalAlignment"))
		p->min_horizontal_position_alignment = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "baseHorizontalAlignment"))
		p->base_horizontal_position_alignment = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "bidirectionalAutoUpperLimit"))
		p->bidirectional_upper_limit = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "minimumMediaSize"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->min_paper_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->min_paper_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "maximumMediaSize"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->max_paper_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->max_paper_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "maximumImageableArea"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->max_imageable_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->max_imageable_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "CDOffset"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->cd_x_offset = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->cd_y_offset = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "CDMediaSize"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->cd_page_width = stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->cd_page_height = stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "extraBottom"))
		p->paper_extra_bottom = stp_xmlstrtoul(val);
	      else if (!strcmp(name, "AlignmentChoices"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->alignment_passes =
		    stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->alignment_choices =
		    stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->alternate_alignment_passes =
		    stp_xmlstrtoul(child->value.text.string);
		  child = child->next;
		  p->alternate_alignment_choices =
		    stp_xmlstrtoul(child->value.text.string);
		}
	      else if (!strcmp(name, "ChannelNames"))
		{
		  stp_mxml_node_t *child = tmp->child;
		  p->channel_names = stp_string_list_create();
		  while (child)
		    {
		      if (child->type == STP_MXML_ELEMENT &&
			  !strcmp(child->value.element.name, "ChannelName"))
			{
			  const char *cname = stp_mxmlElementGetAttr(child, "name");
			  stp_string_list_add_string(p->channel_names, cname, cname);
			}
		      child = child->next;
		    }
		}
	    }
	  else
	    {
	      if (!strcmp(name, "supportsVariableDropsizes"))
		p->flags |= MODEL_VARIABLE_YES;
	      else if (!strcmp(name, "hasFastGraymode"))
		p->flags |= MODEL_GRAYMODE_YES;
	      else if (!strcmp(name, "hasFast360DPI"))
		p->flags |= MODEL_FAST_360_YES;
	      else if (!strcmp(name, "sendZeroAdvance"))
		p->flags |= MODEL_SEND_ZERO_ADVANCE_YES;
	      else if (!strcmp(name, "supportsInkChange"))
		p->flags |= MODEL_SUPPORTS_INK_CHANGE_YES;
	      else if (!strcmp(name, "supportsD4Mode"))
		p->flags |= MODEL_PACKET_MODE_YES;
	      else if (!strcmp(name, "hasInterchangeableInkCartridges"))
		p->flags |= MODEL_INTERCHANGEABLE_INK_YES;
	    }
	}
      tmp = tmp->next;
    }
}

void
stp_escp2_load_model(const stp_vars_t *v, int model)
{
  stp_list_t *dirlist = stpi_data_path();
  stp_list_item_t *item;
  char buf[1024];
  int found = 0;

  stp_xml_init();
  sprintf(buf, "escp2/model/model_%d.xml", model);
  item = stp_list_get_start(dirlist);
  while (item)
    {
      const char *dn = (const char *) stp_list_item_get_data(item);
      char *fn = stpi_path_merge(dn, buf);
      stp_mxml_node_t *doc = stp_mxmlLoadFromFile(NULL, fn, STP_MXML_NO_CALLBACK);
      stp_free(fn);
      if (doc)
	{
	  stp_mxml_node_t *xmod =
	    stp_mxmlFindElement(doc, doc, "escp2:model", NULL, NULL,
				STP_MXML_DESCEND);
	  if (xmod)
	    {
	      const char *stmp = stp_mxmlElementGetAttr(xmod, "id");
	      assert(stmp && stp_xmlstrtol(stmp) == model);
	      if (stmp && stp_xmlstrtol(stmp) == model)
		{
		  load_model_from_file(v, xmod, model);
		  found = 1;
		}
	    }
	  stp_mxmlDelete(doc);
	  if (found)
	    break;
	}
      item = stp_list_item_next(item);
    }
  stp_list_destroy(dirlist);
  if (! found)
    stp_eprintf(v, "Unable to load definition for model %d!\n", model);
}
