/*
 * "$Id$"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include <gimp-print/gimp-print-intl-internal.h>
#include "gimp-print-internal.h"
#include <string.h>
#include <assert.h>
#include "print-escp2.h"
#include "module.h"
#include "weave.h"

#ifdef __GNUC__
#define inline __inline__
#endif

#define OP_JOB_START 1
#define OP_JOB_PRINT 2
#define OP_JOB_END   4

#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct
{
  const char *attr_name;
  short bit_shift;
  short bit_width;
} escp2_printer_attr_t;

static const escp2_printer_attr_t escp2_printer_attrs[] =
{
  { "command_mode",		0, 4 },
  { "horizontal_zero_margin",	4, 1 },
  { "rollfeed",			5, 1 },
  { "variable_mode",		6, 1 },
  { "graymode",		 	7, 1 },
  { "vacuum",			8, 1 },
  { "fast_360",			9, 1 },
  { "send_zero_advance",       10, 1 },
  { "supports_ink_change",     11, 1 },
};

#define INCH(x)		(72 * x)

static const res_t *escp2_find_resolution(stp_const_vars_t v);

#define PARAMETER_INT(s)				\
{							\
  "escp2_" #s, "escp2_" #s, NULL,			\
  STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,	\
  STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1		\
}

#define PARAMETER_RAW(s)				\
{							\
  "escp2_" #s, "escp2_" #s, NULL,			\
  STP_PARAMETER_TYPE_RAW, STP_PARAMETER_CLASS_FEATURE,	\
  STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1		\
}

typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
  int color_only;
} float_param_t;

static const stp_parameter_t the_parameters[] =
{
#if 0
  {
    "AutoMode", N_("Automatic Printing Mode"),
    N_("Automatic printing mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
#endif
  /*
   * Don't check this parameter.  We may offer different settings for
   * different papers, but we need to be able to handle settings from PPD
   * files that don't have constraints set up.
   */
  {
    "Quality", N_("Print Quality"),
    N_("Print Quality"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 0
  },
  {
    "PageSize", N_("Page Size"),
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_PAGE_SIZE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
  {
    "MediaType", N_("Media Type"),
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
  {
    "InputSlot", N_("Media Source"),
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
  {
    "Resolution", N_("Resolution"),
    N_("Resolution of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, -1, 1
  },
  /*
   * Don't check this parameter.  We may offer different settings for
   * different ink sets, but we need to be able to handle settings from PPD
   * files that don't have constraints set up.
   */
  {
    "InkType", N_("Ink Type"),
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED2, 1, 1, -1, 0
  },
  {
    "InkSet", N_("Ink Set"),
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
  {
    "PrintingDirection", N_("Printing Direction"),
    N_("Printing direction (unidirectional is higher quality, but slower)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, -1, 1
  },
  {
    "FullBleed", N_("Full Bleed"),
    N_("Full Bleed"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
  {
    "AdjustDotsize", N_("Adjust dot size as necessary"),
    N_("Adjust dot size as necessary to achieve desired density"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED4, 1, 1, -1, 1
  },
  {
    "OutputOrder", N_("Output Order"),
    N_("Output Order"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 0, 0, -1, 0
  },
  {
    "AlignmentPasses", N_("Alignment Passes"),
    N_("Alignment Passes"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED5, 0, 0, -1, 0
  },
  {
    "AlignmentChoices", N_("Alignment Choices"),
    N_("Alignment Choices"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED5, 0, 0, -1, 0
  },
  {
    "InkChange", N_("Ink change command"),
    N_("Ink change command"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED5, 0, 0, -1, 0
  },
  {
    "AlternateAlignmentPasses", N_("Alternate Alignment Passes"),
    N_("Alternate Alignment Passes"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED5, 0, 0, -1, 0
  },
  {
    "AlternateAlignmentChoices", N_("Alternate Alignment Choices"),
    N_("Alternate Alignment Choices"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED5, 0, 0, -1, 0
  },
  {
    "InkChannels", N_("Ink Channels"),
    N_("Ink Channels"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED5, 0, 0, -1, 0
  },
  PARAMETER_INT(max_hres),
  PARAMETER_INT(max_vres),
  PARAMETER_INT(min_hres),
  PARAMETER_INT(min_vres),
  PARAMETER_INT(nozzles),
  PARAMETER_INT(black_nozzles),
  PARAMETER_INT(fast_nozzles),
  PARAMETER_INT(min_nozzles),
  PARAMETER_INT(min_black_nozzles),
  PARAMETER_INT(min_fast_nozzles),
  PARAMETER_INT(nozzle_separation),
  PARAMETER_INT(black_nozzle_separation),
  PARAMETER_INT(fast_nozzle_separation),
  PARAMETER_INT(separation_rows),
  PARAMETER_INT(max_paper_width),
  PARAMETER_INT(max_paper_height),
  PARAMETER_INT(min_paper_width),
  PARAMETER_INT(min_paper_height),
  PARAMETER_INT(extra_feed),
  PARAMETER_INT(pseudo_separation_rows),
  PARAMETER_INT(base_separation),
  PARAMETER_INT(resolution_scale),
  PARAMETER_INT(initial_vertical_offset),
  PARAMETER_INT(black_initial_vertical_offset),
  PARAMETER_INT(max_black_resolution),
  PARAMETER_INT(zero_margin_offset),
  PARAMETER_INT(extra_720dpi_separation),
  PARAMETER_INT(physical_channels),
  PARAMETER_INT(left_margin),
  PARAMETER_INT(right_margin),
  PARAMETER_INT(top_margin),
  PARAMETER_INT(bottom_margin),
  PARAMETER_INT(alignment_passes),
  PARAMETER_INT(alignment_choices),
  PARAMETER_INT(alternate_alignment_passes),
  PARAMETER_INT(alternate_alignment_choices),
  PARAMETER_RAW(preinit_sequence),
  PARAMETER_RAW(postinit_remote_sequence)
};

static const int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

static const float_param_t float_parameters[] =
{
  {
    {
      "CyanDensity", N_("Cyan Balance"),
      N_("Adjust the cyan balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 1, 1
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "MagentaDensity", N_("Magenta Balance"),
      N_("Adjust the magenta balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 2, 1
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "YellowDensity", N_("Yellow Balance"),
      N_("Adjust the yellow balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 3, 1
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "BlackDensity", N_("Black Balance"),
      N_("Adjust the black balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 0, 1
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "LightCyanTransition", N_("Light Cyan Transition"),
      N_("Light Cyan Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightMagentaTransition", N_("Light Magenta Transition"),
      N_("Light Magenta Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "DarkYellowTransition", N_("Dark Yellow Transition"),
      N_("Dark Yellow Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "GrayTransition", N_("Gray Transition"),
      N_("Gray Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray3Transition", N_("Dark Gray Transition"),
      N_("Dark Gray Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray2Transition", N_("Mid Gray Transition"),
      N_("Medium Gray Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "Gray1Transition", N_("Light Gray Transition"),
      N_("Light Gray Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1
    }, 0.0, 5.0, 1.0, 1
  },
};

static const int float_parameter_count =
sizeof(float_parameters) / sizeof(const float_param_t);


static escp2_privdata_t *
get_privdata(stp_vars_t v)
{
  return (escp2_privdata_t *) stpi_get_component_data(v, "Driver");
}

static model_featureset_t
escp2_get_cap(stp_const_vars_t v, escp2_model_option_t feature)
{
  int model = stpi_get_model_id(v);
  if (feature < 0 || feature >= MODEL_LIMIT)
    return (model_featureset_t) -1;
  else
    {
      model_featureset_t featureset =
	(((1ul << escp2_printer_attrs[feature].bit_width) - 1ul) <<
	 escp2_printer_attrs[feature].bit_shift);
      return stpi_escp2_model_capabilities[model].flags & featureset;
    }
}

static int
escp2_has_cap(stp_const_vars_t v, escp2_model_option_t feature,
	      model_featureset_t class)
{
  int model = stpi_get_model_id(v);
  if (feature < 0 || feature >= MODEL_LIMIT)
    return -1;
  else
    {
      model_featureset_t featureset =
	(((1ul << escp2_printer_attrs[feature].bit_width) - 1ul) <<
	 escp2_printer_attrs[feature].bit_shift);
      if ((stpi_escp2_model_capabilities[model].flags & featureset) == class)
	return 1;
      else
	return 0;
    }
}

#define DEF_SIMPLE_ACCESSOR(f, t)					\
static inline t								\
escp2_##f(stp_const_vars_t v)						\
{									\
  if (stp_check_int_parameter(v, "escp2_" #f, STP_PARAMETER_ACTIVE))	\
    return stp_get_int_parameter(v, "escp2_" #f);			\
  else									\
    {									\
      int model = stpi_get_model_id(v);					\
      return (stpi_escp2_model_capabilities[model].f);			\
    }									\
}

#define DEF_RAW_ACCESSOR(f, t)						\
static inline t								\
escp2_##f(stp_const_vars_t v)						\
{									\
  if (stp_check_raw_parameter(v, "escp2_" #f, STP_PARAMETER_ACTIVE))	\
    return stp_get_raw_parameter(v, "escp2_" #f);			\
  else									\
    {									\
      int model = stpi_get_model_id(v);					\
      return (stpi_escp2_model_capabilities[model].f);			\
    }									\
}

#define DEF_COMPOSITE_ACCESSOR(f, t)			\
static inline t						\
escp2_##f(stp_const_vars_t v)				\
{							\
  int model = stpi_get_model_id(v);			\
  return (stpi_escp2_model_capabilities[model].f);	\
}

#define DEF_ROLL_ACCESSOR(f, t)						\
static inline t								\
escp2_##f(stp_const_vars_t v, int rollfeed)				\
{									\
  if (stp_check_int_parameter(v, "escp2_" #f, STP_PARAMETER_ACTIVE))	\
    return stp_get_int_parameter(v, "escp2_" #f);			\
  else									\
    {									\
      int model = stpi_get_model_id(v);					\
      const res_t *res = escp2_find_resolution(v);			\
      if (res && !(res->softweave))					\
	{								\
	  if (rollfeed)							\
	    return (stpi_escp2_model_capabilities[model].m_roll_##f);	\
	  else								\
	    return (stpi_escp2_model_capabilities[model].m_##f);	\
	}								\
      else								\
	{								\
	  if (rollfeed)							\
	    return (stpi_escp2_model_capabilities[model].roll_##f);	\
	  else								\
	    return (stpi_escp2_model_capabilities[model].f);		\
	}								\
    }									\
}

DEF_SIMPLE_ACCESSOR(max_hres, int)
DEF_SIMPLE_ACCESSOR(max_vres, int)
DEF_SIMPLE_ACCESSOR(min_hres, int)
DEF_SIMPLE_ACCESSOR(min_vres, int)
DEF_SIMPLE_ACCESSOR(nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(black_nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(fast_nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(min_nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(min_black_nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(min_fast_nozzles, unsigned)
DEF_SIMPLE_ACCESSOR(nozzle_separation, unsigned)
DEF_SIMPLE_ACCESSOR(black_nozzle_separation, unsigned)
DEF_SIMPLE_ACCESSOR(fast_nozzle_separation, unsigned)
DEF_SIMPLE_ACCESSOR(separation_rows, unsigned)
DEF_SIMPLE_ACCESSOR(max_paper_width, unsigned)
DEF_SIMPLE_ACCESSOR(max_paper_height, unsigned)
DEF_SIMPLE_ACCESSOR(min_paper_width, unsigned)
DEF_SIMPLE_ACCESSOR(min_paper_height, unsigned)
DEF_SIMPLE_ACCESSOR(extra_feed, unsigned)
DEF_SIMPLE_ACCESSOR(pseudo_separation_rows, int)
DEF_SIMPLE_ACCESSOR(base_separation, int)
DEF_SIMPLE_ACCESSOR(resolution_scale, int)
DEF_SIMPLE_ACCESSOR(initial_vertical_offset, int)
DEF_SIMPLE_ACCESSOR(black_initial_vertical_offset, int)
DEF_SIMPLE_ACCESSOR(max_black_resolution, int)
DEF_SIMPLE_ACCESSOR(zero_margin_offset, int)
DEF_SIMPLE_ACCESSOR(extra_720dpi_separation, int)
DEF_SIMPLE_ACCESSOR(physical_channels, int)
DEF_SIMPLE_ACCESSOR(alignment_passes, int)
DEF_SIMPLE_ACCESSOR(alignment_choices, int)
DEF_SIMPLE_ACCESSOR(alternate_alignment_passes, int)
DEF_SIMPLE_ACCESSOR(alternate_alignment_choices, int)

DEF_ROLL_ACCESSOR(left_margin, unsigned)
DEF_ROLL_ACCESSOR(right_margin, unsigned)
DEF_ROLL_ACCESSOR(top_margin, unsigned)
DEF_ROLL_ACCESSOR(bottom_margin, unsigned)

DEF_RAW_ACCESSOR(preinit_sequence, const stp_raw_t *)
DEF_RAW_ACCESSOR(postinit_remote_sequence, const stp_raw_t *)

DEF_COMPOSITE_ACCESSOR(reslist, const res_t *const *)
DEF_COMPOSITE_ACCESSOR(inkgroup, const inkgroup_t *)
DEF_COMPOSITE_ACCESSOR(input_slots, const input_slot_list_t *)
DEF_COMPOSITE_ACCESSOR(quality_list, const quality_list_t *)

static int
escp2_ink_type(stp_const_vars_t v, int resid)
{
  if (stp_check_int_parameter(v, "escp2_ink_type", STP_PARAMETER_ACTIVE))
    return stp_get_int_parameter(v, "escp2_ink_type");
  else
    {
      int model = stpi_get_model_id(v);
      return stpi_escp2_model_capabilities[model].dot_sizes[resid];
    }
}

static double
escp2_density(stp_const_vars_t v, int resid)
{
  if (stp_check_float_parameter(v, "escp2_density", STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter(v, "escp2_density");
  else
    {
      int model = stpi_get_model_id(v);
      return stpi_escp2_model_capabilities[model].densities[resid];
    }
}

static int
escp2_bits(stp_const_vars_t v, int resid)
{
  if (stp_check_int_parameter(v, "escp2_bits", STP_PARAMETER_ACTIVE))
    return stp_get_int_parameter(v, "escp2_bits");
  else
    {
      int model = stpi_get_model_id(v);
      return stpi_escp2_model_capabilities[model].bits[resid];
    }
}

static double
escp2_base_res(stp_const_vars_t v, int resid)
{
  if (stp_check_float_parameter(v, "escp2_base_res", STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter(v, "escp2_base_res");
  else
    {
      int model = stpi_get_model_id(v);
      return stpi_escp2_model_capabilities[model].base_resolutions[resid];
    }
}

static const escp2_dropsize_t *
escp2_dropsizes(stp_const_vars_t v, int resid)
{
  int model = stpi_get_model_id(v);
  const escp2_drop_list_t *drops = stpi_escp2_model_capabilities[model].drops;
  return (*drops)[resid];
}

static const inklist_t *
escp2_inklist(stp_const_vars_t v)
{
  int model = stpi_get_model_id(v);
  int i;
  const char *ink_list_name = NULL;
  const inkgroup_t *inkgroup = stpi_escp2_model_capabilities[model].inkgroup;

  if (stp_check_string_parameter(v, "InkSet", STP_PARAMETER_ACTIVE))
    ink_list_name = stp_get_string_parameter(v, "InkSet");
  if (ink_list_name)
    {
      for (i = 0; i < inkgroup->n_inklists; i++)
	{
	  if (strcmp(ink_list_name, inkgroup->inklists[i]->name) == 0)
	    return inkgroup->inklists[i];
	}
    }
  return inkgroup->inklists[0];
}

static const shade_t *
escp2_shades(stp_const_vars_t v, int channel)
{
  const inklist_t *inklist = escp2_inklist(v);
  return &((*inklist->shades)[channel]);
}

static const paperlist_t *
escp2_paperlist(stp_const_vars_t v)
{
  const inklist_t *inklist = escp2_inklist(v);
  if (inklist)
    return inklist->papers;
  else
    return NULL;
}

static int
using_automatic_settings(stp_const_vars_t v, auto_mode_t mode)
{
  switch (mode)
    {
    case AUTO_MODE_QUALITY:
      if (stp_check_string_parameter(v, "Quality", STP_PARAMETER_ACTIVE) &&
	  strcmp(stp_get_string_parameter(v, "Quality"), "None") != 0 &&
	  stp_get_output_type(v) != OUTPUT_RAW_PRINTER)
	return 1;
      else
	return 0;
#if 0
    case AUTO_MODE_FULL_AUTO:
      if (stp_check_string_parameter(v, "AutoMode", STP_PARAMETER_ACTIVE) &&
	  strcmp(stp_get_string_parameter(v, "AutoMode"), "None") != 0 &&
	  stp_get_output_type(v) != OUTPUT_RAW_PRINTER)
	return 1;
      else
	return 0;
#endif
    case AUTO_MODE_MANUAL:
      if (!stp_check_string_parameter(v, "Quality", STP_PARAMETER_ACTIVE) ||
	  strcmp(stp_get_string_parameter(v, "Quality"), "None") == 0 ||
	  stp_get_output_type(v) == OUTPUT_RAW_PRINTER)
	return 1;
      else
	return 0;
    }
  return 0;
}    

static int
compute_internal_resid(int hres, int vres)
{
  static const int resolutions[RES_N] =
    {
      0,
      360 * 360,
      720 * 360,
      720 * 720,
      1440 * 720,
      1440 * 1440,
      2880 * 1440,
      2880 * 2880,
    };
  int total_resolution = hres * vres;
  int i;
  for (i = 0; i < RES_N; i++)
    {
      if (total_resolution < resolutions[i])
	return i - 1;
    }
  return RES_N - 1;
}  

static int
compute_resid(const res_t *res)
{
  return compute_internal_resid(res->hres, res->vres);
}


static const input_slot_t *
get_input_slot(stp_const_vars_t v)
{
  int i;
  const char *input_slot = stp_get_string_parameter(v, "InputSlot");
  if (input_slot && strlen(input_slot) > 0)
    {
      const input_slot_list_t *slots = escp2_input_slots(v);
      if (slots)
	{
	  for (i = 0; i < slots->n_input_slots; i++)
	    {
	      if (slots->slots[i].name &&
		  strcmp(input_slot, slots->slots[i].name) == 0)
		{
		  return &(slots->slots[i]);
		  break;
		}
	    }
	}
    }
  return NULL;
}

static const paper_t *
get_media_type(stp_const_vars_t v)
{
  int i;
  const paperlist_t *p = escp2_paperlist(v);
  if (p)
    {
      const char *name = stp_get_string_parameter(v, "MediaType");
      int paper_type_count = p->paper_count;
      if (name)
	{
	  for (i = 0; i < paper_type_count; i++)
	    {
	      if (!strcmp(name, p->papers[i].name))
		return &(p->papers[i]);
	    }
	}
    }
  return NULL;
}

static int
verify_resolution_by_paper_type(stp_const_vars_t v, const res_t *res)
{
  const paper_t *paper = get_media_type(v);
  if (paper)
    {
      switch (paper->paper_class)
	{
	case PAPER_PLAIN:
	  if (res->vres > 720 || res->hres > 720)
	    return 0;
	  break;
	case PAPER_GOOD:
	  if (res->vres < 180 || res->hres < 360 ||
	      res->vres > 720 || res->hres > 1440)
	    return 0;
	  break;
	case PAPER_PHOTO:
	  if (res->vres < 360 || 
	      (res->hres < 720 && res->hres < escp2_max_hres(v)))
	    return 0;
	  break;
	case PAPER_PREMIUM_PHOTO:
	  if (res->vres < 720 ||
	      (res->hres < 720 && res->hres < escp2_max_hres(v)))
	    return 0;
	  break;
	case PAPER_TRANSPARENCY:
	  if (res->vres < 360 || res->hres < 360 ||
	      res->vres > 720 || res->hres > 720)
	    return 0;
	  break;
	}
    }
  return 1;
}

static int
verify_resolution(stp_const_vars_t v, const res_t *res)
{
  int nozzle_width =
    (escp2_base_separation(v) / escp2_nozzle_separation(v));
  int nozzles = escp2_nozzles(v);
  int resid = compute_resid(res);
  if (escp2_ink_type(v, resid) != -1 &&
      res->vres <= escp2_max_vres(v) &&
      res->hres <= escp2_max_hres(v) &&
      res->vres >= escp2_min_vres(v) &&
      res->hres >= escp2_min_hres(v) &&
      (nozzles == 1 ||
       ((res->vres / nozzle_width) * nozzle_width) == res->vres))
    {
      int xdpi = res->hres;
      int physical_xdpi = escp2_base_res(v, resid);
      int horizontal_passes, oversample;
      if (physical_xdpi > xdpi)
	physical_xdpi = xdpi;
      horizontal_passes = xdpi / physical_xdpi;
      oversample = horizontal_passes * res->vertical_passes;
      if (horizontal_passes < 1)
	horizontal_passes = 1;
      if (oversample < 1)
	oversample = 1;
      if (((horizontal_passes * res->vertical_passes) <= 8) &&
	  (! res->softweave || (nozzles > 1 && nozzles > oversample)))
	return 1;
    }
  return 0;
}

static int
verify_papersize(stp_const_vars_t v, const stp_papersize_t *pt)
{
  unsigned int height_limit, width_limit;
  unsigned int min_height_limit, min_width_limit;
  width_limit = escp2_max_paper_width(v);
  height_limit = escp2_max_paper_height(v);
  min_width_limit = escp2_min_paper_width(v);
  min_height_limit = escp2_min_paper_height(v);
  if (strlen(pt->name) > 0 &&
      pt->width <= width_limit && pt->height <= height_limit &&
      (pt->height >= min_height_limit || pt->height == 0) &&
      (pt->width >= min_width_limit || pt->width == 0) &&
      (pt->width == 0 || pt->height > 0 ||
       escp2_has_cap(v, MODEL_ROLLFEED, MODEL_ROLLFEED_YES)))
    return 1;
  else
    return 0;
}

static int
verify_inktype(stp_const_vars_t v, const escp2_inkname_t *inks)
{
  if (inks->inkset == INKSET_EXTENDED)
    return 0;
  else
    return 1;
}

static const char *
get_default_inktype(stp_const_vars_t v)
{
  const inklist_t *ink_list = escp2_inklist(v);
  const paper_t *paper_type = get_media_type(v);
  if (!ink_list)
    return NULL;
  if (!paper_type)
    {
      const paperlist_t *p = escp2_paperlist(v);
      if (p)
	paper_type = &(p->papers[0]);
    }
  if (paper_type && paper_type->preferred_ink_type)
    return paper_type->preferred_ink_type;
  else if (escp2_has_cap(v, MODEL_FAST_360, MODEL_FAST_360_YES) &&
	   stp_check_string_parameter(v, "Resolution", STP_PARAMETER_ACTIVE))
    {
      const res_t *res = escp2_find_resolution(v);
      if (res)
	{
	  int resid = compute_resid(res);
	  if (res->vres == 360 && res->hres == escp2_base_res(v, resid))
	    {
	      int i;
	      for (i = 0; i < ink_list->n_inks; i++)
		if (strcmp(ink_list->inknames[i]->name, "CMYK") == 0)
		  return ink_list->inknames[i]->name;
	    }
	}
    }
  return ink_list->inknames[0]->name;
}


static const escp2_inkname_t *
get_inktype(stp_const_vars_t v)
{
  const char	*ink_type = stp_get_string_parameter(v, "InkType");
  const inklist_t *ink_list = escp2_inklist(v);
  int i;

  if (!ink_type || strcmp(ink_type, "None") == 0 ||
      (ink_list && ink_list->n_inks == 1) ||
      !using_automatic_settings(v, AUTO_MODE_MANUAL))
    ink_type = get_default_inktype(v);

  if (ink_type && ink_list)
    {
      for (i = 0; i < ink_list->n_inks; i++)
	{
	  if (strcmp(ink_type, ink_list->inknames[i]->name) == 0)
	    return ink_list->inknames[i];
	}
    }
  return NULL;
}

static const paper_adjustment_t *
get_media_adjustment(stp_const_vars_t v)
{
  const paper_t *pt = get_media_type(v);
  const inklist_t *ink_list = escp2_inklist(v);
  if (pt && ink_list && ink_list->paper_adjustments)
    {
      const paper_adjustment_list_t *adjlist = ink_list->paper_adjustments;
      const char *paper_name = pt->name;
      int i;
      for (i = 0; i < adjlist->paper_count; i++)
	{
	  if (strcmp(paper_name, adjlist->papers[i].name) == 0)
	    return &(adjlist->papers[i]);
	}
    }
  return NULL;
}


/*
 * 'escp2_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
escp2_list_parameters(stp_const_vars_t v)
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
fill_transition_parameters(stp_parameter_t *description)
{
  description->is_active = 1;
  description->bounds.dbl.lower = 0;
  description->bounds.dbl.upper = 5.0;
  description->deflt.dbl = 1.0;
}

static void
set_density_parameter(stp_const_vars_t v,
		      stp_parameter_t *description,
		      int color)
{
  if (stp_get_output_type(v) != OUTPUT_GRAY &&
      using_automatic_settings(v, AUTO_MODE_MANUAL))
    description->is_active = 1;
  else
    description->is_active = 0;
}  

static void
set_gray_transition_parameter(stp_const_vars_t v,
			      stp_parameter_t *description,
			      int expected_channels)
{
  const escp2_inkname_t *ink_name = get_inktype(v);
  description->is_active = 0;
  if (ink_name && ink_name->channel_set->channels[ECOLOR_K] &&
      (ink_name->channel_set->channels[ECOLOR_K]->n_subchannels ==
       expected_channels) &&
      using_automatic_settings(v, AUTO_MODE_MANUAL))
    fill_transition_parameters(description);
}

static void
set_color_transition_parameter(stp_const_vars_t v,
			       stp_parameter_t *description,
			       int color)
{
  description->is_active = 0;
  if (stp_get_output_type(v) != OUTPUT_GRAY &&
      using_automatic_settings(v, AUTO_MODE_MANUAL))
    {
      const escp2_inkname_t *ink_name = get_inktype(v);
      if (ink_name &&
	  ink_name->channel_set->channel_count == 4 &&
	  ink_name->channel_set->channels[color] &&
	  ink_name->channel_set->channels[color]->n_subchannels == 2)
	fill_transition_parameters(description);
    }
}

static const res_t *
find_default_resolution(stp_const_vars_t v, int desired_hres, int desired_vres,
			int strict)
{
  const res_t *const *res = escp2_reslist(v);
  int i = 0;
  if (desired_hres < 0)
    {
      while (res[i])
	i++;
      i--;
      while (i >= 0)
	{
	  if (verify_resolution(v, res[i]) &&
	      verify_resolution_by_paper_type(v, res[i]))
	    return res[i];
	  i--;
	}
    }
  i = 0;
  while (res[i])
    {
      if (verify_resolution(v, res[i]) &&
	  verify_resolution_by_paper_type(v, res[i]) &&
	  res[i]->vres >= desired_vres && res[i]->hres >= desired_hres &&
	  res[i]->vres <= 2 * desired_vres && res[i]->hres <= 2 * desired_hres)
	return res[i];
      i++;
    }
#if 0
  if (!strict)			/* Try again to find a match */
    {
      i = 0;
      while (res[i])
	{
	  if (verify_resolution(v, res[i]) &&
	      res[i]->vres >= desired_vres &&
	      res[i]->hres >= desired_hres &&
	      res[i]->vres <= 2 * desired_vres &&
	      res[i]->hres <= 2 * desired_hres)
	    return res[i];
	  i++;
	}
    }    
#endif
  return NULL;
}

static const res_t *
find_resolution_from_quality(stp_const_vars_t v, const char *quality,
			     int strict)
{
  int i;
  const quality_list_t *quals = escp2_quality_list(v);
  for (i = 0; i < quals->n_quals; i++)
    {
      const quality_t *q = &(quals->qualities[i]);
      if (strcmp(quality, q->name) == 0 &&
	  (q->min_vres == 0 || escp2_min_vres(v) <= q->min_vres) &&
	  (q->min_hres == 0 || escp2_min_hres(v) <= q->min_hres) &&
	  (q->max_vres == 0 || escp2_max_vres(v) >= q->max_vres) &&
	  (q->max_hres == 0 || escp2_max_hres(v) >= q->max_hres))
	{
	  return find_default_resolution(v, q->desired_hres, q->desired_vres,
					 strict);
	}
    }
  return NULL;
}

static void
escp2_parameters(stp_const_vars_t v, const char *name,
		 stp_parameter_t *description)
{
  int		i;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  for (i = 0; i < float_parameter_count; i++)
    if (strcmp(name, float_parameters[i].param.name) == 0)
      {
	stpi_fill_parameter_settings(description,
				     &(float_parameters[i].param));
	description->deflt.dbl = float_parameters[i].defval;
	description->bounds.dbl.upper = float_parameters[i].max;
	description->bounds.dbl.lower = float_parameters[i].min;
	break;
      }

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stpi_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }

  description->deflt.str = NULL;
  if (strcmp(name, "AutoMode") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string(description->bounds.str, "None",
				 _("Full Manual Control"));
      stp_string_list_add_string(description->bounds.str, "Auto",
				 _("Automatic Setting Control"));
      description->deflt.str = "None"; /* so CUPS and Foomatic don't break */
    }      
  else if (strcmp(name, "PageSize") == 0)
    {
      int papersizes = stp_known_papersizes();
      description->bounds.str = stp_string_list_create();
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t *pt = stp_get_papersize_by_index(i);
	  if (verify_papersize(v, pt))
	    stp_string_list_add_string(description->bounds.str,
				       pt->name, pt->text);
	}
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "Quality") == 0)
    {
      const quality_list_t *quals = escp2_quality_list(v);
      int has_standard_quality = 0;
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string(description->bounds.str, "None",
				 _("Manual Control"));
      for (i = 0; i < quals->n_quals; i++)
	{
	  const quality_t *q = &(quals->qualities[i]);
	  if (((q->min_vres == 0 || escp2_min_vres(v) <= q->min_vres) &&
	       (q->min_hres == 0 || escp2_min_hres(v) <= q->min_hres) &&
	       (q->max_vres == 0 || escp2_max_vres(v) >= q->max_vres) &&
	       (q->max_hres == 0 || escp2_max_hres(v) >= q->max_hres)) &&
	      (find_resolution_from_quality(v, q->name, 1) ||
	       (!stp_check_string_parameter(v, "MediaType",
					    STP_PARAMETER_ACTIVE))))
	    stp_string_list_add_string(description->bounds.str, q->name,
				       q->text);
	  if (strcmp(q->name, "Standard") == 0)
	    has_standard_quality = 1;
	}
      if (has_standard_quality)
	description->deflt.str = "Standard";
      else
	description->deflt.str = "None";
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      const res_t *const *res = escp2_reslist(v);
      const res_t *defval = find_default_resolution(v, 720, 360, 0);
      description->bounds.str = stp_string_list_create();
      i = 0;
      while (res[i])
	{
	  if (verify_resolution(v, res[i]) &&
	      (using_automatic_settings(v, AUTO_MODE_MANUAL) ||
	       !stp_check_string_parameter(v, "MediaType",
					   STP_PARAMETER_ACTIVE) ||
	       verify_resolution_by_paper_type(v, res[i])))
	    stp_string_list_add_string(description->bounds.str,
				       res[i]->name, _(res[i]->text));
	  i++;
	}
      if (defval)
	description->deflt.str = defval->name;
      else
	description->deflt.str = res[0]->name;
      if (!using_automatic_settings(v, AUTO_MODE_MANUAL))
	description->is_active = 0;
    }
  else if (strcmp(name, "InkType") == 0)
    {
      const inklist_t *inks = escp2_inklist(v);
      int ninktypes = inks->n_inks;
      description->bounds.str = stp_string_list_create();
      if (ninktypes > 1)
	{
	  stp_string_list_add_string(description->bounds.str, "None",
				     _("Standard"));
	  for (i = 0; i < ninktypes; i++)
	    if (verify_inktype(v, inks->inknames[i]))
	      stp_string_list_add_string(description->bounds.str,
					 inks->inknames[i]->name,
					 _(inks->inknames[i]->text));
	  description->deflt.str = "None";
	}
      if (ninktypes <= 1 || !using_automatic_settings(v, AUTO_MODE_MANUAL))
	description->is_active = 0;
    }
  else if (strcmp(name, "InkSet") == 0)
    {
      const inkgroup_t *inks = escp2_inkgroup(v);
      int ninklists = inks->n_inklists;
      description->bounds.str = stp_string_list_create();
      if (ninklists > 1)
	{
	  int has_default_choice = 0;
	  for (i = 0; i < ninklists; i++)
	    {
	      stp_string_list_add_string(description->bounds.str,
					 inks->inklists[i]->name,
					 _(inks->inklists[i]->text));
	      if (strcmp(inks->inklists[i]->name, "None") == 0)
		has_default_choice = 1;
	    }
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	  if (!using_automatic_settings(v, AUTO_MODE_MANUAL) &&
	      has_default_choice)
	    description->is_active = 0;
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      const paperlist_t *p = escp2_paperlist(v);
      int nmediatypes = p->paper_count;
      description->bounds.str = stp_string_list_create();
      if (nmediatypes)
	{
	  for (i = 0; i < nmediatypes; i++)
	    stp_string_list_add_string(description->bounds.str,
				       p->papers[i].name,
				       _(p->papers[i].text));
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "InputSlot") == 0)
    {
      const input_slot_list_t *slots = escp2_input_slots(v);
      int ninputslots = slots->n_input_slots;
      description->bounds.str = stp_string_list_create();
      if (ninputslots)
	{
	  for (i = 0; i < ninputslots; i++)
	    stp_string_list_add_string(description->bounds.str,
				       slots->slots[i].name,
				       _(slots->slots[i].text));
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "PrintingDirection") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string
	(description->bounds.str, "None", _("Automatic"));
      stp_string_list_add_string
	(description->bounds.str, "Bidirectional", _("Bidirectional"));
      stp_string_list_add_string
	(description->bounds.str, "Unidirectional", _("Unidirectional"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
      if (!using_automatic_settings(v, AUTO_MODE_MANUAL))
	description->is_active = 0;
    }
  else if (strcmp(name, "OutputOrder") == 0)
    {
      description->bounds.str = stp_string_list_create();
      description->deflt.str = "Reverse";
    }
  else if (strcmp(name, "FullBleed") == 0)
    {
      if (escp2_has_cap(v, MODEL_XZEROMARGIN, MODEL_XZEROMARGIN_YES))
	description->deflt.boolean = 0;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "AdjustDotsize") == 0)
    {
      description->deflt.boolean = 0;
      if (!using_automatic_settings(v, AUTO_MODE_MANUAL))
	description->is_active = 0;
    }
  else if (strcmp(name, "CyanDensity") == 0)
    set_density_parameter(v, description, ECOLOR_C);
  else if (strcmp(name, "MagentaDensity") == 0)
    set_density_parameter(v, description, ECOLOR_M);
  else if (strcmp(name, "YellowDensity") == 0)
    set_density_parameter(v, description, ECOLOR_Y);
  else if (strcmp(name, "BlackDensity") == 0)
    set_density_parameter(v, description, ECOLOR_K);
  else if (strcmp(name, "GrayTransition") == 0)
    set_gray_transition_parameter(v, description, 2);
  else if (strcmp(name, "Gray1Transition") == 0 ||
	   strcmp(name, "Gray2Transition") == 0 ||
	   strcmp(name, "Gray3Transition") == 0)
    set_gray_transition_parameter(v, description, 4);
  else if (strcmp(name, "LightCyanTransition") == 0)
    set_color_transition_parameter(v, description, ECOLOR_C);
  else if (strcmp(name, "LightMagentaTransition") == 0)
    set_color_transition_parameter(v, description, ECOLOR_M);
  else if (strcmp(name, "DarkYellowTransition") == 0)
    set_color_transition_parameter(v, description, ECOLOR_Y);
  else if (strcmp(name, "AlignmentPasses") == 0)
    {
      description->deflt.integer = escp2_alignment_passes(v);
      description->bounds.integer.lower = -1;
      description->bounds.integer.upper = -2;
    }
  else if (strcmp(name, "AlignmentChoices") == 0)
    {
      description->deflt.integer = escp2_alignment_choices(v);
      description->bounds.integer.lower = -1;
      description->bounds.integer.upper = -2;
    }
  else if (strcmp(name, "SupportsInkChange") == 0)
    {
      description->deflt.integer =
	escp2_has_cap(v, MODEL_SUPPORTS_INK_CHANGE,
		      MODEL_SUPPORTS_INK_CHANGE_YES);
      description->bounds.integer.lower = -1;
      description->bounds.integer.upper = -2;
    }
  else if (strcmp(name, "AlternateAlignmentPasses") == 0)
    {
      description->deflt.integer = escp2_alternate_alignment_passes(v);
      description->bounds.integer.lower = -1;
      description->bounds.integer.upper = -2;
    }
  else if (strcmp(name, "AlternateAlignmentChoices") == 0)
    {
      description->deflt.integer = escp2_alternate_alignment_choices(v);
      description->bounds.integer.lower = -1;
      description->bounds.integer.upper = -2;
    }
  else if (strcmp(name, "InkChannels") == 0)
    {
      description->deflt.integer = escp2_physical_channels(v);
      description->bounds.integer.lower = -1;
      description->bounds.integer.upper = -2;
    }
}

static const res_t *
escp2_find_resolution(stp_const_vars_t v)
{
  const char *resolution;
  if (stp_check_string_parameter(v, "Quality", STP_PARAMETER_ACTIVE))
    {
      const res_t *default_res =
	find_resolution_from_quality(v, stp_get_string_parameter(v, "Quality"),
				     0);
      if (default_res)
	{
	  stpi_dprintf(STPI_DBG_ESCP2, v,
		       "Setting resolution to %s from quality %s\n",
		       default_res->name,
		       stp_get_string_parameter(v, "Quality"));
	  return default_res;
	}
      else
	stpi_dprintf(STPI_DBG_ESCP2, v, "Unable to map quality %s\n",
		     stp_get_string_parameter(v, "Quality"));
    }
  resolution = stp_get_string_parameter(v, "Resolution");
  if (resolution)
    {
      const res_t *const *res = escp2_reslist(v);
      int i = 0;
      while (res[i])
	{
	  if (!strcmp(resolution, res[i]->name))
	    return res[i];
	  else if (!strcmp(res[i]->name, ""))
	    return NULL;
	  i++;
	}
    }
  return NULL;
}

static inline int
imax(int a, int b)
{
  if (a > b)
    return a;
  else
    return b;
}

static void
internal_imageable_area(stp_const_vars_t v, int use_paper_margins,
			int *left, int *right, int *bottom, int *top)
{
  int	width, height;			/* Size of page */
  int	rollfeed = 0;			/* Roll feed selected */
  const char *input_slot = stp_get_string_parameter(v, "InputSlot");
  const char *media_size = stp_get_string_parameter(v, "PageSize");
  int left_margin = 0;
  int right_margin = 0;
  int bottom_margin = 0;
  int top_margin = 0;
  const stp_papersize_t *pt = NULL;

  if (media_size && use_paper_margins)
    pt = stp_get_papersize_by_name(media_size);

  if (input_slot && strlen(input_slot) > 0)
    {
      int i;
      const input_slot_list_t *slots = escp2_input_slots(v);
      for (i = 0; i < slots->n_input_slots; i++)
	{
	  if (slots->slots[i].name &&
	      strcmp(input_slot, slots->slots[i].name) == 0)
	    {
	      rollfeed = slots->slots[i].is_roll_feed;
	      break;
	    }
	}
    }

  stpi_default_media_size(v, &width, &height);
  if (pt)
    {
      left_margin = pt->left;
      right_margin = pt->right;
      bottom_margin = pt->bottom;
      top_margin = pt->top;
    }

  left_margin = imax(left_margin, escp2_left_margin(v, rollfeed));
  right_margin = imax(right_margin, escp2_right_margin(v, rollfeed));
  bottom_margin = imax(bottom_margin, escp2_bottom_margin(v, rollfeed));
  top_margin = imax(top_margin, escp2_top_margin(v, rollfeed));

  *left =	left_margin;
  *right =	width - right_margin;
  *top =	top_margin;
  *bottom =	height - bottom_margin;
  if (escp2_has_cap(v, MODEL_XZEROMARGIN, MODEL_XZEROMARGIN_YES) &&
      stp_get_boolean_parameter(v, "FullBleed"))
    {
      *left -= 80 / (360 / 72);	/* 80 per the Epson manual */
      *right += 80 / (360 / 72);	/* 80 per the Epson manual */
    }
}

/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

static void
escp2_imageable_area(stp_const_vars_t v,   /* I */
		     int  *left,	/* O - Left position in points */
		     int  *right,	/* O - Right position in points */
		     int  *bottom,	/* O - Bottom position in points */
		     int  *top)		/* O - Top position in points */
{
  internal_imageable_area(v, 1, left, right, bottom, top);
}

static void
escp2_limit(stp_const_vars_t v,			/* I */
	    int *width, int *height,
	    int *min_width, int *min_height)
{
  *width =	escp2_max_paper_width(v);
  *height =	escp2_max_paper_height(v);
  *min_width =	escp2_min_paper_width(v);
  *min_height =	escp2_min_paper_height(v);
}

static void
escp2_describe_resolution(stp_const_vars_t v, int *x, int *y)
{
  const res_t *res = escp2_find_resolution(v);
  if (res && verify_resolution(v, res))
    {
      *x = res->hres;
      *y = res->vres;
      return;
    }
  *x = -1;
  *y = -1;
}

static int
escp2_has_advanced_command_set(stp_const_vars_t v)
{
  return (escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO) ||
	  escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_1999) ||
	  escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_2000));
}

static int
escp2_use_extended_commands(stp_const_vars_t v, int use_softweave)
{
  return (escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO) ||
	  (escp2_has_cap(v, MODEL_VARIABLE_DOT, MODEL_VARIABLE_YES) &&
	   use_softweave));
}

static int
set_raw_ink_type(stp_vars_t v, stp_image_t *image)
{
  const inklist_t *inks = escp2_inklist(v);
  int ninktypes = inks->n_inks;
  int i;
  /*
   * If we're using raw printer output, we dummy up the appropriate inkset.
   */
  for (i = 0; i < ninktypes; i++)
    if (inks->inknames[i]->inkset == INKSET_EXTENDED &&
	inks->inknames[i]->channel_set->channel_count * 2 == stpi_image_bpp(image))
      {
	stpi_dprintf(STPI_DBG_INK, v, "Changing ink type from %s to %s\n",
		     stp_get_string_parameter(v, "InkType") ?
		     stp_get_string_parameter(v, "InkType") : "NULL",
		     inks->inknames[i]->name);
	stp_set_string_parameter(v, "InkType", inks->inknames[i]->name);
	return 1;
      }
  stpi_eprintf
    (v, _("This printer does not support raw printer output at depth %d\n"),
     stpi_image_bpp(image) / 2);
  return 0;
}

static void
adjust_density_and_ink_type(stp_vars_t v, stp_image_t *image)
{
  escp2_privdata_t *pd = get_privdata(v);
  const paper_adjustment_t *pt = pd->paper_adjustment;
  double paper_density = .8;
  int o_resid = compute_resid(pd->res);

  if (pt)
    paper_density = pt->base_density;

  if (!stp_check_float_parameter(v, "Density", STP_PARAMETER_DEFAULTED))
    {
      stp_set_float_parameter_active(v, "Density", STP_PARAMETER_ACTIVE);
      stp_set_float_parameter(v, "Density", 1.0);
    }

  if (pd->rescale_density)
    stp_scale_float_parameter
      (v, "Density", paper_density * escp2_density(v, o_resid));
  pd->drop_size = escp2_ink_type(v, o_resid);
  pd->ink_resid = o_resid;

  /*
   * If density is greater than 1, try to find the dot size from a lower
   * resolution that will let us print.  This allows use of high ink levels
   * on special paper types that need a lot of ink.
   */
  if (stp_get_float_parameter(v, "Density") > 1.0)
    {
      if (stp_check_int_parameter(v, "escp2_ink_type", STP_PARAMETER_ACTIVE) ||
	  stp_check_int_parameter(v, "escp2_density", STP_PARAMETER_ACTIVE) ||
	  stp_check_int_parameter(v, "escp2_bits", STP_PARAMETER_ACTIVE) ||
	  (stp_check_boolean_parameter(v, "AdjustDotsize",
				       STP_PARAMETER_ACTIVE) &&
	   ! stp_get_boolean_parameter(v, "AdjustDotsize")))
	{
	  stp_set_float_parameter(v, "Density", 1.0);
	}
      else
	{
	  double density = stp_get_float_parameter(v, "Density");
	  int resid = o_resid;
	  int xresid = resid;
	  double xdensity = density;
	  while (density > 1.0 && resid >= RES_360)
	    {
	      int tresid = xresid - 1;
	      int bits_now = escp2_bits(v, resid);
	      double density_now = escp2_density(v, resid);
	      int bits_then = escp2_bits(v, tresid);
	      double density_then = escp2_density(v, tresid);
	      int drop_size_then = escp2_ink_type(v, tresid);

	      /*
	       * If we would change the number of bits in the ink type,
	       * don't try this.  Some resolutions require using a certain
	       * number of bits!
	       */

	      if (bits_now != bits_then || density_then <= 0.0 ||
		  drop_size_then == -1)
		break;
	      xdensity = density * density_then / density_now / 2;
	      xresid = tresid;

	      /*
	       * If we wouldn't get a significant improvement by changing the
	       * resolution, don't waste the effort trying.
	       */
	      if (density / xdensity > 1.001)
		{
		  density = xdensity;
		  resid = tresid;
		}
	    }
	  pd->drop_size = escp2_ink_type(v, resid);
	  pd->ink_resid = resid;
	  if (density > 1.0)
	    density = 1.0;
	  stp_set_float_parameter(v, "Density", density);
	}
    }
}

static void
adjust_print_quality(stp_vars_t v, stp_image_t *image)
{
  escp2_privdata_t *pd = get_privdata(v);
  stp_curve_t   adjustment = NULL;
  const paper_adjustment_t *pt;
  double k_upper = 1.0;
  double k_lower = 0;
  double k_transition = 1.0;

  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */

  pt = pd->paper_adjustment;
  if (pt)
    {
      k_lower = pt->k_lower;
      k_upper = pt->k_upper;
      k_transition = pt->k_transition;
      stp_scale_float_parameter(v, "CyanDensity", pt->cyan);
      stp_scale_float_parameter(v, "MagentaDensity", pt->magenta);
      stp_scale_float_parameter(v, "YellowDensity", pt->yellow);
      stp_scale_float_parameter(v, "BlackDensity", pt->black);
      stp_scale_float_parameter(v, "Saturation", pt->saturation);
      stp_scale_float_parameter(v, "Gamma", pt->gamma);
    }


  if (!stp_check_float_parameter(v, "GCRLower", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRLower", k_lower);
  if (!stp_check_float_parameter(v, "GCRUpper", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRUpper", k_upper);
  if (!stp_check_float_parameter(v, "BlackGamma", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "BlackGamma", k_transition);
    

  if (!stp_check_curve_parameter(v, "HueMap", STP_PARAMETER_ACTIVE) &&
      pt->hue_adjustment)
    {
      adjustment = stp_curve_create_from_string(pt->hue_adjustment);
      stp_set_curve_parameter(v, "HueMap", adjustment);
      stp_set_curve_parameter_active(v, "HueMap", STP_PARAMETER_ACTIVE);
      stp_curve_free(adjustment);
    }
  if (!stp_check_curve_parameter(v, "SatMap", STP_PARAMETER_ACTIVE) &&
      pt->sat_adjustment)
    {
      adjustment = stp_curve_create_from_string(pt->sat_adjustment);
      stp_set_curve_parameter(v, "SatMap", adjustment);
      stp_set_curve_parameter_active(v, "SatMap", STP_PARAMETER_ACTIVE);
      stp_curve_free(adjustment);
    }
  if (!stp_check_curve_parameter(v, "LumMap", STP_PARAMETER_ACTIVE) &&
      pt->lum_adjustment)
    {
      adjustment = stp_curve_create_from_string(pt->lum_adjustment);
      stp_set_curve_parameter(v, "LumMap", adjustment);
      stp_set_curve_parameter_active(v, "LumMap", STP_PARAMETER_ACTIVE);
      stp_curve_free(adjustment);
    }
}

static int
count_channels(const escp2_inkname_t *inks)
{
  int answer = 0;
  int i;
  for (i = 0; i < inks->channel_set->channel_count; i++)
    if (inks->channel_set->channels[i])
      answer += inks->channel_set->channels[i]->n_subchannels;
  return answer;
}

static int
compute_channel_count(const escp2_inkname_t *ink_type, int channel_limit)
{
  int i;
  int physical_channels = 0;
  for (i = 0; i < channel_limit; i++)
    {
      const ink_channel_t *channel = ink_type->channel_set->channels[i];
      if (channel)
	physical_channels += channel->n_subchannels;
    }
  return physical_channels;
}

static double
get_double_param(stp_const_vars_t v, const char *param)
{
  if (param && stp_check_float_parameter(v, param, STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter(v, param);
  else
    return 1.0;
}

static void
setup_inks(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int i, j;
  const escp2_dropsize_t *drops;
  const escp2_inkname_t *ink_type = pd->inkname;
  const paper_adjustment_t *paper = pd->paper_adjustment;

  drops = escp2_dropsizes(v, pd->ink_resid);
  stpi_init_debug_messages(v);
  for (i = 0; i < pd->logical_channels; i++)
    {
      const ink_channel_t *channel = ink_type->channel_set->channels[i];
      if (channel && channel->n_subchannels > 0)
	{
	  const char *param = channel->subchannels[0].channel_density;
	  const shade_t *shades = escp2_shades(v, i);
	  double userval = get_double_param(v, param);
	  if (shades->n_shades < channel->n_subchannels)
	    {
	      stpi_erprintf("Not enough shades!\n");
	    }
	  if (strcmp(param, "BlackDensity") == 0)
	    stpi_channel_set_black_channel(v, i);
	  stpi_dither_set_inks(v, i, 1.0,
			       channel->n_subchannels, shades->shades,
			       drops->numdropsizes, drops->dropsizes);
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      const char *subparam =
		channel->subchannels[j].subchannel_scale;
	      double scale = userval * get_double_param(v, subparam);
	      scale *= get_double_param(v, "Density");
	      stpi_channel_set_density_adjustment(v, i, j, scale);
	      if (paper)
		stpi_channel_set_cutoff_adjustment(v, i, j,
						   paper->subchannel_cutoff);
	    }
	}
    }
  stpi_flush_debug_messages(v);
}

static void
setup_head_offset(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int i;
  int channel_id = 0;
  const escp2_inkname_t *ink_type = pd->inkname;
  pd->head_offset = stpi_zalloc(sizeof(int) * pd->channels_in_use);
  for (i = 0; i < pd->logical_channels; i++)
    {
      const ink_channel_t *channel = ink_type->channel_set->channels[i];
      if (channel)
	{
	  int j;
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      pd->head_offset[channel_id] =
		channel->subchannels[j].head_offset;
	      channel_id++;
	    }
	}
    }
  if (pd->physical_channels == 1)
    pd->head_offset[0] = 0;
  pd->max_head_offset = 0;
  if (pd->physical_channels > 1)
    for (i = 0; i < pd->channels_in_use; i++)
      {
	pd->head_offset[i] = pd->head_offset[i] * pd->res->vres /
	  escp2_base_separation(v);
	if (pd->head_offset[i] > pd->max_head_offset)
	  pd->max_head_offset = pd->head_offset[i];
      }
}

static void
setup_misc(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  pd->input_slot = get_input_slot(v);
  pd->paper_type = get_media_type(v);
  pd->paper_adjustment = get_media_adjustment(v);
  pd->ink_group = escp2_inkgroup(v);
  pd->init_sequence = escp2_preinit_sequence(v);
  pd->deinit_sequence = escp2_postinit_remote_sequence(v);
  pd->advanced_command_set = escp2_has_advanced_command_set(v);
  pd->command_set = escp2_get_cap(v, MODEL_COMMAND);
  pd->variable_dots = escp2_has_cap(v, MODEL_VARIABLE_DOT, MODEL_VARIABLE_YES);
  pd->has_vacuum = escp2_has_cap(v, MODEL_VACUUM, MODEL_VACUUM_YES);
  pd->has_graymode = escp2_has_cap(v, MODEL_GRAYMODE, MODEL_GRAYMODE_YES);
  pd->base_separation = escp2_base_separation(v);
  pd->resolution_scale = escp2_resolution_scale(v);
  pd->use_extended_commands =
    escp2_use_extended_commands(v, pd->res->softweave);
}

static void
allocate_channels(stp_vars_t v, int line_length)
{
  escp2_privdata_t *pd = get_privdata(v);
  const escp2_inkname_t *ink_type = pd->inkname;
  int i;
  int channel_id = 0;

  pd->cols = stpi_zalloc(sizeof(unsigned char *) * pd->channels_in_use);
  pd->channels =
    stpi_zalloc(sizeof(physical_subchannel_t *) * pd->channels_in_use);

  for (i = 0; i < pd->logical_channels; i++)
    {
      const ink_channel_t *channel = ink_type->channel_set->channels[i];
      if (channel)
	{
	  int j;
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      pd->cols[channel_id] = stpi_zalloc(line_length);
	      pd->channels[channel_id] = &(channel->subchannels[j]);
	      stpi_dither_add_channel(v, pd->cols[channel_id], i, j);
	      channel_id++;
	    }
	}
    }
}

static unsigned
gcd(unsigned a, unsigned b)
{
  unsigned tmp;
  if (b > a)
    {
      tmp = a;
      a = b;
      b = tmp;
    }
  while (1)
    {
      tmp = a % b;
      if (tmp == 0)
	return b;
      a = b;
      b = tmp;
    }
}

static unsigned
lcm(unsigned a, unsigned b)
{
  if (a == b)
    return a;
  else
    return a * b / gcd(a, b);
}

static int
adjusted_vertical_resolution(const res_t *res)
{
  if (res->vres >= 720)
    return res->vres;
  else if (res->hres >= 720)	/* Special case 720x360 */
    return 720;
  else if (res->vres % 90 == 0)
    return res->vres;
  else
    return lcm(res->hres, res->vres);
}

static int
adjusted_horizontal_resolution(const res_t *res)
{
  if (res->vres % 90 == 0)
    return res->hres;
  else
    return lcm(res->hres, res->vres);
}

static void
setup_resolution(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  const res_t *res = escp2_find_resolution(v);
  int resid = compute_resid(res);

  int vertical = adjusted_vertical_resolution(res);
  int horizontal = adjusted_horizontal_resolution(res);

  pd->res = res;
  pd->physical_xdpi = escp2_base_res(v, resid);
  if (pd->physical_xdpi > pd->res->hres)
    pd->physical_xdpi = pd->res->hres;

  if (escp2_use_extended_commands(v, pd->res->softweave))
    {
      pd->unit_scale = escp2_max_hres(v);
      pd->horizontal_units = horizontal;
      pd->micro_units = horizontal;
    }
  else
    {
      pd->unit_scale = 3600;
      if (pd->res->hres <= 720)
	pd->micro_units = vertical;
      else
	pd->micro_units = horizontal;
      pd->horizontal_units = vertical;
    }
  pd->vertical_units = vertical;
  pd->page_management_units = vertical;
  pd->printing_resolution = escp2_base_res(v, resid);
}

static void
setup_softweave_parameters(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  pd->horizontal_passes = pd->res->hres / pd->physical_xdpi;
  if (pd->physical_channels == 1 &&
      (pd->res->vres >=
       (escp2_base_separation(v) / escp2_black_nozzle_separation(v))) &&
      (escp2_max_black_resolution(v) < 0 ||
       pd->res->vres <= escp2_max_black_resolution(v)) &&
      escp2_black_nozzles(v))
    pd->use_black_parameters = 1;
  else
    pd->use_black_parameters = 0;
  if (pd->use_fast_360)
    {
      pd->nozzles = escp2_fast_nozzles(v);
      pd->nozzle_separation = escp2_fast_nozzle_separation(v);
      pd->min_nozzles = escp2_min_fast_nozzles(v);
    }
  else if (pd->use_black_parameters)
    {
      pd->nozzles = escp2_black_nozzles(v);
      pd->nozzle_separation = escp2_black_nozzle_separation(v);
      pd->min_nozzles = escp2_min_black_nozzles(v);
    }
  else
    {
      pd->nozzles = escp2_nozzles(v);
      pd->nozzle_separation = escp2_nozzle_separation(v);
      pd->min_nozzles = escp2_min_nozzles(v);
    }
}

static void
setup_microweave_parameters(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  pd->horizontal_passes =
    pd->res->hres / escp2_base_res(v, compute_resid(pd->res));
  pd->nozzles = 1;
  pd->nozzle_separation = 1;
  pd->min_nozzles = 1;
  pd->use_black_parameters = 0;
}

static void
setup_head_parameters(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  /*
   * Set up the output channels
   */
  if (stp_get_output_type(v) == OUTPUT_RAW_PRINTER)
    pd->logical_channels = escp2_physical_channels(v);
  else if (stp_get_output_type(v) == OUTPUT_GRAY)
    pd->logical_channels = 1;
  else
    pd->logical_channels = NCOLORS;

  pd->physical_channels =
    compute_channel_count(pd->inkname, pd->logical_channels);
  if (pd->physical_channels == 0)
    {
      pd->inkname = &stpi_escp2_default_black_inkset;
      pd->physical_channels =
	compute_channel_count(pd->inkname, pd->logical_channels);
    }

  if (escp2_has_cap(v, MODEL_FAST_360, MODEL_FAST_360_YES) &&
      (pd->inkname->inkset == INKSET_CMYK || pd->physical_channels == 1) &&
      pd->res->hres == pd->physical_xdpi && pd->res->vres == 360)
    pd->use_fast_360 = 1;
  else
    pd->use_fast_360 = 0;

  /*
   * Set up the printer-specific parameters (weaving)
   */
  if (pd->res->softweave)
    setup_softweave_parameters(v);
  else
    setup_microweave_parameters(v);
  pd->separation_rows = escp2_separation_rows(v);
  pd->pseudo_separation_rows = escp2_pseudo_separation_rows(v);
  pd->extra_720dpi_separation = escp2_extra_720dpi_separation(v);

  if (pd->horizontal_passes == 0)
    pd->horizontal_passes = 1;

  setup_head_offset(v);

  if (stp_get_output_type(v) == OUTPUT_GRAY && pd->physical_channels == 1)
    {
      if (pd->use_black_parameters)
	pd->initial_vertical_offset =
	  escp2_black_initial_vertical_offset(v) * pd->page_management_units /
	  escp2_base_separation(v);
      else
	pd->initial_vertical_offset = pd->head_offset[0] +
	  (escp2_initial_vertical_offset(v) *
	   pd->page_management_units / escp2_base_separation(v));
    }
  else
    pd->initial_vertical_offset =
      escp2_initial_vertical_offset(v) * pd->page_management_units /
      escp2_base_separation(v);

  pd->printing_initial_vertical_offset = 0;
  pd->bitwidth = escp2_bits(v, compute_resid(pd->res));
}

static void
setup_page(stp_vars_t v)
{
  int n;
  escp2_privdata_t *pd = get_privdata(v);
  const input_slot_t *input_slot = get_input_slot(v);

  stpi_default_media_size(v, &n, &(pd->page_true_height));
  internal_imageable_area(v, 0, &pd->page_left, &pd->page_right,
			  &pd->page_bottom, &pd->page_top);

  pd->page_width = pd->page_right - pd->page_left;
  pd->image_left = stp_get_left(v) - pd->page_left;
  pd->image_width = stp_get_width(v);
  pd->image_scaled_width = pd->image_width * pd->res->hres / 72;
  pd->image_left_position = pd->image_left * pd->micro_units / 72;


  pd->page_height = pd->page_bottom - pd->page_top;
  pd->image_top = stp_get_top(v) - pd->page_top;
  pd->image_height = stp_get_height(v);
  pd->image_scaled_height = pd->image_height * pd->res->vres / 72;

  if (input_slot && input_slot->roll_feed_cut_flags)
    {
      pd->page_true_height += 4; /* Empirically-determined constants */
      pd->page_top += 2;
      pd->page_bottom += 2;
      pd->image_top += 2;
      pd->page_height += 2;
    }
}

static int
escp2_print_data(stp_vars_t v, stp_image_t *image)
{
  escp2_privdata_t *pd = get_privdata(v);
  int errdiv  = stpi_image_height(image) / pd->image_scaled_height;
  int errmod  = stpi_image_height(image) % pd->image_scaled_height;
  int errval  = 0;
  int errlast = -1;
  int errline  = 0;
  int y;

  stpi_image_progress_init(image);

  for (y = 0; y < pd->image_scaled_height; y ++)
    {
      int duplicate_line = 1;
      unsigned zero_mask;
      if ((y & 63) == 0)
	stpi_image_note_progress(image, y, pd->image_scaled_height);

      if (errline != errlast)
	{
	  errlast = errline;
	  duplicate_line = 0;
	  if (stpi_color_get_row(v, image, errline, &zero_mask))
	    return 2;
	}

      stpi_dither(v, y, duplicate_line, zero_mask);

      stpi_write_weave(v, pd->cols);
      errval += errmod;
      errline += errdiv;
      if (errval >= pd->image_scaled_height)
	{
	  errval -= pd->image_scaled_height;
	  errline ++;
	}
    }
  return 1;
}

static int
escp2_print_page(stp_vars_t v, stp_image_t *image)
{
  int status;
  int i;
  escp2_privdata_t *pd = get_privdata(v);
  int out_channels;		/* Output bytes per pixel */
  int line_width = (pd->image_scaled_width + 7) / 8 * pd->bitwidth;

  stpi_initialize_weave
    (v,
     pd->nozzles,
     pd->nozzle_separation * pd->res->vres / escp2_base_separation(v),
     pd->horizontal_passes,
     pd->res->vertical_passes,
     1,
     pd->channels_in_use,
     pd->bitwidth,
     pd->image_scaled_width,
     pd->image_scaled_height,
     pd->image_top * pd->res->vres / 72,
     (pd->page_height + escp2_extra_feed(v)) * pd->res->vres / 72,
     pd->head_offset,
     STPI_WEAVE_ZIGZAG,
     stpi_escp2_flush_pass,
     FILLFUNC,
     PACKFUNC,
     COMPUTEFUNC);

  stpi_set_output_color_model(v, COLOR_MODEL_CMY);
  adjust_print_quality(v, image);
  out_channels = stpi_color_init(v, image, 65536);

  stpi_dither_init(v, image, pd->image_scaled_width, pd->res->hres,
		   pd->res->vres);

  allocate_channels(v, line_width);
  setup_inks(v);

  status = escp2_print_data(v, image);
  stpi_image_progress_conclude(image);
  stpi_flush_all(v);
  stpi_escp2_terminate_page(v);

  /*
   * Cleanup...
   */
  for (i = 0; i < pd->channels_in_use; i++)
    if (pd->cols[i])
      stpi_free(pd->cols[i]);
  stpi_free(pd->cols);
  stpi_free(pd->channels);
  return status;
}

/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
static int
escp2_do_print(stp_vars_t v, stp_image_t *image, int print_op)
{
  int status = 1;

  escp2_privdata_t *pd;

  if (!stp_verify(v))
    {
      stpi_eprintf(v, _("Print options not verified; cannot print.\n"));
      return 0;
    }
  stpi_image_init(image);

  if (stp_get_output_type(v) == OUTPUT_RAW_PRINTER &&
      !set_raw_ink_type(v, image))
    return 0;

  pd = (escp2_privdata_t *) stpi_malloc(sizeof(escp2_privdata_t));
  pd->printed_something = 0;
  pd->last_color = -1;
  pd->last_pass_offset = 0;
  pd->last_pass = -1;
  pd->send_zero_pass_advance =
    escp2_has_cap(v, MODEL_SEND_ZERO_ADVANCE, MODEL_SEND_ZERO_ADVANCE_YES);
  stpi_allocate_component_data(v, "Driver", NULL, NULL, pd);

  if (stp_get_output_type(v) == OUTPUT_RAW_CMYK ||
      stp_get_output_type(v) == OUTPUT_RAW_PRINTER)
    pd->rescale_density = 0;
  else
    pd->rescale_density = 1;

  pd->inkname = get_inktype(v);
  pd->channels_in_use = count_channels(pd->inkname);
  if (stp_get_output_type(v) != OUTPUT_RAW_PRINTER &&
      pd->inkname->channel_set->channel_count == 1)
    stp_set_output_type(v, OUTPUT_GRAY);
  if (stp_get_output_type(v) == OUTPUT_COLOR &&
      pd->inkname->channel_set->channels[ECOLOR_K] != NULL)
    stp_set_output_type(v, OUTPUT_RAW_CMYK);

  setup_resolution(v);
  setup_head_parameters(v);
  setup_page(v);
  setup_misc(v);

  adjust_density_and_ink_type(v, image);
  if (print_op & OP_JOB_START)
    stpi_escp2_init_printer(v);
  if (print_op & OP_JOB_PRINT)
    status = escp2_print_page(v, image);
  if (print_op & OP_JOB_END)
    stpi_escp2_deinit_printer(v);

  stpi_free(pd->head_offset);
  stpi_free(pd);

  return status;
}

static int
escp2_print(stp_const_vars_t v, stp_image_t *image)
{
  stp_vars_t nv = stp_vars_create_copy(v);
  int op = OP_JOB_PRINT;
  int status;
  if (stp_get_job_mode(v) == STP_JOB_MODE_PAGE)
    op = OP_JOB_START | OP_JOB_PRINT | OP_JOB_END;
  stpi_prune_inactive_options(nv);
  status = escp2_do_print(nv, image, op);
  stp_vars_free(nv);
  return status;
}

static int
escp2_job_start(stp_const_vars_t v, stp_image_t *image)
{
  stp_vars_t nv = stp_vars_create_copy(v);
  int status;
  stpi_prune_inactive_options(nv);
  status = escp2_do_print(nv, image, OP_JOB_START);
  stp_vars_free(nv);
  return status;
}

static int
escp2_job_end(stp_const_vars_t v, stp_image_t *image)
{
  stp_vars_t nv = stp_vars_create_copy(v);
  int status;
  stpi_prune_inactive_options(nv);
  status = escp2_do_print(nv, image, OP_JOB_END);
  stp_vars_free(nv);
  return status;
}

static const stpi_printfuncs_t print_escp2_printfuncs =
{
  escp2_list_parameters,
  escp2_parameters,
  stpi_default_media_size,
  escp2_imageable_area,
  escp2_limit,
  escp2_print,
  escp2_describe_resolution,
  stpi_verify_printer_params,
  escp2_job_start,
  escp2_job_end
};

static stpi_internal_family_t print_escp2_module_data =
  {
    &print_escp2_printfuncs,
    NULL
  };


static int
print_escp2_module_init(void)
{
  return stpi_family_register(print_escp2_module_data.printer_list);
}


static int
print_escp2_module_exit(void)
{
  return stpi_family_unregister(print_escp2_module_data.printer_list);
}


/* Module header */
#define stpi_module_version print_escp2_LTX_stpi_module_version
#define stpi_module_data print_escp2_LTX_stpi_module_data

stpi_module_version_t stpi_module_version = {0, 0};

stpi_module_t stpi_module_data =
  {
    "escp2",
    VERSION,
    "Epson family driver",
    STPI_MODULE_CLASS_FAMILY,
    NULL,
    print_escp2_module_init,
    print_escp2_module_exit,
    (void *) &print_escp2_module_data
  };

