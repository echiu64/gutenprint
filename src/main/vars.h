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

typedef void *(*stpi_copy_data_func_t)(void *);
typedef void (*stpi_free_data_func_t)(void *);

extern void stpi_allocate_component_data(stp_vars_t v,
					 const char *name,
					 stpi_copy_data_func_t copyfunc,
					 stpi_free_data_func_t freefunc,
					 void *data);
extern void stpi_destroy_component_data(stp_vars_t v, const char *name);
extern void *stpi_get_component_data(stp_const_vars_t v, const char *name);

extern int stpi_get_verified(stp_const_vars_t);
extern void stpi_set_verified(stp_vars_t, int value);

extern void stpi_copy_options(stp_vars_t vd, stp_const_vars_t vs);

extern void stpi_set_output_color_model(stp_vars_t v, int val);
extern int stpi_get_output_color_model(stp_const_vars_t v);

extern void
stpi_fill_parameter_settings(stp_parameter_t *desc,
			     const stp_parameter_t *param);

#endif /* GIMP_PRINT_INTERNAL_VARS_H */
/*
 * End of "$Id$".
 */
