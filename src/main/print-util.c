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

#define BYTE(expr, byteno) (((expr) >> (8 * byteno)) & 0xff)

void
stp_put16_le(unsigned short sh, const stp_vars_t v)
{
  stp_putc(BYTE(sh, 0), v);
  stp_putc(BYTE(sh, 1), v);
}

void
stp_put16_be(unsigned short sh, const stp_vars_t v)
{
  stp_putc(BYTE(sh, 1), v);
  stp_putc(BYTE(sh, 0), v);
}

void
stp_put32_le(unsigned int sh, const stp_vars_t v)
{
  stp_putc(BYTE(sh, 0), v);
  stp_putc(BYTE(sh, 1), v);
  stp_putc(BYTE(sh, 2), v);
  stp_putc(BYTE(sh, 3), v);
}

void
stp_put32_be(unsigned int sh, const stp_vars_t v)
{
  stp_putc(BYTE(sh, 3), v);
  stp_putc(BYTE(sh, 2), v);
  stp_putc(BYTE(sh, 1), v);
  stp_putc(BYTE(sh, 0), v);
}

void
stp_puts(const char *s, const stp_vars_t v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), s, strlen(s));
}

void
stp_send_command(const stp_vars_t v, const char *command,
		 const char *format, ...)
{
  int i = 0;
  char fchar;
  const char *out_str;
  unsigned short byte_count = 0;
  va_list args;

  if (strlen(format) > 0)
    {
      va_start(args, format);
      for (i = 0; i < strlen(format); i++)
	{
	  switch (format[i])
	    {
	    case 'a':
	    case 'b':
	    case 'B':
	      break;
	    case 'c':
	      (void) va_arg(args, unsigned int);
	      byte_count += 1;
	      break;
	    case 'h':
	    case 'H':
	      (void) va_arg(args, unsigned int);
	      byte_count += 2;
	      break;
	    case 'l':
	    case 'L':
	      (void) va_arg(args, unsigned int);
	      byte_count += 4;
	      break;
	    case 's':
	      out_str = va_arg(args, const char *);
	      byte_count += strlen(out_str);
	      break;
	    }
	}
      va_end(args);
    }

  stp_puts(command, v);

  va_start(args, format);
  while ((fchar = format[0]) != '\0')
    {
      switch (fchar)
	{
	case 'a':
	  stp_put16_le(byte_count, v);
	  break;
	case 'b':
	  stp_put16_le(byte_count, v);
	  break;
	case 'B':
	  stp_put16_be(byte_count, v);
	  break;
	case 'c':
	  stp_putc(va_arg(args, unsigned int), v);
	  break;
	case 'h':
	  stp_put16_le(va_arg(args, unsigned int), v);
	  break;
	case 'H':
	  stp_put16_be(va_arg(args, unsigned int), v);
	  break;
	case 'l':
	  stp_put32_le(va_arg(args, unsigned int), v);
	  break;
	case 'L':
	  stp_put32_be(va_arg(args, unsigned int), v);
	  break;
	case 's':
	  stp_puts(va_arg(args, const char *), v);
	  break;
	}
      format++;
    }
  va_end(args);
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
      stp_abort();
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
      stp_abort();
    }
  return (memptr);
}

void
stp_free(void *ptr)
{
  free(ptr);
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

size_t
stp_strlen(const char *s)
{
  return strlen(s);
}

char *
stp_strndup(const char *s, int n)
{
  char *ret;
  if (!s || n < 0)
    {
      ret = stp_malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    {
      ret = stp_malloc(n + 1);
      memcpy(ret, s, n);
      ret[n] = 0;
      return ret;
    }
}

char *
stp_strdup(const char *s)
{
  char *ret;
  if (!s)
    {
      ret = stp_malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    return stp_strndup(s, stp_strlen(s));
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

stp_curve_t
stp_read_and_compose_curves(const char *s1, const char *s2,
			    stp_curve_compose_t comp)
{
  stp_curve_t ret = NULL;
  stp_curve_t t1 = NULL;
  stp_curve_t t2 = NULL;
  if (s1)
    t1 = stp_curve_allocate_read_string(s1);
  if (s2)
    t2 = stp_curve_allocate_read_string(s2);
  if (t1 && t2)
    stp_curve_compose(&ret, t1, t2, comp, -1);
  if (ret)
    {
      stp_curve_destroy(t1);
      stp_curve_destroy(t2);
      return ret;
    }
  else if (t1)
    {
      stp_curve_destroy(t2);
      return t1;
    }
  else
    return t2;
}

void
stp_abort(void)
{
  abort();
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
