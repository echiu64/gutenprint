/*
 * "$Id$"
 *
 *   printdef XML parser - process gimp-print XML data with libxml2.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and Michael Natterer (mitch@gimp.org)
 *   Copyright 2002 Roger Leigh (roger@whinlatter.uklinux.net)
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include "module.h"
#include "papers.h"
#include "path.h"
#include "printers.h"
#include "vars.h"
#include "xml.h"


static int xmlstrtol(xmlChar *textval);
static unsigned xmlstrtoul(xmlChar *textval);
static float xmlstrtof(xmlChar *textval);
static void stp_xml_process_gimpprint(xmlNodePtr gimpprint);
static void stp_xml_process_printdef(xmlNodePtr printdef);
static void stp_xml_process_family(xmlNodePtr family);
static stp_internal_printer_t *stp_xml_process_printer(xmlNodePtr printer, xmlChar *family, const stp_printfuncs_t *printfuncs);
static void stp_xml_process_paperdef(xmlNodePtr paperdef);
static stp_internal_papersize_t *stp_xml_process_paper(xmlNodePtr paper);


/*
 * Read all available XML files.
 */
int stp_xml_init(void)
{
  stp_list_t *dir_list;                   /* List of directories to scan */
  stp_list_t *file_list;                  /* List of files to load */
  stp_list_item_t *item;                  /* Pointer to current list item */
  xmlFreeFunc xml_free_func = NULL;       /* libXML free function */
  xmlMallocFunc xml_malloc_func = NULL;   /* libXML malloc function */
  xmlReallocFunc xml_realloc_func = NULL; /* libXML realloc function */
  xmlStrdupFunc xml_strdup_func = NULL;   /* libXML strdup function */

  /* Use our memory allocation functions with libXML */
  xmlMemGet (&xml_free_func,
	     &xml_malloc_func,
	     &xml_realloc_func,
	     &xml_strdup_func);
  xmlMemSetup (stp_free,
	       stp_malloc,
	       stp_realloc,
	       stp_strdup);

  /* Make a list of all the available XML files */
  if (!(dir_list = stp_list_create()))
    return 1;
  stp_list_set_freefunc(dir_list, stp_list_node_free_data);
  stp_path_split(dir_list, getenv("STP_DATA_PATH"));
  stp_path_split(dir_list, PKGXMLDATADIR);
  file_list = stp_path_search(dir_list, ".xml");
  stp_list_destroy(dir_list);

  /* Parse each XML file */
  item = stp_list_get_start(file_list);
  while (item)
    {
#ifdef DEBUG
      fprintf(stderr, "stp-xml: source file: %s\n",
	      (const char *) stp_list_item_get_data(item));
#endif
  xmlInitParser();
      stp_xml_parse_file((const char *) stp_list_item_get_data(item));
  xmlCleanupParser();
      item = stp_list_item_next(item);
    }
  stp_list_destroy(file_list);

  /* Restore libXML memory functions to their previous state */
  xmlMemSetup (xml_free_func,
	       xml_malloc_func,
	       xml_realloc_func,
	       xml_strdup_func);

  return 0;
}


/*
 * Parse a single XML file.
 */
int
stp_xml_parse_file(const char *file) /* File to parse */
{
  xmlDocPtr doc;   /* libXML document pointer */
  xmlNodePtr cur;  /* libXML node pointer */

#ifdef DEBUG
  fprintf(stderr, "stp-xml-parse: reading  `%s'...\n", file);
#endif

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

  stp_xml_process_gimpprint(cur);
  xmlFreeDoc(doc);

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

  if (val == LONG_MIN || val == LONG_MAX)
    {
      fprintf(stderr, "Value incorrect: %s\n",
	      strerror(errno));
      exit (EXIT_FAILURE);
    }
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

  if (val == ULONG_MAX)
    {
      fprintf(stderr, "Value incorrect: %s\n",
	      strerror(errno));
      exit (EXIT_FAILURE);
    }
  return val;
}

/*
 * Convert a text string into a float.
 */
static float
xmlstrtof(xmlChar *textval)
{
  float val; /* The value to return */
  val = strtod((const char *) textval, (char **)NULL);

  if (val == HUGE_VAL || val == -HUGE_VAL)
    {
      fprintf(stderr, "Value incorrect: %s\n",
	      strerror(errno));
      exit (EXIT_FAILURE);
    }
  return (float) val;
}


/*
 * Parse the <gimp-print> root node.
 */
static void
stp_xml_process_gimpprint(xmlNodePtr cur) /* The node to parse */
{
  xmlNodePtr child;                       /* Child node pointer */

  child = cur->children;
  while (child)
    {
      /* process nodes with corresponding parser */
      if (!xmlStrcmp(child->name, (const xmlChar *) "printdef"))
	stp_xml_process_printdef(child);
      else if (!xmlStrcmp(child->name, (const xmlChar *) "paperdef"))
	stp_xml_process_paperdef(child);
      child = child->next;
    }
}


/*
 * Parse the <printdef> node.
 */
static void
stp_xml_process_printdef(xmlNodePtr printdef) /* The printdef node */
{
  xmlNodePtr family;                          /* Family child node */

  family = printdef->children;
  while (family)
    {
      if (!xmlStrcmp(family->name, (const xmlChar *) "family"))
	{
	  stp_xml_process_family(family);
	}
      family = family->next;
    }
}


/*
 * Parse the <family> node.
 */
static void
stp_xml_process_family(xmlNodePtr family)     /* The family node */
{
  stp_list_t *family_module_list = NULL;      /* List of valid families */
  stp_list_item_t *family_module_item;        /* Current family */
  xmlChar *family_name;                       /* Name of family */
  xmlNodePtr printer;                         /* printer child node */
  stp_module_t *family_module_data;           /* Family module data */
  stp_internal_family_t *family_data = NULL;  /* Family data */
  int family_valid = 0;                       /* Is family valid? */
  stp_internal_printer_t *outprinter;         /* Generated printer */

  family_module_list = stp_module_get_class(STP_MODULE_CLASS_FAMILY);
  if (!family_module_list)
    return;


  family_name = xmlGetProp(family, (const xmlChar *) "name");
  family_module_item = stp_list_get_start(family_module_list);
  while (family_module_item)
    {
      family_module_data = (stp_module_t *)
	stp_list_item_get_data(family_module_item);
      if (!xmlStrcmp(family_name, (const xmlChar *)
		     family_module_data->name))
	{
#ifdef DEBUG
	  fprintf(stderr, "xml-family: family module: %s\n",
		  family_module_data->name);
#endif
	  family_data = family_module_data->syms;
	  if (family_data->printer_list == NULL)
	    family_data->printer_list = stp_list_create();
	  family_valid = 1;
	}
	  family_module_item = stp_list_item_next(family_module_item);
    }

  printer = family->children;
  while (family_valid && printer)
    {
      if (!xmlStrcmp(printer->name, (const xmlChar *) "printer"))
	{
	  outprinter = stp_xml_process_printer(printer, family_name,
					       family_data->printfuncs);
	  if (outprinter)
	    stp_list_item_create(family_data->printer_list, NULL, outprinter);
	}
      printer = printer->next;
    }

  stp_list_destroy(family_module_list);
  xmlFree (family_name);
  return;
}

typedef struct
{
  const char *property;
  const char *parameter;
} stp_xml_prop_t;

static const stp_xml_prop_t stp_xml_props[] =
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
static stp_internal_printer_t*
stp_xml_process_printer(xmlNodePtr printer,           /* The printer node */
			xmlChar *family,              /* Family name */
			const stp_printfuncs_t *printfuncs)
                                                      /* Family printfuncs */
{
  xmlNodePtr prop;                                    /* Temporary node pointer */
  xmlChar *stmp;                                      /* Temporary string */
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
  stp_internal_printer_t *outprinter;                 /* Generated printer */
  int
    driver = 0,                                       /* Check driver */
    long_name = 0,                                    /* Check long_name */
    color = 0,                                        /* Check color */
    model = 0;                                        /* Check model */

  outprinter = stp_malloc(sizeof(stp_internal_printer_t));
  if (!outprinter)
    return NULL;
  outprinter->printvars = stp_vars_create();
  if (outprinter->printvars == NULL)
    {
      stp_free(outprinter);
      return NULL;
    }

  outprinter->cookie = COOKIE_PRINTER;

  stmp = xmlGetProp(printer, (const xmlChar *) "driver");
  stp_set_driver(outprinter->printvars, (const char *) stmp);
  xmlFree(stmp);
    
  outprinter->long_name =
    (char *) xmlGetProp(printer, (const xmlChar *) "name");
  outprinter->family = stp_strdup((const char *) family);

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
	  if (!xmlStrcmp(stmp, (const xmlChar *) "true"))
	    stp_set_output_type(outprinter->printvars, OUTPUT_COLOR);
	  else
	    stp_set_output_type(outprinter->printvars, OUTPUT_GRAY);
	  xmlFree(stmp);
	  color = 1;
	}
      else if (!xmlStrcmp(prop->name, (const xmlChar *) "model"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  outprinter->model = xmlstrtol(stmp);
	  xmlFree(stmp);
	  model = 1;
	}
      else
	{
	  const stp_xml_prop_t *stp_prop = stp_xml_props;
	  while (stp_prop->property)
	    {
	      if (!xmlStrcmp(prop->name, (const xmlChar *) stp_prop->property))
		{
		  stmp = xmlGetProp(prop, (const xmlChar *) "value");
		  stp_set_float_parameter(outprinter->printvars,
					  stp_prop->parameter,
					  xmlstrtof(stmp));
		  xmlFree(stmp);
		  break;
		}
	      stp_prop++;
	    }	  
	}
      prop = prop->next;
    }
  if (driver && long_name && color && model && printfuncs)
    {
#ifdef DEBUG
      stmp = xmlGetProp(printer, (const xmlChar*) "driver");
      fprintf(stderr, "xml-family: printer: %s\n", stmp);
      xmlFree(stmp);
#endif
      return outprinter;
    }
  stp_free(outprinter);
  return NULL;
}


/*
 * Parse the <paperdef> node.
 */
static void
stp_xml_process_paperdef(xmlNodePtr paperdef) /* The paperdef node */
{
  xmlNodePtr paper;                           /* paper node pointer */
  stp_internal_papersize_t *outpaper;         /* Generated paper */

  paper = paperdef->children;
  while (paper)
    {
      if (!xmlStrcmp(paper->name, (const xmlChar *) "paper"))
	{
	  outpaper = stp_xml_process_paper(paper);
	  if (outpaper)
	    stp_paper_create((stp_papersize_t) outpaper);
	}
      paper = paper->next;
    }
}


/*
 * Process the <paper> node.
 */
static stp_internal_papersize_t *
stp_xml_process_paper(xmlNodePtr paper) /* The paper node */
{
  xmlNodePtr prop;                      /* Temporary node pointer */
  xmlChar *stmp;                        /* Temporary string */
  /* props[] (unused) is the correct tag sequence */
  /*  const char *props[] =
    {
      "name",
      "description",
      "width",
      "height",
      "unit",
      NULL
      };*/
  stp_internal_papersize_t *outpaper;   /* Generated paper */
  int
    id = 0,                             /* Check id is present */
    name = 0,                           /* Check name is present */
    height = 0,                         /* Check height is present */
    width = 0,                          /* Check width is present */
    unit = 0;                           /* Check unit is present */

#ifdef DEBUG
  stmp = xmlGetProp(paper, (const xmlChar*) "name");
  fprintf(stderr, "xml-paper: name: %s\n", stmp);
  xmlFree(stmp);
#endif

  outpaper = stp_malloc(sizeof(stp_internal_papersize_t));
  if (!outpaper)
    return NULL;

  outpaper->name =
    (char *) xmlGetProp(paper, (const xmlChar *) "name");
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
	  outpaper->width = xmlstrtoul(stmp);
	  xmlFree(stmp);
	  width = 1;
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "height"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  outpaper->height = xmlstrtoul(stmp);
	  xmlFree(stmp);
	  height = 1;
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "unit"))
	{
	  stmp = xmlGetProp(prop, (const xmlChar *) "value");
	  if (!xmlStrcmp(stmp,
			 (const xmlChar *) "english"))
	    outpaper->paper_unit = PAPERSIZE_ENGLISH;
	  else if (!xmlStrcmp(stmp,
			      (const xmlChar *) "metric"))
	    outpaper->paper_unit = PAPERSIZE_METRIC;
	  /* Default unit */
	  else
	    outpaper->paper_unit = PAPERSIZE_METRIC;
	  xmlFree(stmp);
	  unit = 1;
	}

      outpaper->top = 0;
      outpaper->left = 0;
      outpaper->bottom = 0;
      outpaper->right = 0;

      prop = prop->next;
    }
  if (id && name && width && height && unit)
    return outpaper;
  stp_free(outpaper);
  outpaper = NULL;
  return NULL;
}
