/*
 * "$Id$"
 *
 *   Gimp-Print color management module - traditional Gimp-Print algorithm.
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
#include "curve-cache.h"

void
stpi_curve_free_curve_cache(cached_curve_t *cache)
{
  if (cache->curve)
    stp_curve_destroy(cache->curve);
  cache->curve = NULL;
  cache->d_cache = NULL;
  cache->s_cache = NULL;
  cache->count = 0;
}

void
stpi_curve_cache_curve_data(cached_curve_t *cache)
{
  if (cache->curve && !cache->d_cache)
    {
      cache->s_cache = stp_curve_get_ushort_data(cache->curve, &(cache->count));
      cache->d_cache = stp_curve_get_data(cache->curve, &(cache->count));
    }
}

stp_curve_t
stpi_curve_cache_get_curve(cached_curve_t *cache)
{
  return cache->curve;
}

void
stpi_curve_cache_curve_invalidate(cached_curve_t *cache)
{
  cache->d_cache = NULL;
  cache->s_cache = NULL;
  cache->count = 0;
}

void
stpi_curve_cache_set_curve(cached_curve_t *cache, stp_curve_t curve)
{
  stpi_curve_cache_curve_invalidate(cache);
  cache->curve = curve;
}

void
stpi_curve_cache_set_curve_copy(cached_curve_t *cache, stp_const_curve_t curve)
{
  stpi_curve_cache_curve_invalidate(cache);
  cache->curve = stp_curve_create_copy(curve);
}

const size_t
stpi_curve_cache_get_count(cached_curve_t *cache)
{
  if (cache->curve)
    {
      if (!cache->d_cache)
	cache->d_cache = stp_curve_get_data(cache->curve, &(cache->count));
      return cache->count;
    }
  else
    return 0;
}

const unsigned short *
stpi_curve_cache_get_ushort_data(cached_curve_t *cache)
{
  if (cache->curve)
    {
      if (!cache->s_cache)
	cache->s_cache =
	  stp_curve_get_ushort_data(cache->curve, &(cache->count));
      return cache->s_cache;
    }
  else
    return NULL;
}

const double *
stpi_curve_cache_get_double_data(cached_curve_t *cache)
{
  if (cache->curve)
    {
      if (!cache->d_cache)
	cache->d_cache = stp_curve_get_data(cache->curve, &(cache->count));
      return cache->d_cache;
    }
  else
    return NULL;
}

void
stpi_curve_cache_copy(cached_curve_t *dest, const cached_curve_t *src)
{
  stpi_curve_cache_curve_invalidate(dest);
  if (dest != src)
    {
      if (src->curve)
	stpi_curve_cache_set_curve_copy(dest, src->curve);
    }
}

