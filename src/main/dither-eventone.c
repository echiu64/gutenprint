/*
 * "$Id$"
 *
 *   EvenTone dither implementation for Gimp-Print
 *
 *   Copyright 2002-2003 Mark Tomlinson (mark@southern.co.nz)
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
 *   This code uses the Eventone dither algorithm. This is described
 *   at the website http://www.artofcode.com/eventone/
 *   This algorithm is covered by US Patents 5,055,942 and 5,917,614
 *   and was invented by Raph Levien <raph@acm.org>
 *   It was made available to be used free of charge in GPL-licensed
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

typedef struct
{
  int	d2x;
  int   d2y;
  stpi_dis_t	d_sq;
  int	aspect;
} eventone_t;


static inline void
print_subc(stpi_dither_t *d, unsigned char *tptr, stpi_ink_defn_t *ink, int subchannel, unsigned char bit, int length)
{
  int bits;
  int j;

  if (tptr != 0)
    {
      switch(bits = ink->bits)
	{
	case 1:
	  tptr[d->ptr_offset] |= bit;
	  return;
	case 2:
	  tptr[d->ptr_offset + length] |= bit;
	  return;
	default:
	  tptr = &tptr[d->ptr_offset];
	  for (j=1; j <= bits; j+=j, tptr += length) {
	    if (j & bits) *tptr |= bit;
	  }
	  return;
	}
    }
}

#define EVEN_C1 256
#define EVEN_C2 222		/* = sqrt(3)/2 * EVEN_C1 */

static void
free_eventone_data(stpi_dither_t *d)
{
  eventone_t *et = (eventone_t *) d->aux_data;
  if (et)
    {
      stpi_free(et);
      d->aux_data = NULL;
    }
}

static int
et_initializer(stpi_dither_t *d, int duplicate_line, int zero_mask)
{
  eventone_t *et = (eventone_t *) d->aux_data;

  if (!et) {
    int i,j;
    for (i = 0; i < CHANNEL_COUNT(d); i++) {
      int size = 2 * MAX_SPREAD + ((d->dst_width + 7) & ~7);
      for (j = 0; j < CHANNEL(d, i).numshades; j++) {
        CHANNEL(d, i).shades[j].errs = stpi_zalloc(size * sizeof(int));
      }
      /* Use ".b" for the density scaler. */
      CHANNEL(d, i).b = 65536 * CHANNEL(d, i).density_adjustment;
    }

    et = stpi_zalloc(sizeof(eventone_t));

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

    for (i = 0; i < CHANNEL_COUNT(d); i++) {
      for (j = 0; j < CHANNEL(d, i).numshades; j++) {
        int x;
	CHANNEL(d, i).shades[j].dis = et->d_sq;
	CHANNEL(d, i).shades[j].et_dis = stpi_malloc(sizeof(stpi_dis_t) * d->dst_width);
	for (x = 0; x < d->dst_width; x++) {
	  CHANNEL(d, i).shades[j].et_dis[x] = et->d_sq;
	}
      }
    }
    d->aux_data = et;
    d->aux_freefunc = free_eventone_data;
  }

  if (!duplicate_line) {
    if ((zero_mask & ((1 << d->n_input_channels) - 1)) !=
	((1 << d->n_input_channels) - 1)) {
	d->last_line_was_empty = 0;
    } else {
	d->last_line_was_empty++;
    }
  } else if (d->last_line_was_empty) {
    d->last_line_was_empty++;
  }

  if (d->last_line_was_empty >= 5) {
    return 0;
  } else if (d->last_line_was_empty == 4) {
      int i,j;
      for (i = 0; i < CHANNEL_COUNT(d); i++)
	for (j = 0; j < CHANNEL(d, i).numshades; j++)
	  memset(&CHANNEL(d, i).shades[j].errs[MAX_SPREAD], 0, d->dst_width * sizeof(int));
      return 0;
  }
  return 1;
}

static inline stpi_shade_segment_t *
split_shades(stpi_dither_channel_t *dc, int x, int *inkp)
{
  int i;
  stpi_shade_segment_t *sp, *sp2;
  int totalink;
  int maxv;
  int orig, nextv;

  if (dc->numshades == 1) {			/* Make single shade more efficient */
    sp = &dc->shades[0];
    if (dc->v == 0) {				/* Special case zero */
      sp->base = 0;
      *inkp = sp->value += sp->errs[x + MAX_SPREAD];
      return sp;
    }
    sp->base = (dc->v * dc->b) / sp->density;
    sp->value += 2 * sp->base + sp->errs[x + MAX_SPREAD];
    *inkp = sp->value - sp->base;
    return sp;
  }

  if (dc->v == 0) {				/* Special case zero */
    sp2 = &dc->shades[0];
    totalink = 0;
    maxv = 0;
    for (i = 0; i < dc->numshades; i++) {
      int value;
      sp = &dc->shades[i];
      sp->base = 0;
      value = (sp->value += sp->errs[x + MAX_SPREAD]);
      totalink += value;
      if (value > maxv) {
        maxv = value;
	sp2 = sp;
      }
    }
    *inkp = totalink;
    return sp2;
  }


  /* General case for many sub-channels follows now */

  totalink = 0;
  maxv = 0;
  nextv = 0;
  orig = dc->o;
  sp2 = &dc->shades[0];

  for (i = dc->numshades - 1; i >= 0; i--) {
    int value;
    sp = &dc->shades[i];

    sp->base = nextv;
    nextv = 0;
    if (orig > sp->lower) {
      if (sp->trans == 0 || orig >= sp->trans) {
        sp->base = (dc->v * dc->b) / sp->density;
      } else {
        sp->base = ((orig - sp->lower) * dc->b) / sp->div1;
	nextv = ((sp->trans - orig) * dc->b) / sp->div2;
	if (orig != dc->v) {
	  sp->base = (sp->base * dc->v) / orig;
	  nextv = (nextv * dc->v) / orig;
	}
      }
      orig = 0;
    }
    sp->value += 2 * sp->base + sp->errs[x + MAX_SPREAD];
    value = sp->value - sp->base;
    totalink += value;
    if (value > maxv) {
      maxv = value;
      sp2 = sp;
    }
  }

  /* Return the subchannel we will be printing */
  *inkp = totalink;
  return sp2;
}

static inline void
advance_eventone_pre(stpi_dither_channel_t *dc, eventone_t *et, int x)
{
  int i;

  for (i = 0; i < dc->numshades; i++) {
    stpi_shade_segment_t *sp = &dc->shades[i];
    stpi_dis_t *etd = &sp->et_dis[x];
    int t = sp->dis.r_sq + sp->dis.dx;
    if (t <= etd->r_sq) { 				/* Do eventone calculations */
      sp->dis.r_sq = t;					/* Nearest pixel same as last one */
      sp->dis.dx += et->d2x;
    } else {
      sp->dis = *etd;					/* Nearest pixel is from a previous line */
    }
  }
}

static inline void
diffuse_error(stpi_dither_channel_t *dc, eventone_t *et, int diff_factor, int x, int direction)
{
  int i;

  for (i = 0; i < dc->numshades; i++) {
    stpi_shade_segment_t *sp = &dc->shades[i];

    /* Eventone updates */

    { stpi_dis_t *etd = &sp->et_dis[x];
      int t = etd->r_sq + etd->dy;		/* r^2 from dot above */
      int u = sp->dis.r_sq + sp->dis.dy;	/* r^2 from dot on this line */
      if (u < t) {				/* If dot from this line is close */
        t = u;					/* Use it instead */
        etd->dx = sp->dis.dx;
        etd->dy = sp->dis.dy;
      }
      etd->dy += et->d2y;

      if (t > 65535) {				/* Do some hard limiting */
        t = 65535;
      }
      etd->r_sq = t;
    }

    /* Error diffusion updates */

    { int fraction = (sp->value + (diff_factor>>1)) / diff_factor;
      int frac_2 = fraction + fraction;
      int frac_3 = frac_2 + fraction;
      sp->errs[x + MAX_SPREAD] = frac_3;
      sp->errs[x + MAX_SPREAD - direction] += frac_2;
      sp->value -= (frac_2 + frac_3);
    }
  }
}

static inline int
eventone_adjust(stpi_shade_segment_t *sp, eventone_t *et, int dither_point, unsigned int desired, unsigned int dotsize)
{
  if (desired == 0) {
    dither_point = 0;
  } else {
    dither_point += sp->dis.r_sq * et->aspect;
    if (desired < dotsize) {
      dither_point -= (EVEN_C1 * dotsize) / desired;
    }
    if (dither_point > 65535) dither_point = 65535;
    else if (dither_point < 0) dither_point = 0;
  }
  return dither_point;
}

static inline int
find_segment(stpi_shade_segment_t *sp, eventone_t *et, int totalink, unsigned int baseink, stpi_ink_defn_t *lower, stpi_ink_defn_t *upper)
{
  lower->range = 0;
  lower->bits = 0;
  if (totalink < 0) totalink = 0;

  { int i;
    stpi_ink_defn_t *ip;

    for (i=0, ip = &sp->dotsizes[0]; i < sp->numdotsizes - 1; i++, ip++) {
      if (ip->dot_size <= totalink) {
        lower->bits = ip->bits;
        lower->range = ip->dot_size;
      } else {
        upper->bits = ip->bits;
        upper->range = ip->dot_size;
        goto found_segment;
      }
    }

    upper->bits = ip->bits;
    upper->range = ip->dot_size;
  }

found_segment:

  { int dither_point;
    if (totalink >= upper->range) {
      dither_point = 65536;
    } else if (totalink <= lower->range) {
      dither_point = 0;
    } else {
      if (lower->range == 0) {
        dither_point = eventone_adjust(sp, et, (totalink * 65536) / upper->range, baseink, upper->range);
      } else {
        dither_point = ((totalink - lower->range) * 65536) / (upper->range - lower->range);
      }
    }
    return dither_point;
  }
}

static void
stpi_dither_raw_et(stpi_dither_t *d,
		  int row,
		  const unsigned short *raw,
		  int duplicate_line,
		  int zero_mask)
{
  eventone_t *et;
  static const int diff_factors[] = {1, 10, 16, 23, 32};

  int		x,
	        length;
  unsigned char	bit;
  int		i;

  int		terminate;
  int		direction;
  int		xerror, xstep, xmod;
  int		aspect = d->y_aspect / d->x_aspect;
  int		diff_factor;
  int		range;
  int		channel_count = CHANNEL_COUNT(d);

  if (!et_initializer(d, duplicate_line, zero_mask)) return;

  et = (eventone_t *) d->aux_data;

  if (aspect >= 4) { aspect = 4; }
  else if (aspect >= 2) { aspect = 2; }
  else aspect = 1;

  diff_factor = diff_factors[aspect];
  length = (d->dst_width + 7) / 8;

  if (row & 1) {
    direction = 1;
    x = 0;
    terminate = d->dst_width;
    d->ptr_offset = 0;
  } else {
    direction = -1;
    x = d->dst_width - 1;
    terminate = -1;
    d->ptr_offset = length - 1;
    raw += channel_count * (d->src_width - 1);
  }
  bit = 1 << (7 - (x & 7));
  xstep  = channel_count * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;

  for (; x != terminate; x += direction) {

    range = 0;

    for (i=0; i < channel_count; i++) {
      int inkspot;
      stpi_shade_segment_t *sp;
      stpi_dither_channel_t *dc = &CHANNEL(d, i);
      stpi_ink_defn_t *inkp;
      stpi_ink_defn_t lower, upper;

      dc->o = dc->v = raw[i];

      advance_eventone_pre(dc, et, x);

      /* Split data into sub-channels */
      /* And incorporate error data from previous line */
      sp =  split_shades(dc, x, &inkspot);

      /* Find which are the two candidate dot sizes */
      range += find_segment(sp, et, inkspot, sp->base, &lower, &upper);

      /* Determine whether to print the larger or smaller dot */

      inkp = &lower;
      if (range >= 32768) {
        range -= 65536;
        inkp = &upper;
      }

      /* Adjust the error to reflect the dot choice */
      if (inkp->bits) {
        int subc = sp->subchannel;

        sp->value -= 2 * inkp->range;
        sp->dis = et->d_sq;

        set_row_ends(dc, x, subc);

        /* Do the printing */
        print_subc(d, dc->ptrs[subc], inkp, subc, bit, length);
      }

      /* Spread the error around to the adjacent dots */
      diffuse_error(dc, et, diff_factor, x, direction);
    }
    if (direction == 1)
      ADVANCE_UNIDIRECTIONAL(d, bit, raw, channel_count, xerror, xstep, xmod);
    else
      ADVANCE_REVERSE(d, bit, raw, channel_count, xerror, xstep, xmod);
  }
  if (direction == -1)
    stpi_dither_reverse_row_ends(d);
}

static void
stpi_dither_raw_cmyk_et(stpi_dither_t *d,
		       int row,
		       const unsigned short *cmyk,
		       int duplicate_line,
		       int zero_mask)
{
  eventone_t *et;
  static const int diff_factors[] = {1, 10, 16, 23, 32};

  int		x,
	        length;
  unsigned char	bit;
  int		i;

  int		terminate;
  int		direction;
  int		xerror, xstep, xmod;
  int		aspect = d->y_aspect / d->x_aspect;
  int		diff_factor;
  int		range;
  int		channel_count = CHANNEL_COUNT(d);

  if (!et_initializer(d, duplicate_line, zero_mask)) return;

  et = (eventone_t *) d->aux_data;

  if (aspect >= 4) { aspect = 4; }
  else if (aspect >= 2) { aspect = 2; }
  else aspect = 1;

  diff_factor = diff_factors[aspect];
  length = (d->dst_width + 7) / 8;

  if (row & 1) {
    direction = 1;
    x = 0;
    terminate = d->dst_width;
    d->ptr_offset = 0;
  } else {
    direction = -1;
    x = d->dst_width - 1;
    terminate = -1;
    d->ptr_offset = length - 1;
    cmyk += 4 * (d->src_width - 1);
  }
  bit = 1 << (7 - (x & 7));
  xstep  = 4 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  xerror = (xmod * x) % d->dst_width;

  for (; x != terminate; x += direction) {
    { int black = cmyk[3];
      CHANNEL(d, ECOLOR_K).v =
      CHANNEL(d, ECOLOR_K).o = black;

      for (i=1; i < channel_count; i++) {
        CHANNEL(d, i).o = black +
        (CHANNEL(d, i).v = cmyk[i-1]);
      }
    }

    /* At this point, the CMYK separation has been done */
    /* And the results are in CHANNEL(d, i).v */

    range = 0;

    for (i = 0; i < channel_count; i++) {
      int inkspot;
      stpi_shade_segment_t *sp;
      stpi_dither_channel_t *dc = &CHANNEL(d, i);
      stpi_ink_defn_t *inkp;
      stpi_ink_defn_t lower, upper;

      advance_eventone_pre(dc, et, x);

      /* Split data into sub-channels */
      /* And incorporate error data from previous line */
      sp =  split_shades(dc, x, &inkspot);

      /* Find which are the two candidate dot sizes */
      range += find_segment(sp, et, inkspot, sp->base, &lower, &upper);

      /* Determine whether to print the larger or smaller dot */
      inkp = &lower;
      if (range >= 32768) {
        range -= 65536;
        inkp = &upper;
      }

      /* Adjust the error to reflect the dot choice */
      if (inkp->bits) {
        int subc = sp->subchannel;
        sp->value -= 2 * inkp->range;
        sp->dis = et->d_sq;

        set_row_ends(dc, x, subc);

        /* Do the printing */
        print_subc(d, dc->ptrs[subc], inkp, subc, bit, length);
      }

      /* Spread the error around to the adjacent dots */
      diffuse_error(dc, et, diff_factor, x, direction);
    }
    if (direction == 1)
      ADVANCE_UNIDIRECTIONAL(d, bit, cmyk, 4, xerror, xstep, xmod);
    else
      ADVANCE_REVERSE(d, bit, cmyk, 4, xerror, xstep, xmod);
  }
  if (direction == -1)
    stpi_dither_reverse_row_ends(d);
}

void
stpi_dither_et(stp_vars_t v,
	      int row,
	      const unsigned short *input,
	      int duplicate_line,
	      int zero_mask)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  if (CHANNEL(d, 0).shades == 0)
    stpi_dither_ed(v, row, input, duplicate_line, zero_mask);
  else if (d->dither_class != OUTPUT_RAW_CMYK ||
	   d->n_ghost_channels > 0)
    stpi_dither_raw_et(d, row, input, duplicate_line, zero_mask);
  else
    stpi_dither_raw_cmyk_et(d, row, input, duplicate_line, zero_mask);
}
