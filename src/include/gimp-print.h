/*
 * "$Id$"
 *
 *   Print plug-in header file for the GIMP.
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
 * Revision History:
 *
 *   See ChangeLog
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef _GIMP_PRINT_H_
#define _GIMP_PRINT_H_

/*
 * Include necessary header files...
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
  extern "C" {
#endif

/*
 * Constants...
 */

#define OUTPUT_GRAY		0	/* Grayscale output */
#define OUTPUT_COLOR		1	/* Color output */
#define OUTPUT_GRAY_COLOR	2 	/* Grayscale output using color */

#define ORIENT_AUTO		-1	/* Best orientation */
#define ORIENT_PORTRAIT		0	/* Portrait orientation */
#define ORIENT_LANDSCAPE	1	/* Landscape orientation */
#define ORIENT_UPSIDEDOWN	2	/* Reverse portrait orientation */
#define ORIENT_SEASCAPE		3	/* Reverse landscape orientation */

#define IMAGE_LINE_ART		0
#define IMAGE_SOLID_TONE	1
#define IMAGE_CONTINUOUS	2
#define IMAGE_MONOCHROME	3
#define NIMAGE_TYPES		4

/*
 * Printer driver control structure.  See "print.c" for the actual list...
 */

typedef struct					/* Plug-in variables */
{
  char	output_to[256],		/* Name of file or command to print to */
	driver[64],		/* Name of printer "driver" */
	ppd_file[256];		/* PPD file */
  int	output_type;		/* Color or grayscale output */
  char	resolution[64],		/* Resolution */
	media_size[64],		/* Media size */
	media_type[64],		/* Media type */
	media_source[64],	/* Media source */
	ink_type[64],		/* Ink or cartridge */
	dither_algorithm[64];	/* Dithering algorithm */
  float	brightness;		/* Output brightness */
  float	scaling;		/* Scaling, percent of printable area */
  int	orientation,		/* Orientation - 0 = port., 1 = land.,
				   -1 = auto */
	left,			/* Offset from lower-lefthand corner, points */
	top;			/* ... */
  float gamma;                  /* Gamma */
  float contrast,		/* Output Contrast */
	cyan,			/* Output red level */
	magenta,		/* Output green level */
	yellow;			/* Output blue level */
  int	linear;			/* Linear density (mostly for testing!) */
  float	saturation;		/* Output saturation */
  float	density;		/* Maximum output density */
  int	image_type;		/* Image type (line art etc.) */
  int	unit;			/* Units for preview area 0=Inch 1=Metric */
  float app_gamma;		/* Application gamma */
  int	page_width;		/* Width of page in points */
  int	page_height;		/* Height of page in points */
  void  *lut;			/* Look-up table */
  unsigned char *cmap;		/* Color map */
  void (*outfunc)(void *data, const char *buffer, size_t bytes);
  void *outdata;
  void (*errfunc)(void *data, const char *buffer, size_t bytes);
  void *errdata;
} stp_vars_t;

typedef enum papersize_unit
{
  PAPERSIZE_ENGLISH,
  PAPERSIZE_METRIC
} stp_papersize_unit_t;

typedef struct
{
  char name[32];
  unsigned width;
  unsigned height;
  stp_papersize_unit_t paper_unit;
} stp_papersize_t;

/*
 * Abstract data type for interfacing with the image creation program
 * (in this case, the Gimp).
 */

typedef struct stp_image
{
  void (*init)(struct stp_image *image);
  void (*reset)(struct stp_image *image);
  void (*transpose)(struct stp_image *image);
  void (*hflip)(struct stp_image *image);
  void (*vflip)(struct stp_image *image);
  void (*crop)(struct stp_image *image,
	       int left, int top, int right, int bottom);
  void (*rotate_ccw)(struct stp_image *image);
  void (*rotate_cw)(struct stp_image *image);
  void (*rotate_180)(struct stp_image *image);
  int  (*bpp)(struct stp_image *image);
  int  (*width)(struct stp_image *image);
  int  (*height)(struct stp_image *image);
  void (*get_row)(struct stp_image *image, unsigned char *data, int row);
  const char *(*get_appname)(struct stp_image *image);
  void (*progress_init)(struct stp_image *image);
  void (*note_progress)(struct stp_image *image, double current, double total);
  void (*progress_conclude)(struct stp_image *image);
  void *rep;
} stp_image_t;

/*
 * Definition of a printer.  A printer definition contains some data
 * about the printer and a set of member functions that operate on it.
 *
 * The data members are:
 *
 * long_name is a human-readable name.  It is intended to be used by
 *   a user interface to print the name of the printer.
 *
 * driver is the short name of the printer.  This is an alternate name
 *   that is used internally.  A user interface may use this for input
 *   purposes, or a client program may use this to generate a filename.
 *   The driver name should consist of lowercase alphanumerics and hyphens
 *   only.
 *
 * model is a model number used only by the underlying driver.  It is
 *   treated as an opaque, but static, identifier.  It should not be a
 *   pointer value, but the exact interpretation of the model number
 *   is up to the driver implementation (it may be an index into an
 *   array, for example).
 *
 * printvars is the default settings for this printer.
 *
 * The member functions are:
 *
 * char **(*parameters)(const struct *printer,
 *                      char *ppd_file,
 *                      char *name,
 *                      int *count)
 *
 *   returns a list of option values of the specified parameter NAME
 *   for the specified PRINTER.  If a PPD filename is specified, the driver
 *   may use that to help generate the valid parameter list.  The number
 *   of options returned is placed in COUNT.  Both the array and the
 *   options themselves are allocated on the heap; it is the caller's
 *   responsibility to free them upon completion of use.  The driver
 *   must therefore return a copy of data.
 *
 *   In all cases, the returned option names should be appropriate for a
 *   user interface to display.
 *
 *   The list of parameters is subject to change.  The currently supported
 *   parameters are:
 *
 *     PageSize returns a list of legal page size names for the printer
 *       in question.
 *
 *     Resolution returns a list of valid resolution settings.  The
 *       resolutions are to be interpreted as opaque names; the caller
 *       must not attempt to interpret them except with the
 *       describe_resolution function described below.  There may be
 *       multiple resolution names that resolve to the same printing
 *       resolution; they may correspond to different quality settings,
 *       for example.
 *
 *     InkType returns a list of legal ink types.  The printer driver may
 *       define these as it sees fit.  If a printer offers a choice of
 *       ink cartridges, the choices would be enumerated here.
 *
 *     MediaType returns a list of legal media types.  The printer driver
 *       may define these as it sees fit.  This is normally different kinds
 *       of paper that the printer can handle.
 *
 *     InputSlot returns a list of legal input sources for the printer.
 *       This is typically things like different input trays, manual feed,
 *       roll feed, and the like.
 *
 * void (*media_size)(const struct stp_printer *printer,
 *                    const stp_vars_t *v,
 *                    int *width,
 *                    int *height)
 *
 *   returns the physical WIDTH and HEIGHT of the page using the settings
 *   in V.  The driver will almost always look at the media_size variable
 *   in V; it may look at other data in V to determine the physical page
 *   size.  WIDTH and HEIGHT are expressed in units of 1/72".
 *
 * void (*imageable_area)(const struct stp_printer *printer,
 *                        const stp_vars_t *v,
 *                        int *left,
 *                        int *right,
 *                        int *bottom,
 *                        int *top)
 *
 *   returns the width of the LEFT, RIGHT, BOTTOM, and TOP border of the
 *   page for the given printer and variable settings.  The caller can
 *   use this, in combination with the media_size member, to determine
 *   the printable region of the page, and if needed, exactly where to
 *   place the image to achieve a given physical placement (e. g.
 *   centering) on the page.  All returned values are in units of
 *   1/72".
 *
 * void (*limit)(const struct stp_printer *printer,
 *               const stp_vars_t *v,
 *               int *width,
 *               int *height)
 *
 *   returns the maximum page size the printer can handle, in units of
 *   1/72".
 *
 * void (*print)(const struct stp_printer *printer,
 *               stp_image_t *image,
 *               const stp_vars_t *v)
 *
 *   prints a page.  The variable settings provided in V are used to control
 *   the printing; PRN is a file pointer that the raw printer output
 *   is to be written to, and IMAGE is an object that sources the input
 *   data to the driver (the contents of which are opaque to the low level
 *   driver and are interpreted by the high level program).
 *
 * const char *(*default_resolution)(const struct stp_printer *printer)
 *
 *   returns the name of the default resolution for the printer.  The
 *   caller must not attempt to free the returned value.
 *
 * void (*describe_resolution)(const struct stp_printer *printer,
 *                             const char *resolution,
 *                             int *x,
 *                             int *y)
 *
 *   returns the horizontal (X) and vertical (Y) resolution of the chosen
 *   RESOLUTION name.  The high level program may choose to use this to
 *   rasterize at an appropriate resolution.
 *   
 */

struct stp_printer;

typedef struct
{
  char	**(*parameters)(const struct stp_printer *printer,
			const char *ppd_file,
                        const char *name, int *count);
  void	(*media_size)(const struct stp_printer *printer, const stp_vars_t *v,
		      int *width, int *height);
  void	(*imageable_area)(const struct stp_printer *printer,
			  const stp_vars_t *v,
                          int *left, int *right, int *bottom, int *top);
  void	(*limit)(const struct stp_printer *printer, const stp_vars_t *v,
		 int *width, int *height);
  void	(*print)(const struct stp_printer *printer,
		 stp_image_t *image, const stp_vars_t *v);
  const char *(*default_resolution)(const struct stp_printer *printer);
  void  (*describe_resolution)(const struct stp_printer *printer,
			       const char *resolution, int *x, int *y);
} stp_printfuncs_t;

typedef struct stp_printer
{
  const char	*long_name,			/* Long name for UI */
	*driver;			/* Short name for printrc file */
  int	model;				/* Model number */
  const stp_printfuncs_t *printfuncs;
  stp_vars_t printvars;
} stp_printer_t;

/*
 * hue_map is an array of 49 doubles representing the mapping of hue
 * from (0..6) to (0..6) in increments of .125.  The hue_map is in CMY space,
 * so hue=0 is cyan.
 */
typedef void (*stp_convert_t)(unsigned char *in, unsigned short *out,
			      int width, int bpp, unsigned char *cmap,
			      const stp_vars_t *vars, const double *hue_map,
			      const double *lum_map, const double *sat_map);

extern void	stp_merge_printvars(stp_vars_t *user, const stp_vars_t *print);
extern void	stp_free_lut(stp_vars_t *v);
extern void	stp_compute_lut(size_t steps, stp_vars_t *v);


extern void	stp_default_media_size(const stp_printer_t *printer,
				       const stp_vars_t *v, int *width,
				       int *height);

extern size_t	  stp_dither_algorithm_count(void);
extern const char *stp_dither_algorithm_name(int);
extern const char *stp_default_dither_algorithm(void);

extern int	      		stp_known_papersizes(void);
extern const stp_papersize_t	*stp_get_papersizes(void);
extern const stp_papersize_t	*stp_get_papersize_by_name(const char *);
extern const stp_papersize_t 	*stp_get_papersize_by_size(int l, int w);

extern int			stp_known_printers(void);
extern const stp_printer_t	*stp_get_printers(void);
extern const stp_printer_t	*stp_get_printer_by_index(int);
extern const stp_printer_t	*stp_get_printer_by_long_name(const char *);
extern const stp_printer_t	*stp_get_printer_by_driver(const char *);
extern int			stp_get_printer_index_by_driver(const char *);

extern stp_convert_t 	stp_choose_colorfunc(int, int,
					     const unsigned char *, int *,
					     const stp_vars_t *);
extern void
stp_compute_page_parameters(int page_right, int page_left, int page_top,
			    int page_bottom, double scaling, int image_width,
			    int image_height, stp_image_t *image, int *orientation,
			    int *page_width, int *page_height, int *out_width,
			    int *out_height, int *left, int *top);

extern int
stp_verify_printer_params(const stp_printer_t *, const stp_vars_t *);
extern const stp_vars_t *stp_default_settings(void);
extern const stp_vars_t *stp_maximum_settings(void);
extern const stp_vars_t *stp_minimum_settings(void);

#ifdef __cplusplus
  }
#endif

#endif /* _GIMP_PRINT_H_ */
/*
 * End of "$Id$".
 */
