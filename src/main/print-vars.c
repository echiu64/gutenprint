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
#include <limits.h>
#include "vars.h"
#include <string.h>

static const stp_internal_vars_t default_vars =
{
	N_ ("ps2"),	       	/* Name of printer "driver" */
	"",			/* Name of PPD file */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	OUTPUT_COLOR,		/* Color or grayscale output */
	1.0,			/* Output brightness */
	-1,			/* left */
	-1,			/* top */
	-1,			/* width */
	-1,			/* height */
	1.0,			/* Screen gamma */
	1.0,			/* Contrast */
	1.0,			/* Cyan */
	1.0,			/* Magenta */
	1.0,			/* Yellow */
	1.0,			/* Output saturation */
	1.0,			/* Density */
	IMAGE_CONTINUOUS,	/* Image type */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0,			/* Page height */
	COLOR_MODEL_RGB,	/* Input color model */
	COLOR_MODEL_RGB		/* Output color model */
};

static const stp_internal_vars_t min_vars =
{
	N_ ("ps2"),		/* Name of printer "driver" */
	"",			/* Name of PPD file */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	0,			/* Color or grayscale output */
	0,			/* Output brightness */
	-1,			/* left */
	-1,			/* top */
	-1,			/* width */
	-1,			/* height */
	0.1,			/* Screen gamma */
	0,			/* Contrast */
	0,			/* Cyan */
	0,			/* Magenta */
	0,			/* Yellow */
	0,			/* Output saturation */
	.1,			/* Density */
	0,			/* Image type */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0,			/* Page height */
	0,			/* Input color model */
	0			/* Output color model */
};

static const stp_internal_vars_t max_vars =
{
	N_ ("ps2"),		/* Name of printer "driver" */
	"",			/* Name of PPD file */
	"",			/* Output resolution */
	"",			/* Size of output media */
	"",			/* Type of output media */
	"",			/* Source of output media */
	"",			/* Ink type */
	"",			/* Dither algorithm */
	OUTPUT_RAW_PRINTER,	/* Color or grayscale output */
	2.0,			/* Output brightness */
	-1,			/* left */
	-1,			/* top */
	-1,			/* width */
	-1,			/* height */
	4.0,			/* Screen gamma */
	4.0,			/* Contrast */
	4.0,			/* Cyan */
	4.0,			/* Magenta */
	4.0,			/* Yellow */
	9.0,			/* Output saturation */
	2.0,			/* Density */
	NIMAGE_TYPES - 1,	/* Image type */
	1.0,			/* Application gamma placeholder */
	0,			/* Page width */
	0,			/* Page height */
	NCOLOR_MODELS - 1,	/* Input color model */
	NCOLOR_MODELS - 1	/* Output color model */
};

stp_vars_t
stp_allocate_vars(void)
{
  void *retval = stp_zalloc(sizeof(stp_internal_vars_t));
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

static size_t
c_strlen(const char *s)
{
  return strlen(s);
}

static char *
c_strndup(const char *s, int n)
{
  char *ret;
  if (!s || n < 0)
    {
      ret = stp_malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    {
      ret = stp_malloc(n + 1);
      memcpy(ret, s, n);
      ret[n] = 0;
      return ret;
    }
}

static char *
c_strdup(const char *s)
{
  char *ret;
  if (!s)
    {
      ret = stp_malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    return c_strndup(s, c_strlen(s));
}

void
stp_free_vars(stp_vars_t vv)
{
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;
  SAFE_FREE(v->driver);
  SAFE_FREE(v->ppd_file);
  SAFE_FREE(v->resolution);
  SAFE_FREE(v->media_size);
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
  if (v->s == val)					\
    return;						\
  SAFE_FREE(v->s);					\
  v->s = c_strdup(val);					\
  v->verified = 0;					\
}							\
							\
void							\
stp_set_##s##_n(stp_vars_t vv, const char *val, int n)	\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  if (v->s == val)					\
    return;						\
  SAFE_FREE(v->s);					\
  v->s = c_strndup(val, n);				\
  v->verified = 0;					\
}							\
							\
const char *						\
stp_get_##s(const stp_vars_t vv)			\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  return v->s;						\
}

#define DEF_FUNCS(s, t)					\
void							\
stp_set_##s(stp_vars_t vv, t val)			\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  v->verified = 0;					\
  v->s = val;						\
}							\
							\
t							\
stp_get_##s(const stp_vars_t vv)			\
{							\
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;	\
  return v->s;						\
}

DEF_STRING_FUNCS(driver)
DEF_STRING_FUNCS(ppd_file)
DEF_STRING_FUNCS(resolution)
DEF_STRING_FUNCS(media_size)
DEF_STRING_FUNCS(media_type)
DEF_STRING_FUNCS(media_source)
DEF_STRING_FUNCS(ink_type)
DEF_STRING_FUNCS(dither_algorithm)
DEF_FUNCS(output_type, int)
DEF_FUNCS(left, int)
DEF_FUNCS(top, int)
DEF_FUNCS(width, int)
DEF_FUNCS(height, int)
DEF_FUNCS(image_type, int)
DEF_FUNCS(page_width, int)
DEF_FUNCS(page_height, int)
DEF_FUNCS(input_color_model, int)
DEF_FUNCS(output_color_model, int)
DEF_FUNCS(brightness, float)
DEF_FUNCS(gamma, float)
DEF_FUNCS(contrast, float)
DEF_FUNCS(cyan, float)
DEF_FUNCS(magenta, float)
DEF_FUNCS(yellow, float)
DEF_FUNCS(saturation, float)
DEF_FUNCS(density, float)
DEF_FUNCS(app_gamma, float)
DEF_FUNCS(lut, void *)
DEF_FUNCS(outdata, void *)
DEF_FUNCS(errdata, void *)
DEF_FUNCS(driver_data, void *)
DEF_FUNCS(cmap, unsigned char *)
DEF_FUNCS(outfunc, stp_outfunc_t)
DEF_FUNCS(errfunc, stp_outfunc_t)

void
stp_set_verified(stp_vars_t vv, int val)
{
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;
  v->verified = val;
}

int
stp_get_verified(const stp_vars_t vv)
{
  stp_internal_vars_t *v = (stp_internal_vars_t *) vv;
  return v->verified;
}

void
stp_copy_options(stp_vars_t vd, const stp_vars_t vs)
{
  const stp_internal_vars_t *src = (const stp_internal_vars_t *)vs;
  stp_internal_vars_t *dest = (stp_internal_vars_t *)vd;
  stp_internal_option_t *opt = (stp_internal_option_t *) src->options;
  stp_internal_option_t *popt = NULL;
  if (opt)
    {
      stp_internal_option_t *nopt = stp_malloc(sizeof(stp_internal_option_t));
      stp_set_verified(vd, 0);
      dest->options = nopt;
      memcpy(nopt, opt, sizeof(stp_internal_option_t));
      nopt->name = stp_malloc(c_strlen(opt->name) + 1);
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
          nopt->name = stp_malloc(c_strlen(opt->name) + 1);
          strcpy(nopt->name, opt->name);
          nopt->data = stp_malloc(opt->length);
          memcpy(nopt->data, opt->data, opt->length);
          opt = opt->next;
          popt = nopt;
        }
    }
}

void
stp_copy_vars(stp_vars_t vd, const stp_vars_t vs)
{
  if (vs == vd)
    return;
  stp_set_driver(vd, stp_get_driver(vs));
  stp_set_driver_data(vd, stp_get_driver_data(vs));
  stp_set_ppd_file(vd, stp_get_ppd_file(vs));
  stp_set_resolution(vd, stp_get_resolution(vs));
  stp_set_media_size(vd, stp_get_media_size(vs));
  stp_set_media_type(vd, stp_get_media_type(vs));
  stp_set_media_source(vd, stp_get_media_source(vs));
  stp_set_ink_type(vd, stp_get_ink_type(vs));
  stp_set_dither_algorithm(vd, stp_get_dither_algorithm(vs));
  stp_set_output_type(vd, stp_get_output_type(vs));
  stp_set_left(vd, stp_get_left(vs));
  stp_set_top(vd, stp_get_top(vs));
  stp_set_width(vd, stp_get_width(vs));
  stp_set_height(vd, stp_get_height(vs));
  stp_set_image_type(vd, stp_get_image_type(vs));
  stp_set_page_width(vd, stp_get_page_width(vs));
  stp_set_page_height(vd, stp_get_page_height(vs));
  stp_set_brightness(vd, stp_get_brightness(vs));
  stp_set_gamma(vd, stp_get_gamma(vs));
  stp_set_contrast(vd, stp_get_contrast(vs));
  stp_set_cyan(vd, stp_get_cyan(vs));
  stp_set_magenta(vd, stp_get_magenta(vs));
  stp_set_yellow(vd, stp_get_yellow(vs));
  stp_set_saturation(vd, stp_get_saturation(vs));
  stp_set_density(vd, stp_get_density(vs));
  stp_set_app_gamma(vd, stp_get_app_gamma(vs));
  stp_set_input_color_model(vd, stp_get_input_color_model(vd));
  stp_set_output_color_model(vd, stp_get_output_color_model(vd));
  stp_set_lut(vd, stp_get_lut(vs));
  stp_set_outdata(vd, stp_get_outdata(vs));
  stp_set_errdata(vd, stp_get_errdata(vs));
  stp_set_cmap(vd, stp_get_cmap(vs));
  stp_set_outfunc(vd, stp_get_outfunc(vs));
  stp_set_errfunc(vd, stp_get_errfunc(vs));
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

#define ICLAMP(value)						\
do								\
{								\
  if (stp_get_##value(user) < stp_get_##value(min))		\
    stp_set_##value(user, stp_get_##value(min));		\
  else if (stp_get_##value(user) > stp_get_##value(max))	\
    stp_set_##value(user, stp_get_##value(max));		\
} while (0)

void
stp_merge_printvars(stp_vars_t user, const stp_vars_t print)
{
  const stp_vars_t max = stp_maximum_settings();
  const stp_vars_t min = stp_minimum_settings();
  stp_set_cyan(user, stp_get_cyan(user) * stp_get_cyan(print));
  ICLAMP(cyan);
  stp_set_magenta(user, stp_get_magenta(user) * stp_get_magenta(print));
  ICLAMP(magenta);
  stp_set_yellow(user, stp_get_yellow(user) * stp_get_yellow(print));
  ICLAMP(yellow);
  stp_set_contrast(user, stp_get_contrast(user) * stp_get_contrast(print));
  ICLAMP(contrast);
  stp_set_brightness(user, stp_get_brightness(user)*stp_get_brightness(print));
  ICLAMP(brightness);
  stp_set_gamma(user, stp_get_gamma(user) / stp_get_gamma(print));
  ICLAMP(gamma);
  stp_set_saturation(user, stp_get_saturation(user)*stp_get_saturation(print));
  ICLAMP(saturation);
  stp_set_density(user, stp_get_density(user) * stp_get_density(print));
  ICLAMP(density);
  if (stp_get_output_type(print) == OUTPUT_GRAY &&
      stp_get_output_type(user) == OUTPUT_COLOR)
    stp_set_output_type(user, OUTPUT_GRAY);
}

const stp_vars_t
stp_default_settings()
{
  return (stp_vars_t) &default_vars;
}

const stp_vars_t
stp_maximum_settings()
{
  return (stp_vars_t) &max_vars;
}

const stp_vars_t
stp_minimum_settings()
{
  return (stp_vars_t) &min_vars;
}

const char *
stp_default_dither_algorithm(void)
{
  return stp_dither_algorithm_name(0);
}
