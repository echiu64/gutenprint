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
#include <stdlib.h>
#include "printers.h"

#define FMIN(a, b) ((a) < (b) ? (a) : (b))


static void stpi_printer_freefunc(stpi_list_item_t *item);
static const char* stpi_printer_namefunc(const stpi_list_item_t *item);
static const char* stpi_printer_long_namefunc(const stpi_list_item_t *item);


static stpi_list_t *printer_list = NULL;


int
stpi_init_printer_list(void)
{
  if(printer_list)
    stpi_list_destroy(printer_list);
  printer_list = stpi_list_create();
  stpi_list_set_freefunc(printer_list, stpi_printer_freefunc);
  stpi_list_set_namefunc(printer_list, stpi_printer_namefunc);
  stpi_list_set_long_namefunc(printer_list, stpi_printer_long_namefunc);
  /* stpi_list_set_sortfunc(printer_list, stpi_printer_sortfunc); */

  return 0;
}

int
stp_printer_model_count(void)
{
  if (printer_list == NULL)
    {
      stpi_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  return stpi_list_get_length(printer_list);
}

static void
check_printer(const stpi_internal_printer_t *p)
{
  if (p == NULL)
    {
      stpi_erprintf("Null stp_printer_t! Please report this bug.\n");
      stpi_abort();
    }
  if (p->cookie != COOKIE_PRINTER)
    {
      stpi_erprintf("Bad stp_printer_t! Please report this bug.\n");
      stpi_abort();
    }
}

const stp_printer_t
stp_get_printer_by_index(int idx)
{
  stpi_list_item_t *printer;
  if (printer_list == NULL)
    {
      stpi_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STPI_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  printer = stpi_list_get_item_by_index(printer_list, idx);
  if (printer == NULL)
    return NULL;
  return (const stp_printer_t) stpi_list_item_get_data(printer);
}

static void
stpi_printer_freefunc(stpi_list_item_t *item)
{
  stpi_internal_printer_t *printer =
    (stpi_internal_printer_t *) stpi_list_item_get_data(item);
  check_printer(printer);
  stpi_free(printer->long_name);
  stpi_free(printer->family);
  stpi_free(printer);
}

static const char *
stpi_printer_namefunc(const stpi_list_item_t *item)
{
  stp_printer_t printer = (stp_printer_t) stpi_list_item_get_data(item);
  return stp_printer_get_driver(printer);
}

static const char *
stpi_printer_long_namefunc(const stpi_list_item_t *item)
{
  stp_printer_t printer = (stp_printer_t) stpi_list_item_get_data(item);
  return stp_printer_get_long_name(printer);
}

const char *
stp_printer_get_long_name(const stp_printer_t p)
{
  const stpi_internal_printer_t *val = (const stpi_internal_printer_t *) p;
  check_printer(val);
  return val->long_name;
}

const char *
stp_printer_get_driver(const stp_printer_t p)
{
  return stp_get_driver(stp_printer_get_defaults(p));
}

const char *
stp_printer_get_family(const stp_printer_t p)
{
  const stpi_internal_printer_t *val = (const stpi_internal_printer_t *) p;
  check_printer(val);
  return val->family;
}

int
stp_printer_get_model(const stp_printer_t p)
{
  const stpi_internal_printer_t *val = (const stpi_internal_printer_t *) p;
  check_printer(val);
  return val->model;
}

static const stpi_printfuncs_t *
stpi_get_printfuncs(const stp_printer_t p)
{
  const stpi_internal_printer_t *val = (const stpi_internal_printer_t *) p;
  check_printer(val);
  return val->printfuncs;
}

const stp_vars_t
stp_printer_get_defaults(const stp_printer_t p)
{
  const stpi_internal_printer_t *val = (const stpi_internal_printer_t *) p;
  check_printer(val);
  return (stp_vars_t) val->printvars;
}



const stp_printer_t
stp_get_printer_by_long_name(const char *long_name)
{
  stpi_list_item_t *printer;
  if (printer_list == NULL)
    {
      stpi_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STPI_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  printer = stpi_list_get_item_by_long_name(printer_list, long_name);
  if (!printer)
    return NULL;
  return (const stp_printer_t) stpi_list_item_get_data(printer);
}

const stp_printer_t
stp_get_printer_by_driver(const char *driver)
{
  stpi_list_item_t *printer;
  if (printer_list == NULL)
    {
      stpi_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STPI_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  printer = stpi_list_get_item_by_name(printer_list, driver);
  if (!printer)
    return NULL;
  return (const stp_printer_t) stpi_list_item_get_data(printer);
}

int
stp_get_printer_index_by_driver(const char *driver)
{
  /* There should be no need to ever know the index! */
  int idx = 0;
  for (idx = 0; idx < stp_printer_model_count(); idx++)
    {
      const stp_printer_t val = stp_get_printer_by_index(idx);
      if (!strcmp(stp_printer_get_driver(val), driver))
	return idx;
    }
  return -1;
}

const stp_printer_t
stp_get_printer(const stp_vars_t v)
{
  return stp_get_printer_by_driver(stp_get_driver(v));
}

int
stpi_get_model_id(const stp_vars_t v)
{
  const stp_printer_t p = stp_get_printer(v);
  return stp_printer_get_model(p);
}

stp_parameter_list_t
stpi_printer_list_parameters(const stp_vars_t v)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  return (printfuncs->list_parameters)(v);
}

void
stpi_printer_describe_parameter(const stp_vars_t v, const char *name,
			       stp_parameter_t *description)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->parameters)(v, name, description);
}

void
stp_get_media_size(const stp_vars_t v, int *width, int *height)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->media_size)(v, width, height);
}

void
stp_get_imageable_area(const stp_vars_t v,
		       int *left, int *right, int *bottom, int *top)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->imageable_area)(v, left, right, bottom, top);
}

void
stp_get_size_limit(const stp_vars_t v, int *max_width, int *max_height,
		   int *min_width, int *min_height)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->limit)(v, max_width, max_height, min_width,min_height);
}

void
stp_describe_resolution(const stp_vars_t v, int *x, int *y)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->describe_resolution)(v, x, y);
}

int
stp_verify(const stp_vars_t v)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  return (printfuncs->verify)(v);
}

int
stp_print(const stp_vars_t v, stp_image_t *image)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  return (printfuncs->print)(v, image);
}

int
stp_start_job(const stp_vars_t v, stp_image_t *image)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  if (!stpi_get_verified(v))
    return 0;
  if (stp_get_job_mode(v) == STP_JOB_MODE_PAGE)
    return 1;
  if (printfuncs->start_job)
    return (printfuncs->start_job)(v, image);
  else
    return 1;
}

int
stp_end_job(const stp_vars_t v, stp_image_t *image)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  if (!stpi_get_verified(v))
    return 0;
  if (stp_get_job_mode(v) == STP_JOB_MODE_JOB)
    return 1;
  if (printfuncs->end_job)
    return (printfuncs->end_job)(v, image);
  else
    return 1;
}

static int
verify_string_param(const stp_vars_t v, const char *parameter,
		    stp_parameter_t *desc)
{
  int answer = 1;
  if (desc->is_mandatory ||
      stp_check_string_parameter(v, parameter, STP_PARAMETER_ACTIVE))
    {
      const char *checkval = stp_get_string_parameter(v, parameter);
      stp_string_list_t vptr = desc->bounds.str;
      size_t count = stp_string_list_count(vptr);
      int i;
      answer = 0;
      if (checkval == NULL)
	{
	  if (count == 0)
	    answer = 1;
	  else
	    {
	      stpi_eprintf(v, _("Value must be set for %s\n"), parameter);
	      answer = 0;
	    }
	}
      else if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(checkval, stp_string_list_param(vptr, i)->name))
	      {
		answer = 1;
		break;
	      }
	  if (!answer)
	    stpi_eprintf(v, _("`%s' is not a valid %s\n"), checkval, parameter);
	}
      else if (strlen(checkval) == 0)
	answer = 1;
      else
	stpi_eprintf(v, _("`%s' is not a valid %s\n"), checkval, parameter);
    }
  stp_parameter_description_free(desc);
  return answer;
}

static int
verify_double_param(const stp_vars_t v, const char *parameter,
		    stp_parameter_t *desc)
{
  if (desc->is_mandatory ||
      stp_check_float_parameter(v, parameter, STP_PARAMETER_ACTIVE))
    {
      double checkval = stp_get_float_parameter(v, parameter);
      if (checkval < desc->bounds.dbl.lower ||
	  checkval > desc->bounds.dbl.upper)
	{
	  stpi_eprintf(v, _("%s must be between %f and %f\n"),
		      parameter, desc->bounds.dbl.lower,
		      desc->bounds.dbl.upper);
	  return 0;
	}
    }
  return 1;
}

static int
verify_int_param(const stp_vars_t v, const char *parameter,
		 stp_parameter_t *desc)
{
  if (desc->is_mandatory ||
      stp_check_int_parameter(v, parameter, STP_PARAMETER_ACTIVE))
    {
      int checkval = stp_get_int_parameter(v, parameter);
      if (checkval < desc->bounds.integer.lower ||
	  checkval > desc->bounds.integer.upper)
	{
	  stpi_eprintf(v, _("%s must be between %d and %d\n"),
		      parameter, desc->bounds.integer.lower,
		      desc->bounds.integer.upper);
	  stp_parameter_description_free(desc);
	  return 0;
	}
    }
  stp_parameter_description_free(desc);
  return 1;
}

static int
verify_curve_param(const stp_vars_t v, const char *parameter,
		    stp_parameter_t *desc)
{
  int answer = 1;
  if (desc->bounds.curve &&
      (desc->is_mandatory ||
       stp_check_curve_parameter(v, parameter, STP_PARAMETER_ACTIVE)))
    {
      stp_curve_t checkval = stp_get_curve_parameter(v, parameter);
      if (checkval)
	{
	  double u0, l0;
	  double u1, l1;
	  stp_curve_get_bounds(checkval, &l0, &u0);
	  stp_curve_get_bounds(desc->bounds.curve, &l1, &u1);
	  if (u0 > u1 || l0 < l1)
	    {
	      stpi_eprintf(v, _("%s bounds must be between %f and %f\n"),
			  parameter, l1, u1);
	      answer = 0;
	    }
	  if (stp_curve_get_wrap(checkval) !=
	      stp_curve_get_wrap(desc->bounds.curve))
	    {
	      stpi_eprintf(v, _("%s wrap mode must be %s\n"),
			  parameter,
			  (stp_curve_get_wrap(desc->bounds.curve) ==
			   STP_CURVE_WRAP_NONE) ?
			  _("no wrap") : _("wrap around"));
	      answer = 0;
	    }
	}
    }
  stp_parameter_description_free(desc);
  return answer;
}

static int
verify_param(const stp_vars_t v, const char *parameter)
{
  stp_parameter_t desc;
  stp_describe_parameter(v, parameter, &desc);
  switch (desc.p_type)
    {
    case STP_PARAMETER_TYPE_STRING_LIST:
      return verify_string_param(v, parameter, &desc);
    case STP_PARAMETER_TYPE_DOUBLE:
      return verify_double_param(v, parameter, &desc);
    case STP_PARAMETER_TYPE_INT:
      return verify_int_param(v, parameter, &desc);
    case STP_PARAMETER_TYPE_CURVE:
      return verify_curve_param(v, parameter, &desc);
    case STP_PARAMETER_TYPE_RAW:
    case STP_PARAMETER_TYPE_FILE:
      stp_parameter_description_free(&desc);
      return 1;			/* No way to verify this here */
    case STP_PARAMETER_TYPE_BOOLEAN:
      stp_parameter_description_free(&desc);
      return 1;			/* Booleans always OK */
    default:
      stpi_eprintf(v, _("Unknown type parameter %s (%d)\n"),
		  parameter, desc.p_type);
      stp_parameter_description_free(&desc);
      return 0;
    }
}

#define CHECK_INT_RANGE(v, component, min, max)				    \
do									    \
{									    \
  if (stp_get_##component((v)) < (min) || stp_get_##component((v)) > (max)) \
    {									    \
      answer = 0;							    \
      stpi_eprintf(v, _("%s out of range (value %d, min %d, max %d)\n"),    \
		  #component, stp_get_##component(v), min, max);	    \
    }									    \
} while (0)

#define CHECK_INT_RANGE_INTERNAL(v, component, min, max)		      \
do									      \
{									      \
  if (stpi_get_##component((v)) < (min) || stpi_get_##component((v)) > (max)) \
    {									      \
      answer = 0;							      \
      stpi_eprintf(v, _("%s out of range (value %d, min %d, max %d)\n"),      \
		  #component, stpi_get_##component(v), min, max);	      \
    }									      \
} while (0)

typedef struct
{
  char *data;
  size_t bytes;
} errbuf_t;

static void
fill_buffer_writefunc(void *priv, const char *buffer, size_t bytes)
{
  errbuf_t *errbuf = (errbuf_t *) priv;
  if (errbuf->bytes == 0)
    errbuf->data = stpi_malloc(bytes + 1);
  else
    errbuf->data = stpi_realloc(errbuf->data, errbuf->bytes + bytes + 1);
  memcpy(errbuf->data + errbuf->bytes, buffer, bytes);
  errbuf->bytes += bytes;
  errbuf->data[errbuf->bytes] = '\0';
}

int
stpi_verify_printer_params(const stp_vars_t v)
{
  errbuf_t errbuf;
  stp_outfunc_t ofunc = stp_get_errfunc(v);
  void *odata = stp_get_errdata(v);

  stp_parameter_list_t params;
  const stp_printer_t p = stp_get_printer(v);
  int nparams;
  int i;
  int answer = 1;
  int left, top, bottom, right;
  const stp_vars_t printvars = stp_printer_get_defaults(p);
  const char *pagesize = stp_get_string_parameter(v, "PageSize");

  stp_set_errfunc((stp_vars_t) v, fill_buffer_writefunc);
  stp_set_errdata((stp_vars_t) v, &errbuf);

  errbuf.data = NULL;
  errbuf.bytes = 0;

  /*
   * Note that in raw CMYK mode the user is responsible for not sending
   * color output to black & white printers!
   */
  if (stp_get_output_type(printvars) == OUTPUT_GRAY &&
      stp_get_output_type(v) == OUTPUT_COLOR)
    {
      answer = 0;
      stpi_eprintf(v, _("Printer does not support color output\n"));
    }
  if (pagesize && strlen(pagesize) > 0)
    {
      answer &= verify_param(v, "PageSize");
    }
  else
    {
      int width, height, min_height, min_width;
      stp_get_size_limit(v, &width, &height, &min_width, &min_height);
      if (stp_get_page_height(v) <= min_height ||
	  stp_get_page_height(v) > height ||
	  stp_get_page_width(v) <= min_width || stp_get_page_width(v) > width)
	{
	  answer = 0;
	  stpi_eprintf(v, _("Image size is not valid\n"));
	}
    }

  stp_get_imageable_area(v, &left, &right, &bottom, &top);

  if (stp_get_top(v) < top)
    {
      answer = 0;
      stpi_eprintf(v, _("Top margin must not be less than zero\n"));
    }

  if (stp_get_left(v) < left)
    {
      answer = 0;
      stpi_eprintf(v, _("Left margin must not be less than zero\n"));
    }

  if (stp_get_height(v) <= 0)
    {
      answer = 0;
      stpi_eprintf(v, _("Height must be greater than zero\n"));
    }

  if (stp_get_width(v) <= 0)
    {
      answer = 0;
      stpi_eprintf(v, _("Width must be greater than zero\n"));
    }

  if (stp_get_left(v) + stp_get_width(v) > right)
    {
      answer = 0;
      stpi_eprintf(v, _("Image is too wide for the page\n"));
    }

  if (stp_get_top(v) + stp_get_height(v) > bottom)
    {
      answer = 0;
      stpi_eprintf(v, _("Image is too long for the page\n"));
    }

  CHECK_INT_RANGE(v, output_type, 0, OUTPUT_RAW_PRINTER);
  CHECK_INT_RANGE(v, input_color_model, 0, NCOLOR_MODELS - 1);
  CHECK_INT_RANGE_INTERNAL(v, output_color_model, 0, NCOLOR_MODELS - 1);
  CHECK_INT_RANGE(v, page_number, 0, INT_MAX);
  CHECK_INT_RANGE(v, job_mode, STP_JOB_MODE_PAGE, STP_JOB_MODE_JOB);

  params = stp_get_parameter_list(v);
  nparams = stp_parameter_list_count(params);
  for (i = 0; i < nparams; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      if (p->p_class != STP_PARAMETER_CLASS_PAGE_SIZE && p->is_active)
	answer &= verify_param(v, p->name);
    }
  stp_parameter_list_free(params);
  stp_set_errfunc((stp_vars_t) v, ofunc);
  stp_set_errdata((stp_vars_t) v, odata);
  stpi_set_verified(v, answer);
  if (errbuf.bytes > 0)
    {
      stpi_eprintf(v, "%s", errbuf.data);
      stpi_free(errbuf.data);
    }
  return answer;
}


int
stpi_family_register(stpi_list_t *family)
{
  stpi_list_item_t *printer_item;
  stpi_internal_printer_t *printer;

  if (printer_list == NULL)
    {
      stpi_init_printer_list();
      if (stpi_debug_level & STPI_DBG_PRINTERS)
	stpi_erprintf
	  ("stpi_family_register(): initialising printer_list...\n");
    }

  if (family)
    {
      printer_item = stpi_list_get_start(family);

      while(printer_item)
	{
	  printer = (stpi_internal_printer_t *)
	    stpi_list_item_get_data(printer_item);
	  check_printer(printer);
	  if (!stpi_list_get_item_by_name(printer_list,
					 stp_get_driver(printer->printvars)))
	    stpi_list_item_create(printer_list, NULL, printer);
	  printer_item = stpi_list_item_next(printer_item);
	}
    }

  return 0;
}

int
stpi_family_unregister(stpi_list_t *family)
{
  stpi_list_item_t *printer_item;
  stpi_list_item_t *old_printer_item;
  stpi_internal_printer_t *printer;

  if (printer_list == NULL)
    {
      stpi_init_printer_list();
      if (stpi_debug_level & STPI_DBG_PRINTERS)
	stpi_erprintf
	  ("stpi_family_unregister(): initialising printer_list...\n");
    }

  if (family)
    {
      printer_item = stpi_list_get_start(family);

      while(printer_item)
	{
	  printer = (stpi_internal_printer_t *)
	    stpi_list_item_get_data(printer_item);
	  check_printer(printer);
	  old_printer_item =
	    stpi_list_get_item_by_name(printer_list,
				      stp_get_driver(printer->printvars));

	  if (old_printer_item)
	    stpi_list_item_destroy(printer_list, old_printer_item);
	  printer_item = stpi_list_item_next(printer_item);
	}
    }
  return 0;
}
