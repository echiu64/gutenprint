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
#include <string.h>

#define FMIN(a, b) ((a) < (b) ? (a) : (b))

static const char *global_parameters[] =
  {
    "PageSize",
    "Resolution",
    "MediaType",
    "InputSlot",
    "InkType",
    "DitherAlgorithm"
  };

const stp_printer_t
stp_get_printer_by_long_name(const char *long_name)
{
  int i;
  if (!long_name)
    return NULL;
  for (i = 0; i < stp_known_printers(); i++)
    {
      const stp_printer_t val = stp_get_printer_by_index(i);
      if (!strcmp(stp_printer_get_long_name(val), long_name))
	return val;
    }
  return NULL;
}

const stp_printer_t
stp_get_printer_by_driver(const char *driver)
{
  int i;
  if (!driver)
    return NULL;
  for (i = 0; i < stp_known_printers(); i++)
    {
      const stp_printer_t val = stp_get_printer_by_index(i);
      if (!strcmp(stp_printer_get_driver(val), driver))
	return val;
    }
  return NULL;
}

int
stp_get_printer_index_by_driver(const char *driver)
{
  int idx = 0;
  if (!driver)
    return -1;
  for (idx = 0; idx < stp_known_printers(); idx++)
    {
      const stp_printer_t val = stp_get_printer_by_index(idx);
      if (!strcmp(stp_printer_get_driver(val), driver))
	return idx;
    }
  return -1;
}

stp_param_list_t
stp_printer_get_parameters(const stp_printer_t printer,
			   const stp_vars_t v,
			   const char *name)
{
  const stp_printfuncs_t *printfuncs = stp_printer_get_printfuncs(printer);
  return (printfuncs->parameters)(printer, v, name);
}

const char *
stp_printer_get_default_parameter(const stp_printer_t printer,
				  const stp_vars_t v,
				  const char *name)
{
  const stp_printfuncs_t *printfuncs = stp_printer_get_printfuncs(printer);
  return (printfuncs->default_parameters)(printer, v, name);
}

void
stp_printer_get_media_size(const stp_printer_t printer,
			   const stp_vars_t v,
			   int *width,
			   int *height)
{
  const stp_printfuncs_t *printfuncs = stp_printer_get_printfuncs(printer);
  (printfuncs->media_size)(printer, v, width, height);
}

void
stp_printer_get_imageable_area(const stp_printer_t printer,
			       const stp_vars_t v,
			       int *left,
			       int *right,
			       int *bottom,
			       int *top)
{
  const stp_printfuncs_t *printfuncs = stp_printer_get_printfuncs(printer);
  (printfuncs->imageable_area)(printer, v, left, right, bottom, top);
}

void
stp_printer_get_size_limit(const stp_printer_t printer,
			   const stp_vars_t v,
			   int *max_width,
			   int *max_height,
			   int *min_width,
			   int *min_height)
{
  const stp_printfuncs_t *printfuncs = stp_printer_get_printfuncs(printer);
  (printfuncs->limit)(printer, v, max_width, max_height, min_width,min_height);
}

void
stp_printer_describe_resolution(const stp_printer_t printer,
				const stp_vars_t v,
				int *x,
				int *y)
{
  const stp_printfuncs_t *printfuncs = stp_printer_get_printfuncs(printer);
  (printfuncs->describe_resolution)(printer, v, x, y);
}

int
stp_printer_verify(const stp_printer_t printer,
		   const stp_vars_t v)
{
  const stp_printfuncs_t *printfuncs = stp_printer_get_printfuncs(printer);
  return (printfuncs->verify)(printer, v);
}

int
stp_print(const stp_printer_t printer,
	  const stp_vars_t v,
	  stp_image_t *image)
{
  const stp_printfuncs_t *printfuncs = stp_printer_get_printfuncs(printer);
  return (printfuncs->print)(printer, v, image);
}

const char **
stp_printer_list_parameters(const stp_printer_t printer,
			    const stp_vars_t v,
			    int *count)
{
  *count = sizeof(global_parameters) / sizeof(char *);
  return global_parameters;
}

stp_parameter_type_t
stp_printer_parameter_type(const stp_printer_t printer,
			   const stp_vars_t v,
			   const char *parameter)
{
  if      (strcmp(parameter, "Resolution"))
    return STP_PARAMETER_TYPE_ENUM_STRING;
  else if (strcmp(parameter, "PageSize"))
    return STP_PARAMETER_TYPE_ENUM_STRING;
  else if (strcmp(parameter, "MediaType"))
    return STP_PARAMETER_TYPE_ENUM_STRING;
  else if (strcmp(parameter, "InputSlot"))
    return STP_PARAMETER_TYPE_ENUM_STRING;
  else if (strcmp(parameter, "InkType"))
    return STP_PARAMETER_TYPE_ENUM_STRING;
  else if (strcmp(parameter, "DitherAlgorithm"))
    return STP_PARAMETER_TYPE_ENUM_STRING;
  else
    return STP_PARAMETER_TYPE_INVALID;
}

stp_parameter_class_t
stp_printer_parameter_class(const stp_printer_t printer,
			    const stp_vars_t v,
			    const char *parameter)
{
  if      (strcmp(parameter, "Resolution"))
    return STP_PARAMETER_CLASS_FEATURE;
  else if (strcmp(parameter, "PageSize"))
    return STP_PARAMETER_CLASS_PAGE_SIZE;
  else if (strcmp(parameter, "MediaType"))
    return STP_PARAMETER_CLASS_FEATURE;
  else if (strcmp(parameter, "InputSlot"))
    return STP_PARAMETER_CLASS_FEATURE;
  else if (strcmp(parameter, "InkType"))
    return STP_PARAMETER_CLASS_FEATURE;
  else if (strcmp(parameter, "DitherAlgorithm"))
    return STP_PARAMETER_CLASS_OUTPUT;
  else
    return STP_PARAMETER_CLASS_INVALID;
}

static int
verify_param(const stp_printer_t p, const stp_vars_t v, const char *parameter)
{
  const char *checkval = stp_get_parameter(v, parameter);
  stp_param_list_t vptr = stp_printer_get_parameters(p, v, parameter);
  size_t count = stp_param_list_count(vptr);
  int answer = 0;
  int i;
  if (checkval == NULL)
    {
      if (count == 0)
	return 1;
      else
	return 0;
    }
  if (count > 0)
    {
      for (i = 0; i < count; i++)
	if (!strcmp(checkval, stp_param_list_param(vptr, i)->name))
	  {
	    answer = 1;
	    break;
	  }
      if (!answer)
	stp_eprintf(v, _("`%s' is not a valid %s\n"), checkval, parameter);
    }
  else if (strlen(checkval) == 0)
    answer = 1;
  else
    stp_eprintf(v, _("`%s' is not a valid %s\n"), checkval, parameter);
  stp_param_list_free(vptr);
  return answer;
}

#define CHECK_FLOAT_RANGE(v, component)					\
do									\
{									\
  const stp_vars_t max = stp_maximum_settings();			\
  const stp_vars_t min = stp_minimum_settings();			\
  if (stp_get_##component((v)) < stp_get_##component(min) ||		\
      stp_get_##component((v)) > stp_get_##component(max))		\
    {									\
      answer = 0;							\
      stp_eprintf(v, _("%s out of range (value %f, min %f, max %f)\n"),	\
		  #component, stp_get_##component(v),			\
		  stp_get_##component(min), stp_get_##component(max));	\
    }									\
} while (0)

#define CHECK_INT_RANGE(v, component)					\
do									\
{									\
  const stp_vars_t max = stp_maximum_settings();			\
  const stp_vars_t min = stp_minimum_settings();			\
  if (stp_get_##component((v)) < stp_get_##component(min) ||		\
      stp_get_##component((v)) > stp_get_##component(max))		\
    {									\
      answer = 0;							\
      stp_eprintf(v, _("%s out of range (value %d, min %d, max %d)\n"),	\
		  #component, stp_get_##component(v),			\
		  stp_get_##component(min), stp_get_##component(max));	\
    }									\
} while (0)

int
stp_verify_printer_params(const stp_printer_t p, const stp_vars_t v)
{
  const char **params;
  int nparams;
  int i;
  int answer = 1;
  int left, top, bottom, right, width, height;
  const stp_vars_t printvars = stp_printer_get_printvars(p);

  /*
   * Note that in raw CMYK mode the user is responsible for not sending
   * color output to black & white printers!
   */
  if (stp_get_output_type(printvars) == OUTPUT_GRAY &&
      (stp_get_output_type(v) == OUTPUT_COLOR ||
       stp_get_output_type(v) == OUTPUT_RAW_CMYK))
    {
      answer = 0;
      stp_eprintf(v, _("Printer does not support color output\n"));
    }
  if (strlen(stp_get_parameter(v, "PageSize")) > 0)
    {
      answer &= verify_param(p, v, "PageSize");
    }
  else
    {
      int min_height, min_width;
      stp_printer_get_size_limit(p, v, &width, &height,
				 &min_width, &min_height);
      if (stp_get_page_height(v) <= min_height ||
	  stp_get_page_height(v) > height ||
	  stp_get_page_width(v) <= min_width || stp_get_page_width(v) > width)
	{
	  answer = 0;
	  stp_eprintf(v, _("Image size is not valid\n"));
	}
    }

  stp_printer_get_imageable_area(p, v, &left, &right, &bottom, &top);

  if (stp_get_top(v) < top)
    {
      answer = 0;
      stp_eprintf(v, _("Top margin must not be less than zero\n"));
    }

  if (stp_get_left(v) < left)
    {
      answer = 0;
      stp_eprintf(v, _("Left margin must not be less than zero\n"));
    }

  if (stp_get_height(v) <= 0)
    {
      answer = 0;
      stp_eprintf(v, _("Height must be greater than zero\n"));
    }

  if (stp_get_width(v) <= 0)
    {
      answer = 0;
      stp_eprintf(v, _("Width must be greater than zero\n"));
    }

  if (stp_get_left(v) + stp_get_width(v) > right)
    {
      answer = 0;
      stp_eprintf(v, _("Image is too wide for the page\n"));
    }

  if (stp_get_top(v) + stp_get_height(v) > bottom)
    {
      answer = 0;
      stp_eprintf(v, _("Image is too long for the page\n"));
    }

  CHECK_FLOAT_RANGE(v, gamma);
  CHECK_FLOAT_RANGE(v, contrast);
  CHECK_FLOAT_RANGE(v, cyan);
  CHECK_FLOAT_RANGE(v, magenta);
  CHECK_FLOAT_RANGE(v, yellow);
  CHECK_FLOAT_RANGE(v, brightness);
  CHECK_FLOAT_RANGE(v, density);
  CHECK_FLOAT_RANGE(v, saturation);
  CHECK_INT_RANGE(v, image_type);
  CHECK_INT_RANGE(v, output_type);
  CHECK_INT_RANGE(v, input_color_model);
  CHECK_INT_RANGE(v, output_color_model);

  params = stp_printer_list_parameters(p, v, &nparams);
  for (i = 0; i < nparams; i++)
    {
      if (stp_printer_parameter_class(p, v, params[i]) ==
	  STP_PARAMETER_CLASS_PAGE_SIZE)
	continue;
      if (strlen(stp_get_parameter(v, params[i])) > 0)
	answer &= verify_param(p, v, params[i]);
    }
  stp_set_verified(v, answer);
  return answer;
}
