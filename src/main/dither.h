/*
 * "$Id$"
 *
 *   libgimpprint dither header.
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

#ifndef GIMP_PRINT_INTERNAL_DITHER_H
#define GIMP_PRINT_INTERNAL_DITHER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
 * ECOLOR_K must be 0
 */
#define ECOLOR_K  0
#define ECOLOR_C  1
#define ECOLOR_M  2
#define ECOLOR_Y  3
#define NCOLORS (4)

typedef struct stpi_dither_matrix_short
{
  int x;
  int y;
  int bytes;
  int prescaled;
  const unsigned short *data;
} stpi_dither_matrix_short_t;

typedef struct stpi_dither_matrix_normal
{
  int x;
  int y;
  int bytes;
  int prescaled;
  const unsigned *data;
} stpi_dither_matrix_normal_t;

typedef struct stpi_dither_matrix
{
  int x;
  int y;
  int bytes;
  int prescaled;
  const void *data;
} stpi_dither_matrix_t;

typedef struct dither_matrix
{
  int base;
  int exp;
  int x_size;
  int y_size;
  int total_size;
  int last_x;
  int last_x_mod;
  int last_y;
  int last_y_mod;
  int index;
  int i_own;
  int x_offset;
  int y_offset;
  unsigned fast_mask;
  unsigned *matrix;
} dither_matrix_t;

extern void stpi_dither_matrix_iterated_init(dither_matrix_t *mat, size_t size,
					     size_t exp, const unsigned *array);
extern void stpi_dither_matrix_shear(dither_matrix_t *mat,
				     int x_shear, int y_shear);
extern void stpi_dither_matrix_init(dither_matrix_t *mat, int x_size,
				    int y_size, const unsigned int *array,
				    int transpose, int prescaled);
extern void stpi_dither_matrix_init_short(dither_matrix_t *mat, int x_size,
					  int y_size,
					  const unsigned short *array,
					  int transpose, int prescaled);
extern int stpi_dither_matrix_validate_array(stp_const_array_t array);
extern void stpi_dither_matrix_init_from_dither_array(dither_matrix_t *mat,
						      stp_const_array_t array,
						      int transpose);
extern void stpi_dither_matrix_destroy(dither_matrix_t *mat);
extern void stpi_dither_matrix_clone(const dither_matrix_t *src,
				     dither_matrix_t *dest,
				     int x_offset, int y_offset);
extern void stpi_dither_matrix_copy(const dither_matrix_t *src,
				    dither_matrix_t *dest);
extern void stpi_dither_matrix_scale_exponentially(dither_matrix_t *mat,
						   double exponent);
extern void stpi_dither_matrix_set_row(dither_matrix_t *mat, int y);
extern stp_array_t stpi_find_standard_dither_array(int x_aspect, int y_aspect);



typedef struct
{
  double value;
  unsigned bit_pattern;
  int subchannel;
  unsigned dot_size;
} stpi_dither_range_simple_t;

typedef struct
{
  double value;
  double lower;
  double upper;
  unsigned bit_pattern;
  int subchannel;
  unsigned dot_size;
} stpi_dither_range_t;

typedef struct
{
   double value[2];
   unsigned bits[2];
   int subchannel[2];
} stpi_dither_range_full_t;

typedef struct stpi_dotsize
{
  unsigned bit_pattern;
  double value;
} stpi_dotsize_t;

typedef struct stpi_shade
{
  double value;
  int subchannel;
  int numsizes;
  const stpi_dotsize_t *dot_sizes;
} stpi_shade_t;

extern stp_parameter_list_t stpi_dither_list_parameters(stp_const_vars_t v);

extern void
stpi_dither_describe_parameter(stp_const_vars_t v, const char *name,
			       stp_parameter_t *description);

extern void stpi_dither_init(stp_vars_t v, stp_image_t *image,
			     int out_width, int xdpi, int ydpi);
extern void stpi_dither_set_iterated_matrix(stp_vars_t v, size_t edge,
					    size_t iterations,
					    const unsigned *data,
					    int prescaled,
					    int x_shear, int y_shear);
extern void stpi_dither_set_matrix(stp_vars_t v, const stpi_dither_matrix_t *mat,
				   int transpose, int x_shear, int y_shear);
extern void stpi_dither_set_matrix_from_dither_array(stp_vars_t v,
						     stp_const_array_t array,
						     int transpose);
extern void stpi_dither_set_transition(stp_vars_t v, double);
extern void stpi_dither_set_randomizer(stp_vars_t v, int color, double);
extern void stpi_dither_set_ranges(stp_vars_t v, int color, int nlevels,
				   const stpi_dither_range_simple_t *ranges,
				   double density);
extern void stpi_dither_set_ranges_full(stp_vars_t v, int color, int nlevels,
					const stpi_dither_range_full_t *ranges,
					double density);
extern void stpi_dither_set_ranges_and_shades_simple(stp_vars_t v,
						     int color, int nlevels,
						     const double *levels,
						     double density);
extern void stpi_dither_set_ink_spread(stp_vars_t v, int spread);
extern void stpi_dither_set_adaptive_limit(stp_vars_t v, double limit);
extern int stpi_dither_get_first_position(stp_vars_t v, int color, int subchan);
extern int stpi_dither_get_last_position(stp_vars_t v, int color, int subchan);
extern void stpi_dither_set_shades(stp_vars_t v, int color, int nshades,
				   const stpi_shade_t *shades, double density);

extern void stpi_dither_add_channel(stp_vars_t v, unsigned char *data,
				    unsigned channel, unsigned subchannel);

extern unsigned char *stpi_dither_get_channel(stp_vars_t v,
					      unsigned channel,
					      unsigned subchannel);

extern void stpi_dither(stp_vars_t v, int row, const unsigned short *input,
			int duplicate_line, int zero_mask);


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_DITHER_H */
/*
 * End of "$Id$".
 */
