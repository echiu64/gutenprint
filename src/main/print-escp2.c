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
#include "print-escp2.h"
#include "module.h"
#include "weave.h"

#ifdef __GNUC__
#define inline __inline__
#endif

#ifdef TEST_UNCOMPRESSED
#define COMPRESSION (0)
#define FILLFUNC stpi_fill_uncompressed
#define COMPUTEFUNC stpi_compute_uncompressed_linewidth
#define PACKFUNC stpi_pack_uncompressed
#else
#define COMPRESSION (1)
#define FILLFUNC stpi_fill_tiff
#define COMPUTEFUNC stpi_compute_tiff_linewidth
#define PACKFUNC stpi_pack_tiff
#endif

#define OP_JOB_START 1
#define OP_JOB_PRINT 2
#define OP_JOB_END   4

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static const escp2_printer_attr_t escp2_printer_attrs[] =
{
  { "command_mode",		0, 4 },
  { "horizontal_zero_margin",	4, 1 },
  { "rollfeed",			5, 1 },
  { "variable_mode",		6, 1 },
  { "graymode",		 	7, 1 },
  { "vacuum",			8, 1 },
  { "fast_360",			9, 1 },
};

#define INCH(x)		(72 * x)

static const res_t *escp2_find_resolution(stp_const_vars_t v,
					  const char *resolution);

typedef struct
{
  /* Basic print head parameters */
  int nozzles;			/* Number of nozzles */
  int min_nozzles;		/* Fewest nozzles we're allowed to use */
  int nozzle_separation;	/* Nozzle separation, in dots */
  int *head_offset;		/* Head offset (for C80-type printers) */
  int max_head_offset;		/* Largest head offset */
  int page_management_units;	/* Page management units (dpi) */
  int vertical_units;		/* Vertical units (dpi) */
  int horizontal_units;		/* Horizontal units (dpi) */
  int micro_units;		/* Micro-units for horizontal positioning */
  int unit_scale;		/* Scale factor for units */

  /* Ink parameters */
  int bitwidth;			/* Number of bits per ink drop */
  int drop_size;		/* ID of the drop size we're using */
  int ink_resid;		/* Array index for the drop set we're using */
  const escp2_inkname_t *inkname; /* Description of the ink set */
  int rescale_density;		/* Do we want to rescale the density? */

  /* Ink channels */
  int logical_channels;		/* Number of logical ink channels (e.g.CMYK) */
  int physical_channels;	/* Number of physical channels (e.g. CcMmYK) */
  int channels_in_use;		/* Number of channels we're using
				   FIXME merge with physical_channels! */
  unsigned char **cols;		/* Output dithered data */
  const physical_subchannel_t **channels; /* Description of each channel */

  /* Miscellaneous printer control */
  int use_black_parameters;	/* Can we use (faster) black head parameters */
  int use_fast_360;		/* Can we use fast 360 DPI 4 color mode */

  /* Weave parameters */
  int horizontal_passes;	/* Number of horizontal passes required
				   to print a complete row */
  int physical_xdpi;		/* Horizontal distance between dots in pass */
  const res_t *res;		/* Description of the printing resolution */

  /* Page parameters */		/* Indexed from top left */
  int page_left;		/* Left edge of page (points) */
  int page_right;		/* Right edge of page (points) */
  int page_top;			/* Top edge of page (points) */
  int page_bottom;		/* Bottom edge of page (points) */
  int page_width;		/* Page width (points) */
  int page_height;		/* Page height (points) */
  int page_true_height;		/* Physical page height (points) */

  /* Image parameters */	/* Indexed from top left */
  int image_height;		/* Height of printed region (points) */
  int image_width;		/* Width of printed region (points) */
  int image_top;		/* First printed row (points) */
  int image_left;		/* Left edge of image (points) */
  int image_scaled_width;	/* Width of printed region (dots) */
  int image_scaled_height;	/* Height of printed region (dots) */
  int image_left_position;	/* Left dot position of image */

  /* Transitory state */
  int printed_something;	/* Have we actually printed anything? */
  int initial_vertical_offset;	/* Vertical offset for C80-type printers */
  int printing_initial_vertical_offset;	/* Vertical offset, for print cmd */
  int last_color;		/* Last color we printed */
  int last_pass_offset;		/* Starting row of last pass we printed */

} escp2_privdata_t;

#define PARAMETER_INT(s)				\
{							\
  "escp2_" #s, "escp2_" #s, NULL,			\
  STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,	\
  STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1		\
}

#define PARAMETER_RAW(s)				\
{							\
  "escp2_" #s, "escp2_" #s, NULL,			\
  STP_PARAMETER_TYPE_RAW, STP_PARAMETER_CLASS_FEATURE,	\
  STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1		\
}

static const stp_parameter_t the_parameters[] =
{
  {
    "PageSize", N_("Page Size"),
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_PAGE_SIZE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "MediaType", N_("Media Type"),
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "InputSlot", N_("Media Source"),
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "Resolution", N_("Resolution"),
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "InkType", N_("Ink Type"),
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "PrintingDirection", N_("Printing Direction"),
    N_("Printing direction (unidirectional is higher quality, but slower)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "FullBleed", N_("Full Bleed"),
    N_("Full Bleed"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "LightCyanTransition", N_("Light Cyan Transition"),
    N_("Light Cyan Transition"),
    STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1
  },
  {
    "LightMagentaTransition", N_("Light Magenta Transition"),
    N_("Light Magenta Transition"),
    STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1
  },
  {
    "DarkYellowTransition", N_("Dark Yellow Transition"),
    N_("Dark Yellow Transition"),
    STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1
  },
  {
    "GrayTransition", N_("Gray Transition"),
    N_("Gray Transition"),
    STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1
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
  PARAMETER_RAW(preinit_sequence),
  PARAMETER_RAW(postinit_remote_sequence)
};

static int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

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
static t								\
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
static t								\
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
static t						\
escp2_##f(stp_const_vars_t v)				\
{							\
  int model = stpi_get_model_id(v);			\
  return (stpi_escp2_model_capabilities[model].f);	\
}

#define DEF_ROLL_ACCESSOR(f, t)						     \
static t								     \
escp2_##f(stp_const_vars_t v, int rollfeed)				     \
{									     \
  if (stp_check_int_parameter(v, "escp2_" #f, STP_PARAMETER_ACTIVE))	     \
    return stp_get_int_parameter(v, "escp2_" #f);			     \
  else									     \
    {									     \
      int model = stpi_get_model_id(v);					     \
      const res_t *res =						     \
	escp2_find_resolution(v, stp_get_string_parameter(v, "Resolution")); \
      if (res && !(res->softweave))					     \
	{								     \
	  if (rollfeed)							     \
	    return (stpi_escp2_model_capabilities[model].m_roll_##f);	     \
	  else								     \
	    return (stpi_escp2_model_capabilities[model].m_##f);	     \
	}								     \
      else								     \
	{								     \
	  if (rollfeed)							     \
	    return (stpi_escp2_model_capabilities[model].roll_##f);	     \
	  else								     \
	    return (stpi_escp2_model_capabilities[model].f);		     \
	}								     \
    }									     \
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

DEF_ROLL_ACCESSOR(left_margin, unsigned)
DEF_ROLL_ACCESSOR(right_margin, unsigned)
DEF_ROLL_ACCESSOR(top_margin, unsigned)
DEF_ROLL_ACCESSOR(bottom_margin, unsigned)

DEF_RAW_ACCESSOR(preinit_sequence, const stp_raw_t *)
DEF_RAW_ACCESSOR(postinit_remote_sequence, const stp_raw_t *)

DEF_COMPOSITE_ACCESSOR(paperlist, const paperlist_t *)
DEF_COMPOSITE_ACCESSOR(reslist, const res_t *)
DEF_COMPOSITE_ACCESSOR(inklist, const inklist_t *)
DEF_COMPOSITE_ACCESSOR(input_slots, const input_slot_list_t *)

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

static const escp2_variable_inkset_t *
escp2_inks(stp_const_vars_t v, int resid, int inkset)
{
  int model = stpi_get_model_id(v);
  const escp2_variable_inklist_t *inks =
    stpi_escp2_model_capabilities[model].inks;
  return (*inks)[inkset][resid];
}

static const input_slot_t *
get_input_slot(stp_vars_t v)
{
  int i;
  const char *input_slot = stp_get_string_parameter(v, "InputSlot");
  if (input_slot && strlen(input_slot) > 0)
    {
      const input_slot_list_t *slots = escp2_input_slots(v);
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
  return NULL;
}  

static const paper_t *
get_media_type(stp_const_vars_t v)
{
  int i;
  const char *name = stp_get_string_parameter(v, "MediaType");
  const paperlist_t *p = escp2_paperlist(v);
  int paper_type_count = p->paper_count;
  if (name)
    {
      for (i = 0; i < paper_type_count; i++)
	{
	  if (!strcmp(name, p->papers[i].name))
	    return &(p->papers[i]);
	}
    }
  return NULL;
}

static int
verify_resolution(stp_const_vars_t v, const res_t *res)
{
  int nozzle_width =
    (escp2_base_separation(v) / escp2_nozzle_separation(v));
  int nozzles = escp2_nozzles(v);
  if (escp2_ink_type(v, res->resid) != -1 &&
      res->vres <= escp2_max_vres(v) &&
      res->hres <= escp2_max_hres(v) &&
      res->vres >= escp2_min_vres(v) &&
      res->hres >= escp2_min_hres(v) &&
      (nozzles == 1 ||
       ((res->vres / nozzle_width) * nozzle_width) == res->vres))
    {
      int xdpi = res->hres;
      int physical_xdpi = escp2_base_res(v, res->resid);
      int horizontal_passes, oversample;
      if (physical_xdpi > xdpi)
	physical_xdpi = xdpi;
      horizontal_passes = xdpi / physical_xdpi;
      oversample = horizontal_passes * res->vertical_passes
	* res->vertical_oversample;
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
verify_inktype( stp_const_vars_t v, const escp2_inkname_t *inks)
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
  const paper_t *pt = get_media_type(v);
  if (!ink_list)
    return NULL;
  if (pt && pt->preferred_ink_type)
    return pt->preferred_ink_type;
  else if (escp2_has_cap(v, MODEL_FAST_360, MODEL_FAST_360_YES) &&
	   stp_check_string_parameter(v, "Resolution", STP_PARAMETER_ACTIVE))
    {
      const res_t *res =
	escp2_find_resolution(v, stp_get_string_parameter(v, "Resolution"));
      if (res->vres == 360 && res->hres == escp2_base_res(v, res->resid))
	{
	  int i;
	  for (i = 0; i < ink_list->n_inks; i++)
	    if (strcmp(ink_list->inknames[i]->name, "CMYK") == 0)
	      return ink_list->inknames[i]->name;
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

  if (!ink_type || strcmp(ink_type, "DEFAULT") == 0)
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
  return ret;
}

static void
escp2_parameters(stp_const_vars_t v, const char *name,
		 stp_parameter_t *description)
{
  int		i;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stpi_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }

  description->deflt.str = NULL;
  if (strcmp(name, "PageSize") == 0)
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
  else if (strcmp(name, "Resolution") == 0)
    {
      const res_t *res = escp2_reslist(v);
      description->bounds.str = stp_string_list_create();
      while (res->hres)
	{
	  if (verify_resolution(v, res))
	    {
	      stp_string_list_add_string(description->bounds.str,
					 res->name, _(res->text));
	      if (res->vres >= 360 && res->hres >= 360 &&
		  description->deflt.str == NULL)
		description->deflt.str = res->name;
	    }
	  res++;
	}
    }
  else if (strcmp(name, "InkType") == 0)
    {
      const inklist_t *inks = escp2_inklist(v);
      int ninktypes = inks->n_inks;
      description->bounds.str = stp_string_list_create();
      if (ninktypes)
	{
	  stp_string_list_add_string(description->bounds.str, "DEFAULT",
				     _("Standard"));
	  for (i = 0; i < ninktypes; i++)
	    if (verify_inktype(v, inks->inknames[i]))
	      stp_string_list_add_string(description->bounds.str,
					 inks->inknames[i]->name,
					 _(inks->inknames[i]->text));
	  description->deflt.str = "DEFAULT";
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
	(description->bounds.str, "Auto", _("Auto"));
      stp_string_list_add_string
	(description->bounds.str, "Bidirectional", _("Bidirectional"));
      stp_string_list_add_string
	(description->bounds.str, "Unidirectional", _("Unidirectional"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "FullBleed") == 0)
    {
      if (escp2_has_cap(v, MODEL_XZEROMARGIN, MODEL_XZEROMARGIN_YES))
	description->deflt.boolean = 0;
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "LightCyanTransition") == 0 ||
	   strcmp(name, "LightMagentaTransition") == 0 ||
	   strcmp(name, "DarkYellowTransition") == 0 ||
	   strcmp(name, "GrayTransition") == 0)
    {
#if 0
      const escp2_inkname_t *inktype = get_inktype(v);
      description->is_active = 0;
      if (inktype)
	{
	  int channel_limit = inktype->channel_limit;
	  if (stp_get_output_type(v) == OUTPUT_GRAY)
	    channel_limit = 1;
	  for (i = 0; i < channel_limit; i++)
	    {
	      if (inktype->channel_parameter_names[i] &&
		  strcmp(name, inktype->channel_parameter_names[i]) == 0)
		{
		  description->is_active = 1;
		  description->bounds.dbl.lower = 0;
		  description->bounds.dbl.upper = 5.0;
		  description->deflt.dbl = 1.0;
		  return;
		}
	    }
	}
#else
      /* For now, work around a problem with the GUI */
      /* whereby we can't activate or deactivate an option based on a */
      /* combo box setting -- rlk 20030317 */
      description->is_active = 1;
      description->bounds.dbl.lower = 0;
      description->bounds.dbl.upper = 5.0;
      description->deflt.dbl = 1.0;
#endif
    }
}

static const res_t *
escp2_find_resolution(stp_const_vars_t v, const char *resolution)
{
  const res_t *res;
  if (!resolution || !strcmp(resolution, ""))
    return NULL;
  for (res = escp2_reslist(v);;res++)
    {
      if (!strcmp(resolution, res->name))
	return res;
      else if (!strcmp(res->name, ""))
	return NULL;
    }
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

  left_margin = MAX(left_margin, escp2_left_margin(v, rollfeed));
  right_margin = MAX(right_margin, escp2_right_margin(v, rollfeed));
  bottom_margin = MAX(bottom_margin, escp2_bottom_margin(v, rollfeed));
  top_margin = MAX(top_margin, escp2_top_margin(v, rollfeed));

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
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const res_t *res;
  res = escp2_reslist(v);

  while (res->hres)
    {
      if (resolution && strcmp(resolution, res->name) == 0 &&
	  verify_resolution(v, res))
	{
	  *x = res->hres;
	  *y = res->vres;
	  return;
	}
      res++;
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

static void
escp2_reset_printer(stp_vars_t v)
{
  /*
   * Magic initialization string that's needed to take printer out of
   * packet mode.
   */
  const stp_raw_t *inits = escp2_preinit_sequence(v);
  if (inits)
    stpi_zfwrite(inits->data, inits->bytes, 1, v);

  stpi_send_command(v, "\033@", "");
}

static void
print_remote_param(stp_vars_t v, const char *param, const char *value)
{
  stpi_send_command(v, "\033(R", "bcscs", '\0', param, ':',
		    value ? value : "NULL");
  stpi_send_command(v, "\033", "ccc", 0, 0, 0);
}

static void
print_remote_int_param(stp_vars_t v, const char *param, int value)
{
  char buf[64];
  (void) snprintf(buf, 64, "%d", value);
  print_remote_param(v, param, buf);
}

static void
print_remote_float_param(stp_vars_t v, const char *param, double value)
{
  char buf[64];
  (void) snprintf(buf, 64, "%f", value);
  print_remote_param(v, param, buf);
}

static escp2_privdata_t *
get_privdata(stp_vars_t v)
{
  return (escp2_privdata_t *) stpi_get_component_data(v, "Driver");
}

static void
print_debug_params(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  stp_parameter_list_t params = stp_get_parameter_list(v);
  int count = stp_parameter_list_count(params);
  int i;
  print_remote_param(v, "Package", PACKAGE);
  print_remote_param(v, "Version", VERSION);
  print_remote_param(v, "Release Date", RELEASE_DATE);
  print_remote_param(v, "Driver", stp_get_driver(v));
  print_remote_int_param(v, "Output Type", stp_get_output_type(v));
  print_remote_int_param(v, "Left", stp_get_left(v));
  print_remote_int_param(v, "Top", stp_get_top(v));
  print_remote_int_param(v, "Page Width", stp_get_page_width(v));
  print_remote_int_param(v, "Page Height", stp_get_page_height(v));
  print_remote_int_param(v, "Input Model", stp_get_input_color_model(v));
  print_remote_int_param(v, "Output Model", stpi_get_output_color_model(v));
  print_remote_int_param(v, "Model", stpi_get_model_id(v));
  print_remote_int_param(v, "Ydpi", pd->res->vres);
  print_remote_int_param(v, "Xdpi", pd->res->hres);
  print_remote_int_param(v, "Use_softweave", pd->res->softweave);
  print_remote_int_param(v, "Use_microweave", pd->res->microweave);
  print_remote_int_param(v, "Page_left", pd->page_left);
  print_remote_int_param(v, "Page_right", pd->page_right);
  print_remote_int_param(v, "Page_top", pd->page_top);
  print_remote_int_param(v, "Page_bottom", pd->page_bottom);
  print_remote_int_param(v, "Page_width", pd->page_width);
  print_remote_int_param(v, "Page_height", pd->page_height);
  print_remote_int_param(v, "Page_true_height", pd->page_true_height);
  print_remote_int_param(v, "Image_left", pd->image_left);
  print_remote_int_param(v, "Image_top", pd->image_top);
  print_remote_int_param(v, "Image_width", pd->image_width);
  print_remote_int_param(v, "Image_height", pd->image_height);
  print_remote_int_param(v, "Image_scaled_width", pd->image_scaled_width);
  print_remote_int_param(v, "Image_scaled_height", pd->image_scaled_height);
  print_remote_int_param(v, "Image_left_position", pd->image_left_position);
  print_remote_int_param(v, "Nozzles", pd->nozzles);
  print_remote_int_param(v, "Nozzle_separation", pd->nozzle_separation);
  print_remote_int_param(v, "Horizontal_passes", pd->horizontal_passes);
  print_remote_int_param(v, "Vertical_passes", pd->res->vertical_passes);
  print_remote_int_param(v, "Vertical_oversample", pd->res->vertical_oversample);
  print_remote_int_param(v, "Vertical_undersample", pd->res->vertical_undersample);
  print_remote_int_param(v, "Vertical_denominator", pd->res->vertical_denominator);
  print_remote_int_param(v, "Physical_xdpi", pd->physical_xdpi);
  print_remote_int_param(v, "Page_management_units", pd->page_management_units);
  print_remote_int_param(v, "Vertical_units", pd->vertical_units);
  print_remote_int_param(v, "Horizontal_units", pd->horizontal_units);
  print_remote_int_param(v, "Micro_units", pd->micro_units);
  print_remote_int_param(v, "Unit_scale", pd->unit_scale);
  print_remote_int_param(v, "Bits", pd->bitwidth);
  print_remote_int_param(v, "Resid", pd->ink_resid);
  print_remote_int_param(v, "Drop Size", pd->drop_size);
  print_remote_int_param(v, "Initial_vertical_offset", pd->initial_vertical_offset);
  print_remote_int_param(v, "Channels_in_use", pd->channels_in_use);
  print_remote_int_param(v, "Logical_channels", pd->logical_channels);
  print_remote_int_param(v, "Physical_channels", pd->physical_channels);
  print_remote_int_param(v, "Use_black_parameters", pd->use_black_parameters);
  print_remote_int_param(v, "Use_fast_360", pd->use_fast_360);
  print_remote_param(v, "Ink name", pd->inkname->name);
  print_remote_int_param(v, "  is_color", pd->inkname->is_color);
  print_remote_int_param(v, "  channels", pd->inkname->channel_limit);
  print_remote_int_param(v, "  inkset", pd->inkname->inkset);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      switch (p->p_type)
	{
	case STP_PARAMETER_TYPE_DOUBLE:
	  if (stp_check_float_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    print_remote_float_param(v, p->name,
				     stp_get_float_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_INT:
	  if (stp_check_int_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    print_remote_int_param(v, p->name,
				   stp_get_int_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_STRING_LIST:
	  if (stp_check_string_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    print_remote_param(v, p->name,
			       stp_get_string_parameter(v, p->name));
	  break;
	case STP_PARAMETER_TYPE_CURVE:
	  if (stp_check_curve_parameter(v, p->name, STP_PARAMETER_DEFAULTED))
	    {
	      char *curve =
		stp_curve_write_string(stp_get_curve_parameter(v, p->name));
	      print_remote_param(v, p->name, curve);
	      stpi_free(curve);
	    }
	  break;
	default:
	  break;
	}
    }
  stp_parameter_list_free(params);
  stpi_send_command(v, "\033", "c", 0);
}

static void
escp2_set_remote_sequence(stp_vars_t v)
{
  /* Magic remote mode commands, whatever they do */
  escp2_privdata_t *pd = get_privdata(v);
  const input_slot_t *input_slot = get_input_slot(v);

  if (stpi_debug_level & STPI_DBG_MARK_FILE)
    print_debug_params(v);
  if (escp2_has_advanced_command_set(v) || input_slot)
    {
      int feed_sequence = 0;
      const paper_t *p = get_media_type(v);
      /* Enter remote mode */
      stpi_send_command(v, "\033(R", "bcs", 0, "REMOTE1");
      if (escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO))
	{
	  if (p)
	    {
	      stpi_send_command(v, "PH", "bcc", 0, p->paper_thickness);
	      if (escp2_has_cap(v, MODEL_VACUUM, MODEL_VACUUM_YES))
		stpi_send_command(v, "SN", "bccc", 0, 5, p->vacuum_intensity);
	      stpi_send_command(v, "SN", "bccc", 0, 4, p->feed_adjustment);
	    }
	}
      else if (escp2_has_advanced_command_set(v))
	{
	  if (p)
	    feed_sequence = p->paper_feed_sequence;
	  /* Function unknown */
	  stpi_send_command(v, "PM", "bh", 0);
	  /* Set mechanism sequence */
	  stpi_send_command(v, "SN", "bccc", 0, 0, feed_sequence);
	  if (stp_get_boolean_parameter(v, "FullBleed"))
	    stpi_send_command(v, "FP", "bch", 0, 0xffb0);
	}
      if (input_slot)
	{
	  int divisor = escp2_base_separation(v) / 360;
	  int height = pd->page_true_height * 5 / divisor;
	  if (input_slot->init_sequence.bytes)
	    stpi_zfwrite(input_slot->init_sequence.data,
			 input_slot->init_sequence.bytes, 1, v);
	  switch (input_slot->roll_feed_cut_flags)
	    {
	    case ROLL_FEED_CUT_ALL:
	      stpi_send_command(v, "JS", "bh", 0);
	      stpi_send_command(v, "CO", "bccccl", 0, 0, 1, 0, 0);
	      stpi_send_command(v, "CO", "bccccl", 0, 0, 0, 0, height);
	      break;
	    case ROLL_FEED_CUT_LAST:
	      stpi_send_command(v, "CO", "bccccl", 0, 0, 1, 0, 0);
	      stpi_send_command(v, "CO", "bccccl", 0, 0, 2, 0, height);
	      break;
	    default:
	      break;
	    }
	}

      /* Exit remote mode */

      stpi_send_command(v, "\033", "ccc", 0, 0, 0);
    }
}

static void
escp2_set_graphics_mode(stp_vars_t v)
{
  stpi_send_command(v, "\033(G", "bc", 1);
}

static void
escp2_set_resolution(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (escp2_use_extended_commands(v, pd->res->softweave))
    stpi_send_command(v, "\033(U", "bccch",
		      pd->unit_scale / pd->page_management_units,
		      pd->unit_scale / pd->vertical_units,
		      pd->unit_scale / pd->horizontal_units,
		      pd->unit_scale);
  else
    stpi_send_command(v, "\033(U", "bc",
		      pd->unit_scale / pd->page_management_units);
}

static void
escp2_set_color(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (pd->use_fast_360)
    stpi_send_command(v, "\033(K", "bcc", 0, 3);
  else if (escp2_has_cap(v, MODEL_GRAYMODE, MODEL_GRAYMODE_YES))
    stpi_send_command(v, "\033(K", "bcc", 0,
		      (pd->use_black_parameters ? 1 : 2));
}

static void
escp2_set_microweave(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  stpi_send_command(v, "\033(i", "bc", pd->res->microweave);
}

static void
escp2_set_printhead_speed(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  const char *direction = stp_get_string_parameter(v, "PrintingDirection");
  int unidirectional;
  if (direction && strcmp(direction, "Unidirectional") == 0)
    unidirectional = 1;
  else if (direction && strcmp(direction, "Bidirectional") == 0)
    unidirectional = 0;
  else if (pd->res->hres >= 720 && pd->res->vres >= 720)
    unidirectional = 1;
  else
    unidirectional = 0;
  if (unidirectional)
    {
      stpi_send_command(v, "\033U", "c", 1);
      if (pd->res->hres > escp2_base_res(v, pd->res->resid))
	stpi_send_command(v, "\033(s", "bc", 2);
    }
  else
    stpi_send_command(v, "\033U", "c", 0);
}

static void
escp2_set_dot_size(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  /* Dot size */
  if (pd->drop_size >= 0)
    stpi_send_command(v, "\033(e", "bcc", 0, pd->drop_size);
}

static void
escp2_set_page_height(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int l = pd->page_management_units * pd->page_true_height / 72;
  if (escp2_use_extended_commands(v, pd->res->softweave))
    stpi_send_command(v, "\033(C", "bl", l);
  else
    stpi_send_command(v, "\033(C", "bh", l);
}

static void
escp2_set_margins(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int bot = pd->page_bottom;
  int top = pd->page_management_units * pd->page_top / 72;

  /* adjust bottom margin for a 480 like head configuration */
  bot -= pd->max_head_offset * 72 / pd->page_management_units;
  if ((pd->max_head_offset * 72 % pd->page_management_units) != 0)
    bot -= 1;
  if (pd->page_bottom < 0)
    bot = 0;

  bot = bot * pd->page_management_units / 72;

  top += pd->initial_vertical_offset;
  if (escp2_use_extended_commands(v, pd->res->softweave) &&
      (escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_2000)||
       escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO)))
    stpi_send_command(v, "\033(c", "bll", top, bot);
  else
    stpi_send_command(v, "\033(c", "bhh", top, bot);
}

static void
escp2_set_form_factor(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (escp2_has_advanced_command_set(v))
    {
      int w = pd->page_width * pd->page_management_units / 72;
      int h = pd->page_true_height * pd->page_management_units / 72;

      if (stp_get_boolean_parameter(v, "FullBleed"))
	/* Make the page 160/360" wider for full bleed printing. */
	/* Per the Epson manual, the margin should be expanded by 80/360" */
	/* so we need to do this on the left and the right */
	w += 320 * pd->page_management_units / 720;

      stpi_send_command(v, "\033(S", "bll", w, h);
    }
}

static void
escp2_set_printhead_resolution(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (escp2_use_extended_commands(v, pd->res->softweave))
    {
      int xres;
      int yres;
      int scale = escp2_resolution_scale(v);

      xres = scale / pd->physical_xdpi;

      if (escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO) &&
	  !pd->res->softweave)
	yres = scale / pd->res->vres;
      else
	yres = (pd->nozzle_separation * scale / escp2_base_separation(v));

      /* Magic resolution cookie */
      stpi_send_command(v, "\033(D", "bhcc", scale, yres, xres);
    }
}

static void
escp2_init_printer(stp_vars_t v)
{
  escp2_reset_printer(v);
  escp2_set_remote_sequence(v);
  escp2_set_graphics_mode(v);
  escp2_set_resolution(v);
  escp2_set_color(v);
  escp2_set_microweave(v);
  escp2_set_printhead_speed(v);
  escp2_set_dot_size(v);
  escp2_set_printhead_resolution(v);
  escp2_set_page_height(v);
  escp2_set_margins(v);
  escp2_set_form_factor(v);
}

static void
escp2_deinit_printer(stp_vars_t v)
{
  const input_slot_t *input_slot = get_input_slot(v);
  stpi_puts("\033@", v);	/* ESC/P2 reset */
  if (escp2_has_advanced_command_set(v) || input_slot)
    {
      const stp_raw_t *deinit = escp2_postinit_remote_sequence(v);
      stpi_send_command(v, "\033(R", "bcs", 0, "REMOTE1");
      if (input_slot && input_slot->deinit_sequence.bytes)
	stpi_zfwrite(input_slot->deinit_sequence.data,
		     input_slot->deinit_sequence.bytes, 1, v);
      /* Load settings from NVRAM */
      stpi_send_command(v, "LD", "b");

      /* Magic deinit sequence reported by Simone Falsini */
      if (deinit)
	stpi_zfwrite(deinit->data, deinit->bytes, 1, v);
      /* Exit remote mode */
      stpi_send_command(v, "\033", "ccc", 0, 0, 0);
    }
}

static void
set_vertical_position(stp_vars_t v, stpi_pass_t *pass)
{
  escp2_privdata_t *pd = get_privdata(v);
  int advance = pass->logicalpassstart - pd->last_pass_offset -
    (escp2_separation_rows(v) - 1);
  advance *= pd->res->vertical_undersample;
  if (pass->logicalpassstart > pd->last_pass_offset ||
      pd->printing_initial_vertical_offset != 0)
    {
      advance += pd->printing_initial_vertical_offset;
      pd->printing_initial_vertical_offset = 0;
      if (escp2_use_extended_commands(v, pd->nozzles > 1))
	stpi_send_command(v, "\033(v", "bl", advance);
      else
	stpi_send_command(v, "\033(v", "bh", advance);
      pd->last_pass_offset = pass->logicalpassstart;
    }
}

static void
set_color(stp_vars_t v, stpi_pass_t *pass, int color)
{
  escp2_privdata_t *pd = get_privdata(v);
  if (pd->last_color != color &&
      ! escp2_use_extended_commands(v, pd->nozzles > 1))
    {
      int ncolor = pd->channels[color]->color;
      int subchannel = pd->channels[color]->subchannel;
      if (subchannel >= 0)
	stpi_send_command(v, "\033(r", "bcc", subchannel, ncolor);
      else
	stpi_send_command(v, "\033r", "c", ncolor);
      pd->last_color = color;
    }
}

static void
set_horizontal_position(stp_vars_t v, stpi_pass_t *pass, int vertical_subpass)
{
  escp2_privdata_t *pd = get_privdata(v);
  int microoffset = vertical_subpass & (pd->horizontal_passes - 1);
  int pos = pd->image_left_position + microoffset;

  if (pos != 0)
    {
      /* Note hard-coded 1440 -- from Epson manuals */
      if (!escp2_has_advanced_command_set(v) && pd->res->hres < 1440)
	stpi_send_command(v, "\033\\", "h", pos);
      else if (escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO) ||
	       (escp2_has_advanced_command_set(v) &&
		escp2_has_cap(v, MODEL_VARIABLE_DOT, MODEL_VARIABLE_YES)))
	stpi_send_command(v, "\033($", "bl", pos);
      else
	stpi_send_command(v, "\033(\\", "bhh", pd->micro_units, pos);
    }
}

static void
send_print_command(stp_vars_t v, stpi_pass_t *pass, int color, int nlines)
{
  escp2_privdata_t *pd = get_privdata(v);
  int lwidth = (pd->image_scaled_width + (pd->horizontal_passes - 1)) /
    pd->horizontal_passes;
  int ydpi = pd->vertical_units;
  if (!escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO) &&
      pd->nozzles == 1 && pd->bitwidth == 1)
    {
      int ygap = 3600 / ydpi;
      int xgap = 3600 / pd->res->hres;
      if (ydpi == 720 && escp2_extra_720dpi_separation(v))
	ygap *= escp2_extra_720dpi_separation(v);
      stpi_send_command(v, "\033.", "cccch", COMPRESSION, ygap, xgap, 1,
			lwidth);
    }
  else if (!escp2_has_cap(v, MODEL_COMMAND, MODEL_COMMAND_PRO) &&
	   escp2_has_cap(v, MODEL_VARIABLE_DOT, MODEL_VARIABLE_NO))
    {
      int ygap = 3600 / ydpi;
      int xgap = 3600 / pd->physical_xdpi;
      if (escp2_extra_720dpi_separation(v))
	ygap *= escp2_extra_720dpi_separation(v);
      else if (escp2_pseudo_separation_rows(v) > 0)
	ygap *= escp2_pseudo_separation_rows(v);
      else
	ygap *= escp2_separation_rows(v);
      stpi_send_command(v, "\033.", "cccch", COMPRESSION, ygap, xgap, nlines,
			lwidth);
    }
  else
    {
      int ncolor = pd->channels[color]->color;
      int nwidth = pd->bitwidth * ((lwidth + 7) / 8);
      if (pd->channels[color]->subchannel >= 0)
	ncolor |= (pd->channels[color]->subchannel << 4);
      stpi_send_command(v, "\033i", "ccchh", ncolor, COMPRESSION,
			pd->bitwidth, nwidth, nlines);
    }
}

static void
send_extra_data(stp_vars_t v, int extralines)
{
  escp2_privdata_t *pd = get_privdata(v);
  int lwidth = (pd->image_scaled_width + (pd->horizontal_passes - 1)) /
    pd->horizontal_passes;
#if TEST_UNCOMPRESSED
  int i;
  for (i = 0; i < pd->bitwidth * (lwidth + 7) / 8; i++)
    stpi_putc(0, v);
#else  /* !TEST_UNCOMPRESSED */
  int k, l;
  int bytes_to_fill = pd->bitwidth * ((lwidth + 7) / 8);
  int full_blocks = bytes_to_fill / 128;
  int leftover = bytes_to_fill % 128;
  int total_bytes = extralines * (full_blocks + 1) * 2;
  unsigned char *buf = stpi_malloc(total_bytes);
  total_bytes = 0;
  for (k = 0; k < extralines; k++)
    {
      for (l = 0; l < full_blocks; l++)
	{
	  buf[total_bytes++] = 129;
	  buf[total_bytes++] = 0;
	}
      if (leftover == 1)
	{
	  buf[total_bytes++] = 1;
	  buf[total_bytes++] = 0;
	}
      else if (leftover > 0)
	{
	  buf[total_bytes++] = 257 - leftover;
	  buf[total_bytes++] = 0;
	}
    }
  stpi_zfwrite((const char *) buf, total_bytes, 1, v);
  stpi_free(buf);
#endif /* TEST_UNCOMPRESSED */
}

static void
flush_pass(stp_vars_t v, int passno, int vertical_subpass)
{
  int j;
  escp2_privdata_t *pd = get_privdata(v);
  stpi_lineoff_t *lineoffs = stpi_get_lineoffsets_by_pass(v, passno);
  stpi_lineactive_t *lineactive = stpi_get_lineactive_by_pass(v, passno);
  const stpi_linebufs_t *bufs = stpi_get_linebases_by_pass(v, passno);
  stpi_pass_t *pass = stpi_get_pass_by_pass(v, passno);
  stpi_linecount_t *linecount = stpi_get_linecount_by_pass(v, passno);
  int minlines = pd->min_nozzles;

  for (j = 0; j < pd->channels_in_use; j++)
    {
      if (lineactive[0].v[j] > 0)
	{
	  int nlines = linecount[0].v[j];
	  int extralines = 0;
	  if (nlines < minlines)
	    {
	      extralines = minlines - nlines;
	      nlines = minlines;
	    }
	  set_vertical_position(v, pass);
	  set_color(v, pass, j);
	  set_horizontal_position(v, pass, vertical_subpass);
	  send_print_command(v, pass, j, nlines);

	  /*
	   * Send the data
	   */
	  stpi_zfwrite((const char *)bufs[0].v[j], lineoffs[0].v[j], 1, v);
	  if (extralines)
	    send_extra_data(v, extralines);
	  stpi_send_command(v, "\r", "");
	  pd->printed_something = 1;
	}
      lineoffs[0].v[j] = 0;
      linecount[0].v[j] = 0;
    }
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
	inks->inknames[i]->channel_limit * 2 == stpi_image_bpp(image))
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
  const paper_t *pt = get_media_type(v);
  double paper_density = .8;

  if (pt)
    paper_density = pt->base_density;

  if (pd->rescale_density)
    stp_scale_float_parameter
      (v, "Density", paper_density * escp2_density(v, pd->res->resid));
  pd->drop_size = escp2_ink_type(v, pd->res->resid);
  pd->ink_resid = pd->res->resid;

  /*
   * If density is greater than 1, try to find the dot size from a lower
   * resolution that will let us print.  This allows use of high ink levels
   * on special paper types that need a lot of ink.
   */
  if (stp_get_float_parameter(v, "Density") > 1.0)
    {
      if (stp_check_int_parameter(v, "escp2_ink_type", STP_PARAMETER_ACTIVE) ||
	  stp_check_int_parameter(v, "escp2_density", STP_PARAMETER_ACTIVE) ||
	  stp_check_int_parameter(v, "escp2_bits", STP_PARAMETER_ACTIVE))
	{
	  stp_set_float_parameter(v, "Density", 1.0);
	}
      else
	{
	  double density = stp_get_float_parameter(v, "Density");
	  int resid = pd->res->resid;
	  int xresid = resid;
	  double xdensity = density;
	  while (density > 1.0 && resid >= RES_360)
	    {
	      int tresid = xresid - 2;
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
  stp_curve_t   lum_adjustment = NULL;
  stp_curve_t   sat_adjustment = NULL;
  stp_curve_t   hue_adjustment = NULL;
  const paper_t *pt;
  double k_upper, k_lower;
  double paper_k_upper;
  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */
  k_lower = pd->inkname->k_lower;
  k_upper = pd->inkname->k_upper;

  pt = get_media_type(v);
  if (pt)
    {
      k_lower *= pt->k_lower_scale;
      paper_k_upper = pt->k_upper;
      k_upper *= pt->k_upper;
      if (pd->channels_in_use >= 5)
	{
	  stp_scale_float_parameter(v, "Cyan", pt->p_cyan);
	  stp_scale_float_parameter(v, "Magenta", pt->p_magenta);
	  stp_scale_float_parameter(v, "Yellow", pt->p_yellow);
	}
      else
	{
	  stp_scale_float_parameter(v, "Cyan", pt->cyan);
	  stp_scale_float_parameter(v, "Magenta", pt->magenta);
	  stp_scale_float_parameter(v, "Yellow", pt->yellow);
	}
      stp_scale_float_parameter(v, "Saturation", pt->saturation);
      stp_scale_float_parameter(v, "Gamma", pt->gamma);
    }
  else				/* Assume some kind of plain paper */
    {
      k_lower *= .1;
      paper_k_upper = .5;
      k_upper *= .5;
    }

  if (!stp_check_float_parameter(v, "GCRLower", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRLower", k_lower);
  if (!stp_check_float_parameter(v, "GCRUpper", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(v, "GCRUpper", k_upper);

  if (!stp_check_curve_parameter(v, "HueMap", STP_PARAMETER_ACTIVE))
    {
      hue_adjustment = stpi_read_and_compose_curves
	(pd->inkname->hue_adjustment, pt ? pt->hue_adjustment : NULL,
	 STP_CURVE_COMPOSE_ADD);
      stp_set_curve_parameter(v, "HueMap", hue_adjustment);
      stp_set_curve_parameter_active(v, "HueMap", STP_PARAMETER_ACTIVE);
      stp_curve_free(hue_adjustment);
    }
  if (!stp_check_curve_parameter(v, "SatMap", STP_PARAMETER_ACTIVE))
    {
      sat_adjustment = stpi_read_and_compose_curves
	(pd->inkname->sat_adjustment, pt ? pt->sat_adjustment : NULL,
	 STP_CURVE_COMPOSE_MULTIPLY);
      stp_set_curve_parameter(v, "SatMap", sat_adjustment);
      stp_set_curve_parameter_active(v, "SatMap", STP_PARAMETER_ACTIVE);
      stp_curve_free(sat_adjustment);
    }
  if (!stp_check_curve_parameter(v, "LumMap", STP_PARAMETER_ACTIVE))
    {
      lum_adjustment = stpi_read_and_compose_curves
	(pd->inkname->lum_adjustment, pt ? pt->lum_adjustment : NULL,
	 STP_CURVE_COMPOSE_MULTIPLY);
      stp_set_curve_parameter(v, "LumMap", lum_adjustment);
      stp_set_curve_parameter_active(v, "LumMap", STP_PARAMETER_ACTIVE);
      stp_curve_free(lum_adjustment);
    }
}

static int
count_channels(const escp2_inkname_t *inks)
{
  int answer = 0;
  int i;
  for (i = 0; i < inks->channel_limit; i++)
    if (inks->channels[i])
      answer += inks->channels[i]->n_subchannels;
  return answer;
}

static const physical_subchannel_t default_black_subchannels[] =
{
  { 0, 0, 0 }
};

static const ink_channel_t default_black_channels =
{
  default_black_subchannels, 1
};

static const escp2_inkname_t default_black_ink =
{
  NULL, NULL, 0, 0, 0, 0, 1, NULL, NULL, NULL,
  {
    &default_black_channels, NULL, NULL, NULL
  },
  {
    NULL, NULL, NULL, NULL
  }
};

static int
compute_channel_count(const escp2_inkname_t *ink_type, int channel_limit)
{
  int i;
  int physical_channels = 0;
  for (i = 0; i < channel_limit; i++)
    {
      const ink_channel_t *channel = ink_type->channels[i];
      if (channel)
	physical_channels += channel->n_subchannels;
    }
  return physical_channels;
}

static void
setup_inks(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int i;
  const escp2_variable_inkset_t *inks;
  const paper_t *pt;
  double paper_k_upper = 0.5;

  pt = get_media_type(v);
  if (pt)
    paper_k_upper = pt->k_upper;
  inks = escp2_inks(v, pd->ink_resid, pd->inkname->inkset);
  if (inks)
    {
      stpi_init_debug_messages(v);
      for (i = 0; i < pd->logical_channels; i++)
	{
	  const escp2_variable_ink_t *ink = (*inks)[i];
	  if (ink)
	    {
	      const char *param = pd->inkname->channel_parameter_names[i];
	      double userval = 1.0;
	      if (param && stp_check_float_parameter(v, param,
						     STP_PARAMETER_ACTIVE))
		userval = stp_get_float_parameter(v, param);
	      stpi_dither_set_ranges(v, i, ink->numranges, ink->range,
				     ink->darkness * paper_k_upper * userval);

	      stpi_dither_set_shades(v, i, ink->numshades, ink->shades,
				     ink->darkness * paper_k_upper * userval);
	    }
	}
      stpi_flush_debug_messages(v);
    }
}

static void
setup_head_offset(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int i;
  int channel_id = 0;
  const escp2_inkname_t *ink_type = pd->inkname;
  pd->head_offset = stpi_zalloc(sizeof(int) * pd->channels_in_use);
  memset(pd->head_offset, 0, sizeof(pd->head_offset));
  for (i = 0; i < pd->logical_channels; i++)
    {
      const ink_channel_t *channel = ink_type->channels[i];
      if (channel)
	{
	  int j;
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      pd->head_offset[channel_id] = channel->channels[j].head_offset;
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
      const ink_channel_t *channel = ink_type->channels[i];
      if (channel)
	{
	  int j;
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      pd->cols[channel_id] = stpi_zalloc(line_length);
	      pd->channels[channel_id] = &(channel->channels[j]);
	      stpi_dither_add_channel(v, pd->cols[channel_id], i, j);
	      channel_id++;
	    }
	}
    }
}

static void
setup_resolution(stp_vars_t v)
{
  escp2_privdata_t *pd = get_privdata(v);
  int vertical;
  int horizontal;
  pd->res =
    escp2_find_resolution(v, stp_get_string_parameter(v, "Resolution"));
  vertical = pd->res->vres * pd->res->vertical_undersample;
  horizontal = pd->res->hres * pd->res->vertical_denominator;

  pd->physical_xdpi = escp2_base_res(v, pd->res->resid);
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
  pd->horizontal_passes = pd->res->hres / escp2_base_res(v, pd->res->resid);
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
      pd->inkname = &default_black_ink;
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
  pd->bitwidth = escp2_bits(v, pd->res->resid);
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
escp2_print_data(stp_vars_t v, stp_image_t *image, unsigned short *out)
{
  escp2_privdata_t *pd = get_privdata(v);
  int errdiv  = stpi_image_height(image) / pd->image_scaled_height;
  int errmod  = stpi_image_height(image) % pd->image_scaled_height;
  int errval  = 0;
  int errlast = -1;
  int errline  = 0;
  int y;

  stpi_image_progress_init(image);

  QUANT(0);
  for (y = 0; y < pd->image_scaled_height; y ++)
    {
      int duplicate_line = 1;
      int zero_mask;
      if ((y & 63) == 0)
	stpi_image_note_progress(image, y, pd->image_scaled_height);

      if (errline != errlast)
	{
	  errlast = errline;
	  duplicate_line = 0;
	  if (stpi_color_get_row(v, image, errline, out, &zero_mask))
	    return 2;
	}
      QUANT(1);

      stpi_dither(v, y, out, duplicate_line, zero_mask);
      QUANT(2);

      stpi_write_weave(v, pd->cols);
      QUANT(3);
      errval += errmod;
      errline += errdiv;
      if (errval >= pd->image_scaled_height)
	{
	  errval -= pd->image_scaled_height;
	  errline ++;
	}
      QUANT(4);
    }
  stpi_image_progress_conclude(image);
  stpi_flush_all(v);
  QUANT(5);
  return 1;
}  

static int
escp2_print_page(stp_vars_t v, stp_image_t *image)
{
  int status;
  int i;
  escp2_privdata_t *pd = get_privdata(v);
  unsigned short *out;	/* Output pixels (16-bit) */
  int out_channels;		/* Output bytes per pixel */
  int line_width = (pd->image_scaled_width + 7) / 8 * pd->bitwidth;

  stpi_initialize_weave
    (v,
     pd->nozzles,
     pd->nozzle_separation * pd->res->vres / escp2_base_separation(v),
     pd->horizontal_passes,
     pd->res->vertical_passes,
     pd->res->vertical_oversample,
     pd->channels_in_use,
     pd->bitwidth,
     pd->image_scaled_width,
     pd->image_scaled_height,
     pd->image_top * pd->res->vres / 72,
     (pd->page_height + escp2_extra_feed(v)) * pd->res->vres / 72,
     pd->head_offset,
     STPI_WEAVE_ZIGZAG,
     flush_pass,
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

  out = stpi_malloc(stpi_image_width(image) * out_channels * 2);

  status = escp2_print_data(v, image, out);

  /*
   * Cleanup...
   */
  stpi_free(out);
  if (!pd->printed_something)
    stpi_send_command(v, "\n", "");
  stpi_send_command(v, "\f", "");	/* Eject page */
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
  stpi_allocate_component_data(v, "Driver", NULL, NULL, pd);

  if (stp_get_output_type(v) == OUTPUT_RAW_CMYK ||
      stp_get_output_type(v) == OUTPUT_RAW_PRINTER)
    pd->rescale_density = 0;
  else
    pd->rescale_density = 1;

  pd->inkname = get_inktype(v);
  pd->channels_in_use = count_channels(pd->inkname);
  if (stp_get_output_type(v) != OUTPUT_RAW_PRINTER && !(pd->inkname->is_color))
    stp_set_output_type(v, OUTPUT_GRAY);
  if (stp_get_output_type(v) == OUTPUT_COLOR &&
      pd->inkname->channels[0] != NULL)
    stp_set_output_type(v, OUTPUT_RAW_CMYK);

  setup_resolution(v);
  setup_head_parameters(v);
  setup_page(v);

  adjust_density_and_ink_type(v, image);
  if (print_op & OP_JOB_START)
    escp2_init_printer(v);
  if (print_op & OP_JOB_PRINT)
    status = escp2_print_page(v, image);
  if (print_op & OP_JOB_END)
    escp2_deinit_printer(v);

  stpi_free(pd->head_offset);
  stpi_free(pd);

#ifdef QUANTIFY
  print_timers(v);
#endif
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

static const stpi_printfuncs_t stpi_escp2_printfuncs =
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

static stpi_internal_family_t stpi_escp2_module_data =
  {
    &stpi_escp2_printfuncs,
    NULL
  };


static int
escp2_module_init(void)
{
  return stpi_family_register(stpi_escp2_module_data.printer_list);
}


static int
escp2_module_exit(void)
{
  return stpi_family_unregister(stpi_escp2_module_data.printer_list);
}


/* Module header */
#define stpi_module_version escp2_LTX_stpi_module_version
#define stpi_module_data escp2_LTX_stpi_module_data

stpi_module_version_t stpi_module_version = {0, 0};

stpi_module_t stpi_module_data =
  {
    "escp2",
    VERSION,
    "Epson family driver",
    STPI_MODULE_CLASS_FAMILY,
    NULL,
    escp2_module_init,
    escp2_module_exit,
    (void *) &stpi_escp2_module_data
  };

