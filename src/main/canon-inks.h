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

/* ink definition */
typedef struct {
  const int bits;                     /* number of bits */
  const int flags;
#define INK_FLAG_5pixel_in_1byte 0x1  /* use special compression where 5 3level pixels get stored in 1 byte */
  int numsizes;                       /* number of possible dot_sizes */
  const stp_dotsize_t *dot_sizes;
} canon_ink_t;


#define DECLARE_INK(bits,dotsizes)      \
static const canon_ink_t canon_##bits##bit_ink = {              \
  bits,0,                                  \
  sizeof(dotsizes)/sizeof(stp_dotsize_t), dotsizes   \
}



/* NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE  NOTE
 *
 * The following dither ranges were taken from print-escp2.c and do NOT
 * represent the requirements of canon inks. Feel free to play with them
 * accoring to the escp2 part of doc/README.new-printer and send me a patch
 * if you get better results. Please send mail to thaller@ph.tum.de
 */


static const stp_dotsize_t single_dotsize[] = {
  { 0x1, 1.0 }
};

DECLARE_INK(1,single_dotsize);


static const stp_dotsize_t two_bit_3level_dotsizes[] = {
  { 0x1, 0.5  },
  { 0x2, 1.0  }
};


static const canon_ink_t canon_2bit_3level_ink = {
  2,INK_FLAG_5pixel_in_1byte,2,two_bit_3level_dotsizes
};


static const stp_dotsize_t two_bit_dotsizes[] = {
  { 0x1, 0.45 },
  { 0x2, 0.68 },
  { 0x3, 1.0 }
};

DECLARE_INK(2,two_bit_dotsizes);


static const stp_dotsize_t three_bit_dotsizes[] = {
  { 0x1, 0.45 },
  { 0x2, 0.55 },
  { 0x3, 0.66 },
  { 0x4, 0.77 },
  { 0x5, 0.88 },
  { 0x6, 1.0 }
};

DECLARE_INK(3,three_bit_dotsizes);


/* A inkset is a list of inks and their (relative) densities 
 * For printers that use the extended SetImage command t)
 * the inkset will be used to build the parameter list
 * therefore invalid inksets will let the printer fallback
 * to a default mode which will then lead to wrong output
 */


typedef struct {
   const int channel;
   const double density;
   const canon_ink_t* ink;
} canon_inkset_t;


/* Inkset for printing in K and 1bit/pixel */
static const canon_inkset_t canon_K_1bit_inkset[] = {
        {'K',1.0,&canon_1bit_ink}
};

/* Inkset for printing in CMY and 1bit/pixel */
static const canon_inkset_t canon_CMY_1bit_inkset[] = {
        {'C',1.0,&canon_1bit_ink},
        {'M',1.0,&canon_1bit_ink},
        {'Y',1.0,&canon_1bit_ink}
};


/* Inkset for printing in CMY and 2bit/pixel */
static const canon_inkset_t canon_CMY_2bit_inkset[] = {
        {'C',1.0,&canon_2bit_ink},
        {'M',1.0,&canon_2bit_ink},
        {'Y',1.0,&canon_2bit_ink}
};

/* Inkset for printing in CMYK and 1bit/pixel */
static const canon_inkset_t canon_CMYK_1bit_inkset[] = {
        {'C',1.0,&canon_1bit_ink},
        {'M',1.0,&canon_1bit_ink},
        {'Y',1.0,&canon_1bit_ink},
        {'K',1.0,&canon_1bit_ink}
};

/* Inkset for printing in CMYK and 2bit/pixel */
static const canon_inkset_t canon_CMYK_2bit_inkset[] = {
        {'C',1.0,&canon_2bit_ink},
        {'M',1.0,&canon_2bit_ink},
        {'Y',1.0,&canon_2bit_ink},
        {'K',1.0,&canon_2bit_ink}
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
        {'C',1.0,&canon_3bit_ink},
        {'M',1.0,&canon_3bit_ink},
        {'Y',1.0,&canon_3bit_ink},
        {'K',1.0,&canon_3bit_ink}
};

/* Inkset for printing in CMYKcm and 1bit/pixel */
static const canon_inkset_t canon_CMYKcm_1bit_inkset[] = {
        {'C',1.0,&canon_1bit_ink},
        {'M',1.0,&canon_1bit_ink},
        {'Y',1.0,&canon_1bit_ink},
        {'K',1.0,&canon_1bit_ink},
        {'c',0.25,&canon_1bit_ink},
        {'m',0.26,&canon_1bit_ink}
};

/* Inkset for printing in CMYKcm and 2bit/pixel */
static const canon_inkset_t canon_CMYKcm_2bit_inkset[] = {
        {'C',1.0,&canon_2bit_ink},
        {'M',1.0,&canon_2bit_ink},
        {'Y',1.0,&canon_2bit_ink},
        {'K',1.0,&canon_2bit_ink},
        {'c',0.33,&canon_2bit_ink},
        {'m',0.33,&canon_2bit_ink}
};

/* Inkset for printing in CMYKcm and 3bit/pixel */
static const canon_inkset_t canon_CMYKcm_3bit_inkset[] = {
        {'C',1.0,&canon_3bit_ink},
        {'M',1.0,&canon_3bit_ink},
        {'Y',1.0,&canon_3bit_ink},
        {'K',1.0,&canon_3bit_ink},
        {'c',0.33,&canon_3bit_ink},
        {'m',0.33,&canon_3bit_ink}
};


/* Default Inkset for the PIXMA iP4000 */
static const canon_inkset_t canon_PIXMA_iP4000_default_inkset[] = {
        {'C',1.0,&canon_2bit_3level_ink},
        {'M',1.0,&canon_2bit_3level_ink},
        {'Y',1.0,&canon_1bit_ink},
        {'K',1.0,&canon_1bit_ink},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {'k',0.0,&canon_2bit_3level_ink},  /* even though we won't use the photo black in this mode its parameters have to be set */
        {0,0.0,NULL}
};

/* Default Inkset for the PIXMA iP4200 */
static const canon_inkset_t canon_PIXMA_iP4200_default_inkset[] = {
        {'C',1.0,&canon_2bit_3level_ink},
        {'M',1.0,&canon_2bit_3level_ink},
        {'Y',1.0,&canon_1bit_ink},
        {'K',1.0,&canon_1bit_ink},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {0,0.0,NULL},
        {'k',0.0,&canon_2bit_3level_ink},  /* even though we won't use the photo black in this mode its parameters have to be set */
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

#endif

