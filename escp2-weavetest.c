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
#include <unistd.h>
#include <string.h>

#define DEBUG_SIGNAL
#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define WEAVETEST
#include "print-escp2.c"

const char header[] = "\
Legend:\n\
A  Negative pass number.\n\
B  Jet number out of range.\n\
C  Starting row of this pass after the current row.\n\
D  Ending row of this pass before the current row.\n\
E  Current row is the ending row of the pass, and the pass number is not\n\
   one greater than the previous completed pass.\n\
F  The current pass's starting row is not consistent with the previously\n\
   observed starting row of this pass.
G  The current pass's ending row is not consistent with the previously\n\
   observed ending row of this pass.\n\
H  The current row does not match the computed current row.\n\
I  The number of missing start rows is less than zero or greater than or\n\
   equal to the actual number of jets.\n\
J  The first printed row of this pass is less than zero.\n\
K  The number of missing start rows of this pass is greater than the\n\
   jet used to print the current row.\n\
L  The subpass printed by the current pass is not consistent with an earlier\n\
   record of the subpass printed by this pass.\n\
M  The same physical row is being printed more than once.\n";


static void
print_header(void)
{
  printf("%s", header);
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
  int hpasses, vpasses, subpasses;
  void *sw;
  if (argc != 7)
    {
      fprintf(stderr, "Usage: %s jets separation hpasses vpasses subpasses rows\n",
	      argv[0]);
      return 2;
    }
  physjets = atoi(argv[1]);
  physsep = atoi(argv[2]);
  hpasses = atoi(argv[3]);
  vpasses = atoi(argv[4]);
  subpasses = atoi(argv[5]);
  nrows = atoi(argv[6]);
  passstarts = malloc(sizeof(int) * nrows);
  logpassstarts = malloc(sizeof(int) * nrows);
  passends = malloc(sizeof(int) * nrows);
  passcounts = malloc(sizeof(int) * nrows);
  physpassstuff = malloc(nrows);
  rowdetail = malloc(nrows * physjets);
  memset(rowdetail, 0, nrows * physjets);
  memset(physpassstuff, -1, nrows);

  sw = initialize_weave(physjets, physsep, hpasses, vpasses, subpasses,
			COLOR_MONOCHROME, 1, 128, 1);
  print_header();
  printf("%13s %5s %5s %5s %10s %10s %10s %10s\n", "", "row", "pass", "jet",
	 "missing", "logical", "physstart", "physend");
  for (i = 0; i < nrows; i++)
    {
      passstarts[i] = -1;
      passends[i] = -1;
    }
  for (i = 0; i < nrows; i++)
    {
      for (j = 0; j < hpasses * vpasses * subpasses; j++)
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
		 w.physpassend);
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
	       logpassstarts[i] <= logpassstarts[i - 1] ? (errors++, 'N') : ' ',
	       i, logpassstarts[i]);
    }
  printf("%d total errors\n", errors);
  destroy_weave(sw);
  if (errors > 0)
    return 1;
  else
    return 0;
}
