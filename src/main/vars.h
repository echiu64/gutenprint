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

typedef void *(*copy_data_func_t)(const stp_vars_t);
typedef void (*destroy_data_func_t)(stp_vars_t);

extern void	*stp_get_color_data(const stp_vars_t);
extern void	stp_set_color_data(stp_vars_t v, void * val);

extern void	*stp_get_driver_data (const stp_vars_t);
extern void     stp_set_driver_data (stp_vars_t, void * val);

extern copy_data_func_t stp_get_copy_color_data_func(const stp_vars_t);
extern void	stp_set_copy_color_data_func(stp_vars_t, copy_data_func_t);

extern destroy_data_func_t stp_get_destroy_color_data_func(const stp_vars_t);
extern void	stp_set_destroy_color_data_func(stp_vars_t,
						destroy_data_func_t);

extern copy_data_func_t stp_get_copy_driver_data_func(const stp_vars_t);
extern void	stp_set_copy_driver_data_func(stp_vars_t, copy_data_func_t);

extern destroy_data_func_t stp_get_destroy_driver_data_func(const stp_vars_t);
extern void	stp_set_destroy_driver_data_func(stp_vars_t,
						 destroy_data_func_t);

extern int      stp_get_verified(const stp_vars_t);
extern void     stp_set_verified(stp_vars_t, int value);

extern void     stp_copy_options(stp_vars_t vd, const stp_vars_t vs);

extern const stp_vars_t stp_minimum_settings(void);
extern const stp_vars_t stp_maximum_settings(void);

extern void
stp_fill_parameter_settings(stp_parameter_t *desc,
			    const stp_parameter_t *param);

#endif /* GIMP_PRINT_INTERNAL_VARS_H */
/*
 * End of "$Id$".
 */
