/*
 * "$Id$"
 *
 *   Ordered dither algorithm
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
#include "dither-impl.h"
#include "dither-inlined-functions.h"

static inline int
print_color_ordered(const stpi_dither_t *d, stpi_dither_channel_t *dc, int x, int y,
		    unsigned char bit, int length, int dontprint)
{
  int density = dc->o;
  int adjusted = dc->v;
  int xdensity = density;
  dither_matrix_t *pick_matrix = &(dc->pick);
  dither_matrix_t *dither_matrix = &(dc->dithermat);
  unsigned rangepoint;
  unsigned virtual_value;
  unsigned vmatrix;
  int i;
  int j;
  unsigned char *tptr;
  unsigned bits;
  unsigned v;
  int levels = dc->nlevels - 1;
  int dither_value = adjusted;
  stpi_dither_segment_t *dd;
  stpi_ink_defn_t *lower;
  stpi_ink_defn_t *upper;

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

      if (xdensity <= dd->lower->range)
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
	  ((unsigned) (xdensity - lower->range)) * 65535 / dd->range_span;
      rangepoint = d->virtual_dot_scale[rangepoint];

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
	virtual_value = lower->value + (dd->value_span * rangepoint / 65535);

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
      if (dither_value > 0 && dither_value >= vmatrix)
	{
	  stpi_ink_defn_t *subc;

	  if (dd->is_same_ink)
	    subc = upper;
	  else
	    {
	      if (rangepoint >= ditherpoint(d, pick_matrix, x))
		subc = upper;
	      else
		subc = lower;
	    }
	  bits = subc->bits;
	  v = subc->value;
	  if (dc->ptr)
	    {
	      tptr = dc->ptr + d->ptr_offset;

	      /*
	       * Lay down all of the bits in the pixel.
	       */
	      if (dontprint < v)
		{
		  set_row_ends(dc, x);
		  for (j = 1; j <= bits; j += j, tptr += length)
		    {
		      if (j & bits)
			tptr[0] |= bit;
		    }
		  return v;
		}
	    }
	}
      return 0;
    }
  return 0;
}

void
stpi_dither_ordered(stp_vars_t v,
		    int row,
		    const unsigned short *raw,
		    int duplicate_line,
		    int zero_mask)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  int		x,
		length;
  unsigned char	bit;
  int i;

  int		terminate;
  int xerror, xstep, xmod;

  if ((zero_mask & ((1 << CHANNEL_COUNT(d)) - 1)) ==
      ((1 << CHANNEL_COUNT(d)) - 1))
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
	  if (CHANNEL(d, i).ptr)
	    {
	      CHANNEL(d, i).v = raw[i];
	      CHANNEL(d, i).o = CHANNEL(d, i).v;
	      print_color_ordered(d, &(CHANNEL(d, i)), x, row, bit, length, 0);
	    }
	}

      QUANT(11);
      ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror, xstep, xmod);
      QUANT(13);
  }
}
