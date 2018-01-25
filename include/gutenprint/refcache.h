/*
 *   libgutenprint reference cache
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

/**
 * @file gutenprint/refcache.h
 * @brief A simple reference cache
 */

#ifndef GUTENPRINT_REFCACHE_H
#define GUTENPRINT_REFCACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gutenprint/string-list.h>

/**
 * Create a new object cache
 * @param name The name of the cache to create
 * @returns True if created, false if cache already exists.
 */

extern int stp_refcache_create(const char *name);

/**
 * Find an item in a cache
 * @param cache the cache to use.
 * @param item the name of the item to cache
 * @returns item if exists, NULL otherwise
 */
extern void *stp_refcache_find_item(const char *cache, const char *item);

/**
 * Add an item to a cache
 * @param cache the cache to use or create.
 * @param item the name of the item to cache
 * @param data pointer to the data item to store
 * @returns true if item added; false if already exists
 */
extern int stp_refcache_add_item(const char *cache, const char *item, void *data);

/**
 * Remove an item from cache.  The item's data is not freed.  If cache
 * or item does not exist this is a no-op.
 * @param cache the cache to use.
 * @param item the name of the item to remove from cache
 */
extern void stp_refcache_remove_item(const char *cache, const char *item);

/**
 * Replace an item's data, or add if it doesn't exist.
 * The old data (if any) is not freed.  Fails if cache does not already exist.
 * @param cache the cache to use.
 * @param item the name of the item to add/update.
 * @param data the data to be added/updated for the cached item.
 */
extern void stp_refcache_replace_item(const char *cache, const char *item,
				      void *data);

/**
 * Destroy a cache.  The individual items are not freed.  If cache does not
 * exist this is a no-op.
 * @param cache the cache to destroy.
 */
extern void stp_refcache_destroy(const char *cache);

/**
 * Return a static list of all caches in existence.
 * @returns list of caches in existence
 */
extern const stp_string_list_t *stp_refcache_list_caches(void);

/**
 * Return a static list of items in a specified cache
 * @param cache name of cache
 * @returns list of items in cache or NULL if cache does not exist
 */
extern const stp_string_list_t *stp_refcache_list_cache_items(const char *cache);

#ifdef __cplusplus
  }
#endif

#endif /* GUTENPRINT_REFCACHE_H */
