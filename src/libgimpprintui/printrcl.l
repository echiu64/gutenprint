/*		-*-Mode: C-*-
 * "$Id$"
 *
 *   printrc parser
 *
 *   Copyright 2003 Robert Krawitz <rlk@alum.mit.edu>
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

#include <gimp-print/gimp-print-intl-internal.h>
#include <gimp-print-ui/gimp-print-ui.h>
#include "gimp-print-ui-internal.h"
#include "printrc.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define YY_NO_UNPUT

int mylineno = 1;

#if 0
#define DBG(x) fprintf(stderr, "'%s'%s\n", yytext, #x);
#else
#define DBG(x)
#endif

static char *
q_strdup(const char *s)
{
  /* Strip the leading and trailing quote */
  if (strlen(s) <= 2)
    return NULL;
  else
    return g_strndup(s + 1, strlen(s) - 2);
}

%}

%option noyywrap

digit		[0-9]
boolean		(True)|(False)
integer		[-+]?{digit}+
float		[-+]?{digit}+(\.{digit}+)?([eE][-+]?{digit}+)?
string         ([\"][^\"\n]*[\"])|([\'][^\']*[\'])
ws		[ \t]+
printrc_hdr	#PRINTRCv2.*\n
word		[a-zA-Z][a-zA-Z0-9_]*

%%

Current-Printer:	DBG(CURRENT_PRINTER); return CURRENT_PRINTER;
Show-All-Paper-Sizes:	DBG(SHOW_ALL_PAPER_SIZES); return SHOW_ALL_PAPER_SIZES;
Printer:		DBG(PRINTER); return PRINTER;
Destination:		DBG(DESTINATION); return DESTINATION;
Scaling:		DBG(SCALING); return SCALING;
Orientation:		DBG(ORIENTATION); return ORIENTATION;
Autosize-Roll-Paper:	DBG(AUTOSIZE_ROLL_PAPER); return AUTOSIZE_ROLL_PAPER;
Unit:			DBG(UNIT); return UNIT;
Driver:			DBG(DRIVER); return DRIVER;
Left:			DBG(LEFT); return LEFT;
Top:			DBG(TOP); return TOP;
Custom_Page_Width:	DBG(CUSTOM_PAGE_WIDTH); return CUSTOM_PAGE_WIDTH;
Custom_Page_Height:	DBG(CUSTOM_PAGE_HEIGHT); return CUSTOM_PAGE_HEIGHT;
Output_Type:		DBG(OUTPUT_TYPE); return OUTPUT_TYPE;
Parameter		DBG(PARAMETER); return PARAMETER;

Int			DBG(pINT); return pINT;
String			DBG(pSTRING_LIST); return pSTRING_LIST;
File			DBG(pFILE); return pFILE;
Double			DBG(pDOUBLE); return pDOUBLE;
Boolean			DBG(pBOOLEAN); return pBOOLEAN;
Curve			DBG(pCURVE); return pCURVE;

{integer}		yylval.ival = atoi(yytext); DBG(tINT) return tINT;
{float}			yylval.dval = strtod(yytext, NULL); DBG(tDOUBLE) return tDOUBLE;
{string}		yylval.sval = q_strdup(yytext); DBG(tSTRING) return tSTRING;
{boolean}		yylval.sval = g_strdup(yytext); DBG(tBOOLEAN) return tBOOLEAN;
{word}			yylval.sval = g_strdup(yytext); DBG(tWORD) return tWORD;
{printrc_hdr}		DBG(PRINTRC_HDR) return PRINTRC_HDR;
{ws}			DBG(whitespace1) 	/* Skip blanks/tabs */
#[^\n]*			DBG(comment1) 	/* Skip comments */
\n			DBG(newline) mylineno++;
