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

/* This file contains definitions for the various inks
*/

#ifndef GUTENPRINT_INTERNAL_CANON_INKS_H
#define GUTENPRINT_INTERNAL_CANON_INKS_H

/* ink definition: 
 *  ink dots can be printed in various sizes 
 *  one size is called level
 *  every level is represented by a bitcombination and a density
 *  the density ranges from 0 (no dot is printed) to 1.0 (maximum dot size)
 *
 *  an ink is therefore defined by the number of bits used for the bitpattern (bitdepth) and the number of possible levels:
 *    a 1 bit ink can have 2 possible levels 0 and 1
 *    a 2 bit ink can have 2*2=4 possible levels with the bitpatterns 0,1,2 and 3 
 *    a 3 bit ink can have 2*2*2=8 possible levels with the bitpatterns 0 to 7
 *    ...
 *  some inks use less levels than possible with the given bitdepth
 *  some inks use special compressions to store for example 5 3 level pixels in 1 byte  
 * naming:
 *  dotsizes are named dotsizes_xl where x is the number of levels (number of dotsizes + 1)
 *  inks are named canon_xb_yl_ink where x is the number of bits representing the y possible ink levels
 *  inks that contain special (compression) flags are named canon_xb_yl_c_ink 
 *
*/


typedef struct {
  const int bits;                     /* bitdepth */
  const int flags;                    /* flags:   */
#define INK_FLAG_5pixel_in_1byte 0x1  /*  use special compression where 5 3level pixels get stored in 1 byte */
  int numsizes;                       /* number of possible {bit,density} tuples */
  const stp_dotsize_t *dot_sizes;     /* pointer to an array of {bit,density} tuples */ 
} canon_ink_t;

/* declare a standard ink */
#define DECLARE_INK(bits,levels)      \
static const canon_ink_t canon_##bits##b_##levels##l_ink = {              \
  bits,0,                                  \
  sizeof(dotsizes_##levels##l)/sizeof(stp_dotsize_t), dotsizes_##levels##l   \
}

/* declare a ink with flags */
#define DECLARE_INK_EXTENDED(bits,levels,flags)      \
static const canon_ink_t canon_##bits##b_##levels##l_c_ink = {              \
  bits,flags,                                  \
  sizeof(dotsizes_##levels##l)/sizeof(stp_dotsize_t), dotsizes_##levels##l   \
}



/* NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE
 *
 * The following dither ranges were taken from print-escp2.c and do NOT
 * represent the requirements of canon inks. Feel free to play with them
 * accoring to the escp2 part of doc/README.new-printer and send me a patch
 * if you get better results. Please send mail to thaller@ph.tum.de
 */


static const stp_dotsize_t dotsizes_2l[] = {
  { 0x1, 1.0 }
};

DECLARE_INK(1,2);


static const stp_dotsize_t dotsizes_3l[] = {
  { 0x1, 0.5  },
  { 0x2, 1.0  }
};

DECLARE_INK(2,3);

DECLARE_INK_EXTENDED(2,3,INK_FLAG_5pixel_in_1byte);

static const stp_dotsize_t dotsizes_4l[] = {
  { 0x1, 0.45 },
  { 0x2, 0.68 },
  { 0x3, 1.0 }
};

DECLARE_INK(2,4);

static const stp_dotsize_t dotsizes_7l[] = {
  { 0x1, 0.45 },
  { 0x2, 0.55 },
  { 0x3, 0.66 },
  { 0x4, 0.77 },
  { 0x5, 0.88 },
  { 0x6, 1.0 }
};

DECLARE_INK(3,7);


/* A inkset is a list of inks and their (relative) densities 
 * For printers that use the extended SetImage command t)
 * the inkset will be used to build the parameter list
 * therefore invalid inksets will let the printer fallback
 * to a default mode which will then lead to wrong output
 * use {0,0.0,NULL} to specify empty inks
 * set density to 0.0 to disable certain inks
 * the paramters will then still occure in the t) command 
 */


typedef struct {
   const int channel;
   const double density;
   const canon_ink_t* ink;
} canon_inkset_t;


/* Inkset for printing in K and 1bit/pixel */
static const canon_inkset_t canon_K_1bit_inkset[] = {
        {'K',1.0,&canon_1b_2l_ink}
};

/* Inkset for printing in CMY and 1bit/pixel */
static const canon_inkset_t canon_CMY_1bit_inkset[] = {
        {'C',1.0,&canon_1b_2l_ink},
        {'M',1.0,&canon_1b_2l_ink},
        {'Y',1.0,&canon_1b_2l_ink}
};


/* Inkset for printing in CMY and 2bit/pixel */
static const canon_inkset_t canon_CMY_2bit_inkset[] = {
        {'C',1.0,&canon_2b_4l_ink},
        {'M',1.0,&canon_2b_4l_ink},
        {'Y',1.0,&canon_2b_4l_ink}
};

/* Inkset for printing in CMYK and 1bit/pixel */
static const canon_inkset_t canon_CMYK_1bit_inkset[] = {
        {'C',1.0,&canon_1b_2l_ink},
        {'M',1.0,&canon_1b_2l_ink},
        {'Y',1.0,&canon_1b_2l_ink},
        {'K',1.0,&canon_1b_2l_ink}
};

/* Inkset for printing in CMYK and 2bit/pixel */
static const canon_inkset_t canon_CMYK_2bit_inkset[] = {
        {'C',1.0,&canon_2b_4l_ink},
        {'M',1.0,&canon_2b_4l_ink},
        {'Y',1.0,&canon_2b_4l_ink},
        {'K',1.0,&canon_2b_4l_ink}
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

/* Inkset for printing in CMYK and 3bit/pixel */
static const canon_inkset_t canon_CMYK_3bit_inkset[] = {
        {'C',1.0,&canon_3b_7l_ink},
        {'M',1.0,&canon_3b_7l_ink},
        {'Y',1.0,&canon_3b_7l_ink},
        {'K',1.0,&canon_3b_7l_ink}
};

/* Inkset for printing in CMYKcm and 1bit/pixel */
static const canon_inkset_t canon_CMYKcm_1bit_inkset[] = {
        {'C',1.0,&canon_1b_2l_ink},
        {'M',1.0,&canon_1b_2l_ink},
        {'Y',1.0,&canon_1b_2l_ink},
        {'K',1.0,&canon_1b_2l_ink},
        {'c',0.25,&canon_1b_2l_ink},
        {'m',0.26,&canon_1b_2l_ink}
};

/* Inkset for printing in CMYKcm and 2bit/pixel */
static const canon_inkset_t canon_CMYKcm_2bit_inkset[] = {
        {'C',1.0,&canon_2b_4l_ink},
        {'M',1.0,&canon_2b_4l_ink},
        {'Y',1.0,&canon_2b_4l_ink},
        {'K',1.0,&canon_2b_4l_ink},
        {'c',0.33,&canon_2b_4l_ink},
        {'m',0.33,&canon_2b_4l_ink}
};

/* Inkset for printing in CMYKcm and 3bit/pixel */
static const canon_inkset_t canon_CMYKcm_3bit_inkset[] = {
        {'C',1.0,&canon_3b_7l_ink},
        {'M',1.0,&canon_3b_7l_ink},
        {'Y',1.0,&canon_3b_7l_ink},
        {'K',1.0,&canon_3b_7l_ink},
        {'c',0.33,&canon_3b_7l_ink},
        {'m',0.33,&canon_3b_7l_ink}
};

/* Default Inkset for the PIXMA iP2000 */
static const canon_inkset_t canon_PIXMA_iP2000_default_inkset[] = {
        {'C',1.0,&canon_2b_3l_ink},
        {'M',1.0,&canon_2b_3l_ink},
        {'Y',1.0,&canon_1b_2l_ink},
        {'K',1.0,&canon_1b_2l_ink},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
};

/* Default Inkset for the PIXMA iP3000 */
static const canon_inkset_t canon_PIXMA_iP3000_default_inkset[] = {
        {'C',1.0,&canon_2b_3l_c_ink},
        {'M',1.0,&canon_2b_3l_c_ink},
        {'Y',1.0,&canon_1b_2l_ink},
        {'K',1.0,&canon_1b_2l_ink},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL}
};

/* Default Inkset for the PIXMA iP4000 */
static const canon_inkset_t canon_PIXMA_iP4000_default_inkset[] = {
        {'C',1.0,&canon_2b_3l_c_ink},
        {'M',1.0,&canon_2b_3l_c_ink},
        {'Y',1.0,&canon_1b_2l_ink},
        {'K',1.0,&canon_1b_2l_ink},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {'k',0.0,&canon_2b_3l_c_ink},  /* even though we won't use the photo black in this mode its parameters have to be set */
        {0,0.0,NULL}
};

/* Default Inkset for the PIXMA iP4200 */
static const canon_inkset_t canon_PIXMA_iP4200_default_inkset[] = {
        {'C',1.0,&canon_2b_3l_c_ink},
        {'M',1.0,&canon_2b_3l_c_ink},
        {'Y',1.0,&canon_1b_2l_ink},
        {'K',1.0,&canon_1b_2l_ink},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {'k',0.0,&canon_2b_3l_c_ink},  /* even though we won't use the photo black in this mode its parameters have to be set */
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
};

/* Default Inkset for the PIXMA iP6700 */
static const canon_inkset_t canon_PIXMA_iP6700_default_inkset[] = {
        {'C',1.0,&canon_2b_3l_ink},
        {'M',1.0,&canon_2b_3l_ink},
        {'Y',1.0,&canon_2b_3l_ink},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {'k',1.0,&canon_2b_3l_ink},
        {0,0.0,NULL}, 
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
};


/* Default Inkset for the MULTIPASS MP150 */
static const canon_inkset_t canon_MULTIPASS_MP150_default_inkset[] = {
        {'C',1.0,&canon_2b_3l_ink},
        {'M',1.0,&canon_2b_3l_ink},
        {'Y',1.0,&canon_1b_2l_ink},
        {'K',1.0,&canon_1b_2l_ink},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
};

#endif

