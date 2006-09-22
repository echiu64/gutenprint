/*
 * "$Id$"
 *
 *   Print plug-in Olympus driver for the GIMP.
 *
 *   Copyright 2003 - 2005
 *   Michael Mraka (Michael.Mraka@linux.cz)
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
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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

#ifdef __GNUC__
#define inline __inline__
#endif

#define OLYMPUS_FEATURE_NONE		0x00000000
#define OLYMPUS_FEATURE_FULL_WIDTH	0x00000001
#define OLYMPUS_FEATURE_FULL_HEIGHT	0x00000002
#define OLYMPUS_FEATURE_BLOCK_ALIGN	0x00000004
#define OLYMPUS_FEATURE_BORDERLESS	0x00000008
#define OLYMPUS_FEATURE_WHITE_BORDER	0x00000010
#define OLYMPUS_FEATURE_PLANE_INTERLACE	0x00000020

#define OLYMPUS_PORTRAIT	0
#define OLYMPUS_LANDSCAPE	1

#ifndef MIN
#  define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif /* !MIN */
#ifndef MAX
#  define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif /* !MAX */
#define PX(pt,dpi)	((pt) * (dpi) / 72)
#define PT(px,dpi)	((px) * 72 / (dpi))

#define MAX_INK_CHANNELS	3
#define MAX_BYTES_PER_CHANNEL	2


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
  int xdpi;
  int ydpi;
} olymp_resolution_t;

typedef struct {
  const olymp_resolution_t *item;
  size_t n_items;
} olymp_resolution_list_t;

typedef struct {
  const char* name;
  const char* text;
  int width_pt;
  int height_pt;
  int border_pt_left;
  int border_pt_right;
  int border_pt_top;
  int border_pt_bottom;
  int print_mode;
} olymp_pagesize_t;

typedef struct {
  const olymp_pagesize_t *item;
  size_t n_items;
} olymp_pagesize_list_t;

typedef struct {
  const char* res_name;
  const char* pagesize_name;
  int width_px;
  int height_px;
} olymp_printsize_t;

typedef struct {
  const olymp_printsize_t *item;
  size_t n_items;
} olymp_printsize_list_t;


typedef struct {
  const char *name;
  const char *text;
  const stp_raw_t seq;
} laminate_t;

typedef struct {
  const laminate_t *item;
  size_t n_items;
} laminate_list_t;

typedef struct
{
  int xdpi, ydpi;
  int xsize, ysize;
  char plane;
  int block_min_x, block_min_y;
  int block_max_x, block_max_y;
  const char* pagesize;
  const laminate_t* laminate;
} olympus_privdata_t;

static olympus_privdata_t privdata;

typedef struct {
  int out_channels;
  int ink_channels;
  int bytes_per_out_channel;
  int bytes_per_ink_channel;
  int plane_interlacing;
  char empty_byte;
  unsigned short **image_data;
  int outh_px, outw_px, outt_px, outb_px, outl_px, outr_px;
  int imgh_px, imgw_px;
  int prnh_px, prnw_px, prnt_px, prnb_px, prnl_px, prnr_px;
  int print_mode;	/* portrait or landscape */
} olympus_print_vars_t;

typedef struct /* printer specific parameters */
{
  int model;		/* printer model number from printers.xml*/
  const ink_list_t *inks;
  const olymp_resolution_list_t *resolution;
  const olymp_pagesize_list_t *pages;
  const olymp_printsize_list_t *printsize;
  int block_size;
  int features;		
  void (*printer_init_func)(stp_vars_t *);
  void (*printer_end_func)(stp_vars_t *);
  void (*plane_init_func)(stp_vars_t *);
  void (*plane_end_func)(stp_vars_t *);
  void (*block_init_func)(stp_vars_t *);
  void (*block_end_func)(stp_vars_t *);
  const char *adj_cyan;		/* default color adjustment */
  const char *adj_magenta;
  const char *adj_yellow;
  const laminate_list_t *laminate;
} olympus_cap_t;


static const olympus_cap_t* olympus_get_model_capabilities(int model);
static const laminate_t* olympus_get_laminate_pattern(stp_vars_t *v);
static void  olympus_print_bytes(stp_vars_t *v, char byte, int count);


static const ink_t cmy_inks[] =
{
  { "CMY", 3, "CMY", "\1\2\3" },
};

static const ink_list_t cmy_ink_list =
{
  cmy_inks, sizeof(cmy_inks) / sizeof(ink_t)
};

static const ink_t ymc_inks[] =
{
  { "CMY", 3, "CMY", "\3\2\1" },
};

static const ink_list_t ymc_ink_list =
{
  ymc_inks, sizeof(ymc_inks) / sizeof(ink_t)
};

static const ink_t rgb_inks[] =
{
  { "RGB", 3, "RGB", "\1\2\3" },
};

static const ink_list_t rgb_ink_list =
{
  rgb_inks, sizeof(rgb_inks) / sizeof(ink_t)
};

static const ink_t bgr_inks[] =
{
  { "RGB", 3, "RGB", "\3\2\1" },
};

static const ink_list_t bgr_ink_list =
{
  bgr_inks, sizeof(bgr_inks) / sizeof(ink_t)
};


/* Olympus P-10 */
static const olymp_resolution_t res_320dpi[] =
{
  { "320x320", 320, 320},
};

static const olymp_resolution_list_t res_320dpi_list =
{
  res_320dpi, sizeof(res_320dpi) / sizeof(olymp_resolution_t)
};

static const olymp_pagesize_t p10_page[] =
{
  { "w288h432", "4 x 6", -1, -1, 0, 0, 16, 0, OLYMPUS_PORTRAIT}, /* 4x6" */
  { "B7", "3.5 x 5", -1, -1, 0, 0, 4, 0, OLYMPUS_PORTRAIT},	 /* 3.5x5" */
  { "Custom", NULL, -1, -1, 28, 28, 48, 48, OLYMPUS_PORTRAIT},
};

static const olymp_pagesize_list_t p10_page_list =
{
  p10_page, sizeof(p10_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t p10_printsize[] =
{
  { "320x320", "w288h432", 1280, 1848},
  { "320x320", "B7",  1144,  1591},
  { "320x320", "Custom", 1280, 1848},
};

static const olymp_printsize_list_t p10_printsize_list =
{
  p10_printsize, sizeof(p10_printsize) / sizeof(olymp_printsize_t)
};

static void p10_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("\033R\033M\033S\2\033N\1\033D\1\033Y", 1, 15, v);
  stp_zfwrite((privdata.laminate->seq).data, 1,
		  (privdata.laminate->seq).bytes, v); /* laminate */
  stp_zfwrite("\033Z\0", 1, 3, v);
}

static void p10_printer_end_func(stp_vars_t *v)
{
  stp_zfwrite("\033P", 1, 2, v);
}

static void p10_block_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033T%c", privdata.plane);
  stp_put16_le(privdata.block_min_x, v);
  stp_put16_le(privdata.block_min_y, v);
  stp_put16_le(privdata.block_max_x + 1, v);
  stp_put16_le(privdata.block_max_y + 1, v);
}

static const laminate_t p10_laminate[] =
{
  {"Coated",  N_("Coated"),  {1, "\x00"}},
  {"None",    N_("None"),    {1, "\x02"}},
};

static const laminate_list_t p10_laminate_list =
{
  p10_laminate, sizeof(p10_laminate) / sizeof(laminate_t)
};


/* Olympus P-200 series */
static const olymp_pagesize_t p200_page[] =
{
  { "ISOB7", "80x125mm", -1, -1, 16, 17, 33, 33, OLYMPUS_PORTRAIT},
  { "Custom", NULL, -1, -1, 16, 17, 33, 33, OLYMPUS_PORTRAIT},
};

static const olymp_pagesize_list_t p200_page_list =
{
  p200_page, sizeof(p200_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t p200_printsize[] =
{
  { "320x320", "ISOB7", 960, 1280},
  { "320x320", "Custom", 960, 1280},
};

static const olymp_printsize_list_t p200_printsize_list =
{
  p200_printsize, sizeof(p200_printsize) / sizeof(olymp_printsize_t)
};

static void p200_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("S000001\0S010001\1", 1, 16, v);
}

static void p200_plane_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "P0%d9999", 3 - privdata.plane+1 );
  stp_put32_be(privdata.xsize * privdata.ysize, v);
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


/* Olympus P-300 series */
static const olymp_resolution_t p300_res[] =
{
  { "306x306", 306, 306},
  { "153x153", 153, 153},
};

static const olymp_resolution_list_t p300_res_list =
{
  p300_res, sizeof(p300_res) / sizeof(olymp_resolution_t)
};

static const olymp_pagesize_t p300_page[] =
{
  { "A6", NULL, -1, -1, 28, 28, 48, 48, OLYMPUS_PORTRAIT},
  { "Custom", NULL, -1, -1, 28, 28, 48, 48, OLYMPUS_PORTRAIT},
};

static const olymp_pagesize_list_t p300_page_list =
{
  p300_page, sizeof(p300_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t p300_printsize[] =
{
  { "306x306", "A6", 1024, 1376},
  { "153x153", "A6",  512,  688},
  { "306x306", "Custom", 1024, 1376},
  { "153x153", "Custom", 512, 688},
};

static const olymp_printsize_list_t p300_printsize_list =
{
  p300_printsize, sizeof(p300_printsize) / sizeof(olymp_printsize_t)
};

static void p300_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("\033\033\033C\033N\1\033F\0\1\033MS\xff\xff\xff"
	      "\033Z", 1, 19, v);
  stp_put16_be(privdata.xdpi, v);
  stp_put16_be(privdata.ydpi, v);
}

static void p300_plane_end_func(stp_vars_t *v)
{
  const char *c = "CMY";
  stp_zprintf(v, "\033\033\033P%cS", c[privdata.plane-1]);
  stp_deprintf(STP_DBG_OLYMPUS, "olympus: p300_plane_end_func: %c\n",
	c[privdata.plane-1]);
}

static void p300_block_init_func(stp_vars_t *v)
{
  const char *c = "CMY";
  stp_zprintf(v, "\033\033\033W%c", c[privdata.plane-1]);
  stp_put16_be(privdata.block_min_y, v);
  stp_put16_be(privdata.block_min_x, v);
  stp_put16_be(privdata.block_max_y, v);
  stp_put16_be(privdata.block_max_x, v);

  stp_deprintf(STP_DBG_OLYMPUS, "olympus: p300_block_init_func: %d-%dx%d-%d\n",
	privdata.block_min_x, privdata.block_max_x,
	privdata.block_min_y, privdata.block_max_y);
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


/* Olympus P-400 series */
static const olymp_resolution_t res_314dpi[] =
{
  { "314x314", 314, 314},
};

static const olymp_resolution_list_t res_314dpi_list =
{
  res_314dpi, sizeof(res_314dpi) / sizeof(olymp_resolution_t)
};

static const olymp_pagesize_t p400_page[] =
{
  { "A4", NULL, -1, -1, 22, 22, 54, 54, OLYMPUS_PORTRAIT},
  { "c8x10", "A5 wide", -1, -1, 58, 59, 84, 85, OLYMPUS_PORTRAIT},
  { "C6", "2 Postcards (A4)", -1, -1, 9, 9, 9, 9, OLYMPUS_PORTRAIT},
  { "Custom", NULL, -1, -1, 22, 22, 54, 54, OLYMPUS_PORTRAIT},
};

static const olymp_pagesize_list_t p400_page_list =
{
  p400_page, sizeof(p400_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t p400_printsize[] =
{
  { "314x314", "A4", 2400, 3200},
  { "314x314", "c8x10", 2000, 2400},
  { "314x314", "C6", 1328, 1920},
  { "314x314", "Custom", 2400, 3200},
};

static const olymp_printsize_list_t p400_printsize_list =
{
  p400_printsize, sizeof(p400_printsize) / sizeof(olymp_printsize_t)
};

static void p400_printer_init_func(stp_vars_t *v)
{
  int wide = (strcmp(privdata.pagesize, "c8x10") == 0
		  || strcmp(privdata.pagesize, "C6") == 0);

  stp_zprintf(v, "\033ZQ"); olympus_print_bytes(v, '\0', 61);
  stp_zprintf(v, "\033FP"); olympus_print_bytes(v, '\0', 61);
  stp_zprintf(v, "\033ZF");
  stp_putc((wide ? '\x40' : '\x00'), v); olympus_print_bytes(v, '\0', 60);
  stp_zprintf(v, "\033ZS");
  if (wide)
    {
      stp_put16_be(privdata.ysize, v);
      stp_put16_be(privdata.xsize, v);
    }
  else
    {
      stp_put16_be(privdata.xsize, v);
      stp_put16_be(privdata.ysize, v);
    }
  olympus_print_bytes(v, '\0', 57);
  stp_zprintf(v, "\033ZP"); olympus_print_bytes(v, '\0', 61);
}

static void p400_plane_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033ZC"); olympus_print_bytes(v, '\0', 61);
}

static void p400_plane_end_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033P"); olympus_print_bytes(v, '\0', 62);
}

static void p400_block_init_func(stp_vars_t *v)
{
  int wide = (strcmp(privdata.pagesize, "c8x10") == 0
		  || strcmp(privdata.pagesize, "C6") == 0);

  stp_zprintf(v, "\033Z%c", '3' - privdata.plane + 1);
  if (wide)
    {
      stp_put16_be(privdata.ysize - privdata.block_max_y - 1, v);
      stp_put16_be(privdata.xsize - privdata.block_max_x - 1, v);
      stp_put16_be(privdata.block_max_y - privdata.block_min_y + 1, v);
      stp_put16_be(privdata.block_max_x - privdata.block_min_x + 1, v);
    }
  else
    {
      stp_put16_be(privdata.block_min_x, v);
      stp_put16_be(privdata.block_min_y, v);
      stp_put16_be(privdata.block_max_x - privdata.block_min_x + 1, v);
      stp_put16_be(privdata.block_max_y - privdata.block_min_y + 1, v);
    }
  olympus_print_bytes(v, '\0', 53);
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


/* Olympus P-440 series */
static const olymp_pagesize_t p440_page[] =
{
  { "A4", NULL, -1, -1, 10, 9, 54, 54, OLYMPUS_PORTRAIT},
  { "c8x10", "A5 wide", -1, -1, 58, 59, 72, 72, OLYMPUS_PORTRAIT},
  { "C6", "2 Postcards (A4)", -1, -1, 9, 9, 9, 9, OLYMPUS_PORTRAIT},
  { "w255h581", "A6 wide", -1, -1, 25, 25, 25, 24, OLYMPUS_PORTRAIT},
  { "Custom", NULL, -1, -1, 22, 22, 54, 54, OLYMPUS_PORTRAIT},
};

static const olymp_pagesize_list_t p440_page_list =
{
  p440_page, sizeof(p440_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t p440_printsize[] =
{
  { "314x314", "A4", 2508, 3200},
  { "314x314", "c8x10", 2000, 2508},
  { "314x314", "C6", 1328, 1920},
  { "314x314", "w255h581", 892, 2320},
  { "314x314", "Custom", 2508, 3200},
};

static const olymp_printsize_list_t p440_printsize_list =
{
  p440_printsize, sizeof(p440_printsize) / sizeof(olymp_printsize_t)
};

static void p440_printer_init_func(stp_vars_t *v)
{
  int wide = ! (strcmp(privdata.pagesize, "A4") == 0
		  || strcmp(privdata.pagesize, "Custom") == 0);

  stp_zprintf(v, "\033FP"); olympus_print_bytes(v, '\0', 61);
  stp_zprintf(v, "\033Y");
  stp_zfwrite((privdata.laminate->seq).data, 1,
		  (privdata.laminate->seq).bytes, v); /* laminate */ 
  olympus_print_bytes(v, '\0', 61);
  stp_zprintf(v, "\033FC"); olympus_print_bytes(v, '\0', 61);
  stp_zprintf(v, "\033ZF");
  stp_putc((wide ? '\x40' : '\x00'), v); olympus_print_bytes(v, '\0', 60);
  stp_zprintf(v, "\033N\1"); olympus_print_bytes(v, '\0', 61);
  stp_zprintf(v, "\033ZS");
  if (wide)
    {
      stp_put16_be(privdata.ysize, v);
      stp_put16_be(privdata.xsize, v);
    }
  else
    {
      stp_put16_be(privdata.xsize, v);
      stp_put16_be(privdata.ysize, v);
    }
  olympus_print_bytes(v, '\0', 57);
  if (strcmp(privdata.pagesize, "C6") == 0)
    {
      stp_zprintf(v, "\033ZC"); olympus_print_bytes(v, '\0', 61);
    }
}

static void p440_printer_end_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033P"); olympus_print_bytes(v, '\0', 62);
}

static void p440_block_init_func(stp_vars_t *v)
{
  int wide = ! (strcmp(privdata.pagesize, "A4") == 0
		  || strcmp(privdata.pagesize, "Custom") == 0);

  stp_zprintf(v, "\033ZT");
  if (wide)
    {
      stp_put16_be(privdata.ysize - privdata.block_max_y - 1, v);
      stp_put16_be(privdata.xsize - privdata.block_max_x - 1, v);
      stp_put16_be(privdata.block_max_y - privdata.block_min_y + 1, v);
      stp_put16_be(privdata.block_max_x - privdata.block_min_x + 1, v);
    }
  else
    {
      stp_put16_be(privdata.block_min_x, v);
      stp_put16_be(privdata.block_min_y, v);
      stp_put16_be(privdata.block_max_x - privdata.block_min_x + 1, v);
      stp_put16_be(privdata.block_max_y - privdata.block_min_y + 1, v);
    }
  olympus_print_bytes(v, '\0', 53);
}

static void p440_block_end_func(stp_vars_t *v)
{
  int pad = (64 - (((privdata.block_max_x - privdata.block_min_x + 1)
	  * (privdata.block_max_y - privdata.block_min_y + 1) * 3) % 64)) % 64;
  stp_deprintf(STP_DBG_OLYMPUS,
		  "olympus: max_x %d min_x %d max_y %d min_y %d\n",
  		  privdata.block_max_x, privdata.block_min_x,
	  	  privdata.block_max_y, privdata.block_min_y);
  stp_deprintf(STP_DBG_OLYMPUS, "olympus: olympus-p440 padding=%d\n", pad);
  olympus_print_bytes(v, '\0', pad);
}


/* Olympus P-S100 */
static const olymp_pagesize_t ps100_page[] =
{
  { "w288h432", "4 x 6", -1, -1, 0, 0, 17, 0, OLYMPUS_PORTRAIT},	/* 4x6" */
  { "B7", "3.5 x 5", -1, -1, 0, 0, 5, 0, OLYMPUS_PORTRAIT},	/* 3.5x5" */
  { "Custom", NULL, -1, -1, 0, 0, 17, 0, OLYMPUS_PORTRAIT},
};

static const olymp_pagesize_list_t ps100_page_list =
{
  ps100_page, sizeof(ps100_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t ps100_printsize[] =
{
  { "314x314", "w288h432", 1254, 1808},
  { "314x314", "B7", 1120, 1554},
  { "314x314", "Custom", 1254, 1808},
};

static const olymp_printsize_list_t ps100_printsize_list =
{
  ps100_printsize, sizeof(ps100_printsize) / sizeof(olymp_printsize_t)
};

static void ps100_printer_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033U"); olympus_print_bytes(v, '\0', 62);
  
  /* stp_zprintf(v, "\033ZC"); olympus_print_bytes(v, '\0', 61); */
  
  stp_zprintf(v, "\033W"); olympus_print_bytes(v, '\0', 62);
  
  stp_zfwrite("\x30\x2e\x00\xa2\x00\xa0\x00\xa0", 1, 8, v);
  stp_put16_be(privdata.ysize, v);	/* paper height (px) */
  stp_put16_be(privdata.xsize, v);	/* paper width (px) */
  olympus_print_bytes(v, '\0', 3);
  stp_putc('\1', v);	/* number of copies */
  olympus_print_bytes(v, '\0', 8);
  stp_putc('\1', v);
  olympus_print_bytes(v, '\0', 15);
  stp_putc('\6', v);
  olympus_print_bytes(v, '\0', 23);

  stp_zfwrite("\033ZT\0", 1, 4, v);
  stp_put16_be(0, v);			/* image width offset (px) */
  stp_put16_be(0, v);			/* image height offset (px) */
  stp_put16_be(privdata.xsize, v);	/* image width (px) */
  stp_put16_be(privdata.ysize, v);	/* image height (px) */
  olympus_print_bytes(v, '\0', 52);
}

static void ps100_printer_end_func(stp_vars_t *v)
{
  int pad = (64 - (((privdata.block_max_x - privdata.block_min_x + 1)
	  * (privdata.block_max_y - privdata.block_min_y + 1) * 3) % 64)) % 64;
  stp_deprintf(STP_DBG_OLYMPUS,
		  "olympus: max_x %d min_x %d max_y %d min_y %d\n",
  		  privdata.block_max_x, privdata.block_min_x,
	  	  privdata.block_max_y, privdata.block_min_y);
  stp_deprintf(STP_DBG_OLYMPUS, "olympus: olympus-ps100 padding=%d\n", pad);
  olympus_print_bytes(v, '\0', pad);		/* padding to 64B blocks */

  stp_zprintf(v, "\033PY"); olympus_print_bytes(v, '\0', 61);
  stp_zprintf(v, "\033u"); olympus_print_bytes(v, '\0', 62);
}


/* Canon CP-100 series */
static const olymp_pagesize_t cpx00_page[] =
{
  { "Postcard", "Postcard 100x148mm", -1, -1, 13, 13, 16, 18, OLYMPUS_PORTRAIT},
  { "w253h337", "CP_L 89x119mm", -1, -1, 13, 13, 15, 15, OLYMPUS_PORTRAIT},
  { "w244h155", "Card 54x86mm", -1, -1, 15, 15, 13, 13, OLYMPUS_PORTRAIT},
  	/* FIXME: Card size should be  w155h244 and LANDSCAPE */
  { "Custom", NULL, -1, -1, 13, 13, 16, 18, OLYMPUS_PORTRAIT},
};

static const olymp_pagesize_list_t cpx00_page_list =
{
  cpx00_page, sizeof(cpx00_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t cpx00_printsize[] =
{
  { "314x314", "Postcard", 1232, 1808},
  { "314x314", "w253h337", 1100, 1456},
  { "314x314", "w244h155", 1040, 672},	/* FIXME: LANDSCAPE, see above */
  { "314x314", "Custom", 1232, 1808},
};

static const olymp_printsize_list_t cpx00_printsize_list =
{
  cpx00_printsize, sizeof(cpx00_printsize) / sizeof(olymp_printsize_t)
};

static void cpx00_printer_init_func(stp_vars_t *v)
{
  char pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? '\1' :
		(strcmp(privdata.pagesize, "w253h337") == 0 ? '\2' :
		(strcmp(privdata.pagesize, "w244h155") == 0 ? '\3' :
		(strcmp(privdata.pagesize, "w283h566") == 0 ? '\4' : 
		 '\1' ))));

  stp_put16_be(0x4000, v);
  stp_putc('\0', v);
  stp_putc(pg, v);
  olympus_print_bytes(v, '\0', 8);
}

static void cpx00_plane_init_func(stp_vars_t *v)
{
  stp_put16_be(0x4001, v);
  stp_put16_le(3 - privdata.plane, v);
  stp_put32_le(privdata.xsize * privdata.ysize, v);
  olympus_print_bytes(v, '\0', 4);
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


/* Canon CP-220 series */
static const olymp_pagesize_t cp220_page[] =
{
  { "Postcard", "Postcard 100x148mm", -1, -1, 13, 13, 16, 18, OLYMPUS_PORTRAIT},
  { "w253h337", "CP_L 89x119mm", -1, -1, 13, 13, 15, 15, OLYMPUS_PORTRAIT},
  { "w244h155", "Card 54x86mm", -1, -1, 15, 15, 13, 13, OLYMPUS_PORTRAIT},
  	/* FIXME: Card size should be  w155h244 and LANDSCAPE */
  { "w283h566", "Wide 100x200mm", -1, -1, 13, 13, 20, 20, OLYMPUS_PORTRAIT},
  { "Custom", NULL, -1, -1, 13, 13, 16, 18, OLYMPUS_PORTRAIT},
};

static const olymp_pagesize_list_t cp220_page_list =
{
  cp220_page, sizeof(cp220_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t cp220_printsize[] =
{
  { "314x314", "Postcard", 1232, 1808},
  { "314x314", "w253h337", 1100, 1456},
  { "314x314", "w244h155", 1040, 672},	/* FIXME: LANDSCAPE, see above */
  { "314x314", "w283h566", 1232, 2416},
  { "314x314", "Custom", 1232, 1808},
};

static const olymp_printsize_list_t cp220_printsize_list =
{
  cp220_printsize, sizeof(cp220_printsize) / sizeof(olymp_printsize_t)
};


/* Sony UP-DP10 */
static const olymp_resolution_t updp10_res[] =
{
  { "300x300", 300, 300},
};

static const olymp_resolution_list_t updp10_res_list =
{
  updp10_res, sizeof(updp10_res) / sizeof(olymp_resolution_t)
};

static const olymp_pagesize_t updp10_page[] =
{
  { "w288h432", "UPC-10P23 (2:3)", -1, -1, 12, 12, 18, 18, OLYMPUS_LANDSCAPE},
  { "w288h387", "UPC-10P34 (3:4)", -1, -1, 12, 12, 16, 16, OLYMPUS_LANDSCAPE},
  { "w288h432", "UPC-10S01 (Sticker)", -1, -1, 12, 12, 18, 18, OLYMPUS_LANDSCAPE},
  { "Custom", NULL, -1, -1, 12, 12, 0, 0, OLYMPUS_LANDSCAPE},
};

static const olymp_pagesize_list_t updp10_page_list =
{
  updp10_page, sizeof(updp10_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t updp10_printsize[] =
{
  { "300x300", "w288h432", 1200, 1800},
  { "300x300", "w288h387", 1200, 1600},
  { "300x300", "Custom", 1200, 1800},
};

static const olymp_printsize_list_t updp10_printsize_list =
{
  updp10_printsize, sizeof(updp10_printsize) / sizeof(olymp_printsize_t)
};

static void updp10_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("\x98\xff\xff\xff\xff\xff\xff\xff"
	      "\x09\x00\x00\x00\x1b\xee\x00\x00"
	      "\x00\x04", 1, 34, v);
  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v); /*laminate pattern*/
  stp_zfwrite("\x00\x00\x00\x00", 1, 4, v);
  stp_put16_be(privdata.ysize, v);
  stp_put16_be(privdata.xsize, v);
  stp_zfwrite("\x14\x00\x00\x00\x1b\x15\x00\x00"
	      "\x00\x0d\x00\x00\x00\x00\x00\x07"
	      "\x00\x00\x00\x00", 1, 20, v);
  stp_put16_be(privdata.ysize, v);
  stp_put16_be(privdata.xsize, v);
  stp_put32_le(privdata.xsize*privdata.ysize*3+11, v);
  stp_zfwrite("\x1b\xea\x00\x00\x00\x00", 1, 6, v);
  stp_put32_be(privdata.xsize*privdata.ysize*3, v);
  stp_zfwrite("\x00", 1, 1, v);
}

static void updp10_printer_end_func(stp_vars_t *v)
{
	stp_zfwrite("\xff\xff\xff\xff\x07\x00\x00\x00"
		    "\x1b\x0a\x00\x00\x00\x00\x00\xfd"
		    "\xff\xff\xff\xff\xff\xff\xff"
		    , 1, 23, v);
}

static const laminate_t updp10_laminate[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x00"}},
  {"Texture", N_("Texture"), {1, "\x08"}},
  {"Matte",   N_("Matte"),   {1, "\x0c"}},
};

static const laminate_list_t updp10_laminate_list =
{
  updp10_laminate, sizeof(updp10_laminate) / sizeof(laminate_t)
};

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


/* Sony UP-DR150 */
static const olymp_resolution_t updr150_res[] =
{
  { "346x346", 346, 346},
};

static const olymp_resolution_list_t updr150_res_list =
{
  updr150_res, sizeof(updr150_res) / sizeof(olymp_resolution_t)
};

static const olymp_pagesize_t updr150_page[] =
{
  { "w288h432",	"2UPC-153 (4x6)", -1, -1, 0, 0, 3, 2, OLYMPUS_LANDSCAPE},
  { "B7",	"2UPC-154 (3.5x5)", -1, -1, 3, 2, 0, 0, OLYMPUS_LANDSCAPE},
  { "w360h504",	"2UPC-155 (5x7)", -1, -1, 0, 0, 4, 4, OLYMPUS_PORTRAIT},
  { "w432h576",	"2UPC-156 (6x8)", -1, -1, 3, 2, 5, 4, OLYMPUS_PORTRAIT},
  { "Custom", NULL, -1, -1, 0, 0, 3, 2, OLYMPUS_LANDSCAPE},
};

static const olymp_pagesize_list_t updr150_page_list =
{
  updr150_page, sizeof(updr150_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t updr150_printsize[] =
{
  { "346x346", "w288h432", 1382, 2048},
  { "346x346", "B7", 1210, 1728},
  { "346x346", "w360h504", 1728, 2380},
  { "346x346", "w432h576", 2048, 2724},
  { "346x346", "Custom", 1382, 2048},
};

static const olymp_printsize_list_t updr150_printsize_list =
{
  updr150_printsize, sizeof(updr150_printsize) / sizeof(olymp_printsize_t)
};

static void updr150_printer_init_func(stp_vars_t *v)
{
  char pg = '\0';

  stp_zfwrite("\x6a\xff\xff\xff\xef\xff\xff\xff", 1, 8, v);
  if (strcmp(privdata.pagesize,"B7") == 0)
    pg = '\x01';
  else if (strcmp(privdata.pagesize,"w288h432") == 0)
    pg = '\x02';
  else if (strcmp(privdata.pagesize,"w360h504") == 0)
    pg = '\x03';
  else if (strcmp(privdata.pagesize,"w432h576") == 0)
    pg = '\x04';
  stp_putc(pg, v);

  stp_zfwrite("\x00\x00\x00\xfc\xff\xff\xff"
	      "\xfb\xff\xff\xff\xf4\xff\xff\xff"
	      "\xf5\xff\xff\xff\x01\x00\x00\x00"
	      "\x07\x00\x00\x00\x1b\xe5\x00\x00"
	      "\x00\x08\x00\x08\x00\x00\x00\x00"
	      "\x00\x00\x00\x00\x00\x01\x00\xed"
	      "\xff\xff\xff\x07\x00\x00\x00\x1b"
	      "\xee\x00\x00\x00\x02\x00\x02\x00"
	      "\x00\x00\x00\x01\x07\x00\x00\x00"
	      "\x1b\x15\x00\x00\x00\x0d\x00\x0d"
	      "\x00\x00\x00\x00\x00\x00\x00\x07"
	      "\x00\x00\x00\x00", 1, 91, v);
  stp_put16_be(privdata.ysize, v);
  stp_put16_be(privdata.xsize, v);
  stp_zfwrite("\xf9\xff\xff\xff\x07\x00\x00\x00"
	      "\x1b\xe1\x00\x00\x00\x0b\x00\x0b"
	      "\x00\x00\x00\x00\x80\x00\x00\x00"
	      "\x00\x00", 1, 26, v);
  stp_put16_be(privdata.ysize, v);
  stp_put16_be(privdata.xsize, v);
  stp_zfwrite("\xf8\xff\xff\xff\x0b\x00\x00\x00\x1b\xea"
	      "\x00\x00\x00\x00", 1, 14, v);
  stp_put32_be(privdata.xsize*privdata.ysize*3, v);
  stp_zfwrite("\x00", 1, 1, v);
  stp_put32_le(privdata.xsize*privdata.ysize*3, v);
}

static void updr150_printer_end_func(stp_vars_t *v)
{
	stp_zfwrite("\xfc\xff\xff"
		    "\xff\xfa\xff\xff\xff\x07\x00\x00"
		    "\x00\x1b\x0a\x00\x00\x00\x00\x00"
		    "\x07\x00\x00\x00\x1b\x17\x00\x00"
		    "\x00\x00\x00\xf3\xff\xff\xff"
		    , 1, 34, v);
}

/* Fujifilm CX-400 */
static const olymp_resolution_t cx400_res[] =
{
  { "317x316", 317, 316},
};

static const olymp_resolution_list_t cx400_res_list =
{
  cx400_res, sizeof(cx400_res) / sizeof(olymp_resolution_t)
};

static const olymp_pagesize_t cx400_page[] =
{
  { "w288h432", NULL, -1, -1, 23, 23, 28, 28, OLYMPUS_PORTRAIT},
  { "w288h387", "4x5 3/8 (Digital Camera 3:4)", -1, -1, 23, 23, 27, 26, OLYMPUS_PORTRAIT},
  { "w288h504", NULL, -1, -1, 23, 23, 23, 22, OLYMPUS_PORTRAIT},
  { "Custom", NULL, -1, -1, 0, 0, 0, 0, OLYMPUS_PORTRAIT},
};

static const olymp_pagesize_list_t cx400_page_list =
{
  cx400_page, sizeof(cx400_page) / sizeof(olymp_pagesize_t)
};

static const olymp_printsize_t cx400_printsize[] =
{
  { "317x316", "w288h387", 1268, 1658},
  { "317x316", "w288h432", 1268, 1842},
  { "317x316", "w288h504", 1268, 2208},
  { "317x316", "Custom", 1268, 1842},
};

static const olymp_printsize_list_t cx400_printsize_list =
{
  cx400_printsize, sizeof(cx400_printsize) / sizeof(olymp_printsize_t)
};

static void cx400_printer_init_func(stp_vars_t *v)
{
  char pg = '\0';
  const char *pname = "XXXXXX";		  				

  stp_deprintf(STP_DBG_OLYMPUS,
	"olympus: fuji driver %s\n", stp_get_driver(v));
  if (strcmp(stp_get_driver(v),"fujifilm-cx400") == 0)
    pname = "NX1000";
  else if (strcmp(stp_get_driver(v),"fujifilm-cx550") == 0)
    pname = "QX200\0";

  stp_zfwrite("FUJIFILM", 1, 8, v);
  stp_zfwrite(pname, 1, 6, v);
  stp_putc('\0', v);
  stp_put16_le(privdata.xsize, v);
  stp_put16_le(privdata.ysize, v);
  if (strcmp(privdata.pagesize,"w288h504") == 0)
    pg = '\x0d';
  else if (strcmp(privdata.pagesize,"w288h432") == 0)
    pg = '\x0c';
  else if (strcmp(privdata.pagesize,"w288h387") == 0)
    pg = '\x0b';
  stp_putc(pg, v);
  stp_zfwrite("\x00\x00\x00\x00\x00\x01\x00\x01\x00\x00\x00\x00"
		  "\x00\x00\x2d\x00\x00\x00\x00", 1, 19, v);
  stp_zfwrite("FUJIFILM", 1, 8, v);
  stp_zfwrite(pname, 1, 6, v);
  stp_putc('\1', v);
}
  
static const olymp_resolution_t all_resolutions[] =
{
  { "306x306", 306, 306},
  { "153x153", 153, 153},
  { "314x314", 314, 314},
  { "300x300", 300, 300},
  { "317x316", 317, 316},
  { "320x320", 320, 320},
  { "346x346", 346, 346},
};

static const olymp_resolution_list_t all_res_list =
{
  all_resolutions, sizeof(all_resolutions) / sizeof(olymp_resolution_t)
};

static const olympus_cap_t olympus_model_capabilities[] =
{
  { /* Olympus P-10, P-11 */
    2, 		
    &rgb_ink_list,
    &res_320dpi_list,
    &p10_page_list,
    &p10_printsize_list,
    1848,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_FULL_HEIGHT
      | OLYMPUS_FEATURE_PLANE_INTERLACE,
    &p10_printer_init_func, &p10_printer_end_func,
    NULL, NULL, 
    &p10_block_init_func, NULL,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    &p10_laminate_list,
  },
  { /* Olympus P-200 */
    4, 		
    &ymc_ink_list,
    &res_320dpi_list,
    &p200_page_list,
    &p200_printsize_list,
    1280,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_BLOCK_ALIGN
      | OLYMPUS_FEATURE_PLANE_INTERLACE,
    &p200_printer_init_func, &p200_printer_end_func,
    &p200_plane_init_func, NULL,
    NULL, NULL,
    p200_adj_any, p200_adj_any, p200_adj_any,
    NULL,
  },
  { /* Olympus P-300 */
    0, 		
    &ymc_ink_list,
    &p300_res_list,
    &p300_page_list,
    &p300_printsize_list,
    16,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_BLOCK_ALIGN
      | OLYMPUS_FEATURE_PLANE_INTERLACE,
    &p300_printer_init_func, NULL,
    NULL, &p300_plane_end_func,
    &p300_block_init_func, NULL,
    p300_adj_cyan, p300_adj_magenta, p300_adj_yellow,
    NULL,
  },
  { /* Olympus P-400 */
    1,
    &ymc_ink_list,
    &res_314dpi_list,
    &p400_page_list,
    &p400_printsize_list,
    180,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_FULL_HEIGHT
      | OLYMPUS_FEATURE_PLANE_INTERLACE,
    &p400_printer_init_func, NULL,
    &p400_plane_init_func, &p400_plane_end_func,
    &p400_block_init_func, NULL,
    p400_adj_cyan, p400_adj_magenta, p400_adj_yellow,
    NULL,
  },
  { /* Olympus P-440 */
    3,
    &bgr_ink_list,
    &res_314dpi_list,
    &p440_page_list,
    &p440_printsize_list,
    128,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_FULL_HEIGHT,
    &p440_printer_init_func, &p440_printer_end_func,
    NULL, NULL,
    &p440_block_init_func, &p440_block_end_func,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    &p10_laminate_list,
  },
  { /* Olympus P-S100 */
    20,
    &bgr_ink_list,
    &res_314dpi_list,
    &ps100_page_list,
    &ps100_printsize_list,
    1808,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_FULL_HEIGHT,
    &ps100_printer_init_func, &ps100_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL,
  },
  { /* Canon CP-100, CP-200, CP-300 */
    1000,
    &ymc_ink_list,
    &res_314dpi_list,
    &cpx00_page_list,
    &cpx00_printsize_list,
    1808,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_FULL_HEIGHT
      | OLYMPUS_FEATURE_BORDERLESS | OLYMPUS_FEATURE_WHITE_BORDER
      | OLYMPUS_FEATURE_PLANE_INTERLACE,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL,
  },
  { /* Canon CP-220, CP-330, SELPHY CP-400, SELPHY CP-500, SELPHY CP-510,
       SELPHY CP-600, SELPHY CP-710 */
    1001,
    &ymc_ink_list,
    &res_314dpi_list,
    &cp220_page_list,
    &cp220_printsize_list,
    1808,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_FULL_HEIGHT
      | OLYMPUS_FEATURE_BORDERLESS | OLYMPUS_FEATURE_WHITE_BORDER
      | OLYMPUS_FEATURE_PLANE_INTERLACE,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL,
  },
  { /* Sony UP-DP10  */
    2000,
    &cmy_ink_list,
    &updp10_res_list,
    &updp10_page_list,
    &updp10_printsize_list,
    1800,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_FULL_HEIGHT
      | OLYMPUS_FEATURE_BORDERLESS,
    &updp10_printer_init_func, &updp10_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    updp10_adj_cyan, updp10_adj_magenta, updp10_adj_yellow,
    &updp10_laminate_list,
  },
  { /* Sony UP-DR150 */
    2001,
    &rgb_ink_list,
    &updr150_res_list,
    &updr150_page_list,
    &updr150_printsize_list,
    1800,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_FULL_HEIGHT,
    &updr150_printer_init_func, &updr150_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL, 
    NULL,
  },
  { /* Fujifilm Printpix CX-400  */
    3000,
    &rgb_ink_list,
    &cx400_res_list,
    &cx400_page_list,
    &cx400_printsize_list,
    2208,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_FULL_HEIGHT
      | OLYMPUS_FEATURE_BORDERLESS,
    &cx400_printer_init_func, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL,
  },
  { /* Fujifilm Printpix CX-550  */
    3001,
    &rgb_ink_list,
    &cx400_res_list,
    &cx400_page_list,
    &cx400_printsize_list,
    2208,
    OLYMPUS_FEATURE_FULL_WIDTH | OLYMPUS_FEATURE_FULL_HEIGHT
      | OLYMPUS_FEATURE_BORDERLESS,
    &cx400_printer_init_func, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL,
  },
};

static const stp_parameter_t the_parameters[] =
{
  {
    "PageSize", N_("Page Size"), N_("Basic Printer Setup"),
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "MediaType", N_("Media Type"), N_("Basic Printer Setup"),
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "InputSlot", N_("Media Source"), N_("Basic Printer Setup"),
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "Resolution", N_("Resolution"), N_("Basic Printer Setup"),
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "InkType", N_("Ink Type"), N_("Advanced Printer Setup"),
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
  },
  {
    "Laminate", N_("Laminate Pattern"), N_("Advanced Printer Setup"),
    N_("Laminate Pattern"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 0, -1, 1, 0
  },
  {
    "Borderless", N_("Borderless"), N_("Advanced Printer Setup"),
    N_("Print without borders"),
    STP_PARAMETER_TYPE_BOOLEAN, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 0, -1, 1, 0
  },
  {
    "PrintingMode", N_("Printing Mode"), N_("Core Parameter"),
    N_("Printing Output Mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1, 0
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

static const olympus_cap_t* olympus_get_model_capabilities(int model)
{
  int i;
  int models = sizeof(olympus_model_capabilities) / sizeof(olympus_cap_t);

  for (i=0; i<models; i++)
    {
      if (olympus_model_capabilities[i].model == model)
        return &(olympus_model_capabilities[i]);
    }
  stp_deprintf(STP_DBG_OLYMPUS,
  	"olympus: model %d not found in capabilities list.\n", model);
  return &(olympus_model_capabilities[0]);
}

static const laminate_t* olympus_get_laminate_pattern(stp_vars_t *v)
{
  const char *lpar = stp_get_string_parameter(v, "Laminate");
  const olympus_cap_t *caps = olympus_get_model_capabilities(
		  				stp_get_model_id(v));
  const laminate_list_t *llist = caps->laminate;
  const laminate_t *l = NULL;
  int i;

  for (i = 0; i < llist->n_items; i++)
    {
      l = &(llist->item[i]);
      if (strcmp(l->name, lpar) == 0)
	 break;
    }
  return l;
}
  
static void
olympus_printsize(const stp_vars_t *v,
		   int  *width,
		   int  *height)
{
  int i;
  const char *page = stp_get_string_parameter(v, "PageSize");
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const olympus_cap_t *caps = olympus_get_model_capabilities(
		  				stp_get_model_id(v));
  const olymp_printsize_list_t *p = caps->printsize;

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
  stp_erprintf("olympus_printsize: printsize not found (%s, %s)\n",
	       page, resolution);
}

static int
olympus_feature(const olympus_cap_t *caps, int feature)
{
  return ((caps->features & feature) == feature);
}

static stp_parameter_list_t
olympus_list_parameters(const stp_vars_t *v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;

  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  return ret;
}

static void
olympus_parameters(const stp_vars_t *v, const char *name,
	       stp_parameter_t *description)
{
  int	i;
  const olympus_cap_t *caps = olympus_get_model_capabilities(
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
  if (strcmp(name, "PageSize") == 0)
    {
      int default_specified = 0;
      const olymp_pagesize_list_t *p = caps->pages;
      const char* text;

      description->bounds.str = stp_string_list_create();
      for (i = 0; i < p->n_items; i++)
	{
          const stp_papersize_t *pt = stp_get_papersize_by_name(
			  p->item[i].name);

	  text = (p->item[i].text ? p->item[i].text : pt->text);
	  stp_string_list_add_string(description->bounds.str,
			  p->item[i].name, gettext(text));
	  if (! default_specified && pt && pt->width > 0 && pt->height > 0)
	    {
	      description->deflt.str = p->item[i].name;
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
      description->is_active = 0;
    }
  else if (strcmp(name, "InputSlot") == 0)
    {
      description->bounds.str = stp_string_list_create();
      description->is_active = 0;
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      char res_text[24];
      const olymp_resolution_list_t *r = caps->resolution;

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
      if (caps->laminate)
        {
          const laminate_list_t *llist = caps->laminate;

          for (i = 0; i < llist->n_items; i++)
            {
              const laminate_t *l = &(llist->item[i]);
	      stp_string_list_add_string(description->bounds.str,
			  	l->name, gettext(l->text));
	    }
          description->deflt.str =
	  stp_string_list_param(description->bounds.str, 0)->name;
          description->is_active = 1;
        }
    }
  else if (strcmp(name, "Borderless") == 0)
    {
      if (olympus_feature(caps, OLYMPUS_FEATURE_BORDERLESS)) 
        description->is_active = 1;
    }
  else if (strcmp(name, "PrintingMode") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string
	(description->bounds.str, "Color", _("Color"));
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else
    description->is_active = 0;
}


static void
olympus_imageable_area_internal(const stp_vars_t *v,
				int  use_maximum_area,
				int  *left,
				int  *right,
				int  *bottom,
				int  *top,
				int  *print_mode)
{
  int width, height;
  int i;
  const char *page = stp_get_string_parameter(v, "PageSize");
  const stp_papersize_t *pt = stp_get_papersize_by_name(page);
  const olympus_cap_t *caps = olympus_get_model_capabilities(
		  				stp_get_model_id(v));
  const olymp_pagesize_list_t *p = caps->pages;

  for (i = 0; i < p->n_items; i++)
    {
      if (strcmp(p->item[i].name,pt->name) == 0)
        {
/*
    	  if (p->item[i].width_pt >= 0)
    		  stp_set_page_width(v, p->item[i].width_pt);
    	  if (p->item[i].height_pt >= 0)
    		  stp_set_page_height(v, p->item[i].height_pt);
*/

          stp_default_media_size(v, &width, &height);
    
          
	  if (use_maximum_area ||
	      (olympus_feature(caps, OLYMPUS_FEATURE_BORDERLESS) &&
	       stp_get_boolean_parameter(v, "Borderless")))
            {
              *left = 0;
              *top  = 0;
              *right  = width;
              *bottom = height;
            }
	  else
	    {
              *left = p->item[i].border_pt_left;
              *top  = p->item[i].border_pt_top;
              *right  = width  - p->item[i].border_pt_right;
              *bottom = height - p->item[i].border_pt_bottom;
	    }
	  *print_mode = p->item[i].print_mode;
          break;
        }
    }
}

static void
olympus_imageable_area(const stp_vars_t *v,
		       int  *left,
		       int  *right,
		       int  *bottom,
		       int  *top)
{
  int not_used;
  olympus_imageable_area_internal(v, 0, left, right, bottom, top, &not_used);
}

static void
olympus_maximum_imageable_area(const stp_vars_t *v,
			       int  *left,
			       int  *right,
			       int  *bottom,
			       int  *top)
{
  int not_used;
  olympus_imageable_area_internal(v, 1, left, right, bottom, top, &not_used);
}

static void
olympus_limit(const stp_vars_t *v,			/* I */
	    int *width, int *height,
	    int *min_width, int *min_height)
{
  *width = 65535;
  *height = 65535;
  *min_width = 1;
  *min_height =	1;
}

static void
olympus_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  int i;

  *x = -1;
  *y = -1;
  if (resolution)
    {
      for (i = 0; i < all_res_list.n_items; i++)
	{
	  if (strcmp(resolution, all_res_list.item[i].name) == 0)
	    {
	      *x = all_res_list.item[i].xdpi;
	      *y = all_res_list.item[i].ydpi;
	    }
	}
    }  
  return;
}

static const char *
olympus_describe_output(const stp_vars_t *v)
{
  return "CMY";
}

static void
olympus_print_bytes(stp_vars_t *v, char byte, int count)
{
  int i;
  for (i = 0; i < count; i++)
    stp_putc(byte, v);
}

static void
olympus_swap_ints(int *a, int *b)
{
  int t = *a;
  *a = *b; 
  *b = t;
}

static void
olympus_adjust_curve(stp_vars_t *v,
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
olympus_exec(stp_vars_t *v,
		void (*func)(stp_vars_t *),
		const char *debug_string)
{
  if (func)
    {
      stp_deprintf(STP_DBG_OLYMPUS, "olympus: %s\n", debug_string);
      (*func)(v);
    }
}

static int
olympus_interpolate(int oldval, int oldsize, int newsize)
{
  /* 
   * This is simple linear interpolation algorithm.
   * When imagesize <> printsize I need rescale image somehow... :-/ 
   */
  return (int)(oldval * newsize / oldsize);
}

static void
olympus_free_image(unsigned short** image_data, stp_image_t *image)
{
  int image_px_height = stp_image_height(image);
  int i;

  for (i = 0; i< image_px_height; i++)
    if (image_data[i])
      stp_free(image_data[i]);
  if (image_data)
    stp_free(image_data);
}

static unsigned short **
olympus_read_image(stp_vars_t *v,
		olympus_print_vars_t *pv,
		stp_image_t *image)
{
  int image_px_width  = stp_image_width(image);
  int image_px_height = stp_image_height(image);
  int row_size = image_px_width * pv->ink_channels * pv->bytes_per_out_channel;
  unsigned short **image_data;
  unsigned int zero_mask;
  int i;

  image_data = stp_zalloc(image_px_height * sizeof(unsigned short *));
  if (!image_data)
    return NULL;	/* ? out of memory ? */

  for (i = 0; i < image_px_height; i++)
    {
      if (stp_color_get_row(v, image, i, &zero_mask))
        {
	  stp_deprintf(STP_DBG_OLYMPUS,
	  	"olympus_read_image: "
		"stp_color_get_row(..., %d, ...) == 0\n", i);
	  olympus_free_image(image_data, image);
	  return NULL;
	}	
      image_data[i] = stp_malloc(row_size);
      if (!image_data[i])
        {
	  stp_deprintf(STP_DBG_OLYMPUS,
	  	"olympus_read_image: "
		"(image_data[%d] = stp_malloc()) == NULL\n", i);
	  olympus_free_image(image_data, image);
	  return NULL;
	}	
      memcpy(image_data[i], stp_channel_get_output(v), row_size);
    }
  stp_image_conclude(image);

  return image_data;
}

static int
olympus_print_pixel(stp_vars_t *v,
		olympus_print_vars_t *pv,
		int row,
		int col,
		int plane)
{
  unsigned short ink[MAX_INK_CHANNELS * MAX_BYTES_PER_CHANNEL], *out;
  unsigned char *ink_u8;
  int i, j;
  
  if (pv->print_mode == OLYMPUS_LANDSCAPE)
    { /* "rotate" image */
      olympus_swap_ints(&col, &row);
      row = (pv->imgw_px - 1) - row;
    }

  out = &(pv->image_data[row][col * pv->out_channels]);

  for (i = 0; i < pv->ink_channels; i++)
    {
      if (pv->out_channels == pv->ink_channels)
        { /* copy out_channel (image) to equiv ink_channel (printer) */
          ink[i] = out[i];
        }
      else if (pv->out_channels < pv->ink_channels)
        { /* several ink_channels (printer) "share" same out_channel (image) */
          ink[i] = out[i * pv->out_channels / pv->ink_channels];
        }
      else /* (pv->out_channels > pv->ink_channels) */
        { /* merge several out_channels (image) into ink_channel (printer) */
          int avg = 0;
          for (j = 0; j < pv->out_channels / pv->ink_channels; j++)
            avg += out[j + i * pv->out_channels / pv->ink_channels];
	  ink[i] = avg * pv->ink_channels / pv->out_channels;
	}
    }
   
  if (pv->bytes_per_ink_channel == 1) /* convert 16bits to 8bit */
    {
      ink_u8 = (unsigned char *) ink;
      for (i = 0; i < pv->ink_channels; i++)
        ink_u8[i] = ink[i] / 257;
    }
	
  if (pv->plane_interlacing)
    stp_zfwrite((char *) ink + plane, pv->bytes_per_ink_channel, 1, v);
  else
    stp_zfwrite((char *) ink, pv->bytes_per_ink_channel, pv->ink_channels, v);

  return 1;
}

static int
olympus_print_row(stp_vars_t *v,
		olympus_print_vars_t *pv,
		int row,
		int plane)
{
  int ret = 0;
  int w, col;
  
  for (w = 0; w < pv->outw_px; w++)
    {
      col = olympus_interpolate(w, pv->outw_px, pv->imgw_px);
      ret = olympus_print_pixel(v, pv, row, col, plane);
      if (ret > 1)
      	break;
    }
  return ret;
}

static int
olympus_print_plane(stp_vars_t *v,
		olympus_print_vars_t *pv,
		olympus_cap_t *caps,
		int plane)
{
  int ret = 0;
  int h, row;
  int out_bytes = (pv->plane_interlacing ? 1 : pv->ink_channels)
  					* pv->bytes_per_ink_channel;


  for (h = 0; h <= pv->prnb_px - pv->prnt_px; h++)
    {
      if (h % caps->block_size == 0)
        { /* block init */
	  privdata.block_min_y = h + pv->prnt_px;
	  privdata.block_min_x = pv->prnl_px;
	  privdata.block_max_y = MIN(h + pv->prnt_px + caps->block_size - 1,
	  					pv->prnb_px);
	  privdata.block_max_x = pv->prnr_px;

	  olympus_exec(v, caps->block_init_func, "caps->block_init");
	}

      if (h + pv->prnt_px < pv->outt_px || h + pv->prnt_px >= pv->outb_px)
        { /* empty part above or below image area */
          olympus_print_bytes(v, pv->empty_byte, out_bytes * pv->prnw_px);
	}
      else
        {
	  if (olympus_feature(caps, OLYMPUS_FEATURE_FULL_WIDTH)
	  	&& pv->outl_px > 0)
	    { /* empty part left of image area */
              olympus_print_bytes(v, pv->empty_byte, out_bytes * pv->outl_px);
	    }

	  row = olympus_interpolate(h + pv->prnt_px - pv->outt_px,
	  					pv->outh_px, pv->imgh_px);
	  stp_deprintf(STP_DBG_OLYMPUS,
	  	"olympus_print_plane: h = %d, row = %d\n", h, row);
	  ret = olympus_print_row(v, pv, row, plane);

	  if (olympus_feature(caps, OLYMPUS_FEATURE_FULL_WIDTH)
	  	&& pv->outr_px < pv->prnw_px)
	    { /* empty part right of image area */
              olympus_print_bytes(v, pv->empty_byte, out_bytes
	      				* (pv->prnw_px - pv->outr_px));
	    }
	}

      if (h + pv->prnt_px == privdata.block_max_y)
        { /* block end */
	  olympus_exec(v, caps->block_end_func, "caps->block_end");
	}
    }
}

/*
 * olympus_print()
 */
static int
olympus_do_print(stp_vars_t *v, stp_image_t *image)
{
  int i;
  olympus_print_vars_t pv;
  int status = 1;
  const char *ink_order = NULL;

  const int model           = stp_get_model_id(v); 
  const char *ink_type      = stp_get_string_parameter(v, "InkType");
  olympus_cap_t *caps = olympus_get_model_capabilities(model);
  int max_print_px_width = 0;
  int max_print_px_height = 0;
  int xdpi, ydpi;	/* Resolution */

  /* output in 1/72" */
  int out_pt_width  = stp_get_width(v);
  int out_pt_height = stp_get_height(v);
  int out_pt_left   = stp_get_left(v);
  int out_pt_top    = stp_get_top(v);

  /* page in 1/72" */
  int page_pt_width  = stp_get_page_width(v);
  int page_pt_height = stp_get_page_height(v);
  int page_pt_left = 0;
  int page_pt_right = 0;
  int page_pt_top = 0;
  int page_pt_bottom = 0;
  int page_mode;	

  int pl;



  if (!stp_verify(v))
    {
      stp_eprintf(v, _("Print options not verified; cannot print.\n"));
      return 0;
    }

  stp_image_init(image);
  pv.imgw_px = stp_image_width(image);
  pv.imgh_px = stp_image_height(image);

  stp_describe_resolution(v, &xdpi, &ydpi);
  olympus_printsize(v, &max_print_px_width, &max_print_px_height);

  privdata.pagesize = stp_get_string_parameter(v, "PageSize");
  if (caps->laminate)
	  privdata.laminate = olympus_get_laminate_pattern(v);

  if (olympus_feature(caps, OLYMPUS_FEATURE_WHITE_BORDER))
    stp_default_media_size(v, &page_pt_right, &page_pt_bottom);
  else
    olympus_imageable_area_internal(v, 0, &page_pt_left, &page_pt_right,
	&page_pt_bottom, &page_pt_top, &page_mode);
  
  pv.prnw_px = MIN(max_print_px_width,
		  	PX(page_pt_right - page_pt_left, xdpi));
  pv.prnh_px = MIN(max_print_px_height,
			PX(page_pt_bottom - page_pt_top, ydpi));
  pv.outw_px = PX(out_pt_width, xdpi);
  pv.outh_px = PX(out_pt_height, ydpi);


  /* if image size is close enough to output size send out original size */
  if (abs(pv.outw_px - pv.imgw_px) < 5)
      pv.outw_px  = pv.imgw_px;
  if (abs(pv.outh_px - pv.imgh_px) < 5)
      pv.outh_px = pv.imgh_px;

  pv.outw_px = MIN(pv.outw_px, pv.prnw_px);
  pv.outh_px = MIN(pv.outh_px, pv.prnh_px);
  pv.outl_px = MIN(PX(out_pt_left - page_pt_left, xdpi),
			pv.prnw_px - pv.outw_px);
  pv.outt_px = MIN(PX(out_pt_top  - page_pt_top, ydpi),
			pv.prnh_px - pv.outh_px);
  pv.outr_px = pv.outl_px + pv.outw_px;
  pv.outb_px = pv.outt_px  + pv.outh_px;
  

  stp_deprintf(STP_DBG_OLYMPUS,
	      "paper (pt)   %d x %d\n"
	      "image (px)   %d x %d\n"
	      "image (pt)   %d x %d\n"
	      "* out (pt)   %d x %d\n"
	      "* out (px)   %d x %d\n"
	      "* left x top (pt) %d x %d\n"
	      "* left x top (px) %d x %d\n"
	      "border (pt) (%d - %d) = %d x (%d - %d) = %d\n"
	      "printable pixels (px)   %d x %d\n"
	      "res (dpi)               %d x %d\n",
	      page_pt_width, page_pt_height,
	      pv.imgw_px, pv.imgh_px,
	      PT(pv.imgw_px, xdpi), PT(pv.imgh_px, ydpi),
	      out_pt_width, out_pt_height,
	      pv.outw_px, pv.outh_px,
	      out_pt_left, out_pt_top,
	      pv.outl_px, pv.outt_px,
	      page_pt_right, page_pt_left, page_pt_right - page_pt_left,
	      page_pt_bottom, page_pt_top, page_pt_bottom - page_pt_top,
	      pv.prnw_px, pv.prnh_px,
	      xdpi, ydpi
	      );	

  privdata.xdpi = xdpi;
  privdata.ydpi = ydpi;
  privdata.xsize = pv.prnw_px;
  privdata.ysize = pv.prnh_px;

  stp_set_string_parameter(v, "STPIOutputType", "CMY");
  
  olympus_adjust_curve(v, caps->adj_cyan, "CyanCurve");
  olympus_adjust_curve(v, caps->adj_magenta, "MagentaCurve");
  olympus_adjust_curve(v, caps->adj_yellow, "YellowCurve");


  /* FIXME: move this into print_init_drv */
  pv.ink_channels = 1;
  if (ink_type)
    {
      for (i = 0; i < caps->inks->n_items; i++)
	if (strcmp(ink_type, caps->inks->item[i].name) == 0)
	  {
	    stp_set_string_parameter(v, "STPIOutputType",
				     caps->inks->item[i].output_type);
	    pv.ink_channels = caps->inks->item[i].output_channels;
	    ink_order = caps->inks->item[i].channel_order;
	    break;
	  }
    }
  stp_channel_reset(v);
  for (i = 0; i < pv.ink_channels; i++)
    stp_channel_add(v, i, 0, 1.0);
  pv.out_channels = stp_color_init(v, image, 65536);
  pv.bytes_per_ink_channel = 1;		/* FIXME: this is printer dependent */
  pv.bytes_per_out_channel = 2;		/* FIXME: this is ??? */
  pv.image_data = olympus_read_image(v, &pv, image);
  pv.empty_byte = (ink_type &&
 		(strcmp(ink_type, "RGB") == 0 || strcmp(ink_type, "BGR") == 0)
		? '\xff' : '\0');
  pv.plane_interlacing = olympus_feature(caps, OLYMPUS_FEATURE_PLANE_INTERLACE);
  pv.print_mode = page_mode;
  if (!pv.image_data)
      return 2;	
  /* /FIXME */


  stp_set_float_parameter(v, "Density", 1.0);


  if (olympus_feature(caps, OLYMPUS_FEATURE_FULL_HEIGHT))
    {
      pv.prnt_px = 0;
      pv.prnb_px = pv.prnh_px - 1;
    }
  else if (olympus_feature(caps, OLYMPUS_FEATURE_BLOCK_ALIGN))
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
  
  if (olympus_feature(caps, OLYMPUS_FEATURE_FULL_WIDTH))
    {
      pv.prnl_px = 0;
      pv.prnr_px = pv.prnw_px - 1;
    }
  else
    {
      pv.prnl_px = pv.outl_px;
      pv.prnr_px = pv.outr_px;
    }
      
  if (pv.print_mode == OLYMPUS_LANDSCAPE)
    {
      olympus_swap_ints(&pv.outh_px, &pv.outw_px);
      olympus_swap_ints(&pv.outt_px, &pv.outl_px);
      olympus_swap_ints(&pv.outb_px, &pv.outr_px);
      
      olympus_swap_ints(&pv.prnh_px, &pv.prnw_px);
      olympus_swap_ints(&pv.prnt_px, &pv.prnl_px);
      olympus_swap_ints(&pv.prnb_px, &pv.prnr_px);
      
      olympus_swap_ints(&pv.imgh_px, &pv.imgw_px);
    }

  /* printer init */
  olympus_exec(v, caps->printer_init_func, "caps->printer_init");

  for (pl = 0; pl < (pv.plane_interlacing ? pv.ink_channels : 1); pl++)
    {
      privdata.plane = ink_order[pl];
      stp_deprintf(STP_DBG_OLYMPUS, "olympus: plane %d\n", privdata.plane);

      /* plane init */
      olympus_exec(v, caps->plane_init_func, "caps->plane_init");
  
      olympus_print_plane(v, &pv, caps, (int) ink_order[pl] - 1);

      /* plane end */
      olympus_exec(v, caps->plane_end_func, "caps->plane_end");
    }

  /* printer end */
  olympus_exec(v, caps->printer_end_func, "caps->printer_end");

  olympus_free_image(pv.image_data, image);
  return status;
}

static int
olympus_print(const stp_vars_t *v, stp_image_t *image)
{
  int status;
  stp_vars_t *nv = stp_vars_create_copy(v);
  stp_prune_inactive_options(nv);
  status = olympus_do_print(nv, image);
  stp_vars_destroy(nv);
  return status;
}

static const stp_printfuncs_t print_olympus_printfuncs =
{
  olympus_list_parameters,
  olympus_parameters,
  stp_default_media_size,
  olympus_imageable_area,
  olympus_maximum_imageable_area,
  olympus_limit,
  olympus_print,
  olympus_describe_resolution,
  olympus_describe_output,
  stp_verify_printer_params,
  NULL,
  NULL
};




static stp_family_t print_olympus_module_data =
  {
    &print_olympus_printfuncs,
    NULL
  };


static int
print_olympus_module_init(void)
{
  return stp_family_register(print_olympus_module_data.printer_list);
}


static int
print_olympus_module_exit(void)
{
  return stp_family_unregister(print_olympus_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_olympus_LTX_stp_module_version
#define stp_module_data print_olympus_LTX_stp_module_data

stp_module_version_t stp_module_version = {0, 0};

stp_module_t stp_module_data =
  {
    "olympus",
    VERSION,
    "Olympus family driver",
    STP_MODULE_CLASS_FAMILY,
    NULL,
    print_olympus_module_init,
    print_olympus_module_exit,
    (void *) &print_olympus_module_data
  };

