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
 * Contents:
 *
 *   gray_to_gray()       - Convert grayscale image data to grayscale.
 *   indexed_to_gray()    - Convert indexed image data to grayscale.
 *   indexed_to_rgb()     - Convert indexed image data to RGB.
 *   rgb_to_gray()        - Convert RGB image data to grayscale.
 *   rgb_to_rgb()         - Convert RGB image data to RGB.
 *   default_media_size() - Return the size of a default page size.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

/* #define PRINT_DEBUG */


#include "print.h"
#include <math.h>

#ifndef __GCC__
#  define inline
#endif /* !__GCC__ */

/*
 * RGB to grayscale luminance constants...
 */

#define LUM_RED		31
#define LUM_GREEN	61
#define LUM_BLUE	8

/* rgb/hsv conversions taken from Gimp common/autostretch_hsv.c */


static inline void
calc_rgb_to_hsv(unsigned short *rgb, double *hue, double *sat, double *val)
{
  double red, green, blue;
  double h, s, v;
  double min, max;
  double delta;

  red   = rgb[0] / 65535.0;
  green = rgb[1] / 65535.0;
  blue  = rgb[2] / 65535.0;

  h = 0.0; /* Shut up -Wall */

  if (red > green)
    {
      if (red > blue)
	max = red;
      else
	max = blue;

      if (green < blue)
	min = green;
      else
	min = blue;
    }
  else
    {
      if (green > blue)
	max = green;
      else
	max = blue;

      if (red < blue)
	min = red;
      else
	min = blue;
    }

  v = max;

  if (max != 0.0)
    s = (max - min) / max;
  else
    s = 0.0;

  if (s == 0.0)
    h = 0.0;
  else
    {
      delta = max - min;

      if (red == max)
	h = (green - blue) / delta;
      else if (green == max)
	h = 2 + (blue - red) / delta;
      else if (blue == max)
	h = 4 + (red - green) / delta;

      h /= 6.0;

      if (h < 0.0)
	h += 1.0;
      else if (h > 1.0)
	h -= 1.0;
    }

  *hue = h;
  *sat = s;
  *val = v;
}

static inline void
calc_hsv_to_rgb(unsigned short *rgb, double h, double s, double v)
{
  double hue, saturation, value;
  double f, p, q, t;

  if (s == 0.0)
    {
      h = v;
      s = v;
      v = v; /* heh */
    }
  else
    {
      hue        = h * 6.0;
      saturation = s;
      value      = v;

      if (hue == 6.0)
	hue = 0.0;

      f = hue - (int) hue;
      p = value * (1.0 - saturation);
      q = value * (1.0 - saturation * f);
      t = value * (1.0 - saturation * (1.0 - f));

      switch ((int) hue)
	{
	case 0:
	  h = value;
	  s = t;
	  v = p;
	  break;

	case 1:
	  h = q;
	  s = value;
	  v = p;
	  break;

	case 2:
	  h = p;
	  s = value;
	  v = t;
	  break;

	case 3:
	  h = p;
	  s = q;
	  v = value;
	  break;

	case 4:
	  h = t;
	  s = p;
	  v = value;
	  break;

	case 5:
	  h = value;
	  s = p;
	  v = q;
	  break;
	}
    }

  rgb[0] = h*65535;
  rgb[1] = s*65535;
  rgb[2] = v*65535;

}


/*
 * 'gray_to_gray()' - Convert grayscale image data to grayscale (brightness
 *                    adjusted).
 */

void
gray_to_gray(unsigned char *grayin,	/* I - RGB pixels */
	     unsigned short *grayout,	/* O - RGB pixels */
	     int    	width,		/* I - Width of row */
	     int    	bpp,		/* I - Bytes-per-pixel in grayin */
	     unsigned char *cmap,	/* I - Colormap (unused) */
	     const vars_t	*vars
	     )
{
  while (width > 0)
    {
      if (bpp == 1)
	/*
	 * No alpha in image...
	 */
	*grayout = vars->lut->composite[grayin[0]];
      else
	*grayout = vars->lut->composite[grayin[0] * grayin[1] / 255 +
				      255 - grayin[1]];
      if (vars->density != 1.0 && vars->image_type != IMAGE_MONOCHROME)
	{
	  float t = ((float) *grayout) / 65536.0;
	  t = (1.0 + ((t - 1.0) * vars->density));
	  if (t < 0.0)
	    t = 0.0;
	  *grayout = (unsigned short) (t * 65536.0);
	}
      grayin += bpp;
      grayout ++;
      width --;
    }
}

/*
 * 'indexed_to_gray()' - Convert indexed image data to grayscale.
 */

void
indexed_to_gray(unsigned char *indexed,		/* I - Indexed pixels */
		unsigned short *gray,		/* O - Grayscale pixels */
		int    width,			/* I - Width of row */
		int    bpp,			/* I - bpp in indexed */
		unsigned char *cmap,		/* I - Colormap */
		const vars_t   *vars			/* I - Saturation */
		)
{
  int		i;
  unsigned char	gray_cmap[256];		/* Grayscale colormap */

  /* Really should precompute this silly thing... */
  for (i = 0; i < 256; i ++, cmap += 3)
    gray_cmap[i] = (cmap[0] * LUM_RED +
		    cmap[1] * LUM_GREEN +
		    cmap[2] * LUM_BLUE) / 100;

  while (width > 0)
    {
      if (bpp == 1)
	/*
	 * No alpha in image...
	 */

	*gray = vars->lut->composite[gray_cmap[*indexed]];
      else
	*gray = vars->lut->composite[gray_cmap[indexed[0] * indexed[1] / 255]
				   + 255 - indexed[1]];
      if (vars->density != 1.0 && vars->image_type != IMAGE_MONOCHROME)
	{
	  float t = ((float) *gray) / 65536.0;
	  t = (1.0 + ((t - 1.0) * vars->density));
	  if (t < 0.0)
	    t = 0.0;
	  *gray = (unsigned short) (t * 65536.0);
	}
      indexed += bpp;
      gray ++;
      width --;
    }
}

void
indexed_to_rgb(unsigned char *indexed,	/* I - Indexed pixels */
	       unsigned short *rgb,	/* O - RGB pixels */
	       int    width,		/* I - Width of row */
	       int    bpp,		/* I - Bytes-per-pixel in indexed */
	       unsigned char *cmap,	/* I - Colormap */
	       const vars_t   *vars		/* I - Saturation */
	       )
{
  while (width > 0)
    {
      double h, s, v;
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */

	  rgb[0] = vars->lut->red[cmap[*indexed * 3 + 0]];
	  rgb[1] = vars->lut->green[cmap[*indexed * 3 + 1]];
	  rgb[2] = vars->lut->blue[cmap[*indexed * 3 + 2]];
	}
      else
	{
	  rgb[0] = vars->lut->red[cmap[indexed[0] * 3 + 0] * indexed[1] / 255
				+ 255 - indexed[1]];
	  rgb[1] = vars->lut->green[cmap[indexed[0] * 3 + 1] * indexed[1] / 255
				  + 255 - indexed[1]];
	  rgb[2] = vars->lut->blue[cmap[indexed[0] * 3 + 2] * indexed[1] / 255
				 + 255 - indexed[1]];
	}
      if (vars->saturation != 1.0)
	{
	  calc_rgb_to_hsv(rgb, &h, &s, &v);
	  s = pow(s, 1.0 / vars->saturation);
	  calc_hsv_to_rgb(rgb, h, s, v);
	}
      if (vars->density != 1.0)
	{
	  float t;
	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      t = ((float) rgb[i]) / 65536.0;
	      t = (1.0 + ((t - 1.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      rgb[i] = (unsigned short) (t * 65536.0);
	    }
	}
      indexed += bpp;
      rgb += 3;
      width --;
    }
}

/*
 * 'rgb_to_gray()' - Convert RGB image data to grayscale.
 */

void
rgb_to_gray(unsigned char *rgb,		/* I - RGB pixels */
	    unsigned short *gray,	/* O - Grayscale pixels */
	    int    width,		/* I - Width of row */
	    int    bpp,			/* I - Bytes-per-pixel in RGB */
	    unsigned char *cmap,	/* I - Colormap (unused) */
	    const vars_t   *vars		/* I - Saturation */
	    )
{
  while (width > 0)
    {
      if (bpp == 3)
	/*
	 * No alpha in image...
	 */
	*gray = vars->lut->composite[(rgb[0] * LUM_RED +
				     rgb[1] * LUM_GREEN +
				     rgb[2] * LUM_BLUE) / 100];
      else
	*gray = vars->lut->composite[((rgb[0] * LUM_RED +
				      rgb[1] * LUM_GREEN +
				      rgb[2] * LUM_BLUE) *
				     rgb[3] / 25500 + 255 - rgb[3])];
      if (vars->density != 1.0 && vars->image_type != IMAGE_MONOCHROME)
	{
	  float t = ((float) *gray) / 65536.0;
	  t = (1.0 + ((t - 1.0) * vars->density));
	  if (t < 0.0)
	    t = 0.0;
	  *gray = (unsigned short) (t * 65536.0);
	}
      rgb += bpp;
      gray ++;
      width --;
    }
}

/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

void
rgb_to_rgb(unsigned char	*rgbin,		/* I - RGB pixels */
	   unsigned short 	*rgbout,	/* O - RGB pixels */
	   int    		width,		/* I - Width of row */
	   int    		bpp,		/* I - Bytes/pix in indexed */
	   unsigned char 	*cmap,		/* I - Colormap */
	   const vars_t  		*vars		/* I - Saturation */
	   )
{
  unsigned ld = vars->density * 65536;
  double is = 1.0 / vars->saturation;
  while (width > 0)
    {
      double h, s, v;
      if (bpp == 3)
	{
	  /*
	   * No alpha in image...
	   */
	  rgbout[0] = vars->lut->red[rgbin[0]];
	  rgbout[1] = vars->lut->green[rgbin[1]];
	  rgbout[2] = vars->lut->blue[rgbin[2]];
	}
      else
	{
	  rgbout[0] = vars->lut->red[rgbin[0] * rgbin[3] / 255 +
				   255 - rgbin[3]];
	  rgbout[1] = vars->lut->green[rgbin[1] * rgbin[3] / 255 +
				     255 - rgbin[3]];
	  rgbout[2] = vars->lut->blue[rgbin[2] * rgbin[3] / 255 +
				    255 - rgbin[3]];
	}
      if (is != 1.0)
	{
	  calc_rgb_to_hsv(rgbout, &h, &s, &v);
	  s = pow(s, is);
	  calc_hsv_to_rgb(rgbout, h, s, v);
	}
      if (ld < 65536)
	{
	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      unsigned t = rgbout[i];
	      t = 65535 - (65535 - t) * ld / 65536;
	      rgbout[i] = (unsigned short) t;
	    }
	}
      rgbin += bpp;
      rgbout += 3;
      width --;
    }
}

/*
 * 'gray_to_rgb()' - Convert gray image data to RGB.
 */

void
gray_to_rgb(unsigned char	*grayin,	/* I - grayscale pixels */
	    unsigned short 	*rgbout,	/* O - RGB pixels */
	    int    		width,		/* I - Width of row */
	    int    		bpp,		/* I - Bytes/pix in indexed */
	    unsigned char 	*cmap,		/* I - Colormap */
	    const vars_t  		*vars		/* I - Saturation */
	    )
{
  while (width > 0)
    {
      if (bpp == 1)
	{
	  /*
	   * No alpha in image...
	   */

	  rgbout[0] = vars->lut->red[grayin[0]];
	  rgbout[1] = vars->lut->green[grayin[0]];
	  rgbout[2] = vars->lut->blue[grayin[0]];
	}
      else
	{
	  int lookup = (grayin[0] * grayin[1] / 255 +
			255 - grayin[1]);
	  rgbout[0] = vars->lut->red[lookup];
	  rgbout[1] = vars->lut->green[lookup];
	  rgbout[2] = vars->lut->blue[lookup];
	}
      if (vars->density != 1.0)
	{
	  float t;
	  int i;
	  for (i = 0; i < 3; i++)
	    {
	      t = ((float) rgbout[i]) / 65536.0;
	      t = (1.0 + ((t - 1.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      rgbout[i] = (unsigned short) (t * 65536.0);
	    }
	}
      grayin += bpp;
      rgbout += 3;
      width --;
    }
}

void
merge_printvars(vars_t *user, const vars_t *print)
{
  user->red = (user->red * print->red) / 100.0;
  user->green = (user->green * print->green) / 100.0;
  user->blue = (user->blue * print->blue) / 100.0;
  user->contrast = (user->contrast * print->contrast) / 100.0;
  user->brightness = (user->brightness * print->brightness) / 100.0;
  user->gamma /= print->gamma;
  user->saturation *= print->saturation;
  user->density *= print->density;
  /* Application gamma comes from the user variables */
}

static lut_t *
allocate_lut(size_t steps)
{
  lut_t *ret = malloc(sizeof(lut_t));
  ret->steps = steps;
  ret->composite = malloc(sizeof(unsigned short) * steps);
  ret->red = malloc(sizeof(unsigned short) * steps);
  ret->green = malloc(sizeof(unsigned short) * steps);
  ret->blue = malloc(sizeof(unsigned short) * steps);
  return ret;
}

void
free_lut(vars_t *v)
{
  if (v->lut)
    {
      if (v->lut->composite)
	free(v->lut->composite);
      if (v->lut->red)
	free(v->lut->red);
      if (v->lut->green)
	free(v->lut->green);
      if (v->lut->blue)
	free(v->lut->blue);
      v->lut->steps = 0;
      v->lut->composite = NULL;
      v->lut->red = NULL;
      v->lut->green = NULL;
      v->lut->blue = NULL;
      free(v->lut);
    }
  v->lut = NULL;
}

/* #define PRINT_LUT */

void
compute_lut(size_t steps, vars_t *uv)
{
  float		pixel,		/* Pixel value */
		red_pixel,	/* Pixel value */
		green_pixel,	/* Pixel value */
		blue_pixel;	/* Pixel value */
  int i;
#ifdef PRINT_LUT
  FILE *ltfile = fopen("/mnt1/lut", "w");
#endif
  /*
   * Got an output file/command, now compute a brightness lookup table...
   */

  float red = 100.0 / uv->red;
  float green = 100.0 / uv->green;
  float blue = 100.0 / uv->blue;
  float print_gamma = uv->gamma;
  float contrast = uv->contrast / 100.0;
  float app_gamma = uv->app_gamma;
  float brightness = 100.0 / uv->brightness;
  float screen_gamma = app_gamma * brightness / 1.7;

  if (red < 0.01)
    red = 0.01;
  if (green < 0.01)
    green = 0.01;
  if (blue < 0.01)
    blue = 0.01;

  uv->lut = allocate_lut(steps);
  for (i = 0; i < steps; i ++)
    {
      float temp_pixel;
      float fsteps = steps;
      pixel = (float) i / (float) (steps - 1);

      /*
       * First, correct contrast
       */
      temp_pixel = fabs((pixel - .5) * 2.0);
      temp_pixel = pow(temp_pixel, 1.0 / contrast);
      if (pixel < .5)
	temp_pixel = -temp_pixel;
      pixel = (temp_pixel / 2.0) + .5;

      /*
       * Second, perform screen gamma correction
       */
      pixel = 1.0 - pow(pixel, screen_gamma);

      /*
       * Third, fix up red, green, blue values
       *
       * I don't know how to do this correctly.  I think that what I'll do
       * is if the correction is less than 1 to multiply it by the
       * correction; if it's greater than 1, hinge it around 64K.
       * Doubtless we can do better.  Oh well.
       */
      if (pixel < 0.0)
	pixel = 0.0;
      else if (pixel > 1.0)
	pixel = 1.0;

      red_pixel = pow(pixel, 1.0 / (red * red));
      green_pixel = pow(pixel, 1.0 / (green * green));
      blue_pixel = pow(pixel, 1.0 / (blue * blue));

      /*
       * Finally, fix up print gamma and scale
       */

      pixel = fsteps * (fsteps - fsteps *
		       pow(pixel, print_gamma));
      red_pixel = fsteps * (fsteps - fsteps *
			   pow(red_pixel, print_gamma));
      green_pixel = fsteps * (fsteps - fsteps *
			     pow(green_pixel, print_gamma));
      blue_pixel = fsteps * (fsteps - fsteps *
			    pow(blue_pixel, print_gamma));

      if (pixel <= 0.0)
	uv->lut->composite[i] = 0;
      else if (pixel >= 65535.0)
	uv->lut->composite[i] = 65535;
      else
	uv->lut->composite[i] = (unsigned)(pixel);

      if (red_pixel <= 0.0)
	uv->lut->red[i] = 0;
      else if (red_pixel >= 65535.0)
	uv->lut->red[i] = 65535;
      else
	uv->lut->red[i] = (unsigned)(red_pixel);

      if (green_pixel <= 0.0)
	uv->lut->green[i] = 0;
      else if (green_pixel >= 65535.0)
	uv->lut->green[i] = 65535;
      else
	uv->lut->green[i] = (unsigned)(green_pixel);

      if (blue_pixel <= 0.0)
	uv->lut->blue[i] = 0;
      else if (blue_pixel >= 65535.0)
	uv->lut->blue[i] = 65535;
      else
	uv->lut->blue[i] = (unsigned)(blue_pixel);
#ifdef PRINT_LUT
      fprintf(ltfile, "%3i  %5d  %5d  %5d  %5d  %f %f %f %f  %f %f %f  %f\n",
	      i, uv->lut->composite[i], uv->lut->red[i],
	      uv->lut->green[i], uv->lut->blue[i], pixel, red_pixel,
	      green_pixel, blue_pixel, print_gamma, screen_gamma,
	      print_gamma, app_gamma);
#endif
    }

#ifdef PRINT_LUT
  fclose(ltfile);
#endif
}

/*
 * 'default_media_size()' - Return the size of a default page size.
 */

/*
 * Sizes are converted to 1|72in, then rounded down so that we don't
 * print off the edge of the paper.
 */
const static papersize_t paper_sizes[] =
{
  /* Common imperial page sizes */
  { "Postcard", 283,  416, PAPERSIZE_ENGLISH },	/* 100mm x 147mm */
  { "4x6",      288,  432, PAPERSIZE_ENGLISH },
  { "5x7",      360,  504, PAPERSIZE_ENGLISH },
  { "5x8",      360,  576, PAPERSIZE_ENGLISH },
  { "6x8",      432,  576, PAPERSIZE_ENGLISH },
  { "8x10",     576,  720, PAPERSIZE_ENGLISH },
  { "Manual",   396,  612, PAPERSIZE_ENGLISH },	/* 5.5in x 8.5in */
  { "Letter",   612,  792, PAPERSIZE_ENGLISH },	/* 8.5in x 11in */
  { "Legal",    612, 1008, PAPERSIZE_ENGLISH },	/* 8.5in x 14in */
  { "Tabloid",  792, 1224, PAPERSIZE_ENGLISH },	/*  11in x 17in */
  { "12x18",    864, 1296, PAPERSIZE_ENGLISH },
  { "13x19",    936, 1368, PAPERSIZE_ENGLISH },

  /* International Paper Sizes (mostly taken from BS4000:1968) */

  /*
   * "A" series: Paper and boards, trimmed sizes
   *
   * "A" sizes are in the ratio 1 : sqrt(2).  A0 has a total area
   * of 1 square metre.  Everything is rounded to the nearest
   * millimetre.  Thus, A0 is 841mm x 1189mm.  Every other A
   * size is obtained by doubling or halving another A size.
   */
  { "4A",       4767, 6740, PAPERSIZE_METRIC },	/* 1682mm x 2378mm */
  { "2A",       3370, 4767, PAPERSIZE_METRIC },	/* 1189mm x 1682mm */
  { "A0",       2383, 3370, PAPERSIZE_METRIC },	/*  841mm x 1189mm */
  { "A1",       1683, 2383, PAPERSIZE_METRIC },	/*  594mm x  841mm */
  { "A2",       1190, 1683, PAPERSIZE_METRIC },	/*  420mm x  594mm */
  { "A3",        841, 1190, PAPERSIZE_METRIC },	/*  297mm x  420mm */
  { "A4",        595,  841, PAPERSIZE_METRIC },	/*  210mm x  297mm */
  { "A5",        419,  595, PAPERSIZE_METRIC },	/*  148mm x  210mm */
  { "A6",        297,  419, PAPERSIZE_METRIC },	/*  105mm x  148mm */
  { "A7",        209,  297, PAPERSIZE_METRIC },	/*   74mm x  105mm */
  { "A8",        147,  209, PAPERSIZE_METRIC },	/*   52mm x   74mm */
  { "A9",        104,  147, PAPERSIZE_METRIC },	/*   37mm x   52mm */
  { "A10",        73,  104, PAPERSIZE_METRIC },	/*   26mm x   37mm */

  /*
   * Stock sizes for normal trims.
   * Allowance for trim is 3 millimetres.
   */
  { "RA0",      2437, 3458, PAPERSIZE_METRIC },	/*  860mm x 1220mm */
  { "RA1",      1729, 2437, PAPERSIZE_METRIC },	/*  610mm x  860mm */
  { "RA2",      1218, 1729, PAPERSIZE_METRIC },	/*  430mm x  610mm */
  { "RA3",       864, 1218, PAPERSIZE_METRIC },	/*  305mm x  430mm */
  { "RA4",       609,  864, PAPERSIZE_METRIC },	/*  215mm x  305mm */

  /*
   * Stock sizes for bled work or extra trims.
   */
  { "SRA0",     2551, 3628, PAPERSIZE_METRIC },	/*  900mm x 1280mm */
  { "SRA1",     1814, 2551, PAPERSIZE_METRIC },	/*  640mm x  900mm */
  { "SRA2",     1275, 1814, PAPERSIZE_METRIC },	/*  450mm x  640mm */
  { "SRA3",      907, 1275, PAPERSIZE_METRIC },	/*  320mm x  450mm */
  { "SRA4",      637,  907, PAPERSIZE_METRIC },	/*  225mm x  320mm */

  /*
   * "B" series: Posters, wall charts and similar items.
   */
  { "4B ISO",   5669, 8016, PAPERSIZE_METRIC },	/* 2000mm x 2828mm */
  { "2B ISO",   4008, 5669, PAPERSIZE_METRIC },	/* 1414mm x 2000mm */
  { "B0 ISO",   2834, 4008, PAPERSIZE_METRIC },	/* 1000mm x 1414mm */
  { "B1 ISO",   2004, 2834, PAPERSIZE_METRIC },	/*  707mm x 1000mm */
  { "B2 ISO",   1417, 2004, PAPERSIZE_METRIC },	/*  500mm x  707mm */
  { "B3 ISO",   1000, 1417, PAPERSIZE_METRIC },	/*  353mm x  500mm */
  { "B4 ISO",    708, 1000, PAPERSIZE_METRIC },	/*  250mm x  353mm */
  { "B5 ISO",    498,  708, PAPERSIZE_METRIC },	/*  176mm x  250mm */
  { "B6 ISO",    354,  498, PAPERSIZE_METRIC },	/*  125mm x  176mm */
  { "B7 ISO",    249,  354, PAPERSIZE_METRIC },	/*   88mm x  125mm */
  { "B8 ISO",    175,  249, PAPERSIZE_METRIC },	/*   62mm x   88mm */
  { "B9 ISO",    124,  175, PAPERSIZE_METRIC },	/*   44mm x   62mm */
  { "B10 ISO",    87,  124, PAPERSIZE_METRIC },	/*   31mm x   44mm */
  
  { "B0 JIS",   2919, 4127, PAPERSIZE_METRIC },
  { "B1 JIS",   2063, 2919, PAPERSIZE_METRIC },
  { "B2 JIS",   1459, 2063, PAPERSIZE_METRIC },
  { "B3 JIS",   1029, 1459, PAPERSIZE_METRIC },
  { "B4 JIS",    727, 1029, PAPERSIZE_METRIC },
  { "B5 JIS",    518,  727, PAPERSIZE_METRIC },
  { "B6 JIS",    362,  518, PAPERSIZE_METRIC },
  { "B7 JIS",    257,  362, PAPERSIZE_METRIC },
  { "B8 JIS",    180,  257, PAPERSIZE_METRIC },
  { "B9 JIS",    127,  180, PAPERSIZE_METRIC },
  { "B10 JIS",    90,  127, PAPERSIZE_METRIC },

  /*
   * "C" series: Envelopes or folders suitable for A size stationery.
   */
  { "C0",       2599, 3676, PAPERSIZE_METRIC },	/*  917mm x 1297mm */
  { "C1",       1836, 2599, PAPERSIZE_METRIC },	/*  648mm x  917mm */
  { "C2",       1298, 1836, PAPERSIZE_METRIC },	/*  458mm x  648mm */
  { "C3",        918, 1298, PAPERSIZE_METRIC },	/*  324mm x  458mm */
  { "C4",        649,  918, PAPERSIZE_METRIC },	/*  229mm x  324mm */
  { "C5",        459,  649, PAPERSIZE_METRIC },	/*  162mm x  229mm */
  { "B6/C4",     354,  918, PAPERSIZE_METRIC },	/*  125mm x  324mm */
  { "C6",        323,  459, PAPERSIZE_METRIC },	/*  114mm x  162mm */
  { "DL",        311,  623, PAPERSIZE_METRIC },	/*  110mm x  220mm */
  { "C7/6",      229,  459, PAPERSIZE_METRIC },	/*   81mm x  162mm */
  { "C7",        229,  323, PAPERSIZE_METRIC },	/*   81mm x  114mm */
  { "C8",        161,  229, PAPERSIZE_METRIC },	/*   57mm x   81mm */
  { "C9",        113,  161, PAPERSIZE_METRIC },	/*   40mm x   57mm */
  { "C10",        79,  113, PAPERSIZE_METRIC },	/*   28mm x   40mm */

  /*
   * Sizes for book production
   * The BPIF and the Publishers Association jointly recommend ten
   * standard metric sizes for case-bound titles as follows:
   */
  { "Crown Quarto",       535,  697, PAPERSIZE_METRIC }, /* 189mm x 246mm */
  { "Large Crown Quarto", 569,  731, PAPERSIZE_METRIC }, /* 201mm x 258mm */
  { "Demy Quarto",        620,  782, PAPERSIZE_METRIC }, /* 219mm x 276mm */
  { "Royal Quarto",       671,  884, PAPERSIZE_METRIC }, /* 237mm x 312mm */
/*{ "ISO A4",             595,  841, PAPERSIZE_METRIC },    210mm x 297mm */
  { "Crown Octavo",       348,  527, PAPERSIZE_METRIC }, /* 123mm x 186mm */
  { "Large Crown Octavo", 365,  561, PAPERSIZE_METRIC }, /* 129mm x 198mm */
  { "Demy Octavo",        391,  612, PAPERSIZE_METRIC }, /* 138mm x 216mm */
  { "Royal Octavo",       442,  663, PAPERSIZE_METRIC }, /* 156mm x 234mm */
/*{ "ISO A5",             419,  595, PAPERSIZE_METRIC },    148mm x 210mm */

  /* Paperback sizes in common usage */
  { "Small paperback",         314, 504, PAPERSIZE_METRIC }, /* 111mm x 178mm */
  { "Penguin small paperback", 314, 513, PAPERSIZE_METRIC }, /* 111mm x 181mm */
  { "Penguin large paperback", 365, 561, PAPERSIZE_METRIC }, /* 129mm x 198mm */

  { "",           0,    0, PAPERSIZE_METRIC }
};

int
known_papersizes(void)
{
  return sizeof(paper_sizes) / sizeof(papersize_t);
}

const papersize_t *
get_papersizes(void)
{
  return paper_sizes;
}

const papersize_t *
get_papersize_by_name(const char *name)
{
  const papersize_t *val = &(paper_sizes[0]);
  while (strlen(val->name) > 0)
    {
      if (!strcmp(val->name, name))
	return val;
      val++;
    }
  return NULL;
}

void
default_media_size(const printer_t *printer,
					/* I - Printer model (not used) */
        	   char *ppd_file,	/* I - PPD file (not used) */
        	   char *media_size,	/* I - Media size */
        	   int  *width,		/* O - Width in points */
        	   int  *length)	/* O - Length in points */
{
  const papersize_t *papersize = get_papersize_by_name(media_size);
  if (!papersize)
    {
      *width = 1;
      *length = 1;
    }
  else
    {
      *width = papersize->width;
      *length = papersize->length;
    }
}

/*
 * The list of printers has been moved to printers.c
 */
#include "print-printers.c"

int
known_printers(void)
{
  return printer_count;
}

const printer_t *
get_printers(void)
{
  return printers;
}

const printer_t *
get_printer_by_index(int idx)
{
  return &(printers[idx]);
}

const printer_t *
get_printer_by_long_name(const char *long_name)
{
  const printer_t *val = &(printers[0]);
  int i;
  for (i = 0; i < known_printers(); i++)
    {
      if (!strcmp(val->long_name, long_name))
	return val;
      val++;
    }
  return NULL;
}

const printer_t *
get_printer_by_driver(const char *driver)
{
  const printer_t *val = &(printers[0]);
  int i;
  for (i = 0; i < known_printers(); i++)
    {
      if (!strcmp(val->driver, driver))
	return val;
      val++;
    }
  return NULL;
}

int
get_printer_index_by_driver(const char *driver)
{
  int idx = 0;
  const printer_t *val = &(printers[0]);
  for (idx = 0; idx < known_printers(); idx++)
    {
      if (!strcmp(val->driver, driver))
	return idx;
      val++;
    }
  return -1;
}

convert_t
choose_colorfunc(int output_type,
		 int image_bpp,
		 const unsigned char *cmap,
		 int *out_bpp)
{
  if (output_type == OUTPUT_COLOR)
    {
      *out_bpp = 3;

      if (image_bpp >= 3)
	return rgb_to_rgb;
      else
	return indexed_to_rgb;
    }
  else if (output_type == OUTPUT_GRAY_COLOR)
    {
      *out_bpp = 3;
      return gray_to_rgb;
    }
  else
    {
      *out_bpp = 1;

      if (image_bpp >= 3)
	return rgb_to_gray;
      else if (cmap == NULL)
	return gray_to_gray;
      else
	return indexed_to_gray;
    }
}

void
compute_page_parameters(int page_right,	/* I */
			int page_left, /* I */
			int page_top, /* I */
			int page_bottom, /* I */
			int scaling, /* I */
			int image_width, /* I */
			int image_height, /* I */
			Image image, /* IO */
			int *orientation, /* IO */
			int *page_width, /* O */
			int *page_height, /* O */
			int *out_width,	/* O */
			int *out_height, /* O */
			int *left, /* O */
			int *top) /* O */
{
  *page_width  = page_right - page_left;
  *page_height = page_top - page_bottom;

  /* In AUTO orientation, just orient the paper the same way as the image. */

  if (*orientation == ORIENT_AUTO)
    {
      if ((*page_width >= *page_height && image_width >= image_height)
         || (*page_height >= *page_width && image_height >= image_width))
        *orientation = ORIENT_PORTRAIT;
      else
        *orientation = ORIENT_LANDSCAPE;
    }

  if (*orientation == ORIENT_LANDSCAPE)
    {
      Image_rotate_ccw(image);

      image_width  = Image_width(image);
      image_height = Image_height(image);
    }

  /*
   * Calculate width/height...
   */

  if (scaling < 0.0)
    {
      /*
       * Scale to pixels per inch...
       */

      *out_width  = image_width * -72.0 / scaling;
      *out_height = image_height * -72.0 / scaling;
    }
  else
    {
      /*
       * Scale by percent...
       */

      *out_width  = *page_width * scaling / 100.0;
      *out_height = *out_width * image_height / image_width;
      if (*out_height > *page_height)
	{
	  *out_height = *page_height * scaling / 100.0;
	  *out_width  = *out_height * image_width / image_height;
	}
    }

  if (*out_width == 0)
    *out_width = 1;
  if (*out_height == 0)
    *out_height = 1;

  if (*orientation == ORIENT_LANDSCAPE)
    {
      int x;

      /*
       * Swap left/top offsets...
       */

      x     = *left;
      *left = *top;
      *top  = *page_height - x - *out_height;
    }

  if (*left < 0)
    *left = (*page_width - *out_width) / 2;

  if (*top < 0)
    *top  = (*page_height - *out_height) / 2;
}
