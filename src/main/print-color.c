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

typedef struct
{
  unsigned steps;
  stp_curve_t composite;
  stp_curve_t red;
  stp_curve_t green;
  stp_curve_t blue;
  stp_curve_t hue_map;
  stp_curve_t lum_map;
  stp_curve_t sat_map;
  const unsigned char *cmap;
  unsigned char *gray_cmap;
  unsigned char *alpha_table;
} lut_t;

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
compute_indexed_alpha_table(lut_t *lut)
{
  if (! (lut->alpha_table))
    {
      unsigned val, alpha;
      lut->alpha_table = stp_malloc(65536 * sizeof(unsigned char));
      for (val = 0; val < 256; val++)
	for (alpha = 0; alpha < 256; alpha++)
	  lut->alpha_table[(val * 256) + alpha] =
	    lut->gray_cmap[val] * alpha / 255 + 255 - alpha;
    }
}


static void
compute_alpha_table(lut_t *lut)
{
  if (! (lut->alpha_table))
    {
      unsigned val, alpha;
      lut->alpha_table = stp_malloc(65536 * sizeof(unsigned char));
      for (val = 0; val < 256; val++)
	for (alpha = 0; alpha < 256; alpha++)
	  lut->alpha_table[(val * 256) + alpha] =
	    val * alpha / 255 + 255 - alpha;
    }
}

static inline unsigned char
alpha_lookup(lut_t *lut, int val, int alpha)
{
  return lut->alpha_table[(val * 256) + alpha];
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
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);

  if (width <= 0)
    return;
  while (width)
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

static void
gray_alpha_to_gray(const stp_vars_t vars,
		   const unsigned char *grayin,
		   unsigned short *grayout,
		   int *zero_mask,
		   int width,
		   int bpp)
{
  int i0 = -1;
  int i1 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *composite;

  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);
  compute_alpha_table(lut);

  if (width <= 0)
    return;
  while (width)
    {
      if (i0 != grayin[0] || i1 != grayin[1])
	{
	  i0 = grayin[0];
	  i1 = grayin[1];
	  o0 = composite[alpha_lookup(lut, i0, i1)];
	  nz |= o0;
	}
      grayout[0] = o0;
      grayin += 2;
      grayout ++;
      width --;
    }
  if (zero_mask)
    *zero_mask = nz ? 0 : 1;
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
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);

  if (width <= 0)
    return;
  while (width)
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
gray_alpha_to_monochrome(const stp_vars_t vars,
			 const unsigned char *grayin,
			 unsigned short *grayout,
			 int *zero_mask,
			 int width,
			 int bpp)
{
  int i0 = -1;
  int i1 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *composite;

  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);
  compute_alpha_table(lut);

  if (width <= 0)
    return;
  while (width)
    {
      if (i0 != grayin[0] || i1 != grayin[1])
	{
	  i0 = grayin[0];
	  i1 = grayin[1];
	  o0 = composite[alpha_lookup(lut, i0, i1)];
	  if (o0 < 32768)
	    o0 = 0;
	  else
	    o0  = 65535;
	  nz |= o0;
	}
      grayout[0] = o0;
      grayin += 2;
      grayout ++;
      width --;
    }
  if (zero_mask)
    *zero_mask = nz ? 0 : 1;
}

/*
 * 'indexed_to_gray()' - Convert indexed image data to grayscale.
 */

static void
indexed_to_gray(const stp_vars_t vars,
		const unsigned char *indexed,
		unsigned short *gray,
		int *zero_mask,
		int width,
		int bpp)
{
  int i0 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);

  if (width <= 0)
    return;
  while (width)
    {
      if (i0 != indexed[0])
	{
	  i0 = indexed[0];
	  o0 = composite[lut->gray_cmap[i0]];
	  nz |= o0;
	}
      gray[0] = o0;
      gray ++;
      indexed ++;
      width --;
    }
  if (zero_mask)
    *zero_mask = nz ? 0 : 1;
}

static void
indexed_alpha_to_gray(const stp_vars_t vars,
		      const unsigned char *indexed,
		      unsigned short *gray,
		      int *zero_mask,
		      int width,
		      int bpp)
{
  int i0 = -1;
  int i1 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *composite;

  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);
  compute_indexed_alpha_table(lut);

  if (width <= 0)
    return;

  while (width)
    {
      if (i0 != indexed[0] || i1 != indexed[1])
	{
	  i0 = indexed[0];
	  i1 = indexed[1];
	  o0 = composite[alpha_lookup(lut, i0, i1)];
	  nz |= o0;
	}
      gray[0] = o0;
      gray ++;
      indexed += 2;
      width --;
    }
  if (zero_mask)
    *zero_mask = nz ? 0 : 1;
}

static void
indexed_to_monochrome(const stp_vars_t vars,
		      const unsigned char *indexed,
		      unsigned short *gray,
		      int *zero_mask,
		      int width,
		      int bpp)
{
  int i0 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);

  if (width <= 0)
    return;
  while (width)
    {
      if (i0 != indexed[0])
	{
	  i0 = indexed[0];
	  o0 = composite[lut->gray_cmap[i0]];
	  if (o0 < 32768)
	    o0 = 0;
	  else
	    o0  = 65535;
	  nz |= o0;
	}
      gray[0] = o0;
      indexed ++;
      gray ++;
      width --;
    }
  if (zero_mask)
    *zero_mask = nz ? 0 : 1;
}

static void
indexed_alpha_to_monochrome(const stp_vars_t vars,
			    const unsigned char *indexed,
			    unsigned short *gray,
			    int *zero_mask,
			    int width,
			    int bpp)
{
  int i0 = -1;
  int i1 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *composite;

  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);
  compute_indexed_alpha_table(lut);

  if (width <= 0)
    return;
  while (width)
    {
      if (i0 != indexed[0] || i1 != indexed[1])
	{
	  i0 = indexed[0];
	  i1 = indexed[1];
	  o0 = composite[alpha_lookup(lut, i0, i1)];
	  if (o0 < 32768)
	    o0 = 0;
	  else
	    o0  = 65535;
	  nz |= o0;
	}
      gray[0] = o0;
      indexed += 2;
      gray ++;
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
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
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

static void
rgb_alpha_to_gray(const stp_vars_t vars,
		  const unsigned char *rgb,
		  unsigned short *gray,
		  int *zero_mask,
		  int width,
		  int bpp)
{
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int i3 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);

  if (width <= 0)
    return;
  while (width)
    {
      if (i0 != rgb[0] || i1 != rgb[1] || i2 != rgb[2] || i3 != rgb[3])
	{
	  i0 = rgb[0];
	  i1 = rgb[1];
	  i2 = rgb[2];
	  i3 = rgb[3];
	  o0= composite[((i0 * LUM_RED + i1 * LUM_GREEN + i2 * LUM_BLUE) *
			 i3 / 25500 + 255 - i3)];
	  nz |= o0;
	}
      gray[0] = o0;
      rgb += 4;
      gray ++;
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
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
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

static void
rgb_alpha_to_monochrome(const stp_vars_t vars,
			const unsigned char *rgb,
			unsigned short *gray,
			int *zero_mask,
			int width,
			int bpp)
{
  int i0 = -1;
  int i1 = -1;
  int i2 = -1;
  int i3 = -1;
  int o0 = 0;
  int nz = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *composite;
  stp_curve_resample(lut->composite, 256);
  composite = stp_curve_get_ushort_data(lut->composite, &count);

  if (width <= 0)
    return;
  while (width)
    {
      if (i0 != rgb[0] || i1 != rgb[1] || i2 != rgb[2] || i3 != rgb[3])
	{
	  i0 = rgb[0];
	  i1 = rgb[1];
	  i2 = rgb[2];
	  i3 = rgb[3];
	  o0= composite[((i0 * LUM_RED + i1 * LUM_GREEN + i2 * LUM_BLUE) *
			 i3 / 25500 + 255 - i3)];
	  if (o0 < 32768)
	    o0 = 0;
	  else
	    o0 = 65535;
	  nz |= o0;
	}
      gray[0] = o0;
      rgb += 4;
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
  int i3 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  int compute_saturation = ssat <= .99999 || ssat >= 1.00001;
  int split_saturation = ssat > 1.4;
  const unsigned short *red = stp_curve_get_ushort_data(lut->red, &count);
  const unsigned short *green = stp_curve_get_ushort_data(lut->green, &count);
  const unsigned short *blue = stp_curve_get_ushort_data(lut->blue, &count);
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
  if (bpp == 2 || bpp == 4)
    compute_alpha_table(lut);

  if (split_saturation)
    ssat = sqrt(ssat);
  if (ssat > 1)
    isat = 1.0 / ssat;
  while (width > 0)
    {
      double h, s, l;
      switch (bpp)
	{
	case 1:
	  /*
	   * No alpha in image, using colormap...
	   */
	  if (i0 == rgbin[0])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = rgbin[0] * 3;
	      i1 = lut->cmap[i0 + 1];
	      i2 = lut->cmap[i0 + 2];
	      i0 = lut->cmap[i0];
	      rgbout[0] = i0 | (i0 << 8);
	      rgbout[1] = i1 | (i1 << 8);
	      rgbout[2] = i2 | (i2 << 8);
	      i0 = rgbin[0];
	    }
	  break;
	case 2:
	  if (i0 == rgbin[0] && i1 == rgbin[1])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = rgbin[0] * 3;
	      i3 = rgbin[1];
	      i1 = alpha_lookup(lut, lut->cmap[i0 + 1], i3);
	      i2 = alpha_lookup(lut, lut->cmap[i0 + 2], i3);
	      i0 = alpha_lookup(lut, i0, i3);

	      rgbout[0] = i0 | (i0 << 8);
	      rgbout[1] = i1 | (i1 << 8);
	      rgbout[2] = i2 | (i2 << 8);
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	    }
	  break;
	case 3:
	  /*
	   * No alpha in image...
	   */
	  if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	      i2 = rgbin[2];
	      rgbout[0] = i0 | (i0 << 8);
	      rgbout[1] = i1 | (i1 << 8);
	      rgbout[2] = i2 | (i2 << 8);
	    }
	  break;
	case 4:
	  if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2] &&
	      i3 == rgbin[3])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i3 = rgbin[3];
	      i0 = alpha_lookup(lut, rgbin[0], i3);
	      i1 = alpha_lookup(lut, rgbin[1], i3);
	      i2 = alpha_lookup(lut, rgbin[2], i3);
	      rgbout[0] = i0 | (i0 << 8);
	      rgbout[1] = i1 | (i1 << 8);
	      rgbout[2] = i2 | (i2 << 8);
	    }
	  break;
	}
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
	
      if ((split_saturation || lut->hue_map || lut->lum_map || lut->sat_map) &&
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
	      if (lut->sat_map)
		{
		  double nh = h * s_points / 6.0;
		  if (stp_curve_interpolate_value(lut->sat_map, nh, &tmp) &&
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
    out:
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
indexed_to_rgb(const stp_vars_t vars,
	       const unsigned char *indexed,
	       unsigned short *rgb,
	       int *zero_mask,
	       int width,
	       int bpp)
{
  rgb_to_rgb(vars, indexed, rgb, zero_mask, width, bpp);
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
  int i3 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *red = stp_curve_get_ushort_data(lut->red, &count);
  const unsigned short *green = stp_curve_get_ushort_data(lut->green, &count);
  const unsigned short *blue = stp_curve_get_ushort_data(lut->blue, &count);
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
  if (bpp == 2 || bpp == 4)
    compute_alpha_table(lut);

  if (split_saturation)
    ssat = sqrt(ssat);
  if (ssat > 1)
    isat = 1.0 / ssat;
  while (width > 0)
    {
      double h, s, l;
      switch (bpp)
	{
	case 1:
	  /*
	   * No alpha in image, using colormap...
	   */
	  if (i0 == rgbin[0])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = rgbin[0] * 3;
	      i1 = lut->cmap[i0 + 1];
	      i2 = lut->cmap[i0 + 2];
	      i0 = lut->cmap[i0];
	      rgbout[0] = i0 | (i0 << 8);
	      rgbout[1] = i1 | (i1 << 8);
	      rgbout[2] = i2 | (i2 << 8);
	      i0 = rgbin[0];
	    }
	  break;
	case 2:
	  if (i0 == rgbin[0] && i1 == rgbin[1])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = rgbin[0] * 3;
	      i3 = rgbin[1];
	      i1 = alpha_lookup(lut, lut->cmap[i0 + 1], i3);
	      i2 = alpha_lookup(lut, lut->cmap[i0 + 2], i3);
	      i0 = alpha_lookup(lut, i0, i3);

	      rgbout[0] = i0 | (i0 << 8);
	      rgbout[1] = i1 | (i1 << 8);
	      rgbout[2] = i2 | (i2 << 8);
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	    }
	  break;
	case 3:
	  /*
	   * No alpha in image...
	   */
	  if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	      i2 = rgbin[2];
	      rgbout[0] = i0 | (i0 << 8);
	      rgbout[1] = i1 | (i1 << 8);
	      rgbout[2] = i2 | (i2 << 8);
	    }
	  break;
	case 4:
	  if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2] &&
	      i3 == rgbin[3])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i3 = rgbin[3];
	      i0 = alpha_lookup(lut, rgbin[0], i3);
	      i1 = alpha_lookup(lut, rgbin[1], i3);
	      i2 = alpha_lookup(lut, rgbin[2], i3);
	      rgbout[0] = i0 | (i0 << 8);
	      rgbout[1] = i1 | (i1 << 8);
	      rgbout[2] = i2 | (i2 << 8);
	    }
	  break;
	}
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
		  if (stp_curve_interpolate_value(lut->sat_map, nh, &tmp) &&
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
    out:
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
solid_indexed_to_rgb(const stp_vars_t vars,
		     const unsigned char *indexed,
		     unsigned short *rgb,
		     int *zero_mask,
		     int width,
		     int bpp)
{
  solid_rgb_to_rgb(vars, indexed, rgb, zero_mask, width, bpp);
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
  int i1 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *red = stp_curve_get_ushort_data(lut->red, &count);
  const unsigned short *green = stp_curve_get_ushort_data(lut->green, &count);
  const unsigned short *blue = stp_curve_get_ushort_data(lut->blue, &count);

  if (bpp == 2)
    compute_indexed_alpha_table(lut);

  while (width > 0)
    {
      unsigned short trgb[3];
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == grayin[0])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = grayin[0];
	      trgb[0] =
	      trgb[1] =
	      trgb[2] = i0 | (i0 << 8);
	    }
	}
      else
	{
	  if (i0 == grayin[0] && i1 == grayin[1])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = alpha_lookup(lut, grayin[0], grayin[1]);
	      trgb[0] =
	      trgb[1] =
	      trgb[2] = i0 | (i0 << 8);
	    }
	}
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
    out:
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
fast_indexed_to_rgb(const stp_vars_t vars,
		    const unsigned char *indexed,
		    unsigned short *rgb,
		    int *zero_mask,
		    int width,
		    int bpp)
{
  int i0 = -1;
  int i1 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *red;
  const unsigned short *green;
  const unsigned short *blue;
  double isat = 1.0;
  double saturation = stp_get_float_parameter(vars, "Saturation");
  double density = stp_get_float_parameter(vars, "Density");

  stp_curve_resample(lut->red, 256);
  stp_curve_resample(lut->green, 256);
  stp_curve_resample(lut->blue, 256);
  red = stp_curve_get_ushort_data(lut->red, &count);
  green = stp_curve_get_ushort_data(lut->green, &count);
  blue = stp_curve_get_ushort_data(lut->blue, &count);
  if (bpp == 2)
    compute_alpha_table(lut);

  if (saturation > 1)
    isat = 1.0 / saturation;
  while (width > 0)
    {
      double h, s, l;
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == indexed[0])
	    {
	      rgb[0] = o0;
	      rgb[1] = o1;
	      rgb[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = indexed[0];
	      rgb[0] = red[lut->cmap[i0 * 3 + 0]];
	      rgb[1] = green[lut->cmap[i0 * 3 + 1]];
	      rgb[2] = blue[lut->cmap[i0 * 3 + 2]];
	    }
	}
      else
	{
	  if (i0 == indexed[0] && i1 == indexed[1])
	    {
	      rgb[0] = o0;
	      rgb[1] = o1;
	      rgb[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = indexed[0];
	      i1 = indexed[1];
	      rgb[0] = alpha_lookup(lut, lut->cmap[i0 * 3 + 0], i1);
	      rgb[1] = alpha_lookup(lut, lut->cmap[i0 * 3 + 1], i1);
	      rgb[2] = alpha_lookup(lut, lut->cmap[i0 * 3 + 2], i1);
	    }
	}
      if (saturation != 1.0)
	{
	  calc_rgb_to_hsl(rgb, &h, &s, &l);
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
	  calc_hsl_to_rgb(rgb, h, s, l);
	}
      if (density != 1.0)
	{
	  int i;
	  for (i = 0; i < 3; i++)
	    rgb[i] = .5 + (rgb[i] * density);
	}
    out:
      o0 = rgb[0];
      o1 = rgb[1];
      o2 = rgb[2];
      nz0 |= o0;
      nz1 |= o1;
      nz2 |= o2;
      indexed += bpp;
      rgb += 3;
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
  int i3 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *red;
  const unsigned short *green;
  const unsigned short *blue;
  double isat = 1.0;
  double saturation = stp_get_float_parameter(vars, "Saturation");

  stp_curve_resample(lut->red, 256);
  stp_curve_resample(lut->green, 256);
  stp_curve_resample(lut->blue, 256);
  red = stp_curve_get_ushort_data(lut->red, &count);
  green = stp_curve_get_ushort_data(lut->green, &count);
  blue = stp_curve_get_ushort_data(lut->blue, &count);
  if (bpp == 4)
    compute_alpha_table(lut);

  if (saturation > 1)
    isat = 1.0 / saturation;
  while (width > 0)
    {
      double h, s, l;
      if (bpp == 3)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	      i2 = rgbin[2];
	      rgbout[0] = red[rgbin[0]];
	      rgbout[1] = green[rgbin[1]];
	      rgbout[2] = blue[rgbin[2]];
	    }
	}
      else
	{
	  if (i0 == rgbin[0] && i1 == rgbin[1] && i2 == rgbin[2] &&
	      i3 == rgbin[3])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = rgbin[0];
	      i1 = rgbin[1];
	      i2 = rgbin[2];
	      i3 = rgbin[3];
	      rgbout[0] = red[alpha_lookup(lut, i0, i3)];
	      rgbout[1] = green[alpha_lookup(lut, i1, i3)];
	      rgbout[2] = blue[alpha_lookup(lut, i2, i3)];
	    }
	}
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
    out:
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
  int i1 = -1;
  int o0 = 0;
  int o1 = 0;
  int o2 = 0;
  int nz0 = 0;
  int nz1 = 0;
  int nz2 = 0;
  lut_t *lut = (lut_t *)(stp_get_lut(vars));
  size_t count;
  const unsigned short *red;
  const unsigned short *green;
  const unsigned short *blue;
  double density = stp_get_float_parameter(vars, "Density");

  stp_curve_resample(lut->red, 256);
  stp_curve_resample(lut->green, 256);
  stp_curve_resample(lut->blue, 256);
  red = stp_curve_get_ushort_data(lut->red, &count);
  green = stp_curve_get_ushort_data(lut->green, &count);
  blue = stp_curve_get_ushort_data(lut->blue, &count);
  if (bpp == 2)
    compute_alpha_table(lut);

  while (width > 0)
    {
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */
	  if (i0 == grayin[0])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      i0 = grayin[0];
	      rgbout[0] = red[grayin[0]];
	      rgbout[1] = green[grayin[0]];
	      rgbout[2] = blue[grayin[0]];
	    }
	}
      else
	{
	  if (i0 == grayin[0] && i1 == grayin[1])
	    {
	      rgbout[0] = o0;
	      rgbout[1] = o1;
	      rgbout[2] = o2;
	      goto out;
	    }
	  else
	    {
	      int lookup = alpha_lookup(lut, grayin[0], grayin[1]);
	      i0 = grayin[0];
	      i1 = grayin[1];
	      rgbout[0] = red[lookup];
	      rgbout[1] = green[lookup];
	      rgbout[2] = blue[lookup];
	    }
	}
      if (density != 1.0)
	{
	  int i;
	  for (i = 0; i < 3; i++)
	    rgbout[i] = .5 + (rgbout[i] * density);
	}
      o0 = rgbout[0];
      o1 = rgbout[1];
      o2 = rgbout[2];
      nz0 |= o0;
      nz1 |= o1;
      nz2 |= o2;
    out:
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

static lut_t *
allocate_lut(void)
{
  lut_t *ret = stp_malloc(sizeof(lut_t));
  ret->composite = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  ret->red = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  ret->green = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  ret->blue = stp_curve_allocate(STP_CURVE_WRAP_NONE);
  stp_curve_set_bounds(ret->composite, 0, 65535);
  stp_curve_set_bounds(ret->red, 0, 65535);
  stp_curve_set_bounds(ret->green, 0, 65535);
  stp_curve_set_bounds(ret->blue, 0, 65535);
  ret->hue_map = NULL;
  ret->lum_map = NULL;
  ret->sat_map = NULL;
  ret->cmap = NULL;
  ret->gray_cmap = NULL;
  ret->alpha_table = NULL;
  return ret;
}

void
stp_free_lut(stp_vars_t v)
{
  if (stp_get_lut(v))
    {
      lut_t *lut = (lut_t *)(stp_get_lut(v));
      if (lut->composite)
	stp_curve_destroy(lut->composite);
      if (lut->red)
	stp_curve_destroy(lut->red);
      if (lut->green)
	stp_curve_destroy(lut->green);
      if (lut->blue)
	stp_curve_destroy(lut->blue);
      if (lut->hue_map)
	stp_curve_destroy(lut->hue_map);
      if (lut->lum_map)
	stp_curve_destroy(lut->lum_map);
      if (lut->sat_map)
	stp_curve_destroy(lut->sat_map);
      if (lut->gray_cmap)
	stp_free(lut->gray_cmap);
      if (lut->alpha_table)
	stp_free(lut->alpha_table);
      memset(lut, 0, sizeof(lut_t));
      stp_free(lut);
      stp_set_lut(v, NULL);
    }
}

void
stp_compute_lut(stp_vars_t v, size_t steps,
		stp_curve_t hue, stp_curve_t lum, stp_curve_t sat)
{
  double	pixel,		/* Pixel value */
		red_pixel,	/* Pixel value */
		green_pixel,	/* Pixel value */
		blue_pixel;	/* Pixel value */
  int i;
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
  double pivot = .25;
  double ipivot = 1.0 - pivot;
  lut_t *lut;
  double *red, *green, *blue, *composite;

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

  if (stp_get_cmap(v))
    {
      int i;
      lut->cmap = stp_get_cmap(v);
      lut->gray_cmap = stp_malloc(256);
      for (i = 0; i < 256; i++)
	lut->gray_cmap[i] = (lut->cmap[(i * 3) + 0] * LUM_RED +
			     lut->cmap[(i * 3) + 1] * LUM_GREEN +
			     lut->cmap[(i * 3) + 2] * LUM_BLUE) / 100;
    }

  /*
   * TODO check that these are wraparound curves and all that
   */
  if (hue)
    lut->hue_map = stp_curve_allocate_copy(hue);
  if (lum)
    lut->lum_map = stp_curve_allocate_copy(lum);
  if (sat)
    lut->sat_map = stp_curve_allocate_copy(sat);
    
  red = stp_malloc(sizeof(double) * steps);
  green = stp_malloc(sizeof(double) * steps);
  blue = stp_malloc(sizeof(double) * steps);
  composite = stp_malloc(sizeof(double) * steps);
  lut->steps = steps;
  
  stp_set_lut(v, lut);
  stp_dprintf(STP_DBG_LUT, v, "stp_compute_lut\n");
  stp_dprintf(STP_DBG_LUT, v, " cyan %.3f\n", cyan);
  stp_dprintf(STP_DBG_LUT, v, " magenta %.3f\n", magenta);
  stp_dprintf(STP_DBG_LUT, v, " yellow %.3f\n", yellow);
  stp_dprintf(STP_DBG_LUT, v, " print_gamma %.3f\n", print_gamma);
  stp_dprintf(STP_DBG_LUT, v, " contrast %.3f\n", contrast);
  stp_dprintf(STP_DBG_LUT, v, " brightness %.3f\n", brightness);
  stp_dprintf(STP_DBG_LUT, v, " screen_gamma %.3f\n", screen_gamma);
  for (i = 0; i < steps; i ++)
    {
      double temp_pixel;
      pixel = (double) i / (double) (steps - 1);

      if (stp_get_input_color_model(v) == COLOR_MODEL_CMY)
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

      if (pixel > .9999 && cyan < .00001)
	red_pixel = 0;
      else
	red_pixel = 1 - pow(1 - pixel, cyan);
      if (pixel > .9999 && magenta < .00001)
	green_pixel = 0;
      else
	green_pixel = 1 - pow(1 - pixel, magenta);
      if (pixel > .9999 && yellow < .00001)
	blue_pixel = 0;
      else
	blue_pixel = 1 - pow(1 - pixel, yellow);

      /*
       * Finally, fix up print gamma and scale
       */

      pixel = 65535 * pow(pixel, print_gamma) + .5;
      red_pixel = 65535 * pow(red_pixel, print_gamma) + .5;
      green_pixel = 65535 * pow(green_pixel, print_gamma) + .5;
      blue_pixel = 65535 * pow(blue_pixel, print_gamma) + .5;
      if (stp_get_output_color_model(v) == COLOR_MODEL_RGB)
	{
	  pixel = 65535 - pixel;
	  red_pixel = 65535 - red_pixel;
	  blue_pixel = 65535 - blue_pixel;
	  green_pixel = 65535 - green_pixel;
	}

      if (pixel <= 0.0)
	composite[i] = 0;
      else if (pixel >= 65535.0)
	composite[i] = 65535;
      else
	composite[i] = (pixel);

      if (red_pixel <= 0.0)
	red[i] = 0;
      else if (red_pixel >= 65535.0)
	red[i] = 65535;
      else
	red[i] = (red_pixel);

      if (green_pixel <= 0.0)
	green[i] = 0;
      else if (green_pixel >= 65535.0)
	green[i] = 65535;
      else
	green[i] = (green_pixel);

      if (blue_pixel <= 0.0)
	blue[i] = 0;
      else if (blue_pixel >= 65535.0)
	blue[i] = 65535;
      else
	blue[i] = (blue_pixel);
      stp_dprintf(STP_DBG_LUT, v,
		  "%3i  %5d  %5d  %5d  %5d\n",
		  i, (unsigned) composite[i], (unsigned) red[i],
		  (unsigned) green[i], (unsigned) blue[i]);
    }
  stp_curve_set_data(lut->composite, steps, composite);
  stp_curve_set_data(lut->red, steps, red);
  stp_curve_set_data(lut->green, steps, green);
  stp_curve_set_data(lut->blue, steps, blue);
}

#define RETURN_COLORFUNC(x)						   \
do									   \
{									   \
  stp_dprintf(STP_DBG_COLORFUNC, v,					   \
	      "stp_choose_colorfunc(type %d bpp %d cmap %d) ==> %s, %d\n", \
	      stp_get_output_type(v), image_bpp, cmap, #x, *out_bpp);	   \
  return (x);								   \
} while (0)

stp_convert_t
stp_choose_colorfunc(const stp_vars_t v,
		     int image_bpp,
		     int *out_bpp)
{
  const unsigned char *cmap = stp_get_cmap(v);
  switch (stp_get_output_type(v))
    {
    case OUTPUT_MONOCHROME:
      *out_bpp = 1;
      switch (image_bpp)
	{
	case 1:
	  if (cmap)
	    RETURN_COLORFUNC(indexed_to_monochrome);
	  else
	    RETURN_COLORFUNC(gray_to_monochrome);
	case 2:
	  if (cmap)
	    RETURN_COLORFUNC(indexed_alpha_to_monochrome);
	  else
	    RETURN_COLORFUNC(gray_alpha_to_monochrome);
	case 3:
	  RETURN_COLORFUNC(rgb_to_monochrome);
	case 4:
	  RETURN_COLORFUNC(rgb_alpha_to_monochrome);
	default:
	  RETURN_COLORFUNC(NULL);
	}
      break;
    case OUTPUT_RAW_CMYK:
      *out_bpp = 4;
      switch (image_bpp)
	{
	case 4:
	  RETURN_COLORFUNC(cmyk_8_to_cmyk);
	case 8:
	  RETURN_COLORFUNC(cmyk_to_cmyk);
	default:
	  RETURN_COLORFUNC(NULL);
	}
      break;
    case OUTPUT_COLOR:
      *out_bpp = 3;
      switch (stp_get_image_type(v))
	{
	case IMAGE_CONTINUOUS:
	  if (image_bpp >= 3)
	    RETURN_COLORFUNC(rgb_to_rgb);
	  else if (cmap == NULL)
	    RETURN_COLORFUNC(gray_to_rgb);
	  else
	    RETURN_COLORFUNC(indexed_to_rgb);
	case IMAGE_SOLID_TONE:
	  if (image_bpp >= 3)
	    RETURN_COLORFUNC(solid_rgb_to_rgb);
	  else if (cmap == NULL)
	    RETURN_COLORFUNC(gray_to_rgb);
	  else
	    RETURN_COLORFUNC(solid_indexed_to_rgb);
	case IMAGE_LINE_ART:
	  if (image_bpp >= 3)
	    RETURN_COLORFUNC(fast_rgb_to_rgb);
	  else if (cmap == NULL)
	    RETURN_COLORFUNC(fast_gray_to_rgb);
	  else
	    RETURN_COLORFUNC(fast_indexed_to_rgb);
	default:
	  RETURN_COLORFUNC(NULL);
	}
    case OUTPUT_RAW_PRINTER:
      if (image_bpp & 1 || image_bpp > 64)
	RETURN_COLORFUNC(NULL);
      *out_bpp = image_bpp / 2;
      RETURN_COLORFUNC(raw_to_raw);
    case OUTPUT_GRAY:
    default:
      *out_bpp = 1;
      switch (image_bpp)
	{
	case 1:
	  if (cmap)
	    RETURN_COLORFUNC(indexed_to_gray);
	  else
	    RETURN_COLORFUNC(gray_to_gray);
	case 2:
	  if (cmap)
	    RETURN_COLORFUNC(indexed_alpha_to_gray);
	  else
	    RETURN_COLORFUNC(gray_alpha_to_gray);
	case 3:
	  RETURN_COLORFUNC(rgb_to_gray);
	case 4:
	  RETURN_COLORFUNC(rgb_alpha_to_gray);
	default:
	  RETURN_COLORFUNC(NULL);
	}
      break;
    }
}
