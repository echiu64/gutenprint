/*
 * "$Id$"
 *
 *   Print plug-in header file for the GIMP.
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
 * Revision History:
 *
 *   See ChangeLog
 */

/*
 * Include necessary header files...
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>
#include <libgimp/gimp.h>


/*
 * Constants...
 */

#define PLUG_IN_VERSION		"2.0.2 - 16 May 1998"
#define PLUG_IN_NAME		"Print"

#define OUTPUT_GRAY		0	/* Grayscale output */
#define OUTPUT_COLOR		1	/* Color output */

#define ORIENT_AUTO		-1	/* Best orientation */
#define ORIENT_PORTRAIT		0	/* Portrait orientation */
#define ORIENT_LANDSCAPE	1	/* Landscape orientation */

#ifndef MIN
#  define MIN(a,b)		((a) < (b) ? (a) : (b))
#  define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif /* !MIN */


/*
 * Printer driver control structure.  See "print.c" for the actual list...
 */

typedef struct
{
  guchar composite[256];
  guchar red[256];
  guchar green[256];
  guchar blue[256];
} lut_t;

typedef struct
{
  gushort composite[256];
  gushort red[256];
  gushort green[256];
  gushort blue[256];
} lut16_t;

typedef struct
{
  char	*long_name,			/* Long name for UI */
	*short_name;			/* Short name for printrc file */
  int	color,				/* TRUE if supports color */
	model;				/* Model number */
  float	gamma,				/* Gamma correction */
	density;			/* Ink "density" or black level */
  char	**(*parameters)(int model, char *ppd_file, char *name, int *count);
					/* Parameter names */
  void	(*media_size)(int model, char *ppd_file, char *media_size,
                      int *width, int *length);
  void	(*imageable_area)(int model, char *ppd_file, char *media_size,
                          int *left, int *right, int *bottom, int *top);
  void	(*print)(int model, char *ppd_file, char *resolution,
                 char *media_size, char *media_type, char *media_source,
                 int output_type, int orientation, float scaling, int left,
                 int top, int copies, FILE *prn, GDrawable *drawable,
                 lut_t *lut, guchar *cmap, lut16_t *lut16,
		 float saturation);	/* Print function */
} printer_t;

typedef void (*convert_t)(guchar *in, guchar *out, int width, int bpp,
                          lut_t *lut, guchar *cmap, float saturation);

typedef void (*convert16_t)(guchar *in, gushort *out, int width, int bpp,
			    lut16_t *lut, guchar *cmap, float saturation);


/*
 * Prototypes...
 */

extern void	dither_black16(gushort *, int, int, int, unsigned char *);

extern void	dither_cmyk16(gushort *, int, int, int, unsigned char *,
			      unsigned char *, unsigned char *,
			      unsigned char *, unsigned char *,
			      unsigned char *, unsigned char *);

extern void	dither_black4(guchar *, int, int, int, unsigned char *);
extern void	dither_black4_16(gushort *, int, int, int, unsigned char *);

extern void	dither_cmyk4(guchar *, int, int, int, unsigned char *,
		             unsigned char *, unsigned char *,
			     unsigned char *);
extern void	dither_cmyk4_16(gushort *, int, int, int, unsigned char *,
				unsigned char *, unsigned char *,
				unsigned char *);


extern void	gray_to_gray(guchar *, guchar *, int, int, lut_t *,
			     guchar *, float);
extern void	indexed_to_gray(guchar *, guchar *, int, int, lut_t *,
				guchar *, float);

extern void	gray_to_gray16(guchar *, gushort *, int, int, lut16_t *,
			       guchar *, float);
extern void	indexed_to_gray16(guchar *, gushort *, int, int, lut16_t *,
				  guchar *, float);
extern void	indexed_to_rgb16(guchar *, gushort *, int, int, lut16_t *,
				 guchar *, float);
extern void	rgb_to_gray16(guchar *, gushort *, int, int, lut16_t *,
			      guchar *, float);
extern void	rgb_to_rgb16(guchar *, gushort *, int, int, lut16_t *,
			     guchar *, float);


extern void	default_media_size(int model, char *ppd_file, char *media_size,
		                   int *width, int *length);


extern char	**escp2_parameters(int model, char *ppd_file, char *name,
		                   int *count);
extern void	escp2_imageable_area(int model, char *ppd_file,
				     char *media_size, int *left, int *right,
				     int *bottom, int *top);
extern void	escp2_print(int model, char *ppd_file, char *resolution,
		            char *media_size, char *media_type,
			    char *media_source, int output_type,
			    int orientation, float scaling, int left,
			    int top, int copies, FILE *prn,
			    GDrawable *drawable, lut_t *lut, guchar *cmap,
			    lut16_t *lut16, float saturation);

extern char	**pcl_parameters(int model, char *ppd_file, char *name,
		                 int *count);
extern void	pcl_imageable_area(int model, char *ppd_file, char *media_size,
		                   int *left, int *right, int *bottom,
				   int *top);
extern void	pcl_print(int model, char *ppd_file, char *resolution,
		          char *media_size, char *media_type,
			  char *media_source, int output_type,
			  int orientation, float scaling,
		          int left, int top, int copies, FILE *prn,
		          GDrawable *drawable, lut_t *lut, guchar *cmap,
			  lut16_t *lut16, float saturation);

extern char	**ps_parameters(int model, char *ppd_file, char *name,
		                int *count);
extern void	ps_media_size(int model, char *ppd_file, char *media_size,
		              int *width, int *length);
extern void	ps_imageable_area(int model, char *ppd_file, char *media_size,
		                  int *left, int *right, int *bottom,
				  int *top);
extern void	ps_print(int model, char *ppd_file, char *resolution,
		         char *media_size, char *media_type,
			 char *media_source, int output_type,
			 int orientation, float scaling, int left, int top,
			 int copies, FILE *prn, GDrawable *drawable,
			 lut_t *lut, guchar *cmap, lut16_t *lut16,
			 float saturation);

extern void calc_hsv_to_rgb16(gushort *rgb, double h, double s, double v);
extern void calc_rgb16_to_hsv(gushort *rgb, double *hue, double *sat,
			      double *val);
extern void calc_hsv_to_rgb(guchar *rgb, double h, double s, double v);
extern void calc_rgb_to_hsv(guchar *rgb, double *hue, double *sat,
			    double *val);

extern void compute_lut(lut_t *lut, lut16_t *lut16, int icontrast,
			float red, float green, float blue, int ibrightness,
			float print_gamma, float gimp_gamma, float user_gamma,
			int linear, float printer_density, float user_density);

/*
 * End of "$Id$".
 */
