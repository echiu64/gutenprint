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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "testpattern.h"

#define YY_NO_UNPUT

int mylineno = 1;

#define DBG(x)						\
do							\
  {							\
    if (getenv("STP_TESTPATTERN_DEBUG"))		\
      fprintf(stderr, "'%s' => %s\n", yytext, #x);	\
  }							\
 while (0)

%}

%option noyywrap

digit		[0-9]
integer		[-+]?{digit}+
float		[-+]?{digit}+(\.{digit}+)?([eE][-+]?{digit}+)?
string		([\"][^\"\n]+[\"])|([a-zA-Z0-9_][-a-zA-Z0-9_]*)
ws		[ \t]+

%%

cyan			DBG(CYAN); return CYAN;
l_cyan			DBG(L_CYAN); return L_CYAN;
magenta			DBG(MAGENTA); return MAGENTA;
l_magenta		DBG(L_MAGENTA); return L_MAGENTA;
yellow			DBG(YELLOW); return YELLOW;
d_yellow		DBG(D_YELLOW); return D_YELLOW;
black			DBG(BLACK); return BLACK;
l_black			DBG(L_BLACK); return L_BLACK;
gamma			DBG(GAMMA); return GAMMA;
level			DBG(LEVEL); return LEVEL;
steps			DBG(STEPS); return STEPS;
ink_limit		DBG(INK_LIMIT); return INK_LIMIT;
width			DBG(WIDTH); return WIDTH;
printer			DBG(PRINTER); return PRINTER;
parameter		DBG(PARAMETER); return PARAMETER;
density			DBG(DENSITY); return DENSITY;
top			DBG(TOP); return TOP;
left			DBG(LEFT); return LEFT;
hsize			DBG(HSIZE); return HSIZE;
vsize			DBG(VSIZE); return VSIZE;
blackline		DBG(BLACKLINE); return BLACKLINE;
pattern			DBG(PATTERN); return PATTERN;
xpattern		DBG(XPATTERN); return XPATTERN;
image			DBG(IMAGE); return IMAGE;
extended		DBG(EXTENDED); return EXTENDED;
grid			DBG(GRID); return GRID;
;			DBG(SEMI); return SEMI;

{integer}		yylval.ival = atoi(yytext); DBG(tINT); return tINT;
{float}			yylval.dval = strtod(yytext, NULL); DBG(tDOUBLE); return tDOUBLE;
{string}		yylval.sval = c_strdup(yytext); DBG(tSTRING); return tSTRING;
{ws}			DBG(whitespace); 	/* Skip blanks/tabs */
#[^\n]*			DBG(comment); 	/* Skip comments */
\n			DBG(newline); mylineno++;
