/*
 * "$Id$"
 *
 *   XML parser - process gimp-print XML data with libxml2.
 *
 *   Copyright 2002-2003 Roger Leigh (roger@whinlatter.uklinux.net)
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
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include "module.h"
#include "path.h"
#include "xml.h"
#if defined(HAVE_VARARGS_H) && !defined(HAVE_STDARG_H)
#include <varargs.h>
#else
#include <stdarg.h>
#endif

typedef struct
{
  char *name;
  stpi_xml_parse_func parse_func;
} stpi_xml_parse_registry;

static stpi_list_t stpi_xml_registry;

static const char *
xml_registry_namefunc(const void *item)
{
  const stpi_xml_parse_registry *xmlp = (const stpi_xml_parse_registry *) item;
  return xmlp->name;
}

static void
xml_registry_freefunc(void *item)
{
  stpi_xml_parse_registry *xmlp = (stpi_xml_parse_registry *) item;
  stpi_free(xmlp->name);
  stpi_free(xmlp);
}

void
stpi_register_xml_parser(const char *name, stpi_xml_parse_func parse_func)
{
  stpi_xml_parse_registry *xmlp;
  stpi_list_item_t *item = stpi_list_get_item_by_name(stpi_xml_registry, name);
  if (item)
    xmlp = (stpi_xml_parse_registry *) stpi_list_item_get_data(item);
  else
    {
      xmlp = stpi_malloc(sizeof(stpi_xml_parse_registry));
      xmlp->name = stpi_strdup(name);
      stpi_list_item_create(stpi_xml_registry, NULL, xmlp);
    }
  xmlp->parse_func = parse_func;
}

void
stpi_unregister_xml_parser(const char *name)
{
  stpi_list_item_t *item = stpi_list_get_item_by_name(stpi_xml_registry, name);
  if (item)
    stpi_list_item_destroy(stpi_xml_registry, item);
}

static void stpi_xml_process_gimpprint(xmlNodePtr gimpprint, const char *file);

/*static xmlFreeFunc xml_free_func = NULL;*/       /* libXML free function */
/*static xmlMallocFunc xml_malloc_func = NULL;*/   /* libXML malloc function */
/*static xmlReallocFunc xml_realloc_func = NULL;*/ /* libXML realloc function */
/*static xmlStrdupFunc xml_strdup_func = NULL;*/   /* libXML strdup function */
static char *saved_lc_collate;                 /* Saved LC_COLLATE */
static char *saved_lc_ctype;                   /* Saved LC_CTYPE */
static char *saved_lc_numeric;                 /* Saved LC_NUMERIC */
static int xml_is_initialised;                 /* Flag for init */

void
stpi_xml_preinit(void)
{
  static int xml_is_preinitialized = 0;
  if (!xml_is_preinitialized)
    {
      stpi_xml_registry = stpi_list_create();
      stpi_list_set_freefunc(stpi_xml_registry, xml_registry_freefunc);
      stpi_list_set_namefunc(stpi_xml_registry, xml_registry_namefunc);
    }
}    

/*
 * Call before using any of the static functions in this file.  All
 * public functions should call this before using any libXML
 * functions.
 */
void
stpi_xml_init(void)
{
  if (xml_is_initialised >= 1)
    {
      xml_is_initialised++;
      return;
    }

  xmlInitParser();
  /* Set some locale facets to "C" */
  saved_lc_collate = setlocale(LC_COLLATE, "C");
  saved_lc_ctype = setlocale(LC_CTYPE, "C");
  saved_lc_numeric = setlocale(LC_NUMERIC, "C");

  /* Use our memory allocation functions with libXML */
  /*  xmlMemGet (&xml_free_func,
	     &xml_malloc_func,
	     &xml_realloc_func,
	     &xml_strdup_func);
  xmlMemSetup (stpi_free,
	       stpi_malloc,
	       stpi_realloc,
	       stpi_strdup);*/
  xml_is_initialised = 1;
}

/*
 * Call after using any of the static functions in this file.  All
 * public functions should call this after using any libXML functions.
 */
void
stpi_xml_exit(void)
{
  if (xml_is_initialised > 1) /* don't restore original state */
    {
      xml_is_initialised--;
      return;
    }
  else if (xml_is_initialised < 1)
    return;
  xmlCleanupParser();

  /* Restore libXML memory functions to their previous state */
  /*  xmlMemSetup (xml_free_func,
	       xml_malloc_func,
	       xml_realloc_func,
	       xml_strdup_func);*/

  /* Restore locale */
  setlocale(LC_COLLATE, saved_lc_collate);
  setlocale(LC_CTYPE, saved_lc_ctype);
  setlocale(LC_NUMERIC, saved_lc_numeric);
  xml_is_initialised = 0;
}

/*
 * Make a list of available XML file names.
 */
static stpi_list_t *
stp_xml_file_list(void)
{
  stpi_list_t *dir_list;                  /* List of directories to scan */
  stpi_list_t *file_list;                 /* List of XML files */

  /* Make a list of all the available XML files */
  if (!(dir_list = stpi_list_create()))
    return NULL;
  stpi_list_set_freefunc(dir_list, stpi_list_node_free_data);
  stpi_path_split(dir_list, getenv("STP_DATA_PATH"));
  stpi_path_split(dir_list, PKGXMLDATADIR);
  file_list = stpi_path_search(dir_list, ".xml");
  stpi_list_destroy(dir_list);

  return file_list;
}

/*
 * Read all available XML files.
 */
int
stpi_xml_init_defaults(void)
{
  stpi_list_t *file_list;                 /* List of files to load */
  stpi_list_item_t *item;                 /* Pointer to current list item */

  stpi_xml_init();

  file_list = stp_xml_file_list();

  /* Parse each XML file */
  item = stpi_list_get_start(file_list);
  while (item)
    {
      if (stpi_debug_level & STPI_DBG_XML)
	stpi_erprintf("stp_xml_init_defaults: source file: %s\n",
		      (const char *) stpi_list_item_get_data(item));
      stpi_xml_parse_file((const char *) stpi_list_item_get_data(item));
      item = stpi_list_item_next(item);
    }
  stpi_list_destroy(file_list);

  stpi_xml_exit();

  return 0;
}


/*
 * Parse a single XML file.
 */
int
stpi_xml_parse_file(const char *file) /* File to parse */
{
  xmlDocPtr doc;   /* libXML document pointer */
  xmlNodePtr cur;  /* libXML node pointer */

  stpi_xml_init();

  if (stpi_debug_level & STPI_DBG_XML)
    stpi_erprintf("stp_xml_parse_file: reading  `%s'...\n", file);

  doc = xmlParseFile(file);

  if (doc == NULL )
    {
      stpi_erprintf("stp_xml_parse_file: %s: parse error\n", file);
      xmlFreeDoc(doc);
      return 1;
    }

  cur = xmlDocGetRootElement(doc);

  if (cur == NULL)
    {
      fprintf(stderr,"empty document\n");
      xmlFreeDoc(doc);
      return 1;
    }

  if (xmlStrcmp(cur->name, (const xmlChar *) "gimp-print"))
    {
      fprintf(stderr,"XML file of the wrong type, root node != gimp-print");
      xmlFreeDoc(doc);
      return 1;
    }

  /* The XML file was read and is the right format */

  stpi_xml_process_gimpprint(cur, file);
  xmlFreeDoc(doc);

  stpi_xml_exit();

  return 0;
}

/*
 * Convert a text string into an integer.
 */
long
stpi_xmlstrtol(xmlChar* textval)
{
  long val; /* The value to return */
  val = strtol((const char *) textval, (char **)NULL, 10);

  return val;
}

/*
 * Convert a text string into an unsigned int.
 */
unsigned long
stpi_xmlstrtoul(xmlChar* textval)
{
  unsigned long val; /* The value to return */
  val = strtoul((const char *) textval, (char **)NULL, 10);

  return val;
}

/*
 * Convert a text string into a double.
 */
double
stpi_xmlstrtod(xmlChar *textval)
{
  double val; /* The value to return */
  val = strtod((const char *) textval, (char **)NULL);

  return val;
}


/*
 * Find a node in an XML tree.  This function takes an xmlNodePtr,
 * followed by a NULL-terminated list of nodes which are required.
 * For example stpi_xml_get_node(myroot, "gimp-print", "dither") will
 * return the first dither node in the tree.  Additional dither nodes
 * cannot be accessed with this function.
 */
xmlNodePtr
stpi_xml_get_node(xmlNodePtr xmlroot, ...)
{
  xmlNodePtr child;
  xmlNodePtr retnode = NULL;
  va_list ap;
  const xmlChar *target = NULL;

  va_start(ap, xmlroot);

  child = xmlroot;
  target = va_arg(ap, const xmlChar *);

  while (child && child->name && target)
    {
      if (!xmlStrcmp(child->name, target))
	{
	  retnode = child;
	  child = child->children;
	  target = va_arg(ap, const xmlChar *);
	  continue;
	}
      child = child->next;
    }
  va_end(ap);
  return retnode;
}

static void
stpi_xml_process_node(xmlNodePtr node, const char *file)
{
  stpi_list_item_t *item =
    stpi_list_get_item_by_name(stpi_xml_registry, (const char *) node->name);
  if (item)
    {
      stpi_xml_parse_registry *xmlp =
	(stpi_xml_parse_registry *) stpi_list_item_get_data(item);
      (xmlp->parse_func)(node, file);
    }
}

/*
 * Parse the <gimp-print> root node.
 */
void
stpi_xml_process_gimpprint(xmlNodePtr cur, const char *file) /* The node to parse */
{
  xmlNodePtr child;                       /* Child node pointer */

  child = cur->children;
  while (child)
    {
      /* process nodes with corresponding parser */
      stpi_xml_process_node(child, file);
      child = child->next;
    }
}

/*
 * Create a basic gimp-print XML document tree root
 */
xmlDocPtr
stpi_xmldoc_create_generic(void)
{
  xmlDocPtr xmldoc;
  xmlNodePtr rootnode;

  /* Create the XML tree */
  xmldoc = xmlNewDoc((const xmlChar *) "1.0");

  rootnode = xmlNewNode(NULL, (const xmlChar *) "gimp-print");
  (void) xmlSetProp(rootnode, (const xmlChar *) "xmlns",
		    (const xmlChar *) "http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0");
  (void) xmlSetProp(rootnode, (const xmlChar *) "xmlns:xsi",
		    (const xmlChar *) "http://www.w3.org/2001/XMLSchema-instance");
  (void) xmlSetProp(rootnode, (const xmlChar *) "xsi:schemaLocation",
		    (const xmlChar *) "http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gimpprint.xsd");

  xmlDocSetRootElement(xmldoc, rootnode);

  return xmldoc;
}



