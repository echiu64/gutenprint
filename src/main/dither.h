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


#include "dither-matrices.h"


extern const stp_dither_matrix_short_t stp_dither_matrix_1_1;
extern const stp_dither_matrix_short_t stp_dither_matrix_2_1;
extern const stp_dither_matrix_short_t stp_dither_matrix_4_1;

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

extern void stp_dither_matrix_iterated_init(dither_matrix_t *mat, size_t size,
					    size_t exp, const unsigned *array);
extern void stp_dither_matrix_shear(dither_matrix_t *mat,
				    int x_shear, int y_shear);
extern void stp_dither_matrix_init(dither_matrix_t *mat, int x_size,
				   int y_size, const unsigned int *array,
				   int transpose, int prescaled);
extern void stp_dither_matrix_init_short(dither_matrix_t *mat, int x_size,
					 int y_size,
					 const unsigned short *array,
					 int transpose, int prescaled);
extern int stp_dither_matrix_validate_curve(const stp_curve_t curve);
extern void stp_dither_matrix_init_from_curve(dither_matrix_t *mat,
					      const stp_curve_t curve);
extern void stp_dither_matrix_destroy(dither_matrix_t *mat);
extern void stp_dither_matrix_clone(const dither_matrix_t *src,
				    dither_matrix_t *dest,
				    int x_offset, int y_offset);
extern void stp_dither_matrix_copy(const dither_matrix_t *src,
				   dither_matrix_t *dest);
extern void stp_dither_matrix_scale_exponentially(dither_matrix_t *mat,
						  double exponent);
extern void stp_dither_matrix_set_row(dither_matrix_t *mat, int y);



typedef struct
{
  double value;
  unsigned bit_pattern;
  int subchannel;
  unsigned dot_size;
} stp_dither_range_simple_t;

typedef struct
{
  double value;
  double lower;
  double upper;
  unsigned bit_pattern;
  int subchannel;
  unsigned dot_size;
} stp_dither_range_t;

typedef struct
{
   double value[2];
   unsigned bits[2];
   int subchannel[2];
} stp_dither_range_full_t;

typedef struct
{
  unsigned subchannel_count;
  unsigned char **c;
} stp_dither_channel_t;

typedef struct
{
  unsigned channel_count;
  stp_dither_channel_t *c;
} stp_dither_data_t;

typedef struct stp_dotsize
{
  unsigned bit_pattern;
  double value;
} stp_dotsize_t;

typedef struct stp_shade
{
  double value;
  int subchannel;
  const stp_dotsize_t *dot_sizes;
  int numsizes;
} stp_shade_t;

extern stp_parameter_list_t stp_dither_list_parameters(const stp_vars_t v);

extern void
stp_dither_describe_parameter(const stp_vars_t v, const char *name,
			      stp_parameter_t *description);

extern void *	stp_dither_init(stp_vars_t v, stp_image_t *image,
				int out_width, int xdpi, int ydpi);
extern void	stp_dither_set_iterated_matrix(void *vd, size_t edge,
					       size_t iterations,
					       const unsigned *data,
					       int prescaled,
					       int x_shear, int y_shear);
extern void	stp_dither_set_matrix(void *vd, const stp_dither_matrix_t *mat,
				      int transpose, int x_shear, int y_shear);
extern void	stp_dither_set_matrix_from_curve(void *vd,
						 const stp_curve_t curve);
extern void	stp_dither_set_transition(void *vd, double);
extern void	stp_dither_set_density(void *vd, double);
extern void	stp_dither_set_black_density(void *vd, double);
extern void 	stp_dither_set_black_lower(void *vd, double);
extern void 	stp_dither_set_black_upper(void *vd, double);
extern void	stp_dither_set_black_level(void *vd, int color, double);
extern void 	stp_dither_set_randomizer(void *vd, int color, double);
extern void 	stp_dither_set_ink_darkness(void *vd, int color, double);
extern void 	stp_dither_set_light_ink(void *vd, int color, double, double);
extern void	stp_dither_set_ranges(void *vd, int color, int nlevels,
				      const stp_dither_range_simple_t *ranges,
				      double density);
extern void	stp_dither_set_ranges_full(void *vd, int color, int nlevels,
					   const stp_dither_range_full_t *ranges,
					   double density);
extern void	stp_dither_set_ranges_simple(void *vd, int color, int nlevels,
					     const double *levels,
					     double density);
extern void	stp_dither_set_ink_spread(void *vd, int spread);
extern void	stp_dither_set_adaptive_limit(void *vd, double limit);
extern int	stp_dither_get_first_position(void *vd, int color,int subchan);
extern int	stp_dither_get_last_position(void *vd, int color, int subchan);
extern void	stp_dither_set_shades(void *vd, int color, int nshades,
				      const stp_shade_t *shades, double density);

extern void	stp_dither_free(void *);

extern stp_dither_data_t *stp_dither_data_allocate(void);
extern void	stp_dither_add_channel(stp_dither_data_t *d, unsigned char *data,
				unsigned channel, unsigned subchannel);
extern void	stp_dither_data_free(stp_dither_data_t *d);

extern void	stp_dither(const unsigned short *, int, void *,
			   stp_dither_data_t *, int duplicate_line,
			   int zero_mask);


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_DITHER_H */
/*
 * End of "$Id$".
 */
