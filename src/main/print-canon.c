/*
 * "$Id$"
 *
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
#include <assert.h>
/* Solaris with gcc has problems because gcc's limits.h doesn't #define */
/* this */
#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

#include "print-canon.h"

#define MAX_CARRIAGE_WIDTH	13 /* This really needs to go away */

/*
 * We really need to get away from this silly static nonsense...
 */
#define MAX_PHYSICAL_BPI 1440
#define MAX_OVERSAMPLED 8
#define MAX_BPP 4
#define COMPBUFWIDTH (MAX_PHYSICAL_BPI * MAX_OVERSAMPLED * MAX_BPP * \
	MAX_CARRIAGE_WIDTH / CHAR_BIT)

#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static int
pack_pixels(unsigned char* buf,int len)
{
  int read_pos = 0;
  int write_pos = 0;
  int shift = 6;
  while(read_pos < len)
  {
    /* read 5pixels a 2 bit */
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

static const int channel_color_map[] =
{
  STP_ECOLOR_K, STP_ECOLOR_C, STP_ECOLOR_M, STP_ECOLOR_Y, STP_ECOLOR_C, STP_ECOLOR_M, STP_ECOLOR_Y
};

static const int subchannel_color_map[] =
{
  0, 0, 0, 0, 1, 1, 1
};

/* K,C,M,Y */
static const double ink_darknesses[] =
{
  1.0, 0.31 / .5, 0.61 / .97, 0.08
};

#define USE_3BIT_FOLD_TYPE 323

/* document feeding */
#define CANON_SLOT_ASF1    1
#define CANON_SLOT_ASF2    2
#define CANON_SLOT_MAN1    4
#define CANON_SLOT_MAN2    8

/* model peculiarities */
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
#define CANON_CAP_rr        0x4000ul
#define CANON_CAP_DUPLEX    0x40000ul

#define CANON_CAP_STD0 (CANON_CAP_b|CANON_CAP_c|CANON_CAP_d|\
                        CANON_CAP_l|CANON_CAP_q|CANON_CAP_t)

#define CANON_CAP_STD1 (CANON_CAP_b|CANON_CAP_c|CANON_CAP_d|CANON_CAP_l|\
                        CANON_CAP_m|CANON_CAP_p|CANON_CAP_q|CANON_CAP_t)


#include "canon-inks.h"
#include "canon-modes.h"
#include "canon-media.h"
#include "canon-printers.h"

typedef struct
{
  const canon_cap_t *caps;
  unsigned char *cols[7];
  stp_shade_t* shades[STP_NCOLORS];
  int num_shades[STP_NCOLORS];
  int delay[7];
  int delay_max;
  int buf_length[7];
  int length;
  int out_width;
  int left;
  int emptylines;
  int bits[7];
  int ink_flags[7];
  int ydpi;
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

} canon_privdata_t;

static void canon_write_line(stp_vars_t *v);


static const char plain_paper_lum_adjustment[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gutenprint>\n"
"<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
"<sequence count=\"48\" lower-bound=\"0\" upper-bound=\"4\">\n"
"1.20 1.22 1.28 1.34 1.39 1.42 1.45 1.48 "  /* C */
"1.50 1.40 1.30 1.25 1.20 1.10 1.05 1.05 "  /* B */
"1.05 1.05 1.05 1.05 1.05 1.05 1.05 1.05 "  /* M */
"1.05 1.05 1.05 1.10 1.10 1.10 1.10 1.10 "  /* R */
"1.10 1.15 1.30 1.45 1.60 1.75 1.90 2.00 "  /* Y */
"2.10 2.00 1.80 1.70 1.60 1.50 1.40 1.30 "  /* G */
"</sequence>\n"
"</curve>\n"
"</gutenprint>\n";

typedef struct {
  const canon_cap_t *caps;
  int is_first_page;
  const canon_paper_t *pt;
  unsigned int ink_type;
  const canon_mode_t* mode;
  const char *source_str;
  const char *duplex_str;
  int page_width;
  int page_height;
  int top;
  int left;
} canon_init_t;


static void canon_advance_paper(stp_vars_t *, int);
static void canon_flush_pass(stp_vars_t *, int, int);

static const stp_parameter_t the_parameters[] =
{
  {
    "PageSize", N_("Page Size"), N_("Basic Printer Setup"),
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "MediaType", N_("Media Type"), N_("Basic Printer Setup"),
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "InputSlot", N_("Media Source"), N_("Basic Printer Setup"),
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "Resolution", N_("Resolution"), N_("Basic Printer Setup"),
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "InkType", N_("Ink Type"), N_("Advanced Printer Setup"),
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "InkChannels", N_("Ink Channels"), N_("Advanced Printer Functionality"),
    N_("Ink Channels"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, -1, 0, 0
  },
  {
    "PrintingMode", N_("Printing Mode"), N_("Core Parameter"),
    N_("Printing Output Mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "Duplex", N_("Double-Sided Printing"), N_("Basic Printer Setup"),
    N_("Duplex/Tumble Setting"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  }
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
      "CyanDensity", N_("Cyan Density"), N_("Output Level Adjustment"),
      N_("Adjust the cyan density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 1, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "MagentaDensity", N_("Magenta Density"), N_("Output Level Adjustment"),
      N_("Adjust the magenta density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 2, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "YellowDensity", N_("Yellow Density"), N_("Output Level Adjustment"),
      N_("Adjust the yellow density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 3, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "BlackDensity", N_("Black Density"), N_("Output Level Adjustment"),
      N_("Adjust the black density"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 0, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "LightCyanTransition", N_("Light Cyan Transition"), N_("Advanced Ink Adjustment"),
      N_("Light Cyan Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightMagentaTransition", N_("Light Magenta Transition"), N_("Advanced Ink Adjustment"),
      N_("Light Magenta Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
 {
    {
      "LightYellowTransition", N_("Light Yellow Transition"), N_("Advanced Ink Adjustment"),
      N_("Light Yellow Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1, 0
    }, 0.0, 5.0, 1.0, 1
  },
};


static const int float_parameter_count =
sizeof(float_parameters) / sizeof(const float_param_t);

/*
 * Duplex support - modes available
 * Note that the internal names MUST match those in cups/genppd.c else the
 * PPD files will not be generated correctly
 *
 * TODO: Support for DuplexNoTumble, the image has to be rotated 
 */

static const stp_param_string_t duplex_types[] =
{
  { "None",             N_ ("Off") },
/*  { "DuplexNoTumble", N_ ("Long Edge (Standard)") } , */
  { "DuplexTumble",     N_ ("Short Edge (Flip)") }
};
#define NUM_DUPLEX (sizeof (duplex_types) / sizeof (stp_param_string_t))



static const canon_paper_t *
get_media_type(const canon_cap_t* caps,const char *name)
{
  int i;
  if (name && caps->paperlist)
    for (i = 0; i < caps->paperlist->count; i++)
      {
	/* translate paper_t.name */
	if (!strcmp(name, caps->paperlist->papers[i].name))
	  return &(caps->paperlist->papers[i]);
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
      if (!strcmp(name,"Cassette")) return 8;
      if (!strcmp(name,"CD")) return 10;
    }

  stp_deprintf(STP_DBG_CANON,"canon: Unknown source type '%s' - reverting to auto\n",name);
  return 4;
}


/* function returns the current set printmode (specified by resolution) */
/* if no mode is set the default mode will be returned */
static const canon_mode_t* canon_get_current_mode(const stp_vars_t *v){
    const char *resolution = stp_get_string_parameter(v, "Resolution");
    const canon_cap_t * caps = canon_get_model_capabilities(stp_get_model_id(v));
    const canon_mode_t* mode = NULL;
    int i;
    if(resolution){
        for(i=0;i<caps->modelist->count;i++){
            if(!strcmp(resolution,caps->modelist->modes[i].name)){
                mode = &caps->modelist->modes[i];
                break;
            }
        }
    }
    if(!mode)
        mode = &caps->modelist->modes[caps->modelist->default_mode];
    return mode;
}

/* function returns the best ink_type for the current mode */
static unsigned int
canon_printhead_colors(const stp_vars_t*v)
{
  int i;
  const canon_mode_t* mode;
  const char *print_mode = stp_get_string_parameter(v, "PrintingMode");
  const char *ink_type = stp_get_string_parameter(v, "InkType");
  if(print_mode && strcmp(print_mode, "BW") == 0)
    return CANON_INK_K;

  if(ink_type){
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
          if(ink_type && !strcmp(canon_inktypes[i].name,ink_type))
              return canon_inktypes[i].ink_type;
     }
  }
  mode = canon_get_current_mode(v);
  for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
    if(mode->ink_types & canon_inktypes[i].ink_type)
        return canon_inktypes[i].ink_type;
  }
  return CANON_INK_K;
}

static unsigned char
canon_size_type(const stp_vars_t *v, const canon_cap_t * caps)
{
  const stp_papersize_t *pp = stp_get_papersize_by_size(stp_get_page_height(v),
							stp_get_page_width(v));
  if (pp)
    {
      const char *name = pp->name;
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

static void
canon_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
    const canon_mode_t* mode = canon_get_current_mode(v);
    *x = mode->xdpi;
    *y = mode->ydpi;
}

static const char *
canon_describe_output(const stp_vars_t *v)
{
  unsigned int ink_type = canon_printhead_colors(v);

  if(ink_type & CANON_INK_CMYK_MASK)
    return "CMYK";
  if(ink_type & CANON_INK_CMY_MASK)
    return "CMY";
  return "Grayscale";
}

static const stp_param_string_t media_sources[] =
              {
                { "Auto",	N_ ("Auto Sheet Feeder") },
                { "Manual",	N_ ("Manual with Pause") },
                { "ManualNP",	N_ ("Manual without Pause") },
		{ "Cassette",   N_ ("Cassette") },
		{ "CD", N_ ("CD tray") }
              };



/*
 * 'canon_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
canon_list_parameters(const stp_vars_t *v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
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
  int		i;

  const canon_cap_t * caps=
    canon_get_model_capabilities(stp_get_model_id(v));
  description->p_type = STP_PARAMETER_TYPE_INVALID;

  if (name == NULL)
    return;

  for (i = 0; i < float_parameter_count; i++)
    if (strcmp(name, float_parameters[i].param.name) == 0)
      {
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
    int height_limit, width_limit;
    int papersizes = stp_known_papersizes();
    description->bounds.str = stp_string_list_create();

    width_limit = caps->max_width;
    height_limit = caps->max_height;

    for (i = 0; i < papersizes; i++) {
      const stp_papersize_t *pt = stp_get_papersize_by_index(i);
      if (strlen(pt->name) > 0 &&
	  pt->width <= width_limit && pt->height <= height_limit)
	{
	  if (stp_string_list_count(description->bounds.str) == 0)
	    description->deflt.str = pt->name;
	  stp_string_list_add_string(description->bounds.str,
				     pt->name, gettext(pt->text));
	}
    }
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    description->bounds.str= stp_string_list_create();
    description->deflt.str = NULL;
    for(i=0;i<caps->modelist->count;i++){
        stp_string_list_add_string(description->bounds.str,
                                        caps->modelist->modes[i].name, gettext(caps->modelist->modes[i].text));
        stp_deprintf(STP_DBG_CANON,"supports mode '%s'\n",
                      caps->modelist->modes[i].name);
        if(i == caps->modelist->default_mode)
            description->deflt.str=caps->modelist->modes[i].name;


    }
  }
  else if (strcmp(name, "InkType") == 0)
  {
    const canon_mode_t* mode = canon_get_current_mode(v);
    description->bounds.str= stp_string_list_create();
    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
      if(mode->ink_types & canon_inktypes[i].ink_type){
          stp_string_list_add_string(description->bounds.str,canon_inktypes[i].name,_(canon_inktypes[i].text));
      }
    }
    description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
  }
  else if (strcmp(name, "InkChannels") == 0)
    {
      unsigned int ink_type = canon_printhead_colors(v);
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
          if(ink_type == canon_inktypes[i].ink_type)
              description->deflt.integer = canon_inktypes[i].num_channels;
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

    for (i = 0; i < count; i ++)
      stp_string_list_add_string(description->bounds.str,
				canon_paper_list[i].name,
				gettext(canon_paper_list[i].text));
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    int count = sizeof(media_sources)/sizeof(media_sources[0]);
    description->bounds.str= stp_string_list_create();
    description->deflt.str= media_sources[0].name;
    for (i = 0; i < count; i ++)
      stp_string_list_add_string(description->bounds.str,
				media_sources[i].name,
				gettext(media_sources[i].text));
  }
  else if (strcmp(name, "PrintingMode") == 0)
  {
    const canon_mode_t* mode = canon_get_current_mode(v);
    description->bounds.str = stp_string_list_create();
    if (mode->ink_types != CANON_INK_K)
      stp_string_list_add_string
	(description->bounds.str, "Color", _("Color"));
    stp_string_list_add_string
      (description->bounds.str, "BW", _("Black and White"));
    description->deflt.str =
      stp_string_list_param(description->bounds.str, 0)->name;
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
}


/*
 * 'canon_imageable_area()' - Return the imageable area of the page.
 */

static void
internal_imageable_area(const stp_vars_t *v,   /* I */
			int  use_paper_margins,
			int  *left,	/* O - Left position in points */
			int  *right,	/* O - Right position in points */
			int  *bottom,	/* O - Bottom position in points */
			int  *top)	/* O - Top position in points */
{
  int	width, length;			/* Size of page */
  int left_margin = 0;
  int right_margin = 0;
  int bottom_margin = 0;
  int top_margin = 0;

  const canon_cap_t * caps= canon_get_model_capabilities(stp_get_model_id(v));
  const char *media_size = stp_get_string_parameter(v, "PageSize");
  const stp_papersize_t *pt = NULL;

  if (media_size && use_paper_margins)
    pt = stp_get_papersize_by_name(media_size);

  stp_default_media_size(v, &width, &length);
  if (pt)
    {
      left_margin = pt->left;
      right_margin = pt->right;
      bottom_margin = pt->bottom;
      top_margin = pt->top;
    }
  left_margin = MAX(left_margin, caps->border_left);
  right_margin = MAX(right_margin, caps->border_right);
  top_margin = MAX(top_margin, caps->border_top);
  bottom_margin = MAX(bottom_margin, caps->border_bottom);

  *left =	left_margin;
  *right =	width - right_margin;
  *top =	top_margin;
  *bottom =	length - bottom_margin;
}

static void
canon_imageable_area(const stp_vars_t *v,   /* I */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  internal_imageable_area(v, 1, left, right, bottom, top);
}

static void
canon_limit(const stp_vars_t *v,  		/* I */
	    int *width,
	    int *height,
	    int *min_width,
	    int *min_height)
{
  const canon_cap_t * caps=
    canon_get_model_capabilities(stp_get_model_id(v));
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

#define PUT(WHAT,VAL,RES) stp_deprintf(STP_DBG_CANON,"canon: "WHAT\
" is %04x =% 5d = %f\" = %f mm\n",(VAL),(VAL),(VAL)/(1.*RES),(VAL)/(RES/25.4))

#define ESC28 "\033\050"
#define ESC5b "\033\133"
#define ESC40 "\033\100"

/* ESC [K --  -- reset printer:
 */
static void
canon_init_resetPrinter(const stp_vars_t *v, canon_init_t *init)
{
  if ( init->caps->control_cmdlist ){
    int i=0;
    while(init->caps->control_cmdlist[i]){
      canon_cmd(v,ESC5b,0x4b, 2, 0x00,0x1f);
      stp_puts("BJLSTART\nControlMode=Common\n",v);
      stp_puts(init->caps->control_cmdlist[i],v);
      stp_putc('\n',v);
      stp_puts("BJLEND\n",v);
      ++i;
    }
  }
  canon_cmd(v,ESC5b,0x4b, 2, 0x00,0x0f);
}

/* ESC ($ -- 0x24 -- cmdSetDuplex --:
 */
static void
canon_init_setDuplex(const stp_vars_t *v, canon_init_t *init)
{
  if (!(init->caps->features & CANON_CAP_DUPLEX))
    return;
  if (strncmp(init->duplex_str, "Duplex", 6)) 
    return;
  /* The same command seems to be needed for both Duplex and DuplexTumble
     no idea about the meanings of the single bytes */
  canon_cmd(v,ESC28,0x24,9,0x01,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x02);
}

/* ESC (a -- 0x61 -- cmdSetPageMode --:
 */
static void
canon_init_setPageMode(const stp_vars_t *v, canon_init_t *init)
{
  if (!(init->caps->features & CANON_CAP_a))
    return;

  if (init->caps->features & CANON_CAP_a)
    canon_cmd(v,ESC28,0x61, 1, 0x01);
}

/* ESC (b -- 0x62 -- -- set data compression:
 */
static void
canon_init_setDataCompression(const stp_vars_t *v, canon_init_t *init)
{
  if (!(init->caps->features & CANON_CAP_b))
    return;

  canon_cmd(v,ESC28,0x62, 1, 0x01);
}

/* ESC (c -- 0x63 -- cmdSetColor --:
 */
static void
canon_init_setColor(const stp_vars_t *v, canon_init_t *init)
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
		if (init->ink_type == CANON_INK_K)
                            arg_63[0]|= 0x01;                                        /* PRINT_COLOUR */

                  arg_63[1] = ((init->pt ? init->pt->media_code_c : 0) << 4)                /* PRINT_MEDIA */
			+ 1;	/* hardcode to High quality for now */		/* PRINT_QUALITY */

                  canon_cmd(v,ESC28,0x63, 2, arg_63[0], arg_63[1]);
		break;

	case 2:			/* are any models using this? */
		break;

	case 3:			/* 720 dpi series - BJC-3000 and descendants */
		if (init->ink_type == CANON_INK_K)
                            arg_63[0]|= 0x01;                                        /* colour mode */

                  arg_63[1] = (init->pt) ? init->pt->media_code_c : 0;                /* print media type */

                 if (init->caps->model == 4202) /* S200 */
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
                         if (init->ink_type == CANON_INK_K)
                           arg_63[4] = 1;
                       }
                   }
                 else
                   arg_63[2] = 2;        /* hardcode to whatever this means for now; quality, apparently */

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
canon_init_setResolution(const stp_vars_t *v, canon_init_t *init)
{
  if (!(init->caps->features & CANON_CAP_d))
    return;

   if (init->caps->model != 4202 || (init->mode->xdpi <= 360))
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
canon_init_setPageMargins(const stp_vars_t *v, canon_init_t *init)
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
canon_init_setTray(const stp_vars_t *v, canon_init_t *init)
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

  if (init->pt) arg_6c_2= init->pt->media_code_l;
  canon_cmd(v,ESC28,0x6c, 2, arg_6c_1, arg_6c_2);
}

/* ESC (m -- 0x6d --  -- :
 */
static void
canon_init_setPrintMode(const stp_vars_t *v, canon_init_t *init)
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
  if ((init->caps->model==7000) && (init->ink_type == CANON_INK_K || init->ink_type == CANON_INK_CcMmYK || init->ink_type == CANON_INK_CcMmYyK))
    arg_6d_1= 0x03;

  if (((init->caps->model==8200 || init->caps->model==4202) && init->ink_type == CANON_INK_K) || init->ink_type == CANON_INK_CMYK)
      arg_6d_1= 0x02;

  if(init->caps->model == 4202 && init->ink_type == CANON_INK_CMY)
      arg_6d_1= 0x02;

  if (init->ink_type == CANON_INK_K)
    arg_6d_2= 0x02;

  if (init->caps->model==8200 || init->caps->model==4202)
    arg_6d_3= 0x01;

  canon_cmd(v,ESC28,0x6d,12, arg_6d_1,
	    0xff,0xff,0x00,0x00,0x07,0x00,
	    arg_6d_a,arg_6d_b,arg_6d_2,0x00,arg_6d_3);
}

/* ESC (p -- 0x70 -- cmdSetPageMargins2 --:
 */
static void
canon_init_setPageMargins2(const stp_vars_t *v, canon_init_t *init)
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
  if (!strcmp(init->source_str,"CD")) {
    canon_cmd(v,ESC28,0x70, 8, 0x00, 0x2a, 0xb0, 0x00,
		               0x00, 0x01, 0xe0, 0x00);
    stp_deprintf(STP_DBG_CANON,"sending cd margins\n");
  } else {
    canon_cmd(v,ESC28,0x70, 8,
   	      arg_70_1, arg_70_2, 0x00, 0x00,
	      arg_70_3, arg_70_4, 0x00, 0x00);
  }
}

/* ESC (q -- 0x71 -- setPageID -- :
 */
static void
canon_init_setPageID(const stp_vars_t *v, canon_init_t *init)
{
  if (!(init->caps->features & CANON_CAP_q))
    return;

  canon_cmd(v,ESC28,0x71, 1, 0x01);
}

/* ESC (r -- 0x72 --  -- :
 */
static void
canon_init_setX72(const stp_vars_t *v, canon_init_t *init)
{
  if ( !( (init->caps->features & CANON_CAP_r)
         || (init->caps->features & CANON_CAP_rr) ) )
    return;

  if ( (init->caps->features & CANON_CAP_r)
       || (init->caps->features & CANON_CAP_rr) )
      canon_cmd(v,ESC28,0x72, 1, 0x61); /* whatever for - 8200/S200 need it */
  if (init->caps->features & CANON_CAP_rr)
      canon_cmd(v,ESC28,0x72, 3, 0x63, 1, 0); /* whatever for - S200 needs it */
      /* probably to set the print direction of the head */
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
canon_init_setImage(const stp_vars_t *v, canon_init_t *init)
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
    buf[1]=0x80;
    buf[2]=0x01;
    for(i=0;i<init->mode->num_inks;i++){
        if(init->mode->inks[i].ink){
          if(init->mode->inks[i].ink->flags & INK_FLAG_5pixel_in_1byte)
            buf[3+i*3+0]=(1<<5)|init->mode->inks[i].ink->bits; /*info*/
          else
            buf[3+i*3+0]=init->mode->inks[i].ink->bits;
          buf[3+i*3+2]= init->mode->inks[i].ink->numsizes+1;/*level*/
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
  if (init->caps->model==4202) /* 1 bit per pixel (arg 4,7,10,13); */
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
    arg_74_3= 0x09;
    if (init->ink_type == CANON_INK_CMY) arg_74_3= 0x02; /* for BC-06 cartridge!!! */
  }

  /* workaround for the bjc8200 in 6color mode - not really understood */
  if (init->caps->model==8200) {
    if (init->ink_type == CANON_INK_CcMmYK) {
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

static void
canon_init_printer(const stp_vars_t *v, canon_init_t *init)
{
  int mytop;
  /* init printer */
  if (init->is_first_page) {
    canon_init_resetPrinter(v,init);       /* ESC [K */
    canon_init_setDuplex(v,init);          /* ESC ($ */
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
  canon_init_setTray(v,init);            /* ESC (l */
  canon_init_setX72(v,init);             /* ESC (r */
  /* some linefeeds */

  mytop= (init->top*init->mode->ydpi)/72;
  canon_cmd(v,ESC28,0x65, 2, (mytop >> 8 ),(mytop & 255));
}

static void
canon_deinit_printer(const stp_vars_t *v, canon_init_t *init)
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
  return 1;
}

static int
canon_end_job(const stp_vars_t *v, stp_image_t *image)
{
  canon_cmd(v,ESC40,0,0);
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
  for (i = 0; i < 7; i++)
    canon_advance_buffer(pd->cols[i], pd->length /*buf_length[i]*/, pd->delay[i]);

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
set_ink_ranges(stp_vars_t *v, const stp_shade_t* shades, int num_shades, int color,unsigned int used_inks,
	       const char *channel_param, const char *subchannel_param)
{
  if (!shades)
    return;

  stp_dither_set_inks_full(v, color, num_shades, shades, 1.0,
			   ink_darknesses[color]);

  stp_channel_set_density_adjustment
    (v, color, 1, (get_double_param(v, channel_param) *
		   get_double_param(v, subchannel_param) *
		   get_double_param(v, "Density")));
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


static void canon_add_ink(canon_privdata_t* privdata,const canon_inkset_t* ink,unsigned int used_inks,int length){
  static const char canon_channel_map[] = "KCMYcmyk";
  static const unsigned int canon_ink_mask[] = {CANON_INK_K_MASK,CANON_INK_CMY_MASK,CANON_INK_CMY_MASK,CANON_INK_CMY_MASK,CANON_INK_CcMmYyKk_MASK,CANON_INK_CcMmYyKk_MASK,CANON_INK_CcMmYyKk_MASK,CANON_INK_CcMmYyKk_MASK};
  int i;
  if(ink->channel && ink->density){
    for(i=0;i<strlen(canon_channel_map);i++){
      if(canon_channel_map[i] == ink->channel && (used_inks & canon_ink_mask[i])){
        int channel = channel_color_map[i];
        int subchannel = privdata->num_shades[channel];
        ++privdata->num_shades[channel];
        privdata->buf_length[i] = ((length *ink->ink->bits)+1)*(privdata->delay[i] + 1);
        privdata->bits[i] = ink->ink->bits;
        privdata->ink_flags[i] = ink->ink->flags;
        privdata->cols[i]=stp_zalloc(privdata->buf_length[i]+1);
        privdata->shades[channel] = stp_realloc(privdata->shades[channel],(subchannel+1)*sizeof(stp_shade_t));
        privdata->shades[channel][subchannel].value = ink->density;
        privdata->shades[channel][subchannel].numsizes = ink->ink->numsizes;
        privdata->shades[channel][subchannel].dot_sizes = ink->ink->dot_sizes;
      }
    }
  }
}



#define CD_X_OFFSET 0
#define CD_Y_OFFSET 230
#define CD_OUTER_RADIUS (329/2)
#define CD_INNER_RADIUS 56


/*
 * 'canon_print()' - Print an image to a CANON printer.
 */
static int
canon_do_print(stp_vars_t *v, stp_image_t *image)
{
  int i;
  int		status = 1;
  int		model = stp_get_model_id(v);
  const char	*media_source = stp_get_string_parameter(v, "InputSlot");
  const char    *duplex_mode =stp_get_string_parameter(v, "Duplex");
  int           page_number = stp_get_int_parameter(v, "PageNumber");
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  canon_init_t  init;
  const canon_cap_t * caps= canon_get_model_capabilities(model);
  const canon_mode_t* mode=NULL;
  const canon_paper_t *pt;
  int		n;		/* Output number */
  int		y;		/* Looping vars */
  canon_privdata_t privdata;
  int		page_width,	/* Width of page */
		page_height,	/* Length of page */
		page_left,
		page_top,
		page_right,
		page_bottom,
		page_true_height,	/* True length of page */
		length,		/* Length of raster data */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast,	/* Last raster line loaded */
		out_width,	/* Width of image on page */
		out_height,	/* Length of image on page */
		out_channels;	/* Output bytes per pixel */
  unsigned	zero_mask;
  int           print_cd= (media_source && (!strcmp(media_source, "CD")));
  int           image_height,
                image_width;
  double        k_upper, k_lower;
  unsigned char *cd_mask = NULL;
  double outer_r_sq = 0;
  double inner_r_sq = 0;

  if (!stp_verify(v))
    {
      stp_eprintf(v, "Print options not verified; cannot print.\n");
      return 0;
    }
  /*
  * Setup a read-only pixel region for the entire image...
  */

  stp_image_init(image);


  /* find the wanted print mode */
  mode = canon_get_current_mode(v);



  /* force grayscale if image is grayscale
   *                 or single black cartridge installed
   */
  init.ink_type = canon_printhead_colors(v);
  if (init.ink_type == CANON_INK_K)
      stp_set_string_parameter(v, "PrintingMode", "BW");

 /*
  * Compute the output size...
  */

  out_width = stp_get_width(v);
  out_height = stp_get_height(v);

  internal_imageable_area(v, 0, &page_left, &page_right,
			  &page_bottom, &page_top);
  if (print_cd) {
    left += CD_X_OFFSET; 
    top += CD_Y_OFFSET;
    /*page_width = CD_OUTER_RADIUS*2;
      page_height = CD_OUTER_RADIUS*2; */
    stp_default_media_size(v, &page_width, &page_height); 
    out_width = page_width;
    out_height = page_height;
  } else {
    left -= page_left;
    top -= page_top;
    page_width = page_right - page_left;
    page_height = page_bottom - page_top;
  }
  image_height = stp_image_height(image);
  image_width = stp_image_width(image);

  stp_default_media_size(v, &n, &page_true_height);
  
  PUT("top        ",top,72);
  PUT("left       ",left,72);
  PUT("page_true_height",page_true_height,72);
  PUT("out_width ", out_width,mode->xdpi);
  PUT("out_height", out_height,mode->ydpi);

  PUT("top     ",top,72);
  PUT("left    ",left,72);

  pt = get_media_type(caps,stp_get_string_parameter(v, "MediaType"));

  init.caps = caps;
  init.pt = pt;
  init.mode = mode;
  init.source_str = media_source;
  init.duplex_str = duplex_mode;
  init.is_first_page = (page_number == 0);
  init.page_width = page_width;
  init.page_height = page_height;
  init.top = top;
  init.left = left;

  canon_init_printer(v, &init);

 /*
  * Convert image size to printer resolution...
  */

  out_width  = mode->xdpi * out_width / 72;
  out_height = mode->ydpi * out_height / 72;

  PUT("out_width ", out_width,mode->xdpi);
  PUT("out_height", out_height,mode->ydpi);

  left = mode->xdpi * left / 72;

  PUT("leftskip",left,mode->xdpi);

  memset(&privdata,0,sizeof(canon_privdata_t));

  if((mode->xdpi == 1440) && (model != 4202)){
    privdata.delay[0] = 0;
    privdata.delay[1] = 112;
    privdata.delay[2] = 224;
    privdata.delay[3] = 336;
    privdata.delay[4] = 112;
    privdata.delay[5] = 224;
    privdata.delay[6] = 336;
    privdata.delay_max = 336;
    stp_deprintf(STP_DBG_CANON,"canon: delay on!\n");
  } else if (model ==4202 ){
    privdata.delay[0]= 0;
    privdata.delay[1]= 0x30;
    privdata.delay[2]= 0x50;
    privdata.delay[3]= 0x70;
    privdata.delay[4]= 0;
    privdata.delay[5]= 0;
    privdata.delay[6]= 0;
    privdata.delay_max= 0x70;
    stp_deprintf(STP_DBG_CANON,"canon: delay for S200 on!\n");
  } else {
    for (i = 0; i < 7; i++)
      privdata.delay[i] = 0;
    privdata.delay_max = 0;
    stp_deprintf(STP_DBG_CANON,"canon: delay off!\n");
  }

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;


  for(i=0;i<mode->num_inks;i++){
      canon_add_ink(&privdata,&mode->inks[i],init.ink_type,length);
  }

  privdata.length = length;
  privdata.left = left;
  privdata.out_width = out_width;
  privdata.caps = caps;
  privdata.ydpi = mode->ydpi;


  if(init.ink_type & CANON_INK_CMYK_MASK)
    stp_set_string_parameter(v, "STPIOutputType", "KCMY");
  else if(init.ink_type & CANON_INK_CMY_MASK)
    stp_set_string_parameter(v, "STPIOutputType", "CMY");
  else
    stp_set_string_parameter(v, "STPIOutputType", "Grayscale");

  stp_deprintf(STP_DBG_CANON,
	       "canon: driver will use colors %s%s%s%s%s%s%s\n",
	       privdata.cols[0] ? "K" : "",
	       privdata.cols[1] ? "C" : "",
	       privdata.cols[2] ? "M" : "",
	       privdata.cols[3] ? "Y" : "",
	       privdata.cols[4] ? "c" : "",
	       privdata.cols[5] ? "m" : "",
	       privdata.cols[6] ? "y" : "");

  stp_deprintf(STP_DBG_CANON,"density is %f\n",
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
  if (pt)
    stp_scale_float_parameter(v, "Density", pt->base_density);
  else			/* Can't find paper type? Assume plain */
    stp_scale_float_parameter(v, "Density", .5);
  stp_scale_float_parameter(v, "Density",mode->density);
  if (stp_get_float_parameter(v, "Density") > 1.0)
    stp_set_float_parameter(v, "Density", 1.0);

  if (init.ink_type == CANON_INK_K)
    stp_scale_float_parameter(v, "Gamma", 1.25);

  stp_deprintf(STP_DBG_CANON,"density is %f\n",
	       stp_get_float_parameter(v, "Density"));

 /*
  * Output the page...
  */

  if (privdata.cols[5]) /* use_6colors */
    k_lower = .4 / privdata.bits[5] + .1; /* c bits */
  else
    k_lower = .25 / privdata.bits[0] ; /* K bits */
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
  if (!stp_check_float_parameter(v, "GCRLower", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRLower", k_lower);
  if (!stp_check_float_parameter(v, "GCRUpper", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRUpper", k_upper);
  stp_dither_init(v, image, out_width, mode->xdpi, mode->ydpi);

  for (i = 0; i < 7; i++)
    {
      if (privdata.cols[i])
	{
	  stp_dither_add_channel(v, privdata.cols[i], channel_color_map[i],
				 subchannel_color_map[i]);
	  if (channel_color_map[i] == STP_ECOLOR_K)
	    stp_channel_set_black_channel(v, STP_ECOLOR_K);
	}
    }

  set_ink_ranges(v, privdata.shades[STP_ECOLOR_C],privdata.num_shades[STP_ECOLOR_C], STP_ECOLOR_C,init.ink_type, "CyanDensity",
		     "LightCyanTransition");
  set_ink_ranges(v, privdata.shades[STP_ECOLOR_M],privdata.num_shades[STP_ECOLOR_M], STP_ECOLOR_M,init.ink_type, "MagentaDensity",
		     "LightMagentaTransition");
  set_ink_ranges(v, privdata.shades[STP_ECOLOR_Y],privdata.num_shades[STP_ECOLOR_Y], STP_ECOLOR_Y,init.ink_type, "YellowDensity",
		     "LightYellowTransition");
  set_ink_ranges(v, privdata.shades[STP_ECOLOR_K],privdata.num_shades[STP_ECOLOR_K], STP_ECOLOR_K,init.ink_type, "BlackDensity", NULL);

  stp_channel_set_density_adjustment
    (v, STP_ECOLOR_C, 0,
     get_double_param(v, "CyanDensity") * get_double_param(v, "Density"));
  stp_channel_set_density_adjustment
    (v, STP_ECOLOR_M, 0,
     get_double_param(v, "MagentaDensity") * get_double_param(v, "Density"));
  stp_channel_set_density_adjustment
    (v, STP_ECOLOR_Y, 0,
     get_double_param(v, "YellowDensity") * get_double_param(v, "Density"));
  stp_channel_set_density_adjustment
    (v, STP_ECOLOR_K, 0,
     get_double_param(v, "BlackDensity") * get_double_param(v, "Density"));

  /* initialize weaving for S200 for resolutions > 360dpi */
  if (mode->flags & MODE_FLAG_WEAVE)
     {
       privdata.stepper_ydpi = 720;
       privdata.nozzle_ydpi = 360;
       if (mode->xdpi == 2880)
         privdata.physical_xdpi = 2880;
       else
         privdata.physical_xdpi = 720;

       stp_deprintf(STP_DBG_CANON,"canon: adjust leftskip: old=%d,\n", privdata.left);
       privdata.left = (int)( (float)privdata.left * (float)privdata.physical_xdpi / (float)mode->xdpi ); /* adjust left margin */
       stp_deprintf(STP_DBG_CANON,"canon: adjust leftskip: new=%d,\n", privdata.left);

       privdata.ncolors = 4;
       privdata.head_offset = stp_zalloc(sizeof(int) * privdata.ncolors);
       memset(privdata.head_offset, 0, sizeof(privdata.head_offset));

       if ( init.ink_type == CANON_INK_K )
           privdata.nozzles = 64; /* black nozzles */
       else
           privdata.nozzles = 24; /* color nozzles */
       if ( init.ink_type == CANON_INK_K )
         {
           privdata.ncolors = 1;
           privdata.head_offset[0] = 0; /* K starts at 0 */
           privdata.head_offset[1] = 0 ;/* how far C starts after K */
           privdata.head_offset[2] = 0;/* how far M starts after K */
           privdata.head_offset[3] = 0;/* how far Y starts after K */
           top += 11;
         }
       else if ( init.ink_type == CANON_INK_CMYK )
         {
           privdata.head_offset[0] = 0; /* K starts at 0 */
           privdata.head_offset[1] = 144 ;/* how far C starts after K */
           privdata.head_offset[2] = 144 + 64;/* how far M starts after K */
           privdata.head_offset[3] = 144 + 64 + 64;/* how far Y starts after K */
           top += 5;
         }
       else  /* colormode == CMY */
         {
           privdata.head_offset[0] = 0; /* K starts at 0 */
           privdata.head_offset[1] = 0 ;/* how far C starts after K */
           privdata.head_offset[2] = 64;/* how far M starts after K */
           privdata.head_offset[3] = 128;/* how far Y starts after K */
           top += 18;
         }

       privdata.nozzle_separation = privdata.stepper_ydpi / privdata.nozzle_ydpi;
       privdata.horizontal_passes = mode->xdpi / privdata.physical_xdpi;
       privdata.vertical_passes = 1;
       privdata.vertical_oversample = privdata.ydpi / privdata.stepper_ydpi;
       privdata.bidirectional = 1; /* 1: bidirectional; 0: unidirectional  printing */
       privdata.direction = 1;
       stp_allocate_component_data(v, "Driver", NULL, NULL, &privdata);
       stp_deprintf(STP_DBG_CANON,"canon: initializing weaving: nozzles=%d, nozzle_separation=%d,\n"
                                    "horizontal_passes=%d, vertical_passes=%d,vertical_oversample=%d,\n"
                                    "ncolors=%d, out_width=%d, out_height=%d\n"
                                    "weave_top=%d, weave_page_height=%d \n"
                                    "head_offset=[%d,%d,%d,%d]  \n",
                                    privdata.nozzles, privdata.nozzle_separation,
                                    privdata.horizontal_passes, privdata.vertical_passes,
                                    privdata.vertical_oversample, privdata.ncolors,
                                    out_width, out_height,
                                    top * privdata.stepper_ydpi / 72, page_height * privdata.stepper_ydpi / 72,
                                    privdata.head_offset[0],privdata.head_offset[1],
                                    privdata.head_offset[2],privdata.head_offset[3]);

       stp_initialize_weave(v, privdata.nozzles, privdata.nozzle_separation,
                                privdata.horizontal_passes, privdata.vertical_passes,
                                privdata.vertical_oversample, privdata.ncolors,
                                1,
                                out_width, out_height,
                                top * privdata.stepper_ydpi / 72,
                                page_height * privdata.stepper_ydpi / 72,
                                privdata.head_offset,
                                STP_WEAVE_ZIGZAG,
                                canon_flush_pass,
                                stp_fill_uncompressed,
                                stp_pack_uncompressed,
                                stp_compute_uncompressed_linewidth);
       privdata.last_pass_offset = 0;
  }


  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;
 
  if (!stp_check_curve_parameter(v, "HueMap", STP_PARAMETER_ACTIVE)) 
    {
      stp_curve_t* hue_adjustment = stp_read_and_compose_curves
	(caps->hue_adjustment,pt->hue_adjustment,
	 STP_CURVE_COMPOSE_ADD, 384);
      if(hue_adjustment){
        stp_set_curve_parameter(v, "HueMap", hue_adjustment);
        stp_curve_destroy(hue_adjustment);
      }
    }
  if (!stp_check_curve_parameter(v, "LumMap", STP_PARAMETER_ACTIVE)) 
    {
      stp_curve_t* lum_adjustment = stp_read_and_compose_curves
	(caps->lum_adjustment,pt->lum_adjustment,
	 STP_CURVE_COMPOSE_MULTIPLY, 384);
      if(lum_adjustment){
        stp_set_curve_parameter(v, "LumMap", lum_adjustment);
        stp_curve_destroy(lum_adjustment);
      }
    }
  if (!stp_check_curve_parameter(v, "SatMap", STP_PARAMETER_ACTIVE)) 
    {
      stp_curve_t* sat_adjustment = stp_read_and_compose_curves
	(caps->sat_adjustment,pt->sat_adjustment,
	 STP_CURVE_COMPOSE_MULTIPLY, 384);
      if(sat_adjustment){
        stp_set_curve_parameter(v, "SatMap", sat_adjustment);
        stp_curve_destroy(sat_adjustment);
      }
    }

  out_channels = stp_color_init(v, image, 65536);
  stp_allocate_component_data(v, "Driver", NULL, NULL, &privdata);

  privdata.emptylines = 0;
  if (print_cd) {
    cd_mask = stp_malloc(1 + (out_width + 7) / 8);
    outer_r_sq = (double)CD_OUTER_RADIUS * (double)CD_OUTER_RADIUS;
    inner_r_sq = (double)CD_INNER_RADIUS * (double)CD_INNER_RADIUS;
  }
  for (y = 0; y < out_height; y ++)
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
	int x_center = CD_OUTER_RADIUS * mode->xdpi / 72;
	int y_distance_from_center =
	  CD_OUTER_RADIUS - (y * 72 / mode->ydpi);
	if (y_distance_from_center < 0)
	  y_distance_from_center = -y_distance_from_center;
	memset(cd_mask, 0, (out_width + 7) / 8);
	if (y_distance_from_center < CD_OUTER_RADIUS)
	  {
	    double y_sq = (double) y_distance_from_center *
	      (double) y_distance_from_center;
	    int x_where = sqrt(outer_r_sq - y_sq) + .5;
	    int scaled_x_where = x_where * mode->xdpi / 72;
	    set_mask(cd_mask, x_center, scaled_x_where,
		     out_width, 1, 0);
	    if (y_distance_from_center < CD_INNER_RADIUS)
	      {
		x_where = sqrt(inner_r_sq - y_sq) + .5;
		scaled_x_where = x_where * mode->ydpi / 72;
		set_mask(cd_mask, x_center, scaled_x_where,
			 out_width, 1, 1);
	      }
	  }
      }
    stp_dither(v, y, duplicate_line, zero_mask, cd_mask);
    if ( mode->flags & MODE_FLAG_WEAVE )
        stp_write_weave(v, privdata.cols);
    else
        canon_printfunc(v);
    errval += errmod;
    errline += errdiv;
    if (errval >= out_height)
    {
      errval -= out_height;
      errline ++;
    }
  }

  if ( mode->flags & MODE_FLAG_WEAVE )
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
    stp_deprintf(STP_DBG_CANON,"\ncanon: flushing %d possibly delayed buffers\n",
		 privdata.delay_max);
    for (y= 0; y<privdata.delay_max; y++) {

      canon_write_line(v);
      for (i = 0; i < 7; i++)
	canon_advance_buffer(privdata.cols[i], privdata.length,
			     privdata.delay[i]);
    }
  }
  }
  stp_image_conclude(image);

 /*
  * Cleanup...
  */

  for (i = 0; i < 7; i++)
    if (privdata.cols[i])
      stp_free(privdata.cols[i]);

  if(cd_mask)
      stp_free(cd_mask);

  for(i=0;i<STP_NCOLORS;i++){
    if(privdata.shades[i])
      stp_free(privdata.shades[i]);
  }


  canon_deinit_printer(v, &init);
  return status;
}

static int
canon_print(const stp_vars_t *v, stp_image_t *image)
{
  int status;
  stp_vars_t *nv = stp_vars_create_copy(v);
  stp_prune_inactive_options(nv);
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
  canon_imageable_area,
  canon_limit,
  canon_print,
  canon_describe_resolution,
  canon_describe_output,
  stp_verify_printer_params,
  canon_start_job,
  canon_end_job
};

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

/*
 * 'canon_write()' - Send graphics using TIFF packbits compression.
 */

static int
canon_write(stp_vars_t *v,		/* I - Print file or command */
	    const canon_cap_t *   caps,	        /* I - Printer model */
	    unsigned char *line,	/* I - Output bitmap data */
	    int           length,	/* I - Length of bitmap data */
	    int           coloridx,	/* I - Which color */
	    int           ydpi,		/* I - Vertical resolution */
	    int           *empty,       /* IO- Preceeding empty lines */
	    int           width,	/* I - Printed width */
	    int           offset, 	/* I - Offset from left side */
	    int           bits,
            int           ink_flags)
{
  unsigned char
    comp_buf[COMPBUFWIDTH + COMPBUFWIDTH / 4],	/* Compression buffer */
    in_fold[COMPBUFWIDTH],
    *in_ptr= line,
    *comp_ptr, *comp_data;
  int newlength;
  int offset2,bitoffset;
  unsigned char color;

  /* Don't send blank lines... */

  if (line[0] == 0 && memcmp(line, line + 1, length - 1) == 0)
    return 0;

  offset2 = offset / 8;
  bitoffset = offset % 8;

  /* fold lsb/msb pairs if drop modulation is active */


  if (bits==2) {
    int pixels_per_byte = 4;
    if(ink_flags & INK_FLAG_5pixel_in_1byte)
      pixels_per_byte = 5;

    stp_fold(line,length,in_fold);
    in_ptr= in_fold;
    length= (length*8/4); /* 4 pixels in 8bit */
    /* calculate the number of compressed bytes that can be sent directly */
    offset2 = offset / pixels_per_byte;
    /* calculate the number of (uncompressed) bits that have to be added to the raster data */
    bitoffset = (offset % pixels_per_byte) * 2;
  }
  if (bits==3) {
    memset(in_fold,0,length);
    canon_fold_3bit(line,length,in_fold);
    in_ptr= in_fold;
    length= (length*8)/3;
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
  /* pack left border rounded to multiples of 8 dots */

  comp_data= comp_buf;
  while (offset2>0) {
    unsigned char toffset = offset2 > 128 ? 128 : offset2;
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
      stp_deprintf(STP_DBG_CANON,"SEVERE BUG IN print-canon.c::canon_write() "
	      "bitoffset=%d!!\n",bitoffset);
  }

    if(ink_flags & INK_FLAG_5pixel_in_1byte)
       length = pack_pixels(in_ptr,length);

  stp_pack_tiff(v, in_ptr, length, comp_data, &comp_ptr, NULL, NULL);
  newlength= comp_ptr - comp_buf;

  /* send packed empty lines if any */

  if (*empty) {
    stp_zfwrite("\033\050\145\002\000", 5, 1, v);
    stp_put16_be(*empty, v);
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
canon_write_line(stp_vars_t *v)
{
  canon_privdata_t *pd =
    (canon_privdata_t *) stp_get_component_data(v, "Driver");
  static const int write_sequence[] = { 0, 3, 2, 1, 6, 5, 4 }; /* KYMCymc */
  static const int write_number[] = { 3, 2, 1, 0, 6, 5, 4 };   /* KYMCymc */
  int i;
  int written= 0;
  for (i = 0; i < 7; i++)
    {
      int col = write_sequence[i];
      int num = write_number[i];
      int bits=pd->bits[col];
      if (pd->cols[col])
	written += canon_write(v, pd->caps,
			       pd->cols[col] + pd->delay[col] * pd->length /*buf_length[i]*/,
			       pd->length, num, pd->ydpi,
			       &(pd->emptylines), pd->out_width,
			       pd->left, bits, pd->ink_flags[col]);
    }
  if (written)
    stp_zfwrite("\033\050\145\002\000\000\001", 7, 1, v);
  else
    pd->emptylines += 1;
}

static void
canon_advance_paper(stp_vars_t *v, int advance)
{
  if ( advance > 0 )
    {
      int a0, a1, a2, a3;
      stp_deprintf(STP_DBG_CANON,"                      --advance paper %d\n", advance);
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

  stp_deprintf(STP_DBG_CANON,"canon_flush_pass: ----pass=%d,---- \n", passno);
  (pd->emptylines) = 0;

  for ( color = 0; color < pd->ncolors; color++ ) /* find max. linecount */
    {
      if ( linecount[0].v[color] > lines )
        lines = linecount[0].v[color];
    }

  for ( line = 0; line < lines; line++ )  /* go through each nozzle f that pass */
    {
      stp_deprintf(STP_DBG_CANON,"                      --line=%d\n", line);

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
/*                stp_deprintf(STP_DBG_CANON,"canon_flush_pass: linelength=%d, bufs[0].v[color]=%p,"
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
                         stp_deprintf(STP_DBG_CANON,"                      --set direction %d\n", pd->direction);
                        }
                    }

                  written += canon_write(v, pd->caps,
                               (unsigned char *)(bufs[0].v[color] + line * linelength),
                               linelength, idx[color], pd->ydpi,
                               &(pd->emptylines), pd->out_width,
                               pd->left, pd->bits[color],0);
                  if (written) stp_deprintf(STP_DBG_CANON,"                        --written color %d,\n", color);

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
  stp_deprintf(STP_DBG_CANON,"                  --ended-- with empty=%d \n", (pd->emptylines));
}

static stp_family_t print_canon_module_data =
  {
    &print_canon_printfuncs,
    NULL
  };


static int
print_canon_module_init(void)
{
  return stp_family_register(print_canon_module_data.printer_list);
}


static int
print_canon_module_exit(void)
{
  return stp_family_unregister(print_canon_module_data.printer_list);
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

