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

static void
stpi_dither_finalize_ranges(stp_vars_t v, stpi_dither_channel_t *s)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
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

      stpi_dprintf(STPI_DBG_INK, v,
		  "    level %d value[0] %d value[1] %d range[0] %d range[1] %d\n",
		  i, s->ranges[i].lower->value, s->ranges[i].upper->value,
		  s->ranges[i].lower->range, s->ranges[i].upper->range);
      stpi_dprintf(STPI_DBG_INK, v,
		  "    xvalue[0] %d xvalue[1] %d\n",
		  s->ranges[i].lower->xvalue, s->ranges[i].upper->xvalue);
      stpi_dprintf(STPI_DBG_INK, v,
		  "       bits[0] %d bits[1] %d subchannel[0] %d subchannel[1] %d\n",
		  s->ranges[i].lower->bits, s->ranges[i].upper->bits,
		  s->ranges[i].lower->subchannel, s->ranges[i].upper->subchannel);
      stpi_dprintf(STPI_DBG_INK, v,
		  "       rangespan %d valuespan %d same_ink %d equal %d\n",
		  s->ranges[i].range_span, s->ranges[i].value_span,
		  s->ranges[i].is_same_ink, s->ranges[i].is_equal);
      if (i > 0 && s->ranges[i].lower->range >= d->adaptive_limit)
	{
	  d->adaptive_limit = s->ranges[i].lower->range + 1;
	  if (d->adaptive_limit > 65535)
	    d->adaptive_limit = 65535;
	  stpi_dprintf(STPI_DBG_INK, v, "Setting adaptive limit to %d\n",
		      d->adaptive_limit);
	}
    }
  if (s->nlevels == 1 && s->ranges[0].upper->bits == 1 &&
      s->ranges[0].upper->subchannel == 0)
    s->very_fast = 1;
  else
    s->very_fast = 0;

  s->subchannels = max_subchannel + 1;
  s->row_ends[0] = stpi_zalloc(s->subchannels * sizeof(int));
  s->row_ends[1] = stpi_zalloc(s->subchannels * sizeof(int));
  s->ptrs = stpi_zalloc(s->subchannels * sizeof(char *));
  stpi_dprintf(STPI_DBG_INK, v,
	      "  bit_max %d signif_bits %d\n", s->bit_max, s->signif_bits);
}

static void
stpi_dither_set_generic_ranges(stp_vars_t v, stpi_dither_channel_t *s, int nlevels,
			      const stpi_dither_range_simple_t *ranges,
			      double density)
{
  double sdensity = s->density_adjustment;
  int i;
  SAFE_FREE(s->ranges);
  SAFE_FREE(s->row_ends[0]);
  SAFE_FREE(s->row_ends[1]);
  SAFE_FREE(s->ptrs);
  SAFE_FREE(s->ink_list);

  s->nlevels = nlevels > 1 ? nlevels + 1 : nlevels;
  s->ranges = (stpi_dither_segment_t *)
    stpi_zalloc(s->nlevels * sizeof(stpi_dither_segment_t));
  s->ink_list = (stpi_ink_defn_t *)
    stpi_zalloc((s->nlevels + 1) * sizeof(stpi_ink_defn_t));
  s->bit_max = 0;
  density *= sdensity;
  s->density = density * 65535;
  stpi_init_debug_messages(v);
  stpi_dprintf(STPI_DBG_INK, v,
	      "stpi_dither_set_generic_ranges nlevels %d density %f\n",
	      nlevels, density);
  for (i = 0; i < nlevels; i++)
    stpi_dprintf(STPI_DBG_INK, v,
		"  level %d value %f pattern %x subchannel %d\n", i,
		ranges[i].value, ranges[i].bit_pattern, ranges[i].subchannel);
  s->ranges[0].lower = &s->ink_list[0];
  s->ranges[0].upper = &s->ink_list[1];
  s->ink_list[0].range = 0;
  s->ink_list[0].value = ranges[0].value * 65535.0;
  s->ink_list[0].xvalue = ranges[0].value * 65535.0 * sdensity;
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
  s->ink_list[1].xvalue = ranges[0].value * 65535.0 * sdensity;
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
	  s->ink_list[l].xvalue = ranges[i].value * 65535.0 * sdensity;
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
  stpi_dither_finalize_ranges(v, s);
  stpi_flush_debug_messages(v);
}

static void
stpi_dither_set_generic_ranges_full(stp_vars_t v, stpi_dither_channel_t *s,
				   int nlevels,
				   const stpi_dither_range_full_t *ranges,
				   double density)
{
  double sdensity = s->density_adjustment;
  int i, j, k;
  SAFE_FREE(s->ranges);
  SAFE_FREE(s->row_ends[0]);
  SAFE_FREE(s->row_ends[1]);
  SAFE_FREE(s->ptrs);
  SAFE_FREE(s->ink_list);

  s->nlevels = nlevels+1;
  s->ranges = (stpi_dither_segment_t *)
    stpi_zalloc(s->nlevels * sizeof(stpi_dither_segment_t));
  s->ink_list = (stpi_ink_defn_t *)
    stpi_zalloc((s->nlevels * 2) * sizeof(stpi_ink_defn_t));
  s->bit_max = 0;
  density *= sdensity;
  s->density = density * 65535;
  stpi_init_debug_messages(v);
  stpi_dprintf(STPI_DBG_INK, v,
	      "stpi_dither_set_ranges nlevels %d density %f\n",
	      nlevels, density);
  for (i = 0; i < nlevels; i++)
    stpi_dprintf(STPI_DBG_INK, v,
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
	  s->ink_list[2*j+k].xvalue = ranges[i].value[k] * 65535 * sdensity;
	  s->ink_list[2*j+k].range = s->ink_list[2 * j + k].value * density;
	  s->ink_list[2*j+k].bits = ranges[i].bits[k];
	  s->ink_list[2*j+k].subchannel = ranges[i].subchannel[k];
	}
      s->ranges[j].lower = &s->ink_list[2*j];
      s->ranges[j].upper = &s->ink_list[2*j+1];
      s->ranges[j].range_span =
	s->ranges[j].upper->range - s->ranges[j].lower->range;
      s->ranges[j].value_span =
	s->ranges[j].upper->value - s->ranges[j].lower->value;
      j++;
    }
  s->ink_list[2*j] = s->ink_list[2*(j-1)+1];
  s->ink_list[2*j+1] = s->ink_list[2*j];
  s->ink_list[2*j+1].range = 65535;
  s->ink_list[2*j+1].value = 65535;	/* ??? Is this correct ??? */
  s->ink_list[2*j+1].xvalue = 65535 * sdensity;	/* ??? Is this correct ??? */
  s->ranges[j].lower = &s->ink_list[2*j];
  s->ranges[j].upper = &s->ink_list[2*j+1];
  s->ranges[j].range_span =
    s->ranges[j].upper->range - s->ranges[j].lower->range;
  s->ranges[j].value_span = 0;
  s->nlevels = j+1;
  stpi_dither_finalize_ranges(v, s);
  stpi_flush_debug_messages(v);
}

void
stpi_dither_set_ranges(stp_vars_t v, int color, int nlevels,
		      const stpi_dither_range_simple_t *ranges, double density)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  if (color < 0 || color >= PHYSICAL_CHANNEL_COUNT(d))
    return;
  stpi_dither_set_generic_ranges(v, &(PHYSICAL_CHANNEL(d, color)), nlevels,
				ranges, density);
}

void
stpi_dither_set_ranges_and_shades_simple(stp_vars_t v, int color, int nlevels,
					 const double *levels, double density)
{
  stpi_dither_range_simple_t *r =
    stpi_malloc(nlevels * sizeof(stpi_dither_range_simple_t));
  stpi_shade_t s;
  stpi_dotsize_t *d = stpi_malloc(nlevels * sizeof(stpi_dotsize_t));
  int i;
  s.dot_sizes = d;
  s.subchannel = 0;
  s.value = 65535.0;
  s.numsizes = nlevels;

  for (i = 0; i < nlevels; i++)
    {
      r[i].bit_pattern = i + 1;
      r[i].dot_size = i + 1;
      r[i].value = levels[i];
      r[i].subchannel = 0;
      d[i].bit_pattern = i + 1;
      d[i].value = levels[i];
    }
  stpi_dither_set_ranges(v, color, nlevels, r, density);
  stpi_dither_set_shades(v, color, 1, &s, density);
  stpi_free(r);
  stpi_free(d);
}

void
stpi_dither_set_ranges_full(stp_vars_t v, int color, int nlevels,
			   const stpi_dither_range_full_t *ranges,
			   double density)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  stpi_dither_set_generic_ranges_full(v, &(PHYSICAL_CHANNEL(d, color)), nlevels,
				     ranges, density);
}

void
stpi_dither_set_shades(stp_vars_t v, int color, int nshades,
		      const stpi_shade_t *shades, double density)
{
  int i, j;

  /* Setting ink_gamma to different values changes the amount
     of photo ink used (or other lighter inks). Set to 0 it uses
     the maximum amount of ink possible without soaking the paper.
     Set to 1.0 it is very conservative.
     0.5 is probably a good compromise
   */

  const double ink_gamma = 0.5;

  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  stpi_dither_channel_t *dc = &(PHYSICAL_CHANNEL(d, color));

  if (dc->shades) {
    for (i = 0; i < dc->numshades; i++) {
      SAFE_FREE(dc->shades[i].dotsizes);
      SAFE_FREE(dc->shades[i].errs);
    }
    SAFE_FREE(dc->shades);
  }

  dc->numshades = nshades;
  dc->shades = stpi_zalloc(nshades * sizeof(stpi_shade_segment_t));

  for (i=0;i<dc->numshades;i++) {
    stpi_shade_segment_t *sp = &dc->shades[i];
    sp->subchannel = shades[i].subchannel;
    sp->value = 0;
    sp->density = 65536.0 * shades[i].value + 0.5;
    if (i == 0) {
      sp->lower = 0;
      sp->trans = 0;
    } else {
      double k;
      k = 65536.0 * density * pow(shades[i-1].value, ink_gamma);
      sp->lower = k * shades[i-1].value + 0.5;
      sp->trans = k * shades[i].value + 0.5;

      /* Precompute some values */
      sp->div1 = (sp->density * (sp->trans - sp->lower)) / sp->trans;
      sp->div2 = (sp[-1].density * (sp->trans - sp->lower)) / sp->lower;
    }

    sp->numdotsizes = shades[i].numsizes;
    sp->dotsizes = stpi_zalloc(sp->numdotsizes * sizeof(stpi_ink_defn_t));

    for (j=0; j < sp->numdotsizes; j++) {
      stpi_ink_defn_t *ip = &sp->dotsizes[j];
      const stpi_dotsize_t *dp = &shades[i].dot_sizes[j];
      ip->value = dp->value * sp->density + 0.5;
      ip->range = density * ip->value;
      ip->bits = dp->bit_pattern;
      ip->subchannel = shades[i].subchannel;
      ip->dot_size = dp->value * 65536.0 + 0.5;
    }
  }
}
