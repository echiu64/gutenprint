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

#define COOKIE_CURVE 0x1ce0b247

typedef struct
{
  int cookie;
  stp_curve_type_t curve_type;
  size_t point_count;
  int wrap_mode;
  double gamma;
  double rlo, rhi;		/* Current range limits */
  double blo, bhi;		/* Bounds */
  double *data;
} stp_internal_curve_t;

static const char *curve_type_names[] =
  {
    "Linear",
    "Spline",
  };

static const int curve_type_count =
(sizeof(curve_type_names) / sizeof(const char *));

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

stp_curve_t
stp_curve_allocate(void)
{
  stp_internal_curve_t *ret =
    stp_zalloc(sizeof(stp_internal_curve_t));
  ret->cookie = COOKIE_CURVE;
  ret->gamma = 1.0;
  ret->rlo = ret->blo = 0.0;
  ret->rhi = ret->bhi = 1.0;
  ret->curve_type = STP_CURVE_TYPE_LINEAR;
  return (stp_curve_t) ret;
}

void
stp_curve_destroy(stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (icurve->data)
    stp_free(icurve->data);
  icurve->cookie = 0;
  icurve->curve_type = -1;
}

void
stp_curve_copy(stp_curve_t dest, const stp_curve_t source)
{
  stp_internal_curve_t *idest = (stp_internal_curve_t *) dest;
  const stp_internal_curve_t *isource = (const stp_internal_curve_t *) source;
  check_curve(idest);
  check_curve(isource);
  (void) memcpy(idest, isource, sizeof(stp_internal_curve_t));
  if (isource->data)
    {
      idest->data = stp_malloc(sizeof(double) * isource->point_count);
      (void) memcpy(idest->data, isource->data,
		    (sizeof(double) * isource->point_count));
    }
}

stp_curve_t
stp_curve_allocate_copy(const stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  stp_curve_t ret;
  check_curve(icurve);
  ret = stp_curve_allocate();
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

int
stp_curve_set_wrap(stp_curve_t curve, int wrap_mode)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (icurve->gamma)
    return 0;
  icurve->wrap_mode = wrap_mode;
  return 1;
}

int
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
  if (icurve->data)
    stp_free(icurve->data);
  icurve->point_count = 0;
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
stp_curve_set_data(stp_curve_t curve, size_t count, double *data)
{
  int i;
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (count < 2)
    return 0;
  for (i = 0; i < count; i++)
    if (! finite(data[i]) || data[i] < icurve->blo || data[i] > icurve->bhi)
      return 0;
  if (icurve->data)
    stp_free(data);
  icurve->point_count = count;
  icurve->data = stp_malloc(sizeof(double) * count);
  (void) memcpy(icurve->data, data, (sizeof(double) * count));
  scan_curve_range(icurve);
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
  int need_to_rescan = 0;
  check_curve(icurve);
  if (where >= icurve->point_count || ! finite(data) ||
      data < icurve->blo || data > icurve->bhi)
    return 0;
  if (data < icurve->rlo)
    icurve->rlo = data;
  else if (data > icurve->rhi)
    icurve->rhi = data;
  else if (icurve->data[where] == icurve->rhi ||
	   icurve->data[where] == icurve->rlo)
    need_to_rescan = 1;
  icurve->data[where] = data;
  if (need_to_rescan)
    scan_curve_range(icurve);
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
stp_curve_rescale(stp_curve_t curve, double scale)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  int i;
  check_curve(icurve);
  if (! finite(scale * icurve->bhi) || ! finite(scale * icurve->blo))
    return 0;
  icurve->bhi *= scale;
  icurve->blo *= scale;
  icurve->rhi *= scale;
  icurve->rlo *= scale;
  if (icurve->point_count)
    for (i = 0; i < icurve->point_count; i++)
      icurve->data[i] *= scale;
  return 1;
}

void
stp_curve_print(FILE *f, const stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  int i;
  check_curve(icurve);
  fprintf(f, "%s;%s;%d;%d;%g;%g;%g;",
	  "STP_CURVE",
	  curve_type_names[icurve->curve_type],
	  icurve->wrap_mode,
	  icurve->point_count,
	  icurve->gamma,
	  icurve->blo,
	  icurve->bhi);
  if (icurve->point_count)
    for (i = 0; i < icurve->point_count; i++)
      fprintf(f, "%g;", icurve->data[i]);
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
  while (1)
    {
      cur_size = snprintf(retval, ret_size - 1, "%s;%s;%d;%d;%g;%g;%g;",
			  "STP_CURVE",
			  curve_type_names[icurve->curve_type],
			  icurve->wrap_mode,
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
  if (icurve->point_count)
    for (i = 0; i < icurve->point_count; i++)
      while (1)
	{
	  int new_size = snprintf(retval + cur_size, ret_size - 1, "%g;",
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
  return retval;
}
  
int
stp_curve_read(FILE *f, stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  stp_curve_t ret;
  stp_internal_curve_t *iret;
  char curve_type_name[32];
  int noffset = 0;
  int i;

  check_curve(icurve);
  ret = stp_curve_allocate();
  iret = (stp_internal_curve_t *) ret;
  fscanf(f, "STP_CURVE;%31s;%d;%d;%lg;%lg;%lg;%n",
	 curve_type_name,
	 &(iret->wrap_mode),
	 &(iret->point_count),
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
  if (!(iret->point_count >= 2 || (iret->point_count == 0 && iret->gamma)))
    goto bad;
  noffset = 0;
  iret->curve_type = -1;
  for (i = 0; i < curve_type_count; i++)
    {
      if (strcmp(curve_type_name, curve_type_names[i]) == 0)
	{
	  iret->curve_type = i;
	  break;
	}
    }
  if (iret->curve_type < 0)
    goto bad;
  if (iret->point_count)
    {
      iret->data = stp_malloc(sizeof(double) * iret->point_count);
      for (i = 0; i < iret->point_count; i++)
	{
	  fscanf(f, "%lg;%n", &(iret->data[i]), &noffset);
	  if (noffset == 0)
	    goto bad;
	  if (! finite(iret->data[i]) || iret->data[i] < iret->blo ||
	      iret->data[i] > iret->bhi)
	    goto bad;
	  noffset = 0;
	}
    }
  scan_curve_range(iret);
  stp_curve_copy(curve, ret);
  stp_curve_destroy(ret);
  return 1;

 bad:
  if (iret->data)
    stp_free(iret->data);
  stp_curve_destroy(ret);
  return 0;
}
  
int
stp_curve_read_string(const char *text, stp_curve_t curve)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  stp_curve_t ret;
  stp_internal_curve_t *iret;
  char curve_type_name[32];
  int offset = 0;
  int noffset = 0;
  int i;

  check_curve(icurve);
  ret = stp_curve_allocate();
  iret = (stp_internal_curve_t *) ret;
  sscanf(text, "STP_CURVE;%31s;%d;%d;%lg;%lg;%lg;%n",
	 curve_type_name,
	 &(iret->wrap_mode),
	 &(iret->point_count),
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
  if (!(iret->point_count >= 2 || (iret->point_count == 0 && iret->gamma)))
    goto bad;
  offset += noffset;
  noffset = 0;
  iret->curve_type = -1;
  for (i = 0; i < curve_type_count; i++)
    {
      if (strcmp(curve_type_name, curve_type_names[i]) == 0)
	{
	  iret->curve_type = i;
	  break;
	}
    }
  if (iret->curve_type < 0)
    goto bad;
  if (iret->point_count)
    {
      iret->data = stp_malloc(sizeof(double) * iret->point_count);
      for (i = 0; i < iret->point_count; i++)
	{
	  sscanf(text + offset, "%lg;%n", &(iret->data[i]), &noffset);
	  if (noffset == 0)
	    goto bad;
	  offset += noffset;
	  noffset = 0;
	  if (! finite(iret->data[i]) || iret->data[i] < iret->blo ||
	      iret->data[i] > iret->bhi)
	    goto bad;
	}
    }
  scan_curve_range(iret);
  stp_curve_copy(curve, ret);
  stp_curve_destroy(ret);
  return offset;

 bad:
  if (iret->data)
    stp_free(iret->data);
  stp_curve_destroy(ret);
  return -1;
}

static double
interpolate_gamma_internal(const stp_curve_t curve, double where)
{
  double gamma = icurve->gamma;
  if (gamma < 0)
    {
      where = 1.0 - where;
      gamma = -gamma;
    }
  return icurve->blo + (icurve->bhi - icurve->blo) * pow(where, gamma);
}
 
int
stp_curve_interpolate_value(const stp_curve_t curve, double where,
			    double *result)
{
  stp_internal_curve_t *icurve = (stp_internal_curve_t *) curve;
  check_curve(icurve);
  if (where < 0 || where > 1)
    return 0;
  if (icurve->points == 0)	/* this means a pure gamma curve */
    {
      *result = interpolate_gamma_internal(curve, where);
      return 1;
    }
}

double stp_curve_interpolate_value(const stp_curve_t curve, double where);
int stp_curve_resample(stp_curve_t curve, size_t points);
