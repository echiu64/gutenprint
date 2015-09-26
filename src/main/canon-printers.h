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

/* TODO-LIST
 *
 *   * adjust the paper sizes based on available Canon PPDs and
 *     Windows driver information
 *
 */
#ifndef GUTENPRINT_INTERNAL_CANON_PRINTERS_H
#define GUTENPRINT_INTERNAL_CANON_PRINTERS_H
#define INCH(x) (72 * x)

typedef struct canon_caps {
  const char* name;   /* model name */
  int model_id;       /* model ID code for use in commands */
  int max_width;      /* maximum printable paper size */
  int max_height;
  int border_left;    /* left margin, points */
  int border_right;   /* right margin, points */
  int border_top;     /* absolute top margin, points */
  int border_bottom;  /* absolute bottom margin, points */
  /*  int raster_lines_per_block; */
  /* number of raster lines in every F) command */
  const canon_slotlist_t* slotlist; /*available paperslots */
  unsigned long features;  /* special bjl settings */
  unsigned char ESC_r_arg; /* argument used for the ESC (r command during init */
  int ESC_l_len; /* length of ESC (l command, in bytes */
  int ESC_P_len; /* length of ESC (P command, in bytes */
  int CassetteTray_Opts; /* upper/lower cassette tray option */
  const char** control_cmdlist;
  const canon_modelist_t* modelist;
  const canon_paperlist_t* paperlist;
  const canon_modeuselist_t* modeuselist;
  const char *lum_adjustment;
  const char *hue_adjustment;
  const char *sat_adjustment;
  const char *channel_order; /* (in gutenprint notation) 0123 => KCMY,  1230 => CMYK etc. */
} canon_cap_t;

static const char standard_sat_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* B */
/* B */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* M */
/* M */  "1.00 0.95 0.90 0.90 0.90 0.90 0.90 0.90 "  /* R */
/* R */  "0.90 0.95 0.95 1.00 1.00 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* G */
/* G */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char standard_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.65 0.67 0.70 0.72 0.77 0.80 0.82 0.85 "  /* B */
/* B */  "0.87 0.86 0.82 0.79 0.79 0.82 0.85 0.88 "  /* M */
/* M */  "0.92 0.95 0.96 0.97 0.97 0.97 0.96 0.96 "  /* R */
/* R */  "0.96 0.97 0.97 0.98 0.99 1.00 1.00 1.00 "  /* Y */
/* Y */  "1.00 0.97 0.95 0.94 0.93 0.92 0.90 0.86 "  /* G */
/* G */  "0.79 0.76 0.71 0.68 0.68 0.68 0.68 0.66 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char standard_hue_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "0.00 0.06 0.10 0.10 0.06 -.01 -.09 -.17 "  /* B */
/* B */  "-.25 -.33 -.38 -.38 -.36 -.34 -.34 -.34 "  /* M */
/* M */  "-.34 -.34 -.36 -.40 -.50 -.40 -.30 -.20 "  /* R */
/* R */  "-.12 -.07 -.04 -.02 0.00 0.00 0.00 0.00 "  /* Y */
/* Y */  "0.00 0.00 0.00 -.05 -.10 -.15 -.22 -.24 "  /* G */
/* G */  "-.26 -.30 -.33 -.28 -.25 -.20 -.13 -.06 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char iP4200_sat_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* B */
/* B */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* M */
/* M */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* R */
/* R */  "1.00 1.00 1.05 1.10 1.20 1.26 1.34 1.41 "  /* Y */
/* Y */  "1.38 1.32 1.24 1.15 1.08 1.00 1.00 1.00 "  /* G */
/* G */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char iP4200_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.52 0.56 0.61 0.67 0.79 0.86 0.91 0.98 "  /* B */
/* B */  "0.97 0.87 0.84 0.81 0.78 0.76 0.74 0.73 "  /* M */
/* M */  "0.74 0.76 0.78 0.83 0.86 0.90 0.98 1.04 "  /* R */
/* R */  "1.04 1.04 1.04 1.04 1.03 1.03 1.03 1.02 "  /* Y */
/* Y */  "1.02 0.97 0.92 0.88 0.83 0.78 0.74 0.71 "  /* G */
/* G */  "0.70 0.62 0.59 0.53 0.48 0.52 0.53 0.51 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char iP4200_hue_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "0.00 -.20 -.30 -.40 -.40 -.30 -.20 0.00 "  /* B */
/* B */  "0.00 -.04 -.01 0.08 0.14 0.16 0.09 0.00 "  /* M */
/* M */  "0.00 0.00 -.05 -.07 -.10 -.08 -.06 0.00 "  /* R */
/* R */  "0.00 0.04 0.08 0.10 0.13 0.10 0.07 0.00 "  /* Y */
/* Y */  "0.00 -.11 -.18 -.23 -.30 -.37 -.46 -.54 "  /* G */
/* G */  "-.53 -.52 -.57 -.50 -.41 -.25 -.17 0.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char MP450_sat_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* B */
/* B */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* M */
/* M */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* R */
/* R */  "1.00 1.00 1.05 1.10 1.20 1.26 1.34 1.41 "  /* Y */
/* Y */  "1.38 1.32 1.24 1.15 1.08 1.00 1.00 1.00 "  /* G */
/* G */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char MP450_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
/* C */  "0.52 0.56 0.61 0.67 0.79 0.86 0.91 0.98 "  /* B */
/* B */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00 "  /* M */
/* M */  "1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.04 "  /* R */
/* R */  "1.04 1.04 1.04 1.04 1.03 1.03 1.03 1.02 "  /* Y */
/* Y */  "1.02 0.97 0.92 0.88 0.83 0.78 0.74 0.71 "  /* G */
/* G */  "0.70 0.62 0.59 0.53 0.48 0.52 0.53 0.51 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char MP450_hue_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"-6\" upper-bound=\"6\">\n"
/* C */  "-.30 -.33 -.35 -.37 -.40 -.30 -.20 0.00 "  /* B */
/* B */  "0.00 -.04 -.01 0.08 0.14 0.16 0.09 0.00 "  /* M */
/* M */  "0.00 0.00 -.05 -.07 -.10 -.08 -.06 0.00 "  /* R */
/* R */  "0.00 0.04 0.08 0.10 0.13 0.10 0.07 0.00 "  /* Y */
/* Y */  "0.00 -.11 -.18 -.23 -.30 -.37 -.46 -.54 "  /* G */
/* G */  "0.00 0.00 -.02 -.05 -.07 -.11 -.17 -.25 "  /* C */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

static const char* control_cmd_ackshort[] = {
  "AckTime=Short",
  NULL
};

static const char* control_cmd_PIXMA_iP4000[] = {
/*"SetTime=20060722092503", */         /*what is this for?*/
  "SetSilent=OFF",
  "PEdgeDetection=ON",
  "SelectParallel=ECP",
  NULL
};

static const char* control_cmd_BJC_i550[] = {
/*"SetTime=20060722092503", */         /*what is this for?*/
  "SetSilent=OFF",
  "SelectParallel=ECP",
  NULL
};

static const char* control_cmd_BJC_i6100[] = {
/*"SetTime=20060722092503", */         /*what is this for?*/
  "SelectParallel=ECP",
  NULL
};

static const char* control_cmd_PIXMA_iP4200[] = {
/*"SetTime=20060722092503", */         /*original driver sends current time, is it needed?*/
  "SetSilent=OFF",
  "PEdgeDetection=ON",
  NULL
};

static const char* control_cmd_PIXMA_iP2700[] = {
/*"SetTime=20060722092503", */         /*what is this for?*/
  "SetSilent=OFF",
  NULL
};

static const char* control_cmd_PIXMA_MG5300[] = {
/*"SetTime=20060722092503", */         /*what is this for?*/
  NULL
};

static const char* control_cmd_PIXMA_MG3500[] = {
/*"SetTime=20060722092503", */         /*what is this for?*/
  "ForcePWDetection=OFF",
  NULL
};

static const char* control_cmd_PIXMA_MG5600[] = {
/*"SetTime=20060722092503", */         /*what is this for?*/
  "ForcePMDetection=OFF",
  NULL
};

static const char iP4500_channel_order[STP_NCOLORS] = {1,2,3,0}; /* CMYK */

static const canon_cap_t canon_model_capabilities[] =
{
  /* the first printer is used as default in case something has gone wrong in printers.xml */
  { /* Canon MULTIPASS MP830 */
    "PIXMA MP830", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP830_modelist,
    &canon_MULTIPASS_MP600_paperlist,
    &canon_MULTIPASS_MP830_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP800 */
    "PIXMA MP800", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP800_modelist,
    &canon_MULTIPASS_MP600_paperlist,
    &canon_MULTIPASS_MP800_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP810 */
    "PIXMA MP810", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP810_modelist,
    &canon_MULTIPASS_MP600_paperlist,
    &canon_MULTIPASS_MP810_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP950 */
    "PIXMA MP950", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP3100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /* features */
    &canon_MULTIPASS_MP950_modelist,
    &canon_MULTIPASS_MP950_paperlist,
    &canon_MULTIPASS_MP950_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP960 */
    "PIXMA MP960", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4600_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /* features */
    &canon_MULTIPASS_MP960_modelist,
    &canon_MULTIPASS_MP960_paperlist,
    &canon_MULTIPASS_MP960_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP970 */
    "PIXMA MP970", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_P|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP970_modelist,
    &canon_MULTIPASS_MP970_paperlist,
    &canon_MULTIPASS_MP970_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP980 */
    "PIXMA MP980", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4600_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_P|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP980_modelist,
    &canon_MULTIPASS_MP980_paperlist,
    &canon_MULTIPASS_MP980_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP990 --- uses XML */
    "PIXMA MP990", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4600_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0x64,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP990_modelist,
    &canon_MULTIPASS_MP990_paperlist,
    &canon_MULTIPASS_MP990_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },

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
    "S200", 3,
    INCH(19/2), INCH(23),      /* from MacOSX driver */
    10, 10, 9, 20,  /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD1 | CANON_CAP_rr,0x61,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_S200_modelist,
    &canon_BJC_S200_paperlist,
    &canon_BJC_S200_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC S300 */
    "S300", 3,
    INCH(17/2), INCH(23),      /* from MacOSX driver */
    10, 10, 9, 15,  /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_px,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_ackshort,
    &canon_BJC_S300_modelist,
    &canon_BJC_S300_paperlist,
    &canon_BJC_S300_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC S330 */
    "S330", 3,
    INCH(17/2), INCH(23),      /* from MacOSX driver */
    10, 10, 9, 15,  /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_px,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_S330_modelist,
    &canon_BJC_S330_paperlist,
    &canon_BJC_S330_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon S500 */
    "S500", 3,
    INCH(17/2), INCH(23),      /* from MacOSX driver */
    10, 10, 9, 15,  /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_p,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i6100,
    &canon_BJC_S500_modelist,
    &canon_BJC_S300_paperlist,
    &canon_BJC_S500_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon S520 */
    "S520", 3,
    INCH(17/2), INCH(23),      /* from MacOSX driver */
    10, 10, 9, 15,  /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_px,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i6100,
    &canon_BJC_S520_modelist,
    &canon_BJC_S520_paperlist,
    &canon_BJC_S520_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC S600 *//* heads: BC-50 */
    "S600", 3,
    842, INCH(17),
    10, 10, 9, 15,
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_p,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i6100,
    &canon_BJC_S600_modelist,
    &canon_BJC_S300_paperlist,
    &canon_BJC_S600_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC S750 */
    "S750", 3,
    INCH(17/2), INCH(23),      /* from MacOSX driver */
    10, 10, 9, 15,  /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_px,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i550,
    &canon_BJC_S750_modelist,
    &canon_BJC_S750_paperlist,
    &canon_BJC_S750_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC S800 */
    "S800", 3,
    842, INCH(17),
    10, 10, 9, 15,
    &canon_BJC_S800_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_p,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i6100,
    &canon_BJC_S800_modelist,
    &canon_BJC_S800_paperlist,
    &canon_BJC_S800_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC S820 */
    "S820", 3,
    INCH(17/2), INCH(23),      /* from MacOSX driver */
    10, 10, 9, 15,  /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_px|CANON_CAP_rr,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i550,
    &canon_BJC_S820_modelist,
    &canon_BJC_S800_paperlist,
    &canon_BJC_S820_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC S830D */
    "S830", 3,
    INCH(17/2), INCH(23),      /* from MacOSX driver */
    10, 10, 9, 15,  /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_px|CANON_CAP_rr,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_S820_modelist,
    &canon_BJC_S800_paperlist,
    &canon_BJC_S820_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC S900 */
    "S900", 3,
    INCH(17/2), INCH(23),      /* from MacOSX driver */
    10, 10, 9, 15,  /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_px|CANON_CAP_rr,0,
    2,0,
    0, /* Upper/Lower Cassette option */    
    control_cmd_PIXMA_iP2700,
    &canon_BJC_S900_modelist,
    &canon_BJC_S800_paperlist,
    &canon_BJC_S900_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon  BJ 30   *//* heads: BC-10 */
    "30", 1,
    INCH(19/2), INCH(14),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_30_modelist,
    &canon_default_paperlist,
    &canon_BJC_30_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon  BJC 85  *//* heads: BC-20 BC-21 BC-22 */
    "85", 1,
    INCH(19/2), INCH(23),      /* from MacOSX driver */
    10, 10, 9, 20, /* confirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_85_modelist,
    &canon_default_paperlist,
    &canon_BJC_85_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC 4300 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    "4300", 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a | CANON_CAP_cart,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_4300_modelist,
    &canon_default_paperlist,
    &canon_BJC_4300_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC 4400 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    "4400", 1,
    INCH(19/2), INCH(14),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_4400_modelist,
    &canon_default_paperlist,
    &canon_BJC_4400_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 4550 *//* heads: BC-21 BCI-21 BC-22 */
    "4550", 3,
    INCH(17), INCH(22),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a | CANON_CAP_cart,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_4550_modelist,
    &canon_default_paperlist,
    &canon_BJC_4550_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC 6000 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    "6000", 3,
    618, 4745,      /* 8.58" x 65.9" (banner paper) */
    10, 10, 9, 20, /* l 3.4mm r 3.4mm t 3mm b 7mm */
    &canon_default_slotlist,
    CANON_CAP_STD1 | CANON_CAP_cart,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i6100,
    &canon_BJC_6000_modelist,
    &canon_default_paperlist,
    &canon_BJC_6000_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC 6200 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    "6200", 3,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD1,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_ackshort,
    &canon_BJC_6000_modelist,
    &canon_default_paperlist,
    &canon_BJC_6000_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  { /* Canon BJC 6500 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    "6500", 3,
    821, INCH(17), /* printing width: 289,6mm = 11.4" = 821 points */
    10, 10, 9, 15, /* l 3.4mm r 3.4mm t 3mm b 5mm */
    &canon_default_slotlist,
    CANON_CAP_STD1 | CANON_CAP_cart,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i6100,
    &canon_BJC_6000_modelist,
    &canon_default_paperlist,
    &canon_BJC_6000_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 8200 *//* heads: BC-50 */
    "8200", 3,
    842, INCH(17),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD1 | CANON_CAP_r,0x61,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_ackshort,
    &canon_BJC_8200_modelist,
    &canon_default_paperlist,
    &canon_BJC_8200_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },


  /* *************** */
  /*                 */
  /* untested models */
  /*                 */
  /* *************** */

  { /* Canon BJC 210 *//* heads: BC-02 BC-05 BC-06 */
    "210", 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_210_modelist,
    &canon_default_paperlist,
    &canon_BJC_210_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 240 *//* heads: BC-02 BC-05 BC-06 */
    "240", 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_240_modelist,
    &canon_default_paperlist,
    &canon_BJC_240_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 250 *//* heads: BC-02 BC-05 BC-06 */
    "250", 1,
    618, 936,      /* 8.58" x 13 " */
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_240_modelist,
    &canon_default_paperlist,
    &canon_BJC_240_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 1000 *//* heads: BC-02 BC-05 BC-06 */
    "1000", 1,
    842, INCH(17),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_240_modelist,
    &canon_default_paperlist,
    &canon_BJC_240_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 2000 *//* heads: BC-20 BC-21 BC-22 BC-29 */
    "2000", 1,
    842, INCH(17),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_2000_modelist,
    &canon_default_paperlist,
    &canon_BJC_2000_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 3000 *//* heads: BC-30 BC-33 BC-34 */
    "3000", 3,
    842, INCH(17),
    10, 10, 9, 15, /* unconfirmed but looks right */
    &canon_MULTIPASS_MP150_slotlist, /* cartridge selection option */
    CANON_CAP_STD0 | CANON_CAP_p | CANON_CAP_cart,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i6100,
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    &canon_BJC_3000_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 6100 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    "6100", 3,
    842, INCH(17),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD1 | CANON_CAP_a | CANON_CAP_r,0x61,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    &canon_BJC_3000_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 7000 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    "7000", 3,
    842, INCH(17),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD1,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_7000_modelist,
    &canon_default_paperlist,
    &canon_BJC_7000_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 7100 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    "7100", 3,
    842, INCH(17),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_7100_modelist,
    &canon_default_paperlist,
    &canon_BJC_7100_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i50 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    /* 50i sold outside of Japan as the i70, but we need i in front for the name */
    "i50", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15,/* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_px|CANON_CAP_BORDERLESS,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i50_modelist,
    &canon_BJC_i50_paperlist,
    &canon_BJC_i50_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i80 *//* heads: BC-60/BC-61 BC-60/BC-62   ??????? */
    "i80", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    2,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i80_modelist,
    &canon_BJC_i80_paperlist,
    &canon_BJC_i80_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
#if 0
  { /* Canon BJC i250 */ /* does not conform to any current model, commands not known yet */
    "i250", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_BORDERLESS,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i250_modelist,
    &canon_BJC_i250_paperlist,
    &canon_BJC_i250_modeuselist,/*not yet created*/
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i320 */ /* does not conform to any current model, commands not known yet */
    "i320", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_BORDERLESS,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i320_modelist,
    &canon_BJC_i320_paperlist,
    &canon_BJC_i320_modeuselist,/*not yet created*/
    NULL,
    NULL,
    NULL,
    NULL
  },
#endif
  { /* Canon BJC i450 */
    "i450", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i450_modelist,
    &canon_BJC_i450_paperlist,
    &canon_BJC_i450_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i455*/
    "i455", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i455_modelist,
    &canon_BJC_i455_paperlist,
    &canon_BJC_i455_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon BJC i550 */
    "i550", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i550,
    &canon_BJC_i550_modelist,
    &canon_BJC_i550_paperlist,
    &canon_BJC_i550_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i560 */
    "i560", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP900_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i550,
    &canon_BJC_i560_modelist,
    &canon_BJC_i560_paperlist,
    &canon_BJC_i560_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon BJC i850 */
    "i850", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP900_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i550,
    &canon_BJC_i850_modelist,
    &canon_BJC_i850_paperlist,
    &canon_BJC_i850_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i860 */
    "i860", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_BJC_i860_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_M|CANON_CAP_BORDERLESS,0,/* ESC (M 0x0 0x0 0x0 */
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i550,
    &canon_BJC_i860_modelist,
    &canon_BJC_i560_paperlist,
    &canon_BJC_i860_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon BJC i900D */
    "i900", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP900_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_M|CANON_CAP_BORDERLESS,0,/* ESC (M 0x0 0x0 0x0 */
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i900_modelist,
    &canon_BJC_i560_paperlist,
    &canon_BJC_i900_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon BJC i950 */
    "i950", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP900_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i950_modelist,
    &canon_BJC_i950_paperlist,
    &canon_BJC_i950_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i960 */
    "i960", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_BJC_i860_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_P|CANON_CAP_M|CANON_CAP_BORDERLESS,0,/* ESC (M 0x0 0x0 0x0 */
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i960_modelist,
    &canon_BJC_i560_paperlist,
    &canon_BJC_i960_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon BJC i990 */
    "i990", 3,
    INCH(17/2), INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_BJC_i860_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_P|CANON_CAP_M|CANON_CAP_BORDERLESS,0,/* ESC (M 0x0 0x0 0x0 */
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i990_modelist,
    &canon_BJC_i560_paperlist,
    &canon_BJC_i990_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon BJC i6100 */
    "i6100", 3,
    933, INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i6100, /* 32 times 0x0 before form feed not implemented */
    &canon_BJC_i6100_modelist,
    &canon_BJC_i6100_paperlist,
    &canon_BJC_i6100_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i9100 */
    "i9100", 3,
    933, INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i9100_modelist,
    &canon_BJC_i9100_paperlist,
    &canon_BJC_i9100_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC i9900 */
    "i9900", 3,
    933, INCH(23), /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP900_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_P|CANON_CAP_M|CANON_CAP_BORDERLESS,0,/* ESC (M 0x0 0x0 0x0 */
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,
    &canon_BJC_i9900_modelist,
    &canon_BJC_i9900_paperlist,
    &canon_BJC_i9900_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },

  /***************/
  /* mini models */
  /***************/

  { /*  mini220 */
    "PIXMA mini220", 3,
    INCH(4), INCH(8), /* US 4" x 8" */       /* from MacOSX driver */
    10, 10, 9, 15, /* for hagaki: 3.4mm L/R, 3mm top, 5mm bottom */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,
    &canon_PIXMA_mini220_modelist,
    &canon_SELPHY_DS810_paperlist,
    &canon_PIXMA_mini220_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /*  mini320 */
    "PIXMA mini320", 3,
    INCH(5), INCH(8),       /* from MacOSX driver */
    10, 10, 9, 15, /* for hagaki: 3.4mm L/R, 3mm top, 5mm bottom */
    &canon_MULTIPASS_MP250_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,
    &canon_PIXMA_mini320_modelist,
    &canon_PIXMA_mini320_paperlist,
    &canon_PIXMA_mini320_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },


  /*************/
  /* DS models */
  /*************/

  { /* Canon DS700 */
    "SELPHY DS700", 3,
    INCH(4), INCH(6), /* US 4" x 6" */       /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    2,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_SELPHY_DS700_modelist,
    &canon_SELPHY_DS700_paperlist,
    &canon_SELPHY_DS700_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon DS810 */
    "SELPHY DS810", 3,
    INCH(4), INCH(8), /* US 4" x 8" */       /* from MacOSX driver */
    10, 10, 9, 15, /* confirmed */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_SELPHY_DS810_modelist,
    &canon_SELPHY_DS810_paperlist,
    &canon_SELPHY_DS810_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },

  /*****************************/
  /*                           */
  /*  extremely fuzzy models   */
  /* (might never work at all) */
  /*                           */
  /*****************************/

  { /* Canon BJC 5100 *//* heads: BC-20 BC-21 BC-22 BC-23 BC-29 */
    "5100", 1,
    INCH(17), INCH(22),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    &canon_BJC_3000_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 5500 *//* heads: BC-20 BC-21 BC-29 */
    "5500", 1,
    INCH(22), INCH(34),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0 | CANON_CAP_a,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_5500_modelist,
    &canon_default_paperlist,
    &canon_BJC_5500_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 6500 *//* heads: BC-30/BC-31 BC-32/BC-31 */
    "6500", 3,
    INCH(17), INCH(22),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD1 | CANON_CAP_a,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_3000_modelist,
    &canon_default_paperlist,
    &canon_BJC_3000_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon BJC 8500 *//* heads: BC-80/BC-81 BC-82/BC-81 */
    "8500", 3,
    INCH(17), INCH(22),
    11, 9, 10, 18, /* unconfirmed */
    &canon_default_slotlist,
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    NULL,
    &canon_BJC_8500_modelist,
    &canon_default_paperlist,
    &canon_BJC_8500_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP90, iP90v */
    "PIXMA iP90", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */        /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_NOBLACK|CANON_CAP_S|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_iP90_modelist,
    &canon_PIXMA_iP90_paperlist,
    &canon_PIXMA_iP90_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP100 */
    "PIXMA iP100", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver, and linux driver v3.70 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_NOBLACK|CANON_CAP_S|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_iP100_modelist,
    &canon_PIXMA_iP100_paperlist,
    &canon_PIXMA_iP100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP110 */
    /* 2 ink carts: (1) CMYK (2) pigment black */
    /* special inksaving options to save ink and/or use only remaining ink: */
    /* (not exclusive): black-saving mode, composite black, black-saving + composite black both active */
    "PIXMA iP110", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_XML|CANON_CAP_v|CANON_CAP_BORDERLESS,0,
    2,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG3500,
    &canon_PIXMA_iP110_modelist,
    &canon_PIXMA_iP110_paperlist,
    &canon_PIXMA_iP110_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
#if 0
  { /* Canon PIXMA iP1000 */
    "PIXMA iP1000", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1000_modelist,
    &canon_PIXMA_iP1000_paperlist,
    &canon_PIXMA_iP1000_modeuselist,/*not created yet*/
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP1200 --- iP1300 same */
    "PIXMA iP1200", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1200_modelist,
    &canon_PIXMA_iP1200_paperlist,
    &canon_PIXMA_iP1200_modeuselist,/*not created yet*/
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP1500 */
    "PIXMA iP1500", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1500_modelist,
    &canon_PIXMA_iP1500_paperlist,
    &canon_PIXMA_iP1500_modeuselist,/*not created yet*/
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP1600 */
    "PIXMA iP1600", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4000,  /*features */
    &canon_MULTIPASS_MP150_modelist,
    &canon_PIXMA_iP1500_paperlist,
    &canon_PIXMA_iP1600_modeuselist,/*not created yet*/
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP1900 */
    "PIXMA iP1900", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* rear only */
    CANON_CAP_STD0|CANON_CAP_P,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1900_modelist,
    &canon_PIXMA_iP1900_paperlist,
    &canon_PIXMA_iP1900_modeuselist,/*not created yet*/
    NULL,
    NULL,
    NULL,
    NULL
  },
#endif
  { /* Canon PIXMA iP2000 */
    "PIXMA iP2000", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP2000_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_iP2000_modelist,
    &canon_PIXMA_iP2000_paperlist,
    &canon_PIXMA_iP2000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
#if 0
  { /* Canon PIXMA iP2200 */
    "PIXMA iP2200", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1900_modelist,
    &canon_PIXMA_iP2200_paperlist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP2500 */
    "PIXMA iP2500", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1900_modelist,
    &canon_PIXMA_iP2200_paperlist,/* OHP experimental */
    &canon_PIXMA_iP2500_modeuselist,/*not created yet*/
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* Canon PIXMA iP2600 */
    "PIXMA iP2600", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* auto sheet feeder only */
    CANON_CAP_STD0,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP1900_modelist,
    &canon_PIXMA_iP2600_paperlist,
    &canon_PIXMA_iP2600_modeuselist,/*not created yet*/
    NULL,
    NULL,
    NULL,
    NULL
  },
#endif
  { /* Canon PIXMA iP2700 */
    "PIXMA iP2700", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.30 (from MacOSX driver only INCH(23) ) */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* rear only */
    CANON_CAP_STD0|CANON_CAP_P|CANON_CAP_I|CANON_CAP_px|CANON_CAP_T|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_iP2700_modelist,
    &canon_PIXMA_iP2700_paperlist,
    &canon_PIXMA_iP2700_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP2800 */
    "PIXMA iP2800", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* not confirmed yet */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* rear only */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_v|CANON_CAP_XML,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /*features */
    &canon_PIXMA_MG2400_modelist,
    &canon_PIXMA_MG2900_paperlist,
    &canon_PIXMA_MG2900_modeuselist, /* incorporate new media */
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP3000 */
    "PIXMA iP3000", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP3100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP3000_modelist,
    &canon_PIXMA_iP3000_paperlist,
    &canon_PIXMA_iP3000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP3100 */
    "PIXMA iP3100", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP3100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP3100_modelist,
    &canon_PIXMA_iP3100_paperlist,
    &canon_PIXMA_iP3100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP3300 */
    "PIXMA iP3300", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP3500_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP520_modelist,
    &canon_PIXMA_iP3300_paperlist,
    &canon_PIXMA_iP3300_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP3500 - like MP520 */
    "PIXMA iP3500", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP3500_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP520_modelist,
    &canon_MULTIPASS_MP520_paperlist,
    &canon_MULTIPASS_MP520_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP3600 */
    "PIXMA iP3600", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP3600_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_r|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP540_modelist,
    &canon_PIXMA_iP3600_paperlist,
    &canon_PIXMA_iP3600_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4000 */
    "PIXMA iP4000", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4000,  /*features */
    &canon_PIXMA_iP4000_modelist,
    &canon_PIXMA_iP4000_paperlist,
    &canon_PIXMA_iP4000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4100 */
    "PIXMA iP4100", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP4100_modelist,
    &canon_PIXMA_iP4100_paperlist,
    &canon_PIXMA_iP4100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4200 */
    "PIXMA iP4200", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP3100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP4200_modelist,
    &canon_PIXMA_iP4200_paperlist,
    &canon_PIXMA_iP4200_modeuselist,
    iP4200_lum_adjustment,
    iP4200_hue_adjustment,
    iP4200_sat_adjustment,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4300 */
    "PIXMA iP4300", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP4300_modelist, /* slight differences to iP4200 */
    &canon_PIXMA_iP4200_paperlist,
    &canon_PIXMA_iP4300_modeuselist,
    iP4200_lum_adjustment,
    iP4200_hue_adjustment,
    iP4200_sat_adjustment,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4500 CD-R tray F */
    "PIXMA iP4500", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4500_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /* features */
    &canon_PIXMA_iP4500_modelist,
    &canon_PIXMA_iP4500_paperlist,
    &canon_PIXMA_iP4500_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4600 */
    "PIXMA iP4600", 3,          /*model, model_id*/
    INCH(17/2), 1917, /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4600_slotlist, /* iP4600 uses ESC (r 0x64 at reset followed by 0x65 later for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_P|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP4600_modelist,
    &canon_PIXMA_iP4600_paperlist,
    &canon_PIXMA_iP4600_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4700 CD-R tray G */
    "PIXMA iP4700", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.20 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4600_slotlist, /* iP4700 uses ESC (r 0x64 at reset followed by 0x65 later for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0x64,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /* features */
    &canon_PIXMA_iP4700_modelist,
    &canon_PIXMA_iP4700_paperlist,
    &canon_PIXMA_iP4700_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4800 CD-R tray G */
    "PIXMA iP4800", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.40 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4600_slotlist, /* iP4800 uses ESC (r 0x64 at reset followed by 0x68 later for CD media only */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0x64,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /* features */
    &canon_PIXMA_iP4700_modelist,/* same for iP4800 */
    &canon_PIXMA_iP4700_paperlist,/* same for iP4800 */
    &canon_PIXMA_iP4700_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP4900 CD-R tray G */
    "PIXMA iP4900", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.60 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4600_slotlist, /* iP4900 uses ESC (r 0x68 command for CD tray only */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features Uses ESC (r only for CD media */
    &canon_PIXMA_iP4900_modelist,
    &canon_PIXMA_MG5200_paperlist,
    &canon_PIXMA_iP4900_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP5000 */
    "PIXMA iP5000", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP5000_modelist,
    &canon_PIXMA_iP5000_paperlist,
    &canon_PIXMA_iP5000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP5300 */
    "PIXMA iP5300", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP3100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP5300_modelist,
    &canon_MULTIPASS_MP600_paperlist,
    &canon_PIXMA_iP5300_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP6000D */
    "PIXMA iP6000", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_P|CANON_CAP_I|CANON_CAP_px|CANON_CAP_BORDERLESS,0x64,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP6000_modelist,
    &canon_PIXMA_iP4000_paperlist,
    &canon_PIXMA_iP6000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP6100D */
    "PIXMA iP6100", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_P|CANON_CAP_I|CANON_CAP_px|CANON_CAP_BORDERLESS,0x64,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP6000_modelist,
    &canon_PIXMA_iP6100_paperlist,
    &canon_PIXMA_iP6100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP6210D */
    "PIXMA iP6210", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* has ESC (T */
    CANON_CAP_STD0|CANON_CAP_T|CANON_CAP_NOBLACK|CANON_CAP_P|CANON_CAP_I|CANON_CAP_px|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_iP6210_modelist,
    &canon_PIXMA_iP90_paperlist,
    &canon_PIXMA_iP6210_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP6600D */
    "PIXMA iP6600", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_P|CANON_CAP_I|CANON_CAP_r|CANON_CAP_px|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP6600_modelist,
    &canon_PIXMA_iP6600_paperlist,
    &canon_PIXMA_iP6600_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP6700D */
    "PIXMA iP6700", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_P|CANON_CAP_I|CANON_CAP_r|CANON_CAP_px|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP6700_modelist,
    &canon_PIXMA_iP6700_paperlist,
    &canon_PIXMA_iP6700_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP7200 CD-R tray J */
    "PIXMA iP7200", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* from linux driver v3.80 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5400_slotlist, /* iP7200 uses ESC (r 0x68 command for CD tray only */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_v|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    1, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features Uses ESC (r only for CD media */
    &canon_PIXMA_iP7200_modelist,
    &canon_PIXMA_MG5400_paperlist,
    &canon_PIXMA_iP7200_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iX4000 --- like iX5000 but includes Transparency and ud1 mode is different */
    "PIXMA iX4000", 3,          /*model, model_id*/
    933, INCH(23),       /* max paper width and height */       /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_iX4000_modelist,
    &canon_PIXMA_iP1500_paperlist,
    &canon_PIXMA_iX4000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iX5000 */
    "PIXMA iX5000", 3,          /*model, model_id*/
    933, INCH(23),       /* max paper width and height */       /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_iX5000_modelist,
    &canon_PIXMA_iP1500_paperlist,
    &canon_PIXMA_iX5000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iX6500 --- with XML */
    "PIXMA iX6500", 3,          /*model, model_id*/
    933, 1917,       /* max paper width and height */       /* from linux v3.50 driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP540_modelist,
    &canon_MULTIPASS_MX880_paperlist,
    &canon_PIXMA_iX6500_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iX6800 --- with XML */
    "PIXMA iX6800", 3,          /*model, model_id*/
    933, 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG3500,  /*features */
    &canon_PIXMA_iX6800_modelist,
    &canon_PIXMA_iX6800_paperlist,
    &canon_PIXMA_iX6800_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iX7000 --- with XML */
    "PIXMA iX7000", 3,          /*model, model_id*/
    933, INCH(23),       /* max paper width and height */       /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iX7000_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_DUPLEX|CANON_CAP_I|CANON_CAP_r|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0x64,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iX7000_modelist,
    &canon_PIXMA_iX7000_paperlist,
    &canon_PIXMA_iX7000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP510 --- like MP520 but without PPGgold paper support */
    "PIXMA MP510", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP520_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP520_modelist,
    &canon_MULTIPASS_MP520_paperlist, /* Windows driver lacks PPGgold, but let us try anyway */
    &canon_MULTIPASS_MP520_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP520 */
    "PIXMA MP520", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP520_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP520_modelist,
    &canon_MULTIPASS_MP520_paperlist,
    &canon_MULTIPASS_MP520_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP530 */
    "PIXMA MP530", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP530_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_r|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP530_modelist,
    &canon_MULTIPASS_MP600_paperlist,
    &canon_MULTIPASS_MP530_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP540 */
    "PIXMA MP540", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_r|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP540_modelist,
    &canon_MULTIPASS_MX330_paperlist,
    &canon_MULTIPASS_MP540_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP550 ---with XML */
    "PIXMA MP550", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.20 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_r|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0x64,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP550_modelist, /* similar to MP540 but fewer modes */
    &canon_MULTIPASS_MP250_paperlist,
    &canon_MULTIPASS_MP550_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP560 ---with XML */
    "PIXMA MP560", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.20 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_I|CANON_CAP_r|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0x64,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP560_modelist, /* like MP550 but has duplex */
    &canon_MULTIPASS_MP250_paperlist,
    &canon_MULTIPASS_MP560_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP600 */
    "PIXMA MP600", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MX850_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_P|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP600_modelist,
    &canon_MULTIPASS_MP600_paperlist,
    &canon_MULTIPASS_MP600_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP610 */
    "PIXMA MP610", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4500_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP610_modelist,
    &canon_MULTIPASS_MP610_paperlist,
    &canon_MULTIPASS_MP610_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP620 */
    "PIXMA MP620", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_r|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP540_modelist,
    &canon_MULTIPASS_MX330_paperlist,
    &canon_MULTIPASS_MP620_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP630 */
    "PIXMA MP630", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5200_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_r|CANON_CAP_DUPLEX|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP640_modelist,
    &canon_MULTIPASS_MP630_paperlist,
    &canon_MULTIPASS_MP630_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP640 */
    "PIXMA MP640", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.20 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5200_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_DUPLEX|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_r|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_MULTIPASS_MP640_modelist,
    &canon_MULTIPASS_MP640_paperlist,
    &canon_MULTIPASS_MP640_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP700/MP730 --- split from iP3000 driver, with modifications */
    /* US model: control_cmd_iP2700 */
    /* Japanese model: control_BJC_i550, so chose this as base (more commands) */
    "PIXMA MP700", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from user manual */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP900_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,0,
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i550,  /*features */
    &canon_MULTIPASS_MP700_modelist,
    &canon_MULTIPASS_MP700_paperlist,
    &canon_MULTIPASS_MP700_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL
  },
  { /* PIXMA MP710/MP740 --- heavily modified from MP700/MP730 */
    "PIXMA MP710", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from user manual */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP710_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP710_modelist,
    &canon_MULTIPASS_MP710_paperlist,
    &canon_MULTIPASS_MP710_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* PIXMA MP750/MP760/MP770/MP780/MP790 */
    "PIXMA MP750", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP3100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_iP4100_modelist,
    &canon_MULTIPASS_MP750_paperlist,
    &canon_MULTIPASS_MP750_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MP900 */
    "PIXMA MP900", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP900_slotlist, /* auto sheetfeeder amd CD tray only */
    CANON_CAP_STD0|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_px|CANON_CAP_P|CANON_CAP_BORDERLESS,0,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP900_modelist,
    &canon_MULTIPASS_MP900_paperlist, /* was: &canon_PIXMA_iP3100_paperlist, */
    &canon_MULTIPASS_MP900_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA Pro9000 */
    "PIXMA Pro9000", 3,          /*model, model_id*/
    INCH(14), INCH(23),       /* max paper width and height */       /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_Pro9000_slotlist,
    CANON_CAP_STD0|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_px|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_Pro9000_modelist,
    &canon_PIXMA_Pro9000_paperlist,
    &canon_PIXMA_Pro9000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA Pro9000 Mk.II */
    "PIXMA Pro9002", 3,          /*model, model_id*/
    INCH(14), 1917,       /* max paper width and height */       /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_Pro9000_slotlist,
    CANON_CAP_STD0|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_px|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_Pro9000mk2_modelist,
    &canon_PIXMA_Pro9000mk2_paperlist,
    &canon_PIXMA_Pro9000mk2_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA Pro9500 */
    "PIXMA Pro9500", 3,          /*model, model_id*/
    INCH(14), INCH(23),       /* max paper width and height */       /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_Pro9000_slotlist,
    CANON_CAP_STD0|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_px|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_Pro9500_modelist,
    &canon_PIXMA_Pro9500_paperlist,
    &canon_PIXMA_Pro9500_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA Pro9500 Mk.II */
    "PIXMA Pro9502", 3,          /*model, model_id*/
    INCH(14), 1917,       /* max paper width and height */       /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_Pro9000_slotlist,
    CANON_CAP_STD0|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_px|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_Pro9500mk2_modelist,
    &canon_PIXMA_Pro9500mk2_paperlist,
    &canon_PIXMA_Pro9500mk2_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP7100 */
    "PIXMA iP7100", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_I|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP7100_modelist,
    &canon_PIXMA_iP7100_limited_paperlist, /* was: &canon_PIXMA_iP7100_paperlist, */
    &canon_PIXMA_iP7100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP7500 */
    "PIXMA iP7500", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_I|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP7500_modelist,
    &canon_PIXMA_iP7500_paperlist,
    &canon_PIXMA_iP7500_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP8100 */
    "PIXMA iP8100", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_I|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP8100_modelist,
    &canon_PIXMA_iP7100_limited_paperlist, /* was: &canon_PIXMA_iP7100_paperlist,*/
    &canon_PIXMA_iP8100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP8500 */
    "PIXMA iP8500", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP8500_modelist,
    &canon_PIXMA_iP8500_limited_paperlist, /* was: &canon_PIXMA_iP8500_paperlist,*/
    &canon_PIXMA_iP8500_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP8600 */
    "PIXMA iP8600", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_I|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_P|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200,  /*features */
    &canon_PIXMA_iP8600_modelist,
    &canon_PIXMA_iP7100_limited_paperlist, /* was: &canon_PIXMA_iP7100_paperlist,*/
    &canon_PIXMA_iP8600_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP8700 CD-R tray L */
    "PIXMA iP8700", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_iP8700_slotlist, /* iP7200 uses ESC (r 0x68 command for CD tray only */
    CANON_CAP_STD0|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_v|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG3500,  /* features Uses ESC (r only for CD media */
    &canon_PIXMA_iP8700_modelist,
    &canon_PIXMA_iP8700_paperlist,
    &canon_PIXMA_iP8700_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA iP9910 */
    "PIXMA iP9910", 3,          /*model, model_id*/
    933, INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP900_slotlist,
    CANON_CAP_STD0|CANON_CAP_I|CANON_CAP_rr|CANON_CAP_px|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_iP9910_modelist,
    &canon_PIXMA_iP9910_limited_paperlist, /* was: &canon_PIXMA_iP9910_paperlist,*/
    &canon_PIXMA_iP9910_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
    { /* Canon PIXUS MP5, PIXUS MP10, SmartBase MPC190, SmartBase MPC200 */
      /* these printers declare themselves as USB mass storage devices
	 rather than printers, and require a special backend. Thus
	 they are not currently included in the supported printers */
    "PIXMA MP5", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* these models only have an auto sheet feeder */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,0, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_PIXMA_MP5_modelist,
    &canon_PIXMA_MP5_paperlist,
    &canon_PIXMA_MP5_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL /* KYMC */
  },
  { /* Canon PIXUS MP55 */
    /* Unknown if requires special backend or not, left in supported
       printers as experimental */
    "PIXMA MP55", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* these models only have an auto sheet feeder */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,0, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i550,  /*features */
    &canon_PIXMA_MP55_modelist,
    &canon_PIXMA_MP55_paperlist,
    &canon_PIXMA_MP55_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL /* KYMC */
  },
  { /* Canon PIXMA MPC400, MPC600F */
    "PIXMA MPC400", 3,          /* model, model_id */
    INCH(17/2), INCH(23),       /* max paper width and height: assumed */
    10, 10, 9, 15,    /* border_left, border_right, border_top, border_bottom: assumed */
    &canon_MULTIPASS_MP150_slotlist, /* these models only have an auto sheet feeder */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_BORDERLESS,0, /* borderless not supported in Windows driver, adding experimentally */
    2,0, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_BJC_i6100,  /* features */
    &canon_PIXMA_MPC400_modelist,
    &canon_PIXMA_MPC400_paperlist,
    &canon_PIXMA_MPC400_modeuselist,
    NULL,
    NULL,
    NULL,
    NULL /* KYMC */
  },
  { /* Canon MULTIPASS MP150 */
    "PIXMA MP150", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP150_modelist,
    &canon_MULTIPASS_MP150_paperlist,
    &canon_MULTIPASS_MP150_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP190 */
    "PIXMA MP190", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP190_modelist,
    &canon_MULTIPASS_MP190_paperlist,
    &canon_MULTIPASS_MP190_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP210 */
    "PIXMA MP210", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP210_modelist,
    &canon_MULTIPASS_MP150_paperlist,
    &canon_MULTIPASS_MP210_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP360/370/375R/390 */
    "PIXMA MP360", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from user manual */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* these models only have an auto sheet feeder */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,2, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP360_modelist,
    &canon_MULTIPASS_MP360_paperlist,
    &canon_MULTIPASS_MP360_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP450 */
    "PIXMA MP450", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP150_modelist,
    &canon_MULTIPASS_MP150_paperlist,
    &canon_MULTIPASS_MP150_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP470 --- no XML */
    "PIXMA MP470", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP470_modelist,
    &canon_MULTIPASS_MP470_paperlist,
    &canon_MULTIPASS_MP470_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP480 --- no XML */
    "PIXMA MP480", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP480_modelist,
    &canon_MULTIPASS_MP480_paperlist,
    &canon_MULTIPASS_MP480_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP490 --- with XML */
    "PIXMA MP490", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.20 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP490_modelist,
    &canon_MULTIPASS_MP493_paperlist,
    &canon_MULTIPASS_MP490_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP493 --- with XML */
    "PIXMA MP493", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP480_modelist,
    &canon_MULTIPASS_MP493_paperlist,
    &canon_MULTIPASS_MP493_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP495 identical to MP280 it seems --- with XML */
    "PIXMA MP495", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.40 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP280_modelist,
    &canon_MULTIPASS_MP280_paperlist,
    &canon_MULTIPASS_MP280_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP230 --- with XML no borderless but leave it in as untested */
    "PIXMA MP230", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.80 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* these models only have a rear tray. Also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_T|CANON_CAP_XML|CANON_CAP_BORDERLESS,0, /* borderless not in Windows driver---untested */
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /*features */
    &canon_MULTIPASS_MP230_modelist,
    &canon_PIXMA_MG3200_paperlist,
    &canon_MULTIPASS_MP230_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP240 */
    "PIXMA MP240", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* these models only have a rear tray. Also uses CAP_T  */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_T|CANON_CAP_BORDERLESS,0,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP240_modelist,
    &canon_MULTIPASS_MP240_paperlist,
    &canon_MULTIPASS_MP240_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP250 --- with XML */
    "PIXMA MP250", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.20 & v3.40 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* these models only have a rear tray. Also uses CAP_T  */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_T|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP250_modelist,
    &canon_MULTIPASS_MP250_paperlist,
    &canon_MULTIPASS_MP250_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP270 --- with XML */
    "PIXMA MP270", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.20 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* these models only have a rear tray. Also uses CAP_T  */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_T|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP250_modelist,
    &canon_MULTIPASS_MP250_paperlist,
    &canon_MULTIPASS_MP250_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MP280 --- with XML */
    "PIXMA MP280", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.40 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* these models only have a rear tray. Also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_T|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MP280_modelist,
    &canon_MULTIPASS_MP280_paperlist,
    &canon_MULTIPASS_MP280_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG2100 */
    "PIXMA MG2100", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.60 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features Also uses CAP_T */
    &canon_PIXMA_MG2100_modelist,
    &canon_PIXMA_MG5100_paperlist,
    &canon_PIXMA_MG2100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG2200 */
    "PIXMA MG2200", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.80 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features Also uses CAP_T */
    &canon_PIXMA_MG2100_modelist,
    &canon_PIXMA_MG3200_paperlist,
    &canon_PIXMA_MG2200_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG2400 */
    "PIXMA MG2400", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* not confirmed yet */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_v|CANON_CAP_XML,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features Also uses CAP_T */
    &canon_PIXMA_MG2400_modelist,
    &canon_PIXMA_MG2400_paperlist,
    &canon_PIXMA_MG2400_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG2900 */
    "PIXMA MG2900", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* not confirmed yet */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_v|CANON_CAP_XML,0,
    2,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features Also uses CAP_T */
    &canon_PIXMA_MG2400_modelist,
    &canon_PIXMA_MG2900_paperlist,
    &canon_PIXMA_MG2900_modeuselist, /* incorporate new media */
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG3100 */
    "PIXMA MG3100", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.60 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features Also uses CAP_T */
    &canon_PIXMA_MG3100_modelist,
    &canon_PIXMA_MG5100_paperlist,
    &canon_PIXMA_MG3100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG3200 */
    "PIXMA MG3200", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.80 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features: also uses CAP_T */
    &canon_PIXMA_MG3100_modelist,
    &canon_PIXMA_MG3200_paperlist,
    &canon_PIXMA_MG3200_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG3500 */
    "PIXMA MG3500", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* unconfirmed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG3500,  /* features: also uses CAP_T */
    &canon_PIXMA_MG3500_modelist, /* same as MG3100 but try to use inktypes to control use of inks in inksets */
    &canon_PIXMA_MG3200_paperlist, /* Canon Photo Hagaki changed to merely Photo Hagaki in Windows driver */
    &canon_PIXMA_MG3500_modeuselist,/* same as MG3200 but try to use inktypes to control use of inks in inksets */
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG3600 */
    /* no support for Esc (s and ESC (u commands yet for duplex */
    "PIXMA MG3600", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* unconfirmed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_T|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG3500,  /* features: also uses CAP_T */
    &canon_PIXMA_MG3600_modelist,
    &canon_PIXMA_MG3600_paperlist, /* Address side media added for inkjet and photo Hagaki, name of HagakiA changed to Hagaki (A) */
    &canon_PIXMA_MG3600_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG5100 */
    "PIXMA MG5100", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.40 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /* features */
    &canon_PIXMA_MG5100_modelist,
    &canon_PIXMA_MG5100_paperlist,
    &canon_PIXMA_MG5100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG5200 --- like MG5100, plus CD tray */
    "PIXMA MG5200", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.40 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5200_slotlist, /* ESC (r only for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /* features */
    &canon_PIXMA_MG5200_modelist,
    &canon_PIXMA_MG5200_paperlist,
    &canon_PIXMA_MG5200_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG5300 --- like MG5200 */
    "PIXMA MG5300", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.60 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5200_slotlist, /* ESC (r only for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features */
    &canon_PIXMA_MG5300_modelist,
    &canon_PIXMA_MG5200_paperlist,
    &canon_PIXMA_MG5300_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG5400 */
    "PIXMA MG5400", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* from linux drver v3.80 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5400_slotlist, /* ESC (r only for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_v|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    1, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features */
    &canon_PIXMA_MG5400_modelist,
    &canon_PIXMA_MG5400_paperlist,
    &canon_PIXMA_MG5400_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG5500 */
    "PIXMA MG5500", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* not confirmed yet */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_v|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG3500,
    &canon_PIXMA_MG5500_modelist,
    &canon_PIXMA_MG3200_paperlist, /* Canon Photo Hagaki changed to merely Photo Hagaki in Windows driver */
    &canon_PIXMA_MG5500_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG5600 */
    /* no support for Esc (s and ESC (u commands yet for duplex */
    "PIXMA MG5600", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* not confirmed yet */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MX7600_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_v|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5600,
    &canon_PIXMA_MG5500_modelist,
    &canon_MAXIFY_iB4000_paperlist, /* Canon Photo Hagaki changed to merely Photo Hagaki in Windows driver */
    &canon_PIXMA_MG5600_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG5700 */
    /* no support for Esc (s and ESC (u commands yet for duplex */
    "PIXMA MG5700", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* not confirmed yet */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MX7600_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_v|CANON_CAP_w|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5600,
    &canon_PIXMA_MG5700_modelist,
    &canon_PIXMA_MG3600_paperlist, /* new media types September 2015 */
    &canon_PIXMA_MG5700_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG6100 */
    "PIXMA MG6100", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.40 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5200_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0x64,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /* features */
    &canon_PIXMA_MG6100_modelist,
    &canon_PIXMA_MG6100_paperlist,
    &canon_PIXMA_MG6100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG6200 */
    "PIXMA MG6200", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.60 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5200_slotlist, /* ESC (r only for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features */
    &canon_PIXMA_MG6200_modelist,
    &canon_PIXMA_MG6100_paperlist,
    &canon_PIXMA_MG6200_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG6300 */
    "PIXMA MG6300", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* from linux driver v3.80 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5400_slotlist, /* ESC (r only for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_v|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    1, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features */
    &canon_PIXMA_MG6300_modelist,
    &canon_PIXMA_MG6300_paperlist,
    &canon_PIXMA_MG6300_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG6500 */
    "PIXMA MG6500", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* not confirmed yet */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5400_slotlist, /* ESC (r only for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_v|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    1, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG3500,
    &canon_PIXMA_MG6500_modelist,
    &canon_PIXMA_MG6300_paperlist,
    &canon_PIXMA_MG6500_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG6700 */
    "PIXMA MG6700", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* not confirmed yet */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5400_slotlist, /* ESC (r only for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_v|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    1, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG3500,
    &canon_PIXMA_MG6500_modelist,
    &canon_PIXMA_iP8700_paperlist,
    &canon_PIXMA_MG6700_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG7500 */
    "PIXMA MG7500", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* not confirmed yet */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5400_slotlist, /* ESC (r only for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_v|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    1, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5600,
    &canon_PIXMA_MG6500_modelist,
    &canon_PIXMA_iP8700_paperlist,
    &canon_PIXMA_MG6700_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG7700 */
    "PIXMA MG7700", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* not confirmed yet */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5400_slotlist, /* ESC (r only for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_v|CANON_CAP_w|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    1, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5600,
    &canon_PIXMA_MG7700_modelist,
    &canon_PIXMA_MG7700_paperlist,
    &canon_PIXMA_MG7700_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG8100 */
    "PIXMA MG8100", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.40 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5200_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0x64,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */    
    control_cmd_PIXMA_iP2700,  /* features */
    &canon_PIXMA_MG8100_modelist,
    &canon_PIXMA_MG6100_paperlist,
    &canon_PIXMA_MG8100_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA MG8200 */
    "PIXMA MG8200", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.60 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5200_slotlist, /* ESC (r only for CD media */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_r|CANON_CAP_rr|CANON_CAP_I|CANON_CAP_P|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features */
    &canon_PIXMA_MG8200_modelist,
    &canon_PIXMA_MG6100_paperlist,
    &canon_PIXMA_MG8200_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  /* MX series */
  { /* Canon MULTIPASS MX300 --- MX310 is the same */
    "PIXMA MX300", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    2,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MX300_modelist,
    &canon_MULTIPASS_MX300_paperlist,
    &canon_MULTIPASS_MX300_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX330 */
    "PIXMA MX330", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MX330_modelist,
    &canon_MULTIPASS_MX330_paperlist,
    &canon_MULTIPASS_MX330_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX340 --- with XML*/
    "PIXMA MX340", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.30 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP150_slotlist, /* these models only have an auto sheet feeder also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MX340_modelist,
    &canon_MULTIPASS_MX340_paperlist,
    &canon_MULTIPASS_MX340_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX360 -- with XML */
    "PIXMA MX360", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.50 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MX360_modelist,
    &canon_MULTIPASS_MX360_paperlist,
    &canon_MULTIPASS_MX360_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX370 --- with XML */
    "PIXMA MX370", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.70 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist, /* only front tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,8, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,
    &canon_MULTIPASS_MX370_modelist,
    &canon_MULTIPASS_MX420_paperlist,
    &canon_MULTIPASS_MX370_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX390 --- with XML */
    "PIXMA MX390", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.90 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist, /* only front tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,
    &canon_MULTIPASS_MX370_modelist,
    &canon_PIXMA_MG3200_paperlist,
    &canon_MULTIPASS_MX390_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX420 --- with XML */
    "PIXMA MX420", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.50 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700,  /*features */
    &canon_MULTIPASS_MX420_modelist,
    &canon_MULTIPASS_MX420_paperlist,
    &canon_MULTIPASS_MX420_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX470 --- with XML */
    "PIXMA MX470", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist, /* only rear tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_v|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG3500,  /*features */
    &canon_MULTIPASS_MX470_modelist,
    &canon_PIXMA_iX6800_paperlist,
    &canon_MULTIPASS_MX470_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX490 --- with XML */
    "PIXMA MX490", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MX7600_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5600,
    &canon_MULTIPASS_E480_modelist,
    &canon_MULTIPASS_E480_paperlist,
    &canon_MULTIPASS_E480_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },    
  { /* Canon MULTIPASS MX510 --- with XML */
    "PIXMA MX510", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.70 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist, /* only front tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,8, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,
    &canon_MULTIPASS_MX510_modelist,
    &canon_MULTIPASS_MX420_paperlist,
    &canon_MULTIPASS_MX510_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX520 --- with XML */
    "PIXMA MX520", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.90 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist, /* only front tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,
    &canon_MULTIPASS_MX510_modelist,
    &canon_PIXMA_MG3200_paperlist,
    &canon_MULTIPASS_MX520_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX530 --- with XML */
    "PIXMA MX530", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist, /* only front tray also uses CAP_T */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG3500,
    &canon_PIXMA_MG3500_modelist,
    &canon_PIXMA_iX6800_paperlist,
    &canon_MULTIPASS_MX530_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX700 */
    "PIXMA MX700", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP520_slotlist, /* front, rear, button, and front/plain-only */
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700, /*features*/
    &canon_MULTIPASS_MP520_modelist,
    &canon_MULTIPASS_MX300_paperlist,
    &canon_MULTIPASS_MX700_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX710 --- with XML */
    "PIXMA MX710", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.70 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,8, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300, /*features*/
    &canon_MULTIPASS_MX710_modelist,
    &canon_MULTIPASS_MX420_paperlist,
    &canon_MULTIPASS_MX710_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX720 --- with XML */
    "PIXMA MX720", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* from linux driver v3.90 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MX720_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    1, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,
    &canon_PIXMA_MX720_modelist,
    &canon_PIXMA_MG3200_paperlist,
    &canon_PIXMA_MX720_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX850 */
    "PIXMA MX850", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MX850_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200, /*features*/
    &canon_PIXMA_iP4500_modelist, /* same inksets as iP4500 */
    &canon_MULTIPASS_MX850_paperlist,
    &canon_MULTIPASS_MX850_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX860 */
    "PIXMA MX860", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200, /*features*/
    &canon_MULTIPASS_MX860_modelist,
    &canon_MULTIPASS_MX330_paperlist,
    &canon_MULTIPASS_MX860_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX870 --- with XML */
    "PIXMA MX870", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.30 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0x64,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700, /*features*/
    &canon_MULTIPASS_MX860_modelist,
    &canon_MULTIPASS_MX340_paperlist,
    &canon_MULTIPASS_MX870_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX880 --- with XML */
    "PIXMA MX880", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.50 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5100_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP2700, /*features*/
    &canon_MULTIPASS_MX880_modelist,
    &canon_MULTIPASS_MX880_paperlist,
    &canon_MULTIPASS_MX880_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX920 --- with XML CD-R tray J */
    "PIXMA MX920", 3,          /*model, model_id*/
    INCH(17/2), INCH(14),       /* max paper width and height */ /* from linux driver v3.90 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG5400_slotlist, /* iP7200 uses ESC (r 0x68 command for CD tray only */
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    1, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,  /* features Uses ESC (r only for CD media */
    &canon_PIXMA_MX920_modelist,
    &canon_PIXMA_MG5400_paperlist,
    &canon_PIXMA_MX920_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS MX7600 */
    "PIXMA MX7600", 3,          /*model, model_id*/
    INCH(17/2), INCH(23),       /* max paper width and height */ /* from MacOSX driver */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MX7600_slotlist,
    CANON_CAP_STD0|CANON_CAP_DUPLEX|CANON_CAP_r|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_BORDERLESS,0x64,
    3,4, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_iP4200, /*features*/
    &canon_MULTIPASS_MX7600_modelist,
    &canon_MULTIPASS_MX7600_paperlist,
    &canon_MULTIPASS_MX7600_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS E400 --- with XML */
    "PIXMA E400", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,
    &canon_PIXMA_P200_modelist,
    &canon_MULTIPASS_E400_paperlist,
    &canon_MULTIPASS_E400_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS E460 --- with XML */
    "PIXMA E460", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML,0,
    2,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300,
    &canon_PIXMA_P200_modelist,
    &canon_MULTIPASS_E400_paperlist,
    &canon_MULTIPASS_E400_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS E480 --- with XML */
    "PIXMA E480", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MX7600_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5600,
    &canon_MULTIPASS_E480_modelist,
    &canon_MULTIPASS_E480_paperlist,
    &canon_MULTIPASS_E480_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },  
  { /* Canon MULTIPASS E500 --- with XML */
    "PIXMA E500", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.60 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,6, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300, /*features*/
    &canon_MULTIPASS_E500_modelist,
    &canon_MULTIPASS_MX880_paperlist,
    &canon_MULTIPASS_E500_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS E510 --- with XML */
    "PIXMA E510", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.80 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300, /*features*/
    &canon_MULTIPASS_E500_modelist,
    &canon_PIXMA_MG3200_paperlist,
    &canon_MULTIPASS_E510_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS E560 --- with XML */
    "PIXMA E560", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_DUPLEX|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300, /*features*/
    &canon_MULTIPASS_E560_modelist,
    &canon_PIXMA_iX6800_paperlist,
    &canon_MULTIPASS_E560_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS E600 --- with XML */
    "PIXMA E600", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.70 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,8, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300, /*features*/
    &canon_MULTIPASS_E500_modelist,
    &canon_MULTIPASS_MX880_paperlist,
    &canon_MULTIPASS_E500_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MULTIPASS E610 --- with XML */
    "PIXMA E610", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height */ /* from linux driver v3.80 */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_PIXMA_MG2100_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300, /*features*/
    &canon_MULTIPASS_E500_modelist,
    &canon_PIXMA_MG3200_paperlist,
    &canon_MULTIPASS_E510_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon PIXMA P200 --- with XML */
    "PIXMA P200", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MP250_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_T|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML|CANON_CAP_BORDERLESS,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300, /*features*/
    &canon_PIXMA_P200_modelist,
    &canon_PIXMA_P200_paperlist,
    &canon_PIXMA_P200_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MAXIFY iB4000 --- with XML */
    /* no support for Esc (s and ESC (u commands yet for duplex */
    "MAXIFY iB4000", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MAXIFY_iB4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300, /*features*/
    &canon_MAXIFY_iB4000_modelist,
    &canon_MAXIFY_iB4000_paperlist,
    &canon_MAXIFY_iB4000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MAXIFY MB2000 --- with XML */
    /* Same: MB5000 */
    /* no support for Esc (s and ESC (u commands yet for duplex */
    "MAXIFY MB2000", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MULTIPASS_MX7600_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300, /*features*/
    &canon_MAXIFY_iB4000_modelist,
    &canon_MAXIFY_iB4000_paperlist,
    &canon_MAXIFY_iB4000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
  { /* Canon MAXIFY MB2300 --- with XML */
    /* Same: MB5300 */
    /* no support for Esc (s and ESC (u commands yet for duplex */
    "MAXIFY MB2300", 3,          /*model, model_id*/
    INCH(17/2), 1917,       /* max paper width and height assumed */
    10, 10, 9, 15,    /*border_left, border_right, border_top, border_bottom */
    &canon_MAXIFY_iB4000_slotlist,
    CANON_CAP_STD0|CANON_CAP_px|CANON_CAP_P|CANON_CAP_I|CANON_CAP_v|CANON_CAP_XML,0,
    3,9, /* ESC (l and (P command lengths */
    0, /* Upper/Lower Cassette option */
    control_cmd_PIXMA_MG5300, /*features*/
    &canon_MAXIFY_iB4000_modelist,
    &canon_MAXIFY_iB4000_paperlist,
    &canon_MAXIFY_iB4000_modeuselist,
    NULL,
    NULL,
    NULL,
    iP4500_channel_order
  },
};

#endif

