/*
 *   Object cache for Gutenprint
 *
 *   Copyright 2017 Robert Krawitz (rlk@alum.mit.edu)
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

#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

/*
 * Lists aren't exactly the right data structure for this...if we start
 * getting into enough items for it to matter, we'll reimplement it them.
 */

static stp_list_t *global_cache_list = NULL;
static stp_string_list_t *global_cache_names = NULL;

typedef struct stp_cache
{
  char *name;
  stp_list_t *cache;
  stp_string_list_t *cache_items;
} stp_refcache_t;

typedef struct
{
  char *name;
  void *content;
} stp_refcache_item_t;

static const char *
stp_refcache_namefunc(const void *cache)
{
  return ((const stp_refcache_t *) cache)->name;
}

static void
stp_refcache_freefunc(void *item)
{
  stp_refcache_t *cache = (stp_refcache_t *) item;
  STP_SAFE_FREE(cache->name);
  stp_list_destroy(cache->cache);
  stp_string_list_destroy(cache->cache_items);
  STP_SAFE_FREE(item);
}

static const char *
stp_refcache_item_namefunc(const void *item)
{
  return ((const stp_refcache_item_t *) item)->name;
}

static void
stp_refcache_item_freefunc(void *item)
{
  stp_refcache_item_t *cache_item = (stp_refcache_item_t *) item;
  STP_SAFE_FREE(cache_item->name);
  STP_SAFE_FREE(item);
}

static void
check_stp_cache(void)
{
  if (! global_cache_list)
    {
      global_cache_list = stp_list_create();
      stp_list_set_namefunc(global_cache_list, stp_refcache_namefunc);
      stp_list_set_freefunc(global_cache_list, stp_refcache_freefunc);
      global_cache_names = stp_string_list_create();
    }
}

int
stp_refcache_create(const char *name)
{
  check_stp_cache();
  if (stp_list_get_item_by_name(global_cache_list, name))
    return 0;
  else
    {
      stp_refcache_t *cache = stp_zalloc(sizeof(stp_refcache_t));
      cache->name = stp_strdup(name);
      cache->cache = stp_list_create();
      cache->cache_items = stp_string_list_create();
      stp_list_set_namefunc(cache->cache, stp_refcache_item_namefunc);
      stp_list_set_freefunc(cache->cache, stp_refcache_item_freefunc);
      stp_list_item_create(global_cache_list, NULL, cache);
      stp_string_list_add_string_unsafe(global_cache_names, name, name);
      return 1;
    }
}

static stp_refcache_t *
find_cache_named(const char *cache)
{
  check_stp_cache();
  stp_list_item_t *item = stp_list_get_item_by_name(global_cache_list, cache);
  if (item)
    return (stp_refcache_t *) stp_list_item_get_data(item);
  else
    return NULL;
}

static stp_refcache_t *
find_or_create_cache_named(const char *cache)
{
  check_stp_cache();
  stp_list_item_t *item = stp_list_get_item_by_name(global_cache_list, cache);
  if (!item)
    {
      stp_refcache_create(cache);
      item = stp_list_get_item_by_name(global_cache_list, cache);
    }
  return (stp_refcache_t *) stp_list_item_get_data(item);
}

void *
stp_refcache_find_item(const char *cache, const char *item)
{
  stp_refcache_t *cache_impl = find_cache_named(cache);
  if (cache_impl)
    {
      stp_list_item_t *item_impl =
	stp_list_get_item_by_name(cache_impl->cache, item);
      if (item_impl)
	return ((stp_refcache_item_t *)stp_list_item_get_data(item_impl))->content;
    }
  return NULL;
}

static void
add_item_to_cache(stp_refcache_t *cache, const char *item, void *data)
{
  stp_refcache_item_t *item_impl = stp_zalloc(sizeof(stp_refcache_item_t));
  item_impl->name = stp_strdup(item);
  item_impl->content = data;
  stp_list_item_create(cache->cache, NULL, item_impl);
  stp_string_list_add_string_unsafe(cache->cache_items, item, item);
}

int
stp_refcache_add_item(const char *cache, const char *item, void *data)
{
  stp_refcache_t *cache_impl = find_or_create_cache_named(cache);
  if (!stp_list_get_item_by_name(cache_impl->cache, item))
    {
      add_item_to_cache(cache_impl, item, data);
      return 1;
    }
  return 0;
}

void
stp_refcache_remove_item(const char *cache, const char *item)
{
  stp_refcache_t *cache_impl = find_cache_named(cache);
  if (cache_impl)
    {
      stp_list_item_t *item_impl =
	stp_list_get_item_by_name(cache_impl->cache, item);
      if (item_impl)
	{
	  stp_list_item_destroy(cache_impl->cache, item_impl);
	  stp_string_list_remove_string(cache_impl->cache_items, item);
	}
    }
}

void
stp_refcache_replace_item(const char *cache, const char *item, void *data)
{
  stp_refcache_t *cache_impl = find_or_create_cache_named(cache);
  stp_list_item_t *item_item =
    stp_list_get_item_by_name(cache_impl->cache, item);
  if (item_item)
    {
      stp_refcache_item_t *item_impl =
	(stp_refcache_item_t *) stp_list_item_get_data(item_item);
      item_impl->content = data;
    }
  else
    {
      add_item_to_cache(cache_impl, item, data);
    }
}

void
stp_refcache_destroy(const char *cache)
{
  check_stp_cache();
  stp_list_item_t *item = stp_list_get_item_by_name(global_cache_list, cache);
  if (item)
    {
      stp_list_item_destroy(global_cache_list, item);
      stp_string_list_remove_string(global_cache_names, cache);
    }
}

const stp_string_list_t *
stp_refcache_list_caches(void)
{
  check_stp_cache();
  return global_cache_names;
}

const stp_string_list_t *
stp_refcache_list_cache_items(const char *cache)
{
  stp_refcache_t *cache_impl = find_cache_named(cache);
  return cache_impl ? cache_impl->cache_items : NULL;
}
