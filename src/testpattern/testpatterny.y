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

typedef struct
{
  const char *name;
  int channel;
} color_t;

static color_t color_map[] =
  {
    { "black", 0 },
    { "cyan", 1 },
    { "red", 1 },
    { "magenta", 2 },
    { "green", 2 },
    { "yellow", 3 },
    { "blue", 3 },
    { "l_black", 4 },
    { "l_cyan", 5 },
    { "l_magenta", 6 },
    { "d_yellow", 4 },
    { NULL, -1 }
  };

static int current_index = 0;
static testpattern_t *current_testpattern;

static int
find_color(const char *name)
{
  int i = 0;
  while (color_map[i].name)
    {
      if (strcmp(color_map[i].name, name) == 0)
	return color_map[i].channel;
      i++;
    }
  return -1;
}

%}

%token <ival> tINT
%token <dval> tDOUBLE
%token <sval> tSTRING

%token CYAN
%token L_CYAN
%token MAGENTA
%token L_MAGENTA
%token YELLOW
%token D_YELLOW
%token BLACK
%token L_BLACK
%token GAMMA
%token LEVEL
%token STEPS
%token INK_LIMIT
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
%token GRID
%token SEMI
%token CHANNEL

%start Thing

%%

COLOR: CYAN | L_CYAN | MAGENTA | L_MAGENTA
	| YELLOW | D_YELLOW | BLACK | L_BLACK
;

extended: EXTENDED tINT
	{ global_ink_depth = $2; }
;

level: LEVEL COLOR tDOUBLE
	{
	  int channel = find_color($2.sval);
	  if (channel >= 0)
	    global_levels[channel] = $3;
	}
;

channel_level: LEVEL tINT tDOUBLE
	{
	  if ($2 >= 0 && $2 <= STP_CHANNEL_LIMIT)
	    global_levels[$2] = $3;
	}
;

gamma: GAMMA COLOR tDOUBLE
	{
	  int channel = find_color($2.sval);
	  if (channel >= 0)
	    global_gammas[channel] = $3;
	}
;

channel_gamma: GAMMA tINT tDOUBLE
	{
	  if ($2 >= 0 && $2 <= STP_CHANNEL_LIMIT)
	    global_gammas[$2] = $3;
	}
;

global_gamma: GAMMA tDOUBLE
	{ global_gamma = $2; }
;
steps: STEPS tINT
	{ steps = $2; }
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

color_block1: tDOUBLE tDOUBLE tDOUBLE
	{
	  if (current_index < STP_CHANNEL_LIMIT)
	    {
	      current_testpattern->d.p.mins[current_index] = $1;
	      current_testpattern->d.p.vals[current_index] = $2;
	      current_testpattern->d.p.gammas[current_index] = $3;
	      current_index++;
	    }
	}
;

color_blocks1a: color_block1 | color_blocks1a color_block1
;

color_blocks1b: /* empty */ | color_blocks1a
;

color_blocks1: color_block1 color_blocks1b
;

color_block2a: COLOR tDOUBLE tDOUBLE tDOUBLE
	{
	  int channel = find_color($1.sval);
	  if (channel >= 0 && channel < STP_CHANNEL_LIMIT)
	    {
	      current_testpattern->d.p.mins[channel] = $2;
	      current_testpattern->d.p.vals[channel] = $3;
	      current_testpattern->d.p.gammas[channel] = $4;
	    }
	}
;

color_block2b: CHANNEL tINT tDOUBLE tDOUBLE tDOUBLE
	{
	  if ($2 >= 0 && $2 < STP_CHANNEL_LIMIT)
	    {
	      current_testpattern->d.p.mins[$2] = $3;
	      current_testpattern->d.p.vals[$2] = $4;
	      current_testpattern->d.p.gammas[$2] = $5;
	    }
	}
;

color_block2: color_block2a | color_block2b
;

color_blocks2a: color_block2 | color_blocks2a color_block2
;

color_blocks2: /* empty */ | color_blocks2a
;

color_blocks: color_blocks1 | color_blocks2
;

patvars: tDOUBLE tDOUBLE tDOUBLE tDOUBLE tDOUBLE
	{
	  current_testpattern->t = E_PATTERN;
	  current_index = 0;
	  current_testpattern->d.p.lower = $1;
	  current_testpattern->d.p.upper = $2;
	  current_testpattern->d.p.levels[1] = $3;
	  current_testpattern->d.p.levels[2] = $4;
	  current_testpattern->d.p.levels[3] = $5;
	  current_testpattern = get_next_testpattern();
	}
;

pattern: PATTERN patvars color_blocks SEMI
;

xpattern: XPATTERN color_blocks SEMI
	{
	  if (global_ink_depth == 0)
	    {
	      fprintf(stderr, "xpattern may only be used with extended color depth\n");
	      exit(1);
	    }
	  current_testpattern->t = E_XPATTERN;
	  current_index = 0;
	  current_testpattern = get_next_testpattern();
	}
;

grid: GRID tINT SEMI
	{
	  current_testpattern->t = E_GRID;
	  current_testpattern->d.g.ticks = $2;
	  current_testpattern = get_next_testpattern();
	}
;

image: IMAGE tINT tINT
	{
	  current_testpattern->t = E_IMAGE;
	  current_testpattern->d.i.x = $2;
	  current_testpattern->d.i.y = $3;
	  if (current_testpattern->d.i.x <= 0 ||
	      current_testpattern->d.i.y <= 0)
	    {
	      fprintf(stderr, "image width and height must be greater than zero\n");
	      exit(1);
	    }
	  return 0;
	}
;

Rule:   gamma | channel_gamma | level | channel_level | global_gamma | steps
	| ink_limit | printer | parameter | density | top | left | hsize
	| vsize | blackline | extended
;

A_Pattern: pattern | xpattern | grid
;

Patterns: /* empty */ | Patterns A_Pattern
;

Image: image
;

Rules: /* empty */ | Rules Rule
;

Output: Patterns | Image
;

Thing: Rules
	{
	  current_testpattern = get_next_testpattern();
	}
	Output
;

%%
