/*
 * "$Id$"
 *
 *   libgimpprint header.
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
 *   See ChangeLog
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifndef GIMP_PRINT_INTERNAL_WEAVE_H
#define GIMP_PRINT_INTERNAL_WEAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MAX_WEAVE (8)


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
} stpi_weave_t;

typedef struct			/* Weave parameters for a specific pass */
{
  int pass;			/* Absolute pass number */
  int missingstartrows;		/* All other values the same as weave_t */
  int logicalpassstart;
  int physpassstart;
  int physpassend;
  int subpass;
} stpi_pass_t;

typedef struct {		/* Offsets from the start of each line */
  int ncolors;
  unsigned long *v;		/* (really pass) */
} stpi_lineoff_t;

typedef struct {		/* Is this line (really pass) active? */
  int ncolors;
  char *v;
} stpi_lineactive_t;

typedef struct {		/* number of rows for a pass */
  int ncolors;
  int *v;
} stpi_linecount_t;

typedef struct {		/* Base pointers for each pass */
  int ncolors;
  unsigned char **v;
} stpi_linebufs_t;

typedef struct {		/* Width of data actually printed */
  int ncolors;
  int *start_pos;
  int *end_pos;
} stpi_linebounds_t;

typedef enum {
  STPI_WEAVE_ZIGZAG,
  STPI_WEAVE_ASCENDING,
  STPI_WEAVE_DESCENDING,
  STPI_WEAVE_ASCENDING_2X,
  STPI_WEAVE_STAGGERED,
  STPI_WEAVE_ASCENDING_3X
} stpi_weave_strategy_t;

typedef int stpi_packfunc(stp_vars_t v,
			  const unsigned char *line, int height,
			  unsigned char *comp_buf,
			  unsigned char **comp_ptr,
			  int *first, int *last);
typedef void stpi_fillfunc(stp_vars_t v, int row, int subpass,
			   int width, int missingstartrows, int color);
typedef void stpi_flushfunc(stp_vars_t v, int passno, int vertical_subpass);
typedef int stpi_compute_linewidth_func(stp_vars_t v, int n);

extern void stpi_initialize_weave(stp_vars_t v, int jets, int separation,
				  int oversample, int horizontal,
				  int vertical, int ncolors, int bitwidth,
				  int linewidth, int line_count,
				  int first_line, int page_height,
				  const int *head_offset,
				  stpi_weave_strategy_t,
				  stpi_flushfunc,
				  stpi_fillfunc,
				  stpi_packfunc,
				  stpi_compute_linewidth_func);

extern stpi_packfunc stpi_pack_tiff;
extern stpi_packfunc stpi_pack_uncompressed;

extern stpi_fillfunc stpi_fill_tiff;
extern stpi_fillfunc stpi_fill_uncompressed;

extern stpi_compute_linewidth_func stpi_compute_tiff_linewidth;
extern stpi_compute_linewidth_func stpi_compute_uncompressed_linewidth;

extern void stpi_flush_all(stp_vars_t v);

extern void
stpi_write_weave(stp_vars_t v, unsigned char *const cols[]);

extern stpi_lineoff_t *
stpi_get_lineoffsets_by_pass(stp_const_vars_t v, int pass);

extern stpi_lineactive_t *
stpi_get_lineactive_by_pass(stp_const_vars_t v, int pass);

extern stpi_linecount_t *
stpi_get_linecount_by_pass(stp_const_vars_t v, int pass);

extern const stpi_linebufs_t *
stpi_get_linebases_by_pass(stp_const_vars_t v, int pass);

extern stpi_pass_t *
stpi_get_pass_by_pass(stp_const_vars_t v, int pass);

extern void
stpi_weave_parameters_by_row(stp_const_vars_t v, int row,
			     int vertical_subpass, stpi_weave_t *w);

#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_WEAVE_H */
/*
 * End of "$Id$".
 */
