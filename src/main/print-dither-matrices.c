/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
 *
 *   Copyright 2001 Robert Krawitz (rlk@alum.mit.edu)
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dither-impl.h"
#include "path.h"
#include "xml.h"

#ifdef __GNUC__
#define inline __inline__
#endif


static unsigned
gcd(unsigned a, unsigned b)
{
  unsigned tmp;
  if (b > a)
    {
      tmp = a;
      a = b;
      b = tmp;
    }
  while (1)
    {
      tmp = a % b;
      if (tmp == 0)
	return b;
      a = b;
      b = tmp;
    }
}

static inline int
calc_ordered_point(unsigned x, unsigned y, int steps, int multiplier,
		   int size, const unsigned *map)
{
  int i, j;
  unsigned retval = 0;
  int divisor = 1;
  int div1;
  for (i = 0; i < steps; i++)
    {
      int xa = (x / divisor) % size;
      int ya = (y / divisor) % size;
      unsigned base;
      base = map[ya + (xa * size)];
      div1 = 1;
      for (j = i; j < steps - 1; j++)
	div1 *= size * size;
      retval += base * div1;
      divisor *= size;
    }
  return retval * multiplier;
}

static int
is_po2(size_t i)
{
  if (i == 0)
    return 0;
  return (((i & (i - 1)) == 0) ? 1 : 0);
}

void
stpi_dither_matrix_iterated_init(dither_matrix_t *mat, size_t size,
				 size_t expt, const unsigned *array)
{
  int i;
  int x, y;
  mat->base = size;
  mat->exp = expt;
  mat->x_size = 1;
  for (i = 0; i < expt; i++)
    mat->x_size *= mat->base;
  mat->y_size = mat->x_size;
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = stpi_malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	mat->matrix[x + y * mat->x_size] =
	  calc_ordered_point(x, y, mat->exp, 1, mat->base, array);
	mat->matrix[x + y * mat->x_size] =
	  (double) mat->matrix[x + y * mat->x_size] * 65536.0 /
	  (double) (mat->x_size * mat->y_size);
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
  if (is_po2(mat->x_size))
    mat->fast_mask = mat->x_size - 1;
  else
    mat->fast_mask = 0;
}

#define MATRIX_POINT(m, x, y, x_size, y_size) \
  ((m)[(((x) + (x_size)) % (x_size)) + ((x_size) * (((y) + (y_size)) % (y_size)))])

void
stpi_dither_matrix_shear(dither_matrix_t *mat, int x_shear, int y_shear)
{
  int i;
  int j;
  int *tmp = stpi_malloc(mat->x_size * mat->y_size * sizeof(int));
  for (i = 0; i < mat->x_size; i++)
    for (j = 0; j < mat->y_size; j++)
      MATRIX_POINT(tmp, i, j, mat->x_size, mat->y_size) =
	MATRIX_POINT(mat->matrix, i, j * (x_shear + 1), mat->x_size,
		    mat->y_size);
  for (i = 0; i < mat->x_size; i++)
    for (j = 0; j < mat->y_size; j++)
      MATRIX_POINT(mat->matrix, i, j, mat->x_size, mat->y_size) =
	MATRIX_POINT(tmp, i * (y_shear + 1), j, mat->x_size, mat->y_size);
  stpi_free(tmp);
}

int
stpi_dither_matrix_validate_array(stp_const_array_t array)
{
  double low, high;
  stp_const_sequence_t seq = stp_array_get_sequence(array);
  stp_sequence_get_bounds(seq, &low, &high);
  if (low < 0 || high > 65535)
    return 0;
  return 1;
}


void
stpi_dither_matrix_init_from_dither_array(dither_matrix_t *mat,
					  stp_const_array_t array,
					  int transpose)
{
  int x, y;
  size_t count;
  const unsigned short *vec;
  int x_size, y_size;
  stp_const_sequence_t seq = stp_array_get_sequence(array);
  stp_array_get_size(array, &x_size, &y_size);

  vec = stp_sequence_get_ushort_data(seq, &count);
  mat->base = x_size;;
  mat->exp = 1;
  mat->x_size = x_size;
  mat->y_size = y_size;
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = stpi_malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	if (transpose)
	  mat->matrix[x + y * mat->x_size] = vec[y + x * mat->y_size];
	else
	  mat->matrix[x + y * mat->x_size] = vec[x + y * mat->x_size];
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
  if (is_po2(mat->x_size))
    mat->fast_mask = mat->x_size - 1;
  else
    mat->fast_mask = 0;
}


void
stpi_dither_matrix_init(dither_matrix_t *mat, int x_size, int y_size,
		       const unsigned int *array, int transpose, int prescaled)
{
  int x, y;
  mat->base = x_size;
  mat->exp = 1;
  mat->x_size = x_size;
  mat->y_size = y_size;
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = stpi_malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	if (transpose)
	  mat->matrix[x + y * mat->x_size] = array[y + x * mat->y_size];
	else
	  mat->matrix[x + y * mat->x_size] = array[x + y * mat->x_size];
	if (!prescaled)
	  mat->matrix[x + y * mat->x_size] =
	    (double) mat->matrix[x + y * mat->x_size] * 65536.0 /
	    (double) (mat->x_size * mat->y_size);
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
  if (is_po2(mat->x_size))
    mat->fast_mask = mat->x_size - 1;
  else
    mat->fast_mask = 0;
}

void
stpi_dither_matrix_init_short(dither_matrix_t *mat, int x_size, int y_size,
			     const unsigned short *array, int transpose,
			     int prescaled)
{
  int x, y;
  mat->base = x_size;
  mat->exp = 1;
  mat->x_size = x_size;
  mat->y_size = y_size;
  mat->total_size = mat->x_size * mat->y_size;
  mat->matrix = stpi_malloc(sizeof(unsigned) * mat->x_size * mat->y_size);
  for (x = 0; x < mat->x_size; x++)
    for (y = 0; y < mat->y_size; y++)
      {
	if (transpose)
	  mat->matrix[x + y * mat->x_size] = array[y + x * mat->y_size];
	else
	  mat->matrix[x + y * mat->x_size] = array[x + y * mat->x_size];
	if (!prescaled)
	  mat->matrix[x + y * mat->x_size] =
	    (double) mat->matrix[x + y * mat->x_size] * 65536.0 /
	    (double) (mat->x_size * mat->y_size);
      }
  mat->last_x = mat->last_x_mod = 0;
  mat->last_y = mat->last_y_mod = 0;
  mat->index = 0;
  mat->i_own = 1;
  if (is_po2(mat->x_size))
    mat->fast_mask = mat->x_size - 1;
  else
    mat->fast_mask = 0;
}

void
stpi_dither_matrix_destroy(dither_matrix_t *mat)
{
  if (mat->i_own && mat->matrix)
    stpi_free(mat->matrix);
  mat->matrix = NULL;
  mat->base = 0;
  mat->exp = 0;
  mat->x_size = 0;
  mat->y_size = 0;
  mat->total_size = 0;
  mat->i_own = 0;
}

void
stpi_dither_matrix_clone(const dither_matrix_t *src, dither_matrix_t *dest,
			int x_offset, int y_offset)
{
  dest->base = src->base;
  dest->exp = src->exp;
  dest->x_size = src->x_size;
  dest->y_size = src->y_size;
  dest->total_size = src->total_size;
  dest->matrix = src->matrix;
  dest->x_offset = x_offset;
  dest->y_offset = y_offset;
  dest->last_x = 0;
  dest->last_x_mod = dest->x_offset % dest->x_size;
  dest->last_y = 0;
  dest->last_y_mod = dest->x_size * (dest->y_offset % dest->y_size);
  dest->index = dest->last_x_mod + dest->last_y_mod;
  dest->fast_mask = src->fast_mask;
  dest->i_own = 0;
}

void
stpi_dither_matrix_copy(const dither_matrix_t *src, dither_matrix_t *dest)
{
  int x;
  dest->base = src->base;
  dest->exp = src->exp;
  dest->x_size = src->x_size;
  dest->y_size = src->y_size;
  dest->total_size = src->total_size;
  dest->matrix = stpi_malloc(sizeof(unsigned) * dest->x_size * dest->y_size);
  for (x = 0; x < dest->x_size * dest->y_size; x++)
    dest->matrix[x] = src->matrix[x];
  dest->x_offset = 0;
  dest->y_offset = 0;
  dest->last_x = 0;
  dest->last_x_mod = 0;
  dest->last_y = 0;
  dest->last_y_mod = 0;
  dest->index = 0;
  dest->fast_mask = src->fast_mask;
  dest->i_own = 1;
}

void
stpi_dither_matrix_scale_exponentially(dither_matrix_t *mat, double exponent)
{
  int i;
  int mat_size = mat->x_size * mat->y_size;
  for (i = 0; i < mat_size; i++)
    {
      double dd = mat->matrix[i] / 65535.0;
      dd = pow(dd, exponent);
      mat->matrix[i] = 65535 * dd;
    }
}

void
stpi_dither_matrix_set_row(dither_matrix_t *mat, int y)
{
  mat->last_y = y;
  mat->last_y_mod = mat->x_size * ((y + mat->y_offset) % mat->y_size);
  mat->index = mat->last_x_mod + mat->last_y_mod;
}

static void
preinit_matrix(stp_vars_t v)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  int i;
  for (i = 0; i < PHYSICAL_CHANNEL_COUNT(d); i++)
    stpi_dither_matrix_destroy(&(PHYSICAL_CHANNEL(d, i).dithermat));
  stpi_dither_matrix_destroy(&(d->dither_matrix));
}

static void
postinit_matrix(stp_vars_t v, int x_shear, int y_shear)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  unsigned rc = 1 + (unsigned) ceil(sqrt(PHYSICAL_CHANNEL_COUNT(d)));
  int i, j;
  int color = 0;
  unsigned x_n = d->dither_matrix.x_size / rc;
  unsigned y_n = d->dither_matrix.y_size / rc;
  if (x_shear || y_shear)
    stpi_dither_matrix_shear(&(d->dither_matrix), x_shear, y_shear);
  for (i = 0; i < rc; i++)
    for (j = 0; j < rc; j++)
      if (color < PHYSICAL_CHANNEL_COUNT(d))
	{
	  stpi_dither_matrix_clone(&(d->dither_matrix),
				  &(PHYSICAL_CHANNEL(d, color).dithermat),
				  x_n * i, y_n * j);
	  color++;
	}
  stpi_dither_set_transition(v, d->transition);
}

void
stpi_dither_set_iterated_matrix(stp_vars_t v, size_t edge, size_t iterations,
			       const unsigned *data, int prescaled,
			       int x_shear, int y_shear)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  preinit_matrix(v);
  stpi_dither_matrix_iterated_init(&(d->dither_matrix), edge, iterations, data);
  postinit_matrix(v, x_shear, y_shear);
}

void
stpi_dither_set_matrix(stp_vars_t v, const stpi_dither_matrix_t *matrix,
		      int transposed, int x_shear, int y_shear)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  int x = transposed ? matrix->y : matrix->x;
  int y = transposed ? matrix->x : matrix->y;
  preinit_matrix(v);
  if (matrix->bytes == 2)
    stpi_dither_matrix_init_short(&(d->dither_matrix), x, y,
				 (const unsigned short *) matrix->data,
				 transposed, matrix->prescaled);
  else if (matrix->bytes == 4)
    stpi_dither_matrix_init(&(d->dither_matrix), x, y,
			   (const unsigned *)matrix->data,
			   transposed, matrix->prescaled);
  postinit_matrix(v, x_shear, y_shear);
}

void
stpi_dither_set_matrix_from_dither_array(stp_vars_t v,
					 stp_const_array_t array,
					 int transpose)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  preinit_matrix(v);
  stpi_dither_matrix_init_from_dither_array(&(d->dither_matrix), array, transpose);
  postinit_matrix(v, 0, 0);
}

void
stpi_dither_set_transition(stp_vars_t v, double exponent)
{
  stpi_dither_t *d = (stpi_dither_t *) stpi_get_component_data(v, "Dither");
  unsigned rc = 1 + (unsigned) ceil(sqrt(PHYSICAL_CHANNEL_COUNT(d)));
  int i, j;
  int color = 0;
  unsigned x_n = d->dither_matrix.x_size / rc;
  unsigned y_n = d->dither_matrix.y_size / rc;
  for (i = 0; i < PHYSICAL_CHANNEL_COUNT(d); i++)
    stpi_dither_matrix_destroy(&(PHYSICAL_CHANNEL(d, i).pick));
  stpi_dither_matrix_destroy(&(d->transition_matrix));
  stpi_dither_matrix_copy(&(d->dither_matrix), &(d->transition_matrix));
  d->transition = exponent;
  if (exponent < .999 || exponent > 1.001)
    stpi_dither_matrix_scale_exponentially(&(d->transition_matrix), exponent);
  for (i = 0; i < rc; i++)
    for (j = 0; j < rc; j++)
      if (color < PHYSICAL_CHANNEL_COUNT(d))
	{
	  stpi_dither_matrix_clone(&(d->dither_matrix),
				  &(PHYSICAL_CHANNEL(d, color).pick),
				  x_n * i, y_n * j);
	  color++;
	}
  if (exponent < .999 || exponent > 1.001)
    for (i = 0; i < 65536; i++)
      {
	double dd = i / 65535.0;
	dd = pow(dd, 1.0 / exponent);
	d->virtual_dot_scale[i] = dd * 65535;
      }
  else
    for (i = 0; i < 65536; i++)
      d->virtual_dot_scale[i] = i;
}

static stpi_list_t *dither_matrix_cache = NULL;

typedef struct
{
  int x;
  int y;
  const char *filename;
} stpi_xml_dither_cache_t;

static const char *
stpi_xml_dither_cache_get(int x, int y)
{
  stpi_list_item_t *ln;

  stpi_xml_init();

  if (stpi_debug_level & STPI_DBG_XML)
    stpi_erprintf("stpi_xml_dither_cache_get: lookup %dx%d... ", x, y);

  ln = stpi_list_get_start(dither_matrix_cache);

  while (ln)
    {
      if (((stpi_xml_dither_cache_t *) stpi_list_item_get_data(ln))->x == x &&
	  ((stpi_xml_dither_cache_t *) stpi_list_item_get_data(ln))->y == y)
	{

	  if (stpi_debug_level & STPI_DBG_XML)
	    stpi_erprintf("found\n");

	  stpi_xml_exit();
	  return ((stpi_xml_dither_cache_t *) stpi_list_item_get_data(ln))->filename;
	}
      ln = stpi_list_item_next(ln);
    }
  if (stpi_debug_level & STPI_DBG_XML)
    stpi_erprintf("missing\n");

  stpi_xml_exit();

  return NULL;
}

static void
stpi_xml_dither_cache_set(int x, int y, const char *filename)
{
  stpi_xml_dither_cache_t *cacheval;

  assert(x && y && filename);

  stpi_xml_init();

  if (dither_matrix_cache == NULL)
    dither_matrix_cache = stpi_list_create();

  if (stpi_xml_dither_cache_get(x, y))
      /* Already cached for this x and y aspect */
    return;

  cacheval = stpi_malloc(sizeof(stpi_xml_dither_cache_t));
  cacheval->x = x;
  cacheval->y = y;
  cacheval->filename = stpi_strdup(filename);

  stpi_list_item_create(dither_matrix_cache, NULL, (void *) cacheval);

  if (stpi_debug_level & STPI_DBG_XML)
    stpi_erprintf("stpi_xml_dither_cache_set: added %dx%d\n", x, y);

  stpi_xml_exit();

  return;
}

/*
 * Parse the <dither-matrix> node.
 */
static int
stpi_xml_process_dither_matrix(xmlNodePtr dm,     /* The dither matrix node */
			       const char *file)  /* Source file */
			       
{
  xmlChar *value;
  int x = -1;
  int y = -1;


  value = xmlGetProp(dm, (const xmlChar *) "x-aspect");
  x = stpi_xmlstrtol(value);
  xmlFree(value);

  value = xmlGetProp(dm, (const xmlChar *) "y-aspect");
  y = stpi_xmlstrtol(value);
  xmlFree(value);

  if (stpi_debug_level & STPI_DBG_XML)
    stpi_erprintf("stpi_xml_process_dither_matrix: x=%d, y=%d\n", x, y);

  stpi_xml_dither_cache_set(x, y, file);
  return 1;
}

static stp_array_t
stpi_dither_array_create_from_xmltree(xmlNodePtr dm) /* Dither matrix node */
{
  xmlChar *stmp;
  xmlNodePtr child;
  stp_array_t ret = NULL;
  int x_aspect, y_aspect; /* Dither matrix size */

  /* Get x-size */
  stmp = xmlGetProp(dm, (const xmlChar *) "x-aspect");
  if (stmp)
    {
      x_aspect = (int) stpi_xmlstrtoul(stmp);
      xmlFree(stmp);
    }
  else
    {
      stpi_erprintf("stpi_dither_array_create_from_xmltree: \"x-aspect\" missing\n");
      goto error;
    }
  /* Get y-size */
  stmp = xmlGetProp(dm, (const xmlChar *) "y-aspect");
  if (stmp)
    {
      y_aspect = (int) stpi_xmlstrtoul(stmp);
      xmlFree(stmp);
    }
  else
    {
      stpi_erprintf("stpi_dither_array_create_from_xmltree: \"y-aspect\" missing\n");
      goto error;
    }

  /* Now read in the array */
  child = dm->children;
  while (child)
    {
      if (!xmlStrcmp(child->name, (const xmlChar *) "array"))
	{
	  ret = stpi_array_create_from_xmltree(child);
	  break;
	}
      child = child->next;
    }

  return ret;

 error:
  if (ret)
    stp_array_destroy(ret);
  return NULL;
}

static stp_array_t
xml_doc_get_dither_array(xmlDocPtr doc)
{
  xmlNodePtr cur;
  xmlNodePtr xmlseq;

  if (doc == NULL )
    {
      fprintf(stderr,"xml_doc_get_dither_array: XML file not parsed successfully.\n");
      return NULL;
    }

  cur = xmlDocGetRootElement(doc);

  if (cur == NULL)
    {
      fprintf(stderr,"xml_doc_get_dither_array: empty document\n");
      xmlFreeDoc(doc);
      return NULL;
    }

  xmlseq = stpi_xml_get_node(cur, "gimp-print", "dither-matrix", NULL);
  if (xmlseq == NULL )
    {
      fprintf(stderr,"xml-doc-get-dither-array: XML file is not a dither matrix.\n");
      return NULL;
    }

  return stpi_dither_array_create_from_xmltree(xmlseq);
}

static stp_array_t
stpi_dither_array_create_from_file(const char* file)
{
  xmlDocPtr doc;   /* libXML document pointer */
  stp_array_t ret;

  stpi_xml_init();

  if (stpi_debug_level & STPI_DBG_XML)
    stpi_erprintf("stpi_dither_array_create_from_file: reading `%s'...\n", file);

  doc = xmlParseFile(file);

  ret = xml_doc_get_dither_array(doc);

  if (doc)
    xmlFreeDoc(doc);

  stpi_xml_exit();

  return ret;
}

static stp_array_t
stpi_xml_get_dither_array(int x, int y)
{
  const char *file;
  stp_array_t ret;

  stpi_xml_init();

  file = stpi_xml_dither_cache_get(x, y);
  if (file == NULL)
    {
      stpi_xml_exit();
      return NULL;
    }

  ret = stpi_dither_array_create_from_file(file);

  stpi_xml_exit();
  return ret;
}

void
stpi_init_dither(void)
{
  stpi_register_xml_parser("dither-matrix", stpi_xml_process_dither_matrix);
}

stp_array_t
stpi_find_standard_dither_array(int x_aspect, int y_aspect)
{
  stp_array_t answer;
  int divisor = gcd(x_aspect, y_aspect);

  x_aspect /= divisor;
  y_aspect /= divisor;

  if (x_aspect == 3)		/* We don't have x3 matrices */
    x_aspect += 1;		/* so cheat */
  if (y_aspect == 3)
    y_aspect += 1;
  
  divisor = gcd(x_aspect, y_aspect);
  x_aspect /= divisor;
  y_aspect /= divisor;

  answer = stpi_xml_get_dither_array(x_aspect, y_aspect);
  if (answer)
    return answer;
  answer = stpi_xml_get_dither_array(y_aspect, x_aspect);
  if (answer)
    return answer;
  return NULL;
}
