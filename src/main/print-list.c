/*
 * "$Id$"
 *
 *   libgimpprint list functions.  A doubly-linked list
 *   implementation, with callbacks for freeing, sorting, and
 *   retrieving nodes by name or long name.
 *
 *   Copyright 2002 Roger Leigh (roger@whinlatter.uklinux.net)
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
 * compile on generic platforms that don't support glib, gimp, gimpprint, etc.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <stdlib.h>
#include <stdio.h>
#include "list.h"
#include <string.h>

/*
 * Uncomment to enable debugging and strict list integrity checking.
 * This may have a significant performance impact.
 */
/*#define STP_LIST_DEBUG 1*/


/* node free callback for node data allocated with stp_malloc() (not
   used by default) */
void
stp_list_node_free_data (stp_list_item_t *item)
{
  stp_internal_list_node_t *ln = (stp_internal_list_node_t *) item;
  stp_free(ln->data);
#ifdef STP_LIST_DEBUG
  fprintf(stderr, "stp_list_node_free_data destructor\n");
#endif
}


/* list head functions */

/* these functions operate on the list as a whole, and not the
   individual nodes in a list */

/* create a new list */
stp_list_t *
stp_list_create(void)
{
  stp_internal_list_head_t *lh;
  lh = (stp_internal_list_head_t *) stp_malloc(sizeof(stp_internal_list_head_t));

  /* initialise an empty list */
  lh->icache = lh->length = 0;
  lh->start = lh->end = lh->cache = (stp_internal_list_node_t *) NULL;
  lh->freefunc = NULL;
  lh->namefunc = NULL;
  lh->long_namefunc = NULL;
  lh->sortfunc = NULL;

#ifdef STP_LIST_DEBUG
  fprintf(stderr, "stp_list_head constructor\n");
#endif
  return (stp_list_t *) lh;
}

/* free a list, freeing all child nodes first */
int
stp_list_destroy(stp_list_t *list)
{
  stp_internal_list_node_t *cur;
  stp_internal_list_node_t *next;
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;

  cur = (stp_internal_list_node_t *) stp_list_get_start(list);
  while(cur)
    {
      next = cur->next;
      stp_list_item_destroy(list, (stp_list_item_t *) cur);
      cur = next;
    }
#ifdef STP_LIST_DEBUG
  fprintf(stderr, "stp_list_head destructor\n");
#endif
  stp_free(lh);

  return 0;
}

int
stp_list_get_length(stp_list_t *list)
{
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  return lh->length;
}

/* find a node */

/* get the first node in the list */
stp_list_item_t *
stp_list_get_start(stp_list_t *list)
{
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  return (stp_list_item_t *) lh->start;
}

/* get the last node in the list */
stp_list_item_t *
stp_list_get_end(stp_list_t *list)
{
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  return (stp_list_item_t *) lh->end;
}

/* get the node by its place in the list */
stp_list_item_t *
stp_list_get_item_by_index(stp_list_t *list, int index)
{
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  stp_internal_list_node_t *ln = NULL;
  int i; /* current index */
  int d = 0; /* direction of list traversal, 0=forward */
  int c = 0; /* use cache? */

  if (index >= lh->length)
    return NULL;

  /* see if using the cache is worthwhile */
  if (lh->icache)
    {
      if (index < (lh->length/2))
	{
	  if (index > abs(index - lh->icache))
	    c = 1;
	  else
	    d = 0;
	}
      else
	{
	  if (lh->length - 1 - index >
	      abs (lh->length - 1 - index - lh->icache))
	    c = 1;
	  else
	    d = 1;
	}
    }


  if (c) /* use the cached index and node */
    {
      if (index > lh->icache) /* forward */
	d = 0;
      else /* backward */
	d = 1;
      i = lh->icache;
      ln = lh->cache;
    }
  else /* start from one end of the list */
    {
      if (d)
	{
	  i = lh->length - 1;
	  ln = (stp_internal_list_node_t *) stp_list_get_end(list);
	}
      else
	{
	  i = 0;
	  ln = (stp_internal_list_node_t *) stp_list_get_start(list);
	}
    }

while (ln && i != index)
  {
    if (d)
      {
	i--;
	ln = ln->prev;
      }
    else
      {
	i++;
	ln = ln->next;
      }
  }

  /* update cache */
  lh->icache = i;
  lh->cache = ln;

  return (stp_list_item_t *) ln;
}

/* get the first node with name; requires a callback function to
   read data */
stp_list_item_t *
stp_list_get_item_by_name(stp_list_t *list, const char *name)
{
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  stp_internal_list_node_t *ln = NULL;

  if (!lh->namefunc)
    return NULL;

  ln = (stp_internal_list_node_t *) stp_list_get_start(list);
  while (ln && strcmp(name,
		      lh->namefunc((stp_list_item_t *) ln)))
    {
      ln = ln->next;
    }

  return (stp_list_item_t *) ln;
}

/* get the first node with long_name; requires a callack function to
   read data */
stp_list_item_t *
stp_list_get_item_by_long_name(stp_list_t *list, const char *long_name)
{
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  stp_internal_list_node_t *ln = NULL;

  if (!lh->long_namefunc)
    return NULL;

  ln = (stp_internal_list_node_t *) stp_list_get_start(list);
  while (ln && strcmp(long_name,
		      lh->long_namefunc((stp_list_item_t *) ln)))
    {
      ln = ln->next;
    }

  return (stp_list_item_t *) ln;
}


/* callback for freeing data */
void
stp_list_set_freefunc(stp_list_t *list,
		      void (*node_freefunc)(stp_list_item_t *item))
{
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  lh->freefunc = node_freefunc;
}

/* callback for getting data name */
void
stp_list_set_namefunc(stp_list_t *list,
		      const char *(*namefunc)(const stp_list_item_t *item))
{
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  lh->namefunc = namefunc;
}

/* callback for getting data long_name */
void
stp_list_set_long_namefunc(stp_list_t *list,
			   const char *(*long_namefunc)(const stp_list_item_t *item))
{
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  lh->long_namefunc = long_namefunc;
}

/* callback for sorting nodes */
void
stp_list_set_sortfunc(stp_list_t *list,
		      int (*sortfunc)(const stp_list_item_t *item1,
				      const stp_list_item_t *item2))
{
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  lh->sortfunc = sortfunc;
}


/* list item functions */

/* these functions operate on individual nodes in a list */

/*
 * create a new node in list, after prev (may be null e.g. if sorting
 * prev is calculated automatically, else defaults to end).  Must be
 * initialised with data (null nodes are disallowed).  The
 * stp_list_item_t type can not exist unless it is associated with an
 * stp_list_t list head.
 */
int
stp_list_item_create(stp_list_t *list,
		     stp_list_item_t *prev,
		     void *data)
{
  stp_internal_list_node_t *ln; /* list node to add */
  stp_internal_list_node_t *lnp; /* list node previous */
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;

  ln = stp_malloc(sizeof(stp_internal_list_node_t));
  ln->prev = ln->next = NULL;

  if (data)
    ln->data = data;
  else
    {
      stp_free(ln);
      return 1;
    }

  if (lh->sortfunc)
    {
      /* set np to the previous node (before the insertion */
      lnp = (stp_internal_list_node_t *) stp_list_get_start(list);
      while (lnp)
	{
	  if (lh->sortfunc((stp_list_item_t *) lnp,
			   (stp_list_item_t *) ln) > 0)
	    break;
	  lnp = lnp->next;
	}
    }
#ifdef STP_LIST_DEBUG
  else /* check prev exists: only use when debugging, due to overhead */
    {
      if (prev)
	{
	  lnp = (stp_internal_list_node_t *) stp_list_get_start(list);
	  while (lnp)
	    {
	      if (lnp == (stp_internal_list_node_t *) prev)
		break;
	      lnp = lnp->next;
	    }
	}
      else
	  lnp = (stp_internal_list_node_t *) NULL;
    }
#else
  lnp = (stp_internal_list_node_t *) prev;
#endif

  /* got lnp; now insert the new ln */

  /* set prev */
  ln->prev = lnp;

  if (!ln->prev) /* insert at start of list */
    {
      if (lh->start) /* list not empty */
	ln->next = lh->start;
      else
	lh->end = ln;
      lh->start = ln;
    }

  /* set next (already set if at start of list) */

  if (!ln->next && ln->prev) /* insert at end of list */
    ln->next = ln->prev->next;

  if (lh->end == ln->prev) /* prev was old end */
    {
      lh->end = ln;
    }

  /* set prev->next */
  if (ln->prev)
    ln->prev->next = ln;

  /* set next->prev */
  if (ln->next)
    ln->next->prev = ln;

  /* increment reference count */
  lh->length++;

#ifdef STP_LIST_DEBUG
  fprintf(stderr, "stp_list_node constructor\n");
#endif
  return 0;
}

/* remove a node from list */
int
stp_list_item_destroy(stp_list_t *list,
		      stp_list_item_t *item)
{
  stp_internal_list_node_t *ln;
  stp_internal_list_head_t *lh = (stp_internal_list_head_t *) list;
  ln = (stp_internal_list_node_t *) item;

  /* decrement reference count */
  lh->length--;

  if (lh->freefunc)
    lh->freefunc(item);
  if (ln->prev)
    ln->prev->next = ln->next;
  else
    lh->start = ln->next;
  if (ln->next)
    ln->next->prev = ln->prev;
  else
    lh->end = ln->prev;
  stp_free(ln);

#ifdef STP_LIST_DEBUG
  fprintf(stderr, "stp_list_node destructor\n");
#endif
  return 0;
}

/* get previous node */
stp_list_item_t *
stp_list_item_prev(stp_list_item_t *item)
{
  stp_internal_list_node_t *ln = (stp_internal_list_node_t *) item;
  return (stp_list_item_t *) ln->prev;
}

/* get next node */
stp_list_item_t *
stp_list_item_next(stp_list_item_t *item)
{
  stp_internal_list_node_t *ln = (stp_internal_list_node_t *) item;
  return (stp_list_item_t *) ln->next;
}

/* get data for node */
void *
stp_list_item_get_data(const stp_list_item_t *item)
{
  stp_internal_list_node_t *ln = (stp_internal_list_node_t *) item;
  return ln->data;
}

/* set data for node */
int
stp_list_item_set_data(stp_list_item_t *item, void *data)
{
  stp_internal_list_node_t *ln = (stp_internal_list_node_t *) item;
  if (data)
    {
      ln->data = data;
      return 0;
    }
  return 1; /* return error if data was NULL */
}
