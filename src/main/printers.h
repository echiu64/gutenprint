/*
 * "$Id$"
 *
 *   libgimpprint header.
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

#ifndef GIMP_PRINT_INTERNAL_PRINTERS_H
#define GIMP_PRINT_INTERNAL_PRINTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "util.h"
#include "vars.h"

typedef struct
{
  stp_parameter_list_t (*list_parameters)(stp_const_vars_t v);
  void  (*parameters)(stp_const_vars_t v, const char *name,
		      stp_parameter_t *);
  void  (*media_size)(stp_const_vars_t v, int *width, int *height);
  void  (*imageable_area)(stp_const_vars_t v,
			  int *left, int *right, int *bottom, int *top);
  void  (*limit)(stp_const_vars_t v, int *max_width, int *max_height,
                 int *min_width, int *min_height);
  int   (*print)(stp_const_vars_t v, stp_image_t *image);
  void  (*describe_resolution)(stp_const_vars_t v, int *x, int *y);
  int   (*verify)(stp_const_vars_t v);
  int   (*start_job)(stp_const_vars_t v, stp_image_t *image);
  int   (*end_job)(stp_const_vars_t v, stp_image_t *image);
} stpi_printfuncs_t;

typedef struct stpi_internal_family
{
  const stpi_printfuncs_t *printfuncs;   /* printfuncs for the printer */
  stpi_list_t             *printer_list; /* list of printers */
} stpi_internal_family_t;

extern int stpi_get_model_id(stp_const_vars_t v);

extern int stpi_verify_printer_params(stp_const_vars_t);

extern int stpi_family_register(stpi_list_t *family);
extern int stpi_family_unregister(stpi_list_t *family);

extern stp_parameter_list_t stpi_printer_list_parameters(stp_const_vars_t v);

extern void
stpi_printer_describe_parameter(stp_const_vars_t v, const char *name,
				stp_parameter_t *description);


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_PRINTERS_H */
/*
 * End of "$Id$".
 */
