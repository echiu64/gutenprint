/*
 * "$Id$"
 *
 *   Error diffusion and closely related adaptive hybrid dither algorithm
 *
 *   Copyright 1997-2003 Michael Sweet (mike@easysw.com) and
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
#include <string.h>
#include "dither-impl.h"
#include "dither-inlined-functions.h"

/*
 * Add the error to the input value.  Notice that we micro-optimize this
 * to save a division when appropriate.  This has yielded measurable
 * improvement -- rlk 20030119
 */

#define UPDATE_COLOR(color, dither) (\
        ((dither) >= 0)? \
                (color) + ((dither) >> 3): \
                (color) - ((-(dither)) >> 3))

static int *
get_errline(stpi_dither_t *d, int row, int color)
{
  if (row < 0 || color < 0 || color >= CHANNEL_COUNT(d))
    return NULL;
  if (CHANNEL(d, color).errs[row & 1])
    return CHANNEL(d, color).errs[row & 1] + MAX_SPREAD;
  else
    {
      int size = 2 * MAX_SPREAD + (16 * ((d->dst_width + 7) / 8));
      CHANNEL(d, color).errs[row & 1] = stpi_zalloc(size * sizeof(int));
      return CHANNEL(d, color).errs[row & 1] + MAX_SPREAD;
    }
}

/*
 * For Floyd-Steinberg, distribute the error residual.  We spread the
 * error to nearby points, spreading more broadly in lighter regions to
 * achieve more uniform distribution of color.  The actual distribution
 * is a triangular function.
 */

static inline int
update_dither(stpi_dither_t *d, int channel, int width,
	      int direction, int *error0, int *error1)
{
  int r = CHANNEL(d, channel).v;
  int o = CHANNEL(d, channel).o;
  int tmp = r;
  int i, dist, dist1;
  int delta, delta1;
  int offset;
  int xs = 65535 / CHANNEL(d, channel).density_adjustment;
  if (tmp == 0)
    return error0[direction];
  if (tmp > xs)
    tmp = xs;
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


/*
 * Print a single dot.  This routine has become awfully complicated
 * awfully fast!
 */

static inline int
print_color(const stpi_dither_t *d, stpi_dither_channel_t *dc, int x, int y,
	    unsigned char bit, int length, int dontprint, int stpi_dither_type)
{
  int base = dc->b;
  int density = dc->o;
  int adjusted = dc->v;
  int xdensity = density;
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
  stpi_dither_segment_t *dd;
  stpi_ink_defn_t *lower;
  stpi_ink_defn_t *upper;

  if (base <= 0 || density <= 0 ||
      (adjusted <= 0 && !(stpi_dither_type & D_ADAPTIVE_BASE)))
    return adjusted;
  if (density > 65535)
    density = 65535;
  xdensity *= dc->density_adjustment;

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

      if (xdensity <= dd->lower->range)
	continue;

      /*
       * If we're using an adaptive dithering method, decide whether
       * to use the Floyd-Steinberg or the ordered method based on the
       * input value.
       */
      if (stpi_dither_type & D_ADAPTIVE_BASE)
	{
	  stpi_dither_type -= D_ADAPTIVE_BASE;

	  if (i < levels || base <= d->adaptive_limit)
	    {
	      stpi_dither_type = D_ORDERED;
	      dither_value = base;
	    }
	  else if (adjusted <= 0)
	    return adjusted;
	}
      dither_value *= dc->density_adjustment;

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
	  ((unsigned) (xdensity - lower->range)) * 65535 / dd->range_span;

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
      if (stpi_dither_type & D_ORDERED_BASE)
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
	  stpi_ink_defn_t *subc;

	  if (dd->is_same_ink)
	    subc = upper;
	  else
	    {
	      rangepoint *= dc->density_adjustment;
	      if (rangepoint >= ditherpoint(d, pick_matrix, x))
		subc = upper;
	      else
		subc = lower;
	    }
	  subchannel = subc->subchannel;
	  bits = subc->bits;
	  v = subc->value;
	  dot_size = subc->dot_size;
	  if (dc->ptrs[subchannel])
	    {
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
	    }
	  if (stpi_dither_type & D_ORDERED_BASE)
	    {
	      double adj = -(int) v;
	      adj /= 2.0;
	      adj /= dc->density_adjustment;
	      adjusted = adj;
	    }
	  else
	    {
	      double adj = v;
	      adj /= dc->density_adjustment;
	      adjusted -= adj;
	    }
	}
      return adjusted;
    }
  return adjusted;
}

static int
shared_ed_initializer(stpi_dither_t *d,
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

  *error = stpi_malloc(CHANNEL_COUNT(d) * sizeof(int **));
  *ndither = stpi_malloc(CHANNEL_COUNT(d) * sizeof(int));
  for (i = 0; i < CHANNEL_COUNT(d); i++)
    {
      (*error)[i] = stpi_malloc(d->error_rows * sizeof(int *));
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
shared_ed_deinitializer(stpi_dither_t *d,
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

static void
stpi_dither_raw_ed(stp_vars_t v,
		  int row,
		  const unsigned short *raw,
		  int duplicate_line,
		  int zero_mask)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
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
					length, 0, d->stpi_dither_type);
	  ndither[i] = update_dither(d, i, d->src_width,
				     direction, error[i][0], error[i][1]);
	}
      QUANT(12);
      ADVANCE_BIDIRECTIONAL(d, bit, raw, direction, CHANNEL_COUNT(d), xerror,
			    xstep, xmod, error, CHANNEL_COUNT(d), d->error_rows);
      QUANT(13);
    }
  shared_ed_deinitializer(d, error, ndither);
  if (direction == -1)
    stpi_dither_reverse_row_ends(d);
}

static void
stpi_dither_raw_cmyk_ed(stp_vars_t v,
		       int row,
		       const unsigned short *cmyk,
		       int duplicate_line,
		       int zero_mask)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
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
					length, 0, d->stpi_dither_type);
	  ndither[i] = update_dither(d, i, d->src_width,
				     direction, error[i][0], error[i][1]);
	}
      QUANT(12);
      ADVANCE_BIDIRECTIONAL(d, bit, cmyk, direction, 4, xerror, xstep, xmod, error,
			    CHANNEL_COUNT(d), d->error_rows);
      QUANT(13);
    }
  shared_ed_deinitializer(d, error, ndither);
  if (direction == -1)
    stpi_dither_reverse_row_ends(d);
}

void
stpi_dither_ed(stp_vars_t v,
	      int row,
	      const unsigned short *input,
	      int duplicate_line,
	      int zero_mask)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
  if (d->dither_class != OUTPUT_RAW_CMYK ||
      d->n_ghost_channels > 0)
    stpi_dither_raw_ed(v, row, input, duplicate_line, zero_mask);
  else
    stpi_dither_raw_cmyk_ed(v, row, input, duplicate_line, zero_mask);
}
