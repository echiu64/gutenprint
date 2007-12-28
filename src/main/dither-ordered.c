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
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include "dither-impl.h"
#include "dither-inlined-functions.h"
#include <assert.h>

typedef struct {
  size_t channels;
  double *drops;
  unsigned short *lb, *ub;
  unsigned short *lut;
} stpi_new_ordered_t;

static int
compare_channels(const stpi_dither_channel_t *dc1,
		 const stpi_dither_channel_t *dc2)
{
  int i;
  if (dc1->nlevels != dc2->nlevels)
    return 0;
  for (i = 0; i < dc1->nlevels; i++)
    if (dc1->ranges[i].upper->value != dc2->ranges[i].upper->value)
      return 0;
  return 1;
}

static void
free_dither_ordered_new(stpi_dither_t *d)
{
  int i;
  stpi_dither_channel_t *dc0 = &CHANNEL(d, 0);
  for (i = CHANNEL_COUNT(d) - 1; i >= 0 ; i--)
    {
      stpi_dither_channel_t *dc = &CHANNEL(d, i);
      if (dc->aux_data && (i == 0 || dc->aux_data != dc0->aux_data))
	{
	  stpi_new_ordered_t *ord = (stpi_new_ordered_t *) dc->aux_data;
	  if (ord->drops)
	    stp_free(ord->drops);
	  if (ord->lb)
	    stp_free(ord->lb);
	  if (ord->ub)
	    stp_free(ord->ub);
	  if (ord->lut)
	    stp_free(ord->lut);
	  stp_free(dc->aux_data);
	}
      dc->aux_data = NULL;
    }
  stp_free(d->aux_data);
}

static void
init_dither_channel_new(stpi_dither_channel_t *dc, stp_vars_t *v)
{
  int j;
  double *rp;
  double *val;
  unsigned short *data;
  stpi_new_ordered_t *ord = stp_malloc(sizeof(stpi_new_ordered_t));
  dc->aux_data = ord;
  ord->channels = dc->nlevels - 1;
  ord->drops = stp_malloc(sizeof(double) * ord->channels);
  rp = stp_malloc(sizeof(double) * ord->channels);
  val = stp_malloc(sizeof(double) * ord->channels);
  ord->lb = stp_malloc(sizeof(unsigned short) * ord->channels);
  ord->ub = stp_malloc(sizeof(unsigned short) * ord->channels);
  data = stp_malloc(sizeof(unsigned short) * 65536 * ord->channels);
  ord->lut = data;
  for (j = 0; j < ord->channels; j++)
    {
      stpi_dither_segment_t *dd = &(dc->ranges[j]);
      ord->drops[j] = (double) dd->upper->value / 65535.0;
    }
  for (j = 0; j < ord->channels; j++)
    {
      if (j == 0)
	ord->lb[j] = 0;
      else if (j == 1)
	{
	  double divisor = ord->drops[j] / ord->drops[j - 1];
	  if (divisor < 3)
	    divisor = 3;
	  ord->lb[j] = 65535 * ord->drops[j - 1] / divisor;
	}
      else
	ord->lb[j] = 65535 * ord->drops[j - 2];
      if (j == ord->channels - 1)
	ord->ub[j] = 65535;
      else
	{
	  int ub = 2 * 65535 * ord->drops[j + 1];
	  if (ub > 65535)
	    ub = 65535;
	  ord->ub[j] = ub;
	}
      stp_dprintf(STP_DBG_INK, v, "        size %.3f lb %5d ub %5d\n",
		  ord->drops[j], ord->lb[j], ord->ub[j]);
    }
  for (j = 0; j < 65536; j++)
    {
      int k;
      int first = -1;
      int last = ord->channels - 1;
      double sum = 0.0;
      int resid = j;
      double total = 0;
      double total_ink = 0;
      /* Need to clean up some of the arithmetic... */
      for (k = 0; k < ord->channels; k++)
	{
	  rp[k] = 0;
	  val[k] = 0;
	  if (j >= ord->lb[k] && j <= ord->ub[k])
	    {
	      if (first < 0)
		first = k;
	      last = k;
	    }
	}
      for (k = last; k >= first && sum < 1; k--)
	{
	  rp[k] = (double) (j - ord->lb[k]) / (ord->ub[k] - ord->lb[k]);
	  rp[k] = rp[k] / ord->drops[k];
	  sum += rp[k];
	  if (sum > 1)
	    {
	      rp[k] -= (sum - 1.0);
	      sum = 1.0;
	    }
	}
      for (k = last; k >= first && resid > 0; k--)
	{
	  val[k] = resid * rp[k];
	  resid -= val[k];
	  if (resid < 0)
	    {
	      val[k] += resid;
	      resid = 0;
	    }
	}
      if (resid > 0)
	for (k = last; k >= first; k--)
	  val[k] += resid * rp[k];
      for (k = first; k <= last; k++)
	total += val[k] * ord->drops[k];
      if (total != j)
	{
	  double ratio = (double) j / (double) total;
	  for (k = first; k <= last; k++)
	    val[k] *= ratio;
	}
      for (k = first; k <= last; k++)
	total_ink += val[k];
      if (total_ink > 65535)
	{
	  stp_eprintf(v, "Error in dither initialization:\n");
	  for (k = 0; k < ord->channels; k++)	      
	    stp_eprintf(v, "   k=%d, size %.3f lb %d ub %d\n",
			k, ord->drops[k], ord->lb[k], ord->lb[k]);
	  stp_eprintf(v, "   j=%d, rp=( ", j);
	  for (k = 0; k < ord->channels; k++)	      
	    stp_eprintf(v, "%9.3f ", rp[k]);
	  stp_eprintf(v, "), vals=( ");
	  for (k = 0; k < ord->channels; k++)	      
	    stp_eprintf(v, "%9.3f ", val[k]);
	  stp_eprintf(v, ")\n");
	  assert(total_ink <= 65535);
	}
      total_ink = 0;
      for (k = ord->channels - 1; k >= 0; k--)
	{
	  total_ink += val[k];
	  data[k] = total_ink;
	}
      if (j % 257 == 0)
	{
	  stp_dprintf(STP_DBG_INK, v, "    %5d:", j);
	  for (k = 0; k < ord->channels; k++)
	    stp_dprintf(STP_DBG_INK, v, " %9.3f", val[k]);
	  stp_dprintf(STP_DBG_INK, v, "  ");
	  for (k = 0; k < ord->channels; k++)
	    stp_dprintf(STP_DBG_INK, v, " %9.3f", rp[k]);
	  stp_dprintf(STP_DBG_INK, v, "  ");
	  for (k = 0; k < ord->channels; k++)
	    stp_dprintf(STP_DBG_INK, v, " %5d", data[k]);
	  stp_dprintf(STP_DBG_INK, v, "\n");
	}
      data += ord->channels;
    }
  stp_free(rp);
  stp_free(val);
}

static void
init_dither_ordered_new(stpi_dither_t *d, stp_vars_t *v)
{
  int i;
  d->aux_data = stp_malloc(1);
  d->aux_freefunc = &free_dither_ordered_new;
  stp_dprintf(STP_DBG_INK, v, "init_dither_ordered_new\n");
  for (i = 0; i < CHANNEL_COUNT(d); i++)
    {
      if (CHANNEL(d, i).nlevels < 2)
	{
	  stp_dprintf(STP_DBG_INK, v, "    channel %d ignored\n", i);
	  CHANNEL(d, i).aux_data = NULL;
	}
      else if (i == 0 || !compare_channels(&CHANNEL(d, 0), &CHANNEL(d, i)))
	{
	  stp_dprintf(STP_DBG_INK, v, "    channel %d\n", i);
	  init_dither_channel_new(&CHANNEL(d, i), v);
	}
      else
	{
	  stp_dprintf(STP_DBG_INK, v, "    channel %d duplicated from channel 0\n", i);
	  CHANNEL(d, i).aux_data = CHANNEL(d, 0).aux_data;
	}
    }
}

static inline void
print_color_ordered_new(const stpi_dither_t *d, stpi_dither_channel_t *dc,
			int val, int x, int y, unsigned char bit, int length)
{
  int i;
  int j;
  unsigned bits;
  int levels = dc->nlevels - 1;
  unsigned dpoint = ditherpoint(d, &(dc->dithermat), x);
  const stpi_new_ordered_t *ord = (const stpi_new_ordered_t *) dc->aux_data;
  unsigned short swhere = (unsigned short) val;
  unsigned short *where = ord ? ord->lut + (val * levels) : &swhere;
  /*
   * Look for the appropriate range into which the input value falls.
   * Notice that we use the input, not the error, to decide what dot type
   * to print (if any).  We actually use the "density" input to permit
   * the caller to use something other that simply the input value, if it's
   * desired to use some function of overall density, rather than just
   * this color's input, for this purpose.
   */
  for (i = levels - 1; i >= 0; i--)
    {
      if (dpoint < where[i])
	{
	  stpi_dither_segment_t *dd = &(dc->ranges[i]);
	  bits = dd->upper->bits;
	  if (bits)
	    {
	      unsigned char *tptr = dc->ptr + d->ptr_offset;

	      /*
	       * Lay down all of the bits in the pixel.
	       */
	      set_row_ends(dc, x);
	      for (j = 1; j <= bits; j += j, tptr += length)
		{
		  if (j & bits)
		    tptr[0] |= bit;
		}

	    }
	  return;
	}
    }
}

static inline void
print_color_ordered(const stpi_dither_t *d, stpi_dither_channel_t *dc, int val,
		    int x, int y, unsigned char bit, int length)
{
  int i;
  int j;
  unsigned bits;
  int levels = dc->nlevels - 1;

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
      stpi_dither_segment_t *dd = &(dc->ranges[i]);

      if (val > dd->lower->value)
	{
	  /*
	   * Where are we within the range.
	   */

	  unsigned rangepoint = val - dd->lower->value;
	  if (dd->value_span < 65535)
	    rangepoint = rangepoint * 65535 / dd->value_span;

	  if (rangepoint >= ditherpoint(d, &(dc->dithermat), x))
	    bits = dd->upper->bits;
	  else
	    bits = dd->lower->bits;

	  if (bits)
	    {
	      unsigned char *tptr = dc->ptr + d->ptr_offset;

	      /*
	       * Lay down all of the bits in the pixel.
	       */
	      set_row_ends(dc, x);
	      for (j = 1; j <= bits; j += j, tptr += length)
		{
		  if (j & bits)
		    tptr[0] |= bit;
		}

	    }
	  return;
	}
    }
}


void
stpi_dither_ordered(stp_vars_t *v,
		    int row,
		    const unsigned short *raw,
		    int duplicate_line,
		    int zero_mask,
		    const unsigned char *mask)
{
  stpi_dither_t *d = (stpi_dither_t *) stp_get_component_data(v, "Dither");
  int		x,
		length;
  unsigned char	bit;
  int i;
  int one_bit_only = 1;
  int one_level_only = 1;

  int xerror, xstep, xmod;

  if ((zero_mask & ((1 << CHANNEL_COUNT(d)) - 1)) ==
      ((1 << CHANNEL_COUNT(d)) - 1))
    return;

  length = (d->dst_width + 7) / 8;

  bit = 128;
  xstep  = CHANNEL_COUNT(d) * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = 0;

  for (i = 0; i < CHANNEL_COUNT(d); i++)
    {
      stpi_dither_channel_t *dc = &(CHANNEL(d, i));
      if (dc->nlevels != 1)
	one_level_only = 0;
      if (dc->nlevels != 1 || dc->ranges[0].upper->bits != 1)
	one_bit_only = 0;
    }

  if (one_bit_only)
    {
      for (x = 0; x < d->dst_width; x ++)
	{
	  if (!mask || (*(mask + d->ptr_offset) & bit))
	    {
	      for (i = 0; i < CHANNEL_COUNT(d); i++)
		{
		  if (raw[i] &&
		      raw[i] >= ditherpoint(d, &(CHANNEL(d, i).dithermat), x))
		    {
		      set_row_ends(&(CHANNEL(d, i)), x);
		      CHANNEL(d, i).ptr[d->ptr_offset] |= bit;
		    }
		}
	    }
	  ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d),
				 xerror, xstep, xmod);
	}
    }
  else if (one_level_only || !(d->stpi_dither_type == D_ORDERED_NEW))
    {
      for (x = 0; x != d->dst_width; x ++)
	{
	  if (!mask || (*(mask + d->ptr_offset) & bit))
	    {
	      for (i = 0; i < CHANNEL_COUNT(d); i++)
		{
		  if (CHANNEL(d, i).ptr && raw[i])
		    print_color_ordered(d, &(CHANNEL(d, i)), raw[i], x, row,
					bit, length);
		}
	    }
	  ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror,
				 xstep, xmod);
	}
    }
  else
    {
      if (! d->aux_data)
	init_dither_ordered_new(d, v);
      for (x = 0; x != d->dst_width; x ++)
	{
	  if (!mask || (*(mask + d->ptr_offset) & bit))
	    {
	      for (i = 0; i < CHANNEL_COUNT(d); i++)
		{
		  if (CHANNEL(d, i).ptr && raw[i])
		    print_color_ordered_new(d, &(CHANNEL(d, i)), raw[i], x,
					    row, bit, length);
		}
	    }
	  ADVANCE_UNIDIRECTIONAL(d, bit, raw, CHANNEL_COUNT(d), xerror,
				 xstep, xmod);
	}
    }
}
