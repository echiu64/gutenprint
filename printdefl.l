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

#define YY_NO_UNPUT

int mylineno = 1;

#if 0
#define DBG(x) fprintf(stderr, "'%s'%s\n", yytext, #x)
#else
#define DBG(x) 0
#endif

%}

%option noyywrap

digit		[0-9]
integer		[-+]?{digit}+
float		[-+]?{digit}+(\.{digit}+)?([eE][-+]?{digit}+)?
class		[a-zA-Z][a-zA-Z0-9_]+
string		[\"][^\"]+[\"]
ws		[ \t]+

%%

\<			DBG(tBEGIN);return tBEGIN;
\>			DBG(tEND);return tEND;
=			DBG(ASSIGN);return ASSIGN;
printer			DBG(PRINTER);return PRINTER;
\/printer		DBG(ENDPRINTER);return ENDPRINTER;
name			DBG(NAME);return NAME;
driver			DBG(DRIVER);return DRIVER;
color			DBG(COLOR);return COLOR;
nocolor			DBG(NOCOLOR);return NOCOLOR;
model			DBG(MODEL);return MODEL;
paramfunc		DBG(PARAMFUNC);return PARAMFUNC;
mediasizefunc		DBG(MEDIASIZEFUNC);return MEDIASIZEFUNC;
imageableareafunc	DBG(IMAGEABLEAREAFUNC);return IMAGEABLEAREAFUNC;
printfunc		DBG(PRINTFUNC);return PRINTFUNC;
brightness		DBG(BRIGHTNESS);return BRIGHTNESS;
gamma			DBG(GAMMA);return GAMMA;
contrast		DBG(CONTRAST);return CONTRAST;
red			DBG(RED);return RED;
green			DBG(GREEN);return GREEN;
blue			DBG(BLUE);return BLUE;
saturation		DBG(SATURATION);return SATURATION;
density			DBG(DENSITY);return DENSITY;
value			DBG(VALUE);return VALUE;

{integer}		yylval.ival = atoi(yytext); DBG(tINT);return tINT;
{float}			yylval.dval = strtod(yytext, NULL); DBG(tDOUBLE);return tDOUBLE;
{string}		yylval.sval = strdup(yytext); DBG(tSTRING);return tSTRING;
{class}			yylval.sval = strdup(yytext); DBG(tCLASS);return tCLASS;
{ws}			DBG(whitespace1);	/* Skip blanks/tabs */
#[^\n]*			DBG(comment1);	/* Skip comments */
\n			DBG(newline);mylineno++;
