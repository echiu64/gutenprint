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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */


/* #define PRINT_DEBUG */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print.h>
#include <gimp-print-internal.h>
#include <gimp-print-intl-internal.h>
#include <limits.h>
#include <math.h>

/*
 * The GNU C library provides a good rand() function, but most others
 * use a separate function called random() to provide the same quality
 * (and size) numbers...
 */

#ifdef HAVE_RANDOM
#  define rand random
#endif /* HAVE_RANDOM */

/* If you don't want detailed performance numbers in this file,
 * uncomment this:
 */
/*#define QUANT(x) */

#define D_FLOYD_HYBRID 0
#define D_FLOYD 1
#define D_ADAPTIVE_BASE 4
#define D_ADAPTIVE_HYBRID (D_ADAPTIVE_BASE | D_FLOYD_HYBRID)
#define D_ADAPTIVE_RANDOM (D_ADAPTIVE_BASE | D_FLOYD)
#define D_ORDERED_BASE 8
#define D_ORDERED (D_ORDERED_BASE)
#define D_FAST_BASE 16
#define D_FAST (D_FAST_BASE)
#define D_VERY_FAST (D_FAST_BASE + 1)

#define DITHER_FAST_STEPS (6)

#define DITHER_MONOCHROME (0)
#define DITHER_BLACK (1)
#define DITHER_CMYK (2)

typedef struct
{
  const char *name;
  int id;
} dither_algo_t;

static const dither_algo_t dither_algos[] =
{
  { N_ ("Adaptive Hybrid"),        D_ADAPTIVE_HYBRID },
  { N_ ("Ordered"),                D_ORDERED },
  { N_ ("Fast"),                   D_FAST },
  { N_ ("Very Fast"),              D_VERY_FAST },
  { N_ ("Adaptive Random"),        D_ADAPTIVE_RANDOM },
  { N_ ("Hybrid Floyd-Steinberg"), D_FLOYD_HYBRID },
  { N_ ("Random Floyd-Steinberg"), D_FLOYD }
};

static const int num_dither_algos = sizeof(dither_algos) / sizeof(dither_algo_t);

#define ERROR_ROWS 2

#define MAX_SPREAD 32

/*
 * A segment of the entire 0-65535 intensity range.
 */
typedef struct dither_segment
{
  unsigned range_l;		/* Bottom of range */
  unsigned range_h;		/* Top of range */
  unsigned value_l;		/* Value of lighter ink */
  unsigned value_h;		/* Value of upper ink */
  unsigned bits_l;		/* Bit pattern of lower */
  unsigned bits_h;		/* Bit pattern of upper */
  unsigned range_span;		/* Span (to avoid calculation on the fly) */
  unsigned value_span;		/* Span of values */
  unsigned dot_size_l;		/* Size of lower dot */
  unsigned dot_size_h;		/* Size of upper dot */
  char isdark_l;		/* Is lower value dark ink? */
  char isdark_h;		/* Is upper value dark ink? */
} dither_segment_t;

typedef struct dither_color
{
  int nlevels;
  unsigned bit_max;
  unsigned signif_bits;
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
				/* error is distributed.  This should be */
				/* between 12 (very broad distribution) and */
				/* 19 (very narrow) */

  unsigned randomizer[NCOLORS]; /* With Floyd-Steinberg dithering, control */
				/* how much randomness is applied to the */
				/* threshold values (0-65535).  With ordered */
				/* dithering, how much randomness is added */
				/* to the matrix value. */

  int k_clevel;			/* Amount of each ink (in 64ths) required */
  int k_mlevel;			/* to create equivalent black */
  int k_ylevel;

  int c_darkness;		/* Perceived "darkness" of each ink, */
  int m_darkness;		/* in 64ths, to calculate CMY-K transitions */
  int y_darkness;

  int dither_type;

  int d_cutoff;			/* When ordered dither is used, threshold */
				/* above which no randomness is used. */
  int adaptive_divisor;
  int adaptive_limit;
  int adaptive_lower_limit;

  int x_aspect;			/* Aspect ratio numerator */
  int y_aspect;			/* Aspect ratio denominator */

  double transition;		/* Exponential scaling for transition region */

  dither_color_t dither[NCOLORS];

  int *errs[ERROR_ROWS][NCOLORS];
  unsigned short *vals[NCOLORS];
  int *offset0_table;
  int *offset1_table;

  int ink_limit;		/* Maximum amount of ink that may be */
				/* deposited */
  int oversampling;
  int last_line_was_empty;

  int dither_class;		/* mono, black, or CMYK */

  /* Hardwiring these matrices in here is an abomination.  This */
  /* eventually needs to be cleaned up. */
  dither_matrix_t mat6;
  dither_matrix_t mat7;

  dither_matrix_t pick[NCOLORS];
  dither_matrix_t dithermat[NCOLORS];
  unsigned short virtual_dot_scale[65536];
} dither_t;

/*
 * Bayer's dither matrix using Judice, Jarvis, and Ninke recurrence relation
 * http://www.cs.rit.edu/~sxc7922/Project/CRT.htm
 */

static const unsigned sq2[] =
{
  0, 2,
  3, 1
};

#if 0
conat static unsigned sq3[] =
{
  3, 2, 7,
  8, 4, 0,
  1, 6, 5
};

/*
 * This magic square taken from
 * http://www.pse.che.tohoku.ac.jp/~msuzuki/MagicSquare.5x5.selfsim.html
 *
 * It is magic in the following ways:
 * Rows and columns
 * Major and minor diagonals
 * Self-complementary
 * Four neighbors at distance of 1 or 2 (diagonal or lateral)
 */

static const unsigned msq0[] =
{
  00, 14, 21, 17,  8,
  22, 18,  5,  4, 11,
  9,   1, 12, 23, 15,
  13, 20, 19,  6,  2,
  16,  7,  3, 10, 24
};

static const unsigned msq1[] =
{
  03, 11, 20, 17,  9,
  22, 19,  8,  1, 10,
  06,  0, 12, 24, 18,
  14, 23, 16,  5,  2,
  15,  7,  4, 13, 21
};

static const unsigned short quic0[] = {
#include "quickmatrix199.h"
};

static const unsigned short quic1[] = {
#include "quickmatrix199-2.h"
};
#endif

static const unsigned short quic2[] = {
#include "quickmatrix257.h"
};

static const unsigned short rect2x1[] = {
#include "ran.367.179.h"
};

static const unsigned short rect4x1[] = {
#include "ran.509.131.h"
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
  return _(dither_algos[id].name);
}

static inline int
calc_ordered_point(unsigned x, unsigned y, int steps, int multiplier,
		   int size, const int *map)
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
	  (long long) mat->matrix[x + y * mat->x_size] * 65536ll /
	  (long long) (mat->x_size * mat->y_size);
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
  free(tmp);
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
	    (long long) mat->matrix[x + y * mat->x_size] * 65536ll /
	    (long long) (mat->x_size * mat->y_size);
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
	    (long long) mat->matrix[x + y * mat->x_size] * 65536ll /
	    (long long) (mat->x_size * mat->y_size);
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
    free(mat->matrix);
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

void *
stp_init_dither(int in_width, int out_width, int horizontal_aspect,
		int vertical_aspect, stp_vars_t v)
{
  int i;
  dither_t *d = stp_malloc(sizeof(dither_t));
  stp_simple_dither_range_t r;
  memset(d, 0, sizeof(dither_t));
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

  if (d->dither_type == D_VERY_FAST)
    stp_dither_set_iterated_matrix(d, 2, DITHER_FAST_STEPS, sq2, 0, 2, 4);
  else if (d->y_aspect == d->x_aspect)
    stp_dither_set_matrix_short(d, 257, 257, quic2, 0, 1, 0, 0);
  else if (d->y_aspect / d->x_aspect == 2)
    stp_dither_set_matrix_short(d, 367, 179, rect2x1, 0, 1, 0, 0);
  else if (d->y_aspect / d->x_aspect == 3)
    stp_dither_set_matrix_short(d, 509, 131, rect4x1, 0, 1, 0, 0);
  else if (d->y_aspect / d->x_aspect == 4)
    stp_dither_set_matrix_short(d, 509, 131, rect4x1, 0, 1, 0, 0);
  else if (d->y_aspect > d->x_aspect)
    stp_dither_set_matrix_short(d, 367, 179, rect2x1, 0, 1, 0, 0);
  else if (d->x_aspect / d->y_aspect == 2)
    stp_dither_set_matrix_short(d, 179, 367, rect2x1, 1, 1, 0, 0);
  else if (d->x_aspect / d->y_aspect == 3)
    stp_dither_set_matrix_short(d, 131, 509, rect4x1, 1, 1, 0, 0);
  else if (d->x_aspect / d->y_aspect == 4)
    stp_dither_set_matrix_short(d, 131, 509, rect4x1, 1, 1, 0, 0);
  else if (d->x_aspect > d->y_aspect)
    stp_dither_set_matrix_short(d, 179, 367, rect2x1, 1, 1, 0, 0);

  d->src_width = in_width;
  d->dst_width = out_width;
  d->adaptive_divisor = 2;

  stp_dither_set_max_ink(d, INT_MAX, 1.0);
  stp_dither_set_ink_spread(d, 13);
  stp_dither_set_black_lower(d, .4);
  stp_dither_set_black_upper(d, .7);
  stp_dither_set_black_levels(d, 1.0, 1.0, 1.0);
  stp_dither_set_randomizers(d, 1.0, 1.0, 1.0, 1.0);
  stp_dither_set_ink_darkness(d, .4, .3, .2);
  stp_dither_set_density(d, 1.0);
  if (stp_get_image_type(v) == IMAGE_MONOCHROME)
    d->dither_class = DITHER_MONOCHROME;
  else if (stp_get_output_type(v) == OUTPUT_GRAY)
    d->dither_class = DITHER_BLACK;
  else
    d->dither_class = DITHER_CMYK;
  return d;
}

static void
preinit_matrix(dither_t *d)
{
  int i;
  for (i = 0; i < NCOLORS; i++)
    destroy_matrix(&(d->dithermat[i]));
  destroy_matrix(&(d->mat6));
}

static void
postinit_matrix(dither_t *d, int x_shear, int y_shear)
{
  unsigned x_3, y_3;
  if (x_shear || y_shear)
    shear_matrix(&(d->mat6), x_shear, y_shear);
  x_3 = d->mat6.x_size / 3;
  y_3 = d->mat6.y_size / 3;
  clone_matrix(&(d->mat6), &(d->dithermat[ECOLOR_C]), 2 * x_3, y_3);
  clone_matrix(&(d->mat6), &(d->dithermat[ECOLOR_M]), x_3, 2 * y_3);
  clone_matrix(&(d->mat6), &(d->dithermat[ECOLOR_Y]), 0, y_3);
  clone_matrix(&(d->mat6), &(d->dithermat[ECOLOR_K]), 0, 0);
  stp_dither_set_transition(d, d->transition);
}

void
stp_dither_set_matrix(void *vd, size_t x, size_t y, const unsigned *data,
		      int transpose, int prescaled, int x_shear, int y_shear)
{
  dither_t *d = (dither_t *) vd;
  preinit_matrix(d);
  init_matrix(&(d->mat6), x, y, data, transpose, prescaled);
  postinit_matrix(d, x_shear, y_shear);
}

void
stp_dither_set_iterated_matrix(void *vd, size_t edge, size_t iterations,
			       const unsigned *data, int prescaled,
			       int x_shear, int y_shear)
{
  dither_t *d = (dither_t *) vd;
  preinit_matrix(d);
  init_iterated_matrix(&(d->mat6), edge, iterations, data);
  postinit_matrix(d, x_shear, y_shear);
}

void
stp_dither_set_matrix_short(void *vd, size_t x, size_t y,
			    const unsigned short *data, int transpose,
			    int prescaled, int x_shear,
			    int y_shear)
{
  dither_t *d = (dither_t *) vd;
  preinit_matrix(d);
  init_matrix_short(&(d->mat6), x, y, data, transpose, prescaled);
  postinit_matrix(d, x_shear, y_shear);
}

void
stp_dither_set_transition(void *vd, double exponent)
{
  int i;
  dither_t *d = (dither_t *) vd;
  int x_3 = d->mat6.x_size / 3;
  int y_3 = d->mat6.y_size / 3;
  for (i = 0; i < NCOLORS; i++)
    destroy_matrix(&(d->pick[i]));
  destroy_matrix(&(d->mat7));
  copy_matrix(&(d->mat6), &(d->mat7));
  d->transition = exponent;
  if (exponent < .999 || exponent > 1.001)
    exponential_scale_matrix(&(d->mat7), exponent);
  if (d->dither_type & D_ORDERED_BASE)
    {
      clone_matrix(&(d->mat7), &(d->pick[ECOLOR_C]), 2 * x_3, y_3);
      clone_matrix(&(d->mat7), &(d->pick[ECOLOR_M]), x_3, 2 * y_3);
      clone_matrix(&(d->mat7), &(d->pick[ECOLOR_Y]), 0, y_3);
      clone_matrix(&(d->mat7), &(d->pick[ECOLOR_K]), 0, 0);
    }
  else
    {
      clone_matrix(&(d->mat7), &(d->pick[ECOLOR_C]), x_3, 0);
      clone_matrix(&(d->mat7), &(d->pick[ECOLOR_M]), 0, 2 * y_3);
      clone_matrix(&(d->mat7), &(d->pick[ECOLOR_Y]), 2 * x_3, 0);
      clone_matrix(&(d->mat7), &(d->pick[ECOLOR_K]), x_3, 2 * y_3);
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
  d->adaptive_limit = d->density / d->adaptive_divisor;
  d->adaptive_lower_limit = d->adaptive_limit / 4;
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

static double
imax(double a, double b)
{
  return ((a > b) ? a : b);
}

void
stp_dither_set_max_ink(void *vd, int levels, double max_ink)
{
  dither_t *d = (dither_t *) vd;
  d->ink_limit = imax(max_ink, 1)*levels;
  d->ink_limit = max_ink*levels+0.5;
#ifdef VERBOSE
  fprintf(stderr, "Maxink: %f %d\n", max_ink, d->ink_limit);
#endif
}

void
stp_dither_set_adaptive_divisor(void *vd, unsigned divisor)
{
  dither_t *d = (dither_t *) vd;
  d->adaptive_divisor = divisor;
  d->adaptive_limit = d->density / d->adaptive_divisor;
  d->adaptive_lower_limit = d->adaptive_limit / 4;
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
      free(d->offset0_table);
      d->offset0_table = NULL;
    }
  if (d->offset1_table)
    {
      free(d->offset1_table);
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

  d->adaptive_limit = d->density / d->adaptive_divisor;
  d->adaptive_lower_limit = d->adaptive_limit / 4;
}

void
stp_dither_set_black_levels(void *vd, double c, double m, double y)
{
  dither_t *d = (dither_t *) vd;
  d->k_clevel = (int) (c * 64);
  d->k_mlevel = (int) (m * 64);
  d->k_ylevel = (int) (y * 64);
}

void
stp_dither_set_randomizers(void *vd, double c, double m, double y, double k)
{
  dither_t *d = (dither_t *) vd;
  d->randomizer[ECOLOR_C] = c * 65535;
  d->randomizer[ECOLOR_M] = m * 65535;
  d->randomizer[ECOLOR_Y] = y * 65535;
  d->randomizer[ECOLOR_K] = k * 65535;
}

void
stp_dither_set_ink_darkness(void *vd, double c, double m, double y)
{
  dither_t *d = (dither_t *) vd;
  d->c_darkness = (int) (c * 64);
  d->m_darkness = (int) (m * 64);
  d->y_darkness = (int) (y * 64);
}

void
stp_dither_set_light_inks(void *vd, double c, double m, double y, double density)
{
  stp_simple_dither_range_t range[2];
  range[0].bit_pattern = 1;
  range[0].is_dark = 0;
  range[1].value = 1;
  range[1].bit_pattern = 1;
  range[1].is_dark = 1;
  range[1].dot_size = 1;
  if (c > 0)
    {
      range[0].value = c;
      range[0].dot_size = 1;
      stp_dither_set_ranges(vd, ECOLOR_C, 2, range, density);
    }
  if (m > 0)
    {
      range[0].value = m;
      range[0].dot_size = 1;
      stp_dither_set_ranges(vd, ECOLOR_M, 2, range, density);
    }
  if (y > 0)
    {
      range[0].value = y;
      range[0].dot_size = 1;
      stp_dither_set_ranges(vd, ECOLOR_Y, 2, range, density);
    }
}

static void
stp_dither_set_generic_ranges(dither_color_t *s, int nlevels,
			  const stp_simple_dither_range_t *ranges, double density)
{
  int i;
  unsigned lbit;
  if (s->ranges)
    free(s->ranges);
  s->nlevels = nlevels > 1 ? nlevels + 1 : nlevels;
  s->ranges = (dither_segment_t *)
    stp_malloc(s->nlevels * sizeof(dither_segment_t));
  s->bit_max = 0;
#ifdef VERBOSE
  fprintf(stderr, "stp_dither_set_generic_ranges nlevels %d density %f\n", nlevels, density);
  for (i = 0; i < nlevels; i++)
    fprintf(stderr, "  level %d value %f pattern %x is_dark %d\n", i,
	    ranges[i].value, ranges[i].bit_pattern, ranges[i].is_dark);
#endif
  s->ranges[0].range_l = 0;
  s->ranges[0].value_l = ranges[0].value * 65536.0;
  s->ranges[0].bits_l = ranges[0].bit_pattern;
  s->ranges[0].isdark_l = ranges[0].is_dark;
  s->ranges[0].dot_size_l = ranges[0].dot_size;
  if (nlevels == 1)
    s->ranges[0].range_h = 65536;
  else
    s->ranges[0].range_h = ranges[0].value * 65536.0 * density;
  if (s->ranges[0].range_h > 65536)
    s->ranges[0].range_h = 65536;
  s->ranges[0].value_h = ranges[0].value * 65536.0;
  if (s->ranges[0].value_h > 65536)
    s->ranges[0].value_h = 65536;
  s->ranges[0].bits_h = ranges[0].bit_pattern;
  if (ranges[0].bit_pattern > s->bit_max)
    s->bit_max = ranges[0].bit_pattern;
  s->ranges[0].isdark_h = ranges[0].is_dark;
  s->ranges[0].dot_size_h = ranges[0].dot_size;
  s->ranges[0].range_span = s->ranges[0].range_h;
  s->ranges[0].value_span = 0;
  if (s->nlevels > 1)
    {
      for (i = 0; i < nlevels - 1; i++)
	{
	  int l = i + 1;
	  s->ranges[l].range_l = s->ranges[i].range_h;
	  s->ranges[l].value_l = s->ranges[i].value_h;
	  s->ranges[l].bits_l = s->ranges[i].bits_h;
	  s->ranges[l].isdark_l = s->ranges[i].isdark_h;
	  s->ranges[l].dot_size_l = s->ranges[i].dot_size_h;
	  if (i == nlevels - 1)
	    s->ranges[l].range_h = 65536;
	  else
	    s->ranges[l].range_h =
	      (ranges[l].value + ranges[l].value) * 65536.0 * density / 2;
	  if (s->ranges[l].range_h > 65536)
	    s->ranges[l].range_h = 65536;
	  s->ranges[l].value_h = ranges[l].value * 65536.0;
	  if (s->ranges[l].value_h > 65536)
	    s->ranges[l].value_h = 65536;
	  s->ranges[l].bits_h = ranges[l].bit_pattern;
	  if (ranges[l].bit_pattern > s->bit_max)
	    s->bit_max = ranges[l].bit_pattern;
	  s->ranges[l].isdark_h = ranges[l].is_dark;
	  s->ranges[l].dot_size_h = ranges[l].dot_size;
	  s->ranges[l].range_span =
	    s->ranges[l].range_h - s->ranges[l].range_l;
	  s->ranges[l].value_span =
	    s->ranges[l].value_h - s->ranges[l].value_l;
	}
      i++;
      s->ranges[i].range_l = s->ranges[i - 1].range_h;
      s->ranges[i].value_l = s->ranges[i - 1].value_h;
      s->ranges[i].bits_l = s->ranges[i - 1].bits_h;
      s->ranges[i].isdark_l = s->ranges[i - 1].isdark_h;
      s->ranges[i].dot_size_l = s->ranges[i - 1].dot_size_h;
      s->ranges[i].range_h = 65536;
      s->ranges[i].value_h = s->ranges[i].value_l;
      s->ranges[i].bits_h = s->ranges[i].bits_l;
      s->ranges[i].isdark_h = s->ranges[i].isdark_l;
      s->ranges[i].dot_size_h = s->ranges[i].dot_size_l;
      s->ranges[i].range_span = s->ranges[i].range_h - s->ranges[i].range_l;
      s->ranges[i].value_span = s->ranges[i].value_h - s->ranges[i].value_l;
    }
  lbit = s->bit_max;
  s->signif_bits = 0;
  while (lbit > 0)
    {
      s->signif_bits++;
      lbit >>= 1;
    }
#ifdef VERBOSE
  for (i = 0; i < s->nlevels; i++)
    {
      fprintf(stderr, "    level %d value_l %d value_h %d range_l %d range_h %d\n",
	      i, s->ranges[i].value_l, s->ranges[i].value_h,
	      s->ranges[i].range_l, s->ranges[i].range_h);
      fprintf(stderr, "       bits_l %d bits_h %d isdark_l %d isdark_h %d\n",
	      s->ranges[i].bits_l, s->ranges[i].bits_h,
	      s->ranges[i].isdark_l, s->ranges[i].isdark_h);
      fprintf(stderr, "       rangespan %d valuespan %d\n",
	      s->ranges[i].range_span, s->ranges[i].value_span);
    }
  fprintf(stderr, "  bit_max %d signif_bits %d\n", s->bit_max, s->signif_bits);
#endif
}

static void
stp_dither_set_generic_ranges_full(dither_color_t *s, int nlevels,
			       const stp_full_dither_range_t *ranges,
			       double density, int max_ink)
{
  int i, j;
  unsigned lbit;
  if (s->ranges)
    free(s->ranges);
  s->nlevels = nlevels > 1 ? nlevels + 1 : nlevels;
  s->nlevels = nlevels+1;
  s->ranges = (dither_segment_t *)
    stp_malloc(s->nlevels * sizeof(dither_segment_t));
  s->bit_max = 0;
#ifdef VERBOSE
  fprintf(stderr, "stp_dither_set_ranges nlevels %d density %f\n", nlevels, density);
  for (i = 0; i < nlevels; i++)
    fprintf(stderr, "  level %d value: low %f high %f pattern low %x high %x is_dark low %d high %d\n", i,
	    ranges[i].value_l, ranges[i].value_h, ranges[i].bits_l, ranges[i].bits_h,ranges[i].isdark_l, ranges[i].isdark_h);
#endif
  for(i=j=0; i < nlevels; i++) {
    if (ranges[i].bits_h > s->bit_max)
      s->bit_max = ranges[i].bits_h;
    if (ranges[i].bits_l > s->bit_max)
      s->bit_max = ranges[i].bits_l;
    s->ranges[j].dot_size_l = ranges[i].bits_l; /* FIXME */
    s->ranges[j].dot_size_h = ranges[i].bits_h;
	/*if(s->ranges[j].dot_size_l > max_ink || s->ranges[j].dot_size_h > max_ink)
		   continue;*/
    s->ranges[j].value_l = ranges[i].value_l * 65535;
    s->ranges[j].value_h = ranges[i].value_h * 65535;
    s->ranges[j].range_l = s->ranges[j].value_l*density;
    s->ranges[j].range_h = s->ranges[j].value_h*density;
    s->ranges[j].bits_l = ranges[i].bits_l;
    s->ranges[j].bits_h = ranges[i].bits_h;
    s->ranges[j].isdark_l = ranges[i].isdark_l;
    s->ranges[j].isdark_h = ranges[i].isdark_h;
    s->ranges[j].range_span = s->ranges[j].range_h-s->ranges[j].range_l;
    s->ranges[j].value_span = s->ranges[j].value_h-s->ranges[j].value_l;
	j++;
  }
  s->ranges[j].range_l = s->ranges[j - 1].range_h;
  s->ranges[j].value_l = s->ranges[j - 1].value_h;
  s->ranges[j].bits_l = s->ranges[j - 1].bits_h;
  s->ranges[j].isdark_l = s->ranges[j - 1].isdark_h;
  s->ranges[j].dot_size_l = s->ranges[j - 1].dot_size_h;
  s->ranges[j].range_h = 65535;
  s->ranges[j].value_h = 65535;
  s->ranges[j].bits_h = s->ranges[j].bits_l;
  s->ranges[j].isdark_h = s->ranges[j].isdark_l;
  s->ranges[j].dot_size_h = s->ranges[j].dot_size_l;
  s->ranges[j].range_span = s->ranges[j].range_h - s->ranges[j].range_l;
  s->ranges[j].value_span = 0;
  s->nlevels = j+1;
  lbit = s->bit_max;
  s->signif_bits = 0;
  while (lbit > 0)
    {
      s->signif_bits++;
      lbit >>= 1;
    }
#ifdef VERBOSE
  for (i = 0; i < s->nlevels; i++)
    {
      fprintf(stderr, "    level %d value_l %d value_h %d range_l %d range_h %d\n",
	      i, s->ranges[i].value_l, s->ranges[i].value_h,
	      s->ranges[i].range_l, s->ranges[i].range_h);
      fprintf(stderr, "       bits_l %d bits_h %d isdark_l %d isdark_h %d\n",
	      s->ranges[i].bits_l, s->ranges[i].bits_h,
	      s->ranges[i].isdark_l, s->ranges[i].isdark_h);
      fprintf(stderr, "       rangespan %d valuespan %d\n",
	      s->ranges[i].range_span, s->ranges[i].value_span);
    }
  fprintf(stderr, "  bit_max %d signif_bits %d\n", s->bit_max, s->signif_bits);
#endif
}

void
stp_dither_set_ranges(void *vd, int color, int nlevels,
		  const stp_simple_dither_range_t *ranges, double density)
{
  dither_t *d = (dither_t *) vd;
  if (color < 0 || color >= NCOLORS)
    return;
  stp_dither_set_generic_ranges(&(d->dither[color]), nlevels, ranges, density);
}

void
stp_dither_set_ranges_simple(void *vd, int color, int nlevels,
			 const double *levels, double density)
{
  stp_simple_dither_range_t *r = stp_malloc(nlevels * sizeof(stp_simple_dither_range_t));
  int i;
  for (i = 0; i < nlevels; i++)
    {
      r[i].bit_pattern = i + 1;
      r[i].dot_size = i + 1;
      r[i].value = levels[i];
      r[i].is_dark = 1;
    }
  stp_dither_set_ranges(vd, color, nlevels, r, density);
  free(r);
}

void
stp_dither_set_ranges_full(void *vd, int color, int nlevels,
		       const stp_full_dither_range_t *ranges, double density)
{
  dither_t *d = (dither_t *) vd;
  stp_dither_set_generic_ranges_full(&(d->dither[color]), nlevels, ranges, density,
				 d->ink_limit);
}

void
stp_free_dither(void *vd)
{
  dither_t *d = (dither_t *) vd;
  int i;
  int j;
  for (j = 0; j < NCOLORS; j++)
    {
      if (d->vals[j])
	{
	  free(d->vals[j]);
	  d->vals[j] = NULL;
	}
      for (i = 0; i < ERROR_ROWS; i++)
	{
	  if (d->errs[i][j])
	    {
	      free(d->errs[i][j]);
	      d->errs[i][j] = NULL;
	    }
	}
      free(d->dither[i].ranges);
      d->dither[i].ranges = NULL;
      destroy_matrix(&(d->pick[i]));
      destroy_matrix(&(d->dithermat[i]));
    }
  if (d->offset0_table)
    {
      free(d->offset0_table);
      d->offset0_table = NULL;
    }
  if (d->offset1_table)
    {
      free(d->offset1_table);
      d->offset1_table = NULL;
    }
  destroy_matrix(&(d->mat6));
  destroy_matrix(&(d->mat7));
  free(d);
}

static int *
get_errline(dither_t *d, int row, int color)
{
  if (row < 0 || color < 0 || color >= NCOLORS)
    return NULL;
  if (d->errs[row & 1][color])
    return d->errs[row & 1][color] + MAX_SPREAD;
  else
    {
      int size = 2 * MAX_SPREAD + (16 * ((d->dst_width + 7) / 8));
      d->errs[row & 1][color] = stp_malloc(size * sizeof(int));
      memset(d->errs[row & 1][color], 0, size * sizeof(int));
      return d->errs[row & 1][color] + MAX_SPREAD;
    }
}

static unsigned short *
get_valueline(dither_t *d, int color)
{
  if (color < 0 || color >= NCOLORS)
    return NULL;
  if (d->vals[color])
    return d->vals[color];
  else
    {
      int size = (8 * ((d->dst_width + 7) / 8));
      d->vals[color] = stp_malloc(size * sizeof(unsigned short));
      return d->vals[color];
    }
}

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
update_dither(int r, int o, int width, int odb, int odb_mask,
	      int direction, int *error0, int *error1, dither_t *d)
{
  int tmp = r;
  int i, dist, dist1;
  int delta, delta1;
  int offset;
  if (tmp == 0)
    return error0[direction];
  if (tmp > 65535)
    tmp = 65535;
  if (odb >= 16 || o >= 2048)
    {
      tmp += tmp;
      tmp += tmp;
      error1[0] += tmp;
      return error0[direction] + tmp;
    }
  else
    {
      int tmpo = o << 5;
      offset = ((65535 - tmpo) >> odb) +
	((tmp & odb_mask) > (tmpo & odb_mask));
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
    }
  delta = dist;
  delta1 = dist1;
  for (i = -offset; i < 0; i++)
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

/*
 * Print a single dot.  This routine has become awfully complicated
 * awfully fast!
 *
 * Note that the ink budget is both an input and an output parameter
 */

static inline int
print_color(dither_t *d, dither_color_t *rv, int base, int density,
	    int adjusted, int x, int y, unsigned char *c, unsigned char *lc,
	    unsigned char bit, int height, unsigned randomizer, int dontprint,
	    int *ink_budget, dither_matrix_t *pick_matrix,
	    dither_matrix_t *dither_matrix, int dither_type)
{
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

  if ((adjusted <= 0 && !(dither_type & D_ADAPTIVE_BASE)) ||
      base <= 0 || density <= 0)
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

      if (density <= dd->range_l)
	continue;

      /*
       * If we're using an adaptive dithering method, decide whether
       * to use the Floyd-Steinberg or the ordered method based on the
       * input value.  The choice of 1/128 is somewhat arbitrary and
       * could stand to be parameterized.  Another possibility would be
       * to scale to something less than pure ordered at 0 input value.
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
      if (dd->range_span == 0 ||
	  (dd->value_span == 0 && dd->isdark_l == dd->isdark_h))
	rangepoint = 32768;
      else
	rangepoint =
	  ((unsigned) (density - dd->range_l)) * 65536 / dd->range_span;

      /*
       * Compute the virtual dot size that we're going to print.
       * This is somewhere between the two candidate dot sizes.
       * This is scaled between the high and low value.
       */

      if (dd->value_span == 0)
	virtual_value = dd->value_h;
      else if (dd->range_span == 0)
	virtual_value = (dd->value_h + dd->value_l) / 2;
      else if (dd->value_h == 65536 && rangepoint == 65536)
	virtual_value = 65536;
      else
	virtual_value = dd->value_l +
	  (dd->value_span * d->virtual_dot_scale[rangepoint] / 65536);

      /*
       * Reduce the randomness as the base value increases, to get
       * smoother output in the midtones.  Idea suggested by
       * Thomas Tonino.
       */
      if (dither_type & D_ORDERED_BASE)
	randomizer = 65536;	/* With ordered dither, we need this */
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
	   * First, compute a value between 0 and 65536 that will be
	   * scaled to produce an offset from the desired threshold.
	   */
	  switch (dither_type)
	    {
	    case D_FLOYD:
	      /*
	       * Floyd-Steinberg: use a mildly Gaussian random number.
	       * This might be a bit too Gaussian.
	       */
	      vmatrix = ((rand() & 0xffff000) +
			 (rand() & 0xffff000)) >> 13;
	      break;
	    case D_FLOYD_HYBRID:
	      /*
	       * Hybrid Floyd-Steinberg: use a matrix to generate the offset.
	       */
	    case D_ORDERED:
	    default:
	      vmatrix = ditherpoint(d, dither_matrix, x);
	    }
	  /*
	   * Note that vmatrix cannot be 65536 here, and virtual_value
	   * cannot exceed 65536, so we cannot overflow.
	   */

	  /*
	   * Now, scale the virtual dot size appropriately.  Note that
	   * we'll get something evenly distributed between 0 and
	   * the virtual dot size, centered on the dot size / 2,
	   * which is the normal threshold value.
	   */
	  vmatrix = vmatrix * virtual_value / 65536;
	  if (randomizer != 65536)
	    {
	      /*
	       * We want vmatrix to be scaled between 0 and
	       * virtual_value when randomizer is 65536 (fully random).
	       * When it's less, we want it to scale through part of
	       * that range. In all cases, it should center around
	       * virtual_value / 2.
	       *
	       * vbase is the bottom of the scaling range.
	       */
	      unsigned vbase = virtual_value * (65536u - randomizer) /
		131072u;
	      vmatrix = vmatrix * randomizer / 65536;
	      vmatrix += vbase;
	    }
	} /* randomizer != 0 */

      /*
       * After all that, printing is almost an afterthought.
       * Pick the actual dot size (using a matrix here) and print it.
       */
      if (dither_value >= vmatrix)
	{
	  if (dd->isdark_h == dd->isdark_l && dd->bits_h == dd->bits_l)
	    {
	      isdark = dd->isdark_h;
	      bits = dd->bits_h;
	      v = dd->value_h;
	      dot_size = dd->dot_size_h;
	    }
	  else if (rangepoint >= ditherpoint(d, pick_matrix, x))
	    {
	      isdark = dd->isdark_h;
	      bits = dd->bits_h;
	      v = dd->value_h;
	      dot_size = dd->dot_size_h;
	    }
	  else
	    {
	      isdark = dd->isdark_l;
	      bits = dd->bits_l;
	      v = dd->value_l;
	      dot_size = dd->dot_size_l;
	    }
	  tptr = isdark ? c : lc;

	  /*
	   * Lay down all of the bits in the pixel.
	   */
	  if (dontprint < v && (!ink_budget || *ink_budget >= dot_size))
	    {
	      for (j = 1; j <= bits; j += j, tptr += height)
		{
		  if (j & bits)
		    tptr[0] |= bit;
		}
	      if (ink_budget)
		*ink_budget -= dot_size;
	    }
	  if (dither_type & D_ORDERED_BASE)
	    adjusted = -(int) (2 * v / 4);
	  else
	    adjusted -= v;
	}
      return adjusted;
    }
  return adjusted;
}

static inline void
print_color_fast(dither_t *d, dither_color_t *rv, int base,
		 int adjusted, int x, int y, unsigned char *c,
		 unsigned char *lc, unsigned char bit, int height,
		 dither_matrix_t *dither_matrix, int very_fast)
{
  int i;
  int levels = rv->nlevels - 1;
  int j;
  unsigned char *tptr;
  unsigned bits;

  if (adjusted <= 0 || base <= 0)
    return;
  if (very_fast)
    {
      if (adjusted >= ditherpoint(d, dither_matrix, x))
	c[0] |= bit;
      return;
    }
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
      dither_segment_t *dd = &(rv->ranges[i]);
      unsigned vmatrix;
      if (base <= dd->range_l)
	continue;

      vmatrix = (dd->value_h * ditherpoint(d, dither_matrix, x)) >> 16;

      /*
       * After all that, printing is almost an afterthought.
       * Pick the actual dot size (using a matrix here) and print it.
       */
      if (adjusted >= vmatrix)
	{
	  bits = dd->bits_h;
	  tptr = dd->isdark_h ? c : lc;

	  /*
	   * Lay down all of the bits in the pixel.
	   */
	  if (bits == 1)
	    {
	      tptr[0] |= bit;
	    }
	  else
	    {
	      for (j = 1; j <= bits; j += j, tptr += height)
		{
		  if (j & bits)
		    tptr[0] |= bit;
		}
	    }
	}
      return;
    }
  return;
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
		      unsigned char 	    *black,
		      int		    duplicate_line,
		      int		  zero_mask)
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		height;		/* Height of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  dither_t *d = (dither_t *) vd;
  dither_matrix_t *kdither = &(d->dithermat[ECOLOR_K]);
  unsigned bits = d->dither[ECOLOR_K].signif_bits;
  int j;
  unsigned char *tptr;
  int dst_width = d->dst_width;
  height = (d->dst_width + 7) / 8;

  memset(black, 0, height * bits);
  if (zero_mask)
    return;
  kptr = black;

  bit = 128;
  x = 0;

  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  matrix_set_row(d, kdither, row);
  for (x = 0; x < dst_width; x++)
    {
      if (gray[0] && (d->density >= ditherpoint(d, kdither, x)))
	{
	  tptr = kptr;
	  for (j = 0; j < bits; j++, tptr += height)
	    tptr[0] |= bit;
	}
      bit >>= 1;
      if (bit == 0)
	{
	  kptr ++;
	  bit = 128;
	}
      if (d->src_width == d->dst_width)
	gray++;
      else
	{
	  gray += xstep;
	  xerror += xmod;
	  if (xerror >= d->dst_width)
	    {
	      xerror -= d->dst_width;
	      gray++;
	    }
	}	  
    }
}

/*
 * 'stp_dither_black()' - Dither grayscale pixels to black.
 * This is for grayscale output.
 */

static void
stp_dither_black_fast(const unsigned short   *gray,
		      int           row,
		      void 		*vd,
		      unsigned char *black,
		      int		duplicate_line,
		      int		  zero_mask)
{
  int		x,		/* Current X coordinate */
		height;		/* Height of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k;
  dither_t *d = (dither_t *) vd;
  dither_color_t *kd = &(d->dither[ECOLOR_K]);
  dither_matrix_t *kdither = &(d->dithermat[ECOLOR_K]);
  int dst_width = d->dst_width;
  int dither_very_fast = 0;
  int xerror, xstep, xmod;

  height = (d->dst_width + 7) / 8;

  memset(black, 0, height * d->dither[ECOLOR_K].signif_bits);
  if (zero_mask)
    return;

  if (kd->nlevels == 1 && kd->ranges[0].bits_h == 1 && kd->ranges[0].isdark_h)
    dither_very_fast = 1;

  bit = 128;
  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  kptr = black;
  matrix_set_row(d, &(d->dithermat[ECOLOR_K]), row);

  for (x = 0; x < dst_width; x++)
    {
      k = gray[0];
      print_color_fast(d, kd, k, k, x, row, kptr, NULL, bit, height, kdither,
		       dither_very_fast);

      bit >>= 1;
      if (bit == 0)
	{
	  kptr ++;
	  bit = 128;
	}
      if (d->src_width == d->dst_width)
	gray++;
      else
	{
	  gray += xstep;
	  xerror += xmod;
	  if (xerror >= d->dst_width)
	    {
	      xerror -= d->dst_width;
	      gray++;
	    }
	}	  
    }
}

static void
stp_dither_black_ordered(const unsigned short   *gray,
			 int           	row,
			 void 		*vd,
			 unsigned char 	*black,
			 int		duplicate_line,
			 int		  zero_mask)
{

  int		x,		/* Current X coordinate */
		height;		/* Height of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k;		/* Current black error */
  dither_t *d = (dither_t *) vd;
  int terminate;
  int ink_budget;
  int xerror, xstep, xmod;

  height = (d->dst_width + 7) / 8;

  memset(black, 0, height * d->dither[ECOLOR_K].signif_bits);
  if (zero_mask)
    return;

  bit = 128;
  x = 0;
  terminate = d->dst_width;
  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  xerror = 0;

  kptr = black;
  matrix_set_row(d, &(d->dithermat[ECOLOR_K]), row);
  matrix_set_row(d, &(d->pick[ECOLOR_K]), row);

  for (x = 0; x < terminate; x ++)
    {
      ink_budget = d->ink_limit;

      k = gray[0];
      print_color(d, &(d->dither[ECOLOR_K]), k, k, k, x, row, kptr, NULL, bit,
		  height, d->randomizer[ECOLOR_K], 0, &ink_budget,
		  &(d->pick[ECOLOR_K]), &(d->dithermat[ECOLOR_K]),
		  d->dither_type);
      bit >>= 1;
      if (bit == 0)
	{
	  kptr ++;
	  bit = 128;
	}
      if (d->src_width == d->dst_width)
	gray++;
      else
	{
	  gray += xstep;
	  xerror += xmod;
	  if (xerror >= d->dst_width)
	    {
	      xerror -= d->dst_width;
	      gray++;
	    }
	}	  
    }
}

static void
stp_dither_black_ed(const unsigned short   *gray,
		    int           	row,
		    void 		*vd,
		    unsigned char 	*black,
		    int		duplicate_line,
		    int		  zero_mask)
{

  int		x,		/* Current X coordinate */
		height;		/* Height of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k, ok,		/* Current black error */
		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  dither_t *d = (dither_t *) vd;
  int terminate;
  int direction = row & 1 ? 1 : -1;
  int odb = d->spread;
  int odb_mask = (1 << odb) - 1;
  int ink_budget;
  int xerror, xstep, xmod;

  bit = (direction == 1) ? 128 : 1 << (7 - ((d->dst_width - 1) & 7));
  x = (direction == 1) ? 0 : d->dst_width - 1;
  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  xerror = (direction == 1) ? 0 : (xmod * (d->dst_width - 1)) % d->dst_width;
  if (direction == -1)
    gray += d->src_width - 1;
  terminate = (direction == 1) ? d->dst_width : -1;

  height = (d->dst_width + 7) / 8;

  kerror0 = get_errline(d, row, ECOLOR_K);
  kerror1 = get_errline(d, row + 1, ECOLOR_K);

  memset(black, 0, height * d->dither[ECOLOR_K].signif_bits);
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

  memset(kerror1, 0, d->dst_width * sizeof(int));

  if (d->last_line_was_empty >= 4)
    {
      if (d->last_line_was_empty == 4)
	memset(kerror0, 0, d->dst_width * sizeof(int));
      return;
    }

  kptr = black;
  if (direction == -1)
    {
      kerror0 += d->dst_width - 1;
      kerror1 += d->dst_width - 1;
      kptr = black + height - 1;
    }
  matrix_set_row(d, &(d->dithermat[ECOLOR_K]), row);
  matrix_set_row(d, &(d->pick[ECOLOR_K]), row);

  for (ditherk = kerror0[0];
       x != terminate;
       x += direction,
	 kerror0 += direction,
	 kerror1 += direction)
    {
      ink_budget = d->ink_limit;

      k = gray[0];
      ok = k;
      k = UPDATE_COLOR(k, ditherk);
      k = print_color(d, &(d->dither[ECOLOR_K]), ok, ok, k, x, row, kptr, NULL,
		      bit, height, d->randomizer[ECOLOR_K], 0, &ink_budget,
		      &(d->pick[ECOLOR_K]), &(d->dithermat[ECOLOR_K]),
		      d->dither_type);
      ditherk = update_dither(k, ok, d->src_width, odb, odb_mask,
			      direction, kerror0, kerror1, d);

      if (direction == 1)
	{
	  bit >>= 1;
	  if (bit == 0)
	    {
	      kptr ++;
	      bit = 128;
	    }
	  if (d->src_width == d->dst_width)
	    gray++;
	  else
	    {
	      gray += xstep;
	      xerror += xmod;
	      if (xerror >= d->dst_width)
		{
		  xerror -= d->dst_width;
		  gray++;
		}
	    }	  
	}
      else
	{
	  if (bit == 128)
	    {
	      kptr --;
	      bit = 1;
	    }
	  else
	    bit <<= 1;
	  if (d->src_width == d->dst_width)
	    gray--;
	  else
	    {
	      gray -= xstep;
	      xerror -= xmod;
	      if (xerror < 0)
		{
		  xerror += d->dst_width;
		  gray--;
		}
	    }	  
	}
    }
}

#define USMIN(a, b) ((a) < (b) ? (a) : (b))

static inline void
update_cmy(const dither_t *d, int c, int m, int y, int k,
	    int *nc, int *nm, int *ny)
{
  /*
   * We're not printing black, but let's adjust the CMY levels to
   * produce better reds, greens, and blues...
   *
   * This code needs to be tuned
   */

  unsigned ck = c - k;
  unsigned mk = m - k;
  unsigned yk = y - k;

  *nc  = ((unsigned) (65535 - c / 4)) * ck / 65535 + k;
  *nm  = ((unsigned) (65535 - m / 4)) * mk / 65535 + k;
  *ny  = ((unsigned) (65535 - y / 4)) * yk / 65535 + k;
}

static inline void
update_cmyk(const dither_t *d, int c, int m, int y, int k,
	    int *nc, int *nm, int *ny, int *nk, int *jk)
{
  int ak;
  int kdarkness;
  unsigned ks, kl;
  int ub, lb;
  int ok;
  int bk;
  unsigned density;

  ub = d->k_upper;    /* Upper bound */
  lb = d->k_lower;    /* Lower bound */
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

  kdarkness = c + c + m + m + y - d->density2;
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

      if (d->k_clevel == 64)
	c -= ok;
      else
	c -= (ok * d->k_clevel) >> 6;
      if (c < 0)
	c = 0;

      if (d->k_mlevel == 64)
	m -= ok;
      else
	m -= (ok * d->k_mlevel) >> 6;
      if (m < 0)
	m = 0;

      if (d->k_ylevel == 64)
	y -= ok;
      else
	y -= (ok * d->k_ylevel) >> 6;
      if (y < 0)
	y = 0;
    }
  *nc = c;
  *nm = m;
  *ny = y;
  *nk = bk;
  *jk = k;
}

static void
stp_dither_cmyk_fast(const unsigned short  *cmy,
		     int           row,
		     void 	    *vd,
		     unsigned char *cyan,
		     unsigned char *lcyan,
		     unsigned char *magenta,
		     unsigned char *lmagenta,
		     unsigned char *yellow,
		     unsigned char *lyellow,
		     unsigned char *black,
		     int	       duplicate_line,
		     int		  zero_mask)
{
  int		x,		/* Current X coordinate */
		height;		/* Height of output bitmap in bytes */
  int		c, m, y, k,	/* CMYK values */
    		oc, om, oy, ok;
  unsigned char	bit,		/* Current bit */
    		*cptr,		/* Current cyan pixel */
    		*mptr,		/* Current magenta pixel */
    		*yptr,		/* Current yellow pixel */
    		*lmptr,		/* Current light magenta pixel */
    		*lcptr,		/* Current light cyan pixel */
    		*lyptr,		/* Current light yellow pixel */
    		*kptr;		/* Current black pixel */
  dither_t	*d = (dither_t *) vd;
  int i;

  dither_color_t *cd = &(d->dither[ECOLOR_C]);
  dither_matrix_t *cdither = &(d->dithermat[ECOLOR_C]);
  dither_color_t *md = &(d->dither[ECOLOR_M]);
  dither_matrix_t *mdither = &(d->dithermat[ECOLOR_M]);
  dither_color_t *yd = &(d->dither[ECOLOR_Y]);
  dither_matrix_t *ydither = &(d->dithermat[ECOLOR_Y]);
  dither_color_t *kd = &(d->dither[ECOLOR_K]);
  dither_matrix_t *kdither = &(d->dithermat[ECOLOR_K]);
  int dst_width = d->dst_width;
  int cdither_very_fast = 0;
  int mdither_very_fast = 0;
  int ydither_very_fast = 0;
  int kdither_very_fast = 0;
  int xerror, xstep, xmod;

  height = (d->dst_width + 7) / 8;

  if (cyan)
    memset(cyan, 0, height * d->dither[ECOLOR_C].signif_bits);
  if (lcyan)
    memset(lcyan, 0, height * d->dither[ECOLOR_C].signif_bits);
  if (magenta)
    memset(magenta, 0, height * d->dither[ECOLOR_M].signif_bits);
  if (lmagenta)
    memset(lmagenta, 0, height * d->dither[ECOLOR_M].signif_bits);
  if (yellow)
    memset(yellow, 0, height * d->dither[ECOLOR_Y].signif_bits);
  if (lyellow)
    memset(lyellow, 0, height * d->dither[ECOLOR_Y].signif_bits);
  if (black)
    memset(black, 0, height * d->dither[ECOLOR_K].signif_bits);

  if ((zero_mask & 7) == 7)
    return;

  if (cd->nlevels == 1 && cd->ranges[0].bits_h == 1 && cd->ranges[0].isdark_h)
    cdither_very_fast = 1;
  if (md->nlevels == 1 && md->ranges[0].bits_h == 1 && md->ranges[0].isdark_h)
    mdither_very_fast = 1;
  if (yd->nlevels == 1 && yd->ranges[0].bits_h == 1 && yd->ranges[0].isdark_h)
    ydither_very_fast = 1;
  if (kd->nlevels == 1 && kd->ranges[0].bits_h == 1 && kd->ranges[0].isdark_h)
    kdither_very_fast = 1;

  /*
   * Boilerplate
   */

  bit = 128;
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  x = 0;
  cptr = cyan;
  mptr = magenta;
  yptr = yellow;
  lcptr = lcyan;
  lmptr = lmagenta;
  lyptr = lyellow;
  kptr = black;

  k = 0;			/* Shut up the compiler */
  ok = 0;
  for (i = 0; i < NCOLORS; i++)
    matrix_set_row(d, &(d->dithermat[i]), row);

  /*
   * Main loop starts here!
   */
  QUANT(14);
  for (; x != dst_width; x++)
    {
      /*
       * First get the standard CMYK separation color values.
       */

      c = cmy[0];
      m = cmy[1];
      y = cmy[2];
      oc = c;
      om = m;
      oy = y;

      /*
       * If we're doing ordered dither, and there's no ink, we aren't
       * going to print anything.
       */
      if (c > 0 || m > 0 || y > 0)
	{
	  if (black)
	    {
	      k = USMIN(c, USMIN(m, y));
	      if (k < d->densityh)
		k = 0;
	      else
		k = (d->density - 1) - ((d->density - 1 - k) * 2);
	      c -= k;
	      m -= k;
	      y -= k;
	      ok = k;
	      if (ok > 0 && d->density != d->black_density)
		ok = (unsigned) ok * (unsigned) d->black_density / d->density;
	      if (ok > 65535)
		ok = 65535;
	    }
	  QUANT(15);

	  if (black)
	    print_color_fast(d, kd, ok, k, x, row, kptr, NULL, bit, height,
			     kdither, kdither_very_fast);
	  print_color_fast(d, cd, oc, c, x, row, cptr, lcptr, bit, height,
			   cdither, cdither_very_fast);
	  print_color_fast(d, md, om, m, x, row, mptr, lmptr, bit, height,
			   mdither, mdither_very_fast);
	  print_color_fast(d, yd, oy, y, x, row, yptr, lyptr, bit, height,
			   ydither, ydither_very_fast);
	  QUANT(16);
	}

      /*****************************************************************
       * Advance the loop
       *****************************************************************/

      bit >>= 1;
      if (bit == 0)
	{
	  cptr ++;
	  lcptr ++;
	  mptr ++;
	  lmptr ++;
	  yptr ++;
	  lyptr ++;
	  kptr ++;
	  bit       = 128;
	}
      if (d->src_width == d->dst_width)
	cmy += 3;
      else
	{
	  cmy += xstep;
	  xerror += xmod;
	  if (xerror >= d->dst_width)
	    {
	      xerror -= d->dst_width;
	      cmy += 3;
	    }
	}	  
      QUANT(17);
    }
  /*
   * Main loop ends here!
   */
}

static void
stp_dither_cmyk_ordered(const unsigned short  *cmy,
			int           row,
			void 	    *vd,
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
  int		x,		/* Current X coordinate */
		height;		/* Height of output bitmap in bytes */
  int		c, m, y, k,	/* CMYK values */
		oc, om, oy;
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*lmptr,		/* Current light magenta pixel */
		*lcptr,		/* Current light cyan pixel */
		*lyptr,		/* Current light yellow pixel */
		*kptr;		/* Current black pixel */
  int		bk = 0;
  dither_t	*d = (dither_t *) vd;
  int i;

  int		terminate;
  int		first_color = row % 3;
  int		printed_black;
  int		ink_budget;
  int xerror, xstep, xmod;

  height = (d->dst_width + 7) / 8;

  if (cyan)
    memset(cyan, 0, height * d->dither[ECOLOR_C].signif_bits);
  if (lcyan)
    memset(lcyan, 0, height * d->dither[ECOLOR_C].signif_bits);
  if (magenta)
    memset(magenta, 0, height * d->dither[ECOLOR_M].signif_bits);
  if (lmagenta)
    memset(lmagenta, 0, height * d->dither[ECOLOR_M].signif_bits);
  if (yellow)
    memset(yellow, 0, height * d->dither[ECOLOR_Y].signif_bits);
  if (lyellow)
    memset(lyellow, 0, height * d->dither[ECOLOR_Y].signif_bits);
  if (black)
    memset(black, 0, height * d->dither[ECOLOR_K].signif_bits);
  /*
   * First, generate the CMYK separation.  If there's nothing in
   * this row, and we're using an ordered dither, there's no reason
   * to do anything at all.
   */
  if ((zero_mask & 7) == 7)
    return;

  /*
   * Boilerplate
   */

  bit = 128;
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  x = 0;
  terminate = d->dst_width;
  cptr = cyan;
  mptr = magenta;
  yptr = yellow;
  lcptr = lcyan;
  lmptr = lmagenta;
  lyptr = lyellow;
  kptr = black;

  for (i = 0; i < NCOLORS; i++)
    {
      matrix_set_row(d, &(d->dithermat[i]), row);
      matrix_set_row(d, &(d->pick[i]), row);
    }
   QUANT(6);
  /*
   * Main loop starts here!
   */
  for (; x != terminate; x ++)
    {
      /*
       * First get the standard CMYK separation color values.
       */

      c = cmy[0];
      m = cmy[1];
      y = cmy[2];
      oc = c;
      om = m;
      oy = y;

      /*
       * If we're doing ordered dither, and there's no ink, we aren't
       * going to print anything.
       */
      if (c == 0 && m == 0 && y == 0)
	goto advance;
      QUANT(7);

      k = USMIN(c, USMIN(m, y));

      /*
       * At this point we've computed the basic CMYK separations.
       * Now we adjust the levels of each to improve the print quality.
       */

      if (k > 0)
	{
	  if (black != NULL)
	    update_cmyk(d, oc, om, oy, k, &c, &m, &y, &bk, &k);
	  else
	    update_cmy(d, oc, om, oy, k, &c, &m, &y);
	}

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
       * (in particular light and dark variants) of inks.  Very dark regions
       * with slight color tints should be printed with dark inks, not with
       * the light inks that would be implied by the small amount of remnant
       * CMY.
       *
       * It's quite likely that for simple four-color printers ordinary
       * CMYK separations would work.  It's possible that they would work
       * for variable dot sizes, too.
       */

      QUANT(9);

      ink_budget = d->ink_limit;

      if (black)
	{
	  int tk;
	  if (d->density != d->black_density)
	    k = k * d->black_density / d->density;
	  tk = print_color(d, &(d->dither[ECOLOR_K]), bk, bk, k, x, row,
			       kptr, NULL, bit, height, 0, 0, &ink_budget,
			       &(d->pick[ECOLOR_K]), &(d->dithermat[ECOLOR_K]),
			       d->dither_type);
	  printed_black = k - tk;
	  k = tk;
	}
      else
	printed_black = 0;

      QUANT(10);
      /*
       * If the printed density is high, ink reduction loses too much
       * ink.  However, at low densities it seems to be safe.  Of course,
       * at low densities it won't do as much.
       */
      if (d->density > 45000)
	printed_black = 0;

      /*
       * Uh oh spaghetti-o!
       *
       * It has been determined experimentally that inlining print_color
       * saves a substantial amount of time.  However, expanding this out
       * as a switch drastically increases the code volume by about 10 KB.
       * The solution for now (until we do this properly, via an array)
       * is to use this ugly code.
       */

      if (first_color == ECOLOR_M)
	goto ecm;
      else if (first_color == ECOLOR_Y)
	goto ecy;
    ecc:
      print_color(d, &(d->dither[ECOLOR_C]), oc, oc, c, x, row, cptr, lcptr,
		  bit, height, d->randomizer[ECOLOR_C], printed_black,
		  &ink_budget, &(d->pick[ECOLOR_C]), &(d->dithermat[ECOLOR_C]),
		  d->dither_type);
      if (first_color == ECOLOR_M)
	goto out;
    ecm:
      print_color(d, &(d->dither[ECOLOR_M]), om, om, m, x, row, mptr, lmptr,
		  bit, height, d->randomizer[ECOLOR_M], printed_black,
		  &ink_budget, &(d->pick[ECOLOR_M]), &(d->dithermat[ECOLOR_M]),
		  d->dither_type);
      if (first_color == ECOLOR_Y)
	goto out;
    ecy:
      print_color(d, &(d->dither[ECOLOR_Y]), oy, oy, y, x, row, yptr, lyptr,
		  bit, height, d->randomizer[ECOLOR_Y], printed_black,
		  &ink_budget, &(d->pick[ECOLOR_Y]), &(d->dithermat[ECOLOR_Y]),
		  d->dither_type);
      if (first_color != ECOLOR_C)
	goto ecc;
    out:

      QUANT(11);
      /*****************************************************************
       * Advance the loop
       *****************************************************************/

      QUANT(12);
    advance:
      bit >>= 1;
      if (bit == 0)
	{
	  cptr ++;
	  lcptr ++;
	  mptr ++;
	  lmptr ++;
	  yptr ++;
	  lyptr ++;
	  kptr ++;
	  bit       = 128;
	}
      first_color++;
      if (first_color >= 3)
	first_color = 0;
      if (d->src_width == d->dst_width)
	cmy += 3;
      else
	{
	  cmy += xstep;
	  xerror += xmod;
	  if (xerror >= d->dst_width)
	    {
	      xerror -= d->dst_width;
	      cmy += 3;
	    }
	}	  

      QUANT(13);
  }
  /*
   * Main loop ends here!
   */
}

static void
stp_dither_cmyk_ed(const unsigned short  *cmy,
		   int           row,
		   void 	    *vd,
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
  int		x,		/* Current X coordinate */
    		height;		/* Height of output bitmap in bytes */
  int		c, m, y, k,	/* CMYK values */
		oc, om, oy;
  unsigned char	bit,		/* Current bit */
    		*cptr,		/* Current cyan pixel */
    		*mptr,		/* Current magenta pixel */
    		*yptr,		/* Current yellow pixel */
    		*lmptr,		/* Current light magenta pixel */
    		*lcptr,		/* Current light cyan pixel */
    		*lyptr,		/* Current light yellow pixel */
    		*kptr;		/* Current black pixel */
  int		i, j;
  int		ndither[NCOLORS];
  int		*error[NCOLORS][ERROR_ROWS];
  int		bk = 0;
  dither_t	*d = (dither_t *) vd;

  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int		odb = d->spread;
  int		odb_mask = (1 << odb) - 1;
  int		first_color = row % 3;
  int		printed_black;
  int		ink_budget;
  int xerror, xstep, xmod;

  height = (d->dst_width + 7) / 8;

  if (cyan)
    memset(cyan, 0, height * d->dither[ECOLOR_C].signif_bits);
  if (lcyan)
    memset(lcyan, 0, height * d->dither[ECOLOR_C].signif_bits);
  if (magenta)
    memset(magenta, 0, height * d->dither[ECOLOR_M].signif_bits);
  if (lmagenta)
    memset(lmagenta, 0, height * d->dither[ECOLOR_M].signif_bits);
  if (yellow)
    memset(yellow, 0, height * d->dither[ECOLOR_Y].signif_bits);
  if (lyellow)
    memset(lyellow, 0, height * d->dither[ECOLOR_Y].signif_bits);
  if (black)
    memset(black, 0, height * d->dither[ECOLOR_K].signif_bits);
  /*
   * First, generate the CMYK separation.  If there's nothing in
   * this row, and we're using an ordered dither, there's no reason
   * to do anything at all.
   */
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

  /*
   * Boilerplate
   */

  bit = (direction == 1) ? 128 : 1 << (7 - ((d->dst_width - 1) & 7));
  x = (direction == 1) ? 0 : d->dst_width - 1;
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (direction == 1) ? 0 : (xmod * (d->dst_width - 1)) % d->dst_width;
  if (direction == -1)
    cmy += (3 * (d->src_width - 1));
  terminate = (direction == 1) ? d->dst_width : -1;

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
  cptr = cyan;
  mptr = magenta;
  yptr = yellow;
  lcptr = lcyan;
  lmptr = lmagenta;
  lyptr = lyellow;
  kptr = black;
  if (direction == -1)
    {
      for (i = 0; i < NCOLORS; i++)
	{
	  for (j = 0; j < ERROR_ROWS; j++)
	    error[i][j] += d->dst_width - 1;
	}
      cptr = cyan + height - 1;
      lcptr = lcyan + height - 1;
      mptr = magenta + height - 1;
      lmptr = lmagenta + height - 1;
      yptr = yellow + height - 1;
      lyptr = lyellow + height - 1;
      kptr = black + height - 1;
      first_color = (first_color + d->dst_width - 1) % 3;
    }
  for (i = 0; i < NCOLORS; i++)
    {
      ndither[i] = error[i][0][0];
      matrix_set_row(d, &(d->dithermat[i]), row);
      matrix_set_row(d, &(d->pick[i]), row);
    }
  QUANT(6);
  /*
   * Main loop starts here!
   */
  for (; x != terminate; x += direction)
    {
      /*
       * First get the standard CMYK separation color values.
       */

      c = cmy[0];
      m = cmy[1];
      y = cmy[2];
      oc = c;
      om = m;
      oy = y;

      /*
       * If we're doing ordered dither, and there's no ink, we aren't
       * going to print anything.
       */
      if (c == 0 && m == 0 && y == 0)
	{
	  c = UPDATE_COLOR(c, ndither[ECOLOR_C]);
	  m = UPDATE_COLOR(m, ndither[ECOLOR_M]);
	  y = UPDATE_COLOR(y, ndither[ECOLOR_Y]);
	  k = UPDATE_COLOR(0, ndither[ECOLOR_K]);
	  goto out;
	}

      QUANT(7);

      k = USMIN(c, USMIN(m, y));
      bk = 0;

      /*
       * At this point we've computed the basic CMYK separations.
       * Now we adjust the levels of each to improve the print quality.
       */

      if (k > 0)
	{
	  if (black != NULL)
	    update_cmyk(d, oc, om, oy, k, &c, &m, &y, &bk, &k);
	  else
	    update_cmy(d, oc, om, oy, k, &c, &m, &y);
	}

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
       * (in particular light and dark variants) of inks.  Very dark regions
       * with slight color tints should be printed with dark inks, not with
       * the light inks that would be implied by the small amount of remnant
       * CMY.
       *
       * It's quite likely that for simple four-color printers ordinary
       * CMYK separations would work.  It's possible that they would work
       * for variable dot sizes, too.
       */

      c = UPDATE_COLOR(c, ndither[ECOLOR_C]);
      m = UPDATE_COLOR(m, ndither[ECOLOR_M]);
      y = UPDATE_COLOR(y, ndither[ECOLOR_Y]);
      if (d->density != d->black_density)
	k = k * d->black_density / d->density;
      k = UPDATE_COLOR(k, ndither[ECOLOR_K]);

      QUANT(9);

      ink_budget = d->ink_limit;

      if (black)
	{
	  int tk = print_color(d, &(d->dither[ECOLOR_K]), bk, bk, k, x, row,
			       kptr, NULL, bit, height, 0, 0, &ink_budget,
			       &(d->pick[ECOLOR_K]), &(d->dithermat[ECOLOR_K]),
			       d->dither_type);
	  printed_black = k - tk;
	  k = tk;
	}
      else
        printed_black = 0;

      QUANT(10);
      /*
       * If the printed density is high, ink reduction loses too much
       * ink.  However, at low densities it seems to be safe.  Of course,
       * at low densities it won't do as much.
       */
      if (d->density > 45000)
	printed_black = 0;

      /*
       * Uh oh spaghetti-o!
       *
       * It has been determined experimentally that inlining print_color
       * saves a substantial amount of time.  However, expanding this out
       * as a switch drastically increases the code volume by about 10 KB.
       * The solution for now (until we do this properly, via an array)
       * is to use this ugly code.
       */

      if (first_color == ECOLOR_M)
	goto ecm;
      else if (first_color == ECOLOR_Y)
	goto ecy;
    ecc:
      c = print_color(d, &(d->dither[ECOLOR_C]), oc, oc,
		      c, x, row, cptr, lcptr, bit, height,
		      d->randomizer[ECOLOR_C], printed_black, &ink_budget,
		      &(d->pick[ECOLOR_C]), &(d->dithermat[ECOLOR_C]),
		      d->dither_type);
      if (first_color == ECOLOR_M)
	goto out;
    ecm:
      m = print_color(d, &(d->dither[ECOLOR_M]), om, om,
		      m, x, row, mptr, lmptr, bit, height,
		      d->randomizer[ECOLOR_M], printed_black, &ink_budget,
		      &(d->pick[ECOLOR_M]), &(d->dithermat[ECOLOR_M]),
		      d->dither_type);
      if (first_color == ECOLOR_Y)
	goto out;
    ecy:
      y = print_color(d, &(d->dither[ECOLOR_Y]), oy, oy,
		      y, x, row, yptr, lyptr, bit, height,
		      d->randomizer[ECOLOR_Y], printed_black, &ink_budget,
		      &(d->pick[ECOLOR_Y]), &(d->dithermat[ECOLOR_Y]),
		      d->dither_type);
      if (first_color != ECOLOR_C)
	goto ecc;
    out:

      QUANT(11);
      ndither[ECOLOR_C] = update_dither(c, oc, d->src_width, odb, odb_mask,
					direction, error[ECOLOR_C][0],
					error[ECOLOR_C][1], d);
      ndither[ECOLOR_M] = update_dither(m, om, d->src_width, odb, odb_mask,
					direction, error[ECOLOR_M][0],
					error[ECOLOR_M][1], d);
      ndither[ECOLOR_Y] = update_dither(y, oy, d->src_width, odb, odb_mask,
					direction, error[ECOLOR_Y][0],
					error[ECOLOR_Y][1], d);
      ndither[ECOLOR_K] = update_dither(k, bk, d->src_width, odb, odb_mask,
					direction, error[ECOLOR_K][0],
					error[ECOLOR_K][1], d);

      /*****************************************************************
       * Advance the loop
       *****************************************************************/

      QUANT(12);
      if (direction == 1)
	{
	  bit >>= 1;
	  if (bit == 0)
	    {
	      cptr ++;
	      lcptr ++;
	      mptr ++;
	      lmptr ++;
	      yptr ++;
	      lyptr ++;
	      kptr ++;
	      bit       = 128;
	    }
	  first_color++;
	  if (first_color >= 3)
	    first_color = 0;
	  if (d->src_width == d->dst_width)
	    cmy += 3;
	  else
	    {
	      cmy += xstep;
	      xerror += xmod;
	      if (xerror >= d->dst_width)
		{
		  xerror -= d->dst_width;
		  cmy += 3;
		}
	    }	  
	}
      else
	{
	  if (bit == 128)
	    {
	      cptr --;
	      lcptr --;
	      mptr --;
	      lmptr --;
	      yptr --;
	      lyptr --;
	      kptr --;
	      bit       = 1;
	    }
	  else
	    bit <<= 1;
	  first_color--;
	  if (first_color <= 0)
	    first_color = 2;
	  if (d->src_width == d->dst_width)
	    cmy -= 3;
	  else
	    {
	      cmy -= xstep;
	      xerror -= xmod;
	      if (xerror < 0)
		{
		  xerror += d->dst_width;
		  cmy -= 3;
		}
	    }	  
	}
      for (i = 0; i < NCOLORS; i++)
	for (j = 0; j < ERROR_ROWS; j++)
	  error[i][j] += direction;
      QUANT(13);
    }
  /*
   * Main loop ends here!
   */
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
  dither_t *d = (dither_t *) vd;
  switch (d->dither_class)
    {
    case DITHER_MONOCHROME:
      stp_dither_monochrome(input, row, vd, black, duplicate_line, zero_mask);
      break;
    case DITHER_BLACK:
      if (d->dither_type & D_FAST_BASE)
	stp_dither_black_fast(input, row, vd, black, duplicate_line,
			      zero_mask);
      else if (d->dither_type & D_ORDERED_BASE)
	stp_dither_black_ordered(input, row, vd, black, duplicate_line,
				 zero_mask);
      else
	stp_dither_black_ed(input, row, vd, black, duplicate_line, zero_mask);
      break;
    case DITHER_CMYK:
      if (d->dither_type & D_FAST_BASE)
	stp_dither_cmyk_fast(input, row, vd, cyan, lcyan, magenta, lmagenta,
			     yellow, lyellow, black, duplicate_line,
			     zero_mask);
      else if (d->dither_type & D_ORDERED_BASE)
	stp_dither_cmyk_ordered(input, row, vd, cyan, lcyan, magenta, lmagenta,
				yellow, lyellow, black, duplicate_line,
				zero_mask);
      else
	stp_dither_cmyk_ed(input, row, vd, cyan, lcyan, magenta, lmagenta,
			   yellow, lyellow, black, duplicate_line, zero_mask);
    }
}
