/*
 * "$Id$"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1999-2000 Robert Krawitz (rlk@alum.mit.edu)
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
 *
 * Test for the soft weave algorithm.  This program calculates the weave
 * parameters for each input line and verifies that a number of conditions
 * are met.  Currently, the conditions checked are:
 *
 * 1) Pass # is >= 0
 * 2) The nozzle is within the physical bounds of the printer
 * 3) The computed starting row of the pass is not greater than the row
 *    index itself.
 * 4) The computed end of the pass is not less than the row index itself.
 * 5) If this row is the last row of a pass, that the last pass completed
 *    was one less than the current pass that we're just completing.
 * 6) For a given pass, the computed starting row of the pass is the same
 *    for all rows comprising the pass.
 * 7) For a given pass, the computed last row of the pass is the same
 *    for all rows comprising the pass.
 * 8) The input row is the same as the row computed by the algorithm.
 * 9) If there are phantom rows within the pass (unused jets before the
 *    first jet that is actually printing anything), then the number of
 *    phantom rows is not less than zero nor greater than or equal to the
 *    number of physical jets in the printer.
 * 10) The physical starting row of the pass (disregarding any phantom
 *    rows) is at least zero.
 * 11) If there are phantom rows, the number of phantom rows is less than
 *    the current nozzle number.
 * 12) If we are using multiple subpasses for each row, that each row within
 *    this pass is part of the same logical subpass.  Thus, if the pass in
 *    question is the first subpass for a given row, that it is the first
 *    subpass for ALL rows comprising the pass.  This doesn't matter if we're
 *    simply overlaying data; it's important if we have to shift the print head
 *    slightly between each pass to accomplish high-resolution printing.
 * 13) We are not overprinting a specified (row, subpass) pair.
 *
 * In addition to these per-row checks, we calculate the following global
 * correctness checks:
 *
 * 1) No pass starts (logically) at a later row than an earlier pass.
 */

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

#define MIN(x, y) ((x) <= (y) ? (x) : (y))

typedef struct			/* Weave parameters for a specific row */
{
  int row;			/* Absolute row # */
  int pass;			/* Computed pass # */
  int jet;			/* Which physical nozzle we're using */
  int missingstartrows;		/* Phantom rows (nonexistent rows that */
				/* would be printed by nozzles lower than */
				/* the first nozzle we're using this pass; */
				/* with the current algorithm, always zero */
  int logicalpassstart;		/* Offset in rows (from start of image) */
				/* that the printer must be for this row */
				/* to print correctly with the specified jet */
  int physpassstart;		/* Offset in rows to the first row printed */
				/* in this pass.  Currently always equal to */
				/* logicalpassstart */
  int physpassend;		/* Offset in rows (from start of image) to */
				/* the last row that will be printed this */
				/* pass (assuming that we're printing a full */
				/* pass). */
} weave_t;

typedef struct			/* Weave parameters for a specific pass */
{
  int pass;			/* Absolute pass number */
  int missingstartrows;		/* All other values the same as weave_t */
  int logicalpassstart;
  int physpassstart;
  int physpassend;
  int subpass;
} pass_t;

typedef union {			/* Offsets from the start of each line */
  off_t v[6];			/* (really pass) */
  struct {
    off_t k;
    off_t m;
    off_t c;
    off_t y;
    off_t M;
    off_t C;
  } p;
} lineoff_t;

typedef union {			/* Base pointers for each pass */
  unsigned char *v[6];
  struct {
    unsigned char *k;
    unsigned char *m;
    unsigned char *c;
    unsigned char *y;
    unsigned char *M;
    unsigned char *C;
  } p;
} linebufs_t;

typedef struct {
  unsigned char *linebufs;	/* Actual data buffers */
  linebufs_t *linebases;	/* Base address of each row buffer */
  lineoff_t *lineoffsets;	/* Offsets within each row buffer */
  int *linecounts;		/* How many rows we've printed this pass */
  pass_t *passes;		/* Circular list of pass numbers */
  int last_pass_offset;		/* Starting row (offset from the start of */
				/* the image) of the most recently printed */
				/* pass (so we can determine how far to */
				/* advance the paper) */
  int last_pass;		/* Number of the most recently printed pass */

  int njets;			/* Number of jets in use */
  int separation;		/* Separation between jets */

  int weavefactor;		/* Interleave factor (jets / separation) */
  int jetsused;			/* How many jets we can actually use */
  int initialoffset;		/* Distance between the first row we're */
				/* printing and the logical first row */
				/* (first nozzle of the first pass). */
				/* Currently this is zero. */
  int jetsleftover;		/* How many jets we're *not* using. */
				/* This can be used to rotate exactly */
				/* what jets we're using.  Currently this */
				/* is not used. */
  int weavespan;		/* How many rows total are bracketed by */
				/* one pass (separation * (jets - 1) */
  int horizontal_weave;		/* Number of horizontal passes required */
				/* This is > 1 for some of the ultra-high */
				/* resolution modes */
  int vertical_subpasses;	/* Number of passes per line (for better */
				/* quality) */
  int vmod;			/* Number of banks of passes */
  int oversample;		/* Excess precision per row */
  int realjets;
  int pass_adjustment;

  int is_monochrome;	/* Printing monochrome? */
  int bitwidth;		/* Bits per pixel */
  int lineno;
} escp2_softweave_t;

/*
 * Mapping between color and linear index.  The colors are
 * black, magenta, cyan, yellow, light magenta, light cyan
 */

const static int color_indices[16] = { 0, 1, 2, -1,
				       3, -1, -1, -1,
				       -1, 4, 5, -1,
				       -1, -1, -1, -1 };
const static int colors[6] = { 0, 1, 2, 4, 1, 2 };
const static int densities[6] = { 0, 0, 0, 0, 1, 1 };

static int
get_color_by_params(int plane, int density)
{
  if (plane > 4 || plane < 0 || density > 1 || density < 0)
    return -1;
  return color_indices[density * 8 + plane];
}

/*
 * Initialize the weave parameters
 */
static void *
initialize_weave(int jets, int sep, int osample, int v_subpasses,
		 int monochrome, int width)
{
  int i;
  int k;
  escp2_softweave_t *sw = malloc(sizeof (escp2_softweave_t));
  char *bufbase;
  if (jets <= 1)
    sw->separation = 1;
  if (sep <= 0)
    sw->separation = 1;
  else
    sw->separation = sep;
  sw->njets = jets;
  sw->realjets = jets;
  if (v_subpasses <= 0)
    v_subpasses = 1;
  sw->oversample = osample * v_subpasses;
  sw->vertical_subpasses = v_subpasses;
  sw->njets /= sw->oversample;
  sw->vmod = sw->separation * sw->oversample;
  sw->horizontal_weave = osample;
  sw->pass_adjustment = (osample * sep + jets - 1) / jets;

  sw->weavefactor = (sw->njets + sw->separation - 1) / sw->separation;
  sw->jetsused = MIN(((sw->weavefactor) * sw->separation), sw->njets);
  sw->initialoffset = (sw->jetsused - sw->weavefactor - 1) * sw->separation;
  if (sw->initialoffset < 0)
    sw->initialoffset = 0;
  sw->jetsleftover = sw->njets - sw->jetsused;
  sw->weavespan = (sw->jetsused - 1) * sw->separation;

  sw->is_monochrome = monochrome;
  sw->bitwidth = width;

  sw->last_pass_offset = 0;
  sw->last_pass = -1;

  sw->linebufs = malloc(6 * 3072 * sw->vmod * jets * sw->oversample *
			sw->bitwidth);
  sw->lineoffsets = malloc(sw->vmod * sizeof(lineoff_t) * sw->oversample);
  sw->linebases = malloc(sw->vmod * sizeof(linebufs_t) * sw->oversample);
  sw->passes = malloc(sw->vmod * sizeof(pass_t));
  sw->linecounts = malloc(sw->vmod * sizeof(int));
  sw->lineno = 0;

  bufbase = sw->linebufs;
  
  for (i = 0; i < sw->vmod; i++)
    {
      int j;
      sw->passes[i].pass = -1;
      for (k = 0; k < sw->oversample; k++)
	{
	  for (j = 0; j < 6; j++)
	    {
	      sw->linebases[k * sw->vmod + i].v[j] = bufbase;
	      bufbase += 3072 * jets * sw->bitwidth;
	    }
	}
    }
  return (void *) sw;
}

static void
destroy_weave(void *vsw)
{
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  free(sw->linecounts);
  free(sw->passes);
  free(sw->linebases);
  free(sw->lineoffsets);
  free(sw->linebufs);
  free(vsw);
}

/*
 * Compute the weave parameters for the given row.  This computation is
 * rather complex, and I need to go back and write down very carefully
 * what's going on here.
 */

#ifdef DEBUG_SIGNAL
static int 
divv(int x, int y)
{
  if (x < 0 || y < 0)
    kill(getpid(), SIGFPE);
  else
    return x / y;
}

static int 
modd(int x, int y)
{
  if (x < 0 || y < 0)
    kill(getpid(), SIGFPE);
  else
    return x % y;
}
#else

#define divv(x, y) ((x) / (y))
#define modd(x, y) ((x) % (y))

#endif

/*
 * Compute the weave parameters for the given row.  This computation is
 * rather complex, and I need to go back and write down very carefully
 * what's going on here.
 */

static void
weave_parameters_by_row(const escp2_softweave_t *sw, int row,
			int vertical_subpass, weave_t *w)
{
  int passblockstart = divv((row + sw->initialoffset), sw->jetsused);
  int internaljetsused = sw->jetsused * sw->oversample;
  int subpass_adjustment;

  w->row = row;
  w->pass = sw->pass_adjustment + (passblockstart - (sw->separation - 1)) +
    modd((sw->separation + row - passblockstart), sw->separation);
  subpass_adjustment = modd(divv((sw->separation + w->pass + 1),
				 sw->separation), sw->oversample);
  subpass_adjustment = sw->oversample - subpass_adjustment - 1;
  vertical_subpass = modd((sw->oversample + vertical_subpass +
			   subpass_adjustment), sw->oversample);
  w->pass += sw->separation * vertical_subpass;
  w->logicalpassstart = (w->pass * sw->jetsused) - sw->initialoffset -
    (sw->weavefactor * sw->separation) +
    modd((w->pass + sw->separation - 2), sw->separation);
  w->jet = divv((row + (sw->realjets * sw->separation) - w->logicalpassstart),
		sw->separation) - sw->realjets;
  w->jet += sw->jetsused * (sw->oversample - 1);
  if (w->jet >= sw->realjets)
    {
      w->jet -= sw->realjets;
      w->pass += sw->vmod;
    }
  w->logicalpassstart = w->row - (w->jet * sw->separation);
  if (w->logicalpassstart >= 0)
    w->physpassstart = w->logicalpassstart;
  else
    w->physpassstart = w->logicalpassstart +
      (sw->separation * divv((sw->separation - 1 - w->logicalpassstart),
			     sw->separation));
  w->physpassend = (internaljetsused - 1) * sw->separation +
    w->logicalpassstart;
  w->missingstartrows = divv((w->physpassstart - w->logicalpassstart),
			    sw->separation);
}

static lineoff_t *
get_lineoffsets(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->lineoffsets[w.pass % sw->vmod]);
}

static int *
get_linecount(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->linecounts[w.pass % sw->vmod]);
}

static const linebufs_t *
get_linebases(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->linebases[w.pass % sw->vmod]);
}

static pass_t *
get_pass_by_row(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->passes[w.pass % sw->vmod]);
}

static lineoff_t *
get_lineoffsets_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->lineoffsets[pass % sw->vmod]);
}

static int *
get_linecount_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->linecounts[pass % sw->vmod]);
}

static const linebufs_t *
get_linebases_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->linebases[pass % sw->vmod]);
}

static pass_t *
get_pass_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->passes[pass % sw->vmod]);
}

int
main(int argc, char **argv)
{
  int i;
  int j;
  weave_t w;
  int errors = 0;
  int lastpass = -1;
  int newestpass = -1;
  int *passstarts;
  int *logpassstarts;
  int *passends;
  int *passcounts;
  char *physpassstuff;
  char *rowdetail;
  int nrows;
  int physjets;
  int physsep;
  int hpasses, vpasses;
  void *sw;
  if (argc != 6)
    {
      fprintf(stderr, "Usage: %s jets separation hpasses vpasses rows\n",
	      argv[0]);
      return 2;
    }
  physjets = strtol(argv[1], 0, 0);
  physsep = strtol(argv[2], 0, 0);
  hpasses = strtol(argv[3], 0, 0);
  vpasses = strtol(argv[4], 0, 0);
  nrows = strtol(argv[5], 0, 0);
  passstarts = malloc(sizeof(int) * nrows);
  logpassstarts = malloc(sizeof(int) * nrows);
  passends = malloc(sizeof(int) * nrows);
  passcounts = malloc(sizeof(int) * nrows);
  physpassstuff = malloc(nrows);
  rowdetail = malloc(nrows * physjets);
  memset(rowdetail, 0, nrows * physjets);
  memset(physpassstuff, -1, nrows);

  sw = initialize_weave(physjets, physsep, hpasses, vpasses, 0, 1);
  printf("%13s %5s %5s %5s %10s %10s %10s %10s\n", "", "row", "pass", "jet",
	 "missing", "logical", "physstart", "physend");
  for (i = 0; i < nrows; i++)
    {
      passstarts[i] = -1;
      passends[i] = -1;
    }
  for (i = 0; i < nrows; i++)
    {
      for (j = 0; j < hpasses * vpasses; j++)
	{
	  int physrow;
	  weave_parameters_by_row((escp2_softweave_t *)sw, i, j, &w);
	  physrow = w.logicalpassstart + physsep * w.jet;
	  printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%5d %5d %5d %10d %10d %10d %10d\n",
		 w.pass < 0 ? (errors++, 'A') : ' ',
		 w.jet < 0 || w.jet > physjets - 1 ? (errors++, 'B') : ' ',
		 w.physpassstart > w.row ? (errors++, 'C') : ' ',
		 w.physpassend < w.row ? (errors++, 'D') : ' ',
		 w.physpassend == w.row && lastpass + 1 != w.pass ? (errors++, 'E') : ' ',
		 passstarts[w.pass] != -1 && passstarts[w.pass] !=w.physpassstart ?
		 (errors++, 'F') : ' ',
		 passends[w.pass] != -1 && passends[w.pass] !=w.physpassend ?
		 (errors++, 'G') : ' ',
		 w.row != physrow ? (errors++, 'H') : ' ',
		 w.missingstartrows < 0 || w.missingstartrows > physjets - 1 ?
		 (errors++, 'I') : ' ',
		 w.physpassstart < 0 ? (errors++, 'J') : ' ',
		 w.missingstartrows > w.jet ? (errors++, 'K') : ' ',
		 physpassstuff[w.pass] >= 0 && physpassstuff[w.pass] != j ?
		 (errors++, 'L') : ' ',
		 rowdetail[w.pass * physjets + w.jet] == 1 ? (errors++, 'M') :
		 ' ',
		 w.row, w.pass, w.jet,
		 w.missingstartrows, w.logicalpassstart, w.physpassstart,
		 w.physpassend,
		 rowdetail[w.pass * physjets + w.jet]);
	  if (w.physpassend == w.row)
	    {
	      lastpass = w.pass;
	      passends[w.pass] = -2;
	    }
	  else
	    passends[w.pass] = w.physpassend;
	  passstarts[w.pass] = w.physpassstart;
	  logpassstarts[w.pass] = w.logicalpassstart;
	  rowdetail[w.pass * physjets + w.jet] = 1;
	  if (physpassstuff[w.pass] == -1)
	    physpassstuff[w.pass] = j;
	  if (w.pass > newestpass)
	    newestpass = w.pass;
	}
    }
  printf("Unterminated passes:\n");
  for (i = 0; i <= newestpass; i++)
    if (passends[i] >= -1 && passends[i] < nrows)
      printf("%d %d\n", i, passends[i]);
  printf("Last terminated pass: %d\n", lastpass);
  printf("Pass starts:\n");
  for (i = 0; i < newestpass; i++)
    {
      if (i == 0)
	printf("%c %d %d\n", ' ', i, logpassstarts[i]);
      else
	printf("%c %d %d\n",
	       logpassstarts[i] <= logpassstarts[i - 1] ? (errors++, 'A') : ' ',
	       i, logpassstarts[i]);
    }
  printf("%d total errors\n", errors);
  return 0;
}
