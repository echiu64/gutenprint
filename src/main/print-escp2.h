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
 */

/*
 * Printer capabilities.
 *
 * Various classes of printer capabilities are represented by bitmasks.
 */

typedef unsigned long model_cap_t;
typedef unsigned long model_featureset_t;


/*
 * For each printer, we can select from a variety of dot sizes.
 * For single dot size printers, the available sizes are usually 0,
 * which is the "default", and some subset of 1-4.  For simple variable
 * dot size printers (with only one kind of variable dot size), the
 * variable dot size is specified as 0x10.  For newer printers, there
 * is a choice of variable dot sizes available, 0x10, 0x11, and 0x12 in
 * order of increasing size.
 *
 * Normally, we want to specify the smallest dot size that lets us achieve
 * a density of less than .8 or thereabouts (above that we start to get
 * some dither artifacts).  This needs to be tested for each printer and
 * resolution.
 *
 * An entry of -1 in a slot means that this resolution is not available.
 */

typedef int escp2_dot_size_t[13];

/*
 * Choose the number of bits to use at each resolution.
 */

typedef int escp2_bits_t[13];

/*
 * Choose the base resolution to use at each resolution.
 */

typedef int escp2_base_resolutions_t[13];

/*
 * Specify the base density for each available resolution.
 * This obviously depends upon the dot size.
 */

typedef double escp2_densities_t[13];

/*
 * Definition of the multi-level inks available to a given printer.
 * Each printer may use a different kind of ink droplet for variable
 * and single drop size for each supported horizontal resolution and
 * type of ink (4 or 6 color).
 *
 * Recall that 6 color ink is treated as simply another kind of
 * multi-level ink, but the driver offers the user a choice of 4 and
 * 6 color ink, so we need to define appropriate inksets for both
 * kinds of ink.
 *
 * Stuff like the MIS 4 and 6 "color" monochrome inks doesn't fit into
 * this model very nicely, so we'll either have to special case it
 * or find some way of handling it in here.
 */

#define RES_120_M	 0
#define RES_120		 1
#define RES_180_M	 2
#define RES_180		 3
#define RES_360_M	 4
#define RES_360		 5
#define RES_720_360_M	 6
#define RES_720_360	 7
#define RES_720_M	 8
#define RES_720		 9
#define RES_1440_720_M	 10
#define RES_1440_720	 11
#define RES_1440_1440_M	 12
#define RES_1440_1440	 13
#define RES_2880_720_M	 14
#define RES_2880_720	 15
#define RES_2880_1440_M	 16
#define RES_2880_1440	 17
#define RES_N		 18

typedef struct escp2_variable_ink
{
  const stp_simple_dither_range_t *range;
  int count;
  double density;
} escp2_variable_ink_t;

typedef const escp2_variable_ink_t *escp2_variable_inkset_t[NCOLORS];

typedef const escp2_variable_inkset_t *escp2_variable_inklist_t[][RES_N / 2];

typedef struct
{
  const char *name;
  const char *text;
  int paper_feed_sequence;
  int platen_gap;
  double base_density;
  double k_lower_scale;
  double k_upper;
  double cyan;
  double magenta;
  double yellow;
  double p_cyan;
  double p_magenta;
  double p_yellow;
  double saturation;
  double gamma;
  int feed_adjustment;
  int vacuum_intensity;
  int paper_thickness;
  const double *hue_adjustment;
  const double *lum_adjustment;
  const double *sat_adjustment;
} paper_t;

typedef struct
{
  int paper_count;
  const paper_t *papers;
} paperlist_t;


#define MODEL_INIT_MASK		0xful /* Is a special init sequence */
#define MODEL_INIT_STANDARD	0x0ul /* required for this printer, and if */
#define MODEL_INIT_NEW		0x1ul /* so, what */

#define MODEL_HASBLACK_MASK	0x10ul /* Can this printer print black ink */
#define MODEL_HASBLACK_YES	0x00ul /* when it is also printing color? */
#define MODEL_HASBLACK_NO	0x10ul

#define MODEL_COLOR_MASK	0x60ul /* Is this a 6-color printer? */
#define MODEL_COLOR_4		0x00ul
#define MODEL_COLOR_6		0x20ul
#define MODEL_COLOR_7		0x40ul

#define MODEL_GRAYMODE_MASK	0x80ul /* Does this printer support special */
#define MODEL_GRAYMODE_NO	0x00ul /* fast black printing? */
#define MODEL_GRAYMODE_YES	0x80ul

#define MODEL_720DPI_MODE_MASK	0x300ul /* Does this printer require old */
#define MODEL_720DPI_DEFAULT	0x000ul /* or new setting for printing */
#define MODEL_720DPI_600	0x100ul /* 720 dpi?  Only matters for */
					 /* single dot size printers */

#define MODEL_VARIABLE_DOT_MASK	0xc00ul /* Does this printer support var */
#define MODEL_VARIABLE_NORMAL	0x000ul /* dot size printing? The newest */
#define MODEL_VARIABLE_4	0x400ul /* printers support multiple modes */
#define MODEL_VARIABLE_MULTI	0x800ul /* of variable dot sizes. */

#define MODEL_COMMAND_MASK	0xf000ul /* What general command set does */
#define MODEL_COMMAND_1998	0x0000ul
#define MODEL_COMMAND_1999	0x1000ul /* The 1999 series printers */
#define MODEL_COMMAND_2000	0x2000ul /* The 2000 series printers */
#define MODEL_COMMAND_PRO	0x3000ul /* Stylus Pro printers */

#define MODEL_INK_MASK		0x10000ul /* Does this printer support */
#define MODEL_INK_NORMAL	0x00000ul /* different types of inks? */
#define MODEL_INK_SELECTABLE	0x10000ul /* Only the Stylus Pro's do */

#define MODEL_ROLLFEED_MASK	0x20000ul /* Does this printer support */
#define MODEL_ROLLFEED_NO	0x00000ul /* a roll feed? */
#define MODEL_ROLLFEED_YES	0x20000ul

#define MODEL_XZEROMARGIN_MASK	0x40000ul /* Does this printer support */
#define MODEL_XZEROMARGIN_NO	0x00000ul /* zero margin mode? */
#define MODEL_XZEROMARGIN_YES	0x40000ul /* (print to the edge of the paper) */

#define MODEL_YZEROMARGIN_MASK	0x80000ul /* Does this printer support */
#define MODEL_YZEROMARGIN_NO	0x00000ul /* zero margin mode? */
#define MODEL_YZEROMARGIN_YES	0x80000ul /* (print to the edge of the paper) */

#define MODEL_MICROWEAVE_MASK		0x700000ul
#define MODEL_MICROWEAVE_NO		0x000000ul
#define MODEL_MICROWEAVE_YES		0x100000ul
#define MODEL_MICROWEAVE_ENHANCED	0x200000ul

#define MODEL_VACUUM_MASK		0x800000ul
#define MODEL_VACUUM_NO			0x000000ul
#define MODEL_VACUUM_YES		0x800000ul

#define MODEL_MICROWEAVE_EXCEPTION_MASK   0x3000000ul
#define MODEL_MICROWEAVE_EXCEPTION_NORMAL 0x0000000ul
#define MODEL_MICROWEAVE_EXCEPTION_360    0x1000000ul
#define MODEL_MICROWEAVE_EXCEPTION_BLACK  0x2000000ul

#define MODEL_DEINITIALIZE_JE_MASK      0x4000000ul
#define MODEL_DEINITIALIZE_JE_NO        0x0000000ul
#define MODEL_DEINITIALIZE_JE_YES       0x4000000ul

#define MODEL_INIT			(0)
#define MODEL_HASBLACK			(1)
#define MODEL_COLOR			(2)
#define MODEL_GRAYMODE			(3)
#define MODEL_720DPI_MODE		(4)
#define MODEL_VARIABLE_DOT		(5)
#define MODEL_COMMAND			(6)
#define MODEL_INK			(7)
#define MODEL_ROLLFEED			(8)
#define MODEL_XZEROMARGIN		(9)
#define MODEL_YZEROMARGIN		(10)
#define MODEL_MICROWEAVE		(11)
#define MODEL_VACUUM			(12)
#define MODEL_MICROWEAVE_EXCEPTION	(13)
#define MODEL_DEINITIALIZE_JE           (14)
#define MODEL_LIMIT			(15)

typedef struct
{
  const char *attr_name;
  int shift;
  int bits;
} escp2_printer_attr_t;

typedef struct
{
  const char *name;
  const char *text;
  int hres;
  int vres;
  int softweave;
  int microweave;
  int vertical_passes;
  int vertical_oversample;
  int unidirectional;
  int vertical_undersample;
  int vertical_denominator;
  int resid;
} res_t;

typedef struct
{
  int color;
  int density;
} physical_subchannel_t;

typedef struct
{
  const physical_subchannel_t *channels;
  int n_subchannels;
} ink_channel_t;

typedef struct
{
  const char *name;
  const char *text;
  int is_color;
  int inkset;
  double k_lower;
  double k_upper;
  const ink_channel_t *channels[NCOLORS];
} escp2_inkname_t;

typedef struct
{
  const escp2_inkname_t **inknames;
  int n_inks;
} inklist_t;

typedef struct escp2_printer
{
  model_cap_t	flags;		/* Bitmask of flags, see below */
/*****************************************************************************/
  int		nozzles;	/* Number of nozzles per color */
  int		min_nozzles;	/* Minimum number of nozzles per color */
  int		nozzle_separation; /* Separation between rows, in 1/360" */
  int		black_nozzles;	/* Number of black nozzles (may be extra) */
  int		min_black_nozzles;	/* # of black nozzles (may be extra) */
  int		black_nozzle_separation; /* Separation between rows */
/*****************************************************************************/
  int		base_separation; /* Basic unit of row separation */
  int		base_resolution; /* Base hardware line spacing (above this */
				/* always requires multiple passes) */
  int		enhanced_resolution;/* Above this we use the */
				    /* enhanced_xres rather than xres */
  int		resolution_scale;   /* Scaling factor for ESC(D command */
  int		max_black_resolution; /* Above this resolution, we */
				      /* must use color parameters */
				      /* rather than (faster) black */
				      /* only parameters*/
  int		max_hres;
  int		max_vres;
  int		min_hres;
  int		min_vres;
/*****************************************************************************/
  int		max_paper_width; /* Maximum paper width, in points */
  int		max_paper_height; /* Maximum paper height, in points */
  int		min_paper_width; /* Maximum paper width, in points */
  int		min_paper_height; /* Maximum paper height, in points */
/*****************************************************************************/
				/* SHEET FED: */
				/* Softweave: */
  int		left_margin;	/* Left margin, points */
  int		right_margin;	/* Right margin, points */
  int		top_margin;	/* Absolute top margin, points */
  int		bottom_margin;	/* Absolute bottom margin, points */
				/* "Micro"weave: */
  int		m_left_margin;	/* Left margin, points */
  int		m_right_margin;	/* Right margin, points */
  int		m_top_margin;	/* Absolute top margin, points */
  int		m_bottom_margin;	/* Absolute bottom margin, points */
				/* ROLL FEED: */
				/* Softweave: */
  int		roll_left_margin;	/* Left margin, points */
  int		roll_right_margin;	/* Right margin, points */
  int		roll_top_margin;	/* Absolute top margin, points */
  int		roll_bottom_margin;	/* Absolute bottom margin, points */
				/* "Micro"weave: */
  int		m_roll_left_margin;	/* Left margin, points */
  int		m_roll_right_margin;	/* Right margin, points */
  int		m_roll_top_margin;	/* Absolute top margin, points */
  int		m_roll_bottom_margin;	/* Absolute bottom margin, points */
/*****************************************************************************/
  int		extra_feed;	/* Extra distance the paper can be spaced */
				/* beyond the bottom margin, in 1/360". */
				/* (maximum useful value is */
				/* nozzles * nozzle_separation) */
  int		separation_rows; /* Some printers require funky spacing */
				/* arguments in microweave mode. */
  int		pseudo_separation_rows;/* Some printers require funky */
				/* spacing arguments in softweave mode */

  int           zero_margin_offset;   /* Offset to use to achieve */
				      /* zero-margin printing */
		 /* The stylus 480 and 580 have an unusual arrangement of
				  color jets that need special handling */
  const int *head_offset;
  int		initial_vertical_offset;
  int		black_initial_vertical_offset;

/*****************************************************************************/
  const int *dot_sizes;		/* Vector of dot sizes for resolutions */
  const double *densities;	/* List of densities for each printer */
  const escp2_variable_inklist_t *inks; /* Choices of inks for this printer */
/*****************************************************************************/
  const double *lum_adjustment;
  const double *hue_adjustment;
  const double *sat_adjustment;
/*****************************************************************************/
  const paperlist_t *paperlist;
  const res_t *reslist;
  const inklist_t *inklist;
/*****************************************************************************/
  const int *bits;
  const int *base_resolutions;
} escp2_stp_printer_t;

extern const escp2_stp_printer_t stp_escp2_model_capabilities[];
