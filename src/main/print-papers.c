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
#include "papers.h"

static stpi_list_t *paper_list = NULL;

static void
stpi_paper_freefunc(stpi_list_item_t *item)
{
  stpi_internal_papersize_t *paper =
    (stpi_internal_papersize_t *) stpi_list_item_get_data(item);
  stpi_free(paper->name);
  stpi_free(paper->text);
  stpi_free(paper->comment);
  stpi_free(paper);
}

static const char *
stpi_paper_namefunc(const stpi_list_item_t *item)
{
  stpi_internal_papersize_t *paper =
    (stpi_internal_papersize_t *) stpi_list_item_get_data(item);
  return stp_papersize_get_name(paper);
}

static const char *
stpi_paper_long_namefunc(const stpi_list_item_t *item)
{
  stpi_internal_papersize_t *paper =
    (stpi_internal_papersize_t *) stpi_list_item_get_data(item);
  return stp_papersize_get_text(paper);
}

int
stpi_paper_list_init(void)
{
  if(paper_list)
    stpi_list_destroy(paper_list);
  paper_list = stpi_list_create();
  stpi_list_set_freefunc(paper_list, stpi_paper_freefunc);
  stpi_list_set_namefunc(paper_list, stpi_paper_namefunc);
  stpi_list_set_long_namefunc(paper_list, stpi_paper_long_namefunc);
  /* stpi_list_set_sortfunc(stpi_paper_sortfunc); */

  return 0;
}


int stpi_paper_create(stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  stpi_list_item_t *paper_item;

  if (paper_list == NULL)
    {
      stpi_paper_list_init();
      if (stpi_debug_level & STPI_DBG_PAPER)
	stpi_erprintf("stpi_paper_create(): initialising paper_list...\n");
    }

  /* Check the paper does not already exist */
  paper_item = stpi_list_get_start(paper_list);
  while (paper_item)
    {
      const stpi_internal_papersize_t *ep = (const stpi_internal_papersize_t *)
	stpi_list_item_get_data(paper_item);
      if (ep && !strcmp(p->name, ep->name))
	return 1;
      paper_item = stpi_list_item_next(paper_item);
    }

  /* Add paper to list */
  stpi_list_item_create(paper_list, NULL, (void *) p);

  return 0;
}

int stpi_paper_destroy(stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  stpi_list_item_t *paper_item;

  if (paper_list == NULL)
    {
      stpi_erprintf("No papers found: "
		   "is STP_MODULE_PATH correct?\n");
      stpi_paper_list_init();
      if (stpi_debug_level & STPI_DBG_PAPER)
	stpi_erprintf("stpi_paper_destroy(): initialising paper_list...\n");
    }

  /* Check if paper exists */
  paper_item = stpi_list_get_start(paper_list);
  while (paper_item)
    {
      const stpi_internal_papersize_t *ep = (const stpi_internal_papersize_t *)
	stpi_list_item_get_data(paper_item);
      if (ep && !strcmp(p->name, ep->name))
	{
	  stpi_list_item_destroy (paper_list, paper_item);
	  return 0;
	}
      paper_item = stpi_list_item_next(paper_item);
    }
  /* Paper did not exist */
  return 1;
}


int
stp_known_papersizes(void)
{
  if (paper_list == NULL)
    {
      stpi_erprintf("No papers found: "
		   "is STP_MODULE_PATH correct?\n");
      stpi_paper_list_init();
    }

  return stpi_list_get_length(paper_list);
}

const char *
stp_papersize_get_name(const stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  return p->name;
}

const char *
stp_papersize_get_text(const stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  return _(p->text);
}

unsigned
stp_papersize_get_width(const stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  return p->width;
}

unsigned
stp_papersize_get_height(const stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  return p->height;
}

unsigned
stp_papersize_get_top(const stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  return p->top;
}

unsigned
stp_papersize_get_left(const stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  return p->left;
}

unsigned
stp_papersize_get_bottom(const stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  return p->bottom;
}

unsigned
stp_papersize_get_right(const stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  return p->right;
}

stp_papersize_unit_t
stp_papersize_get_unit(const stp_papersize_t pt)
{
  const stpi_internal_papersize_t *p = (const stpi_internal_papersize_t *) pt;
  return p->paper_unit;
}

const stp_papersize_t
stp_get_papersize_by_name(const char *name)
{
  stpi_list_item_t *paper;

  if (paper_list == NULL)
    {
      stpi_erprintf("No papers found: "
		   "is STP_MODULE_PATH correct?\n");
      stpi_paper_list_init();
    }

  paper = stpi_list_get_item_by_name(paper_list, name);
  if (!paper)
    return NULL;
  else return (const stp_papersize_t) stpi_list_item_get_data(paper);
}

const stp_papersize_t
stp_get_papersize_by_index(int index)
{
  stpi_list_item_t *paper;

  if (paper_list == NULL)
    {
      stpi_erprintf("No papers found: "
		   "is STP_MODULE_PATH correct?\n");
      stpi_paper_list_init();
    }

  paper = stpi_list_get_item_by_index(paper_list, index);
  if (!paper)
    return NULL;
  else return (const stp_papersize_t) stpi_list_item_get_data(paper);
}

static int
paper_size_mismatch(int l, int w, const stpi_internal_papersize_t *val)
{
  int hdiff = abs(l - (int) val->height);
  int vdiff = abs(w - (int) val->width);
  return hdiff + vdiff;
}

const stp_papersize_t
stp_get_papersize_by_size(int l, int w)
{
  int score = INT_MAX;
  const stpi_internal_papersize_t *ref = NULL;
  const stpi_internal_papersize_t *val = NULL;
  int i;
  int sizes = stp_known_papersizes();
  for (i = 0; i < sizes; i++)
    {
      val = stp_get_papersize_by_index(i);

      if (val->width == w && val->height == l)
	return (stp_papersize_t) val;
      else
	{
	  int myscore = paper_size_mismatch(l, w, val);
	  if (myscore < score && myscore < 20)
	    {
	      ref = val;
	      score = myscore;
	    }
	}
    }
  return (stp_papersize_t) ref;
}

void
stpi_default_media_size(const stp_vars_t v,	/* I */
		       int  *width,		/* O - Width in points */
		       int  *height)	/* O - Height in points */
{
  if (stp_get_page_width(v) > 0 && stp_get_page_height(v) > 0)
    {
      *width = stp_get_page_width(v);
      *height = stp_get_page_height(v);
    }
  else
    {
      const char *page_size = stp_get_string_parameter(v, "PageSize");
      stp_papersize_t papersize = NULL;
      if (page_size)
	papersize = stp_get_papersize_by_name(page_size);
      if (!papersize)
	{
	  *width = 1;
	  *height = 1;
	}
      else
	{
	  *width = stp_papersize_get_width(papersize);
	  *height = stp_papersize_get_height(papersize);
	}
      if (*width == 0)
	*width = 612;
      if (*height == 0)
	*height = 792;
    }
}
