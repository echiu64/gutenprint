/*
 * "$Id$"
 *
 *   XML parser - process gimp-print XML data with libxml2.
 *
 *   Copyright 1997-2002 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and Michael Natterer (mitch@gimp.org)
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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <libxml/globals.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlIO.h>
#include <libxml/encoding.h>
#include "array.h"
#include "curve.h"
#include "dither.h"
#include "list.h"
#include "module.h"
#include "papers.h"
#include "path.h"
#include "printers.h"
#include "vars.h"
#include "xml.h"


static int xmlstrtol(xmlChar *textval);
static unsigned xmlstrtoul(xmlChar *textval);
static double xmlstrtod(xmlChar *textval);
static void stpi_xml_process_gimpprint(const char *file, xmlNodePtr gimpprint);
static void stpi_xml_process_printdef(xmlNodePtr printdef);
static void stpi_xml_process_family(xmlNodePtr family);
static stpi_internal_printer_t *stp_printer_create_from_xmltree(xmlNodePtr printer, xmlChar *family, const stpi_printfuncs_t *printfuncs);
static void stpi_xml_process_paperdef(xmlNodePtr paperdef);
static void stpi_xml_process_dither_matrix(const char *file, xmlNodePtr dm);
static stp_papersize_t *stpi_xml_process_paper(xmlNodePtr paper);

static void stpi_xml_dither_cache_set(int x, int y, const char *filename);
static const char *stpi_xml_dither_cache_get(int x, int y);
stp_curve_t stp_curve_create_from_xmltree(xmlNodePtr curve);
static xmlNodePtr stp_xml_get_node(xmlNodePtr xmlroot, ...);
static xmlDocPtr stpi_xmldoc_create_generic(void);
static stp_array_t stpi_array_create_from_xmltree(xmlNodePtr array);


/*static xmlFreeFunc xml_free_func = NULL;*/       /* libXML free function */
/*static xmlMallocFunc xml_malloc_func = NULL;*/   /* libXML malloc function */
/*static xmlReallocFunc xml_realloc_func = NULL;*/ /* libXML realloc function */
/*static xmlStrdupFunc xml_strdup_func = NULL;*/   /* libXML strdup function */
static char *saved_lc_collate;                 /* Saved LC_COLLATE */
static char *saved_lc_ctype;                   /* Saved LC_CTYPE */
static char *saved_lc_numeric;                 /* Saved LC_NUMERIC */
static int xml_is_initialised;                 /* Flag for init */

/*
 * Call before using any of the static functions in this file.  All
 * public functions should call this before using any libXML
 * functions.
 */
static void stpi_xml_init(void)
{
  if (xml_is_initialised >= 1)
    {
      xml_is_initialised++;
      return;
    }

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
static void stpi_xml_exit(void)
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
int stpi_xml_init_defaults(void)
{
  stpi_list_t *file_list;                 /* List of files to load */
  stpi_list_item_t *item;                 /* Pointer to current list item */

  xmlInitParser();
  stpi_xml_init();

  file_list = stp_xml_file_list();

  /* Parse each XML file */
  item = stpi_list_get_start(file_list);
  while (item)
    {
      if (stpi_debug_level & STPI_DBG_XML)
	stpi_erprintf("stp-xml: source file: %s\n",
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
    stpi_erprintf("stp-xml-parse: reading  `%s'...\n", file);

  doc = xmlParseFile(file);

  if (doc == NULL )
    {
      fprintf(stderr,"XML file not parsed successfully. \n");
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

  stpi_xml_process_gimpprint(file, cur);
  xmlFreeDoc(doc);

  stpi_xml_exit();

  return 0;
}


/*
 * Convert a text string into an integer.
 */
static int
xmlstrtol(xmlChar* textval)
{
  int val; /* The value to return */
  val = strtol((const char *) textval, (char **)NULL, 10);

  return val;
}


/*
 * Convert a text string into an unsigned int.
 */
static unsigned
xmlstrtoul(xmlChar* textval)
{
  int val; /* The value to return */
  val = strtoul((const char *) textval, (char **)NULL, 10);

  return val;
}

/*
 * Convert a text string into a double.
 */
static double
xmlstrtod(xmlChar *textval)
{
  double val; /* The value to return */
  val = strtod((const char *) textval, (char **)NULL);

  return val;
}


/*
 * Find a node in an XML tree.  This function takes an xmlNodePtr,
 * followed by a NULL-terminated list of nodes which are required.
 * For example stp_xml_get_node(myroot, "gimp-print", "dither") will
 * return the first dither node in the tree.  Additional dither nodes
 * cannot be accessed with this function.
 */
static xmlNodePtr
stp_xml_get_node(xmlNodePtr xmlroot, ...)
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


/*
 * Parse the <gimp-print> root node.
 */
static void
stpi_xml_process_gimpprint(const char *file, xmlNodePtr cur) /* The node to parse */
{
  xmlNodePtr child;                       /* Child node pointer */

  child = cur->children;
  while (child)
    {
      /* process nodes with corresponding parser */
      if (!xmlStrcmp(child->name, (const xmlChar *) "printdef"))
	stpi_xml_process_printdef(child);
      else if (!xmlStrcmp(child->name, (const xmlChar *) "paperdef"))
	stpi_xml_process_paperdef(child);
      else if (!xmlStrcmp(child->name, (const xmlChar *) "dither-matrix"))
	stpi_xml_process_dither_matrix(file, child);
      child = child->next;
    }
}


/*
 * Parse the <printdef> node.
 */
static void
stpi_xml_process_printdef(xmlNodePtr printdef) /* The printdef node */
{
  xmlNodePtr family;                          /* Family child node */

  family = printdef->children;
  while (family)
    {
      if (!xmlStrcmp(family->name, (const xmlChar *) "family"))
	{
	  stpi_xml_process_family(family);
	}
      family = family->next;
    }
}


/*
 * Parse the <family> node.
 */
static void
stpi_xml_process_family(xmlNodePtr family)     /* The family node */
{
  stpi_list_t *family_module_list = NULL;      /* List of valid families */
  stpi_list_item_t *family_module_item;        /* Current family */
  xmlChar *family_name;                       /* Name of family */
  xmlNodePtr printer;                         /* printer child node */
  stpi_module_t *family_module_data;           /* Family module data */
  stpi_internal_family_t *family_data = NULL;  /* Family data */
  int family_valid = 0;                       /* Is family valid? */
  stpi_internal_printer_t *outprinter;         /* Generated printer */

  family_module_list = stpi_module_get_class(STPI_MODULE_CLASS_FAMILY);
  if (!family_module_list)
    return;


  family_name = xmlGetProp(family, (const xmlChar *) "name");
  family_module_item = stpi_list_get_start(family_module_list);
  while (family_module_item)
    {
      family_module_data = (stpi_module_t *)
	stpi_list_item_get_data(family_module_item);
      if (!xmlStrcmp(family_name, (const xmlChar *)
		     family_module_data->name))
	{
	  if (stpi_debug_level & STPI_DBG_XML)
	    stpi_erprintf("xml-family: family module: %s\n",
			  family_module_data->name);
	  family_data = family_module_data->syms;
	  if (family_data->printer_list == NULL)
	    family_data->printer_list = stpi_list_create();
	  family_valid = 1;
	}
	  family_module_item = stpi_list_item_next(family_module_item);
    }

  printer = family->children;
  while (family_valid && printer)
    {
      if (!xmlStrcmp(printer->name, (const xmlChar *) "printer"))
	{
	  outprinter = stp_printer_create_from_xmltree(printer, family_name,
					       family_data->printfuncs);
	  if (outprinter)
	    stpi_list_item_create(family_data->printer_list, NULL, outprinter);
	}
      printer = printer->next;
    }

  stpi_list_destroy(family_module_list);
  xmlFree (family_name);
  return;
}

typedef struct
{
  const char *property;
  const char *parameter;
} stpi_xml_prop_t;

static const stpi_xml_prop_t stpi_xml_props[] =
{
  { "black", "Black" },
  { "cyan", "Cyan" },
  { "yellow", "Yellow" },
  { "magenta", "Magenta" },
  { "brightness", "Brightness" },
  { "gamma", "Gamma" },
  { "density", "Density" },
  { "saturation", "Saturation" },
  { "blackdensity", "BlackDensity" },
  { "cyandensity", "CyanDensity" },
  { "yellowdensity", "YellowDensity" },
  { "magentadensity", "MagentaDensity" },
  { "gcrlower", "GCRLower" },
  { "gcrupper", "GCRupper" },
  { NULL, NULL }
};


/*
 * Parse the printer node, and return the generated printer.  Returns
 * NULL on failure.
 */
static stpi_internal_printer_t*
stp_printer_create_from_xmltree(xmlNodePtr printer, /* The printer node */
				xmlChar *family,    /* Family name */
				const stpi_printfuncs_t *printfuncs)
                                                    /* Family printfuncs */
{
  xmlNodePtr prop;                                  /* Temporary node pointer */
  xmlChar *stmp;                                    /* Temporary string */
 /* props[] (unused) is the correct tag sequence */
  /*  const char *props[] =
    {
      "color",
      "model",
      "black",
      "cyan",
      "yellow",
      "magenta",
      "brightness",
      "gamma",
      "density",
      "saturation",
      "blackgamma",
      "cyangamma",
      "yellowgamma",
      "magentagamma",
      "gcrlower",
      "gcrupper",
      NULL
      };*/
  stpi_internal_printer_t *outprinter;                 /* Generated printer */
  int
    driver = 0,                                       /* Check driver */
    long_name = 0,                                    /* Check long_name */
    color = 0,                                        /* Check color */
    model = 0;                                        /* Check model */

  outprinter = stpi_malloc(sizeof(stpi_internal_printer_t));
  if (!outprinter)
    return NULL;
  outprinter->printvars = stp_vars_create();
  if (outprinter->printvars == NULL)
    {
      stpi_free(outprinter);
      return NULL;
    }

  outprinter->cookie = COOKIE_PRINTER;

  stmp = xmlGetProp(printer, (const xmlChar *) "driver");
  stp_set_driver(outprinter->printvars, (const char *) stmp);
  xmlFree(stmp);
    
  outprinter->long_name =
    (char *) xmlGetProp(printer, (const xmlChar *) "name");
  outprinter->family = stpi_strdup((const char *) family);

  if (stp_get_driver(outprinter->printvars))
    driver = 1;
  if (outprinter->long_name)
    long_name = 1;

  outprinter->printfuncs = printfuncs;

  prop = printer->children;
  while (prop)
    {
      if (!xmlStrcmp(prop->name, (const xmlChar *) "color"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  if (stmp)
	    {
	      if (!xmlStrcmp(stmp, (const xmlChar *) "true"))
		stp_set_output_type(outprinter->printvars, OUTPUT_COLOR);
	      else
		stp_set_output_type(outprinter->printvars, OUTPUT_GRAY);
	      color = 1;
	      xmlFree(stmp);
	    }
	}
      else if (!xmlStrcmp(prop->name, (const xmlChar *) "model"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  if (stmp)
	    {
	      outprinter->model = xmlstrtol(stmp);
	      model = 1;
	      xmlFree(stmp);
	    }
	}
      else
	{
	  const stpi_xml_prop_t *stp_prop = stpi_xml_props;
	  while (stp_prop->property)
	    {
	      if (!xmlStrcmp(prop->name, (const xmlChar *) stp_prop->property))
		{
		  stmp = xmlGetProp(prop, (const xmlChar *) "value");
		  if (stmp)
		    {
		      stp_set_float_parameter(outprinter->printvars,
					      stp_prop->parameter,
					      (float) xmlstrtod(stmp));
		      xmlFree(stmp);
		      break;
		    }
		}
	      stp_prop++;
	    }
	}
      prop = prop->next;
    }
  if (driver && long_name && color && model && printfuncs)
    {
      if (stpi_debug_level & STPI_DBG_XML)
	{
	  stmp = xmlGetProp(printer, (const xmlChar*) "driver");
	  stpi_erprintf("xml-family: printer: %s\n", stmp);
	  xmlFree(stmp);
	}
      return outprinter;
    }
  stpi_free(outprinter);
  return NULL;
}


/*
 * Parse the <paperdef> node.
 */
static void
stpi_xml_process_paperdef(xmlNodePtr paperdef) /* The paperdef node */
{
  xmlNodePtr paper;                           /* paper node pointer */
  stp_papersize_t *outpaper;         /* Generated paper */

  paper = paperdef->children;
  while (paper)
    {
      if (!xmlStrcmp(paper->name, (const xmlChar *) "paper"))
	{
	  outpaper = stpi_xml_process_paper(paper);
	  if (outpaper)
	    stpi_paper_create(outpaper);
	}
      paper = paper->next;
    }
}


/*
 * Process the <paper> node.
 */
static stp_papersize_t *
stpi_xml_process_paper(xmlNodePtr paper) /* The paper node */
{
  xmlNodePtr prop;                              /* Temporary node pointer */
  xmlChar *stmp;                                /* Temporary string */
  /* props[] (unused) is the correct tag sequence */
  /*  const char *props[] =
    {
      "name",
      "description",
      "width",
      "height",
      "left",
      "right",
      "bottom",
      "top",
      "unit",
      NULL
      };*/
  stp_papersize_t *outpaper;   /* Generated paper */
  int
    id = 0,			/* Check id is present */
    name = 0,			/* Check name is present */
    height = 0,			/* Check height is present */
    width = 0,			/* Check width is present */
    left = 0,			/* Check left is present */
    right = 0,			/* Check right is present */
    bottom = 0,			/* Check bottom is present */
    top = 0,			/* Check top is present */
    unit = 0;			/* Check unit is present */

  if (stpi_debug_level & STPI_DBG_XML)
    {
      stmp = xmlGetProp(paper, (const xmlChar*) "name");
      stpi_erprintf("xml-paper: name: %s\n", stmp);
      xmlFree(stmp);
    }

  outpaper = stpi_malloc(sizeof(stp_papersize_t));
  if (!outpaper)
    return NULL;

  outpaper->name =
    (char *) xmlGetProp(paper, (const xmlChar *) "name");

  outpaper->top = 0;
  outpaper->left = 0;
  outpaper->bottom = 0;
  outpaper->right = 0;
  if (outpaper->name)
    id = 1;

  prop = paper->children;
  while(prop)
    {
      if (!xmlStrcmp(prop->name, (const xmlChar *) "description"))
	{
	  outpaper->text = (char *)
	    xmlGetProp(prop, (const xmlChar *) "value");
	  name = 1;
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "comment"))
	{
	  outpaper->comment = (char *)
	    xmlGetProp(prop, (const xmlChar *) "value");
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "width"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  if (stmp)
	    {
	      outpaper->width = xmlstrtoul(stmp);
	      xmlFree(stmp);
	      width = 1;
	    }
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "height"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  if (stmp)
	    {
	      outpaper->height = xmlstrtoul(stmp);
	      xmlFree(stmp);
	      height = 1;
	    }
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "left"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  outpaper->left = xmlstrtoul(stmp);
	  xmlFree(stmp);
	  left = 1;
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "right"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  outpaper->right = xmlstrtoul(stmp);
	  xmlFree(stmp);
	  right = 1;
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "bottom"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  outpaper->bottom = xmlstrtoul(stmp);
	  xmlFree(stmp);
	  bottom = 1;
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "top"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  outpaper->top = xmlstrtoul(stmp);
	  xmlFree(stmp);
	  top = 1;
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "unit"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  if (stmp)
	    {
	      if (!xmlStrcmp(stmp, (const xmlChar *) "english"))
		outpaper->paper_unit = PAPERSIZE_ENGLISH_STANDARD;
	      else if (!xmlStrcmp(stmp, (const xmlChar *) "english-extended"))
		outpaper->paper_unit = PAPERSIZE_ENGLISH_EXTENDED;
	      else if (!xmlStrcmp(stmp, (const xmlChar *) "metric"))
		outpaper->paper_unit = PAPERSIZE_METRIC_STANDARD;
	      else if (!xmlStrcmp(stmp, (const xmlChar *) "metric-extended"))
		outpaper->paper_unit = PAPERSIZE_METRIC_EXTENDED;
	      /* Default unit */
	      else
		outpaper->paper_unit = PAPERSIZE_METRIC_EXTENDED;
	      xmlFree(stmp);
	      unit = 1;
	    }
	}

      prop = prop->next;
    }
  if (id && name && width && height && unit) /* Margins are optional */
    return outpaper;
  stpi_free(outpaper);
  outpaper = NULL;
  return NULL;
}


/*
 * Parse the <dither-matrix> node.
 */
static void
stpi_xml_process_dither_matrix(const char *file,  /* Source file */
			       xmlNodePtr dm)     /* The dither matrix node */
{
  xmlChar *value;
  int x = -1;
  int y = -1;


  value = xmlGetProp(dm, (const xmlChar *) "x-aspect");
  x = xmlstrtol(value);
  xmlFree(value);

  value = xmlGetProp(dm, (const xmlChar *) "y-aspect");
  y = xmlstrtol(value);
  xmlFree(value);

  if (stpi_debug_level & STPI_DBG_XML)
    fprintf(stderr, "dither-matrix: x=%d, y=%d\n", x, y);

  stpi_xml_dither_cache_set(x, y, file);

}

static stpi_list_t *dither_matrix_cache = NULL;

typedef struct
{
  int x;
  int y;
  const char *filename;
} stpi_xml_dither_cache_t;

void
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
    fprintf(stderr, "xml-dither-cache: added %dx%d\n", x, y);

  stpi_xml_exit();

  return;
}

const char *
stpi_xml_dither_cache_get(int x, int y)
{
  stpi_list_item_t *ln;

  stpi_xml_init();

  if (stpi_debug_level & STPI_DBG_XML)
    fprintf(stderr, "xml-dither-cache: lookup %dx%d... ", x, y);

  ln = stpi_list_get_start(dither_matrix_cache);

  while (ln)
    {
      if (((stpi_xml_dither_cache_t *) stpi_list_item_get_data(ln))->x == x &&
	  ((stpi_xml_dither_cache_t *) stpi_list_item_get_data(ln))->y == y)
	{

	  if (stpi_debug_level & STPI_DBG_XML)
	    fprintf(stderr, "found\n");

	  stpi_xml_exit();
	  return ((stpi_xml_dither_cache_t *) stpi_list_item_get_data(ln))->filename;
	}
      ln = stpi_list_item_next(ln);
    }
  if (stpi_debug_level & STPI_DBG_XML)
    fprintf(stderr, "missing\n");

  stpi_xml_exit();

  return NULL;
}

stp_array_t
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

static stp_sequence_t
stp_sequence_create_from_xmltree(xmlNodePtr da, size_t extra_points)
{
  xmlChar *stmp;
  stp_sequence_t ret = NULL;
  size_t point_count;
  double low, high;
  xmlChar buf[100];
  xmlChar *bufptr = &buf[0];
  int i, j;
  int offset = 0;
  int maxlen;
  int found;


  ret = stp_sequence_create();

  /* Get curve point count */
  stmp = xmlGetProp(da, (const xmlChar *) "count");
  if (stmp)
    {
      point_count = (size_t) xmlstrtoul(stmp);
      /*      if ((xmlstrtol(stmp)) < 0)
	{
	  fprintf(stderr, "stp-sequence-create: \"count\" is less than zero\n");
	  goto error;
	  }*/
      xmlFree(stmp);
    }
  else
    {
      fprintf(stderr, "stp-sequence-create: \"count\" missing\n");
      goto error;
    }
  /* Get lower bound */
  stmp = xmlGetProp(da, (const xmlChar *) "lower-bound");
  if (stmp)
    {
      low = xmlstrtod(stmp);
      xmlFree(stmp);
    }
  else
    {
      fprintf(stderr, "stp-sequence-create: \"lower-bound\" missing\n");
      goto error;
    }
  /* Get upper bound */
  stmp = xmlGetProp(da, (const xmlChar *) "upper-bound");
  if (stmp)
    {
      high = xmlstrtod(stmp);
      xmlFree(stmp);
    }
  else
    {
      fprintf(stderr, "stp-sequence-create: \"upper-bound\" missing\n");
      goto error;
    }

  if (stpi_debug_level & STPI_DBG_XML)
    stpi_erprintf("stp_sequence_set_size %d\n", point_count + extra_points);
  stp_sequence_set_size(ret, point_count + extra_points);
  stp_sequence_set_bounds(ret, low, high);

  /* Now read in the data points */
  stmp = xmlNodeGetContent(da);
  if (stmp)
    {
      double tmpval;
      maxlen = strlen((const char *) stmp);
      for (i = 0; i < point_count; i++)
	{
	  memset(bufptr, 0, 100);
	  *(bufptr + 99) = '\0';
	  for (j = 0, found = 0; j < 99; j++)
	    {
	      if (offset + j > maxlen)
		{
		  if (found == 0)
		    {
		      xmlFree(stmp);
		      fprintf(stderr,
			      "stp-sequence-create: read aborted: too little data "
			      "(n=%d, needed %d)\n", i, point_count);
		      goto error;
		    }
		  else /* Hit end, but we have some data */
		    {
		      *(bufptr + j) = '\0';
		      break;
		    }
		}
	      if (!isspace((const char) *(stmp + offset + j)))
		found = 1; /* found a printing character */
	      else if (found) /* space found, and we've seen chars */
		{
		  *(bufptr + j) = '\0';
		  break;
		}
	      *(bufptr + j) = *(stmp + offset + j);
	    }
	  offset += j;
	  tmpval = xmlstrtod(buf);
	  if (! finite(tmpval)
	      || ( tmpval == 0 && errno == ERANGE )
	      || tmpval < low
	      || tmpval > high)
	    {
	      xmlFree(stmp);
	      fprintf(stderr, "stp-sequence-create: read aborted: "
		      "datum out of bounds: "
		      "%g (require %g <= n <= %g), n=%d\n",
		      tmpval, low, high, i);
	      goto error;
	    }
	  /* Datum was valid, so now add to the array */
	  stp_sequence_set_point(ret, i, tmpval);
	}
      xmlFree(stmp);
    }
  return ret;

 error:
  fprintf(stderr, "stp-sequence-create: error during array read\n");
  if (ret)
    stp_sequence_destroy(ret);
  return NULL;
}



static xmlNodePtr
stp_xmltree_create_from_sequence(stp_sequence_t seq,   /* The sequence */
				     size_t extra_points)
{
  size_t pointcount;
  double low;
  double high;

  char *count;
  char *lower_bound;
  char *upper_bound;

  xmlNodePtr seqnode;

  int i;                 /* loop counter */

  pointcount = stp_sequence_get_size(seq);
  pointcount -= extra_points;
  stp_sequence_get_bounds(seq, &low, &high);

  /* should count be of greater precision? */
  stpi_asprintf(&count, "%lu", (unsigned long) pointcount);
  stpi_asprintf(&lower_bound, "%g", low);
  stpi_asprintf(&upper_bound, "%g", high);

  seqnode = xmlNewNode(NULL, (const xmlChar *) "sequence");
  (void) xmlSetProp(seqnode, (const xmlChar *) "count",
		    (const xmlChar *) count);
  (void) xmlSetProp(seqnode, (const xmlChar *) "lower-bound",
		    (const xmlChar *) lower_bound);
  (void) xmlSetProp(seqnode, (const xmlChar *) "upper-bound",
		    (const xmlChar *) upper_bound);

  stpi_free(count);
  stpi_free(lower_bound);
  stpi_free(upper_bound);

  /* Write the curve points into the node content */
  {
    /* Calculate total size */
    int datasize = 0;
    xmlChar *data;
    xmlChar *offset;

    for (i = 0; i < pointcount; i++)
      {
	double dval;
	char *sval;

	if ((stp_sequence_get_point(seq, i, &dval)) != 1)
	  goto error;

	stpi_asprintf(&sval, "%g", dval);

	datasize += strlen(sval) + 1; /* Add 1 for space separator and
					 NUL termination */
	stpi_free(sval);
      }
    datasize += 2; /* Add leading and trailing newlines */
    /* Allocate a big enough string */
    data = (xmlChar *) stpi_malloc(sizeof(xmlChar) * datasize);
    offset = data;
    *(offset) = '\n'; /* Add leading newline */
    offset++;
    /* Populate the string */
    for (i = 0; i < pointcount; i++)
      {
	double dval;
	char *sval;

	if ((stp_sequence_get_point(seq, i, &dval)) == 0)
	  goto error;

	stpi_asprintf(&sval, "%g", dval);

	strcpy((char *) offset, sval); /* Add value */
	offset += strlen (sval);
	if ((i + 1) % 12)
	  *offset = ' '; /* Add space */
	else
	  *offset = '\n'; /* Add newline every 12 points */
	offset++;

	stpi_free(sval);
      }
    *(offset -1) = '\n'; /* Add trailing newline */
    *(offset) = '\0'; /* Add NUL terminator */
    xmlNodeAddContent(seqnode, (xmlChar *) data);
    stpi_free(data);
  }
  return seqnode;

 error:
  if (seqnode)
    xmlFreeNode(seqnode);
  return NULL;
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
      x_aspect = (int) xmlstrtoul(stmp);
      xmlFree(stmp);
    }
  else
    {
      fprintf(stderr, "stp-dither-matrix-create: \"x-aspect\" missing\n");
      goto error;
    }
  /* Get y-size */
  stmp = xmlGetProp(dm, (const xmlChar *) "y-aspect");
  if (stmp)
    {
      y_aspect = (int) xmlstrtoul(stmp);
      xmlFree(stmp);
    }
  else
    {
      fprintf(stderr, "stp-dither-matrix-create: \"y-aspect\" missing\n");
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
      fprintf(stderr,"xml-doc-get-dither-array: XML file not parsed successfully.\n");
      return NULL;
    }

  cur = xmlDocGetRootElement(doc);

  if (cur == NULL)
    {
      fprintf(stderr,"xml-doc-get-dither-array: empty document\n");
      xmlFreeDoc(doc);
      return NULL;
    }

  xmlseq = stp_xml_get_node(cur, "gimp-print", "dither-matrix", NULL);
  if (xmlseq == NULL )
    {
      fprintf(stderr,"xml-doc-get-dither-array: XML file is not a dither matrix.\n");
      return NULL;
    }

  return stpi_dither_array_create_from_xmltree(xmlseq);
}


stp_array_t
stpi_dither_array_create_from_file(const char* file)
{
  xmlDocPtr doc;   /* libXML document pointer */
  stp_array_t ret;

  stpi_xml_init();

  if (stpi_debug_level & STPI_DBG_XML)
    fprintf(stderr, "stp-xml-parse: reading `%s'...\n", file);

  doc = xmlParseFile(file);

  ret = xml_doc_get_dither_array(doc);

  if (doc)
    xmlFreeDoc(doc);

  stpi_xml_exit();

  return ret;
}


stp_curve_t
stp_curve_create_from_xmltree(xmlNodePtr curve)  /* The curve node */
{
  xmlChar *stmp;                          /* Temporary string */
  xmlNodePtr child;                       /* Child sequence node */
  stp_curve_t ret = NULL;                 /* Curve to return */
  stpi_internal_curve_t *iret;            /* Internal curve pointer */
  stp_curve_type_t curve_type;            /* Type of curve */
  stp_curve_wrap_mode_t wrap_mode;        /* Curve wrap mode */
  double gamma;                           /* Gamma value */
  size_t extra_points = 0;                /* real point count - point count */

  stpi_xml_init();

  /* Get curve type */
  stmp = xmlGetProp(curve, (const xmlChar *) "type");
  if (stmp)
    {
      if (!xmlStrcmp(stmp, (const xmlChar *) "linear"))
	  curve_type = STP_CURVE_TYPE_LINEAR;
      else if (!xmlStrcmp(stmp, (const xmlChar *) "spline"))
	  curve_type = STP_CURVE_TYPE_SPLINE;
      else
	{
	  fprintf(stderr, "stp-curve-create: %s: \"type\" invalid\n", stmp);
	  xmlFree (stmp);
	  goto error;
	}
      xmlFree (stmp);
    }
  else
    {
      fprintf(stderr, "stp-curve-create: \"type\" missing\n");
      goto error;
    }
  /* Get curve wrap mode */
  stmp = xmlGetProp(curve, (const xmlChar *) "wrap");
  if (stmp)
    {
      if (!xmlStrcmp(stmp, (const xmlChar *) "nowrap"))
	wrap_mode = STP_CURVE_WRAP_NONE;
      else if (!xmlStrcmp(stmp, (const xmlChar *) "wrap"))
	{
	  wrap_mode = STP_CURVE_WRAP_AROUND;
	  extra_points++;
	}
      else
	{
	  fprintf(stderr, "stp-curve-create: %s: \"wrap\" invalid\n", stmp);
	  xmlFree (stmp);
	  goto error;
	}
      xmlFree (stmp);
    }
  else
    {
      fprintf(stderr, "stp-curve-create: \"wrap\" missing\n");
      goto error;
    }
  /* Get curve gamma */
  stmp = xmlGetProp(curve, (const xmlChar *) "gamma");
  if (stmp)
    {
      gamma = (size_t) xmlstrtod(stmp);
      xmlFree(stmp);
    }
  else
    {
      fprintf(stderr, "stp-curve-create: \"gamma\" missing\n");
      goto error;
    }
  /* Set up the curve */
  ret = stp_curve_create(wrap_mode);
  iret = (stpi_internal_curve_t *) ret;
  stp_curve_set_interpolation_type(ret, curve_type);

  /* Get the sequence data */
  stp_sequence_destroy(iret->seq); /* Replace with new da */
  iret->seq = NULL;


  child = curve->children;
  while (child)
    {
      if (!xmlStrcmp(child->name, (const xmlChar *) "sequence"))
	{
	  iret->seq =
	    stp_sequence_create_from_xmltree(child, extra_points);
	  break;
	}
      child = child->next;
    }

  if (iret->seq == NULL)
    goto error;
  iret->point_count = stp_sequence_get_size(iret->seq);
  iret->point_count -= extra_points; /* remove extra wrap around points */

  if (stpi_curve_check_parameters(iret, iret->point_count) == 0)
    {
      fprintf(stderr, "stp-curve-create: parameter check failed\n");
      goto error;
    }
  if (gamma)
    {
      size_t points = iret->point_count;
      stp_curve_set_gamma(ret, gamma);
      stp_curve_resample(ret, points);
    }
  else /* Not a gamma curve */
    if (wrap_mode == STP_CURVE_WRAP_AROUND)
      {
	double tmpval;
	if ((stp_sequence_get_point(iret->seq, 0, &tmpval)) == 0)
	  {
	    fprintf(stderr, "stp-curve-create: sequence point read failed\n");
	    goto error;
	  }
	stp_sequence_set_point(iret->seq, iret->point_count, tmpval);
      }

  iret->recompute_interval = 1;

  stpi_xml_exit();

  return ret;

 error:
  fprintf(stderr, "stp-curve-create: error during curve read\n");
  if (ret)
    stp_curve_free(ret);
  stpi_xml_exit();
  return NULL;
}


static stp_curve_t
xml_doc_get_curve(xmlDocPtr doc)
{
  xmlNodePtr cur;
  xmlNodePtr xmlcurve;
  stp_curve_t curve = NULL;

  if (doc == NULL )
    {
      fprintf(stderr,"xml-doc-get-curve: XML file not parsed successfully.\n");
      return NULL;
    }

  cur = xmlDocGetRootElement(doc);

  if (cur == NULL)
    {
      fprintf(stderr,"xml-doc-get-curve: empty document\n");
      xmlFreeDoc(doc);
      return NULL;
    }

  xmlcurve = stp_xml_get_node(cur, "gimp-print", "curve", NULL);

  if (xmlcurve)
    curve = stp_curve_create_from_xmltree(xmlcurve);

  return curve;
}


stp_curve_t
stp_curve_create_from_file(const char* file)
{
  xmlDocPtr doc;   /* libXML document pointer */
  stp_curve_t curve = NULL;

  stpi_xml_init();

  if (stpi_debug_level & STPI_DBG_XML)
    fprintf(stderr, "stp-xml-parse: reading `%s'...\n", file);

  doc = xmlParseFile(file);

  curve = xml_doc_get_curve(doc);

  if (doc)
    xmlFreeDoc(doc);

  stpi_xml_exit();

  return curve;
}

stp_curve_t
stp_curve_create_from_string(const char* string)
{
  xmlDocPtr doc;   /* libXML document pointer */
  stp_curve_t curve = NULL;

  stpi_xml_init();

  if (stpi_debug_level & STPI_DBG_XML)
    fprintf(stderr, "stp-xml-parse: reading string...\n");

  doc = xmlParseMemory(string, strlen(string));

  curve = xml_doc_get_curve(doc);

  if (doc)
    xmlFreeDoc(doc);

  stpi_xml_exit();

  return curve;
}


/*
 * Create a basic gimp-print XML document tree root
 */
static xmlDocPtr
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


xmlNodePtr
stp_xmltree_create_from_curve(stp_curve_t curve)  /* The curve */
{
  stpi_internal_curve_t *icurve = (stpi_internal_curve_t *) curve;
  stp_curve_wrap_mode_t wrapmode;
  stp_curve_type_t interptype;
  double gammaval;

  char *wrap;
  size_t extra_points = 0;
  char *type;
  char *gamma;

  xmlNodePtr curvenode;
  xmlNodePtr child;

  stpi_xml_init();

  /* Get curve details */
  wrapmode = stp_curve_get_wrap(curve);
  if (wrapmode == STP_CURVE_WRAP_AROUND)
    extra_points++;
  interptype = stp_curve_get_interpolation_type(curve);
  gammaval = stp_curve_get_gamma(curve);

  /* Construct the allocated strings required */
  stpi_asprintf(&wrap, "%s", stpi_wrap_mode_names[wrapmode]);
  stpi_asprintf(&type, "%s", stpi_curve_type_names[interptype]);
  stpi_asprintf(&gamma, "%g", gammaval);

  curvenode = xmlNewNode(NULL, (const xmlChar *) "curve");
  (void) xmlSetProp(curvenode, (const xmlChar *) "wrap", (const xmlChar *) wrap);
  (void) xmlSetProp(curvenode, (const xmlChar *) "type", (const xmlChar *) type);
  (void) xmlSetProp(curvenode, (const xmlChar *) "gamma", (const xmlChar *) gamma);

  stpi_free(wrap);
  stpi_free(type);
  stpi_free(gamma);

  child = stp_xmltree_create_from_sequence(icurve->seq, extra_points);
  if (child == NULL)
    goto error;
  xmlAddChild(curvenode, child);

  stpi_xml_exit();

  return curvenode;

 error:
  if (curvenode)
    xmlFreeNode(curvenode);
  if (child)
    xmlFreeNode(child);
  stpi_xml_exit();

  return NULL;
}

static xmlDocPtr
xmldoc_create_from_curve(stp_curve_t curve)
{
  xmlDocPtr xmldoc;
  xmlNodePtr rootnode;
  xmlNodePtr curvenode;

  /* Get curve details */
  curvenode = stp_xmltree_create_from_curve(curve);
  if (curvenode == NULL)
    return NULL;

  /* Create the XML tree */
  xmldoc = stpi_xmldoc_create_generic();
  if (xmldoc == NULL)
    return NULL;
  rootnode = xmlDocGetRootElement(xmldoc);
  if (rootnode == NULL)
    {
      xmlFreeDoc(xmldoc);
      return NULL;
    }

  xmlAddChild(rootnode, curvenode);

  return xmldoc;
}

int
stp_curve_write(FILE *file, stp_curve_t curve)  /* The curve */
{
  xmlDocPtr xmldoc;
  xmlCharEncodingHandlerPtr xmlenc;
  xmlOutputBufferPtr xmlbuf;

  stpi_xml_init();

  xmldoc = xmldoc_create_from_curve(curve);
  if (curve == NULL)
    {
      stpi_xml_exit();
      return 1;
    }

  /* Save the XML file */

  xmlenc = xmlGetCharEncodingHandler(XML_CHAR_ENCODING_UTF8);
  xmlbuf = xmlOutputBufferCreateFile(file, xmlenc);
  xmlSaveFormatFileTo(xmlbuf, xmldoc, "UTF-8", 1);
  /* xmlOutputBufferFlush(xmlbuf); */
  /* xmlOutputBufferClose(xmlbuf); */

  xmlFreeDoc(xmldoc);

  stpi_xml_exit();

  return 0;
}

xmlChar *
stp_curve_write_string(stp_curve_t curve)  /* The curve */
{
  xmlDocPtr xmldoc;
  xmlChar *output = NULL;
  int size;

  stpi_xml_init();

  xmldoc = xmldoc_create_from_curve(curve);
  if (curve == NULL)
    {
      stpi_xml_exit();
      return NULL;
    }

  /* Save the XML file */

  xmlDocDumpFormatMemory(xmldoc, &output, &size, 1);

  xmlFreeDoc(xmldoc);

  stpi_xml_exit();

  return output;
}




static stp_array_t
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
      x_size = (int) xmlstrtoul(stmp);
      xmlFree(stmp);
    }
  else
    {
      fprintf(stderr, "stp-array-create: \"x-size\" missing\n");
      goto error;
    }
  /* Get y-size */
  stmp = xmlGetProp(array, (const xmlChar *) "y-size");
  if (stmp)
    {
      y_size = (int) xmlstrtoul(stmp);
      xmlFree(stmp);
    }
  else
    {
      fprintf(stderr, "stp-array-create: \"y-size\" missing\n");
      goto error;
    }

  /* Get the sequence data */

  child = array->children;
  while (child)
    {
      if (!xmlStrcmp(child->name, (const xmlChar *) "sequence"))
	{
	  seq = stp_sequence_create_from_xmltree(child, 0);
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
      fprintf(stderr, "stp-array-create: size mismatch between array and sequence\n");
      goto error;
    }

  return ret;

 error:
  fprintf(stderr, "stp-array-create: error during curve read\n");
  if (ret)
    stp_array_destroy(ret);
  return NULL;
}

