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


#ifndef GIMP_PRINT_CURVE_H
#define GIMP_PRINT_CURVE_H

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Curve code borrowed from GTK+, http://www.gtk.org/
 */

typedef void *stp_curve_t;

typedef enum
{
  STP_CURVE_TYPE_LINEAR,       /* linear interpolation */
  STP_CURVE_TYPE_SPLINE        /* spline interpolation */
} stp_curve_type_t;

typedef enum
{
  STP_CURVE_WRAP_NONE,		/* curve doesn't wrap */
  STP_CURVE_WRAP_AROUND		/* curve wraps to its starting point */
} stp_curve_wrap_mode_t;

typedef enum
{
  STP_CURVE_COMPOSE_ADD,
  STP_CURVE_COMPOSE_MULTIPLY,
  STP_CURVE_COMPOSE_EXPONENTIATE
} stp_curve_compose_t;

typedef enum
{
  STP_CURVE_BOUNDS_RESCALE,	/* Rescale the bounds */
  STP_CURVE_BOUNDS_CLIP,	/* Clip the curve to the existing bounds */
  STP_CURVE_BOUNDS_ERROR	/* Error if bounds are violated */
} stp_curve_bounds_t;


/*
 * Create a new curve.  Curves have y=lower..upper.
 * Default bounds are 0..1.  Default interpolation type is
 * linear.  There are no points allocated, and the gamma is defaulted
 * to 1.
 *
 * A wrapped curve has the same value at x=0 and x=1.
 * The wrap mode of a curve cannot be changed except by routines that
 * destroy the old curve entirely (e. g. stp_curve_copy, stp_curve_read).
 */

extern stp_curve_t stp_curve_create(stp_curve_wrap_mode_t wrap);
extern stp_curve_t stp_curve_create_copy(const stp_curve_t curve);
extern void stp_curve_copy(stp_curve_t dest, const stp_curve_t source);
extern void stp_curve_free(stp_curve_t curve);

/*
 * Set the lower and upper bounds on a curve.  If any existing points
 * on the curve are outside of the bounds, FALSE is returned.
 * To change the bounds adjusting data as required, use stp_curve_rescale.
 */
extern int stp_curve_set_bounds(stp_curve_t curve, double low, double high);
extern void stp_curve_get_bounds(const stp_curve_t curve,
				 double *low, double *high);

extern stp_curve_wrap_mode_t stp_curve_get_wrap(const stp_curve_t curve);

/*
 * Get the lowest and highest value of points in the curve.
 * This does not account for any interpolation that may place
 * intermediate points outside of the curve.
 */
extern void stp_curve_get_range(const stp_curve_t curve,
				double *low, double *high);

/*
 * Return the number of allocated points in the curve.
 */
extern size_t stp_curve_count_points(const stp_curve_t curve);

extern int stp_curve_set_interpolation_type(stp_curve_t curve,
					    stp_curve_type_t itype);
extern stp_curve_type_t stp_curve_get_interpolation_type(const stp_curve_t curve);

/*
 * Set all data points of the curve.  If any of the data points fall outside
 * the bounds, the operation is not performed and FALSE is returned.  Count
 * (number of points) must be at least two.
 *
 * The number of points must not exceed 1048576.
 */
extern int stp_curve_set_data(stp_curve_t curve, size_t count,
			      const double *data);


extern int stp_curve_set_float_data(stp_curve_t curve,
				    size_t count, const float *data);
extern int stp_curve_set_long_data(stp_curve_t curve,
				   size_t count, const long *data);
extern int stp_curve_set_ulong_data(stp_curve_t curve,
				    size_t count, const unsigned long *data);
extern int stp_curve_set_int_data(stp_curve_t curve,
				  size_t count, const int *data);
extern int stp_curve_set_uint_data(stp_curve_t curve,
				   size_t count, const unsigned int *data);
extern int stp_curve_set_short_data(stp_curve_t curve,
				    size_t count, const short *data);
extern int stp_curve_set_ushort_data(stp_curve_t curve,
				     size_t count, const unsigned short *data);

/*
 * Return a curve containing a subrange of data from the source curve.
 * If the start or count is invalid, the returned curve will compare equal
 * to NULL (i. e. it will be a null pointer).  start + count must not exceed
 * the number of points in the curve, and count must be at least 2.
 * The return curve is a non-wrapping curve.
 */
extern stp_curve_t stp_curve_get_subrange(const stp_curve_t curve,
					  size_t start, size_t count);

/*
 * Set part of a curve to the range in the second curve.  The data in the
 * range must fit within both the bounds and the number of points in the
 * first curve.
 */
extern int stp_curve_set_subrange(stp_curve_t curve, const stp_curve_t range,
				  size_t start);

/*
 * Return a pointer to the curve's raw data.
 *
 * This data is not guaranteed to be valid beyond the next non-const
 * curve call.
 *
 * If the curve is a pure gamma curve (no associated points),
 * a null pointer is returned and the count is 0.
 */
extern const double *stp_curve_get_data(const stp_curve_t curve, size_t *count);


extern const float *stp_curve_get_float_data(const stp_curve_t curve,
					     size_t *count);
extern const long *stp_curve_get_long_data(const stp_curve_t curve,
					   size_t *count);
extern const unsigned long *stp_curve_get_ulong_data(const stp_curve_t curve,
						     size_t *count);
extern const int *stp_curve_get_int_data(const stp_curve_t curve,
					 size_t *count);
extern const unsigned int *stp_curve_get_uint_data(const stp_curve_t curve,
						   size_t *count);
extern const short *stp_curve_get_short_data(const stp_curve_t curve,
					     size_t *count);
extern const unsigned short *stp_curve_get_ushort_data(const stp_curve_t curve,
						       size_t *count);

/*
 * Get the underlying stp_sequence_t data structure which stp_curve_t
 * is derived from.  This can be used for fast access to the raw data.
 */
extern stp_sequence_t stp_curve_get_sequence(const stp_curve_t curve);

/*
 * Set the gamma of a curve.  This replaces all existing points along
 * the curve.  The bounds are set to 0..1.  If the gamma value is positive,
 * the function is increasing; if negative, the function is decreasing.
 * Count must be either 0 or at least 2.  If the count is zero, the gamma
 * of the curve is set for interpolation purposes, but points cannot be
 * assigned to.
 *
 * If the gamma value is illegal (0, infinity, or NaN), FALSE is returned.
 * It is also illegal, and FALSE is returned, to set gamma on a wrap-mode
 * curve.
 */
extern int stp_curve_set_gamma(stp_curve_t curve, double gamma);

/*
 * Return the gamma value of the curve.  A returned value of 0 indicates
 * that the curve does not have a valid gamma value.
 */
extern double stp_curve_get_gamma(const stp_curve_t curve);

/*
 * Set a point along the curve.  Return FALSE if data is outside the valid
 * bounds or 'where' is outside the number of valid points.
 *
 * This call destroys any gamma value assigned to the curve.
 */
extern int stp_curve_set_point(stp_curve_t curve, size_t where, double data);

/*
 * Get a point along the curve.  If 'where' is outside of the number of valid
 * points, FALSE is returned and the value of data is not changed.
 */
extern int stp_curve_get_point(const stp_curve_t curve, size_t where,
			       double *data);

/*
 * Interpolate a point along the curve.  If 'where' is less than 0 or
 * greater than the number of points, an error is returned.  If
 * interpolation would produce a value outside of the allowed range
 * (as could happen with spline interpolation), the value is clipped
 * to the range.
 */
extern int stp_curve_interpolate_value(const stp_curve_t curve,
				       double where, double *result);

/*
 * Resample a curve (change the number of points).  Returns FALSE if the
 * number of points is invalid (less than two, except that zero points is
 * permitted for a gamma curve).  This does not destroy the gamma
 * value of a curve.  Points are interpolated as required; any interpolation
 * that would place points outside of the bounds of the curve will be clipped
 * to the bounds.
 *
 * The number of points must not exceed 1048576.
 */
extern int stp_curve_resample(stp_curve_t curve, size_t points);

/*
 * Rescale a curve (multiply all points by a scaling constant).  This
 * also rescales the bounds.  Returns false if this would exceed floating
 * point limits.
 *
 * Note that this currently destroys the gamma property of the curve.
 */
extern int stp_curve_rescale(stp_curve_t curve, double scale,
			     stp_curve_compose_t mode,
			     stp_curve_bounds_t bounds_mode);

/*
 * Print and read back the value of a curve.  The printable
 * representation is guaranteed to contain only 7-bit printable ASCII
 * characters, and is null-terminated.  The curve will not contain any
 * space, newline, or comma characters.  Furthermore, a printed curve
 * will be read back correctly in all locales.
 *
 * Return of a NULL pointer indicates an error occurred.  This should
 * never happen unless the curve has been overwritten with invalid
 * data.
 *
 * These calls are not guaranteed to provide more than 6 decimal places
 * of precision or +/-0.5e-6 accuracy, whichever is less.
 *
 * The printable representation is allocated on the heap; the caller must
 * free it.
 *
 * NOTE that these calls are not thread-safe!  These routines may
 * manipulate the locale to achieve a safe representation.
 */
extern char *stp_curve_print_string(const stp_curve_t curve);
extern void stp_curve_print(FILE *f, const stp_curve_t curve);

/*
 * If reading the curve fails, the existing curve will not be changed.
 *
 * stp_curve_read_string returns the number of bytes read.  If that
 * number is -1, the read failed.
 */
extern int stp_curve_read(FILE *f, stp_curve_t curve);
extern stp_curve_t stp_curve_create_read(FILE *f);
extern int stp_curve_read_string(const char *text, stp_curve_t curve);
extern stp_curve_t stp_curve_create_read_string(const char *text);

/*
 * Compose two curves, creating a third curve.  Only add and multiply
 * composition is currently supported.
 *
 * If element-wise composition fails, an error is returned.
 *
 * If the number of points requested in the output is -1, the resulting
 * number of points will be the least common multiplier of the number of
 * points in the input and output curves.
 *
 * The number of points must be at least two, unless the curve is a gamma
 * curve and the operation chosen is multiplication or division.
 *
 * If both curves are gamma curves with the same sign, and the
 * operation is multiplication or division, the returned curve is a
 * gamma curve with the appropriate number of points.
 *
 * Both curves must have the same wraparound type.
 *
 * A new curve is created and returned.
 *
 * The number of points must not exceed 1048576;
 * if the number of points requested is -1, the number of points will not
 * be allowed to exceed 1048576.
 */

extern int stp_curve_compose(stp_curve_t *retval,
			     const stp_curve_t a, const stp_curve_t b,
			     stp_curve_compose_t mode, int points);

/* Include curve read/write functions */

#include <gimp-print/xml.h>


#endif /* GIMP_PRINT_CURVE_H */
/*
 * End of "$Id$".
 */
