/*
 * "$Id$"
 *
 *   Dither routine entrypoints
 *
 *   Copyright 2003 Robert Krawitz (rlk@alum.mit.edu)
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

typedef struct
{
  double value;
  double lower;
  double upper;
  double density;
} stpi_subchannel_t;

typedef struct
{
  unsigned subchannel_count;
  stpi_subchannel_t *sc;
  unsigned short *lut;
} stpi_channel_t;

typedef struct
{
  unsigned channel_count;
  unsigned total_channels;
  unsigned input_channels;
  stpi_channel_t *c;
  size_t width;
  unsigned short *input_data;
  unsigned short *data;
  int initialized;
} stpi_channel_group_t;


static void
clear_a_channel(stpi_channel_group_t *cg, int channel)
{
  if (channel < cg->channel_count)
    {
      SAFE_FREE(cg->c[channel].sc);
      SAFE_FREE(cg->c[channel].lut);
      cg->c[channel].subchannel_count = 0;
    }
}

static void
stpi_channel_clear(void *vc)
{
  stpi_channel_group_t *cg = (stpi_channel_group_t *) vc;
  int i;
  if (cg->channel_count > 0)
    for (i = 0; i < cg->channel_count; i++)
      clear_a_channel(cg, i);
  if (cg->data != cg->input_data)
    SAFE_FREE(cg->data);
  SAFE_FREE(cg->input_data);
  SAFE_FREE(cg->c);
  cg->channel_count = 0;
  cg->total_channels = 0;
  cg->input_channels = 0;
  cg->initialized = 0;
}

void
stpi_channel_reset(stp_vars_t v)
{
  stpi_channel_group_t *cg =
    ((stpi_channel_group_t *) stpi_get_component_data(v, "Channel"));
  if (cg)
    stpi_channel_clear(cg);
}

void
stpi_channel_reset_channel(stp_vars_t v, int channel)
{
  stpi_channel_group_t *cg =
    ((stpi_channel_group_t *) stpi_get_component_data(v, "Channel"));
  if (cg)
    clear_a_channel(cg, channel);
}

static void
stpi_channel_free(void *vc)
{
  stpi_channel_clear(vc);
  stpi_free(vc);
}

void
stpi_channel_add(stp_vars_t v, unsigned channel, unsigned subchannel,
		 double value, double density)
{
  stpi_channel_group_t *cg =
    ((stpi_channel_group_t *) stpi_get_component_data(v, "Channel"));
  stpi_channel_t *chan;
  if (!cg)
    {
      cg = stpi_zalloc(sizeof(stpi_channel_group_t));
      stpi_allocate_component_data(v, "Channel", NULL, stpi_channel_free, cg);
    }				   
  if (channel >= cg->channel_count)
    {
      unsigned oc = cg->channel_count;
      cg->c = stpi_realloc(cg->c, sizeof(stpi_channel_t) * (channel + 1));
      memset(cg->c + oc, 0, sizeof(stpi_channel_t) * (channel + 1 - oc));
      if (channel >= cg->channel_count)
	cg->channel_count = channel + 1;
    }
  chan = cg->c + channel;
  if (subchannel >= chan->subchannel_count)
    {
      unsigned oc = chan->subchannel_count;
      chan->sc =
	stpi_realloc(chan->sc, sizeof(stpi_subchannel_t) * (subchannel + 1));
      (void) memset
	(chan->sc + oc, 0, sizeof(stpi_subchannel_t) * (subchannel + 1 - oc));
      chan->sc[subchannel].value = value;
      if (subchannel >= chan->subchannel_count)
	chan->subchannel_count = subchannel + 1;
    }
  chan->sc[subchannel].value = value;
  chan->sc[subchannel].density = density;
}

static int
input_needs_splitting(stp_const_vars_t v)
{
  const stpi_channel_group_t *cg =
    ((const stpi_channel_group_t *) stpi_get_component_data(v, "Channel"));
#if 1
  return cg->total_channels != cg->input_channels;
#else
  int i;
  if (!cg || cg->channel_count <= 0)
    return 0;
  for (i = 0; i < cg->channel_count; i++)
    {
      if (cg->c[i].subchannel_count > 1)
	return 1;
    }
  return 0;
#endif
}

void
stpi_channel_initialize(stp_vars_t v, stp_image_t *image,
			int input_channel_count)
{
  stpi_channel_group_t *cg =
    ((stpi_channel_group_t *) stpi_get_component_data(v, "Channel"));
  int width = stpi_image_width(image);
  int i;
  if (!cg)
    {
      cg = stpi_zalloc(sizeof(stpi_channel_group_t));
      stpi_allocate_component_data(v, "Channel", NULL, stpi_channel_free, cg);
    }				   
  if (cg->initialized)
    return;
  cg->initialized = 1;
  for (i = 0; i < cg->channel_count; i++)
    {
      stpi_channel_t *c = &(cg->c[i]);
      int sc = c->subchannel_count;
      if (sc > 1)
	{
	  int k;
	  int val = 0;
	  int next_breakpoint;
	  c->lut = stpi_zalloc(sizeof(unsigned short) * sc * 65536);
	  next_breakpoint = c->sc[0].value * 65535 * c->sc[0].density / 2;
	  if (next_breakpoint > 65535)
	    next_breakpoint = 65535;
	  while (val <= next_breakpoint)
	    {
	      int value = (int) ((double) val / c->sc[0].value);
	      c->lut[val * sc + sc - 1] = value;
	      val++;
	    }

	  for (k = 0; k < sc - 1; k++)
	    {
	      double this_val = c->sc[k].value;
	      double next_val = c->sc[k + 1].value;
	      double this_density = c->sc[k].density;
	      double next_density = c->sc[k + 1].density;
	      double upper = 1.0;
	      double lower = 1.0;
	      int range;
	      int base = val;
	      double density = sqrt(this_density * next_density);
	      next_breakpoint = (this_val + next_val) * 65535 * density / 2;
	      if (next_breakpoint > 65535)
		next_breakpoint = 65535;
	      range = next_breakpoint - val;
	      upper = 1.0 / next_val;
	      lower = 1.0 / this_val;
	      while (val <= next_breakpoint)
		{
		  double where = ((double) val - base) / (double) range;
		  c->lut[val * sc + sc - k - 2] = val * where * upper;
		  c->lut[val * sc + sc - k - 1] = val * (1.0 - where) * lower;
		  val++;
		}
	    }
	  while (val <= 65535)
	    {
	      c->lut[val * sc] = val / c->sc[sc - 1].value;
	      val++;
	    }
	}     
      cg->total_channels += c->subchannel_count;
    }
  cg->input_channels = input_channel_count;
  cg->width = width;
  cg->data = stpi_malloc(sizeof(unsigned short) * cg->total_channels * width);
  if (!input_needs_splitting(v))
    {
      cg->input_data = cg->data;
      return;
    }
  cg->input_data =
    stpi_malloc(sizeof(unsigned short) * cg->input_channels * width);
}

void
stpi_channel_convert(stp_const_vars_t v, unsigned *zero_mask)
{
  stpi_channel_group_t *cg =
    ((stpi_channel_group_t *) stpi_get_component_data(v, "Channel"));
  const unsigned short *input;
  unsigned short *output;
  int i;
  int nz[32];
  if (!input_needs_splitting(v))
    return;
  memset(nz, 0, sizeof(nz));
  input = cg->input_data;
  output = cg->data;
  for (i = 0; i < cg->width; i++)
    {
      int zero_ptr = 0;
      int j;
      for (j = 0; j < cg->channel_count; j++)
	{
	  stpi_channel_t *c = &(cg->c[j]);
	  int s_count = c->subchannel_count;
	  if (s_count == 1)
	    nz[zero_ptr++] |= *(output++) = *(input);
	  else if (s_count > 1)
	    {
	      int k;
	      for (k = 0; k < s_count; k++)
		nz[zero_ptr++] |= (*output++) =
		  c->lut[((*input) * s_count) + k];
	    }
	  input++;
	}
    }
  if (zero_mask)
    {
      *zero_mask = 0;
      for (i = 0; i < 32; i++)
	{
	  if (nz[i] == 0)
	    *zero_mask |= 1 << i;
	}
    }
}

unsigned short *
stpi_channel_get_input(stp_const_vars_t v)
{
  stpi_channel_group_t *cg =
    ((stpi_channel_group_t *) stpi_get_component_data(v, "Channel"));
  return (unsigned short *) cg->input_data;
}

unsigned short *
stpi_channel_get_output(stp_const_vars_t v)
{
  stpi_channel_group_t *cg =
    ((stpi_channel_group_t *) stpi_get_component_data(v, "Channel"));
  return cg->data;
}
