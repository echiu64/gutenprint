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


/*
 * ECOLOR_K must be 0
 */
#define ECOLOR_K  0
#define ECOLOR_C  1
#define ECOLOR_M  2
#define ECOLOR_Y  3
#define ECOLOR_LC 4
#define ECOLOR_LM 5
#define ECOLOR_LY 6
#define NCOLORS (4)
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
} stp_weave_t;

typedef struct			/* Weave parameters for a specific pass */
{
  int pass;			/* Absolute pass number */
  int missingstartrows;		/* All other values the same as weave_t */
  int logicalpassstart;
  int physpassstart;
  int physpassend;
  int subpass;
} stp_pass_t;

typedef struct {		/* Offsets from the start of each line */
  int ncolors;
  unsigned long *v;		/* (really pass) */
} stp_lineoff_t;

typedef struct {		/* Is this line (really pass) active? */
  int ncolors;
  char *v;
} stp_lineactive_t;

typedef struct {		/* number of rows for a pass */
  int ncolors;
  int *v;
} stp_linecount_t;

typedef struct {		/* Base pointers for each pass */
  int ncolors;
  unsigned char **v;
} stp_linebufs_t;

typedef struct {		/* Width of data actually printed */
  int ncolors;
  int *start_pos;
  int *end_pos;
} stp_linebounds_t;

typedef struct stp_softweave
{
  stp_linebufs_t *linebases;	/* Base address of each row buffer */
  stp_lineoff_t *lineoffsets;	/* Offsets within each row buffer */
  stp_lineactive_t *lineactive;	/* Does this line have anything printed? */
  stp_linecount_t *linecounts;	/* How many rows we've printed this pass */
  stp_linebounds_t *linebounds;	/* Starting and ending print column */
  stp_pass_t *passes;		/* Circular list of pass numbers */
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
  stp_weave_t wcache;
  int rcache;
  int vcache;
  stp_vars_t v;
  void (*flushfunc)(struct stp_softweave *sw, int passno, int model,
		    int width, int hoffset, int ydpi, int xdpi,
		    int physical_xdpi, int vertical_subpass);
  void (*fill_start)(struct stp_softweave *sw, int row, int subpass,
		     int width, int missingstartrows, int color);
  int (*pack)(const unsigned char *in, int bytes,
	      unsigned char *out, unsigned char **optr,
	      int *first, int *last);
  int (*compute_linewidth)(const struct stp_softweave *sw, int n);
} stp_softweave_t;


extern void	stp_fold(const unsigned char *line, int single_height,
			 unsigned char *outbuf);

extern void	stp_split_2(int height, int bits, const unsigned char *in,
			    unsigned char *outhi, unsigned char *outlo);

extern void	stp_split_4(int height, int bits, const unsigned char *in,
			    unsigned char *out0, unsigned char *out1,
			    unsigned char *out2, unsigned char *out3);

extern void	stp_unpack_2(int height, int bits, const unsigned char *in,
			     unsigned char *outlo, unsigned char *outhi);

extern void	stp_unpack_4(int height, int bits, const unsigned char *in,
			     unsigned char *out0, unsigned char *out1,
			     unsigned char *out2, unsigned char *out3);

extern void	stp_unpack_8(int height, int bits, const unsigned char *in,
			     unsigned char *out0, unsigned char *out1,
			     unsigned char *out2, unsigned char *out3,
			     unsigned char *out4, unsigned char *out5,
			     unsigned char *out6, unsigned char *out7);

extern int	stp_pack_tiff(const unsigned char *line, int height,
			      unsigned char *comp_buf,
			      unsigned char **comp_ptr,
			      int *first, int *last);

extern int	stp_pack_uncompressed(const unsigned char *line, int height,
				      unsigned char *comp_buf,
				      unsigned char **comp_ptr,
				      int *first, int *last);

extern void *stp_initialize_weave(int jets, int separation, int oversample,
				  int horizontal, int vertical,
				  int ncolors, int width, int linewidth,
				  int lineheight,
				  int first_line, int phys_lines, int strategy,
                                  int *head_offset,  /* Get from model - used for 480/580 printers */
				  stp_vars_t v,
				  void (*flushfunc)(stp_softweave_t *sw,
						    int passno, int model,
						    int width, int hoffset,
						    int ydpi, int xdpi,
						    int physical_xdpi,
						    int vertical_subpass),
				  void (*fill_start)(stp_softweave_t *sw,
						     int row,
						     int subpass, int width,
						     int missingstartrows,
						     int vertical_subpass),
				  int (*pack)(const unsigned char *in,
					      int bytes, unsigned char *out,
					      unsigned char **optr,
					      int *first, int *last),
				  int (*compute_linewidth)(const stp_softweave_t *sw,
							   int n));

extern void stp_fill_tiff(stp_softweave_t *sw, int row, int subpass,
			  int width, int missingstartrows, int color);
extern void stp_fill_uncompressed(stp_softweave_t *sw, int row, int subpass,
				  int width, int missingstartrows, int color);

extern int stp_compute_tiff_linewidth(const stp_softweave_t *sw, int n);
extern int stp_compute_uncompressed_linewidth(const stp_softweave_t *sw, int n);

extern void stp_flush_all(void *, int model, int width, int hoffset,
			  int ydpi, int xdpi, int physical_xdpi);

extern void
stp_write_weave(void *        vsw,
		int           length,	/* I - Length of bitmap data */
		int           ydpi,	/* I - Vertical resolution */
		int           model,	/* I - Printer model */
		int           width,	/* I - Printed width */
		int           offset,	/* I - Offset from left side of page */
		int	      xdpi,
		int	      physical_xdpi,
		unsigned char *const cols[]);

extern stp_lineoff_t *
stp_get_lineoffsets_by_pass(const stp_softweave_t *sw, int pass);

extern stp_lineactive_t *
stp_get_lineactive_by_pass(const stp_softweave_t *sw, int pass);

extern stp_linecount_t *
stp_get_linecount_by_pass(const stp_softweave_t *sw, int pass);

extern const stp_linebufs_t *
stp_get_linebases_by_pass(const stp_softweave_t *sw, int pass);

extern stp_pass_t *
stp_get_pass_by_pass(const stp_softweave_t *sw, int pass);

extern void
stp_weave_parameters_by_row(const stp_softweave_t *sw, int row,
			    int vertical_subpass, stp_weave_t *w);

extern void stp_destroy_weave(void *);

extern void stp_destroy_weave_params(void *vw);


#ifdef __cplusplus
  }
#endif

#endif /* GIMP_PRINT_INTERNAL_WEAVE_H */
/*
 * End of "$Id$".
 */
