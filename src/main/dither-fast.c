/*
 * "$Id$"
 *
 *   Fast dither algorithm
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

static inline void
print_color_fast(const dither_t *d, dither_channel_t *dc, int x, int y,
		 unsigned char bit, int length)
{
  int density = dc->o;
  int adjusted = dc->v;
  int xdensity = density;
  dither_matrix_t *dither_matrix = &(dc->dithermat);
  int i;
  int levels = dc->nlevels - 1;
  int j;
  unsigned char *tptr;
  unsigned bits;

  if (density <= 0 || adjusted <= 0)
    return;
  adjusted *= dc->density_adjustment;
  xdensity *= dc->density_adjustment;
  for (i = levels; i >= 0; i--)
    {
      dither_segment_t *dd = &(dc->ranges[i]);
      unsigned vmatrix;
      unsigned rangepoint;
      unsigned dpoint;
      unsigned range0;
      ink_defn_t *subc;

      range0 = dd->lower->range;
      if (xdensity <= range0)
	continue;
      dpoint = ditherpoint(d, dither_matrix, x);

      if (dd->is_same_ink)
	subc = dd->upper;
      else
	{
	  rangepoint = ((xdensity - range0) << 16) / dd->range_span *
	    dc->density_adjustment;
	  if (rangepoint >= dpoint)
	    subc = dd->upper;
	  else
	    subc = dd->lower;
	}
      vmatrix = ((subc->value * dpoint) >> 16);

      /*
       * After all that, printing is almost an afterthought.
       * Pick the actual dot size (using a matrix here) and print it.
       */
      if (adjusted >= vmatrix && dc->ptrs[subc->subchannel])
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

extern void
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

  if (d->n_ghost_channels)
    {
      stp_dither_raw_fast(cmyk, row, d, duplicate_line, zero_mask);
      return;
    }

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
      ADVANCE_UNIDIRECTIONAL(d, bit, cmyk, 4, xerror, xstep, xmod);
      QUANT(17);
    }
}

void
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
      ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror, xstep, xmod);
      QUANT(17);
    }
}
