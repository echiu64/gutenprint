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
 */


#ifndef GIMP_PRINT_INTERNAL_VARS_H
#define GIMP_PRINT_INTERNAL_VARS_H

#ifdef __cplusplus
extern "C" {
#endif


#include <sys/types.h>

typedef struct stp_internal_option
{
  int	cookie;
  char *name;
  size_t length;
  char *data;
  struct stp_internal_option *next;
  struct stp_internal_option *prev;
} stp_internal_option_t;

typedef struct					/* Plug-in variables */
{
  int	cookie;
  const char	*driver,		/* Name of printer "driver" */
	*ppd_file,		/* PPD file */
        *resolution,		/* Resolution */
	*media_size_name,	/* Media size */
	*media_type,		/* Media type */
	*media_source,		/* Media source */
	*ink_type,		/* Ink or cartridge */
	*dither_algorithm;	/* Dithering algorithm */
  int	output_type;		/* Color or grayscale output */
  float	brightness;		/* Output brightness */
  int	left,			/* Offset from left-upper corner, points */
	top,			/* ... */
        width,			/* Width of the image, points */
	height;			/* ... */
  float gamma;                  /* Gamma */
  float contrast,		/* Output Contrast */
	cyan,			/* Output red level */
	magenta,		/* Output green level */
	yellow;			/* Output blue level */
  float	saturation;		/* Output saturation */
  float	density;		/* Maximum output density */
  int	image_type;		/* Image type (line art etc.) */
  float app_gamma;		/* Application gamma */
  int	page_width;		/* Width of page in points */
  int	page_height;		/* Height of page in points */
  int	input_color_model;	/* Color model for this device */
  int	output_color_model;	/* Color model for this device */
  int	page_number;
  stp_job_mode_t job_mode;
  void  *lut;			/* Look-up table */
  void  *driver_data;		/* Private data of the driver */
  void (*outfunc)(void *data, const char *buffer, size_t bytes);
  void *outdata;
  void (*errfunc)(void *data, const char *buffer, size_t bytes);
  void *errdata;
  stp_internal_option_t *options;
  int verified;			/* Ensure that params are OK! */
} stp_internal_vars_t;

extern void stp_set_lut(stp_vars_t v, void * val);
extern void * stp_get_lut(const stp_vars_t v);

extern void     stp_set_driver_data (stp_vars_t vv, void * val);
extern void *   stp_get_driver_data (const stp_vars_t vv);

extern void     stp_set_verified(stp_vars_t vv, int value);
extern int      stp_get_verified(stp_vars_t vv);

extern void     stp_copy_options(stp_vars_t vd, const stp_vars_t vs);

extern const stp_vars_t stp_minimum_settings(void);
extern const stp_vars_t stp_maximum_settings(void);

extern void
stp_describe_internal_parameter(const stp_vars_t v, const char *name,
				stp_parameter_t *description);

extern void
stp_fill_parameter_settings(stp_parameter_t *desc, const char *name);


#endif /* GIMP_PRINT_INTERNAL_VARS_H */
/*
 * End of "$Id$".
 */
