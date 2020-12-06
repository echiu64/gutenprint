/*
 *
 *   Print plug-in CANON BJL driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Andy Thaller (thaller@ph.tum.de)
 *
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
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
#include <math.h>

#include "print-canon.h"

#ifndef MIN
#  define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif /* !MIN */
#ifndef MAX
#  define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* !MAX */

/* 5 3-level pixels into 1 byte */
static int
pack_pixels(unsigned char* buf,int len)
{
  int read_pos = 0;
  int write_pos = 0;
  int shift = 6;
  while(read_pos < len)
  {
    /* read 5pixels at 2 bit */
    unsigned short value = buf[read_pos] << 8;
    if(read_pos+1 < len)
      value += buf[read_pos + 1];
    if(shift)       /*6,4,2,0*/
      value >>= shift;
    /* write 8bit value representing the 10 bit pixel combination */
    buf[write_pos] = tentoeight[value & 1023];
    ++write_pos;
    if(shift == 0)
    {
      shift = 6;
      read_pos += 2;
    }
    else
    {
      shift -= 2;
      ++read_pos;
    }
  }
  return write_pos;
}

/* 3 4-bit-per-pixel  5-level pixels into 1 byte */
static int
pack_pixels3_5(unsigned char* buf,int len)
{
  int read_pos = 0;
  int write_pos = 0;
  int shift = 4;
  while(read_pos < len)
  {
    /* read 3pixels at 4 bit */
    unsigned short value = buf[read_pos] << 8;
    if(read_pos+1 < len)
      value += buf[read_pos + 1];
    if(shift)       /*4,0*/
      value >>= shift;
    /* write 8bit value representing the 12 bit pixel combination */
    buf[write_pos] = twelve2eight[value & 4095];
    ++write_pos;
    if(shift == 0)
    {
      shift = 4;
      read_pos += 2;
    }
    else
    {
      shift -= 4;
      ++read_pos;
    }
  }
  return write_pos;
}

/* 3 4-bit-per-pixel 6-level pixels into 1 byte */
static int
pack_pixels3_6(unsigned char* buf,int len)
{
  int read_pos = 0;
  int write_pos = 0;
  int shift = 4;
  while(read_pos < len)
  {
    /* read 3pixels at 4 bit */
    unsigned short value = buf[read_pos] << 8;
    if(read_pos+1 < len)
      value += buf[read_pos + 1];
    if(shift)       /*4,0*/
      value >>= shift;
    /* write 8bit value representing the 12 bit pixel combination */
    buf[write_pos] = twelve2eight2[value & 4095];
    ++write_pos;
    if(shift == 0)
    {
      shift = 4;
      read_pos += 2;
    }
    else
    {
      shift -= 4;
      ++read_pos;
    }
  }
  return write_pos;
}

/* model peculiarities */
#define CANON_CAP_MSB_FIRST  0x02ul    /* how to send data           */
#define CANON_CAP_a          0x04ul
#define CANON_CAP_b          0x08ul
#define CANON_CAP_q          0x10ul
#define CANON_CAP_m          0x20ul
#define CANON_CAP_d          0x40ul
#define CANON_CAP_t          0x80ul
#define CANON_CAP_c          0x100ul
#define CANON_CAP_p          0x200ul
#define CANON_CAP_l          0x400ul
#define CANON_CAP_r          0x800ul
#define CANON_CAP_g          0x1000ul
#define CANON_CAP_px         0x2000ul
#define CANON_CAP_rr         0x4000ul
#define CANON_CAP_I          0x8000ul
#define CANON_CAP_T          0x10000ul /* cartridge selection for PIXMA printers with Black and Color carts */
#define CANON_CAP_P          0x20000ul /* related to media type selection */
#define CANON_CAP_DUPLEX     0x40000ul /* duplex printing */
#define CANON_CAP_XML        0x80000ul /* use of XML at start and end of print data */
#define CANON_CAP_CARTRIDGE  0x100000ul /* not sure of this yet */
#define CANON_CAP_M          0x200000ul /* not sure of this yet */
#define CANON_CAP_S          0x400000ul /* not sure of this yet */
#define CANON_CAP_cart       0x800000ul /* BJC printers with Color, Black, Photo options */
#define CANON_CAP_BORDERLESS 0x1000000ul /* borderless (fullbleed) printing */
#define CANON_CAP_NOBLACK    0x2000000ul /* no Black cartridge selection */
#define CANON_CAP_v          0x4000000ul /* not sure of this yet */
#define CANON_CAP_w          0x8000000ul /* related to media type selection */
#define CANON_CAP_s          0x10000000ul /* not sure of this yet: duplex-related? */
#define CANON_CAP_u          0x20000000ul /* not sure of this yet: duplex-related? */

#define CANON_CAP_STD0 (CANON_CAP_b|CANON_CAP_c|CANON_CAP_d|\
                        CANON_CAP_l|CANON_CAP_q|CANON_CAP_t)

#define CANON_CAP_STD1 (CANON_CAP_b|CANON_CAP_c|CANON_CAP_d|CANON_CAP_l|\
                        CANON_CAP_m|CANON_CAP_p|CANON_CAP_q|CANON_CAP_t)

#include "canon-inks.h"
#include "canon-modes.h"
#include "canon-media.h"
#include "canon-media-mode.h"
#include "canon-printers.h"

typedef struct {
    char name;
    const canon_ink_t* props;
    unsigned char* buf;
    unsigned char* comp_buf_offset;
    unsigned int buf_length;
    unsigned int delay;
} canon_channel_t;

typedef struct
{
  const canon_mode_t* mode;
  const canon_slot_t* slot;
  const canon_paper_t *pt;
  /* cartridge selection for CANON_CAP_T and CANON_CAP_cart */
  const char *ink_set;
  const canon_modeuse_t* modeuse;
  /* final inks used for output, after selection process completed */
  unsigned int used_inks;
  int num_channels;
  int quality;
  canon_channel_t* channels;
  char* channel_order;
  const canon_cap_t *caps;
  unsigned char *comp_buf;
  unsigned char *fold_buf;
  int delay_max;
  int buf_length_max;
  int length;
  int out_width;
  int out_height;
  int page_width;
  int page_height;
  int top;
  int left;
  int emptylines;
  int ncolors; /* number of colors to print with */
  int physical_xdpi, nozzle_ydpi, stepper_ydpi;
  int nozzles;   /* count of inkjets for one pass */
  int nozzle_separation;
  int horizontal_passes;
  int vertical_passes;
  int vertical_oversample;
  int *head_offset;
  int last_pass_offset;
  int bidirectional; /* tells us if we are allowed to print bidirectional */
  int direction;     /* stores the last direction of the print head */
  int weave_bits[4];
  const char *duplex_str;
  int is_first_page;
  double cd_inner_radius;
  double cd_outer_radius;
} canon_privdata_t;

static const canon_modeuse_t* select_media_modes(stp_vars_t *v, const canon_paper_t* media_type,const canon_modeuselist_t* mlist);
static int compare_mode_valid(stp_vars_t *v,const canon_mode_t* mode,const canon_modeuse_t* muse, const canon_modeuselist_t* mlist);
static const canon_mode_t* suitable_mode_monochrome(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,int quality,const char *duplex_mode);
static const canon_mode_t* find_first_matching_mode_monochrome(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,const char *duplex_mode);
static const canon_mode_t* find_first_matching_mode(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,const char *duplex_mode);
static const canon_mode_t* suitable_mode_color(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,int quality,const char *duplex_mode);
static const canon_mode_t* find_first_matching_mode_color(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,const char *duplex_mode);
static const canon_mode_t* suitable_mode_photo(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,int quality,const char *duplex_mode);
static const canon_mode_t* find_first_matching_mode_photo(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,const char *duplex_mode);
static const canon_mode_t* suitable_mode_general(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,int quality,const char *duplex_mode);
static const char* find_ink_type(stp_vars_t *v,const canon_mode_t* mode,const char *printing_mode);
static const canon_mode_t* canon_check_current_mode(stp_vars_t *v);

static void canon_write_line(stp_vars_t *v);

static void canon_advance_paper(stp_vars_t *, int);
static void canon_flush_pass(stp_vars_t *, int, int);
static void canon_write_multiraster(stp_vars_t *v,canon_privdata_t* pd,int y);

static void fix_papersize(unsigned char arg_ESCP_1, int *paper_width, int *paper_length);

static const stp_parameter_t the_parameters[] =
{
  {
    "PageSize", N_("Page Size"), "Color=No,Category=Basic Printer Setup",
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MediaType", N_("Media Type"), "Color=Yes,Category=Basic Printer Setup",
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "InputSlot", N_("Media Source"), "Color=No,Category=Basic Printer Setup",
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CassetteTray", N_("Cassette Tray"), "Color=No,Category=Basic Printer Setup",
    N_("Tray selection for cassette media source"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDInnerRadius", N_("CD Hub Size"), "Color=No,Category=Basic Printer Setup",
    N_("Print only outside of the hub of the CD, or all the way to the hole"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDOuterDiameter", N_("CD Size (Custom)"), "Color=No,Category=Basic Printer Setup",
    N_("Variable adjustment for the outer diameter of CD"),
    STP_PARAMETER_TYPE_DIMENSION, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDInnerDiameter", N_("CD Hub Size (Custom)"), "Color=No,Category=Basic Printer Setup",
    N_("Variable adjustment to the inner hub of the CD"),
    STP_PARAMETER_TYPE_DIMENSION, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDXAdjustment", N_("CD Horizontal Fine Adjustment"), "Color=No,Category=Advanced Printer Setup",
    N_("Fine adjustment to horizontal position for CD printing"),
    STP_PARAMETER_TYPE_DIMENSION, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CDYAdjustment", N_("CD Vertical Fine Adjustment"), "Color=No,Category=Advanced Printer Setup",
    N_("Fine adjustment to horizontal position for CD printing"),
    STP_PARAMETER_TYPE_DIMENSION, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Resolution", N_("Resolution"), "Color=Yes,Category=Basic Printer Setup",
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  /*
   * Don't check this parameter.  We may offer different settings for
   * different ink sets, but we need to be able to handle settings from PPD
   * files that don't have constraints set up.
   */
  {
    "InkType", N_("Ink Type"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 0, 0
  },
  {
    "InkChannels", N_("Ink Channels"), "Color=Yes,Category=Advanced Printer Functionality",
    N_("Ink Channels"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  /*
   * Don't check this parameter.  We may offer different settings for
   * different ink sets, but we need to be able to handle settings from PPD
   * files that don't have constraints set up.
   */
  {
    "PrintingMode", N_("Printing Mode"), "Color=Yes,Category=Core Parameter",
    N_("Printing Output Mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 0, 0
  },
  /*
   * Don't check this parameter.  We may offer different settings for
   * different ink sets, but we need to be able to handle settings from PPD
   * files that don't have constraints set up.
   */
  {
    "InkSet", N_("Ink Set"), "Color=Yes,Category=Basic Printer Setup",
    N_("Type of inkset in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 0, 0
  },
  {
    "FullBleed", N_("Borderless"), "Color=No,Category=Basic Printer Setup",
    N_("Print without borders"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Duplex", N_("Double-Sided Printing"), "Color=No,Category=Basic Printer Setup",
    N_("Duplex/Tumble Setting"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Quality", N_("Print Quality"), "Color=Yes,Category=Basic Output Adjustment",
    N_("Print Quality"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 0, 0
  },
  {
    "Orientation", N_("Orientation"), "Color=No,Category=Basic Printer Setup",
    N_("Orientation, Portrait, Landscape, Upside Down, Seascape"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0,
  },
};

static const int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
  int color_only;
} float_param_t;

static const float_param_t float_parameters[] =
{
  {
    {
      "CyanDensity", N_("Cyan Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the cyan density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 1, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "MagentaDensity", N_("Magenta Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the magenta density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 2, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "YellowDensity", N_("Yellow Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the yellow density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 3, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "BlackDensity", N_("Black Density"), "Color=Yes,Category=Output Level Adjustment",
      N_("Adjust the black density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 0, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "LightCyanTrans", N_("Light Cyan Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Cyan Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightMagentaTrans", N_("Light Magenta Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Magenta Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
 {
    {
      "LightYellowTrans", N_("Light Yellow Transition"), "Color=Yes,Category=Advanced Ink Adjustment",
      N_("Light Yellow Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, STP_CHANNEL_NONE, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
};

static const int float_parameter_count =
sizeof(float_parameters) / sizeof(const float_param_t);

/*
 * Duplex support - modes available
 * Note that the internal names MUST match those in cups/genppd.c else the
 * PPD files will not be generated correctly
 */

static const stp_param_string_t duplex_types[] =
{
  { "None",             N_ ("Off") },
  { "DuplexNoTumble",   N_ ("Long Edge (Standard)") },
  { "DuplexTumble",     N_ ("Short Edge (Flip)") }
};
#define NUM_DUPLEX (sizeof (duplex_types) / sizeof (stp_param_string_t))

/*
 * Orientation support - modes available
 * Note that the internal names MUST match those in cups/genppd.c else the
 * PPD files will not be generated correctly
 */

static const stp_param_string_t orientation_types[] = {
  {"Portrait", N_("Portrait")},
  {"Landscape", N_("Landscape")},
  {"UpsideDown", N_("Reverse Portrait")},
  {"Seascape", N_("Reverse Landscape")},
};
#define NUM_ORIENTATION (sizeof (orientation_types) / sizeof (stp_param_string_t))

static const canon_paper_t *
get_media_type(const canon_cap_t* caps,const char *name)
{
  int i;
  if (name && caps->paperlist)
    {
      for (i = 0; i < caps->paperlist->count; i++)
        {
	  /* translate paper_t.name */
	  if (!strcmp(name, caps->paperlist->papers[i].name))
	    return &(caps->paperlist->papers[i]);
        }
      return &(caps->paperlist->papers[0]);
    }
  return NULL;
}

static const char* canon_families[] = {
 "", /* the old BJC printers */
 "S",         /*  1 */
 "i",         /*  2 */
 "PIXMA iP",  /*  3 */
 "PIXMA iX",  /*  4 */
 "PIXMA MP",  /*  5 */
 "PIXUS",     /*  6 */
 "PIXMA Pro", /*  7 */
 "PIXMA MG",  /*  8 */
 "PIXMA MX",  /*  9 */
 "SELPHY DS", /* 10 */
 "PIXMA mini",/* 11 */
 "PIXMA E",   /* 12 */
 "PIXMA P",   /* 13 */
 "MAXIFY iB", /* 14 */
 "MAXIFY MB", /* 15 */
 "PIXMA MPC", /* 16 */
 "PIXMA G",   /* 17 */
 "PIXMA TS",  /* 18 */
 "PIXMA TR",  /* 19 */
 "PIXMA XK",  /* 20 */
 "PIXMA GM",  /* 21 */
};

/* canon model ids look like the following
   FFMMMMMM
   FF: family is the offset in the canon_families struct
   MMMMMM: model nr
*/
static char* canon_get_printername(const stp_vars_t* v)
{
  unsigned int model = stp_get_model_id(v);
  unsigned int family = model / 1000000;
  unsigned int nr = model - family * 1000000;
  char* name;
  size_t len;
  if(family >= sizeof(canon_families) / sizeof(canon_families[0])){
    stp_eprintf(v,"canon_get_printername: no family %i using default BJC\n", family);
    family = 0;
  }
  len = strlen(canon_families[family]) + 7; /* max model nr. + terminating 0 */
  name = stp_zalloc(len);
  snprintf(name,len,"%s%u",canon_families[family],nr);
  stp_dprintf(STP_DBG_CANON, v,"canon_get_printername: current printer name: %s\n", name);
  return name;
}

static const canon_cap_t * canon_get_model_capabilities(const stp_vars_t*v)
{
  int i;
  char* name = canon_get_printername(v);
  int models= sizeof(canon_model_capabilities) / sizeof(canon_cap_t);
  for (i=0; i<models; i++) {
    if (!strcmp(canon_model_capabilities[i].name,name)) {
      stp_free(name);
      return &(canon_model_capabilities[i]);
    }
  }
  stp_eprintf(v,"canon: model %s not found in capabilities list=> using default\n",name);
  stp_free(name);
  return &(canon_model_capabilities[0]);
}

static const canon_slot_t *
canon_source_type(const char *name, const canon_cap_t * caps)
{
    if(name){
        int i;
        for(i=0; i<caps->slotlist->count; i++){
            if( !strcmp(name,caps->slotlist->slots[i].name))
                 return &(caps->slotlist->slots[i]);
        }
    }
    return &(caps->slotlist->slots[0]);
}

/* function returns the current set printmode (specified by resolution) */
/* if no mode is set the default mode will be returned */
static const canon_mode_t* canon_get_current_mode(const stp_vars_t *v){
#if 0
    const char* input_slot = stp_get_string_parameter(v, "InputSlot");
    const char *quality = stp_get_string_parameter(v, "Quality");
#endif
    const char *resolution = stp_get_string_parameter(v, "Resolution");
    const canon_cap_t * caps = canon_get_model_capabilities(v);
    const canon_mode_t* mode = NULL;
    const char *ink_type = stp_get_string_parameter(v, "InkType");/*debug*/
    const char *ink_set = stp_get_string_parameter(v, "InkSet");/*debug*/
    int i;

    stp_dprintf(STP_DBG_CANON, v,"Entered canon_get_current_mode\n");

    if (ink_set)
      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: InkSet value (high priority): '%s'\n",ink_set);
    else
      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: InkSet value is NULL\n");

    if (ink_type)
      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: InkType value (low priority): '%s'\n",ink_type);
    else
      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: InkType value is NULL\n");

    if(resolution){
      for(i=0;i<caps->modelist->count;i++){
            if(!strcmp(resolution,caps->modelist->modes[i].name)){
                mode = &caps->modelist->modes[i];
                break;
            }
        }
    }
#if 0
    if(!mode)
        mode = &caps->modelist->modes[caps->modelist->default_mode];
#endif

#if 0
    if(quality && strcmp(quality, "None") == 0)
        quality = "Standard";

    if(quality && !strcmp(quality,"Standard")){
        return &caps->modelist->modes[caps->modelist->default_mode];
    }
#endif

#if 0
    /* only some modes can print to cd */
    if(input_slot && !strcmp(input_slot,"CD") && !(mode->flags & MODE_FLAG_CD)){
        for(i=0;i<caps->modelist->count;i++){
            if(caps->modelist->modes[i].flags & MODE_FLAG_CD){
                mode = &caps->modelist->modes[i];
                break;
            }
        }
    }
#endif

    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: current mode is '%s'\n",resolution ? resolution : "(null)");

    return mode;
}

const canon_modeuse_t* select_media_modes(stp_vars_t *v, const canon_paper_t* media_type,const canon_modeuselist_t* mlist){
  const canon_modeuse_t* muse = NULL;
  int i;
  for(i=0;i<mlist->count;i++){
    if(!strcmp(media_type->name,mlist->modeuses[i].name)){
      muse = &mlist->modeuses[i];
      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: mode searching: assigned media '%s'\n",mlist->name);
      break;
    }
  }
  return muse;
}

int compare_mode_valid(stp_vars_t *v,const canon_mode_t* mode,const canon_modeuse_t* muse, const canon_modeuselist_t* mlist){
  int i=0;
  int modecheck=1;
  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: mode searching: assigned mode-media '%s'\n",mlist->name);
  while (muse->mode_name_list[i]!=NULL){
    if(!strcmp(mode->name,muse->mode_name_list[i])){
      modecheck=0;
      break;
    }
    i++;
  }
  return modecheck;
}

const canon_mode_t* suitable_mode_monochrome(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,int quality,const char *duplex_mode) {
  const canon_mode_t* mode=NULL;
  int i=0;
  int j;
  int modefound=0;

  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Entered suitable_mode_monochrome\n");

  while ((muse->mode_name_list[i]!=NULL) && (modefound != 1)){
    for(j=0;j<caps->modelist->count;j++){
      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
	if ( (muse->use_flags & INKSET_BLACK_MODEREPL) ) {
	  /* only look at modes with MODE_FLAG_BLACK if INKSET_BLACK_MODEREPL is in force */
	  if ( (caps->modelist->modes[j].quality >= quality) && (caps->modelist->modes[j].flags & MODE_FLAG_BLACK) ){
	    /* keep setting the mode until lowest matching quality is found */
	    if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	      /* duplex check -- rare for monochrome, cannot remember any such case */
	      mode = &caps->modelist->modes[j];
	      modefound=1;
	    }
	  }
	  break; /* go to next mode in muse list */
	}
	else { /* no special replacement modes for black inkset */
	  if ( (caps->modelist->modes[j].quality >= quality) ){
	    /* keep setting the mode until lowest matching quality is found */
	    if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	      /* duplex check -- rare for monochrome, cannot remember any such case */
	      mode = &caps->modelist->modes[j];
	      modefound=1;
	    }
	  }
	  break; /* go to next mode in muse list */
	}
      }
    }
    i++;
  }
  return mode;
}

const canon_mode_t* find_first_matching_mode_monochrome(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,const char *duplex_mode) {
  const canon_mode_t* mode=NULL;
  /* only look at modes with MODE_FLAG_BLACK if INKSET_BLACK_MODEREPL is in force */
  int i=0;
  int modefound=0;
  int j;

  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Entered find_first_matching_mode_monochrome\n");

  while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
    /* pick first mode with MODE_FLAG_BLACK */
    for(j=0;j<caps->modelist->count;j++){
      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
	/* only look at modes with MODE_FLAG_BLACK if INKSET_BLACK_MODEREPL is in force */
	if ( (caps->modelist->modes[j].flags & MODE_FLAG_BLACK) ) {
	  if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	    /* duplex check -- rare for monochrome, cannot remember any such case */
	    mode = &caps->modelist->modes[j];
	    modefound=1;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (find_first_matching_mode_monochrome): picked monochrome mode (%s)\n",mode->name);
	  }
	}
	break; /* go to next mode in muse list */
      }
    }
    i++;
  }
  return mode;
}

const canon_mode_t* find_first_matching_mode(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,const char *duplex_mode) {
  const canon_mode_t* mode=NULL;
  int i=0;
  int modefound=0;
  int j;

  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Entered find_first_matching_mode\n");

  while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
    for(j=0;j<caps->modelist->count;j++){
      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
	if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	  /* duplex check */
	  mode = &caps->modelist->modes[j];
	  modefound=1;
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (find_first_matching_mode): picked mode without inkset limitation (%s)\n",mode->name);
	}
	break; /* go to next mode in muse list */
      }
    }
    i++;
  }
  return mode;
}

const canon_mode_t* suitable_mode_color(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,int quality,const char *duplex_mode) {
  const canon_mode_t* mode=NULL;
  int i=0;
  int j;
  int modefound=0;

  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Entered suitable_mode_color\n");

  while ((muse->mode_name_list[i]!=NULL) && (modefound != 1)){
    for(j=0;j<caps->modelist->count;j++){
      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
	if ( (muse->use_flags & INKSET_COLOR_MODEREPL) ) {
	  /* only look at modes with MODE_FLAG_COLOR if INKSET_COLOR_MODEREPL is in force */
	  if ( (caps->modelist->modes[j].quality >= quality)  && (caps->modelist->modes[j].flags & MODE_FLAG_COLOR) ) {
	    /* keep setting the mode until lowest matching quality is found */
	    if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	      /* duplex check */
	      mode = &caps->modelist->modes[j];
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (suitable_mode_color): picked mode with special replacement inkset (%s)\n",mode->name);
	      modefound=1;
	    }
	  }
	  break; /* go to next mode in muse list */
	}
	else { /* no special replacement modes for color inkset */
	  if ( (caps->modelist->modes[j].quality >= quality) ){
	    /* keep setting the mode until lowest matching quality is found */
	    if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	      /* duplex check */
	      mode = &caps->modelist->modes[j];
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (suitable_mode_color): picked mode without any special replacement inkset (%s)\n",mode->name);
	      modefound=1;
	    }
	  }
	  break; /* go to next mode in muse list */
	}
      }
    }
    i++;
  }
  return mode;
}

const canon_mode_t* find_first_matching_mode_color(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,const char *duplex_mode) {
  const canon_mode_t* mode=NULL;
  /* only look at modes with MODE_FLAG_COLOR if INKSET_COLOR_MODEREPL is in force */
  int i=0;
  int modefound=0;
  int j;

  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Entered find_first_matching_mode_color\n");

  while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
    /* pick first mode with MODE_FLAG_COLOR */
    for(j=0;j<caps->modelist->count;j++){
      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
	/* only look at modes with MODE_FLAG_COLOR if INKSET_COLOR_MODEREPL is in force */
	if ( (caps->modelist->modes[j].flags & MODE_FLAG_COLOR) ) {
	  if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	    /* duplex check */
	    mode = &caps->modelist->modes[j];
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (find_first_matching_mode_color): picked first mode with special replacement inkset (%s)\n",mode->name);
	    modefound=1;
	  }
	}
	break; /* go to next mode in muse list */
      }
    }
    i++;
  }
  return mode;
}

const canon_mode_t* suitable_mode_photo(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,int quality,const char *duplex_mode) {
  const canon_mode_t* mode=NULL;
  int i=0;
  int j;
  int modefound=0;

  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Entered suitable_mode_photo\n");

  while ((muse->mode_name_list[i]!=NULL) && (modefound != 1)){
    for(j=0;j<caps->modelist->count;j++){
      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
	if ( (muse->use_flags & INKSET_PHOTO_MODEREPL) ) {
	  /* only look at modes with MODE_FLAG_PHOTO if INKSET_PHOTO_MODEREPL is in force */
	  if ( (caps->modelist->modes[j].quality >= quality)  && (caps->modelist->modes[j].flags & MODE_FLAG_PHOTO) ) {
	    /* keep setting the mode until lowest matching quality is found */
	    if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	      /* duplex check */
	      mode = &caps->modelist->modes[j];
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (suitable_mode_photo): picked first mode with special replacement inkset (%s)\n",mode->name);
	      modefound=1;
	    }
	  }
	  break; /* go to next mode in muse list */
	}
	else { /* if no special replacement modes for photo inkset */
	  if ( (caps->modelist->modes[j].quality >= quality) ){
	    /* keep setting the mode until lowest matching quality is found */
	    if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	      /* duplex check */
	      mode = &caps->modelist->modes[j];
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (suitable_mode_photo): picked first mode with photo inkset (%s)\n",mode->name);
	      modefound=1;
	    }
	  }
	  break; /* go to next mode in muse list */
	}
      }
    }
    i++;
  }
  return mode;
}

const canon_mode_t* find_first_matching_mode_photo(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,const char *duplex_mode) {
  const canon_mode_t* mode=NULL;
  /* only look at modes with MODE_FLAG_PHOTO if INKSET_PHOTO_MODEREPL is in force */
  int i=0;
  int modefound=0;
  int j;

  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Entered find_first_matching_mode_photo\n");

  while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
    /* pick first mode with MODE_FLAG_PHOTO */
    for(j=0;j<caps->modelist->count;j++){
      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
	/* only look at modes with MODE_FLAG_PHOTO if INKSET_PHOTO_MODEREPL is in force */
	if ( (caps->modelist->modes[j].flags & MODE_FLAG_PHOTO) ) {
	  if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	    /* duplex check */
	    mode = &caps->modelist->modes[j];
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (find_first_matching_mode_photo): picked first mode with photo inkset (%s)\n",mode->name);
	    modefound=1;
	  }
	}
	break; /* go to next mode in muse list */
      }
    }
    i++;
  }
  return mode;
}

const canon_mode_t* suitable_mode_general(stp_vars_t *v,const canon_modeuse_t* muse,const canon_cap_t *caps,int quality,const char *duplex_mode) {
  const canon_mode_t* mode=NULL;
  int i=0;
  int j;
  int modefound=0;

  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Entered suitable_mode_general\n");


  while ((muse->mode_name_list[i]!=NULL) && (modefound != 1)){
    for(j=0;j<caps->modelist->count;j++){
      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
	if ( (caps->modelist->modes[j].quality >= quality) ) {
	  /* keep setting the mode until lowest matching quality is found */
	  if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
	    /* duplex check */
	    mode = &caps->modelist->modes[j];
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (suitable_mode_general): picked first mode with lowest matching quality (%s)\n",mode->name);
	    modefound=1;
	  }
	}
	break; /* go to next mode in muse list */
      }
    }
    i++;
  }
  return mode;
}

const char* find_ink_type(stp_vars_t *v,const canon_mode_t* mode,const char *printing_mode) {
  int i,inkfound;
  const char *ink_type = stp_get_string_parameter(v, "InkType");

  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Entered find_ink_type\n");

  /* if InkType does not match that of mode, change InkType to match it */
  /* choose highest color as default, as there is only one option for Black */
  /* if InkType does not match that of mode, change InkType to match it */
  /* choose highest color as default, as there is only one option for Black */
  if (printing_mode && !strcmp(printing_mode,"BW")) {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
    stp_set_string_parameter(v, "InkType", "Gray");
    ink_type = stp_get_string_parameter(v, "InkType");
  } else {
    inkfound=0;
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
	if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
	  inkfound=1;
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
	  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
	  ink_type = stp_get_string_parameter(v, "InkType");
	  break;
	}
      }
    }
    /* if no match found choose first available inkset */
    if (inkfound==0) {
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
	  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
	    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
	    ink_type = stp_get_string_parameter(v, "InkType");
	    inkfound=1; /* set */
	    break;
	  }
	}
      }
    }
  }
  return ink_type;
}

/* function checks printmode (specified by resolution) */
/* and substitutes a mode if needed. NULL is returned for now */
const canon_mode_t* canon_check_current_mode(stp_vars_t *v){
#if 0
  const char* input_slot = stp_get_string_parameter(v, "InputSlot");
  const char *quality = stp_get_string_parameter(v, "Quality");
#endif
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const char *ink_set = stp_get_string_parameter(v, "InkSet");
  const char *duplex_mode = stp_get_string_parameter(v, "Duplex");
  const char *ink_type = stp_get_string_parameter(v, "InkType");
  const char *printing_mode = stp_get_string_parameter(v, "PrintingMode");
  const canon_cap_t * caps = canon_get_model_capabilities(v);
  const canon_mode_t* mode = NULL;
  const canon_modeuselist_t* mlist = caps->modeuselist;
  const canon_modeuse_t* muse = NULL;
  const canon_paper_t* media_type = get_media_type(caps,stp_get_string_parameter(v, "MediaType"));
  int i,j;
  int modecheck, quality, modefound;
#if 0
  int inkfound;
#endif

  stp_dprintf(STP_DBG_CANON, v,"Entered canon_check_current_mode: got PrintingMode %s\n",printing_mode);

  /* Logic: priority of options
     1. Media --- always present for final selection.
     2. InkSet (cartridge selection) --- optional as only some printers offer this.
     3. PrintingMode --- for printers which have K-only monochrome modes can choose BW.
     4. Duplex --- for printers that have special duplex modes need to skip non-duplex modes.
     5. InkType --- suggestion only, since modes are tied to ink types in most Canon printers.
     6. Quality --- suggestion, based on initially-selected mode.
     7. Mode --- once chosen, InkType is selected based on quality, inkset, printing mode.
  */

  /*
    canon-mode-media:
    INKSET_BLACK_SUPPORT: media type supports black-only cartridge
    INKSET_COLOR_SUPPORT: media type supports color-only cartridge
    INKSET_BLACK_MODEREPL: media type has special modes for black-only cartridge
    INKSET_COLOR_MODEREPL: media type has special modes for black-only cartridge
    INKSET_PHOTO_SUPPORT: media type supports special photo cartridge
    DUPLEX_MODEREPL: media type has (a) special mode(s) for duplex
    canon-modes.h:
    MODE_FLAG_BLACK: mode can be used for supporting black-only cartridge
    MODE_FLAG_COLOR: mode can be used for supporting color-only cartridge
    MODE_FLAG_PHOTO: mode can be used for supporting photo cartridge
    MODE_FLAG_NOPDUPLEX: mode cannot be used for duplex, must be skipped
  */

  if(resolution){
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  check_current_mode --- (Initial) Resolution already known: '%s'\n",resolution);
    for(i=0;i<caps->modelist->count;i++){
      if(!strcmp(resolution,caps->modelist->modes[i].name)){
	mode = &caps->modelist->modes[i];
	break;
      }
    }
  }
  else {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  check_current_mode --- (Initial) Resolution not yet known \n");
  }

  if (ink_set)
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: (Initial) InkSet value (high priority): '%s'\n",ink_set);
  else
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: (Initial) InkSet value is NULL\n");

  if (ink_type)
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: (Initial) InkType value (low priority): '%s'\n",ink_type);
  else
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: (Initial) InkType value is NULL\n");

  /* beginning of mode replacement code */
  if (media_type && resolution && mode) {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  check_current_mode --- Resolution, Media, Mode all known \n");
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: media type selected: '%s'\n",media_type->name);
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: (Initial) Gutenprint: mode initially active: '%s'\n",mode->name);

    /* scroll through modeuse list to find media */
    muse = select_media_modes(v,media_type,mlist);

    /* now scroll through to find if the mode is in the modeuses list */
    modecheck=compare_mode_valid(v,mode,muse,mlist);

    stp_dprintf(STP_DBG_CANON, v,"DEBUG: (mode replacement) Gutenprint: modecheck value: '%i'\n",modecheck);

    /* if we did not find a valid mode, need to replace it */
    if (modecheck!=0) {

      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (check_current_mode): no suitable mode exists, need to find a mode\n");

      quality = mode->quality;
      /* Black InkSet */
      if (ink_set && !strcmp(ink_set,"Black"))  {
	stp_set_string_parameter(v, "PrintingMode","BW");
	printing_mode = stp_get_string_parameter(v, "PrintingMode");
	if (!(mode->ink_types & CANON_INK_K)) {

	  /* need a new mode:
	     loop through modes in muse list searching for a matching inktype, comparing quality
	  */
	  mode=suitable_mode_monochrome(v,muse,caps,quality,duplex_mode);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media matching the InkSet limitation */
	    if ( (muse->use_flags & INKSET_BLACK_MODEREPL) ) {
	      mode=find_first_matching_mode_monochrome(v,muse,caps,duplex_mode);
	    }
	    else {  /* no special replacement modes for black inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	      stp_dprintf(STP_DBG_CANON, v,"Gutenprint (check_current_mode): Ink set is set to CANON_INK_K but did no special conditions, so first match taken: '%s'\n",mode->name);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Black): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
	}
	else {
	  /* mode is fine */
	  /* matched expected K inkset, but need to check if Duplex matches, and if not, get a new mode with right inkset */
	  mode=suitable_mode_monochrome(v,muse,caps,quality,duplex_mode);
	  stp_dprintf(STP_DBG_CANON, v,"Gutenprint (check_current_mode): Ink set is set to CANON_INK_K and mode already fits (but need to check for duplex condition): '%s'\n",mode->name);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media matching the InkSet limitation */
	    if ( (muse->use_flags & INKSET_BLACK_MODEREPL) ) {
	    mode=find_first_matching_mode_monochrome(v,muse,caps,duplex_mode);
	    stp_dprintf(STP_DBG_CANON, v,"Gutenprint (check_current_mode): Ink set is set to CANON_INK_K, pick first one matching condition for black mode replacement: '%s'\n",mode->name);
	    }
	    else {  /* no special replacement modes for black inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	      stp_dprintf(STP_DBG_CANON, v,"Gutenprint (check_current_mode): Ink set is set to CANON_INK_K, pick first one matching condition without mode replacement needed: '%s'\n",mode->name);
	    }
	  }

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Black): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Black): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Black): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Black): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif

#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Black): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
#endif

	}
      } /* End of Black Inkset */
      /*-------------------------------------------------------------------------------------------------*/
      /* Color InkSet */
      /* Added limitation: "Color" for BJC corresponds to "Both" on other types */
      else if ( (ink_set && !strcmp(ink_set,"Color")) && (caps->features & CANON_CAP_T) ) {
	stp_set_string_parameter(v, "PrintingMode","Color");
	printing_mode = stp_get_string_parameter(v, "PrintingMode");
	if (!(mode->ink_types & CANON_INK_CMY)) {
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): inkset incorrect for Color cartridge---need new mode\n");
	  /* need a new mode
	     loop through modes in muse list searching for a matching inktype, comparing quality
	  */
	  mode=suitable_mode_color(v,muse,caps,quality,duplex_mode);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media */
	    if ( (muse->use_flags & INKSET_COLOR_MODEREPL) ) {
	      mode=find_first_matching_mode_color(v,muse,caps,duplex_mode);
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): Decided on first matching mode for color inkset (%s)\n",mode->name);
	    }
	    else {  /* no special replacement modes for color inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): Decided on first matching mode with no special replacement modes for color inkset (%s)\n",mode->name);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif
#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
#endif

	}
	else {
	  /* mode is fine */
	  /* matched expected RGB inkset, but need to check if Duplex matches, and if not, get a new mode with right inkset */
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): inkset OK but need to check other parameters\n");
	  mode=suitable_mode_color(v,muse,caps,quality,duplex_mode);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media */
	    if ( (muse->use_flags & INKSET_COLOR_MODEREPL) ) {
	      mode=find_first_matching_mode_color(v,muse,caps,duplex_mode);
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): Decided on first matching mode for color inkset (%s)\n",mode->name);
	    }
	    else {  /* no special replacement modes for color inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): Decided on first matching mode with no special replacement modes for color inkset (%s)\n",mode->name);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif
#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
#endif

	}
      }  /* end of Color InkSet */
      /*-------------------------------------------------------------------------------------------------*/
      /* Photo cartridge selection: only use modes that support it */
      else if (ink_set && !strcmp(ink_set,"Photo")) {
	/* Photo cartridge printing does not seem to have any monochrome option */
	stp_set_string_parameter(v, "PrintingMode","Color");
	printing_mode = stp_get_string_parameter(v, "PrintingMode");
	/* need to match photo cartridge mode flag */
	if (!(mode->flags & MODE_FLAG_PHOTO)) {
	  /* need a new mode
	     loop through modes in muse list searching for a matching inkset, comparing quality
	  */
	  mode=suitable_mode_photo(v,muse,caps,quality,duplex_mode);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media */
	    if ( (muse->use_flags & INKSET_PHOTO_MODEREPL) ) {
	      mode=find_first_matching_mode_photo(v,muse,caps,duplex_mode);
	    }
	    else {  /* no special replacement modes for photo inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif
#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
#endif

	}
	else {
	  /* mode is fine */
	  /* matched expected inkset, but need to check if Duplex matches, and if not, get a new mode with right inkset */
	  mode=suitable_mode_photo(v,muse,caps,quality,duplex_mode);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media */
	    if ( (muse->use_flags & INKSET_PHOTO_MODEREPL) ) {
	      mode=find_first_matching_mode_photo(v,muse,caps,duplex_mode);
	    }
	    else {  /* no special replacement modes for photo inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif
#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }

#endif

	}
      } /* end of Photo Inkset  */
      /*-------------------------------------------------------------------------------------------------*/
      /* no restrictions for InkSet "Both" (non-BJC) or "Color" (BJC) or if no InkSet set yet --- do not worry about InkSet at all */
      else {
	if (printing_mode && !strcmp(printing_mode,"Color")) {
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode Color\n");
	  /* must skip K-only inksets if they exist: they only exist if the option "BW" is also declared but we cannot check if an option exists or not */
	  i=0;
	  quality = mode->quality;
	  modefound=0;
	  while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
	    for(j=0;j<caps->modelist->count;j++){
	      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		if ( (caps->modelist->modes[j].quality >= quality) ) {
		  if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
		    /* duplex check */
		    if (caps->modelist->modes[j].ink_types > CANON_INK_K) {
		      mode = &caps->modelist->modes[j];
		      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (check_current_mode, Both/Color, printmode color): picked first mode with color inkset (%s)\n",mode->name);
		      modefound=1;
		    }
		  }
		}
		break; /* go to next mode in muse list */
	      }
	    }
	    i++;
	  }
	}
	else if (printing_mode && !strcmp(printing_mode,"BW")) {
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode BW\n");
	  /* need to find K-only inksets: they must exist since we declared the printer to have this capability! */
	  i=0;
	  quality = mode->quality;
	  modefound=0;
	  while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
	    for(j=0;j<caps->modelist->count;j++){
	      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		if ( (caps->modelist->modes[j].quality >= quality) ) {
		  if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
		    /* duplex check */
		    if (caps->modelist->modes[j].ink_types & CANON_INK_K) { /* AND means support for CANON_IN_K is included */
		      mode = &caps->modelist->modes[j];
		      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (check_current_mode, Both/Color, printmode BW): picked first mode with mono inkset (%s)\n",mode->name);
		      modefound=1;
		    }
		  }
		}
		break; /* go to next mode in muse list */
	      }
	    }
	    i++;
	  }
	}
	else { /* no restriction from PrintingMode if not set yet */
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode not set yet\n");
	  /* if mode is not a matching duplex mode, need to find a new one */
	  i=0;
	  quality = mode->quality;
	  modefound=0;
	  while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
	    for(j=0;j<caps->modelist->count;j++){
	      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		if ( (caps->modelist->modes[j].quality >= quality) ) {
		  if ( !(duplex_mode) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
		    /* duplex check */
		    mode = &caps->modelist->modes[j];
		      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (check_current_mode, Both/Color, printmode unset): picked first mode with quality match (%s)\n",mode->name);
		    modefound=1;
		  }
		}
		break; /* go to next mode in muse list */
	      }
	    }
	    i++;
	  }
	}
	/* if no mode was found yet, repeat with no restrictions --- since some media may not allow PrintingMode to be what was selected */
	if (modefound==0) {
	  i=0;
	  quality = mode->quality;
	  while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
	    for(j=0;j<caps->modelist->count;j++){
	      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		if ( (caps->modelist->modes[j].quality >= quality) ) {
		  if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
		    /* duplex check */
		    mode = &caps->modelist->modes[j];
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (check_current_mode, Both/Color, no printmode): picked first mode with quality match (%s)\n",mode->name);
		    modefound=1;
		    /* set PrintingMode to whatever the mode is capable of */
		    if (caps->modelist->modes[j].ink_types > CANON_INK_K) {
		      stp_set_string_parameter(v,"PrintingMode","Color");
		      printing_mode = stp_get_string_parameter(v, "PrintingMode");
		      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode set to Color\n");
		    } else {
		      stp_set_string_parameter(v,"PrintingMode","BW");
		      printing_mode = stp_get_string_parameter(v, "PrintingMode");
		      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode set to BW\n");
		    }
		  }
		}
		break; /* go to next mode in muse list */
	      }
	    }
	    i++;
	  }
	}

	ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	/* if InkType does not match that of mode, change InkType to match it */
	/* choose highest color as default, as there is only one option for Black */
	/* if InkType does not match that of mode, change InkType to match it */
	/* choose highest color as default, as there is only one option for Black */
	if (printing_mode && !strcmp(printing_mode,"BW")) {
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	  stp_set_string_parameter(v, "InkType", "Gray");
	  ink_type = stp_get_string_parameter(v, "InkType");
	} else {
	  inkfound=0;
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
	      if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		inkfound=1;
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
	  /* if no match found choose first available inkset */
	  if (inkfound==0) {
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---choosing first available. InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  inkfound=1; /* set */
		  break;
		}
	      }
	    }
	  }
	}
#endif

	/* end of cartridge option block */

	stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: mode searching: replaced mode with: '%s'\n",mode->name);

#if 0
	/* set InkType for the mode decided upon */
	for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	  if (mode->ink_types & canon_inktypes[i].ink_type) {
	    if (strcmp(ink_type,canon_inktypes[i].name)) {
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (Mode found): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
	      stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
	      ink_type = stp_get_string_parameter(v, "InkType");
	      break;
	    }
	  }
	}
#endif

      }
    }
    /* -------------------------------------- modecheck returned 0 for compare_mode_valid ------------------------------------------*/
    else { /* we did find the mode in the list for media, so it should take precedence over other settings, as it is more specific. */

      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (check_current_mode):  mode exists, need to check for consistency (%s)\n",mode->name);
      quality = mode->quality;
      /* Black InkSet */
      if (ink_set && !strcmp(ink_set,"Black")) {
	stp_set_string_parameter(v, "PrintingMode","BW");
	printing_mode = stp_get_string_parameter(v, "PrintingMode");
	if (!(mode->ink_types & CANON_INK_K)) {
	  /* need a new mode:
	     loop through modes in muse list searching for a matching inktype, comparing quality
	  */
	  mode=suitable_mode_monochrome(v,muse,caps,quality,duplex_mode);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media */
	    if ( (muse->use_flags & INKSET_BLACK_MODEREPL) ) {
	      mode=find_first_matching_mode_monochrome(v,muse,caps,duplex_mode);
	    }
	    else {  /* no special replacement modes for black inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif
#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Black): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
#endif

	}
	else {
	  /* mode is fine */
	  /* matched expected K inkset, but need to check if Duplex matches, and if not, get a new mode with right inkset */
	  mode=suitable_mode_monochrome(v,muse,caps,quality,duplex_mode);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media */
	    if ( (muse->use_flags & INKSET_BLACK_MODEREPL) ) {
	      mode=find_first_matching_mode_monochrome(v,muse,caps,duplex_mode);
	    }
	    else {  /* no special replacement modes for black inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif
#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Black): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
#endif

	}
      } /* End of Black Inkset */
      /*-------------------------------------------------------------------------------------------------*/
      /* InkSet Color */
      else if ( (ink_set && !strcmp(ink_set,"Color")) && (caps->features & CANON_CAP_T) ) {
	stp_set_string_parameter(v, "PrintingMode","Color");
	printing_mode = stp_get_string_parameter(v, "PrintingMode");
	if (!(mode->ink_types & CANON_INK_CMY)) { /* Color InkSet */
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): inkset incorrect for Color cartridge---need new mode\n");
	  /* need a new mode
	     loop through modes in muse list searching for a matching inktype, comparing quality
	  */
	  mode=suitable_mode_color(v,muse,caps,quality,duplex_mode);

	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media */
	    if ( (muse->use_flags & INKSET_COLOR_MODEREPL) ) {
	      mode=find_first_matching_mode_monochrome(v,muse,caps,duplex_mode);
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): Decided on first matching mode for color inkset (%s)\n",mode->name);
	    }
	    else {  /* no special replacement modes for color inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): Decided on first matching mode with no special replacement modes for color inkset (%s)\n",mode->name);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif
#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
#endif

	}
	else {
	  /* mode is fine */
	  /* matched expected RGB inkset, but need to check if Duplex matches, and if not, get a new mode with right inkset */
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): inkset OK but need to check other parameters\n");
	  mode=suitable_mode_color(v,muse,caps,quality,duplex_mode); /* something wrong here!!! */
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): returned mode for color inkset: %s\n",mode->name);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media */
	    if ( (muse->use_flags & INKSET_COLOR_MODEREPL) ) {
	      mode=find_first_matching_mode_monochrome(v,muse,caps,duplex_mode);
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): Decided on first matching mode for color inkset (%s)\n",mode->name);
	    }
	    else {  /* no special replacement modes for color inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): Decided on first matching mode with no special replacement modes for color inkset (%s)\n",mode->name);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif
#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
#endif

	}
      } /* End of Color Inkset */
      /*-------------------------------------------------------------------------------------------------*/
      /* Photo cartridge selection: only use modes that support it */
      else if (ink_set && !strcmp(ink_set,"Photo")) {
	/* Photo cartridge printing does not seem to have any monochrome option */
	stp_set_string_parameter(v, "PrintingMode","Color");
	printing_mode = stp_get_string_parameter(v, "PrintingMode");
	/* need to match photo cartridge mode flag */
	if (!(mode->flags & MODE_FLAG_PHOTO)) {
	  /* need a new mode
	     loop through modes in muse list searching for a matching inkset, comparing quality
	  */
	  mode=suitable_mode_photo(v,muse,caps,quality,duplex_mode);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media */
	    if ( (muse->use_flags & INKSET_PHOTO_MODEREPL) ) {
	      mode=find_first_matching_mode_photo(v,muse,caps,duplex_mode);
	    }
	    else {  /* no special replacement modes for photo inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif
#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
#endif

	}
	else {
	  /* mode is fine */
	  /* matched expected inkset, but need to check if Duplex matches, and if not, get a new mode with right inkset */
	  mode=suitable_mode_photo(v,muse,caps,quality,duplex_mode);
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  if (modefound == 0) { /* still did not find a mode: pick first one for that media */
	    if ( (muse->use_flags & INKSET_PHOTO_MODEREPL) ) {
	      mode=find_first_matching_mode_photo(v,muse,caps,duplex_mode);
	    }
	    else {  /* no special replacement modes for photo inkset */
	      mode=find_first_matching_mode(v,muse,caps,duplex_mode);
	    }
	  }
	  if (!mode)
	    modefound=0;
	  else
	    modefound=1;

	  ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  /* if InkType does not match that of mode, change InkType to match it */
	  /* choose highest color as default, as there is only one option for Black */
	  if (printing_mode && !strcmp(printing_mode,"BW")) {
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	    stp_set_string_parameter(v, "InkType", "Gray");
	    ink_type = stp_get_string_parameter(v, "InkType");
	  } else {
	    inkfound=0;
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		  inkfound=1;
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* if no match found choose first available inkset */
	    if (inkfound==0) {
	      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
		if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		  if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		    stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		    ink_type = stp_get_string_parameter(v, "InkType");
		    inkfound=1; /* set */
		    break;
		  }
		}
	      }
	    }
	  }
#endif
#if 0
	  /* set InkType for the mode found */
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) {
	      if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Color): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
#endif

	}
      } /* end of Photo Inkset  */
      /*-------------------------------------------------------------------------------------------------*/
      /* no restrictions for InkSet "Both" (non-BJC) or "Color" (BJC) or if no InkSet set yet */
      else {
	if (printing_mode && !strcmp(printing_mode,"Color")) {
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode Color\n");
	  /* must skip K-only inksets if they exist: they only exist if the option "BW" is also declared but we cannot check if an option exists or not */
	    i=0;
	    quality = mode->quality;
	    modefound=0;
	    while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
	      for(j=0;j<caps->modelist->count;j++){
		if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		  if ( (caps->modelist->modes[j].quality >= quality) ) {
		    if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
		      /* duplex check */
		      if (caps->modelist->modes[j].ink_types > CANON_INK_K) {
			if (!strcmp(mode->name,caps->modelist->modes[j].name)) {
			  mode = &caps->modelist->modes[j];
			  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) Color: Decided on mode (%s)\n",mode->name);
			  modefound=1;
			}
		      }
		    }
		  }
		  break; /* go to next mode in muse list */
		}
	      }
	      i++;
	    }
	}
	else if (printing_mode && !strcmp(printing_mode,"BW")) {
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode BW\n");
	  /* need to find K-only inksets: they must exist since we declared the printer to have this capability! */
	    i=0;
	    quality = mode->quality;
	    modefound=0;
	    while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
	      for(j=0;j<caps->modelist->count;j++){
		if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		  if ( (caps->modelist->modes[j].quality >= quality) ) {
		    if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
		      /* duplex check */
		      if (caps->modelist->modes[j].ink_types & CANON_INK_K) { /* AND means CANON_INK_K is included in the support */
			if (!strcmp(mode->name,caps->modelist->modes[j].name)) {
			  mode = &caps->modelist->modes[j];
			  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) BW: Decided on mode (%s)\n",mode->name);
			  modefound=1;
			}
		      }
		    }
		  }
		  break; /* go to next mode in muse list */
		}
	      }
	      i++;
	    }
	}
	else { /* no restriction from PrintingMode if not set yet */
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode not set yet\n");
	    i=0;
	    quality = mode->quality;
	    modefound=0;
	    while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
	      for(j=0;j<caps->modelist->count;j++){
		if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		  if ( (caps->modelist->modes[j].quality >= quality) ) {
		    if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
		      /* duplex check */
		      if (!strcmp(mode->name,caps->modelist->modes[j].name)) {
			mode = &caps->modelist->modes[j];
			stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode not set yet: Decided on first matching mode with quality match (%s)\n",mode->name);
			modefound=1;
		      }
		    }
		  }
		  break; /* go to next mode in muse list */
		}
	      }
	      i++;
	    }
	}
	/* if no mode was found yet, repeat with no restrictions --- since some media may not allow PrintingMode to be what was selected */
	if (modefound==0) {
	  i=0;
	  quality = mode->quality;
	  while ( (muse->mode_name_list[i]!=NULL)  && (modefound != 1) ) {
	    for(j=0;j<caps->modelist->count;j++){
	      if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		if ( (caps->modelist->modes[j].quality >= quality) ) {
		  if ( (duplex_mode && strncmp(duplex_mode,"Duplex",6)) || !(muse->use_flags & DUPLEX_SUPPORT) || !(caps->modelist->modes[j].flags & MODE_FLAG_NODUPLEX) ) {
		    /* duplex check */
		    mode = &caps->modelist->modes[j];
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) No mode previously found---catch-all: Decided on first matching mode (%s)\n",mode->name);
		    modefound=1;
		    /* set PrintingMode to whatever the mode is capable of */
		    if (caps->modelist->modes[j].ink_types > CANON_INK_K){
		      stp_set_string_parameter(v,"PrintingMode","Color");
		      printing_mode = stp_get_string_parameter(v, "PrintingMode");
		      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode set to Color\n");
		    } else {
		      stp_set_string_parameter(v,"PrintingMode","BW");
		      printing_mode = stp_get_string_parameter(v, "PrintingMode");
		      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both) PrintingMode set to BW\n");
		    }
		  }
		}
		break; /* go to next mode in muse list */
	      }
	    }
	    i++;
	  }
	}

	ink_type=find_ink_type(v,mode,printing_mode);
#if 0
	/* if InkType does not match that of mode, change InkType to match it */
	/* choose highest color as default, as there is only one option for Black */
	/* if InkType does not match that of mode, change InkType to match it */
	/* choose highest color as default, as there is only one option for Black */
	if (printing_mode && !strcmp(printing_mode,"BW")) {
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %u (%s)\n",CANON_INK_K, "Gray");
	  stp_set_string_parameter(v, "InkType", "Gray");
	  ink_type = stp_get_string_parameter(v, "InkType");
	} else {
	  inkfound=0;
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType of mode %s is currently set as %s\n",mode->name,ink_type);
	  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	    if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
	      if ( !(strcmp(ink_type,canon_inktypes[i].name))) {
		inkfound=1;
		stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType match found %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		ink_type = stp_get_string_parameter(v, "InkType");
		break;
	      }
	    }
	  }
	  /* if no match found choose first available inkset */
	  if (inkfound==0) {
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) { /* a mode can have several ink_types: must compare with ink_type if set */
		if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): No match found---choosing first available. InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  inkfound=1; /* set */
		  break;
		}
	      }
	    }
	  }
	}
#endif
      }
    }
  }
  /* end of mode replacement code */


#if 0
  if(quality && strcmp(quality, "None") == 0)
    quality = "Standard";

  if(quality && !strcmp(quality,"Standard")){
    return &caps->modelist->modes[caps->modelist->default_mode];
  }
#endif

#if 0
  /* only some modes can print to cd */
  if(input_slot && !strcmp(input_slot,"CD") && !(mode->flags & MODE_FLAG_CD)){
    for(i=0;i<caps->modelist->count;i++){
      if(caps->modelist->modes[i].flags & MODE_FLAG_CD){
	mode = &caps->modelist->modes[i];
	break;
      }
    }
  }
#endif


  if (mode) {
    stp_set_string_parameter(v, "Resolution",mode->text);  /* check_current_mode checks resolution! */
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  check_current_mode --- updated Resolution: '%s'\n",mode->name);
  }

  if (mode) {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  check_current_mode --- Final returned mode: '%s'\n",mode->name);
  }
  else {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  check_current_mode --- Final returned mode is NULL \n");
  }

  /* set PrintingMode in case of Inkset precedence */
  if (mode) { /* final mode takes precedence */
    if (mode->ink_types == CANON_INK_K)
      stp_set_string_parameter(v, "PrintingMode", "BW");
    else
      stp_set_string_parameter(v, "PrintingMode", "Color");
  }
  else if (ink_type) { /* mode not yet known, but InkType known. InkSet should have been handled together with InkType above */
    if (!strcmp(ink_type,"Gray"))
      stp_set_string_parameter(v, "PrintingMode", "BW");
    else
      stp_set_string_parameter(v, "PrintingMode", "Color");
  }

  if (printing_mode) {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: Final PrintingMode %s\n",printing_mode);
  } else { /* should not happen */
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: Final PrintingMode is NULL\n");
  }
  if (ink_set) {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: Final InkSet value (high priority): '%s'\n",ink_set);
  } else {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: Final InkSet value is NULL\n");
  }

  if (ink_type) {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: Final InkType value (low priority): '%s'\n",ink_type);
  } else {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: Final InkType value is NULL\n");
  }

  return mode;
}

/* function returns the best ink_type for the current mode */
static unsigned int
canon_printhead_colors(const stp_vars_t*v)
{
  int i,j;
  const canon_mode_t* mode;
  const canon_cap_t * caps = canon_get_model_capabilities(v);
  const char *print_mode = stp_get_string_parameter(v, "PrintingMode");
  const char *ink_type = stp_get_string_parameter(v, "InkType");
  const char *ink_set = stp_get_string_parameter(v, "InkSet");

  stp_dprintf(STP_DBG_CANON, v,"Entered canon_printhead_colors: got PrintingMode %s\n",print_mode);

  /* if a mode is available, use it. Else mode is NULL */
  stp_dprintf(STP_DBG_CANON, v,"Calling get_current_parameter from canon_printhead_colors\n");
  mode = canon_get_current_mode(v);

  /* get the printing mode again */
  print_mode = stp_get_string_parameter(v, "PrintingMode");

  /* if the printing mode was already selected as BW, accept it */
  if(print_mode && !strcmp(print_mode, "BW") && !(caps->features & CANON_CAP_NOBLACK) ){ /* workaround in case BW is a default */
    stp_dprintf(STP_DBG_CANON, v,"(canon_printhead_colors[BW]) Found InkType %u (CANON_INK_K)\n",CANON_INK_K);
    stp_dprintf(STP_DBG_CANON, v,"(canon_printhead_colors[BW]) NOBLACK? %lu\n",(caps->features & CANON_CAP_NOBLACK));
    return CANON_INK_K;
  }
  /* alternatively, if the cartridge selection is in force, and black cartridge is selected, accept it */
  if(ink_set && !strcmp(ink_set, "Black")){
    stp_dprintf(STP_DBG_CANON, v,"(canon_printhead_colors[BW]) Found InkSet black selection\n");
    return CANON_INK_K;
  }

  /* originally finds selected InkType of form: CANON_INK_<inks> */
  /* but this is incorrect, since it does not check media or mode */
  /* change: deal with mode set and mode not set cases */

  /* if mode was already set, then return the ink types for only that mode */

  if (mode) {
    /* if an inktype selected check what it is */
    if(ink_type){
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	if (mode->ink_types & canon_inktypes[i].ink_type) {
	  stp_dprintf(STP_DBG_CANON, v,"(canon_printhead_colors[inktype]) Found InkType %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
	  return canon_inktypes[i].ink_type;
	}
      }
    }
    else {
      /* find the matching inks for the mode: chooses the first one found for a mode! */
      /* ink types are arranged in decreasing order so those with more meta inks are discovered first */
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	if(mode->ink_types & canon_inktypes[i].ink_type) {
	  stp_dprintf(STP_DBG_CANON, v,"(canon_printhead_colors[mode]) Found InkType %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
	  return canon_inktypes[i].ink_type;
	}
      }
    }
  }
  else { /* mode not yet set */
    /* if an inktype selected check what it is */
    if(ink_type){
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	if(ink_type && !strcmp(canon_inktypes[i].name,ink_type)) {
	  stp_dprintf(STP_DBG_CANON, v,"(canon_printhead_colors[inktype]) Found InkType %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
	  return canon_inktypes[i].ink_type;
	}
      }
    }
    else { /* no ink type selected yet */
      stp_dprintf(STP_DBG_CANON, v,"canon_printhead_colors: no mode and no inktype: we have to choose the highest one to return\n");
      /* loop through all modes, and return the highest inktype found */
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	for(j=0;j<caps->modelist->count;j++){
	  if(caps->modelist->modes[j].ink_types & canon_inktypes[i].ink_type){
	    stp_dprintf(STP_DBG_CANON, v," highest inktype found ---  %s(%s)\n",canon_inktypes[i].name,canon_inktypes[i].text);
	    return canon_inktypes[i].ink_type;
	  }
	}
      }
    }

  }

  /* originally as fallback choose CANON_INK_K */
  /* However, some Canon printers do not have monochrome mode at all, only color meta ink modes, like iP6000 series */
#if 0
  stp_dprintf(STP_DBG_CANON, v,"(canon_printhead_colors[fall-through]) Returning InkType %i(CANON_INK_K)\n",CANON_INK_K);
  return CANON_INK_K;
#endif
  /* new fallback: loop through ink type in reverse order, picking first one found, which if CANON_INK_K is supported will be that, else the lowest amount of color */
  for(i=((sizeof(canon_inktypes)/sizeof(canon_inktypes[0]))-1);i>=0;i--){
    for(j=0;j<caps->modelist->count;j++){
      if(caps->modelist->modes[j].ink_types & canon_inktypes[i].ink_type){
	stp_dprintf(STP_DBG_CANON, v," lowest inktype found ---  %s(%s)\n",canon_inktypes[i].name,canon_inktypes[i].text);
	return canon_inktypes[i].ink_type;
      }
    }
  }

  /* if fails until here, return something reasonable in most situations */
  return CANON_INK_K;

}

static unsigned char
canon_size_type(const stp_vars_t *v, const canon_cap_t * caps)
{
  const stp_papersize_list_t *list = stpi_get_standard_papersize_list();
  const stp_papersize_t *pp = stpi_get_papersize_by_size(list,
							stp_get_page_height(v),
							stp_get_page_width(v));

  stp_dprintf(STP_DBG_CANON, v,"canon: entered canon_size_type\n");

  if (pp)
    {
      const char *name = pp->name;
      stp_dprintf(STP_DBG_CANON, v,"canon: in canon_size_type is pp->name: '%s'\n",name);
      /* used internally: do not translate */
      /* built ins:                                  Japanese driver notation */
      if (!strcmp(name,"A5"))          return 0x01;
      if (!strcmp(name,"A4"))          return 0x03;
      if (!strcmp(name,"A3"))          return 0x05;
      if (!strcmp(name,"B5"))          return 0x08;
      if (!strcmp(name,"B4"))          return 0x0a;
      if (!strcmp(name,"Letter"))      return 0x0d;
      if (!strcmp(name,"Legal"))       return 0x0f;
      if (!strcmp(name,"Tabloid"))     return 0x11; /* 11x17 inch */
      if (!strcmp(name,"w283h420"))    return 0x14; /* Hagaki */
      /*      if (!strcmp(name,"COM10"))       return 0x16;*/
      /*      if (!strcmp(name,"DL"))          return 0x17;*/
      if (!strcmp(name,"LetterExtra")) return 0x2a; /* Letter navi --- Letter+ */
      if (!strcmp(name,"A4Extra"))     return 0x2b; /* A4navi --- A4+ */
      if (!strcmp(name,"A3plus"))      return 0x2c; /* A3navi --- A3+ (13x19 inch) */
      if (!strcmp(name,"w288h144"))    return 0x2d; /* 4x2 inch labels */
      if (!strcmp(name,"COM10"))       return 0x2e; /* US Comm #10 Env */
      if (!strcmp(name,"DL"))          return 0x2f; /* Euro DL Env */
      if (!strcmp(name,"w297h666"))    return 0x30; /* Western Env #4 (you4) */
      if (!strcmp(name,"w277h538"))    return 0x31; /* Western Env #6 (you6) */
      if (!strcmp(name,"w252h360J"))   return 0x32; /* L --- similar to US 3.5x5 inch size */
      if (!strcmp(name,"w360h504J"))   return 0x33; /* 2L --- similar to US5x7 inch */
      if (!strcmp(name,"w288h432J"))   return 0x34; /* KG --- same size as US 4x6 inch */
      /* if (!strcmp(name,"CD5Inch"))  return 0x35; */ /* CD Custom Tray */
      if (!strcmp(name,"w155h257"))    return 0x36; /* Japanese Business Card 55mm x 91mm */
      if (!strcmp(name,"w360h504"))    return 0x37; /* US5x7 inch */
      if (!strcmp(name,"w420h567"))    return 0x39; /* Oufuku Hagaki --- but should be w567h420 */
      if (!strcmp(name,"w340h666"))    return 0x3a; /* Japanese Long Env #3 (chou3) */
      if (!strcmp(name,"w255h581"))    return 0x3b; /* Japanese Long Env #4 (chou4) */
      /* if (!strcmp(name,"CD5Inch"))  return 0x3f; */ /* CD Tray A */
      /* if (!strcmp(name,"CD5Inch"))  return 0x40; */ /* CD Tray B */
      if (!strcmp(name,"w155h244"))    return 0x41; /* Business/Credit Card 54mm x 86mm */

      /* Fine Art paper codes */
      /* iP6700D, iP7100, iP7500, iP8100, iP8600, iP9910 */
      /* iX7000 */
      /* MG6100, M6200, MG8100, MG8200 */
      /* MX7600 */
      /* MP950, MP960, MP970, MP980, MP990 */
      /* if (!strcmp(name,"A4"))       return 0x42; */ /* FineArt A4 35mm border --- iP7100: gap is 18 */
      /* if (!strcmp(name,"A3"))       return 0x43; */ /* FineArt A3 35mm border */
      /* if (!strcmp(name,"A3plus"))   return 0x44; */ /* FineArt A3plus 35mm border */
      /* if (!strcmp(name,"Letter"))   return 0x45; */ /* FineArt Letter 35mm border */

      if (!strcmp(name,"w288h576"))    return 0x46; /* US4x8 */
      if (!strcmp(name,"w1008h1224J")) return 0x47; /* HanKire --- 14in x 17 inch */
      if (!strcmp(name,"720h864J"))    return 0x48; /* YonKire --- 10in x 12 inch */
      if (!strcmp(name,"c8x10J"))      return 0x49; /* RokuKire --- same size as 8x10 inch */

      /* if (!strcmp(name,"CD5Inch"))  return 0x4a; */ /* CD Tray C */
      /* if (!strcmp(name,"CD5Inch"))  return 0x4b; */ /* CD Tray D */
      /* if (!strcmp(name,"CD5Inch"))  return 0x4c; */ /* CD Tray E */

      /* Fine Art paper codes */
      /* Pro series (9000, 9000 Mk.2, 9500, 9500 Mk.2, PRO-1) */
      /* if (!strcmp(name,"A4"))       return 0x4d; */ /* FineArt A4 35mm border */
      /* if (!strcmp(name,"A3"))       return 0x4e; */ /* FineArt A3 35mm border */
      /* if (!strcmp(name,"Letter"))   return 0x4f; */ /* FineArt Letter 35mm border */
      /* if (!strcmp(name,"A3plus"))   return 0x50; */ /* FineArt A3plus 35mm border */

      /* if (!strcmp(name,"CD5Inch"))  return 0x51; */ /* CD Tray F */
      if (!strcmp(name,"w288h512"))    return 0x52; /* Wide 101.6x180.6mm */
      /* w283h566 Wide postcard 100mm x 200mm */

      /* media size codes for CD (and other media depending on printer model */

      if (!strcmp(name,"CD5Inch"))     return 0x53; /* CD Tray G --- arbitrary choice here, modify in ESC (P command */
      /* if (!strcmp(name,"CD5Inch"))  return 0x56; */ /* CD Tray G-late */
      /* if (!strcmp(name,"CD5Inch"))  return 0x57; */ /* CD Tray H */

      /* Fine Art paper codes */
      /* MG6300, MG6500, MG6700, MG7100, MG7500 (only A4), MG6900, MG7700  */
      /* iP8700, iX6800 (A3 also) */
      /* if (!strcmp(name,"A4"))       return 0x58; */ /* FineArt A4 35mm border */
      /* if (!strcmp(name,"A3"))       return 0x59; */ /* FineArt A3 35mm border */
      /* if (!strcmp(name,"Letter"))   return 0x5a; */ /* FineArt Letter 35mm border */
      /* if (!strcmp(name,"A3plus"))   return 0x5d; */ /* FineArt A3plus 35mm border */

      /* if (!strcmp(name,"CD5Inch"))  return 0x5b; */ /* CD Tray J */
      /* if (!strcmp(name,"CD5Inch"))  return 0x62; */ /* CD Tray L */

      if (!strcmp(name,"A6"))          return 0x63;

      if (!strcmp(name,"LegalIndia"))  return 0x8d; /* Legal (India) */
      if (!strcmp(name,"Oficio"))      return 0x8e; /* Oficio */
      if (!strcmp(name,"M-Oficio"))    return 0x8f; /* Mexico Oficio */

      if (!strcmp(name,"w612h936"))    return 0x90; /* (American) Foolscap */
      if (!strcmp(name,"Executive"))   return 0x91;

      if (!strcmp(name,"C5"))          return 0x92; /* C5 Env */
      if (!strcmp(name,"Monarch"))     return 0x93; /* Monarch Env */

      if (!strcmp(name,"B-Oficio"))    return 0x94; /* Brazil Oficio */

      if (!strcmp(name,"w360h360"))    return 0xba; /* square 5x5 inch */

      /* custom */

      stp_dprintf(STP_DBG_CANON, v,"canon: Unknown paper size '%s' - using custom\n",name);
    } else {
      stp_dprintf(STP_DBG_CANON, v,"canon: Couldn't look up paper size %fx%f - "
	      "using custom\n",stp_get_page_height(v), stp_get_page_width(v));
    }
  return 0;
}

/* fix paper_width and paper_length for known papersizes in ESC (p command */
static void fix_papersize(unsigned char arg_ESCP_1, int *paper_width, int *paper_length){

  switch(arg_ESCP_1)
    {
    case 0x63: *paper_width = 2481; *paper_length = 3497; break;; /* A6 */
    case 0x01: *paper_width = 3497; *paper_length = 4961; break;; /* A5 */
    case 0x03: *paper_width = 4961; *paper_length = 7016; break;; /* A4 */
    case 0x05: *paper_width = 7016; *paper_length = 9922; break;; /* A3 */
    case 0x08: *paper_width = 4300; *paper_length = 6071; break;; /* B5 */
    case 0x0a: *paper_width = 6071; *paper_length = 8599; break;; /* B4 */
    case 0x0d: *paper_width = 5100; *paper_length = 6600; break;; /* Letter */
    case 0x0f: *paper_width = 5100; *paper_length = 8400; break;; /* Legal */
    case 0x8d: *paper_width = 5079; *paper_length = 8150; break;; /* Legal (India) */
    case 0x8e: *paper_width = 5100; *paper_length = 7500; break;; /* Oficio */
    case 0x94: *paper_width = 5103; *paper_length = 8386; break;; /* Brazil Oficio */
    case 0x8f: *paper_width = 5103; *paper_length = 8056; break;; /* Mexico Oficio */
    case 0x90: *paper_width = 5100; *paper_length = 7800; break;; /* (American) Foolscap */
    case 0x91: *paper_width = 4352; *paper_length = 6300; break;; /* Executive */
    case 0x11: *paper_width = 6600; *paper_length = 10200; break;; /* Tabloid : 11x17" */
      /* Letter+, A4+ only seem to be available in shrink-to-fit */
      /* case 0x2a: paper_width = ( init->page_width + border_left + border_right ) * unit / 72; break;; */ /* LetterExtra : Letter navi, Letter+ */
      /* case 0x2b: paper_width = ( init->page_width + border_left + border_right ) * unit / 72; break;; */ /* A4Extra : A4navi, A4+ */
    case 0x2c: *paper_width = 7772; *paper_length = 11410; break;; /* A3Extra : A3navi, A3+ (13x19") */
      /* case 0x2d: paper_width = ( init->page_width + border_left + border_right ) * unit / 72; break;; */ /* w288h144 : 4x2" labels */
      /* Hagaki media */
    case 0x14: *paper_width = 2363; *paper_length = 3497; break;; /* w283h420 : Hagaki */
      /* Oufuku Hagaki should be swapped: w567h420, same height as Hagaki */
      /* case 0x39: paper_width = 4725;  l: 3497 */
      /* w420h567 : Oufuku Hagaki */
      /* case 0x39: paper_width=(init->page_width + border_left + border_right) * unit / 72; break;;*/ /* leave untouched since orientation wrong */
    case 0x52: *paper_width = 2400; *paper_length = 4267; break;; /* w288h512 : Wide101.6x180.6mm */
      /* Envelope media */
    case 0x16: *paper_width = 2475; *paper_length = 5700; break;; /* COM10 : US Commercial #10 */
    case 0x17: *paper_width = 2599; *paper_length = 5197; break;; /* DL : Euro DL */
    case 0x2e: *paper_width = 2475; *paper_length = 5700; break;; /* COM10 : US Commercial #10 */
    case 0x2f: *paper_width = 2599; *paper_length = 5197; break;; /* DL : Euro DL */
    case 0x30: *paper_width = 2481; *paper_length = 5552; break;; /* w297xh666 : Western Env #4 (you4) */
    case 0x31: *paper_width = 2155; *paper_length = 4489;  break;; /* w277xh538 : Western Env #6 (you6) */
    case 0x3a: *paper_width = 2835; *paper_length = 5552; break;; /* w340xh666 : Japanese Long Env #3 (chou3) */
    case 0x3b: *paper_width = 2126; *paper_length = 4843; break;; /* w255xh581 : Japanese Long Env #4 (chou4) */
    case 0x92: *paper_width = 3827; *paper_length = 5410; break;; /* C5 */
    case 0x93: *paper_width = 2325; *paper_length = 4500; break;; /* Monarch */
      /* Photo media */
    case 0x32: *paper_width = 2103; *paper_length = 3000; break;; /* w252h360 : L --- similar to US 3.5x5" */
    case 0x33: *paper_width = 3000; *paper_length = 4205; break;; /* w360h504 : 2L --- similar to US 5x7" */
    case 0x37: *paper_width = 3000; *paper_length = 4200; break;; /* w360h504 : US 5x7" */
    case 0x34: *paper_width = 2400; *paper_length = 3600; break;; /* w288h432J : KG --- same as US 4x6" */
    case 0x46: *paper_width = 2400; *paper_length = 4800; break;; /* w288h576 : US 4x8" */
    case 0xba: *paper_width = 3000; *paper_length = 3000; break;; /* w360h360 : square 5x5" */
      /* CD media */
    case 0x35: *paper_width = 3207; *paper_length = 6041; break;;  /* CD5Inch : CD Custom Tray */
    case 0x3f: *paper_width = 3378; *paper_length = 6206; break;;  /* CD5Inch : CD Tray A */
    case 0x40: *paper_width = 3095; *paper_length = 5640; break;;  /* CD5Inch : CD Tray B */
    case 0x4a: *paper_width = 3095; *paper_length = 5640; break;;  /* CD5Inch : CD Tray C */
    case 0x4b: *paper_width = 3095; *paper_length = 5640; break;;  /* CD5Inch : CD Tray D */
    case 0x4c: *paper_width = 4063; *paper_length = 6497; break;;  /* CD5Inch : CD Tray E */
    case 0x51: *paper_width = 3095; *paper_length = 5730; break;;  /* CD5Inch : CD Tray F */
    case 0x53: *paper_width = 3095; *paper_length = 6008; break;;  /* CD5Inch : CD Tray G */
    case 0x56: *paper_width = 3095; *paper_length = 6008; break;;  /* CD5Inch : CD Tray G late version */
    case 0x57: *paper_width = 3572; *paper_length = 8953; break;;  /* CD5Inch : CD Tray H */
    case 0x5b: *paper_width = 3071; *paper_length = 5311; break;;  /* CD5Inch : CD Tray J */
      /* no printer using Tray L yet supported */
      /* case 0x62: paper_width = ( init->page_width + border_left + border_right ) * unit / 72; break;; */  /* CD5Inch : CD Tray L */
    case 0xbc: *paper_width = 3494; *paper_length = 4928; break;;  /* CD5Inch : CD Tray M */
      /* Business/Credit Card media */
    case 0x36: *paper_width = 1300; *paper_length = 2150; break;; /* w155h257 : Japanese Business Card 55x91mm */
    case 0x41: *paper_width = 1276; *paper_length = 2032; break;; /* w155h244 : Business/Credit Card 54x86mm */
      /* Fine Art media */
    case 0x42: *paper_width = 4961; *paper_length = 7016; break;; /* FineArt A4 35mm border */
    case 0x43: *paper_width = 7016; *paper_length = 9922; break;; /* FineArt A3 35mm border */
    case 0x44: *paper_width = 7772; *paper_length = 11410; break;; /* FineArt A3+ 35mm border */
    case 0x45: *paper_width = 5100; *paper_length = 6600; break;; /* FineArt Letter 35mm border */
    case 0x4d: *paper_width = 4961; *paper_length = 7016; break;; /* FineArt A4 35mm border */
    case 0x4e: *paper_width = 7016; *paper_length = 9922; break;; /* FineArt A3 35mm border */
    case 0x4f: *paper_width = 5100; *paper_length = 6600; break;; /* FineArt Letter 35mm border */
    case 0x50: *paper_width = 7772; *paper_length = 11410; break;; /* FineArt A3+ 35mm border */
    case 0x58: *paper_width = 4961; *paper_length = 7016; break;; /* FineArt A4 35mm border */
    case 0x59: *paper_width = 7016; *paper_length = 9922; break;; /* FineArt A3 35mm border */
    case 0x5a: *paper_width = 5100; *paper_length = 6600; break;; /* FineArt Letter 35mm border */
    case 0x5d: *paper_width = 7772; *paper_length = 11410; break;; /* FineArt A3+ 35mm border */
      /* Other media */
    case 0x47: *paper_width = 8400; *paper_length = 10200; break;; /* w1008h1224J : HanKire --- 14x17" */
    case 0x48: *paper_width = 6000; *paper_length = 7200; break;; /* 720h864J : YonKire --- 10x12" */
    case 0x49: *paper_width = 4800; *paper_length = 6000; break;; /* c8x10J : RokuKire --- same as 8x10" */
      /* default */
      /* default: paper_width=(init->page_width + border_left + border_right) * unit / 72; break;; */ /* custom */
    }
}

static void
canon_describe_resolution(const stp_vars_t *v, stp_resolution_t *x, stp_resolution_t *y)
{
  const canon_mode_t* mode = NULL;
  const canon_cap_t * caps = canon_get_model_capabilities(v);

  /* if mode is not yet set, it remains NULL */
  stp_dprintf(STP_DBG_CANON, v,"Calling get_current_parameter from canon_describe_resolution\n");
  mode = canon_get_current_mode(v);

  if(!mode)
    mode = &caps->modelist->modes[caps->modelist->default_mode];

  if (mode) {
    *x = mode->xdpi;
    *y = mode->ydpi;
  }
}

static const char *
canon_describe_output(const stp_vars_t *v)
{
  unsigned int ink_type = canon_printhead_colors(v);

  if(ink_type & CANON_INK_CMYK_MASK)
    return "CMYK";
  if(ink_type & CANON_INK_CMY_MASK)
    return "CMY";
  /* Gernot added */
  /*if(ink_type & CANON_INK_cmy_MASK)
    return "cmy";*/
  return "Grayscale";
}

/*
 * 'canon_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
canon_list_parameters(const stp_vars_t *v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  stp_parameter_list_t *tmp_list;

  int i;

  /* Set up dithering */
  tmp_list = stp_dither_list_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_destroy(tmp_list);

  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  return ret;
}

static void
canon_parameters(const stp_vars_t *v, const char *name,
		 stp_parameter_t *description)
{
  int		i,j;

  const canon_cap_t * caps=
    canon_get_model_capabilities(v);
  description->p_type = STP_PARAMETER_TYPE_INVALID;

  if (name == NULL)
    return;

  for (i = 0; i < float_parameter_count; i++)
    if (strcmp(name, float_parameters[i].param.name) == 0)
      {
	/* presumably need to return the maximum number of inks the printer can handle */
	unsigned int ink_type = canon_printhead_colors(v);

	stp_fill_parameter_settings(description,
				    &(float_parameters[i].param));
	description->deflt.dbl = float_parameters[i].defval;
	description->bounds.dbl.upper = float_parameters[i].max;
	description->bounds.dbl.lower = float_parameters[i].min;
	if (ink_type != CANON_INK_K || !float_parameters[i].color_only)
	  description->is_active = 1;
	else
	  description->is_active = 0;
	return;
      }

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }
  if (strcmp(name, "PageSize") == 0)
    {
      const char* input_slot = stp_get_string_parameter(v, "InputSlot");
      unsigned int height_limit, width_limit;
      description->bounds.str = stp_string_list_create();

      width_limit = caps->max_width;
      height_limit = caps->max_height;

      if(input_slot && !strcmp(input_slot,"CD")){
        stp_string_list_add_string
          (description->bounds.str, "CD5Inch", _("CD - 5 inch"));
        stp_string_list_add_string
          (description->bounds.str, "CD3Inch", _("CD - 3 inch"));
        stp_string_list_add_string
          (description->bounds.str, "CDCustom", _("CD - Custom"));
      }else{
	  const stp_papersize_list_t *paper_sizes =
	    stpi_get_standard_papersize_list();
	  const stp_papersize_list_item_t *ptli =
	    stpi_papersize_list_get_start(paper_sizes);
	  while (ptli)
	    {
	      const stp_papersize_t *pt = stpi_paperlist_item_get_data(ptli);
	      if (pt->paper_size_type == PAPERSIZE_TYPE_STANDARD ||
		  pt->paper_size_type == PAPERSIZE_TYPE_ENVELOPE) {
		if (strlen(pt->name) > 0 &&
		    pt->width <= width_limit && pt->height <= height_limit){
		  stp_string_list_add_string(description->bounds.str,
					     pt->name, gettext(pt->text));
		}
	      }
	      ptli = stpi_paperlist_item_next(ptli);
	    }
      }
      description->deflt.str =
        stp_string_list_param(description->bounds.str, 0)->name;
  }
  else if (strcmp(name, "CDInnerRadius") == 0 )
    {
      const char* input_slot = stp_get_string_parameter(v, "InputSlot");
      description->bounds.str = stp_string_list_create();
      if ( (!input_slot || !strcmp(input_slot,"CD")) &&
         (!stp_get_string_parameter(v, "PageSize") ||
          strcmp(stp_get_string_parameter(v, "PageSize"), "CDCustom") != 0) )
	{
	  stp_string_list_add_string
	    (description->bounds.str, "None", _("Normal"));
	  stp_string_list_add_string
	    (description->bounds.str, "Small", _("Print To Hub"));
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "CDInnerDiameter") == 0 )
    {
      const char* input_slot = stp_get_string_parameter(v, "InputSlot");
      description->bounds.dimension.lower = 16 * 10 * 72 / 254;
      description->bounds.dimension.upper = 43 * 10 * 72 / 254;
      description->deflt.dimension = 43 * 10 * 72 / 254;
      if ( (!input_slot || !strcmp(input_slot,"CD")) &&
         (!stp_get_string_parameter(v, "PageSize") ||
         strcmp(stp_get_string_parameter(v, "PageSize"), "CDCustom") == 0) )
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "CDOuterDiameter") == 0 )
    {
      const char* input_slot = stp_get_string_parameter(v, "InputSlot");
      description->bounds.dimension.lower = 65 * 10 * 72 / 254;
      description->bounds.dimension.upper = 120 * 10 * 72 / 254;
      description->deflt.dimension = 329;
      if ( (!input_slot || !strcmp(input_slot,"CD")) &&
         (!stp_get_string_parameter(v, "PageSize") ||
          strcmp(stp_get_string_parameter(v, "PageSize"), "CDCustom") == 0) )
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "CDXAdjustment") == 0 ||
	   strcmp(name, "CDYAdjustment") == 0)
    {
      const char* input_slot = stp_get_string_parameter(v, "InputSlot");
      description->bounds.dimension.lower = -15;
      description->bounds.dimension.upper = 15;
      description->deflt.dimension = 0;
      if (!input_slot || !strcmp(input_slot,"CD"))
	description->is_active = 1;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "Resolution") == 0)
  {
#if 0
    const char* input_slot = stp_get_string_parameter(v, "InputSlot");
#endif
    description->bounds.str= stp_string_list_create();
    description->deflt.str = NULL;
    for(i=0;i<caps->modelist->count;i++){
#if 0
      if(!((!input_slot || !strcmp(input_slot,"CD")) && !(caps->modelist->modes[i].flags & MODE_FLAG_CD)))
#endif
          stp_string_list_add_string(description->bounds.str,
				     caps->modelist->modes[i].name, gettext(caps->modelist->modes[i].text));
        stp_dprintf(STP_DBG_CANON, v,"supports mode '%s'\n",
		     caps->modelist->modes[i].name);
        if(i == caps->modelist->default_mode)
	  description->deflt.str=caps->modelist->modes[i].name;
    }
  }
  else if (strcmp(name, "InkType") == 0)
  {
    const canon_mode_t* mode = NULL;

    stp_dprintf(STP_DBG_CANON, v,"Calling get_current_parameter from InkType block in canon_parameters\n");
    mode=canon_get_current_mode(v);

    description->bounds.str= stp_string_list_create();
    if (mode) {
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	if(mode->ink_types & canon_inktypes[i].ink_type){
          stp_string_list_add_string(description->bounds.str,canon_inktypes[i].name,_(canon_inktypes[i].text));
	  stp_dprintf(STP_DBG_CANON, v," mode known --- Added InkType %s(%s) for mode %s (inktype %u)\n",canon_inktypes[i].name,canon_inktypes[i].text,mode->name,mode->ink_types);
	}
      }
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
    }
    /* mode not defined yet --- needed for PPD generation */
    else {
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	for(j=0;j<caps->modelist->count;j++){
	  if(caps->modelist->modes[j].ink_types & canon_inktypes[i].ink_type){
	    stp_string_list_add_string(description->bounds.str,canon_inktypes[i].name,_(canon_inktypes[i].text));
	    stp_dprintf(STP_DBG_CANON, v," no mode --- Added InkType %s(%s) for mode (%s) inktypes %u\n",canon_inktypes[i].name,canon_inktypes[i].text,caps->modelist->modes[j].name,caps->modelist->modes[j].ink_types);
	    break;
	  }
	}
      }
      /* default type must be deduced from the default mode */
      /* use color if available, so break after first (color) is found, since inkt types ordered with gray last */
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	if(caps->modelist->modes[caps->modelist->default_mode].ink_types & canon_inktypes[i].ink_type){
	  description->deflt.str = canon_inktypes[i].name;
	  break;
	}
      }
    }
    /* default type must be deduced from the default mode */
    /*description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;*/
  }
  else if (strcmp(name, "InkChannels") == 0)
    {
      unsigned int ink_type = canon_printhead_colors(v);
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	if(ink_type == canon_inktypes[i].ink_type){
              description->deflt.integer = canon_inktypes[i].num_channels;
	      stp_dprintf(STP_DBG_CANON, v,"Added %d InkChannels\n",canon_inktypes[i].num_channels);
	}
      }
      description->bounds.integer.lower = -1;
      description->bounds.integer.upper = -1;
    }
  else if (strcmp(name, "MediaType") == 0)
  {
    const canon_paper_t * canon_paper_list = caps->paperlist->papers;
    int count = caps->paperlist->count;
    description->bounds.str= stp_string_list_create();
    description->deflt.str= canon_paper_list[0].name;

    for (i = 0; i < count; i ++) {
      stp_string_list_add_string(description->bounds.str,
				canon_paper_list[i].name,
				gettext(canon_paper_list[i].text));

      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  Added Media Type: '%s'\n",canon_paper_list[i].name);
    }
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    const canon_slot_t * canon_slot_list = caps->slotlist->slots;
    int count = caps->slotlist->count;
    description->bounds.str= stp_string_list_create();
    description->deflt.str= canon_slot_list[0].name;

    for (i = 0; i < count; i ++)
      stp_string_list_add_string(description->bounds.str,
				 canon_slot_list[i].name,
				 gettext(canon_slot_list[i].text));
  }
  else if (strcmp(name, "CassetteTray") == 0)
  {
    description->bounds.str= stp_string_list_create();
    description->is_active = 0;
    if (caps->CassetteTray_Opts) {
      const char* input_slot = stp_get_string_parameter(v, "InputSlot");
      stp_string_list_add_string
	(description->bounds.str, "Default", _("Driver-Controlled"));
      stp_string_list_add_string
	(description->bounds.str, "Upper", _("Upper Tray/Cassette 1"));
      stp_string_list_add_string
	(description->bounds.str, "Lower", _("Lower Tray/Cassette 2"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
      if (!input_slot || !strcmp(input_slot,"Cassette"))
	description->is_active = 1;
    }
  }
  else if (strcmp(name, "PrintingMode") == 0)
  {
    int found_color, found_mono;
    const canon_mode_t* mode = NULL;
    /* mode remains NULL if not yet set */

    stp_dprintf(STP_DBG_CANON, v,"Calling get_current_mode from PrintingMode block in canon_parameter\n");
    mode = canon_get_current_mode(v);

    /* If mode is not set need to search ink types for all modes and
       see whether we have any color there
     */

    stp_dprintf(STP_DBG_CANON, v,"PrintingMode---entered enumeration block in canon_printers\n");

    description->bounds.str = stp_string_list_create();

    stp_dprintf(STP_DBG_CANON, v,"PrintingMode---created list\n");

    if (mode) {
      stp_dprintf(STP_DBG_CANON, v,"PrintingMode: (mode known) what is the current mode inktype value: %i\n",mode->ink_types);
      /* e.g., ink_types is 21 = 16 + 4 + 1 */
      if (mode->ink_types > 1) {
	stp_string_list_add_string
	  (description->bounds.str, "Color", _("Color"));
	stp_dprintf(STP_DBG_CANON, v,"PrintingMode: (mode known) added Color\n");
      }
      if (mode->ink_types & CANON_INK_K) {
	stp_string_list_add_string
	  (description->bounds.str, "BW", _("Black and White"));
	stp_dprintf(STP_DBG_CANON, v,"PrintingMode: (mode known) added BW\n");
      }
    }
#if 0
      /* original code */
      if (mode)
	if (mode->ink_types != CANON_INK_K) {
	  stp_string_list_add_string
	    (description->bounds.str, "Color", _("Color"));
	  stp_dprintf(STP_DBG_CANON, v,"PrintingMode: (mode known) added Color\n");
	}
#endif

    else { /* mode not known yet --- needed for PPD generation */
      stp_dprintf(STP_DBG_CANON, v,"PrintingMode: entered mode not known conditional block\n");
      /* add code to find color inks */
      /* default type must be deduced from the default mode */
      /* use color if available, so break after first (color) is found, since ink types ordered with gray last */
      found_color=0;
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	for(j=0;j<caps->modelist->count;j++){
	  if(caps->modelist->modes[j].ink_types > 1){
	    stp_string_list_add_string
	      (description->bounds.str, "Color", _("Color"));
	    found_color=1;
	    stp_dprintf(STP_DBG_CANON, v,"PrintingMode: (mode not known) added Color\n");
	    break;
	  }
	}
	if (found_color==1)
	  break;
      }
      found_mono=0;
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	for(j=0;j<caps->modelist->count;j++){
	  if(caps->modelist->modes[j].ink_types & CANON_INK_K){
	    stp_string_list_add_string
	      (description->bounds.str, "BW", _("Black and White"));
	    found_mono=1;
	    stp_dprintf(STP_DBG_CANON, v,"PrintingMode: (mode not known) added BW\n");
	    break;
	  }
	}
	if (found_mono==1)
	  break;
      }

#if 0
      /* ink types for default mode*/
      if(caps->modelist->modes[caps->modelist->default_mode].ink_types > 1){
	stp_string_list_add_string
	  (description->bounds.str, "Color", _("Color"));
	stp_dprintf(STP_DBG_CANON, v,"PrintingMode: (mode not known) added Color\n");
      }
      if(caps->modelist->modes[caps->modelist->default_mode].ink_types & CANON_INK_K){
	stp_string_list_add_string
	(description->bounds.str, "BW", _("Black and White"));
	stp_dprintf(STP_DBG_CANON, v,"PrintingMode: (mode not known) added BW\n");
      }
#endif
#if 0
      /* original code */
      stp_string_list_add_string
	(description->bounds.str, "BW", _("Black and White"));
      stp_dprintf(STP_DBG_CANON, v,"PrintingMode: added BW\n");
#endif
    }

    /* original code --- fine as is */
    description->deflt.str =
      stp_string_list_param(description->bounds.str, 0)->name;
  }
  else if (strcmp(name, "InkSet") == 0)
    {
      description->bounds.str= stp_string_list_create();
      if (caps->features & CANON_CAP_T) {
	stp_string_list_add_string
	  (description->bounds.str, "Both", _("Both"));
	if (!(caps->features & CANON_CAP_NOBLACK)) {
	    stp_string_list_add_string
	      (description->bounds.str, "Black", _("Black"));
	}
	stp_string_list_add_string
	  (description->bounds.str, "Color", _("Color"));
      } /* mutually exclusive */
      else if (caps->features & CANON_CAP_cart) {
	stp_string_list_add_string
	  (description->bounds.str, "Color", _("Color"));
	stp_string_list_add_string
	  (description->bounds.str, "Black", _("Black"));
	stp_string_list_add_string
	  (description->bounds.str, "Photo", _("Photo"));
      } else {
	/* make sure to have at least a default value: no choice */
	stp_string_list_add_string
	  (description->bounds.str, "None", _("None"));
      }
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  /* Test implementation of borderless printing */
  else if (strcmp(name, "FullBleed") == 0)
    {
      const char* input_slot = stp_get_string_parameter(v, "InputSlot");
      if (input_slot && !strcmp(input_slot,"CD"))
	description->is_active = 0;
      else if (caps->features & CANON_CAP_BORDERLESS)
	description->deflt.boolean = 0;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "Duplex") == 0)
  {
    int offer_duplex=0;

    description->bounds.str = stp_string_list_create();

/*
 * Don't offer the Duplex/Tumble options if the JobMode parameter is
 * set to "Page" Mode.
 * "Page" mode is set by the Gimp Plugin, which only outputs one page at a
 * time, so Duplex/Tumble is meaningless.
 */

    if (stp_get_string_parameter(v, "JobMode"))
        offer_duplex = strcmp(stp_get_string_parameter(v, "JobMode"), "Page");
    else
     offer_duplex=1;

    if (offer_duplex && (caps->features & CANON_CAP_DUPLEX))
    {
      description->deflt.str = duplex_types[0].name;
      for (i=0; i < NUM_DUPLEX; i++)
        {
          stp_string_list_add_string(description->bounds.str,
				     duplex_types[i].name,gettext(duplex_types[i].text));
        }
    }
    else
      description->is_active = 0;
  }
  else if (strcmp(name, "Orientation") == 0)
  {
    description->bounds.str = stp_string_list_create();
    description->deflt.str = orientation_types[0].name;
    for (i=0; i < NUM_ORIENTATION; i++)
      {
        stp_string_list_add_string(description->bounds.str,
				   orientation_types[i].name,gettext(orientation_types[i].text));
      }
  }
  else if (strcmp(name, "Quality") == 0)
  {
#if 0
    int has_standard_quality = 0;
#endif
    description->bounds.str = stp_string_list_create();
    stp_string_list_add_string(description->bounds.str, "None",
			       _("Manual Control"));
    stp_string_list_add_string(description->bounds.str, "Standard",
			       _("Standard"));
    description->deflt.str = "Standard";
  }
  /* Cartridge selection for those printers that have it */
  else if (strcmp(name, "Cartridge") == 0)
  {
#if 0
    int offer_cartridge_selection = 0;
#endif
    description->bounds.str = stp_string_list_create();
    stp_string_list_add_string(description->bounds.str, "Both",
			       _("Both"));
    stp_string_list_add_string(description->bounds.str, "Color",
			       _("Color"));
    stp_string_list_add_string(description->bounds.str, "Black",
			       _("Black"));

    /* description->deflt.str = "Both"; */
    /* Note: not necessary set cartridge if Mono mode */

    if (caps->features & CANON_CAP_CARTRIDGE)
      {
	description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
      }
    else
      description->is_active = 0;
  }

}


/*
 * 'canon_imageable_area()' - Return the imageable area of the page.
 */

static void
internal_imageable_area(const stp_vars_t *v,   /* I */
			int  use_paper_margins,
			int use_maximum_area,
			stp_dimension_t  *left,	/* O - Left position in points */
			stp_dimension_t  *right,	/* O - Right position in points */
			stp_dimension_t  *bottom,	/* O - Bottom position in points */
			stp_dimension_t  *top)	/* O - Top position in points */
{
  stp_dimension_t width, length;	/* Size of page */
  int cd = 0;                           /* CD selected */
  const char *media_size = stp_get_string_parameter(v, "PageSize");
  stp_dimension_t left_margin = 0;
  stp_dimension_t right_margin = 0;
  stp_dimension_t bottom_margin = 0;
  stp_dimension_t top_margin = 0;
  const stp_papersize_t *pt = NULL;
  const char* input_slot = stp_get_string_parameter(v, "InputSlot");

  const canon_cap_t * caps= canon_get_model_capabilities(v);

  if (media_size)
    pt = stp_describe_papersize(v, media_size);

  if(input_slot && !strcmp(input_slot,"CD"))
    cd = 1;

  stp_default_media_size(v, &width, &length);

  if (cd) {
    /* ignore printer margins for the cd print, margins get adjusted in do_print for now */
    if (pt) {
      /* move code from do_print here */
    }
    else {
      /* move code from do_print here */
    }
  }
  /* non-CD media */
  else {
    if (pt && use_paper_margins) {
      left_margin = pt->left;
      right_margin = pt->right;
      bottom_margin = pt->bottom;
      top_margin = pt->top;
    }
    /* limit to printer capabilities---without fullbleed */
    left_margin = MAX(left_margin, caps->border_left);
    right_margin = MAX(right_margin, caps->border_right);
    top_margin = MAX(top_margin, caps->border_top);
    bottom_margin = MAX(bottom_margin, caps->border_bottom);
  }

  /* temporarily limit to non-CD media until page size code moved here from do_print */
  /* Note: written below code to handle CD case as well */
  if(!cd){
    stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: about to enter the borderless condition block\n");
    stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: is borderless available? %016lx\n",caps->features & CANON_CAP_BORDERLESS);
    stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: is borderless selected? %d\n",stp_get_boolean_parameter(v, "FullBleed"));

    if ( (caps->features & CANON_CAP_BORDERLESS) &&
	 (use_maximum_area || (!cd && stp_get_boolean_parameter(v, "FullBleed")))) {

      stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: entered borderless condition\n");

      if (pt) {

	stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: entered pt condition\n");

	if (pt->left <= 0 && pt->right <= 0 && pt->top <= 0 && pt->bottom <= 0) {

	  stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: entered margin<=0 condition\n");

	  if (use_paper_margins) {
	    unsigned width_limit = caps->max_width;
	    left_margin = -8;
	    right_margin = -8;
	    if (width - right_margin - 3 > width_limit)
	      right_margin = width - width_limit - 3;
	    top_margin = -6;
	    bottom_margin = -15;

	    stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: use_paper_margins so set margins all to -7\n");

	  }
	  else {
	    left_margin = 0;
	    right_margin = 0;
	    top_margin = 0;
	    bottom_margin = 0;

	    stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: does not use paper margins so set margins all to 0\n");

	  }
	}
      }
    }
  }

  stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: left_margin %f\n",left_margin);
  stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: right_margin %f\n",right_margin);
  stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: top_margin %f\n",top_margin);
  stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: bottom_margin %f\n",bottom_margin);

  *left =	left_margin;
  *right =	width - right_margin;
  *top =	top_margin;
  *bottom =	length - bottom_margin;

  stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: page_left %f\n",*left);
  stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: page_right %f\n",*right);
  stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: page_top %f\n",*top);
  stp_dprintf(STP_DBG_CANON, v,"internal_imageable_area: page_bottom %f\n",*bottom);

}

static void
canon_imageable_area(const stp_vars_t *v,   /* I */
                     stp_dimension_t  *left,	/* O - Left position in points */
                     stp_dimension_t  *right,	/* O - Right position in points */
                     stp_dimension_t  *bottom,	/* O - Bottom position in points */
                     stp_dimension_t  *top)		/* O - Top position in points */
{
  internal_imageable_area(v, 1, 0, left, right, bottom, top);
}

static void
canon_maximum_imageable_area(const stp_vars_t *v,   /* I */
                     stp_dimension_t  *left,	/* O - Left position in points */
                     stp_dimension_t  *right,	/* O - Right position in points */
                     stp_dimension_t  *bottom,	/* O - Bottom position in points */
                     stp_dimension_t  *top)		/* O - Top position in points */
{
  internal_imageable_area(v, 1, 1, left, right, bottom, top);
}

static void
canon_limit(const stp_vars_t *v,  		/* I */
	    stp_dimension_t *width,
	    stp_dimension_t *height,
	    stp_dimension_t *min_width,
	    stp_dimension_t *min_height)
{
  const canon_cap_t * caps=
    canon_get_model_capabilities(v);
  *width =	caps->max_width;
  *height =	caps->max_height;
  *min_width = 1;
  *min_height = 1;
}

/*
 * 'canon_cmd()' - Sends a command with variable args
 */
static void
canon_cmd(const stp_vars_t *v, /* I - the printer         */
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

#define PUT(V,WHAT,VAL,RES) stp_dprintf(STP_DBG_CANON,V,"canon: "WHAT	\
" is %04x =% 5d = %f\" = %f mm\n",(VAL),(VAL),(VAL)/(1.*RES),(VAL)/(RES/25.4))

#define ESC28 "\033\050"
#define ESC5b "\033\133"
#define ESC40 "\033\100"

static void canon_control_cmd(const stp_vars_t*v,const char* cmd){
      canon_cmd(v,ESC5b,0x4b, 2, 0x00,0x1f);
      stp_puts("BJLSTART\nControlMode=Common\n",v);
      stp_puts(cmd,v);
      stp_putc('\n',v);
      stp_puts("BJLEND\n",v);
}


/* ESC [K --  -- reset printer:
 */
static void
canon_init_resetPrinter(const stp_vars_t *v, const canon_privdata_t *init)
{
  if ( init->caps->control_cmdlist ){
    int i=0;
    while(init->caps->control_cmdlist[i]){
      canon_control_cmd(v,init->caps->control_cmdlist[i]);
      ++i;
    }
  }
  if(!strcmp(init->slot->name,"CD"))
    canon_control_cmd(v,"MediaDetection=ON");
  canon_cmd(v,ESC5b,0x4b, 2, 0x00,0x0f);
}

/* ESC ($ -- 0x24 -- cmdSetDuplex --:
 */
static void
canon_init_setDuplex(const stp_vars_t *v, const canon_privdata_t *init)
{
  if (!(init->caps->features & CANON_CAP_DUPLEX))
    return;
  if (strncmp(init->duplex_str, "Duplex", 6)) {
    if ( !(strcmp(init->caps->name,"i860")) || !(strcmp(init->caps->name,"i865")) || !(strcmp(init->caps->name,"i950")) || !(strcmp(init->caps->name,"i960")) || !(strcmp(init->caps->name,"i990")) ) {
      /* i860, i865, i950, i960, i990 use ESC ($ command even for simplex mode */
      canon_cmd(v,ESC28,0x24,9,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
      return;
    }
    else
      return;
  }
  /* The same command seems to be needed for both Duplex and DuplexTumble
     no idea about the meanings of the single bytes */
  canon_cmd(v,ESC28,0x24,9,0x01,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x02);
}

/* ESC (a -- 0x61 -- cmdSetPageMode --:
 */
static void
canon_init_setPageMode(const stp_vars_t *v, const canon_privdata_t *init)
{
  if (!(init->caps->features & CANON_CAP_a))
    return;

  if (init->caps->features & CANON_CAP_a)
    canon_cmd(v,ESC28,0x61, 1, 0x01);
}

/* ESC (b -- 0x62 -- -- set data compression:
 */
static void
canon_init_setDataCompression(const stp_vars_t *v, const canon_privdata_t *init)
{
  if (!(init->caps->features & CANON_CAP_b))
    return;

  canon_cmd(v,ESC28,0x62, 1, 0x01);
}

/* ESC (c -- 0x63 -- cmdSetColor --:
 */
static void
canon_init_setColor(const stp_vars_t *v, const canon_privdata_t *init)
{
  unsigned char
    numargs, arg_63[6];

  if (!(init->caps->features & CANON_CAP_c))
    return;

  numargs = 3;
  arg_63[0] = init->caps->model_id << 4; /* MODEL_ID */

  switch ( init->caps->model_id ) {

  	case 0:			/* very old 360 dpi series: BJC-800/820 */
		break;		/*	tbd */

  	case 1:			/* 360 dpi series - BJC-4000, BJC-210, BJC-70 and their descendants */
		if (init->used_inks == CANON_INK_K)
                            arg_63[0]|= 0x01;                                        /* PRINT_COLOUR */


//		  if ( (!strcmp(init->caps->name,"85")) ||  (!strcmp(init->caps->name,"1000")) ) /* BJC-85, BJC-1000 */
//		    arg_63[1] = (init->pt) ? init->pt->media_code_c : 0;                /* print media type */
//		  else /* original, not sure which models follow this at all */
		    arg_63[1] = ((init->pt ? init->pt->media_code_c : 0) << 4)                /* PRINT_MEDIA */
		      + 1;	/* hardcode to High quality for now */		/* PRINT_QUALITY */

		    if (!strcmp(init->caps->name,"2100")) { /* BJC-2100: ESC (c command length is 3 */
		      if (!strcmp(init->mode->name,"720x360dpi"))
			arg_63[1] = 0x00;
		      else if (!strcmp(init->mode->name,"360x360dpi_draft"))
			arg_63[1] = 0x00;
		      else if (!strcmp(init->mode->name,"180x180dpi"))
			arg_63[1] = 0x02;
		      /* else keep at 01 hard-coded as above - logic unknown */
		      canon_cmd(v,ESC28,0x63, 3, arg_63[0], arg_63[1], 0x00);
		    } else /* length 2 in legacy code */
		      canon_cmd(v,ESC28,0x63, 2, arg_63[0], arg_63[1]);
		break;

	case 2:			/* are any models using this? */
		break;

	case 3:			/* 720 dpi series - BJC-3000 and descendants */
		if (init->used_inks == CANON_INK_K)
                            arg_63[0]|= 0x01;                                        /* colour mode */

                  arg_63[1] = (init->pt) ? init->pt->media_code_c : 0;                /* print media type */

                 if (!strcmp(init->caps->name,"S200")) /* S200 */
                   {
                     if ((init->mode->xdpi == 720) && (init->mode->ydpi == 720 ))
                       arg_63[2] = 1;
                     else
                       arg_63[2] = 4; /* hardcoded: quality 3  (may be 0...4) */
                     /* bidirectional is controlled via quality: 0..2 is bidi, 3 and 4 uni */
                     /* not every combination works, no idea about the principle */
                     if ( (init->mode->xdpi > 360) || (init->mode->ydpi > 360) )
                       {
                         numargs = 6;
                         arg_63[3] = 0x10; arg_63[4] = 6; arg_63[5] = 8; /* arg5 makes a vert. offset for K */
                         if (init->used_inks == CANON_INK_K)
                           arg_63[4] = 1;
                       }
                   }
		 else if (!strcmp(init->caps->name,"4550")) /* BJC-4550 */
		    {
		      numargs = 3;
		      arg_63[2] = 0; /* not used in Black and Color, no idea about PhotoColor yet */
		      arg_63[1] = init->quality;     /* hardcode to whatever this means for now; quality, apparently */
		    }
                 else
                   arg_63[2] = init->quality;        /* hardcode to whatever this means for now; quality, apparently */

                 stp_zprintf(v, "\033\050\143");
                 stp_put16_le(numargs, v);
                 stp_zfwrite((const char *)arg_63, numargs, 1, v);
		break;
  	}

  return;
}

/* ESC (d -- 0x64 -- -- set raster resolution:
 */
static void
canon_init_setResolution(const stp_vars_t *v, const canon_privdata_t *init)
{
  if (!(init->caps->features & CANON_CAP_d))
    return;

   if (strcmp(init->caps->name,"S200") || (init->mode->xdpi <= 360))
  canon_cmd(v,ESC28,0x64, 4,
	    (init->mode->ydpi >> 8 ), (init->mode->ydpi & 255),
	    (init->mode->xdpi >> 8 ), (init->mode->xdpi & 255));
   else
     if (init->mode->xdpi < 2880)
       canon_cmd(v,ESC28,0x64, 4,
         (720 >> 8), (720 & 255),
         (720 >> 8), (720 & 255));
     else
       canon_cmd(v,ESC28,0x64, 4,
         (720 >> 8), (720 & 255),
         (2880 >> 8), (2880 & 255));
  }

/* ESC (g -- 0x67 -- cmdSetPageMargins --:
 */
static void
canon_init_setPageMargins(const stp_vars_t *v, const canon_privdata_t *init)
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
canon_init_setTray(const stp_vars_t *v, const canon_privdata_t *init)
{
  unsigned char
    arg_6c_1 = 0x00,
    arg_6c_2 = 0x00, /* plain paper */
    arg_6c_3 = 0x00; /* special cases like iP7100 to be handled */

  if (!(init->caps->features & CANON_CAP_l))
    return;

  arg_6c_1 = init->caps->model_id << 4;

  arg_6c_1|= (init->slot->code & 0x0f);

  /* set gap for MP710/740 if thick media selected */
  if (!strcmp(init->slot->name,"AutoThick"))
    if ( (!strcmp(init->caps->name,"PIXMA MP710")) || (!strcmp(init->caps->name,"PIXMA MP740")) )
      arg_6c_3 = 0x10;

  switch ( init->caps->model_id ) {
  case 0:
    break;
  case 1:
    if (init->pt) arg_6c_2 = ((init->pt ? init->pt->media_code_l : 0) << 4);                /* PRINT_MEDIA */
    break;
  case 2:
    break;
  case 3:
    if (init->pt) arg_6c_2 = init->pt->media_code_l;                                        /* PRINT_MEDIA */
    break;
  }

  /* select between length 2 and 3 byte variations of command */
  /*if(init->caps->model_id >= 3)*/
  if(init->caps->ESC_l_len == 3)
    canon_cmd(v,ESC28,0x6c, 3, arg_6c_1, arg_6c_2, arg_6c_3); /* 3rd arg is "gap" */
  else /* else 2 bytes---no other option for now */
    canon_cmd(v,ESC28,0x6c, 2, arg_6c_1, arg_6c_2);
}

/* ESC (m -- 0x6d --  -- :
 */
static void
canon_init_setPrintMode(const stp_vars_t *v, const canon_privdata_t *init)
{
  unsigned char
    arg_6d_1 = 0x03, /* color printhead? */
    arg_6d_2 = 0x00, /* 00=color  02=b/w */
    arg_6d_3 = 0x00, /* only 01 for bjc8200 and S200*/
                     /* S200:for envelope and t-shirt transfer = 03 */
    arg_6d_a = 0x03, /* A4 paper */
    arg_6d_b = 0x00;

  if (!(init->caps->features & CANON_CAP_m))
    return;

  arg_6d_a= canon_size_type(v,init->caps);
  if (!arg_6d_a)
    arg_6d_b= 1;

  arg_6d_1= 0x04;

  if ((!strcmp(init->caps->name,"7000")) && (init->used_inks == CANON_INK_K || init->used_inks == CANON_INK_CcMmYK || init->used_inks == CANON_INK_CcMmYyK))
    arg_6d_1= 0x03;

  if (((!strcmp(init->caps->name,"8200") || !strcmp(init->caps->name,"S200")) && init->used_inks == CANON_INK_K) || init->used_inks == CANON_INK_CMYK)
      arg_6d_1= 0x02;

  if(!strcmp(init->caps->name,"S200") && init->used_inks == CANON_INK_CMY)
      arg_6d_1= 0x02;

  if (init->used_inks == CANON_INK_K)
    arg_6d_2= 0x02;

  if (!strcmp(init->caps->name,"8200") || !strcmp(init->caps->name,"S200"))
    arg_6d_3= 0x01;

  canon_cmd(v,ESC28,0x6d,12, arg_6d_1,
	    0xff,0xff,0x00,0x00,0x07,0x00,
	    arg_6d_a,arg_6d_b,arg_6d_2,0x00,arg_6d_3);
}

/* ESC (M -- 0x4d --  -- :
 */
static void
canon_init_setESC_M(const stp_vars_t *v, const canon_privdata_t *init)
{
  if (!(init->caps->features & CANON_CAP_M))
    return;

  canon_cmd(v,ESC28,0x4d, 3, 0x00, 0x00, 0x00);
}

/* ESC (p -- 0x70 -- cmdSetPageMargins2 --:
 */
static void
canon_init_setPageMargins2(const stp_vars_t *v, canon_privdata_t *init)
{
  unsigned char arg_70_1,arg_70_2,arg_70_3,arg_70_4;

  int border_left,border_right,border_top,border_bottom;
  int border_left2,border_top2;
  int border_right2;
  int border_bottom2;
  int area_right,area_top;
  int test_cd; /* variable for activating experimental adjustments */
  /* CD tray size adjustments for paper dimensions */
  int adjust_tray_custom_length, adjust_tray_custom_width;
  int adjust_tray_A_length, adjust_tray_A_width;
  int adjust_tray_BCD_length, adjust_tray_BCD_width;
  int adjust_tray_E_length, adjust_tray_E_width;
  int adjust_tray_F_length, adjust_tray_F_width;
  int adjust_tray_G_length, adjust_tray_G_width;
  int adjust_tray_H_length, adjust_tray_H_width;
  int adjust_tray_J_length, adjust_tray_J_width;
  int adjust_tray_L_length, adjust_tray_L_width;
  /* CD tray border adjustments */
  int adjust_tray_custom_left, adjust_tray_custom_right, adjust_tray_custom_top, adjust_tray_custom_bottom;
  int adjust_tray_A_left, adjust_tray_A_right, adjust_tray_A_top, adjust_tray_A_bottom;
  int adjust_tray_BCD_left, adjust_tray_BCD_right, adjust_tray_BCD_top, adjust_tray_BCD_bottom;
  int adjust_tray_E_left, adjust_tray_E_right, adjust_tray_E_top, adjust_tray_E_bottom;
  int adjust_tray_F_left, adjust_tray_F_right, adjust_tray_F_top, adjust_tray_F_bottom;
  int adjust_tray_G_left, adjust_tray_G_right, adjust_tray_G_top, adjust_tray_G_bottom;
  int adjust_tray_H_left, adjust_tray_H_right, adjust_tray_H_top, adjust_tray_H_bottom;
  int adjust_tray_J_left, adjust_tray_J_right, adjust_tray_J_top, adjust_tray_J_bottom;
  int adjust_tray_L_left, adjust_tray_L_right, adjust_tray_L_top, adjust_tray_L_bottom;
  int paper_width, paper_length;

  /* Canon printer firmware requires paper_width (and paper_length?)
     to be exact matches in units of 1/600 inch.
     To this end, papersize code is used to find the papersize for the
     printjob, and paper_width and paper_length set to exact values,
     rather than calculated.
  */
  unsigned char arg_ESCP_1 = (init->pt) ? canon_size_type(v,init->caps) : 0x03; /* default size A4 */
  stp_dprintf(STP_DBG_CANON, v,"setPageMargins2: arg_ESCP_1 = '%x'\n",arg_ESCP_1);

  /* TOFIX: what exactly is to be sent?
   * Is it the printable length or the bottom border?
   * Is is the printable width or the right border?
   */

  int unit = 600;
  int printable_width = (init->page_width + 1)*5/6;
  int printable_length = (init->page_height + 1)*5/6;

  const char* input_slot = stp_get_string_parameter(v, "InputSlot");
  int print_cd = (input_slot && (!strcmp(input_slot, "CD")));

  stp_dprintf(STP_DBG_CANON, v,"setPageMargins2: print_cd = %d\n",print_cd);

  test_cd = 1;
  /*
    Adjustments for different CD trays:
    All based on the following settings:
    CANON_CD_X = 176
    CANON_CD_Y = 405

    Custom:
    printable_width = init->page_width + 24;
    printable_length = init->page_height + 132;

    A:
    printable_width = init->page_width + 44;
    printable_length = init->page_height + 151;

    B/C/D:
    printable_width = init->page_width + 10;
    printable_length = init->page_height + 84;

    E:
    printable_width = init->page_width + 127;
    printable_length = init->page_height + 186;

    F:
    printable_width = init->page_width + 10;
    printable_length = init->page_height + 95;

    G:
    printable_width = init->page_width + 10;
    printable_length = init->page_height + 127;

    H:
    printable_width = init->page_width + 68;
    printable_length = init->page_height + 481;

    J:
    printable_width = init->page_width + 8;
    printable_length = init->page_height + 44;

    L:
    printable_width = init->page_width + 10;
    printable_length = init->page_height + 263;
  */

  /* adjust paper dimensions for CD in points */
  adjust_tray_custom_length = 132;
  adjust_tray_custom_width = 24;
  adjust_tray_A_length = 151;
  adjust_tray_A_width = 44;
  adjust_tray_BCD_length = 84;
  adjust_tray_BCD_width = 10;
  adjust_tray_E_length = 186;
  adjust_tray_E_width = 127;
  adjust_tray_F_length = 95;
  adjust_tray_F_width = 10; /* 10 calculated, but empirically 11 */
  adjust_tray_G_length = 127;
  adjust_tray_G_width = 10; /* 10 calculated, but empirically 11 */
  adjust_tray_H_length = 481;
  adjust_tray_H_width = 68;
  adjust_tray_J_length = 44;
  adjust_tray_J_width = 8;
  adjust_tray_L_length = 263;
  adjust_tray_L_width = 10;

  /* ensure less than or equal to Windows measurements  by border adjustments in points  */
  adjust_tray_custom_left = 0;
  adjust_tray_custom_right = 0;
  adjust_tray_custom_top = 0;
  adjust_tray_custom_bottom = 0;
  adjust_tray_A_left = 0;
  adjust_tray_A_right = 0;
  adjust_tray_A_top = 0;
  adjust_tray_A_bottom = 0;
  adjust_tray_BCD_left = 0;
  adjust_tray_BCD_right = 0;
  adjust_tray_BCD_top = 0;
  adjust_tray_BCD_bottom = 0;
  adjust_tray_E_left = 0;
  adjust_tray_E_right = 0;
  adjust_tray_E_top = 0;
  adjust_tray_E_bottom = 0;
  adjust_tray_F_left = 0;
  adjust_tray_F_right = 0;
  adjust_tray_F_top = 0;
  adjust_tray_F_bottom = 0;
  adjust_tray_G_left = 0;
  adjust_tray_G_right = 0;
  adjust_tray_G_top = 0;
  adjust_tray_G_bottom = 0;
  adjust_tray_H_left = 0;
  adjust_tray_H_right = 0;
  adjust_tray_H_top = 0;
  adjust_tray_H_bottom = 0;
  adjust_tray_J_left = 0;
  adjust_tray_J_right = 0;
  adjust_tray_J_top = 0;
  adjust_tray_J_bottom = 0;
  adjust_tray_L_left = 0;
  adjust_tray_L_right = 0;
  adjust_tray_L_top = 0;
  adjust_tray_L_bottom = 0;

  if ((print_cd) && test_cd) {
    /* test all of the parameters for CD */
    stp_dprintf(STP_DBG_CANON, v,"==========Start Test Printout=========='\n");

    stp_dprintf(STP_DBG_CANON, v, "Tray Custom original init->page_width (pts): '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom original init->page_height (pts): '%d'\n",init->page_height);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom modified init->page_width (pts): '%d'\n",init->page_width + adjust_tray_custom_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom modified init->page_height (pts): '%d'\n",init->page_height + adjust_tray_custom_length);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom printable width (pts): '%d'\n",(init->page_width + adjust_tray_custom_width + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom printable length (pts): '%d'\n",(init->page_height + adjust_tray_custom_length + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom page_width (1/600): '%d'\n",(init->page_width + adjust_tray_custom_width) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom page_height (1/600): '%d'\n",(init->page_height + adjust_tray_custom_length) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom paper_width (1/600): '%d'\n",(init->page_width + adjust_tray_custom_width + 20 + adjust_tray_custom_left + adjust_tray_custom_right) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom paper_height (1/600): '%d'\n",(init->page_height + adjust_tray_custom_length + 24 + adjust_tray_custom_top + adjust_tray_custom_bottom) * unit / 72);

    stp_dprintf(STP_DBG_CANON, v, "Tray A original init->page_width (pts): '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray A original init->page_height (pts): '%d'\n",init->page_height);
    stp_dprintf(STP_DBG_CANON, v, "Tray A modified init->page_width (pts): '%d'\n",init->page_width + adjust_tray_A_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray A modified init->page_height (pts): '%d'\n",init->page_height + adjust_tray_A_length);
    stp_dprintf(STP_DBG_CANON, v, "Tray A printable width (pts): '%d'\n",(init->page_width + adjust_tray_A_width + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray A printable length (pts): '%d'\n",(init->page_height + adjust_tray_A_length + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray A page_width (1/600): '%d'\n",(init->page_width + adjust_tray_A_width) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray A page_height (1/600): '%d'\n",(init->page_height + adjust_tray_A_length) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray A paper_width (1/600): '%d'\n",(init->page_width + adjust_tray_A_width + 20 + adjust_tray_A_left + adjust_tray_A_right ) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray A paper_height (1/600): '%d'\n",(init->page_height + adjust_tray_A_length + 24 + adjust_tray_A_top + adjust_tray_A_bottom) * unit / 72);

    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D original init->page_width (pts): '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D original init->page_height (pts): '%d'\n",init->page_height);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D modified init->page_width (pts): '%d'\n",init->page_width + adjust_tray_BCD_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D modified init->page_height (pts): '%d'\n",init->page_height + adjust_tray_BCD_length);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D printable width (pts): '%d'\n",(init->page_width + adjust_tray_BCD_width + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D printable length (pts): '%d'\n",(init->page_height + adjust_tray_BCD_length + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D page_width (1/600): '%d'\n",(init->page_width + adjust_tray_BCD_width) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D page_height (1/600): '%d'\n",(init->page_height + adjust_tray_BCD_length) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D paper_width (1/600): '%d'\n",(init->page_width + adjust_tray_BCD_width + 20 + adjust_tray_BCD_left + adjust_tray_BCD_right) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D paper_height (1/600): '%d'\n",(init->page_height + adjust_tray_BCD_length + 24 + adjust_tray_BCD_top + adjust_tray_BCD_bottom) * unit / 72);

    stp_dprintf(STP_DBG_CANON, v, "Tray E original init->page_width (pts): '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray E original init->page_height (pts): '%d'\n",init->page_height);
    stp_dprintf(STP_DBG_CANON, v, "Tray E modified init->page_width (pts): '%d'\n",init->page_width + adjust_tray_E_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray E modified init->page_height (pts): '%d'\n",init->page_height + adjust_tray_E_length);
    stp_dprintf(STP_DBG_CANON, v, "Tray E printable width (pts): '%d'\n",(init->page_width + adjust_tray_E_width + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray E printable length (pts): '%d'\n",(init->page_height + adjust_tray_E_length + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray E page_width (1/600): '%d'\n",(init->page_width + adjust_tray_E_width) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray E page_height (1/600): '%d'\n",(init->page_height + adjust_tray_E_length) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray E paper_width (1/600): '%d'\n",(init->page_width + adjust_tray_E_width + 20 + adjust_tray_E_left + adjust_tray_E_right) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray E paper_height (1/600): '%d'\n",(init->page_height + adjust_tray_E_length + 24 + adjust_tray_E_top + adjust_tray_E_bottom) * unit / 72);

    stp_dprintf(STP_DBG_CANON, v, "Tray F original init->page_width (pts): '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray F original init->page_height (pts): '%d'\n",init->page_height);
    stp_dprintf(STP_DBG_CANON, v, "Tray F modified init->page_width (pts): '%d'\n",init->page_width + adjust_tray_F_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray F modified init->page_height (pts): '%d'\n",init->page_height + adjust_tray_F_length);
    stp_dprintf(STP_DBG_CANON, v, "Tray F printable width (pts): '%d'\n",(init->page_width + adjust_tray_F_width + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray F printable length (pts): '%d'\n",(init->page_height + adjust_tray_F_length + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray F page_width (1/600): '%d'\n",(init->page_width + adjust_tray_F_width) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray F page_height (1/600): '%d'\n",(init->page_height + adjust_tray_F_length) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray F paper_width (1/600): '%d'\n",(init->page_width + adjust_tray_F_width + 20 + adjust_tray_F_left + adjust_tray_F_right) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray F paper_height (1/600): '%d'\n",(init->page_height + adjust_tray_F_length + 24 + adjust_tray_F_top + adjust_tray_F_bottom) * unit / 72);

    stp_dprintf(STP_DBG_CANON, v, "Tray G original init->page_width (pts): '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray G original init->page_height (pts): '%d'\n",init->page_height);
    stp_dprintf(STP_DBG_CANON, v, "Tray G modified init->page_width (pts): '%d'\n",init->page_width + adjust_tray_G_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray G modified init->page_height (pts): '%d'\n",init->page_height + adjust_tray_G_length);
    stp_dprintf(STP_DBG_CANON, v, "Tray G printable width (pts): '%d'\n",(init->page_width + adjust_tray_G_width + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray G printable length (pts): '%d'\n",(init->page_height + adjust_tray_G_length + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray G page_width (1/600): '%d'\n",(init->page_width + adjust_tray_G_width) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray G page_height (1/600): '%d'\n",(init->page_height + adjust_tray_G_length) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray G paper_width (1/600): '%d'\n",(init->page_width + adjust_tray_G_width + 20 + adjust_tray_G_left + adjust_tray_G_right) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray G paper_height (1/600): '%d'\n",(init->page_height + adjust_tray_G_length + 24 + adjust_tray_G_top + adjust_tray_G_bottom) * unit / 72);

    stp_dprintf(STP_DBG_CANON, v, "Tray H original init->page_width (pts): '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray H original init->page_height (pts): '%d'\n",init->page_height);
    stp_dprintf(STP_DBG_CANON, v, "Tray H modified init->page_width (pts): '%d'\n",init->page_width + adjust_tray_H_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray H modified init->page_height (pts): '%d'\n",init->page_height + adjust_tray_H_length);
    stp_dprintf(STP_DBG_CANON, v, "Tray H printable width (pts): '%d'\n",(init->page_width + adjust_tray_H_width + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray H printable length (pts): '%d'\n",(init->page_height + adjust_tray_H_length + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray H page_width (1/600): '%d'\n",(init->page_width + adjust_tray_H_width) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray H page_height (1/600): '%d'\n",(init->page_height + adjust_tray_H_length) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray H paper_width (1/600): '%d'\n",(init->page_width + adjust_tray_H_width + 20 + adjust_tray_H_left + adjust_tray_H_right) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray H paper_height (1/600): '%d'\n",(init->page_height + adjust_tray_H_length + 24 + adjust_tray_H_top + adjust_tray_H_bottom) * unit / 72);

    stp_dprintf(STP_DBG_CANON, v, "Tray J original init->page_width (pts): '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray J original init->page_height (pts): '%d'\n",init->page_height);
    stp_dprintf(STP_DBG_CANON, v, "Tray J modified init->page_width (pts): '%d'\n",init->page_width + adjust_tray_J_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray J modified init->page_height (pts): '%d'\n",init->page_height + adjust_tray_J_length);
    stp_dprintf(STP_DBG_CANON, v, "Tray J printable width (pts): '%d'\n",(init->page_width + adjust_tray_J_width + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray J printable length (pts): '%d'\n",(init->page_height + adjust_tray_J_length + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray J page_width (1/600): '%d'\n",(init->page_width + adjust_tray_J_width) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray J page_height (1/600): '%d'\n",(init->page_height + adjust_tray_J_length) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray J paper_width (1/600): '%d'\n",(init->page_width + adjust_tray_J_width + 20 + adjust_tray_J_left + adjust_tray_J_right) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray J paper_height (1/600): '%d'\n",(init->page_height + adjust_tray_J_length + 24 + adjust_tray_J_top + adjust_tray_J_bottom) * unit / 72);

    stp_dprintf(STP_DBG_CANON, v, "Tray L original init->page_width (pts): '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray L original init->page_height (pts): '%d'\n",init->page_height);
    stp_dprintf(STP_DBG_CANON, v, "Tray L modified init->page_width (pts): '%d'\n",init->page_width + adjust_tray_L_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray L modified init->page_height (pts): '%d'\n",init->page_height + adjust_tray_L_length);
    stp_dprintf(STP_DBG_CANON, v, "Tray L printable width (pts): '%d'\n",(init->page_width + adjust_tray_L_width + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray L printable length (pts): '%d'\n",(init->page_height + adjust_tray_L_length + 1) * 5 / 6);
    stp_dprintf(STP_DBG_CANON, v, "Tray L page_width (1/600): '%d'\n",(init->page_width + adjust_tray_L_width) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray L page_height (1/600): '%d'\n",(init->page_height + adjust_tray_L_length) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray L paper_width (1/600): '%d'\n",(init->page_width + adjust_tray_L_width + 20 + adjust_tray_L_left + adjust_tray_L_right) * unit / 72);
    stp_dprintf(STP_DBG_CANON, v, "Tray L paper_height (1/600): '%d'\n",(init->page_height + adjust_tray_L_length + 24 + adjust_tray_L_top + adjust_tray_L_bottom) * unit / 72);

    stp_dprintf(STP_DBG_CANON, v,"==========End Test Printout=========='\n");
  }

  /* Tray Custom */
  if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA MP710")) && (test_cd==1) ) {

    init->page_width +=24;
    init->page_height +=132;

    stp_dprintf(STP_DBG_CANON, v, "Tray Custom modified init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom modified init->page_height: '%d'\n",init->page_height);

    printable_width=  (init->page_width+ 1)*5/6;
    printable_length= (init->page_height + 1)*5/6;

    stp_dprintf(STP_DBG_CANON, v, "Tray Custom modified printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray Custom modified printable_length: '%d'\n",printable_length);

  }

  /* Tray A */
  if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP9910")) && (test_cd==1) ) {

    init->page_width +=44;
    init->page_height +=151;

    stp_dprintf(STP_DBG_CANON, v, "Tray A modified init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray A modified init->page_height: '%d'\n",init->page_height);

    printable_width=  (init->page_width+ 1)*5/6;
    printable_length= (init->page_height + 1)*5/6;

    stp_dprintf(STP_DBG_CANON, v, "Tray A modified printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray A modified printable_length: '%d'\n",printable_length);

  }

  /* Tray B,C,D */
  if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP3000")) && (test_cd==1) ) {

    init->page_width +=10;
    init->page_height +=84;

    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D modified init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/B modified init->page_height: '%d'\n",init->page_height);

    printable_width=  (init->page_width+ 1)*5/6;
    printable_length= (init->page_height + 1)*5/6;

    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D modified printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray B/C/D modified printable_length: '%d'\n",printable_length);

  }

  /* Tray E */
  if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA Pro9000")) && (test_cd==1) ) {

    init->page_width +=127;
    init->page_height +=186;

    stp_dprintf(STP_DBG_CANON, v, "Tray E modified init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray E modified init->page_height: '%d'\n",init->page_height);

    printable_width=  (init->page_width+ 1)*5/6;
    printable_length= (init->page_height + 1)*5/6;

    stp_dprintf(STP_DBG_CANON, v, "Tray E modified printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray E modified printable_length: '%d'\n",printable_length);

  }

  /* Tray F */
  if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP4500")) && (test_cd==1) ) {
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: Tray F init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: Tray F init->page_height: '%d'\n",init->page_height);

    stp_dprintf(STP_DBG_CANON, v, "Tray F init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray F init->page_height: '%d'\n",init->page_height);
    stp_dprintf(STP_DBG_CANON, v, "Tray F initial printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray F initial printable_length: '%d'\n",printable_length);

    init->page_width +=10;
    init->page_height +=95;

    stp_dprintf(STP_DBG_CANON, v, "Tray F modified init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray F modified init->page_height: '%d'\n",init->page_height);

    printable_width=  (init->page_width+ 1)*5/6;
    printable_length= (init->page_height + 1)*5/6;

    stp_dprintf(STP_DBG_CANON, v, "Tray F modified printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray F modified printable_length: '%d'\n",printable_length);

    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: Tray F modified printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: Tray F modified printable_length: '%d'\n",printable_length);
  }

  /* Tray G */
  if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP4700")) && (test_cd==1) ) {

    init->page_width +=10;
    init->page_height +=127;

    stp_dprintf(STP_DBG_CANON, v, "Tray G modified init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray G modified init->page_height: '%d'\n",init->page_height);

    printable_width=  (init->page_width+ 1)*5/6;
    printable_length= (init->page_height + 1)*5/6;

    stp_dprintf(STP_DBG_CANON, v, "Tray G modified printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray G modified printable_length: '%d'\n",printable_length);

  }

  /* Tray H */
  if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA PRO-1")) && (test_cd==1) ) {

    init->page_width +=68;
    init->page_height +=481;

    stp_dprintf(STP_DBG_CANON, v, "Tray H modified init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray H modified init->page_height: '%d'\n",init->page_height);

    printable_width=  (init->page_width+ 1)*5/6;
    printable_length= (init->page_height + 1)*5/6;

    stp_dprintf(STP_DBG_CANON, v, "Tray H modified printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray H modified printable_length: '%d'\n",printable_length);

  }

  /* Tray J */
  if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP7200")) && (test_cd==1) ) {

    init->page_width +=8;
    init->page_height +=44;

    stp_dprintf(STP_DBG_CANON, v, "Tray J modified init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray J modified init->page_height: '%d'\n",init->page_height);

    printable_width=  (init->page_width+ 1)*5/6;
    printable_length= (init->page_height + 1)*5/6;

    stp_dprintf(STP_DBG_CANON, v, "Tray J modified printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray J modified printable_length: '%d'\n",printable_length);

  }

  /* Tray L */
  if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP8700")) && (test_cd==1) ) {

    init->page_width +=10;
    init->page_height +=263;

    stp_dprintf(STP_DBG_CANON, v, "Tray L modified init->page_width: '%d'\n",init->page_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray L modified init->page_height: '%d'\n",init->page_height);

    printable_width=  (init->page_width+ 1)*5/6;
    printable_length= (init->page_height + 1)*5/6;

    stp_dprintf(STP_DBG_CANON, v, "Tray L modified printable_width: '%d'\n",printable_width);
    stp_dprintf(STP_DBG_CANON, v, "Tray L modified printable_length: '%d'\n",printable_length);

  }

  /* Tray M - TODO */
  /* to add code here */

  if ( (init->caps->features & CANON_CAP_BORDERLESS) &&
       !(print_cd) && stp_get_boolean_parameter(v, "FullBleed") )
    {
      stp_dprintf(STP_DBG_CANON, v,"canon_init_setPageMargins2: for borderless set printable length and width to 0\n");
      /* set to 0 for borderless */
      printable_width = 0;
      printable_length = 0;
    }

  /* arguments for short form of Esc (p command  */
  arg_70_1= (printable_length >> 8) & 0xff;
  arg_70_2= (printable_length) & 0xff;
  arg_70_3= (printable_width >> 8) & 0xff;
  arg_70_4= (printable_width) & 0xff;


  if (!(init->caps->features & CANON_CAP_px) && !(init->caps->features & CANON_CAP_p))
	return;

  if ((init->caps->features & CANON_CAP_px) ) {
    /* workaround for CD writing that uses CANON_CAP_px --- fix with capabilities */
    if ( !( input_slot && !(strcmp(input_slot,"CD")) ) || !(strcmp(init->caps->name,"PIXMA iP4500")) || !(strcmp(init->caps->name,"PIXMA iP4600")) || !(strcmp(init->caps->name,"PIXMA iP4700")) || !(strcmp(init->caps->name,"PIXMA iP4800")) || !(strcmp(init->caps->name,"PIXMA iP4900")) || !(strcmp(init->caps->name,"PIXMA iP7200")) || !(strcmp(init->caps->name,"PIXMA MP980")) || !(strcmp(init->caps->name,"PIXMA MP990")) || !(strcmp(init->caps->name,"PIXMA MG5200")) || !(strcmp(init->caps->name,"PIXMA MG5300")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG6500")) || !(strcmp(init->caps->name,"PIXMA MG6700")) || !(strcmp(init->caps->name,"PIXMA MG7500")) || !(strcmp(init->caps->name,"PIXMA MG7700")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA MP710")) || !(strcmp(init->caps->name,"PIXMA iP3000")) || !(strcmp(init->caps->name,"PIXMA Pro9000")) || !(strcmp(init->caps->name,"PIXMA iP8700")) || !(strcmp(init->caps->name,"PIXMA iX6800")) || !(strcmp(init->caps->name,"PIXMA TS8000")) )
      /* need to check if iP9910, MP710, iP3000, Pro9000 use Esc (p */
      {

	/* original borders */
	border_left=init->caps->border_left;
	border_right=init->caps->border_right;
	border_top=init->caps->border_top;
	border_bottom=init->caps->border_bottom;

	if (print_cd) {
	  border_top=9;
	  border_bottom=15; /* was 9 originally */
	}

	/* modified borders */
	border_left2=border_left;
	border_top2=border_top;
	border_right2=border_right;
	border_bottom2=border_bottom;

	if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA MP710")) && (test_cd==1) ) {
	  border_left2   = border_left + adjust_tray_custom_left;
	  border_right2  = border_right + adjust_tray_custom_right;
	  border_top2    = border_top + adjust_tray_custom_top;
	  border_bottom2 = border_bottom + adjust_tray_custom_bottom;
	}

	if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP9910")) && (test_cd==1) ) {
	  border_left2   = border_left + adjust_tray_A_left;
	  border_right2  = border_right + adjust_tray_A_right;
	  border_top2    = border_top + adjust_tray_A_top;
	  border_bottom2 = border_bottom + adjust_tray_A_bottom;
	}

	if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP3000")) && (test_cd==1) ) {
	  border_left2   = border_left + adjust_tray_BCD_left;
	  border_right2  = border_right + adjust_tray_BCD_right;
	  border_top2    = border_top + adjust_tray_BCD_top;
	  border_bottom2 = border_bottom + adjust_tray_BCD_bottom;
	}

	if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA Pro9000")) && (test_cd==1) ) {
	  border_left2   = border_left + adjust_tray_E_left;
	  border_right2  = border_right + adjust_tray_E_right;
	  border_top2    = border_top + adjust_tray_E_top;
	  border_bottom2 = border_bottom + adjust_tray_E_bottom;
	}

	if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP4500")) && (test_cd==1) ) {
	  border_left2   = border_left + adjust_tray_F_left;
	  border_right2  = border_right + adjust_tray_F_right;
	  border_top2    = border_top + adjust_tray_F_top;
	  border_bottom2 = border_bottom + adjust_tray_F_bottom;
	}

	if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP4700")) && (test_cd==1) ) {
	  border_left2   = border_left + adjust_tray_G_left;
	  border_right2  = border_right + adjust_tray_G_right;
	  border_top2    = border_top + adjust_tray_G_top;
	  border_bottom2 = border_bottom + adjust_tray_G_bottom;
	}

	if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA PRO-1")) && (test_cd==1) ) {
	  border_left2   = border_left + adjust_tray_H_left;
	  border_right2  = border_right + adjust_tray_H_right;
	  border_top2    = border_top + adjust_tray_H_top;
	  border_bottom2 = border_bottom + adjust_tray_H_bottom;
	}

	if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP7200")) && (test_cd==1) ) {
	  border_left2   = border_left + adjust_tray_J_left;
	  border_right2  = border_right + adjust_tray_J_right;
	  border_top2    = border_top + adjust_tray_J_top;
	  border_bottom2 = border_bottom + adjust_tray_J_bottom;
	}

	if ( (print_cd) && !(strcmp(init->caps->name,"PIXMA iP8700")) && (test_cd==1) ) {
	  border_left2   = border_left + adjust_tray_L_left;
	  border_right2  = border_right + adjust_tray_L_right;
	  border_top2    = border_top + adjust_tray_L_top;
	  border_bottom2 = border_bottom + adjust_tray_L_bottom;
	}

	/* this does not seem to need adjustment, so use original borders */
	area_right = border_left * unit / 72;
	area_top = border_top * unit / 72;

	if ( (init->caps->features & CANON_CAP_BORDERLESS) &&
	     !(print_cd) && stp_get_boolean_parameter(v, "FullBleed") ) {
	  border_left2=-8; /* -8 mini series -6 */
	  border_right2=-8; /* -8 */
	  border_top2=-6; /* -6 standard */
	  border_bottom2=-15; /* -15 standard */
	  area_right = border_left2 * unit / 72;
	  area_top = border_top2 * unit / 72;
	}

	stp_dprintf(STP_DBG_CANON, v,"setPageMargins2: init->page_height = %d\n",init->page_height);
	stp_dprintf(STP_DBG_CANON, v,"setPageMargins2: printable_length = %d\n",printable_length);
	stp_dprintf(STP_DBG_CANON, v,"setPageMargins2: paper_height = %d\n",(init->page_height + border_top + border_bottom) * unit / 72);


	stp_dprintf(STP_DBG_CANON, v, "final init->page_width: '%d'\n",init->page_width);
	stp_dprintf(STP_DBG_CANON, v, "final init->page_height: '%d'\n",init->page_height);
	stp_dprintf(STP_DBG_CANON, v, "final printable_width: '%d'\n",printable_width);
	stp_dprintf(STP_DBG_CANON, v, "final printable_length: '%d'\n",printable_length);

	stp_zfwrite(ESC28,2,1,v); /* ESC( */
	stp_putc(0x70,v);         /* p    */
	stp_put16_le(46, v);      /* len  */
	/* 0 for borderless, calculated otherwise */
	stp_put16_be(printable_length,v); /* printable_length */
	stp_put16_be(0,v);
	/* 0 for borderless, calculated otherwise */
	stp_put16_be(printable_width,v); /* printable_width */
	stp_put16_be(0,v);
	stp_put32_be(0,v);
	stp_put16_be(unit,v);

	/* depends on borderless or not: uses modified borders */
	stp_put32_be(area_right,v); /* area_right : Windows seems to use 9.6, gutenprint uses 10 */
	stp_put32_be(area_top,v);  /* area_top : Windows seems to use 8.4, gutenprint uses 15 */

	/* calculated depending on borderless or not: uses modified borders */
	if ( (init->caps->features & CANON_CAP_BORDERLESS) &&
	     !(print_cd) && stp_get_boolean_parameter(v, "FullBleed") ) {
	  /* borderless */
	  stp_put32_be((init->page_width - border_left2 - border_right2 ) * unit / 72,v); /* area_width */
	  stp_put32_be((init->page_height - border_top2 - border_bottom2 ) * unit / 72,v); /* area_length */
	}
	else {
	  if ( (print_cd) && (test_cd==1) ) { /* bordered for CD */
	    stp_put32_be(init->page_width * unit / 72,v); /* area_width */
	    stp_put32_be(init->page_height * unit / 72,v); /* area_length */
	  }
	  else { /* bordered non CD media */
	    stp_put32_be((init->page_width) * unit / 72,v); /* area_width */
	    stp_put32_be((init->page_height) * unit / 72,v); /* area_length */
	  }
	}

	/* 0 under all currently known circumstances */
	stp_put32_be(0,v); /* paper_right : Windows also 0 here for all Trays */
	stp_put32_be(0,v); /* paper_top : Windows also 0 here for all Trays */

	/* standard paper sizes, unchanged for borderless so use
	   original borders */

	/* discovered that paper_width needs to be same as Windows
	   dimensions for Canon printer firmware to automatically
	   determine which tray to pull paper from automatically.
	*/

	if ( (init->caps->features & CANON_CAP_BORDERLESS) &&
	     !(print_cd) && stp_get_boolean_parameter(v, "FullBleed") ) {
	  /* borderless */
	  stp_put32_be((init->page_width) * unit / 72,v); /* paper_width */
	  stp_put32_be((init->page_height) * unit / 72,v); /* paper_length */
	}
	else {
	  if ( (print_cd) && (test_cd==1) ) { /* bordered for CD */
	    paper_width = (init->page_width + border_left2 + border_right2) * unit / 72; /* paper_width */
	    paper_length = (init->page_height + border_top2 + border_bottom2) * unit / 72; /* paper_length */
	    fix_papersize(arg_ESCP_1, &paper_width, &paper_length);
	    stp_put32_be(paper_width,v); /* paper_width */
	    stp_put32_be(paper_length,v); /* paper_length */
	  }
	  else { /* bordered non CD media */
            /* set by calculation first, then correct if necessary */
	    paper_width = (init->page_width + border_left + border_right) * unit / 72; /* paper_width */
	    paper_length = (init->page_height + border_top + border_bottom) * unit / 72; /* paper_length */
            fix_papersize(arg_ESCP_1, &paper_width, &paper_length);
	    stp_put32_be(paper_width,v); /* paper_width */
	    stp_put32_be(paper_length,v); /* paper_length */
	  }
	}

	return;
      }
  }
  canon_cmd(v,ESC28,0x70, 8,
   	      arg_70_1, arg_70_2, 0x00, 0x00,
	      arg_70_3, arg_70_4, 0x00, 0x00);
}

/* ESC (P -- 0x50 -- unknown -- :
   Seems to set media and page information. Different byte lengths
   depending on printer model.
   Page rotation option in driver [ESC (v] influences ESC (P command
   parameters 5 and 6:
       none: 1 0
     90 deg: 2 0
    180 deg: 1 1
    270 deg: 2 1
*/
static void
canon_init_setESC_P(const stp_vars_t *v, const canon_privdata_t *init)
{
  unsigned char arg_ESCP_1, arg_ESCP_2, arg_ESCP_5, arg_ESCP_6, arg_ESCP_7, arg_ESCP_9;

  stp_dimension_t width, length;
  /*  const char *media_size = stp_get_string_parameter(v, "PageSize");
      const stp_papersize_t *pt = NULL; */
  const char* orientation_type = stp_get_string_parameter(v, "Orientation");
  const char* input_slot = stp_get_string_parameter(v, "InputSlot");
  const char* input_tray = stp_get_string_parameter(v, "CassetteTray");
  /* const canon_cap_t * caps= canon_get_model_capabilities(v); */
  int print_cd = (input_slot && (!strcmp(input_slot, "CD")));
  int tray_upper = (input_tray && (!strcmp(input_tray, "Upper")));
  int tray_lower = (input_tray && (!strcmp(input_tray, "Lower")));
  int tray_user_select;
  unsigned char user_ESCP_9;

  if(!(init->caps->features & CANON_CAP_P))
    return;

  /*  if (media_size)
      pt = stp_describe_papersize(v, media_size); */
  stp_default_media_size(v, &width, &length);
  if (tray_upper || tray_lower)
    tray_user_select=1;
  else
    tray_user_select=0;
  if (tray_upper)
    user_ESCP_9=0x01;
  else if (tray_lower)
    user_ESCP_9=0x02;
  else
    user_ESCP_9=0x00; /* fall-through setting, but this value is not used */

  arg_ESCP_1 = (init->pt) ? canon_size_type(v,init->caps): 0x03; /* media size: set to A4 size as default */
  stp_dprintf(STP_DBG_CANON, v,"canon: ESCP (P code read paper size, resulting arg_ESCP_1: '%x'\n",arg_ESCP_1);
  arg_ESCP_2 = (init->pt) ? init->pt->media_code_P: 0x00; /* media type: set to plain as default  */
  arg_ESCP_5 = 0x01; /* default for portrait orientation */
  arg_ESCP_6 = 0x00; /* default for portrait orientation */

  arg_ESCP_7 = 0x01; /* default - use unknown, with some printers set
			to 0x00 for envelope sizes (and other?)
			regardless of media type */

  if( orientation_type && !strcmp(orientation_type,"Portrait")) { /* none */
    arg_ESCP_5 = 0x01;
    arg_ESCP_6 = 0x00;
  }
  else if( orientation_type && !strcmp(orientation_type,"Landscape")) { /* 90 deg */
    arg_ESCP_5 = 0x02;
    arg_ESCP_6 = 0x00;
  }
  else if( orientation_type && !strcmp(orientation_type,"UpsideDown")) { /* 180 deg */
    arg_ESCP_5 = 0x01;
    arg_ESCP_6 = 0x01;
  }
  else if( orientation_type && !strcmp(orientation_type,"Seascape")) { /* 270 deg */
    arg_ESCP_5 = 0x02;
    arg_ESCP_6 = 0x01;
  }


  /* Code for last argument in 9-byte ESC (P printers with and upper and lower tray included in the cassette input source
     The intention appears to be to allow printing of photos and non-photo paper without needing to change trays.
     Note, envelopes are printed from the lower tray.

     Upper tray specification from MG7700 user manual:
     Min width:  3.5  inches /  89.0 mm  (L size)
     Min height: 5    inches / 127.0 mm  (L size)
     Max width:  7.87 inches / 200.0 mm  (marked up to 5 inches)
     Max height: 7.28 inches / 184.9 mm

     Lower tray specification:
     Min width:  3.54 inches /  90.0 mm (takes business envelopes)
     Min height: 7.29 inches / 185.0 mm
     Max width:  8.5  inches / 215.9 mm
     Max height: 14   inches / 355.6 mm

     Conditions:
     Init:  Upper tray
     Media:
            Hagaki, Photo media --> upper tray
	    Envelopes  (argESCP_2 == 0x08) --> lower tray
	    Other --> lower tray
     Size:  Length:
            < 7.29 inches (524 pt = 184.85 mm; 525 pt = 185.2 mm ) --> upper tray
            >=7.29 inches (524 pt = 184.85 mm; 525 pt = 185.2 mm ) --> lower tray

     Printers with this capability:
     Code 0xd for cassette (standard cassette code is 0x8)

     MG5400
     MG6300
     MG6500
     MG6700
     MG6900 - same as MG7700
     MG7500
     MG7700
     iP7200
     MX720
     MX920
   */


  if ( !(strcmp(init->caps->name,"PIXMA iP7200")) || !(strcmp(init->caps->name,"PIXMA MG5400")) || !(strcmp(init->caps->name,"PIXMA MG6300")) || !(strcmp(init->caps->name,"PIXMA MG6500")) || !(strcmp(init->caps->name,"PIXMA MG6700")) || !(strcmp(init->caps->name,"PIXMA MG7500")) || !(strcmp(init->caps->name,"PIXMA MG7700")) || !(strcmp(init->caps->name,"PIXMA MX720")) || !(strcmp(init->caps->name,"PIXMA MX920")) ) {
    /* default: use upper tray of cassette with two trays, condition check later */
    arg_ESCP_9 = 0x01;
  }
  else if ( !(strcmp(init->caps->name,"PIXMA E400")) || !(strcmp(init->caps->name,"PIXMA E460")) ||  !(strcmp(init->caps->name,"PIXMA E470")) || !(strcmp(init->caps->name,"PIXMA E480")) || !(strcmp(init->caps->name,"PIXMA E560")) || !(strcmp(init->caps->name,"PIXMA E3100")) || !(strcmp(init->caps->name,"PIXMA TS5000")) || !(strcmp(init->caps->name,"PIXMA TS6000")) || !(strcmp(init->caps->name,"PIXMA TS8000")) || !(strcmp(init->caps->name,"PIXMA G1000")) || !(strcmp(init->caps->name,"PIXMA G4000")) || !(strcmp(init->caps->name,"PIXMA MG2900")) || !(strcmp(init->caps->name,"PIXMA MG3500")) || !(strcmp(init->caps->name,"PIXMA MG3600")) || !(strcmp(init->caps->name,"PIXMA MG5500")) || !(strcmp(init->caps->name,"PIXMA MG5600")) || !(strcmp(init->caps->name,"PIXMA iP110")) || !(strcmp(init->caps->name,"PIXMA iP2800")) || !(strcmp(init->caps->name,"PIXMA iP8700")) || !(strcmp(init->caps->name,"PIXMA iX6800")) || !(strcmp(init->caps->name,"MAXIFY iB4000")) || !(strcmp(init->caps->name,"MAXIFY iB4100")) || !(strcmp(init->caps->name,"MAXIFY MB2000")) || !(strcmp(init->caps->name,"MAXIFY MB2100")) || !(strcmp(init->caps->name,"MAXIFY MB2300")) || !(strcmp(init->caps->name,"MAXIFY MB2700")) || !(strcmp(init->caps->name,"PIXMA MX470")) || !(strcmp(init->caps->name,"PIXMA MX490")) ) {
    arg_ESCP_9 = 0xff;
  }

  else {
    arg_ESCP_9 = 0x00;
  }

  if ( !(strcmp(init->caps->name,"PIXMA E470")) || !(strcmp(init->caps->name,"PIXMA G4000")) || !(strcmp(init->caps->name,"PIXMA TS5000")) || !(strcmp(init->caps->name,"PIXMA TS6000")) || !(strcmp(init->caps->name,"PIXMA TS8000")) ){
    /* all envelope sizes it appears, even if media is not Envelope */
    /* currently not sure about custom sizes, thus only specify here the known Canon envelope sizes */
      switch(arg_ESCP_2)
	{
	case 0x2e: arg_ESCP_7=0x00; break;; /* US Comm #10 */
	case 0x2f: arg_ESCP_7=0x00; break;; /* Euro DL Env */
	case 0x30: arg_ESCP_7=0x00; break;; /* Western Env #4 (you4) */
	case 0x31: arg_ESCP_7=0x00; break;; /* Western Env #6 (you6) */
	case 0x3a: arg_ESCP_7=0x00; break;; /* Japanese Long Env #3 (chou3) */
	case 0x3b: arg_ESCP_7=0x00; break;; /* Japanese Long Env #4 (chou4) */
	  /* new envelopes for some printers added for completeness */
	case 0x92: arg_ESCP_7=0x00; break;; /* C5 */
	case 0x93: arg_ESCP_7=0x00; break;; /* Monarch */
	}
  }

  /* workaround for CD media */

  if ( (arg_ESCP_2 == 0x1f) || ( arg_ESCP_2 == 0x20) ) {
    if ( arg_ESCP_1 == 0x53 ) {
      /* Tray G as default */
      arg_ESCP_1 = 0x53;
      /* Custom CD tray */
      if ( !(strcmp(init->caps->name,"i865")) || !(strcmp(init->caps->name,"PIXMA MP710")) || !(strcmp(init->caps->name,"PIXMA MP740")) || !(strcmp(init->caps->name,"PIXMA MP900")) ) {
	arg_ESCP_1 = 0x35;
      }
      /* Tray A */
      if ( !(strcmp(init->caps->name,"PIXMA iP9910")) ) {
	arg_ESCP_1 = 0x3f;
      }
      /* Tray B */
      if ( !(strcmp(init->caps->name,"PIXMA MP750")) || !(strcmp(init->caps->name,"PIXMA MP760")) || !(strcmp(init->caps->name,"PIXMA MP770")) || !(strcmp(init->caps->name,"PIXMA MP780")) || !(strcmp(init->caps->name,"PIXMA MP790")) || !(strcmp(init->caps->name,"PIXMA iP3000")) || !(strcmp(init->caps->name,"PIXMA iP3100")) || !(strcmp(init->caps->name,"PIXMA iP4000")) || !(strcmp(init->caps->name,"PIXMA iP4100")) || !(strcmp(init->caps->name,"PIXMA iP5000")) || !(strcmp(init->caps->name,"PIXMA iP6000")) || !(strcmp(init->caps->name,"PIXMA iP6100")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8500")) || !(strcmp(init->caps->name,"PIXMA iP8600")) ) {
	arg_ESCP_1 = 0x40;
      }
      /* Tray C */
      if ( !(strcmp(init->caps->name,"PIXMA MP950")) || !(strcmp(init->caps->name,"PIXMA iP4200")) || !(strcmp(init->caps->name,"PIXMA iP5200")) || !(strcmp(init->caps->name,"PIXMA iP6700")) || !(strcmp(init->caps->name,"PIXMA iP7500")) ) {
	arg_ESCP_1 = 0x4a;
      }
      /* Tray D */
      if ( !(strcmp(init->caps->name,"PIXMA MP500")) || !(strcmp(init->caps->name,"PIXMA MP530")) || !(strcmp(init->caps->name,"PIXMA MP800")) || !(strcmp(init->caps->name,"PIXMA MP830")) ) {
	arg_ESCP_1 = 0x4b;
      }
      /* Tray E */
      if ( !(strcmp(init->caps->name,"PIXMA Pro9000")) || !(strcmp(init->caps->name,"PIXMA Pro9002")) || !(strcmp(init->caps->name,"PIXMA Pro9500")) || !(strcmp(init->caps->name,"PIXMA Pro95002")) ) {
	arg_ESCP_1 = 0x4c;
      }
      /* Tray F */
      if ( !(strcmp(init->caps->name,"PIXMA MP600")) || !(strcmp(init->caps->name,"PIXMA MP610")) || !(strcmp(init->caps->name,"PIXMA MP810")) || !(strcmp(init->caps->name,"PIXMA MP960")) || !(strcmp(init->caps->name,"PIXMA MP970")) || !(strcmp(init->caps->name,"PIXMA MX850")) || !(strcmp(init->caps->name,"PIXMA iP4300")) || !(strcmp(init->caps->name,"PIXMA iP4500")) || !(strcmp(init->caps->name,"PIXMA iP5300")) ) {
	arg_ESCP_1 = 0x51;
      }
      /* Tray G from iP4600 onwards */
      if ( !(strcmp(init->caps->name,"PIXMA iP4600")) || !(strcmp(init->caps->name,"PIXMA iP4700")) || !(strcmp(init->caps->name,"PIXMA iP4800")) || !(strcmp(init->caps->name,"PIXMA iP4900")) || !(strcmp(init->caps->name,"PIXMA MG5200")) || !(strcmp(init->caps->name,"PIXMA MG5300")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) ) {
	arg_ESCP_1 = 0x56;
      }
      /* Tray J from iP7200 onwards */
      if ( !(strcmp(init->caps->name,"PIXMA iP7200")) || !(strcmp(init->caps->name,"PIXMA MG5400")) || !(strcmp(init->caps->name,"PIXMA MG6300"))  || !(strcmp(init->caps->name,"PIXMA MG6500")) || !(strcmp(init->caps->name,"PIXMA MG6700")) || !(strcmp(init->caps->name,"PIXMA MG7500")) || !(strcmp(init->caps->name,"PIXMA MG7700")) || !(strcmp(init->caps->name,"PIXMA MX920")) ) {
	arg_ESCP_1 = 0x5b;
	arg_ESCP_9 = 0x00;
      }
      /* Tray L from iP8700 onwards */
      if ( !(strcmp(init->caps->name,"PIXMA iP8700"))  ) {
	arg_ESCP_1 = 0x62;
      }
      /* Tray M from TS8000 onwards */
      if ( !(strcmp(init->caps->name,"PIXMA TS8000"))  ) {
	arg_ESCP_1 = 0xbc;
      }

    }
  }
      /*  850i:  CD Tray custom: none --- no ESC (P */
      /*  865i:  CD Tray custom: 0x35               */
      /* MP500:  CD Tray D     : 0x4b               */
      /* MP530:  CD Tray D     : 0x4b               */
      /* MP600:  CD Tray F     : 0x51               */
      /* MP610:  CD Tray F     : 0x51               */
      /* MP630:  CD Tray G     : 0x53               */
      /* MP640:  CD Tray G     : 0x53               */
      /* MP700:  CD tray custom: none --- no ESC (P */
      /* MP710:  CD tray custom: 0x35               */
      /* MP730:  CD tray custom: none --- no ESC (P */
      /* MP740:  CD tray custom: 0x35               */
      /* MP750:  CD Tray B     : 0x40               */
      /* MP760:  CD Tray B     : 0x40               */
      /* MP770:  CD Tray B     : 0x40               */
      /* MP780:  CD Tray B     : 0x40               */
      /* MP790:  CD Tray B     : 0x40               */
      /* MP800:  CD Tray D     : 0x4b               */
      /* MP810:  CD Tray F     : 0x51               */
      /* MP830:  CD Tray D     : 0x4b               */
      /* MP900:  CD Tray custom: 0x35               */
      /* MP950:  CD Tray C     : 0x4a               */
      /* MP960:  CD Tray F     : 0x51               */
      /* MP970:  CD Tray F     : 0x51               */
      /* MP980:  CD Tray G     : 0x53               */
      /* MP990:  CD Tray G     : 0x53               */
      /* MX850:  CD Tray F     : 0x51               */
      /* MX920:  CD Tray J     : 0x5b               */
      /* iP3000: CD Tray B     : 0x40               */
      /* iP3100: CD Tray B     : 0x40               */
      /* iP4000: CD Tray B     : 0x40               */
      /* iP4100: CD Tray B     : 0x40               */
      /* iP4200: CD Tray C     : 0x4a               */
      /* iP4300: CD Tray F     : 0x51               */
      /* iP4500: CD Tray F     : 0x51               */
      /* iP4600: CD Tray G     : 0x53               */
      /* iP4700: CD Tray G     : 0x53               */
      /* iP4800: CD Tray G     : 0x56               */
      /* iP4900: CD Tray G     : 0x56               */
      /* iP5000: CD Tray B     : 0x40               */
      /* iP5200: CD Tray C     : 0x4a               */
      /* iP5300: CD Tray F     : 0x51               */
      /* iP6000D:CD Tray B     : 0x40               */
      /* iP6100D:CD Tray B     : 0x40               */
      /* iP6700D:CD Tray C     : 0x4a               */
      /* iP7100: CD Tray B     : 0x40               */
      /* iP7200: CD Tray J     : 0x5b               */
      /* iP7500: CD Tray C     : 0x4a               */
      /* iP8100: CD Tray B     : 0x40               */
      /* iP8500 :CD Tray B     : 0x40               */
      /* iP8600: CD Tray B     : 0x40               */
      /* iP8700: CD Tray L     : 0x62               */
      /* iP9910: CD Tray A     : 0x3f               */
      /* MG5200: CD Tray G     : 0x56               */
      /* MG5300: CD Tray G     : 0x56               */
      /* MG5400: CD Tray J     : 0x5b               */
      /* MG6100: CD Tray G     : 0x56               */
      /* MG6200: CD Tray G     : 0x56               */
      /* MG6300: CD Tray J     : 0x5b               */
      /* MG6500: CD Tray J     : 0x5b               */
      /* MG6700: CD Tray J     : 0x5b               */
      /* MG6900: CD Tray J     : 0x5b               */
      /* MG7500: CD Tray J     : 0x5b               */
      /* MG7700: CD Tray J     : 0x5b               */
      /* MG8100: CD Tray G     : 0x56               */
      /* MG8200: CD Tray G     : 0x56               */
      /* pro9000:CD Tray E     : 0x4c               */
      /* pro9000mk2:CD Tray E  : 0x4c               */
      /* pro9500:CD Tray E     : 0x4c               */
      /* pro9500mk2:CD Tray E  : 0x4c               */
      /* PRO-1:  CD Tray H     : 0x57               */
      /* TS8000: CD Tray M     : 0xbc               */

  /* workaround for FineArt media having same size as non-FineArt media */

      /* Fine Art paper codes */
      /* iP6700D, iP7100, iP7500, iP8100, iP8600, iP9910 */
      /* iX7000 */
      /* MG6100, M6200, MG8100, MG8200 */
      /* MX7600 */
      /* MP950, MP960, MP970, MP980, MP990 */
      /* if (!strcmp(name,"A4"))       return 0x42; */ /* FineArt A4 35mm border --- iP7100: gap is 18 */
      /* if (!strcmp(name,"A3"))       return 0x43; */ /* FineArt A3 35mm border */
      /* if (!strcmp(name,"A3plus"))   return 0x44; */ /* FineArt A3plus 35mm border */
      /* if (!strcmp(name,"Letter"))   return 0x45; */ /* FineArt Letter 35mm border */

      /* Fine Art paper codes */
      /* Pro series (9000, 9000 Mk.2, 9500, 9500 Mk.2, PRO-1) */
      /* if (!strcmp(name,"A4"))       return 0x4d; */ /* FineArt A4 35mm border */
      /* if (!strcmp(name,"A3"))       return 0x4e; */ /* FineArt A3 35mm border */
      /* if (!strcmp(name,"A3plus"))   return 0x50; */ /* FineArt A3plus 35mm border */
      /* if (!strcmp(name,"Letter"))   return 0x4f; */ /* FineArt Letter 35mm border */

      /* Fine Art paper codes */
      /* MG6300, MG6500, MG6700, MG7100, MG7500 */
      /* if (!strcmp(name,"A4"))       return 0x58; */ /* FineArt A4 35mm border */
      /* if (!strcmp(name,"Letter"))   return 0x5a; */ /* FineArt Letter 35mm border */

      /* Fine Art paper codes */
      /* iP8700, iX6800 */
      /* if (!strcmp(name,"A4"))       return 0x58; */ /* FineArt A4 35mm border */
      /* if (!strcmp(name,"A3"))       return 0x59; */ /* FineArt A3 35mm border */
      /* if (!strcmp(name,"A3plus"))   return 0x5d; */ /* FineArt A3plus 35mm border */
      /* if (!strcmp(name,"Letter"))   return 0x5a; */ /* FineArt Letter 35mm border */


  /* iP7100 is an exception needing yet another papersize code */
  if ( (arg_ESCP_2 == 0x28) || ( arg_ESCP_2 == 0x29) || (arg_ESCP_2 ==  0x2c) || (arg_ESCP_2 == 0x31) ) {
    /* A4 FineArt */
    if ( arg_ESCP_1 == 0x03 ) {
      if ( !(strcmp(init->caps->name,"PIXMA MP950")) || !(strcmp(init->caps->name,"PIXMA MP960")) || !(strcmp(init->caps->name,"PIXMA MP970")) || !(strcmp(init->caps->name,"PIXMA MP980")) || !(strcmp(init->caps->name,"PIXMA MP990")) || !(strcmp(init->caps->name,"PIXMA MX7600")) || !(strcmp(init->caps->name,"PIXMA iP6700")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP7500")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8600")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA iX7000")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG8200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) ) {
	arg_ESCP_1 = 0x42;
      }
      else if ( !(strcmp(init->caps->name,"PIXMA Pro9000")) || !(strcmp(init->caps->name,"PIXMA Pro9002")) || !(strcmp(init->caps->name,"PIXMA Pro9500")) || !(strcmp(init->caps->name,"PIXMA Pro9502")) ) {
	arg_ESCP_1 = 0x4d;
      }
      else if ( !(strcmp(init->caps->name,"PIXMA MG6300")) || !(strcmp(init->caps->name,"PIXMA MG6500")) || !(strcmp(init->caps->name,"PIXMA MG6700")) || !(strcmp(init->caps->name,"PIXMA MG7500")) || !(strcmp(init->caps->name,"PIXMA iP8700")) || !(strcmp(init->caps->name,"PIXMA iX6800")) || !(strcmp(init->caps->name,"PIXMA TS8000")) ) {
	arg_ESCP_1 = 0x58;
      }
      else {
	/* default back to non-FineArt */
	arg_ESCP_1 = 0x03;
      }
    }
    /* A3 FineArt */
    if ( arg_ESCP_1 == 0x05 ) {
      if ( !(strcmp(init->caps->name,"PIXMA MP950")) || !(strcmp(init->caps->name,"PIXMA MP960")) || !(strcmp(init->caps->name,"PIXMA MP970")) || !(strcmp(init->caps->name,"PIXMA MP980")) || !(strcmp(init->caps->name,"PIXMA MP990")) || !(strcmp(init->caps->name,"PIXMA MX7600")) || !(strcmp(init->caps->name,"PIXMA iP6700")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP7500")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8600")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA iX7000")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG8200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) ) {
	arg_ESCP_1 = 0x43;
      }
      else if ( !(strcmp(init->caps->name,"PIXMA Pro9000")) || !(strcmp(init->caps->name,"PIXMA Pro9002")) || !(strcmp(init->caps->name,"PIXMA Pro9500")) || !(strcmp(init->caps->name,"PIXMA Pro9502")) ) {
	arg_ESCP_1 = 0x4e;
      }
#if 0
      /* no known papersize code yet, since these printers do not handle A3 */
      else if ( !(strcmp(init->caps->name,"PIXMA MG6300")) || !(strcmp(init->caps->name,"PIXMA MG6500")) || !(strcmp(init->caps->name,"PIXMA MG6700")) || !(strcmp(init->caps->name,"PIXMA MG7500")) || !(strcmp(init->caps->name,"PIXMA iP8700")) || !(strcmp(init->caps->name,"PIXMA iX6800")) ) {
        arg_ESCP_1 = 0x59;
	}
#endif
      else {
	/* default back to non-FineArt */
	arg_ESCP_1 = 0x05;
      }
    }
    /* Letter FineArt */
    if ( arg_ESCP_1 == 0x0d ) {
      if ( !(strcmp(init->caps->name,"PIXMA MP950")) || !(strcmp(init->caps->name,"PIXMA MP960")) || !(strcmp(init->caps->name,"PIXMA MP970")) || !(strcmp(init->caps->name,"PIXMA MP980")) || !(strcmp(init->caps->name,"PIXMA MP990")) || !(strcmp(init->caps->name,"PIXMA MX7600")) || !(strcmp(init->caps->name,"PIXMA iP6700")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP7500")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8600")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA iX7000")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG8200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) ) {
	arg_ESCP_1 = 0x45;
      }
      else if ( !(strcmp(init->caps->name,"PIXMA Pro9000")) || !(strcmp(init->caps->name,"PIXMA Pro9002")) || !(strcmp(init->caps->name,"PIXMA Pro9500")) || !(strcmp(init->caps->name,"PIXMA Pro9502")) ) {
	arg_ESCP_1 = 0x4f;
      }
      else if ( !(strcmp(init->caps->name,"PIXMA MG6300")) || !(strcmp(init->caps->name,"PIXMA MG6500")) || !(strcmp(init->caps->name,"PIXMA MG6700")) || !(strcmp(init->caps->name,"PIXMA MG7500")) || !(strcmp(init->caps->name,"PIXMA iP8700")) || !(strcmp(init->caps->name,"PIXMA iX6800")) || !(strcmp(init->caps->name,"PIXMA TS8000")) ) {
	arg_ESCP_1 = 0x5a;
      }
      else {
	/* default back to non-FineArt */
	arg_ESCP_1 = 0x0d;
      }
    }
    /* A3plus FineArt */
    if ( arg_ESCP_1 == 0x2c ) {
      if ( !(strcmp(init->caps->name,"PIXMA MP950")) || !(strcmp(init->caps->name,"PIXMA MP960")) || !(strcmp(init->caps->name,"PIXMA MP970")) || !(strcmp(init->caps->name,"PIXMA MP980")) || !(strcmp(init->caps->name,"PIXMA MP990")) || !(strcmp(init->caps->name,"PIXMA MX7600")) || !(strcmp(init->caps->name,"PIXMA iP6700")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP7500")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8600")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA iX7000")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG8200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) ) {
	arg_ESCP_1 = 0x44;
      }
      else if ( !(strcmp(init->caps->name,"PIXMA Pro9000")) || !(strcmp(init->caps->name,"PIXMA Pro9002")) || !(strcmp(init->caps->name,"PIXMA Pro9500")) || !(strcmp(init->caps->name,"PIXMA Pro9502")) ) {
	arg_ESCP_1 = 0x50;
      }
#if 0
      else if ( !(strcmp(init->caps->name,"PIXMA iP8700")) || !(strcmp(init->caps->name,"PIXMA iX6800")) ) {
        arg_ESCP_1 = 0x5d;
	}
#endif
      else {
	/* default back to non-FineArt */
	arg_ESCP_1 = 0x2c;
      }
    }
  }

  /* workaround for media type based differences in 9-parameter ESC (P commands */
  /* These printers use 0x02 (lower tray) usually and for envelopes, 0x01 (upper tray) with various Hagaki/Photo media, and 0x00 with CD media */
  if ( !(strcmp(init->caps->name,"PIXMA iP7200")) || !(strcmp(init->caps->name,"PIXMA MG5400")) || !(strcmp(init->caps->name,"PIXMA MG6300")) || !(strcmp(init->caps->name,"PIXMA MG6500")) || !(strcmp(init->caps->name,"PIXMA MG6700")) || !(strcmp(init->caps->name,"PIXMA MG7500")) || !(strcmp(init->caps->name,"PIXMA MG7700")) || !(strcmp(init->caps->name,"PIXMA MX720")) || !(strcmp(init->caps->name,"PIXMA MX920")) ) {
    if (tray_user_select && !print_cd)
      arg_ESCP_9=user_ESCP_9;
    else {
      switch(arg_ESCP_2)
	{
	  /* Hagaki media */
	case 0x07: arg_ESCP_9=0x01; break;;
	case 0x14: arg_ESCP_9=0x01; break;; /* not used with any of these models */
	case 0x1b: arg_ESCP_9=0x01; break;;
	case 0x36: arg_ESCP_9=0x01; break;;
	case 0x38: arg_ESCP_9=0x01; break;;
	case 0x46: arg_ESCP_9=0x01; break;;
	case 0x47: arg_ESCP_9=0x01; break;;
	case 0x48: arg_ESCP_9=0x01; break;;
	  /* Photo media here */
	case 0x32: arg_ESCP_9=0x01; break;;
	case 0x33: arg_ESCP_9=0x01; break;;
	case 0x3f: arg_ESCP_9=0x01; break;;
	case 0x2a: arg_ESCP_9=0x01; break;;
	case 0x16: arg_ESCP_9=0x01; break;;
	case 0x44: arg_ESCP_9=0x01; break;; /* MG6700, MG7500 only, instead of 0x16 */
	case 0x1c: arg_ESCP_9=0x01; break;;
	case 0x24: arg_ESCP_9=0x01; break;;
	  /* Envelope media */
	case 0x08: arg_ESCP_9=0x02; break;;
	  /* CD media */
	case 0x1f: arg_ESCP_9=0x00; break;;
	case 0x20: arg_ESCP_9=0x00; break;;
	  /* other media default to lower tray */
	default:   arg_ESCP_9=0x02; break;;
	}

      /* condition for length to use lower tray: 7.29 in equals 524-525 points */
      if ( (arg_ESCP_9 == 0x01) && ( length > 524 ) ) {
	arg_ESCP_9=0x02;
      }

      /* even if user does not select correct CD media type, set appropriately */
      if (print_cd)
	arg_ESCP_9=0x00;
    }
  }

  /* MG6700, MG7500, TS8000 uses 0xff with CD media tray */
  if (   !(strcmp(init->caps->name,"PIXMA MG6700")) || !(strcmp(init->caps->name,"PIXMA MG7500"))  || !(strcmp(init->caps->name,"PIXMA TS8000")) ) {
    if (tray_user_select && !print_cd)
      arg_ESCP_9=user_ESCP_9;
    else {
      switch(arg_ESCP_2)
	{
	  /* CD media */
	case 0x1f: arg_ESCP_9=0xff; break;;
	case 0x20: arg_ESCP_9=0xff; break;;
	}

      /* even if user does not select correct CD media type, set appropriately */
      if (print_cd)
	arg_ESCP_9=0x00;
    }
  }

  if ( init->caps->ESC_P_len == 9 ) /* support for new devices from October 2012. */
    {/* the 4th of the 6 bytes is the media type. 2nd byte is media size. Both read from canon-media array. */

      if ( !(strcmp(init->caps->name,"PIXMA MG7700")) ) {
	/* output with 3 extra 0s at the end */
	canon_cmd( v,ESC28,0x50,12,0x00,arg_ESCP_1,0x00,arg_ESCP_2,arg_ESCP_5,arg_ESCP_6,arg_ESCP_7,0x00,arg_ESCP_9,0x00,0x00,0x00 );
      }
      else {

      /* arg_ESCP_1 = 0x03; */ /* A4 size */
      /* arg_ESCP_2 = 0x00; */ /* plain media */
      /*                             size                media                                  tray */
      canon_cmd( v,ESC28,0x50,9,0x00,arg_ESCP_1,0x00,arg_ESCP_2,arg_ESCP_5,arg_ESCP_6,arg_ESCP_7,0x00,arg_ESCP_9 );

      }
    }
  else if ( init->caps->ESC_P_len == 8 ) /* support for new devices from 2012. */
    {/* the 4th of the 6 bytes is the media type. 2nd byte is media size. Both read from canon-media array. */

      /* arg_ESCP_1 = 0x03; */ /* A4 size */
      /* arg_ESCP_2 = 0x00; */ /* plain media */
      /*                             size                media             */
      canon_cmd( v,ESC28,0x50,8,0x00,arg_ESCP_1,0x00,arg_ESCP_2,arg_ESCP_5,arg_ESCP_6,arg_ESCP_7,0x00 );
    }
  else if ( init->caps->ESC_P_len == 6 ) /* first devices with XML header and ender */
    {/* the 4th of the 6 bytes is the media type. 2nd byte is media size. Both read from canon-media array. */

      /* arg_ESCP_1 = 0x03; */ /* A4 size */
      /* arg_ESCP_2 = 0x00; */ /* plain media */
      /*                             size                media             */
      canon_cmd( v,ESC28,0x50,6,0x00,arg_ESCP_1,0x00,arg_ESCP_2,arg_ESCP_5,arg_ESCP_6 );
    }
  else if ( init->caps->ESC_P_len == 4 )  {/* 4 bytes */
    /*                             size            media       */
    canon_cmd( v,ESC28,0x50,4,0x00,arg_ESCP_1,0x00,arg_ESCP_2 );
  }
  else if ( init->caps->ESC_P_len == 2 )  {
    /* 2 bytes only */
      canon_cmd( v,ESC28,0x50,2,0x00,arg_ESCP_1 );
    }
  else /* error in definition */
    stp_dprintf(STP_DBG_CANON, v,"SEVERE BUG IN print-canon.c::canon_init_setESC_P() "
		 "ESC_P_len=%d!!\n",init->caps->ESC_P_len);
}

/* ESC (s -- 0x73 -- :
   used in some newer printers for duplex pages except last one.
   When capability available, used for non-tumble and tumble (unlike Esc (u which is non-tumble only)
   Limitation: outputs on every page */
static void
canon_init_setESC_s(const stp_vars_t *v, const canon_privdata_t *init)
{
  if (!(init->caps->features & CANON_CAP_DUPLEX))
    return;
  if (!(init->caps->features & CANON_CAP_s))
    return;

  canon_cmd(v,ESC28,0x73, 1, 0x00);
}

/* ESC (S -- 0x53 -- unknown -- :
   Required by iP90/iP90v and iP100 printers.
 */
static void
canon_init_setESC_S(const stp_vars_t *v, const canon_privdata_t *init)
{
  unsigned char arg_ESCS_01,arg_ESCS_04,arg_ESCS_09,arg_ESCS_11;

  if (!(init->caps->features & CANON_CAP_S))
    return;

  if (!(init->mode->flags & MODE_FLAG_S))
    return;

  /* iP90 defaults: based on non-photo media */
  arg_ESCS_01 = 0x01;
  arg_ESCS_04 = 0xff;
  arg_ESCS_09 = 0x1a;
  arg_ESCS_11 = 0x68;

  /* hard-coded for different media, color and  quality settings */
  /* iP90 bytes 1,4,9 and 11 vary */
  if ( !(strcmp(init->caps->name,"PIXMA iP90")) ) {
    if ( !strcmp(init->mode->name,"600x600dpi_high2") || !strcmp(init->mode->name,"600x600dpi_high4") ) {
      /* if inkset is color then set special bytes, else leave default.
	 Note: in this case the mode only has CMYK, mono is a different mode.
      */
      if (init->used_inks == CANON_INK_CMYK) {
	arg_ESCS_01 = 0xc1;
	arg_ESCS_04 = 0xf1;
	arg_ESCS_09 = 0x50;
	arg_ESCS_11 = 0x28;
      }
    } else if ( !strcmp(init->mode->name,"300x300dpi_draft") ) {
      /* set regardless of inkset */
      arg_ESCS_09 = 0x02;
      arg_ESCS_11 = 0x08;
    } else if ( !strcmp(init->mode->name,"600x600dpi_draft2") ) {
      /* if inkset is color then set special bytes, else leave default */
      if (init->used_inks == CANON_INK_CMYK) {
	arg_ESCS_09 = 0x0a;
	arg_ESCS_11 = 0x28;
      }
    } else if ( !strcmp(init->mode->name,"600x600dpi_photohigh") || !strcmp(init->mode->name,"600x600dpi_photo") || !strcmp(init->mode->name,"600x600dpi_photodraft") ) {
      /* almost all photo media need (same) changes from defaults */
      /* exception: "600x600dpi_photohigh2" no ESC (S command */
      /* exception: "600x600dpi_tshirt" no ESC (S command */
      arg_ESCS_01 = 0xc1;
      arg_ESCS_04 = 0xf0;
      arg_ESCS_09 = 0x50;
      arg_ESCS_11 = 0x28;
    }
  }
  else if ( !(strcmp(init->caps->name,"PIXMA iP100")) ) {
    /* iP100 bytes 9 and 11 vary */
    if ( !strcmp(init->mode->name,"300x300dpi_draft") ) {
      /* set regardless of inkset */
      arg_ESCS_09 = 0x02;
      arg_ESCS_11 = 0x08;
    } else if  ( !strcmp(init->mode->name,"600x600dpi_photohigh2") || !strcmp(init->mode->name,"600x600dpi_photohigh") || !strcmp(init->mode->name,"600x600dpi_photo2") || !strcmp(init->mode->name,"600x600dpi_photo") || !strcmp(init->mode->name,"600x600dpi_tshirt") ) {
      /* all photo media need (same) changes from defaults */
      arg_ESCS_09 = 0x0a;
      arg_ESCS_11 = 0x28;
    }
  }

      canon_cmd(v,ESC28,0x53,54,arg_ESCS_01,0x02,0xff,arg_ESCS_04,0x41,0x02,0x00,0x01,arg_ESCS_09,0x00,arg_ESCS_11,0x00,0x01,0x01,0x03,0x02,0x01,0x01,0x01,0x03,0x02,0x00,0x07,0x06,0x02,0x01,0x02,0x04,0x04,0x04,0x05,0x06,0x08,0x08,0x08,0x0a,0x0a,0x09,0x00,0x03,0x02,0x01,0x01,0x01,0x01,0x01,0x06,0x02,0x02,0x02,0x03,0x04,0x05,0x06);

}

/* ESC (T -- 0x54 -- setCartridge -- :
 */
static void
canon_init_setCartridge(const stp_vars_t *v, const canon_privdata_t *init)
{
  const char *ink_set;

  if (!(init->caps->features & CANON_CAP_T))
    return;

  ink_set = stp_get_string_parameter(v, "InkSet");

  if (ink_set && !(strcmp(ink_set,"Both"))) {
    if ( !(strcmp(init->caps->name,"PIXMA iP90")) || !(strcmp(init->caps->name,"PIXMA iP100")) || !(strcmp(init->caps->name,"PIXMA iP110")) ) {
      canon_cmd(v,ESC28,0x54,3,0x02,0x00,0x00); /* default for iP90, iP100, iP110 */
    }
    else if ( !(strcmp(init->caps->name,"PIXMA iP6210")) ) {
      canon_cmd(v,ESC28,0x54,3,0x03,0x06,0x06); /* default for iP6210D, iP6220D, iP6310D */
      /* both:  0x3 0x6 0x6 */
      /* color: 0x3 0x1 0x1 */
    }
    else {
      canon_cmd(v,ESC28,0x54,3,0x03,0x04,0x04); /* default: both cartridges */
    }
  }
  else if (ink_set && !(strcmp(ink_set,"Black"))) {
    if ( !(strcmp(init->caps->name,"PIXMA iP90")) || !(strcmp(init->caps->name,"PIXMA iP100")) || !(strcmp(init->caps->name,"PIXMA iP110")) ) {
      canon_cmd(v,ESC28,0x54,3,0x02,0x00,0x00); /* default for iP90, iP100, iP110 */
    }
    else if ( !(strcmp(init->caps->name,"PIXMA iP6210")) ) {
	canon_cmd(v,ESC28,0x54,3,0x03,0x06,0x06); /* default for iP6210D, iP6220D, iP6310D */
	/* both:  0x3 0x6 0x6 */
	/* color: 0x3 0x1 0x1 */
	/* workaround since does not have black option */
    }
    else {
      canon_cmd(v,ESC28,0x54,3,0x03,0x02,0x02); /* default: black cartridge */
    }
  }
  else if (ink_set && !(strcmp(ink_set,"Color"))) {
    if ( !(strcmp(init->caps->name,"PIXMA iP90")) || !(strcmp(init->caps->name,"PIXMA iP100")) || !(strcmp(init->caps->name,"PIXMA iP110")) ) {
      canon_cmd(v,ESC28,0x54,3,0x02,0x00,0x01); /* composite for iP90, iP100, iP110 */
      /* black save     : 2 1 0 for selected plain (600dpi std) modes, rest remain 2 0 0 */
      /* composite black: 2 0 1 for selected plain (600dpi std & draft) modes, rest remain 2 0 0 */
      /* both above set : AND of bytes above */
    }
    else if ( !(strcmp(init->caps->name,"PIXMA iP6210")) ) {
      canon_cmd(v,ESC28,0x54,3,0x03,0x01,0x01); /* default for iP6210D, iP6220D, iP6310D */
      /* both:  0x3 0x6 0x6 */
      /* color: 0x3 0x1 0x1 */
    }
    else {
      canon_cmd(v,ESC28,0x54,3,0x03,0x01,0x01); /* default: color cartridges */
    }
  }
  else {
    canon_cmd(v,ESC28,0x54,3,0x03,0x04,0x04); /* default: both cartridges */
  }
}

/* ESC (q -- 0x71 -- setPageID -- :
 */
static void
canon_init_setPageID(const stp_vars_t *v, const canon_privdata_t *init)
{
  if (!(init->caps->features & CANON_CAP_q))
    return;

  canon_cmd(v,ESC28,0x71, 1, 0x01);
}

/* ESC (r -- 0x72 --  -- :
 */
static void
canon_init_setX72(const stp_vars_t *v, const canon_privdata_t *init)
{
  if ( !( (init->caps->features & CANON_CAP_r)
         || (init->caps->features & CANON_CAP_rr) ) )
    return;

  if ( (init->caps->features & CANON_CAP_r)
       || (init->caps->features & CANON_CAP_rr) )
    if  (init->caps->ESC_r_arg != 0) /* only output arg if non-zero */
      canon_cmd(v,ESC28,0x72, 1, init->caps->ESC_r_arg); /* whatever for - 8200/S200 need it */
  if (init->caps->features & CANON_CAP_rr) {
    if ( !(strcmp(init->caps->name,"S200")) ) {
      canon_cmd(v,ESC28,0x72, 3, 0x63, 1, 0); /* whatever for - S200 needs it */
      /* probably to set the print direction of the head */
    }
    else if ( !(strcmp(init->caps->name,"S820")) || !(strcmp(init->caps->name,"S900")) || !(strcmp(init->caps->name,"S9000")) || !(strcmp(init->caps->name,"i950")) || !(strcmp(init->caps->name,"i960")) || !(strcmp(init->caps->name,"i9100")) || !(strcmp(init->caps->name,"i9900")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8500")) || !(strcmp(init->caps->name,"PIXMA iP8600")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA MP900")) || !(strcmp(init->caps->name,"PIXMA Pro9000")) || !(strcmp(init->caps->name,"PIXMA Pro9002")) || !(strcmp(init->caps->name,"PIXMA Pro9500")) || !(strcmp(init->caps->name,"PIXMA Pro9502")) ) {
      canon_cmd(v,ESC28,0x72, 2, 0x62, 0); /* 2 bytes */
    }
    /* CD mode only */
    else if ( (init->mode->flags & MODE_FLAG_CD) && (!(strcmp(init->caps->name,"PIXMA iP4600")) || !(strcmp(init->caps->name,"PIXMA iP4700")) || !(strcmp(init->caps->name,"PIXMA MP980")) || !(strcmp(init->caps->name,"PIXMA MP990")) ) ) {
      canon_cmd(v,ESC28,0x72, 1, 0x65);
    }
    /* CD mode only */
    else if ( (init->mode->flags & MODE_FLAG_CD) && ( !(strcmp(init->caps->name,"PIXMA iP4800")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG8100")) ) ) {
      canon_cmd(v,ESC28,0x72, 1, 0x68);
    }
    /* CD mode only -- no ESC (r at all otherwise */
    else if ( (init->mode->flags & MODE_FLAG_CD) && ( !(strcmp(init->caps->name,"PIXMA iP4900")) || !(strcmp(init->caps->name,"PIXMA MG5200")) || !(strcmp(init->caps->name,"PIXMA MG5300")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG8200")) || !(strcmp(init->caps->name,"PIXMA TS8000")) ) ) {
      canon_cmd(v,ESC28,0x72, 1, 0x68); /* same as above case? */
    }
    /* other cases here */
  }
}

/* ESC (r -- 0x72 -- ??? set direction ??? -- :
   only works if quality = 01  (S200) */
static void
canon_set_X72(const stp_vars_t *v, int x72arg)
{
  canon_cmd(v,ESC28,0x72, 3, 0x63, x72arg, 0);
}

/* ESC (t -- 0x74 -- cmdSetImage --:
 */
static void
canon_init_setImage(const stp_vars_t *v, const canon_privdata_t *init)
{
  unsigned char
    arg_74_1 = 0x01, /* 1 bit per pixel */
    arg_74_2 = 0x00, /*  */
    arg_74_3 = 0x01; /* 01 <= 360 dpi    09 >= 720 dpi */

  if (!(init->caps->features & CANON_CAP_t))
    return;

  if(init->mode->flags & MODE_FLAG_EXTENDED_T)  /*code requires extended mode settings*/
  {
    int i;
    int length = init->mode->num_inks*3 + 3;
    unsigned char* buf = stp_zalloc(length);
    buf[0]=0x80;
    if(init->mode->flags & MODE_FLAG_PRO){
        buf[1]=0x90; /* was 0x10, but this should probably be 0x90 */
    	buf[2]=0x4;
    }else if(init->mode->flags & MODE_FLAG_IP8500){
    	buf[1]=0x00;
    	buf[2]=0x01;
    }else if(init->mode->flags & MODE_FLAG_MP130){
    	buf[1]=0x04;
    	buf[2]=0x01;
    }else if(init->mode->flags & MODE_FLAG_MP360){
    	buf[1]=0x84;
    	buf[2]=0x01;
    }else{
    	buf[1]=0x80;
    	buf[2]=0x01;
    }
    for(i=0;i<init->mode->num_inks;i++){
        if(init->mode->inks[i].channel != 0){ /* 0 means ink is used in gutenprint for sub-channel setup but not intended for printer */
          if(init->mode->inks[i].ink->flags & INK_FLAG_5pixel_in_1byte)
            buf[3+i*3+0]=(1<<5)|init->mode->inks[i].ink->bits; /*info*/
           /*else if(init->mode->inks[i].ink->flags & INK_FLAG_lowresmode)
             {
               buf[3+i*3+1]=0x01;
               buf[3+i*3+0]=init->mode->inks[i].ink->bits;
             }*/
	  else if(init->mode->inks[i].ink->flags & INK_FLAG_3pixel5level_in_1byte)
            buf[3+i*3+0]=(1<<5)|init->mode->inks[i].ink->bits; /*info*/
	  else if(init->mode->inks[i].ink->flags & INK_FLAG_3pixel6level_in_1byte)
            buf[3+i*3+0]=(1<<5)|init->mode->inks[i].ink->bits; /*info*/
          else
            buf[3+i*3+0]=init->mode->inks[i].ink->bits;

          /* workaround for now on the 4-4 inkset and others */
          /*if (init->mode->inks[i].ink->bits == 4)
            buf[3+i*3+2] = 0x04;*/
          /*else if (init->mode->inks[i].ink->bits == 2)
            buf[3+i*3+2] = 0x04;*/
          /*else if (init->mode->inks[i].ink->bits == 1)
            buf[3+i*3+2] = 0x02;*/
	  /*else*/ /* normal operation */
	    buf[3+i*3+2]= init->mode->inks[i].ink->numsizes+1;/*level*/
          /*else
            buf[3+i*3+2] = 0x00;*/
          /* this should show that there is an error */
       }
    }
    stp_zfwrite(ESC28,2,1,v);
    stp_putc(0x74,v);
    stp_put16_le(length,v);
    stp_zfwrite((char*)buf,length,1,v);
    stp_free(buf);
    return;
  }

  /* other models mostly hardcoded stuff not really understood ;( */
  if (!strcmp(init->caps->name,"S200")) /* 1 bit per pixel (arg 4,7,10,13); */
                               /* 2 level per pixel (arg 6,9,12,15) for each color */
                               /* though we print only 1bit/pixel - but this is how */
                               /* the windows driver works */
  {
    canon_cmd(v,ESC28,0x74, 30, 0x80, 4, 1, 1, 0, 2, 1, 0, 2, 1, 0, 2, 1, 0, 2,\
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    return;
  }

  if (init->mode->xdpi==1440) arg_74_2= 0x04;
  if (init->mode->ydpi>=720)  arg_74_3= 0x09;

  if (init->mode->inks[0].ink->bits>1) {
    arg_74_1= 0x02;
    arg_74_2= 0x80;
    arg_74_3= 0x09; /* default for most media */
    /* FIXME: (Gernot) below must be corrected I think, since CMY is
       not a function of the cartridge but of the driver selection of
       which logical inks get sent to the printer. So here the
       printers that use this should be enumerated, rather than a
       generic condition based on CANON_INK_CMY */
    if (init->used_inks == CANON_INK_CMY) arg_74_3= 0x02; /* for BC-06 cartridge!!! */
    /* example of better way: for BJC-3000 series */
    if  ( !strcmp(init->caps->name,"3000") || !strcmp(init->caps->name,"4300")) {
      /* but if photo cartridge selection, set differently again */
      if (init->mode->flags & MODE_FLAG_PHOTO)
	arg_74_3= 0x0a;
      /* T-Shirt (3), Backprint Film (3) or Transparencies (2) */
      else if ((init->pt->media_code_c==2) || (init->pt->media_code_c==3))
	arg_74_3= 0x01;
      else
	/* other media */
	arg_74_3= 0x09; /* return to default after broken code above */
    }
    if  ( (!strcmp(init->caps->name,"2000")) || (!strcmp(init->caps->name,"2100")) ) {
      /* but if photo cartridge selection, set differently again */
      if (init->mode->flags & MODE_FLAG_PHOTO)
	arg_74_3= 0x0a;
      else
	/* other media */
	arg_74_3= 0x09; /* return to default after broken code above */
    }

  }

  /* workaround for the bjc8200 in 6color mode - not really understood */
  if (!strcmp(init->caps->name,"8200")) {
    if (init->used_inks == CANON_INK_CcMmYK) {
      arg_74_1= 0xff;
      arg_74_2= 0x90;
      arg_74_3= 0x04;
      if (init->mode->ydpi>600)  arg_74_3= 0x09;
    } else {
      arg_74_1= 0x01;
      arg_74_2= 0x00;
      arg_74_3= 0x01;
      if (init->mode->ydpi>600)  arg_74_3= 0x09;
    }
  }

  canon_cmd(v,ESC28,0x74, 3, arg_74_1, arg_74_2, arg_74_3);
}

/* ESC (I (J (L
 */
static void
canon_init_setMultiRaster(const stp_vars_t *v, const canon_privdata_t *init){

  int i; /* introduced for channel counting */
  char* raster_channel_order; /* introduced for channel counting */

  if(!(init->caps->features & CANON_CAP_I))
	return;

  canon_cmd(v,ESC28,0x49, 1, 0x1);  /* enable MultiLine Raster? */
  /* canon_cmd(v,ESC28,0x4a, 1, init->caps->raster_lines_per_block); */
  canon_cmd(v,ESC28,0x4a, 1, init->mode->raster_lines_per_block);    /* set number of lines per raster block */

  /* set the color sequence */
  stp_zfwrite("\033(L", 3, 1, v);
  stp_put16_le(init->num_channels, v);
  /* add an exception here to add 0x60 of cmy channels for those printers/modes that require it */
  /* Note: 2017-06-28 this section needs revision, many printers not entered here, thus modes either not supported or incorrectly output */
  raster_channel_order=init->channel_order;
  if (  !(strcmp(init->caps->name,"PIXMA E3100")) || !(strcmp(init->caps->name,"PIXMA MP140")) || !(strcmp(init->caps->name,"PIXMA MP150")) || !(strcmp(init->caps->name,"PIXMA MP160")) || !(strcmp(init->caps->name,"PIXMA MP170")) || !(strcmp(init->caps->name,"PIXMA MP180")) || !(strcmp(init->caps->name,"PIXMA MP190")) || !(strcmp(init->caps->name,"PIXMA MP210")) || !(strcmp(init->caps->name,"PIXMA MP220")) || !(strcmp(init->caps->name,"PIXMA MP240")) || !(strcmp(init->caps->name,"PIXMA MP250")) || !(strcmp(init->caps->name,"PIXMA MP270")) || !(strcmp(init->caps->name,"PIXMA MP280")) || !(strcmp(init->caps->name,"PIXMA MP450")) || !(strcmp(init->caps->name,"PIXMA MP460")) || !(strcmp(init->caps->name,"PIXMA MP470")) || !(strcmp(init->caps->name,"PIXMA MP480")) || !(strcmp(init->caps->name,"PIXMA MP490")) || !(strcmp(init->caps->name,"PIXMA MP495")) || !(strcmp(init->caps->name,"PIXMA MX300")) || !(strcmp(init->caps->name,"PIXMA MX310")) || !(strcmp(init->caps->name,"PIXMA MX330")) || !(strcmp(init->caps->name,"PIXMA MX340")) || !(strcmp(init->caps->name,"PIXMA MX350")) || !(strcmp(init->caps->name,"PIXMA MX360"))  || !(strcmp(init->caps->name,"PIXMA MX370")) || !(strcmp(init->caps->name,"PIXMA MX410")) || !(strcmp(init->caps->name,"PIXMA MX510")) || !(strcmp(init->caps->name,"PIXMA MX520")) || !(strcmp(init->caps->name,"PIXMA iP2700")) || !(strcmp(init->caps->name,"PIXMA MG2100")) || !(strcmp(init->caps->name,"PIXMA MG2400")) || !(strcmp(init->caps->name,"PIXMA MG2500")) || !(strcmp(init->caps->name,"PIXMA MG3500")) || !(strcmp(init->caps->name,"PIXMA MG3600")) || !(strcmp(init->caps->name,"PIXMA G1000")) || !(strcmp(init->caps->name,"PIXMA G4000")) )
    {
      /* if cmy there, add 0x60 to each --- all modes using cmy require it */
      for(i=0;i<init->num_channels;i++){
	switch(init->channel_order[i]){
	case 'c':raster_channel_order[i]+=0x60; break;;
	case 'm':raster_channel_order[i]+=0x60; break;;
	case 'y':raster_channel_order[i]+=0x60; break;;
	}
      }
      /* Gernot: debug */
      /* if CMY there, add 0x80 to each to change to cmy+0x60 */
      /*     for(i=0;i<init->num_channels;i++){
	switch(init->channel_order[i]){
	case 'C':raster_channel_order[i]+=0x80; break;;
	case 'M':raster_channel_order[i]+=0x80; break;;
	case 'Y':raster_channel_order[i]+=0x80; break;;
	}
	}*/
      stp_zfwrite((const char *)raster_channel_order,init->num_channels, 1, v);
    }
  /* note these names are from canon-printers.h, only separate driver strings are required */
  else if ( !(strcmp(init->caps->name,"PIXMA iP6210")) ) {
    /* if cmy there, add 0x60 to each --- only some modes using cmy require it */
    /* case one: all modes with only cmy */
    if (init->num_channels==3) {
      for(i=0;i<init->num_channels;i++){
	switch(init->channel_order[i]){
	case 'c':raster_channel_order[i]+=0x60; break;;
	case 'm':raster_channel_order[i]+=0x60; break;;
	case 'y':raster_channel_order[i]+=0x60; break;;
	}
      }
    }
    /* case two: CMYcmy modes, but not CMYkcm modes */
    else if ( (init->num_channels==6) && (init->used_inks==CANON_INK_CMY) ) {
      for(i=0;i<init->num_channels;i++){
	switch(init->channel_order[i]){
	case 'c':raster_channel_order[i]+=0x60; break;;
	case 'm':raster_channel_order[i]+=0x60; break;;
	case 'y':raster_channel_order[i]+=0x60; break;;
	}
      }
    }
    /* case three: CMYkm modes with 0x80 to subtract from all inks with 2 or 8 bits */
    else if ( (init->num_channels==6) && (init->used_inks==CANON_INK_CcMmYK) && ((init->mode->inks[0].ink->bits==2) || (init->mode->inks[0].ink->bits==8)) ) {
      for(i=0;i<init->num_channels;i++){
	switch(init->channel_order[i]){
	case 'C':raster_channel_order[i]+=0x80; break;;
	case 'M':raster_channel_order[i]+=0x80; break;;
	case 'Y':raster_channel_order[i]+=0x80; break;;
	case 'c':raster_channel_order[i]+=0x80; break;;
	case 'm':raster_channel_order[i]+=0x80; break;;
	case 'k':raster_channel_order[i]+=0x80; break;;
	}
      }
    }
    stp_zfwrite((const char *)raster_channel_order,init->num_channels, 1, v);
  }
  else
    {
      stp_zfwrite((const char *)init->channel_order,init->num_channels, 1, v);
    }
}

/* ESC (u -- 0x75 -- even pages duplex for long side only -- */
static void
canon_init_setESC_u(const stp_vars_t *v, const canon_privdata_t *init)
{
  if (!(init->caps->features & CANON_CAP_DUPLEX))
    return;

  canon_cmd( v,ESC28,0x75, 1, 0x01 );
}

/* ESC (v -- 0x76 -- */
/* page rotation in Windows driver settings: */
/*      none: 0x0 */
/*    90 deg: 0x1 */
/*   180 deg: 0x2 */
/*   270 deg: 0x3 */
/* also influences ESC (P command parameters 5 and 6 */
/*      none: 1 0 */
/*    90 deg: 2 0 */
/*   180 deg: 1 1 */
/*   270 deg: 2 1 */
static void
canon_init_setESC_v(const stp_vars_t *v, const canon_privdata_t *init)
{
  const char  *orientation_type =stp_get_string_parameter(v, "Orientation");
  unsigned char arg_ESCv_1 = 0x00;

  if (!(init->caps->features & CANON_CAP_v))
    return;

  if( orientation_type && !strcmp(orientation_type,"Portrait")) /* none */
    arg_ESCv_1 = 0x00;
  else if( orientation_type && !strcmp(orientation_type,"Landscape")) /* 90 deg */
    arg_ESCv_1 = 0x01;
  else if( orientation_type && !strcmp(orientation_type,"UpsideDown")) /* 180 deg */
    arg_ESCv_1 = 0x02;
  else if( orientation_type && !strcmp(orientation_type,"Seascape")) /* 270 deg */
    arg_ESCv_1 = 0x03;

  canon_cmd( v,ESC28,0x76, 1, arg_ESCv_1 );
}

/* ESC (w -- 0x77 -- :
   Unknown.
  new September 2015, currently only 1 byte.
*/
static void
canon_init_setESC_w(const stp_vars_t *v, const canon_privdata_t *init)
{
  unsigned char arg_ESCw_1;
  if (!(init->caps->features & CANON_CAP_w))
    return;

  arg_ESCw_1 = (init->pt) ? init->pt->media_code_w: 0x00;
  canon_cmd( v,ESC28,0x77, 1, arg_ESCw_1 );
}

static void
canon_init_printer(const stp_vars_t *v, canon_privdata_t *init)
{
  unsigned int mytop;
  int          page_number = stp_get_int_parameter(v, "PageNumber");
  const char  *duplex_mode = stp_get_string_parameter(v, "Duplex");
  /* init printer */
  if (init->is_first_page) {
    canon_init_resetPrinter(v,init);     /* ESC [K */
    canon_init_setESC_M(v,init);         /* ESC (M */
    canon_init_setDuplex(v,init);        /* ESC ($ */
  }
  canon_init_setPageMode(v,init);        /* ESC (a */
  canon_init_setDataCompression(v,init); /* ESC (b */
  canon_init_setPageID(v,init);          /* ESC (q */
  canon_init_setPrintMode(v,init);       /* ESC (m */
  canon_init_setResolution(v,init);      /* ESC (d */
  canon_init_setImage(v,init);           /* ESC (t */
  canon_init_setColor(v,init);           /* ESC (c */
  canon_init_setPageMargins(v,init);     /* ESC (g */
  canon_init_setPageMargins2(v,init);    /* ESC (p */
  canon_init_setESC_P(v,init);           /* ESC (P */
  canon_init_setCartridge(v,init);       /* ESC (T */
  canon_init_setESC_S(v,init);           /* ESC (S */
  canon_init_setTray(v,init);            /* ESC (l */
  canon_init_setX72(v,init);             /* ESC (r */
  canon_init_setESC_v(v,init);           /* ESC (v */
    if (init->is_first_page)
      canon_init_setESC_w(v,init);       /* ESC (w */
  if((page_number & 1) && duplex_mode && !strcmp(duplex_mode,"DuplexNoTumble"))
    canon_init_setESC_u(v,init);         /* ESC (u 0x1 */
  if(duplex_mode)
    canon_init_setESC_s(v,init);         /* ESC (s 0x0 */
  canon_init_setMultiRaster(v,init);     /* ESC (I (J (L */

  /* some linefeeds */

  mytop= (init->top*init->mode->ydpi)/72;

  if(init->caps->features & CANON_CAP_I)
    /* mytop /= init->caps->raster_lines_per_block; */
    mytop /= init->mode->raster_lines_per_block;

  if(mytop)
    canon_cmd(v,ESC28,0x65, 2, (mytop >> 8 ),(mytop & 255));
}

static void
canon_deinit_printer(const stp_vars_t *v, const canon_privdata_t *init)
{
  /* eject page */
  stp_putc(0x0c,v);

  /* say goodbye */
  canon_cmd(v,ESC28,0x62,1,0);
  if (init->caps->features & CANON_CAP_a)
    canon_cmd(v,ESC28,0x61, 1, 0);
}

static int
canon_start_job(const stp_vars_t *v, stp_image_t *image)
{
  const canon_cap_t * caps = canon_get_model_capabilities(v);
  /* output XML for iP2700 and other devices */
  if (caps->features & CANON_CAP_XML) {
    int length=strlen(prexml_iP2700); /* 680 */
    stp_zfwrite((const char*)prexml_iP2700,length,1,v);
  }
  return 1;
}

static int
canon_end_job(const stp_vars_t *v, stp_image_t *image)
{
  const canon_cap_t * caps = canon_get_model_capabilities(v);
  canon_cmd(v,ESC40,0,0);
  /* output XML for iP2700 and other devices */
  if (caps->features & CANON_CAP_XML) {
    int length=strlen(postxml_iP2700); /* 263 */
    stp_zfwrite((const char*)postxml_iP2700,length,1,v);
  }
  return 1;
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

static void
canon_printfunc(stp_vars_t *v)
{
  int i;
  canon_privdata_t *pd = (canon_privdata_t *) stp_get_component_data(v, "Driver");
  canon_write_line(v);
  for (i = 0; i < pd->num_channels ; i++)
    canon_advance_buffer(pd->channels[i].buf, pd->length, pd->channels[i].delay);

}

static double
get_double_param(stp_vars_t *v, const char *param)
{
  if (param && stp_check_float_parameter(v, param, STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter(v, param);
  else
    return 1.0;
}



static void
set_mask(unsigned char *cd_mask, int x_center, int scaled_x_where,
         int limit, int expansion, int invert)
{
  int clear_val = invert ? 255 : 0;
  int set_val = invert ? 0 : 255;
  int bytesize = 8 / expansion;
  int byteextra = bytesize - 1;
  int first_x_on = x_center - scaled_x_where;
  int first_x_off = x_center + scaled_x_where;
  if (first_x_on < 0)
    first_x_on = 0;
  if (first_x_on > limit)
    first_x_on = limit;
  if (first_x_off < 0)
    first_x_off = 0;
  if (first_x_off > limit)
    first_x_off = limit;
  first_x_on += byteextra;
  if (first_x_off > (first_x_on - byteextra))
    {
      int first_x_on_byte = first_x_on / bytesize;
      int first_x_on_mod = expansion * (byteextra - (first_x_on % bytesize));
      int first_x_on_extra = ((1 << first_x_on_mod) - 1) ^ clear_val;
      int first_x_off_byte = first_x_off / bytesize;
      int first_x_off_mod = expansion * (byteextra - (first_x_off % bytesize));
      int first_x_off_extra = ((1 << 8) - (1 << first_x_off_mod)) ^ clear_val;
      if (first_x_off_byte < first_x_on_byte)
        {
          /* This can happen, if 6 or fewer points are turned on */
          cd_mask[first_x_on_byte] = first_x_on_extra & first_x_off_extra;
        }
      else
        {
          if (first_x_on_extra != clear_val)

            cd_mask[first_x_on_byte - 1] = first_x_on_extra;
          if (first_x_off_byte > first_x_on_byte)
            memset(cd_mask + first_x_on_byte, set_val,
                   first_x_off_byte - first_x_on_byte);
          if (first_x_off_extra != clear_val)
            cd_mask[first_x_off_byte] = first_x_off_extra;
        }
    }
}


/* get delay settings for the specified color and mode */
static int canon_get_delay(canon_privdata_t* privdata,char color){
    int i=0;
    int delay = 0;
    const canon_delay_t* delaylist = privdata->mode->delay;

    while(delaylist && delaylist[i].color){
        if(delaylist[i].color == color){
           delay = delaylist[i].delay;
           break;
        }
        ++i;
    }
    if(delay > privdata->delay_max)
       privdata->delay_max = delay;
    return delay;
}


/* add a single channel to the dither engine */
static int canon_setup_channel(stp_vars_t *v,canon_privdata_t* privdata,int channel,int subchannel,const canon_inkset_t* ink,stp_shade_t** shades){
    if(ink->channel && ink->density > 0.0){
        int delay = canon_get_delay(privdata,ink->channel);
        canon_channel_t* current;
	stp_dprintf(STP_DBG_CANON, v, "canon_setup_channel: (start) privdata->num_channels %d\n", privdata->num_channels);
	stp_dprintf(STP_DBG_CANON, v, "canon_setup_channel: (start) privdata->channel_order %s\n", privdata->channel_order);

        /* create a new channel */
        privdata->channels = stp_realloc(privdata->channels,sizeof(canon_channel_t) * (privdata->num_channels + 1));
        privdata->channel_order = stp_realloc(privdata->channel_order,privdata->num_channels + 2);
        /* update channel order */
        privdata->channel_order[privdata->num_channels]=ink->channel;
	stp_dprintf(STP_DBG_CANON, v, "canon_setup_channel: ink->channel %c\n", ink->channel);
        privdata->channel_order[privdata->num_channels+1]='\0';
	stp_dprintf(STP_DBG_CANON, v, "canon_setup_channel: (terminated)privdata->channel_order %s\n", privdata->channel_order);
        current = &(privdata->channels[privdata->num_channels]);
        ++privdata->num_channels;
        /* fill ink properties */
        current->name = ink->channel;
	stp_dprintf(STP_DBG_CANON, v, "canon_setup_channel: current->name %c\n", current->name);
        current->props = ink->ink;
        current->delay = delay;
        /* calculate buffer length */
        current->buf_length = ((privdata->length * current->props->bits)+1)*(delay + 1);
        /* update maximum buffer length */
        if(current->buf_length > privdata->buf_length_max)
             privdata->buf_length_max = current->buf_length;
        /* allocate buffer for the raster data */
        current->buf = stp_zalloc(current->buf_length + 1);
        /* add channel to the dither engine */
        stp_dither_add_channel(v, current->buf , channel , subchannel);

        /* add shades to the shades array */
        *shades = stp_realloc(*shades,(subchannel + 1) * sizeof(stp_shade_t));
	/* move previous shades up one position as set_inks_full expects the subchannels first */
	if(subchannel)
		memmove(*shades + 1,*shades,sizeof(stp_shade_t) * subchannel);
        (*shades)[0].value = ink->density;
	stp_dprintf(STP_DBG_CANON, v, "canon_setup_channel: ink->density %.3f\n", ink->density);
        (*shades)[0].numsizes = ink->ink->numsizes;
	/* test for 4-4 inket with 8 levels spaced every 2nd */
	/*if (ink->ink->bits == 4)
	  (*shades)[0].numsizes = 8;*/

        (*shades)[0].dot_sizes = ink->ink->dot_sizes;
        return 1;
    }
    return 0;
}





/* setup the dither channels */
static void canon_setup_channels(stp_vars_t *v,canon_privdata_t* privdata){
    /* (in gutenprint notation) => KCMY,  1230 => CMYK etc. */
    const char default_channel_order[STP_NCOLORS] = {0,1,2,3};
    /* codes for the primary channels */
    const char primary[STP_NCOLORS] = {'K','C','M','Y',};
    /* codes for the subchannels */
    const char secondary[STP_NCOLORS] = {'k','c','m','y'};
    /* names of the density adjustment controls */
    const char *primary_density_control[STP_NCOLORS] = {"BlackDensity","CyanDensity","MagentaDensity","YellowDensity"};
    const char *secondary_density_control[STP_NCOLORS] = {NULL,"LightCyanTrans","LightMagentaTrans","LightYellowTrans"};
    /* ink darkness for every channel */
    const double ink_darkness[] = {1.0, 0.31 / .5, 0.61 / .97, 0.08};
    const char* channel_order = default_channel_order;



    int channel;
    int channel_idx;

    if(privdata->caps->channel_order)
        channel_order = privdata->caps->channel_order;


    /* loop through the dither channels */
    for(channel_idx = 0; channel_idx < STP_NCOLORS ; channel_idx++){
        int i;
        unsigned int subchannel = 0;
        stp_shade_t* shades = NULL;
	int is_black_channel = 0;
        channel = channel_order[channel_idx];
	stp_dprintf(STP_DBG_CANON, v, "canon_setup_channels: channel %d\n", channel);
        if(channel == STP_ECOLOR_K && privdata->used_inks & CANON_INK_K_MASK){ /* black channel */
            /* find K and k inks */
            for(i=0;i<privdata->mode->num_inks;i++){
                const canon_inkset_t* ink = &privdata->mode->inks[i];
                if(ink->channel == primary[channel] || ink->channel == secondary[channel]) {
                    subchannel += canon_setup_channel(v,privdata,channel,subchannel,ink,&shades);
		    /* Gernot: add */
		    stp_dprintf(STP_DBG_CANON, v, "canon_setup_channels: got a black channel\n");
		}
            }
	    is_black_channel = 1;
	    /* adding cmy channels */
	    /*}else if(channel != STP_ECOLOR_K && privdata->used_inks & (CANON_INK_CMY_MASK | CANON_INK_cmy_MASK)){ */ /* color channels */
        }else if(channel != STP_ECOLOR_K && privdata->used_inks & CANON_INK_CMY_MASK){  /* color channels */
            for(i=0;i<privdata->mode->num_inks;i++){
                const canon_inkset_t* ink = &privdata->mode->inks[i];
		stp_dprintf(STP_DBG_CANON, v, "canon_setup_channels: loop non-K inks %d\n", i);
                /*if(ink->channel == primary[channel] || ((privdata->used_inks & (CANON_INK_CcMmYyKk_MASK | CANON_INK_cmy_MASK)) && (ink->channel == secondary[channel]))) {*/
                if(ink->channel == primary[channel] || ((privdata->used_inks & (CANON_INK_CcMmYyKk_MASK|CANON_INK_CMY_MASK)) && (ink->channel == secondary[channel]))) {
		/* Gernot: see if this works: use the masks that includes secondary channels */
                /* if(ink->channel == primary[channel] || ((privdata->used_inks & CANON_INK_CMYKk_MASK ) && (ink->channel == secondary[channel]))) {*/
		  subchannel += canon_setup_channel(v,privdata,channel,subchannel,ink,&shades);
		  stp_dprintf(STP_DBG_CANON, v, "canon_setup_channels: adding subchannel\n");
		}
		else {
		  stp_dprintf(STP_DBG_CANON, v, "canon_setup_channels: not creating subchannel\n");
		}
            }
        }

        /* set inks and density */
        if(shades){
          stp_dither_set_inks_full(v,channel, subchannel, shades, 1.0, ink_darkness[channel]);
          for(i=0;i<subchannel;i++){
            double density = get_double_param(v, primary_density_control[channel]) * get_double_param(v, "Density");
	    stp_dprintf(STP_DBG_CANON, v, "canon_setup_channels: loop subchannels for shades %d\n", i);
            if(i > 0 && secondary_density_control[channel])
              density *= get_double_param(v, secondary_density_control[channel]);
            stp_channel_set_density_adjustment(v,channel,subchannel,density);
          }
	  if (is_black_channel)
	    stp_channel_set_black_channel(v, channel);
          stp_free(shades);
        }
    }
}







/* FIXME move this to printercaps, and/or Esc (p command  */
#define CANON_CD_X 176
#define CANON_CD_Y 405

static void setup_page(stp_vars_t* v,canon_privdata_t* privdata){
  const char *media_source = stp_get_string_parameter(v, "InputSlot");
  const char *cd_type = stp_get_string_parameter(v, "PageSize");
  int print_cd = (media_source && (!strcmp(media_source, "CD")));
  stp_dimension_t page_left,
                  page_top,
                  page_right,
                  page_bottom;
  int hub_size = 0;

#if 0
  /* needed in workaround for Oufuku Hagaki */
  const stp_papersize_t *pp = stpi_get_papersize_by_size(v,
							stp_get_page_height(v),
							stp_get_page_width(v));

  if (pp)
    {
      const char *name = pp->name;
      if (!strcmp(name,"w420h567")) {
	/* workaround for Oufuku Hagaki: wrong orientation */
	privdata->page_width = stp_get_width(v);
	privdata->page_height = stp_get_height(v);
	stp_set_page_width(v, privdata->page_height);
	stp_set_page_height(v, privdata->page_width);
      }
    }
#endif

  if (cd_type && (strcmp(cd_type, "CDCustom") == 0 ))
    {
      int outer_diameter = stp_get_dimension_parameter(v, "CDOuterDiameter");
      stp_set_page_width(v, outer_diameter);
      stp_set_page_height(v, outer_diameter);
      stp_set_width(v, outer_diameter);
      stp_set_height(v, outer_diameter);
      hub_size = stp_get_dimension_parameter(v, "CDInnerDiameter");
    }
  else
    {
      const char *inner_radius_name = stp_get_string_parameter(v, "CDInnerRadius");
      hub_size = 43 * 10 * 72 / 254;		/* 43 mm standard CD hub */

      if (inner_radius_name && strcmp(inner_radius_name, "Small") == 0)
	hub_size = 16 * 10 * 72 / 254;		/* 15 mm prints to the hole - play it
						   safe and print 16 mm */
    }

  privdata->top = stp_get_top(v);
  privdata->left = stp_get_left(v);
  privdata->out_width = stp_get_width(v); /* check Epson: page_true_width */
  privdata->out_height = stp_get_height(v); /* check Epson: page_true_height */

  stp_dprintf(STP_DBG_CANON, v,"stp_get_width: privdata->out_width is %i\n",privdata->out_width);
  stp_dprintf(STP_DBG_CANON, v,"stp_get_height: privdata->out_height is %i\n",privdata->out_height);

  /* Don't use full bleed mode if the paper itself has a margin */
  if (privdata->left > 0 || privdata->top > 0)
    stp_set_boolean_parameter(v, "FullBleed", 0);

  internal_imageable_area(v, 0, 0, &page_left, &page_right,
                          &page_bottom, &page_top);
  if (print_cd) {
    privdata->cd_inner_radius = hub_size / 2;
    privdata->cd_outer_radius = stp_get_width(v) / 2;
    privdata->left = CANON_CD_X - privdata->cd_outer_radius + stp_get_dimension_parameter(v, "CDXAdjustment");;
    privdata->top = CANON_CD_Y - privdata->cd_outer_radius + stp_get_dimension_parameter(v, "CDYAdjustment");
    privdata->page_width = privdata->left + privdata->out_width;
    privdata->page_height = privdata->top + privdata->out_height;
  } else {
    privdata->left -= page_left;
    privdata->top -= page_top; /* checked in Epson: matches */
    privdata->page_width = page_right - page_left; /* checked in Epson: matches */
    privdata->page_height = page_bottom - page_top; /* checked in Epson: matches */
  }

  stp_dprintf(STP_DBG_CANON, v, "setup_page page_top = %f\n",page_top);
  stp_dprintf(STP_DBG_CANON, v, "setup_page page_bottom = %f\n",page_bottom);
  stp_dprintf(STP_DBG_CANON, v, "setup_page page_left = %f\n",page_left);
  stp_dprintf(STP_DBG_CANON, v, "setup_page page_right = %f\n",page_right);
  stp_dprintf(STP_DBG_CANON, v, "setup_page top = %i\n",privdata->top);
  stp_dprintf(STP_DBG_CANON, v, "setup_page left = %i\n",privdata->left);
  stp_dprintf(STP_DBG_CANON, v, "setup_page out_height = %i\n",privdata->out_height);
  stp_dprintf(STP_DBG_CANON, v, "setup_page page_height = %i\n",privdata->page_height);
  stp_dprintf(STP_DBG_CANON, v, "setup_page page_width = %i\n",privdata->page_width);

}


/* combine all curve parameters in s and apply them */
static void canon_set_curve_parameter(stp_vars_t *v,const char* type,stp_curve_compose_t comp,const char* s1,const char* s2,const char* s3){
  const char * s[3];
  size_t count = sizeof(s) / sizeof(s[0]);
  stp_curve_t *ret = NULL;
  int curve_count = 0;
  int i;
  const size_t piecewise_point_count = 384;


  /* ignore settings from the printercaps if the user specified his own parameters */
  if(stp_check_curve_parameter(v,type, STP_PARAMETER_ACTIVE))
    return;

  /* init parameter list (FIXME pass array directly???)*/
  s[0] = s1;
  s[1] = s2;
  s[2] = s3;

  /* skip empty curves */
  for(i=0;i<count;i++){
    if(s[i])
      s[curve_count++] = s[i];
  }

  /* combine curves */
  if(curve_count){
    for(i=0;i<curve_count;i++){
      stp_curve_t* t_tmp = stp_curve_create_from_string(s[i]);
      if(t_tmp){
        if(stp_curve_is_piecewise(t_tmp)){
          stp_curve_resample(t_tmp, piecewise_point_count);
        }
        if(!ret){
          ret = t_tmp;
        }else{
          stp_curve_t* t_comp = NULL;
          stp_curve_compose(&t_comp, ret, t_tmp, comp, -1);
          if(t_comp){
            stp_curve_destroy(ret);
            ret = t_comp;
          }
          stp_curve_destroy(t_tmp);
        }
      }
    }
  }

  /* apply result */
  if(ret){
    stp_set_curve_parameter(v, type, ret);
    stp_curve_destroy(ret);
  }
}

/*
 * 'canon_print()' - Print an image to a CANON printer.
 */
static int
canon_do_print(stp_vars_t *v, stp_image_t *image)
{
  int i;
  int		status = 1;
  const char	*media_source = stp_get_string_parameter(v, "InputSlot");
  const char    *ink_type = stp_get_string_parameter(v, "InkType");
  const char    *duplex_mode = stp_get_string_parameter(v, "Duplex");
  int           page_number = stp_get_int_parameter(v, "PageNumber");
  const canon_cap_t * caps = canon_get_model_capabilities(v);
  const canon_modeuselist_t* mlist = caps->modeuselist;
#if 0
  const canon_modeuse_t* muse;
#endif
  /*  int monocheck = 0;
      int colcheck = 0; */
  int		x,y;		/* Looping vars */
  canon_privdata_t privdata;
  int		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
                errlast;	/* Last raster line loaded */
#if 0
		out_channels;	/* Output bytes per pixel */
#endif
  unsigned	zero_mask;
  int           print_cd = (media_source && (!strcmp(media_source, "CD")));
  int           image_height;
#if 0
  int           image_width;
#endif
  double        k_upper, k_lower;
  unsigned char *cd_mask = NULL;
  double outer_r_sq = 0;
  double inner_r_sq = 0;
  unsigned char* weave_cols[4] ; /* TODO clean up weaving code to be more generic */

  stp_dprintf(STP_DBG_CANON, v, "Entering canon_do_print\n");

  if (!stp_verify(v))
    {
      stp_eprintf(v, "Print options not verified; cannot print.\n");
      return 0;
    }
  /*
  * Setup a read-only pixel region for the entire image...
  */

  stp_image_init(image);


  /* rotate even pages for DuplexNoTumble */
  if((page_number & 1) && duplex_mode && !strcmp(duplex_mode,"DuplexNoTumble"))
  	image = stpi_buffer_image(image,BUFFER_FLAG_FLIP_X | BUFFER_FLAG_FLIP_Y);

  memset(&privdata,0,sizeof(canon_privdata_t));
  privdata.caps = caps;

  /* find the wanted media type */
  /* - media type has priority
     - then we select source
     - then inkset (cartridge selection)
     - then we select duplex
     - after that we compare if mode is compatible with media
     - if not, we replace it using closest quality setting
     - then we decide on printhead colors based on actual mode to use
   */

  privdata.pt = get_media_type(caps,stp_get_string_parameter(v, "MediaType"));
  privdata.slot = canon_source_type(media_source,caps);

  /* ---  make adjustment to InkSet based on Media --- */

  /* cartridge selection if any: default is Both---but should change to NULL if CANON_CAP_T is not available */
  /* check if InkSet chosen is possible for this Media */
  /* - if Black, check if modes for selected media have a black flag */
  /*   else, set InkSet to "Both" for now */

  /* scroll through modeuse list to find media */
  for(i=0;i<mlist->count;i++){
    if(!strcmp(privdata.pt->name,mlist->modeuses[i].name)){
#if 0
      muse = &mlist->modeuses[i];
#endif
      break;
    }
  }

  if ( !strcmp(stp_get_string_parameter(v, "InkSet"),"Black")) {
    /* check if there is any mode for that media with K-only inktype */
    /* if not, change it to "Both" */
    /* NOTE: User cannot force monochrome printing here, since that would require changing the Color Model */
    if (!(mlist->modeuses[i].use_flags & INKSET_BLACK_SUPPORT)) {
      stp_set_string_parameter(v, "InkSet", "Both");
    }
  }
  /* Color-only */
  else if ( !strcmp(stp_get_string_parameter(v, "InkSet"),"Color") && (caps->features & CANON_CAP_T) ) {
    /* check if there is any mode for that media with no K in the inkset at all */
    /* if not, change it to "Both" */
    if (!(mlist->modeuses[i].use_flags & INKSET_COLOR_SUPPORT)) {
      stp_set_string_parameter(v, "InkSet", "Both");
    }
  }
  /* no restriction for "Both" (non-BJC) or "Color" (BJC) or "Photo" yet */

  /* get InkSet after adjustment */
  privdata.ink_set = stp_get_string_parameter(v, "InkSet");

  /* --- no current restrictions for Duplex setting --- */

  /* in particular, we do not constrain duplex printing to certain media */
  privdata.duplex_str = duplex_mode;

  /* ---  make adjustment to InkType to comply with InkSet --- */

  /*  although InSet adjustment is pre-supposed, even if InkSet is not
      adjusted, the InkType adjustment will be validated against mode
      later */
  if (!strcmp(privdata.ink_set,"Black")) {
    if (strcmp(ink_type,"Gray")) {/* if ink_type is NOT set to Gray yet */
      stp_dprintf(STP_DBG_CANON, v, "canon_do_print: InkSet Black, so InkType set to Gray\n");
      stp_set_string_parameter(v, "InkType", "Gray");
    }
  } /* Color-only */
  else if ( !strcmp(privdata.ink_set,"Color") && (caps->features & CANON_CAP_T) ) {
    if (strcmp(ink_type,"RGB")) {/* if ink_type is NOT set to RGB (CMY) yet */
      stp_dprintf(STP_DBG_CANON, v, "canon_do_print: InkSet Color, so InkType changed to RGB (CMY)\n");
      stp_set_string_parameter(v, "InkType", "RGB");
    }
  } /* no restriction for InkSet set to "Both" or "Photo" */

  /* --- make adjustments to mode --- */

  /* find the wanted print mode: NULL if not yet set */
  stp_dprintf(STP_DBG_CANON, v, "canon_do_print: calling canon_get_current_mode\n");
  privdata.mode = canon_get_current_mode(v);

  if(!privdata.mode) {
    privdata.mode = &caps->modelist->modes[caps->modelist->default_mode];
  }

  /* then call get_current_mode again to sort out the correct matching of parameters and mode selection */

  stp_dprintf(STP_DBG_CANON, v, "canon_do_print: calling canon_check_current_mode\n");

  privdata.mode = canon_check_current_mode(v);

  /* --- completed all adjustments: options should be consistent --- */

  /* set quality */
  privdata.quality = privdata.mode->quality;


  /* force grayscale if image is grayscale
   *                 or single black cartridge installed
   */
  privdata.used_inks = canon_printhead_colors(v);
  if (privdata.used_inks == CANON_INK_K)
      stp_set_string_parameter(v, "PrintingMode", "BW");
  else
    stp_set_string_parameter(v, "PrintingMode", "Color");

  setup_page(v,&privdata);

  image_height = stp_image_height(image);

#if 0
  image_width = stp_image_width(image);
#endif

  privdata.is_first_page = (page_number == 0);

 /*
  * Convert image size to printer resolution...
  */
#if 0
  stp_dprintf(STP_DBG_CANON, v,"canon_do_print: unused image_width is %i pts(?)\n",image_width);
#endif
  stp_dprintf(STP_DBG_CANON, v,"canon_do_print: privdata.out_width is %i pts\n",privdata.out_width);
  stp_dprintf(STP_DBG_CANON, v,"canon_do_print: privdata.out_height is %i pts\n",privdata.out_height);
  stp_dprintf(STP_DBG_CANON, v,"canon_do_print: privdata.left is %i pts\n",privdata.left);

  privdata.out_width  = privdata.mode->xdpi * privdata.out_width / 72;
  privdata.out_height = privdata.mode->ydpi * privdata.out_height / 72;
  privdata.left       = privdata.mode->xdpi * privdata.left / 72;

  stp_dprintf(STP_DBG_CANON, v,"canon_do_print: privdata.out_width is %i dots\n",privdata.out_width);
  stp_dprintf(STP_DBG_CANON, v,"canon_do_print: privdata.out_height is %i dots\n",privdata.out_height);
  stp_dprintf(STP_DBG_CANON, v,"canon_do_print: privdata.left is %i dots\n",privdata.left);

  stp_dprintf(STP_DBG_CANON, v,"density is %f\n",
               stp_get_float_parameter(v, "Density"));

  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */

  if (!stp_check_float_parameter(v, "Density", STP_PARAMETER_DEFAULTED))
    {
      stp_set_float_parameter_active(v, "Density", STP_PARAMETER_ACTIVE);
      stp_set_float_parameter(v, "Density", 1.0);
    }

  stp_scale_float_parameter(v, "Density", privdata.pt->base_density);
  stp_scale_float_parameter(v, "Density",privdata.mode->density);

  if (stp_get_float_parameter(v, "Density") > 1.0)
    stp_set_float_parameter(v, "Density", 1.0);

  if (privdata.used_inks == CANON_INK_K)
    stp_scale_float_parameter(v, "Gamma", 1.25);
  stp_scale_float_parameter( v, "Gamma", privdata.mode->gamma );

  stp_dprintf(STP_DBG_CANON, v,"density is %f\n",
               stp_get_float_parameter(v, "Density"));

  if(privdata.used_inks & CANON_INK_CMYK_MASK)
    stp_set_string_parameter(v, "STPIOutputType", "KCMY");
  else if(privdata.used_inks & CANON_INK_CMY_MASK)
    stp_set_string_parameter(v, "STPIOutputType", "CMY");
  /* Gernot: addition */
  /*else if(privdata.used_inks & CANON_INK_cmy_MASK)
    stp_set_string_parameter(v, "STPIOutputType", "cmy");*/
  else
    stp_set_string_parameter(v, "STPIOutputType", "Grayscale");

  privdata.length = (privdata.out_width + 7) / 8;

  stp_dprintf(STP_DBG_CANON, v,"privdata.length is %i\n",privdata.length);
  stp_dprintf(STP_DBG_CANON, v,"canon_do_print: privdata.num_channels is %i\n",privdata.num_channels);

  stp_dither_init(v, image, privdata.out_width, privdata.mode->xdpi, privdata.mode->ydpi);


  stp_dprintf(STP_DBG_CANON, v,"privdata.out_width is %i (after stp_dither_init)\n",privdata.out_width);
  stp_dprintf(STP_DBG_CANON, v,"privdata.length is %i (after stp_dither_init)\n",privdata.length);
  stp_dprintf(STP_DBG_CANON, v,"canon_do_print: privdata.num_channels is %i (after stp_dither_init)\n",privdata.num_channels);

  canon_setup_channels(v,&privdata);

  stp_dprintf(STP_DBG_CANON, v,"privdata.out_width is %i (after canon_setup_channels)\n",privdata.out_width);
  stp_dprintf(STP_DBG_CANON, v,"privdata.length is %i (after canon_setup_channels)\n",privdata.length);
  stp_dprintf(STP_DBG_CANON, v,"canon_do_print: privdata.num_channels is %i (after canon_setup_channels)\n",privdata.num_channels);

  stp_dprintf(STP_DBG_CANON, v,
	       "canon: driver will use colors %s\n",privdata.channel_order);

  /* Allocate compression buffer */
  if(caps->features & CANON_CAP_I)
    /*privdata.comp_buf = stp_zalloc(privdata.buf_length_max * 2 * caps->raster_lines_per_block * privdata.num_channels); */
      privdata.comp_buf = stp_zalloc(stp_compute_tiff_linewidth(v, privdata.buf_length_max * 2 * privdata.mode->raster_lines_per_block * privdata.num_channels)); /* for multiraster we need to buffer 8 lines for every color */
  else
      privdata.comp_buf = stp_zalloc(stp_compute_tiff_linewidth(v, privdata.buf_length_max * 2));
  /* Allocate fold buffer */
  privdata.fold_buf = stp_zalloc(stp_compute_tiff_linewidth(v, privdata.buf_length_max));



 /*
  * Output the page...
  */

   /* FIXME this is probably broken, kept for backward compatibility */
   if(privdata.num_channels > 4){
       k_lower = 0.4 / privdata.channels[4].props->bits + .1;
   }else
       k_lower = 0.25;

  k_lower *= privdata.pt->k_lower_scale;
  k_upper = privdata.pt->k_upper;

  if (!stp_check_float_parameter(v, "GCRLower", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRLower", k_lower);
  if (!stp_check_float_parameter(v, "GCRUpper", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRUpper", k_upper);


  /* init the printer */
  canon_init_printer(v, &privdata);

  /* initialize weaving for S200 for resolutions > 360dpi */
  if (privdata.mode->flags & MODE_FLAG_WEAVE)
     {
       char weave_color_order[] = "KCMY";

       privdata.stepper_ydpi = 720;
       privdata.nozzle_ydpi = 360;
       if (privdata.mode->xdpi == 2880)
         privdata.physical_xdpi = 2880;
       else
         privdata.physical_xdpi = 720;

       stp_dprintf(STP_DBG_CANON, v,"canon: adjust leftskip: old=%d,\n", privdata.left);
       privdata.left = (int)( (float)privdata.left * (float)privdata.physical_xdpi / (float)privdata.mode->xdpi ); /* adjust left margin */
       stp_dprintf(STP_DBG_CANON, v,"canon: adjust leftskip: new=%d,\n", privdata.left);

       privdata.ncolors = 4;
       privdata.head_offset = stp_zalloc(sizeof(int) * privdata.ncolors);
       memset(privdata.head_offset, 0, sizeof(*privdata.head_offset));

       if ( privdata.used_inks == CANON_INK_K )
           privdata.nozzles = 64; /* black nozzles */
       else
           privdata.nozzles = 24; /* color nozzles */
       if ( privdata.used_inks == CANON_INK_K )
         {
           privdata.ncolors = 1;
           privdata.head_offset[0] = 0; /* K starts at 0 */
           privdata.head_offset[1] = 0 ;/* how far C starts after K */
           privdata.head_offset[2] = 0;/* how far M starts after K */
           privdata.head_offset[3] = 0;/* how far Y starts after K */
           privdata.top += 11;
         }
       else if ( privdata.used_inks == CANON_INK_CMYK )
         {
           privdata.head_offset[0] = 0; /* K starts at 0 */
           privdata.head_offset[1] = 144 ;/* how far C starts after K */
           privdata.head_offset[2] = 144 + 64;/* how far M starts after K */
           privdata.head_offset[3] = 144 + 64 + 64;/* how far Y starts after K */
           privdata.top += 5;
         }
       else  /* colormode == CMY */
         {
           privdata.ncolors = 3;
           privdata.head_offset[0] = 0; /* K starts at 0 */
           privdata.head_offset[1] = 0 ;/* how far C starts after K */
           privdata.head_offset[2] = 64;/* how far M starts after K */
           privdata.head_offset[3] = 128;/* how far Y starts after K */
           privdata.top += 18;
         }

       privdata.nozzle_separation = privdata.stepper_ydpi / privdata.nozzle_ydpi;
       privdata.horizontal_passes = privdata.mode->xdpi / privdata.physical_xdpi;
       privdata.vertical_passes = 1;
       privdata.vertical_oversample = privdata.mode->ydpi / privdata.stepper_ydpi;
       privdata.bidirectional = 1; /* 1: bidirectional; 0: unidirectional  printing */
       privdata.direction = 1;
       stp_allocate_component_data(v, "Driver", NULL, NULL, &privdata);
       stp_dprintf(STP_DBG_CANON, v,"canon: initializing weaving: nozzles=%d, nozzle_separation=%d,\n"
                                    "horizontal_passes=%d, vertical_passes=%d,vertical_oversample=%d,\n"
                                    "ncolors=%d, out_width=%d, out_height=%d\n"
                                    "weave_top=%d, weave_page_height=%d \n"
                                    "head_offset=[%d,%d,%d,%d]  \n",
                                    privdata.nozzles, privdata.nozzle_separation,
                                    privdata.horizontal_passes, privdata.vertical_passes,
                                    privdata.vertical_oversample, privdata.ncolors,
                                    privdata.out_width, privdata.out_height,
                                    privdata.top * privdata.stepper_ydpi / 72, privdata.page_height * privdata.stepper_ydpi / 72,
                                    privdata.head_offset[0],privdata.head_offset[1],
                                    privdata.head_offset[2],privdata.head_offset[3]);

       stp_initialize_weave(v, privdata.nozzles, privdata.nozzle_separation,
                                privdata.horizontal_passes, privdata.vertical_passes,
                                privdata.vertical_oversample, privdata.ncolors,
                                1,
                                privdata.out_width, privdata.out_height,
                                privdata.top * privdata.stepper_ydpi / 72,
                                privdata.page_height * privdata.stepper_ydpi / 72,
                                privdata.head_offset,
                                STP_WEAVE_ZIGZAG,
                                canon_flush_pass,
                                stp_fill_uncompressed,
                                stp_pack_uncompressed,
                                stp_compute_tiff_linewidth);
       privdata.last_pass_offset = 0;

       if (stp_get_debug_level() & STP_DBG_CANON) {
	 for(x=0;x<4;x++){
	   stp_dprintf(STP_DBG_CANON, v, "DEBUG print-canon weave: weave_color_order[%d]: %u\n",
		       x, (unsigned int)weave_color_order[x]);
	 }
	 for(x=0;x<privdata.num_channels;x++){
	   stp_dprintf(STP_DBG_CANON, v, "DEBUG print-canon weave: channel_order[%d]: %u\n",
		       x, (unsigned int)privdata.channel_order[x]);
	 }
       }

       for(i=0;i<4;i++){/* need all 4 channels for weave_cols, but not for privdata.num_channels! */
	   /* see if it helps to initialize to zero */
	   weave_cols[i] = 0;
	   privdata.weave_bits[i] = 0;

           for(x=0;x<privdata.num_channels;x++){
	     if(weave_color_order[i] == privdata.channel_order[x]){
	       weave_cols[i] = privdata.channels[x].buf;
	       privdata.weave_bits[i] = privdata.channels[x].props->bits;
	       stp_dprintf(STP_DBG_CANON, v, "DEBUG print-canon weave: set weave_cols[%d] to privdata.channels[%d].buf\n",
			     i, x);
	     }
           }
       }
  }


  errdiv  = image_height / privdata.out_height;
  errmod  = image_height % privdata.out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;

  /* set Hue, Lum and Sat Maps */
  canon_set_curve_parameter(v,"HueMap",STP_CURVE_COMPOSE_ADD,caps->hue_adjustment,privdata.pt->hue_adjustment,privdata.mode->hue_adjustment);
  canon_set_curve_parameter(v,"LumMap",STP_CURVE_COMPOSE_MULTIPLY,caps->lum_adjustment,privdata.pt->lum_adjustment,privdata.mode->lum_adjustment);
  canon_set_curve_parameter(v,"SatMap",STP_CURVE_COMPOSE_MULTIPLY,caps->sat_adjustment,privdata.pt->sat_adjustment,privdata.mode->sat_adjustment);

#if 0
  out_channels = stp_color_init(v, image, 65536);
#endif
  (void) stp_color_init(v, image, 65536);
  stp_allocate_component_data(v, "Driver", NULL, NULL, &privdata);

  privdata.emptylines = 0;
  if (print_cd) {
    cd_mask = stp_malloc(1 + (privdata.out_width + 7) / 8);
    outer_r_sq = (double)privdata.cd_outer_radius * (double)privdata.cd_outer_radius;
    inner_r_sq = (double)privdata.cd_inner_radius * (double)privdata.cd_inner_radius;
  }
  for (y = 0; y < privdata.out_height; y ++)
  {
    int duplicate_line = 1;

    if (errline != errlast)
    {
      errlast = errline;
      duplicate_line = 0;
      if (stp_color_get_row(v, image, errline, &zero_mask))
	{
	  status = 2;
	  break;
	}
    }
    if (print_cd)
      {
	int x_center = privdata.cd_outer_radius * privdata.mode->xdpi / 72;
	int y_distance_from_center =
	  privdata.cd_outer_radius - (y * 72 / privdata.mode->ydpi);
	if (y_distance_from_center < 0)
	  y_distance_from_center = -y_distance_from_center;
	memset(cd_mask, 0, (privdata.out_width + 7) / 8);
	if (y_distance_from_center < privdata.cd_outer_radius)
	  {
	    double y_sq = (double) y_distance_from_center *
	      (double) y_distance_from_center;
	    int x_where = sqrt(outer_r_sq - y_sq) + .5;
	    int scaled_x_where = x_where * privdata.mode->xdpi / 72;
	    set_mask(cd_mask, x_center, scaled_x_where,
		     privdata.out_width, 1, 0);
	    if (y_distance_from_center < privdata.cd_inner_radius)
	      {
		x_where = sqrt(inner_r_sq - y_sq) + .5;
		scaled_x_where = x_where * privdata.mode->ydpi / 72;
		set_mask(cd_mask, x_center, scaled_x_where,
			 privdata.out_width, 1, 1);
	      }
	  }
      }
    stp_dither(v, y, duplicate_line, zero_mask, cd_mask);
    if ( privdata.mode->flags & MODE_FLAG_WEAVE )
        stp_write_weave(v, weave_cols);
    else if ( caps->features & CANON_CAP_I)
        canon_write_multiraster(v,&privdata,y);
    else
        canon_printfunc(v);
    errval += errmod;
    errline += errdiv;
    if (errval >= privdata.out_height)
    {
      errval -= privdata.out_height;
      errline ++;
    }
  }

  if ( privdata.mode->flags & MODE_FLAG_WEAVE )
  {
      stp_flush_all(v);
      canon_advance_paper(v, 5);
  }
  else
  {

  /*
   * Flush delayed buffers...
   */

  if (privdata.delay_max) {
    stp_dprintf(STP_DBG_CANON, v,"\ncanon: flushing %d possibly delayed buffers\n",
		 privdata.delay_max);
    for (y= 0; y<privdata.delay_max; y++) {

      canon_write_line(v);
      for (i = 0; i < privdata.num_channels; i++)
	canon_advance_buffer(privdata.channels[i].buf, privdata.length,
			     privdata.channels[i].delay);
    }
  }
  }
  stp_image_conclude(image);

 /*
  * Cleanup...
  */

  stp_free(privdata.fold_buf);
  stp_free(privdata.comp_buf);

  if(cd_mask)
      stp_free(cd_mask);


  canon_deinit_printer(v, &privdata);
  /* canon_end_job does not get called for jobmode automatically */
  if(!stp_get_string_parameter(v, "JobMode") ||
    strcmp(stp_get_string_parameter(v, "JobMode"), "Page") == 0){
    canon_end_job(v,image);
  }

  for(i=0;i< privdata.num_channels;i++)
      if(privdata.channels[i].buf)
          stp_free(privdata.channels[i].buf);
  if(privdata.channels)
      stp_free(privdata.channels);

  stp_free(privdata.channel_order);
  if (privdata.head_offset)
    stp_free(privdata.head_offset);


  return status;
}

static int
canon_print(const stp_vars_t *v, stp_image_t *image)
{
  int status;
  stp_vars_t *nv = stp_vars_create_copy(v);
  status = canon_do_print(nv, image);
  stp_vars_destroy(nv);
  return status;
}

static const stp_printfuncs_t print_canon_printfuncs =
{
  canon_list_parameters,
  canon_parameters,
  stp_default_media_size,
  canon_imageable_area,
  canon_maximum_imageable_area,
  canon_limit,
  canon_print,
  canon_describe_resolution,
  canon_describe_output,
  stp_verify_printer_params,
  canon_start_job,
  canon_end_job,
  NULL,
  stpi_standard_describe_papersize
};

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


/* fold, apply the necessary compression, pack tiff and return the compressed length */
static int canon_compress(stp_vars_t *v, canon_privdata_t *pd, unsigned char* line,int length,int offset,unsigned char* comp_buf,int bits, int ink_flags)
{
  unsigned char
    *in_ptr= line,
    *comp_ptr, *comp_data;
  int offset2,bitoffset;

  /* Don't send blank lines... */

  if (line[0] == 0 && memcmp(line, line + 1, (length * bits)  - 1) == 0)
    return 0;

  /* if no modulation: 1 bit per pixel */

  offset2 = offset / 8;
  bitoffset = offset % 8;

  /* fold lsb/msb pairs if drop modulation is active */

  if (bits==2) {
    int pixels_per_byte = 4;
    if(ink_flags & INK_FLAG_5pixel_in_1byte)
      pixels_per_byte = 5;

    stp_fold(line,length,pd->fold_buf);
    in_ptr    = pd->fold_buf;
    length    = (length*8/4); /* 4 pixels in 8bit */
    /* calculate the number of compressed bytes that can be sent directly */
    offset2   = offset / pixels_per_byte;
    /* calculate the number of (uncompressed) bits that have to be added to the raster data */
    bitoffset = (offset % pixels_per_byte) * 2;
  }
  else if (bits==3) {
    stp_fold_3bit_323(line,length,pd->fold_buf);
    in_ptr  = pd->fold_buf;
    length  = (length*8)/3;
    offset2 = offset/3;
#if 0
    switch(offset%3){
    case 0: offset= (offset/3)*8;   break;
    case 1: offset= (offset/3)*8/*+3 CAREFUL! CANNOT SHIFT _AFTER_ RECODING!!*/; break;
    case 2: offset= (offset/3)*8/*+5 CAREFUL! CANNOT SHIFT _AFTER_ RECODING!!*/; break;
    }
#endif
    bitoffset= 0;
  }
  else if (bits==4) {
    int pixels_per_byte = 2;
    if(ink_flags & INK_FLAG_3pixel5level_in_1byte)
      pixels_per_byte = 3;
    else if(ink_flags & INK_FLAG_3pixel6level_in_1byte)
      pixels_per_byte = 3;

    stp_fold_4bit(line,length,pd->fold_buf);
    in_ptr    = pd->fold_buf;
    length    = (length*8)/2; /* 2 pixels in 8 bits */
    /* calculate the number of compressed bytes that can be sent directly */
    offset2   = offset / pixels_per_byte;
    /* calculate the number of (uncompressed) bits that have to be added to the raster data */
    bitoffset = (offset % pixels_per_byte) * 2; /* not sure what this value means */
  }
  else if (bits==8) {
    stp_fold_8bit(line,length,pd->fold_buf);
    in_ptr= pd->fold_buf;
    length    = length*8; /* 1 pixel per 8 bits */
    offset2   = offset;
    bitoffset = 0;
  }

  /* pack left border rounded to multiples of 8 dots */

  comp_data= comp_buf;

  while (offset2>0) {
    unsigned char toffset = offset2 > 127 ? 127 : offset2;
    comp_data[0] = 1 - toffset;
    comp_data[1] = 0;
    comp_data += 2;
    offset2-= toffset;
  }
  if (bitoffset) {
    if (bitoffset<8)
      {
	in_ptr[ length++ ] = 0;
	canon_shift_buffer(in_ptr,length,bitoffset);
      }
    else if (bitoffset == 8)
      {
	memmove(in_ptr + 1,in_ptr,length++);
	in_ptr[0] = 0;
      }
    else
      stp_dprintf(STP_DBG_CANON, v,"SEVERE BUG IN print-canon.c::canon_write() "
		   "bitoffset=%d!!\n",bitoffset);
  }

  if(ink_flags & INK_FLAG_5pixel_in_1byte)
    length = pack_pixels(in_ptr,length);
  else if(ink_flags & INK_FLAG_3pixel5level_in_1byte)
    length = pack_pixels3_5(in_ptr,length);
  else if(ink_flags & INK_FLAG_3pixel6level_in_1byte)
    length = pack_pixels3_6(in_ptr,length);


  stp_pack_tiff(v, in_ptr, length, comp_data, &comp_ptr, NULL, NULL);

  return comp_ptr - comp_buf;
}

/*
 * 'canon_write()' - Send graphics using TIFF packbits compression.
 */

static int
canon_write(stp_vars_t *v,		/* I - Print file or command */
            canon_privdata_t *pd,       /* privdata */
	    const canon_cap_t *   caps,	        /* I - Printer model */
	    unsigned char *line,	/* I - Output bitmap data */
	    int           length,	/* I - Length of bitmap data */
	    int           coloridx,	/* I - Which color */
	    int           *empty,       /* IO- Preceding empty lines */
	    int           width,	/* I - Printed width */
	    int           offset, 	/* I - Offset from left side */
	    int           bits,
            int           ink_flags)
{

  unsigned char color;
  int newlength = canon_compress(v,pd,line,length,offset,pd->comp_buf,bits,ink_flags);
  if(!newlength)
      return 0;
  /* send packed empty lines if any */

  if (*empty) {
    stp_zfwrite("\033\050\145\002\000", 5, 1, v);
    stp_put16_be(*empty, v);
    *empty= 0;
  }

 /* Send a line of raster graphics... */

  stp_zfwrite("\033\050\101", 3, 1, v);
  stp_put16_le(newlength + 1, v);
  color= "CMYKcmyk"[coloridx];
  if (!color) color= 'K';
  stp_putc(color,v);
  stp_zfwrite((const char *)pd->comp_buf, newlength, 1, v);
  stp_putc('\015', v);
  return 1;
}


static void
canon_write_line(stp_vars_t *v)
{
  canon_privdata_t *pd =
    (canon_privdata_t *) stp_get_component_data(v, "Driver");
  char write_sequence[] = "KYMCymck";
  static const int write_number[] = { 3, 2, 1, 0, 6, 5, 4, 7 };   /* KYMCymc */
  int i;
  int written= 0;
  for (i = 0; i < strlen(write_sequence) ; i++)
    {
      int x;
      const canon_channel_t* channel=NULL;
      int num = write_number[i];

      /* TODO optimize => move reorder code to do_print */
      for(x=0;x < pd->num_channels; x++){
	if(pd->channels[x].name == write_sequence[i]){
              channel = &(pd->channels[x]);
              break;
          }
      }
      if(channel){
        written += canon_write(v, pd, pd->caps,
                               channel->buf + channel->delay * pd->length /*buf_length[i]*/,
                               pd->length, num,
                               &(pd->emptylines), pd->out_width,
                               pd->left, channel->props->bits, channel->props->flags);
      }
    }
  if (written)
    stp_zfwrite("\033\050\145\002\000\000\001", 7, 1, v);
  else
    pd->emptylines += 1;
}


/* write one multiraster block */
static void canon_write_block(stp_vars_t* v,canon_privdata_t* pd,unsigned char* start, unsigned char* end){
    unsigned int length = end - start;
    if(!length)
        return;
    stp_zfwrite("\033(F", 3, 1, v);
    stp_put16_le(length, v);
    stp_zfwrite((const char *)start, length, 1, v);
}


static void canon_write_multiraster(stp_vars_t *v,canon_privdata_t* pd,int y){
    int i;
    /*int raster_lines_per_block = pd->caps->raster_lines_per_block;*/
    int raster_lines_per_block = pd->mode->raster_lines_per_block;
    unsigned int max_length = 2*pd->buf_length_max * raster_lines_per_block;
    /* a new raster block begins */
    if(!(y % raster_lines_per_block)){
        if(y != 0){
            /* write finished blocks */
            for(i=0;i<pd->num_channels;i++)
                canon_write_block(v,pd,pd->comp_buf + i * max_length,pd->channels[i].comp_buf_offset);
        }
        /* reset start offsets */
        for(i=0;i<pd->num_channels;i++)
            pd->channels[i].comp_buf_offset = pd->comp_buf + i * max_length;
    }
    /* compress lines and add them to the buffer */
    for(i=0;i<pd->num_channels;i++){
       pd->channels[i].comp_buf_offset += canon_compress(v,pd, pd->channels[i].buf,pd->length,pd->left,pd->channels[i].comp_buf_offset,pd->channels[i].props->bits, pd->channels[i].props->flags);
       *(pd->channels[i].comp_buf_offset) = 0x80; /* terminate the line */
        ++pd->channels[i].comp_buf_offset;
    }
    if(y == pd->out_height - 1){
        /* we just compressed our last line */
        if(pd->out_height % raster_lines_per_block){
            /* but our raster block is not finished yet */
            int missing = raster_lines_per_block - (pd->out_height % raster_lines_per_block); /* calculate missing lines */
            for(i=0;i<pd->num_channels;i++){
                /* add missing empty lines and write blocks */
                int x;
                for(x=0;x < missing ; x++){
                  *(pd->channels[i].comp_buf_offset) = 0x80; /* terminate the line */
                  ++pd->channels[i].comp_buf_offset;
                }
                canon_write_block(v,pd,pd->comp_buf + i * max_length,pd->channels[i].comp_buf_offset);
            }
        }
    }
}


static void
canon_advance_paper(stp_vars_t *v, int advance)
{
  if ( advance > 0 )
    {
      int a0, a1, a2, a3;
      stp_dprintf(STP_DBG_CANON, v,"                      --advance paper %d\n", advance);
      a0 = advance         & 0xff;
      a1 = (advance >> 8)  & 0xff;
      a2 = (advance >> 16) & 0xff;
      a3 = (advance >> 24) & 0xff;
      stp_zprintf(v, "\033(e%c%c%c%c%c%c", 4, 0, a3, a2, a1, a0);
    }
}

static void
canon_flush_pass(stp_vars_t *v, int passno, int vertical_subpass)
{
  stp_lineoff_t        *lineoffs   = stp_get_lineoffsets_by_pass(v, passno);
  stp_lineactive_t     *lineactive = stp_get_lineactive_by_pass(v, passno);
  const stp_linebufs_t *bufs       = stp_get_linebases_by_pass(v, passno);
  stp_pass_t           *pass       = stp_get_pass_by_pass(v, passno);
  stp_linecount_t      *linecount  = stp_get_linecount_by_pass(v, passno);
  canon_privdata_t      *pd         = (canon_privdata_t *) stp_get_component_data(v, "Driver");
  int                    papershift = (pass->logicalpassstart - pd->last_pass_offset);

  int color, line, written = 0, linelength = 0, lines = 0;
  int idx[4]={3, 0, 1, 2}; /* color numbering is different between canon_write and weaving */

  stp_dprintf(STP_DBG_CANON, v,"canon_flush_pass: ----pass=%d,---- \n", passno);
  (pd->emptylines) = 0;

  for ( color = 0; color < pd->ncolors; color++ ) /* find max. linecount */
    {
      if ( linecount[0].v[color] > lines )
        lines = linecount[0].v[color];
    }

  for ( line = 0; line < lines; line++ )  /* go through each nozzle f that pass */
    {
      stp_dprintf(STP_DBG_CANON, v,"                      --line=%d\n", line);

      if ( written > 0 )
        canon_cmd(v,ESC28,0x65, 2, 0, 1); /* go to next nozzle*/
                                           /* if there was printed some data */

      written = 0;
      for ( color = 0; color < pd->ncolors; color++ )
        {
          if ( line < linecount[0].v[color] )  /* try only existing lines */
            {
              if ( lineactive[0].v[color] > 0 )
                {
                  linelength = lineoffs[0].v[color] / linecount[0].v[color];
/*                stp_dprintf(STP_DBG_CANON, v,"canon_flush_pass: linelength=%d, bufs[0].v[color]=%p,"
                  "bufs[0].v[color]+line * linelength=%p, empty=%d \n", linelength, bufs[0].v[color],
                   bufs[0].v[color] + line * linelength, (pd->emptylines));
*/
                  if ( pass->logicalpassstart - pd->last_pass_offset > 0 )
                    {
                      canon_advance_paper(v, papershift);
                      pd->last_pass_offset = pass->logicalpassstart;
                      if (pd->bidirectional)
                        {
                         pd->direction = (pd->direction +1) & 1;
                         canon_set_X72(v, pd->direction);
                         stp_dprintf(STP_DBG_CANON, v,"                      --set direction %d\n", pd->direction);
                        }
                    }

                  written += canon_write(v, pd, pd->caps,
                               (unsigned char *)(bufs[0].v[color] + line * linelength),
                               linelength, idx[color],
                               &(pd->emptylines), pd->out_width,
                               pd->left, pd->weave_bits[color],0);
                  if (written) stp_dprintf(STP_DBG_CANON, v,"                        --written color %d,\n", color);

                }
            }
        }

      if ( written == 0 ) /* count unused nozzles */
        (pd->emptylines) += 1;
    }

  for ( color = 0; color < pd->ncolors; color++ )
    {
      lineoffs[0].v[color] = 0;
      linecount[0].v[color] = 0;
    }
  stp_dprintf(STP_DBG_CANON, v,"                  --ended-- with empty=%d \n", (pd->emptylines));
}

static stp_family_t print_canon_module_data =
  {
    &print_canon_printfuncs,
    NULL
  };


static int
print_canon_module_init(void)
{
  return stpi_family_register(print_canon_module_data.printer_list);
}


static int
print_canon_module_exit(void)
{
  return stpi_family_unregister(print_canon_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_canon_LTX_stp_module_version
#define stp_module_data print_canon_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "canon",
    VERSION,
    "Canon family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    print_canon_module_init,
    print_canon_module_exit,
    (void *) &print_canon_module_data
  };
