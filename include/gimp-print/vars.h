/*
 * "$id: vars.h,v 1.3.4.4 2004/03/09 03:00:25 rlk Exp $"
 *
 *   libgimpprint stp_vars_t core functions.
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


#ifndef __GIMP_PRINT_VARS_H__
#define __GIMP_PRINT_VARS_H__

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Constants...
 */

/*
 * Opaque representation of printer setttings.
 *
 * The representation of printer settings has changed dramatically from 4.2.
 * All (well most, anyway) settings outside of basics such as the printer
 * model and sizing settings are now typed parameters.
 */
typedef void *stp_vars_t;
typedef const void *stp_const_vars_t;


/*
 * The following types are permitted for a printer setting.  Not all
 * are currently implemented.
 */
typedef enum
{
  STP_PARAMETER_TYPE_STRING_LIST, /* Single string choice from a list */
  STP_PARAMETER_TYPE_INT,	/* Integer */
  STP_PARAMETER_TYPE_BOOLEAN,	/* Boolean */
  STP_PARAMETER_TYPE_DOUBLE,	/* Floating point number */
  STP_PARAMETER_TYPE_CURVE,	/* Curve */
  STP_PARAMETER_TYPE_FILE,	/* Filename (NYI, need to consider security) */
  STP_PARAMETER_TYPE_RAW,	/* Raw, opaque data */
  STP_PARAMETER_TYPE_ARRAY,     /* Array */
  STP_PARAMETER_TYPE_INVALID
} stp_parameter_type_t;

/*
 * What kind of setting this is, for purpose of user interface.
 */
typedef enum
{
  STP_PARAMETER_CLASS_FEATURE,	/* Printer feature */
  STP_PARAMETER_CLASS_OUTPUT,	/* Output control */
  STP_PARAMETER_CLASS_CORE,	/* Core Gimp-Print parameter */
  STP_PARAMETER_CLASS_INVALID
} stp_parameter_class_t;

/*
 * What "level" a setting is at, for UI design.
 */
typedef enum
{
  STP_PARAMETER_LEVEL_BASIC,
  STP_PARAMETER_LEVEL_ADVANCED,
  STP_PARAMETER_LEVEL_ADVANCED1,
  STP_PARAMETER_LEVEL_ADVANCED2,
  STP_PARAMETER_LEVEL_ADVANCED3,
  STP_PARAMETER_LEVEL_ADVANCED4,
  STP_PARAMETER_LEVEL_INTERNAL,	/* Parameters used only within Gimp-Print */
  STP_PARAMETER_LEVEL_EXTERNAL,	/* Parameters used only outside Gimp-Print */
  STP_PARAMETER_LEVEL_INVALID
} stp_parameter_level_t;

/*
 * Whether a parameter is currently active (i. e. whether its value
 * should be used by the driver or not).  All parameters default to being
 * active unless explicitly "turned off".
 */
typedef enum
{
  STP_PARAMETER_INACTIVE,
  STP_PARAMETER_DEFAULTED,
  STP_PARAMETER_ACTIVE
} stp_parameter_activity_t;


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
typedef const void *stp_const_string_list_t;

/*
 * Other parameter types
 */

typedef struct
{
  size_t bytes;
  const void *data;
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
  const char *category;		/* User-visible category name */
  const char *help;		/* Help string */
  stp_parameter_type_t p_type;
  stp_parameter_class_t p_class;
  stp_parameter_level_t p_level;
  unsigned char is_mandatory;
  unsigned char is_active;
  unsigned char channel;
  unsigned char verify_this_parameter;	/* Should the verify system check this? */
  unsigned char read_only;
  union				/* Limits on the values */
  {				/* the parameter may take */
    stp_curve_t curve;
    stp_double_bound_t dbl;
    stp_int_bound_t integer;
    stp_string_list_t str;
    stp_array_t array;
  } bounds;
  union				/* Default value of the parameter */
  {
    stp_curve_t curve;
    double dbl;
    int integer;
    int boolean;
    const char *str;
    stp_array_t array;
  } deflt;
} stp_parameter_t;

typedef void *stp_parameter_list_t;
typedef const void *stp_const_parameter_list_t;


/*
 * Output function supplied by the calling application.
 */
typedef void (*stp_outfunc_t) (void *data, const char *buffer, size_t bytes);


/****************************************************************
*                                                               *
* BASIC PRINTER SETTINGS                                        *
*                                                               *
****************************************************************/

/*
 * Constructors, destructors
 */
extern stp_vars_t stp_vars_create(void);
extern void stp_vars_copy(stp_vars_t vd, stp_const_vars_t vs);
extern stp_vars_t stp_vars_create_copy(stp_const_vars_t vs);
extern void stp_vars_free(stp_vars_t v);

/* 
 * Set/get the name of the printer driver
 */
extern void stp_set_driver(stp_vars_t v, const char *val);
extern void stp_set_driver_n(stp_vars_t v, const char *val, int bytes);
extern const char *stp_get_driver(stp_const_vars_t v);

/*
 * Set/get the name of the color conversion routine, if not default
 */
extern void stp_set_color_conversion(stp_vars_t v, const char *val);
extern void stp_set_color_conversion_n(stp_vars_t v, const char *val, int bytes);
extern const char *stp_get_color_conversion(stp_const_vars_t v);

/*
 * Set/get the position and size of the image
 */
extern void stp_set_left(stp_vars_t v, int val);
extern int stp_get_left(stp_const_vars_t v);

extern void stp_set_top(stp_vars_t v, int val);
extern int stp_get_top(stp_const_vars_t v);

extern void stp_set_width(stp_vars_t v, int val);
extern int stp_get_width(stp_const_vars_t v);

extern void stp_set_height(stp_vars_t v, int val);
extern int stp_get_height(stp_const_vars_t v);

/*
 * For custom page widths, these functions may be used.
 */
extern void stp_set_page_width(stp_vars_t v, int val);
extern int stp_get_page_width(stp_const_vars_t v);

extern void stp_set_page_height(stp_vars_t v, int val);
extern int stp_get_page_height(stp_const_vars_t v);

/*
 * These functions are used to print output and diagnostic information
 * respectively.  These must be supplied by the caller.  Outdata and
 * errdata are passed as arguments to outfunc and errfunc; typically
 * they will be file descriptors.
 */
extern void stp_set_outfunc(stp_vars_t v, stp_outfunc_t val);
extern stp_outfunc_t stp_get_outfunc(stp_const_vars_t v);

extern void stp_set_errfunc(stp_vars_t v, stp_outfunc_t val);
extern stp_outfunc_t stp_get_errfunc(stp_const_vars_t v);

extern void stp_set_outdata(stp_vars_t v, void *val);
extern void *stp_get_outdata(stp_const_vars_t v);

extern void stp_set_errdata(stp_vars_t v, void *val);
extern void *stp_get_errdata(stp_const_vars_t v);

/*
 * Merge defaults for a printer with user-chosen settings.
 * This is likely to go away.
 */
extern void stp_merge_printvars(stp_vars_t user, stp_const_vars_t print);


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

extern stp_parameter_list_t stp_get_parameter_list(stp_const_vars_t v);

extern size_t stp_parameter_list_count(stp_const_parameter_list_t list);

extern const stp_parameter_t *
stp_parameter_find(stp_const_parameter_list_t list, const char *name);

extern const stp_parameter_t *
stp_parameter_list_param(stp_const_parameter_list_t list, size_t item);

extern void stp_parameter_list_free(stp_parameter_list_t list);

extern stp_parameter_list_t stp_parameter_list_create(void);

extern void stp_parameter_list_add_param(stp_parameter_list_t list,
					 const stp_parameter_t *item);

extern stp_parameter_list_t
stp_parameter_list_copy(stp_const_parameter_list_t list);

extern void
stp_parameter_list_append(stp_parameter_list_t list,
			  stp_const_parameter_list_t append);

extern void
stp_describe_parameter(stp_const_vars_t v, const char *name,
		       stp_parameter_t *description);

extern void stp_parameter_description_free(stp_parameter_t *description);

extern const stp_parameter_t *
stp_parameter_find_in_settings(stp_const_vars_t v, const char *name);

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
extern void stp_set_string_parameter_n(stp_vars_t v, const char *parameter,
				       const char *value, size_t bytes);
extern void stp_set_file_parameter(stp_vars_t v, const char *parameter,
				   const char *value);
extern void stp_set_file_parameter_n(stp_vars_t v, const char *parameter,
				     const char *value, size_t bytes);
extern void stp_set_float_parameter(stp_vars_t v, const char *parameter,
				    double value);
extern void stp_set_int_parameter(stp_vars_t v, const char *parameter,
				  int value);
extern void stp_set_boolean_parameter(stp_vars_t v, const char *parameter,
				      int value);
extern void stp_set_curve_parameter(stp_vars_t v, const char *parameter,
				    stp_const_curve_t value);
extern void stp_set_array_parameter(stp_vars_t v, const char *parameter,
				    stp_const_array_t value);
extern void stp_set_raw_parameter(stp_vars_t v, const char *parameter,
				  const void *value, size_t bytes);

extern void stp_scale_float_parameter(stp_vars_t v, const char *param,
				      double scale);

extern void stp_set_default_string_parameter(stp_vars_t v,
					     const char *parameter,
					     const char *value);
extern void stp_set_default_string_parameter_n(stp_vars_t v,
					       const char *parameter,
					       const char *value, size_t bytes);
extern void stp_set_default_file_parameter(stp_vars_t v,
					   const char *parameter,
					   const char *value);
extern void stp_set_default_file_parameter_n(stp_vars_t v,
					     const char *parameter,
					     const char *value, size_t bytes);
extern void stp_set_default_float_parameter(stp_vars_t v,
					    const char *parameter,
					    double value);
extern void stp_set_default_int_parameter(stp_vars_t v,
					  const char *parameter,
					  int value);
extern void stp_set_default_boolean_parameter(stp_vars_t v,
					      const char *parameter,
					      int value);
extern void stp_set_default_curve_parameter(stp_vars_t v,
					    const char *parameter,
					    stp_const_curve_t value);
extern void stp_set_default_array_parameter(stp_vars_t v,
					    const char *parameter,
					    stp_const_array_t value);
extern void stp_set_default_raw_parameter(stp_vars_t v,
					  const char *parameter,
					  const void *value, size_t bytes);

extern const char *stp_get_string_parameter(stp_const_vars_t v,
					    const char *param);
extern const char *stp_get_file_parameter(stp_const_vars_t v,
					  const char *param);
extern double stp_get_float_parameter(stp_const_vars_t v,
					    const char *param);
extern int stp_get_int_parameter(stp_const_vars_t v,
				 const char *param);
extern int stp_get_boolean_parameter(stp_const_vars_t v,
				     const char *param);
extern stp_const_curve_t stp_get_curve_parameter(stp_const_vars_t v,
						 const char *param);
extern stp_const_array_t stp_get_array_parameter(stp_const_vars_t v,
						 const char *param);
extern const stp_raw_t *stp_get_raw_parameter(stp_const_vars_t v,
					      const char *param);

extern void stp_clear_string_parameter(stp_vars_t v, const char *param);
extern void stp_clear_file_parameter(stp_vars_t v, const char *param);
extern void stp_clear_float_parameter(stp_vars_t v, const char *param);
extern void stp_clear_int_parameter(stp_vars_t v, const char *param);
extern void stp_clear_boolean_parameter(stp_vars_t v, const char *param);
extern void stp_clear_curve_parameter(stp_vars_t v, const char *param);
extern void stp_clear_array_parameter(stp_vars_t v, const char *param);
extern void stp_clear_raw_parameter(stp_vars_t v, const char *param);

extern void stp_set_string_parameter_active(stp_const_vars_t v,
					    const char *param,
					    stp_parameter_activity_t active);
extern void stp_set_file_parameter_active(stp_const_vars_t v,
					  const char *param,
					  stp_parameter_activity_t active);
extern void stp_set_float_parameter_active(stp_const_vars_t v,
					 const char *param,
					 stp_parameter_activity_t active);
extern void stp_set_int_parameter_active(stp_const_vars_t v,
					 const char *param,
					 stp_parameter_activity_t active);
extern void stp_set_boolean_parameter_active(stp_const_vars_t v,
					     const char *param,
					     stp_parameter_activity_t active);
extern void stp_set_curve_parameter_active(stp_const_vars_t v,
					   const char *param,
					   stp_parameter_activity_t active);
extern void stp_set_array_parameter_active(stp_const_vars_t v,
					   const char *param,
					   stp_parameter_activity_t active);
extern void stp_set_raw_parameter_active(stp_const_vars_t v,
					 const char *param,
					 stp_parameter_activity_t active);

extern int stp_check_string_parameter(stp_const_vars_t v, const char *param,
				      stp_parameter_activity_t active);
extern int stp_check_file_parameter(stp_const_vars_t v, const char *param,
				    stp_parameter_activity_t active);
extern int stp_check_float_parameter(stp_const_vars_t v, const char *param,
				     stp_parameter_activity_t active);
extern int stp_check_int_parameter(stp_const_vars_t v, const char *param,
				   stp_parameter_activity_t active);
extern int stp_check_boolean_parameter(stp_const_vars_t v, const char *param,
				       stp_parameter_activity_t active);
extern int stp_check_curve_parameter(stp_const_vars_t v, const char *param,
				     stp_parameter_activity_t active);
extern int stp_check_array_parameter(stp_const_vars_t v, const char *param,
				     stp_parameter_activity_t active);
extern int stp_check_raw_parameter(stp_const_vars_t v, const char *param,
				   stp_parameter_activity_t active);

extern stp_parameter_activity_t
stp_get_string_parameter_active(stp_const_vars_t v, const char *param);
extern stp_parameter_activity_t
stp_get_file_parameter_active(stp_const_vars_t v, const char *param);
extern stp_parameter_activity_t
stp_get_float_parameter_active(stp_const_vars_t v, const char *param);
extern stp_parameter_activity_t
stp_get_int_parameter_active(stp_const_vars_t v, const char *param);
extern stp_parameter_activity_t
stp_get_boolean_parameter_active(stp_const_vars_t v, const char *param);
extern stp_parameter_activity_t
stp_get_curve_parameter_active(stp_const_vars_t v, const char *param);
extern stp_parameter_activity_t
stp_get_array_parameter_active(stp_const_vars_t v, const char *param);
extern stp_parameter_activity_t
stp_get_raw_parameter_active(stp_const_vars_t v, const char *param);




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
extern void stp_get_media_size(stp_const_vars_t v, int *width, int *height);

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
extern void stp_get_imageable_area(stp_const_vars_t v, int *left, int *right,
				   int *bottom, int *top);

/*
 * Retrieve the minimum and maximum size limits for custom media sizes
 * with the current printer settings.
 */
extern void
stp_get_size_limit(stp_const_vars_t v, int *max_width, int *max_height,
		   int *min_width, int *min_height);


/*
 * Retrieve the printing resolution of the selected resolution.  If the
 * resolution is invalid, -1 will be returned in both x and y.
 */
extern void stp_describe_resolution(stp_const_vars_t v, int *x, int *y);

/*
 * Verify that the parameters selected are consistent with those allowed
 * by the driver.  This must be called prior to printing; failure to do
 * so will result in printing failing.  Status of 0 represents failure;
 * status of 1 represents success; other status values are reserved.
 */
extern int stp_verify(stp_vars_t v);

/*
 * Default global settings.  The main use of this is to provide a usable
 * stp_vars_t for purposes of parameter inquiry in the absence of a
 * specific printer.  This is currently used in a variety of places
 * to get information on the standard color parameters without querying
 * a particular printer.
 */
extern stp_const_vars_t stp_default_settings(void);


#ifdef __cplusplus
  }
#endif

#endif /* __GIMP_PRINT_VARS_H__ */
/*
 * End of "$Id$".
 */
