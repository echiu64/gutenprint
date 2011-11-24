/*
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
 *   Copyright (c) 2006 - 2007 Sascha Sommer (saschasommer@freenet.de)
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

/* This file contains definitions for the various printmodes
*/

#ifndef GUTENPRINT_INTERNAL_CANON_MODES_H
#define GUTENPRINT_INTERNAL_CANON_MODES_H

static const char iP4200_300dpi_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.60 1.60 1.60 1.60 1.60 1.60 1.60 1.60 "  /* B */
/* B */  "1.60 1.60 1.60 1.60 1.60 1.60 1.60 1.60 "  /* M */
/* M */  "1.60 1.60 1.55 1.50 1.45 1.40 1.35 1.35 "  /* R */
/* R */  "1.35 1.35 1.35 1.35 1.35 1.35 1.35 1.35 "  /* Y */
/* Y */  "1.35 1.42 1.51 1.58 1.60 1.60 1.60 1.60 "  /* G */
/* G */  "1.60 1.60 1.60 1.60 1.60 1.60 1.60 1.60 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char iP4200_300dpi_draft_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "2.13 2.15 2.20 2.25 2.30 2.35 2.40 2.40 "  /* B */
/* B */  "2.40 2.40 2.35 2.30 2.22 2.10 2.08 1.92 "  /* M */
/* M */  "1.90 1.85 1.80 1.70 1.60 1.55 1.42 1.35 "  /* R */
/* R */  "1.35 1.35 1.35 1.35 1.30 1.34 1.38 1.40 "  /* Y */
/* Y */  "1.40 1.45 1.55 1.68 1.80 1.92 2.02 2.10 "  /* G */
/* G */  "2.10 2.05 1.95 1.90 2.00 2.10 2.11 2.13 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

/* delay settings 
 sometimes the raster data has to be sent 
 | K     |
   | C     |
     | M     |
       | Y     |
 instead of 
 | K
 | C
 | M
 | Y

 following tables can be used to account for this

*/

typedef struct {
    unsigned char color;
    unsigned int delay;
} canon_delay_t;

/* delay settings for the different printmodes  last entry has to be {0,0} */
static const canon_delay_t delay_1440[] = {{'C',112},{'M',224},{'Y',336},{'c',112},{'m',224},{'y',336},{0,0}};
static const canon_delay_t delay_S200[] = {{'C',0x30},{'M',0x50},{'Y',0x70},{0,0}};



/*
 * A printmode is defined by its resolution (xdpi x ydpi), the inkset
 * and the installed printhead.
 *
 * For a hereby defined printmode we specify the density and gamma multipliers
 * and the ink definition with optional adjustments for lum, hue and sat
 *
 */
typedef struct {
  const int xdpi;                      /* horizontal resolution */
  const int ydpi;                      /* vertical resolution   */
  const unsigned int ink_types;        /* the used color channels */
  const char* name;                    /* internal name do not translate, must not contain spaces */
  const char* text;                    /* description */
  const int num_inks;
  const canon_inkset_t *inks;          /* ink definition        */
  const int raster_lines_per_block;    /* number of raster lines in every F) command */
  const unsigned int flags;
#define MODE_FLAG_WEAVE 0x1            /* this mode requires weaving */
#define MODE_FLAG_EXTENDED_T 0x2       /* this mode requires extended color settings in the esc t) command */
#define MODE_FLAG_CD 0x4               /* this mode can be used to print to cds */
#define MODE_FLAG_PRO 0x8              /* special ink settings for the PIXMA Pro9500 not sure of this... maybe 0x4 */
#define MODE_FLAG_IP8500 0x10          /* special ink settings for the PIXMA iP8500 */
#define MODE_FLAG_MP360 0x20           /* special ink settings for the PIXMA MP360 */
#define MODE_FLAG_MP130 0x40           /* special ink settings for early devices */
#define MODE_FLAG_S 0x80               /* mode-related setup command on iP90/iP9v and iP100 */
  const canon_delay_t* delay;          /* delay settings for this printmode */
  const double density;                /* density multiplier    */
  const double gamma;                  /* gamma multiplier      */
  const char *lum_adjustment;          /* optional lum adj.     */
  const char *hue_adjustment;          /* optional hue adj.     */
  const char *sat_adjustment;          /* optional sat adj.     */
  const int quality;                   /* quality (unused for some printers, default is usually 2) */
} canon_mode_t;

#define INKSET(a) sizeof(canon_##a##_inkset)/sizeof(canon_inkset_t),canon_##a##_inkset


typedef struct {
  const char *name;
  const short count;
  const short default_mode;
  const canon_mode_t *modes;
} canon_modelist_t;

#define DECLARE_MODES(name,default_mode)               \
static const canon_modelist_t name##_modelist = {      \
  #name,                                               \
  sizeof(name##_modes) / sizeof(canon_mode_t),         \
  default_mode,                                        \
  name##_modes                                         \
}


/* modelist for every printer,modes ordered by ascending resolution ink_type and quality */
static const canon_mode_t canon_BJC_30_modes[] = {
  {  180, 180,CANON_INK_K,"180x180dpi",N_("180x180 DPI"),INKSET(1_K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K,"360x360dpi",N_("360x360 DPI"),INKSET(1_K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 360,CANON_INK_K,"720x360dpi",N_("720x360 DPI"),INKSET(1_K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_30,0);


static const canon_mode_t canon_BJC_85_modes[] = {
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
     "360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
     "360x360dmt",N_("360x360 DPI DMT"),INKSET(6_C4M4Y4K4c4m4),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
     "720x360dpi",N_("720x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_85,0);


/* we treat the printers that can either print in K or CMY as CMYK printers here by assigning a CMYK inkset */
static const canon_mode_t canon_BJC_210_modes[] = {
  {   90,  90,CANON_INK_K | CANON_INK_CMY,"90x90dpi",N_("90x90 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  180, 180,CANON_INK_K | CANON_INK_CMY,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 360,CANON_INK_K | CANON_INK_CMY,"720x360dpi",N_("720x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_210,0);


static const canon_mode_t canon_BJC_240_modes[] = {
  {   90,  90,CANON_INK_K | CANON_INK_CMY,"90x90dpi",N_("90x90 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  180, 180,CANON_INK_K | CANON_INK_CMY,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dmt",N_("360x360 DMT"),INKSET(4_C4M4Y4K4),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 360,CANON_INK_K | CANON_INK_CMY,"720x360dpi",N_("720x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_240,0);


static const canon_mode_t canon_BJC_2000_modes[] = {
  {  180, 180,CANON_INK_CMYK,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_2000,0);


static const canon_mode_t canon_BJC_3000_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dmt",N_("360x360 DPI DMT"),INKSET(6_C4M4Y4K4c4m4),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"720x720dpi",N_("720x720 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"1440x720dpi",N_("1440x720 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,delay_1440,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_3000,0);


static const canon_mode_t canon_BJC_4300_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dmt",N_("360x360 DPI DMT"),INKSET(6_C4M4Y4K4c4m4),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"720x720dpi",N_("720x720 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"1440x720dpi",N_("1440x720 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,delay_1440,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_4300,0);



static const canon_mode_t canon_BJC_4400_modes[] = {
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
     "360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
     "720x360dpi",N_("720x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_4400,0);


static const canon_mode_t canon_BJC_5500_modes[] = {
  {  180, 180,CANON_INK_CMYK | CANON_INK_CcMmYK,"180x180dpi",N_("180x180 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_5500,0);


static const canon_mode_t canon_BJC_6000_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.8,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dmt",N_("360x360 DPI DMT"),INKSET(6_C4M4Y4K4c4m4),8,0,NULL,1.8,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"720x720dpi",N_("720x720 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"1440x720dpi",N_("1440x720 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,delay_1440,0.5,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_6000,0);


static const canon_mode_t canon_BJC_7000_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,3.5,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.8,1.0,NULL,NULL,NULL,2},
  { 1200, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"1200x600dpi",N_("1200x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_7000,0);

static const canon_mode_t canon_BJC_7100_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  { 1200, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"1200x600dpi",N_("1200x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_7100,0);

static const canon_mode_t canon_BJC_i50_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM gloss"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/coated"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i50,0);

static const canon_mode_t canon_BJC_i70_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM gloss"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/coated"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI MEDIUM Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},/* ostensibly mono only */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* ostensibly color only */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i70,0);

static const canon_mode_t canon_BJC_i80_modes[] = {
  /* original modes */
  /* {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2}, */
  /* {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2}, */
  /* plain modes --- Env/Hagaki same */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(9_C4M4Y4K2c4m4y4plain2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYKcmy */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* mono only---color untested*/
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes --- cmy not supported, only CMYcmy */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO gloss"),INKSET(9_C4M4Y4K2c4m4y4photo2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4photo2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i80,0);

/* not known what is format of sending of logical color data, just sequence of (z commands like some iP devices */
/* all modes use MP360 flag */
static const canon_mode_t canon_BJC_i250_modes[] = {
  /* plain modes --- Env/Hagaki same (except no draft mode) */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2h),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(9_C2M2Y2K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(9_C2M2Y2K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono modes */
  {  600, 600,CANON_INK_K,"300x300dpi_highmono",N_("300x300 DPI HIGH MONO"),INKSET(9_K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(9_K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO"),INKSET(9_K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI"),INKSET(9_K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT"),INKSET(9_K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(9_C7M7Y7K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM gloss"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/coated"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2h),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i250,0);

/* not known what is format of sending of logical color data, just sequence of (z commands like some iP devices */
/* all modes use MP360 flag */
static const canon_mode_t canon_BJC_i320_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI MEDIUM"),INKSET(9_C3M3Y2K2h),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2h),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(9_C2M2Y2K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(9_C2M2Y2K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono modes */
  {  600, 600,CANON_INK_K,"300x300dpi_highmono",N_("300x300 DPI HIGH MONO"),INKSET(9_K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(9_K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(9_K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT"),INKSET(9_K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(9_C7M7Y7K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,4},/* untested */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM gloss"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO coated/glossFilm"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2h),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki --- different quality settings to plain */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2h),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i320,0);

/* has similarities to MP360 series */
static const canon_mode_t canon_BJC_i450_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color only */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes --- some modes use cmy, not supported */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT (plusGlossy)"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH Env/Hagaki MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i450,0);

/* similarities to MP360 series */
static const canon_mode_t canon_BJC_i455_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo3",N_("600x600 DPI PHOTO HIGH (gloss)"),INKSET(9_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYcmy */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* CMYcmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT (super)"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH Env/Hagaki MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i455,0);

static const canon_mode_t canon_BJC_i550_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(9_C7M7Y7K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM gloss"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/super/glossFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT coated"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki MONO"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i550,0);

static const canon_mode_t canon_BJC_i560_modes[] = {
  /* original modes (2) */
  /* {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,3.5,1.0,NULL,NULL,NULL,2}, */
  /* {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.8,1.0,NULL,NULL,NULL,2}, */
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},/*untested*/
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGloss"),INKSET(9_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT coated"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* inkjet Hagaki --- untested */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki bw=0 for some reason---just sending K data to printer */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i560,0);

static const canon_mode_t canon_BJC_i850_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},/*untested*/
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT coated"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT CD"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i850,0);

/* same modes as iP4000, iP4100 */
static const canon_mode_t canon_BJC_i860_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4k4p),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT super"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untest */
  /* inkjet Hagaki --- untested */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(9_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki --- no mono in Windows driver, so this is untested. Also Windows shows only quality 2,1 */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* untested */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* OHP CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt: CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i860,0);

/* i860 with CD capability */
static const canon_mode_t canon_BJC_i865_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4k4p),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT super"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untest */
  /* inkjet Hagaki --- untested */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(9_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki --- no mono in Windows driver, so this is untested. Also Windows shows only quality 2,1 */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* untested */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI CD HIGH"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 CD PHOTO DRAFT"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* OHP CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt: CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i865,0);

/* i900D */
static const canon_mode_t canon_BJC_i900_modes[] = {
  /* plain modes --- no mono modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y3K3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1bpp */
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST pro"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y3K3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i900,0);

static const canon_mode_t canon_BJC_i950_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI MEDIUM"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std",N_("600x600 DPI"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO LOW"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated/PhotoFilm"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO pro/coated"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO / DRAFT PHOTO coated"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki --- untested */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt --- same as plain std */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki --- same as plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high5",N_("600x600 DPI STD Env/Hagaki"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI LOW Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std5",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono4",N_("600x600 DPI MONO LOW Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono5",N_("600x600 DPI MONO DRAFT Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_BJC_i950,0);

static const canon_mode_t canon_BJC_i960_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH (Duplex"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI MEDIUM"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std",N_("600x600 DPI"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO LOW"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO pro/coated"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO / DRAFT PHOTO coated"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki --- untested */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt --- same as plain std */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki --- same as plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high5",N_("600x600 DPI MEDIUM Env/Hagaki"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI STD Env/Hagaki"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std5",N_("600x600 DPI LOW Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},

  {  600, 600,CANON_INK_K,"600x600dpi_draftmono4",N_("600x600 DPI MONO LOW Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono5",N_("600x600 DPI MONO DRAFT Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_BJC_i960,0);

static const canon_mode_t canon_BJC_i6100_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI MEDIUM"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST gloss"),INKSET(9_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/super"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/gloss/coated"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/super"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/gloss/coated"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT coated"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH Transparency"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i6100,0);

static const canon_mode_t canon_BJC_i9100_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C5M5Y5K9c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI LOW"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/plusGloss"),INKSET(9_C5M5Y5K5c9m9_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/gloss/coated"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/coated"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/gloss/coated"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C5M5Y5K9c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_BJC_i9100,0);

static const canon_mode_t canon_BJC_i9900_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std",N_("600x600 DPI MEDIUM"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO LOW"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes --- many use R, G inks which are not yet supported */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH coated"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM coated"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO coated/other"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT gloss/plusGloss"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt --- same as plain std */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki --- same as plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft5",N_("600x600 DPI LOW Env/Hagaki"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono4",N_("600x600 DPI MONO LOW Env/Hagaki"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono5",N_("600x600 DPI MONO DRAFT Env/Hagaki"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_BJC_i9900,0);

static const canon_mode_t canon_BJC_8200_modes[] = {
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  { 1200,1200,CANON_INK_CMYK,"1200x1200dpi",N_("1200x1200 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_8200,0);


static const canon_mode_t canon_BJC_8500_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_8500,0);


static const canon_mode_t canon_S200_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
     "360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,delay_S200,2.0,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
     "720x720dpi",N_("720x720 DPI"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_WEAVE,delay_S200,1.0,1.0,NULL,NULL,NULL,2},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
    "1440x720dpi",N_("1440x720 DPI"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_WEAVE,delay_S200,0.5,1.0,NULL,NULL,NULL,2},
  { 1440,1440,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
    "1440x1440dpi",N_("1440x1440 DPI"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_WEAVE,delay_S200,0.3,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_S200,0);

static const canon_mode_t canon_BJC_S300_modes[] = {
  /* original two modes */
  /*{  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},*/
  /*{  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},*/
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI MEDIUM"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST gloss/PhotoCard"),INKSET(9_C7M7Y7K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss/PhotoCards"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO coated/PhotoFilm/PhotoCard"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_S300,0);

static const canon_mode_t canon_BJC_S330_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI MEDIUM"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST gloss"),INKSET(9_C7M7Y7K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO coated/matte/PhotoFilm"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_S330,0);

static const canon_mode_t canon_BJC_S500_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST gloss/coated/PhotoCard"),INKSET(9_C7M7Y7K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss/coated/PhotoCard"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO gloss/coated/PhotoCard"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT pro"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_S500,0);

static const canon_mode_t canon_BJC_S520_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST gloss"),INKSET(9_C7M7Y7K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/plusGloss/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/gloss/coated"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/plusGloss/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/gloss/coated"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT coated"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_S520,0);

static const canon_mode_t canon_BJC_S600_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST gloss/coated/PhotoCard"),INKSET(9_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/PhotoFilm"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss/coated/PhotoCard"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/PhotoFilm"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO gloss/coated/PhotoCard"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT coated"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki --- untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh5",N_("600x600 DPI HIGHEST Transparency"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH Transparency"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_S600,0);

static const canon_mode_t canon_BJC_S750_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST gloss"),INKSET(9_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/gloss/coated/plusGloss"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/gloss/coated/plusGloss"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT coated"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_S750,0);

static const canon_mode_t canon_BJC_S800_modes[] = {
  /* plain modes --- also for Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(9_C9M9Y9K9c9m9),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO 2"),INKSET(9_K9),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(1_K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI PHOTO HIGHEST pro/other"),INKSET(9_C9M9Y9K9c9m9),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST matte/gloss/coated/PhotoCard/"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH plusGloss/PhotoFilm"),INKSET(9_C9M9Y9K9c9m9),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH pro/matte/gloss/coated/PhotoCard/other"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/gloss/coated/PhotoCard"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO other"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT other"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* mono photo modes for PPother only --- manual feed preferred in Windows driver */
  {  600, 600,CANON_INK_K,"600x600dpi_photomonohigh2",N_("600x600 DPI PHOTO HIGHEST MONO other"),INKSET(9_K9),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_K,"600x600dpi_photomonohigh",N_("600x600 DPI PHOTO HIGH MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_photomono",N_("600x600 DPI PHOTO MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_photomonodraft",N_("600x600 DPI PHOTO MONO DRAFT"),INKSET(1_K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_S800,0);

/* Windows driver shows fewer modes than S800, but it could be they exist */
/* modes present in S800 but not accessible in Windows driver for S820 are marked as untested */
static const canon_mode_t canon_BJC_S820_modes[] = {
  /* plain modes --- also for Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(9_C9M9Y9K9c9m9),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO 2"),INKSET(9_K9),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI PHOTO HIGHEST pro/other"),INKSET(9_C9M9Y9K9c9m9),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST matte/gloss/coated/PhotoCard/"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH plusGloss/PhotoFilm"),INKSET(9_C9M9Y9K9c9m9),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH pro/matte/gloss/coated/PhotoCard/other"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/gloss/coated/PhotoCard"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO other"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_S820,0);

static const canon_mode_t canon_BJC_S900_modes[] = {
  /* plain modes --- also for Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH 2"),INKSET(9_C5M5Y5K9c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO 2"),INKSET(9_K9),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh5",N_("600x600 DPI PHOTO HIGHEST pro"),INKSET(9_C5M5Y5K5c9m9_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI PHOTO HIGHEST coated"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH plusGloss"),INKSET(9_C5M5Y5K5c9m9_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/gloss/PhotoFilm"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH pro/coated"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/gloss/coated"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO DRAFT"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_S900,0);

/* no K used in any modes */
static const canon_mode_t canon_PIXMA_mini220_modes[] = {
  /*{  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
    {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},*/
  /* Hagaki */
  /*{  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH Hagaki"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI Hagaki"),INKSET(9_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_mini220,0);

static const canon_mode_t canon_PIXMA_mini320_modes[] = {
  /* most modes used unsupported inks */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(11_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_mini320,0);

static const canon_mode_t canon_SELPHY_DS700_modes[] = {
  /*  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH 2"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},*/
  /*  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},*/
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO 2"),INKSET(9_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo3",N_("600x600 DPI HAGAKI HIGH"),INKSET(9_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft2",N_("600x600 DPI HAGAKI"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_SELPHY_DS700,4);

static const canon_mode_t canon_SELPHY_DS810_modes[] = {
  /*  { 1200,1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGH 2"),INKSET(9_c3m3y3),32,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},*/
  /*  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO 2"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(9_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Hagaki */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo3",N_("600x600 DPI HAGAKI HIGH"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft2",N_("600x600 DPI HAGAKI"),INKSET(9_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_SELPHY_DS810,4);


/* most modes use ESC (S */
static const canon_mode_t canon_PIXMA_iP90_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C9M9Y9K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,4},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DRAFT DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1-bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1-bpp */
  /* photo modes */
  /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH 2"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},*/ /* no ESC (S, but uses cmy inks which are not yet supported */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C9M9Y9K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C9M9Y9K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGloss"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C9M9Y9K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* TST */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C9M9Y9K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* no ESC (S */
};
DECLARE_MODES(canon_PIXMA_iP90,0);

/* all modes use ESC (S */
static const canon_mode_t canon_PIXMA_iP100_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH MONO"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,4},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(11_C2M2Y2K2),8,MODE_FLAG_IP8500|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(11_C2M2Y2K2),8,MODE_FLAG_IP8500|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST pro"),INKSET(11_C9M9Y4k6),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(11_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(11_C5M5Y3k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(11_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(11_C6M6Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* TST */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(11_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP100,0);

/* testing */
static const canon_mode_t canon_PIXMA_iP1000_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(9_C4M4Y4K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2h),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DRAFT DPI"),INKSET(9_C3M3Y2K2h),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(9_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(9_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH 2"),INKSET(9_C5M5Y5),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C5M5Y5),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(9_C5M5Y5),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP1000,4);

/* testing */
static const canon_mode_t canon_PIXMA_iP1200_modes[] = {
  /* both cartridges */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(13_C4M4Y4K2c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2b),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DRAFT DPI"),INKSET(13_C3M3Y2K2b),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color only modes*/
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi",N_("1200x1200 DPI PHOTO"),INKSET(13_c3m3y3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT 2"),INKSET(13_C5M5Y5),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C5M5Y5),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* removed black */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH 4"),INKSET(13_C4M4Y4c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH 3"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI 3"),INKSET(13_C3M3Y2b),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI 2"),INKSET(13_C5M5Y5),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT 2"),INKSET(13_C3M3Y2b),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_PIXMA_iP1200,4);

static const canon_mode_t canon_PIXMA_iP1500_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C9M9Y9K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2h),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(9_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(9_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO 2"),INKSET(9_C4M4Y4K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_C4M4Y4K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* photo */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH 2"),INKSET(9_C9M9Y9K2c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C9M9Y9K2c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO 2"),INKSET(9_C9M9Y9K2c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP1500,3);

/* testing */
static const canon_mode_t canon_PIXMA_iP1900_modes[] = {
  /* plain, Hagaki, Env */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2b),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo high --- unlikely to work */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* photo std --- unlikely to work */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* photo high 2 --- unlikely to work */
  {  1200, 1200,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH2"),INKSET(13_c3m3y3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  /* photo draft */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C5M5Y5),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color only plain, hagaki, Env*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(13_C4M4Y4K2c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI 2"),INKSET(13_C3M3Y2b),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_2",N_("300x300 DPI 2"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT 2"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},

};
DECLARE_MODES(canon_PIXMA_iP1900,2);

static const canon_mode_t canon_PIXMA_iP2000_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1-bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1-bpp */
  /* photo modes */
  /*{  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_c9m9y9),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},*/ /* need to add cmy support */
  /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_c9m9y9),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(9_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* OHP */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /*  T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP2000,0);

static const canon_mode_t canon_PIXMA_iP3000_modes[] = {
  /* plain modes --- same for duplex */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* with K */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* no K */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* no K */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* no K */
  /* inkjet Hagaki --- untested */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* no K */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* no K */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* no K */
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI CD HIGH"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* no K */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI CD"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* no K */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI CD DRAFT"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* no K */
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* with K */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP3000,0);

/* as iP3000 but with Hagaki/inkjet Hagaki */
static const canon_mode_t canon_PIXMA_iP3100_modes[] = {
  /* plain modes --- same for duplex */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* with K */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* no K */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* no K */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* no K */
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* no K */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* no K */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* no K */
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI CD HIGH"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* no K */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI CD"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* no K */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI CD DRAFT"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* no K */
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* with K */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP3100,0);

static const canon_mode_t canon_PIXMA_iP4000_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI 2"),INKSET(9_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /*Gernot adding the normal hi-quality mode for iP4000 here*/
  {  600, 600,CANON_INK_CcMmYK | CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH 2"),INKSET(9_C4M4Y4K2c4m4k4p),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
/*  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL},*/ /* this mode is used for CD printing, K is ignored by the printer then, the separation between the small and large dot inks needs more work */
/*  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_superphoto",N_("600x600 DPI Superphoto"),INKSET(9_C8M8Y8c16m16k8),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,4}, */
  /* T-Shirt: CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* OHP: K & k --- experimental */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Envelope --- K & k experimental */
  /*{  600, 600,CANON_INK_CcMmYKk | CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH ENV"),INKSET(9_C4M4Y4K2c4m4k4p),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},*/
  {  600, 600,CANON_INK_CMYKk | CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI ENV"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP4000,3);

/* iP4100, iP4100R */
static const canon_mode_t canon_PIXMA_iP4100_modes[] = {
  /* plain modes --- same for duplex */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4k4p),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT super"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH CD"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI CD"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT CD"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(9_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* OHP CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt: CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP4100,0);

static const canon_mode_t canon_PIXMA_iP4200_modes[] = {
  /* plain modes */
  /* highest plain mode --- temporarily also for CDs : CcMmYKk */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Q2 - standard mode for this driver (in windows driver it's Q3) */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Q1 - normal 300x300 mode (in windows driver it's Q4 - normal darkness of printout ) */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,iP4200_300dpi_lum_adjustment,NULL,NULL,1},
  /* Q0 - fastest mode (in windows driver it's Q5, printer uses 50% of ink ( I think )) */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,iP4200_300dpi_draft_lum_adjustment,NULL,NULL,0},
  /* photo modes mostly use unsupported ink positions */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet hagaki */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_photo2",N_("600x600 DPI PHOTO HAGAKI"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO HAGAKI DRAFT"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Hagaki, Env: K & k */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH (Env/Hagaki)"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI (Env/Hagaki)"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-Shirt"),INKSET(22_C4M4Y4K2k4one),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP4200,0);

static const canon_mode_t canon_PIXMA_iP4300_modes[] = {
  /* plain modes */
  /* highest plain mode --- temporarily also for CDs : CcMmYKk */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,iP4200_300dpi_lum_adjustment,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,iP4200_300dpi_draft_lum_adjustment,NULL,NULL,0},
  /* photo modes mostly use unsupported ink positions */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet hagaki */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_photo2",N_("600x600 DPI PHOTO HAGAKI"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO HAGAKI DRAFT"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Hagaki, Env: K & k */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_high2",N_("600x600 DPI HIGH (Env/Hagaki)"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI (Env/Hagaki)"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-Shirt"),INKSET(22_C4M4Y4K2k4one),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP4300,0);

/* plain high mode has more output inks than declared in inkset, and unknown to boot */
static const canon_mode_t canon_PIXMA_iP5000_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},/* 1-bpp, untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1-bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1-bpp */
  /* photo --- most use plain high mode which is currently unsupported */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT PlusGlossy"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* CMYcmk */
  /* TST CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* OHP CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photohigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photo",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP5000,0);

static const canon_mode_t canon_PIXMA_iP6000_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (Duplex)"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y3K3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST / CD"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH / CD"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO / CD"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* OHP */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y3K3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP6000,0);

/* iP6210D, iP6220D, iP6310D */
static const canon_mode_t canon_PIXMA_iP6210_modes[] = {
  /* both cartridges */
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4k4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y3k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes -- most use non-identified inks */
  /* standard mode is same as high for plain media */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4k4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Hagaki/Envelope */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y4k4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C4M4Y4k4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y3k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color only modes---not yet accessible since ESC (T not programmed fully */
  /* plain modes --- those that use supported inks */
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI (Color-only)"),INKSET(13_C3M3Y2b),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI (Color-only)"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT (Color-only)"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},*/
  /* Hagaki/Envelope */
  /*  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki (Color-only)"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
      {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki (Color-only)"),INKSET(13_C3M3Y2b),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},*/
  /* T-shirt */
  /*  {  600, 600,CANON_INK_CMY,"600x600dpi_std5",N_("600x600 DPI T-SHIRT (Color-only)"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},  */
};
DECLARE_MODES(canon_PIXMA_iP6210,0);

static const canon_mode_t canon_PIXMA_iP6700_modes[] = {
  /* plain modes --- duplex same CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(19_C3M3Y3k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYkcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(19_C7M7Y4c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(19_C6M6Y4c6m6k4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYkcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(19_C7M7Y4c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(19_C6M6Y4c6m6k4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Fine Art */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI FineArt"),INKSET(19_C7M7Y4c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* CD media --- subchannels reordered */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh5",N_("600x600 DPI HIGH CD"),INKSET(19_C4M4Y4c4m4k4CD),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo5",N_("600x600 DPI CD"),INKSET(19_C4M4Y4c4m4k4CD),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft5",N_("600x600 DPI DRAFT CD"),INKSET(19_C4M4Y4c4m4k4CD),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Hagaki, Envelope CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(19_C6M6Y4c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(19_C6M6Y4c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(19_C3M3Y3k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(19_C4M4Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP6700,0);

static const canon_mode_t canon_MULTIPASS_MP150_modes[] = {
  /* plain modes */
  /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_glossphotohigh",N_("600x600 DPI GLOSSPHOTO HIGH"),INKSET(13_C4M4Y4c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, */ /* need to check cmy */
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, */ /* need to check cmy */
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, */ /* need to check cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGloss"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI TSHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP150,0);

static const canon_mode_t canon_MULTIPASS_MP170_modes[] = {
  /* plain modes */
  /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},*/ /* need to check cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_glossphotohigh",N_("600x600 DPI GLOSSPHOTO HIGH"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, */ /* need to check cmy */
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, */ /* need to check cmy */
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, */ /* need to check cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGloss"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI TSHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP170,0);

static const canon_mode_t canon_MULTIPASS_MP190_modes[] = {
  /* plain modes */
  /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},*/ /* need to check cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes --- high uses unsupported inks */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI TSHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
};
DECLARE_MODES(canon_MULTIPASS_MP190,0);

static const canon_mode_t canon_MULTIPASS_MP210_modes[] = {
  /* both cartridges */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color only modes: photo */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO pro/gloss/plusGloss/plusGlossDS"),INKSET(13_C4M4Y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGloss"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* removed black */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI (COLOR-ONLY)"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI DRAFT (COLOR-ONLY)"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI (COLOR-ONLY)"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft4",N_("300x300 DPI DRAFT (COLOR-ONLY)"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_MULTIPASS_MP210,0);

static const canon_mode_t canon_MULTIPASS_MP220_modes[] = {
  /* plain modes */
  /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  /* {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_glossphotohigh",N_("600x600 DPI PHOTO HIGHEST pro"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, */ /* need to check cmy */
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, */ /* need to check cmy */
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/coated/inkjetHagaki"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, */ /* need to check cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGloss"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI TSHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP220,0);

/* MP360, MP370, MP375R, MP390 */
static const canon_mode_t canon_MULTIPASS_MP360_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  /* inkjet Hagaki use same modes as other photo media */
  /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST (Pro)"),INKSET(9_C4M4Y4K2c4m4y4minor),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, */ /* cmy */
  /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4y4minor),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},*/ /* cmy */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo3",N_("600x600 DPI PHOTO HIGH (gloss)"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYcmy */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* CMYcmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT (super)"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* OHP */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH Env/Hagaki MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP360,0);

static const canon_mode_t canon_MULTIPASS_MX300_modes[] = {
  /* plain modes */
  /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},*/ /* need to check cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  /* {  1200, 1200,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},*/ /* need to check cmy */
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH 2"),INKSET(13_c9m9y9),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},*/ /* need to check cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO 2"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},*/ /* need to check cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI TSHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono3",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color-only */
  /* {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},*/ /* need to check cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MX300,0);

static const canon_mode_t canon_MULTIPASS_MX330_modes[] = {
  /* plain media */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color-only*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX330,0);

static const canon_mode_t canon_MULTIPASS_MX340_modes[] = {
  /* plain media */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* color-only*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX340,0);

static const canon_mode_t canon_MULTIPASS_MX360_modes[] = {
  /* plain media */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
/* color only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH (COLOR-ONLY)"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI (COLOR-ONLY)"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT (COLOR-ONLY)"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI (COLOR-ONLY)"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT (COLOR-ONLY)"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* photo modes */
  /*{  1200, 600,CANON_INK_cmy,"1200x1200dpi_photohigh",N_("1200x1200 DPI PHOTO HIGH"),INKSET(13_c3m3y3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* TST */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO DPI Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},  
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO DPI Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},  
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki (COLOR-ONLY)"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki (COLOR-ONLY)"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX360,0);

static const canon_mode_t canon_MULTIPASS_MP240_modes[] = {
  /* plain color modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* plain mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* plain color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Env/Hagaki color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high5",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP240,0);

static const canon_mode_t canon_MULTIPASS_MP250_modes[] = {
  /* plain color modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (Duplex)"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* plain mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO (Duplex)"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* plain color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high5",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP250,0);

static const canon_mode_t canon_MULTIPASS_MP280_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* plain mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* TST */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* color-only cartridge modes for plain, allHagaki, Hagaki, Env */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* Env/Hagaki color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP280,0);

/* still need to add 1200dpi mode user-defined */
/* some modes require subtractions in the ink names, not yet supported */
static const canon_mode_t canon_MULTIPASS_MP470_modes[] = {
  /* plain std */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* plain fast */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* add some B/W modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* add photo paper modes --- Std does not work for Matte Photo paper, HiRes, inkjet Hagaki */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* end in color mode */
  /* color-only cartridge modes for plain, Hagaki, Env */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_MULTIPASS_MP470,0);

/* still need to add 1200dpi mode user-defined */
static const canon_mode_t canon_MULTIPASS_MP480_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP480,0);

/* still need to add 1200dpi mode user-defined */
static const canon_mode_t canon_MULTIPASS_MP490_modes[] = {
  /* plain hi */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* plain std */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* plain fast */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* non-photo Hagaki & Env */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /*add some B/W modes*/
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /*add photo paper modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /*end in color mode */
  /* color-only cartridge modes for plain, Hagaki, Env */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_MULTIPASS_MP490,0);

/* same family: iP3300, iP3500, MX700, MP510, MP520 */
static const canon_mode_t canon_MULTIPASS_MP520_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO super/glossGold/gloss"),INKSET(22_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* CMYcm */
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* CMY */
  /* TST mode CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki --- same as plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP520,0);

/* photo modes mostly use unsopprted inks */
static const canon_mode_t canon_MULTIPASS_MP530_modes[] = {
  /* plain modes --- duplex same */
  {  600, 600,CANON_INK_K|CANON_INK_CcMmYKk,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYKcmk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO DRAFT super"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2k4one),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP530,0);

static const canon_mode_t canon_MULTIPASS_MP540_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* 16 */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO inkjet Hagaki"),INKSET(13_C5M5Y4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* 8 not 16 */
  /* T-Shirt --- same as photo high */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP540,0);

/* similar to MP540 but fewer modes */
static const canon_mode_t canon_MULTIPASS_MP550_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT 2"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH HAGAKI"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO HAGAKI"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt --- same as photo high */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP550,0);

/* similar to MP540 but fewer modes */
static const canon_mode_t canon_MULTIPASS_MP560_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH (Duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT 2"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH HAGAKI"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO HAGAKI"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt --- same as photo high */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP560,0);

/* similar to MP830 */
/* most photo modes use unsupported inks */
static const canon_mode_t canon_MULTIPASS_MP600_modes[] = {
  /* plain modes --- duplex same */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* k off */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes --- most unsupported */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* K off */
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* K off */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* K off */
  /* TST */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2k4one),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* K off */
  /* Env/Hagaki --- same as plain modes */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP600,0);

static const canon_mode_t canon_MULTIPASS_MP640_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH (Duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT 2"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki / CD */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH HAGAKI / CD"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO HAGAKI / CD"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt --- same as photo high */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP640,0);

static const canon_mode_t canon_MULTIPASS_MP900_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO LOW"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO pro/coated"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO / DRAFT PHOTO coated"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT CD"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* OHP */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt --- same as plain std */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki --- same as plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono4",N_("600x600 DPI MONO Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP900,0);

static const canon_mode_t canon_PIXMA_iP2700_modes[] = {
  /* user-defined mode: highest resolution, no mono */
  /*{  1200, 1200,CANON_INK_CMYK,"1200x1200dpi_high",N_("1200x1200 DPI HIGH"),INKSET(13_c3m3y3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},*/
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* B/W modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI MONO HIGH Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Std mode using only color cartridge */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI STANDARD COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP2700,0);

/* MX860 --- similar to iP4500 */
static const canon_mode_t canon_MULTIPASS_MX860_modes[] = {
  /* plain modes --- duplex same CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX860,0);

/* MX870 --- similar to iP4500 */
static const canon_mode_t canon_MULTIPASS_MX870_modes[] = {
  /* high mode -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* standard mode -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},*/
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* draft modes -- B/W also */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* PhotoHi mode for high quality photo papers */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO 2"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* PhotoHi3 for inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH 3"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO 3"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX870,3);

static const canon_mode_t canon_MULTIPASS_MX7600_modes[] = {
  /* high mode -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K3k4_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* standard mode -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* draft modes -- B/W also */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_MULTIPASS_MX7600,3);

static const canon_mode_t canon_PIXMA_iP4500_modes[] = {
  /* plain modes -- duplex same (except no mono) */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT Plus/PlusGlossyII"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(13_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI HIGH CD"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI CD"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI DRAFT CD"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C6M6Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt CMYk --- note this mode uses 2 bits not 4 */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP4500,0);

static const canon_mode_t canon_PIXMA_iP4600_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /*untested*/
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI HIGH PHOTO/CD"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH 2"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/*untested*/
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO 2 / CD"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT 2"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Hagaki and Env modes used K and k both */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI HIGH T-SHIRT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_iP4600,0);

static const canon_mode_t canon_PIXMA_iP4700_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (Duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /*T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_iP4700,0);

static const canon_mode_t canon_PIXMA_iP4900_modes[] = {
  /* plain modes -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* duplex, Env, Hagaki  modes use K and k both */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Duplex/Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Duplex/Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_iP4900,0);

static const canon_mode_t canon_PIXMA_iP5300_modes[] = {
  /* plain modes --- duplex same */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(13_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT CD"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C6M6Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP5300,0);

static const canon_mode_t canon_MULTIPASS_MP800_modes[] = {
  /* most photo modes use some unknown inks */
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Fast Photo paper mode CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki: PhotoFastCMYcmk, Std CMYk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP800,0);

static const canon_mode_t canon_MULTIPASS_MP810_modes[] = {
  /* plain paper modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* Photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH 2 / CD"),INKSET(13_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO 2 / CD"),INKSET(13_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT 2 / CD"),INKSET(13_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(13_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C6M6Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},

};
DECLARE_MODES(canon_MULTIPASS_MP810,0);

/* similar to MP600 */
/* most photo modes use unsupported inks */
static const canon_mode_t canon_MULTIPASS_MP830_modes[] = {
  /* plain modes --- duplex same */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT super"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*CMYk*/
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2k4one),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP830,0);

static const canon_mode_t canon_MULTIPASS_MP950_modes[] = {
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_MULTIPASS_MP950,3);

/* high resolution mono modes use unknown inks */
static const canon_mode_t canon_MULTIPASS_MP960_modes[] = {
  /* plain modes CMYK */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(19_C6M6Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI STD"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* inkjet Hagaki --- same as photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI inkjet Hagaki"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Fine Art CMYcmk */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI FineArt"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(19_C6M6Y4K2c6m6k4hagaki),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt transfers CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI T-SHIRT TRANSFERS"),INKSET(19_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* photo modes high & std (600dpi) CMYcmk */
  /* photo mode 2 (600dpi) CMYcmk --- works, but c,m may need a bit of tone/density adjusting */
  /* photo mode 3 CD (600dpi) CMYcmk --- c,m too light, may need adjusting tone/density */
  /* inkjet hagaki outputs in CMYcmk std, hi --- c,m too light, may need adjusting tone/density */
};
DECLARE_MODES(canon_MULTIPASS_MP960,0);

static const canon_mode_t canon_MULTIPASS_MP970_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI HIGH (Duplex)"),INKSET(19_C6M6Y4K2c6m6k4off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},  
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt transfers CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(19_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},  
  /* Env/Hagaki CMYKk --- same for duplex */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(19_C6M6Y4K2c6m6k4hagaki),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(19_C6M6Y4K2c6m6k4hagaki),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_MULTIPASS_MP970,0);

/* most photo modes use H (grey) so unsupported presently */
static const canon_mode_t canon_MULTIPASS_MP980_modes[] = {
  /* normal modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(16_C6M6Y4K2k4off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH (Duplex)"),INKSET(16_C3M3Y2K2k3off_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(16_C3M3Y2K2k3off_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(16_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(16_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Photo modes: matte, high resolution */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(16_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO matte/coated"),INKSET(16_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD -- same as above */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH CD"),INKSET(16_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI CD"),INKSET(16_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Photo modes: inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(16_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI inkjet Hagaki"),INKSET(16_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt --- same as Photo High */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(16_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Photo modes: other */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(16_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(16_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(16_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP980,0);

static const canon_mode_t canon_MULTIPASS_MP990_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K2C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk --- most use grey (H) or unsupported inks */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/other"),INKSET(30_M6K6m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP990,0);

static const canon_mode_t canon_PIXMA_iX4000_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Photo: CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT super"),INKSET(22_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki --- same as plain */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* TST mode */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(22_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(22_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iX4000,0);

static const canon_mode_t canon_PIXMA_iX5000_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Photo: CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT super"),INKSET(22_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki --- same as plain */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* TST mode */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iX5000,0);

static const canon_mode_t canon_PIXMA_iX7000_modes[] = {
 /* plain */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K3k4_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0}, /* untested */
  /* photo CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C6M6Y2K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y2K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C6M6Y2K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iX7000,0);

/* mono uses bw=2 */
static const canon_mode_t canon_PIXMA_Pro9000_modes[] = {
/* Plain Modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI LOW"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT 2"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* plain mono */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono",N_("600x600 DPI MONO HIGH"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT 2"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
/* Photo Modes */
/* highest uses R,G so cannot support */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH GlossPro/proPlat/pro"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT gloss"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT super"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomed2",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomed",N_("600x600 DPI inkjet Hagaki"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI CD"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* photo mono */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomonohigh",N_("600x600 DPI PHOTO MONO HIGH / CD"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomonomed",N_("600x600 DPI PHOTO MONO MEDIUM"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomono",N_("600x600 DPI PHOTO MONO / CD"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomonodraft",N_("600x600 DPI PHOTO MONO DRAFT"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki 2"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* board, canvas, FineArt, Museum Etching */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh5",N_("600x600 DPI Board/Canvas/FineArt/Museum"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
};
DECLARE_MODES(canon_PIXMA_Pro9000,0);

/* all use normal BJ format and ink codes */
/* mono uses bw=2 */
static const canon_mode_t canon_PIXMA_Pro9000mk2_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT 2"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* plain mono modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono",N_("600x600 DPI MONO HIGH"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT 2"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes --- highest use RGH so cannot support yet */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(11_C6M6Y6K6c16m16_c),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(11_C6M6Y6K6c6m6_c),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM pro/proPlat/"),INKSET(11_C6M6Y6K6c16m16_c),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(11_C6M6Y6K6c6m6_c),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO gloss"),INKSET(11_C6M6Y6K6c6m6_c),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(11_C6M6Y6K6c16m16_c),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(11_C6M6Y6K6c6m6_c),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(11_C6M6Y6K6c6m6_c),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(11_C6M6Y6K6c6m6_c),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* photo mono */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomonohigh",N_("600x600 DPI PHOTO MONO HIGH / CD"),INKSET(11_C16M16Y16K16c16m16),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomonomed",N_("600x600 DPI PHOTO MONO MEDIUM"),INKSET(11_C16M16Y16K16c16m16),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomono",N_("600x600 DPI PHOTO MONO / CD"),INKSET(11_C16M16Y16K16c16m16),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomonodraft",N_("600x600 DPI PHOTO MONO DRAFT"),INKSET(11_C16M16Y16K16c16m16),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* board, canvas, FineArt, Museum Etching */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh5",N_("600x600 DPI Board/Canvas/FineArt/Museum"),INKSET(11_C6M6Y6K6c6m6_c),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
};
DECLARE_MODES(canon_PIXMA_Pro9000mk2,0);

/* all modes use normal BJ ink and format codes */
/* However, most modes use RGH inks as well as CMYKcmyk, and so cannot be currently supported */
/* the Pro9500 has fewer quality settings in the driver than the Pro9500 Mk.II but I put the "missing" ones in anyway */
static const canon_mode_t canon_PIXMA_Pro9500_modes[] = {
  /* plain mode: fast, 1 only but maybe accept 2 and 0 quality settings too */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
/* plain mono: these modes use CMYK inks also */
/* used for plain, hagaki */
/*             bw=2                           */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono2",N_("600x600 DPI MONO PLAIN/HAGAKI HIGH"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono",N_("600x600 DPI MONO HAGAKI STD"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_mono",N_("600x600 DPI MONO PLAIN STD"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT2"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
/* Photo modes: only mono using CMYK-only can be supported currently */
/* this has k instead of K */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_monophoto",N_("600x600 DPI MONO PHOTO"),INKSET(11_C16M16Y16k16),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
};
DECLARE_MODES(canon_PIXMA_Pro9500,0);

/* all modes use normal BJ ink and format codes */
/* However, most modes use RGH inks as well as CMYKcmyk, and so cannot be currently supported */
static const canon_mode_t canon_PIXMA_Pro9500mk2_modes[] = {
/* plain: 3 variations on the Fast mode: 600/1 and 600/0 are supported, but perhaps 600/2 is also possible */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
/* plain mono: these modes use CMYK inks also */
/* used for plain, hagaki */
/*             bw=2                           */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono2",N_("600x600 DPI MONO PLAIN/HAGAKI HIGH"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono",N_("600x600 DPI MONO HAGAKI STD"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_mono",N_("600x600 DPI MONO PLAIN STD"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT2"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
/* Photo modes: only mono using CMYK-only can be supported currently */
/* this has k instead of K */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_monophoto",N_("600x600 DPI MONO PHOTO"),INKSET(11_C16M16Y16k16),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
};
DECLARE_MODES(canon_PIXMA_Pro9500mk2,0);

/* iP7100 */
/* ESC (r command is 0x64 but another one befor data is sent: ESC (r 0x62 0x0 */
static const canon_mode_t canon_PIXMA_iP7100_modes[] = {
  /* plain modes: color */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Duplex"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI plain/T-Shirt"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Hagaki/Env */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* plain modes: mono */
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO / CD High"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO superDS/gloss"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI CD Draft"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Fine Art*/
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO FineArt"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP7100,0);

static const canon_mode_t canon_PIXMA_iP7500_modes[] = {
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4K2c6m6k4off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Fine Art */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI FineArt"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Hagaki, Envelope */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Hagaki/Env"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Hagaki/Env"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Hagaki/Env"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD media */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(19_C4M4Y4c4m4k4CD),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(19_C4M4Y4c4m4k4CD),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPIDRAFT CD"),INKSET(19_C4M4Y4c4m4k4CD),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(19_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP7500,0);

/* iP8100 */
/* ESC (r command is 0x64 but another one befor data is sent: ESC (r 0x62 0x0 */
static const canon_mode_t canon_PIXMA_iP8100_modes[] = {
  /* plain modes: color */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Duplex"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI plain/T-Shirt"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Hagaki/Env */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* plain modes: mono */
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  /* one mode for PPpro ud1, hi cannot be supported yet as it uses Red ink (R) in addtion to CMYKcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO / CD High"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO superDS/gloss"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT super"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI CD Draft"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Fine Art*/
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO FineArt"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP8100,0);

/* iP8500 */
/* ESC (r command is 0x64 but another one befor data is sent: ESC (r 0x62 0x0 */
static const canon_mode_t canon_PIXMA_iP8500_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Duplex"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* plain modes: mono */
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  /* some modes cannot be supported yet as they use Red (R) and Green (G) ink in addition to CMYKcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH coated"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO coated/other"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plus"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT plusDS/glossy"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD CMYKcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT CD"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency CMYK */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt CMYK */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP8500,0);

/* iP8600 */
/* ESC (r command is 0x64 but another one befor data is sent: ESC (r 0x62 0x0 */
static const canon_mode_t canon_PIXMA_iP8600_modes[] = {
  /* plain modes: color */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Duplex"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI plain/T-Shirt"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* plain modes: mono */
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  /* one mode for PPpro ud1, hi, and PPsuper hi cannot be supported yet as it uses Red (R) and Green (G) ink in addition to CMYKcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH superDS/gloss"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO pro/super/matte/coated CD High"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO superDS/gloss"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT super"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI CD Draft"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki separate out */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjetHagaki"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjetHagaki"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Fine Art */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI PHOTO FineArt"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP8600,0);

/* iP9910 */
/* ESC (r command before data is sent: ESC (r 0x62 0x0 */
static const canon_mode_t canon_PIXMA_iP9910_modes[] = {
  /* plain modes: color */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Duplex"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI plain/T-Shirt"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Hagaki/Env */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* plain modes: mono */
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  /* one mode for PPpro hi, and PPsuper hi cannot be supported yet as it uses Red (R) and Green (G) ink in addtion to CMYKcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH superDS/gloss"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO pro"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/super/matte/coated/other CD High"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO gloss CD Draft"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT super"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki separate out */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjetHagaki"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjetHagaki"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Fine Art */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH FineArt"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP9910,0);

static const canon_mode_t canon_PIXMA_MG2100_modes[] = {
  /* high mode -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env mode */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (Env/Hagaki)"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* standard mode -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft mode -- B/W also */
  {  300, 300,CANON_INK_CMY,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Std mode using only color cartridge */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH (Color-only)"),INKSET(13_C3M3Y2K3off),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI STANDARD (Color-only)"),INKSET(13_C3M3Y2K3off),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI (Color-only)"),INKSET(13_C2M2Y2K2off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* B/W modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH (Black-only)"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI HIGH Env/Hagaki (Black-only)"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI (Black-only)"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI (Black-only)"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_MG2100,0);

static const canon_mode_t canon_PIXMA_MG5100_modes[] = {
  /* plain modes --- duplex no mono */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (Duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C8M8Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Hagaki/Env: use K and k both CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG5100,0);

static const canon_mode_t canon_PIXMA_MG5200_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (Duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG5200,0);

static const canon_mode_t canon_PIXMA_MG5300_modes[] = {
  /* plain modes -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex/Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Duplex/Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Duplex/Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/coated"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG5300,0);

/* for this printer most photo modes have grey (H) ink. */
static const canon_mode_t canon_PIXMA_MG6100_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K2C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (Duplex)"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/coated"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO other"),INKSET(30_M6K6m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG6100,0);

/* for this printer most photo modes have grey (H) ink. */
static const canon_mode_t canon_PIXMA_MG6200_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K2C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex/Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/coated"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO other"),INKSET(30_M6K6m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG6200,0);

/* for this printer most photo modes have grey (H) ink. */
static const canon_mode_t canon_PIXMA_MG8100_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K2C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex/Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/coated"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO other"),INKSET(30_M6K6m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG8100,0);

/* for this printer most photo modes have grey (H) ink. */
static const canon_mode_t canon_PIXMA_MG8200_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K2C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* duplex, Hagaki, Env modes use K and k */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/coated"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/coated"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO other"),INKSET(30_M6K6m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG8200,0);


#endif

