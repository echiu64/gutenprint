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
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
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
#define D_EVENTONE 32

#define DITHER_FAST_STEPS (6)

typedef struct
{
  const char *name;
  const char *text;
  int id;
} dither_algo_t;

static const dither_algo_t dither_algos[] =
{
  /* Note to translators: "EvenTone" is the proper name, rather than a */
  /* descriptive name, of this algorithm. */  
  { "EvenTone", N_ ("EvenTone"),               D_EVENTONE },
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
 * An end of a dither segment, describing one ink
 */

typedef struct ink_defn
{
  unsigned range;
  unsigned value;
  unsigned bits;
  unsigned dot_size;
  int subchannel;
} ink_defn_t;
 
/*
 * A segment of the entire 0-65535 intensity range.
 */

typedef struct dither_segment
{
  ink_defn_t *lower;
  ink_defn_t *upper;
  unsigned range_span;
  unsigned value_span;
  int is_same_ink;
  int is_equal;
} dither_segment_t;

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
  int nlevels;
  unsigned bit_max;
  unsigned signif_bits;
  unsigned density;

  int v;
  int o;
  int b;
  int very_fast;
  int subchannels;

  int maxdot;			/* Maximum dot size */

  ink_defn_t *ink_list;

  dither_segment_t *ranges;
  int **errs;
  unsigned short *vals;

  dither_matrix_t pick;
  dither_matrix_t dithermat;
  int *row_ends[2];
  unsigned char **ptrs;
} dither_channel_t;

typedef struct {
	int dx, dy, r_sq;
} dis_t;

typedef struct {
	int	d2x, d2y;
	dis_t	d_sq;
	int	aspect;
	dis_t	**dis;
	int	*recip;
} eventone_t;

typedef struct {
	dis_t dis;
	int wetness, point;
	int maxdot_dens;		/* Max dot size * density */
	int maxdot_wet;			/* Maximum wetness allowed */
	dither_segment_t dr;
} et_chdata_t;

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
  int adaptive_input_set;
  int adaptive_limit;

  int x_aspect;			/* Aspect ratio numerator */
  int y_aspect;			/* Aspect ratio denominator */

  double transition;		/* Exponential scaling for transition region */

  int *offset0_table;
  int *offset1_table;

  int oversampling;
  int last_line_was_empty;
  int ptr_offset;
  int n_channels;
  int n_input_channels;
  int n_ghost_channels;		/* FIXME: For composite grayscale
  				 * This is a really ugly way of doing this.
  				 * Of course, ultimately we'll eliminate
  				 * the CMYK special casing, which will
  				 * get rid of this. */
  int error_rows;

  int dither_class;		/* mono, black, or CMYK */

  dither_matrix_t dither_matrix;
  dither_matrix_t transition_matrix;
  dither_channel_t *channel;

  unsigned short virtual_dot_scale[65536];
  void (*ditherfunc)(const unsigned short *, int, struct dither *, int, int);
  eventone_t *eventone;
  stp_vars_t v;
} dither_t;

typedef void ditherfunc_t(const unsigned short *, int, struct dither *, int, int);

static ditherfunc_t
  stp_dither_monochrome,
  stp_dither_monochrome_very_fast,
  stp_dither_cmyk_fast,
  stp_dither_cmyk_very_fast,
  stp_dither_cmyk_ordered,
  stp_dither_cmyk_ed,
  stp_dither_cmyk_et,
  stp_dither_raw_cmyk_fast,
  stp_dither_raw_cmyk_very_fast,
  stp_dither_raw_cmyk_ordered,
  stp_dither_raw_cmyk_ed,
  stp_dither_raw_cmyk_et,
  stp_dither_raw_fast,
  stp_dither_raw_very_fast,
  stp_dither_raw_ordered,
  stp_dither_raw_ed,
  stp_dither_raw_et;


#define CHANNEL(d, c) ((d)->channel[(c) + (d)->n_ghost_channels])
#define PHYSICAL_CHANNEL(d, c) ((d)->channel[(c)])
#define CHANNEL_COUNT(d) ((d)->n_channels - (d)->n_ghost_channels)
#define PHYSICAL_CHANNEL_COUNT(d) ((d)->n_channels)

#define SAFE_FREE(x)				\
do						\
{						\
  if ((x))					\
    stp_free((char *)(x));			\
  ((x)) = NULL;					\
} while (0)

/*
 * Bayer's dither matrix using Judice, Jarvis, and Ninke recurrence relation
 * http://www.cs.rit.edu/~sxc7922/Project/CRT.htm
 */

static const unsigned sq2[] =
{
  0, 2,
  3, 1
};

void
stp_dither_algorithms(stp_string_list_t valptrs)
{
  int i;
  for (i = 0; i < num_dither_algos; i++)
    stp_string_list_add_param(valptrs, dither_algos[i].name,
			     _(dither_algos[i].text));
}

/*
 * These really belong with print-dither.c.  However, inlining has yielded
 * significant (measured) speedup, even with the more complicated dither
 * function. --rlk 20011219
 */

static inline unsigned
ditherpoint_fast(const dither_t *d, dither_matrix_t *mat, int x)
{
  return mat->matrix[(mat->last_y_mod+((x + mat->x_offset) & mat->fast_mask))];
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
  for (i = 0; i < CHANNEL_COUNT(d); i++)
    for (j = 0; j < CHANNEL(d, i).subchannels; j++)
      {
	int tmp = CHANNEL(d, i).row_ends[0][j];
	CHANNEL(d, i).row_ends[0][j] =
	  CHANNEL(d, i).row_ends[1][j];
	CHANNEL(d, i).row_ends[1][j] = tmp;
      }
}

stp_dither_data_t *
stp_dither_data_allocate(void)
{
  stp_dither_data_t *ret = stp_zalloc(sizeof(stp_dither_data_t));
  ret->channel_count = 0;
  ret->c = NULL;
  return ret;
}

void
stp_dither_add_channel(stp_dither_data_t *d, unsigned char *data,
		unsigned channel, unsigned subchannel)
{
  stp_dither_channel_t *chan;
  if (channel >= d->channel_count)
    {
      unsigned oc = d->channel_count;
      d->c = stp_realloc(d->c, sizeof(stp_dither_channel_t) * (channel + 1));
      (void) memset(d->c + oc, 0, sizeof(stp_dither_channel_t) * (channel + 1 - oc));
      d->channel_count = channel + 1;
    }
  chan = d->c + channel;
  if (subchannel >= chan->subchannel_count)
    {
      unsigned oc = chan->subchannel_count;
      chan->c =
	stp_realloc(chan->c, sizeof(unsigned char *) * (subchannel + 1));
      (void) memset(chan->c + oc, 0,
		    sizeof(unsigned char *) * (subchannel + 1 - oc));
      chan->subchannel_count = subchannel + 1;
    }
  chan->c[subchannel] = data;
}

void
stp_dither_data_free(stp_dither_data_t *d)
{
  int i;
  for (i = 0; i < d->channel_count; i++)
    stp_free(d->c[i].c);
  stp_free(d->c);
  stp_free(d);
}

#define RETURN_DITHERFUNC(func, v)				\
do								\
{								\
  stp_dprintf(STP_DBG_COLORFUNC, v, "ditherfunc %s\n", #func);	\
  return (func);						\
} while (0)

static ditherfunc_t *
stp_set_dither_function(dither_t *d, int image_bpp)
{
  int i;
  const char *algorithm = stp_get_string_parameter(d->v, "DitherAlgorithm");
  d->dither_type = D_ADAPTIVE_HYBRID;
  if (algorithm)
    {
      for (i = 0; i < num_dither_algos; i++)
	{
	  if (!strcmp(algorithm, _(dither_algos[i].name)))
	    {
	      d->dither_type = dither_algos[i].id;
	      break;
	    }
	}
    }
  switch (d->dither_class)
    {
    case OUTPUT_MONOCHROME:
      d->n_channels = 1;
      d->n_input_channels = 1;
      switch (d->dither_type)
	{
	case D_VERY_FAST:
	  RETURN_DITHERFUNC(stp_dither_monochrome_very_fast, d->v);
	default:
	  RETURN_DITHERFUNC(stp_dither_monochrome, d->v);
	}
      break;
    case OUTPUT_GRAY:
      d->n_channels = 1;
      d->n_input_channels = 1;
      switch (d->dither_type)
	{
	case D_FAST:
	  RETURN_DITHERFUNC(stp_dither_raw_fast, d->v);
	case D_VERY_FAST:
	  RETURN_DITHERFUNC(stp_dither_raw_very_fast, d->v);
	case D_ORDERED:
	  RETURN_DITHERFUNC(stp_dither_raw_ordered, d->v);
	case D_EVENTONE:
	  RETURN_DITHERFUNC(stp_dither_raw_et, d->v);
	default:
	  RETURN_DITHERFUNC(stp_dither_raw_ed, d->v);
	}
      break;
    case OUTPUT_RAW_PRINTER:
      d->n_channels = image_bpp / 2;
      d->n_input_channels = image_bpp / 2;
      switch (d->dither_type)
	{
	case D_FAST:
	  RETURN_DITHERFUNC(stp_dither_raw_fast, d->v);
	case D_VERY_FAST:
	  RETURN_DITHERFUNC(stp_dither_raw_very_fast, d->v);
	case D_ORDERED:
	  RETURN_DITHERFUNC(stp_dither_raw_ordered, d->v);
	case D_EVENTONE:
	  RETURN_DITHERFUNC(stp_dither_raw_et, d->v);
	default:
	  RETURN_DITHERFUNC(stp_dither_raw_ed, d->v);
	}
      break;
    case OUTPUT_COLOR:
      d->n_channels = 4;
      d->n_input_channels = 3;
      switch (d->dither_type)
	{
	case D_FAST:
	  RETURN_DITHERFUNC(stp_dither_cmyk_fast, d->v);
	case D_VERY_FAST:
	  RETURN_DITHERFUNC(stp_dither_cmyk_very_fast, d->v);
	case D_ORDERED:
	  RETURN_DITHERFUNC(stp_dither_cmyk_ordered, d->v);
	case D_EVENTONE:
	  RETURN_DITHERFUNC(stp_dither_cmyk_et, d->v);
	default:
	  RETURN_DITHERFUNC(stp_dither_cmyk_ed, d->v);
	}
      break;
    case OUTPUT_RAW_CMYK:
      d->n_channels = 4;
      d->n_input_channels = 4;
      switch (d->dither_type)
	{
	case D_FAST:
	  RETURN_DITHERFUNC(stp_dither_raw_cmyk_fast, d->v);
	case D_VERY_FAST:
	  RETURN_DITHERFUNC(stp_dither_raw_cmyk_very_fast, d->v);
	case D_ORDERED:
	  RETURN_DITHERFUNC(stp_dither_raw_cmyk_ordered, d->v);
	case D_EVENTONE:
	  RETURN_DITHERFUNC(stp_dither_raw_cmyk_et, d->v);
	default:
	  RETURN_DITHERFUNC(stp_dither_raw_cmyk_ed, d->v);
	}
      break;
    }
  RETURN_DITHERFUNC(NULL, d->v);
}

void *
stp_dither_init(int in_width, int out_width, int image_bpp,
		int xdpi, int ydpi, stp_vars_t v)
{
  int i;
  dither_t *d = stp_zalloc(sizeof(dither_t));
  stp_dither_range_simple_t r;
  d->v = v;
  d->dither_class = stp_get_output_type(v);
  d->error_rows = ERROR_ROWS;
  d->n_ghost_channels = 0;
  d->ditherfunc = stp_set_dither_function(d, image_bpp);

  d->channel = stp_zalloc(d->n_channels * sizeof(dither_channel_t));
  r.value = 1.0;
  r.bit_pattern = 1;
  r.subchannel = 0;
  r.dot_size = 1;
  for (i = 0; i < d->n_channels; i++)
    {
      stp_dither_set_ranges(d, i, 1, &r, 1.0);
      PHYSICAL_CHANNEL(d, i).errs = stp_zalloc(d->error_rows * sizeof(int *));
    }
  d->offset0_table = NULL;
  d->offset1_table = NULL;
  if (xdpi > ydpi)
    {
      d->x_aspect = 1;
      d->y_aspect = xdpi / ydpi;
    }
  else
    {
      d->x_aspect = ydpi / xdpi;
      d->y_aspect = 1;
    }
  d->transition = 1.0;
  d->adaptive_input = .75;
  d->adaptive_input_set = 0;

  if (d->dither_type == D_VERY_FAST)
    stp_dither_set_iterated_matrix(d, 2, DITHER_FAST_STEPS, sq2, 0, 2, 4);
  else
    {
      stp_dither_matrix_t *mat;
      int transposed = 0;
      if (d->y_aspect == d->x_aspect)
	mat = (stp_dither_matrix_t *) &stp_dither_matrix_1_1;
      else if (d->y_aspect > d->x_aspect)
	{
	  transposed = 0;
	  if (d->y_aspect / d->x_aspect == 2)
	    mat = (stp_dither_matrix_t *) &stp_dither_matrix_2_1;
	  else if (d->y_aspect / d->x_aspect == 3)
	    mat = (stp_dither_matrix_t *) &stp_dither_matrix_4_1;
	  else if (d->y_aspect / d->x_aspect == 4)
	    mat = (stp_dither_matrix_t *) &stp_dither_matrix_4_1;
	  else
	    mat = (stp_dither_matrix_t *) &stp_dither_matrix_2_1;
	}
      else
	{
	  transposed = 1;
	  if (d->x_aspect / d->y_aspect == 2)
	    mat = (stp_dither_matrix_t *) &stp_dither_matrix_2_1;
	  else if (d->x_aspect / d->y_aspect == 3)
	    mat = (stp_dither_matrix_t *) &stp_dither_matrix_4_1;
	  else if (d->x_aspect / d->y_aspect == 4)
	    mat = (stp_dither_matrix_t *) &stp_dither_matrix_4_1;
	  else
	    mat = (stp_dither_matrix_t *) &stp_dither_matrix_2_1;
	}
      stp_dither_set_matrix(d, mat, transposed, 0, 0);
    }

  d->src_width = in_width;
  d->dst_width = out_width;

  stp_dither_set_ink_spread(d, 13);
  stp_dither_set_black_lower(d, .4);
  stp_dither_set_black_upper(d, .7);
  for (i = 0; i <= d->n_channels; i++)
    {
      stp_dither_set_black_level(d, i, 1.0);
      stp_dither_set_randomizer(d, i, 1.0);
    }
  stp_dither_set_ink_darkness(d, ECOLOR_C, 2);
  stp_dither_set_ink_darkness(d, ECOLOR_M, 2);
  stp_dither_set_ink_darkness(d, ECOLOR_Y, 1);
  stp_dither_set_density(d, 1.0);
  return d;
}

static void
preinit_matrix(dither_t *d)
{
  int i;
  for (i = 0; i < PHYSICAL_CHANNEL_COUNT(d); i++)
    stp_dither_matrix_destroy(&(PHYSICAL_CHANNEL(d, i).dithermat));
  stp_dither_matrix_destroy(&(d->dither_matrix));
}

static void
postinit_matrix(dither_t *d, int x_shear, int y_shear)
{
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
  stp_dither_set_transition(d, d->transition);
}

void
stp_dither_set_iterated_matrix(void *vd, size_t edge, size_t iterations,
			       const unsigned *data, int prescaled,
			       int x_shear, int y_shear)
{
  dither_t *d = (dither_t *) vd;
  preinit_matrix(d);
  stp_dither_matrix_iterated_init(&(d->dither_matrix), edge, iterations, data);
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
    stp_dither_matrix_init_short(&(d->dither_matrix), x, y,
				 (const unsigned short *) matrix->data,
				 transposed, matrix->prescaled);
  else if (matrix->bytes == 4)
    stp_dither_matrix_init(&(d->dither_matrix), x, y,
			   (const unsigned *)matrix->data,
			   transposed, matrix->prescaled);
  postinit_matrix(d, x_shear, y_shear);
}

void
stp_dither_set_transition(void *vd, double exponent)
{
  dither_t *d = (dither_t *) vd;
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
  d->adaptive_input_set = 1;
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
  SAFE_FREE(d->offset0_table);
  SAFE_FREE(d->offset1_table);
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
  if (i < 0 || i >= PHYSICAL_CHANNEL_COUNT(d))
    return;
  PHYSICAL_CHANNEL(d, i).k_level = (int) v * 64;
}

void
stp_dither_set_randomizer(void *vd, int i, double v)
{
  dither_t *d = (dither_t *) vd;
  if (i < 0 || i >= PHYSICAL_CHANNEL_COUNT(d))
    return;
  PHYSICAL_CHANNEL(d, i).randomizer = v * 65535;
}

void
stp_dither_set_ink_darkness(void *vd, int i, double v)
{
  dither_t *d = (dither_t *) vd;
  if (i < 0 || i >= PHYSICAL_CHANNEL_COUNT(d))
    return;
  PHYSICAL_CHANNEL(d, i).darkness = (int) (v * 64);
}

void
stp_dither_set_light_ink(void *vd, int i, double v, double density)
{
  dither_t *d = (dither_t *) vd;
  stp_dither_range_simple_t range[2];
  if (i < 0 || i >= PHYSICAL_CHANNEL_COUNT(d) || v <= 0 || v > 1)
    return;
  range[0].bit_pattern = 1;
  range[0].subchannel = 1;
  range[0].value = v;
  range[0].dot_size = 1;
  range[1].bit_pattern = 1;
  range[1].subchannel = 0;
  range[1].value = 1;
  range[1].dot_size = 1;
  stp_dither_set_ranges(vd, i, 2, range, density);
}

static void
stp_dither_finalize_ranges(dither_t *d, dither_channel_t *s)
{
  int max_subchannel = 0;
  int i;
  unsigned lbit = s->bit_max;
  s->signif_bits = 0;
  while (lbit > 0)
    {
      s->signif_bits++;
      lbit >>= 1;
    }

  s->maxdot = 0;

  for (i = 0; i < s->nlevels; i++)
    {
      if (s->ranges[i].lower->subchannel > max_subchannel)
	max_subchannel = s->ranges[i].lower->subchannel;
      if (s->ranges[i].upper->subchannel > max_subchannel)
	max_subchannel = s->ranges[i].upper->subchannel;
      if (s->ranges[i].lower->subchannel == s->ranges[i].upper->subchannel &&
	  s->ranges[i].lower->dot_size == s->ranges[i].upper->dot_size)
	s->ranges[i].is_same_ink = 1;
      else
	s->ranges[i].is_same_ink = 0;
      if (s->ranges[i].range_span > 0 &&
	  (s->ranges[i].value_span > 0 ||
	   s->ranges[i].lower->subchannel != s->ranges[i].upper->subchannel))
	s->ranges[i].is_equal = 0;
      else
	s->ranges[i].is_equal = 1;

      if (s->ranges[i].lower->dot_size > s->maxdot)
	s->maxdot = s->ranges[i].lower->dot_size;
      if (s->ranges[i].upper->dot_size > s->maxdot)
	s->maxdot = s->ranges[i].upper->dot_size;

      stp_dprintf(STP_DBG_INK, d->v,
		  "    level %d value[0] %d value[1] %d range[0] %d range[1] %d\n",
		  i, s->ranges[i].lower->value, s->ranges[i].upper->value,
		  s->ranges[i].lower->range, s->ranges[i].upper->range);
      stp_dprintf(STP_DBG_INK, d->v,
		  "       bits[0] %d bits[1] %d subchannel[0] %d subchannel[1] %d\n",
		  s->ranges[i].lower->bits, s->ranges[i].upper->bits,
		  s->ranges[i].lower->subchannel, s->ranges[i].upper->subchannel);
      stp_dprintf(STP_DBG_INK, d->v,
		  "       rangespan %d valuespan %d same_ink %d equal %d\n",
		  s->ranges[i].range_span, s->ranges[i].value_span,
		  s->ranges[i].is_same_ink, s->ranges[i].is_equal);
      if (!d->adaptive_input_set && i > 0 &&
	  s->ranges[i].lower->range >= d->adaptive_limit)
	{
	  d->adaptive_limit = s->ranges[i].lower->range + 1;
	  if (d->adaptive_limit > 65535)
	    d->adaptive_limit = 65535;
	  d->adaptive_input = (double) d->adaptive_limit / (double) d->density;
	  stp_dprintf(STP_DBG_INK, d->v,
		      "Setting adaptive limit to %d, input %f\n",
		      d->adaptive_limit, d->adaptive_input);
	}
    }
  if (s->nlevels == 1 && s->ranges[0].upper->bits == 1 &&
      s->ranges[0].upper->subchannel == 0)
    s->very_fast = 1;
  else
    s->very_fast = 0;
	       
  s->subchannels = max_subchannel + 1;
  s->row_ends[0] = stp_zalloc(s->subchannels * sizeof(int));
  s->row_ends[1] = stp_zalloc(s->subchannels * sizeof(int));
  s->ptrs = stp_zalloc(s->subchannels * sizeof(char *));
  stp_dprintf(STP_DBG_INK, d->v,
	      "  bit_max %d signif_bits %d\n", s->bit_max, s->signif_bits);
}

static void
stp_dither_set_generic_ranges(dither_t *d, dither_channel_t *s, int nlevels,
			      const stp_dither_range_simple_t *ranges,
			      double density)
{
  int i;
  SAFE_FREE(s->ranges);
  SAFE_FREE(s->row_ends[0]);
  SAFE_FREE(s->row_ends[1]);
  SAFE_FREE(s->ptrs);
  SAFE_FREE(s->ink_list);

  s->nlevels = nlevels > 1 ? nlevels + 1 : nlevels;
  s->ranges = (dither_segment_t *)
    stp_zalloc(s->nlevels * sizeof(dither_segment_t));
  s->ink_list = (ink_defn_t *)
    stp_zalloc((s->nlevels + 1) * sizeof(ink_defn_t));
  s->bit_max = 0;
  s->density = density * 65535;
  stp_dprintf(STP_DBG_INK, d->v,
	      "stp_dither_set_generic_ranges nlevels %d density %f\n",
	      nlevels, density);
  for (i = 0; i < nlevels; i++)
    stp_dprintf(STP_DBG_INK, d->v,
		"  level %d value %f pattern %x subchannel %d\n", i,
		ranges[i].value, ranges[i].bit_pattern, ranges[i].subchannel);
  s->ranges[0].lower = &s->ink_list[0];
  s->ranges[0].upper = &s->ink_list[1];
  s->ink_list[0].range = 0;
  s->ink_list[0].value = ranges[0].value * 65535.0;
  s->ink_list[0].bits = ranges[0].bit_pattern;
  s->ink_list[0].subchannel = ranges[0].subchannel;
  s->ink_list[0].dot_size = ranges[0].dot_size;
  if (nlevels == 1)
    s->ink_list[1].range = 65535;
  else
    s->ink_list[1].range = ranges[0].value * 65535.0 * density;
  if (s->ink_list[1].range > 65535)
    s->ink_list[1].range = 65535;
  s->ink_list[1].value = ranges[0].value * 65535.0;
  if (s->ink_list[1].value > 65535)
    s->ink_list[1].value = 65535;
  s->ink_list[1].bits = ranges[0].bit_pattern;
  if (ranges[0].bit_pattern > s->bit_max)
    s->bit_max = ranges[0].bit_pattern;
  s->ink_list[1].subchannel = ranges[0].subchannel;
  s->ink_list[1].dot_size = ranges[0].dot_size;
  s->ranges[0].range_span = s->ranges[0].upper->range;
  s->ranges[0].value_span = 0;
  if (s->nlevels > 1)
    {
      for (i = 1; i < nlevels; i++)
	{
	  int l = i + 1;
	  s->ranges[i].lower = &s->ink_list[i];
	  s->ranges[i].upper = &s->ink_list[l];

	  s->ink_list[l].range =
	    (ranges[i].value + ranges[i].value) * 32768.0 * density;
	  if (s->ink_list[l].range > 65535)
	    s->ink_list[l].range = 65535;
	  s->ink_list[l].value = ranges[i].value * 65535.0;
	  if (s->ink_list[l].value > 65535)
	    s->ink_list[l].value = 65535;
	  s->ink_list[l].bits = ranges[i].bit_pattern;
	  if (ranges[i].bit_pattern > s->bit_max)
	    s->bit_max = ranges[i].bit_pattern;
	  s->ink_list[l].subchannel = ranges[i].subchannel;
	  s->ink_list[l].dot_size = ranges[i].dot_size;
	  s->ranges[i].range_span =
	    s->ink_list[l].range - s->ink_list[i].range;
	  s->ranges[i].value_span =
	    s->ink_list[l].value - s->ink_list[i].value;
	}
      s->ranges[i].lower = &s->ink_list[i];
      s->ranges[i].upper = &s->ink_list[i+1];
      s->ink_list[i+1] = s->ink_list[i];
      s->ink_list[i+1].range = 65535;
      s->ranges[i].range_span = s->ink_list[i+1].range - s->ink_list[i].range;
      s->ranges[i].value_span = s->ink_list[i+1].value - s->ink_list[i].value;
    }
  stp_dither_finalize_ranges(d, s);
}

static void
stp_dither_set_generic_ranges_full(dither_t *d, dither_channel_t *s,
				   int nlevels,
				   const stp_dither_range_full_t *ranges,
				   double density)
{
  int i, j, k;
  SAFE_FREE(s->ranges);
  SAFE_FREE(s->row_ends[0]);
  SAFE_FREE(s->row_ends[1]);
  SAFE_FREE(s->ptrs);
  SAFE_FREE(s->ink_list);

  s->nlevels = nlevels+1;
  s->ranges = (dither_segment_t *)
    stp_zalloc(s->nlevels * sizeof(dither_segment_t));
  s->ink_list = (ink_defn_t *)
    stp_zalloc((s->nlevels * 2) * sizeof(ink_defn_t));
  s->bit_max = 0;
  s->density = density * 65535;
  stp_dprintf(STP_DBG_INK, d->v,
	      "stp_dither_set_ranges nlevels %d density %f\n",
	      nlevels, density);
  for (i = 0; i < nlevels; i++)
    stp_dprintf(STP_DBG_INK, d->v,
		"  level %d value: low %f high %f pattern low %x "
		"high %x subchannel low %d high %d\n", i,
		ranges[i].value[0], ranges[i].value[1],
		ranges[i].bits[0], ranges[i].bits[1],ranges[i].subchannel[0],
		ranges[i].subchannel[1]);
  for(i=j=0; i < nlevels; i++)
    {
      for (k = 0; k < 2; k++)
	{
	  if (ranges[i].bits[k] > s->bit_max)
	    s->bit_max = ranges[i].bits[k];
	  s->ink_list[2*j+k].dot_size = ranges[i].bits[k]; /* FIXME */
	  s->ink_list[2*j+k].value = ranges[i].value[k] * 65535;
	  s->ink_list[2*j+k].range = s->ink_list[2*j+k].value*density;
	  s->ink_list[2*j+k].bits = ranges[i].bits[k];
	  s->ink_list[2*j+k].subchannel = ranges[i].subchannel[k];
	}
      s->ranges[j].lower = &s->ink_list[2*j];
      s->ranges[j].upper = &s->ink_list[2*j+1];
      s->ranges[j].range_span = s->ranges[j].upper->range - s->ranges[j].lower->range;
      s->ranges[j].value_span = s->ranges[j].upper->value - s->ranges[j].lower->value;
      j++;
    }
  s->ink_list[2*j] = s->ink_list[2*(j-1)+1];
  s->ink_list[2*j+1] = s->ink_list[2*j];
  s->ink_list[2*j+1].range = 65535;
  s->ink_list[2*j+1].value = 65535;	/* ??? Is this correct ??? */
  s->ranges[j].lower = &s->ink_list[2*j];
  s->ranges[j].upper = &s->ink_list[2*j+1];
  s->ranges[j].range_span = s->ranges[j].upper->range - s->ranges[j].lower->range;
  s->ranges[j].value_span = 0;
  s->nlevels = j+1;
  stp_dither_finalize_ranges(d, s);
}

void
stp_dither_set_ranges(void *vd, int color, int nlevels,
		      const stp_dither_range_simple_t *ranges, double density)
{
  dither_t *d = (dither_t *) vd;
  if (color < 0 || color >= PHYSICAL_CHANNEL_COUNT(d))
    return;
  stp_dither_set_generic_ranges(d, &(PHYSICAL_CHANNEL(d, color)), nlevels,
				ranges, density);
}

void
stp_dither_set_ranges_simple(void *vd, int color, int nlevels,
			     const double *levels, double density)
{
  stp_dither_range_simple_t *r =
    stp_malloc(nlevels * sizeof(stp_dither_range_simple_t));
  int i;
  for (i = 0; i < nlevels; i++)
    {
      r[i].bit_pattern = i + 1;
      r[i].dot_size = i + 1;
      r[i].value = levels[i];
      r[i].subchannel = 0;
    }
  stp_dither_set_ranges(vd, color, nlevels, r, density);
  stp_free(r);
}

void
stp_dither_set_ranges_full(void *vd, int color, int nlevels,
			   const stp_dither_range_full_t *ranges,
			   double density)
{
  dither_t *d = (dither_t *) vd;
  stp_dither_set_generic_ranges_full(d, &(PHYSICAL_CHANNEL(d, color)), nlevels,
				     ranges, density);
}

void
stp_dither_free(void *vd)
{
  dither_t *d = (dither_t *) vd;
  int i;
  int j;
  for (j = 0; j < PHYSICAL_CHANNEL_COUNT(d); j++)
    {
      SAFE_FREE(PHYSICAL_CHANNEL(d, j).vals);
      SAFE_FREE(PHYSICAL_CHANNEL(d, j).row_ends[0]);
      SAFE_FREE(PHYSICAL_CHANNEL(d, j).row_ends[1]);
      SAFE_FREE(PHYSICAL_CHANNEL(d, j).ptrs);
      SAFE_FREE(PHYSICAL_CHANNEL(d, j).ink_list);
      if (PHYSICAL_CHANNEL(d, j).errs)
	{
	  for (i = 0; i < d->error_rows; i++)
	    SAFE_FREE(PHYSICAL_CHANNEL(d, j).errs[i]);
	  SAFE_FREE(PHYSICAL_CHANNEL(d, j).errs);
	}
      SAFE_FREE(PHYSICAL_CHANNEL(d, j).ranges);
      stp_dither_matrix_destroy(&(PHYSICAL_CHANNEL(d, j).pick));
      stp_dither_matrix_destroy(&(PHYSICAL_CHANNEL(d, j).dithermat));
    }
  SAFE_FREE(d->offset0_table);
  SAFE_FREE(d->offset1_table);
  stp_dither_matrix_destroy(&(d->dither_matrix));
  stp_dither_matrix_destroy(&(d->transition_matrix));
  if (d->eventone) {
    eventone_t *et = d->eventone;
    stp_free(et->recip);
    for (i=0; i<PHYSICAL_CHANNEL_COUNT(d); i++) {
      stp_free(et->dis[i]);
    }
    stp_free(et->dis);
    stp_free(d->eventone);
  }
  stp_free(d->channel);
  stp_free(d);
}

int
stp_dither_get_first_position(void *vd, int color, int subchannel)
{
  dither_t *d = (dither_t *) vd;
  if (color < 0 || color >= PHYSICAL_CHANNEL_COUNT(d))
    return -1;
  if (subchannel < 0 || subchannel >= PHYSICAL_CHANNEL(d, color).subchannels)
    return -1;
  return PHYSICAL_CHANNEL(d, color).row_ends[0][subchannel];
}

int
stp_dither_get_last_position(void *vd, int color, int subchannel)
{
  dither_t *d = (dither_t *) vd;
  if (color < 0 || color >= PHYSICAL_CHANNEL_COUNT(d))
    return -1;
  if (subchannel < 0 || subchannel >= PHYSICAL_CHANNEL(d, color).subchannels)
    return -1;
  return PHYSICAL_CHANNEL(d, color).row_ends[0][subchannel];
}

static int *
get_errline(dither_t *d, int row, int color)
{
  if (row < 0 || color < 0 || color >= CHANNEL_COUNT(d))
    return NULL;
  if (CHANNEL(d, color).errs[row & 1])
    return CHANNEL(d, color).errs[row & 1] + MAX_SPREAD;
  else
    {
      int size = 2 * MAX_SPREAD + (16 * ((d->dst_width + 7) / 8));
      CHANNEL(d, color).errs[row & 1] = stp_zalloc(size * sizeof(int));
      return CHANNEL(d, color).errs[row & 1] + MAX_SPREAD;
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

#define ADVANCE_REVERSE(d, bit, input, width, xerror, xmod)	\
do								\
{								\
  if (bit == 128)						\
    {								\
      d->ptr_offset--;						\
      bit = 1;							\
    }								\
  else								\
    bit <<= 1;							\
  if (d->src_width == d->dst_width)				\
    input -= (width);						\
  else								\
    {								\
      input -= xstep;						\
      xerror -= xmod;						\
      if (xerror < 0)						\
	{							\
	  xerror += d->dst_width;				\
	  input -= (width);					\
	}							\
    }								\
} while (0)

#define ADVANCE_BIDIRECTIONAL(d, bit, in, dir, width, xer, xmod, err, N, S) \
do									    \
{									    \
  int i;								    \
  int j;								    \
  for (i = 0; i < N; i++)						    \
    for (j = 0; j < S; j++)						    \
      err[i][j] += dir;							    \
  if (dir == 1)								    \
    ADVANCE_UNIDIRECTIONAL(d, bit, in, width, xer, xmod);		    \
  else									    \
    ADVANCE_REVERSE(d, bit, in, width, xer, xmod);			    \
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
  for (i = 1; i < CHANNEL_COUNT(d); i++)
    answer = USMIN(answer, CHANNEL(d, i).v);
  return answer;
}

static inline void
set_row_ends(dither_channel_t *dc, int x, int subchannel)
{
  if (dc->row_ends[0][subchannel] == -1)
    dc->row_ends[0][subchannel] = x;
  dc->row_ends[1][subchannel] = x;
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
  unsigned randomizer = dc->randomizer;
  dither_matrix_t *pick_matrix = &(dc->pick);
  dither_matrix_t *dither_matrix = &(dc->dithermat);
  unsigned rangepoint = 32768;
  unsigned virtual_value;
  unsigned vmatrix;
  int i;
  int j;
  int subchannel;
  unsigned char *tptr;
  unsigned bits;
  unsigned v;
  unsigned dot_size;
  int levels = dc->nlevels - 1;
  int dither_value = adjusted;
  dither_segment_t *dd;
  ink_defn_t *lower;
  ink_defn_t *upper;

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
      dd = &(dc->ranges[i]);

      if (density <= dd->lower->range)
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
       
      lower = dd->lower;
      upper = dd->upper;
      
      if (!dd->is_equal)
	rangepoint =
	  ((unsigned) (density - lower->range)) * 65535 / dd->range_span;

      /*
       * Compute the virtual dot size that we're going to print.
       * This is somewhere between the two candidate dot sizes.
       * This is scaled between the high and low value.
       */

      if (dd->value_span == 0)
	virtual_value = upper->value;
      else if (dd->range_span == 0)
	virtual_value = (upper->value + lower->value) / 2;
      else
	virtual_value = lower->value +
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
	  ink_defn_t *subc;

	  if (dd->is_same_ink)
	    subc = upper;
	  else
	    {
	      rangepoint = rangepoint * dc->density / 65535u;
	      if (rangepoint >= ditherpoint(d, pick_matrix, x))
		subc = upper;
	      else
		subc = lower;
	    }
	  subchannel = subc->subchannel;
	  bits = subc->bits;
	  v = subc->value;
	  dot_size = subc->dot_size;
	  tptr = dc->ptrs[subchannel] + d->ptr_offset;

	  /*
	   * Lay down all of the bits in the pixel.
	   */
	  if (dontprint < v)
	    {
	      set_row_ends(dc, x, subchannel);
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
  dither_matrix_t *pick_matrix = &(dc->pick);
  dither_matrix_t *dither_matrix = &(dc->dithermat);
  unsigned rangepoint;
  unsigned virtual_value;
  unsigned vmatrix;
  int i;
  int j;
  int subchannel;
  unsigned char *tptr;
  unsigned bits;
  unsigned v;
  unsigned dot_size;
  int levels = dc->nlevels - 1;
  int dither_value = adjusted;
  dither_segment_t *dd;
  ink_defn_t *lower;
  ink_defn_t *upper;

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
      dd = &(dc->ranges[i]);

      if (density <= dd->lower->range)
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

      lower = dd->lower;
      upper = dd->upper;

      if (dd->is_equal)
	rangepoint = 32768;
      else
	rangepoint =
	  ((unsigned) (density - lower->range)) * 65535 / dd->range_span;

      /*
       * Compute the virtual dot size that we're going to print.
       * This is somewhere between the two candidate dot sizes.
       * This is scaled between the high and low value.
       */

      if (dd->value_span == 0)
	virtual_value = upper->value;
      else if (dd->range_span == 0)
	virtual_value = (upper->value + lower->value) / 2;
      else
	virtual_value = lower->value +
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
	  ink_defn_t *subc;

	  if (dd->is_same_ink)
	    subc = upper;
	  else
	    {
	      rangepoint = rangepoint * dc->density / 65535u;
	      if (rangepoint >= ditherpoint(d, pick_matrix, x))
		subc = upper;
	      else
		subc = lower;
	    }
	  subchannel = subc->subchannel;
	  bits = subc->bits;
	  v = subc->value;
	  dot_size = subc->dot_size;
	  tptr = dc->ptrs[subchannel] + d->ptr_offset;

	  /*
	   * Lay down all of the bits in the pixel.
	   */
	  if (dontprint < v)
	    {
	      set_row_ends(dc, x, subchannel);
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
  dither_matrix_t *dither_matrix = &(dc->dithermat);
  int i;
  int levels = dc->nlevels - 1;
  int j;
  unsigned char *tptr;
  unsigned bits;

  if (density <= 0 || adjusted <= 0)
    return;
  for (i = levels; i >= 0; i--)
    {
      dither_segment_t *dd = &(dc->ranges[i]);
      unsigned vmatrix;
      unsigned rangepoint;
      unsigned dpoint;
      unsigned range0;
      ink_defn_t *subc;

      range0 = dd->lower->range;
      if (density <= range0)
	continue;
      dpoint = ditherpoint(d, dither_matrix, x);

      if (dd->is_same_ink)
	subc = dd->upper;
      else
	{
	  rangepoint = ((density - range0) << 16) / dd->range_span;
	  rangepoint = (rangepoint * dc->density) >> 16;
	  if (rangepoint >= dpoint)
	    subc = dd->upper;
	  else
	    subc = dd->lower;
	}
      vmatrix = (subc->value * dpoint) >> 16;

      /*
       * After all that, printing is almost an afterthought.
       * Pick the actual dot size (using a matrix here) and print it.
       */
      if (adjusted >= vmatrix)
	{
	  int subchannel = subc->subchannel;
	  bits = subc->bits;
	  tptr = dc->ptrs[subchannel] + d->ptr_offset;
	  set_row_ends(dc, x, subchannel);

	  /*
	   * Lay down all of the bits in the pixel.
	   */
	  for (j = 1; j <= bits; j += j, tptr += length)
	    {
	      if (j & bits)
		tptr[0] |= bit;
	    }
	}
      return;
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

  for (i = 1; i < CHANNEL_COUNT(d); i++)
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

      for (i = 1; i < CHANNEL_COUNT(d); i++)
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
    for (i = 1; i < CHANNEL_COUNT(d); i++)
      CHANNEL(d, i).v = CHANNEL(d, i).o;
  CHANNEL(d, ECOLOR_K).b = bk;
  CHANNEL(d, ECOLOR_K).v = k;
}

static int
shared_ed_initializer(dither_t *d,
		      int row,
		      int duplicate_line,
		      int zero_mask,
		      int length,
		      int direction,
		      int ****error,
		      int **ndither)
{
  int i, j;
  if (!duplicate_line)
    {
      if ((zero_mask & ((1 << d->n_input_channels) - 1)) !=
	  ((1 << d->n_input_channels) - 1))
	d->last_line_was_empty = 0;
      else
	d->last_line_was_empty++;
    }
  else if (d->last_line_was_empty)
    d->last_line_was_empty++;
  if (d->last_line_was_empty >= 5)
    return 0;
  else if (d->last_line_was_empty == 4)
    {
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	for (j = 0; j < d->error_rows; j++)
	  memset(get_errline(d, row + j, i), 0, d->dst_width * sizeof(int));
      return 0;
    }
  d->ptr_offset = (direction == 1) ? 0 : length - 1;

  *error = stp_malloc(CHANNEL_COUNT(d) * sizeof(int **));
  *ndither = stp_malloc(CHANNEL_COUNT(d) * sizeof(int));
  for (i = 0; i < CHANNEL_COUNT(d); i++)
    {
      (*error)[i] = stp_malloc(d->error_rows * sizeof(int *));
      for (j = 0; j < d->error_rows; j++)
	{
	  (*error)[i][j] = get_errline(d, row + j, i);
	  if (j == d->error_rows - 1)
	    memset((*error)[i][j], 0, d->dst_width * sizeof(int));
	  if (direction == -1)
	    (*error)[i][j] += d->dst_width - 1;
	}
      (*ndither)[i] = (*error)[i][0][0];
    }
  return 1;
}

static void
shared_ed_deinitializer(dither_t *d,
			int ***error,
			int *ndither)
{
  int i;
  for (i = 0; i < CHANNEL_COUNT(d); i++)
    {
      SAFE_FREE(error[i]);
    }
  SAFE_FREE(error);
  SAFE_FREE(ndither);
}

static inline void find_segment(dither_t *d, dither_channel_t *dc, int wetness, int density, dither_segment_t *range)
{
	int i;
	ink_defn_t *di;
	int max_dot;
	
	if (wetness < 0) max_dot = 0;
	else max_dot = wetness >> 16;

	range->lower = range->upper = dc->ranges[0].lower;

	for (i = dc->nlevels-1; i > 0; i--) {
		di = dc->ranges[i].lower;
		if (density < di->value) continue;
		if (max_dot < di->dot_size) continue;
		range->lower = di;
		range->upper = di;
		break;
	}

	for (; i < dc->nlevels; i++) {
		di = dc->ranges[i].upper;
		if (max_dot < di->dot_size) continue;
		range->upper = di;
		if (density < di->value) break;
	}
}

#define EVEN_C1 256
#define EVEN_C2 222		/* = sqrt(3)/2 * EVEN_C1 */

static void
eventone_init(dither_t *d, et_chdata_t **cd)
{
  int i;
  eventone_t *et = d->eventone;

  if (!et) {

    et = stp_zalloc(sizeof(eventone_t));

    { int xa, ya;
      xa = d->x_aspect / d->y_aspect;
      if (xa == 0) xa = 1;
      et->d_sq.dx = xa * xa;
      et->d2x = 2 * et->d_sq.dx;
  
      ya = d->y_aspect / d->x_aspect;
      if (ya == 0) ya = 1;
      et->d_sq.dy = ya * ya;
      et->d2y = 2 * et->d_sq.dy;
    
      et->aspect = EVEN_C2 / (xa * ya);
      et->d_sq.r_sq = 0;
    }
  
    et->recip = stp_malloc(65536 * sizeof(int));
    et->dis = stp_malloc(sizeof(dis_t *) * PHYSICAL_CHANNEL_COUNT(d));
  
    for (i=0; i < PHYSICAL_CHANNEL_COUNT(d); i++) {
      int x;
      et->dis[i] = stp_malloc(sizeof(dis_t) * d->dst_width);
      for (x = 0; x < d->dst_width; x++) {
	et->dis[i][x] = et->d_sq;
      }
    }

    for (i=0; i < 65536; i++) {
      if (i == 0)
        et->recip[i] = EVEN_C1 * 65536;
      else
        et->recip[i] = EVEN_C1 * 65536 / i;
    }

    for (i = 0; i < PHYSICAL_CHANNEL_COUNT(d); i++) {
      PHYSICAL_CHANNEL(d, i).ranges[0].lower->value = 0;
      PHYSICAL_CHANNEL(d, i).ranges[0].lower->range = 0;
      PHYSICAL_CHANNEL(d, i).ranges[0].lower->bits = 0;
      PHYSICAL_CHANNEL(d, i).ranges[0].lower->subchannel = 0;
      PHYSICAL_CHANNEL(d, i).ranges[0].lower->dot_size = 0;
    }

    d->eventone = et;
  }

  { et_chdata_t *p;
    *cd = stp_malloc(sizeof(et_chdata_t) * PHYSICAL_CHANNEL_COUNT(d));

    for (i = 0, p = *cd; i < PHYSICAL_CHANNEL_COUNT(d); i++, p++)
    {
      p->wetness = 0;
      p->maxdot_dens = PHYSICAL_CHANNEL(d, i).maxdot * d->density;
      p->maxdot_wet = (65536 + d->density / 2) * PHYSICAL_CHANNEL(d, i).maxdot;
      p->dis = et->d_sq;
    }
  }
}

static inline void
advance_eventone_pre(dis_t *cdd, eventone_t *et, dis_t *etd)
{
  int t = cdd->r_sq + cdd->dx;
  if (t <= etd->r_sq) { 				/* Do eventone calculations */
    cdd->r_sq = t;					/* Nearest pixel same as last one */
    cdd->dx += et->d2x;
  } else {
    *cdd = *etd;					/* Nearest pixel is from a previous line */
  }
}
		

static inline void
advance_eventone_post(et_chdata_t *cd, eventone_t *et, dis_t *etd)
{
  int t, u;

  if (cd->point > 0) {				/* If something was printed, */
    cd->dis = et->d_sq;				/* Reset dx, dy counters (structure copy) */
  }

  t = etd->r_sq + etd->dy;			/* r^2 from dot above */
  u = cd->dis.r_sq + cd->dis.dy;		/* r^2 from dot on this line */
  if (u < t) {					/* If dot from this line is closer */
    t = u;					/* Use it instead */
    etd->dx = cd->dis.dx;
    etd->dy = cd->dis.dy;
  }
  etd->dy += et->d2y;

  if (t > 65535) {				/* Do some hard limiting */
    t = 65535;
  }
  etd->r_sq = t;
}

static inline int
eventone_adjust(dither_segment_t *range, eventone_t *et, int r_sq, int base, int value)
{
  unsigned upper;
  unsigned lower;
  unsigned value_span;
  int ditherpoint;
	
  lower = range->lower->value;
  upper = range->upper->value;
  value_span = upper - lower;

  if (value >= upper) {
    ditherpoint = 65535;
  } else {
    if (value <= lower) {
      ditherpoint = 0;
    } else {
      ditherpoint = ((unsigned)(value - lower) << 16) / value_span;
    }
    /* Adjust for Eventone here */
    if (lower == 0) {
      ditherpoint += r_sq * et->aspect;
      if (base < upper) {
	ditherpoint -= et->recip[(base<<16) / value_span];
      }
      if (ditherpoint > 65535) ditherpoint = 65535;
      else if (ditherpoint < 0) ditherpoint = 0;
    }
  }
  return ditherpoint;
}

static inline void
print_all_inks(dither_t *d, et_chdata_t *cd, int print_inks, unsigned pick, unsigned char bit, int length)
{
  int i, mask;
  for (i = 0, mask = 1; i < CHANNEL_COUNT(d); mask <<= 1, cd++, i++) {
    int j;
    ink_defn_t *subc;
    int bits;
    unsigned char *tptr;
    
    if (!(print_inks & mask)) continue;
    subc = (pick & mask) ? cd->dr.upper : cd->dr.lower;
    if (!(CHANNEL(d, i).ptrs[subc->subchannel]))
      continue;

    bits = subc->bits;
    if (bits == 0) continue;

    tptr = CHANNEL(d, i).ptrs[subc->subchannel] + d->ptr_offset;
    cd->wetness += subc->dot_size << 16;
    
    if (bits == 1) {
      *tptr |= bit;
    } else {
      for (j=1; j <= bits; j+=j, tptr += length) {
        if (j & bits) *tptr |= bit;
      }
    }
  }
}

static inline void
diffuse_error(dither_t *d, int *ndither, int ***error, int aspect, int direction)
{
  int i;
  int fraction, frac_2, frac_3;
  int *err;
  static const int diff_fact[] = {1, 10, 16, 23, 32};
  int factor = diff_fact[aspect];
  
  for (i=0; i < CHANNEL_COUNT(d); i++, ndither++, error++) {
    fraction = (*ndither + (factor>>1)) / factor;
    frac_2 = fraction + fraction;
    frac_3 = frac_2 + fraction;
    err = (*error)[1];
    err[0] += frac_3;
    err[-direction] += frac_2;
    *ndither += (*error)[0][direction] - frac_2 - frac_3;
  }
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
		      dither_t 		    *d,
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
  dither_channel_t *dc = &(CHANNEL(d, ECOLOR_K));
  dither_matrix_t *kdither = &(dc->dithermat);
  unsigned bits = dc->signif_bits;
  int j;
  unsigned char *tptr;
  int dst_width = d->dst_width;
  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
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
	  set_row_ends(dc, x, 0);
	  for (j = 0; j < bits; j++, tptr += length)
	    tptr[0] |= bit;
	}
      ADVANCE_UNIDIRECTIONAL(d, bit, gray, 1, xerror, xmod);
    }
}

static void
stp_dither_monochrome_very_fast(const unsigned short  *gray,
				int           	    row,
				dither_t 		    *d,
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
  dither_channel_t *dc = &(CHANNEL(d, ECOLOR_K));
  dither_matrix_t *kdither = &(dc->dithermat);
  int dst_width = d->dst_width;
  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
    return;
  if (!dc->very_fast)
    {
      stp_dither_monochrome(gray, row, d, duplicate_line, zero_mask);
      return;
    }

  kptr = CHANNEL(d, ECOLOR_K).ptrs[0];
  length = (d->dst_width + 7) / 8;

  bit = 128;
  x = 0;

  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  for (x = 0; x < dst_width; x++)
    {
      if (gray[0] && (d->density > ditherpoint_fast(d, kdither, x)))
	{
	  set_row_ends(dc, x, 0);
	  kptr[d->ptr_offset] |= bit;
	}
      ADVANCE_UNIDIRECTIONAL(d, bit, gray, 1, xerror, xmod);
    }
}

static void
stp_dither_cmyk_fast(const unsigned short  *cmy,
		     int           row,
		     dither_t 	    *d,
		     int	       duplicate_line,
		     int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  int i;

  int dst_width = d->dst_width;
  int xerror, xstep, xmod;

  if (d->n_ghost_channels)
    {
      stp_dither_raw_fast(cmy, row, d, duplicate_line, zero_mask);
      return;
    }

  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
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
	  if (k < lb)
	    k = 0;
	  else if (k < ub)
	    k = (k - lb) * ub / d->bound_range;
	  for (i = 1; i < CHANNEL_COUNT(d); i++)
	    CHANNEL(d, i).v -= k;
	  ok = k;
	  if (ok > 0 && d->density != d->black_density)
	    ok = (unsigned) ok * (unsigned) d->black_density / d->density;
	  if (ok > 65535)
	    ok = 65535;
	  QUANT(15);
	  CHANNEL(d, ECOLOR_K).v = k;
	  CHANNEL(d, ECOLOR_K).o = ok;

	  for (i = 0; i < CHANNEL_COUNT(d); i++)
	    print_color_fast(d, &(CHANNEL(d, i)), x, row, bit, length);
	  QUANT(16);
	}
      ADVANCE_UNIDIRECTIONAL(d, bit, cmy, 3, xerror, xmod);
      QUANT(17);
    }
}

static void
stp_dither_cmyk_very_fast(const unsigned short  *cmy,
			  int           row,
			  dither_t 	    *d,
			  int	       duplicate_line,
			  int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  int i;

  int dst_width = d->dst_width;
  int xerror, xstep, xmod;

  if (d->n_ghost_channels)
    {
      stp_dither_raw_very_fast(cmy, row, d, duplicate_line, zero_mask);
      return;
    }

  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
    return;

  for (i = 0; i < CHANNEL_COUNT(d); i++)
    if (!(CHANNEL(d, i).very_fast))
      {
	stp_dither_cmyk_fast(cmy, row, d, duplicate_line, zero_mask);
	return;
      }

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

      if (nonzero)
	{
	  int k = compute_black(d);
	  for (i = 1; i < CHANNEL_COUNT(d); i++)
	    CHANNEL(d, i).v -= k;
	  QUANT(15);
	  CHANNEL(d, ECOLOR_K).v = k;

	  for (i = 0; i < CHANNEL_COUNT(d); i++)
	    {
	      dither_channel_t *dc = &(CHANNEL(d, i));
	      if (dc->v > ditherpoint_fast(d, &(dc->dithermat), x))
		{
		  set_row_ends(dc, x, 0);
		  dc->ptrs[0][d->ptr_offset] |= bit;
		}
	    }
	  QUANT(16);
	}
      ADVANCE_UNIDIRECTIONAL(d, bit, cmy, 3, xerror, xmod);
      QUANT(17);
    }
}

static void
stp_dither_cmyk_ordered(const unsigned short  *cmy,
			int           row,
			dither_t 	    *d,
			int		  duplicate_line,
			int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  int i;

  int		terminate;
  int xerror, xstep, xmod;

  if (d->n_ghost_channels)
    {
      stp_dither_raw_ordered(cmy, row, d, duplicate_line, zero_mask);
      return;
    }

  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
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
      for (i = 0; i < CHANNEL_COUNT(d); i++)
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

	  for (i = 0; i < CHANNEL_COUNT(d); i++)
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
		   dither_t 	    *d,
		   int		  duplicate_line,
		   int		  zero_mask)
{
  int		x,
	        length;
  unsigned char	bit;
  int		i;
  int		*ndither;
  int		***error;

  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int xerror, xstep, xmod;

  if (d->n_ghost_channels)
    {
      stp_dither_raw_ed(cmy, row, d, duplicate_line, zero_mask);
      return;
    }

  length = (d->dst_width + 7) / 8;
  if (!shared_ed_initializer(d, row, duplicate_line, zero_mask, length,
			     direction, &error, &ndither))
    return;

  x = (direction == 1) ? 0 : d->dst_width - 1;
  bit = 1 << (7 - (x & 7));
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;
  terminate = (direction == 1) ? d->dst_width : -1;

  if (direction == -1)
    cmy += (3 * (d->src_width - 1));

  QUANT(6);
  for (; x != terminate; x += direction)
    {
      int nonzero = 0;
      int printed_black = 0;
      CHANNEL(d, ECOLOR_C).v = cmy[0];
      CHANNEL(d, ECOLOR_M).v = cmy[1];
      CHANNEL(d, ECOLOR_Y).v = cmy[2];
      for (i = 0; i < CHANNEL_COUNT(d); i++)
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

	  for (i = 1; i < CHANNEL_COUNT(d); i++)
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

	  for (i = 0; i < CHANNEL_COUNT(d); i++)
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
	for (i = 0; i < CHANNEL_COUNT(d); i++)
	  CHANNEL(d, i).v = UPDATE_COLOR(CHANNEL(d, i).v, ndither[i]);

      QUANT(11);
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	ndither[i] = update_dither(d, i, d->src_width,
				   direction, error[i][0], error[i][1]);

      QUANT(12);
      ADVANCE_BIDIRECTIONAL(d, bit, cmy, direction, 3, xerror, xmod, error,
			    CHANNEL_COUNT(d), d->error_rows);
      QUANT(13);
    }
  shared_ed_deinitializer(d, error, ndither);
  if (direction == -1)
    reverse_row_ends(d);
}

/* This code uses the Eventone dither algorithm. This is described
 * at the website http://www.artofcode.com/eventone/
 * This algorithm is covered by US Patents 5,055,942 and 5,917,614
 * and was invented by Raph Levien <raph@acm.org>
 * It was made available to be used free of charge in open source
 * code.
 */

static void
stp_dither_cmyk_et(const unsigned short  *cmy,
		   int           row,
		   dither_t 	 *d,
		   int		 duplicate_line,
		   int		 zero_mask)
{
  int		x,
	        length;
  unsigned char	bit;
  int		i;
  int		*ndither;
  eventone_t	*et;
  et_chdata_t	*cd;

  int		***error;
  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int		xerror, xstep, xmod;
  int		aspect = d->y_aspect / d->x_aspect;
  
  if (d->n_ghost_channels)
    {
      stp_dither_raw_et(cmy, row, d, duplicate_line, zero_mask);
      return;
    }

  if (aspect >= 4) { aspect = 4; }
  else if (aspect >= 2) { aspect = 2; }
  else aspect = 1;

  length = (d->dst_width + 7) / 8;
  if (!shared_ed_initializer(d, row, duplicate_line, zero_mask, length,
			     direction, &error, &ndither))
    return;

  eventone_init(d, &cd);
  et = d->eventone;

  x = (direction == 1) ? 0 : d->dst_width - 1;
  bit = 1 << (7 - (x & 7));
  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;
  terminate = (direction == 1) ? d->dst_width : -1;
  if (direction == -1) {
    cmy += (3 * (d->src_width - 1));
  }

  QUANT(6);
  for (; x != terminate; x += direction)
    { unsigned pick = 0, print_inks;
      int range = 0;
      unsigned channel_mask;
      et_chdata_t *p;
      
      CHANNEL(d, ECOLOR_K).b = 0;

      for (i=1; i < CHANNEL_COUNT(d); i++) {
        int value = cmy[i-1];

	CHANNEL(d, i).o = value;				/* Remember value we want printed here */
	CHANNEL(d, i).v = value;
	if (i == 1 || value < CHANNEL(d, ECOLOR_K).o)
	    CHANNEL(d, ECOLOR_K).o = value;			/* Set black to minimum of C,M,Y */
      }

      CHANNEL(d, ECOLOR_K).v = CHANNEL(d, ECOLOR_K).o;
      if (CHANNEL(d, ECOLOR_K).v > 0) {
        update_cmyk(d);
      }
	
      for (i = 1; i < CHANNEL_COUNT(d); i++)
	CHANNEL(d, i).b = CHANNEL(d, i).v;

      for (i=0, channel_mask = 1, p = cd; i < CHANNEL_COUNT(d); i++, channel_mask <<= 1, p++) {
        int base;
        int value;
	int maxwet;
	dis_t *etd = &et->dis[i][x];

	advance_eventone_pre(&p->dis, et, etd);

	if ((p->wetness -= p->maxdot_dens) < 0) p->wetness = 0;

	base = CHANNEL(d, i).b;
	value = ndither[i] + base;
	if (value < 0) value = 0;				/* Dither can make this value negative */

        maxwet = (CHANNEL(d, i).b * CHANNEL(d, i).maxdot >> 1)
	 + p->maxdot_wet - p->wetness;
	
        find_segment(d, &CHANNEL(d, i), maxwet, value, &p->dr);
	
	range += eventone_adjust(&p->dr, et, p->dis.r_sq, base, value);
	if (range >= 32768) {
	  pick |= channel_mask;
	  range -= 65536;
	  value = p->dr.upper->value;
	} else {
	  value = p->dr.lower->value;
	}
        /* Adjust error values for dither */
	ndither[i] += 2 * (base - value);
	p->point = value;
	advance_eventone_post(p, et, etd);
      }

      { int printed_black;
	int adjusted_black;

	print_inks = (1 << ECOLOR_C)|(1 << ECOLOR_M)|(1<<ECOLOR_Y);

        printed_black = cd[ECOLOR_K].point;
	adjusted_black = printed_black;
	if (printed_black > 0 ) {
	  if (d->black_density != d->density) {
	    adjusted_black = (unsigned)printed_black * (unsigned)d->density / d->black_density;
	  }

          /* Only print the black ink if it means we can avoid printing another ink, otherwise we're just wasting ink */

	  for (i=1; i < CHANNEL_COUNT(d); i++) {
            if (cd[i].point <= adjusted_black) {
	      print_inks = (1 << ECOLOR_K);
	      for (i=1; i < d->n_channels; i++) {
	        if (cd[i].point > adjusted_black) {
		  /* Find which channels we actually print */
	          print_inks |= (1 << i);
	        }
	      }
	      break;
	    }
          }
	}
      }

      /* Now we can finally print it! */
      
      print_all_inks(d, cd, print_inks, pick, bit, length);

      QUANT(11);
  
      /* Diffuse the error round a bit */
      diffuse_error(d, ndither, error, aspect, direction);

      QUANT(12);
      ADVANCE_BIDIRECTIONAL(d, bit, cmy, direction, 3, xerror, xmod, error,
			    CHANNEL_COUNT(d), d->error_rows);
      QUANT(13);
    }

    stp_free(cd);
    shared_ed_deinitializer(d, error, ndither);
}

static void
stp_dither_raw_cmyk_fast(const unsigned short  *cmyk,
			 int           row,
			 dither_t 	    *d,
			 int	       duplicate_line,
			 int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  int i;

  int dst_width = d->dst_width;
  int xerror, xstep, xmod;
  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
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
      for (i = 0; i < CHANNEL_COUNT(d); i++)
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
stp_dither_raw_cmyk_very_fast(const unsigned short  *cmyk,
			      int           row,
			      dither_t 	    *d,
			      int	       duplicate_line,
			      int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  int i;

  int dst_width = d->dst_width;
  int xerror, xstep, xmod;
  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
    return;

  for (i = 0; i < CHANNEL_COUNT(d); i++)
    if (!(CHANNEL(d, i).very_fast))
      {
	stp_dither_raw_cmyk_fast(cmyk, row, d, duplicate_line, zero_mask);
	return;
      }

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
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	{
	  dither_channel_t *dc = &(CHANNEL(d, i));
	  if (dc->ptrs[0] && dc->v > ditherpoint_fast(d, &(dc->dithermat), x))
	    {
	      set_row_ends(dc, x, 0);
	      dc->ptrs[0][d->ptr_offset] |= bit;
	    }
	}
      QUANT(16);
      ADVANCE_UNIDIRECTIONAL(d, bit, cmyk, 4, xerror, xmod);
      QUANT(17);
    }
}

static void
stp_dither_raw_cmyk_ordered(const unsigned short  *cmyk,
			    int           row,
			    dither_t 	    *d,
			    int		  duplicate_line,
			    int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  int i;

  int		terminate;
  int xerror, xstep, xmod;

  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
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
      for (i = 0; i < CHANNEL_COUNT(d); i++)
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
		       dither_t 	    *d,
		       int		  duplicate_line,
		       int		  zero_mask)
{
  int		x,
    		length;
  unsigned char	bit;
  int		i;
  int		*ndither;
  int		***error;

  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int xerror, xstep, xmod;

  length = (d->dst_width + 7) / 8;
  if (!shared_ed_initializer(d, row, duplicate_line, zero_mask, length,
			     direction, &error, &ndither))
    return;

  x = (direction == 1) ? 0 : d->dst_width - 1;
  bit = 1 << (7 - (x & 7));
  xstep  = 4 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;
  terminate = (direction == 1) ? d->dst_width : -1;

  if (direction == -1)
    cmyk += (4 * (d->src_width - 1));

  QUANT(6);
  for (; x != terminate; x += direction)
    {
      int extra_k;
      CHANNEL(d, ECOLOR_K).v = cmyk[3];
      CHANNEL(d, ECOLOR_C).v = cmyk[0];
      CHANNEL(d, ECOLOR_M).v = cmyk[1];
      CHANNEL(d, ECOLOR_Y).v = cmyk[2];
      extra_k = compute_black(d) + CHANNEL(d, ECOLOR_K).v;
      for (i = 0; i < CHANNEL_COUNT(d); i++)
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
			    CHANNEL_COUNT(d), d->error_rows);
      QUANT(13);
    }
  shared_ed_deinitializer(d, error, ndither);
  if (direction == -1)
    reverse_row_ends(d);
}

static void
stp_dither_raw_cmyk_et(const unsigned short  *cmyk,
		   int           row,
		   dither_t 	 *d,
		   int		 duplicate_line,
		   int		 zero_mask)
{
  int		x,
	        length;
  unsigned char	bit;
  int		i;
  int		*ndither;
  eventone_t	*et;
  et_chdata_t	*cd;

  int		***error;
  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int		xerror, xstep, xmod;
  int		aspect = d->y_aspect / d->x_aspect;
  
  if (aspect >= 4) { aspect = 4; }
  else if (aspect >= 2) { aspect = 2; }
  else aspect = 1;

  length = (d->dst_width + 7) / 8;
  if (!shared_ed_initializer(d, row, duplicate_line, zero_mask, length,
			     direction, &error, &ndither))
    return;

  eventone_init(d, &cd);
  et = d->eventone;

  x = (direction == 1) ? 0 : d->dst_width - 1;
  bit = 1 << (7 - (x & 7));
  xstep  = 4 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;
  terminate = (direction == 1) ? d->dst_width : -1;
  if (direction == -1) {
    cmyk += (4 * (d->src_width - 1));
  }

  QUANT(6);
  for (; x != terminate; x += direction)
    { unsigned pick = 0, print_inks;
      int range = 0;
      unsigned channel_mask;
      et_chdata_t *p;
      
      { int value = cmyk[3];		/* Order of input is C,M,Y,K */
	CHANNEL(d, ECOLOR_K).o = value;				/* Remember value we want printed here */
	CHANNEL(d, ECOLOR_K).v = value;
	CHANNEL(d, ECOLOR_K).b = value;
      }
      
      for (i=1; i < CHANNEL_COUNT(d); i++) {
        int value = cmyk[i-1];
	CHANNEL(d, i).o = value;				/* Remember value we want printed here */
	CHANNEL(d, i).v = value;
	CHANNEL(d, i).b = value;
      }

      for (i=0, channel_mask = 1, p = cd; i < CHANNEL_COUNT(d); i++, channel_mask <<= 1, p++) {
        int value;
	int base;
	int maxwet;
	dis_t *etd = &et->dis[i][x];

	advance_eventone_pre(&p->dis, et, etd);

	if ((p->wetness -= p->maxdot_dens) < 0) p->wetness = 0;

	base = CHANNEL(d, i).b;
	value = ndither[i] + base;
	if (value < 0) value = 0;				/* Dither can make this value negative */
	
        maxwet = (CHANNEL(d, i).b * CHANNEL(d, i).maxdot >> 1)
	 + p->maxdot_wet - p->wetness;
	
        find_segment(d, &CHANNEL(d, i), maxwet, value, &p->dr);
	
	range += eventone_adjust(&p->dr, et, p->dis.r_sq, base, value);
	if (range >= 32768) {
	  pick |= channel_mask;
	  range -= 65536;
	  value = p->dr.upper->value;
	} else {
	  value = p->dr.lower->value;
	}
        /* Adjust error values for dither */
	ndither[i] += 2 * (base - value);
	p->point = value;
	advance_eventone_post(p, et, etd);
      }
	
      { int useblack = 0;		/* Do we print black at all? */
	int printed_black;
	int adjusted_black;

        printed_black = cd[ECOLOR_K].point;
	adjusted_black = printed_black;
	if (printed_black > 0 && d->black_density != d->density) {
	  adjusted_black = (unsigned)printed_black * (unsigned)d->density / d->black_density;
	}

        /* Only print the black ink if it means we can avoid printing another ink, otherwise we're just wasting ink */

        if (printed_black > 0) {
	  for (i=1; i < CHANNEL_COUNT(d); i++) {
            if (cd[i].point <= adjusted_black) {
	      useblack = 1;
	      break;
	    }
          }
	}
	
	/* Find which channels we actually print */

	/* Adjust colours to print based on black ink */
        if (useblack) {
	  print_inks = (1 << ECOLOR_K);
	  for (i=1; i < CHANNEL_COUNT(d); i++) {
	    if (cd[i].point > adjusted_black) {
	      print_inks |= (1 << i);
	    }
	  }
        } else {
	  print_inks = (1 << ECOLOR_C)|(1 << ECOLOR_M)|(1<<ECOLOR_Y);
	}
      }

      /* Now we can finally print it! */
      
      print_all_inks(d, cd, print_inks, pick, bit, length);

      QUANT(11);
  
      /* Diffuse the error round a bit */
      diffuse_error(d, ndither, error, aspect, direction);

      QUANT(12);
      ADVANCE_BIDIRECTIONAL(d, bit, cmyk, direction, 4, xerror, xmod, error,
			    CHANNEL_COUNT(d), d->error_rows);
      QUANT(13);
    }

    stp_free(cd);
    shared_ed_deinitializer(d, error, ndither);
}

static void
stp_dither_raw_fast(const unsigned short  *raw,
		    int           row,
		    dither_t 	    *d,
		    int	       duplicate_line,
		    int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  int i;

  int dst_width = d->dst_width;
  int xerror, xstep, xmod;
  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = CHANNEL_COUNT(d) * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;

  QUANT(14);
  for (x = 0; x != dst_width; x++)
    {
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	{
	  CHANNEL(d, i).v = raw[i];
	  CHANNEL(d, i).o = CHANNEL(d, i).v;
	  if (CHANNEL(d, i).ptrs[0])
	    print_color_fast(d, &(CHANNEL(d, i)), x, row, bit, length);
	}
      QUANT(16);
      ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror, xmod);
      QUANT(17);
    }
}

static void
stp_dither_raw_very_fast(const unsigned short  *raw,
			 int           row,
			 dither_t 	    *d,
			 int	       duplicate_line,
			 int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
      int i;

  int dst_width = d->dst_width;
  int xerror, xstep, xmod;
  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
    return;

  for (i = 0; i < CHANNEL_COUNT(d); i++)
    if (!(CHANNEL(d, i).very_fast))
      {
	stp_dither_raw_fast(raw, row, d, duplicate_line, zero_mask);
	return;
      }

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = CHANNEL_COUNT(d) * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  x = 0;

  QUANT(14);
  for (x = 0; x != dst_width; x++)
    {
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	{
	  dither_channel_t *dc = &(CHANNEL(d, i));
	  if (dc->ptrs[0] && raw[i] > ditherpoint_fast(d, &(dc->dithermat), x))
	    {
	      set_row_ends(dc, x, 0);
	      dc->ptrs[0][d->ptr_offset] |= bit;
	    }
	}
      QUANT(16);
      ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror, xmod);
      QUANT(17);
    }
}

static void
stp_dither_raw_ordered(const unsigned short  *raw,
		       int           row,
		       dither_t 	    *d,
		       int		  duplicate_line,
		       int		  zero_mask)
{
  int		x,
		length;
  unsigned char	bit;
  int i;

  int		terminate;
  int xerror, xstep, xmod;

  if ((zero_mask & ((1 << d->n_input_channels) - 1)) ==
      ((1 << d->n_input_channels) - 1))
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = CHANNEL_COUNT(d) * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;
  terminate = d->dst_width;

  QUANT(6);
  for (x = 0; x != terminate; x ++)
    {
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	{
	  CHANNEL(d, i).v = raw[i];
	  CHANNEL(d, i).o = CHANNEL(d, i).v;
	  print_color_ordered(d, &(CHANNEL(d, i)), x, row, bit, length, 0);
	}

      QUANT(11);
      ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror, xmod);
      QUANT(13);
  }
}

static void
stp_dither_raw_ed(const unsigned short  *raw,
		  int           row,
		  dither_t 	    *d,
		  int		  duplicate_line,
		  int		  zero_mask)
{
  int		x,
    		length;
  unsigned char	bit;
  int		i;
  int		*ndither;
  int		***error;

  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int xerror, xstep, xmod;

  length = (d->dst_width + 7) / 8;
  if (!shared_ed_initializer(d, row, duplicate_line, zero_mask, length,
			     direction, &error, &ndither))
    return;

  x = (direction == 1) ? 0 : d->dst_width - 1;
  bit = 1 << (7 - (x & 7));
  xstep  = CHANNEL_COUNT(d) * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;
  terminate = (direction == 1) ? d->dst_width : -1;

  if (direction == -1)
    raw += (CHANNEL_COUNT(d) * (d->src_width - 1));

  QUANT(6);
  for (; x != terminate; x += direction)
    {
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	{
	  CHANNEL(d, i).v = raw[i];
	  CHANNEL(d, i).o = CHANNEL(d, i).v;
	  CHANNEL(d, i).b = CHANNEL(d, i).v;
	  CHANNEL(d, i).v = UPDATE_COLOR(CHANNEL(d, i).v, ndither[i]);
	  CHANNEL(d, i).v = print_color(d, &(CHANNEL(d, i)), x, row, bit,
					length, 0, d->dither_type);
	  ndither[i] = update_dither(d, i, d->src_width,
				     direction, error[i][0], error[i][1]);
	}
      QUANT(12);
      ADVANCE_BIDIRECTIONAL(d, bit, raw, direction, CHANNEL_COUNT(d), xerror,
			    xmod, error, CHANNEL_COUNT(d), d->error_rows);
      QUANT(13);
    }
  shared_ed_deinitializer(d, error, ndither);
  if (direction == -1)
    reverse_row_ends(d);
}

static void
stp_dither_raw_et(const unsigned short  *raw,
		   int           row,
		   dither_t 	 *d,
		   int		 duplicate_line,
		   int		 zero_mask)
{
  int		x, length;
  unsigned char	bit;
  int		*ndither;
  eventone_t	*et;
  et_chdata_t	*cd;

  int		***error;
  int		terminate;
  int		direction = row & 1 ? 1 : -1;
  int		xerror, xstep, xmod;
  int		aspect = d->y_aspect / d->x_aspect;
  unsigned      print_inks = (1 << CHANNEL_COUNT(d)) - 1;

  if (aspect >= 4)
    aspect = 4; 
  else if (aspect >= 2)
    aspect = 2; 
  else aspect = 1;

  length = (d->dst_width + 7) / 8;
  if (!shared_ed_initializer(d, row, duplicate_line, zero_mask, length,
			     direction, &error, &ndither))
    return;

  eventone_init(d, &cd);
  et = d->eventone;

  x = (direction == 1) ? 0 : d->dst_width - 1;
  bit = 1 << (7 - (x & 7));
  xstep  = CHANNEL_COUNT(d) * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;
  terminate = (direction == 1) ? d->dst_width : -1;

  if (direction == -1)
    raw += (CHANNEL_COUNT(d) * (d->src_width - 1));

  QUANT(6);
  for (; x != terminate; x += direction)
    {
      unsigned pick = 0;
      unsigned channel_mask;
      int i;
      int range = 0;
      et_chdata_t *p;
      
      for (i = 0, channel_mask = 1, p = cd; i < CHANNEL_COUNT(d); i++, channel_mask <<= 1, p++)
	{
	  int value = raw[i];
	  int base = value;
	  int maxwet;
	  dis_t *etd = &et->dis[i][x];

	  CHANNEL(d, i).b = value;
	  CHANNEL(d, i).v = value;
	  CHANNEL(d, i).o = value;
	  
	  advance_eventone_pre(&p->dis, et, etd);

	  if ((p->wetness -= p->maxdot_dens) < 0)
	    p->wetness = 0;

	  value = ndither[i] + base;
	  if (value < 0)  /* Dither can make this value negative */
	    value = 0;
	
	  maxwet = (CHANNEL(d, i).b * CHANNEL(d,i).maxdot >> 1)
	    + p->maxdot_wet - p->wetness;
	
	  find_segment(d, &CHANNEL(d, i), maxwet, value, &p->dr);
	
	  range += eventone_adjust(&p->dr, et, p->dis.r_sq, base, value);
	  if (range >= 32768) {
	    pick |= channel_mask;
	    range -= 32768;
	    value = p->dr.upper->value;
	  } else {
	    value = p->dr.lower->value;
	  }
	  /* Adjust error values for dither */
	  ndither[i] += 2 * (base - value);
	  p->point = value;
          advance_eventone_post(p, et, etd);
	}

      /* Now we can finally print it! */
      
      print_all_inks(d, cd, print_inks, pick, bit, length);

      QUANT(11);
  
      /* Diffuse the error round a bit */
      diffuse_error(d, ndither, error, aspect, direction);

      QUANT(12);
      ADVANCE_BIDIRECTIONAL(d, bit, raw, direction, CHANNEL_COUNT(d),
			    xerror, xmod, error, CHANNEL_COUNT(d),
			    d->error_rows);
      QUANT(13);
    }
  stp_free(cd);
  shared_ed_deinitializer(d, error, ndither);
}

void
stp_dither(const unsigned short  *input,
	   int           row,
	   void 	  *vd,
	   stp_dither_data_t *dt,
	   int		  duplicate_line,
	   int		  zero_mask)
{
  int i, j;
  dither_t *d = (dither_t *) vd;
  int ghost_channels = 0;
  for (i = 0; i < PHYSICAL_CHANNEL_COUNT(d); i++)
    {
      for (j = 0; j < PHYSICAL_CHANNEL(d, i).subchannels; j++)
	{
	  if (i >= dt->channel_count || j >= dt->c[i].subchannel_count)
	    {
	      PHYSICAL_CHANNEL(d, i).ptrs[j] = NULL;
	      ghost_channels++;
	    }
	  else
	    PHYSICAL_CHANNEL(d, i).ptrs[j] = dt->c[i].c[j];
	  if (PHYSICAL_CHANNEL(d, i).ptrs[j])
	    memset(PHYSICAL_CHANNEL(d, i).ptrs[j], 0,
		   (d->dst_width + 7) / 8 * PHYSICAL_CHANNEL(d, i).signif_bits);
	  PHYSICAL_CHANNEL(d, i).row_ends[0][j] = -1;
	  PHYSICAL_CHANNEL(d, i).row_ends[1][j] = -1;
	}
      stp_dither_matrix_set_row(&(PHYSICAL_CHANNEL(d, i).dithermat), row);
      stp_dither_matrix_set_row(&(PHYSICAL_CHANNEL(d, i).pick), row);
    }
  d->n_ghost_channels = ghost_channels;
  d->ptr_offset = 0;
  (d->ditherfunc)(input, row, d, duplicate_line, zero_mask);
}
