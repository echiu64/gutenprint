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

typedef struct
{
  const char *name;
  const char *text;
  unsigned width;
  unsigned height;
  unsigned top;
  unsigned left;
  unsigned bottom;
  unsigned right;
  stp_papersize_unit_t paper_unit;
} stp_internal_papersize_t;

/*
 * Sizes are converted to 1/72in, then rounded down so that we don't
 * print off the edge of the paper.
 */
#include "papers-oldlist.h"

int
stp_known_papersizes(void)
{
  return sizeof(paper_sizes) / sizeof(stp_internal_papersize_t) - 1;
}

const char *
stp_papersize_get_name(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->name;
}

const char *
stp_papersize_get_text(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return _(p->text);
}

unsigned
stp_papersize_get_width(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->width;
}

unsigned
stp_papersize_get_height(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->height;
}

unsigned
stp_papersize_get_top(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->top;
}

unsigned
stp_papersize_get_left(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->left;
}

unsigned
stp_papersize_get_bottom(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->bottom;
}

unsigned
stp_papersize_get_right(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->right;
}

stp_papersize_unit_t
stp_papersize_get_unit(const stp_papersize_t pt)
{
  const stp_internal_papersize_t *p = (const stp_internal_papersize_t *) pt;
  return p->paper_unit;
}

#if 1
/*
 * This is, of course, blatantly thread-unsafe.  However, it certainly
 * speeds up genppd by a lot!
 */
const stp_papersize_t
stp_get_papersize_by_name(const char *name)
{
  static int last_used_papersize = 0;
  int base = last_used_papersize;
  int sizes = stp_known_papersizes();
  int i;
  if (!name)
    return NULL;
  for (i = 0; i < sizes; i++)
    {
      int size_to_try = (i + base) % sizes;
      const stp_internal_papersize_t *val = &(paper_sizes[size_to_try]);
      if (!strcmp(val->name, name))
	{
	  last_used_papersize = size_to_try;
	  return (const stp_papersize_t) val;
	}
    }
  return NULL;
}
#else
const stp_papersize_t
stp_get_papersize_by_name(const char *name)
{
  const stp_internal_papersize_t *val = &(paper_sizes[0]);
  if (!name)
    return NULL;
  while (stp_strlen(val->name) > 0)
    {
      if (!strcmp(val->name, name))
	return (stp_papersize_t) val;
      val++;
    }
  return NULL;
}
#endif

const stp_papersize_t
stp_get_papersize_by_index(int index)
{
  if (index < 0 || index >= stp_known_papersizes())
    return NULL;
  else
    return (stp_papersize_t) &(paper_sizes[index]);
}

static int
paper_size_mismatch(int l, int w, const stp_internal_papersize_t *val)
{
  int hdiff = abs(l - (int) val->height);
  int vdiff = abs(w - (int) val->width);
  return hdiff + vdiff;
}

const stp_papersize_t
stp_get_papersize_by_size(int l, int w)
{
  int score = INT_MAX;
  const stp_internal_papersize_t *ref = NULL;
  const stp_internal_papersize_t *val = &(paper_sizes[0]);
  int sizes = stp_known_papersizes();
  int i;
  for (i = 0; i < sizes; i++)
    {
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
      val++;
    }
  return (stp_papersize_t) ref;
}

void
stp_default_media_size(const stp_vars_t v,	/* I */
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
      const stp_papersize_t papersize =
	stp_get_papersize_by_name(stp_get_string_parameter(v, "PageSize"));
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
