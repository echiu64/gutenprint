/*		-*- Mode: C -*-
 *  $Id$
 *
 *   Gimp-Print header file
 *
 *   Copyright 1997-2002 Michael Sweet (mike@easysw.com) and
 *      Robert Krawitz (rlk@alum.mit.edu)
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

#ifndef __GIMP_PRINT_H__
#define __GIMP_PRINT_H__

/*
 * Include necessary header files...
 */

#include <stddef.h>     /* For size_t */
#include <stdio.h>		/* For FILE */

/*
 * Autogen-time versioning
 */
#include <gimp-print/gimp-print-version.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Library versioning details
 * Version information is included from gimp-print-version.h
 */

#define GIMPPRINT_CHECK_VERSION(major,minor,micro)	\
  (GIMPPRINT_MAJOR_VERSION >  (major) ||		\
  (GIMPPRINT_MAJOR_VERSION == (major) &&		\
   GIMPPRINT_MINOR_VERSION > (minor)) ||		\
  (GIMPPRINT_MAJOR_VERSION == (major) &&		\
   GIMPPRINT_MINOR_VERSION == (minor) &&		\
   GIMPPRINT_MICRO_VERSION >= (micro)))

extern const unsigned int gimpprint_major_version;
extern const unsigned int gimpprint_minor_version;
extern const unsigned int gimpprint_micro_version;
extern const unsigned int gimpprint_current_interface;
extern const unsigned int gimpprint_binary_age;
extern const unsigned int gimpprint_interface_age;
extern const char *stp_check_version(unsigned int required_major,
				     unsigned int required_minor,
				     unsigned int required_micro);

/*
 * Constants...
 */

#define OUTPUT_GRAY             0       /* Grayscale output */
#define OUTPUT_COLOR            1       /* Color output */
#define OUTPUT_RAW_CMYK         2       /* Raw CMYK output */
#define OUTPUT_RAW_PRINTER	3	/* Printer-specific raw output */

#define COLOR_MODEL_RGB         0
#define COLOR_MODEL_CMY         1
#define NCOLOR_MODELS           2

/*
 * Abstract data type for interfacing with the image creation program
 * (in this case, the Gimp).
 *
 * The progress_init(), note_progress(), and progress_conclude() members
 * are used to enable the image layer to deliver notification of
 * progress to the user.  It is likely that these functions will be
 * dropped in the future, and if desired must be implemented in
 * get_row().
 *
 * get_appname() should return the name of the application.  This is
 * embedded in the output by some drivers.
 *
 * width() and height() return the dimensions of the image in pixels.
 *
 * bpp(), or bytes per pixel, is used in combination with the output type
 * and presence of a color map, if supplied, to determine the format
 * of the input:
 *
 * Output_type is OUTPUT_COLOR, or OUTPUT_GRAY:
 *
 *    bpp           No color map                Color map present
 *     1            grayscale                   indexed color (256 colors)
 *     2            grayscale w/alpha           indexed color w/alpha
 *     3            RGB                         N/A
 *     4            N/A                         RGB w/alpha (RGBA)
 *
 * Output_type is OUTPUT_CMYK:
 *
 *    bpp           No color map                Color map present
 *     4            8 bits/plane CMYK           N/A
 *     8            16 bits/plane CMYK          N/A
 *
 * Output type is OUTPUT_RAW_PRINTER:
 *
 *    If the printer supports OUTPUT_RAW_PRINTER, the bpp value should be
 *    2 x the number of channels desired to print (the precise modes
 *    available are printer-dependent).  Each plane is therefore 2 bytes
 *    (16 bits) deep.
 *
 * init() is used to perform any initialization required by the image
 * layer for the image.  It will be called once per image.  reset() is
 * called to reset the image to the beginning.  It may (in principle)
 * be called multiple times if a page is being printed more than once.
 * The reset() call may be removed in the future.
 *
 * get_row() transfers the data from the image to the gimp-print
 * library.  It is called from the driver layer.  It should copy WIDTH
 * (as returned by the width() member) pixels of data into the data
 * buffer.  It normally returns STP_IMAGE_STATUS_OK; if something goes wrong,
 * or the application wishes to stop producing any further output
 * (e. g. because the user cancelled the print job), it should return
 * STP_IMAGE_STATUS_ABORT.  This will cause the driver to flush any remaining
 * data to the output.  It will always request rows in monotonically
 * ascending order, but it may skip rows (if, for example, the
 * resolution of the input is higher than the resolution of the
 * output).
 */

typedef enum
{
  STP_JOB_MODE_PAGE,
  STP_JOB_MODE_JOB
} stp_job_mode_t;

typedef enum stp_image_status
{
  STP_IMAGE_STATUS_OK,
  STP_IMAGE_STATUS_ABORT
} stp_image_status_t;

typedef struct stp_image
{
  void (*init)(struct stp_image *image);
  void (*reset)(struct stp_image *image);
  int  (*bpp)(struct stp_image *image);
  int  (*width)(struct stp_image *image);
  int  (*height)(struct stp_image *image);
  stp_image_status_t (*get_row)(struct stp_image *image, unsigned char *data,
                                size_t byte_limit, int row);
  const char *(*get_appname)(struct stp_image *image);
  void (*progress_init)(struct stp_image *image);
  void (*note_progress)(struct stp_image *image, double current, double total);
  void (*progress_conclude)(struct stp_image *image);
  void *rep;
  /* Optional extensions for user interface below */
  void (*transpose)(struct stp_image *image);
  void (*hflip)(struct stp_image *image);
  void (*vflip)(struct stp_image *image);
  void (*rotate_ccw)(struct stp_image *image);
  void (*rotate_cw)(struct stp_image *image);
  void (*rotate_180)(struct stp_image *image);
  void (*crop)(struct stp_image *image, int left, int top,
	       int right, int bottom);
} stp_image_t;


/*
 * Basic data types for Gimp-Print
 */

/*
 * Opaque representation of a printer model
 */
typedef void *stp_printer_t;

/*
 * Opaque representation of printer setttings.
 *
 * The representation of printer settings has changed dramatically from 4.2.
 * All (well most, anyway) settings outside of basics such as the printer
 * model and sizing settings are now typed parameters.
 */
typedef void *stp_vars_t;

/*
 * The following types are permitted for a printer setting.  Not all
 * are currently implemented.
 */
typedef enum stp_parameter_type
{
  STP_PARAMETER_TYPE_STRING_LIST, /* Single string choice from a list */
  STP_PARAMETER_TYPE_INT,	/* Integer */
  STP_PARAMETER_TYPE_BOOLEAN,	/* Boolean */
  STP_PARAMETER_TYPE_DOUBLE,	/* Floating point number */
  STP_PARAMETER_TYPE_CURVE,	/* Curve */
  STP_PARAMETER_TYPE_FILE,	/* Filename (NYI, need to consider security) */
  STP_PARAMETER_TYPE_RAW,	/* Raw, opaque data */
  STP_PARAMETER_TYPE_INVALID
} stp_parameter_type_t;

/*
 * What kind of setting this is, for purpose of user interface.
 */
typedef enum stp_parameter_class
{
  STP_PARAMETER_CLASS_FEATURE,	/* Printer feature */
  STP_PARAMETER_CLASS_OUTPUT,	/* Output control */
  STP_PARAMETER_CLASS_PAGE_SIZE, /* Special for specifying page size */
  STP_PARAMETER_CLASS_INVALID
} stp_parameter_class_t;

/*
 * What "level" a setting is at, for UI design.
 */
typedef enum stp_parameter_level
{
  STP_PARAMETER_LEVEL_BASIC,
  STP_PARAMETER_LEVEL_ADVANCED,
  STP_PARAMETER_LEVEL_ADVANCED1,
  STP_PARAMETER_LEVEL_ADVANCED2,
  STP_PARAMETER_LEVEL_ADVANCED3,
  STP_PARAMETER_LEVEL_ADVANCED4,
  STP_PARAMETER_LEVEL_ADVANCED5,
  STP_PARAMETER_LEVEL_INVALID
} stp_parameter_level_t;

/*
 * Whether a parameter is currently active (i. e. whether its value
 * should be used by the driver or not).  All parameters default to being
 * active unless explicitly "turned off".
 */
typedef enum stp_parameter_activity
{
  STP_PARAMETER_INACTIVE,
  STP_PARAMETER_DEFAULTED,
  STP_PARAMETER_ACTIVE
} stp_parameter_activity_t;

/*
 * Curve code borrowed from GTK+, http://www.gtk.org/
 */
typedef enum
{
  STP_CURVE_TYPE_LINEAR,       /* linear interpolation */
  STP_CURVE_TYPE_SPLINE        /* spline interpolation */
} stp_curve_type_t;

typedef enum
{
  STP_CURVE_WRAP_NONE,		/* curve doesn't wrap */
  STP_CURVE_WRAP_AROUND		/* curve wraps to its starting point */
} stp_curve_wrap_mode_t;

typedef enum
{
  STP_CURVE_COMPOSE_ADD,
  STP_CURVE_COMPOSE_MULTIPLY,
  STP_CURVE_COMPOSE_EXPONENTIATE
} stp_curve_compose_t;

typedef enum
{
  STP_CURVE_BOUNDS_RESCALE,	/* Rescale the bounds */
  STP_CURVE_BOUNDS_CLIP,	/* Clip the curve to the existing bounds */
  STP_CURVE_BOUNDS_ERROR	/* Error if bounds are violated */
} stp_curve_bounds_t;

/*
 * Representation of a choice list of strings.  The choices themselves
 * consist of a key and a human-readable name.  The list object is
 * opaque.
 */
typedef struct
{
  const char	*name,	/* Option name */
		*text;	/* Human-readable (translated) text */
} stp_param_string_t;

typedef void *stp_string_list_t;

/*
 * Other parameter types
 */

typedef void *stp_curve_t;

typedef struct
{
  size_t bytes;
  void *data;
} stp_raw_t;

typedef struct
{
  double lower;
  double upper;
} stp_double_bound_t;

typedef struct
{
  int lower;
  int upper;
} stp_int_bound_t;

/*
 * Description of a parameter
 */
typedef struct
{
  const char *name;		/* Internal name (key) */
  const char *text;		/* User-visible name */
  const char *help;		/* Help string */
  stp_parameter_type_t p_type;
  stp_parameter_class_t p_class;
  stp_parameter_level_t p_level;
  int is_mandatory;
  int is_active;
  int channel;
  union				/* Limits on the values */
  {				/* the parameter may take */
    stp_curve_t curve;
    stp_double_bound_t dbl;
    stp_int_bound_t integer;
    stp_string_list_t  str;
  } bounds;
  union				/* Default value of the parameter */
  {
    stp_curve_t curve;
    double dbl;
    int integer;
    int boolean;
    const char *str;
  } deflt;
} stp_parameter_t;

typedef void *stp_parameter_list_t;

/*
 * Paper size
 */
typedef enum stp_papersize_unit
{
  PAPERSIZE_ENGLISH,
  PAPERSIZE_METRIC
} stp_papersize_unit_t;

typedef struct
{
  char *name;
  char *text;
  char *comment;
  unsigned width;
  unsigned height;
  unsigned top;
  unsigned left;
  unsigned bottom;
  unsigned right;
  stp_papersize_unit_t paper_unit;
} stp_papersize_t;

/*
 * Output function supplied by the calling application.
 */
typedef void (*stp_outfunc_t) (void *data, const char *buffer, size_t bytes);


/****************************************************************
*                                                               *
* PRINTER DESCRIPTION                                           *
*                                                               *
****************************************************************/

extern int stp_printer_model_count(void);
extern const stp_printer_t stp_get_printer_by_index(int idx);
extern const stp_printer_t stp_get_printer_by_long_name(const char *long_name);
extern const stp_printer_t stp_get_printer_by_driver(const char *driver);
extern const stp_printer_t stp_get_printer(const stp_vars_t v);
extern int stp_get_printer_index_by_driver(const char *driver);

extern const char *stp_printer_get_long_name(const stp_printer_t p);
extern const char *stp_printer_get_driver(const stp_printer_t p);
extern const char *stp_printer_get_family(const stp_printer_t p);
extern int stp_printer_get_model(const stp_printer_t p);
extern const stp_vars_t stp_printer_get_defaults(const stp_printer_t p);
extern void stp_set_printer_defaults(stp_vars_t, const stp_printer_t);


/****************************************************************
*                                                               *
* BASIC PRINTER SETTINGS                                        *
*                                                               *
****************************************************************/

extern stp_vars_t stp_vars_create(void);
extern void stp_vars_copy(stp_vars_t vd, const stp_vars_t vs);
extern stp_vars_t stp_vars_create_copy(const stp_vars_t vs);
extern void stp_vars_free(stp_vars_t v);

extern void stp_set_driver(stp_vars_t v, const char *val);
extern void stp_set_driver_n(stp_vars_t v, const char *val, int bytes);
extern const char *stp_get_driver(const stp_vars_t v);

extern void stp_set_left(stp_vars_t v, int val);
extern int stp_get_left(const stp_vars_t v);

extern void stp_set_top(stp_vars_t v, int val);
extern int stp_get_top(const stp_vars_t v);

extern void stp_set_width(stp_vars_t v, int val);
extern int stp_get_width(const stp_vars_t v);

extern void stp_set_height(stp_vars_t v, int val);
extern int stp_get_height(const stp_vars_t v);

extern void stp_set_job_mode(stp_vars_t, stp_job_mode_t);
extern stp_job_mode_t stp_get_job_mode(const stp_vars_t);

extern void stp_set_page_number(stp_vars_t, int);
extern int stp_get_page_number(const stp_vars_t);

/*
 * For custom page widths, these functions may be used.
 */
extern void stp_set_page_width(stp_vars_t v, int val);
extern int stp_get_page_width(const stp_vars_t v);

extern void stp_set_page_height(stp_vars_t v, int val);
extern int stp_get_page_height(const stp_vars_t v);

/*
 * Set output type.  This is likely to change further.
 */
extern void stp_set_output_type(stp_vars_t v, int val);
extern int stp_get_output_type(const stp_vars_t v);

/*
 * Input color model refers to how the data is being sent to the
 * driver library; the default is RGB.  Output color model refers to
 * the characteristics of the device; the default is CMYK.  The output
 * color model is set by the printer driver and cannot be overridden.
 * It is provided to permit applications to generate previews using
 * the color machinery in Gimp-Print.  If this is done, normally
 * the output color model will be RGB.
 *
 * This is subject to change.
 */
extern void stp_set_input_color_model(stp_vars_t v, int val);
extern int stp_get_input_color_model(const stp_vars_t v);

/*
 * These functions are used to print output and diagnostic information
 * respectively.  These must be supplied by the caller.  Outdata and
 * errdata are passed as arguments to outfunc and errfunc; typically
 * they will be file descriptors.
 */
extern void stp_set_outfunc(const stp_vars_t v, stp_outfunc_t val);
extern stp_outfunc_t stp_get_outfunc(const stp_vars_t v);

extern void stp_set_errfunc(const stp_vars_t v, stp_outfunc_t val);
extern stp_outfunc_t stp_get_errfunc(const stp_vars_t v);

extern void stp_set_outdata(stp_vars_t v, void *val);
extern void *stp_get_outdata(const stp_vars_t v);

extern void stp_set_errdata(stp_vars_t v, void *val);
extern void *stp_get_errdata(const stp_vars_t v);

/*
 * Merge defaults for a printer with user-chosen settings.
 * This is likely to go away.
 */
extern void stp_merge_printvars(stp_vars_t user, const stp_vars_t print);


/****************************************************************
*                                                               *
* PARAMETER MANAGEMENT                                          *
*                                                               *
****************************************************************/

/*
 * List the available parameters for the currently chosen settings.
 * This does not fill in the bounds and defaults; it merely provides
 * a list of settings.  To fill in detailed information for a setting,
 * use stp_describe_parameter.
 */

extern stp_parameter_list_t stp_get_parameter_list(const stp_vars_t v);

extern size_t stp_parameter_list_count(const stp_parameter_list_t list);

extern const stp_parameter_t *
stp_parameter_find(const stp_parameter_list_t list, const char *name);

extern const stp_parameter_t *
stp_parameter_list_param(const stp_parameter_list_t list, size_t item);

extern void stp_parameter_list_free(stp_parameter_list_t list);

extern stp_parameter_list_t stp_parameter_list_create(void);

extern void stp_parameter_list_add_param(stp_parameter_list_t list,
					 const stp_parameter_t *item);

extern stp_parameter_list_t
stp_parameter_list_copy(const stp_parameter_list_t list);

extern void
stp_parameter_list_append(stp_parameter_list_t list,
			  const stp_parameter_list_t append);

extern void
stp_describe_parameter(const stp_vars_t v, const char *name,
		       stp_parameter_t *description);

extern void stp_parameter_description_free(stp_parameter_t *description);

extern const stp_parameter_t *
stp_parameter_find_in_settings(const stp_vars_t v, const char *name);

/*
 * Manipulators for different parameter types.
 * The "set" routines set the value of the parameter.
 *
 * The "set_n" variants for string and file take a counted string
 * rather than a null-terminated string.  It is illegal for a NULL
 * to be present inside the string.
 *
 * The "set_default" routines set the value if the parameter is not
 * already set.  This avoids having to check if the parameter is set
 * prior to setting it, if you do not want to override the existing
 * value.
 *
 * The "get" routines return the value of the parameter.
 *
 * The "check" routines check if the parameter is set.
 */

extern void stp_set_string_parameter(stp_vars_t v, const char *parameter,
				     const char *value);
extern void stp_set_default_string_parameter(stp_vars_t v,
					     const char *parameter,
					     const char *value);
extern void stp_set_string_parameter_n(stp_vars_t v, const char *parameter,
				       const char *value, int bytes);
extern void stp_set_default_string_parameter_n(stp_vars_t v,
					       const char *parameter,
					       const char *value, int bytes);
extern const char *stp_get_string_parameter(const stp_vars_t v,
					    const char *param);
extern void stp_clear_string_parameter(const stp_vars_t v, const char *param);
extern void stp_set_string_parameter_active(const stp_vars_t v,
					    const char *param,
					    stp_parameter_activity_t active);
extern stp_parameter_activity_t
stp_get_string_parameter_active(const stp_vars_t v, const char *param);
extern int stp_check_string_parameter(const stp_vars_t v, const char *param,
				      stp_parameter_activity_t active);

extern void stp_set_file_parameter(stp_vars_t v, const char *parameter,
				   const char *value);
extern void stp_set_default_file_parameter(stp_vars_t v,
					   const char *parameter,
					   const char *value);
extern void stp_set_file_parameter_n(stp_vars_t v, const char *parameter,
				     const char *value, int bytes);
extern void stp_set_default_file_parameter_n(stp_vars_t v,
					     const char *parameter,
					     const char *value, int bytes);
extern const char *stp_get_file_parameter(const stp_vars_t v,
					  const char *param);
extern void stp_clear_file_parameter(const stp_vars_t v, const char *param);
extern void stp_set_file_parameter_active(const stp_vars_t v,
					  const char *param,
					  stp_parameter_activity_t active);
extern stp_parameter_activity_t
stp_get_file_parameter_active(const stp_vars_t v, const char *param);
extern int stp_check_file_parameter(const stp_vars_t v, const char *param,
				    stp_parameter_activity_t active);

extern void stp_set_float_parameter(stp_vars_t v, const char *parameter,
				    double value);
extern void stp_set_default_float_parameter(stp_vars_t v,
					    const char *parameter,
					    double value);
extern double stp_get_float_parameter(const stp_vars_t v,
					    const char *param);
extern void stp_clear_float_parameter(const stp_vars_t v, const char *param);
extern stp_parameter_activity_t
stp_get_float_parameter_active(const stp_vars_t v, const char *param);
extern void stp_set_float_parameter_active(const stp_vars_t v,
					 const char *param,
					 stp_parameter_activity_t active);
extern int stp_check_float_parameter(const stp_vars_t v, const char *param,
				     stp_parameter_activity_t active);
extern void stp_scale_float_parameter(const stp_vars_t v, const char *param,
				      double scale);

extern void stp_set_int_parameter(stp_vars_t v, const char *parameter,
				  int value);
extern void stp_set_default_int_parameter(stp_vars_t v,
					  const char *parameter,
					  int value);
extern int stp_get_int_parameter(const stp_vars_t v,
				 const char *param);
extern void stp_clear_int_parameter(const stp_vars_t v, const char *param);
extern void stp_set_int_parameter_active(const stp_vars_t v,
					 const char *param,
					 stp_parameter_activity_t active);
extern stp_parameter_activity_t
stp_get_int_parameter_active(const stp_vars_t v, const char *param);
extern int stp_check_int_parameter(const stp_vars_t v, const char *param,
				   stp_parameter_activity_t active);

extern void stp_set_boolean_parameter(stp_vars_t v, const char *parameter,
				      int value);
extern void stp_set_default_boolean_parameter(stp_vars_t v,
					      const char *parameter,
					      int value);
extern int stp_get_boolean_parameter(const stp_vars_t v,
				     const char *param);
extern void stp_clear_boolean_parameter(const stp_vars_t v, const char *param);
extern void stp_set_boolean_parameter_active(const stp_vars_t v,
					     const char *param,
					     stp_parameter_activity_t active);
extern stp_parameter_activity_t
stp_get_boolean_parameter_active(const stp_vars_t v, const char *param);
extern int stp_check_boolean_parameter(const stp_vars_t v, const char *param,
				       stp_parameter_activity_t active);

extern void stp_set_curve_parameter(stp_vars_t v, const char *parameter,
				    const stp_curve_t value);
extern void stp_set_default_curve_parameter(stp_vars_t v,
					    const char *parameter,
					    const stp_curve_t value);
extern const stp_curve_t stp_get_curve_parameter(const stp_vars_t v,
						 const char *param);
extern void stp_clear_curve_parameter(const stp_vars_t v, const char *param);
extern void stp_set_curve_parameter_active(const stp_vars_t v,
					   const char *param,
					   stp_parameter_activity_t active);
extern stp_parameter_activity_t
stp_get_curve_parameter_active(const stp_vars_t v, const char *param);
extern int stp_check_curve_parameter(const stp_vars_t v, const char *param,
				     stp_parameter_activity_t active);

extern void stp_set_raw_parameter(stp_vars_t v, const char *parameter,
				  const void *value, int bytes);

extern void stp_set_default_raw_parameter(stp_vars_t v,
					  const char *parameter,
					  const void *value, int bytes);
extern const stp_raw_t *stp_get_raw_parameter(const stp_vars_t v,
					      const char *param);
extern void stp_clear_raw_parameter(const stp_vars_t v, const char *param);
extern void stp_set_raw_parameter_active(const stp_vars_t v,
					 const char *param,
					 stp_parameter_activity_t active);
extern stp_parameter_activity_t
stp_get_raw_parameter_active(const stp_vars_t v, const char *param);
extern int stp_check_raw_parameter(const stp_vars_t v, const char *param,
				   stp_parameter_activity_t active);



/****************************************************************
*                                                               *
* LISTS OF STRINGS                                              *
*                                                               *
****************************************************************/

extern stp_string_list_t stp_string_list_create(void);
extern void stp_string_list_free(stp_string_list_t list);

extern stp_param_string_t *
stp_string_list_param(const stp_string_list_t list, size_t element);

extern size_t stp_string_list_count(const stp_string_list_t list);

extern stp_string_list_t
stp_string_list_create_copy(const stp_string_list_t list);

extern void stp_string_list_add_string(stp_string_list_t list,
				       const char *name, const char *text);

extern stp_string_list_t
stp_string_list_create_from_params(const stp_param_string_t *list,
				   size_t count);

extern int
stp_string_list_is_present(stp_string_list_t list, const char *value);


/****************************************************************
*                                                               *
* CURVES                                                        *
*                                                               *
****************************************************************/

/*
 * Create a new curve.  Curves have y=lower..upper.
 * Default bounds are 0..1.  Default interpolation type is
 * linear.  There are no points allocated, and the gamma is defaulted
 * to 1.
 *
 * A wrapped curve has the same value at x=0 and x=1.
 * The wrap mode of a curve cannot be changed except by routines that
 * destroy the old curve entirely (e. g. stp_curve_copy, stp_curve_read).
 */

extern stp_curve_t stp_curve_create(stp_curve_wrap_mode_t wrap);
extern stp_curve_t stp_curve_create_copy(const stp_curve_t curve);
extern void stp_curve_copy(stp_curve_t dest, const stp_curve_t source);
extern void stp_curve_free(stp_curve_t curve);

/*
 * Set the lower and upper bounds on a curve.  If any existing points
 * on the curve are outside of the bounds, FALSE is returned.
 * To change the bounds adjusting data as required, use stp_curve_rescale.
 */
extern int stp_curve_set_bounds(stp_curve_t curve, double low, double high);
extern void stp_curve_get_bounds(const stp_curve_t curve,
				 double *low, double *high);

extern stp_curve_wrap_mode_t stp_curve_get_wrap(const stp_curve_t curve);

/*
 * Get the lowest and highest value of points in the curve.
 * This does not account for any interpolation that may place
 * intermediate points outside of the curve.
 */
extern void stp_curve_get_range(const stp_curve_t curve,
				double *low, double *high);

/*
 * Return the number of allocated points in the curve.
 */
extern size_t stp_curve_count_points(const stp_curve_t curve);

extern int stp_curve_set_interpolation_type(stp_curve_t curve,
					    stp_curve_type_t itype);
extern stp_curve_type_t stp_curve_get_interpolation_type(const stp_curve_t curve);

/*
 * Set all data points of the curve.  If any of the data points fall outside
 * the bounds, the operation is not performed and FALSE is returned.  Count
 * (number of points) must be at least two.
 *
 * The number of points must not exceed 1048576.
 */
extern int stp_curve_set_data(stp_curve_t curve, size_t count,
			      const double *data);
extern int stp_curve_set_float_data(stp_curve_t curve, size_t count,
				    const float *data);
extern int stp_curve_set_long_data(stp_curve_t curve, size_t count,
				   const long *data);
extern int stp_curve_set_ulong_data(stp_curve_t curve, size_t count,
				    const unsigned long *data);
extern int stp_curve_set_int_data(stp_curve_t curve, size_t count,
				    const int *data);
extern int stp_curve_set_uint_data(stp_curve_t curve, size_t count,
				   const unsigned int *data);
extern int stp_curve_set_short_data(stp_curve_t curve, size_t count,
				    const short *data);
extern int stp_curve_set_ushort_data(stp_curve_t curve, size_t count,
				     const unsigned short *data);

/*
 * Return a curve containing a subrange of data from the source curve.
 * If the start or count is invalid, the returned curve will compare equal
 * to NULL (i. e. it will be a null pointer).  start + count must not exceed
 * the number of points in the curve, and count must be at least 2.
 * The return curve is a non-wrapping curve.
 */
extern stp_curve_t stp_curve_get_subrange(const stp_curve_t curve,
					  size_t start, size_t count);

/*
 * Set part of a curve to the range in the second curve.  The data in the
 * range must fit within both the bounds and the number of points in the
 * first curve.
 */
extern int stp_curve_set_subrange(stp_curve_t curve, const stp_curve_t range,
				  size_t start);

/*
 * Return a pointer to the curve's raw data.
 *
 * This data is not guaranteed to be valid beyond the next non-const
 * curve call.
 *
 * If the curve is a pure gamma curve (no associated points),
 * a null pointer is returned and the count is 0.
 */
extern const double *stp_curve_get_data(const stp_curve_t curve, size_t *count);

/*
 * Convert the curve data to another data type.
 *
 * The pointer returned is owned by the curve, and is not guaranteed
 * to be valid beyond the next non-const curve call;
 *
 * If the bounds of the curve exceed the limits of the data type,
 * NULL is returned.
 */
extern const float *stp_curve_get_float_data(const stp_curve_t curve,
					     size_t *count);
extern const long *stp_curve_get_long_data(const stp_curve_t curve,
					   size_t *count);
extern const unsigned long *stp_curve_get_ulong_data(const stp_curve_t curve,
						     size_t *count);
extern const int *stp_curve_get_int_data(const stp_curve_t curve,
					 size_t *count);
extern const unsigned int *stp_curve_get_uint_data(const stp_curve_t curve,
						   size_t *count);
extern const short *stp_curve_get_short_data(const stp_curve_t curve,
					     size_t *count);
extern const unsigned short *stp_curve_get_ushort_data(const stp_curve_t curve,
						       size_t *count);

/*
 * Set the gamma of a curve.  This replaces all existing points along
 * the curve.  The bounds are set to 0..1.  If the gamma value is positive,
 * the function is increasing; if negative, the function is decreasing.
 * Count must be either 0 or at least 2.  If the count is zero, the gamma
 * of the curve is set for interpolation purposes, but points cannot be
 * assigned to.
 *
 * If the gamma value is illegal (0, infinity, or NaN), FALSE is returned.
 * It is also illegal, and FALSE is returned, to set gamma on a wrap-mode
 * curve.
 */
extern int stp_curve_set_gamma(stp_curve_t curve, double gamma);

/*
 * Return the gamma value of the curve.  A returned value of 0 indicates
 * that the curve does not have a valid gamma value.
 */
extern double stp_curve_get_gamma(const stp_curve_t curve);

/*
 * Set a point along the curve.  Return FALSE if data is outside the valid
 * bounds or 'where' is outside the number of valid points.
 *
 * This call destroys any gamma value assigned to the curve.
 */
extern int stp_curve_set_point(stp_curve_t curve, size_t where, double data);

/*
 * Get a point along the curve.  If 'where' is outside of the number of valid
 * points, FALSE is returned and the value of data is not changed.
 */
extern int stp_curve_get_point(const stp_curve_t curve, size_t where,
			       double *data);

/*
 * Interpolate a point along the curve.  If 'where' is less than 0 or
 * greater than the number of points, an error is returned.  If
 * interpolation would produce a value outside of the allowed range
 * (as could happen with spline interpolation), the value is clipped
 * to the range.
 */
extern int stp_curve_interpolate_value(const stp_curve_t curve,
				       double where, double *result);

/*
 * Resample a curve (change the number of points).  Returns FALSE if the
 * number of points is invalid (less than two, except that zero points is
 * permitted for a gamma curve).  This does not destroy the gamma
 * value of a curve.  Points are interpolated as required; any interpolation
 * that would place points outside of the bounds of the curve will be clipped
 * to the bounds.
 *
 * The number of points must not exceed 65536 (65535 for a wrap-mode curve).
 */
extern int stp_curve_resample(stp_curve_t curve, size_t points);

/*
 * Rescale a curve (multiply all points by a scaling constant).  This
 * also rescales the bounds.  Returns false if this would exceed floating
 * point limits.
 *
 * Note that this currently destroys the gamma property of the curve.
 */
extern int stp_curve_rescale(stp_curve_t curve, double scale,
			     stp_curve_compose_t mode,
			     stp_curve_bounds_t bounds_mode);

/*
 * Print and read back the value of a curve.  The printable
 * representation is guaranteed to contain only 7-bit printable ASCII
 * characters, and is null-terminated.  The curve will not contain any
 * space, newline, or comma characters.  Furthermore, a printed curve
 * will be read back correctly in all locales.
 *
 * Return of a NULL pointer indicates an error occurred.  This should
 * never happen unless the curve has been overwritten with invalid
 * data.
 *
 * These calls are not guaranteed to provide more than 6 decimal places
 * of precision or +/-0.5e-6 accuracy, whichever is less.
 *
 * The printable representation is allocated on the heap; the caller must
 * free it.
 *
 * NOTE that these calls are not thread-safe!  These routines may
 * manipulate the locale to achieve a safe representation.
 */
extern char *stp_curve_print_string(const stp_curve_t curve);
extern void stp_curve_print(FILE *f, const stp_curve_t curve);

/*
 * If reading the curve fails, the existing curve will not be changed.
 *
 * stp_curve_read_string returns the number of bytes read.  If that
 * number is -1, the read failed.
 */
extern int stp_curve_read(FILE *f, stp_curve_t curve);
extern stp_curve_t stp_curve_create_read(FILE *f);
extern int stp_curve_read_string(const char *text, stp_curve_t curve);
extern stp_curve_t stp_curve_create_read_string(const char *text);

/*
 * Compose two curves, creating a third curve.  Only add and multiply
 * composition is currently supported.
 *
 * If element-wise composition fails, an error is returned.
 *
 * If the number of points requested in the output is -1, the resulting
 * number of points will be the least common multiplier of the number of
 * points in the input and output curves.
 *
 * The number of points must be at least two, unless the curve is a gamma
 * curve and the operation chosen is multiplication or division.
 *
 * If both curves are gamma curves with the same sign, and the
 * operation is multiplication or division, the returned curve is a
 * gamma curve with the appropriate number of points.
 *
 * Both curves must have the same wraparound type.
 *
 * A new curve is created and returned.
 *
 * The number of points must not exceed 65536 (65535 for a wrap-mode curve);
 * if the number of points requested is -1, the number of points will not
 * be allowed to exceed 65535 or 65536.
 */

extern int stp_curve_compose(stp_curve_t *retval,
			     const stp_curve_t a, const stp_curve_t b,
			     stp_curve_compose_t mode, int points);


/****************************************************************
*                                                               *
* PAPER SIZE MANAGEMENT                                         *
*                                                               *
****************************************************************/

extern int stp_known_papersizes(void);
extern const stp_papersize_t *stp_get_papersize_by_name(const char *name);
extern const stp_papersize_t *stp_get_papersize_by_size(int l, int w);
extern const stp_papersize_t *stp_get_papersize_by_index(int index);

/****************************************************************
*                                                               *
* INFORMATIONAL QUERIES                                         *
*                                                               *
****************************************************************/

/*
 * Retrieve the media size of the media type set in V, expressed in units
 * of 1/72".  If the media size is invalid, width and height will be set
 * to -1.  Values of 0 for width or height indicate that the dimension
 * is variable, so that custom page sizes or roll paper can be used.
 * In this case, the size limit should be used to determine maximum and
 * minimum values permitted.
 */
extern void stp_get_media_size(const stp_vars_t v, int *width, int *height);

/*
 * Retrieve the boundaries of the printable area of the page.  In combination
 * with the media size, this can be used to determine the actual printable
 * region, which callers can use to place the image precisely.  The
 * dimensions are relative to the top left of the physical page.
 *
 * If a customizable page size is used (see stp_printer_get_media_size),
 * the actual desired width and/or height must be filled in using
 * stp_set_page_width and/or stp_set_page_height.  If these are not filled
 * in, the margins will be returned.
 *
 * Returned values may be negative if a printer is capable of full bleed
 * by printing beyond the physical boundaries of the page.
 *
 * If the media size stored in V is invalid, the return value
 * will be indeterminate.  It is up to the user to specify legal values.
 */
extern void stp_get_imageable_area(const stp_vars_t v, int *left, int *right,
				   int *bottom, int *top);

/*
 * Retrieve the minimum and maximum size limits for custom media sizes
 * with the current printer settings.
 */
extern void
stp_get_size_limit(const stp_vars_t v, int *max_width, int *max_height,
		   int *min_width, int *min_height);


/*
 * Retrieve the printing resolution of the selected resolution.  If the
 * resolution is invalid, -1 will be returned in both x and y.
 */
extern void stp_describe_resolution(const stp_vars_t v, int *x, int *y);

/*
 * Verify that the parameters selected are consistent with those allowed
 * by the driver.  This must be called prior to printing; failure to do
 * so will result in printing failing.  Status of 0 represents failure;
 * status of 1 represents success; other status values are reserved.
 */
extern int stp_verify(const stp_vars_t v);

/*
 * Default global settings.  The main use of this is to provide a usable
 * stp_vars_t for purposes of parameter inquiry in the absence of a
 * specific printer.  This is currently used in a variety of places
 * to get information on the standard color parameters without querying
 * a particular printer.
 */
extern const stp_vars_t stp_default_settings(void);


/****************************************************************
*                                                               *
* OPERATIONS                                                    *
*                                                               *
****************************************************************/

/*
 * stp_init() must be called prior to any other use of the library.
 */
extern int stp_init(void);

/*
 * Actually print the image.  Return value of 0 represents failure; status of 1
 * represents success; status of 2 represents abort requested by the driver.
 */
extern int stp_print(const stp_vars_t v, stp_image_t *image);

/*
 * Must be called prior to the first call to stp_print().
 */
extern int stp_start_job(const stp_vars_t, stp_image_t *image);

extern int stp_end_job(const stp_vars_t, stp_image_t *image);

/*
 * Set the encoding that all translated strings are output in.
 */
extern const char *stp_set_output_codeset(const char *codeset);

/*
 * Remove inactive and unclaimed options from the list
 */
extern void stp_prune_inactive_options(stp_vars_t v);

#ifdef __cplusplus
  }
#endif

#endif /* __GIMP_PRINT_H__ */
/*
 * End of $Id$
 */
