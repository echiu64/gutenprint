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

extern void stpi_zprintf(const stp_vars_t v, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));

extern void stpi_zfwrite(const char *buf, size_t bytes, size_t nitems,
			const stp_vars_t v);

extern void stpi_putc(int ch, const stp_vars_t v);
extern void stpi_put16_le(unsigned short sh, const stp_vars_t v);
extern void stpi_put16_be(unsigned short sh, const stp_vars_t v);
extern void stpi_put32_le(unsigned int sh, const stp_vars_t v);
extern void stpi_put32_be(unsigned int sh, const stp_vars_t v);
extern void stpi_puts(const char *s, const stp_vars_t v);
extern void stpi_send_command(const stp_vars_t v, const char *command,
			     const char *format, ...);

extern void stpi_erputc(int ch);

extern void stpi_eprintf(const stp_vars_t v, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));
extern void stpi_erprintf(const char *format, ...)
       __attribute__((format(__printf__, 1, 2)));
extern void stpi_asprintf(char **strp, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));
extern void stpi_catprintf(char **strp, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));

/*
 * vfscanf isn't universal, so we have to do this kind of ugly hack
 */
extern FILE *stpi_xio_init_string_input(const char *s);
extern void *stpi_xio_init_string_output(void);
extern void *stpi_xio_init_file_output(FILE *f);
extern char *stpi_xio_get_string_output(void *ixio, size_t *size);
extern void stpi_xio_printf(void *ixio, const char *format, ...);
extern void stpi_xio_puts(const char *s, void *ixio);
extern void stpi_xio_putc(int c, void *ixio);
extern void stpi_xio_fwrite(const void *ptr, size_t size, size_t count, void *ixio);
extern char *stpi_xio_fgets(char *s, int size, void *ixio);
extern void stpi_xio_free(void *ixio);

#define STPI_DBG_LUT 		0x1
#define STPI_DBG_COLORFUNC	0x2
#define STPI_DBG_INK		0x4
#define STPI_DBG_PS		0x8
#define STPI_DBG_PCL		0x10
#define STPI_DBG_ESCP2		0x20
#define STPI_DBG_CANON		0x40
#define STPI_DBG_LEXMARK	0x80
#define STPI_DBG_WEAVE_PARAMS	0x100
#define STPI_DBG_ROWS		0x200
#define STPI_DBG_MARK_FILE      0x400
#define STPI_DBG_LIST           0x800
#define STPI_DBG_MODULE         0x1000
#define STPI_DBG_PATH           0x2000
#define STPI_DBG_PAPER          0x4000
#define STPI_DBG_PRINTERS       0x8000
#define STPI_DBG_XML            0x10000
extern unsigned long stpi_debug_level;

extern void stpi_dprintf(unsigned long level, const stp_vars_t v,
			const char *format, ...)
       __attribute__((format(__printf__, 3, 4)));
extern void stpi_deprintf(unsigned long level, const char *format, ...)
       __attribute__((format(__printf__, 2, 3)));
extern void stpi_init_debug_messages(const stp_vars_t v);
extern void stpi_flush_debug_messages(const stp_vars_t v);


extern void *stpi_malloc (size_t);
extern void *stpi_zalloc (size_t);
extern void *stpi_realloc (void *ptr, size_t);
extern void stpi_free(void *ptr);

extern size_t stpi_strlen(const char *s);
extern char *stpi_strndup(const char *s, int n);
extern char *stpi_strdup(const char *s);
extern stp_curve_t stpi_read_and_compose_curves(const char *s1, const char *s2,
						stp_curve_compose_t comp);
extern void stpi_abort(void);

/****************************************************************
*                                                               *
* LISTS                                                         *
*                                                               *
****************************************************************/

typedef void stpi_list_item_t;
typedef void stpi_list_t;
typedef void (*node_freefunc)(stpi_list_item_t *);
typedef void *(*node_copyfunc)(const stpi_list_item_t *);
typedef const char *(*node_namefunc)(const stpi_list_item_t *);
typedef int (*node_sortfunc)(const stpi_list_item_t *, const stpi_list_item_t *);

extern void stpi_list_node_free_data(stpi_list_item_t *item);
extern stpi_list_t *stpi_list_create(void);
extern stpi_list_t *stpi_list_copy(stpi_list_t *list);
extern int stpi_list_destroy(stpi_list_t *list);
extern stpi_list_item_t *stpi_list_get_start(stpi_list_t *list);
extern stpi_list_item_t *stpi_list_get_end(stpi_list_t *list);
extern stpi_list_item_t *stpi_list_get_item_by_index(stpi_list_t *list,
						   int index);
extern stpi_list_item_t *stpi_list_get_item_by_name(stpi_list_t *list,
						  const char *name);
extern stpi_list_item_t *stpi_list_get_item_by_long_name(stpi_list_t *list,
						       const char *long_name);
extern int stpi_list_get_length(stpi_list_t *list);

extern void stpi_list_set_freefunc(stpi_list_t *list, node_freefunc);
extern node_freefunc stpi_list_get_freefunc(stpi_list_t *list);

extern void stpi_list_set_copyfunc(stpi_list_t *list, node_copyfunc);
extern node_copyfunc stpi_list_get_copyfunc(stpi_list_t *list);

extern void stpi_list_set_namefunc(stpi_list_t *list, node_namefunc);
extern node_namefunc stpi_list_get_namefunc(stpi_list_t *list);

extern void stpi_list_set_long_namefunc(stpi_list_t *list, node_namefunc);
extern node_namefunc stpi_list_get_long_namefunc(stpi_list_t *list);

extern void stpi_list_set_sortfunc(stpi_list_t *list, node_sortfunc);
extern node_sortfunc stpi_list_get_sortfunc(stpi_list_t *list);

extern int stpi_list_item_create(stpi_list_t *list,
				stpi_list_item_t *next,
				void *data);
extern int stpi_list_item_destroy(stpi_list_t *list,
				 stpi_list_item_t *item);
extern stpi_list_item_t *stpi_list_item_prev(stpi_list_item_t *item);
extern stpi_list_item_t *stpi_list_item_next(stpi_list_item_t *item);
extern void *stpi_list_item_get_data(const stpi_list_item_t *item);
extern int stpi_list_item_set_data(stpi_list_item_t *item,
				  void *data);
extern void stpi_default_media_size(const stp_vars_t v,
				    int *width, int *height);

/*
 * Remove inactive and unclaimed options from the list
 */
extern void stpi_prune_inactive_options(stp_vars_t v);


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
