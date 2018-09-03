/*
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <sys/param.h>

typedef struct
{
  char *name;
  stp_papersize_list_t *list;
} papersize_list_impl_t;

static stp_list_t *list_of_papersize_lists = NULL;

static void
papersize_list_impl_freefunc(void *item)
{
  papersize_list_impl_t *papersize_list = (papersize_list_impl_t *) item;
  stp_list_destroy(papersize_list->list);
  STP_SAFE_FREE(papersize_list->name);
  STP_SAFE_FREE(papersize_list);
}

static const char *
papersize_list_impl_namefunc(const void *item)
{
  return ((const papersize_list_impl_t *) item)->name;
}

static const char *
papersize_list_impl_long_namefunc(const void *item)
{
  return ((const papersize_list_impl_t *) item)->name;
}

static void
check_list_of_papersize_lists(void)
{
  if (! list_of_papersize_lists)
    {
      stp_deprintf(STP_DBG_PAPER, "Initializing...\n");
      list_of_papersize_lists = stp_list_create();
      stp_list_set_freefunc(list_of_papersize_lists, papersize_list_impl_freefunc);
      stp_list_set_namefunc(list_of_papersize_lists, papersize_list_impl_namefunc);
      stp_list_set_long_namefunc(list_of_papersize_lists, papersize_list_impl_long_namefunc);
    }
}

static void
stpi_papersize_freefunc(void *item)
{
  stp_papersize_t *paper = (stp_papersize_t *) (item);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
  STP_SAFE_FREE(paper->name);
  STP_SAFE_FREE(paper->text);
  STP_SAFE_FREE(paper->comment);
#pragma GCC diagnostic pop
  STP_SAFE_FREE(paper);
}

static const char *
stpi_papersize_namefunc(const void *item)
{
  const stp_papersize_t *paper = (const stp_papersize_t *) (item);
  return paper->name;
}

static const char *
stpi_papersize_long_namefunc(const void *item)
{
  const stp_papersize_t *paper = (const stp_papersize_t *) (item);
  return paper->text;
}

stp_papersize_list_t *
stpi_create_papersize_list(void)
{
  stp_list_t *papersize_list = stp_list_create();
  stp_list_set_freefunc(papersize_list, stpi_papersize_freefunc);
  stp_list_set_namefunc(papersize_list, stpi_papersize_namefunc);
  stp_list_set_long_namefunc(papersize_list, stpi_papersize_long_namefunc);
  return (stp_papersize_list_t *) papersize_list;
}

int
stpi_papersize_create(stp_papersize_list_t *list, stp_papersize_t *p)
{
  stp_list_item_t *papersize_item;

  /*
   * Check the paper does not already exist
   * Not the most efficient way of doing it, but the number of papers
   * is not large enough to be a significant bottleneck.
   */
  papersize_item = stp_list_get_start(list);
  while (papersize_item)
    {
      const stp_papersize_t *ep =
	(const stp_papersize_t *) stp_list_item_get_data(papersize_item);
      if (ep && !strcmp(p->name, ep->name))
	{
	  stp_erprintf("Duplicate paper size `%s'\n", p->name);
	  stpi_papersize_freefunc(p);
	  return 1;
	}
      papersize_item = stp_list_item_next(papersize_item);
    }

  /* Add paper to list */
  stp_list_item_create(list, NULL, (void *) p);

  return 0;
}

int
stpi_papersize_count(const stp_papersize_list_t *paper_size_list)
{
  return stp_list_get_length(paper_size_list);
}

const stp_papersize_t *
stpi_get_papersize_by_name(const stp_papersize_list_t *list, const char *name)
{
  stp_list_item_t *paper;

  paper = stp_list_get_item_by_name(list, name);
  if (!paper)
    return NULL;
  else
    return (const stp_papersize_t *) stp_list_item_get_data(paper);
}

const stp_papersize_t *
stpi_get_listed_papersize(const char *name, const char *papersize_list)
{
  const stp_papersize_list_t *list =
    stpi_get_papersize_list_named(papersize_list, "");
  if (list)
    return stpi_get_papersize_by_name(list, name);
  else
    return NULL;
}

const stp_papersize_t *
stpi_standard_describe_papersize(const stp_vars_t *v, const char *name)
{
  STPI_ASSERT(v, NULL);
  return stpi_get_listed_papersize(name, "standard");
}

const stp_papersize_t *
stp_describe_papersize(const stp_vars_t *v, const char *name)
{
  return stpi_printer_describe_papersize(v, name);
}

static int
papersize_size_mismatch(stp_dimension_t l, stp_dimension_t w,
			const stp_papersize_t *val)
{
  stp_dimension_t hdiff = STP_DABS(l - (stp_dimension_t) val->height);
  stp_dimension_t vdiff = STP_DABS(w - (stp_dimension_t) val->width);
  return hdiff > vdiff ? hdiff : vdiff;
}

static const stp_papersize_t *
get_papersize_by_size_internal(const stp_papersize_list_t *list,
			       stp_dimension_t l, stp_dimension_t w,
			       int exact)
{
  int score = INT_MAX;
  const stp_papersize_t *ref = NULL;
  const stp_papersize_t *val = NULL;
  const stp_papersize_list_item_t *ptli =
    stpi_papersize_list_get_start(list);
  STPI_ASSERT(list, NULL);
  while (ptli)
    {
      val = stpi_paperlist_item_get_data(ptli);

      if (val->width == w && val->height == l)
	{
	  if (val->top == 0 && val->left == 0 &&
	      val->bottom == 0 && val->right == 0)
	    return val;
	  else
	    ref = val;
	}
      else if (!exact)
	{
	  int myscore = papersize_size_mismatch(l, w, val);
	  if (myscore < score && myscore < 5)
	    {
	      ref = val;
	      score = myscore;
	    }
	}
      ptli = stpi_paperlist_item_next(ptli);
    }
  return ref;
}

const stp_papersize_t *
stpi_get_papersize_by_size(const stp_papersize_list_t *list,
			  stp_dimension_t l, stp_dimension_t w)
{
  return get_papersize_by_size_internal(list, l, w, 0);
}

const stp_papersize_t *
stpi_get_papersize_by_size_exact(const stp_papersize_list_t *list,
				stp_dimension_t l, stp_dimension_t w)
{
  return get_papersize_by_size_internal(list, l, w, 1);
}

void
stp_default_media_size(const stp_vars_t *v,	/* I */
		       stp_dimension_t  *width,		/* O - Width in points */
		       stp_dimension_t  *height) 	/* O - Height in points */
{
  if (stp_get_page_width(v) > 0 && stp_get_page_height(v) > 0)
    {
      *width = stp_get_page_width(v);
      *height = stp_get_page_height(v);
    }
  else
    {
      const char *page_size = stp_get_string_parameter(v, "PageSize");
      const stp_papersize_t *papersize = NULL;
      if (page_size)
	papersize = stp_describe_papersize(v, page_size);
      if (!papersize)
	{
	  *width = 1;
	  *height = 1;
	}
      else
	{
	  *width = papersize->width;
	  *height = papersize->height;
	}
      if (*width == 0)
	*width = 612;
      if (*height == 0)
	*height = 792;
    }
}

/*
 * Process the <paper> node.
 */
static stp_papersize_t *
stp_xml_process_paper(stp_mxml_node_t *paper) /* The paper node */
{
  stp_mxml_node_t *prop;	/* Temporary node pointer */
  const char *stmp;		/* Temporary string */
  /* props[] (unused) is the correct tag sequence */
  /*  const char *props[] =
    {
      "name",
      "description",
      "width",
      "height",
      "left",
      "right",
      "bottom",
      "top",
      "unit",
      "type",
      NULL
      };*/
  stp_papersize_t *outpaper;   /* Generated paper */
  int
    id = 0,			/* Check id is present */
    name = 0,			/* Check name is present */
    height = 0,			/* Check height is present */
    width = 0,			/* Check width is present */
    unit = 0;			/* Check unit is present */

  if (stp_get_debug_level() & STP_DBG_XML)
    {
      stmp = stp_mxmlElementGetAttr(paper, (const char*) "name");
      stp_erprintf("stp_xml_process_paper: name: %s\n", stmp);
    }

  outpaper = stp_zalloc(sizeof(stp_papersize_t));
  if (!outpaper)
    return NULL;

  outpaper->name = stp_strdup(stp_mxmlElementGetAttr(paper, "name"));

  outpaper->top = 0;
  outpaper->left = 0;
  outpaper->bottom = 0;
  outpaper->right = 0;
  outpaper->paper_size_type = PAPERSIZE_TYPE_STANDARD;
  if (outpaper->name)
    id = 1;

  prop = paper->child;
  while(prop)
    {
      if (prop->type == STP_MXML_ELEMENT)
	{
	  const char *prop_name = prop->value.element.name;

	  if (!strcmp(prop_name, "description"))
	    {
	      outpaper->text = stp_strdup(stp_mxmlElementGetAttr(prop, "value"));
	      name = 1;
	    }
	  if (!strcmp(prop_name, "comment"))
	    outpaper->comment = stp_strdup(stp_mxmlElementGetAttr(prop, "value"));
	  if (!strcmp(prop_name, "width"))
	    {
	      stmp = stp_mxmlElementGetAttr(prop, "value");
	      if (stmp)
		{
		  outpaper->width = stp_xmlstrtodim(stmp);
		  width = 1;
		}
	    }
	  if (!strcmp(prop_name, "height"))
	    {
	      stmp = stp_mxmlElementGetAttr(prop, "value");
	      if (stmp)
		{
		  outpaper->height = stp_xmlstrtodim(stmp);
		  height = 1;
		}
	    }
	  if (!strcmp(prop_name, "left"))
	    {
	      stmp = stp_mxmlElementGetAttr(prop, "value");
	      outpaper->left = stp_xmlstrtodim(stmp);
	    }
	  if (!strcmp(prop_name, "right"))
	    {
	      stmp = stp_mxmlElementGetAttr(prop, "value");
	      outpaper->right = stp_xmlstrtodim(stmp);
	    }
	  if (!strcmp(prop_name, "bottom"))
	    {
	      stmp = stp_mxmlElementGetAttr(prop, "value");
	      outpaper->bottom = stp_xmlstrtodim(stmp);
	    }
	  if (!strcmp(prop_name, "top"))
	    {
	      stmp = stp_mxmlElementGetAttr(prop, "value");
	      outpaper->top = stp_xmlstrtodim(stmp);
	    }
	  if (!strcmp(prop_name, "unit"))
	    {
	      stmp = stp_mxmlElementGetAttr(prop, "value");
	      if (stmp)
		{
		  if (!strcmp(stmp, "english"))
		    outpaper->paper_unit = PAPERSIZE_ENGLISH_STANDARD;
		  else if (!strcmp(stmp, "english-extended"))
		    outpaper->paper_unit = PAPERSIZE_ENGLISH_EXTENDED;
		  else if (!strcmp(stmp, "metric"))
		    outpaper->paper_unit = PAPERSIZE_METRIC_STANDARD;
		  else if (!strcmp(stmp, "metric-extended"))
		    outpaper->paper_unit = PAPERSIZE_METRIC_EXTENDED;
		  /* Default unit */
		  else
		    outpaper->paper_unit = PAPERSIZE_METRIC_EXTENDED;
		  unit = 1;
		}
	    }
	  if (!strcmp(prop_name, "type"))
	    {
	      stmp = stp_mxmlElementGetAttr(prop, "value");
	      if (stmp)
		{
		  if (!strcmp(stmp, "envelope"))
		    outpaper->paper_size_type = PAPERSIZE_TYPE_ENVELOPE;
		  else
		    outpaper->paper_size_type = PAPERSIZE_TYPE_STANDARD;
		}
	    }
	}
      prop = prop->next;
    }
  if (id && name && width && height && unit) /* Margins and type are optional */
    return outpaper;
  stp_free(outpaper);
  outpaper = NULL;
  return NULL;
}

/*
 * Parse the <paperdef> node.
 */
static int
stp_xml_process_papersize_def(stp_mxml_node_t *paperdef, const char *file,
			      stp_papersize_list_t *papersize_list)
{
  stp_mxml_node_t *paper;                           /* paper node pointer */
  stp_papersize_t *outpaper;         /* Generated paper */

  paper = paperdef->child;
  while (paper)
    {
      if (paper->type == STP_MXML_ELEMENT)
	{
	  const char *papersize_name = paper->value.element.name;
	  if (!strcmp(papersize_name, "paper"))
	    {
	      outpaper = stp_xml_process_paper(paper);
	      if (outpaper)
		stpi_papersize_create(papersize_list, outpaper);
	    }
	}
      paper = paper->next;
    }
  return 1;
}

const stp_papersize_list_t *
stpi_get_papersize_list_named(const char *name, const char *file)
{
  stp_list_item_t *item;
  papersize_list_impl_t *impl;

  check_list_of_papersize_lists();
  item = stp_list_get_item_by_name(list_of_papersize_lists, name);
  if (item)
    {
      impl = (papersize_list_impl_t *) stp_list_item_get_data(item);
    }
  else
    {
      char buf[MAXPATHLEN+1];
      stp_deprintf(STP_DBG_PAPER, "Loading paper list %s from %s\n",
		   name, file ? file : "(null)");
      if (! file)
	return NULL;
      else if (!strcmp(file, ""))
	(void) snprintf(buf, MAXPATHLEN, "papers/%s.xml", name);
      else
	strncpy(buf, file, MAXPATHLEN);
      stp_mxml_node_t *node =
	stp_xml_parse_file_from_path_safe(buf, "paperdef", NULL);
      const char *stmp = stp_mxmlElementGetAttr(node, "name");
      STPI_ASSERT(stmp && !strcmp(name, stmp), NULL);
      impl = stp_malloc(sizeof(papersize_list_impl_t));
      impl->name = stp_strdup(name);
      impl->list = stpi_create_papersize_list();
      stp_deprintf(STP_DBG_PAPER, "    Loading %s\n", stmp);
      stp_list_item_create(list_of_papersize_lists, NULL, impl);
      stp_xml_process_papersize_def(node, buf, impl->list);
    }
  return impl->list;
}

stp_papersize_list_t *
stpi_find_papersize_list_named(const char *name)
{
  stp_list_item_t *item;

  check_list_of_papersize_lists();
  item = stp_list_get_item_by_name(list_of_papersize_lists, name);
  if (item)
    {
      papersize_list_impl_t *impl =
	(papersize_list_impl_t *) stp_list_item_get_data(item);
      if (impl)
	return impl->list;
    }
  return NULL;
}

stp_papersize_list_t *
stpi_new_papersize_list(const char *name)
{
  stp_list_item_t *item;
  papersize_list_impl_t *impl;

  check_list_of_papersize_lists();
  item = stp_list_get_item_by_name(list_of_papersize_lists, name);
  if (item)
    return NULL;
  impl = stp_malloc(sizeof(papersize_list_impl_t));
  impl->name = stp_strdup(name);
  impl->list = stpi_create_papersize_list();
  stp_list_item_create(list_of_papersize_lists, NULL, impl);
  return impl->list;
}

const stp_papersize_list_t *
stpi_get_standard_papersize_list(void)
{
  return stpi_get_papersize_list_named("standard", "");
}
