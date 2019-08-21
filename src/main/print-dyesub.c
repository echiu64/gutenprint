/*
 *
 *   Print plug-in DyeSub driver (formerly Olympus driver) for Gutenprint
 *
 *   Copyright 2003-2006 Michael Mraka (Michael.Mraka@linux.cz)
 *
 *   Copyright 2007-2019 Solomon Peachy (pizza@shaftnet.org)
 *
 *   The plug-in is based on the code of the RAW plugin for the GIMP of
 *   Michael Sweet (mike@easysw.com) and Robert Krawitz (rlk@alum.mit.edu)
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <time.h>  /* For strftime() and localtime_r() */
#ifdef __GNUC__
#define inline __inline__
#endif

#define S6145_YMC  /* Generate YMC data for S6145 family */

#define DYESUB_FEATURE_NONE		 0x00000000
#define DYESUB_FEATURE_FULL_WIDTH	 0x00000001
#define DYESUB_FEATURE_FULL_HEIGHT	 0x00000002
#define DYESUB_FEATURE_BLOCK_ALIGN	 0x00000004
#define DYESUB_FEATURE_BORDERLESS	 0x00000008
#define DYESUB_FEATURE_WHITE_BORDER	 0x00000010
#define DYESUB_FEATURE_PLANE_INTERLACE	 0x00000020
#define DYESUB_FEATURE_PLANE_LEFTTORIGHT 0x00000040
#define DYESUB_FEATURE_ROW_INTERLACE	 0x00000080
#define DYESUB_FEATURE_DUPLEX            0x00000800
#define DYESUB_FEATURE_MONOCHROME        0x00001000
#define DYESUB_FEATURE_NATIVECOPIES      0x00002000

#define DYESUB_PORTRAIT  0
#define DYESUB_LANDSCAPE 1

#define OP_JOB_START 1
#define OP_JOB_PRINT 2
#define OP_JOB_END   4

#ifndef MIN
#  define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif /* !MIN */
#ifndef MAX
#  define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif /* !MAX */
#define PX(pt,dpi)	(int)(((stp_dimension_t)(pt) * (stp_resolution_t)(dpi) / (stp_resolution_t)72) + 0.5f)
#define PT(px,dpi)	((stp_resolution_t)(px) * (stp_resolution_t)72 / (stp_dimension_t)(dpi))
#define PT1		PT
#define LIST(list_t, list_name, items_t, items_name) \
	static const list_t list_name = \
	{ \
	  items_name, sizeof(items_name) / sizeof(items_t) \
	}

#define MAX_INK_CHANNELS	3
#define SIZE_THRESHOLD		6

/*
 * Random implementation from POSIX.1-2001 to yield reproducible results.
 */
typedef struct
{
  const char *output_type;
  int output_channels;
  const char *name;
  const char *channel_order;
} ink_t;

typedef struct {
  const ink_t *item;
  size_t n_items;
} ink_list_t;

typedef struct {
  const char* name;
  stp_resolution_t w_dpi;
  stp_resolution_t h_dpi;
} dyesub_resolution_t;

typedef struct {
  const dyesub_resolution_t *item;
  size_t n_items;
} dyesub_resolution_list_t;

typedef struct {
  stp_papersize_t psize;
  int print_mode;
} dyesub_pagesize_t;

#define DEFINE_PAPER(__n, __t, __w, __h, __bl, __br, __bt, __bb, __pm)	\
	{								\
	  .psize = {							\
	    .name = __n,						\
	    .text = N_(__t),						\
	    .width = __w,						\
	    .height = __h,						\
	    .top = __bt,						\
	    .left = __bl,						\
	    .bottom = __bb,						\
	    .right = __br,						\
	    .paper_unit = PAPERSIZE_ENGLISH_STANDARD,			\
	    .paper_size_type = PAPERSIZE_TYPE_STANDARD,			\
	  },								\
	    .print_mode = __pm,						\
	    }

#define DEFINE_PAPER_SIMPLE(__n, __t, __w, __h, __pm)	\
	DEFINE_PAPER(__n, __t, __w, __h, 0, 0, 0, 0, __pm)

typedef struct {
  const dyesub_pagesize_t *item;
  size_t n_items;
} dyesub_pagesize_list_t;

typedef struct {
  const char* res_name;
  const char* pagesize_name;
  int width_px;
  int height_px;
} dyesub_printsize_t;

typedef struct {
  const dyesub_printsize_t *item;
  size_t n_items;
} dyesub_printsize_list_t;

typedef struct {
  const char *name;
  const char *text;
  const stp_raw_t seq;
} overcoat_t;

typedef struct {
  const overcoat_t *item;
  size_t n_items;
} overcoat_list_t;

typedef struct {
  const char* name;
  const char* text;
  const stp_raw_t seq;
} dyesub_media_t;

typedef struct {
  const dyesub_media_t *item;
  size_t n_items;
} dyesub_media_list_t;

typedef struct {
  const char* name;
  const char *text;
} dyesub_stringitem_t;

typedef struct {
  const dyesub_stringitem_t *item;
  size_t n_items;
} dyesub_stringlist_t;

/* Private data for some of the major dyesub driver families */
typedef struct
{
  int multicut;
  int nocutwaste;
  const char *print_speed; /* DS820 only */
} dnp_privdata_t;

typedef struct
{
  int quality;
  int finedeep;
  int contrast;
} mitsu9550_privdata_t;

typedef struct
{
  int gamma;
  int unk_gg;
  /* below are up-d897 only */
  int dark;
  int light;
  int advance;
  int sharp;
} sonymd_privdata_t;

typedef struct
{
  int quality;
  int overcoat_offset;
  int use_lut;
  int sharpen;
  int delay;
  int deck;
} mitsu70x_privdata_t;

typedef struct
{
  int sharpen;
} kodak9810_privdata_t;

typedef struct
{
  int sharpen;
  int matte_intensity;
} kodak8500_privdata_t;

typedef struct
{
  int matte_intensity;
  int dust_removal;
} shinko1245_privdata_t;

typedef struct
{
  int clear_mem;
  int cont_print;
  int gamma;
  int flags;
  int comment;
  int contrast;
  int sharpen;
  int brightness;
  char userlut[34];
  char usercomment[40];
  char commentbuf[19];  /* With one extra byte for null termination */
} mitsu_p95d_privdata_t;

typedef struct
{
  int resin_k;
  int reject;
  int colorsure;
  int holokote;
  int holokote_custom;
  int holopatch;
  int overcoat;
  int overcoat_dpx;
  const char *overcoat_hole; /* XXX TODO: add custom option? */
  const char *overcoat_hole_dpx; /* XXX TODO: add custom option? */
  int align_start;
  int align_end;
  int power_color;
  int power_resin;
  int power_overcoat;
  int gamma;
  int duplex;
  char mag1[79];  /* Mag stripe row 1, 78 alphanumeric */
  char mag2[40];  /* Mag stripe row 2, 39 numeric */
  char mag3[107]; /* Mag stripe row 3, 106 numeric */
  int mag_coer;   /* 1 = high, 0 = low */
} magicard_privdata_t;

/* Private data for dyesub driver as a whole */
typedef struct
{
  stp_resolution_t w_dpi, h_dpi;
  stp_dimension_t w_size, h_size;
  char plane;
  int block_min_w, block_min_h;
  int block_max_w, block_max_h;
  const char* pagesize;
  const overcoat_t* overcoat;
  const dyesub_media_t* media;
  const char* slot;
  int print_mode;
  const char* duplex_mode;
  int page_number;
  int copies;
  int horiz_offset;
  union {
   dnp_privdata_t dnp;
   mitsu9550_privdata_t m9550;
   mitsu70x_privdata_t m70x;
   kodak9810_privdata_t k9810;
   kodak8500_privdata_t k8500;
   shinko1245_privdata_t s1245;
   mitsu_p95d_privdata_t m95d;
   magicard_privdata_t magicard;
   sonymd_privdata_t sonymd;
  } privdata;
} dyesub_privdata_t;

typedef struct {
  int out_channels;
  int ink_channels;
  const char *ink_order;
  int byteswap;
  int plane_interlacing;
  int row_interlacing;
  unsigned char empty_byte[MAX_INK_CHANNELS];  /* one for each color plane */
  unsigned short **image_data;
  int outh_px, outw_px, outt_px, outb_px, outl_px, outr_px;
  int imgh_px, imgw_px;
  int prnh_px, prnw_px, prnt_px, prnb_px, prnl_px, prnr_px;
  int print_mode;	/* portrait or landscape */
  int image_rows;
  int plane_lefttoright;
} dyesub_print_vars_t;

typedef struct /* printer specific parameters */
{
  int model;		/* printer model number from printers.xml*/
  const ink_list_t *inks;
  const dyesub_resolution_list_t *resolution;
  const dyesub_pagesize_list_t *pages;
  const dyesub_printsize_list_t *printsize;
  int block_size;  /* Really # of rows in a block */
  int features;
  void (*printer_init_func)(stp_vars_t *);
  void (*printer_end_func)(stp_vars_t *);
  void (*plane_init_func)(stp_vars_t *);
  void (*plane_end_func)(stp_vars_t *);
  void (*block_init_func)(stp_vars_t *);
  void (*block_end_func)(stp_vars_t *);
  void (*adjust_curves)(stp_vars_t *);
  const overcoat_list_t *overcoat;
  const dyesub_media_list_t *media;
  void (*job_start_func)(stp_vars_t *);
  void (*job_end_func)(stp_vars_t *);
  const stp_parameter_t *parameters;
  int parameter_count;
  int (*load_parameters)(const stp_vars_t *, const char *name, stp_parameter_t *);
  int (*parse_parameters)(stp_vars_t *);
} dyesub_cap_t;


static int dyesub_feature(const dyesub_cap_t *caps, int feature);
static const dyesub_cap_t* dyesub_get_model_capabilities(const stp_vars_t *v, int model);
static const overcoat_t* dyesub_get_overcoat_pattern(stp_vars_t *v);
static const dyesub_media_t* dyesub_get_mediatype(stp_vars_t *v);
static void  dyesub_nputc(stp_vars_t *v, char byte, int count);
static void  dyesub_adjust_curve(stp_vars_t *v,
				 const char *color_adj,
				 const char *color_curve);

static dyesub_privdata_t *
get_privdata(stp_vars_t *v)
{
  return (dyesub_privdata_t *) stp_get_component_data(v, "Driver");
}

static const ink_t cmy_inks[] =
{
  { "CMY", 3, "CMY", "\1\2\3" },
};

LIST(ink_list_t, cmy_ink_list, ink_t, cmy_inks);

static const ink_t ymc_inks[] =
{
  { "CMY", 3, "CMY", "\3\2\1" },
};

LIST(ink_list_t, ymc_ink_list, ink_t, ymc_inks);

static const ink_t rgb_inks[] =
{
  { "RGB", 3, "RGB", "\1\2\3" },
};

LIST(ink_list_t, rgb_ink_list, ink_t, rgb_inks);

static const ink_t bgr_inks[] =
{
  { "RGB", 3, "RGB", "\3\2\1" },
};

LIST(ink_list_t, bgr_ink_list, ink_t, bgr_inks);

static const ink_t w_inks[] =
{
  { "Whitescale", 1, "BW", "\1" },
};

LIST(ink_list_t, w_ink_list, ink_t, w_inks);

/* Olympus P-10 */
static const dyesub_resolution_t res_310dpi[] =
{
  { "310x310", 310, 310},
};

LIST(dyesub_resolution_list_t, res_310dpi_list, dyesub_resolution_t, res_310dpi);

static const dyesub_pagesize_t p10_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1280,310), PT1(1848,310), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1144,310), PT1(1591,310), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, p10_page_list, dyesub_pagesize_t, p10_page);

static const dyesub_printsize_t p10_printsize[] =
{
  { "310x310", "w288h432", 1280, 1848},
  { "310x310", "B7",  1144,  1591},
};

LIST(dyesub_printsize_list_t, p10_printsize_list, dyesub_printsize_t, p10_printsize);

static void p10_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("\033R\033M\033S\2\033N\1\033D\1\033Y", 1, 15, v);
  stp_write_raw(&(pd->overcoat->seq), v); /* overcoat */
  stp_zfwrite("\033Z\0", 1, 3, v);
}

static void p10_printer_end_func(stp_vars_t *v)
{
  stp_zfwrite("\033P", 1, 2, v);
}

static void p10_block_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zprintf(v, "\033T%c", pd->plane);
  stp_put16_le(pd->block_min_w, v);
  stp_put16_le(pd->block_min_h, v);
  stp_put16_le(pd->block_max_w + 1, v);
  stp_put16_le(pd->block_max_h + 1, v);
}

static const overcoat_t p10_overcoat[] =
{
  {"Coated",  N_("Coated"),  {1, "\x00"}},
  {"None",    N_("None"),    {1, "\x02"}},
};

LIST(overcoat_list_t, p10_overcoat_list, overcoat_t, p10_overcoat);


/* Olympus P-200 series */
static const dyesub_resolution_t res_320dpi[] =
{
  { "320x320", 320, 320},
};

LIST(dyesub_resolution_list_t, res_320dpi_list, dyesub_resolution_t, res_320dpi);

static const dyesub_pagesize_t p200_page[] =
{
  DEFINE_PAPER( "ISOB7", "80x125mm", PT(960,320), PT(1280,320), 16, 17, 33, 33, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, p200_page_list, dyesub_pagesize_t, p200_page);

static const dyesub_printsize_t p200_printsize[] =
{
  { "320x320", "ISOB7", 960, 1280},
};

LIST(dyesub_printsize_list_t, p200_printsize_list, dyesub_printsize_t, p200_printsize);

static void p200_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("S000001\0S010001\1", 1, 16, v);
}

static void p200_plane_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zprintf(v, "P0%d9999", 3 - pd->plane+1 );
  stp_put32_be(pd->w_size * pd->h_size, v);
}

static void p200_printer_end_func(stp_vars_t *v)
{
  stp_zprintf(v, "P000001\1");
}

static const char p200_adj_any[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"33\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.000000 0.039216 0.078431 0.117647 0.152941 0.192157 0.231373 0.266667\n"
  "0.301961 0.341176 0.376471 0.411765 0.447059 0.482353 0.513725 0.549020\n"
  "0.580392 0.615686 0.647059 0.678431 0.709804 0.741176 0.768627 0.796078\n"
  "0.827451 0.854902 0.878431 0.905882 0.929412 0.949020 0.972549 0.988235\n"
  "1.000000\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static void p200_adjust_curves(stp_vars_t *v)
{
  dyesub_adjust_curve(v, p200_adj_any, "CyanCurve");
  dyesub_adjust_curve(v, p200_adj_any, "MagentaCurve");
  dyesub_adjust_curve(v, p200_adj_any, "YellowCurve");
}

/* Olympus P-300 series */
static const dyesub_resolution_t p300_res[] =
{
  { "306x306", 306, 306},
  { "153x153", 153, 153},
};

LIST(dyesub_resolution_list_t, p300_res_list, dyesub_resolution_t, p300_res);

static const dyesub_pagesize_t p300_page[] =
{
  DEFINE_PAPER( "A6", "A6", PT1(1024,306), PT1(1376,306), 28, 28, 48, 48, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, p300_page_list, dyesub_pagesize_t, p300_page);

static const dyesub_printsize_t p300_printsize[] =
{
  { "306x306", "A6", 1024, 1376},
  { "153x153", "A6",  512,  688},
};

LIST(dyesub_printsize_list_t, p300_printsize_list, dyesub_printsize_t, p300_printsize);

static void p300_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("\033\033\033C\033N\1\033F\0\1\033MS\xff\xff\xff"
	      "\033Z", 1, 19, v);
  stp_put16_be(pd->w_dpi, v);
  stp_put16_be(pd->h_dpi, v);
}

static void p300_plane_end_func(stp_vars_t *v)
{
  const char *c = "CMY";
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zprintf(v, "\033\033\033P%cS", c[pd->plane-1]);
  stp_dprintf(STP_DBG_DYESUB, v, "dyesub: p300_plane_end_func: %c\n",
	c[pd->plane-1]);
}

static void p300_block_init_func(stp_vars_t *v)
{
  const char *c = "CMY";
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zprintf(v, "\033\033\033W%c", c[pd->plane-1]);
  stp_put16_be(pd->block_min_h, v);
  stp_put16_be(pd->block_min_w, v);
  stp_put16_be(pd->block_max_h, v);
  stp_put16_be(pd->block_max_w, v);

  stp_dprintf(STP_DBG_DYESUB, v, "dyesub: p300_block_init_func: %d-%dx%d-%d\n",
	pd->block_min_w, pd->block_max_w,
	pd->block_min_h, pd->block_max_h);
}

static const char p300_adj_cyan[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.078431 0.211765 0.250980 0.282353 0.309804 0.333333 0.352941 0.368627\n"
  "0.388235 0.403922 0.427451 0.443137 0.458824 0.478431 0.498039 0.513725\n"
  "0.529412 0.545098 0.556863 0.576471 0.592157 0.611765 0.627451 0.647059\n"
  "0.666667 0.682353 0.701961 0.713725 0.725490 0.729412 0.733333 0.737255\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static const char p300_adj_magenta[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.047059 0.211765 0.250980 0.278431 0.305882 0.333333 0.349020 0.364706\n"
  "0.380392 0.396078 0.415686 0.435294 0.450980 0.466667 0.482353 0.498039\n"
  "0.513725 0.525490 0.541176 0.556863 0.572549 0.592157 0.611765 0.631373\n"
  "0.650980 0.670588 0.694118 0.705882 0.721569 0.741176 0.745098 0.756863\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static const char p300_adj_yellow[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.047059 0.117647 0.203922 0.250980 0.274510 0.301961 0.321569 0.337255\n"
  "0.352941 0.364706 0.380392 0.396078 0.407843 0.423529 0.439216 0.450980\n"
  "0.466667 0.482353 0.498039 0.513725 0.533333 0.552941 0.572549 0.596078\n"
  "0.615686 0.635294 0.650980 0.666667 0.682353 0.690196 0.701961 0.713725\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static void p300_adjust_curves(stp_vars_t *v)
{
  dyesub_adjust_curve(v, p300_adj_cyan, "CyanCurve");
  dyesub_adjust_curve(v, p300_adj_magenta, "MagentaCurve");
  dyesub_adjust_curve(v, p300_adj_yellow, "YellowCurve");
}

/* Olympus P-400 series */
static const dyesub_resolution_t res_314dpi[] =
{
  { "314x314", 314, 314},
};

LIST(dyesub_resolution_list_t, res_314dpi_list, dyesub_resolution_t, res_314dpi);

static const dyesub_pagesize_t p400_page[] =
{
  DEFINE_PAPER( "A4", "A4", PT1(2400,314), PT1(3200,314), 22, 22, 54, 54, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10", "A5 wide", PT1(2000,314), PT1(2400,314), 58, 59, 84, 85, DYESUB_PORTRAIT),
  DEFINE_PAPER( "C6", "2 Postcards (A4)", PT1(1328,314), PT1(1920,314), 9, 9, 9, 9, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, p400_page_list, dyesub_pagesize_t, p400_page);

static const dyesub_printsize_t p400_printsize[] =
{
  { "314x314", "A4", 2400, 3200},
  { "314x314", "c8x10", 2000, 2400},
  { "314x314", "C6", 1328, 1920},
};

LIST(dyesub_printsize_list_t, p400_printsize_list, dyesub_printsize_t, p400_printsize);

static void p400_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  int wide = (strcmp(pd->pagesize, "c8x10") == 0
		  || strcmp(pd->pagesize, "C6") == 0);

  stp_zprintf(v, "\033ZQ"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033FP"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033ZF");
  stp_putc((wide ? '\x40' : '\x00'), v); dyesub_nputc(v, '\0', 60);
  stp_zprintf(v, "\033ZS");
  if (wide)
    {
      stp_put16_be(pd->h_size, v);
      stp_put16_be(pd->w_size, v);
    }
  else
    {
      stp_put16_be(pd->w_size, v);
      stp_put16_be(pd->h_size, v);
    }
  dyesub_nputc(v, '\0', 57);
  stp_zprintf(v, "\033ZP"); dyesub_nputc(v, '\0', 61);
}

static void p400_plane_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033ZC"); dyesub_nputc(v, '\0', 61);
}

static void p400_plane_end_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033P"); dyesub_nputc(v, '\0', 62);
}

static void p400_block_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  int wide = (strcmp(pd->pagesize, "c8x10") == 0
		  || strcmp(pd->pagesize, "C6") == 0);

  stp_zprintf(v, "\033Z%c", '3' - pd->plane + 1);
  if (wide)
    {
      stp_put16_be(pd->h_size - pd->block_max_h - 1, v);
      stp_put16_be(pd->w_size - pd->block_max_w - 1, v);
      stp_put16_be(pd->block_max_h - pd->block_min_h + 1, v);
      stp_put16_be(pd->block_max_w - pd->block_min_w + 1, v);
    }
  else
    {
      stp_put16_be(pd->block_min_w, v);
      stp_put16_be(pd->block_min_h, v);
      stp_put16_be(pd->block_max_w - pd->block_min_w + 1, v);
      stp_put16_be(pd->block_max_h - pd->block_min_h + 1, v);
    }
  dyesub_nputc(v, '\0', 53);
}

static const char p400_adj_cyan[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.003922 0.031373 0.058824 0.090196 0.125490 0.156863 0.184314 0.219608\n"
  "0.250980 0.278431 0.309804 0.341176 0.376471 0.403922 0.439216 0.470588\n"
  "0.498039 0.517647 0.533333 0.545098 0.564706 0.576471 0.596078 0.615686\n"
  "0.627451 0.647059 0.658824 0.678431 0.690196 0.705882 0.721569 0.737255\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static const char p400_adj_magenta[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.003922 0.031373 0.062745 0.098039 0.125490 0.156863 0.188235 0.215686\n"
  "0.250980 0.282353 0.309804 0.345098 0.376471 0.407843 0.439216 0.470588\n"
  "0.501961 0.521569 0.549020 0.572549 0.592157 0.619608 0.643137 0.662745\n"
  "0.682353 0.713725 0.737255 0.756863 0.784314 0.807843 0.827451 0.850980\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static const char p400_adj_yellow[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.003922 0.027451 0.054902 0.090196 0.121569 0.156863 0.184314 0.215686\n"
  "0.250980 0.282353 0.309804 0.345098 0.372549 0.400000 0.435294 0.466667\n"
  "0.498039 0.525490 0.552941 0.580392 0.607843 0.631373 0.658824 0.678431\n"
  "0.698039 0.725490 0.760784 0.784314 0.811765 0.839216 0.866667 0.890196\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static void p400_adjust_curves(stp_vars_t *v)
{
  dyesub_adjust_curve(v, p400_adj_cyan, "CyanCurve");
  dyesub_adjust_curve(v, p400_adj_magenta, "MagentaCurve");
  dyesub_adjust_curve(v, p400_adj_yellow, "YellowCurve");
}

/* Olympus P-440 series */
static const dyesub_pagesize_t p440_page[] =
{
  DEFINE_PAPER( "A4", "A4", PT1(2508,314), PT1(3200,314), 10, 9, 54, 54, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10", "A5 wide", PT1(2000,314), PT1(2508,314), 58, 59, 72, 72, DYESUB_PORTRAIT),
  DEFINE_PAPER( "C6", "2 Postcards (A4)", PT1(1328,314), PT1(1920,314), 9, 9, 9, 9, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w255h581", "A6 wide", PT1(892,314), PT1(2320,314), 25, 25, 25, 24, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, p440_page_list, dyesub_pagesize_t, p440_page);

static const dyesub_printsize_t p440_printsize[] =
{
  { "314x314", "A4", 2508, 3200},
  { "314x314", "c8x10", 2000, 2508},
  { "314x314", "C6", 1328, 1920},
  { "314x314", "w255h581", 892, 2320},
};

LIST(dyesub_printsize_list_t, p440_printsize_list, dyesub_printsize_t, p440_printsize);

static void p440_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  int wide = strcmp(pd->pagesize, "A4") != 0;

  stp_zprintf(v, "\033FP"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033Y");
  stp_write_raw(&(pd->overcoat->seq), v); /* overcoat */
  dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033FC"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033ZF");
  stp_putc((wide ? '\x40' : '\x00'), v); dyesub_nputc(v, '\0', 60);
  stp_zprintf(v, "\033N\1"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033ZS");
  if (wide)
    {
      stp_put16_be(pd->h_size, v);
      stp_put16_be(pd->w_size, v);
    }
  else
    {
      stp_put16_be(pd->w_size, v);
      stp_put16_be(pd->h_size, v);
    }
  dyesub_nputc(v, '\0', 57);
  if (strcmp(pd->pagesize, "C6") == 0)
    {
      stp_zprintf(v, "\033ZC"); dyesub_nputc(v, '\0', 61);
    }
}

static void p440_printer_end_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033P"); dyesub_nputc(v, '\0', 62);
}

static void p440_block_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  int wide = strcmp(pd->pagesize, "A4") != 0;

  stp_zprintf(v, "\033ZT");
  if (wide)
    {
      stp_put16_be(pd->h_size - pd->block_max_h - 1, v);
      stp_put16_be(pd->w_size - pd->block_max_w - 1, v);
      stp_put16_be(pd->block_max_h - pd->block_min_h + 1, v);
      stp_put16_be(pd->block_max_w - pd->block_min_w + 1, v);
    }
  else
    {
      stp_put16_be(pd->block_min_w, v);
      stp_put16_be(pd->block_min_h, v);
      stp_put16_be(pd->block_max_w - pd->block_min_w + 1, v);
      stp_put16_be(pd->block_max_h - pd->block_min_h + 1, v);
    }
  dyesub_nputc(v, '\0', 53);
}

static void p440_block_end_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  int pad = (64 - (((pd->block_max_w - pd->block_min_w + 1)
	  * (pd->block_max_h - pd->block_min_h + 1) * 3) % 64)) % 64;
  stp_dprintf(STP_DBG_DYESUB, v,
		  "dyesub: max_x %d min_x %d max_y %d min_y %d\n",
  		  pd->block_max_w, pd->block_min_w,
	  	  pd->block_max_h, pd->block_min_h);
  stp_dprintf(STP_DBG_DYESUB, v, "dyesub: olympus-p440 padding=%d\n", pad);
  dyesub_nputc(v, '\0', pad);
}


/* Olympus P-S100 */
static const dyesub_pagesize_t ps100_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1254,306), PT1(1808,306), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1120,306), PT1(1554,306), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, ps100_page_list, dyesub_pagesize_t, ps100_page);

static const dyesub_printsize_t ps100_printsize[] =
{
  { "306x306", "w288h432", 1254, 1808},
  { "306x306", "B7", 1120, 1554},
};

LIST(dyesub_printsize_list_t, ps100_printsize_list, dyesub_printsize_t, ps100_printsize);

static void ps100_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zprintf(v, "\033U"); dyesub_nputc(v, '\0', 62);

  /* stp_zprintf(v, "\033ZC"); dyesub_nputc(v, '\0', 61); */

  stp_zprintf(v, "\033W"); dyesub_nputc(v, '\0', 62);

  stp_zfwrite("\x30\x2e\x00\xa2\x00\xa0\x00\xa0", 1, 8, v);
  stp_put16_be(pd->h_size, v);	/* paper height (px) */
  stp_put16_be(pd->w_size, v);	/* paper width (px) */
  dyesub_nputc(v, '\0', 3);
  stp_putc(pd->copies, v);	/* number of copies */
  dyesub_nputc(v, '\0', 8);
  stp_putc('\1', v);
  dyesub_nputc(v, '\0', 15);
  stp_putc('\6', v);
  dyesub_nputc(v, '\0', 23);

  stp_zfwrite("\033ZT\0", 1, 4, v);
  stp_put16_be(0, v);			/* image width offset (px) */
  stp_put16_be(0, v);			/* image height offset (px) */
  stp_put16_be(pd->w_size, v);	/* image width (px) */
  stp_put16_be(pd->h_size, v);	/* image height (px) */
  dyesub_nputc(v, '\0', 52);
}

static void ps100_printer_end_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  int pad = (64 - (((pd->block_max_w - pd->block_min_w + 1)
	  * (pd->block_max_h - pd->block_min_h + 1) * 3) % 64)) % 64;
  stp_dprintf(STP_DBG_DYESUB, v,
		  "dyesub: max_x %d min_x %d max_y %d min_y %d\n",
  		  pd->block_max_w, pd->block_min_w,
	  	  pd->block_max_h, pd->block_min_h);
  stp_dprintf(STP_DBG_DYESUB, v, "dyesub: olympus-ps100 padding=%d\n", pad);
  dyesub_nputc(v, '\0', pad);		/* padding to 64B blocks */

  stp_zprintf(v, "\033PY"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033u"); dyesub_nputc(v, '\0', 62);
}


/* Canon CP-10 */
static const dyesub_resolution_t res_300dpi[] =
{
  { "300x300", 300, 300},
};

LIST(dyesub_resolution_list_t, res_300dpi_list, dyesub_resolution_t, res_300dpi);

static const dyesub_pagesize_t cp10_page[] =
{
  DEFINE_PAPER( "w155h244", "Card 54x86mm", PT1(662,300), PT1(1040,300), 6, 6, 29, 29, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, cp10_page_list, dyesub_pagesize_t, cp10_page);

static const dyesub_printsize_t cp10_printsize[] =
{
  { "300x300", "w155h244", 662, 1040},
};

LIST(dyesub_printsize_list_t, cp10_printsize_list, dyesub_printsize_t, cp10_printsize);

/* Canon CP-100 series */
static const dyesub_pagesize_t cpx00_page[] =
{
  DEFINE_PAPER( "Postcard", "Postcard 100x148mm", PT1(1232,300), PT1(1808,300), 13, 13, 16, 19, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w253h337", "CP_L 89x119mm", PT1(1100,300), PT1(1456,300), 13, 13, 15, 15, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w155h244", "Card 54x86mm", PT1(672,300), PT1(1040,300), 13, 13, 15, 15, DYESUB_LANDSCAPE),
};

LIST(dyesub_pagesize_list_t, cpx00_page_list, dyesub_pagesize_t, cpx00_page);

static const dyesub_printsize_t cpx00_printsize[] =
{
  { "300x300", "Postcard", 1232, 1808},
  { "300x300", "w253h337", 1100, 1456},
  { "300x300", "w155h244", 672, 1040},
};

LIST(dyesub_printsize_list_t, cpx00_printsize_list, dyesub_printsize_t, cpx00_printsize);

static void cp10_printer_init_func(stp_vars_t *v)
{
  stp_put16_be(0x4000, v);
  dyesub_nputc(v, '\0', 10);
}

static void cpx00_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg = (strcmp(pd->pagesize, "Postcard") == 0 ? '\1' :
		(strcmp(pd->pagesize, "w253h337") == 0 ? '\2' :
		(strcmp(pd->pagesize, "w155h244") == 0 ?
			(strcmp(stp_get_driver(v),"canon-cp10") == 0 ?
				'\0' : '\3' ) :
		(strcmp(pd->pagesize, "w283h566") == 0 ? '\4' :
		 '\1' ))));

  stp_put16_be(0x4000, v);
  stp_putc('\0', v);
  stp_putc(pg, v);
  dyesub_nputc(v, '\0', 8);
}

static void cpx00_plane_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_put16_be(0x4001, v);
  stp_putc(3 - pd->plane, v);
  stp_putc('\0', v);
  stp_put32_le(pd->w_size * pd->h_size, v);
  dyesub_nputc(v, '\0', 4);
}

static const char cpx00_adj_cyan[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.000000 0.035294 0.070588 0.101961 0.117647 0.168627 0.180392 0.227451\n"
  "0.258824 0.286275 0.317647 0.341176 0.376471 0.411765 0.427451 0.478431\n"
  "0.505882 0.541176 0.576471 0.611765 0.654902 0.678431 0.705882 0.737255\n"
  "0.764706 0.792157 0.811765 0.839216 0.862745 0.894118 0.909804 0.925490\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static const char cpx00_adj_magenta[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.011765 0.019608 0.035294 0.047059 0.054902 0.101961 0.133333 0.156863\n"
  "0.192157 0.235294 0.274510 0.321569 0.360784 0.403922 0.443137 0.482353\n"
  "0.521569 0.549020 0.584314 0.619608 0.658824 0.705882 0.749020 0.792157\n"
  "0.831373 0.890196 0.933333 0.964706 0.988235 0.992157 0.992157 0.996078\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static const char cpx00_adj_yellow[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.003922 0.015686 0.015686 0.023529 0.027451 0.054902 0.094118 0.129412\n"
  "0.180392 0.219608 0.250980 0.286275 0.317647 0.341176 0.388235 0.427451\n"
  "0.470588 0.509804 0.552941 0.596078 0.627451 0.682353 0.768627 0.796078\n"
  "0.890196 0.921569 0.949020 0.968627 0.984314 0.992157 0.992157 1.000000\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static void cpx00_adjust_curves(stp_vars_t *v)
{
  dyesub_adjust_curve(v, cpx00_adj_cyan, "CyanCurve");
  dyesub_adjust_curve(v, cpx00_adj_magenta, "MagentaCurve");
  dyesub_adjust_curve(v, cpx00_adj_yellow, "YellowCurve");
}

/* Canon CP-220 series */
static const dyesub_pagesize_t cp220_page[] =
{
  DEFINE_PAPER( "Postcard", "Postcard 100x148mm", PT1(1232,300), PT1(1808,300), 13, 13, 16, 19, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w253h337", "CP_L 89x119mm", PT1(1100,300), PT1(1456,300), 13, 13, 15, 15, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w155h244", "Card 54x86mm", PT1(672,300), PT1(1040,300), 13, 13, 15, 15, DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w283h566", "Wide 100x200mm", PT1(1232,300), PT1(2416,300), 13, 13, 20, 20, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, cp220_page_list, dyesub_pagesize_t, cp220_page);

static const dyesub_printsize_t cp220_printsize[] =
{
  { "300x300", "Postcard", 1232, 1808},
  { "300x300", "w253h337", 1100, 1456},
  { "300x300", "w155h244", 672, 1040},
  { "300x300", "w283h566", 1232, 2416},
};

LIST(dyesub_printsize_list_t, cp220_printsize_list, dyesub_printsize_t, cp220_printsize);

/* Canon SELPHY CP790 */
static void cp790_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg = (strcmp(pd->pagesize, "Postcard") == 0 ? '\0' :
		(strcmp(pd->pagesize, "w253h337") == 0 ? '\1' :
		(strcmp(pd->pagesize, "w155h244") == 0 ? '\2' :
		(strcmp(pd->pagesize, "w283h566") == 0 ? '\3' :
		 '\0' ))));

  stp_put16_be(0x4000, v);
  stp_putc(pg, v);
  stp_putc('\0', v);
  dyesub_nputc(v, '\0', 8);
  stp_put32_le(pd->w_size * pd->h_size, v);
}

/* Canon SELPHY ES series */
static void es1_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg = (strcmp(pd->pagesize, "Postcard") == 0 ? 0x11 :
	     (strcmp(pd->pagesize, "w253h337") == 0 ? 0x12 :
	      (strcmp(pd->pagesize, "w155h244") == 0 ? 0x13 : 0x11)));

  stp_put16_be(0x4000, v);
  stp_putc(0x10, v);  /* 0x20 for P-BW */
  stp_putc(pg, v);
  dyesub_nputc(v, '\0', 8);
}

static void es1_plane_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  unsigned char plane = 0;

  switch (pd->plane) {
  case 3: /* Y */
    plane = 0x01;
    break;
  case 2: /* M */
    plane = 0x03;
    break;
  case 1: /* C */
    plane = 0x07;
    break;
  }

  stp_put16_be(0x4001, v);
  stp_putc(0x1, v); /* 0x02 for P-BW */
  stp_putc(plane, v);
  stp_put32_le(pd->w_size * pd->h_size, v);
  dyesub_nputc(v, '\0', 4);
}

static void es2_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg2 = 0x0;
  char pg = (strcmp(pd->pagesize, "Postcard") == 0 ? 0x1:
	     (strcmp(pd->pagesize, "w253h337") == 0 ? 0x2 :
	      (strcmp(pd->pagesize, "w155h244") == 0 ? 0x3 : 0x1)));

  if (pg == 0x03)
    pg2 = 0x01;

  stp_put16_be(0x4000, v);
  stp_putc(pg, v);
  stp_putc(0x0, v);

  stp_putc(0x2, v);
  dyesub_nputc(v, 0x0, 2);
  stp_putc(0x0, v);  /*  0x1 for P-BW */

  dyesub_nputc(v, 0x0, 3);
  stp_putc(pg2, v);
  stp_put32_le(pd->w_size * pd->h_size, v);
}

static void es2_plane_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_put16_be(0x4001, v);
  stp_putc(4 - pd->plane, v);
  stp_putc(0x0, v);
  dyesub_nputc(v, '\0', 8);
}

static void es3_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg = (strcmp(pd->pagesize, "Postcard") == 0 ? 0x1:
	     (strcmp(pd->pagesize, "w253h337") == 0 ? 0x2 :
	      (strcmp(pd->pagesize, "w155h244") == 0 ? 0x3 : 0x1)));

    /* We also have Pg and Ps  (Gold/Silver) papers on the ES3/30/40 */

  stp_put16_be(0x4000, v);
  stp_putc(pg, v);
  stp_putc(0x0, v);  /* 0x1 for P-BW */
  dyesub_nputc(v, 0x0, 8);
  stp_put32_le(pd->w_size * pd->h_size, v);
}

static void es3_printer_end_func(stp_vars_t *v)
{
  stp_put16_be(0x4020, v);
  dyesub_nputc(v, 0x0, 10);
}

static void es40_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg = (strcmp(pd->pagesize, "Postcard") == 0 ? 0x0:
	     (strcmp(pd->pagesize, "w253h337") == 0 ? 0x1 :
	      (strcmp(pd->pagesize, "w155h244") == 0 ? 0x2 : 0x0)));

    /* We also have Pg and Ps  (Gold/Silver) papers on the ES3/30/40 */

  stp_put16_be(0x4000, v);
  stp_putc(pg, v);
  stp_putc(0x0, v);  /*  0x1 for P-BW */
  dyesub_nputc(v, 0x0, 8);

  stp_put32_le(pd->w_size * pd->h_size, v);
}

/* Canon SELPHY CP900 */
static void cp900_printer_end_func(stp_vars_t *v)
{
  dyesub_nputc(v, 0x0, 4);
}

/* Canon CP820/CP910/CP1000/CP1200 and beynod */
static const dyesub_pagesize_t cp910_page[] =
{
  DEFINE_PAPER( "Postcard", "Postcard 100x148mm", PT1(1248,300), PT1(1872,300), 13, 13, 16, 19, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w253h337", "CP_L 89x119mm", PT1(1152,300), PT1(1472,300), 13, 13, 15, 15, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w155h244", "Card 54x86mm", PT1(668,300), PT1(1088,300), 13, 13, 15, 15, DYESUB_LANDSCAPE),
};

LIST(dyesub_pagesize_list_t, cp910_page_list, dyesub_pagesize_t, cp910_page);

static const dyesub_printsize_t cp910_printsize[] =
{
  { "300x300", "Postcard", 1248, 1872},
  { "300x300", "w253h337", 1152, 1472},
  { "300x300", "w155h244", 668, 1088},
};

LIST(dyesub_printsize_list_t, cp910_printsize_list, dyesub_printsize_t, cp910_printsize);

static void cp910_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg;

  stp_zfwrite("\x0f\x00\x00\x40\x00\x00\x00\x00", 1, 8, v);
  stp_zfwrite("\x00\x00\x00\x00\x00\x00\x01\x00", 1, 8, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);

  pg = (strcmp(pd->pagesize, "Postcard") == 0 ? 0x50 :
                (strcmp(pd->pagesize, "w253h337") == 0 ? 0x4c :
                (strcmp(pd->pagesize, "w155h244") == 0 ? 0x43 :
                 0x50 )));
  stp_putc(pg, v);

  dyesub_nputc(v, '\0', 4);
  stp_putc(0x01, v);

  stp_put32_le(pd->w_size, v);
  stp_put32_le(pd->h_size, v);
}

/* Sony DPP-EX5, DPP-EX7 */
static const dyesub_resolution_t res_403dpi[] =
{
  { "403x403", 403, 403},
};

LIST(dyesub_resolution_list_t, res_403dpi_list, dyesub_resolution_t, res_403dpi);

/* only Postcard pagesize is supported */
static const dyesub_pagesize_t dppex5_page[] =
{
  DEFINE_PAPER( "w288h432", "Postcard", PT1(1664,403), PT1(2466,403), 13, 14, 18, 17, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, dppex5_page_list, dyesub_pagesize_t, dppex5_page);

static const dyesub_printsize_t dppex5_printsize[] =
{
  { "403x403", "w288h432", 1664, 2466},
};

LIST(dyesub_printsize_list_t, dppex5_printsize_list, dyesub_printsize_t, dppex5_printsize);

static void dppex5_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("DPEX\0\0\0\x80", 1, 8, v);
  stp_zfwrite("DPEX\0\0\0\x82", 1, 8, v);
  stp_zfwrite("DPEX\0\0\0\x84", 1, 8, v);
  stp_put32_be(pd->w_size, v);
  stp_put32_be(pd->h_size, v);
  stp_zfwrite("S\0o\0n\0y\0 \0D\0P\0P\0-\0E\0X\0\x35\0", 1, 24, v);
  dyesub_nputc(v, '\0', 40);
  stp_zfwrite("\1\4\0\4\xdc\0\x24\0\3\3\1\0\1\0\x82\0", 1, 16, v);
  stp_zfwrite("\xf4\5\xf8\3\x64\0\1\0\x0e\0\x93\1\2\0\1\0", 1, 16, v);
  stp_zfwrite("\x93\1\1\0\0\0", 1, 6, v);
  stp_zfwrite("P\0o\0s\0t\0 \0c\0a\0r\0d\0", 1, 18, v);
  dyesub_nputc(v, '\0', 46);
  stp_zfwrite("\x93\1\x18", 1, 3, v);
  dyesub_nputc(v, '\0', 19);
  stp_zfwrite("\2\0\0\0\3\0\0\0\1\0\0\0\1", 1, 13, v);
  dyesub_nputc(v, '\0', 19);
  stp_zprintf(v, "5EPD");
  dyesub_nputc(v, '\0', 4);
  stp_zfwrite((pd->overcoat->seq).data, 1,
			(pd->overcoat->seq).bytes, v); /*overcoat pattern*/
  stp_zfwrite("\0d\0d\0d", 1, 6, v);
  dyesub_nputc(v, '\0', 21);
}

static void dppex5_block_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("DPEX\0\0\0\x85", 1, 8, v);
  stp_put32_be((pd->block_max_w - pd->block_min_w + 1)
  		* (pd->block_max_h - pd->block_min_h + 1) * 3, v);
}

static void dppex5_printer_end(stp_vars_t *v)
{
  stp_zfwrite("DPEX\0\0\0\x83", 1, 8, v);
  stp_zfwrite("DPEX\0\0\0\x81", 1, 8, v);
}

static const overcoat_t dppex5_overcoat[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x00"}},
  {"Texture", N_("Texture"), {1, "\x01"}},
};

LIST(overcoat_list_t, dppex5_overcoat_list, overcoat_t, dppex5_overcoat);


/* Sony UP-DP10 */
static const dyesub_pagesize_t updp10_page[] =
{
 DEFINE_PAPER( "w288h432", "UPC-10P23 (4x6)", PT(1200,300), PT(1800,300), 12, 12, 18, 18, DYESUB_LANDSCAPE),
 DEFINE_PAPER( "w288h387", "UPC-10P34 (4x5)", PT(1200,300), PT(1600,300), 12, 12, 16, 16, DYESUB_LANDSCAPE),
};

LIST(dyesub_pagesize_list_t, updp10_page_list, dyesub_pagesize_t, updp10_page);

static const dyesub_printsize_t updp10_printsize[] =
{
  { "300x300", "w288h432", 1200, 1800},
  { "300x300", "w288h387", 1200, 1600},
};

LIST(dyesub_printsize_list_t, updp10_printsize_list, dyesub_printsize_t, updp10_printsize);

static void updp10_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("\x98\xff\xff\xff"
	      "\xff\xff\xff\xff"
	      "\x09\x00\x00\x00"
	      "\x1b\xee\x00\x00"
	      "\x00\x02\x00", 1, 19, v);
  stp_put16_be(pd->copies, v);
  stp_zfwrite("\x12\x00\x00\x00"
              "\x1b\xe1\x00\x00\x00\x0b\x00\x00\x04", 1, 13, v);
  stp_zfwrite((pd->overcoat->seq).data, 1,
			(pd->overcoat->seq).bytes, v); /*overcoat pattern*/
  stp_zfwrite("\x00\x00\x00\x00", 1, 4, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_zfwrite("\x14\x00\x00\x00"
	      "\x1b\x15\x00\x00\x00\x0d\x00\x00"
	      "\x00\x00\x00\x07\x00\x00\x00\x00", 1, 20, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_put32_le(pd->w_size*pd->h_size*3+11, v);
  stp_zfwrite("\x1b\xea\x00\x00\x00\x00", 1, 6, v);
  stp_put32_be(pd->w_size*pd->h_size*3, v);
  stp_zfwrite("\x00", 1, 1, v);
}

static void updp10_printer_end_func(stp_vars_t *v)
{
  stp_zfwrite("\xff\xff\xff\xff"
	      "\x07\x00\x00\x00"
	      "\x1b\x0a\x00\x00\x00\x00\x00"
	      "\xfd\xff\xff\xff"
	      "\xff\xff\xff\xff", 1, 23, v);
}

static const overcoat_t updp10_overcoat[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x00"}},
  {"Texture", N_("Texture"), {1, "\x08"}},
  {"Matte",   N_("Matte"),   {1, "\x0c"}},
};

LIST(overcoat_list_t, updp10_overcoat_list, overcoat_t, updp10_overcoat);

static const char updp10_adj_cyan[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"33\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.113725 0.188235 0.247059 0.286275 0.317647 0.345098 0.368627 0.384314\n"
  "0.400000 0.407843 0.423529 0.439216 0.450980 0.466667 0.482353 0.498039\n"
  "0.509804 0.525490 0.545098 0.560784 0.580392 0.596078 0.619608 0.643137\n"
  "0.662745 0.686275 0.709804 0.729412 0.756863 0.780392 0.811765 0.843137\n"
  "1.000000\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static const char updp10_adj_magenta[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"33\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.105882 0.211765 0.286275 0.333333 0.364706 0.388235 0.403922 0.415686\n"
  "0.427451 0.439216 0.450980 0.462745 0.478431 0.494118 0.505882 0.521569\n"
  "0.537255 0.552941 0.568627 0.584314 0.600000 0.619608 0.643137 0.662745\n"
  "0.682353 0.709804 0.733333 0.760784 0.792157 0.823529 0.858824 0.890196\n"
  "1.000000\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static const char updp10_adj_yellow[] =
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<gutenprint>\n"
  "<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
  "<sequence count=\"33\" lower-bound=\"0\" upper-bound=\"1\">\n"
  "0.101961 0.160784 0.196078 0.227451 0.243137 0.254902 0.266667 0.286275\n"
  "0.309804 0.337255 0.368627 0.396078 0.423529 0.443137 0.462745 0.478431\n"
  "0.501961 0.517647 0.537255 0.556863 0.576471 0.596078 0.619608 0.643137\n"
  "0.666667 0.690196 0.709804 0.737255 0.760784 0.780392 0.796078 0.803922\n"
  "1.000000\n"
  "</sequence>\n"
  "</curve>\n"
  "</gutenprint>\n";

static void updp10_adjust_curves(stp_vars_t *v)
{
  dyesub_adjust_curve(v, updp10_adj_cyan, "CyanCurve");
  dyesub_adjust_curve(v, updp10_adj_magenta, "MagentaCurve");
  dyesub_adjust_curve(v, updp10_adj_yellow, "YellowCurve");
}

/* Sony UP-DR100 */
static const dyesub_pagesize_t updr100_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1382,334), PT1(2048,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1210,334), PT1(1710,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1710,334), PT1(2380,334), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(2048,334), PT1(2724,334), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, updr100_page_list, dyesub_pagesize_t, updr100_page);

static const dyesub_printsize_t updr100_printsize[] =
{
  { "334x334", "w288h432", 1382, 2048},
  { "334x334", "B7", 1210, 1710},
  { "334x334", "w360h504", 1710, 2380},
  { "334x334", "w432h576", 2048, 2724},
};

LIST(dyesub_printsize_list_t, updr100_printsize_list, dyesub_printsize_t, updr100_printsize);

static void updr100_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("UPD8D\x00\x00\x00\x10\x03\x00\x00", 1, 12, v);
  stp_put32_le(pd->w_size, v);
  stp_put32_le(pd->h_size, v);
  stp_zfwrite("\x1e\x00\x03\x00\x01\x00\x4e\x01\x00\x00", 1, 10, v);
  stp_write_raw(&(pd->overcoat->seq), v); /* overcoat pattern */
  dyesub_nputc(v, '\0', 13);
  stp_zfwrite("\x01\x00\x01\x00\x03", 1, 5, v);
  dyesub_nputc(v, '\0', 19);
}

static void updr100_printer_end_func(stp_vars_t *v)
{
  stp_zfwrite("UPD8D\x00\x00\x00\x02", 1, 9, v);
  dyesub_nputc(v, '\0', 25);
  stp_zfwrite("\x9d\x02\x00\x04\x00\x00\xc0\xe7"
  	      "\x9d\x02\x54\xe9\x9d\x02\x9d\x71"
	      "\x00\x73\xfa\x71\x00\x73\xf4\xea"
	      "\x9d\x02\xa8\x3e\x00\x73\x9c\xeb\x9d\x02"
	      , 1, 34, v);
}

static const overcoat_t updr100_overcoat[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x01"}},
  {"Texture", N_("Texture"), {1, "\x03"}},
  {"Matte",   N_("Matte"),   {1, "\x04"}},
};

LIST(overcoat_list_t, updr100_overcoat_list, overcoat_t, updr100_overcoat);


/* Sony UP-DR150 */
static const dyesub_resolution_t res_334dpi[] =
{
  { "334x334", 334, 334},
};

LIST(dyesub_resolution_list_t, res_334dpi_list, dyesub_resolution_t, res_334dpi);

static const dyesub_pagesize_t updr150_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1382,334), PT1(2048,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1210,334), PT1(1728,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1728,334), PT1(2380,334), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(2048,334), PT1(2724,334), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, updr150_page_list, dyesub_pagesize_t, updr150_page);

static const dyesub_printsize_t updr150_printsize[] =
{
  { "334x334", "w288h432", 1382, 2048},
  { "334x334", "B7", 1210, 1728},
  { "334x334", "w360h504", 1728, 2380},
  { "334x334", "w432h576", 2048, 2724},
};

LIST(dyesub_printsize_list_t, updr150_printsize_list, dyesub_printsize_t, updr150_printsize);

static void updr150_200_printer_init_func(stp_vars_t *v, int updr200)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg;

  stp_zfwrite("\x6a\xff\xff\xff"
	      "\xef\xff\xff\xff", 1, 8, v);

  if (strcmp(pd->pagesize,"B7") == 0)
    pg = '\x01';
  else if (strcmp(pd->pagesize,"w288h432") == 0)
    pg = '\x02';
  else if (updr200 && strcmp(pd->pagesize,"w288h432-div2") == 0)
    pg = '\x02';
  else if (strcmp(pd->pagesize,"w360h504") == 0)
    pg = '\x03';
  else if (updr200 && strcmp(pd->pagesize,"w360h504-div2") == 0)
    pg = '\x03';
  else if (strcmp(pd->pagesize,"w432h576") == 0)
    pg = '\x04';
  else if (updr200 && strcmp(pd->pagesize,"w432h576-div2") == 0)
    pg = '\x04';
  else
    pg = 0;

  stp_put32_le(pg, v);

  stp_zfwrite("\xfc\xff\xff\xff"
	      "\xfb\xff\xff\xff"
	      "\xf4\xff\xff\xff"
	      "\xf5\xff\xff\xff",
	      1, 16, v);

  /* Multicut mode */
  if (updr200) {
    if (!strcmp(pd->pagesize,"w288h432-div2") ||
	!strcmp(pd->pagesize,"w360h504-div2") ||
	!strcmp(pd->pagesize,"w432h576-div2"))
      pg = 0x01;
    else
      pg = 0x02;
  } else {
    pg = 0x01;
  }

  stp_put32_le(pg, v);

  stp_zfwrite("\x07\x00\x00\x00"
	      "\x1b\xe5\x00\x00\x00\x08\x00"
	      "\x08\x00\x00\x00"
	      "\x00\x00\x00\x00\x00\x00\x01\x00"
	      "\xed\xff\xff\xff"
	      "\x07\x00\x00\x00"
	      "\x1b\xee\x00\x00\x00\x02\x00"
	      "\x02\x00\x00\x00", 1, 42, v);
  stp_put16_be(pd->copies, v);

  if (updr200) { /* UP-DR200-specific! */
    stp_zfwrite("\x07\x00\x00\x00"
		"\x1b\xc0\x00\x03\x00\x05\x00", 1, 11, v);
    stp_zfwrite("\x05\x00\x00\x00"
		"\x02\x03\x00\x01", 1, 8, v);

    if (!strcmp(pd->pagesize,"w288h432-div2") ||
	!strcmp(pd->pagesize,"w360h504-div2") ||
	!strcmp(pd->pagesize,"w432h576-div2"))
      stp_putc(0x02, v);
    else
      stp_putc(0x00, v);
  }

  stp_zfwrite("\x07\x00\x00\x00"
	      "\x1b\x15\x00\x00\x00\x0d\x00"
	      "\x0d\x00\x00\x00"
	      "\x00\x00\x00\x00\x07\x00\x00\x00\x00", 1, 24, v);

  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);

  stp_zfwrite("\xf9\xff\xff\xff",
	      1, 4, v);
  stp_zfwrite("\x07\x00\x00\x00"
	      "\x1b\xe1\x00\x00\x00\x0b\x00"
	      "\x0b\x00\x00\x00\x00\x80", 1, 17, v);
  stp_zfwrite((pd->overcoat->seq).data, 1,
			(pd->overcoat->seq).bytes, v); /*overcoat pattern*/

  stp_zfwrite("\x00\x00\x00\x00", 1, 4, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_zfwrite("\xf8\xff\xff\xff", 1, 4, v);

  /* Each data block has this header.  Can actually have multiple blocks! */
  stp_zfwrite("\xec\xff\xff\xff", 1, 4, v);
  stp_zfwrite("\x0b\x00\x00\x00\x1b\xea"
	      "\x00\x00\x00\x00", 1, 10, v);
  stp_put32_be(pd->w_size*pd->h_size*3, v);
  stp_zfwrite("\x00", 1, 1, v);
  stp_put32_le(pd->w_size*pd->h_size*3, v);
}

static void updr150_printer_init_func(stp_vars_t *v)
{
  updr150_200_printer_init_func(v, 0);
}

static void updr150_200_printer_end_func(stp_vars_t *v, int updr200)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("\xeb\xff\xff\xff"
	      "\xfc\xff\xff\xff"
	      "\xfa\xff\xff\xff",
	      1, 12, v);
  stp_zfwrite("\x07\x00\x00\x00"
	      "\x1b\x0a\x00\x00\x00\x00\x00"
	      "\x07\x00\x00\x00"
	      "\x1b\x17\x00\x00\x00\x00\x00",
	      1, 22, v);

  /* Multicut mode */
  if (updr200) {
    if (!strcmp(pd->pagesize,"w288h432-div2") ||
	!strcmp(pd->pagesize,"w360h504-div2") ||
	!strcmp(pd->pagesize,"w432h576-div2")) {
      stp_zfwrite("\x07\x00\x00\x00"
		  "\x1b\xc0\x00\x03\x00\x05\x00", 1, 11, v);
      stp_zfwrite("\x05\x00\x00\x00"
		  "\x02\x03\x00\x01\x01", 1, 9, v);
    }
  }
  stp_zfwrite("\xf3\xff\xff\xff",
	      1, 4, v);
}

static void updr150_printer_end_func(stp_vars_t *v)
{
  updr150_200_printer_end_func(v, 0);
}

static void updr200_printer_end_func(stp_vars_t *v)
{
  updr150_200_printer_end_func(v, 1);
}

/* Sony UP-DR200 */
static const dyesub_pagesize_t updr200_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1382,334), PT1(2048,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1382,334), PT1(2048,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1210,334), PT1(1728,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1728,334), PT1(2380,334), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w360h504-div2", "3.5x5*2", PT1(1728,334), PT1(2420,334), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(2048,334), PT1(2724,334), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576-div2", "4x6*2", PT1(2048,334), PT1(2764,334), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, updr200_page_list, dyesub_pagesize_t, updr200_page);

static const dyesub_printsize_t updr200_printsize[] =
{
  { "334x334", "w288h432", 1382, 2048},
  { "334x334", "w288h432-div2", 1382, 2048},
  { "334x334", "B7", 1210, 1728},
  { "334x334", "w360h504", 1728, 2380},
  { "334x334", "w360h504-div2", 1728, 2420},
  { "334x334", "w432h576", 2048, 2724},
  { "334x334", "w432h576-div2", 2048, 2764},
};

LIST(dyesub_printsize_list_t, updr200_printsize_list, dyesub_printsize_t, updr200_printsize);

static const overcoat_t updr200_overcoat[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x00"}},
  {"Matte",   N_("Matte"),   {1, "\x0c"}},
  {"Glossy_NoCorr",  N_("Glossy_NoCorr"),  {1, "\x10"}},
  {"Matte_NoCorr",  N_("Matte_NoCorr"),  {1, "\x1c"}},
};

LIST(overcoat_list_t, updr200_overcoat_list, overcoat_t, updr200_overcoat);

static void updr200_printer_init_func(stp_vars_t *v)
{
  updr150_200_printer_init_func(v, 1);
}

/* Sony UP-CR10L / DNP SL10 */
/* Note:  This printer reverses the traditional X and Y axes. */
static const dyesub_pagesize_t upcr10_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1848,300), PT1(1248,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1536,300), PT1(1100,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1536,300), PT1(2148,300), DYESUB_LANDSCAPE),
};

LIST(dyesub_pagesize_list_t, upcr10_page_list, dyesub_pagesize_t, upcr10_page);

static const dyesub_printsize_t upcr10_printsize[] =
{
  { "300x300", "w288h432", 1848, 1248},
  { "300x300", "B7", 1536, 1100},
  { "300x300", "w360h504", 1536, 2148},
};

LIST(dyesub_printsize_list_t, upcr10_printsize_list, dyesub_printsize_t, upcr10_printsize);

static void upcr10_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg = 0;

  stp_zfwrite("\x60\xff\xff\xff"
	      "\xf8\xff\xff\xff", 1, 8, v);

  if (strcmp(pd->pagesize,"B7") == 0)
    pg = '\xff';
  else if (strcmp(pd->pagesize,"w288h432") == 0)
    pg = '\xfe';
  else if (strcmp(pd->pagesize,"w360h504") == 0)
    pg = '\xfd';

  stp_putc(pg, v);
  stp_zfwrite("\xff\xff\xff"
	      "\x14\x00\x00\x00"
	      "\x1b\x15\x00\x00\x00\x0d\x00\x00"
	      "\x00\x00\x00\x07\x00\x00\x00\x00", 1, 23, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_zfwrite("\xfb\xff\xff\xff"
	      "\xf4\xff\xff\xff\x0b\x00\x00\x00"
	      "\x1b\xea\x00\x00\x00\x00", 1, 18, v);
  stp_put32_be(pd->w_size * pd->h_size * 3, v);
  stp_putc(0, v);
  stp_put32_le(pd->w_size * pd->h_size * 3, v);
}

static void upcr10_printer_end_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("\xf3\xff\xff\xff"
	      "\x0f\x00\x00\x00"
	      "\x1b\xe5\x00\x00\x00\x08\x00\x00"
	      "\x00\x00\x00\x00\x00\x00\x00", 1, 23, v);
  stp_zfwrite("\x12\x00\x00\x00\x1b\xe1\x00\x00"
	      "\x00\x0b\x00\x00\x80\x00\x00\x00"
	      "\x00\x00", 1, 18, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_zfwrite("\xfa\xff\xff\xff"
	      "\x09\x00\x00\x00"
	      "\x1b\xee\x00\x00\x00\x02\x00", 1, 15, v);
  stp_put16_be(pd->copies, v);
  stp_zfwrite("\x07\x00\x00\x00"
	      "\x1b\x0a\x00\x00\x00\x00\x00", 1, 11, v);
  stp_zfwrite("\xf9\xff\xff\xff"
	      "\xfc\xff\xff\xff"
	      "\x07\x00\x00\x00"
	      "\x1b\x17\x00\x00\x00\x00\x00", 1, 19, v);
  stp_zfwrite("\xf7\xff\xff\xff", 1, 4, v);
}

/* Sony UP-D895/897MD */
/* Note:  These printers reverse the traditional X and Y axes.  1280 is _always_ the Y axis. */
static const dyesub_pagesize_t sony_d89x_page[] =
{
  DEFINE_PAPER_SIMPLE( "w213h284", "960x1280", PT1(960,325), PT1(1280,325), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w284h284", "1280x1280", PT1(1280,325), PT1(1280,325), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w426h284", "1920x1280", PT1(1920,325), PT1(1280,325), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w852h284", "3840x1280", PT1(3840,325), PT1(1280,325), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w907h284", "4096x1280", PT1(4096,325), PT1(1280,325), DYESUB_PORTRAIT),
  /* A true "custom" size, printer will cut at the image boundary */
  DEFINE_PAPER_SIMPLE( "Custom", "Custom", -1, PT1(1280,325), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, sony_d89x_page_list, dyesub_pagesize_t, sony_d89x_page);

static const dyesub_printsize_t sony_d89x_printsize[] =
{
  { "325x325", "w213h284", 960, 1280},
  { "325x325", "w284h284", 1280, 1280},
  { "325x325", "w426h284", 1920, 1280},
  { "325x325", "w852h284", 3840, 1280},
  { "325x325", "w907h284", 4096, 1280},
  { "325x325", "Custom", 4096, 1280}, /* Maximum */
};

LIST(dyesub_printsize_list_t, sony_d89x_printsize_list, dyesub_printsize_t, sony_d89x_printsize);

static const dyesub_stringitem_t sony_upd895_gammas[] =
{
  { "Soft", N_ ("Soft") },
  { "Normal", N_ ("Normal") },
  { "Hard", N_ ("Hard") },
};
LIST(dyesub_stringlist_t, sony_upd895_gamma_list, dyesub_stringitem_t, sony_upd895_gammas);

static const stp_parameter_t sony_upd895_parameters[] =
{
  {
    "SonyGamma", N_("Printer Gamma Correction"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Gamma Correction"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define sony_upd895_parameter_count (sizeof(sony_upd895_parameters) / sizeof(const stp_parameter_t))

static int
sony_upd895_load_parameters(const stp_vars_t *v, const char *name,
			    stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

    if (strcmp(name, "SonyGamma") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &sony_upd895_gamma_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 2)->name;
      description->is_active = 1;
    }
  else
   {
     return 0;
   }
  return 1;
}

static int sony_upd895_parse_parameters(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  const char *gamma = stp_get_string_parameter(v, "SonyGamma");

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  pd->privdata.sonymd.unk_gg = 0x0; // XXX Likely it is "empty rows"

  pd->privdata.sonymd.dark = 0;
  pd->privdata.sonymd.light = 0;
  pd->privdata.sonymd.advance = 0;
  pd->privdata.sonymd.sharp = 0;

  if (!strcmp(gamma, "Hard")) {
    pd->privdata.sonymd.gamma = 0x03;
  } else if (!strcmp(gamma, "Normal")) {
    pd->privdata.sonymd.gamma = 0x02;
  } else if (!strcmp(gamma, "Soft")) {
    pd->privdata.sonymd.gamma = 0x01;
  } else{
    pd->privdata.sonymd.gamma = 0x00;
  }
}

static void sony_upd89x_printer_init_func(stp_vars_t *v, int is_897)
{
  dyesub_privdata_t *pd = get_privdata(v);

  if (is_897)
    {
      stp_zfwrite("\x8e\xff\xff\xff\xfc\xff\xff\xff"
		  "\xfb\xff\xff\xff\xf5\xff\xff\xff"
		  "\xf1\xff\xff\xff\xf0\xff\xff\xff"
		  "\xef\xff\xff\xff", 1, 28, v);
    }
  else
  {
    stp_zfwrite("\x9c\xff\xff\xff"
		"\x97\xff\xff\xff", 1, 8, v);
    dyesub_nputc(v, 0x0, 12);
    stp_put32_be(0xffffffff, v);
  }

  stp_put32_le(20, v);
  stp_zfwrite("\x1b\x15\x00\x00\x00\x0d\x00\x00"
	      "\x00\x00\x00\x01\x00\x00", 1, 14, v);
  stp_put16_be(pd->privdata.sonymd.unk_gg, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);

  stp_put32_le(11, v);
  stp_zfwrite("\x1b\xea\x00\x00\x00\x00", 1, 6, v);
  stp_put32_be(pd->h_size * pd->w_size, v);
  stp_putc(0x00, v);
  stp_put32_le(pd->h_size * pd->w_size, v);
}

static void sony_upd895_printer_init_func(stp_vars_t *v)
{
  sony_upd89x_printer_init_func(v, 0);
}

static void sony_upd897_printer_init_func(stp_vars_t *v)
{
  sony_upd89x_printer_init_func(v, 1);
}

static void sony_upd89x_printer_end_func(stp_vars_t *v, int is_897)
{
  dyesub_privdata_t *pd = get_privdata(v);

  if (is_897)
    {
       stp_put32_be(0xeaffffff, v);
    }
  else
    {
       stp_put32_be(0xffffffff, v);
    }

  stp_put32_le(9, v);
  stp_zfwrite("\x1b\xee\x00\x00\x00\x02\x00", 1, 7, v);
  stp_put16_be(pd->copies, v);

  if (is_897)
    {
      stp_put32_be(0xeeffffff, v);
      stp_put32_be(1, v);
    }

  stp_put32_le(15, v);
  stp_zfwrite("\x1b\xe5\x00\x00\x00\x08\x00\x00\x00\x00\x00", 1, 11, v);
  stp_putc(pd->privdata.sonymd.dark, v);
  stp_putc(pd->privdata.sonymd.light, v);
  stp_putc(pd->privdata.sonymd.sharp, v);
  stp_putc(pd->privdata.sonymd.advance, v);

  if (is_897)
    {
      stp_put32_be(0xebffffff, v);
      stp_put32_be(2, v);  // Sharpness?  02 and 05 seen
    }

  stp_put32_le(12, v);
  stp_zfwrite("\x1b\xc0\x00\x00\x00\x05\x00\x02", 1, 8, v);
  stp_zfwrite("\x00\x00\x01", 1, 3, v);
  stp_putc(pd->privdata.sonymd.gamma, v);

  if (is_897)
    {
      stp_put32_be(0xecffffff, v);
      stp_put32_be(1, v); // 00 01 02 seen
    }

  stp_put32_le(17, v);
  stp_zfwrite("\x1b\xc0\x00\x01\x00\x0a\x00\x02", 1, 8, v);
  stp_zfwrite("\x01\x00\x06", 1, 3, v);
  dyesub_nputc(v, 0x0, 6);

  if (is_897)
    {
      stp_put32_be(0xedffffff, v);
      stp_put32_be(0, v);
    }

  stp_put32_le(18, v);
  stp_zfwrite("\x1b\xe1\x00\x00\x00\x0b\x00\x00\x08\00", 1, 10, v);
  stp_put16_be(0x00, v); // XXX GG GG, based on print size.
  dyesub_nputc(v, 0x0, 2);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);

  if (is_897)
    {
      stp_put32_be(0xfaffffff, v);
    }

  stp_put32_le(7, v);
  stp_zfwrite("\x1b\x0a\x00\x00\x00\x00\x00", 1, 7, v);

  if (is_897)
    {
       stp_zfwrite("\xfc\xff\xff\xff"
		   "\xfd\xff\xff\xff"
		   "\xff\xff\xff\xff", 1, 12, v);
       stp_put32_le(7, v);
       stp_zfwrite("\x1b\x17\x00\x00\x00\x00\x00", 1, 7, v);
       stp_put32_be(0xf4ffffff, v);
    }
  else
    {
       stp_zfwrite("\xfd\xff\xff\xff"
		   "\xf7\xff\xff\xff"
		   "\xf8\xff\xff\xff", 1, 12, v);
    }

}

static void sony_upd895_printer_end_func(stp_vars_t *v)
{
  sony_upd89x_printer_end_func(v, 0);
}

static void sony_upd897_printer_end_func(stp_vars_t *v)
{
  sony_upd89x_printer_end_func(v, 1);
}

static const dyesub_stringitem_t sony_upd897_gammas[] =
{
  { "Softest", N_ ("Softest") },
  { "Soft", N_ ("Soft") },
  { "Normal", N_ ("Normal") },
  { "Hard", N_ ("Hard") },
};
LIST(dyesub_stringlist_t, sony_upd897_gamma_list, dyesub_stringitem_t, sony_upd897_gammas);

static const stp_parameter_t sony_upd897_parameters[] =
{
  {
    "SonyGamma", N_("Printer Gamma Correction"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Gamma Correction"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Sharpen", N_("Image Sharpening"), "Color=No,Category=Advanced Printer Setup",
    N_("Sharpening to apply to image (0 is off, 14 is max"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Darkness", N_("Darkness"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Image Darkness Adjustment"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Lightness", N_("Lightness"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Image Lightness Adjustment"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Advance", N_("Advance"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Image Advance Adjustment"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },

};
#define sony_upd897_parameter_count (sizeof(sony_upd897_parameters) / sizeof(const stp_parameter_t))

static int
sony_upd897_load_parameters(const stp_vars_t *v, const char *name,
			    stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

    if (strcmp(name, "SonyGamma") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &sony_upd897_gamma_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 3)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "Darkness") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -64;
      description->bounds.integer.upper = 64;
      description->is_active = 1;
    }
  else if (strcmp(name, "Lightness") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -64;
      description->bounds.integer.upper = 64;
      description->is_active = 1;
    }
  else if (strcmp(name, "Advance") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -32;
      description->bounds.integer.upper = 32;
      description->is_active = 1;
    }
  else if (strcmp(name, "Sharpen") == 0)
    {
      description->deflt.integer = 2;
      description->bounds.integer.lower = 0;
      description->bounds.integer.upper = 14;
      description->is_active = 1;
    }
  else
   {
     return 0;
   }
  return 1;
}

static int sony_upd897_parse_parameters(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  const char *gamma = stp_get_string_parameter(v, "SonyGamma");

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  pd->privdata.sonymd.unk_gg = 0xa2; // XXX Suspect it's empty rows
  pd->privdata.sonymd.dark = stp_get_int_parameter(v, "Darkness");
  pd->privdata.sonymd.light = stp_get_int_parameter(v, "Lightness");
  pd->privdata.sonymd.advance = stp_get_int_parameter(v, "Advance");
  pd->privdata.sonymd.sharp = stp_get_int_parameter(v, "Sharpen");

  if (!strcmp(gamma, "Hard")) {
    pd->privdata.sonymd.gamma = 0x03;
  } else if (!strcmp(gamma, "Normal")) {
    pd->privdata.sonymd.gamma = 0x02;
  } else if (!strcmp(gamma, "Soft")) {
    pd->privdata.sonymd.gamma = 0x01;
  } else if (!strcmp(gamma, "Softer")) {
    pd->privdata.sonymd.gamma = 0x04;
  } else{
    pd->privdata.sonymd.gamma = 0x00;
  }
}

/* Sony UP-D898 family */
static void sony_upd898_printer_init_func(stp_vars_t *v)
{
  char hdrbuf[256];
  char buf[256];

  dyesub_privdata_t *pd = get_privdata(v);

  int  hdrlen;

  /* Generate PJL header */
  memset(buf, 0, sizeof(buf));
  hdrlen = snprintf(buf, sizeof(buf),
		    "\x1b%%-12345X\r\n"
		    "@PJL JOB NAME=\"Gutenprint\" \r\n"
		    "@PJL ENTER LANGUAGE=SONY-PDL-DS2\r\n");
  buf[255] = 0;

  /* Generate block header */
  memset(hdrbuf, 0, sizeof(hdrbuf));
  snprintf(hdrbuf, sizeof(hdrbuf), "JOBSIZE=PJL-H,%d,%s,6,0,0,0",
	   hdrlen, pd->pagesize);

  /* Write block header */
  stp_zfwrite(hdrbuf, 1, sizeof(hdrbuf), v);
  /* Write PJL header */
  stp_zfwrite(buf, 1, hdrlen, v);

  /* Generate payload header */
  hdrlen = pd->w_size * pd->h_size + 274 + 23;
  memset(hdrbuf, 0, sizeof(hdrbuf));
  snprintf(hdrbuf, sizeof(hdrbuf), "JOBSIZE=PDL,%d",
	   hdrlen);
  /* Write block header */
  stp_zfwrite(hdrbuf, 1, sizeof(hdrbuf), v);

  /* Write 274 bytes of payload header */
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x10, v);
  stp_putc(0x0f, v);
  stp_putc(0x00, v);
  stp_putc(0x1c, v);
  dyesub_nputc(v, 0, 7);

  dyesub_nputc(v, 0, 7);
  stp_putc(0x01, v);
  stp_putc(0x02, v);
  stp_putc(0x00, v);
  stp_putc(0x09, v);
  stp_putc(0x00, v);
  stp_putc(pd->copies, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x11, v);

  stp_putc(0x01, v);
  stp_putc(0x08, v);
  stp_putc(0x00, v);
  stp_putc(0x1a, v);
  dyesub_nputc(v, 0, 4);
  stp_put16_be(pd->w_size, v);  // fixed at 0x500/1280
  stp_put16_be(pd->h_size, v);
  stp_putc(0x09, v);
  stp_putc(0x00, v);
  stp_putc(0x28, v);
  stp_putc(0x01, v);

  stp_putc(0x10, v);
  stp_putc(0xd4, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x03, v);
  stp_putc(0x58, v);
  stp_put16_be(pd->h_size, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x13, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x04, v);
  stp_putc(0x00, v);
  stp_putc(0x80, v);

  stp_putc(0x00, v);
  stp_putc(0x23, v);
  stp_putc(0x00, v);
  stp_putc(0x0c, v);
  stp_putc(0x01, v);
  stp_putc(0x09, v);
  stp_put16_be(pd->w_size, v);  // fixed at 0x500/1280
  stp_put16_be(pd->h_size, v);
  dyesub_nputc(v, 0, 4);
  stp_putc(0x08, v);
  stp_putc(0xff, v);

  stp_putc(0x08, v);
  stp_putc(0x00, v);
  stp_putc(0x19, v);
  dyesub_nputc(v, 0, 4);
  stp_put16_be(pd->w_size, v);  // fixed at 0x500/1280
  stp_put16_be(pd->h_size, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x81, v);
  stp_putc(0x80, v);
  stp_putc(0x00, v);

  stp_putc(0x8f, v);
  stp_putc(0x00, v);
  stp_putc(0xb8, v);
  dyesub_nputc(v, 0, 13);

  dyesub_nputc(v, 0, 16 * 9);

  dyesub_nputc(v, 0, 11);
  stp_putc(0xc0, v);
  stp_putc(0x00, v);
  stp_putc(0x82, v);
  stp_put32_be(pd->w_size* pd->h_size, v);
}

static void sony_updneo_printer_end_func(stp_vars_t *v)
{
  /* write post-payload trailing stuff. */
  dyesub_nputc(v, '\xff', 16);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x14, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x12, v);
  stp_putc(0x00, v);

  /* Write block header */
  stp_zfwrite("JOBSIZE=PJL-T,302", 1, 17, v);
  dyesub_nputc(v, 0, 256-17);

  /* Write block */
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x8f, v);
  stp_putc(0x01, v);
  stp_putc(0x11, v);
  dyesub_nputc(v, 0, 276);

  /* And finally, the PJL footer */
  stp_zfwrite("@PJL EOJ\r\n\\x1b%%-12345X\r\n", 1, 21, v);
}

/* Sony UP-CR20 family */
static const dyesub_pagesize_t upcr20_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1382,334), PT1(2048,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1210,334), PT1(1728,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1728,334), PT1(2380,334), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w360h504-div2", "3.5x5*2", PT1(1728,334), PT1(2420,334), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(2048,334), PT1(2724,334), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576-div2", "4x6*2", PT1(2048,334), PT1(2764,334), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, upcr20_page_list, dyesub_pagesize_t, upcr20_page);

static const dyesub_printsize_t upcr20_printsize[] =
{
  { "334x334", "w288h432", 1382, 2048},
  { "334x334", "B7", 1210, 1728},
  { "334x334", "w360h504", 1728, 2380},
  { "334x334", "w360h504-div2", 1728, 2420},
  { "334x334", "w432h576", 2048, 2724},
  { "334x334", "w432h576-div2", 2048, 2764},
};

LIST(dyesub_printsize_list_t, upcr20_printsize_list, dyesub_printsize_t, upcr20_printsize);

static const overcoat_t upcr20_overcoat[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x01"}},
  {"Matte",   N_("Matte"),   {1, "\x03"}},
};

LIST(overcoat_list_t, upcr20_overcoat_list, overcoat_t, upcr20_overcoat);

static void sony_upcr20_printer_init_func(stp_vars_t *v)
{
  char hdrbuf[256];
  char buf[256];

  dyesub_privdata_t *pd = get_privdata(v);

  int  hdrlen;

  /* Generate PJL header */
  memset(buf, 0, sizeof(buf));
  hdrlen = snprintf(buf, sizeof(buf),
		    "\x1b%%-12345X\r\n"
		    "@PJL JOB NAME=\"Gutenprint\" \r\n"
		    "@PJL ENTER LANGUAGE=SONY-PDL-DS2\r\n");
  buf[255] = 0;

  /* Generate block header */
  memset(hdrbuf, 0, sizeof(hdrbuf));
  snprintf(hdrbuf, sizeof(hdrbuf), "JOBSIZE=PJL-H,%d,%s,64,0,0,0",
	   hdrlen, pd->pagesize);

  /* Write block header */
  stp_zfwrite(hdrbuf, 1, sizeof(hdrbuf), v);
  /* Write PJL header */
  stp_zfwrite(buf, 1, hdrlen, v);

  /* Generate payload header */
  hdrlen = pd->w_size * pd->h_size * 3 + 274 + 23;
  memset(hdrbuf, 0, sizeof(hdrbuf));
  snprintf(hdrbuf, sizeof(hdrbuf), "JOBSIZE=PDL,%d",
	   hdrlen);
  /* Write block header */
  stp_zfwrite(hdrbuf, 1, sizeof(hdrbuf), v);

  char pg = 0;
  if (strcmp(pd->pagesize,"B7") == 0)
    pg = 0x40;
  else if (strcmp(pd->pagesize,"w288h432") == 0)
    pg = 0x48;
  else if (strcmp(pd->pagesize,"w360h504") == 0)
    pg = 0x41;
  else if (strcmp(pd->pagesize,"w360h504-div2") == 0)
    pg = 0x41;
  else if (strcmp(pd->pagesize,"w432h576") == 0)
    pg = 0x49;
  else if (strcmp(pd->pagesize,"w432h576-div2") == 0)
    pg = 0x49;

  /* Write 274 bytes of payload header */
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x10, v);
  stp_putc(0x0f, v);
  stp_putc(0x00, v);
  stp_putc(0x1c, v);
  dyesub_nputc(v, 0, 7);

  dyesub_nputc(v, 0, 4);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x02, v);
  stp_putc(0x00, v);
  stp_putc(0x16, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x02, v);
  stp_putc(0x00, v);
  stp_putc(0x09, v);

  stp_putc(0x00, v);
  stp_putc(pd->copies, v);
  stp_putc(0x02, v);
  stp_putc(0x00, v);
  stp_putc(0x06, v);
  stp_putc(0x01, v);
  stp_zfwrite((pd->overcoat->seq).data, 1,
	      (pd->overcoat->seq).bytes, v); /*overcoat pattern, 1 byte */
  stp_putc(0x03, v);
  stp_putc(0x00, v);
  stp_putc(0x1d, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  if (strcmp(pd->pagesize,"w360h504-div2") == 0 ||
      strcmp(pd->pagesize,"w432h576-div2") == 0) {
    stp_putc(0x03, v);
    stp_putc(0x00, v);
    stp_putc(0x1e, v);
    stp_putc(0x00, v);
    stp_putc(0x01, v);
    stp_putc(0x02, v);
  }
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x20, v);

  stp_putc(0x01, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x27, v);
  stp_putc(pg, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x11, v);
  stp_putc(0x01, v);
  stp_putc(0x08, v);
  stp_putc(0x00, v);
  stp_putc(0x1a, v);
  dyesub_nputc(v, 0, 4);

  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x13, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x04, v);
  stp_putc(0x00, v);
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x23, v);
  stp_putc(0x00, v);
  stp_putc(0x10, v);

  stp_putc(0x03, v);
  stp_putc(0x00, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  dyesub_nputc(v, 0, 4);
  stp_putc(0x08, v);
  stp_putc(0x08, v);
  stp_putc(0x08, v);
  stp_putc(0xff, v);
  stp_putc(0xff, v);
  stp_putc(0xff, v);

  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x17, v);
  stp_putc(0x00, v);
  stp_putc(0x08, v);
  stp_putc(0x00, v);
  stp_putc(0x19, v);
  dyesub_nputc(v, 0, 4);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_putc(0x00, v);

  stp_putc(0x00, v);
  stp_putc(0x81, v);
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x8f, v);
  stp_putc(0x00, v);
  if (strcmp(pd->pagesize,"w360h504-div2") == 0 ||
      strcmp(pd->pagesize,"w432h576-div2") == 0) {
    stp_putc(0x9e, v);
    dyesub_nputc(v, 0, 3);
  } else {
    stp_putc(0xa4, v);
    dyesub_nputc(v, 0, 9);
  }

  dyesub_nputc(v, 0, 16 * 8);

  dyesub_nputc(v, 0, 11);
  stp_putc(0xc0, v);
  stp_putc(0x00, v);
  stp_putc(0x82, v);
  stp_put32_be(pd->w_size* pd->h_size * 3, v);
}

/* Sony UP-DR80 family */
static const dyesub_pagesize_t updr80md_page[] =
{
  DEFINE_PAPER_SIMPLE( "A4", "A4", PT1(2392,301), PT1(3400,301), DYESUB_PORTRAIT), /* Letter */
  DEFINE_PAPER_SIMPLE( "Letter", "Letter", PT1(2464,301), PT1(3192,301), DYESUB_PORTRAIT), /* A4 */
};

LIST(dyesub_pagesize_list_t, updr80md_page_list, dyesub_pagesize_t, updr80md_page);

static const dyesub_printsize_t updr80md_printsize[] =
{
  { "301x301", "A4", 2392, 3400},
  { "301x301", "Letter", 2464, 3192},
};

LIST(dyesub_printsize_list_t, updr80md_printsize_list, dyesub_printsize_t, updr80md_printsize);

static void sony_updr80md_printer_init_func(stp_vars_t *v)
{
  char hdrbuf[256];
  char buf[256];

  dyesub_privdata_t *pd = get_privdata(v);

  int  hdrlen;

  /* Generate PJL header */
  memset(buf, 0, sizeof(buf));
  hdrlen = snprintf(buf, sizeof(buf),
		    "\x1b%%-12345X\r\n"
		    "@PJL JOB NAME=\"Gutenprint\" \r\n"
		    "@PJL ENTER LANGUAGE=SONY-PDL-DS2\r\n");
  buf[255] = 0;

  /* Generate block header */
  memset(hdrbuf, 0, sizeof(hdrbuf));
  snprintf(hdrbuf, sizeof(hdrbuf), "JOBSIZE=PJL-H,%d,%s,4,0,0,0",
	   hdrlen, pd->pagesize);

  /* Write block header */
  stp_zfwrite(hdrbuf, 1, sizeof(hdrbuf), v);
  /* Write PJL header */
  stp_zfwrite(buf, 1, hdrlen, v);

  /* Generate payload header */
  hdrlen = pd->w_size * pd->h_size * 3 + 296 + 23;
  memset(hdrbuf, 0, sizeof(hdrbuf));
  snprintf(hdrbuf, sizeof(hdrbuf), "JOBSIZE=PDL,%d",
	   hdrlen);
  /* Write block header */
  stp_zfwrite(hdrbuf, 1, sizeof(hdrbuf), v);

  char pg = 0;
  if (strcmp(pd->pagesize,"Letter") == 0)
    pg = 0x00;
  else if (strcmp(pd->pagesize,"A4") == 0)
    pg = 0x56;

  /* Write 296 bytes of payload header */
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x10, v);
  stp_putc(0x0f, v);
  stp_putc(0x00, v);
  stp_putc(0x1c, v);
  dyesub_nputc(v, 0, 7);

  dyesub_nputc(v, 0, 7);
  stp_putc(pg, v);
  stp_putc(0x02, v);
  stp_putc(0x00, v);
  stp_putc(0x16, v);
  stp_putc(0x00, v);
  stp_putc(0x01, v);
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x15, v);

  stp_putc(0x00, v);
  stp_putc(0x12, v);
  stp_putc(0x55, v);
  stp_putc(0x50, v);
  stp_putc(0x44, v);
  stp_putc(0x52, v);
  stp_putc(0x38, v);
  stp_putc(0x30, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x4c, v);
  stp_putc(0x55, v);
  stp_putc(0x54, v);
  stp_putc(0x30, v);    // XXX 30 LUT0, 2f NO LUT
  stp_putc(0x00, v);
  stp_putc(0x00, v);

  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);    // 00 XXX LUT0, ff NO LUT
  stp_putc(0x02, v);
  stp_putc(0x00, v);
  stp_putc(0x09, v);
  stp_putc(0x00, v);
  stp_putc(pd->copies, v);
  stp_putc(0x02, v);
  stp_putc(0x00, v);
  stp_putc(0x06, v);
  stp_putc(0x01, v);
  stp_putc(0x03, v);
  stp_putc(0x04, v);
  stp_putc(0x00, v);

  stp_putc(0x1d, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x05, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x20, v);
  stp_putc(0x00, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x11, v);
  stp_putc(0x01, v);
  stp_putc(0x08, v);
  stp_putc(0x00, v);
  stp_putc(0x1a, v);

  dyesub_nputc(v, 0, 4);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x13, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x04, v);
  stp_putc(0x00, v);
  stp_putc(0x80, v);

  stp_putc(0x00, v);
  stp_putc(0x23, v);
  stp_putc(0x00, v);
  stp_putc(0x10, v);
  stp_putc(0x03, v);
  stp_putc(0x00, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  dyesub_nputc(v, 0, 4);
  stp_putc(0x08, v);
  stp_putc(0x08, v);

  stp_putc(0x08, v);
  stp_putc(0xff, v);
  stp_putc(0xff, v);
  stp_putc(0xff, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x17, v);
  stp_putc(0x00, v);
  stp_putc(0x08, v);
  stp_putc(0x00, v);
  stp_putc(0x19, v);
  dyesub_nputc(v, 0, 4);

  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x81, v);
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x8f, v);
  stp_putc(0x00, v);
  stp_putc(0xa6, v);
  dyesub_nputc(v, 0, 5);

  dyesub_nputc(v, 0, 16 * 10);

  stp_putc(0x00, v);
  stp_putc(0xc0, v);
  stp_putc(0x00, v);
  stp_putc(0x82, v);
  stp_put32_be(pd->w_size* pd->h_size * 3, v);
}

static void sony_updr80md_printer_end_func(stp_vars_t *v)
{
  /* write post-payload trailing stuff. */
//  dyesub_nputc(v, '\xff', 16);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x14, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x12, v);
  stp_putc(0x00, v);

  /* Write block header */
  stp_zfwrite("JOBSIZE=PJL-T,302", 1, 17, v);
  dyesub_nputc(v, 0, 256-17);

  /* Write block */
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x8f, v);
  stp_putc(0x01, v);
  stp_putc(0x11, v);
  dyesub_nputc(v, 0, 276);

  /* And finally, the PJL footer */
  stp_zfwrite("@PJL EOJ\r\n\\x1b%%-12345X\r\n", 1, 21, v);
}

/* Fujifilm CX-400 */
static const dyesub_pagesize_t cx400_page[] =
{
  DEFINE_PAPER( "w288h432", "4x6", PT1(1268,310), PT1(1658,310), 24, 24, 23, 22, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w288h387", "4x5 3/8", PT1(1268,310), PT1(1842,310), 24, 24, 23, 23, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w288h504", "4x7", PT1(1268,310), PT1(2208,310), 24, 24, 23, 22, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, cx400_page_list, dyesub_pagesize_t, cx400_page);

static const dyesub_printsize_t cx400_printsize[] =
{
  { "310x310", "w288h387", 1268, 1658},
  { "310x310", "w288h432", 1268, 1842},
  { "310x310", "w288h504", 1268, 2208},
};

LIST(dyesub_printsize_list_t, cx400_printsize_list, dyesub_printsize_t, cx400_printsize);

static void cx400_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg = '\0';
  const char *pname = "XXXXXX";

  stp_dprintf(STP_DBG_DYESUB, v,
	"dyesub: fuji driver %s\n", stp_get_driver(v));
  if (strcmp(stp_get_driver(v),"fujifilm-cx400") == 0)
    pname = "NX1000";
  else if (strcmp(stp_get_driver(v),"fujifilm-cx550") == 0)
    pname = "QX200\0";

  stp_zfwrite("FUJIFILM", 1, 8, v);
  stp_zfwrite(pname, 1, 6, v);
  stp_putc('\0', v);
  stp_put16_le(pd->w_size, v);
  stp_put16_le(pd->h_size, v);
  if (strcmp(pd->pagesize,"w288h504") == 0)
    pg = '\x0d';
  else if (strcmp(pd->pagesize,"w288h432") == 0)
    pg = '\x0c';
  else if (strcmp(pd->pagesize,"w288h387") == 0)
    pg = '\x0b';
  stp_putc(pg, v);
  stp_zfwrite("\x00\x00\x00\x00\x00\x01\x00\x01\x00\x00\x00\x00"
		  "\x00\x00\x2d\x00\x00\x00\x00", 1, 19, v);
  stp_zfwrite("FUJIFILM", 1, 8, v);
  stp_zfwrite(pname, 1, 6, v);
  stp_putc('\1', v);
}

/* Fujifilm NX-500 */
static const dyesub_resolution_t res_306dpi[] =
{
  { "306x306", 306, 306},
};

LIST(dyesub_resolution_list_t, res_306dpi_list, dyesub_resolution_t, res_306dpi);

static const dyesub_pagesize_t nx500_page[] =
{
  DEFINE_PAPER( "Postcard", "Postcard", PT1(1024,306), PT1(1518,306), 21, 21, 29, 29, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, nx500_page_list, dyesub_pagesize_t, nx500_page);

static const dyesub_printsize_t nx500_printsize[] =
{
  { "306x306", "Postcard", 1024, 1518},
};

LIST(dyesub_printsize_list_t, nx500_printsize_list, dyesub_printsize_t, nx500_printsize);

static void nx500_printer_init_func(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("INFO-QX-20--MKS\x00\x00\x00M\x00W\00A\x00R\00E", 1, 27, v);
  dyesub_nputc(v, '\0', 21);
  stp_zfwrite("\x80\x00\x02", 1, 3, v);
  dyesub_nputc(v, '\0', 20);
  stp_zfwrite("\x02\x01\x01", 1, 3, v);
  dyesub_nputc(v, '\0', 2);
  stp_put16_le(pd->h_size, v);
  stp_put16_le(pd->w_size, v);
  stp_zfwrite("\x00\x02\x00\x70\x2f", 1, 5, v);
  dyesub_nputc(v, '\0', 43);
}


/* Kodak Easyshare Dock family */
static const dyesub_pagesize_t kodak_dock_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1248,300), PT1(1856,300), DYESUB_PORTRAIT), /* 4x6 */
};

LIST(dyesub_pagesize_list_t, kodak_dock_page_list, dyesub_pagesize_t, kodak_dock_page);

static const dyesub_printsize_t kodak_dock_printsize[] =
{
  { "300x300", "w288h432", 1248, 1856},
};

LIST(dyesub_printsize_list_t, kodak_dock_printsize_list, dyesub_printsize_t, kodak_dock_printsize);

static void kodak_dock_printer_init(stp_vars_t *v)
{
  stp_put16_be(0x3000, v);
  dyesub_nputc(v, '\0', 10);
}

static void kodak_dock_plane_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_put16_be(0x3001, v);
  stp_put16_le(3 - pd->plane, v);
  stp_put32_le(pd->w_size*pd->h_size, v);
  dyesub_nputc(v, '\0', 4);
}

/* Kodak 6800 */
static const dyesub_pagesize_t kodak_6800_page[] =
{
  DEFINE_PAPER_SIMPLE( "w144h432", "2x6", PT1(636,300), PT1(1844,300),	DYESUB_LANDSCAPE), /* 2x6 */
  DEFINE_PAPER_SIMPLE( "w216h432", "3x6", PT1(936,300), PT1(1844,300),	DYESUB_LANDSCAPE), /* 3x6 */
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1240,300), PT1(1844,300), DYESUB_LANDSCAPE), /* 4x6 */
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1282,300), PT1(1844,300), DYESUB_LANDSCAPE), /* 4x6-div2 */
  DEFINE_PAPER_SIMPLE( "w432h432", "6x6", PT1(1836,300), PT1(1844,300), DYESUB_LANDSCAPE), /* 6x6 */
  DEFINE_PAPER_SIMPLE( "w432h432-div2", "3x6*2", PT1(1844,300), PT1(1882,300), DYESUB_PORTRAIT), /* 3x6*2 */
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1844,300), PT1(2434,300), DYESUB_PORTRAIT), /* 6x8 */
  DEFINE_PAPER_SIMPLE( "w432h576-div2", "4x6 *2", PT1(1844,300), PT1(2490,300), DYESUB_PORTRAIT), /* 6x8-div2 */
};

LIST(dyesub_pagesize_list_t, kodak_6800_page_list, dyesub_pagesize_t, kodak_6800_page);

static const dyesub_printsize_t kodak_6800_printsize[] =
{
  { "300x300", "w144h432", 636, 1844},
  { "300x300", "w216h432", 936, 1844},
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "w288h432-div2", 1282, 1844},
  { "300x300", "w432h432", 1836, 1844},
  { "300x300", "w432h432-div2", 1844, 1882},
  { "300x300", "w432h576", 1844, 2434},
  { "300x300", "w432h576-div2", 1844, 2490},
};

LIST(dyesub_printsize_list_t, kodak_6800_printsize_list, dyesub_printsize_t, kodak_6800_printsize);

static const overcoat_t kodak_6800_overcoat[] =
{
  {"Coated", N_("Coated"), {1, "\x01"}},
  {"None",  N_("None"),  {1, "\x00"}},
};

LIST(overcoat_list_t, kodak_6800_overcoat_list, overcoat_t, kodak_6800_overcoat);

/* Kodak 6850 */
static const dyesub_pagesize_t kodak_6850_page[] =
{
  DEFINE_PAPER_SIMPLE( "w144h432", "2x6", PT1(636,300), PT1(1844,300),	DYESUB_LANDSCAPE), /* 2x6 */
  DEFINE_PAPER_SIMPLE( "w216h432", "3x6", PT1(936,300), PT1(1844,300),	DYESUB_LANDSCAPE), /* 3x6 */
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1240,300), PT1(1844,300), DYESUB_LANDSCAPE), /* 4x6 */
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1282,300), PT1(1844,300), DYESUB_LANDSCAPE), /* 4x6-div2 */
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1548,300), PT1(2140,300), DYESUB_PORTRAIT), /* 5x7 */
  DEFINE_PAPER_SIMPLE( "w432h432", "6x6", PT1(1836,300), PT1(1844,300), DYESUB_LANDSCAPE), /* 6x6 */
  DEFINE_PAPER_SIMPLE( "w432h432-div2", "3x6*2", PT1(1844,300), PT1(1882,300), DYESUB_PORTRAIT), /* 3x6*2 */
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1844,300), PT1(2434,300), DYESUB_PORTRAIT), /* 6x8 */
  DEFINE_PAPER_SIMPLE( "w432h576-div2", "4x6 *2", PT1(1844,300), PT1(2490,300), DYESUB_PORTRAIT), /* 6x8-div2 */
};

LIST(dyesub_pagesize_list_t, kodak_6850_page_list, dyesub_pagesize_t, kodak_6850_page);

static const dyesub_printsize_t kodak_6850_printsize[] =
{
  { "300x300", "w144h432", 636, 1844},
  { "300x300", "w216h432", 936, 1844},
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "w288h432-div2", 1282, 1844},
  { "300x300", "w360h504", 1548, 2140},
  { "300x300", "w432h432", 1836, 1844},
  { "300x300", "w432h432-div2", 1844, 1882},
  { "300x300", "w432h576", 1844, 2434},
  { "300x300", "w432h576-div2", 1844, 2490},
};

LIST(dyesub_printsize_list_t, kodak_6850_printsize_list, dyesub_printsize_t, kodak_6850_printsize);

static unsigned short short_to_packed_bcd(unsigned short val)
{
  unsigned short bcd;
  unsigned short i;

  /* Handle from 0-9999 */
  i = val % 10;
  bcd = i;
  val /= 10;
  i = val % 10;
  bcd |= (i << 4);
  val /= 10;
  i = val % 10;
  bcd |= (i << 8);
  val /= 10;
  i = val % 10;
  bcd |= (i << 12);

  return bcd;
}

static void kodak_68xx_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("\x03\x1b\x43\x48\x43\x0a\x00\x01", 1, 8, v);
  stp_put16_be(short_to_packed_bcd(pd->copies), v); /* Number of copies in BCD */
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);

  if (!strcmp(pd->pagesize,"w360h504"))
	  stp_putc(0x07, v); /* All 5" sizes */
  else
	  stp_putc(0x06, v); /* All others are 6" sizes */

  stp_zfwrite((pd->overcoat->seq).data, 1,
			(pd->overcoat->seq).bytes, v);

  if (!strcmp(pd->pagesize,"w360h504"))
	  stp_putc(0x00, v);
  else if (!strcmp(pd->pagesize,"w144h432"))
	  stp_putc(0x21, v);
  else if (!strcmp(pd->pagesize,"w216h432"))
	  stp_putc(0x23, v);
  else if (!strcmp(pd->pagesize,"w288h432"))
	  stp_putc(0x01, v);
  else if (!strcmp(pd->pagesize,"w288h432-div2"))
	  stp_putc(0x20, v);
  else if (!strcmp(pd->pagesize,"w432h432"))
	  stp_putc(0x00, v);
  else if (!strcmp(pd->pagesize,"w432h432-div2"))
	  stp_putc(0x00, v);
  else if (!strcmp(pd->pagesize,"w432h576"))
	  stp_putc(0x00, v);
  else if (!strcmp(pd->pagesize,"w432h576-div2"))
	  stp_putc(0x02, v);
  else
	  stp_putc(0x00, v); /* Catch-all, just in case */

}

/* Kodak 605 */
static const dyesub_pagesize_t kodak_605_page[] =
{
  DEFINE_PAPER_SIMPLE( "w144h432", "2x6", PT1(636,300), PT1(1844,300),	DYESUB_LANDSCAPE), /* 2x6 */
  DEFINE_PAPER_SIMPLE( "w216h432", "3x6", PT1(936,300), PT1(1844,300),	DYESUB_LANDSCAPE), /* 3x6 */
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1240,300), PT1(1844,300), DYESUB_LANDSCAPE), /* 4x6 */
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1500,300), PT1(2100,300), DYESUB_PORTRAIT), /* 5x7 */
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1844,300), PT1(2434,300), DYESUB_PORTRAIT), /* 6x8 */
};

LIST(dyesub_pagesize_list_t, kodak_605_page_list, dyesub_pagesize_t, kodak_605_page);

static const dyesub_printsize_t kodak_605_printsize[] =
{
  { "300x300", "w144h432", 636, 1844},
  { "300x300", "w216h432", 936, 1844},
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "w360h504", 1500, 2100},
  { "300x300", "w432h576", 1844, 2434},
};

LIST(dyesub_printsize_list_t, kodak_605_printsize_list, dyesub_printsize_t, kodak_605_printsize);

static void kodak_605_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("\x01\x40\x0a\x00\x01", 1, 5, v);
  stp_put16_be(short_to_packed_bcd(pd->copies), v); /* Number of copies in BCD */
  stp_put16_le(pd->w_size, v);
  stp_put16_le(pd->h_size, v);

  if (!strcmp(pd->pagesize,"w144h432"))
	  stp_putc(0x12, v);
  else if (!strcmp(pd->pagesize,"w216h432"))
	  stp_putc(0x14, v);
  else if (!strcmp(pd->pagesize,"w288h432"))
	  stp_putc(0x01, v);
  else if (!strcmp(pd->pagesize,"w432h576"))
	  stp_putc(0x03, v);
  else if (!strcmp(pd->pagesize,"w360h504"))
	  stp_putc(0x02, v);
  else
	  stp_putc(0x01, v); /* Just in case */

  stp_zfwrite((pd->overcoat->seq).data, 1,
			(pd->overcoat->seq).bytes, v);
  stp_putc(0x00, v);
}

static const overcoat_t kodak_605_overcoat[] =
{
  {"Coated", N_("Coated"), {1, "\x02"}},
  {"None",  N_("None"),  {1, "\x01"}},
};

LIST(overcoat_list_t, kodak_605_overcoat_list, overcoat_t, kodak_605_overcoat);

/* Kodak 1400 */
static const dyesub_resolution_t res_301dpi[] =
{
  { "301x301", 301, 301},
};

LIST(dyesub_resolution_list_t, res_301dpi_list, dyesub_resolution_t, res_301dpi);

static const dyesub_pagesize_t kodak_1400_page[] =
{
  /* Printer has 1" non-printable area on top and bottom of page, not part of
     data sent over.

     Printer requires full-bleed data horizontally. However, not all pixels
     are actually printed.  35+35 (8x14 paper) or 76+76 (8x12 paper) are
     effectively discarded (ie ~0.125" and ~0.250" respectively).

     The printer can technically print a little wider but these dimensions are
     defined by the lamination area, which is fixed.
  */
  DEFINE_PAPER( "w612h864", "8.5x12", PT1(2560,301), PT(3010,301)+72*2, PT1(76,301), PT(76,301), 72, 72, DYESUB_PORTRAIT), /* 8x12 */
  DEFINE_PAPER( "Legal", "8.5x14", PT1(2560,301), PT(3612,301)+72*2, PT1(35,301), PT1(35,301), 72, 72, DYESUB_PORTRAIT), /* 8x14 */
  DEFINE_PAPER( "A4", "A4",       PT1(2560,301), PT(3010,301)+72*2, PT1(76,301), PT(76,301), 0, 0, DYESUB_PORTRAIT), /* A4, identical to 8x12 */
};

LIST(dyesub_pagesize_list_t, kodak_1400_page_list, dyesub_pagesize_t, kodak_1400_page);

static const dyesub_media_t kodak_1400_media[] =
{
  { "Glossy", N_("Glossy"), {2, "\x00\x3c"}},
  { "Matte+5",  N_("Matte +5"),  {2, "\x01\x28"}},
  { "Matte+4",  N_("Matte +4"),  {2, "\x01\x2e"}},
  { "Matte+3",  N_("Matte +3"),  {2, "\x01\x34"}},
  { "Matte+2",  N_("Matte +2"),  {2, "\x01\x3a"}},
  { "Matte+1",  N_("Matte +1"),  {2, "\x01\x40"}},
  { "Matte",    N_("Matte"),     {2, "\x01\x46"}},
  { "Matte-1",  N_("Matte -1"),  {2, "\x01\x52"}},
  { "Matte-2",  N_("Matte -2"),  {2, "\x01\x5e"}},
  { "Matte-3",  N_("Matte -3"),  {2, "\x01\x6a"}},
  { "Matte-4",  N_("Matte -4"),  {2, "\x01\x76"}},
  { "Matte-5",  N_("Matte -5"),  {2, "\x01\x82"}},
};
LIST(dyesub_media_list_t, kodak_1400_media_list, dyesub_media_t, kodak_1400_media);

static const dyesub_printsize_t kodak_1400_printsize[] =
{
  { "301x301", "w612h864", 2560, 3010},
  { "301x301", "Legal", 2560, 3612},
  { "301x301", "A4", 2560, 3010},
};

LIST(dyesub_printsize_list_t, kodak_1400_printsize_list, dyesub_printsize_t, kodak_1400_printsize);

static void kodak_1400_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("PGHD", 1, 4, v);
  stp_put16_le(pd->w_size, v);
  dyesub_nputc(v, 0x00, 2);
  stp_put16_le(pd->h_size, v);
  dyesub_nputc(v, 0x00, 2);
  stp_put32_le(pd->h_size*pd->w_size, v);
  dyesub_nputc(v, 0x00, 4);
  stp_zfwrite((pd->media->seq).data, 1, 1, v);  /* Matte or Glossy? */
  stp_zfwrite((pd->overcoat->seq).data, 1,
			(pd->overcoat->seq).bytes, v);
  stp_putc(0x01, v);
  stp_zfwrite((const char*)((pd->media->seq).data) + 1, 1, 1, v); /* Lamination intensity */
  dyesub_nputc(v, 0x00, 12);
}

/* Kodak 805 */
static const dyesub_pagesize_t kodak_805_page[] =
{
  /* Identical to the Kodak 1400 except for the lack of A4 support.
     See the 1400 comments for explanations of this. */
  DEFINE_PAPER( "w612h864", "8.5x12", PT1(2560,301), PT(3010,301)+72*2, PT1(76,301), PT(76,301), 72, 72, DYESUB_PORTRAIT), /* 8x12 */
  DEFINE_PAPER( "Legal", "8.5x14", PT1(2560,301), PT(3612,301)+72*2, PT1(35,301), PT1(35,301), 72, 72, DYESUB_PORTRAIT), /* 8x14 */
};

LIST(dyesub_pagesize_list_t, kodak_805_page_list, dyesub_pagesize_t, kodak_805_page);

static const dyesub_printsize_t kodak_805_printsize[] =
{
  { "301x301", "w612h864", 2560, 3010},
  { "301x301", "Legal", 2560, 3612},
};

LIST(dyesub_printsize_list_t, kodak_805_printsize_list, dyesub_printsize_t, kodak_805_printsize);

static void kodak_805_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("PGHD", 1, 4, v);
  stp_put16_le(pd->w_size, v);
  dyesub_nputc(v, 0x00, 2);
  stp_put16_le(pd->h_size, v);
  dyesub_nputc(v, 0x00, 2);
  stp_put32_le(pd->h_size*pd->w_size, v);
  dyesub_nputc(v, 0x00, 5);
  stp_zfwrite((pd->overcoat->seq).data, 1,
			(pd->overcoat->seq).bytes, v);
  stp_putc(0x01, v);
  stp_putc(0x3c, v); /* Lamination intensity; fixed on glossy media */
  dyesub_nputc(v, 0x00, 12);
}

/* Kodak 9810 / 8800 */
static const dyesub_pagesize_t kodak_9810_page[] =
{
  DEFINE_PAPER_SIMPLE( "c8x10", "8x10", PT1(2464,300), PT1(3024,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w576h864", "8x12", PT1(2464,300), PT1(3624,300), DYESUB_PORTRAIT),
};
LIST(dyesub_pagesize_list_t, kodak_9810_page_list, dyesub_pagesize_t, kodak_9810_page);

static const dyesub_printsize_t kodak_9810_printsize[] =
{
  { "300x300", "c8x10", 2464, 3024},
  { "300x300", "w576h864", 2464, 3624},
};

LIST(dyesub_printsize_list_t, kodak_9810_printsize_list, dyesub_printsize_t, kodak_9810_printsize);

static const overcoat_t kodak_9810_overcoat[] =
{
  {"Coated", N_("Coated"), {3, "\x4f\x6e\x20"}},
  {"None",  N_("None"),  {3, "\x4f\x66\x66"}},
};

LIST(overcoat_list_t, kodak_9810_overcoat_list, overcoat_t, kodak_9810_overcoat);

static const stp_parameter_t kodak_9810_parameters[] =
{
  {
    "Sharpen", N_("Image Sharpening"), "Color=No,Category=Advanced Printer Setup",
    N_("Sharpening to apply to image (0 is off, 18 is normal, 24 is max"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define kodak_9810_parameter_count (sizeof(kodak_9810_parameters) / sizeof(const stp_parameter_t))

static int
kodak_9810_load_parameters(const stp_vars_t *v, const char *name,
			   stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "Sharpen") == 0)
    {
      description->deflt.integer = 18;
      description->bounds.integer.lower = 0;
      description->bounds.integer.upper = 24;
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int kodak_9810_parse_parameters(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  /* Parse options */
  pd->privdata.k9810.sharpen = stp_get_int_parameter(v, "Sharpen");

  return 1;
}

static void kodak_9810_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

/* Command stream header */
  stp_putc(0x1b, v);
  stp_zfwrite("MndROSETTA V001.00100000020525072696E74657242696E4D6F74726C", 1, 59, v);

  /* Begin Job */
  stp_putc(0x1b, v);
  stp_zfwrite("MndBgnJob  Print   ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(8, v);
  stp_zfwrite("\x56\x30\x30\x31\x2e\x30\x30\x30", 1, 8, v);

  /* Job Definition Start */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsSrtJbDefSetup   ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(0, v);

  /* Paper selection */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsJbMkMed Name    ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(64, v);
  if (pd->h_size == 3624) {
    stp_zfwrite("YMCX 8x12 Glossy", 1, 16, v);
  } else {
    stp_zfwrite("YMCX 8x10 Glossy", 1, 16, v);
  }
  dyesub_nputc(v, 0x00, 48);

  /* Paper Selection II */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsPgMedia Name    ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(64, v);
  stp_zfwrite("\x38\x22", 1, 2, v);
  dyesub_nputc(v, 0x00, 62);

  /* Lamination */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsJbLam   ", 1, 11, v);
  stp_zfwrite((pd->overcoat->seq).data, 1,
			(pd->overcoat->seq).bytes, v);
  dyesub_nputc(v, 0x20, 5);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(0, v);

  /* Job Definition End */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsStpJbDef        ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(0, v);

  /* Begin Page */
  stp_putc(0x1b, v);
  stp_zfwrite("MndBgnLPageNormal  ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(4, v);
  stp_put32_be(1, v);

  /* Page dimensions I -- maybe this is physical media size? */
  stp_putc(0x1b, v);
  stp_zfwrite("MndSetLPage        ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(8, v);
  stp_put32_be(pd->w_size, v);
  stp_put32_be(pd->h_size, v);

  /* Page dimensions II -- maybe this is image data size? */
  stp_putc(0x1b, v);
  stp_zfwrite("MndImSpec  Size    ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(16, v);
  stp_put32_be(pd->w_size, v);
  stp_put32_be(pd->h_size, v);
  stp_put32_be(pd->w_size, v);
  stp_put32_be(0, v);

  /* Positioning within page? */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsImPositnSpecify ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(8, v);
  stp_put32_be(0, v);  /* Presumably X */
  stp_put32_be(0, v);  /* Presumably Y */

  /* Sharpening */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsImSharp SetLevel", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(2, v);
  stp_putc(0xFF, v);
  stp_putc(pd->privdata.k9810.sharpen, v);

  /* Number of Copies */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsPgCopies        ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(4, v);
  stp_put32_be(pd->copies, v);

  /* Mirroring */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsPgMirrorNone    ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(0, v);

  /* Rotation */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsPgRotateNone    ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(0, v);

  /* Cut list -- seems to be list of be16 row offsets for cuts. */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsCutList         ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(4, v);

  /* Cut at start/end of sheet */
  if (pd->h_size == 3624) {
    stp_zfwrite("\x00\x0c\x0e\x1c", 1, 4, v);
  } else {
    stp_zfwrite("\x00\x0c\x0b\xc4", 1, 4, v);
  }

#if 0  /* Additional Known Cut lists */
  /* Single cut, down the center */
  stp_put32_be(6, v);
  if (pd->h_size == 3624) {
    stp_zfwrite("\x00\x0c\x07\x14\x0e\x1c", 1, 6, v);
  } else {
    stp_zfwrite("\x00\x0c\x05\xe8\x0b\xc4", 1, 6, v);
  }
  /* Double-Slug Cut, down the center */
  stp_put32_be(8, v);
  if (pd->h_size == 3624) {
    stp_zfwrite("\x00\x0c\x07\x01\x07\x27\x0e\x1c", 1, 6, v);
  } else {
    stp_zfwrite("\x00\x0c\x05\xd5\x05\xfb\x0b\xc4", 1, 6, v);
  }
#endif

}

static void kodak_9810_printer_end(stp_vars_t *v)
{
  /* End Page */
  stp_putc(0x1b, v);
  stp_zfwrite("MndEndLPage        ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(0, v);

  /* End Job */
  stp_putc(0x1b, v);
  stp_zfwrite("MndEndJob          ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(0, v);
}

static void kodak_9810_plane_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Data block */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsData    Block   ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be((pd->w_size * pd->h_size) + 8, v);
  stp_zfwrite("Image   ", 1, 8, v);
}

/* Kodak 8810 */
static const dyesub_pagesize_t kodak_8810_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h576", "8x4", PT1(1208,300), PT1(2464,300), DYESUB_LANDSCAPE),
//  DEFINE_PAPER_SIMPLE( "w360h576", "8x5", PT1(1508,300), PT1(2464,300), DYESUB_LANDSCAPE).
//  DEFINE_PAPER_SIMPLE( "w432h576", "8x6", PT1(1808,300), PT1(2464,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w576h576", "8x8", PT1(2408,300), PT1(2464,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "c8x10", "8x10", PT1(2464,300), PT1(3024,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "A4", "203x297mm", PT1(2464,300), PT1(3531,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w576h864", "8x12", PT1(2464,300), PT1(3624,300), DYESUB_PORTRAIT),
};
LIST(dyesub_pagesize_list_t, kodak_8810_page_list, dyesub_pagesize_t, kodak_8810_page);

static const dyesub_printsize_t kodak_8810_printsize[] =
{
  { "300x300", "w288h576", 1208, 2464},
//  { "300x300", "w360h576", 1508, 2464},
//  { "300x300", "w432h576", 1808, 2464},
  { "300x300", "w576h576", 2408, 2464},
  { "300x300", "c8x10", 2464, 3024},
  { "300x300", "A4", 2464, 3531},
  { "300x300", "w576h864", 2464, 3624},
};

LIST(dyesub_printsize_list_t, kodak_8810_printsize_list, dyesub_printsize_t, kodak_8810_printsize);

static const overcoat_t kodak_8810_overcoat[] =
{
  {"Glossy", N_("Glossy"), {1, "\x02"}},
  {"Satin",  N_("Satin"),  {1, "\x03"}},
};

LIST(overcoat_list_t, kodak_8810_overcoat_list, overcoat_t, kodak_8810_overcoat);

static void kodak_8810_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_putc(0x01, v);
  stp_putc(0x40, v);
  stp_putc(0x12, v);
  stp_putc(0x00, v);
  stp_putc(0x01, v);
  stp_put16_le(pd->copies, v);
  stp_put16_le(pd->w_size, v);
  stp_put16_le(pd->h_size, v);
  stp_put16_le(pd->w_size, v);
  stp_put16_le(pd->h_size, v);
  dyesub_nputc(v, 0, 4);
  stp_zfwrite((pd->overcoat->seq).data, 1,
	      (pd->overcoat->seq).bytes, v);
  stp_putc(0x00, v); /* Method -- 00 is normal, 02 is x2, 03 is x3 */
  stp_putc(0x00, v); /* Reserved */
}

/* Kodak 7000/7010 */
static const dyesub_pagesize_t kodak_7000_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1240,300), PT1(1844,300), DYESUB_LANDSCAPE), /* 4x6 */
  DEFINE_PAPER_SIMPLE( "w432h432", "6x6", PT1(1836,300), PT1(1844,300), DYESUB_LANDSCAPE), /* 4x6 */
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1844,300), PT1(2434,300), DYESUB_PORTRAIT), /* 6x8 */
};

LIST(dyesub_pagesize_list_t, kodak_7000_page_list, dyesub_pagesize_t, kodak_7000_page);

static const dyesub_printsize_t kodak_7000_printsize[] =
{
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "w432h432", 1836, 1844},
  { "300x300", "w432h576", 1844, 2434},
};

LIST(dyesub_printsize_list_t, kodak_7000_printsize_list, dyesub_printsize_t, kodak_7000_printsize);
static const overcoat_t kodak_7000_overcoat[] =
{
  {"Glossy", N_("Glossy"), {1, "\x02"}},
  {"Satin",  N_("Satin"),  {1, "\x03"}},
};

LIST(overcoat_list_t, kodak_7000_overcoat_list, overcoat_t, kodak_7000_overcoat);

static void kodak_70xx_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("\x01\x40\x0a\x00\x01", 1, 5, v);
  stp_put16_le(pd->copies, v);
  stp_put16_le(pd->w_size, v);
  stp_put16_le(pd->h_size, v);

  if (!strcmp(pd->pagesize,"w288h432"))
	  stp_putc(0x01, v);
  else if (!strcmp(pd->pagesize,"w432h432"))
	  stp_putc(0x0e, v);
  else if (!strcmp(pd->pagesize,"w432h576"))
	  stp_putc(0x03, v);
  else if (!strcmp(pd->pagesize,"w360h504"))
	  stp_putc(0x06, v);
  else
	  stp_putc(0x01, v);

  stp_zfwrite((pd->overcoat->seq).data, 1,
			(pd->overcoat->seq).bytes, v);
  stp_putc(0x00, v);
}

/* Kodak 7015 */
static const dyesub_pagesize_t kodak_7015_page[] =
{
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1548,300), PT1(2140,300), DYESUB_PORTRAIT), /* 5x7 */
};

LIST(dyesub_pagesize_list_t, kodak_7015_page_list, dyesub_pagesize_t, kodak_7015_page);

static const dyesub_printsize_t kodak_7015_printsize[] =
{
  { "300x300", "w360h504", 1548, 2140},
};

LIST(dyesub_printsize_list_t, kodak_7015_printsize_list, dyesub_printsize_t, kodak_7015_printsize);

/* Kodak Professional 8500 */
static const dyesub_pagesize_t kodak_8500_page[] =
{
  DEFINE_PAPER_SIMPLE( "w612h864", "8.5x12", PT1(2508,314), PT1(3134,314), DYESUB_PORTRAIT), /* 8.5x12 & A4 */
  DEFINE_PAPER_SIMPLE( "Letter", "8.5x11", PT1(2508,314), PT1(2954,314), DYESUB_PORTRAIT), /* Letter */
};

LIST(dyesub_pagesize_list_t, kodak_8500_page_list, dyesub_pagesize_t, kodak_8500_page);

static const dyesub_printsize_t kodak_8500_printsize[] =
{
  { "314x314", "w612h864", 2508, 3134},
  { "314x314", "Letter", 2508, 2954},
};

LIST(dyesub_printsize_list_t, kodak_8500_printsize_list, dyesub_printsize_t, kodak_8500_printsize);

static const dyesub_media_t kodak_8500_media[] =
{
  { "Glossy", N_("Glossy"), {1, "\x00"}},
  { "Matte",  N_("Matte"), {1, "\x01"}},
};
LIST(dyesub_media_list_t, kodak_8500_media_list, dyesub_media_t, kodak_8500_media);

static const overcoat_t kodak_8500_overcoat[] =
{
  {"Coated", N_("Coated"), {1, "\x00"}},
  {"None",  N_("None"),  {1, "\x02"}},
};

LIST(overcoat_list_t, kodak_8500_overcoat_list, overcoat_t, kodak_8500_overcoat);

static const stp_parameter_t kodak_8500_parameters[] =
{
  {
    "Sharpen", N_("Image Sharpening"), "Color=No,Category=Advanced Printer Setup",
    N_("Sharpening to apply to image (-5 through +5)"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MatteIntensity", N_("Matte Intensity"), "Color=No,Category=Advanced Printer Setup",
    N_("Strength of matte lamination pattern (-5 through +5)"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define kodak_8500_parameter_count (sizeof(kodak_8500_parameters) / sizeof(const stp_parameter_t))

static int
kodak_8500_load_parameters(const stp_vars_t *v, const char *name,
			   stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "Sharpen") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -5;
      description->bounds.integer.upper = 5;
      description->is_active = 1;
    }
  else if (strcmp(name, "MatteIntensity") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -5;
      description->bounds.integer.upper = 5;
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int kodak_8500_parse_parameters(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  /* Parse options */
  pd->privdata.k8500.sharpen = stp_get_int_parameter(v, "Sharpen");
  pd->privdata.k8500.matte_intensity = stp_get_int_parameter(v, "MatteIntensity");

  return 1;
}

static void kodak_8500_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

/* Start with NULL block */
  dyesub_nputc(v, 0x00, 64);
  /* Number of copies */
  stp_putc(0x1b, v);
  stp_putc(0x4e, v);
  stp_putc(pd->copies > 50 ? 50 : pd->copies, v); /* 1-50 */
  dyesub_nputc(v, 0x00, 61);
  /* Paper type.  Fixed. */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x46, v);
  stp_putc(0x00, v); /* Fixed */
  dyesub_nputc(v, 0x00, 60);
  /* Print dimensions */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x53, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  dyesub_nputc(v, 0x00, 57);
  /* Sharpening */
  stp_putc(0x1b, v);
  stp_putc(0x46, v);
  stp_putc(0x50, v);
  stp_putc(pd->privdata.k8500.sharpen, v);
  dyesub_nputc(v, 0x00, 60);
  /* Lamination */
  stp_putc(0x1b, v);
  stp_putc(0x59, v);
  if (*((const char*)((pd->overcoat->seq).data)) == 0x02) { /* No lamination */
    stp_putc(0x02, v);
    stp_putc(0x00, v);
  } else {
    stp_zfwrite((const char*)((pd->media->seq).data), 1,
		(pd->media->seq).bytes, v);
    if (*((const char*)((pd->media->seq).data)) == 0x01) { /* Matte */
      stp_putc(pd->privdata.k8500.matte_intensity, v);
    } else {
      stp_putc(0x00, v);
    }
  }
  dyesub_nputc(v, 0x00, 60);
  /* Unknown */
  stp_putc(0x1b, v);
  stp_putc(0x46, v);
  stp_putc(0x47, v);
  dyesub_nputc(v, 0x00, 61);

  /* Data header */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x54, v);
  dyesub_nputc(v, 0x00, 2);
  stp_put16_be(0, v); /* Starting row for this block */
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v); /* Number of rows in this block */
  dyesub_nputc(v, 0x00, 53);
}

static void kodak_8500_printer_end(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Pad data to 64-byte block */
  unsigned int length = pd->w_size * pd->h_size * 3;
  length %= 64;
  if (length) {
    length = 64 - length;
    dyesub_nputc(v, 0x00, length);
  }

  /* Page Footer */
  stp_putc(0x1b, v);
  stp_putc(0x50, v);
  dyesub_nputc(v, 0x00, 62);
}

/* Mitsubishi P95D/DW */
static const dyesub_resolution_t res_325dpi[] =
{
  { "325x325", 325, 325},
};

LIST(dyesub_resolution_list_t, res_325dpi_list, dyesub_resolution_t, res_325dpi);

/* All are "custom" page sizes..  bleh.. */
static const dyesub_pagesize_t mitsu_p95d_page[] =
{
  DEFINE_PAPER_SIMPLE( "w213h284", "1280x960", PT1(960,325), PT1(1280,325), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w227h284", "1280x1024", PT1(1024,325), PT1(1280,325), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w284h284", "1280x1280", PT1(1280,325), PT1(1280,325), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w284h426", "1280x1920", PT1(1280,325), PT1(1920,325), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w284h1277", "1280x5760", PT1(1280,325), PT1(5760,325), DYESUB_PORTRAIT),
  /* A true "custom" size, printer will cut at the image boundary */
  DEFINE_PAPER_SIMPLE( "Custom", "Custom", PT1(1280,325), -1, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, mitsu_p95d_page_list, dyesub_pagesize_t, mitsu_p95d_page);

static const dyesub_printsize_t mitsu_p95d_printsize[] =
{
  { "325x325", "w213h284", 960, 1280},
  { "325x325", "w227h284", 1024, 1280},
  { "325x325", "w284h284", 1280, 1280},
  { "325x325", "w284h426", 1280, 1920},
  { "325x325", "w284h1277", 1280, 5760},
  { "325x325", "Custom", 1280, 5760}, /* Maximum */
};

LIST(dyesub_printsize_list_t, mitsu_p95d_printsize_list, dyesub_printsize_t, mitsu_p95d_printsize);

static const dyesub_media_t mitsu_p95d_medias[] =
{
  {"Standard",  N_("Standard (KP61B)"), {1, "\x00"}},
  {"HighDensity", N_("High Density (KP65HM)"), {1, "\x01"}},
  {"HighGlossy", N_("High Glossy (KP91HG)"), {1, "\x02"}},
  {"HighGlossyK95HG", N_("High Glosy (K95HG)"), {1, "\x03"}},
};

LIST(dyesub_media_list_t, mitsu_p95d_media_list, dyesub_media_t, mitsu_p95d_medias);

static const dyesub_stringitem_t mitsu_p95d_gammas[] =
{
  { "Printer",      N_ ("Printer-Defined Setting") },
  { "T1", N_ ("Table 1") },
  { "T2", N_ ("Table 2") },
  { "T3", N_ ("Table 3") },
  { "T4", N_ ("Table 4") },
  { "T5", N_ ("Table 5") },
  { "LUT", N_ ("Use LUT") },
};
LIST(dyesub_stringlist_t, mitsu_p95d_gamma_list, dyesub_stringitem_t, mitsu_p95d_gammas);

static const dyesub_stringitem_t mitsu_p95d_buzzers[] =
{
  { "Off",      N_ ("Off") },
  { "Low", N_ ("Low") },
  { "High", N_ ("High") },
};
LIST(dyesub_stringlist_t, mitsu_p95d_buzzer_list, dyesub_stringitem_t, mitsu_p95d_buzzers);

static const dyesub_stringitem_t mitsu_p95d_cutters[] =
{
  { "PaperSave",      N_ ("Paper Save") },
  { "4mm", N_ ("4mm") },
  { "5mm", N_ ("5mm") },
  { "6mm", N_ ("6mm") },
  { "7mm", N_ ("7mm") },
  { "8mm", N_ ("8mm") },
};
LIST(dyesub_stringlist_t, mitsu_p95d_cutter_list, dyesub_stringitem_t, mitsu_p95d_cutters);

static const dyesub_stringitem_t mitsu_p95d_comments[] =
{
  { "Off",      N_ ("Off") },
  { "Settings", N_ ("Printer Settings") },
  { "Date", N_ ("Date") },
  { "DateTime", N_ ("Date and Time") },
};
LIST(dyesub_stringlist_t, mitsu_p95d_comment_list, dyesub_stringitem_t, mitsu_p95d_comments);

static const stp_parameter_t mitsu_p95d_parameters[] =
{
  {
    "P95Gamma", N_("Printer Gamma Correction"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Gamma Correction"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Buzzer", N_("Printer Buzzer"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Buzzer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MediaCut", N_("Media Cut Length"), "Color=No,Category=Advanced Printer Setup",
    N_("Media Cut Length"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Comment", N_("Generate Comment"), "Color=No,Category=Advanced Printer Setup",
    N_("Generate Comment"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "ClearMemory", N_("Clear Memory"), "Color=No,Category=Advanced Printer Setup",
    N_("Clear Memory"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "ContinuousPrint", N_("Continuous Printing"), "Color=No,Category=Advanced Printer Setup",
    N_("Continuous Printing"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "P95Brightness", N_("Brightness"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Brightness Adjustment"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "P95Contrast", N_("Contrast"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Contrast Adjustment"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "UserComment", N_("User Comment"), "Color=No,Category=Advanced Printer Setup",
    N_("User-specified comment (0-34 characters from 0x20->0x7E), null terminated if under 34 characters long"),
    STP_PARAMETER_TYPE_RAW, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 0, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "UserLUT", N_("User LUT"), "Color=No,Category=Advanced Printer Setup",
    N_("User-specified Lookup Table, must be exactly 34 bytes in long"),
    STP_PARAMETER_TYPE_RAW, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 0, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define mitsu_p95d_parameter_count (sizeof(mitsu_p95d_parameters) / sizeof(const stp_parameter_t))

static int
mitsu_p95d_load_parameters(const stp_vars_t *v, const char *name,
			 stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

    if (strcmp(name, "P95Gamma") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu_p95d_gamma_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    } else if (strcmp(name, "Buzzer") == 0) {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu_p95d_buzzer_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 2)->name;
      description->is_active = 1;
    } else if (strcmp(name, "MediaCut") == 0) {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu_p95d_cutter_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 2)->name;
      description->is_active = 1;
    } else if (strcmp(name, "Comment") == 0) {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu_p95d_comment_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    } else if (strcmp(name, "ClearMemory") == 0) {
      description->is_active = 1;
      description->deflt.boolean = 0;
    } else if (strcmp(name, "ContinuousPrint") == 0) {
      description->is_active = 1;
      description->deflt.boolean = 0;
    } else if (strcmp(name, "P95Brightness") == 0) {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -127;
      description->bounds.integer.upper = 127;
      description->is_active = 1;
    } else if (strcmp(name, "P95Contrast") == 0) {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -127;
      description->bounds.integer.upper = 127;
      description->is_active = 1;
    } else if (strcmp(name, "UserComment") == 0) {
      description->is_active = 1;
    } else if (strcmp(name, "UserLUT") == 0) {
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static const char *p95d_lut = "\x00\x12\x01\x5e\x03\x52\x05\xdc\x08\x66\x0a\x96\x0c\x3a\x0d\x70\x0e\x42\x0e\xce\x0f\x32\x0f\x78\x0f\xa0\x0f\xb4\x0f\xc8\x0f\xd8\x0f\xff";  /* Taken from "P95D.lut" dated 2016-05-25 */

static int mitsu_p95d_parse_parameters(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  const char *gamma = stp_get_string_parameter(v, "P95Gamma");
  const char *buzzer = stp_get_string_parameter(v, "Buzzer");
  const char *cutter = stp_get_string_parameter(v, "MediaCut");
  const char *comment = stp_get_string_parameter(v, "Comment");
  const stp_raw_t *usercomment = NULL;
  const stp_raw_t *userlut = NULL;

  /* Sanity check */
  if (stp_check_raw_parameter(v, "UserComment", STP_PARAMETER_ACTIVE)) {
    usercomment = stp_get_raw_parameter(v, "UserComment");
    if (usercomment->bytes > 34) {
      stp_eprintf(v, _("StpUserComment must be between 0 and 34 bytes!\n"));
      return 0;
    }
  }

  if (stp_check_raw_parameter(v, "UserLUT", STP_PARAMETER_ACTIVE)) {
    userlut = stp_get_raw_parameter(v, "UserLUT");
    if (usercomment->bytes != 34) {
      stp_eprintf(v, _("StpUserLUT must be exactly 34 bytes!\n"));
      return 0;
    }
  }

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  /* Parse options */
  pd->privdata.m95d.clear_mem = stp_get_boolean_parameter(v, "ClearMemory");
  pd->privdata.m95d.cont_print = stp_get_boolean_parameter(v, "ContinuousPrint");

  if (pd->copies > 200)
    pd->copies = 200;

  pd->privdata.m95d.brightness = stp_get_int_parameter(v, "P95Brightness");
  pd->privdata.m95d.contrast = stp_get_int_parameter(v, "P95Contrast");

  if (!strcmp(gamma, "Printer")) {
    pd->privdata.m95d.gamma = 0x00;
  } else if (!strcmp(gamma, "T1")) {
    pd->privdata.m95d.gamma = 0x01;
  } else if (!strcmp(gamma, "T2")) {
    pd->privdata.m95d.gamma = 0x02;
  } else if (!strcmp(gamma, "T3")) {
    pd->privdata.m95d.gamma = 0x03;
  } else if (!strcmp(gamma, "T4")) {
    pd->privdata.m95d.gamma = 0x04;
  } else if (!strcmp(gamma, "T5")) {
    pd->privdata.m95d.gamma = 0x05;
  } else if (!strcmp(gamma, "LUT")) {
    pd->privdata.m95d.gamma = 0x10;
  }

  if (!strcmp(buzzer, "Off")) {
    pd->privdata.m95d.flags |= 0x00;
  } else if (!strcmp(buzzer, "Low")) {
    pd->privdata.m95d.flags |= 0x02;
  } else if (!strcmp(buzzer, "High")) {
    pd->privdata.m95d.flags |= 0x03;
  }

  if (!strcmp(cutter, "PaperSave")) {
    pd->privdata.m95d.flags |= 0x54;
  } else if (!strcmp(cutter, "4mm")) {
    pd->privdata.m95d.flags |= 0x40;
  } else if (!strcmp(cutter, "5mm")) {
    pd->privdata.m95d.flags |= 0x50;
  } else if (!strcmp(cutter, "6mm")) {
    pd->privdata.m95d.flags |= 0x60;
  } else if (!strcmp(cutter, "7mm")) {
    pd->privdata.m95d.flags |= 0x70;
  } else if (!strcmp(cutter, "8mm")) {
    pd->privdata.m95d.flags |= 0x80;
  }

  if (!strcmp(comment, "Off")) {
    memset(pd->privdata.m95d.commentbuf, 0, sizeof(pd->privdata.m95d.commentbuf));
    pd->privdata.m95d.comment = 0;
  } else if (!strcmp(comment, "Settings")) {
    memset(pd->privdata.m95d.commentbuf, 0, sizeof(pd->privdata.m95d.commentbuf));
    pd->privdata.m95d.comment = 1;
  } else if (!strcmp(comment, "Date")) {
    struct tm tmp;
    time_t t;
    t = stpi_time(NULL);
    localtime_r(&t, &tmp);
    strftime(pd->privdata.m95d.commentbuf, sizeof(pd->privdata.m95d.commentbuf), "        %F", &tmp);
    pd->privdata.m95d.comment = 2;
  } else if (!strcmp(comment, "DateTime")) {
    struct tm tmp;
    time_t t;
    t = stpi_time(NULL);
    localtime_r(&t, &tmp);
    strftime(pd->privdata.m95d.commentbuf, sizeof(pd->privdata.m95d.commentbuf), "  %F %R", &tmp);
    pd->privdata.m95d.comment = 3;
  }

  if (usercomment) {
    if (strncmp("None", usercomment->data, usercomment->bytes)) {
      int i;
      memcpy(pd->privdata.m95d.usercomment, usercomment->data, usercomment->bytes);
      if (usercomment->bytes < 34)
        pd->privdata.m95d.usercomment[usercomment->bytes] = 0;
      for (i = 0 ; i < usercomment->bytes ; i++) {
        if (pd->privdata.m95d.usercomment[i] < 0x20 ||
	    pd->privdata.m95d.usercomment[i] > 0x7F)
	  pd->privdata.m95d.usercomment[i] = 0x20;
      }
    }
  } else {
    memset(pd->privdata.m95d.usercomment, 0x20, sizeof(pd->privdata.m95d.usercomment));
  }

  if (userlut) {
    memcpy(pd->privdata.m95d.userlut, userlut->data, userlut->bytes);
  } else {
    memcpy(pd->privdata.m95d.userlut, p95d_lut, sizeof(pd->privdata.m95d.userlut));
  }

  return 1;
}

static void mitsu_p95d_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Header */
  stp_putc(0x1b, v);
  stp_putc(0x51, v);

  /* Clear memory */
  if (pd->privdata.m95d.clear_mem) {
    stp_putc(0x1b, v);
    stp_putc(0x5a, v);
    stp_putc(0x43, v);
    stp_putc(0x00, v);
  }

  /* Page Setup */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x20, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x0a, v);
  stp_putc(0x00, v);
  stp_putc(0x02, v);
  dyesub_nputc(v, 0x00, 6);
  stp_put16_be(pd->w_size, v);  /* Columns */
  stp_put16_be(pd->h_size, v);  /* Rows */

  /* This is only set under Windows if a "custom" size is selected,
     but the USB comms always show it set to 1... */
  if (!strcmp(pd->pagesize,"Custom"))
   stp_putc(0x01, v);
  else
    stp_putc(0x00, v);
  dyesub_nputc(v, 0x00, 31);

  /* Print Options */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x21, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x4a, v);
  stp_putc(0xaa, v);
  stp_putc(0x00, v);
  stp_putc(0x20, v);
  stp_zfwrite((pd->media->seq).data, 1, 1, v);  /* Media Type */
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x64, v);
  if (pd->privdata.m95d.cont_print)
    stp_putc(0xff, v);
  else
    stp_putc(pd->copies, v);
  stp_putc(0x00, v);
  stp_putc(pd->privdata.m95d.comment, v);
  stp_zfwrite(pd->privdata.m95d.commentbuf, 1, sizeof(pd->privdata.m95d.commentbuf) -1, v);
  dyesub_nputc(v, 0x00, 3);
  stp_putc(0x02, v);
  dyesub_nputc(v, 0x00, 11);
  stp_putc(pd->privdata.m95d.flags, v);

  /* Gamma */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x22, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x15, v);
  if (pd->privdata.m95d.gamma == 0x10)
    stp_putc(0x01, v);
  else
    stp_putc(0x00, v);
  dyesub_nputc(v, 0x00, 5);
  stp_putc(pd->privdata.m95d.gamma, v);
  stp_putc(pd->privdata.m95d.brightness, v);
  stp_putc(pd->privdata.m95d.contrast, v);
  stp_putc(0x00, v);

  if (pd->privdata.m95d.gamma == 0x10) {
    stp_zfwrite(pd->privdata.m95d.userlut, 1, sizeof(pd->privdata.m95d.userlut), v); /* XXX only for K95HG? */
  } else {
    dyesub_nputc(v, 0x00, 34);
  }

  /* User Comment */
  stp_putc(0x1b, v);
  stp_putc(0x58, v);
  stp_zfwrite(pd->privdata.m95d.usercomment, 1, sizeof(pd->privdata.m95d.usercomment) - 6, v);  /* 34 on P95, 40 on P93 */
}

static void mitsu_p95d_plane_start(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Plane header */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x74, v);
  stp_putc(0x00, v);
  stp_put16_be(0, v);  /* Column Offset */
  stp_put16_be(0, v);  /* Row Offset */
  stp_put16_be(pd->w_size, v);  /* Columns */
  stp_put16_be(pd->h_size, v);  /* Rows */
}

static void mitsu_p95d_printer_end(stp_vars_t *v)
{
  /* Kick off the actual print */
  stp_putc(0x1b, v);
  stp_putc(0x50, v);
}

/* Mitsubishi P93D/DW */

static const dyesub_media_t mitsu_p93d_medias[] =
{
  {"Standard",  N_("Standard (KP61B)"), {1, "\x02"}},
  {"HighDensity", N_("High Density (KP65HM)"), {1, "\x00"}},
  {"HighGlossy", N_("High Glossy (KP91HG)"), {1, "\x01"}},
};

LIST(dyesub_media_list_t, mitsu_p93d_media_list, dyesub_media_t, mitsu_p93d_medias);

static const dyesub_stringitem_t mitsu_p93d_gammas[] =
{
  { "T1", N_ ("Table 1") },
  { "T2", N_ ("Table 2") },
  { "T3", N_ ("Table 3") },
  { "T4", N_ ("Table 4") },
  { "T5", N_ ("Table 5") },
};
LIST(dyesub_stringlist_t, mitsu_p93d_gamma_list, dyesub_stringitem_t, mitsu_p93d_gammas);

static const stp_parameter_t mitsu_p93d_parameters[] =
{
  {
    "P93Gamma", N_("Printer Gamma Correction"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Gamma Correction"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Buzzer", N_("Printer Buzzer"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Buzzer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "PaperSaving", N_("Paper Saving Mode"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Paper Saving Mode"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Comment", N_("Generate Comment"), "Color=No,Category=Advanced Printer Setup",
    N_("Generate Comment"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "ClearMemory", N_("Clear Memory"), "Color=No,Category=Advanced Printer Setup",
    N_("Clear Memory"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "ContinuousPrint", N_("Continuous Printing"), "Color=No,Category=Advanced Printer Setup",
    N_("Continuous Printing"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "P93Brightness", N_("Brightness"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Brightness Adjustment"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "P93Contrast", N_("Contrast"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Contrast Adjustment"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Sharpen", N_("Image Sharpening"), "Color=No,Category=Advanced Printer Setup",
    N_("Sharpening to apply to image (0 is soft, 1 is normal, 2 is hard"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "UserComment", N_("User Comment"), "Color=No,Category=Advanced Printer Setup",
    N_("User-specified comment (0-40 characters from 0x20->0x7E), null terminated if under 40 characters long"),
    STP_PARAMETER_TYPE_RAW, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 0, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define mitsu_p93d_parameter_count (sizeof(mitsu_p93d_parameters) / sizeof(const stp_parameter_t))

static int
mitsu_p93d_load_parameters(const stp_vars_t *v, const char *name,
			 stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

    if (strcmp(name, "P93Gamma") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu_p93d_gamma_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    } else if (strcmp(name, "Buzzer") == 0) {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu_p95d_buzzer_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 2)->name;
      description->is_active = 1;
    } else if (strcmp(name, "PaperSaving") == 0) {
      description->deflt.boolean = 0;
      description->is_active = 1;
    } else if (strcmp(name, "Comment") == 0) {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu_p95d_comment_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    } else if (strcmp(name, "ClearMemory") == 0) {
      description->is_active = 1;
      description->deflt.boolean = 0;
    } else if (strcmp(name, "ContinuousPrint") == 0) {
      description->is_active = 1;
      description->deflt.boolean = 0;
    } else if (strcmp(name, "P93Brightness") == 0) {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -127;
      description->bounds.integer.upper = 127;
      description->is_active = 1;
    } else if (strcmp(name, "P93Contrast") == 0) {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -127;
      description->bounds.integer.upper = 127;
      description->is_active = 1;
    } else if (strcmp(name, "Sharpen") == 0) {
      description->deflt.integer = 1;
      description->bounds.integer.lower = 0;
      description->bounds.integer.upper = 2;
      description->is_active = 1;
    } else if (strcmp(name, "UserComment") == 0) {
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int mitsu_p93d_parse_parameters(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  const char *gamma = stp_get_string_parameter(v, "P93Gamma");
  const char *buzzer = stp_get_string_parameter(v, "Buzzer");
  const char *comment = stp_get_string_parameter(v, "Comment");
  const stp_raw_t *usercomment = NULL;

  /* Sanity check */
  if (stp_check_raw_parameter(v, "UserComment", STP_PARAMETER_ACTIVE)) {
    usercomment = stp_get_raw_parameter(v, "UserComment");
    if (usercomment->bytes > 40) {
      stp_eprintf(v, _("StpUserComment must be between 0 and 40 bytes!\n"));
      return 0;
    }
  }

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  /* Parse options */
  pd->privdata.m95d.clear_mem = stp_get_boolean_parameter(v, "ClearMemory");
  pd->privdata.m95d.cont_print = stp_get_boolean_parameter(v, "ContinuousPrint");

  if (pd->copies > 200)
    pd->copies = 200;

  if (!strcmp(gamma, "T1")) {
    pd->privdata.m95d.gamma = 0x00;
  } else if (!strcmp(gamma, "T2")) {
    pd->privdata.m95d.gamma = 0x01;
  } else if (!strcmp(gamma, "T3")) {
    pd->privdata.m95d.gamma = 0x02;
  } else if (!strcmp(gamma, "T4")) {
    pd->privdata.m95d.gamma = 0x03;
  } else if (!strcmp(gamma, "T5")) {
    pd->privdata.m95d.gamma = 0x04;
  }

  if (!strcmp(buzzer, "Off")) {
    pd->privdata.m95d.flags |= 0x00;
  } else if (!strcmp(buzzer, "Low")) {
    pd->privdata.m95d.flags |= 0x02;
  } else if (!strcmp(buzzer, "High")) {
    pd->privdata.m95d.flags |= 0x03;
  }

  pd->privdata.m95d.brightness = stp_get_int_parameter(v, "P93Brightness");
  pd->privdata.m95d.contrast = stp_get_int_parameter(v, "P93Contrast");
  pd->privdata.m95d.sharpen = stp_get_int_parameter(v, "Sharpen");

  if (stp_get_boolean_parameter(v, "PaperSaving")) {
    pd->privdata.m95d.flags |= 0x04;
  }

  if (!strcmp(comment, "Off")) {
    memset(pd->privdata.m95d.commentbuf, 0, sizeof(pd->privdata.m95d.commentbuf));
    pd->privdata.m95d.comment = 0;
  } else if (!strcmp(comment, "Settings")) {
    memset(pd->privdata.m95d.commentbuf, 0, sizeof(pd->privdata.m95d.commentbuf));
    pd->privdata.m95d.comment = 1;
  } else if (!strcmp(comment, "Date")) {
    struct tm tmp;
    time_t t;
    t = stpi_time(NULL);
    localtime_r(&t, &tmp);
    strftime(pd->privdata.m95d.commentbuf, sizeof(pd->privdata.m95d.commentbuf), "        %F", &tmp);
    pd->privdata.m95d.comment = 2;
  } else if (!strcmp(comment, "DateTime")) {
    struct tm tmp;
    time_t t;
    t = stpi_time(NULL);
    localtime_r(&t, &tmp);
    strftime(pd->privdata.m95d.commentbuf, sizeof(pd->privdata.m95d.commentbuf), "  %F %R", &tmp);
    pd->privdata.m95d.comment = 3;
  }

  if (usercomment) {
    if (strncmp("None", usercomment->data, usercomment->bytes)) {
      int i;
      memcpy(pd->privdata.m95d.usercomment, usercomment->data, usercomment->bytes);
      if (usercomment->bytes < 40)
        pd->privdata.m95d.usercomment[usercomment->bytes] = 0;
      for (i = 0 ; i < usercomment->bytes ; i++) {
        if (pd->privdata.m95d.usercomment[i] < 0x20 ||
	    pd->privdata.m95d.usercomment[i] > 0x7F)
	  pd->privdata.m95d.usercomment[i] = 0x20;
      }
    }
  } else {
    memset(pd->privdata.m95d.usercomment, 0x20, sizeof(pd->privdata.m95d.usercomment));
  }

  return 1;
}

static void mitsu_p93d_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Header */
  stp_putc(0x1b, v);
  stp_putc(0x51, v);

  /* Clear memory */
  if (pd->privdata.m95d.clear_mem) {
    stp_putc(0x1b, v);
    stp_putc(0x5a, v);
    stp_putc(0x43, v);
    stp_putc(0x00, v);
  }

  /* Page Setup */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x20, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x0a, v);
  dyesub_nputc(v, 0x00, 8);
  stp_put16_be(pd->w_size, v);  /* Columns */
  stp_put16_be(pd->h_size, v);  /* Rows */

  /* This is only set under Windows if a "custom" size is selected,
     but the USB comms always show it set to 1... */
  if (!strcmp(pd->pagesize,"Custom"))
    stp_putc(0x01, v);
  else
    stp_putc(0x00, v);
  dyesub_nputc(v, 0x00, 31);

  /* Print Options */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x21, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x4a, v);
  stp_putc(0xaa, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_zfwrite((pd->media->seq).data, 1, 1, v);  /* Media Type */
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  if (pd->privdata.m95d.cont_print)
    stp_putc(0xff, v);
  else
    stp_putc(pd->copies, v);
  stp_putc(0x00, v);
  stp_putc(pd->privdata.m95d.comment, v);
  stp_zfwrite(pd->privdata.m95d.commentbuf, 1, sizeof(pd->privdata.m95d.commentbuf) -1, v);
  dyesub_nputc(v, 0x00, 3);
  stp_putc(0x02, v);
  dyesub_nputc(v, 0x00, 11);
  stp_putc(pd->privdata.m95d.flags, v);

  /* Gamma */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x22, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0xd5, v);
  dyesub_nputc(v, 0x00, 6);

  stp_putc(pd->privdata.m95d.sharpen, v);  // XXX
  stp_putc(0x00, v);
  stp_putc(pd->privdata.m95d.gamma, v);
  stp_putc(0x00, v);
  stp_putc(pd->privdata.m95d.brightness, v);
  stp_putc(0x00, v);
  stp_putc(pd->privdata.m95d.contrast, v);
  dyesub_nputc(v, 0x00, 31);

  /* User Comment */
  stp_putc(0x1b, v);
  stp_putc(0x58, v);
  stp_zfwrite(pd->privdata.m95d.usercomment, 1, sizeof(pd->privdata.m95d.usercomment), v);
}

/* Mitsubishi CP3020D/DU/DE */
static const dyesub_pagesize_t mitsu_cp3020d_page[] =
{
  DEFINE_PAPER_SIMPLE( "A4", "A4", PT1(2508,314), PT1(3134,314), DYESUB_PORTRAIT), /* A4 */
  DEFINE_PAPER_SIMPLE( "Legal", "Letter Long", PT1(2508,314), PT1(3762,314), DYESUB_PORTRAIT), /* Letter */
};

LIST(dyesub_pagesize_list_t, mitsu_cp3020d_page_list, dyesub_pagesize_t, mitsu_cp3020d_page);

static const dyesub_printsize_t mitsu_cp3020d_printsize[] =
{
  { "314x314", "A4", 2508, 3134},
  { "314x314", "Legal", 2508, 3762},
};

LIST(dyesub_printsize_list_t, mitsu_cp3020d_printsize_list, dyesub_printsize_t, mitsu_cp3020d_printsize);

static void mitsu_cp3020d_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Start with NULL block */
  dyesub_nputc(v, 0x00, 64);
  /* Unknown */
  stp_putc(0x1b, v);
  stp_putc(0x51, v);
  dyesub_nputc(v, 0x00, 62);
  /* Paper type */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x46, v);
  if (pd->h_size == 3762)
    stp_putc(0x04, v);
  else
    stp_putc(0x00, v);
  dyesub_nputc(v, 0x00, 60);
  /* Number of copies */
  stp_putc(0x1b, v);
  stp_putc(0x4e, v);
  stp_putc(pd->copies > 50 ? 50 : pd->copies, v); /* 1-50 */
  dyesub_nputc(v, 0x00, 61);
  /* Unknown */
  stp_putc(0x1b, v);
  stp_putc(0x46, v);
  stp_putc(0x53, v);
  dyesub_nputc(v, 0x00, 61);
  /* Lamination.  Fixed on. */
  stp_putc(0x1b, v);
  stp_putc(0x59, v);
  dyesub_nputc(v, 0x00, 62);
  /* High Contrast */
  stp_putc(0x1b, v);
  stp_putc(0x46, v);
  stp_putc(0x43, v);
  stp_putc(0x00, v);  /* XXX or 0x01 for "High Contrast" */
  dyesub_nputc(v, 0x00, 60);
  /* Print dimensions */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x53, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  dyesub_nputc(v, 0x00, 57);
}

static void mitsu_cp3020d_printer_end(stp_vars_t *v)
{
  /* Page Footer */
  stp_putc(0x1b, v);
  stp_putc(0x50, v);
  dyesub_nputc(v, 0x00, 62);
}

static void mitsu_cp3020d_plane_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Plane data header */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x30 + 4 - pd->plane, v); /* Y = x31, M = x32, C = x33 */
  dyesub_nputc(v, 0x00, 2);
  stp_put16_be(0, v); /* Starting row for this block */
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v); /* Number of rows in this block */
  dyesub_nputc(v, 0x00, 53);
}

static void mitsu_cp3020d_plane_end(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  /* Pad data to 64-byte block */
  unsigned int length = pd->w_size * pd->h_size;
  length %= 64;
  if (length) {
    length = 64 - length;
    dyesub_nputc(v, 0x00, length);
  }
}

/* Mitsubishi CP3020DA/DAE */
static void mitsu_cp3020da_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Init */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x20, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x0a, v);
  stp_putc(0x10, v);
  dyesub_nputc(v, 0x00, 7);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  dyesub_nputc(v, 0x00, 32);
  /* Page count */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x21, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x20, v);
  stp_putc(0x00, v);
  stp_putc(0x02, v);
  dyesub_nputc(v, 0x00, 19);
  stp_putc(pd->copies > 50 ? 50 : pd->copies, v);  /* 1-50 */
  dyesub_nputc(v, 0x00, 20);
  /* Contrast ? */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x22, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0xf0, v);
  dyesub_nputc(v, 0x00, 4);
  stp_putc(0x00, v); /* x00 = Photo, x01 = High Contrast, x02 = Natural */
  dyesub_nputc(v, 0x00, 39);
  /* Unknown */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x26, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x20, v);
  dyesub_nputc(v, 0x00, 6);
  stp_putc(0x01, v);
  dyesub_nputc(v, 0x00, 37);
}

static void mitsu_cp3020da_printer_end(stp_vars_t *v)
{
  /* Page Footer */
  stp_putc(0x1b, v);
  stp_putc(0x50, v);
  stp_putc(0x51, v);
  stp_putc(0x50, v);
}

static void mitsu_cp3020da_plane_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Plane data header */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x54, v);
  stp_putc(0x00, v);
  stp_put16_be(0, v); /* Starting column for this block */
  stp_put16_be(0, v); /* Starting row for this block */
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v); /* Number of rows in this block */
}

/* Mitsubishi 9500D/DW */
static const dyesub_resolution_t res_m9500[] =
{
  { "346x346", 346, 346},
  { "346x792", 346, 792},
};

LIST(dyesub_resolution_list_t, res_m9500_list, dyesub_resolution_t, res_m9500);

static const dyesub_pagesize_t mitsu_cp9500_page[] =
{
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1240,346), PT1(1812,346), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1416,346), PT1(2152,346), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1812,346), PT1(2452,346), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(2152,346), PT1(2792,346), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(2152,346), PT1(3146,346), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, mitsu_cp9500_page_list, dyesub_pagesize_t, mitsu_cp9500_page);

static const dyesub_printsize_t mitsu_cp9500_printsize[] =
{
  { "346x346", "B7", 1240, 1812},
  { "346x792", "B7", 2480, 1812},
  { "346x346", "w288h432", 1416, 2152},
  { "346x792", "w288h432", 2832, 2152},
  { "346x346", "w360h504", 1812, 2452},
  { "346x792", "w360h504", 1812, 4904},
  { "346x346", "w432h576", 2152, 2792},
  { "346x792", "w432h576", 2152, 5584},
  { "346x346", "w432h648", 2152, 3146},
  { "346x792", "w432h648", 2152, 6292},
};

LIST(dyesub_printsize_list_t, mitsu_cp9500_printsize_list, dyesub_printsize_t, mitsu_cp9500_printsize);

static void mitsu_cp9500_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Init */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x21, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x22, v);
  stp_putc(0xa8, v);
  stp_putc(0x03, v);
  dyesub_nputc(v, 0x00, 18);
  stp_put16_be(pd->copies, v);
  dyesub_nputc(v, 0x00, 19);
  stp_putc(0x01, v);
  /* Parameters 1 */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x20, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x0a, v);
  stp_putc(0x10, v);
  dyesub_nputc(v, 0x00, 7);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  dyesub_nputc(v, 0x00, 32);
  /* Parameters 2 */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x22, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0xf0, v);
  dyesub_nputc(v, 0x00, 5);
  stp_putc(0x00, v); //  XXX 0x01 for "High Contrast" mode
  dyesub_nputc(v, 0x00, 38);
  /* Unknown */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x26, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x70, v);
  dyesub_nputc(v, 0x00, 6);
  stp_putc(0x01, v);
  stp_putc(0x01, v);
  dyesub_nputc(v, 0x00, 36);
}

static void mitsu_cp9500_printer_end(stp_vars_t *v)
{
  /* Page Footer */
  stp_putc(0x1b, v);
  stp_putc(0x50, v);
  stp_putc(0x57, v);
  stp_putc(0x00, v);
}

static const dyesub_stringitem_t mitsu9500_contrasts[] =
{
  { "Photo",      N_ ("Photo") },
  { "HighContrast", N_ ("High Contrast") },
};
LIST(dyesub_stringlist_t, mitsu9500_contrast_list, dyesub_stringitem_t, mitsu9500_contrasts);

static const stp_parameter_t mitsu9500_parameters[] =
{
  {
    "CP9500Contrast", N_("Printer Contrast"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Contrast"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define mitsu9500_parameter_count (sizeof(mitsu9500_parameters) / sizeof(const stp_parameter_t))

static int
mitsu9500_load_parameters(const stp_vars_t *v, const char *name,
			 stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "CP9500Contrast") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu9500_contrast_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int mitsu9500_parse_parameters(stp_vars_t *v)
{
  const char *contrast = stp_get_string_parameter(v, "CP9500Contrast");
  dyesub_privdata_t *pd = get_privdata(v);

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  if (strcmp(contrast, "HighContrast") == 0) {
    pd->privdata.m9550.contrast = 1;
  } else {
    pd->privdata.m9550.contrast = 0;
  }

  return 1;
}

/* Mitsubishi 9550D/DW */
static const dyesub_resolution_t res_346dpi[] =
{
  { "346x346", 346, 346},
};

LIST(dyesub_resolution_list_t, res_346dpi_list, dyesub_resolution_t, res_346dpi);

static const dyesub_pagesize_t mitsu_cp9550_page[] =
{
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1240,346), PT1(1812,346), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1416,346), PT1(2152,346), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1416,346), PT1(2152,346), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1812,346), PT1(2452,346), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(2152,346), PT1(2792,346), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h612", "6x8.5", PT1(2152,346), PT1(2956,346), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(2152,346), PT1(3146,346), DYESUB_PORTRAIT),
  /* XXX also 3.3x6 and 3.5x6!
     XXX also 4x6*2, 4.4*6*2, 3x6*3, 2x6*4!  (Built on 6x9 media) */
};

LIST(dyesub_pagesize_list_t, mitsu_cp9550_page_list, dyesub_pagesize_t, mitsu_cp9550_page);

static const dyesub_printsize_t mitsu_cp9550_printsize[] =
{
  { "346x346", "B7", 1240, 1812},
  { "346x346", "w288h432", 1416, 2152},
  { "346x346", "w288h432-div2", 1416, 2152},
  { "346x346", "w360h504", 1812, 2452},
  { "346x346", "w432h576", 2152, 2792},
  { "346x346", "w432h612", 2152, 2956},
  { "346x346", "w432h648", 2152, 3146},
};

LIST(dyesub_printsize_list_t, mitsu_cp9550_printsize_list, dyesub_printsize_t, mitsu_cp9550_printsize);

static const dyesub_stringitem_t mitsu9550_qualities[] =
{
  { "Fine",      N_ ("Fine") },
  { "SuperFine", N_ ("Super Fine") },
  { "FineDeep", N_ ("Fine Deep") }
};
LIST(dyesub_stringlist_t, mitsu9550_quality_list, dyesub_stringitem_t, mitsu9550_qualities);

static const stp_parameter_t mitsu9550_parameters[] =
{
  {
    "PrintSpeed", N_("Print Speed"), "Color=No,Category=Advanced Printer Setup",
    N_("Print Speed"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define mitsu9550_parameter_count (sizeof(mitsu9550_parameters) / sizeof(const stp_parameter_t))

static int
mitsu9550_load_parameters(const stp_vars_t *v, const char *name,
			 stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "PrintSpeed") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu9550_quality_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int mitsu9550_parse_parameters(stp_vars_t *v)
{
  const char *quality = stp_get_string_parameter(v, "PrintSpeed");
  dyesub_privdata_t *pd = get_privdata(v);

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  pd->privdata.m9550.quality = 0;
  pd->privdata.m9550.finedeep = 0;

  /* Parse options */
  if (strcmp(quality, "SuperFine") == 0) {
     pd->privdata.m9550.quality = 0x80;
  } else if (strcmp(quality, "FineDeep") == 0) {
     pd->privdata.m9550.finedeep = 1;
  }

  return 1;
}

static void mitsu_cp9550_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Init */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x20, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x0a, v);
  stp_putc(0x10, v);
  dyesub_nputc(v, 0x00, 7);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  dyesub_nputc(v, 0x00, 32);
  /* Parameters 1 */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x21, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x22, v);
  stp_putc(0x08, v);
  stp_putc(0x03, v);
  dyesub_nputc(v, 0x00, 18);
  stp_put16_be(pd->copies, v);
  dyesub_nputc(v, 0x00, 2);
  if (strcmp(pd->pagesize,"w288h432-div2") == 0)
    stp_putc(0x83, v);
  else
    stp_putc(0x00, v);
  dyesub_nputc(v, 0x00, 5);
  stp_putc(pd->privdata.m9550.quality, v);
  dyesub_nputc(v, 0x00, 10);
  stp_putc(0x01, v);
  /* Parameters 2 */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x22, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x40, v);
  dyesub_nputc(v, 0x00, 5);
  stp_putc(pd->privdata.m9550.finedeep, v);
  dyesub_nputc(v, 0x00, 38);
  /* Unknown */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x26, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x70, v);
  dyesub_nputc(v, 0x00, 6);
  stp_putc(0x01, v);
  stp_putc(0x01, v);
  dyesub_nputc(v, 0x00, 36);
}

static void mitsu_cp9550_printer_end(stp_vars_t *v)
{
  /* Page Footer */
  stp_putc(0x1b, v);
  stp_putc(0x50, v);
  stp_putc(0x46, v);
  stp_putc(0x00, v);
}

/* Mitsubishi CP9550DW-S */

static const dyesub_pagesize_t mitsu_cp9550s_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1416,346), PT1(2152,346), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1416,346), PT1(2152,346), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1812,346), PT1(2452,346), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(2152,346), PT1(2792,346), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h612", "6x8.5", PT1(2152,346), PT1(2956,346), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(2152,346), PT1(3146,346), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, mitsu_cp9550s_page_list, dyesub_pagesize_t, mitsu_cp9550s_page);

static const dyesub_printsize_t mitsu_cp9550s_printsize[] =
{
  { "346x346", "w288h432", 1416, 2152},
  { "346x346", "w288h432-div2", 1416, 2152},
  { "346x346", "w360h504", 1812, 2452},
  { "346x346", "w432h576", 2152, 2792},
  { "346x346", "w432h612", 2152, 2956},
  { "346x346", "w432h648", 2152, 3146},
};

LIST(dyesub_printsize_list_t, mitsu_cp9550s_printsize_list, dyesub_printsize_t, mitsu_cp9550s_printsize);

static void mitsu_cp9550s_printer_end(stp_vars_t *v)
{
  /* Page Footer */
  stp_putc(0x1b, v);
  stp_putc(0x50, v);
  stp_putc(0x47, v);
  stp_putc(0x00, v);
}

/* Mitsubishi 9600D/DW */
static const dyesub_resolution_t res_mitsu9600_dpi[] =
{
  { "300x300", 300, 300},
  { "600x600", 600, 600},
};

LIST(dyesub_resolution_list_t, res_mitsu9600_dpi_list, dyesub_resolution_t, res_mitsu9600_dpi);

static const dyesub_pagesize_t mitsu_cp9600_page[] =
{
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1076,300), PT1(1572,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1228,300), PT1(1868,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1572,300), PT1(2128,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1868,300), PT1(2442,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h612", "6x8.5", PT1(1868,300), PT1(2564,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(1868,300), PT1(2730,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, mitsu_cp9600_page_list, dyesub_pagesize_t, mitsu_cp9600_page);

static const dyesub_printsize_t mitsu_cp9600_printsize[] =
{
  { "300x300", "B7", 1076, 1572},
  { "600x600", "B7", 2152, 3144},
  { "300x300", "w288h432", 1228, 1868},
  { "600x600", "w288h432", 2458, 3736},
  { "300x300", "w360h504", 1572, 2128},
  { "600x600", "w360h504", 3144, 4256},
  { "300x300", "w432h576", 1868, 2442},
  { "600x600", "w432h576", 3736, 4846},
  { "300x300", "w432h612", 1868, 2564},
  { "600x600", "w432h612", 3736, 5130},
  { "300x300", "w432h648", 1868, 2730},
  { "600x600", "w432h648", 3736, 5462},
};

LIST(dyesub_printsize_list_t, mitsu_cp9600_printsize_list, dyesub_printsize_t, mitsu_cp9600_printsize);

static void mitsu_cp9600_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Parameters 1 */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x21, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x22, v);
  stp_putc(0x00, v);
  stp_putc(0x03, v);
  dyesub_nputc(v, 0x00, 18);
  stp_put16_be(pd->copies, v);
  dyesub_nputc(v, 0x00, 19);
  stp_putc(0x01, v);
  /* Parameters 2 */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x20, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x0a, v);
  stp_putc(0x10, v);
  dyesub_nputc(v, 0x00, 7);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  dyesub_nputc(v, 0x00, 32);
  /* Parameters 3 */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x26, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x60, v);
  dyesub_nputc(v, 0x00, 6);
  stp_putc(0x01, v);
  dyesub_nputc(v, 0x00, 37);
}

static void mitsu_cp9600_printer_end(stp_vars_t *v)
{
  /* Page Footer */
  stp_putc(0x1b, v);
  stp_putc(0x50, v);
  stp_putc(0x48, v);
  stp_putc(0x00, v);
}

/* Mitsubishi 9810D/DW */
static const dyesub_pagesize_t mitsu_cp9810_page[] =
{
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1076,300), PT1(1572,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1228,300), PT1(1868,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1572,300), PT1(2128,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1868,300), PT1(2442,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h612", "6x8.5", PT1(1868,300), PT1(2564,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(1868,300), PT1(2730,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, mitsu_cp9810_page_list, dyesub_pagesize_t, mitsu_cp9810_page);

static const dyesub_printsize_t mitsu_cp9810_printsize[] =
{
  { "300x300", "B7", 1076, 1572},
  { "300x300", "w288h432", 1228, 1868},
  { "300x300", "w360h504", 1572, 2128},
  { "300x300", "w432h576", 1868, 2442},
  { "300x300", "w432h612", 1868, 2564},
  { "300x300", "w432h648", 1868, 2730},
};

LIST(dyesub_printsize_list_t, mitsu_cp9810_printsize_list, dyesub_printsize_t, mitsu_cp9810_printsize);

static const overcoat_t mitsu_cp9810_overcoat[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x00"}},
  {"Matte", N_("Matte"), {1, "\x01"}},
};

LIST(overcoat_list_t, mitsu_cp9810_overcoat_list, overcoat_t, mitsu_cp9810_overcoat);

static const dyesub_stringitem_t mitsu9810_qualities[] =
{
  { "Fine",      N_ ("Fine (Standard Media") },
  { "FineHG",    N_ ("Fine (High Grade Media)") },
  { "SuperFine", N_ ("Super Fine") },
};
LIST(dyesub_stringlist_t, mitsu9810_quality_list, dyesub_stringitem_t, mitsu9810_qualities);

static const stp_parameter_t mitsu98xx_parameters[] =
{
  {
    "PrintSpeed", N_("Print Speed"), "Color=No,Category=Advanced Printer Setup",
    N_("Print Speed"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "UseLUT", N_("Internal Color Correction"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Use Internal Color Correction"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define mitsu98xx_parameter_count (sizeof(mitsu98xx_parameters) / sizeof(const stp_parameter_t))

static int
mitsu98xx_load_parameters(const stp_vars_t *v, const char *name,
			  stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "PrintSpeed") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu9810_quality_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "UseLUT") == 0)
    {
      description->deflt.boolean = 0;
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int mitsu98xx_parse_parameters(stp_vars_t *v)
{
  const char *quality = stp_get_string_parameter(v, "PrintSpeed");
  dyesub_privdata_t *pd = get_privdata(v);
  const overcoat_t *overcoat = NULL;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  pd->privdata.m9550.quality = 0;

  /* Parse options */
  if (strcmp(quality, "SuperFine") == 0) {
     pd->privdata.m9550.quality = 0x80;
  } else if (strcmp(quality, "FineHG") == 0) {
     pd->privdata.m9550.quality = 0x11; /* Extension, backend corrects */
  } else if (strcmp(quality, "Fine") == 0) {
    pd->privdata.m9550.quality = 0x10;
  }

  pd->privdata.m70x.use_lut = stp_get_boolean_parameter(v, "UseLUT");

  /* Matte lamination forces SuperFine mode */
  if (caps->overcoat) {
    overcoat = dyesub_get_overcoat_pattern(v);
    if (*((const char*)((overcoat->seq).data)) != 0x00) {
      pd->privdata.m9550.quality = 0x80;
    }
  }

  return 1;
}

static void mitsu_cp98xx_printer_init(stp_vars_t *v, int model)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Init */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x20, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x0a, v);
  stp_putc(model, v);
  dyesub_nputc(v, 0x00, 7);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  if (model == 0x90) {
	  stp_zfwrite((pd->overcoat->seq).data, 1,
		      (pd->overcoat->seq).bytes, v); /* Lamination */
  } else {
	  stp_putc(0x00, v);
  }
  dyesub_nputc(v, 0x00, 31);

  /* Parameters 1 */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x21, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x80, v);
  stp_putc(0x00, v);
  stp_putc(0x22, v);
  stp_putc(0x08, v);
  stp_putc(0x01, v);
  dyesub_nputc(v, 0x00, 18);
  stp_put16_be(pd->copies, v);
  dyesub_nputc(v, 0x00, 8);
  stp_putc(pd->privdata.m9550.quality, v);
  dyesub_nputc(v, 0x00, 9);
  stp_putc(pd->privdata.m70x.use_lut, v);  /* Use LUT? EXTENSION! */
  stp_putc(0x01, v);

  /* Unknown */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x26, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x70, v);
  dyesub_nputc(v, 0x00, 6);
  stp_putc(0x01, v);
  stp_putc(0x01, v);
  dyesub_nputc(v, 0x00, 36);

  /* Put out a single plane header */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x54, v);
  stp_putc(0x80, v);  /* special flag to say this is 8bpp packed BGR */
  stp_put16_be(0, v); /* Starting column for this block */
  stp_put16_be(0, v); /* Starting row for this block */
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v); /* Number of rows in this block */
}

static void mitsu_cp9810_printer_init(stp_vars_t *v)
{
  mitsu_cp98xx_printer_init(v, 0x90);
}

static void mitsu_cp9800_printer_init(stp_vars_t *v)
{
  mitsu_cp98xx_printer_init(v, 0x10);
}

static void mitsu_cp9810_printer_end(stp_vars_t *v)
{
  /* Job Footer */
  stp_putc(0x1b, v);
  stp_putc(0x50, v);
  stp_putc(0x4c, v); /* XXX 9800DW-S uses 0x4e, backend corrects */
  stp_putc(0x00, v);
}

/* Mitsubishi CP-D70D/CP-D707 */
static const dyesub_pagesize_t mitsu_cpd70x_page[] =
{
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1076,300), PT1(1568,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1228,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1228,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1568,300), PT1(2128,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w360h504-div2", "3.5x5*2", PT1(1568,300), PT1(2128,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h432", "6x6", PT1(1820,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1864,300), PT1(2422,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h612", "6x8.5", PT1(1864,300), PT1(2564,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(1864,300), PT1(2730,300), DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-div2", "4x6*2", PT1(1864,300), PT1(2730,300), 0, 0, PT1(236,300), 0, DYESUB_PORTRAIT),
#if 0 /* Theoretically supported, no way to test */
  DEFINE_PAPER_SIMPLE( "w432h576-div4", "2x6*4", PT1(1864,300), PT1(2730,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648-div3", "3x6*3", PT1(1864,300), PT1(2730,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648-div2", "4.4x6*2", PT1(1864,300), PT1(2730,300), DYESUB_PORTRAIT),
#endif
};

LIST(dyesub_pagesize_list_t, mitsu_cpd70x_page_list, dyesub_pagesize_t, mitsu_cpd70x_page);

static const dyesub_printsize_t mitsu_cpd70x_printsize[] =
{
  { "300x300", "B7", 1076, 1568},
  { "300x300", "w288h432", 1228, 1864},
  { "300x300", "w288h432-div2", 1228, 1864},
  { "300x300", "w360h504", 1568, 2128},
  { "300x300", "w360h504-div2", 1568, 2128},
  { "300x300", "w432h432", 1820, 1864},
  { "300x300", "w432h576", 1864, 2422},
  { "300x300", "w432h612", 1864, 2564},
  { "300x300", "w432h648", 1864, 2730},
  { "300x300", "w432h576-div2", 1864, 2730},
#if 0
  { "300x300", "w432h576-div4", 1864, 2730},
  { "300x300", "w432h648-div3", 1864, 2730},
  { "300x300", "w432h648-div2", 1864, 2730},
#endif
};

LIST(dyesub_printsize_list_t, mitsu_cpd70x_printsize_list, dyesub_printsize_t, mitsu_cpd70x_printsize);

static const overcoat_t mitsu_cpd70x_overcoat[] =
{
  {"Glossy", N_("Glossy"), {1, "\x00"}},
  {"Matte", N_("Matte"), {1, "\x02"}},
};

LIST(overcoat_list_t, mitsu_cpd70x_overcoat_list, overcoat_t, mitsu_cpd70x_overcoat);

static const dyesub_stringitem_t mitsu70x_qualities[] =
{
  { "Fine",      N_ ("Fine") },
  { "SuperFine", N_ ("Super Fine") },
  { "UltraFine", N_ ("Ultra Fine") }
};
LIST(dyesub_stringlist_t, mitsu70x_quality_list, dyesub_stringitem_t, mitsu70x_qualities);

static const stp_parameter_t mitsu70x_parameters[] =
{
  {
    "PrintSpeed", N_("Print Speed"), "Color=No,Category=Advanced Printer Setup",
    N_("Print Speed"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "UseLUT", N_("Internal Color Correction"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Use Internal Color Correction"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Sharpen", N_("Image Sharpening"), "Color=No,Category=Advanced Printer Setup",
    N_("Sharpening to apply to image (0 is off, 1 is min, 9 is max"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define mitsu70x_parameter_count (sizeof(mitsu70x_parameters) / sizeof(const stp_parameter_t))

static const dyesub_stringitem_t mitsu707_decks[] =
{
  { "Auto",  N_ ("Automatic") },
  { "Lower", N_ ("Lower Deck") },
  { "Upper", N_ ("Upper Deck") }
};
LIST(dyesub_stringlist_t, mitsu707_deck_list, dyesub_stringitem_t, mitsu707_decks);

static const stp_parameter_t mitsu707_parameters[] =
{
  {
    "PrintSpeed", N_("Print Speed"), "Color=No,Category=Advanced Printer Setup",
    N_("Print Speed"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "UseLUT", N_("Internal Color Correction"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Use Internal Color Correction"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Sharpen", N_("Image Sharpening"), "Color=No,Category=Advanced Printer Setup",
    N_("Sharpening to apply to image (0 is off, 1 is min, 9 is max"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Deck", N_("Printer Deck"), "Color=No,Category=Advanced Printer Setup",
    N_("Printer Deck"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define mitsu707_parameter_count (sizeof(mitsu707_parameters) / sizeof(const stp_parameter_t))

static int
mitsu70x_load_parameters(const stp_vars_t *v, const char *name,
			 stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "PrintSpeed") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu70x_quality_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "UseLUT") == 0)
    {
      description->deflt.boolean = 0;
      description->is_active = 1;
    }
  else if (strcmp(name, "Sharpen") == 0)
    {
      description->deflt.integer = 4;
      description->bounds.integer.lower = 0;
      description->bounds.integer.upper = 9;
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int
mitsu707_load_parameters(const stp_vars_t *v, const char *name,
			 stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "PrintSpeed") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu70x_quality_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "UseLUT") == 0)
    {
      description->deflt.boolean = 0;
      description->is_active = 1;
    }
  else if (strcmp(name, "Sharpen") == 0)
    {
      description->deflt.integer = 4;
      description->bounds.integer.lower = 0;
      description->bounds.integer.upper = 9;
      description->is_active = 1;
    }
  else if (strcmp(name, "Deck") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu707_deck_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int mitsu70x_parse_parameters(stp_vars_t *v)
{
  const char *quality = stp_get_string_parameter(v, "PrintSpeed");
  dyesub_privdata_t *pd = get_privdata(v);

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  /* Parse options */
  if (strcmp(quality, "SuperFine") == 0) {
     pd->privdata.m70x.quality = 3;
  } else if (strcmp(quality, "UltraFine") == 0) {
     pd->privdata.m70x.quality = 4;
  } else if (strcmp(quality, "Fine") == 0) {
     pd->privdata.m70x.quality = 0;
  } else {
     pd->privdata.m70x.quality = 0;
  }

  /* For D707 only */
  pd->privdata.m70x.deck = 0;
  if (stp_check_string_parameter(v, "Deck", STP_PARAMETER_ACTIVE)) {
    const char *deck = stp_get_string_parameter(v, "Deck");
    if (strcmp(deck, "Auto") == 0)
      pd->privdata.m70x.deck = 0;
    else if (strcmp(deck, "Lower") == 0)
      pd->privdata.m70x.deck = 1;
    else if (strcmp(deck, "Upper") == 0)
      pd->privdata.m70x.deck = 2;
  }

  pd->privdata.m70x.use_lut = stp_get_boolean_parameter(v, "UseLUT");
  pd->privdata.m70x.sharpen = stp_get_int_parameter(v, "Sharpen");

  return 1;
}

static void mitsu_cpd70k60_job_start(stp_vars_t *v)
{
  /* Printer wakeup, once per job. */
  stp_putc(0x1b, v);
  stp_putc(0x45, v);
  stp_putc(0x57, v);
  stp_putc(0x55, v);
  dyesub_nputc(v, 0x00, 508);
}

static void mitsu_cpd70k60_printer_init(stp_vars_t *v, unsigned char model)
{
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
						stp_get_model_id(v));
  dyesub_privdata_t *pd = get_privdata(v);

  /* Each copy gets this.. */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x54, v);
  stp_putc(model, v); /* k60 == x00, EK305 == x90, d70x/d80 == x01, ask300 = 0x80 */
  dyesub_nputc(v, 0x00, 12);

  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  if (caps->overcoat && *((const char*)((pd->overcoat->seq).data)) != 0x00) {
    stp_put16_be(pd->w_size, v);
    if (model == 0x00 || model == 0x90) {
      pd->privdata.m70x.overcoat_offset = 0;
      if (!pd->privdata.m70x.quality)
	pd->privdata.m70x.quality = 4;  /* Matte Lamination forces UltraFine on K60 or K305 */
    } else {
      /* Overcoat a slightly larger boundary in Matte mode */
      pd->privdata.m70x.overcoat_offset = 12;
      if (!pd->privdata.m70x.quality)
        pd->privdata.m70x.quality = 3; /* Matte Lamination forces Superfine (or UltraFine) */
    }
    stp_put16_be(pd->h_size + pd->privdata.m70x.overcoat_offset, v);
  } else {
    /* Glossy lamination here */
    stp_put16_be(0, v);
    stp_put16_be(0, v);
  }
  stp_putc(pd->privdata.m70x.quality, v);
  dyesub_nputc(v, 0x00, 7);

  if (model == 0x01) {
    stp_putc(pd->privdata.m70x.deck, v); /* D70x: 0x00 Auto deck selection, 0x01 for Lower, 0x02 for Upper. */
  } else {
    stp_putc(0x01, v); /* All others have a single "lower" deck */
  }
  dyesub_nputc(v, 0x00, 7);

  stp_putc(0x00, v); /* Lamination always enabled */

  if (caps->overcoat) {
    stp_zfwrite((pd->overcoat->seq).data, 1,
		(pd->overcoat->seq).bytes, v); /* Lamination mode */
  } else {
    stp_putc(0x00, v);
  }
  dyesub_nputc(v, 0x00, 6);

  /* Multi-cut control */
  if (strcmp(pd->pagesize,"w432h576-div2") == 0) {
    stp_putc(0x01, v);
#if 0
  } else if (strcmp(pd->pagesize,"w432h648-div2") == 0) {
    stp_putc(0x02, v);
  } else if (strcmp(pd->pagesize,"w432h648-div3") == 0) {
    stp_putc(0x03, v);
  } else if (strcmp(pd->pagesize,"w432h576-div4") == 0) {
    stp_putc(0x04, v);
#endif
  } else if (strcmp(pd->pagesize,"w360h504-div2") == 0) {
    stp_putc(0x01, v);
  } else if (strcmp(pd->pagesize,"w288h432-div2") == 0) {
    stp_putc(0x05, v);
  } else {
    stp_putc(0x00, v);
  }
  dyesub_nputc(v, 0x00, 12);
  /* The next four bytes are EXTENSIONS, backend consumes! */
  stp_putc(pd->privdata.m70x.sharpen, v);
  stp_putc(0x01, v);  /* Mark as 8bpp BGR rather than 16bpp YMC cooked */
  stp_putc(pd->privdata.m70x.use_lut, v);  /* Use LUT? */
  stp_putc(0x01, v);  /* Tell the backend the data's in the proper order */
  /* end extension */
  dyesub_nputc(v, 0x00, 447); /* Pad to 512-byte block */
}

static void mitsu_cpd70x_printer_init(stp_vars_t *v)
{
  mitsu_cpd70k60_printer_init(v, 0x01);
}

/* Mitsubishi CP-K60D */
static const dyesub_pagesize_t mitsu_cpk60_page[] =
{
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1076,300), PT1(1568,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1218,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1218,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1568,300), PT1(2128,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w360h504-div2", "3.5x5*2", PT1(1568,300), PT1(2190,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h432", "6x6", PT1(1820,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1864,300), PT1(2422,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576-div2", "4x6*2", PT1(1864,300), PT1(2454,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, mitsu_cpk60_page_list, dyesub_pagesize_t, mitsu_cpk60_page);

static const dyesub_printsize_t mitsu_cpk60_printsize[] =
{
  { "300x300", "B7", 1076, 1568},
  { "300x300", "w288h432", 1218, 1864},
  { "300x300", "w288h432-div2", 1218, 1864},
  { "300x300", "w360h504", 1568, 2128},
  { "300x300", "w360h504-div2", 1568, 2190},
  { "300x300", "w432h432", 1820, 1864},
  { "300x300", "w432h576", 1864, 2422},
  { "300x300", "w432h576-div2", 1864, 2454},
};

LIST(dyesub_printsize_list_t, mitsu_cpk60_printsize_list, dyesub_printsize_t, mitsu_cpk60_printsize);

static void mitsu_cpk60_printer_init(stp_vars_t *v)
{
  mitsu_cpd70k60_printer_init(v, 0x00);
}

/* Identical to the D70 except for one fewer quality mode */
static const dyesub_stringitem_t mitsu_k60_qualities[] =
{
  { "Fine",      N_ ("Fine") },
  { "UltraFine", N_ ("Ultra Fine") }
};
LIST(dyesub_stringlist_t, mitsu_k60_quality_list, dyesub_stringitem_t, mitsu_k60_qualities);

static int
mitsu_k60_load_parameters(const stp_vars_t *v, const char *name,
			  stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "PrintSpeed") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu_k60_quality_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "UseLUT") == 0)
    {
      description->deflt.boolean = 0;
      description->is_active = 1;
    }
  else if (strcmp(name, "Sharpen") == 0)
    {
      description->deflt.integer = 4;
      description->bounds.integer.lower = 0;
      description->bounds.integer.upper = 9;
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static const dyesub_pagesize_t mitsu_cpd80_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1228,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1228,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h360", "5x5", PT1(1524,300), PT1(1568,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1568,300), PT1(2128,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h432", "6x6", PT1(1864,300), PT1(1820,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1864,300), PT1(2422,300), DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-div2", "4x6*2", PT1(1864,300), PT1(2730,300), 0, 0, PT1(236,300), 0, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, mitsu_cpd80_page_list, dyesub_pagesize_t, mitsu_cpd80_page);

static const dyesub_printsize_t mitsu_cpd80_printsize[] =
{
  { "300x300", "w288h432", 1228, 1864},
  { "300x300", "w288h432-div2", 1228, 1864},
  { "300x300", "w360h360", 1524, 1568},
  { "300x300", "w360h504", 1568, 2128},
  { "300x300", "w432h432", 1864, 1820},
  { "300x300", "w432h576", 1864, 2422},
  { "300x300", "w432h576-div2", 1864, 2730},
};

LIST(dyesub_printsize_list_t, mitsu_cpd80_printsize_list, dyesub_printsize_t, mitsu_cpd80_printsize);

/* Kodak 305 */
static const dyesub_pagesize_t kodak305_page[] =
{
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1076,300), PT1(1568,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1218,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1218,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h360", "5x5", PT1(1524,300), PT1(1568,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1568,300), PT1(2128,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w360h504-div2", "3.5x5*2", PT1(1568,300), PT1(2190,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h432", "6x6", PT1(1820,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1864,300), PT1(2422,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576-div2", "4x6*2", PT1(1864,300), PT1(2454,300),	DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, kodak305_page_list, dyesub_pagesize_t, kodak305_page);

static const dyesub_printsize_t kodak305_printsize[] =
{
  { "300x300", "B7", 1076, 1568},
  { "300x300", "w288h432", 1218, 1864},
  { "300x300", "w288h432-div2", 1218, 1864},
  { "300x300", "w360h360", 1524, 1568},
  { "300x300", "w360h504", 1568, 2128},
  { "300x300", "w360h504-div2", 1568, 2190},
  { "300x300", "w432h432", 1820, 1864},
  { "300x300", "w432h576", 1864, 2422},
  { "300x300", "w432h576-div2", 1864, 2454},
};

LIST(dyesub_printsize_list_t, kodak305_printsize_list, dyesub_printsize_t, kodak305_printsize);

static void kodak305_printer_init(stp_vars_t *v)
{
  mitsu_cpd70k60_printer_init(v, 0x90);
}

/* Mitsubishi CP-D90D */
static const dyesub_pagesize_t mitsu_cpd90_page[] =
{
  DEFINE_PAPER_SIMPLE( "w144h432", "2x6", PT1(625,300), PT1(1852,300),	DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1076,300), PT1(1550,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1226,300), PT1(1852,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1226,300), PT1(1852,300),	DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1550,300), PT1(2128,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w360h360", "5x5", PT1(1527,300), PT1(1550,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w432h432", "6x6", PT1(1827,300), PT1(1852,300), DYESUB_LANDSCAPE),
  // XXX add 6x6+2x6!
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1852,300), PT1(2428,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576-div2", "4x6*2", PT1(1852,300), PT1(2488,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h612", "6x8.5", PT1(1852,300), PT1(2568,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(1852,300), PT1(2729,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648-div2", "4.4x6*2", PT1(1852,300), PT1(2728,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648-div3", "3x6*2", PT1(1852,300), PT1(2724,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648-div4", "2x6*4", PT1(1852,300), PT1(2628,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, mitsu_cpd90_page_list, dyesub_pagesize_t, mitsu_cpd90_page);

static const dyesub_printsize_t mitsu_cpd90_printsize[] =
{
  { "300x300", "w144h432", 625, 1852},
  { "300x300", "B7", 1076, 1550},
  { "300x300", "w288h432", 1226, 1852},
  { "300x300", "w288h432-div2", 1226, 1852},
  { "300x300", "w360h360", 1527, 1550},
  { "300x300", "w360h504", 1550, 2128},
  { "300x300", "w432h432", 1827, 1852},
  // XXX add 6x6+2x6!
  { "300x300", "w432h576", 1852, 2428},
  { "300x300", "w432h576-div2", 1852, 2488},
  { "300x300", "w432h612", 1852, 2568},
  { "300x300", "w432h648", 1852, 2729},
  { "300x300", "w432h648-div2", 1852, 2728},
  { "300x300", "w432h648-div3", 1852, 2724},
  { "300x300", "w432h648-div4", 1852, 2628},
};
LIST(dyesub_printsize_list_t, mitsu_cpd90_printsize_list, dyesub_printsize_t, mitsu_cpd90_printsize);

static const dyesub_stringitem_t mitsu_d90_qualities[] =
{
  { "Auto",      N_ ("Automatic") },
  { "Fine",      N_ ("Fine") },
  { "UltraFine", N_ ("Ultra Fine") }
};
LIST(dyesub_stringlist_t, mitsu_d90_quality_list, dyesub_stringitem_t, mitsu_d90_qualities);

static const stp_parameter_t mitsu_d90_parameters[] =
{
  {
    "PrintSpeed", N_("Print Speed"), "Color=No,Category=Advanced Printer Setup",
    N_("Print Speed"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "UseLUT", N_("Internal Color Correction"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Use Internal Color Correction"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Sharpen", N_("Image Sharpening"), "Color=No,Category=Advanced Printer Setup",
    N_("Sharpening to apply to image (0 is off, 1 is min, 9 is max"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "ComboWait", N_("Combo Print Wait Time"), "Color=No,Category=Advanced Printer Setup",
    N_("How many seconds to wait for a second print before starting"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define mitsu_d90_parameter_count (sizeof(mitsu_d90_parameters) / sizeof(const stp_parameter_t))

static int
mitsu_d90_load_parameters(const stp_vars_t *v, const char *name,
			  stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "PrintSpeed") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &mitsu_d90_quality_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "UseLUT") == 0)
    {
      description->deflt.boolean = 0;
      description->is_active = 1;
    }
  else if (strcmp(name, "Sharpen") == 0)
    {
      description->deflt.integer = 4;
      description->bounds.integer.lower = 0;
      description->bounds.integer.upper = 9;
      description->is_active = 1;
    }
  else if (strcmp(name, "ComboWait") == 0)
    {
      description->deflt.integer = 5;
      description->bounds.integer.lower = 1;
      description->bounds.integer.upper = 25;
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int mitsu_d90_parse_parameters(stp_vars_t *v)
{
  const char *quality = stp_get_string_parameter(v, "PrintSpeed");
  dyesub_privdata_t *pd = get_privdata(v);

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  /* Parse options */
  if (strcmp(quality, "UltraFine") == 0) {
     pd->privdata.m70x.quality = 3;
  } else if (strcmp(quality, "Fine") == 0) {
     pd->privdata.m70x.quality = 2;
  } else {
     pd->privdata.m70x.quality = 0;
  }

  pd->privdata.m70x.use_lut = stp_get_boolean_parameter(v, "UseLUT");
  pd->privdata.m70x.sharpen = stp_get_int_parameter(v, "Sharpen");
  pd->privdata.m70x.delay = stp_get_int_parameter(v, "ComboWait");

  return 1;
}

static void mitsu_cpd90_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Start things going */
  stp_putc(0x1b, v);
  stp_putc(0x53, v);
  stp_putc(0x50, v);
  stp_putc(0x30, v);
  stp_putc(0x00, v);
  stp_putc(0x33, v);
  stp_put16_be(pd->w_size, v);  /* Columns */
  stp_put16_be(pd->h_size, v);  /* Rows */
  stp_putc(0x64, v);
  stp_putc(0x00, v);
  stp_putc(0x00, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);

  if (strcmp(pd->pagesize,"w432h576-div2") == 0) {
    stp_putc(0x01, v);
    stp_putc(0x04, v);
    stp_putc(0xbe, v);
    dyesub_nputc(v, 0x00, 6);
  } else if (strcmp(pd->pagesize,"w288h432-div2") == 0) {
    stp_putc(0x00, v);
    stp_putc(0x02, v);
    stp_putc(0x65, v);
    dyesub_nputc(v, 0x00, 6);
  } else if (strcmp(pd->pagesize,"w432h648-div2") == 0) {
    stp_putc(0x01, v);
    stp_putc(0x05, v);
    stp_putc(0x36, v);
    dyesub_nputc(v, 0x00, 6);
  } else if (strcmp(pd->pagesize,"w432h648-div3") == 0) {
    stp_putc(0x00, v);
    stp_putc(0x03, v);
    stp_putc(0x90, v);
    stp_putc(0x00, v);
    stp_putc(0x07, v);
    stp_putc(0x14, v);
    dyesub_nputc(v, 0x00, 3);
  } else if (strcmp(pd->pagesize,"w432h648-div4") == 0) {
    stp_putc(0x00, v);
    stp_putc(0x02, v);
    stp_putc(0x97, v);
    stp_putc(0x00, v);
    stp_putc(0x05, v);
    stp_putc(0x22, v);
    stp_putc(0x00, v);
    stp_putc(0x07, v);
    stp_putc(0xad, v);
  } else {
    dyesub_nputc(v, 0x00, 9);
  }
  dyesub_nputc(v, 0x00, 24);

  stp_zfwrite((pd->overcoat->seq).data, 1,
	      (pd->overcoat->seq).bytes, v); /* Lamination mode */
  stp_putc(pd->privdata.m70x.quality, v);
  stp_putc(pd->privdata.m70x.use_lut, v);
  stp_putc(pd->privdata.m70x.sharpen, v); /* Horizontal */
  stp_putc(pd->privdata.m70x.sharpen, v); /* Vertical */
  dyesub_nputc(v, 0x00, 11);

  dyesub_nputc(v, 0x00, 512 - 64);

  /* Data Plane header */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x54, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);
  stp_putc(0x09, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put16_be(pd->w_size, v);  /* Columns */
  stp_put16_be(pd->h_size, v);  /* Rows */
  dyesub_nputc(v, 0x00, 2);

  dyesub_nputc(v, 0x00, 512 - 16);
}

static void mitsu_cpd90_job_end(stp_vars_t *v)
{
  int delay;
  if (stp_check_int_parameter(v, "ComboWait", STP_PARAMETER_ACTIVE))
    delay = stp_get_int_parameter(v, "ComboWait");
  else
    delay = 5;

  /* Wrap it up */
  stp_putc(0x1b, v);
  stp_putc(0x42, v);
  stp_putc(0x51, v);
  stp_putc(0x31, v);
  stp_putc(0x00, v);
  stp_putc(delay, v);
}

/* Fujifilm ASK-300 */
static const dyesub_pagesize_t fuji_ask300_page[] =
{
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1076,300), PT1(1568,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1228,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1228,300), PT1(1864,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1568,300), PT1(2128,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w360h504-div2", "3.5x5*2", PT1(1568,300), PT1(2128,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1864,300), PT1(2422,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(1864,300), PT1(2730,300), DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-div2", "4x6*2", PT1(1864,300), PT1(2730,300), 0, 0, PT1(236,300), 0, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, fuji_ask300_page_list, dyesub_pagesize_t, fuji_ask300_page);

static const dyesub_printsize_t fuji_ask300_printsize[] =
{
  { "300x300", "B7", 1076, 1568},
  { "300x300", "w288h432", 1228, 1864},
  { "300x300", "w288h432-div2", 1228, 1864},
  { "300x300", "w360h504", 1568, 2128},
  { "300x300", "w360h504-div2", 1568, 2128},
  { "300x300", "w432h576", 1864, 2422},
  { "300x300", "w432h648", 1864, 2730},
  { "300x300", "w432h576-div2", 1864, 2730},
};

LIST(dyesub_printsize_list_t, fuji_ask300_printsize_list, dyesub_printsize_t, fuji_ask300_printsize);

static void fuji_ask300_printer_init(stp_vars_t *v)
{
  mitsu_cpd70k60_printer_init(v, 0x80);
}

/* Fujifilm ASK-2000/2500 */
static const dyesub_pagesize_t fuji_ask2000_page[] =
{
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1074,300), PT1(1536,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1228,300), PT1(1832,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1568,300), PT1(2130,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1832,300), PT1(2432,300), DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-div2", "4x6*2", PT1(1832,300), PT1(2732,300), 0, 0, PT1(236,300), 0, DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(1832,300), PT1(2748,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, fuji_ask2000_page_list, dyesub_pagesize_t, fuji_ask2000_page);

static const dyesub_printsize_t fuji_ask2000_printsize[] =
{
  { "300x300", "B7", 1074, 1536},
  { "300x300", "w288h432", 1228, 1832},
  { "300x300", "w360h504", 1536, 2130},
  { "300x300", "w432h576", 1864, 2432},
  { "300x300", "w432h576-div2", 1864, 2732},
  { "300x300", "w432h648", 1864, 2748},
};

LIST(dyesub_printsize_list_t, fuji_ask2000_printsize_list, dyesub_printsize_t, fuji_ask2000_printsize);

static void fuji_ask2000_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("\x1b\x23\x00\x00\x00\x04\x00\xff\xff\xff\xff", 1, 11, v);
  stp_zfwrite("\x1b\x1e\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 1, 19, v);
  stp_zfwrite("\x1b\xee\x00\x00\x00\x02\x00", 1, 7, v);
  stp_put16_be(pd->copies, v);
  stp_zfwrite("\x1b\xe1\x00\x00\x00\x0b\x00\x00\x04\x0c\x00\x00\x00\x00", 1, 14, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_zfwrite("\x1b\x15\x00\x00\x00\x0d\x00\x00\x00\x00\x00\x60\x00\x00\x00\x00", 1, 16, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_zfwrite("\x1b\xea\x00\x00\x00\x00", 1, 6, v);
  stp_put32_be(pd->w_size * pd->h_size * 3, v); /* Data length */
  stp_putc(0x00, v);
}

static void fuji_ask2000_printer_end(stp_vars_t *v)
{
  stp_zfwrite("\x1b\x23\x00\x00\x00\x04\x00\xff\xff\xff\xff", 1, 11, v);
  stp_zfwrite("\x1b\x0a\x00\x00\x00\x00\x00", 1, 7, v);
  stp_zfwrite("\x1b\x23\x00\x00\x00\x04\x00\xff\xff\xff\xff", 1, 11, v);
}

/* Fujifilm ASK-4000 */
static const dyesub_pagesize_t fuji_ask4000_page[] =
{
  DEFINE_PAPER_SIMPLE( "c8x10", "8x10", PT1(2444,300), PT1(3044,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w576h864", "8x12", PT1(2444,300), PT1(3644,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, fuji_ask4000_page_list, dyesub_pagesize_t, fuji_ask4000_page);

static const dyesub_printsize_t fuji_ask4000_printsize[] =
{
  { "300x300", "c8x10", 2444, 3044},
  { "300x300", "w576h864", 2444, 3644},
};

LIST(dyesub_printsize_list_t, fuji_ask4000_printsize_list, dyesub_printsize_t, fuji_ask4000_printsize);

static void fuji_ask4000_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_zfwrite("\x1b\x1e\x00\x00\x00\x0c\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 1, 19, v);
  stp_zfwrite("\x1b\xee\x00\x00\x00\x02\x00", 1, 7, v);
  stp_put16_be(pd->copies, v);
  stp_zfwrite("\x1b\xe1\x00\x00\x00\x0b\x00\x00\x04\x0c\x00\x00\x00\x00", 1, 14, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  stp_zfwrite("\x1b\xea\x00\x00\x00\x00", 1, 6, v);
  stp_put32_be(pd->w_size * pd->h_size * 3, v); /* Data length */
  stp_putc(0x00, v);
}

static void fuji_ask4000_printer_end(stp_vars_t *v)
{
  stp_zfwrite("\x1b\x0a\x00\x00\x00\x00\x00", 1, 7, v);
}

/* Shinko CHC-S9045 (experimental) */
static const dyesub_pagesize_t shinko_chcs9045_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1240,300), PT1(1844,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "B7","3.5x5", PT1(1088,300), PT1(1548,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1548,300), PT1(2140,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(1844,300), PT1(2740,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w283h425", "Sticker paper", PT1(1092,300), PT1(1726,300), DYESUB_LANDSCAPE),
};

LIST(dyesub_pagesize_list_t, shinko_chcs9045_page_list, dyesub_pagesize_t, shinko_chcs9045_page);

static const dyesub_printsize_t shinko_chcs9045_printsize[] =
{
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "B7", 1088, 1548},
  { "300x300", "w360h504", 1548, 2140},
  { "300x300", "w432h648", 1844, 2740},
  { "300x300", "w283h425", 1092, 1726},
};

LIST(dyesub_printsize_list_t, shinko_chcs9045_printsize_list, dyesub_printsize_t, shinko_chcs9045_printsize);

static void shinko_chcs9045_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char pg = '\0';
  char sticker = '\0';

  stp_zprintf(v, "\033CHC\n");
  stp_put16_be(1, v);
  stp_put16_be(1, v);
  stp_put16_be(pd->w_size, v);
  stp_put16_be(pd->h_size, v);
  if (strcmp(pd->pagesize,"B7") == 0)
    pg = '\1';
  else if (strcmp(pd->pagesize,"w360h504") == 0)
    pg = '\3';
  else if (strcmp(pd->pagesize,"w432h576") == 0)
    pg = '\5';
  else if (strcmp(pd->pagesize,"w283h425") == 0)
    sticker = '\3';
  stp_putc(pg, v);
  stp_putc('\0', v);
  stp_putc(sticker, v);
  dyesub_nputc(v, '\0', 4338);
}

/* Shinko CHC-S2145 */
static const dyesub_pagesize_t shinko_chcs2145_page[] =
{
  DEFINE_PAPER_SIMPLE( "w144h432", "2x6", PT1(634,300), PT1(1844,300),	DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432", "4x6", PT1(1240,300), PT1(1844,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2",	"2x6*2", PT1(1240,300), PT1(1844,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "B7", "3.5x5", PT1(1088,300), PT1(1548,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504", "5x7", PT1(1548,300), PT1(2140,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576", "6x8", PT1(1844,300), PT1(2434,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576-div2",	"4x6*2", PT1(1844,300), PT1(2492,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(1844,300), PT1(2740,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, shinko_chcs2145_page_list, dyesub_pagesize_t, shinko_chcs2145_page);

static const dyesub_printsize_t shinko_chcs2145_printsize[] =
{
  { "300x300", "w144h432", 634, 1844},
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "w288h432-div2", 1240, 1844},
  { "300x300", "B7", 1088, 1548},
  { "300x300", "w360h504", 1548, 2140},
  { "300x300", "w432h576", 1844, 2434},
  { "300x300", "w432h576-div2", 1844, 2492},
  { "300x300", "w432h648", 1844, 2740},
};

LIST(dyesub_printsize_list_t, shinko_chcs2145_printsize_list, dyesub_printsize_t, shinko_chcs2145_printsize);

static const overcoat_t shinko_chcs2145_overcoat[] =
{
  {"PrinterDefault",  N_("Printer Default"),  {4, "\x01\0\0\0"}},
  {"Glossy",  N_("Glossy"),  {4, "\x02\0\0\0"}},
  {"GlossyFine",  N_("Glossy Fine"),  {4, "\x03\0\0\0"}},
  {"Matte",  N_("Matte"),  {4, "\x04\0\0\0"}},
  {"MatteFine",  N_("Matte Fine"),  {4, "\x05\0\0\0"}},
  {"ExtraGlossy",  N_("Extra Glossy"),  {4, "\x06\0\0\0"}},
  {"ExtraGlossyFine",  N_("Extra Glossy Fine"),  {4, "\x07\0\0\0"}},
};

LIST(overcoat_list_t, shinko_chcs2145_overcoat_list, overcoat_t, shinko_chcs2145_overcoat);

static void shinko_chcs2145_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  int media = 0;

  if (strcmp(pd->pagesize,"w288h432") == 0)
    media = '\0';
  else if (strcmp(pd->pagesize,"w288h432-div2") == 0)
    media = '\0';
  else if (strcmp(pd->pagesize,"B7") == 0)
    media = '\1';
  else if (strcmp(pd->pagesize,"w360h504") == 0)
    media = '\3';
  else if (strcmp(pd->pagesize,"w432h576") == 0)
    media = '\6';
  else if (strcmp(pd->pagesize,"w432h648") == 0)
    media = '\5';
  else if (strcmp(pd->pagesize,"w432h576-div2") == 0)
    media = '\5';
  else if (strcmp(pd->pagesize,"w144h432") == 0)
    media = '\7';

  stp_put32_le(0x10, v);
  stp_put32_le(2145, v);  /* Printer Model */
  stp_put32_le(0x00, v);
  stp_put32_le(0x01, v);

  stp_put32_le(0x64, v);
  stp_put32_le(0x00, v);
  stp_put32_le(media, v);  /* Media Type */
  stp_put32_le(0x00, v);

  if (strcmp(pd->pagesize,"w432h576-div2") == 0) {
    stp_put32_le(0x02, v);
  } else if (strcmp(pd->pagesize,"w288h432-div2") == 0) {
    stp_put32_le(0x04, v);
  } else {
    stp_put32_le(0x00, v);  /* Print Method */
  }

  stp_zfwrite((pd->overcoat->seq).data, 1,
	      (pd->overcoat->seq).bytes, v); /* Print Mode */
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
  stp_put32_le(pd->w_size, v); /* Columns */
  stp_put32_le(pd->h_size, v); /* Rows */
  stp_put32_le(pd->copies, v); /* Copies */

  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(pd->w_dpi, v);  /* Dots Per Inch */
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
}

static void shinko_chcs2145_printer_end(stp_vars_t *v)
{
  stp_putc(0x04, v);
  stp_putc(0x03, v);
  stp_putc(0x02, v);
  stp_putc(0x01, v);
}

/* Shinko CHC-S1245 */
static const dyesub_pagesize_t shinko_chcs1245_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h576", "8x4", PT1(1229,300), PT1(2446,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h576", "8x5", PT1(1530,300), PT1(2446,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w432h576", "8x6", PT1(1831,300), PT1(2446,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w576h576", "8x8", PT1(2436,300), PT1(2446,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w576h576-div2", "8x4*2", PT1(2446,300), PT1(2468,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "c8x10", "8x10", PT1(2446,300), PT1(3036,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "c8x10-w576h432_w576h288", "8x6+8x4", PT1(2446,300), PT1(3070,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "c8x10-div2", "8x5*2", PT1(2446,300), PT1(3070,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w576h864", "8x12", PT1(2446,300), PT1(3636,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w576h864-div2", "8x6*2", PT1(2446,300), PT1(3672,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w576h864-div3", "8x4*3", PT1(2446,300), PT1(3707,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, shinko_chcs1245_page_list, dyesub_pagesize_t, shinko_chcs1245_page);

static const dyesub_printsize_t shinko_chcs1245_printsize[] =
{
  { "300x300", "w288h576", 1229, 2446},
  { "300x300", "w360h576", 1530, 2446},
  { "300x300", "w432h576", 1831, 2446},
  { "300x300", "w576h576", 2436, 2446},
  { "300x300", "w576h576-div2", 2446, 2468},
  { "300x300", "c8x10", 2446, 3036},
  { "300x300", "c8x10-w576h432_w576h288", 2446, 3070},
  { "300x300", "c8x10-div2", 2446, 3070},
  { "300x300", "w576h864", 2446, 3636},
  { "300x300", "w576h864-div2", 2446, 3672},
  { "300x300", "w576h864-div3", 2446, 3707},
};

LIST(dyesub_printsize_list_t, shinko_chcs1245_printsize_list, dyesub_printsize_t, shinko_chcs1245_printsize);

static const overcoat_t shinko_chcs1245_overcoat[] =
{
  {"PrinterDefault",  N_("Printer Default"),  {4, "\x01\x00\x00\x00"}},
  {"Glossy",  N_("Glossy"),  {4, "\x02\x00\x00\x00"}},
  {"GlossyFine",  N_("Glossy Fine"),  {4, "\x03\x00\x00\x00"}},
  {"Matte",  N_("Matte"),  {4, "\x04\x00\x00\x00"}},
  {"MatteFine",  N_("Matte Fine"),  {4, "\x05\x00\x00\x00"}},
};

LIST(overcoat_list_t, shinko_chcs1245_overcoat_list, overcoat_t, shinko_chcs1245_overcoat);

static const dyesub_stringitem_t shinko_chcs1245_dusts[] =
{
  { "PrinterDefault",      N_ ("Printer Default") },
  { "Off", N_ ("Off") },
  { "On", N_ ("On") }
};
LIST(dyesub_stringlist_t, shinko_chcs1245_dust_list, dyesub_stringitem_t, shinko_chcs1245_dusts);

static const stp_parameter_t shinko_chcs1245_parameters[] =
{
  {
    "DustRemoval", N_("Dust Removal"), "Color=No,Category=Advanced Printer Setup",
    N_("Print Speed"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MatteIntensity", N_("Matte Intensity"), "Color=No,Category=Advanced Printer Setup",
    N_("Strength of matte lamination pattern (-25 through +25)"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define shinko_chcs1245_parameter_count (sizeof(shinko_chcs1245_parameters) / sizeof(const stp_parameter_t))

static int
shinko_chcs1245_load_parameters(const stp_vars_t *v, const char *name,
			   stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "DustRemoval") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &shinko_chcs1245_dust_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "MatteIntensity") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -25;
      description->bounds.integer.upper = 25;
      description->is_active = 1;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int shinko_chcs1245_parse_parameters(stp_vars_t *v)
{
  const char *dust = stp_get_string_parameter(v, "DustRemoval");
  dyesub_privdata_t *pd = get_privdata(v);

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  /* Parse options */

  if (strcmp(dust, "PrinterDefault") == 0) {
     pd->privdata.s1245.dust_removal = 3;
  } else if (strcmp(dust, "Off") == 0) {
     pd->privdata.s1245.dust_removal = 1;
  } else if (strcmp(dust, "On") == 0) {
     pd->privdata.s1245.dust_removal = 2;
  } else {
     pd->privdata.s1245.dust_removal = 0;
  }

  pd->privdata.s1245.matte_intensity = stp_get_int_parameter(v, "MatteIntensity");

  return 1;
}

static void shinko_chcs1245_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  int media = 0;

  if (strcmp(pd->pagesize,"w288h576") == 0)
    media = 5;
  else if (strcmp(pd->pagesize,"w360h576") == 0)
    media = 4;
  else if (strcmp(pd->pagesize,"w432h576") == 0)
    media = 6;
  else if (strcmp(pd->pagesize,"w576h576") == 0)
    media = 9;
  else if (strcmp(pd->pagesize,"w576h576-div2") == 0)
    media = 2;
  else if (strcmp(pd->pagesize,"c8x10") == 0)
    media = 0;
  else if (strcmp(pd->pagesize,"c8x10-w576h432_w576h288") == 0)
    media = 3;
  else if (strcmp(pd->pagesize,"c8x10-div2") == 0)
    media = 1;
  else if (strcmp(pd->pagesize,"w576h864") == 0)
    media = 0;
  else if (strcmp(pd->pagesize,"w576h864-div2") == 0)
    media = 7;
  else if (strcmp(pd->pagesize,"w576h864-div3") == 0)
    media = 8;

  stp_put32_le(0x10, v);
  stp_put32_le(1245, v);  /* Printer Model */
  stp_put32_le(0x00, v);
  stp_put32_le(0x01, v);

  stp_put32_le(0x64, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x10, v);  /* Seems to be fixed */
  stp_put32_le(0x00, v);

  stp_put32_le(media, v);
  stp_zfwrite((pd->overcoat->seq).data, 1,
	      (pd->overcoat->seq).bytes, v); /* Print Mode */
  stp_put32_le(0x00, v);
  if (((const unsigned char*)(pd->overcoat->seq).data)[0] == 0x02 ||
      ((const unsigned char*)(pd->overcoat->seq).data)[0] == 0x03) {
    stp_put32_le(0x07fffffff, v);  /* Glossy */
  } else {
    stp_put32_le(pd->privdata.s1245.matte_intensity, v);  /* matte intensity */
  }

  stp_put32_le(pd->privdata.s1245.dust_removal, v); /* Dust Removal Mode */
  stp_put32_le(pd->w_size, v); /* Columns */
  stp_put32_le(pd->h_size, v); /* Rows */
  stp_put32_le(pd->copies, v); /* Copies */

  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(pd->w_dpi, v);  /* Dots Per Inch */
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
}

/* Shinko CHC-S6245 */
static const dyesub_pagesize_t shinko_chcs6245_page[] =
{
  DEFINE_PAPER_SIMPLE( "w288h576", "8x4", PT1(1236,300), PT1(2464,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h576", "8x5", PT1(1536,300), PT1(2464,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w432h576", "8x6", PT1(1836,300), PT1(2464,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w576h576", "8x8", PT1(2436,300), PT1(2464,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w576h576-div2", "8x4*2", PT1(2464,300), PT1(2494,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "c8x10", "8x10", PT1(2464,300), PT1(3036,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "c8x10-div2", "8x5*2", PT1(2464,300), PT1(3094,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w576h864", "8x12", PT1(2464,300), PT1(3636,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w576h864-div2", "8x6*2", PT1(2464,300), PT1(3694,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w576h864-div3", "8x4*3", PT1(2464,300), PT1(3742,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, shinko_chcs6245_page_list, dyesub_pagesize_t, shinko_chcs6245_page);

static const dyesub_printsize_t shinko_chcs6245_printsize[] =
{
  { "300x300", "w288h576", 1236, 2464},
  { "300x300", "w360h576", 1536, 2464},
  { "300x300", "w432h576", 1836, 2464},
  { "300x300", "w576h576", 2436, 2464},
  { "300x300", "w576h576-div2", 2464, 2494},
  { "300x300", "c8x10", 2464, 3036},
  { "300x300", "c8x10-div2", 2464, 3094},
  { "300x300", "w576h864", 2464, 3636},
  { "300x300", "w576h864-div2", 2464, 3694},
  { "300x300", "w576h864-div3", 2464, 3742},
};

LIST(dyesub_printsize_list_t, shinko_chcs6245_printsize_list, dyesub_printsize_t, shinko_chcs6245_printsize);

static const overcoat_t shinko_chcs6245_overcoat[] =
{
  {"Glossy", N_("Glossy"), {4, "\x03\x00\x00\x00"}},
  {"Matte", N_("Matte"), {4, "\x02\x00\x00\x00"}},
  {"None", N_("None"), {4, "\x01\x00\x00\x00"}},
};

LIST(overcoat_list_t, shinko_chcs6245_overcoat_list, overcoat_t, shinko_chcs6245_overcoat);

static void shinko_chcs6245_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  int media = 0;

  if (strcmp(pd->pagesize,"w288h576") == 0)
    media = 0x20;
  else if (strcmp(pd->pagesize,"w360h576") == 0)
    media = 0x21;
  else if (strcmp(pd->pagesize,"w432h576") == 0)
    media = 0x22;
  else if (strcmp(pd->pagesize,"w576h576") == 0)
    media = 0x23;
  else if (strcmp(pd->pagesize,"c8x10") == 0)
    media = 0x10;
  else if (strcmp(pd->pagesize,"w576h864") == 0)
    media = 0x11;
  else if (strcmp(pd->pagesize,"w576h576-div2") == 0)
    media = 0x30;
  else if (strcmp(pd->pagesize,"c8x10-div2") == 0)
    media = 0x31;
  else if (strcmp(pd->pagesize,"w576h864-div2") == 0)
    media = 0x32;
  else if (strcmp(pd->pagesize,"w576h864-div3") == 0)
    media = 0x40;

  stp_put32_le(0x10, v);
  stp_put32_le(6245, v);  /* Printer Model */
  stp_put32_le(0x01, v);
  stp_put32_le(0x01, v);

  stp_put32_le(0x64, v);
  stp_put32_le(0x00, v);
  stp_put32_le(media, v);
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_zfwrite((pd->overcoat->seq).data, 1,
	      (pd->overcoat->seq).bytes, v); /* Lamination */
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
  stp_put32_le(pd->w_size, v); /* Columns */
  stp_put32_le(pd->h_size, v); /* Rows */
  stp_put32_le(pd->copies, v); /* Copies */

  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(pd->w_dpi, v);  /* Dots Per Inch */
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
}

/* Shinko CHC-S6145 */
static const dyesub_pagesize_t shinko_chcs6145_page[] =
{
  DEFINE_PAPER_SIMPLE( "w144h432",	"2x6", PT1(634,300), PT1(1844,300),	DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432",	"4x6", PT1(1240,300), PT1(1844,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1240,300), PT1(1844,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h360",	"5x5", PT1(1536,300), PT1(1548,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504",	"5x7", PT1(1548,300), PT1(2140,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h432",	"6x6", PT1(1832,300), PT1(1844,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w432h576",	"6x8", PT1(1844,300), PT1(2434,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576-w432h432_w432h144", "6x6+2x6", PT1(1844,300), PT1(2434,300),DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576-div2", "4x6*2", PT1(1844,300), PT1(2492,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h648", "6x9", PT1(1844,300), PT1(2740,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, shinko_chcs6145_page_list, dyesub_pagesize_t, shinko_chcs6145_page);

static const dyesub_printsize_t shinko_chcs6145_printsize[] =
{
  { "300x300", "w144h432", 634, 1844},
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "w288h432-div2", 1240, 1844},
  { "300x300", "w360h360", 1536, 1548},
  { "300x300", "w360h504", 1548, 2140},
  { "300x300", "w432h432", 1832, 1844},
  { "300x300", "w432h576", 1844, 2434},
  { "300x300", "w432h576-w432h432_w432h144", 1844, 2434},
  { "300x300", "w432h576-div2", 1844, 2492},
  { "300x300", "w432h648", 1844, 2740},
};

LIST(dyesub_printsize_list_t, shinko_chcs6145_printsize_list, dyesub_printsize_t, shinko_chcs6145_printsize);

static const overcoat_t shinko_chcs6145_overcoat[] =
{
  {"PrinterDefault",  N_("Printer Default"),  {4, "\0\0\0\0"}},
  {"None",  N_("None"),  {4, "\x01\0\0\0"}},
  {"Glossy",  N_("Glossy"),  {4, "\x02\0\0\0"}},
  {"Matte",  N_("Matte"),  {4, "\x03\0\0\0"}},
};

LIST(overcoat_list_t, shinko_chcs6145_overcoat_list, overcoat_t, shinko_chcs6145_overcoat);

static void shinko_chcs6145_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  int media = 0;

  if (strcmp(pd->pagesize,"w288h432") == 0)
    media = 0x00;
  else if (strcmp(pd->pagesize,"w288h432-div2") == 0)
    media = 0x00;
  else if (strcmp(pd->pagesize,"w360h360") == 0)
    media = 0x08;
  else if (strcmp(pd->pagesize,"w360h504") == 0)
    media = 0x03;
  else if (strcmp(pd->pagesize,"w432h432") == 0)
    media = 0x06;
  else if (strcmp(pd->pagesize,"w432h576") == 0)
    media = 0x06;
  else if (strcmp(pd->pagesize,"w144h432") == 0)
    media = 0x07;
  else if (strcmp(pd->pagesize,"w432h576-w432h432_w432h144") == 0)
    media = 0x06;
  else if (strcmp(pd->pagesize,"w432h576-div2") == 0)
    media = 0x06;
  else if (strcmp(pd->pagesize,"w432h648") == 0)
    media = 0x05;

  stp_put32_le(0x10, v);
  stp_put32_le(6145, v);  /* Printer Model */
  if (!strcmp(pd->pagesize,"w360h360") ||
      !strcmp(pd->pagesize,"w360h504"))
	  stp_put32_le(0x02, v); /* 5" media */
  else
	  stp_put32_le(0x03, v); /* 6" media */
  stp_put32_le(0x01, v);

  stp_put32_le(0x64, v);
  stp_put32_le(0x00, v);
  stp_put32_le(media, v);  /* Media Type */
  stp_put32_le(0x00, v);

  if (strcmp(pd->pagesize,"w432h576-w432h432_w432h144") == 0) {
    stp_put32_le(0x05, v);
  } else if (strcmp(pd->pagesize,"w288h432-div2") == 0) {
    stp_put32_le(0x04, v);
  } else if (strcmp(pd->pagesize,"w432h576-div2") == 0) {
    stp_put32_le(0x02, v);
  } else {
    stp_put32_le(0x00, v);
  }
  stp_put32_le(0x00, v);  /* XXX quality; 00 == default, 0x01 == std */
  stp_zfwrite((pd->overcoat->seq).data, 1,
	      (pd->overcoat->seq).bytes, v); /* Lamination */
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
  stp_put32_le(pd->w_size, v); /* Columns */
  stp_put32_le(pd->h_size, v); /* Rows */
  stp_put32_le(pd->copies, v); /* Copies */

  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(pd->w_dpi, v);  /* Dots Per Inch */
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);

#ifdef S6145_YMC
  stp_put32_le(0x01, v);
#else
  stp_put32_le(0x00, v);
#endif
}

static void shinko_chcs2245_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  int media = 0;

  if (strcmp(pd->pagesize,"w288h432") == 0)
    media = 0x00;
  else if (strcmp(pd->pagesize,"w288h432-div2") == 0)
    media = 0x00;
  else if (strcmp(pd->pagesize,"w360h360") == 0)
    media = 0x08;
  else if (strcmp(pd->pagesize,"w360h504") == 0)
    media = 0x03;
  else if (strcmp(pd->pagesize,"w432h432") == 0)
    media = 0x06;
  else if (strcmp(pd->pagesize,"w432h576") == 0)
    media = 0x06;
  else if (strcmp(pd->pagesize,"w144h432") == 0)
    media = 0x07;
  else if (strcmp(pd->pagesize,"w432h576-w432h432_w432h144") == 0)
    media = 0x06;
  else if (strcmp(pd->pagesize,"w432h576-div2") == 0)
    media = 0x06;
  else if (strcmp(pd->pagesize,"w432h648") == 0)
    media = 0x05;

  stp_put32_le(0x10, v);
  stp_put32_le(2245, v);  /* Printer Model */
  if (!strcmp(pd->pagesize,"w360h360") ||
      !strcmp(pd->pagesize,"w360h504"))
	  stp_put32_le(0x02, v); /* 5" media */
  else
	  stp_put32_le(0x03, v); /* 6" media */
  stp_put32_le(0x01, v);

  stp_put32_le(0x64, v);
  stp_put32_le(0x00, v);
  stp_put32_le(media, v);  /* Media Type */
  stp_put32_le(0x00, v);

  if (strcmp(pd->pagesize,"w432h576-w432h432_w432h144") == 0) {
    stp_put32_le(0x05, v);
  } else if (strcmp(pd->pagesize,"w288h432-div2") == 0) {
    stp_put32_le(0x04, v);
  } else if (strcmp(pd->pagesize,"w432h576-div2") == 0) {
    stp_put32_le(0x02, v);
  } else {
    stp_put32_le(0x00, v);
  }
  stp_put32_le(0x00, v);  /* XXX quality; 00 == default, 0x01 == std */
  stp_zfwrite((pd->overcoat->seq).data, 1,
	      (pd->overcoat->seq).bytes, v); /* Lamination */
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
  stp_put32_le(pd->w_size, v); /* Columns */
  stp_put32_le(pd->h_size, v); /* Rows */
  stp_put32_le(pd->copies, v); /* Copies */

  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(pd->w_dpi, v);  /* Dots Per Inch */
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);

#ifdef S6145_YMC
  stp_put32_le(0x01, v);
#else
  stp_put32_le(0x00, v);
#endif
}

/* Ciaat Brava 21 */
static const dyesub_pagesize_t ciaat_brava21_page[] =
{
  DEFINE_PAPER_SIMPLE( "w144h432",	"2x6", PT1(634,300), PT1(1844,300),	DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432",	"4x6", PT1(1240,300), PT1(1844,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w288h432-div2", "2x6*2", PT1(1240,300), PT1(1844,300),	DYESUB_LANDSCAPE),
  DEFINE_PAPER_SIMPLE( "w360h504",	"5x7", PT1(1548,300), PT1(2140,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576",	"6x8", PT1(1844,300), PT1(2434,300), DYESUB_PORTRAIT),
  DEFINE_PAPER_SIMPLE( "w432h576-div2",	"4x6*2", PT1(1844,300), PT1(2492,300), DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, ciaat_brava21_page_list, dyesub_pagesize_t, ciaat_brava21_page);

static const dyesub_printsize_t ciaat_brava21_printsize[] =
{
  { "300x300", "w144h432", 634, 1844},
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "w288h432-div2", 1240, 1844},
  { "300x300", "w360h504", 1548, 2140},
  { "300x300", "w432h576", 1844, 2434},
  { "300x300", "w432h576-div2", 1844, 2492},
};

LIST(dyesub_printsize_list_t, ciaat_brava21_printsize_list, dyesub_printsize_t, ciaat_brava21_printsize);

/* Dai Nippon Printing DS40 */
static const dyesub_resolution_t res_dnpds40_dpi[] =
{
  { "300x300", 300, 300},
  { "300x600", 300, 600},
};

LIST(dyesub_resolution_list_t, res_dnpds40_dpi_list, dyesub_resolution_t, res_dnpds40_dpi);

/* Imaging area is wider than print size, we always must supply the printer with the full imaging width. */
static const dyesub_pagesize_t dnpds40_page[] =
{
  DEFINE_PAPER( "B7", "3.5x5", PT1(1088,300), PT1(1920,300), 0, 0, PT(186,300), PT(186,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w288h432", "4x6", PT1(1240,300), PT1(1920,300), 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w288h432-div2", "2x6*2", PT1(1240,300), PT1(1920,300), 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w360h504", "5x7", PT1(1920,300), PT1(2138,300), PT(186,300), PT(186,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w360h504-div2", "3.5x5*2", PT1(1920,300), PT1(2176,300), PT(186,300), PT(186,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h432", "6x6", PT1(1836,300), PT1(1920,300), 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w432h576", "6x8", PT1(1920,300), PT1(2436,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-div4", "2x6*4", PT1(1920,300), PT1(2436,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-div2", "4x6*2", PT1(1920,300), PT1(2498,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-w432h432_w432h144", "6x6+2x6", PT1(1920,300), PT1(2436,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h648", "6x9", PT1(1920,300), PT1(2740,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, dnpds40_page_list, dyesub_pagesize_t, dnpds40_page);

static const dyesub_printsize_t dnpds40_printsize[] =
{
  { "300x300", "B7", 1088, 1920},
  { "300x600", "B7", 2176, 1920},
  { "300x300", "w288h432", 1240, 1920},
  { "300x600", "w288h432", 2480, 1920},
  { "300x300", "w288h432-div2", 1240, 1920},
  { "300x600", "w288h432-div2", 2480, 1920},
  { "300x300", "w360h504", 1920, 2138},
  { "300x600", "w360h504", 1920, 4276},
  { "300x300", "w360h504-div2", 1920, 2176},
  { "300x600", "w360h504-div2", 1920, 4352},
  { "300x300", "w432h432", 1836, 1920},
  { "300x600", "w432h432", 3672, 1920},
  { "300x300", "w432h576", 1920, 2436},
  { "300x600", "w432h576", 1920, 4872},
  { "300x300", "w432h576-div4", 1920, 2436},
  { "300x600", "w432h576-div4", 1920, 4872},
  { "300x300", "w432h576-div2", 1920, 2498},
  { "300x600", "w432h576-div2", 1920, 4996},
  { "300x300", "w432h576-w432h432_w432h144", 1920, 2436},
  { "300x600", "w432h576-w432h432_w432h144", 1920, 4872},
  { "300x300", "w432h648", 1920, 2740},
  { "300x600", "w432h648", 1920, 5480},
};

LIST(dyesub_printsize_list_t, dnpds40_printsize_list, dyesub_printsize_t, dnpds40_printsize);

static const overcoat_t dnpds40_overcoat[] =
{
  {"Glossy",  N_("Glossy"),  {3, "000"}},
  {"Matte", N_("Matte"), {3, "001"}},
};

LIST(overcoat_list_t, dnpds40_overcoat_list, overcoat_t, dnpds40_overcoat);

static const stp_parameter_t ds40_parameters[] =
{
  {
    "NoCutWaste", N_("No Cut-Paper Waste"), "Color=No,Category=Advanced Printer Setup",
    N_("No Cut-Paper Waste"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 0, STP_CHANNEL_NONE, 1, 0
  },
};

#define ds40_parameter_count (sizeof(ds40_parameters) / sizeof(const stp_parameter_t))

static int
ds40_load_parameters(const stp_vars_t *v, const char *name,
		     stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
					stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "NoCutWaste") == 0)
    {
      description->is_active = 1;
      description->deflt.boolean = 0;
    }
  else
    {
     return 0;
    }

  return 1;
}

static int ds40_parse_parameters(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  int nocutwaste = stp_get_boolean_parameter(v, "NoCutWaste");

  if (pd) {
    pd->privdata.dnp.nocutwaste = nocutwaste;
  }

  return 1;
}

static void dnp_printer_start_common(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Configure Lamination */
  stp_zprintf(v, "\033PCNTRL OVERCOAT        0000000800000");
  stp_zfwrite((pd->overcoat->seq).data, 1,
	      (pd->overcoat->seq).bytes, v); /* Lamination mode */

  /* Set quantity.. Backend overrides as needed. */
  stp_zprintf(v, "\033PCNTRL QTY             00000008%07d\r", pd->copies);
}

static void dnpds40_printer_start(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Common code */
  dnp_printer_start_common(v);

  /* Set cutter option to "normal" */
  if (!strcmp(pd->pagesize, "w432h576-w432h432_w432h144")) {
    stp_zprintf(v, "\033PCNTRL FULL_CUTTER_SET 00000016");
    stp_zprintf(v, "060020000000000\r");
  } else if (!strcmp(pd->pagesize, "w288h432-div2") ||
	     !strcmp(pd->pagesize, "w432h576-div4")) {
    stp_zprintf(v, "\033PCNTRL CUTTER          00000008");
    stp_zprintf(v, "00000120");
  } else if (pd->privdata.dnp.nocutwaste) {
    stp_zprintf(v, "\033PCNTRL CUTTER          00000008");
    stp_zprintf(v, "00000001");
  } else {
    stp_zprintf(v, "\033PCNTRL CUTTER          00000008");
    stp_zprintf(v, "00000000");
  }

  /* Configure multi-cut/page size */
  stp_zprintf(v, "\033PIMAGE MULTICUT        00000008000000");

  if (!strcmp(pd->pagesize, "B7")) {
    stp_zprintf(v, "01");
  } else if (!strcmp(pd->pagesize, "w288h432")) {
    stp_zprintf(v, "02");
  } else if (!strcmp(pd->pagesize, "w360h504")) {
    stp_zprintf(v, "03");
  } else if (!strcmp(pd->pagesize, "w360h504-div2")) {
    stp_zprintf(v, "22");
  } else if (!strcmp(pd->pagesize, "w432h432")) {
    stp_zprintf(v, "27");
  } else if (!strcmp(pd->pagesize, "w432h576")) {
    stp_zprintf(v, "04");
  } else if (!strcmp(pd->pagesize, "w432h576-w432h432_w432h144")) {
    stp_zprintf(v, "04");
  } else if (!strcmp(pd->pagesize, "w432h648")) {
    stp_zprintf(v, "05");
  } else if (!strcmp(pd->pagesize, "w432h576-div2")) {
    stp_zprintf(v, "12");
  } else if (!strcmp(pd->pagesize, "w288h432-div2")) {
    stp_zprintf(v, "02");
  } else if (!strcmp(pd->pagesize, "w432h576-div4")) {
    stp_zprintf(v, "04");
  } else {
    stp_zprintf(v, "00"); /* should be impossible. */
  }
}

static void dnpds40_printer_end(stp_vars_t *v)
{
  stp_zprintf(v, "\033PCNTRL START"); dyesub_nputc(v, ' ', 19);
}

static void dnpds40_plane_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  char p = (pd->plane == 3 ? 'Y' :
	    (pd->plane == 2 ? 'M' :
	     'C' ));

  long PadSize = 10;
  long FSize = (pd->w_size*pd->h_size) + 1024 + 54 + PadSize;

  /* Printer command plus length of data to follow */
  stp_zprintf(v, "\033PIMAGE %cPLANE          %08ld", p, FSize);

  /* Each plane is essentially a tweaked BMP file */

  /* BMP header */
  stp_zprintf(v, "BM");
  stp_put32_le(FSize, v);
  dyesub_nputc(v, '\0', 4);
  stp_put32_le(1088, v);  /* Offset to pixel data: 1024 + (14+40-10) + 10 */

  /* DIB header */
  stp_put32_le(40, v); /* DIB header size */
  stp_put32_le(pd->w_size, v);
  stp_put32_le(pd->h_size, v);
  stp_put16_le(1, v); /* single channel */
  stp_put16_le(8, v); /* 8bpp */
  dyesub_nputc(v, '\0', 8); /* compression + image size are ignored */
  stp_put32_le(11808, v); /* horizontal pixels per meter, fixed at 300dpi */
  if (pd->h_dpi == 600)
    stp_put32_le(23615, v); /* vertical pixels per meter @ 600dpi */
  else if (pd->h_dpi == 334)
    stp_put32_le(13146, v); /* vertical pixels per meter @ 334dpi */
  else
    stp_put32_le(11808, v); /* vertical pixels per meter @ 300dpi */
  stp_put32_le(256, v);    /* entries in color table  */
  stp_put32_le(0, v);      /* no important colors */
  dyesub_nputc(v, '\0', 1024);    /* RGB Array, unused by printer */
  dyesub_nputc(v, '\0', PadSize); /* Pading to align plane data */
}

/* Dai Nippon Printing DS80 */

/* Imaging area is wider than print size, we always must supply the
   printer with the full imaging width. */
static const dyesub_pagesize_t dnpds80_page[] =
{
  DEFINE_PAPER( "w288h576", "8x4", PT1(1236,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w360h576", "8x5", PT1(1536,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w432h576", "8x6", PT1(1836,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w576h576", "8x8", PT1(2436,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w576h576-div2", "8x4*2", PT1(2502,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w576h648-w576h360_w576h288", "8x5+8x4", PT1(2560,300), PT1(2802,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10", "8x10", PT1(2560,300), PT1(3036,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10-div2", "8x5*2", PT1(2560,300), PT1(3102,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10-w576h432_w576h288", "8x6+8x4", PT1(2560,300), PT1(3102,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h792-w576h432_w576h360", "8x6+8x5", PT1(2560,300), PT1(3402,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h842", "8x11.7", PT1(2560,300), PT1(3544,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864", "8x12", PT1(2560,300), PT1(3636,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864-div2", "8x6*2", PT1(2560,300), PT1(3702,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864-w576h576_w576h288", "8x8+8x4", PT1(2560,300), PT1(3702,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864-div3", "8x4*3", PT1(2560,300), PT1(3768,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, dnpds80_page_list, dyesub_pagesize_t, dnpds80_page);

static const dyesub_printsize_t dnpds80_printsize[] =
{
  { "300x300", "w288h576", 1236, 2560},
  { "300x600", "w288h576", 2472, 2560},
  { "300x300", "w360h576", 1536, 2560},
  { "300x600", "w360h576", 3072, 2560},
  { "300x300", "w432h576", 1836, 2560},
  { "300x600", "w432h576", 3672, 2560},
  { "300x300", "w576h576", 2436, 2560},
  { "300x600", "w576h576", 4872, 2560},
  { "300x300", "w576h576-div2", 2502, 2560},
  { "300x600", "w576h576-div2", 5004, 2560},
  { "300x300", "w576h648-w576h360_w576h288", 2560, 2802},
  { "300x600", "w576h648-w576h360_w576h288", 2560, 5604},
  { "300x300", "c8x10", 2560, 3036},
  { "300x600", "c8x10", 2560, 6072},
  { "300x300", "c8x10-div2", 2560, 3102},
  { "300x600", "c8x10-div2", 2560, 6204},
  { "300x300", "c8x10-w576h432_w576h288", 2560, 3102},
  { "300x600", "c8x10-w576h432_w576h288", 2560, 6204},
  { "300x300", "w576h792-w576h432_w576h360", 2560, 3402},
  { "300x600", "w576h792-w576h432_w576h360", 2560, 6804},
  { "300x300", "w576h842", 2560, 3544},
  { "300x600", "w576h842", 2560, 7088},
  { "300x300", "w576h864", 2560, 3636},
  { "300x600", "w576h864", 2560, 7272},
  { "300x300", "w576h864-div2", 2560, 3702},
  { "300x600", "w576h864-div2", 2560, 7404},
  { "300x300", "w576h864-w576h576_w576h288", 2560, 3702},
  { "300x600", "w576h864-w576h576_w576h288", 2560, 7404},
  { "300x300", "w576h864-div3", 2560, 3768},
  { "300x600", "w576h864-div3", 2560, 7536},
};

LIST(dyesub_printsize_list_t, dnpds80_printsize_list, dyesub_printsize_t, dnpds80_printsize);

static int dnpds80_parse_parameters(stp_vars_t *v)
{
  const char *pagesize = stp_get_string_parameter(v, "PageSize");
  dyesub_privdata_t *pd = get_privdata(v);
  int multicut = 0;
  int nocutwaste = stp_get_boolean_parameter(v, "NoCutWaste");

  if (!strcmp(pagesize, "c8x10")) {
    multicut = 6;
  } else if (!strcmp(pagesize, "w576h864")) {
    multicut = 7;
  } else if (!strcmp(pagesize, "w288h576")) {
    multicut = 8;
  } else if (!strcmp(pagesize, "w360h576")) {
    multicut = 9;
  } else if (!strcmp(pagesize, "w432h576")) {
    multicut = 10;
  } else if (!strcmp(pagesize, "w576h576")) {
    multicut = 11;
  } else if (!strcmp(pagesize, "w576h576-div2")) {
    multicut = 13;
  } else if (!strcmp(pagesize, "c8x10-div2")) {
    multicut = 14;
  } else if (!strcmp(pagesize, "w576h864-div2")) {
    multicut = 15;
  } else if (!strcmp(pagesize, "w576h648-w576h360_w576h288")) {
    multicut = 16;
  } else if (!strcmp(pagesize, "c8x10-w576h432_w576h288")) {
    multicut = 17;
  } else if (!strcmp(pagesize, "w576h792-w576h432_w576h360")) {
    multicut = 18;
  } else if (!strcmp(pagesize, "w576h864-w576h576_w576h288")) {
    multicut = 19;
  } else if (!strcmp(pagesize, "w576h864-div3")) {
    multicut = 20;
  } else if (!strcmp(pagesize, "w576h842")) {
    multicut = 21;
  } else {
    stp_eprintf(v, _("Illegal print size selected for roll media!\n"));
    return 0;
  }

  /* No need to set global params if there's no privdata yet */
  if (pd) {
    pd->privdata.dnp.multicut = multicut;
    pd->privdata.dnp.nocutwaste = nocutwaste;
  }

 return 1;
}

static void dnpds80_printer_start(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Common code */
  dnp_printer_start_common(v);

  /* Set cutter option to "normal" */
  stp_zprintf(v, "\033PCNTRL CUTTER          00000008");
  if (pd->privdata.dnp.nocutwaste)
    stp_zprintf(v, "00000001");
  else
    stp_zprintf(v, "00000000");

  /* Configure multi-cut/page size */
  stp_zprintf(v, "\033PIMAGE MULTICUT        00000008%08d", pd->privdata.dnp.multicut);
}

/* Dai Nippon Printing DS80DX */
static const dyesub_media_t dnpds80dx_medias[] =
{
  {"Roll",  N_("Roll"), {0, ""}},
  {"Sheet", N_("Sheet"), {0, ""}},
};

LIST(dyesub_media_list_t, dnpds80dx_media_list, dyesub_media_t, dnpds80dx_medias);

static int dnpds80dx_parse_parameters(stp_vars_t *v)
{
  const char *pagesize;
  const dyesub_media_t* media = NULL;
  const char* duplex_mode;
  dyesub_privdata_t *pd = get_privdata(v);
  int multicut = 0;
  int nocutwaste;
  int pagenum;

  pagesize = stp_get_string_parameter(v, "PageSize");
  duplex_mode = stp_get_string_parameter(v, "Duplex");
  media = dyesub_get_mediatype(v);
  nocutwaste = stp_get_boolean_parameter(v, "NoCutWaste");
  pagenum = stp_get_int_parameter(v, "PageNumber");

  if (!strcmp(media->name, "Roll")) {
    if (strcmp(duplex_mode, "None") && strcmp(duplex_mode, "Standard")) {
      stp_eprintf(v, _("Duplex not supported on roll media, switching to sheet media!\n"));
      stp_set_string_parameter(v, "MediaType", "Sheet");
    } else {
      /* If we're not using duplex and roll media, this is
	 effectively a DS80 (non-DX) */
      return dnpds80_parse_parameters(v);
    }
  }

  if (!strcmp(pagesize, "c8x10")) {
    multicut = 6;
  } else if (!strcmp(pagesize, "w576h864")) {
    multicut = 7;
  } else if (!strcmp(pagesize, "w288h576")) {
    multicut = 8;
  } else if (!strcmp(pagesize, "w360h576")) {
    multicut = 9;
  } else if (!strcmp(pagesize, "w432h576")) {
    multicut = 10;
  } else if (!strcmp(pagesize, "w576h576")) {
    multicut = 11;
  } else if (!strcmp(pagesize, "w576h774-w576h756")) {
    multicut = 25;
  } else if (!strcmp(pagesize, "w576h774")) {
    multicut = 26;
  } else if (!strcmp(pagesize, "w576h576-div2")) {
    multicut = 13;
  } else if (!strcmp(pagesize, "c8x10-div2")) {
    multicut = 14;
  } else if (!strcmp(pagesize, "w576h864-div2")) {
    multicut = 15;
  } else if (!strcmp(pagesize, "w576h864-div3sheet")) {
    multicut = 28;
  } else {
    stp_eprintf(v, _("Illegal print size selected for sheet media!\n"));
    return 0;
  }

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  /* Add correct offset to multicut mode based on duplex state */
  if (!strcmp(duplex_mode, "None") || !strcmp(duplex_mode, "Standard"))
     multicut += 100; /* Simplex */
  else if (pagenum & 1)
     multicut += 300; /* Duplex, back */
  else
     multicut += 200; /* Duplex, front */

  pd->privdata.dnp.multicut = multicut;
  pd->privdata.dnp.nocutwaste = nocutwaste;

  return 1;
}

/* This is the same as the DS80, except with 10.5" and 10.75" sizes
   only meant for sheet media.  Duplex is *only* supported on sheet media.

   Also, 8x4*3 differs depending on if you're using sheet or roll media,
   hence the almost-duplicated definition.

*/

static const dyesub_pagesize_t dnpds80dx_page[] =
{
  DEFINE_PAPER( "w288h576", "8x4", PT1(1236,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w360h576", "8x5", PT1(1536,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w432h576", "8x6", PT1(1836,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w576h576", "8x8", PT1(2436,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w576h576-div2", "8x4*2", PT1(2502,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w576h648-w576h360_w576h288", "8x5+8x4", PT1(2560,300), PT1(2802,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10", "8x10", PT1(2560,300), PT1(3036,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10-div2", "8x5*2", PT1(2560,300), PT1(3102,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10-w576h432_w576h288", "8x6+8x4", PT1(2560,300), PT1(3102,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h774-w576h756", "8x10.5 SHEET", PT1(2560,300), PT1(3186,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h774", "8x10.75 SHEET", PT1(2560,300), PT1(3186,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h792-w576h432_w576h360", "8x6+8x5", PT1(2560,300), PT1(3402,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h842", "8x11.7", PT1(2560,300), PT1(3544,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864", "8x12", PT1(2560,300), PT1(3636,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864-div2", "8x6*2", PT1(2560,300), PT1(3702,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864-w576h576_w576h288", "8x8+8x4", PT1(2560,300), PT1(3702,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864-div3", "8x4*3", PT1(2560,300), PT1(3768,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864-div3sheet", "8x4*3 SHEET", PT1(2560,300), PT1(3702,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, dnpds80dx_page_list, dyesub_pagesize_t, dnpds80dx_page);

static const dyesub_printsize_t dnpds80dx_printsize[] =
{
  { "300x300", "w288h576", 1236, 2560},
  { "300x600", "w288h576", 2472, 2560},
  { "300x300", "w360h576", 1536, 2560},
  { "300x600", "w360h576", 3072, 2560},
  { "300x300", "w432h576", 1836, 2560},
  { "300x600", "w432h576", 3672, 2560},
  { "300x300", "w576h576", 2436, 2560},
  { "300x600", "w576h576", 4872, 2560},
  { "300x300", "w576h576-div2", 2502, 2560},
  { "300x600", "w576h576-div2", 5004, 2560},
  { "300x300", "w576h648-w576h360_w576h288", 2560, 2802},
  { "300x600", "w576h648-w576h360_w576h288", 2560, 5604},
  { "300x300", "c8x10", 2560, 3036},
  { "300x600", "c8x10", 2560, 6072},
  { "300x300", "c8x10-div2", 2560, 3102},
  { "300x600", "c8x10-div2", 2560, 6204},
  { "300x300", "c8x10-w576h432_w576h288", 2560, 3102},
  { "300x600", "c8x10-w576h432_w576h288", 2560, 6204},
  { "300x300", "w576h774", 2560, 3186},
  { "300x600", "w576h774", 2560, 6372},
  { "300x300", "w576h774-w576h756", 2560, 3186},
  { "300x600", "w576h774-w576h756", 2560, 6372},
  { "300x300", "w576h792-w576h432_w576h360", 2560, 3402},
  { "300x600", "w576h792-w576h432_w576h360", 2560, 6804},
  { "300x300", "w576h842", 2560, 3544},
  { "300x600", "w576h842", 2560, 7088},
  { "300x300", "w576h864", 2560, 3636},
  { "300x600", "w576h864", 2560, 7272},
  { "300x300", "w576h864-div2", 2560, 3702},
  { "300x600", "w576h864-div2", 2560, 7404},
  { "300x300", "w576h864-w576h576_w576h288", 2560, 3702},
  { "300x600", "w576h864-w576h576_w576h288", 2560, 7404},
  { "300x300", "w576h864-div3", 2560, 3768},
  { "300x600", "w576h864-div3", 2560, 7536},
  { "300x300", "w576h864-div3sheet", 2560, 3702},
  { "300x600", "w576h864-div3sheet", 2560, 7404},
};

LIST(dyesub_printsize_list_t, dnpds80dx_printsize_list, dyesub_printsize_t, dnpds80dx_printsize);

/* Dai Nippon Printing DS-RX1 */
/* Imaging area is wider than print size, we always must supply the
   printer with the full imaging width. */
static const dyesub_pagesize_t dnpsrx1_page[] =
{
  DEFINE_PAPER( "B7", "3.5x5", PT1(1088,300), PT1(1920,300), 0, 0, PT(186,300), PT(186,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w288h432", "4x6", PT1(1240,300), PT1(1920,300), 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w288h432-div2", "2x6*2", PT1(1240,300), PT1(1920,300), 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w360h360","5x5", PT1(1540,300), PT1(1920,300), 0, 0, PT(186,300), PT(186,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w360h504",	"5x7", PT1(1920,300), PT1(2138,300), PT(186,300), PT(186,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w360h504-div2",	"3.5x5*2", PT1(1920,300), PT1(2176,300), PT(186,300), PT(186,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h432", "6x6", PT1(1836,300), PT1(1920,300), 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w432h576", "6x8", PT1(1920,300), PT1(2436,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-div4", "2x6*4", PT1(1920,300), PT1(2436,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-div2", "4x6*2", PT1(1920,300), PT1(2498,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),

};

LIST(dyesub_pagesize_list_t, dnpsrx1_page_list, dyesub_pagesize_t, dnpsrx1_page);

static const dyesub_printsize_t dnpsrx1_printsize[] =
{
  { "300x300", "B7", 1088, 1920},
  { "300x600", "B7", 2176, 1920},
  { "300x300", "w288h432", 1240, 1920},
  { "300x600", "w288h432", 2480, 1920},
  { "300x300", "w288h432-div2", 1240, 1920},
  { "300x600", "w288h432-div2", 2480, 1920},
  { "300x300", "w360h360", 1540, 1920},
  { "300x600", "w360h360", 3080, 1920},
  { "300x300", "w360h504", 1920, 2138},
  { "300x600", "w360h504", 1920, 4276},
  { "300x300", "w360h504-div2", 1920, 2176},
  { "300x600", "w360h504-div2", 1920, 4352},
  { "300x300", "w432h432", 1836, 1920},
  { "300x600", "w432h432", 3672, 1920},
  { "300x300", "w432h576", 1920, 2436},
  { "300x600", "w432h576", 1920, 4872},
  { "300x300", "w432h576-div4", 1920, 2436},
  { "300x600", "w432h576-div4", 1920, 4872},
  { "300x300", "w432h576-div2", 1920, 2498},
  { "300x600", "w432h576-div2", 1920, 4996},
};

LIST(dyesub_printsize_list_t, dnpsrx1_printsize_list, dyesub_printsize_t, dnpsrx1_printsize);

static void dnpdsrx1_printer_start(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Common code */
  dnp_printer_start_common(v);

  /* Set cutter option to "normal" */
  stp_zprintf(v, "\033PCNTRL CUTTER          00000008");
  if (!strcmp(pd->pagesize, "w288h432-div2")) {
    stp_zprintf(v, "00000120");
  } else if (!strcmp(pd->pagesize, "w432h576-div4")) {
    stp_zprintf(v, "00000120");
  } else if (pd->privdata.dnp.nocutwaste) {
    stp_zprintf(v, "00000001");
  } else {
    stp_zprintf(v, "00000000");
  }

  /* Configure multi-cut/page size */
  stp_zprintf(v, "\033PIMAGE MULTICUT        00000008000000");

  if (!strcmp(pd->pagesize, "B7")) {
    stp_zprintf(v, "01");
  } else if (!strcmp(pd->pagesize, "w288h432")) {
    stp_zprintf(v, "02");
  } else if (!strcmp(pd->pagesize, "w360h360")) {
    stp_zprintf(v, "29");
  } else if (!strcmp(pd->pagesize, "w360h504")) {
    stp_zprintf(v, "03");
  } else if (!strcmp(pd->pagesize, "w360h504-div2")) {
    stp_zprintf(v, "22");
  } else if (!strcmp(pd->pagesize, "w432h432")) {
    stp_zprintf(v, "27");
  } else if (!strcmp(pd->pagesize, "w432h576")) {
    stp_zprintf(v, "04");
  } else if (!strcmp(pd->pagesize, "w432h576-div2")) {
    stp_zprintf(v, "12");
  } else if (!strcmp(pd->pagesize, "w288h432-div2")) {
    stp_zprintf(v, "02");
  } else if (!strcmp(pd->pagesize, "w432h576-div4")) {
    stp_zprintf(v, "04");
  } else {
    stp_zprintf(v, "00");
  }
}

/* Dai Nippon Printing DS620 */
static const overcoat_t dnpds620_overcoat[] =
{
  {"Glossy",  N_("Glossy"),  {3, "000"}},
  {"Matte", N_("Matte"), {3, "001"}},
  {"MatteFine", N_("Matte Fine"), {3, "021"}},
  {"MatteLuster", N_("Matte Luster"), {3, "022"}},
};

LIST(overcoat_list_t, dnpds620_overcoat_list, overcoat_t, dnpds620_overcoat);

/* Imaging area is wider than print size, we always must supply the
   printer with the full imaging width. */
static const dyesub_pagesize_t dnpds620_page[] =
{
  DEFINE_PAPER( "B7", "3.5x5", PT1(1088,300), PT1(1920,300), 0, 0, PT(186,300), PT(186,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w288h432", "4x6", PT1(1240,300), PT1(1920,300), 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w288h432-div2", "2x6*2", PT1(1240,300), PT1(1920,300), 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w324h432", "4.5x6", PT1(1386,300), PT1(1920,300), 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w360h360",	"5x5", PT1(1540,300), PT1(1920,300), 0, 0, PT(186,300), PT(186,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w360h504",	"5x7", PT1(1920,300), PT1(2138,300), PT(186,300), PT(186,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w360h504-div2",	"3.5x5*2", PT1(1920,300), PT1(2176,300), PT(186,300), PT(186,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w360h504-w360h360_w360h144", "5x5+2x5", PT1(1920,300), PT1(2138,300), PT(186,300), PT(186,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h432",	"6x6", PT1(1836,300), PT1(1920,300), 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w432h576", "6x8", PT1(1920,300), PT1(2436,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-w432h432_w432h144", "6x6+2x6", PT1(1920,300), PT1(2436,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-div4", "2x6*4", PT1(1920,300), PT1(2436,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576-div2", "4x6*2", PT1(1920,300), PT1(2498,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h648", "6x9", PT1(1920,300), PT1(2740,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h648-div2", "4.5x6*2", PT1(1920,300), PT1(2802,300), PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, dnpds620_page_list, dyesub_pagesize_t, dnpds620_page);

static const dyesub_printsize_t dnpds620_printsize[] =
{
  { "300x300", "B7", 1088, 1920},
  { "300x600", "B7", 2176, 1920},
  { "300x300", "w288h432", 1240, 1920},
  { "300x600", "w288h432", 2480, 1920},
  { "300x300", "w288h432-div2", 1240, 1920},
  { "300x600", "w288h432-div2", 2480, 1920},
  { "300x300", "w324h432", 1386, 1920},
  { "300x600", "w324h432", 2772, 1920},
  { "300x300", "w360h360", 1540, 1920},
  { "300x600", "w360h360", 3080, 1920},
  { "300x300", "w360h504", 1920, 2138},
  { "300x600", "w360h504", 1920, 4276},
  { "300x300", "w360h504-w360h360_w360h144", 1920, 2138},
  { "300x600", "w360h504-w360h360_w360h144", 1920, 4276},
  { "300x300", "w360h504-div2", 1920, 2176},
  { "300x600", "w360h504-div2", 1920, 4352},
  { "300x300", "w432h432", 1836, 1920},
  { "300x600", "w432h432", 3672, 1920},
  { "300x300", "w432h576", 1920, 2436},
  { "300x600", "w432h576", 1920, 4872},
  { "300x300", "w432h576-div4", 1920, 2436},
  { "300x600", "w432h576-div4", 1920, 4872},
  { "300x300", "w432h576-w432h432_w432h144", 1920, 2436},
  { "300x600", "w432h576-w432h432_w432h144", 1920, 4872},
  { "300x300", "w432h576-div2", 1920, 2498},
  { "300x600", "w432h576-div2", 1920, 4996},
  { "300x300", "w432h648", 1920, 2740},
  { "300x600", "w432h648", 1920, 5480},
  { "300x300", "w432h648-div2", 1920, 2802},
  { "300x600", "w432h648-div2", 1920, 5604},
};

LIST(dyesub_printsize_list_t, dnpds620_printsize_list, dyesub_printsize_t, dnpds620_printsize);

static void dnpds620_printer_start(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  int trim = 0; /* XXX add another parameter to control this.  0 or 12-22 for intermediate scrap on fw >= 1.20. */
  int cut1 = 0, cut2 = 0, cut3 = 0, cut4 = 0;
  int multicut = 0;

  /* Common code */
  dnp_printer_start_common(v);

  /* Multicut when 8x6 media is in use */
  if (!strcmp(pd->pagesize, "w432h576-div4")) {
    cut1 = cut2 = cut3 = cut4 = 20;
  } else if (!strcmp(pd->pagesize, "w432h576-w432h432_w432h144")) {
    cut1 = 60;
    cut2 = 20;
  } else if (!strcmp(pd->pagesize, "w360h504-w360h360_w360h144")) {
    cut1 = 50;
    cut2 = 20;
  } else if (!strcmp(pd->pagesize, "w288h432-div2")) {
    cut1 = cut2 = 20;
  }

  /* Cutter */
  stp_zprintf(v, "\033PCNTRL CUTTER          00000008%08d", pd->privdata.dnp.nocutwaste ? 1 : 0);
  if (cut1) {
    stp_zprintf(v, "\033PCNTRL FULL_CUTTER_SET 00000016");
    stp_zprintf(v, "%03d%03d%03d%03d%03d\r", cut1, cut2, cut3, cut4, trim);
  }

  /* Configure multi-cut/page size */
  if (!strcmp(pd->pagesize, "B7")) {
    multicut = 1;
  } else if (!strcmp(pd->pagesize, "w288h432") || !strcmp(pd->pagesize, "w288h432-div2")) {
    multicut = 2;
  } else if (!strcmp(pd->pagesize, "w324h432")) {
    multicut = 30;
  } else if (!strcmp(pd->pagesize, "w360h360")) {
    multicut = 29;
  } else if (!strcmp(pd->pagesize, "w360h504") || !strcmp(pd->pagesize, "w360h504-w360h360_w360h144")) {
    multicut = 3;
  } else if (!strcmp(pd->pagesize, "w360h504-div2")) {
    multicut = 22;
  } else if (!strcmp(pd->pagesize, "w432h432")) {
    multicut = 27;
  } else if (!strcmp(pd->pagesize, "w432h576") || !strcmp(pd->pagesize, "w432h576-w432h432_w432h144") || !strcmp(pd->pagesize, "w432h576-div4")) {
    multicut = 4;
  } else if (!strcmp(pd->pagesize, "w432h576-div2")) {
    multicut = 12;
  } else if (!strcmp(pd->pagesize, "w432h648")) {
    multicut = 5;
  } else if (!strcmp(pd->pagesize, "w432h648-div2")) {
    multicut = 31;
  }
  stp_zprintf(v, "\033PIMAGE MULTICUT        00000008000000%02d", multicut);
}

/* Dai Nippon Printing DS820 */

/* Imaging area is wider than print size, we always must supply the
   printer with the full imaging width. */
static const dyesub_pagesize_t dnpds820_page[] =
{
  DEFINE_PAPER( "w288h576", "8x4", PT1(1236,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w360h576", "8x5", PT1(1536,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w432h576", "8x6", PT1(1836,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w504h576", "8x7", PT1(2136,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w576h576", "8x8", PT1(2436,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w576h576-div2", "8x4*2", PT1(2502,300), PT1(2560,300), 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w576h648", "8x9", PT1(2560,300), PT1(2736,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h648-w576h360_w576h288", "8x5+8x4", PT1(2560,300), PT1(2802,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10", "8x10", PT1(2560,300), PT1(3036,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10-div2", "8x5*2", PT1(2560,300), PT1(3102,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "c8x10-w576h432_w576h288", "8x6+8x4", PT1(2560,300), PT1(3102,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h792-w576h432_w576h360", "8x6+8x5", PT1(2560,300), PT1(3402,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h842", "8x11.7", PT1(2560,300), PT1(3544,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864", "8x12", PT1(2560,300), PT1(3636,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864-div2", "8x6*2", PT1(2560,300), PT1(3702,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864-w576h576_w576h288", "8x8+8x4", PT1(2560,300), PT1(3702,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w576h864-div3", "8x4*3", PT1(2560,300), PT1(3768,300), PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "A4x4inch", "A4x4inch", PT1(1236,300), PT1(2560,300), 0, 0, PT(16,300), PT(16,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "A4x5inch", "A4x5inch", PT1(1536,300), PT1(2560,300), 0, 0, PT(16,300), PT(16,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "A5", "A5", PT1(1784,300), PT1(2560,300), 0, 0, PT(16,300), PT(16,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "A4x6inch", "A4x6inch", PT1(1836,300), PT1(2560,300), 0, 0, PT(16,300), PT(16,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "A4x8inch", "A4x8inch", PT1(2436,300), PT1(2560,300), 0, 0, PT(16,300), PT(16,300), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "A4x10inch", "A4x10inch", PT1(2560,300), PT1(3036,300), PT(16,300), PT(16,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "A4x10inch-div2", "A4x5inch*2", PT1(2560,300), PT1(3102,300), PT(16,300), PT(16,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "A4", "A4", PT1(2560,300), PT1(3544,300), PT(16,300), PT(16,300), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "A4-div2", "A5*2", PT1(2560,300), PT1(3598,300), PT(16,300), PT(16,300), 0, 0, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, dnpds820_page_list, dyesub_pagesize_t, dnpds820_page);

static const dyesub_printsize_t dnpds820_printsize[] =
{
  { "300x300", "w288h576", 1236, 2560},
  { "300x600", "w288h576", 2472, 2560},
  { "300x300", "w360h576", 1536, 2560},
  { "300x600", "w360h576", 3072, 2560},
  { "300x300", "w432h576", 1836, 2560},
  { "300x600", "w432h576", 3672, 2560},
  { "300x300", "w504h576", 2136, 2560},
  { "300x600", "w504h576", 4272, 2560},
  { "300x300", "w576h576", 2436, 2560},
  { "300x600", "w576h576", 4872, 2560},
  { "300x300", "w576h576-div2", 2502, 2560},
  { "300x600", "w576h576-div2", 5004, 2560},
  { "300x300", "w576h648", 2560, 2736},
  { "300x600", "w576h648", 2560, 5472},
  { "300x300", "w576h648-w576h360_w576h288", 2560, 2802},
  { "300x600", "w576h648-w576h360_w576h288", 2560, 5604},
  { "300x300", "c8x10", 2560, 3036},
  { "300x600", "c8x10", 2560, 6072},
  { "300x300", "c8x10-div2", 2560, 3102},
  { "300x600", "c8x10-div2", 2560, 6204},
  { "300x300", "c8x10-w576h432_w576h288", 2560, 3102},
  { "300x600", "c8x10-w576h432_w576h288", 2560, 6204},
  { "300x300", "w576h792-w576h432_w576h360", 2560, 3402},
  { "300x600", "w576h792-w576h432_w576h360", 2560, 6804},
  { "300x300", "w576h842", 2560, 3544},
  { "300x600", "w576h842", 2560, 7088},
  { "300x300", "w576h864", 2560, 3636},
  { "300x600", "w576h864", 2560, 7272},
  { "300x300", "w576h864-div2", 2560, 3702},
  { "300x600", "w576h864-div2", 2560, 7404},
  { "300x300", "w576h864-w576h576_w576h288", 2560, 3702},
  { "300x600", "w576h864-w576h576_w576h288", 2560, 7404},
  { "300x300", "w576h864-div3", 2560, 3768},
  { "300x600", "w576h864-div3", 2560, 7536},
  { "300x300", "A4x4inch", 1236, 2560},
  { "300x600", "A4x4inch", 2472, 2560},
  { "300x300", "A4x5inch", 1536, 2560},
  { "300x600", "A4x5inch", 3072, 2560},
  { "300x300", "A5", 1784, 2560},
  { "300x600", "A5", 3568, 2560},
  { "300x300", "A4x6inch", 1836, 2560},
  { "300x600", "A4x6inch", 3672, 2560},
  { "300x300", "A4x8inch", 2436, 2560},
  { "300x600", "A4x8inch", 4872, 2560},
  { "300x300", "A4x10inch", 2560, 3036},
  { "300x600", "A4x10inch", 2560, 6072},
  { "300x300", "A4x10inch-div2", 2560, 3102},
  { "300x600", "A4x10inch-div2", 2560, 6204},
  { "300x300", "A4", 2560, 3544},
  { "300x600", "A4", 2560, 7088},
  { "300x300", "A4-div2", 2560, 3598},
  { "300x600", "A4-div2", 2560, 7196},
};

LIST(dyesub_printsize_list_t, dnpds820_printsize_list, dyesub_printsize_t, dnpds820_printsize);

static void dnpds820_printer_start(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  /* Common code */
  dnp_printer_start_common(v);

  /* No-cut waste */
  stp_zprintf(v, "\033PCNTRL CUTTER          00000008%08d", pd->privdata.dnp.nocutwaste ? 1 : 0);

  /* Configure multi-cut/page size */
  stp_zprintf(v, "\033PIMAGE MULTICUT        00000008000000");

  if (!strcmp(pd->pagesize, "c8x10")) {
    stp_zprintf(v, "06");
  } else if (!strcmp(pd->pagesize, "w576h864")) {
    stp_zprintf(v, "07");
  } else if (!strcmp(pd->pagesize, "w288h576")) {
    stp_zprintf(v, "08");
  } else if (!strcmp(pd->pagesize, "w360h576")) {
    stp_zprintf(v, "09");
  } else if (!strcmp(pd->pagesize, "w432h576")) {
    stp_zprintf(v, "10");
  } else if (!strcmp(pd->pagesize, "w576h576")) {
    stp_zprintf(v, "11");
  } else if (!strcmp(pd->pagesize, "w576h576-div2")) {
    stp_zprintf(v, "13");
  } else if (!strcmp(pd->pagesize, "c8x10-div2")) {
    stp_zprintf(v, "14");
  } else if (!strcmp(pd->pagesize, "w576h864-div2")) {
    stp_zprintf(v, "15");
  } else if (!strcmp(pd->pagesize, "w576h648-w576h360_w576h288")) {
    stp_zprintf(v, "16");
  } else if (!strcmp(pd->pagesize, "c8x10-w576h432_w576h288")) {
    stp_zprintf(v, "17");
  } else if (!strcmp(pd->pagesize, "w576h792-w576h432_w576h360")) {
    stp_zprintf(v, "18");
  } else if (!strcmp(pd->pagesize, "w576h864-w576h576_w576h288")) {
    stp_zprintf(v, "19");
  } else if (!strcmp(pd->pagesize, "w576h864-div3")) {
    stp_zprintf(v, "20");
  } else if (!strcmp(pd->pagesize, "w576h842")) {
    stp_zprintf(v, "21");
  } else if (!strcmp(pd->pagesize, "w504h576")) {
    stp_zprintf(v, "32");
  } else if (!strcmp(pd->pagesize, "w576h648")) {
    stp_zprintf(v, "33");
  } else if (!strcmp(pd->pagesize, "A5")) {
    stp_zprintf(v, "34");
  } else if (!strcmp(pd->pagesize, "A4x4inch")) {
    stp_zprintf(v, "36");
  } else if (!strcmp(pd->pagesize, "A4x5inch")) {
    stp_zprintf(v, "37");
  } else if (!strcmp(pd->pagesize, "A4x6inch")) {
    stp_zprintf(v, "38");
  } else if (!strcmp(pd->pagesize, "A4x8inch")) {
    stp_zprintf(v, "39");
  } else if (!strcmp(pd->pagesize, "A4x10inch")) {
    stp_zprintf(v, "40");
  } else if (!strcmp(pd->pagesize, "A4x10inch-div2")) {
    stp_zprintf(v, "43");
  } else if (!strcmp(pd->pagesize, "A4")) {
    stp_zprintf(v, "41");
  } else if (!strcmp(pd->pagesize, "A4-div2")) {
    stp_zprintf(v, "35");
  } else {
    stp_zprintf(v, "00"); /* should not be possible */
  }

  if (!strcmp(pd->privdata.dnp.print_speed, "LowSpeed")) {
    stp_zprintf(v, "\033PCNTRL PRINTSPEED      0000000800000020");
  } else if (!strcmp(pd->privdata.dnp.print_speed, "HighDensity")) {
    stp_zprintf(v, "\033PCNTRL PRINTSPEED      0000000800000030");
  }
}

static const dyesub_stringitem_t dnpds820_print_speeds[] =
{
  { "Normal",      N_ ("Normal") },
  { "LowSpeed",    N_ ("Low Speed") },
  { "HighDensity", N_ ("High Density") }
};
LIST(dyesub_stringlist_t, dnpds820_printspeeds_list, dyesub_stringitem_t, dnpds820_print_speeds);

static const stp_parameter_t ds820_parameters[] =
{
  {
    "PrintSpeed", N_("Print Speed"), "Color=No,Category=Advanced Printer Setup",
    N_("Print Speed"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "NoCutWaste", N_("No Cut-Paper Waste"), "Color=No,Category=Advanced Printer Setup",
    N_("No Cut-Paper Waste"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 0, STP_CHANNEL_NONE, 1, 0
  },
};
#define ds820_parameter_count (sizeof(ds820_parameters) / sizeof(const stp_parameter_t))

static int
ds820_load_parameters(const stp_vars_t *v, const char *name,
			 stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "PrintSpeed") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &dnpds820_printspeeds_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "NoCutWaste") == 0)
    {
      description->is_active = 1;
      description->deflt.boolean = 0;
    }
  else
  {
     return 0;
  }
  return 1;
}

static int ds820_parse_parameters(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  const char *print_speed = stp_get_string_parameter(v, "PrintSpeed");
  int nocutwaste = stp_get_boolean_parameter(v, "NoCutWaste");

  if (pd) {
    pd->privdata.dnp.print_speed = print_speed;
    pd->privdata.dnp.nocutwaste = nocutwaste;
  }

  return 1;
}

/* Citizen CW-01 */
static const dyesub_resolution_t res_citizen_cw01_dpi[] =
{
  { "334x334", 334, 334},
  { "334x600", 334, 600},
};

LIST(dyesub_resolution_list_t, res_citizen_cw01_dpi_list, dyesub_resolution_t,res_citizen_cw01_dpi);

static const dyesub_pagesize_t citizen_cw01_page[] =
{
  DEFINE_PAPER( "w252h338", "3.5x4.7", PT1(1210,334), PT1(2048,334), 0, 0, PT(225,334), PT(225,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "B7", "3.5x5", PT1(1210,334), PT1(2048,334), 0, 0, PT(169,334), PT(169,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w288h432", "4x6", PT1(1380,334), PT1(2048,334), 0, 0, PT(5,334), PT(5,334), DYESUB_LANDSCAPE),
  DEFINE_PAPER( "w338h504",	"4.7x7", PT1(2048,334), PT1(2380,334), PT(225,334), PT(225,334), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w360h504",	"5x7", PT1(2048,334), PT1(2380,334), PT(169,334), PT(169,334), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h576", "6x8", PT1(2048,334), PT1(2710,300), PT(5,334), PT(5,334), 0, 0, DYESUB_PORTRAIT),
  DEFINE_PAPER( "w432h648", "6x9", PT1(2048,334), PT1(3050,334), PT(5,334), PT(5,334), 0, 0, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, citizen_cw01_page_list, dyesub_pagesize_t, citizen_cw01_page);

static const dyesub_printsize_t citizen_cw01_printsize[] =
{
  { "334x334", "w252h338", 1210, 2048},
  { "334x600", "w252h338", 2176, 2048},
  { "334x334", "B7", 1210, 2048},
  { "334x600", "B7", 2176, 2048},
  { "334x334", "w288h432", 1380, 2048},
  { "334x600", "w288h432", 2480, 2048},
  { "334x334", "w338h504", 2048, 2380},
  { "334x600", "w338h504", 2048, 4276},
  { "334x334", "w360h504", 2048, 2380},
  { "334x600", "w360h504", 2048, 4276},
  { "334x334", "w432h576", 2048, 2710},
  { "334x600", "w432h576", 2048, 4870},
  { "334x334", "w432h648", 2048, 3050},
  { "334x600", "w432h648", 2048, 5480},
};

LIST(dyesub_printsize_list_t, citizen_cw01_printsize_list, dyesub_printsize_t, citizen_cw01_printsize);

static void citizen_cw01_printer_start(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);
  /* Set quantity.. Backend overrides as needed. */
  stp_zprintf(v, "\033PCNTRL QTY             00000008%07d\r", pd->copies);
  /* Set cutter, nothing fancy */
  stp_zprintf(v, "\033PCNTRL CUTTER          0000000800000000");

  /* CW-01 has no other smarts.  No multicut, no matte. */
}

/* Magicard Series */
static const dyesub_pagesize_t magicard_page[] =
{
  DEFINE_PAPER( "w155h244", "ID-1/CR80", PT1(672,300), PT1(1016,300), PT1(15, 300), PT1(15,300), 0, 0, DYESUB_PORTRAIT),
};

LIST(dyesub_pagesize_list_t, magicard_page_list, dyesub_pagesize_t, magicard_page);

static const dyesub_printsize_t magicard_printsize[] =
{
  { "300x300", "w155h244", 672, 1016},
};

LIST(dyesub_printsize_list_t, magicard_printsize_list, dyesub_printsize_t, magicard_printsize);

static const overcoat_t magicard_overcoat[] =
{
  {"Off",  N_("Off"),  {3, "OFF"}},
  {"On",  N_("On"),  {2, "ON"}},
};

LIST(overcoat_list_t, magicard_overcoat_list, overcoat_t, magicard_overcoat);

static void magicard_printer_init(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  dyesub_nputc(v, 0x05, 64);  /* ATTN/Clear */
  stp_putc(0x01, v);       /* Start command sequence */
  stp_zprintf(v, ",NOC1");
  stp_zprintf(v, ",VER%d.%d.%d", STP_MAJOR_VERSION, STP_MINOR_VERSION, STP_MICRO_VERSION); // XXX include "pre" or other tag.
  stp_zprintf(v, ",LANENG"); // Dunno about other options.
  stp_zprintf(v, ",TDT%08X", (unsigned int)stpi_time(NULL)); /* Some sort of timestamp. Unknown epoch. */
//  stp_zprintf(v, ",LC%d", 1); // Force media type.  LC1/LC3/LC6/LC8 for YMCKO/MONO/KO/YMCKOK
  stp_zprintf(v, ",REJ%s", pd->privdata.magicard.reject ? "ON" : "OFF"); /* Faulty card rejection. */
  stp_zprintf(v, ",ESS%d", pd->copies); /* Number of copies */
  stp_zprintf(v, ",KEE,RT2");
  if (pd->duplex_mode &&
      strcmp(pd->duplex_mode, "None") &&
      strcmp(pd->duplex_mode, "Standard")) /* Duplex enabled? */
    {
      stp_zprintf(v, ",DPXON,PAG%d", 1 + (pd->page_number & 1));
      if (!(pd->page_number & 1))
        {
          /* Color format of BACk side -- eg CKO or KO or C or CO or K.  We don't support K/KO-only! */
          stp_zprintf(v, ",BAC%s%s",
		      pd->privdata.magicard.resin_k ? "CK" : "C",
		      pd->privdata.magicard.overcoat ? "O" : "");
	}
    }
  else
    {
      stp_zprintf(v, ",DPXOFF,PAG1");
    }
  stp_zprintf(v, ",SLW%s", pd->privdata.magicard.colorsure ? "ON" : "OFF"); /* "Colorsure printing" */
  stp_zprintf(v, ",IMF%s", "BGR"); /* Image format -- as opposed to K, BGRK and others. */
  stp_zprintf(v, ",XCO0,YCO0"); // ??
  stp_zprintf(v, ",WID%u,HGT%u", (unsigned int)pd->h_size, (unsigned int)pd->w_size - 30); /* Width & Height */

  /* Overcoat options are unique per-side */
  if (!(pd->page_number & 1))
  {
    stp_zprintf(v, ",OVR%s", pd->privdata.magicard.overcoat ? "ON" : "OFF" );
    if (pd->privdata.magicard.overcoat && pd->privdata.magicard.overcoat_hole)
    {
      if (!strcmp("SmartCard", pd->privdata.magicard.overcoat_hole))
        stp_zprintf(v, ",NCT%d,%d,%d,%d", 90, 295, 260, 450);
      else if (!strcmp("SmartCardLarge", pd->privdata.magicard.overcoat_hole))
        stp_zprintf(v, ",NCT%d,%d,%d,%d", 75, 275, 280, 470);
      else if (!strcmp("MagStripe", pd->privdata.magicard.overcoat_hole))
        stp_zprintf(v, ",NCT%d,%d,%d,%d", 0, 420, 1025, 590);
      else if (!strcmp("MagStripeLarge", pd->privdata.magicard.overcoat_hole))
        stp_zprintf(v, ",NCT%d,%d,%d,%d", 0, 400, 1025, 610);
      /* XXX TODO: Add ability to specify custom hole sizes */
    }
  } else {
    stp_zprintf(v, ",OVR%s", pd->privdata.magicard.overcoat_dpx ? "ON" : "OFF" );
    if (pd->privdata.magicard.overcoat_dpx && pd->privdata.magicard.overcoat_hole_dpx)
    {
      if (!strcmp("SmartCard", pd->privdata.magicard.overcoat_hole_dpx))
        stp_zprintf(v, ",NCT%d,%d,%d,%d", 90, 295, 260, 450);
      else if (!strcmp("SmartCardLarge", pd->privdata.magicard.overcoat_hole_dpx))
        stp_zprintf(v, ",NCT%d,%d,%d,%d", 75, 275, 280, 470);
      else if (!strcmp("MagStripe", pd->privdata.magicard.overcoat_hole_dpx))
        stp_zprintf(v, ",NCT%d,%d,%d,%d", 0, 420, 1025, 590);
      else if (!strcmp("MagStripeLarge", pd->privdata.magicard.overcoat_hole_dpx))
        stp_zprintf(v, ",NCT%d,%d,%d,%d", 0, 400, 1025, 610);
      /* XXX TODO: Add ability to specify custom hole sizes */
    }
  }
  stp_zprintf(v, ",NNNOFF"); // ??
  if (!(pd->page_number & 1))
  {
    stp_zprintf(v, ",USF%s", pd->privdata.magicard.holokote ? "ON" : "OFF");  /* Disable Holokote. */
    if (pd->privdata.magicard.holokote)
    {
      stp_zprintf(v, ",HKT%d", pd->privdata.magicard.holokote);
      stp_zprintf(v, ",CKI%s", pd->privdata.magicard.holokote_custom? "ON" : "OFF");
      stp_zprintf(v, ",HKMFFFFFF,TRO0"); // HKM == area. each bit is a separate area, 1-24.  Not sure about TRO
    }

    if (pd->privdata.magicard.holopatch)
    {
      stp_zprintf(v, ",HPHON,PAT%d", pd->privdata.magicard.holopatch);
    }
  } else {
    stp_zprintf(v, ",USFOFF");  /* Disable Holokote on duplex side. */
  }

  /* Magnetic stripe. Only program on the FRONT side. */
  if (!(pd->page_number & 1))
  {
    if (pd->privdata.magicard.mag1[0]) {
	  stp_zprintf(v, ",MAG1,BPI210,MPC7,COE%c,%s",
		      pd->privdata.magicard.mag_coer ? 'H': 'L',
		  pd->privdata.magicard.mag1);
    }
    if (pd->privdata.magicard.mag2[0]) {
	  stp_zprintf(v, ",MAG2,BPI75,MPC5,COE%c,%s",
		      pd->privdata.magicard.mag_coer ? 'H': 'L',
		  pd->privdata.magicard.mag2);
    }
    if (pd->privdata.magicard.mag3[0]) {
	  stp_zprintf(v, ",MAG3,BPI210,MPC7,COE%c,%s",
		      pd->privdata.magicard.mag_coer ? 'H': 'L',
		  pd->privdata.magicard.mag3);
    }
  }

  stp_zprintf(v, ",PCT%d,%d,%d,%d", 0, 0, 1025, 641); // print area? (seen 1025/1015/999,641)
  stp_zprintf(v, ",ICC%d", pd->privdata.magicard.gamma);  /* Gamma curve. 0-2 */
  if (pd->privdata.magicard.power_color != 50)
    stp_zprintf(v, ",CPW%d", pd->privdata.magicard.power_color); /* RGB/Color power. 0-100 */
  if (pd->privdata.magicard.power_overcoat != 50)
    stp_zprintf(v, ",OPW%d", pd->privdata.magicard.power_overcoat); /* Overcoat power. 0-100 */
  if (pd->privdata.magicard.power_resin != 50)
    stp_zprintf(v, ",KPW%d", pd->privdata.magicard.power_resin); /* Black/Resin power. 0-100 */
  if (pd->privdata.magicard.align_start != 50)
    stp_zprintf(v, ",SOI%d", pd->privdata.magicard.align_start); /* Card Start alignment, 0-100 */
  if (pd->privdata.magicard.align_end != 50)
    stp_zprintf(v, ",EOI%d", pd->privdata.magicard.align_end); /* Card End alignment, 0-100 */
  stp_zprintf(v, ",DDD50"); // ??
  stp_zprintf(v, ",X-GP-8");  /* GP extension, tells backend data is 8bpp */
  if (pd->privdata.magicard.resin_k)
    stp_zprintf(v, ",X-GP-RK"); /* GP extension, tells backend to extract resin-K layer */
  stp_zprintf(v, ",SZB%d", (int)(pd->w_size * pd->h_size));  /* 8bpp, needs to be 6bpp */
  stp_zprintf(v, ",SZG%d", (int)(pd->w_size * pd->h_size));
  stp_zprintf(v, ",SZR%d", (int)(pd->w_size * pd->h_size));
//  stp_zprintf(v, ",SZK%d", (int)(pd->w_size * pd->h_size)); /* 8bpp, needs to be 1bpp */
  stp_putc(0x1c, v);  /* Terminate command data */
}

static void magicard_printer_end(stp_vars_t *v)
{
  stp_putc(0x03, v);  /* Terminate the command sequence */
}

static void magicard_plane_end(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  stp_putc(0x1c, v); /* Terminate the image data */
  switch (pd->plane)
  {
  case 3:
    stp_putc(0x42, v); /* Blue */
    break;
  case 2:
    stp_putc(0x47, v); /* Green */
    break;
  case 1:
    stp_putc(0x52, v); /* Red */
    break;
  default:
//    stp_putc(0x4b, v); /* Black/Resin */
    break;
  }
  stp_putc(0x3a, v);
}

static const dyesub_stringitem_t magicard_black_types[] =
{
  { "Composite",   N_ ("Composite (CMY)") },
  { "Resin",       N_ ("Resin Black") },
};
LIST(dyesub_stringlist_t, magicard_black_types_list, dyesub_stringitem_t, magicard_black_types);

static const dyesub_stringitem_t magicard_mag_coer[] =
{
  { "Low", N_ ("Low") },
  { "High", N_ ("High") },
};
LIST(dyesub_stringlist_t, magicard_mag_coer_list, dyesub_stringitem_t, magicard_mag_coer);

static const stp_parameter_t magicard_parameters[] =
{
  {
    "BlackType", N_("Black Type"), "Color=No,Category=Advanced Printer Setup",
    N_("Black Type"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 0, STP_CHANNEL_NONE, 1, 0
  },
  {
    "RejectBad", N_("Reject Bad Cards"), "Color=No,Category=Advanced Printer Setup",
    N_("Reject Bad Cards"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 0, STP_CHANNEL_NONE, 1, 0
  },
  {
    "ColorSure", N_("Enable Colorsure"), "Color=No,Category=Advanced Printer Setup",
    N_("Enable Colorsure"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 0, STP_CHANNEL_NONE, 1, 0
  },
  {
    "GammaCurve", N_("Printer Gamma Curve"), "Color=No,Category=Advanced Printer Setup",
    N_("Internal Gamma Curve to apply (0 is none)"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "PowerColor", N_("Color Power Level"), "Color=No,Category=Advanced Printer Setup",
    N_("Power level for color passes"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "PowerBlack", N_("Black Power Level"), "Color=No,Category=Advanced Printer Setup",
    N_("Power level for black pass"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "PowerOC", N_("Overcoat Power Level"), "Color=No,Category=Advanced Printer Setup",
    N_("Power level for overcoat pass"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "AlignStart", N_("Card Start Alignment"), "Color=No,Category=Advanced Printer Setup",
    N_("Fine-tune card start position"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "AlignEnd", N_("Card End Alignment"), "Color=No,Category=Advanced Printer Setup",
    N_("Fine-tune card end position"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "CardOffset", N_("Horizontal Card offset"), "Color=No,Category=Advanced Printer Setup",
    N_("Fine-tune card horizontal centering"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Holokote", N_("Holokote"), "Color=No,Category=Advanced Printer Setup",
    N_("Holokote option"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "HolokoteCustom", N_("Custom Holokote Key"), "Color=No,Category=Advanced Printer Setup",
    N_("Use an optional custom Holokote key"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Holopatch", N_("HoloPatch"), "Color=No,Category=Advanced Printer Setup",
    N_("Position of the HoloPatch"),
    STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "OvercoatHole", N_("Overcoat Hole"), "Color=No,Category=Advanced Printer Setup",
    N_("Area to not cover with an overcoat layer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "OvercoatHoleDuplex", N_("Overcoat Hole Duplex"), "Color=No,Category=Advanced Printer Setup",
    N_("Area to not cover with an overcoat layer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {  /* Note this is called "LaminateDuplex" rather than "OvercoatDuplex"
        to align with the mis-named "Laminate" option.
     */
    "LaminateDuplex", N_("Overcoat Pattern Duplex"), "Color=No,Category=Advanced Printer Setup",
    N_("Overcoat Pattern Duplex"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 0, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MagCoer", N_("Magnetic Stripe Coercivity"), "Color=No,Category=Advanced Printer Setup",
    N_("Magnetic Stripe Coercivity Type"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MagStripe1", N_("Magnetic Stripe Row 1"), "Color=No,Category=Advanced Printer Setup",
    N_("ISO 7811 alphanumeric data to be encoded in the first magnetic stripe row (0-79 characters)"),
    STP_PARAMETER_TYPE_RAW, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 0, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MagStripe2", N_("Magnetic Stripe Row 2"), "Color=No,Category=Advanced Printer Setup",
    N_("ISO 7811 alphanumeric data to be encoded in the second magnetic stripe row (0-40 digits)"),
    STP_PARAMETER_TYPE_RAW, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 0, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MagStripe3", N_("Magnetic Stripe Row 3"), "Color=No,Category=Advanced Printer Setup",
    N_("ISO 7811 alphanumeric data to be encoded in the third magnetic stripe row (0-107 digits)"),
    STP_PARAMETER_TYPE_RAW, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_ADVANCED, 0, 1, STP_CHANNEL_NONE, 1, 0
  },
};
#define magicard_parameter_count (sizeof(magicard_parameters) / sizeof(const stp_parameter_t))

static const dyesub_stringitem_t magicard_holokotes[] =
{
  { "Off",      N_ ("Off") },
  { "UltraSecure",    N_ ("Ultra Secure") },
  { "Rings", N_ ("Interlocking Rings") },
  { "Flex", N_ ("Flex") },
};
LIST(dyesub_stringlist_t, magicard_holokotes_list, dyesub_stringitem_t, magicard_holokotes);

static const dyesub_stringitem_t magicard_overcoat_holes[] =
{
  { "None",      N_ ("None") },
  { "SmartCard",    N_ ("Smart Card Chip") },
  { "SmartCardLarge", N_ ("Smart Card Chip (Large)") },
  { "MagStripe", N_ ("Magnetic Stripe") },
  { "MagStripeLarge", N_ ("Magnetic Stripe (Large)") },
};
LIST(dyesub_stringlist_t, magicard_overcoat_holes_list, dyesub_stringitem_t, magicard_overcoat_holes);

static int
magicard_load_parameters(const stp_vars_t *v, const char *name,
			 stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  if (caps->parameter_count && caps->parameters)
    {
      for (i = 0; i < caps->parameter_count; i++)
        if (strcmp(name, caps->parameters[i].name) == 0)
          {
	    stp_fill_parameter_settings(description, &(caps->parameters[i]));
	    break;
          }
    }

  if (strcmp(name, "BlackType") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &magicard_black_types_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "RejectBad") == 0)
    {
      description->deflt.boolean = 0;
      description->is_active = 1;
    }
  else if (strcmp(name, "ColorSure") == 0)
    {
      description->deflt.boolean = 1;
      description->is_active = 1;
    }
  else if (strcmp(name, "GammaCurve") == 0)
    {
      description->deflt.integer = 1;
      description->bounds.integer.lower = 0;
      description->bounds.integer.upper = 2;
      description->is_active = 1;
    }
  else if (strcmp(name, "PowerColor") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -50;
      description->bounds.integer.upper = 50;
      description->is_active = 1;
    }
  else if (strcmp(name, "PowerBlack") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -50;
      description->bounds.integer.upper = 50;
      description->is_active = 1;
    }
  else if (strcmp(name, "PowerOC") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -50;
      description->bounds.integer.upper = 50;
      description->is_active = 1;
    }
  else if (strcmp(name, "AlignStart") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -50;
      description->bounds.integer.upper = 50;
      description->is_active = 1;
    }
  else if (strcmp(name, "AlignEnd") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -50;
      description->bounds.integer.upper = 50;
      description->is_active = 1;
    }
  else if (strcmp(name, "CardOffset") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = -15;
      description->bounds.integer.upper = 15;
      description->is_active = 1;
    }
  else if (strcmp(name, "Holokote") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &magicard_holokotes_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "HolokoteCustom") == 0)
    {
      description->deflt.boolean = 0;
      description->is_active = 1;
    }
  else if (strcmp(name, "Holopatch") == 0)
    {
      description->deflt.integer = 0;
      description->bounds.integer.lower = 0;
      description->bounds.integer.upper = 24;
      description->is_active = 1;
    }
  else if (strcmp(name, "OvercoatHole") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &magicard_overcoat_holes_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "OvercoatHoleDuplex") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &magicard_overcoat_holes_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      /* This feature only applies if the printer is duplexing! */
      if (dyesub_feature(caps, DYESUB_FEATURE_DUPLEX))
        description->is_active = 1;
    }
  else if (strcmp(name, "OvercoatDuplex") == 0)
    {
      description->bounds.str = stp_string_list_create();
      if (caps->overcoat)
        {
          const overcoat_list_t *llist = caps->overcoat;

          for (i = 0; i < llist->n_items; i++)
            {
              const overcoat_t *l = &(llist->item[i]);
	      stp_string_list_add_string(description->bounds.str,
					 l->name, gettext(l->text));
	    }
          description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
	  /* This feature only applies if the printer is duplexing! */
	  if (dyesub_feature(caps, DYESUB_FEATURE_DUPLEX))
		  description->is_active = 1;
        } else {
	  description->is_active = 0;
        }
    }
  else if (strcmp(name, "MagCoer") == 0)
    {
      description->bounds.str = stp_string_list_create();

      const dyesub_stringlist_t *mlist = &magicard_mag_coer_list;
      for (i = 0; i < mlist->n_items; i++)
        {
	  const dyesub_stringitem_t *m = &(mlist->item[i]);
	  stp_string_list_add_string(description->bounds.str,
				       m->name, m->text); /* Do *not* want this translated, otherwise use gettext(m->text) */
	}
      description->deflt.str = stp_string_list_param(description->bounds.str, 0)->name;
      description->is_active = 1;
    }
  else if (strcmp(name, "MagStripe1") == 0)
    {
      description->is_active = 1;
    }
  else if (strcmp(name, "MagStripe2") == 0)
    {
      description->is_active = 1;
    }
  else if (strcmp(name, "MagStripe3") == 0)
    {
      description->is_active = 1;
    }
  else
    {
       return 0;
    }
  return 1;
}

static int magicard_parse_parameters(stp_vars_t *v)
{
  dyesub_privdata_t *pd = get_privdata(v);

  const char *lpar = stp_get_string_parameter(v, "Laminate");
  const char *lpar_dpx = stp_get_string_parameter(v, "LaminateDuplex");
  const char *mag_coer = stp_get_string_parameter(v, "MagCoer");
  const char *holokote = stp_get_string_parameter(v, "Holokote");
  int holopatch = stp_get_int_parameter(v, "Holopatch");
  const char *overcoat_hole = stp_get_string_parameter(v, "OvercoatHole");
  const char *overcoat_hole_dpx = stp_get_string_parameter(v, "OvercoatHoleDuplex");
  int holokote_custom = stp_get_boolean_parameter(v, "HolokoteCustom");
  const char *blacktype = stp_get_string_parameter(v, "BlackType");
  const stp_raw_t *magstripe1 = NULL;
  const stp_raw_t *magstripe2 = NULL;
  const stp_raw_t *magstripe3 = NULL;

  if (overcoat_hole && !strcmp("None", overcoat_hole))
    overcoat_hole = NULL;

  /* If overcoat is off, we can't use holokote or holopatch */
  if (lpar && strcmp("On", lpar)) {
    if ((holokote && strcmp(holokote, "Off")) || holopatch || overcoat_hole || holokote_custom) {
      stp_eprintf(v, _("Holokote, Holopatch, and Overcoat hole features require Overcoat to be enabled!\n"));
      return 0;
    }
  }

  /* Sanity check magstripe */
  if (stp_check_raw_parameter(v, "MagStripe1", STP_PARAMETER_ACTIVE)) {
    magstripe1 = stp_get_raw_parameter(v, "MagStripe1");
    if (magstripe1->bytes >= 79) {
      stp_eprintf(v, _("StpMagStripe1 must be between 0 and 78 bytes!\n"));
      return 0;
    }
  }
  if (stp_check_raw_parameter(v, "MagStripe2", STP_PARAMETER_ACTIVE)) {
    magstripe2 = stp_get_raw_parameter(v, "MagStripe2");
    if (magstripe2->bytes >= 40) {
      stp_eprintf(v, _("StpMagStripe2 must be between 0 and 39 bytes!\n"));
      return 0;
    }
  }
  if (stp_check_raw_parameter(v, "MagStripe3", STP_PARAMETER_ACTIVE)) {
    magstripe1 = stp_get_raw_parameter(v, "MagStripe3");
    if (magstripe1->bytes >= 107) {
      stp_eprintf(v, _("StpMagStripe3 must be between 0 and 106 bytes!\n"));
      return 0;
    }
  }

  /* No need to set global params if there's no privdata yet */
  if (!pd)
    return 1;

  pd->privdata.magicard.overcoat = lpar && !strcmp("On", lpar);
  pd->privdata.magicard.overcoat_dpx = lpar_dpx && !strcmp("On", lpar_dpx);
  pd->privdata.magicard.resin_k = blacktype && !strcmp("Resin",blacktype);
  pd->privdata.magicard.reject = stp_get_boolean_parameter(v, "RejectBad");
  pd->privdata.magicard.colorsure = stp_get_boolean_parameter(v, "ColorSure");
  pd->privdata.magicard.gamma = stp_get_int_parameter(v, "GammaCurve");
  pd->privdata.magicard.power_color = stp_get_int_parameter(v, "PowerColor") + 50;
  pd->privdata.magicard.power_resin = stp_get_int_parameter(v, "PowerBlack") + 50;
  pd->privdata.magicard.power_overcoat = stp_get_int_parameter(v, "PowerOC") + 50;
  pd->privdata.magicard.align_start = stp_get_int_parameter(v, "AlignStart") + 50;
  pd->privdata.magicard.align_end = stp_get_int_parameter(v, "AlignEnd") + 50;
  pd->privdata.magicard.holopatch = holopatch;
  pd->privdata.magicard.overcoat_hole = overcoat_hole;
  pd->privdata.magicard.overcoat_hole_dpx = overcoat_hole_dpx;

  pd->horiz_offset = stp_get_int_parameter(v, "CardOffset");

  pd->privdata.magicard.holokote = 0;
  if (holokote) {
    if (!strcmp(holokote, "UltraSecure")) {
      pd->privdata.magicard.holokote = 1;
    } else if (!strcmp(holokote, "InterlockingRings")) {
      pd->privdata.magicard.holokote = 2;
    } else if (!strcmp(holokote, "Flex")) {
      pd->privdata.magicard.holokote = 3;
    }
  }
  pd->privdata.magicard.holokote_custom = holokote_custom;

  pd->privdata.magicard.mag_coer = mag_coer && !strcmp("High", mag_coer);

  if (magstripe1 && magstripe1->bytes) {
    int i;
    memcpy(pd->privdata.magicard.mag1, magstripe1->data, magstripe1->bytes);
    pd->privdata.magicard.mag1[magstripe1->bytes] = 0;
    for (i = 0 ; i < magstripe1->bytes ; i++) {
      if (pd->privdata.magicard.mag1[i] < 0x20 ||
	  pd->privdata.magicard.mag1[i] > 0x5f) {
	stp_eprintf(v, _("Illegal Alphanumeric in Magstripe, 0x20->0x5F ASCII only\n"));
	return 0;
      }
    }
    if (pd->privdata.magicard.mag1[0] != '%') {
      stp_eprintf(v, _("Magstripe alphanumeric data must start with '%%'\n"));
      return 0;
    }
    if (pd->privdata.magicard.mag1[magstripe1->bytes - 1] != '?') {
      stp_eprintf(v, _("Magstripe string must end with '?'\n"));
      return 0;
    }
  }
  if (magstripe2 && magstripe2->bytes) {
    int i;
    memcpy(pd->privdata.magicard.mag2, magstripe2->data, magstripe2->bytes);
    pd->privdata.magicard.mag2[magstripe2->bytes] = 0;
    for (i = 0 ; i < magstripe2->bytes ; i++) {
      if (pd->privdata.magicard.mag2[i] < 0x30 ||
	  pd->privdata.magicard.mag2[i] > 0x3f) {
	stp_eprintf(v, _("Illegal Numeric in Magstripe, 0x30->0x3F ASCII only\n"));
	return 0;
      }
    }
    if (pd->privdata.magicard.mag2[0] != ';') {
      stp_eprintf(v, _("Magstripe numeric data must start with ';'\n"));
      return 0;
    }
    if (pd->privdata.magicard.mag2[magstripe2->bytes - 1] != '?') {
      stp_eprintf(v, _("Magstripe data must end with '?'\n"));
      return 0;
    }
  }
  if (magstripe3 && magstripe3->bytes) {
    int i;
    memcpy(pd->privdata.magicard.mag3, magstripe3->data, magstripe3->bytes);
    pd->privdata.magicard.mag3[magstripe3->bytes] = 0;
    for (i = 0 ; i < magstripe3->bytes ; i++) {
      if (pd->privdata.magicard.mag3[i] < 0x30 ||
	  pd->privdata.magicard.mag3[i] > 0x3f) {
	stp_eprintf(v, _("Illegal Numeric in Magstripe, 0x30->0x3F ASCII only\n"));
	return 0;
      }
    }
    if (pd->privdata.magicard.mag3[0] != ';') {
      stp_eprintf(v, _("Magstripe numeric data must start with ';'\n"));
      return 0;
    }
    if (pd->privdata.magicard.mag3[magstripe3->bytes - 1] != '?') {
      stp_eprintf(v, _("Magstripe data must end with '?'\n"));
      return 0;
    }
  }
  return 1;
}

/* Model capabilities */

static const dyesub_cap_t dyesub_model_capabilities[] =
{
  { /* Olympus P-10, P-11 */
    2,
    &rgb_ink_list,
    &res_310dpi_list,
    &p10_page_list,
    &p10_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &p10_printer_init_func, &p10_printer_end_func,
    NULL, NULL,
    &p10_block_init_func, NULL,
    NULL,
    &p10_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Olympus P-200 */
    4,
    &ymc_ink_list,
    &res_320dpi_list,
    &p200_page_list,
    &p200_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_BLOCK_ALIGN
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &p200_printer_init_func, &p200_printer_end_func,
    &p200_plane_init_func, NULL,
    NULL, NULL,
    &p200_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Olympus P-300 */
    0,
    &ymc_ink_list,
    &p300_res_list,
    &p300_page_list,
    &p300_printsize_list,
    16,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_BLOCK_ALIGN
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &p300_printer_init_func, NULL,
    NULL, &p300_plane_end_func,
    &p300_block_init_func, NULL,
    &p300_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Olympus P-400 */
    1,
    &ymc_ink_list,
    &res_314dpi_list,
    &p400_page_list,
    &p400_printsize_list,
    180,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &p400_printer_init_func, NULL,
    &p400_plane_init_func, &p400_plane_end_func,
    &p400_block_init_func, NULL,
    &p400_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Olympus P-440 */
    3,
    &bgr_ink_list,
    &res_314dpi_list,
    &p440_page_list,
    &p440_printsize_list,
    128,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &p440_printer_init_func, &p440_printer_end_func,
    NULL, NULL,
    &p440_block_init_func, &p440_block_end_func,
    NULL,
    &p10_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Olympus P-S100 */
    20,
    &bgr_ink_list,
    &res_306dpi_list,
    &ps100_page_list,
    &ps100_printsize_list,
    1808,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &ps100_printer_init_func, &ps100_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon CP-10 */
    1002,
    &ymc_ink_list,
    &res_300dpi_list,
    &cp10_page_list,
    &cp10_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &cp10_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    &cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon CP-100, CP-200, CP-300 */
    1000,
    &ymc_ink_list,
    &res_300dpi_list,
    &cpx00_page_list,
    &cpx00_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    &cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon CP-220, CP-330, SELPHY CP400, SELPHY CP500, SELPHY CP510,
       SELPHY CP520, SELPHY CP530, SELPHY CP600, SELPHY CP710,
       SELPHY CP720, SELPHY CP730, SELPHY CP740, SELPHY CP750,
       SELPHY CP760, SELPHY CP770, SELPHY CP780 */
    1001,
    &ymc_ink_list,
    &res_300dpi_list,
    &cp220_page_list,
    &cp220_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon SELPHY ES1 */
    1003,
    &ymc_ink_list,
    &res_300dpi_list,
    &cpx00_page_list,
    &cpx00_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &es1_printer_init_func, NULL,
    &es1_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon SELPHY ES2, SELPHY ES20 */
    1005,
    &ymc_ink_list,
    &res_300dpi_list,
    &cpx00_page_list,
    &cpx00_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &es2_printer_init_func, NULL,
    &es2_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon SELPHY ES3, SELPHY ES30 */
    1006,
    &ymc_ink_list,
    &res_300dpi_list,
    &cpx00_page_list,
    &cpx00_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &es3_printer_init_func, &es3_printer_end_func,
    &es2_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon SELPHY ES40 */
    1007,
    &ymc_ink_list,
    &res_300dpi_list,
    &cpx00_page_list,
    &cpx00_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &es40_printer_init_func, &es3_printer_end_func,
    &es2_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon SELPHY CP790 */
    1008,
    &ymc_ink_list,
    &res_300dpi_list,
    &cp220_page_list,
    &cp220_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &cp790_printer_init_func, &es3_printer_end_func,
    &es2_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon SELPHY CP800 */
    1009,
    &ymc_ink_list,
    &res_300dpi_list,
    &cpx00_page_list,
    &cpx00_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon SELPHY CP900 */
    1010,
    &ymc_ink_list,
    &res_300dpi_list,
    &cpx00_page_list,
    &cpx00_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &cpx00_printer_init_func, &cp900_printer_end_func,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Canon CP820, CP910, CP1000, CP1200 */
    1011,
    &cmy_ink_list,
    &res_300dpi_list,
    &cp910_page_list,
    &cp910_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &cp910_printer_init_func, NULL,
    NULL, NULL,
    NULL, NULL,
    cpx00_adjust_curves,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Sony UP-DP10  */
    2000,
    &cmy_ink_list,
    &res_300dpi_list,
    &updp10_page_list,
    &updp10_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_NATIVECOPIES,
    &updp10_printer_init_func, &updp10_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    updp10_adjust_curves,
    &updp10_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Sony UP-DR150 */
    2001,
    &rgb_ink_list,
    &res_334dpi_list,
    &updr150_page_list,
    &updr150_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &updr150_printer_init_func, &updr150_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL,
    &updp10_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Sony DPP-EX5, DPP-EX7 */
    2002,
    &rgb_ink_list,
    &res_403dpi_list,
    &dppex5_page_list,
    &dppex5_printsize_list,
    100,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS,
    &dppex5_printer_init, &dppex5_printer_end,
    NULL, NULL,
    &dppex5_block_init, NULL,
    NULL,
    &dppex5_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Sony UP-DR100 */
    2003,
    &rgb_ink_list,
    &res_334dpi_list,
    &updr100_page_list,
    &updr100_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &updr100_printer_init_func, &updr100_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL,
    &updr100_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Sony UP-DR200 */
    2004,
    &rgb_ink_list,
    &res_334dpi_list,
    &updr200_page_list,
    &updr200_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &updr200_printer_init_func, &updr200_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL,
    &updr200_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Sony UP-CR10L / DNP SL10 */
    2005,
    &rgb_ink_list,
    &res_300dpi_list,
    &upcr10_page_list,
    &upcr10_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &upcr10_printer_init_func, &upcr10_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Sony UP-D895 Family */
    2006,
    &w_ink_list,
    &res_325dpi_list,
    &sony_d89x_page_list,
    &sony_d89x_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_MONOCHROME | DYESUB_FEATURE_NATIVECOPIES,
    &sony_upd895_printer_init_func, &sony_upd895_printer_end_func,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    sony_upd895_parameters,
    sony_upd895_parameter_count,
    sony_upd895_load_parameters,
    sony_upd895_parse_parameters,
  },
  { /* Sony UP-D897 Family */
    2007,
    &w_ink_list,
    &res_325dpi_list,
    &sony_d89x_page_list,
    &sony_d89x_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_MONOCHROME | DYESUB_FEATURE_NATIVECOPIES,
    &sony_upd897_printer_init_func, &sony_upd897_printer_end_func,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    sony_upd897_parameters,
    sony_upd897_parameter_count,
    sony_upd897_load_parameters,
    sony_upd897_parse_parameters,
  },
  { /* Sony UP-D898 Family */
    2008,
    &w_ink_list,
    &res_325dpi_list,
    &sony_d89x_page_list,
    &sony_d89x_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_MONOCHROME | DYESUB_FEATURE_NATIVECOPIES,
    &sony_upd898_printer_init_func, &sony_updneo_printer_end_func,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Sony UP-CR20L */
    2009,
    &rgb_ink_list,
    &res_334dpi_list,
    &upcr20_page_list,
    &upcr20_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &sony_upcr20_printer_init_func, &sony_updneo_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL,
    &upcr20_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Sony UP-DR80 series */
    2010,
    &rgb_ink_list,
    &res_301dpi_list,
    &updr80md_page_list,
    &updr80md_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &sony_updr80md_printer_init_func, &sony_updr80md_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Fujifilm Printpix CX-400  */
    3000,
    &rgb_ink_list,
    &res_310dpi_list,
    &cx400_page_list,
    &cx400_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS,
    &cx400_printer_init_func, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Fujifilm Printpix CX-550  */
    3001,
    &rgb_ink_list,
    &res_310dpi_list,
    &cx400_page_list,
    &cx400_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS,
    &cx400_printer_init_func, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Fujifilm FinePix NX-500  */
    3002,
    &rgb_ink_list,
    &res_306dpi_list,
    &nx500_page_list,
    &nx500_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &nx500_printer_init_func, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Kodak Easyshare Dock family */
    4000,
    &ymc_ink_list,
    &res_300dpi_list,
    &kodak_dock_page_list,
    &kodak_dock_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &kodak_dock_printer_init, NULL,
    &kodak_dock_plane_init, NULL,
    NULL, NULL,
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Kodak Photo Printer 6800 */
    4001,
    &rgb_ink_list,
    &res_300dpi_list,
    &kodak_6800_page_list,
    &kodak_6800_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &kodak_68xx_printer_init, NULL,
    NULL, NULL, /* No plane funcs */
    NULL, NULL, /* No block funcs */
    NULL,
    &kodak_6800_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Kodak Photo Printer 6850 */
    4002,
    &rgb_ink_list,
    &res_300dpi_list,
    &kodak_6850_page_list,
    &kodak_6850_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &kodak_68xx_printer_init, NULL,
    NULL, NULL, /* No plane funcs */
    NULL, NULL, /* No block funcs */
    NULL,
    &kodak_6800_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Kodak Photo Printer 605 */
    4003,
    &rgb_ink_list,
    &res_300dpi_list,
    &kodak_605_page_list,
    &kodak_605_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &kodak_605_printer_init, NULL,
    NULL, NULL, /* No plane funcs */
    NULL, NULL, /* No block funcs */
    NULL,
    &kodak_605_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Kodak Professional 1400 */
    4004,
    &bgr_ink_list,
    &res_301dpi_list,
    &kodak_1400_page_list,
    &kodak_1400_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH
      | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_ROW_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &kodak_1400_printer_init, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL,
    &kodak_6800_overcoat_list, &kodak_1400_media_list,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Kodak Photo Printer 805 */
    4005,
    &bgr_ink_list,
    &res_301dpi_list,
    &kodak_805_page_list,
    &kodak_805_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH
      | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_ROW_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &kodak_805_printer_init, NULL,
    NULL, NULL, /* No plane funcs */
    NULL, NULL, /* No block funcs */
    NULL,
    &kodak_6800_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Kodak Professional 9810 (and 8800) */
    4006,
    &ymc_ink_list,
    &res_300dpi_list,
    &kodak_9810_page_list,
    &kodak_9810_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &kodak_9810_printer_init, &kodak_9810_printer_end,
    &kodak_9810_plane_init, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &kodak_9810_overcoat_list, NULL,
    NULL, NULL,
    kodak_9810_parameters,
    kodak_9810_parameter_count,
    kodak_9810_load_parameters,
    kodak_9810_parse_parameters,
  },
  { /* Kodak 8810 */
    4007,
    &rgb_ink_list,
    &res_300dpi_list,
    &kodak_8810_page_list,
    &kodak_8810_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &kodak_8810_printer_init, NULL,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &kodak_8810_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Kodak 7000/7010 */
    4008,
    &rgb_ink_list,
    &res_300dpi_list,
    &kodak_7000_page_list,
    &kodak_7000_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &kodak_70xx_printer_init, NULL,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &kodak_7000_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Kodak 7015 */
    4009,
    &rgb_ink_list,
    &res_300dpi_list,
    &kodak_7015_page_list,
    &kodak_7015_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &kodak_70xx_printer_init, NULL,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &kodak_7000_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Kodak Professional 8500 */
    4100,
    &bgr_ink_list,
    &res_314dpi_list,
    &kodak_8500_page_list,
    &kodak_8500_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &kodak_8500_printer_init, &kodak_8500_printer_end,
    NULL, NULL, /* No plane funcs */
    NULL, NULL, /* No block funcs */
    NULL,
    &kodak_8500_overcoat_list, &kodak_8500_media_list,
    NULL, NULL,
    kodak_8500_parameters,
    kodak_8500_parameter_count,
    kodak_8500_load_parameters,
    kodak_8500_parse_parameters,
  },
  { /* Mitsubishi CP3020D/DU/DE */
    4101,
    &ymc_ink_list,
    &res_314dpi_list,
    &mitsu_cp3020d_page_list,
    &mitsu_cp3020d_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cp3020d_printer_init, &mitsu_cp3020d_printer_end,
    &mitsu_cp3020d_plane_init, &mitsu_cp3020d_plane_end,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Mitsubishi CP3020DA/DAE */
    4102,
    &bgr_ink_list,
    &res_314dpi_list,
    &mitsu_cp3020d_page_list,
    &mitsu_cp3020d_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cp3020da_printer_init, &mitsu_cp3020da_printer_end,
    &mitsu_cp3020da_plane_init, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Mitsubishi CP9550D */
    4103,
    &rgb_ink_list,
    &res_346dpi_list,
    &mitsu_cp9550_page_list,
    &mitsu_cp9550_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cp9550_printer_init, &mitsu_cp9550_printer_end,
    &mitsu_cp3020da_plane_init, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    mitsu9550_parameters,
    mitsu9550_parameter_count,
    mitsu9550_load_parameters,
    mitsu9550_parse_parameters,
  },
  { /* Mitsubishi CP9810D */
    4104,
    &bgr_ink_list,
    &res_300dpi_list,
    &mitsu_cp9810_page_list,
    &mitsu_cp9810_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cp9810_printer_init, &mitsu_cp9810_printer_end,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &mitsu_cp9810_overcoat_list, NULL,
    NULL, NULL,
    mitsu98xx_parameters,
    mitsu98xx_parameter_count,
    mitsu98xx_load_parameters,
    mitsu98xx_parse_parameters,
  },
  { /* Mitsubishi CPD70D */
    4105,
    &bgr_ink_list,
    &res_300dpi_list,
    &mitsu_cpd70x_page_list,
    &mitsu_cpd70x_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_WHITE_BORDER | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cpd70x_printer_init, NULL,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &mitsu_cpd70x_overcoat_list, NULL,
    mitsu_cpd70k60_job_start, NULL,
    mitsu70x_parameters,
    mitsu70x_parameter_count,
    mitsu70x_load_parameters,
    mitsu70x_parse_parameters,
  },
  { /* Mitsubishi CPK60D */
    4106,
    &bgr_ink_list,
    &res_300dpi_list,
    &mitsu_cpk60_page_list,
    &mitsu_cpk60_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cpk60_printer_init, NULL,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &mitsu_cpd70x_overcoat_list, NULL,
    mitsu_cpd70k60_job_start, NULL,
    mitsu70x_parameters,
    mitsu70x_parameter_count,
    mitsu_k60_load_parameters,
    mitsu70x_parse_parameters,
  },
  { /* Mitsubishi CPD80D */
    4107,
    &bgr_ink_list,
    &res_300dpi_list,
    &mitsu_cpd80_page_list,
    &mitsu_cpd80_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_WHITE_BORDER | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cpd70x_printer_init, NULL,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &mitsu_cpd70x_overcoat_list, NULL,
    mitsu_cpd70k60_job_start, NULL,
    mitsu70x_parameters,
    mitsu70x_parameter_count,
    mitsu70x_load_parameters,
    mitsu70x_parse_parameters,
  },
  { /* Kodak 305 */
    4108,
    &bgr_ink_list,
    &res_300dpi_list,
    &kodak305_page_list,
    &kodak305_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &kodak305_printer_init, NULL,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &mitsu_cpd70x_overcoat_list, NULL,
    mitsu_cpd70k60_job_start, NULL,
    mitsu70x_parameters,
    mitsu70x_parameter_count,
    mitsu_k60_load_parameters,
    mitsu70x_parse_parameters,
  },
  { /* Mitsubishi CPD90D */
    4109,
    &bgr_ink_list,
    &res_300dpi_list,
    &mitsu_cpd90_page_list,
    &mitsu_cpd90_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cpd90_printer_init, NULL,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &mitsu_cpd70x_overcoat_list, NULL,
    NULL, mitsu_cpd90_job_end,
    mitsu_d90_parameters,
    mitsu_d90_parameter_count,
    mitsu_d90_load_parameters,
    mitsu_d90_parse_parameters,
  },
  { /* Mitsubishi CP9600D */
    4110,
    &rgb_ink_list,
    &res_mitsu9600_dpi_list,
    &mitsu_cp9600_page_list,
    &mitsu_cp9600_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cp9600_printer_init, &mitsu_cp9600_printer_end,
    &mitsu_cp3020da_plane_init, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Mitsubishi CP9550DW-S */
    4111,
    &rgb_ink_list,
    &res_346dpi_list,
    &mitsu_cp9550s_page_list,
    &mitsu_cp9550s_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cp9550_printer_init, &mitsu_cp9550s_printer_end,
    &mitsu_cp3020da_plane_init, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    mitsu9550_parameters,
    mitsu9550_parameter_count,
    mitsu9550_load_parameters,
    mitsu9550_parse_parameters,
  },
  { /* Fujifilm ASK-300 */
    4112,
    &bgr_ink_list,
    &res_300dpi_list,
    &fuji_ask300_page_list,
    &fuji_ask300_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_WHITE_BORDER | DYESUB_FEATURE_NATIVECOPIES,
    &fuji_ask300_printer_init, NULL,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &mitsu_cpd70x_overcoat_list, NULL,
    mitsu_cpd70k60_job_start, NULL,
    mitsu70x_parameters,
    mitsu70x_parameter_count,
    mitsu_k60_load_parameters,
    mitsu70x_parse_parameters,
  },
  { /* Mitsubishi CP9800D */
    4113,
    &bgr_ink_list,
    &res_300dpi_list,
    &mitsu_cp9810_page_list,
    &mitsu_cp9810_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cp9800_printer_init, &mitsu_cp9810_printer_end,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    mitsu98xx_parameters,
    mitsu98xx_parameter_count,
    mitsu98xx_load_parameters,
    mitsu98xx_parse_parameters,
  },
  { /* Mitsubishi P95D/DW */
    4114,
    &w_ink_list,
    &res_325dpi_list,
    &mitsu_p95d_page_list,
    &mitsu_p95d_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_MONOCHROME | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_p95d_printer_init, &mitsu_p95d_printer_end,
    &mitsu_p95d_plane_start, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, &mitsu_p95d_media_list,
    NULL, NULL,
    mitsu_p95d_parameters,
    mitsu_p95d_parameter_count,
    mitsu_p95d_load_parameters,
    mitsu_p95d_parse_parameters,
  },
  { /* Mitsubishi CP9500D */
    4115,
    &rgb_ink_list,
    &res_m9500_list,
    &mitsu_cp9500_page_list,
    &mitsu_cp9500_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cp9500_printer_init, &mitsu_cp9500_printer_end,
    &mitsu_cp3020da_plane_init, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    mitsu9500_parameters,
    mitsu9500_parameter_count,
    mitsu9500_load_parameters,
    mitsu9500_parse_parameters,
  },
  { /* Mitsubishi P93D/DW */
    4116,
    &w_ink_list,
    &res_325dpi_list,
    &mitsu_p95d_page_list,
    &mitsu_p95d_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_MONOCHROME | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_p93d_printer_init, &mitsu_p95d_printer_end,
    &mitsu_p95d_plane_start, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, &mitsu_p93d_media_list,
    NULL, NULL,
    mitsu_p93d_parameters,
    mitsu_p93d_parameter_count,
    mitsu_p93d_load_parameters,
    mitsu_p93d_parse_parameters,
  },
  { /* Mitsubishi CPD707D */
    4117,
    &bgr_ink_list,
    &res_300dpi_list,
    &mitsu_cpd70x_page_list,
    &mitsu_cpd70x_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_WHITE_BORDER | DYESUB_FEATURE_NATIVECOPIES,
    &mitsu_cpd70x_printer_init, NULL,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    &mitsu_cpd70x_overcoat_list, NULL,
    mitsu_cpd70k60_job_start, NULL,
    mitsu707_parameters,
    mitsu707_parameter_count,
    mitsu707_load_parameters,
    mitsu70x_parse_parameters,
  },
  { /* Fujifilm ASK-2000/2500 */
    4200,
    &ymc_ink_list,
    &res_300dpi_list,
    &fuji_ask2000_page_list,
    &fuji_ask2000_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES | DYESUB_FEATURE_PLANE_INTERLACE,
    &fuji_ask2000_printer_init, &fuji_ask2000_printer_end,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Fujifilm ASK-4000 */
    4201,
    &ymc_ink_list,
    &res_300dpi_list,
    &fuji_ask4000_page_list,
    &fuji_ask4000_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES | DYESUB_FEATURE_PLANE_INTERLACE,
    &fuji_ask4000_printer_init, &fuji_ask4000_printer_end,
    NULL, NULL,
    NULL, NULL, /* No block funcs */
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Shinko CHC-S9045 (experimental) */
    5000,
    &rgb_ink_list,
    &res_300dpi_list,
    &shinko_chcs9045_page_list,
    &shinko_chcs9045_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &shinko_chcs9045_printer_init, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Shinko/Sinfonia CHC-S2145 */
    5001,
    &rgb_ink_list,
    &res_300dpi_list,
    &shinko_chcs2145_page_list,
    &shinko_chcs2145_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &shinko_chcs2145_printer_init, &shinko_chcs2145_printer_end,
    NULL, NULL,  /* No planes */
    NULL, NULL,  /* No blocks */
    NULL,
    &shinko_chcs2145_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Shinko/Sinfonia CHC-S1245 */
    5002,
    &rgb_ink_list,
    &res_300dpi_list,
    &shinko_chcs1245_page_list,
    &shinko_chcs1245_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &shinko_chcs1245_printer_init, &shinko_chcs2145_printer_end,
    NULL, NULL,  /* No planes */
    NULL, NULL,  /* No blocks */
    NULL,
    &shinko_chcs1245_overcoat_list, NULL,
    NULL, NULL,
    shinko_chcs1245_parameters,
    shinko_chcs1245_parameter_count,
    shinko_chcs1245_load_parameters,
    shinko_chcs1245_parse_parameters,
  },
  { /* Shinko/Sinfonia CHC-S6245 */
    5003,
    &rgb_ink_list,
    &res_300dpi_list,
    &shinko_chcs6245_page_list,
    &shinko_chcs6245_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &shinko_chcs6245_printer_init, &shinko_chcs2145_printer_end,
    NULL, NULL,  /* No planes */
    NULL, NULL,  /* No blocks */
    NULL,
    &shinko_chcs6245_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Shinko/Sinfonia CHC-S6145 */
    5004,
#ifdef S6145_YMC
    &ymc_ink_list,
#else
    &rgb_ink_list,
#endif
    &res_300dpi_list,
    &shinko_chcs6145_page_list,
    &shinko_chcs6145_printsize_list,
    SHRT_MAX,
#ifdef S6145_YMC
    DYESUB_FEATURE_PLANE_INTERLACE |
#endif
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &shinko_chcs6145_printer_init, &shinko_chcs2145_printer_end,
    NULL, NULL,  /* No planes */
    NULL, NULL,  /* No blocks */
    NULL,
    &shinko_chcs6145_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* CIAAT Brava 21 (aka CHC-S6145D) */
    5005,
#ifdef S6145_YMC
    &ymc_ink_list,
#else
    &rgb_ink_list,
#endif
    &res_300dpi_list,
    &ciaat_brava21_page_list,
    &ciaat_brava21_printsize_list,
    SHRT_MAX,
#ifdef S6145_YMC
    DYESUB_FEATURE_PLANE_INTERLACE |
#endif
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &shinko_chcs6145_printer_init, &shinko_chcs2145_printer_end,
    NULL, NULL,  /* No planes */
    NULL, NULL,  /* No blocks */
    NULL,
    &shinko_chcs6145_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Shinko/Sinfonia CHC-S2245 */
    5006,
#ifdef S6145_YMC
    &ymc_ink_list,
#else
    &rgb_ink_list,
#endif
    &res_300dpi_list,
    &shinko_chcs6145_page_list,
    &shinko_chcs6145_printsize_list,
    SHRT_MAX,
#ifdef S6145_YMC
    DYESUB_FEATURE_PLANE_INTERLACE |
#endif
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &shinko_chcs2245_printer_init, &shinko_chcs2145_printer_end,
    NULL, NULL,  /* No planes */
    NULL, NULL,  /* No blocks */
    NULL,
    &shinko_chcs6145_overcoat_list, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Dai Nippon Printing DS40 */
    6000,
    &bgr_ink_list,
    &res_dnpds40_dpi_list,
    &dnpds40_page_list,
    &dnpds40_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &dnpds40_printer_start, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL,
    &dnpds40_overcoat_list, NULL,
    NULL, NULL,
    ds40_parameters,
    ds40_parameter_count,
    ds40_load_parameters,
    ds40_parse_parameters,
  },
  { /* Dai Nippon Printing DS80 */
    6001,
    &bgr_ink_list,
    &res_dnpds40_dpi_list,
    &dnpds80_page_list,
    &dnpds80_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &dnpds80_printer_start, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL,
    &dnpds40_overcoat_list, NULL,
    NULL, NULL,
    ds40_parameters,
    ds40_parameter_count,
    ds40_load_parameters,
    dnpds80_parse_parameters,
  },
  { /* Dai Nippon Printing DSRX1 */
    6002,
    &bgr_ink_list,
    &res_dnpds40_dpi_list,
    &dnpsrx1_page_list,
    &dnpsrx1_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &dnpdsrx1_printer_start, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL,
    &dnpds40_overcoat_list, NULL,
    NULL, NULL,
    ds40_parameters,
    ds40_parameter_count,
    ds40_load_parameters,
    ds40_parse_parameters,
  },
  { /* Dai Nippon Printing DS620 */
    6003,
    &bgr_ink_list,
    &res_dnpds40_dpi_list,
    &dnpds620_page_list,
    &dnpds620_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &dnpds620_printer_start, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL,
    &dnpds620_overcoat_list, NULL,
    NULL, NULL,
    ds40_parameters,
    ds40_parameter_count,
    ds40_load_parameters,
    ds40_parse_parameters,
  },
  { /* Citizen CW-01 */
    6005,
    &bgr_ink_list,
    &res_citizen_cw01_dpi_list,
    &citizen_cw01_page_list,
    &citizen_cw01_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_NATIVECOPIES,
    &citizen_cw01_printer_start, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, 0, NULL, NULL,
  },
  { /* Dai Nippon Printing DS80DX */
    6006,
    &bgr_ink_list,
    &res_dnpds40_dpi_list,
    &dnpds80dx_page_list,
    &dnpds80dx_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_DUPLEX | DYESUB_FEATURE_NATIVECOPIES,
    &dnpds80_printer_start, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL,
    &dnpds40_overcoat_list, &dnpds80dx_media_list,
    NULL, NULL,
    ds40_parameters,
    ds40_parameter_count,
    ds40_load_parameters,
    dnpds80dx_parse_parameters,
  },
  { /* Dai Nippon Printing DS820 */
    6007,
    &bgr_ink_list,
    &res_dnpds40_dpi_list,
    &dnpds820_page_list,
    &dnpds820_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT | DYESUB_FEATURE_NATIVECOPIES ,
    &dnpds820_printer_start, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL,
    &dnpds620_overcoat_list, NULL,
    NULL, NULL,
    ds820_parameters,
    ds820_parameter_count,
    ds820_load_parameters,
    ds820_parse_parameters,
  },
  { /* Magicard Series w/ Duplex */
    7000,
    &ymc_ink_list,
    &res_300dpi_list,
    &magicard_page_list,
    &magicard_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_DUPLEX | DYESUB_FEATURE_NATIVECOPIES,
    &magicard_printer_init, &magicard_printer_end,
    NULL, magicard_plane_end,
    NULL, NULL,
    NULL,
    &magicard_overcoat_list, NULL,
    NULL, NULL,
    magicard_parameters,
    magicard_parameter_count,
    magicard_load_parameters,
    magicard_parse_parameters,
  },
  { /* Magicard Series w/o Duplex */
    7001,
    &ymc_ink_list,
    &res_300dpi_list,
    &magicard_page_list,
    &magicard_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_NATIVECOPIES,
    &magicard_printer_init, &magicard_printer_end,
    NULL, magicard_plane_end,
    NULL, NULL,
    NULL,
    &magicard_overcoat_list, NULL,
    NULL, NULL,
    magicard_parameters,
    magicard_parameter_count,
    magicard_load_parameters,
    magicard_parse_parameters,
  },
};

static const stp_parameter_t the_parameters[] =
{
  {
    "PageSize", N_("Page Size"), "Color=No,Category=Basic Printer Setup",
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "MediaType", N_("Media Type"), "Color=Yes,Category=Basic Printer Setup",
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "InputSlot", N_("Media Source"), "Color=No,Category=Basic Printer Setup",
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Resolution", N_("Resolution"), "Color=Yes,Category=Basic Printer Setup",
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "InkType", N_("Ink Type"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  { /* TRANSLATORS: Some dye sublimation printers are able to achieve
       better durability of output by covering it with transparent
       overcoat surface. This surface can be of different patterns:
       common are matte, glossy or texture.

       This is called "Laminate" instead of "Overcoat" for backwards
       compatibility reasons.
    */
    "Laminate", N_("Overcoat Pattern"), "Color=No,Category=Advanced Printer Setup",
    N_("Overcoat Pattern"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 0, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Borderless", N_("Borderless"), "Color=No,Category=Advanced Printer Setup",
    N_("Print without borders"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 0, STP_CHANNEL_NONE, 1, 0
  },
  {
    "PrintingMode", N_("Printing Mode"), "Color=Yes,Category=Core Parameter",
    N_("Printing Output Mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "Duplex", N_("Double-Sided Printing"), "Color=No,Category=Basic Printer Setup",
    N_("Duplex/Tumble Setting"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
  {
    "NativeCopies", N_("Printer Generates Copies Natively"), "Color=No,Category=Job Mode",
    N_("Printer Generates Copies"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_INTERNAL, 1, 0, STP_CHANNEL_NONE, 0, 1
  },
};

static int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
  int color_only;
} float_param_t;

static const float_param_t float_parameters[] =
{
  {
    {
      "CyanDensity", N_("Cyan Balance"), N_("Output Level Adjustment"),
      N_("Adjust the cyan balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 1, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "MagentaDensity", N_("Magenta Balance"), N_("Output Level Adjustment"),
      N_("Adjust the magenta balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 2, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "YellowDensity", N_("Yellow Balance"), N_("Output Level Adjustment"),
      N_("Adjust the yellow balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 3, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "BlackDensity", N_("Black Balance"), N_("Output Level Adjustment"),
      N_("Adjust the black balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 0, 1, 0
    }, 0.0, 2.0, 1.0, 1
  },
};

static const int float_parameter_count =
sizeof(float_parameters) / sizeof(const float_param_t);

/*
 * Duplex support - modes available
 * Note that the internal names MUST match those in cups/genppd.c else the
 * PPD files will not be generated correctly
 */

static const stp_param_string_t duplex_types[] =
{
  { "None",             N_ ("Off") },
  { "DuplexNoTumble",   N_ ("Long Edge (Standard)") },
  { "DuplexTumble",     N_ ("Short Edge (Flip)") }
};
#define NUM_DUPLEX (sizeof (duplex_types) / sizeof (stp_param_string_t))

static const dyesub_cap_t* dyesub_get_model_capabilities(const stp_vars_t *v, int model)
{
  int i;
  int models = sizeof(dyesub_model_capabilities) / sizeof(dyesub_cap_t);

  for (i=0; i<models; i++)
    {
      if (dyesub_model_capabilities[i].model == model)
        return &(dyesub_model_capabilities[i]);
    }
  stp_dprintf(STP_DBG_DYESUB, v,
  	"dyesub: model %d not found in capabilities list.\n", model);
  return &(dyesub_model_capabilities[0]);
}

static const overcoat_t* dyesub_get_overcoat_pattern(stp_vars_t *v)
{
  const char *lpar = stp_get_string_parameter(v, "Laminate");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));
  const overcoat_list_t *llist = caps->overcoat;
  const overcoat_t *l = NULL;
  int i;

  for (i = 0; i < llist->n_items; i++)
    {
      l = &(llist->item[i]);
      if (strcmp(l->name, lpar) == 0)
	 break;
    }
  return l;
}

static const dyesub_media_t* dyesub_get_mediatype(stp_vars_t *v)
{
  const char *mpar = stp_get_string_parameter(v, "MediaType");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
                                              stp_get_model_id(v));
  const dyesub_media_list_t *mlist = caps->media;
  const dyesub_media_t *m = NULL;
  int i;

  for (i = 0; i < mlist->n_items; i++)
    {
      m = &(mlist->item[i]);
      if (strcmp(m->name, mpar) == 0)
        break;
    }
  return m;
}

static void
dyesub_printsize(const stp_vars_t *v,
		   int  *width,
		   int  *height)
{
  int i;
  const char *page = stp_get_string_parameter(v, "PageSize");
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));
  const dyesub_printsize_list_t *p = caps->printsize;

  for (i = 0; i < p->n_items; i++)
    {
      if (strcmp(p->item[i].res_name,resolution) == 0 &&
          strcmp(p->item[i].pagesize_name,page) == 0)
        {
          *width  = p->item[i].width_px;
	  *height = p->item[i].height_px;
          return;
        }
    }
  stp_eprintf(v, "dyesub_printsize: printsize not found (%s, %s)\n",
	       page, resolution);
}

static int
dyesub_feature(const dyesub_cap_t *caps, int feature)
{
  return ((caps->features & feature) == feature);
}

static stp_parameter_list_t
dyesub_list_parameters(const stp_vars_t *v)
{
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v, stp_get_model_id(v));

  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;

  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  if (caps->parameter_count && caps->parameters)
    for (i = 0; i < caps->parameter_count ; i++)
      stp_parameter_list_add_param(ret, &(caps->parameters[i]));

  return ret;
}

static void
dyesub_parameters(const stp_vars_t *v, const char *name,
	       stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  description->deflt.str = NULL;
  for (i = 0; i < float_parameter_count; i++)
    if (strcmp(name, float_parameters[i].param.name) == 0)
      {
	stp_fill_parameter_settings(description,
				     &(float_parameters[i].param));
	description->deflt.dbl = float_parameters[i].defval;
	description->bounds.dbl.upper = float_parameters[i].max;
	description->bounds.dbl.lower = float_parameters[i].min;
      }

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }
  if (caps->load_parameters) /* do *NOT* use dyesub_exec() here */
    {
      if (caps->load_parameters(v, name, description))
        return; /* Ie parameter handled */
    }

  if (strcmp(name, "PageSize") == 0)
    {
      int default_specified = 0;
      const dyesub_pagesize_list_t *p = caps->pages;

      description->bounds.str = stp_string_list_create();

      /* Walk the list of pagesizes for the printer */
      for (i = 0; i < p->n_items; i++)
	{
	  stp_string_list_add_string(description->bounds.str,
				     p->item[i].psize.name,
				     gettext(p->item[i].psize.text));
	  if (! default_specified &&
	      p->item[i].psize.width > 0 && p->item[i].psize.height > 0)
	    {
	      description->deflt.str = p->item[i].psize.name;
	      default_specified = 1;
	    }
	}
      if (!default_specified)
	description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      description->bounds.str = stp_string_list_create();
      if (caps->media) {
	const dyesub_media_list_t *mlist = caps->media;

	for (i = 0; i < mlist->n_items; i++)
	  {
	    const dyesub_media_t *m = &(mlist->item[i]);
	    stp_string_list_add_string(description->bounds.str,
				       m->name, gettext(m->text));
	  }
	description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
	description->is_active = 1;
      } else {
	description->is_active = 0;
      }
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      char res_text[24];
      const dyesub_resolution_list_t *r = caps->resolution;

      description->bounds.str = stp_string_list_create();
      for (i = 0; i < r->n_items; i++)
        {
	  sprintf(res_text, "%s DPI", r->item[i].name);
	  stp_string_list_add_string(description->bounds.str,
		r->item[i].name, gettext(res_text));
	}
      if (r->n_items < 1)
        description->is_active = 0;
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "InkType") == 0)
    {
      description->bounds.str = stp_string_list_create();
      for (i = 0; i < caps->inks->n_items; i++)
	stp_string_list_add_string(description->bounds.str,
			  caps->inks->item[i].name, gettext(caps->inks->item[i].name));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
      if (caps->inks->n_items < 2)
        description->is_active = 0;
    }
  else if (strcmp(name, "Laminate") == 0)
    {
      description->bounds.str = stp_string_list_create();
      if (caps->overcoat)
        {
          const overcoat_list_t *llist = caps->overcoat;

          for (i = 0; i < llist->n_items; i++)
            {
              const overcoat_t *l = &(llist->item[i]);
	      stp_string_list_add_string(description->bounds.str,
			  	l->name, gettext(l->text));
	    }
          description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
          description->is_active = 1;
        } else {
	  description->is_active = 0;
        }
    }
  else if (strcmp(name, "Borderless") == 0)
    {
      if (dyesub_feature(caps, DYESUB_FEATURE_BORDERLESS))
        description->is_active = 1;
    }
  else if (strcmp(name, "PrintingMode") == 0)
    {
      description->bounds.str = stp_string_list_create();
      if (dyesub_feature(caps, DYESUB_FEATURE_MONOCHROME))
        stp_string_list_add_string(description->bounds.str,
				   "BW", _("Black and White"));
      else
        stp_string_list_add_string(description->bounds.str,
				   "Color", _("Color"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "Duplex") == 0)
    {
      int offer_duplex=0;

      description->bounds.str = stp_string_list_create();

      /*
       * Don't offer the Duplex/Tumble options if the JobMode parameter is
       * set to "Page" Mode.
       * "Page" mode is set by the Gimp Plugin, which only outputs one page at a
       * time, so Duplex/Tumble is meaningless.
       */

      if (stp_get_string_parameter(v, "JobMode"))
	offer_duplex = strcmp(stp_get_string_parameter(v, "JobMode"), "Page");
      else
	offer_duplex=1;

      if (offer_duplex && dyesub_feature(caps, DYESUB_FEATURE_DUPLEX))
      {
	description->deflt.str = duplex_types[0].name;
	for (i=0; i < NUM_DUPLEX; i++)
        {
          stp_string_list_add_string(description->bounds.str,
                                     duplex_types[i].name,gettext(duplex_types[i].text));
        }
      }
      else
        description->is_active = 0;
    }
  else if (strcmp(name, "NativeCopies") == 0)
    {
      description->deflt.boolean = dyesub_feature(caps, DYESUB_FEATURE_NATIVECOPIES);
      description->is_active = 1;
    }
  else
    description->is_active = 0;
}


static const dyesub_pagesize_t*
dyesub_get_pagesize(const stp_vars_t *v, const char *page)
{
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));
  const dyesub_pagesize_list_t *p = caps->pages;
  int i;
  if (page == NULL)
    return NULL;

  for (i = 0; i < p->n_items; i++)
    {
      if (strcmp(p->item[i].psize.name,page) == 0)
          return &(p->item[i]);
    }
  return NULL;
}

static const dyesub_pagesize_t*
dyesub_current_pagesize(const stp_vars_t *v)
{
  const char *page = stp_get_string_parameter(v, "PageSize");
  return dyesub_get_pagesize(v, page);
}

static const stp_papersize_t *
dyesub_describe_papersize(const stp_vars_t *v, const char *name)
{
  const dyesub_pagesize_t *pagesize = dyesub_get_pagesize(v, name);
  if (pagesize)
    return &(pagesize->psize);
  else
    return NULL;
}

static void
dyesub_media_size(const stp_vars_t *v,
		stp_dimension_t *width,
		stp_dimension_t *height)
{
  const dyesub_pagesize_t *pt = dyesub_current_pagesize(v);
  stp_default_media_size(v, width, height);

  if (pt && pt->psize.width > 0)
    *width = pt->psize.width;
  if (pt && pt->psize.height > 0)
    *height = pt->psize.height;
}

static void
dyesub_imageable_area_internal(const stp_vars_t *v,
				int  use_maximum_area,
				stp_dimension_t  *left,
				stp_dimension_t  *right,
				stp_dimension_t  *bottom,
				stp_dimension_t  *top,
				int  *print_mode)
{
  stp_dimension_t width, height;
  const dyesub_pagesize_t *pt = dyesub_current_pagesize(v);
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
		  				stp_get_model_id(v));

  dyesub_media_size(v, &width, &height);
  if (use_maximum_area
      || (dyesub_feature(caps, DYESUB_FEATURE_BORDERLESS) &&
          stp_get_boolean_parameter(v, "Borderless"))
      || !pt)
    {
      *left = 0;
      *top  = 0;
      *right  = width;
      *bottom = height;
    }
  else
    {
      *left = pt->psize.left;
      *top  = pt->psize.top;
      *right  = width  - pt->psize.right;
      *bottom = height - pt->psize.bottom;
    }
  if (pt)
    *print_mode = pt->print_mode;
  else
    *print_mode = DYESUB_PORTRAIT;
}

static void
dyesub_imageable_area(const stp_vars_t *v,
		       stp_dimension_t  *left,
		       stp_dimension_t  *right,
		       stp_dimension_t  *bottom,
		       stp_dimension_t  *top)
{
  int not_used;
  dyesub_imageable_area_internal(v, 0, left, right, bottom, top, &not_used);
}

static void
dyesub_maximum_imageable_area(const stp_vars_t *v,
			       stp_dimension_t  *left,
			       stp_dimension_t  *right,
			       stp_dimension_t  *bottom,
			       stp_dimension_t  *top)
{
  int not_used;
  const int model = stp_get_model_id(v);
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v, model);

  /* For printers that report FEATURE_WHITE_BORDER, we need to
     respect the margins they define as that's the printable area.
     The SELPHY models support FEATURE_BORDERLESS as well, so handle
     that special case. */

  dyesub_imageable_area_internal(v,
    (!(dyesub_feature(caps, DYESUB_FEATURE_WHITE_BORDER) &&
       !dyesub_feature(caps, DYESUB_FEATURE_BORDERLESS))),
    left, right, bottom, top, &not_used);
}

static void
dyesub_limit(const stp_vars_t *v,			/* I */
	    stp_dimension_t *width, stp_dimension_t *height,
	    stp_dimension_t *min_width, stp_dimension_t *min_height)
{
  *width  = SHRT_MAX;
  *height = SHRT_MAX;
  *min_width  = 1;
  *min_height =	1;
}

static void
dyesub_describe_resolution(const stp_vars_t *v,
			   stp_resolution_t *x, stp_resolution_t *y)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
  							stp_get_model_id(v));
  const dyesub_resolution_list_t *r = caps->resolution;
  int i;

  *x = -1;
  *y = -1;
  if (resolution)
    {
      for (i = 0; i < r->n_items; i++)
	{
	  if (strcmp(resolution, r->item[i].name) == 0)
	    {
	      *x = r->item[i].w_dpi;
	      *y = r->item[i].h_dpi;
	      break;
	    }
	}
    }
  return;
}

static const char *
dyesub_describe_output_internal(const stp_vars_t *v, dyesub_print_vars_t *pv)
{
  const char *ink_type      = stp_get_string_parameter(v, "InkType");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v,
  							stp_get_model_id(v));
  const char *output_type;
  int i;

  pv->ink_channels = 1;
  pv->ink_order = "\1";
  output_type = "CMY";

  if (ink_type)
    {
      for (i = 0; i < caps->inks->n_items; i++)
	if (strcmp(ink_type, caps->inks->item[i].name) == 0)
	  {
	    output_type = caps->inks->item[i].output_type;
	    pv->ink_channels = caps->inks->item[i].output_channels;
	    pv->ink_order = caps->inks->item[i].channel_order;
	    break;
	  }
    }

  return output_type;
}

static const char *
dyesub_describe_output(const stp_vars_t *v)
{
  dyesub_print_vars_t ipv;
  return dyesub_describe_output_internal(v, &ipv);
}

#define NPUTC_BUFSIZE (512)

static void
dyesub_nputc(stp_vars_t *v, char byte, int count)
{
  char buf[NPUTC_BUFSIZE];

  if (count == 1)
    stp_putc(byte, v);
  else
    {
      int i;
      int size = count;
      int blocks = size / NPUTC_BUFSIZE;
      int leftover = size % NPUTC_BUFSIZE;
      if (size > NPUTC_BUFSIZE)
	size = NPUTC_BUFSIZE;
      (void) memset(buf, byte, size);
      if (blocks)
	for (i = 0; i < blocks; i++)
	  stp_zfwrite(buf, size, 1, v);
      if (leftover)
	stp_zfwrite(buf, leftover, 1, v);
    }
}

static void
dyesub_swap_ints(int *a, int *b)
{
  int t = *a;
  *a = *b;
  *b = t;
}

static void
dyesub_adjust_curve(stp_vars_t *v,
		const char *color_adj,
		const char *color_curve)
{
  stp_curve_t   *adjustment = NULL;

  if (color_adj &&
        !stp_check_curve_parameter(v, color_curve, STP_PARAMETER_ACTIVE))
    {
      adjustment = stp_curve_create_from_string(color_adj);
      stp_set_curve_parameter(v, color_curve, adjustment);
      stp_set_curve_parameter_active(v, color_curve, STP_PARAMETER_ACTIVE);
      stp_curve_destroy(adjustment);
    }
}

static void
dyesub_exec(stp_vars_t *v,
		void (*func)(stp_vars_t *),
		const char *debug_string)
{
  if (func)
    {
      stp_dprintf(STP_DBG_DYESUB, v, "dyesub: %s\n", debug_string);
      (*func)(v);
    }
}

static int
dyesub_exec_check(stp_vars_t *v,
		  int (*func)(stp_vars_t *),
		  const char *debug_string)
{
  if (func)
    {
      stp_dprintf(STP_DBG_DYESUB, v, "dyesub: %s\n", debug_string);
      return (*func)(v);
    }
  return 1;
}

/* XXX FIXME:  This is "point" interpolation.  Be smarter!
   eg:  Average  (average all pixels that touch this one)
        BiLinear (scale based on linear interpolation)
        BiCubic  (scale based on weighted average, based on proximity)
        Lanczos  (awesome!! but slow)
*/
static int
dyesub_interpolate(int point, int olddim, int newdim)
{
#if 0
  /* Perform arithematic rounding.  Is there a point? */
  int result = ((point * 2 * newdim / olddim) + 1) / 2;
  if (result >= newdim)
    result--;
#else
  int result = (point * newdim / olddim);
#endif

  return result;
}

static void
dyesub_free_image(dyesub_print_vars_t *pv, stp_image_t *image)
{
  unsigned short** image_data = pv->image_data;
  int image_px_height = pv->image_rows;
  int i;

  for (i = 0; i< image_px_height; i++)
    if (image_data[i])
      stp_free(image_data[i]);
  if (image_data)
    stp_free(image_data);
}

static unsigned short **
dyesub_read_image(stp_vars_t *v,
		dyesub_print_vars_t *pv,
		stp_image_t *image)
{
  int image_px_width  = stp_image_width(image);
  int image_px_height = stp_image_height(image);
  int row_size = image_px_width * pv->ink_channels * sizeof(short);
  unsigned short **image_data;
  unsigned int zero_mask;
  int i;

  image_data = stp_zalloc(image_px_height * sizeof(unsigned short *));
  pv->image_rows = 0;
  if (!image_data)
    return NULL;	/* ? out of memory ? */

  for (i = 0; i < image_px_height; i++)
    {
      if (stp_color_get_row(v, image, i, &zero_mask))
        {
	  stp_dprintf(STP_DBG_DYESUB, v,
	  	"dyesub_read_image: "
		"stp_color_get_row(..., %d, ...) == 0\n", i);
	  dyesub_free_image(pv, image);
	  return NULL;
	}
      image_data[i] = stp_malloc(row_size);
      pv->image_rows = i+1;
      if (!image_data[i])
        {
	  stp_dprintf(STP_DBG_DYESUB, v,
	  	"dyesub_read_image: "
		"(image_data[%d] = stp_malloc()) == NULL\n", i);
	  dyesub_free_image(pv, image);
	  return NULL;
	}
      memcpy(image_data[i], stp_channel_get_output(v), row_size);
    }
  return image_data;
}

static void
dyesub_render_pixel_u8(unsigned short *src, char *dest,
		       dyesub_print_vars_t *pv,
		       int plane)
{
  /* Scale down to output bit depth */
#if 0
  *dest = src[plane] >> 8; // XXX does this make more sense than division?
#else
  *dest = src[plane] / 257;
#endif
}

static void
dyesub_render_pixel_packed_u8(unsigned short *src, char *dest,
			      dyesub_print_vars_t *pv)
{
  int i;

  /* copy out_channel (image) to equiv ink_channel (printer) */
  for (i = 0; i < pv->ink_channels; i++)
    {
      dyesub_render_pixel_u8(src, dest + i, pv, pv->ink_order[i]-1);
    }
}

static void
dyesub_render_row_packed_u8(stp_vars_t *v,
			    dyesub_print_vars_t *pv,
			    const dyesub_cap_t *caps,
			    int in_row,
			    char *dest,
			    int bytes_per_pixel)
{
  int w;
  unsigned short *src;

  for (w = 0; w < pv->outw_px; w++)
    {
      int row = in_row;
      int col = dyesub_interpolate(w, pv->outw_px, pv->imgw_px);
      if (pv->plane_lefttoright)
	col = pv->imgw_px - col - 1;
      if (pv->print_mode == DYESUB_LANDSCAPE)
        { /* "rotate" image */
          dyesub_swap_ints(&col, &row);
          row = (pv->imgw_px - 1) - row;
        }
      src = &(pv->image_data[row][col * pv->out_channels]);

      dyesub_render_pixel_packed_u8(src, dest + w*bytes_per_pixel, pv);
    }
}

static void
dyesub_render_row_interlaced_u8(stp_vars_t *v,
				dyesub_print_vars_t *pv,
				const dyesub_cap_t *caps,
				int in_row,
				char *dest,
				int plane)
{
  int w;
  unsigned short *src;

  for (w = 0; w < pv->outw_px; w++)
    {
      int row = in_row;
      int col = dyesub_interpolate(w, pv->outw_px, pv->imgw_px);
      if (pv->plane_lefttoright)
	col = pv->imgw_px - col - 1;
      if (pv->print_mode == DYESUB_LANDSCAPE)
        { /* "rotate" image */
          dyesub_swap_ints(&col, &row);
          row = (pv->imgw_px - 1) - row;
        }
      src = &(pv->image_data[row][col * pv->out_channels]);

      dyesub_render_pixel_u8(src, dest + w, pv, plane);
    }
}

static int
dyesub_print_plane(stp_vars_t *v,
		   dyesub_print_vars_t *pv,
		   dyesub_privdata_t *pd,
		   const dyesub_cap_t *caps,
		   int plane)
{
  int h;
  int bpp = ((pv->plane_interlacing || pv->row_interlacing) ? 1 : pv->ink_channels);
  size_t rowlen = pv->prnw_px * bpp;
  char *destrow = stp_malloc(rowlen); /* Allocate a buffer for the rendered rows */
  if (!destrow)
    return 0;  /* ? out of memory ? */

  /* Pre-Fill in the blank bits of the row. */
  if (dyesub_feature(caps, DYESUB_FEATURE_FULL_WIDTH))
    {
      /* empty part left of image area */
      if (pv->outl_px > 0)
        {
          memset(destrow, pv->empty_byte[plane], bpp * pv->outl_px);
        }
      /* empty part right of image area */
      if (pv->outr_px < pv->prnw_px)
        {
          memset(destrow + rowlen - bpp * (pv->prnw_px - pv->outr_px),
		 pv->empty_byte[plane],
		 bpp * (pv->prnw_px - pv->outr_px));
        }
    }

  for (h = 0; h <= pv->prnb_px - pv->prnt_px; h++)
    {
      int p = pv->row_interlacing ? 0 : plane;

      do {

      if (h % caps->block_size == 0)
        { /* block init */
	  pd->block_min_h = h + pv->prnt_px;
	  pd->block_min_w = pv->prnl_px;
	  pd->block_max_h = MIN(h + pv->prnt_px + caps->block_size - 1,
	  					pv->prnb_px);
	  pd->block_max_w = pv->prnr_px;

	  dyesub_exec(v, caps->block_init_func, "caps->block_init");
	}

      /* Generate a single row */
      if (h + pv->prnt_px < pv->outt_px || h + pv->prnt_px >= pv->outb_px)
        { /* empty part above or below image area */
	  memset(destrow, pv->empty_byte[plane], rowlen);
	  /* FIXME: This is inefficient; it won't change once generated.. */
	}
      else
        {
	  int srcrow = dyesub_interpolate(h + pv->prnt_px - pv->outt_px,
					  pv->outh_px, pv->imgh_px);

	  stp_dprintf(STP_DBG_DYESUB, v,
		       "dyesub_print_plane: h = %d, row = %d\n", h, srcrow);

	  if (pv->plane_interlacing || pv->row_interlacing)
	    {
	      dyesub_render_row_interlaced_u8(v, pv, caps, srcrow,
						destrow + bpp * pv->outl_px, p);
	    }
	  else
            dyesub_render_row_packed_u8(v, pv, caps, srcrow,
					destrow + bpp * pv->outl_px, bpp);
	}
      /* And send it out */
      stp_zfwrite(destrow, rowlen, 1, v);

      if (h + pv->prnt_px == pd->block_max_h)
        { /* block end */
	  dyesub_exec(v, caps->block_end_func, "caps->block_end");
	}

      } while (pv->row_interlacing && ++p < pv->ink_channels);
    }

  stp_free(destrow);
  return 1;
}

/*
 * dyesub_print()
 */
static int
dyesub_do_print(stp_vars_t *v, stp_image_t *image, int print_op)
{
  int i;
  dyesub_print_vars_t pv;
  int status = 1;

  const int model           = stp_get_model_id(v);
  const char *ink_type;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(v, model);
  int max_print_px_width = 0;
  int max_print_px_height = 0;
  int w_dpi, h_dpi;
  stp_resolution_t wr_dpi, hr_dpi;	/* Resolution */

  /* output in 1/72" */
  stp_dimension_t out_pt_width  = stp_get_width(v);
  stp_dimension_t out_pt_height = stp_get_height(v);
  stp_dimension_t out_pt_left   = stp_get_left(v);
  stp_dimension_t out_pt_top    = stp_get_top(v);

  /* page in 1/72" */
  stp_dimension_t page_pt_width  = stp_get_page_width(v);
  stp_dimension_t page_pt_height = stp_get_page_height(v);
  stp_dimension_t page_pt_left = 0;
  stp_dimension_t page_pt_right = 0;
  stp_dimension_t page_pt_top = 0;
  stp_dimension_t page_pt_bottom = 0;
  int page_mode;

  int pl;

  dyesub_privdata_t *pd;

  if (!stp_verify(v))
    {
      stp_eprintf(v, _("Print options not verified; cannot print.\n"));
      return 0;
    }

  /* Clean up private state */
  (void) memset(&pv, 0, sizeof(pv));

  /* Allocate privdata structure */
  pd = stp_zalloc(sizeof(dyesub_privdata_t));
  stp_allocate_component_data(v, "Driver", NULL, NULL, pd);

  /* Parse any per-printer parameters *before* the generic ones */
  dyesub_exec_check(v, caps->parse_parameters, "caps->parse_parameters");

  stp_image_init(image);
  pv.imgw_px = stp_image_width(image);
  pv.imgh_px = stp_image_height(image);

  stp_describe_resolution(v, &wr_dpi, &hr_dpi);
  w_dpi = (int) wr_dpi;
  h_dpi = (int) hr_dpi;
  dyesub_printsize(v, &max_print_px_width, &max_print_px_height);

  /* Duplex processing -- Rotate even pages for DuplexNoTumble */
  pd->duplex_mode = stp_get_string_parameter(v, "Duplex");
  pd->page_number = stp_get_int_parameter(v, "PageNumber");
  if((pd->page_number & 1) && pd->duplex_mode && !strcmp(pd->duplex_mode,"DuplexNoTumble"))
    image = stpi_buffer_image(image,BUFFER_FLAG_FLIP_X | BUFFER_FLAG_FLIP_Y);

  /* Check to see if we're to generate more than one copy */
  if (stp_check_boolean_parameter(v, "NativeCopies", STP_PARAMETER_ACTIVE) &&
      stp_get_boolean_parameter(v, "NativeCopies") &&
      stp_check_int_parameter(v, "NumCopies", STP_PARAMETER_ACTIVE))
    pd->copies = stp_get_int_parameter(v, "NumCopies");
  else
    pd->copies = 1;
  /* FIXME: What about Collation? Any special handling here? */

  pd->pagesize = stp_get_string_parameter(v, "PageSize");
  if (caps->overcoat)
	  pd->overcoat = dyesub_get_overcoat_pattern(v);
  if (caps->media)
	  pd->media = dyesub_get_mediatype(v);

  dyesub_imageable_area_internal(v,
  	(dyesub_feature(caps, DYESUB_FEATURE_WHITE_BORDER) ? 1 : 0),
	&page_pt_left, &page_pt_right, &page_pt_bottom, &page_pt_top,
	&page_mode);

  /* Swap DPI so these computations will work out properly */
  if (page_mode == DYESUB_LANDSCAPE)
    dyesub_swap_ints(&w_dpi, &h_dpi);

  pv.prnw_px = MIN(max_print_px_width,
		  	PX(page_pt_right - page_pt_left, w_dpi));
  pv.prnh_px = MIN(max_print_px_height,
			PX(page_pt_bottom - page_pt_top, h_dpi));
  pv.outw_px = PX(out_pt_width, w_dpi);
  pv.outh_px = PX(out_pt_height, h_dpi);

  /* if image size is close enough to output size send out original size */
  if (abs(pv.outw_px - pv.imgw_px) < SIZE_THRESHOLD)
      pv.outw_px  = pv.imgw_px;
  if (abs(pv.outh_px - pv.imgh_px) < SIZE_THRESHOLD)
      pv.outh_px = pv.imgh_px;

  pv.outw_px = MIN(pv.outw_px, pv.prnw_px);
  pv.outh_px = MIN(pv.outh_px, pv.prnh_px);
  pv.outl_px = MIN(PX(out_pt_left - page_pt_left, w_dpi),
			pv.prnw_px - pv.outw_px);
  pv.outt_px = MIN(PX(out_pt_top  - page_pt_top, h_dpi),
			pv.prnh_px - pv.outh_px);
  pv.outr_px = pv.outl_px + pv.outw_px;
  pv.outb_px = pv.outt_px + pv.outh_px;

  /* Swap back so that everything that follows will work. */
  if (page_mode == DYESUB_LANDSCAPE)
    dyesub_swap_ints(&w_dpi, &h_dpi);

  stp_dprintf(STP_DBG_DYESUB, v,
	      "paper (pt)   %f x %f\n"
	      "image (px)   %d x %d\n"
	      "image (pt)   %f x %f\n"
	      "* out (pt)   %f x %f\n"
	      "* out (px)   %d x %d\n"
	      "* left x top (pt) %f x %f\n"
	      "* left x top (px) %d x %d\n"
	      "border (pt) (%f - %f) = %f x (%f - %f) = %f\n"
	      "printable pixels (px)   %d x %d\n"
	      "res (dpi)               %d x %d\n",
	      page_pt_width, page_pt_height,
	      pv.imgw_px, pv.imgh_px,
	      PT(pv.imgw_px, w_dpi), PT(pv.imgh_px, h_dpi),
	      out_pt_width, out_pt_height,
	      pv.outw_px, pv.outh_px,
	      out_pt_left, out_pt_top,
	      pv.outl_px, pv.outt_px,
	      page_pt_right, page_pt_left, page_pt_right - page_pt_left,
	      page_pt_bottom, page_pt_top, page_pt_bottom - page_pt_top,
	      pv.prnw_px, pv.prnh_px,
	      w_dpi, h_dpi
	      );

  ink_type = dyesub_describe_output_internal(v, &pv);
  stp_set_string_parameter(v, "STPIOutputType", ink_type);
  stp_channel_reset(v);
  for (i = 0; i < pv.ink_channels; i++)
    stp_channel_add(v, i, 0, 1.0);

  pv.out_channels = stp_color_init(v, image, 256);
  stp_set_float_parameter(v, "AppGammaScale", 1.0);

  /* If there's a mismatch in channels, that is ALWAYS a problem */
  if (pv.out_channels != pv.ink_channels)
    {
       stp_dprintf(STP_DBG_DYESUB, v,
		    "Input and output channel count mismatch! (%d vs %d)\n", pv.out_channels, pv.ink_channels);
      stp_image_conclude(image);
      stp_free(pd);
      return 2;
    }

  pv.image_data = dyesub_read_image(v, &pv, image);
  if (ink_type)
    {
      if (strcmp(ink_type, "RGB") == 0 ||
	  strcmp(ink_type, "BGR") == 0 ||
	  strcmp(ink_type, "Whitescale") == 0)
        {
	  pv.empty_byte[0] = 0xff;
	  pv.empty_byte[1] = 0xff;
	  pv.empty_byte[2] = 0xff;
	}
      else
        {
	  pv.empty_byte[0] = 0x0;
	  pv.empty_byte[1] = 0x0;
	  pv.empty_byte[2] = 0x0;
	}
    }
  else
    {
      pv.empty_byte[0] = 0x0;
      pv.empty_byte[1] = 0x0;
      pv.empty_byte[2] = 0x0;
    }

  pv.plane_interlacing = dyesub_feature(caps, DYESUB_FEATURE_PLANE_INTERLACE);
  pv.row_interlacing = dyesub_feature(caps, DYESUB_FEATURE_ROW_INTERLACE);
  pv.plane_lefttoright = dyesub_feature(caps, DYESUB_FEATURE_PLANE_LEFTTORIGHT);
  pv.print_mode = page_mode;
  if (!pv.image_data)
    {
      stp_image_conclude(image);
      stp_free(pd);
      return 2;
    }

  if (dyesub_feature(caps, DYESUB_FEATURE_FULL_HEIGHT))
    {
      pv.prnt_px = 0;
      pv.prnb_px = pv.prnh_px - 1;
    }
  else if (dyesub_feature(caps, DYESUB_FEATURE_BLOCK_ALIGN))
    {
      pv.prnt_px = pv.outt_px - (pv.outt_px % caps->block_size);
      				/* floor to multiple of block_size */
      pv.prnb_px = (pv.outb_px - 1) + (caps->block_size - 1)
      		- ((pv.outb_px - 1) % caps->block_size);
				/* ceil to multiple of block_size */
    }
  else
    {
      pv.prnt_px = pv.outt_px;
      pv.prnb_px = pv.outb_px - 1;
    }

  if (dyesub_feature(caps, DYESUB_FEATURE_FULL_WIDTH))
    {
      pv.prnl_px = 0;
      pv.prnr_px = pv.prnw_px - 1;
    }
  else
    {
      pv.prnl_px = pv.outl_px;
      pv.prnr_px = pv.outr_px;
    }

  if (pv.print_mode == DYESUB_LANDSCAPE)
    {
      dyesub_swap_ints(&pv.outh_px, &pv.outw_px);
      dyesub_swap_ints(&pv.outt_px, &pv.outl_px);
      dyesub_swap_ints(&pv.outb_px, &pv.outr_px);

      dyesub_swap_ints(&pv.prnh_px, &pv.prnw_px);
      dyesub_swap_ints(&pv.prnt_px, &pv.prnl_px);
      dyesub_swap_ints(&pv.prnb_px, &pv.prnr_px);

      dyesub_swap_ints(&pv.imgh_px, &pv.imgw_px);
    }

  /* Adjust margins if the driver asks, to fine-tune horizontal position. */
  pv.outl_px += pd->horiz_offset;
  pv.outr_px += pd->horiz_offset;
  /* Make sure we're still legal */
  if (pv.outl_px < 0)
    pv.outl_px = 0;
  if (pv.outr_px > pv.prnw_px)
    pv.outr_px = pv.prnw_px;

  /* By this point, we're finally DONE mangling the pv structure,
     and can start calling into the bulk of the driver code. */

  /* assign private data *after* swaping image dimensions */
  pd->w_dpi = w_dpi;
  pd->h_dpi = h_dpi;
  pd->w_size = pv.prnw_px;
  pd->h_size = pv.prnh_px;
  pd->print_mode = pv.print_mode;

  /* FIXME:  Provide a way of disabling/altering these curves */
  /* XXX reuse 'UseLUT' from mitsu70x?  or 'SimpleGamma' ? */
  dyesub_exec(v, caps->adjust_curves, "caps->adjust_curves");

  /* Send out job init if we're in page mode */
  if (print_op & OP_JOB_START)
    dyesub_exec(v, caps->job_start_func, "caps->job_start");

  /* printer init */
  dyesub_exec(v, caps->printer_init_func, "caps->printer_init");

  for (pl = 0; pl < (pv.plane_interlacing ? pv.ink_channels : 1); pl++)
    {
      pd->plane = pv.ink_order[pl];
      stp_dprintf(STP_DBG_DYESUB, v, "dyesub: plane %d\n", pd->plane);

      /* plane init */
      dyesub_exec(v, caps->plane_init_func, "caps->plane_init");

      dyesub_print_plane(v, &pv, pd, caps, (int) pv.ink_order[pl] - 1);

      /* plane end */
      dyesub_exec(v, caps->plane_end_func, "caps->plane_end");
    }

  /* printer end */
  dyesub_exec(v, caps->printer_end_func, "caps->printer_end");

  /* Job end, if we're in page mode */
  if (print_op & OP_JOB_END)
    dyesub_exec(v, caps->job_end_func, "caps->job_end");

  if (pv.image_data) {
    dyesub_free_image(&pv, image);
  }

  stp_image_conclude(image);
  stp_free(pd);

  return status;
}

static int
dyesub_print(const stp_vars_t *v, stp_image_t *image)
{
  int status;
  int op = OP_JOB_PRINT;

  stp_vars_t *nv = stp_vars_create_copy(v);

  if (!stp_get_string_parameter(v, "JobMode") ||
      strcmp(stp_get_string_parameter(v, "JobMode"), "Page") == 0)
    op = OP_JOB_START | OP_JOB_PRINT | OP_JOB_END;

  status = dyesub_do_print(nv, image, op);
  stp_vars_destroy(nv);
  return status;
}

static int
dyesub_job_start(const stp_vars_t *v, stp_image_t *image)
{
  const dyesub_cap_t *caps;
  stp_vars_t *nv = stp_vars_create_copy(v);

  caps = dyesub_get_model_capabilities(v, stp_get_model_id(nv));

  if (caps->job_start_func)
     caps->job_start_func(nv);
  stp_vars_destroy(nv);

  return 1;
}

static int
dyesub_job_end(const stp_vars_t *v, stp_image_t *image)
{
  const dyesub_cap_t *caps;
  stp_vars_t *nv = stp_vars_create_copy(v);

  caps = dyesub_get_model_capabilities(v, stp_get_model_id(nv));

  if (caps->job_end_func)
    caps->job_end_func(nv);
  stp_vars_destroy(nv);

  return 1;
}

static int dyesub_verify_printer_params(stp_vars_t *v)
{
  const int model           = stp_get_model_id(v);
  const dyesub_cap_t *caps  = dyesub_get_model_capabilities(v, model);
  int result;
  result = stp_verify_printer_params(v);
  if (result != 1)
    return result;

  /* Sanity-check printer-specific parameters if a function exists */
  result = dyesub_exec_check(v, caps->parse_parameters, "caps->parse_parameters");
  return result;
}

static const stp_printfuncs_t print_dyesub_printfuncs =
{
  dyesub_list_parameters,
  dyesub_parameters,
  dyesub_media_size,
  dyesub_imageable_area,
  dyesub_maximum_imageable_area,
  dyesub_limit,
  dyesub_print,
  dyesub_describe_resolution,
  dyesub_describe_output,
  dyesub_verify_printer_params,
  dyesub_job_start,
  dyesub_job_end,
  NULL,
  dyesub_describe_papersize
};

static stp_family_t print_dyesub_module_data =
  {
    &print_dyesub_printfuncs,
    NULL
  };

static int
print_dyesub_module_init(void)
{
  return stpi_family_register(print_dyesub_module_data.printer_list);
}


static int
print_dyesub_module_exit(void)
{
  return stpi_family_unregister(print_dyesub_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_dyesub_LTX_stp_module_version
#define stp_module_data print_dyesub_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "dyesub",
    VERSION,
    "DyeSub family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    print_dyesub_module_init,
    print_dyesub_module_exit,
    (void *) &print_dyesub_module_data
  };
