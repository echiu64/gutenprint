/*
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
 *   Copyright (c) 2006 Sascha Sommer (saschasommer@freenet.de)
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
  const unsigned int ink_types;         /* the used color channels */
  const char* name;                    /* internal name do not translate, must not contain spaces */
  const char* text;                    /* description */
  const int num_inks;
  const canon_inkset_t *inks;          /* ink definition        */
  const unsigned int flags;
#define MODE_FLAG_WEAVE 0x1            /* this mode requires weaving */
#define MODE_FLAG_EXTENDED_T 0x2       /* this mode requires extended color settings in the esc t) command */
  const double density;                /* density multiplier    */
  const double gamma;                  /* gamma multiplier      */
  const char *lum_adjustment;          /* optional lum adj.     */
  const char *hue_adjustment;          /* optional hue adj.     */
  const char *sat_adjustment;          /* optional sat adj.     */
} canon_mode_t;

#define INKSET(a) sizeof(a)/sizeof(canon_inkset_t),a


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
  {  180, 180,CANON_INK_K,"180x180dpi",N_("180x180 DPI"),INKSET(canon_K_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  360, 360,CANON_INK_K,"360x360dpi",N_("360x360 DPI"),INKSET(canon_K_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  720, 360,CANON_INK_K,"720x360dpi",N_("720x360 DPI"),INKSET(canon_K_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_30,0);


static const canon_mode_t canon_BJC_85_modes[] = {
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
              "360x360dpi",N_("360x360 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
              "360x360dmt",N_("360x360 DPI DMT"),INKSET(canon_CMYKcm_2bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  720, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
              "720x360dpi",N_("720x360 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_85,0);


/* we treat the printers that can either print in K or CMY as CMYK printers here by assigning a CMYK inkset */
static const canon_mode_t canon_BJC_210_modes[] = {
  {   90,  90,CANON_INK_K | CANON_INK_CMY,"90x90dpi",N_("90x90 DPI"),INKSET(canon_CMYK_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  180, 180,CANON_INK_K | CANON_INK_CMY,"180x180dpi",N_("180x180 DPI"),INKSET(canon_CMYK_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dpi",N_("360x360 DPI"),INKSET(canon_CMYK_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  720, 360,CANON_INK_K | CANON_INK_CMY,"720x360dpi",N_("720x360 DPI"),INKSET(canon_CMYK_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_210,0);


static const canon_mode_t canon_BJC_240_modes[] = {
  {   90,  90,CANON_INK_K | CANON_INK_CMY,"90x90dpi",N_("90x90 DPI"),INKSET(canon_CMYK_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  180, 180,CANON_INK_K | CANON_INK_CMY,"180x180dpi",N_("180x180 DPI"),INKSET(canon_CMYK_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dpi",N_("360x360 DPI"),INKSET(canon_CMYK_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  360, 360,CANON_INK_K | CANON_INK_CMY,"360x360dmt",N_("360x360 DMT"),INKSET(canon_CMYK_2bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  720, 360,CANON_INK_K | CANON_INK_CMY,"720x360dpi",N_("720x360 DPI"),INKSET(canon_CMYK_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_240,0);


static const canon_mode_t canon_BJC_2000_modes[] = {
  {  180, 180,CANON_INK_CMYK,"180x180dpi",N_("180x180 DPI"),INKSET(canon_CMYK_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  360, 360,CANON_INK_CMYK,"360x360dpi",N_("360x360 DPI"),INKSET(canon_CMYK_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_2000,0);


static const canon_mode_t canon_BJC_3000_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dmt",N_("360x360 DPI DMT"),INKSET(canon_CMYKcm_2bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"720x720dpi",N_("720x720 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"1440x720dpi",N_("1440x720 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_3000,0);


static const canon_mode_t canon_BJC_4300_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dmt",N_("360x360 DPI DMT"),INKSET(canon_CMYKcm_2bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"720x720dpi",N_("720x720 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"1440x720dpi",N_("1440x720 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_4300,0);



static const canon_mode_t canon_BJC_4400_modes[] = {
  {  360, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
              "360x360dpi",N_("360x360 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  720, 360,CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
              "720x360dpi",N_("720x360 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_4400,0);


static const canon_mode_t canon_BJC_5500_modes[] = {
  {  180, 180,CANON_INK_CMYK | CANON_INK_CcMmYK,"180x180dpi",N_("180x180 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_5500,0);


static const canon_mode_t canon_BJC_6000_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dpi",N_("360x360 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.8,1.0,NULL,NULL,NULL},
  {  360, 360,CANON_INK_CMYK | CANON_INK_CcMmYK,"360x360dmt",N_("360x360 DPI DMT"),INKSET(canon_CMYKcm_2bit_inkset),0,1.8,1.0,NULL,NULL,NULL},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"720x720dpi",N_("720x720 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CcMmYK,"1440x720dpi",N_("1440x720 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,0.5,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_6000,0);


static const canon_mode_t canon_BJC_7000_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi",N_("300x300 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,3.5,1.0,NULL,NULL,NULL},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi",N_("600x600 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.8,1.0,NULL,NULL,NULL},
  { 1200, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"1200x600dpi",N_("1200x600 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_7000,0);


static const canon_mode_t canon_BJC_7100_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYyK,"300x300dpi",N_("300x300 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"600x600dpi",N_("600x600 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  { 1200, 600,CANON_INK_CMYK | CANON_INK_CcMmYyK,"1200x600dpi",N_("1200x600 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_7100,0);

static const canon_mode_t canon_BJC_8200_modes[] = {
  {  300, 300,CANON_INK_CMYK,"300x300dpi",N_("300x300 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  { 1200,1200,CANON_INK_CMYK,"1200x1200dpi",N_("1200x1200 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_8200,0);


static const canon_mode_t canon_BJC_8500_modes[] = {
  {  300, 300,CANON_INK_CMYK | CANON_INK_CcMmYK,"300x300dpi",N_("300x300 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
  {  600, 600,CANON_INK_CMYK | CANON_INK_CcMmYK,"600x600dpi",N_("600x600 DPI"),INKSET(canon_CMYKcm_1bit_inkset),0,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_BJC_8500,0);


static const canon_mode_t canon_S200_modes[] = {
  {  360, 360,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
              "360x360dpi",N_("360x360 DPI"),INKSET(canon_CMYK_1bit_inkset),0,2.0,1.0,NULL,NULL,NULL},
  {  720, 720,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
              "720x720dpi",N_("720x720 DPI"),INKSET(canon_CMYK_1bit_inkset),MODE_FLAG_WEAVE,1.0,1.0,NULL,NULL,NULL},
  { 1440, 720,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
              "1440x720dpi",N_("1440x720 DPI"),INKSET(canon_CMYK_1bit_inkset),MODE_FLAG_WEAVE,0.5,1.0,NULL,NULL,NULL},
  { 1440,1440,CANON_INK_CMYK | CANON_INK_CMY | CANON_INK_K,
              "1440x1440dpi",N_("1440x1440 DPI"),INKSET(canon_CMYK_1bit_inkset),MODE_FLAG_WEAVE,0.3,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_S200,0);


static const canon_mode_t canon_PIXMA_iP4000_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(canon_PIXMA_iP4000_default_inkset),MODE_FLAG_EXTENDED_T,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_PIXMA_iP4000,0);


static const canon_mode_t canon_PIXMA_iP4200_modes[] = {
  {  600, 600,CANON_INK_CMYK,"600x600dpi",N_("600x600 DPI"),INKSET(canon_PIXMA_iP4200_default_inkset),MODE_FLAG_EXTENDED_T,1.0,1.0,NULL,NULL,NULL},
};
DECLARE_MODES(canon_PIXMA_iP4200,0);

#endif

