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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "module.h"
#include "xml.h"

#define FMIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct
{
  stp_outfunc_t ofunc;
  void *odata;
  char *data;
  size_t bytes;
} debug_msgbuf_t;

typedef struct
{
  enum
    {
      TYPE_INVALID,
      TYPE_STRING,
      TYPE_FILE
    } xtype;
  union
  {
    FILE *f;
    struct
    {
      char *data;
      off_t offset;
    } s;
  } d;
} xio_t;


/*
 * We cannot avoid use of the (non-ANSI) vsnprintf here; ANSI does
 * not provide a safe, length-limited sprintf function.
 */

#define STPI_VASPRINTF(result, bytes, format)				\
{									\
  int current_allocation = 64;						\
  result = stpi_malloc(current_allocation);				\
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
	  result = stpi_malloc(current_allocation);			\
	}								\
    }									\
}

void
stpi_zprintf(const stp_vars_t v, const char *format, ...)
{
  char *result;
  int bytes;
  STPI_VASPRINTF(result, bytes, format);
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), result, bytes);
  stpi_free(result);
}

void
stpi_asprintf(char **strp, const char *format, ...)
{
  char *result;
  int bytes;
  STPI_VASPRINTF(result, bytes, format);
  *strp = result;
}

void
stpi_catprintf(char **strp, const char *format, ...)
{
  char *result1;
  char *result2;
  int bytes;
  STPI_VASPRINTF(result1, bytes, format);
  stpi_asprintf(&result2, "%s%s", *strp, result1);
  stpi_free(result1);
  *strp = result2;
}
  

void
stpi_zfwrite(const char *buf, size_t bytes, size_t nitems, const stp_vars_t v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), buf, bytes * nitems);
}

void
stpi_putc(int ch, const stp_vars_t v)
{
  unsigned char a = (unsigned char) ch;
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), (char *) &a, 1);
}

#define BYTE(expr, byteno) (((expr) >> (8 * byteno)) & 0xff)

void
stpi_put16_le(unsigned short sh, const stp_vars_t v)
{
  stpi_putc(BYTE(sh, 0), v);
  stpi_putc(BYTE(sh, 1), v);
}

void
stpi_put16_be(unsigned short sh, const stp_vars_t v)
{
  stpi_putc(BYTE(sh, 1), v);
  stpi_putc(BYTE(sh, 0), v);
}

void
stpi_put32_le(unsigned int in, const stp_vars_t v)
{
  stpi_putc(BYTE(in, 0), v);
  stpi_putc(BYTE(in, 1), v);
  stpi_putc(BYTE(in, 2), v);
  stpi_putc(BYTE(in, 3), v);
}

void
stpi_put32_be(unsigned int in, const stp_vars_t v)
{
  stpi_putc(BYTE(in, 3), v);
  stpi_putc(BYTE(in, 2), v);
  stpi_putc(BYTE(in, 1), v);
  stpi_putc(BYTE(in, 0), v);
}

void
stpi_puts(const char *s, const stp_vars_t v)
{
  (stp_get_outfunc(v))((void *)(stp_get_outdata(v)), s, strlen(s));
}

void
stpi_send_command(const stp_vars_t v, const char *command,
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
	    case 'd':
	    case 'D':
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

  stpi_puts(command, v);

  va_start(args, format);
  while ((fchar = format[0]) != '\0')
    {
      switch (fchar)
	{
	case 'a':
	  stpi_putc(byte_count, v);
	  break;
	case 'b':
	  stpi_put16_le(byte_count, v);
	  break;
	case 'B':
	  stpi_put16_be(byte_count, v);
	  break;
	case 'd':
	  stpi_put32_le(byte_count, v);
	  break;
	case 'D':
	  stpi_put32_be(byte_count, v);
	  break;
	case 'c':
	  stpi_putc(va_arg(args, unsigned int), v);
	  break;
	case 'h':
	  stpi_put16_le(va_arg(args, unsigned int), v);
	  break;
	case 'H':
	  stpi_put16_be(va_arg(args, unsigned int), v);
	  break;
	case 'l':
	  stpi_put32_le(va_arg(args, unsigned int), v);
	  break;
	case 'L':
	  stpi_put32_be(va_arg(args, unsigned int), v);
	  break;
	case 's':
	  stpi_puts(va_arg(args, const char *), v);
	  break;
	}
      format++;
    }
  va_end(args);
}

void
stpi_eprintf(const stp_vars_t v, const char *format, ...)
{
  int bytes;
  if (stp_get_errfunc(v))
    {
      char *result;
      STPI_VASPRINTF(result, bytes, format);
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
stpi_erputc(int ch)
{
  putc(ch, stderr);
}

void
stpi_erprintf(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

unsigned long stpi_debug_level = 0;

static void
stpi_init_debug(void)
{
  static int debug_initialized = 0;
  if (!debug_initialized)
    {
      const char *dval = getenv("STP_DEBUG");
      debug_initialized = 1;
      if (dval)
	{
	  stpi_debug_level = strtoul(dval, 0, 0);
	  stpi_erprintf("Gimp-Print %s %s\n", VERSION, RELEASE_DATE);
	}
    }
}

void
stpi_dprintf(unsigned long level, const stp_vars_t v, const char *format, ...)
{
  int bytes;
  stpi_init_debug();
  if ((level & stpi_debug_level) && stp_get_errfunc(v))
    {
      char *result;
      STPI_VASPRINTF(result, bytes, format);
      (stp_get_errfunc(v))((void *)(stp_get_errdata(v)), result, bytes);
      free(result);
    }
}

void
stpi_deprintf(unsigned long level, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  stpi_init_debug();
  if (level & stpi_debug_level)
    vfprintf(stderr, format, args);
  va_end(args);
}

static void
fill_buffer_writefunc(void *priv, const char *buffer, size_t bytes)
{
  debug_msgbuf_t *msgbuf = (debug_msgbuf_t *) priv;
  if (msgbuf->bytes == 0)
    msgbuf->data = stpi_malloc(bytes + 1);
  else
    msgbuf->data = stpi_realloc(msgbuf->data, msgbuf->bytes + bytes + 1);
  memcpy(msgbuf->data + msgbuf->bytes, buffer, bytes);
  msgbuf->bytes += bytes;
  msgbuf->data[msgbuf->bytes] = '\0';
}

void
stpi_init_debug_messages(const stp_vars_t v)
{
  int verified_flag = stpi_get_verified(v);
  debug_msgbuf_t *msgbuf = stpi_malloc(sizeof(debug_msgbuf_t));
  msgbuf->ofunc = stp_get_errfunc(v);
  msgbuf->odata = stp_get_errdata(v);
  msgbuf->data = NULL;
  msgbuf->bytes = 0;
  stp_set_errfunc((stp_vars_t) v, fill_buffer_writefunc);
  stp_set_errdata((stp_vars_t) v, msgbuf);
  stpi_set_verified(v, verified_flag);
}

void
stpi_flush_debug_messages(const stp_vars_t v)
{
  int verified_flag = stpi_get_verified(v);
  debug_msgbuf_t *msgbuf = (debug_msgbuf_t *)stp_get_errdata(v);
  stp_set_errfunc((stp_vars_t) v, msgbuf->ofunc);
  stp_set_errdata((stp_vars_t) v, msgbuf->odata);
  stpi_set_verified(v, verified_flag);
  if (msgbuf->bytes > 0)
    {
      stpi_eprintf(v, "%s", msgbuf->data);
      stpi_free(msgbuf->data);
    }
  stpi_free(msgbuf);
}

/* pointers to the allocation functions to use, which may be set by
   client applications */
void *(*stpi_malloc_func)(size_t size) = malloc;
void *(*stpi_realloc_func)(void *ptr, size_t size) = realloc;
void (*stpi_free_func)(void *ptr) = free;

void *
stpi_malloc (size_t size)
{
  register void *memptr = NULL;

  if ((memptr = stpi_malloc_func (size)) == NULL)
    {
      fputs("Virtual memory exhausted.\n", stderr);
      stpi_abort();
    }
  return (memptr);
}

void *
stpi_zalloc (size_t size)
{
  register void *memptr = stpi_malloc(size);
  (void) memset(memptr, 0, size);
  return (memptr);
}

void *
stpi_realloc (void *ptr, size_t size)
{
  register void *memptr = NULL;

  if (size > 0 && ((memptr = stpi_realloc_func (ptr, size)) == NULL))
    {
      fputs("Virtual memory exhausted.\n", stderr);
      stpi_abort();
    }
  return (memptr);
}

void
stpi_free(void *ptr)
{
  stpi_free_func(ptr);
}

FILE *
stpi_xio_init_string_input(const char *s)
{
  char template[64];
  int fd;
  mode_t fmode = umask(077);

  strcpy(template, "/tmp/gpxioXXXXXX");
  fd = mkstemp(template);
  umask(fmode);
  if (fd == -1)
    {
      return NULL;
    }
  else
    {
      (void) unlink(template);
      if (write(fd, s, strlen(s)) != strlen(s))
	{
	  (void) close(fd);
	  return NULL;
	}
      (void) lseek(fd, 0, SEEK_SET);
      return fdopen(fd, "r");
    }
}

void *
stpi_xio_init_string_output(void)
{
  xio_t *xio = stpi_zalloc(sizeof(xio_t));
  xio->xtype = TYPE_STRING;
  xio->d.s.data = NULL;
  xio->d.s.offset = 0;
  return xio;
}

static void
check_xio(xio_t *xio)
{
  if (xio->xtype != TYPE_FILE && xio->xtype != TYPE_STRING)
    {
      stpi_erprintf("Bad xio!\n");
      stpi_abort();
    }
}

char *
stpi_xio_get_string_output(void *ixio, size_t *size)
{
  xio_t *xio = (xio_t *) ixio;
  check_xio(xio);
  if (xio->xtype != TYPE_STRING)
    return NULL;
  if (size)
    *size = xio->d.s.offset;
  return xio->d.s.data;
}

void *
stpi_xio_init_file_output(FILE *f)
{
  xio_t *xio = stpi_zalloc(sizeof(xio_t));
  xio->xtype = TYPE_FILE;
  xio->d.f = f;
  return xio;
}

void
stpi_xio_free(void *ixio)
{
  xio_t *xio = (xio_t *) ixio;
  check_xio(xio);
  /* Note that we do not free the string; someone else should be using it! */
  stpi_free(xio);
}

static void
append_xio_output(xio_t *xio, const char *data, int bytes)
{
  char *result2;
  switch (xio->xtype)
    {
    case TYPE_FILE:
      fwrite(data, bytes, 1, xio->d.f);
      break;
    case TYPE_STRING:
      result2 = stpi_malloc(xio->d.s.offset + bytes + 1);
      if (xio->d.s.data)
	{
	  memcpy(result2, xio->d.s.data, xio->d.s.offset);
	  stpi_free(xio->d.s.data);
	}
      memcpy(result2 + xio->d.s.offset, data, bytes);
      result2[bytes + xio->d.s.offset] = '\0';
      xio->d.s.offset += bytes;
      xio->d.s.data = result2;
      break;
    default:
      break;
    }
}

void
stpi_xio_printf(void *ixio, const char *format, ...)
{
  xio_t *xio = (xio_t *) ixio;
  char *result;
  int bytes;
  check_xio(xio);
  STPI_VASPRINTF(result, bytes, format);
  append_xio_output(xio, result, bytes);
  stpi_free(result);
}

void
stpi_xio_puts(const char *s, void *ixio)
{
  xio_t *xio = (xio_t *) ixio;
  check_xio(xio);
  append_xio_output(xio, s, strlen(s));
}

void
stpi_xio_putc(int c, void *ixio)
{
  xio_t *xio = (xio_t *) ixio;
  char cc = (char) c;
  check_xio(xio);
  append_xio_output(xio, &cc, 1);
}

void
stpi_xio_fwrite(const void *ptr, size_t size, size_t count, void *ixio)
{
  xio_t *xio = (xio_t *) ixio;
  check_xio(xio);
  append_xio_output(xio, ptr, size * count);
}

int
stp_init(void)
{
  static int stpi_is_initialised = 0;
  if (!stpi_is_initialised)
    {
      /* Things that are only initialised once */
      /* Set up gettext */
#ifdef ENABLE_NLS
      setlocale (LC_ALL, "");
      bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
#endif
      stpi_init_debug();
      /* Load modules */
      if (stpi_module_load())
	return 1;
      /* Load XML data */
      if (stpi_xml_init())
	return 1;
      /* Initialise modules */
      if (stpi_module_init())
	return 1;
    }

  stpi_is_initialised = 1;
  return 0;
}

size_t
stpi_strlen(const char *s)
{
  return strlen(s);
}

char *
stpi_strndup(const char *s, int n)
{
  char *ret;
  if (!s || n < 0)
    {
      ret = stpi_malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    {
      ret = stpi_malloc(n + 1);
      memcpy(ret, s, n);
      ret[n] = 0;
      return ret;
    }
}

char *
stpi_strdup(const char *s)
{
  char *ret;
  if (!s)
    {
      ret = stpi_malloc(1);
      ret[0] = 0;
      return ret;
    }
  else
    return stpi_strndup(s, stpi_strlen(s));
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
stpi_read_and_compose_curves(const char *s1, const char *s2,
			    stp_curve_compose_t comp)
{
  stp_curve_t ret = NULL;
  stp_curve_t t1 = NULL;
  stp_curve_t t2 = NULL;
  if (s1)
    t1 = stp_curve_create_read_string(s1);
  if (s2)
    t2 = stp_curve_create_read_string(s2);
  if (t1 && t2)
    stp_curve_compose(&ret, t1, t2, comp, -1);
  if (ret)
    {
      stp_curve_free(t1);
      stp_curve_free(t2);
      return ret;
    }
  else if (t1)
    {
      stp_curve_free(t2);
      return t1;
    }
  else
    return t2;
}

void
stpi_abort(void)
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

  stpi_eprintf(v, "%s", "Quantify timers:\n");
  for (i = 0; i <= quantify_high_index; i++)
    {
      if (quantify_counts[i] > 0)
	{
	  stpi_eprintf(v,
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
