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

#ifndef GIMP_PRINT_INTERNAL_UTIL_H
#define GIMP_PRINT_INTERNAL_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


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

typedef void *(*copy_data_func_t)(const stp_vars_t);
typedef void (*destroy_data_func_t)(stp_vars_t);

/* Color conversion function -- almost surely will change! */
typedef void (*stp_convert_t) (const stp_vars_t vars, const unsigned char *in,
                               unsigned short *out, int *zero_mask,
                               int width, int bpp);

extern void stp_zprintf(const stp_vars_t v, const char *format, ...);

extern void stp_zfwrite(const char *buf, size_t bytes, size_t nitems,
			const stp_vars_t v);

extern void stp_putc(int ch, const stp_vars_t v);
extern void stp_erputc(int ch);

extern void stp_puts(const char *s, const stp_vars_t v);

extern void stp_eprintf(const stp_vars_t v, const char *format, ...);
extern void stp_erprintf(const char *format, ...);

#define STP_DBG_LUT 		0x1
#define STP_DBG_COLORFUNC	0x2
#define STP_DBG_INK		0x4
#define STP_DBG_PS		0x8
#define STP_DBG_PCL		0x10
#define STP_DBG_ESCP2		0x20
#define STP_DBG_CANON		0x40
#define STP_DBG_LEXMARK		0x80
#define STP_DBG_WEAVE_PARAMS	0x100
#define STP_DBG_ROWS		0x200
#define STP_DBG_MARK_FILE       0x400
extern void stp_dprintf(unsigned long level, const stp_vars_t v,
			const char *format, ...);
extern void stp_deprintf(unsigned long level, const char *format, ...);
extern unsigned long stp_debug_level;

extern void *stp_malloc (size_t);
extern void *stp_zalloc (size_t);
extern void *stp_realloc (void *ptr, size_t);
extern void stp_free(void *ptr);

extern size_t stp_strlen(const char *s);
extern char *stp_strndup(const char *s, int n);
extern char *stp_strdup(const char *s);
extern stp_curve_t stp_read_and_compose_curves(const char *s1, const char *s2,
					       stp_curve_compose_t comp);
extern void stp_abort(void);

/*
 * Choose the appropriate color function depending upon choice of input
 * and output.
 */
extern stp_convert_t stp_choose_colorfunc(const stp_vars_t v, int image_bpp,
					  int *out_channels);

/*
 * hue_map, lum_map, and sat_map adjust the hue, luminosity, and saturation
 * of the print.
 */
extern void stp_compute_lut(stp_vars_t v, size_t steps, stp_curve_t hue,
			    stp_curve_t lum, stp_curve_t sat);
extern void stp_free_lut(stp_vars_t v);

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

#endif /* GIMP_PRINT_INTERNAL_UTIL_H */
/*
 * End of "$Id$".
 */
