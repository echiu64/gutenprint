/*
 * "$Id$"
 *
 *   Print plug-in color management for the GIMP.
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
#include "xml.h"

#ifdef __GNUC__
#define inline __inline__
#endif

/* Color conversion function */
typedef void (*stp_convert_t) (const stp_vars_t vars, const unsigned char *in,
                               unsigned short *out, int *zero_mask,
                               int width, int bpp);

typedef struct
{
  unsigned steps;
  unsigned char *in_data;
  int image_bpp;
  int image_width;
  stp_convert_t colorfunc;
  stp_curve_t composite;
  stp_curve_t black;
  stp_curve_t cyan;
  stp_curve_t magenta;
  stp_curve_t yellow;
  stp_curve_t hue_map;
  stp_curve_t lum_map;
  stp_curve_t sat_map;
  stp_curve_t gcr_curve;
  unsigned short *cmy_tmp;
} lut_t;

typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
  int color_only;
} float_param_t;

static float_param_t float_parameters[] =
{
  {
    {
      "ImageOptimization", N_("Image Type"),
      N_("Optimize the settings for the type of image to be printed"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
    }, 0.0, 0.0, 0.0, 0
  },
  {
    {
      "Brightness", N_("Brightness"),
      N_("Brightness of the print (0 is solid black, 2 is solid white)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
    }, 0.0, 2.0, 1.0, 0
  },
  {
    {
      "Contrast", N_("Contrast"),
      N_("Contrast of the print (0 is solid gray)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
    }, 0.0, 4.0, 1.0, 0
  },
  {
    {
      "LinearContrast", N_("Linear Contrast Adjustment"),
      N_("Use linear vs. fixed end point contrast adjustment"),
      STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, -1
    }, 0.0, 0.0, 0.0, 0
  },
  {
    {
      "Gamma", N_("Gamma"),
      N_("Adjust the gamma of the print. Larger values will "
	 "produce a generally brighter print, while smaller "
	 "values will produce a generally darker print. "
	 "Black and white will remain the same, unlike with "
	 "the brightness adjustment."),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, -1
    }, 0.1, 4.0, 1.0, 0
  },
  {
    {
      "AppGamma", N_("AppGamma"),
      N_("Gamma value assumed by application"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED5, 0, 1, -1
    }, 0.1, 4.0, 1.0, 0
  },
  {
    {
      "Cyan", N_("Cyan"),
      N_("Adjust the cyan gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 1
    }, 0.0, 4.0, 1.0, 1
  },
  {
    {
      "Magenta", N_("Magenta"),
      N_("Adjust the magenta gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 2
    }, 0.0, 4.0, 1.0, 1
  },
  {
    {
      "Yellow", N_("Yellow"),
      N_("Adjust the yellow gamma"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 1, 1, 3
    }, 0.0, 4.0, 1.0, 1
  },
  {
    {
      "CyanDensity", N_("Cyan Balance"),
      N_("Adjust the cyan balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 1
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "MagentaDensity", N_("Magenta Balance"),
      N_("Adjust the magenta balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 2
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "YellowDensity", N_("Yellow Balance"),
      N_("Adjust the yellow balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 3
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "BlackDensity", N_("Black Balance"),
      N_("Adjust the black balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "Saturation", N_("Saturation"),
      N_("Adjust the saturation (color balance) of the print\n"
	 "Use zero saturation to produce grayscale output "
	 "using color and black inks"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1, 1, -1
    }, 0.0, 9.0, 1.0, 1
  },
  {
    {
      "Black", N_("GCR Transition"),
      N_("Adjust the gray component transition rate"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0
    }, 0.0, 1.0, 1.0, 1
  },
  {
    {
      "GCRLower", N_("GCR Lower Bound"),
      N_("Lower bound of gray component reduction"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0
    }, 0.0, 1.0, 0.2, 1
  },
  {
    {
      "GCRUpper", N_("GCR Upper Bound"),
      N_("Upper bound of gray component reduction"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, 0
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
      "CompositeCurve", N_("Composite Curve"),
      N_("Composite (Grayscale) curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, -1
    }, &color_curve_bounds, 0, 0
  },
  {
    {
      "CyanCurve", N_("Cyan Curve"),
      N_("Cyan curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 1
    }, &color_curve_bounds, 1, 0
  },
  {
    {
      "MagentaCurve", N_("Magenta Curve"),
      N_("Magenta curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 2
    }, &color_curve_bounds, 1, 0
  },
  {
    {
      "YellowCurve", N_("Yellow Curve"),
      N_("Yellow curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 3
    }, &color_curve_bounds, 1, 0
  },
  {
    {
      "BlackCurve", N_("Black Curve"),
      N_("Black curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED2, 0, 1, 0
    }, &color_curve_bounds, 1, 0
  },
  {
    {
      "HueMap", N_("Hue Map"),
      N_("Hue adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, -1
    }, &hue_map_bounds, 1, 1
  },
  {
    {
      "SatMap", N_("Saturation Map"),
      N_("Saturation adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, -1
    }, &sat_map_bounds, 1, 1
  },
  {
    {
      "LumMap", N_("Luminosity Map"),
      N_("Luminosity adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, -1
    }, &lum_map_bounds, 1, 1
  },
  {
    {
      "GCRCurve", N_("Gray Component Reduction"),
      N_("Gray component reduction curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED3, 0, 1, 0
    }, &gcr_curve_bounds, 1, 0
  },
};

static const int curve_parameter_count =
sizeof(curve_parameters) / sizeof(curve_param_t);

/*
 * RGB to grayscale luminance constants...
 */

#define LUM_RED		31
#define LUM_GREEN	61
#define LUM_BLUE	8

/* rgb/hsl conversions taken from Gimp common/autostretch_hsv.c */

#define FMAX(a, b) ((a) > (b) ? (a) : (b))
#define FMIN(a, b) ((a) < (b) ? (a) : (b))


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

static inline void
update_cmyk(unsigned short *rgb)
{
  int c = 65535 - rgb[0];
  int m = 65535 - rgb[1];
  int y = 65535 - rgb[2];
  int nc, nm, ny;
  int k;
  if (c == m && c == y)
    return;
  k = FMIN(FMIN(c, m), y);

  /*
   * This is an attempt to achieve better color balance.  The goal
   * is to weaken the pure cyan, magenta, and yellow and strengthen
   * pure red, green, and blue.
   *
   * We also don't want S=1 V=1 cyan to be 100% cyan; it's simply
   * too dark.
   */

  nc = (c * 3 + FMIN(c, FMAX(m, y)) * 4 + FMAX(m, y) * 0 + k) / 8;
  nm = (m * 3 + FMIN(m, FMAX(c, y)) * 4 + FMAX(c, y) * 0 + k) / 8;
  ny = (y * 3 + FMIN(y, FMAX(c, m)) * 4 + FMAX(c, m) * 0 + k) / 8;

  /*
   * Make sure we didn't go overboard.  We don't want to go too
   * close to white unnecessarily.
   */
  nc = c + (nc - c) / 3;
  nm = m + (nm - m) / 3;
  ny = y + (ny - y) / 3;

  if (nc > 65535)
    nc = 65535;
  if (nm > 65535)
    nm = 65535;
  if (ny > 65535)
    ny = 65535;

  rgb[0] = 65535 - nc;
  rgb[1] = 65535 - nm;
  rgb[2] = 65535 - ny;
}

/*
 * 'gray_to_gray()' - Convert grayscale image data to grayscale (brightness
 *                    adjusted).
 */

static void
gray_to_gray(const stp_vars_t vars,
	     const unsigned char *grayin,
	     unsigned short *grayout,
	     int *zero_mask,
	     int width,
	     int bpp)
{
  int i0 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);

  while (width > 0)
    {
      if (i0 != grayin[0])
	{
	  i0 = grayin[0];
	  o0 = composite[i0];
	  nz |= o0;
	}
      grayout[0] = o0;
      grayin ++;
      grayout ++;
      width --;
    }
  if (zero_mask)
    *zero_mask = nz ? 0 : 1;
}

/*
 * 'rgb_to_gray()' - Convert RGB image data to grayscale.
 */

static void
rgb_to_gray(const stp_vars_t vars,
	    const unsigned char *rgb,
	    unsigned short *gray,
	    int *zero_mask,
	    int width,
	    int bpp)
{
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);

  if (width <= 0)
    return;
  while (width)
    {
      if (i0 != rgb[0] || i1 != rgb[1] || i2 != rgb[2])
	{
	  i0 = rgb[0];
	  i1 = rgb[1];
	  i2 = rgb[2];
	  o0 = composite[(i0 * LUM_RED + i1 * LUM_GREEN + i2 * LUM_BLUE) /
			 100];
	  nz |= o0;
	}
      gray[0] = o0;
      rgb += 3;
      gray ++;
      width --;
    }
  if (zero_mask)
    *zero_mask = nz ? 0 : 1;
}

static inline double
update_saturation(double sat, double adjust, double isat)
{
  if (adjust < 1)
    sat *= adjust;
  else
    {
      double s1 = sat * adjust;
      double s2 = 1.0 - ((1.0 - adjust) * isat);
      sat = FMIN(s1, s2);
    }
  if (sat > 1)
    sat = 1.0;
  return sat;
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
adjust_hue(lut_t *lut, size_t h_points, double h)
{
  if (lut->hue_map)
    {
      double nh = h * h_points / 6.0;
      double tmp;
      if (stp_curve_interpolate_value(lut->hue_map, nh, &tmp))
	{
	  h += tmp;
	  if (h < 0.0)
	    h += 6.0;
	  else if (h >= 6.0)
	    h -= 6.0;
	}
    }
  return h;
}

static inline double
adjust_lum(lut_t *lut, size_t l_points, double l, double s, double h)
{
  if (lut->lum_map && l > .0001 && l < .9999)
    {
      double nh = h * l_points / 6.0;
      double tmp;
      if (stp_curve_interpolate_value(lut->lum_map, nh, &tmp) &&
	  (tmp < .9999 || tmp > 1.0001))
	{
	  double el = tmp;
	  el = 1.0 + (s * (el - 1.0));
	  if (l > .5)
	    el = 1.0 + ((2.0 * (1.0 - l)) * (el - 1.0));
	  l = 1.0 - pow(1.0 - l, el);
	}
    }
  return l;
}

static inline double
adjust_sat(lut_t *lut, size_t s_points, double s, double h)
{
  if (lut->sat_map)
    {
      double tmp;
      double nh = h * s_points / 6.0;
      if (stp_curve_interpolate_value(lut->sat_map, nh, &tmp) &&
	  (tmp < .9999 || tmp > 1.0001))
	{
	  s = update_saturation(s, tmp, tmp > 1.0 ? 1.0 / tmp : 1.0);
	}
    }
  return s;
}

static inline void
adjust_hsl(unsigned short *rgbout, lut_t *lut, int do_lum, double ssat,
	   double isat, size_t h_points, size_t s_points, size_t l_points,
	   int split_saturation)
{
  if ((split_saturation || lut->hue_map || (do_lum && lut->lum_map) ||
       lut->sat_map) &&
      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
    {
      double h, s, l;
      calc_rgb_to_hsl(rgbout, &h, &s, &l);
      s = update_saturation(s, ssat, isat);
      if (lut->hue_map || lut->lum_map || lut->sat_map)
	{
	  h = adjust_hue(lut, h_points, h);
	  if (do_lum)
	    l = adjust_lum(lut, l_points, l, s, h);
	  s = adjust_sat(lut, s_points, s, h);
	}
      calc_hsl_to_rgb(rgbout, h, s, l);
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

/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

static void
rgb_to_rgb(const stp_vars_t vars,
	   const unsigned char *rgbin,
	   unsigned short *rgbout,
	   int *zero_mask,
	   int width,
	   int bpp)
{
  double isat = 1.0;
  double ssat = stp_get_float_parameter(vars, "Saturation");
  size_t count;
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  int compute_saturation = ssat <= .99999 || ssat >= 1.00001;
  int split_saturation = ssat > 1.4;
  const unsigned short *red = stp_curve_get_ushort_data(lut->cyan, &count);
  const unsigned short *green = stp_curve_get_ushort_data(lut->magenta, &count);
  const unsigned short *blue = stp_curve_get_ushort_data(lut->yellow, &count);
  size_t h_points = 0;
  size_t l_points = 0;
  size_t s_points = 0;
  int do_update_cmyk = 1;
  int do_adjust_lum = 1;

  if (strcmp(stp_get_string_parameter(vars, "ImageOptimization"),
	     "HSLAdjust") == 0)
    do_update_cmyk = 0;
  if (strcmp(stp_get_string_parameter(vars, "ImageOptimization"),
	     "Solid") == 0)
    do_adjust_lum = 0;

  if (lut->hue_map)
    h_points = stp_curve_count_points(lut->hue_map);
  if (lut->lum_map)
    l_points = stp_curve_count_points(lut->lum_map);
  if (lut->sat_map)
    s_points = stp_curve_count_points(lut->sat_map);

  if (split_saturation)
    ssat = sqrt(ssat);
  if (ssat > 1)
    isat = 1.0 / ssat;
  while (width > 0)
    {
      if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2])
	{
	  rgbout[0] = o0;
	  rgbout[1] = o1;
	  rgbout[2] = o2;
	}
      else
	{
	  i0 = rgbin[0];
	  i1 = rgbin[1];
	  i2 = rgbin[2];
	  rgbout[0] = i0 | (i0 << 8);
	  rgbout[1] = i1 | (i1 << 8);
	  rgbout[2] = i2 | (i2 << 8);
	  if ((compute_saturation) &&
	      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
	    update_saturation_from_rgb(rgbout, ssat, isat);
	  if (do_update_cmyk)
	    update_cmyk(rgbout);	/* Fiddle with the INPUT */
	  lookup_rgb(lut, rgbout, red, green, blue);
	  adjust_hsl(rgbout, lut, do_adjust_lum, ssat, isat, h_points,
		     s_points, l_points, split_saturation);
	  o0 = rgbout[0];
	  o1 = rgbout[1];
	  o2 = rgbout[2];
	  nz0 |= o0;
	  nz1 |= o1;
	  nz2 |= o2;
	}
      rgbin += bpp;
      rgbout += 3;
      width --;
    }
  if (zero_mask)
    {
      *zero_mask = nz0 ? 0 : 1;
      *zero_mask |= nz1 ? 0 : 2;
      *zero_mask |= nz2 ? 0 : 4;
    }
}

/*
 * 'gray_to_rgb()' - Convert gray image data to RGB.
 */

static void
gray_to_rgb(const stp_vars_t vars,
	    const unsigned char	*grayin,
	    unsigned short *rgbout,
	    int *zero_mask,
	    int width,
	    int bpp)
{
  int i0 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  size_t count;
  const unsigned short *red;
  const unsigned short *green;
  const unsigned short *blue;
  int do_update_cmyk = 1;
  if (strcmp(stp_get_string_parameter(vars, "ImageOptimization"),
	     "HSLAdjust") == 0)
    do_update_cmyk = 0;

  red = stp_curve_get_ushort_data(lut->cyan, &count);
  green = stp_curve_get_ushort_data(lut->magenta, &count);
  blue = stp_curve_get_ushort_data(lut->yellow, &count);

  while (width > 0)
    {
      if (i0 == grayin[0])
	{
	  rgbout[0] = o0;
	  rgbout[1] = o1;
	  rgbout[2] = o2;
	}
      else
	{
	  i0 = grayin[0];
	  rgbout[0] =
	    rgbout[1] =
	    rgbout[2] = i0 | (i0 << 8);
	  if (do_update_cmyk)
	    update_cmyk(rgbout);
	  lookup_rgb(lut, rgbout, red, green, blue);
	  o0 = rgbout[0];
	  o1 = rgbout[1];
	  o2 = rgbout[2];
	  nz0 |= o0;
	  nz1 |= o1;
	  nz2 |= o2;
	}
      grayin += bpp;
      rgbout += 3;
      width --;
    }
  if (zero_mask)
    {
      *zero_mask = nz0 ? 0 : 1;
      *zero_mask |= nz1 ? 0 : 2;
      *zero_mask |= nz2 ? 0 : 4;
    }
}


/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

static void
fast_rgb_to_rgb(const stp_vars_t vars,
		const unsigned char *rgbin,
		unsigned short *rgbout,
		int *zero_mask,
		int width,
		int bpp)
{
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  size_t count;
  const unsigned short *red;
  const unsigned short *green;
  const unsigned short *blue;
  double isat = 1.0;
  double saturation = stp_get_float_parameter(vars, "Saturation");

  stp_curve_resample(lut->cyan, 256);
  stp_curve_resample(lut->magenta, 256);
  stp_curve_resample(lut->yellow, 256);
  red = stp_curve_get_ushort_data(lut->cyan, &count);
  green = stp_curve_get_ushort_data(lut->magenta, &count);
  blue = stp_curve_get_ushort_data(lut->yellow, &count);

  if (saturation > 1)
    isat = 1.0 / saturation;
  while (width > 0)
    {
      if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2])
	{
	  rgbout[0] = o0;
	  rgbout[1] = o1;
	  rgbout[2] = o2;
	}
      else
	{
	  i0 = rgbin[0];
	  i1 = rgbin[1];
	  i2 = rgbin[2];
	  rgbout[0] = red[rgbin[0]];
	  rgbout[1] = green[rgbin[1]];
	  rgbout[2] = blue[rgbin[2]];
	  if (saturation != 1.0)
	    update_saturation_from_rgb(rgbout, saturation, isat);
	  o0 = rgbout[0];
	  o1 = rgbout[1];
	  o2 = rgbout[2];
	  nz0 |= o0;
	  nz1 |= o1;
	  nz2 |= o2;
	}
      rgbin += bpp;
      rgbout += 3;
      width --;
    }
  if (zero_mask)
    {
      *zero_mask = nz0 ? 0 : 1;
      *zero_mask |= nz1 ? 0 : 2;
      *zero_mask |= nz2 ? 0 : 4;
    }
}

/*
 * 'gray_to_rgb()' - Convert gray image data to RGB.
 */

static void
fast_gray_to_rgb(const stp_vars_t vars,
		 const unsigned char *grayin,
		 unsigned short *rgbout,
		 int *zero_mask,
		 int width,
		 int bpp)
{
  int i0 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  size_t count;
  const unsigned short *red;
  const unsigned short *green;
  const unsigned short *blue;

  stp_curve_resample(lut->cyan, 256);
  stp_curve_resample(lut->magenta, 256);
  stp_curve_resample(lut->yellow, 256);
  red = stp_curve_get_ushort_data(lut->cyan, &count);
  green = stp_curve_get_ushort_data(lut->magenta, &count);
  blue = stp_curve_get_ushort_data(lut->yellow, &count);

  while (width > 0)
    {
      if (i0 == grayin[0])
	{
	  rgbout[0] = o0;
	  rgbout[1] = o1;
	  rgbout[2] = o2;
	}
      else
	{
	  i0 = grayin[0];
	  rgbout[0] = red[grayin[0]];
	  rgbout[1] = green[grayin[0]];
	  rgbout[2] = blue[grayin[0]];
	  o0 = rgbout[0];
	  o1 = rgbout[1];
	  o2 = rgbout[2];
	  nz0 |= o0;
	  nz1 |= o1;
	  nz2 |= o2;
	}
      grayin += bpp;
      rgbout += 3;
      width --;
    }
  if (zero_mask)
    {
      *zero_mask = nz0 ? 0 : 1;
      *zero_mask |= nz1 ? 0 : 2;
      *zero_mask |= nz2 ? 0 : 4;
    }
}

static stp_curve_t
compute_gcr_curve(const stp_vars_t vars)
{
  stp_curve_t curve;
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  double k_lower = 0.0;
  double k_upper = 1.0;
  double k_gamma = 1.0;
  double *tmp_data = stpi_malloc(sizeof(double) * lut->steps);
  int i;

  if (stp_check_float_parameter(vars, "GCRUpper", STP_PARAMETER_DEFAULTED))
    k_upper = stp_get_float_parameter(vars, "GCRUpper");
  if (stp_check_float_parameter(vars, "GCRLower", STP_PARAMETER_DEFAULTED))
    k_lower = stp_get_float_parameter(vars, "GCRLower");
  if (stp_check_float_parameter(vars, "Black", STP_PARAMETER_DEFAULTED))
    k_gamma = stp_get_float_parameter(vars, "Black");
  k_upper *= lut->steps;
  k_lower *= lut->steps;

  if (k_lower > lut->steps)
    k_lower = lut->steps;
  if (k_upper < k_lower)
    k_upper = k_lower + 1;

  for (i = 0; i < k_lower; i ++)
    tmp_data[i] = 0;
  if (k_upper < lut->steps)
    {
      for (i = ceil(k_lower); i < k_upper; i ++)
	{
	  double where = (i - k_lower) / (k_upper - k_lower);
	  tmp_data[i] = 65535.0 * k_upper * (1.0 - pow(1.0 - where, k_gamma)) /
	    (double) (lut->steps - 1);
	}
      for (i = ceil(k_upper); i < lut->steps; i ++)
	tmp_data[i] = 65535.0 * i / (double) (lut->steps - 1);
    }
  else if (k_lower < lut->steps)
    for (i = ceil(k_lower); i < lut->steps; i ++)
      {
	double where = (i - k_lower) / (k_upper - k_lower);
	tmp_data[i] = 65535.0 * k_upper * pow(where, k_gamma) /
	  (double) (lut->steps - 1);
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

static void
generic_rgb_to_cmyk(const stp_vars_t vars,
		    const unsigned short *in,
		    unsigned short *out,
		    int *zero_mask,
		    int width,
		    int bpp)
{
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));
  int step = 65535 / (lut->steps - 1); /* 1 or 257 */

  const unsigned short *gcr_lookup;
  const unsigned short *black_lookup;
  size_t points;
  int i;

  if (!lut->gcr_curve)
    {
      if (stp_check_curve_parameter(vars, "GCRCurve", STP_PARAMETER_DEFAULTED))
	{
	  int i;
	  double data;
	  size_t count;
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

  gcr_lookup = stp_curve_get_ushort_data(lut->gcr_curve, &points);
  stp_curve_resample(lut->black, lut->steps);
  black_lookup = stp_curve_get_ushort_data(lut->black, &points);

  for (i = 0; i < width; i++, out += 4, in += 3)
    {
      int j;
      int c = in[0];
      int m = in[1];
      int y = in[2];
      int k = FMIN(c, FMIN(m, y));
      for (j = 0; j < 3; j++)
	out[j] = in[j];
      if (k == 0)
	out[3] = 0;
      else
	{
	  int where, resid;
	  int kk;
	  if (lut->steps == 65536)
	    kk = gcr_lookup[k];
	  else
	    {
	      where = k / step;
	      resid = k % step;
	      if (resid == 0)
		kk = gcr_lookup[where];
	      else
		kk = gcr_lookup[where] +
		  (gcr_lookup[where + 1] - gcr_lookup[where]) * resid / step;
	    }
	  if (kk > k)
	    kk = k;
	  if (lut->steps == 65536)
	    out[3] = black_lookup[kk];
	  else
	    {
	      where = kk / step;
	      resid = kk % step;
	      if (resid)
		out[3] = black_lookup[where] +
		  (black_lookup[where + 1] - black_lookup[where]) * resid /
		  step;
	      else
		out[3] = black_lookup[where];
	    }
	  for (j = 0; j < 3; j++)
	    out[j] -= kk;
	}
    }
}

#define RGB_TO_CMYK_FUNC(name)						\
static void								\
name##_to_cmyk(const stp_vars_t vars,					\
	       const unsigned char *in,					\
	       unsigned short *out,					\
	       int *zero_mask,						\
	       int width,						\
	       int bpp)							\
{									\
  lut_t *lut = (lut_t *)(stpi_get_component_data(vars, "Color"));	\
  if (!lut->cmy_tmp)							\
    lut->cmy_tmp = stpi_malloc(4 * 2 * width);				\
  name##_to_rgb(vars, in, lut->cmy_tmp, zero_mask, width, bpp);		\
  generic_rgb_to_cmyk(vars, lut->cmy_tmp, out, zero_mask, width, bpp);	\
}

RGB_TO_CMYK_FUNC(gray)
RGB_TO_CMYK_FUNC(fast_gray)
RGB_TO_CMYK_FUNC(rgb)
RGB_TO_CMYK_FUNC(fast_rgb)

static void
cmyk_8_to_cmyk(const stp_vars_t vars,
	       const unsigned char *cmykin,
	       unsigned short *cmykout,
	       int *zero_mask,
	       int width,
	       int bpp)
{
  int i;
  int j;
  int nz[4];
  static unsigned short	lut[256];
  static double print_gamma = -1.0;

  memset(nz, 0, sizeof(nz));
  if (print_gamma != stp_get_float_parameter(vars, "Gamma"))
  {
    print_gamma = stp_get_float_parameter(vars, "Gamma");

    for (i = 0; i < 256; i ++)
      lut[i] = 65535.0 * pow((double)i / 255.0, print_gamma) + 0.5;
  }

  for (i = 0; i < width; i++)
    {
      j = *cmykin++;
      nz[0] |= j;
      *cmykout++ = lut[j];

      j = *cmykin++;
      nz[1] |= j;
      *cmykout++ = lut[j];

      j = *cmykin++;
      nz[2] |= j;
      *cmykout++ = lut[j];

      j = *cmykin++;
      nz[3] |= j;
      *cmykout++ = lut[j];
    }
  if (zero_mask)
    {
      *zero_mask = nz[0] ? 0 : 1;
      *zero_mask |= nz[1] ? 0 : 2;
      *zero_mask |= nz[2] ? 0 : 4;
      *zero_mask |= nz[3] ? 0 : 8;
    }
}

static void
cmyk_8_to_gray(const stp_vars_t vars,
	       const unsigned char *cmykin,
	       unsigned short *grayout,
	       int *zero_mask,
	       int width,
	       int bpp)
{
  int i;
  int j;
  int nz[4];
  static unsigned short	lut[256];
  static double print_gamma = -1.0;

  memset(nz, 0, sizeof(nz));
  if (print_gamma != stp_get_float_parameter(vars, "Gamma"))
  {
    print_gamma = stp_get_float_parameter(vars, "Gamma");

    for (i = 0; i < 256; i ++)
      lut[i] = 65535.0 * pow((double)i / 255.0, print_gamma) + 0.5;
  }

  for (i = 0; i < width; i++)
    {
      j = *cmykin++;
      nz[0] |= j;
      *grayout++ = lut[j];
    }
  if (zero_mask)
    {
      *zero_mask = nz[0] ? 0 : 1;
    }
}

static void
raw_to_raw(const stp_vars_t vars,
	   const unsigned char *rawin,
	   unsigned short *rawout,
	   int *zero_mask,
	   int width,
	   int bpp)
{
  int i;
  int j;
  int nz[32];
  int colors;
  const unsigned short *srawin = (const unsigned short *) rawin;
  colors = bpp / 2;

  memset(nz, 0, sizeof(nz));
  for (i = 0; i < width; i++)
    {
      for (j = 0; j < colors; j++)
	{
	  nz[j] |= srawin[j];
	  rawout[j] = srawin[j];
	}
      srawin += colors;
      rawout += colors;
    }
  if (zero_mask)
    {
      *zero_mask = 0;
      for (i = 0; i < colors; i++)
	if (! nz[i])
	  *zero_mask |= 1 << i;
    }
}

static void
cmyk_to_cmyk(const stp_vars_t vars,
	     const unsigned char *cmykin,
	     unsigned short *cmykout,
	     int *zero_mask,
	     int width,
	     int bpp)
{
  int i;
  int j;
  int nz[4];
  const unsigned short *scmykin = (const unsigned short *) cmykin;

  memset(nz, 0, sizeof(nz));
  for (i = 0; i < width; i++)
    {
      for (j = 0; j < 4; j++)
	{
	  nz[j] |= scmykin[j];
	  cmykout[j] = scmykin[j];
	}
      scmykin += 4;
      cmykout += 4;
    }
  if (zero_mask)
    {
      *zero_mask = nz[0] ? 0 : 1;
      *zero_mask |= nz[1] ? 0 : 2;
      *zero_mask |= nz[2] ? 0 : 4;
      *zero_mask |= nz[3] ? 0 : 8;
    }
}

static void
cmyk_to_gray(const stp_vars_t vars,
	     const unsigned char *cmykin,
	     unsigned short *grayout,
	     int *zero_mask,
	     int width,
	     int bpp)
{
  int i;
  int nz[4];
  const unsigned short *scmykin = (const unsigned short *) cmykin;

  memset(nz, 0, sizeof(nz));
  for (i = 0; i < width; i++)
    {
      nz[0] |= scmykin[0];
      grayout[0] = scmykin[0];
      scmykin += 4;
      grayout += 1;
    }
  if (zero_mask)
    {
      *zero_mask = nz[0] ? 0 : 1;
    }
}

int
stpi_color_get_row(const stp_vars_t v, stp_image_t *image, int row,
		  unsigned short *out, int *zero_mask)
{
  const lut_t *lut = (const lut_t *)(stpi_get_component_data(v, "Color"));
  unsigned char *in = lut->in_data;
  if (stpi_image_get_row(image, in, lut->image_width * lut->image_bpp, row) !=
      STP_IMAGE_STATUS_OK)
    return 2;
  (lut->colorfunc)(v, in, out, zero_mask, lut->image_width, lut->image_bpp);
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
  dest->cmy_tmp = NULL;		/* Don't copy working storage */
  if (src->in_data)
    dest->in_data = stpi_malloc(src->image_width * src->image_bpp);
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
  if (lut->in_data)
    stpi_free(lut->in_data);
  if (lut->cmy_tmp)
    stpi_free(lut->cmy_tmp);
  memset(lut, 0, sizeof(lut_t));
  stpi_free(lut);
}

static stp_curve_t
compute_a_curve(stp_curve_t curve, size_t steps, double c_gamma,
		double print_gamma, double contrast, double app_gamma,
		double brightness, double screen_gamma, int input_color_model,
		int output_color_model, int linear_contrast_adjustment)
{
  double *tmp = stpi_malloc(sizeof(double) * steps);
  double pivot = .25;
  double ipivot = 1.0 - pivot;
  double xcontrast = pow(contrast, contrast);
  double xgamma = pow(pivot, screen_gamma);
  int i;
  int isteps = steps;
  if (steps > 256)
    isteps = 256;
  for (i = 0; i < isteps; i ++)
    {
      double temp_pixel, pixel;
      pixel = (double) i / (double) (isteps - 1);

      if (input_color_model == COLOR_MODEL_CMY)
	pixel = 1.0 - pixel;

      /*
       * First, correct contrast
       */
      if (pixel >= .5)
	temp_pixel = 1.0 - pixel;
      else
	temp_pixel = pixel;
      if (contrast > 3.99999)
	{
	  if (temp_pixel < .5)
	    temp_pixel = 0;
	  else
	    temp_pixel = 1;
	}
      if (temp_pixel <= .000001 && contrast <= .0001)
	temp_pixel = .5;
      else if (temp_pixel > 1)
	temp_pixel = .5 * pow(2 * temp_pixel, xcontrast);
      else if (temp_pixel < 1)
	{
	  if (linear_contrast_adjustment)
	    temp_pixel = 0.5 -
	      ((0.5 - .5 * pow(2 * temp_pixel, contrast)) * contrast);
	  else
	    temp_pixel = 0.5 -
	      ((0.5 - .5 * pow(2 * temp_pixel, contrast)));
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
      if (brightness < 1)
	pixel = pixel * brightness;
      else
	pixel = 1 - ((1 - pixel) * (2 - brightness));

      /*
       * Third, correct for the screen gamma
       */

      pixel = 1.0 -
	(1.0 / (1.0 - xgamma)) *
	(pow(pivot + ipivot * pixel, screen_gamma) - xgamma);

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

      pixel = 65535 * pow(pixel, print_gamma) + .5;
      if (output_color_model == COLOR_MODEL_RGB)
	pixel = 65535 - pixel;

      if (pixel <= 0.0)
	tmp[i] = 0;
      else if (pixel >= 65535.0)
	tmp[i] = 65535;
      else
	tmp[i] = (pixel);
    }
  stp_curve_set_data(curve, isteps, tmp);
  if (isteps != steps)
    stp_curve_resample(curve, steps);
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
  double gamma = stp_curve_get_gamma(curve);
  double *tmp_data;

  stp_curve_get_bounds(curve, &lo, &hi);

  if (gamma)
    stp_curve_set_gamma(curve, -gamma);
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
stpi_compute_lut(stp_vars_t v, size_t steps)
{
  stp_curve_t hue = NULL;
  stp_curve_t lum = NULL;
  stp_curve_t sat = NULL;
  stp_curve_t composite_curve = NULL;
  stp_curve_t cyan_curve = NULL;
  stp_curve_t magenta_curve = NULL;
  stp_curve_t yellow_curve = NULL;
  stp_curve_t black_curve = NULL;
  /*
   * Got an output file/command, now compute a brightness lookup table...
   */

  int linear_contrast_adjustment =
    stp_get_boolean_parameter(v, "LinearContrast");
  double cyan = stp_get_float_parameter(v, "Cyan");
  double magenta = stp_get_float_parameter(v, "Magenta");
  double yellow = stp_get_float_parameter(v, "Yellow");
  double print_gamma = stp_get_float_parameter(v, "Gamma");
  double contrast = stp_get_float_parameter(v, "Contrast");
  double brightness = stp_get_float_parameter(v, "Brightness");
  double app_gamma = 1.0;
  double screen_gamma;
  lut_t *lut;
  int input_color_model = stp_get_input_color_model(v);
  int output_color_model = stpi_get_output_color_model(v);

  if (stp_check_float_parameter(v, "AppGamma", STP_PARAMETER_ACTIVE))
    app_gamma = stp_get_float_parameter(v, "AppGamma");
  screen_gamma = app_gamma / 4.0; /* "Empirical" */
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

  stpi_allocate_component_data(v, "Color", copy_lut, free_lut, lut);
  stpi_dprintf(STPI_DBG_LUT, v, "stpi_compute_lut\n");
  stpi_dprintf(STPI_DBG_LUT, v, " cyan %.3f\n", cyan);
  stpi_dprintf(STPI_DBG_LUT, v, " magenta %.3f\n", magenta);
  stpi_dprintf(STPI_DBG_LUT, v, " yellow %.3f\n", yellow);
  stpi_dprintf(STPI_DBG_LUT, v, " print_gamma %.3f\n", print_gamma);
  stpi_dprintf(STPI_DBG_LUT, v, " contrast %.3f\n", contrast);
  stpi_dprintf(STPI_DBG_LUT, v, " brightness %.3f\n", brightness);
  stpi_dprintf(STPI_DBG_LUT, v, " screen_gamma %.3f\n", screen_gamma);
  if (composite_curve)
    {
      stp_curve_copy(lut->composite, composite_curve);
      invert_curve(lut->composite, input_color_model, output_color_model);
      stp_curve_rescale(lut->composite, 65535.0, STP_CURVE_COMPOSE_MULTIPLY,
			STP_CURVE_BOUNDS_RESCALE);
      stp_curve_resample(lut->composite, steps);
    }
  else
    {
      compute_a_curve(lut->composite, steps, 1.0, print_gamma, contrast,
		      app_gamma, brightness, screen_gamma,
		      input_color_model, output_color_model,
		      linear_contrast_adjustment);
    }
  if (black_curve)
    stp_curve_copy(lut->black, black_curve);
  else
    stp_curve_copy(lut->black, color_curve_bounds);
  stp_curve_rescale(lut->black, 65535.0, STP_CURVE_COMPOSE_MULTIPLY,
		    STP_CURVE_BOUNDS_RESCALE);
  stp_curve_resample(lut->black, steps);
  if (cyan_curve)
    {
      stp_curve_copy(lut->cyan, cyan_curve);
      invert_curve(lut->cyan, input_color_model, output_color_model);
      stp_curve_rescale(lut->cyan, 65535.0, STP_CURVE_COMPOSE_MULTIPLY,
			STP_CURVE_BOUNDS_RESCALE);
      stp_curve_resample(lut->cyan, steps);
    }
  else
    compute_a_curve(lut->cyan, steps, cyan, print_gamma, contrast,
		    app_gamma, brightness, screen_gamma,
		    input_color_model, output_color_model,
		    linear_contrast_adjustment);
  if (magenta_curve)
    {
      stp_curve_copy(lut->magenta, magenta_curve);
      invert_curve(lut->magenta, input_color_model, output_color_model);
      stp_curve_rescale(lut->magenta, 65535.0, STP_CURVE_COMPOSE_MULTIPLY,
			STP_CURVE_BOUNDS_RESCALE);
      stp_curve_resample(lut->magenta, steps);
    }
  else
    compute_a_curve(lut->magenta, steps, magenta, print_gamma, contrast,
		    app_gamma, brightness, screen_gamma,
		    input_color_model, output_color_model,
		    linear_contrast_adjustment);
  if (yellow_curve)
    {
      stp_curve_copy(lut->yellow, yellow_curve);
      invert_curve(lut->yellow, input_color_model, output_color_model);
      stp_curve_rescale(lut->yellow, 65535.0, STP_CURVE_COMPOSE_MULTIPLY,
			STP_CURVE_BOUNDS_RESCALE);
      stp_curve_resample(lut->yellow, steps);
    }
  else
    compute_a_curve(lut->yellow, steps, yellow, print_gamma, contrast,
		    app_gamma, brightness, screen_gamma,
		    input_color_model, output_color_model,
		    linear_contrast_adjustment);
}

static void
set_null_colorfunc(void)
{
  stpi_erprintf("No colorfunc chosen!\n");
}

#define SET_COLORFUNC(x)						    \
stpi_dprintf(STPI_DBG_COLORFUNC, v,					    \
	    "at line %d stp_choose_colorfunc(type %d bpp %d) ==> %s, %d\n", \
	    __LINE__, stp_get_output_type(v), image_bpp, #x, out_channels); \
lut->colorfunc = x;							    \
break

int
stpi_color_init(stp_vars_t v, stp_image_t *image, size_t steps)
{
  const char *image_type = stp_get_string_parameter(v, "ImageOptimization");
  int itype = 0;
  int out_channels = 0;
  int image_bpp = stpi_image_bpp(image);
  lut_t *lut;
  if (steps != 256 && steps != 65536)
    return -1;

  stpi_compute_lut(v, steps);
  lut = (lut_t *)(stpi_get_component_data(v, "Color"));
  lut->image_bpp = image_bpp;
  lut->image_width = stpi_image_width(image);
  if (image_type)
    {
      if (strcmp(image_type, "LineArt") == 0)
	itype = 0;
      else if (strcmp(image_type, "Solid") == 0)
	itype = 1;
      else if (strcmp(image_type, "Photograph") == 0)
	itype = 2;
      else if (strcmp(image_type, "HSLAdjust") == 0)
	itype = 3;
    }
  switch (stp_get_output_type(v))
    {
    case OUTPUT_RAW_CMYK:
      out_channels = 4;
      switch (image_bpp)
	{
	case 1:
	  switch (itype)
	    {
	    case 3:
	    case 2:
	    case 1:
	      SET_COLORFUNC(gray_to_cmyk);
	    case 0:
	      SET_COLORFUNC(fast_gray_to_cmyk);
	    default:
	      set_null_colorfunc();
	      SET_COLORFUNC(NULL);
	      break;
	    }
	  break;
	case 3:
	  switch (itype)
	    {
	    case 3:
	    case 2:
	    case 1:
	      SET_COLORFUNC(rgb_to_cmyk);
	    case 0:
	      SET_COLORFUNC(fast_rgb_to_cmyk);
	    default:
	      set_null_colorfunc();
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 4:
	  SET_COLORFUNC(cmyk_8_to_cmyk);
	case 8:
	  SET_COLORFUNC(cmyk_to_cmyk);
	default:
	  set_null_colorfunc();
	  SET_COLORFUNC(NULL);
	}
      break;
    case OUTPUT_COLOR:
      out_channels = 3;
      switch (image_bpp)
	{
	case 3:
	  switch (itype)
	    {
	    case 3:
	    case 2:
	    case 1:
	      SET_COLORFUNC(rgb_to_rgb);
	    case 0:
	      SET_COLORFUNC(fast_rgb_to_rgb);
	    default:
	      set_null_colorfunc();
	      SET_COLORFUNC(NULL);
	    }
	  break;
	case 1:
	  switch (itype)
	    {
	    case 3:
	    case 2:
	    case 1:
	      SET_COLORFUNC(gray_to_rgb);
	    case 0:
	      SET_COLORFUNC(fast_gray_to_rgb);
	    default:
	      set_null_colorfunc();
	      SET_COLORFUNC(NULL);
	      break;
	    }
	  break;
	default:
	  set_null_colorfunc();
	  SET_COLORFUNC(NULL);
	}
      break;
    case OUTPUT_RAW_PRINTER:
      if ((image_bpp & 1) || image_bpp < 2 || image_bpp > 64)
	{
	  set_null_colorfunc();
	  SET_COLORFUNC(NULL);
	}
      out_channels = image_bpp / 2;
      SET_COLORFUNC(raw_to_raw);
    case OUTPUT_GRAY:
      out_channels = 1;
      switch (image_bpp)
	{
	case 1:
	  SET_COLORFUNC(gray_to_gray);
	case 3:
	  SET_COLORFUNC(rgb_to_gray);
	case 4:
	  SET_COLORFUNC(cmyk_8_to_gray);
	case 8:
	  SET_COLORFUNC(cmyk_to_gray);
	default:
	  set_null_colorfunc();
	  SET_COLORFUNC(NULL);
	}
      break;
    default:
      set_null_colorfunc();
      SET_COLORFUNC(NULL);
    }
  lut->in_data = stpi_malloc(stpi_image_width(image) * image_bpp);
  return out_channels;
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

stp_parameter_list_t
stpi_color_list_parameters(const stp_vars_t v)
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

void
stpi_color_describe_parameter(const stp_vars_t v, const char *name,
			      stp_parameter_t *description)
{
  int i;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  initialize_standard_curves();
  if (name == NULL)
    return;
  for (i = 0; i < float_parameter_count; i++)
    {
      float_param_t *param = &(float_parameters[i]);
      if (strcmp(name, param->param.name) == 0)
	{
	  stpi_fill_parameter_settings(description, &(param->param));
	  if (param->color_only && stp_get_output_type(v) == OUTPUT_GRAY)
	    description->is_active = 0;
	  else
	    description->is_active = 1;
	  switch (param->param.p_type)
	    {
	    case STP_PARAMETER_TYPE_BOOLEAN:
	      description->deflt.boolean = (int) param->defval;
	    case STP_PARAMETER_TYPE_DOUBLE:
	      description->bounds.dbl.upper = param->max;
	      description->bounds.dbl.lower = param->min;
	      description->deflt.dbl = param->defval;
	      break;
	    case STP_PARAMETER_TYPE_STRING_LIST:
	      if (!strcmp(param->param.name, "ImageOptimization"))
		{
		  description->bounds.str = stp_string_list_create();
		  stp_string_list_add_string
		    (description->bounds.str, "LineArt", _("Line Art"));
		  stp_string_list_add_string
		    (description->bounds.str, "HSLAdjust", _("HSL-corrected"));
		  stp_string_list_add_string
		    (description->bounds.str, "Solid", _("Solid Colors"));
		  stp_string_list_add_string
		    (description->bounds.str, "Photograph", _("Photographs"));
		  description->deflt.str = "LineArt";
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
	  if (param->hsl_only &&
	      stp_check_string_parameter(v, "ImageOptimization",
					 STP_PARAMETER_DEFAULTED) &&
	      strcmp(stp_get_string_parameter(v, "ImageOptimization"),
		     "LineArt") == 0)
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
