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
#if defined(HAVE_VARARGS_H) && !defined(HAVE_STDARG_H)
#include <varargs.h>
#else
#include <stdarg.h>
#endif
#include <string.h>
#include <stdio.h>
#include "print-escp2.h"
#include "module.h"

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

#define BYTE(expr, byteno) (((expr) >> (8 * byteno)) & 0xff)

static void flush_pass(stpi_softweave_t *sw, int passno, int vertical_subpass);

static const int dotidmap[] =
{ 0, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 10, 11, 12, 12 };

static int
resid2dotid(int resid)
{
  if (resid < 0 || resid >= RES_N)
    return -1;
  return dotidmap[resid];
}

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

static const res_t *escp2_find_resolution(int model, const stp_vars_t v,
					  const char *resolution);

typedef struct
{
  int undersample;
  int denominator;
  int initial_vertical_offset;
  int min_nozzles;
  int printed_something;
  int last_color;
  int hoffset;
  int model;
  int width;
  int ydpi;
  int xdpi;
  int physical_xdpi;
  const physical_subchannel_t **channels;
} escp2_privdata_t;

typedef struct escp2_init
{
  int model;
  int output_type;
  int ydpi;
  int xdpi;
  int physical_xdpi;
  int page_true_height;
  int page_width;
  int page_top;
  int page_bottom;
  int nozzles;
  int nozzle_separation;
  int horizontal_passes;
  int bits;
  int drop_size;
  int ink_resid;
  int initial_vertical_offset;
  int total_channels;
  int use_black_parameters;
  int channel_limit;
  int use_fast_360;
  int print_op;
  int rescale_density;
  const res_t *res;
  const escp2_inkname_t *inkname;
  const input_slot_t *input_slot;
  stp_vars_t v;
} escp2_init_t;

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
    "InkType", N_("Ink Type"),
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
  },
  {
    "Resolution", N_("Resolution"),
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
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
  PARAMETER_INT(base_resolution),
  PARAMETER_INT(enhanced_resolution),
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
  PARAMETER_INT(roll_left_margin),
  PARAMETER_INT(roll_right_margin),
  PARAMETER_INT(roll_top_margin),
  PARAMETER_INT(roll_bottom_margin),
  PARAMETER_RAW(preinit_sequence),
  PARAMETER_RAW(postinit_remote_sequence)
};

static int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

static int
escp2_has_cap(int model, escp2_model_option_t feature,
	      model_featureset_t class, const stp_vars_t v)
{
  if (feature < 0 || feature >= MODEL_LIMIT)
    return -1;
  else
    {
      model_featureset_t featureset =
	(((1ul << escp2_printer_attrs[feature].bit_width) - 1ul) <<
	 escp2_printer_attrs[feature].bit_shift);
      return ((stpi_escp2_model_capabilities[model].flags & featureset)==class);
    }
}

#define DEF_SIMPLE_ACCESSOR(f, t)					\
static t								\
escp2_##f(int model, const stp_vars_t v)				\
{									\
  if (stp_check_int_parameter(v, "escp2_" #f, STP_PARAMETER_ACTIVE))	\
    return stp_get_int_parameter(v, "escp2_" #f);			\
  return (stpi_escp2_model_capabilities[model].f);			\
}

#define DEF_RAW_ACCESSOR(f, t)						\
static t								\
escp2_##f(int model, const stp_vars_t v)				\
{									\
  if (stp_check_raw_parameter(v, "escp2_" #f, STP_PARAMETER_ACTIVE))	\
    return stp_get_raw_parameter(v, "escp2_" #f);			\
  return (stpi_escp2_model_capabilities[model].f);			\
}

#define DEF_COMPOSITE_ACCESSOR(f, t)			\
static t						\
escp2_##f(int model, const stp_vars_t v)		\
{							\
  return (stpi_escp2_model_capabilities[model].f);	\
}

#define DEF_MICROWEAVE_ACCESSOR(f, t)					\
static t								\
escp2_##f(int model, const stp_vars_t v)				\
{									\
  const res_t *res =							\
    escp2_find_resolution(model, v,					\
			  stp_get_string_parameter(v, "Resolution"));	\
  if (stp_check_int_parameter(v, "escp2_" #f, STP_PARAMETER_ACTIVE))	\
    return stp_get_int_parameter(v, "escp2_" #f);			\
  if (res && !(res->softweave))						\
    return (stpi_escp2_model_capabilities[model].m_##f);		\
  else									\
    return (stpi_escp2_model_capabilities[model].f);			\
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
DEF_SIMPLE_ACCESSOR(base_resolution, int)
DEF_SIMPLE_ACCESSOR(enhanced_resolution, int)
DEF_SIMPLE_ACCESSOR(resolution_scale, int)
DEF_SIMPLE_ACCESSOR(initial_vertical_offset, int)
DEF_SIMPLE_ACCESSOR(black_initial_vertical_offset, int)
DEF_SIMPLE_ACCESSOR(max_black_resolution, int)
DEF_SIMPLE_ACCESSOR(zero_margin_offset, int)
DEF_SIMPLE_ACCESSOR(extra_720dpi_separation, int)
DEF_SIMPLE_ACCESSOR(physical_channels, int)

DEF_MICROWEAVE_ACCESSOR(left_margin, unsigned)
DEF_MICROWEAVE_ACCESSOR(right_margin, unsigned)
DEF_MICROWEAVE_ACCESSOR(top_margin, unsigned)
DEF_MICROWEAVE_ACCESSOR(bottom_margin, unsigned)
DEF_MICROWEAVE_ACCESSOR(roll_left_margin, unsigned)
DEF_MICROWEAVE_ACCESSOR(roll_right_margin, unsigned)
DEF_MICROWEAVE_ACCESSOR(roll_top_margin, unsigned)
DEF_MICROWEAVE_ACCESSOR(roll_bottom_margin, unsigned)

DEF_RAW_ACCESSOR(preinit_sequence, const stp_raw_t *)
DEF_RAW_ACCESSOR(postinit_remote_sequence, const stp_raw_t *)

DEF_COMPOSITE_ACCESSOR(paperlist, const paperlist_t *)
DEF_COMPOSITE_ACCESSOR(reslist, const res_t *)
DEF_COMPOSITE_ACCESSOR(inklist, const inklist_t *)
DEF_COMPOSITE_ACCESSOR(input_slots, const input_slot_list_t *)

static int
escp2_ink_type(int model, int resid, const stp_vars_t v)
{
  int dotid = resid2dotid(resid);
  if (stp_check_int_parameter(v, "escp2_ink_type", STP_PARAMETER_ACTIVE))
    return stp_get_int_parameter(v, "escp2_ink_type");
  return stpi_escp2_model_capabilities[model].dot_sizes[dotid];
}

static double
escp2_density(int model, int resid, const stp_vars_t v)
{
  int dotid = resid2dotid(resid);
  if (stp_check_float_parameter(v, "escp2_density", STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter(v, "escp2_density");
  return stpi_escp2_model_capabilities[model].densities[dotid];
}

static int
escp2_bits(int model, int resid, const stp_vars_t v)
{
  int dotid = resid2dotid(resid);
  if (stp_check_int_parameter(v, "escp2_bits", STP_PARAMETER_ACTIVE))
    return stp_get_int_parameter(v, "escp2_bits");
  return stpi_escp2_model_capabilities[model].bits[dotid];
}

static double
escp2_base_res(int model, int resid, const stp_vars_t v)
{
  int dotid = resid2dotid(resid);
  if (stp_check_float_parameter(v, "escp2_base_res", STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter(v, "escp2_base_res");
  return stpi_escp2_model_capabilities[model].base_resolutions[dotid];
}

static const escp2_variable_inkset_t *
escp2_inks(int model, int resid, int inkset, const stp_vars_t v)
{
  const escp2_variable_inklist_t *inks =
    stpi_escp2_model_capabilities[model].inks;
  resid /= 2;
  return (*inks)[inkset][resid];
}

static const paper_t *
get_media_type(int model, const char *name, const stp_vars_t v)
{
  int i;
  const paperlist_t *p = escp2_paperlist(model, v);
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
escp2_has_advanced_command_set(int model, const stp_vars_t v)
{
  return (escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO,v) ||
	  escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_1999,v) ||
	  escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_2000,v));
}

static int
escp2_use_extended_commands(int model, const stp_vars_t v, int use_softweave)
{
  return (escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO, v) ||
	  (escp2_has_cap(model, MODEL_VARIABLE_DOT, MODEL_VARIABLE_YES, v) &&
	   use_softweave));
}

static int
verify_resolution(const res_t *res, int model, const stp_vars_t v)
{
  int nozzle_width =
    (escp2_base_separation(model, v) / escp2_nozzle_separation(model, v));
  int nozzles = escp2_nozzles(model, v);
  if (escp2_ink_type(model, res->resid, v) != -1 &&
      res->vres <= escp2_max_vres(model, v) &&
      res->hres <= escp2_max_hres(model, v) &&
      res->vres >= escp2_min_vres(model, v) &&
      res->hres >= escp2_min_hres(model, v) &&
      (nozzles == 1 ||
       ((res->vres / nozzle_width) * nozzle_width) == res->vres))
    {
      int xdpi = res->hres;
      int physical_xdpi = escp2_base_res(model, res->resid, v);
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
verify_papersize(const stp_papersize_t pt, int model, const stp_vars_t v)
{
  unsigned int height_limit, width_limit;
  unsigned int min_height_limit, min_width_limit;
  unsigned int pwidth = stp_papersize_get_width(pt);
  unsigned int pheight = stp_papersize_get_height(pt);
  width_limit = escp2_max_paper_width(model, v);
  height_limit = escp2_max_paper_height(model, v);
  min_width_limit = escp2_min_paper_width(model, v);
  min_height_limit = escp2_min_paper_height(model, v);
  if (strlen(stp_papersize_get_name(pt)) > 0 &&
      pwidth <= width_limit && pheight <= height_limit &&
      (pheight >= min_height_limit || pheight == 0) &&
      (pwidth >= min_width_limit || pwidth == 0) &&
      (pwidth == 0 || pheight > 0 ||
       escp2_has_cap(model, MODEL_ROLLFEED, MODEL_ROLLFEED_YES, v)))
    return 1;
  else
    return 0;
}

static int
verify_inktype(const escp2_inkname_t *inks, int model, const stp_vars_t v)
{
  if (inks->inkset == INKSET_EXTENDED)
    return 0;
  else
    return 1;
}

/*
 * 'escp2_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
escp2_list_parameters(const stp_vars_t v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  return ret;
}

static void
escp2_parameters(const stp_vars_t v, const char *name,
		 stp_parameter_t *description)
{
  int		i;
  int model = stpi_get_model_id(v);
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (model < 0 || model >= stpi_escp2_model_limit)
    {
      stpi_eprintf(v, _("Model %d out of range.\n"), model);
      return;
    }

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
	  const stp_papersize_t pt = stp_get_papersize_by_index(i);
	  if (verify_papersize(pt, model, v))
	    stp_string_list_add_string(description->bounds.str,
				      stp_papersize_get_name(pt),
				      stp_papersize_get_text(pt));
	}
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      const res_t *res = escp2_reslist(model, v);
      description->bounds.str = stp_string_list_create();
      while (res->hres)
	{
	  if (verify_resolution(res, model, v))
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
      const inklist_t *inks = escp2_inklist(model, v);
      int ninktypes = inks->n_inks;
      description->bounds.str = stp_string_list_create();
      if (ninktypes)
	{
	  for (i = 0; i < ninktypes; i++)
	    if (verify_inktype(inks->inknames[i], model, v))
	      stp_string_list_add_string(description->bounds.str,
					inks->inknames[i]->name,
					_(inks->inknames[i]->text));
	  description->deflt.str =
	    stp_string_list_param(description->bounds.str, 0)->name;
	}
      else
	description->is_active = 0;
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      const paperlist_t *p = escp2_paperlist(model, v);
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
      const input_slot_list_t *slots = escp2_input_slots(model, v);
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
}

static const res_t *
escp2_find_resolution(int model, const stp_vars_t v, const char *resolution)
{
  const res_t *res;
  if (!resolution || !strcmp(resolution, ""))
    return NULL;
  for (res = escp2_reslist(model, v);;res++)
    {
      if (!strcmp(resolution, res->name))
	return res;
      else if (!strcmp(res->name, ""))
	return NULL;
    }
}

/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

static void
escp2_imageable_area(const stp_vars_t v,   /* I */
		     int  *left,	/* O - Left position in points */
		     int  *right,	/* O - Right position in points */
		     int  *bottom,	/* O - Bottom position in points */
		     int  *top)		/* O - Top position in points */
{
  int	width, height;			/* Size of page */
  int	rollfeed = 0;			/* Roll feed selected */
  const char *input_slot = stp_get_string_parameter(v, "InputSlot");
  int model = stpi_get_model_id(v);
  if (model < 0 || model >= stpi_escp2_model_limit)
    {
      stpi_eprintf(v, _("Model %d out of range.\n"), model);
      return;
    }

  if (input_slot && strlen(input_slot) > 0)
    {
      int i;
      const input_slot_list_t *slots = escp2_input_slots(model, v);
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

  if (rollfeed)
    {
      *left =	escp2_roll_left_margin(model, v);
      *right =	width - escp2_roll_right_margin(model, v);
      *top =	escp2_roll_top_margin(model, v);
      *bottom =	height - escp2_roll_bottom_margin(model, v);
    }
  else
    {
      *left =	escp2_left_margin(model, v);
      *right =	width - escp2_right_margin(model, v);
      *top =	escp2_top_margin(model, v);
      *bottom =	height - escp2_bottom_margin(model, v);
    }
}

static void
escp2_limit(const stp_vars_t v,			/* I */
	    int *width, int *height,
	    int *min_width, int *min_height)
{
  int model = stpi_get_model_id(v);
  if (model < 0 || model >= stpi_escp2_model_limit)
    {
      stpi_eprintf(v, _("Model %d out of range.\n"), model);
      return;
    }
  *width =	escp2_max_paper_width(model, v);
  *height =	escp2_max_paper_height(model, v);
  *min_width =	escp2_min_paper_width(model, v);
  *min_height =	escp2_min_paper_height(model, v);
}

static void
escp2_describe_resolution(const stp_vars_t v, int *x, int *y)
{
  int model = stpi_get_model_id(v);
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const res_t *res;
  if (model < 0 || model >= stpi_escp2_model_limit)
    {
      stpi_eprintf(v, _("Model %d out of range.\n"), model);
      return;
    }
  res = escp2_reslist(model, v);

  while (res->hres)
    {
      if (resolution && strcmp(resolution, res->name) == 0 &&
	  verify_resolution(res, model, v))
	{
	  *x = res->external_hres;
	  *y = res->external_vres;
	  return;
	}
      res++;
    }
  *x = -1;
  *y = -1;
}

static void
escp2_reset_printer(const escp2_init_t *init)
{
  /*
   * Magic initialization string that's needed to take printer out of
   * packet mode.
   */
  const stp_raw_t *inits = escp2_preinit_sequence(init->model, init->v);
  if (inits)
    stpi_zfwrite(inits->data, inits->bytes, 1, init->v);

  stpi_send_command(init->v, "\033@", "");
}

static void
print_remote_param(const stp_vars_t v, const char *param, const char *value)
{
  stpi_send_command(v, "\033(R", "bcscs", '\0', param, ':',
		   value ? value : "NULL");
  stpi_send_command(v, "\033", "ccc", 0, 0, 0);
}

static void
print_remote_int_param(const stp_vars_t v, const char *param, int value)
{
  char buf[64];
  (void) snprintf(buf, 64, "%d", value);
  print_remote_param(v, param, buf);
}

static void
print_remote_float_param(const stp_vars_t v, const char *param, double value)
{
  char buf[64];
  (void) snprintf(buf, 64, "%f", value);
  print_remote_param(v, param, buf);
}

static void
print_debug_params(const escp2_init_t *init)
{
  print_remote_param(init->v, "Package", PACKAGE);
  print_remote_param(init->v, "Version", VERSION);
  print_remote_param(init->v, "Release Date", RELEASE_DATE);
  print_remote_param(init->v, "Driver", stp_get_driver(init->v));
  print_remote_param(init->v, "Resolution", stp_get_string_parameter(init->v, "Resolution"));
  print_remote_param(init->v, "Media Size", stp_get_string_parameter(init->v, "PageSize"));
  print_remote_param(init->v, "Media Type", stp_get_string_parameter(init->v, "MediaType"));
  print_remote_param(init->v, "Media Source", stp_get_string_parameter(init->v, "InputSlot"));
  print_remote_param(init->v, "Ink Type", stp_get_string_parameter(init->v, "InkType"));
  print_remote_param(init->v, "Dither", stp_get_string_parameter(init->v, "DitherAlgorithm"));
  print_remote_int_param(init->v, "Output Type", stp_get_output_type(init->v));
  print_remote_int_param(init->v, "Left", stp_get_left(init->v));
  print_remote_int_param(init->v, "Top", stp_get_top(init->v));
  print_remote_param(init->v, "Image Type", stp_get_string_parameter(init->v, "ImageOptimization"));
  print_remote_int_param(init->v, "Page Width", stp_get_page_width(init->v));
  print_remote_int_param(init->v, "Page Height", stp_get_page_height(init->v));
  print_remote_int_param(init->v, "Input Model", stp_get_input_color_model(init->v));
  print_remote_int_param(init->v, "Output Model", stpi_get_output_color_model(init->v));
  print_remote_float_param(init->v, "Brightness", stp_get_float_parameter(init->v, "Brightness"));
  print_remote_float_param(init->v, "Gamma", stp_get_float_parameter(init->v, "Gamma"));
  print_remote_float_param(init->v, "App Gamma", stp_get_float_parameter(init->v, "AppGamma"));
  print_remote_float_param(init->v, "Contrast", stp_get_float_parameter(init->v, "Contrast"));
  print_remote_float_param(init->v, "Cyan", stp_get_float_parameter(init->v, "Cyan"));
  print_remote_float_param(init->v, "Magenta", stp_get_float_parameter(init->v, "Magenta"));
  print_remote_float_param(init->v, "Yellow", stp_get_float_parameter(init->v, "Yellow"));
  print_remote_float_param(init->v, "Saturation", stp_get_float_parameter(init->v, "Saturation"));
  print_remote_float_param(init->v, "Density", stp_get_float_parameter(init->v, "Density"));
  print_remote_int_param(init->v, "Model", init->model);
  print_remote_int_param(init->v, "Output_type", init->output_type);
  print_remote_int_param(init->v, "Ydpi", init->ydpi);
  print_remote_int_param(init->v, "Xdpi", init->xdpi);
  print_remote_int_param(init->v, "Physical_xdpi", init->physical_xdpi);
  print_remote_int_param(init->v, "Use_softweave", init->res->softweave);
  print_remote_int_param(init->v, "Use_microweave", init->res->microweave);
  print_remote_int_param(init->v, "Page_true_height", init->page_true_height);
  print_remote_int_param(init->v, "Page_width", init->page_width);
  print_remote_int_param(init->v, "Page_top", init->page_top);
  print_remote_int_param(init->v, "Page_bottom", init->page_bottom);
  print_remote_int_param(init->v, "Nozzles", init->nozzles);
  print_remote_int_param(init->v, "Nozzle_separation", init->nozzle_separation);
  print_remote_int_param(init->v, "Horizontal_passes", init->horizontal_passes);
  print_remote_int_param(init->v, "Vertical_passes", init->res->vertical_passes);
  print_remote_int_param(init->v, "Vertical_oversample", init->res->vertical_oversample);
  print_remote_int_param(init->v, "Bits", init->bits);
  print_remote_int_param(init->v, "Unidirectional", init->res->unidirectional);
  print_remote_int_param(init->v, "Resid", init->res->resid);
  print_remote_int_param(init->v, "Drop Size", init->drop_size);
  print_remote_int_param(init->v, "Initial_vertical_offset", init->initial_vertical_offset);
  print_remote_int_param(init->v, "Total_channels", init->total_channels);
  print_remote_int_param(init->v, "Use_black_parameters", init->use_black_parameters);
  print_remote_int_param(init->v, "Channel_limit", init->channel_limit);
  print_remote_int_param(init->v, "Use_fast_360", init->use_fast_360);
  print_remote_param(init->v, "Ink name", init->inkname->name);
  print_remote_int_param(init->v, "  is_color", init->inkname->is_color);
  print_remote_int_param(init->v, "  channels", init->inkname->channel_limit);
  print_remote_int_param(init->v, "  inkset", init->inkname->inkset);
  stpi_send_command(init->v, "\033", "c", 0);
}

static void
escp2_set_remote_sequence(const escp2_init_t *init)
{
  /* Magic remote mode commands, whatever they do */

  if (stpi_debug_level & STPI_DBG_MARK_FILE)
    print_debug_params(init);
  if (escp2_has_advanced_command_set(init->model, init->v) || init->input_slot)
    {
      int feed_sequence = 0;
      const paper_t *p =
	get_media_type(init->model,
		       stp_get_string_parameter(init->v, "MediaType"),
		       init->v);
      /* Enter remote mode */
      stpi_send_command(init->v, "\033(R", "bcs", 0, "REMOTE1");
      if (escp2_has_cap(init->model, MODEL_COMMAND,
			MODEL_COMMAND_PRO, init->v))
	{
	  if (p)
	    {
	      stpi_send_command(init->v, "PH", "bcc", 0, p->paper_thickness);
	      if (escp2_has_cap(init->model, MODEL_VACUUM, MODEL_VACUUM_YES,
				init->v))
		stpi_send_command(init->v, "SN", "bccc", 0, 5,
				   p->vacuum_intensity);
	      stpi_send_command(init->v, "SN", "bccc", 0, 4,
				 p->feed_adjustment);
	    }
	}
      else if (escp2_has_advanced_command_set(init->model, init->v))
	{
	  if (p)
	    feed_sequence = p->paper_feed_sequence;
	  /* Function unknown */
	  stpi_send_command(init->v, "PM", "bh", 0);
	  /* Set mechanism sequence */
	  stpi_send_command(init->v, "SN", "bccc", 0, 0, feed_sequence);
	  if (escp2_has_cap(init->model, MODEL_XZEROMARGIN,
			    MODEL_XZEROMARGIN_YES, init->v))
	    stpi_send_command(init->v, "FP", "bch", 0, 0xffb0);
	}
      if (init->input_slot)
	{
	  int divisor = escp2_base_separation(init->model, init->v) / 360;
	  int height = init->page_true_height * 5 / divisor;
	  if (init->input_slot->init_sequence.bytes)
	    stpi_zfwrite(init->input_slot->init_sequence.data,
			init->input_slot->init_sequence.bytes, 1, init->v);
	  switch (init->input_slot->roll_feed_cut_flags)
	    {
	    case ROLL_FEED_CUT_ALL:
	      stpi_send_command(init->v, "JS", "bh", 0);
	      stpi_send_command(init->v, "CO", "bccccl", 0, 0, 1, 0, 0);
	      stpi_send_command(init->v, "CO", "bccccl", 0, 0, 0, 0, height);
	      break;
	    case ROLL_FEED_CUT_LAST:
	      stpi_send_command(init->v, "CO", "bccccl", 0, 0, 1, 0, 0);
	      stpi_send_command(init->v, "CO", "bccccl", 0, 0, 2, 0, height);
	      break;
	    default:
	      break;
	    }
	}

      /* Exit remote mode */

      stpi_send_command(init->v, "\033", "ccc", 0, 0, 0);
    }
}

static void
escp2_set_graphics_mode(const escp2_init_t *init)
{
  stpi_send_command(init->v, "\033(G", "bc", 1);
}

static void
escp2_set_resolution(const escp2_init_t *init)
{
  if (escp2_use_extended_commands(init->model, init->v, init->res->softweave))
    {
      int hres = escp2_max_hres(init->model, init->v);
      stpi_send_command(init->v, "\033(U", "bccch", hres / init->ydpi,
			 hres / init->ydpi, hres / init->xdpi, hres);
    }
  else
    stpi_send_command(init->v, "\033(U", "bc", 3600 / init->ydpi);
}

static void
escp2_set_color(const escp2_init_t *init)
{
  if (init->use_fast_360)
    stpi_send_command(init->v, "\033(K", "bc", 3);
  else if (escp2_has_cap(init->model, MODEL_GRAYMODE, MODEL_GRAYMODE_YES,
			 init->v))
    stpi_send_command(init->v, "\033(K", "bc",
		     (init->use_black_parameters ? 1 : 2));
}

static void
escp2_set_microweave(const escp2_init_t *init)
{
  stpi_send_command(init->v, "\033(i", "bc", init->res->microweave);
}

static void
escp2_set_printhead_speed(const escp2_init_t *init)
{
  if (init->res->unidirectional)
    {
      stpi_send_command(init->v, "\033U", "c", 1);
      if (init->xdpi > escp2_enhanced_resolution(init->model, init->v))
	stpi_send_command(init->v, "\033(s", "bc", 2);
    }
  else
    stpi_send_command(init->v, "\033U", "c", 0);
}

static void
escp2_set_dot_size(const escp2_init_t *init)
{
  /* Dot size */
  if (init->drop_size >= 0)
    stpi_send_command(init->v, "\033(e", "bcc", 0, init->drop_size);
}

static void
escp2_set_page_height(const escp2_init_t *init)
{
  int l = init->ydpi * init->page_true_height / 72;
  if (escp2_use_extended_commands(init->model, init->v, init->res->softweave))
    stpi_send_command(init->v, "\033(C", "bl", l);
  else
    stpi_send_command(init->v, "\033(C", "bh", l);
}

static void
escp2_set_margins(const escp2_init_t *init)
{
  int bot = init->ydpi * init->page_bottom / 72;
  int top = init->ydpi * init->page_top / 72;

  top += init->initial_vertical_offset;
  if (escp2_use_extended_commands(init->model, init->v,init->res->softweave) &&
      (escp2_has_cap(init->model,MODEL_COMMAND,MODEL_COMMAND_2000,init->v)||
       escp2_has_cap(init->model,MODEL_COMMAND,MODEL_COMMAND_PRO,init->v)))
    stpi_send_command(init->v, "\033(c", "bll", top, bot);
  else
    stpi_send_command(init->v, "\033(c", "bhh", top, bot);
}

static void
escp2_set_form_factor(const escp2_init_t *init)
{
  if (escp2_has_advanced_command_set(init->model, init->v))
    {
      int w = init->page_width * init->ydpi / 72;
      int h = init->page_true_height * init->ydpi / 72;

      if (escp2_has_cap(init->model, MODEL_XZEROMARGIN, MODEL_XZEROMARGIN_YES,
			init->v))
	/* Make the page 2/10" wider (probably ignored by the printer) */
	w += 144 * init->xdpi / 720;

      stpi_send_command(init->v, "\033(S", "bll", w, h);
    }
}

static void
escp2_set_printhead_resolution(const escp2_init_t *init)
{
  if (escp2_use_extended_commands(init->model, init->v, init->res->softweave))
    {
      int xres;
      int yres;
      int scale = escp2_resolution_scale(init->model, init->v);

      xres = scale / init->physical_xdpi;

      if (escp2_has_cap(init->model, MODEL_COMMAND, MODEL_COMMAND_PRO,
			init->v) && !init->res->softweave)
	yres = scale / init->ydpi;
      else
	yres = (init->nozzle_separation * scale /
		escp2_base_separation(init->model, init->v));

      /* Magic resolution cookie */
      stpi_send_command(init->v, "\033(D", "bhcc", scale, yres, xres);
    }
}

static void
escp2_init_printer(const escp2_init_t *init)
{
  escp2_reset_printer(init);
  escp2_set_remote_sequence(init);
  escp2_set_graphics_mode(init);
  escp2_set_resolution(init);
  escp2_set_color(init);
  escp2_set_microweave(init);
  escp2_set_printhead_speed(init);
  escp2_set_dot_size(init);
  escp2_set_printhead_resolution(init);
  escp2_set_page_height(init);
  escp2_set_margins(init);
  escp2_set_form_factor(init);
}

static void
escp2_deinit_printer(const escp2_init_t *init)
{
  stpi_puts("\033@", init->v);	/* ESC/P2 reset */
  if (escp2_has_advanced_command_set(init->model, init->v) || init->input_slot)
    {
      const stp_raw_t *deinit =
	escp2_postinit_remote_sequence(init->model, init->v);
      stpi_send_command(init->v, "\033(R", "bcs", 0, "REMOTE1");
      if (init->input_slot && init->input_slot->deinit_sequence.bytes)
	stpi_zfwrite(init->input_slot->deinit_sequence.data,
		    init->input_slot->deinit_sequence.bytes, 1, init->v);
      /* Load settings from NVRAM */
      stpi_send_command(init->v, "LD", "b");

      /* Magic deinit sequence reported by Simone Falsini */
      if (deinit)
	stpi_zfwrite(deinit->data, deinit->bytes, 1, init->v);
      /* Exit remote mode */
      stpi_send_command(init->v, "\033", "ccc", 0, 0, 0);
    }
}

static void
adjust_density_and_ink_type(escp2_init_t *init, stp_image_t *image)
{
  const paper_t *pt;
  double paper_density = .8;
  stp_vars_t v = init->v;
  pt = get_media_type(init->model,stp_get_string_parameter(v, "MediaType"), v);
  if (pt)
    paper_density = pt->base_density;

  if (init->rescale_density)
    stp_scale_float_parameter
      (v, "Density",
       paper_density * escp2_density(init->model, init->res->resid, v));
  init->drop_size = escp2_ink_type(init->model, init->res->resid, v);
  init->ink_resid = init->res->resid;

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
	  int resid = init->res->resid;
	  while (density > 1.0 && resid >= RES_360_M)
	    {
	      int tresid = resid - 2;
	      int bits_now = escp2_bits(init->model, resid, v);
	      double density_now = escp2_density(init->model, resid, v);
	      int bits_then = escp2_bits(init->model, tresid, v);
	      double density_then = escp2_density(init->model, tresid, v);
	      int drop_size_then = escp2_ink_type(init->model, tresid, v);

	      /*
	       * If the softweave/microweave isn't available at the lower
	       * resolution, try the drop size for the other flavor.
	       */
	      if (drop_size_then == -1)
		{
		  tresid ^= 1;
		  bits_then = escp2_bits(init->model, tresid, v);
		  density_then = escp2_density(init->model, tresid ,v);
		  drop_size_then = escp2_ink_type(init->model, tresid, v);
		}

	      /*
	       * If we would change the number of bits in the ink type,
	       * don't try this.  Some resolutions require using a certain
	       * number of bits!
	       */

	      if (bits_now != bits_then || density_then <= 0.0 ||
		  drop_size_then == -1)
		break;
	      density = density * density_now / density_then / 2;
	      resid = tresid;
	    }
	  init->drop_size = escp2_ink_type(init->model, resid, init->v);
	  init->ink_resid = resid;
	  if (density > 1.0)
	    density = 1.0;
	  stp_set_float_parameter(v, "Density", density);
	}
    }
}

static int
adjust_print_quality(const escp2_init_t *init, stp_image_t *image)
{
  int cols;
  stp_curve_t   lum_adjustment = NULL;
  stp_curve_t   sat_adjustment = NULL;
  stp_curve_t   hue_adjustment = NULL;
  const paper_t *pt;
  const stp_vars_t nv = init->v;
  double k_upper, k_lower;
  double paper_k_upper;
  /*
   * Compute the LUT.  For now, it's 8 bit, but that may eventually
   * sometimes change.
   */
  k_lower = init->inkname->k_lower;
  k_upper = init->inkname->k_upper;

  pt = get_media_type(init->model,
		      stp_get_string_parameter(nv, "MediaType"), nv);
  if (pt)
    {
      k_lower *= pt->k_lower_scale;
      paper_k_upper = pt->k_upper;
      k_upper *= pt->k_upper;
      if (init->total_channels >= 5)
	{
	  stp_scale_float_parameter(nv, "Cyan", pt->p_cyan);
	  stp_scale_float_parameter(nv, "Magenta", pt->p_magenta);
	  stp_scale_float_parameter(nv, "Yellow", pt->p_yellow);
	}
      else
	{
	  stp_scale_float_parameter(nv, "Cyan", pt->cyan);
	  stp_scale_float_parameter(nv, "Magenta", pt->magenta);
	  stp_scale_float_parameter(nv, "Yellow", pt->yellow);
	}
      stp_scale_float_parameter(nv, "Saturation", pt->saturation);
      stp_scale_float_parameter(nv, "Gamma", pt->gamma);
    }
  else				/* Assume some kind of plain paper */
    {
      k_lower *= .1;
      paper_k_upper = .5;
      k_upper *= .5;
    }

  if (!stp_check_float_parameter(nv, "GCRLower", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(nv, "GCRLower", k_lower);
  if (!stp_check_float_parameter(nv, "GCRUpper", STP_PARAMETER_ACTIVE))
    stp_set_default_float_parameter(nv, "GCRUpper", k_upper);

  if (!stp_check_curve_parameter(nv, "HueMap", STP_PARAMETER_ACTIVE))
    {
      hue_adjustment = stpi_read_and_compose_curves
	(init->inkname->hue_adjustment, pt ? pt->hue_adjustment : NULL,
	 STP_CURVE_COMPOSE_ADD);
      stp_set_curve_parameter(nv, "HueMap", hue_adjustment);
      stp_curve_free(hue_adjustment);
    }
  if (!stp_check_curve_parameter(nv, "SatMap", STP_PARAMETER_ACTIVE))
    {
      sat_adjustment = stpi_read_and_compose_curves
	(init->inkname->sat_adjustment, pt ? pt->sat_adjustment : NULL,
	 STP_CURVE_COMPOSE_MULTIPLY);
      stp_set_curve_parameter(nv, "SatMap", sat_adjustment);
      stp_curve_free(sat_adjustment);
    }
  if (!stp_check_curve_parameter(nv, "LumMap", STP_PARAMETER_ACTIVE))
    {
      lum_adjustment = stpi_read_and_compose_curves
	(init->inkname->lum_adjustment, pt ? pt->lum_adjustment : NULL,
	 STP_CURVE_COMPOSE_MULTIPLY);
      stp_set_curve_parameter(nv, "LumMap", lum_adjustment);
      stp_curve_free(lum_adjustment);
    }
  cols = stpi_color_init(nv, image, 65536);
  return cols;
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

static const escp2_inkname_t *
get_inktype(const stp_vars_t v, int model)
{
  const char	*ink_type = stp_get_string_parameter(v, "InkType");
  const inklist_t *ink_list = escp2_inklist(model, v);
  int i;

  if (ink_type)
    {
      for (i = 0; i < ink_list->n_inks; i++)
	{
	  if (strcmp(ink_type, ink_list->inknames[i]->name) == 0)
	    return ink_list->inknames[i];
	}
    }
  return NULL;
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
  }
};

static int
compute_channel_count(const escp2_inkname_t *ink_type,
		      int channel_limit)
{
  int i;
  int channels_in_use = 0;
  for (i = 0; i < channel_limit; i++)
    {
      const ink_channel_t *channel = ink_type->channels[i];
      if (channel)
	channels_in_use += channel->n_subchannels;
    }
  return channels_in_use;
}

static void
setup_inks(const escp2_init_t *init)
{
  int i;
  const escp2_variable_inkset_t *inks;
  const stp_vars_t nv = init->v;
  const paper_t *pt;
  double paper_k_upper = 0.5;

  pt = get_media_type(init->model,
		      stp_get_string_parameter(nv, "MediaType"), nv);
  if (pt)
    paper_k_upper = pt->k_upper;
  inks = escp2_inks(init->model, init->ink_resid, init->inkname->inkset, nv);
  if (inks)
    {
      stpi_init_debug_messages(nv);
      for (i = 0; i < init->channel_limit; i++)
	{
	  const escp2_variable_ink_t *ink = (*inks)[i];
	  if (ink)
	    {
	      stpi_dither_set_ranges(nv, i, ink->numranges, ink->range,
				    ink->density * paper_k_upper);

	      stpi_dither_set_shades(nv, i, ink->numshades, ink->shades,
				    ink->density * paper_k_upper);
	    }
	}
      stpi_flush_debug_messages(nv);
    }
}

static int
setup_ink_types(const escp2_inkname_t *ink_type,
		escp2_privdata_t *privdata,
		unsigned char **cols,
		int *head_offset,
		stp_vars_t v,
		int channel_limit,
		int line_length)
{
  int i;
  int channels_in_use = 0;
  for (i = 0; i < channel_limit; i++)
    {
      const ink_channel_t *channel = ink_type->channels[i];
      if (channel)
	{
	  int j;
	  for (j = 0; j < channel->n_subchannels; j++)
	    {
	      cols[channels_in_use] = stpi_zalloc(line_length);
	      privdata->channels[channels_in_use] = &(channel->channels[j]);
	      stpi_dither_add_channel(v, cols[channels_in_use], i, j);
	      head_offset[channels_in_use] = channel->channels[j].head_offset;
	      channels_in_use++;
	    }
	}
    }
  return channels_in_use;
}

static int
escp2_print_data(stp_vars_t v, stp_image_t *image, void *weave,
		 int height, int width, unsigned short *out,
		 unsigned char **cols)
{
  int errdiv  = stpi_image_height(image) / height;
  int errmod  = stpi_image_height(image) % height;
  int errval  = 0;
  int errlast = -1;
  int errline  = 0;
  int y;

  stpi_image_progress_init(image);

  QUANT(0);
  for (y = 0; y < height; y ++)
    {
      int duplicate_line = 1;
      int zero_mask;
      if ((y & 63) == 0)
	stpi_image_note_progress(image, y, height);

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

      stpi_write_weave(weave, width, cols);
      QUANT(3);
      errval += errmod;
      errline += errdiv;
      if (errval >= height)
	{
	  errval -= height;
	  errline ++;
	}
      QUANT(4);
    }
  stpi_image_progress_conclude(image);
  stpi_flush_all(weave);
  QUANT(5);
  return 1;
}  

/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
static int
escp2_do_print(stp_vars_t v, stp_image_t *image, int print_op)
{
  int		model = stpi_get_model_id(v);
  int		output_type = stp_get_output_type(v);
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		status = 1;

  int		i;

  const res_t	*res;
  int		resid;
  int		xdpi;
  int		ydpi;	/* Resolution */
  int		physical_ydpi;
  int		physical_xdpi;
  int		undersample;

  int		n;		/* Output number */
  unsigned short *out;	/* Output pixels (16-bit) */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		page_true_height;	/* True height of page */

  int		nozzles;
  int		nozzle_separation;
  int		horizontal_passes;

  void *	weave;
  escp2_init_t	init;
  int		max_vres;
  int 		*head_offset;
  int 		max_head_offset;
  escp2_privdata_t privdata;
  const escp2_inkname_t *ink_type;
  int 		total_channels;
  int 		channels_in_use;
  int 		channel_limit;
  const char *input_slot = stp_get_string_parameter(v, "InputSlot");

  if (!stp_verify(v))
    {
      stpi_eprintf(v, _("Print options not verified; cannot print.\n"));
      return 0;
    }
  if (model < 0 || model >= stpi_escp2_model_limit)
    {
      stpi_eprintf(v, _("Model %d out of range.\n"), model);
      return 0;
    }

  stpi_image_init(image);

  if (output_type == OUTPUT_RAW_PRINTER)
    {
      const inklist_t *inks = escp2_inklist(model, v);
      int ninktypes = inks->n_inks;
      int i;
      int found = 0;
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
	    found = 1;
	    break;
	  }
      if (!found)
	{
	  stpi_eprintf(v, _("This printer does not support raw printer output at depth %d\n"),
		      stpi_image_bpp(image) / 2);
	  return 0;
	}
    }

  if (output_type == OUTPUT_RAW_CMYK || output_type == OUTPUT_RAW_PRINTER)
    init.rescale_density = 0;
  else
    init.rescale_density = 1;

  privdata.undersample = 1;
  privdata.denominator = 1;
  privdata.initial_vertical_offset = 0;
  privdata.printed_something = 0;
  privdata.last_color = -1;
  stpi_set_driver_data(v, &privdata);

  ink_type = get_inktype(v, model);
  total_channels = count_channels(ink_type);
  if (output_type != OUTPUT_GRAY &&
      output_type != OUTPUT_RAW_PRINTER && !ink_type->is_color)
    {
      output_type = OUTPUT_GRAY;
      stp_set_output_type(v, OUTPUT_GRAY);
    }
  if (output_type == OUTPUT_COLOR && ink_type->channels[0] != NULL)
    {
      output_type = OUTPUT_RAW_CMYK;
      stp_set_output_type(v, OUTPUT_RAW_CMYK);
    }

 /*
  * Figure out the output resolution...
  */
  res = escp2_find_resolution(model, v,
			      stp_get_string_parameter(v, "Resolution"));
  if (res->softweave)
    max_vres = escp2_max_vres(model, v);
  else
    max_vres = escp2_base_resolution(model, v);
  xdpi = res->hres;
  ydpi = res->vres;
  resid = res->resid;
  undersample = res->vertical_undersample;
  privdata.undersample = res->vertical_undersample;
  privdata.denominator = res->vertical_denominator;

  physical_xdpi = escp2_base_res(model, res->resid, v);
  if (physical_xdpi > xdpi)
    physical_xdpi = xdpi;

  physical_ydpi = ydpi;
  if (ydpi > max_vres)
    physical_ydpi = max_vres;

  escp2_imageable_area(v, &page_left, &page_right, &page_bottom, &page_top);
  left -= page_left;
  top -= page_top;
  page_width = page_right - page_left;
  page_height = page_bottom - page_top;

  stpi_default_media_size(v, &n, &page_true_height);

  left = physical_ydpi * undersample * left / 72 / res->vertical_denominator;

 /*
  * Adjust for zero-margin printing...
  */

  if (escp2_has_cap(model, MODEL_XZEROMARGIN, MODEL_XZEROMARGIN_YES, v))
    {
     /*
      * In zero-margin mode, the origin is about 3/20" to the left of the
      * paper's left edge.
      */
      left += escp2_zero_margin_offset(model, v) * physical_ydpi *
	undersample / max_vres / res->vertical_denominator;
    }

  /*
   * Set up the output channels
   */
  head_offset = stpi_zalloc(sizeof(int) * total_channels);

  memset(head_offset, 0, sizeof(head_offset));
  if (output_type == OUTPUT_RAW_PRINTER)
    channel_limit = escp2_physical_channels(model, v);
  else if (output_type == OUTPUT_GRAY)
    channel_limit = 1;
  else
    channel_limit = NCOLORS;

  channels_in_use = compute_channel_count(ink_type, channel_limit);
  if (channels_in_use == 0)
    {
      ink_type = &default_black_ink;
      channels_in_use = compute_channel_count(ink_type, channel_limit);
    }
  if (channels_in_use == 1)
    head_offset[0] = 0;
  if (escp2_has_cap(model, MODEL_FAST_360, MODEL_FAST_360_YES, v) &&
      (ink_type->inkset == INKSET_CMYK || channels_in_use == 1) &&
      xdpi == 360 && ydpi == 360)
    init.use_fast_360 = 1;
  else
    init.use_fast_360 = 0;

  /*
   * Set up the printer-specific parameters (weaving)
   */
  if (res->softweave)
    {
      horizontal_passes = xdpi / physical_xdpi;
      if (channels_in_use == 1 &&
	  (ydpi >= (escp2_base_separation(model, v) /
		    escp2_black_nozzle_separation(model, v))) &&
	  (escp2_max_black_resolution(model, v) < 0 ||
	   ydpi <= escp2_max_black_resolution(model, v)) &&
	  escp2_black_nozzles(model, v))
	init.use_black_parameters = 1;
      else
	init.use_black_parameters = 0;
      if (init.use_fast_360)
	{
	  nozzles = escp2_fast_nozzles(model, v);
	  nozzle_separation = escp2_fast_nozzle_separation(model, v);
	  privdata.min_nozzles = escp2_min_fast_nozzles(model, v);
	}
      else if (init.use_black_parameters)
	{
	  nozzles = escp2_black_nozzles(model, v);
	  nozzle_separation = escp2_black_nozzle_separation(model, v);
	  privdata.min_nozzles = escp2_min_black_nozzles(model, v);
	}
      else
	{
	  nozzles = escp2_nozzles(model, v);
	  nozzle_separation = escp2_nozzle_separation(model, v);
	  privdata.min_nozzles = escp2_min_nozzles(model, v);
	}
      init.nozzle_separation = nozzle_separation;
      nozzle_separation =
	nozzle_separation * ydpi / escp2_base_separation(model, v);
    }
  else
    {
      horizontal_passes = xdpi / escp2_base_resolution(model, v);
      nozzles = 1;
      privdata.min_nozzles = 1;
      nozzle_separation = 1;
      init.nozzle_separation = nozzle_separation;
      init.use_black_parameters = 0;
    }
  init.nozzles = nozzles;

  if (horizontal_passes == 0)
    horizontal_passes = 1;

  max_head_offset = 0;
  if (channels_in_use > 1)
    for (i = 0; i < total_channels; i++)
      {
	head_offset[i] = head_offset[i] * ydpi/escp2_base_separation(model,v);
	if (head_offset[i] > max_head_offset)
	  max_head_offset = head_offset[i];
      }

 /*
  * Send ESC/P2 initialization commands...
  */
  init.model = model;
  init.v = v;
  init.output_type = output_type;
  init.ydpi = ydpi * undersample;
  if (init.ydpi > escp2_max_vres(init.model, init.v))
    init.ydpi = escp2_max_vres(init.model, init.v);
  init.xdpi = xdpi;
  init.physical_xdpi = physical_xdpi;
  init.res = res;
  init.page_true_height = page_true_height;
  init.page_width = page_width;
  init.page_top = page_top;
  if (init.output_type == OUTPUT_GRAY && channels_in_use == 1)
    {
      if (init.use_black_parameters)
	init.initial_vertical_offset =
	  escp2_black_initial_vertical_offset(init.model, init.v) * init.ydpi /
	  escp2_base_separation(model, v);
      else
	init.initial_vertical_offset =
	  head_offset[0] +
	  (escp2_initial_vertical_offset(init.model, init.v) *
	   init.ydpi / escp2_base_separation(model, v));
    }
  else
    init.initial_vertical_offset =
      escp2_initial_vertical_offset(init.model, init.v) * init.ydpi /
      escp2_base_separation(model, v);

   /* adjust bottom margin for a 480 like head configuration */
  init.page_bottom = page_bottom - max_head_offset * 72 / ydpi;
  if ((max_head_offset * 72 % ydpi) != 0)
    init.page_bottom -= 1;
  if (init.page_bottom < 0)
    init.page_bottom = 0;

  init.horizontal_passes = horizontal_passes;
  init.bits = escp2_bits(model, init.res->resid, v);
  init.inkname = ink_type;
  init.total_channels = total_channels;
  init.channel_limit = channel_limit;

  init.input_slot = NULL;
  if (input_slot && strlen(input_slot) > 0)
    {
      const input_slot_list_t *slots = escp2_input_slots(model, v);
      for (i = 0; i < slots->n_input_slots; i++)
	{
	  if (slots->slots[i].name &&
	      strcmp(input_slot, slots->slots[i].name) == 0)
	    {
	      init.input_slot = &(slots->slots[i]);
	      break;
	    }
	}
    }
  if (init.input_slot && init.input_slot->roll_feed_cut_flags)
    {
      init.page_true_height += 4;
      init.page_top += 2;
      init.page_bottom += 2;
      top += 2;
      page_height += 2;
    }

  adjust_density_and_ink_type(&init, image);
  if (print_op & OP_JOB_START)
    escp2_init_printer(&init);
  if (print_op & OP_JOB_PRINT)
    {
      int out_width;		/* Width of image on page */
      int out_height;		/* Height of image on page */
      int out_channels;		/* Output bytes per pixel */
      int length;		/* Length of raster data */
      unsigned char **cols =
	stpi_zalloc(sizeof(unsigned char *) * total_channels);

      privdata.channels =
	stpi_zalloc(sizeof(physical_subchannel_t *) * total_channels);

      /*
       * Compute the output size...
       */
      out_width = stp_get_width(v);
      out_height = stp_get_height(v);

      /*
       * Convert image size to printer resolution...
       */

      out_width  = xdpi * out_width / 72;
      out_height = ydpi * out_height / 72;
      length = (out_width + 7) / 8;
      privdata.hoffset = left;
      privdata.xdpi = xdpi;
      privdata.model = model;
      privdata.physical_xdpi = physical_xdpi;
      privdata.ydpi = ydpi;
      privdata.width = out_width;

      /*
       * Allocate memory for the raster data...
       */

      weave = stpi_initialize_weave(nozzles, nozzle_separation,
				   horizontal_passes, res->vertical_passes,
				   res->vertical_oversample, total_channels,
				   init.bits, out_width, out_height,
				   top * physical_ydpi / 72,
				   (page_height * physical_ydpi / 72 +
				    escp2_extra_feed(model, v) *
				    physical_ydpi /
				    escp2_base_resolution(model, v)),
				   1, head_offset, v, flush_pass,
				   FILLFUNC, PACKFUNC, COMPUTEFUNC);

      stpi_set_output_color_model(v, COLOR_MODEL_CMY);

      out_channels = adjust_print_quality(&init, image);
      stpi_dither_init(v, image, out_width, xdpi, ydpi);
      channels_in_use = setup_ink_types(ink_type, &privdata, cols, head_offset,
					v, channel_limit, length * init.bits);
      setup_inks(&init);

      out = stpi_malloc(stpi_image_width(image) * out_channels * 2);

      status = escp2_print_data(v, image, weave, out_height, length,
				out, cols);

      /*
       * Cleanup...
       */
      stpi_destroy_weave(weave);
      stpi_dither_free(v);
      stpi_free(out);
      if (!privdata.printed_something)
	stpi_send_command(v, "\n", "");
      stpi_send_command(v, "\f", "");	/* Eject page */
      for (i = 0; i < total_channels; i++)
	if (cols[i])
	  stpi_free((unsigned char *) cols[i]);
      stpi_free(cols);
      stpi_free(privdata.channels);
    }
  if (print_op & OP_JOB_END)
    escp2_deinit_printer(&init);

  stpi_free(head_offset);

#ifdef QUANTIFY
  print_timers(v);
#endif
  return status;
}

static int
escp2_print(const stp_vars_t v, stp_image_t *image)
{
  stp_vars_t nv = stp_vars_create_copy(v);
  int op = OP_JOB_PRINT;
  int status;
  if (stp_get_job_mode(v) == STP_JOB_MODE_PAGE)
    op = OP_JOB_START | OP_JOB_PRINT | OP_JOB_END;
  status = escp2_do_print(nv, image, op);
  stp_vars_free(nv);
  return status;
}

static int
escp2_job_start(const stp_vars_t v, stp_image_t *image)
{
  stp_vars_t nv = stp_vars_create_copy(v);
  int status = escp2_do_print(nv, image, OP_JOB_START);
  stp_vars_free(nv);
  return status;
}

static int
escp2_job_end(const stp_vars_t v, stp_image_t *image)
{
  stp_vars_t nv = stp_vars_create_copy(v);
  int status = escp2_do_print(nv, image, OP_JOB_END);
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

static void
set_vertical_position(stpi_softweave_t *sw, stpi_pass_t *pass, int model,
		      const stp_vars_t v)
{
  escp2_privdata_t *pd = (escp2_privdata_t *) stpi_get_driver_data(v);
  int advance = pass->logicalpassstart - sw->last_pass_offset -
    (escp2_separation_rows(model, v) - 1);
  advance *= pd->undersample;
  if (pass->logicalpassstart > sw->last_pass_offset ||
      pd->initial_vertical_offset != 0)
    {
      advance += pd->initial_vertical_offset;
      pd->initial_vertical_offset = 0;
      if (escp2_use_extended_commands(model, v, sw->jets > 1))
	stpi_send_command(v, "\033(v", "bl", advance);
      else
	stpi_send_command(v, "\033(v", "bh", advance);
      sw->last_pass_offset = pass->logicalpassstart;
    }
}

static void
set_color(stpi_softweave_t *sw, stpi_pass_t *pass, int model, const stp_vars_t v,
	  int color)
{
  escp2_privdata_t *pd = (escp2_privdata_t *) stpi_get_driver_data(v);
  if (pd->last_color != color &&
      ! escp2_use_extended_commands(model, v, sw->jets > 1))
    {
      int ncolor = pd->channels[color]->color;
      int density = pd->channels[color]->density;
      if (density >= 0)
	stpi_send_command(v, "\033(r", "bcc", density, ncolor);
      else
	stpi_send_command(v, "\033r", "c", ncolor);
      pd->last_color = color;
    }
}

static void
set_horizontal_position(stpi_softweave_t *sw, stpi_pass_t *pass, int model,
			const stp_vars_t v, int hoffset, int ydpi,
			int xdpi, int vertical_subpass)
{
  int microoffset = vertical_subpass & (sw->horizontal_weave - 1);
  escp2_privdata_t *pd = (escp2_privdata_t *) stpi_get_driver_data(v);
  if (!escp2_has_advanced_command_set(model, v) &&
      (xdpi <= escp2_base_resolution(model, v) ||
       escp2_max_hres(model, v) < 1440))
    {
      int pos = (hoffset + microoffset);
      if (pos > 0)
	stpi_send_command(v, "\033\\", "h", pos);
    }
  else if (escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO,v) ||
	   (escp2_has_advanced_command_set(model, v) &&
	    escp2_has_cap(model, MODEL_VARIABLE_DOT, MODEL_VARIABLE_YES, v)))
    {
      int pos = ((hoffset * xdpi * pd->denominator / ydpi) + microoffset);
      if (pos > 0)
	stpi_send_command(v, "\033($", "bl", pos);
    }
  else
    {
      int pos = ((hoffset * escp2_max_hres(model, v) * pd->denominator / ydpi)+
		 microoffset);
      if (pos > 0)
	stpi_send_command(v, "\033(\\", "bhh", 1440, pos);
    }
}

static void
send_print_command(stpi_softweave_t *sw, stpi_pass_t *pass, int model, int color,
		   int lwidth, const stp_vars_t v, int hoffset, int ydpi,
		   int xdpi, int physical_xdpi, int nlines)
{
  if (!escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO,v) &&
      sw->jets == 1 && sw->bitwidth == 1)
    {
      int ygap = 3600 / ydpi;
      int xgap = 3600 / xdpi;
      if (ydpi == 720 && escp2_extra_720dpi_separation(model, v))
	ygap *= escp2_extra_720dpi_separation(model, v);
      stpi_send_command(v, "\033.", "cccch", COMPRESSION, ygap, xgap, 1,
			 lwidth);
    }
  else if (!escp2_has_cap(model, MODEL_COMMAND, MODEL_COMMAND_PRO,v) &&
	   escp2_has_cap(model, MODEL_VARIABLE_DOT, MODEL_VARIABLE_NO, v))
    {
      int ygap = 3600 / ydpi;
      int xgap = 3600 / physical_xdpi;
      if (escp2_extra_720dpi_separation(model, v))
	ygap *= escp2_extra_720dpi_separation(model, v);
      else if (escp2_pseudo_separation_rows(model, v) > 0)
	ygap *= escp2_pseudo_separation_rows(model, v);
      else
	ygap *= escp2_separation_rows(model, v);
      stpi_send_command(v, "\033.", "cccch", COMPRESSION, ygap, xgap, nlines,
			 lwidth);
    }
  else
    {
      escp2_privdata_t *pd = (escp2_privdata_t *) stpi_get_driver_data(v);
      int ncolor = pd->channels[color]->color;
      int nwidth = sw->bitwidth * ((lwidth + 7) / 8);
      if (pd->channels[color]->density >= 0)
	ncolor |= (pd->channels[color]->density << 4);
      stpi_send_command(v, "\033i", "ccchh", ncolor, COMPRESSION,
			 sw->bitwidth, nwidth, nlines);
    }
}

static void
send_extra_data(stpi_softweave_t *sw, stp_vars_t v, int extralines, int lwidth)
{
  int k, l;
  int bytes_to_fill = sw->bitwidth * ((lwidth + 7) / 8);
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
}

static void
flush_pass(stpi_softweave_t *sw, int passno, int vertical_subpass)
{
  int j;
  const stp_vars_t v = (sw->v);
  escp2_privdata_t *pd = (escp2_privdata_t *) stpi_get_driver_data(v);
  stpi_lineoff_t *lineoffs = stpi_get_lineoffsets_by_pass(sw, passno);
  stpi_lineactive_t *lineactive = stpi_get_lineactive_by_pass(sw, passno);
  const stpi_linebufs_t *bufs = stpi_get_linebases_by_pass(sw, passno);
  stpi_pass_t *pass = stpi_get_pass_by_pass(sw, passno);
  stpi_linecount_t *linecount = stpi_get_linecount_by_pass(sw, passno);
  int width = pd->width;
  int lwidth = (width + (sw->horizontal_weave - 1)) / sw->horizontal_weave;
  int hoffset = pd->hoffset;
  int model = pd->model;
  int xdpi = pd->xdpi;
  int ydpi = pd->ydpi;
  int physical_xdpi = pd->physical_xdpi;

  ydpi *= pd->undersample;

  if (ydpi > escp2_max_vres(model, v))
    ydpi = escp2_max_vres(model, v);
  for (j = 0; j < sw->ncolors; j++)
    {
      if (lineactive[0].v[j] > 0)
	{
	  int nlines = linecount[0].v[j];
	  int minlines = pd->min_nozzles;
	  int extralines = 0;
	  if (nlines < minlines)
	    {
	      extralines = minlines - nlines;
	      nlines = minlines;
	    }
	  set_vertical_position(sw, pass, model, v);
	  set_color(sw, pass, model, v, j);
	  set_horizontal_position(sw, pass, model, v, hoffset, ydpi, xdpi,
				  vertical_subpass);
	  send_print_command(sw, pass, model, j, lwidth, v, hoffset, ydpi,
			     xdpi, physical_xdpi, nlines);

	  /*
	   * Send the data
	   */
	  stpi_zfwrite((const char *)bufs[0].v[j], lineoffs[0].v[j], 1, v);
	  if (extralines)
	    send_extra_data(sw, v, extralines, lwidth);
	  stpi_send_command(v, "\r", "");
	  pd->printed_something = 1;
	}
      lineoffs[0].v[j] = 0;
      linecount[0].v[j] = 0;
    }

  sw->last_pass = pass->pass;
  pass->pass = -1;
}


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

