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
#include "dither-impl.h"

int
stpi_dither_translate_channel(stp_vars_t v, unsigned channel,
			      unsigned subchannel)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  unsigned chan_idx;
  if (!d)
    return -1;
  if (channel >= d->channel_count)
    return -1;
  if (subchannel >= d->subchannel_count[channel])
    return -1;
  chan_idx = d->channel_index[channel];
  return chan_idx + subchannel;
}

unsigned char *
stpi_dither_get_channel(stp_vars_t v, unsigned channel, unsigned subchannel)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  int place = stpi_dither_translate_channel(v, channel, subchannel);
  if (place >= 0)
    return d->channel[place].ptr;
  else
    return NULL;
}

static void
insert_channel(stp_vars_t v, stpi_dither_t *d, int channel)
{
  unsigned oc = d->channel_count;
  int i;
  d->channel_index =
    stpi_realloc (d->channel_index, sizeof(unsigned) * (channel + 1));
  d->subchannel_count =
    stpi_realloc (d->subchannel_count, sizeof(unsigned) * (channel + 1));
  for (i = oc; i < channel + 1; i++)
    {
      if (oc == 0)
	d->channel_index[i] = 0;
      else
	d->channel_index[i] =
	  d->channel_index[oc - 1] + d->subchannel_count[oc - 1];
      d->subchannel_count[i] = 0;
    }
  d->channel_count = channel + 1;
}

void
stpi_dither_channel_destroy(stpi_dither_channel_t *channel)
{
  int i;
  SAFE_FREE(channel->vals);
  SAFE_FREE(channel->ink_list);
  if (channel->errs)
    {
      for (i = 0; i < channel->error_rows; i++)
	SAFE_FREE(channel->errs[i]);
      SAFE_FREE(channel->errs);
    }
  SAFE_FREE(channel->errs);
  SAFE_FREE(channel->ranges);
  if (channel->shades)
    {
      for (i = 0; i < channel->numshades; i++)
	{
	  SAFE_FREE(channel->shades[i].dotsizes);
	  SAFE_FREE(channel->shades[i].errs);
	  SAFE_FREE(channel->shades[i].et_dis);
	}
      SAFE_FREE(channel->shades);
    }
  stpi_dither_matrix_destroy(&(channel->pick));
  stpi_dither_matrix_destroy(&(channel->dithermat));
}  

static void
initialize_channel(stp_vars_t v, int channel, int subchannel)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  int idx = stpi_dither_translate_channel(v, channel, subchannel);
  stpi_dither_channel_t *dc = &(CHANNEL(d, idx));
  stpi_shade_t shade;
  stpi_dotsize_t dot;
  memset(dc, 0, sizeof(stpi_dither_channel_t));
  stpi_dither_matrix_clone(&(d->dither_matrix), &(dc->dithermat), 0, 0);
  stpi_dither_matrix_clone(&(d->transition_matrix), &(dc->pick), 0, 0);
  shade.dot_sizes = &dot;
  shade.value = 65535.0;
  shade.numsizes = 1;
  dot.bit_pattern = 1;
  dot.value = 1.0;
  stpi_dither_set_inks_full(v, channel, 1, &shade, 1.0);
}

static void
insert_subchannel(stp_vars_t v, stpi_dither_t *d, int channel, int subchannel)
{
  int i;
  unsigned oc = d->subchannel_count[channel];
  unsigned increment = subchannel - oc + 1;
  unsigned old_place = d->channel_index[channel] + oc;
  stpi_dither_channel_t *nc =
    stpi_malloc(sizeof(stpi_dither_channel_t) *
		(d->total_channel_count + increment));
      
  if (d->channel)
    {
      /*
       * Copy the old channels, including all subchannels of the current
       * channel that already existed.
       */
      memcpy(nc, d->channel, sizeof(stpi_dither_channel_t) * old_place);
      if (old_place < d->total_channel_count)
	/*
	 * If we're inserting a new subchannel in the middle somewhere,
	 * we need to move everything else up
	 */
	memcpy(nc + old_place + increment, d->channel + old_place,
	       (sizeof(stpi_dither_channel_t) *
		(d->total_channel_count - old_place)));
      stpi_free(d->channel);
    }
  d->channel = nc;
  if (channel < d->channel_count - 1)
    /* Now fix up the subchannel offsets */
    for (i = channel + 1; i < d->channel_count; i++)
      d->channel_index[i] += increment;
  d->subchannel_count[channel] = subchannel + 1;
  d->total_channel_count += increment;
  for (i = oc; i < oc + increment; i++)
    initialize_channel(v, channel, i);
}

void
stpi_dither_finalize(stp_vars_t v)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  if (!d->finalized)
    {
      int i;
      unsigned rc = 1 + (unsigned) ceil(sqrt(CHANNEL_COUNT(d)));
      unsigned x_n = d->dither_matrix.x_size / rc;
      unsigned y_n = d->dither_matrix.y_size / rc;
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	{
	  stpi_dither_channel_t *dc = &(CHANNEL(d, i));
	  stpi_dither_matrix_clone(&(d->dither_matrix), &(dc->dithermat),
				   x_n * (i % rc), y_n * (i / rc));
	  stpi_dither_matrix_clone(&(d->dither_matrix), &(dc->pick),
				   x_n * (i % rc), y_n * (i / rc));
	}
      d->finalized = 1;
    }
}

void
stpi_dither_add_channel(stp_vars_t v, unsigned char *data,
			unsigned channel, unsigned subchannel)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  int idx;
  if (channel >= d->channel_count)
    insert_channel(v, d, channel);
  if (subchannel >= d->subchannel_count[channel])
    insert_subchannel(v, d, channel, subchannel);
  idx = stpi_dither_translate_channel(v, channel, subchannel);
  d->channel[idx].ptr = data;
}

static void
stpi_dither_finalize_ranges(stp_vars_t v, stpi_dither_channel_t *dc)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  int i;
  unsigned lbit = dc->bit_max;
  dc->signif_bits = 0;
  while (lbit > 0)
    {
      dc->signif_bits++;
      lbit >>= 1;
    }

  dc->maxdot = 0;

  for (i = 0; i < dc->nlevels; i++)
    {
      if (dc->ranges[i].lower->bits == dc->ranges[i].upper->bits)
	dc->ranges[i].is_same_ink = 1;
      else
	dc->ranges[i].is_same_ink = 0;
      if (dc->ranges[i].range_span > 0 && dc->ranges[i].value_span > 0)
	dc->ranges[i].is_equal = 0;
      else
	dc->ranges[i].is_equal = 1;

      if (dc->ranges[i].lower->dot_size > dc->maxdot)
	dc->maxdot = dc->ranges[i].lower->dot_size;
      if (dc->ranges[i].upper->dot_size > dc->maxdot)
	dc->maxdot = dc->ranges[i].upper->dot_size;

      stpi_dprintf(STPI_DBG_INK, v,
		   "    level %d value[0] %d value[1] %d range[0] %d range[1] %d\n",
		   i, dc->ranges[i].lower->value, dc->ranges[i].upper->value,
		   dc->ranges[i].lower->range, dc->ranges[i].upper->range);
      stpi_dprintf(STPI_DBG_INK, v,
		   "       bits[0] %d bits[1] %d\n dot_size[0] %d dot_size[1] %d\n",
		   dc->ranges[i].lower->bits, dc->ranges[i].upper->bits,
		   dc->ranges[i].lower->dot_size,
		   dc->ranges[i].upper->dot_size);
      stpi_dprintf(STPI_DBG_INK, v,
		   "       rangespan %d valuespan %d same_ink %d equal %d\n",
		   dc->ranges[i].range_span, dc->ranges[i].value_span,
		   dc->ranges[i].is_same_ink, dc->ranges[i].is_equal);
      if (i > 0 && dc->ranges[i].lower->range >= d->adaptive_limit)
	{
	  d->adaptive_limit = dc->ranges[i].lower->range + 1;
	  if (d->adaptive_limit > 65535)
	    d->adaptive_limit = 65535;
	  stpi_dprintf(STPI_DBG_INK, v, "Setting adaptive limit to %d\n",
		       d->adaptive_limit);
	}
    }
  if (dc->nlevels == 1 && dc->ranges[0].upper->bits == 1)
    dc->very_fast = 1;
  else
    dc->very_fast = 0;

  stpi_dprintf(STPI_DBG_INK, v,
	       "  bit_max %d signif_bits %d\n", dc->bit_max, dc->signif_bits);
}

static void
stpi_dither_set_ranges(stp_vars_t v, int color,
		       const stpi_shade_t *shade, double density)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  stpi_dither_channel_t *dc = &(CHANNEL(d, color));
  const stpi_dotsize_t *ranges = shade->dot_sizes;
  int nlevels = shade->numsizes;
  int i;

  SAFE_FREE(dc->ranges);
  SAFE_FREE(dc->ink_list);

  dc->nlevels = nlevels > 1 ? nlevels + 1 : nlevels;
  dc->ranges = (stpi_dither_segment_t *)
    stpi_zalloc(dc->nlevels * sizeof(stpi_dither_segment_t));
  dc->ink_list = (stpi_ink_defn_t *)
    stpi_zalloc((dc->nlevels + 1) * sizeof(stpi_ink_defn_t));
  dc->bit_max = 0;
  dc->density = density * 65535;
  stpi_init_debug_messages(v);
  stpi_dprintf(STPI_DBG_INK, v,
	      "stpi_dither_set_ranges channel %d nlevels %d density %f\n",
	      color, nlevels, density);
  for (i = 0; i < nlevels; i++)
    stpi_dprintf(STPI_DBG_INK, v,
		"  level %d value %f pattern %x\n", i,
		ranges[i].value, ranges[i].bit_pattern);
  dc->ranges[0].lower = &dc->ink_list[0];
  dc->ranges[0].upper = &dc->ink_list[1];
  dc->ink_list[0].range = 0;
  dc->ink_list[0].value = ranges[0].value * 65535.0;
  dc->ink_list[0].bits = ranges[0].bit_pattern;
  if (nlevels == 1)
    dc->ink_list[1].range = 65535;
  else
    dc->ink_list[1].range = ranges[0].value * 65535.0 * density;
  if (dc->ink_list[1].range > 65535)
    dc->ink_list[1].range = 65535;
  dc->ink_list[1].value = ranges[0].value * 65535.0;
  if (dc->ink_list[1].value > 65535)
    dc->ink_list[1].value = 65535;
  dc->ink_list[1].bits = ranges[0].bit_pattern;
  if (ranges[0].bit_pattern > dc->bit_max)
    dc->bit_max = ranges[0].bit_pattern;
  dc->ranges[0].range_span = dc->ranges[0].upper->range;
  dc->ranges[0].value_span = 0;
  if (dc->nlevels > 1)
    {
      for (i = 1; i < nlevels; i++)
	{
	  int l = i + 1;
	  dc->ranges[i].lower = &dc->ink_list[i];
	  dc->ranges[i].upper = &dc->ink_list[l];

	  dc->ink_list[l].range =
	    (ranges[i].value + ranges[i].value) * 32768.0 * density;
	  if (dc->ink_list[l].range > 65535)
	    dc->ink_list[l].range = 65535;
	  dc->ink_list[l].value = ranges[i].value * 65535.0;
	  if (dc->ink_list[l].value > 65535)
	    dc->ink_list[l].value = 65535;
	  dc->ink_list[l].bits = ranges[i].bit_pattern;
	  if (ranges[i].bit_pattern > dc->bit_max)
	    dc->bit_max = ranges[i].bit_pattern;
	  dc->ranges[i].range_span =
	    dc->ink_list[l].range - dc->ink_list[i].range;
	  dc->ranges[i].value_span =
	    dc->ink_list[l].value - dc->ink_list[i].value;
	}
      dc->ranges[i].lower = &dc->ink_list[i];
      dc->ranges[i].upper = &dc->ink_list[i+1];
      dc->ink_list[i+1] = dc->ink_list[i];
      dc->ink_list[i+1].range = 65535;
      dc->ranges[i].range_span =
	dc->ink_list[i+1].range - dc->ink_list[i].range;
      dc->ranges[i].value_span =
	dc->ink_list[i+1].value - dc->ink_list[i].value;
    }
  stpi_dither_finalize_ranges(v, dc);
  stpi_flush_debug_messages(v);
}

void
stpi_dither_set_inks_simple(stp_vars_t v, int color, int nlevels,
			    const double *levels, double density)
{
  stpi_shade_t s;
  stpi_dotsize_t *d = stpi_malloc(nlevels * sizeof(stpi_dotsize_t));
  int i;
  s.dot_sizes = d;
  s.value = 65535.0;
  s.numsizes = nlevels;

  for (i = 0; i < nlevels; i++)
    {
      d[i].bit_pattern = i + 1;
      d[i].value = levels[i];
    }
  stpi_dither_set_inks_full(v, color, 1, &s, density);
  stpi_free(d);
}

void
stpi_dither_set_inks_full(stp_vars_t v, int color, int nshades,
			  const stpi_shade_t *shades, double density)
{
  int i, j;
  int idx;
  stpi_dither_channel_t *dc;
  stpi_shade_segment_t *sp;
  double k;
  stpi_ink_defn_t *ip;
  const stpi_dotsize_t *dp;

  /* Setting ink_gamma to different values changes the amount
     of photo ink used (or other lighter inks). Set to 0 it uses
     the maximum amount of ink possible without soaking the paper.
     Set to 1.0 it is very conservative.
     0.5 is probably a good compromise
  */

  const double ink_gamma = 0.5;

  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");

  stpi_channel_reset_channel(v, color);

  for (i=0; i < nshades; i++)
    {
      idx = stpi_dither_translate_channel(v, color, i);
      dc = &(CHANNEL(d, idx));

      if (dc->shades)
	{
	  for (j = 0; j < dc->numshades; j++)
	    {
	      SAFE_FREE(dc->shades[j].dotsizes);
	      SAFE_FREE(dc->shades[j].errs);
	    }
	  SAFE_FREE(dc->shades);
	}

      dc->numshades = 1;
      dc->shades = stpi_zalloc(dc->numshades * sizeof(stpi_shade_segment_t));

      sp = &dc->shades[0];
      sp->value = 1.0;
      stpi_channel_add(v, color, i, shades[i].value);
      sp->density = 65536.0;
      if (i == 0 || density == 0)
	{
	  sp->lower = 0;
	  sp->trans = 0;
	}
      else
	{
	  k = 65536.0 * density * pow(shades[i-1].value, ink_gamma);
	  sp->lower = k * shades[i-1].value + 0.5;
	  sp->trans = k * shades[i].value + 0.5;

	  /* Precompute some values */
	  sp->div1 = (sp->density * (sp->trans - sp->lower)) / sp->trans;
	  sp->div2 = (CHANNEL(d, idx - 1).shades[0].density *
		      (sp->trans - sp->lower)) / sp->lower;
	}

      sp->numdotsizes = shades[i].numsizes;
      sp->dotsizes = stpi_zalloc(sp->numdotsizes * sizeof(stpi_ink_defn_t));
      if (idx >= 0)
	stpi_dither_set_ranges(v, idx, &shades[i], density);
      for (j=0; j < sp->numdotsizes; j++)
	{
	  ip = &sp->dotsizes[j];
	  dp = &shades[i].dot_sizes[j];
	  ip->value = dp->value * sp->density + 0.5;
	  ip->range = density * ip->value;
	  ip->bits = dp->bit_pattern;
	  ip->dot_size = dp->value * 65536.0 + 0.5;
	}
    }
}

void
stpi_dither_set_inks(stp_vars_t v, int color, double density,
		     int nshades, const double *svalues,
		     int ndotsizes, const double *dvalues)
{
  int i, j;
  stpi_shade_t *shades = stpi_malloc(sizeof(stpi_shade_t) * nshades);
  stpi_dotsize_t *dotsizes = stpi_malloc(sizeof(stpi_dotsize_t) * ndotsizes);
  j = 0;
  for (i = 0; i < ndotsizes; i++)
    {
      /* Skip over any zero-valued dot sizes */
      if (dvalues[i] > 0)
	{
	  dotsizes[j].value = dvalues[i];
	  dotsizes[j].bit_pattern = i + 1;
	  j++;
	}
    }
  for (i = 0; i < nshades; i++)
    {
      shades[i].value = svalues[i];
      shades[i].numsizes = j;
      shades[i].dot_sizes = dotsizes;
    }
  stpi_dither_set_inks_full(v, color, nshades, shades, density);
  stpi_free(dotsizes);
  stpi_free(shades);
}
