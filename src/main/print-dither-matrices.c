/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
 *
 *   Copyright 2001 Robert Krawitz (rlk@alum.mit.edu)
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
 *
 * Revision History:
 *
 *   See ChangeLog
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <math.h>
#include <string.h>
#include "path.h"
#include <stdlib.h>
#include <stdio.h>
#include "dither-impl.h"

#ifdef __GNUC__
#define inline __inline__
#endif

static stp_curve_t
read_matrix_from_file(const char *pathname)
{
  stp_curve_t the_curve;
  FILE *fp = fopen(pathname, "r");
  if (!fp)
    return NULL;
  the_curve = stp_curve_allocate_read(fp);
  (void) fclose(fp);
  if (!the_curve)
    return NULL;
  if (stp_dither_matrix_validate_curve(the_curve))
    return the_curve;
  else
    {
      stp_curve_destroy(the_curve);
      return NULL;
    }
}

static stp_curve_t
try_file(const char *name, stp_list_t *file_list)
{
  stp_list_item_t *item = stp_list_get_start(file_list);
  while (item)
    {
      const char *pathname = stp_list_item_get_data(item);
      if (pathname)
	{
	  const char *filename = rindex(pathname, '/');
	  if (!filename)
	    filename = pathname;
	  else
	    filename++;
	  if (strcmp(name, filename) == 0)
	    {
	      stp_curve_t answer = read_matrix_from_file(pathname);
	      if (answer)
		{
		  stp_list_destroy(file_list); /* WATCH OUT! */
		  return answer;
		}
	    }
	}
      item = stp_list_item_next(item);
    }
  return NULL;
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

stp_curve_t
stp_find_standard_dither_matrix(int x_aspect, int y_aspect)
{
  stp_list_t *dir_list;                   /* List of directories to scan */
  stp_list_t *file_list;                  /* List of files to load */
  stp_curve_t answer;
  int divisor = gcd(x_aspect, y_aspect);
  char filename[64];

  if (!(dir_list = stp_list_create()))
    return NULL;
  x_aspect /= divisor;
  y_aspect /= divisor;
  stp_list_set_freefunc(dir_list, stp_list_node_free_data);
  stp_path_split(dir_list, getenv("STP_DATA_PATH"));
  stp_path_split(dir_list, PKGMISCDATADIR);
  file_list = stp_path_search(dir_list, ".mat");
  stp_list_destroy(dir_list);
  (void) snprintf(filename, 64, "%dx%d.mat", x_aspect, y_aspect);
  answer = try_file(filename, file_list);
  if (answer)
    return answer;
  (void) snprintf(filename, 64, "%dx%d.mat", y_aspect, x_aspect);
  answer = try_file(filename, file_list);
  if (answer)
    return answer;
  (void) snprintf(filename, 64, "%dx%d.mat", x_aspect + 1, y_aspect);
  answer = try_file(filename, file_list);
  if (answer)
    return answer;
  (void) snprintf(filename, 64, "%dx%d.mat", y_aspect + 1, x_aspect);
  answer = try_file(filename, file_list);
  if (answer)
    return answer;
  (void) snprintf(filename, 64, "%dx%d.mat", x_aspect - 1, y_aspect);
  answer = try_file(filename, file_list);
  if (answer)
    return answer;
  (void) snprintf(filename, 64, "%dx%d.mat", y_aspect - 1, x_aspect);
  answer = try_file(filename, file_list);
  if (answer)
    return answer;
  (void) snprintf(filename, 64, "%dx%d.mat", x_aspect - 1, y_aspect - 1);
  answer = try_file(filename, file_list);
  if (answer)
    return answer;
  (void) snprintf(filename, 64, "%dx%d.mat", y_aspect - 1, x_aspect - 1);
  answer = try_file(filename, file_list);
  if (answer)
    return answer;
  return NULL;
}


static inline int
calc_ordered_point(unsigned x, unsigned y, int steps, int multiplier,
		   int size, const unsigned *map)
{
  int i, j;
  unsigned retval = 0;
  int divisor = 1;
  int div1;
  for (i = 0; i < steps; i++)
    {
      int xa = (x / divisor) % size;
      int ya = (y / divisor) % size;
      unsigned base;
      base = map[ya + (xa * size)];
      div1 = 1;
      for (j = i; j < steps - 1; j++)
	div1 *= size * size;
      retval += base * div1;
      divisor *= size;
    }
  return retval * multiplier;
}

static int
is_po2(size_t i)
{
  if (i == 0)
    return 0;
  return (((i & (i - 1)) == 0) ? 1 : 0);
}

void
stp_dither_matrix_iterated_init(dither_matrix_t *mat, size_t size, size_t exp,
				const unsigned *array)
{
  int i;
  int x, y;
  mat->base = size;
  mat->exp = exp;
  mat->x_size = 1;
  for (i = 0; i < exp; i++)
    mat->x_size *= mat->base;
  mat->y_size = mat->x_size;
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = stp_malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	mat->matrix[x + y * mat->x_size] =
	  calc_ordered_point(x, y, mat->exp, 1, mat->base, array);
	mat->matrix[x + y * mat->x_size] =
	  (double) mat->matrix[x + y * mat->x_size] * 65536.0 /
	  (double) (mat->x_size * mat->y_size);
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
  if (is_po2(mat->x_size))
    mat->fast_mask = mat->x_size - 1;
  else
    mat->fast_mask = 0;
}

#define MATRIX_POINT(m, x, y, x_size, y_size) \
  ((m)[(((x) + (x_size)) % (x_size)) + ((x_size) * (((y) + (y_size)) % (y_size)))])

void
stp_dither_matrix_shear(dither_matrix_t *mat, int x_shear, int y_shear)
{
  int i;
  int j;
  int *tmp = stp_malloc(mat->x_size * mat->y_size * sizeof(int));
  for (i = 0; i < mat->x_size; i++)
    for (j = 0; j < mat->y_size; j++)
      MATRIX_POINT(tmp, i, j, mat->x_size, mat->y_size) =
	MATRIX_POINT(mat->matrix, i, j * (x_shear + 1), mat->x_size,
		    mat->y_size);
  for (i = 0; i < mat->x_size; i++)
    for (j = 0; j < mat->y_size; j++)
      MATRIX_POINT(mat->matrix, i, j, mat->x_size, mat->y_size) =
	MATRIX_POINT(tmp, i * (y_shear + 1), j, mat->x_size, mat->y_size);
  stp_free(tmp);
}

int
stp_dither_matrix_validate_curve(const stp_curve_t curve)
{
  double low, high;
  stp_curve_get_bounds(curve, &low, &high);
  if (low < 0 || high > 65535)
    return 0;
  if (!stp_curve_get_point(curve, 0, &low))
    return 0;
  if (!stp_curve_get_point(curve, 1, &high))
    return 0;
  if (low * high != stp_curve_count_points(curve) - 2)
    return 0;
  return 1;
}

void
stp_dither_matrix_init_from_curve(dither_matrix_t *mat,
				  const stp_curve_t curve,
				  int transpose)
				  
{
  int x, y;
  size_t count;
  const unsigned short *vec = stp_curve_get_ushort_data(curve, &count);
  mat->base = vec[0];
  mat->exp = 1;
  mat->x_size = vec[0];
  mat->y_size = vec[1];
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = stp_malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	if (transpose)
	  mat->matrix[x + y * mat->x_size] = vec[2 + y + x * mat->y_size];
	else
	  mat->matrix[x + y * mat->x_size] = vec[2 + x + y * mat->x_size];
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
  if (is_po2(mat->x_size))
    mat->fast_mask = mat->x_size - 1;
  else
    mat->fast_mask = 0;
}

void
stp_dither_matrix_init(dither_matrix_t *mat, int x_size, int y_size,
		       const unsigned int *array, int transpose, int prescaled)
{
  int x, y;
  mat->base = x_size;
  mat->exp = 1;
  mat->x_size = x_size;
  mat->y_size = y_size;
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = stp_malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	if (transpose)
	  mat->matrix[x + y * mat->x_size] = array[y + x * mat->y_size];
	else
	  mat->matrix[x + y * mat->x_size] = array[x + y * mat->x_size];
	if (!prescaled)
	  mat->matrix[x + y * mat->x_size] =
	    (double) mat->matrix[x + y * mat->x_size] * 65536.0 /
	    (double) (mat->x_size * mat->y_size);
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
  if (is_po2(mat->x_size))
    mat->fast_mask = mat->x_size - 1;
  else
    mat->fast_mask = 0;
}

void
stp_dither_matrix_init_short(dither_matrix_t *mat, int x_size, int y_size,
			     const unsigned short *array, int transpose,
			     int prescaled)
{
  int x, y;
  mat->base = x_size;
  mat->exp = 1;
  mat->x_size = x_size;
  mat->y_size = y_size;
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = stp_malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	if (transpose)
	  mat->matrix[x + y * mat->x_size] = array[y + x * mat->y_size];
	else
	  mat->matrix[x + y * mat->x_size] = array[x + y * mat->x_size];
	if (!prescaled)
	  mat->matrix[x + y * mat->x_size] =
	    (double) mat->matrix[x + y * mat->x_size] * 65536.0 /
	    (double) (mat->x_size * mat->y_size);
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
  if (is_po2(mat->x_size))
    mat->fast_mask = mat->x_size - 1;
  else
    mat->fast_mask = 0;
}

void
stp_dither_matrix_destroy(dither_matrix_t *mat)
{
  if (mat->i_own && mat->matrix)
    stp_free(mat->matrix);
  mat->matrix = NULL;
  mat->base = 0;
  mat->exp = 0;
  mat->x_size = 0;
  mat->y_size = 0;
  mat->total_size = 0;
  mat->i_own = 0;
}

void
stp_dither_matrix_clone(const dither_matrix_t *src, dither_matrix_t *dest,
			int x_offset, int y_offset)
{
  dest->base = src->base;
  dest->exp = src->exp;
  dest->x_size = src->x_size;
  dest->y_size = src->y_size;
  dest->total_size = src->total_size;
  dest->matrix = src->matrix;
  dest->x_offset = x_offset;
  dest->y_offset = y_offset;
  dest->last_x = 0;
  dest->last_x_mod = dest->x_offset % dest->x_size;
  dest->last_y = 0;
  dest->last_y_mod = dest->x_size * (dest->y_offset % dest->y_size);
  dest->index = dest->last_x_mod + dest->last_y_mod;
  dest->fast_mask = src->fast_mask;
  dest->i_own = 0;
}

void
stp_dither_matrix_copy(const dither_matrix_t *src, dither_matrix_t *dest)
{
  int x;
  dest->base = src->base;
  dest->exp = src->exp;
  dest->x_size = src->x_size;
  dest->y_size = src->y_size;
  dest->total_size = src->total_size;
  dest->matrix = stp_malloc(sizeof(unsigned) * dest->x_size * dest->y_size);
  for (x = 0; x < dest->x_size * dest->y_size; x++)
    dest->matrix[x] = src->matrix[x];
  dest->x_offset = 0;
  dest->y_offset = 0;
  dest->last_x = 0;
  dest->last_x_mod = 0;
  dest->last_y = 0;
  dest->last_y_mod = 0;
  dest->index = 0;
  dest->fast_mask = src->fast_mask;
  dest->i_own = 1;
}

void
stp_dither_matrix_scale_exponentially(dither_matrix_t *mat, double exponent)
{
  int i;
  int mat_size = mat->x_size * mat->y_size;
  for (i = 0; i < mat_size; i++)
    {
      double dd = mat->matrix[i] / 65535.0;
      dd = pow(dd, exponent);
      mat->matrix[i] = 65535 * dd;
    }
}

void
stp_dither_matrix_set_row(dither_matrix_t *mat, int y)
{
  mat->last_y = y;
  mat->last_y_mod = mat->x_size * ((y + mat->y_offset) % mat->y_size);
  mat->index = mat->last_x_mod + mat->last_y_mod;
}

static void
preinit_matrix(stp_vars_t v)
{
  dither_t *d = (dither_t *) stp_get_dither_data(v);
  int i;
  for (i = 0; i < PHYSICAL_CHANNEL_COUNT(d); i++)
    stp_dither_matrix_destroy(&(PHYSICAL_CHANNEL(d, i).dithermat));
  stp_dither_matrix_destroy(&(d->dither_matrix));
}

static void
postinit_matrix(stp_vars_t v, int x_shear, int y_shear)
{
  dither_t *d = (dither_t *) stp_get_dither_data(v);
  unsigned rc = 1 + (unsigned) ceil(sqrt(PHYSICAL_CHANNEL_COUNT(d)));
  int i, j;
  int color = 0;
  unsigned x_n = d->dither_matrix.x_size / rc;
  unsigned y_n = d->dither_matrix.y_size / rc;
  if (x_shear || y_shear)
    stp_dither_matrix_shear(&(d->dither_matrix), x_shear, y_shear);
  for (i = 0; i < rc; i++)
    for (j = 0; j < rc; j++)
      if (color < PHYSICAL_CHANNEL_COUNT(d))
	{
	  stp_dither_matrix_clone(&(d->dither_matrix),
				  &(PHYSICAL_CHANNEL(d, color).dithermat),
				  x_n * i, y_n * j);
	  color++;
	}
  stp_dither_set_transition(v, d->transition);
}

void
stp_dither_set_iterated_matrix(stp_vars_t v, size_t edge, size_t iterations,
			       const unsigned *data, int prescaled,
			       int x_shear, int y_shear)
{
  dither_t *d = (dither_t *) stp_get_dither_data(v);
  preinit_matrix(v);
  stp_dither_matrix_iterated_init(&(d->dither_matrix), edge, iterations, data);
  postinit_matrix(v, x_shear, y_shear);
}

void
stp_dither_set_matrix(stp_vars_t v, const stp_dither_matrix_t *matrix,
		      int transposed, int x_shear, int y_shear)
{
  dither_t *d = (dither_t *) stp_get_dither_data(v);
  int x = transposed ? matrix->y : matrix->x;
  int y = transposed ? matrix->x : matrix->y;
  preinit_matrix(v);
  if (matrix->bytes == 2)
    stp_dither_matrix_init_short(&(d->dither_matrix), x, y,
				 (const unsigned short *) matrix->data,
				 transposed, matrix->prescaled);
  else if (matrix->bytes == 4)
    stp_dither_matrix_init(&(d->dither_matrix), x, y,
			   (const unsigned *)matrix->data,
			   transposed, matrix->prescaled);
  postinit_matrix(v, x_shear, y_shear);
}

void
stp_dither_set_matrix_from_curve(stp_vars_t v, const stp_curve_t curve,
				 int transpose)
{
  dither_t *d = (dither_t *) stp_get_dither_data(v);
  preinit_matrix(v);
  stp_dither_matrix_init_from_curve(&(d->dither_matrix), curve, transpose);
  postinit_matrix(v, 0, 0);
}

void
stp_dither_set_transition(stp_vars_t v, double exponent)
{
  dither_t *d = (dither_t *) stp_get_dither_data(v);
  unsigned rc = 1 + (unsigned) ceil(sqrt(PHYSICAL_CHANNEL_COUNT(d)));
  int i, j;
  int color = 0;
  unsigned x_n = d->dither_matrix.x_size / rc;
  unsigned y_n = d->dither_matrix.y_size / rc;
  for (i = 0; i < PHYSICAL_CHANNEL_COUNT(d); i++)
    stp_dither_matrix_destroy(&(PHYSICAL_CHANNEL(d, i).pick));
  stp_dither_matrix_destroy(&(d->transition_matrix));
  stp_dither_matrix_copy(&(d->dither_matrix), &(d->transition_matrix));
  d->transition = exponent;
  if (exponent < .999 || exponent > 1.001)
    stp_dither_matrix_scale_exponentially(&(d->transition_matrix), exponent);
  for (i = 0; i < rc; i++)
    for (j = 0; j < rc; j++)
      if (color < PHYSICAL_CHANNEL_COUNT(d))
	{
	  stp_dither_matrix_clone(&(d->dither_matrix),
				  &(PHYSICAL_CHANNEL(d, color).pick),
				  x_n * i, y_n * j);
	  color++;
	}
  if (exponent < .999 || exponent > 1.001)
    for (i = 0; i < 65536; i++)
      {
	double dd = i / 65535.0;
	dd = pow(dd, 1.0 / exponent);
	d->virtual_dot_scale[i] = dd * 65535;
      }
  else
    for (i = 0; i < 65536; i++)
      d->virtual_dot_scale[i] = i;
}
