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

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <math.h>
#include <limits.h>
#if defined(HAVE_VARARGS_H) && !defined(HAVE_STDARG_H)
#include <varargs.h>
#else
#include <stdarg.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FMIN(a, b) ((a) < (b) ? (a) : (b))

/*
 * We cannot avoid use of the (non-ANSI) vsnprintf here; ANSI does
 * not provide a safe, length-limited sprintf function.
 */

#define STP_VASPRINTF(result, bytes, format)				\
{									\
  int current_allocation = 64;						\
  result = stp_malloc(current_allocation);				\
  while (1)								\
    {									\
      va_list args;							\
      va_start(args, format);						\
      bytes = vsnprintf(result, current_allocation, format, args);	\
      va_end(args);							\
      if (bytes >= 0 && bytes < current_allocation)			\
	break;								\
      else								\
	{								\
	  free (result);						\
	  if (bytes < 0)						\
	    current_allocation *= 2;					\
	  else								\
	    current_allocation = bytes + 1;				\
	  result = stp_malloc(current_allocation);			\
	}								\
    }									\
}

void
stp_zprintf(const stp_vars_t v, const char *format, ...)
{
  char *result;
  int bytes;
  STP_VASPRINTF(result, bytes, format);
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), result, bytes);
  free(result);
}

void
stp_zfwrite(const char *buf, size_t bytes, size_t nitems, const stp_vars_t v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), buf, bytes * nitems);
}

void
stp_putc(int ch, const stp_vars_t v)
{
  char a = (char) ch;
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), &a, 1);
}

void
stp_puts(const char *s, const stp_vars_t v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), s, strlen(s));
}

void
stp_eprintf(const stp_vars_t v, const char *format, ...)
{
  int bytes;
  if (stp_get_errfunc(v))
    {
      char *result;
      STP_VASPRINTF(result, bytes, format);
      (stp_get_errfunc(v))((void *)(stp_get_errdata(v)), result, bytes);
      free(result);
    }
  else
    {
      va_list args;
      va_start(args, format);
      vfprintf(stderr, format, args);
      va_end(args);
    }
}

void
stp_erputc(int ch)
{
  putc(ch, stderr);
}

void
stp_erprintf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

unsigned long stp_debug_level = 0;

static void
stp_init_debug(void)
{
  static int debug_initialized = 0;
  if (!debug_initialized)
    {
      const char *dval = getenv("STP_DEBUG");
      debug_initialized = 1;
      if (dval)
	{
	  stp_debug_level = strtoul(dval, 0, 0);
	  stp_erprintf("Gimp-Print %s %s\n", VERSION, RELEASE_DATE);
	}
    }
}

void
stp_dprintf(unsigned long level, const stp_vars_t v, const char *format, ...)
{
  int bytes;
  stp_init_debug();
  if ((level & stp_debug_level) && stp_get_errfunc(v))
    {
      char *result;
      STP_VASPRINTF(result, bytes, format);
      (stp_get_errfunc(v))((void *)(stp_get_errdata(v)), result, bytes);
      free(result);
    }
}

void
stp_deprintf(unsigned long level, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  stp_init_debug();
  if (level & stp_debug_level)
    vfprintf(stderr, format, args);
  va_end(args);
}

void *
stp_malloc (size_t size)
{
  register void *memptr = NULL;

  if ((memptr = malloc (size)) == NULL)
    {
      fputs("Virtual memory exhausted.\n", stderr);
      exit (EXIT_FAILURE);
    }
  return (memptr);
}

void *
stp_zalloc (size_t size)
{
  register void *memptr = stp_malloc(size);
  (void) memset(memptr, 0, size);
  return (memptr);
}

void *
stp_realloc (void *ptr, size_t size)
{
  register void *memptr = NULL;

  if (size > 0 && ((memptr = realloc (ptr, size)) == NULL))
    {
      fputs("Virtual memory exhausted.\n", stderr);
      exit (EXIT_FAILURE);
    }
  return (memptr);
}

void
stp_free(void *ptr)
{
  free(ptr);
}

typedef struct
{
  size_t count;
  size_t active_count;
  stp_param_t *list;
} stp_internal_param_list_t;

static char *
c_strdup(const char *s)
{
  char *ret = stp_malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

stp_param_list_t
stp_param_list_allocate(void)
{
  return (stp_param_list_t) stp_zalloc(sizeof(stp_internal_param_list_t));
}

void
stp_param_list_free(stp_param_list_t list)
{
  stp_internal_param_list_t *ilist = (stp_internal_param_list_t *) list;
  size_t i = 0;
  while (i < ilist->active_count)
    {
      stp_free((void *) (ilist->list[i].name));
      stp_free((void *) (ilist->list[i].text));
      i++;
    }
  if (ilist->list)
    stp_free(ilist->list);
  stp_free(ilist);
}

stp_param_t *
stp_param_list_param(const stp_param_list_t list, size_t element)
{
  const stp_internal_param_list_t *ilist = (stp_internal_param_list_t *) list;
  if (element >= ilist->active_count)
    return NULL;
  else
    return &(ilist->list[element]);
}

size_t
stp_param_list_count(const stp_param_list_t list)
{
  const stp_internal_param_list_t *ilist = (stp_internal_param_list_t *) list;
  return ilist->active_count;
}

stp_param_list_t
stp_param_list_duplicate(const stp_param_list_t list)
{
  size_t i = 0;
  stp_param_list_t retval = stp_param_list_allocate();
  while (i < stp_param_list_count(list))
    {
      const stp_param_t *param = stp_param_list_param(list, i);
      stp_param_list_add_param(retval, param->name, param->text);
      i++;
    }
  return retval;
}

stp_param_list_t
stp_param_list_duplicate_params(const stp_param_t *list, size_t count)
{
  size_t i = 0;
  stp_param_list_t retval = stp_param_list_allocate();
  while (i < count)
    {
      stp_param_list_add_param(retval, list[i].name, list[i].text);
      i++;
    }
  return retval;
}

void
stp_param_list_add_param(stp_param_list_t list,
			 const char *name, const char *text)
{
  stp_internal_param_list_t *ilist = (stp_internal_param_list_t *) list;
  if (ilist->count == 0)
    {
      ilist->list = stp_zalloc(sizeof(stp_param_t));
      ilist->count = 1;
    }
  else if (ilist->active_count == ilist->count)
    {
      ilist->list =
	stp_realloc(ilist->list, 2 * ilist->count * sizeof(stp_param_t));
      ilist->count *= 2;
    }
  ilist->list[ilist->active_count].name = c_strdup(name);
  ilist->list[ilist->active_count].text = c_strdup(text);
  ilist->active_count++;
}

int
stp_init(void)
{
  static int stp_is_initialised = 0;
  if (!stp_is_initialised)
    {
      /* Things that are only initialised once */
      /* Set up gettext */
#ifdef ENABLE_NLS
      setlocale (LC_ALL, "");
      bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
#endif
      stp_init_debug();
      stp_init_printer_list();
    }
  stp_is_initialised = 1;
  return (0);
}

const char *
stp_set_output_codeset(const char *codeset)
{
#ifdef ENABLE_NLS
  return (const char *)(bind_textdomain_codeset(PACKAGE, codeset));
#else
  return "US-ASCII";
#endif
}

#ifdef QUANTIFY
unsigned quantify_counts[NUM_QUANTIFY_BUCKETS] = {0};
struct timeval quantify_buckets[NUM_QUANTIFY_BUCKETS] = {{0,0}};
int quantify_high_index = 0;
int quantify_first_time = 1;
struct timeval quantify_cur_time;
struct timeval quantify_prev_time;

void print_timers(const stp_vars_t v)
{
  int i;

  stp_eprintf(v, "%s", "Quantify timers:\n");
  for (i = 0; i <= quantify_high_index; i++)
    {
      if (quantify_counts[i] > 0)
	{
	  stp_eprintf(v,
		      "Bucket %d:\t%ld.%ld s\thit %u times\n", i,
		      quantify_buckets[i].tv_sec, quantify_buckets[i].tv_usec,
		      quantify_counts[i]);
	  quantify_buckets[i].tv_sec = 0;
	  quantify_buckets[i].tv_usec = 0;
	  quantify_counts[i] = 0;
	}
    }
}
#endif
