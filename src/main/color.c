/*
 * "$Id$"
 *
 *   Gimp-Print color module interface.
 *
 *   Copyright (C) 2003  Roger Leigh (rleigh@debian.org)
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
#include <string.h>
#include <stdlib.h>
#include "module.h"


static const char* stpi_color_namefunc(const void *item);
static const char* stpi_color_long_namefunc(const void *item);

static stpi_list_t *color_list = NULL;


static int
stpi_init_color_list(void)
{
  if(color_list)
    stpi_list_destroy(color_list);
  color_list = stpi_list_create();
  stpi_list_set_namefunc(color_list, stpi_color_namefunc);
  stpi_list_set_long_namefunc(color_list, stpi_color_long_namefunc);
  /* stpi_list_set_sortfunc(color_list, stpi_color_sortfunc); */

  return 0;
}

static inline void
check_list(void)
{
  if (color_list == NULL)
    {
      stpi_erprintf("No color drivers found: "
		   "are STP_DATA_PATH and STP_MODULE_PATH correct?\n");
      stpi_init_color_list();
    }
}



int
stp_color_count(void)
{
  if (color_list == NULL)
    {
      stpi_erprintf("No color modules found: "
		    "is STP_MODULE_PATH correct?\n");
      stpi_init_color_list();
    }
  return stpi_list_get_length(color_list);
}


static inline void
check_color(const stpi_internal_color_t *c)
{
  if (c == NULL)
    {
      stpi_erprintf("Null stp_color_t! Please report this bug.\n");
      stpi_abort();
    }
  if (c->cookie != COOKIE_COLOR)
    {
      stpi_erprintf("Bad stp_color_t! Please report this bug.\n");
      stpi_abort();
    }
}


stp_const_color_t
stp_get_color_by_index(int idx)
{
  stpi_list_item_t *color;

  check_list();

  color = stpi_list_get_item_by_index(color_list, idx);
  if (color == NULL)
    return NULL;
  return (stp_const_color_t) stpi_list_item_get_data(color);
}


static const char *
stpi_color_namefunc(const void *item)
{
  const stpi_internal_color_t *color = (const stpi_internal_color_t *) item;
  check_color(color);
  return color->short_name;
}


static const char *
stpi_color_long_namefunc(const void *item)
{
  const stpi_internal_color_t *color = (const stpi_internal_color_t *) item;
  check_color(color);
  return color->long_name;
}


const char *
stp_color_get_name(stp_const_color_t c)
{
  const stpi_internal_color_t *val = (const stpi_internal_color_t *) c;
  check_color(val);
  return val->short_name;
}

const char *
stp_color_get_long_name(stp_const_color_t c)
{
  const stpi_internal_color_t *val = (const stpi_internal_color_t *) c;
  check_color(val);
  return gettext(val->long_name);
}


static const stpi_colorfuncs_t *
stpi_get_colorfuncs(stp_const_color_t c)
{
  const stpi_internal_color_t *val = (const stpi_internal_color_t *) c;
  check_color(val);
  return val->colorfuncs;
}


stp_const_color_t
stp_get_color_by_name(const char *name)
{
  stpi_list_item_t *color;

  check_list();

  color = stpi_list_get_item_by_name(color_list, name);
  if (!color)
    return NULL;
  return (stp_const_color_t) stpi_list_item_get_data(color);
}

stp_const_color_t
stpi_get_color_by_colorfuncs(stpi_colorfuncs_t *colorfuncs)
{
  stpi_list_item_t *color_item;
  stpi_internal_color_t *color;

  check_list();

  color_item = stpi_list_get_start(color_list);
  while (color_item)
    {
      color = (stpi_internal_color_t *) stpi_list_item_get_data(color_item);
      if (color->colorfuncs == colorfuncs)
	return (stp_const_color_t) color;
      color_item = stpi_list_item_next(color_item);
    }
  return NULL;
}


int
stpi_color_init(stp_vars_t v,
		stp_image_t *image,
		size_t steps)
{
  const stpi_colorfuncs_t *colorfuncs =
    stpi_get_colorfuncs(stp_get_color_by_name(stp_get_color_conversion(v)));
  return colorfuncs->init(v, image, steps);
}

int
stpi_color_get_row(stp_const_vars_t v,
		   stp_image_t *image,
		   int row,
		   unsigned *zero_mask)
{
  const stpi_colorfuncs_t *colorfuncs =
    stpi_get_colorfuncs(stp_get_color_by_name(stp_get_color_conversion(v)));
  return colorfuncs->get_row(v, image, row, zero_mask);
}

stp_parameter_list_t
stpi_color_list_parameters(stp_const_vars_t v)
{
  const stpi_colorfuncs_t *colorfuncs =
    stpi_get_colorfuncs(stp_get_color_by_name(stp_get_color_conversion(v)));
  return colorfuncs->list_parameters(v);
}

void
stpi_color_describe_parameter(stp_const_vars_t v, const char *name,
			       stp_parameter_t *description)
{
  const stpi_colorfuncs_t *colorfuncs =
    stpi_get_colorfuncs(stp_get_color_by_name(stp_get_color_conversion(v)));
  colorfuncs->describe_parameter(v, name, description);
}


int
stpi_color_register(const stpi_internal_color_t *color)
{
  if (color_list == NULL)
    {
      stpi_init_color_list();
      if (stpi_debug_level & STPI_DBG_COLORFUNC)
	stpi_erprintf
	  ("stpi_color_register(): initialising color_list...\n");
    }

  check_color(color);

  if (color)
    {
      /* Add new color algorithm if it does not already exist */
      if (stp_get_color_by_name(color->short_name) == NULL)
	{
	  if (stpi_debug_level & STPI_DBG_COLORFUNC)
	    stpi_erprintf("stpi_color_register(): registered colour module \"%s\"\n",
			  color->short_name);
	  stpi_list_item_create(color_list, NULL, color);
	}
    }

  return 0;
}

int
stpi_color_unregister(const stpi_internal_color_t *color)
{
  stpi_list_item_t *color_item;
  stpi_internal_color_t *color_data;

  if (color_list == NULL)
    {
      stpi_init_color_list();
      if (stpi_debug_level & STPI_DBG_COLORFUNC)
	stpi_erprintf
	  ("stpi_family_unregister(): initialising color_list...\n");
    }

  check_color(color);

  color_item = stpi_list_get_start(color_list);
  while (color_item)
    {
      color_data = (stpi_internal_color_t *) stpi_list_item_get_data(color_item);
      if (strcmp(color->short_name, color_data->short_name) == 0)
	{
	  if (stpi_debug_level & STPI_DBG_COLORFUNC)
	    stpi_erprintf("stpi_color_unregister(): unregistered colour module \"%s\"\n",
			  color->short_name);
	  stpi_list_item_destroy(color_list, color_item);
	  break;
	}
      color_item = stpi_list_item_next(color_item);
    }

  return 0;
}

