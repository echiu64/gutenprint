/*
 * "$Id$"
 *
 *   Dither routine entrypoints
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
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <math.h>
#include <string.h>
#include "array.h"
#include "sequence.h"
#include "dither-impl.h"

static const stpi_dither_algorithm_t dither_algos[] =
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

static const int num_dither_algos = sizeof(dither_algos)/sizeof(stpi_dither_algorithm_t);


/*
 * Bayer's dither matrix using Judice, Jarvis, and Ninke recurrence relation
 * http://www.cs.rit.edu/~sxc7922/Project/CRT.htm
 */

static const unsigned sq2[] =
{
  0, 2,
  3, 1
};

static const stp_parameter_t dither_parameters[] =
{
  {
    "Density", N_("Density"),
    N_("Adjust the density (amount of ink) of the print. "
       "Reduce the density if the ink bleeds through the "
       "paper or smears; increase the density if black "
       "regions are not solid."),
    STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED2, 1, 1, -1
  },
  {
    "DitherAlgorithm", N_("Dither Algorithm"),
    N_("Choose the dither algorithm to be used.\n"
       "Adaptive Hybrid usually produces the best all-around quality.\n"
       "EvenTone is a new, experimental algorithm that often produces excellent results.\n"
       "Ordered is faster and produces almost as good quality on photographs.\n"
       "Fast and Very Fast are considerably faster, and work well for text and line art.\n"
       "Hybrid Floyd-Steinberg generally produces inferior output."),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED2, 1, 1, -1
  },
};

static int dither_parameter_count =
sizeof(dither_parameters) / sizeof(const stp_parameter_t);

stp_parameter_list_t
stpi_dither_list_parameters(const stp_vars_t v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < dither_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(dither_parameters[i]));
  return ret;
}

void
stpi_dither_describe_parameter(const stp_vars_t v, const char *name,
			       stp_parameter_t *description)
{
  int i;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;
  description->deflt.str = NULL;
  if (strcmp(name, "DitherAlgorithm") == 0)
    {
      stpi_fill_parameter_settings(description, &(dither_parameters[1]));
      description->bounds.str = stp_string_list_create();
      for (i = 0; i < num_dither_algos; i++)
	{
	  const stpi_dither_algorithm_t *dt = &dither_algos[i];
	  stp_string_list_add_string(description->bounds.str,
				    dt->name, dt->text);
	}
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "Density") == 0)
    {
      stpi_fill_parameter_settings(description, &(dither_parameters[0]));
      description->bounds.dbl.upper = 8.0;
      description->bounds.dbl.lower = 0.1;
      description->deflt.dbl = 1.0;
    }
}

unsigned char *
stpi_dither_get_channel(stp_vars_t v, unsigned channel, unsigned subchannel)
{
  stpi_dither_data_t *d = &(((stpi_dither_t *) stpi_get_dither_data(v))->dt);
  stpi_dither_channel_data_t *chan;
  if (channel >= d->channel_count)
    return NULL;
  chan = d->c + channel;
  if (subchannel >= chan->subchannel_count)
    return NULL;
  return chan->c[subchannel];
}

void
stpi_dither_add_channel(stp_vars_t v, unsigned char *data,
			unsigned channel, unsigned subchannel)
{
  stpi_dither_data_t *d = &(((stpi_dither_t *) stpi_get_dither_data(v))->dt);
  stpi_dither_channel_data_t *chan;
  if (channel >= d->channel_count)
    {
      unsigned oc = d->channel_count;
      d->c = stpi_realloc
	(d->c, sizeof(stpi_dither_channel_data_t) * (channel + 1));
      (void) memset
	(d->c + oc, 0, sizeof(stpi_dither_channel_data_t) * (channel + 1- oc));
      d->channel_count = channel + 1;
    }
  chan = d->c + channel;
  if (subchannel >= chan->subchannel_count)
    {
      unsigned oc = chan->subchannel_count;
      chan->c =
	stpi_realloc(chan->c, sizeof(unsigned char *) * (subchannel + 1));
      (void) memset
	(chan->c + oc, 0, sizeof(unsigned char *) * (subchannel + 1 - oc));
      chan->subchannel_count = subchannel + 1;
    }
  chan->c[subchannel] = data;
}

#define RETURN_DITHERFUNC(func, v)					\
do									\
{									\
  stpi_dprintf(STPI_DBG_COLORFUNC, v, "ditherfunc %s\n", #func);	\
  return (func);							\
} while (0)

static stpi_ditherfunc_t *
stpi_set_dither_function(stp_vars_t v, int image_bpp)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
  int i;
  const char *algorithm = stp_get_string_parameter(v, "DitherAlgorithm");
  d->stpi_dither_type = D_ADAPTIVE_HYBRID;
  if (algorithm)
    {
      for (i = 0; i < num_dither_algos; i++)
	{
	  if (!strcmp(algorithm, _(dither_algos[i].name)))
	    {
	      d->stpi_dither_type = dither_algos[i].id;
	      break;
	    }
	}
    }
  switch (d->stpi_dither_type)
    {
    case D_FAST:
      RETURN_DITHERFUNC(stpi_dither_fast, v);
    case D_VERY_FAST:
      RETURN_DITHERFUNC(stpi_dither_very_fast, v);
    case D_ORDERED:
      RETURN_DITHERFUNC(stpi_dither_ordered, v);
    case D_EVENTONE:
      RETURN_DITHERFUNC(stpi_dither_et, v);
    default:
      RETURN_DITHERFUNC(stpi_dither_ed, v);
    }
}

void
stpi_dither_init(stp_vars_t v, stp_image_t *image, int out_width,
		 int xdpi, int ydpi)
{
  int i;
  int in_width = stpi_image_width(image);
  int image_bpp = stpi_image_bpp(image);
  stpi_dither_t *d = stpi_zalloc(sizeof(stpi_dither_t));
  stpi_dither_range_simple_t r;
  const stpi_dotsize_t ds = {1, 1.0};
  stpi_shade_t shade;
  static const char *channels[] =
    {
      "BlackDensity", "CyanDensity", "MagentaDensity", "YellowDensity"
    };

  stpi_set_dither_data(v, d);

  d->dither_class = stp_get_output_type(v);
  d->error_rows = ERROR_ROWS;
  d->n_ghost_channels = 0;
  switch (d->dither_class)
    {
    case OUTPUT_GRAY:
      d->n_channels = 1;
      d->n_input_channels = 1;
      break;
    case OUTPUT_COLOR:
      d->n_channels = 4;
      d->n_input_channels = 3;
      break;
    case OUTPUT_RAW_PRINTER:
      d->n_channels = image_bpp / 2;
      d->n_input_channels = image_bpp / 2;
      break;
    case OUTPUT_RAW_CMYK:
      d->n_channels = 4;
      d->n_input_channels = 4;
      break;
    }
  d->ditherfunc = stpi_set_dither_function(v, image_bpp);

  d->channel = stpi_zalloc(d->n_channels * sizeof(stpi_dither_channel_t));
  r.value = 1.0;
  r.bit_pattern = 1;
  r.subchannel = 0;
  r.dot_size = 1;

  shade.value = 1.0;
  shade.subchannel = 0;
  shade.dot_sizes = &ds;
  shade.numsizes = 1;
  if (stp_check_float_parameter(v, "Density", STP_PARAMETER_ACTIVE))
    d->fdensity = stp_get_float_parameter(v, "Density");
  else
    d->fdensity = 1.0;
  d->d_cutoff = 4096;

  stpi_init_debug_messages(v);
  for (i = 0; i < d->n_channels; i++)
    {
      stpi_dither_set_ranges(v, i, 1, &r, 1.0);
      PHYSICAL_CHANNEL(d, i).shades = NULL;
      PHYSICAL_CHANNEL(d, i).numshades = 0;
      stpi_dither_set_shades(v, i, 1, &shade, 1.0);
      PHYSICAL_CHANNEL(d, i).errs = stpi_zalloc(d->error_rows * sizeof(int *));
      PHYSICAL_CHANNEL(d, i).density_adjustment = 1;
      if (i < sizeof(channels) / sizeof(const char *) &&
	  (stp_check_float_parameter(v, channels[i], STP_PARAMETER_ACTIVE)))
	PHYSICAL_CHANNEL(d, i).density_adjustment =
	  stp_get_float_parameter(v, channels[i]);
      PHYSICAL_CHANNEL(d, i).density_adjustment *= d->fdensity;
      PHYSICAL_CHANNEL(d, i).sqrt_density_adjustment =
	sqrt(PHYSICAL_CHANNEL(d, i).density_adjustment);
    }
  stpi_flush_debug_messages(v);
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
  d->adaptive_limit = .75 * 65535;

  if (d->stpi_dither_type == D_VERY_FAST)
    {
      if (stp_check_int_parameter(v, "DitherVeryFastSteps",
				  STP_PARAMETER_ACTIVE))
	stpi_dither_set_iterated_matrix
	  (v, 2, stp_get_int_parameter(v, "DitherVeryFastSteps"), sq2, 0, 2,4);
      else
	stpi_dither_set_iterated_matrix(v, 2, DITHER_FAST_STEPS, sq2, 0, 2, 4);
    }
  else if (stp_check_array_parameter(v, "DitherMatrix",
				     STP_PARAMETER_ACTIVE) &&
	   (stpi_dither_matrix_validate_array
	    (stp_get_array_parameter(v, "DitherMatrix"))))
    {
      stpi_dither_set_matrix_from_dither_array
	(v, stp_get_array_parameter(v, "DitherMatrix"), 0);
    }
  else
    {
      stp_array_t array;
      int transposed;
	array = stpi_find_standard_dither_array(d->y_aspect, d->x_aspect);
      transposed = d->y_aspect < d->x_aspect ? 1 : 0;
      if (array)
	{
	  stpi_dither_set_matrix_from_dither_array(v, array, transposed);
	  stp_array_destroy(array);
	}
      else
	{
	  stpi_eprintf(v, "Cannot find dither matrix file!  Aborting.\n");
	  stpi_abort();
	}
    }

  d->src_width = in_width;
  d->dst_width = out_width;

  stpi_dither_set_ink_spread(v, 13);
  for (i = 0; i <= d->n_channels; i++)
    {
      stpi_dither_set_randomizer(v, i, 1.0);
    }
  d->dt.channel_count = 0;
  d->dt.c = NULL;
}

void
stpi_dither_set_adaptive_limit(stp_vars_t v, double limit)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
  d->adaptive_limit = limit;
}

void
stpi_dither_set_ink_spread(stp_vars_t v, int spread)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
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
      d->offset0_table = stpi_malloc(sizeof(int) * max_offset);
      d->offset1_table = stpi_malloc(sizeof(int) * max_offset);
      for (i = 0; i < max_offset; i++)
	{
	  d->offset0_table[i] = (i + 1) * (i + 1);
	  d->offset1_table[i] = ((i + 1) * i) / 2;
	}
    }
  d->spread_mask = (1 << d->spread) - 1;
}

void
stpi_dither_set_randomizer(stp_vars_t v, int i, double val)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
  if (i < 0 || i >= PHYSICAL_CHANNEL_COUNT(d))
    return;
  PHYSICAL_CHANNEL(d, i).randomizer = val * 65535;
}

void
stpi_dither_free(stp_vars_t v)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
  stpi_dither_data_t *dt = &(d->dt);
  int i;
  int j;
  stpi_set_dither_data(v, NULL);
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
      if (PHYSICAL_CHANNEL(d, j).shades) {
        for (i = 0; i < PHYSICAL_CHANNEL(d, j).numshades; i++) {
          SAFE_FREE(PHYSICAL_CHANNEL(d, j).shades[i].dotsizes);
	  SAFE_FREE(PHYSICAL_CHANNEL(d, j).shades[i].errs);
	  SAFE_FREE(PHYSICAL_CHANNEL(d, j).shades[i].et_dis);
        }
        SAFE_FREE(PHYSICAL_CHANNEL(d, j).shades);
      }
      stpi_dither_matrix_destroy(&(PHYSICAL_CHANNEL(d, j).pick));
      stpi_dither_matrix_destroy(&(PHYSICAL_CHANNEL(d, j).dithermat));
    }
  SAFE_FREE(d->offset0_table);
  SAFE_FREE(d->offset1_table);
  stpi_dither_matrix_destroy(&(d->dither_matrix));
  stpi_dither_matrix_destroy(&(d->transition_matrix));
  if (d->aux_freefunc)
    (d->aux_freefunc)(d);
  stpi_free(d->channel);
  for (i = 0; i < dt->channel_count; i++)
    stpi_free(dt->c[i].c);
  stpi_free(dt->c);
  stpi_free(d);
}

void
stpi_dither_reverse_row_ends(stpi_dither_t *d)
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

int
stpi_dither_get_first_position(stp_vars_t v, int color, int subchannel)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
  if (color < 0 || color >= PHYSICAL_CHANNEL_COUNT(d))
    return -1;
  if (subchannel < 0 || subchannel >= PHYSICAL_CHANNEL(d, color).subchannels)
    return -1;
  return PHYSICAL_CHANNEL(d, color).row_ends[0][subchannel];
}

int
stpi_dither_get_last_position(stp_vars_t v, int color, int subchannel)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
  if (color < 0 || color >= PHYSICAL_CHANNEL_COUNT(d))
    return -1;
  if (subchannel < 0 || subchannel >= PHYSICAL_CHANNEL(d, color).subchannels)
    return -1;
  return PHYSICAL_CHANNEL(d, color).row_ends[0][subchannel];
}

void
stpi_dither(stp_vars_t v, int row, const unsigned short *input,
	    int duplicate_line, int zero_mask)
{
  int i, j;
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_dither_data(v);
  int ghost_channels = 0;
  for (i = 0; i < PHYSICAL_CHANNEL_COUNT(d); i++)
    {
      for (j = 0; j < PHYSICAL_CHANNEL(d, i).subchannels; j++)
	{
	  if (i >= d->dt.channel_count || j >= d->dt.c[i].subchannel_count)
	    {
	      PHYSICAL_CHANNEL(d, i).ptrs[j] = NULL;
	      ghost_channels++;
	    }
	  else
	    PHYSICAL_CHANNEL(d, i).ptrs[j] = d->dt.c[i].c[j];
	  if (PHYSICAL_CHANNEL(d, i).ptrs[j])
	    memset(PHYSICAL_CHANNEL(d, i).ptrs[j], 0,
		   (d->dst_width + 7) / 8 * PHYSICAL_CHANNEL(d, i).signif_bits);
	  PHYSICAL_CHANNEL(d, i).row_ends[0][j] = -1;
	  PHYSICAL_CHANNEL(d, i).row_ends[1][j] = -1;
	}
      stpi_dither_matrix_set_row(&(PHYSICAL_CHANNEL(d, i).dithermat), row);
      stpi_dither_matrix_set_row(&(PHYSICAL_CHANNEL(d, i).pick), row);
    }
  d->n_ghost_channels = ghost_channels;
  d->ptr_offset = 0;
  (d->ditherfunc)(v, row, input, duplicate_line, zero_mask);
}
