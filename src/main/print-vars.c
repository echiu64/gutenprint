/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
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
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include "vars.h"
#include <string.h>

#define COOKIE_VARS      0x1a18376c

typedef struct stp_internal_option
{
  int	cookie;
  char *name;
  size_t length;
  char *data;
  struct stp_internal_option *next;
  struct stp_internal_option *prev;
} stp_internal_option_t;

typedef struct					/* Plug-in variables */
{
  int	cookie;
  const char *driver;		/* Name of printer "driver" */
  int	output_type;		/* Color or grayscale output */
  float	brightness;		/* Output brightness */
  float gamma;                  /* Gamma */
  float contrast;		/* Output Contrast */
  float	cyan;			/* Output red level */
  float	magenta;		/* Output green level */
  float	yellow;			/* Output blue level */
  float	saturation;		/* Output saturation */
  float	density;		/* Maximum output density */
  const char *ppd_file;		/* PPD file */
  const char *resolution;	/* Resolution */
  const char *media_size_name;	/* Media size */
  const char *media_type;	/* Media type */
  const char *media_source;	/* Media source */
  const char *ink_type;		/* Ink or cartridge */
  const char *dither_algorithm;	/* Dithering algorithm */
  int	left;			/* Offset from left-upper corner, points */
  int	top;			/* ... */
  int	width;			/* Width of the image, points */
  int	height;			/* ... */
  int	image_type;		/* Image type (line art etc.) */
  float app_gamma;		/* Application gamma */
  int	page_width;		/* Width of page in points */
  int	page_height;		/* Height of page in points */
  int	input_color_model;	/* Color model for this device */
  int	output_color_model;	/* Color model for this device */
  int	page_number;
  stp_job_mode_t job_mode;
  void  *color_data;		/* Private data of the color module */
  void	*(*copy_color_data_func)(const stp_vars_t);
  void	(*destroy_color_data_func)(stp_vars_t);
  void  *driver_data;		/* Private data of the family driver module */
  void	*(*copy_driver_data_func)(const stp_vars_t);
  void	(*destroy_driver_data_func)(stp_vars_t);
  void (*outfunc)(void *data, const char *buffer, size_t bytes);
  void *outdata;
  void (*errfunc)(void *data, const char *buffer, size_t bytes);
  void *errdata;
  stp_internal_option_t *options;
  int verified;			/* Ensure that params are OK! */
} stp_internal_vars_t;

static const stp_internal_vars_t default_vars =
{
	COOKIE_VARS,
	N_ ("ps2"),	       	/* Name of printer "driver" */
	OUTPUT_COLOR,		/* Color or grayscale output */
	1.0,			/* Output brightness */
	1.0,			/* Screen gamma */
	1.0,			/* Contrast */
	1.0,			/* Cyan */
	1.0,			/* Magenta */
	1.0,			/* Yellow */
	1.0,			/* Output saturation */
	1.0,			/* Density */
	"",			/* Name of PPD file */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	-1,			/* left */
	-1,			/* top */
	-1,			/* width */
	-1,			/* height */
	IMAGE_CONTINUOUS,	/* Image type */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0,			/* Page height */
	COLOR_MODEL_RGB,	/* Input color model */
	COLOR_MODEL_RGB,	/* Output color model */
	0,			/* Page number */
	STP_JOB_MODE_PAGE	/* Job mode */
};

static const stp_internal_vars_t min_vars =
{
	COOKIE_VARS,
	N_ ("ps2"),		/* Name of printer "driver" */
	0,			/* Color or grayscale output */
	0,			/* Output brightness */
	0.1,			/* Screen gamma */
	0,			/* Contrast */
	0,			/* Cyan */
	0,			/* Magenta */
	0,			/* Yellow */
	0,			/* Output saturation */
	.1,			/* Density */
	"",			/* Name of PPD file */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	-1,			/* left */
	-1,			/* top */
	-1,			/* width */
	-1,			/* height */
	0,			/* Image type */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0,			/* Page height */
	0,			/* Input color model */
	0,			/* Output color model */
	0,			/* Page number */
	STP_JOB_MODE_PAGE	/* Job mode */
};

static const stp_internal_vars_t max_vars =
{
	COOKIE_VARS,
	N_ ("ps2"),		/* Name of printer "driver" */
	OUTPUT_RAW_PRINTER,	/* Color or grayscale output */
	2.0,			/* Output brightness */
	4.0,			/* Screen gamma */
	4.0,			/* Contrast */
	4.0,			/* Cyan */
	4.0,			/* Magenta */
	4.0,			/* Yellow */
	9.0,			/* Output saturation */
	2.0,			/* Density */
	"",			/* Name of PPD file */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	-1,			/* left */
	-1,			/* top */
	-1,			/* width */
	-1,			/* height */
	NIMAGE_TYPES - 1,	/* Image type */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0,			/* Page height */
	NCOLOR_MODELS - 1,	/* Input color model */
	NCOLOR_MODELS - 1,	/* Output color model */
	INT_MAX,		/* Page number */
	STP_JOB_MODE_JOB	/* Job mode */
};

static const stp_parameter_t global_parameters[] =
  {
    {
      "PageSize", N_("Page Size"),
      N_("Size of the paper being printed to"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_PAGE_SIZE,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "MediaType", N_("Media Type"),
      N_("Type of media (plain paper, photo paper, etc.)"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "InputSlot", N_("Media Source"),
      N_("Source (input slot) of the media"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "InkType", N_("Ink Type"),
      N_("Type of ink in the printer"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "Resolution", N_("Resolutions"),
      N_("Resolution and quality of the print"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "DitherAlgorithm", N_("Dither Algorithm"),
      N_("Choose the dither algorithm to be used.\n"
	 "Adaptive Hybrid usually produces the best all-around quality.\n"
	 "EvenTone is a new, experimental algorithm that often produces excellent results.\n"
	 "Ordered is faster and produces almost as good quality on photographs.\n"
	 "Fast and Very Fast are considerably faster, and work well for text and line art.\n"
	 "Hybrid Floyd-Steinberg generally produces inferior output."),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "Brightness", N_("Brightness"),
      N_("Brightness of the print (0 is solid black, 2 is solid white)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "Contrast", N_("Contrast"),
      N_("Contrast of the print (0 is solid gray)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "Density", N_("Density"),
      N_("Adjust the density (amount of ink) of the print. "
	 "Reduce the density if the ink bleeds through the "
	 "paper or smears; increase the density if black "
	 "regions are not solid."),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "Gamma", N_("Gamma"),
      N_("Adjust the gamma of the print. Larger values will "
	 "produce a generally brighter print, while smaller "
	 "values will produce a generally darker print. "
	 "Black and white will remain the same, unlike with "
	 "the brightness adjustment."),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "AppGamma", N_("AppGamma"),
      N_("Gamma value assumed by application"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED
    },
    {
      "Cyan", N_("Cyan"),
      N_("Adjust the cyan balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "Magenta", N_("Magenta"),
      N_("Adjust the magenta balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "Yellow", N_("Yellow"),
      N_("Adjust the yellow balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      "Saturation", N_("Saturation"),
      N_("Adjust the saturation (color balance) of the print\n"
	 "Use zero saturation to produce grayscale output "
	 "using color and black inks"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC
    },
    {
      NULL, NULL, NULL,
      STP_PARAMETER_TYPE_INVALID, STP_PARAMETER_CLASS_INVALID,
      STP_PARAMETER_LEVEL_INVALID
    }
  };

const stp_vars_t
stp_default_settings(void)
{
  return (stp_vars_t) &default_vars;
}

const stp_vars_t
stp_maximum_settings(void)
{
  return (stp_vars_t) &max_vars;
}

const stp_vars_t
stp_minimum_settings(void)
{
  return (stp_vars_t) &min_vars;
}

stp_vars_t
stp_allocate_vars(void)
{
  stp_internal_vars_t *retval = stp_zalloc(sizeof(stp_internal_vars_t));
  retval->cookie = COOKIE_VARS;
  stp_copy_vars(retval, (stp_vars_t)&default_vars);
  return (retval);
}

#define SAFE_FREE(x)				\
do						\
{						\
  if ((x))					\
    stp_free((char *)(x));			\
  ((x)) = NULL;					\
} while (0)

static void
check_vars(const stp_internal_vars_t *v)
{
  if (v->cookie != COOKIE_VARS)
    {
      stp_erprintf("Bad stp_vars_t!\n");
      stp_abort();
    }
}

void
stp_free_vars(stp_vars_t vv)
{
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;
  check_vars(v);
  if (stp_get_destroy_color_data_func(vv))
    (*stp_get_destroy_color_data_func(vv))(vv);
  if (stp_get_destroy_driver_data_func(vv))
    (*stp_get_destroy_driver_data_func(vv))(vv);
  SAFE_FREE(v->driver);
  SAFE_FREE(v->ppd_file);
  SAFE_FREE(v->resolution);
  SAFE_FREE(v->media_size_name);
  SAFE_FREE(v->media_type);
  SAFE_FREE(v->media_source);
  SAFE_FREE(v->ink_type);
  SAFE_FREE(v->dither_algorithm);
  stp_free(v);
}

#define DEF_STRING_FUNCS(s)				\
void							\
stp_set_##s(stp_vars_t vv, const char *val)		\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  check_vars(v);					\
  if (v->s == val)					\
    return;						\
  SAFE_FREE(v->s);					\
  v->s = stp_strdup(val);				\
  v->verified = 0;					\
}							\
							\
void							\
stp_set_##s##_n(stp_vars_t vv, const char *val, int n)	\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  check_vars(v);					\
  if (v->s == val)					\
    return;						\
  SAFE_FREE(v->s);					\
  v->s = stp_strndup(val, n);				\
  v->verified = 0;					\
}							\
							\
const char *						\
stp_get_##s(const stp_vars_t vv)			\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  check_vars(v);					\
  return v->s;						\
}

#define DEF_STRING_FUNCS_INTERNAL(s)			\
static void						\
stp_set_##s##_n(stp_vars_t vv, const char *val, int n)	\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  check_vars(v);					\
  if (v->s == val)					\
    return;						\
  SAFE_FREE(v->s);					\
  v->s = stp_strndup(val, n);				\
  v->verified = 0;					\
}							\
							\
static const char *					\
stp_get_##s(const stp_vars_t vv)			\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  check_vars(v);					\
  return v->s;						\
}

#define DEF_FUNCS(s, t, u)				\
u void							\
stp_set_##s(stp_vars_t vv, t val)			\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  check_vars(v);					\
  v->verified = 0;					\
  v->s = val;						\
}							\
							\
u t							\
stp_get_##s(const stp_vars_t vv)			\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  check_vars(v);					\
  return v->s;						\
}

DEF_STRING_FUNCS(driver)
DEF_STRING_FUNCS(ppd_file)
DEF_FUNCS(output_type, int, )
DEF_FUNCS(left, int, )
DEF_FUNCS(top, int, )
DEF_FUNCS(width, int, )
DEF_FUNCS(height, int, )
DEF_FUNCS(image_type, int, )
DEF_FUNCS(page_width, int, )
DEF_FUNCS(page_height, int, )
DEF_FUNCS(input_color_model, int, )
DEF_FUNCS(output_color_model, int, )
DEF_FUNCS(page_number, int, )
DEF_FUNCS(job_mode, stp_job_mode_t, )
DEF_FUNCS(outdata, void *, )
DEF_FUNCS(errdata, void *, )
DEF_FUNCS(color_data, void *, )
DEF_FUNCS(copy_color_data_func, copy_data_func_t, )
DEF_FUNCS(destroy_color_data_func, destroy_data_func_t, )
DEF_FUNCS(driver_data, void *, )
DEF_FUNCS(copy_driver_data_func, copy_data_func_t, )
DEF_FUNCS(destroy_driver_data_func, destroy_data_func_t, )
DEF_FUNCS(outfunc, stp_outfunc_t, )
DEF_FUNCS(errfunc, stp_outfunc_t, )

DEF_STRING_FUNCS_INTERNAL(resolution)
DEF_STRING_FUNCS_INTERNAL(media_size_name)
DEF_STRING_FUNCS_INTERNAL(media_type)
DEF_STRING_FUNCS_INTERNAL(media_source)
DEF_STRING_FUNCS_INTERNAL(ink_type)
DEF_STRING_FUNCS_INTERNAL(dither_algorithm)
DEF_FUNCS(brightness, float, static)
DEF_FUNCS(gamma, float, static)
DEF_FUNCS(contrast, float, static)
DEF_FUNCS(cyan, float, static)
DEF_FUNCS(magenta, float, static)
DEF_FUNCS(yellow, float, static)
DEF_FUNCS(saturation, float, static)
DEF_FUNCS(density, float, static)
DEF_FUNCS(app_gamma, float, static)

void
stp_set_verified(stp_vars_t vv, int val)
{
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;
  check_vars(v);
  v->verified = val;
}

int
stp_get_verified(const stp_vars_t vv)
{
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;
  check_vars(v);
  return v->verified;
}

void
stp_copy_options(stp_vars_t vd, const stp_vars_t vs)
{
  const stp_internal_vars_t *src = (const stp_internal_vars_t *)vs;
  stp_internal_vars_t *dest = (stp_internal_vars_t *)vd;
  stp_internal_option_t *popt = NULL;
  stp_internal_option_t *opt;
  check_vars(src);
  check_vars(dest);
  opt = (stp_internal_option_t *) src->options;
  if (opt)
    {
      stp_internal_option_t *nopt = stp_malloc(sizeof(stp_internal_option_t));
      stp_set_verified(vd, 0);
      dest->options = nopt;
      memcpy(nopt, opt, sizeof(stp_internal_option_t));
      nopt->name = stp_malloc(stp_strlen(opt->name) + 1);
      strcpy(nopt->name, opt->name);
      nopt->data = stp_malloc(opt->length);
      memcpy(nopt->data, opt->data, opt->length);
      opt = opt->next;
      popt = nopt;
      while (opt)
        {
          nopt = stp_malloc(sizeof(stp_internal_option_t));
          memcpy(nopt, opt, sizeof(stp_internal_option_t));
          nopt->prev = popt;
          popt->next = nopt;
          nopt->name = stp_malloc(stp_strlen(opt->name) + 1);
          strcpy(nopt->name, opt->name);
          nopt->data = stp_malloc(opt->length);
          memcpy(nopt->data, opt->data, opt->length);
          opt = opt->next;
          popt = nopt;
        }
    }
}

void
stp_set_string_parameter_n(stp_vars_t v, const char *parameter,
			   const char *value, int bytes)
{
  if      (strcmp(parameter, "Resolution") == 0)
    stp_set_resolution_n(v, value, bytes);
  else if (strcmp(parameter, "PageSize") == 0)
    stp_set_media_size_name_n(v, value, bytes);
  else if (strcmp(parameter, "MediaType") == 0)
    stp_set_media_type_n(v, value, bytes);
  else if (strcmp(parameter, "InputSlot") == 0)
    stp_set_media_source_n(v, value, bytes);
  else if (strcmp(parameter, "InkType") == 0)
    stp_set_ink_type_n(v, value, bytes);
  else if (strcmp(parameter, "DitherAlgorithm") == 0)
    stp_set_dither_algorithm_n(v, value, bytes);
  else
    stp_eprintf(v, "WARNING: Attempt to set unknown parameter %s to %s\n",
		parameter, value);
}

void
stp_set_string_parameter(stp_vars_t v, const char *parameter,
			 const char *value)
{
  int byte_count = 0;
  if (value)
    byte_count = strlen(value);
  stp_set_string_parameter_n(v, parameter, value, byte_count);
}

void
stp_set_curve_parameter(stp_vars_t v, const char *parameter,
			const stp_curve_t curve)
{
  stp_eprintf(v, "WARNING: Attempt to retrieve unknown parameter %s\n",
	      parameter);
}

void
stp_set_float_parameter(stp_vars_t v, const char *parameter, double value)
{
  if (strcmp(parameter, "Brightness") == 0)
    stp_set_brightness(v, value);
  else if (strcmp(parameter, "Contrast") == 0)
    stp_set_contrast(v, value);
  else if (strcmp(parameter, "Density") == 0)
    stp_set_density(v, value);
  else if (strcmp(parameter, "Gamma") == 0)
    stp_set_gamma(v, value);
  else if (strcmp(parameter, "AppGamma") == 0)
    stp_set_app_gamma(v, value);
  else if (strcmp(parameter, "Cyan") == 0)
    stp_set_cyan(v, value);
  else if (strcmp(parameter, "Magenta") == 0)
    stp_set_magenta(v, value);
  else if (strcmp(parameter, "Yellow") == 0)
    stp_set_yellow(v, value);
  else if (strcmp(parameter, "Saturation") == 0)
    stp_set_saturation(v, value);
}

const double
stp_get_float_parameter(stp_vars_t v, const char *parameter)
{
  if (strcmp(parameter, "Brightness") == 0)
    return stp_get_brightness(v);
  else if (strcmp(parameter, "Contrast") == 0)
    return stp_get_contrast(v);
  else if (strcmp(parameter, "Density") == 0)
    return stp_get_density(v);
  else if (strcmp(parameter, "AppGamma") == 0)
    return stp_get_app_gamma(v);
  else if (strcmp(parameter, "Gamma") == 0)
    return stp_get_gamma(v);
  else if (strcmp(parameter, "Cyan") == 0)
    return stp_get_cyan(v);
  else if (strcmp(parameter, "Magenta") == 0)
    return stp_get_magenta(v);
  else if (strcmp(parameter, "Yellow") == 0)
    return stp_get_yellow(v);
  else if (strcmp(parameter, "Saturation") == 0)
    return stp_get_saturation(v);
  else
    {
      stp_eprintf(v, "WARNING: Attempt to retrieve unknown parameter %s\n",
		  parameter);
      return 0;
    }
}

const stp_curve_t
stp_get_curve_parameter(stp_vars_t v, const char *parameter)
{
  stp_eprintf(v, "WARNING: Attempt to retrieve unknown parameter %s\n",
	      parameter);
  return NULL;
}

const char *
stp_get_string_parameter(stp_vars_t v, const char *parameter)
{
  if      (strcmp(parameter, "Resolution") == 0)
    return stp_get_resolution(v);
  else if (strcmp(parameter, "PageSize") == 0)
    return stp_get_media_size_name(v);
  else if (strcmp(parameter, "MediaType") == 0)
    return stp_get_media_type(v);
  else if (strcmp(parameter, "InputSlot") == 0)
    return stp_get_media_source(v);
  else if (strcmp(parameter, "InkType") == 0)
    return stp_get_ink_type(v);
  else if (strcmp(parameter, "DitherAlgorithm") == 0)
    return stp_get_dither_algorithm(v);
  else
    {
      stp_eprintf(v, "WARNING: Attempt to retrieve unknown parameter %s\n",
		  parameter);
      return NULL;
    }
}

void
stp_fill_parameter_settings(stp_parameter_t *desc, const char *name)
{
  const stp_parameter_t *param = global_parameters;
  while (param->name)
    {
      if (strcmp(name, param->name) == 0)
	{
	  desc->type = param->type;
	  desc->level = param->level;
	  desc->class = param->class;
	  desc->name = stp_strdup(param->name);
	  desc->text = stp_strdup(param->text);
	  desc->help = stp_strdup(param->help);
	  return;
	}
      param++;
    }
}

void
stp_copy_vars(stp_vars_t vd, const stp_vars_t vs)
{
  int count;
  int i;
  const stp_parameter_t *params;
  if (vs == vd)
    return;
  stp_set_driver(vd, stp_get_driver(vs));
  if (stp_get_copy_driver_data_func(vs))
    stp_set_driver_data(vd, (stp_get_copy_driver_data_func(vs))(vs));
  else
    stp_set_driver_data(vd, stp_get_driver_data(vs));
  if (stp_get_copy_color_data_func(vs))
    stp_set_color_data(vd, (stp_get_copy_color_data_func(vs))(vs));
  else
    stp_set_color_data(vd, stp_get_color_data(vs));
  stp_set_copy_driver_data_func(vd, stp_get_copy_driver_data_func(vs));
  stp_set_copy_color_data_func(vd, stp_get_copy_color_data_func(vs));
  stp_set_ppd_file(vd, stp_get_ppd_file(vs));
  stp_set_output_type(vd, stp_get_output_type(vs));
  stp_set_left(vd, stp_get_left(vs));
  stp_set_top(vd, stp_get_top(vs));
  stp_set_width(vd, stp_get_width(vs));
  stp_set_height(vd, stp_get_height(vs));
  stp_set_image_type(vd, stp_get_image_type(vs));
  stp_set_page_width(vd, stp_get_page_width(vs));
  stp_set_page_height(vd, stp_get_page_height(vs));
  stp_set_input_color_model(vd, stp_get_input_color_model(vd));
  stp_set_output_color_model(vd, stp_get_output_color_model(vd));
  stp_set_outdata(vd, stp_get_outdata(vs));
  stp_set_errdata(vd, stp_get_errdata(vs));
  stp_set_outfunc(vd, stp_get_outfunc(vs));
  stp_set_errfunc(vd, stp_get_errfunc(vs));
  params = stp_list_parameters(vs, &count);
  for (i = 0; i < count; i++)
    switch (params[i].type)
      {
      case STP_PARAMETER_TYPE_STRING_LIST:
      case STP_PARAMETER_TYPE_FILE:
	stp_set_string_parameter(vd, params[i].name,
				 stp_get_string_parameter(vs, params[i].name));
	break;
      case STP_PARAMETER_TYPE_DOUBLE:
	stp_set_float_parameter(vd, params[i].name,
				stp_get_float_parameter(vs, params[i].name));
	break;
      case STP_PARAMETER_TYPE_CURVE:
	stp_set_curve_parameter(vd, params[i].name,
				stp_get_curve_parameter(vs, params[i].name));
	break;
      default:
	break;
      }
  stp_copy_options(vd, vs);
  stp_set_verified(vd, stp_get_verified(vs));
}

stp_vars_t
stp_allocate_copy(const stp_vars_t vs)
{
  stp_vars_t vd = stp_allocate_vars();
  stp_copy_vars(vd, vs);
  return (vd);
}

void
stp_merge_printvars(stp_vars_t user, const stp_vars_t print)
{
  int count;
  int i;
  const stp_parameter_t *params = stp_list_parameters(print, &count);
  for (i = 0; i < count; i++)
    if (params[i].type == STP_PARAMETER_TYPE_DOUBLE &&
	params[i].class == STP_PARAMETER_CLASS_OUTPUT &&
	params[i].level == STP_PARAMETER_LEVEL_BASIC)
      {
	stp_parameter_t desc;
	double usrval = stp_get_float_parameter(user, params[i].name);
	double prnval = stp_get_float_parameter(print, params[i].name);
	stp_describe_parameter(print, params[i].name, &desc);
	if (strcmp(params[i].name, "Gamma") == 0)
	  usrval /= prnval;
	else
	  usrval *= prnval;
	if (usrval < desc.bounds.dbl.lower)
	  usrval = desc.bounds.dbl.lower;
	else if (usrval > desc.bounds.dbl.upper)
	  usrval = desc.bounds.dbl.upper;
	stp_set_float_parameter(user, params[i].name, usrval);
      }
  if (stp_get_output_type(print) == OUTPUT_GRAY &&
      (stp_get_output_type(user) == OUTPUT_COLOR ||
       stp_get_output_type(user) == OUTPUT_RAW_CMYK))
    stp_set_output_type(user, OUTPUT_GRAY);
}

void
stp_set_printer_defaults(stp_vars_t v, const stp_printer_t p)
{
  const stp_parameter_t *params;
  int count;
  int i;
  stp_parameter_t desc;
  stp_set_driver(v, stp_printer_get_driver(p));
  params = stp_list_parameters(v, &count);
  for (i = 0; i < count; i++)
    {
      if (params[i].type == STP_PARAMETER_TYPE_STRING_LIST)
	{
	  stp_describe_parameter(v, params[i].name, &desc);
	  stp_set_string_parameter(v, params[i].name, desc.deflt.str);
	  stp_string_list_free(desc.bounds.str);
	}
    }
}

void
stp_describe_internal_parameter(const stp_vars_t v, const char *name,
				stp_parameter_t *description)
{
  const stp_parameter_t *param = global_parameters;
  if (strcmp(name, "DitherAlgorithm") == 0)
    {
      stp_fill_parameter_settings(description, name);
      description->bounds.str = stp_string_list_allocate();
      stp_dither_algorithms(description->bounds.str);
      description->deflt.str = 
	stp_string_list_param(description->bounds.str, 0)->name;
      return;
    }
  while (param->type)
    {
      if (param->type == STP_PARAMETER_TYPE_DOUBLE &&
	  strcmp(name, param->name) == 0)
	{
	  stp_fill_parameter_settings(description, name);
	  if (description->type == STP_PARAMETER_TYPE_DOUBLE)
	    {
	      description->bounds.dbl.lower =
		stp_get_float_parameter(stp_minimum_settings(), name);
	      description->bounds.dbl.upper =
		stp_get_float_parameter(stp_maximum_settings(), name);
	      description->deflt.dbl =
		stp_get_float_parameter(stp_default_settings(), name);
	    }
	  return;
	}
      param++;
    }
  description->type = STP_PARAMETER_TYPE_INVALID;
}

const stp_parameter_t *
stp_list_parameters(const stp_vars_t v, int *count)
{
  *count = (sizeof(global_parameters) / sizeof(const stp_parameter_t)) - 1;
  return global_parameters;
}
