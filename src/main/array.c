/*
 * "$Id$"
 *
 *   Array data type.  This type is designed to be derived from by
 *   the curve and dither matrix types.
 *
 *   Copyright 2002-2003 Robert Krawitz (rlk@alum.mit.edu)
 *   Copyright 2003      Roger Leigh (roger@whinlatter.uklinux.net)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "xml.h"


#define COOKIE_ARRAY 0x58dd1c48

typedef struct
{
  int cookie;
  int x_size;
  int y_size;
  stp_sequence_t data;
} stpi_internal_array_t;

/*
 * We could do more sanity checks here if we want.
 */
static void
check_array(const stpi_internal_array_t *v)
{
  if (v == NULL)
    {
      stpi_erprintf("Null stp_array_t! Please report this bug.\n");
      stpi_abort();
    }
  if (v->cookie != COOKIE_ARRAY)
    {
      stpi_erprintf("Bad stp_array_t! Please report this bug.\n");
      stpi_abort();
    }
}


static void array_ctor(stpi_internal_array_t *ia)
{
  ia->cookie = COOKIE_ARRAY;
  ia->data = stp_sequence_create();
  stp_sequence_set_size(ia->data, ia->x_size * ia->y_size);
}

stp_array_t
stp_array_create(int x_size, int y_size)
{
  stpi_internal_array_t *ret;
  ret = stpi_zalloc(sizeof(stpi_internal_array_t));
  ret->x_size = x_size;
  ret->y_size = y_size;
  ret->data = NULL;
  array_ctor(ret);
  return (stp_array_t) ret;
}


static void
array_dtor(stpi_internal_array_t *ia)
{
  if (ia->data)
    stp_sequence_destroy(ia->data);
  memset(ia, 0, sizeof(stpi_internal_array_t));
}

void
stp_array_destroy(stp_array_t array)
{
  stpi_internal_array_t *ia = (stpi_internal_array_t *) array;
  check_array(ia);
  array_dtor(ia);
  stpi_free(ia);
}

void stp_array_copy(stp_array_t dest,
		       const stp_array_t source)
{
  stpi_internal_array_t *idest = (stpi_internal_array_t *) dest;
  stpi_internal_array_t *isource = (stpi_internal_array_t *) source;
  check_array(idest);
  check_array(isource);

  idest->x_size = isource->x_size;
  idest->y_size = isource->y_size;
  if (idest->data)
    stp_sequence_destroy(idest->data);
  idest->data = stp_sequence_create_copy(isource->data);
}

stp_array_t
stp_array_create_copy(const stp_array_t array)
{
  stpi_internal_array_t *ia = (stpi_internal_array_t *) array;
  stp_array_t ret;
  check_array(ia);
  ret = stp_array_create(0, 0); /* gets freed next */
  stp_array_copy(ret, array);
  return ret;
}


int
stp_array_set_size(stp_array_t array, int x_size, int y_size)
{
  stpi_internal_array_t *ia = (stpi_internal_array_t *) array;
  check_array(ia);
  if (ia->data) /* Free old data */
    stp_sequence_destroy(ia->data);
  ia->x_size = x_size;
  ia->y_size = y_size;
  ia->data = stp_sequence_create();
  stp_sequence_set_size(ia->data, ia->x_size * ia->y_size);
  return 1;
}


void
stp_array_get_size(const stp_array_t array, int *x_size, int *y_size)
{
  stpi_internal_array_t *ia = (stpi_internal_array_t *) array;
  check_array(ia);
  *x_size = ia->x_size;
  *y_size = ia->y_size;
  return;
}



int
stp_array_set_data(stp_array_t array,
		   const double *data)
{
  stpi_internal_array_t *ia = (stpi_internal_array_t *) array;
  check_array(ia);
  stp_sequence_set_data(ia->data, ia->x_size * ia->y_size,
			data);
  return 1;
}


void
stp_array_get_data(const stp_array_t array, size_t *size,
		   const double **data)
{
  stpi_internal_array_t *ia = (stpi_internal_array_t *) array;
  check_array(ia);
  stp_sequence_get_data(ia->data, size, data);
}


int
stp_array_set_point(stp_array_t array, int x, int y,
		       double data)
{
  stpi_internal_array_t *ia = (stpi_internal_array_t *) array;
  check_array(ia);

  if (((ia->x_size * x) + y) >= (ia->x_size * ia->y_size))
    return 0;

  return stp_sequence_set_point(ia->data,
				(ia->x_size * x) + y, data);
}

int
stp_array_get_point(const stp_array_t array, int x, int y,
		    double *data)
{
  stpi_internal_array_t *ia = (stpi_internal_array_t *) array;
  check_array(ia);

  if (((ia->x_size * x) + y) >= ia->x_size * ia->y_size)
    return 0;
  return stp_sequence_get_point(ia->data,
				(ia->x_size * x) + y, data);
}


const stp_sequence_t
stp_array_get_sequence(const stp_array_t array)
{
  stpi_internal_array_t *ia = (stpi_internal_array_t *) array;
  check_array(ia);

  return ia->data;
}

stp_array_t
stpi_array_create_from_xmltree(xmlNodePtr array)  /* The array node */
{
  xmlChar *stmp;                          /* Temporary string */
  xmlNodePtr child;                       /* Child sequence node */
  int x_size, y_size;
  size_t count;
  stp_sequence_t seq = NULL;
  stp_array_t ret = NULL;
  stpi_internal_array_t *iret;

  stmp = xmlGetProp(array, (const xmlChar *) "x-size");
  if (stmp)
    {
      x_size = (int) stpi_xmlstrtoul(stmp);
      xmlFree(stmp);
    }
  else
    {
      stpi_erprintf("stpi_array_create_from_xmltree: \"x-size\" missing\n");
      goto error;
    }
  /* Get y-size */
  stmp = xmlGetProp(array, (const xmlChar *) "y-size");
  if (stmp)
    {
      y_size = (int) stpi_xmlstrtoul(stmp);
      xmlFree(stmp);
    }
  else
    {
      stpi_erprintf("stpi_array_create_from_xmltree: \"y-size\" missing\n");
      goto error;
    }

  /* Get the sequence data */

  child = array->children;
  while (child)
    {
      if (!xmlStrcmp(child->name, (const xmlChar *) "sequence"))
	{
	  seq = stpi_sequence_create_from_xmltree(child);
	  break;
	}
      child = child->next;
    }

  if (seq == NULL)
    goto error;

  ret = stp_array_create(x_size, y_size);
  iret = (stpi_internal_array_t *) ret;
  if (iret->data)
    stp_sequence_destroy(iret->data);
  iret->data = seq;

  count = stp_sequence_get_size(seq);
  if (count != (x_size * y_size))
    {
      stpi_erprintf("stpi_array_create_from_xmltree: size mismatch between array and sequence\n");
      goto error;
    }

  return ret;

 error:
  stpi_erprintf("stpi_array_create_from_xmltree: error during curve read\n");
  if (ret)
    stp_array_destroy(ret);
  return NULL;
}
