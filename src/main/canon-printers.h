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

/* This file contains the capabilities of the various canon printers
*/

#ifndef GUTENPRINT_INTERNAL_CANON_PRINTERS_H
#define GUTENPRINT_INTERNAL_CANON_PRINTERS_H


typedef struct canon_caps {
  int model;          /* model number as used in printers.xml */
  int model_id;       /* model ID code for use in commands */
  int max_width;      /* maximum printable paper size */
  int max_height;
  int border_left;    /* left margin, points */
  int border_right;   /* right margin, points */
  int border_top;     /* absolute top margin, points */
  int border_bottom;  /* absolute bottom margin, points */
  int slots;          /* available paperslots */
  unsigned long features;       /* special bjl settings */
  const canon_modelist_t* modelist;
  const canon_paperlist_t* paperlist;
  const char *lum_adjustment;
  const char *hue_adjustment;
  const char *sat_adjustment;
} canon_cap_t;


static const canon_cap_t canon_model_capabilities[] =
{
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

  { /* Canon S200x *//* heads: BC-24 */
    4202, 3,
    618, 936,       /* 8.58" x 13 " */
    10, 10, 9, 20,
    CANON_SLOT_ASF1,
    CANON_CAP_STD1 | CANON_CAP_rr,
    &canon_S200_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon  BJ 30   *//* heads: BC-10 */
    30, 1,
    9.5*72, 14*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a,
    &canon_BJC_30_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon  BJC 85  *//* heads: BC-20 BC-21 BC-22 */
    85, 1,
    9.5*72, 14*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a,
    &canon_BJC_85_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 4300 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    4300, 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD0,
    &canon_BJC_4300_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 4400 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    4400, 1,
    9.5*72, 14*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a,
    &canon_BJC_4400_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 6000 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    6000, 3,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD1 | CANON_CAP_ACKSHORT,
    &canon_BJC_6000_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 6200 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    6200, 3,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD1 | CANON_CAP_ACKSHORT,
    &canon_BJC_6000_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },

  { /* Canon BJC 6500 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    6500, 3,
    842, 17*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD1,
    &canon_BJC_6000_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 8200 *//* heads: BC-50 */
    8200, 3,
    842, 17*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD1 | CANON_CAP_r | CANON_CAP_ACKSHORT,
    &canon_BJC_8200_modelist,
    &canon_default_paperlist,
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
    11, 9, 10, 18,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD0,
    &canon_BJC_210_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 240 *//* heads: BC-02 BC-05 BC-06 */
    240, 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD0,
    &canon_BJC_240_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 250 *//* heads: BC-02 BC-05 BC-06 */
    250, 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18,
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1,
    CANON_CAP_STD0,
    &canon_BJC_240_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 1000 *//* heads: BC-02 BC-05 BC-06 */
    1000, 1,
    842, 17*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a,
    &canon_BJC_240_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 2000 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    2000, 1,
    842, 17*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a,
    &canon_BJC_2000_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 3000 *//* heads: BC-30 BC-33 BC-34 */
    3000, 3,
    842, 17*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a, /*FIX? should have _r? */
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 6100 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    6100, 3,
    842, 17*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD1 | CANON_CAP_a | CANON_CAP_r,
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 7000 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    7000, 3,
    842, 17*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD1,
    &canon_BJC_7000_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 7100 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    7100, 3,
    842, 17*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0,
    &canon_BJC_7100_modelist,
    &canon_default_paperlist,
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
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0,
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 5500 *//* heads: BC-20 BC-21 BC-29 */
    5500, 1,
    22*72, 34*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0 | CANON_CAP_a,
    &canon_BJC_5500_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 6500 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    6500, 3,
    17*72, 22*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD1 | CANON_CAP_a,
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon BJC 8500 *//* heads: BC-80/BC-81 BC-82/BC-81 */
    8500, 3,
    17*72, 22*72,
    11, 9, 10, 18,
    CANON_SLOT_ASF1,
    CANON_CAP_STD0,
    &canon_BJC_8500_modelist,
    &canon_default_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon PIXMA iP3000 */
    3999, 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    11, 9, 10, 18,    /*border_left, border_right, border_top, border_bottom */
    CANON_SLOT_ASF1,  /*paper slot */
    CANON_CAP_STD0|CANON_CAP_DUPLEX,  /*features */
    &canon_PIXMA_iP3000_modelist,
    &canon_PIXMA_iP4000_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon PIXMA iP4000 */
    4000, 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    11, 9, 10, 18,    /*border_left, border_right, border_top, border_bottom */
    CANON_SLOT_ASF1,  /*paper slot */
    CANON_CAP_STD0|CANON_CAP_DUPLEX,  /*features */
    &canon_PIXMA_iP4000_modelist,
    &canon_PIXMA_iP4000_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon PIXMA iP4200 */
    4200, 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    11, 9, 10, 18,    /*border_left, border_right, border_top, border_bottom */
    CANON_SLOT_ASF1,  /*paper slot */
    CANON_CAP_STD0|CANON_CAP_DUPLEX,  /*features */
    &canon_PIXMA_iP4200_modelist,
    &canon_PIXMA_iP4000_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
  { /* Canon MULTIPASS MP150 */
    4201, 3,          /*model, model_id*/
    842, 17*72,       /* max paper width and height */
    11, 9, 10, 18,    /*border_left, border_right, border_top, border_bottom */
    CANON_SLOT_ASF1,  /*paper slot */
    CANON_CAP_STD0|CANON_CAP_DUPLEX,  /*features */
    &canon_MULTIPASS_MP150_modelist,
    &canon_PIXMA_iP4000_paperlist,
    standard_lum_adjustment,
    standard_hue_adjustment,
    standard_sat_adjustment
  },
};

#endif

