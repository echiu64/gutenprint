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

static const res_t r_360x90dpi =
{ "360x90dpi",     N_("360 x 90 DPI Fast Economy"),
  360,  90,   0, 0, 1 };
static const res_t r_360x90sw =
{ "360x90sw",      N_("360 x 90 DPI Fast Economy"),
  360,  90,   1, 0, 1 };

static const res_t r_360x120dpi =
{ "360x120dpi",    N_("360 x 120 DPI Economy"),
  360,  120,  0, 0, 1 };
static const res_t r_360x120sw =
{ "360x120sw",     N_("360 x 120 DPI Economy"),
  360,  120,  1, 0, 1 };

static const res_t r_180dpi =
{ "180dpi",        N_("180 DPI Economy"),
  180,  180,  0, 0, 1 };
static const res_t r_180sw =
{ "180sw",         N_("180 DPI Economy"),
  180,  180,  1, 0, 1 };

static const res_t r_360x180dpi =
{ "360x180dpi",    N_("360 x 180 DPI Draft"),
  360,  180,  0, 0, 1 };
static const res_t r_360x180sw =
{ "360x180sw",     N_("360 x 180 DPI Draft"),
  360,  180,  1, 0, 1 };

static const res_t r_360x240dpi =
{ "360x240dpi",    N_("360 x 240 DPI Draft"),
  360,  240,  0, 0, 1 };
static const res_t r_360x240sw =
{ "360x240sw",     N_("360 x 240 DPI Draft"),
  360,  240,  1, 0, 1 };

static const res_t r_360mw =
{ "360mw",         N_("360 DPI Microweave"),
  360,  360,  0, 1, 1 };
static const res_t r_360dpi =
{ "360dpi",        N_("360 DPI"),
  360,  360,  0, 0, 1 };
static const res_t r_360sw =
{ "360sw",         N_("360 DPI"),
  360,  360,  1, 0, 1 };

static const res_t r_720x360sw =
{ "720x360sw",     N_("720 x 360 DPI"),
  720,  360,  1, 0, 1 };

static const res_t r_720mw =
{ "720mw",         N_("720 DPI Microweave"),
  720,  720,  0, 1, 1 };
static const res_t r_720sw =
{ "720sw",         N_("720 DPI"),
  720,  720,  1, 0, 1 };
static const res_t r_720hq =
{ "720hq",         N_("720 DPI High Quality"),
  720,  720,  1, 0, 2 };
static const res_t r_720hq2 =
{ "720hq2",        N_("720 DPI Highest Quality"),
  720,  720,  1, 0, 4 };

static const res_t r_1440x720sw =
{ "1440x720sw",    N_("1440 x 720 DPI"),
  1440, 720,  1, 0, 1 };
static const res_t r_1440x720hq2 =
{ "1440x720hq2",   N_("1440 x 720 DPI Highest Quality"),
  1440, 720,  1, 0, 2 };

static const res_t r_2880x720sw =
{ "2880x720sw",    N_("2880 x 720 DPI"),
  2880, 720,  1, 0, 1};

static const res_t r_1440x1440sw =
{ "1440x1440sw",   N_("1440 x 1440 DPI"),
  1440, 1440, 1, 0, 1};

static const res_t r_2880x1440sw =
{ "2880x1440sw",   N_("2880 x 1440 DPI"),
  2880, 1440, 1, 0, 1};

static const res_t r_2880x2880sw =
{ "2880x2880sw",   N_("2880 x 2880 DPI"),
  2880, 2880, 1, 0, 1};



const res_t *const stpi_escp2_720dpi_reslist[] =
{
  &r_360x90dpi,

  &r_360x120dpi,

  &r_180dpi,

  &r_360x240dpi,

  &r_360x180dpi,

  &r_360mw,
  &r_360dpi,

  &r_720x360sw,

  &r_720mw,

  NULL
};

const res_t *const stpi_escp2_1440dpi_reslist[] =
{
  &r_360x90sw,

  &r_360x120sw,

  &r_180sw,

  &r_360x240sw,

  &r_360x180sw,

  &r_360sw,

  &r_720x360sw,

  &r_720sw,

  &r_1440x720sw,
  &r_1440x720hq2,

  NULL
};

const res_t *const stpi_escp2_standard_reslist[] =
{
  &r_360x90sw,

  &r_360x120sw,

  &r_180sw,

  &r_360x240sw,

  &r_360x180sw,

  &r_360sw,

  &r_720x360sw,

  &r_720sw,

  &r_1440x720sw,

  &r_2880x720sw,

  &r_1440x1440sw,

  &r_2880x1440sw,

  NULL
};

const res_t *const stpi_escp2_g3_reslist[] =
{
  &r_360x90dpi,

  &r_360x120dpi,

  &r_180dpi,

  &r_360x240dpi,

  &r_360x180dpi,

  &r_360mw,
  &r_360dpi,

  &r_720x360sw,

  &r_720sw,
  &r_720hq,

  &r_1440x720sw,
  &r_1440x720hq2,

  NULL
};

const res_t *const stpi_escp2_superfine_reslist[] =
{
  &r_360x180sw,

  &r_360sw,

  &r_720x360sw,

  &r_720sw,

  &r_1440x720sw,

  &r_2880x1440sw,

  &r_2880x2880sw,

  NULL
};

const res_t *const stpi_escp2_720dpi_soft_reslist[] =
{
  &r_360x90dpi,

  &r_360x120sw,

  &r_180dpi,

  &r_360x240sw,

  &r_360x180dpi,

  &r_360mw,
  &r_360dpi,

  &r_720x360sw,

  &r_720sw,
  &r_720hq,
  &r_720hq2,

  NULL
};

const res_t *const stpi_escp2_sc500_reslist[] =
{
  &r_360x90dpi,

  &r_360x120dpi,

  &r_180dpi,

  &r_360x240dpi,

  &r_360x180dpi,

  &r_360mw,
  &r_360dpi,

  &r_720x360sw,

  &r_720mw,
  &r_720sw,
  &r_720hq,
  &r_720hq2,

  NULL
};

const res_t *const stpi_escp2_sc640_reslist[] =
{
  &r_360x90dpi,

  &r_180dpi,

  &r_360x180dpi,

  &r_360mw,
  &r_360dpi,

  &r_720x360sw,

  &r_720mw,
  &r_720sw,

  &r_1440x720sw,
  &r_1440x720hq2,

  NULL
};

const res_t *const stpi_escp2_sc660_reslist[] =
{
  &r_360x90sw,

  &r_180sw,

  &r_360x180sw,

  &r_360mw,
  &r_360dpi,

  &r_720x360sw,

  &r_720sw,

  &r_1440x720sw,
  &r_1440x720hq2,

  NULL
};

static const res_t r_360fol =
{ "360fol",        N_("360 DPI Full Overlap"),
  360,  360,  0, 2, 1 };
static const res_t r_360fol2 =
{ "360fol2",       N_("360 DPI FOL2"),
  360,  360,  0, 4, 1 };
static const res_t r_360mw2 =
{ "360mw2",        N_("360 DPI MW2"),
  360,  360,  0, 5, 1 };

static const res_t r_720x360dpi =
{ "720x360dpi",    N_("720 x 360 DPI"),
  720,  360,  0, 0, 1 };
static const res_t r_720x360mw =
{ "720x360mw",     N_("720 x 360 DPI Microweave"),
  720,  360,  0, 1, 1 };
static const res_t r_720x360fol =
{ "720x360fol",    N_("720 x 360 DPI FOL"),
  720,  360,  0, 2, 1 };
static const res_t r_720x360fol2 =
{ "720x360fol2",   N_("720 x 360 DPI FOL2"),
  720,  360,  0, 4, 1 };
static const res_t r_720x360mw2 =
{ "720x360mw2",    N_("720 x 360 DPI MW2"),
  720,  360,  0, 5, 1 };

static const res_t r_720fol =
{ "720fol",        N_("720 DPI Full Overlap"),
  720,  720,  0, 2, 1 };
static const res_t r_720fourp =
{ "720fourp",      N_("720 DPI Four Pass"),
  720,  720,  0, 3, 1 };

static const res_t r_1440x720mw =
{ "1440x720mw",    N_("1440 x 720 DPI Microweave"),
  1440, 720,  0, 1, 1 };
static const res_t r_1440x720fol =
{ "1440x720fol",   N_("1440 x 720 DPI FOL"),
  1440, 720,  0, 2, 1 };
static const res_t r_1440x720fourp =
{ "1440x720fourp", N_("1440 x 720 DPI Four Pass"),
  1440, 720,  0, 3, 1 };

static const res_t r_2880x720mw =
{ "2880x720mw",    N_("2880 x 720 DPI Microweave"),
  2880, 720,  0, 1, 1 };
static const res_t r_2880x720fol =
{ "2880x720fol",   N_("2880 x 720 DPI FOL"),
  2880, 720,  0, 2, 1 };
static const res_t r_2880x720fourp =
{ "2880x720fourp", N_("2880 x 720 DPI Four Pass"),
  2880, 720,  0, 3, 1 };

static const res_t r_1440x1440mw =
{ "1440x1440mw",    N_("1440 x 1440 DPI Microweave"),
  1440, 1440, 0, 1, 1 };
static const res_t r_1440x1440fol =
{ "1440x1440fol",   N_("1440 x 1440 DPI FOL"),
  1440, 1440, 0, 2, 1 };
static const res_t r_1440x1440fourp =
{ "1440x1440fourp", N_("1440 x 1440 DPI Four Pass"),
  1440, 1440, 0, 3, 1 };

static const res_t r_2880x1440mw =
{ "2880x1440mw",    N_("2880 x 1440 DPI Microweave"),
  2880, 1440, 0, 1, 1 };
static const res_t r_2880x1440fol =
{ "2880x1440fol",   N_("2880 x 1440 DPI FOL"),
  2880, 1440, 0, 2, 1 };
static const res_t r_2880x1440fourp =
{ "2880x1440fourp", N_("2880 x 1440 DPI Four Pass"),
  2880, 1440, 0, 3, 1 };

const res_t *const stpi_escp2_pro_reslist[] =
{
  &r_360x90dpi,

  &r_360x120dpi,

  &r_180dpi,

  &r_360x240dpi,

  &r_360x180dpi,

  &r_360mw,
  &r_360dpi,
  &r_360fol,
  &r_360fol2,
  &r_360mw2,

  &r_720x360dpi,
  &r_720x360mw,
  &r_720x360fol,
  &r_720x360fol2,
  &r_720x360mw2,

  &r_720mw,
  &r_720fol,
  &r_720fourp,

  &r_1440x720mw,
  &r_1440x720fol,
  &r_1440x720fourp,

  &r_2880x720mw,
  &r_2880x720fol,
  &r_2880x720fourp,

  &r_1440x1440mw,
  &r_1440x1440fol,
  &r_1440x1440fourp,

  &r_2880x1440mw,
  &r_2880x1440fol,
  &r_2880x1440fourp,

  NULL
};
