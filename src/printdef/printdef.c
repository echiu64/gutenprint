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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include <libxml/parser.h>
#include <gimp-print/gimp-print.h>
#include "../main/vars.h"
#include "printdef.h"


void output_start(void);
void output_printer(stp_printdef_printer_t* printer);
void output_end(void);
int xmlstrtol(xmlChar *value);
float xmlstrtof(xmlChar *textval);
void stp_xml_process_gimpprint(xmlNodePtr gimpprint);
void stp_xml_process_printdef(xmlNodePtr printdef);
void stp_xml_process_family(xmlNodePtr family);
stp_printdef_printer_t *stp_xml_process_printer(xmlNodePtr printer, xmlChar *family);


/* Available "family" drivers */
const char *family_names[] =
{
  "canon",
  "escp2",
  "pcl",
  "ps",
  "lexmark",
  NULL
};


int main(int argc, char *argv[])
{
  xmlDocPtr doc;
  xmlNodePtr cur;

  if (argc != 2)
    {
      fprintf(stderr, "Usage: %s printers.xml\n", argv[0]);
      exit (EXIT_FAILURE);
    }

#ifdef DEBUG
  fprintf(stderr, "Reading XML file `%s'...", argv[1]);
#endif

  doc = xmlParseFile(argv[1]);

  if (doc == NULL )
    {
      fprintf(stderr,"XML file not parsed successfully. \n");
      xmlFreeDoc(doc);
      return EXIT_FAILURE;
    }

  cur = xmlDocGetRootElement(doc);

  if (cur == NULL)
    {
      fprintf(stderr,"empty document\n");
      xmlFreeDoc(doc);
      return EXIT_FAILURE;
    }

  if (xmlStrcmp(cur->name, (const xmlChar *) "gimp-print"))
    {
      fprintf(stderr,"XML file of the wrong type, root node != gimp-print");
      xmlFreeDoc(doc);
      return EXIT_FAILURE;
    }

  /* The XML file was read and is the right format */

#ifdef DEBUG
  fprintf(stderr, "done.\n");
  fprintf(stderr, "Writing header...");
#endif
  output_start();
#ifdef DEBUG
  fprintf(stderr, "done.\n");

  fprintf(stderr, "Processing XML parse tree:\n");
#endif
  stp_xml_process_gimpprint(cur);

#ifdef DEBUG
  fprintf(stderr, "Writing footer...");
#endif
  output_end();
#ifdef DEBUG
  fprintf(stderr, "done.\n");
#endif

  return EXIT_SUCCESS;
}

void output_start(void)
{
  int i = 0;

  fputs("/* This file is automatically generated.  See printers.xml.\n"
	"   DO NOT EDIT! */\n\n",
	stdout);
  while(family_names[i])
    {
      fprintf(stdout, "const extern stp_printfuncs_t stp_%s_printfuncs;\n",
	      (const char *) family_names[i]);
      i++;
    }
  fputs("\nstatic const stp_internal_printer_t stp_old_printer_list[] =\n"
	"{\n",
	stdout);
}

void output_printer(stp_printdef_printer_t *printer)
{
  fprintf(stdout, "  {\n");
  fprintf(stdout, "    \"%s\",\n", printer->long_name);
  fprintf(stdout, "    \"%s\",\n", printer->driver);
  fprintf(stdout, "    %d,\n", printer->model);
  fprintf(stdout, "    &stp_%s_printfuncs,\n", printer->family);
  fprintf(stdout, "    {\n");
  fprintf(stdout, "      \"%s\",\n", printer->printvars.driver);   /* driver */
  fprintf(stdout, "      \"\",\n");      /* ppd_file */
  fprintf(stdout, "      \"\",\n");      /* resolution */
  fprintf(stdout, "      \"\",\n");      /* media_size */
  fprintf(stdout, "      \"\",\n");      /* media_type */
  fprintf(stdout, "      \"\",\n");      /* media_source */
  fprintf(stdout, "      \"\",\n");      /* ink_type */
  fprintf(stdout, "      \"\",\n");      /* dither_algorithm */
  fprintf(stdout, "      %d,\n", printer->printvars.output_type);
  fprintf(stdout, "      %.3f,\n", printer->printvars.brightness);
  fprintf(stdout, "      0,\n");         /* left */
  fprintf(stdout, "      0,\n");         /* top */
  fprintf(stdout, "      0,\n");         /* width */
  fprintf(stdout, "      0,\n");         /* height */
  fprintf(stdout, "      %.3f,\n", printer->printvars.gamma);
  fprintf(stdout, "      %.3f,\n", printer->printvars.contrast);
  fprintf(stdout, "      %.3f,\n", printer->printvars.cyan);
  fprintf(stdout, "      %.3f,\n", printer->printvars.magenta);
  fprintf(stdout, "      %.3f,\n", printer->printvars.yellow);
  fprintf(stdout, "      %.3f,\n", printer->printvars.saturation);
  fprintf(stdout, "      %.3f,\n", printer->printvars.density);
  fprintf(stdout, "    }\n");
  fprintf(stdout, "  },\n");

}

void output_end(void)
{
  const char *footer =
    "};\n"
    "/* End of generated data */\n";

  fputs(footer, stdout);
}

int
xmlstrtol(xmlChar* textval)
 {
  int val;
  val = strtol((const char *) textval, (char **)NULL, 10);

  if (val == LONG_MIN || val == LONG_MAX)
    {
      fprintf(stderr, "Value incorrect: %s\n",
	      strerror(errno));
      exit (EXIT_FAILURE);
    }
  else if (val == LONG_MIN || val == LONG_MAX)
    {
      fprintf(stderr, "Value incorrect: %s\n",
	      "Out of range for integer type");
      exit (EXIT_FAILURE);
    }
  return val;
}


float
xmlstrtof(xmlChar *textval)
{
  float val;
  val = strtod((const char *) textval, (char **)NULL);

  if (val == HUGE_VAL || val == -HUGE_VAL)
    {
      fprintf(stderr, "Value incorrect: %s\n",
	      strerror(errno));
      exit (EXIT_FAILURE);
    }
  return (float) val;
}


void
stp_xml_process_gimpprint(xmlNodePtr cur)
{
  xmlNodePtr child;
  child = cur->children;
  while (child)
    {
      if (!xmlStrcmp(child->name, (const xmlChar *) "printdef"))
	  stp_xml_process_printdef(child);
      child = child->next;
    }
}


void
stp_xml_process_printdef(xmlNodePtr printdef)
{
  xmlNodePtr family;

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


void
stp_xml_process_family(xmlNodePtr family)
{
  xmlChar *family_name;
  xmlNodePtr printer;
  int i = 0, family_valid = 0;

  family_name = xmlGetProp(family, (const xmlChar *) "name");
  while (family_names[i])
    {
      if (!xmlStrcmp(family_name, (const xmlChar *) family_names[i]))
	  family_valid = 1;
      i++;
    }
#ifdef DEBUG
  fprintf(stderr, "  %s:\n", (const char *) family_name);
#endif

  printer = family->children;
  while (family_valid && printer)
    {
      if (!xmlStrcmp(printer->name, (const xmlChar *) "printer"))
	output_printer(stp_xml_process_printer(printer, family_name));
      printer = printer->next;
    }
}


stp_printdef_printer_t*
stp_xml_process_printer(xmlNodePtr printer, xmlChar *family)
{
  xmlNodePtr prop;
  /* props[] (unused) is the correct tag sequence */
  /*  const char *props[] =
    {
      "color",
      "model",
      "cyan",
      "yellow",
      "magenta",
      "brightness",
      "gamma",
      "density",
      "saturation",
      NULL
      };*/
  static stp_printdef_printer_t outprinter;

  /* Default values */
  outprinter.printvars.top = -1;
  outprinter.model = -1;
  outprinter.printvars.brightness = 1.0;
  outprinter.printvars.gamma = 1.0;
  outprinter.printvars.contrast = 1.0;
  outprinter.printvars.cyan = 1.0;
  outprinter.printvars.magenta = 1.0;
  outprinter.printvars.yellow = 1.0;
  outprinter.printvars.saturation = 1.0;
  outprinter.printvars.density = 1.0;

#ifdef DEBUG
  fprintf(stderr, "    %s\n",
	  xmlGetProp(printer, (const xmlChar*) "driver"));
#endif

  outprinter.long_name =
    (const char *) xmlGetProp(printer, (const xmlChar *) "name");
  outprinter.driver =
    (const char *) xmlGetProp(printer, (const xmlChar *) "driver");
  outprinter.printvars.driver =
    (const char *) xmlGetProp(printer, (const xmlChar *) "driver");

  outprinter.family =
    (const char *) family;


  prop = printer->children;
  while(prop)
    {
      if (!xmlStrcmp(prop->name, (const xmlChar *) "color"))
	{
	  if (!xmlStrcmp(xmlGetProp(prop, (const xmlChar *) "value"),
			 (const xmlChar *) "true"))
	    outprinter.printvars.output_type = OUTPUT_COLOR;
	  else
	    outprinter.printvars.output_type = OUTPUT_GRAY;
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "model"))
	{
	  outprinter.model =
	    xmlstrtol(xmlGetProp(prop,
				 (const xmlChar *) "value"));
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "cyan"))
	{
	  outprinter.printvars.cyan =
	    xmlstrtof(xmlGetProp(prop,
				 (const xmlChar *) "value"));
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "yellow"))
	{
	  outprinter.printvars.yellow =
	    xmlstrtof(xmlGetProp(prop,
				 (const xmlChar *) "value"));
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "magenta"))
	{
	  outprinter.printvars.magenta =
	    xmlstrtof(xmlGetProp(prop,
				 (const xmlChar *) "value"));
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "brightness"))
	{
	  outprinter.printvars.brightness =
	    xmlstrtof(xmlGetProp(prop,
				 (const xmlChar *) "value"));
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "gamma"))
	{
	  outprinter.printvars.gamma =
	    xmlstrtof(xmlGetProp(prop,
				 (const xmlChar *) "value"));
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "density"))
	{
	  outprinter.printvars.density =
	    xmlstrtof(xmlGetProp(prop,
				 (const xmlChar *) "value"));
	}
      if (!xmlStrcmp(prop->name, (const xmlChar *) "saturation"))
	{
	  outprinter.printvars.saturation =
	    xmlstrtof(xmlGetProp(prop,
				 (const xmlChar *) "value"));
	}
      prop = prop->next;
    }
  return &outprinter;
}
