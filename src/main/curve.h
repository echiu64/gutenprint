/*
 * "$Id$"
 *
 *   libgimpprint curve functions.
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
 * compile on generic platforms that don't support glib, gimp, gimpprint, etc.
 */

#ifndef GIMP_PRINT_INTERNAL_CURVE_H
#define GIMP_PRINT_INTERNAL_CURVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gimp-print/sequence.h>

#define COOKIE_CURVE 0x1ce0b247
static const int curve_point_limit = 1048576;


typedef struct
{
  int cookie;
  stp_curve_type_t curve_type;
  size_t point_count;           /* Number of points.  The real point
				   count is stored in the sequence */
  stp_curve_wrap_mode_t wrap_mode;
  int recompute_interval;	/* Do we need to recompute the deltas? */
  double gamma;			/* 0.0 means that the curve is not a gamma */
  stp_sequence_t seq;           /* Double array (contains the curve data) */
  double *interval;		/* We allocate an extra slot for the
				   wrap-around value. */

} stpi_internal_curve_t;


extern const char *stpi_curve_type_names[];
extern const int stpi_curve_type_count;
extern const char *stpi_wrap_mode_names[];
extern const int stpi_wrap_mode_count;

extern int stpi_curve_check_parameters(stpi_internal_curve_t *curve, size_t points);
extern int stpi_curve_set_points(stpi_internal_curve_t *curve, size_t points);

#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_CURVE_H */
