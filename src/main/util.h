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

#ifndef __GNUC__
#ifndef __attribute__
#define __attribute__(ignore)
#endif
#endif

extern void stp_zprintf(const stp_vars_t v, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));

extern void stp_zfwrite(const char *buf, size_t bytes, size_t nitems,
			const stp_vars_t v);

extern void stp_putc(int ch, const stp_vars_t v);
extern void stp_put16_le(unsigned short sh, const stp_vars_t v);
extern void stp_put16_be(unsigned short sh, const stp_vars_t v);
extern void stp_put32_le(unsigned int sh, const stp_vars_t v);
extern void stp_put32_be(unsigned int sh, const stp_vars_t v);
extern void stp_puts(const char *s, const stp_vars_t v);
extern void stp_send_command(const stp_vars_t v, const char *command,
			     const char *format, ...);

extern void stp_erputc(int ch);

extern void stp_eprintf(const stp_vars_t v, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));
extern void stp_erprintf(const char *format, ...)
       __attribute__((format(__printf__, 1, 2)));

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
extern unsigned long stp_debug_level;

extern void stp_dprintf(unsigned long level, const stp_vars_t v,
			const char *format, ...)
       __attribute__((format(__printf__, 3, 4)));
extern void stp_deprintf(unsigned long level, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));
extern void stp_init_debug_messages(const stp_vars_t v);
extern void stp_flush_debug_messages(const stp_vars_t v);


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

/****************************************************************
*                                                               *
* LISTS                                                         *
*                                                               *
****************************************************************/

typedef void stp_list_item_t;
typedef void stp_list_t;
typedef void (*node_freefunc)(stp_list_item_t *);
typedef void *(*node_copyfunc)(const stp_list_item_t *);
typedef const char *(*node_namefunc)(const stp_list_item_t *);
typedef int (*node_sortfunc)(const stp_list_item_t *, const stp_list_item_t *);

extern void stp_list_node_free_data(stp_list_item_t *item);
extern stp_list_t *stp_list_create(void);
extern stp_list_t *stp_list_copy(stp_list_t *list);
extern int stp_list_destroy(stp_list_t *list);
extern stp_list_item_t *stp_list_get_start(stp_list_t *list);
extern stp_list_item_t *stp_list_get_end(stp_list_t *list);
extern stp_list_item_t *stp_list_get_item_by_index(stp_list_t *list,
						   int index);
extern stp_list_item_t *stp_list_get_item_by_name(stp_list_t *list,
						  const char *name);
extern stp_list_item_t *stp_list_get_item_by_long_name(stp_list_t *list,
						       const char *long_name);
extern int stp_list_get_length(stp_list_t *list);

extern void stp_list_set_freefunc(stp_list_t *list, node_freefunc);
extern node_freefunc stp_list_get_freefunc(stp_list_t *list);

extern void stp_list_set_copyfunc(stp_list_t *list, node_copyfunc);
extern node_copyfunc stp_list_get_copyfunc(stp_list_t *list);

extern void stp_list_set_namefunc(stp_list_t *list, node_namefunc);
extern node_namefunc stp_list_get_namefunc(stp_list_t *list);

extern void stp_list_set_long_namefunc(stp_list_t *list, node_namefunc);
extern node_namefunc stp_list_get_long_namefunc(stp_list_t *list);

extern void stp_list_set_sortfunc(stp_list_t *list, node_sortfunc);
extern node_sortfunc stp_list_get_sortfunc(stp_list_t *list);

extern int stp_list_item_create(stp_list_t *list,
				stp_list_item_t *next,
				void *data);
extern int stp_list_item_destroy(stp_list_t *list,
				 stp_list_item_t *item);
extern stp_list_item_t *stp_list_item_prev(stp_list_item_t *item);
extern stp_list_item_t *stp_list_item_next(stp_list_item_t *item);
extern void *stp_list_item_get_data(const stp_list_item_t *item);
extern int stp_list_item_set_data(stp_list_item_t *item,
				  void *data);

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

#define QUANT(number)							\
do									\
{									\
  gettimeofday(&quantify_cur_time, NULL);				\
  assert(number < NUM_QUANTIFY_BUCKETS);				\
  quantify_counts[number]++;						\
									\
  if (quantify_first_time)						\
    {									\
      quantify_first_time = 0;						\
    }									\
  else									\
    {									\
      if (number > quantify_high_index) quantify_high_index = number;	\
      if (quantify_prev_time.tv_usec > quantify_cur_time.tv_usec)	\
	{								\
	  quantify_buckets[number].tv_usec +=				\
	    ((quantify_cur_time.tv_usec + 1000000) -			\
	     quantify_prev_time.tv_usec);				\
	  quantify_buckets[number].tv_sec +=				\
	    (quantify_cur_time.tv_sec - quantify_prev_time.tv_sec - 1);	\
	}								\
      else								\
	{								\
	  quantify_buckets[number].tv_sec +=				\
	    quantify_cur_time.tv_sec - quantify_prev_time.tv_sec;	\
	  quantify_buckets[number].tv_usec +=				\
	    quantify_cur_time.tv_usec - quantify_prev_time.tv_usec;	\
	}								\
      if (quantify_buckets[number].tv_usec >= 1000000)			\
	{								\
	  quantify_buckets[number].tv_usec -= 1000000;			\
	  quantify_buckets[number].tv_sec++;				\
	}								\
    }									\
  gettimeofday(&quantify_prev_time, NULL);				\
} while (0)

extern void print_timers(void );
#endif


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_UTIL_H */
/*
 * End of "$Id$".
 */
