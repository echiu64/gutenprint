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

#ifndef GIMP_PRINT_INTERNAL_ESCP2_H
#define GIMP_PRINT_INTERNAL_ESCP2_H


#define PHYSICAL_CHANNEL_LIMIT 7

/*
 * Printer capabilities.
 *
 * Various classes of printer capabilities are represented by bitmasks.
 */

typedef unsigned long model_cap_t;
typedef unsigned long model_featureset_t;


#define RES_LOW		 0
#define RES_360		 1
#define RES_720_360	 2
#define RES_720		 3
#define RES_1440_720	 4
#define RES_2880_720	 5
#define RES_2880_1440	 6
#define RES_2880_2880	 7
#define RES_N		 8

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

typedef int escp2_dot_size_t[RES_N];

/*
 * Choose the number of bits to use at each resolution.
 */

typedef int escp2_bits_t[RES_N];

/*
 * Choose the base resolution to use at each resolution.
 */

typedef int escp2_base_resolutions_t[RES_N];

/*
 * Specify the base density for each available resolution.
 * This obviously depends upon the dot size.
 */

typedef double escp2_densities_t[RES_N];

typedef struct escp2_variable_ink
{
  double darkness;
  const stpi_shade_t *shades;
  int numshades;
} escp2_variable_ink_t;

typedef const escp2_variable_ink_t *escp2_variable_inkset_t[PHYSICAL_CHANNEL_LIMIT];

typedef const escp2_variable_inkset_t *escp2_variable_inklist_t[][RES_N];

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
  const char *hue_adjustment;
  const char *lum_adjustment;
  const char *sat_adjustment;
  const char *preferred_ink_type;
} paper_t;

typedef struct
{
  int paper_count;
  const paper_t *papers;
} paperlist_t;


#define MODEL_COMMAND_MASK	0xful /* What general command set does */
#define MODEL_COMMAND_1998	0x0ul
#define MODEL_COMMAND_1999	0x1ul /* The 1999 series printers */
#define MODEL_COMMAND_2000	0x2ul /* The 2000 series printers */
#define MODEL_COMMAND_PRO	0x3ul /* Stylus Pro printers */

#define MODEL_XZEROMARGIN_MASK	0x10ul /* Does this printer support */
#define MODEL_XZEROMARGIN_NO	0x00ul /* zero margin mode? */
#define MODEL_XZEROMARGIN_YES	0x10ul /* (print to edge of the paper) */

#define MODEL_ROLLFEED_MASK	0x20ul /* Does this printer support */
#define MODEL_ROLLFEED_NO	0x00ul /* a roll feed? */
#define MODEL_ROLLFEED_YES	0x20ul

#define MODEL_VARIABLE_DOT_MASK	0x40ul /* Does this printer support var */
#define MODEL_VARIABLE_NO	0x00ul /* dot size printing? The newest */
#define MODEL_VARIABLE_YES	0x40ul /* printers support multiple modes */

#define MODEL_GRAYMODE_MASK	0x80ul /* Does this printer support special */
#define MODEL_GRAYMODE_NO	0x00ul /* fast black printing? */
#define MODEL_GRAYMODE_YES	0x80ul

#define MODEL_VACUUM_MASK	0x100ul
#define MODEL_VACUUM_NO		0x000ul
#define MODEL_VACUUM_YES	0x100ul

#define MODEL_FAST_360_MASK	0x200ul
#define MODEL_FAST_360_NO	0x000ul
#define MODEL_FAST_360_YES	0x200ul

typedef enum
{
  MODEL_COMMAND,
  MODEL_XZEROMARGIN,
  MODEL_ROLLFEED,
  MODEL_VARIABLE_DOT,
  MODEL_GRAYMODE,
  MODEL_VACUUM,
  MODEL_FAST_360,
  MODEL_LIMIT
} escp2_model_option_t;

typedef struct
{
  const char *attr_name;
  int bit_shift;
  int bit_width;
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
  int vertical_undersample;
  int vertical_denominator;
  int resid;
} res_t;

typedef struct
{
  int color;
  int subchannel;
  int head_offset;
  const char *channel_density;
  const char *subchannel_scale;
} physical_subchannel_t;

typedef struct
{
  const physical_subchannel_t *subchannels;
  int n_subchannels;
} ink_channel_t;

typedef enum
{
  INKSET_CMYK           = 0,
  INKSET_CcMmYK         = 1,
  INKSET_CcMmYyK        = 2,
  INKSET_CcMmYKk        = 3,
  INKSET_PIEZO_QUADTONE = 4,
  INKSET_EXTENDED	= 5
} inkset_id_t;

typedef struct
{
  const char *name;
  const char *text;
  int is_color;
  inkset_id_t inkset;
  double k_lower;
  double k_upper;
  int channel_limit;
  const char *lum_adjustment;
  const char *hue_adjustment;
  const char *sat_adjustment;
  const ink_channel_t *channels[PHYSICAL_CHANNEL_LIMIT];
} escp2_inkname_t;

typedef struct
{
  const escp2_inkname_t *const *inknames;
  size_t n_inks;
} inklist_t;

#define ROLL_FEED_CUT_ALL (1)
#define ROLL_FEED_CUT_LAST (2)

typedef struct
{
  const char *name;
  const char *text;
  int is_roll_feed;
  unsigned roll_feed_cut_flags;
  const stp_raw_t init_sequence;
  const stp_raw_t deinit_sequence;
} input_slot_t;

typedef struct
{
  const input_slot_t *slots;
  size_t n_input_slots;
} input_slot_list_t;

typedef struct escp2_printer
{
  model_cap_t	flags;		/* Bitmask of flags, see above */
/*****************************************************************************/
  /* Basic head configuration */
  int		nozzles;	/* Number of nozzles per color */
  int		min_nozzles;	/* Minimum number of nozzles per color */
  int		nozzle_separation; /* Separation between rows, in 1/360" */
  int		black_nozzles;	/* Number of black nozzles (may be extra) */
  int		min_black_nozzles;	/* # of black nozzles (may be extra) */
  int		black_nozzle_separation; /* Separation between rows */
  int		fast_nozzles;	/* Number of fast nozzles */
  int		min_fast_nozzles;	/* # of fast nozzles (may be extra) */
  int		fast_nozzle_separation; /* Separation between rows */
  int		physical_channels; /* Number of ink channels */
/*****************************************************************************/
  /* Print head resolution */
  int		base_separation; /* Basic unit of row separation */
  int		resolution_scale;   /* Scaling factor for ESC(D command */
  int		max_black_resolution; /* Above this resolution, we */
				      /* must use color parameters */
				      /* rather than (faster) black */
				      /* only parameters*/
  int		max_hres;
  int		max_vres;
  int		min_hres;
  int		min_vres;
  /* Miscellaneous printer-specific data */
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
  int		initial_vertical_offset;
  int		black_initial_vertical_offset;
  int		extra_720dpi_separation;
/*****************************************************************************/
  /* Paper size limits */
  int		max_paper_width; /* Maximum paper width, in points */
  int		max_paper_height; /* Maximum paper height, in points */
  int		min_paper_width; /* Maximum paper width, in points */
  int		min_paper_height; /* Maximum paper height, in points */
/*****************************************************************************/
  /* Borders */
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
  const int *dot_sizes;		/* Vector of dot sizes for resolutions */
  const double *densities;	/* List of densities for each printer */
/*****************************************************************************/
  const escp2_variable_inklist_t *inks; /* Choices of inks for this printer */
  const paperlist_t *paperlist;
/*****************************************************************************/
  const res_t *reslist;
  const inklist_t *inklist;
/*****************************************************************************/
  const int *bits;
  const int *base_resolutions;
  const input_slot_list_t *input_slots;
/*****************************************************************************/
  const stp_raw_t *preinit_sequence;
  const stp_raw_t *postinit_remote_sequence;
} stpi_escp2_printer_t;

extern const stpi_escp2_printer_t stpi_escp2_model_capabilities[];
extern const int stpi_escp2_model_limit;

extern const escp2_variable_inklist_t stpi_escp2_simple_inks;
extern const escp2_variable_inklist_t stpi_escp2_spro10000_inks;
extern const escp2_variable_inklist_t stpi_escp2_variable_2pl_inks;
extern const escp2_variable_inklist_t stpi_escp2_variable_3pl_inks;
extern const escp2_variable_inklist_t stpi_escp2_variable_3pl_pigment_inks;
extern const escp2_variable_inklist_t stpi_escp2_variable_4pl_inks;
extern const escp2_variable_inklist_t stpi_escp2_variable_4pl_pigment_inks;
extern const escp2_variable_inklist_t stpi_escp2_variable_680_4pl_inks;
extern const escp2_variable_inklist_t stpi_escp2_variable_6pl_inks;
extern const escp2_variable_inklist_t stpi_escp2_variable_pigment_inks;
extern const escp2_variable_inklist_t stpi_escp2_variable_x80_6pl_inks;

extern const inklist_t stpi_escp2_c80_inklist;
extern const inklist_t stpi_escp2_cmy_inklist;
extern const inklist_t stpi_escp2_photo7_inklist;
extern const inklist_t stpi_escp2_photo7_japan_inklist;
extern const inklist_t stpi_escp2_photo_inklist;
extern const inklist_t stpi_escp2_standard_inklist;
extern const inklist_t stpi_escp2_x80_inklist;

extern const paperlist_t stpi_escp2_c80_paper_list;
extern const paperlist_t stpi_escp2_sp780_paper_list;
extern const paperlist_t stpi_escp2_sp950_paper_list;
extern const paperlist_t stpi_escp2_standard_paper_list;

extern const res_t stpi_escp2_superfine_reslist[];
extern const res_t stpi_escp2_no_microweave_reslist[];
extern const res_t stpi_escp2_pro_reslist[];
extern const res_t stpi_escp2_sp5000_reslist[];
extern const res_t stpi_escp2_standard_reslist[];
extern const res_t stpi_escp2_720dpi_reslist[];
extern const res_t stpi_escp2_720dpi_soft_reslist[];
extern const res_t stpi_escp2_1440dpi_reslist[];
extern const res_t stpi_escp2_g3_reslist[];
extern const res_t stpi_escp2_sc500_reslist[];
extern const res_t stpi_escp2_sc640_reslist[];
extern const res_t stpi_escp2_sc660_reslist[];

typedef struct
{
  /* Basic print head parameters */
  int nozzles;			/* Number of nozzles */
  int min_nozzles;		/* Fewest nozzles we're allowed to use */
  int nozzle_separation;	/* Nozzle separation, in dots */
  int *head_offset;		/* Head offset (for C80-type printers) */
  int max_head_offset;		/* Largest head offset */
  int page_management_units;	/* Page management units (dpi) */
  int vertical_units;		/* Vertical units (dpi) */
  int horizontal_units;		/* Horizontal units (dpi) */
  int micro_units;		/* Micro-units for horizontal positioning */
  int unit_scale;		/* Scale factor for units */

  /* Ink parameters */
  int bitwidth;			/* Number of bits per ink drop */
  int drop_size;		/* ID of the drop size we're using */
  int ink_resid;		/* Array index for the drop set we're using */
  const escp2_inkname_t *inkname; /* Description of the ink set */
  int rescale_density;		/* Do we want to rescale the density? */

  /* Ink channels */
  int logical_channels;		/* Number of logical ink channels (e.g.CMYK) */
  int physical_channels;	/* Number of physical channels (e.g. CcMmYK) */
  int channels_in_use;		/* Number of channels we're using
				   FIXME merge with physical_channels! */
  unsigned char **cols;		/* Output dithered data */
  const physical_subchannel_t **channels; /* Description of each channel */

  /* Miscellaneous printer control */
  int use_black_parameters;	/* Can we use (faster) black head parameters */
  int use_fast_360;		/* Can we use fast 360 DPI 4 color mode */
  int advanced_command_set;	/* Uses one of the advanced command sets */
  int use_extended_commands;	/* Do we use the extended commands? */
  const input_slot_t *input_slot; /* Input slot description */
  const paper_t *paper_type;	/* Paper type */
  const stp_raw_t *init_sequence; /* Initialization sequence */
  const stp_raw_t *deinit_sequence; /* De-initialization sequence */
  model_featureset_t command_set; /* Which command set this printer supports */
  int variable_dots;		/* Print supports variable dot sizes */
  int has_vacuum;		/* Printer supports vacuum command */
  int has_graymode;		/* Printer supports fast grayscale mode */
  int base_separation;		/* Basic unit of separation */
  int resolution_scale;		/* Scale factor for ESC(D command */
  int printing_resolution;	/* Printing resolution for this resolution */
  int separation_rows;		/* Row separation scaling */
  int pseudo_separation_rows;	/* Special row separation for some printers */
  int extra_720dpi_separation;	/* Sepcial separation needed at 720 DPI */

  /* weave parameters */
  int horizontal_passes;	/* Number of horizontal passes required
				   to print a complete row */
  int physical_xdpi;		/* Horizontal distance between dots in pass */
  const res_t *res;		/* Description of the printing resolution */

  /* Page parameters */		/* Indexed from top left */
  int page_left;		/* Left edge of page (points) */
  int page_right;		/* Right edge of page (points) */
  int page_top;			/* Top edge of page (points) */
  int page_bottom;		/* Bottom edge of page (points) */
  int page_width;		/* Page width (points) */
  int page_height;		/* Page height (points) */
  int page_true_height;		/* Physical page height (points) */

  /* Image parameters */	/* Indexed from top left */
  int image_height;		/* Height of printed region (points) */
  int image_width;		/* Width of printed region (points) */
  int image_top;		/* First printed row (points) */
  int image_left;		/* Left edge of image (points) */
  int image_scaled_width;	/* Width of printed region (dots) */
  int image_scaled_height;	/* Height of printed region (dots) */
  int image_left_position;	/* Left dot position of image */

  /* Transitory state */
  int printed_something;	/* Have we actually printed anything? */
  int initial_vertical_offset;	/* Vertical offset for C80-type printers */
  int printing_initial_vertical_offset;	/* Vertical offset, for print cmd */
  int last_color;		/* Last color we printed */
  int last_pass_offset;		/* Starting row of last pass we printed */

} escp2_privdata_t;

extern void stpi_escp2_init_printer(stp_vars_t v);
extern void stpi_escp2_deinit_printer(stp_vars_t v);
extern void stpi_escp2_flush_pass(stp_vars_t v, int passno,
				  int vertical_subpass);

#ifdef TEST_UNCOMPRESSED
#define COMPRESSION (0)
#define FILLFUNC stpi_fill_uncompressed
#define COMPUTEFUNC stpi_compute_uncompressed_linewidth
#define PACKFUNC stpi_pack_uncompressed
#else
#define COMPRESSION (1)
#define FILLFUNC stpi_fill_tiff
#define COMPUTEFUNC stpi_compute_tiff_linewidth
#define PACKFUNC stpi_pack_tiff
#endif

#endif /* GIMP_PRINT_INTERNAL_ESCP2_H */
/*
 * End of "$Id$".
 */
