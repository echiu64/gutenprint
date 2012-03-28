/*
 * "$Id$"
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

#include "print-canon.h"

#ifndef MIN
#  define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif /* !MIN */
#ifndef MAX
#  define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* !MAX */


/* set this to 1 to see errors in make, set to 0 (normal) to avoid noise */
#define ERRPRINT 0



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
#define CANON_CAP_px        0x2000ul
#define CANON_CAP_rr        0x4000ul
#define CANON_CAP_I         0x8000ul
#define CANON_CAP_T         0x10000ul /* not sure of this yet! */
#define CANON_CAP_P         0x20000ul
#define CANON_CAP_DUPLEX    0x40000ul
#define CANON_CAP_XML       0x80000ul /* not sure of this yet */
#define CANON_CAP_CARTRIDGE 0x100000ul /* not sure of this yet */
#define CANON_CAP_M         0x200000ul /* not sure of this yet */
#define CANON_CAP_S         0x400000ul /* not sure of this yet */

#define CANON_CAP_STD0 (CANON_CAP_b|CANON_CAP_c|CANON_CAP_d|\
                        CANON_CAP_l|CANON_CAP_q|CANON_CAP_t)

#define CANON_CAP_STD1 (CANON_CAP_b|CANON_CAP_c|CANON_CAP_d|CANON_CAP_l|\
                        CANON_CAP_m|CANON_CAP_p|CANON_CAP_q|CANON_CAP_t)


#include "canon-inks.h"
#include "canon-modes.h"
#include "canon-media.h"
#include "canon-printers.h"
/* Gernot: cross-reference between media and modes */
#include "canon-media-mode.h"

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
  /* cartridge selection for CANON_CAP_T */
  const char *ink_set;
  /* Gernot: cross-reference between media and modes */
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

static void canon_write_line(stp_vars_t *v);

static void canon_advance_paper(stp_vars_t *, int);
static void canon_flush_pass(stp_vars_t *, int, int);
static void canon_write_multiraster(stp_vars_t *v,canon_privdata_t* pd,int y);

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
  {
    "InkType", N_("Ink Type"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "InkChannels", N_("Ink Channels"), "Color=Yes,Category=Advanced Printer Functionality",
    N_("Ink Channels"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 0, 0, STP_CHANNEL_NONE, 0, 0
  },
  {
    "PrintingMode", N_("Printing Mode"), "Color=Yes,Category=Core Parameter",
    N_("Printing Output Mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
#if 1
  {
    "InkSet", N_("Ink Set"), "Color=Yes,Category=Basic Printer Setup",
    N_("Type of inkset in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
#endif
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
 "S",
 "i",
 "PIXMA iP",
 "PIXMA iX",
 "PIXMA MP",
 "PIXUS",
 "PIXMA Pro",
 "PIXMA MG",
 "PIXMA MX",
 "SELPHY DS",
 "PIXMA mini",
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
    stp_erprintf("canon_get_printername: no family %i using default BJC\n", family);
    family = 0;
  }
  len = strlen(canon_families[family]) + 7; /* max model nr. + terminating 0 */
  name = stp_zalloc(len);
  snprintf(name,len,"%s%u",canon_families[family],nr);
  if (ERRPRINT)
    stp_erprintf("canon_get_printername: current printer name: %s\n", name);
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
  stp_erprintf("canon: model %s not found in capabilities list=> using default\n",name);
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
/* if no mode is set the NULL will be returned */
static const canon_mode_t* canon_get_current_mode(const stp_vars_t *v){
#if 0
    const char* input_slot = stp_get_string_parameter(v, "InputSlot");
    const char *quality = stp_get_string_parameter(v, "Quality");
#endif
    const char *resolution = stp_get_string_parameter(v, "Resolution");
    const char *ink_set = stp_get_string_parameter(v, "InkSet");
    const char *ink_type = stp_get_string_parameter(v, "InkType");
    const canon_cap_t * caps = canon_get_model_capabilities(v);
    const canon_mode_t* mode = NULL;
    const canon_modeuselist_t* mlist = &canon_MULTIPASS_MP610_modeuselist;
    const canon_modeuse_t* muse = NULL;
    const canon_paper_t* media_type = get_media_type(caps,stp_get_string_parameter(v, "MediaType"));
    int i,j;
    int modecheck, quality, modefound;
    char* replaceres;

    if(resolution){
      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  get_current_mode --- Resolution already known: '%s'\n",resolution);
      if (ERRPRINT)
	stp_erprintf("DEBUG: Gutenprint:  get_current_mode --- Resolution already known: '%s'\n",resolution);
      for(i=0;i<caps->modelist->count;i++){
	if(!strcmp(resolution,caps->modelist->modes[i].name)){
	  mode = &caps->modelist->modes[i];
	  break;
	}
      }
    }
    else {
      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  get_current_mode --- Resolution not yet known \n");
      if (ERRPRINT)
	stp_erprintf("DEBUG: Gutenprint:  get_current_mode --- Resolution not yet known \n");
    }
    
    /* beginning of mode replacement code */
    if (media_type && resolution && mode) {
      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  get_current_mode --- Resolution, Media, Mode all known \n");
      if (ERRPRINT)
	stp_erprintf("DEBUG: Gutenprint:  get_current_mode --- Resolution, Media, Mode all known \n");
      
      if ( (!strcmp(caps->name,"PIXMA MP610")) ) {
	
	stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: media type selected: '%s'\n",media_type->name);
	if (ERRPRINT)
	  stp_erprintf("DEBUG: Gutenprint: media type selected: '%s'\n",media_type->name);
	
	/* scroll through modeuse list to find media */
	for(i=0;i<mlist->count;i++){
	  if(!strcmp(media_type->name,mlist->modeuses[i].name)){
	    muse = &mlist->modeuses[i];
	    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: mode searching: assigned media '%s'\n",muse->name);
	    if (ERRPRINT)
	      stp_erprintf("DEBUG: Gutenprint: mode searching: assigned media '%s'\n",muse->name);
	    break;
	  }
	}
	/* now scroll through to find if the mode is in the modeuses list */
	i=0;
	modecheck=1;
	while (muse->mode_name_list[i]!=NULL){
	  /* need to check for duplex in case it is a replacement for another mode */
	  if(!strcmp(mode->name,muse->mode_name_list[i])){
	    modecheck=0;
	    break;
	  }
	  i++;
	}
	stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: modecheck value: '%i'\n",modecheck);
	if (ERRPRINT)
	  stp_erprintf("DEBUG: Gutenprint: modecheck value: '%i'\n",modecheck);

	if (ink_set)
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: InkSet value: '%s'\n",ink_set);
	else
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: InkSet value is NULL\n");

	if (ink_type)
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: InkType value: '%s'\n",ink_type);
	else
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: InkType value is NULL\n");

	/* if we did not find a mode*/
	if (modecheck!=0) {

	  /* pick first mode name for now, for that media */
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: first item in the name list: '%s'\n",muse->mode_name_list[0]);
	  if (ERRPRINT)
	    stp_erprintf("DEBUG: Gutenprint: first item in the name list: '%s'\n",muse->mode_name_list[0]);
	  
	  /* TODO: finally want to choose new mode based on InkSet, InkType, Duplex setting, quality of original mode */

	  /* old, simple find mode code */
	  replaceres = muse->mode_name_list[0];

	  /* start new find mode code */
#if 0
	  quality = mode->quality;
	  if (ink_set && !strcmp(ink_set,"Black")) {
	    if (!(mode->ink_types & CANON_INK_K)) {
	      /* need a new mode: 
		 loop through modes in muse list searching for a matching inktype, comparing quality
		 quality is major factor
	       */
	      i=0;
	      modefound=0;
	      while ((muse->mode_name_list[i]!=NULL) && (modefound != 1)){
		/* need to check for duplex in case it is a replacement for another mode */
		for(j=0;j<caps->modelist->count;j++){
		  if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		    if (caps->modelist->modes[j].quality >= quality){ /* keep setting the mode until lowest matching quality is found */
		      mode = &caps->modelist->modes[j];
		      modefound=1;
		    }
		    break; /* go to next mode in muse list */
		  }
		}
		i++;
	      }
	      if (modefound == 0) { /* still did not find a mode: pick first one for that media */
		replaceres = muse->mode_name_list[0];
		for(i=0;i<caps->modelist->count;i++){
		  if(!strcmp(replaceres,caps->modelist->modes[i].name)){
		    mode = &caps->modelist->modes[i];
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: setting mode finally to be: '%s'\n",mode->name);
		    if (ERRPRINT)
		      stp_erprintf("DEBUG: Gutenprint: setting mode finally to be: '%s'\n",mode->name);
		    break;
		  }
		}
	      }
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
	      if (strcmp(ink_type,"Gray")) /* if it it not set to Gray already */
		stp_set_string_parameter(v, "InkType", "Gray");
	      ink_type = stp_get_string_parameter(v, "InkType");
	    }
	  }
	  else if (ink_set && !strcmp(ink_set,"Color")) {
	    if (!(mode->ink_types & CANON_INK_CMY)) {
	      /* need a new mode
		 loop through modes in muse list searching for a matching inktype, comparing quality
		 quality is major factor
	      */
	      i=0;
	      modefound=0;
	      while ((muse->mode_name_list[i]!=NULL) && (modefound != 1)){
		/* need to check for duplex in case it is a replacement for another mode */
		for(j=0;j<caps->modelist->count;j++){
		  if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		    if (caps->modelist->modes[j].quality >= quality){ /* keep setting the mode until lowest matching quality is found */
		      mode = &caps->modelist->modes[j];
		      modefound=1;
		    }
		    break; /* go to next mode in muse list */
		  }
		}
		i++;
	      }
	      if (modefound == 0) { /* still did not find a mode: pick first one for that media */
		replaceres = muse->mode_name_list[0];
		for(i=0;i<caps->modelist->count;i++){
		  if(!strcmp(replaceres,caps->modelist->modes[i].name)){
		    mode = &caps->modelist->modes[i];
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: setting mode finally to be: '%s'\n",mode->name);
		    if (ERRPRINT)
		      stp_erprintf("DEBUG: Gutenprint: setting mode finally to be: '%s'\n",mode->name);
		    break;
		  }
		}
	      }
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
	    }
	    else {
	      /* mode is fine */
	      if (strcmp(stp_get_string_parameter(v, "InkType"),"RGB")) { /* if it it not set to RGB/CMY already */
		stp_set_string_parameter(v, "InkType", "RGB");
		ink_type = stp_get_string_parameter(v, "InkType");
	      }
	    }
	  } /* no restrictions for InkSet "Both" or if no InkSet set yet */
	  else {
	    if (!ink_set) {
	      stp_set_string_parameter(v, "InkSet", "Both");
	      ink_set = stp_get_string_parameter(v, "InkSet");
	    }
	    /* if InkType does not match that of mode, change InkType to match it */
	    /* choose highest color as default, as there is only one option for Black */
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) {
		if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* set InkType for the mode found */
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) {
		if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	  }
#endif
	  /* ended find a good mode code */
	  
	  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: mode searching: replaced mode with: '%s'\n",replaceres);
	  if (ERRPRINT)
	    stp_erprintf("DEBUG: Gutenprint: mode searching: replaced mode with: '%s'\n",replaceres);
	  
	  /* finally, do again with replaceres what we did with resolution */
	  for(i=0;i<caps->modelist->count;i++){
	    if(!strcmp(replaceres,caps->modelist->modes[i].name)){
	      mode = &caps->modelist->modes[i];
	      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: setting mode finally to be: '%s'\n",mode->name);
	      if (ERRPRINT)
		stp_erprintf("DEBUG: Gutenprint: setting mode finally to be: '%s'\n",mode->name);
	      break;
	    }
	  }      
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
	}

	else { /* we did find the mode in the list for media, so it should take precedence over other settings, as it is more specific. */
	  /* need to check for duplex in case it is a replacement for another mode */
	  quality = mode->quality;
	  if (ink_set && !strcmp(ink_set,"Black")) {
	    if (!(mode->ink_types & CANON_INK_K)) {
	      /* need a new mode: 
		 loop through modes in muse list searching for a matching inktype, comparing quality
		 quality is major factor
	       */
	      i=0;
	      modefound=0;
	      while ((muse->mode_name_list[i]!=NULL) && (modefound != 1)){
		/* need to check for duplex in case it is a replacement for another mode */
		for(j=0;j<caps->modelist->count;j++){
		  if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		    if (caps->modelist->modes[j].quality >= quality){ /* keep setting the mode until lowest matching quality is found */
		      mode = &caps->modelist->modes[j];
		      modefound=1;
		    }
		    break; /* go to next mode in muse list */
		  }
		}
		i++;
	      }
	      if (modefound == 0) { /* still did not find a mode: pick first one for that media */
		replaceres = muse->mode_name_list[0];
		for(i=0;i<caps->modelist->count;i++){
		  if(!strcmp(replaceres,caps->modelist->modes[i].name)){
		    mode = &caps->modelist->modes[i];
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: setting mode finally to be: '%s'\n",mode->name);
		    if (ERRPRINT)
		      stp_erprintf("DEBUG: Gutenprint: setting mode finally to be: '%s'\n",mode->name);
		    break;
		  }
		}
	      }
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
	      if (strcmp(ink_type,"Gray")) {/* if it it not set to Gray already */
		stp_set_string_parameter(v, "InkType", "Gray");
		ink_type = stp_get_string_parameter(v, "InkType");
	      }
	    }
	  }
	  else if (ink_set && !strcmp(ink_set,"Color")) {
	    if (!(mode->ink_types & CANON_INK_CMY)) {
	      /* need a new mode
		 loop through modes in muse list searching for a matching inktype, comparing quality
		 quality is major factor
	      */
	      i=0;
	      modefound=0;
	      while ((muse->mode_name_list[i]!=NULL) && (modefound != 1)){
		/* need to check for duplex in case it is a replacement for another mode */
		for(j=0;j<caps->modelist->count;j++){
		  if(!strcmp(muse->mode_name_list[i],caps->modelist->modes[j].name)){/* find right place in canon-modes list */
		    if (caps->modelist->modes[j].quality >= quality){ /* keep setting the mode until lowest matching quality is found */
		      mode = &caps->modelist->modes[j];
		      modefound=1;
		    }
		    break; /* go to next mode in muse list */
		  }
		}
		i++;
	      }
	      if (modefound == 0) { /* still did not find a mode: pick first one for that media */
		replaceres = muse->mode_name_list[0];
		for(i=0;i<caps->modelist->count;i++){
		  if(!strcmp(replaceres,caps->modelist->modes[i].name)){
		    mode = &caps->modelist->modes[i];
		    stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint: setting mode finally to be: '%s'\n",mode->name);
		    if (ERRPRINT)
		      stp_erprintf("DEBUG: Gutenprint: setting mode finally to be: '%s'\n",mode->name);
		    break;
		  }
		}
	      }
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
	    }
	    else {
	      /* mode is fine */
	      if (strcmp(stp_get_string_parameter(v, "InkType"),"RGB")) { /* if it it not set to RGB/CMY already */
		stp_set_string_parameter(v, "InkType", "RGB");
		ink_type = stp_get_string_parameter(v, "InkType");
	      }
	    }
	  } /* no restrictions for InkSet "Both" or if no InkSet set yet */
	  else {
	    if (!ink_set) {
	      stp_set_string_parameter(v, "InkSet", "Both");
	      ink_set = stp_get_string_parameter(v, "InkSet");
	    }
	    /* if InkType does not match that of mode, change InkType to match it */
	    /* choose highest color as default, as there is only one option for Black */
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) {
		if ((!ink_type) || (strcmp(ink_type,canon_inktypes[i].name))) { /* if InkType does not match selected mode ink type*/
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	    /* set InkType for the mode found */
	    for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	      if (mode->ink_types & canon_inktypes[i].ink_type) {
		if (strcmp(ink_type,canon_inktypes[i].name)) { /* if InkType does not match selected mode ink type*/
		  stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint (InkSet:Both): InkType changed to %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
		  stp_set_string_parameter(v, "InkType", canon_inktypes[i].name);
		  ink_type = stp_get_string_parameter(v, "InkType");
		  break;
		}
	      }
	    }
	  }
	}
	
      } /* limited to MP610 for now */
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
      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  get_current_mode --- Final returned mode: '%s'\n",mode->name);
      if (ERRPRINT)
	stp_erprintf("DEBUG: Gutenprint:  get_current_mode --- Final returned mode: '%s'\n",mode->name);
    }
    else {
      stp_dprintf(STP_DBG_CANON, v,"DEBUG: Gutenprint:  get_current_mode --- Final returned mode is NULL \n");
      if (ERRPRINT)
	stp_erprintf("DEBUG: Gutenprint:  get_current_mode --- Final returned mode is NULL \n");
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

  if (ERRPRINT)
    stp_erprintf(" entered canon_printhead_colors: got PrintingMode %s\n",print_mode);

  /* if the printing mode was already selected as BW, accept it */
  if(print_mode && !strcmp(print_mode, "BW")){
    stp_dprintf(STP_DBG_CANON, v,"(canon_printhead_colors[BW]) Found InkType %i(CANON_INK_K)\n",CANON_INK_K);
    if (ERRPRINT)
      stp_erprintf(" canon_printhead_colors Found InkType %i(CANON_INK_K)\n",CANON_INK_K);
    return CANON_INK_K;
  }
  /* alternatively, if the cartridge selection is in force, and black cartride is selected, accept it */
  if(ink_set && !strcmp(ink_set, "Black")){
    stp_dprintf(STP_DBG_CANON, v,"(canon_printhead_colors[BW]) Found InkSet black selection\n");
    if (ERRPRINT)
      stp_erprintf("(canon_printhead_colors[BW]) Found InkSet black selection\n");
    return CANON_INK_K;
  }


  /* if a mode is available, use it. Else mode is NULL */
  if (ERRPRINT)
    stp_erprintf("Calling get_current_parameter from canon_printhead_colors");
  mode = canon_get_current_mode(v);

  
  /* originaly finds selected InkType of form: CANON_INK_<inks> */
  /* but this is incorrect, since it does not check media or mode */
  /* change: deal with mode set and mode not set cases */

  /* if mode was already set, then return the ink types for only that mode */

  if (mode) {
    /* if an inktype selected check what it is */
    if(ink_type){
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	if (mode->ink_types & canon_inktypes[i].ink_type) {
	  stp_dprintf(STP_DBG_CANON, v,"(canon_printhead_colors[inktype]) Found InkType %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
	  if (ERRPRINT)
	    stp_erprintf("(canon_printhead_colors[inktype]) Found InkType %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
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
	  if (ERRPRINT)
	    stp_erprintf("(canon_printhead_colors[mode]) Found InkType %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
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
	  if (ERRPRINT)
	    stp_erprintf("(canon_printhead_colors[inktype]) Found InkType %i(%s)\n",canon_inktypes[i].ink_type,canon_inktypes[i].name);
	  return canon_inktypes[i].ink_type;
	}
      }
    }
    else { /* no ink type selected yet */
      if (ERRPRINT)
	stp_erprintf(" canon_printhead_colors: no mode and no inktype: we have to choose the highest one to return\n");
      /* loop through all modes, and return the highest inktype found */
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	for(j=0;j<caps->modelist->count;j++){
	  if(caps->modelist->modes[j].ink_types & canon_inktypes[i].ink_type){
	    stp_dprintf(STP_DBG_CANON, v," highest inktype found ---  %s(%s)\n",canon_inktypes[i].name,canon_inktypes[i].text);
	    if (ERRPRINT)
	      stp_erprintf(" highest inktype found --- %s(%s)\n",canon_inktypes[i].name,canon_inktypes[i].text);
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
  if (ERRPRINT)
    stp_erprintf("(canon_printhead_colors[fall-through]) Found InkType %i(CANON_INK_K)\n",CANON_INK_K);
  return CANON_INK_K;
#endif
  /* new fallback: loop through ink type in reverse order, picking first one found, which if CANON_INK_K is supported will be that, else the lowest amount of color */
  for(i=((sizeof(canon_inktypes)/sizeof(canon_inktypes[0]))-1);i=0;i--){
    for(j=0;j<caps->modelist->count;j++){
      if(caps->modelist->modes[j].ink_types & canon_inktypes[i].ink_type){
	stp_dprintf(STP_DBG_CANON, v," lowest inktype found ---  %s(%s)\n",canon_inktypes[i].name,canon_inktypes[i].text);
	if (ERRPRINT)
	  stp_erprintf(" lowest inktype found --- %s(%s)\n",canon_inktypes[i].name,canon_inktypes[i].text);
	return canon_inktypes[i].ink_type;
      }      
    }
  }


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
      /* built ins:                                  Japanese driver notation */
      if (!strcmp(name,"A5"))          return 0x01;
      if (!strcmp(name,"A4"))          return 0x03;
      if (!strcmp(name,"A3"))          return 0x05;
      if (!strcmp(name,"B5"))          return 0x08; 
      if (!strcmp(name,"B4"))          return 0x0a;
      if (!strcmp(name,"Letter"))      return 0x0d;
      if (!strcmp(name,"Legal"))       return 0x0f;
      if (!strcmp(name,"Tabloid"))     return 0x11; /* 11x17 */
      if (!strcmp(name,"w283h420"))    return 0x14; /* Hagaki */
      /*      if (!strcmp(name,"COM10"))       return 0x16;*/
      /*      if (!strcmp(name,"DL"))          return 0x17;*/
      if (!strcmp(name,"LetterExtra")) return 0x2a; /* Letter navi --- Letter+ */
      if (!strcmp(name,"A4Extra"))     return 0x2b; /* A4navi --- A4+ */
      if (!strcmp(name,"A3plus"))      return 0x2c; /* A3navi --- A3+ */
      if (!strcmp(name,"w288h144"))    return 0x2d; /* ??? */
      if (!strcmp(name,"COM10"))       return 0x2e; /* US Comm #10 Env */
      if (!strcmp(name,"DL"))          return 0x2f; /* Euro DL Env */
      if (!strcmp(name,"w297h666"))    return 0x30; /* Western Env #4 (you4) */
      if (!strcmp(name,"w277h538"))    return 0x31; /* Western Env #6 (you6) */
      if (!strcmp(name,"w252h360J"))   return 0x32; /* L --- similar to US 3.5x5 size */
      if (!strcmp(name,"w360h504J"))   return 0x33; /* 2L --- similar to US5x7 */
      if (!strcmp(name,"w288h432J"))   return 0x34; /* KG --- same size as US 4x6 */
      if (!strcmp(name,"w155h257"))    return 0x36; /* Japanese Business Card 55mm x 91mm */
      if (!strcmp(name,"w360h504"))    return 0x37; /* US5x7 */
      if (!strcmp(name,"w420h567"))    return 0x39; /* Ofuku Hagaki */
      if (!strcmp(name,"w340h666"))    return 0x3a; /* Japanese Long Env #3 (chou3) */
      if (!strcmp(name,"w255h581"))    return 0x3b; /* Japanese Long Env #4 (chou4) */
      if (!strcmp(name,"w155h244"))    return 0x41; /* Business/Credit Card 54mm x 86mm */
      /* if (!strcmp(name,"A4"))       return 0x42; */ /* FineArt A4 35mm border --- iP7100: gap is 18 */
      /* if (!strcmp(name,"A3"))       return 0x43; */ /* FineArt A3 35mm border --- iP7100: gap is 18 */
      /* if (!strcmp(name,"Letter"))   return 0x44; */ /* FineArt Letter 35mm border --- iP7100: gap is 18 */
      if (!strcmp(name,"w288h576"))    return 0x46; /* US4x8 */
      if (!strcmp(name,"w1008h1224J")) return 0x47; /* HanKire --- 14in x 17in */
      if (!strcmp(name,"720h864J"))    return 0x48; /* YonKire --- 10in x 12 in*/
      if (!strcmp(name,"c8x10J"))      return 0x49; /* RokuKire --- same size as 8x10 */
      /* if (!strcmp(name,"A4"))       return 0x4d; */ /* ArtA4 35mm border */
      /* if (!strcmp(name,"A3"))       return 0x4e; */ /* ArtA3 35mm border */
      /* if (!strcmp(name,"Letter"))   return 0x4f; */ /* ArtLetter 35mm border */
      if (!strcmp(name,"w288h512"))    return 0x52; /* Wide101.6x180.6 */
      /* w283h566 Wide postcard 148mm x 200mm */

      /* media size codes for CD (and other media depending on printer model */
      if (!strcmp(name,"CD5Inch"))    return 0x53; /* CD --- arbitrary choice here, modify in ESC (P command */
      /* similar needed for FineArt media which have common sizes but different codes */

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
  const canon_mode_t* mode = NULL;
  const canon_cap_t * caps = canon_get_model_capabilities(v);
 
  /* if mode is not yet set, it remains NULL */
  if (ERRPRINT)
    stp_erprintf("Calling get_current_parameter from canon_describe_resolution");
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
      int height_limit, width_limit;
      int papersizes = stp_known_papersizes();
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
        for (i = 0; i < papersizes; i++) {
          const stp_papersize_t *pt = stp_get_papersize_by_index(i);
          if (strlen(pt->name) > 0 &&
	      pt->width <= width_limit && pt->height <= height_limit){
	    stp_string_list_add_string(description->bounds.str,
				     pt->name, gettext(pt->text));
           }
        }
      }
      description->deflt.str =
        stp_string_list_param(description->bounds.str, 0)->name;
  }
  else if (strcmp(name, "CDInnerRadius") == 0 )
    {
      const char* input_slot = stp_get_string_parameter(v, "InputSlot");
      description->bounds.str = stp_string_list_create();
      if (input_slot && !strcmp(input_slot,"CD") &&
         (!stp_get_string_parameter(v, "PageSize") ||
          strcmp(stp_get_string_parameter(v, "PageSize"), "CDCustom") != 0))
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
      if (input_slot && !strcmp(input_slot,"CD") &&
         (!stp_get_string_parameter(v, "PageSize") ||
         strcmp(stp_get_string_parameter(v, "PageSize"), "CDCustom") == 0))
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
      if (input_slot && !strcmp(input_slot,"CD") &&
         (!stp_get_string_parameter(v, "PageSize") ||
          strcmp(stp_get_string_parameter(v, "PageSize"), "CDCustom") == 0))
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
      if (input_slot && !strcmp(input_slot,"CD"))
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
	if(!(input_slot && !strcmp(input_slot,"CD") && !(caps->modelist->modes[i].flags & MODE_FLAG_CD)))
#endif
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
    const canon_mode_t* mode = NULL;

    if (ERRPRINT)
      stp_erprintf("Calling get_current_parameter from InkType block in canon_parameters");
    mode=canon_get_current_mode(v);

    description->bounds.str= stp_string_list_create();
    if (mode) {
      for(i=0;i<sizeof(canon_inktypes)/sizeof(canon_inktypes[0]);i++){
	if(mode->ink_types & canon_inktypes[i].ink_type){
          stp_string_list_add_string(description->bounds.str,canon_inktypes[i].name,_(canon_inktypes[i].text));
	  stp_dprintf(STP_DBG_CANON, v," mode known --- Added InkType %s(%s) for %s\n",canon_inktypes[i].name,canon_inktypes[i].text,mode->name);
	  if (ERRPRINT)
	    stp_erprintf(" mode known --- Added InkType %s(%s) for %s\n",canon_inktypes[i].name,canon_inktypes[i].text,mode->name);
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
	    stp_dprintf(STP_DBG_CANON, v," no mode --- Added InkType %s(%s)\n",canon_inktypes[i].name,canon_inktypes[i].text);
	    if (ERRPRINT)
	      stp_erprintf(" no mode --- Added InkType %s(%s)\n",canon_inktypes[i].name,canon_inktypes[i].text);
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
	      if (ERRPRINT)
		stp_erprintf("Added %d InkChannels\n",canon_inktypes[i].num_channels);
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
  else if (strcmp(name, "PrintingMode") == 0)
  {
    int found_color, found_mono;
    const canon_mode_t* mode = NULL;
    /* mode remains NULL if not yet set */
    
    if (ERRPRINT)
      stp_erprintf("Calling get_current_mode from PrintingMode block in canon_parameter");
    mode = canon_get_current_mode(v);
    
    /* If mode is not set need to search ink types for all modes and
       see whether we have any color there
     */

    if (ERRPRINT)
      stp_erprintf("DEBUG: Gutenprint: PrintingMode---entered enumeration block in canon_printers\n");

    description->bounds.str = stp_string_list_create();

    if (ERRPRINT)
      stp_erprintf("DEBUG: Gutenprint: PrintingMode---created list\n");

    if (mode) {
      if (ERRPRINT)
	stp_erprintf("DEBUG: Gutenprint: PrintingMode: (mode known) what is the current mode inktype value: %i\n",mode->ink_types);
      /* e.g., ink_types is 21 = 16 + 4 + 1 */
      if (mode->ink_types > 1) {
	stp_string_list_add_string
	  (description->bounds.str, "Color", _("Color"));
	if (ERRPRINT)
	  stp_erprintf("DEBUG: Gutenprint: PrintingMode: (mode known) added Color\n");
      }
      if (mode->ink_types & CANON_INK_K) {
	stp_string_list_add_string
	  (description->bounds.str, "BW", _("Black and White"));
	if (ERRPRINT)
	  stp_erprintf("DEBUG: Gutenprint: PrintingMode: (mode known) added BW\n");
      }
    }
#if 0
      /* original code */
      if (mode)
	if (mode->ink_types != CANON_INK_K) {
	  stp_string_list_add_string
	    (description->bounds.str, "Color", _("Color"));
	  if (ERRPRINT)
	    stp_erprintf("DEBUG: Gutenprint: PrintingMode: (mode known) added Color\n");
	}
#endif

    else { /* mode not known yet --- needed for PPD generation */
      if (ERRPRINT)
	stp_erprintf("DEBUG: Gutenprint: PrintingMode: entered mode not known conditional block\n");
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
	    if (ERRPRINT)
	      stp_erprintf("DEBUG: Gutenprint: PrintingMode: (mode not known) added Color\n");
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
	    if (ERRPRINT)
	      stp_erprintf("DEBUG: Gutenprint: PrintingMode: (mode not known) added BW\n");
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
	if (ERRPRINT)
	  stp_erprintf("DEBUG: Gutenprint: PrintingMode: (mode not known) added Color\n");
      }
      if(caps->modelist->modes[caps->modelist->default_mode].ink_types & CANON_INK_K){
	stp_string_list_add_string
	(description->bounds.str, "BW", _("Black and White"));
	if (ERRPRINT)
	  stp_erprintf("DEBUG: Gutenprint: PrintingMode: (mode not known) added BW\n");	
      }
#endif
#if 0
      /* original code */
      stp_string_list_add_string
	(description->bounds.str, "BW", _("Black and White"));
      if (ERRPRINT)
	stp_erprintf("DEBUG: Gutenprint: PrintingMode: added BW\n");
#endif
    }
    
    /* original code --- fine as is */
    description->deflt.str =
      stp_string_list_param(description->bounds.str, 0)->name;
  } 
  else if (strcmp(name, "InkSet") == 0)
    {
      /* const canon_mode_t* mode = canon_get_current_mode(v); */
      description->bounds.str= stp_string_list_create();
      if (caps->features & CANON_CAP_T) {
	stp_string_list_add_string
	  (description->bounds.str, "Black", _("Black"));
	stp_string_list_add_string
	  (description->bounds.str, "Color", _("Color"));
      }
      /* for now make sure to have at least a default value */
      stp_string_list_add_string
	(description->bounds.str, "Both", _("Both"));
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
  int cd = 0;

  const canon_cap_t * caps= canon_get_model_capabilities(v);
  const char *media_size = stp_get_string_parameter(v, "PageSize");
  const stp_papersize_t *pt = NULL;
  const char* input_slot = stp_get_string_parameter(v, "InputSlot");

  if(input_slot && !strcmp(input_slot,"CD"))
    cd = 1;

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
  /* ignore printer margins for the cd print, margins get adjusted in do_print */
  if(!cd){
    left_margin = MAX(left_margin, caps->border_left);
    right_margin = MAX(right_margin, caps->border_right);
    top_margin = MAX(top_margin, caps->border_top);
    bottom_margin = MAX(bottom_margin, caps->border_bottom);
  }

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

#define PUT(WHAT,VAL,RES) stp_deprintf(STP_DBG_CANON,"canon: "WHAT\
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
    if ( !(strcmp(init->caps->name,"i860")) || !(strcmp(init->caps->name,"i865")) || !(strcmp(init->caps->name,"i950")) ) {
      /* i860, i865, i950 use ESC ($ command even for simplex mode */
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

                  arg_63[1] = ((init->pt ? init->pt->media_code_c : 0) << 4)                /* PRINT_MEDIA */
			+ 1;	/* hardcode to High quality for now */		/* PRINT_QUALITY */

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

  if (init->pt) arg_6c_2= init->pt->media_code_l;
  if(init->caps->model_id >= 3)
    /* not all MP models have this, I believe it is a generation-related issue */
    if ( (!strcmp(init->caps->name,"PIXMA MP450")) || (!strcmp(init->caps->name,"PIXMA MP460")) || (!strcmp(init->caps->name,"i80")) ) 
      canon_cmd(v,ESC28,0x6c, 2, arg_6c_1, arg_6c_2); /* add MP150-MP470, and i80 */
    else
      canon_cmd(v,ESC28,0x6c, 3, arg_6c_1, arg_6c_2, arg_6c_3); /* 3rd arg is "gap" */
  else
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
canon_init_setPageMargins2(const stp_vars_t *v, const canon_privdata_t *init)
{
  /* TOFIX: what exactly is to be sent?
   * Is it the printable length or the bottom border?
   * Is is the printable width or the right border?
   */
  int printable_width=  (init->page_width + 1)*5/6;
  int printable_length= (init->page_height + 1)*5/6;

  unsigned char arg_70_1= (printable_length >> 8) & 0xff;
  unsigned char arg_70_2= (printable_length) & 0xff;
  unsigned char arg_70_3= (printable_width >> 8) & 0xff;
  unsigned char arg_70_4= (printable_width) & 0xff;
  const char* input_slot = stp_get_string_parameter(v, "InputSlot");

  if (!(init->caps->features & CANON_CAP_px) && !(init->caps->features & CANON_CAP_p))
	return;

  if ((init->caps->features & CANON_CAP_px) ) { /* && !(input_slot && !strcmp(input_slot,"CD")) ) */
    if ( !(input_slot && !strcmp(input_slot,"CD")) || !(strcmp(init->caps->name,"PIXMA iP4600")) || !(strcmp(init->caps->name,"PIXMA iP4700")) || !(strcmp(init->caps->name,"PIXMA iP4800")) || !(strcmp(init->caps->name,"PIXMA iP4900")) || !(strcmp(init->caps->name,"PIXMA MP980")) || !(strcmp(init->caps->name,"PIXMA MP990")) || !(strcmp(init->caps->name,"PIXMA MG5200")) || !(strcmp(init->caps->name,"PIXMA MG5300")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) )
      {
	unsigned int unit = 600;

	int border_left=init->caps->border_left;
	int border_right=init->caps->border_right;
	int border_top=init->caps->border_top;
	int border_bottom=init->caps->border_bottom;
	if (input_slot && !strcmp(input_slot,"CD")) {
	  border_top=9;
	  border_bottom=9;
	}

	stp_zfwrite(ESC28,2,1,v); /* ESC( */
	stp_putc(0x70,v);         /* p    */
	stp_put16_le(46, v);      /* len  */
	stp_put16_be(printable_length,v); /* Windows 698, gutenprint 570 */
	stp_put16_be(0,v);
	stp_put16_be(printable_width,v); /* Windows 352, gutenprint 342 */
	stp_put16_be(0,v);
	stp_put32_be(0,v);
	stp_put16_be(unit,v);
	
	stp_put32_be(border_left * unit / 72,v); /* area_right : Windows seems to use 9.6, gutenprint uses 10 */
	stp_put32_be(border_top * unit / 72,v);  /* area_top : Windows seems to use 8.4, gutenprint uses 15 */
	stp_put32_be(init->page_width  * unit / 72,v); /* area_width : Windows seems to use 352 for Tray G, gutenprint uses 340.92 */
	stp_put32_be(init->page_height * unit / 72,v); /* area_length : Windows seems to use 698.28 for Tray G, gutenprint uses 570 */
	stp_put32_be(0,v); /* paper_right : Windows also 0 here for all Trays */
	stp_put32_be(0,v); /* paper_top : Windows also 0 here for all Trays */
	stp_put32_be((init->page_width + border_left + border_right) * unit / 72,v); /* paper_width : Windows 371.4, gutenprint 360.96 */
	stp_put32_be((init->page_height + border_top + border_bottom) * unit / 72,v); /* paper_height : Windows 720.96, gutenprint 600 */
	return;
      }
  }
  canon_cmd(v,ESC28,0x70, 8,
   	      arg_70_1, arg_70_2, 0x00, 0x00,
	      arg_70_3, arg_70_4, 0x00, 0x00);
}

/* ESC (P -- 0x50 -- unknown -- :
    pt = stp_get_papersize_by_name(media_size);
   seems to set media and page information. Different byte lengths depending on printer model. */
static void
canon_init_setESC_P(const stp_vars_t *v, const canon_privdata_t *init)
{
  unsigned char arg_ESCP_1, arg_ESCP_2;
  if(!(init->caps->features & CANON_CAP_P))
    return;

  arg_ESCP_1 = (init->pt) ? canon_size_type(v,init->caps): 0x03;
  arg_ESCP_2 = (init->pt) ? init->pt->media_code_P: 0x00;

  /* workaround for CD media */

  if ( (arg_ESCP_2 == 0x1f) || ( arg_ESCP_2 == 0x20) ) {
    if ( arg_ESCP_1 == 0x53 ) {
      /* Tray G as default */
      arg_ESCP_1 = 0x53;
      /* Custom CD tray */
      if ( !(strcmp(init->caps->name,"Canon i865")) || !(strcmp(init->caps->name,"PIXMA MP710")) || !(strcmp(init->caps->name,"PIXMA MP740")) || !(strcmp(init->caps->name,"PIXMA MP900")) ) {
	arg_ESCP_1 = 0x35;
      }
      /* Tray A */
      if ( !(strcmp(init->caps->name,"PIXMA iP9910")) ) {
	arg_ESCP_1 = 0x3f;
      }
      /* Tray B */
      if ( !(strcmp(init->caps->name,"PIXMA MP750")) || !(strcmp(init->caps->name,"PIXMA MP760")) || !(strcmp(init->caps->name,"PIXMA MP770")) || !(strcmp(init->caps->name,"PIXMA MP780")) || !(strcmp(init->caps->name,"PIXMA MP790")) || !(strcmp(init->caps->name,"PIXMA iP3000")) || !(strcmp(init->caps->name,"PIXMA iP3100")) || !(strcmp(init->caps->name,"PIXMA iP4000")) || !(strcmp(init->caps->name,"PIXMA iP4100")) || !(strcmp(init->caps->name,"PIXMA iP4100R")) || !(strcmp(init->caps->name,"PIXMA iP5000")) || !(strcmp(init->caps->name,"PIXMA iP6000D")) || !(strcmp(init->caps->name,"PIXMA iP6100D")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8500")) || !(strcmp(init->caps->name,"PIXMA iP8600")) ) {
	arg_ESCP_1 = 0x40;
      }
      /* Tray C */
      if ( !(strcmp(init->caps->name,"PIXMA MP950")) || !(strcmp(init->caps->name,"PIXMA iP4200")) || !(strcmp(init->caps->name,"PIXMA iP5200")) || !(strcmp(init->caps->name,"PIXMA iP6700D")) || !(strcmp(init->caps->name,"PIXMA iP7500")) ) {
	arg_ESCP_1 = 0x4a;
      }
      /* Tray D */
      if ( !(strcmp(init->caps->name,"PIXMA MP500")) || !(strcmp(init->caps->name,"PIXMA MP530")) || !(strcmp(init->caps->name,"PIXMA MP800")) || !(strcmp(init->caps->name,"PIXMA MP830")) ) {
	arg_ESCP_1 = 0x4b;
      }
      /* Tray E */
      if ( !(strcmp(init->caps->name,"PIXMA Pro9000")) || !(strcmp(init->caps->name,"PIXMA Pro9000mk2")) || !(strcmp(init->caps->name,"PIXMA Pro9500")) || !(strcmp(init->caps->name,"PIXMA Pro9500mk2")) ) {
	arg_ESCP_1 = 0x4c;
      }
      /* Tray F */
      if ( !(strcmp(init->caps->name,"PIXMA MP600")) || !(strcmp(init->caps->name,"PIXMA MP610")) || !(strcmp(init->caps->name,"PIXMA MP810")) || !(strcmp(init->caps->name,"PIXMA MP960")) || !(strcmp(init->caps->name,"PIXMA MP970")) || !(strcmp(init->caps->name,"PIXMA MX850")) || !(strcmp(init->caps->name,"PIXMA iP4300")) || !(strcmp(init->caps->name,"PIXMA iP4500")) || !(strcmp(init->caps->name,"PIXMA iP5300")) ) {
	arg_ESCP_1 = 0x51;
      }
      /* Tray G from iP4800 onwards */
      if ( !(strcmp(init->caps->name,"PIXMA iP4800")) || !(strcmp(init->caps->name,"PIXMA iP4900")) || !(strcmp(init->caps->name,"PIXMA MG5200")) || !(strcmp(init->caps->name,"PIXMA MG5300")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) ) {
	arg_ESCP_1 = 0x56;
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
      /* iP7500: CD Tray C     : 0x4a               */
      /* iP8100: CD Tray B     : 0x40               */
      /* iP8500 :CD Tray B     : 0x40               */
      /* iP8600: CD Tray B     : 0x40               */
      /* iP9910: CD Tray A     : 0x3f               */
      /* MG5200: CD Tray G     : 0x56               */
      /* MG5300: CD Tray G     : 0x56               */
      /* MG6100: CD Tray G     : 0x56               */
      /* MG6200: CD Tray G     : 0x56               */
      /* MG8100: CD Tray G     : 0x56               */
      /* MG8200: CD Tray G     : 0x56               */
      /* pro9000:CD Tray E     : 0x4c               */
      /* pro9000mk2:CD Tray E  : 0x4c               */
      /* pro9500:CD Tray E     : 0x4c               */
      /* pro9500mk2:CD Tray E  : 0x4c               */
      /* PRO-1:  CD Tray H     : 0x57               */



  /* workaround for FineArt media having same size as non-FineArt media */

      /* MP950:  FineArtA4     : 0x42               */
      /* MP960:  FineArtA4     : 0x42               */
      /* MP970:  FineArtA4     : 0x42               */
      /* MP980:  FineArtA4     : 0x42               */
      /* MP990:  FineArtA4     : 0x42               */
      /* MX7600: FineArtA4     : 0x42               */
      /* iP6700D:FineArtA4     : 0x42               */
      /* iP7100: FineArtA4     : 0x42               */
      /* iP7500: FineArtA4     : 0x42               */
      /* iP8100: FineArtA4     : 0x42               */
      /* iP8600: FineArtA4     : 0x42               */
      /* iP9910: FineArtA4     : 0x42               */
      /* iX7000: FineArtA4     : 0x42               */
      /* MG6100: FineArtA4     : 0x42               */
      /* MG6200: FineArtA4     : 0x42               */
      /* MG8100: FineArtA4     : 0x42               */
      /* MG8200: FineArtA4     : 0x42               */
      /* pro9000:FineArtA4     : 0x4d               */
      /* pro9000mk2:FineArtA4  : 0x4d               */
      /* pro9500:FineArtA4     : 0x4d               */
      /* pro9500mk2:FineArtA4  : 0x4d               */
      /* PRO-1:  FineArtA4     : 0x4d               */

  /* iP7100 is an exception needing yet another papersize code */
  if ( (arg_ESCP_2 == 0x28) || ( arg_ESCP_2 == 0x29) || (arg_ESCP_2 ==  0x2c) || (arg_ESCP_2 == 0x31) ) {
    /* A4 */
    if ( arg_ESCP_1 == 0x03 ) {
      /* default */
      arg_ESCP_1 = 0x4d;
      if ( !(strcmp(init->caps->name,"PIXMA MP950")) || !(strcmp(init->caps->name,"PIXMA MP960")) || !(strcmp(init->caps->name,"PIXMA MP970")) || !(strcmp(init->caps->name,"PIXMA MP980")) || !(strcmp(init->caps->name,"PIXMA MP990")) || !(strcmp(init->caps->name,"PIXMA MX7600")) || !(strcmp(init->caps->name,"PIXMA iP6700D")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP7500")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8600")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA iX7000")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) ) {
	arg_ESCP_1 = 0x42;
      }
    }
    /* A3 */
    if ( arg_ESCP_1 == 0x05 ) {
      arg_ESCP_1 = 0x4e;
      if ( !(strcmp(init->caps->name,"PIXMA MP950")) || !(strcmp(init->caps->name,"PIXMA MP960")) || !(strcmp(init->caps->name,"PIXMA MP970")) || !(strcmp(init->caps->name,"PIXMA MP980")) || !(strcmp(init->caps->name,"PIXMA MP990")) || !(strcmp(init->caps->name,"PIXMA MX7600")) || !(strcmp(init->caps->name,"PIXMA iP6700D")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP7500")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8600")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA iX7000")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) ) {
	arg_ESCP_1 = 0x43;
      }
    }
    /* Letter */
    if ( arg_ESCP_1 == 0x0d ) {
      arg_ESCP_1 = 0x4f;
      if ( !(strcmp(init->caps->name,"PIXMA MP950")) || !(strcmp(init->caps->name,"PIXMA MP960")) || !(strcmp(init->caps->name,"PIXMA MP970")) || !(strcmp(init->caps->name,"PIXMA MP980")) || !(strcmp(init->caps->name,"PIXMA MP990")) || !(strcmp(init->caps->name,"PIXMA MX7600")) || !(strcmp(init->caps->name,"PIXMA iP6700D")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP7500")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8600")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA iX7000")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) ) {
	arg_ESCP_1 = 0x44;
      }
    }
  }

  /* models that add two more bytes "1 0" to the end of the usual 4-byte sequence: */
  /* TODO: the code for 6-byte sequences can be replaced by a simple test of XML-capability */
  /* MX340 */
  /* MX350 --- same driver as MX340 */
  /* MX360 */
  /* MX410 --- same driver as MX360 */
  /* MX420 */
  /* MX870 */
  /* MX880 */
  /* MP250 */
  /* MP270 */
  /* MP280 */
  /* MP490 */
  /* MP493 */
  /* MP495 */
  /* MP550 */
  /* MP560 */
  /* MP640 */
  /* MP990 */
  /* iX6500 */
  /* iX7000 */
  /* iP2700 */
  /* iP4700 */
  /* iP4800 */
  /* iP4900 */
  /* iP9910 */
  /* MG2100 */
  /* MG3100 */
  /* MG4100 */
  /* MG5100 */
  /* MG5200 */
  /* MG5300 */
  /* MG6100 */
  /* MG6200 */
  /* MG8100 */
  /* MG8200 */
  if ( !(strcmp(init->caps->name,"PIXMA MX340")) || !(strcmp(init->caps->name,"PIXMA MX350")) || !(strcmp(init->caps->name,"PIXMA MX360")) || !(strcmp(init->caps->name,"PIXMA MX410")) || !(strcmp(init->caps->name,"PIXMA MX420")) || !(strcmp(init->caps->name,"PIXMA MX870")) || !(strcmp(init->caps->name,"PIXMA MX880")) || !(strcmp(init->caps->name,"PIXMA MP250")) || !(strcmp(init->caps->name,"PIXMA MP270")) || !(strcmp(init->caps->name,"PIXMA MP280")) || !(strcmp(init->caps->name,"PIXMA MP490")) || !(strcmp(init->caps->name,"PIXMA MP493")) || !(strcmp(init->caps->name,"PIXMA MP495")) || !(strcmp(init->caps->name,"PIXMA MP550")) || !(strcmp(init->caps->name,"PIXMA MP560")) || !(strcmp(init->caps->name,"PIXMA MP640")) ||  !(strcmp(init->caps->name,"PIXMA MP990")) || !(strcmp(init->caps->name,"PIXMA iX6500")) || !(strcmp(init->caps->name,"PIXMA iX7000")) || !(strcmp(init->caps->name,"PIXMA iP2700")) || !(strcmp(init->caps->name,"PIXMA iP4700")) || !(strcmp(init->caps->name,"PIXMA iP4800")) || !(strcmp(init->caps->name,"PIXMA iP4900")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA MG2100")) || !(strcmp(init->caps->name,"PIXMA MG3100")) || !(strcmp(init->caps->name,"PIXMA MG4100")) || !(strcmp(init->caps->name,"PIXMA MG5100")) || !(strcmp(init->caps->name,"PIXMA MG5200")) || !(strcmp(init->caps->name,"PIXMA MG5300")) || !(strcmp(init->caps->name,"PIXMA MG6100")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG8100")) || !(strcmp(init->caps->name,"PIXMA MG8200")) )
 /* add a lot more here: try if(init->caps->model_id >= 3) how to guess for 4 bytes or more */
    {/* the 4th of the 6 bytes is the media type. 2nd byte is media size. Both read from canon-media array. */

      /* arg_ESCP_1 = 0x03; */ /* A4 size */
      /* arg_ESCP_2 = 0x00; */ /* plain media */
      /*                             size                media             */
      canon_cmd( v,ESC28,0x50,6,0x00,arg_ESCP_1,0x00,arg_ESCP_2,0x01,0x00);
    }
  else if ( !(strcmp(init->caps->name,"i80")) || !(strcmp(init->caps->name,"i560")) || !(strcmp(init->caps->name,"i860")) || !(strcmp(init->caps->name,"i865")) || !(strcmp(init->caps->name,"i900")) || !(strcmp(init->caps->name,"i950")) || !(strcmp(init->caps->name,"i960")) || !(strcmp(init->caps->name,"i9900")) || !(strcmp(init->caps->name,"SELPHY DS700")) || !(strcmp(init->caps->name,"PIXMA MP130")) || !(strcmp(init->caps->name,"PIXMA MP360")) || !(strcmp(init->caps->name,"PIXMA MP370")) || !(strcmp(init->caps->name,"PIXMA MP375R")) || !(strcmp(init->caps->name,"PIXMA MP390")) || !(strcmp(init->caps->name,"PIXMA MP710")) || !(strcmp(init->caps->name,"PIXMA MP740")) || !(strcmp(init->caps->name,"PIXMA MP750")) || !(strcmp(init->caps->name,"PIXMA MP760")) || !(strcmp(init->caps->name,"PIXMA MP770")) || !(strcmp(init->caps->name,"PIXMA MP780")) || !(strcmp(init->caps->name,"PIXMA MP790")) || !(strcmp(init->caps->name,"PIXMA MP900"))  || !(strcmp(init->caps->name,"PIXMA iP1000"))  || !(strcmp(init->caps->name,"PIXMA iP1500"))  || !(strcmp(init->caps->name,"PIXMA iP2000"))  || !(strcmp(init->caps->name,"PIXMA iP3000"))  || !(strcmp(init->caps->name,"PIXMA iP3100"))  || !(strcmp(init->caps->name,"PIXMA iP4000")) || !(strcmp(init->caps->name,"PIXMA iP4100")) || !(strcmp(init->caps->name,"PIXMA iP5000"))  || !(strcmp(init->caps->name,"PIXMA iP6000D")) || !(strcmp(init->caps->name,"PIXMA iP6100D")) || !(strcmp(init->caps->name,"PIXMA iP8500"))  )  {
    /* 2 bytes only */
      canon_cmd( v,ESC28,0x50,2,0x00,arg_ESCP_1 );
    }	
  else /* 4 bytes */
    /*                             size            media       */
    canon_cmd( v,ESC28,0x50,4,0x00,arg_ESCP_1,0x00,arg_ESCP_2 );
}

/* ESC (T -- 0x54 -- setCartridge -- :
 */
static void
canon_init_setCartridge(const stp_vars_t *v, const canon_privdata_t *init)
{
  if (!(init->caps->features & CANON_CAP_T))
    return;

  const char *ink_set = stp_get_string_parameter(v, "InkSet");
  if (ink_set && !(strcmp(ink_set,"Both"))) {
    if ( !(strcmp(init->caps->name,"PIXMA iP90")) || !(strcmp(init->caps->name,"PIXMA iP90v")) ) {
      canon_cmd(v,ESC28,0x54,3,0x02,0x00,0x00); /* default for iP90, iP100 */
      /* black save     : 2 1 0 for selected modes, rest 2 0 0 */
      /* composite black: 2 0 1 for selected modes, rest 2 0 0 */
      /* both blacks    : 2 1 1 for selected modes, some 2 0 1, rest 2 0 0  */
    } 
    else if ( !(strcmp(init->caps->name,"PIXMA iP6210")) || !(strcmp(init->caps->name,"PIXMA iP6220")) || !(strcmp(init->caps->name,"PIXMA iP6310")) ) {
      canon_cmd(v,ESC28,0x54,3,0x03,0x06,0x06); /* default for iP6210D, iP6220D, iP6310D */
      /* both:  0x3 0x6 0x6 */
      /* color: 0x3 0x1 0x1 */
    }
    else {
      canon_cmd(v,ESC28,0x54,3,0x03,0x04,0x04); /* default: both cartridges */
    }
  }
  else if (ink_set && !(strcmp(ink_set,"Black"))) {
    if ( !(strcmp(init->caps->name,"PIXMA iP90")) || !(strcmp(init->caps->name,"PIXMA iP90v")) ) {
      canon_cmd(v,ESC28,0x54,3,0x02,0x01,0x01); /* default for iP90, iP100 */
      /* black save     : 2 1 0 for selected modes, rest 2 0 0 */
      /* composite black: 2 0 1 for selected modes, rest 2 0 0 */
      /* both blacks    : 2 1 1 for selected modes, some 2 0 1, rest 2 0 0  */
    } 
    else if ( !(strcmp(init->caps->name,"PIXMA iP6210")) || !(strcmp(init->caps->name,"PIXMA iP6220")) || !(strcmp(init->caps->name,"PIXMA iP6310")) ) {
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
    if ( !(strcmp(init->caps->name,"PIXMA iP90")) || !(strcmp(init->caps->name,"PIXMA iP90v")) ) {
      canon_cmd(v,ESC28,0x54,3,0x02,0x00,0x00); /* default for iP90, iP100 */
      /* black save     : 2 1 0 for selected modes, rest 2 0 0 */
      /* composite black: 2 0 1 for selected modes, rest 2 0 0 */
      /* both blacks    : 2 1 1 for selected modes, some 2 0 1, rest 2 0 0  */
      /* workaround since maybe no color option on these printers */
    } 
    else if ( !(strcmp(init->caps->name,"PIXMA iP6210")) || !(strcmp(init->caps->name,"PIXMA iP6220")) || !(strcmp(init->caps->name,"PIXMA iP6310")) ) {
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
    else if ( !(strcmp(init->caps->name,"S820")) || !(strcmp(init->caps->name,"S900")) || !(strcmp(init->caps->name,"i950")) || !(strcmp(init->caps->name,"i960")) || !(strcmp(init->caps->name,"i9100")) || !(strcmp(init->caps->name,"i9900")) || !(strcmp(init->caps->name,"PIXMA iP7100")) || !(strcmp(init->caps->name,"PIXMA iP8100")) || !(strcmp(init->caps->name,"PIXMA iP8500")) || !(strcmp(init->caps->name,"PIXMA iP8600")) || !(strcmp(init->caps->name,"PIXMA iP9910")) || !(strcmp(init->caps->name,"PIXMA MP900")) || !(strcmp(init->caps->name,"PIXMA Pro9000")) || !(strcmp(init->caps->name,"PIXMA Pro9002")) || !(strcmp(init->caps->name,"PIXMA Pro9500")) || !(strcmp(init->caps->name,"PIXMA Pro9502")) ) {
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
    else if ( (init->mode->flags & MODE_FLAG_CD) && ( !(strcmp(init->caps->name,"PIXMA iP4900")) || !(strcmp(init->caps->name,"PIXMA MG5200")) || !(strcmp(init->caps->name,"PIXMA MG5300")) || !(strcmp(init->caps->name,"PIXMA MG6200")) || !(strcmp(init->caps->name,"PIXMA MG8200")) ) ) {
      canon_cmd(v,ESC28,0x72, 1, 0x68);
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
    if  (!strcmp(init->caps->name,"3000") || !strcmp(init->caps->name,"4300")) {
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
  raster_channel_order=init->channel_order;
  if ( !(strcmp(init->caps->name,"PIXMA MP140")) || !(strcmp(init->caps->name,"PIXMA MP150")) || !(strcmp(init->caps->name,"PIXMA MP160")) || !(strcmp(init->caps->name,"PIXMA MP170")) || !(strcmp(init->caps->name,"PIXMA MP180")) || !(strcmp(init->caps->name,"PIXMA MP190")) || !(strcmp(init->caps->name,"PIXMA MP210")) || !(strcmp(init->caps->name,"PIXMA MP220")) || !(strcmp(init->caps->name,"PIXMA MP240")) || !(strcmp(init->caps->name,"PIXMA MP250")) || !(strcmp(init->caps->name,"PIXMA MP270")) || !(strcmp(init->caps->name,"PIXMA MP280")) || !(strcmp(init->caps->name,"PIXMA MP450")) || !(strcmp(init->caps->name,"PIXMA MP460")) || !(strcmp(init->caps->name,"PIXMA MP470")) || !(strcmp(init->caps->name,"PIXMA MP480")) || !(strcmp(init->caps->name,"PIXMA MP490")) || !(strcmp(init->caps->name,"PIXMA MP495")) || !(strcmp(init->caps->name,"PIXMA MX300")) || !(strcmp(init->caps->name,"PIXMA MX310")) || !(strcmp(init->caps->name,"PIXMA MX330")) || !(strcmp(init->caps->name,"PIXMA MX340")) || !(strcmp(init->caps->name,"PIXMA MX350")) || !(strcmp(init->caps->name,"PIXMA MX360")) || !(strcmp(init->caps->name,"PIXMA MX410")) || !(strcmp(init->caps->name,"PIXMA iP2700")) || !(strcmp(init->caps->name,"PIXMA MG2100")) )
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
  else if ( !(strcmp(init->caps->name,"PIXMA iP6210")) || !(strcmp(init->caps->name,"PIXMA iP6220")) || !(strcmp(init->caps->name,"PIXMA iP6310")) ) {
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
    stp_zfwrite((const char *)raster_channel_order,init->num_channels, 1, v);
  }
  else
    {
      stp_zfwrite((const char *)init->channel_order,init->num_channels, 1, v);
    }
}




static void
canon_init_printer(const stp_vars_t *v, const canon_privdata_t *init)
{
  unsigned int mytop;
  /* init printer */
  if (init->is_first_page) {
    canon_init_resetPrinter(v,init);       /* ESC [K */
    canon_init_setESC_M(v,init);           /* ESC (M */
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
  canon_init_setESC_P(v,init);           /* ESC (P */
  canon_init_setCartridge(v,init);       /* ESC (T */
  canon_init_setTray(v,init);            /* ESC (l */
  canon_init_setX72(v,init);             /* ESC (r */
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
	stp_dprintf(STP_DBG_CANON, v, "canon_setup_channel: privdata->num_channels %d\n", privdata->num_channels);
        /* create a new channel */
        privdata->channels = stp_realloc(privdata->channels,sizeof(canon_channel_t) * (privdata->num_channels + 1));
        privdata->channel_order = stp_realloc(privdata->channel_order,privdata->num_channels + 2);
        /* update channel order */
        privdata->channel_order[privdata->num_channels]=ink->channel;
	stp_dprintf(STP_DBG_CANON, v, "canon_setup_channel: ink->channel %c\n", ink->channel);
        privdata->channel_order[privdata->num_channels+1]='\0';
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







/* FIXME move this to printercaps */
#define CANON_CD_X 176
#define CANON_CD_Y 405

static void setup_page(stp_vars_t* v,canon_privdata_t* privdata){
  const char    *media_source = stp_get_string_parameter(v, "InputSlot");
  const char *cd_type = stp_get_string_parameter(v, "PageSize");
  int print_cd= (media_source && (!strcmp(media_source, "CD")));
  int           page_left,
                page_top,
                page_right,
                page_bottom;
  int hub_size = 0;

 
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
  privdata->out_width = stp_get_width(v);
  stp_deprintf(STP_DBG_CANON,"stp_get_width: privdata->out_width is %i\n",privdata->out_width);

  privdata->out_height = stp_get_height(v);

  internal_imageable_area(v, 0, &page_left, &page_right,
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
    privdata->top -= page_top;
    privdata->page_width = page_right - page_left;
    privdata->page_height = page_bottom - page_top;
  }

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
  const char    *duplex_mode =stp_get_string_parameter(v, "Duplex");
  int           page_number = stp_get_int_parameter(v, "PageNumber");
  const canon_cap_t * caps= canon_get_model_capabilities(v);
  const canon_modeuselist_t* mlist = &canon_MULTIPASS_MP610_modeuselist;
  const canon_modeuse_t* muse = NULL;
  int monocheck = 0;
  int colcheck = 0;
  int		y;		/* Looping vars */
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
  int           print_cd= (media_source && (!strcmp(media_source, "CD")));
  int           image_height;
#if 0
                image_width;
#endif
  double        k_upper, k_lower;
  unsigned char *cd_mask = NULL;
  double outer_r_sq = 0;
  double inner_r_sq = 0;
  unsigned char* weave_cols[4] ; /* TODO clean up weaving code to be more generic */

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
  if ( (!strcmp(caps->name,"PIXMA MP610")) ) { /* limited to MP610 for now */
    
    /* scroll through modeuse list to find media */
    for(i=0;i<mlist->count;i++){
      if(!strcmp(privdata.pt->name,mlist->modeuses[i].name)){
	muse = &mlist->modeuses[i];
	break;
      }
    }

    if ( !strcmp(stp_get_string_parameter(v, "InkSet"),"Black")) {
      /* check if there is any mode for that media with K-only inktype */
      /* if not, change it to "Both" */
      /* NOTE: User cannot force monochrome printing here, since that would require changing the Color Model */
      if (!(mlist->modeuses[i].use_flags & INKSET_BLACK)) {
	stp_set_string_parameter(v, "InkSet", "Both");	
      }
    }
    else if ( !strcmp(stp_get_string_parameter(v, "InkSet"),"Color")) {
      /* check if there is any mode for that media with no K in the inkset at all */
      /* if not, change it to "Both" */
      if (!(mlist->modeuses[i].use_flags & INKSET_COLOR)) {
	stp_set_string_parameter(v, "InkSet", "Both");	
      }
    } /* no restriction for "Both" yet --- note there are other cartridge types too! */

  } /* limited to MP610 for now */    

  /* get InkSet after adjustment */
  privdata.ink_set = stp_get_string_parameter(v, "InkSet");

  /* --- no current restrictions for Duplex setting --- */

  /* in particular, we do not constraint duplex printing to certain media */
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
  }
  else if (!strcmp(privdata.ink_set,"Color")) {
    if (strcmp(ink_type,"RGB")) {/* if ink_type is NOT set to RGB (CMY) yet */
      stp_dprintf(STP_DBG_CANON, v, "canon_do_print: InkSet Color, so InkType changed to RGB (CMY)\n");
      stp_set_string_parameter(v, "InkType", "RGB");
    }
  } /* no restriction for InkSet set to "Both" */

  /* --- make adjustments to mode --- */

  /* find the wanted print mode: NULL if not yet set */
  if (ERRPRINT)
    stp_erprintf("Calling get_current_parameter from canon_do_print routine (before default set)");
  privdata.mode = canon_get_current_mode(v);

  if(!privdata.mode) {
    privdata.mode = &caps->modelist->modes[caps->modelist->default_mode];
    /* then call get_current_mode again to sort out the correct matching of parameters and mode selection */
  if (ERRPRINT)
    stp_erprintf("Calling get_current_parameter from canon_do_print routine (after default set)");
    privdata.mode = canon_get_current_mode(v);
  }

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
    stp_set_string_parameter(v, "PrintingMode", "Color"); /* Gernot: added */

  setup_page(v,&privdata);

  image_height = stp_image_height(image);

#if 0
  image_width = stp_image_width(image);
#endif

  privdata.is_first_page = (page_number == 0);

 /*
  * Convert image size to printer resolution...
  */

  privdata.out_width  = privdata.mode->xdpi * privdata.out_width / 72;

  privdata.out_height = privdata.mode->ydpi * privdata.out_height / 72;

  privdata.left = privdata.mode->xdpi * privdata.left / 72;

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

  stp_scale_float_parameter(v, "Density", privdata.pt->base_density);
  stp_scale_float_parameter(v, "Density",privdata.mode->density);

  if (stp_get_float_parameter(v, "Density") > 1.0)
    stp_set_float_parameter(v, "Density", 1.0);

  if (privdata.used_inks == CANON_INK_K)
    stp_scale_float_parameter(v, "Gamma", 1.25);
  stp_scale_float_parameter( v, "Gamma", privdata.mode->gamma );

  stp_deprintf(STP_DBG_CANON,"density is %f\n",
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
  stp_deprintf(STP_DBG_CANON,"privdata.out_width is %i\n",privdata.out_width);
  stp_deprintf(STP_DBG_CANON,"privdata.length is %i\n",privdata.length);

  stp_dither_init(v, image, privdata.out_width, privdata.mode->xdpi, privdata.mode->ydpi);

  canon_setup_channels(v,&privdata);


  stp_deprintf(STP_DBG_CANON,
	       "canon: driver will use colors %s\n",privdata.channel_order);

  /* Allocate compression buffer */
  if(caps->features & CANON_CAP_I)
    /*privdata.comp_buf = stp_zalloc(privdata.buf_length_max * 2 * caps->raster_lines_per_block * privdata.num_channels); */
      privdata.comp_buf = stp_zalloc(privdata.buf_length_max * 2 * privdata.mode->raster_lines_per_block * privdata.num_channels); /* for multiraster we need to buffer 8 lines for every color */
  else
      privdata.comp_buf = stp_zalloc(privdata.buf_length_max * 2);
  /* Allocate fold buffer */
  privdata.fold_buf = stp_zalloc(privdata.buf_length_max);



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

       stp_deprintf(STP_DBG_CANON,"canon: adjust leftskip: old=%d,\n", privdata.left);
       privdata.left = (int)( (float)privdata.left * (float)privdata.physical_xdpi / (float)privdata.mode->xdpi ); /* adjust left margin */
       stp_deprintf(STP_DBG_CANON,"canon: adjust leftskip: new=%d,\n", privdata.left);

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
       stp_deprintf(STP_DBG_CANON,"canon: initializing weaving: nozzles=%d, nozzle_separation=%d,\n"
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
                                stp_compute_uncompressed_linewidth);
       privdata.last_pass_offset = 0;


       for(i=0;i<4;i++){
           int x;
           for(x=0;x<privdata.num_channels;x++){
               if(weave_color_order[i] == privdata.channel_order[x])
                   weave_cols[i] = privdata.channels[x].buf;
                   privdata.weave_bits[i] = privdata.channels[x].props->bits;
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
    stp_deprintf(STP_DBG_CANON,"\ncanon: flushing %d possibly delayed buffers\n",
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
  canon_end_job,
  NULL
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


/* fold, apply 5 pixel in 1 byte compression, pack tiff and return the compressed length */
static int canon_compress(stp_vars_t *v, canon_privdata_t *pd, unsigned char* line,int length,int offset,unsigned char* comp_buf,int bits, int ink_flags)
{
  unsigned char
    *in_ptr= line,
    *comp_ptr, *comp_data;
  int offset2,bitoffset;

  /* Don't send blank lines... */

  if (line[0] == 0 && memcmp(line, line + 1, (length * bits)  - 1) == 0)
    return 0;

  offset2 = offset / 8;
  bitoffset = offset % 8;

  /* fold lsb/msb pairs if drop modulation is active */


  if (bits==2) {
    int pixels_per_byte = 4;
    if(ink_flags & INK_FLAG_5pixel_in_1byte)
      pixels_per_byte = 5;

    stp_fold(line,length,pd->fold_buf);
    in_ptr= pd->fold_buf;
    length= (length*8/4); /* 4 pixels in 8bit */
    /* calculate the number of compressed bytes that can be sent directly */
    offset2 = offset / pixels_per_byte;
    /* calculate the number of (uncompressed) bits that have to be added to the raster data */
    bitoffset = (offset % pixels_per_byte) * 2;
  }
  else if (bits==3) {
    stp_fold_3bit_323(line,length,pd->fold_buf);
    in_ptr= pd->fold_buf;
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
  else if (bits==4) {
    stp_fold_4bit(line,length,pd->fold_buf);
    in_ptr= pd->fold_buf;
    length= (length*8)/2;
    offset2 = offset / 2; 
    bitoffset= offset % 2;
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
      stp_deprintf(STP_DBG_CANON,"SEVERE BUG IN print-canon.c::canon_write() "
	      "bitoffset=%d!!\n",bitoffset);
  }

    if(ink_flags & INK_FLAG_5pixel_in_1byte)
       length = pack_pixels(in_ptr,length);

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
	    int           *empty,       /* IO- Preceeding empty lines */
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

                  written += canon_write(v, pd, pd->caps,
                               (unsigned char *)(bufs[0].v[color] + line * linelength),
                               linelength, idx[color],
                               &(pd->emptylines), pd->out_width,
                               pd->left, pd->weave_bits[color],0);
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

