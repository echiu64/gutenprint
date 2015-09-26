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
  /* cartridge/InkSet-specific modes for replacement */
#define MODE_FLAG_BLACK 0x100          /* selection of black cartridge: replacement modes */
#define MODE_FLAG_COLOR 0x200          /* selection of color cartridge: replacement modes */
#define MODE_FLAG_PHOTO 0x400          /* selection of photo color cartridge in BJC series printers (in lieu of color and black, or combined color/black) */
#define MODE_FLAG_NODUPLEX 0x800       /* selection of mode for duplex: list modes that do not work for duplex---skip these in mode searches  */
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
  {  720, 360,CANON_INK_K,"720x360dpi",N_("720x360 DPI"),INKSET(1_K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K,"360x360dpi",N_("360x360 DPI"),INKSET(1_K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  180, 180,CANON_INK_K,"180x180dpi",N_("180x180 DPI"),INKSET(1_K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_30,0);


static const canon_mode_t canon_BJC_85_modes[] = {
  {  720, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
     "720x360dpi",N_("720x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
     "360x360dmt",N_("360x360 DPI DMT"),INKSET(6_C4M4Y4K4c4m4),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
     "360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_85,0);


/* we treat the printers that can either print in K or CMY as CMYK printers here by assigning a CMYK inkset */
static const canon_mode_t canon_BJC_210_modes[] = {
  {  720, 360,CANON_INK_K | CANON_INK_CMY,"720x360dpi",N_("720x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  180, 180,CANON_INK_K | CANON_INK_CMY,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {   90,  90,CANON_INK_K | CANON_INK_CMY,"90x90dpi",N_("90x90 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_210,0);


static const canon_mode_t canon_BJC_240_modes[] = {
  {  720, 360,CANON_INK_K | CANON_INK_CMY,"720x360dpi",N_("720x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dmt",N_("360x360 DMT"),INKSET(4_C4M4Y4K4),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  180, 180,CANON_INK_K | CANON_INK_CMY,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {   90,  90,CANON_INK_K | CANON_INK_CMY,"90x90dpi",N_("90x90 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_240,0);


static const canon_mode_t canon_BJC_2000_modes[] = {
  {  360, 360,CANON_INK_CMYK,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  180, 180,CANON_INK_CMYK,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_2000,0);

/* these printers only have CMYK, so CcMmYK is not required I think */
/* using color (CMYK) cartridge, some media use CMYK, others only use CMY --- need to check this as my test image lacked Y apparently */
/* some modes use ESC (t 0x2 0x80 0x9 which implies bits per ink > 1, but how to know how many? */
/* added MODE_FLAG_PHOTO as a means to set ESC (t */
static const canon_mode_t canon_BJC_3000_modes[] = {
  { 1440, 720,CANON_INK_K | CANON_INK_CMY | CANON_INK_CMYK | CANON_INK_CcMmYK,"1440x720dpi",N_("1440x720 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,delay_1440,1.0,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_K | CANON_INK_CMY | CANON_INK_CMYK | CANON_INK_CcMmYK,"720x720dpi",N_("720x720 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY | CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMY | CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dmt",N_("360x360 DMT"),INKSET(6_C4M4Y4K4c4m4),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi_draft",N_("360x360 DPI DRAFT"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  180, 180,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,"180x180dpi",N_("180x180 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  360, 360,CANON_INK_CMY | CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi_photo",N_("360x360 DPI PHOTO CARTRIDGE"),INKSET(6_C4M4Y4K4c4m4),8,MODE_FLAG_PHOTO,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_3000,1);

/* these printers only have K and CMYK, so CcMmYK is not required */
/* using color (CMYK) cartridge, all media use CMYK */
/* some modes use ESC (t 0x2 0x80 0x9 which implies bits per ink > 1, but how to know how many? */
/* added MODE_FLAG_PHOTO as a means to set ESC (t */
static const canon_mode_t canon_BJC_4300_modes[] = {
  {  760, 360,CANON_INK_K | CANON_INK_CMYK,"720x360dpi",N_("720x360 DPI"),INKSET(4_C4M4Y4K4),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  360, 360,CANON_INK_K | CANON_INK_CMYK,"360x360dpi_high",N_("360x360 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* different code */
  {  360, 360,CANON_INK_K | CANON_INK_CMYK,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  360, 360,CANON_INK_K | CANON_INK_CMYK,"360x360dmt",N_("360x360 DMT"),INKSET(4_C4M4Y4K4),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  360, 360,CANON_INK_K | CANON_INK_CMYK,"360x360dpi_draft",N_("360x360 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  180, 180,CANON_INK_K | CANON_INK_CMYK,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK,"360x360dpi_photo",N_("360x360 DPI PHOTO CARTRIDGE"),INKSET(4_C4M4Y4K4),8,MODE_FLAG_PHOTO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* there is another 720x360 mono mode */
};
DECLARE_MODES(canon_BJC_4300,1);

static const canon_mode_t canon_BJC_4400_modes[] = {
  {  720, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
     "720x360dpi",N_("720x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
     "360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_4400,0);

/* this seems to use different ESC (c bit for quality than pixma devices. */
/* these printers only have K, or (subset of) CMYK, so CcMmYK is not required. */
/* using color (CMYK) cartridge, get CMYK */
/* added MODE_FLAG_PHOTO as a means to set ESC (t */
static const canon_mode_t canon_BJC_4550_modes[] = {
  {  720, 360,CANON_INK_K|CANON_INK_CMYK,"720x360dpi_high",N_("720x360 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  720, 360,CANON_INK_K|CANON_INK_CMYK,"720x360dpi",N_("720x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  360, 360,CANON_INK_K|CANON_INK_CMYK,"360x360dpi_high",N_("360x360 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  360, 360,CANON_INK_K|CANON_INK_CMYK,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  180, 180,CANON_INK_K|CANON_INK_CMYK,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK,"360x360dpi_photo",N_("360x360 DPI PHOTO CARTRIDGE"),INKSET(4_C4M4Y4K4),8,MODE_FLAG_PHOTO,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_4550,1);

static const canon_mode_t canon_BJC_5500_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  180, 180,CANON_INK_CMYK | CANON_INK_CcMmYK,"180x180dpi",N_("180x180 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_5500,0);

/* HS mode:              bi-directional
   HQ mode (360x360):    bi-directional
   Fine mode (720x720):  uni-directional
   Fine mode (1440x720): uni-directional
*/
/* added MODE_FLAG_PHOTO as a means to set ESC (t 
   requires setting for those modes that are not 1bpp
   probably 4-bpp for photo mode, 2-bpp non-photo mode
*/
static const canon_mode_t canon_BJC_6000_modes[] = {
  /* Color/Black cartidge modes */
  { 1440, 720,CANON_INK_CMYK|CANON_INK_CMY|CANON_INK_K,"1440x720dpi",N_("1440x720 DPI"),INKSET(4_C2M2Y2K2),8,0,delay_1440,0.5,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CMYK|CANON_INK_CMY|CANON_INK_K,"720x720dpi",N_("720x720 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK|CANON_INK_CMY|CANON_INK_K,"360x360dpi_high",N_("360x360 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.8,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK|CANON_INK_K,"360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.8,1.0,NULL,NULL,NULL,1},/* 1bpp */
  {  360, 360,CANON_INK_CMYK|CANON_INK_K,"360x360dmt",N_("360x360 DPI DMT"),INKSET(4_C4M4Y4K4),8,0,NULL,1.8,1.0,NULL,NULL,NULL,2},/* unknown mode */
  {  180, 180,CANON_INK_CMYK|CANON_INK_K,"180x180dpi",N_("180x180 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.8,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* Photo cartridge modes */
  { 1440, 720,CANON_INK_CcMmYK|CANON_INK_CMYK|CANON_INK_CMY|CANON_INK_K,"1440x720dpi_photo",N_("1440x720 DPI PHOTO"),INKSET(6_C4M4Y4K4c4m4),8,MODE_FLAG_PHOTO,delay_1440,0.5,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CcMmYK|CANON_INK_CMYK|CANON_INK_CMY|CANON_INK_K,"720x720dpi_photo",N_("720x720 DPI PHOTO"),INKSET(6_C4M4Y4K4c4m4),8,MODE_FLAG_PHOTO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CcMmYK|CANON_INK_CMYK|CANON_INK_CMY|CANON_INK_K,"360x360dpi_highphoto",N_("360x360 DPI HIGH PHOTO"),INKSET(6_C4M4Y4K4c4m4),8,MODE_FLAG_PHOTO,NULL,1.8,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CcMmYK|CANON_INK_CMYK|CANON_INK_K,"360x360dpi_photo",N_("360x360 DPI PHOTO"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.8,1.0,NULL,NULL,NULL,1},/* 1bpp - double the data as for non-photo, maybe uses weaving? */
  {  180, 180,CANON_INK_CMYK|CANON_INK_K,"180x180dpi_photo",N_("180x180 DPI PHOTO"),INKSET(4_C2M2Y2K2),8,0,NULL,1.8,1.0,NULL,NULL,NULL,0},/* 1bpp - same as non-photo */
};
DECLARE_MODES(canon_BJC_6000,1);

static const canon_mode_t canon_BJC_7000_modes[] = {
  { 1200, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"1200x600dpi",N_("1200x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.8,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,3.5,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_7000,1);

static const canon_mode_t canon_BJC_7100_modes[] = {
  { 1200, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"1200x600dpi",N_("1200x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_7100,1);

/* 50i sold as i70 outside of Japan */
static const canon_mode_t canon_BJC_i50_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM gloss"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* color only */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI MONO Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},/* mono only */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i50,1);

/* inkjetHagaki and Hagaki from 80i */
static const canon_mode_t canon_BJC_i80_modes[] = {
  /* legacy modes, from before Windows printjobs fully analysed */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_std2",N_("300x300 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2}, 
  /* plain modes --- Env/Hagaki same */ 
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYKcmy */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* mono only---color untested*/
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* 1bpp */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0}, /* 1bpp */
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST Pro"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO other"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI DRAFT plusGlossy"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjetHagaki */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohpdraft",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYKcmy */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high4",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* mono only---color untested*/
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i80,4);

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
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2h),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i250,1);

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
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO HiRes/glossFilm"),INKSET(9_C5M5Y5K2),8,MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
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
DECLARE_MODES(canon_BJC_i320,2);

/* inkjetHagaki and Hagaki from 450i */
static const canon_mode_t canon_BJC_i450_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color only */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST Pro"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO Other"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT (plusGlossy)"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_BJC_i450,2);

/* inkjetHagaki and Hagaki from 455i */
static const canon_mode_t canon_BJC_i455_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST Pro"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH (gloss)"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYcmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* CMYcmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO Other"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT (plusGlossy)"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* CMYcmy */
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
DECLARE_MODES(canon_BJC_i455,2);

/* inkjetHagaki and Hagaki from 550i */
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
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM gloss"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/plusGlossy/glossFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH Env/Hagaki MONO"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i550,2);

/* inkjetHagaki, Hagaki and CD from 560i */
static const canon_mode_t canon_BJC_i560_modes[] = {
  /* legacy modes from before Windows printjobs fully analysed */
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi_high3",N_("600x600 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.8,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi_std2",N_("300x300 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,3.5,1.0,NULL,NULL,NULL,2},
  /* plain modes */  
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},/*untested*/
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT CD"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_i560,3);

/* inkjetHagaki, Hagaki and CD from 850i */
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
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
DECLARE_MODES(canon_BJC_i850,1);

/* inkjetHagaki, Hagaki and CD from 860i */
static const canon_mode_t canon_BJC_i860_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4k4p),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(9_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT CD"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt: CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i860,1);

/* inkjetHagaki, Hagaki and CD from 900PD */
static const canon_mode_t canon_BJC_i900_modes[] = {
  /* plain modes --- no mono modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y3K3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1bpp */
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST pro"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* CD CMYKcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh5",N_("600x600 DPI HIGH CD"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo5",N_("600x600 DPI CD"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft5",N_("600x600 DPI DRAFT CD"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki --- no mono modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y3K3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_i900,1);

/* inkjetHagaki, Hagaki and CD from 950i */
static const canon_mode_t canon_BJC_i950_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI MEDIUM"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO LOW"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes/PhotoFilm"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO pro/HiRes"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo5",N_("600x600 DPI CD"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft5",N_("600x600 DPI CD DRAFT"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI LOW Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* T-Shirt --- same as plain std */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki --- same as plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high5",N_("600x600 DPI STD Env/Hagaki"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI LOW Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std5",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono4",N_("600x600 DPI MONO LOW Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono5",N_("600x600 DPI MONO DRAFT Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_BJC_i950,2);

/* inkjetHagaki, Hagaki and CD from 960i */
static const canon_mode_t canon_BJC_i960_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH (duplex)"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI MEDIUM"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO LOW"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO pro/HiRes"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo5",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT CD"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI LOW Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0}, /*untested*/
  /* T-Shirt --- same as plain std */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki --- same as plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high5",N_("600x600 DPI MEDIUM Env/Hagaki"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI STD Env/Hagaki"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std5",N_("600x600 DPI LOW Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft5",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono4",N_("600x600 DPI MONO LOW Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono5",N_("600x600 DPI MONO DRAFT Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_BJC_i960,2);

/* inkjetHagaki and Hagaki from 990i */
static const canon_mode_t canon_BJC_i990_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH (duplex)"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI MEDIUM"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO LOW"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes --- many use R ink which is not yet supported */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH HiRes"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM HiRes"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO HiRes/other"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT gloss"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki --- high mode uses unsupported R ink */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT CD"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt --- same as plain std */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki --- same as plain modes: but no special duplex modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki"),INKSET(9_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft5",N_("600x600 DPI LOW Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono4",N_("600x600 DPI MONO LOW Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono5",N_("600x600 DPI MONO DRAFT Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_BJC_i990,2);

/* inkjetHagaki and Hagaki from 6100i */
static const canon_mode_t canon_BJC_i6100_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI MEDIUM"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST gloss"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/plusGlossy"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/gloss/HiRes"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/plusGlossy"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/gloss/HiRes"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
DECLARE_MODES(canon_BJC_i6100,2);

/* inkjetHagaki and Hagaki from 9100i */
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
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/plusGlossy"),INKSET(9_C5M5Y5K5c9m9_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/gloss/HiRes"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/HiRes"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/gloss/HiRes"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C5M5Y5K5c9m9_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C5M5Y5K9c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT Env/Hagaki"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
};
DECLARE_MODES(canon_BJC_i9100,1);

static const canon_mode_t canon_BJC_i9900_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI MEDIUM"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI LOW"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO LOW"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT"),INKSET(11_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes --- many use R, G inks which are not yet supported */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH HiRes"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photomed",N_("600x600 DPI PHOTO MEDIUM HiRes"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO HiRes/other"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT gloss/plusGlossy"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki --- high mode uses unsupported R,G inks */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT CD"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
DECLARE_MODES(canon_BJC_i9900,1);

static const canon_mode_t canon_BJC_8200_modes[] = {
  { 1200,1200,CANON_INK_CMYK,"1200x1200dpi",N_("1200x1200 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},

};
DECLARE_MODES(canon_BJC_8200,1);


static const canon_mode_t canon_BJC_8500_modes[] = {
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYK,"600x600dpi",N_("600x600 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYK,"300x300dpi",N_("300x300 DPI"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_8500,0);


static const canon_mode_t canon_S200_modes[] = {
  { 1440,1440,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
    "1440x1440dpi",N_("1440x1440 DPI"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_WEAVE,delay_S200,0.3,1.0,NULL,NULL,NULL,2},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
    "1440x720dpi",N_("1440x720 DPI"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_WEAVE,delay_S200,0.5,1.0,NULL,NULL,NULL,2},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
     "720x720dpi",N_("720x720 DPI"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_WEAVE,delay_S200,1.0,1.0,NULL,NULL,NULL,2},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
     "360x360dpi",N_("360x360 DPI"),INKSET(4_C2M2Y2K2),8,0,delay_S200,2.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_S200,2);

static const canon_mode_t canon_BJC_S300_modes[] = {
  /* legacy modes from before Windows printjobs fully analysed */
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYK,"600x600dpi_std4",N_("600x600 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYK,"300x300dpi_std2",N_("300x300 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI MEDIUM"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST gloss/PhotoCard"),INKSET(9_C7M7Y7K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss/PhotoCards"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO HiRes/PhotoFilm/PhotoCard"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_S300,4);

static const canon_mode_t canon_BJC_S330_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI MEDIUM"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST gloss"),INKSET(9_C7M7Y7K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO HiRes/matte/PhotoFilm"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_S330,2);

static const canon_mode_t canon_BJC_S500_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST gloss/HiRes/PhotoCard"),INKSET(9_C7M7Y7K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss/HiRes/PhotoCard"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO gloss/HiRes/PhotoCard"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_S500,1);

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
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/plusGlossy/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/gloss/HiRes"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/plusGlossy/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/gloss/HiRes"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
DECLARE_MODES(canon_BJC_S520,2);

static const canon_mode_t canon_BJC_S600_modes[] = {
  /* legacy modes from before Windows printjobs fully analysed */
  { 1200,1200,CANON_INK_CMYK,"1200x1200dpi",N_("1200x1200 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_std2",N_("300x300 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST gloss/HiRes/PhotoCard"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/PhotoFilm"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH gloss/HiRes/PhotoCard"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/PhotoFilm"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO gloss/HiRes/PhotoCard"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
DECLARE_MODES(canon_BJC_S600,4);

static const canon_mode_t canon_BJC_S750_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST gloss"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH pro/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/gloss/HiRes/plusGlossy"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/PhotoFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/gloss/HiRes/plusGlossy"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_BJC_S750,1);

static const canon_mode_t canon_BJC_S800_modes[] = {
  /* legacy modes from before Windows printjobs fully analysed */
  { 1200,1200,CANON_INK_CMYK,"1200x1200dpi",N_("1200x1200 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI (LEGACY)"),INKSET(6_C2M2Y2K2c2m2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST matte/gloss/HiRes/PhotoCard/"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH plusGlossy/PhotoFilm"),INKSET(9_C9M9Y9K9c9m9),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH pro/matte/gloss/HiRes/PhotoCard/other"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/gloss/HiRes/PhotoCard"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_BJC_S800,5);

/* Windows driver shows fewer modes than S800, but it could be they exist */
/* modes present in S800 but not accessible in Windows driver for S820 are marked as untested */
static const canon_mode_t canon_BJC_S820_modes[] = {
  /* plain modes --- also for Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(9_C9M9Y9K9c9m9),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* untested */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO 2"),INKSET(9_K9),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI PHOTO HIGHEST pro/other"),INKSET(9_C9M9Y9K9c9m9),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST matte/gloss/HiRes/PhotoCard/"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH plusGlossy/PhotoFilm"),INKSET(9_C9M9Y9K9c9m9),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH pro/matte/gloss/HiRes/PhotoCard/other"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/gloss/HiRes/PhotoCard"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO other"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_S820,2);

static const canon_mode_t canon_BJC_S900_modes[] = {
  /* plain modes --- also for Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH 2"),INKSET(9_C5M5Y5K9c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO 2"),INKSET(9_K9),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(9_K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(9_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh5",N_("600x600 DPI PHOTO HIGHEST pro"),INKSET(9_C5M5Y5K5c9m9_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI PHOTO HIGHEST HiRes"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH plusGlossy"),INKSET(9_C5M5Y5K5c9m9_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/gloss/PhotoFilm"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH pro/HiRes"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/gloss/HiRes"),INKSET(9_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO DRAFT"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Transparency"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K5_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_BJC_S900,1);

/* no K used in any modes */
static const canon_mode_t canon_PIXMA_mini220_modes[] = {
  { 1200,1200,CANON_INK_CMY,"1200x1200dpi_photohigh",N_("1200x1200 DPI PHOTO HIGHEST Pro"),INKSET(9_c3m3y3),32,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},/* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(9_c3m3y3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH Gloss"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYcmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* CMYcmy */
  /* Hagaki */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh3",N_("600x600 DPI HIGH Hagaki"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYcmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI Hagaki"),INKSET(9_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_mini220,3);

static const canon_mode_t canon_PIXMA_mini320_modes[] = {
  /* most modes used unsupported inks */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(11_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_mini320,0);

static const canon_mode_t canon_SELPHY_DS700_modes[] = {
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST Pro"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: no subtraction */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy: no subtraction */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH Glossy"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Hagaki */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo3",N_("600x600 DPI HIGH Hagaki"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft2",N_("600x600 DPI Hagaki"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_SELPHY_DS700,3);

static const canon_mode_t canon_SELPHY_DS810_modes[] = {
  { 1200,1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST Pro"),INKSET(9_c3m3y3),32,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: no subtraction */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy: no subtraction */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH Glossy"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* Hagaki */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo3",N_("600x600 DPI HIGH Hagaki"),INKSET(9_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft2",N_("600x600 DPI Hagaki"),INKSET(9_C5M5Y5),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_SELPHY_DS810,3);


/* most modes use ESC (S */
static const canon_mode_t canon_PIXMA_iP90_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C9M9Y9K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,4},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},/*|MODE_FLAG_COLOR*/
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DRAFT DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},/*|MODE_FLAG_COLOR*/
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1-bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1-bpp */
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST Pro"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: no subtraction , no ESC (S */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C9M9Y9K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C9M9Y9K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* TST */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C9M9Y9K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* no ESC (S */
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C9M9Y9K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP90,2);

/* all modes use ESC (S */
static const canon_mode_t canon_PIXMA_iP100_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH MONO"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,4},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},/*|MODE_FLAG_COLOR*/
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},/*|MODE_FLAG_COLOR*/
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(11_C2M2Y2K2),8,MODE_FLAG_IP8500|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(11_C2M2Y2K2),8,MODE_FLAG_IP8500|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST Pro"),INKSET(11_C9M9Y4k6),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(11_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO Matte/HiRes"),INKSET(11_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(11_C5M5Y3k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* TST */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(11_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(11_C6M6Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_S,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP100,2);

/* simpler compared to iP110, in fact commands in line with other normal-sized PIXMA devices */
/* no Esc (S command */
/* 2 ink carts: (1) CMYk (2) pigment black K */
/* special inksaving options to save ink and/or use only remaining ink: */
/* (not exclusive): black-saving mode, composite black, black-saving + composite black both active */
static const canon_mode_t canon_PIXMA_iP110_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH MONO"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},/* color untested */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(11_C2M2Y2K2),8,MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(11_C2M2Y2K2),8,MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST (Plus Glossy II)"),INKSET(11_C9M9Y4k6),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(11_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO (Matte/HiRes)"),INKSET(11_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(11_C5M5Y3k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* TST - copied from iP100 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(11_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high4",N_("600x600 DPI HIGH (Env/Hagaki)"),INKSET(11_C6M6Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH MONO (Env/Hagaki)"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI (Env/Hagaki)"),INKSET(11_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP110,2);

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
DECLARE_MODES(canon_PIXMA_iP1000,2);

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
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi",N_("1200x1200 DPI PHOTO"),INKSET(13_c3m3y3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy */
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
DECLARE_MODES(canon_PIXMA_iP1200,2);

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
DECLARE_MODES(canon_PIXMA_iP1500,1);

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
  /* photo */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGH2"),INKSET(13_c3m3y3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C5M5Y5),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color only plain, hagaki, Env*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(13_C4M4Y4K2c4m4y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI 2"),INKSET(13_C3M3Y2b),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_2",N_("300x300 DPI 2"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT 2"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},

};
DECLARE_MODES(canon_PIXMA_iP1900,1);

static const canon_mode_t canon_PIXMA_iP2000_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1-bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1-bpp */
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST Pro"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH Gloss"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO Other"),INKSET(9_c9m9y9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh4",N_("600x600 DPI PHOTO HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI PHOTO Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /*  T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* color untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP2000,2);

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
DECLARE_MODES(canon_PIXMA_iP3000,1);

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
DECLARE_MODES(canon_PIXMA_iP3100,1);

static const canon_mode_t canon_PIXMA_iP4000_modes[] = {
  /* plain modes --- same for duplex */
  /* NOTE: temporarily allowing plain modes for CD printing */
  /* legacy modes were used before printer capabilites were improved to handle real mode from driver */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high4",N_("600x600 DPI HIGH (LEGACY)"),INKSET(4_C2M2Y2K2),16,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},/* legacy */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std4",N_("600x600 DPI (LEGACY)"),INKSET(4_C2M2Y2K2),16,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},/* legacy */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT (LEGACY)"),INKSET(4_C2M2Y2K2),16,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},/* legacy */
  /* actual printer modes --- same as for iP4100 */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4k4p),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
  /* Transparency CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt: CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP4000,4);

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
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
  /* Transparency CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt: CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP4100,1);

static const canon_mode_t canon_PIXMA_iP4200_modes[] = {
  /* plain modes */
  /* NOTE: temporarily allowing plain modes for CD printing */
  /* highest plain mode: CcMmYKk */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Q2 - standard mode for this driver (in windows driver it's Q3) */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Q1 - normal 300x300 mode (in windows driver it's Q4 - normal darkness of printout ) */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,iP4200_300dpi_lum_adjustment,NULL,NULL,1},
  /* Q0 - fastest mode (in windows driver it's Q5, printer uses 50% of ink ( I think )) */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,iP4200_300dpi_draft_lum_adjustment,NULL,NULL,0},
  /* photo modes mostly use unsupported ink positions -- also use one for temporary CD printing */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet hagaki */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-Shirt"),INKSET(22_C4M4Y4K2k4one),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency CMYKk --- MP500 */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photohigh3",N_("600x600 DPI HIGH Transparency"),INKSET(22_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photo3",N_("600x600 DPI Transparency"),INKSET(22_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP4200,1);

static const canon_mode_t canon_PIXMA_iP4300_modes[] = {
  /* plain modes */
  /* NOTE: temporarily allowing plain modes for CD printing */
  /* highest plain mode: CcMmYKk */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,iP4200_300dpi_lum_adjustment,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,iP4200_300dpi_draft_lum_adjustment,NULL,NULL,0},
  /* photo modes mostly use unsupported ink positions --- use photodraft for CD temporarily also */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet hagaki */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2k4one),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency CMYKk --- MP500 */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photohigh3",N_("600x600 DPI HIGH Transparency"),INKSET(22_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photo3",N_("600x600 DPI Transparency"),INKSET(22_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP4300,1);

/* plain high mode has more output inks than declared in inkset, and unknown to boot */
static const canon_mode_t canon_PIXMA_iP5000_modes[] = {
  /* plain modes */
    /* NOTE: temporarily allowing plain modes for CD printing */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},/* 1-bpp, untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1-bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1-bpp */
  /* photo --- most use plain high mode which is currently unsupported --- photodraft temporarily for CD printing */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT PlusGlossy"),INKSET(9_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* CMYcmk */
  /* TST CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP5000,0);

static const canon_mode_t canon_PIXMA_iP6000_modes[] = {
  /* plain modes */
  /* NOTE: temporarily allowing plain modes for CD printing */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(9_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y3K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST / CD"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH / CD"),INKSET(9_C8M8Y4K4c8m8),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO / CD"),INKSET(9_C4M4Y4K4c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_PIXMA_iP6000,2);

/* iP6210D, iP6220D, iP6310D, iP6320D */
static const canon_mode_t canon_PIXMA_iP6210_modes[] = {
  /* both cartridges */
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4k4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y3k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},/* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2b),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  1200, 1200,CANON_INK_CcMmYK,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST Pro"),INKSET(13_C3M3Y3k3c3m3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},/* CMYkcm less 0x80 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C9M9Y9k9c9m9),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYkcm less 0x80 */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4k4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* color-only */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh4",N_("1200x1200 DPI PHOTO HIGHEST Pro COLOR-ONLY"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},/*same*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH COLOR-ONLY"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh5",N_("600x600 DPI PHOTO HIGH plusGlossy COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},/* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},/* cmy: subtract 0x60 */
  /* T-shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std5",N_("600x600 DPI T-SHIRT COLOR-ONLY"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y4k4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C4M4Y4k4c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y3k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},/* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2b),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP6210,1);

static const canon_mode_t canon_PIXMA_iP6600_modes[] = {
  /* plain modes --- duplex same CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(19_C3M3Y3k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYkcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(19_C7M7Y4c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(19_C6M6Y4c6m6k4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYkcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(19_C7M7Y4c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(19_C6M6Y4c6m6k4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_PIXMA_iP6600,1);

static const canon_mode_t canon_PIXMA_iP6700_modes[] = {
  /* plain modes --- duplex same CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(19_C3M3Y3k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYkcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(19_C7M7Y4c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(19_C6M6Y4c6m6k4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_PIXMA_iP6700,1);

/* ------------------------------Start of MP series------------------------------ */

/* MP5, MP10, MPC190, MPC200 */
static const canon_mode_t canon_PIXMA_MP5_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST Gloss"),INKSET(9_C7M7Y7K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH Pro,Super,Matte,HiRes,GlossyFilm"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
    {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO High Gloss"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO Matte,HiRes,GlossyFilm"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO Pro,Super,Gloss"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* inkjet Kansei Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI PHOTO HIGH inkjet Hagaki"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI PHOTO inkjet Hagaki"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_stdmono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_MP5,1);

/* MP55 */
static const canon_mode_t canon_PIXMA_MP55_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_medium",N_("600x600 MEDIUM DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST Gloss"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH Pro,Super,GlossyFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH Matte,Gloss,HiRes"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO Pro,Super,GlossyFilm"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO Matte,Gloss,HiRes"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Kansei Hagaki Note: has monochrome */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI PHOTO HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI PHOTO inkjet Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},  
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_MP55,2);

static const canon_mode_t canon_PIXMA_MPC400_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes (PPother, PPmatte are experimental) */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST HiRes,Glossy,GlossyPhotoCard,Matte"),INKSET(9_C4M4Y4K2c4m4y4twobit),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
      {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH Pro,GlossyFilm"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH HiRes,Glossy,GlossyPhotoCard,Matte,Other"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
    {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO Pro,GlossyFilm"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO HiRes,Glossy,GlossyPhotoCard,Matte,Other"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes,Matte"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki: experimental as not included in overseas model drivers */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh4",N_("600x600 DPI PHOTO HIGH inkjet Hagaki"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo4",N_("600x600 DPI PHOTO inkjet Hagaki"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},  
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C2M2Y2K2c2m2y2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki (Hagaki is experimental as not included in overseas model drivers) */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_MPC400,1);

static const canon_mode_t canon_MULTIPASS_MP150_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* untested: but Env/Hagaki uses it */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh3",N_("1200x1200 DPI PHOTO HIGHEST Pro"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH Gloss"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI TSHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP150,1);

static const canon_mode_t canon_MULTIPASS_MP190_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST Pro/Gold"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO Matte/HiRes/inkjetHagaki"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI TSHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
};
DECLARE_MODES(canon_MULTIPASS_MP190,1);

static const canon_mode_t canon_MULTIPASS_MP210_modes[] = {
  /* both cartridges */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST Pro"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO Matte/HiRes/inkjetHagaki"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP210,1);

/* MP360, MP370, MP375R, MP390 */
static const canon_mode_t canon_MULTIPASS_MP360_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK | CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),16,0,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGHEST Pro"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},/* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C9M9Y9K2c9m9y9photo8),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH Gloss"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/*CMYcmy*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4y4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* CMYcmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C5M5Y5K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4y4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_MP360,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK | CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP360,2);

static const canon_mode_t canon_MULTIPASS_MX300_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/*cmy: subtract 0x60*/
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},/*cmy: subtract 0x60*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST Pro"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},/*cmy: subtract 0x60*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /*cmy: subtract 0x60*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO Matte/HiRes/inkjetHagaki"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},/*cmy: subtract 0x60*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI TSHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/*cmy: subtract 0x60*/
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono3",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH 2"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},/*cmy: subtract 0x60*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MX300,1);

static const canon_mode_t canon_MULTIPASS_MX330_modes[] = {
  /* plain media */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color-only*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST glossGold"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},/* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono3",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
};
DECLARE_MODES(canon_MULTIPASS_MX330,1);

static const canon_mode_t canon_MULTIPASS_MX340_modes[] = {
  /* plain media */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* color-only*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST glossGold"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},/* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono3",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
};
DECLARE_MODES(canon_MULTIPASS_MX340,1);

static const canon_mode_t canon_MULTIPASS_MX360_modes[] = {
  /* plain media */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
/* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH (COLOR-ONLY)"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI (COLOR-ONLY)"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT (COLOR-ONLY)"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI (COLOR-ONLY)"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT (COLOR-ONLY)"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST glossProPlat"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},  
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* TST */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_MULTIPASS_MX360,1);

static const canon_mode_t canon_MULTIPASS_MX370_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI STD COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST proPlat"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* CMYcmy, cmy less 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX370,1);

static const canon_mode_t canon_MULTIPASS_MX420_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST proPlat"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX420,1);

/* same as MX420, but try to use inktypes to control use of inks in inkets */
static const canon_mode_t canon_MULTIPASS_MX470_modes[] = {
  /* plain modes - borderless uses CMY */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high5",N_("600x600 DPI HIGH (COLOR-ONLY)"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST proPlat"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH / T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki modes - borderless uses CMY */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX470,1);

static const canon_mode_t canon_MULTIPASS_MX510_modes[] = {
  /* plain modes --- try to use based on inkset definitions */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std4",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST proPlat"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* CMYcmy, cmy less 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki --- try to use based on inkset definitions */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX510,2);

static const canon_mode_t canon_MULTIPASS_MP230_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST proPlat"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* TST */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* Env/Hagaki color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
};
DECLARE_MODES(canon_MULTIPASS_MP230,1);

static const canon_mode_t canon_MULTIPASS_MP240_modes[] = {
  /* plain color modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST Pro"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* Env/Hagaki color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
};
DECLARE_MODES(canon_MULTIPASS_MP240,1);

static const canon_mode_t canon_MULTIPASS_MP250_modes[] = {
  /* plain color modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO (duplex)"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST glossPro/proPhotoHagaki"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high5",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  600, 600,CANON_INK_K,"600x600dpi_highmono5",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
};
DECLARE_MODES(canon_MULTIPASS_MP250,2);

static const canon_mode_t canon_MULTIPASS_MP280_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST glossProPlat"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* TST */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono2",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* Env/Hagaki color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft4",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
};
DECLARE_MODES(canon_MULTIPASS_MP280,1);

static const canon_mode_t canon_MULTIPASS_MP470_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested but Env/Hagaki use it */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST Pro"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_c9m9y9),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo2",N_("600x600 DPI PHOTO Matte/HiRes/inkjetHagaki"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C5M5Y5),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C4M4Y4K2c4m4y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono3",N_("600x600 DPI DRAFT MONO Env/Hagaki"),INKSET(13_K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C4M4Y4c4m4y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* subtract 0x60 from cmy */
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft42",N_("600x600 DPI DRAFT Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP470,1);

static const canon_mode_t canon_MULTIPASS_MP480_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft2",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft2",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST glossPro"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP480,1);

static const canon_mode_t canon_MULTIPASS_MP490_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_draftmono",N_("600x600 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K,"300x300dpi_draftmono",N_("300x300 DPI DRAFT MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_draft3",N_("600x600 DPI DRAFT COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMY,"300x300dpi_draft3",N_("300x300 DPI DRAFT COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_photohigh2",N_("1200x1200 DPI PHOTO HIGHEST glossPro/proPhotoHagaki"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* cmy: subtract 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP490,1);

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
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO plusGlossy/glossGold/gloss"),INKSET(22_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_MULTIPASS_MP520,1);

/* photo modes mostly use unsupprted inks */
static const canon_mode_t canon_MULTIPASS_MP530_modes[] = {
  /* plain modes --- duplex same */
  /* NOTE: temporarily allowing plain modes for CD printing */
  {  600, 600,CANON_INK_K|CANON_INK_CcMmYKk,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},/* CMYKcmk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcmk --- temporarily using as CD mode also */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2k4one),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP530,1);

static const canon_mode_t canon_MULTIPASS_MP540_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* 16 */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO inkjet Hagaki"),INKSET(13_C5M5Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* 8 not 16 */
  /* T-Shirt --- same as photo high */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP540,1);

/* similar to MP540 but fewer modes */
static const canon_mode_t canon_MULTIPASS_MP550_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT 2"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt --- same as photo high */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP550,1);

/* similar to MP540 but fewer modes */
static const canon_mode_t canon_MULTIPASS_MP560_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt --- same as photo high */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP560,2);

/* similar to MP830 */
/* most photo modes use unsupported inks */
static const canon_mode_t canon_MULTIPASS_MP600_modes[] = {
  /* plain modes --- duplex same */
  /* NOTE: temporarily allowing plain modes for CD printing */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},/* k off */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes --- most unsupported --- using photodraft for CD temporarily too */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},/* K off */
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* K off */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /* K off */
  /* TST */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2k4one),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* K off */
  /* Env/Hagaki --- same as plain modes */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP600,1);

static const canon_mode_t canon_MULTIPASS_MP640_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/* untested */
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* inkjet Hagaki / CD */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjetHagaki/CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjetHagaki/CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt --- same as photo high */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP640,2);

/* similar to iP3000, but no duplex and different default standard modes, and all modes use K */
static const canon_mode_t canon_MULTIPASS_MP700_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* with K */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes CMYKcm/CMYK */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki CMYKcm/CMYK -- from Japanese model */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGHEST inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* untested */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD CMYKcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI CD HIGH"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI CD"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI CD DRAFT"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency CMYK */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYK */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKcm/CMYK */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGHEST Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* untested */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP700,1);

/* similar to iP3000 --- unlike MP700/730, the MP710/740 also do not use K for photo modes */
static const canon_mode_t canon_MULTIPASS_MP710_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* with K */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_high",N_("300x300 DPI HIGH"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,1},/* 1bpp */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(4_C2M2Y2K2),8,0,NULL,1.0,1.0,NULL,NULL,NULL,0},/* 1bpp */
  /* photo modes CMYcm/CMY */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT HiRes"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki CMYcm/CMY */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGHEST inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* untested */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI CD HIGH"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI CD"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI CD DRAFT"),INKSET(9_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency CMYK */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohphigh",N_("600x600 DPI HIGH Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_ohp",N_("600x600 DPI Transparency"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMY */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(9_C4M4Y4K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKcm/CMYK */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGHEST Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/* untested */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(9_C4M4Y4K2c4m4plain),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(9_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_MULTIPASS_MP710,1);

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
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO pro/HiRes"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO / DRAFT PHOTO HiRes"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(9_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT CD"),INKSET(9_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Transparency */
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
DECLARE_MODES(canon_MULTIPASS_MP900,1);

static const canon_mode_t canon_PIXMA_iP2700_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* mono modes */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI HIGH MONO"),INKSET(13_K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high3",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std3",N_("600x600 DPI COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std3",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  1200, 1200,CANON_INK_CMY,"1200x1200dpi_high",N_("1200x1200 DPI HIGHEST glossPro"),INKSET(13_c3m3y3),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},/* cmy: subtract 0x60*/
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono2",N_("600x600 DPI HIGH MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono2",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP2700,1);

static const canon_mode_t canon_MULTIPASS_MX710_modes[] = {
  /* plain modes --- duplex uses CMYKk for high and standard, fast remains the same */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI (duplex)"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  /*highest proPlat mode unsupported */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX710,2);

static const canon_mode_t canon_PIXMA_MX720_modes[] = {
 /* plain - used for mono also. For duplex use CMYKk */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K3k4_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH (Duplex)"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI (Duplex)"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYKk|CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH inkjetHagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO inkjetHagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_MX720,2);

/* MX860 --- similar to iP4500 */
static const canon_mode_t canon_MULTIPASS_MX860_modes[] = {
  /* plain modes --- duplex same CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX860,2);

/* added duplex to MP540 */
static const canon_mode_t canon_MULTIPASS_MX880_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3}, /* 16 */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO inkjet Hagaki"),INKSET(13_C5M5Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* 8 not 16 */
  /* T-Shirt --- same as photo high */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX880,1);

static const canon_mode_t canon_PIXMA_MX920_modes[] = {
 /* plain - used for mono also. For duplex use CMYKk */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K3k4_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH (Duplex)"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI (Duplex)"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYKk|CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH CD/inkjetHagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO CD/inkjetHagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_MX920,2);

static const canon_mode_t canon_MULTIPASS_MX7600_modes[] = {
  /* plain modes CMYK --- duplex same */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K3k4_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2k2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C6M6Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, CANON_INK_K|600,CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C6M6Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, CANON_INK_K|600,CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C6M6Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MX7600,1);

static const canon_mode_t canon_MULTIPASS_E480_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(7_K3C4M4Y3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH (COLOR-ONLY)"),INKSET(7_K2C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(7_K2C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(7_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST plus Glossy II"),INKSET(7_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* CMYcmy, cmy less 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(7_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(7_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH (Env/Hagaki)"),INKSET(7_K2C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI (Env/Hagaki)"),INKSET(7_K2C3M3Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_E480,1);

static const canon_mode_t canon_MULTIPASS_E500_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (COLOR-ONLY"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST proPlat"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* CMYcmy, cmy less 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_E500,2);

static const canon_mode_t canon_MULTIPASS_E560_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH DUPLEX / COLOR-ONLY"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST proPlat"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* CMYcmy, cmy less 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH / T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_E560,2);

static const canon_mode_t canon_PIXMA_P200_modes[] = {
  /* plain modes, also for Env/Hagaki */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST plus Glossy II"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4}, /* CMYcmy, cmy less 0x60 */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_P200,1);

static const canon_mode_t canon_PIXMA_iP4500_modes[] = {
  /* plain modes -- duplex same (except no mono) */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT Plus/PlusGlossyII"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(13_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* CD CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT CD"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C6M6Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt CMYk --- note this mode uses 2 bits not 4 */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP4500,1);

static const canon_mode_t canon_PIXMA_iP4600_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1}, /*untested*/
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI HIGH PHOTO/CD"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes/CD"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT/CD"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH 2"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/*untested*/
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT 2"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft3",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* Hagaki and Env modes used K and k both */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/*untested*/
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI HIGH T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_iP4600,1);

static const canon_mode_t canon_PIXMA_iP4700_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /*T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_iP4700,2);

static const canon_mode_t canon_PIXMA_iP4900_modes[] = {
  /* plain */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* duplex, Env, Hagaki  modes use K and k both */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex/Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex/Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_iP4900,1);

static const canon_mode_t canon_PIXMA_iP5300_modes[] = {
  /* plain modes --- duplex same */
  /* NOTE: temporarily allowing plain modes for CD printing */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_PIXMA_iP5300,1);

/* similar to iP5300 but PPplusGlossy different standard mode, PPGgold added */
static const canon_mode_t canon_MULTIPASS_MP610_modes[] = {
  /* plain modes --- duplex same */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C4M4Y4K2c4m4y4on2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy/GlossGold"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_MULTIPASS_MP610,1);

static const canon_mode_t canon_MULTIPASS_MP800_modes[] = {
  /* most photo modes use some unknown inks */
  /* plain modes */
  /* NOTE: temporarily allowing plain modes for CD printing */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Fast Photo paper mode CMYcmk --- also using temporarily for CD printing */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki: PhotoFastCMYcmk, Std CMYk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP800,1);

static const canon_mode_t canon_MULTIPASS_MP810_modes[] = {
  /* plain paper modes */
  /* NOTE: temporarily allowing plain modes for CD printing */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  /* Photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH 2 / CD"),INKSET(13_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO 2 / CD"),INKSET(13_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT 2 / CD"),INKSET(13_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(13_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C6M6Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},

};
DECLARE_MODES(canon_MULTIPASS_MP810,1);

/* similar to MP600 */
/* most photo modes use unsupported inks */
static const canon_mode_t canon_MULTIPASS_MP830_modes[] = {
  /* plain modes --- duplex same */
  /* NOTE: temporarily allowing plain modes for CD printing */
  {  600, 600,CANON_INK_CcMmYKk|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4k4on),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes --- photodraft for CD temporarily too */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(22_C3M3Y3K2c3m3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photodraft2",N_("600x600 DPI DRAFT inkjet Hagaki"),INKSET(22_C3M3Y2K2k3photo_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*CMYk*/
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(22_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(22_C4M4Y4K2k4one),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP830,1);

static const canon_mode_t canon_MULTIPASS_MP950_modes[] = {
  /* plain modes CMYK --- same for duplex */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4K2c6m6k4off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI STD"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* inkjet Hagaki --- same as photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Fine Art CMYcmk */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh4",N_("600x600 DPI FineArt"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh5",N_("600x600 DPI HIGH CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo5",N_("600x600 DPI CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft5",N_("600x600 DPI DRAFT CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt transfers CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(19_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Transparency"),INKSET(19_C4M4Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Transparency"),INKSET(19_C4M4Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP950,1);

/* high resolution mono modes use unknown inks */
static const canon_mode_t canon_MULTIPASS_MP960_modes[] = {
  /* plain modes CMYK */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH (duplex)"),INKSET(19_C6M6Y4K2c6m6k4off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI STD"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  /* inkjet Hagaki --- same as photo modes */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Fine Art CMYcmk */
  {  600, 600,CANON_INK_CcMmYyK,"600x600dpi_photohigh5",N_("600x600 DPI FineArt"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(19_C6M6Y4K2c6m6k4hagaki),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(19_C3M3Y3K2k3),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft4",N_("600x600 DPI DRAFT CD"),INKSET(19_C4M4Y4c4m4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt transfers CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(19_C4M4Y4K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Transparency CMYKk --- untested (taken from MP950) */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_high4",N_("600x600 DPI HIGH Transparency"),INKSET(19_C4M4Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_std4",N_("600x600 DPI Transparency"),INKSET(19_C4M4Y4K2k4on),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* photo modes high & std (600dpi) CMYcmk */
  /* photo mode 2 (600dpi) CMYcmk --- works, but c,m may need a bit of tone/density adjusting */
  /* photo mode 3 CD (600dpi) CMYcmk --- c,m too light, may need adjusting tone/density */
  /* inkjet hagaki outputs in CMYcmk std, hi --- c,m too light, may need adjusting tone/density */
};
DECLARE_MODES(canon_MULTIPASS_MP960,2);

static const canon_mode_t canon_MULTIPASS_MP970_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH (duplex)"),INKSET(19_C6M6Y4K2c6m6k4off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/* untested */
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes CMYcmk */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH / FineArt"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},  
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(19_C6M6Y4K2c6m6k4hagaki),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(19_C6M6Y4K2c6m6k4hagaki),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_MULTIPASS_MP970,2);

/* most photo modes use H (grey) so unsupported presently */
static const canon_mode_t canon_MULTIPASS_MP980_modes[] = {
  /* normal modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(16_C6M6Y4K2k4off),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH (duplex)"),INKSET(16_C3M3Y2K2k3off_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(16_C3M3Y2K2k3off_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(16_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(16_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Photo modes: matte, high resolution */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(16_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(16_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD -- same as above */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH CD"),INKSET(16_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI CD"),INKSET(16_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Photo modes: inkjet Hagaki */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(16_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(16_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-shirt --- same as Photo High */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(16_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Photo modes: other */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(16_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(16_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(16_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MULTIPASS_MP980,2);

static const canon_mode_t canon_MULTIPASS_MP990_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI HIGH"),INKSET(30_K2C6M6Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH (duplex)"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
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
DECLARE_MODES(canon_MULTIPASS_MP990,1);

static const canon_mode_t canon_PIXMA_iX4000_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Photo: CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(22_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
DECLARE_MODES(canon_PIXMA_iX4000,1);

static const canon_mode_t canon_PIXMA_iX5000_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(22_C4M4Y4K2c4m4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(22_C3M3Y2K2_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(22_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* Photo: CMYcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(22_C4M4Y4K2c4m4photo),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(22_C3M3Y3K2c3m3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
DECLARE_MODES(canon_PIXMA_iX5000,1);

static const canon_mode_t canon_PIXMA_iX6800_modes[] = {
 /* plain */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K3k4_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0}, /* untested */
  /* photo CMYKk */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH / T-Shirt"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
    {  600, 600,CANON_INK_CMYKk,"600x600dpi_photo2",N_("600x600 DPI PHOTO Matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Inkjet Hagaki*/
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photohigh2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photo3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high2",N_("600x600 DPI"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
    {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_std2",N_("600x600 DPI"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iX6800,1);

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
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photo2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C6M6Y2K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photodraft2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C6M6Y2K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Fine Art */
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_photo3",N_("600x600 DPI HIGH FineArt"),INKSET(13_C6M6Y2K2k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_iX7000,1);

/* mono uses bw=2 */
static const canon_mode_t canon_PIXMA_Pro9000_modes[] = {
  /* Plain Modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high2",N_("600x600 DPI HIGH 2"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},/* experimental */
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* experimental */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI LOW"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT 2"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* plain mono */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono",N_("600x600 DPI MONO HIGH"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},/* experimental */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* experimental */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT 2"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
/* Photo Modes */
/* highest uses R,G so cannot support */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO HIGH gloss"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH GlossPro/proPlat/pro"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO DRAFT gloss"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki 2"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},/* experimental */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI MONO HIGH Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},/* experimental */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},/* experimental */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft3",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* board, canvas, FineArt, Museum Etching */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh5",N_("600x600 DPI Board/Canvas/FineArt/Museum"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
};
DECLARE_MODES(canon_PIXMA_Pro9000,1);

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
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(11_C6M6Y6K6c6m6_c),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
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
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI MONO HIGH Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft2",N_("600x600 DPI DRAFT Env/Hagaki"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* board, canvas, FineArt, Museum Etching */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh5",N_("600x600 DPI Board/Canvas/FineArt/Museum"),INKSET(11_C6M6Y6K6c6m6_c),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
};
DECLARE_MODES(canon_PIXMA_Pro9000mk2,1);

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
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono2",N_("600x600 DPI MONO PLAIN/Hagaki HIGH"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono",N_("600x600 DPI MONO Hagaki STD"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_mono",N_("600x600 DPI MONO PLAIN STD"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT2"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
/* Photo modes: only mono using CMYK-only can be supported currently */
/* this has k instead of K --- also CD */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photomono",N_("600x600 DPI MONO PHOTO / CD"),INKSET(11_C16M16Y16k16),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
};
DECLARE_MODES(canon_PIXMA_Pro9500,1);

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
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono2",N_("600x600 DPI MONO PLAIN/Hagaki HIGH"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_highmono",N_("600x600 DPI MONO Hagaki STD"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_mono",N_("600x600 DPI MONO PLAIN STD"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono",N_("600x600 DPI MONO DRAFT"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draftmono2",N_("600x600 DPI MONO DRAFT2"),INKSET(11_C2M2Y2K2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
/* Photo modes: only mono using CMYK-only can be supported currently */
/* this has k instead of K --- also CD */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photomono",N_("600x600 DPI MONO PHOTO / CD"),INKSET(11_C16M16Y16k16),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,4},
};
DECLARE_MODES(canon_PIXMA_Pro9500mk2,1);

/* iP7100 */
/* ESC (r command is 0x64 but another one befor data is sent: ESC (r 0x62 0x0 */
static const canon_mode_t canon_PIXMA_iP7100_modes[] = {
  /* plain modes: color */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
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
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO / CD High"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO plusDS/gloss"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI CD Draft"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Fine Art*/
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO FineArt"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP7100,2);

static const canon_mode_t canon_PIXMA_iP7200_modes[] = {
 /* plain - used for mono also. For duplex use CMYKk */
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K3k4_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH (Duplex)"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI (Duplex)"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH CD/inkjetHagaki/HiRes"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI PHOTO CD/inkjetHagaki/HiRes"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki */
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYKk|CANON_INK_K,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP7200,2);

static const canon_mode_t canon_PIXMA_iP7500_modes[] = {
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(19_C6M6Y4K2c6m6k4off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK|CANON_INK_K,"600x600dpi",N_("600x600 DPI"),INKSET(19_C3M3Y3K2k3off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi",N_("300x300 DPI"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_CMYK|CANON_INK_K,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(19_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},
  /* photo modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(19_C7M7Y4K2c7m7k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(19_C6M6Y4K2c6m6k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(19_C4M4Y4K2c4m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_PIXMA_iP7500,1);

/* iP8100 */
/* ESC (r command is 0x64 but another one befor data is sent: ESC (r 0x62 0x0 */
static const canon_mode_t canon_PIXMA_iP8100_modes[] = {
  /* plain modes: color */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
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
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO / CD High"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO plusDS/gloss"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI CD Draft"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Fine Art*/
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI PHOTO FineArt"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP8100,2);

/* iP8500 */
/* ESC (r command is 0x64 but another one befor data is sent: ESC (r 0x62 0x0 */
static const canon_mode_t canon_PIXMA_iP8500_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
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
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH HiRes"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO HiRes/other"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_PIXMA_iP8500,2);

/* iP8600 */
/* ESC (r command is 0x64 but another one befor data is sent: ESC (r 0x62 0x0 */
static const canon_mode_t canon_PIXMA_iP8600_modes[] = {
  /* plain modes: color */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
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
  /* one mode for PPpro ud1, hi, and PPplusGlossy hi cannot be supported yet as it uses Red (R) and Green (G) ink in addition to CMYKcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH plusDS/gloss"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO pro/plusGlossy/matte/HiRes CD High"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO plusDS/gloss"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
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
DECLARE_MODES(canon_PIXMA_iP8600,2);

/* for this printer most photo modes have grey (H) ink. */
/* borderless: plain (qlty 2 only), hagaki (qlty 3 and 2) use std mode with CMYk, those with photo modes unchanged. No env, HiRes, TST for borderless */
/* Mono modes not yet checked */
static const canon_mode_t canon_PIXMA_iP8700_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K3C6M6Y4k4off_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K3C3M3Y2k3off_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(30_K3C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(30_K3C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes/T-shirt"),INKSET(30_M8K8m4k4H8off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(30_M8K8m4k4H8off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO other"),INKSET(30_M6K6m4k4H6off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},/*untested*/
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_iP8700,1);

/* iP9910 */
/* ESC (r command before data is sent: ESC (r 0x62 0x0 */
static const canon_mode_t canon_PIXMA_iP9910_modes[] = {
  /* plain modes: color */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(11_C6M6Y6K9c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,4},
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
  /* one mode for PPpro hi, and PPplusGlossy hi cannot be supported yet as it uses Red (R) and Green (G) ink in addtion to CMYKcm */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH plusDS/gloss"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo",N_("600x600 DPI PHOTO pro"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO pro/plusGlossy/matte/HiRes/other CD High"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft",N_("600x600 DPI PHOTO gloss CD Draft"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photodraft2",N_("600x600 DPI PHOTO DRAFT plusGlossy"),INKSET(11_C5M5Y5K5c5m5_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* inkjet Hagaki separate out */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjetHagaki"),INKSET(11_C6M6Y6K6c16m16_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photo3",N_("600x600 DPI inkjetHagaki"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Fine Art */
  {  600, 600,CANON_INK_CcMmYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH FineArt"),INKSET(11_C6M6Y6K6c6m6_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_PRO,NULL,1.0,1.0,NULL,NULL,NULL,4},
  /* Transparency */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_std4",N_("600x600 DPI Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_draft4",N_("600x600 DPI DRAFT Transparency"),INKSET(11_C6M6Y6K6_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},
};
DECLARE_MODES(canon_PIXMA_iP9910,2);

static const canon_mode_t canon_PIXMA_MG2100_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI MONO HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2K3off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI STANDARD COLOR-ONLY"),INKSET(13_C3M3Y2K3off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2K2off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST proPlat"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI MONO HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2K3off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI STANDARD Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2K3off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_MG2100,1);

static const canon_mode_t canon_PIXMA_MG2400_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI (incl. T-shirt)"),INKSET(13_C3M3Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST Plus Glossy II"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_MG2400,1);

static const canon_mode_t canon_PIXMA_MG3100_modes[] = {
  /* plain modes - borderless uses CMY */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high5",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* mono */
  {  600, 600,CANON_INK_K,"600x600dpi_highmono",N_("600x600 DPI MONO HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K,"300x300dpi_mono",N_("300x300 DPI MONO"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* color-only modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high2",N_("600x600 DPI HIGH COLOR-ONLY"),INKSET(13_C3M3Y2K3off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std2",N_("600x600 DPI STANDARD COLOR-ONLY"),INKSET(13_C3M3Y2K3off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_CMY,"300x300dpi_std2",N_("300x300 DPI COLOR-ONLY"),INKSET(13_C2M2Y2K2off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST proPlat"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt */
  {  600, 600,CANON_INK_CMY,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* Env/Hagaki modes - borderless uses CMY */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_highmono3",N_("600x600 DPI MONO HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K,"600x600dpi_mono3",N_("600x600 DPI MONO Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* color-only */
  {  600, 600,CANON_INK_CMY,"600x600dpi_high4",N_("600x600 DPI HIGH Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2K3off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_std4",N_("600x600 DPI STANDARD Env/Hagaki COLOR-ONLY"),INKSET(13_C3M3Y2K3off),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_MG3100,2);

/* same as MG3100, but try to use inktypes to control use of inks in inkets */
static const canon_mode_t canon_PIXMA_MG3500_modes[] = {
  /* plain modes - borderless uses CMY */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high5",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Photo modes */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST proPlat"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH / T-Shirt"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki modes - borderless uses CMY */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_MG3500,2);

static const canon_mode_t canon_PIXMA_MG3600_modes[] = {
  /* plain modes - borderless uses CMY */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y3K4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_high5",N_("600x600 DPI HIGH (color)"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* Photo modes (incl. inkjet Hagaki, inkjet Photo Hagaki) */
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGHEST proPlat"),INKSET(13_C14M14Y14c14m14y14),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,4},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH / T-Shirt"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMY,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C4M4Y4),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki modes - borderless uses CMY */
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMY|CANON_INK_CMYK,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K3),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_BLACK|MODE_FLAG_COLOR,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_MG3600,2);

static const canon_mode_t canon_PIXMA_MG5100_modes[] = {
  /* plain modes --- duplex no mono */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},/*mono untested*/
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Hagaki/Env: use K and k both CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),8,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG5100,2);

static const canon_mode_t canon_PIXMA_MG5200_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,1},/*untested*/
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi_draft",N_("300x300 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,0},/*untested*/
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG5200,2);

static const canon_mode_t canon_PIXMA_MG5300_modes[] = {
  /* plain modes -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K2y4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K2y3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex/Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex/Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex/Env/Hagaki"),INKSET(13_C3M3Y2K2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG5300,1);

static const canon_mode_t canon_PIXMA_MG5400_modes[] = {
  /* plain modes -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K3k4_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex/Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex/Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex/Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C8M8Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG5400,1);

/* borderless: plain (qlty 2 only), hagaki (qlty 3 and 2) use std mode with CMYk, those with photo modes unchanged. No env, HiRes, TST for borderless */
static const canon_mode_t canon_PIXMA_MG5500_modes[] = {
  /* plain modes -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C4M4Y4K3k4off_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex/Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex/Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex/Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI PHOTO HIGH matte/HiRes/T-shirt"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_MG5500,1);

static const canon_mode_t canon_PIXMA_MG5700_modes[] = {
  /* plain modes -- B/W also */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(13_C6M6Y4K3k4_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(13_C3M3Y2K3k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex"),INKSET(13_C6M6Y4K3k4on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(13_C2M2Y2K2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH (incl. Photo Hagaki)"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO (incl. Photo Hagaki)"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh2",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI inkjet Hagaki"),INKSET(13_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/(plain)Hagaki*/
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(13_C3M3Y2K3k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(13_C6M6Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG5700,1);

/* for this printer most photo modes have grey (H) ink. */
static const canon_mode_t canon_PIXMA_MG6100_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K2C6M6Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH (duplex)"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_PIXMA_MG6100,2);

/* for this printer most photo modes have grey (H) ink. */
static const canon_mode_t canon_PIXMA_MG6200_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K2C6M6Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex/Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_PIXMA_MG6200,1);

/* for this printer most photo modes have grey (H) ink. */
static const canon_mode_t canon_PIXMA_MG6300_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K3C6M6Y4k4off_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K3C3M3Y2k3off_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex/Env/Hagaki CMYKk and K */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex/Env/Hagaki"),INKSET(30_K3C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex/Env/Hagaki"),INKSET(30_K3C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* most photo modes CMYkH */
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(30_M8K8m4k4H8off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(30_M8K8m4k4H8off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* above also for PPgloss mono only CMYk */
  /* PPother */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO PPother"),INKSET(30_M6K6m4k4H6off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},/* untested */
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* T-Shirt CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_tshirt",N_("600x600 DPI T-SHIRT"),INKSET(30_M8K8m4k4H8off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
};
DECLARE_MODES(canon_PIXMA_MG6300,1);

/* for this printer most photo modes have grey (H) ink. */
/* borderless: plain (qlty 2 only), hagaki (qlty 3 and 2) use std mode with CMYk, those with photo modes unchanged. No env, HiRes, TST for borderless */
static const canon_mode_t canon_PIXMA_MG6500_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K3C6M6Y4k4off_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K3C3M3Y2k3off_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex/Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex/Env/Hagaki"),INKSET(30_K3C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex/Env/Hagaki"),INKSET(30_K3C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes/T-shirt"),INKSET(30_M8K8m4k4H8off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(30_M8K8m4k4H8off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo2",N_("600x600 DPI PHOTO other"),INKSET(30_M6K6m4k4H6off),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI inkjet Hagaki"),INKSET(30_C5M5Y4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_MG6500,1);

/* for this printer most photo modes have grey (H) ink. */
/* borderless: plain (qlty 2 only), hagaki (qlty 3 and 2) use std mode with CMYk, those with photo modes unchanged. No env, HiRes, TST for borderless */
static const canon_mode_t canon_PIXMA_MG7700_modes[] = {
  /* plain modes simplex */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K3C6M6Y4k4off4bit),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K3C3M3Y2k3off2bit),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* duplex CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex"),INKSET(30_K3C6M6Y4k4on4bit),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex"),INKSET(30_K3C3M3Y2k3on2bit),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* plain draft (also duplex) */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_draft",N_("600x600 DPI DRAFT"),INKSET(30_K2C2M2Y2),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes/T-shirt"),INKSET(30_M8K8m4k4H8off4bit),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(30_M6K6m4k4H6off4bit),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* CD CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh3",N_("600x600 DPI HIGH CD"),INKSET(30_C5M5Y5k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo3",N_("600x600 DPI CD"),INKSET(30_C5M5Y5k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_CD,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* inkjet Hagaki CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh4",N_("600x600 DPI HIGH inkjet Hagaki"),INKSET(30_C5M5Y5k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo4",N_("600x600 DPI Hagaki"),INKSET(30_C5M5Y5k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/(plain)Hagaki*/
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high3",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(30_K3C3M3Y2k3on2bit),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std3",N_("600x600 DPI Env/Hagaki"),INKSET(30_K3C3M3Y2k3on2bit),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_PIXMA_MG7700,1);

/* for this printer most photo modes have grey (H) ink. */
static const canon_mode_t canon_PIXMA_MG8100_modes[] = {
  /* plain modes --- duplex no mono CMYK */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K2C6M6Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Duplex/Env/Hagaki CMYKk */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_PIXMA_MG8100,1);

/* for this printer most photo modes have grey (H) ink. */
static const canon_mode_t canon_PIXMA_MG8200_modes[] = {
  /* plain modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(30_K2C6M6Y4k4),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(30_K2C3M3Y2k3_c),16,MODE_FLAG_EXTENDED_T|MODE_FLAG_NODUPLEX,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* duplex, Hagaki, Env modes use K and k */
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_high2",N_("600x600 DPI HIGH duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYKk,"600x600dpi_std2",N_("600x600 DPI duplex/Env/Hagaki"),INKSET(30_K2C3M3Y2k3on_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* draft */
  {  300, 300,CANON_INK_K|CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(30_K2C2M2Y2),8,MODE_FLAG_EXTENDED_T|MODE_FLAG_IP8500,NULL,1.0,1.0,NULL,NULL,NULL,1},
  /* photo modes CMYk */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH matte/HiRes"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO matte/HiRes"),INKSET(30_M8K8m4k4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
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
DECLARE_MODES(canon_PIXMA_MG8200,1);

/* MAXIFY models: iB4000, MB2000, MB2300, MB5000, MB5300 */
static const canon_mode_t canon_MAXIFY_iB4000_modes[] = {
  /* plain modes, including duplex */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high",N_("600x600 DPI HIGH"),INKSET(4_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(4_C3M3Y3K3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  {  600, 600,CANON_INK_K,"600x600dpi_mono",N_("600x600 DPI MONO"),INKSET(4_K2),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Photo modes, unusually using CMYK */
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photohigh",N_("600x600 DPI PHOTO HIGH"),INKSET(4_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_CMYK,"600x600dpi_photo",N_("600x600 DPI PHOTO"),INKSET(4_C4M4Y4K4),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
  /* Env/Hagaki modes */
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_high2",N_("600x600 DPI HIGH Env/Hagaki"),INKSET(4_C3M3Y3K3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,3},
  {  600, 600,CANON_INK_K|CANON_INK_CMYK,"600x600dpi_std2",N_("600x600 DPI Env/Hagaki"),INKSET(4_C3M3Y3K3_c),16,MODE_FLAG_EXTENDED_T,NULL,1.0,1.0,NULL,NULL,NULL,2},
};
DECLARE_MODES(canon_MAXIFY_iB4000,1);

#endif

