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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>

static void
free_list_element(stpi_list_item_t *item)
{
  stp_param_string_t *string =
    (stp_param_string_t *) stpi_list_item_get_data(item);
  stpi_free((char *) string->name);
  stpi_free((char *) string->text);
  stpi_free(string);
}

static const char *
namefunc(const stpi_list_item_t *item)
{
  stp_param_string_t *string =
    (stp_param_string_t *) stpi_list_item_get_data(item);
  return string->name;
}

static void *
copyfunc(const stpi_list_item_t *item)
{
  stp_param_string_t *string =
    (stp_param_string_t *) stpi_list_item_get_data(item);
  stp_param_string_t *new_string = stpi_malloc(sizeof(stp_param_string_t));
  new_string->name = stpi_strdup(string->name);
  new_string->text = stpi_strdup(string->text);
  return new_string;
}

static const char *
long_namefunc(const stpi_list_item_t *item)
{
  stp_param_string_t *string =
    (stp_param_string_t *) stpi_list_item_get_data(item);
  return string->text;
}

stp_string_list_t
stp_string_list_create(void)
{
  stpi_list_t *ret = stpi_list_create();
  stpi_list_set_freefunc(ret, free_list_element);
  stpi_list_set_namefunc(ret, namefunc);
  stpi_list_set_copyfunc(ret, copyfunc);
  stpi_list_set_long_namefunc(ret, long_namefunc);
  return (stp_string_list_t) ret;
}

void
stp_string_list_free(stp_string_list_t list)
{
  stpi_list_destroy((stpi_list_t *) list);
}

stp_param_string_t *
stp_string_list_param(const stp_string_list_t list, size_t element)
{
  return (stp_param_string_t *) stpi_list_item_get_data
    (stpi_list_get_item_by_index((stpi_list_t *)list, element));
}

size_t
stp_string_list_count(const stp_string_list_t list)
{
  return stpi_list_get_length((stpi_list_t *)list);
}

stp_string_list_t
stp_string_list_create_copy(const stp_string_list_t list)
{
  return (stp_string_list_t) stpi_list_copy((stpi_list_t *)list);
}

stp_string_list_t
stp_string_list_create_from_params(const stp_param_string_t *list, size_t count)
{
  size_t i = 0;
  stp_string_list_t retval = stp_string_list_create();
  for (i = 0; i < count; i++)
    stp_string_list_add_string(retval, list[i].name, list[i].text);
  return retval;
}

void
stp_string_list_add_string(stp_string_list_t list,
			  const char *name, const char *text)
{
  stp_param_string_t *new_string = stpi_malloc(sizeof(stp_param_string_t));
  new_string->name = stpi_strdup(name);
  new_string->text = stpi_strdup(text);
  stpi_list_item_create((stpi_list_t *) list, NULL, new_string);
}
