/*
 * "$Id$"
 *
 *   libgimpprint array functions.
 *
 *   Copyright 2003 Roger Leigh (rleigh@debian.org)
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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gimpprint, etc.
 */

#ifndef __GIMP_PRINT_ARRAY_H__
#define __GIMP_PRINT_ARRAY_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <gimp-print/sequence.h>


typedef void *stp_array_t;
typedef const void *stp_const_array_t;


extern stp_array_t stp_array_create(int x_size, int y_size);
extern void stp_array_destroy(stp_array_t array);

extern void stp_array_copy(stp_array_t dest, stp_const_array_t source);
extern stp_array_t stp_array_create_copy(stp_const_array_t array);

extern int stp_array_set_size(stp_array_t array, int x_size, int y_size);
extern void stp_array_get_size(stp_const_array_t array, int *x_size, int *y_size);

extern int stp_array_set_data(stp_array_t array, const double *data);
extern void stp_array_get_data(stp_const_array_t array, size_t *size,
			       const double **data);

extern int stp_array_set_point(stp_array_t array, int x, int y,
			       double data);
extern int stp_array_get_point(stp_const_array_t array, int x, int y,
			       double *data);

extern stp_const_sequence_t stp_array_get_sequence(stp_const_array_t array);


#ifdef __cplusplus
  }
#endif

#endif /* __GIMP_PRINT_ARRAY_H__ */
