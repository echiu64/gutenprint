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

typedef struct
{
  int cookie;
  size_t count;
  size_t active_count;
  stp_param_string_t *list;
} stp_internal_param_list_t;

static void
check_param_list(const stp_internal_param_list_t *v)
{
  if (v->cookie != COOKIE_PARAM_LIST)
    {
      stp_erprintf("Bad string list!\n");
      exit(2);
    }
}

stp_string_list_t
stp_string_list_allocate(void)
{
  stp_internal_param_list_t *ret =
    stp_zalloc(sizeof(stp_internal_param_list_t));
  ret->cookie = COOKIE_PARAM_LIST;
  return (stp_string_list_t) ret;
}

void
stp_string_list_free(stp_string_list_t list)
{
  stp_internal_param_list_t *ilist = (stp_internal_param_list_t *) list;
  size_t i = 0;
  check_param_list(ilist);
  while (i < ilist->active_count)
    {
      stp_free((void *) (ilist->list[i].name));
      stp_free((void *) (ilist->list[i].text));
      i++;
    }
  if (ilist->list)
    stp_free(ilist->list);
  stp_free(ilist);
}

stp_param_string_t *
stp_string_list_param(const stp_string_list_t list, size_t element)
{
  const stp_internal_param_list_t *ilist = (stp_internal_param_list_t *) list;
  check_param_list(ilist);
  if (element >= ilist->active_count)
    return NULL;
  else
    return &(ilist->list[element]);
}

size_t
stp_string_list_count(const stp_string_list_t list)
{
  const stp_internal_param_list_t *ilist = (stp_internal_param_list_t *) list;
  check_param_list(ilist);
  return ilist->active_count;
}

stp_string_list_t
stp_string_list_duplicate(const stp_string_list_t list)
{
  size_t i = 0;
  stp_string_list_t retval = stp_string_list_allocate();
  while (i < stp_string_list_count(list))
    {
      const stp_param_string_t *param = stp_string_list_param(list, i);
      stp_string_list_add_param(retval, param->name, param->text);
      i++;
    }
  return retval;
}

stp_string_list_t
stp_string_list_duplicate_params(const stp_param_string_t *list, size_t count)
{
  size_t i = 0;
  stp_string_list_t retval = stp_string_list_allocate();
  while (i < count)
    {
      stp_string_list_add_param(retval, list[i].name, list[i].text);
      i++;
    }
  return retval;
}

void
stp_string_list_add_param(stp_string_list_t list,
			 const char *name, const char *text)
{
  stp_internal_param_list_t *ilist = (stp_internal_param_list_t *) list;
  check_param_list(ilist);
  if (ilist->count == 0)
    {
      ilist->list = stp_zalloc(sizeof(stp_param_string_t));
      ilist->count = 1;
    }
  else if (ilist->active_count == ilist->count)
    {
      ilist->list =
	stp_realloc(ilist->list,
		    2 * ilist->count * sizeof(stp_param_string_t));
      ilist->count *= 2;
    }
  ilist->list[ilist->active_count].name = stp_strdup(name);
  ilist->list[ilist->active_count].text = stp_strdup(text);
  ilist->active_count++;
}
