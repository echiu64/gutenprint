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

typedef struct
{
  void  (*parameters)(const stp_vars_t v, const char *name,
		      stp_parameter_t *);
  void  (*media_size)(const stp_vars_t v, int *width, int *height);
  void  (*imageable_area)(const stp_vars_t v,
			  int *left, int *right, int *bottom, int *top);
  void  (*limit)(const stp_vars_t v, int *max_width, int *max_height,
                 int *min_width, int *min_height);
  int   (*print)(const stp_vars_t v, stp_image_t *image);
  void  (*describe_resolution)(const stp_vars_t v, int *x, int *y);
  int   (*verify)(const stp_vars_t v);
  int   (*start_job)(const stp_vars_t v, stp_image_t *image);
  int   (*end_job)(const stp_vars_t v, stp_image_t *image);
} stp_printfuncs_t;

extern int stp_get_model(const stp_vars_t v);

extern const stp_printfuncs_t *stp_printer_get_printfuncs(const stp_printer_t p);

extern int stp_verify_printer_params(const stp_vars_t);
extern int stp_init_printer_list(void);


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_PRINTERS_H */
/*
 * End of "$Id$".
 */
