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
#include <string.h>

#define COOKIE_LIST    0xbfea218e

struct stpi_internal_list_node;

typedef struct stpi_internal_list_node
{
  const void *data;                          /* data */
  struct stpi_internal_list_node *prev; /* previous node */
  struct stpi_internal_list_node *next; /* next node */
} stpi_internal_list_node_t;


typedef struct stpi_internal_list_head
{
  int cookie;			/* Magic cookie */
  int icache;                               /* index no of cached node */
  int length;                               /* number of nodes */
  struct stpi_internal_list_node *start;     /* start node */
  struct stpi_internal_list_node *end;       /* end node */
  struct stpi_internal_list_node *cache;     /* cached node */
  node_freefunc freefunc;	/* callback: free node data */
  node_copyfunc copyfunc;	/* callback: copy node */
  node_namefunc namefunc;	/* callback: get node name */
  node_namefunc long_namefunc;	/* callback: get node long name */
  node_sortfunc sortfunc;	/* callback: compare (sort) nodes */
  char *name_cache;
  struct stpi_internal_list_node *name_cache_node;
  char *long_name_cache;
  struct stpi_internal_list_node *long_name_cache_node;
} stpi_internal_list_head_t;
  
static void
set_name_cache(stpi_internal_list_head_t *lh,
	       const char *name,
	       struct stpi_internal_list_node *cache)
{
  if (lh->name_cache)
    stpi_free(lh->name_cache);
  lh->name_cache = NULL;
  if (name)
    lh->name_cache = stpi_strdup(name);
  lh->name_cache_node = cache;
}
  
static void
set_long_name_cache(stpi_internal_list_head_t *lh,
		    const char *long_name,
		    struct stpi_internal_list_node *cache)
{
  if (lh->long_name_cache)
    stpi_free(lh->long_name_cache);
  lh->long_name_cache = NULL;
  if (long_name)
    lh->long_name_cache = stpi_strdup(long_name);
  lh->long_name_cache_node = cache;
}

static inline void
clear_cache(stpi_internal_list_head_t *lh)
{
  set_name_cache(lh, NULL, NULL);
  set_long_name_cache(lh, NULL, NULL);
}

/* node free callback for node data allocated with stpi_malloc() (not
   used by default) */
void
stpi_list_node_free_data (void *item)
{
  stpi_free((void *) item);
  if (stpi_debug_level & STPI_DBG_LIST)
    stpi_erprintf("stpi_list_node_free_data destructor\n");
}

static void
null_list(void)
{
  stpi_erprintf("Null stpi_list_t! Please report this bug.\n");
  stpi_abort();
}  

static void
bad_list(void)
{
  stpi_erprintf("Bad stpi_list_t! Please report this bug.\n");
  stpi_abort();
}

static inline void
check_list(const stpi_internal_list_head_t *v)
{
  if (v == NULL)
    null_list();
  if (v->cookie != COOKIE_LIST)
    bad_list();
}

static inline stpi_list_item_t *
get_start_internal(const stpi_list_t *list)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  check_list(lh);
  return (stpi_list_item_t *) lh->start;
}

static inline stpi_list_item_t *
get_end_internal(const stpi_list_t *list)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  check_list(lh);
  return (stpi_list_item_t *) lh->end;
}

/* list head functions */

/* these functions operate on the list as a whole, and not the
   individual nodes in a list */

/* create a new list */
stpi_list_t *
stpi_list_create(void)
{
  stpi_internal_list_head_t *lh;
  lh = (stpi_internal_list_head_t *) stpi_malloc(sizeof(stpi_internal_list_head_t));

  /* initialise an empty list */
  lh->cookie = COOKIE_LIST;
  lh->icache = lh->length = 0;
  lh->start = lh->end = lh->cache = (stpi_internal_list_node_t *) NULL;
  lh->freefunc = NULL;
  lh->namefunc = NULL;
  lh->long_namefunc = NULL;
  lh->sortfunc = NULL;
  lh->copyfunc = NULL;
  lh->name_cache = NULL;
  lh->name_cache_node = NULL;
  lh->long_name_cache = NULL;
  lh->long_name_cache_node = NULL;

  if (stpi_debug_level & STPI_DBG_LIST)
    stpi_erprintf("stpi_list_head constructor\n");
  return (stpi_list_t *) lh;
}

stpi_list_t *
stpi_list_copy(const stpi_list_t *list)
{
  stpi_list_t *ret;
  node_copyfunc copyfunc = stpi_list_get_copyfunc(list);
  stpi_list_item_t *item = get_start_internal(list);

  check_list((const stpi_internal_list_head_t *) list);

  ret = stpi_list_create();
  stpi_list_set_copyfunc(ret, stpi_list_get_copyfunc(list));
  /* If we use default (shallow) copy, we can't free the elements of it */
  if (stpi_list_get_copyfunc(list))
    stpi_list_set_freefunc(ret, stpi_list_get_freefunc(list));
  stpi_list_set_namefunc(ret, stpi_list_get_namefunc(list));
  stpi_list_set_long_namefunc(ret, stpi_list_get_long_namefunc(list));
  stpi_list_set_sortfunc(ret, stpi_list_get_sortfunc(list));
  while (item)
    {
      const void *data = ((stpi_internal_list_node_t *) item)->data;
      if (copyfunc)
	stpi_list_item_create (ret, NULL, (*copyfunc)(data));
      else
	stpi_list_item_create(ret, NULL, data);
      item = stpi_list_item_next(item);
    }
  return ret;
}

/* free a list, freeing all child nodes first */
int
stpi_list_destroy(stpi_list_t *list)
{
  stpi_internal_list_node_t *cur;
  stpi_internal_list_node_t *next;
  stpi_internal_list_head_t *lh = (stpi_internal_list_head_t *) list;

  check_list(lh);
  clear_cache(lh);
  cur = (stpi_internal_list_node_t *) get_start_internal(list);
  while(cur)
    {
      next = cur->next;
      stpi_list_item_destroy(list, (stpi_list_item_t *) cur);
      cur = next;
    }
  if (stpi_debug_level & STPI_DBG_LIST)
    stpi_erprintf("stpi_list_head destructor\n");
  lh->cookie = 0;
  stpi_free(lh);

  return 0;
}

int
stpi_list_get_length(const stpi_list_t *list)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  check_list(lh);
  return lh->length;
}

/* find a node */

/* get the first node in the list */

stpi_list_item_t *
stpi_list_get_start(const stpi_list_t *list)
{
  return get_start_internal(list);
}

/* get the last node in the list */

stpi_list_item_t *
stpi_list_get_end(const stpi_list_t *list)
{
  return get_end_internal(list);
}

/* get the node by its place in the list */
stpi_list_item_t *
stpi_list_get_item_by_index(const stpi_list_t *list, int idx)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  stpi_internal_list_node_t *ln = NULL;
  int i; /* current index */
  int d = 0; /* direction of list traversal, 0=forward */
  int c = 0; /* use cache? */
  check_list(lh);

  if (idx >= lh->length)
    return NULL;

  /* see if using the cache is worthwhile */
  if (lh->icache)
    {
      if (idx < (lh->length/2))
	{
	  if (idx > abs(idx - lh->icache))
	    c = 1;
	  else
	    d = 0;
	}
      else
	{
	  if (lh->length - 1 - idx >
	      abs (lh->length - 1 - idx - lh->icache))
	    c = 1;
	  else
	    d = 1;
	}
    }


  if (c) /* use the cached index and node */
    {
      if (idx > lh->icache) /* forward */
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
	  ln = (stpi_internal_list_node_t *) get_end_internal(list);
	}
      else
	{
	  i = 0;
	  ln = (stpi_internal_list_node_t *) get_start_internal(list);
	}
    }

  while (ln && i != idx)
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
  ((stpi_internal_list_head_t *)lh)->icache = i;
  ((stpi_internal_list_head_t *)lh)->cache = ln;

  return (stpi_list_item_t *) ln;
}

static stpi_internal_list_node_t *
stpi_list_get_item_by_name_internal(const stpi_list_t *list, const char *name)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  stpi_internal_list_node_t *ln =
    (stpi_internal_list_node_t *) get_start_internal(list);
  while (ln && strcmp(name, lh->namefunc(ln->data)))
    {
      ln = ln->next;
    }
  return ln;
}

/* get the first node with name; requires a callback function to
   read data */
stpi_list_item_t *
stpi_list_get_item_by_name(const stpi_list_t *list, const char *name)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  stpi_internal_list_node_t *ln = NULL;
  check_list(lh);

  if (!lh->namefunc)
    return NULL;

  if (lh->name_cache && name && lh->name_cache_node)
    {
      const char *new_name;
      ln = lh->name_cache_node;
      /* Is this the item we've cached? */
      if (strcmp(name, lh->name_cache) == 0 &&
	  strcmp(name, lh->namefunc(ln->data)) == 0)
	return (stpi_list_item_t *) ln;
      
      /* If not, check the next item in case we're searching the list */
      ln = ln->next;
      if (ln)
	{
	  new_name = lh->namefunc((stpi_list_item_t *) ln->data);
	  if (strcmp(name, new_name) == 0)
	    {
	      set_name_cache((stpi_internal_list_head_t *) lh, new_name, ln);
	      return (stpi_list_item_t *) ln;
	    }
	}
      /* If not, check the index cache */
      ln = lh->cache;
      if (ln)
	{
	  new_name = lh->namefunc((stpi_list_item_t *) ln->data);
	  if (strcmp(name, new_name) == 0)
	    {
	      set_name_cache((stpi_internal_list_head_t *) lh, new_name, ln);
	      return (stpi_list_item_t *) ln;
	    }
	}
    }

  ln = stpi_list_get_item_by_name_internal(list, name);

  if (ln)
    set_name_cache((stpi_internal_list_head_t *) lh, name, ln);

  return (stpi_list_item_t *) ln;
}


static stpi_internal_list_node_t *
stpi_list_get_item_by_long_name_internal(const stpi_list_t *list,
					 const char *long_name)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  stpi_internal_list_node_t *ln =
    (stpi_internal_list_node_t *) get_start_internal(list);
  while (ln && strcmp(long_name, lh->long_namefunc(ln->data)))
    {
      ln = ln->next;
    }
  return ln;
}

/* get the first node with long_name; requires a callack function to
   read data */
stpi_list_item_t *
stpi_list_get_item_by_long_name(const stpi_list_t *list, const char *long_name)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  stpi_internal_list_node_t *ln = NULL;
  check_list(lh);

  if (!lh->long_namefunc)
    return NULL;

  if (lh->long_name_cache && long_name && lh->long_name_cache_node)
    {
      const char *new_long_name;
      ln = lh->long_name_cache_node;
      /* Is this the item we've cached? */
      if (strcmp(long_name, lh->long_name_cache) == 0 &&
	  strcmp(long_name, lh->long_namefunc(ln->data)) == 0)
	return (stpi_list_item_t *) ln;
      
      /* If not, check the next item in case we're searching the list */
      ln = ln->next;
      if (ln)
	{
	  new_long_name = lh->long_namefunc((stpi_list_item_t *) ln->data);
	  if (strcmp(long_name, new_long_name) == 0)
	    {
	      set_long_name_cache((stpi_internal_list_head_t *) lh,
				  new_long_name, ln);
	      return (stpi_list_item_t *) ln;
	    }
	}
      /* If not, check the index cache */
      ln = lh->cache;
      if (ln)
	{
	  new_long_name = lh->long_namefunc((stpi_list_item_t *) ln->data);
	  if (strcmp(long_name, new_long_name) == 0)
	    {
	      set_long_name_cache((stpi_internal_list_head_t *) lh,
				  new_long_name, ln);
	      return (stpi_list_item_t *) ln;
	    }
	}
    }

  ln = stpi_list_get_item_by_long_name_internal(list, long_name);

  if (ln)
    set_long_name_cache((stpi_internal_list_head_t *) lh, long_name, ln);

  return (stpi_list_item_t *) ln;
}


/* callback for freeing data */
void
stpi_list_set_freefunc(stpi_list_t *list, node_freefunc freefunc)
{
  stpi_internal_list_head_t *lh = (stpi_internal_list_head_t *) list;
  check_list(lh);
  lh->freefunc = freefunc;
}

node_freefunc
stpi_list_get_freefunc(const stpi_list_t *list)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  check_list(lh);
  return lh->freefunc;
}

/* callback for copying data */
void
stpi_list_set_copyfunc(stpi_list_t *list, node_copyfunc copyfunc)
{
  stpi_internal_list_head_t *lh = (stpi_internal_list_head_t *) list;
  check_list(lh);
  lh->copyfunc = copyfunc;
}

node_copyfunc
stpi_list_get_copyfunc(const stpi_list_t *list)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  check_list(lh);
  return lh->copyfunc;
}

/* callback for getting data name */
void
stpi_list_set_namefunc(stpi_list_t *list, node_namefunc namefunc)
{
  stpi_internal_list_head_t *lh = (stpi_internal_list_head_t *) list;
  check_list(lh);
  lh->namefunc = namefunc;
}

node_namefunc
stpi_list_get_namefunc(const stpi_list_t *list)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  check_list(lh);
  return lh->namefunc;
}

/* callback for getting data long_name */
void
stpi_list_set_long_namefunc(stpi_list_t *list, node_namefunc long_namefunc)
{
  stpi_internal_list_head_t *lh = (stpi_internal_list_head_t *) list;
  check_list(lh);
  lh->long_namefunc = long_namefunc;
}

node_namefunc
stpi_list_get_long_namefunc(const stpi_list_t *list)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  check_list(lh);
  return lh->long_namefunc;
}

/* callback for sorting nodes */
void
stpi_list_set_sortfunc(stpi_list_t *list, node_sortfunc sortfunc)
{
  stpi_internal_list_head_t *lh = (stpi_internal_list_head_t *) list;
  check_list(lh);
  lh->sortfunc = sortfunc;
}

node_sortfunc
stpi_list_get_sortfunc(const stpi_list_t *list)
{
  const stpi_internal_list_head_t *lh =
    (const stpi_internal_list_head_t *) list;
  check_list(lh);
  return lh->sortfunc;
}


/* list item functions */

/* these functions operate on individual nodes in a list */

/*
 * create a new node in list, before next (may be null e.g. if sorting
 * next is calculated automatically, else defaults to end).  Must be
 * initialised with data (null nodes are disallowed).  The
 * stpi_list_item_t type can not exist unless it is associated with an
 * stpi_list_t list head.
 */
int
stpi_list_item_create(stpi_list_t *list,
		      const stpi_list_item_t *next,
		      const void *data)
{
  stpi_internal_list_node_t *ln; /* list node to add */
  stpi_internal_list_node_t *lnn; /* list node next */
  stpi_internal_list_head_t *lh = (stpi_internal_list_head_t *) list;
  check_list(lh);

  clear_cache(lh);

  ln = stpi_malloc(sizeof(stpi_internal_list_node_t));
  ln->prev = ln->next = NULL;

  if (data)
    ln->data = data;
  else
    {
      stpi_free(ln);
      return 1;
    }

  if (lh->sortfunc)
    {
      /* set np to the previous node (before the insertion */
      lnn = (stpi_internal_list_node_t *) get_end_internal(list);
      while (lnn)
	{
	  if (lh->sortfunc(lnn->data, ln->data) <= 0)
	    break;
	  lnn = lnn->prev;
	}
    }
  else if (stpi_debug_level & STPI_DBG_LIST)
    {
      if (next)
	{
	  lnn = (stpi_internal_list_node_t *) get_start_internal(list);
	  while (lnn)
	    {
	      if (lnn == (stpi_internal_list_node_t *) next)
		break;
	      lnn = lnn->prev;
	    }
	}
      else
	lnn = (stpi_internal_list_node_t *) NULL;
    }
  else
    lnn = (stpi_internal_list_node_t *) next;

  /* got lnp; now insert the new ln */

  /* set next */
  ln->next = lnn;

  if (!ln->prev) /* insert at start of list */
    {
      if (lh->start) /* list not empty */
	ln->prev = lh->end;
      else
	lh->start = ln;
      lh->end = ln;
    }

  /* set prev (already set if at start of list) */

  if (!ln->prev && ln->next) /* insert at end of list */
    ln->prev = ln->next->prev;

  if (lh->start == ln->next) /* prev was old end */
    {
      lh->start = ln;
    }

  /* set next->prev */
  if (ln->next)
    ln->next->prev = ln;

  /* set prev->next */
  if (ln->prev)
    ln->prev->next = ln;

  /* increment reference count */
  lh->length++;

  if (stpi_debug_level & STPI_DBG_LIST)
    stpi_erprintf("stpi_list_node constructor\n");
  return 0;
}

/* remove a node from list */
int
stpi_list_item_destroy(stpi_list_t *list, stpi_list_item_t *item)
{
  stpi_internal_list_node_t *ln;
  stpi_internal_list_head_t *lh = (stpi_internal_list_head_t *) list;
  check_list(lh);
  ln = (stpi_internal_list_node_t *) item;

  clear_cache(lh);
  /* decrement reference count */
  lh->length--;

  if (lh->freefunc)
    lh->freefunc(((stpi_internal_list_node_t *) item)->data);
  if (ln->prev)
    ln->prev->next = ln->next;
  else
    lh->start = ln->next;
  if (ln->next)
    ln->next->prev = ln->prev;
  else
    lh->end = ln->prev;
  stpi_free(ln);

  if (stpi_debug_level & STPI_DBG_LIST)
    stpi_erprintf("stpi_list_node destructor\n");
  return 0;
}

/* get previous node */
stpi_list_item_t *
stpi_list_item_prev(const stpi_list_item_t *item)
{
  const stpi_internal_list_node_t *ln =
    (const stpi_internal_list_node_t *) item;
  return (stpi_list_item_t *) ln->prev;
}

/* get next node */
stpi_list_item_t *
stpi_list_item_next(const stpi_list_item_t *item)
{
  const stpi_internal_list_node_t *ln =
    (const stpi_internal_list_node_t *) item;
  return (stpi_list_item_t *) ln->next;
}

/* get data for node */
void *
stpi_list_item_get_data(const stpi_list_item_t *item)
{
  const stpi_internal_list_node_t *ln =
    (const stpi_internal_list_node_t *) item;
  return (void *) (ln->data);
}

/* set data for node */
int
stpi_list_item_set_data(stpi_list_item_t *item, void *data)
{
  stpi_internal_list_node_t *ln = (stpi_internal_list_node_t *) item;
  if (data)
    {
      ln->data = data;
      return 0;
    }
  return 1; /* return error if data was NULL */
}
