/*
 * "$Id$"
 *
 *   Gimp-Print color management module - traditional Gimp-Print algorithm.
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
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <math.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <string.h>
#include "module.h"
#include "xml.h"

#ifdef __GNUC__
#define inline __inline__
#endif

/* Color conversion function */
typedef unsigned (*stp_convert_t)(stp_const_vars_t vars,
				  const unsigned char *in,
				  unsigned short *out);

typedef struct
{
  unsigned steps;
  unsigned char *in_data;
  int image_bpp;
  int image_width;
  int out_channels;
  int channels_are_initialized;
  int input_color_model;
  int output_color_model;
  stp_convert_t colorfunc;
  stp_curve_t composite;
  stp_curve_t black;
  stp_curve_t cyan;
  stp_curve_t magenta;
  stp_curve_t yellow;
  stp_curve_t hue_map;
  const double *hue_cache;
  size_t hue_count;
  stp_curve_t lum_map;
  const double *lum_cache;
  size_t lum_count;
  stp_curve_t sat_map;
  const double *sat_cache;
  size_t sat_count;
  stp_curve_t gcr_curve;
  unsigned short *cmy_tmp;
  unsigned short *cmyk_lut;
} lut_t;

typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
  int color_only;
} float_param_t;

static const float_param_t float_parameters[] =
{
  {
    {
      "ColorCorrection", N_("Color Correction"), N_("Basic Image Adjustment"),
      N_("Color correction to be applied"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 1, 1, -1, 1
    }, 0.0, 0.0, 0.0, 0
  },
  {
    {
      "Brightness", N_("Brightness"), N_("Basic Image Adjustment"),
      N_("Brightness of the print (0 is solid black, 2 is solid white)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
    }, 0.0, 2.0, 1.0, 0
  },
  {
    {
      "Contrast", N_("Contrast"), N_("Basic Image Adjustment"),
      N_("Contrast of the print (0 is solid gray)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
    }, 0.0, 4.0, 1.0, 0
  },
  {
    {
      "LinearContrast", N_("Linear Contrast Adjustment"), N_("Advanced Image Control"),
      N_("Use linear vs. fixed end point contrast adjustment"),
      STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 1, 1, -1, 1
    }, 0.0, 0.0, 0.0, 0
  },
  {
    {
      "Gamma", N_("Gamma"), N_("Gamma"),
      N_("Adjust the gamma of the print. Larger values will "
	 "produce a generally brighter print, while smaller "
	 "values will produce a generally darker print. "
	 "Black and white will remain the same, unlike with "
	 "the brightness adjustment."),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
    }, 0.1, 4.0, 1.0, 0
  },
  {
    {
      "AppGamma", N_("AppGamma"), N_("Gamma"),
      N_("Gamma value assumed by application"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_INTERNAL, 0, 1, -1, 1
    }, 0.1, 4.0, 1.0, 0
  },
  {
    {
      "CyanGamma", N_("Cyan"), N_("Gamma"),
      N_("Adjust the cyan gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 1, 1
    }, 0.0, 4.0, 1.0, 1
  },
  {
    {
      "MagentaGamma", N_("Magenta"), N_("Gamma"),
      N_("Adjust the magenta gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 2, 1
    }, 0.0, 4.0, 1.0, 1
  },
  {
    {
      "YellowGamma", N_("Yellow"), N_("Gamma"),
      N_("Adjust the yellow gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 3, 1
    }, 0.0, 4.0, 1.0, 1
  },
  {
    {
      "Saturation", N_("Saturation"), N_("Basic Image Adjustment"),
      N_("Adjust the saturation (color balance) of the print\n"
	 "Use zero saturation to produce grayscale output "
	 "using color and black inks"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
    }, 0.0, 9.0, 1.0, 1
  },
  /* Need to think this through a bit more -- rlk 20030712 */
  {
    {
      "InkLimit", N_("Ink Limit"), N_("Advanced Output Control"),
      N_("Limit the total ink printed to the page"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 0
    }, 0.0, 32.0, 32.0, 1
  },
  {
    {
      "BlackGamma", N_("GCR Transition"), N_("Advanced Output Control"),
      N_("Adjust the gray component transition rate"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0, 1
    }, 0.0, 1.0, 1.0, 1
  },
  {
    {
      "GCRLower", N_("GCR Lower Bound"), N_("Advanced Output Control"),
      N_("Lower bound of gray component reduction"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0, 1
    }, 0.0, 1.0, 0.2, 1
  },
  {
    {
      "GCRUpper", N_("GCR Upper Bound"), N_("Advanced Output Control"),
      N_("Upper bound of gray component reduction"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0, 1
    }, 0.0, 5.0, 0.5, 1
  },
};

static const int float_parameter_count =
sizeof(float_parameters) / sizeof(float_param_t);

typedef struct
{
  stp_parameter_t param;
  stp_curve_t *defval;
  int color_only;
  int hsl_only;
} curve_param_t;

typedef struct {
  int input_color_model;
  int output_color_model;
  int steps;
  int linear_contrast_adjustment;
  double print_gamma;
  double app_gamma;
  double screen_gamma;
  double brightness;
  double contrast;
} lut_params_t;

static int standard_curves_initialized = 0;

static stp_curve_t hue_map_bounds = NULL;
static stp_curve_t lum_map_bounds = NULL;
static stp_curve_t sat_map_bounds = NULL;
static stp_curve_t color_curve_bounds = NULL;
static stp_curve_t gcr_curve_bounds = NULL;

static curve_param_t curve_parameters[] =
{
  {
    {
      "CompositeCurve", N_("Composite Curve"), N_("Output Curves"),
      N_("Composite (Grayscale) curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, -1, 1
    }, &color_curve_bounds, 0, 0
  },
  {
    {
      "CyanCurve", N_("Cyan Curve"), N_("Output Curves"),
      N_("Cyan curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 1, 1
    }, &color_curve_bounds, 1, 0
  },
  {
    {
      "MagentaCurve", N_("Magenta Curve"), N_("Output Curves"),
      N_("Magenta curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 2, 1
    }, &color_curve_bounds, 1, 0
  },
  {
    {
      "YellowCurve", N_("Yellow Curve"), N_("Output Curves"),
      N_("Yellow curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 3, 1
    }, &color_curve_bounds, 1, 0
  },
  {
    {
      "BlackCurve", N_("Black Curve"), N_("Output Curves"),
      N_("Black curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 0, 1
    }, &color_curve_bounds, 1, 0
  },
  {
    {
      "HueMap", N_("Hue Map"), N_("Advanced HSL Curves"),
      N_("Hue adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, -1, 1
    }, &hue_map_bounds, 1, 1
  },
  {
    {
      "SatMap", N_("Saturation Map"), N_("Advanced HSL Curves"),
      N_("Saturation adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, -1, 1
    }, &sat_map_bounds, 1, 1
  },
  {
    {
      "LumMap", N_("Luminosity Map"), N_("Advanced HSL Curves"),
      N_("Luminosity adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, -1, 1
    }, &lum_map_bounds, 1, 1
  },
  {
    {
      "GCRCurve", N_("Gray Component Reduction"), N_("Advanced Output Control"),
      N_("Gray component reduction curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, 0, 1
    }, &gcr_curve_bounds, 1, 0
  },
};

static const int curve_parameter_count =
sizeof(curve_parameters) / sizeof(curve_param_t);

static stp_curve_t compute_gcr_curve(stp_const_vars_t vars);

/*
 * RGB to grayscale luminance constants...
 */

#define LUM_RED		31
#define LUM_GREEN	61
#define LUM_BLUE	8

/* rgb/hsl conversions taken from Gimp common/autostretch_hsv.c */

#define FMAX(a, b) ((a) > (b) ? (a) : (b))
#define FMIN(a, b) ((a) < (b) ? (a) : (b))


static void
initialize_cmyk_lut(stp_const_vars_t vars, size_t count)
{
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  if (!lut->cmyk_lut)
    {
      int i;
      double print_gamma = stp_get_float_parameter(vars, "Gamma");
      lut->cmyk_lut = stpi_malloc(sizeof(unsigned short) * count);

      for (i = 0; i < count; i ++)
	lut->cmyk_lut[i] =
	  (65535.0 * pow((double)i / (double) (count - 1), print_gamma)) + 0.5;
    }
}

static void
initialize_gcr_curve(stp_const_vars_t vars)
{
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  if (!lut->gcr_curve)
    {
      if (stp_check_curve_parameter(vars, "GCRCurve", STP_PARAMETER_DEFAULTED))
	{
	  double data;
	  size_t count;
	  int i;
	  lut->gcr_curve =
	    stp_curve_create_copy(stp_get_curve_parameter(vars, "GCRCurve"));
	  stp_curve_resample(lut->gcr_curve, lut->steps);
	  count = stp_curve_count_points(lut->gcr_curve);
	  stp_curve_set_bounds(lut->gcr_curve, 0.0, 65535.0);
	  for (i = 0; i < count; i++)
	    {
	      stp_curve_get_point(lut->gcr_curve, i, &data);
	      data = 65535.0 * data * (double) i / (count - 1);
	      stp_curve_set_point(lut->gcr_curve, i, data);
	    }
	}
      else
	lut->gcr_curve = compute_gcr_curve(vars);
    }
}

static inline void
calc_rgb_to_hsl(unsigned short *rgb, double *hue, double *sat,
		double *lightness)
{
  double red, green, blue;
  double h, s, l;
  double min, max;
  double delta;
  int maxval;

  red   = rgb[0] / 65535.0;
  green = rgb[1] / 65535.0;
  blue  = rgb[2] / 65535.0;

  if (red > green)
    {
      if (red > blue)
	{
	  max = red;
	  maxval = 0;
	}
      else
	{
	  max = blue;
	  maxval = 2;
	}
      min = FMIN(green, blue);
    }
  else
    {
      if (green > blue)
	{
	  max = green;
	  maxval = 1;
	}
      else
	{
	  max = blue;
	  maxval = 2;
	}
      min = FMIN(red, blue);
    }

  l = (max + min) / 2.0;
  delta = max - min;

  if (delta < .000001)	/* Suggested by Eugene Anikin <eugene@anikin.com> */
    {
      s = 0.0;
      h = 0.0;
    }
  else
    {
      if (l <= .5)
	s = delta / (max + min);
      else
	s = delta / (2 - max - min);

      if (maxval == 0)
	h = (green - blue) / delta;
      else if (maxval == 1)
	h = 2 + (blue - red) / delta;
      else
	h = 4 + (red - green) / delta;

      if (h < 0.0)
	h += 6.0;
      else if (h > 6.0)
	h -= 6.0;
    }

  *hue = h;
  *sat = s;
  *lightness = l;
}

static inline double
hsl_value(double n1, double n2, double hue)
{
  if (hue < 0)
    hue += 6.0;
  else if (hue > 6)
    hue -= 6.0;
  if (hue < 1.0)
    return (n1 + (n2 - n1) * hue);
  else if (hue < 3.0)
    return (n2);
  else if (hue < 4.0)
    return (n1 + (n2 - n1) * (4.0 - hue));
  else
    return (n1);
}

static inline void
calc_hsl_to_rgb(unsigned short *rgb, double h, double s, double l)
{
  if (s < .0000001)
    {
      if (l > 1)
	l = 1;
      else if (l < 0)
	l = 0;
      rgb[0] = l * 65535;
      rgb[1] = l * 65535;
      rgb[2] = l * 65535;
    }
  else
    {
      double m1, m2;
      double h1, h2;
      h1 = h + 2;
      h2 = h - 2;

      if (l < .5)
	m2 = l * (1 + s);
      else
	m2 = l + s - (l * s);
      m1 = (l * 2) - m2;
      rgb[0] = 65535 * hsl_value(m1, m2, h1);
      rgb[1] = 65535 * hsl_value(m1, m2, h);
      rgb[2] = 65535 * hsl_value(m1, m2, h2);
    }
}

static inline double
update_saturation(double sat, double adjust, double isat)
{
  if (adjust < 1)
    sat *= adjust;
  else
    {
      double s1 = sat * adjust;
      double s2 = 1.0 - ((1.0 - sat) * isat);
      sat = FMIN(s1, s2);
    }
  if (sat > 1)
    sat = 1.0;
  return sat;
}

static inline double
interpolate_value(const double *vec, double val)
{
  double base = floor(val);
  double frac = val - base;
  int ibase = (int) base;
  double lval = vec[ibase];
  if (frac > 0)
    lval += (vec[ibase + 1] - lval) * frac;
  return lval;
}

static inline void
update_saturation_from_rgb(unsigned short *rgb, double adjust, double isat)
{
  double h, s, l;
  calc_rgb_to_hsl(rgb, &h, &s, &l);
  s = update_saturation(s, adjust, isat);
  calc_hsl_to_rgb(rgb, h, s, l);
}

static inline double
adjust_hue(const double *hue_map, double hue, size_t points)
{
  if (hue_map)
    {
      hue += interpolate_value(hue_map, hue * points / 6.0);
      if (hue < 0.0)
	hue += 6.0;
      else if (hue >= 6.0)
	hue -= 6.0;
    }
  return hue;
}

static inline void
adjust_hsl(unsigned short *rgbout, lut_t *lut, double ssat,
	   double isat, size_t h_points, size_t s_points, size_t l_points,
	   int split_saturation)
{
  if ((split_saturation || lut->hue_map || lut->lum_map ||
       lut->sat_map) &&
      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
    {
      double h, s, l;
      double oh;
      rgbout[0] ^= 65535;
      rgbout[1] ^= 65535;
      rgbout[2] ^= 65535;
      calc_rgb_to_hsl(rgbout, &h, &s, &l);
      s = update_saturation(s, ssat, isat);
      oh = h;
      h = adjust_hue(lut->hue_cache, h, h_points);
      if (lut->lum_map && l > 0.0001 && l < .9999)
	{
	  double nh = oh * l_points / 6.0;
	  double el = interpolate_value(lut->lum_cache, nh);
	  double sreflection = .8 - ((1.0 - el) / 1.3) ;
	  double isreflection = 1.0 - sreflection;
	  double sadj = l - sreflection;
	  double isadj = 1;
	  double sisadj = 1;
	  if (sadj > 0)
	    {
	      isadj = (1.0 / isreflection) * (isreflection - sadj);
	      sisadj = sqrt(isadj);
	      /*
		s *= isadj * sisadj;
	      */
	      s *= sqrt(isadj * sisadj);
	    }
	  if (el < .9999)
	    {
	      double es = s;
	      es = 1 - es;
	      es *= es * es;
	      es = 1 - es;
	      el = 1.0 + (es * (el - 1.0));
	      l *= el;
	    }
	  else if (el > 1.0001)
	    l = 1.0 - pow(1.0 - l, el);
	  if (sadj > 0)
	    {
	      /*	          s *= sqrt(isadj); */
	      l = 1.0 - ((1.0 - l) * sqrt(sqrt(sisadj)));
	    }
	}
      if (lut->sat_map)
	{
	  double nh = oh * s_points / 6.0;
	  double tmp = interpolate_value(lut->sat_cache, nh);
	  if (tmp < .9999 || tmp > 1.0001)
	    {
	      s = update_saturation(s, tmp, tmp > 1.0 ? 1.0 / tmp : 1.0);
	    }
	}
      calc_hsl_to_rgb(rgbout, h, s, l);
      rgbout[0] ^= 65535;
      rgbout[1] ^= 65535;
      rgbout[2] ^= 65535;
    }
}

static inline void
adjust_hsl_bright(unsigned short *rgbout, lut_t *lut, double ssat,
	   double isat, size_t h_points, size_t s_points, size_t l_points,
	   int split_saturation)
{
  if ((split_saturation || lut->hue_map || lut->lum_map) &&
      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
    {
      double h, s, l;
      rgbout[0] ^= 65535;
      rgbout[1] ^= 65535;
      rgbout[2] ^= 65535;
      calc_rgb_to_hsl(rgbout, &h, &s, &l);
      s = update_saturation(s, ssat, isat);
      h = adjust_hue(lut->hue_cache, h, h_points);
      if (lut->lum_map && l > 0.0001 && l < .9999)
	{
	  double nh = h * l_points / 6.0;
	  double el = interpolate_value(lut->lum_cache, nh);
	  el = 1.0 + (s * (el - 1.0));
	  l = 1.0 - pow(1.0 - l, el);
	}
      calc_hsl_to_rgb(rgbout, h, s, l);
      rgbout[0] ^= 65535;
      rgbout[1] ^= 65535;
      rgbout[2] ^= 65535;
    }
}

static inline void
lookup_rgb(lut_t *lut, unsigned short *rgbout,
	   const unsigned short *red, const unsigned short *green,
	   const unsigned short *blue)
{
  if (lut->steps == 65536)
    {
      rgbout[0] = red[rgbout[0]];
      rgbout[1] = green[rgbout[1]];
      rgbout[2] = blue[rgbout[2]];
    }
  else
    {
      rgbout[0] = red[rgbout[0] / 256];
      rgbout[1] = green[rgbout[1] / 256];
      rgbout[2] = blue[rgbout[2] / 256];
    }
}

static inline int
mem_eq(const unsigned short *i1, const unsigned short *i2, int count)
{
  int i;
  for (i = 0; i < count; i++)
    if (i1[i] != i2[i])
      return 0;
  return 1;
}

static unsigned
generic_cmy_to_kcmy(stp_const_vars_t vars, const unsigned short *in,
		    unsigned short *out)
{
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  int width = lut->image_width;
  int step = 65535 / (lut->steps - 1); /* 1 or 257 */

  const unsigned short *gcr_lookup;
  const unsigned short *black_lookup;
  size_t points;
  int i;
  int j;
  unsigned short nz[4];
  unsigned retval = 0;
  const unsigned short *input_cache = NULL;
  const unsigned short *output_cache = NULL;

  initialize_gcr_curve(vars);
  gcr_lookup = stp_curve_get_ushort_data(lut->gcr_curve, &points);
  stp_curve_resample(lut->black, lut->steps);
  black_lookup = stp_curve_get_ushort_data(lut->black, &points);
  memset(nz, 0, sizeof(nz));

  for (i = 0; i < width; i++, out += 4, in += 3)
    {
      if (input_cache && mem_eq(input_cache, in, 3))
	{
	  for (j = 0; j < 4; j++)
	    out[j] = output_cache[j];
	}
      else
	{
	  int c = in[0];
	  int m = in[1];
	  int y = in[2];
	  int k = FMIN(c, FMIN(m, y));
	  input_cache = in;
	  out[0] = 0;
	  for (j = 0; j < 3; j++)
	    out[j + 1] = in[j];
	  if (k > 0)
	    {
	      int where, resid;
	      int kk;
	      if (lut->steps == 65536)
		kk = gcr_lookup[k];
	      else
		{
		  where = k / step;
		  resid = k % step;
		  kk = gcr_lookup[where];
		  if (resid > 0)
		    kk += (gcr_lookup[where + 1] - gcr_lookup[where]) * resid /
		      step;
		}
	      if (kk > k)
		kk = k;
	      if (kk > 0)
		{
		  if (lut->steps == 65536)
		    out[0] = black_lookup[kk];
		  else
		    {
		      int k_out;
		      where = kk / step;
		      resid = kk % step;
		      k_out = black_lookup[where];
		      if (resid > 0)
			k_out +=
			  (black_lookup[where + 1] - black_lookup[where]) * resid /
			  step;
		      out[0] = k_out;
		    }
		  out[1] -= kk;
		  out[2] -= kk;
		  out[3] -= kk;
		}
	    }
	  output_cache = out;
	  for (j = 0; j < 4; j++)
	    if (out[j])
	      nz[j] = 1;
	}
    }
  for (j = 0; j < 4; j++)
    if (nz[j] == 0)
      retval |= (1 << j);
  return retval;
}

/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

#define RGB_TO_RGB_FUNC(T, bits)					      \
static unsigned								      \
rgb_##bits##_to_rgb(stp_const_vars_t vars, const unsigned char *in,	      \
		   unsigned short *out)					      \
{									      \
  int i;								      \
  double isat = 1.0;							      \
  double ssat = stp_get_float_parameter(vars, "Saturation");		      \
  size_t count;								      \
  int i0 = -1;								      \
  int i1 = -1;								      \
  int i2 = -1;								      \
  unsigned short o0 = 0;						      \
  unsigned short o1 = 0;						      \
  unsigned short o2 = 0;						      \
  unsigned short nz0 = 0;						      \
  unsigned short nz1 = 0;						      \
  unsigned short nz2 = 0;						      \
  const T *s_in = (const T *) in;					      \
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	      \
  int compute_saturation = ssat <= .99999 || ssat >= 1.00001;		      \
  int split_saturation = ssat > 1.4;					      \
  const unsigned short *red =						      \
    stp_curve_get_ushort_data(lut->cyan, &count);			      \
  const unsigned short *green =						      \
    stp_curve_get_ushort_data(lut->magenta, &count);			      \
  const unsigned short *blue =						      \
    stp_curve_get_ushort_data(lut->yellow, &count);			      \
  size_t h_points = 0;							      \
  size_t l_points = 0;							      \
  size_t s_points = 0;							      \
  int bright_color_adjustment = 0;					      \
									      \
  if (strcmp(stp_get_string_parameter(vars, "ColorCorrection"),		      \
	     "Bright") == 0)						      \
    bright_color_adjustment = 1;					      \
									      \
  if (lut->hue_map) {							      \
    h_points = stp_curve_count_points(lut->hue_map);			      \
    lut->hue_cache = stp_curve_get_data(lut->hue_map, &(lut->hue_count));     \
  } else {								      \
    lut->hue_cache = 0;							      \
  }									      \
  if (lut->lum_map) {							      \
    l_points = stp_curve_count_points(lut->lum_map);			      \
    lut->lum_cache = stp_curve_get_data(lut->lum_map, &(lut->lum_count));     \
  } else {								      \
    lut->lum_cache = 0;							      \
  }									      \
  if (lut->sat_map) {							      \
    s_points = stp_curve_count_points(lut->sat_map);			      \
    lut->sat_cache = stp_curve_get_data(lut->sat_map, &(lut->sat_count));     \
  } else {								      \
    lut->sat_cache = 0;							      \
  }									      \
									      \
  if (split_saturation)							      \
    ssat = sqrt(ssat);							      \
  if (ssat > 1)								      \
    isat = 1.0 / ssat;							      \
  for (i = 0; i < lut->image_width; i++)				      \
    {									      \
      if (i0 == s_in[0] && i1 == s_in[1] && i2 == s_in[2])		      \
	{								      \
	  out[0] = o0;							      \
	  out[1] = o1;							      \
	  out[2] = o2;							      \
	}								      \
      else								      \
	{								      \
	  i0 = s_in[0];							      \
	  i1 = s_in[1];							      \
	  i2 = s_in[2];							      \
	  out[0] = i0 * (65535u / (unsigned) ((1 << bits) - 1));	      \
	  out[1] = i1 * (65535u / (unsigned) ((1 << bits) - 1));	      \
	  out[2] = i2 * (65535u / (unsigned) ((1 << bits) - 1));	      \
	  if ((compute_saturation) && (out[0] != out[1] || out[0] != out[2])) \
	    update_saturation_from_rgb(out, ssat, isat);		      \
	  if (bright_color_adjustment)					      \
	    adjust_hsl_bright(out, lut, ssat, isat, h_points,		      \
			      s_points, l_points, split_saturation);	      \
	  else								      \
	    adjust_hsl(out, lut, ssat, isat, h_points,			      \
		       s_points, l_points, split_saturation);		      \
	  lookup_rgb(lut, out, red, green, blue);			      \
	  o0 = out[0];							      \
	  o1 = out[1];							      \
	  o2 = out[2];							      \
	  nz0 |= o0;							      \
	  nz1 |= o1;							      \
	  nz2 |= o2;							      \
	}								      \
      s_in += 3;							      \
      out += 3;								      \
    }									      \
  return (nz0 ? 1 : 0) +  (nz1 ? 2 : 0) +  (nz2 ? 4 : 0);		      \
}

RGB_TO_RGB_FUNC(unsigned char, 8)
RGB_TO_RGB_FUNC(unsigned short, 16)

/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

#define FAST_RGB_TO_RGB_FUNC(T, bits)					 \
static unsigned								 \
fast_rgb_##bits##_to_rgb(stp_const_vars_t vars, const unsigned char *in, \
		       unsigned short *out)				 \
{									 \
  int i;								 \
  int i0 = -1;								 \
  int i1 = -1;								 \
  int i2 = -1;								 \
  int o0 = 0;								 \
  int o1 = 0;								 \
  int o2 = 0;								 \
  int nz0 = 0;								 \
  int nz1 = 0;								 \
  int nz2 = 0;								 \
  const T *s_in = (const T *) in;					 \
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	 \
  size_t count;								 \
  const unsigned short *red;						 \
  const unsigned short *green;						 \
  const unsigned short *blue;						 \
  double isat = 1.0;							 \
  double saturation = stp_get_float_parameter(vars, "Saturation");	 \
									 \
  stp_curve_resample(lut->cyan, 1 << bits);				 \
  stp_curve_resample(lut->magenta, 1 << bits);				 \
  stp_curve_resample(lut->yellow, 1 << bits);				 \
  red = stp_curve_get_ushort_data(lut->cyan, &count);			 \
  green = stp_curve_get_ushort_data(lut->magenta, &count);		 \
  blue = stp_curve_get_ushort_data(lut->yellow, &count);		 \
									 \
  if (saturation > 1)							 \
    isat = 1.0 / saturation;						 \
  for (i = 0; i < lut->image_width; i++)				 \
    {									 \
      if (i0 == s_in[0] && i1 == s_in[1] && i2 == s_in[2])		 \
	{								 \
	  out[0] = o0;							 \
	  out[1] = o1;							 \
	  out[2] = o2;							 \
	}								 \
      else								 \
	{								 \
	  i0 = s_in[0];							 \
	  i1 = s_in[1];							 \
	  i2 = s_in[2];							 \
	  out[0] = red[s_in[0]];					 \
	  out[1] = green[s_in[1]];					 \
	  out[2] = blue[s_in[2]];					 \
	  if (saturation != 1.0)					 \
	    update_saturation_from_rgb(out, saturation, isat);		 \
	  o0 = out[0];							 \
	  o1 = out[1];							 \
	  o2 = out[2];							 \
	  nz0 |= o0;							 \
	  nz1 |= o1;							 \
	  nz2 |= o2;							 \
	}								 \
      s_in += 3;							 \
      out += 3;								 \
    }									 \
  return (nz0 ? 1 : 0) +  (nz1 ? 2 : 0) +  (nz2 ? 4 : 0);		 \
}

FAST_RGB_TO_RGB_FUNC(unsigned char, 8)
FAST_RGB_TO_RGB_FUNC(unsigned short, 16)

/*
 * 'gray_to_rgb()' - Convert gray image data to RGB.
 */

#define GRAY_TO_RGB_FUNC(T, bits)					\
static unsigned								\
gray_##bits##_to_rgb(stp_const_vars_t vars, const unsigned char *in,	\
		   unsigned short *out)					\
{									\
  int i;								\
  int i0 = -1;								\
  int o0 = 0;								\
  int o1 = 0;								\
  int o2 = 0;								\
  int nz0 = 0;								\
  int nz1 = 0;								\
  int nz2 = 0;								\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
  size_t count;								\
  const unsigned short *red;						\
  const unsigned short *green;						\
  const unsigned short *blue;						\
									\
  stp_curve_resample(lut->cyan, 1 << bits);				\
  stp_curve_resample(lut->magenta, 1 << bits);				\
  stp_curve_resample(lut->yellow, 1 << bits);				\
  red = stp_curve_get_ushort_data(lut->cyan, &count);			\
  green = stp_curve_get_ushort_data(lut->magenta, &count);		\
  blue = stp_curve_get_ushort_data(lut->yellow, &count);		\
									\
  for (i = 0; i < lut->image_width; i++)				\
    {									\
      if (i0 == s_in[0])						\
	{								\
	  out[0] = o0;							\
	  out[1] = o1;							\
	  out[2] = o2;							\
	}								\
      else								\
	{								\
	  i0 = s_in[0];							\
	  out[0] = red[s_in[0]];					\
	  out[1] = green[s_in[0]];					\
	  out[2] = blue[s_in[0]];					\
	  o0 = out[0];							\
	  o1 = out[1];							\
	  o2 = out[2];							\
	  nz0 |= o0;							\
	  nz1 |= o1;							\
	  nz2 |= o2;							\
	}								\
      s_in += 1;							\
      out += 3;								\
    }									\
  return (nz0 ? 1 : 0) +  (nz1 ? 2 : 0) +  (nz2 ? 4 : 0);		\
}

GRAY_TO_RGB_FUNC(unsigned char, 8)
GRAY_TO_RGB_FUNC(unsigned short, 16)

#define RGB_TO_KCMY_FUNC(name, bits)					\
static unsigned								\
name##_##bits##_to_kcmy(stp_const_vars_t vars, const unsigned char *in,	\
	                unsigned short *out)				\
{									\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
  if (!lut->cmy_tmp)							\
    lut->cmy_tmp = stpi_malloc(4 * 2 * lut->image_width);		\
  name##_##bits##_to_rgb(vars, in, lut->cmy_tmp);			\
  return generic_cmy_to_kcmy(vars, lut->cmy_tmp, out);			\
}

RGB_TO_KCMY_FUNC(gray, 8)
RGB_TO_KCMY_FUNC(rgb, 8)
RGB_TO_KCMY_FUNC(fast_rgb, 8)
RGB_TO_KCMY_FUNC(gray, 16)
RGB_TO_KCMY_FUNC(rgb, 16)
RGB_TO_KCMY_FUNC(fast_rgb, 16)

#define RGB_TO_KCMY_LINE_ART_FUNC(T, name)				\
static unsigned								\
name##_to_kcmy_line_art(stp_const_vars_t vars,				\
			const unsigned char *in,			\
			unsigned short *out)				\
{									\
  int i;								\
  int z = 15;								\
  const T *s_in = (const T *) in;					\
  unsigned high_bit = ((1 << ((sizeof(T) * 8) - 1)));			\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  unsigned mask = 0;							\
  memset(out, 0, width * 4 * sizeof(unsigned short));			\
  if (lut->output_color_model != lut->input_color_model)		\
    mask = (1 << (sizeof(T) * 8)) - 1;					\
									\
  for (i = 0; i < width; i++, out += 4, s_in += 3)			\
    {									\
      unsigned c = s_in[0] ^ mask;					\
      unsigned m = s_in[1] ^ mask;					\
      unsigned y = s_in[2] ^ mask;					\
      unsigned k = (c < m ? (c < y ? c : y) : (m < y ? m : y));		\
      if (k >= high_bit)						\
	{								\
	  c -= k;							\
	  m -= k;							\
	  y -= k;							\
	}								\
      if (k >= high_bit)						\
	{								\
	  z &= 0xe;							\
	  out[0] = 65535;						\
	}								\
      if (c >= high_bit)						\
	{								\
	  z &= 0xd;							\
	  out[1] = 65535;						\
	}								\
      if (m >= high_bit)						\
	{								\
	  z &= 0xb;							\
	  out[2] = 65535;						\
	}								\
      if (y >= high_bit)						\
	{								\
	  z &= 0x7;							\
	  out[3] = 65535;						\
	}								\
    }									\
  return z;								\
}

RGB_TO_KCMY_LINE_ART_FUNC(unsigned char, rgb_8)
RGB_TO_KCMY_LINE_ART_FUNC(unsigned short, rgb_16)

#define CMYK_TO_KCMY_LINE_ART_FUNC(T, name)				\
static unsigned								\
name##_to_kcmy_line_art(stp_const_vars_t vars,				\
			const unsigned char *in,			\
			unsigned short *out)				\
{									\
  int i;								\
  int z = 15;								\
  const T *s_in = (const T *) in;					\
  unsigned desired_high_bit = 0;					\
  unsigned high_bit = 1 << ((sizeof(T) * 8) - 1);			\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  memset(out, 0, width * 4 * sizeof(unsigned short));			\
  if (lut->output_color_model == lut->input_color_model)		\
    desired_high_bit = high_bit;					\
									\
  for (i = 0; i < width; i++, out += 4, s_in += 4)			\
    {									\
      if ((s_in[3] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 0xe;							\
	  out[0] = 65535;						\
	}								\
      if ((s_in[0] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 0xd;							\
	  out[1] = 65535;						\
	}								\
      if ((s_in[1] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 0xb;							\
	  out[2] = 65535;						\
	}								\
      if ((s_in[2] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 0x7;							\
	  out[3] = 65535;						\
	}								\
    }									\
  return z;								\
}

CMYK_TO_KCMY_LINE_ART_FUNC(unsigned char, cmyk_8)
CMYK_TO_KCMY_LINE_ART_FUNC(unsigned short, cmyk_16)

#define GRAY_TO_COLOR_LINE_ART_FUNC(T, name, bits, channels)		\
static unsigned								\
gray_##bits##_to_##name##_line_art(stp_const_vars_t vars,		\
				   const unsigned char *in,		\
				   unsigned short *out)			\
{									\
  int i;								\
  int z = (1 << channels) - 1;						\
  int desired_high_bit = 0;						\
  unsigned high_bit = 1 << ((sizeof(T) * 8) - 1);			\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  memset(out, 0, width * channels * sizeof(unsigned short));		\
  if (lut->output_color_model == lut->input_color_model)		\
    desired_high_bit = high_bit;					\
									\
  for (i = 0; i < width; i++, out += channels, s_in++)			\
    {									\
      if ((s_in[0] & high_bit) == desired_high_bit)			\
	{								\
	  int j;							\
	  z = 0;							\
	  for (j = 0; j < channels; j++)				\
	    out[j] = 65535;						\
	}								\
    }									\
  return z;								\
}

GRAY_TO_COLOR_LINE_ART_FUNC(unsigned char, rgb, 8, 3)
GRAY_TO_COLOR_LINE_ART_FUNC(unsigned short, rgb, 16, 3)
GRAY_TO_COLOR_LINE_ART_FUNC(unsigned char, kcmy, 8, 4)
GRAY_TO_COLOR_LINE_ART_FUNC(unsigned short, kcmy, 16, 4)

#define RGB_TO_RGB_LINE_ART_FUNC(T, name)				\
static unsigned								\
name##_to_rgb_line_art(stp_const_vars_t vars,				\
		       const unsigned char *in,				\
		       unsigned short *out)				\
{									\
  int i;								\
  int z = 7;								\
  int desired_high_bit = 0;						\
  unsigned high_bit = ((1 << ((sizeof(T) * 8) - 1)) * 4);		\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  memset(out, 0, width * 3 * sizeof(unsigned short));			\
  if (lut->input_color_model == lut->output_color_model)		\
    desired_high_bit = high_bit;					\
									\
  for (i = 0; i < width; i++, out += 3, s_in += 3)			\
    {									\
      if ((s_in[0] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 6;							\
	  out[0] = 65535;						\
	}								\
      if ((s_in[1] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 5;							\
	  out[1] = 65535;						\
	}								\
      if ((s_in[1] & high_bit) == desired_high_bit)			\
	{								\
	  z &= 3;							\
	  out[2] = 65535;						\
	}								\
    }									\
  return z;								\
}

RGB_TO_RGB_LINE_ART_FUNC(unsigned char, rgb_8)
RGB_TO_RGB_LINE_ART_FUNC(unsigned short, rgb_16)

#define COLOR_TO_GRAY_LINE_ART_FUNC(T, name, channels, max_channels)	\
static unsigned								\
name##_to_gray_line_art(stp_const_vars_t vars,				\
			const unsigned char *in,			\
			unsigned short *out)				\
{									\
  int i;								\
  int z = 1;								\
  int desired_high_bit = 0;						\
  unsigned high_bit = ((1 << ((sizeof(T) * 8) - 1)) * max_channels);	\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  memset(out, 0, width * sizeof(unsigned short));			\
  if (lut->output_color_model == lut->input_color_model)		\
    desired_high_bit = high_bit;					\
									\
  for (i = 0; i < width; i++, out++, s_in += channels)			\
    {									\
      unsigned gval =							\
	(max_channels - channels) * (1 << ((sizeof(T) * 8) - 1));	\
      int j;								\
      for (j = 0; j < channels; j++)					\
	gval += s_in[j];						\
      if ((gval & high_bit) == desired_high_bit)			\
	{								\
	  out[0] = 65535;						\
	  z = 0;							\
	}								\
    }									\
  return z;								\
}

COLOR_TO_GRAY_LINE_ART_FUNC(unsigned char, cmyk_8, 4, 4)
COLOR_TO_GRAY_LINE_ART_FUNC(unsigned short, cmyk_16, 4, 4)
COLOR_TO_GRAY_LINE_ART_FUNC(unsigned char, color_8, 3, 4)
COLOR_TO_GRAY_LINE_ART_FUNC(unsigned short, color_16, 3, 4)
COLOR_TO_GRAY_LINE_ART_FUNC(unsigned char, gray_8, 1, 1)
COLOR_TO_GRAY_LINE_ART_FUNC(unsigned short, gray_16, 1, 1)

#define CMYK_TO_KCMY_FUNC(T, size)					\
static unsigned								\
cmyk_##size##_to_kcmy(stp_const_vars_t vars,				\
		      const unsigned char *in,				\
		      unsigned short *out)				\
{									\
  int i;								\
  unsigned retval = 0;							\
  int j;								\
  int nz[4];								\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
									\
  initialize_cmyk_lut(vars, (1<<size)-1);				\
  memset(nz, 0, sizeof(nz));						\
									\
  for (i = 0; i < lut->image_width; i++, out += 4)			\
    {									\
      for (j = 0; j < 4; j++)						\
	{								\
	  int outpos = (j + 1) & 3;					\
	  int inval = *s_in++;						\
	  nz[outpos] |= j;						\
	  out[outpos] = lut->cmyk_lut[inval];				\
	}								\
    }									\
  for (j = 0; j < 4; j++)						\
    if (nz[j] == 0)							\
      retval |= (1 << j);						\
  return retval;							\
}

CMYK_TO_KCMY_FUNC(unsigned char, 8)
CMYK_TO_KCMY_FUNC(unsigned short, 16)

/*
 * 'gray_to_gray()' - Convert grayscale image data to grayscale (brightness
 *                    adjusted).
 */


#define GRAY_TO_GRAY_FUNC(T, bits)					\
static unsigned								\
gray_##bits##_to_gray(stp_const_vars_t vars,				\
		      const unsigned char *in,				\
		      unsigned short *out)				\
{									\
  int i;								\
  int i0 = -1;								\
  int o0 = 0;								\
  int nz = 0;								\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
  int width = lut->image_width;						\
  size_t count;								\
  const unsigned short *composite;					\
									\
  stp_curve_resample(lut->composite, 1 << bits);			\
  composite = stp_curve_get_ushort_data(lut->composite, &count);	\
									\
  memset(out, 0, width * sizeof(unsigned short));			\
									\
  for (i = 0; i < lut->image_width; i++)				\
    {									\
      if (i0 != s_in[0])						\
	{								\
	  i0 = s_in[0];							\
	  o0 = composite[i0];						\
	  nz |= o0;							\
	}								\
      out[0] = o0;							\
      s_in ++;								\
      out ++;								\
    }									\
  return nz == 0;							\
}

GRAY_TO_GRAY_FUNC(unsigned char, 8)
GRAY_TO_GRAY_FUNC(unsigned short, 16)

/*
 * 'rgb_to_gray()' - Convert RGB image data to grayscale.
 */

#define COLOR_TO_GRAY_FUNC(T, bits)					   \
static unsigned								   \
color_##bits##_to_gray(stp_const_vars_t vars,				   \
		       const unsigned char *in,				   \
		       unsigned short *out)				   \
{									   \
  int i;								   \
  int i0 = -1;								   \
  int i1 = -1;								   \
  int i2 = -1;								   \
  int o0 = 0;								   \
  int nz = 0;								   \
  const T *s_in = (const T *) in;					   \
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	   \
  size_t count;								   \
  const unsigned short *composite;					   \
  int l_red = LUM_RED;							   \
  int l_green = LUM_GREEN;						   \
  int l_blue = LUM_BLUE;						   \
									   \
  stp_curve_resample(lut->composite, 1 << bits);			   \
  composite = stp_curve_get_ushort_data(lut->composite, &count);	   \
									   \
  if (lut->input_color_model == COLOR_MODEL_CMY)			   \
    {									   \
      l_red = (100 - l_red) / 2;					   \
      l_green = (100 - l_green) / 2;					   \
      l_blue = (100 - l_blue) / 2;					   \
    }									   \
									   \
  for (i = 0; i < lut->image_width; i++)				   \
    {									   \
      if (i0 != s_in[0] || i1 != s_in[1] || i2 != s_in[2])		   \
	{								   \
	  i0 = s_in[0];							   \
	  i1 = s_in[1];							   \
	  i2 = s_in[2];							   \
	  o0 = composite[(i0 * l_red + i1 * l_green + i2 * l_blue) / 100]; \
	  nz |= o0;							   \
	}								   \
      out[0] = o0;							   \
      s_in += 3;							   \
      out ++;								   \
    }									   \
  return nz == 0;							   \
}

COLOR_TO_GRAY_FUNC(unsigned char, 8)
COLOR_TO_GRAY_FUNC(unsigned short, 16)


#define CMYK_TO_GRAY_FUNC(T, size)					\
static unsigned								\
cmyk_##size##_to_gray(stp_const_vars_t vars,				\
		      const unsigned char *in,				\
		      unsigned short *out)				\
{									\
  int i;								\
  int j;								\
  int nz = 0;								\
  const T *s_in = (const T *) in;					\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
									\
  initialize_cmyk_lut(vars, (1<<size)-1);				\
									\
  for (i = 0; i < lut->image_width; i++, s_in += 4, out ++)		\
    {									\
      for (j = 0; j < 4; j++)						\
	{								\
	  nz |= s_in[0];						\
	  out[0] = lut->cmyk_lut[s_in[0]];				\
	}								\
    }									\
  return nz ? 1 : 0;							\
}

CMYK_TO_GRAY_FUNC(unsigned char, 8)
CMYK_TO_GRAY_FUNC(unsigned short, 16)

static unsigned
cmyk_16_to_gray_raw(stp_const_vars_t vars, const unsigned char *in,
		    unsigned short *out)
{
  int i;
  int nz[4];
  const unsigned short *usin = (const unsigned short *) in;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));

  memset(nz, 0, sizeof(nz));
  for (i = 0; i < lut->image_width; i++)
    {
      nz[0] |= usin[0];
      out[0] = usin[0];
      usin += 4;
      out += 1;
    }
  return nz[0] ? 1 : 0;
}

static unsigned
cmyk_16_to_kcmy_raw(stp_const_vars_t vars, const unsigned char *in,
		    unsigned short *out)
{
  int i;
  int j;
  unsigned retval = 0;
  int nz[4];
  const unsigned short *usin = (const unsigned short *) in;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));

  memset(nz, 0, sizeof(nz));
  for (i = 0; i < lut->image_width; i++)
    {
      out[0] = usin[3];
      out[1] = usin[0];
      out[2] = usin[1];
      out[3] = usin[2];
      for (j = 0; j < 4; j++)
	nz[j] |= out[j];
      usin += 4;
      out += 4;
    }
  for (j = 0; j < 4; j++)
    if (nz[j] == 0)
      retval |= (1 << j);
  return retval;
}

static unsigned
raw_to_raw(stp_const_vars_t vars, const unsigned char *in,
	   unsigned short *out)
{
  int i;
  int j;
  unsigned retval = 0;
  int nz[STP_CHANNEL_LIMIT];
  int colors;
  const unsigned short *usin = (const unsigned short *) in;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  colors = lut->image_bpp / 2;

  memset(nz, 0, sizeof(nz));
  for (i = 0; i < lut->image_width; i++)
    {
      for (j = 0; j < colors; j++)
	{
	  nz[j] |= usin[j];
	  out[j] = usin[j];
	}
      usin += colors;
      out += colors;
    }
  for (j = 0; j < colors; j++)
    if (nz[j] == 0)
      retval |= (1 << j);
  return retval;
}


static void
initialize_channels(stp_vars_t v, stp_image_t *image)
{
  lut_t *lut = (lut_t *)(stpi_get_component_data(v, "Color"));
  if (stp_check_float_parameter(v, "InkLimit", STP_PARAMETER_ACTIVE))
    stpi_channel_set_ink_limit(v, stp_get_float_parameter(v, "InkLimit"));
  stpi_channel_initialize(v, image, lut->out_channels);
  lut->channels_are_initialized = 1;
}

static int
stpi_color_traditional_get_row(stp_const_vars_t v,
			       stp_image_t *image,
			       int row,
			       unsigned *zero_mask)
{
  const lut_t *lut = (const lut_t *)(stpi_get_component_data(v, "Color"));
  unsigned zero;
  if (stpi_image_get_row(image, lut->in_data,
			 lut->image_width * lut->image_bpp, row)
      != STP_IMAGE_STATUS_OK)
    return 2;
  if (!lut->channels_are_initialized)
    initialize_channels((stp_vars_t)v, image);
  zero = (lut->colorfunc)(v, lut->in_data, stpi_channel_get_input(v));
  if (zero_mask)
    *zero_mask = zero;
  stpi_channel_convert(v, zero_mask);
  return 0;
}

static lut_t *
allocate_lut(void)
{
  lut_t *ret = stpi_malloc(sizeof(lut_t));
  ret->composite = stp_curve_create(STP_CURVE_WRAP_NONE);
  ret->black = stp_curve_create(STP_CURVE_WRAP_NONE);
  ret->cyan = stp_curve_create(STP_CURVE_WRAP_NONE);
  ret->magenta = stp_curve_create(STP_CURVE_WRAP_NONE);
  ret->yellow = stp_curve_create(STP_CURVE_WRAP_NONE);
  stp_curve_set_bounds(ret->composite, 0, 65535);
  stp_curve_set_bounds(ret->black, 0, 65535);
  stp_curve_set_bounds(ret->cyan, 0, 65535);
  stp_curve_set_bounds(ret->magenta, 0, 65535);
  stp_curve_set_bounds(ret->yellow, 0, 65535);
  ret->hue_map = NULL;
  ret->lum_map = NULL;
  ret->sat_map = NULL;
  ret->gcr_curve = NULL;
  ret->steps = 0;
  ret->image_bpp = 0;
  ret->image_width = 0;
  ret->in_data = NULL;
  ret->colorfunc = NULL;
  ret->cmy_tmp = NULL;
  ret->cmyk_lut = NULL;
  ret->channels_are_initialized = 0;
  return ret;
}

static void *
copy_lut(void *vlut)
{
  const lut_t *src = (const lut_t *)vlut;
  lut_t *dest;
  if (!src)
    return NULL;
  dest = allocate_lut();
  dest->composite = stp_curve_create_copy(src->composite);
  dest->cyan = stp_curve_create_copy(src->cyan);
  dest->magenta = stp_curve_create_copy(src->magenta);
  dest->yellow = stp_curve_create_copy(src->yellow);
  if (src->hue_map)
    dest->hue_map = stp_curve_create_copy(src->hue_map);
  if (src->lum_map)
    dest->lum_map = stp_curve_create_copy(src->lum_map);
  if (src->sat_map)
    dest->sat_map = stp_curve_create_copy(src->sat_map);
  if (src->gcr_curve)
    dest->gcr_curve = stp_curve_create_copy(src->gcr_curve);
  dest->steps = src->steps;
  dest->colorfunc = src->colorfunc;
  dest->image_bpp = src->image_bpp;
  dest->image_width = src->image_width;
  dest->input_color_model = src->input_color_model;
  dest->output_color_model = src->output_color_model;
  dest->cmy_tmp = NULL;		/* Don't copy working storage */
  if (src->in_data)
    {
      dest->in_data = stpi_malloc(src->image_width * src->image_bpp);
      memset(dest->in_data, 0, src->image_width * src->image_bpp);
    }
  else
    dest->in_data = NULL;
  return dest;
}

static void
free_lut(void *vlut)
{
  lut_t *lut = (lut_t *)vlut;
  if (lut->composite)
    stp_curve_free(lut->composite);
  if (lut->black)
    stp_curve_free(lut->black);
  if (lut->cyan)
    stp_curve_free(lut->cyan);
  if (lut->magenta)
    stp_curve_free(lut->magenta);
  if (lut->yellow)
    stp_curve_free(lut->yellow);
  if (lut->hue_map)
    stp_curve_free(lut->hue_map);
  if (lut->lum_map)
    stp_curve_free(lut->lum_map);
  if (lut->sat_map)
    stp_curve_free(lut->sat_map);
  if (lut->gcr_curve)
    stp_curve_free(lut->gcr_curve);
  SAFE_FREE(lut->in_data);
  SAFE_FREE(lut->cmy_tmp);
  SAFE_FREE(lut->cmyk_lut);
  memset(lut, 0, sizeof(lut_t));
  stpi_free(lut);
}

static stp_curve_t
compute_gcr_curve(stp_const_vars_t vars)
{
  stp_curve_t curve;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  double k_lower = 0.0;
  double k_upper = 1.0;
  double k_gamma = 1.0;
  double i_k_gamma = 1.0;
  double *tmp_data = stpi_malloc(sizeof(double) * lut->steps);
  int i;

  if (stp_check_float_parameter(vars, "GCRUpper", STP_PARAMETER_DEFAULTED))
    k_upper = stp_get_float_parameter(vars, "GCRUpper");
  if (stp_check_float_parameter(vars, "GCRLower", STP_PARAMETER_DEFAULTED))
    k_lower = stp_get_float_parameter(vars, "GCRLower");
  if (stp_check_float_parameter(vars, "BlackGamma", STP_PARAMETER_DEFAULTED))
    k_gamma = stp_get_float_parameter(vars, "BlackGamma");
  k_upper *= lut->steps;
  k_lower *= lut->steps;

  if (k_lower > lut->steps)
    k_lower = lut->steps;
  if (k_upper < k_lower)
    k_upper = k_lower + 1;
  i_k_gamma = 1.0 / k_gamma;

  for (i = 0; i < k_lower; i ++)
    tmp_data[i] = 0;
  if (k_upper < lut->steps)
    {
      for (i = ceil(k_lower); i < k_upper; i ++)
	{
	  double where = (i - k_lower) / (k_upper - k_lower);
	  double g1 = pow(where, i_k_gamma);
	  double g2 = 1.0 - pow(1.0 - where, k_gamma);
	  double value = (g1 > g2 ? g1 : g2);
	  tmp_data[i] = 65535.0 * k_upper * value / (double) (lut->steps - 1);
	  tmp_data[i] = floor(tmp_data[i] + .5);
	}
      for (i = ceil(k_upper); i < lut->steps; i ++)
	tmp_data[i] = 65535.0 * i / (double) (lut->steps - 1);
    }
  else if (k_lower < lut->steps)
    for (i = ceil(k_lower); i < lut->steps; i ++)
      {
	double where = (i - k_lower) / (k_upper - k_lower);
	double g1 = pow(where, i_k_gamma);
	double g2 = 1.0 - pow(1.0 - where, k_gamma);
	double value = (g1 > g2 ? g1 : g2);
	tmp_data[i] = 65535.0 * lut->steps * value / (double) (lut->steps - 1);
	tmp_data[i] = floor(tmp_data[i] + .5);
      }
  curve = stp_curve_create(STP_CURVE_WRAP_NONE);
  stp_curve_set_bounds(curve, 0, 65535);
  if (! stp_curve_set_data(curve, lut->steps, tmp_data))
    {
      stpi_eprintf(vars, "set curve data failed!\n");
      stpi_abort();
    }
  stpi_free(tmp_data);
  return curve;
}

static stp_curve_t
compute_a_curve(stp_curve_t curve, double c_gamma, lut_params_t *l)
{
  double *tmp = stpi_malloc(sizeof(double) * l->steps);
  double pivot = .25;
  double ipivot = 1.0 - pivot;
  double xcontrast = pow(l->contrast, l->contrast);
  double xgamma = pow(pivot, l->screen_gamma);
  int i;
  int isteps = l->steps;
  if (isteps > 256)
    isteps = 256;
  for (i = 0; i < isteps; i ++)
    {
      double temp_pixel, pixel;
      pixel = (double) i / (double) (isteps - 1);

      if (l->input_color_model == COLOR_MODEL_CMY)
	pixel = 1.0 - pixel;

      /*
       * First, correct contrast
       */
      if (pixel >= .5)
	temp_pixel = 1.0 - pixel;
      else
	temp_pixel = pixel;
      if (l->contrast > 3.99999)
	{
	  if (temp_pixel < .5)
	    temp_pixel = 0;
	  else
	    temp_pixel = 1;
	}
      if (temp_pixel <= .000001 && l->contrast <= .0001)
	temp_pixel = .5;
      else if (temp_pixel > 1)
	temp_pixel = .5 * pow(2 * temp_pixel, xcontrast);
      else if (temp_pixel < 1)
	{
	  if (l->linear_contrast_adjustment)
	    temp_pixel = 0.5 -
	      ((0.5 - .5 * pow(2 * temp_pixel, l->contrast)) * l->contrast);
	  else
	    temp_pixel = 0.5 -
	      ((0.5 - .5 * pow(2 * temp_pixel, l->contrast)));
	}
      if (temp_pixel > .5)
	temp_pixel = .5;
      else if (temp_pixel < 0)
	temp_pixel = 0;
      if (pixel < .5)
	pixel = temp_pixel;
      else
	pixel = 1 - temp_pixel;

      /*
       * Second, do brightness
       */
      if (l->brightness < 1)
	pixel = pixel * l->brightness;
      else
	pixel = 1 - ((1 - pixel) * (2 - l->brightness));

      /*
       * Third, correct for the screen gamma
       */

      pixel = 1.0 -
	(1.0 / (1.0 - xgamma)) *
	(pow(pivot + ipivot * pixel, l->screen_gamma) - xgamma);

      /*
       * Third, fix up cyan, magenta, yellow values
       */
      if (pixel < 0.0)
	pixel = 0.0;
      else if (pixel > 1.0)
	pixel = 1.0;

      if (pixel > .9999 && c_gamma < .00001)
	pixel = 0;
      else
	pixel = 1 - pow(1 - pixel, c_gamma);

      /*
       * Finally, fix up print gamma and scale
       */

      pixel = 65535 * pow(pixel, l->print_gamma);	/* was + 0.5 here */
      if (l->output_color_model == COLOR_MODEL_RGB)
	pixel = 65535 - pixel;

      if (pixel <= 0.0)
	tmp[i] = 0;
      else if (pixel >= 65535.0)
	tmp[i] = 65535;
      else
	tmp[i] = (pixel);
      tmp[i] = floor(tmp[i] + 0.5);			/* rounding is done here */
    }
  stp_curve_set_data(curve, isteps, tmp);
  if (isteps != l->steps)
    stp_curve_resample(curve, l->steps);
  stpi_free(tmp);
  return curve;
}

static void
invert_curve(stp_curve_t curve, int in_model, int out_model)
{
  double lo, hi;
  int i;
  size_t count;
  const double *data = stp_curve_get_data(curve, &count);
  double f_gamma = stp_curve_get_gamma(curve);
  double *tmp_data;

  stp_curve_get_bounds(curve, &lo, &hi);

  if (f_gamma)
    stp_curve_set_gamma(curve, -f_gamma);
  else
    {
      tmp_data = stpi_malloc(sizeof(double) * count);
      for (i = 0; i < count; i++)
	tmp_data[i] = data[count - i - 1];
      stp_curve_set_data(curve, count, tmp_data);
      stpi_free(tmp_data);
    }
  if (in_model == out_model)
    {
      stp_curve_rescale(curve, -1, STP_CURVE_COMPOSE_MULTIPLY,
			STP_CURVE_BOUNDS_RESCALE);
      stp_curve_rescale(curve, lo + hi, STP_CURVE_COMPOSE_ADD,
			STP_CURVE_BOUNDS_RESCALE);
    }
}

static void
compute_one_lut(stp_curve_t lut_curve, stp_const_curve_t curve,
		double density, lut_params_t *l)
{
  if (curve)
    {
      stp_curve_copy(lut_curve, curve);
      invert_curve(lut_curve, l->input_color_model, l->output_color_model);
      stp_curve_rescale(lut_curve, 65535.0, STP_CURVE_COMPOSE_MULTIPLY,
			STP_CURVE_BOUNDS_RESCALE);
      stp_curve_resample(lut_curve, l->steps);
    }
  else
    {
      compute_a_curve(lut_curve, density, l);
    }
}

static void
stpi_compute_lut(stp_vars_t v, size_t steps)
{
  stp_const_curve_t hue = NULL;
  stp_const_curve_t lum = NULL;
  stp_const_curve_t sat = NULL;
  stp_const_curve_t composite_curve = NULL;
  stp_const_curve_t cyan_curve = NULL;
  stp_const_curve_t magenta_curve = NULL;
  stp_const_curve_t yellow_curve = NULL;
  stp_const_curve_t black_curve = NULL;
  double cyan = 1.0;
  double magenta = 1.0;
  double yellow = 1.0;
  lut_t *lut;
  lut_params_t l;

  if (stp_check_float_parameter(v, "CyanGamma", STP_PARAMETER_DEFAULTED))
    cyan = stp_get_float_parameter(v, "CyanGamma");
  if (stp_check_float_parameter(v, "MagentaGamma", STP_PARAMETER_DEFAULTED))
    magenta = stp_get_float_parameter(v, "MagentaGamma");
  if (stp_check_float_parameter(v, "YellowGamma", STP_PARAMETER_DEFAULTED))
    yellow = stp_get_float_parameter(v, "YellowGamma");

  l.input_color_model = stp_get_input_color_model(v);
  l.output_color_model = stpi_get_output_color_model(v);
  l.steps = steps;
  l.linear_contrast_adjustment = 0;
  l.print_gamma = 1.0;
  l.app_gamma = 1.0;
  l.contrast = 1.0;
  l.brightness = 1.0;

  if (stp_check_boolean_parameter(v, "LinearContrast", STP_PARAMETER_DEFAULTED))
    l.linear_contrast_adjustment =
      stp_get_boolean_parameter(v, "LinearContrast");
  if (stp_check_float_parameter(v, "Gamma", STP_PARAMETER_DEFAULTED))
    l.print_gamma = stp_get_float_parameter(v, "Gamma");
  if (stp_check_float_parameter(v, "Contrast", STP_PARAMETER_DEFAULTED))
    l.contrast = stp_get_float_parameter(v, "Contrast");
  if (stp_check_float_parameter(v, "Brightness", STP_PARAMETER_DEFAULTED))
    l.brightness = stp_get_float_parameter(v, "Brightness");

  if (stp_check_float_parameter(v, "AppGamma", STP_PARAMETER_ACTIVE))
    l.app_gamma = stp_get_float_parameter(v, "AppGamma");
  l.screen_gamma = l.app_gamma / 4.0; /* "Empirical" */
  lut = allocate_lut();

  if (stp_check_curve_parameter(v, "HueMap", STP_PARAMETER_DEFAULTED))
    hue = stp_get_curve_parameter(v, "HueMap");
  if (stp_check_curve_parameter(v, "LumMap", STP_PARAMETER_DEFAULTED))
    lum = stp_get_curve_parameter(v, "LumMap");
  if (stp_check_curve_parameter(v, "SatMap", STP_PARAMETER_DEFAULTED))
    sat = stp_get_curve_parameter(v, "SatMap");
  if (stp_get_curve_parameter_active(v, "CompositeCurve") >=
      stp_get_float_parameter_active(v, "Gamma"))
    composite_curve = stp_get_curve_parameter(v, "CompositeCurve");
  if (stp_get_curve_parameter_active(v, "CyanCurve") >=
      stp_get_float_parameter_active(v, "Cyan"))
    cyan_curve = stp_get_curve_parameter(v, "CyanCurve");
  if (stp_get_curve_parameter_active(v, "MagentaCurve") >=
      stp_get_float_parameter_active(v, "Magenta"))
    magenta_curve = stp_get_curve_parameter(v, "MagentaCurve");
  if (stp_get_curve_parameter_active(v, "YellowCurve") >=
      stp_get_float_parameter_active(v, "Yellow"))
    yellow_curve = stp_get_curve_parameter(v, "YellowCurve");
  if (stp_check_curve_parameter(v, "BlackCurve", STP_PARAMETER_DEFAULTED))
    black_curve = stp_get_curve_parameter(v, "BlackCurve");

  /*
   * TODO check that these are wraparound curves and all that
   */
  if (hue)
    lut->hue_map = stp_curve_create_copy(hue);
  if (lum)
    lut->lum_map = stp_curve_create_copy(lum);
  if (sat)
    lut->sat_map = stp_curve_create_copy(sat);

  lut->steps = steps;
  lut->input_color_model = l.input_color_model;
  lut->output_color_model = l.output_color_model;

  stpi_allocate_component_data(v, "Color", copy_lut, free_lut, lut);
  stpi_dprintf(STPI_DBG_LUT, v, "stpi_compute_lut\n");
  stpi_dprintf(STPI_DBG_LUT, v, " cyan %.3f\n", cyan);
  stpi_dprintf(STPI_DBG_LUT, v, " magenta %.3f\n", magenta);
  stpi_dprintf(STPI_DBG_LUT, v, " yellow %.3f\n", yellow);
  stpi_dprintf(STPI_DBG_LUT, v, " print_gamma %.3f\n", l.print_gamma);
  stpi_dprintf(STPI_DBG_LUT, v, " contrast %.3f\n", l.contrast);
  stpi_dprintf(STPI_DBG_LUT, v, " brightness %.3f\n", l.brightness);
  stpi_dprintf(STPI_DBG_LUT, v, " screen_gamma %.3f\n", l.screen_gamma);

  compute_one_lut(lut->composite, composite_curve, 1.0, &l);

  if (black_curve)
    stp_curve_copy(lut->black, black_curve);
  else
    stp_curve_copy(lut->black, color_curve_bounds);
  stp_curve_rescale(lut->black, 65535.0, STP_CURVE_COMPOSE_MULTIPLY,
		    STP_CURVE_BOUNDS_RESCALE);
  stp_curve_resample(lut->black, steps);

  compute_one_lut(lut->cyan, cyan_curve, cyan, &l);

  compute_one_lut(lut->magenta, magenta_curve, magenta, &l);

  compute_one_lut(lut->yellow, yellow_curve, yellow, &l);
}

#define SET_COLORFUNC(x)						     \
if (!x)									     \
  stpi_erprintf("No colorfunc chosen ");				     \
stpi_dprintf(STPI_DBG_COLORFUNC, v,					     \
	     "at line %d stp_choose_colorfunc(type %d bpp %d) ==> %s, %d\n", \
	     __LINE__, stp_get_output_type(v), image_bpp, #x,		     \
	     lut->out_channels);					     \
lut->colorfunc = x;							     \
break

static int
stpi_color_traditional_init(stp_vars_t v,
			    stp_image_t *image,
			    size_t steps)
{
  const char *image_type = stp_get_string_parameter(v, "ImageType");
  const char *color_correction = stp_get_string_parameter(v, "ColorCorrection");
  int itype = 0;
  int image_bpp = stpi_image_bpp(image);
  lut_t *lut;
  if (steps != 256 && steps != 65536)
    return -1;

  stpi_compute_lut(v, steps);
  lut = (lut_t *)(stpi_get_component_data(v, "Color"));
  lut->image_bpp = image_bpp;
  lut->image_width = stpi_image_width(image);
  if (image_type && strcmp(image_type, "None") != 0)
    {
      if (strcmp(image_type, "Text") == 0)
	itype = 3;
      else
	itype = 2;
    }
  else if (color_correction)
    {
      if (strcmp(color_correction, "Uncorrected") == 0)
	itype = 0;
      else if (strcmp(color_correction, "Bright") == 0)
	itype = 1;
      else if (strcmp(color_correction, "Accurate") == 0)
	itype = 2;
      else if (strcmp(color_correction, "None") == 0)
	itype = 2;
      else if (strcmp(color_correction, "Threshold") == 0)
	itype = 3;
    }
  switch (stp_get_output_type(v))
    {
    case OUTPUT_RAW_CMYK:
      lut->out_channels = 4;
      switch (image_bpp)
	{
	case 1:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(gray_8_to_kcmy_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(gray_8_to_kcmy);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 2:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(gray_16_to_kcmy_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(gray_16_to_kcmy);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 3:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(rgb_8_to_kcmy_line_art);
	    case 2:
	    case 1:
	      SET_COLORFUNC(rgb_8_to_kcmy);
	    case 0:
	      SET_COLORFUNC(fast_rgb_8_to_kcmy);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 6:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(rgb_16_to_kcmy_line_art);
	    case 2:
	    case 1:
	      SET_COLORFUNC(rgb_16_to_kcmy);
	    case 0:
	      SET_COLORFUNC(fast_rgb_16_to_kcmy);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 4:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(cmyk_8_to_kcmy_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(cmyk_8_to_kcmy);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 8:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(cmyk_16_to_kcmy_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(cmyk_16_to_kcmy_raw);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	default:
	  SET_COLORFUNC(NULL);
	}
      break;
    case OUTPUT_COLOR:
      lut->out_channels = 3;
      switch (image_bpp)
	{
	case 1:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(gray_8_to_rgb_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(gray_8_to_rgb);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 2:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(gray_16_to_rgb_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(gray_16_to_rgb);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 3:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(rgb_8_to_rgb_line_art);
	    case 2:
	    case 1:
	      SET_COLORFUNC(rgb_8_to_rgb);
	    case 0:
	      SET_COLORFUNC(fast_rgb_8_to_rgb);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 6:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(rgb_16_to_rgb_line_art);
	    case 2:
	    case 1:
	      SET_COLORFUNC(rgb_16_to_rgb);
	    case 0:
	      SET_COLORFUNC(fast_rgb_16_to_rgb);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	default:
	  SET_COLORFUNC(NULL);
	}
      break;
    case OUTPUT_RAW_PRINTER:
      if ((image_bpp & 1) || image_bpp < 2 || image_bpp > 64)
	{
	  SET_COLORFUNC(NULL);
	}
      lut->out_channels = image_bpp / 2;
      SET_COLORFUNC(raw_to_raw);
    case OUTPUT_GRAY:
      lut->out_channels = 1;
      switch (image_bpp)
	{
	case 1:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(gray_8_to_gray_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(gray_8_to_gray);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 2:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(gray_16_to_gray_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(gray_16_to_gray);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 3:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(color_8_to_gray_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(color_8_to_gray);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 6:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(color_16_to_gray_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(color_16_to_gray);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 4:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(cmyk_8_to_gray_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(cmyk_8_to_gray);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 8:
	  switch (itype)
	    {
	    case 3:
	      SET_COLORFUNC(cmyk_16_to_gray_line_art);
	    case 2:
	    case 1:
	    case 0:
	      SET_COLORFUNC(cmyk_16_to_gray_raw);
	    default:
	      SET_COLORFUNC(NULL);
	    }
	  break;
	default:
	  SET_COLORFUNC(NULL);
	}
      break;
    default:
      SET_COLORFUNC(NULL);
    }
  lut->in_data = stpi_malloc(stpi_image_width(image) * image_bpp);
  memset(lut->in_data, 0, stpi_image_width(image) * image_bpp);
  return lut->out_channels;
}

static void
initialize_standard_curves(void)
{
  if (!standard_curves_initialized)
    {
      int i;
      hue_map_bounds = stp_curve_create_from_string
	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 "<gimp-print>\n"
	 "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
	 "<sequence count=\"2\" lower-bound=\"-6\" upper-bound=\"6\">\n"
	 "0 0\n"
	 "</sequence>\n"
	 "</curve>\n"
	 "</gimp-print>");
      lum_map_bounds = stp_curve_create_from_string
	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 "<gimp-print>\n"
	 "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
	 "<sequence count=\"2\" lower-bound=\"0\" upper-bound=\"4\">\n"
	 "1 1\n"
	 "</sequence>\n"
	 "</curve>\n"
	 "</gimp-print>");
      sat_map_bounds = stp_curve_create_from_string
	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 "<gimp-print>\n"
	 "<curve wrap=\"wrap\" type=\"linear\" gamma=\"0\">\n"
	 "<sequence count=\"2\" lower-bound=\"0\" upper-bound=\"4\">\n"
	 "1 1\n"
	 "</sequence>\n"
	 "</curve>\n"
	 "</gimp-print>");
      color_curve_bounds = stp_curve_create_from_string
	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 "<gimp-print>\n"
	 "<curve wrap=\"nowrap\" type=\"linear\" gamma=\"1.0\">\n"
	 "<sequence count=\"0\" lower-bound=\"0\" upper-bound=\"1\">\n"
	 "</sequence>\n"
	 "</curve>\n"
	 "</gimp-print>");
      gcr_curve_bounds = stp_curve_create_from_string
	("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
	 "<gimp-print>\n"
	 "<curve wrap=\"nowrap\" type=\"linear\" gamma=\"0.0\">\n"
	 "<sequence count=\"2\" lower-bound=\"0\" upper-bound=\"1\">\n"
	 "1 1\n"
	 "</sequence>\n"
	 "</curve>\n"
	 "</gimp-print>");
      for (i = 0; i < curve_parameter_count; i++)
	curve_parameters[i].param.deflt.curve =
	 *(curve_parameters[i].defval);
      standard_curves_initialized = 1;
    }
}

static stp_parameter_list_t
stpi_color_traditional_list_parameters(stp_const_vars_t v)
{
  stpi_list_t *ret = stp_parameter_list_create();
  int i;
  initialize_standard_curves();
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  for (i = 0; i < curve_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(curve_parameters[i].param));
  return ret;
}

static void
stpi_color_traditional_describe_parameter(stp_const_vars_t v,
					  const char *name,
					  stp_parameter_t *description)
{
  int i;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  initialize_standard_curves();
  if (name == NULL)
    return;
  for (i = 0; i < float_parameter_count; i++)
    {
      const float_param_t *param = &(float_parameters[i]);
      if (strcmp(name, param->param.name) == 0)
	{
	  stpi_fill_parameter_settings(description, &(param->param));
	  if (param->color_only && stp_get_output_type(v) == OUTPUT_GRAY)
	    description->is_active = 0;
	  if (stp_check_string_parameter(v, "ImageType", STP_PARAMETER_ACTIVE) &&
	      strcmp(stp_get_string_parameter(v, "ImageType"), "None") != 0 &&
	      description->p_level > STP_PARAMETER_LEVEL_BASIC)
	    description->is_active = 0;
	  switch (param->param.p_type)
	    {
	    case STP_PARAMETER_TYPE_BOOLEAN:
	      description->deflt.boolean = (int) param->defval;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      description->bounds.dbl.upper = param->max;
	      description->bounds.dbl.lower = param->min;
	      description->deflt.dbl = param->defval;
	      if (strcmp(name, "InkLimit") == 0)
		{
		  stp_parameter_t ink_limit_desc;
		  stp_describe_parameter(v, "InkChannels", &ink_limit_desc);
		  if (ink_limit_desc.p_type == STP_PARAMETER_TYPE_INT)
		    {
		      if (ink_limit_desc.deflt.integer > 1)
			{
			  description->bounds.dbl.upper =
			    ink_limit_desc.deflt.integer;
			  description->deflt.dbl =
			    ink_limit_desc.deflt.integer;
			}
		      else
			description->is_active = 0;
		    }
		  stp_parameter_description_free(&ink_limit_desc);
		}
	      break;
	    case STP_PARAMETER_TYPE_STRING_LIST:
	      if (!strcmp(param->param.name, "ColorCorrection"))
		{
		  description->bounds.str = stp_string_list_create();
		  stp_string_list_add_string
		    (description->bounds.str, "None", _("Default"));
		  stp_string_list_add_string
		    (description->bounds.str, "Accurate", _("High Accuracy"));
		  stp_string_list_add_string
		    (description->bounds.str, "Bright", _("Bright"));
		  stp_string_list_add_string
		    (description->bounds.str, "Threshold", _("Threshold"));
		  stp_string_list_add_string
		    (description->bounds.str, "Uncorrected", _("Uncorrected"));
		  description->deflt.str = "None";
		}
	      break;
	    default:
	      break;
	    }
	  return;
	}
    }
  for (i = 0; i < curve_parameter_count; i++)
    {
      curve_param_t *param = &(curve_parameters[i]);
      if (strcmp(name, param->param.name) == 0)
	{
	  description->is_active = 1;
	  stpi_fill_parameter_settings(description, &(param->param));
	  if (param->color_only && stp_get_output_type(v) == OUTPUT_GRAY)
	    description->is_active = 0;
	  if (stp_check_string_parameter(v, "ImageType", STP_PARAMETER_ACTIVE) &&
	      strcmp(stp_get_string_parameter(v, "ImageType"), "None") != 0 &&
	      description->p_level > STP_PARAMETER_LEVEL_BASIC)
	    description->is_active = 0;
	  if (param->hsl_only &&
	      stp_check_string_parameter(v, "ColorCorrection",
					 STP_PARAMETER_DEFAULTED) &&
	      strcmp(stp_get_string_parameter(v, "ColorCorrection"),
		     "Uncorrected") == 0)
	    description->is_active = 0;
	  switch (param->param.p_type)
	    {
	    case STP_PARAMETER_TYPE_CURVE:
	      description->deflt.curve = *(param->defval);
	      description->bounds.curve =
		stp_curve_create_copy(*(param->defval));
	      break;
	    default:
	      break;
	    }
	  return;
	}
    }
}


static const stpi_colorfuncs_t stpi_color_traditional_colorfuncs =
{
  &stpi_color_traditional_init,
  &stpi_color_traditional_get_row,
  &stpi_color_traditional_list_parameters,
  &stpi_color_traditional_describe_parameter
};

static const stpi_internal_color_t stpi_color_traditional_module_data =
  {
    COOKIE_COLOR,
    "traditional",
    N_("Traditional Gimp-Print color conversion"),
    &stpi_color_traditional_colorfuncs
  };


static int
color_traditional_module_init(void)
{
  return stpi_color_register(&stpi_color_traditional_module_data);
}


static int
color_traditional_module_exit(void)
{
  return stpi_color_unregister(&stpi_color_traditional_module_data);
}


/* Module header */
#define stpi_module_version color_traditional_LTX_stpi_module_version
#define stpi_module_data color_traditional_LTX_stpi_module_data

stpi_module_version_t stpi_module_version = {0, 0};

stpi_module_t stpi_module_data =
  {
    "traditional",
    VERSION,
    "Traditional Gimp-Print color conversion",
    STPI_MODULE_CLASS_COLOR,
    NULL,
    color_traditional_module_init,
    color_traditional_module_exit,
    (void *) &stpi_color_traditional_module_data
  };
