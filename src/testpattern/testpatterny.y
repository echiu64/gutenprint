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

extern int mylineno;

extern int yylex(void);
char *quotestrip(const char *i);
char *endstrip(const char *i);

extern char* yytext;

static int yyerror( const char *s )
{
	fprintf(stderr,"stdin:%d: %s before '%s'\n",mylineno,s,yytext);
	return 0;
}

%}

%token <ival> tINT
%token <dval> tDOUBLE
%token <sval> tSTRING

%token C_GAMMA
%token M_GAMMA
%token Y_GAMMA
%token K_GAMMA
%token LC_GAMMA
%token LM_GAMMA
%token LK_GAMMA
%token GAMMA
%token C_LEVEL
%token M_LEVEL
%token Y_LEVEL
%token K_LEVEL
%token LC_LEVEL
%token LM_LEVEL
%token LK_LEVEL
%token LEVELS
%token INK_LIMIT
%token INK
%token WIDTH
%token PRINTER
%token PARAMETER
%token DENSITY
%token TOP
%token LEFT
%token HSIZE
%token VSIZE
%token BLACKLINE
%token PATTERN
%token XPATTERN
%token EXTENDED
%token IMAGE

%start Thing

%%

extended: EXTENDED tINT
	{ global_ink_depth = $2; }
;
global_c_level: C_LEVEL tDOUBLE
	{ global_c_level = $2; }
;
global_m_level: M_LEVEL tDOUBLE
	{ global_m_level = $2; }
;
global_y_level: Y_LEVEL tDOUBLE
	{ global_y_level = $2; }
;
global_k_level: K_LEVEL tDOUBLE
	{ global_k_level = $2; }
;
global_c_gamma: C_GAMMA tDOUBLE
	{ global_c_gamma = $2; }
;
global_m_gamma: M_GAMMA tDOUBLE
	{ global_m_gamma = $2; }
;
global_y_gamma: Y_GAMMA tDOUBLE
	{ global_y_gamma = $2; }
;
global_k_gamma: K_GAMMA tDOUBLE
	{ global_k_gamma = $2; }
;
global_lc_level: LC_LEVEL tDOUBLE
	{ global_lc_level = $2; }
;
global_lm_level: LM_LEVEL tDOUBLE
	{ global_lm_level = $2; }
;
global_lk_level: LK_LEVEL tDOUBLE
	{ global_lk_level = $2; }
;
global_lc_gamma: LC_GAMMA tDOUBLE
	{ global_lc_gamma = $2; }
;
global_lm_gamma: LM_GAMMA tDOUBLE
	{ global_lm_gamma = $2; }
;
global_lk_gamma: LK_GAMMA tDOUBLE
	{ global_lk_gamma = $2; }
;
global_gamma: GAMMA tDOUBLE
	{ global_gamma = $2; }
;
levels: LEVELS tINT
	{ levels = $2; }
;
ink_limit: INK_LIMIT tDOUBLE
	{ ink_limit = $2; }
;
printer: PRINTER tSTRING
	{ printer = $2; }
;
parameter: PARAMETER tSTRING tSTRING
	{
	  stp_set_string_parameter(tv, $2, $3);
	  free($2);
	  free($3);
	}
;
density: DENSITY tDOUBLE
	{ density = $2; }
;
top: TOP tDOUBLE
	{ xtop = $2; }
;
left: LEFT tDOUBLE
	{ xleft = $2; }
;
hsize: HSIZE tDOUBLE
	{ hsize = $2; }
;
vsize: VSIZE tDOUBLE
	{ vsize = $2; }
;
blackline: BLACKLINE tINT
	{ noblackline = !($2); }
;

pattern: PATTERN tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE
	tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE
	tDOUBLE tDOUBLE
	{
	  testpattern_t *t = get_next_testpattern();
	  t->t = E_PATTERN;
	  t->d.p.c_min = $2;
	  t->d.p.c = $3;
	  t->d.p.c_gamma = $4;
	  t->d.p.m_min = $5;
	  t->d.p.m = $6;
	  t->d.p.m_gamma = $7;
	  t->d.p.y_min = $8;
	  t->d.p.y = $9;
	  t->d.p.y_gamma = $10;
	  t->d.p.k_min = $11;
	  t->d.p.k = $12;
	  t->d.p.k_gamma = $13;
	  t->d.p.c_level = $14;
	  t->d.p.m_level = $15;
	  t->d.p.y_level = $16;
	  t->d.p.lower = $17;
	  t->d.p.upper = $18;
	}
;

xpattern: XPATTERN tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE
	  tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE
	  tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE
	{
	  testpattern_t *t = get_next_testpattern();
	  if (global_ink_depth == 0)
	    {
	      fprintf(stderr, "xpattern may only be used with extended color depth\n");
	      exit(1);
	    }
	  t->t = E_XPATTERN;
	  t->d.p.k_min = $2;
	  t->d.p.k = $3;
	  t->d.p.k_gamma = $4;
	  t->d.p.lk_min = $5;
	  t->d.p.lk = $6;
	  t->d.p.lk_gamma = $7;
	  t->d.p.c_min = $8;
	  t->d.p.c = $9;
	  t->d.p.c_gamma = $10;
	  t->d.p.lc_min = $11;
	  t->d.p.lc = $12;
	  t->d.p.lc_gamma = $13;
	  t->d.p.m_min = $14;
	  t->d.p.m = $15;
	  t->d.p.m_gamma = $16;
	  t->d.p.lm_min = $17;
	  t->d.p.lm = $18;
	  t->d.p.lm_gamma = $19;
	  t->d.p.y_min = $20;
	  t->d.p.y = $21;
	  t->d.p.y_gamma = $22;
	}
;

image: IMAGE tINT tINT
	{
	  testpattern_t *t = get_next_testpattern();
	  t->t = E_IMAGE;
	  t->d.i.x = $2;
	  t->d.i.y = $3;
	  if (t->d.i.x <= 0 || t->d.i.y <= 0)
	    {
	      fprintf(stderr, "image width and height must be greater than zero\n");
	      exit(1);
	    }
	  return 0;
	}
;

Empty:
;

Rule:   global_k_level | global_c_level | global_m_level | global_y_level
	| global_lk_level | global_lc_level | global_lm_level
	| global_c_gamma | global_m_gamma | global_y_gamma | global_k_gamma
	| global_lc_gamma | global_lm_gamma | global_lk_gamma
	| global_gamma | levels | ink_limit | printer | parameter | density
	| top | left | hsize | vsize | blackline | extended
;

A_Pattern: pattern | xpattern
;

Patterns: Patterns A_Pattern | Empty
;

Image: image
;

Rules: Rules Rule | Empty
;

Output: Patterns | Image
;

Thing: Rules Output Empty
;

%%
