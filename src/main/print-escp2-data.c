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
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include "print-escp2.h"

/*
 * Dot sizes are for:
 *
 *  0: 120/180 DPI micro
 *  1: 120/180 DPI soft
 *  2: 360 micro
 *  3: 360 soft
 *  4: 720x360 micro
 *  5: 720x360 soft
 *  6: 720 micro
 *  7: 720 soft
 *  8: 1440x720 micro
 *  9: 1440x720 soft
 * 10: 2880x720 micro
 * 11: 2880x720 soft
 * 12: 2880x1440
 */

/*   0     1     2     3     4     5     6     7     8     9    10    11    12 */

static const escp2_dot_size_t g1_dotsizes =
{ -2,     -1,   -2,   -1,   -1,   -2,   -2,   -1,   -1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t g2_dotsizes =
{ -2,     -1,   -2,   -1,   -2,   -2,   -2,   -2,   -1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sc600_dotsizes =
{  4,     -1,    4,   -1,   -1,    3,    2,    2,   -1,    1,   -1,   -1,   -1 };

static const escp2_dot_size_t g3_dotsizes =
{  3,     -1,    3,   -1,   -1,    2,    1,    1,   -1,    1,   -1,   -1,   -1 };

static const escp2_dot_size_t photo_dotsizes =
{  3,     -1,    3,   -1,   -1,    2,   -1,    1,   -1,    4,   -1,   -1,   -1 };

static const escp2_dot_size_t sp5000_dotsizes =
{ -1,      3,   -1,    3,   -1,    2,   -1,    1,   -1,    4,   -1,   -1,   -1 };

static const escp2_dot_size_t sc440_dotsizes =
{  3,     -1,    3,   -1,   -1,    2,   -1,    1,   -1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t sc640_dotsizes =
{  3,     -1,    3,   -1,   -1,    2,    1,    1,   -1,    1,   -1,   -1,   -1 };

static const escp2_dot_size_t c6pl_dotsizes =
{ -1,   0x10,   -1, 0x10,   -1, 0x10,   -1, 0x10,   -1, 0x10,   -1, 0x10, 0x10 };

static const escp2_dot_size_t c3pl_dotsizes =
{ -1,   0x11,   -1, 0x11,   -1, 0x11,   -1, 0x10,   -1, 0x10,   -1, 0x10, 0x10 };

static const escp2_dot_size_t c4pl_dotsizes =
{ -1,   0x12,   -1, 0x12,   -1, 0x12,   -1, 0x11,   -1, 0x10,   -1, 0x10, 0x10 };

static const escp2_dot_size_t sc720_dotsizes =
{ -1,   0x12,   -1, 0x12,   -1, 0x11,   -1, 0x11,   -1, 0x11,   -1,   -1,   -1 };

static const escp2_dot_size_t sc660_dotsizes =
{ -1,      3,    3,   -1,    3,    0,   -1,    0,   -1,    0,   -1,   -1,   -1 };

static const escp2_dot_size_t sc480_dotsizes =
{ -1,   0x13,   -1, 0x13,   -1, 0x13,   -1, 0x10,   -1, 0x10,   -1, 0x10, 0x10 };

static const escp2_dot_size_t sc670_dotsizes =
{ -1,   0x12,   -1, 0x12,   -1, 0x12,   -1, 0x11,   -1, 0x11,   -1,   -1,   -1 };

static const escp2_dot_size_t sp2000_dotsizes =
{ -1,   0x11,   -1, 0x11,   -1, 0x11,   -1, 0x10,   -1, 0x10,   -1,   -1,   -1 };

static const escp2_dot_size_t spro_dye_dotsizes =
{    3,   -1,    3,   -1,    3,   -1,    1,   -1,    1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t spro_pigment_dotsizes =
{    3,   -1,    3,   -1,    2,   -1,    1,   -1,    1,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t spro10000_dotsizes =
{    4,   -1, 0x11,   -1, 0x11,   -1, 0x10,   -1, 0x10,   -1,   -1,   -1,   -1 };

static const escp2_dot_size_t c3pl_pigment_dotsizes =
{   -1, 0x10,   -1, 0x10,   -1, 0x10,   -1, 0x11,   -1, 0x12,   -1, 0x12,  0x12 };

static const escp2_dot_size_t c2pl_dotsizes =
{   -1, 0x12,   -1, 0x12,   -1, 0x12,   -1, 0x11,   -1, 0x13,   -1, 0x13,  0x10 };

static const escp2_dot_size_t c4pl_pigment_dotsizes =
{ -1,   0x12,   -1, 0x12,   -1, 0x12,   -1, 0x11,   -1, 0x11,   -1, 0x10, 0x10 };

static const escp2_dot_size_t spro_c4pl_pigment_dotsizes =
{ 0x11,   -1, 0x11,   -1, 0x11,   -1, 0x10,   -1, 0x10,   -1,    5,    5,    5 };

/*
 * Bits are for:
 *
 *  0: 120/180 DPI micro
 *  1: 120/180 DPI soft
 *  2: 360 micro
 *  3: 360 soft
 *  4: 720x360 micro
 *  5: 720x360 soft
 *  6: 720 micro
 *  7: 720 soft
 *  8: 1440x720 micro
 *  9: 1440x720 soft
 * 10: 2880x720 micro
 * 11: 2880x720 soft
 * 12: 2880x1440
 */

/*   0     1     2     3     4     5     6     7     8     9    10    11    12 */

static const escp2_bits_t variable_bits =
{    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2 };

static const escp2_bits_t stp950_bits =
{    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    1 };

static const escp2_bits_t ultrachrome_bits =
{    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,    1,    1,    1 };

static const escp2_bits_t standard_bits =
{    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,    1 };

/*
 * Base resolutions are for:
 *
 *  0: 120/180 DPI micro
 *  1: 120/180 DPI soft
 *  2: 360 micro
 *  3: 360 soft
 *  4: 720x360 micro
 *  5: 720x360 soft
 *  6: 720 micro
 *  7: 720 soft
 *  8: 1440x720 micro
 *  9: 1440x720 soft
 * 10: 2880x720 micro
 * 11: 2880x720 soft
 * 12: 2880x1440
 */

/*   0     1     2     3     4     5     6     7     8     9    10    11    12 */

static const escp2_base_resolutions_t standard_base_res =
{  720,  720,  720,  720,  720,  720,  720,  720,  720,  720,  720,  720,  720 };

static const escp2_base_resolutions_t g3_base_res =
{  720,  720,  720,  720,  720,  720,  720,  720,  360,  360,  360,  360,  360 };

static const escp2_base_resolutions_t variable_base_res =
{  360,  360,  360,  360,  360,  360,  360,  360,  360,  360,  360,  360,  360 };

static const escp2_base_resolutions_t stp950_base_res =
{  360,  360,  360,  360,  360,  360,  360,  360,  360,  360,  720,  720,  720 };

static const escp2_base_resolutions_t ultrachrome_base_res =
{  360,  360,  360,  360,  360,  360,  360,  360,  360,  360,  720,  720,  720 };

static const escp2_base_resolutions_t stc900_base_res =
{  360,  360,  360,  360,  360,  360,  360,  360,  180,  180,  180,  180,  180 };

static const escp2_base_resolutions_t pro_base_res =
{ 2880, 2880, 2880, 2880, 2880, 2880, 2880, 2880, 2880, 2880, 2880, 2880, 2880 };

/*
 * Densities are for:
 *
 *  0: 120/180 DPI micro
 *  1: 120/180 DPI soft
 *  2: 360 micro
 *  3: 360 soft
 *  4: 720x360 micro
 *  5: 720x360 soft
 *  6: 720 micro
 *  7: 720 soft
 *  8: 1440x720 micro
 *  9: 1440x720 soft
 * 10: 2880x720 micro
 * 11: 2880x720 soft
 * 12: 2880x1440
 */

/*  0    1    2    3    4     5      6     7      8      9     10     11     12   */

static const escp2_densities_t g1_densities =
{ 2.6, 2.6, 1.3, 1.3, 1.3,  1.3,  0.568, 0.568,   0.0,   0.0,   0.0,   0.0,   0.0 };

static const escp2_densities_t sc1500_densities =
{ 2.6, 2.6, 1.3, 1.3, 1.3,  1.3,  0.631, 0.631,   0.0,   0.0,   0.0,   0.0,   0.0 };

static const escp2_densities_t g3_densities =
{ 2.6, 2.6, 1.3, 1.3, 1.3,  1.3,  0.775, 0.775, 0.388, 0.388, 0.194, 0.194, 0.097 };

static const escp2_densities_t photo_densities =
{ 2.6, 2.6, 1.3, 1.3, 1.3,  1.3,  0.775, 0.775, 0.55,  0.55,  0.275, 0.275, 0.138 };

static const escp2_densities_t sc440_densities =
{ 4.0, 4.0, 2.0, 2.0, 1.0,  1.0,  0.900, 0.900, 0.45,  0.45,  0.45,  0.45,  0.113 };

static const escp2_densities_t sc480_densities =
{ 2.8, 2.8, 0.0, 1.4, 0.0,  0.7,  0.0,   0.710, 0.0,   0.710, 0.0,   0.546, 0.273 };

static const escp2_densities_t sc980_densities =
{ 2.6, 2.6, 1.3, 1.3, 0.65, 0.65, 0.646, 0.511, 0.49,  0.49,  0.637, 0.637, 0.455 };

static const escp2_densities_t c6pl_densities =
{ 2.6, 4.0, 1.3, 2.0, 0.65, 1.0,  0.646, 0.568, 0.323, 0.568, 0.568, 0.568, 0.284 };

static const escp2_densities_t c3pl_densities =
{ 2.6, 2.6, 1.3, 1.3, 0.65, 0.65, 0.646, 0.73,  0.7,   0.7,   0.91,  0.91,  0.455 };

static const escp2_densities_t sc680_densities =
{ 2.4, 2.4, 1.2, 1.2, 0.60, 0.60, 0.792, 0.792, 0.792, 0.792, 0.594, 0.594, 0.297 };

static const escp2_densities_t c4pl_densities =
{ 2.6, 2.6, 1.3, 1.3, 0.65, 0.65, 0.431, 0.568, 0.784, 0.784, 0.593, 0.593, 0.297 };

static const escp2_densities_t sc660_densities =
{ 4.0, 4.0, 2.0, 2.0, 1.0,  1.0,  0.646, 0.646, 0.323, 0.323, 0.162, 0.162, 0.081 };

static const escp2_densities_t sp2000_densities =
{ 2.6, 2.6, 1.3, 1.3, 0.65, 0.65, 0.775, 0.852, 0.388, 0.438, 0.219, 0.219, 0.110 };

static const escp2_densities_t spro_dye_densities =
{ 2.6, 2.6, 1.3, 1.3, 1.3,  1.3,  0.775, 0.775, 0.388, 0.388, 0.275, 0.275, 0.138 };

static const escp2_densities_t spro_pigment_densities =
{ 3.0, 3.0, 1.5, 1.5, 0.78, 0.78, 0.775, 0.775, 0.388, 0.388, 0.194, 0.194, 0.097 };

static const escp2_densities_t spro10000_densities =
{ 2.6, 2.6, 1.3, 1.3, 0.65, 0.65, 0.431, 0.710, 0.216, 0.784, 0.392, 0.392, 0.196 };

static const escp2_densities_t c3pl_pigment_densities =
{ 2.6, 2.6, 1.3, 1.3, 0.69, 0.69, 0.511, 0.511, 0.765, 0.765, 0.585, 0.585, 0.293 };

static const escp2_densities_t c2pl_densities =
{ 2.3, 2.3, 1.15,1.15,0.57, 0.57, 0.650, 0.650, 0.650, 0.650, 0.650, 0.650, 0.650 };

static const escp2_densities_t c4pl_pigment_densities =
{ 2.7, 2.7, 1.35,1.35,0.68, 0.68, 0.518, 0.518, 0.518, 0.518, 0.518, 0.518, 0.259 };


static const input_slot_t standard_roll_feed_input_slots[] =
{
  {
    "Standard",
    N_("Standard"),
    0,
    0,
    { 16, (char *) "IR\002\000\000\001EX\006\000\000\000\000\000\005\000" },
    { 6, (char *) "IR\002\000\000\000"}
  },
  {
    "Roll",
    N_("Roll Feed"),
    1,
    0,
    { 16, (char *) "IR\002\000\000\001EX\006\000\000\000\000\000\005\001" },
    { 6, (char *) "IR\002\000\000\002" }
  }
};

static const input_slot_list_t standard_roll_feed_input_slot_list =
{
  standard_roll_feed_input_slots,
  sizeof(standard_roll_feed_input_slots) / sizeof(const input_slot_t)
};

static const input_slot_t cutter_roll_feed_input_slots[] =
{
  {
    "Standard",
    N_("Standard"),
    0,
    0,
    { 16, (char *) "IR\002\000\000\001EX\006\000\000\000\000\000\005\000" },
    { 6, (char *) "IR\002\000\000\000"}
  },
  {
    "RollCutPage",
    N_("Roll Feed (cut each page)"),
    1,
    1,
    { 16, (char *) "IR\002\000\000\001EX\006\000\000\000\000\000\005\001" },
    { 6, (char *) "IR\002\000\000\002" }
  },
  {
    "RollCutNone",
    N_("Roll Feed (do not cut)"),
    1,
    0,
    { 16, (char *) "IR\002\000\000\001EX\006\000\000\000\000\000\005\001" },
    { 6, (char *) "IR\002\000\000\002" }
  }
};

static const input_slot_list_t cutter_roll_feed_input_slot_list =
{
  cutter_roll_feed_input_slots,
  sizeof(cutter_roll_feed_input_slots) / sizeof(const input_slot_t)
};

static const input_slot_t pro_roll_feed_input_slots[] =
{
  {
    "Standard",
    N_("Standard"),
    0,
    0,
    { 7, (char *) "PP\003\000\000\002\000" },
    { 0, (char *) "" }
  },
  {
    "Roll",
    N_("Roll Feed"),
    1,
    0,
    { 7, (char *) "PP\003\000\000\003\000" },
    { 0, (char *) "" }
  }
};

static const input_slot_list_t pro_roll_feed_input_slot_list =
{
  pro_roll_feed_input_slots,
  sizeof(pro_roll_feed_input_slots) / sizeof(const input_slot_t)
};

static const input_slot_t sp5000_input_slots[] =
{
  {
    "CutSheet1",
    N_("Cut Sheet Bin 1"),
    0,
    0,
    { 7, (char *) "PP\003\000\000\001\001" },
    { 0, (char *) "" }
  },
  {
    "CutSheet2",
    N_("Cut Sheet Bin 2"),
    0,
    0,
    { 7, (char *) "PP\003\000\000\002\001" },
    { 0, (char *) "" }
  },
  {
    "CutSheetAuto",
    N_("Cut Sheet Autoselect"),
    0,
    0,
    { 7, (char *) "PP\003\000\000\001\377" },
    { 0, (char *) "" }
  },
  {
    "ManualSelect",
    N_("Manual Selection"),
    0,
    0,
    { 7, (char *) "PP\003\000\000\002\001" },
    { 0, (char *) "" }
  }
};

static const input_slot_list_t sp5000_input_slot_list =
{
  sp5000_input_slots,
  sizeof(sp5000_input_slots) / sizeof(const input_slot_t)
};

static const input_slot_list_t default_input_slot_list =
{
  NULL,
  0,
};

static const stp_raw_t new_init_sequence =
{
  29, (char *) "\0\0\0\033\001@EJL 1284.4\n@EJL     \n\033@"
};

static const stp_raw_t je_deinit_sequence =
{
  5, (char *) "JE\001\000\000"
};

#define INCH(x)		(72 * x)

const stpi_escp2_printer_t stpi_escp2_model_capabilities[] =
{
  /* FIRST GENERATION PRINTERS */
  /* 0: Stylus Color */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    15, 1, 4, 15, 1, 4, 15, 1, 4, 4,
    360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    g1_dotsizes, g1_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, standard_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 1: Stylus Color 400/500 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 4,
    360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    g2_dotsizes, g1_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, standard_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 2: Stylus Color 1500 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    1, 1, 1, 1, 1, 1, 1, 1, 1, 4,
    360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17), INCH(44), INCH(2), INCH(4),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    g1_dotsizes, sc1500_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_cmy_inklist,
    standard_bits, standard_base_res, &standard_roll_feed_input_slot_list,
    NULL, NULL
  },
  /* 3: Stylus Color 600 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    32, 1, 4, 32, 1, 4, 32, 1, 4, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    8, 9, 0, 30, 8, 9, 0, 30, 8, 9, 0, 0, 8, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 8,
    sc600_dotsizes, g3_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, g3_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 4: Stylus Color 800 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    64, 1, 2, 64, 1, 2, 64, 1, 2, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    8, 9, 9, 40, 8, 9, 9, 40, 8, 9, 0, 0, 8, 9, 0, 0,
    0, 1, 4, 0, 0, 0, 0,
    g3_dotsizes, g3_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, g3_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 5: Stylus Color 850 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    64, 1, 2, 128, 1, 1, 128, 1, 1, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 4, 0, 0, 0, 0,
    g3_dotsizes, g3_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, g3_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 6: Stylus Color 1520 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    64, 1, 2, 64, 1, 2, 64, 1, 2, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17), INCH(44), INCH(2), INCH(4),
    8, 9, 9, 40, 8, 9, 9, 40, 8, 9, 0, 0, 8, 9, 0, 0,
    0, 1, 4, 0, 0, 0, 0,
    g3_dotsizes, g3_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, g3_base_res, &standard_roll_feed_input_slot_list,
    NULL, NULL
  },

  /* SECOND GENERATION PRINTERS */
  /* 7: Stylus Photo 700 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    32, 1, 4, 32, 1, 4, 32, 1, 4, 6,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 30, 9, 9, 0, 30, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 8,
    photo_dotsizes, photo_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    standard_bits, g3_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 8: Stylus Photo EX */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    32, 1, 4, 32, 1, 4, 32, 1, 4, 6,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(118 / 10), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 30, 9, 9, 0, 30, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 8,
    photo_dotsizes, photo_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    standard_bits, g3_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 9: Stylus Photo */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    32, 1, 4, 32, 1, 4, 32, 1, 4, 6,
    360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 30, 9, 9, 0, 30, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 8,
    photo_dotsizes, photo_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    standard_bits, g3_base_res, &default_input_slot_list,
    NULL, NULL
  },

  /* THIRD GENERATION PRINTERS */
  /* 10: Stylus Color 440/460 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    21, 1, 4, 21, 1, 4, 21, 1, 4, 4,
    360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 8,
    sc440_dotsizes, sc440_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_no_microweave_reslist, &stpi_escp2_standard_inklist,
    standard_bits, standard_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 11: Stylus Color 640 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    32, 1, 4, 32, 1, 4, 32, 1, 4, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 8,
    sc640_dotsizes, sc440_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, standard_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 12: Stylus Color 740/Stylus Scan 2000/Stylus Scan 2500 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 144, 1, 1, 144, 1, 1, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    c6pl_dotsizes, c6pl_densities,
    &stpi_escp2_variable_6pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 13: Stylus Color 900 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    96, 1, 2, 192, 1, 1, 192, 1, 1, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    c3pl_dotsizes, c3pl_densities,
    &stpi_escp2_variable_3pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    variable_bits, stc900_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 14: Stylus Photo 750 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 6,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    c6pl_dotsizes, c6pl_densities,
    &stpi_escp2_variable_6pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 15: Stylus Photo 1200 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 6,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    c6pl_dotsizes, c6pl_densities,
    &stpi_escp2_variable_6pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &standard_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 16: Stylus Color 860 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 144, 1, 1, 144, 1, 1, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 17: Stylus Color 1160 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 144, 1, 1, 144, 1, 1, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 18: Stylus Color 660 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    32, 1, 4, 32, 1, 4, 32, 1, 4, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 9, 9, 9, 9, 9, 26, 9, 9, 9, 0, 9, 9, 9, 0,
    0, 1, 8, 0, 0, 0, 8,
    sc660_dotsizes,sc660_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, standard_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 19: Stylus Color 760 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 144, 1, 1, 144, 1, 1, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 20: Stylus Photo 720 (Australia) */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    32, 1, 4, 32, 1, 4, 32, 1, 4, 6,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    sc720_dotsizes, c6pl_densities,
    &stpi_escp2_variable_6pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 21: Stylus Color 480 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    15, 15, 3, 48, 48, 3, 48, 48, 3, 4,
    360, 720, 720, 14400, 360, 720, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, -99, 0, 0,
    sc480_dotsizes, sc480_densities,
    &stpi_escp2_variable_x80_6pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_x80_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 22: Stylus Photo 870/875 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 6,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 9, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 97, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &standard_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 23: Stylus Photo 1270 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 6,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 9, 0, 0, 0, 9, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 97, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &standard_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 24: Stylus Color 3000 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    64, 1, 2, 128, 1, 1, 128, 1, 1, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17), INCH(44), INCH(2), INCH(4),
    8, 9, 9, 40, 8, 9, 9, 40, 8, 9, 0, 0, 8, 9, 0, 0,
    0, 1, 4, 0, 0, 0, 0,
    g3_dotsizes, g3_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, g3_base_res, &standard_roll_feed_input_slot_list,
    NULL, NULL
  },
  /* 25: Stylus Color 670 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    32, 1, 4, 64, 1, 2, 64, 1, 2, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    sc670_dotsizes, c6pl_densities,
    &stpi_escp2_variable_6pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 26: Stylus Photo 2000P */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 144, 1, 1, 144, 1, 1, 6,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    sp2000_dotsizes, sp2000_densities,
    &stpi_escp2_variable_pigment_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 27: Stylus Pro 5000 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    64, 1, 2, 64, 1, 2, 64, 1, 2, 6,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 30, 9, 9, 0, 30, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 4,
    sp5000_dotsizes, photo_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_sp5000_reslist, &stpi_escp2_photo_inklist,
    standard_bits, g3_base_res, &sp5000_input_slot_list,
    NULL, NULL
  },
  /* 28: Stylus Pro 7000 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    1, 1, 1, 1, 1, 1, 1, 1, 1, 6,
    360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(24), INCH(1200), INCH(7), INCH(7),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 9, 9, 9, 9, 9, 9,
    0, 1, 0, 0, 0, 0, 0,
    spro_dye_dotsizes, spro_dye_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_pro_reslist, &stpi_escp2_photo_inklist,
    standard_bits, pro_base_res, &pro_roll_feed_input_slot_list,
    NULL, NULL
  },
  /* 29: Stylus Pro 7500 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_YES |
     MODEL_FAST_360_NO),
    1, 1, 1, 1, 1, 1, 1, 1, 1, 6,
    360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(24), INCH(1200), INCH(7), INCH(7),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 9, 9, 9, 9, 9, 9,
    0, 1, 0, 0, 0, 0, 0,
    spro_pigment_dotsizes, spro_pigment_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_pro_reslist, &stpi_escp2_photo_inklist,
    standard_bits, pro_base_res, &pro_roll_feed_input_slot_list,
    NULL, NULL
  },
  /* 30: Stylus Pro 9000 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    1, 1, 1, 1, 1, 1, 1, 1, 1, 6,
    360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(44), INCH(1200), INCH(7), INCH(7),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 9, 9, 9, 9, 9, 9,
    0, 1, 0, 0, 0, 0, 0,
    spro_dye_dotsizes, spro_dye_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_pro_reslist, &stpi_escp2_photo_inklist,
    standard_bits, pro_base_res, &pro_roll_feed_input_slot_list,
    NULL, NULL
  },
  /* 31: Stylus Pro 9500 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_YES |
     MODEL_FAST_360_NO),
    1, 1, 1, 1, 1, 1, 1, 1, 1, 6,
    360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(44), INCH(1200), INCH(7), INCH(7),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 9, 9, 9, 9, 9, 9,
    0, 1, 0, 0, 0, 0, 0,
    spro_pigment_dotsizes, spro_pigment_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_pro_reslist, &stpi_escp2_photo_inklist,
    standard_bits, pro_base_res, &pro_roll_feed_input_slot_list,
    NULL, NULL
  },
  /* 32: Stylus Color 777/680 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 144, 1, 1, 144, 1, 1, 4,
    360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    c4pl_dotsizes, sc680_densities,
    &stpi_escp2_variable_680_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 33: Stylus Color 880/83/C60 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 144, 1, 1, 144, 1, 1, 4,
    360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 34: Stylus Color 980 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    96, 1, 2, 192, 1, 1, 192, 1, 1, 4,
    360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    192, 1, 0, 0, 0, 0, 0,
    c3pl_dotsizes, sc980_densities,
    &stpi_escp2_variable_3pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 35: Stylus Photo 780/790/810/820 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_YES | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 6,
    360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 55, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_sp780_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 36: Stylus Photo 785/890/895/915 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 6,
    360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 55, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &standard_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 37: Stylus Photo 1280/1290 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 6,
    360, 720, 720, 14400, -1, 2880, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 55, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &standard_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 38: Stylus Color 580 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    15, 15, 3, 48, 48, 3, 48, 48, 3, 4,
    360, 720, 720, 14400, 360, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, -99, 0, 0,
    sc480_dotsizes, sc480_densities,
    &stpi_escp2_variable_x80_6pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_x80_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 39: Stylus Color Pro XL */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 4,
    360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    g1_dotsizes, g1_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, standard_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 40: Stylus Pro 5500 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_YES |
     MODEL_FAST_360_NO),
    1, 1, 1, 1, 1, 1, 1, 1, 1, 6,
    360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(1200), INCH(2), INCH(4),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    spro_pigment_dotsizes, spro_pigment_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_pro_reslist, &stpi_escp2_photo_inklist,
    standard_bits, pro_base_res, &sp5000_input_slot_list,
    NULL, NULL
  },
  /* 41: Stylus Pro 10000 */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_PRO | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_YES |
     MODEL_FAST_360_NO),
    1, 1, 1, 1, 1, 1, 1, 1, 1, 6,
    360, 1440, 1440, 14400, -1, 1440, 720, 90, 90,
    INCH(44), INCH(1200), INCH(7), INCH(7),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 9, 9, 9, 9, 9, 9,
    0, 1, 0, 0, 0, 0, 0,
    spro10000_dotsizes, spro10000_densities,
    &stpi_escp2_spro10000_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_pro_reslist, &stpi_escp2_photo_inklist,
    variable_bits, pro_base_res, &pro_roll_feed_input_slot_list,
    NULL, NULL
  },
  /* 42: Stylus C20SX/C20UX */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    15, 15, 3, 48, 48, 3, 48, 48, 3, 4,
    360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, -99, 0, 0,
    sc480_dotsizes, sc480_densities,
    &stpi_escp2_variable_x80_6pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_x80_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 43: Stylus C40SX/C40UX/C41SX/C41UX/C42SX/C42UX */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    15, 15, 3, 48, 48, 3, 48, 48, 3, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, -99, 0, 0,
    sc480_dotsizes, sc480_densities,
    &stpi_escp2_variable_x80_6pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_x80_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 44: Stylus C70/C80 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    60, 60, 2, 180, 180, 2, 180, 180, 2, 4,
    360, 720, 720, 14400, -1, 2880, 1440, 360, 180,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, -240, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_densities,
    &stpi_escp2_variable_3pl_pigment_inks, &stpi_escp2_c80_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_c80_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 45: Stylus Color Pro */
  {
    (MODEL_VARIABLE_NO | MODEL_COMMAND_1998 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 4,
    360, 720, 720, 14400, -1, 720, 720, 90, 90,
    INCH(17 / 2), INCH(44), INCH(2), INCH(4),
    9, 9, 9, 40, 9, 9, 9, 40, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    g1_dotsizes, g1_densities,
    &stpi_escp2_simple_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    standard_bits, standard_base_res, &default_input_slot_list,
    NULL, NULL
  },
  /* 46: Stylus Photo 950/960 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_VACUUM_NO |
     MODEL_FAST_360_YES),
    96, 96, 2, 96, 96, 2, 24, 24, 1, 6,
    360, 720, 720, 14400, -1, 2880, 1440, 360, 180,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 190, 0, 0, 0,
    c2pl_dotsizes, c2pl_densities,
    &stpi_escp2_variable_2pl_inks, &stpi_escp2_sp950_paper_list,
    stpi_escp2_escp950_reslist, &stpi_escp2_photo_inklist,
    stp950_bits, stp950_base_res, &cutter_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 47: Stylus Photo 2100/2200 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_VACUUM_NO |
     MODEL_FAST_360_YES),
    96, 96, 2, 96, 96, 2, 192, 192, 1, 7,
    360, 720, 720, 14400, -1, 2880, 1440, 360, 180,
    INCH(13), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 190, 0, 0, 0,
    c4pl_pigment_dotsizes, c4pl_pigment_densities,
    &stpi_escp2_variable_4pl_pigment_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_escp2200_reslist, &stpi_escp2_photo7_inklist,
    ultrachrome_bits, ultrachrome_base_res, &cutter_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 48: Stylus Pro 7600 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_PRO | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_YES |
     MODEL_FAST_360_NO),
    1, 1, 1, 1, 1, 1, 1, 1, 1, 7,
    360, 2880, 2880, 14400, -1, 2880, 1440, 360, 180,
    INCH(24), INCH(1200), INCH(7), INCH(7),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    spro_c4pl_pigment_dotsizes, c4pl_pigment_densities,
    &stpi_escp2_variable_4pl_pigment_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_pro_reslist, &stpi_escp2_photo7_inklist,
    ultrachrome_bits, pro_base_res, &pro_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 49: Stylus Pro 9600 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_PRO | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_YES |
     MODEL_FAST_360_NO),
    1, 1, 1, 1, 1, 1, 1, 1, 1, 7,
    360, 2880, 2880, 14400, -1, 2880, 1440, 360, 180,
    INCH(44), INCH(1200), INCH(7), INCH(7),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    spro_c4pl_pigment_dotsizes, c4pl_pigment_densities,
    &stpi_escp2_variable_4pl_pigment_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_pro_reslist, &stpi_escp2_photo7_inklist,
    ultrachrome_bits, pro_base_res, &pro_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 50: Stylus Photo 825/830 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_YES | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 6,
    360, 720, 720, 14400, -1, 2880, 1440, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 55, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_sp780_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 51: Stylus Photo 925 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 48, 1, 3, 48, 1, 3, 6,
    360, 720, 720, 14400, -1, 2880, 1440, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 55, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &cutter_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 52: Stylus Color C62 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    48, 1, 3, 144, 1, 1, 144, 1, 1, 4,
    360, 720, 720, 14400, -1, 2880, 1440, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_standard_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 53: Japanese PM-950C */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_YES | MODEL_XZEROMARGIN_YES | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    96, 1, 2, 96, 1, 2, 96, 1, 2, 6,
    360, 720, 720, 14400, -1, 2880, 1440, 360, 180,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 0, 190, 0, 0, 0,
    c2pl_dotsizes, c2pl_densities,
    &stpi_escp2_variable_2pl_inks, &stpi_escp2_sp950_paper_list,
    stpi_escp2_escp950_reslist, &stpi_escp2_photo7_japan_inklist,
    stp950_bits, stp950_base_res, &cutter_roll_feed_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 54: Stylus Photo EX3 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    32, 1, 4, 32, 1, 4, 32, 1, 4, 6,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(13), INCH(44), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 0, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, 0, 0, 0,
    sc720_dotsizes, c6pl_densities,
    &stpi_escp2_variable_6pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_photo_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 55: Stylus C82/CX-5200 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    59, 60, 2, 180, 180, 2, 180, 180, 2, 4,
    360, 720, 720, 14400, -1, 2880, 1440, 360, 180,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, -240, 0, 0,
    c3pl_pigment_dotsizes, c3pl_pigment_densities,
    &stpi_escp2_variable_3pl_pigment_inks, &stpi_escp2_c80_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_c80_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
  /* 56: Stylus C50 */
  {
    (MODEL_VARIABLE_YES | MODEL_COMMAND_2000 | MODEL_GRAYMODE_YES |
     MODEL_ROLLFEED_NO | MODEL_XZEROMARGIN_NO | MODEL_VACUUM_NO |
     MODEL_FAST_360_NO),
    15, 15, 3, 48, 48, 3, 48, 48, 3, 4,
    360, 720, 720, 14400, -1, 1440, 720, 90, 90,
    INCH(17 / 2), INCH(1200), INCH(2), INCH(4),
    9, 9, 0, 9, 9, 9, 9, 9, 9, 9, 0, 0, 9, 9, 0, 0,
    0, 1, 0, 0, -99, 0, 0,
    c4pl_dotsizes, c4pl_densities,
    &stpi_escp2_variable_4pl_inks, &stpi_escp2_standard_paper_list,
    stpi_escp2_standard_reslist, &stpi_escp2_x80_inklist,
    variable_bits, variable_base_res, &default_input_slot_list,
    &new_init_sequence, &je_deinit_sequence
  },
};

const int stpi_escp2_model_limit =
sizeof(stpi_escp2_model_capabilities) / sizeof(stpi_escp2_printer_t);
