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
#include "xml.h"
#include "module.h"

#define FMIN(a, b) ((a) < (b) ? (a) : (b))


static void stpi_printer_freefunc(void *item);
static const char* stpi_printer_namefunc(const void *item);
static const char* stpi_printer_long_namefunc(const void *item);

static stpi_list_t *printer_list = NULL;

#define COOKIE_PRINTER    0x0722922c

typedef struct stpi_internal_printer
{
  int        cookie;            /* Magic number */
  const char *driver;
  char       *long_name;        /* Long name for UI */
  char       *family;           /* Printer family */
  char	     *manufacturer;	/* Printer manufacturer */
  int        model;             /* Model number */
  const stpi_printfuncs_t *printfuncs;
  stp_vars_t printvars;
} stpi_internal_printer_t;

static int
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
null_printer(void)
{
  stpi_erprintf("Null stp_printer_t! Please report this bug.\n");
  stpi_abort();
}

static void
bad_printer(void)
{
  stpi_erprintf("Bad stp_printer_t! Please report this bug.\n");
  stpi_abort();
}

static inline void
check_printer(const stpi_internal_printer_t *p)
{
  if (p == NULL)
    null_printer();
  if (p->cookie != COOKIE_PRINTER)
    bad_printer();
}

static inline stpi_internal_printer_t *
get_printer(stp_printer_t printer)
{
  stpi_internal_printer_t *val = (stpi_internal_printer_t *) printer;
  check_printer(val);
  return val;
}

static inline const stpi_internal_printer_t *
get_const_printer(stp_const_printer_t printer)
{
  const stpi_internal_printer_t *val = (const stpi_internal_printer_t *) printer;
  check_printer(val);
  return val;
}

stp_const_printer_t
stp_get_printer_by_index(int idx)
{
  stpi_list_item_t *printer;
  if (printer_list == NULL)
    {
      stpi_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  printer = stpi_list_get_item_by_index(printer_list, idx);
  if (printer == NULL)
    return NULL;
  return (stp_const_printer_t) stpi_list_item_get_data(printer);
}

static void
stpi_printer_freefunc(void *item)
{
  stpi_internal_printer_t *printer = get_printer(item);
  stpi_free(printer->long_name);
  stpi_free(printer->family);
  stpi_free(printer);
}

const char *
stp_printer_get_driver(stp_const_printer_t p)
{
  const stpi_internal_printer_t *val = get_const_printer(p);
  return val->driver;
}

static const char *
stpi_printer_namefunc(const void *item)
{
  const stpi_internal_printer_t *val = get_const_printer(item);
  return val->driver;
}

const char *
stp_printer_get_long_name(stp_const_printer_t p)
{
  const stpi_internal_printer_t *val = get_const_printer(p);
  return val->long_name;
}

static const char *
stpi_printer_long_namefunc(const void *item)
{
  const stpi_internal_printer_t *val = get_const_printer(item);
  return val->long_name;
}

const char *
stp_printer_get_family(stp_const_printer_t p)
{
  const stpi_internal_printer_t *val = get_const_printer(p);
  return val->family;
}

const char *
stp_printer_get_manufacturer(stp_const_printer_t p)
{
  const stpi_internal_printer_t *val = get_const_printer(p);
  return val->manufacturer;
}

int
stp_printer_get_model(stp_const_printer_t p)
{
  const stpi_internal_printer_t *val = get_const_printer(p);
  return val->model;
}

static inline const stpi_printfuncs_t *
stpi_get_printfuncs(stp_const_printer_t p)
{
  const stpi_internal_printer_t *val = get_const_printer(p);
  return val->printfuncs;
}

stp_const_vars_t
stp_printer_get_defaults(stp_const_printer_t p)
{
  const stpi_internal_printer_t *val = get_const_printer(p);
  return (stp_vars_t) val->printvars;
}



stp_const_printer_t
stp_get_printer_by_long_name(const char *long_name)
{
  stpi_list_item_t *printer;
  if (printer_list == NULL)
    {
      stpi_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  printer = stpi_list_get_item_by_long_name(printer_list, long_name);
  if (!printer)
    return NULL;
  return (stp_const_printer_t) stpi_list_item_get_data(printer);
}

stp_const_printer_t
stp_get_printer_by_driver(const char *driver)
{
  stpi_list_item_t *printer;
  if (printer_list == NULL)
    {
      stpi_erprintf("No printer drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_printer_list();
    }
  printer = stpi_list_get_item_by_name(printer_list, driver);
  if (!printer)
    return NULL;
  return (stp_const_printer_t) stpi_list_item_get_data(printer);
}

int
stp_get_printer_index_by_driver(const char *driver)
{
  /* There should be no need to ever know the index! */
  int idx = 0;
  for (idx = 0; idx < stp_printer_model_count(); idx++)
    {
      stp_const_printer_t val = stp_get_printer_by_index(idx);
      if (!strcmp(stp_printer_get_driver(val), driver))
	return idx;
    }
  return -1;
}

stp_const_printer_t
stp_get_printer(stp_const_vars_t v)
{
  return stp_get_printer_by_driver(stp_get_driver(v));
}

int
stpi_get_model_id(stp_const_vars_t v)
{
  stp_const_printer_t p = stp_get_printer_by_driver(stp_get_driver(v));
  const stpi_internal_printer_t *val = get_const_printer(p);
  return val->model;
}

stp_parameter_list_t
stpi_printer_list_parameters(stp_const_vars_t v)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  return (printfuncs->list_parameters)(v);
}

void
stpi_printer_describe_parameter(stp_const_vars_t v, const char *name,
			       stp_parameter_t *description)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->parameters)(v, name, description);
}

static void
set_printer_defaults(stp_vars_t v, int core_only)
{
  stp_parameter_list_t *params;
  int count;
  int i;
  stp_parameter_t desc;
  params = stp_get_parameter_list(v);
  count = stp_parameter_list_count(params);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      if (p->is_mandatory &&
	  (!core_only || p->p_class == STP_PARAMETER_CLASS_CORE))
	{
	  stp_describe_parameter(v, p->name, &desc);
	  switch (p->p_type)
	    {
	    case STP_PARAMETER_TYPE_STRING_LIST:
	      stp_set_string_parameter(v, p->name, desc.deflt.str);
	      stp_set_string_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
	      break;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      stp_set_float_parameter(v, p->name, desc.deflt.dbl);
	      stp_set_float_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
	      break;
	    case STP_PARAMETER_TYPE_INT:
	      stp_set_int_parameter(v, p->name, desc.deflt.integer);
	      stp_set_int_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
	      break;
	    case STP_PARAMETER_TYPE_BOOLEAN:
	      stp_set_boolean_parameter(v, p->name, desc.deflt.boolean);
	      stp_set_boolean_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
	      break;
	    case STP_PARAMETER_TYPE_CURVE:
	      stp_set_curve_parameter(v, p->name, desc.deflt.curve);
	      stp_set_curve_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
	      break;
	    case STP_PARAMETER_TYPE_ARRAY:
	      stp_set_array_parameter(v, p->name, desc.deflt.array);
	      stp_set_array_parameter_active(v, p->name, STP_PARAMETER_ACTIVE);
	      break;
	    default:
	      break;
	    }
	  stp_parameter_description_free(&desc);
	}
    }
  stp_parameter_list_free(params);
}

void
stp_set_printer_defaults(stp_vars_t v, stp_const_printer_t printer)
{
  stp_set_driver(v, stp_printer_get_driver(printer));
  set_printer_defaults(v, 0);
}

void
stpi_initialize_printer_defaults(void)
{
  stpi_list_item_t *printer_item;
  if (printer_list == NULL)
    {
      stpi_init_printer_list();
      stpi_deprintf
	(STPI_DBG_PRINTERS,
	 "stpi_family_register(): initialising printer_list...\n");
    }
  printer_item = stpi_list_get_start(printer_list);
  while (printer_item)
    {
      set_printer_defaults
	(get_printer(stpi_list_item_get_data(printer_item))->printvars, 1);
      printer_item = stpi_list_item_next(printer_item);
    }
}

void
stp_get_media_size(stp_const_vars_t v, int *width, int *height)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->media_size)(v, width, height);
}

void
stp_get_imageable_area(stp_const_vars_t v,
		       int *left, int *right, int *bottom, int *top)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->imageable_area)(v, left, right, bottom, top);
}

void
stp_get_size_limit(stp_const_vars_t v, int *max_width, int *max_height,
		   int *min_width, int *min_height)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->limit)(v, max_width, max_height, min_width,min_height);
}

void
stp_describe_resolution(stp_const_vars_t v, int *x, int *y)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  (printfuncs->describe_resolution)(v, x, y);
}

const char *
stpi_describe_output(stp_const_vars_t v)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  return (printfuncs->describe_output)(v);
}

int
stp_verify(stp_vars_t v)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  stp_vars_t nv = stp_vars_create_copy(v);
  int status;
  stpi_prune_inactive_options(nv);
  status = (printfuncs->verify)(nv);
  stpi_set_verified(v, stpi_get_verified(nv));
  stp_vars_free(nv);
  return status;
}

int
stp_print(stp_const_vars_t v, stp_image_t *image)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  return (printfuncs->print)(v, image);
}

int
stp_start_job(stp_const_vars_t v, stp_image_t *image)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  if (!stp_get_string_parameter(v, "JobMode") ||
      strcmp(stp_get_string_parameter(v, "JobMode"), "Page") == 0)
    return 1;
  if (printfuncs->start_job)
    return (printfuncs->start_job)(v, image);
  else
    return 1;
}

int
stp_end_job(stp_const_vars_t v, stp_image_t *image)
{
  const stpi_printfuncs_t *printfuncs =
    stpi_get_printfuncs(stp_get_printer(v));
  if (!stp_get_string_parameter(v, "JobMode") ||
      strcmp(stp_get_string_parameter(v, "JobMode"), "Page") == 0)
    return 1;
  if (printfuncs->end_job)
    return (printfuncs->end_job)(v, image);
  else
    return 1;
}

static int
verify_string_param(stp_const_vars_t v, const char *parameter,
		    stp_parameter_t *desc, int quiet)
{
  stpi_parameter_verify_t answer = PARAMETER_OK;
  stpi_dprintf(STPI_DBG_VARS, v, "    Verifying string %s\n", parameter);
  if (desc->is_mandatory ||
      stp_check_string_parameter(v, parameter, STP_PARAMETER_ACTIVE))
    {
      const char *checkval = stp_get_string_parameter(v, parameter);
      stp_string_list_t vptr = desc->bounds.str;
      size_t count = 0;
      int i;
      stpi_dprintf(STPI_DBG_VARS, v, "     value %s\n",
		   checkval ? checkval : "(null)");
      if (vptr)
	count = stp_string_list_count(vptr);
      answer = PARAMETER_BAD;
      if (checkval == NULL)
	{
	  if (count == 0)
	    answer = PARAMETER_OK;
	  else
	    {
	      if (!quiet)
		stpi_eprintf(v, _("Value must be set for %s\n"), parameter);
	      answer = PARAMETER_BAD;
	    }
	}
      else if (count > 0)
	{
	  for (i = 0; i < count; i++)
	    if (!strcmp(checkval, stp_string_list_param(vptr, i)->name))
	      {
		answer = PARAMETER_OK;
		break;
	      }
	  if (!answer && !quiet)
	    stpi_eprintf(v, _("`%s' is not a valid %s\n"), checkval, parameter);
	}
      else if (strlen(checkval) == 0)
	answer = PARAMETER_OK;
      else if (!quiet)
	stpi_eprintf(v, _("`%s' is not a valid %s\n"), checkval, parameter);
    }
  stp_parameter_description_free(desc);
  return answer;
}

static int
verify_double_param(stp_const_vars_t v, const char *parameter,
		    stp_parameter_t *desc, int quiet)
{
  stpi_dprintf(STPI_DBG_VARS, v, "    Verifying double %s\n", parameter);
  if (desc->is_mandatory ||
      stp_check_float_parameter(v, parameter, STP_PARAMETER_ACTIVE))
    {
      double checkval = stp_get_float_parameter(v, parameter);
      if (checkval < desc->bounds.dbl.lower ||
	  checkval > desc->bounds.dbl.upper)
	{
	  if (!quiet)
	    stpi_eprintf(v, _("%s must be between %f and %f (is %f)\n"),
			 parameter, desc->bounds.dbl.lower,
			 desc->bounds.dbl.upper, checkval);
	  return PARAMETER_BAD;
	}
    }
  return PARAMETER_OK;
}

static int
verify_int_param(stp_const_vars_t v, const char *parameter,
		 stp_parameter_t *desc, int quiet)
{
  stpi_dprintf(STPI_DBG_VARS, v, "    Verifying int %s\n", parameter);
  if (desc->is_mandatory ||
      stp_check_int_parameter(v, parameter, STP_PARAMETER_ACTIVE))
    {
      int checkval = stp_get_int_parameter(v, parameter);
      if (checkval < desc->bounds.integer.lower ||
	  checkval > desc->bounds.integer.upper)
	{
	  if (!quiet)
	    stpi_eprintf(v, _("%s must be between %d and %d (is %d)\n"),
			 parameter, desc->bounds.integer.lower,
			 desc->bounds.integer.upper, checkval);
	  stp_parameter_description_free(desc);
	  return PARAMETER_BAD;
	}
    }
  stp_parameter_description_free(desc);
  return PARAMETER_OK;
}

static int
verify_curve_param(stp_const_vars_t v, const char *parameter,
		    stp_parameter_t *desc, int quiet)
{
  stpi_parameter_verify_t answer = 1;
  stpi_dprintf(STPI_DBG_VARS, v, "    Verifying curve %s\n", parameter);
  if (desc->bounds.curve &&
      (desc->is_mandatory ||
       stp_check_curve_parameter(v, parameter, STP_PARAMETER_ACTIVE)))
    {
      stp_const_curve_t checkval = stp_get_curve_parameter(v, parameter);
      if (checkval)
	{
	  double u0, l0;
	  double u1, l1;
	  stp_curve_get_bounds(checkval, &l0, &u0);
	  stp_curve_get_bounds(desc->bounds.curve, &l1, &u1);
	  if (u0 > u1 || l0 < l1)
	    {
	      if (!quiet)
		stpi_eprintf(v, _("%s bounds must be between %f and %f\n"),
			     parameter, l1, u1);
	      answer = PARAMETER_BAD;
	    }
	  if (stp_curve_get_wrap(checkval) !=
	      stp_curve_get_wrap(desc->bounds.curve))
	    {
	      if (!quiet)
		stpi_eprintf(v, _("%s wrap mode must be %s\n"),
			     parameter,
			     (stp_curve_get_wrap(desc->bounds.curve) ==
			      STP_CURVE_WRAP_NONE) ?
			     _("no wrap") : _("wrap around"));
	      answer = PARAMETER_BAD;
	    }
	}
    }
  stp_parameter_description_free(desc);
  return answer;
}

stpi_parameter_verify_t
stpi_verify_parameter(stp_const_vars_t v, const char *parameter,
		      int quiet)
{
  stp_parameter_t desc;
  quiet = 0;
  stpi_dprintf(STPI_DBG_VARS, v, "  Verifying %s\n", parameter);
  stp_describe_parameter(v, parameter, &desc);
  if (!desc.is_active)
    {
      stp_parameter_description_free(&desc);
      return PARAMETER_INACTIVE;
    }
  switch (desc.p_type)
    {
    case STP_PARAMETER_TYPE_STRING_LIST:
      return verify_string_param(v, parameter, &desc, quiet);
    case STP_PARAMETER_TYPE_DOUBLE:
      return verify_double_param(v, parameter, &desc, quiet);
    case STP_PARAMETER_TYPE_INT:
      return verify_int_param(v, parameter, &desc, quiet);
    case STP_PARAMETER_TYPE_CURVE:
      return verify_curve_param(v, parameter, &desc, quiet);
    case STP_PARAMETER_TYPE_RAW:
    case STP_PARAMETER_TYPE_FILE:
      stp_parameter_description_free(&desc);
      return PARAMETER_OK;		/* No way to verify this here */
    case STP_PARAMETER_TYPE_BOOLEAN:
      stp_parameter_description_free(&desc);
      return PARAMETER_OK;		/* Booleans always OK */
    default:
      if (!quiet)
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
stpi_verify_printer_params(stp_vars_t v)
{
  errbuf_t errbuf;
  stp_outfunc_t ofunc = stp_get_errfunc(v);
  void *odata = stp_get_errdata(v);

  stp_parameter_list_t params;
  int nparams;
  int i;
  int answer = 1;
  int left, top, bottom, right;
  const char *pagesize = stp_get_string_parameter(v, "PageSize");

  stp_set_errfunc((stp_vars_t) v, fill_buffer_writefunc);
  stp_set_errdata((stp_vars_t) v, &errbuf);

  errbuf.data = NULL;
  errbuf.bytes = 0;

  if (pagesize && strlen(pagesize) > 0)
    {
      if (stpi_verify_parameter(v, "PageSize", 0) == 0)
	answer = 0;
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
	  stpi_eprintf(v, _("Page size is not valid\n"));
	}
      stpi_dprintf(STPI_DBG_PAPER, v,
		   "page size max %d %d min %d %d actual %d %d\n",
		   width, height, min_width, min_height,
		   stp_get_page_width(v), stp_get_page_height(v));
    }

  stp_get_imageable_area(v, &left, &right, &bottom, &top);

  stpi_dprintf(STPI_DBG_PAPER, v,
	       "page      left %d top %d right %d bottom %d\n",
	       left, top, right, bottom);
  stpi_dprintf(STPI_DBG_PAPER, v,
	       "requested left %d top %d width %d height %d\n",
	       stp_get_left(v), stp_get_top(v),
	       stp_get_width(v), stp_get_height(v));

  if (stp_get_top(v) < top)
    {
      answer = 0;
      stpi_eprintf(v, _("Top margin must not be less than %d\n"), top);
    }

  if (stp_get_left(v) < left)
    {
      answer = 0;
      stpi_eprintf(v, _("Left margin must not be less than %d\n"), left);
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
      stpi_eprintf(v, _("Image is too wide for the page: left margin is %d, width %d, right edge is %d\n"),
		   stp_get_left(v), stp_get_width(v), right);
    }

  if (stp_get_top(v) + stp_get_height(v) > bottom)
    {
      answer = 0;
      stpi_eprintf(v, _("Image is too long for the page: top margin is %d, height %d, bottom edge is %d\n"),
		   stp_get_left(v), stp_get_width(v), right);
    }

  CHECK_INT_RANGE(v, page_number, 0, INT_MAX);

  params = stp_get_parameter_list(v);
  nparams = stp_parameter_list_count(params);
  for (i = 0; i < nparams; i++)
    {
      const stp_parameter_t *param = stp_parameter_list_param(params, i);
      stpi_dprintf(STPI_DBG_VARS, v, "Checking %s %d %d\n", param->name,
		   param->is_active, param->verify_this_parameter);

      if (strcmp(param->name, "PageSize") != 0 &&
	  param->is_active && param->verify_this_parameter &&
	  stpi_verify_parameter(v, param->name, 0) == 0)
	answer = 0;
    }
  stp_parameter_list_free(params);
  stp_set_errfunc((stp_vars_t) v, ofunc);
  stp_set_errdata((stp_vars_t) v, odata);
  stpi_set_verified((stp_vars_t) v, answer);
  if (errbuf.bytes > 0)
    {
      stpi_eprintf(v, "%s", errbuf.data);
      stpi_free(errbuf.data);
    }
  return answer;
}


typedef struct
{
  const char *property;
  const char *parameter;
} stpi_xml_prop_t;

static const stpi_xml_prop_t stpi_xml_props[] =
{
  { "black", "BlackGamma" },
  { "cyan", "CyanGamma" },
  { "yellow", "YellowGamma" },
  { "magenta", "MagentaGamma" },
  { "brightness", "Brightness" },
  { "gamma", "Gamma" },
  { "density", "Density" },
  { "saturation", "Saturation" },
  { "blackdensity", "BlackDensity" },
  { "cyandensity", "CyanDensity" },
  { "yellowdensity", "YellowDensity" },
  { "magentadensity", "MagentaDensity" },
  { "gcrlower", "GCRLower" },
  { "gcrupper", "GCRupper" },
  { NULL, NULL }
};

int
stpi_family_register(stpi_list_t *family)
{
  stpi_list_item_t *printer_item;
  const stpi_internal_printer_t *printer;

  if (printer_list == NULL)
    {
      stpi_init_printer_list();
      stpi_deprintf
	(STPI_DBG_PRINTERS,
	 "stpi_family_register(): initialising printer_list...\n");
    }

  if (family)
    {
      printer_item = stpi_list_get_start(family);

      while(printer_item)
	{
	  printer = get_printer(stpi_list_item_get_data(printer_item));
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
  const stpi_internal_printer_t *printer;

  if (printer_list == NULL)
    {
      stpi_init_printer_list();
      stpi_deprintf
	(STPI_DBG_PRINTERS,
	 "stpi_family_unregister(): initialising printer_list...\n");
    }

  if (family)
    {
      printer_item = stpi_list_get_start(family);

      while(printer_item)
	{
	  printer = get_printer(stpi_list_item_get_data(printer_item));
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

/*
 * Parse the printer node, and return the generated printer.  Returns
 * NULL on failure.
 */
static stpi_internal_printer_t*
stp_printer_create_from_xmltree(mxml_node_t *printer, /* The printer node */
				const char *family,    /* Family name */
				const stpi_printfuncs_t *printfuncs)
                                                    /* Family printfuncs */
{
  mxml_node_t *prop;                                  /* Temporary node pointer */
  const char *stmp;                                    /* Temporary string */
 /* props[] (unused) is the correct tag sequence */
  /*  const char *props[] =
    {
      "model",
      "black",
      "cyan",
      "yellow",
      "magenta",
      "brightness",
      "gamma",
      "density",
      "saturation",
      "blackgamma",
      "cyangamma",
      "yellowgamma",
      "magentagamma",
      "gcrlower",
      "gcrupper",
      NULL
      };*/
  stpi_internal_printer_t *outprinter;                 /* Generated printer */
  int
    driver = 0,                                       /* Check driver */
    long_name = 0,                                    /* Check long_name */
    model = 0;                                        /* Check model */

  outprinter = stpi_zalloc(sizeof(stpi_internal_printer_t));
  if (!outprinter)
    return NULL;
  outprinter->printvars = stp_vars_create();
  if (outprinter->printvars == NULL)
    {
      stpi_free(outprinter);
      return NULL;
    }

  outprinter->cookie = COOKIE_PRINTER;

  stmp = stpi_mxmlElementGetAttr(printer, "driver");
  stp_set_driver(outprinter->printvars, (const char *) stmp);

  outprinter->long_name = stpi_strdup(stpi_mxmlElementGetAttr(printer, "name"));
  outprinter->manufacturer = stpi_strdup(stpi_mxmlElementGetAttr(printer, "manufacturer"));
  outprinter->family = stpi_strdup((const char *) family);

  if (stp_get_driver(outprinter->printvars))
    driver = 1;
  if (outprinter->long_name)
    long_name = 1;

  outprinter->printfuncs = printfuncs;

  prop = printer->child;
  while (prop)
    {
      if (prop->type == MXML_ELEMENT)
	{
	  const char *prop_name = prop->value.element.name;
	  if (!strcmp(prop_name, "model"))
	    {
	      stmp = stpi_mxmlElementGetAttr(prop, "value");
	      if (stmp)
		{
		  outprinter->model = stpi_xmlstrtol(stmp);
		  model = 1;
		}
	    }
	  else
	    {
	      const stpi_xml_prop_t *stp_prop = stpi_xml_props;
	      while (stp_prop->property)
		{
		  if (!strcmp(prop_name, stp_prop->property))
		    {
		      stmp = stpi_mxmlElementGetAttr(prop, "value");
		      if (stmp)
			{
			  stp_set_float_parameter(outprinter->printvars,
						  stp_prop->parameter,
						  (float) stpi_xmlstrtod(stmp));
			  break;
			}
		    }
		  stp_prop++;
		}
	    }
	}
      prop = prop->next;
    }
  if (driver && long_name && model && printfuncs)
    {
      if (stpi_get_debug_level() & STPI_DBG_XML)
	{
	  stmp = stpi_mxmlElementGetAttr(printer, "driver");
	  stpi_erprintf("stp_printer_create_from_xmltree: printer: %s\n", stmp);
	}
      outprinter->driver = stp_get_driver(outprinter->printvars);
      return outprinter;
    }
  stpi_free(outprinter);
  return NULL;
}

/*
 * Parse the <family> node.
 */
static void
stpi_xml_process_family(mxml_node_t *family)     /* The family node */
{
  stpi_list_t *family_module_list = NULL;      /* List of valid families */
  stpi_list_item_t *family_module_item;        /* Current family */
  const char *family_name;                       /* Name of family */
  mxml_node_t *printer;                         /* printer child node */
  stpi_module_t *family_module_data;           /* Family module data */
  stpi_internal_family_t *family_data = NULL;  /* Family data */
  int family_valid = 0;                       /* Is family valid? */
  stpi_internal_printer_t *outprinter;         /* Generated printer */

  family_module_list = stpi_module_get_class(STPI_MODULE_CLASS_FAMILY);
  if (!family_module_list)
    return;

  family_name = stpi_mxmlElementGetAttr(family, "name");
  family_module_item = stpi_list_get_start(family_module_list);
  while (family_module_item)
    {
      family_module_data = (stpi_module_t *)
	stpi_list_item_get_data(family_module_item);
      if (!strcmp(family_name, family_module_data->name))
	{
	  stpi_deprintf(STPI_DBG_XML,
			"stpi_xml_process_family: family module: %s\n",
			family_module_data->name);
	  family_data = family_module_data->syms;
	  if (family_data->printer_list == NULL)
	    family_data->printer_list = stpi_list_create();
	  family_valid = 1;
	}
      family_module_item = stpi_list_item_next(family_module_item);
    }

  printer = family->child;
  while (family_valid && printer)
    {
      if (printer->type == MXML_ELEMENT)
	{
	  const char *printer_name = printer->value.element.name;
	  if (!strcmp(printer_name, "printer"))
	    {
	      outprinter =
		stp_printer_create_from_xmltree(printer, family_name,
						family_data->printfuncs);
	      if (outprinter)
		stpi_list_item_create(family_data->printer_list, NULL,
				      outprinter);
	    }
	}
      printer = printer->next;
    }

  stpi_list_destroy(family_module_list);
  return;
}

/*
 * Parse the <printdef> node.
 */
static int
stpi_xml_process_printdef(mxml_node_t *printdef, const char *file) /* The printdef node */
{
  mxml_node_t *family;                          /* Family child node */

  family = printdef->child;
  while (family)
    {
      if (family->type == MXML_ELEMENT)
	{
	  const char *family_name = family->value.element.name;
	  if (!strcmp(family_name, "family"))
	    {
	      stpi_xml_process_family(family);
	    }
	}
      family = family->next;
    }
  return 1;
}

void
stpi_init_printer(void)
{
  stpi_register_xml_parser("printdef", stpi_xml_process_printdef);
  stpi_register_xml_preload("printers.xml");
}
