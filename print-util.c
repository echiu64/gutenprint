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
 *   $Log$
 *   Revision 1.80  2000/03/01 12:48:07  rlk
 *   fix alpha channel in rgb_to_gray
 *
 *   Revision 1.79  2000/02/29 02:59:14  rlk
 *   1) Should be able to speed up black printing quite a bit for some models.
 *
 *   2) Add support for 1160 (tentative!)
 *
 *   Revision 1.78  2000/02/26 00:14:44  rlk
 *   Rename dither_{black,cmyk}4 to dither_{black,cmyk}_n, and add argument to specify how levels are to be encoded
 *
 *   Revision 1.77  2000/02/25 02:22:37  rlk
 *   1) Stylus Color 460 (really a variant 440, at least until I learn otherwise).
 *
 *   2) Major optimization for gs driver in particular: elide rows that are blank.
 *
 *   3) make variable dot size the default for those printers.
 *
 *   Revision 1.76  2000/02/21 20:32:37  rlk
 *   Important dithering bug fixes:
 *
 *   1) Avoid runaway black buildup.
 *
 *   2) Some conversion functions weren't doing density
 *
 *   Revision 1.75  2000/02/21 15:12:57  rlk
 *   Minor release prep
 *
 *   Revision 1.74  2000/02/17 01:09:10  rlk
 *   Alpha channel problems
 *
 *   Revision 1.73  2000/02/16 00:59:19  rlk
 *   1) Use correct convert functions (canon, escp2, pcl, ps).
 *
 *   2) Fix gray_to_rgb increment (print-util)
 *
 *   3) Fix dither update (print-dither)
 *
 *   Revision 1.72  2000/02/13 03:14:26  rlk
 *   Bit of an oops here about printer models; also start on print-gray-using-color mode for better quality
 *
 *   Revision 1.71  2000/02/13 02:01:38  rlk
 *   Build a Ghostscript driver!  No idea if it works yet...
 *
 *   Revision 1.70  2000/02/09 02:56:27  rlk
 *   Put lut inside vars
 *
 *   Revision 1.69  2000/02/08 12:09:23  davehill
 *   Deskjet 600C is CMY, the rest of the 6xxC series are CMYK.
 *
 *   Revision 1.68  2000/02/06 18:40:53  rlk
 *   Split out dither stuff from print-util
 *
 *   Revision 1.67  2000/02/06 04:36:20  rlk
 *   Fill in the setter functions for the dither stuff
 *
 *   Revision 1.66  2000/02/06 03:59:09  rlk
 *   More work on the generalized dithering parameters stuff.  At this point
 *   it really looks like a proper object.  Also dynamically allocate the error
 *   buffers.  This segv'd a lot, which forced me to efence it, which was just
 *   as well because I found a few problems as a result...
 *
 *   Revision 1.65  2000/02/05 20:57:39  rlk
 *   Minor reorg
 *
 *   Revision 1.64  2000/02/05 20:02:10  rlk
 *   some more silly problems
 *
 *   Revision 1.63  2000/02/05 14:56:41  rlk
 *   1) print-util.c: decrement rather than increment counter!
 *
 *   2) print-escp2.c: don't advance the paper a negative (or, with some printers,
 *   a very large positive) amount.
 *
 *   Revision 1.62  2000/02/04 09:40:28  gandy
 *   Models BJC-1000/2000/3000/6000/6100/7000/7100 ready for testing.
 *
 *   Revision 1.61  2000/02/04 01:02:15  rlk
 *   Prelim support for 850/860/870/1200; fix stupid bug in ESC(S
 *
 *   Revision 1.60  2000/02/02 13:17:10  rlk
 *   Add a few more parameters to the dither_t struct.
 *
 *   Revision 1.59  2000/02/02 03:03:55  rlk
 *   Move all the constants into members of a struct.  This will eventually permit
 *   us to use different dithering constants for each printer, or even vary them
 *   on the fly.  Currently there's a static dither_t that contains constants,
 *   but that's the easy part to fix...
 *
 *   Revision 1.58  2000/02/01 09:01:40  gandy
 *   Add print-canon.c: Support for the BJC 6000 and possibly others
 *
 *   Revision 1.57  2000/01/29 02:34:30  rlk
 *   1) Remove globals from everything except print.c.
 *
 *   2) Remove broken 1440x720 and 2880x720 microweave modes.
 *
 *   Revision 1.56  2000/01/28 03:59:53  rlk
 *   Move printers to print-util; also add top/left/bottom/right boxes to the UI
 *
 *   Revision 1.55  2000/01/25 19:51:27  rlk
 *   1) Better attempt at supporting newer Epson printers.
 *
 *   2) Generalized paper size support.
 *
 *   Revision 1.54  2000/01/21 00:53:39  rlk
 *   1) Add a few more paper sizes.
 *
 *   2) Clean up Makefile.standalone.
 *
 *   3) Nominal support for Stylus Color 850.
 *
 *   Revision 1.53  2000/01/21 00:18:59  rlk
 *   Describe the algorithms in print-util.c.
 *
 *   Revision 1.52  2000/01/17 22:23:31  rlk
 *   Print 3.1.0
 *
 *   Revision 1.51  2000/01/17 02:05:47  rlk
 *   Much stuff:
 *
 *   1) Fixes from 3.0.5
 *
 *   2) First cut at enhancing monochrome and four-level printing with stuff from
 *   the color print function.
 *
 *   3) Preliminary support (pre-support) for 440/640/740/900/750/1200.
 *
 *   Revision 1.50  2000/01/15 00:57:53  rlk
 *   Intermediate version
 *
 *   Revision 1.49  2000/01/08 23:30:37  rlk
 *   Some tweaking
 *
 *   Revision 1.48  1999/12/30 23:58:07  rlk
 *   Silly little bug...
 *
 *   Revision 1.47  1999/12/26 19:02:46  rlk
 *   Performance stuff
 *
 *   Revision 1.46  1999/12/25 17:47:17  rlk
 *   Cleanup
 *
 *   Revision 1.45  1999/12/25 00:41:01  rlk
 *   some minor improvement
 *
 *   Revision 1.44  1999/12/24 12:57:38  rlk
 *   Reduce grain; improve red
 *
 *   Revision 1.43  1999/12/22 03:24:34  rlk
 *   round length up, not down
 *
 *   Revision 1.42  1999/12/22 03:12:17  rlk
 *   More constant fiddling
 *
 *   Revision 1.41  1999/12/22 01:34:28  rlk
 *   Reverse direction each pass
 *
 *   Revision 1.40  1999/12/18 23:45:07  rlk
 *   typo
 *
 *   Revision 1.39  1999/12/12 20:49:01  rlk
 *   Various changes
 *
 *   Revision 1.38  1999/12/11 23:12:06  rlk
 *   Better matching between cmy/k
 *
 *   Smoother dither!
 *
 *   Revision 1.37  1999/12/05 23:24:08  rlk
 *   don't want PRINT_LUT in release
 *
 *   Revision 1.36  1999/12/05 04:33:34  rlk
 *   Good results for the night.
 *
 *   Revision 1.35  1999/12/04 19:01:05  rlk
 *   better use of light colors
 *
 *   Revision 1.34  1999/12/02 02:09:45  rlk
 *   .
 *
 *   Revision 1.33  1999/11/25 00:02:03  rlk
 *   Revamped many controls
 *
 *   Revision 1.32  1999/11/23 02:11:37  rlk
 *   Rationalize variables, pass 3
 *
 *   Revision 1.31  1999/11/23 01:33:37  rlk
 *   First stage of simplifying the variable stuff
 *
 *   Revision 1.30  1999/11/16 00:59:00  rlk
 *   More fine tuning
 *
 *   Revision 1.29  1999/11/14 21:37:13  rlk
 *   Revamped contrast
 *
 *   Revision 1.28  1999/11/14 18:59:22  rlk
 *   Final preparations for release to Olof
 *
 *   Revision 1.27  1999/11/14 00:57:11  rlk
 *   Mix black in sooner gives better density.
 *
 *   Revision 1.26  1999/11/13 02:31:29  rlk
 *   Finally!  Good settings!
 *
 *   Revision 1.25  1999/11/12 03:34:40  rlk
 *   More tweaking
 *
 *   Revision 1.24  1999/11/12 02:18:32  rlk
 *   Stubs for dynamic memory allocation
 *
 *   Revision 1.23  1999/11/12 01:53:37  rlk
 *   Remove silly spurious stuff
 *
 *   Revision 1.22  1999/11/12 01:51:47  rlk
 *   Much better black
 *
 *   Revision 1.21  1999/11/10 01:13:06  rlk
 *   Support up to 2880 dpi
 *
 *   Revision 1.20  1999/11/07 22:16:42  rlk
 *   Bug fixes; try to improve dithering slightly
 *
 *   Revision 1.19  1999/10/29 01:01:16  rlk
 *   Smoother rendering of darker colors
 *
 *   Revision 1.18  1999/10/28 02:01:15  rlk
 *   One bug, two effects:
 *
 *   1) Handle 4-color correctly (it was treating the 4-color too much like the
 *   6-color).
 *
 *   2) An attempt to handle both cases with the same code path led to a
 *   discontinuity that depending upon the orientation of a color gradient would
 *   lead to either white or dark lines at the point that the dark version of
 *   the color would kick in.
 *
 *   Revision 1.17  1999/10/26 23:58:31  rlk
 *   indentation
 *
 *   Revision 1.16  1999/10/26 23:36:51  rlk
 *   Comment out all remaining 16-bit code, and rename 16-bit functions to "standard" names
 *
 *   Revision 1.15  1999/10/26 02:10:30  rlk
 *   Mostly fix save/load
 *
 *   Move all gimp, glib, gtk stuff into print.c (take it out of everything else).
 *   This should help port it to more general purposes later.
 *
 *   Revision 1.14  1999/10/25 23:31:59  rlk
 *   16-bit clean
 *
 *   Revision 1.13  1999/10/25 00:14:46  rlk
 *   Remove more of the 8-bit code, now that it is tested
 *
 *   Revision 1.12  1999/10/23 20:26:48  rlk
 *   Move LUT calculation to print-util
 *
 *   Revision 1.11  1999/10/21 01:27:37  rlk
 *   More progress toward full 16-bit rendering
 *
 *   Revision 1.10  1999/10/19 02:04:59  rlk
 *   Merge all of the single-level print_cmyk functions
 *
 *   Revision 1.9  1999/10/18 01:37:02  rlk
 *   Remove spurious stuff
 *
 *   Revision 1.8  1999/10/17 23:44:07  rlk
 *   16-bit everything (untested)
 *
 *   Revision 1.7  1999/10/17 23:01:01  rlk
 *   Move various dither functions into print-utils.c
 *
 *   Revision 1.6  1999/10/14 01:59:59  rlk
 *   Saturation
 *
 *   Revision 1.5  1999/10/03 23:57:20  rlk
 *   Various improvements
 *
 *   Revision 1.4  1999/09/18 15:18:47  rlk
 *   A bit more random
 *
 *   Revision 1.3  1999/09/14 21:43:43  rlk
 *   Some hoped-for improvements
 *
 *   Revision 1.2  1999/09/12 00:12:24  rlk
 *   Current best stuff
 *
 *   Revision 1.10  1998/05/17 07:16:49  yosh
 *   0.99.31 fun
 *
 *   updated print plugin
 *
 *   -Yosh
 *
 *   Revision 1.14  1998/05/16  18:27:59  mike
 *   Cleaned up dithering functions - unnecessary extra data in dither buffer.
 *
 *   Revision 1.13  1998/05/13  17:00:36  mike
 *   Minor change to CMYK generation code - now cube black difference value
 *   for better colors.
 *
 *   Revision 1.12  1998/05/08  19:20:50  mike
 *   Updated CMYK generation code to use new method.
 *   Updated dithering algorithm (slightly more uniform now, less speckling)
 *   Added default media size function.
 *
 *   Revision 1.11  1998/03/01  18:03:27  mike
 *   Whoops - need to add 255 - alpha to the output values (transparent to white
 *   and not transparent to black...)
 *
 *   Revision 1.10  1998/03/01  17:20:48  mike
 *   Updated alpha code to do alpha computation before gamma/brightness lut.
 *
 *   Revision 1.9  1998/03/01  17:13:46  mike
 *   Updated CMY/CMYK conversion code for dynamic BG and hue adjustment.
 *   Added alpha channel support to color conversion functions.
 *
 *   Revision 1.8  1998/01/21  21:33:47  mike
 *   Replaced Burkes dither with stochastic (random) dither.
 *
 *   Revision 1.7  1997/10/02  17:57:26  mike
 *   Replaced ordered dither with Burkes dither (error-diffusion).
 *   Now dither K separate from CMY.
 *
 *   Revision 1.6  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.5  1997/07/26  18:43:04  mike
 *   Fixed dither_black and dither_cmyk - wasn't clearing extra bits
 *   (caused problems with A3/A4 size output).
 *
 *   Revision 1.5  1997/07/26  18:43:04  mike
 *   Fixed dither_black and dither_cmyk - wasn't clearing extra bits
 *   (caused problems with A3/A4 size output).
 *
 *   Revision 1.4  1997/07/02  18:46:26  mike
 *   Fixed stupid bug in dither_black() - wasn't comparing against gray
 *   pixels (comparing against the first black byte - d'oh!)
 *   Changed 255 in dither matrix to 254 to shade correctly.
 *
 *   Revision 1.4  1997/07/02  18:46:26  mike
 *   Fixed stupid bug in dither_black() - wasn't comparing against gray
 *   pixels (comparing against the first black byte - d'oh!)
 *   Changed 255 in dither matrix to 254 to shade correctly.
 *
 *   Revision 1.3  1997/07/02  13:51:53  mike
 *   Added rgb_to_rgb and gray_to_gray conversion functions.
 *   Standardized calling args to conversion functions.
 *
 *   Revision 1.2  1997/07/01  19:28:44  mike
 *   Updated dither matrix.
 *   Fixed scaling bugs in dither_*() functions.
 *
 *   Revision 1.1  1997/06/19  02:18:15  mike
 *   Initial revision
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


static void
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

static void
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
	     vars_t	*vars
	     )
{
  if (bpp == 1)
    {
      /*
       * No alpha in image...
       */

      while (width > 0)
	{
	  *grayout = vars->lut.composite[*grayin];
	  if (vars->density != 1.0)
	    {
	      float t = ((float) *grayout) / 65536.0;
	      t = (1.0 + ((t - 1.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      *grayout = (unsigned short) (t * 65536.0);
	    }
	  grayin ++;
	  grayout ++;
	  width --;
	}
    }
  else
    {
      /*
       * Handle alpha in image...
       */

      while (width > 0)
	{
	  *grayout = vars->lut.composite[grayin[0] * grayin[1] / 255 +
					255 - grayin[1]];
	  if (vars->density != 1.0)
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
		vars_t   *vars			/* I - Saturation */
		)
{
  int		i;
  unsigned char	gray_cmap[256];		/* Grayscale colormap */


  for (i = 0; i < 256; i ++, cmap += 3)
    gray_cmap[i] = (cmap[0] * LUM_RED +
		    cmap[1] * LUM_GREEN +
		    cmap[2] * LUM_BLUE) / 100;

  if (bpp == 1)
    {
      /*
       * No alpha in image...
       */

      while (width > 0)
	{
	  *gray = vars->lut.composite[gray_cmap[*indexed]];
	  if (vars->density != 1.0)
	    {
	      float t = ((float) *gray) / 65536.0;
	      t = (1.0 + ((t - 1.0) * vars->density));
	      if (t < 0.0)
		t = 0.0;
	      *gray = (unsigned short) (t * 65536.0);
	    }
	  indexed ++;
	  gray ++;
	  width --;
	}
    }
  else
    {
      /*
       * Handle alpha in image...
       */

      while (width > 0)
	{
	  *gray = vars->lut.composite[gray_cmap[indexed[0] * indexed[1] / 255]
				     + 255 - indexed[1]];
	  if (vars->density != 1.0)
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
}


void
indexed_to_rgb(unsigned char *indexed,	/* I - Indexed pixels */
	       unsigned short *rgb,	/* O - RGB pixels */
	       int    width,		/* I - Width of row */
	       int    bpp,		/* I - Bytes-per-pixel in indexed */
	       unsigned char *cmap,	/* I - Colormap */
	       vars_t   *vars		/* I - Saturation */
	       )
{
  if (bpp == 1)
    {
      /*
       * No alpha in image...
       */

      while (width > 0)
	{
	  double h, s, v;
	  rgb[0] = vars->lut.red[cmap[*indexed * 3 + 0]];
	  rgb[1] = vars->lut.green[cmap[*indexed * 3 + 1]];
	  rgb[2] = vars->lut.blue[cmap[*indexed * 3 + 2]];
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
	  rgb += 3;
	  indexed ++;
	  width --;
	}
    }
  else
    {
      /*
       * RGBA image...
       */

      while (width > 0)
	{
	  double h, s, v;
	  rgb[0] = vars->lut.red[cmap[indexed[0] * 3 + 0] * indexed[1] / 255
				+ 255 - indexed[1]];
	  rgb[1] = vars->lut.green[cmap[indexed[0] * 3 + 1] * indexed[1] / 255
				  + 255 - indexed[1]];
	  rgb[2] = vars->lut.blue[cmap[indexed[0] * 3 + 2] * indexed[1] / 255
				 + 255 - indexed[1]];
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
	  rgb += 3;
	  indexed += bpp;
	  width --;
	}
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
	    vars_t   *vars		/* I - Saturation */
	    )
{
  if (bpp == 3)
    {
      /*
       * No alpha in image...
       */

      while (width > 0)
	{
	  *gray = vars->lut.composite[(rgb[0] * LUM_RED +
				       rgb[1] * LUM_GREEN +
				       rgb[2] * LUM_BLUE) / 100];
	  gray ++;
	  rgb += 3;
	  width --;
	}
    }
  else
    {
      /*
       * Image has alpha channel...
       */

      while (width > 0)
	{
	  *gray = vars->lut.composite[((rgb[0] * LUM_RED +
					rgb[1] * LUM_GREEN +
					rgb[2] * LUM_BLUE) *
				       rgb[3] / 100 + 255 - rgb[3])];
	  gray ++;
	  rgb += bpp;
	  width --;
	}
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
	   vars_t  		*vars		/* I - Saturation */
	   )
{
  if (bpp == 3)
    {
      /*
       * No alpha in image...
       */

      while (width > 0)
	{
	  double h, s, v;
	  rgbout[0] = vars->lut.red[rgbin[0]];
	  rgbout[1] = vars->lut.green[rgbin[1]];
	  rgbout[2] = vars->lut.blue[rgbin[2]];
	  if (vars->saturation != 1.0 || vars->contrast != 100)
	    {
	      calc_rgb_to_hsv(rgbout, &h, &s, &v);
	      if (vars->saturation != 1.0)
		s = pow(s, 1.0 / vars->saturation);
#if 0
	      if (vars->contrast != 100)
		{
		  double contrast = vars->contrast / 100.0;
		  double tv = fabs(v - .5) * 2.0;
		  tv = pow(tv, 1.0 / (contrast * contrast));
		  if (v < .5)
		    tv = - tv;
		  v = (tv / 2.0) + .5;
		}
#endif
	      calc_hsv_to_rgb(rgbout, h, s, v);
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
	  rgbin += 3;
	  rgbout += 3;
	  width --;
	}
    }
  else
    {
      /*
       * RGBA image...
       */

      while (width > 0)
	{
	  double h, s, v;
	  rgbout[0] = vars->lut.red[rgbin[0] * rgbin[3] / 255 +
				   255 - rgbin[3]];
	  rgbout[1] = vars->lut.green[rgbin[1] * rgbin[3] / 255 +
				     255 - rgbin[3]];
	  rgbout[2] = vars->lut.blue[rgbin[2] * rgbin[3] / 255 +
				    255 - rgbin[3]];
	  if (vars->saturation != 1.0 || vars->contrast != 100)
	    {
	      calc_rgb_to_hsv(rgbout, &h, &s, &v);
	      if (vars->saturation != 1.0)
		s = pow(s, 1.0 / vars->saturation);
#if 0
	      if (vars->contrast != 100)
		{
		  double contrast = vars->contrast / 100.0;
		  double tv = fabs(v - .5) * 2.0;
		  tv = pow(tv, 1.0 / (contrast * contrast));
		  if (v < .5)
		    tv = - tv;
		  v = (tv / 2.0) + .5;
		}
#endif
	      calc_hsv_to_rgb(rgbout, h, s, v);
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
	  rgbin += bpp;
	  rgbout += 3;
	  width --;
	}
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
	    vars_t  		*vars		/* I - Saturation */
	    )
{
  if (bpp == 1)
    {
      /*
       * No alpha in image...
       */

      while (width > 0)
	{
	  rgbout[0] = vars->lut.red[grayin[0]];
	  rgbout[1] = vars->lut.green[grayin[0]];
	  rgbout[2] = vars->lut.blue[grayin[0]];
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
	  grayin++;
	  rgbout += 3;
	  width --;
	}
    }
  else
    {
      /*
       * RGBA image...
       */

      while (width > 0)
	{
	  rgbout[0] = vars->lut.red[(grayin[0] * grayin[1] / 255 +
				     255 - grayin[1])];
	  rgbout[1] = vars->lut.green[(grayin[0] * grayin[1] / 255 +
				       255 - grayin[1])];
	  rgbout[2] = vars->lut.blue[(grayin[0] * grayin[1] / 255 +
				      255 - grayin[1])];
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
}

/* #define PRINT_LUT */

void
compute_lut(float print_gamma,
	    float app_gamma,
	    vars_t *v)
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

  float red = 100.0 / v->red ;
  float green = 100.0 / v->green;
  float blue = 100.0 / v->blue;
  float contrast;
  contrast = v->contrast / 100.0;
  if (red < 0.01)
    red = 0.01;
  if (green < 0.01)
    green = 0.01;
  if (blue < 0.01)
    blue = 0.01;

  if (v->linear)
    {
      screen_gamma = app_gamma / 1.7;
      brightness   = v->brightness / 100.0;
    }
  else
    {
      brightness   = 100.0 / v->brightness;
      screen_gamma = app_gamma * brightness / 1.7;
    }

  print_gamma = v->gamma / print_gamma;

  for (i = 0; i < 256; i ++)
    {
      if (v->linear)
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
	  v->lut.composite[i] = adjusted_pixel;
	  v->lut.red[i] = adjusted_pixel;
	  v->lut.green[i] = adjusted_pixel;
	  v->lut.blue[i] = adjusted_pixel;
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
	    v->lut.composite[i] = 0;
	  else if (pixel >= 65535.0)
	    v->lut.composite[i] = 65535;
	  else
	    v->lut.composite[i] = (unsigned)(pixel);

	  if (red_pixel <= 0.0)
	    v->lut.red[i] = 0;
	  else if (red_pixel >= 65535.0)
	    v->lut.red[i] = 65535;
	  else
	    v->lut.red[i] = (unsigned)(red_pixel);

	  if (green_pixel <= 0.0)
	    v->lut.green[i] = 0;
	  else if (green_pixel >= 65535.0)
	    v->lut.green[i] = 65535;
	  else
	    v->lut.green[i] = (unsigned)(green_pixel);

	  if (blue_pixel <= 0.0)
	    v->lut.blue[i] = 0;
	  else if (blue_pixel >= 65535.0)
	    v->lut.blue[i] = 65535;
	  else
	    v->lut.blue[i] = (unsigned)(blue_pixel);
	}
#ifdef PRINT_LUT
      fprintf(ltfile, "%3i  %5d  %5d  %5d  %5d  %f %f %f %f  %f %f %f  %f\n",
	      i, v->lut.composite[i], v->lut.red[i],
	      v->lut.green[i], v->lut.blue[i], pixel, red_pixel,
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

#ifndef ESCP2_GHOST
const
#endif
static
printer_t	printers[] =	/* List of supported printer types */
{
#ifndef ESCP2_GHOST
  { "PostScript Level 1",	"ps",		1,	0,	1.000,	1.000,
    ps_parameters,	ps_media_size,	ps_imageable_area,	ps_print },
  { "PostScript Level 2",	"ps2",		1,	1,	1.000,	1.000,
    ps_parameters,	ps_media_size,	ps_imageable_area,	ps_print },

  { "HP DeskJet 500, 520",	"pcl-500",	0,	500,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 500C, 540C",	"pcl-501",	1,	501,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 550C, 560C",	"pcl-550",	1,	550,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 600/600C",	"pcl-600",	1,	600,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 600 series",	"pcl-601",	1,	601,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 800 series",	"pcl-800",	1,	800,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 1100C, 1120C",	"pcl-1100",	1,	1100,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP DeskJet 1200C, 1600C",	"pcl-1200",	1,	1200,	0.818,	0.786,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet II series",	"pcl-2",	0,	2,	1.000,	0.596,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet III series",	"pcl-3",	0,	3,	1.000,	0.596,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet 4 series",	"pcl-4",	0,	4,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet 4V, 4Si",	"pcl-4v",	0,	5,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet 5 series",	"pcl-5",	0,	4,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet 5Si",		"pcl-5si",	0,	5,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
  { "HP LaserJet 6 series",	"pcl-6",	0,	4,	1.000,	0.615,
    pcl_parameters,	default_media_size,	pcl_imageable_area,	pcl_print },
#endif

  { "EPSON Stylus Color",	"escp2",	1,	0,	0.597,	0.568,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color Pro",	"escp2-pro",	1,	1,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color Pro XL","escp2-proxl",	1,	1,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 1500",	"escp2-1500",	1,	2,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 400",	"escp2-400",	1,	1,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 440",	"escp2-440",	1,	10,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 460",	"escp2-460",	1,	10,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 500",	"escp2-500",	1,	1,	0.597,	0.631,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 600",	"escp2-600",	1,	3,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 640",	"escp2-640",	1,	11,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 740",	"escp2-740",	1,	12,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 800",	"escp2-800",	1,	4,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 850",	"escp2-850",	1,	5,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 860",	"escp2-860",	1,	16,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 900",	"escp2-900",	1,	13,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 1160",	"escp2-1160",	1,	17,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 1520",	"escp2-1520",	1,	6,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Color 3000",	"escp2-3000",	1,	6,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo 700",	"escp2-700",	1,	7,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo EX",	"escp2-ex",	1,	8,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo 750",	"escp2-750",	1,	14,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo 870",	"escp2-870",	1,	14,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo 1200",	"escp2-1200",	1,	15,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo 1270",	"escp2-1270",	1,	15,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },
  { "EPSON Stylus Photo",	"escp2-photo",	1,	9,	0.585,	0.646,
    escp2_parameters,	default_media_size,	escp2_imageable_area,	escp2_print },

#ifndef ESCP2_GHOST
  { "CANON BJC 1000",           "bjc-1000",     1,      1000,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 2000",           "bjc-2000",     1,      2000,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 3000",           "bjc-3000",     1,      3000,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 6000",           "bjc-6000",     1,      6000,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 6100",           "bjc-6100",     1,      6100,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 7000",           "bjc-7000",     1,      7000,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
  { "CANON BJC 7100",           "bjc-7100",     1,      7100,   1.0,    0.8,
    canon_parameters,   default_media_size,     canon_imageable_area,   canon_print },
#endif
};


int
known_printers(void)
{
  return sizeof(printers) / sizeof(printer_t);
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

/*
 * End of "$Id$".
 */
