/*
 * "$Id$"
 *
 *   libgimpprint sequence functions.
 *
 *   Copyright 2003 Roger Leigh (roger@whinlatter.uklinux.net)
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

#ifndef GIMP_PRINT_SEQUENCE_H
#define GIMP_PRINT_SEQUENCE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void *stp_sequence_t;


extern stp_sequence_t stp_sequence_create(void);
extern void stp_sequence_destroy(stp_sequence_t sequence);

extern void stp_sequence_copy(stp_sequence_t dest,
			      const stp_sequence_t source);
extern stp_sequence_t stp_sequence_create_copy(const stp_sequence_t sequence);

extern int stp_sequence_set_bounds(stp_sequence_t sequence,
				   double low, double high);
extern void stp_sequence_get_bounds(const stp_sequence_t sequence,
				    double *low, double *high);

extern void stp_sequence_get_range(const stp_sequence_t sequence,
				   double *low, double *high);

extern int stp_sequence_set_size(stp_sequence_t sequence, size_t size);
extern size_t stp_sequence_get_size(const stp_sequence_t sequence);

extern int stp_sequence_set_data(stp_sequence_t sequence,
				 size_t count, const double *data);
extern int stp_sequence_set_subrange(stp_sequence_t sequence,
				     size_t where, size_t size,
				     const double *data);
extern void stp_sequence_get_data(const stp_sequence_t sequence,
				  size_t *size, const double **data);

extern int stp_sequence_set_point(stp_sequence_t sequence,
				  size_t where, double data);
extern int stp_sequence_get_point(const stp_sequence_t sequence,
				  size_t where, double *data);


extern int stp_sequence_set_float_data(stp_sequence_t sequence,
				       size_t count, const float *data);
extern int stp_sequence_set_long_data(stp_sequence_t sequence,
				      size_t count, const long *data);
extern int stp_sequence_set_ulong_data(stp_sequence_t sequence,
				       size_t count, const unsigned long *data);
extern int stp_sequence_set_int_data(stp_sequence_t sequence,
				     size_t count, const int *data);
extern int stp_sequence_set_uint_data(stp_sequence_t sequence,
				      size_t count, const unsigned int *data);
extern int stp_sequence_set_short_data(stp_sequence_t sequence,
				       size_t count, const short *data);
extern int stp_sequence_set_ushort_data(stp_sequence_t sequence,
					size_t count, const unsigned short *data);

/*
 * Convert the curve data to another data type.
 *
 * The pointer returned is owned by the curve, and is not guaranteed
 * to be valid beyond the next non-const curve call;
 *
 * If the bounds of the curve exceed the limits of the data type,
 * NULL is returned.
 */
extern const float *stp_sequence_get_float_data(const stp_sequence_t sequence,
						size_t *count);
extern const long *stp_sequence_get_long_data(const stp_sequence_t sequence,
					      size_t *count);
extern const unsigned long *stp_sequence_get_ulong_data(const stp_sequence_t sequence,
							size_t *count);
extern const int *stp_sequence_get_int_data(const stp_sequence_t sequence,
					    size_t *count);
extern const unsigned int *stp_sequence_get_uint_data(const stp_sequence_t sequence,
						      size_t *count);
extern const short *stp_sequence_get_short_data(const stp_sequence_t sequence,
						size_t *count);
extern const unsigned short *stp_sequence_get_ushort_data(const stp_sequence_t sequence,
							  size_t *count);


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_SEQUENCE_H */
