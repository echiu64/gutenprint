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

typedef struct
{
  char *name;
  stp_parameter_type_t typ;
  stp_parameter_activity_t active;
  union
  {
    int ival;
    int bval;
    double dval;
    stp_curve_t cval;
    stp_raw_t rval;
  } value;
} value_t;

typedef struct					/* Plug-in variables */
{
  int	cookie;
  const char *driver;		/* Name of printer "driver" */
  int	output_type;		/* Color or grayscale output */
  float app_gamma;		/* Application gamma */
  int	input_color_model;	/* Color model for this device */
  int	output_color_model;	/* Color model for this device */
  stp_job_mode_t job_mode;
  int	left;			/* Offset from left-upper corner, points */
  int	top;			/* ... */
  int	width;			/* Width of the image, points */
  int	height;			/* ... */
  int	page_width;		/* Width of page in points */
  int	page_height;		/* Height of page in points */
  int	page_number;
  stpi_list_t *params[STP_PARAMETER_TYPE_INVALID];
  void  *color_data;		/* Private data of the color module */
  void	*(*copy_color_data_func)(const stp_vars_t);
  void	(*destroy_color_data_func)(stp_vars_t);
  void  *driver_data;		/* Private data of the family driver module */
  void	*(*copy_driver_data_func)(const stp_vars_t);
  void	(*destroy_driver_data_func)(stp_vars_t);
  void  *dither_data;		/* Private data of the family dither module */
  void	*(*copy_dither_data_func)(const stp_vars_t);
  void	(*destroy_dither_data_func)(stp_vars_t);
  void (*outfunc)(void *data, const char *buffer, size_t bytes);
  void *outdata;
  void (*errfunc)(void *data, const char *buffer, size_t bytes);
  void *errdata;
  int verified;			/* Ensure that params are OK! */
} stpi_internal_vars_t;

static int standard_vars_initialized = 0;

static stpi_internal_vars_t default_vars =
{
	COOKIE_VARS,
	N_ ("ps2"),	       	/* Name of printer "driver" */
	OUTPUT_COLOR,		/* Color or grayscale output */
	1.0,			/* Application gamma placeholder */
	COLOR_MODEL_RGB,	/* Input color model */
	COLOR_MODEL_RGB,	/* Output color model */
	STP_JOB_MODE_PAGE	/* Job mode */
};

static const char *
value_namefunc(const stpi_list_item_t *item)
{
  const value_t *v = (value_t *)stpi_list_item_get_data(item);
  return v->name;
}

static void
value_freefunc(stpi_list_item_t *item)
{
  value_t *v = (value_t *)stpi_list_item_get_data(item);
  switch (v->typ)
    {
    case STP_PARAMETER_TYPE_STRING_LIST:
    case STP_PARAMETER_TYPE_FILE:
    case STP_PARAMETER_TYPE_RAW:
      stpi_free(v->value.rval.data);
      break;
    case STP_PARAMETER_TYPE_CURVE:
      stp_curve_free(v->value.cval);
      break;
    case STP_PARAMETER_TYPE_ARRAY:
      stp_array_destroy(v->value.cval);
      break;
    default:
      break;
    }
  stpi_free(v->name);
  stpi_free(v);
}

static stpi_list_t *
create_vars_list(void)

{
  stpi_list_t *ret = stpi_list_create();
  stpi_list_set_freefunc(ret, value_freefunc);
  stpi_list_set_namefunc(ret, value_namefunc);
  return ret;
}

static value_t *
value_copy(const stpi_list_item_t *item)
{
  value_t *ret = stpi_malloc(sizeof(value_t));
  const value_t *v = (value_t *)stpi_list_item_get_data(item);
  ret->name = stpi_strdup(v->name);
  ret->typ = v->typ;
  ret->active = v->active;
  switch (v->typ)
    {
    case STP_PARAMETER_TYPE_CURVE:
      ret->value.cval = stp_curve_create_copy(v->value.cval);
      break;
    case STP_PARAMETER_TYPE_ARRAY:
      ret->value.cval = stp_array_create_copy(v->value.cval);
      break;
    case STP_PARAMETER_TYPE_STRING_LIST:
    case STP_PARAMETER_TYPE_FILE:
    case STP_PARAMETER_TYPE_RAW:
      ret->value.rval.bytes = v->value.rval.bytes;
      ret->value.rval.data = stpi_malloc(ret->value.rval.bytes + 1);
      memcpy(ret->value.rval.data, v->value.rval.data, v->value.rval.bytes);
      ((char *) (ret->value.rval.data))[v->value.rval.bytes] = '\0';
      break;
    case STP_PARAMETER_TYPE_INT:
    case STP_PARAMETER_TYPE_BOOLEAN:
      ret->value.ival = v->value.ival;
      break;
    case STP_PARAMETER_TYPE_DOUBLE:
      ret->value.dval = v->value.dval;
      break;
    default:
      break;
    }
  return ret;
}

static stpi_list_t *
copy_value_list(const stpi_list_t *src)
{
  stpi_list_t *ret = create_vars_list();
  stpi_list_item_t *item = stpi_list_get_start((stpi_list_t *)src);
  while (item)
    {
      stpi_list_item_create(ret, NULL, value_copy(item));
      item = stpi_list_item_next(item);
    }
  return ret;
}

static void
initialize_standard_vars(void)
{
  if (!standard_vars_initialized)
    {
      int i;
      for (i = 0; i < STP_PARAMETER_TYPE_INVALID; i++)
	default_vars.params[i] = create_vars_list();
      standard_vars_initialized = 1;
    }
}

const stp_vars_t
stp_default_settings(void)
{
  initialize_standard_vars();
  return (stp_vars_t) &default_vars;
}

stp_vars_t
stp_vars_create(void)
{
  int i;
  stpi_internal_vars_t *retval = stpi_zalloc(sizeof(stpi_internal_vars_t));
  initialize_standard_vars();
  retval->cookie = COOKIE_VARS;
  for (i = 0; i < STP_PARAMETER_TYPE_INVALID; i++)
    retval->params[i] = create_vars_list();
  stp_vars_copy(retval, (stp_vars_t)&default_vars);
  return (retval);
}

#define SAFE_FREE(x)				\
do						\
{						\
  if ((x))					\
    stpi_free((char *)(x));			\
  ((x)) = NULL;					\
} while (0)

static void
check_vars(const stpi_internal_vars_t *v)
{
  if (v == NULL)
    {
      stpi_erprintf("Null stp_vars_t! Please report this bug.\n");
      stpi_abort();
    }
  if (v->cookie != COOKIE_VARS)
    {
      stpi_erprintf("Bad stp_vars_t! Please report this bug.\n");
      stpi_abort();
    }
}

void
stp_vars_free(stp_vars_t vv)
{
  int i;
  stpi_internal_vars_t *v = (stpi_internal_vars_t *) vv;
  check_vars(v);
  if (stpi_get_destroy_color_data_func(vv))
    (*stpi_get_destroy_color_data_func(vv))(vv);
  if (stpi_get_destroy_driver_data_func(vv))
    (*stpi_get_destroy_driver_data_func(vv))(vv);
  if (stpi_get_destroy_dither_data_func(vv))
    (*stpi_get_destroy_dither_data_func(vv))(vv);
  for (i = 0; i < STP_PARAMETER_TYPE_INVALID; i++)
    stpi_list_destroy(v->params[i]);
  SAFE_FREE(v->driver);
  stpi_free(v);
}

#define DEF_STRING_FUNCS(s, i)					\
void								\
stp##i##_set_##s(stp_vars_t vv, const char *val)		\
{								\
  stpi_internal_vars_t *v = (stpi_internal_vars_t *) vv;	\
  check_vars(v);						\
  if (v->s == val)						\
    return;							\
  SAFE_FREE(v->s);						\
  v->s = stpi_strdup(val);					\
  v->verified = 0;						\
}								\
								\
void								\
stp##i##_set_##s##_n(stp_vars_t vv, const char *val, int n)	\
{								\
  stpi_internal_vars_t *v = (stpi_internal_vars_t *) vv;	\
  check_vars(v);						\
  if (v->s == val)						\
    return;							\
  SAFE_FREE(v->s);						\
  v->s = stpi_strndup(val, n);					\
  v->verified = 0;						\
}								\
								\
const char *							\
stp##i##_get_##s(const stp_vars_t vv)				\
{								\
  stpi_internal_vars_t *v = (stpi_internal_vars_t *) vv;	\
  check_vars(v);						\
  return v->s;							\
}

#define DEF_FUNCS(s, t, u, i)					\
u void								\
stp##i##_set_##s(stp_vars_t vv, t val)				\
{								\
  stpi_internal_vars_t *v = (stpi_internal_vars_t *) vv;	\
  check_vars(v);						\
  v->verified = 0;						\
  v->s = val;							\
}								\
								\
u t								\
stp##i##_get_##s(const stp_vars_t vv)				\
{								\
  stpi_internal_vars_t *v = (stpi_internal_vars_t *) vv;	\
  check_vars(v);						\
  return v->s;							\
}

#define DEF_INTERNAL_STRING_FUNCS(s) DEF_STRING_FUNCS(s, i)
#define DEF_EXTERNAL_STRING_FUNCS(s) DEF_STRING_FUNCS(s,)
#define DEF_INTERNAL_FUNCS(s, t, u) DEF_FUNCS(s, t, u, i)
#define DEF_EXTERNAL_FUNCS(s, t, u) DEF_FUNCS(s, t, u,)

DEF_EXTERNAL_STRING_FUNCS(driver)
DEF_EXTERNAL_FUNCS(output_type, int, )
DEF_EXTERNAL_FUNCS(left, int, )
DEF_EXTERNAL_FUNCS(top, int, )
DEF_EXTERNAL_FUNCS(width, int, )
DEF_EXTERNAL_FUNCS(height, int, )
DEF_EXTERNAL_FUNCS(page_width, int, )
DEF_EXTERNAL_FUNCS(page_height, int, )
DEF_EXTERNAL_FUNCS(input_color_model, int, )
DEF_EXTERNAL_FUNCS(page_number, int, )
DEF_EXTERNAL_FUNCS(job_mode, stp_job_mode_t, )
DEF_EXTERNAL_FUNCS(outdata, void *, )
DEF_EXTERNAL_FUNCS(errdata, void *, )
DEF_EXTERNAL_FUNCS(outfunc, stp_outfunc_t, )
DEF_EXTERNAL_FUNCS(errfunc, stp_outfunc_t, )

DEF_INTERNAL_FUNCS(output_color_model, int, )
DEF_INTERNAL_FUNCS(color_data, void *, )
DEF_INTERNAL_FUNCS(copy_color_data_func, stpi_copy_data_func_t, )
DEF_INTERNAL_FUNCS(destroy_color_data_func, stpi_destroy_data_func_t, )
DEF_INTERNAL_FUNCS(driver_data, void *, )
DEF_INTERNAL_FUNCS(copy_driver_data_func, stpi_copy_data_func_t, )
DEF_INTERNAL_FUNCS(destroy_driver_data_func, stpi_destroy_data_func_t, )
DEF_INTERNAL_FUNCS(dither_data, void *, )
DEF_INTERNAL_FUNCS(copy_dither_data_func, stpi_copy_data_func_t, )
DEF_INTERNAL_FUNCS(destroy_dither_data_func, stpi_destroy_data_func_t, )


void
stpi_set_verified(stp_vars_t vv, int val)
{
  stpi_internal_vars_t *v = (stpi_internal_vars_t *) vv;
  check_vars(v);
  v->verified = val;
}

int
stpi_get_verified(const stp_vars_t vv)
{
  stpi_internal_vars_t *v = (stpi_internal_vars_t *) vv;
  check_vars(v);
  return v->verified;
}

static void
set_default_raw_parameter(stpi_list_t *list, const char *parameter,
			  const char *value, int bytes, int typ)
{
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (value && !item)
    {
      value_t *val = stpi_malloc(sizeof(value_t));
      val->name = stpi_strdup(parameter);
      val->typ = typ;
      val->active = STP_PARAMETER_DEFAULTED;
      stpi_list_item_create(list, NULL, val);
      val->value.rval.data = stpi_malloc(bytes + 1);
      memcpy(val->value.rval.data, value, bytes);
      ((char *) val->value.rval.data)[bytes] = '\0';
      val->value.rval.bytes = bytes;
    }
}

static void
set_raw_parameter(stpi_list_t *list, const char *parameter, const char *value,
		  int bytes, int typ)
{
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (value)
    {
      value_t *val;
      if (item)
	{
	  val = (value_t *) stpi_list_item_get_data(item);
	  if (val->active == STP_PARAMETER_DEFAULTED)
	    val->active = STP_PARAMETER_ACTIVE;
	  stpi_free(val->value.rval.data);
	}
      else
	{
	  val = stpi_malloc(sizeof(value_t));
	  val->name = stpi_strdup(parameter);
	  val->typ = typ;
	  val->active = STP_PARAMETER_ACTIVE;
	  stpi_list_item_create(list, NULL, val);
	}
      val->value.rval.data = stpi_malloc(bytes + 1);
      memcpy(val->value.rval.data, value, bytes);
      ((char *) val->value.rval.data)[bytes] = '\0';
      val->value.rval.bytes = bytes;
    }
  else if (item)
    stpi_list_item_destroy(list, item);
}

void
stp_set_string_parameter_n(stp_vars_t v, const char *parameter,
			   const char *value, int bytes)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_STRING_LIST];
  set_raw_parameter(list, parameter, value, bytes,
		    STP_PARAMETER_TYPE_STRING_LIST);
  stpi_set_verified(v, 0);
}

void
stp_set_string_parameter(stp_vars_t v, const char *parameter,
			 const char *value)
{
  int byte_count = 0;
  if (value)
    byte_count = strlen(value);
  stp_set_string_parameter_n(v, parameter, value, byte_count);
  stpi_set_verified(v, 0);
}

void
stp_set_default_string_parameter_n(stp_vars_t v, const char *parameter,
				   const char *value, int bytes)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_STRING_LIST];
  set_default_raw_parameter(list, parameter, value, bytes,
			    STP_PARAMETER_TYPE_STRING_LIST);
  stpi_set_verified(v, 0);
}

void
stp_set_default_string_parameter(stp_vars_t v, const char *parameter,
				 const char *value)
{
  int byte_count = 0;
  if (value)
    byte_count = strlen(value);
  stp_set_default_string_parameter_n(v, parameter, value, byte_count);
  stpi_set_verified(v, 0);
}

void
stp_clear_string_parameter(stp_vars_t v, const char *parameter)
{
  stp_set_string_parameter(v, parameter, NULL);
}

const char *
stp_get_string_parameter(const stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_STRING_LIST];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      return val->value.rval.data;
    }
  else
    return NULL;
}

void
stp_set_raw_parameter(stp_vars_t v, const char *parameter,
		      const void *value, int bytes)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_RAW];
  set_raw_parameter(list, parameter, value, bytes, STP_PARAMETER_TYPE_RAW);
  stpi_set_verified(v, 0);
}

void
stp_set_default_raw_parameter(stp_vars_t v, const char *parameter,
			      const void *value, int bytes)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_RAW];
  set_default_raw_parameter(list, parameter, value, bytes,
			    STP_PARAMETER_TYPE_RAW);
  stpi_set_verified(v, 0);
}

void
stp_clear_raw_parameter(stp_vars_t v, const char *parameter)
{
  stp_set_raw_parameter(v, parameter, NULL, 0);
}

const stp_raw_t *
stp_get_raw_parameter(const stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_RAW];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      return &(val->value.rval);
    }
  else
    return NULL;
}

void
stp_set_file_parameter(stp_vars_t v, const char *parameter,
		       const char *value)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_FILE];
  int byte_count = 0;
  if (value)
    byte_count = strlen(value);
  set_raw_parameter(list, parameter, value, byte_count,
		    STP_PARAMETER_TYPE_FILE);
  stpi_set_verified(v, 0);
}

void
stp_set_file_parameter_n(stp_vars_t v, const char *parameter,
			 const char *value, int byte_count)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_FILE];
  set_raw_parameter(list, parameter, value, byte_count,
		    STP_PARAMETER_TYPE_FILE);
  stpi_set_verified(v, 0);
}

void
stp_set_default_file_parameter(stp_vars_t v, const char *parameter,
			       const char *value)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_FILE];
  int byte_count = 0;
  if (value)
    byte_count = strlen(value);
  set_default_raw_parameter(list, parameter, value, byte_count,
			    STP_PARAMETER_TYPE_FILE);
  stpi_set_verified(v, 0);
}

void
stp_set_default_file_parameter_n(stp_vars_t v, const char *parameter,
				 const char *value, int byte_count)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_FILE];
  set_default_raw_parameter(list, parameter, value, byte_count,
			    STP_PARAMETER_TYPE_FILE);
  stpi_set_verified(v, 0);
}

void
stp_clear_file_parameter(stp_vars_t v, const char *parameter)
{
  stp_set_file_parameter(v, parameter, NULL);
}

const char *
stp_get_file_parameter(const stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_FILE];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      return val->value.rval.data;
    }
  else
    return NULL;
}

void
stp_set_curve_parameter(stp_vars_t v, const char *parameter,
			const stp_curve_t curve)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_CURVE];
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (curve)
    {
      value_t *val;
      if (item)
	{
	  val = (value_t *) stpi_list_item_get_data(item);
	  if (val->active == STP_PARAMETER_DEFAULTED)
	    val->active = STP_PARAMETER_ACTIVE;
	  stp_curve_free(val->value.cval);
	}
      else
	{
	  val = stpi_malloc(sizeof(value_t));
	  val->name = stpi_strdup(parameter);
	  val->typ = STP_PARAMETER_TYPE_CURVE;
	  val->active = STP_PARAMETER_ACTIVE;
	  stpi_list_item_create(list, NULL, val);
	}
      val->value.cval = stp_curve_create_copy(curve);
    }
  else if (item)
    stpi_list_item_destroy(list, item);
  stpi_set_verified(v, 0);
}

void
stp_set_default_curve_parameter(stp_vars_t v, const char *parameter,
				const stp_curve_t curve)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_CURVE];
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (!item)
    {
      if (curve)
	{
	  value_t *val;
	  val = stpi_malloc(sizeof(value_t));
	  val->name = stpi_strdup(parameter);
	  val->typ = STP_PARAMETER_TYPE_CURVE;
	  val->active = STP_PARAMETER_DEFAULTED;
	  stpi_list_item_create(list, NULL, val);
	  val->value.cval = stp_curve_create_copy(curve);
	}
    }
  stpi_set_verified(v, 0);
}

void
stp_clear_curve_parameter(stp_vars_t v, const char *parameter)
{
  stp_set_curve_parameter(v, parameter, NULL);
}

const stp_curve_t
stp_get_curve_parameter(const stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_CURVE];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      return val->value.cval;
    }
  else
    return NULL;
}

void
stp_set_array_parameter(stp_vars_t v, const char *parameter,
			const stp_array_t array)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_ARRAY];
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (array)
    {
      value_t *val;
      if (item)
	{
	  val = (value_t *) stpi_list_item_get_data(item);
	  if (val->active == STP_PARAMETER_DEFAULTED)
	    val->active = STP_PARAMETER_ACTIVE;
	  stp_array_destroy(val->value.cval);
	}
      else
	{
	  val = stpi_malloc(sizeof(value_t));
	  val->name = stpi_strdup(parameter);
	  val->typ = STP_PARAMETER_TYPE_ARRAY;
	  val->active = STP_PARAMETER_ACTIVE;
	  stpi_list_item_create(list, NULL, val);
	}
      val->value.cval = stp_array_create_copy(array);
    }
  else if (item)
    stpi_list_item_destroy(list, item);
  stpi_set_verified(v, 0);
}

void
stp_set_default_array_parameter(stp_vars_t v, const char *parameter,
				const stp_array_t array)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_ARRAY];
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (!item)
    {
      if (array)
	{
	  value_t *val;
	  val = stpi_malloc(sizeof(value_t));
	  val->name = stpi_strdup(parameter);
	  val->typ = STP_PARAMETER_TYPE_ARRAY;
	  val->active = STP_PARAMETER_DEFAULTED;
	  stpi_list_item_create(list, NULL, val);
	  val->value.cval = stp_array_create_copy(array);
	}
    }
  stpi_set_verified(v, 0);
}

void
stp_clear_array_parameter(stp_vars_t v, const char *parameter)
{
  stp_set_array_parameter(v, parameter, NULL);
}

const stp_array_t
stp_get_array_parameter(const stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_ARRAY];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      return val->value.cval;
    }
  else
    return NULL;
}

void
stp_set_int_parameter(stp_vars_t v, const char *parameter, int ival)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_INT];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      if (val->active == STP_PARAMETER_DEFAULTED)
	val->active = STP_PARAMETER_ACTIVE;
    }
  else
    {
      val = stpi_malloc(sizeof(value_t));
      val->name = stpi_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_INT;
      val->active = STP_PARAMETER_ACTIVE;
      stpi_list_item_create(list, NULL, val);
    }
  val->value.ival = ival;
  stpi_set_verified(v, 0);
}

void
stp_set_default_int_parameter(stp_vars_t v, const char *parameter, int ival)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_INT];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (!item)
    {
      val = stpi_malloc(sizeof(value_t));
      val->name = stpi_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_INT;
      val->active = STP_PARAMETER_DEFAULTED;
      stpi_list_item_create(list, NULL, val);
      val->value.ival = ival;
    }
  stpi_set_verified(v, 0);
}

void
stp_clear_int_parameter(stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_INT];
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    stpi_list_item_destroy(list, item);
  stpi_set_verified(v, 0);
}

int
stp_get_int_parameter(const stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_INT];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      return val->value.ival;
    }
  else
    {
      stp_parameter_t desc;
      stp_describe_parameter(v, parameter, &desc);
      if (desc.p_type == STP_PARAMETER_TYPE_INT)
	{
	  int intval = desc.deflt.integer;
	  stp_parameter_description_free(&desc);
	  return intval;
	}
      else
	{
	  stp_parameter_description_free(&desc);
	  stpi_erprintf
	    ("GIMP-PRINT: Attempt to retrieve unset integer parameter %s\n",
	     parameter);
	  return 0;
	}
    }
}

void
stp_set_boolean_parameter(stp_vars_t v, const char *parameter, int ival)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_BOOLEAN];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      if (val->active == STP_PARAMETER_DEFAULTED)
	val->active = STP_PARAMETER_ACTIVE;
    }
  else
    {
      val = stpi_malloc(sizeof(value_t));
      val->name = stpi_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_BOOLEAN;
      val->active = STP_PARAMETER_ACTIVE;
      stpi_list_item_create(list, NULL, val);
    }
  if (ival)
    val->value.ival = 1;
  else
    val->value.ival = 0;
  stpi_set_verified(v, 0);
}

void
stp_set_default_boolean_parameter(stp_vars_t v, const char *parameter,
				  int ival)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_BOOLEAN];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (!item)
    {
      val = stpi_malloc(sizeof(value_t));
      val->name = stpi_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_BOOLEAN;
      val->active = STP_PARAMETER_DEFAULTED;
      stpi_list_item_create(list, NULL, val);
      if (ival)
	val->value.ival = 1;
      else
	val->value.ival = 0;
    }
  stpi_set_verified(v, 0);
}

void
stp_clear_boolean_parameter(stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_BOOLEAN];
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    stpi_list_item_destroy(list, item);
  stpi_set_verified(v, 0);
}

int
stp_get_boolean_parameter(const stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_BOOLEAN];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      return val->value.ival;
    }
  else
    {
      stp_parameter_t desc;
      stp_describe_parameter(v, parameter, &desc);
      if (desc.p_type == STP_PARAMETER_TYPE_BOOLEAN)
	{
	  int boolean = desc.deflt.boolean;
	  stp_parameter_description_free(&desc);
	  return boolean;
	}
      else
	{
	  stp_parameter_description_free(&desc);
	  stpi_erprintf
	    ("GIMP-PRINT: Attempt to retrieve unset boolean parameter %s\n",
	     parameter);
	  return 0;
	}
    }
}

void
stp_set_float_parameter(stp_vars_t v, const char *parameter, double dval)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_DOUBLE];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      if (val->active == STP_PARAMETER_DEFAULTED)
	val->active = STP_PARAMETER_ACTIVE;
    }
  else
    {
      val = stpi_malloc(sizeof(value_t));
      val->name = stpi_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_DOUBLE;
      val->active = STP_PARAMETER_ACTIVE;
      stpi_list_item_create(list, NULL, val);
    }
  val->value.dval = dval;
  stpi_set_verified(v, 0);
}

void
stp_set_default_float_parameter(stp_vars_t v, const char *parameter,
				double dval)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_DOUBLE];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (!item)
    {
      val = stpi_malloc(sizeof(value_t));
      val->name = stpi_strdup(parameter);
      val->typ = STP_PARAMETER_TYPE_DOUBLE;
      val->active = STP_PARAMETER_DEFAULTED;
      stpi_list_item_create(list, NULL, val);
      val->value.dval = dval;
    }
  stpi_set_verified(v, 0);
}

void
stp_clear_float_parameter(stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_DOUBLE];
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    stpi_list_item_destroy(list, item);
  stpi_set_verified(v, 0);
}

double
stp_get_float_parameter(const stp_vars_t v, const char *parameter)
{
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[STP_PARAMETER_TYPE_DOUBLE];
  value_t *val;
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    {
      val = (value_t *) stpi_list_item_get_data(item);
      return val->value.dval;
    }
  else
    {
      stp_parameter_t desc;
      stp_describe_parameter(v, parameter, &desc);
      if (desc.p_type == STP_PARAMETER_TYPE_DOUBLE)
	{
	  double dbl = desc.deflt.dbl;
	  stp_parameter_description_free(&desc);
	  return dbl;
	}
      else
	{
	  stp_parameter_description_free(&desc);
	  stpi_erprintf
	    ("GIMP-PRINT: Attempt to retrieve unset float parameter %s\n",
	     parameter);
	  return 1.0;
	}
    }
}

void
stp_scale_float_parameter(const stp_vars_t v, const char *parameter,
			  double scale)
{
  if (stp_check_float_parameter(v, parameter, STP_PARAMETER_INACTIVE))
    stp_set_float_parameter(v, parameter,
			    stp_get_float_parameter(v, parameter) * scale);
}

static int
check_parameter_generic(const stp_vars_t v, stp_parameter_type_t p_type,
			const char *parameter, stp_parameter_activity_t active)
{
  const stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[p_type];
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item && active <= ((value_t *) stpi_list_item_get_data(item))->active)
    return 1;
  else
    return 0;
}

#define CHECK_FUNCTION(type, index)					\
int									\
stp_check_##type##_parameter(const stp_vars_t v, const char *parameter,	\
			     stp_parameter_activity_t active)		\
{									\
  return check_parameter_generic(v, index, parameter, active);		\
}

CHECK_FUNCTION(string, STP_PARAMETER_TYPE_STRING_LIST)
CHECK_FUNCTION(file, STP_PARAMETER_TYPE_FILE)
CHECK_FUNCTION(float, STP_PARAMETER_TYPE_DOUBLE)
CHECK_FUNCTION(int, STP_PARAMETER_TYPE_INT)
CHECK_FUNCTION(boolean, STP_PARAMETER_TYPE_BOOLEAN)
CHECK_FUNCTION(curve, STP_PARAMETER_TYPE_CURVE)
CHECK_FUNCTION(array, STP_PARAMETER_TYPE_ARRAY)
CHECK_FUNCTION(raw, STP_PARAMETER_TYPE_RAW)

static stp_parameter_activity_t
get_parameter_active_generic(const stp_vars_t v, stp_parameter_type_t p_type,
			     const char *parameter)
{
  const stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[p_type];
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item)
    return ((value_t *) stpi_list_item_get_data(item))->active;
  else
    return 0;
}

#define GET_PARAMETER_ACTIVE_FUNCTION(type, index)			     \
stp_parameter_activity_t						     \
stp_get_##type##_parameter_active(const stp_vars_t v, const char *parameter) \
{									     \
  return get_parameter_active_generic(v, index, parameter);		     \
}

GET_PARAMETER_ACTIVE_FUNCTION(string, STP_PARAMETER_TYPE_STRING_LIST)
GET_PARAMETER_ACTIVE_FUNCTION(file, STP_PARAMETER_TYPE_FILE)
GET_PARAMETER_ACTIVE_FUNCTION(float, STP_PARAMETER_TYPE_DOUBLE)
GET_PARAMETER_ACTIVE_FUNCTION(int, STP_PARAMETER_TYPE_INT)
GET_PARAMETER_ACTIVE_FUNCTION(boolean, STP_PARAMETER_TYPE_BOOLEAN)
GET_PARAMETER_ACTIVE_FUNCTION(curve, STP_PARAMETER_TYPE_CURVE)
GET_PARAMETER_ACTIVE_FUNCTION(array, STP_PARAMETER_TYPE_ARRAY)
GET_PARAMETER_ACTIVE_FUNCTION(raw, STP_PARAMETER_TYPE_RAW)

static void
set_parameter_active_generic(const stp_vars_t v, stp_parameter_type_t p_type,
			     const char *parameter,
			     stp_parameter_activity_t active)
{
  const stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  stpi_list_t *list = vv->params[p_type];
  stpi_list_item_t *item = stpi_list_get_item_by_name(list, parameter);
  if (item && (active == STP_PARAMETER_ACTIVE ||
	       active == STP_PARAMETER_INACTIVE))
    ((value_t *) stpi_list_item_get_data(item))->active = active;
}

#define SET_PARAMETER_ACTIVE_FUNCTION(type, index)			     \
void									     \
stp_set_##type##_parameter_active(const stp_vars_t v, const char *parameter, \
				  stp_parameter_activity_t active)	     \
{									     \
  set_parameter_active_generic(v, index, parameter, active);		     \
}

SET_PARAMETER_ACTIVE_FUNCTION(string, STP_PARAMETER_TYPE_STRING_LIST)
SET_PARAMETER_ACTIVE_FUNCTION(file, STP_PARAMETER_TYPE_FILE)
SET_PARAMETER_ACTIVE_FUNCTION(float, STP_PARAMETER_TYPE_DOUBLE)
SET_PARAMETER_ACTIVE_FUNCTION(int, STP_PARAMETER_TYPE_INT)
SET_PARAMETER_ACTIVE_FUNCTION(boolean, STP_PARAMETER_TYPE_BOOLEAN)
SET_PARAMETER_ACTIVE_FUNCTION(curve, STP_PARAMETER_TYPE_CURVE)
SET_PARAMETER_ACTIVE_FUNCTION(array, STP_PARAMETER_TYPE_ARRAY)
SET_PARAMETER_ACTIVE_FUNCTION(raw, STP_PARAMETER_TYPE_RAW)

void
stpi_fill_parameter_settings(stp_parameter_t *desc,
			    const stp_parameter_t *param)
{
  if (param)
    {
      desc->p_type = param->p_type;
      desc->p_level = param->p_level;
      desc->p_class = param->p_class;
      desc->is_mandatory = param->is_mandatory;
      desc->is_active = param->is_active;
      desc->channel = param->channel;
      desc->name = param->name;
      desc->text = param->text;
      desc->help = param->help;
      return;
    }
}

void
stp_vars_copy(stp_vars_t vd, const stp_vars_t vs)
{
  int i;
  stpi_internal_vars_t *vvd = (stpi_internal_vars_t *)vd;
  const stpi_internal_vars_t *vvs = (const stpi_internal_vars_t *)vs;

  if (vs == vd)
    return;
  stp_set_driver(vd, stp_get_driver(vs));
  if (stpi_get_copy_driver_data_func(vs))
    stpi_set_driver_data(vd, (stpi_get_copy_driver_data_func(vs))(vs));
  else
    stpi_set_driver_data(vd, stpi_get_driver_data(vs));
  if (stpi_get_copy_dither_data_func(vs))
    stpi_set_dither_data(vd, (stpi_get_copy_dither_data_func(vs))(vs));
  else
    stpi_set_dither_data(vd, stpi_get_dither_data(vs));
  if (stpi_get_copy_color_data_func(vs))
    stpi_set_color_data(vd, (stpi_get_copy_color_data_func(vs))(vs));
  else
    stpi_set_color_data(vd, stpi_get_color_data(vs));
  for (i = 0; i < STP_PARAMETER_TYPE_INVALID; i++)
    {
      stpi_list_destroy(vvd->params[i]);
      vvd->params[i] = copy_value_list(vvs->params[i]);
    }

  stpi_set_copy_driver_data_func(vd, stpi_get_copy_driver_data_func(vs));
  stpi_set_copy_dither_data_func(vd, stpi_get_copy_dither_data_func(vs));
  stpi_set_copy_color_data_func(vd, stpi_get_copy_color_data_func(vs));
  stp_set_output_type(vd, stp_get_output_type(vs));
  stp_set_left(vd, stp_get_left(vs));
  stp_set_top(vd, stp_get_top(vs));
  stp_set_width(vd, stp_get_width(vs));
  stp_set_height(vd, stp_get_height(vs));
  stp_set_page_width(vd, stp_get_page_width(vs));
  stp_set_page_height(vd, stp_get_page_height(vs));
  stp_set_input_color_model(vd, stp_get_input_color_model(vd));
  stpi_set_output_color_model(vd, stpi_get_output_color_model(vd));
  stp_set_outdata(vd, stp_get_outdata(vs));
  stp_set_errdata(vd, stp_get_errdata(vs));
  stp_set_outfunc(vd, stp_get_outfunc(vs));
  stp_set_errfunc(vd, stp_get_errfunc(vs));
  stpi_set_verified(vd, stpi_get_verified(vs));
}

void
stpi_prune_inactive_options(stp_vars_t v)
{
  stp_parameter_list_t params = stp_get_parameter_list(v);
  stpi_internal_vars_t *vv = (stpi_internal_vars_t *)v;
  int i;
  for (i = 0; i < STP_PARAMETER_TYPE_INVALID; i++)
    {
      stpi_list_t *list = vv->params[i];
      stpi_list_item_t *item = stpi_list_get_start(list);
      while (item)
	{
	  stpi_list_item_t *next = stpi_list_item_next(item);
	  value_t *var = (value_t *)stpi_list_item_get_data(item);
	  if (var->active < STP_PARAMETER_DEFAULTED ||
	      !(stp_parameter_find(params, var->name)))
	    stpi_list_item_destroy(list, item);
	  item = next;
	}
    }
  stp_parameter_list_free(params);
}

stp_vars_t
stp_vars_create_copy(const stp_vars_t vs)
{
  stp_vars_t vd = stp_vars_create();
  stp_vars_copy(vd, vs);
  return (vd);
}

void
stp_merge_printvars(stp_vars_t user, const stp_vars_t print)
{
  int count;
  int i;
  stp_parameter_list_t params = stp_get_parameter_list(print);
  count = stp_parameter_list_count(params);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      if (p->p_type == STP_PARAMETER_TYPE_DOUBLE &&
	  p->p_class == STP_PARAMETER_CLASS_OUTPUT &&
	  p->p_level == STP_PARAMETER_LEVEL_BASIC)
	{
	  stp_parameter_t desc;
	  double usrval = stp_get_float_parameter(user, p->name);
	  double prnval = stp_get_float_parameter(print, p->name);
	  stp_describe_parameter(print, p->name, &desc);
	  if (strcmp(p->name, "Gamma") == 0)
	    usrval /= prnval;
	  else
	    usrval *= prnval;
	  if (usrval < desc.bounds.dbl.lower)
	    usrval = desc.bounds.dbl.lower;
	  else if (usrval > desc.bounds.dbl.upper)
	    usrval = desc.bounds.dbl.upper;
	  stp_set_float_parameter(user, p->name, usrval);
	  stp_parameter_description_free(&desc);
	}
    }
  if (stp_get_output_type(print) == OUTPUT_GRAY &&
      (stp_get_output_type(user) == OUTPUT_COLOR ||
       stp_get_output_type(user) == OUTPUT_RAW_CMYK))
    stp_set_output_type(user, OUTPUT_GRAY);
  stp_parameter_list_free(params);
}

void
stp_set_printer_defaults(stp_vars_t v, const stp_printer_t p)
{
  stp_parameter_list_t *params;
  int count;
  int i;
  stp_parameter_t desc;
  stp_set_driver(v, stp_printer_get_driver(p));
  params = stp_get_parameter_list(v);
  count = stp_parameter_list_count(params);
  for (i = 0; i < count; i++)
    {
      const stp_parameter_t *p = stp_parameter_list_param(params, i);
      if (p->is_mandatory)
	{
	  stp_describe_parameter(v, p->name, &desc);
	  switch (p->p_type)
	    {
	    case STP_PARAMETER_TYPE_STRING_LIST:
	      stp_set_string_parameter(v, p->name, desc.deflt.str);
	      break;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      stp_set_float_parameter(v, p->name, desc.deflt.dbl);
	      break;
	    case STP_PARAMETER_TYPE_INT:
	      stp_set_int_parameter(v, p->name, desc.deflt.integer);
	      break;
	    case STP_PARAMETER_TYPE_BOOLEAN:
	      stp_set_boolean_parameter(v, p->name, desc.deflt.boolean);
	      break;
	    case STP_PARAMETER_TYPE_CURVE:
	      stp_set_curve_parameter(v, p->name, desc.deflt.curve);
	      break;
	    case STP_PARAMETER_TYPE_ARRAY:
	      stp_set_array_parameter(v, p->name, desc.deflt.array);
	      break;
	    default:
	      break;
	    }
	  stp_parameter_description_free(&desc);
	}
    }
  stp_parameter_list_free(params);
}

static const char *
param_namefunc(const stpi_list_item_t *item)
{
  const stp_parameter_t *param =
    (const stp_parameter_t *) stpi_list_item_get_data(item);
  return param->name;
}

static const char *
param_longnamefunc(const stpi_list_item_t *item)
{
  const stp_parameter_t *param =
    (const stp_parameter_t *) stpi_list_item_get_data(item);
  return param->text;
}

stp_parameter_list_t
stp_parameter_list_create(void)
{
  stpi_list_t *ret = stpi_list_create();
  stpi_list_set_namefunc(ret, param_namefunc);
  stpi_list_set_long_namefunc(ret, param_longnamefunc);
  return (stp_parameter_list_t) ret;
}

void
stp_parameter_list_add_param(stp_parameter_list_t list,
			     const stp_parameter_t *item)
{
  stpi_list_t *ilist = (stpi_list_t *) list;
  stpi_list_item_create(ilist, NULL, (void *) item);
}

stp_parameter_list_t
stp_get_parameter_list(const stp_vars_t v)
{
  stp_parameter_list_t ret = stp_parameter_list_create();
  stp_parameter_list_t tmp_list;

  tmp_list = stpi_printer_list_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_free(tmp_list);

  tmp_list = stpi_color_list_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_free(tmp_list);

  tmp_list = stpi_dither_list_parameters(v);
  stp_parameter_list_append(ret, tmp_list);
  stp_parameter_list_free(tmp_list);

  return ret;
}

void
stp_describe_parameter(const stp_vars_t v, const char *name,
		       stp_parameter_t *description)
{
  description->p_type = STP_PARAMETER_TYPE_INVALID;
/* Set these to NULL in case stpi_*_describe_parameter() doesn't */
  description->bounds.str = NULL;
  description->deflt.str = NULL;
  stpi_printer_describe_parameter(v, name, description);
  if (description->p_type != STP_PARAMETER_TYPE_INVALID)
    return;
  stpi_color_describe_parameter(v, name, description);
  if (description->p_type != STP_PARAMETER_TYPE_INVALID)
    return;
  stpi_dither_describe_parameter(v, name, description);
}

void
stp_parameter_description_free(stp_parameter_t *desc)
{
  switch (desc->p_type)
    {
    case STP_PARAMETER_TYPE_CURVE:
      if (desc->bounds.curve)
	stp_curve_free(desc->bounds.curve);
      desc->bounds.curve = NULL;
      break;
    case STP_PARAMETER_TYPE_ARRAY:
      if (desc->bounds.array)
	stp_array_destroy(desc->bounds.array);
      desc->bounds.array = NULL;
      break;
    case STP_PARAMETER_TYPE_STRING_LIST:
      if (desc->bounds.str)
	stp_string_list_free(desc->bounds.str);
      desc->bounds.str = NULL;
      break;
    default:
      break;
    }
}

const stp_parameter_t *
stp_parameter_find_in_settings(const stp_vars_t v, const char *name)
{
  stp_parameter_list_t param_list = stp_get_parameter_list(v);
  const stp_parameter_t *param = stp_parameter_find(param_list, name);
  stp_parameter_list_free(param_list);
  return param;
}

size_t
stp_parameter_list_count(const stp_parameter_list_t list)
{
  stpi_list_t *ilist = (stpi_list_t *)list;
  return stpi_list_get_length(ilist);
}

const stp_parameter_t *
stp_parameter_find(const stp_parameter_list_t list, const char *name)
{
  stpi_list_t *ilist = (stpi_list_t *)list;
  stpi_list_item_t *item = stpi_list_get_item_by_name(ilist, name);
  if (item)
    return (stp_parameter_t *) stpi_list_item_get_data(item);
  else
    return NULL;
}

const stp_parameter_t *
stp_parameter_list_param(const stp_parameter_list_t list, size_t item)
{
  stpi_list_t *ilist = (stpi_list_t *)list;
  if (item >= stpi_list_get_length(ilist))
    return NULL;
  else
    return (stp_parameter_t *)
      stpi_list_item_get_data(stpi_list_get_item_by_index(ilist, item));
}

void
stp_parameter_list_free(stp_parameter_list_t list)
{
  stpi_list_destroy((stpi_list_t *)list);
}

stp_parameter_list_t
stp_parameter_list_copy(const stp_parameter_list_t list)
{
  stpi_list_t *ret = stp_parameter_list_create();
  int i;
  size_t count = stp_parameter_list_count(list);
  for (i = 0; i < count; i++)
    stpi_list_item_create(ret, NULL, (void *)stp_parameter_list_param(list, i));
  return ret;
}

void
stp_parameter_list_append(stp_parameter_list_t list,
			  const stp_parameter_list_t append)
{
  int i;
  stpi_list_t *ilist = (stpi_list_t *)list;
  size_t count = stp_parameter_list_count(append);
  for (i = 0; i < count; i++)
    stpi_list_item_create(ilist, NULL,
			 (void *) stp_parameter_list_param(append, i));
}
