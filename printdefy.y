/*
 * "$Id$"
 *
 *   Parse printer definition pseudo-XML
 *
 *   Copyright 2000 Robert Krawitz (rlk@alum.mit.edu)
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

%{

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "printdef.h"

extern int mylineno;
printer_t thePrinter;
char *quotestrip(const char *i);
char *endstrip(const char *i);

void
initialize_the_printer(const char *name, const char *driver)
{
  strncpy(thePrinter.name, name, 63);
  strncpy(thePrinter.driver, driver, 63);
  thePrinter.model = -1;
  thePrinter.paramfunc[0] = 0;
  strcpy(thePrinter.paramfunc, "NULL");
  strcpy(thePrinter.mediasizefunc, "default_media_size");
  strcpy(thePrinter.imageableareafunc, "NULL");
  strcpy(thePrinter.printfunc, "NULL");
  thePrinter.imageableareafunc[0] = 0;
  thePrinter.printfunc[0] = 0;
  thePrinter.brightness = 100;
  thePrinter.gamma = 1.0;
  thePrinter.contrast = 100;
  thePrinter.red = 100;
  thePrinter.green = 100;
  thePrinter.blue = 100;
  thePrinter.saturation = 1.0;
  thePrinter.density = 1.0;
}

void
output_the_printer()
{
  printf("  {\n");
  printf("    %s,\n", thePrinter.name);
  printf("    %s,\n", thePrinter.driver);
  printf("    %d,\n", thePrinter.model);
  printf("    %s,\n", thePrinter.paramfunc);
  printf("    %s,\n", thePrinter.mediasizefunc);
  printf("    %s,\n", thePrinter.imageableareafunc);
  printf("    %s,\n", thePrinter.printfunc);
  printf("    {\n");
  printf("      \"\",\n");	/* output_to */
  printf("      \"\",\n");	/* driver */
  printf("      \"\",\n");	/* ppd_file */
  printf("      %d,\n", thePrinter.isColor);
  printf("      \"\",\n");	/* resolution */
  printf("      \"\",\n");	/* media_size */
  printf("      \"\",\n");	/* media_type */
  printf("      \"\",\n");	/* media_source */
  printf("      \"\",\n");	/* ink_type */
  printf("      %d,\n", thePrinter.brightness);
  printf("      1.0,\n");	/* scaling */
  printf("      -1,\n");	/* orientation */
  printf("      0,\n");		/* top */
  printf("      0,\n");		/* left */
  printf("      %.3f,\n", thePrinter.gamma);
  printf("      %d,\n", thePrinter.contrast);
  printf("      %d,\n", thePrinter.red);
  printf("      %d,\n", thePrinter.green);
  printf("      %d,\n", thePrinter.blue);
  printf("      0,\n");		/* linear */
  printf("      %.3f,\n", thePrinter.saturation);
  printf("      %.3f\n", thePrinter.density);
  printf("    }\n");
  printf("  },\n");
}

extern int mylineno;
extern char* yytext;

int yyerror( const char *s )
{
	fprintf(stderr,"stdin:%d: %s before '%s'\n",mylineno,s,yytext);
	return 0;
}

%}

%token <ival> tINT
%token <dval> tDOUBLE
%token <sval> tSTRING tCLASS
%token tBEGIN tEND ASSIGN PRINTER NAME DRIVER COLOR NOCOLOR MODEL PARAMFUNC
%token MEDIASIZEFUNC IMAGEABLEAREAFUNC PRINTFUNC BRIGHTNESS GAMMA CONTRAST
%token RED GREEN BLUE SATURATION DENSITY ENDPRINTER VALUE

%start Printers

%%

printerstart:	tBEGIN PRINTER NAME ASSIGN tSTRING DRIVER ASSIGN tSTRING tEND
	{ initialize_the_printer($5, $8); }
;
printerstartalt: tBEGIN PRINTER DRIVER ASSIGN tSTRING NAME ASSIGN tSTRING tEND
	{ initialize_the_printer($8, $5); }
;
printerend: 		tBEGIN ENDPRINTER tEND
	{ output_the_printer(); }
;
color:			tBEGIN COLOR tEND { thePrinter.isColor = 1; }
;
nocolor:		tBEGIN NOCOLOR tEND { thePrinter.isColor = 0; }
;
model:			tBEGIN MODEL VALUE ASSIGN tINT tEND
	{ thePrinter.model = $5; }
;
paramfunc:		tBEGIN PARAMFUNC VALUE ASSIGN tCLASS tEND
	{ strncpy(thePrinter.paramfunc, $5, 63); }
;
mediasizefunc:		tBEGIN MEDIASIZEFUNC VALUE ASSIGN tCLASS tEND
	{ strncpy(thePrinter.mediasizefunc, $5, 63); }
;
imageableareafunc:	tBEGIN IMAGEABLEAREAFUNC VALUE ASSIGN tCLASS tEND
	{ strncpy(thePrinter.imageableareafunc, $5, 63); }
;
printfunc:		tBEGIN PRINTFUNC VALUE ASSIGN tCLASS tEND
	{ strncpy(thePrinter.printfunc, $5, 63); }
;
brightness:		tBEGIN BRIGHTNESS VALUE ASSIGN tINT tEND
	{ thePrinter.brightness = $5; }
;
gamma:			tBEGIN GAMMA VALUE ASSIGN tDOUBLE tEND
	{ thePrinter.gamma = $5; }
;
contrast:		tBEGIN CONTRAST VALUE ASSIGN tINT tEND
	{ thePrinter.contrast = $5; }
;
red:			tBEGIN RED VALUE ASSIGN tINT tEND
	{ thePrinter.red = $5; }
;
green:			tBEGIN GREEN VALUE ASSIGN tINT tEND
	{ thePrinter.green = $5; }
;
blue:			tBEGIN BLUE VALUE ASSIGN tINT tEND
	{ thePrinter.blue = $5; }
;
saturation:		tBEGIN SATURATION VALUE ASSIGN tDOUBLE tEND
	{ thePrinter.saturation = $5; }
;
density:		tBEGIN DENSITY VALUE ASSIGN tDOUBLE tEND
	{ thePrinter.density = $5; }
;

Empty:

pstart: printerstart | printerstartalt
;

parg: color | nocolor | model | paramfunc | mediasizefunc | imageableareafunc |
	printfunc | brightness | gamma | contrast | red | green | blue |
	saturation | density

pargs: pargs parg | parg

Printer: pstart pargs printerend | pstart printerend

Printers: Printers Printer | Empty

%%

int
main()
{
  int retval;
  printf("/* This file is automatically generated.  See printers.xml.\n");
  printf("   DO NOT EDIT! */\n");
  printf("const static printer_t printers[] =\n");
  printf("{\n");
  retval = yyparse();
  printf("};\n");
  printf("const static int printer_count = sizeof(printers) / sizeof(printer_t);\n");
  return retval;
}
