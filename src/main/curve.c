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
  curve->point_count = 0;
  curve->recompute_interval = 0;
  invalidate_auxiliary_data(curve);
}

static void
compute_linear_deltas(stpi_internal_curve_t *curve)
{
  int i;
  size_t point_count;
  const double *data;

  stp_sequence_get_data(curve->seq, &point_count, &data);
  if (data == NULL)
    return;

  point_count--; /* One less than the real point count */
  if (point_count <= 0) /* No intervals can be computed */
    return;

  curve->interval = stpi_malloc(sizeof(double) * point_count);
  for (i = 0; i < point_count; i++)
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
  size_t real_point_count;
  double sig;
  double p;

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
	  int im1a = im1 % curve->point_count;
	  int ia = i % curve->point_count;
	  int ip1a = ip1 % curve->point_count;

	  sig = (i - im1) / (ip1 - im1);
	  p = sig * y2a[im1] + 2.0;
	  y2a[i] = (sig - 1.0) / p;

	  ua[i] = y[ip1a] - 2 * y[ia] + y[im1a];
	  ua[i] = 3.0 * ua[i] - sig * ua[im1] / p;
	}
      y2a[count - 1] = 0.0;
      for (k = count - 2 ; k >= 0; k--)
	y2a[k] = y2a[k] * y2a[k + 1] + ua[k];
      memcpy(u, ua + ((reps / 2) * curve->point_count),
	     sizeof(double) * real_point_count);
      memcpy(y2, y2a + ((reps / 2) * curve->point_count),
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

int
stpi_curve_set_points(stpi_internal_curve_t *curve, size_t points)
{
  size_t real_point_count;
  if (points < 2)
    return 0;
  if (points > curve_point_limit ||
      (curve->wrap_mode == STP_CURVE_WRAP_AROUND &&
       points > curve_point_limit - 1))
    return 0;
  clear_curve_data(curve);
  curve->point_count = points;
  real_point_count = points;
  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    real_point_count++;
  if ((stp_sequence_set_size(curve->seq, real_point_count)) == 0)
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
  idest->point_count = isource->point_count;
  idest->wrap_mode = isource->wrap_mode;
  idest->gamma = isource->gamma;
  if (isource->seq)
    idest->seq = stp_sequence_create_copy(isource->seq);
  else
    idest->seq = NULL;
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
  return icurve->point_count;
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
  if (icurve->seq)
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
  int i;
  int real_count = count;
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
      return 0;
  stpi_curve_set_points(icurve, count); /* Now allocates double array, too */
  icurve->gamma = 0.0;
  /* This will free the data, if the current da size is less than
     count, but this was already set above */
  stp_sequence_set_data(icurve->seq, count, data);
  if (icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
    stp_sequence_set_point(icurve->seq, count, *(data+0));
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
  *count = icurve->point_count;
  stp_sequence_get_data(icurve->seq, count, &ret);
  return ret;
}


/* "Overloaded" functions */

#define DEFINE_DATA_SETTER(t, name)					     \
int									     \
stp_curve_set_##name##_data(stp_curve_t curve,                               \
                            size_t count, const t *data)                     \
{									     \
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;           \
  stp_sequence_t seq;                                                        \
                                                                             \
  check_curve(icurve);                                                       \
  seq = stp_curve_get_sequence(curve);                                       \
									     \
  return stp_sequence_set_##name##_data(seq, count, data);                   \
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
  icurve->gamma = 0.0;

  if ((stp_sequence_set_point(icurve->seq, where, data)) == 0)
    return 0;
  if (where == 0 && icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
    if ((stp_sequence_set_point(icurve->seq,
				icurve->point_count, data)) == 0)
      return 0;
  invalidate_auxiliary_data(icurve);
  return 1;
}

int
stp_curve_get_point(const stp_curve_t curve, size_t where, double *data)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  check_curve(icurve);
  if (where >= icurve->point_count)
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
  size_t real_point_count = stp_sequence_get_size(icurve->seq);
  int i;
  double nblo;
  double nbhi;

  check_curve(icurve);
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

  if (icurve->point_count)
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
      stp_sequence_set_bounds(icurve->seq, nbhi, nblo);
      icurve->gamma = 0.0;
      stp_sequence_set_data(icurve->seq, real_point_count, tmp);
      stpi_free(tmp);
      icurve->recompute_interval = 1;
      invalidate_auxiliary_data(icurve);
    }
  return 1;
}


int
stpi_curve_check_parameters(stpi_internal_curve_t *curve, size_t points)
{
  double blo, bhi;
  size_t real_point_count;
  if (curve->gamma && curve->wrap_mode)
    {
#ifdef DEBUG
      fprintf(stderr, "curve sets both gamma and wrap_mode\n");
#endif
      return 0;
    }
  stp_sequence_get_bounds(curve->seq, &blo, &bhi);
  if (blo > bhi)
    {
#ifdef DEBUG
      fprintf(stderr, "curve low bound is greater than high bound\n");
#endif
      return 0;
    }
  real_point_count = stp_sequence_get_size(curve->seq);
  if (points != real_point_count &&
      points != (real_point_count - 1))
    {
#ifdef DEBUG
      fprintf(stderr,
	      "curve point count (%lu) and double array size (%lu) do not match\n",
	      (unsigned long) points,
	      (unsigned long) stp_sequence_get_size(curve->seq));
#endif
      return 0;
    }
  /*  if ((stpi_curve_set_points(curve, points)) == 0)
    {
#ifdef DEBUG
      fprintf(stderr, "failed to set curve points\n");
#endif
      return 0;
      }*/
/* This now destroys sequence data, so isn't an appropriate
   check... */
  return 1;
}


static inline double
interpolate_gamma_internal(const stp_curve_t curve, double where)
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  double gamma = icurve->gamma;
  double blo, bhi;
  size_t real_point_count = stp_sequence_get_size(icurve->seq);
  if (real_point_count)
    where /= (real_point_count - 1);
  if (gamma < 0)
    {
      where = 1.0 - where;
      gamma = -gamma;
    }
  stp_sequence_get_bounds(icurve->seq, &blo, &bhi);
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
      double a = 1.0 - frac;
      double b = frac;
      double ival, ip1val;
      double retval;
      int i = integer;
      int ip1 = integer + 1;

      if (ip1 >= icurve->point_count)
	ip1 -= icurve->point_count;
      retval = ((a * a * a - a) * icurve->interval[i] +
		(b * b * b - b) * icurve->interval[ip1]);

      if ((stp_sequence_get_point(icurve->seq, i, &ival)) == 0 ||
	  (stp_sequence_get_point(icurve->seq, ip1, &ip1val)) == 0)
	return HUGE_VAL; /* Infinity */

      retval = a * ival + b * ip1val + retval / 6;

      stp_sequence_get_bounds(icurve->seq, &bhi, &blo);
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
  int limit;
  check_curve(icurve);
  limit = icurve->point_count;
  if (icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
    limit += 1;

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
  int limit = points;
  int old;
  int i;
  double *new_vec;

  check_curve(icurve);

  if (points == icurve->point_count && icurve->seq)
    return 1;

  if (points < 2)
    return 0;

  old = icurve->point_count - 1;
  if (icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
    {
      limit++;
      old++;
    }
  if (limit > curve_point_limit)
    return 0;
  if (old < 0)
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
  stpi_curve_set_points(icurve, points);
  stp_sequence_set_data(icurve->seq, limit, new_vec);
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
  unsigned points_a = stp_curve_count_points(a);
  unsigned points_b = stp_curve_count_points(b);
  for (i = 0; i < points; i++)
    {
      if (!stp_curve_interpolate_value
	  (a, (double) i * (points_a - 1) / (points - 1), &pa))
	return 0;
      if (!stp_curve_interpolate_value
	  (b, (double) i * (points_b - 1) / (points - 1), &pb))
	return 0;
      if (mode == STP_CURVE_COMPOSE_ADD)
	pa += pb;
      else
	pa *= pb;
      if (! finite(pa))
	return 0;
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
