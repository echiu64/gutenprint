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
 * See bottom
 */

/* #define PRINT_DEBUG */


#include "print.h"
#include <math.h>

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


/* #define PRINT_LUT */

void
compute_lut(const vars_t *pv,
	    float app_gamma,
	    vars_t *uv)
{
  float		brightness,	/* Computed brightness */
		screen_gamma,	/* Screen gamma correction */
		pixel,		/* Pixel value */
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

  float red = 10000.0 / (uv->red * pv->red) ;
  float green = 10000.0 / (uv->green * pv->green);
  float blue = 10000.0 / (uv->blue * pv->blue);
  float print_gamma = uv->gamma / pv->gamma;
  float contrast = (uv->contrast * pv->contrast) / 10000.0;
  if (red < 0.01)
    red = 0.01;
  if (green < 0.01)
    green = 0.01;
  if (blue < 0.01)
    blue = 0.01;

  if (uv->linear)
    {
      screen_gamma = app_gamma / 1.7;
      brightness   = (uv->brightness * pv->brightness) / 10000.0;
    }
  else
    {
      brightness   = 10000.0 / (uv->brightness * pv->brightness);
      screen_gamma = app_gamma * brightness / 1.7;
    }

  uv->lut = malloc(sizeof(lut_t));
  for (i = 0; i < 256; i ++)
    {
      if (uv->linear)
	{
	  double adjusted_pixel;
	  pixel = adjusted_pixel = (float) i / 255.0;

	  if (brightness < 1.0)
	    adjusted_pixel = adjusted_pixel * brightness;
	  else if (brightness > 1.0)
	    adjusted_pixel = 1.0 - ((1.0 - adjusted_pixel) / brightness);

	  if (pixel < 0)
	    adjusted_pixel = 0;
	  else if (pixel > 1.0)
	    adjusted_pixel = 1.0;

	  adjusted_pixel = pow(adjusted_pixel,
			       print_gamma * screen_gamma * print_gamma);

	  adjusted_pixel *= 65535.0;

	  red_pixel = green_pixel = blue_pixel = adjusted_pixel;
	  uv->lut->composite[i] = adjusted_pixel;
	  uv->lut->red[i] = adjusted_pixel;
	  uv->lut->green[i] = adjusted_pixel;
	  uv->lut->blue[i] = adjusted_pixel;
	}
      else
	{
	  float temp_pixel;
	  pixel = (float) i / 255.0;

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

	  pixel = 256.0 * (256.0 - 256.0 *
			   pow(pixel, print_gamma));
	  red_pixel = 256.0 * (256.0 - 256.0 *
			       pow(red_pixel, print_gamma));
	  green_pixel = 256.0 * (256.0 - 256.0 *
				 pow(green_pixel, print_gamma));
	  blue_pixel = 256.0 * (256.0 - 256.0 *
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
	}
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

const static papersize_t paper_sizes[] =
{
  { "Postcard", 283,  416 },
  { "4x6",	288,  432 },
  { "A6", 	295,  417 },
  { "5x8", 	360,  576 },
  { "A5", 	424,  597 },
  { "B5", 	518,  727 },
  { "8x10", 	576,  720 },
  { "A4", 	595,  842 },
  { "Letter", 	612,  792 },
  { "Legal", 	612,  1008 },
  { "B4", 	727,  1029 },
  { "Tabloid",  792,  1214 },
  { "A3", 	842,  1191 },
  { "12x18", 	864,  1296 },
  { "13x19", 	936,  1368 },
  { "A2", 	1188, 1684 },
  { "", 	0,    0 }
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
default_media_size(int  model,		/* I - Printer model */
        	   char *ppd_file,	/* I - PPD file (not used) */
        	   char *media_size,	/* I - Media size */
        	   int  *width,		/* O - Width in points */
        	   int  *length)	/* O - Length in points */
{
  const papersize_t *papersize = get_papersize_by_name(media_size);
  if (!papersize)
    {
      *width = 0;
      *length = 0;
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
