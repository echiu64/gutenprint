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

#ifndef GIMP_PRINT_INTERNAL_CURVE_CACHE_H
#define GIMP_PRINT_INTERNAL_CURVE_CACHE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>

typedef struct
{
  stp_curve_t curve;
  const double *d_cache;
  const unsigned short *s_cache;
  size_t count;
} cached_curve_t;

extern void stpi_curve_free_curve_cache(cached_curve_t *cache);

extern void stpi_curve_cache_curve_data(cached_curve_t *cache);

extern stp_curve_t stpi_curve_cache_get_curve(cached_curve_t *cache);

extern void stpi_curve_cache_curve_invalidate(cached_curve_t *cache);

extern void stpi_curve_cache_set_curve(cached_curve_t *cache,
				       stp_curve_t curve);

extern void stpi_curve_cache_set_curve_copy(cached_curve_t *cache,
					    stp_const_curve_t curve);

extern const size_t stpi_curve_cache_get_count(cached_curve_t *cache);

extern const unsigned short *stpi_curve_cache_get_ushort_data(cached_curve_t *cache);

extern const double *stpi_curve_cache_get_double_data(cached_curve_t *cache);

extern void stpi_curve_cache_copy(cached_curve_t *dest,
				  const cached_curve_t *src);

#define CURVE_CACHE_FAST_USHORT(cache) ((cache)->s_cache)
#define CURVE_CACHE_FAST_DOUBLE(cache) ((cache)->d_cache)
#define CURVE_CACHE_FAST_COUNT(cache) ((cache)->count)

#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_CURVE_CACHE_H */
