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
  int density_is_adjusted;
  unsigned char *in_data;
  int image_bpp;
  int image_width;
  stp_convert_t colorfunc;
  stp_curve_t composite;
  stp_curve_t cyan;
  stp_curve_t magenta;
  stp_curve_t yellow;
  stp_curve_t hue_map;
  stp_curve_t lum_map;
  stp_curve_t sat_map;
} lut_t;

typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
} float_param_t;

static float_param_t float_parameters[] =
{
  {
    {
      "Brightness", N_("Brightness"),
      N_("Brightness of the print (0 is solid black, 2 is solid white)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1
    }, 0.0, 2.0, 1.0
  },
  {
    {
      "Contrast", N_("Contrast"),
      N_("Contrast of the print (0 is solid gray)"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1
    }, 0.0, 4.0, 1.0
  },
  {
    {
      "Density", N_("Density"),
      N_("Adjust the density (amount of ink) of the print. "
	 "Reduce the density if the ink bleeds through the "
	 "paper or smears; increase the density if black "
	 "regions are not solid."),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1
    }, 0.1, 2.0, 1.0
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
      STP_PARAMETER_LEVEL_BASIC, 1
    }, 0.1, 4.0, 1.0
  },
  {
    {
      "AppGamma", N_("AppGamma"),
      N_("Gamma value assumed by application"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 1
    }, 0.1, 4.0, 1.0
  },
  {
    {
      "Cyan", N_("Cyan"),
      N_("Adjust the cyan balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1
    }, 0.0, 4.0, 1.0
  },
  {
    {
      "Magenta", N_("Magenta"),
      N_("Adjust the magenta balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1
    }, 0.0, 4.0, 1.0
  },
  {
    {
      "Yellow", N_("Yellow"),
      N_("Adjust the yellow balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1
    }, 0.0, 4.0, 1.0
  },
  {
    {
      "Saturation", N_("Saturation"),
      N_("Adjust the saturation (color balance) of the print\n"
	 "Use zero saturation to produce grayscale output "
	 "using color and black inks"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 1
    }, 0.0, 9.0, 1.0
  },
  {
    {
      "ImageOptimization", N_("Image Type"),
      N_("Optimize the settings for the type of image to be printed"),
      STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_BASIC, 0
    },
  }
};

static const int float_parameter_count =
sizeof(float_parameters) / sizeof(float_param_t);

typedef struct
{
  const stp_parameter_t param;
  stp_curve_t *defval;
} curve_param_t;

static int standard_curves_initialized = 0;

static stp_curve_t hue_map_bounds = NULL;
static stp_curve_t lum_map_bounds = NULL;
static stp_curve_t sat_map_bounds = NULL;
static stp_curve_t color_curve_bounds = NULL;

static curve_param_t curve_parameters[] =
{
  {
    {
      "CompositeCurve", N_("Composite Curve"),
      N_("Composite (Grayscale) curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0
    }, &color_curve_bounds
  },
  {
    {
      "CyanCurve", N_("Cyan Curve"),
      N_("Cyan curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0
    }, &color_curve_bounds
  },
  {
    {
      "MagentaCurve", N_("Magenta Curve"),
      N_("Magenta curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0
    }, &color_curve_bounds
  },
  {
    {
      "YellowCurve", N_("Yellow Curve"),
      N_("Yellow curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0
    }, &color_curve_bounds
  },
  {
    {
      "HueMap", N_("Hue Map"),
      N_("Hue adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 0
    }, &hue_map_bounds
  },
  {
    {
      "SatMap", N_("Saturation Map"),
      N_("Saturation adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 0
    }, &sat_map_bounds
  },
  {
    {
      "LumMap", N_("Luminosity Map"),
      N_("Luminosity adjustment curve"),
      STP_PARAMETER_TYPE_CURVE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED1, 0
    }, &lum_map_bounds
  },
};

static const int curve_parameter_count =
sizeof(curve_parameters) / sizeof(float_param_t);

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

static void
adjust_density(const stp_vars_t vars, lut_t *lut)
{
  if (! lut->density_is_adjusted)
    {
      stp_curve_rescale(lut->composite,
			stp_get_float_parameter(vars, "Density"),
			STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_CLIP);
      stp_curve_rescale(lut->cyan,
			stp_get_float_parameter(vars, "Density"),
			STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_CLIP);
      stp_curve_rescale(lut->magenta,
			stp_get_float_parameter(vars, "Density"),
			STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_CLIP);
      stp_curve_rescale(lut->yellow,
			stp_get_float_parameter(vars, "Density"),
			STP_CURVE_COMPOSE_MULTIPLY, STP_CURVE_BOUNDS_CLIP);
      lut->density_is_adjusted = 1;
    }
}


static void
gray_to_monochrome(const stp_vars_t vars,
		   const unsigned char *grayin,
		   unsigned short *grayout,
		   int *zero_mask,
		   int width,
		   int bpp)
{
  int i0 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stp_get_color_data(vars));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);

  while (width > 0)
    {
      if (i0 != grayin[0])
	{
	  i0 = grayin[0];
	  o0 = composite[grayin[0]];
	  if (o0 < 32768)
	    o0 = 0;
	  else
	    o0  = 65535;
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

static void
rgb_to_monochrome(const stp_vars_t vars,
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
  lut_t *lut = (lut_t *)(stp_get_color_data(vars));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);

  while (width > 0)
    {
      if (i0 != rgb[0] || i1 != rgb[1] || i2 != rgb[2])
	{
	  i0 = rgb[0];
	  i1 = rgb[1];
	  i2 = rgb[2];
	  o0 = composite[(i0 * LUM_RED + i1 * LUM_GREEN +
			       i2 * LUM_BLUE) / 100];
	  if (o0 < 32768)
	    o0 = 0;
	  else
	    o0 = 65535;
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
  lut_t *lut = (lut_t *)(stp_get_color_data(vars));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  adjust_density(vars, lut);
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
  lut_t *lut = (lut_t *)(stp_get_color_data(vars));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  adjust_density(vars, lut);
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
  unsigned ld = stp_get_float_parameter(vars, "Density") * 65536;
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
  lut_t *lut = (lut_t *)(stp_get_color_data(vars));
  int compute_saturation = ssat <= .99999 || ssat >= 1.00001;
  int split_saturation = ssat > 1.4;
  const unsigned short *red = stp_curve_get_ushort_data(lut->cyan, &count);
  const unsigned short *green = stp_curve_get_ushort_data(lut->magenta, &count);
  const unsigned short *blue = stp_curve_get_ushort_data(lut->yellow, &count);
  size_t h_points = 0;
  size_t l_points = 0;
  size_t s_points = 0;
  double tmp;
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
	  double h, s, l;
	  i0 = rgbin[0];
	  i1 = rgbin[1];
	  i2 = rgbin[2];
	  rgbout[0] = i0 | (i0 << 8);
	  rgbout[1] = i1 | (i1 << 8);
	  rgbout[2] = i2 | (i2 << 8);
	  if ((compute_saturation) &&
	      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
	    {
	      calc_rgb_to_hsl(rgbout, &h, &s, &l);
	      if (ssat < 1)
		s *= ssat;
	      else
		{
		  double s1 = s * ssat;
		  double s2 = 1.0 - ((1.0 - s) * isat);
		  s = FMIN(s1, s2);
		}
	      if (s > 1)
		s = 1.0;
	      calc_hsl_to_rgb(rgbout, h, s, l);
	    }
	  update_cmyk(rgbout);	/* Fiddle with the INPUT */
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

	  if ((split_saturation || lut->hue_map || lut->lum_map ||
	       lut->sat_map) &&
	      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
	    {
	      calc_rgb_to_hsl(rgbout, &h, &s, &l);
	      if (split_saturation)
		{
		  if (ssat < 1)
		    s *= ssat;
		  else
		    {
		      double s1 = s * ssat;
		      double s2 = 1.0 - ((1.0 - s) * isat);
		      s = FMIN(s1, s2);
		    }
		}
	      if (s > 1)
		s = 1.0;
	      if (lut->hue_map || lut->lum_map || lut->sat_map)
		{
		  if (lut->hue_map)
		    {
		      double nh = h * h_points / 6.0;
		      if (stp_curve_interpolate_value(lut->hue_map, nh, &tmp))
			{
			  h += tmp;
			  if (h < 0.0)
			    h += 6.0;
			  else if (h >= 6.0)
			    h -= 6.0;
			}
		    }
		  if (lut->lum_map && l > .0001 && l < .9999)
		    {
		      double nh = h * l_points / 6.0;
		      if (stp_curve_interpolate_value(lut->lum_map, nh, &tmp)&&
			  (tmp < .9999 || tmp > 1.0001))
			{
			  double el = tmp;
			  el = 1.0 + (s * (el - 1.0));
			  if (l > .5)
			    el = 1.0 + ((2.0 * (1.0 - l)) * (el - 1.0));
			  l = 1.0 - pow(1.0 - l, el);
			}
		    }
		  if (lut->sat_map)
		    {
		      double nh = h * s_points / 6.0;
		      if (stp_curve_interpolate_value(lut->sat_map, nh, &tmp)&&
			  (tmp < .9999 || tmp > 1.0001))
			{
			  s = 1.0 - pow(1.0 - s, tmp);
			}
		    }
		}
	      calc_hsl_to_rgb(rgbout, h, s, l);
	    }
	  if (ld < 65536)
	    {
	      int i;
	      for (i = 0; i < 3; i++)
		{
		  unsigned t = rgbout[i];
		  t = t * ld / 65536;
		  rgbout[i] = (unsigned short) t;
		}
	    }
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

static void
solid_rgb_to_rgb(const stp_vars_t vars,
		 const unsigned char *rgbin,
		 unsigned short *rgbout,
		 int *zero_mask,
		 int width,
		 int bpp)
{
  unsigned ld = stp_get_float_parameter(vars, "Density") * 65536;
  double isat = 1.0;
  double ssat = stp_get_float_parameter(vars, "Saturation");
  int compute_saturation = ssat <= .99999 || ssat >= 1.00001;
  int split_saturation = ssat > 1.4;
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stp_get_color_data(vars));
  size_t count;
  const unsigned short *red = stp_curve_get_ushort_data(lut->cyan, &count);
  const unsigned short *green = stp_curve_get_ushort_data(lut->magenta, &count);
  const unsigned short *blue = stp_curve_get_ushort_data(lut->yellow, &count);
  size_t h_points = 0;
  size_t l_points = 0;
  size_t s_points = 0;
  double tmp;
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
	  double h, s, l;
	  i0 = rgbin[0];
	  i1 = rgbin[1];
	  i2 = rgbin[2];
	  rgbout[0] = i0 | (i0 << 8);
	  rgbout[1] = i1 | (i1 << 8);
	  rgbout[2] = i2 | (i2 << 8);
	  if ((compute_saturation) &&
	      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
	    {
	      calc_rgb_to_hsl(rgbout, &h, &s, &l);
	      if (ssat < 1)
		s *= ssat;
	      else
		{
		  double s1 = s * ssat;
		  double s2 = 1.0 - ((1.0 - s) * isat);
		  s = FMIN(s1, s2);
		}
	      if (s > 1)
		s = 1.0;
	      calc_hsl_to_rgb(rgbout, h, s, l);
	    }
	  update_cmyk(rgbout);	/* Fiddle with the INPUT */
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
	  if ((split_saturation || lut->hue_map || lut->sat_map) &&
	      (rgbout[0] != rgbout[1] || rgbout[0] != rgbout[2]))
	    {
	      calc_rgb_to_hsl(rgbout, &h, &s, &l);
	      if (split_saturation)
		{
		  if (ssat < 1)
		    s *= ssat;
		  else
		    {
		      double s1 = s * ssat;
		      double s2 = 1.0 - ((1.0 - s) * isat);
		      s = FMIN(s1, s2);
		    }
		}
	      if (s > 1)
		s = 1.0;
	      if (lut->hue_map || lut->sat_map)
		{
		  if (lut->hue_map)
		    {
		      double nh = h * h_points / 6.0;
		      if (stp_curve_interpolate_value(lut->hue_map, nh, &tmp))
			{
			  h += tmp;
			  if (h < 0.0)
			    h += 6.0;
			  else if (h >= 6.0)
			    h -= 6.0;
			}
		    }
		  if (lut->sat_map)
		    {
		      double nh = h * s_points / 6.0;
		      if (stp_curve_interpolate_value(lut->sat_map, nh, &tmp)&&
			  (tmp < .9999 || tmp > 1.0001))
			{
			  s = 1.0 - pow(1.0 - s, tmp);
			}
		    }
		}
	      calc_hsl_to_rgb(rgbout, h, s, l);
	    }
	  if (ld < 65536)
	    {
	      int i;
	      for (i = 0; i < 3; i++)
		{
		  unsigned t = rgbout[i];
		  t = t * ld / 65536;
		  rgbout[i] = (unsigned short) t;
		}
	    }
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
  lut_t *lut = (lut_t *)(stp_get_color_data(vars));
  size_t count;
  const unsigned short *red;
  const unsigned short *green;
  const unsigned short *blue;
  adjust_density(vars, lut);
  red = stp_curve_get_ushort_data(lut->cyan, &count);
  green = stp_curve_get_ushort_data(lut->magenta, &count);
  blue = stp_curve_get_ushort_data(lut->yellow, &count);

  while (width > 0)
    {
      unsigned short trgb[3];
      if (i0 == grayin[0])
	{
	  rgbout[0] = o0;
	  rgbout[1] = o1;
	  rgbout[2] = o2;
	}
      else
	{
	  i0 = grayin[0];
	  trgb[0] =
	    trgb[1] =
	    trgb[2] = i0 | (i0 << 8);
	  update_cmyk(trgb);
	  if (lut->steps == 65536)
	    {
	      rgbout[0] = red[trgb[0]];
	      rgbout[1] = green[trgb[1]];
	      rgbout[2] = blue[trgb[2]];
	    }
	  else
	    {
	      rgbout[0] = red[trgb[0] / 256];
	      rgbout[1] = green[trgb[1] / 256];
	      rgbout[2] = blue[trgb[2] / 256];
	    }
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
  unsigned ld = stp_get_float_parameter(vars, "Density") * 65536;
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stp_get_color_data(vars));
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
      double h, s, l;
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
	    {
	      calc_rgb_to_hsl(rgbout, &h, &s, &l);
	      if (saturation < 1)
		s *= saturation;
	      else if (saturation > 1)
		{
		  double s1 = s * saturation;
		  double s2 = 1.0 - ((1.0 - s) * isat);
		  s = FMIN(s1, s2);
		}
	      if (s > 1)
		s = 1.0;
	      calc_hsl_to_rgb(rgbout, h, s, l);
	    }
	  if (ld < 65536)
	    {
	      int i;
	      for (i = 0; i < 3; i++)
		rgbout[i] = rgbout[i] * ld / 65536;
	    }
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
  lut_t *lut = (lut_t *)(stp_get_color_data(vars));
  size_t count;
  const unsigned short *red;
  const unsigned short *green;
  const unsigned short *blue;

  stp_curve_resample(lut->cyan, 256);
  stp_curve_resample(lut->magenta, 256);
  stp_curve_resample(lut->yellow, 256);
  adjust_density(vars, lut);
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
  static double density = -1.0;
  static double print_gamma = -1.0;

  memset(nz, 0, sizeof(nz));
  if (density != stp_get_float_parameter(vars, "Density") ||
      print_gamma != stp_get_float_parameter(vars, "Gamma"))
  {
    density     = stp_get_float_parameter(vars, "Density");
    print_gamma = stp_get_float_parameter(vars, "Gamma");

    for (i = 0; i < 256; i ++)
      lut[i] = 65535.0 * density * pow((double)i / 255.0, print_gamma) + 0.5;
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
cmyk_8_to_monochrome(const stp_vars_t vars,
		     const unsigned char *cmykin,
		     unsigned short *grayout,
		     int *zero_mask,
		     int width,
		     int bpp)
{
  int i;
  int j;
  int nz[4];

  memset(nz, 0, sizeof(nz));
  for (i = 0; i < width; i++)
    {
      j = *cmykin++;
      if (j < 32768)
	j = 0;
      else
	j = 65535;
      nz[0] |= j;
      *grayout++ = j;
    }
  if (zero_mask)
    {
      *zero_mask = nz[0] ? 0 : 1;
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
  static double density = -1.0;
  static double print_gamma = -1.0;

  memset(nz, 0, sizeof(nz));
  if (density != stp_get_float_parameter(vars, "Density") ||
      print_gamma != stp_get_float_parameter(vars, "Gamma"))
  {
    density     = stp_get_float_parameter(vars, "Density");
    print_gamma = stp_get_float_parameter(vars, "Gamma");

    for (i = 0; i < 256; i ++)
      lut[i] = 65535.0 * density * pow((double)i / 255.0, print_gamma) + 0.5;
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

static void
cmyk_to_monochrome(const stp_vars_t vars,
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
      unsigned short out = scmykin[0];
      if (out < 32768)
	out = 0;
      else
	out = 65535;
      nz[0] |= out;
      grayout[0] = out;
      scmykin += 4;
      grayout += 1;
    }
  if (zero_mask)
    {
      *zero_mask = nz[0] ? 0 : 1;
    }
}

int
stp_color_get_row(const stp_vars_t v, stp_image_t *image, int row,
		  unsigned short *out, int *zero_mask)
{
  const lut_t *lut = (const lut_t *)(stp_get_color_data(v));
  unsigned char *in = lut->in_data;
  if (stp_image_get_row(image, in, lut->image_width * lut->image_bpp, row) !=
      STP_IMAGE_OK)
    return 2;
  (lut->colorfunc)(v, in, out, zero_mask, lut->image_width, lut->image_bpp);
  return 0;
}

static lut_t *
allocate_lut(void)
{
  lut_t *ret = stp_malloc(sizeof(lut_t));
  ret->composite = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  ret->cyan = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  ret->magenta = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  ret->yellow = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  stp_curve_set_bounds(ret->composite, 0, 65535);
  stp_curve_set_bounds(ret->cyan, 0, 65535);
  stp_curve_set_bounds(ret->magenta, 0, 65535);
  stp_curve_set_bounds(ret->yellow, 0, 65535);
  ret->hue_map = NULL;
  ret->lum_map = NULL;
  ret->sat_map = NULL;
  ret->steps = 0;
  ret->density_is_adjusted = 0;
  ret->image_bpp = 0;
  ret->image_width = 0;
  ret->in_data = NULL;
  ret->colorfunc = NULL;
  return ret;
}

static void *
copy_lut(const stp_vars_t v)
{
  const lut_t *src = (const lut_t *)(stp_get_color_data(v));
  lut_t *dest;
  if (!src)
    return NULL;
  dest = allocate_lut();
  dest->composite = stp_curve_allocate_copy(src->composite);
  dest->cyan = stp_curve_allocate_copy(src->cyan);
  dest->magenta = stp_curve_allocate_copy(src->magenta);
  dest->yellow = stp_curve_allocate_copy(src->yellow);
  if (src->hue_map)
    dest->hue_map = stp_curve_allocate_copy(src->hue_map);
  if (src->lum_map)
    dest->lum_map = stp_curve_allocate_copy(src->lum_map);
  if (src->sat_map)
    dest->sat_map = stp_curve_allocate_copy(src->sat_map);
  dest->density_is_adjusted = src->density_is_adjusted;
  dest->steps = src->steps;
  dest->colorfunc = src->colorfunc;
  dest->image_bpp = src->image_bpp;
  dest->image_width = src->image_width;
  if (src->in_data)
    dest->in_data = stp_malloc(src->image_width * src->image_bpp);
  else
    dest->in_data = NULL;
  return dest;
}

static void
stp_free_lut(stp_vars_t v)
{
  if (stp_get_color_data(v))
    {
      lut_t *lut = (lut_t *)(stp_get_color_data(v));
      if (lut->composite)
	stp_curve_destroy(lut->composite);
      if (lut->cyan)
	stp_curve_destroy(lut->cyan);
      if (lut->magenta)
	stp_curve_destroy(lut->magenta);
      if (lut->yellow)
	stp_curve_destroy(lut->yellow);
      if (lut->hue_map)
	stp_curve_destroy(lut->hue_map);
      if (lut->lum_map)
	stp_curve_destroy(lut->lum_map);
      if (lut->sat_map)
	stp_curve_destroy(lut->sat_map);
      if (lut->in_data)
	stp_free(lut->in_data);
      memset(lut, 0, sizeof(lut_t));
      stp_free(lut);
      stp_set_color_data(v, NULL);
    }
}

static stp_curve_t
compute_a_curve(stp_curve_t curve, size_t steps, double c_gamma,
		double print_gamma, double contrast, double app_gamma,
		double brightness, double screen_gamma, int input_color_model,
		int output_color_model)
{
  double *tmp = stp_malloc(sizeof(double) * steps);
  double pivot = .25;
  double ipivot = 1.0 - pivot;
  int i;
  for (i = 0; i < steps; i ++)
    {
      double temp_pixel, pixel;
      pixel = (double) i / (double) (steps - 1);

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
	temp_pixel = .5 * pow(2 * temp_pixel, pow(contrast, contrast));
      else if (temp_pixel < 1)
	temp_pixel = 0.5 -
	  ((0.5 - .5 * pow(2 * temp_pixel, contrast)) * contrast);
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
	(1.0 / (1.0 - pow(pivot, screen_gamma))) *
	(pow(pivot + ipivot * pixel, screen_gamma) - pow(pivot, screen_gamma));

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
  stp_curve_set_data(curve, steps, tmp);
  stp_free(tmp);
}

static void
stp_compute_lut(stp_vars_t v, size_t steps)
{
  stp_curve_t hue = stp_get_curve_parameter(v, "HueMap");
  stp_curve_t lum = stp_get_curve_parameter(v, "LumMap");
  stp_curve_t sat = stp_get_curve_parameter(v, "SatMap");
  stp_curve_t composite_curve = stp_get_curve_parameter(v, "CompositeCurve");
  stp_curve_t cyan_curve = stp_get_curve_parameter(v, "CyanCurve");
  stp_curve_t magenta_curve = stp_get_curve_parameter(v, "MagentaCurve");
  stp_curve_t yellow_curve = stp_get_curve_parameter(v, "YellowCurve");
  /*
   * Got an output file/command, now compute a brightness lookup table...
   */

  double cyan = stp_get_float_parameter(v, "Cyan");
  double magenta = stp_get_float_parameter(v, "Magenta");
  double yellow = stp_get_float_parameter(v, "Yellow");
  double print_gamma = stp_get_float_parameter(v, "Gamma");
  double contrast = stp_get_float_parameter(v, "Contrast");
  double app_gamma = stp_get_float_parameter(v, "AppGamma");
  double brightness = stp_get_float_parameter(v, "Brightness");
  double screen_gamma = app_gamma / 4.0; /* "Empirical" */
  lut_t *lut;

  /*
   * Monochrome mode simply thresholds the input
   * to decide whether to print at all.  The printer gamma
   * is intended to represent the analog response of the printer.
   * Using it shifts the threshold, which is not the intent
   * of how this works.
   */
  if (stp_get_output_type(v) == OUTPUT_MONOCHROME)
    print_gamma = 1.0;

  lut = allocate_lut();

  /*
   * TODO check that these are wraparound curves and all that
   */
  if (hue)
    lut->hue_map = stp_curve_allocate_copy(hue);
  if (lum)
    lut->lum_map = stp_curve_allocate_copy(lum);
  if (sat)
    lut->sat_map = stp_curve_allocate_copy(sat);

  lut->steps = steps;

  stp_set_color_data(v, lut);
  stp_set_copy_color_data_func(v, copy_lut);
  stp_set_destroy_color_data_func(v, stp_free_lut);
  stp_dprintf(STP_DBG_LUT, v, "stp_compute_lut\n");
  stp_dprintf(STP_DBG_LUT, v, " cyan %.3f\n", cyan);
  stp_dprintf(STP_DBG_LUT, v, " magenta %.3f\n", magenta);
  stp_dprintf(STP_DBG_LUT, v, " yellow %.3f\n", yellow);
  stp_dprintf(STP_DBG_LUT, v, " print_gamma %.3f\n", print_gamma);
  stp_dprintf(STP_DBG_LUT, v, " contrast %.3f\n", contrast);
  stp_dprintf(STP_DBG_LUT, v, " brightness %.3f\n", brightness);
  stp_dprintf(STP_DBG_LUT, v, " screen_gamma %.3f\n", screen_gamma);
  if (composite_curve)
    {
      stp_curve_copy(lut->composite, composite_curve);
      stp_curve_resample(lut->composite, steps);
    }
  else
    compute_a_curve(lut->composite, steps, 1.0, print_gamma, contrast,
		    app_gamma, brightness, screen_gamma,
		    stp_get_input_color_model(v),
		    stp_get_output_color_model(v));
  if (cyan_curve)
    {
      stp_curve_copy(lut->cyan, cyan_curve);
      stp_curve_resample(lut->cyan, steps);
    }
  else
    compute_a_curve(lut->cyan, steps, cyan, print_gamma, contrast,
		    app_gamma, brightness, screen_gamma,
		    stp_get_input_color_model(v),
		    stp_get_output_color_model(v));
  if (magenta_curve)
    {
      stp_curve_copy(lut->magenta, magenta_curve);
      stp_curve_resample(lut->magenta, steps);
    }
  else
    compute_a_curve(lut->magenta, steps, magenta, print_gamma, contrast,
		    app_gamma, brightness, screen_gamma,
		    stp_get_input_color_model(v),
		    stp_get_output_color_model(v));
  if (yellow_curve)
    {
      stp_curve_copy(lut->yellow, yellow_curve);
      stp_curve_resample(lut->yellow, steps);
    }
  else
    compute_a_curve(lut->yellow, steps, yellow, print_gamma, contrast,
		    app_gamma, brightness, screen_gamma,
		    stp_get_input_color_model(v),
		    stp_get_output_color_model(v));
}

static void
set_null_colorfunc(void)
{
  stp_erprintf("No colorfunc chosen!\n");
}

#define SET_COLORFUNC(x)						    \
stp_dprintf(STP_DBG_COLORFUNC, v,					    \
	    "at line %d stp_choose_colorfunc(type %d bpp %d) ==> %s, %d\n", \
	    __LINE__, stp_get_output_type(v), image_bpp, #x, out_channels); \
lut->colorfunc = x;							    \
break

int
stp_color_init(stp_vars_t v,
	       stp_image_t *image,
	       size_t steps)
{
  const char *image_type = stp_get_string_parameter(v, "ImageOptimization");
  int itype = 0;
  int out_channels = 0;
  int image_bpp = stp_image_bpp(image);
  lut_t *lut;
  stp_compute_lut(v, steps);
  lut = (lut_t *)(stp_get_color_data(v));
  lut->image_bpp = image_bpp;
  lut->image_width = stp_image_width(image);
  if (image_type)
    {
      if (strcmp(image_type, "LineArt") == 0)
	itype = 0;
      else if (strcmp(image_type, "Solid") == 0)
	itype = 1;
      else if (strcmp(image_type, "Photograph") == 0)
	itype = 2;
    }
  switch (stp_get_output_type(v))
    {
    case OUTPUT_MONOCHROME:
      out_channels = 1;
      switch (image_bpp)
	{
	case 1:
	  SET_COLORFUNC(gray_to_monochrome);
	case 3:
	  SET_COLORFUNC(rgb_to_monochrome);
	case 4:
	  SET_COLORFUNC(cmyk_8_to_monochrome);
	case 8:
	  SET_COLORFUNC(cmyk_to_monochrome);
	default:
	  set_null_colorfunc();
	  SET_COLORFUNC(NULL);
	}
      break;
    case OUTPUT_RAW_CMYK:
      out_channels = 4;
      switch (image_bpp)
	{
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
	    case 2:
	      SET_COLORFUNC(rgb_to_rgb);
	    case 1:
	      SET_COLORFUNC(solid_rgb_to_rgb);
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
  lut->in_data = stp_malloc(stp_image_width(image) * image_bpp);
  return out_channels;
}

stp_parameter_list_t
stp_color_list_parameters(const stp_vars_t v)
{
  stp_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  for (i = 0; i < curve_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(curve_parameters[i].param));
  return ret;
}

void
stp_color_describe_parameter(const stp_vars_t v, const char *name,
			      stp_parameter_t *description)
{
  int i;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;
  if (!standard_curves_initialized)
    {
      hue_map_bounds = stp_curve_allocate_read_string
	("STP_CURVE;Wrap ;Linear ;2;0.0;-6.0;6.0:0;0;");
      lum_map_bounds = stp_curve_allocate_read_string
	("STP_CURVE;Wrap ;Linear ;2;0.0;0.0;4.0:0;0;");
      sat_map_bounds = stp_curve_allocate_read_string
	("STP_CURVE;Wrap ;Linear ;2;0.0;0.0;4.0:0;0;");
      color_curve_bounds = stp_curve_allocate_read_string
	("STP_CURVE;Nowrap ;Linear ;2;1.0;0.0;65535.0:");
      standard_curves_initialized = 1;
    }
  for (i = 0; i < float_parameter_count; i++)
    {
      float_param_t *param = &(float_parameters[i]);
      if (strcmp(name, param->param.name) == 0)
	{
	  stp_fill_parameter_settings(description, &(param->param));
	  switch (param->param.p_type)
	    {
	    case STP_PARAMETER_TYPE_DOUBLE:
	      description->bounds.dbl.upper = param->max;
	      description->bounds.dbl.lower = param->min;
	      description->deflt.dbl = param->defval;
	      break;
	    case STP_PARAMETER_TYPE_STRING_LIST:
	      if (!strcmp(param->param.name, "ImageOptimization"))
		{
		  description->bounds.str = stp_string_list_allocate();
		  stp_string_list_add_param
		    (description->bounds.str, "LineArt", _("Line Art"));
		  stp_string_list_add_param
		    (description->bounds.str, "Solid", _("Solid Colors"));
		  stp_string_list_add_param
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
	  stp_fill_parameter_settings(description, &(param->param));
	  switch (param->param.p_type)
	    {
	    case STP_PARAMETER_TYPE_CURVE:
	      description->deflt.curve = *(param->defval);
	      description->bounds.curve = *(param->defval);
	      break;
	    default:
	      break;
	    }
	  return;
	}
    }
}
