/*
 * "$Id$"
 *
 *   Test pattern generator for Gimp-Print
 *
 *   Copyright 2001 Robert Krawitz <rlk@alum.mit.edu>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "../../lib/libprintut.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <gimp-print/gimp-print-ui.h>
#include "gimp-print-ui-internal.h"
#include "printrc.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern int mylineno;

extern int yylex(void);
char *quotestrip(const char *i);
char *endstrip(const char *i);

extern int mylineno;
extern char* yytext;

static int yyerror( const char *s )
{
  fprintf(stderr,"stdin:%d: %s before '%s'\n", mylineno, s, yytext);
  return 0;
}

static stpui_plist_t *current_printer = NULL;

%}

%token <ival> tINT
%token <dval> tDOUBLE
%token <sval> tBOOLEAN
%token <sval> tSTRING
%token <sval> tWORD

%token CURRENT_PRINTER
%token SHOW_ALL_PAPER_SIZES
%token PRINTER
%token DESTINATION
%token SCALING
%token ORIENTATION
%token UNIT
%token DRIVER
%token LEFT
%token TOP
%token CUSTOM_PAGE_WIDTH
%token CUSTOM_PAGE_HEIGHT
%token OUTPUT_TYPE
%token PRINTRC_HDR
%token PARAMETER
%token pINT
%token pSTRING_LIST
%token pFILE
%token pDOUBLE
%token pBOOLEAN
%token pCURVE

%start Thing

%%

Printer: PRINTER tSTRING tSTRING
	{
	  current_printer = stpui_plist_create($2, $3);
	  g_free($2);
	  g_free($3);
	}
;

Destination: DESTINATION tSTRING
	{
	  stpui_plist_set_output_to(current_printer, $2);
	  g_free($2);
	}
;

Scaling: SCALING tDOUBLE
	{ current_printer->scaling = $2; }
;

Orientation: ORIENTATION tINT
	{ current_printer->orientation = $2; }
;

Unit: UNIT tINT
	{ current_printer->unit = $2; }
;

Left: LEFT tINT
	{ stp_set_left(current_printer->v, $2); }
;

Top: TOP tINT
	{ stp_set_top(current_printer->v, $2); }
;

Output_Type: OUTPUT_TYPE tINT
	{ stp_set_output_type(current_printer->v, $2); }
;

Custom_Page_Width: CUSTOM_PAGE_WIDTH tINT
	{ stp_set_page_width(current_printer->v, $2); }
;

Custom_Page_Height: CUSTOM_PAGE_HEIGHT tINT
	{ stp_set_page_height(current_printer->v, $2); }
;

Empty:
;

Int_Param: tWORD pINT tBOOLEAN tINT
	{
	  stp_set_int_parameter(current_printer->v, $1, $4);
	  if (strcmp($3, "False") == 0)
	    stp_set_int_parameter_active(current_printer->v, $1,
					 STP_PARAMETER_INACTIVE);
	  else
	    stp_set_int_parameter_active(current_printer->v, $1,
					 STP_PARAMETER_ACTIVE);
	  g_free($1);
	  g_free($3);
	}
;

String_List_Param: tWORD pSTRING_LIST tBOOLEAN tSTRING
	{
	  stp_set_string_parameter(current_printer->v, $1, $4);
	  if (strcmp($3, "False") == 0)
	    stp_set_string_parameter_active(current_printer->v, $1,
					    STP_PARAMETER_INACTIVE);
	  else
	    stp_set_string_parameter_active(current_printer->v, $1,
					    STP_PARAMETER_ACTIVE);
	  g_free($1);
	  g_free($3);
	  g_free($4);
	}
;

File_Param: tWORD pFILE tBOOLEAN tSTRING
	{
	  stp_set_file_parameter(current_printer->v, $1, $4);
	  if (strcmp($3, "False") == 0)
	    stp_set_file_parameter_active(current_printer->v, $1,
					  STP_PARAMETER_INACTIVE);
	  else
	    stp_set_file_parameter_active(current_printer->v, $1,
					  STP_PARAMETER_ACTIVE);
	  g_free($1);
	  g_free($3);
	  g_free($4);
	}
;

Double_Param: tWORD pDOUBLE tBOOLEAN tDOUBLE
	{
	  stp_set_float_parameter(current_printer->v, $1, $4);
	  if (strcmp($3, "False") == 0)
	    stp_set_float_parameter_active(current_printer->v, $1,
					   STP_PARAMETER_INACTIVE);
	  else
	    stp_set_float_parameter_active(current_printer->v, $1,
					   STP_PARAMETER_ACTIVE);
	  g_free($1);
	  g_free($3);
	}
;

Boolean_Param: tWORD pBOOLEAN tBOOLEAN tBOOLEAN
	{
	  if (strcmp($4, "False") == 0)
	    stp_set_boolean_parameter(current_printer->v, $1, 0);
	  else
	    stp_set_boolean_parameter(current_printer->v, $1, 1);
	  if (strcmp($3, "False") == 0)
	    stp_set_boolean_parameter_active(current_printer->v, $1,
					     STP_PARAMETER_INACTIVE);
	  else
	    stp_set_boolean_parameter_active(current_printer->v, $1,
					     STP_PARAMETER_ACTIVE);
	  g_free($1);
	  g_free($3);
	  g_free($4);
	}
;

Curve_Param: tWORD pCURVE tBOOLEAN tSTRING
	{
	  stp_curve_t curve = stp_curve_create_read_string($4);
	  if (curve)
	    {
	      stp_set_curve_parameter(current_printer->v, $1, curve);
	      if (strcmp($3, "False") == 0)
		stp_set_curve_parameter_active(current_printer->v, $1,
					       STP_PARAMETER_INACTIVE);
	      else
		stp_set_curve_parameter_active(current_printer->v, $1,
					       STP_PARAMETER_ACTIVE);
	      stp_curve_free(curve);
	    }
	  g_free($1);
	  g_free($3);
	  g_free($4);
	}
;

Typed_Param: Int_Param | String_List_Param | File_Param | Double_Param
	| Boolean_Param | Curve_Param
;

Parameter: PARAMETER Typed_Param
;

Parameters: Parameters Parameter | Empty
;

A_Printer: Printer Destination Scaling Orientation Unit Left Top
	Custom_Page_Width Custom_Page_Height Output_Type Parameters
;

Printers: Printers A_Printer | Empty
;

Current_Printer: CURRENT_PRINTER tSTRING
	{ stpui_printrc_current_printer = $2; }
;

Show_All_Paper_Sizes: SHOW_ALL_PAPER_SIZES tBOOLEAN
	{
	  if (strcmp($2, "True") == 0)
	    stpui_show_all_paper_sizes = 1;
	  else
	    stpui_show_all_paper_sizes = 0;
	}
;

Global: Current_Printer | Show_All_Paper_Sizes

Globals: Globals Global | Empty
;

Thing: PRINTRC_HDR Globals Printers
;

%%
