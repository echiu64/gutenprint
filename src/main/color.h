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

#ifndef GIMP_PRINT_INTERNAL_COLOR_H
#define GIMP_PRINT_INTERNAL_COLOR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
 * Initialize the color machinery.  Return value is the number
 * of columns of output
 */
extern int stpi_color_init(stp_vars_t v, stp_image_t *image, size_t steps);

/*
 * Acquire input and perform color conversion.  Return value
 * is status; zero is success.
 */
extern int stpi_color_get_row(stp_const_vars_t v, stp_image_t *image,
			      int row, unsigned short *out, int *zero_mask);

extern stp_parameter_list_t stpi_color_list_parameters(stp_const_vars_t v);

extern void
stpi_color_describe_parameter(stp_const_vars_t v, const char *name,
			      stp_parameter_t *description);

#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_COLOR_H */
