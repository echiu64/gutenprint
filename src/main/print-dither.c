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
#include <gimp-print/gimp-print-intl-internal.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#ifdef __GNUC__
#define inline __inline__
#endif

#define D_FLOYD_HYBRID 0
#define D_ADAPTIVE_BASE 4
#define D_ADAPTIVE_HYBRID (D_ADAPTIVE_BASE | D_FLOYD_HYBRID)
#define D_ORDERED_BASE 8
#define D_ORDERED (D_ORDERED_BASE)
#define D_FAST_BASE 16
#define D_FAST (D_FAST_BASE)
#define D_VERY_FAST (D_FAST_BASE + 1)

#define DITHER_FAST_STEPS (6)

typedef struct
{
  const char *name;
  const char *text;
  int id;
} dither_algo_t;

static const dither_algo_t dither_algos[] =
{
  { "Adaptive",	N_ ("Adaptive Hybrid"),        D_ADAPTIVE_HYBRID },
  { "Ordered",	N_ ("Ordered"),                D_ORDERED },
  { "Fast",	N_ ("Fast"),                   D_FAST },
  { "VeryFast",	N_ ("Very Fast"),              D_VERY_FAST },
  { "Floyd",	N_ ("Hybrid Floyd-Steinberg"), D_FLOYD_HYBRID }
};

static const int num_dither_algos = sizeof(dither_algos)/sizeof(dither_algo_t);

#define ERROR_ROWS 2

#define MAX_SPREAD 32

/*
 * A segment of the entire 0-65535 intensity range.
 */
typedef struct dither_segment
{
  unsigned range[2];		/* Bottom, top of range */
  unsigned value[2];		/* Value of lower, upper ink */
  unsigned bits[2];		/* Bit pattern of lower, upper */
  unsigned range_span;		/* Span (to avoid calculation on the fly) */
  unsigned value_span;		/* Span of values */
  unsigned dot_size[2];		/* Size of lower, upper dot */
  int isdark[2];		/* Is lower, upper value dark ink? */
  int is_same_ink;		/* Are both endpoints using the same dots? */
  int is_equal;			/* Are both endpoints using the same ink? */
} dither_segment_t;

typedef struct dither_color
{
  int nlevels;
  unsigned bit_max;
  unsigned signif_bits;
  unsigned density;
  int row_ends[2][2];
  dither_segment_t *ranges;
} dither_color_t;

typedef struct dither_matrix
{
  int base;
  int exp;
  int x_size;
  int y_size;
  int total_size;
  int last_x;
  int last_x_mod;
  int last_y;
  int last_y_mod;
  int index;
  int i_own;
  int x_offset;
  int y_offset;
  unsigned fast_mask;
  unsigned *matrix;
} dither_matrix_t;

typedef struct dither_channel
{
  unsigned randomizer;		/* With Floyd-Steinberg dithering, control */
				/* how much randomness is applied to the */
				/* threshold values (0-65535).  With ordered */
				/* dithering, how much randomness is added */
				/* to the matrix value. */
  int k_level;			/* Amount of each ink (in 64ths) required */
				/* to create equivalent black */
  int darkness;			/* Perceived "darkness" of each ink, */
				/* in 64ths, to calculate CMY-K transitions */
  dither_color_t dither;
  int *errs[ERROR_ROWS];
  unsigned short *vals;
  dither_matrix_t pick;
  dither_matrix_t dithermat;

  int v;
  int o;
  int b;
  int very_fast;
  unsigned char *ptrs[2];
} dither_channel_t;

typedef struct dither
{
  int src_width;		/* Input width */
  int dst_width;		/* Output width */

  int density;			/* Desired density, 0-1.0 (scaled 0-65535) */
  int black_density;		/* Desired density, 0-1.0 (scaled 0-65535) */
  int k_lower;			/* Transition range (lower/upper) for CMY */
  int k_upper;			/* vs. K */
  int density2;			/* Density * 2 */
  int densityh;			/* Density / 2 */
  unsigned dlb_range;
  unsigned bound_range;

  int spread;			/* With Floyd-Steinberg, how widely the */
  int spread_mask;		/* error is distributed.  This should be */
				/* between 12 (very broad distribution) and */
				/* 19 (very narrow) */

  int dither_type;

  int d_cutoff;			/* When ordered dither is used, threshold */
				/* above which no randomness is used. */
  double adaptive_input;
  int adaptive_limit;

  int x_aspect;			/* Aspect ratio numerator */
  int y_aspect;			/* Aspect ratio denominator */

  double transition;		/* Exponential scaling for transition region */

  int *offset0_table;
  int *offset1_table;

  int oversampling;
  int last_line_was_empty;
  int ptr_offset;

  int dither_class;		/* mono, black, or CMYK */

  dither_matrix_t dither_matrix;
  dither_matrix_t transition_matrix;
  dither_channel_t channel[NCOLORS];

  unsigned short virtual_dot_scale[65536];
  stp_vars_t v;
} dither_t;

#define CHANNEL(d, c) ((d)->channel[(c)])

/*
 * Bayer's dither matrix using Judice, Jarvis, and Ninke recurrence relation
 * http://www.cs.rit.edu/~sxc7922/Project/CRT.htm
 */

static const unsigned sq2[] =
{
  0, 2,
  3, 1
};

size_t
stp_dither_algorithm_count(void)
{
  return num_dither_algos;
}

const char *
stp_dither_algorithm_name(int id)
{
  if (id < 0 || id >= num_dither_algos)
    return NULL;
  return (dither_algos[id].name);
}

const char *
stp_dither_algorithm_text(int id)
{
  if (id < 0 || id >= num_dither_algos)
    return NULL;
  return _(dither_algos[id].text);
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
  int bits = 0;
  size_t j = 1;
  int k;
  for (k = 0; k < CHAR_BIT * sizeof(size_t); k++)
    {
      if (j & i)
	{
	  bits++;
	  if (bits > 1)
	    return 0;
	}
      j <<= 1;
    }
  return bits;
}

static void
init_iterated_matrix(dither_matrix_t *mat, size_t size, size_t exp,
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

#define DITHERPOINT(m, x, y, x_size, y_size) \
  ((m)[(((x) + (x_size)) % (x_size)) + ((x_size) * (((y) + (y_size)) % (y_size)))])

static void
shear_matrix(dither_matrix_t *mat, int x_shear, int y_shear)
{
  int i;
  int j;
  int *tmp = stp_malloc(mat->x_size * mat->y_size * sizeof(int));
  for (i = 0; i < mat->x_size; i++)
    for (j = 0; j < mat->y_size; j++)
      DITHERPOINT(tmp, i, j, mat->x_size, mat->y_size) =
	DITHERPOINT(mat->matrix, i, j * (x_shear + 1), mat->x_size,
		    mat->y_size);
  for (i = 0; i < mat->x_size; i++)
    for (j = 0; j < mat->y_size; j++)
      DITHERPOINT(mat->matrix, i, j, mat->x_size, mat->y_size) =
	DITHERPOINT(tmp, i * (y_shear + 1), j, mat->x_size, mat->y_size);
  stp_free(tmp);
}

static void
init_matrix(dither_matrix_t *mat, int x_size, int y_size,
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

static void
init_matrix_short(dither_matrix_t *mat, int x_size, int y_size,
		  const unsigned short *array, int transpose, int prescaled)
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

static void
destroy_matrix(dither_matrix_t *mat)
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

static void
clone_matrix(const dither_matrix_t *src, dither_matrix_t *dest,
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

static void
copy_matrix(const dither_matrix_t *src, dither_matrix_t *dest)
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

static void
exponential_scale_matrix(dither_matrix_t *mat, double exponent)
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

static void
matrix_set_row(const dither_t *d, dither_matrix_t *mat, int y)
{
  mat->last_y = y;
  mat->last_y_mod = mat->x_size * ((y + mat->y_offset) % mat->y_size);
  mat->index = mat->last_x_mod + mat->last_y_mod;
}

static inline unsigned
ditherpoint(const dither_t *d, dither_matrix_t *mat, int x)
{
  if (mat->fast_mask)
    return mat->matrix[(mat->last_y_mod +
			((x + mat->x_offset) & mat->fast_mask))];
  /*
   * This rather bizarre code is an attempt to avoid having to compute a lot
   * of modulus and multiplication operations, which are typically slow.
   */

  if (x == mat->last_x + 1)
    {
      mat->last_x_mod++;
      mat->index++;
      if (mat->last_x_mod >= mat->x_size)
	{
	  mat->last_x_mod -= mat->x_size;
	  mat->index -= mat->x_size;
	}
    }
  else if (x == mat->last_x - 1)
    {
      mat->last_x_mod--;
      mat->index--;
      if (mat->last_x_mod < 0)
	{
	  mat->last_x_mod += mat->x_size;
	  mat->index += mat->x_size;
	}
    }
  else if (x == mat->last_x)
    {
    }
  else
    {
      mat->last_x_mod = (x + mat->x_offset) % mat->x_size;
      mat->index = mat->last_x_mod + mat->last_y_mod;
    }
  mat->last_x = x;
  return mat->matrix[mat->index];
}

static void
reverse_row_ends(dither_t *d)
{
  int i, j;
  for (i = 0; i < NCOLORS; i++)
    for (j = 0; j < 2; j++)
      {
	int tmp = CHANNEL(d, i).dither.row_ends[0][j];
	CHANNEL(d, i).dither.row_ends[0][j] =
	  CHANNEL(d, i).dither.row_ends[1][j];
	CHANNEL(d, i).dither.row_ends[1][j] = tmp;
      }
}

void *
stp_init_dither(int in_width, int out_width, int horizontal_aspect,
		int vertical_aspect, stp_vars_t v)
{
  int i;
  dither_t *d = stp_malloc(sizeof(dither_t));
  stp_simple_dither_range_t r;
  memset(d, 0, sizeof(dither_t));
  d->v = v;
  r.value = 1.0;
  r.bit_pattern = 1;
  r.is_dark = 1;
  r.dot_size = 1;
  for (i = 0; i < NCOLORS; i++)
    stp_dither_set_ranges(d, i, 1, &r, 1.0);
  d->offset0_table = NULL;
  d->offset1_table = NULL;
  d->x_aspect = horizontal_aspect;
  d->y_aspect = vertical_aspect;

  d->dither_type = D_FLOYD_HYBRID;
  for (i = 0; i < num_dither_algos; i++)
    {
      if (!strcmp(stp_get_dither_algorithm(v), _(dither_algos[i].name)))
	{
	  d->dither_type = dither_algos[i].id;
	  break;
	}
    }
  d->transition = 1.0;
  d->adaptive_input = .75;

  if (d->dither_type == D_VERY_FAST)
    stp_dither_set_iterated_matrix(d, 2, DITHER_FAST_STEPS, sq2, 0, 2, 4);
  else
    {
      stp_dither_matrix_t *mat;
      int transposed = 0;
      if (d->y_aspect == d->x_aspect)
	mat = (stp_dither_matrix_t *) &stp_1_1_matrix;
      else if (d->y_aspect > d->x_aspect)
	{
	  transposed = 0;
	  if (d->y_aspect / d->x_aspect == 2)
	    mat = (stp_dither_matrix_t *) &stp_2_1_matrix;
	  else if (d->y_aspect / d->x_aspect == 3)
	    mat = (stp_dither_matrix_t *) &stp_4_1_matrix;
	  else if (d->y_aspect / d->x_aspect == 4)
	    mat = (stp_dither_matrix_t *) &stp_4_1_matrix;
	  else
	    mat = (stp_dither_matrix_t *) &stp_2_1_matrix;
	}
      else
	{
	  transposed = 1;
	  if (d->x_aspect / d->y_aspect == 2)
	    mat = (stp_dither_matrix_t *) &stp_2_1_matrix;
	  else if (d->x_aspect / d->y_aspect == 3)
	    mat = (stp_dither_matrix_t *) &stp_4_1_matrix;
	  else if (d->x_aspect / d->y_aspect == 4)
	    mat = (stp_dither_matrix_t *) &stp_4_1_matrix;
	  else
	    mat = (stp_dither_matrix_t *) &stp_2_1_matrix;
	}
      stp_dither_set_matrix(d, mat, transposed, 0, 0);
    }

  d->src_width = in_width;
  d->dst_width = out_width;

  stp_dither_set_ink_spread(d, 13);
  stp_dither_set_black_lower(d, .4);
  stp_dither_set_black_upper(d, .7);
  for (i = 0; i <= NCOLORS; i++)
    {
      stp_dither_set_black_level(d, i, 1.0);
      stp_dither_set_randomizer(d, i, 1.0);
    }
  stp_dither_set_ink_darkness(d, ECOLOR_C, 2);
  stp_dither_set_ink_darkness(d, ECOLOR_M, 2);
  stp_dither_set_ink_darkness(d, ECOLOR_Y, 1);
  stp_dither_set_density(d, 1.0);
  d->dither_class = stp_get_output_type(v);
  return d;
}

static void
preinit_matrix(dither_t *d)
{
  int i;
  for (i = 0; i < NCOLORS; i++)
    destroy_matrix(&(CHANNEL(d, i).dithermat));
  destroy_matrix(&(d->dither_matrix));
}

static void
postinit_matrix(dither_t *d, int x_shear, int y_shear)
{
  unsigned x_3, y_3;
  if (x_shear || y_shear)
    shear_matrix(&(d->dither_matrix), x_shear, y_shear);
  x_3 = d->dither_matrix.x_size / 3;
  y_3 = d->dither_matrix.y_size / 3;
  clone_matrix(&(d->dither_matrix), &(CHANNEL(d, ECOLOR_C).dithermat),
	       2 * x_3, y_3);
  clone_matrix(&(d->dither_matrix), &(CHANNEL(d, ECOLOR_M).dithermat),
	       x_3, 2 * y_3);
  clone_matrix(&(d->dither_matrix), &(CHANNEL(d, ECOLOR_Y).dithermat), 0, y_3);
  clone_matrix(&(d->dither_matrix), &(CHANNEL(d, ECOLOR_K).dithermat), 0, 0);
  stp_dither_set_transition(d, d->transition);
}

void
stp_dither_set_iterated_matrix(void *vd, size_t edge, size_t iterations,
			       const unsigned *data, int prescaled,
			       int x_shear, int y_shear)
{
  dither_t *d = (dither_t *) vd;
  preinit_matrix(d);
  init_iterated_matrix(&(d->dither_matrix), edge, iterations, data);
  postinit_matrix(d, x_shear, y_shear);
}

void
stp_dither_set_matrix(void *vd, const stp_dither_matrix_t *matrix,
		      int transposed, int x_shear, int y_shear)
{
  dither_t *d = (dither_t *) vd;
  int x = transposed ? matrix->y : matrix->x;
  int y = transposed ? matrix->x : matrix->y;
  preinit_matrix(d);
  if (matrix->bytes == 2)
    init_matrix_short(&(d->dither_matrix), x, y,
		      (const unsigned short *) matrix->data,
		      transposed, matrix->prescaled);
  else if (matrix->bytes == 4)
    init_matrix(&(d->dither_matrix), x, y, (const unsigned *)matrix->data,
		transposed, matrix->prescaled);
  postinit_matrix(d, x_shear, y_shear);
}

void
stp_dither_set_transition(void *vd, double exponent)
{
  int i;
  dither_t *d = (dither_t *) vd;
  int x_3 = d->dither_matrix.x_size / 3;
  int y_3 = d->dither_matrix.y_size / 3;
  for (i = 0; i < NCOLORS; i++)
    destroy_matrix(&(CHANNEL(d, i).pick));
  destroy_matrix(&(d->transition_matrix));
  copy_matrix(&(d->dither_matrix), &(d->transition_matrix));
  d->transition = exponent;
  if (exponent < .999 || exponent > 1.001)
    exponential_scale_matrix(&(d->transition_matrix), exponent);
  clone_matrix(&(d->transition_matrix), &(CHANNEL(d, ECOLOR_C).pick),
	       2 * x_3, y_3);
  clone_matrix(&(d->transition_matrix), &(CHANNEL(d, ECOLOR_M).pick),
	       x_3, 2 * y_3);
  clone_matrix(&(d->transition_matrix), &(CHANNEL(d, ECOLOR_Y).pick), 0, y_3);
  clone_matrix(&(d->transition_matrix), &(CHANNEL(d, ECOLOR_K).pick), 0, 0);
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

void
stp_dither_set_density(void *vd, double density)
{
  dither_t *d = (dither_t *) vd;
  if (density > 1)
    density = 1;
  else if (density < 0)
    density = 0;
  d->k_upper = d->k_upper * density;
  d->k_lower = d->k_lower * density;
  d->density = (int) ((65535 * density) + .5);
  d->density2 = 2 * d->density;
  d->densityh = d->density / 2;
  d->dlb_range = d->density - d->k_lower;
  d->bound_range = d->k_upper - d->k_lower;
  d->d_cutoff = d->density / 16;
  d->adaptive_limit = d->density * d->adaptive_input;
  stp_dither_set_black_density(vd, density);
}

void
stp_dither_set_black_density(void *vd, double density)
{
  dither_t *d = (dither_t *) vd;
  if (density > 1)
    density = 1;
  else if (density < 0)
    density = 0;
  d->black_density = (int) ((65535 * density) + .5);
}

void
stp_dither_set_adaptive_limit(void *vd, double limit)
{
  dither_t *d = (dither_t *) vd;
  d->adaptive_input = limit;
  d->adaptive_limit = d->density * limit;
}

void
stp_dither_set_black_lower(void *vd, double k_lower)
{
  dither_t *d = (dither_t *) vd;
  d->k_lower = (int) (k_lower * 65535);
}

void
stp_dither_set_black_upper(void *vd, double k_upper)
{
  dither_t *d = (dither_t *) vd;
  d->k_upper = (int) (k_upper * 65535);
}

void
stp_dither_set_ink_spread(void *vd, int spread)
{
  dither_t *d = (dither_t *) vd;
  if (d->offset0_table)
    {
      stp_free(d->offset0_table);
      d->offset0_table = NULL;
    }
  if (d->offset1_table)
    {
      stp_free(d->offset1_table);
      d->offset1_table = NULL;
    }
  if (spread >= 16)
    {
      d->spread = 16;
    }
  else
    {
      int max_offset;
      int i;
      d->spread = spread;
      max_offset = (1 << (16 - spread)) + 1;
      d->offset0_table = stp_malloc(sizeof(int) * max_offset);
      d->offset1_table = stp_malloc(sizeof(int) * max_offset);
      for (i = 0; i < max_offset; i++)
	{
	  d->offset0_table[i] = (i + 1) * (i + 1);
	  d->offset1_table[i] = ((i + 1) * i) / 2;
	}
    }
  d->spread_mask = (1 << d->spread) - 1;
  d->adaptive_limit = d->density * d->adaptive_input;
}

void
stp_dither_set_black_level(void *vd, int i, double v)
{
  dither_t *d = (dither_t *) vd;
  if (i < 0 || i >= NCOLORS)
    return;
  CHANNEL(d, i).k_level = (int) v * 64;
}

void
stp_dither_set_randomizer(void *vd, int i, double v)
{
  dither_t *d = (dither_t *) vd;
  if (i < 0 || i >= NCOLORS)
    return;
  CHANNEL(d, i).randomizer = v * 65535;
}

void
stp_dither_set_ink_darkness(void *vd, int i, double v)
{
  dither_t *d = (dither_t *) vd;
  if (i < 0 || i >= NCOLORS)
    return;
  CHANNEL(d, i).darkness = (int) (v * 64);
}

void
stp_dither_set_light_ink(void *vd, int i, double v, double density)
{
  stp_simple_dither_range_t range[2];
  if (i < 0 || i >= NCOLORS || v <= 0 || v > 1)
    return;
  range[0].bit_pattern = 1;
  range[0].is_dark = 0;
  range[1].value = 1;
  range[1].bit_pattern = 1;
  range[1].is_dark = 1;
  range[1].dot_size = 1;
  range[0].value = v;
  range[0].dot_size = 1;
  stp_dither_set_ranges(vd, i, 2, range, density);
}

static void
stp_dither_set_generic_ranges(dither_t *d, dither_color_t *s, int nlevels,
			      const stp_simple_dither_range_t *ranges,
			      double density)
{
  int i;
  unsigned lbit;
  if (s->ranges)
    stp_free(s->ranges);
  s->nlevels = nlevels > 1 ? nlevels + 1 : nlevels;
  s->ranges = (dither_segment_t *)
    stp_malloc(s->nlevels * sizeof(dither_segment_t));
  s->bit_max = 0;
  s->density = density * 65535;
  stp_dprintf(STP_DBG_INK, d->v,
	      "stp_dither_set_generic_ranges nlevels %d density %f\n",
	      nlevels, density);
  for (i = 0; i < nlevels; i++)
    stp_dprintf(STP_DBG_INK, d->v,
		"  level %d value %f pattern %x is_dark %d\n", i,
		ranges[i].value, ranges[i].bit_pattern, ranges[i].is_dark);
  s->ranges[0].range[0] = 0;
  s->ranges[0].value[0] = ranges[0].value * 65535.0;
  s->ranges[0].bits[0] = ranges[0].bit_pattern;
  s->ranges[0].isdark[0] = ranges[0].is_dark;
  s->ranges[0].dot_size[0] = ranges[0].dot_size;
  if (nlevels == 1)
    s->ranges[0].range[1] = 65535;
  else
    s->ranges[0].range[1] = ranges[0].value * 65535.0 * density;
  if (s->ranges[0].range[1] > 65535)
    s->ranges[0].range[1] = 65535;
  s->ranges[0].value[1] = ranges[0].value * 65535.0;
  if (s->ranges[0].value[1] > 65535)
    s->ranges[0].value[1] = 65535;
  s->ranges[0].bits[1] = ranges[0].bit_pattern;
  if (ranges[0].bit_pattern > s->bit_max)
    s->bit_max = ranges[0].bit_pattern;
  s->ranges[0].isdark[1] = ranges[0].is_dark;
  s->ranges[0].dot_size[1] = ranges[0].dot_size;
  s->ranges[0].range_span = s->ranges[0].range[1];
  s->ranges[0].value_span = 0;
  if (s->nlevels > 1)
    {
      for (i = 0; i < nlevels - 1; i++)
	{
	  int l = i + 1;
	  s->ranges[l].range[0] = s->ranges[i].range[1];
	  s->ranges[l].value[0] = s->ranges[i].value[1];
	  s->ranges[l].bits[0] = s->ranges[i].bits[1];
	  s->ranges[l].isdark[0] = s->ranges[i].isdark[1];
	  s->ranges[l].dot_size[0] = s->ranges[i].dot_size[1];
	  if (i == nlevels - 1)
	    s->ranges[l].range[1] = 65535;
	  else
	    s->ranges[l].range[1] =
	      (ranges[l].value + ranges[l].value) * 32768.0 * density;
	  if (s->ranges[l].range[1] > 65535)
	    s->ranges[l].range[1] = 65535;
	  s->ranges[l].value[1] = ranges[l].value * 65535.0;
	  if (s->ranges[l].value[1] > 65535)
	    s->ranges[l].value[1] = 65535;
	  s->ranges[l].bits[1] = ranges[l].bit_pattern;
	  if (ranges[l].bit_pattern > s->bit_max)
	    s->bit_max = ranges[l].bit_pattern;
	  s->ranges[l].isdark[1] = ranges[l].is_dark;
	  s->ranges[l].dot_size[1] = ranges[l].dot_size;
	  s->ranges[l].range_span =
	    s->ranges[l].range[1] - s->ranges[l].range[0];
	  s->ranges[l].value_span =
	    s->ranges[l].value[1] - s->ranges[l].value[0];
	}
      i++;
      s->ranges[i].range[0] = s->ranges[i - 1].range[1];
      s->ranges[i].value[0] = s->ranges[i - 1].value[1];
      s->ranges[i].bits[0] = s->ranges[i - 1].bits[1];
      s->ranges[i].isdark[0] = s->ranges[i - 1].isdark[1];
      s->ranges[i].dot_size[0] = s->ranges[i - 1].dot_size[1];
      s->ranges[i].range[1] = 65535;
      s->ranges[i].value[1] = s->ranges[i].value[0];
      s->ranges[i].bits[1] = s->ranges[i].bits[0];
      s->ranges[i].isdark[1] = s->ranges[i].isdark[0];
      s->ranges[i].dot_size[1] = s->ranges[i].dot_size[0];
      s->ranges[i].range_span = s->ranges[i].range[1] - s->ranges[i].range[0];
      s->ranges[i].value_span = s->ranges[i].value[1] - s->ranges[i].value[0];
    }
  lbit = s->bit_max;
  s->signif_bits = 0;
  while (lbit > 0)
    {
      s->signif_bits++;
      lbit >>= 1;
    }
  for (i = 0; i < s->nlevels; i++)
    {
      if (s->ranges[i].isdark[0] == s->ranges[i].isdark[1] &&
	  s->ranges[i].dot_size[0] == s->ranges[i].dot_size[1])
	s->ranges[i].is_same_ink = 1;
      else
	s->ranges[i].is_same_ink = 0;
      if (s->ranges[i].range_span > 0 &&
	  (s->ranges[i].value_span > 0 ||
	   s->ranges[i].isdark[0] != s->ranges[i].isdark[1]))
	s->ranges[i].is_equal = 0;
      else
	s->ranges[i].is_equal = 1;
      stp_dprintf(STP_DBG_INK, d->v,
		  "    level %d value[0] %d value[1] %d range[0] %d range[1] %d\n",
		  i, s->ranges[i].value[0], s->ranges[i].value[1],
		  s->ranges[i].range[0], s->ranges[i].range[1]);
      stp_dprintf(STP_DBG_INK, d->v,
		  "       bits[0] %d bits[1] %d isdark[0] %d isdark[1] %d\n",
		  s->ranges[i].bits[0], s->ranges[i].bits[1],
		  s->ranges[i].isdark[0], s->ranges[i].isdark[1]);
      stp_dprintf(STP_DBG_INK, d->v,
		  "       rangespan %d valuespan %d same_ink %d equal %d\n",
		  s->ranges[i].range_span, s->ranges[i].value_span,
		  s->ranges[i].is_same_ink, s->ranges[i].is_equal);
    }
  stp_dprintf(STP_DBG_INK, d->v,
	      "  bit_max %d signif_bits %d\n", s->bit_max, s->signif_bits);
}

static void
stp_dither_set_generic_ranges_full(dither_t *d, dither_color_t *s, int nlevels,
				   const stp_full_dither_range_t *ranges,
				   double density)
{
  int i, j, k;
  unsigned lbit;
  if (s->ranges)
    stp_free(s->ranges);
  s->nlevels = nlevels > 1 ? nlevels + 1 : nlevels;
  s->nlevels = nlevels+1;
  s->ranges = (dither_segment_t *)
    stp_malloc(s->nlevels * sizeof(dither_segment_t));
  s->bit_max = 0;
  s->density = density * 65535;
  stp_dprintf(STP_DBG_INK, d->v,
	      "stp_dither_set_ranges nlevels %d density %f\n",
	      nlevels, density);
  for (i = 0; i < nlevels; i++)
    stp_dprintf(STP_DBG_INK, d->v,
		"  level %d value: low %f high %f pattern low %x "
		"high %x is_dark low %d high %d\n", i,
		ranges[i].value[0], ranges[i].value[1],
		ranges[i].bits[0], ranges[i].bits[1],ranges[i].isdark[0],
		ranges[i].isdark[1]);
  for(i=j=0; i < nlevels; i++)
    {
      for (k = 0; k < 2; k++)
	{
	  if (ranges[i].bits[k] > s->bit_max)
	    s->bit_max = ranges[i].bits[k];
	  s->ranges[j].dot_size[k] = ranges[i].bits[k]; /* FIXME */
	  s->ranges[j].value[k] = ranges[i].value[k] * 65535;
	  s->ranges[j].range[k] = s->ranges[j].value[k]*density;
	  s->ranges[j].bits[k] = ranges[i].bits[k];
	  s->ranges[j].isdark[k] = ranges[i].isdark[k];
	}
    s->ranges[j].range_span = s->ranges[j].range[1]-s->ranges[j].range[0];
    s->ranges[j].value_span = s->ranges[j].value[1]-s->ranges[j].value[0];
    j++;
  }
  s->ranges[j].range[0] = s->ranges[j - 1].range[1];
  s->ranges[j].value[0] = s->ranges[j - 1].value[1];
  s->ranges[j].bits[0] = s->ranges[j - 1].bits[1];
  s->ranges[j].isdark[0] = s->ranges[j - 1].isdark[1];
  s->ranges[j].dot_size[0] = s->ranges[j - 1].dot_size[1];
  s->ranges[j].range[1] = 65535;
  s->ranges[j].value[1] = 65535;
  s->ranges[j].bits[1] = s->ranges[j].bits[0];
  s->ranges[j].isdark[1] = s->ranges[j].isdark[0];
  s->ranges[j].dot_size[1] = s->ranges[j].dot_size[0];
  s->ranges[j].range_span = s->ranges[j].range[1] - s->ranges[j].range[0];
  s->ranges[j].value_span = 0;
  s->nlevels = j+1;
  lbit = s->bit_max;
  s->signif_bits = 0;
  while (lbit > 0)
    {
      s->signif_bits++;
      lbit >>= 1;
    }
  for (i = 0; i < s->nlevels; i++)
    {
      if (s->ranges[i].isdark[0] == s->ranges[i].isdark[1] &&
	  s->ranges[i].dot_size[0] == s->ranges[i].dot_size[1])
	s->ranges[i].is_same_ink = 1;
      else
	s->ranges[i].is_same_ink = 0;
      if (s->ranges[i].range_span > 0 &&
	  (s->ranges[i].value_span > 0 ||
	   s->ranges[i].isdark[0] != s->ranges[i].isdark[1]))
	s->ranges[i].is_equal = 0;
      else
	s->ranges[i].is_equal = 1;
      stp_dprintf(STP_DBG_INK, d->v,
		  "    level %d value[0] %d value[1] %d range[0] %d range[1] %d\n",
		  i, s->ranges[i].value[0], s->ranges[i].value[1],
		  s->ranges[i].range[0], s->ranges[i].range[1]);
      stp_dprintf(STP_DBG_INK, d->v,
		  "       bits[0] %d bits[1] %d isdark[0] %d isdark[1] %d\n",
		  s->ranges[i].bits[0], s->ranges[i].bits[1],
		  s->ranges[i].isdark[0], s->ranges[i].isdark[1]);
      stp_dprintf(STP_DBG_INK, d->v,
		  "       rangespan %d valuespan %d same_ink %d equal %d\n",
		  s->ranges[i].range_span, s->ranges[i].value_span,
		  s->ranges[i].is_same_ink, s->ranges[i].is_equal);
    }
  stp_dprintf(STP_DBG_INK, d->v,
	      "  bit_max %d signif_bits %d\n", s->bit_max, s->signif_bits);
}

void
stp_dither_set_ranges(void *vd, int color, int nlevels,
		      const stp_simple_dither_range_t *ranges, double density)
{
  dither_t *d = (dither_t *) vd;
  if (color < 0 || color >= NCOLORS)
    return;
  stp_dither_set_generic_ranges(d, &(CHANNEL(d, color).dither), nlevels,
				ranges, density);
}

void
stp_dither_set_ranges_simple(void *vd, int color, int nlevels,
			     const double *levels, double density)
{
  stp_simple_dither_range_t *r =
    stp_malloc(nlevels * sizeof(stp_simple_dither_range_t));
  int i;
  for (i = 0; i < nlevels; i++)
    {
      r[i].bit_pattern = i + 1;
      r[i].dot_size = i + 1;
      r[i].value = levels[i];
      r[i].is_dark = 1;
    }
  stp_dither_set_ranges(vd, color, nlevels, r, density);
  stp_free(r);
}

void
stp_dither_set_ranges_full(void *vd, int color, int nlevels,
			   const stp_full_dither_range_t *ranges,
			   double density)
{
  dither_t *d = (dither_t *) vd;
  stp_dither_set_generic_ranges_full(d, &(CHANNEL(d, color).dither), nlevels,
				     ranges, density);
}

void
stp_free_dither(void *vd)
{
  dither_t *d = (dither_t *) vd;
  int i;
  int j;
  for (j = 0; j < NCOLORS; j++)
    {
      if (CHANNEL(d, j).vals)
	{
	  stp_free(CHANNEL(d, j).vals);
	  CHANNEL(d, j).vals = NULL;
	}
      for (i = 0; i < ERROR_ROWS; i++)
	{
	  if (CHANNEL(d, j).errs[i])
	    {
	      stp_free(CHANNEL(d, j).errs[i]);
	      CHANNEL(d, j).errs[i] = NULL;
	    }
	}
      stp_free(CHANNEL(d, j).dither.ranges);
      CHANNEL(d, j).dither.ranges = NULL;
      destroy_matrix(&(CHANNEL(d, j).pick));
      destroy_matrix(&(CHANNEL(d, j).dithermat));
    }
  if (d->offset0_table)
    {
      stp_free(d->offset0_table);
      d->offset0_table = NULL;
    }
  if (d->offset1_table)
    {
      stp_free(d->offset1_table);
      d->offset1_table = NULL;
    }
  destroy_matrix(&(d->dither_matrix));
  destroy_matrix(&(d->transition_matrix));
  stp_free(d);
}

int
stp_dither_get_first_position(void *vd, int color, int is_dark)
{
  dither_t *d = (dither_t *) vd;
  if (color < 0 || color >= NCOLORS)
    return -1;
  if (is_dark)
    return CHANNEL(d, color).dither.row_ends[0][0];
  else
    return CHANNEL(d, color).dither.row_ends[0][1];
}

int
stp_dither_get_last_position(void *vd, int color, int is_dark)
{
  dither_t *d = (dither_t *) vd;
  if (color < 0 || color >= NCOLORS)
    return -1;
  if (is_dark)
    return CHANNEL(d, color).dither.row_ends[1][0];
  else
    return CHANNEL(d, color).dither.row_ends[1][1];
}

static int *
get_errline(dither_t *d, int row, int color)
{
  if (row < 0 || color < 0 || color >= NCOLORS)
    return NULL;
  if (CHANNEL(d, color).errs[row & 1])
    return CHANNEL(d, color).errs[row & 1] + MAX_SPREAD;
  else
    {
      int size = 2 * MAX_SPREAD + (16 * ((d->dst_width + 7) / 8));
      CHANNEL(d, color).errs[row & 1] = stp_malloc(size * sizeof(int));
      memset(CHANNEL(d, color).errs[row & 1], 0, size * sizeof(int));
      return CHANNEL(d, color).errs[row & 1] + MAX_SPREAD;
    }
}

static unsigned short *
get_valueline(dither_t *d, int color)
{
  if (color < 0 || color >= NCOLORS)
    return NULL;
  if (CHANNEL(d, color).vals)
    return CHANNEL(d, color).vals;
  else
    {
      int size = (8 * ((d->dst_width + 7) / 8));
      CHANNEL(d, color).vals = stp_malloc(size * sizeof(unsigned short));
      return CHANNEL(d, color).vals;
    }
}

#define ADVANCE_UNIDIRECTIONAL(d, bit, input, width, xerror, xmod)	\
do									\
{									\
  bit >>= 1;								\
  if (bit == 0)								\
    {									\
      d->ptr_offset++;							\
      bit = 128;							\
    }									\
  if (d->src_width == d->dst_width)					\
    input += (width);							\
  else									\
    {									\
      input += xstep;							\
      xerror += xmod;							\
      if (xerror >= d->dst_width)					\
	{								\
	  xerror -= d->dst_width;					\
	  input += (width);						\
	}								\
    }									\
} while (0)

#define ADVANCE_BIDIRECTIONAL(d, bit, in, dir, width, xer, xmod, err, N, S) \
do									    \
{									    \
  int i;								    \
  int j;								    \
  if (dir == 1)								    \
    {									    \
      bit >>= 1;							    \
      if (bit == 0)							    \
	{								    \
	  d->ptr_offset++;						    \
	  bit = 128;							    \
	}								    \
      if (d->src_width == d->dst_width)					    \
	in += (width);							    \
      else								    \
	{								    \
	  in += xstep;							    \
	  xer += xmod;							    \
	  if (xer >= d->dst_width)					    \
	    {								    \
	      xer -= d->dst_width;					    \
	      in += (width);						    \
	    }								    \
	}								    \
    }									    \
  else									    \
    {									    \
      if (bit == 128)							    \
	{								    \
	  d->ptr_offset--;						    \
	  bit = 1;							    \
	}								    \
      else								    \
	bit <<= 1;							    \
      if (d->src_width == d->dst_width)					    \
	in -= (width);							    \
      else								    \
	{								    \
	  in -= xstep;							    \
	  xer -= xmod;							    \
	  if (xer < 0)							    \
	    {								    \
	      xer += d->dst_width;					    \
	      in -= (width);						    \
	    }								    \
	}								    \
    }									    \
  for (i = 0; i < N; i++)						    \
    for (j = 0; j < S; j++)						    \
      err[i][j] += direction;						    \
} while (0)

/*
 * Add the error to the input value.  Notice that we micro-optimize this
 * to save a division when appropriate.
 */

#define UPDATE_COLOR(color, dither) (\
        ((dither) >= 0)? \
                (color) + ((dither) >> 3): \
                (color) - ((-(dither)) >> 3))

/*
 * For Floyd-Steinberg, distribute the error residual.  We spread the
 * error to nearby points, spreading more broadly in lighter regions to
 * achieve more uniform distribution of color.  The actual distribution
 * is a triangular function.
 */

static inline int
update_dither(dither_t *d, int channel, int width,
	      int direction, int *error0, int *error1)
{
  int r = CHANNEL(d, channel).v;
  int o = CHANNEL(d, channel).o;
  int tmp = r;
  int i, dist, dist1;
  int delta, delta1;
  int offset;
  if (tmp == 0)
    return error0[direction];
  if (tmp > 65535)
    tmp = 65535;
  if (d->spread >= 16 || o >= 2048)
    {
      tmp += tmp;
      tmp += tmp;
      error1[0] += tmp;
      return error0[direction] + tmp;
    }
  else
    {
      int tmpo = o << 5;
      offset = ((65535 - tmpo) >> d->spread) +
	((tmp & d->spread_mask) > (tmpo & d->spread_mask));
    }
  switch (offset)
    {
    case 0:
      tmp += tmp;
      tmp += tmp;
      error1[0] += tmp;
      return error0[direction] + tmp;
    case 1:
      error1[-1] += tmp;
      error1[1] += tmp;
      tmp += tmp;
      error1[0] += tmp;
      tmp += tmp;
      return error0[direction] + tmp;
    default:
      tmp += tmp;
      tmp += tmp;
      dist = tmp / d->offset0_table[offset];
      dist1 = tmp / d->offset1_table[offset];
      delta = dist;
      delta1 = dist1;
      for (i = -offset; i; i++)
	{
	  error1[i] += delta;
	  error1[-i] += delta;
	  error0[i] += delta1;
	  error0[-i] += delta1;
	  delta1 += dist1;
	  delta += dist;
	}
      error1[0] += delta;
      return error0[direction];
    }
}

#define USMIN(a, b) ((a) < (b) ? (a) : (b))

static inline int
compute_black(const dither_t *d)
{
  int answer = INT_MAX;
  int i;
  for (i = 1; i < NCOLORS; i++)
    answer = USMIN(answer, CHANNEL(d, i).v);
  return answer;
}

/*
 * Print a single dot.  This routine has become awfully complicated
 * awfully fast!
 */

static inline int
print_color(const dither_t *d, dither_channel_t *dc, int x, int y,
	    unsigned char bit, int length, int dontprint, int dither_type)
{
  int base = dc->b;
  int density = dc->o;
  int adjusted = dc->v;
  dither_color_t *rv = &(dc->dither);
  unsigned randomizer = dc->randomizer;
  dither_matrix_t *pick_matrix = &(dc->pick);
  dither_matrix_t *dither_matrix = &(dc->dithermat);
  unsigned rangepoint = 32768;
  unsigned virtual_value;
  unsigned vmatrix;
  int i;
  int j;
  int isdark;
  unsigned char *tptr;
  unsigned bits;
  unsigned v;
  unsigned dot_size;
  int levels = rv->nlevels - 1;
  int dither_value = adjusted;
  dither_segment_t *dd;

  if (base <= 0 || density <= 0 ||
      (adjusted <= 0 && !(dither_type & D_ADAPTIVE_BASE)))
    return adjusted;
  if (density > 65535)
    density = 65535;

  /*
   * Look for the appropriate range into which the input value falls.
   * Notice that we use the input, not the error, to decide what dot type
   * to print (if any).  We actually use the "density" input to permit
   * the caller to use something other that simply the input value, if it's
   * desired to use some function of overall density, rather than just
   * this color's input, for this purpose.
   */
  for (i = levels; i >= 0; i--)
    {
      dd = &(rv->ranges[i]);

      if (density <= dd->range[0])
	continue;

      /*
       * If we're using an adaptive dithering method, decide whether
       * to use the Floyd-Steinberg or the ordered method based on the
       * input value.
       */
      if (dither_type & D_ADAPTIVE_BASE)
	{
	  dither_type -= D_ADAPTIVE_BASE;

	  if (base <= d->adaptive_limit)
	    {
	      dither_type = D_ORDERED;
	      dither_value = base;
	    }
	  else if (adjusted <= 0)
	    return adjusted;
	}

      /*
       * Where are we within the range.  If we're going to print at
       * all, this determines the probability of printing the darker
       * vs. the lighter ink.  If the inks are identical (same value
       * and darkness), it doesn't matter.
       *
       * We scale the input linearly against the top and bottom of the
       * range.
       */
      if (!dd->is_equal)
	rangepoint =
	  ((unsigned) (density - dd->range[0])) * 65535 / dd->range_span;

      /*
       * Compute the virtual dot size that we're going to print.
       * This is somewhere between the two candidate dot sizes.
       * This is scaled between the high and low value.
       */

      if (dd->value_span == 0)
	virtual_value = dd->value[1];
      else if (dd->range_span == 0)
	virtual_value = (dd->value[1] + dd->value[0]) / 2;
      else
	virtual_value = dd->value[0] +
	  (dd->value_span * d->virtual_dot_scale[rangepoint] / 65535);

      /*
       * Reduce the randomness as the base value increases, to get
       * smoother output in the midtones.  Idea suggested by
       * Thomas Tonino.
       */
      if (dither_type & D_ORDERED_BASE)
	randomizer = 65535;	/* With ordered dither, we need this */
      else if (randomizer > 0)
	{
	  if (base > d->d_cutoff)
	    randomizer = 0;
	  else if (base > d->d_cutoff / 2)
	    randomizer = randomizer * 2 * (d->d_cutoff - base) / d->d_cutoff;
	}

      /*
       * Compute the comparison value to decide whether to print at
       * all.  If there is no randomness, simply divide the virtual
       * dotsize by 2 to get standard "pure" Floyd-Steinberg (or "pure"
       * matrix dithering, which degenerates to a threshold).
       */
      if (randomizer == 0)
	vmatrix = virtual_value / 2;
      else
	{
	  /*
	   * First, compute a value between 0 and 65535 that will be
	   * scaled to produce an offset from the desired threshold.
	   */
	  vmatrix = ditherpoint(d, dither_matrix, x);
	  /*
	   * Now, scale the virtual dot size appropriately.  Note that
	   * we'll get something evenly distributed between 0 and
	   * the virtual dot size, centered on the dot size / 2,
	   * which is the normal threshold value.
	   */
	  vmatrix = vmatrix * virtual_value / 65535;
	  if (randomizer != 65535)
	    {
	      /*
	       * We want vmatrix to be scaled between 0 and
	       * virtual_value when randomizer is 65535 (fully random).
	       * When it's less, we want it to scale through part of
	       * that range. In all cases, it should center around
	       * virtual_value / 2.
	       *
	       * vbase is the bottom of the scaling range.
	       */
	      unsigned vbase = virtual_value * (65535u - randomizer) /
		131070u;
	      vmatrix = vmatrix * randomizer / 65535;
	      vmatrix += vbase;
	    }
	} /* randomizer != 0 */

      /*
       * After all that, printing is almost an afterthought.
       * Pick the actual dot size (using a matrix here) and print it.
       */
      if (dither_value >= vmatrix)
	{
	  int subchannel;

	  if (dd->is_same_ink)
	    subchannel = 1;
	  else
	    {
	      rangepoint = rangepoint * rv->density / 65535u;
	      if (rangepoint >= ditherpoint(d, pick_matrix, x))
		subchannel = 1;
	      else
		subchannel = 0;
	    }
	  isdark = dd->isdark[subchannel];
	  bits = dd->bits[subchannel];
	  v = dd->value[subchannel];
	  dot_size = dd->dot_size[subchannel];
	  tptr = dc->ptrs[1 - isdark] + d->ptr_offset;

	  /*
	   * Lay down all of the bits in the pixel.
	   */
	  if (dontprint < v)
	    {
	      if (rv->row_ends[0][1 - isdark] == -1)
		rv->row_ends[0][1 - isdark] = x;
	      rv->row_ends[1][1 - isdark] = x;
	      for (j = 1; j <= bits; j += j, tptr += length)
		{
		  if (j & bits)
		    tptr[0] |= bit;
		}
	    }
	  if (dither_type & D_ORDERED_BASE)
	    adjusted = -(int) v / 2;
	  else
	    adjusted -= v;
	}
      return adjusted;
    }
  return adjusted;
}

static inline int
print_color_ordered(const dither_t *d, dither_channel_t *dc, int x, int y,
		    unsigned char bit, int length, int dontprint)
{
  int density = dc->o;
  int adjusted = dc->v;
  dither_color_t *rv = &(dc->dither);
  dither_matrix_t *pick_matrix = &(dc->pick);
  dither_matrix_t *dither_matrix = &(dc->dithermat);
  unsigned rangepoint;
  unsigned virtual_value;
  unsigned vmatrix;
  int i;
  int j;
  int isdark;
  unsigned char *tptr;
  unsigned bits;
  unsigned v;
  unsigned dot_size;
  int levels = rv->nlevels - 1;
  int dither_value = adjusted;
  dither_segment_t *dd;

  if (adjusted <= 0 || density <= 0)
    return 0;
  if (density > 65535)
    density = 65535;

  /*
   * Look for the appropriate range into which the input value falls.
   * Notice that we use the input, not the error, to decide what dot type
   * to print (if any).  We actually use the "density" input to permit
   * the caller to use something other that simply the input value, if it's
   * desired to use some function of overall density, rather than just
   * this color's input, for this purpose.
   */
  for (i = levels; i >= 0; i--)
    {
      dd = &(rv->ranges[i]);

      if (density <= dd->range[0])
	continue;

      /*
       * Where are we within the range.  If we're going to print at
       * all, this determines the probability of printing the darker
       * vs. the lighter ink.  If the inks are identical (same value
       * and darkness), it doesn't matter.
       *
       * We scale the input linearly against the top and bottom of the
       * range.
       */
      if (dd->is_equal)
	rangepoint = 32768;
      else
	rangepoint =
	  ((unsigned) (density - dd->range[0])) * 65535 / dd->range_span;

      /*
       * Compute the virtual dot size that we're going to print.
       * This is somewhere between the two candidate dot sizes.
       * This is scaled between the high and low value.
       */

      if (dd->value_span == 0)
	virtual_value = dd->value[1];
      else if (dd->range_span == 0)
	virtual_value = (dd->value[1] + dd->value[0]) / 2;
      else
	virtual_value = dd->value[0] +
	  (dd->value_span * d->virtual_dot_scale[rangepoint] / 65535);

      /*
       * Compute the comparison value to decide whether to print at
       * all.  If there is no randomness, simply divide the virtual
       * dotsize by 2 to get standard "pure" Floyd-Steinberg (or "pure"
       * matrix dithering, which degenerates to a threshold).
       */
      /*
       * First, compute a value between 0 and 65535 that will be
       * scaled to produce an offset from the desired threshold.
       */
      vmatrix = ditherpoint(d, dither_matrix, x);
      /*
       * Now, scale the virtual dot size appropriately.  Note that
       * we'll get something evenly distributed between 0 and
       * the virtual dot size, centered on the dot size / 2,
       * which is the normal threshold value.
       */
      vmatrix = vmatrix * virtual_value / 65535;

      /*
       * After all that, printing is almost an afterthought.
       * Pick the actual dot size (using a matrix here) and print it.
       */
      if (dither_value >= vmatrix)
	{
	  int subchannel;

	  if (dd->is_same_ink)
	    subchannel = 1;
	  else
	    {
	      rangepoint = rangepoint * rv->density / 65535u;
	      if (rangepoint >= ditherpoint(d, pick_matrix, x))
		subchannel = 1;
	      else
		subchannel = 0;
	    }
	  isdark = dd->isdark[subchannel];
	  bits = dd->bits[subchannel];
	  v = dd->value[subchannel];
	  dot_size = dd->dot_size[subchannel];
	  tptr = dc->ptrs[1 - isdark] + d->ptr_offset;

	  /*
	   * Lay down all of the bits in the pixel.
	   */
	  if (dontprint < v)
	    {
	      if (rv->row_ends[0][1 - isdark] == -1)
		rv->row_ends[0][1 - isdark] = x;
	      rv->row_ends[1][1 - isdark] = x;
	      for (j = 1; j <= bits; j += j, tptr += length)
		{
		  if (j & bits)
		    tptr[0] |= bit;
		}
	      return v;
	    }
	}
      return 0;
    }
  return 0;
}

static inline void
print_color_fast(const dither_t *d, dither_channel_t *dc, int x, int y,
		 unsigned char bit, int length)
{
  int density = dc->o;
  int adjusted = dc->v;
  dither_color_t *rv = &(dc->dither);
  dither_matrix_t *dither_matrix = &(dc->dithermat);

  if (density <= 0 || adjusted <= 0)
    return;
  if (dc->very_fast)
    {
      if (adjusted > ditherpoint(d, dither_matrix, x))
	{
	  if (rv->row_ends[0][0] == -1)
	    rv->row_ends[0][0] = x;
	  rv->row_ends[1][0] = x;
	  dc->ptrs[0][d->ptr_offset] |= bit;
	}
    }
  else
    {
      int i;
      int levels = rv->nlevels - 1;
      int j;
      unsigned char *tptr;
      unsigned bits;
      for (i = levels; i >= 0; i--)
	{
	  dither_segment_t *dd = &(rv->ranges[i]);
	  unsigned vmatrix;
	  unsigned rangepoint;
	  unsigned dpoint;
	  unsigned subchannel;
	  if (density <= dd->range[0])
	    continue;
	  dpoint = ditherpoint(d, dither_matrix, x);

	  if (dd->is_same_ink)
	    subchannel = 1;
	  else
	    {
	      rangepoint = ((density - dd->range[0]) << 16) / dd->range_span;
	      rangepoint = (rangepoint * rv->density) >> 16;
	      if (rangepoint >= dpoint)
		subchannel = 1;
	      else
		subchannel = 0;
	    }
	  vmatrix = (dd->value[subchannel] * dpoint) >> 16;

	  /*
	   * After all that, printing is almost an afterthought.
	   * Pick the actual dot size (using a matrix here) and print it.
	   */
	  if (adjusted >= vmatrix)
	    {
	      int isdark = dd->isdark[subchannel];
	      bits = dd->bits[subchannel];
	      tptr = dc->ptrs[1 - isdark] + d->ptr_offset;
	      if (rv->row_ends[0][1 - isdark] == -1)
		rv->row_ends[0][1 - isdark] = x;
	      rv->row_ends[1][1 - isdark] = x;

	      /*
	       * Lay down all of the bits in the pixel.
	       */
	      if (bits == 1)
		{
		  tptr[0] |= bit;
		}
	      else
		{
		  for (j = 1; j <= bits; j += j, tptr += length)
		    {
		      if (j & bits)
			tptr[0] |= bit;
		    }
		}
	    }
	  return;
	}
    }
}

static inline void
update_cmyk(dither_t *d)
{
  int ak;
  int i;
  int kdarkness = 0;
  unsigned ks, kl;
  int ub, lb;
  int ok;
  int bk;
  unsigned density;
  int k = CHANNEL(d, ECOLOR_K).o;

  ub = d->k_upper;
  lb = d->k_lower;
  density = d->density;

  /*
   * Calculate total ink amount.
   * If there is a lot of ink, black gets added sooner. Saves ink
   * and with a lot of ink the black doesn't show as speckles.
   *
   * k already contains the grey contained in CMY.
   * First we find out if the color is darker than the K amount
   * suggests, and we look up where is value is between
   * lowerbound and density:
   */

  for (i = 1; i < NCOLORS; i++)
    kdarkness += CHANNEL(d, i).o * CHANNEL(d, i).darkness / 64;
  kdarkness -= d->density2;

  if (kdarkness > (k + k + k))
    ok = kdarkness / 3;
  else
    ok = k;
  if (ok <= lb)
    kl = 0;
  else if (ok >= density)
    kl = density;
  else
    kl = (unsigned) ( ok - lb ) * density / d->dlb_range;

  /*
   * We have a second value, ks, that will be the scaler.
   * ks is initially showing where the original black
   * amount is between upper and lower bounds:
   */

  if (k >= ub)
    ks = density;
  else if (k <= lb)
    ks = 0;
  else
    ks = (unsigned) (k - lb) * density / d->bound_range;

  /*
   * ks is then processed by a second order function that produces
   * an S curve: 2ks - ks^2. This is then multiplied by the
   * darkness value in kl. If we think this is too complex the
   * following line can be tried instead:
   * ak = ks;
   */
  ak = ks;
  if (kl == 0 || ak == 0)
    k = 0;
  else if (ak == density)
    k = kl;
  else
    k = (unsigned) kl * (unsigned) ak / density;
  ok = k;
  bk = k;
  if (bk > 0 && density != d->black_density)
    bk = (unsigned) bk * (unsigned) d->black_density / density;
  if (bk > 65535)
    bk = 65535;

  if (k && ak && ok > 0)
    {
      int i;
      /*
       * Because black is always fairly neutral, we do not have to
       * calculate the amount to take out of CMY. The result will
       * be a bit dark but that is OK. If things are okay CMY
       * cannot go negative here - unless extra K is added in the
       * previous block. We multiply by ak to prevent taking out
       * too much. This prevents dark areas from becoming very
       * dull.
       */

      if (ak == density)
	ok = k;
      else
	ok = (unsigned) k * (unsigned) ak / density;

      for (i = 1; i < NCOLORS; i++)
	{
	  if (CHANNEL(d, i).k_level == 64)
	    CHANNEL(d, i).v -= ok;
	  else
	    CHANNEL(d, i).v -= (ok * CHANNEL(d, i).k_level) >> 6;
	  if (CHANNEL(d, i).v < 0)
	    CHANNEL(d, i).v = 0;
	}
    }
  else
    for (i = 1; i < NCOLORS; i++)
      CHANNEL(d, i).v = CHANNEL(d, i).o;
  CHANNEL(d, ECOLOR_K).b = bk;
  CHANNEL(d, ECOLOR_K).v = k;
}

/*
 * Dithering functions!
 *
 * Documentation moved to README.dither
 */

/*
 * 'stp_dither_monochrome()' - Dither grayscale pixels to black using a hard
 * threshold.  This is for use with predithered output, or for text
 * or other pure black and white only.
 */

static void
stp_dither_monochrome(const unsigned short  *gray,
		      int           	    row,
		      void 		    *vd,
		      int		    duplicate_line,
		      int		  zero_mask)
{
  int		x,
		xerror,
		xstep,
		xmod,
		length;
  unsigned char	bit,
		*kptr;
  dither_t *d = (dither_t *) vd;
  dither_matrix_t *kdither = &(CHANNEL(d, ECOLOR_K).dithermat);
  unsigned bits = CHANNEL(d, ECOLOR_K).dither.signif_bits;
  dither_color_t *rv = &(CHANNEL(d, ECOLOR_K).dither);
  int j;
  unsigned char *tptr;
  int dst_width = d->dst_width;
  if (zero_mask)
    return;

  kptr = CHANNEL(d, ECOLOR_K).ptrs[0];
  length = (d->dst_width + 7) / 8;

  bit = 128;
  x = 0;

  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  for (x = 0; x < dst_width; x++)
    {
      if (gray[0] && (d->density >= ditherpoint(d, kdither, x)))
	{
	  tptr = kptr + d->ptr_offset;
	  if (rv->row_ends[0][0] == -1)
	    rv->row_ends[0][0] = x;
	  rv->row_ends[1][0] = x;
	  for (j = 0; j < bits; j++, tptr += length)
	    tptr[0] |= bit;
	}
      ADVANCE_UNIDIRECTIONAL(d, bit, gray, 1, xerror, xmod);
    }
}

/*
 * 'stp_dither_black()' - Dither grayscale pixels to black.
 * This is for grayscale output.
 */

static void
stp_dither_black_fast(const unsigned short   *gray,
		      int           	row,
		      void 		*vd,
		      int		duplicate_line,
		      int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  dither_t *d = (dither_t *) vd;
  int dst_width = d->dst_width;
  int xerror, xstep, xmod;

  if (zero_mask)
    return;
  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  xerror = 0;

  for (x = 0; x < dst_width; x++)
    {
      CHANNEL(d, ECOLOR_K).v = gray[0];
      CHANNEL(d, ECOLOR_K).o = gray[0];
      print_color_fast(d, &(CHANNEL(d, ECOLOR_K)), x, row, bit, length);
      ADVANCE_UNIDIRECTIONAL(d, bit, gray, 1, xerror, xmod);
    }
}

static void
stp_dither_black_ordered(const unsigned short   *gray,
			 int           	row,
			 void 		*vd,
			 int		duplicate_line,
			 int		  zero_mask)
{

  int		x,
		length;
  unsigned char	bit;
  dither_t *d = (dither_t *) vd;
  int terminate;
  int xerror, xstep, xmod;

  if (zero_mask)
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  x = 0;
  terminate = d->dst_width;
  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  xerror = 0;

  for (x = 0; x < terminate; x ++)
    {
      CHANNEL(d, ECOLOR_K).o = CHANNEL(d, ECOLOR_K).v = gray[0];
      print_color_ordered(d, &(CHANNEL(d, ECOLOR_K)), x, row, bit, length, 0);
      ADVANCE_UNIDIRECTIONAL(d, bit, gray, 1, xerror, xmod);
    }
}

static void
stp_dither_black_ed(const unsigned short   *gray,
		    int           	row,
		    void 		*vd,
		    int		duplicate_line,
		    int		  zero_mask)
{

  int		x,
		length;
  unsigned char	bit;
  int		*error[1][ERROR_ROWS];
  int		ditherk;
  dither_t *d = (dither_t *) vd;
  int terminate;
  int direction = row & 1 ? 1 : -1;
  int xerror, xstep, xmod;

  if (!duplicate_line)
    {
      if (!zero_mask)
	d->last_line_was_empty = 0;
      else
	d->last_line_was_empty++;
    }
  else if (d->last_line_was_empty)
    d->last_line_was_empty++;
  if (d->last_line_was_empty >= 5)
    return;

  error[ECOLOR_K][0] = get_errline(d, row, ECOLOR_K);
  error[ECOLOR_K][1] = get_errline(d, row + 1, ECOLOR_K);

  memset(error[ECOLOR_K][1], 0, d->dst_width * sizeof(int));

  if (d->last_line_was_empty >= 4)
    {
      if (d->last_line_was_empty == 4)
	memset(error[ECOLOR_K][0], 0, d->dst_width * sizeof(int));
      return;
    }

  length = (d->dst_width + 7) / 8;

  x = (direction == 1) ? 0 : d->dst_width - 1;
  bit = 1 << (7 - (x & 7));
  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;
  terminate = (direction == 1) ? d->dst_width : -1;
  if (direction == -1)
    {
      gray += d->src_width - 1;
      error[ECOLOR_K][0] += x;
      error[ECOLOR_K][1] += x;
      d->ptr_offset = length - 1;
    }
  ditherk = error[ECOLOR_K][0][0];

  for (; x != terminate; x += direction)
    {
      CHANNEL(d, ECOLOR_K).b = gray[0];
      CHANNEL(d, ECOLOR_K).o = gray[0];
      CHANNEL(d, ECOLOR_K).v = UPDATE_COLOR(gray[0], ditherk);
      CHANNEL(d, ECOLOR_K).v = print_color(d, &(CHANNEL(d, ECOLOR_K)), x, row,
					   bit, length, 0, d->dither_type);
      ditherk = update_dither(d, ECOLOR_K, d->src_width, direction,
			      error[ECOLOR_K][0], error[ECOLOR_K][1]);
      ADVANCE_BIDIRECTIONAL(d, bit, gray, direction, 1, xerror, xmod, error,
			    1, ERROR_ROWS);
    }
  if (direction == -1)
    reverse_row_ends(d);
}

static void
stp_dither_cmy_fast(const unsigned short  *cmy,
		    int           row,
		    void 	    *vd,
		    int	       duplicate_line,
		    int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  dither_t	*d = (dither_t *) vd;
  int i;
  int dst_width = d->dst_width;
  int xerror, xstep, xmod;

  if ((zero_mask & 7) == 7)
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  x = 0;

  QUANT(14);
  for (; x != dst_width; x++)
    {
      CHANNEL(d, ECOLOR_C).v = CHANNEL(d, ECOLOR_C).o = cmy[0];
      CHANNEL(d, ECOLOR_M).v = CHANNEL(d, ECOLOR_M).o = cmy[1];
      CHANNEL(d, ECOLOR_Y).v = CHANNEL(d, ECOLOR_Y).o = cmy[2];

      for (i = 1; i < NCOLORS; i++)
	print_color_fast(d, &(CHANNEL(d, i)), x, row, bit, length);
      QUANT(16);
      ADVANCE_UNIDIRECTIONAL(d, bit, cmy, 3, xerror, xmod);
      QUANT(17);
    }
}

static void
stp_dither_cmy_ordered(const unsigned short  *cmy,
		       int           row,
		       void 	    *vd,
		       int		  duplicate_line,
		       int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  dither_t *d = (dither_t *) vd;
  int i;

  int terminate;
  int xerror, xstep, xmod;

  if ((zero_mask & 7) == 7)
    return;
  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  x = 0;
  terminate = d->dst_width;

  QUANT(6);
  for (; x != terminate; x ++)
    {
      CHANNEL(d, ECOLOR_C).v = CHANNEL(d, ECOLOR_C).o = cmy[0];
      CHANNEL(d, ECOLOR_M).v = CHANNEL(d, ECOLOR_M).o = cmy[1];
      CHANNEL(d, ECOLOR_Y).v = CHANNEL(d, ECOLOR_Y).o = cmy[2];
      QUANT(9);
      for (i = 1; i < NCOLORS; i++)
	print_color_ordered(d, &(CHANNEL(d, i)), x, row, bit, length, 0);
      QUANT(12);
      ADVANCE_UNIDIRECTIONAL(d, bit, cmy, 3, xerror, xmod);
      QUANT(13);
  }
}

static void
stp_dither_cmy_ed(const unsigned short  *cmy,
		  int           row,
		  void 	    *vd,
		  int		  duplicate_line,
		  int		  zero_mask)
{
  int		x,
    		length;
  unsigned char	bit;
  int		i, j;
  int		ndither[NCOLORS];
  int		*error[NCOLORS][ERROR_ROWS];
  dither_t	*d = (dither_t *) vd;

  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int xerror, xstep, xmod;
  if (!duplicate_line)
    {
      if ((zero_mask & 7) != 7)
	d->last_line_was_empty = 0;
      else
	d->last_line_was_empty++;
    }
  else if (d->last_line_was_empty)
    d->last_line_was_empty++;
  if (d->last_line_was_empty >= 5)
    return;

  for (i = 1; i < NCOLORS; i++)
    {
      for (j = 0; j < ERROR_ROWS; j++)
	error[i][j] = get_errline(d, row + j, i);
      memset(error[i][j - 1], 0, d->dst_width * sizeof(int));
    }
  if (d->last_line_was_empty >= 4)
    {
      if (d->last_line_was_empty == 4)
	{
	  for (i = 1; i < NCOLORS; i++)
	    {
	      for (j = 0; j < ERROR_ROWS - 1; j++)
		memset(error[i][j], 0, d->dst_width * sizeof(int));
	    }
	}
      return;
    }

  length = (d->dst_width + 7) / 8;

  x = (direction == 1) ? 0 : d->dst_width - 1;
  bit = 1 << (7 - (x & 7));
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;
  terminate = (direction == 1) ? d->dst_width : -1;
  if (direction == -1)
    {
      cmy += (3 * (d->src_width - 1));
      for (i = 1; i < NCOLORS; i++)
	{
	  for (j = 0; j < ERROR_ROWS; j++)
	    error[i][j] += d->dst_width - 1;
	}
      d->ptr_offset = length - 1;
    }
  for (i = 1; i < NCOLORS; i++)
    ndither[i] = error[i][0][0];
  QUANT(6);
  for (; x != terminate; x += direction)
    {
      CHANNEL(d, ECOLOR_C).v = cmy[0];
      CHANNEL(d, ECOLOR_M).v = cmy[1];
      CHANNEL(d, ECOLOR_Y).v = cmy[2];

      for (i = 1; i < NCOLORS; i++)
	{
	  QUANT(9);
	  CHANNEL(d, i).o = CHANNEL(d, i).b = CHANNEL(d, i).v;
	  CHANNEL(d, i).v = UPDATE_COLOR(CHANNEL(d, i).v, ndither[i]);
	  CHANNEL(d, i).v = print_color(d, &(CHANNEL(d, i)), x, row, bit,
					length, 0, d->dither_type);
	  ndither[i] = update_dither(d, i, d->src_width,
				     direction, error[i][0], error[i][1]);
	  QUANT(10);
	}

      QUANT(12);
      ADVANCE_BIDIRECTIONAL(d, bit, cmy, direction, 3, xerror, xmod, error,
			    NCOLORS, ERROR_ROWS);
      QUANT(13);
    }
  if (direction == -1)
    reverse_row_ends(d);
}

static void
stp_dither_cmyk_fast(const unsigned short  *cmy,
		     int           row,
		     void 	    *vd,
		     int	       duplicate_line,
		     int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  dither_t	*d = (dither_t *) vd;
  int i;

  int dst_width = d->dst_width;
  int xerror, xstep, xmod;

  if ((zero_mask & 7) == 7)
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  x = 0;

  QUANT(14);
  for (; x != dst_width; x++)
    {
      int nonzero = 0;
      nonzero |= CHANNEL(d, ECOLOR_C).v = cmy[0];
      nonzero |= CHANNEL(d, ECOLOR_M).v = cmy[1];
      nonzero |= CHANNEL(d, ECOLOR_Y).v = cmy[2];
      CHANNEL(d, ECOLOR_C).o = cmy[0];
      CHANNEL(d, ECOLOR_M).o = cmy[1];
      CHANNEL(d, ECOLOR_Y).o = cmy[2];

      if (nonzero)
	{
	  int ok;
	  unsigned lb = d->k_lower;
	  unsigned ub = d->k_upper;
	  int k = compute_black(d);
	  if (d->dither_type != D_VERY_FAST)
	    {
	      if (k < lb)
		k = 0;
	      else if (k < ub)
		k = (k - lb) * ub / d->bound_range;
	    }
	  for (i = 1; i < NCOLORS; i++)
	    CHANNEL(d, i).v -= k;
	  ok = k;
	  if (d->dither_type != D_VERY_FAST)
	    {
	      if (ok > 0 && d->density != d->black_density)
		ok = (unsigned) ok * (unsigned) d->black_density / d->density;
	      if (ok > 65535)
		ok = 65535;
	    }
	  QUANT(15);
	  CHANNEL(d, ECOLOR_K).v = k;
	  CHANNEL(d, ECOLOR_K).o = ok;

	  for (i = 0; i < NCOLORS; i++)
	    print_color_fast(d, &(CHANNEL(d, i)), x, row, bit, length);
	  QUANT(16);
	}
      ADVANCE_UNIDIRECTIONAL(d, bit, cmy, 3, xerror, xmod);
      QUANT(17);
    }
}

static void
stp_dither_cmyk_ordered(const unsigned short  *cmy,
			int           row,
			void 	    *vd,
			int		  duplicate_line,
			int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  dither_t	*d = (dither_t *) vd;
  int i;

  int		terminate;
  int xerror, xstep, xmod;

  if ((zero_mask & 7) == 7)
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  x = 0;
  terminate = d->dst_width;

  QUANT(6);
  for (; x != terminate; x ++)
    {
      int nonzero = 0;
      int printed_black = 0;
      CHANNEL(d, ECOLOR_C).v = cmy[0];
      CHANNEL(d, ECOLOR_M).v = cmy[1];
      CHANNEL(d, ECOLOR_Y).v = cmy[2];
      for (i = 0; i < NCOLORS; i++)
	nonzero |= CHANNEL(d, i).o = CHANNEL(d, i).v;

      if (nonzero)
	{
	  QUANT(7);

	  CHANNEL(d, ECOLOR_K).o = CHANNEL(d, ECOLOR_K).v = compute_black(d);

	  if (CHANNEL(d, ECOLOR_K).v > 0)
	    update_cmyk(d);

	  QUANT(9);

	  if (d->density != d->black_density)
	    CHANNEL(d, ECOLOR_K).v =
	      CHANNEL(d, ECOLOR_K).v * d->black_density / d->density;

	  for (i = 0; i < NCOLORS; i++)
	    {
	      int tmp = print_color_ordered(d, &(CHANNEL(d, i)), x, row, bit,
					    length, printed_black);
	      if (i == ECOLOR_K && d->density <= 45000)
		printed_black = CHANNEL(d, i).v - tmp;
	    }
	  QUANT(12);
	}
      ADVANCE_UNIDIRECTIONAL(d, bit, cmy, 3, xerror, xmod);
      QUANT(13);
  }
}

static void
stp_dither_cmyk_ed(const unsigned short  *cmy,
		   int           row,
		   void 	    *vd,
		   int		  duplicate_line,
		   int		  zero_mask)
{
  int		x,
	        length;
  unsigned char	bit;
  int		i, j;
  int		ndither[NCOLORS];
  int		*error[NCOLORS][ERROR_ROWS];
  dither_t	*d = (dither_t *) vd;

  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int xerror, xstep, xmod;

  if (!duplicate_line)
    {
      if ((zero_mask & 7) != 7)
	d->last_line_was_empty = 0;
      else
	d->last_line_was_empty++;
    }
  else if (d->last_line_was_empty)
    d->last_line_was_empty++;
  if (d->last_line_was_empty >= 5)
    return;
  length = (d->dst_width + 7) / 8;

  for (i = 0; i < NCOLORS; i++)
    {
      for (j = 0; j < ERROR_ROWS; j++)
	error[i][j] = get_errline(d, row + j, i);
      memset(error[i][j - 1], 0, d->dst_width * sizeof(int));
    }
  if (d->last_line_was_empty >= 4)
    {
      if (d->last_line_was_empty == 4)
	{
	  for (i = 0; i < NCOLORS; i++)
	    {
	      for (j = 0; j < ERROR_ROWS - 1; j++)
		memset(error[i][j], 0, d->dst_width * sizeof(int));
	    }
	}
      return;
    }

  x = (direction == 1) ? 0 : d->dst_width - 1;
  bit = 1 << (7 - (x & 7));
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;
  terminate = (direction == 1) ? d->dst_width : -1;
  if (direction == -1)
    {
      cmy += (3 * (d->src_width - 1));
      for (i = 0; i < NCOLORS; i++)
	for (j = 0; j < ERROR_ROWS; j++)
	  error[i][j] += d->dst_width - 1;
      d->ptr_offset = length - 1;
    }
  for (i = 0; i < NCOLORS; i++)
    ndither[i] = error[i][0][0];
  QUANT(6);
  for (; x != terminate; x += direction)
    {
      int nonzero = 0;
      int printed_black = 0;
      CHANNEL(d, ECOLOR_C).v = cmy[0];
      CHANNEL(d, ECOLOR_M).v = cmy[1];
      CHANNEL(d, ECOLOR_Y).v = cmy[2];
      for (i = 0; i < NCOLORS; i++)
	nonzero |= (CHANNEL(d, i).o = CHANNEL(d, i).v);

      if (nonzero)
	{
	  QUANT(7);

	  CHANNEL(d, ECOLOR_K).v = compute_black(d);
	  CHANNEL(d, ECOLOR_K).o = CHANNEL(d, ECOLOR_K).v;
	  CHANNEL(d, ECOLOR_K).b = 0;

	  /*
	   * At this point we've computed the basic CMYK separations.
	   * Now we adjust the levels of each to improve the print quality.
	   */

	  if (CHANNEL(d, ECOLOR_K).v > 0)
	    update_cmyk(d);

	  for (i = 1; i < NCOLORS; i++)
	    CHANNEL(d, i).b = CHANNEL(d, i).v;

	  QUANT(8);
	  /*
	   * We've done all of the cmyk separations at this point.
	   * Now to do the dithering.
	   *
	   * At this point:
	   *
	   * bk = Amount of black printed with black ink
	   * ak = Adjusted "raw" K value
	   * k = raw K value derived from CMY
	   * oc, om, oy = raw CMY values assuming no K component
	   * c, m, y = CMY values adjusted for the presence of K
	   *
	   * The main reason for this rather elaborate setup, where we have
	   * 8 channels at this point, is to handle variable intensities
	   * (in particular light and dark variants) of inks. Very dark regions
	   * with slight color tints should be printed with dark inks, not with
	   * the light inks that would be implied by the small amount of
	   * remnant CMY.
	   *
	   * It's quite likely that for simple four-color printers ordinary
	   * CMYK separations would work.  It's possible that they would work
	   * for variable dot sizes, too.
	   */

	  QUANT(9);

	  if (d->density != d->black_density)
	    CHANNEL(d, ECOLOR_K).v =
	      CHANNEL(d, ECOLOR_K).v * d->black_density / d->density;

	  CHANNEL(d, ECOLOR_K).o = CHANNEL(d, ECOLOR_K).b;

	  for (i = 0; i < NCOLORS; i++)
	    {
	      int tmp;
	      CHANNEL(d, i).v = UPDATE_COLOR(CHANNEL(d, i).v, ndither[i]);
	      tmp = print_color(d, &(CHANNEL(d, i)), x, row, bit, length,
				printed_black, d->dither_type);
	      if (i == ECOLOR_K && d->density <= 45000)
		printed_black = CHANNEL(d, i).v - tmp;
	      CHANNEL(d, i).v = tmp;
	    }
	}
      else
	for (i = 0; i < NCOLORS; i++)
	  CHANNEL(d, i).v = UPDATE_COLOR(CHANNEL(d, i).v, ndither[i]);

      QUANT(11);
      for (i = 0; i < NCOLORS; i++)
	ndither[i] = update_dither(d, i, d->src_width,
				   direction, error[i][0], error[i][1]);

      QUANT(12);
      ADVANCE_BIDIRECTIONAL(d, bit, cmy, direction, 3, xerror, xmod, error,
			    NCOLORS, ERROR_ROWS);
      QUANT(13);
    }
  if (direction == -1)
    reverse_row_ends(d);
}

static void
stp_dither_raw_cmyk_fast(const unsigned short  *cmyk,
			 int           row,
			 void 	    *vd,
			 int	       duplicate_line,
			 int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  dither_t	*d = (dither_t *) vd;
  int i;

  int dst_width = d->dst_width;
  int xerror, xstep, xmod;
  if ((zero_mask & 7) == 7)
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = 4 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  x = 0;

  QUANT(14);
  for (; x != dst_width; x++)
    {
      int extra_k;
      CHANNEL(d, ECOLOR_C).v = cmyk[0];
      CHANNEL(d, ECOLOR_M).v = cmyk[1];
      CHANNEL(d, ECOLOR_Y).v = cmyk[2];
      CHANNEL(d, ECOLOR_K).v = cmyk[3];
      extra_k = compute_black(d) + CHANNEL(d, ECOLOR_K).v;
      for (i = 0; i < NCOLORS; i++)
	{
	  CHANNEL(d, i).o = CHANNEL(d, i).v;
	  if (i != ECOLOR_K)
	    CHANNEL(d, i).o += extra_k;
	  if (CHANNEL(d, i).ptrs[0])
	    print_color_fast(d, &(CHANNEL(d, i)), x, row, bit, length);
	}
      QUANT(16);
      ADVANCE_UNIDIRECTIONAL(d, bit, cmyk, 4, xerror, xmod);
      QUANT(17);
    }
}

static void
stp_dither_raw_cmyk_ordered(const unsigned short  *cmyk,
			    int           row,
			    void 	    *vd,
			    int		  duplicate_line,
			    int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  dither_t	*d = (dither_t *) vd;
  int i;

  int		terminate;
  int xerror, xstep, xmod;

  if ((zero_mask & 7) == 7)
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = 4 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  x = 0;
  terminate = d->dst_width;

  QUANT(6);
  for (; x != terminate; x ++)
    {
      int extra_k;
      CHANNEL(d, ECOLOR_K).v = cmyk[3];
      CHANNEL(d, ECOLOR_C).v = cmyk[0];
      CHANNEL(d, ECOLOR_M).v = cmyk[1];
      CHANNEL(d, ECOLOR_Y).v = cmyk[2];
      extra_k = compute_black(d) + CHANNEL(d, ECOLOR_K).v;
      for (i = 0; i < NCOLORS; i++)
	{
	  CHANNEL(d, i).o = CHANNEL(d, i).v;
	  if (i != ECOLOR_K)
	    CHANNEL(d, i).o += extra_k;
	  print_color_ordered(d, &(CHANNEL(d, i)), x, row, bit, length, 0);
	}

      QUANT(11);
      ADVANCE_UNIDIRECTIONAL(d, bit, cmyk, 4, xerror, xmod);
      QUANT(13);
  }
}


static void
stp_dither_raw_cmyk_ed(const unsigned short  *cmyk,
		       int           row,
		       void 	    *vd,
		       int		  duplicate_line,
		       int		  zero_mask)
{
  int		x,
    		length;
  unsigned char	bit;
  int		i, j;
  int		ndither[NCOLORS];
  int		*error[NCOLORS][ERROR_ROWS];
  dither_t	*d = (dither_t *) vd;

  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int xerror, xstep, xmod;

  if (!duplicate_line)
    {
      if ((zero_mask & 7) != 7)
	d->last_line_was_empty = 0;
      else
	d->last_line_was_empty++;
    }
  else if (d->last_line_was_empty)
    d->last_line_was_empty++;
  if (d->last_line_was_empty >= 5)
    return;

  for (i = 0; i < NCOLORS; i++)
    {
      for (j = 0; j < ERROR_ROWS; j++)
	error[i][j] = get_errline(d, row + j, i);
      memset(error[i][j - 1], 0, d->dst_width * sizeof(int));
    }
  if (d->last_line_was_empty >= 4)
    {
      if (d->last_line_was_empty == 4)
	for (i = 0; i < NCOLORS; i++)
	  for (j = 0; j < ERROR_ROWS - 1; j++)
	    memset(error[i][j], 0, d->dst_width * sizeof(int));
      return;
    }

  length = (d->dst_width + 7) / 8;

  x = (direction == 1) ? 0 : d->dst_width - 1;
  bit = 1 << (7 - (x & 7));
  xstep  = 4 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;
  terminate = (direction == 1) ? d->dst_width : -1;
  if (direction == -1)
    {
      cmyk += (4 * (d->src_width - 1));
      for (i = 0; i < NCOLORS; i++)
	{
	  for (j = 0; j < ERROR_ROWS; j++)
	    error[i][j] += d->dst_width - 1;
	}
      d->ptr_offset = length - 1;
    }
  for (i = 0; i < NCOLORS; i++)
    ndither[i] = error[i][0][0];

  QUANT(6);
  for (; x != terminate; x += direction)
    {
      int extra_k;
      CHANNEL(d, ECOLOR_K).v = cmyk[3];
      CHANNEL(d, ECOLOR_C).v = cmyk[0];
      CHANNEL(d, ECOLOR_M).v = cmyk[1];
      CHANNEL(d, ECOLOR_Y).v = cmyk[2];
      extra_k = compute_black(d) + CHANNEL(d, ECOLOR_K).v;
      for (i = 0; i < NCOLORS; i++)
	{
	  CHANNEL(d, i).o = CHANNEL(d, i).v;
	  if (i != ECOLOR_K)
	    CHANNEL(d, i).o += extra_k;
	  CHANNEL(d, i).b = CHANNEL(d, i).v;
	  CHANNEL(d, i).v = UPDATE_COLOR(CHANNEL(d, i).v, ndither[i]);
	  CHANNEL(d, i).v = print_color(d, &(CHANNEL(d, i)), x, row, bit,
					length, 0, d->dither_type);
	  ndither[i] = update_dither(d, i, d->src_width,
				     direction, error[i][0], error[i][1]);
	}
      QUANT(12);
      ADVANCE_BIDIRECTIONAL(d, bit, cmyk, direction, 4, xerror, xmod, error,
			    NCOLORS, ERROR_ROWS);
      QUANT(13);
    }
  if (direction == -1)
    reverse_row_ends(d);
}

void
stp_dither(const unsigned short  *input,
	   int           row,
	   void 	  *vd,
	   unsigned char *cyan,
	   unsigned char *lcyan,
	   unsigned char *magenta,
	   unsigned char *lmagenta,
	   unsigned char *yellow,
	   unsigned char *lyellow,
	   unsigned char *black,
	   int		  duplicate_line,
	   int		  zero_mask)
{
  int i, j;
  dither_t *d = (dither_t *) vd;
  CHANNEL(d, ECOLOR_K).ptrs[0] = black;
  CHANNEL(d, ECOLOR_K).ptrs[1] = NULL;
  CHANNEL(d, ECOLOR_C).ptrs[0] = cyan;
  CHANNEL(d, ECOLOR_C).ptrs[1] = lcyan;
  CHANNEL(d, ECOLOR_M).ptrs[0] = magenta;
  CHANNEL(d, ECOLOR_M).ptrs[1] = lmagenta;
  CHANNEL(d, ECOLOR_Y).ptrs[0] = yellow;
  CHANNEL(d, ECOLOR_Y).ptrs[1] = lyellow;
  for (i = 0; i < NCOLORS; i++)
    {
      for (j = 0; j < 2; j++)
	if (CHANNEL(d, i).ptrs[j])
	  memset(CHANNEL(d, i).ptrs[j], 0,
		 (d->dst_width + 7) / 8 * CHANNEL(d, i).dither.signif_bits);
      CHANNEL(d, i).dither.row_ends[0][1] = -1;
      CHANNEL(d, i).dither.row_ends[0][0] = -1;
      CHANNEL(d, i).dither.row_ends[1][1] = -1;
      CHANNEL(d, i).dither.row_ends[1][0] = -1;
      if (CHANNEL(d, i).dither.nlevels == 1 &&
	  CHANNEL(d, i).dither.ranges[0].bits[1] == 1 &&
	  CHANNEL(d, i).dither.ranges[0].isdark[1])
	CHANNEL(d, i).very_fast = 1;
      else
	CHANNEL(d, i).very_fast = 0;
      matrix_set_row(d, &(CHANNEL(d, i).dithermat), row);
      matrix_set_row(d, &(CHANNEL(d, i).pick), row);
    }
  d->ptr_offset = 0;
  switch (d->dither_class)
    {
    case OUTPUT_MONOCHROME:
      stp_dither_monochrome(input, row, vd, duplicate_line, zero_mask);
      break;
    case OUTPUT_GRAY:
      if (d->dither_type & D_FAST_BASE)
	stp_dither_black_fast(input, row, vd, duplicate_line, zero_mask);
      else if (d->dither_type & D_ORDERED_BASE)
	stp_dither_black_ordered(input, row, vd, duplicate_line, zero_mask);
      else
	stp_dither_black_ed(input, row, vd, duplicate_line, zero_mask);
      break;
    case OUTPUT_COLOR:
      if (black)
	{
	  if (d->dither_type & D_FAST_BASE)
	    stp_dither_cmyk_fast(input, row, vd, duplicate_line, zero_mask);
	  else if (d->dither_type & D_ORDERED_BASE)
	    stp_dither_cmyk_ordered(input, row, vd, duplicate_line, zero_mask);
	  else
	    stp_dither_cmyk_ed(input, row, vd, duplicate_line, zero_mask);
	}
      else
	{
	  if (d->dither_type & D_FAST_BASE)
	    stp_dither_cmy_fast(input, row, vd, duplicate_line, zero_mask);
	  else if (d->dither_type & D_ORDERED_BASE)
	    stp_dither_cmy_ordered(input, row, vd, duplicate_line, zero_mask);
	  else
	    stp_dither_cmy_ed(input, row, vd, duplicate_line, zero_mask);
	}
      break;
    case OUTPUT_RAW_CMYK:
      if (d->dither_type & D_FAST_BASE)
	stp_dither_raw_cmyk_fast(input, row, vd, duplicate_line, zero_mask);
      else if (d->dither_type & D_ORDERED_BASE)
	stp_dither_raw_cmyk_ordered(input, row, vd, duplicate_line, zero_mask);
      else
	stp_dither_raw_cmyk_ed(input, row, vd, duplicate_line, zero_mask);
      break;
    }
}
