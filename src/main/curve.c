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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "curve.h"
#include "xml.h"

#ifdef __GNUC__
#define inline __inline__
#endif


const char *stpi_curve_type_names[] =
  {
    "linear",
    "spline",
  };

const int stpi_curve_type_count =
(sizeof(stpi_curve_type_names) / sizeof(const char *));

const char *stpi_wrap_mode_names[] =
  {
    "nowrap",
    "wrap"
  };

const int stpi_wrap_mode_count =
(sizeof(stpi_wrap_mode_names) / sizeof(const char *));

/*
 * We could do more sanity checks here if we want.
 */
static void
check_curve(const stpi_internal_curve_t *v)
{
  if (v == NULL)
    {
      stpi_erprintf("Null curve! Please report this bug.\n");
      stpi_abort();
    }
  if (v->cookie != COOKIE_CURVE)
    {
      stpi_erprintf("Bad curve! Please report this bug.\n");
      stpi_abort();
    }
  if (v->seq == NULL)
    {
      stpi_erprintf("Bad curve (seq == NULL)! Please report this bug.\n");
      stpi_abort();
    }
}

/*
 * Get the total number of points in the base sequence class
 */
static size_t
get_real_point_count(const stpi_internal_curve_t *curve)
{
  return stp_sequence_get_size(curve->seq);
}

/*
 * Get the number of points used by the curve (that are visible to the
 * user).  This is the real point count, but is decreased by 1 if the
 * curve wraps around.
 */
static size_t
get_point_count(const stpi_internal_curve_t *curve)
{
  size_t count;

  count = stp_sequence_get_size(curve->seq);
  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    count -= 1;

  return count;
}

static void
invalidate_auxiliary_data(stpi_internal_curve_t *curve)
{
  if (curve->interval)
    {
      stpi_free(curve->interval);
      curve->interval = NULL;
    }
}

static void
clear_curve_data(stpi_internal_curve_t *curve)
{
  if (curve->seq)
    stp_sequence_set_size(curve->seq, 0);
  curve->recompute_interval = 0;
  invalidate_auxiliary_data(curve);
}

static void
compute_linear_deltas(stpi_internal_curve_t *curve)
{
  int i;
  size_t delta_count;
  size_t seq_point_count;
  const double *data;

  stp_sequence_get_data(curve->seq, &seq_point_count, &data);
  if (data == NULL)
    return;

  delta_count = get_real_point_count(curve);

  if (delta_count <= 1) /* No intervals can be computed */
    return;
  delta_count--; /* One less than the real point count.  Note size_t
		    is unsigned. */

  curve->interval = stpi_malloc(sizeof(double) * delta_count);
  for (i = 0; i < delta_count; i++)
    curve->interval[i] = data[i + 1] - data[i];
}

static void
compute_spline_deltas(stpi_internal_curve_t *curve)
{
  int i;
  int k;
  double *u;
  double *y2;
  const double *y;
  size_t point_count;
  size_t real_point_count;
  double sig;
  double p;

  point_count = get_point_count(curve);

  stp_sequence_get_data(curve->seq, &real_point_count, &y);
  u = stpi_malloc(sizeof(double) * real_point_count);
  y2 = stpi_malloc(sizeof(double) * real_point_count);

  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    {
      int reps = 3;
      int count = reps * real_point_count;
      double *y2a = stpi_malloc(sizeof(double) * count);
      double *ua = stpi_malloc(sizeof(double) * count);
      y2a[0] = 0.0;
      ua[0] = 0.0;
      for (i = 1; i < count - 1; i++)
	{
	  int im1 = (i - 1);
	  int ip1 = (i + 1);
	  int im1a = im1 % point_count;
	  int ia = i % point_count;
	  int ip1a = ip1 % point_count;

	  sig = (i - im1) / (ip1 - im1);
	  p = sig * y2a[im1] + 2.0;
	  y2a[i] = (sig - 1.0) / p;

	  ua[i] = y[ip1a] - 2 * y[ia] + y[im1a];
	  ua[i] = 3.0 * ua[i] - sig * ua[im1] / p;
	}
      y2a[count - 1] = 0.0;
      for (k = count - 2 ; k >= 0; k--)
	y2a[k] = y2a[k] * y2a[k + 1] + ua[k];
      memcpy(u, ua + ((reps / 2) * point_count),
	     sizeof(double) * real_point_count);
      memcpy(y2, y2a + ((reps / 2) * point_count),
	     sizeof(double) * real_point_count);
      stpi_free(y2a);
      stpi_free(ua);
    }
  else
    {
      int count = real_point_count - 1;

      y2[0] = 0;
      u[0] = 2 * (y[1] - y[0]);
      for (i = 1; i < count; i++)
	{
	  int im1 = (i - 1);
	  int ip1 = (i + 1);

	  sig = (i - im1) / (ip1 - im1);
	  p = sig * y2[im1] + 2.0;
	  y2[i] = (sig - 1.0) / p;

	  u[i] = y[ip1] - 2 * y[i] + y[im1];
	  u[i] = 3.0 * u[i] - sig * u[im1] / p;
	}

      u[count] = 2 * (y[count] - y[count - 1]);
      y2[count] = 0.0;

      u[count] = 0.0;
      for (k = real_point_count - 2; k >= 0; k--)
	y2[k] = y2[k] * y2[k + 1] + u[k];
    }

  curve->interval = y2;
  stpi_free(u);
}

/*
 * Recompute the delta values for interpolation.
 * When we actually do support spline curves, this routine will
 * compute the second derivatives for that purpose, too.
 */
static void
compute_intervals(stpi_internal_curve_t *curve)
{
  if (curve->interval)
    {
      stpi_free(curve->interval);
      curve->interval = NULL;
    }
  if (stp_sequence_get_size(curve->seq) > 0)
    {
      switch (curve->curve_type)
	{
	case STP_CURVE_TYPE_SPLINE:
	  compute_spline_deltas(curve);
	  break;
	case STP_CURVE_TYPE_LINEAR:
	  compute_linear_deltas(curve);
	  break;
	}
    }
  curve->recompute_interval = 0;
}

static int
stpi_curve_set_points(stpi_internal_curve_t *curve, size_t points)
{
  if (points < 2)
    return 0;
  if (points > curve_point_limit ||
      (curve->wrap_mode == STP_CURVE_WRAP_AROUND &&
       points > curve_point_limit - 1))
    return 0;
  clear_curve_data(curve);
  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    points++;
  if ((stp_sequence_set_size(curve->seq, points)) == 0)
    return 0;
  return 1;
}

/*
 * Create a default curve
 */
static void
stpi_curve_ctor(stpi_internal_curve_t *curve, stp_curve_wrap_mode_t wrap_mode)
{
  curve->cookie = COOKIE_CURVE;
  curve->seq = stp_sequence_create();
  stp_sequence_set_bounds(curve->seq, 0.0, 1.0);
  curve->curve_type = STP_CURVE_TYPE_LINEAR;
  curve->wrap_mode = wrap_mode;
  stpi_curve_set_points(curve, 2);
  curve->recompute_interval = 1;
  if (wrap_mode == STP_CURVE_WRAP_NONE)
    curve->gamma = 1.0;
  stp_sequence_set_point(curve->seq, 0, 0);
  stp_sequence_set_point(curve->seq, 1, 1);
}

stp_curve_t
stp_curve_create(stp_curve_wrap_mode_t wrap_mode)
{
  stpi_internal_curve_t *ret;
  if (wrap_mode != STP_CURVE_WRAP_NONE && wrap_mode != STP_CURVE_WRAP_AROUND)
    return NULL;
  ret = stpi_zalloc(sizeof(stpi_internal_curve_t));
  stpi_curve_ctor(ret, wrap_mode);
  return (stp_curve_t) ret;
}

static void
curve_dtor(stp_curve_t curve)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  clear_curve_data(icurve);
  if (icurve->seq)
    stp_sequence_destroy(icurve->seq);
  memset(icurve, 0, sizeof(stpi_internal_curve_t));
  icurve->curve_type = -1;
}

void
stp_curve_free(stp_curve_t curve)
{
  if (curve)
    {
      curve_dtor(curve);
      stpi_free(curve);
    }
}

void
stp_curve_copy(stp_curve_t dest, const stp_curve_t source)
{
  stpi_internal_curve_t *idest = (stpi_internal_curve_t *) dest;
  const stpi_internal_curve_t *isource = (const stpi_internal_curve_t *) source;
  check_curve(idest);
  check_curve(isource);
  curve_dtor(dest);
  idest->cookie = isource->cookie;
  idest->curve_type = isource->curve_type;
  idest->wrap_mode = isource->wrap_mode;
  idest->gamma = isource->gamma;
  idest->seq = stp_sequence_create_copy(isource->seq);
  idest->recompute_interval = 1;
}

stp_curve_t
stp_curve_create_copy(const stp_curve_t curve)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  stp_curve_t ret;
  check_curve(icurve);
  ret = stp_curve_create(icurve->wrap_mode);
  stp_curve_copy(ret, curve);
  return ret;
}

int
stp_curve_set_bounds(stp_curve_t curve, double low, double high)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  return stp_sequence_set_bounds(icurve->seq, low, high);
}

void
stp_curve_get_bounds(const stp_curve_t curve, double *low, double *high)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  stp_sequence_get_bounds(icurve->seq, low, high);
}

/*
 * Find the minimum and maximum points on the curve.  This does not
 * attempt to find the minimum and maximum interpolations; with cubic
 * splines these could exceed the boundaries.  That's OK; the interpolation
 * code will clip them to the bounds.
 */
void
stp_curve_get_range(const stp_curve_t curve, double *low, double *high)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  stp_sequence_get_range(icurve->seq, low, high);
}

size_t
stp_curve_count_points(const stp_curve_t curve)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  return get_point_count(icurve);
}

stp_curve_wrap_mode_t
stp_curve_get_wrap(const stp_curve_t curve)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  return icurve->wrap_mode;
}

int
stp_curve_set_interpolation_type(stp_curve_t curve, stp_curve_type_t itype)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  if (itype < 0 || itype >= stpi_curve_type_count)
    return 0;
  icurve->curve_type = itype;
  return 1;
}

stp_curve_type_t
stp_curve_get_interpolation_type(const stp_curve_t curve)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  return icurve->curve_type;
}

int
stp_curve_set_gamma(stp_curve_t curve, double gamma)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  if (icurve->wrap_mode || ! finite(gamma) || gamma == 0.0)
    return 0;
  clear_curve_data(icurve);
  icurve->gamma = gamma;
  stp_curve_resample(curve, 2);
  return 1;
}

double
stp_curve_get_gamma(const stp_curve_t curve)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  return icurve->gamma;
}

int
stp_curve_set_data(stp_curve_t curve, size_t count, const double *data)
{
  size_t i;
  size_t real_count = count;
  double low, high;
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  if (count < 2)
    return 0;
  if (icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
    real_count++;
  if (real_count > curve_point_limit)
    return 0;

  /* Validate the data before we commit to it. */
  stp_sequence_get_bounds(icurve->seq, &low, &high);
  for (i = 0; i < count; i++)
    if (! finite(data[i]) || data[i] < low || data[i] > high)
      {
	stpi_erprintf("stp_curve_set_data: datum out of bounds: "
		      "%g (require %g <= x <= %g), n = %d\n",
		      data[i], low, high, i);
	return 0;
      }
  /* Allocate sequence; also accounts for WRAP_MODE */
  stpi_curve_set_points(icurve, count);
  icurve->gamma = 0.0;
  stp_sequence_set_subrange(icurve->seq, 0, count, data);

  if (icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
    stp_sequence_set_point(icurve->seq, count, data[0]);
  icurve->recompute_interval = 1;

  return 1;
}


/*
 * Note that we return a pointer to the raw data here.
 * A lot of operations change the data vector, that's why we don't
 * guarantee it across non-const calls.
 */
const double *
stp_curve_get_data(const stp_curve_t curve, size_t *count)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  const double *ret;
  check_curve(icurve);
  stp_sequence_get_data(icurve->seq, count, &ret);
  *count = get_point_count(icurve);
  return ret;
}


/* "Overloaded" functions */

#define DEFINE_DATA_SETTER(t, name)                                        \
int                                                                        \
stp_curve_set_##name##_data(stp_curve_t curve, size_t count, const t *data) \
{                                                                          \
  double *tmp_data;                                                        \
  size_t i;                                                                \
  int status;                                                              \
  size_t real_count = count;                                               \
                                                                           \
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;         \
  check_curve(icurve);                                                     \
  if (count < 2)                                                           \
    return 0;                                                              \
  if (icurve->wrap_mode == STP_CURVE_WRAP_AROUND)                          \
    real_count++;                                                          \
  if (real_count > curve_point_limit)                                      \
    return 0;                                                              \
  tmp_data = stpi_malloc(count * sizeof(double));                          \
  for (i = 0; i < count; i++)                                              \
    tmp_data[i] = (double) data[i];                                        \
  status = stp_curve_set_data(curve, count, tmp_data);                     \
  stpi_free(tmp_data);                                                     \
  return status;                                                           \
 }

DEFINE_DATA_SETTER(float, float)
DEFINE_DATA_SETTER(long, long)
DEFINE_DATA_SETTER(unsigned long, ulong)
DEFINE_DATA_SETTER(int, int)
DEFINE_DATA_SETTER(unsigned int, uint)
DEFINE_DATA_SETTER(short, short)
DEFINE_DATA_SETTER(unsigned short, ushort)


#define DEFINE_DATA_ACCESSOR(t, name)				         \
const t *								 \
stp_curve_get_##name##_data(const stp_curve_t curve, size_t *count)      \
{									 \
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;       \
  stp_sequence_t seq;                                                    \
                                                                         \
  check_curve(icurve);						         \
  seq = stp_curve_get_sequence(curve);                                   \
  return stp_sequence_get_##name##_data(seq, count);                     \
}

DEFINE_DATA_ACCESSOR(float, float)
DEFINE_DATA_ACCESSOR(long, long)
DEFINE_DATA_ACCESSOR(unsigned long, ulong)
DEFINE_DATA_ACCESSOR(int, int)
DEFINE_DATA_ACCESSOR(unsigned int, uint)
DEFINE_DATA_ACCESSOR(short, short)
DEFINE_DATA_ACCESSOR(unsigned short, ushort)


stp_curve_t
stp_curve_get_subrange(const stp_curve_t curve, size_t start, size_t count)
{
  stp_curve_t retval;
  size_t ncount;
  double blo, bhi;
  const double *data;
  if (start + count > stp_curve_count_points(curve) || count < 2)
    return NULL;
  retval = stp_curve_create(STP_CURVE_WRAP_NONE);
  stp_curve_get_bounds(curve, &blo, &bhi);
  stp_curve_set_bounds(retval, blo, bhi);
  data = stp_curve_get_data(curve, &ncount);
  if (! stp_curve_set_data(retval, count, data + start))
    {
      stp_curve_free(retval);
      return NULL;
    }
  return retval;
}

int
stp_curve_set_subrange(stp_curve_t curve, const stp_curve_t range,
		       size_t start)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  double blo, bhi;
  double rlo, rhi;
  const double *data;
  size_t count;
  check_curve(icurve);
  if (start + stp_curve_count_points(range) > stp_curve_count_points(curve))
    return 0;
  stp_sequence_get_bounds(icurve->seq, &blo, &bhi);
  stp_sequence_get_range(icurve->seq, &rlo, &rhi);
  if (rlo < blo || rhi > bhi)
    return 0;
  stp_sequence_get_data(icurve->seq, &count, &data);
  icurve->recompute_interval = 1;
  icurve->gamma = 0.0;
  invalidate_auxiliary_data(icurve);
  stp_sequence_set_subrange(icurve->seq, start, count, data);
  return 1;
}


int
stp_curve_set_point(stp_curve_t curve, size_t where, double data)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  if (where >= get_point_count(icurve))
    return 0;
  icurve->gamma = 0.0;

  if ((stp_sequence_set_point(icurve->seq, where, data)) == 0)
    return 0;
  if (where == 0 && icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
    if ((stp_sequence_set_point(icurve->seq,
				get_point_count(icurve), data)) == 0)
      return 0;
  invalidate_auxiliary_data(icurve);
  return 1;
}

int
stp_curve_get_point(const stp_curve_t curve, size_t where, double *data)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  if (where >= get_point_count(icurve))
    return 0;
  return stp_sequence_get_point(icurve->seq, where, data);
}

stp_sequence_t
stp_curve_get_sequence(const stp_curve_t curve)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  return icurve->seq;
}

int
stp_curve_rescale(stp_curve_t curve, double scale,
		  stp_curve_compose_t mode, stp_curve_bounds_t bounds_mode)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  size_t real_point_count;
  int i;
  double nblo;
  double nbhi;

  check_curve(icurve);

  real_point_count = get_real_point_count(icurve);

  stp_sequence_get_bounds(icurve->seq, &nblo, &nbhi);
  if (bounds_mode == STP_CURVE_BOUNDS_RESCALE)
    {
      switch (mode)
	{
	case STP_CURVE_COMPOSE_ADD:
	  nblo += scale;
	  nbhi += scale;
	  break;
	case STP_CURVE_COMPOSE_MULTIPLY:
	  if (scale < 0)
	    {
	      double tmp = nblo * scale;
	      nblo = nbhi * scale;
	      nbhi = tmp;
	    }
	  else
	    {
	      nblo *= scale;
	      nbhi *= scale;
	    }
	  break;
	case STP_CURVE_COMPOSE_EXPONENTIATE:
	  if (scale == 0.0)
	    return 0;
	  if (nblo < 0)
	    return 0;
	  nblo = pow(nblo, scale);
	  nbhi = pow(nbhi, scale);
	  break;
	default:
	  return 0;
	}
    }

  if (! finite(nbhi) || ! finite(nblo))
    return 0;

  if (get_point_count(icurve))
    {
      double *tmp = stpi_malloc(sizeof(double) * real_point_count);
      size_t count;
      const double *data;
      stp_sequence_get_data(icurve->seq, &count, &data);
      for (i = 0; i < real_point_count; i++)
	{
	  switch (mode)
	    {
	    case STP_CURVE_COMPOSE_ADD:
	      tmp[i] = *(data+i) + scale;
	      break;
	    case STP_CURVE_COMPOSE_MULTIPLY:
	      tmp[i] = *(data+i) * scale;
	      break;
	    case STP_CURVE_COMPOSE_EXPONENTIATE:
	      tmp[i] = pow(*(data+i), scale);
	      break;
	    }
	  if (tmp[i] > nbhi || tmp[i] < nblo)
	    {
	      if (bounds_mode == STP_CURVE_BOUNDS_ERROR)
		{
		  stpi_free(tmp);
		  return(0);
		}
	      else if (tmp[i] > nbhi)
		tmp[i] = nbhi;
	      else
		tmp[i] = nblo;
	    }
	}
      stp_sequence_set_bounds(icurve->seq, nblo, nbhi);
      icurve->gamma = 0.0;
      stpi_curve_set_points(icurve, real_point_count);
      stp_sequence_set_subrange(icurve->seq, 0, real_point_count, tmp);
      stpi_free(tmp);
      icurve->recompute_interval = 1;
      invalidate_auxiliary_data(icurve);
    }
  return 1;
}


int
stpi_curve_check_parameters(stp_curve_t *curve, size_t points)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;  double blo, bhi;
  if (icurve->gamma && icurve->wrap_mode)
    {
#ifdef DEBUG
      fprintf(stderr, "curve sets both gamma and wrap_mode\n");
#endif
      return 0;
    }
  stp_sequence_get_bounds(icurve->seq, &blo, &bhi);
  if (blo > bhi)
    {
#ifdef DEBUG
      fprintf(stderr, "curve low bound is greater than high bound\n");
#endif
      return 0;
    }
  return 1;
}


static inline double
interpolate_gamma_internal(const stp_curve_t curve, double where)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  double gamma = icurve->gamma;
  double blo, bhi;
  size_t real_point_count;

  real_point_count = get_real_point_count(icurve);;

  if (real_point_count)
    where /= (real_point_count - 1);
  if (gamma < 0)
    {
      where = 1.0 - where;
      gamma = -gamma;
    }
  stp_sequence_get_bounds(icurve->seq, &blo, &bhi);
#ifdef DEBUG
  fprintf(stderr, "interpolate_gamma %f %f %f %f %f\n", where, gamma,
	  blo, bhi, pow(where, gamma));
#endif
  return blo + (bhi - blo) * pow(where, gamma);
}

static inline double
interpolate_point_internal(const stp_curve_t curve, double where)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  int integer = where;
  double frac = where - (double) integer;
  double bhi, blo;

  if (frac == 0.0)
    {
      double val;
      if ((stp_sequence_get_point(icurve->seq, integer, &val)) == 0)
	return HUGE_VAL; /* Infinity */
      return val;
    }
  if (icurve->recompute_interval)
    compute_intervals(icurve);
  if (icurve->curve_type == STP_CURVE_TYPE_LINEAR)
    {
      double val;
      if ((stp_sequence_get_point(icurve->seq, integer, &val)) == 0)
	return HUGE_VAL; /* Infinity */
      return val + frac * icurve->interval[integer];
    }
  else
    {
      size_t point_count;
      double a = 1.0 - frac;
      double b = frac;
      double ival, ip1val;
      double retval;
      int i = integer;
      int ip1 = integer + 1;

      point_count = get_point_count(icurve);

      if (ip1 >= point_count)
	ip1 -= point_count;
      retval = ((a * a * a - a) * icurve->interval[i] +
		(b * b * b - b) * icurve->interval[ip1]);

      if ((stp_sequence_get_point(icurve->seq, i, &ival)) == 0 ||
	  (stp_sequence_get_point(icurve->seq, ip1, &ip1val)) == 0)
	return HUGE_VAL; /* Infinity */

      retval = a * ival + b * ip1val + retval / 6;

      stp_sequence_get_bounds(icurve->seq, &blo, &bhi);
      if (retval > bhi)
	retval = bhi;
      if (retval < blo)
	retval = blo;
      return retval;
    }
}

int
stp_curve_interpolate_value(const stp_curve_t curve, double where,
			    double *result)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  size_t limit;
  check_curve(icurve);

  limit = get_real_point_count(icurve);

  if (where < 0 || where > limit)
    return 0;
  if (icurve->gamma)	/* this means a pure gamma curve */
    *result = interpolate_gamma_internal(curve, where);
  else
    *result = interpolate_point_internal(curve, where);
  return 1;
}

int
stp_curve_resample(stp_curve_t curve, size_t points)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  size_t limit = points;
  size_t old;
  size_t i;
  double *new_vec;

  check_curve(icurve);

  if (points == get_point_count(icurve) && icurve->seq)
    return 1;

  if (points < 2)
    return 1;

  if (icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
      limit++;
  if (limit > curve_point_limit)
    return 0;
  old = get_real_point_count(icurve);
  if (old)
    old--;
  if (!old)
    old = 1;

  new_vec = stpi_malloc(sizeof(double) * limit);

  /*
   * Be very careful how we calculate the location along the scale!
   * If we're not careful how we do it, we might get a small roundoff
   * error
   */
  for (i = 0; i < limit; i++)
    if (icurve->gamma)
      new_vec[i] =
	interpolate_gamma_internal(curve, ((double) i * (double) old /
					   (double) (limit - 1)));
    else
      new_vec[i] =
	interpolate_point_internal(curve, ((double) i * (double) old /
					   (double) (limit - 1)));
  stpi_curve_set_points(icurve, limit);
  stp_sequence_set_subrange(icurve->seq, 0, limit, new_vec);
  icurve->recompute_interval = 1;
  stpi_free(new_vec);
  return 1;
}

static unsigned
gcd(unsigned a, unsigned b)
{
  unsigned tmp;
  if (b > a)
    {
      tmp = a;
      a = b;
      b = tmp;
    }
  while (1)
    {
      tmp = a % b;
      if (tmp == 0)
	return b;
      a = b;
      b = tmp;
    }
}

static unsigned
lcm(unsigned a, unsigned b)
{
  if (a == b)
    return a;
  else if (a * b == 0)
    return a > b ? a : b;
  else
    {
      double rval = (double) a / gcd(a, b) * b;
      if (rval > curve_point_limit)
	return curve_point_limit;
      else
	return rval;
    }
}

static int
create_gamma_curve(stp_curve_t *retval, double lo, double hi, double gamma,
		   int points)
{
  *retval = stp_curve_create(STP_CURVE_WRAP_NONE);
  if (stp_curve_set_bounds(*retval, lo, hi) &&
      stp_curve_set_gamma(*retval, gamma) &&
      stp_curve_resample(*retval, points))
    return 1;
  stp_curve_free(*retval);
  *retval = 0;
  return 0;
}

static int
interpolate_points(const stp_curve_t a, const stp_curve_t b,
		   stp_curve_compose_t mode,
		   int points, double *tmp_data)
{
  double pa, pb;
  int i;
  size_t points_a = stp_curve_count_points(a);
  size_t points_b = stp_curve_count_points(b);
  for (i = 0; i < points; i++)
    {
      if (!stp_curve_interpolate_value
	  (a, (double) i * (points_a - 1) / (points - 1), &pa))
	{
	  stpi_erprintf("interpolate_points: interpolate curve a value failed\n");
	  return 0;
	}
      if (!stp_curve_interpolate_value
	  (b, (double) i * (points_b - 1) / (points - 1), &pb))
	{
	  stpi_erprintf("interpolate_points: interpolate curve b value failed\n");
	  return 0;
	}
      if (mode == STP_CURVE_COMPOSE_ADD)
	pa += pb;
      else
	pa *= pb;
      if (! finite(pa))
	{
	  stpi_erprintf("interpolate_points: interpolated point %lu is invalid\n",
			(unsigned long) i);
	  return 0;
	}
      tmp_data[i] = pa;
    }
  return 1;
}

int
stp_curve_compose(stp_curve_t *retval,
		  const stp_curve_t a, const stp_curve_t b,
		  stp_curve_compose_t mode, int points)
{
  stp_curve_t ret;
  double *tmp_data;
  double gamma_a = stp_curve_get_gamma(a);
  double gamma_b = stp_curve_get_gamma(b);
  unsigned points_a = stp_curve_count_points(a);
  unsigned points_b = stp_curve_count_points(b);
  double alo, ahi, blo, bhi;

  if (mode != STP_CURVE_COMPOSE_ADD && mode != STP_CURVE_COMPOSE_MULTIPLY)
    return 0;
  if (stp_curve_get_wrap(a) != stp_curve_get_wrap(b))
    return 0;
  stp_curve_get_bounds(a, &alo, &ahi);
  stp_curve_get_bounds(b, &blo, &bhi);
  if (mode == STP_CURVE_COMPOSE_MULTIPLY && (alo < 0 || blo < 0))
    return 0;

  if (stp_curve_get_wrap(a) == STP_CURVE_WRAP_AROUND)
    {
      points_a++;
      points_b++;
    }
  if (points == -1)
    {
      points = lcm(points_a, points_b);
      if (stp_curve_get_wrap(a) == STP_CURVE_WRAP_AROUND)
	points--;
    }
  if (points < 2 || points > curve_point_limit ||
      ((stp_curve_get_wrap(a) == STP_CURVE_WRAP_AROUND) &&
       points > curve_point_limit - 1))
    return 0;

  if (gamma_a && gamma_b && gamma_a * gamma_b > 0 &&
      mode == STP_CURVE_COMPOSE_MULTIPLY)
    return create_gamma_curve(retval, alo * blo, ahi * bhi, gamma_a + gamma_b,
			      points);
  tmp_data = stpi_malloc(sizeof(double) * points);
  if (!interpolate_points(a, b, mode, points, tmp_data))
    {
      stpi_free(tmp_data);
      return 0;
    }
  ret = stp_curve_create(stp_curve_get_wrap(a));
  if (mode == STP_CURVE_COMPOSE_ADD)
    {
      stp_curve_rescale(ret, (ahi - alo) + (bhi - blo),
			STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_RESCALE);
      stp_curve_rescale(ret, alo + blo,
			STP_CURVE_COMPOSE_ADD, STP_CURVE_BOUNDS_RESCALE);
    }
  else
    {
      stp_curve_rescale(ret, (ahi - alo) * (bhi - blo),
			STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_RESCALE);
      stp_curve_rescale(ret, alo * blo,
			STP_CURVE_COMPOSE_ADD, STP_CURVE_BOUNDS_RESCALE);
    }
  if (! stp_curve_set_data(ret, points, tmp_data))
    goto bad1;
  *retval = ret;
  stpi_free(tmp_data);
  return 1;
 bad1:
  stp_curve_free(ret);
  stpi_free(tmp_data);
  return 0;
}


static stp_curve_t
stp_curve_create_from_xmltree(xmlNodePtr curve)  /* The curve node */
{
  xmlChar *stmp;                          /* Temporary string */
  xmlNodePtr child;                       /* Child sequence node */
  stp_curve_t ret = NULL;                 /* Curve to return */
  stp_curve_type_t curve_type;            /* Type of curve */
  stp_curve_wrap_mode_t wrap_mode;        /* Curve wrap mode */
  double gamma;                           /* Gamma value */
  stp_sequence_t seq = NULL;              /* Sequence data */
  double low, high;                       /* Sequence bounds */

  stpi_xml_init();
  /* Get curve type */
  stmp = xmlGetProp(curve, (const xmlChar *) "type");
  if (stmp)
    {
      if (!xmlStrcmp(stmp, (const xmlChar *) "linear"))
	  curve_type = STP_CURVE_TYPE_LINEAR;
      else if (!xmlStrcmp(stmp, (const xmlChar *) "spline"))
	  curve_type = STP_CURVE_TYPE_SPLINE;
      else
	{
	  stpi_erprintf("stp_curve_create_from_xmltree: %s: \"type\" invalid\n", stmp);
	  xmlFree (stmp);
	  goto error;
	}
      xmlFree (stmp);
    }
  else
    {
      stpi_erprintf("stp_curve_create_from_xmltree: \"type\" missing\n");
      goto error;
    }
  /* Get curve wrap mode */
  stmp = xmlGetProp(curve, (const xmlChar *) "wrap");
  if (stmp)
    {
      if (!xmlStrcmp(stmp, (const xmlChar *) "nowrap"))
	wrap_mode = STP_CURVE_WRAP_NONE;
      else if (!xmlStrcmp(stmp, (const xmlChar *) "wrap"))
	{
	  wrap_mode = STP_CURVE_WRAP_AROUND;
	}
      else
	{
	  stpi_erprintf("stp_curve_create_from_xmltree: %s: \"wrap\" invalid\n", stmp);
	  xmlFree (stmp);
	  goto error;
	}
      xmlFree (stmp);
    }
  else
    {
      stpi_erprintf("stp_curve_create_from_xmltree: \"wrap\" missing\n");
      goto error;
    }
  /* Get curve gamma */
  stmp = xmlGetProp(curve, (const xmlChar *) "gamma");
  if (stmp)
    {
      gamma = stpi_xmlstrtod(stmp);
      xmlFree(stmp);
    }
  else
    {
      stpi_erprintf("stp_curve_create_from_xmltree: \"gamma\" missing\n");
      goto error;
    }
  /* If gamma is set, wrap_mode must be STP_CURVE_WRAP_NONE */
  if (gamma && wrap_mode != STP_CURVE_WRAP_NONE)
    {
      stpi_erprintf("stp_curve_create_from_xmltree: "
		    "gamma set and \"wrap\" is not STP_CURVE_WRAP_NONE\n");
      goto error;
    }

  /* Set up the curve */
  ret = stp_curve_create(wrap_mode);
  stp_curve_set_interpolation_type(ret, curve_type);

  child = curve->children;
  while (child)
    {
      if (!xmlStrcmp(child->name, (const xmlChar *) "sequence"))
	{
	  seq = stpi_sequence_create_from_xmltree(child);
	  break;
	}
      child = child->next;
    }

  if (seq == NULL)
    {
      stpi_erprintf("stp_curve_create_from_xmltree: sequence read failed\n");
      goto error;
    }

  /* Set curve bounds */
  stp_sequence_get_bounds(seq, &low, &high);
  stp_curve_set_bounds(ret, low, high);

  if (gamma)
    stp_curve_set_gamma(ret, gamma);
  else /* Not a gamma curve, so set points */
    {
      size_t seq_count;
      const double* data;

      stp_sequence_get_data(seq, &seq_count, &data);
      if (stp_curve_set_data(ret, seq_count, data) == 0)
	{
	  stpi_erprintf("stp_curve_create_from_xmltree: failed to set curve data\n");
	  goto error;
	}
    }

  if (seq)
    {
      stp_sequence_destroy(seq);
      seq = NULL;
    }

    /* Validate curve */
  if (stpi_curve_check_parameters(ret, stp_curve_count_points(ret)) == 0)
    {
      stpi_erprintf("stp_curve_create_from_xmltree: parameter check failed\n");
      goto error;
    }

  stpi_xml_exit();

  return ret;

 error:
  stpi_erprintf("stp_curve_create_from_xmltree: error during curve read\n");
  if (ret)
    stp_curve_free(ret);
  stpi_xml_exit();
  return NULL;
}


static xmlNodePtr
stp_xmltree_create_from_curve(stp_curve_t curve)  /* The curve */
{
  stp_curve_wrap_mode_t wrapmode;
  stp_curve_type_t interptype;
  double gammaval, low, high;
  stp_sequence_t seq;

  char *wrap;
  char *type;
  char *gamma;

  xmlNodePtr curvenode = NULL;
  xmlNodePtr child = NULL;

  stpi_xml_init();

  /* Get curve details */
  wrapmode = stp_curve_get_wrap(curve);
  interptype = stp_curve_get_interpolation_type(curve);
  gammaval = stp_curve_get_gamma(curve);

  if (gammaval && wrapmode != STP_CURVE_WRAP_NONE)
    {
      stpi_erprintf("stp_xmltree_create_from_curve: "
		    "curve sets gamma and wrap_mode is not STP_CURVE_WRAP_NONE\n");
      goto error;
    }

  /* Construct the allocated strings required */
  stpi_asprintf(&wrap, "%s", stpi_wrap_mode_names[wrapmode]);
  stpi_asprintf(&type, "%s", stpi_curve_type_names[interptype]);
  stpi_asprintf(&gamma, "%g", gammaval);

  curvenode = xmlNewNode(NULL, (const xmlChar *) "curve");
  (void) xmlSetProp(curvenode, (const xmlChar *) "wrap", (const xmlChar *) wrap);
  (void) xmlSetProp(curvenode, (const xmlChar *) "type", (const xmlChar *) type);
  (void) xmlSetProp(curvenode, (const xmlChar *) "gamma", (const xmlChar *) gamma);

  stpi_free(wrap);
  stpi_free(type);
  stpi_free(gamma);

  seq = stp_sequence_create();
  stp_curve_get_bounds(curve, &low, &high);
  stp_sequence_set_bounds(seq, low, high);
  if (gammaval != 0) /* A gamma curve does not require sequence data */
    {
      stp_sequence_set_size(seq, 0);
    }
  else
    {
      const double *data;
      size_t count;
      data = stp_curve_get_data(curve, &count);
      stp_sequence_set_data(seq, count, data);
    }

  child = stpi_xmltree_create_from_sequence(seq);

  if (seq)
    {
      stp_sequence_destroy(seq);
      seq = NULL;
    }

  if (child == NULL)
    {
      stpi_erprintf("stp_xmltree_create_from_curve: sequence node is NULL\n");
      goto error;
    }
  xmlAddChild(curvenode, child);

  stpi_xml_exit();

  return curvenode;

 error:
  stpi_erprintf("stp_xmltree_create_from_curve: error during xmltree creation\n");
  if (curvenode)
    xmlFreeNode(curvenode);
  if (child)
    xmlFreeNode(child);
  stpi_xml_exit();

  return NULL;
}

static xmlDocPtr
xmldoc_create_from_curve(stp_curve_t curve)
{
  xmlDocPtr xmldoc;
  xmlNodePtr rootnode;
  xmlNodePtr curvenode;

  /* Get curve details */
  curvenode = stp_xmltree_create_from_curve(curve);
  if (curvenode == NULL)
    {
      stpi_erprintf("xmldoc_create_from_curve: error creating curve node\n");
      return NULL;
    }
  /* Create the XML tree */
  xmldoc = stpi_xmldoc_create_generic();
  if (xmldoc == NULL)
    {
      stpi_erprintf("xmldoc_create_from_curve: error creating XML document\n");
      return NULL;
    }
  rootnode = xmlDocGetRootElement(xmldoc);
  if (rootnode == NULL)
    {
      xmlFreeDoc(xmldoc);
      stpi_erprintf("xmldoc_create_from_curve: error getting XML document root node\n");
      return NULL;
    }

  xmlAddChild(rootnode, curvenode);

  return xmldoc;
}

int
stp_curve_write(FILE *file, stp_curve_t curve)  /* The curve */
{
  xmlDocPtr xmldoc = NULL;
  xmlCharEncodingHandlerPtr xmlenc;
  xmlOutputBufferPtr xmlbuf;

  stpi_xml_init();

  xmldoc = xmldoc_create_from_curve(curve);
  if (xmldoc == NULL)
    {
      stpi_xml_exit();
      return 1;
    }

  /* Save the XML file */

  xmlenc = xmlGetCharEncodingHandler(XML_CHAR_ENCODING_UTF8);
  xmlbuf = xmlOutputBufferCreateFile(file, xmlenc);
  xmlSaveFormatFileTo(xmlbuf, xmldoc, "UTF-8", 1);
  /* xmlOutputBufferFlush(xmlbuf); */
  /* xmlOutputBufferClose(xmlbuf); */

  if (xmldoc)
    xmlFreeDoc(xmldoc);

  stpi_xml_exit();

  return 0;
}

char *
stp_curve_write_string(stp_curve_t curve)  /* The curve */
{
  xmlDocPtr xmldoc = NULL;
  xmlChar *output = NULL;
  char *noutput;
  int size;

  stpi_xml_init();

  xmldoc = xmldoc_create_from_curve(curve);
  if (xmldoc == NULL)
    {
      stpi_xml_exit();
      return NULL;
    }

  /* Save the XML file */

  xmlDocDumpFormatMemory(xmldoc, &output, &size, 1);

  if (xmldoc)
    xmlFreeDoc(xmldoc);

  stpi_xml_exit();

  noutput = stpi_strdup((char *)output);
  xmlFree(output);
  return noutput;
}

static stp_curve_t
xml_doc_get_curve(xmlDocPtr doc)
{
  xmlNodePtr cur;
  xmlNodePtr xmlcurve;
  stp_curve_t curve = NULL;

  if (doc == NULL )
    {
      fprintf(stderr,"xml_doc_get_curve: XML file not parsed successfully.\n");
      return NULL;
    }

  cur = xmlDocGetRootElement(doc);

  if (cur == NULL)
    {
      fprintf(stderr,"xml_doc_get_curve: empty document\n");
      xmlFreeDoc(doc);
      return NULL;
    }

  xmlcurve = stpi_xml_get_node(cur, "gimp-print", "curve", NULL);

  if (xmlcurve)
    curve = stp_curve_create_from_xmltree(xmlcurve);

  return curve;
}


stp_curve_t
stp_curve_create_from_file(const char* file)
{
  xmlDocPtr doc;   /* libXML document pointer */
  stp_curve_t curve = NULL;

  stpi_xml_init();

  if (stpi_debug_level & STPI_DBG_XML)
    stpi_erprintf("stp_curve_create_from_file: reading `%s'...\n", file);

  doc = xmlParseFile(file);

  curve = xml_doc_get_curve(doc);

  if (doc)
    xmlFreeDoc(doc);

  stpi_xml_exit();

  return curve;
}

stp_curve_t
stp_curve_create_from_string(const char* string)
{
  xmlDocPtr doc;   /* libXML document pointer */
  stp_curve_t curve = NULL;

  stpi_xml_init();

  if (stpi_debug_level & STPI_DBG_XML)
    stpi_erprintf("stp_curve_create_from_string: reading string...\n");

  doc = xmlParseMemory(string, strlen(string));

  curve = xml_doc_get_curve(doc);

  if (doc)
    xmlFreeDoc(doc);

  stpi_xml_exit();

  return curve;
}
