/*
 * "$Id$"
 *
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
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
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

/*
 * Large parts of this file (mainly the ink handling) is based on
 * print-escp2.c -- refer to README.new-printer on how to adjust the colors
 * for a certain model.
 */

/* TODO-LIST
 *
 *   * adjust the colors of all supported models
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <string.h>
#include <stdio.h>
#if defined(HAVE_VARARGS_H) && !defined(HAVE_STDARG_H)
#include <varargs.h>
#else
#include <stdarg.h>
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include "module.h"
#include "printers.h"

#if (0)
#define EXPERIMENTAL_STUFF 0
#endif

#define MAX_CARRIAGE_WIDTH	13 /* This really needs to go away */

/*
 * We really need to get away from this silly static nonsense...
 */
#define MAX_PHYSICAL_BPI 1440
#define MAX_OVERSAMPLED 8
#define MAX_BPP 4
#define COMPBUFWIDTH (MAX_PHYSICAL_BPI * MAX_OVERSAMPLED * MAX_BPP * \
	MAX_CARRIAGE_WIDTH / CHAR_BIT)

#define USE_3BIT_FOLD_TYPE 323

/*
 * For each printer, we can select from a variety of dot sizes.
 * For single dot size printers, the available sizes are usually 0,
 * which is the "default", and some subset of 1-4.  For simple variable
 * dot size printers (with only one kind of variable dot size), the
 * variable dot size is specified as 0x10.  For newer printers, there
 * is a choice of variable dot sizes available, 0x10, 0x11, and 0x12 in
 * order of increasing size.
 *
 * Normally, we want to specify the smallest dot size that lets us achieve
 * a density of less than .8 or thereabouts (above that we start to get
 * some dither artifacts).  This needs to be tested for each printer and
 * resolution.
 *
 * An entry of -1 in a slot means that this resolution is not available.
 *              0                      standard dot sizes are used.
 *              1                      drop modulation is used.
 */

/* We know a per-model base resolution (like 180dpi or 150dpi)
 * and multipliers for the base resolution in the dotsize-, densities-
 * and inklist:
 * for 180dpi base resolution we would have
 *   s_r11_4 for 4color ink @180dpi
 *   s_r22_4 for 4color ink @360dpi
 *   s_r33_4 for 4color ink @720dpi
 *   s_r43_4 for 4color ink @1440x720dpi
 */

typedef struct canon_dot_sizes
{
  int dot_r11;    /*  180x180  or   150x150  */
  int dot_r22;    /*  360x360  or   300x300  */
  int dot_r33;    /*  720x720  or   600x600  */
  int dot_r43;    /* 1440x720  or  1200x600  */
  int dot_r44;    /* 1440x1440 or  1200x1200 */
  int dot_r55;    /* 2880x2880 or  2400x2400 */
} canon_dot_size_t;

/*
 * Specify the base density for each available resolution.
 *
 */

typedef struct canon_densities
{
  double d_r11;  /*  180x180  or   150x150  */
  double d_r22;  /*  360x360  or   300x300  */
  double d_r33;  /*  720x720  or   600x600  */
  double d_r43;  /* 1440x720  or  1200x600  */
  double d_r44;  /* 1440x1440 or  1200x1200 */
  double d_r55;  /* 2880x2880 or  2400x2400 */
} canon_densities_t;



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
 *
 * Stuff like the MIS 4 and 6 "color" monochrome inks doesn't fit into
 * this model very nicely, so we'll either have to special case it
 * or find some way of handling it in here.
 */

typedef struct canon_variable_ink
{
  const stp_dither_range_simple_t *range;
  int count;
  double density;
} canon_variable_ink_t;

typedef struct canon_variable_inkset
{
  const canon_variable_ink_t *c;
  const canon_variable_ink_t *m;
  const canon_variable_ink_t *y;
  const canon_variable_ink_t *k;
} canon_variable_inkset_t;

/*
 * currenty unaccounted for are the 7color printers and the 3color ones
 * (which use CMY only printheads)
 *
 */

typedef struct canon_variable_inklist
{
  const int bits;
  const int colors;
  const canon_variable_inkset_t *r11;    /*  180x180  or   150x150  */
  const canon_variable_inkset_t *r22;    /*  360x360  or   300x300  */
  const canon_variable_inkset_t *r33;    /*  720x720  or   600x600  */
  const canon_variable_inkset_t *r43;    /* 1440x720  or  1200x600  */
  const canon_variable_inkset_t *r44;    /* 1440x1440 or  1200x1200 */
  const canon_variable_inkset_t *r55;    /* 2880x2880 or  2400x2400 */
} canon_variable_inklist_t;


#ifdef EXPERIMENTAL_STUFF
/*
 * A printmode is defined by its resolution (xdpi x ydpi), the bits per pixel
 * and the installed printhead.
 *
 * For a hereby defined printmode we specify the density and gamma multipliers
 * and the ink definition with optional adjustments for lum, hue and sat
 *
 */
typedef struct canon_variable_printmode
{
  const int xdpi;                      /* horizontal resolution */
  const int ydpi;                      /* vertical resolution   */
  const int bits;                      /* bits per pixel        */
  const int printhead;                 /* installed printhead   */
  const int quality;                   /* maximum init-quality  */
  const double density;                /* density multiplier    */
  const double gamma;                  /* gamma multiplier      */
  const canon_variable_inkset_t *inks; /* ink definition        */
  const char *lum_adjustment;          /* optional lum adj.     */
  const char *hue_adjustment;          /* optional hue adj.     */
  const char *sat_adjustment;          /* optional sat adj.     */
} canon_variable_printmode_t;
#endif

/* NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE
 *
 * The following dither ranges were taken from print-escp2.c and do NOT
 * represent the requirements of canon inks. Feel free to play with them
 * accoring to the escp2 part of doc/README.new-printer and send me a patch
 * if you get better results. Please send mail to thaller@ph.tum.de
 */

/*
 * Dither ranges specifically for Cyan/LightCyan (see NOTE above)
 *
 */
static const stp_dither_range_simple_t canon_dither_ranges_Cc_1bit[] =
{
  { 0.25, 0x1, 1, 1 },
  { 1.0,  0x1, 0, 1 }
};

static const canon_variable_ink_t canon_ink_Cc_1bit =
{
  canon_dither_ranges_Cc_1bit,
  sizeof(canon_dither_ranges_Cc_1bit) / sizeof(stp_dither_range_simple_t),
  .75
};

/*
 * Dither ranges specifically for Magenta/LightMagenta (see NOTE above)
 *
 */
static const stp_dither_range_simple_t canon_dither_ranges_Mm_1bit[] =
{
  { 0.26, 0x1, 1, 1 },
  { 1.0,  0x1, 0, 1 }
};

static const canon_variable_ink_t canon_ink_Mm_1bit =
{
  canon_dither_ranges_Mm_1bit,
  sizeof(canon_dither_ranges_Mm_1bit) / sizeof(stp_dither_range_simple_t),
  .75
};


/*
 * Dither ranges specifically for any Color and 2bit/pixel (see NOTE above)
 *
 */
static const stp_dither_range_simple_t canon_dither_ranges_X_2bit[] =
{
  { 0.45,  0x1, 0, 1 },
  { 0.68,  0x2, 0, 2 },
  { 1.0,   0x3, 0, 3 }
};

static const canon_variable_ink_t canon_ink_X_2bit =
{
  canon_dither_ranges_X_2bit,
  sizeof(canon_dither_ranges_X_2bit) / sizeof(stp_dither_range_simple_t),
  1.0
};

/*
 * Dither ranges specifically for any Color/LightColor and 2bit/pixel
 * (see NOTE above)
 */
static const stp_dither_range_simple_t canon_dither_ranges_Xx_2bit[] =
{
  { 0.15,  0x1, 1, 1 },
  { 0.227, 0x2, 1, 2 },
/*{ 0.333, 0x3, 1, 3 }, */
  { 0.45,  0x1, 0, 1 },
  { 0.68,  0x2, 0, 2 },
  { 1.0,   0x3, 0, 3 }
};

static const canon_variable_ink_t canon_ink_Xx_2bit =
{
  canon_dither_ranges_Xx_2bit,
  sizeof(canon_dither_ranges_Xx_2bit) / sizeof(stp_dither_range_simple_t),
  1.0
};

/*
 * Dither ranges specifically for any Color and 3bit/pixel
 * (see NOTE above)
 *
 * BIG NOTE: The bjc8200 has this kind of ink. One Byte seems to hold
 *           drop sizes for 3 pixels in a 3/2/2 bit fashion.
 *           Size values for 3bit-sized pixels range from 1 to 7,
 *           size values for 2bit-sized picels from 1 to 3 (kill msb).
 *
 *
 */
static const stp_dither_range_simple_t canon_dither_ranges_X_3bit[] =
{
  { 0.45,  0x1, 0, 1 },
  { 0.55,  0x2, 0, 2 },
  { 0.66,  0x3, 0, 3 },
  { 0.77,  0x4, 0, 4 },
  { 0.88,  0x5, 0, 5 },
  { 1.0,   0x6, 0, 6 }
};

static const canon_variable_ink_t canon_ink_X_3bit =
{
  canon_dither_ranges_X_3bit,
  sizeof(canon_dither_ranges_X_3bit) / sizeof(stp_dither_range_simple_t),
  1.0
};

/*
 * Dither ranges specifically for any Color/LightColor and 3bit/pixel
 * (see NOTE above)
 */
static const stp_dither_range_simple_t canon_dither_ranges_Xx_3bit[] =
{
  { 0.15,  0x1, 1, 1 },
  { 0.227, 0x2, 1, 2 },
  { 0.333, 0x3, 1, 3 },
/*  { 0.333, 0x3, 1, 3 }, */
  { 0.45,  0x1, 0, 1 },
  { 0.55,  0x2, 0, 2 },
  { 0.66,  0x3, 0, 3 },
  { 0.77,  0x4, 0, 4 },
  { 0.88,  0x5, 0, 5 },
  { 1.0,   0x6, 0, 6 }
};

static const canon_variable_ink_t canon_ink_Xx_3bit =
{
  canon_dither_ranges_Xx_3bit,
  sizeof(canon_dither_ranges_Xx_3bit) / sizeof(stp_dither_range_simple_t),
  1.0
};


/* Inkset for printing in CMY and 1bit/pixel */
static const canon_variable_inkset_t ci_CMY_1 =
{
  NULL,
  NULL,
  NULL,
  NULL
};

/* Inkset for printing in CMY and 2bit/pixel */
static const canon_variable_inkset_t ci_CMY_2 =
{
  &canon_ink_X_2bit,
  &canon_ink_X_2bit,
  &canon_ink_X_2bit,
  NULL
};

/* Inkset for printing in CMYK and 1bit/pixel */
static const canon_variable_inkset_t ci_CMYK_1 =
{
  NULL,
  NULL,
  NULL,
  NULL
};

/* Inkset for printing in CcMmYK and 1bit/pixel */
static const canon_variable_inkset_t ci_CcMmYK_1 =
{
  &canon_ink_Cc_1bit,
  &canon_ink_Mm_1bit,
  NULL,
  NULL
};

/* Inkset for printing in CMYK and 2bit/pixel */
static const canon_variable_inkset_t ci_CMYK_2 =
{
  &canon_ink_X_2bit,
  &canon_ink_X_2bit,
  &canon_ink_X_2bit,
  &canon_ink_X_2bit
};

/* Inkset for printing in CcMmYK and 2bit/pixel */
static const canon_variable_inkset_t ci_CcMmYK_2 =
{
  &canon_ink_Xx_2bit,
  &canon_ink_Xx_2bit,
  &canon_ink_X_2bit,
  &canon_ink_X_2bit
};

/* Inkset for printing in CMYK and 3bit/pixel */
static const canon_variable_inkset_t ci_CMYK_3 =
{
  &canon_ink_X_3bit,
  &canon_ink_X_3bit,
  &canon_ink_X_3bit,
  &canon_ink_X_3bit
};

/* Inkset for printing in CcMmYK and 3bit/pixel */
static const canon_variable_inkset_t ci_CcMmYK_3 =
{
  &canon_ink_Xx_3bit,
  &canon_ink_Xx_3bit,
  &canon_ink_X_3bit,
  &canon_ink_X_3bit,
};


typedef canon_variable_inklist_t* canon_variable_inklist_p;

/* Ink set should be applicable for any CMYK based model */
static const canon_variable_inklist_t canon_ink_standard[] =
{
  {
    1,4,
    &ci_CMYK_1, &ci_CMYK_1, &ci_CMYK_1,
    &ci_CMYK_1, &ci_CMYK_1, &ci_CMYK_1,
  },
};

/* Ink set for printers using CMY and CMY photo printing, 1 or 2bit/pixel */
static const canon_variable_inklist_t canon_ink_oldphoto[] =
{
  {
    1,3,
    &ci_CMY_1, &ci_CMY_1, &ci_CMY_1,
    &ci_CMY_1, &ci_CMY_1, &ci_CMY_1,
  },
  {
    2,3,
    &ci_CMY_2, &ci_CMY_2,
    &ci_CMY_2, &ci_CMY_2,
    &ci_CMY_2, &ci_CMY_2,
  },
};

/* Ink set for printers using CMYK and CcMmYK printing, 1 or 2bit/pixel */
static const canon_variable_inklist_t canon_ink_standardphoto[] =
{
  {
    1,4,
    &ci_CMYK_1, &ci_CMYK_1, &ci_CMYK_1,
    &ci_CMYK_1, &ci_CMYK_1, &ci_CMYK_1,
  },
  {
    2,4,
    &ci_CMYK_2, &ci_CMYK_2,
    &ci_CMYK_2, &ci_CMYK_2,
    &ci_CMYK_2, &ci_CMYK_2,
  },
  {
    1,6,
    &ci_CcMmYK_1, &ci_CcMmYK_1, &ci_CcMmYK_1,
    &ci_CcMmYK_1, &ci_CcMmYK_1, &ci_CcMmYK_1,
  },
  {
    2,6,
    &ci_CcMmYK_2, &ci_CcMmYK_2, &ci_CcMmYK_2,
    &ci_CcMmYK_2, &ci_CcMmYK_2, &ci_CcMmYK_2,
  },
};

/* Ink set for printers using CMYK and CcMmYK printing, 1 or 3bit/pixel */
static const canon_variable_inklist_t canon_ink_superphoto[] =
{
  {
    1,4,
    &ci_CMYK_1, &ci_CMYK_1, &ci_CMYK_1,
    &ci_CMYK_1, &ci_CMYK_1, &ci_CMYK_1,
  },
  {
    3,4,
    &ci_CMYK_3, &ci_CMYK_3, &ci_CMYK_3,
    &ci_CMYK_3, &ci_CMYK_3, &ci_CMYK_3,
  },
  {
    3,6,
    &ci_CcMmYK_3, &ci_CcMmYK_3, &ci_CcMmYK_3,
    &ci_CcMmYK_3, &ci_CcMmYK_3, &ci_CcMmYK_3,
  },
};


static const char standard_sat_adjustment[] =
"STP_CURVE;Wrap ;Linear ; 48;0;0.0;4.0:"
"1.00;1.10;1.20;1.30;1.40;1.50;1.60;1.70;"  /* C */
"1.80;1.90;1.90;1.90;1.70;1.50;1.30;1.10;"  /* B */
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"  /* M */
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"  /* R */
"1.00;1.00;1.00;1.10;1.20;1.30;1.40;1.50;"  /* Y */
"1.50;1.40;1.30;1.20;1.10;1.00;1.00;1.00;"; /* G */

static const char standard_lum_adjustment[] =
"STP_CURVE;Wrap ;Linear ; 48;0;0.0;4.0:"
"0.50;0.60;0.70;0.80;0.90;0.86;0.82;0.79;"  /* C */
"0.78;0.80;0.83;0.87;0.90;0.95;1.05;1.15;"  /* B */
"1.30;1.25;1.20;1.15;1.12;1.09;1.06;1.03;"  /* M */
"1.00;1.00;1.00;1.00;1.00;1.00;1.00;1.00;"  /* R */
"1.00;0.90;0.80;0.70;0.65;0.60;0.55;0.52;"  /* Y */
"0.48;0.47;0.47;0.49;0.49;0.49;0.52;0.51;"; /* G */

static const char standard_hue_adjustment[] =
"STP_CURVE;Wrap ;Linear ; 48;0;-6.0;6.0:"
"0.00;0.05;0.04;0.01;-.03;-.10;-.18;-.26;"  /* C */
"-.35;-.43;-.40;-.32;-.25;-.18;-.10;-.07;"  /* B */
"0.00;-.04;-.09;-.13;-.18;-.23;-.27;-.31;"  /* M */
"-.35;-.38;-.30;-.23;-.15;-.08;0.00;-.02;"  /* R */
"0.00;0.08;0.10;0.08;0.05;0.03;-.03;-.12;"  /* Y */
"-.20;0.17;-.20;-.17;-.15;-.12;-.10;-.08;"; /* G */

typedef enum {
  COLOR_MONOCHROME = 1,
  COLOR_CMY = 3,
  COLOR_CMYK = 4,
  COLOR_CCMMYK= 6,
  COLOR_CCMMYYK= 7
} colormode_t;

typedef struct canon_caps {
  int model;          /* model number as used in printers.xml */
  int model_id;       /* model ID code for use in commands */
  int max_width;      /* maximum printable paper size */
  int max_height;
  int base_res;       /* base resolution - shall be 150 or 180 */
  int max_xdpi;       /* maximum horizontal resolution */
  int max_ydpi;       /* maximum vertical resolution */
  int max_quality;
  int border_left;    /* left margin, points */
  int border_right;   /* right margin, points */
  int border_top;     /* absolute top margin, points */
  int border_bottom;  /* absolute bottom margin, points */
  int inks;           /* installable cartridges (CANON_INK_*) */
  int slots;          /* available paperslots */
  unsigned long features;       /* special bjl settings */
#ifdef EXPERIMENTAL_STUFF
  const canon_variable_printmode_t *printmodes;
  int printmodes_cnt;
#else
  int dummy;
  const canon_dot_size_t dot_sizes;   /* Vector of dot sizes for resolutions */
  const canon_densities_t densities;   /* List of densities for each printer */
  const canon_variable_inklist_t *inxs; /* Choices of inks for this printer */
  int inxs_cnt;                         /* number of ink definitions in inxs */
#endif
  const char *lum_adjustment;
  const char *hue_adjustment;
  const char *sat_adjustment;
} canon_cap_t;

static void canon_write_line(const stp_vars_t, const canon_cap_t *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     int, int, int, int *,int);


/* Codes for possible ink-tank combinations.
 * Each combo is represented by the colors that can be used with
 * the installed ink-tank(s)
 * Combinations of the codes represent the combinations allowed for a model
 * Note that only preferrable combinations should be used
 */
#define CANON_INK_K           1
#define CANON_INK_CMY         2
#define CANON_INK_CMYK        4
#define CANON_INK_CcMmYK      8
#define CANON_INK_CcMmYyK    16

#define CANON_INK_BLACK_MASK (CANON_INK_K|CANON_INK_CMYK|CANON_INK_CcMmYK)

#define CANON_INK_PHOTO_MASK (CANON_INK_CcMmYK|CANON_INK_CcMmYyK)

/* document feeding */
#define CANON_SLOT_ASF1    1
#define CANON_SLOT_ASF2    2
#define CANON_SLOT_MAN1    4
#define CANON_SLOT_MAN2    8

/* model peculiarities */
#define CANON_CAP_DMT       0x01ul    /* Drop Modulation Technology */
#define CANON_CAP_MSB_FIRST 0x02ul    /* how to send data           */
#define CANON_CAP_a         0x04ul
#define CANON_CAP_b         0x08ul
#define CANON_CAP_q         0x10ul
#define CANON_CAP_m         0x20ul
#define CANON_CAP_d         0x40ul
#define CANON_CAP_t         0x80ul
#define CANON_CAP_c         0x100ul
#define CANON_CAP_p         0x200ul
#define CANON_CAP_l         0x400ul
#define CANON_CAP_r         0x800ul
#define CANON_CAP_g         0x1000ul
#define CANON_CAP_ACKSHORT  0x2000ul

#define CANON_CAP_STD0 (CANON_CAP_b|CANON_CAP_c|CANON_CAP_d|\
                        CANON_CAP_l|CANON_CAP_q|CANON_CAP_t)

#define CANON_CAP_STD1 (CANON_CAP_b|CANON_CAP_c|CANON_CAP_d|CANON_CAP_l|\
                        CANON_CAP_m|CANON_CAP_p|CANON_CAP_q|CANON_CAP_t)

#ifdef EXPERIMENTAL_STUFF
#define CANON_MODES(A) A,sizeof(A)/sizeof(canon_variable_printmode_t*)
#else
#define CANON_MODES(A) 0
#endif

#define CANON_INK(A) A,sizeof(A)/sizeof(canon_variable_inklist_t*)


#ifdef EXPERIMENTAL_STUFF

#define BC_10   CANON_INK_K       /* b/w   */
#define BC_11   CANON_INK_CMYK    /* color */
#define BC_12   CANON_INK_CMYK    /* photo */
#define BC_20   CANON_INK_K       /* b/w   */
#define BC_21   CANON_INK_CMYK    /* color */
#define BC_22   CANON_INK_CMYK    /* photo */
#define BC_29   0                 /* neon! */
#define BC_3031 CANON_INK_CMYK    /* color */
#define BC_3231 CANON_INK_CcMmYK  /* photo */


static const canon_variable_printmode_t canon_nomodes[] =
{
  {0,0,0,0,0,0,0,0,0,0}
};

static const canon_variable_printmode_t canon_modes_30[] = {
  {  180, 180, 1, BC_10, 2,  1.0, 1.0, &ci_CMYK_1,   0,0,0 },
  {  360, 360, 1, BC_10, 2,  1.0, 1.0, &ci_CMYK_1,   0,0,0 },
  {  720, 360, 1, BC_10, 2,  1.0, 1.0, &ci_CMYK_1,   0,0,0 },
};

static const canon_variable_printmode_t canon_modes_85[] = {
  {  360, 360, 1, BC_10, 2,  1.0, 1.0, &ci_CMYK_1,   0,0,0 },
  {  360, 360, 1, BC_11, 2,  1.0, 1.0, &ci_CMYK_1,   0,0,0 },
  {  360, 360, 2, BC_11, 2,  1.0, 1.0, &ci_CMYK_2,   0,0,0 },
  {  360, 360, 1, BC_21, 2,  1.0, 1.0, &ci_CMYK_1,   0,0,0 },
  {  360, 360, 2, BC_21, 2,  1.0, 1.0, &ci_CMYK_2,   0,0,0 },
};

static const canon_variable_printmode_t canon_modes_2x00[] = {
  {  360, 360, 1, BC_20, 2,  1.0, 1.0, &ci_CMYK_1,   0,0,0 },
  {  360, 360, 1, BC_21, 2,  1.0, 1.0, &ci_CMYK_1,   0,0,0 },
  {  360, 360, 1, BC_22, 2,  1.0, 1.0, &ci_CMYK_1,   0,0,0 },
};

static const canon_variable_printmode_t canon_modes_6x00[] = {
  {  360, 360, 1, BC_3031, 2,  1.8, 1.0, &ci_CMYK_1,   0,0,0 },
  {  360, 360, 2, BC_3031, 2,  1.8, 1.0, &ci_CMYK_2,   0,0,0 },
  {  720, 720, 1, BC_3031, 2,  1.0, 1.0, &ci_CMYK_1,   0,0,0 },
  { 1440, 720, 1, BC_3031, 2,  0.5, 1.0, &ci_CMYK_1,   0,0,0 },
  {  360, 360, 1, BC_3231, 2,  1.8, 1.0, &ci_CcMmYK_1, 0,0,0 },
  {  360, 360, 2, BC_3231, 2,  1.8, 1.0, &ci_CcMmYK_2, 0,0,0 },
  {  720, 720, 1, BC_3231, 2,  1.0, 1.0, &ci_CcMmYK_1, 0,0,0 },
  { 1440, 720, 1, BC_3231, 2,  0.5, 1.0, &ci_CcMmYK_1, 0,0,0 },
};
#endif

static const canon_cap_t canon_model_capabilities[] =
{
  /* default settings for unknown models */

  {   -1, 17*72/2,842,180,180,20,20,20,20, CANON_INK_K, CANON_SLOT_ASF1, 0 },

  /* ******************************** */
  /*                                  */
  /* tested and color-adjusted models */
  /*                                  */
  /* ******************************** */




  /* ************************************ */
  /*                                      */
  /* tested models w/out color-adjustment */
  /*                                      */
  /* ************************************ */


  { /* Canon  BJ 30   *//* heads: BC-10 */
    30, 1,
    9.5*72, 14*72,
    90, 360, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a,
    CANON_MODES(canon_modes_30),
#ifndef EXPERIMENTAL_STUFF
    {-1,0,0,0,-1,-1}, /*090x090 180x180 360x360 720x360 720x720 1440x1440*/
    {1,1,1,1,1,1},    /*------- 180x180 360x360 720x360 ------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon  BJC 85  *//* heads: BC-20 BC-21 BC-22 */
    85, 1,
    9.5*72, 14*72,
    90, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a | CANON_CAP_DMT,
    CANON_MODES(canon_modes_85),
#ifndef EXPERIMENTAL_STUFF
    {-1,-1,1,0,-1,-1},/*090x090 180x180 360x360 720x360 720x720 1440x1440*/
    {1,1,1,1,1,1},    /*------- ------- 360x360 720x360 ------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 4300 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    4300, 1,
    618, 936,      /* 8.58" x 13 " */
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD0 | CANON_CAP_DMT,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {-1,1,0,0,-1,-1}, /*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*------- 360x360 720x720 1440x720 --------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 4400 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    4400, 1,
    9.5*72, 14*72,
    90, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K | CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a | CANON_CAP_DMT,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {-1,-1,0,0,-1,-1},/*090x090 180x180 360x360 720x360 720x720 1440x1440*/
    {1,1,1,1,1,1},    /*------- ------- 360x360 720x360 ------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 6000 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    6000, 3,
    618, 936,      /* 8.58" x 13 " */
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD1 | CANON_CAP_DMT | CANON_CAP_ACKSHORT,
    CANON_MODES(canon_modes_6x00),
#ifndef EXPERIMENTAL_STUFF
    {-1,1,0,0,-1,-1}, /*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1.8,1,0.5,1,1},/*------- 360x360 720x720 1440x720 --------- ---------*/
    CANON_INK(canon_ink_standardphoto),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 6200 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    6200, 3,
    618, 936,      /* 8.58" x 13 " */
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD1 | CANON_CAP_DMT | CANON_CAP_ACKSHORT,
    CANON_MODES(canon_modes_6x00),
#ifndef EXPERIMENTAL_STUFF
    {-1,1,0,0,-1,-1}, /*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {0,1.8,1,.5,0,0}, /*------- 360x360 720x720 1440x720 --------- ---------*/
    CANON_INK(canon_ink_standardphoto),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 6500 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    6500, 3,
    842, 17*72,
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD1 | CANON_CAP_DMT,
    CANON_MODES(canon_modes_6x00),
#ifndef EXPERIMENTAL_STUFF
    {-1,1,0,0,-1,-1}, /*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {0,1.8,1,.5,0,0}, /*------- 360x360 720x720 1440x720 --------- ---------*/
    CANON_INK(canon_ink_standardphoto),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 8200 *//* heads: BC-50 */
    8200, 3,
    842, 17*72,
    150, 1200,1200, 4,
    11, 9, 10, 18,
    CANON_INK_CMYK, /*  | CANON_INK_CcMmYK */
    CANON_SLOT_ASF1,
    CANON_CAP_STD1 | CANON_CAP_r | CANON_CAP_DMT | CANON_CAP_ACKSHORT,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {-1,0,0,-1,0,-1}, /*150x150 300x300 600x600 1200x600 1200x1200 2400x2400*/
    {1,1,1,1,1,1},    /*------- 300x300 600x600 -------- 1200x1200 ---------*/
    CANON_INK(canon_ink_superphoto),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },


  /* *************** */
  /*                 */
  /* untested models */
  /*                 */
  /* *************** */


  { /* Canon BJC 210 *//* heads: BC-02 BC-05 BC-06 */
    210, 1,
    618, 936,      /* 8.58" x 13 " */
    90, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K | CANON_INK_CMY,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD0,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {0,0,0,0,-1,-1},/*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*180x180 360x360 ------- -------- --------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 240 *//* heads: BC-02 BC-05 BC-06 */
    240, 1,
    618, 936,      /* 8.58" x 13 " */
    90, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K | CANON_INK_CMY,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD0 | CANON_CAP_DMT,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {0,0,1,0,-1,-1},/*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*180x180 360x360 ------- -------- --------- ---------*/
    CANON_INK(canon_ink_oldphoto),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 250 *//* heads: BC-02 BC-05 BC-06 */
    250, 1,
    618, 936,      /* 8.58" x 13 " */
    90, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K | CANON_INK_CMY,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD0 | CANON_CAP_DMT,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {0,0,1,0,-1,-1},/*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*180x180 360x360 ------- -------- --------- ---------*/
    CANON_INK(canon_ink_oldphoto),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 1000 *//* heads: BC-02 BC-05 BC-06 */
    1000, 1,
    842, 17*72,
    90, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_K | CANON_INK_CMY,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_DMT | CANON_CAP_a,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {0,0,1,0,-1,-1},  /*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*180x180 360x360 ------- -------- --------- ---------*/
    CANON_INK(canon_ink_oldphoto),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 2000 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    2000, 1,
    842, 17*72,
    180, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {0,0,-1,-1,-1,-1},/*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*180x180 360x360 ------- -------- --------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 3000 *//* heads: BC-30 BC-33 BC-34 */
    3000, 3,
    842, 17*72,
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a | CANON_CAP_DMT, /*FIX? should have _r? */
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {-1,1,0,0,-1,-1}, /*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*------- 360x360 720x720 1440x720 --------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 6100 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    6100, 3,
    842, 17*72,
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD1 | CANON_CAP_a | CANON_CAP_r | CANON_CAP_DMT,
    CANON_MODES(canon_modes_6x00),
#ifndef EXPERIMENTAL_STUFF
    {-1,1,0,0,-1,-1}, /*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*------- 360x360 720x720 1440x720 --------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 7000 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    7000, 3,
    842, 17*72,
    150, 1200, 600, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYyK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD1,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {-1,0,0,0,-1,-1}, /*150x150 300x300 600x600 1200x600 1200x1200 2400x2400*/
    {1,3.5,1.8,1,1,1},/*------- 300x300 600x600 1200x600 --------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 7100 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    7100, 3,
    842, 17*72,
    150, 1200, 600, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYyK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {-1,0,0,0,-1,-1}, /*150x150 300x300 600x600 1200x600 1200x1200 2400x2400*/
    {1,1,1,1,1,1},    /*------- 300x300 600x600 1200x600 --------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  /*****************************/
  /*                           */
  /*  extremely fuzzy models   */
  /* (might never work at all) */
  /*                           */
  /*****************************/

  { /* Canon BJC 5100 *//* heads: BC-20 BC-21 BC-22 BC-23 BC-29 */
    5100, 1,
    17*72, 22*72,
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_DMT,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {-1,1,0,0,-1,-1}, /*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*------- 360x360 720x720 1440x720 --------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 5500 *//* heads: BC-20 BC-21 BC-29 */
    5500, 1,
    22*72, 34*72,
    180, 720, 360, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {0,0,-1,-1,-1,-1},/*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*180x180 360x360 ------- -------- --------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 6500 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    6500, 3,
    17*72, 22*72,
    180, 1440, 720, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD1 | CANON_CAP_a | CANON_CAP_DMT,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {-1,1,0,0,-1,-1}, /*180x180 360x360 720x720 1440x720 1440x1440 2880x2880*/
    {1,1,1,1,1,1},    /*------- 360x360 720x720 1440x720 --------- ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 8500 *//* heads: BC-80/BC-81 BC-82/BC-81 */
    8500, 3,
    17*72, 22*72,
    150, 1200,1200, 2,
    11, 9, 10, 18,
    CANON_INK_CMYK | CANON_INK_CcMmYK,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0,
    CANON_MODES(canon_nomodes),
#ifndef EXPERIMENTAL_STUFF
    {-1,0,0,-1,0,-1}, /*150x150 300x300 600x600 1200x600 1200x1200 2400x2400*/
    {1,1,1,1,1,1},    /*------- 300x300 600x600 -------- 1200x1200 ---------*/
    CANON_INK(canon_ink_standard),
#endif
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
};

typedef struct {
  int x;
  int y;
  const char *name;
  const char *text;
  const char *name_dmt;
  const char *text_dmt;
} canon_res_t;

static const canon_res_t canon_resolutions[] = {
  { 90, 90, "90x90dpi", N_("90x90 DPI"), "90x90dmt", N_("90x90 DPI DMT") },
  { 180, 180, "180x180dpi", N_("180x180 DPI"), "180x180dmt", N_("180x180 DPI DMT") },
  { 360, 360, "360x360dpi", N_("360x360 DPI"), "360x360dmt", N_("360x360 DPI DMT") },
  { 720, 360, "720x360dpi", N_("720x360 DPI"), "720x360dmt", N_("720x360 DPI DMT") },
  { 720, 720, "720x720dpi", N_("720x720 DPI"), "720x720dmt", N_("720x720 DPI DMT") },
  { 1440, 720, "1440x720dpi", N_("1440x720 DPI"), "1440x720dmt", N_("1440x720 DPI DMT") },
  { 1440, 1440, "1440x1440dpi", N_("1440x1440 DPI"), "1440x1440dmt", N_("1440x1440 DPI DMT") },
  { 2880, 2880, "2880x2880dpi", N_("2880x2880 DPI"), "2880x2880dmt", N_("2880x2880 DPI DMT") },
  { 150, 150, "150x150dpi", N_("150x150 DPI"), "150x150dmt", N_("150x150 DPI DMT") },
  { 300, 300, "300x300dpi", N_("300x300 DPI"), "300x300dmt", N_("300x300 DPI DMT") },
  { 600, 300, "600x300dpi", N_("600x300 DPI"), "600x300dmt", N_("600x300 DPI DMT") },
  { 600, 600, "600x600dpi", N_("600x600 DPI"), "600x600dmt", N_("600x600 DPI DMT") },
  { 1200, 600, "1200x600dpi", N_("1200x600 DPI"), "1200x600dmt", N_("1200x600 DPI DMT") },
  { 1200, 1200, "1200x1200dpi", N_("1200x1200 DPI"), "1200x1200dmt", N_("1200x1200 DPI DMT") },
  { 2400, 2400, "2400x2400dpi", N_("2400x2400 DPI"), "2400x2400dmt", N_("2400x2400 DPI DMT") },
  { 0, 0, NULL, NULL, NULL, NULL }
};

static const char *plain_paper_lum_adjustment =
"STP_CURVE;Wrap ;Linear ; 48;0;0.0;4.0:"
"1.20;1.22;1.28;1.34;1.39;1.42;1.45;1.48;"  /* C */
"1.50;1.40;1.30;1.25;1.20;1.10;1.05;1.05;"  /* B */
"1.05;1.05;1.05;1.05;1.05;1.05;1.05;1.05;"  /* M */
"1.05;1.05;1.05;1.10;1.10;1.10;1.10;1.10;"  /* R */
"1.10;1.15;1.30;1.45;1.60;1.75;1.90;2.00;"  /* Y */
"2.10;2.00;1.80;1.70;1.60;1.50;1.40;1.30;"; /* G */

typedef struct {
  const char *name;
  const char *text;
  int media_code;
  double base_density;
  double k_lower_scale;
  double k_upper;
  const char *hue_adjustment;
  const char *lum_adjustment;
  const char *sat_adjustment;
} paper_t;

typedef struct {
  const canon_cap_t *caps;
  int output_type;
  const paper_t *pt;
  int print_head;
  int colormode;
  const char *source_str;
  int xdpi;
  int ydpi;
  int page_width;
  int page_height;
  int top;
  int left;
  int bits;
} canon_init_t;

static const paper_t canon_paper_list[] = {
  { "Plain",		N_ ("Plain Paper"),                0x00, 0.50, 0.25, 0.500, 0, 0, 0 },
  { "Transparency",	N_ ("Transparencies"),             0x02, 1.00, 1.00, 0.900, 0, 0, 0 },
  { "BackPrint",	N_ ("Back Print Film"),            0x03, 1.00, 1.00, 0.900, 0, 0, 0 },
  { "Fabric",		N_ ("Fabric Sheets"),              0x04, 0.50, 0.25, 0.500, 0, 0, 0 },
  { "Envelope",		N_ ("Envelope"),                   0x08, 0.50, 0.25, 0.500, 0, 0, 0 },
  { "Coated",		N_ ("High Resolution Paper"),      0x07, 0.78, 0.25, 0.500, 0, 0, 0 },
  { "TShirt",		N_ ("T-Shirt Transfers"),          0x03, 0.50, 0.25, 0.500, 0, 0, 0 },
  { "GlossyFilm",	N_ ("High Gloss Film"),            0x06, 1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPaper",	N_ ("Glossy Photo Paper"),         0x05, 1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyCard",	N_ ("Glossy Photo Cards"),         0x0a, 1.00, 1.00, 0.999, 0, 0, 0 },
  { "GlossyPro",	N_ ("Photo Paper Pro"),            0x09, 1.00, 1.00, 0.999, 0, 0, 0 },
  { "Other",		N_ ("Other"),                      0x00, 0.50, 0.25, .5, 0, 0, 0 },
};

static const int paper_type_count = sizeof(canon_paper_list) / sizeof(paper_t);

static const stp_parameter_t the_parameters[] =
{
  {
    "PageSize", N_("Page Size"),
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_PAGE_SIZE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1
  },
  {
    "MediaType", N_("Media Type"),
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1
  },
  {
    "InputSlot", N_("Media Source"),
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1
  },
  {
    "InkType", N_("Ink Type"),
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1
  },
  {
    "Resolution", N_("Resolution"),
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1
  },
};

static int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

static const paper_t *
get_media_type(const char *name)
{
  int i;
  if (name)
    for (i = 0; i < paper_type_count; i++)
      {
	/* translate paper_t.name */
	if (!strcmp(name, canon_paper_list[i].name))
	  return &(canon_paper_list[i]);
      }
  return NULL;
}


static const canon_cap_t * canon_get_model_capabilities(int model)
{
  int i;
  int models= sizeof(canon_model_capabilities) / sizeof(canon_cap_t);
  for (i=0; i<models; i++) {
    if (canon_model_capabilities[i].model == model) {
      return &(canon_model_capabilities[i]);
    }
  }
  stp_deprintf(STP_DBG_CANON,"canon: model %d not found in capabilities list.\n",model);
  return &(canon_model_capabilities[0]);
}

static int
canon_source_type(const char *name, const canon_cap_t * caps)
{
  /* used internally: do not translate */
  if (name)
    {
      if (!strcmp(name,"Auto"))    return 4;
      if (!strcmp(name,"Manual"))    return 0;
      if (!strcmp(name,"ManualNP")) return 1;
    }

  stp_deprintf(STP_DBG_CANON,"canon: Unknown source type '%s' - reverting to auto\n",name);
  return 4;
}

static int
canon_printhead_type(const char *name, const canon_cap_t * caps)
{
  /* used internally: do not translate */
  if (name)
    {
      if (!strcmp(name,"Gray"))             return 0;
      if (!strcmp(name,"RGB"))             return 1;
      if (!strcmp(name,"CMYK"))       return 2;
      if (!strcmp(name,"PhotoCMY"))       return 3;
      if (!strcmp(name,"Photo"))             return 4;
      if (!strcmp(name,"PhotoCMYK")) return 5;
    }
  if (name && *name == 0) {
    if (caps->inks & CANON_INK_CMYK) return 2;
    if (caps->inks & CANON_INK_CMY)  return 1;
    if (caps->inks & CANON_INK_K)    return 0;
  }

  stp_deprintf(STP_DBG_CANON,"canon: Unknown head combo '%s' - reverting to black",name);
  return 0;
}

static colormode_t
canon_printhead_colors(const char *name, const canon_cap_t * caps)
{
  /* used internally: do not translate */
  if (name)
    {
      if (!strcmp(name,"Gray"))             return COLOR_MONOCHROME;
      if (!strcmp(name,"RGB"))             return COLOR_CMY;
      if (!strcmp(name,"CMYK"))       return COLOR_CMYK;
      if (!strcmp(name,"PhotoCMY"))       return COLOR_CCMMYK;
      if (!strcmp(name,"PhotoCMYK")) return COLOR_CCMMYYK;
    }

  if (name && *name == 0) {
    if (caps->inks & CANON_INK_CMYK) return COLOR_CMYK;
    if (caps->inks & CANON_INK_CMY)  return COLOR_CMY;
    if (caps->inks & CANON_INK_K)    return COLOR_MONOCHROME;
  }

  stp_deprintf(STP_DBG_CANON,"canon: Unknown head combo '%s' - reverting to black",name);
  return COLOR_MONOCHROME;
}

static unsigned char
canon_size_type(const stp_vars_t v, const canon_cap_t * caps)
{
  const stp_papersize_t pp = stp_get_papersize_by_size(stp_get_page_height(v),
						       stp_get_page_width(v));
  if (pp)
    {
      const char *name = stp_papersize_get_name(pp);
      /* used internally: do not translate */
      /* built ins: */
      if (!strcmp(name,"A5"))          return 0x01;
      if (!strcmp(name,"A4"))          return 0x03;
      if (!strcmp(name,"B5"))          return 0x08;
      if (!strcmp(name,"Letter"))      return 0x0d;
      if (!strcmp(name,"Legal"))       return 0x0f;
      if (!strcmp(name,"COM10")) return 0x16;
      if (!strcmp(name,"DL")) return 0x17;
      if (!strcmp(name,"LetterExtra"))     return 0x2a;
      if (!strcmp(name,"A4Extra"))         return 0x2b;
      if (!strcmp(name,"w288h144"))   return 0x2d;
      /* custom */

      stp_deprintf(STP_DBG_CANON,"canon: Unknown paper size '%s' - using custom\n",name);
    } else {
      stp_deprintf(STP_DBG_CANON,"canon: Couldn't look up paper size %dx%d - "
	      "using custom\n",stp_get_page_height(v), stp_get_page_width(v));
    }
  return 0;
}

#ifndef EXPERIMENTAL_STUFF
static int canon_res_code(const canon_cap_t * caps, int xdpi, int ydpi)
{
  int x, y, res= 0;

  for (x=1; x<6; x++) if ((xdpi/caps->base_res) == (1<<(x-1))) res= (x<<4);
  for (y=1; y<6; y++) if ((ydpi/caps->base_res) == (1<<(y-1))) res|= y;

  return res;
}
#else
static const canon_variable_printmode_t *canon_printmode(const canon_cap_t * caps,
							 int xdpi, int ydpi,
							 int bpp, int head)
{
  const canon_variable_printmode_t *modes;
  int modes_cnt;
  int i;
  if (!caps) return 0;
  modes= caps->printmodes;
  modes_cnt= caps->printmodes_cnt;
  /* search for the right printmode: */
  for (i=0; i<modes_cnt; i++) {
    if ((modes[i].xdpi== xdpi) && (modes[i].ydpi== ydpi) &&
	(modes[i].bits== bpp) && (modes[i].printhead== head))
      {
	return &(modes[i]);
      }
  }
  /* nothing found -> either return 0 or apply some policy to
   * get a fallback printmode
   */
  if (modes[0].xdpi) return modes;
  return 0;
}
#endif

static int
canon_ink_type(const canon_cap_t * caps, int res_code)
{
#ifndef EXPERIMENTAL_STUFF
  switch (res_code)
    {
    case 0x11: return caps->dot_sizes.dot_r11;
    case 0x22: return caps->dot_sizes.dot_r22;
    case 0x33: return caps->dot_sizes.dot_r33;
    case 0x43: return caps->dot_sizes.dot_r43;
    case 0x44: return caps->dot_sizes.dot_r44;
    case 0x55: return caps->dot_sizes.dot_r55;
    }
  return -1;
#else
  return -1;
#endif
}

static const char *
canon_lum_adjustment(int model)
{
  const canon_cap_t * caps= canon_get_model_capabilities(model);
  return (caps->lum_adjustment);
}

static const char *
canon_hue_adjustment(int model)
{
  const canon_cap_t * caps= canon_get_model_capabilities(model);
  return (caps->hue_adjustment);
}

static const char *
canon_sat_adjustment(int model)
{
  const canon_cap_t * caps= canon_get_model_capabilities(model);
  return (caps->sat_adjustment);
}

static double
canon_density(const canon_cap_t * caps, int res_code)
{
#ifndef EXPERIMENTAL_STUFF
  switch (res_code)
    {
    case 0x11: return caps->densities.d_r11;
    case 0x22: return caps->densities.d_r22;
    case 0x33: return caps->densities.d_r33;
    case 0x43: return caps->densities.d_r43;
    case 0x44: return caps->densities.d_r44;
    case 0x55: return caps->densities.d_r55;
    default:
      stp_deprintf(STP_DBG_CANON,"no such res_code 0x%x in density of model %d\n",
	      res_code,caps->model);
      return 0.2;
    }
#else
  return 0.2;
#endif
}

static const canon_variable_inkset_t *
canon_inks(const canon_cap_t * caps, int res_code, int colors, int bits)
{
#ifndef EXPERIMENTAL_STUFF
  const canon_variable_inklist_t *inks = caps->inxs;
  int i;

  if (!inks)
    return NULL;

  for (i=0; i<caps->inxs_cnt; i++) {
      stp_deprintf(STP_DBG_CANON,"hmm, trying ink for resolution code "
	      "%x, %d bits, %d colors\n",res_code,inks[i].bits,inks[i].colors);
    if ((inks[i].bits==bits) && (inks[i].colors==colors)) {
      stp_deprintf(STP_DBG_CANON,"wow, found ink for resolution code "
	      "%x, %d bits, %d colors\n",res_code,bits,colors);
      switch (res_code)
	{
	case 0x11: return inks[i].r11;
	case 0x22: return inks[i].r22;
	case 0x33: return inks[i].r33;
	case 0x43: return inks[i].r43;
	case 0x44: return inks[i].r44;
	case 0x55: return inks[i].r55;
	}
    }
  }
  stp_deprintf(STP_DBG_CANON,"ooo, found no ink for resolution code "
	  "%x, %d bits, %d colors in all %d defs!\n",
	  res_code,bits,colors,caps->inxs_cnt);
  return NULL;
#else
  return NULL;
#endif
}

static void
canon_describe_resolution(const stp_vars_t v, int *x, int *y)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const canon_res_t *res = canon_resolutions;
  while (res->x > 0)
    {
      if (strcmp(resolution, res->name) == 0 ||
	  strcmp(resolution, res->name_dmt) == 0)
	{
	  *x = res->x;
	  *y = res->y;
	  return;
	}
      res++;
    }
  *x = -1;
  *y = -1;
}

static stp_param_string_t media_sources[] =
              {
                { "Auto",	N_ ("Auto Sheet Feeder") },
                { "Manual",	N_ ("Manual with Pause") },
                { "ManualNP",	N_ ("Manual without Pause") }
              };


/*
 * 'canon_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
canon_list_parameters(const stp_vars_t v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  return ret;
}

static void
canon_parameters(const stp_vars_t v, const char *name,
		 stp_parameter_t *description)
{
  int		i;

  const canon_cap_t * caps=
    canon_get_model_capabilities(stp_get_model(v));
  description->p_type = STP_PARAMETER_TYPE_INVALID;

  if (name == NULL)
    return;

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }
  if (strcmp(name, "PageSize") == 0)
  {
    int height_limit, width_limit;
    int papersizes = stp_known_papersizes();
    description->bounds.str = stp_string_list_allocate();

    width_limit = caps->max_width;
    height_limit = caps->max_height;

    for (i = 0; i < papersizes; i++) {
      const stp_papersize_t pt = stp_get_papersize_by_index(i);
      if (strlen(stp_papersize_get_name(pt)) > 0 &&
	  stp_papersize_get_width(pt) <= width_limit &&
	  stp_papersize_get_height(pt) <= height_limit)
	{
	  if (stp_string_list_count(description->bounds.str) == 0)
	    description->deflt.str = stp_papersize_get_name(pt);
	  stp_string_list_add_param(description->bounds.str,
				   stp_papersize_get_name(pt),
				   stp_papersize_get_text(pt));
	}
    }
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    int x,y;
    int t;
    description->bounds.str= stp_string_list_allocate();
    description->deflt.str = NULL;

    for (x=1; x<6; x++) {
      for (y=x-1; y<x+1; y++) {
	if ((t= canon_ink_type(caps,(x<<4)|y)) > -1) {
	  int xx = (1<<x)/2*caps->base_res;
	  int yy = (1<<y)/2*caps->base_res;
	  const canon_res_t *res = canon_resolutions;
	  while (res->x > 0) {
	    if (xx == res->x && yy == res->y) {
	      stp_string_list_add_param(description->bounds.str,
					res->name, _(res->text));
	      stp_deprintf(STP_DBG_CANON,"supports mode '%s'\n",
			   res->name);
	      if (xx >= 300 && yy >= 300 && description->deflt.str == NULL)
		description->deflt.str = res->name;
	      if (t == 1) {
		stp_string_list_add_param(description->bounds.str,
					  res->name_dmt, _(res->text_dmt));
		stp_deprintf(STP_DBG_CANON,"supports mode '%s'\n",
			     res->name_dmt);
	      }
	      break;
	    }
	    res++;
	  }
	}
      }
    }
  }
  else if (strcmp(name, "InkType") == 0)
  {
    description->bounds.str= stp_string_list_allocate();
    /* used internally: do not translate */
    if ((caps->inks & CANON_INK_K))
      stp_string_list_add_param(description->bounds.str,
			       "Gray", _("Black"));
    if ((caps->inks & CANON_INK_CMY))
      stp_string_list_add_param(description->bounds.str,
			       "RGB", _("CMY Color"));
    if ((caps->inks & CANON_INK_CMYK))
      stp_string_list_add_param(description->bounds.str,
			       "CMYK", _("CMYK Color"));
    if ((caps->inks & CANON_INK_CcMmYK))
      stp_string_list_add_param(description->bounds.str,
			       "PhotoCMY", _("Photo CcMmY Color"));
    if ((caps->inks & CANON_INK_CcMmYyK))
      stp_string_list_add_param(description->bounds.str,
			       "PhotoCMYK", _("Photo CcMmYK Color"));
    description->deflt.str =
      stp_string_list_param(description->bounds.str, 0)->name;
  }
  else if (strcmp(name, "MediaType") == 0)
  {
    int count = sizeof(canon_paper_list) / sizeof(canon_paper_list[0]);
    description->bounds.str= stp_string_list_allocate();
    description->deflt.str= canon_paper_list[0].name;

    for (i = 0; i < count; i ++)
      stp_string_list_add_param(description->bounds.str,
				canon_paper_list[i].name,
				_(canon_paper_list[i].text));
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    int count = 3;
    description->bounds.str= stp_string_list_allocate();
    description->deflt.str= media_sources[0].name;
    for (i = 0; i < count; i ++)
      stp_string_list_add_param(description->bounds.str,
				media_sources[i].name,
				_(media_sources[i].text));
  }
}


/*
 * 'canon_imageable_area()' - Return the imageable area of the page.
 */

static void
canon_imageable_area(const stp_vars_t v,   /* I */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */

  const canon_cap_t * caps= canon_get_model_capabilities(stp_get_model(v));

  stp_default_media_size(v, &width, &length);

  *left   = caps->border_left;
  *right  = width - caps->border_right;
  *top    = caps->border_top;
  *bottom = length - caps->border_bottom;
}

static void
canon_limit(const stp_vars_t v,  		/* I */
	    int *width,
	    int *height,
	    int *min_width,
	    int *min_height)
{
  const canon_cap_t * caps=
    canon_get_model_capabilities(stp_get_model(v));
  *width =	caps->max_width;
  *height =	caps->max_height;
  *min_width = 1;
  *min_height = 1;
}

/*
 * 'canon_cmd()' - Sends a command with variable args
 */
static void
canon_cmd(const stp_vars_t v, /* I - the printer         */
	  const char *ini, /* I - 2 bytes start code  */
	  const char cmd,  /* I - command code        */
	  int  num,  /* I - number of arguments */
	  ...        /* I - the args themselves */
	  )
{
  unsigned char *buffer = stp_zalloc(num + 1);
  int i;
  va_list ap;

  if (num)
    {
      va_start(ap, num);
      for (i=0; i<num; i++)
	buffer[i]= (unsigned char) va_arg(ap, int);
      va_end(ap);
    }

  stp_zfwrite(ini,2,1,v);
  if (cmd)
    {
      stp_putc(cmd,v);
      stp_put16_le(num, v);
      if (num)
	stp_zfwrite((const char *)buffer,num,1,v);
    }
  stp_free(buffer);
}

#define PUT(WHAT,VAL,RES) stp_deprintf(STP_DBG_CANON,"canon: "WHAT\
" is %04x =% 5d = %f\" = %f mm\n",(VAL),(VAL),(VAL)/(1.*RES),(VAL)/(RES/25.4))

#define ESC28 "\033\050"
#define ESC5b "\033\133"
#define ESC40 "\033\100"

#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#define MAX(a,b) (((a)>(b)) ? (a) : (b))

/* ESC [K --  -- reset printer:
 */
static void
canon_init_resetPrinter(const stp_vars_t v, canon_init_t *init)
{
  unsigned long f=init->caps->features;
  if (f & (CANON_CAP_ACKSHORT))
    {
      canon_cmd(v,ESC5b,0x4b, 2, 0x00,0x1f);
      stp_puts("BJLSTART\nControlMode=Common\n",v);
      if (f & CANON_CAP_ACKSHORT) stp_puts("AckTime=Short\n",v);
      stp_puts("BJLEND\n",v);
    }
  canon_cmd(v,ESC5b,0x4b, 2, 0x00,0x0f);
}

/* ESC (a -- 0x61 -- cmdSetPageMode --:
 */
static void
canon_init_setPageMode(const stp_vars_t v, canon_init_t *init)
{
  if (!(init->caps->features & CANON_CAP_a))
    return;

  if (init->caps->features & CANON_CAP_a)
    canon_cmd(v,ESC28,0x61, 1, 0x01);
}

/* ESC (b -- 0x62 -- -- set data compression:
 */
static void
canon_init_setDataCompression(const stp_vars_t v, canon_init_t *init)
{
  if (!(init->caps->features & CANON_CAP_b))
    return;

  canon_cmd(v,ESC28,0x62, 1, 0x01);
}

/* ESC (c -- 0x63 -- cmdSetColor --:
 */
static void
canon_init_setColor(const stp_vars_t v, canon_init_t *init)
{
  unsigned char
    arg_63_1, arg_63_2, arg_63_3;


  if (!(init->caps->features & CANON_CAP_c))
    return;

  arg_63_1 = init->caps->model_id << 4;						/* MODEL_ID */

  switch ( init->caps->model_id ) {

  	case 0:			/* very old 360 dpi series: BJC-800/820 */
		break;		/*	tbd */

  	case 1:			/* 360 dpi series - BJC-4000, BJC-210, BJC-70 and their descendants */
		if (init->output_type==OUTPUT_GRAY)
    			arg_63_1|= 0x01;					/* PRINT_COLOUR */

  		arg_63_2 = ((init->pt ? init->pt->media_code : 0) << 4)		/* PRINT_MEDIA */
			+ 1;	/* hardcode to High quality for now */		/* PRINT_QUALITY */

  		canon_cmd(v,ESC28,0x63, 2, arg_63_1, arg_63_2);
		break;

	case 2:			/* are any models using this? */
		break;

	case 3:			/* 720 dpi series - BJC-3000 and descendants */
		if (init->output_type==OUTPUT_GRAY)
    			arg_63_1|= 0x01;					/* colour mode */

  		arg_63_2 = (init->pt) ? init->pt->media_code : 0;		/* print media type */

		arg_63_3 = 2;	/* hardcode to whatever this means for now */	/* quality, apparently */

  		canon_cmd(v,ESC28,0x63, 3, arg_63_1, arg_63_2, arg_63_3);
		break;
  	}

  return;
}

/* ESC (d -- 0x64 -- -- set raster resolution:
 */
static void
canon_init_setResolution(const stp_vars_t v, canon_init_t *init)
{
  if (!(init->caps->features & CANON_CAP_d))
    return;

  canon_cmd(v,ESC28,0x64, 4,
	    (init->ydpi >> 8 ), (init->ydpi & 255),
	    (init->xdpi >> 8 ), (init->xdpi & 255));
}

/* ESC (g -- 0x67 -- cmdSetPageMargins --:
 */
static void
canon_init_setPageMargins(const stp_vars_t v, canon_init_t *init)
{
  /* TOFIX: what exactly is to be sent?
   * Is it the printable length or the bottom border?
   * Is is the printable width or the right border?
   */

  int minlength= 0;
  int minwidth= 0;
  int length= init->page_height*5/36;
  int width= init->page_width*5/36;

  if (!(init->caps->features & CANON_CAP_g))
    return;

  if (minlength>length) length= minlength;
  if (minwidth>width) width= minwidth;

  canon_cmd(v,ESC28,0x67, 4, 0,
	    (unsigned char)(length),1,
	    (unsigned char)(width));

}

/* ESC (l -- 0x6c -- cmdSetTray --:
 */
static void
canon_init_setTray(const stp_vars_t v, canon_init_t *init)
{
  unsigned char
    arg_6c_1 = 0x00,
    arg_6c_2 = 0x00; /* plain paper */

  /* int media= canon_media_type(media_str,caps); */
  int source= canon_source_type(init->source_str,init->caps);

  if (!(init->caps->features & CANON_CAP_l))
    return;

  arg_6c_1 = init->caps->model_id << 4;

  arg_6c_1|= (source & 0x0f);

  if (init->pt) arg_6c_2= init->pt->media_code;

  canon_cmd(v,ESC28,0x6c, 2, arg_6c_1, arg_6c_2);
}

/* ESC (m -- 0x6d --  -- :
 */
static void
canon_init_setPrintMode(const stp_vars_t v, canon_init_t *init)
{
  unsigned char
    arg_6d_1 = 0x03, /* color printhead? */
    arg_6d_2 = 0x00, /* 00=color  02=b/w */
    arg_6d_3 = 0x00, /* only 01 for bjc8200 */
    arg_6d_a = 0x03, /* A4 paper */
    arg_6d_b = 0x00;

  if (!(init->caps->features & CANON_CAP_m))
    return;

  arg_6d_a= canon_size_type(v,init->caps);
  if (!arg_6d_a)
    arg_6d_b= 1;

  if (init->print_head==0)
    arg_6d_1= 0x03;
  else if (init->print_head<=2)
    arg_6d_1= 0x02;
  else if (init->print_head<=4)
    arg_6d_1= 0x04;
  if (init->output_type==OUTPUT_GRAY)
    arg_6d_2= 0x02;

  if (init->caps->model==8200)
    arg_6d_3= 0x01;

  canon_cmd(v,ESC28,0x6d,12, arg_6d_1,
	    0xff,0xff,0x00,0x00,0x07,0x00,
	    arg_6d_a,arg_6d_b,arg_6d_2,0x00,arg_6d_3);
}

/* ESC (p -- 0x70 -- cmdSetPageMargins2 --:
 */
static void
canon_init_setPageMargins2(const stp_vars_t v, canon_init_t *init)
{
  /* TOFIX: what exactly is to be sent?
   * Is it the printable length or the bottom border?
   * Is is the printable width or the right border?
   */

  int printable_width=  init->page_width*5/6;
  int printable_length= init->page_height*5/6;

  unsigned char arg_70_1= (printable_length >> 8) & 0xff;
  unsigned char arg_70_2= (printable_length) & 0xff;
  unsigned char arg_70_3= (printable_width >> 8) & 0xff;
  unsigned char arg_70_4= (printable_width) & 0xff;

  if (!(init->caps->features & CANON_CAP_p))
    return;

  canon_cmd(v,ESC28,0x70, 8,
	    arg_70_1, arg_70_2, 0x00, 0x00,
	    arg_70_3, arg_70_4, 0x00, 0x00);
}

/* ESC (q -- 0x71 -- setPageID -- :
 */
static void
canon_init_setPageID(const stp_vars_t v, canon_init_t *init)
{
  if (!(init->caps->features & CANON_CAP_q))
    return;

  canon_cmd(v,ESC28,0x71, 1, 0x01);
}

/* ESC (r -- 0x72 --  -- :
 */
static void
canon_init_setX72(const stp_vars_t v, canon_init_t *init)
{
  if (!(init->caps->features & CANON_CAP_r))
    return;

  canon_cmd(v,ESC28,0x72, 1, 0x61); /* whatever for - 8200 needs it */
}

/* ESC (t -- 0x74 -- cmdSetImage --:
 */
static void
canon_init_setImage(const stp_vars_t v, canon_init_t *init)
{
  unsigned char
    arg_74_1 = 0x01, /* 1 bit per pixel */
    arg_74_2 = 0x00, /*  */
    arg_74_3 = 0x01; /* 01 <= 360 dpi    09 >= 720 dpi */

  if (!(init->caps->features & CANON_CAP_t))
    return;

  if (init->xdpi==1440) arg_74_2= 0x04;
  if (init->ydpi>=720)  arg_74_3= 0x09;

  if (init->bits>1) {
    arg_74_1= 0x02;
    arg_74_2= 0x80;
    arg_74_3= 0x09;
    if (init->colormode == COLOR_CMY) arg_74_3= 0x02; /* for BC-06 cartridge!!! */
  }

  /* workaround for the bjc8200 in 6color mode - not really understood */
  if (init->caps->model==8200) {
    if (init->colormode==COLOR_CCMMYK) {
      arg_74_1= 0xff;
      arg_74_2= 0x90;
      arg_74_3= 0x04;
      init->bits=3;
      if (init->ydpi>600)  arg_74_3= 0x09;
    } else {
      arg_74_1= 0x01;
      arg_74_2= 0x00;
      arg_74_3= 0x01;
      if (init->ydpi>600)  arg_74_3= 0x09;
    }
  }

  canon_cmd(v,ESC28,0x74, 3, arg_74_1, arg_74_2, arg_74_3);
}

static void
canon_init_printer(const stp_vars_t v, canon_init_t *init)
{
  int mytop;
  /* init printer */

  canon_init_resetPrinter(v,init);       /* ESC [K */
  canon_init_setPageMode(v,init);        /* ESC (a */
  canon_init_setDataCompression(v,init); /* ESC (b */
  canon_init_setPageID(v,init);          /* ESC (q */
  canon_init_setPrintMode(v,init);       /* ESC (m */
  canon_init_setResolution(v,init);      /* ESC (d */
  canon_init_setImage(v,init);           /* ESC (t */
  canon_init_setColor(v,init);           /* ESC (c */
  canon_init_setPageMargins(v,init);     /* ESC (g */
  canon_init_setPageMargins2(v,init);    /* ESC (p */
  canon_init_setTray(v,init);            /* ESC (l */
  canon_init_setX72(v,init);             /* ESC (r */

  /* some linefeeds */

  mytop= (init->top*init->ydpi)/72;
  canon_cmd(v,ESC28,0x65, 2, (mytop >> 8 ),(mytop & 255));
}

static void
canon_deinit_printer(const stp_vars_t v, canon_init_t *init)
{
  /* eject page */
  stp_putc(0x0c,v);

  /* say goodbye */
  canon_cmd(v,ESC28,0x62,1,0);
  if (init->caps->features & CANON_CAP_a)
    canon_cmd(v,ESC28,0x61, 1, 0);
  canon_cmd(v,ESC40,0,0);
}

/*
 * 'advance_buffer()' - Move (num) lines of length (len) down one line
 *                      and sets first line to 0s
 *                      accepts NULL pointers as buf
 *                  !!! buf must contain more than (num) lines !!!
 *                      also sets first line to 0s if num<1
 */
static void
canon_advance_buffer(unsigned char *buf, int len, int num)
{
  if (!buf || !len) return;
  if (num>0) memmove(buf+len,buf,len*num);
  memset(buf,0,len);
}

/*
 * 'canon_print()' - Print an image to a CANON printer.
 */
static int
canon_print(const stp_vars_t v, stp_image_t *image)
{
  int		status = 1;
  int		model = stp_get_model(v);
  const char	*resolution = stp_get_string_parameter(v, "Resolution");
  const char	*media_source = stp_get_string_parameter(v, "InputSlot");
  int 		output_type = stp_get_output_type(v);
  const char	*ink_type = stp_get_string_parameter(v, "InkType");
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  int		n;		/* Output number */
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned char *black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*yellow,	/* Yellow bitmap data */
		*lcyan,		/* Light cyan bitmap data */
		*lmagenta,	/* Light magenta bitmap data */
		*lyellow;	/* Light yellow bitmap data */
  int           delay_k,
                delay_c,
                delay_m,
                delay_y,
                delay_lc,
                delay_lm,
                delay_ly,
                delay_max;
  int		page_width,	/* Width of page */
		page_height,	/* Length of page */
		page_left,
		page_top,
		page_right,
		page_bottom,
		page_true_height,	/* True length of page */
		out_width,	/* Width of image on page */
		out_height,	/* Length of image on page */
		out_channels,	/* Output bytes per pixel */
		length,		/* Length of raster data */
                buf_length,     /* Length of raster data buffer (dmt) */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  int		zero_mask;
  int           bits= 1;
  int           image_height,
                image_width,
                image_bpp;
  int           res_code;
  int           use_6color= 0;
  double        k_upper, k_lower;
  int           emptylines= 0;
  stp_vars_t	nv = stp_allocate_copy(v);
  stp_curve_t lum_adjustment = NULL;
  stp_curve_t hue_adjustment = NULL;
  stp_curve_t sat_adjustment = NULL;

  canon_init_t  init;
  const canon_cap_t * caps= canon_get_model_capabilities(model);
  int printhead= canon_printhead_type(ink_type,caps);
  colormode_t colormode = canon_printhead_colors(ink_type,caps);
  const paper_t *pt;
  const canon_variable_inkset_t *inks;
  const canon_res_t *res = canon_resolutions;

  if (!stp_verify(nv))
    {
      stp_eprintf(nv, "Print options not verified; cannot print.\n");
      return 0;
    }

  PUT("top        ",top,72);
  PUT("left       ",left,72);

  /*
  * Setup a read-only pixel region for the entire image...
  */

  stp_image_init(image);
  image_bpp = stp_image_bpp(image);

  /* force grayscale if image is grayscale
   *                 or single black cartridge installed
   */

  if (printhead == 0 || caps->inks == CANON_INK_K)
    {
      output_type = OUTPUT_GRAY;
      stp_set_output_type(nv, output_type);
    }

  if (output_type == OUTPUT_GRAY)
    colormode = COLOR_MONOCHROME;
  stp_set_output_color_model(nv, COLOR_MODEL_CMY);

 /*
  * Figure out the output resolution...
  */

  xdpi = -1;
  ydpi = -1;
  while (res->x > 0) {
    if (strcmp(resolution, res->name) == 0 ||
	strcmp(resolution, res->name_dmt) == 0)
      {
	xdpi = res->x;
	ydpi = res->y;
	break;
      }
    res++;
  }

  stp_deprintf(STP_DBG_CANON,"canon: resolution=%dx%d\n",xdpi,ydpi);
  stp_deprintf(STP_DBG_CANON,"       rescode   =0x%x\n",canon_res_code(caps,xdpi,ydpi));
  res_code= canon_res_code(caps,xdpi,ydpi);

  if (strcmp(resolution, res->name_dmt) == 0 &&
      (caps->features & CANON_CAP_DMT)) {
    bits= 2;
    stp_deprintf(STP_DBG_CANON,"canon: using drop modulation technology\n");
  }

 /*
  * Compute the output size...
  */

  out_width = stp_get_width(v);
  out_height = stp_get_height(v);

  canon_imageable_area(nv, &page_left, &page_right, &page_bottom, &page_top);
  left -= page_left;
  top -= page_top;
  page_width = page_right - page_left;
  page_height = page_bottom - page_top;

  image_height = stp_image_height(image);
  image_width = stp_image_width(image);

  stp_default_media_size(nv, &n, &page_true_height);

  PUT("top        ",top,72);
  PUT("left       ",left,72);
  PUT("page_true_height",page_true_height,72);
  PUT("out_width ", out_width,xdpi);
  PUT("out_height", out_height,ydpi);

  stp_image_progress_init(image);

  PUT("top     ",top,72);
  PUT("left    ",left,72);

  pt = get_media_type(stp_get_string_parameter(nv, "MediaType"));

  init.caps = caps;
  init.output_type = output_type;
  init.pt = pt;
  init.print_head = printhead;
  init.colormode = colormode;
  init.source_str = media_source;
  init.xdpi = xdpi;
  init.ydpi = ydpi;
  init.page_width = page_width;
  init.page_height = page_height;
  init.top = top;
  init.left = left;
  init.bits = bits;

  canon_init_printer(nv, &init);

  /* possibly changed during initialitation
   * to enforce valid modes of operation:
   */
  bits= init.bits;
  xdpi= init.xdpi;
  ydpi= init.ydpi;

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  PUT("out_width ", out_width,xdpi);
  PUT("out_height", out_height,ydpi);

  left = xdpi * left / 72;

  PUT("leftskip",left,xdpi);

  if(xdpi==1440){
    delay_k= 0;
    delay_c= 112;
    delay_m= 224;
    delay_y= 336;
    delay_lc= 112;
    delay_lm= 224;
    delay_ly= 336;
    delay_max= 336;
    stp_deprintf(STP_DBG_CANON,"canon: delay on!\n");
  } else {
    delay_k= delay_c= delay_m= delay_y= delay_lc= delay_lm= delay_ly=0;
    delay_max=0;
    stp_deprintf(STP_DBG_CANON,"canon: delay off!\n");
  }

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  buf_length= length*bits;

  stp_deprintf(STP_DBG_CANON,"canon: buflength is %d!\n",buf_length);

  if (colormode==COLOR_MONOCHROME) {
    black   = stp_zalloc(buf_length*(delay_k+1));
    cyan    = NULL;
    magenta = NULL;
    lcyan   = NULL;
    lmagenta= NULL;
    yellow  = NULL;
    lyellow = NULL;
  } else {
    cyan    = stp_zalloc(buf_length*(delay_c+1));
    magenta = stp_zalloc(buf_length*(delay_m+1));
    yellow  = stp_zalloc(buf_length*(delay_y+1));

    if (colormode!=COLOR_CMY)
      black = stp_zalloc(buf_length*(delay_k+1));
    else
      black = NULL;

    if (colormode==COLOR_CCMMYK || colormode==COLOR_CCMMYYK) {
      use_6color= 1;
      lcyan = stp_zalloc(buf_length*(delay_lc+1));
      lmagenta = stp_zalloc(buf_length*(delay_lm+1));
      if (colormode==CANON_INK_CcMmYyK)
	lyellow = stp_zalloc(buf_length*(delay_lc+1));
      else
	lyellow = NULL;
    } else {
      lcyan = NULL;
      lmagenta = NULL;
      lyellow = NULL;
    }
  }

  stp_deprintf(STP_DBG_CANON,"canon: driver will use colors %s%s%s%s%s%s\n",
	       cyan ? "C" : "", lcyan ? "c" : "", magenta ? "M" : "",
	       lmagenta ? "m" : "", yellow ? "Y" : "", black ? "K" : "");

  stp_deprintf(STP_DBG_CANON,"density is %f\n",
	       stp_get_float_parameter(nv, "Density"));

  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */
  if (output_type != OUTPUT_RAW_PRINTER && output_type != OUTPUT_RAW_CMYK)
    {
      if (pt)
	stp_scale_float_parameter(nv, "Density", pt->base_density);
      else			/* Can't find paper type? Assume plain */
	stp_scale_float_parameter(nv, "Density", .5);
      stp_scale_float_parameter(nv, "Density", canon_density(caps, res_code));
    }
  if (stp_get_float_parameter(nv, "Density") > 1.0)
    stp_set_float_parameter(nv, "Density", 1.0);
  if (colormode == COLOR_MONOCHROME)
    stp_scale_float_parameter(nv, "Gamma", 1.25);

  stp_deprintf(STP_DBG_CANON,"density is %f\n",
	       stp_get_float_parameter(nv, "Density"));

 /*
  * Output the page...
  */


  if (use_6color)
    k_lower = .4 / bits + .1;
  else
    k_lower = .25 / bits;
  if (pt)
    {
      k_lower *= pt->k_lower_scale;
      k_upper = pt->k_upper;
    }
  else
    {
      k_lower *= .5;
      k_upper = .5;
    }
  stp_set_default_float_parameter(nv, "GCRLower", k_lower);
  stp_set_default_float_parameter(nv, "GCRUpper", k_upper);
  stp_dither_init(nv, image, out_width, xdpi, ydpi);

  if ((inks = canon_inks(caps, res_code, colormode, bits))!=0)
    {
      if (inks->c)
	stp_dither_set_ranges(nv, ECOLOR_C, inks->c->count, inks->c->range,
			      inks->c->density *
			      stp_get_float_parameter(nv, "Density"));
      if (inks->m)
	stp_dither_set_ranges(nv, ECOLOR_M, inks->m->count, inks->m->range,
			      inks->m->density *
			      stp_get_float_parameter(nv, "Density"));
      if (inks->y)
	stp_dither_set_ranges(nv, ECOLOR_Y, inks->y->count, inks->y->range,
			      inks->y->density *
			      stp_get_float_parameter(nv, "Density"));
      if (inks->k)
	stp_dither_set_ranges(nv, ECOLOR_K, inks->k->count, inks->k->range,
			      inks->k->density *
			      stp_get_float_parameter(nv, "Density"));
    }

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;

  if (!stp_check_curve_parameter(nv, "HueMap"))
    {
      hue_adjustment = stp_read_and_compose_curves
	(canon_hue_adjustment(model),
	 pt ? pt->hue_adjustment : NULL, STP_CURVE_COMPOSE_ADD);
      stp_set_curve_parameter(nv, "HueMap", hue_adjustment);
      stp_curve_destroy(hue_adjustment);
    }
  if (!stp_check_curve_parameter(nv, "LumMap"))
    {
      lum_adjustment = stp_read_and_compose_curves
	(canon_lum_adjustment(model),
	 pt ? pt->lum_adjustment : NULL, STP_CURVE_COMPOSE_MULTIPLY);
      stp_set_curve_parameter(nv, "LumMap", lum_adjustment);
      stp_curve_destroy(lum_adjustment);
    }
  if (!stp_check_curve_parameter(nv, "SatMap"))
    {
      sat_adjustment = stp_read_and_compose_curves
	(canon_sat_adjustment(model),
	 pt ? pt->sat_adjustment : NULL, STP_CURVE_COMPOSE_MULTIPLY);
      stp_set_curve_parameter(nv, "SatMap", sat_adjustment);
      stp_curve_destroy(sat_adjustment);
    }
  if (output_type == OUTPUT_COLOR && black)
    {
      output_type = OUTPUT_RAW_CMYK;
      stp_set_output_type(nv, OUTPUT_RAW_CMYK);
    }

  out_channels = stp_color_init(nv, image, 65536);

  out = stp_zalloc(image_width * out_channels * 2);

  stp_dither_add_channel(nv, black, ECOLOR_K, 0);
  stp_dither_add_channel(nv, cyan, ECOLOR_C, 0);
  stp_dither_add_channel(nv, lcyan, ECOLOR_C, 1);
  stp_dither_add_channel(nv, magenta, ECOLOR_M, 0);
  stp_dither_add_channel(nv, lmagenta, ECOLOR_M, 1);
  stp_dither_add_channel(nv, yellow, ECOLOR_Y, 0);

  for (y = 0; y < out_height; y ++)
  {
    int duplicate_line = 1;
    if ((y & 63) == 0)
      stp_image_note_progress(image, y, out_height);

    if (errline != errlast)
    {
      errlast = errline;
      duplicate_line = 0;
      if (stp_color_get_row(nv, image, errline, out, &zero_mask))
	{
	  status = 2;
	  break;
	}
    }

    stp_dither(nv, y, out, duplicate_line, zero_mask);

    canon_write_line(nv, caps, ydpi,
		     black,    delay_k,
		     cyan,     delay_c,
		     magenta,  delay_m,
		     yellow,   delay_y,
		     lcyan,    delay_lc,
		     lmagenta, delay_lm,
		     lyellow,  delay_ly,
		     length, out_width, left, &emptylines, bits);

    canon_advance_buffer(black,   buf_length,delay_k);
    canon_advance_buffer(cyan,    buf_length,delay_c);
    canon_advance_buffer(magenta, buf_length,delay_m);
    canon_advance_buffer(yellow,  buf_length,delay_y);
    canon_advance_buffer(lcyan,   buf_length,delay_lc);
    canon_advance_buffer(lmagenta,buf_length,delay_lm);
    canon_advance_buffer(lyellow, buf_length,delay_ly);

    errval += errmod;
    errline += errdiv;
    if (errval >= out_height)
    {
      errval -= out_height;
      errline ++;
    }
  }

  /*
   * Flush delayed buffers...
   */

  if (delay_max) {
    stp_deprintf(STP_DBG_CANON,"\ncanon: flushing %d possibly delayed buffers\n",
	    delay_max);
    for (y= 0; y<delay_max; y++) {

      canon_write_line(nv, caps, ydpi,
		       black,    delay_k,
		       cyan,     delay_c,
		       magenta,  delay_m,
		       yellow,   delay_y,
		       lcyan,    delay_lc,
		       lmagenta, delay_lm,
		       lyellow,  delay_ly,
		       length, out_width, left, &emptylines, bits);

      canon_advance_buffer(black,   buf_length,delay_k);
      canon_advance_buffer(cyan,    buf_length,delay_c);
      canon_advance_buffer(magenta, buf_length,delay_m);
      canon_advance_buffer(yellow,  buf_length,delay_y);
      canon_advance_buffer(lcyan,   buf_length,delay_lc);
      canon_advance_buffer(lmagenta,buf_length,delay_lm);
      canon_advance_buffer(lyellow, buf_length,delay_ly);
    }
  }

  stp_image_progress_conclude(image);

  stp_dither_free(nv);

 /*
  * Cleanup...
  */

  stp_free(out);

  if (black != NULL)    stp_free(black);
  if (cyan != NULL)     stp_free(cyan);
  if (magenta != NULL)  stp_free(magenta);
  if (yellow != NULL)   stp_free(yellow);
  if (lcyan != NULL)    stp_free(lcyan);
  if (lmagenta != NULL) stp_free(lmagenta);
  if (lyellow != NULL)  stp_free(lyellow);

  canon_deinit_printer(nv, &init);
  stp_vars_free(nv);
  return status;
}

static const stp_printfuncs_t stp_canon_printfuncs =
{
  canon_list_parameters,
  canon_parameters,
  stp_default_media_size,
  canon_imageable_area,
  canon_limit,
  canon_print,
  canon_describe_resolution,
  stp_verify_printer_params,
  NULL,
  NULL
};

/*
 * 'canon_fold_lsb_msb()' fold 2 lines in order lsb/msb
 */

static void
canon_fold_2bit(const unsigned char *line,
		int single_length,
		unsigned char *outbuf)
{
  int i;
  for (i = 0; i < single_length; i++) {
    outbuf[0] =
      ((line[0] & (1 << 7)) >> 1) |
      ((line[0] & (1 << 6)) >> 2) |
      ((line[0] & (1 << 5)) >> 3) |
      ((line[0] & (1 << 4)) >> 4) |
      ((line[single_length] & (1 << 7)) >> 0) |
      ((line[single_length] & (1 << 6)) >> 1) |
      ((line[single_length] & (1 << 5)) >> 2) |
      ((line[single_length] & (1 << 4)) >> 3);
    outbuf[1] =
      ((line[0] & (1 << 3)) << 3) |
      ((line[0] & (1 << 2)) << 2) |
      ((line[0] & (1 << 1)) << 1) |
      ((line[0] & (1 << 0)) << 0) |
      ((line[single_length] & (1 << 3)) << 4) |
      ((line[single_length] & (1 << 2)) << 3) |
      ((line[single_length] & (1 << 1)) << 2) |
      ((line[single_length] & (1 << 0)) << 1);
    line++;
    outbuf += 2;
  }
}

#ifndef USE_3BIT_FOLD_TYPE
#error YOU MUST CHOOSE A VALUE FOR USE_3BIT_FOLD_TYPE
#endif

#if USE_3BIT_FOLD_TYPE == 333

static void
canon_fold_3bit(const unsigned char *line,
		int single_length,
		unsigned char *outbuf)
{
  int i;
  for (i = 0; i < single_length; i++) {
    outbuf[0] =
      ((line[0] & (1 << 7)) >> 2) |
      ((line[0] & (1 << 6)) >> 4) |
      ((line[single_length] & (1 << 7)) >> 1) |
      ((line[single_length] & (1 << 6)) >> 3) |
      ((line[single_length] & (1 << 5)) >> 5) |
      ((line[2*single_length] & (1 << 7)) << 0) |
      ((line[2*single_length] & (1 << 6)) >> 2) |
      ((line[2*single_length] & (1 << 5)) >> 4);
    outbuf[1] =
      ((line[0] & (1 << 5)) << 2) |
      ((line[0] & (1 << 4)) << 0) |
      ((line[0] & (1 << 3)) >> 2) |
      ((line[single_length] & (1 << 4)) << 1) |
      ((line[single_length] & (1 << 3)) >> 1) |
      ((line[2*single_length] & (1 << 4)) << 2) |
      ((line[2*single_length] & (1 << 3)) << 0) |
      ((line[2*single_length] & (1 << 2)) >> 2);
    outbuf[2] =
      ((line[0] & (1 << 2)) << 4) |
      ((line[0] & (1 << 1)) << 2) |
      ((line[0] & (1 << 0)) << 0) |
      ((line[single_length] & (1 << 2)) << 5) |
      ((line[single_length] & (1 << 1)) << 3) |
      ((line[single_length] & (1 << 0)) << 1) |
      ((line[2*single_length] & (1 << 1)) << 4) |
      ((line[2*single_length] & (1 << 0)) << 2);
    line++;
    outbuf += 3;
  }
}

#elif USE_3BIT_FOLD_TYPE == 323

static void
canon_fold_3bit(const unsigned char *line,
		int single_length,
		unsigned char *outbuf)
{
  unsigned char A0,A1,A2,B0,B1,B2,C0,C1,C2;
  const unsigned char *last= line+single_length;

  for (; line < last; line+=3, outbuf+=8) {

    A0= line[0]; B0= line[single_length]; C0= line[2*single_length];

    if (line<last-2) {
      A1= line[1]; B1= line[single_length+1]; C1= line[2*single_length+1];
    } else {
      A1= 0; B1= 0; C1= 0;
    }
    if (line<last-1) {
      A2= line[2]; B2= line[single_length+2]; C2= line[2*single_length+2];
    } else {
      A2= 0; B2= 0; C2= 0;
    }

    outbuf[0] =
      ((C0 & 0x80) >> 0) |
      ((B0 & 0x80) >> 1) |
      ((A0 & 0x80) >> 2) |
      ((B0 & 0x40) >> 2) |
      ((A0 & 0x40) >> 3) |
      ((C0 & 0x20) >> 3) |
      ((B0 & 0x20) >> 4) |
      ((A0 & 0x20) >> 5);
    outbuf[1] =
      ((C0 & 0x10) << 3) |
      ((B0 & 0x10) << 2) |
      ((A0 & 0x10) << 1) |
      ((B0 & 0x08) << 1) |
      ((A0 & 0x08) << 0) |
      ((C0 & 0x04) >> 0) |
      ((B0 & 0x04) >> 1) |
      ((A0 & 0x04) >> 2);
    outbuf[2] =
      ((C0 & 0x02) << 6) |
      ((B0 & 0x02) << 5) |
      ((A0 & 0x02) << 4) |
      ((B0 & 0x01) << 4) |
      ((A0 & 0x01) << 3) |
      ((C1 & 0x80) >> 5) |
      ((B1 & 0x80) >> 6) |
      ((A1 & 0x80) >> 7);
    outbuf[3] =
      ((C1 & 0x40) << 1) |
      ((B1 & 0x40) << 0) |
      ((A1 & 0x40) >> 1) |
      ((B1 & 0x20) >> 1) |
      ((A1 & 0x20) >> 2) |
      ((C1 & 0x10) >> 2) |
      ((B1 & 0x10) >> 3) |
      ((A1 & 0x10) >> 4);
    outbuf[4] =
      ((C1 & 0x08) << 4) |
      ((B1 & 0x08) << 3) |
      ((A1 & 0x08) << 2) |
      ((B1 & 0x04) << 2) |
      ((A1 & 0x04) << 1) |
      ((C1 & 0x02) << 1) |
      ((B1 & 0x02) >> 0) |
      ((A1 & 0x02) >> 1);
    outbuf[5] =
      ((C1 & 0x01) << 7) |
      ((B1 & 0x01) << 6) |
      ((A1 & 0x01) << 5) |
      ((B2 & 0x80) >> 3) |
      ((A2 & 0x80) >> 4) |
      ((C2 & 0x40) >> 4) |
      ((B2 & 0x40) >> 5) |
      ((A2 & 0x40) >> 6);
    outbuf[6] =
      ((C2 & 0x20) << 2) |
      ((B2 & 0x20) << 1) |
      ((A2 & 0x20) << 0) |
      ((B2 & 0x10) >> 0) |
      ((A2 & 0x10) >> 1) |
      ((C2 & 0x08) >> 1) |
      ((B2 & 0x08) >> 2) |
      ((A2 & 0x08) >> 3);
    outbuf[7] =
      ((C2 & 0x04) << 5) |
      ((B2 & 0x04) << 4) |
      ((A2 & 0x04) << 3) |
      ((B2 & 0x02) << 3) |
      ((A2 & 0x02) << 2) |
      ((C2 & 0x01) << 2) |
      ((B2 & 0x01) << 1) |
      ((A2 & 0x01) << 0);
  }
}

#else
#error 3BIT FOLD TYPE NOT IMPLEMENTED
#endif

static void
canon_shift_buffer(unsigned char *line,int length,int bits)
{
  int i,j;
  for (j=0; j<bits; j++) {
    for (i=length-1; i>0; i--) {
      line[i]= (line[i] >> 1) | (line[i-1] << 7);
    }
    line[0] = line[0] >> 1;
  }
}

static void
canon_shift_buffer2(unsigned char *line,int length,int bits)
{
  int i;
  for (i=length-1; i>0; i--) {
    line[i]= (line[i] >> bits) | (line[i-1] << (8-bits));
  }
  line[0] = line[0] >> bits;
}


/*
 * 'canon_write()' - Send graphics using TIFF packbits compression.
 */

static int
canon_write(const stp_vars_t v,		/* I - Print file or command */
	    const canon_cap_t *   caps,	        /* I - Printer model */
	    unsigned char *line,	/* I - Output bitmap data */
	    int           length,	/* I - Length of bitmap data */
	    int           coloridx,	/* I - Which color */
	    int           ydpi,		/* I - Vertical resolution */
	    int           *empty,       /* IO- Preceeding empty lines */
	    int           width,	/* I - Printed width */
	    int           offset, 	/* I - Offset from left side */
	    int           bits)
{
  unsigned char
    comp_buf[COMPBUFWIDTH],		/* Compression buffer */
    in_fold[COMPBUFWIDTH],
    *in_ptr= line,
    *comp_ptr, *comp_data;
  int newlength;
  int offset2,bitoffset;
  unsigned char color;

 /* Don't send blank lines... */

  if (line[0] == 0 && memcmp(line, line + 1, length - 1) == 0)
    return 0;

  /* fold lsb/msb pairs if drop modulation is active */



  if (bits==2) {
    memset(in_fold,0,length*2);
    canon_fold_2bit(line,length,in_fold);
    in_ptr= in_fold;
    length= (length*8/4); /* 4 pixels in 8bit */
    offset= (offset*8/4); /* 4 pixels in 8bit  */
  }
  if (bits==3) {
    memset(in_fold,0,length*3);
    canon_fold_3bit(line,length,in_fold);
    in_ptr= in_fold;
    length= (length*8)/3;
    offset= (offset/3)*8;
#if 0
    switch(offset%3){
    case 0: offset= (offset/3)*8;   break;
    case 1: offset= (offset/3)*8/*+3 CAREFUL! CANNOT SHIFT _AFTER_ RECODING!!*/; break;
    case 2: offset= (offset/3)*8/*+5 CAREFUL! CANNOT SHIFT _AFTER_ RECODING!!*/; break;
    }
#endif
  }
  /* pack left border rounded to multiples of 8 dots */

  comp_data= comp_buf;
  offset2= offset/8;
  bitoffset= offset%8;
  while (offset2>0) {
    unsigned char toffset = offset2 > 128 ? 128 : offset2;
    comp_data[0] = 1 - toffset;
    comp_data[1] = 0;
    comp_data += 2;
    offset2-= toffset;
  }
  if (bitoffset) {
    if (bitoffset<8)
      canon_shift_buffer(in_ptr,length,bitoffset);
    else
      stp_deprintf(STP_DBG_CANON,"SEVERE BUG IN print-canon.c::canon_write() "
	      "bitoffset=%d!!\n",bitoffset);
  }

  stp_pack_tiff(in_ptr, length, comp_data, &comp_ptr, NULL, NULL);
  newlength= comp_ptr - comp_buf;

  /* send packed empty lines if any */

  if (*empty) {
    stp_zfwrite("\033\050\145\002\000", 5, 1, v);
    stp_put16_le(*empty, v);
    *empty= 0;
  }

 /* Send a line of raster graphics... */

  stp_zfwrite("\033\050\101", 3, 1, v);
  stp_put16_le(newlength + 1, v);
  color= "CMYKcmy"[coloridx];
  if (!color) color= 'K';
  stp_putc(color,v);
  stp_zfwrite((const char *)comp_buf, newlength, 1, v);
  stp_putc('\015', v);
  return 1;
}


static void
canon_write_line(const stp_vars_t v,	/* I - Print file or command */
		 const canon_cap_t *   caps,	/* I - Printer model */
		 int           ydpi,	/* I - Vertical resolution */
		 unsigned char *k,	/* I - Output bitmap data */
		 int           dk,	/* I -  */
		 unsigned char *c,	/* I - Output bitmap data */
		 int           dc,	/* I -  */
		 unsigned char *m,	/* I - Output bitmap data */
		 int           dm,	/* I -  */
		 unsigned char *y,	/* I - Output bitmap data */
		 int           dy,	/* I -  */
		 unsigned char *lc,	/* I - Output bitmap data */
		 int           dlc,	/* I -  */
		 unsigned char *lm,	/* I - Output bitmap data */
		 int           dlm,	/* I -  */
		 unsigned char *ly,	/* I - Output bitmap data */
		 int           dly,	/* I -  */
		 int           l,	/* I - Length of bitmap data */
		 int           width,	/* I - Printed width */
		 int           offset,  /* I - horizontal offset */
		 int           *empty,  /* IO- unflushed empty lines */
		 int           bits)
{
  int written= 0;

  if (k) written+=
    canon_write(v, caps, k+ dk*l,  l, 3, ydpi, empty, width, offset, bits);
  if (y) written+=
    canon_write(v, caps, y+ dy*l,  l, 2, ydpi, empty, width, offset, bits);
  if (m) written+=
    canon_write(v, caps, m+ dm*l,  l, 1, ydpi, empty, width, offset, bits);
  if (c) written+=
    canon_write(v, caps, c+ dc*l,  l, 0, ydpi, empty, width, offset, bits);
  if (ly) written+=
    canon_write(v, caps, ly+dly*l, l, 6, ydpi, empty, width, offset, bits);
  if (lm) written+=
    canon_write(v, caps, lm+dlm*l, l, 5, ydpi, empty, width, offset, bits);
  if (lc) written+=
    canon_write(v, caps, lc+dlc*l, l, 4, ydpi, empty, width, offset, bits);

  if (written||(empty==0))
    stp_zfwrite("\033\050\145\002\000\000\001", 7, 1, v);
  else
    (*empty)+= 1;
}


static stp_internal_family_t stp_canon_module_data =
  {
    &stp_canon_printfuncs,
    NULL
  };


static int
canon_module_init(void)
{
  return stp_family_register(stp_canon_module_data.printer_list);
}


static int
canon_module_exit(void)
{
  return stp_family_unregister(stp_canon_module_data.printer_list);
}


/* Module header */
#define stp_module_version stp_canon_LTX_stp_module_version
#define stp_module_data stp_canon_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "canon",
    VERSION,
    "Canon family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    canon_module_init,
    canon_module_exit,
    (void *) &stp_canon_module_data
  };

