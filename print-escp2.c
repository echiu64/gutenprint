/*
 * "$Id$"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
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
 * Contents:
 *
 *   escp2_parameters()     - Return the parameter values for the given
 *                            parameter.
 *   escp2_imageable_area() - Return the imageable area of the page.
 *   escp2_print()          - Print an image to an EPSON printer.
 *   escp2_write()          - Send 6-color ESC/P2 graphics using TIFF packbits compression.
 *
 * Revision History:
 *
 * See bottom of file
 */

/*
 * Stylus Photo EX added by Robert Krawitz <rlk@alum.mit.edu> August 30, 1999
 */

#ifndef WEAVETEST
#include "print.h"
#endif

#ifdef DEBUG_SIGNAL
#include <signal.h>
#endif

typedef enum {
  COLOR_MONOCHROME,
  COLOR_CMYK,
  COLOR_CCMMYK
} colormode_t;

/*
 * Mapping between color and linear index.  The colors are
 * black, magenta, cyan, yellow, light magenta, light cyan
 */

static const int color_indices[16] = { 0, 1, 2, -1,
				       3, -1, -1, -1,
				       -1, 4, 5, -1,
				       -1, -1, -1, -1 };
static const int colors[6] = { 0, 1, 2, 4, 1, 2 };
static const int densities[6] = { 0, 0, 0, 0, 1, 1 };

#ifndef WEAVETEST

/*
 * Local functions...
 */

static void 
escp2_write_microweave(FILE *, const unsigned char *,
		       const unsigned char *, const unsigned char *,
		       const unsigned char *, const unsigned char *,
		       const unsigned char *, int, int, int, int, int,
		       int, int);
static void *initialize_weave(int jets, int separation, int oversample,
			      int horizontal, int vertical,
			      colormode_t colormode, int width, int linewidth,
			      int lineheight, int vertical_row_separation,
			      int first_line, int phys_lines);
static void escp2_flush(void *, int model, int width, int hoffset, int ydpi,
			int xdpi, FILE *prn);
static void
escp2_write_weave(void *, FILE *, int, int, int, int, int, int,
		  const unsigned char *c, const unsigned char *m,
		  const unsigned char *y, const unsigned char *k,
		  const unsigned char *C, const unsigned char *M);
static void escp2_init_microweave(void);
static void escp2_free_microweave(void);

static void destroy_weave(void *);

#define PHYSICAL_BPI 720
#define MAX_OVERSAMPLED 4
#define MAX_BPP 2
#define BITS_PER_BYTE 8
#define COMPBUFWIDTH (PHYSICAL_BPI * MAX_OVERSAMPLED * MAX_BPP * \
	MAX_CARRIAGE_WIDTH / BITS_PER_BYTE)

/*
 * Printer capabilities.
 *
 * Various classes of printer capabilities are represented by bitmasks.
 */

typedef unsigned long long model_cap_t;
typedef unsigned long long model_featureset_t;

typedef struct escp2_printer
{
  model_cap_t	flags;		/* Bitmask of flags, see below */
  int 		nozzles;	/* Number of nozzles per color */
  int		nozzle_separation; /* Separation between rows, in 1/720" */
  int		black_nozzles;	/* Number of black nozzles (may be extra) */
  int		xres;		/* Normal distance between dots in */
				/* softweave mode (inverse inches) */
  int		softweave_dot_size;/* Dot size to use in softweave mode */
  int		microweave_dot_size; /* Dot size to use in microweave mode */
  int		max_paper_width; /* Maximum paper width, in points*/
  int		max_paper_height; /* Maximum paper height, in points */
  int		left_margin;	/* Left margin, points */
  int		right_margin;	/* Right margin, points */
  int		top_margin;	/* Absolute top margin, points */
  int		bottom_margin;	/* Absolute bottom margin, points */
  int		separation_rows; /* Some printers require funky spacing */
				/* arguments in microweave mode. */
  int		pseudo_separation_rows;/* Some printers require funky */
				/* spacing arguments in softweave mode */
} escp2_printer_t;

#define MODEL_INIT_MASK		0xf
#define MODEL_INIT_STANDARD	0x0
#define MODEL_INIT_900		0x1

#define MODEL_HASBLACK_MASK	0x10
#define MODEL_HASBLACK_YES	0x00
#define MODEL_HASBLACK_NO	0x10

#define MODEL_6COLOR_MASK	0x20
#define MODEL_6COLOR_NO		0x00
#define MODEL_6COLOR_YES	0x20

#define MODEL_1440DPI_MASK	0x40
#define MODEL_1440DPI_NO	0x00
#define MODEL_1440DPI_YES	0x40

#define MODEL_GRAYMODE_MASK	0x80
#define MODEL_GRAYMODE_NO	0x00
#define MODEL_GRAYMODE_YES	0x80

#define MODEL_720DPI_MODE_MASK	0x300
#define MODEL_720DPI_DEFAULT	0x000
#define MODEL_720DPI_600	0x100
#define MODEL_720DPI_PHOTO	0x100 /* 0x200 for experimental stuff */

#define MODEL_VARIABLE_DOT_MASK	0xc00
#define MODEL_VARIABLE_NORMAL	0x000
#define MODEL_VARIABLE_4	0x400

#define MODEL_COMMAND_MASK	0xf000
#define MODEL_COMMAND_GENERIC	0x0000
#define MODEL_COMMAND_1999	0x1000 /* The 1999 series printers */

#define INCH_8_5	(17 * 72 / 2)
#define INCH_11		(11 * 72)
#define INCH_13		(13 * 72)
#define INCH_14		(14 * 72)
#define INCH_16_5	(33 * 72 / 2)
#define INCH_17		(17 * 72)
#define INCH_19		(19 * 72)
#define INCH_24		(24 * 72)

/*
 * SUGGESTED SETTINGS FOR STYLUS PHOTO EX:
 * Brightness 127
 * Blue 92
 * Saturation 1.2
 *
 * Another group of settings that has worked well for me is
 * Brightness 110
 * Gamma 1.2
 * Contrast 97
 * Blue 88
 * Saturation 1.1
 * Density 1.5
 *
 * With the current code, the following settings seem to work nicely:
 * Brightness ~110
 * Gamma 1.3
 * Contrast 80
 * Green 94
 * Blue 89
 * Saturation 1.15
 * Density 1.6
 *
 * The green and blue will vary somewhat with different inks
 */

static double dot_sizes[] = { 0.333, 0.5, 1.0 };

static simple_dither_range_t variable_dither_ranges[] =
{
  { 0.074, 0x1, 0, 1 },
  { 0.111, 0x2, 0, 2 },
  { 0.222, 0x3, 0, 3 },
  { 0.333, 0x1, 1, 1 },
  { 0.5,   0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static simple_dither_range_t standard_dither_ranges[] =
{
  { 0.333, 0x1, 1, 1 },
  { 0.5,   0x2, 1, 2 },
  { 1.0,   0x3, 1, 3 }
};

static simple_dither_range_t mis_sixtone_ranges[] =
{
  { 0.15, 0x000001, 1, 1 },	/* LC */
  { 0.25, 0x000010, 1, 1 },	/* C */
  { 0.45, 0x000100, 1, 1 },	/* LM */
  { 0.50, 0x001000, 1, 1 },	/* Y */
  { 0.75, 0x010000, 1, 1 },	/* M */
  { 1.00, 0x100000, 1, 1 }	/* K */
};

static full_dither_range_t normal_dither_ranges[] =
{
  { 0.0,   0.5,  0x0, 0x1, 1, 1},
  { 0.5,   0.67, 0x1, 0x2, 1, 1},
  { 0.67,  1.0,  0x2, 0x3, 1, 1}
};

static full_dither_range_t stp870_c1_dither_ranges[] =
{
  { 0.0,  0.18, 0x0, 0x1, 0, 0},
  { 0.18, 0.25, 0x1, 0x2, 0, 0},
  { 0.25, 0.52, 0x2, 0x1, 0, 1},
  { 0.25, 0.39, 0x2, 0x3, 0, 0},
  { 0.39, 0.52, 0x3, 0x1, 0, 1},
  { 0.52, 0.67, 0x1, 0x2, 1, 1},
  { 0.67, 0.96, 0x2, 0x3, 1, 1}
};

static full_dither_range_t stp870_c2_dither_ranges[] =
{
  { 0.0,  0.18, 0x0, 0x1, 0, 0},
  { 0.18, 0.25, 0x1, 0x2, 0, 0},
  { 0.25, 0.39, 0x2, 0x3, 0, 0},
  { 0.39, 0.67, 0x3, 0x2, 0, 1},
  { 0.67, 0.96, 0x2, 0x3, 1, 1}
};

static full_dither_range_t stp870_m1_dither_ranges[] =
{
  { 0.0,  0.18, 0x0, 0x1, 0, 0},
  { 0.18, 0.24, 0x1, 0x2, 0, 0},
  { 0.24, 0.52, 0x2, 0x1, 0, 1},
  { 0.24, 0.36, 0x2, 0x3, 0, 0},
  { 0.36, 0.52, 0x3, 0x1, 0, 1},
  { 0.52, 0.67, 0x1, 0x2, 1, 1},
  { 0.67, 1.0,  0x2, 0x3, 1, 1}
};

static full_dither_range_t stp870_m2_dither_ranges[] =
{
  { 0.0,  0.18, 0x0, 0x1, 0, 0},
  { 0.18, 0.24, 0x1, 0x2, 0, 0},
  { 0.24, 0.36, 0x2, 0x3, 0, 0},
  { 0.36, 0.67, 0x3, 0x2, 0, 1},
  { 0.67, 1.0,  0x2, 0x3, 1, 1}
};

static full_dither_range_t stp870_y_dither_ranges[] =
{
  { 0.0,  0.55,  0x0, 0x1, 1, 1},
  { 0.55,  0.71,  0x1, 0x2, 1, 1},
  { 0.71,  1.0,  0x2, 0x3, 1, 1}
};

static full_dither_range_t stp870_k_dither_ranges[] =
{
  { 0.0,  0.55,  0x0, 0x1, 1, 1},
  { 0.55,  0.67, 0x1, 0x2, 1, 1},
  { 0.67, 1.0,   0x2, 0x3, 1, 1}
};

/*
 * A lot of these are guesses
 */

static escp2_printer_t model_capabilities[] =
{
  /* FIRST GENERATION PRINTERS */
  /* 0: Stylus Color */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_NO),
    1, 1, 1, 720, -1, -1, INCH_8_5, INCH_14, 14, 14, 9, 49, 1, 0
  },
  /* 1: Stylus Color Pro/Pro XL/400/500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO | MODEL_1440DPI_NO),
    48, 6, 48, 720, -1, 1, INCH_8_5, INCH_14, 14, 14, 0, 24, 1, 0
  },
  /* 2: Stylus Color 1500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_NO
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO | MODEL_1440DPI_NO),
    1, 1, 1, 720, -1, 1, INCH_11, INCH_17, 14, 14, 9, 49, 1, 0
  },
  /* 3: Stylus Color 600 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    32, 8, 32, 720, 0, 2, INCH_8_5, INCH_14, 8, 9, 0, 24, 1, 0
  },
  /* 4: Stylus Color 800 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    64, 4, 64, 720, 0, 2, INCH_8_5, INCH_14, 8, 9, 0, 24, 1, 4
  },
  /* 5: Stylus Color 850 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    64, 4, 128, 720, 0, 2, INCH_8_5, INCH_14, 8, 9, 0, 24, 1, 4
  },
  /* 6: Stylus Color 1520/3000 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    1, 1, 128, 720, -1, 2, INCH_16_5, INCH_24, 8, 9, 9, 49, 4, 0
  },

  /* SECOND GENERATION PRINTERS */
  /* 7: Stylus Photo 700 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO | MODEL_1440DPI_YES),
    32, 8, 32, 720, 0, 3, INCH_8_5, INCH_14, 9, 9, 0, 24, 1, 0
  },
  /* 8: Stylus Photo EX */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO | MODEL_1440DPI_YES),
    32, 8, 32, 720, 0, 3, INCH_11, INCH_17, 9, 9, 0, 24, 1, 0
  },
  /* 9: Stylus Photo */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO | MODEL_1440DPI_NO),
    32, 8, 32, 720, 0, 3, INCH_8_5, INCH_14, 9, 9, 0, 24, 1, 0
  },

  /* THIRD GENERATION PRINTERS */
  /* 10: Stylus Color 440/460 */
  /* Thorsten Schnier has confirmed that the separation is 8.  Why on */
  /* earth anyone would use 21 nozzles when designing a print head is */
  /* completely beyond me, but there you are... */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_NO),
    21, 8, 64, 720, 2, 3, INCH_8_5, INCH_14, 9, 9, 0, 9, 1, 0
  },
  /* 11: Stylus Color 640 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    32, 8, 64, 720, 0, 3, INCH_8_5, INCH_14, 9, 9, 0, 9, 1, 0
  },
  /* 12: Stylus Color 740 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    48, 6, 144, 360, 0, 3, INCH_11, INCH_17, 9, 9, 0, 9, 1, 0
  },
  /* 13: Stylus Color 900 */
  /* Dale Pontius thinks the spacing is 3 jets??? */
  /* No, Eric Sharkey verified that it's 2! */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    96, 2, 192, 360, 0, 1, INCH_8_5, INCH_14, 9, 9, 0, 9, 1, 0
  },
  /* 14: Stylus Photo 750, 870 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES
     | MODEL_6COLOR_YES | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO | MODEL_1440DPI_YES),
    48, 6, 48, 360, 0, 4, INCH_8_5, INCH_14, 9, 9, 0, 9, 1, 0
  },
  /* 15: Stylus Photo 1200, 1270 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES
     | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO | MODEL_1440DPI_YES),
    48, 6, 48, 360, 0, 4, INCH_13, INCH_19, 9, 9, 0, 9, 1, 0
  },
  /* 16: Stylus Color 860 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    48, 6, 144, 360, 0, 2, INCH_8_5, INCH_14, 9, 9, 0, 9, 1, 0
  },
  /* 17: Stylus Color 1160 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    48, 6, 144, 360, 0, 2, INCH_13, INCH_19, 9, 9, 0, 9, 1, 0
  },
  /* 18: Stylus Color 660 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    32, 8, 64, 720, 0, 3, INCH_8_5, INCH_14, 9, 9, 0, 9, 1, 8
  },
  /* 19: Stylus Color 760 */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    48, 6, 144, 360, 0, 3, INCH_8_5, INCH_14, 9, 9, 0, 9, 1, 0
  },

};

typedef struct {
  const char name[65];
  int hres;
  int vres;
  int softweave;
  int horizontal_passes;
  int vertical_passes;
  int vertical_subsample;
} res_t;

static const res_t escp2_reslist[] = {
  { "360 DPI", 360, 360, 0, 1, 1, 1 },
  { "720 DPI Microweave", 720, 720, 0, 1, 1, 1 },
  { "720 DPI Softweave", 720, 720, 1, 1, 1, 1 },
  { "720 DPI High Quality", 720, 720, 1, 1, 2, 1 },
  { "720 DPI Highest Quality", 720, 720, 1, 1, 4, 1 },
  { "1440 x 720 DPI Microweave", 1440, 720, 0, 2, 1, 1 },
  { "1440 x 720 DPI Softweave", 1440, 720, 1, 2, 1, 1 },
  { "1440 x 720 DPI Highest Quality", 1440, 720, 1, 2, 2, 1 },
  { "1440 x 720 DPI Enhanced", 1440, 1440, 1, 2, 1, 2 },
  { "2880 x 720 DPI Two-pass", 2880, 720, 1, 4, 1, 1 },
  /* { "1440 x 720 DPI Two-pass Microweave", 2880, 720, 0, 1, 1 }, */
  { "", 0, 0, 0, 0, 0 }
};

typedef struct {
  const char name[65];
  int is_color;
  int variable_dot_size;
  int dot_size_bits;
  simple_dither_range_t *standard_dither;
  simple_dither_range_t *photo_dither;
} ink_t;

static const ink_t escp2_inklist[] = {
  { "Variable Dot Size", 1, 1, 2, standard_dither_ranges, variable_dither_ranges },
  { "Single Dot Size", 1, 1, 1, NULL, NULL },
  { "MIS Six Tone Monochrome", 0, 0, 1, NULL, mis_sixtone_ranges }
};

#ifdef ESCP2_GHOST
const char *
escp2_resname(int resolution)
{
  if (resolution < 0 ||
      resolution >= ((sizeof(escp2_reslist) / sizeof(res_t)) - 1))
    return NULL;
  else
    return escp2_reslist[resolution].name;
}
#endif

static int
escp2_has_cap(int model, model_featureset_t featureset,
	      model_featureset_t class)
{
  return ((model_capabilities[model].flags & featureset) == class);
}

static model_featureset_t
escp2_cap(int model, model_featureset_t featureset)
{
  return (model_capabilities[model].flags & featureset);
}

static unsigned
escp2_nozzles(int model)
{
  return (model_capabilities[model].nozzles);
}

static unsigned
escp2_black_nozzles(int model)
{
  return (model_capabilities[model].black_nozzles);
}

static unsigned
escp2_nozzle_separation(int model)
{
  return (model_capabilities[model].nozzle_separation);
}

static unsigned
escp2_separation_rows(int model)
{
  return (model_capabilities[model].separation_rows);
}

static unsigned
escp2_xres(int model)
{
  return (model_capabilities[model].xres);
}

static int
escp2_soft_ink(int model)
{
  return (model_capabilities[model].softweave_dot_size);
}

static int
escp2_micro_ink(int model)
{
  return (model_capabilities[model].microweave_dot_size);
}

static unsigned
escp2_max_paper_width(int model)
{
  return (model_capabilities[model].max_paper_width);
}

static unsigned
escp2_max_paper_height(int model)
{
  return (model_capabilities[model].max_paper_height);
}

static unsigned
escp2_left_margin(int model)
{
  return (model_capabilities[model].left_margin);
}

static unsigned
escp2_right_margin(int model)
{
  return (model_capabilities[model].right_margin);
}

static unsigned
escp2_top_margin(int model)
{
  return (model_capabilities[model].top_margin);
}

static unsigned
escp2_bottom_margin(int model)
{
  return (model_capabilities[model].bottom_margin);
}

static int
escp2_pseudo_separation_rows(int model)
{
  return (model_capabilities[model].pseudo_separation_rows);
}

/*
 * 'escp2_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
escp2_parameters(int  model,		/* I - Printer model */
                 char *ppd_file,	/* I - PPD file (not used) */
                 char *name,		/* I - Name of parameter */
                 int  *count)		/* O - Number of values */
{
  int		i;
  char		**valptrs;

  static char *ink_types[] =
  {
    "Variable Dot Size",
    "Single Dot Size"
  };

  static char *media_types[] =
  {
    "Standard Paper",
    "Glossy Film"
  };

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0)
    {
      unsigned int length_limit, width_limit;
      const papersize_t *papersizes = get_papersizes();
      valptrs = malloc(sizeof(char *) * known_papersizes());
      *count = 0;
      width_limit = escp2_max_paper_width(model);
      length_limit = escp2_max_paper_height(model);
      for (i = 0; i < known_papersizes(); i++)
	{
	  if (strlen(papersizes[i].name) > 0 &&
	      papersizes[i].width <= width_limit &&
	      papersizes[i].length <= length_limit)
	    {
	      valptrs[*count] = malloc(strlen(papersizes[i].name) + 1);
	      strcpy(valptrs[*count], papersizes[i].name);
	      (*count)++;
	    }
	}
      return (valptrs);
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      const res_t *res = &(escp2_reslist[0]);
      valptrs = malloc(sizeof(char *) * sizeof(escp2_reslist) / sizeof(res_t));
      *count = 0;
      while(res->hres)
	{
	  if (escp2_has_cap(model, MODEL_1440DPI_MASK, MODEL_1440DPI_YES) ||
	      (res->hres <= 720 && res->vres <= 720))
	    {
	      int nozzles = escp2_nozzles(model);
	      int separation = escp2_nozzle_separation(model);
	      int max_weave = nozzles / separation;
	      if ((((720 / escp2_xres(model)) * res->horizontal_passes *
		    res->vertical_passes) <= 8) &&
		  ((720 / escp2_xres(model)) * res->horizontal_passes <= 4) &&
		  (! res->softweave ||
		   (nozzles > 1 && res->vertical_passes <= max_weave)))
		{
		  valptrs[*count] = malloc(strlen(res->name) + 1);
		  strcpy(valptrs[*count], res->name);
		  (*count)++;
		}
	    }
	  res++;
	}
      return (valptrs);
    }
  else if (strcmp(name, "InkType") == 0)
    {
      if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_NORMAL))
	return NULL;
      else
	{
	  int ninktypes = sizeof(ink_types) / sizeof(char *);
	  valptrs = malloc(sizeof(char *) * ninktypes);
	  for (i = 0; i < ninktypes; i++)
	    {
	      valptrs[i] = malloc(strlen(ink_types[i]) + 1);
	      strcpy(valptrs[i], ink_types[i]);
	    }
	  *count = ninktypes;
	  return valptrs;
	}
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      int nmediatypes = sizeof(media_types) / sizeof(char *);
      valptrs = malloc(sizeof(char *) * nmediatypes);
      for (i = 0; i < nmediatypes; i++)
	{
	  valptrs[i] = malloc(strlen(media_types[i]) + 1);
	  strcpy(valptrs[i], media_types[i]);
	}
      *count = nmediatypes;
      return valptrs;
    }
  else
    return (NULL);

}

/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

void
escp2_imageable_area(int  model,	/* I - Printer model */
                     char *ppd_file,	/* I - PPD file (not used) */
                     char *media_size,	/* I - Media size */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */

  default_media_size(model, ppd_file, media_size, &width, &length);
  *left =	escp2_left_margin(model);
  *right =	width - escp2_right_margin(model);
  *top =	length - escp2_top_margin(model);
  *bottom =	escp2_bottom_margin(model);
}

static void
escp2_init_printer(FILE *prn,int model, int output_type, int ydpi, int xdpi,
		   int use_softweave, int page_length, int page_width,
		   int page_top, int page_bottom, int top, int nozzles,
		   int nozzle_separation, int horizontal_passes,
		   int vertical_passes, int vertical_subsample, int bits)
{
  int l, t;
  if (ydpi > 720)
    ydpi = 720;
  l = ydpi * page_length / 72;
  t = ydpi * page_top / 72;

  /*
   * Hack that seems to be necessary for these silly things to print.
   * Apparently this lets USB port work if needed.
   */
  if (escp2_has_cap(model, MODEL_INIT_MASK, MODEL_INIT_900))
    fprintf(prn, "%c%c%c\033\001@EJL 1284.4\n@EJL     \n\033@", 0, 0, 0);

  fputs("\033@", prn); 				/* ESC/P2 reset */

  /* Magic remote mode commands, whatever they do */
  if (escp2_has_cap(model, MODEL_COMMAND_MASK, MODEL_COMMAND_1999))
    fprintf(prn,
	    "\033(R\010%c%cREMOTE1PM\002%c%c%cSN\003%c%c%c\001\033%c%c%c",
	    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

  fwrite("\033(G\001\000\001", 6, 1, prn);	/* Enter graphics mode */

  /* Set up print resolution */
  if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4) &&
      use_softweave)
    fprintf(prn, "\033(U\005%c%c%c%c\240\005", 0, 1440 / ydpi, 1440 / ydpi,
	    1440 / (ydpi * (horizontal_passes > 2 ? 2 : 1)));
  else
    fprintf(prn, "\033(U\001%c%c", 0, 3600 / ydpi);	    

  /* Gray/color */
  if (escp2_has_cap(model, MODEL_GRAYMODE_MASK, MODEL_GRAYMODE_YES))
    fprintf(prn, "\033(K\002%c%c%c", 0, 0,
	    (output_type == OUTPUT_GRAY ? 1 : 2));

  /* Microweave? */
  fprintf(prn, "\033(i\001%c%c", 0,
	  (use_softweave || ydpi < 720) ? 0 : 1);

  /* Direction/speed */
  if (horizontal_passes * vertical_passes * vertical_subsample > 2)
    {
      fprintf(prn, "\033U%c", 1);
      if (xdpi > 720)		/* Slow mode if available */
	fprintf(prn, "\033(s%c%c%c", 1, 0, 2);
    }
  else
    fprintf(prn, "\033U%c", 0);

  /* Dot size */
  if (!use_softweave)
    {
      if (escp2_micro_ink(model) > 0)
	fprintf(prn, "\033(e\002%c%c%c", 0, 0, escp2_micro_ink(model));
    }
  else if (bits > 1)
    fwrite("\033(e\002\000\000\020", 7, 1, prn); /* Variable dots */
  else if (escp2_soft_ink(model) >= 0)
    fprintf(prn, "\033(e\002%c%c%c", 0, 0, escp2_soft_ink(model));

  /* Set up page */
  if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4) &&
      use_softweave)
    {
      /* Page length */
      fprintf(prn, "\033(C\004%c%c%c%c%c", 0,
	      l & 0xff, (l >> 8) & 0xff, (l >> 16) & 0xff, (l >> 24) & 0xff);

      /* Top/bottom margins */
      if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
	fprintf(prn, "\033(c\010%c%c%c%c%c%c%c%c%c", 0,
		t & 0xff, t >> 8, (t >> 16) & 0xff, (t >> 24) & 0xff,
		l & 0xff, l >> 8, (l >> 16) & 0xff, (l >> 24) & 0xff);
      else
	fprintf(prn, "\033(c\004%c%c%c%c%c", 0,
		t & 0xff, t >> 8, l & 0xff, l >> 8);

      /* Page form factor */
      fprintf(prn, "\033(S\010%c%c%c%c%c%c%c%c%c", 0,
	      (((page_width * 720 / 72) >> 0) & 0xff),
	      (((page_width * 720 / 72) >> 8) & 0xff),
	      (((page_width * 720 / 72) >> 16) & 0xff),
	      (((page_width * 720 / 72) >> 24) & 0xff),
	      (((page_length * 720 / 72) >> 0) & 0xff),
	      (((page_length * 720 / 72) >> 8) & 0xff),
	      (((page_length * 720 / 72) >> 16) & 0xff),
	      (((page_length * 720 / 72) >> 24) & 0xff));

      /* Magic resolution cookie */
      fprintf(prn, "\033(D%c%c%c%c%c%c", 4, 0, 14400 % 256, 14400 / 256,
	      escp2_nozzle_separation(model) * 14400 / 720,
	      14400 / escp2_xres(model));

      if (!use_softweave || ydpi < 720)
	fprintf(prn, "\033(v\004%c%c%c%c%c", 0,
		top & 0xff, top >> 8, (top >> 16) & 0xff, (top >> 24) & 0xff);
    }
  else
    {
      /* Page length */
      fprintf(prn, "\033(C\002%c%c%c", 0, l & 255, l >> 8);

      /* Top/bottom margins */
      fprintf(prn, "\033(c\004%c%c%c%c%c", 0,
	      t & 0xff, t >> 8, l & 0xff, l >> 8);

      if (escp2_has_cap(model, MODEL_COMMAND_MASK, MODEL_COMMAND_1999))
	fprintf(prn, "\033(S\010%c%c%c%c%c%c%c%c%c", 0,
		(((page_width * 720 / 72) >> 0) & 0xff),
		(((page_width * 720 / 72) >> 8) & 0xff),
		(((page_width * 720 / 72) >> 16) & 0xff),
		(((page_width * 720 / 72) >> 24) & 0xff),
		(((page_length * 720 / 72) >> 0) & 0xff),
		(((page_length * 720 / 72) >> 8) & 0xff),
		(((page_length * 720 / 72) >> 16) & 0xff),
		(((page_length * 720 / 72) >> 24) & 0xff));

      if (!use_softweave || ydpi < 720)
	fprintf(prn, "\033(v\002%c%c%c", 0, top & 0xff, top >> 8);
    }
}

/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
void
escp2_print(const printer_t *printer,		/* I - Model */
            int       copies,		/* I - Number of copies */
            FILE      *prn,		/* I - File to print to */
	    Image     image,		/* I - Image to print */
            unsigned char    *cmap,	/* I - Colormap (for indexed images) */
	    const vars_t    *v)
{
  int		model = printer->model;
  char 		*ppd_file = v->ppd_file;
  char 		*resolution = v->resolution;
  char 		*media_size = v->media_size;
  char		*media_type = v->media_type;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  char          *ink_type = v->ink_type;
  float 	scaling = v->scaling;
  int		top = v->top;
  int		left = v->left;
  int		x, y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  int		n;		/* Output number */
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned char	*in,		/* Input pixels */
		*black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*lcyan,		/* Light cyan bitmap data */
		*lmagenta,	/* Light magenta bitmap data */
		*yellow;	/* Yellow bitmap data */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		page_length,	/* True length of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_bpp,	/* Output bytes per pixel */
		temp_width,	/* Temporary width of image on page */
		temp_height,	/* Temporary height of image on page */
		landscape,	/* True if we rotate the output 90 degrees */
		length,		/* Length of raster data */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  convert_t	colorfunc = 0;	/* Color conversion function... */
  int           image_height,
                image_width,
                image_bpp;
  int		use_softweave = 0;
  int		nozzles = 1;
  int		nozzle_separation = 1;
  int		horizontal_passes = 1;
  int		real_horizontal_passes = 1;
  int		vertical_passes = 1;
  int		vertical_subsample = 1;
  const res_t 	*res;
  int		bits;
  void *	weave = NULL;
  void *	dither;
  colormode_t colormode = COLOR_CCMMYK;
  int		separation_rows = escp2_separation_rows(model);
  int		use_glossy_film = 0;
  int		ink_spread;
  vars_t	nv;

  memcpy(&nv, v, sizeof(vars_t));

  if (!strcmp(media_type, "Glossy Film"))
    use_glossy_film = 1;

  if (nv.image_type == IMAGE_MONOCHROME)
    {
      colormode = COLOR_MONOCHROME;
      output_type = OUTPUT_GRAY;
      bits = 1;
    }
  else if (output_type == OUTPUT_GRAY)
    colormode = COLOR_MONOCHROME;
  else if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
    colormode = COLOR_CCMMYK;
  else
    colormode = COLOR_CMYK;


 /*
  * Setup a read-only pixel region for the entire image...
  */

  Image_init(image);
  image_height = Image_height(image);
  image_width = Image_width(image);
  image_bpp = Image_bpp(image);

 /*
  * Choose the correct color conversion function...
  */

  if (image_bpp < 3 && cmap == NULL && output_type == OUTPUT_COLOR)
    output_type = OUTPUT_GRAY_COLOR;	/* Force grayscale output */

  if (output_type == OUTPUT_COLOR)
  {
    out_bpp = 3;

    if (image_bpp >= 3)
      colorfunc = rgb_to_rgb;
    else
      colorfunc = indexed_to_rgb;
  }
  else if (output_type == OUTPUT_GRAY_COLOR)
  {
    out_bpp = 3;
    colorfunc = gray_to_rgb;
  }
  else
  {
    out_bpp = 1;

    if (image_bpp >= 3)
      colorfunc = rgb_to_gray;
    else if (cmap == NULL)
      colorfunc = gray_to_gray;
    else
      colorfunc = indexed_to_gray;
  }

 /*
  * Figure out the output resolution...
  */
  for (res = &escp2_reslist[0];;res++)
    {
      if (!strcmp(resolution, res->name))
	{
	  use_softweave = res->softweave;
	  horizontal_passes = res->horizontal_passes;
	  real_horizontal_passes = horizontal_passes;
	  vertical_passes = res->vertical_passes;
	  vertical_subsample = res->vertical_subsample;
	  if (use_softweave && escp2_xres(model) < 720)
	    horizontal_passes *= 720 / escp2_xres(model);
	  xdpi = res->hres;
	  ydpi = res->vres;
	  if (output_type == OUTPUT_GRAY)
	    {
	      nozzles = escp2_black_nozzles(model);
	      if (nozzles == 0)
		nozzles = escp2_nozzles(model);
	    }
	  else
	    nozzles = escp2_nozzles(model);
	  nozzle_separation = escp2_nozzle_separation(model);
	  break;
	}
      else if (!strcmp(resolution, ""))
	{
	  return;	  
	}
    }
  if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4) &&
      use_softweave && strcmp(ink_type, "Single Dot Size") != 0 &&
      nv.image_type != IMAGE_MONOCHROME)
    bits = 2;
  else
    bits = 1;
 /*
  * Compute the output size...
  */

  landscape   = 0;
  escp2_imageable_area(model, ppd_file, media_size, &page_left, &page_right,
                       &page_bottom, &page_top);

  page_width  = page_right - page_left;
  page_height = page_top - page_bottom;

  default_media_size(model, ppd_file, media_size, &n, &page_length);

 /*
  * Portrait width/height...
  */

  if (scaling < 0.0)
  {
   /*
    * Scale to pixels per inch...
    */

    out_width  = image_width * -72.0 / scaling;
    out_height = image_height * -72.0 / scaling;
  }
  else
  {
   /*
    * Scale by percent...
    */

    out_width  = page_width * scaling / 100.0;
    out_height = out_width * image_height / image_width;
    if (out_height > page_height)
    {
      out_height = page_height * scaling / 100.0;
      out_width  = out_height * image_width / image_height;
    }
  }

  if (out_width == 0)
    out_width = 1;
  if (out_height == 0)
    out_height = 1;

 /*
  * Landscape width/height...
  */

  if (scaling < 0.0)
  {
   /*
    * Scale to pixels per inch...
    */

    temp_width  = image_height * -72.0 / scaling;
    temp_height = image_width * -72.0 / scaling;
  }
  else
  {
   /*
    * Scale by percent...
    */

    temp_width  = page_width * scaling / 100.0;
    temp_height = temp_width * image_width / image_height;
    if (temp_height > page_height)
    {
      temp_height = page_height;
      temp_width  = temp_height * image_height / image_width;
    }
  }

 /*
  * See which orientation has the greatest area (or if we need to rotate the
  * image to fit it on the page...)
  */

  if (orientation == ORIENT_AUTO)
  {
    if (scaling < 0.0)
    {
      if ((out_width > page_width && out_height < page_width) ||
          (out_height > page_height && out_width < page_height))
	orientation = ORIENT_LANDSCAPE;
      else
	orientation = ORIENT_PORTRAIT;
    }
    else
    {
      if ((temp_width * temp_height) > (out_width * out_height))
	orientation = ORIENT_LANDSCAPE;
      else
	orientation = ORIENT_PORTRAIT;
    }
  }

  if (orientation == ORIENT_LANDSCAPE)
  {
    out_width  = temp_width;
    out_height = temp_height;
    landscape  = 1;

   /*
    * Swap left/top offsets...
    */

    x    = top;
    top  = left;
    left = page_width - x - out_width;
  }

  if (left < 0)
    left = (page_width - out_width) / 2;

  if (top < 0)
    top  = (page_height - out_height) / 2;

 /*
  * Let the user know what we're doing...
  */

  Image_progress_init(image);

 /*
  * Send ESC/P2 initialization commands...
  */
  page_length += (39 + (escp2_nozzles(model) * 2) *
		  escp2_nozzle_separation(model)) / 10; /* Top and bottom */
  page_top = 0;

  escp2_init_printer(prn, model, output_type, ydpi, xdpi, use_softweave,
		     page_length, page_width, page_top, page_bottom,
		     top, nozzles, nozzle_separation,
		     horizontal_passes, vertical_passes, vertical_subsample,
		     bits);

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  left = ydpi * left / 72;
  if (ydpi > 720)
    left = left * 720 / ydpi;

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  if (output_type == OUTPUT_GRAY)
  {
    black   = malloc(length * bits);
    cyan    = NULL;
    magenta = NULL;
    lcyan    = NULL;
    lmagenta = NULL;
    yellow  = NULL;
  }
  else
  {
    cyan    = malloc(length * bits);
    magenta = malloc(length * bits);
    yellow  = malloc(length * bits);
  
    if (escp2_has_cap(model, MODEL_HASBLACK_MASK, MODEL_HASBLACK_YES))
      black = malloc(length * bits);
    else
      black = NULL;
    if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES)) {
      lcyan = malloc(length * bits);
      lmagenta = malloc(length * bits);
    } else {
      lcyan = NULL;
      lmagenta = NULL;
    }
  }
  if (use_softweave)
    /* Epson printers are all 720 physical dpi */
    weave = initialize_weave(nozzles, nozzle_separation, horizontal_passes,
			     vertical_passes, vertical_subsample, colormode,
			     bits, out_width * escp2_xres(model) / 720,
			     out_height, separation_rows,
			     top * 720 / 72, page_height * 720 / 72);
  else
    escp2_init_microweave();
      

 /*
  * Output the page, rotating as necessary...
  */

  nv.density = nv.density * printer->printvars.density /
    (horizontal_passes * vertical_subsample);
  if (bits == 2)
    nv.density *= 3.3;
  if (nv.density > 1.0)
    nv.density = 1.0;
  nv.saturation *= printer->printvars.saturation;

  if (landscape)
    dither = init_dither(image_height, out_width, &nv);
  else
    dither = init_dither(image_width, out_width, &nv);

  dither_set_black_levels(dither, 1.0, 1.0, 1.0);
  if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
    dither_set_black_lower(dither, .5 / bits);
  else
    dither_set_black_lower(dither, .25 / bits);
  if (use_glossy_film)
    dither_set_black_upper(dither, .999);
  else
    dither_set_black_upper(dither, .6);
  if (bits == 2)
    dither_set_adaptive_divisor(dither, 8);
  else
    dither_set_adaptive_divisor(dither, 2);
  if (bits == 2)
    {
      int dsize = (sizeof(variable_dither_ranges) /
		   sizeof(simple_dither_range_t));
      dither_set_y_ranges_simple(dither, 3, dot_sizes, nv.density);
      dither_set_k_ranges_simple(dither, 3, dot_sizes, nv.density);
      if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
	{
	  dither_set_c_ranges(dither, dsize, variable_dither_ranges,
			      nv.density);
	  dither_set_m_ranges(dither, dsize, variable_dither_ranges,
			      nv.density);
	}
      else
	{	
	  dither_set_c_ranges_simple(dither, 3, dot_sizes, nv.density);
	  dither_set_m_ranges_simple(dither, 3, dot_sizes, nv.density);
	}
    }
  else if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
    dither_set_light_inks(dither, .25, .25, 0.0, nv.density);

  switch (nv.image_type)
    {
    case IMAGE_LINE_ART:
      dither_set_ink_spread(dither, 19);
      break;
    case IMAGE_SOLID_TONE:
      dither_set_ink_spread(dither, 15);
      break;
    case IMAGE_CONTINUOUS:
      ink_spread = 13;
      if (ydpi > 720)
	ink_spread++;
      if (bits > 1)
	ink_spread++;
      dither_set_ink_spread(dither, ink_spread);
      break;
    }	    
  dither_set_density(dither, nv.density);

  if (landscape)
  {
    in  = malloc(image_height * image_bpp);
    out = malloc(image_height * out_bpp * 2);

    errdiv  = image_width / out_height;
    errmod  = image_width % out_height;
    errval  = 0;
    errlast = -1;
    errline  = image_width - 1;
    
    for (x = 0; x < out_height; x ++)
    {
      if ((x & 255) == 0)
 	Image_note_progress(image, x, out_height);

      if (errline != errlast)
      {
        errlast = errline;
	Image_get_col(image, in, errline);
      }

      (*colorfunc)(in, out, image_height, image_bpp, cmap, &nv);

      if (nv.image_type == IMAGE_MONOCHROME)
	dither_fastblack(out, x, dither, black);
      else if (output_type == OUTPUT_GRAY)
	dither_black(out, x, dither, black);
      else
	dither_cmyk(out, x, dither, cyan, lcyan, magenta, lmagenta,
		    yellow, 0, black);

      if (use_softweave)
	escp2_write_weave(weave, prn, length, ydpi, model, out_width, left,
			  xdpi, cyan, magenta, yellow, black, lcyan, lmagenta);
      else
	escp2_write_microweave(prn, black, cyan, magenta, yellow, lcyan,
			       lmagenta, length, xdpi, ydpi, model,
			       out_width, left, bits);

      errval += errmod;
      errline -= errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline --;
      }
    }
    if (use_softweave)
      escp2_flush(weave, model, out_width, left, ydpi, xdpi, prn);
    else
      escp2_free_microweave();
  }
  else
  {
    in  = malloc(image_width * image_bpp);
    out = malloc(image_width * out_bpp * 2);

    errdiv  = image_height / out_height;
    errmod  = image_height % out_height;
    errval  = 0;
    errlast = -1;
    errline  = 0;
    
    for (y = 0; y < out_height; y ++)
    {
      if ((y & 255) == 0)
	Image_note_progress(image, y, out_height);

      if (errline != errlast)
      {
        errlast = errline;
	Image_get_row(image, in, errline);
      }

      (*colorfunc)(in, out, image_width, image_bpp, cmap, &nv);

      if (nv.image_type == IMAGE_MONOCHROME)
	dither_fastblack(out, y, dither, black);
      else if (output_type == OUTPUT_GRAY)
	dither_black(out, y, dither, black);
      else
	dither_cmyk(out, y, dither, cyan, lcyan, magenta, lmagenta,
		    yellow, 0, black);

      if (use_softweave)
	escp2_write_weave(weave, prn, length, ydpi, model, out_width, left,
			  xdpi, cyan, magenta, yellow, black, lcyan, lmagenta);
      else
	escp2_write_microweave(prn, black, cyan, magenta, yellow, lcyan,
			       lmagenta, length, xdpi, ydpi, model,
			       out_width, left, bits);
      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline ++;
      }
    }
    if (use_softweave)
      escp2_flush(weave, model, out_width, left, ydpi, xdpi, prn);
    else
      escp2_free_microweave();
  }
  free_dither(dither);

 /*
  * Cleanup...
  */

  free(in);
  free(out);
  if (use_softweave)
    destroy_weave(weave);

  if (black != NULL)
    free(black);
  if (cyan != NULL)
    {
      free(cyan);
      free(magenta);
      free(yellow);
    }
  if (lcyan != NULL)
    {
      free(lcyan);
      free(lmagenta);
    }

  putc('\014', prn);			/* Eject page */
  fputs("\033@", prn);			/* ESC/P2 reset */
}

static void
escp2_fold(const unsigned char *line,
	   int single_length,
	   unsigned char *outbuf)
{
  int i;
  for (i = 0; i < single_length; i++)
    {
      outbuf[0] =
	((line[0] & (1 << 7)) >> 1) +
	((line[0] & (1 << 6)) >> 2) +
	((line[0] & (1 << 5)) >> 3) +
	((line[0] & (1 << 4)) >> 4) +
	((line[single_length] & (1 << 7)) >> 0) +
	((line[single_length] & (1 << 6)) >> 1) +
	((line[single_length] & (1 << 5)) >> 2) +
	((line[single_length] & (1 << 4)) >> 3);
      outbuf[1] =
	((line[0] & (1 << 3)) << 3) +
	((line[0] & (1 << 2)) << 2) +
	((line[0] & (1 << 1)) << 1) +
	((line[0] & (1 << 0)) << 0) +
	((line[single_length] & (1 << 3)) << 4) +
	((line[single_length] & (1 << 2)) << 3) +
	((line[single_length] & (1 << 1)) << 2) +
	((line[single_length] & (1 << 0)) << 1);
      line++;
      outbuf += 2;
    }
}

static void
escp2_split_2(int length,
	      int bits,
	      const unsigned char *in,
	      unsigned char *outhi,
	      unsigned char *outlo)
{
  int i, j;
  int row = 0;
  int base = (1 << bits) - 1;
  for (i = 0; i < length * 2; i++)
    {
      unsigned char inbyte = in[i];
      outlo[i] = 0;
      outhi[i] = 0;
      for (j = base; j < 256; j <<= bits)
	{
	  if (inbyte & j)
	    {
	      if (row == 0)
		{
		  outlo[i] |= j & inbyte;
		  row = 1;
		}
	      else
		{
		  outhi[i] |= j & inbyte;
		  row = 0;
		}
	    }
	}
    }
}

static void
escp2_split_4(int length,
	      int bits,
	      const unsigned char *in,
	      unsigned char *out0,
	      unsigned char *out1,
	      unsigned char *out2,
	      unsigned char *out3)
{
  int i, j;
  int row = 0;
  int base = (1 << bits) - 1;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = in[i];
      out0[i] = 0;
      out1[i] = 0;
      out2[i] = 0;
      out3[i] = 0;
      for (j = base; j < 256; j <<= bits)
	{
	  if (inbyte & j)
	    {
	      switch (row)
		{
		case 0:
		  out0[i] |= j & inbyte;
		  break;
		case 1:
		  out1[i] |= j & inbyte;
		  break;
		case 2:
		  out2[i] |= j & inbyte;
		  break;
		case 3:
		  out3[i] |= j & inbyte;
		  break;
		}
	      row = (row + 1) & 3;
	    }
	}
    }
}


static void
escp2_unpack_2_1(int length,
		 const unsigned char *in,
		 unsigned char *outlo,
		 unsigned char *outhi)
{
  int i;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = *in;
      if (!(i & 1))
	{
	  *outlo =
	    ((inbyte & (1 << 7)) << 0) +
	    ((inbyte & (1 << 5)) << 1) +
	    ((inbyte & (1 << 3)) << 2) +
	    ((inbyte & (1 << 1)) << 3);
	  *outhi =
	    ((inbyte & (1 << 6)) << 1) +
	    ((inbyte & (1 << 4)) << 2) +
	    ((inbyte & (1 << 2)) << 3) +
	    ((inbyte & (1 << 0)) << 4);
	}
      else
	{
	  *outlo +=
	    ((inbyte & (1 << 1)) >> 1) +
	    ((inbyte & (1 << 3)) >> 2) +
	    ((inbyte & (1 << 5)) >> 3) +
	    ((inbyte & (1 << 7)) >> 4);
	  *outhi +=
	    ((inbyte & (1 << 0)) >> 0) +
	    ((inbyte & (1 << 2)) >> 1) +
	    ((inbyte & (1 << 4)) >> 2) +
	    ((inbyte & (1 << 6)) >> 3);
	  outlo++;
	  outhi++;
	}
      in++;
    }
}

static void
escp2_unpack_2_2(int length,
		 const unsigned char *in,
		 unsigned char *outlo,
		 unsigned char *outhi)
{
  int i;
  for (i = 0; i < length * 2; i++)
    {
      unsigned char inbyte = *in;
      if (!(i & 1))
	{
	  *outlo =
	    ((inbyte & (3 << 6)) << 0) +
	    ((inbyte & (3 << 2)) << 2);
	  *outhi =
	    ((inbyte & (3 << 4)) << 2) +
	    ((inbyte & (3 << 0)) << 4);
	}
      else
	{
	  *outlo +=
	    ((inbyte & (3 << 6)) >> 4) +
	    ((inbyte & (3 << 2)) >> 2);
	  *outhi +=
	    ((inbyte & (3 << 4)) >> 2) +
	    ((inbyte & (3 << 0)) >> 0);
	  outlo++;
	  outhi++;
	}
      in++;
    }
}

static void
escp2_unpack_2(int length,
	       int bits,
	       const unsigned char *in,
	       unsigned char *outlo,
	       unsigned char *outhi)
{
  if (bits == 1)
    escp2_unpack_2_1(length, in, outlo, outhi);
  else
    escp2_unpack_2_2(length, in, outlo, outhi);
}

static void
escp2_unpack_4_1(int length,
		 const unsigned char *in,
		 unsigned char *out0,
		 unsigned char *out1,
		 unsigned char *out2,
		 unsigned char *out3)
{
  int i;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = *in;
      switch (i & 3)
	{
	case 0:
	  *out0 =
	    ((inbyte & (1 << 7)) << 0) +
	    ((inbyte & (1 << 3)) << 3);
	  *out1 =
	    ((inbyte & (1 << 6)) << 1) +
	    ((inbyte & (1 << 2)) << 4);
	  *out2 =
	    ((inbyte & (1 << 5)) << 2) +
	    ((inbyte & (1 << 1)) << 5);
	  *out3 =
	    ((inbyte & (1 << 4)) << 3) +
	    ((inbyte & (1 << 0)) << 6);
	  break;
	case 1:
	  *out0 +=
	    ((inbyte & (1 << 7)) >> 2) +
	    ((inbyte & (1 << 3)) << 1);
	  *out1 +=
	    ((inbyte & (1 << 6)) >> 1) +
	    ((inbyte & (1 << 2)) << 2);
	  *out2 +=
	    ((inbyte & (1 << 5)) >> 0) +
	    ((inbyte & (1 << 1)) << 3);
	  *out3 +=
	    ((inbyte & (1 << 4)) << 1) +
	    ((inbyte & (1 << 0)) << 4);
	  break;
	case 2:
	  *out0 +=
	    ((inbyte & (1 << 7)) >> 4) +
	    ((inbyte & (1 << 3)) >> 1);
	  *out1 +=
	    ((inbyte & (1 << 6)) >> 3) +
	    ((inbyte & (1 << 2)) << 0);
	  *out2 +=
	    ((inbyte & (1 << 5)) >> 2) +
	    ((inbyte & (1 << 1)) << 1);
	  *out3 +=
	    ((inbyte & (1 << 4)) >> 1) +
	    ((inbyte & (1 << 0)) << 2);
	  break;
	case 3:
	  *out0 +=
	    ((inbyte & (1 << 7)) >> 6) +
	    ((inbyte & (1 << 3)) >> 3);
	  *out1 +=
	    ((inbyte & (1 << 6)) >> 5) +
	    ((inbyte & (1 << 2)) >> 2);
	  *out2 +=
	    ((inbyte & (1 << 5)) >> 4) +
	    ((inbyte & (1 << 1)) >> 1);
	  *out3 +=
	    ((inbyte & (1 << 4)) >> 3) +
	    ((inbyte & (1 << 0)) >> 0);
	  out0++;
	  out1++;
	  out2++;
	  out3++;
	  break;
	}
      in++;
    }
}

static void
escp2_unpack_4_2(int length,
		 const unsigned char *in,
		 unsigned char *out0,
		 unsigned char *out1,
		 unsigned char *out2,
		 unsigned char *out3)
{
  int i;
  for (i = 0; i < length * 2; i++)
    {
      unsigned char inbyte = *in;
      switch (i & 3)
	{
	case 0:
	  *out0 = ((inbyte & (3 << 6)) << 0);
	  *out1 = ((inbyte & (3 << 4)) << 2);
	  *out2 = ((inbyte & (3 << 2)) << 4);
	  *out3 = ((inbyte & (3 << 0)) << 6);
	  break;
	case 1:
	  *out0 += ((inbyte & (3 << 6)) >> 2);
	  *out1 += ((inbyte & (3 << 4)) << 0);
	  *out2 += ((inbyte & (3 << 2)) << 2);
	  *out3 += ((inbyte & (3 << 0)) << 4);
	  break;
	case 2:
	  *out0 += ((inbyte & (3 << 6)) >> 4);
	  *out1 += ((inbyte & (3 << 4)) >> 2);
	  *out2 += ((inbyte & (3 << 2)) << 0);
	  *out3 += ((inbyte & (3 << 0)) << 2);
	  break;
	case 3:
	  *out0 += ((inbyte & (3 << 6)) >> 6);
	  *out1 += ((inbyte & (3 << 4)) >> 4);
	  *out2 += ((inbyte & (3 << 2)) >> 2);
	  *out3 += ((inbyte & (3 << 0)) >> 0);
	  out0++;
	  out1++;
	  out2++;
	  out3++;
	  break;
	}
      in++;
    }
}

static void
escp2_unpack_4(int length,
	       int bits,
	       const unsigned char *in,
	       unsigned char *out0,
	       unsigned char *out1,
	       unsigned char *out2,
	       unsigned char *out3)
{
  if (bits == 1)
    escp2_unpack_4_1(length, in, out0, out1, out2, out3);
  else
    escp2_unpack_4_2(length, in, out0, out1, out2, out3);
}

static int
escp2_pack(const unsigned char *line,
	   int length,
	   unsigned char *comp_buf,
	   unsigned char **comp_ptr)
{
  const unsigned char *start;		/* Start of compressed data */
  unsigned char repeat;			/* Repeating char */
  int count;			/* Count of compressed bytes */
  int tcount;			/* Temporary count < 128 */
  int active = 0;		/* Have we found data? */

  /*
   * Compress using TIFF "packbits" run-length encoding...
   */

  (*comp_ptr) = comp_buf;

  while (length > 0)
    {
      /*
       * Get a run of non-repeated chars...
       */

      start  = line;
      line   += 2;
      length -= 2;

      while (length > 0 && (line[-2] != line[-1] || line[-1] != line[0]))
	{
	  if (! active && (line[-2] || line[-1] || line[0]))
	    active = 1;
	  line ++;
	  length --;
	}

      line   -= 2;
      length += 2;

      /*
       * Output the non-repeated sequences (max 128 at a time).
       */

      count = line - start;
      while (count > 0)
	{
	  tcount = count > 128 ? 128 : count;

	  (*comp_ptr)[0] = tcount - 1;
	  memcpy((*comp_ptr) + 1, start, tcount);

	  (*comp_ptr) += tcount + 1;
	  start    += tcount;
	  count    -= tcount;
	}

      if (length <= 0)
	break;

      /*
       * Find the repeated sequences...
       */

      start  = line;
      repeat = line[0];
      if (repeat)
	active = 1;

      line ++;
      length --;

      while (length > 0 && *line == repeat)
	{
	  line ++;
	  length --;
	}

      /*
       * Output the repeated sequences (max 128 at a time).
       */

      count = line - start;
      while (count > 0)
	{
	  tcount = count > 128 ? 128 : count;

	  (*comp_ptr)[0] = 1 - tcount;
	  (*comp_ptr)[1] = repeat;

	  (*comp_ptr) += 2;
	  count    -= tcount;
	}
    }
  return active;
}

static unsigned char *microweave_s = 0;
static unsigned char *microweave_comp_ptr[6][4];
static int microweave_setactive[6][4];
static int accumulated_spacing = 0;
static int last_color = -1;

#define MICRO_S(c, l) (microweave_s + COMPBUFWIDTH * (l) + COMPBUFWIDTH * (c) * 4)

static void
escp2_init_microweave()
{
  if (!microweave_s)
    microweave_s = malloc(6 * 4 * COMPBUFWIDTH);
  accumulated_spacing = 0;
}

static void
escp2_free_microweave()
{
  if (microweave_s)
    {
      free(microweave_s);
      microweave_s = NULL;
    }
}

static int
escp2_do_microweave_pack(const unsigned char *line,
			 int length,
			 int oversample,
			 int bits,
			 int color)
{
  static unsigned char *pack_buf = NULL;
  static unsigned char *s[4] = { NULL, NULL, NULL, NULL };
  const unsigned char *in;
  int i;
  int retval = 0;
  if (!pack_buf)
    pack_buf = malloc(COMPBUFWIDTH);
  for (i = 0; i < oversample; i++)
    {
      if (!s[i])
	s[i] = malloc(COMPBUFWIDTH);
    }

  if (!line ||
      (line[0] == 0 && memcmp(line, line + 1, (bits * length) - 1) == 0))
    {
      for (i = 0; i < 4; i++)
	microweave_setactive[color][i] = 0;
      return 0;
    }
  if (bits == 1)
    in = line;
  else
    {
      escp2_fold(line, length, pack_buf);
      in = pack_buf;
    }
  switch (oversample)
    {
    case 1:
      memcpy(s[0], in, bits * length);
      break;
    case 2:
      escp2_unpack_2(length, bits, in, s[0], s[1]);
      break;
    case 4:
      escp2_unpack_4(length, bits, in, s[0], s[1], s[2], s[3]);
      break;
    }
  for (i = 0; i < oversample; i++)
    {
      microweave_setactive[color][i] =
	escp2_pack(s[i], length * bits, MICRO_S(color, i),
		   &(microweave_comp_ptr[color][i]));
      retval |= microweave_setactive[color][i];
    }
  return retval;
}

static void
escp2_write_microweave(FILE          *prn,	/* I - Print file or command */
		       const unsigned char *k,	/* I - Output bitmap data */
		       const unsigned char *c,	/* I - Output bitmap data */
		       const unsigned char *m,	/* I - Output bitmap data */
		       const unsigned char *y,	/* I - Output bitmap data */
		       const unsigned char *lc,	/* I - Output bitmap data */
		       const unsigned char *lm,	/* I - Output bitmap data */
		       int           length,	/* I - Length of bitmap data */
		       int           xdpi,	/* I - Horizontal resolution */
		       int           ydpi,	/* I - Vertical resolution */
		       int           model,	/* I - Printer model */
		       int           width,	/* I - Printed width */
		       int           offset,	/* I - Offset from left side */
		       int	     bits)
{
  int i, j;
  int oversample = 1;
  int gsetactive = 0;
  if (xdpi > 720)
    oversample = xdpi / 720;

  gsetactive |= escp2_do_microweave_pack(k, length, oversample, bits, 0);
  gsetactive |= escp2_do_microweave_pack(m, length, oversample, bits, 1);
  gsetactive |= escp2_do_microweave_pack(c, length, oversample, bits, 2);
  gsetactive |= escp2_do_microweave_pack(y, length, oversample, bits, 3);
  gsetactive |= escp2_do_microweave_pack(lm, length, oversample, bits, 4);
  gsetactive |= escp2_do_microweave_pack(lc, length, oversample, bits, 5);
  if (!gsetactive)
    {
      accumulated_spacing++;
      return;
    }
  for (i = 0; i < oversample; i++)
    {
      for (j = 0; j < 6; j++)
	{
	  if (!microweave_setactive[j][i])
	    continue;
	  if (accumulated_spacing > 0)
	    fprintf(prn, "\033(v\002%c%c%c", 0, accumulated_spacing % 256,
		    (accumulated_spacing >> 8) % 256);
	  accumulated_spacing = 0;
	  /*
	   * Set the print head position.
	   */

	  if (escp2_has_cap(model, MODEL_1440DPI_MASK, MODEL_1440DPI_YES) &&
	      xdpi > 720)
	    {
	      if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK,
				MODEL_VARIABLE_4))
		{
		  if (((offset * xdpi / 1440) + i) > 0)
		    fprintf(prn, "\033($%c%c%c%c%c%c", 4, 0,
			    ((offset * xdpi / 1440) + i) & 255,
			    (((offset * xdpi / 1440) + i) >> 8) & 255,
			    (((offset * xdpi / 1440) + i) >> 16) & 255,
			    (((offset * xdpi / 1440) + i) >> 24) & 255);
		}
	      else
		{
		  if (((offset * 1440 / ydpi) + i) > 0)
		    fprintf(prn, "\033(\\%c%c%c%c%c%c", 4, 0, 160, 5,
			    ((offset * 1440 / ydpi) + i) & 255,
			    ((offset * 1440 / ydpi) + i) >> 8);
		}
	    }
	  else
	    {
	      if (offset > 0)
		fprintf(prn, "\033\\%c%c", offset & 255, offset >> 8);
	    }
	  if (j != last_color)
	    {
	      if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
		fprintf(prn, "\033(r\002%c%c%c", 0, densities[j], colors[j]);
	      else
		fprintf(prn, "\033r%c", colors[j]);
	      last_color = j;
	    }
	  /*
	   * Send a line of raster graphics...
	   */

	  switch (ydpi)				/* Raster graphics header */
	    {
	    case 180 :
	      fwrite("\033.\001\024\024\001", 6, 1, prn);
	      break;
	    case 360 :
	      fwrite("\033.\001\012\012\001", 6, 1, prn);
	      break;
	    case 720 :
	      if (escp2_has_cap(model, MODEL_720DPI_MODE_MASK,
				MODEL_720DPI_600))
		fwrite("\033.\001\050\005\001", 6, 1, prn);
	      else
		fwrite("\033.\001\005\005\001", 6, 1, prn);
	      break;
	    }
	  putc(width & 255, prn);	/* Width of raster line in pixels */
	  putc(width >> 8, prn);

	  fwrite(MICRO_S(j, i), microweave_comp_ptr[j][i] - MICRO_S(j, i),
		 1, prn);
	  putc('\r', prn);
	}
    }
  accumulated_spacing++;
}


/*
 * "Soft" weave
 *
 * The Epson Stylus Color/Photo printers don't have memory to print
 * using all of the nozzles in the print head.  For example, the Stylus Photo
 * 700/EX has 32 nozzles.  At 720 dpi, with an 8" wide image, a single line
 * requires (8 * 720 * 6 / 8) bytes, or 4320 bytes (because the Stylus Photo
 * printers have 6 ink colors).  To use 32 nozzles would require 138240 bytes.
 * It's actually worse than that, though, because the nozzles are spaced 8
 * rows apart.  Therefore, in order to store enough data to permit sending the
 * page as a simple raster, the printer would require enough memory to store
 * 256 rows, or 1105920 bytes.  Considering that the Photo EX can print
 * 11" wide, we're looking at more like 1.5 MB.  In fact, these printers are
 * capable of 1440 dpi horizontal resolution.  This would require 3 MB.  The
 * printers actually have 64K.
 *
 * With the newer (750 and 1200) printers it's even worse, since these printers
 * support multiple dot sizes.  But that's neither here nor there.
 *
 * The printer is capable of printing an image fed to it as single raster
 * lines.  This is called MicroWeave (tm).  It actually produces extremely
 * high quality output, but it only uses one nozzle per color per pass.
 * This means that it has to make a lot of passes to print a page, so it's
 * extremely slow (a full 8.5x11" page takes over 30 minutes!).  It's also
 * not possible to print very close to the bottom of the page with MicroWeave
 * since only the first nozzle is used, and the head cannot get closer than
 * some distance from the edge of the page.
 *
 * The solution is to have the host rearrange the output so that a single
 * pass is fed to the print head.  This means that we have to feed the printer
 * every 8th line as a single pass, and we then have to interleave ("weave")
 * the other raster lines as separate passes.  This allows us to use all 32
 * nozzles, and achieve much higher printing speed.
 *
 * What makes this interesting is that there are many different ways of
 * of accomplishing this goal.  The naive way would be to divide the image
 * up into groups of 256 rows, and print all the mod8=0 rows in the first pass,
 * mod8=1 rows in the second, and so forth.  The problem with this approach
 * is that the individual ink jets are not perfectly uniform; some emit
 * slightly bigger or smaller drops than others.  Since each group of 8
 * adjacent rows is printed with the same nozzle, that means that there will
 * be distinct streaks of lighter and darker bands within the image (8 rows
 * is 1/90", which is visible; 1/720" is not).  Possibly worse is that these
 * patterns will repeat every 256 rows.  This creates banding patterns that
 * are about 1/3" wide.
 *
 * So we have to do something to break up this patterning.
 *
 * Epson does not publish the weaving algorithms that they use in their
 * bundled drivers.  Indeed, their developer web site
 * (http://www.ercipd.com/isv/edr_docs.htm) does not even describe how to
 * do this weaving at all; it says that the only way to achieve 720 dpi
 * is to use MicroWeave.  It does note (correctly) that 1440 dpi horizontal
 * can only be achieved by the driver (i. e. in software).  The manual
 * actually makes it fairly clear how to do this (it requires two passes
 * with horizontal head movement between passes), and it is presumably
 * possible to do this with MicroWeave.
 *
 * The information about how to do this is apparently available under NDA.
 * It's actually easy enough to reverse engineer what's inside a print file
 * with a simple Perl script.  There are presumably other printer commands
 * that are not documented and may not be as easy to reverse engineer.
 *
 * I considered a few algorithms to perform the weave.  The first one I
 * devised let me use only (jets - distance_between_jets + 1) nozzles, or
 * 25.  This is OK in principle, but it's slower than using all nozzles.
 * By playing around with it some more, I came up with an algorithm that
 * lets me use all of the nozzles, except near the top and bottom of the
 * page.
 *
 * This still produces some banding, though.  Even better quality can be
 * achieved by using multiple nozzles on the same line.  How do we do this?
 * In 1440x720 mode, we're printing two output lines at the same vertical
 * position.  However, if we want four passes, we have to effectively print
 * each line twice.  Actually doing this would increase the density, so
 * what we do is print half the dots on each pass.  This produces near-perfect
 * output, and it's far faster than using "MicroWeave".
 *
 * The current algorithm is not completely general.  The number of passes
 * is limited to (nozzles / gap).  On the Photo EX class printers, that limits
 * it to 4 -- 32 nozzles, an inter-nozzle gap of 8 lines.  Furthermore, there
 * are a number of routines that are only coded up to 4 passes.
 *
 * The routine initialize_weave calculates the basic parameters, given
 * the number of jets and separation between jets, in rows.
 *
 * -- Robert Krawitz <rlk@alum.mit.edu) November 3, 1999
 */

#endif /* !WEAVETEST */

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

typedef union {			/* Is this line active? */
  char v[6];			/* (really pass) */
  struct {
    char k;
    char m;
    char c;
    char y;
    char M;
    char C;
  } p;
} lineactive_t;

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
  linebufs_t *linebases;	/* Base address of each row buffer */
  lineoff_t *lineoffsets;	/* Offsets within each row buffer */
  lineactive_t *lineactive;	/* Does this line have anything printed? */
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
  int realjets;			/* Actual number of jets */
  int pass_adjustment;		/* Magical */
  int post_pass_adjustment;	/* Even more magical */
  int ncolors;			/* How many colors (1, 4, or 6) */
  int horizontal_width;		/* Line width in output pixels */
  int vertical_height;		/* Image height in output pixels */
  int firstline;		/* Actual first line (referenced to paper) */
  int page_lines;		/* Number of lines on the paper (physical) */
  int header;			/* Number of initial lines that cannot be */
				/* printed with normal weave */
  int footer;			/* Number of trailing lines that cannot be */
				/* printed with normal weave */
  int header_passes;		/* Extra passes printed at the beginning */
  int last_real_pass;		/* Do not touch this upon pain of death */

  int bitwidth;			/* Bits per pixel */
  int lineno;
  int vertical_oversample;	/* Vertical oversampling */
  int current_vertical_subpass;
  int separation_rows;		/* Vertical spacing between rows. */
				/* This is used for the 1520/3000, which */
				/* use a funny value for the "print density */
				/* in the vertical direction". */
  int last_color;
} escp2_softweave_t;

#ifndef WEAVETEST
static int
get_color_by_params(int plane, int density)
{
  if (plane > 4 || plane < 0 || density > 1 || density < 0)
    return -1;
  return color_indices[density * 8 + plane];
}
#endif

/*
 * Initialize the weave parameters
 *
 * Rules:
 *
 * 1) Currently, osample * v_subpasses * v_subsample <= 4
 *
 * 2) first_line >= 0
 *
 * 3) line_height < physlines
 *
 * 4) phys_lines >= 2 * jets * sep
 */
static void *
initialize_weave(int jets,	/* Width of print head */
		 int sep,	/* Separation in rows between jets */
		 int osample,	/* Horizontal oversample */
		 int v_subpasses, /* Vertical passes */
		 int v_subsample, /* Vertical oversampling */
		 colormode_t colormode,	/* mono, 4 color, 6 color */
		 int width,	/* bits/pixel */
		 int linewidth,	/* Width of a line, in pixels */
		 int lineheight, /* Number of lines that will be printed */
		 int separation_rows, /* Vertical spacing adjustment */
				/* for weird printers (1520/3000, */
				/* although they don't seem to do softweave */
				/* anyway) */
		 int first_line, /* First line that will be printed on page */
		 int phys_lines) /* Total height of the page in rows */
{
  int i;
  int k;
  int lastline;
  escp2_softweave_t *sw = malloc(sizeof (escp2_softweave_t));
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
  sw->oversample = osample * v_subpasses * v_subsample;
  sw->vertical_subpasses = v_subpasses;
  sw->njets /= sw->oversample;
  if (sw->njets == 0)
    sw->njets = 1;
  sw->vmod = sw->separation * sw->oversample;
  sw->horizontal_weave = osample;
  sw->pass_adjustment = (sw->oversample * sep + jets - 1) / jets;
  sw->separation_rows = separation_rows;
  sw->firstline = first_line;
  if (first_line < sw->realjets * sw->separation)
    {
      sw->header = (sw->realjets * sw->separation) - first_line;
      if (sw->header < sw->separation)
	sw->header_passes = sw->header * sw->oversample;
      else
	sw->header_passes = sw->separation * sw->oversample;
    }
  else
    {
      sw->header = 0;
      sw->header_passes = 0;
    }
  lastline = first_line + lineheight;
  if (lastline > phys_lines)
    phys_lines = lastline + 1;
  sw->page_lines = phys_lines;
  if (lastline > phys_lines - (sw->realjets * sw->separation))
    sw->footer = phys_lines - first_line - (sw->realjets * sw->separation);
  else
    sw->footer = lastline;

  sw->weavefactor = (sw->njets + sw->separation - 1) / sw->separation;
  sw->jetsused = MIN(((sw->weavefactor) * sw->separation), sw->njets);
  sw->initialoffset = (sw->jetsused - sw->weavefactor - 1) * sw->separation;
  if (sw->initialoffset < 0)
    sw->initialoffset = 0;
  sw->jetsleftover = sw->njets - sw->jetsused;
  sw->weavespan = (sw->jetsused - 1) * sw->separation;
  sw->vertical_oversample = v_subsample;
  if (sw->separation > sw->njets)
    sw->post_pass_adjustment = (sw->separation / sw->njets);
  else
    sw->post_pass_adjustment = 0;
  sw->last_real_pass = -1;

  sw->bitwidth = width;

  sw->last_pass_offset = 0;
  sw->last_pass = -1;
  sw->current_vertical_subpass = 0;
  sw->last_color = -1;

  switch (colormode)
    {
    case COLOR_MONOCHROME:
      sw->ncolors = 1;
      break;
    case COLOR_CMYK:
      sw->ncolors = 4;
      break;
    case COLOR_CCMMYK:
    default:
      sw->ncolors = 6;
      break;
    }
  
  /*
   * It's possible for the "compression" to actually expand the line by
   * one part in 128.
   */

  sw->horizontal_width = (linewidth + 128 + 7) * 129 / 128;
  sw->vertical_height = lineheight;
  sw->lineoffsets = malloc(sw->vmod * sizeof(lineoff_t) * sw->oversample);
  sw->lineactive = malloc(sw->vmod * sizeof(lineactive_t) * sw->oversample);
  sw->linebases = malloc(sw->vmod * sizeof(linebufs_t) * sw->oversample);
  sw->passes = malloc(sw->vmod * sizeof(pass_t));
  sw->linecounts = malloc(sw->vmod * sizeof(int));
  sw->lineno = 0;

  for (i = 0; i < sw->vmod; i++)
    {
      int j;
      sw->passes[i].pass = -1;
      for (k = 0; k < sw->oversample; k++)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      sw->linebases[k * sw->vmod + i].v[j] =
		malloc(jets * sw->bitwidth * sw->horizontal_width / 8);
	    }
	}
    }
  return (void *) sw;
}

static void
destroy_weave(void *vsw)
{
  int i, j, k;
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  free(sw->linecounts);
  free(sw->passes);
  free(sw->lineactive);
  free(sw->lineoffsets);
  for (i = 0; i < sw->vmod; i++)
    {
      for (k = 0; k < sw->oversample; k++)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      free(sw->linebases[k * sw->vmod + i].v[j]);
	    }
	}
    }
  free(sw->linebases);
  free(vsw);
}

static int
footer(void *vsw)
{
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  return sw->footer;
}

#ifdef WEAVETEST

static void
set_last_pass(void *vsw, int pass)
{
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  sw->last_pass = pass;
}

#endif

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
    {
      kill(getpid(), SIGFPE);
      return 0;
    }
  else
    return x / y;
}

static int 
modd(int x, int y)
{
  if (x < 0 || y < 0)
    {
      kill(getpid(), SIGFPE);
      return 0;
    }
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
 * what's going on here.  That is, if I can figure it out myself :-)
 */

static inline int
roundup(int x, int y)
{
  if (x < 0)
    return -(y * divv(-x, y));
  else
    return y * divv((x + y - 1), y);
}

static inline int
rounddown(int x, int y)
{
  if (x < 0)
    return -(y * divv((y - x - 1), y));
  else
    return y * divv(x, y);
}

static void
weave_parameters_by_row(const escp2_softweave_t *sw, int row,
			int vertical_subpass, weave_t *w)
{
  w->row = row;
  if (row < sw->header)
    {
      int passbase;
      row = row + (sw->realjets * sw->separation) - sw->header;
      passbase = row % sw->separation;
      w->pass = passbase * sw->oversample + vertical_subpass;
      w->logicalpassstart = passbase + sw->header -
	(sw->realjets * sw->separation);
      if (w->logicalpassstart >= 0)
	w->physpassstart = w->logicalpassstart;
      else
	w->physpassstart = w->logicalpassstart +
	  roundup(-w->logicalpassstart, sw->separation);
      w->jet = (w->row - w->logicalpassstart) / sw->separation;
      if (w->logicalpassstart >= 0)
	w->missingstartrows = 0;
      else
	w->missingstartrows = (sw->separation - w->logicalpassstart - 1) /
	  sw->separation;
      w->physpassend = w->logicalpassstart +
	((sw->realjets - 1) * sw->separation);
      if (sw->header_passes < sw->separation * sw->oversample)
	w->pass -= ((sw->separation * sw->oversample) - sw->header_passes);
      if (w->pass > sw->last_real_pass)
	((escp2_softweave_t *) sw)->last_real_pass = w->pass;
    }
  else if (row >= sw->footer)
    {
      int passbase;
      row -= sw->footer;
      passbase = row % sw->separation;
      w->pass = passbase * sw->oversample + vertical_subpass +
	sw->last_real_pass + 1;
      w->logicalpassstart = passbase + sw->footer;
      w->physpassstart = w->logicalpassstart;
      w->jet = row / sw->separation;
      w->missingstartrows = 0;
      w->physpassend = w->logicalpassstart +
	(sw->realjets - 1) * sw->separation;
    }
  else
    {
      int passblockstart;
      int internaljetsused = sw->jetsused * sw->oversample;
      int subpass_adjustment;
      row -= sw->header;
      passblockstart = divv((row + sw->initialoffset), sw->jetsused);
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
      w->jet = divv((row + (sw->realjets * sw->separation) -
		     w->logicalpassstart), sw->separation) - sw->realjets;
      w->jet += sw->jetsused * (sw->oversample - 1);
      w->pass += sw->post_pass_adjustment;
      if (w->jet >= sw->realjets)
	{
	  w->jet -= sw->realjets;
	  w->pass += sw->vmod;
	}
      else if (w->jet < 0)
	{
	  w->jet += sw->realjets;
	  w->pass -= sw->vmod;
	}
      w->logicalpassstart = row - (w->jet * sw->separation);
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
      w->logicalpassstart += sw->header;
      w->physpassend += sw->header;
      w->physpassstart += sw->header;
      w->pass += sw->header_passes;
      if (w->pass > sw->last_real_pass)
	((escp2_softweave_t *) sw)->last_real_pass = w->pass;
    }
}

#ifndef WEAVETEST

static lineoff_t *
get_lineoffsets(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->lineoffsets[w.pass % sw->vmod]);
}

static lineactive_t *
get_lineactive(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->lineactive[w.pass % sw->vmod]);
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

static lineactive_t *
get_lineactive_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->lineactive[pass % sw->vmod]);
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

/*
 * If there are phantom rows at the beginning of a pass, fill them in so
 * that the printer knows exactly what it doesn't have to print.  We're
 * using RLE compression here.  Each line must be specified independently,
 * so we have to compute how many full blocks (groups of 128 bytes, or 1024
 * "off" pixels) and how much leftover is needed.  Note that we can only
 * RLE-encode groups of 2 or more bytes; single bytes must be specified
 * with a count of 1.
 */

static void
fillin_start_rows(const escp2_softweave_t *sw, int row, int subpass,
		  int width, int missingstartrows)
{
  lineoff_t *offsets = get_lineoffsets(sw, row, subpass);
  const linebufs_t *bufs = get_linebases(sw, row, subpass);
  int i = 0;
  int k = 0;
  int j;
  width = sw->bitwidth * width * 8;
  for (k = 0; k < missingstartrows; k++)
    {
      int bytes_to_fill = width;
      int full_blocks = bytes_to_fill / (128 * 8);
      int leftover = (7 + (bytes_to_fill % (128 * 8))) / 8;
      int l = 0;
  
      while (l < full_blocks)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      (bufs[0].v[j][2 * i]) = 129;
	      (bufs[0].v[j][2 * i + 1]) = 0;
	    }
	  i++;
	  l++;
	}
      if (leftover == 1)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      (bufs[0].v[j][2 * i]) = 1;
	      (bufs[0].v[j][2 * i + 1]) = 0;
	    }
	  i++;
	}
      else if (leftover > 0)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      (bufs[0].v[j][2 * i]) = 257 - leftover;
	      (bufs[0].v[j][2 * i + 1]) = 0;
	    }
	  i++;
	}
    }
  for (j = 0; j < sw->ncolors; j++)
    offsets[0].v[j] = 2 * i;
}

static void
initialize_row(const escp2_softweave_t *sw, int row, int width)
{
  weave_t w;
  int i;
  for (i = 0; i < sw->oversample; i++)
    {
      weave_parameters_by_row(sw, row, i, &w);
      if (w.physpassstart == row)
	{
	  lineoff_t *lineoffs = get_lineoffsets(sw, row, i);
	  lineactive_t *lineactive = get_lineactive(sw, row, i);
	  int *linecount = get_linecount(sw, row, i);
	  int j;
	  pass_t *pass = get_pass_by_row(sw, row, i);
	  pass->pass = w.pass;
	  pass->missingstartrows = w.missingstartrows;
	  pass->logicalpassstart = w.logicalpassstart;
	  pass->physpassstart = w.physpassstart;
	  pass->physpassend = w.physpassend;
	  pass->subpass = i;
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      lineoffs[0].v[j] = 0;
	      lineactive[0].v[j] = 0;
	    }
	  *linecount = 0;
	  if (w.missingstartrows > 0)
	    fillin_start_rows(sw, row, i, width, w.missingstartrows);
	}
    }
}

/*
 * A fair bit of this code is duplicated from escp2_write.  That's rather
 * a pity.  It's also not correct for any but the 6-color printers.  One of
 * these days I'll unify it.
 */
static void
flush_pass(escp2_softweave_t *sw, int passno, int model, int width,
	   int hoffset, int ydpi, int xdpi, FILE *prn, int vertical_subpass)
{
  int j;
  lineoff_t *lineoffs = get_lineoffsets_by_pass(sw, passno);
  lineactive_t *lineactive = get_lineactive_by_pass(sw, passno);
  const linebufs_t *bufs = get_linebases_by_pass(sw, passno);
  pass_t *pass = get_pass_by_pass(sw, passno);
  int *linecount = get_linecount_by_pass(sw, passno);
  int lwidth = (width + (sw->horizontal_weave - 1)) / sw->horizontal_weave;
  int microoffset = vertical_subpass & (sw->horizontal_weave - 1);
  if (ydpi > 720)
    ydpi = 720;
  if (passno == 0)
    {
      sw->last_pass_offset = pass->logicalpassstart;
      if (sw->firstline >= sw->realjets * sw->separation)
	sw->last_pass_offset -= sw->firstline - sw->realjets * sw->separation;
    }
  for (j = 0; j < sw->ncolors; j++)
    {
      if (lineactive[0].v[j] == 0)
	continue;
      if (pass->logicalpassstart > sw->last_pass_offset)
	{
	  int advance = pass->logicalpassstart - sw->last_pass_offset -
	    (sw->separation_rows - 1);
	  int alo = advance % 256;
	  int ahi = advance / 256;
	  if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4))
	    {
	      int a3 = (advance >> 16) % 256;
	      int a4 = (advance >> 24) % 256;
	      ahi = ahi % 256;
	      fprintf(prn, "\033(v\004%c%c%c%c%c", 0, alo, ahi, a3, a4);
	    }
	  else
	    fprintf(prn, "\033(v\002%c%c%c", 0, alo, ahi);
	  sw->last_pass_offset = pass->logicalpassstart;
	}
      if (last_color != j)
	{
	  if (ydpi >= 720 &&
	      escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4))
	    ;
	  else if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
	    fprintf(prn, "\033(r\002%c%c%c", 0, densities[j], colors[j]);
	  else
	    fprintf(prn, "\033r%c", colors[j]);
	  last_color = j;
	}
      if (escp2_has_cap(model, MODEL_1440DPI_MASK, MODEL_1440DPI_YES))
	{
	  /* FIXME need a more general way of specifying column */
	  /* separation */
	  if (escp2_has_cap(model, MODEL_COMMAND_MASK, MODEL_COMMAND_1999))
	    {
	      int pos = ((hoffset * xdpi / 720) + microoffset);
	      if (pos > 0)
		fprintf(prn, "\033($%c%c%c%c%c%c", 4, 0,
			pos & 255, (pos >> 8) & 255,
			(pos >> 16) & 255, (pos >> 24) & 255);
	    }
	  else
	    {
	      int pos = ((hoffset * 1440 / ydpi) + microoffset);
	      if (pos > 0)
		fprintf(prn, "\033(\\%c%c%c%c%c%c", 4, 0, 160, 5,
			pos & 255, pos >> 8);
	    }
	}
      else
	{
	  int pos = (hoffset + microoffset);
	  if (pos > 0)
	    fprintf(prn, "\033\\%c%c", pos & 255, pos >> 8);
	}
      if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4))
	{
	  int ncolor = (densities[j] << 4) | colors[j];
	  int nlines = *linecount + pass->missingstartrows;
	  int nwidth = sw->bitwidth * ((lwidth + 7) / 8);
	  fprintf(prn, "\033i%c%c%c%c%c%c%c", ncolor, 1, sw->bitwidth,
		  nwidth & 255, nwidth >> 8, nlines & 255, nlines >> 8);
	}
      else
	{
	  switch (ydpi)			/* Raster graphics header */
	    {
	    case 180 :
	      fwrite("\033.\001\024\024\001", 6, 1, prn);
	      break;
	    case 360 :
	      fwrite("\033.\001\012\012\001", 6, 1, prn);
	      break;
	    case 720 :
	      if (escp2_has_cap(model, MODEL_720DPI_MODE_MASK,
				MODEL_720DPI_600))
		fprintf(prn, "\033.%c%c%c%c", 1, 8 * 5, 5,
			*linecount + pass->missingstartrows);
	      else if (escp2_pseudo_separation_rows(model) > 0)
		fprintf(prn, "\033.%c%c%c%c", 1,
			5 * escp2_pseudo_separation_rows(model) , 5,
			*linecount + pass->missingstartrows);
	      else
		fprintf(prn, "\033.%c%c%c%c", 1, 5 * sw->separation_rows , 5,
			*linecount + pass->missingstartrows);
	      break;
	    }
	  putc(lwidth & 255, prn);	/* Width of raster line in pixels */
	  putc(lwidth >> 8, prn);
	}
	  
      fwrite(bufs[0].v[j], lineoffs[0].v[j], 1, prn);
      putc('\r', prn);
    }
  sw->last_pass = pass->pass;
  pass->pass = -1;
}

static void
add_to_row(escp2_softweave_t *sw, int row, unsigned char *buf, size_t nbytes,
	   int plane, int density, int subpass, int setactive)
{
  weave_t w;
  int color = get_color_by_params(plane, density);
  lineoff_t *lineoffs = get_lineoffsets(sw, row, subpass);
  lineactive_t *lineactive = get_lineactive(sw, row, subpass);
  const linebufs_t *bufs = get_linebases(sw, row, subpass);
  weave_parameters_by_row(sw, row, subpass, &w);
  memcpy(bufs[0].v[color] + lineoffs[0].v[color], buf, nbytes);
  lineoffs[0].v[color] += nbytes;
  if (setactive)
    lineactive[0].v[color] = 1;
}

static void
finalize_row(escp2_softweave_t *sw, int row, int model, int width,
	     int hoffset, int ydpi, int xdpi, FILE *prn)
{
  int i;
  for (i = 0; i < sw->oversample; i++)
    {
      weave_t w;
      int *lines = get_linecount(sw, row, i);
      weave_parameters_by_row(sw, row, i, &w);
      (*lines)++;
      if (w.physpassend == row)
	{
	  if (w.pass >= 0)
	    flush_pass(sw, w.pass, model, width, hoffset, ydpi, xdpi, prn, i);
	}
    }
  if (row == footer(sw) - 1)
    escp2_flush(sw, model, width, hoffset, ydpi, xdpi, prn);
}

static void
escp2_flush(void *vsw, int model, int width, int hoffset,
	    int ydpi, int xdpi, FILE *prn)
{
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  while (1)
    {
      pass_t *pass = get_pass_by_pass(sw, sw->last_pass + 1);
      if (pass->pass < 0)
	return;
      flush_pass(sw, pass->pass, model, width, hoffset, ydpi, xdpi, prn,
		 pass->subpass);
    }
}

static void
escp2_write_weave(void *        vsw,
		  FILE          *prn,	/* I - Print file or command */
		  int           length,	/* I - Length of bitmap data */
		  int           ydpi,	/* I - Vertical resolution */
		  int           model,	/* I - Printer model */
		  int           width,	/* I - Printed width */
		  int           offset,
		  int		xdpi,
		  const unsigned char *c,
		  const unsigned char *m,
		  const unsigned char *y,
		  const unsigned char *k,
		  const unsigned char *C,
		  const unsigned char *M)
{
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  static unsigned char *s[8];
  static unsigned char *fold_buf;
  static unsigned char *comp_buf;
  int xlength = (length + sw->horizontal_weave - 1) / sw->horizontal_weave;
  unsigned char *comp_ptr;
  int i, j;
  int setactive;
  int h_passes = sw->horizontal_weave * sw->vertical_subpasses;
  const unsigned char *cols[6];
  cols[0] = k;
  cols[1] = m;
  cols[2] = c;
  cols[3] = y;
  cols[4] = M;
  cols[5] = C;
  if (!fold_buf)
    fold_buf = malloc(COMPBUFWIDTH);
  if (!comp_buf)
    comp_buf = malloc(COMPBUFWIDTH);
  for (i = 0; i < sw->horizontal_weave * sw->vertical_subpasses; i++)
    if (!s[i])
      s[i] = malloc(COMPBUFWIDTH);
  

  if (sw->current_vertical_subpass == 0)
    initialize_row(sw, sw->lineno, xlength);
  
  for (j = 0; j < sw->ncolors; j++)
    {
      if (cols[j])
	{
	  const unsigned char *in;
	  if (sw->bitwidth == 2)
	    {
	      escp2_fold(cols[j], length, fold_buf);
	      in = fold_buf;
	    }
	  else
	    in = cols[j];
	  if (h_passes > 1)
	    {
	      switch (sw->horizontal_weave)
		{
		case 2:
		  escp2_unpack_2(length, sw->bitwidth, in, s[0], s[1]);
		  break;
		case 4:
		  escp2_unpack_4(length, sw->bitwidth, in,
				 s[0], s[1], s[2], s[3]);
		  break;
		}
	      switch (sw->vertical_subpasses)
		{
		case 4:
		  switch (sw->horizontal_weave)
		    {
		    case 1:
		      escp2_split_4(length, sw->bitwidth, in,
				    s[0], s[1], s[2], s[3]);
		      break;
		    case 2:
		      escp2_split_4(length, sw->bitwidth, s[0],
				    s[0], s[2], s[4], s[6]);
		      escp2_split_4(length, sw->bitwidth, s[1],
				    s[1], s[3], s[5], s[7]);
		      break;
		    }
		  break;
		case 2:
		  switch (sw->horizontal_weave)
		    {
		    case 1:
		      escp2_split_2(xlength, sw->bitwidth, in, s[0], s[1]);
		      break;
		    case 2:
		      escp2_split_2(xlength, sw->bitwidth, s[0], s[0], s[2]);
		      escp2_split_2(xlength, sw->bitwidth, s[1], s[1], s[3]);
		      break;
		    case 4:
		      escp2_split_2(xlength, sw->bitwidth, s[0], s[0], s[4]);
		      escp2_split_2(xlength, sw->bitwidth, s[1], s[1], s[5]);
		      escp2_split_2(xlength, sw->bitwidth, s[2], s[2], s[6]);
		      escp2_split_2(xlength, sw->bitwidth, s[3], s[3], s[7]);
		      break;
		    }
		  break;
		  /* case 1 is taken care of because the various unpack */
		  /* functions will do the trick themselves */
		}
	      for (i = 0; i < h_passes; i++)
		{
		  int k = sw->current_vertical_subpass * h_passes;
		  setactive = escp2_pack(s[i], sw->bitwidth * xlength,
					 comp_buf, &comp_ptr);
		  add_to_row(sw, sw->lineno, comp_buf, comp_ptr - comp_buf,
			     colors[j], densities[j], k + i, setactive);
		}
	    }
	  else
	    {
	      int k = sw->current_vertical_subpass * h_passes;
	      setactive = escp2_pack(in, length * sw->bitwidth,
				     comp_buf, &comp_ptr);
	      add_to_row(sw, sw->lineno, comp_buf, comp_ptr - comp_buf,
			 colors[j], densities[j], k, setactive);
	    }
	}
    }
  sw->current_vertical_subpass++;
  if (sw->current_vertical_subpass >= sw->vertical_oversample)
    {
      finalize_row(sw, sw->lineno, model, width, offset, ydpi, xdpi, prn);
      sw->lineno++;
      sw->current_vertical_subpass = 0;
    }
}

#endif
