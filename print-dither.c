/*
 * "$Id$"
 *
 *   Print plug-in driver utility functions for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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
 * Revision History:
 *
 *   $Log$
 *   Revision 1.8  2000/03/02 03:09:03  rlk
 *   Performance, by replacing long long with int
 *
 *   Revision 1.7  2000/02/28 01:26:11  rlk
 *   Try to improve high resolution quality
 *
 *   Revision 1.6  2000/02/26 00:14:44  rlk
 *   Rename dither_{black,cmyk}4 to dither_{black,cmyk}_n, and add argument to specify how levels are to be encoded
 *
 *   Revision 1.5  2000/02/21 20:32:37  rlk
 *   Important dithering bug fixes:
 *
 *   1) Avoid runaway black buildup.
 *
 *   2) Some conversion functions weren't doing density
 *
 *   Revision 1.4  2000/02/18 02:30:01  rlk
 *   A few more dithering bugs
 *
 *   Revision 1.3  2000/02/16 00:59:19  rlk
 *   1) Use correct convert functions (canon, escp2, pcl, ps).
 *
 *   2) Fix gray_to_rgb increment (print-util)
 *
 *   3) Fix dither update (print-dither)
 *
 *   Revision 1.2  2000/02/07 01:35:05  rlk
 *   Try to improve variable dot stuff
 *
 *   Revision 1.1  2000/02/06 18:40:53  rlk
 *   Split out dither stuff from print-util
 *
 */


/* #define PRINT_DEBUG */


#include "print.h"



/*
 * Error buffer for dither functions.  This needs to be at least 14xMAXDPI
 * (currently 720) to avoid problems...
 *
 * Want to dynamically allocate this so we can save memory!
 */

#define ERROR_ROWS 2
#define NCOLORS (4)

#define ECOLOR_C 0
#define ECOLOR_M 1
#define ECOLOR_Y 2
#define ECOLOR_K 3

typedef struct dither
{
  int *errs[ERROR_ROWS][NCOLORS];
  int src_width;
  int dst_width;
  int horizontal_overdensity;
  int overdensity_bits;

  int cbits;			/* Oversample counters for the various inks */
  int lcbits;
  int mbits;
  int lmbits;
  int ybits;
  int lybits;
  int kbits;

  int k_lower;			/* Transition range (lower/upper) for CMY */
  int k_upper;			/* vs. K */

  int lc_level;			/* Relative levels (0-65536) for light */
  int lm_level;			/* inks vs. full-strength inks */
  int ly_level;

  int c_randomizer;		/* Randomizers.  MORE EXPLANATION */
  int m_randomizer;
  int y_randomizer;
  int k_randomizer;

  int k_clevel;			/* Amount of each ink (in 64ths) required */
  int k_mlevel;			/* to create equivalent black */
  int k_ylevel;

  int c_darkness;		/* Perceived "darkness" of each ink, */
  int m_darkness;		/* in 64ths, to calculate CMY-K transitions */
  int y_darkness;

  int nc_l;			/* Number of levels of each color available */
  int nc_log;			/* Log of number of levels (how many bits) */
  int *c_transitions;		/* Vector of transition points between */
  int *c_levels;		/* Vector of actual levels */

  int nlc_l;
  int nlc_log;
  int *lc_transitions;
  int *lc_levels;

  int nm_l;
  int nm_log;
  int *m_transitions;
  int *m_levels;

  int nlm_l;
  int nlm_log;
  int *lm_transitions;
  int *lm_levels;

  int ny_l;
  int ny_log;
  int *y_transitions;
  int *y_levels;

  int nly_l;
  int nly_log;
  int *ly_transitions;
  int *ly_levels;

  int nk_l;
  int nk_log;
  int *k_transitions;
  int *k_levels;

} dither_t;

void *
init_dither(int in_width, int out_width, int horizontal_overdensity)
{
  dither_t *d = malloc(sizeof(dither_t));
  memset(d, 0, sizeof(dither_t));

  d->horizontal_overdensity = horizontal_overdensity;
  switch (horizontal_overdensity)
    {
    case 0:
    case 1:
      d->overdensity_bits = 0;
      break;
    case 2:
      d->overdensity_bits = 1;
      break;
    case 4:
      d->overdensity_bits = 2;
      break;
    case 8:
      d->overdensity_bits = 3;
      break;
    }
  d->src_width = in_width;
  d->dst_width = out_width;
  d->cbits = 1;
  d->lcbits = 1;
  d->mbits = 1;
  d->lmbits = 1;
  d->ybits = 1;
  d->lybits = 1;
  d->kbits = 1;
  d->k_lower = 12 * 256;
  d->k_upper = 128 * 256;
  d->lc_level = 24576;
  d->lm_level = 24576;
  d->ly_level = 24576;
  d->c_randomizer = 0;
  d->m_randomizer = 0;
  d->y_randomizer = 0;
  d->k_randomizer = 4;
  d->k_clevel = 64;
  d->k_mlevel = 64;
  d->k_ylevel = 64;
  d->c_darkness = 22;
  d->m_darkness = 16;
  d->y_darkness = 10;
  d->nc_l = 4;
  d->nc_log = 2;
  d->c_transitions = malloc(4 * sizeof(int));
  d->c_levels = malloc(4 * sizeof(int));
  d->nlc_l = 4;
  d->nlc_log = 2;
  d->lc_transitions = malloc(4 * sizeof(int));
  d->lc_levels = malloc(4 * sizeof(int));
  d->nm_l = 4;
  d->nm_log = 2;
  d->m_transitions = malloc(4 * sizeof(int));
  d->m_levels = malloc(4 * sizeof(int));
  d->nlm_l = 4;
  d->nlm_log = 2;
  d->lm_transitions = malloc(4 * sizeof(int));
  d->lm_levels = malloc(4 * sizeof(int));
  d->ny_l = 4;
  d->ny_log = 2;
  d->y_transitions = malloc(4 * sizeof(int));
  d->y_levels = malloc(4 * sizeof(int));
  d->nly_l = 4;
  d->nly_log = 2;
  d->ly_transitions = malloc(4 * sizeof(int));
  d->ly_levels = malloc(4 * sizeof(int));
  d->nk_l = 4;
  d->nk_log = 2;
  d->k_transitions = malloc(4 * sizeof(int));
  d->k_levels = malloc(4 * sizeof(int));
  d->c_levels[0] = 0;
  d->c_transitions[0] = 0;
  d->c_levels[1] = 32767;
  d->c_transitions[1] = (32767 + 0) / 2;
  d->c_levels[2] = 213 * 256;
  d->c_transitions[2] = ((213 * 256) + 32767) / 2;
  d->c_levels[3] = 65535;
  d->c_transitions[3] = (65535 + (213 * 256)) / 2;
  d->lc_levels[0] = 0;
  d->lc_transitions[0] = 0;
  d->lc_levels[1] = 32767;
  d->lc_transitions[1] = (32767 + 0) / 2;
  d->lc_levels[2] = 213 * 256;
  d->lc_transitions[2] = ((213 * 256) + 32767) / 2;
  d->lc_levels[3] = 65535;
  d->lc_transitions[3] = (65535 + (213 * 256)) / 2;
  d->m_levels[0] = 0;
  d->m_transitions[0] = 0;
  d->m_levels[1] = 32767;
  d->m_transitions[1] = (32767 + 0) / 2;
  d->m_levels[2] = 213 * 256;
  d->m_transitions[2] = ((213 * 256) + 32767) / 2;
  d->m_levels[3] = 65535;
  d->m_transitions[3] = (65535 + (213 * 256)) / 2;
  d->lm_levels[0] = 0;
  d->lm_transitions[0] = 0;
  d->lm_levels[1] = 32767;
  d->lm_transitions[1] = (32767 + 0) / 2;
  d->lm_levels[2] = 213 * 256;
  d->lm_transitions[2] = ((213 * 256) + 32767) / 2;
  d->lm_levels[3] = 65535;
  d->lm_transitions[3] = (65535 + (213 * 256)) / 2;
  d->y_levels[0] = 0;
  d->y_transitions[0] = 0;
  d->y_levels[1] = 32767;
  d->y_transitions[1] = (32767 + 0) / 2;
  d->y_levels[2] = 213 * 256;
  d->y_transitions[2] = ((213 * 256) + 32767) / 2;
  d->y_levels[3] = 65535;
  d->y_transitions[3] = (65535 + (213 * 256)) / 2;
  d->ly_levels[0] = 0;
  d->ly_transitions[0] = 0;
  d->ly_levels[1] = 32767;
  d->ly_transitions[1] = (32767 + 0) / 2;
  d->ly_levels[2] = 213 * 256;
  d->ly_transitions[2] = ((213 * 256) + 32767) / 2;
  d->ly_levels[3] = 65535;
  d->ly_transitions[3] = (65535 + (213 * 256)) / 2;
  d->k_levels[0] = 0;
  d->k_transitions[0] = 0;
  d->k_levels[1] = 32767;
  d->k_transitions[1] = (32767 + 0) / 2;
  d->k_levels[2] = 213 * 256;
  d->k_transitions[2] = ((213 * 256) + 32767) / 2;
  d->k_levels[3] = 65535;
  d->k_transitions[3] = (65535 + (213 * 256)) / 2;
  return d;
}  

void
dither_set_black_lower(void *vd, double k_lower)
{
  dither_t *d = (dither_t *) vd;
  d->k_lower = (int) (k_lower * 65536);
}

double
dither_get_black_lower(void *vd)
{
  dither_t *d = (dither_t *) vd;
  return d->k_lower / 65536.0;
}

void
dither_set_black_upper(void *vd, double k_upper)
{
  dither_t *d = (dither_t *) vd;
  d->k_upper = (int) (k_upper * 65536);
}

double
dither_get_black_upper(void *vd)
{
  dither_t *d = (dither_t *) vd;
  return d->k_upper / 65536.0;
}

void
dither_set_black_levels(void *vd, double c, double m, double y)
{
  dither_t *d = (dither_t *) vd;
  d->k_clevel = (int) (c * 64);
  d->k_mlevel = (int) (m * 64);
  d->k_ylevel = (int) (y * 64);
}

void
dither_get_black_levels(void *vd, double *c, double *m, double *y)
{
  dither_t *d = (dither_t *) vd;
  *c = d->k_clevel / 64.0;
  *m = d->k_mlevel / 64.0;
  *y = d->k_ylevel / 64.0;
}

void
dither_set_randomizers(void *vd, int c, int m, int y, int k)
{
  dither_t *d = (dither_t *) vd;
  d->c_randomizer = c;
  d->m_randomizer = m;
  d->y_randomizer = y;
  d->k_randomizer = k;
}

void
dither_get_randomizers(void *vd, int *c, int *m, int *y, int *k)
{
  dither_t *d = (dither_t *) vd;
  *c = d->c_randomizer;
  *m = d->m_randomizer;
  *y = d->y_randomizer;
  *k = d->k_randomizer;
}

void
dither_set_ink_darkness(void *vd, double c, double m, double y)
{
  dither_t *d = (dither_t *) vd;
  d->c_darkness = (int) (c * 64);
  d->m_darkness = (int) (m * 64);
  d->y_darkness = (int) (y * 64);
}

void
dither_get_ink_darkness(void *vd, double *c, double *m, double *y)
{
  dither_t *d = (dither_t *) vd;
  *c = d->c_darkness / 64.0;
  *m = d->m_darkness / 64.0;
  *y = d->y_darkness / 64.0;
}

void
dither_set_light_inks(void *vd, double c, double m, double y)
{
  dither_t *d = (dither_t *) vd;
  d->lc_level = (int) (c * 65536);
  d->lm_level = (int) (m * 65536);
  d->ly_level = (int) (y * 65536);
}

void
dither_get_light_inks(void *vd, double *c, double *m, double *y)
{
  dither_t *d = (dither_t *) vd;
  *c = d->lc_level / 65536.0;
  *m = d->lm_level / 65536.0;
  *y = d->ly_level / 65536.0;
}

void
dither_set_c_levels(void *vd, int nlevels, double *levels)
{
  int i;
  dither_t *d = (dither_t *) vd;
  if (d->c_transitions)
    {
      free(d->c_transitions);
      free(d->c_levels);
    }
  d->c_transitions = malloc(nlevels * sizeof(int));
  d->c_levels = malloc(nlevels * sizeof(int));
  d->nc_l = nlevels;
  for (i = 0; i < nlevels; i++)
    {
      d->c_levels[i] = (int) (levels[i] * 65536);
      if (i > 0)
	d->c_transitions[i] = (d->c_levels[i] + d->c_levels[i-1]) / 2;
      else
	d->c_transitions[i] = 0;
    }
  d->nc_log = 0;
  while (nlevels > 1)
    {
      d->nc_log++;
      nlevels >>= 1;
    }
}
      

void
dither_set_lc_levels(void *vd, int nlevels, double *levels)
{
  int i;
  dither_t *d = (dither_t *) vd;
  if (d->lc_transitions)
    {
      free(d->lc_transitions);
      free(d->lc_levels);
    }
  d->lc_transitions = malloc(nlevels * sizeof(int));
  d->lc_levels = malloc(nlevels * sizeof(int));
  d->nlc_l = nlevels;
  for (i = 0; i < nlevels; i++)
    {
      d->lc_levels[i] = (int) (levels[i] * 65536);
      if (i > 0)
	d->lc_transitions[i] = (d->lc_levels[i] + d->lc_levels[i-1]) / 2;
      else
	d->lc_transitions[i] = 0;
    }
  d->nlc_log = 0;
  while (nlevels > 1)
    {
      d->nlc_log++;
      nlevels >>= 1;
    }
}

void
dither_set_m_levels(void *vd, int nlevels, double *levels)
{
  int i;
  dither_t *d = (dither_t *) vd;
  if (d->m_transitions)
    {
      free(d->m_transitions);
      free(d->m_levels);
    }
  d->m_transitions = malloc(nlevels * sizeof(int));
  d->m_levels = malloc(nlevels * sizeof(int));
  d->nm_l = nlevels;
  for (i = 0; i < nlevels; i++)
    {
      d->m_levels[i] = (int) (levels[i] * 65536);
      if (i > 0)
	d->m_transitions[i] = (d->m_levels[i] + d->m_levels[i-1]) / 2;
      else
	d->m_transitions[i] = 0;
    }
  d->nm_log = 0;
  while (nlevels > 1)
    {
      d->nm_log++;
      nlevels >>= 1;
    }
}

void
dither_set_lm_levels(void *vd, int nlevels, double *levels)
{
  int i;
  dither_t *d = (dither_t *) vd;
  if (d->lm_transitions)
    {
      free(d->lm_transitions);
      free(d->lm_levels);
    }
  d->lm_transitions = malloc(nlevels * sizeof(int));
  d->lm_levels = malloc(nlevels * sizeof(int));
  d->nlm_l = nlevels;
  for (i = 0; i < nlevels; i++)
    {
      d->lm_levels[i] = (int) (levels[i] * 65536);
      if (i > 0)
	d->lm_transitions[i] = (d->lm_levels[i] + d->lm_levels[i-1]) / 2;
      else
	d->lm_transitions[i] = 0;
    }
  d->nlm_log = 0;
  while (nlevels > 1)
    {
      d->nlm_log++;
      nlevels >>= 1;
    }
}
      
void
dither_set_y_levels(void *vd, int nlevels, double *levels)
{
  int i;
  dither_t *d = (dither_t *) vd;
  if (d->y_transitions)
    {
      free(d->y_transitions);
      free(d->y_levels);
    }
  d->y_transitions = malloc(nlevels * sizeof(int));
  d->y_levels = malloc(nlevels * sizeof(int));
  d->ny_l = nlevels;
  for (i = 0; i < nlevels; i++)
    {
      d->y_levels[i] = (int) (levels[i] * 65536);
      if (i > 0)
	d->y_transitions[i] = (d->y_levels[i] + d->y_levels[i-1]) / 2;
      else
	d->y_transitions[i] = 0;
    }
  d->ny_log = 0;
  while (nlevels > 1)
    {
      d->ny_log++;
      nlevels >>= 1;
    }
}
      
void
dither_set_ly_levels(void *vd, int nlevels, double *levels)
{
  int i;
  dither_t *d = (dither_t *) vd;
  if (d->ly_transitions)
    {
      free(d->ly_transitions);
      free(d->ly_levels);
    }
  d->ly_transitions = malloc(nlevels * sizeof(int));
  d->ly_levels = malloc(nlevels * sizeof(int));
  d->nly_l = nlevels;
  for (i = 0; i < nlevels; i++)
    {
      d->ly_levels[i] = (int) (levels[i] * 65536);
      if (i > 0)
	d->ly_transitions[i] = (d->ly_levels[i] + d->ly_levels[i-1]) / 2;
      else
	d->ly_transitions[i] = 0;
    }
  d->nly_log = 0;
  while (nlevels > 1)
    {
      d->nly_log++;
      nlevels >>= 1;
    }
}
      
void
dither_set_k_levels(void *vd, int nlevels, double *levels)
{
  int i;
  dither_t *d = (dither_t *) vd;
  if (d->k_transitions)
    {
      free(d->k_transitions);
      free(d->k_levels);
    }
  d->k_transitions = malloc(nlevels * sizeof(int));
  d->k_levels = malloc(nlevels * sizeof(int));
  d->nk_l = nlevels;
  for (i = 0; i < nlevels; i++)
    {
      d->k_levels[i] = (int) (levels[i] * 65536);
      if (i > 0)
	d->k_transitions[i] = (d->k_levels[i] + d->k_levels[i-1]) / 2;
      else
	d->k_transitions[i] = 0;
    }
  d->nk_log = 0;
  while (nlevels > 1)
    {
      d->nk_log++;
      nlevels >>= 1;
    }
}

void
free_dither(void *vd)
{
  dither_t *d = (dither_t *) vd;
  int i;
  int j;
  for (i = 0; i < ERROR_ROWS; i++)
    {
      for (j = 0; j < NCOLORS; j++)
	{
	  if (d->errs[i][j])
	    {
	      free(d->errs[i][j]);
	      d->errs[i][j] = NULL;
	    }
	}
    }
  free(d->c_transitions);
  free(d->c_levels);
  d->c_transitions = NULL;
  d->c_levels = NULL;
  free(d->lc_transitions);
  free(d->lc_levels);
  d->lc_transitions = NULL;
  d->lc_levels = NULL;
  free(d->m_transitions);
  free(d->m_levels);
  d->m_transitions = NULL;
  d->m_levels = NULL;
  free(d->lm_transitions);
  free(d->lm_levels);
  d->lm_transitions = NULL;
  d->lm_levels = NULL;
  free(d->y_transitions);
  free(d->y_levels);
  d->y_transitions = NULL;
  d->y_levels = NULL;
  free(d->ly_transitions);
  free(d->ly_levels);
  d->ly_transitions = NULL;
  d->ly_levels = NULL;
  free(d->k_transitions);
  free(d->k_levels);
  d->k_transitions = NULL;
  d->k_levels = NULL;
  free(d);
}

void
scale_dither(void *vd, int scale)
{
  dither_t *d = (dither_t *) vd;
  d->k_lower /= (scale * scale);
  d->k_upper /= scale;
  d->lc_level /= scale;
  d->lm_level /= scale;
  d->ly_level /= scale;
  switch (scale)
    {
    case 0:
    case 1:
      d->overdensity_bits = 0;
      break;
    case 2:
      d->overdensity_bits = 1;
      break;
    case 4:
      d->overdensity_bits = 2;
      break;
    case 8:
      d->overdensity_bits = 3;
      break;
    }
}
  

static int *
get_errline(dither_t *d, int row, int color)
{
  if (row < 0 || color < 0 || color >= NCOLORS)
    return NULL;
  if (d->errs[row & 1][color])
    return d->errs[row & 1][color];
  else
    {
      int size = (16 * ((d->dst_width + 7) / 8));
      d->errs[row & 1][color] = malloc(size * sizeof(int));
      memset(d->errs[row & 1][color], 0, size * sizeof(int));
      return d->errs[row & 1][color];
    }
}

/*
 * Dithering functions!
 *
 * We currently have four dithering functions:
 *
 * 1) dither_black produces a single level of black from grayscale input.
 *    This is used by most printers when printing grayscale.
 *
 * 2) dither_cmyk produces 3, 4, 6, or 7 color output (actually, it can
 *    deal with any combination of dark and light colored inks, but not a
 *    "light black" ink, although that would be nice :-) ).
 *
 * 3) dither_black_n produces n levels of black.  This was originally
 *    written by Mike Sweet for various HP printers, but it's useful for
 *    some of the newer Epson Stylus printers, too.  Used for variable dot
 *    size or multi-level inks such as the MIS archival inks.
 *
 * 4) dither_cmyk_n does likewise for color output.
 *
 * Many of these routines (in particular dither_cmyk and the 4-level functions)
 * have constants hard coded that are tuned for particular printers.  Needless
 * to say, this must go.
 *
 * The dithering algorithm is a basic error diffusion, with a few tweaks of
 * my own.  Error diffusion works by taking the output error at a given pixel
 * and "diffusing" it into surrounding pixels.  Output error is the difference
 * between the amount of ink output and the input level at each pixel.  For
 * simple printers, with one or four ink colors and only one dot size, the
 * amount of ink output is either 65536 (i. e. full output) or 0 (no output).
 * The difference between this and the input level is the error.  Normal
 * error diffusion adds part of this error to the adjoining pixels in the
 * next column and the next row (the algorithm simply scans each row in turn,
 * never backing up).  The error adds up until it reaches a threshold (half of
 * the full output level, or 32768), at which point a dot is output, the
 * output is subtracted from the current value, and the (now negative) error
 * is diffused similarly.
 *
 * Handling multiple output levels makes life a bit more complicated.  In
 * principle, it shouldn't be much harder: simply figure out what the ratio
 * between the available output levels is and have multiple thresholds.
 * In practice, getting these right involves a lot of trial and error.  The
 * other thing that's important is to maximize the number of dots that have
 * some ink.  This will reduce the amount of speckling.  More on this later.
 *
 * The next question: how do we handle black when printing in color?  Black
 * ink is much darker than colored inks.  It's possible to produce black by
 * adding some mixture of cyan, magenta, and yellow -- in principle.  In
 * practice, the black really isn't very black, and different inks and
 * different papers will produce different color casts.  However, by using
 * CMY to produce gray, we can output a lot more dots!  This makes for a much
 * smoother image.  What's more, one cyan, one magenta, and one yellow dot
 * produce less darkness than one black dot, so we're outputting that many
 * more dots.  Better yet, with 6 or 7 color printers, we have to output even
 * more light ink dots.  So Epson Stylus Photo printers can produce really
 * smooth grays -- if we do everything right.  The right idea is to use
 * CMY at lower black levels, and gradually mix in black as the overall
 * amount of ink increases, so the black dots don't really become visible
 * within the ink mass.
 *
 * I stated earlier that I've tweaked the basic error diffusion algorithm.
 * Here's what I've done to improve it:
 *
 * 1) We use a randomized threshold to decide when to print.  This does
 *    two things for us: it reduces the slightly squiggly diagonal lines
 *    that are the mark of error diffusion; and it allows us to lay down
 *    some ink even in very light areas near the edge of the image.
 *    The squiggly lines that error diffusion algorithms tend to generate
 *    are caused by the gradual accumulation of error.  This error is
 *    partially added horizontally and partially vertically.  The horizontal
 *    accumulation results in a dot eventually being printed.  The vertical
 *    accumulation results in a dot getting laid down in roughly the same
 *    horizontal position in the next row.  The diagonal squigglies result
 *    from the error being added to pixels one forward and one below the
 *    current pixel; these lines slope from the top right to the bottom left
 *    of the image.
 *
 *    Error diffusion also results in pale areas being completely white near
 *    the top left of the image (the origin of the printing coordinates).
 *    This is because enough error has to accumulate for anything at all to
 *    get printed.
 *
 *    Randomizing the threshold somewhat breaks up the diagonals to some
 *    degree by randomizing the exact location that the accumulated output
 *    crosses the threshold.  It reduces the false white areas by allowing
 *    some dots to be printed even when the accumulated output level is very
 *    low.  It doesn't result in excess ink because the full output level
 *    is still subtracted and diffused.
 *
 * 2) Alternating scan direction between rows (first row is scanned left to
 *    right, second is scanned right to left, and so on).  This also helps
 *    break up white areas, and it also seems to break up squigglies a bit.
 *    Furthermore, it eliminates directional biases in the horizontal
 *    direction.
 *
 * 3) Diffusing the error into more pixels.  Instead of diffusing the entire
 *    error into (X+1, Y) and (X, Y+1), we diffuse it into (X+1, Y),
 *    (X+K, Y+1), (X, Y+1), (X-K, Y+1) where K depends upon the output level
 *    (it never exceeds about 5 dots, and is greater at higher output
 *    levels).  This really reduces squigglies and graininess.
 *
 * 4) Don't lay down any colored ink if we're laying down black ink.  There's
 *    no point; the colored ink won't show.  We still pretend that we did
 *    for purposes of error diffusion (otherwise excessive error will build
 *    up, and will take a long time to clear, resulting in heavy bleeding of
 *    ink into surrounding areas, which is very ugly indeed), but we don't
 *    bother wasting the ink.
 *
 * 5) Oversampling.  This is how to print 1440x720 with Epson Stylus printers.
 *    Printing full density at 1440x720 will result in excess ink being laid
 *    down.  The trick is to print only every other dot.  We still compute
 *    the error as though we printed every dot.  It turns out that
 *    randomizing which dots are printed results in very speckled output.
 *
 * What about multiple output levels?  For 6 and 7 color printers, simply
 * using different threshold levels has a problem: the pale inks have trouble
 * being seen when a lot of darker ink is being printed.  So rather than
 * just using the output level of the particular color to decide which ink
 * to print, we look at the total density (sum of all output levels).
 * If the density's high enough, we prefer to use the dark ink.  Speckling
 * is less visible when there's a lot of ink, anyway.  I haven't yet figured
 * out what to do for multiple levels of one color.
 *
 * Speaking of 6 colors a bit more, I also determined (empirically!) that
 * simply subtracting the appropriate output level (full for the dark ink,
 * partial for the light ink) results in speckling.  Why?  Well, after printing
 * a dark dot, it takes longer to "recharge" the output level than after
 * printing a light dot.  What I do instead is compute a probability of
 * printing either a light or a dark dot when I exceed the threshold.  Picking
 * a random number decides which color ink to lay down.  However, rather than
 * subtracting the level appropriate for what I printed, I subtract a scaled
 * amount between the two levels.  The amount is scaled by the probability:
 * (L + P * (D - L)) where L is the light level, P is the probability of
 * printing dark, D is the dark level.  This also results in smoother output.
 *
 * You'll note that I haven't quoted a single source on color or printing
 * theory.  I simply did all of this empirically.
 *
 * There are various other tricks to reduce speckling.  One that I've seen
 * is to reduce the amount of ink printed in regions where one color
 * (particularly cyan, which is perceived as the darkest) is very pale.
 * This does reduce speckling all right, but it also results in strange
 * tonal curves and weird (to my eye) colors.
 *
 * Another, better trick is to print fixed patterns corresponding to given
 * output levels (basically a patterned screen).  This is probably much
 * better, but it's harder to get right in areas of rapidly varying color
 * and probably requires more lookup tables.  But it might be worth a shot.
 */

/*
 * 'dither_black()' - Dither grayscale pixels to black.
 */

void
dither_black(unsigned short     *gray,		/* I - Grayscale pixels */
	     int           	row,		/* I - Current Y coordinate */
	     void *vd,
	     unsigned char 	*black)		/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k,		/* Current black error */
		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dithering bitmask */
  dither_t *d = (dither_t *) vd;
  int terminate;
  int direction = row & 1 ? 1 : -1;

  bit = (direction == 1) ? 128 : 1 << (7 - ((d->dst_width - 1) & 7));
  x = (direction == 1) ? 0 : d->dst_width - 1;
  terminate = (direction == 1) ? d->dst_width : -1;

  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  length = (d->dst_width + 7) / 8;

  kerror0 = get_errline(d, row, ECOLOR_K);
  kerror1 = get_errline(d, row + 1, ECOLOR_K);
  memset(kerror1, 0, d->dst_width * sizeof(int));

  memset(black, 0, length);
  kptr = black;
  xerror = 0;
  if (direction == -1)
    {
      kerror0 += d->dst_width - 1;
      kerror1 += d->dst_width - 1;
      kptr = black + length - 1;
      xstep = -xstep; 
      gray += d->src_width - 1;
      xerror = ((d->dst_width - 1) * xmod) % d->dst_width;
      xmod = -xmod;
    }

  for (ditherbit = rand() & 0xffff, ditherk = kerror0[0];
       x != terminate;
       ditherbit = rand() & 0xffff,
       x += direction,
	 kerror0 += direction,
	 kerror1 += direction)
  {
    int offset;
    k = 65535 - *gray;
    offset = ((15 - (((k & 0xf000) >> 12))) * d->horizontal_overdensity) >> 1;
    k += ditherk / 8;
    if (x < offset)
      offset = x;
    else if (x > d->dst_width - offset - 1)
      offset = d->dst_width - x - 1;

    if (k > 32768 + ((ditherbit >> d->k_randomizer) -
		     (32768 >> d->k_randomizer)))
    {
      if (d->kbits++ == d->horizontal_overdensity)
	{
	  *kptr |= bit;
	  d->kbits = 1;
	}
      k -= 65535;
    }

    if (ditherbit & bit)
    {
      int tmpk = k;
      if (tmpk > 65535)
	tmpk = 65535;
      kerror1[-offset] += tmpk;
      kerror1[0] += 3 * tmpk;
      kerror1[offset] += tmpk;
      if (x > 0 && x < (d->dst_width - 1))
	ditherk    = kerror0[direction] + 3 * tmpk;
    }
    else
    {
      int tmpk = k;
      if (tmpk > 65535)
	tmpk = 65535;
      kerror1[-offset] += tmpk;
      kerror1[0] += tmpk;
      kerror1[offset] += tmpk;
      if (x > 0 && x < (d->dst_width - 1))
	ditherk    = kerror0[direction] + 5 * tmpk;
    }

    if (direction == 1)
      {
	if (bit == 1)
	  {
	    kptr ++;
	    bit       = 128;
	  }
	else
	  bit >>= 1;
      }
    else
      {
	if (bit == 128)
	  {
	    kptr --;
	    bit       = 1;
	  }
	else
	  bit <<= 1;
      }

    gray   += xstep;
    xerror += xmod;
    if (xerror >= d->dst_width)
      {
	xerror -= d->dst_width;
	gray   += direction;
      }
    else if (xerror < 0)
      {
	xerror += d->dst_width;
	gray   += direction;
      }      
  }
}

/*
 * 'dither_cmyk6()' - Dither RGB pixels to cyan, magenta, light cyan,
 * light magenta, yellow, and black.
 *
 * Added by Robert Krawitz <rlk@alum.mit.edu> August 30, 1999.
 *
 * Let's be really aggressive and use a single routine for ALL cmyk dithering,
 * including 6 and 7 color.
 *
 * Note that this is heavily tuned for Epson Stylus Photo printers.
 * This should be generalized for other CMYK and CcMmYK printers.  All
 * of these constants were empirically determined, and are subject to review.
 */

/*
 * Ratios of dark to light inks.  The darker ink should be DE / NU darker
 * than the light ink.
 *
 * It is essential to be very careful about use of parentheses with these
 * macros!
 *
 * Increasing the denominators results in use of more dark ink.  This creates
 * more saturated colors.
 */

/*
 * Lower and upper bounds for mixing CMY with K to produce gray scale.
 * Reducing KDARKNESS_LOWER results in more black being used with relatively
 * light grays, which causes speckling.  Increasing KDARKNESS_UPPER results
 * in more CMY being used in dark tones, which results in less pure black.
 * Decreasing the gap too much results in sharp crossover and stairstepping.
 */

/*
 * Randomizing values for deciding when to output a bit.  Normally with the
 * error diffusion algorithm a bit is not output until the accumulated value
 * of the pixel crosses a threshold.  This randomizes the threshold, which
 * results in fewer obnoxious diagonal jaggies in pale regions.  Smaller values
 * result in greater randomizing.  We use less randomness for black output
 * to avoid production of black speckles in light regions.
 */

#define UPDATE_COLOR(r)				\
do {						\
  o##r = r;					\
  r += dither##r / 8;				\
} while (0)

#define DO_PRINT_COLOR(color)				\
do {							\
  if (d->color##bits++ == d->horizontal_overdensity)	\
    {							\
      *color##ptr |= bit;				\
      d->color##bits = 1;				\
    }							\
} while(0)

#define PRINT_COLOR(color, r, R, d1, d2)			\
do {								\
  int comp0 = (32768 + ((ditherbit##d2 >> d->r##_randomizer) -	\
			(32768 >> d->r##_randomizer)));		\
  if (!l##color)						\
    {								\
      if (r > comp0)						\
	{							\
	  DO_PRINT_COLOR(r);					\
	  r -= 65536;						\
	}							\
    }								\
  else								\
    {								\
      int compare = (comp0 * d->l##r##_level) >> 16;		\
      if (r <= (d->l##r##_level))				\
	{							\
	  if (r > compare)					\
	    {							\
	      DO_PRINT_COLOR(l##r);				\
	      r -= d->l##r##_level << d->overdensity_bits;	\
	    }							\
	}							\
      else if (r > compare)					\
	{							\
	  int cutoff = ((density - d->l##r##_level) * 65536 /	\
			(65536 - d->l##r##_level));		\
	  int sub;						\
	  if (cutoff >= 0)					\
	    sub = d->l##r##_level +				\
	      (((65535ll - d->l##r##_level) * cutoff) >> 16);	\
	  else							\
	    sub = d->l##r##_level +				\
	      ((65535ll - d->l##r##_level) * cutoff / 65536);	\
	  if (ditherbit##d1 > cutoff)				\
	    {							\
	      DO_PRINT_COLOR(l##r);				\
	    }							\
	  else							\
	    {							\
	      DO_PRINT_COLOR(r);				\
	    }							\
	  if (sub < d->l##r##_level)				\
	    r -= d->l##r##_level << d->overdensity_bits;	\
	  else if (sub > 65535)					\
	    r -= 65536;						\
	  else							\
	    r -= sub;						\
	}							\
    }								\
} while (0)

#define UPDATE_DITHER(r, d2, x, width)					\
do {									\
  int offset = (255 - (((o##r & 0xff00) >> 8)))				\
					>> (5 - d->overdensity_bits);	\
  int tmp##r = r;							\
  if (tmp##r > 65535)							\
    tmp##r = 65535;							\
  if (offset > x)							\
    offset = x;								\
  else if (offset > d->dst_width - x - 1)				\
    offset = d->dst_width - x - 1;					\
  if (ditherbit##d2 & bit)						\
    {									\
      r##error1[-offset] += tmp##r;					\
      r##error1[0] += 3 * tmp##r;					\
      r##error1[offset] += tmp##r;					\
      if (x > 0 && x < (d->dst_width - 1))				\
	dither##r    = r##error0[direction] + 3 * tmp##r;		\
    }									\
  else									\
    {									\
      r##error1[-offset] += tmp##r;					\
      r##error1[0] +=  tmp##r;						\
      r##error1[offset] += tmp##r;					\
      if (x > 0 && x < (d->dst_width - 1))				\
	dither##r    = r##error0[direction] + 5 * tmp##r;		\
    }									\
} while (0)

void
dither_cmyk(unsigned short  *rgb,	/* I - RGB pixels */
	    int           row,		/* I - Current Y coordinate */
	    void *vd,
	    unsigned char *cyan,	/* O - Cyan bitmap pixels */
	    unsigned char *lcyan,	/* O - Light cyan bitmap pixels */
	    unsigned char *magenta,	/* O - Magenta bitmap pixels */
	    unsigned char *lmagenta,	/* O - Light magenta bitmap pixels */
	    unsigned char *yellow,	/* O - Yellow bitmap pixels */
	    unsigned char *lyellow,	/* O - Light yellow bitmap pixels */
	    unsigned char *black)	/* O - Black bitmap pixels */
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  long		c, m, y, k,	/* CMYK values */
		oc, om, ok, oy,
		divk;		/* Inverse of K */
  int		diff;		/* Average color difference */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*lmptr,		/* Current light magenta pixel */
		*lcptr,		/* Current light cyan pixel */
		*lyptr,		/* Current light yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc,	/* Next error value in buffer */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		dithery,	/* Next error value in buffer */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		ditherm,	/* Next error value in buffer */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */
  int nk;
  int ck;
  int bk;
  int ub, lb;
  int ditherbit0, ditherbit1, ditherbit2, ditherbit3;
  int density;
  dither_t *d = (dither_t *) vd;

  /*
   * If d->horizontal_overdensity is > 1, we want to output a bit only so many
   * times that a bit would be generated.  These serve as counters for making
   * that decision.  We make these variable static rather than reinitializing
   * at zero each line to avoid having a line of bits near the edge of the
   * image.
   */

#ifdef PRINT_DEBUG
  long long odk, odc, odm, ody, dk, dc, dm, dy, xk, xc, xm, xy, yc, ym, yy;
  FILE *dbg;
#endif

  int terminate;
  int direction = row & 1 ? 1 : -1;

  bit = (direction == 1) ? 128 : 1 << (7 - ((d->dst_width - 1) & 7));
  x = (direction == 1) ? 0 : d->dst_width - 1;
  terminate = (direction == 1) ? d->dst_width : -1;

  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  length = (d->dst_width + 7) / 8;

  cerror0 = get_errline(d, row, ECOLOR_C);
  cerror1 = get_errline(d, row + 1, ECOLOR_C);

  merror0 = get_errline(d, row, ECOLOR_M);
  merror1 = get_errline(d, row + 1, ECOLOR_M);

  yerror0 = get_errline(d, row, ECOLOR_Y);
  yerror1 = get_errline(d, row + 1, ECOLOR_Y);

  kerror0 = get_errline(d, row, ECOLOR_K);
  kerror1 = get_errline(d, row + 1, ECOLOR_K);
  memset(kerror1, 0, d->dst_width * sizeof(int));
  memset(cerror1, 0, d->dst_width * sizeof(int));
  memset(merror1, 0, d->dst_width * sizeof(int));
  memset(yerror1, 0, d->dst_width * sizeof(int));
  cptr = cyan;
  mptr = magenta;
  yptr = yellow;
  lcptr = lcyan;
  lmptr = lmagenta;
  lyptr = lyellow;
  kptr = black;
  xerror = 0;
  if (direction == -1)
    {
      cerror0 += d->dst_width - 1;
      cerror1 += d->dst_width - 1;
      merror0 += d->dst_width - 1;
      merror1 += d->dst_width - 1;
      yerror0 += d->dst_width - 1;
      yerror1 += d->dst_width - 1;
      kerror0 += d->dst_width - 1;
      kerror1 += d->dst_width - 1;
      cptr = cyan + length - 1;
      if (lcptr)
	lcptr = lcyan + length - 1;
      mptr = magenta + length - 1;
      if (lmptr)
	lmptr = lmagenta + length - 1;
      yptr = yellow + length - 1;
      if (lyptr)
	lyptr = lyellow + length - 1;
      if (kptr)
	kptr = black + length - 1;
      xstep = -xstep;
      rgb += 3 * (d->src_width - 1);
      xerror = ((d->dst_width - 1) * xmod) % d->dst_width;
      xmod = -xmod;
    }

  memset(cyan, 0, length);
  if (lcyan)
    memset(lcyan, 0, length);
  memset(magenta, 0, length);
  if (lmagenta)
    memset(lmagenta, 0, length);
  memset(yellow, 0, length);
  if (lyellow)
    memset(lyellow, 0, length);
  if (black)
    memset(black, 0, length);

#ifdef PRINT_DEBUG
  dbg = fopen("/mnt1/dbg", "a");
#endif

  /*
   * Main loop starts here!
   */
  for (ditherbit = rand(),
	 ditherc = cerror0[0], ditherm = merror0[0], dithery = yerror0[0],
	 ditherk = kerror0[0],
	 ditherbit0 = ditherbit & 0xffff,
	 ditherbit1 = ((ditherbit >> 8) & 0xffff),
	 ditherbit2 = (((ditherbit >> 16) & 0x7fff) +
		       ((ditherbit & 0x100) << 7)),
	 ditherbit3 = (((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
		       ((ditherbit >> 8) & 0xff00));
       x != terminate;
       x += direction,
	 cerror0 += direction,
	 cerror1 += direction,
	 merror0 += direction,
	 merror1 += direction,
	 yerror0 += direction,
	 yerror1 += direction,
	 kerror0 += direction,
	 kerror1 += direction)
  {

   /*
    * First compute the standard CMYK separation color values...
    */
		   
    int maxlevel;
    int ak;
    int kdarkness;

    c = 65535 - (unsigned) rgb[0];
    m = 65535 - (unsigned) rgb[1];
    y = 65535 - (unsigned) rgb[2];
    oc = c;
    om = m;
    oy = y;
    k = MIN(c, MIN(m, y));
#ifdef PRINT_DEBUG
    xc = c;
    xm = m;
    xy = y;
    xk = k;
    yc = c;
    ym = m;
    yy = y;
#endif
    maxlevel = MAX(c, MAX(m, y));

    if (black != NULL)
    {
     /*
      * Since we're printing black, adjust the black level based upon
      * the amount of color in the pixel (colorful pixels get less black)...
      */
      int xdiff = (abs(c - m) + abs(c - y) + abs(m - y)) / 3;
      int tk;

      diff = 65536 - xdiff;
      diff = ((long long) diff * (long long) diff * (long long) diff) >> 32;
      diff--;
      if (diff < 0)
	diff = 0;
      k    = (int) (((unsigned) diff * (unsigned) k) >> 16);
      ak = k;
      divk = 65535 - k;
      if (divk == 0)
        c = m = y = 0;	/* Grayscale */
      else
      {
       /*
        * Full color; update the CMY values for the black value and reduce
        * CMY as necessary to give better blues, greens, and reds... :)
        */
	unsigned ck = c - k;
	unsigned mk = m - k;
	unsigned yk = y - k;

        c  = ((unsigned) (65535 - ((rgb[2] + rgb[1]) >> 3))) * ck /
	  (unsigned) divk;
        m  = ((unsigned) (65535 - ((rgb[1] + rgb[0]) >> 3))) * mk /
	  (unsigned) divk;
        y  = ((unsigned) (65535 - ((rgb[0] + rgb[2]) >> 3))) * yk /
	  (unsigned) divk;
      }
#ifdef PRINT_DEBUG
      yc = c;
      ym = m;
      yy = y;
#endif

      /*
       * kdarkness is an artificially computed darkness value for deciding
       * how much black vs. CMY to use for the k component.  This is
       * empirically determined.
       */
      ok = k;
      nk = k + (ditherk) / 8;
      tk = (((c * d->c_darkness) + (m * d->m_darkness) + (y * d->y_darkness))
	    >> 6);
      kdarkness = MAX(tk, ak);
      if (kdarkness < d->k_upper)
	{
	  int rb;
	  ub = d->k_upper;
	  lb = d->k_lower;
	  rb = ub - lb;
#ifdef PRINT_DEBUG
	  fprintf(dbg, "Black: kd %d ub %d lb %d rb %d test %d range %d\n",
		  kdarkness, ub, lb, rb, ditherbit % rb, kdarkness - lb);
#endif
	  if (kdarkness <= lb)
	    {
	      bk = 0;
	      ub = 0;
	      lb = 1;
	    }
	  else if (kdarkness < ub)
	    {
	      if (rb == 0 || (ditherbit % rb) < (kdarkness - lb))
		bk = nk;
	      else
		bk = 0;
	    }
	  else
	    {
	      ub = 1;
	      lb = 1;
	      bk = nk;
	    }
	}
      else
	{
#ifdef PRINT_DEBUG
	  fprintf(dbg, "Black real\n");
#endif
	  bk = nk;
	}
      ck = nk - bk;
    
      c += (d->k_clevel * ck) >> 6;
      m += (d->k_mlevel * ck) >> 6;
      y += (d->k_ylevel * ck) >> 6;

      /*
       * Don't allow cmy to grow without bound.
       */
      if (c > 65535)
	c = 65535;
      if (m > 65535)
	m = 65535;
      if (y > 65535)
	y = 65535;
      k = bk;
#ifdef PRINT_DEBUG
      odk = ditherk;
      dk = k;
#endif
      if (k > (32767 + ((ditherbit0 >> d->k_randomizer) -
			(32768 >> d->k_randomizer))))
	{
	  DO_PRINT_COLOR(k);
	  k -= 65535;
	}

      UPDATE_DITHER(k, 1, x, d->src_width);
    }
    else
    {
     /*
      * We're not printing black, but let's adjust the CMY levels to produce
      * better reds, greens, and blues...
      */

      unsigned ck = c - k;
      unsigned mk = m - k;
      unsigned yk = y - k;

      ok = 0;
      c  = ((unsigned) (65535 - rgb[1] / 4)) * ck / 65535 + k;
      m  = ((unsigned) (65535 - rgb[2] / 4)) * mk / 65535 + k;
      y  = ((unsigned) (65535 - rgb[0] / 4)) * yk / 65535 + k;
    }

    density = (c + m + y) >> d->overdensity_bits;
    UPDATE_COLOR(c);
    UPDATE_COLOR(m);
    UPDATE_COLOR(y);
    density += (c + m + y) >> d->overdensity_bits;
/*     density >>= 1; */

    if (!kptr || !(*kptr & bit))
      {
	PRINT_COLOR(cyan, c, C, 1, 2);
	PRINT_COLOR(magenta, m, M, 2, 3);
	PRINT_COLOR(yellow, y, Y, 3, 0);
      }

    UPDATE_DITHER(c, 2, x, d->dst_width);
    UPDATE_DITHER(m, 3, x, d->dst_width);
    UPDATE_DITHER(y, 0, x, d->dst_width);

    /*****************************************************************
     * Advance the loop
     *****************************************************************/
#ifdef PRINT_DEBUG
    fprintf(dbg, "   x %d y %d  r %d g %d b %d  xc %lld xm %lld xy %lld yc "
	    "%lld ym %lld yy %lld xk %lld  diff %lld divk %lld  oc %lld om "
	    "%lld oy %lld ok %lld  c %lld m %lld y %lld k %lld  %c%c%c%c%c%c%c"
	    "  dk %lld dc %lld dm %lld dy %lld  kd %d ck %d bk %d nk %d ub %d "
	    "lb %d\n",
	    x, row,
	    rgb[0], rgb[1], rgb[2],
	    xc, xm, xy, yc, ym, yy, xk, diff, divk,
	    oc, om, oy, ok,
	    dc, dm, dy, dk,
	    (*cptr & bit) ? 'c' : ' ',
	    (lcyan && (*lcptr & bit)) ? 'C' : ' ',
	    (*mptr & bit) ? 'm' : ' ',
	    (lmagenta && (*lmptr & bit)) ? 'M' : ' ',
	    (*yptr & bit) ? 'y' : ' ',
	    (lyellow && (*lyptr & bit)) ? 'Y' : ' ',
	    (black && (*kptr & bit)) ? 'k' : ' ',
	    odk, odc, odm, ody,
	    kdarkness, ck, bk, nk, ub, lb);
    fprintf(dbg, "x %d dir %d c %x %x m %x %x y %x %x k %x %x rgb %x bit %x\n",
	    x, direction, cptr, cyan, mptr, magenta, yptr, yellow, kptr, black,
	    rgb, bit);
#endif

    ditherbit = rand();
    ditherbit0 = ditherbit & 0xffff;
    ditherbit1 = ((ditherbit >> 8) & 0xffff);
    ditherbit2 = ((ditherbit >> 16) & 0x7fff) + ((ditherbit & 0x100) << 7);
    ditherbit3 = ((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
      ((ditherbit >> 8) & 0xff00);
    if (direction == 1)
      {
	if (bit == 1)
	  {
	    cptr ++;
	    if (lcptr)
	      lcptr ++;
	    mptr ++;
	    if (lmptr)
	      lmptr ++;
	    yptr ++;
	    if (lyptr)
	      lyptr ++;
	    if (kptr)
	      kptr ++;
	    bit       = 128;
	  }
	else
	  bit >>= 1;
      }
    else
      {
	if (bit == 128)
	  {
	    cptr --;
	    if (lcptr)
	      lcptr --;
	    mptr --;
	    if (lmptr)
	      lmptr --;
	    yptr --;
	    if (lyptr)
	      lyptr --;
	    if (kptr)
	      kptr --;
	    bit       = 1;
	  }
	else
	  bit <<= 1;
      }

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= d->dst_width)
      {
	xerror -= d->dst_width;
	rgb    += 3 * direction;
      }
    else if (xerror < 0)
      {
	xerror += d->dst_width;
	rgb    += 3 * direction;
      }      
  }
  /*
   * Main loop ends here!
   */
#ifdef PRINT_DEBUG
  fprintf(dbg, "\n");
  fclose(dbg);
#endif
}


/*
 * Constants for 4-level dithering functions...
 * NOTE that these constants are HP-specific!
 */

/*
 * 'dither_black_n()' - Dither grayscale pixels to n levels of black.
 */

void
dither_black_n(unsigned short   *gray,		/* I - Grayscale pixels */
	       int           	row,		/* I - Current Y coordinate */
	       void 		*vd,
	       unsigned char 	*black,		/* O - Black bitmap pixels */
	       int		use_log_encoding)
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  unsigned char	bit,		/* Current bit */
		*kptr;		/* Current black pixel */
  int		k,		/* Current black error */
		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dithering bitmask */
  dither_t *d = (dither_t *) vd;
  int terminate;
  int direction = row & 1 ? 1 : -1;

  bit = (direction == 1) ? 128 : 1 << (7 - ((d->dst_width - 1) & 7));
  x = (direction == 1) ? 0 : d->dst_width - 1;
  terminate = (direction == 1) ? d->dst_width : -1;

  xstep  = d->src_width / d->dst_width;
  xmod   = d->src_width % d->dst_width;
  length = (d->dst_width + 7) / 8;

  kerror0 = get_errline(d, row, ECOLOR_K);
  kerror1 = get_errline(d, row + 1, ECOLOR_K);
  memset(kerror1, 0, d->dst_width * sizeof(int));

  memset(black, 0, length * d->nk_log);
  kptr = black;
  xerror = 0;
  if (direction == -1)
    {
      kerror0 += d->dst_width - 1;
      kerror1 += d->dst_width - 1;
      kptr = black + length - 1;
      xstep = -xstep;
      gray += d->src_width - 1;
      xerror = ((d->dst_width - 1) * xmod) % d->dst_width;
      xmod = -xmod;
    }

  for (ditherbit = rand() & 0xffff, ditherk = kerror0[0];
       x != terminate;
       ditherbit = rand() & 0xffff,
	 x += direction,
	 kerror0 += direction,
	 kerror1 += direction)
  {
    int ok;
    int i;
    int offset;
    k = 65535 - *gray;
    offset = ((15 - (((k & 0xf000) >> 12))) * d->horizontal_overdensity) >> 1;
    ok = k;
    k += ditherk / 8;
    if (x < offset)
      offset = x;
    else if (x > d->dst_width - offset - 1)
      offset = d->dst_width - x - 1;

    if (use_log_encoding)
      {
	for (i = d->nk_l - 1; i > 0; i--)
	  {
	    if (k > d->k_transitions[i])
	      {
		if (d->kbits++ == d->horizontal_overdensity)
		  {
		    int j;
		    unsigned char *tptr = kptr;
		    for (j = 1; j <= i; j += j, tptr += length)
		      {
			if (j & i)
			  *tptr |= bit;
		      }
		    d->kbits = 1;
		  }
		k -= d->k_levels[i];
		break;
	      }
	  }
      }
    else
      {
	unsigned char *tptr = kptr;
	for (i = d->nk_l - 1; i > 0; i--)
	  {
	    if (k > d->k_transitions[i])
	      {
		if (d->kbits++ == d->horizontal_overdensity)
		  {
		    *tptr |= bit;
		    d->kbits = 1;
		  }
		k -= d->k_levels[i];
		break;
	      }
	    tptr += length;
	  }
      }

    if (ditherbit & bit)
    {
      int tmpk = k;
      if (tmpk > 65535)
	tmpk = 65535;
      kerror1[-offset] += tmpk;
      kerror1[0] += 3 * tmpk;
      kerror1[offset] += tmpk;
      if (x > 0 && x < (d->dst_width - 1))
	ditherk    = kerror0[direction] + 3 * tmpk;
    }
    else
    {
      int tmpk = k;
      if (tmpk > 65535)
	tmpk = 65535;
      kerror1[-offset] += tmpk;
      kerror1[0] += tmpk;
      kerror1[offset] += tmpk;
      if (x > 0 && x < (d->dst_width - 1))
	ditherk    = kerror0[direction] + 5 * tmpk;
    }

    if (direction == 1)
      {
	if (bit == 1)
	  {
	    kptr ++;
	    bit       = 128;
	  }
	else
	  bit >>= 1;
      }
    else
      {
	if (bit == 128)
	  {
	    kptr --;
	    bit       = 1;
	  }
	else
	  bit <<= 1;
      }

    gray   += xstep;
    xerror += xmod;
    if (xerror >= d->dst_width)
      {
	xerror -= d->dst_width;
	gray   += direction;
      }
    else if (xerror < 0)
      {
	xerror += d->dst_width;
	gray   += direction;
      }      
  }
}

/*
 * 'dither_cmyk_n()' - Dither RGB pixels to n levels of cyan, magenta, yellow,
 *                     and black.
 */

#define DO_PRINT_COLOR_4(base, r, ratio)			\
do {								\
  int i;							\
  if (use_log_encoding)						\
    {								\
      for (i = d->n##r##_l - 1; i > 0; i--)			\
	{							\
	  if (base > d->r##_transitions[i])			\
	    {							\
	      if (d->r##bits++ == d->horizontal_overdensity)	\
		{						\
		  int j;					\
		  unsigned char *tptr = r##ptr;			\
		  for (j = 1; j <= i; j += j, tptr += length)	\
		    {						\
		      if (j & i)				\
			*t##ptr |= bit;				\
		    }						\
		  d->r##bits = 1;				\
		}						\
	      base -= d->r##_levels[i];				\
	      break;						\
	    }							\
	}							\
    }								\
  else								\
    {								\
      unsigned char *tptr = r##ptr;				\
      for (i = d->n##r##_l - 1; i > 0; i--)			\
	{							\
	  if (base > d->r##_transitions[i])			\
	    {							\
	      if (d->r##bits++ == d->horizontal_overdensity)	\
		{						\
		  *t##ptr |= bit;				\
		  d->r##bits = 1;				\
		}						\
	      base -= d->r##_levels[i];				\
	      break;						\
	    }							\
	  tptr += length;					\
	}							\
    }								\
} while (0)

#define PRINT_COLOR_4(color, r, R, d1, d2)				\
do {									\
  int comp0 = (32768 + ((ditherbit##d2 >> d->r##_randomizer) -		\
			(32768 >> d->r##_randomizer)));			\
  if (!l##color)							\
    {									\
      DO_PRINT_COLOR_4(r, r, 1);					\
    }									\
  else									\
    {									\
      int compare = comp0 * d->l##r##_level >> 16;			\
      if (r <= (d->l##r##_level))					\
	{								\
	  if (r > compare)						\
	    {								\
	      DO_PRINT_COLOR_4(r, l##r, d->l##r##_level / 65536);	\
	    }								\
	}								\
      else if (r > compare)						\
	{								\
	  int cutoff = ((density - d->l##r##_level) * 65536 /		\
			(65536 - d->l##r##_level));			\
	  int sub;							\
	  if (cutoff >= 0)						\
	    sub = d->l##r##_level +					\
	      (((65535ll - d->l##r##_level) * cutoff) >> 16);		\
	  else								\
	    sub = d->l##r##_level +					\
	      ((65535ll - d->l##r##_level) * cutoff / 65536);		\
	  if (ditherbit##d1 > cutoff)					\
	    {								\
	      DO_PRINT_COLOR_4(r, l##r, d->l##r##_level / 65536);	\
	    }								\
	  else								\
	    {								\
	      DO_PRINT_COLOR_4(r, r, 1);				\
	    }								\
	}								\
    }									\
} while (0)

void
dither_cmyk_n(unsigned short  *rgb,	/* I - RGB pixels */
	      int           row,	/* I - Current Y coordinate */
	      void 	    *vd,
	      unsigned char *cyan,	/* O - Cyan bitmap pixels */
	      unsigned char *lcyan,	/* O - Light cyan bitmap pixels */
	      unsigned char *magenta,	/* O - Magenta bitmap pixels */
	      unsigned char *lmagenta,	/* O - Light magenta bitmap pixels */
	      unsigned char *yellow,	/* O - Yellow bitmap pixels */
	      unsigned char *lyellow,	/* O - Light yellow bitmap pixels */
	      unsigned char *black,	/* O - Black bitmap pixels */
	      int	    use_log_encoding)
{
  int		x,		/* Current X coordinate */
		xerror,		/* X error count */
		xstep,		/* X step */
		xmod,		/* X error modulus */
		length;		/* Length of output bitmap in bytes */
  int		c, m, y, k,	/* CMYK values */
		oc, om, ok, oy,
		divk;		/* Inverse of K */
  int   	diff;		/* Average color difference */
  unsigned char	bit,		/* Current bit */
		*cptr,		/* Current cyan pixel */
		*mptr,		/* Current magenta pixel */
		*yptr,		/* Current yellow pixel */
		*lmptr,		/* Current light magenta pixel */
		*lcptr,		/* Current light cyan pixel */
		*lyptr,		/* Current light yellow pixel */
		*kptr;		/* Current black pixel */
  int		ditherc,	/* Next error value in buffer */
		*cerror0,	/* Pointer to current error row */
		*cerror1;	/* Pointer to next error row */
  int		dithery,	/* Next error value in buffer */
		*yerror0,	/* Pointer to current error row */
		*yerror1;	/* Pointer to next error row */
  int		ditherm,	/* Next error value in buffer */
		*merror0,	/* Pointer to current error row */
		*merror1;	/* Pointer to next error row */
  int		ditherk,	/* Next error value in buffer */
		*kerror0,	/* Pointer to current error row */
		*kerror1;	/* Pointer to next error row */
  int		ditherbit;	/* Random dither bitmask */
  int nk;
  int ck;
  int bk;
  int ub, lb;
  int ditherbit0, ditherbit1, ditherbit2, ditherbit3;
  int	density;
  dither_t *d = (dither_t *) vd;

  /*
   * If d->horizontal_overdensity is > 1, we want to output a bit only so many
   * times that a bit would be generated.  These serve as counters for making
   * that decision.  We make these variable static rather than reinitializing
   * at zero each line to avoid having a line of bits near the edge of the
   * image.
   */
  int terminate;
  int direction = row & 1 ? 1 : -1;

  bit = (direction == 1) ? 128 : 1 << (7 - ((d->dst_width - 1) & 7));
  x = (direction == 1) ? 0 : d->dst_width - 1;
  terminate = (direction == 1) ? d->dst_width : -1;

  xstep  = 3 * (d->src_width / d->dst_width);
  xmod   = d->src_width % d->dst_width;
  length = (d->dst_width + 7) / 8;

  cerror0 = get_errline(d, row, ECOLOR_C);
  cerror1 = get_errline(d, row + 1, ECOLOR_C);

  merror0 = get_errline(d, row, ECOLOR_M);
  merror1 = get_errline(d, row + 1, ECOLOR_M);

  yerror0 = get_errline(d, row, ECOLOR_Y);
  yerror1 = get_errline(d, row + 1, ECOLOR_Y);

  kerror0 = get_errline(d, row, ECOLOR_K);
  kerror1 = get_errline(d, row + 1, ECOLOR_K);
  memset(kerror1, 0, d->dst_width * sizeof(int));
  memset(cerror1, 0, d->dst_width * sizeof(int));
  memset(merror1, 0, d->dst_width * sizeof(int));
  memset(yerror1, 0, d->dst_width * sizeof(int));
  cptr = cyan;
  mptr = magenta;
  yptr = yellow;
  lcptr = lcyan;
  lmptr = lmagenta;
  lyptr = lyellow;
  kptr = black;
  xerror = 0;
  if (direction == -1)
    {
      cerror0 += d->dst_width - 1;
      cerror1 += d->dst_width - 1;
      merror0 += d->dst_width - 1;
      merror1 += d->dst_width - 1;
      yerror0 += d->dst_width - 1;
      yerror1 += d->dst_width - 1;
      kerror0 += d->dst_width - 1;
      kerror1 += d->dst_width - 1;
      cptr = cyan + length - 1;
      if (lcptr)
	lcptr = lcyan + length - 1;
      mptr = magenta + length - 1;
      if (lmptr)
	lmptr = lmagenta + length - 1;
      yptr = yellow + length - 1;
      if (lyptr)
	lyptr = lyellow + length - 1;
      if (kptr)
	kptr = black + length - 1;
      xstep = -xstep;
      rgb += 3 * (d->src_width - 1);
      xerror = ((d->dst_width - 1) * xmod) % d->dst_width;
      xmod = -xmod;
    }

  memset(cyan, 0, length * d->nc_log);
  if (lcyan)
    memset(lcyan, 0, length * d->nlc_log);
  memset(magenta, 0, length * d->nm_log);
  if (lmagenta)
    memset(lmagenta, 0, length * d->nlm_log);
  memset(yellow, 0, length * d->ny_log);
  if (lyellow)
    memset(lyellow, 0, length * d->nly_log);
  if (black)
    memset(black, 0, length * d->nk_log);

  /*
   * Main loop starts here!
   */
  for (ditherbit = rand(),
	 ditherc = cerror0[0], ditherm = merror0[0], dithery = yerror0[0],
	 ditherk = kerror0[0],
	 ditherbit0 = ditherbit & 0xffff,
	 ditherbit1 = ((ditherbit >> 8) & 0xffff),
	 ditherbit2 = (((ditherbit >> 16) & 0x7fff) +
		       ((ditherbit & 0x100) << 7)),
	 ditherbit3 = (((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
		       ((ditherbit >> 8) & 0xff00));
       x != terminate;
       x += direction,
	 cerror0 += direction,
	 cerror1 += direction,
	 merror0 += direction,
	 merror1 += direction,
	 yerror0 += direction,
	 yerror1 += direction,
	 kerror0 += direction,
	 kerror1 += direction)
  {

   /*
    * First compute the standard CMYK separation color values...
    */
		   
    int maxlevel;
    int ak;
    int kdarkness;

    c = 65535 - (unsigned) rgb[0];
    m = 65535 - (unsigned) rgb[1];
    y = 65535 - (unsigned) rgb[2];
    oc = c;
    om = m;
    oy = y;
    k = MIN(c, MIN(m, y));
    maxlevel = MAX(c, MAX(m, y));

    if (black != NULL)
    {
     /*
      * Since we're printing black, adjust the black level based upon
      * the amount of color in the pixel (colorful pixels get less black)...
      */
      int tk;
      int xdiff = (abs(c - m) + abs(c - y) + abs(m - y)) / 3;

      diff = 65536 - xdiff;
      diff = ((long long) diff * (long long) diff * (long long) diff) >> 32;
      diff--;
      if (diff < 0)
	diff = 0;
      k    = (int) (((unsigned) diff * (unsigned) k) >> 16);
      ak = k;
      divk = 65535 - k;
      if (divk == 0)
        c = m = y = 0;	/* Grayscale */
      else
      {
       /*
        * Full color; update the CMY values for the black value and reduce
        * CMY as necessary to give better blues, greens, and reds... :)
        */
	unsigned ck = c - k;
	unsigned mk = m - k;
	unsigned yk = y - k;

        c  = ((unsigned) (65535 - ((rgb[2] + rgb[1]) >> 3))) * ck /
	  (unsigned) divk;
        m  = ((unsigned) (65535 - ((rgb[1] + rgb[0]) >> 3))) * mk /
	  (unsigned) divk;
        y  = ((unsigned) (65535 - ((rgb[0] + rgb[2]) >> 3))) * yk /
	  (unsigned) divk;
      }

      /*
       * kdarkness is an artificially computed darkness value for deciding
       * how much black vs. CMY to use for the k component.  This is
       * empirically determined.
       */
      ok = k;
      nk = k + (ditherk) / 8;
      tk = (((c * d->c_darkness) + (m * d->m_darkness) + (y * d->y_darkness))
	    >> 6);
      kdarkness = MAX(tk, ak);
      if (kdarkness < d->k_upper)
	{
	  int rb;
	  ub = d->k_upper;
	  lb = d->k_lower;
	  rb = ub - lb;
	  if (kdarkness <= lb)
	    {
	      bk = 0;
	      ub = 0;
	      lb = 1;
	    }
	  else if (kdarkness < ub)
	    {
	      if (rb == 0 || (ditherbit % rb) < (kdarkness - lb))
		bk = nk;
	      else
		bk = 0;
	    }
	  else
	    {
	      ub = 1;
	      lb = 1;
	      bk = nk;
	    }
	}
      else
	{
	  bk = nk;
	}
      ck = nk - bk;
    
      /*
       * These constants are empirically determined to produce a CMY value
       * that looks reasonably gray and is reasonably well balanced tonally
       * with black.  As usual, this is very ad hoc and needs to be
       * generalized.
       */
      c += (d->k_clevel * ck) >> 6;
      m += (d->k_mlevel * ck) >> 6;
      y += (d->k_ylevel * ck) >> 6;

      /*
       * Don't allow cmy to grow without bound.
       */
      if (c > 65535)
	c = 65535;
      if (m > 65535)
	m = 65535;
      if (y > 65535)
	y = 65535;
      k = bk;
      DO_PRINT_COLOR_4(k, k, 1);
      UPDATE_DITHER(k, 1, x, d->src_width);
    }
    else
    {
     /*
      * We're not printing black, but let's adjust the CMY levels to produce
      * better reds, greens, and blues...
      */

      unsigned ck = c - k;
      unsigned mk = m - k;
      unsigned yk = y - k;

      ok = 0;
      c  = ((unsigned) (65535 - rgb[1] / 4)) * ck / 65535 + k;
      m  = ((unsigned) (65535 - rgb[2] / 4)) * mk / 65535 + k;
      y  = ((unsigned) (65535 - rgb[0] / 4)) * yk / 65535 + k;
    }

    density = (c + m + y) >> d->overdensity_bits;
    UPDATE_COLOR(c);
    UPDATE_COLOR(m);
    UPDATE_COLOR(y);
    density += (c + m + y) >> d->overdensity_bits;
/*     density >>= 1; */

    if (!kptr || !(*kptr & bit))
      {
	PRINT_COLOR_4(cyan, c, C, 1, 2);
	PRINT_COLOR_4(magenta, m, M, 2, 3);
	PRINT_COLOR_4(yellow, y, Y, 3, 0);
      }

    UPDATE_DITHER(c, 2, x, d->dst_width);
    UPDATE_DITHER(m, 3, x, d->dst_width);
    UPDATE_DITHER(y, 0, x, d->dst_width);

    /*****************************************************************
     * Advance the loop
     *****************************************************************/

    ditherbit = rand();
    ditherbit0 = ditherbit & 0xffff;
    ditherbit1 = ((ditherbit >> 8) & 0xffff);
    ditherbit2 = ((ditherbit >> 16) & 0x7fff) + ((ditherbit & 0x100) << 7);
    ditherbit3 = ((ditherbit >> 24) & 0x7f) + ((ditherbit & 1) << 7) +
      ((ditherbit >> 8) & 0xff00);
    if (direction == 1)
      {
	if (bit == 1)
	  {
	    cptr ++;
	    if (lcptr)
	      lcptr ++;
	    mptr ++;
	    if (lmptr)
	      lmptr ++;
	    yptr ++;
	    if (lyptr)
	      lyptr ++;
	    if (kptr)
	      kptr ++;
	    bit       = 128;
	  }
	else
	  bit >>= 1;
      }
    else
      {
	if (bit == 128)
	  {
	    cptr --;
	    if (lcptr)
	      lcptr --;
	    mptr --;
	    if (lmptr)
	      lmptr --;
	    yptr --;
	    if (lyptr)
	      lyptr --;
	    if (kptr)
	      kptr --;
	    bit       = 1;
	  }
	else
	  bit <<= 1;
      }

    rgb    += xstep;
    xerror += xmod;
    if (xerror >= d->dst_width)
      {
	xerror -= d->dst_width;
	rgb    += 3 * direction;
      }
    else if (xerror < 0)
      {
	xerror += d->dst_width;
	rgb    += 3 * direction;
      }      
  }
  /*
   * Main loop ends here!
   */
}
