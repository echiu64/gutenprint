/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
 *
 *   Copyright 1997-1998 Michael Sweet (mike@easysw.com)
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
 *   dither_black()       - Dither grayscale pixels to black.
 *   dither_cmyk()        - Dither RGB pixels to cyan, magenta, yellow, and
 *                          black.
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


/*
 * RGB to grayscale luminance constants...
 */

#define LUM_RED		31
#define LUM_GREEN	61
#define LUM_BLUE	8


/*
 * Error buffer for dither functions.  This needs to be at least 14xMAXDPI
 * (currently 720) to avoid problems...
 */

int	error[2][4][14*720+1] =
	{
	  { { 0 }, { 0 }, { 0 } , { 0 } },
	  { { 0 }, { 0 }, { 0 } , { 0 } }
	};


/*
int	error6[2][7][14*720+1] =
	{
	  { { 0 }, { 0 }, { 0 } , { 0 }, { 0 }, { 0 } },
	  { { 0 }, { 0 }, { 0 } , { 0 }, { 0 }, { 0 } }
	};
*/


/*
 * 'dither_black()' - Dither grayscale pixels to black.
 */

void
dither_black(guchar        *gray,	/* I - Grayscale pixels */
             int           row,		/* I - Current Y coordinate */
             int           src_width,	/* I - Width of input row */
             int           dst_width,	/* I - Width of output row */
             unsigned char *black)	/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k,		/* Current black error */
		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dithering bitmask */


  xstep  = src_width / dst_width;
  xmod   = src_width % dst_width;
  length = (dst_width) / 8;

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];

  memset(black, 0, length);

  for (x = 0, bit = 128, kptr = black, xerror = 0,
           ditherbit = rand(), ditherk = *kerror0;
       x < dst_width;
       x ++, kerror0 ++, kerror1 ++)
  {
    k = 255 - *gray + ditherk / 8;

    if (k > 127)
    {
      *kptr |= bit;
      k -= 255;
    };

    if (ditherbit & bit)
    {
      kerror1[0] = 5 * k;
      ditherk    = kerror0[1] + 3 * k;
    }
    else
    {
      kerror1[0] = 3 * k;
      ditherk    = kerror0[1] + 5 * k;
    };

    if (bit == 1)
    {
      kptr ++;
      bit       = 128;
      ditherbit = rand();
    }
    else
      bit >>= 1;

    gray   += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      gray   ++;
    };
  };
}


/*
 * 'dither_cmyk()' - Dither RGB pixels to cyan, magenta, yellow, and black.
 */

void
dither_cmyk(guchar        *rgb,		/* I - RGB pixels */
            int           row,		/* I - Current Y coordinate */
            int           src_width,	/* I - Width of input row */
            int           dst_width,	/* I - Width of output rows */
            unsigned char *cyan,	/* O - Cyan bitmap pixels */
            unsigned char *magenta,	/* O - Magenta bitmap pixels */
            unsigned char *yellow,	/* O - Yellow bitmap pixels */
            unsigned char *black)	/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  int		c, m, y, k,	/* CMYK values */
		divk,		/* Inverse of K */
		diff;		/* Average color difference */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc,	/* Next error value in buffer */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		dithery,	/* Next error value in buffer */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		ditherm,	/* Next error value in buffer */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */

  xstep  = 3 * (src_width / dst_width);
  xmod   = src_width % dst_width;
  length = (dst_width) / 8;

  cerror0 = error[row & 1][0];
  cerror1 = error[1 - (row & 1)][0];

  merror0 = error[row & 1][1];
  merror1 = error[1 - (row & 1)][1];

  yerror0 = error[row & 1][2];
  yerror1 = error[1 - (row & 1)][2];

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];

  memset(cyan, 0, length);
  memset(magenta, 0, length);
  memset(yellow, 0, length);
  if (black != NULL)
    memset(black, 0, length);

  for (x = 0, bit = 128, cptr = cyan, mptr = magenta, yptr = yellow,
           kptr = black, xerror = 0, ditherbit = rand(), ditherc = cerror0[0],
           ditherm = merror0[0], dithery = yerror0[0], ditherk = kerror0[0];
       x < dst_width;
       x ++, cerror0 ++, cerror1 ++, merror0 ++, merror1 ++, yerror0 ++,
           yerror1 ++, kerror0 ++, kerror1 ++)
  {
   /*
    * First compute the standard CMYK separation color values...
    */

    c = 255 - rgb[0];
    m = 255 - rgb[1];
    y = 255 - rgb[2];
    k = MIN(c, MIN(m, y));

    if (black != NULL)
    {
     /*
      * Since we're printing black, adjust the black level based upon
      * the amount of color in the pixel (colorful pixels get less black)...
      */

      diff = 255 - (abs(c - m) + abs(c - y) + abs(m - y)) / 3;
      diff = diff * diff * diff / 65025; /* diff = diff^3 */
      k    = diff * k / 255;
      divk = 255 - k;
      
      if (divk == 0)
        c = m = y = 0;	/* Grayscale */
      else
      {
       /*
        * Full color; update the CMY values for the black value and reduce
        * CMY as necessary to give better blues, greens, and reds... :)
        */

        c  = (255 - rgb[1] / 4) * (c - k) / divk;
        m  = (255 - rgb[2] / 4) * (m - k) / divk;
        y  = (255 - rgb[0] / 4) * (y - k) / divk;
      };

      k += ditherk / 8;
      if (k > 127)
      {
	*kptr |= bit;
	k -= 255;
      };

      if (ditherbit & bit)
      {
	kerror1[0] = 5 * k;
	ditherk    = kerror0[1] + 3 * k;
      }
      else
      {
	kerror1[0] = 3 * k;
	ditherk    = kerror0[1] + 5 * k;
      };

      if (bit == 1)
        kptr ++;
    }
    else
    {
     /*
      * We're not printing black, but let's adjust the CMY levels to produce
      * better reds, greens, and blues...
      */

      c  = (255 - rgb[1] / 4) * (c - k) / 255 + k;
      m  = (255 - rgb[2] / 4) * (m - k) / 255 + k;
      y  = (255 - rgb[0] / 4) * (y - k) / 255 + k;
    };

    c += ditherc / 8;
    if (c > 127)
    {
      *cptr |= bit;
      c -= 255;
    };


    if (ditherbit & bit)
    {
      cerror1[0] = 5 * c;
      ditherc    = cerror0[1] + 3 * c;
    }
    else
    {
      cerror1[0] = 3 * c;
      ditherc    = cerror0[1] + 5 * c;
    };

    m += ditherm / 8;
    if (m > 127)
    {
      *mptr |= bit;
      m -= 255;
    };

    if (ditherbit & bit)
    {
      merror1[0] = 5 * m;
      ditherm    = merror0[1] + 3 * m;
    }
    else
    {
      merror1[0] = 3 * m;
      ditherm    = merror0[1] + 5 * m;
    };

    y += dithery / 8;
    if (y > 127)
    {
      *yptr |= bit;
      y -= 255;
    };

    if (ditherbit & bit)
    {
      yerror1[0] = 5 * y;
      dithery    = yerror0[1] + 3 * y;
    }
    else
    {
      yerror1[0] = 3 * y;
      dithery    = yerror0[1] + 5 * y;
    };

    if (bit == 1)
    {
      cptr ++;
      mptr ++;
      yptr ++;
      bit       = 128;
      ditherbit = rand();
    }
    else
      bit >>= 1;

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      rgb    += 3;
    };
  };
}

/*
 * 'dither_cmyk6()' - Dither RGB pixels to cyan, magenta, light cyan,
 * light magenta, yellow, and black.
 *
 * Added by Robert Krawitz <rlk@alum.mit.edu> August 30, 1999.
 */

#define TURNOVER_K_L 24
#define TURNOVER_K_H 80
#define KDARKNESS_LIMIT 60

/*
 * Ratios of dark to light inks.  The darker ink should be DE / NU darker
 * than the light ink.
 *
 * It is essential to be very careful about use of parentheses with these
 * macros!
 */

#define NU_C 1
#define DE_C 3
#define NU_M 1
#define DE_M 3

#define I_RATIO_M NU_M / DE_M
#define I_RATIO_M1 NU_M / (DE_M + NU_M)
#define RATIO_M DE_M / NU_M
#define RATIO_M1 (DE_M + NU_M) / NU_M
#define I_RATIO_C NU_C / DE_C
#define I_RATIO_C1 NU_C / (DE_C + NU_C)
#define RATIO_C DE_C / NU_C
#define RATIO_C1 (DE_C + NU_C) / NU_C

void
dither_cmyk6(guchar        *rgb,	/* I - RGB pixels */
	     int           row,		/* I - Current Y coordinate */
	     int           src_width,	/* I - Width of input row */
	     int           dst_width,	/* I - Width of output rows */
	     unsigned char *cyan,	/* O - Cyan bitmap pixels */
	     unsigned char *magenta,	/* O - Magenta bitmap pixels */
	     unsigned char *lcyan,	/* O - Light cyan bitmap pixels */
	     unsigned char *lmagenta,	/* O - Light magenta bitmap pixels */
	     unsigned char *yellow,	/* O - Yellow bitmap pixels */
	     unsigned char *black)	/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  int		c, m, y, k,	/* CMYK values */
		lc, lm, oc, om, ok, oy, lastc, lastm,
		divk,		/* Inverse of K */
		diff;		/* Average color difference */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*lmptr,		/* Current magenta pixel */
		*lcptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc,	/* Next error value in buffer */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		dithery,	/* Next error value in buffer */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		ditherm,	/* Next error value in buffer */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */
  int nk;
  int ck;
  int bk;
  int ub, lb;
#ifdef PRINT_DEBUG
  int odk, odc, odm, ody, dk, dc, dm, dy, xk, xc, xm, xy, yc, ym, yy;
  FILE *dbg;
#endif

  xstep  = 3 * (src_width / dst_width);
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  cerror0 = error[row & 1][0];
  cerror1 = error[1 - (row & 1)][0];

  merror0 = error[row & 1][1];
  merror1 = error[1 - (row & 1)][1];

  yerror0 = error[row & 1][5];
  yerror1 = error[1 - (row & 1)][5];

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];

  memset(cyan, 0, length);
  memset(magenta, 0, length);
  memset(lcyan, 0, length);
  memset(lmagenta, 0, length);
  memset(yellow, 0, length);
  if (black != NULL)
    memset(black, 0, length);

#ifdef PRINT_DEBUG
  dbg = fopen("/mnt1/dbg", "a");
#endif
  for (x = 0, bit = 128, cptr = cyan, mptr = magenta, yptr = yellow,
	   lcptr = lcyan, lmptr = lmagenta,
           kptr = black, xerror = 0, ditherbit = rand(), ditherc = cerror0[0],
           ditherm = merror0[0], dithery = yerror0[0], ditherk = kerror0[0];
       x < dst_width;
       x ++, cerror0 ++, cerror1 ++, merror0 ++, merror1 ++, yerror0 ++,
           yerror1 ++, kerror0 ++, kerror1 ++)
  {

   /*
    * First compute the standard CMYK separation color values...
    */
    int ditherbit0 = ditherbit & 0xff;
    int ditherbit1 = ((ditherbit >> 8) & 0xff);
    int ditherbit2 = ((ditherbit >> 16) & 0xff);
    int ditherbit3 = ((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) ;
		   
#if 0
    int cdarkness;
    int mdarkness;
#endif
    int maxlevel;
    int ak;
    int kdarkness;

    lastc = 0;
    lastm = 0;
    c = 255 - (unsigned) rgb[0];
    m = 255 - (unsigned) rgb[1];
    y = 255 - (unsigned) rgb[2];
    oc = c;
    om = m;
    oy = y;
    lc = 0;
    lm = 0;
    k = MIN(c, MIN(m, y));
#ifdef PRINT_DEBUG
    xc = c;
    xm = m;
    xy = y;
    xk = k;
    yc = c;
    ym = m;
    yy = y;
#endif
    maxlevel = MAX(c, MAX(m, y));
    /*
     * In situations with low black value, it's probably better to use CMY
     * to handle the black value.  Maybe 0 < k < 63 use CMY, 63 < k < 127
     * or whatever to split the difference, and above that use black?
     *
     * Next idea: compare min level to max level.  If the difference is large
     * enough, we'll do the color thing, otherwise just go straight
     * black.
     *
     * The idea is to avoid nasty black dots in an otherwise light
     * region.
     *
     * -- rlk 19990829
     */
#if 0
    if (kdarkness < 32) {
      if (k < 32 - kdarkness)
	k = 0;
      else if (k < 64 - kdarkness)
	k = (2 * k - (64 - kdarkness));
    }
#endif

    if (black != NULL)
    {
     /*
      * Since we're printing black, adjust the black level based upon
      * the amount of color in the pixel (colorful pixels get less black)...
      */

      diff = 255 - (abs(c - m) + abs(c - y) + abs(m - y)) / 3;
      diff = diff * diff * diff / 65025; /* diff = diff^3 */
      k    = diff * k / 255;
      ak = k;
      divk = 255 - k;
      
      if (divk == 0)
        c = m = y = lm = lc = 0;	/* Grayscale */
      else
      {
       /*
        * Full color; update the CMY values for the black value and reduce
        * CMY as necessary to give better blues, greens, and reds... :)
        */

        c  = (255 - rgb[1] / 8) * (c - k) / divk;
        m  = (255 - rgb[2] / 8) * (m - k) / divk;
        y  = (255 - rgb[0] / 8) * (y - k) / divk;
      };
      kdarkness = (c + m + c) / 3;
#ifdef PRINT_DEBUG
      yc = c;
      ym = m;
      yy = y;
#endif
      /* Need to do the diffuse-the-black-into-cmyk thing here, too */
      ok = k;
      nk = k + (ditherk + 7) / 8;
      if (kdarkness < KDARKNESS_LIMIT)
	{
	  if (ak < (KDARKNESS_LIMIT - kdarkness) / 2)
	    {
	      bk = 0;
	      ub = 0;
	      lb = 1;
	    }
	  else if (ak < (KDARKNESS_LIMIT - kdarkness))
	    {
	      ub = (ak * 2) - (KDARKNESS_LIMIT - kdarkness);
	      lb = KDARKNESS_LIMIT - kdarkness;
	      if (((ditherbit / bit) % lb) < ub)
		bk = nk;
	      else
		bk = 0;
	      /* bk = nk * ub / lb; */
	      /* bk = nk * ((2 * (ak + kdarkness)) - KDARKNESS_LIMIT) / (ak + kdarkness); */
	    }
	  else
	    {
	      ub = 1;
	      lb = 1;
	      bk = nk;
	    }
	}
      else
	bk = nk;
      ck = nk - bk;
    
      c += ck ;
      m += ck * 3 / 2;
      y += ck * 3 / 2;
      k = bk;
#ifdef PRINT_DEBUG
      odk = ditherk;
      dk = k;
#endif
      if (k > 127 + ((ditherbit0 / 2) - 64))
	{
	  *kptr |= bit;
	  k -= 255;
	};

      if (ditherbit0 & bit)
	{
	  kerror1[0] = 5 * k;
	  ditherk    = kerror0[1] + 3 * k;
	}
      else
	{
	  kerror1[0] = 3 * k;
	  ditherk    = kerror0[1] + 5 * k;
	};

      if (bit == 1)
        kptr ++;
    }
    else
    {
     /*
      * We're not printing black, but let's adjust the CMY levels to produce
      * better reds, greens, and blues...
      */

      ok = 0;
      c  = (255 - rgb[1] / 4) * (c - k) / 255 + k;
      m  = (255 - rgb[2] / 4) * (m - k) / 255 + k;
      y  = (255 - rgb[0] / 4) * (y - k) / 255 + k;
    };

#if 0
    cdarkness = MAX((oy / 2), MAX ((ok), MAX(om / 2, oc)));
    mdarkness = MAX((oy / 2), MAX ((ok), MAX(om, oc / 2)));
#endif

    oc = c;
    c += (ditherc + 7) / 8;
#ifdef PRINT_DEBUG
    odc = ditherc;
    dc = c;
#endif
    if ((oc <= 255 * I_RATIO_C1) &&
	(c > ((((127 - lastc) * I_RATIO_C1) - lastc)
	      + (((ditherbit2 / 2) - 64) * I_RATIO_C1))))
      {
	*lcptr |= bit;
	c -= 255 * I_RATIO_C1;
	lastc = c;
      }
    else if (/* oc >= 255 && */ c > 127 + ((ditherbit2 / 2) - 64))
      {
	*cptr |= bit;
	c -= 255;
	lastc = c;
      }
    else if (c > (((127 - lastc) * I_RATIO_C1) - lastc) + ((127 - lastc) * oc * I_RATIO_C / 255)
	     /* + (((ditherbit0 / 2) - 64) * I_RATIO_C1) */)
      {
	if (ditherbit2 < ((oc - (255 * I_RATIO_C1)) * RATIO_C1 * I_RATIO_C))
	  {
	    *cptr |= bit;
	    c -= 255;
	    lastc = c;
	  }
	else
	  {
	    *lcptr |= bit;
	    c -= 255 / I_RATIO_C1;
	    lastc = c;
	  }
      }

    if (ditherbit2 & bit)
    {
      cerror1[0] = 5 * c;
      ditherc    = cerror0[1] + 3 * c;
    }
    else
    {
      cerror1[0] = 3 * c;
      ditherc    = cerror0[1] + 5 * c;
    };



    om = m;
    m += (ditherm + 7) / 8;
#ifdef PRINT_DEBUG
    odm = ditherm;
    dm = m;
#endif
    if ((om <= 255 * I_RATIO_M1) &&
	(m > ((((127 - lastm) * I_RATIO_M1) - lastm)
	      + (((ditherbit1 / 2) - 64) * I_RATIO_M1))))
      {
	*lmptr |= bit;
	m -= 255 * I_RATIO_M1;
	lastm = m;
      }
    else if (/* om >= 255 && */ m > 127 + ((ditherbit1 / 2) - 64))
      {
	*mptr |= bit;
	m -= 255;
	lastm = m;
      }
    else if (m > (((127 - lastm) * I_RATIO_M1) - lastm) + ((127 - lastm) * om * I_RATIO_M / 255))
      {
	if (ditherbit1 < ((om - (255 * I_RATIO_M1)) * RATIO_M1 * I_RATIO_M))
	  {
	    *mptr |= bit;
	    m -= 255;
	    lastm = m;
	  }
	else
	  {
	    *lmptr |= bit;
	    m -= 255 / I_RATIO_M1;
	    lastm = m;
	  }
      }

    if (ditherbit1 & bit)
    {
      merror1[0] = 5 * m;
      ditherm    = merror0[1] + 3 * m;
    }
    else
    {
      merror1[0] = 3 * m;
      ditherm    = merror0[1] + 5 * m;
    };


    oy = y;
    y += (dithery + 7) / 8;
#ifdef PRINT_DEBUG
    ody = dithery;
    dy = y;
#endif

    if (y > (127 + ((ditherbit3 / 2) - 64)))
    {
      *yptr |= bit;
      y -= 255;
    };

    if (ditherbit3 & bit)
    {
      yerror1[0] = 5 * y;
      dithery    = yerror0[1] + 3 * y;
    }
    else
    {
      yerror1[0] = 3 * y;
      dithery    = yerror0[1] + 5 * y;
    };

#ifdef PRINT_DEBUG
    fprintf(dbg, "   x %d y %d  r %d g %d b %d  xc %d xm %d xy %d yc %d ym %d yy %d xk %d  diff %d divk %d  oc %d om %d oy %d ok %d  c %d m %d y %d k %d  %c%c%c%c%c%c  dk %d dc %d dm %d dy %d  kd %d ck %d bk %d nk %d ub %d lb %d\n",
	    x, row,
	    rgb[0], rgb[1], rgb[2],
	    xc, xm, xy, yc, ym, yy, xk, diff, divk,
	    oc, om, oy, ok,
	    dc, dm, dy, dk,
	    (*cptr & bit) ? 'c' : ' ',
	    (*lcptr & bit) ? 'C' : ' ',
	    (*mptr & bit) ? 'm' : ' ',
	    (*lmptr & bit) ? 'M' : ' ',
	    (*yptr & bit) ? 'y' : ' ',
	    (*kptr & bit) ? 'k' : ' ',
	    odk, odc, odm, ody,
	    kdarkness, ck, bk, nk, ub, lb);
#endif
	    
    if (bit == 1)
    {
      cptr ++;
      mptr ++;
      lcptr ++;
      lmptr ++;
      yptr ++;
      bit       = 128;
      ditherbit = rand();
    }
    else
      bit >>= 1;

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      rgb    += 3;
    };
  };
#ifdef PRINT_DEBUG
  fprintf(dbg, "\n");
  fclose(dbg);
#endif
}

#if 0
#define KDARKNESS_LIMIT16 (KDARKNESS_LIMIT * 256)
#endif

#define KDARKNESS_LOWER_16 (80 * 256)
#define KDARKNESS_UPPER_16 (144 * 256)

void
dither_cmyk6_16(gushort       *rgb,	/* I - RGB pixels */
		int           row,	/* I - Current Y coordinate */
		int           src_width,/* I - Width of input row */
		int           dst_width,/* I - Width of output rows */
		unsigned char *cyan,	/* O - Cyan bitmap pixels */
		unsigned char *magenta,	/* O - Magenta bitmap pixels */
		unsigned char *lcyan,	/* O - Light cyan bitmap pixels */
		unsigned char *lmagenta,/* O - Light magenta bitmap pixels */
		unsigned char *yellow,	/* O - Yellow bitmap pixels */
		unsigned char *black)	/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  long long	c, m, y, k,	/* CMYK values */
		lc, lm, oc, om, ok, oy,
		divk;		/* Inverse of K */
  long long     diff;		/* Average color difference */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*lmptr,		/* Current magenta pixel */
		*lcptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc,	/* Next error value in buffer */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		dithery,	/* Next error value in buffer */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		ditherm,	/* Next error value in buffer */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */
  int nk;
  int ck;
  int bk;
  int ub, lb;
  int ditherbit0, ditherbit1, ditherbit2, ditherbit3;
#ifdef PRINT_DEBUG
  long long odk, odc, odm, ody, dk, dc, dm, dy, xk, xc, xm, xy, yc, ym, yy;
  FILE *dbg;
#endif

  xstep  = 3 * (src_width / dst_width);
  xmod   = src_width % dst_width;
  length = (dst_width + 7) / 8;

  cerror0 = error[row & 1][0];
  cerror1 = error[1 - (row & 1)][0];

  merror0 = error[row & 1][1];
  merror1 = error[1 - (row & 1)][1];

  yerror0 = error[row & 1][5];
  yerror1 = error[1 - (row & 1)][5];

  kerror0 = error[row & 1][3];
  kerror1 = error[1 - (row & 1)][3];

  memset(cyan, 0, length);
  memset(magenta, 0, length);
  memset(lcyan, 0, length);
  memset(lmagenta, 0, length);
  memset(yellow, 0, length);
  if (black != NULL)
    memset(black, 0, length);

#ifdef PRINT_DEBUG
  dbg = fopen("/mnt1/dbg", "a");
#endif

  /*
   * Main loop starts here!
   */
  for (x = 0, bit = 128,
	 cptr = cyan, mptr = magenta, yptr = yellow, lcptr = lcyan, lmptr = lmagenta,
	 kptr = black, xerror = 0, ditherbit = rand(),
	 ditherc = cerror0[0], ditherm = merror0[0], dithery = yerror0[0],
	 ditherk = kerror0[0],
	 ditherbit0 = ditherbit & 0xffff,
	 ditherbit1 = ((ditherbit >> 8) & 0xffff),
	 ditherbit2 = (((ditherbit >> 16) & 0x7fff) +
		       ((ditherbit & 0x100) << 7)),
	 ditherbit3 = (((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
		       ((ditherbit >> 8) & 0xff00));
       x < dst_width;
       x ++, cerror0 ++, cerror1 ++, merror0 ++, merror1 ++, yerror0 ++,
           yerror1 ++, kerror0 ++, kerror1 ++)
  {

   /*
    * First compute the standard CMYK separation color values...
    */
		   
#if 0
    int cdarkness;
    int mdarkness;
#endif
    int maxlevel;
    int ak;
    int kdarkness;

    c = 65535 - (unsigned) rgb[0];
    m = 65535 - (unsigned) rgb[1];
    y = 65535 - (unsigned) rgb[2];
    oc = c;
    om = m;
    oy = y;
    lc = 0;
    lm = 0;
    k = MIN(c, MIN(m, y));
#ifdef PRINT_DEBUG
    xc = c;
    xm = m;
    xy = y;
    xk = k;
    yc = c;
    ym = m;
    yy = y;
#endif
    maxlevel = MAX(c, MAX(m, y));
    /*
     * In situations with low black value, it's probably better to use CMY
     * to handle the black value.  Maybe 0 < k < 63 use CMY, 63 < k < 127
     * or whatever to split the difference, and above that use black?
     *
     * Next idea: compare min level to max level.  If the difference is large
     * enough, we'll do the color thing, otherwise just go straight
     * black.
     *
     * The idea is to avoid nasty black dots in an otherwise light
     * region.
     *
     * -- rlk 19990829
     */
#if 0
    if (kdarkness < 32) {
      if (k < 32 - kdarkness)
	k = 0;
      else if (k < 16384 - kdarkness)
	k = (2 * k - (16384 - kdarkness));
    }
#endif

    if (black != NULL)
    {
     /*
      * Since we're printing black, adjust the black level based upon
      * the amount of color in the pixel (colorful pixels get less black)...
      */

      diff = 65536 - (abs(c - m) + abs(c - y) + abs(m - y)) / 3;
      diff = diff * diff * diff / (65536ll * 65536ll); /* diff = diff^3 */
      diff--;
      if (diff < 0)
	diff = 0;
      k    = diff * k / 65535ll;
      ak = k;
      divk = 65535 - k;
      if (divk == 0)
        c = m = y = lm = lc = 0;	/* Grayscale */
      else
      {
       /*
        * Full color; update the CMY values for the black value and reduce
        * CMY as necessary to give better blues, greens, and reds... :)
        */

        c  = (65535 - (unsigned) rgb[1] / 4) * (c - k) / divk;
        m  = (65535 - (unsigned) rgb[2] / 4) * (m - k) / divk;
        y  = (65535 - (unsigned) rgb[0] / 4) * (y - k) / divk;
      };
      kdarkness = (c + m + c) / 3;
#ifdef PRINT_DEBUG
      yc = c;
      ym = m;
      yy = y;
#endif
      /* Need to do the diffuse-the-black-into-cmyk thing here, too */
      ok = k;
      nk = k + (ditherk) / 8;
      if (kdarkness < KDARKNESS_UPPER_16)
	{
	  if (ak < (KDARKNESS_UPPER_16 - kdarkness) * KDARKNESS_LOWER_16 / KDARKNESS_UPPER_16)
	    {
	      bk = 0;
	      ub = 0;
	      lb = 1;
	    }
	  else if (ak < (KDARKNESS_UPPER_16 - kdarkness))
	    {
	      lb = ((KDARKNESS_UPPER_16 - KDARKNESS_LOWER_16) *
		    (KDARKNESS_UPPER_16 - kdarkness) / KDARKNESS_UPPER_16);
	      ub = lb - ((KDARKNESS_UPPER_16 - kdarkness) - ak);
	      if (lb == 0 || ((ditherbit / bit) % lb) < ub)
		bk = nk;
	      else
		bk = 0;
	    }
	  else
	    {
	      ub = 1;
	      lb = 1;
	      bk = nk;
	    }
	}
      else
	bk = nk;
      ck = nk - bk;
    
      c += ck ;
      m += ck * 9 / 8;
      y += ck * 3 / 2;
      if (c > 65535)
	c = 65536;
      if (m > 65535)
	m = 65536;
      if (y > 65535)
	y = 65536;
      k = bk;
#ifdef PRINT_DEBUG
      odk = ditherk;
      dk = k;
#endif
      if (k > 32767 + ((ditherbit0 / 2) - 16384))
	{
	  *kptr |= bit;
	  k -= 65535;
	};

      if (ditherbit0 & bit)
	{
	  kerror1[0] = 5 * k;
	  ditherk    = kerror0[1] + 3 * k;
	}
      else
	{
	  kerror1[0] = 3 * k;
	  ditherk    = kerror0[1] + 5 * k;
	};

      if (bit == 1)
        kptr ++;
    }
    else
    {
     /*
      * We're not printing black, but let's adjust the CMY levels to produce
      * better reds, greens, and blues...
      */

      ok = 0;
      c  = (65535 - rgb[1] / 4) * (c - k) / 65535 + k;
      m  = (65535 - rgb[2] / 4) * (m - k) / 65535 + k;
      y  = (65535 - rgb[0] / 4) * (y - k) / 65535 + k;
    }


    /*****************************************************************
     * Cyan
     *****************************************************************/
    oc = c;
    c += (ditherc) / 8;
#ifdef PRINT_DEBUG
    odc = ditherc;
    dc = c;
#endif

    if ((oc <= (65536 * I_RATIO_C1 * 2 / 3)))
      {
	if (c > (32767 + (((long long) ditherbit2 / 4) - 8192)) * I_RATIO_C1)
	  {
#ifdef PRINT_DEBUG
	    fprintf(dbg, "Case 1: oc %lld c %lld test %lld\n", oc, c,
		    (32767 + (((long long) ditherbit2 / 4) - 8192)) * I_RATIO_C1);
#endif
	    *lcptr |= bit;
	    c -= 65535 * I_RATIO_C1;
	  }
      }
    else if (c > ((32767 + (((long long) ditherbit2 / 4) - 8192)) * oc / 65536))
      {
	if (ditherbit1 >
	    ((oc - (65536 * I_RATIO_C1 * 2 / 3)) * 65536 /
	     (65536 - (65536 * I_RATIO_C1 * 2 / 3))))
	  {
#ifdef PRINT_DEBUG
	    fprintf(dbg, "Case 2: oc %lld c %lld ditherbit1 %d ditherbit2 %d num %lld den %lld test1 %lld test2 %lld\n",
		    oc, c, ditherbit1, ditherbit2,
		    oc, 65536ll,
		    ((32767 + (((long long) ditherbit2 / 4) - 8192)) * oc / 65536),
		    ((oc - (65536 * I_RATIO_C1 * 2 / 3)) * 65536 /
		     (65536 - (65536 * I_RATIO_C1 * 2 / 3))));
#endif
	    *lcptr |= bit;
	    c -= 65535 * I_RATIO_C1;
	  }
	else
	  {
#ifdef PRINT_DEBUG
	    fprintf(dbg, "Case 3: oc %lld c %lld ditherbit1 %d ditherbit2 %d num %lld den %lld test1 %lld test2 %lld\n",
		    oc, c, ditherbit1, ditherbit2,
		    oc, 65536ll,
		    ((32767 + (((long long) ditherbit2 / 4) - 8192)) * oc / 65536),
		    ((oc - (65536 * I_RATIO_C1 * 2 / 3)) * 65536 /
		     (65536 - (65536 * I_RATIO_C1 * 2 / 3))));
#endif
	    *cptr |= bit;
	    c -= 65535;
	  }
      }

    if (ditherbit2 & bit)
    {
      cerror1[0] = 5 * c;
      ditherc    = cerror0[1] + 3 * c;
    }
    else
    {
      cerror1[0] = 3 * c;
      ditherc    = cerror0[1] + 5 * c;
    };


    /*****************************************************************
     * Magenta
     *****************************************************************/
    om = m;
    m += (ditherm) / 8;
#ifdef PRINT_DEBUG
    odm = ditherm;
    dm = m;
#endif

    if ((om <= (65536 * I_RATIO_M1 * 2 / 3)))
      {
	if (m > (32767 + (((long long) ditherbit2 / 4) - 8192)) * I_RATIO_M1)
	  {
#ifdef PRINT_DEBUG
	    fprintf(dbg, "Case 1: om %lld m %lld test %lld\n", om, m,
		    (32767 + (((long long) ditherbit2 / 4) - 8192)) * I_RATIO_M1);
#endif
	    *lmptr |= bit;
	    m -= 65535 * I_RATIO_M1;
	  }
      }
    else if (m > ((32767 + (((long long) ditherbit2 / 4) - 8192)) * om / 65536))
      {
	if (ditherbit1 >
	    ((om - (65536 * I_RATIO_M1 * 2 / 3)) * 65536 /
	     (65536 - (65536 * I_RATIO_M1 * 2 / 3))))
	  {
#ifdef PRINT_DEBUG
	    fprintf(dbg, "Case 2: om %lld m %lld ditherbit1 %d ditherbit2 %d num %lld den %lld test1 %lld test2 %lld\n",
		    om, m, ditherbit1, ditherbit2,
		    om, 65536ll,
		    ((32767 + (((long long) ditherbit2 / 4) - 8192)) * om / 65536),
		    ((om - (65536 * I_RATIO_M1 * 2 / 3)) * 65536 /
		     (65536 - (65536 * I_RATIO_M1 * 2 / 3))));
#endif
	    *lmptr |= bit;
	    m -= 65535 * I_RATIO_M1;
	  }
	else
	  {
#ifdef PRINT_DEBUG
	    fprintf(dbg, "Case 3: om %lld m %lld ditherbit1 %d ditherbit2 %d num %lld den %lld test1 %lld test2 %lld\n",
		    om, m, ditherbit1, ditherbit2,
		    om, 65536ll,
		    ((32767 + (((long long) ditherbit2 / 4) - 8192)) * om / 65536),
		    ((om - (65536 * I_RATIO_M1 * 2 / 3)) * 65536 /
		     (65536 - (65536 * I_RATIO_M1 * 2 / 3))));
#endif
	    *mptr |= bit;
	    m -= 65535;
	  }
      }

    if (ditherbit1 & bit)
    {
      merror1[0] = 5 * m;
      ditherm    = merror0[1] + 3 * m;
    }
    else
    {
      merror1[0] = 3 * m;
      ditherm    = merror0[1] + 5 * m;
    };


    oy = y;
    y += (dithery) / 8;
#ifdef PRINT_DEBUG
    ody = dithery;
    dy = y;
#endif


    /*****************************************************************
     * Yellow
     *****************************************************************/
    if (y > (32767 + ((ditherbit3 / 2) - 16384)))
    {
      *yptr |= bit;
      y -= 65535;
    };

    if (ditherbit3 & bit)
    {
      yerror1[0] = 5 * y;
      dithery    = yerror0[1] + 3 * y;
    }
    else
    {
      yerror1[0] = 3 * y;
      dithery    = yerror0[1] + 5 * y;
    };


    /*****************************************************************
     * Advance the loop
     *****************************************************************/
#ifdef PRINT_DEBUG
    fprintf(dbg, "   x %d y %d  r %d g %d b %d  xc %lld xm %lld xy %lld yc %lld ym %lld yy %lld xk %lld  diff %lld divk %lld  oc %lld om %lld oy %lld ok %lld  c %lld m %lld y %lld k %lld  %c%c%c%c%c%c  dk %lld dc %lld dm %lld dy %lld  kd %d ck %d bk %d nk %d ub %d lb %d\n",
	    x, row,
	    rgb[0], rgb[1], rgb[2],
	    xc, xm, xy, yc, ym, yy, xk, diff, divk,
	    oc, om, oy, ok,
	    dc, dm, dy, dk,
	    (*cptr & bit) ? 'c' : ' ',
	    (*lcptr & bit) ? 'C' : ' ',
	    (*mptr & bit) ? 'm' : ' ',
	    (*lmptr & bit) ? 'M' : ' ',
	    (*yptr & bit) ? 'y' : ' ',
	    (*kptr & bit) ? 'k' : ' ',
	    odk, odc, odm, ody,
	    kdarkness, ck, bk, nk, ub, lb);
#endif
	    
    if (bit == 1)
      {
	cptr ++;
	mptr ++;
	lcptr ++;
	lmptr ++;
	yptr ++;
	ditherbit = rand();
	ditherbit0 = ditherbit & 0xffff;
	ditherbit1 = ((ditherbit >> 8) & 0xffff);
	ditherbit2 = ((ditherbit >> 16) & 0x7fff) + ((ditherbit & 0x100) << 7);
	ditherbit3 = ((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
	  ((ditherbit >> 8) & 0xff00);
	bit       = 128;
      }
    else
      {
	/*
	 * Shuffle the dither information around a bit
	 * This should be faster than regenerating the random number from
	 * scratch
	 */
	int dithertmp0 = (ditherbit1 >> 14) ^ ((ditherbit3 &0x3fff) << 2);
	int dithertmp1 = (ditherbit2 >> 14) ^ ((ditherbit2 &0x3fff) << 2);
	int dithertmp2 = (ditherbit3 >> 14) ^ ((ditherbit1 &0x3fff) << 2);
	int dithertmp3 = (ditherbit0 >> 14) ^ ((ditherbit0 &0x3fff) << 2);
	ditherbit0 = dithertmp0;
	ditherbit1 = dithertmp1;
	ditherbit2 = dithertmp2;
	ditherbit3 = dithertmp3;
	bit >>= 1;
      }

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= dst_width)
    {
      xerror -= dst_width;
      rgb    += 3;
    };
  };
  /*
   * Main loop ends here!
   */
#ifdef PRINT_DEBUG
  fprintf(dbg, "\n");
  fclose(dbg);
#endif
}


/*
 * 'gray_to_gray()' - Convert grayscale image data to grayscale (brightness
 *                    adjusted).
 */

void
gray_to_gray(guchar *grayin,	/* I - RGB pixels */
             guchar *grayout,	/* O - RGB pixels */
             int    width,	/* I - Width of row */
             int    bpp,	/* I - Bytes-per-pixel in grayin */
             lut_t  *lut,	/* I - Brightness lookup table */
             guchar *cmap)	/* I - Colormap (unused) */
{
  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *grayout = lut->composite[*grayin];

      grayin ++;
      grayout ++;
      width --;
    };
  }
  else
  {
   /*
    * Handle alpha in image...
    */

    while (width > 0)
    {
      *grayout = lut->composite[grayin[0] * grayin[1] / 255] + 255 - grayin[1];

      grayin += bpp;
      grayout ++;
      width --;
    };
  };
}


/*
 * 'indexed_to_gray()' - Convert indexed image data to grayscale.
 */

void
indexed_to_gray(guchar *indexed,	/* I - Indexed pixels */
                guchar *gray,		/* O - Grayscale pixels */
        	int    width,		/* I - Width of row */
        	int    bpp,		/* I - Bytes-per-pixel in indexed */
                lut_t  *lut,		/* I - Brightness lookup table */
                guchar *cmap)		/* I - Colormap */
{
  int		i;			/* Looping var */
  unsigned char	gray_cmap[256];		/* Grayscale colormap */


  for (i = 0; i < 256; i ++, cmap += 3)
    gray_cmap[i] = (cmap[0] * LUM_RED + cmap[1] * LUM_GREEN + cmap[2] * LUM_BLUE) / 100;

  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[gray_cmap[*indexed]];
      indexed ++;
      gray ++;
      width --;
    };
  }
  else
  {
   /*
    * Handle alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[gray_cmap[indexed[0] * indexed[1] / 255] + 255 - indexed[1]];
      indexed += bpp;
      gray ++;
      width --;
    };
  };
}


/*
 * 'indexed_to_rgb()' - Convert indexed image data to RGB.
 */

void
indexed_to_rgb(guchar *indexed,		/* I - Indexed pixels */
               guchar *rgb,		/* O - RGB pixels */
               int    width,		/* I - Width of row */
               int    bpp,		/* I - Bytes-per-pixel in indexed */
               lut_t  *lut,		/* I - Brightness lookup table */
               guchar *cmap)		/* I - Colormap */
{
  if (bpp == 1)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      rgb[0] = lut->red[cmap[*indexed * 3 + 0]];
      rgb[1] = lut->green[cmap[*indexed * 3 + 1]];
      rgb[2] = lut->blue[cmap[*indexed * 3 + 2]];
      rgb += 3;
      indexed ++;
      width --;
    };
  }
  else
  {
   /*
    * RGBA image...
    */

    while (width > 0)
    {
      rgb[0] = lut->red[cmap[indexed[0] * 3 + 0] * indexed[1] / 255 + 255 - indexed[1]];
      rgb[1] = lut->green[cmap[indexed[0] * 3 + 1] * indexed[1] / 255 + 255 - indexed[1]];
      rgb[2] = lut->blue[cmap[indexed[0] * 3 + 2] * indexed[1] / 255 + 255 - indexed[1]];
      rgb += 3;
      indexed += bpp;
      width --;
    };
  };
}


/*
 * 'rgb_to_gray()' - Convert RGB image data to grayscale.
 */

void
rgb_to_gray(guchar *rgb,		/* I - RGB pixels */
            guchar *gray,		/* O - Grayscale pixels */
            int    width,		/* I - Width of row */
            int    bpp,			/* I - Bytes-per-pixel in RGB */
            lut_t  *lut,		/* I - Brightness lookup table */
            guchar *cmap)		/* I - Colormap (unused) */
{
  if (bpp == 3)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      *gray = lut->composite[(rgb[0] * LUM_RED +
			      rgb[1] * LUM_GREEN +
			      rgb[2] * LUM_BLUE) / 100];
      gray ++;
      rgb += 3;
      width --;
    };
  }
  else
  {
   /*
    * Image has alpha channel...
    */

    while (width > 0)
    {
      *gray = lut->composite[(rgb[0] * LUM_RED +
			      rgb[1] * LUM_GREEN +
			      rgb[2] * LUM_BLUE) *
			    rgb[3] / 25500 + 255 - rgb[3]];
      gray ++;
      rgb += bpp;
      width --;
    };
  };
}


/*
 * 'rgb_to_rgb()' - Convert rgb image data to RGB.
 */

void
rgb_to_rgb(guchar *rgbin,		/* I - RGB pixels */
           guchar *rgbout,		/* O - RGB pixels */
           int    width,		/* I - Width of row */
           int    bpp,			/* I - Bytes-per-pixel in indexed */
           lut_t  *lut,			/* I - Brightness lookup table */
           guchar *cmap)		/* I - Colormap */
{
  if (bpp == 3)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      rgbout[0] = lut->red[rgbin[0]];
      rgbout[1] = lut->green[rgbin[1]];
      rgbout[2] = lut->blue[rgbin[2]];
      rgbin += 3;
      rgbout += 3;
      width --;
    };
  }
  else
  {
   /*
    * RGBA image...
    */

    while (width > 0)
    {
      rgbout[0] = lut->red[rgbin[0] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbout[1] = lut->green[rgbin[1] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbout[2] = lut->blue[rgbin[2] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbin += bpp;
      rgbout += 3;
      width --;
    };
  };
}


/*
 * 'rgb_to_rgb16()' - Convert rgb image data to RGB.
 */

void
rgb_to_rgb16(guchar *rgbin,		/* I - RGB pixels */
	     gushort *rgbout,		/* O - RGB pixels */
	     int    width,		/* I - Width of row */
	     int    bpp,		/* I - Bytes-per-pixel in indexed */
	     lut16_t *lut,		/* I - Brightness lookup table */
	     guchar *cmap)		/* I - Colormap */
{
  if (bpp == 3)
  {
   /*
    * No alpha in image...
    */

    while (width > 0)
    {
      rgbout[0] = lut->red[rgbin[0]];
      rgbout[1] = lut->green[rgbin[1]];
      rgbout[2] = lut->blue[rgbin[2]];
      rgbin += 3;
      rgbout += 3;
      width --;
    };
  }
  else
  {
   /*
    * RGBA image...
    */

    while (width > 0)
    {
      rgbout[0] = lut->red[rgbin[0] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbout[1] = lut->green[rgbin[1] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbout[2] = lut->blue[rgbin[2] * rgbin[3] / 255 + 255 - rgbin[3]];
      rgbin += bpp;
      rgbout += 3;
      width --;
    };
  };
}


/*
 * 'default_media_size()' - Return the size of a default page size.
 */

void
default_media_size(int  model,		/* I - Printer model */
        	   char *ppd_file,	/* I - PPD file (not used) */
        	   char *media_size,	/* I - Media size */
        	   int  *width,		/* O - Width in points */
        	   int  *length)	/* O - Length in points */
{
  if (strcmp(media_size, "Letter") == 0)
  {
    *width  = 612;
    *length = 792;
  }
  else if (strcmp(media_size, "Legal") == 0)
  {
    *width  = 612;
    *length = 1008;
  }
  else if (strcmp(media_size, "Tabloid") == 0)
  {
    *width  = 792;
    *length = 1214;
  }
  else if (strcmp(media_size, "12x18") == 0)
  {
    *width  = 864;
    *length = 1296;
  }
  else if (strcmp(media_size, "A4") == 0)
  {
    *width  = 595;
    *length = 842;
  }
  else if (strcmp(media_size, "A3") == 0)
  {
    *width  = 842;
    *length = 1191;
  }
  else
  {
    *width  = 0;
    *length = 0;
  };
}


/*
 * End of "$Id$".
 */
