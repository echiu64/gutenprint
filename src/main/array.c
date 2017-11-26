/*
 *
 *   Array data type.  This type is designed to be derived from by
 *   the curve and dither matrix types.
 *
 *   Copyright 2002-2003 Robert Krawitz (rlk@alum.mit.edu)
 *   Copyright 2003      Roger Leigh (rleigh@debian.org)
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <strings.h>


struct stp_array
{
  stp_sequence_t *data; /* First member, to allow typecasting to sequence. */
  int x_size;
  int y_size;
};

/*
 * We could do more sanity checks here if we want.
 */
#define CHECK_ARRAY(array) STPI_ASSERT(array != NULL, NULL)

static void array_ctor(stp_array_t *array)
{
  array->data = stp_sequence_create();
  stp_sequence_set_size(array->data, array->x_size * array->y_size);
}

stp_array_t *
stp_array_create(int x_size, int y_size)
{
  stp_array_t *ret;
  ret = stp_zalloc(sizeof(stp_array_t));
  ret->x_size = x_size;
  ret->y_size = y_size;
  ret->data = NULL;
  array_ctor(ret);
  return ret;
}


static void
array_dtor(stp_array_t *array)
{
  if (array->data)
    stp_sequence_destroy(array->data);
  memset(array, 0, sizeof(stp_array_t));
}

void
stp_array_destroy(stp_array_t *array)
{
  CHECK_ARRAY(array);
  array_dtor(array);
  stp_free(array);
}

void
stp_array_copy(stp_array_t *dest, const stp_array_t *source)
{
  CHECK_ARRAY(dest);
  CHECK_ARRAY(source);

  dest->x_size = source->x_size;
  dest->y_size = source->y_size;
  if (dest->data)
    stp_sequence_destroy(dest->data);
  dest->data = stp_sequence_create_copy(source->data);
}

stp_array_t *
stp_array_create_copy(const stp_array_t *array)
{
  stp_array_t *ret;
  CHECK_ARRAY(array);
  ret = stp_array_create(0, 0); /* gets freed next */
  stp_array_copy(ret, array);
  return ret;
}


void
stp_array_set_size(stp_array_t *array, int x_size, int y_size)
{
  CHECK_ARRAY(array);
  if (array->data) /* Free old data */
    stp_sequence_destroy(array->data);
  array->x_size = x_size;
  array->y_size = y_size;
  array->data = stp_sequence_create();
  stp_sequence_set_size(array->data, array->x_size * array->y_size);
}

void
stp_array_get_size(const stp_array_t *array, int *x_size, int *y_size)
{
  CHECK_ARRAY(array);
  *x_size = array->x_size;
  *y_size = array->y_size;
  return;
}

void
stp_array_set_data(stp_array_t *array, const double *data)
{
  CHECK_ARRAY(array);
  stp_sequence_set_data(array->data, array->x_size * array->y_size,
			data);
}

void
stp_array_get_data(const stp_array_t *array, size_t *size, const double **data)
{
  CHECK_ARRAY(array);
  stp_sequence_get_data(array->data, size, data);
}

int
stp_array_set_point(stp_array_t *array, int x, int y, double data)
{
  CHECK_ARRAY(array);

  if (((array->x_size * x) + y) >= (array->x_size * array->y_size))
    return 0;

  return stp_sequence_set_point(array->data, (array->x_size * x) + y, data);}

int
stp_array_get_point(const stp_array_t *array, int x, int y, double *data)
{
  CHECK_ARRAY(array);

  if (((array->x_size * x) + y) >= array->x_size * array->y_size)
    return 0;
  return stp_sequence_get_point(array->data,
				(array->x_size * x) + y, data);
}

const stp_sequence_t *
stp_array_get_sequence(const stp_array_t *array)
{
  CHECK_ARRAY(array);

  return array->data;
}

static stp_array_t *
xml_doc_get_array(stp_mxml_node_t *doc)
{
  stp_mxml_node_t *cur;
  stp_mxml_node_t *xmlarray;
  stp_array_t *array = NULL;

  if (doc == NULL )
    {
      stp_deprintf(STP_DBG_ARRAY_ERRORS,
		   "xml_doc_get_array: XML file not parsed successfully.\n");
      return NULL;
    }

  cur = doc->child;

  if (cur == NULL)
    {
      stp_deprintf(STP_DBG_ARRAY_ERRORS,
		   "xml_doc_get_array: empty document\n");
      return NULL;
    }

  xmlarray = stp_xml_get_node(cur, "gutenprint", "array", NULL);

  if (xmlarray)
    array = stp_array_create_from_xmltree(xmlarray);

  return array;
}

stp_array_t *
stp_array_create_from_file(const char* file)
{
  stp_array_t *array = NULL;
  stp_mxml_node_t *doc;
  FILE *fp = NULL;
  if (file[0] != '/' && strncmp(file, "./", 2) && strncmp(file, "../", 3))
    {
      char *fn = stp_path_find_file(NULL, file);
      if (fn)
	{
	  fp = fopen(file, "r");
	  free(fn);
	}
    }
  else if (file)
    {
      fp = fopen(file, "r");
    }
  if (!fp)
    {
      stp_deprintf(STP_DBG_ARRAY_ERRORS,
		   "stp_array_create_from_file: unable to open %s: %s\n",
		    file, strerror(errno));
      return NULL;
    }
  stp_deprintf(STP_DBG_XML, "stp_array_create_from_file: reading `%s'...\n",
	       file);

  stp_xml_init();

  doc = stp_mxmlLoadFile(NULL, fp, STP_MXML_NO_CALLBACK);

  array = xml_doc_get_array(doc);

  if (doc)
    stp_mxmlDelete(doc);

  stp_xml_exit();
  (void) fclose(fp);
  return array;

}

stp_array_t *
stp_array_create_from_xmltree(stp_mxml_node_t *array)  /* The array node */
{
  const char *stmp;                          /* Temporary string */
  stp_mxml_node_t *child;                       /* Child sequence node */
  int x_size, y_size;
  size_t count;
  stp_sequence_t *seq = NULL;
  stp_array_t *ret = NULL;

  /* FIXME Need protection against unlimited recursion */
  if ((stmp = stp_mxmlElementGetAttr(array, "src")) != NULL)
    return stp_array_create_from_file(stmp);
  stmp = stp_mxmlElementGetAttr(array, "x-size");
  if (stmp)
    {
      x_size = (int) strtoul(stmp, NULL, 0);
    }
  else
    {
      stp_erprintf("stp_array_create_from_xmltree: \"x-size\" missing\n");
      goto error;
    }
  /* Get y-size */
  stmp = stp_mxmlElementGetAttr(array, "y-size");
  if (stmp)
    {
      y_size = (int) strtoul(stmp, NULL, 0);
    }
  else
    {
      stp_erprintf("stp_array_create_from_xmltree: \"y-size\" missing\n");
      goto error;
    }

  /* Get the sequence data */

  child = stp_xml_get_node(array, "sequence", NULL);
  if (child)
    seq = stp_sequence_create_from_xmltree(child);

  if (seq == NULL)
    goto error;

  ret = stp_array_create(x_size, y_size);
  if (ret->data)
    stp_sequence_destroy(ret->data);
  ret->data = seq;

  count = stp_sequence_get_size(seq);
  if (count != (x_size * y_size))
    {
      stp_erprintf("stp_array_create_from_xmltree: size mismatch between array and sequence\n");
      goto error;
    }

  return ret;

 error:
  stp_erprintf("stp_array_create_from_xmltree: error during array read\n");
  if (ret)
    stp_array_destroy(ret);
  return NULL;
}

stp_mxml_node_t *
stp_xmltree_create_from_array(const stp_array_t *array)  /* The array */
{
  int x_size, y_size;
  char *xs, *ys;

  stp_mxml_node_t *arraynode = NULL;
  stp_mxml_node_t *child = NULL;

  stp_xml_init();

  /* Get array details */
  stp_array_get_size(array, &x_size, &y_size);

  /* Construct the allocated strings required */
  stp_asprintf(&xs, "%d", x_size);
  stp_asprintf(&ys, "%d", y_size);

  arraynode = stp_mxmlNewElement(NULL, "array");
  stp_mxmlElementSetAttr(arraynode, "x-size", xs);
  stp_mxmlElementSetAttr(arraynode, "y-size", ys);
  stp_free(xs);
  stp_free(ys);

  child = stp_xmltree_create_from_sequence(stp_array_get_sequence(array));

  if (child)
    stp_mxmlAdd(arraynode, STP_MXML_ADD_AFTER, NULL, child);
  else
    {
      stp_mxmlDelete(arraynode);
      arraynode = NULL;
    }

  stp_xml_exit();

  return arraynode;
}

static stp_mxml_node_t *
xmldoc_create_from_array(const stp_array_t *array)
{
  stp_mxml_node_t *xmldoc;
  stp_mxml_node_t *rootnode;
  stp_mxml_node_t *arraynode;

  /* Get array details */
  arraynode = stp_xmltree_create_from_array(array);
  if (arraynode == NULL)
    {
      stp_deprintf(STP_DBG_ARRAY_ERRORS,
		   "xmldoc_create_from_array: error creating array node\n");
      return NULL;
    }
  /* Create the XML tree */
  xmldoc = stp_xmldoc_create_generic();
  if (xmldoc == NULL)
    {
      stp_deprintf(STP_DBG_ARRAY_ERRORS,
		   "xmldoc_create_from_array: error creating XML document\n");
      return NULL;
    }
  rootnode = xmldoc->child;
  if (rootnode == NULL)
    {
      stp_mxmlDelete(xmldoc);
      stp_deprintf(STP_DBG_ARRAY_ERRORS,
		   "xmldoc_create_from_array: error getting XML document root node\n");
      return NULL;
    }

  stp_mxmlAdd(rootnode, STP_MXML_ADD_AFTER, NULL, arraynode);

  return xmldoc;
}

static int
array_whitespace_callback(stp_mxml_node_t *node, int where)
{
  if (node->type != STP_MXML_ELEMENT)
    return 0;
  if (strcasecmp(node->value.element.name, "gutenprint") == 0)
    {
      switch (where)
	{
	case STP_MXML_WS_AFTER_OPEN:
	case STP_MXML_WS_BEFORE_CLOSE:
	case STP_MXML_WS_AFTER_CLOSE:
	  return '\n';
	case STP_MXML_WS_BEFORE_OPEN:
	default:
	  return 0;
	}
    }
  else if (strcasecmp(node->value.element.name, "array") == 0)
    {
      switch (where)
	{
	case STP_MXML_WS_AFTER_OPEN:
	  return '\n';
	case STP_MXML_WS_BEFORE_CLOSE:
	case STP_MXML_WS_AFTER_CLOSE:
	case STP_MXML_WS_BEFORE_OPEN:
	default:
	  return 0;
	}
    }
  else if (strcasecmp(node->value.element.name, "sequence") == 0)
    {
      const char *count;
      switch (where)
	{
	case STP_MXML_WS_BEFORE_CLOSE:
	  count = stp_mxmlElementGetAttr(node, "count");
	  if (strcmp(count, "0") == 0)
	    return 0;
	  else
	    return '\n';
	case STP_MXML_WS_AFTER_OPEN:
	case STP_MXML_WS_AFTER_CLOSE:
	  return '\n';
	case STP_MXML_WS_BEFORE_OPEN:
	default:
	  return 0;
	}
    }
  else
    return 0;
}


int
stp_array_write(FILE *file, const stp_array_t *array)  /* The array */
{
  stp_mxml_node_t *xmldoc = NULL;

  stp_xml_init();

  xmldoc = xmldoc_create_from_array(array);
  if (xmldoc == NULL)
    {
      stp_xml_exit();
      return 1;
    }

  stp_mxmlSaveFile(xmldoc, file, array_whitespace_callback);

  if (xmldoc)
    stp_mxmlDelete(xmldoc);

  stp_xml_exit();

  return 0;
}

char *
stp_array_write_string(const stp_array_t *array)  /* The array */
{
  stp_mxml_node_t *xmldoc = NULL;
  char *retval;

  stp_xml_init();

  xmldoc = xmldoc_create_from_array(array);
  if (xmldoc == NULL)
    {
      stp_xml_exit();
      return NULL;
    }

  retval = stp_mxmlSaveAllocString(xmldoc, array_whitespace_callback);

  if (xmldoc)
    stp_mxmlDelete(xmldoc);

  stp_xml_exit();

  return retval;
}
