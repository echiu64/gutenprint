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
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    15, 1, 0, 4, 15, 1, 0, 4, 15, 1, 0, 4, 4,
    360, 14400, -1, 720, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    g1_dotsizes, g1_densities, standard_bits, standard_base_res,
    "simple", "720dpi", "standard", "standard", NULL, "standard"
  },
  /* 1: Stylus Color 400/500 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 4,
    360, 14400, -1, 720, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    g2_dotsizes, g1_densities, standard_bits, standard_base_res,
    "simple", "sc500", "standard", "standard", NULL, "standard"
  },
  /* 2: Stylus Color 1500 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 4,
    360, 14400, -1, 720, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(17), INCH(44), INCH(2), INCH(2), INCH(17), INCH(44),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    g1_dotsizes, sc1500_densities, standard_bits, standard_base_res,
    "simple", "sc500", "cmy", "standard", NULL, "standard"
  },
  /* 3: Stylus Color 600 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    32, 1, 0, 4, 32, 1, 0, 4, 32, 1, 0, 4, 4,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 8, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    8, 9, 0, 30, 8, 9, 0, 30, 8, 9, 0, 0, 8, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    sc600_dotsizes, g3_densities, standard_bits, g3_base_res,
    "simple", "g3", "standard", "standard", NULL, "standard"
  },
  /* 4: Stylus Color 800 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    64, 1, 0, 2, 64, 1, 0, 2, 64, 1, 0, 2, 4,
    360, 14400, -1, 1440, 720, 180, 180,
    0, 1, 4, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    8, 9, 9, 40, 8, 9, 9, 40, 8, 9, 0, 0, 8, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    g3_dotsizes, g3_densities, standard_bits, g3_base_res,
    "simple", "g3", "standard", "standard", NULL, "standard"
  },
  /* 5: Stylus Color 850 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    64, 1, 0, 2, 64, 1, 0, 2, 64, 1, 0, 2, 4,
    360, 14400, -1, 1440, 720, 180, 180,
    0, 1, 4, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    g3_dotsizes, g3_densities, standard_bits, g3_base_res,
    "simple", "g3", "standard", "standard", NULL, "standard"
  },
  /* 6: Stylus Color 1520 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    64, 1, 0, 2, 64, 1, 0, 2, 64, 1, 0, 2, 4,
    360, 14400, -1, 1440, 720, 180, 180,
    0, 1, 4, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(17), INCH(44), INCH(2), INCH(2), INCH(136 / 10), INCH(44),
    8, 9, 9, 40, 8, 9, 9, 40, 8, 9, 0, 0, 8, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    g3_dotsizes, g3_densities, standard_bits, g3_base_res,
    "simple", "g3", "standard", "standard", NULL, "standard"
  },

  /* SECOND GENERATION PRINTERS */
  /* 7: Stylus Photo 700 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    32, 1, 0, 4, 32, 1, 0, 4, 32, 1, 0, 4, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 8, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 0, 30, 9, 9, 0, 30, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 15, 0, 0,		/* Is it really 15 pairs??? */
    sp700_dotsizes, sp700_densities, standard_bits, g3_base_res,
    "simple", "g3", "photo_gen1", "standard", NULL, "photo"
  },
  /* 8: Stylus Photo EX */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_NO | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    32, 1, 0, 4, 32, 1, 0, 4, 32, 1, 0, 4, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 8, 1, 28800, 720 * 720,
    INCH(118 / 10), INCH(44), INCH(2), INCH(2), INCH(118 / 10), INCH(44),
    9, 9, 0, 30, 9, 9, 0, 30, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    sp700_dotsizes, sp700_densities, standard_bits, g3_base_res,
    "simple", "g3", "photo_gen1", "standard", NULL, "photo"
  },
  /* 9: Stylus Photo */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    32, 1, 0, 4, 32, 1, 0, 4, 32, 1, 0, 4, 6,
    360, 14400, -1, 720, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 8, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 0, 30, 9, 9, 0, 30, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    sp700_dotsizes, sp700_densities, standard_bits, g3_base_res,
    "simple", "g3_720dpi", "photo_gen1", "standard", NULL, "photo"
  },

  /* THIRD GENERATION PRINTERS */
  /* 10: Stylus Color 440/460 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    21, 1, 0, 4, 21, 1, 0, 4, 21, 1, 0, 4, 4,
    360, 14400, -1, 720, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 8, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 15, 0, 0,
    sc440_dotsizes, sc440_densities, standard_bits, standard_base_res,
    "simple", "g3_720dpi", "standard", "standard", NULL, "standard"
  },
  /* 11: Stylus Color 640 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    32, 1, 0, 4, 32, 1, 0, 4, 32, 1, 0, 4, 4,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 8, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 15, 0, 0,
    sc640_dotsizes, sc440_densities, standard_bits, standard_base_res,
    "simple", "sc640", "standard", "standard", NULL, "standard"
  },
  /* 12: Stylus Color 740/Stylus Scan 2000/Stylus Scan 2500 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 144, 1, 0, 1, 144, 1, 0, 1, 4,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c6pl_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "standard", "standard", NULL, "standard"
  },
  /* 13: Stylus Color 900 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    96, 1, 0, 2, 192, 1, 0, 1, 192, 1, 0, 1, 4,
    360, 14400, -1, 1440, 720, 180, 180,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c3pl_dotsizes, c3pl_densities, variable_bits, stc900_base_res,
    "variable_3pl", "1440dpi", "standard", "standard", NULL, "standard"
  },
  /* 14: Stylus Photo 750 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c6pl_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "photo_gen1", "standard", NULL, "photo"
  },
  /* 15: Stylus Photo 1200 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(13), INCH(44), INCH(2), INCH(2), INCH(13), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c6pl_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "photo_gen1", "standard", NULL, "photo"
  },
  /* 16: Stylus Color 860 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 144, 1, 0, 1, 144, 1, 0, 1, 4,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_1440_4pl", "1440dpi", "standard", "standard", NULL, "standard"
  },
  /* 17: Stylus Color 1160 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 144, 1, 0, 1, 144, 1, 0, 1, 4,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(13), INCH(44), INCH(2), INCH(2), INCH(13), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_1440_4pl", "1440dpi", "standard", "standard", NULL, "standard"
  },
  /* 18: Stylus Color 660 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    32, 1, 0, 4, 32, 1, 0, 4, 32, 1, 0, 4, 4,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 8, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 9, 9, 9, 9, 9, 26, 9, 9, 9, 0, 9, 9, 9, 0, -1, -1, 0, 0, 0,
    1, 15, 0, 0,
    sc660_dotsizes, sc660_densities, standard_bits, standard_base_res,
    "simple", "sc640", "standard", "standard", NULL, "standard"
  },
  /* 19: Stylus Color 760 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 144, 1, 0, 1, 144, 1, 0, 1, 4,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_1440_4pl", "1440dpi", "standard", "standard", NULL, "standard"
  },
  /* 20: Stylus Photo 720 (Australia) */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    32, 1, 0, 4, 32, 1, 0, 4, 32, 1, 0, 4, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    sp720_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "photo_gen1", "standard", NULL, "photo"
  },
  /* 21: Stylus Color 480 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_YES |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    15, 15, 0, 3, 48, 48, 0, 3, 48, 48, 0, 3, 4,
    360, 14400, 360, 720, 720, 90, 90,
    0, 1, 0, 0, 0, -99, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    sc480_dotsizes, sc480_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "720dpi_soft", "x80", "standard", NULL, "standard"
  },
  /* 22: Stylus Photo 870/875 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_1440_4pl", "1440dpi", "photo_gen2", "standard", NULL, "photo"
  },
  /* 23: Stylus Photo 1270 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(13), INCH(1200), INCH(2), INCH(2), INCH(13), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_1440_4pl", "1440dpi", "photo_gen2", "standard", NULL, "photo"
  },
  /* 24: Stylus Color 3000 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    64, 1, 0, 2, 64, 1, 0, 2, 64, 1, 0, 2, 4,
    360, 14400, -1, 1440, 720, 180, 180,
    0, 1, 4, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(17), INCH(44), INCH(2), INCH(2), INCH(17), INCH(44),
    8, 9, 9, 40, 8, 9, 9, 40, 8, 9, 0, 0, 8, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    g3_dotsizes, g3_densities, standard_bits, g3_base_res,
    "simple", "g3", "standard", "standard", NULL, "standard"
  },
  /* 25: Stylus Color 670 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    32, 1, 0, 4, 64, 1, 0, 2, 64, 1, 0, 2, 4,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    sc670_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "standard", "standard", NULL, "standard"
  },
  /* 26: Stylus Photo 2000P */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 144, 1, 0, 1, 144, 1, 0, 1, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(13), INCH(1200), INCH(2), INCH(2), INCH(13), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    2, 15, 0, 0,
    sp2000_dotsizes, sp2000_densities, variable_bits, variable_base_res,
    "variable_2000p", "1440dpi", "photo_pigment", "standard", NULL, "photo"
  },
  /* 27: Stylus Pro 5000 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    64, 1, 0, 2, 64, 1, 0, 2, 64, 1, 0, 2, 6,
    360, 14400, -1, 1440, 720, 180, 180,
    0, 1, 0, 0, 0, 0, 0, 4, 1, 28800, 720 * 720,
    INCH(13), INCH(44), INCH(2), INCH(2), INCH(13), INCH(44),
    9, 9, 0, 30, 9, 9, 0, 30, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    spro5000_dotsizes, sp700_densities, standard_bits, g3_base_res,
    "simple", "1440dpi", "pro_gen1", "standard", NULL, "photo"
  },
  /* 28: Stylus Pro 7000 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(24), INCH(1200), INCH(7), INCH(7), INCH(24), INCH(1200),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 9, 9, 9, 9, 9, 9, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    spro_dye_dotsizes, spro_dye_densities, standard_bits, pro_base_res,
    "simple", "pro", "pro_gen1", "standard", "pro7000", "photo"
  },
  /* 29: Stylus Pro 7500 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(24), INCH(1200), INCH(7), INCH(7), INCH(24), INCH(1200),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 9, 9, 9, 9, 9, 9, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    spro_pigment_dotsizes, spro_pigment_densities, standard_bits, pro_base_res,
    "simple", "pro", "pro_pigment", "standard", "pro7500", "photo"
  },
  /* 30: Stylus Pro 9000 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(44), INCH(1200), INCH(7), INCH(7), INCH(44), INCH(1200),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 9, 9, 9, 9, 9, 9, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    spro_dye_dotsizes, spro_dye_densities, standard_bits, pro_base_res,
    "simple", "pro", "pro_gen1", "standard", "pro7000", "photo"
  },
  /* 31: Stylus Pro 9500 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(44), INCH(1200), INCH(7), INCH(7), INCH(44), INCH(1200),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 9, 9, 9, 9, 9, 9, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    spro_pigment_dotsizes, spro_pigment_densities, standard_bits, pro_base_res,
    "simple", "pro", "pro_pigment", "standard", "pro7500", "photo"
  },
  /* 32: Stylus Color 777/680 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 144, 1, 0, 1, 144, 1, 0, 1, 4,
    360, 14400, -1, 2880, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "standard", "standard", NULL, "standard"
  },
  /* 33: Stylus Color 880/83/C60 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 144, 1, 0, 1, 144, 1, 0, 1, 4,
    360, 14400, -1, 2880, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "standard", "standard", NULL, "standard"
  },
  /* 34: Stylus Color 980 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    96, 1, 0, 2, 192, 1, 0, 1, 192, 1, 0, 1, 4,
    360, 14400, -1, 2880, 720, 180, 180,
    38, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c3pl_dotsizes, sc980_densities, variable_bits, variable_base_res,
    "variable_3pl", "2880dpi", "standard", "standard", NULL, "standard"
  },
  /* 35: Stylus Photo 780/790 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 2880, 720, 90, 90,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "photo_gen2", "standard", NULL, "photo"
  },
  /* 36: Stylus Photo 785/890/895/915/935 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 2880, 720, 90, 90,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "photo_gen2", "standard", NULL, "photo"
  },
  /* 37: Stylus Photo 1280/1290 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 2880, 720, 90, 90,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(13), INCH(1200), INCH(2), INCH(2), INCH(13), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "photo_gen2", "standard", NULL, "photo"
  },
  /* 38: Stylus Color 580 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_YES |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    15, 15, 0, 3, 48, 48, 0, 3, 48, 48, 0, 3, 4,
    360, 14400, 360, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, -99, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    sc480_dotsizes, sc480_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "1440dpi", "x80", "standard", NULL, "standard"
  },
  /* 39: Stylus Color Pro XL */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    16, 1, 0, 4, 16, 1, 0, 4, 16, 1, 0, 4, 4,
    360, 14400, -1, 720, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(13), INCH(1200), INCH(2), INCH(2), INCH(13), INCH(1200),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    g1_dotsizes, g1_densities, standard_bits, standard_base_res,
    "simple", "720dpi", "standard", "standard", NULL, "standard"
  },
  /* 40: Stylus Pro 5500 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(13), INCH(1200), INCH(2), INCH(2), INCH(13), INCH(1200),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    spro_pigment_dotsizes, spro_pigment_densities, standard_bits, pro_base_res,
    "simple", "pro", "pro_pigment", "standard", "pro7500", "photo"
  },
  /* 41: Stylus Pro 10000 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(44), INCH(1200), INCH(7), INCH(7), INCH(44), INCH(1200),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 9, 9, 9, 9, 9, 9, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    spro10000_dotsizes, spro10000_densities, variable_bits, pro_base_res,
    "spro10000", "pro", "pro_gen2", "standard", "pro7000", "photo"
  },
  /* 42: Stylus C20SX/C20UX */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_YES |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    15, 15, 0, 3, 48, 48, 0, 3, 48, 48, 0, 3, 4,
    360, 14400, -1, 720, 720, 90, 90,
    0, 1, 0, 0, 0, -99, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    sc480_dotsizes, sc480_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "720dpi_soft", "x80", "standard", NULL, "standard"
  },
  /* 43: Stylus C40SX/C40UX/C41SX/C41UX/C42SX/C42UX */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_YES |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    15, 15, 0, 3, 48, 48, 0, 3, 48, 48, 0, 3, 4,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, -99, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    sc480_dotsizes, sc480_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "1440dpi", "x80", "standard", NULL, "standard"
  },
  /* 44: Stylus C70/C80 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    60, 60, 0, 2, 180, 180, 0, 2, 180, 180, 0, 2, 4,
    360, 14400, -1, 2880, 1440, 360, 180,
    0, 1, 0, 0, 0, -240, 0, 0, 1, 28800, 720 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    4, 15, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c80", "standard", NULL, "standard"
  },
  /* 45: Stylus Color Pro */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_NO| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    16, 1, 0, 4, 16, 1, 0, 4, 16, 1, 0, 4, 4,
    360, 14400, -1, 720, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(44), INCH(2), INCH(2), INCH(17 / 2), INCH(44),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    1, 7, 0, 0,
    g1_dotsizes, g1_densities, standard_bits, standard_base_res,
    "simple", "720dpi", "standard", "standard", NULL, "standard"
  },
  /* 46: Stylus Photo 950/960 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_YES |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    96, 96, 0, 2, 96, 96, 0, 2, 24, 24, 0, 1, 6,
    360, 14400, -1, 2880, 1440, 360, 180,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 0, 0, 24,
    4, 15, 0, 0,
    c2pl_dotsizes, c2pl_densities, stp950_bits, stp950_base_res,
    "variable_2pl", "superfine", "f360_photo", "standard", NULL, "sp960"
  },
  /* 47: Stylus Photo 2100/2200 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_H_ONLY | MODEL_FAST_360_YES |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_YES |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    96, 96, 0, 2, 96, 96, 0, 2, 192, 192, 0, 1, 7,
    360, 14400, -1, 2880, 1440, 360, 180,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(13), INCH(1200), INCH(2), INCH(2), INCH(13), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 0, 0, 0,
    4, 15, 0, 0,
    c4pl_pigment_dotsizes, c4pl_pigment_densities, ultrachrome_bits, ultrachrome_base_res,
    "variable_ultrachrome", "superfine", "f360_ultrachrome", "standard", NULL, "sp2200"
  },
  /* 48: Stylus Pro 7600 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_PRO | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 7,
    360, 14400, -1, 2880, 1440, 360, 180,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(24), INCH(1200), INCH(7), INCH(7), INCH(24), INCH(1200),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    spro_c4pl_pigment_dotsizes, c4pl_pigment_densities, ultrachrome_bits, pro_base_res,
    "variable_ultrachrome", "pro", "pro_ultrachrome", "standard", "pro7600", "photo"
  },
  /* 49: Stylus Pro 9600 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_PRO | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 7,
    360, 14400, -1, 2880, 1440, 360, 180,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(44), INCH(1200), INCH(7), INCH(7), INCH(44), INCH(1200),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    spro_c4pl_pigment_dotsizes, c4pl_pigment_densities, ultrachrome_bits, pro_base_res,
    "variable_ultrachrome", "pro", "pro_ultrachrome", "standard", "pro7600", "photo"
  },
  /* 50: Stylus Photo 825/830 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 2880, 1440, 90, 90,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880_1440dpi", "photo_gen2", "standard", NULL, "photo"
  },
  /* 51: Stylus Photo 925 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 2880, 1440, 90, 90,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880_1440dpi", "photo_gen2", "standard", NULL, "photo"
  },
  /* 52: Stylus Color C62 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    48, 1, 0, 3, 144, 1, 0, 1, 144, 1, 0, 1, 4,
    360, 14400, -1, 2880, 1440, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880_1440dpi", "standard", "standard", NULL, "standard"
  },
  /* 53: Japanese PM-950C */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_YES |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    96, 96, 0, 2, 96, 96, 0, 2, 24, 24, 0, 1, 6,
    360, 14400, -1, 2880, 1440, 360, 180,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 0, 0, 24,
    4, 15, 0, 0,
    c2pl_dotsizes, c2pl_densities, stp950_bits, stp950_base_res,
    "variable_2pl", "superfine", "f360_photo7_japan", "standard", NULL, "pm_950c"
  },
  /* 54: Stylus Photo EX3 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    32, 1, 0, 4, 32, 1, 0, 4, 32, 1, 0, 4, 6,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(13), INCH(44), INCH(2), INCH(2), INCH(13), INCH(44),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    sp720_dotsizes, c6pl_densities, variable_bits, variable_base_res,
    "variable_6pl", "1440dpi", "photo_gen1", "standard", NULL, "photo"
  },
  /* 55: Stylus C82/CX-5200 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    59, 60, 0, 2, 180, 180, 0, 2, 180, 180, 0, 2, 4,
    360, 14400, -1, 2880, 1440, 360, 180,
    0, 1, 0, 0, 0, -240, 0, 0, 1, 28800, 720 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    4, 15, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c82", "standard", NULL, "standard"
  },
  /* 56: Stylus C50 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    15, 15, 0, 3, 48, 48, 0, 3, 48, 48, 0, 3, 4,
    360, 14400, -1, 1440, 720, 90, 90,
    0, 1, 0, 0, 0, -99, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "1440dpi", "x80", "standard", NULL, "standard"
  },
  /* 57: Japanese PM-970C */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_YES |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    180, 180, 0, 2, 360, 360, 0, 1, 360, 360, 0, 1, 7,
    360, 14400, -1, 2880, 2880, 720, 360,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    4, 15, 0, 0,
    c1_8pl_dotsizes, c1_8pl_densities, c1_8_bits, c1_8_base_res,
    "variable_2pl", "superfine", "f360_photo7_japan", "standard", NULL, "pm_950c"
  },
  /* 58: Japanese PM-930C */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 90, 0, 2, 90, 90, 0, 2, 90, 90, 0, 2, 6,
    360, 14400, -1, 2880, 2880, 720, 360,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    4, 15, 0, 0,
    c1_8pl_dotsizes, c1_8pl_densities, c1_8_bits, c1_8_base_res,
    "variable_2pl", "superfine", "photo_gen2", "standard", NULL, "photo"
  },
  /* 59: Stylus C43SX/C43UX/C44SX/C44UX (WRONG -- see 43!) */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_YES |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    15, 15, 0, 3, 48, 48, 0, 3, 48, 48, 0, 3, 4,
    360, 14400, -1, 2880, 720, 90, 90,
    0, 1, 0, 0, 0, -99, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_densities, variable_bits, variable_base_res,
    "variable_x80_6pl", "1440dpi", "x80", "standard", NULL, "standard"
  },
  /* 60: Stylus C84 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    59, 60, 0, 2, 180, 180, 0, 2, 180, 180, 0, 2, 4,
    360, 14400, -1, 2880, 1440, 360, 180,
    0, 1, 0, 80, 42, -240, 0, 0, 1, 28800, 720 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    4, 15, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c82", "standard", NULL, "standard"
  },
  /* 61: Stylus Color C63/C64 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    29, 30, 0, 3, 90, 90, 0, 3, 90, 90, 0, 3, 4,
    360, 14400, -1, 2880, 1440, 360, 120,
    0, 1, 0, 80, 42, -180, 0, 0, 1, 28800, 1440 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    4, 15, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c64", "standard", NULL, "standard"
  },
  /* 62: Stylus Photo 900 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 2880, 720, 90, 90,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, 399, 394, 595, 842, 24,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "photo_gen2", "standard", NULL, "photo"
  },
  /* 63: Stylus Photo R300 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_FULL | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 1, 0, 3, 90, 1, 0, 3, 90, 1, 0, 3, 6,
    360, 14400, -1, 2880, 1440, 360, 120,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 24,
    4, 15, 0, 0,
    p3pl_dotsizes, p3pl_densities, variable_bits, variable_base_res,
    "variable_3pl_pmg", "superfine", "photo_gen3", "standard", NULL, "photo"
  },
  /* 64: PM-G800/Stylus Photo R800 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2005 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_FULL | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    180, 1, 0, 2, 180, 1, 0, 2, 180, 1, 0, 2, 8,
    360, 28800, -1, 5760, 2880, 360, 180,
    180 * 2, 1, 0, 80, 42, 0, 0, 0, 1, 180, 5760 * 2880,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 11, 9, 9, 0, 11, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 24,
    4, 15, 0, 0,
    p1_5pl_dotsizes, p1_5pl_densities, variable_bits, c1_5_base_res,
    "variable_1_5pl", "superfine", "cmykrb", "v2880", NULL, "r800"
  },
  /* 65: Stylus Photo CX4600 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 1, 0, 3, 90, 1, 0, 3, 90, 1, 0, 3, 4,
    360, 14400, -1, 5760, 1440, 360, 120,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 180, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 0,
    4, 15, 0, 0,
    p3pl_dotsizes, p3pl_densities, variable_bits, variable_base_res,
    "variable_3pl_pmg", "superfine", "cx3650", "standard", NULL, "mfp2005"
  },
  /* 66: Stylus Color C65/C66 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    29, 30, 0, 3, 90, 90, 0, 3, 90, 90, 0, 3, 4,
    360, 14400, -1, 2880, 1440, 360, 120,
    0, 1, 0, 80, 42, -180, 0, 0, 1, 28800, 1440 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    4, 15, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_c66_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment_c66", "2880_1440dpi", "c64", "standard", NULL, "standard"
  },
  /* 67: Stylus Photo R1800 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2005 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_FULL | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    180, 1, 0, 2, 180, 1, 0, 2, 180, 1, 0, 2, 8,
    360, 28800, -1, 5760, 2880, 360, 180,
    180 * 2, 1, 0, 96, 42, 0, 0, 0, 1, 180, 5760 * 2880,
    INCH(13), INCH(1200), INCH(2), INCH(2), INCH(13), INCH(1200),
    9, 9, 0, 11, 9, 9, 0, 11, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 24,
    4, 15, 0, 0,
    p1_5pl_dotsizes, p1_5pl_densities, variable_bits, c1_5_base_res,
    "variable_1_5pl", "superfine", "cmykrb", "v2880", NULL, "r800"
  },
  /* 68: PM-G820 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_FULL | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    180, 1, 0, 2, 180, 1, 0, 2, 180, 1, 0, 2, 8,
    360, 14400, -1, 5760, 2880, 360, 180,
    180 * 2, 1, 0, 80, 42, 0, 0, 0, 1, 180, 5760 * 2880,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 11, 9, 9, 0, 11, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 24,
    4, 15, 0, 0,
    p1_5pl_dotsizes, p1_5pl_densities, variable_bits, c1_5_base_res,
    "variable_1_5pl", "superfine", "cmykrb", "v2880", NULL, "r800"
  },
  /* 69: Stylus C86 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    59, 60, 0, 2, 180, 180, 0, 2, 180, 180, 0, 2, 4,
    360, 14400, -1, 2880, 2880, 360, 180,
    0, 1, 0, 80, 42, -240, 0, 0, 1, 28800, 1440 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    4, 15, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c82", "standard", NULL, "standard"
  },
  /* 70: Stylus Photo RX700 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2005 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_FULL | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    180, 1, 0, 2, 180, 1, 0, 2, 180, 1, 0, 2, 6,
    360, 28800, -1, 5760, 2880, 360, 180,
    10, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 204, 263, 595, 842, 0,
    4, 15, 0, 0,
    p1_5pl_dotsizes, p1_5pl_densities, variable_bits, c1_5_base_res,
    "variable_1_5pl", "superfine", "photo_gen4", "p1_5", NULL, "rx700"
  },
  /* 71: Stylus Photo R2400 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2005 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_FULL | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_YES |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    180, 1, 0, 2, 180, 1, 0, 2, 180, 1, 0, 2, 8,
    360, 14400, -1, 5760, 2880, 360, 180,
    10, 1, 0, 80, 42, 0, 0, 0, 1, 180, 1440 * 1440,
    INCH(13), INCH(1200), INCH(2), INCH(2), INCH(13), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 0,
    4, 15, 0, 0,
    p3_5pl_dotsizes, p3_5pl_densities, variable_bits, c1_5_base_res,
    "variable_r2400", "superfine", "f360_ultrachrome_k3", "v2880", NULL, "r2400"
  },
  /* 72: Stylus CX3700/3800/3810 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    29, 30, 1, 3, 90, 90, 0, 3, 90, 90, 0, 3, 4,
    360, 14400, -1, 2880, 1440, 360, 120,
    0, 1, 0, 80, 42, -180, 0, 0, 1, 28800, 1440 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    4, 15, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_c66_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment_c66", "2880_1440dpi", "c64", "standard", NULL, "cx3800"
  },
  /* 73: E-100/PictureMate */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_FULL | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 1, 0, 3, 90, 1, 0, 3, 90, 1, 0, 3, 6,
    360, 28800, -1, 5760, 1440, 1440, 720,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(4), INCH(1200), INCH(2), INCH(2), INCH(4), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 0,
    4, 15, 0, 0,
    picturemate_dotsizes, picturemate_densities, variable_bits, c1_5_base_res,
    "variable_picturemate", "picturemate", "picturemate_6", "picturemate", NULL, "picturemate_6"
  },
  /* 74: PM-A650 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 90, 0, 3, 90, 90, 0, 3, 90, 90, 0, 3, 4,
    360, 14400, -1, 5760, 1440, 360, 120,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    4, 15, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_c66_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment_c66", "superfine", "c64", "standard", NULL, "standard"
  },
  /* 75: Japanese PM-A750 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_YES |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 90, 0, 3, 90, 90, 0, 3, 90, 90, 0, 3, 4,
    360, 14400, -1, 5760, 1440, 360, 120,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 0, 0, 0,
    4, 15, 0, 0,
    c2pl_dotsizes, c2pl_densities, variable_bits, variable_base_res,
    "variable_2pl", "superfine", "c64", "standard", NULL, "standard"
  },
  /* 76: Japanese PM-A890 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2005 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_YES |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 90, 0, 3, 90, 90, 0, 3, 90, 90, 0, 3, 6,
    360, 14400, -1, 5760, 1440, 360, 120,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 0, 0, 0,
    4, 15, 0, 0,
    c2pl_dotsizes, c2pl_densities, variable_bits, variable_base_res,
    "variable_2pl", "superfine", "photo_gen4", "standard", NULL, "standard"
  },
  /* 77: Japanese PM-D600 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 1, 0, 3, 90, 1, 0, 3, 90, 1, 0, 3, 4,
    360, 14400, -1, 2880, 1440, 360, 120,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 0,
    4, 15, 0, 0,
    p3pl_dotsizes, p3pl_densities, variable_bits, variable_base_res,
    "variable_3pl_pmg", "superfine", "c64", "standard", NULL, "photo"
  },
  /* 78: Stylus Photo 810/820 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_YES),
    48, 1, 0, 3, 48, 1, 0, 3, 48, 1, 0, 3, 6,
    360, 14400, -1, 2880, 720, 90, 90,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 720 * 720,
    INCH(19 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    3, 15, 0, 0,
    c4pl_dotsizes, c4pl_2880_densities, variable_bits, variable_base_res,
    "variable_2880_4pl", "2880dpi", "photo_gen2", "standard", NULL, "photo"
  },
  /* 79: Stylus CX6400 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    59, 60, 0, 2, 180, 180, 0, 2, 180, 180, 0, 2, 4,
    360, 14400, -1, 2880, 1440, 360, 180,
    0, 1, 0, 80, 42, -240, 0, 0, 1, 28800, 720 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    4, 15, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c82", "standard", NULL, "standard"
  },
  /* 80: Stylus CX6600 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    59, 60, 0, 2, 180, 180, 0, 2, 180, 180, 0, 2, 4,
    360, 14400, -1, 2880, 2880, 360, 180,
    0, 1, 0, 80, 42, -240, 0, 0, 1, 28800, 1440 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    4, 15, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment", "2880_1440dpi", "c82", "standard", NULL, "standard"
  },
  /* 81: Stylus Photo R260 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2005 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 1, 0, 4, 90, 1, 0, 4, 90, 1, 0, 4, 6,
    360, 14400, -1, 5760, 2880, 360, 90,
    90 * 4, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 5760 * 2880,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, 204, 189, 595, 842, 24,
    4, 15, 0, 0,
    claria_dotsizes, claria_densities, variable_bits, c1_5_base_res,
    "variable_claria", "superfine", "claria", "v2880", NULL, "sp1400"
  },
  /* 82: Stylus Photo 1400 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2005 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 1, 0, 4, 90, 1, 0, 4, 90, 1, 0, 4, 6,
    360, 14400, -1, 5760, 2880, 360, 90,
    90 * 4, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 5760 * 2880,
    INCH(13), INCH(1200), INCH(2), INCH(2), INCH(13), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, 204, 263, 595, 842, 24,
    4, 15, 0, 0,
    claria_1400_dotsizes, claria_1400_densities, variable_bits, claria_1400_base_res,
    "variable_claria_1400", "claria_1400", "claria", "v2880", NULL, "sp1400"
  },
  /* 83: Stylus Photo R240 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_FULL | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 1, 0, 3, 90, 1, 0, 3, 90, 1, 0, 3, 4,
    360, 14400, -1, 5760, 1440, 360, 120,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 24,
    4, 15, 0, 0,
    p3pl_dotsizes, p3pl_densities, variable_bits, variable_base_res,
    "variable_3pl_pmg", "superfine", "photo_gen3_4", "standard", NULL, "standard"
  },
  /* 84: Stylus Photo RX500 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_FULL | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 1, 0, 3, 90, 1, 0, 3, 90, 1, 0, 3, 6,
    360, 14400, -1, 2880, 1440, 360, 120,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 24,
    4, 15, 0, 0,
    p3pl_dotsizes, p3pl_densities, variable_bits, variable_base_res,
    "variable_3pl_pmg", "superfine", "photo_gen3", "standard", NULL, "photo"
  },
  /* 85: Stylus C120 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2005 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_YES | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    59, 60, 0, 2, 360, 1, 0, 1, 360, 1, 0, 1, 4,
    360, 14400, -1, 5760, 2880, 360, 180,
    0, 1, 0, 80, 42, -240, 0, 0, 1, 28800, 5760 * 2880,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 24,
    4, 15, 0, 0,
    c3pl_pigment_c120_dotsizes, c3pl_pigment_c120_densities, variable_bits, variable_base_res,
    "variable_3pl_pigment_c120", "superfine", "c120", "standard", NULL, "c120"
  },
  /* 86: PictureMate 4-color */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ZEROMARGIN_FULL | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    90, 1, 0, 3, 90, 1, 0, 3, 90, 1, 0, 3, 4,
    360, 28800, -1, 5760, 1440, 1440, 720,
    0, 1, 0, 80, 42, 0, 0, 0, 1, 28800, 1440 * 1440,
    INCH(4), INCH(1200), INCH(2), INCH(2), INCH(4), INCH(1200),
    9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 9, 9, 0, 0, 204, 191, 595, 842, 0,
    4, 15, 0, 0,
    picturemate_dotsizes, picturemate_densities, variable_bits, c1_5_base_res,
    "variable_picturemate", "picturemate", "picturemate_4", "picturemate", NULL, "picturemate_4"
  },
  /* 87: B-500DN */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2005 | MODEL_GRAYMODE_YES |
     MODEL_ZEROMARGIN_NO | MODEL_FAST_360_NO |
     MODEL_SEND_ZERO_ADVANCE_YES | MODEL_SUPPORTS_INK_CHANGE_NO |
     MODEL_PACKET_MODE_YES| MODEL_INTERCHANGEABLE_INK_NO |
     MODEL_ENVELOPE_LANDSCAPE_NO),
    360, 1, 0, 1, 360, 1, 0, 1, 360, 1, 0, 1, 4,
    360, 14400, -1, 5760, 2880, 360, 360,
    0, 1, 0, 80, 42, -7, 0, 0, 1, 28800, 720 * 720,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(2), INCH(17 / 2), INCH(1200),
    9, 9, 27, 27, 9, 9, 27, 27, 9, 9, 0, 0, 9, 9, 0, 0, -1, -1, 0, 0, 0,
    4, 15, 0, 0,
    c3pl_pigment_b500_dotsizes, c3pl_pigment_b500_densities, variable_bits, b500_base_res,
    "variable_3pl_pigment_b500", "superfine", "b500", "standard", NULL, "standard"
  },
};

const int stpi_escp2_model_limit =
sizeof(stpi_escp2_model_capabilities) / sizeof(stpi_escp2_printer_t);
