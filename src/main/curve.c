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

#define COOKIE_CURVE 0x1ce0b247

typedef struct
{
  int cookie;
  stp_curve_type_t curve_type;
  size_t point_count;
  size_t real_point_count;
  stp_curve_wrap_mode_t wrap_mode;
  int recompute_interval;
  int recompute_range;
  double gamma;
  double rlo, rhi;		/* Current range limits */
  double blo, bhi;		/* Bounds */
  double *data;			/* We allocate an extra slot for the
				   wrap-around value. */
  double *interval;		/* We allocate an extra slot for the
				   wrap-around value. */
} stp_internal_curve_t;

static const char *curve_type_names[] =
  {
    "Linear",
    "Spline",
  };

static const int curve_type_count =
(sizeof(curve_type_names) / sizeof(const char *));

static const char *wrap_mode_names[] =
  {
    "Nowrap",
    "Wrap"
  };

static const int wrap_mode_count =
(sizeof(wrap_mode_names) / sizeof(const char *));

static void
check_curve(const stp_internal_curve_t *v)
{
  if (v->cookie != COOKIE_CURVE)
    {
      stp_erprintf("Bad curve!\n");
      exit(2);
    }
}

static void
scan_curve_range(stp_internal_curve_t *curve)
{
  int i;
  curve->rlo = HUGE_VAL;
  curve->rhi = -HUGE_VAL;
  if (curve->point_count)
    for (i = 0; i < curve->point_count; i++)
      {
	if (curve->data[i] < curve->rlo)
	  curve->rlo = curve->data[i];
	if (curve->data[i] > curve->rhi)
	  curve->rhi = curve->data[i];
      }
}

static void
clear_curve_data(stp_internal_curve_t *curve)
{
  if (curve->data)
    stp_free(curve->data);
  if (curve->interval)
    stp_free(curve->interval);
  curve->data = NULL;
  curve->interval = NULL;
  curve->point_count = 0;
  curve->real_point_count = 0;
  curve->recompute_interval = 0;
}

static void
compute_intervals(stp_internal_curve_t *curve)
{
  int i;
  if (curve->interval)
    {
      stp_free(curve->interval);
      curve->interval = NULL;
    }
  if (curve->real_point_count > 0)
    {
      curve->interval =
	stp_malloc(sizeof(double) * (curve->real_point_count - 1));
      for (i = 0; i < curve->real_point_count - 1; i++)
	curve->interval[i] = curve->data[i + 1] - curve->data[i];
    }
  curve->recompute_interval = 0;
}    

static int
set_curve_points(stp_internal_curve_t *curve, size_t points)
{
  if (points < 2)
    return 0;
  if (points > 65536 ||
      (curve->wrap_mode == STP_CURVE_WRAP_AROUND && points > 65535))
    return 0;
  clear_curve_data(curve);
  curve->point_count = points;
  curve->real_point_count = points;
  if (curve->wrap_mode == STP_CURVE_WRAP_AROUND)
    curve->real_point_count++;
  return 1;
}

stp_curve_t
stp_curve_allocate(stp_curve_wrap_mode_t wrap_mode)
{
  stp_internal_curve_t *ret;
  if (wrap_mode != STP_CURVE_WRAP_NONE && wrap_mode != STP_CURVE_WRAP_AROUND)
    return NULL;
  ret = stp_zalloc(sizeof(stp_internal_curve_t));
  ret->cookie = COOKIE_CURVE;
  ret->rlo = ret->blo = 0.0;
  ret->rhi = ret->bhi = 1.0;
  ret->curve_type = STP_CURVE_TYPE_LINEAR;
  ret->wrap_mode = wrap_mode;
  set_curve_points(ret, 2);
  ret->data = stp_zalloc(ret->real_point_count * sizeof(double));
  ret->recompute_interval = 1;
  if (wrap_mode == STP_CURVE_WRAP_NONE)
    ret->gamma = 1.0;
  ret->data[0] = 0;
  ret->data[1] = 1;
  return (stp_curve_t) ret;
}

static void
curve_destroy_internal(stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  clear_curve_data(icurve);
  memset(icurve, 0, sizeof(stp_internal_curve_t));
  icurve->curve_type = -1;
}

void
stp_curve_destroy(stp_curve_t curve)
{
  curve_destroy_internal(curve);
  stp_free(curve);
}

void
stp_curve_copy(stp_curve_t dest, const stp_curve_t source)
{
  stp_internal_curve_t *idest = (stp_internal_curve_t *) dest;
  const stp_internal_curve_t *isource = (const stp_internal_curve_t *) source;
  check_curve(idest);
  check_curve(isource);
  curve_destroy_internal(dest);
  (void) memcpy(idest, isource, sizeof(stp_internal_curve_t));
  if (isource->data)
    {
      idest->data = stp_malloc(sizeof(double) * (isource->real_point_count));
      (void) memcpy(idest->data, isource->data,
		    (sizeof(double) * (isource->real_point_count)));
    }
  if (isource->interval)
    {
      idest->interval =
	stp_malloc(sizeof(double) * (isource->real_point_count - 1));
      (void) memcpy(idest->interval, isource->interval,
		    (sizeof(double) * (isource->real_point_count - 1)));
    }
}

stp_curve_t
stp_curve_allocate_copy(const stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  stp_curve_t ret;
  check_curve(icurve);
  ret = stp_curve_allocate(icurve->wrap_mode);
  stp_curve_copy(ret, curve);
  return ret;
}

int
stp_curve_set_bounds(stp_curve_t curve, double low, double high)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (low > high)
    return 0;
  if (low > icurve->blo || high < icurve->bhi)
    return 0;
  icurve->blo = low;
  icurve->bhi = high;
  return 1;
}

void
stp_curve_get_bounds(const stp_curve_t curve, double *low, double *high)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  *low = icurve->blo;
  *high = icurve->bhi;
}

void
stp_curve_get_range(const stp_curve_t curve, double *low, double *high)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (icurve->recompute_range)
    scan_curve_range(icurve);
  *low = icurve->rlo;
  *high = icurve->rhi;
}

size_t
stp_curve_count_points(const stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  return icurve->point_count;
}

stp_curve_wrap_mode_t
stp_curve_get_wrap(const stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  return icurve->wrap_mode;
}

int
stp_curve_set_interpolation_type(stp_curve_t curve, stp_curve_type_t itype)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (itype < 0 || itype >= curve_type_count)
    return 0;
  icurve->curve_type = itype;
  return 1;
}

stp_curve_type_t
stp_curve_get_interpolation_type(const stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  return icurve->curve_type;
}

int
stp_curve_set_gamma(stp_curve_t curve, size_t count, double gamma)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (icurve->wrap_mode || ! finite(gamma) || gamma == 0.0)
    return 0;
  if (count > 65536)
    return 0;
  if (icurve->data)
    clear_curve_data(icurve);
  icurve->gamma = gamma;
  stp_curve_resample(curve, count);
  return 1;
}

double
stp_curve_get_gamma(const stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  return icurve->gamma;
}

int
stp_curve_set_data(stp_curve_t curve, size_t count, const double *data)
{
  int i;
  int real_count = count;
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (count < 2)
    return 0;
  if (icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
    real_count++;
  if (real_count > 65536)
    return 0;

  for (i = 0; i < count; i++)
    if (! finite(data[i]) || data[i] < icurve->blo || data[i] > icurve->bhi)
      return 0;
  set_curve_points(icurve, count);
  icurve->data = stp_zalloc(icurve->real_point_count * sizeof(double));
  icurve->gamma = 0.0;
  memcpy(icurve->data, data, (sizeof(double) * icurve->real_point_count));
  if (icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
    icurve->data[icurve->point_count] = icurve->data[0];
  icurve->recompute_interval = 1;
  icurve->recompute_range = 1;
  return 1;
}

const double *
stp_curve_get_data(const stp_curve_t curve, size_t *count)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  *count = icurve->point_count;
  return icurve->data;
}

int
stp_curve_set_point(stp_curve_t curve, size_t where, double data)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (where >= icurve->point_count || ! finite(data) ||
      data < icurve->blo || data > icurve->bhi)
    return 0;
  if (data < icurve->rlo ||
      data > icurve->rhi ||
      icurve->data[where] == icurve->rhi ||
      icurve->data[where] == icurve->rlo)
    icurve->recompute_range = 1;
  icurve->gamma = 0.0;
  icurve->data[where] = data;
  if (where == 0 && icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
      icurve->data[icurve->point_count] = data;
  icurve->recompute_interval = 1;
  return 1;
}

int
stp_curve_get_point(const stp_curve_t curve, size_t where, double *data)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (where >= icurve->point_count)
    return 0;
  *data = icurve->data[where];
  return 1;
}

int
stp_curve_rescale(stp_curve_t curve, double scale,
		  stp_curve_compose_t mode, stp_curve_bounds_t bounds_mode)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  int i;
  double nblo;
  double nbhi;

  check_curve(icurve);
  nblo = icurve->blo;
  nbhi = icurve->bhi;
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
      double *tmp = stp_malloc(sizeof(double) * icurve->real_point_count);
      for (i = 0; i < icurve->real_point_count; i++)
	{
	  switch (mode)
	    {
	    case STP_CURVE_COMPOSE_ADD:
	      tmp[i] = icurve->data[i] + scale;
	      break;
	    case STP_CURVE_COMPOSE_MULTIPLY:
	      tmp[i] = icurve->data[i] * scale;
	      break;
	    case STP_CURVE_COMPOSE_EXPONENTIATE:
	      tmp[i] = pow(icurve->data[i], scale);
	      break;
	    }
	  if (tmp[i] > nbhi || tmp[i] < nblo)
	    {
	      if (bounds_mode == STP_CURVE_BOUNDS_ERROR)
		{
		  stp_free(tmp);
		  return(0);
		}
	      else if (tmp[i] > nbhi)
		tmp[i] = nbhi;
	      else
		tmp[i] = nblo;
	    }
	}
      icurve->bhi = nbhi;
      icurve->blo = nblo;
      icurve->gamma = 0.0;
      memcpy(icurve->data, tmp, sizeof(double) * icurve->real_point_count);
      stp_free(tmp);
      icurve->recompute_range = 1;
      icurve->recompute_interval = 1;
    }
  return 1;
}

void
stp_curve_print(FILE *f, const stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  int i;
  check_curve(icurve);
  setlocale(LC_ALL, "C");
  fprintf(f, "%s;%s ;%s ;%d;%g;%g;%g;",
	  "STP_CURVE",
	  wrap_mode_names[icurve->wrap_mode],
	  curve_type_names[icurve->curve_type],
	  icurve->point_count,
	  icurve->gamma,
	  icurve->blo,
	  icurve->bhi);
/*  if (icurve->gamma == 0 && icurve->point_count) */
    for (i = 0; i < icurve->point_count; i++)
      fprintf(f, "%g;", icurve->data[i]);
  setlocale(LC_ALL, "");
}

char *
stp_curve_print_string(const stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  int i;
  char *retval;
  int ret_size = 128;
  int cur_size = 0;
  check_curve(icurve);
  retval = stp_zalloc(ret_size);
  setlocale(LC_ALL, "C");
  while (1)
    {
      cur_size = snprintf(retval, ret_size - 1, "%s;%s ;%s ;%d;%g;%g;%g;",
			  "STP_CURVE",
			  wrap_mode_names[icurve->wrap_mode],
			  curve_type_names[icurve->curve_type],
			  icurve->point_count,
			  icurve->gamma,
			  icurve->blo,
			  icurve->bhi);
      if (cur_size < 0 || cur_size >= ret_size - 1)
	{
	  ret_size *= 2;
	  retval = stp_realloc(retval, ret_size);
	}
      else
	break;
    }
  cur_size = strlen(retval);
  if (icurve->gamma == 0 && icurve->point_count)
    for (i = 0; i < icurve->point_count; i++)
      while (1)
	{
	  int new_size = snprintf(retval + cur_size,
				  ret_size - cur_size- 1, "%g;",
				  icurve->data[i]);
	  if (new_size < 0 || new_size + cur_size >= ret_size - 1)
	    {
	      ret_size *= 2;
	      retval = stp_realloc(retval, ret_size);
	    }
	  else
	    {
	      cur_size += new_size;
	      break;
	    }
	}
  setlocale(LC_ALL, "");
  return retval;
}

int
stp_curve_read(FILE *f, stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  stp_curve_t ret;
  stp_internal_curve_t *iret;
  char curve_type_name[32];
  char wrap_mode_name[32];
  stp_curve_wrap_mode_t wrap_mode = (stp_curve_wrap_mode_t) -1;
  stp_curve_type_t curve_type = (stp_curve_type_t) -1;
  int noffset = 0;
  int i;
  int points;

  check_curve(icurve);
  fscanf(f, "STP_CURVE;%31s;%31s;%n",
	 curve_type_name,
	 wrap_mode_name,
	 &noffset);
  if (noffset == 0)
    return 0;
  noffset = 0;
  for (i = 0; i < wrap_mode_count; i++)
    {
      if (strcmp(wrap_mode_name, wrap_mode_names[i]) == 0)
	{
	  wrap_mode = i;
	  break;
	}
    }
  if (wrap_mode == (stp_curve_wrap_mode_t) -1)
    return 0;

  for (i = 0; i < curve_type_count; i++)
    {
      if (strcmp(curve_type_name, curve_type_names[i]) == 0)
	{
	  curve_type = i;
	  break;
	}
    }
  if (curve_type == (stp_curve_type_t) -1)
    return 0;

  setlocale(LC_ALL, "C");
  ret = stp_curve_allocate(wrap_mode);
  iret = (stp_internal_curve_t *) ret;
  iret->curve_type = curve_type;

  fscanf(f, "%d;%lg;%lg;%lg;%n",
	 &points,
	 &(iret->gamma),
	 &(iret->blo),
	 &(iret->bhi),
	 &noffset);
  if (noffset == 0)
    goto bad;
  if (iret->gamma && iret->wrap_mode)
    goto bad;
  if (iret->blo > iret->bhi)
    goto bad;
  if (!set_curve_points(iret, points))
    goto bad;
  if (iret->gamma)
    {
      set_curve_points(iret, 2);
      stp_curve_resample(ret, points);
    }
  else
    {
      iret->data = stp_malloc(sizeof(double) * (iret->real_point_count));
      for (i = 0; i < iret->point_count; i++)
	{
	  noffset = 0;
	  fscanf(f, "%lg;%n", &(iret->data[i]), &noffset);
	  if (noffset == 0)
	    goto bad;
	  if (! finite(iret->data[i]) || iret->data[i] < iret->blo ||
	      iret->data[i] > iret->bhi)
	    goto bad;
	}
      if (wrap_mode == STP_CURVE_WRAP_AROUND)
	iret->data[iret->point_count] = iret->data[0];
    }
  iret->recompute_interval = 1;
  iret->recompute_range = 1;
  stp_curve_copy(curve, ret);
  stp_curve_destroy(ret);
  setlocale(LC_ALL, "");
  return 1;

 bad:
  stp_curve_destroy(ret);
  setlocale(LC_ALL, "");
  return 0;
}

int
stp_curve_read_string(const char *text, stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  stp_curve_t ret;
  stp_internal_curve_t *iret;
  char curve_type_name[32];
  char wrap_mode_name[32];
  stp_curve_wrap_mode_t wrap_mode = (stp_curve_wrap_mode_t) -1;
  stp_curve_type_t curve_type = (stp_curve_type_t) -1;
  int offset = 0;
  int noffset = 0;
  int i;
  int points;

  check_curve(icurve);
  sscanf(text, "STP_CURVE;%31s ;%31s ;%n",
	 wrap_mode_name,
	 curve_type_name,
	 &noffset);
  if (noffset == 0)
    return 0;
  offset = noffset;
  noffset = 0;
  for (i = 0; i < wrap_mode_count; i++)
    {
      if (strcmp(wrap_mode_name, wrap_mode_names[i]) == 0)
	{
	  wrap_mode = i;
	  break;
	}
    }
  if (wrap_mode == (stp_curve_wrap_mode_t) -1)
    return 0;

  for (i = 0; i < curve_type_count; i++)
    {
      if (strcmp(curve_type_name, curve_type_names[i]) == 0)
	{
	  curve_type = i;
	  break;
	}
    }
  if (curve_type == (stp_curve_type_t) -1)
    return 0;

  setlocale(LC_ALL, "C");
  ret = stp_curve_allocate(wrap_mode);
  iret = (stp_internal_curve_t *) ret;
  iret->curve_type = curve_type;

  sscanf(text + offset, "%d;%lg;%lg;%lg;%n",
	 &points,
	 &(iret->gamma),
	 &(iret->blo),
	 &(iret->bhi),
	 &noffset);
  if (noffset == 0)
    {
      stp_curve_destroy(ret);
      goto bad;
    }
  if (iret->gamma && iret->wrap_mode)
    {
      stp_curve_destroy(ret);
      goto bad;
    }
  if (iret->blo > iret->bhi)
    {
      stp_curve_destroy(ret);
      goto bad;
    }
  if (!set_curve_points(iret, points))
    {
      stp_curve_destroy(ret);
      goto bad;
    }
  offset += noffset;
  noffset = 0;
  if (iret->gamma)
    {
      set_curve_points(iret, 2);
      stp_curve_resample(ret, points);
    }
  else
    {
      iret->data = stp_malloc(sizeof(double) * (iret->real_point_count));
      for (i = 0; i < iret->point_count; i++)
	{
	  noffset = 0;
	  sscanf(text + offset, "%lg;%n", &(iret->data[i]), &noffset);
	  if (noffset == 0 ||
	      ! finite(iret->data[i]) ||
	      iret->data[i] < iret->blo ||
	      iret->data[i] > iret->bhi)
	    {
	      stp_curve_destroy(ret);
	      goto bad;
	    }
	  offset += noffset;
	}
      if (wrap_mode == STP_CURVE_WRAP_AROUND)
	{
	  iret->data[iret->point_count] = iret->data[0];
	}
    }
  iret->recompute_interval = 1;
  iret->recompute_range = 1;
  stp_curve_copy(curve, ret);
  stp_curve_destroy(ret);
  setlocale(LC_ALL, "");
  return offset;

 bad:
  setlocale(LC_ALL, "");
  return 0;
}

static double
interpolate_gamma_internal(const stp_curve_t curve, double where)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  double gamma = icurve->gamma;
  if (icurve->real_point_count)
    where /= (icurve->real_point_count - 1);
  if (gamma < 0)
    {
      where = 1.0 - where;
      gamma = -gamma;
    }
  return icurve->blo + (icurve->bhi - icurve->blo) * pow(where, gamma);
}

static double
interpolate_point_internal(const stp_curve_t curve, double where)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  double finteger;
  int integer;
  double frac = modf(where, &finteger);
  integer = finteger;

  if (frac == 0.0)
    return icurve->data[integer];
  if (icurve->recompute_interval)
    compute_intervals(icurve);
  if (icurve->curve_type == STP_CURVE_TYPE_LINEAR)
    return icurve->data[integer] + frac * icurve->interval[integer];
  else
    /* Placeholder linear interpolation until we write the spline code */
    return icurve->data[integer] + frac * icurve->interval[integer];
}

int
stp_curve_interpolate_value(const stp_curve_t curve, double where,
			    double *result)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
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
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  int limit = points;
  int old;
  int i;
  double *new_vec;

  check_curve(icurve);

  if (points == icurve->point_count)
    return 1;

  if (points < 2)
    return 0;

  old = icurve->point_count - 1;
  if (icurve->wrap_mode == STP_CURVE_WRAP_AROUND)
    {
      limit++;
      old++;
    }
  if (limit > 65536)
    return 0;
  if (old < 0)
    old = 1;

  new_vec = stp_malloc(sizeof(double) * limit);

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
  set_curve_points(icurve, points);
  icurve->data = new_vec;
  icurve->recompute_interval = 1;
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

/*
 * This is limited to 65536.
 */
static unsigned
lcm(unsigned a, unsigned b)
{
  if (a == b)
    return a;
  else if (a * b == 0)
    return a > b ? a : b;
  else
    return a * b / gcd(a, b);
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
  double pa, pb;
  double nrange, ndelta;
  int i;

  stp_curve_get_bounds(a, &alo, &ahi);
  stp_curve_get_bounds(b, &blo, &bhi);

  if (stp_curve_get_wrap(a) != stp_curve_get_wrap(b))
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
  if (points > 65536 ||
      ((stp_curve_get_wrap(a) == STP_CURVE_WRAP_AROUND) && points > 65535))
    return 0;
  if (mode == STP_CURVE_COMPOSE_MULTIPLY && (alo < 0 || blo < 0))
    return 0;

  if (gamma_a && gamma_b && gamma_a * gamma_b > 0 &&
      mode == STP_CURVE_COMPOSE_MULTIPLY)
    {
      *retval = stp_curve_allocate(STP_CURVE_WRAP_NONE);
      stp_curve_set_bounds(*retval, alo * blo, ahi * bhi);
      return stp_curve_set_gamma(*retval, points, gamma_a + gamma_b);
    }
  if (points < 2)
    return 0;
  tmp_data = stp_malloc(sizeof(double) * points);
  for (i = 0; i < points; i++)
    {
      if (!stp_curve_interpolate_value(a, i * (points_a - 1) / (points - 1),
				       &pa))
	goto bad;
      if (!stp_curve_interpolate_value(b, i * (points_b - 1) / (points - 1),
				       &pb))
	goto bad;
      switch (mode)
	{
	case STP_CURVE_COMPOSE_ADD:
	  pa += pb;
	  break;
	case STP_CURVE_COMPOSE_MULTIPLY:
	  pa *= pb;
	  break;
	default:
	  goto bad;
	}
      if (! finite(pa))
	goto bad;
      tmp_data[i] = pa;
    }
  ret = stp_curve_allocate(stp_curve_get_wrap(a));
  switch (mode)
    {
    case STP_CURVE_COMPOSE_ADD:
      stp_curve_rescale(ret, (ahi - alo) + (bhi - blo),
			STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_RESCALE);
      stp_curve_rescale(ret, alo + blo,
			STP_CURVE_COMPOSE_ADD, STP_CURVE_BOUNDS_RESCALE);
      break;
    case STP_CURVE_COMPOSE_MULTIPLY:
      stp_curve_rescale(ret, (ahi - alo) * (bhi - blo),
			STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_RESCALE);
      stp_curve_rescale(ret, alo * blo,
			STP_CURVE_COMPOSE_ADD, STP_CURVE_BOUNDS_RESCALE);
      break;
    default:
      goto bad;
    }
  if (! stp_curve_set_data(ret, points, tmp_data))
    goto bad1;
  *retval = ret;
  stp_free(tmp_data);
  return 1;
 bad1:
  stp_curve_destroy(ret);
 bad:
  stp_free(tmp_data);
  return 0;
}
