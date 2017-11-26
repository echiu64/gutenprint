/*
 *
 *   XML parser - process Gutenprint XML data with mxml.
 *
 *   Copyright 2002-2003 Roger Leigh (rleigh@debian.org)
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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#if defined(HAVE_VARARGS_H) && !defined(HAVE_STDARG_H)
#include <varargs.h>
#else
#include <stdarg.h>
#endif

typedef struct
{
  char *name;
  stp_xml_parse_func parse_func;
} stpi_xml_parse_registry;

static stp_list_t *stpi_xml_registry;

static stp_list_t *stpi_xml_preloads;

static stp_list_t *stpi_xml_files_loaded;

static stp_string_list_t *cached_xml_files;

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
  stp_free(xmlp->name);
  stp_free(xmlp);
}

static const char *
xml_preload_namefunc(const void *item)
{
  return (const char *) item;
}

static void
xml_preload_freefunc(void *item)
{
  stp_free(item);
}

void
stp_register_xml_parser(const char *name, stp_xml_parse_func parse_func)
{
  stpi_xml_parse_registry *xmlp;
  stp_list_item_t *item = stp_list_get_item_by_name(stpi_xml_registry, name);
  if (item)
    xmlp = (stpi_xml_parse_registry *) stp_list_item_get_data(item);
  else
    {
      xmlp = stp_malloc(sizeof(stpi_xml_parse_registry));
      xmlp->name = stp_strdup(name);
      stp_list_item_create(stpi_xml_registry, NULL, xmlp);
    }
  xmlp->parse_func = parse_func;
}

void
stp_unregister_xml_parser(const char *name)
{
  stp_list_item_t *item = stp_list_get_item_by_name(stpi_xml_registry, name);
  if (item)
    stp_list_item_destroy(stpi_xml_registry, item);
}

void
stp_register_xml_preload(const char *filename)
{
  stp_list_item_t *item = stp_list_get_item_by_name(stpi_xml_preloads, filename);
  if (!item)
    {
      char *the_filename = stp_strdup(filename);
      stp_list_item_create(stpi_xml_preloads, NULL, the_filename);
    }
}

void
stp_unregister_xml_preload(const char *name)
{
  stp_list_item_t *item = stp_list_get_item_by_name(stpi_xml_preloads, name);
  if (item)
    stp_list_item_destroy(stpi_xml_preloads, item);
}


static void stpi_xml_process_gutenprint(stp_mxml_node_t *gutenprint, const char *file);

static char *saved_locale;                 /* Saved LC_ALL */
static int xml_is_initialised;                 /* Flag for init */

void
stp_xml_preinit(void)
{
  if (! stpi_xml_registry)
    {
      stpi_xml_registry = stp_list_create();
      stp_list_set_freefunc(stpi_xml_registry, xml_registry_freefunc);
      stp_list_set_namefunc(stpi_xml_registry, xml_registry_namefunc);
    }
  if (! stpi_xml_preloads)
    {
      stpi_xml_preloads = stp_list_create();
      stp_list_set_freefunc(stpi_xml_preloads, xml_preload_freefunc);
      stp_list_set_namefunc(stpi_xml_preloads, xml_preload_namefunc);
    }
  if (! stpi_xml_files_loaded)
    {
      stpi_xml_files_loaded = stp_list_create();
      stp_list_set_freefunc(stpi_xml_files_loaded, xml_preload_freefunc);
      stp_list_set_namefunc(stpi_xml_files_loaded, xml_preload_namefunc);
    }
  if (! cached_xml_files)
    {
      cached_xml_files = stp_string_list_create();
    }
}

/*
 * Call before using any of the static functions in this file.  All
 * public functions should call this before using any mxml
 * functions.
 */
void
stp_xml_init(void)
{
  stp_deprintf(STP_DBG_XML, "stp_xml_init: entering at level %d\n",
	       xml_is_initialised);
  if (xml_is_initialised >= 1)
    {
      xml_is_initialised++;
      return;
    }

  /* Set some locale facets to "C" */
#ifdef HAVE_LOCALE_H
  saved_locale = stp_strdup(setlocale(LC_ALL, NULL));
  stp_deprintf(STP_DBG_XML, "stp_xml_init: saving locale %s\n", saved_locale);
  setlocale(LC_ALL, "C");
#endif

  xml_is_initialised = 1;
}

/*
 * Call after using any of the static functions in this file.  All
 * public functions should call this after using any mxml functions.
 */
void
stp_xml_exit(void)
{
  stp_deprintf(STP_DBG_XML, "stp_xml_exit: entering at level %d\n",
	       xml_is_initialised);
  if (xml_is_initialised > 1) /* don't restore original state */
    {
      xml_is_initialised--;
      return;
    }
  else if (xml_is_initialised < 1)
    {
      stp_erprintf("stp_xml_exit: unmatched stp_xml_init!\n");
      stp_abort();
    }

  /* Restore locale */
#ifdef HAVE_LOCALE_H
  stp_deprintf(STP_DBG_XML, "stp_xml_exit: restoring locale %s\n", saved_locale);
  setlocale(LC_ALL, saved_locale);
  stp_free(saved_locale);
  saved_locale = NULL;
#endif
  xml_is_initialised = 0;
}

void
stp_xml_parse_file_named(const char *name)
{
  stp_xml_preinit();
  stp_deprintf(STP_DBG_XML, "stp_xml_parse_file_named(%s)\n", name);
  if (! stp_list_get_item_by_name(stpi_xml_files_loaded, name))
    {
      char *file_name = stp_path_find_file(NULL, name);
      if (file_name)
	{
	  stp_xml_parse_file(file_name);
	  free(file_name);
	}
    }
}

/*
 * Read all available XML files.
 */
int
stp_xml_init_defaults(void)
{
  stp_list_item_t *item;                 /* Pointer to current list item */

  stp_xml_init();

  /* Parse each XML file */
  item = stp_list_get_start(stpi_xml_preloads);
  while (item)
    {
      stp_deprintf(STP_DBG_XML, "stp_xml_init_defaults: source file: %s\n",
		   (const char *) stp_list_item_get_data(item));
      stp_xml_parse_file_named((const char *) stp_list_item_get_data(item));
      item = stp_list_item_next(item);
    }
  stp_list_destroy(stpi_xml_preloads);

  stp_xml_exit();

  return 0;
}

/*
 * Parse a single XML file.
 */
int
stp_xml_parse_file(const char *file) /* File to parse */
{
  stp_mxml_node_t *doc;
  stp_mxml_node_t *cur;
  int status = 0;

  stp_deprintf(STP_DBG_XML, "stp_xml_parse_file: reading  `%s'...\n", file);

  stp_xml_init();

  doc = stp_mxmlLoadFromFile(NULL, file, STP_MXML_NO_CALLBACK);

  if ((cur = stp_xml_get_node(doc, "gutenprint", NULL)) == NULL)
    {
      stp_erprintf("stp_xml_parse_file: %s: parse error\n", file);
      status = 1;
    }
  else
    /* The XML file was read and is the right format */
    stpi_xml_process_gutenprint(cur, file);

  stp_mxmlDelete(doc);

  stp_xml_exit();

  return status;
}

static stp_mxml_node_t *
xml_try_parse_file_1(const char *pathname, const char *topnodename)
{
  stp_mxml_node_t *root =
    stp_mxmlLoadFromFile(NULL, pathname, STP_MXML_NO_CALLBACK);
  if (root)
    {
      stp_mxml_node_t *answer =
	stp_xml_get_node(root, "gutenprint", topnodename, NULL);
      if (answer)
	return answer;
      stp_mxmlDelete(root);
      return NULL;
    }
  else
    return NULL;
}

static stp_mxml_node_t *
xml_try_parse_file(const char *pathname, const char *topnodename)
{
  stp_xml_init();
  stp_mxml_node_t *answer = xml_try_parse_file_1(pathname, topnodename);
  stp_xml_exit();
  return answer;
}

static void
xml_cache_file(const char *name, const char *cache, stp_mxml_node_t *node)
{
  char *addr_string;
  stp_asprintf(&addr_string, "%p", (void *) node);
  /*
   * A given XML object should never be in multiple caches!  However,
   * it's possible that different nodes of the same file will be in different
   * caches.
   */
  STPI_ASSERT(!stp_string_list_is_present(cached_xml_files, addr_string), NULL);
  if (cache)
    {
      stp_refcache_add_item(cache, name, node);
      stp_string_list_add_string_unsafe(cached_xml_files, addr_string, cache);
    }
  else
    stp_string_list_add_string_unsafe(cached_xml_files, addr_string, "");
  stp_free(addr_string);
}

static stp_mxml_node_t *
xml_parse_file_from_path(const char *name, const char *topnodename,
			 const char *path, const char *cache)
{
  stp_mxml_node_t *answer = NULL;
  if (!(name[0] != '/' && strncmp(name, "./", 2) && strncmp(name, "../", 3)))
    answer = xml_try_parse_file(name, topnodename);
  else
    {
      stp_list_t *path_to_search;
      stp_list_item_t *item;
      if (path)
	path_to_search = stp_generate_path(path);
      else
	path_to_search = stp_data_path();
      item = stp_list_get_start(path_to_search);
      while (item)
	{
	  const char *dn = (const char *) stp_list_item_get_data(item);
	  char *ffn = stpi_path_merge(dn, name);
	  answer = xml_try_parse_file(ffn, topnodename);
	  stp_free(ffn);
	  if (answer)
	    break;
	  item = stp_list_item_next(item);
	}
      stp_list_destroy(path_to_search);
    }
  if (answer)
    xml_cache_file(name, cache, answer);
  return answer;
}

stp_mxml_node_t *
stp_xml_parse_file_from_path_uncached(const char *name, const char *topnodename,
				      const char *path)
{
  return xml_parse_file_from_path(name, topnodename, path, NULL);
}

stp_mxml_node_t *
stp_xml_parse_file_from_path_uncached_safe(const char *name,
					   const char *topnodename,
					   const char *path)
{
  stp_mxml_node_t *answer =
    xml_parse_file_from_path(name, topnodename, path, NULL);
  if (! answer)
    {
      stp_erprintf("Cannot find file %s of type %s\n", name, topnodename);
      stp_abort();
    }
  return answer;
}

stp_mxml_node_t *
stp_xml_parse_file_from_path(const char *name, const char *topnodename,
			     const char *path)
{
  char *cache;
  void *data;
  stp_asprintf(&cache, "%s_%s_%s", "xml_cache", topnodename,
	       path ? path : "DEFAULT");
  data = stp_refcache_find_item(cache, name);
  if (! data)
    data = xml_parse_file_from_path(name, topnodename, path, cache);
  stp_free(cache);
  return (stp_mxml_node_t *) data;
}

stp_mxml_node_t *
stp_xml_parse_file_from_path_safe(const char *name, const char *topnodename,
				  const char *path)
{
  stp_mxml_node_t *answer = stp_xml_parse_file_from_path(name, topnodename,
							 path);
  if (! answer)
    {
      stp_erprintf("FATAL: Cannot find file %s of type %s\n", name, topnodename);
      stp_abort();
    }
  return answer;
}

void
stp_xml_free_parsed_file(stp_mxml_node_t *node)
{
  char *addr_string;
  /* free(NULL) is legal and a no-op. */
  if (! node)
    return;
  stp_asprintf(&addr_string, "%p", (void *) node);
  stp_param_string_t *cache_entry =
    stp_string_list_find(cached_xml_files, addr_string);
  if (! cache_entry)
    {
      stp_erprintf("FATAL: Trying to free unrecorded node %s\n", addr_string);
      stp_abort();
    }
  if (cache_entry->text && cache_entry->text[0] != '\0')
    stp_refcache_remove_item(cache_entry->text, addr_string);
  stp_string_list_remove_string(cached_xml_files, addr_string);
  stp_free(addr_string);
  while (node->parent && node->parent != node)
    node = node->parent;
  stp_xml_init();
  stp_mxmlDelete(node);
  stp_xml_exit();
}

/*
 * Convert a text string into an integer.
 */
long
stp_xmlstrtol(const char *textval)
{
  long val; /* The value to return */
  val = strtol(textval, (char **)NULL, 0);

  return val;
}

/*
 * Convert a text string into an unsigned int.
 */
unsigned long
stp_xmlstrtoul(const char *textval)
{
  unsigned long val; /* The value to return */
  val = strtoul(textval, (char **)NULL, 0);

  return val;
}

/*
 * Convert a text string into a double.
 */
double
stp_xmlstrtod(const char *textval)
{
  double val; /* The value to return */
  val = strtod(textval, (char **)NULL);

  return val;
}

/*
 * Convert a text string into a dimension.
 */
stp_dimension_t
stp_xmlstrtodim(const char *textval)
{
  double val; /* The value to return */
  val = (stp_dimension_t) strtod(textval, (char **)NULL);

  return val;
}

/*
 * Convert an encoded text string into a raw.
 */
stp_raw_t *
stp_xmlstrtoraw(const char *textval)
{
  size_t tcount;
  stp_raw_t *raw;
  unsigned char *answer;
  unsigned char *aptr;
  if (! textval || *textval == 0)
    return NULL;
  tcount = strlen(textval);
  raw = stp_zalloc(sizeof(stp_raw_t));
  answer = stp_malloc(tcount + 1); /* Worst case -- we may not need it all */
  aptr = answer;
  raw->data = answer;
  while (*textval)
    {
      if (*textval != '\\')
	{
	  *aptr++ = *textval++;
	  raw->bytes++;
	}
      else
	{
	  textval++;
	  if (textval[0] >= '0' && textval[0] <= '3' &&
	      textval[1] >= '0' && textval[1] <= '7' &&
	      textval[2] >= '0' && textval[2] <= '7')
	    {
	      *aptr++ = (((textval[0] - '0') << 6) +
			 ((textval[1] - '0') << 3) +
			 ((textval[2] - '0') << 0));
	      raw->bytes++;
	      textval += 3;
	    }
	  else if (textval[0] == '\0' || textval[1] == '\0' || textval[2] == '\0')
	    break;
	  else
	    textval += 3;
	}
    }
  *aptr = '\0';
  return raw;
}

char *
stp_rawtoxmlstr(const stp_raw_t *raw)
{
  if (raw && raw->bytes > 0)
    {
      int i;
      const unsigned char *data = (const unsigned char *) (raw->data);
      char *answer = stp_malloc((raw->bytes * 4) + 1); /* \012 */
      unsigned char *aptr = (unsigned char *) answer;
      for (i = 0; i < raw->bytes; i++)
	{
	  if (data[i] > ' ' && data[i] < '\177' && data[i] != '\\' &&
	      data[i] != '<' && data[i] != '>' && data[i] != '&')
	    *aptr++ = data[i];
	  else
	    {
	      *aptr++ = '\\';
	      *aptr++ = '0' + ((data[i] & '\300') >> 6);
	      *aptr++ = '0' + ((data[i] & '\070') >> 3);
	      *aptr++ = '0' + ((data[i] & '\007') >> 0);
	    }
	}
      *aptr = '\0';
      return answer;
    }
  return NULL;
}

char *
stp_strtoxmlstr(const char *str)
{
  if (str && strlen(str) > 0)
    {
      int i;
      int bytes = strlen(str);
      const unsigned char *data = (const unsigned char *) (str);
      char *answer = stp_malloc((bytes * 4) + 1); /* "\012" is worst case */
      unsigned char *aptr = (unsigned char *) answer;
      for (i = 0; i < bytes; i++)
	{
	  if (data[i] > ' ' && data[i] < '\177' && data[i] != '\\' &&
	      data[i] != '<' && data[i] != '>' && data[i] != '&')
	    *aptr++ = data[i];
	  else
	    {
	      *aptr++ = '\\';
	      *aptr++ = '0' + ((data[i] & '\300') >> 6);
	      *aptr++ = '0' + ((data[i] & '\070') >> 3);
	      *aptr++ = '0' + ((data[i] & '\007') >> 0);
	    }
	}
      *aptr = '\0';
      return answer;
    }
  return NULL;
}

void
stp_prtraw(const stp_raw_t *raw, FILE *fp)
{
  if (raw && raw->bytes > 0)
    {
      int i;
      const unsigned char *data = (const unsigned char *) (raw->data);
      for (i = 0; i < raw->bytes; i++)
	{
	  if (data[i] > ' ' && data[i] < '\177' && data[i] != '\\' &&
	      data[i] != '<' && data[i] != '>' && data[i] != '&')
	    fputc(data[i], fp);
	  else
	    {
	      fputc('\\', fp);
	      fputc('0' + ((data[i] & '\300') >> 6), fp);
	      fputc('0' + ((data[i] & '\070') >> 3), fp);
	      fputc('0' + ((data[i] & '\007') >> 0), fp);
	    }
	}
    }
}


/*
 * Find a node in an XML tree.  This function takes an xmlNodePtr,
 * followed by a NULL-terminated list of nodes which are required.
 * For example stp_xml_get_node(myroot, "gutenprint", "dither") will
 * return the first dither node in the tree.  Additional dither nodes
 * cannot be accessed with this function.
 */
stp_mxml_node_t *
stp_xml_get_node(stp_mxml_node_t *xmlroot, ...)
{
  stp_mxml_node_t *child;
  va_list ap;
  const char *target = NULL;

  va_start(ap, xmlroot);

  child = xmlroot;
  target = va_arg(ap, const char *);

  stp_xml_init();
  while (target && child)
    {
      child = stp_mxmlFindElement(child, child, target, NULL, NULL,
				  STP_MXML_DESCEND);
      target = va_arg(ap, const char *);
    }
  stp_xml_exit();
  va_end(ap);
  return child;
}

static void
stpi_xml_process_node(stp_mxml_node_t *node, const char *file)
{
  stp_list_item_t *item =
    stp_list_get_item_by_name(stpi_xml_registry, node->value.element.name);
  if (item)
    {
      stpi_xml_parse_registry *xmlp =
	(stpi_xml_parse_registry *) stp_list_item_get_data(item);
      (xmlp->parse_func)(node, file);
    }
}

/*
 * Parse the <gutenprint> root node.
 */
static void
stpi_xml_process_gutenprint(stp_mxml_node_t *cur, const char *file) /* The node to parse */
{
  stp_mxml_node_t *child;                       /* Child node pointer */

  child = cur->child;
  while (child)
    {
      /* process nodes with corresponding parser */
      if (child->type == STP_MXML_ELEMENT)
	stpi_xml_process_node(child, file);
      child = child->next;
    }
}

/*
 * Create a basic gutenprint XML document tree root
 */
stp_mxml_node_t *
stp_xmldoc_create_generic(void)
{
  stp_mxml_node_t *doc;
  stp_mxml_node_t *rootnode;

  stp_xml_init();
  /* Create the XML tree */
  doc = stp_mxmlNewElement(NULL, "?xml");
  stp_mxmlElementSetAttr(doc, "version", "1.0");

  rootnode = stp_mxmlNewElement(doc, "gutenprint");
  stp_mxmlElementSetAttr
    (rootnode, "xmlns", "http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0");
  stp_mxmlElementSetAttr
    (rootnode, "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
  stp_mxmlElementSetAttr
    (rootnode, "xsi:schemaLocation",
     "http://gimp-print.sourceforge.net/xsd/gp.xsd-1.0 gutenprint.xsd");
  stp_xml_exit();

  return doc;
}

void
stpi_print_xml_node(stp_mxml_node_t *node)
{
  int i;
  stp_erprintf("Node @%p:\n", (void *) node);
  stp_erprintf("    Type %d\n", node->type);
  stp_erprintf("    Next @%p\n", (void *) node->next);
  stp_erprintf("    Prev @%p\n", (void *) node->prev);
  stp_erprintf("    Parent @%p\n", (void *) node->parent);
  stp_erprintf("    Child @%p\n", (void *) node->child);
  stp_erprintf("    Last @%p\n", (void *) node->last_child);
  stp_erprintf("    Value: ");
  switch (node->type)
    {
    case STP_MXML_ELEMENT:
      stp_erprintf("\n        Element, name: %s\n", node->value.element.name);
      stp_erprintf("        Attrs: %d\n", node->value.element.num_attrs);
      for (i = 0; i < node->value.element.num_attrs; i++)
	stp_erprintf("            %s    =>    %s\n",
		     node->value.element.attrs[i].name,
		     node->value.element.attrs[i].value);
      break;
    case STP_MXML_INTEGER:
      stp_erprintf(" Integer:    %d\n", node->value.integer);
      break;
    case STP_MXML_REAL:
      stp_erprintf(" Real:       %f\n", node->value.real);
      break;
    case STP_MXML_DIMENSION:
      stp_erprintf(" Dimension:  %f\n", node->value.real);
      break;
    case STP_MXML_OPAQUE:
      stp_erprintf(" Opaque:    '%s'\n", node->value.opaque);
      break;
    case STP_MXML_TEXT:
      stp_erprintf(" Text:       %d '%s'\n", node->value.text.whitespace,
		   node->value.text.string);
      break;
    default:
      stp_erprintf("UNKNOWN!\n");
    }
}
