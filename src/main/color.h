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


#include <gimp-print/color.h>


typedef struct
{
  int (*init)(stp_vars_t v, stp_image_t *image, size_t steps);
  int (*get_row)(stp_const_vars_t v, stp_image_t *image,
		 int row, unsigned *zero_mask);
  stp_parameter_list_t (*list_parameters)(stp_const_vars_t v);
  void (*describe_parameter)(stp_const_vars_t v, const char *name,
			     stp_parameter_t *description);
} stpi_colorfuncs_t;


#define COOKIE_COLOR   0x05d892e2

typedef struct stpi_internal_color
{
  int        cookie;            /* Magic number */
  const char *short_name;       /* Color module name */
  const char *long_name;        /* Long name for UI */
  const stpi_colorfuncs_t *colorfuncs;
} stpi_internal_color_t;


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
			      int row, unsigned *zero_mask);

extern stp_parameter_list_t stpi_color_list_parameters(stp_const_vars_t v);

extern void stpi_color_describe_parameter(stp_const_vars_t v, const char *name,
					  stp_parameter_t *description);

extern void stpi_channel_reset(stp_vars_t v);
extern void stpi_channel_reset_channel(stp_vars_t v, int channel);

extern void stpi_channel_add(stp_vars_t v, unsigned channel,
			     unsigned subchannel, double value);

extern void stpi_channel_set_density_adjustment(stp_vars_t v,
						int color, int subchannel,
						double adjustment);


extern void stpi_channel_initialize(stp_vars_t v, stp_image_t *image,
				    int input_channel_count);

extern void stpi_channel_convert(stp_const_vars_t v, unsigned *zero_mask);

extern unsigned short * stpi_channel_get_input(stp_const_vars_t v);

extern unsigned short * stpi_channel_get_output(stp_const_vars_t v);



extern stp_const_color_t
stpi_get_color_by_colorfuncs(stpi_colorfuncs_t *colorfuncs);

extern int
stpi_color_register(const stpi_internal_color_t *color);

extern int
stpi_color_unregister(const stpi_internal_color_t *color);

#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_COLOR_H */
