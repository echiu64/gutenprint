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

typedef struct stpi_softweave
{
  stpi_linebufs_t *linebases;	/* Base address of each row buffer */
  stpi_lineoff_t *lineoffsets;	/* Offsets within each row buffer */
  stpi_lineactive_t *lineactive;	/* Does this line have anything printed? */
  stpi_linecount_t *linecounts;	/* How many rows we've printed this pass */
  stpi_linebounds_t *linebounds;	/* Starting and ending print column */
  stpi_pass_t *passes;		/* Circular list of pass numbers */
  int last_pass_offset;		/* Starting row (offset from the start of */
				/* the page) of the most recently printed */
				/* pass (so we can determine how far to */
				/* advance the paper) */
  int last_pass;		/* Number of the most recently printed pass */

  int jets;			/* Number of jets per color */
  int virtual_jets;		/* Number of jets per color, taking into */
				/* account the head offset */
  int separation;		/* Offset from one jet to the next in rows */
  void *weaveparm;		/* Weave calculation parameter block */

  int horizontal_weave;		/* Number of horizontal passes required */
				/* This is > 1 for some of the ultra-high */
				/* resolution modes */
  int vertical_subpasses;	/* Number of passes per line (for better */
				/* quality) */
  int vmod;			/* Number of banks of passes */
  int oversample;		/* Excess precision per row */
  int repeat_count;		/* How many times a pass is repeated */
  int ncolors;			/* How many colors */
  int linewidth;		/* Line width in input pixels */
  int vertical_height;		/* Image height in output pixels */
  int firstline;		/* Actual first line (referenced to paper) */

  int bitwidth;			/* Bits per pixel */
  int lineno;
  int vertical_oversample;	/* Vertical oversampling */
  int current_vertical_subpass;
  int horizontal_width;		/* Horizontal width, in bits */
  int *head_offset;		/* offset of printheads */
  unsigned char *s[MAX_WEAVE];
  unsigned char *fold_buf;
  unsigned char *comp_buf;
  stpi_weave_t wcache;
  int rcache;
  int vcache;
  stp_vars_t v;
  void (*flushfunc)(struct stpi_softweave *sw, int passno,
		    int vertical_subpass);
  void (*fill_start)(struct stpi_softweave *sw, int row, int subpass,
		     int width, int missingstartrows, int color);
  int (*pack)(const unsigned char *in, int bytes,
	      unsigned char *out, unsigned char **optr,
	      int *first, int *last);
  int (*compute_linewidth)(const struct stpi_softweave *sw, int n);
} stpi_softweave_t;


extern void	stpi_fold(const unsigned char *line, int single_height,
			  unsigned char *outbuf);

extern void	stpi_split_2(int height, int bits, const unsigned char *in,
			     unsigned char *outhi, unsigned char *outlo);

extern void	stpi_split_4(int height, int bits, const unsigned char *in,
			     unsigned char *out0, unsigned char *out1,
			     unsigned char *out2, unsigned char *out3);

extern void	stpi_unpack_2(int height, int bits, const unsigned char *in,
			      unsigned char *outlo, unsigned char *outhi);

extern void	stpi_unpack_4(int height, int bits, const unsigned char *in,
			      unsigned char *out0, unsigned char *out1,
			      unsigned char *out2, unsigned char *out3);

extern void	stpi_unpack_8(int height, int bits, const unsigned char *in,
			      unsigned char *out0, unsigned char *out1,
			      unsigned char *out2, unsigned char *out3,
			      unsigned char *out4, unsigned char *out5,
			      unsigned char *out6, unsigned char *out7);

extern int	stpi_pack_tiff(const unsigned char *line, int height,
			       unsigned char *comp_buf,
			       unsigned char **comp_ptr,
			       int *first, int *last);

extern int	stpi_pack_uncompressed(const unsigned char *line, int height,
				       unsigned char *comp_buf,
				       unsigned char **comp_ptr,
				       int *first, int *last);

extern void *stpi_initialize_weave(stp_vars_t v, int jets, int separation,
				   int oversample, int horizontal,
				   int vertical, int ncolors, int width,
				   int linewidth, int lineheight,
				   int first_line, int phys_lines,
				   int strategy, int *head_offset,
				   void (*flushfunc)(stpi_softweave_t *sw,
						     int passno,
						     int vertical_subpass),
				   void (*fill_start)(stpi_softweave_t *sw,
						      int row,
						      int subpass, int width,
						      int missingstartrows,
						      int vertical_subpass),
				   int (*pack)(const unsigned char *in,
					       int bytes, unsigned char *out,
					       unsigned char **optr,
					       int *first, int *last),
				   int (*compute_linewidth)(const stpi_softweave_t *sw,
							    int n));

extern void stpi_fill_tiff(stpi_softweave_t *sw, int row, int subpass,
			   int width, int missingstartrows, int color);
extern void stpi_fill_uncompressed(stpi_softweave_t *sw, int row, int subpass,
				   int width, int missingstartrows, int color);

extern int stpi_compute_tiff_linewidth(const stpi_softweave_t *sw, int n);
extern int stpi_compute_uncompressed_linewidth(const stpi_softweave_t *sw, int n);

extern void stpi_flush_all(void *);

extern void
stpi_write_weave(void *        vsw,
		 int           length,	/* I - Length of bitmap data */
		 unsigned char *const cols[]);

extern stpi_lineoff_t *
stpi_get_lineoffsets_by_pass(const stpi_softweave_t *sw, int pass);

extern stpi_lineactive_t *
stpi_get_lineactive_by_pass(const stpi_softweave_t *sw, int pass);

extern stpi_linecount_t *
stpi_get_linecount_by_pass(const stpi_softweave_t *sw, int pass);

extern const stpi_linebufs_t *
stpi_get_linebases_by_pass(const stpi_softweave_t *sw, int pass);

extern stpi_pass_t *
stpi_get_pass_by_pass(const stpi_softweave_t *sw, int pass);

extern void
stpi_weave_parameters_by_row(const stpi_softweave_t *sw, int row,
			     int vertical_subpass, stpi_weave_t *w);

extern void stpi_destroy_weave(void *);

#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_WEAVE_H */
/*
 * End of "$Id$".
 */
