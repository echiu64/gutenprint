/*
 * "$Id$"
 *
 *   Print plug-in header file for the GIMP.
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

#ifndef _GIMP_PRINT_INTERNAL_H_
#define _GIMP_PRINT_INTERNAL_H_

#define ECOLOR_C 0
#define ECOLOR_M 1
#define ECOLOR_Y 2
#define ECOLOR_K 3
#define NCOLORS (4)

#define MAX_CARRIAGE_WIDTH	80 /* This really needs to go away */
				/* For now, this is wide enough for 4B ISO */

typedef struct
{
  double value;
  unsigned bit_pattern;
  int is_dark;
  unsigned dot_size;
} stp_simple_dither_range_t;

typedef struct
{
  double value;
  double lower;
  double upper;
  unsigned bit_pattern;
  int is_dark;
  unsigned dot_size;
} stp_dither_range_t;

typedef struct
{
   double value_l;
   double value_h;
   unsigned bits_l;
   unsigned bits_h;
   int isdark_l;
   int isdark_h;
} stp_full_dither_range_t;

/*
 * Prototypes...
 */

extern void *	stp_init_dither(int in_width, int out_width,
				int horizontal_aspect,
				int vertical_aspect, stp_vars_t *vars);
extern void	stp_dither_set_transition(void *vd, double);
extern void	stp_dither_set_density(void *vd, double);
extern void	stp_dither_set_black_density(void *vd, double);
extern void 	stp_dither_set_black_lower(void *vd, double);
extern void 	stp_dither_set_black_upper(void *vd, double);
extern void	stp_dither_set_black_levels(void *vd, double, double, double);
extern void 	stp_dither_set_randomizers(void *vd, double, double, double,
					   double);
extern void 	stp_dither_set_ink_darkness(void *vd, double, double, double);
extern void 	stp_dither_set_light_inks(void *vd, double, double, double,
					  double);
extern void	stp_dither_set_ranges(void *vd, int color, int nlevels,
				      const stp_simple_dither_range_t *ranges,
				      double density);
extern void	stp_dither_set_ranges_full(void *vd, int color, int nlevels,
					   const stp_full_dither_range_t *ranges,
					   double density);
extern void	stp_dither_set_ranges_simple(void *vd, int color, int nlevels,
					     const double *levels,
					     double density);
extern void	stp_dither_set_ranges_complete(void *vd, int color, int nlevels,
					       const stp_dither_range_t *ranges);
extern void	stp_dither_set_ink_spread(void *vd, int spread);
extern void	stp_dither_set_max_ink(void *vd, int, double);
extern void	stp_dither_set_x_oversample(void *vd, int os);
extern void	stp_dither_set_y_oversample(void *vd, int os);
extern void	stp_dither_set_adaptive_divisor(void *vd, unsigned divisor);


extern void	stp_free_dither(void *);


extern void	stp_dither_monochrome(const unsigned short *, int, void *,
				      unsigned char *, int duplicate_line);

extern void	stp_dither_black(const unsigned short *, int, void *,
				 unsigned char *, int duplicate_line);

extern void	stp_dither_cmyk(const unsigned short *, int, void *,
				unsigned char *,
				unsigned char *, unsigned char *,
				unsigned char *, unsigned char *,
				unsigned char *, unsigned char *,
				int duplicate_line);


extern void *	stp_initialize_weave_params(int S, int J, int O,
					    int firstrow, int lastrow,
					    int pageheight, int strategy);
extern void	stp_calculate_row_parameters(void *w, int row, int subpass,
					     int *pass, int *jet,
					     int *startrow, int *phantomrows,
					     int *jetsused);
extern void	stp_destroy_weave_params(void *vw);

extern void	stp_fold(const unsigned char *line, int single_height,
			 unsigned char *outbuf);

extern void	stp_split_2(int height, int bits, const unsigned char *in,
			    unsigned char *outhi, unsigned char *outlo);

extern void	stp_split_4(int height, int bits, const unsigned char *in,
			    unsigned char *out0, unsigned char *out1,
			    unsigned char *out2, unsigned char *out3);

extern void	stp_unpack_2(int height, int bits, const unsigned char *in,
			     unsigned char *outlo, unsigned char *outhi);

extern void	stp_unpack_4(int height, int bits, const unsigned char *in,
			     unsigned char *out0, unsigned char *out1,
			     unsigned char *out2, unsigned char *out3);

extern void	stp_unpack_8(int height, int bits, const unsigned char *in,
			     unsigned char *out0, unsigned char *out1,
			     unsigned char *out2, unsigned char *out3,
			     unsigned char *out4, unsigned char *out5,
			     unsigned char *out6, unsigned char *out7);

extern int	stp_pack(const unsigned char *line, int height,
			 unsigned char *comp_buf, unsigned char **comp_ptr);

/* Uncomment the next line to get performance statistics:
 * look for QUANT(#) in the code. At the end of escp2-print
 * run, it will print out how long and how many time did 
 * certain pieces of code take. Of course, don't forget about
 * overhead of call to gettimeofday - it's not zero.
 * If you need more detailed performance stats, just put
 * QUANT() calls in the interesting spots in the code */
/*#define QUANTIFY*/
#ifdef QUANTIFY
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#else
#define QUANT(n)
#endif

#ifdef QUANTIFY
/* Used for performance analysis - to be called before and after
 * the interval to be quantified */
#define NUM_QUANTIFY_BUCKETS 1024
extern unsigned quantify_counts[NUM_QUANTIFY_BUCKETS];
extern struct timeval quantify_buckets[NUM_QUANTIFY_BUCKETS];
extern int quantify_high_index;
extern int quantify_first_time;
extern struct timeval quantify_cur_time;
extern struct timeval quantify_prev_time;

#define QUANT(number) \
{\
    gettimeofday(&quantify_cur_time, NULL);\
    assert(number < NUM_QUANTIFY_BUCKETS);\
    quantify_counts[number]++;\
\
    if (quantify_first_time) {\
        quantify_first_time = 0;\
    } else {\
        if (number > quantify_high_index) quantify_high_index = number;\
        if (quantify_prev_time.tv_usec > quantify_cur_time.tv_usec) {\
           quantify_buckets[number].tv_usec += ((quantify_cur_time.tv_usec + 1000000) - quantify_prev_time.tv_usec);\
           quantify_buckets[number].tv_sec += (quantify_cur_time.tv_sec - quantify_prev_time.tv_sec - 1);\
        } else {\
           quantify_buckets[number].tv_sec += quantify_cur_time.tv_sec - quantify_prev_time.tv_sec;\
           quantify_buckets[number].tv_usec += quantify_cur_time.tv_usec - quantify_prev_time.tv_usec;\
        }\
        if (quantify_buckets[number].tv_usec >= 1000000)\
        {\
           quantify_buckets[number].tv_usec -= 1000000;\
           quantify_buckets[number].tv_sec++;\
        }\
    }\
\
    gettimeofday(&quantify_prev_time, NULL);\
}

extern void  print_timers(void );
#endif

#ifdef __cplusplus
  }
#endif

#endif /* _GIMP_PRINT_INTERNAL_H_ */
/*
 * End of "$Id$".
 */
