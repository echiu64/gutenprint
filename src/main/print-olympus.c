/*
 * "$Id$"
 *
 *   Print plug-in DyeSub driver (formerly Olympus driver) for the GIMP.
 *
 *   Copyright 2003 - 2006
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

/* #define DNPX2 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include "gutenprint-internal.h"
#include <gutenprint/gutenprint-intl-internal.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#ifdef __GNUC__
#define inline __inline__
#endif

#define DYESUB_FEATURE_NONE		 0x00000000
#define DYESUB_FEATURE_FULL_WIDTH	 0x00000001
#define DYESUB_FEATURE_FULL_HEIGHT	 0x00000002
#define DYESUB_FEATURE_BLOCK_ALIGN	 0x00000004
#define DYESUB_FEATURE_BORDERLESS	 0x00000008
#define DYESUB_FEATURE_WHITE_BORDER	 0x00000010
#define DYESUB_FEATURE_PLANE_INTERLACE	 0x00000020
#define DYESUB_FEATURE_PLANE_LEFTTORIGHT 0x00000040
#define DYESUB_FEATURE_ROW_INTERLACE	 0x00000080
#define DYESUB_FEATURE_12BPP             0x00000100
#define DYESUB_FEATURE_16BPP             0x00000200
#define DYESUB_FEATURE_BIGENDIAN         0x00000400
#define DYESUB_FEATURE_RGBtoYCBCR        0x00000800

#define DYESUB_PORTRAIT  0
#define DYESUB_LANDSCAPE 1

#ifndef MIN
#  define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif /* !MIN */
#ifndef MAX
#  define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif /* !MAX */
#define PX(pt,dpi)	((pt) * (dpi) / 72)
#define PT(px,dpi)	((px) * 72 / (dpi))
#define LIST(list_t, list_name, items_t, items_name) \
	static const list_t list_name = \
	{ \
	  items_name, sizeof(items_name) / sizeof(items_t) \
	}

#define MAX_INK_CHANNELS	3
#define MAX_BYTES_PER_CHANNEL	2
#define SIZE_THRESHOLD		6

/*
 * Random implementation from POSIX.1-2001 to yield reproducible results.
 */
static int xrand(unsigned long *seed)
{
  *seed = *seed * 1103515245ul + 12345ul;
  return ((unsigned) (*seed / 65536ul) % 32768ul);
}

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
  int w_dpi;
  int h_dpi;
} dyesub_resolution_t;

typedef struct {
  const dyesub_resolution_t *item;
  size_t n_items;
} dyesub_resolution_list_t;

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
} dyesub_pagesize_t;

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
} laminate_t;

typedef struct {
  const laminate_t *item;
  size_t n_items;
} laminate_list_t;

typedef struct {
  const char* name;
  const char* text;
  const stp_raw_t seq;
} dyesub_media_t;

typedef struct {
  const dyesub_media_t *item;
  size_t n_items;
} dyesub_media_list_t;

#define NPUTC_BUFSIZE (4096)

typedef struct
{
  int w_dpi, h_dpi;
  int w_size, h_size;
  char plane;
  int block_min_w, block_min_h;
  int block_max_w, block_max_h;
  const char* pagesize;
  const laminate_t* laminate;
  const dyesub_media_t* media;
  int print_mode;
  int bpp;
  char nputc_buf[NPUTC_BUFSIZE];
} dyesub_privdata_t;

static dyesub_privdata_t privdata;

typedef struct {
  int out_channels;
  int ink_channels;
  const char *ink_order;
  int bytes_per_ink_channel;
  int bits_per_ink_channel;
  int byteswap;
  int plane_interlacing;
  int row_interlacing;
  char empty_byte[MAX_INK_CHANNELS];  /* one for each color plane */
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
  const dyesub_media_list_t *media;
} dyesub_cap_t;


static const dyesub_cap_t* dyesub_get_model_capabilities(int model);
static const laminate_t* dyesub_get_laminate_pattern(stp_vars_t *v);
static const dyesub_media_t* dyesub_get_mediatype(stp_vars_t *v);
static void  dyesub_nputc(stp_vars_t *v, char byte, int count);


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

/* Olympus P-10 */
static const dyesub_resolution_t res_310dpi[] =
{
  { "310x310", 310, 310},
};

LIST(dyesub_resolution_list_t, res_310dpi_list, dyesub_resolution_t, res_310dpi);

static const dyesub_pagesize_t p10_page[] =
{
  { "w288h432", "4 x 6", 298, 430, 0, 0, 0, 0, DYESUB_PORTRAIT}, /* 4x6" */
  { "B7", "3.5 x 5", 266, 370, 0, 0, 0, 0, DYESUB_PORTRAIT},	 /* 3.5x5" */
  { "Custom", NULL, 298, 430, 28, 28, 48, 48, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, p10_page_list, dyesub_pagesize_t, p10_page);

static const dyesub_printsize_t p10_printsize[] =
{
  { "310x310", "w288h432", 1280, 1848},
  { "310x310", "B7",  1144,  1591},
  { "310x310", "Custom", 1280, 1848},
};

LIST(dyesub_printsize_list_t, p10_printsize_list, dyesub_printsize_t, p10_printsize);

static void p10_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("\033R\033M\033S\2\033N\1\033D\1\033Y", 1, 15, v);
  stp_write_raw(&(privdata.laminate->seq), v); /* laminate */
  stp_zfwrite("\033Z\0", 1, 3, v);
}

static void p10_printer_end_func(stp_vars_t *v)
{
  stp_zfwrite("\033P", 1, 2, v);
}

static void p10_block_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033T%c", privdata.plane);
  stp_put16_le(privdata.block_min_w, v);
  stp_put16_le(privdata.block_min_h, v);
  stp_put16_le(privdata.block_max_w + 1, v);
  stp_put16_le(privdata.block_max_h + 1, v);
}

static const laminate_t p10_laminate[] =
{
  {"Coated",  N_("Coated"),  {1, "\x00"}},
  {"None",    N_("None"),    {1, "\x02"}},
};

LIST(laminate_list_t, p10_laminate_list, laminate_t, p10_laminate);


/* Olympus P-200 series */
static const dyesub_resolution_t res_320dpi[] =
{
  { "320x320", 320, 320},
};

LIST(dyesub_resolution_list_t, res_320dpi_list, dyesub_resolution_t, res_320dpi);

static const dyesub_pagesize_t p200_page[] =
{
  { "ISOB7", "80x125mm", -1, -1, 16, 17, 33, 33, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 16, 17, 33, 33, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, p200_page_list, dyesub_pagesize_t, p200_page);

static const dyesub_printsize_t p200_printsize[] =
{
  { "320x320", "ISOB7", 960, 1280},
  { "320x320", "Custom", 960, 1280},
};

LIST(dyesub_printsize_list_t, p200_printsize_list, dyesub_printsize_t, p200_printsize);

static void p200_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("S000001\0S010001\1", 1, 16, v);
}

static void p200_plane_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "P0%d9999", 3 - privdata.plane+1 );
  stp_put32_be(privdata.w_size * privdata.h_size, v);
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
static const dyesub_resolution_t p300_res[] =
{
  { "306x306", 306, 306},
  { "153x153", 153, 153},
};

LIST(dyesub_resolution_list_t, p300_res_list, dyesub_resolution_t, p300_res);

static const dyesub_pagesize_t p300_page[] =
{
  { "A6", NULL, -1, -1, 28, 28, 48, 48, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 28, 28, 48, 48, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, p300_page_list, dyesub_pagesize_t, p300_page);

static const dyesub_printsize_t p300_printsize[] =
{
  { "306x306", "A6", 1024, 1376},
  { "153x153", "A6",  512,  688},
  { "306x306", "Custom", 1024, 1376},
  { "153x153", "Custom", 512, 688},
};

LIST(dyesub_printsize_list_t, p300_printsize_list, dyesub_printsize_t, p300_printsize);

static void p300_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("\033\033\033C\033N\1\033F\0\1\033MS\xff\xff\xff"
	      "\033Z", 1, 19, v);
  stp_put16_be(privdata.w_dpi, v);
  stp_put16_be(privdata.h_dpi, v);
}

static void p300_plane_end_func(stp_vars_t *v)
{
  const char *c = "CMY";
  stp_zprintf(v, "\033\033\033P%cS", c[privdata.plane-1]);
  stp_deprintf(STP_DBG_DYESUB, "dyesub: p300_plane_end_func: %c\n",
	c[privdata.plane-1]);
}

static void p300_block_init_func(stp_vars_t *v)
{
  const char *c = "CMY";
  stp_zprintf(v, "\033\033\033W%c", c[privdata.plane-1]);
  stp_put16_be(privdata.block_min_h, v);
  stp_put16_be(privdata.block_min_w, v);
  stp_put16_be(privdata.block_max_h, v);
  stp_put16_be(privdata.block_max_w, v);

  stp_deprintf(STP_DBG_DYESUB, "dyesub: p300_block_init_func: %d-%dx%d-%d\n",
	privdata.block_min_w, privdata.block_max_w,
	privdata.block_min_h, privdata.block_max_h);
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
static const dyesub_resolution_t res_314dpi[] =
{
  { "314x314", 314, 314},
};

LIST(dyesub_resolution_list_t, res_314dpi_list, dyesub_resolution_t, res_314dpi);

static const dyesub_pagesize_t p400_page[] =
{
  { "A4", NULL, -1, -1, 22, 22, 54, 54, DYESUB_PORTRAIT},
  { "c8x10", "A5 wide", -1, -1, 58, 59, 84, 85, DYESUB_PORTRAIT},
  { "C6", "2 Postcards (A4)", -1, -1, 9, 9, 9, 9, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 22, 22, 54, 54, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, p400_page_list, dyesub_pagesize_t, p400_page);

static const dyesub_printsize_t p400_printsize[] =
{
  { "314x314", "A4", 2400, 3200},
  { "314x314", "c8x10", 2000, 2400},
  { "314x314", "C6", 1328, 1920},
  { "314x314", "Custom", 2400, 3200},
};

LIST(dyesub_printsize_list_t, p400_printsize_list, dyesub_printsize_t, p400_printsize);

static void p400_printer_init_func(stp_vars_t *v)
{
  int wide = (strcmp(privdata.pagesize, "c8x10") == 0
		  || strcmp(privdata.pagesize, "C6") == 0);

  stp_zprintf(v, "\033ZQ"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033FP"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033ZF");
  stp_putc((wide ? '\x40' : '\x00'), v); dyesub_nputc(v, '\0', 60);
  stp_zprintf(v, "\033ZS");
  if (wide)
    {
      stp_put16_be(privdata.h_size, v);
      stp_put16_be(privdata.w_size, v);
    }
  else
    {
      stp_put16_be(privdata.w_size, v);
      stp_put16_be(privdata.h_size, v);
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
  int wide = (strcmp(privdata.pagesize, "c8x10") == 0
		  || strcmp(privdata.pagesize, "C6") == 0);

  stp_zprintf(v, "\033Z%c", '3' - privdata.plane + 1);
  if (wide)
    {
      stp_put16_be(privdata.h_size - privdata.block_max_h - 1, v);
      stp_put16_be(privdata.w_size - privdata.block_max_w - 1, v);
      stp_put16_be(privdata.block_max_h - privdata.block_min_h + 1, v);
      stp_put16_be(privdata.block_max_w - privdata.block_min_w + 1, v);
    }
  else
    {
      stp_put16_be(privdata.block_min_w, v);
      stp_put16_be(privdata.block_min_h, v);
      stp_put16_be(privdata.block_max_w - privdata.block_min_w + 1, v);
      stp_put16_be(privdata.block_max_h - privdata.block_min_h + 1, v);
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


/* Olympus P-440 series */
static const dyesub_pagesize_t p440_page[] =
{
  { "A4", NULL, -1, -1, 10, 9, 54, 54, DYESUB_PORTRAIT},
  { "c8x10", "A5 wide", -1, -1, 58, 59, 72, 72, DYESUB_PORTRAIT},
  { "C6", "2 Postcards (A4)", -1, -1, 9, 9, 9, 9, DYESUB_PORTRAIT},
  { "w255h581", "A6 wide", -1, -1, 25, 25, 25, 24, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 22, 22, 54, 54, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, p440_page_list, dyesub_pagesize_t, p440_page);

static const dyesub_printsize_t p440_printsize[] =
{
  { "314x314", "A4", 2508, 3200},
  { "314x314", "c8x10", 2000, 2508},
  { "314x314", "C6", 1328, 1920},
  { "314x314", "w255h581", 892, 2320},
  { "314x314", "Custom", 2508, 3200},
};

LIST(dyesub_printsize_list_t, p440_printsize_list, dyesub_printsize_t, p440_printsize);

static void p440_printer_init_func(stp_vars_t *v)
{
  int wide = ! (strcmp(privdata.pagesize, "A4") == 0
		  || strcmp(privdata.pagesize, "Custom") == 0);

  stp_zprintf(v, "\033FP"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033Y");
  stp_write_raw(&(privdata.laminate->seq), v); /* laminate */ 
  dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033FC"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033ZF");
  stp_putc((wide ? '\x40' : '\x00'), v); dyesub_nputc(v, '\0', 60);
  stp_zprintf(v, "\033N\1"); dyesub_nputc(v, '\0', 61);
  stp_zprintf(v, "\033ZS");
  if (wide)
    {
      stp_put16_be(privdata.h_size, v);
      stp_put16_be(privdata.w_size, v);
    }
  else
    {
      stp_put16_be(privdata.w_size, v);
      stp_put16_be(privdata.h_size, v);
    }
  dyesub_nputc(v, '\0', 57);
  if (strcmp(privdata.pagesize, "C6") == 0)
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
  int wide = ! (strcmp(privdata.pagesize, "A4") == 0
		  || strcmp(privdata.pagesize, "Custom") == 0);

  stp_zprintf(v, "\033ZT");
  if (wide)
    {
      stp_put16_be(privdata.h_size - privdata.block_max_h - 1, v);
      stp_put16_be(privdata.w_size - privdata.block_max_w - 1, v);
      stp_put16_be(privdata.block_max_h - privdata.block_min_h + 1, v);
      stp_put16_be(privdata.block_max_w - privdata.block_min_w + 1, v);
    }
  else
    {
      stp_put16_be(privdata.block_min_w, v);
      stp_put16_be(privdata.block_min_h, v);
      stp_put16_be(privdata.block_max_w - privdata.block_min_w + 1, v);
      stp_put16_be(privdata.block_max_h - privdata.block_min_h + 1, v);
    }
  dyesub_nputc(v, '\0', 53);
}

static void p440_block_end_func(stp_vars_t *v)
{
  int pad = (64 - (((privdata.block_max_w - privdata.block_min_w + 1)
	  * (privdata.block_max_h - privdata.block_min_h + 1) * 3) % 64)) % 64;
  stp_deprintf(STP_DBG_DYESUB,
		  "dyesub: max_x %d min_x %d max_y %d min_y %d\n",
  		  privdata.block_max_w, privdata.block_min_w,
	  	  privdata.block_max_h, privdata.block_min_h);
  stp_deprintf(STP_DBG_DYESUB, "dyesub: olympus-p440 padding=%d\n", pad);
  dyesub_nputc(v, '\0', pad);
}


/* Olympus P-S100 */
static const dyesub_pagesize_t ps100_page[] =
{
  { "w288h432", "4 x 6", 296, 426, 0, 0, 0, 0, DYESUB_PORTRAIT},/* 4x6" */
  { "B7", "3.5 x 5", 264, 366, 0, 0, 0, 0, DYESUB_PORTRAIT},	/* 3.5x5" */
  { "Custom", NULL, 296, 426, 0, 0, 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, ps100_page_list, dyesub_pagesize_t, ps100_page);

static const dyesub_printsize_t ps100_printsize[] =
{
  { "306x306", "w288h432", 1254, 1808},
  { "306x306", "B7", 1120, 1554},
  { "306x306", "Custom", 1254, 1808},
};

LIST(dyesub_printsize_list_t, ps100_printsize_list, dyesub_printsize_t, ps100_printsize);

static void ps100_printer_init_func(stp_vars_t *v)
{
  stp_zprintf(v, "\033U"); dyesub_nputc(v, '\0', 62);
  
  /* stp_zprintf(v, "\033ZC"); dyesub_nputc(v, '\0', 61); */
  
  stp_zprintf(v, "\033W"); dyesub_nputc(v, '\0', 62);
  
  stp_zfwrite("\x30\x2e\x00\xa2\x00\xa0\x00\xa0", 1, 8, v);
  stp_put16_be(privdata.h_size, v);	/* paper height (px) */
  stp_put16_be(privdata.w_size, v);	/* paper width (px) */
  dyesub_nputc(v, '\0', 3);
  stp_putc('\1', v);	/* number of copies */
  dyesub_nputc(v, '\0', 8);
  stp_putc('\1', v);
  dyesub_nputc(v, '\0', 15);
  stp_putc('\6', v);
  dyesub_nputc(v, '\0', 23);

  stp_zfwrite("\033ZT\0", 1, 4, v);
  stp_put16_be(0, v);			/* image width offset (px) */
  stp_put16_be(0, v);			/* image height offset (px) */
  stp_put16_be(privdata.w_size, v);	/* image width (px) */
  stp_put16_be(privdata.h_size, v);	/* image height (px) */
  dyesub_nputc(v, '\0', 52);
}

static void ps100_printer_end_func(stp_vars_t *v)
{
  int pad = (64 - (((privdata.block_max_w - privdata.block_min_w + 1)
	  * (privdata.block_max_h - privdata.block_min_h + 1) * 3) % 64)) % 64;
  stp_deprintf(STP_DBG_DYESUB,
		  "dyesub: max_x %d min_x %d max_y %d min_y %d\n",
  		  privdata.block_max_w, privdata.block_min_w,
	  	  privdata.block_max_h, privdata.block_min_h);
  stp_deprintf(STP_DBG_DYESUB, "dyesub: olympus-ps100 padding=%d\n", pad);
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
  { "w155h244", "Card 54x86mm", 159, 250, 6, 6, 29, 29, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 6, 6, 29, 29, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, cp10_page_list, dyesub_pagesize_t, cp10_page);

static const dyesub_printsize_t cp10_printsize[] =
{
  { "300x300", "w155h244", 662, 1040},
  { "300x300", "Custom", 662, 1040},
};

LIST(dyesub_printsize_list_t, cp10_printsize_list, dyesub_printsize_t, cp10_printsize);

/* Canon CP-100 series */
static const dyesub_pagesize_t cpx00_page[] =
{
  { "Postcard", "Postcard 100x148mm", 296, 434, 13, 13, 16, 19, DYESUB_PORTRAIT},
  { "w253h337", "CP_L 89x119mm", 264, 350, 13, 13, 15, 15, DYESUB_PORTRAIT},
  { "w155h244", "Card 54x86mm", 162, 250, 13, 13, 15, 15, DYESUB_LANDSCAPE},
  { "Custom", NULL, 296, 434, 13, 13, 16, 19, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, cpx00_page_list, dyesub_pagesize_t, cpx00_page);

static const dyesub_printsize_t cpx00_printsize[] =
{
  { "300x300", "Postcard", 1232, 1808},
  { "300x300", "w253h337", 1100, 1456},
  { "300x300", "w155h244", 672, 1040},
  { "300x300", "Custom", 1232, 1808},
};

LIST(dyesub_printsize_list_t, cpx00_printsize_list, dyesub_printsize_t, cpx00_printsize);

static void cp10_printer_init_func(stp_vars_t *v)
{
  stp_put16_be(0x4000, v);
  dyesub_nputc(v, '\0', 10);
}

static void cpx00_printer_init_func(stp_vars_t *v)
{
  char pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? '\1' :
		(strcmp(privdata.pagesize, "w253h337") == 0 ? '\2' :
		(strcmp(privdata.pagesize, "w155h244") == 0 ? 
			(strcmp(stp_get_driver(v),"canon-cp10") == 0 ?
				'\0' : '\3' ) :
		(strcmp(privdata.pagesize, "w283h566") == 0 ? '\4' :
		 '\1' ))));

  stp_put16_be(0x4000, v);
  stp_putc('\0', v);
  stp_putc(pg, v);
  dyesub_nputc(v, '\0', 8);
}

static void cpx00_plane_init_func(stp_vars_t *v)
{
  stp_put16_be(0x4001, v);
  stp_putc(3 - privdata.plane, v);
  stp_putc('\0', v);
  stp_put32_le(privdata.w_size * privdata.h_size, v);
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


/* Canon CP-220 series */
static const dyesub_pagesize_t cp220_page[] =
{
  { "Postcard", "Postcard 100x148mm", 296, 434, 13, 13, 16, 19, DYESUB_PORTRAIT},
  { "w253h337", "CP_L 89x119mm", 264, 350, 13, 13, 15, 15, DYESUB_PORTRAIT},
  { "w155h244", "Card 54x86mm", 162, 250, 13, 13, 15, 15, DYESUB_LANDSCAPE},
  { "w283h566", "Wide 100x200mm", 296, 580, 13, 13, 20, 20, DYESUB_PORTRAIT},
  { "Custom", NULL, 296, 434, 13, 13, 16, 19, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, cp220_page_list, dyesub_pagesize_t, cp220_page);

static const dyesub_printsize_t cp220_printsize[] =
{
  { "300x300", "Postcard", 1232, 1808},
  { "300x300", "w253h337", 1100, 1456},
  { "300x300", "w155h244", 672, 1040},
  { "300x300", "w283h566", 1232, 2416},
  { "300x300", "Custom", 1232, 1808},
};

LIST(dyesub_printsize_list_t, cp220_printsize_list, dyesub_printsize_t, cp220_printsize);

/* Canon SELPHY CP790 */
static void cp790_printer_init_func(stp_vars_t *v)
{
  char pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? '\0' :
		(strcmp(privdata.pagesize, "w253h337") == 0 ? '\1' :
		(strcmp(privdata.pagesize, "w155h244") == 0 ? '\2' :
		(strcmp(privdata.pagesize, "w283h566") == 0 ? '\3' : 
		 '\0' ))));

  stp_put16_be(0x4000, v);
  stp_putc(pg, v);
  stp_putc('\0', v);
  dyesub_nputc(v, '\0', 8);
  stp_put32_le(privdata.w_size * privdata.h_size, v);
}

/* Canon SELPHY ES series */
static void es1_printer_init_func(stp_vars_t *v)
{
  char pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? 0x11 :
	     (strcmp(privdata.pagesize, "w253h337") == 0 ? 0x12 :
	      (strcmp(privdata.pagesize, "w155h244") == 0 ? 0x13 : 0x11)));

  stp_put16_be(0x4000, v);
  stp_putc(0x10, v);  /* 0x20 for P-BW */
  stp_putc(pg, v);
  dyesub_nputc(v, '\0', 8);
}

static void es1_plane_init_func(stp_vars_t *v)
{
  unsigned char plane = 0;

  switch (privdata.plane) {
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
  stp_put32_le(privdata.w_size * privdata.h_size, v);
  dyesub_nputc(v, '\0', 4);
}

static void es2_printer_init_func(stp_vars_t *v)
{
  char pg2 = 0x0;
  char pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? 0x1:
	     (strcmp(privdata.pagesize, "w253h337") == 0 ? 0x2 :
	      (strcmp(privdata.pagesize, "w155h244") == 0 ? 0x3 : 0x1)));

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
  stp_put32_le(privdata.w_size * privdata.h_size, v);
}

static void es2_plane_init_func(stp_vars_t *v)
{
  stp_put16_be(0x4001, v);
  stp_putc(4 - privdata.plane, v);  
  stp_putc(0x0, v);
  dyesub_nputc(v, '\0', 8);
}

static void es3_printer_init_func(stp_vars_t *v)
{
  char pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? 0x1:
	     (strcmp(privdata.pagesize, "w253h337") == 0 ? 0x2 :
	      (strcmp(privdata.pagesize, "w155h244") == 0 ? 0x3 : 0x1)));

    /* We also have Pg and Ps  (Gold/Silver) papers on the ES3/30/40 */

  stp_put16_be(0x4000, v);
  stp_putc(pg, v);
  stp_putc(0x0, v);  /* 0x1 for P-BW */
  dyesub_nputc(v, 0x0, 8);
  stp_put32_le(privdata.w_size * privdata.h_size, v);
}

static void es3_printer_end_func(stp_vars_t *v)
{
  stp_put16_be(0x4020, v);
  dyesub_nputc(v, 0x0, 10);
}

static void es40_printer_init_func(stp_vars_t *v)
{
  char pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? 0x0:
	     (strcmp(privdata.pagesize, "w253h337") == 0 ? 0x1 :
	      (strcmp(privdata.pagesize, "w155h244") == 0 ? 0x2 : 0x0)));

    /* We also have Pg and Ps  (Gold/Silver) papers on the ES3/30/40 */

  stp_put16_be(0x4000, v);
  stp_putc(pg, v);
  stp_putc(0x0, v);  /*  0x1 for P-BW */
  dyesub_nputc(v, 0x0, 8);

  stp_put32_le(privdata.w_size * privdata.h_size, v);
}

/* Canon SELPHY CP900 */
static void cp900_printer_end_func(stp_vars_t *v)
{
  dyesub_nputc(v, 0x0, 4);
}

/* Canon CP820/CP910 */
static const dyesub_pagesize_t cp910_page[] =
{
  { "Postcard", "Postcard 100x148mm", PT(1248,300)+1, PT(1872,300)+1, 13, 13, 16, 19, DYESUB_PORTRAIT},
  { "w253h337", "CP_L 89x119mm", PT(1104,300)+1, PT(1536,300)+1, 13, 13, 15, 15, DYESUB_PORTRAIT},
  { "w155h244", "Card 54x86mm", PT(1088,300)+1, PT(668,300)+1, 13, 13, 15, 15, DYESUB_LANDSCAPE},
  { "Custom", NULL, PT(1248,300)+1, PT(1872,300)+1, 13, 13, 16, 19, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, cp910_page_list, dyesub_pagesize_t, cp910_page);

static const dyesub_printsize_t cp910_printsize[] =
{
  { "300x300", "Postcard", 1248, 1872},
  { "300x300", "w253h337", 1104, 1536},
  { "300x300", "w155h244", 668, 1088},
  { "300x300", "Custom", 1248, 1872},
};

LIST(dyesub_printsize_list_t, cp910_printsize_list, dyesub_printsize_t, cp910_printsize);

static void cp910_printer_init_func(stp_vars_t *v)
{
  char pg;

  stp_zfwrite("\x0f\x00\x00\x40\x00\x00\x00\x00", 1, 8, v);
  stp_zfwrite("\x00\x00\x00\x00\x00\x00\x01\x00", 1, 8, v);
  stp_putc(0x01, v);
  stp_putc(0x00, v);

  pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? 0x50 :
                (strcmp(privdata.pagesize, "w253h337") == 0 ? 0x4c :
                (strcmp(privdata.pagesize, "w155h244") == 0 ? 0x43 :
                 0x50 )));
  stp_putc(pg, v);

  dyesub_nputc(v, '\0', 5);

  pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? 0xe0 :
                (strcmp(privdata.pagesize, "w253h337") == 0 ? 0x80 :
                (strcmp(privdata.pagesize, "w155h244") == 0 ? 0x40 :
                 0xe0 )));
  stp_putc(pg, v);

  stp_putc(0x04, v);
  dyesub_nputc(v, '\0', 2);

  pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? 0x50 :
                (strcmp(privdata.pagesize, "w253h337") == 0 ? 0xc0 :
                (strcmp(privdata.pagesize, "w155h244") == 0 ? 0x9c :
                 0x50 )));
  stp_putc(pg, v);

  pg = (strcmp(privdata.pagesize, "Postcard") == 0 ? 0x07 :
                (strcmp(privdata.pagesize, "w253h337") == 0 ? 0x05 :
                (strcmp(privdata.pagesize, "w155h244") == 0 ? 0x02 :
                 0x07 )));
  stp_putc(pg, v);

  dyesub_nputc(v, '\0', 2);
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
  { "w288h432", "Postcard", PT(1664,403)+1, PT(2466,403)+1, 13, 14, 18, 17,
  							DYESUB_PORTRAIT},
  { "Custom", NULL, PT(1664,403)+1, PT(2466,403)+1, 13, 14, 18, 17,
  							DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, dppex5_page_list, dyesub_pagesize_t, dppex5_page);

static const dyesub_printsize_t dppex5_printsize[] =
{
  { "403x403", "w288h432", 1664, 2466},
  { "403x403", "Custom", 1664, 2466},
};

LIST(dyesub_printsize_list_t, dppex5_printsize_list, dyesub_printsize_t, dppex5_printsize);

static void dppex5_printer_init(stp_vars_t *v)
{
  stp_zfwrite("DPEX\0\0\0\x80", 1, 8, v);
  stp_zfwrite("DPEX\0\0\0\x82", 1, 8, v);
  stp_zfwrite("DPEX\0\0\0\x84", 1, 8, v);
  stp_put32_be(privdata.w_size, v);
  stp_put32_be(privdata.h_size, v);
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
  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v); /*laminate pattern*/
  stp_zfwrite("\0d\0d\0d", 1, 6, v);
  dyesub_nputc(v, '\0', 21);
}

static void dppex5_block_init(stp_vars_t *v)
{
  stp_zfwrite("DPEX\0\0\0\x85", 1, 8, v);
  stp_put32_be((privdata.block_max_w - privdata.block_min_w + 1)
  		* (privdata.block_max_h - privdata.block_min_h + 1) * 3, v);
}

static void dppex5_printer_end(stp_vars_t *v)
{
  stp_zfwrite("DPEX\0\0\0\x83", 1, 8, v);
  stp_zfwrite("DPEX\0\0\0\x81", 1, 8, v);
}

static const laminate_t dppex5_laminate[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x00"}},
  {"Texture", N_("Texture"), {1, "\x01"}},
};

LIST(laminate_list_t, dppex5_laminate_list, laminate_t, dppex5_laminate);


/* Sony UP-DP10 */
static const dyesub_pagesize_t updp10_page[] =
{
  { "w288h432", "UPC-10P23 (4x6)", -1, -1, 12, 12, 18, 18, DYESUB_LANDSCAPE},
  { "w288h387", "UPC-10P34 (4x5)", -1, 384, 12, 12, 16, 16, DYESUB_LANDSCAPE},
#if 0
  /* We can't have two paper sizes that are the same size --rlk 20080813 */
  { "w288h432", "UPC-10S01 (Sticker)", -1, -1, 12, 12, 18, 18, DYESUB_LANDSCAPE},
#endif
  { "Custom", NULL, -1, -1, 12, 12, 0, 0, DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, updp10_page_list, dyesub_pagesize_t, updp10_page);

static const dyesub_printsize_t updp10_printsize[] =
{
  { "300x300", "w288h432", 1200, 1800},
  { "300x300", "w288h387", 1200, 1600},
  { "300x300", "Custom", 1200, 1800},
};

LIST(dyesub_printsize_list_t, updp10_printsize_list, dyesub_printsize_t, updp10_printsize);

static void updp10_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("\x98\xff\xff\xff\xff\xff\xff\xff"
	      "\x09\x00\x00\x00\x1b\xee\x00\x00"
	      "\x00\x02\x00\x00\x01\x12\x00\x00"
	      "\x00\x1b\xe1\x00\x00\x00\x0b\x00"
	      "\x00\x04", 1, 34, v);
  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v); /*laminate pattern*/
  stp_zfwrite("\x00\x00\x00\x00", 1, 4, v);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  stp_zfwrite("\x14\x00\x00\x00\x1b\x15\x00\x00"
	      "\x00\x0d\x00\x00\x00\x00\x00\x07"
	      "\x00\x00\x00\x00", 1, 20, v);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  stp_put32_le(privdata.w_size*privdata.h_size*3+11, v);
  stp_zfwrite("\x1b\xea\x00\x00\x00\x00", 1, 6, v);
  stp_put32_be(privdata.w_size*privdata.h_size*3, v);
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

LIST(laminate_list_t, updp10_laminate_list, laminate_t, updp10_laminate);

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


/* Sony UP-DR100 */
static const dyesub_pagesize_t updr100_page[] =
{
  { "w288h432",	"4x6", 298, 442, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "B7",	"3.5x5", 261, 369, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w360h504",	"5x7", 369, 514, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "w432h576",	"6x8", 442, 588, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "Custom", NULL, 298, 442, 0, 0, 0, 0, DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, updr100_page_list, dyesub_pagesize_t, updr100_page);

static const dyesub_printsize_t updr100_printsize[] =
{
  { "334x334", "w288h432", 1382, 2048},
  { "334x334", "B7", 1210, 1710},
  { "334x334", "w360h504", 1710, 2380},
  { "334x334", "w432h576", 2048, 2724},
  { "334x334", "Custom", 1382, 2048},
};

LIST(dyesub_printsize_list_t, updr100_printsize_list, dyesub_printsize_t, updr100_printsize);

static void updr100_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("UPD8D\x00\x00\x00\x10\x03\x00\x00", 1, 12, v);
  stp_put32_le(privdata.w_size, v);
  stp_put32_le(privdata.h_size, v);
  stp_zfwrite("\x1e\x00\x03\x00\x01\x00\x4e\x01\x00\x00", 1, 10, v);
  stp_write_raw(&(privdata.laminate->seq), v); /* laminate pattern */
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

static const laminate_t updr100_laminate[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x01"}},
  {"Texture", N_("Texture"), {1, "\x03"}},
  {"Matte",   N_("Matte"),   {1, "\x04"}},
};

LIST(laminate_list_t, updr100_laminate_list, laminate_t, updr100_laminate);


/* Sony UP-DR150 */
static const dyesub_resolution_t res_334dpi[] =
{
  { "334x334", 334, 334},
};

LIST(dyesub_resolution_list_t, res_334dpi_list, dyesub_resolution_t, res_334dpi);

static const dyesub_pagesize_t updr150_page[] =
{
  { "w288h432", "2UPC-153 (4x6)", PT(1382,334)+1, PT(2048,334)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "B7", "2UPC-154 (3.5x5)", PT(1210,334)+1, PT(1728,334)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w360h504", "2UPC-155 (5x7)", PT(1728,334)+1, PT(2380,334)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "w432h576", "2UPC-156 (6x8)", PT(2048,334)+1, PT(2724,334)+1, 0, 0, 0, DYESUB_PORTRAIT},
  { "Custom", NULL, PT(1382,334)+1, PT(2048,334)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, updr150_page_list, dyesub_pagesize_t, updr150_page);

static const dyesub_printsize_t updr150_printsize[] =
{
  { "334x334", "w288h432", 1382, 2048},
  { "334x334", "B7", 1210, 1728},
  { "334x334", "w360h504", 1728, 2380},
  { "334x334", "w432h576", 2048, 2724},
  { "334x334", "Custom", 1382, 2048},
};

LIST(dyesub_printsize_list_t, updr150_printsize_list, dyesub_printsize_t, updr150_printsize);

static void updr150_200_printer_init_func(stp_vars_t *v, int updr200)
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
	      "\x00\x00\x00\x01", 1, 67, v);

  if (updr200) { /* UP-DR200-specific! */
    stp_zfwrite("\x07\x00\x00\x00"
		"\x1b\xc0\x00\x03\x00\x05", 1, 10, v);
    stp_putc(0x00, v);  /* 0x02 for doubled-up prints. */
    /* eg 2x6 on 4x6 media, 3.5x5 on 5x7 media, 4x6 on 8x6 media */
  }
    
  stp_zfwrite("\x05\x00\x00\x00"
	      "\x02\x03\x00\x01\x00", 1, 9, v);
  stp_zfwrite("\x07\x00\x00\x00"
	      "\x1b\x15\x00\x00\x00\x0d\x00\x0d"
	      "\x00\x00\x00\x00\x00\x00\x00\x07"
	      "\x00\x00\x00\x00", 1, 24, v);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  stp_zfwrite("\xf9\xff\xff\xff\x07\x00\x00\x00"
	      "\x1b\xe1\x00\x00\x00\x0b\x00\x0b"
	      "\x00\x00\x00\x00\x80", 1, 21, v);

  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v); /*laminate pattern*/

  stp_zfwrite("\x00\x00\x00\x00", 1, 4, v);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  stp_zfwrite("\xf8\xff\xff\xff"
	      "\xec\xff\xff\xff"
	      "\x0b\x00\x00\x00\x1b\xea"
	      "\x00\x00\x00\x00", 1, 18, v);
  stp_put32_be(privdata.w_size*privdata.h_size*3, v);
  stp_zfwrite("\x00", 1, 1, v);
  stp_put32_le(privdata.w_size*privdata.h_size*3, v);
}

static void updr150_printer_init_func(stp_vars_t *v)
{
  updr150_200_printer_init_func(v, 0);
}

static void updr150_printer_end_func(stp_vars_t *v)
{
	stp_zfwrite("\xeb\xff\xff\xff"
		    "\xfc\xff\xff"
		    "\xff\xfa\xff\xff\xff\x07\x00\x00"
		    "\x00\x1b\x0a\x00\x00\x00\x00\x00"
		    "\x07\x00\x00\x00\x1b\x17\x00\x00"
		    "\x00\x00\x00\xf3\xff\xff\xff"
		    , 1, 38, v);
}

/* Sony UP-DR200 */
static const laminate_t updr200_laminate[] =
{
  {"Glossy",  N_("Glossy"),  {1, "\x00"}},
  {"Matte",   N_("Matte"),   {1, "\x0c"}},
};

LIST(laminate_list_t, updr200_laminate_list, laminate_t, updr200_laminate);

static void updr200_printer_init_func(stp_vars_t *v)
{
  updr150_200_printer_init_func(v, 1);
}

/* Sony UP-CR10L / DNP SL10 */
static const dyesub_pagesize_t upcr10_page[] =
{
  { "w288h432", "4x6", PT(1248,300)+1, PT(1848,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "B7", "3.5x5", PT(1100,300)+1, PT(1536,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w360h504", "5x7", PT(1536,300)+1, PT(2148,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, upcr10_page_list, dyesub_pagesize_t, upcr10_page);

static const dyesub_printsize_t upcr10_printsize[] =
{
  { "300x300", "w288h432", 1248, 1848},
  { "300x300", "B7", 1100, 1536},
  { "300x300", "w360h504", 1536, 2148},
};

LIST(dyesub_printsize_list_t, upcr10_printsize_list, dyesub_printsize_t, upcr10_printsize);

static void upcr10_printer_init_func(stp_vars_t *v)
{
	stp_zfwrite("\x60\xff\xff\xff"
		    "\xf8\xff\xff\xff"
		    "\xfd\xff\xff\xff\x14\x00\x00\x00"
		    "\x1b\x15\x00\x00\x00\x0d\x00\x00"
		    "\x00\x00\x00\x07\x00\x00\x00\x00", 1, 32, v);
	stp_put16_be(privdata.w_size, v);
	stp_put16_be(privdata.h_size, v);
	stp_zfwrite("\xfb\xff\xff\xff"
		    "\xf4\xff\xff\xff\x0b\x00\x00\x00"
		    "\x1b\xea\x00\x00\x00\x00", 1, 18, v);
	stp_put32_be(privdata.w_size * privdata.h_size * 3, v);
	stp_putc(0, v);
	stp_put32_le(privdata.w_size * privdata.h_size * 3, v);
}

static void upcr10_printer_end_func(stp_vars_t *v)
{
	stp_zfwrite("\xf3\xff\xff\xff"
		    "\x0f\x00\x00\x00"
		    "\x1b\xe5\x00\x00\x00\x08\x00\x00"
		    "\x00\x00\x00\x00\x00\x0d\x00", 1, 23, v);
	stp_zfwrite("\x12\x00\x00\x00\x1b\xe1\x00\x00"
		    "\x000x0b\x00\x00\x80\x08\x00\x00"
		    "\x00\x00", 1, 18, v);
	stp_put16_be(privdata.w_size, v);
	stp_put16_be(privdata.h_size, v);
	stp_zfwrite("\xfa\xff\xff\xff"
		    "\x09\x00\x00\x00"
		    "\x1b\xee\x00\x00\x00\x02\x00\x00", 1, 16, v);
	stp_putc(1, v); /* Copies */
	stp_zfwrite("\x07\x00\x00\x00"
		    "\x1b\x17\x00\x00\x00\x00\x00", 1, 11, v);
	stp_zfwrite("\xf9\xff\xff\xff"
		    "\xfc\xff\xff\xff"
		    "\x07\x00\x00\x00"
		    "\x1b\x17\x00\x00\x00\x00\x00", 1, 19, v);
	stp_zfwrite("\xf7\xff\xff\xff", 1, 4, v);
}

/* Fujifilm CX-400 */
static const dyesub_pagesize_t cx400_page[] =
{
  { "w288h432", NULL, 295, 428, 24, 24, 23, 22, DYESUB_PORTRAIT},
  { "w288h387", "4x5 3/8", 295, 386, 24, 24, 23, 23, DYESUB_PORTRAIT},
  { "w288h504", NULL, 295, 513, 24, 24, 23, 22, DYESUB_PORTRAIT},
  { "Custom", NULL, 295, 428, 0, 0, 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, cx400_page_list, dyesub_pagesize_t, cx400_page);

static const dyesub_printsize_t cx400_printsize[] =
{
  { "310x310", "w288h387", 1268, 1658},
  { "310x310", "w288h432", 1268, 1842},
  { "310x310", "w288h504", 1268, 2208},
  { "310x310", "Custom", 1268, 1842},
};

LIST(dyesub_printsize_list_t, cx400_printsize_list, dyesub_printsize_t, cx400_printsize);

static void cx400_printer_init_func(stp_vars_t *v)
{
  char pg = '\0';
  const char *pname = "XXXXXX";

  stp_deprintf(STP_DBG_DYESUB,
	"dyesub: fuji driver %s\n", stp_get_driver(v));
  if (strcmp(stp_get_driver(v),"fujifilm-cx400") == 0)
    pname = "NX1000";
  else if (strcmp(stp_get_driver(v),"fujifilm-cx550") == 0)
    pname = "QX200\0";

  stp_zfwrite("FUJIFILM", 1, 8, v);
  stp_zfwrite(pname, 1, 6, v);
  stp_putc('\0', v);
  stp_put16_le(privdata.w_size, v);
  stp_put16_le(privdata.h_size, v);
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
  

/* Fujifilm NX-500 */
static const dyesub_resolution_t res_306dpi[] =
{
  { "306x306", 306, 306},
};

LIST(dyesub_resolution_list_t, res_306dpi_list, dyesub_resolution_t, res_306dpi);

static const dyesub_pagesize_t nx500_page[] =
{
  { "Postcard", NULL, -1, -1, 21, 21, 29, 29, DYESUB_PORTRAIT},
  { "Custom", NULL, -1, -1, 21, 21, 29, 29, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, nx500_page_list, dyesub_pagesize_t, nx500_page);

static const dyesub_printsize_t nx500_printsize[] =
{
  { "306x306", "Postcard", 1024, 1518},
  { "306x306", "Custom", 1024, 1518},
};

LIST(dyesub_printsize_list_t, nx500_printsize_list, dyesub_printsize_t, nx500_printsize);

static void nx500_printer_init_func(stp_vars_t *v)
{
  stp_zfwrite("INFO-QX-20--MKS\x00\x00\x00M\x00W\00A\x00R\00E", 1, 27, v);
  dyesub_nputc(v, '\0', 21);
  stp_zfwrite("\x80\x00\x02", 1, 3, v);
  dyesub_nputc(v, '\0', 20);
  stp_zfwrite("\x02\x01\x01", 1, 3, v);
  dyesub_nputc(v, '\0', 2);
  stp_put16_le(privdata.h_size, v);
  stp_put16_le(privdata.w_size, v);
  stp_zfwrite("\x00\x02\x00\x70\x2f", 1, 5, v);
  dyesub_nputc(v, '\0', 43);
}
  

/* Kodak Easyshare Dock family */
static const dyesub_pagesize_t kodak_dock_page[] =
{
  { "w288h432", NULL, PT(1248,300)+1, PT(1856,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT}, /* 4x6 */
  { "Custom", NULL, PT(1248,300)+1, PT(1856,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT}, /* 4x6 */
};

LIST(dyesub_pagesize_list_t, kodak_dock_page_list, dyesub_pagesize_t, kodak_dock_page);

static const dyesub_printsize_t kodak_dock_printsize[] =
{
  { "300x300", "w288h432", 1248, 1856},
  { "300x300", "Custom", 1248, 1856},
};

LIST(dyesub_printsize_list_t, kodak_dock_printsize_list, dyesub_printsize_t, kodak_dock_printsize);

static void kodak_dock_printer_init(stp_vars_t *v)
{
  stp_put16_be(0x3000, v);
  dyesub_nputc(v, '\0', 10);
}

static void kodak_dock_plane_init(stp_vars_t *v)
{
  stp_put16_be(0x3001, v);
  stp_put16_le(3 - privdata.plane, v);
  stp_put32_le(privdata.w_size*privdata.h_size, v);
  dyesub_nputc(v, '\0', 4);
}

/* Kodak 6800 */
static const dyesub_pagesize_t kodak_6800_page[] =
{
  { "w288h432", "4x6", PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE}, /* 4x6 */
  { "w432h576", "6x8", PT(1844,300)+1, PT(2434,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT}, /* 6x8 */
  { "Custom", NULL, PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE}, /* 4x6 */
};

LIST(dyesub_pagesize_list_t, kodak_6800_page_list, dyesub_pagesize_t, kodak_6800_page);

static const dyesub_printsize_t kodak_6800_printsize[] =
{
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "w432h576", 1844, 2434},
  { "300x300", "Custom", 1240, 1844},
};

LIST(dyesub_printsize_list_t, kodak_6800_printsize_list, dyesub_printsize_t, kodak_6800_printsize);

static const laminate_t kodak_6800_laminate[] =
{
  {"Coated", N_("Coated"), {1, "\x01"}},
  {"None",  N_("None"),  {1, "\x00"}},
};

LIST(laminate_list_t, kodak_6800_laminate_list, laminate_t, kodak_6800_laminate);


static void kodak_6800_printer_init(stp_vars_t *v)
{
  stp_zfwrite("\x03\x1b\x43\x48\x43\x0a\x00\x01\x00", 1, 9, v);
  stp_putc(0x01, v);  /* Number of copies */
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  stp_putc(privdata.h_size == 1240 ? 0x00 : 0x06, v); /* XXX seen it on some 4x6 prints too! */
  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v);
  stp_putc(0x00, v);
}

/* Kodak 6850 */
static const dyesub_pagesize_t kodak_6850_page[] =
{
  { "w288h432", "4x6", PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE}, /* 4x6 */
  { "w360h504", "5x7", PT(1548,300)+1, PT(2140,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT}, /* 5x7 */
  { "w432h576", "6x8", PT(1844,300)+1, PT(2434,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT}, /* 6x8 */
  { "Custom", NULL, PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE}, /* 4x6 */
};

LIST(dyesub_pagesize_list_t, kodak_6850_page_list, dyesub_pagesize_t, kodak_6850_page);

static const dyesub_printsize_t kodak_6850_printsize[] =
{
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "w360h504", 1548, 2140},
  { "300x300", "w432h576", 1844, 2434},
  { "300x300", "Custom", 1240, 1844},
};

LIST(dyesub_printsize_list_t, kodak_6850_printsize_list, dyesub_printsize_t, kodak_6850_printsize);

static void kodak_6850_printer_init(stp_vars_t *v)
{
  stp_zfwrite("\x03\x1b\x43\x48\x43\x0a\x00\x01\x00", 1, 9, v);
  stp_putc(0x01, v); /* Number of copies */
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  stp_putc(privdata.h_size == 1240 ? 0x00 : 
	   privdata.h_size == 1548 ? 0x07 : 0x06, v);
  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v);
  stp_putc(0x00, v);
}

/* Kodak 605 */
static const dyesub_pagesize_t kodak_605_page[] =
{
  { "w288h432", "4x6", PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE}, /* 4x6 */
  { "w360h504", "5x7", PT(1500,300)+1, PT(2100,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT}, /* 5x7 */
  { "w432h576", "6x8", PT(1844,300)+1, PT(2434,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT}, /* 6x8 */
  { "Custom", NULL, PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE}, /* 4x6 */
};

LIST(dyesub_pagesize_list_t, kodak_605_page_list, dyesub_pagesize_t, kodak_605_page);

static const dyesub_printsize_t kodak_605_printsize[] =
{
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "w360h504", 1500, 2100},
  { "300x300", "w432h576", 1844, 2434},
  { "300x300", "Custom", 1240, 1844},
};

LIST(dyesub_printsize_list_t, kodak_605_printsize_list, dyesub_printsize_t, kodak_605_printsize);

static void kodak_605_printer_init(stp_vars_t *v)
{
  stp_zfwrite("\x01\x40\x0a\x00\x01", 1, 5, v);
  stp_putc(0x01, v); /* Number of copies */
  stp_putc(0x00, v);
  stp_put16_le(privdata.w_size, v);
  stp_put16_le(privdata.h_size, v);
  if (privdata.h_size == 1240)
	  stp_putc(0x01, v);
  else if (privdata.h_size == 2100)
	  stp_putc(0x02, v);
  else if (privdata.h_size == 2434)
	  stp_putc(0x03, v);
  else if (privdata.h_size == 2490)
	  stp_putc(0x04, v);
  else
	  stp_putc(0x01, v);

  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v);
  stp_putc(0x00, v);
}

static const laminate_t kodak_605_laminate[] =
{
  {"Coated", N_("Coated"), {1, "\x02"}},
  {"None",  N_("None"),  {1, "\x01"}},
};

LIST(laminate_list_t, kodak_605_laminate_list, laminate_t, kodak_605_laminate);

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
  { "w612h864", "8.5 x 12", PT(2560,301)+1, PT(3010,301)+72*2, PT(76,301)+1, PT(76,301), 72, 72, DYESUB_PORTRAIT}, /* 8x12 */
  { "Legal", "8.5 x 14", PT(2560,301)+1, PT(3612,301)+72*2, PT(35,301)+1, PT(35,301)+1, 72, 72, DYESUB_PORTRAIT}, /* 8x14 */
  { "A4", "A4",       PT(2560,301)+1, PT(3010,301)+72*2, PT(76,301)+1, PT(76,301), 0, 0, DYESUB_PORTRAIT}, /* A4, indentical to 8x12 */
  { "Custom", NULL,   PT(2560,301)+1, PT(3010,301)+72*2, PT(76,301)+1, PT(76,301), 72, 72, DYESUB_PORTRAIT},
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
  { "301x301", "Custom", 2560, 3010},
};

LIST(dyesub_printsize_list_t, kodak_1400_printsize_list, dyesub_printsize_t, kodak_1400_printsize);

static void kodak_1400_printer_init(stp_vars_t *v)
{
  stp_zfwrite("PGHD", 1, 4, v);
  stp_put16_le(privdata.w_size, v);
  dyesub_nputc(v, 0x00, 2);
  stp_put16_le(privdata.h_size, v);
  dyesub_nputc(v, 0x00, 2);
  stp_put32_le(privdata.h_size*privdata.w_size, v);
  dyesub_nputc(v, 0x00, 4);
  stp_zfwrite((privdata.media->seq).data, 1, 1, v);  /* Matte or Glossy? */
  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v);
  stp_putc(0x01, v);
  stp_zfwrite((const char*)((privdata.media->seq).data) + 1, 1, 1, v); /* Lamination intensity */
  dyesub_nputc(v, 0x00, 12);
}

/* Kodak 805 */
static const dyesub_pagesize_t kodak_805_page[] =
{
  /* Identical to the Kodak 1400 except for the lack of A4 support.
     See the 1400 comments for explanations of this. */
  { "w612h864", "8.5 x 12", PT(2560,301)+1, PT(3010,301)+72*2, PT(76,301)+1, PT(76,301), 72, 72, DYESUB_PORTRAIT}, /* 8x12 */
  { "Legal", "8.5 x 14", PT(2560,301)+1, PT(3612,301)+72*2, PT(35,301)+1, PT(35,301)+1, 72, 72, DYESUB_PORTRAIT}, /* 8x14 */
  { "Custom", NULL,   PT(2560,301)+1, PT(3010,301)+72*2, PT(76,301)+1, PT(76,301), 72, 72, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, kodak_805_page_list, dyesub_pagesize_t, kodak_805_page);

static const dyesub_printsize_t kodak_805_printsize[] =
{
  { "301x301", "w612h864", 2560, 3010},
  { "301x301", "Legal", 2560, 3612},
  { "301x301", "Custom", 2560, 3010},
};

LIST(dyesub_printsize_list_t, kodak_805_printsize_list, dyesub_printsize_t, kodak_805_printsize);

static void kodak_805_printer_init(stp_vars_t *v)
{
  stp_zfwrite("PGHD", 1, 4, v);
  stp_put16_le(privdata.w_size, v);
  dyesub_nputc(v, 0x00, 2);
  stp_put16_le(privdata.h_size, v);
  dyesub_nputc(v, 0x00, 2);
  stp_put32_le(privdata.h_size*privdata.w_size, v);
  dyesub_nputc(v, 0x00, 5);
  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v);
  stp_putc(0x01, v);
  stp_putc(0x3c, v); /* Lamination intensity; fixed on glossy media */
  dyesub_nputc(v, 0x00, 12);
}

/* Kodak 9810 */
static const dyesub_pagesize_t kodak_9810_page[] =
{
  { "c8x10", "8x10", PT(2464,300)+1, PT(3024,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "w576h864", "8x12", PT(2464,300)+1, PT(3624,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "Custom", NULL, PT(2464,300)+1, PT(3024,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
};
LIST(dyesub_pagesize_list_t, kodak_9810_page_list, dyesub_pagesize_t, kodak_9810_page);

static const dyesub_printsize_t kodak_9810_printsize[] =
{
  { "300x300", "c8x10", 2464, 3024},
  { "300x300", "w576h864", 2464, 3624},
  { "300x300", "Custom", 2464, 3024},
};

LIST(dyesub_printsize_list_t, kodak_9810_printsize_list, dyesub_printsize_t, kodak_9810_printsize);

static const laminate_t kodak_9810_laminate[] =
{
  {"Coated", N_("Coated"), {3, "\x4f\x6e\x20"}},
  {"None",  N_("None"),  {3, "\x4f\x66\x66"}},
};

LIST(laminate_list_t, kodak_9810_laminate_list, laminate_t, kodak_9810_laminate);

static void kodak_9810_printer_init(stp_vars_t *v)
{
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
  if (privdata.h_size == 3624) {
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
  stp_zfwrite((privdata.laminate->seq).data, 1,
			(privdata.laminate->seq).bytes, v);
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
  stp_put32_be(privdata.w_size, v);
  stp_put32_be(privdata.h_size, v);

  /* Page dimensions II -- maybe this is image data size? */
  stp_putc(0x1b, v);
  stp_zfwrite("MndImSpec  Size    ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(16, v);
  stp_put32_be(privdata.w_size, v);
  stp_put32_be(privdata.h_size, v);
  stp_put32_be(privdata.w_size, v);
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
  stp_putc(0x12, v);  /* SHARPENING -- 0 is off, 0x12 Normal, 0x19 is High */

  /* Number of Copies */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsPgCopies        ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be(4, v);
  stp_put32_be(1, v);  /* Number of copies, at least 1 */

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
  if (privdata.h_size == 3624) {
    stp_zfwrite("\x00\x0c\x0e\x1c", 1, 4, v);
  } else {
    stp_zfwrite("\x00\x0c\x0b\xc4", 1, 4, v);
  }

#if 0  /* Additional Known Cut lists */
  /* Single cut, down the center */
  stp_put32_be(6, v);
  if (privdata.h_size == 3624) {
    stp_zfwrite("\x00\x0c\x07\x14\x0e\x1c", 1, 6, v);
  } else {
    stp_zfwrite("\x00\x0c\x05\xe8\x0b\xc4", 1, 6, v);
  }
  /* Double-Slug Cut, down the center */
  stp_put32_be(8, v);
  if (privdata.h_size == 3624) {
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
  /* Data block */
  stp_putc(0x1b, v);
  stp_zfwrite("FlsData    Block   ", 1, 19, v);
  dyesub_nputc(v, 0x00, 4);
  stp_put32_be((privdata.w_size * privdata.h_size) + 8, v);
  stp_zfwrite("Image   ", 1, 8, v);
}

/* Kodak 8810 */
static const dyesub_pagesize_t kodak_8810_page[] =
{
  { "w288h576", "8x4", PT(1208,300)+1, PT(2464,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "c8x10", "8x10", PT(2464,300)+1, PT(3024,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
//  { "???", "203x297mm", PT(2464,300)+1, PT(3531,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "w576h864", "8x12", PT(2464,300)+1, PT(3624,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "Custom", NULL, PT(2464,300)+1, PT(3624,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
};
LIST(dyesub_pagesize_list_t, kodak_8810_page_list, dyesub_pagesize_t, kodak_8810_page);

static const dyesub_printsize_t kodak_8810_printsize[] =
{
  { "300x300", "w288h576", 1208, 2464},
  { "300x300", "c8x10", 2464, 3024},
//  { "300x300", "???", 2464, 3531},
  { "300x300", "w576h864", 2464, 3624},
  { "300x300", "Custom", 2464, 3624},
};

LIST(dyesub_printsize_list_t, kodak_8810_printsize_list, dyesub_printsize_t, kodak_8810_printsize);

static const laminate_t kodak_8810_laminate[] =
{
  {"Glossy", N_("Glossy"), {1, "\x03"}},
  {"Satin",  N_("Satin"),  {1, "\x02"}},
};

LIST(laminate_list_t, kodak_8810_laminate_list, laminate_t, kodak_8810_laminate);

static void kodak_8810_printer_init(stp_vars_t *v)
{
  stp_putc(0x01, v);
  stp_putc(0x40, v);
  stp_putc(0x12, v);
  stp_putc(0x00, v);
  stp_putc(0x01, v);
  stp_putc(0x01, v); /* Actually, # of copies */
  stp_putc(0x00, v);
  stp_put16_le(privdata.w_size, v);
  stp_put16_le(privdata.h_size, v);
  stp_put16_le(privdata.w_size, v);
  stp_put16_le(privdata.h_size, v);
  dyesub_nputc(v, 0, 4);
  stp_zfwrite((privdata.laminate->seq).data, 1,
	      (privdata.laminate->seq).bytes, v);
  dyesub_nputc(v, 0, 2);
}

/* Kodak Professional 8500 */
static const dyesub_pagesize_t kodak_8500_page[] =
{
  { "w612h864", "8.5 x 12", PT(2508,314)+1, PT(3134,314)+1, 0, 0, 0, 0, DYESUB_PORTRAIT}, /* 8.5x12 & A4 */
  { "Letter", "8.5 x 11", PT(2508,314)+1, PT(2954,314)+1, 0, 0, 0, 0, DYESUB_PORTRAIT}, /* Letter */
  { "Custom", NULL,   PT(2508,314), PT(3134,314)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, kodak_8500_page_list, dyesub_pagesize_t, kodak_8500_page);

static const dyesub_printsize_t kodak_8500_printsize[] =
{
  { "314x314", "w612h864", 2508, 3134},
  { "314x314", "Letter", 2508, 2954},
  { "314x314", "Custom", 2508, 3134},
};

LIST(dyesub_printsize_list_t, kodak_8500_printsize_list, dyesub_printsize_t, kodak_8500_printsize);

static const dyesub_media_t kodak_8500_media[] =
{
  { "Glossy", N_("Glossy"), {2, "\x00\x00"}},
  { "Matte+5",  N_("Matte +5"),  {2, "\x01\x05"}},
  { "Matte+4",  N_("Matte +4"),  {2, "\x01\x04"}},
  { "Matte+3",  N_("Matte +3"),  {2, "\x01\x03"}},
  { "Matte+2",  N_("Matte +2"),  {2, "\x01\x02"}},
  { "Matte+1",  N_("Matte +1"),  {2, "\x01\x01"}},
  { "Matte",    N_("Matte"),     {2, "\x01\x00"}},
  { "Matte-1",  N_("Matte -1"),  {2, "\x01\xff"}},
  { "Matte-2",  N_("Matte -2"),  {2, "\x01\xfe"}},
  { "Matte-3",  N_("Matte -3"),  {2, "\x01\xfd"}},
  { "Matte-4",  N_("Matte -4"),  {2, "\x01\xfc"}},
  { "Matte-5",  N_("Matte -5"),  {2, "\x01\xfb"}},
};
LIST(dyesub_media_list_t, kodak_8500_media_list, dyesub_media_t, kodak_8500_media);

static const laminate_t kodak_8500_laminate[] =
{
  {"Coated", N_("Coated"), {1, "\x00"}},
  {"None",  N_("None"),  {1, "\x02"}},
};

LIST(laminate_list_t, kodak_8500_laminate_list, laminate_t, kodak_8500_laminate);

static void kodak_8500_printer_init(stp_vars_t *v)
{
  /* Start with NULL block */
  dyesub_nputc(v, 0x00, 64);
  /* Number of copies */
  stp_putc(0x1b, v);
  stp_putc(0x4e, v);
  stp_putc(1, v); /* XXX always 1 for now, up to 50 */
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
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  dyesub_nputc(v, 0x00, 57);
  /* Sharpening -- XXX not exported. */
  stp_putc(0x1b, v);
  stp_putc(0x46, v);
  stp_putc(0x50, v);
  stp_putc(0, v);  /* 8-bit signed, range is +- 5.  IOW, 0xfb->0x5 */
  dyesub_nputc(v, 0x00, 60);
  /* Lamination */
  stp_putc(0x1b, v);
  stp_putc(0x59, v);
  if (*((const char*)((privdata.laminate->seq).data)) == 0x02) { /* None */
    stp_putc(0x02, v);
    stp_putc(0x00, v);
  } else {
    stp_zfwrite((const char*)((privdata.media->seq).data), 1, 
		(privdata.media->seq).bytes, v);
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
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v); /* Number of rows in this block */
  dyesub_nputc(v, 0x00, 53);
}

static void kodak_8500_printer_end(stp_vars_t *v)
{
  /* Pad data to 64-byte block */
  unsigned int length = privdata.w_size * privdata.h_size * 3;
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

/* Mitsubishi CP3020D/DU/DE */
static const dyesub_pagesize_t mitsu_cp3020d_page[] =
{
  { "A4", "A4", PT(2508,314)+1, PT(3134,314)+1, 0, 0, 0, 0, DYESUB_PORTRAIT}, /* A4 */
  { "Legal", "Letter Long", PT(2508,314)+1, PT(3762,314)+1, 0, 0, 0, 0, DYESUB_PORTRAIT}, /* Letter */
  { "Custom", NULL,   PT(2508,314), PT(3134,314)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, mitsu_cp3020d_page_list, dyesub_pagesize_t, mitsu_cp3020d_page);

static const dyesub_printsize_t mitsu_cp3020d_printsize[] =
{
  { "314x314", "A4", 2508, 3134},
  { "314x314", "Legal", 2508, 3762},
  { "314x314", "Custom", 2508, 3134},
};

LIST(dyesub_printsize_list_t, mitsu_cp3020d_printsize_list, dyesub_printsize_t, mitsu_cp3020d_printsize);

static void mitsu_cp3020d_printer_init(stp_vars_t *v)
{
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
  if (privdata.h_size == 3762)
    stp_putc(0x04, v);
  else
    stp_putc(0x00, v);
  dyesub_nputc(v, 0x00, 60);
  /* Number of copies */
  stp_putc(0x1b, v);
  stp_putc(0x4e, v);
  stp_putc(1, v); /* XXX always 1 for now, up to 50 */
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
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
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
  /* Plane data header */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x30 + 4 - privdata.plane, v); /* Y = x31, M = x32, C = x33 */
  dyesub_nputc(v, 0x00, 2);
  stp_put16_be(0, v); /* Starting row for this block */
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v); /* Number of rows in this block */
  dyesub_nputc(v, 0x00, 53);
}

static void mitsu_cp3020d_plane_end(stp_vars_t *v)
{
  /* Pad data to 64-byte block */
  unsigned int length = privdata.w_size * privdata.h_size;
  length %= 64;
  if (length) {
    length = 64 - length;
    dyesub_nputc(v, 0x00, length);
  }
}

/* Mitsubishi CP3020DA/DAE */
static void mitsu_cp3020da_printer_init(stp_vars_t *v)
{
  /* Init */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x20, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x0a, v);
  stp_putc(0x10, v);
  dyesub_nputc(v, 0x00, 7);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
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
  stp_putc(0x01, v);  /* Copies -- 01-50d */
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
  /* Plane data header */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x54, v);
  stp_putc((privdata.bpp > 8) ? 0x10: 0x00, v);
  dyesub_nputc(v, 0x00, 2);
  stp_put16_be(0, v); /* Starting row for this block */
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v); /* Number of rows in this block */
}

/* Mitsubishi 9550D/DW */
static const dyesub_resolution_t res_346dpi[] =
{
  { "346x346", 346, 346},
};

LIST(dyesub_resolution_list_t, res_346dpi_list, dyesub_resolution_t, res_346dpi);

static const dyesub_pagesize_t mitsu_cp9550_page[] =
{
  { "B7", "3.5x5", PT(1240,346)+1, PT(1812,346)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
  { "w288h432", "4x6", PT(1416,346)+1, PT(2152,346)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
#ifdef DNPX2
  { "2x6_x2", "2x6*2", PT(1416,346)+1, PT(2152,346)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
#endif
  { "w360h504", "5x7", PT(1812,346)+1, PT(2402,346)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h576", "6x8", PT(2152,346)+1, PT(2792,346)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h612", "6x8.5", PT(2152,346)+1, PT(2956,346)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h648", "6x9", PT(2152,346)+1, PT(3146,346)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "Custom", NULL, PT(1416,346)+1, PT(2152,346)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, mitsu_cp9550_page_list, dyesub_pagesize_t, mitsu_cp9550_page);

static const dyesub_printsize_t mitsu_cp9550_printsize[] =
{
  { "346x346", "B7", 1240, 1812},
  { "346x346", "w288h432", 1416, 2152},
#ifdef DNPX2
  { "346x346", "2x6_x2", 1416, 2152},
#endif
  { "346x346", "w360h504", 1812, 2402},
  { "346x346", "w432h576", 2152, 2792},
  { "346x346", "w432h612", 2152, 2956},
  { "346x346", "w432h648", 2152, 3146},
  { "346x346", "Custom", 1416, 2152},
};

LIST(dyesub_printsize_list_t, mitsu_cp9550_printsize_list, dyesub_printsize_t, mitsu_cp9550_printsize);

static void mitsu_cp9550_printer_init(stp_vars_t *v)
{
  /* Init */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x20, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x0a, v);
  stp_putc(0x10, v);
  dyesub_nputc(v, 0x00, 7);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
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
  dyesub_nputc(v, 0x00, 19);
  stp_putc(0x01, v);  /* This is Copies on other models.. */
  dyesub_nputc(v, 0x00, 2);
  if (strcmp(privdata.pagesize,"2x6_x2") == 0)
    stp_putc(0x83, v);
  else
    stp_putc(0x00, v);
  dyesub_nputc(v, 0x00, 5);
  stp_putc(0x00, v); /* XXX 00 == normal, 80 = fine */
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
  stp_putc(0x00, v);  /* XXX 00 == normal, 01 == finedeep (some models only) */
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

/* Mitsubishi 9810D/DW */
static const dyesub_pagesize_t mitsu_cp9810_page[] =
{
  { "B7", "3.5x5", PT(1076,300)+1, PT(1572,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
  { "w288h432", "4x6", PT(1228,300)+1, PT(1868,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
  { "w360h504", "5x7", PT(1572,300)+1, PT(2128,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h576", "6x8", PT(1868,300)+1, PT(2442,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h612", "6x8.5", PT(1868,300)+1, PT(2564,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h648", "6x9", PT(1868,300)+1, PT(2730,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "Custom", NULL, PT(1220,300)+1, PT(1868,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
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
  { "300x300", "Custom", 1220, 1868},
};

LIST(dyesub_printsize_list_t, mitsu_cp9810_printsize_list, dyesub_printsize_t, mitsu_cp9810_printsize);

static const laminate_t mitsu_cp9810_laminate[] =
{
  {"Matte", N_("Matte"), {1, "\x01"}},
  {"None",  N_("None"),  {1, "\x00"}},
};

LIST(laminate_list_t, mitsu_cp9810_laminate_list, laminate_t, mitsu_cp9810_laminate);

static void mitsu_cp9810_printer_init(stp_vars_t *v)
{
  /* Init */
  stp_putc(0x1b, v);
  stp_putc(0x57, v);
  stp_putc(0x20, v);
  stp_putc(0x2e, v);
  stp_putc(0x00, v);
  stp_putc(0x0a, v);
  stp_putc(0x90, v);
  dyesub_nputc(v, 0x00, 7);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  stp_zfwrite((privdata.laminate->seq).data, 1,
	      (privdata.laminate->seq).bytes, v); /* Lamination */
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
  dyesub_nputc(v, 0x00, 19);
  stp_putc(0x01, v); /* Copies */
  dyesub_nputc(v, 0x00, 8);
  stp_putc(0x80, v); /* XXX 10 == Fine, 80 = SuperFine (required for lamination) */
  dyesub_nputc(v, 0x00, 10);
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
}

static void mitsu_cp9810_printer_end(stp_vars_t *v)
{
  /* Job Footer */
  stp_putc(0x1b, v);
  stp_putc(0x50, v);
  stp_putc(0x4c, v);
  stp_putc(0x00, v);

  if (*((const char*)((privdata.laminate->seq).data)) == 0x01) {

    /* Generate a full plane of lamination data */

    /* The Windows drivers generate a lamination pattern consisting of
       four values: 0x0202, 0x01f1, 0x0808, 0x0737 in roughly a 16:10:4:1
       ratio. 

       There seem to be some patterns but more analysis is needed.
    */

    int r, c;
    unsigned long seed = 1;

    mitsu_cp3020da_plane_init(v); /* First generate plane header */

    /* Now generate lamination pattern */
    for (c = 0 ; c < privdata.w_size ; c++) {
      for (r = 0 ; r < privdata.h_size ; r++) {
	int i = xrand(&seed) & 0x1f;
	if (i < 16)
	  stp_put16_be(0x0202, v);
	else if (i < 26)
	  stp_put16_be(0x01f1, v);
	else if (i < 30)
	  stp_put16_be(0x0808, v);
	else
	  stp_put16_be(0x0737, v);
      }
    }

    /* Lamination Footer */
    stp_putc(0x1b, v);
    stp_putc(0x50, v);
    stp_putc(0x56, v);
    stp_putc(0x00, v);
  }
}

/* Mitsubishi CP-D70D/CP-D707 */
static const dyesub_pagesize_t mitsu_cpd70x_page[] =
{
  { "B7", "3.5x5", PT(1076,300)+1, PT(1568,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
  { "w288h432", "4x6", PT(1228,300)+1, PT(1864,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
#ifdef DNPX2
  { "2x6_x2", "4x6*2", PT(1228,300)+1, PT(1864,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
#endif
  { "w360h504", "5x7", PT(1568,300)+1, PT(2128,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h432", "6x6", PT(1820,300)+1, PT(1864,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
  { "w432h576", "6x8", PT(1864,300)+1, PT(2422,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h612", "6x8.5", PT(1864,300)+1, PT(2564,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h648", "6x9", PT(1864,300)+1, PT(2730,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
#ifdef DNPX2
  { "4x6_x2", "4x6*2", PT(1864,300)+1, PT(2730,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
#endif
};

LIST(dyesub_pagesize_list_t, mitsu_cpd70x_page_list, dyesub_pagesize_t, mitsu_cpd70x_page);

static const dyesub_printsize_t mitsu_cpd70x_printsize[] =
{
  { "300x300", "B7", 1076, 1568},
  { "300x300", "w288h432", 1228, 1864},
#ifdef DNPX2
  { "300x300", "2x6_x2", 1228, 1864},
#endif
  { "300x300", "w360h504", 1568, 2128},
  { "300x300", "w432h432", 1820, 1864},
  { "300x300", "w432h576", 1864, 2422},
  { "300x300", "w432h612", 1864, 2564},
  { "300x300", "w432h648", 1864, 2730},
#ifdef DNPX2
  { "300x300", "4x6_x2", 1864, 2730},
#endif
};

LIST(dyesub_printsize_list_t, mitsu_cpd70x_printsize_list, dyesub_printsize_t, mitsu_cpd70x_printsize);

static const laminate_t mitsu_cpd70x_laminate[] =
{
  {"Glossy", N_("Glossy"), {1, "\x00"}},
  {"Matte", N_("Matte"), {1, "\x02"}},
};

LIST(laminate_list_t, mitsu_cpd70x_laminate_list, laminate_t, mitsu_cpd70x_laminate);

static void mitsu_cpd70k60_printer_init(stp_vars_t *v, int is_k60, int is_305)
{
  /* Printer init */
  stp_putc(0x1b, v);
  stp_putc(0x45, v);
  stp_putc(0x57, v);
  stp_putc(0x55, v);
  dyesub_nputc(v, 0x00, 508);

  /* Each copy gets this.. */
  stp_putc(0x1b, v);
  stp_putc(0x5a, v);
  stp_putc(0x54, v);
  if (is_k60) {
    stp_putc(0x00, v);
  } else if (is_305) {
    stp_putc(0x90, v);
  } else {
    stp_putc(0x01, v);
  }
  dyesub_nputc(v, 0x00, 12);

  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  if (*((const char*)((privdata.laminate->seq).data)) != 0x00) {
    /* Laminate a slightly larger boundary */
    stp_put16_be(privdata.w_size, v);
    stp_put16_be(privdata.h_size + 12, v);
    if (is_k60) {
      stp_putc(0x04, v); /* Matte Lamination forces UltraFine */
    } else {
      stp_putc(0x03, v); /* Matte Lamination forces Superfine (or UltraFine) */
    }
  } else {
    dyesub_nputc(v, 0x00, 4);  /* Ie no Lamination */
    stp_putc(0x00, v); /* Fine mode */
  }
  dyesub_nputc(v, 0x00, 7);

  if (is_k60 || is_305) {
    stp_putc(0x01, v); /* K60 has a single "lower" deck */
  } else {
    stp_putc(0x00, v);  /* Auto deck selection, or 0x01 for Lower, 0x02 for Upper */
  }
  dyesub_nputc(v, 0x00, 8);

  stp_zfwrite((privdata.laminate->seq).data, 1,
	      (privdata.laminate->seq).bytes, v); /* Lamination mode */
  dyesub_nputc(v, 0x00, 6);

  /* Multi-cut control */
  if (is_305) {
	  if (strcmp(privdata.pagesize,"w288h432") == 0) {
		  stp_putc(0x01, v);
	  } else {
		  stp_putc(0x00, v);
	  }
  } else {
	  if (strcmp(privdata.pagesize,"4x6_x2") == 0) {
		  stp_putc(0x01, v);
	  } else if (strcmp(privdata.pagesize,"B7_x2") == 0) {
		  stp_putc(0x01, v);
	  } else if (strcmp(privdata.pagesize,"2x6_x2") == 0) {
		  stp_putc(0x05, v);
	  } else {
		  stp_putc(0x00, v);
	  }
  }
  dyesub_nputc(v, 0x00, 15);

  dyesub_nputc(v, 0x00, 448); /* Pad to 512-byte block */
}

static void mitsu_cpd70x_printer_init(stp_vars_t *v)
{
	mitsu_cpd70k60_printer_init(v, 0, 0);
}

static void mitsu_cpd70x_printer_end(stp_vars_t *v)
{
  /* If Matte lamination is enabled, generate a lamination plane */
  if (*((const char*)((privdata.laminate->seq).data)) != 0x00) {

    /* The Windows drivers generate a lamination pattern consisting of
       three values: 0xe84b, 0x286a, 0x6c22 */

    int r, c;
    unsigned long seed = 1;

    /* Now generate lamination pattern */
    for (c = 0 ; c < privdata.w_size ; c++) {
      for (r = 0 ; r < privdata.h_size + 12 ; r++) {
	int i = xrand(&seed) & 0x3f;
	if (i < 42)
	  stp_put16_be(0xe84b, v);
	else if (i < 62)
	  stp_put16_be(0x286a, v);
	else
	  stp_put16_be(0x6c22, v);
      }
    }
    /* Pad up to a 512-byte block */
    dyesub_nputc(v, 0x00, 512 - ((privdata.w_size * (privdata.h_size + 12) * 2) % 512));
  }
}

static void mitsu_cpk60_printer_end(stp_vars_t *v)
{
  /* If Matte lamination is enabled, generate a lamination plane */
  if (*((const char*)((privdata.laminate->seq).data)) != 0x00) {

    /* The Windows drivers generate a lamination pattern consisting of
       three values: 0x9d00, 0x6500, 0x2900 */

    int r, c;
    unsigned long seed = 1;

    /* Now generate lamination pattern */
    for (c = 0 ; c < privdata.w_size ; c++) {
      for (r = 0 ; r < privdata.h_size + 12 ; r++) {
	int i = xrand(&seed) & 0x3f;
	if (i < 42)
	  stp_put16_be(0x9d00, v);
	else if (i < 62)
	  stp_put16_be(0x2900, v);
	else
	  stp_put16_be(0x6500, v);
      }
    }
    /* Pad up to a 512-byte block */
    dyesub_nputc(v, 0x00, 512 - ((privdata.w_size * (privdata.h_size + 12) * 2) % 512));
  }
}


static void mitsu_cpd70x_plane_end(stp_vars_t *v)
{
  /* Pad up to a 512-byte block */
  dyesub_nputc(v, 0x00, 512 - ((privdata.h_size * privdata.w_size * 2) % 512));
}

/* Mitsubishi CP-K60D */
static const dyesub_pagesize_t mitsu_cpk60_page[] =
{
  { "B7", "3.5x5", PT(1076,300)+1, PT(1568,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
  { "w288h432", "4x6", PT(1218,300)+1, PT(1864,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
#ifdef DNPX2
  { "2x6_x2", "2x6*2", PT(1218,300)+1, PT(1864,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
#endif
  { "w360h504", "5x7", PT(1568,300)+1, PT(2128,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
#ifdef DNPX2
  { "B7_x2", "3.5x5*2", PT(1568,300)+1, PT(2190,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
#endif
  { "w432h432", "6x6", PT(1864,300)+1, PT(1820,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h576", "6x8", PT(1864,300)+1, PT(2422,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
#ifdef DNPX2
  { "4x6_x2", "4x6*2", PT(1864,300)+1, PT(2454,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
#endif
};

LIST(dyesub_pagesize_list_t, mitsu_cpk60_page_list, dyesub_pagesize_t, mitsu_cpk60_page);

static const dyesub_printsize_t mitsu_cpk60_printsize[] =
{
  { "300x300", "B7", 1076, 1568},
  { "300x300", "w288h432", 1218, 1864},
#ifdef DNPX2
  { "300x300", "2x6_x2", 1218, 1864},
#endif
  { "300x300", "w360h504", 1568, 2128},
#ifdef DNPX2
  { "B7_x2", "3.5x5*24", 1568, 2190},
#endif
  { "300x300", "w432h432", 1864, 1820},
  { "300x300", "w432h576", 1864, 2422},
#ifdef DNPX2
  { "300x300", "4x6_x2", 1864, 2454},
#endif
};

LIST(dyesub_printsize_list_t, mitsu_cpk60_printsize_list, dyesub_printsize_t, mitsu_cpk60_printsize);

static void mitsu_cpk60_printer_init(stp_vars_t *v)
{
  mitsu_cpd70k60_printer_init(v, 1, 0);
}

static const dyesub_pagesize_t mitsu_cpd80_page[] =
{
  { "w288h432", "4x6", PT(1228,300)+1, PT(1864,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
#ifdef DNPX2
  { "2x6_x2", "2x6*2", PT(1228,300)+1, PT(1864,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
#endif
  { "w360h360", "5x5", PT(1524,300)+1, PT(1568,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
  { "w360h504", "5x7", PT(1568,300)+1, PT(2128,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h432", "6x6", PT(1864,300)+1, PT(1820,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "w432h576", "6x8", PT(1864,300)+1, PT(2422,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
#ifdef DNPX2
  { "4x6_x2", "4x6*2", PT(1864,300)+1, PT(2730,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
#endif
};

LIST(dyesub_pagesize_list_t, mitsu_cpd80_page_list, dyesub_pagesize_t, mitsu_cpd80_page);

static const dyesub_printsize_t mitsu_cpd80_printsize[] =
{
  { "300x300", "w288h432", 1228, 1864},
#ifdef DNPX2
  { "300x300", "2x6_x2", 1228, 1864},
#endif
  { "300x300", "w360h360", 1524, 1568},
  { "300x300", "w360h504", 1568, 2128},
  { "300x300", "w432h432", 1864, 1820},
  { "300x300", "w432h576", 1864, 2422},
#ifdef DNPX2
  { "300x300", "4x6_x2", 1864, 2730},
#endif
};

LIST(dyesub_printsize_list_t, mitsu_cpd80_printsize_list, dyesub_printsize_t, mitsu_cpd80_printsize);


/* Kodak 305 */
static const dyesub_pagesize_t kodak305_page[] =
{
  { "w288h432", "4x6", PT(1218,300)+1, PT(1864,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
  { "w432h576", "6x8", PT(1864,300)+1, PT(2422,300)+1, 0, 0, 0, 0,
  						DYESUB_PORTRAIT},
  { "Custom", NULL, PT(1218,300)+1, PT(1864,300)+1, 0, 0, 0, 0,
  						DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, kodak305_page_list, dyesub_pagesize_t, kodak305_page);

static const dyesub_printsize_t kodak305_printsize[] =
{
  { "300x300", "w288h432", 1218, 1864},
  { "300x300", "w432h576", 1864, 2422},
  { "300x300", "Custom", 1218, 1864},
};

LIST(dyesub_printsize_list_t, kodak305_printsize_list, dyesub_printsize_t, kodak305_printsize);

static void kodak305_printer_init(stp_vars_t *v)
{
	mitsu_cpd70k60_printer_init(v, 0, 1);
}

/* Shinko CHC-S9045 (experimental) */
static const dyesub_pagesize_t shinko_chcs9045_page[] =
{
  { "w288h432",	"4x6", PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "B7",	"3.5x5", PT(1088,300)+1, PT(1548,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "w360h504",	"5x7", PT(1548,300)+1, PT(2140,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
  { "w432h576",	"6x9", PT(1844,300)+1, PT(2740,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
  { "w283h425",	"Sticker paper", PT(1092,300)+1, PT(1726,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "Custom",   NULL, PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, shinko_chcs9045_page_list, dyesub_pagesize_t, shinko_chcs9045_page);

static const dyesub_printsize_t shinko_chcs9045_printsize[] =
{
  { "300x300", "w288h432", 1240, 1844},
  { "300x300", "B7", 1088, 1548},
  { "300x300", "w360h504", 1548, 2140},
  { "300x300", "w432h576", 1844, 2740},
  { "300x300", "w283h425", 1092, 1726},
  { "300x300", "Custom", 1240, 1844},
};

LIST(dyesub_printsize_list_t, shinko_chcs9045_printsize_list, dyesub_printsize_t, shinko_chcs9045_printsize);

static void shinko_chcs9045_printer_init(stp_vars_t *v)
{
  char pg = '\0';
  char sticker = '\0';

  stp_zprintf(v, "\033CHC\n");
  stp_put16_be(1, v);
  stp_put16_be(1, v);
  stp_put16_be(privdata.w_size, v);
  stp_put16_be(privdata.h_size, v);
  if (strcmp(privdata.pagesize,"B7") == 0)
    pg = '\1';
  else if (strcmp(privdata.pagesize,"w360h504") == 0)
    pg = '\3';
  else if (strcmp(privdata.pagesize,"w432h576") == 0)
    pg = '\5';
  else if (strcmp(privdata.pagesize,"w283h425") == 0)
    sticker = '\3';
  stp_putc(pg, v);
  stp_putc('\0', v);
  stp_putc(sticker, v);
  dyesub_nputc(v, '\0', 4338);
}

/* Shinko CHC-S2145 */
static const dyesub_pagesize_t shinko_chcs2145_page[] =
{
  { "w144h432",	"2x6", PT(634,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "w288h432",	"4x6", PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
#ifdef DNPX2
  { "2x6_x2",	"2x6*2", PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
#endif
  { "B7",	"3.5x5", PT(1088,300)+1, PT(1548,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "w360h504",	"5x7", PT(1548,300)+1, PT(2140,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
  { "w432h576",	"6x8", PT(1844,300)+1, PT(2434,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
#ifdef DNPX2
  { "4x6_x2",	"4x6*2", PT(1844,300)+1, PT(2492,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
#endif
  { "w432h648",	"6x9", PT(1844,300)+1, PT(2740,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
  { "Custom",   NULL, PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
};

LIST(dyesub_pagesize_list_t, shinko_chcs2145_page_list, dyesub_pagesize_t, shinko_chcs2145_page);

static const dyesub_printsize_t shinko_chcs2145_printsize[] =
{
  { "300x300", "w144h432", 634, 1844},
  { "300x300", "w288h432", 1240, 1844},
#ifdef DNPX2
  { "300x300", "2x6_x2", 1240, 1844},
#endif
  { "300x300", "B7", 1088, 1548},
  { "300x300", "w360h504", 1548, 2140},
  { "300x300", "w432h576", 1844, 2434},
#ifdef DNPX2
  { "300x300", "4x6_x2", 1844, 2492},
#endif
  { "300x300", "w432h648", 1844, 2740},
  { "300x300", "Custom", 1240, 1844},
};

LIST(dyesub_printsize_list_t, shinko_chcs2145_printsize_list, dyesub_printsize_t, shinko_chcs2145_printsize);

static const laminate_t shinko_chcs2145_laminate[] =
{
  {"PrinterDefault",  N_("Printer Default"),  {4, "\x01\0\0\0"}},
  {"Glossy",  N_("Glossy"),  {4, "\x02\0\0\0"}},
  {"GlossyFine",  N_("Glossy Fine"),  {4, "\x03\0\0\0"}},
  {"Matte",  N_("Matte"),  {4, "\x04\0\0\0"}},
  {"MatteFine",  N_("Matte Fine"),  {4, "\x05\0\0\0"}},
  {"ExtraGlossy",  N_("Extra Glossy"),  {4, "\x06\0\0\0"}},
  {"ExtraGlossyFine",  N_("Extra Glossy Fine"),  {4, "\x07\0\0\0"}},
};

LIST(laminate_list_t, shinko_chcs2145_laminate_list, laminate_t, shinko_chcs2145_laminate);

static void shinko_chcs2145_printer_init(stp_vars_t *v)
{
  int media = 0;

  if (strcmp(privdata.pagesize,"w288h432") == 0)
    media = '\0';
  else if (strcmp(privdata.pagesize,"2x6_x2") == 0)
    media = '\0';
  else if (strcmp(privdata.pagesize,"B7") == 0)
    media = '\1';
  else if (strcmp(privdata.pagesize,"w360h504") == 0)
    media = '\3';
  else if (strcmp(privdata.pagesize,"w432h576") == 0)
    media = '\6';
  else if (strcmp(privdata.pagesize,"w432h648") == 0)
    media = '\5';
  else if (strcmp(privdata.pagesize,"4x6_x2") == 0)
    media = '\5';
  else if (strcmp(privdata.pagesize,"w144h432") == 0)
    media = '\7';

  stp_put32_le(0x10, v);
  stp_put32_le(2145, v);  /* Printer Model */
  stp_put32_le(0x00, v);
  stp_put32_le(0x01, v);

  stp_put32_le(0x64, v);
  stp_put32_le(0x00, v);
  stp_put32_le(media, v);  /* Media Type */
  stp_put32_le(0x00, v);

  if (strcmp(privdata.pagesize,"4x6_x2") == 0) {
    stp_put32_le(0x02, v);
  } else if (strcmp(privdata.pagesize,"2x6_x2") == 0) {
    stp_put32_le(0x04, v);
  } else {
    stp_put32_le(0x00, v);  /* Print Method */
  }

  stp_zfwrite((privdata.laminate->seq).data, 1,
	      (privdata.laminate->seq).bytes, v); /* Print Mode */
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
  stp_put32_le(privdata.w_size, v); /* Columns */
  stp_put32_le(privdata.h_size, v); /* Rows */
  stp_put32_le(0x01, v);            /* Copies */

  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(privdata.w_dpi, v);  /* Dots Per Inch */
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
  { "w288h576", "8x4", PT(1236,300)+1, PT(2446,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w360h576", "8x5", PT(1536,300)+1, PT(2446,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w432h576", "8x6", PT(1836,300)+1, PT(2446,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w576h576", "8x8", PT(2436,300)+1, PT(2446,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "c8x10", "8x10", PT(2446,300)+1, PT(3036,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "w576h864", "8x12", PT(2446,300)+1, PT(3636,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, shinko_chcs1245_page_list, dyesub_pagesize_t, shinko_chcs1245_page);

static const dyesub_printsize_t shinko_chcs1245_printsize[] =
{
  { "300x300", "w288h576", 1236, 2446},
  { "300x300", "w360h576", 1536, 2446},
  { "300x300", "w432h576", 1836, 2446},
  { "300x300", "w576h576", 2436, 2446},
  { "300x300", "c8x10", 2446, 3036},
  { "300x300", "w576h864", 2446, 3636},
};

LIST(dyesub_printsize_list_t, shinko_chcs1245_printsize_list, dyesub_printsize_t, shinko_chcs1245_printsize);

static void shinko_chcs1245_printer_init(stp_vars_t *v)
{
  int media = 0;

  if (strcmp(privdata.pagesize,"w288h576") == 0)
    media = 5;
  else if (strcmp(privdata.pagesize,"w360h576") == 0)
    media = 4;
  else if (strcmp(privdata.pagesize,"w432h576") == 0)
    media = 6;
  else if (strcmp(privdata.pagesize,"w576h576") == 0)
    media = 9;
  else if (strcmp(privdata.pagesize,"c8x10") == 0)
    media = 0;
  else if (strcmp(privdata.pagesize,"w576h864") == 0)
    media = 0;

  stp_put32_le(0x10, v);
  stp_put32_le(1245, v);  /* Printer Model */
  stp_put32_le(0x00, v);
  stp_put32_le(0x01, v);

  stp_put32_le(0x64, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x10, v);  /* Seems to be fixed */
  stp_put32_le(0x00, v);

  stp_put32_le(media, v);
  stp_put32_le(0x01, v);  // XXX 0x01 printer default, 0x05 matte, 0x03 glossy, 
  stp_put32_le(0x00, v);
  stp_put32_le(0x07fffffff, v);  // XXX glossy. if non-matte, 0x05 and 0x0000000 signed, +-25.

  stp_put32_le(0x00, v); // XXX 0x00 printer default, 0x02 for "dust removal" on, 0x01 for off.
  stp_put32_le(privdata.w_size, v); /* Columns */
  stp_put32_le(privdata.h_size, v); /* Rows */
  stp_put32_le(0x01, v);            /* Copies */

  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(privdata.w_dpi, v);  /* Dots Per Inch */
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
  { "w288h576", "8x4", PT(1236,300)+1, PT(2464,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w360h576", "8x5", PT(1536,300)+1, PT(2464,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w432h576", "8x6", PT(1836,300)+1, PT(2464,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "w576h576", "8x8", PT(2436,300)+1, PT(2464,300)+1, 0, 0, 0, 0, DYESUB_LANDSCAPE},
  { "c8x10", "8x10", PT(2464,300)+1, PT(3036,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
  { "w576h864", "8x12", PT(2464,300)+1, PT(3636,300)+1, 0, 0, 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, shinko_chcs6245_page_list, dyesub_pagesize_t, shinko_chcs6245_page);

static const dyesub_printsize_t shinko_chcs6245_printsize[] =
{
  { "300x300", "w288h576", 1236, 2464},
  { "300x300", "w360h576", 1536, 2464},
  { "300x300", "w432h576", 1836, 2464},
  { "300x300", "w576h576", 2436, 2464},
  { "300x300", "c8x10", 2464, 3036},
  { "300x300", "w576h864", 2464, 3636},
};

LIST(dyesub_printsize_list_t, shinko_chcs6245_printsize_list, dyesub_printsize_t, shinko_chcs6245_printsize);

static const laminate_t shinko_chcs6245_laminate[] =
{
  {"Glossy", N_("Glossy"), {4, "\x03\x00\x00\x00"}},
  {"Matte", N_("Matte"), {4, "\x02\x00\x00\x00"}},
  {"None", N_("None"), {4, "\x01\x00\x00\x00"}},
};

LIST(laminate_list_t, shinko_chcs6245_laminate_list, laminate_t, shinko_chcs6245_laminate);

static void shinko_chcs6245_printer_init(stp_vars_t *v)
{
  int media = 0;

  if (strcmp(privdata.pagesize,"w288h576") == 0)
    media = 0x20;
  else if (strcmp(privdata.pagesize,"w360h576") == 0)
    media = 0x21;
  else if (strcmp(privdata.pagesize,"w432h576") == 0)
    media = 0x22;
  else if (strcmp(privdata.pagesize,"w576h576") == 0)
    media = 0x23;
  else if (strcmp(privdata.pagesize,"c8x10") == 0)
    media = 0x10;
  else if (strcmp(privdata.pagesize,"w576h864") == 0)
    media = 0x11;

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
  stp_zfwrite((privdata.laminate->seq).data, 1,
	      (privdata.laminate->seq).bytes, v); /* Lamination */
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
  stp_put32_le(privdata.w_size, v); /* Columns */
  stp_put32_le(privdata.h_size, v); /* Rows */
  stp_put32_le(0x01, v);            /* Copies */

  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(privdata.w_dpi, v);  /* Dots Per Inch */
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
  { "w144h432",	"2x6", PT(634,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "w288h432",	"4x6", PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
#ifdef DNPX2
  { "2x6_x2",	"2x6*2", PT(1240,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
#endif
  { "w360h360",	"5x5", PT(1536,300)+1, PT(1548,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "w360h504",	"5x7", PT(1548,300)+1, PT(2140,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
  { "w432h432",	"6x6", PT(1832,300)+1, PT(1844,300)+1, 0, 0, 0, 0,
  							DYESUB_LANDSCAPE},
  { "w432h576",	"6x8", PT(1844,300)+1, PT(2434,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
#ifdef DNPX2
  { "4x6_2x6",	"4x6+2x6", PT(1844,300)+1, PT(2434,300)+1, 0, 0, 0, 0,
  							DYESUB_PORTRAIT},
#endif
};

LIST(dyesub_pagesize_list_t, shinko_chcs6145_page_list, dyesub_pagesize_t, shinko_chcs6145_page);

static const dyesub_printsize_t shinko_chcs6145_printsize[] =
{
  { "300x300", "w144h432", 634, 1844},
  { "300x300", "w288h432", 1240, 1844},
#ifdef DNPX2
  { "300x300", "2x6_x2", 1240, 1844},
#endif
  { "300x300", "w360h360", 1536, 1548},
  { "300x300", "w360h504", 1548, 2140},
  { "300x300", "w432h432", 1832, 1844},
  { "300x300", "w432h576", 1844, 2434},
#ifdef DNPX2
  { "300x300", "4x6_2x6", 1844, 2434},
#endif
};

LIST(dyesub_printsize_list_t, shinko_chcs6145_printsize_list, dyesub_printsize_t, shinko_chcs6145_printsize);

static const laminate_t shinko_chcs6145_laminate[] =
{
  {"PrinterDefault",  N_("Printer Default"),  {4, "\x01\0\0\0"}},
  {"Glossy",  N_("Glossy"),  {4, "\x02\0\0\0"}},
  {"Matte",  N_("Matte"),  {4, "\x03\0\0\0"}},
};

LIST(laminate_list_t, shinko_chcs6145_laminate_list, laminate_t, shinko_chcs6145_laminate);

static void shinko_chcs6145_printer_init(stp_vars_t *v)
{
  int media = 0;

  if (strcmp(privdata.pagesize,"w288h432") == 0)
    media = 0x00;
  else if (strcmp(privdata.pagesize,"2x6_x2") == 0)
    media = 0x00;
  else if (strcmp(privdata.pagesize,"w360h360") == 0)
    media = 0x08;
  else if (strcmp(privdata.pagesize,"w360h504") == 0)
    media = 0x03;
  else if (strcmp(privdata.pagesize,"w432h432") == 0)
    media = 0x06;
  else if (strcmp(privdata.pagesize,"w432h576") == 0)
    media = 0x06;
  else if (strcmp(privdata.pagesize,"w144h432") == 0)
    media = 0x07;
  else if (strcmp(privdata.pagesize,"w4x6_2x6") == 0)
    media = 0x06;

  stp_put32_le(0x10, v);
  stp_put32_le(6145, v);  /* Printer Model */
  if (!strcmp(privdata.pagesize,"w360h360") ||
      !strcmp(privdata.pagesize,"w360h504"))
	  stp_put32_le(0x02, v); /* 5" media */
  else
	  stp_put32_le(0x03, v); /* 6" media */
  stp_put32_le(0x01, v);

  stp_put32_le(0x64, v);
  stp_put32_le(0x00, v);
  stp_put32_le(media, v);  /* Media Type */
  stp_put32_le(0x00, v);

  if (strcmp(privdata.pagesize,"6x6_2x6") == 0) {
    stp_put32_le(0x05, v);
  } else if (strcmp(privdata.pagesize,"2x6_x2") == 0) {
    stp_put32_le(0x04, v);
  } else {
    stp_put32_le(0x00, v);
  }
  stp_put32_le(0x00, v);  // XX quality; 00 == default, 0x01 == std
  stp_zfwrite((privdata.laminate->seq).data, 1,
	      (privdata.laminate->seq).bytes, v); /* Lamination */
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
  stp_put32_le(privdata.w_size, v); /* Columns */
  stp_put32_le(privdata.h_size, v); /* Rows */
  stp_put32_le(0x01, v);            /* Copies */

  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(privdata.w_dpi, v);  /* Dots Per Inch */
  stp_put32_le(0xffffffce, v);

  stp_put32_le(0x00, v);
  stp_put32_le(0xffffffce, v);
  stp_put32_le(0x00, v);
  stp_put32_le(0x00, v);

  stp_put32_le(0x00, v);
}


/* Dai Nippon Printing DS40 */
static const dyesub_resolution_t res_dnpds40_dpi[] =
{
  { "300x300", 300, 300},
  { "300x600", 300, 600},
};

LIST(dyesub_resolution_list_t, res_dnpds40_dpi_list, dyesub_resolution_t, res_dnpds40_dpi);

/* Imaging area is wider than print size, we always must supply the 
   printer with the full imaging width. */
static const dyesub_pagesize_t dnpds40_dock_page[] =
{
  { "B7", "3.5x5", PT(1088,300)+1, PT(1920,300)+1, 0, 0, PT(112,300), PT(112,300), DYESUB_LANDSCAPE},
  { "w288h432", "4x6", PT(1240,300)+1, PT(1920,300)+1, 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE},
#ifdef DNPX2
  { "2x6_x2", "2x6*2", PT(1240,300)+1, PT(1920,300)+1, 0, 0, PT(38,300), PT(38,300), DYESUB_LANDSCAPE},
#endif
  { "w360h504",	"5x7", PT(1920,300)+1, PT(2138,300)+1, PT(112,300), PT(112,300), 0, 0, DYESUB_PORTRAIT},
  { "A5", "6x8", PT(1920,300)+1, PT(2436,300)+1, PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT},
#ifdef DNPX2
  { "4x6_x2", "4x6*2", PT(1920,300)+1, PT(2498,300)+1, PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT},
#endif
  { "w432h576", "6x9", PT(1920,300)+1, PT(2740,300)+1, PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, dnpds40_dock_page_list, dyesub_pagesize_t, dnpds40_dock_page);

static const dyesub_printsize_t dnpds40_dock_printsize[] =
{
  { "300x300", "B7", 1088, 1920},
  { "300x600", "B7", 2176, 1920},
  { "300x300", "w288h432", 1240, 1920},
  { "300x600", "w288h432", 2480, 1920},
#ifdef DNPX2
  { "300x300", "2x6_x2", 1240, 1920},
  { "300x600", "2x6_x2", 2480, 1920},
#endif
  { "300x300", "w360h504", 1920, 2138},
  { "300x600", "w360h504", 1920, 4276},
  { "300x300", "A5", 1920, 2436},
  { "300x600", "A5", 1920, 4872},
#ifdef DNPX2
  { "300x300", "4x6_x2", 1920, 2498},
  { "300x600", "4x6_x2", 1920, 4996},
#endif
  { "300x300", "w432h576", 1920, 2740},
  { "300x600", "w432h576", 1920, 5480},
};

LIST(dyesub_printsize_list_t, dnpds40_dock_printsize_list, dyesub_printsize_t, dnpds40_dock_printsize);

static const laminate_t dnpds40_laminate[] =
{
  {"Glossy",  N_("Glossy"),  {2, "00"}},
  {"Matte", N_("Matte"), {2, "01"}},
};

LIST(laminate_list_t, dnpds40_laminate_list, laminate_t, dnpds40_laminate);


static void dnpds40ds80_printer_start(stp_vars_t *v)
{
  /* XXX Unknown purpose. */
  stp_zprintf(v, "\033PCNTRL RETENTION       0000000800000000");

  /* Configure Lamination */
  stp_zprintf(v, "\033PCNTRL OVERCOAT        00000008000000");
  stp_zfwrite((privdata.laminate->seq).data, 1,
	      (privdata.laminate->seq).bytes, v); /* Lamination mode */

  /* Don't resume after error.. XXX should be in backend */
  stp_zprintf(v, "\033PCNTRL BUFFCNTRL       0000000800000000");

  /* Set quantity.. Backend overrides as needed. */
  stp_zprintf(v, "\033PCNTRL QTY             000000080000001\r");

}

static void dnpds40_printer_start(stp_vars_t *v)
{
  /* Common code */
  dnpds40ds80_printer_start(v);

  /* Set cutter option to "normal" */
  stp_zprintf(v, "\033PCNTRL CUTTER          0000000800000");
  if (!strcmp(privdata.pagesize, "2x6_x2")) {
    stp_zprintf(v, "120");
  } else {
    stp_zprintf(v, "000");
  }

  /* Configure multi-cut/page size */
  stp_zprintf(v, "\033PIMAGE MULTICUT        00000008000000");

  if (!strcmp(privdata.pagesize, "B7")) {
    stp_zprintf(v, "01");
  } else if (!strcmp(privdata.pagesize, "w288h432")) {
    stp_zprintf(v, "02");
  } else if (!strcmp(privdata.pagesize, "w360h504")) {
    stp_zprintf(v, "03");
  } else if (!strcmp(privdata.pagesize, "A5")) {
    stp_zprintf(v, "04");
  } else if (!strcmp(privdata.pagesize, "w432h576")) {
    stp_zprintf(v, "05");
  } else if (!strcmp(privdata.pagesize, "4x6_x2")) {
    stp_zprintf(v, "12");
  } else {
    stp_zprintf(v, "00");
  }
}

static void dnpds40_printer_end(stp_vars_t *v)
{
  stp_zprintf(v, "\033PCNTRL START"); dyesub_nputc(v, ' ', 19);
}

static void dnpds40_plane_init(stp_vars_t *v)
{
  char p = (privdata.plane == 3 ? 'Y' :
	    (privdata.plane == 2 ? 'M' :
	     'C' ));

  long PadSize = 10;
  long FSize = (privdata.w_size*privdata.h_size) + 1024 + 54 + PadSize;

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
  stp_put32_le(privdata.w_size, v);
  stp_put32_le(privdata.h_size, v);
  stp_put16_le(1, v); /* single channel */
  stp_put16_le(8, v); /* 8bpp */
  dyesub_nputc(v, '\0', 8); /* compression + image size are ignored */
  stp_put32_le(11808, v); /* horizontal pixels per meter, fixed at 300dpi */
  if (privdata.h_dpi == 600)
    stp_put32_le(23615, v); /* vertical pixels per meter @ 600dpi */
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
static const dyesub_pagesize_t dnpds80_dock_page[] =
{
  { "w288h576", "8x4", PT(1236,300)+1, PT(2560,300)+1, 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE},
  { "w360h576", "8x5", PT(1536,300)+1, PT(2560,300)+1, 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE},
  { "w432h576", "8x6", PT(1836,300)+1, PT(2560,300)+1, 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE},
  { "w576h576", "8x8", PT(2436,300)+1, PT(2560,300)+1, 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE},
#ifdef DNPX2
  { "8x4_x2", "8x4*2", PT(2502,300)+1, PT(2560,300)+1, 0, 0, PT(56,300), PT(56,300), DYESUB_LANDSCAPE},
  { "8x5_8x4", "8x5+8x4", PT(2560,300)+1, PT(2802,300)+1, PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT},
#endif
  { "c8x10", "8x10", PT(2560,300)+1, PT(3036,300)+1, PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT},
#ifdef DNPX2
  { "8x5_x2", "8x5*2", PT(2560,300)+1, PT(3036,300)+1, PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT},
  { "8x6_8x4", "8x6+8x4", PT(2560,300)+1, PT(3036,300)+1, PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT},
  { "8x6_8x5", "8x6+8x5", PT(2560,300)+1, PT(3402,300)+1, PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT},
#endif
  { "A4", "A4 Length", PT(2560,300)+1, PT(3544,300)+1, PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT},
  { "w576h864", "8x12", PT(2560,300)+1, PT(3636,300)+1, PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT},
#ifdef DNPX2
  { "8x6_x2", "8x6*2", PT(2560,300)+1, PT(3702,300)+1, PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT},
  { "8x8_8x4", "8x8+8x4", PT(2560,300)+1, PT(3702,300)+1, PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT},
  { "8x4_x3", "8x4*3", PT(2560,300)+1, PT(3768,300)+1, PT(56,300), PT(56,300), 0, 0, DYESUB_PORTRAIT},
#endif
};

LIST(dyesub_pagesize_list_t, dnpds80_dock_page_list, dyesub_pagesize_t, dnpds80_dock_page);

static const dyesub_printsize_t dnpds80_dock_printsize[] =
{
  { "300x300", "w288h576", 1236, 2560},
  { "300x600", "w288h576", 2472, 2560},
  { "300x300", "w360h576", 1536, 2560},
  { "300x600", "w360h576", 3072, 2560},
  { "300x300", "w432h576", 1836, 2560},
  { "300x600", "w432h576", 3672, 2560},
  { "300x300", "w576h576", 2436, 2560},
  { "300x600", "w576h576", 4872, 2560},
#ifdef DNPX2
  { "300x300", "8x4_x2", 2502, 2560},
  { "300x600", "8x4_x2", 5004, 2560},
  { "300x300", "8x5_8x4", 2560, 2802},
  { "300x600", "8x5_8x4", 2560, 5604},
#endif
  { "300x300", "c8x10", 2560, 3036},
  { "300x600", "c8x10", 2560, 6072},
#ifdef DNPX2
  { "300x300", "8x5_x2", 2560, 3102},
  { "300x600", "8x5_x2", 2560, 6204},
  { "300x300", "8x6_8x4", 2560, 3102},
  { "300x600", "8x6_8x4", 2560, 6204},
  { "300x300", "8x6_8x5", 2560, 3402},
  { "300x600", "8x6_8x5", 2560, 6804},
#endif
  { "300x300", "A4", 2560, 3544},
  { "300x600", "A4", 2560, 7088},
  { "300x300", "w576h864", 2560, 3636},
  { "300x600", "w576h864", 2560, 7272},
#ifdef DNPX2
  { "300x300", "8x6_x2", 2560, 3702},
  { "300x600", "8x6_x2", 2560, 7404},
  { "300x300", "8x8_8x4", 2560, 3702},
  { "300x600", "8x8_8x4", 2560, 7404},
  { "300x300", "8x4_x3", 2560, 3768},
  { "300x600", "8x4_x3", 2560, 7536},
#endif
};

LIST(dyesub_printsize_list_t, dnpds80_dock_printsize_list, dyesub_printsize_t, dnpds80_dock_printsize);

static void dnpds80_printer_start(stp_vars_t *v)
{
  /* Common code */
  dnpds40ds80_printer_start(v);

  /* Set cutter option to "normal" */
  stp_zprintf(v, "\033PCNTRL CUTTER          0000000800000000");

  /* Configure multi-cut/page size */
  stp_zprintf(v, "\033PIMAGE MULTICUT        00000008000000");

  if (!strcmp(privdata.pagesize, "c8x10")) {
    stp_zprintf(v, "06");
  } else if (!strcmp(privdata.pagesize, "w576h864")) {
    stp_zprintf(v, "07");
  } else if (!strcmp(privdata.pagesize, "w288h576")) {
    stp_zprintf(v, "08");
  } else if (!strcmp(privdata.pagesize, "w360h576")) {
    stp_zprintf(v, "09");
  } else if (!strcmp(privdata.pagesize, "w432h576")) {
    stp_zprintf(v, "10");
  } else if (!strcmp(privdata.pagesize, "w576h576")) {
    stp_zprintf(v, "11");
  } else if (!strcmp(privdata.pagesize, "8x4_x2")) {
    stp_zprintf(v, "13");
  } else if (!strcmp(privdata.pagesize, "8x5_x2")) {
    stp_zprintf(v, "14");
  } else if (!strcmp(privdata.pagesize, "8x6_x2")) {
    stp_zprintf(v, "15");
  } else if (!strcmp(privdata.pagesize, "8x5_8x4")) {
    stp_zprintf(v, "16");
  } else if (!strcmp(privdata.pagesize, "8x6_8x4")) {
    stp_zprintf(v, "17");
  } else if (!strcmp(privdata.pagesize, "8x6_8x5")) {
    stp_zprintf(v, "18");
  } else if (!strcmp(privdata.pagesize, "8x8_8x4")) {
    stp_zprintf(v, "19");
  } else if (!strcmp(privdata.pagesize, "8x4_x3")) {
    stp_zprintf(v, "20");
  } else if (!strcmp(privdata.pagesize, "A4")) {
    stp_zprintf(v, "21");
  } else {
    stp_zprintf(v, "00");
  }
}

/* Imaging area is wider than print size, we always must supply the 
   printer with the full imaging width. */
static const dyesub_pagesize_t dnpsrx1_dock_page[] =
{
  { "B7",	"3.5x5", PT(1920,300)+1, PT(1088,300)+1, PT(112,300), PT(112,300), 0, 0, DYESUB_PORTRAIT},
  { "w288h432", "4x6", PT(1920,300)+1, PT(1240,300)+1, PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT},
#ifdef DNPX2
  { "2x6_x2", "2x6*2", PT(1920,300)+1, PT(1240,300)+1, PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT},
#endif
  { "w360h504",	"5x7", PT(1920,300)+1, PT(2138,300)+1, PT(112,300), PT(112,300), 0, 0, DYESUB_PORTRAIT},
  { "A5", "6x8", PT(1920,300)+1, PT(2436,300)+1, PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT},
#ifdef DNPX2
  { "4x6_x2", "4x6*2", PT(1920,300)+1, PT(2498,300)+1, PT(38,300), PT(38,300), 0, 0, DYESUB_PORTRAIT},
#endif
};

LIST(dyesub_pagesize_list_t, dnpsrx1_dock_page_list, dyesub_pagesize_t, dnpsrx1_dock_page);

static const dyesub_printsize_t dnpsrx1_dock_printsize[] =
{
  { "300x300", "B7", 1920, 1088},
  { "300x600", "B7", 1920, 2176},
  { "300x300", "w288h432", 1920, 1240},
  { "300x600", "w288h432", 1920, 2480},
#ifdef DNPX2
  { "300x300", "2x6_x2", 1920, 1240},
  { "300x600", "2x6_x2", 1920, 2480},
#endif
  { "300x300", "w360h504", 1920, 2138},
  { "300x600", "w360h504", 1920, 4276},
  { "300x300", "A5", 1920, 2436},
  { "300x600", "A5", 1920, 4872},
#ifdef DNPX2
  { "300x300", "4x6_x2", 1920, 2498},
  { "300x600", "4x6_x2", 1920, 4996},
#endif
};

LIST(dyesub_printsize_list_t, dnpsrx1_dock_printsize_list, dyesub_printsize_t, dnpsrx1_dock_printsize);

static void dnpdsrx1_printer_start(stp_vars_t *v)
{
  /* Common code */
  dnpds40ds80_printer_start(v);

  /* Set cutter option to "normal" */
  stp_zprintf(v, "\033PCNTRL CUTTER          0000000800000");
  if (!strcmp(privdata.pagesize, "2x6_x2")) {
    stp_zprintf(v, "120");
  } else {
    stp_zprintf(v, "000");
  }

  /* Configure multi-cut/page size */
  stp_zprintf(v, "\033PIMAGE MULTICUT        00000008000000");

  if (!strcmp(privdata.pagesize, "B7")) {
    stp_zprintf(v, "01");
  } else if (!strcmp(privdata.pagesize, "w288h432")) {
    stp_zprintf(v, "02");
  } else if (!strcmp(privdata.pagesize, "w360h504")) {
    stp_zprintf(v, "03");
  } else if (!strcmp(privdata.pagesize, "A5")) {
    stp_zprintf(v, "04");
  } else if (!strcmp(privdata.pagesize, "4x6_x2")) {
    stp_zprintf(v, "12");
  } else {
    stp_zprintf(v, "00");
  }
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
  { "w252h338", "3.5x4.7", PT(1210,334)+1, PT(2048,334)+1, 0, 0, PT(225,334), PT(225,334), DYESUB_LANDSCAPE},
  { "B7", "3.5x5", PT(1210,334)+1, PT(2048,334)+1, 0, 0, PT(169,334), PT(169,334), DYESUB_LANDSCAPE},
  { "w288h432", "4x6", PT(1380,334)+1, PT(2048,334)+1, 0, 0, PT(5,334), PT(5,334), DYESUB_LANDSCAPE},
  { "w338h504",	"4.7x7", PT(2048,334)+1, PT(2380,334)+1, PT(225,334), PT(225,334), 0, 0, DYESUB_PORTRAIT},
  { "w360h504",	"5x7", PT(2048,334)+1, PT(2380,334)+1, PT(169,334), PT(169,334), 0, 0, DYESUB_PORTRAIT},
  { "A5", "6x8", PT(2048,334)+1, PT(2710,300)+1, PT(5,334), PT(5,334), 0, 0, DYESUB_PORTRAIT},
  { "w432h576", "6x9", PT(2048,334)+1, PT(3050,334)+1, PT(5,334), PT(5,334), 0, 0, DYESUB_PORTRAIT},
};

LIST(dyesub_pagesize_list_t, citizen_cw01_page_list, dyesub_pagesize_t, citizen_cw01_page);

static const dyesub_printsize_t citizen_cw01_printsize[] =
{
  { "334x334", "w252h338", 1210, 2048},
  { "334x600", "w252h388", 2176, 2048},
  { "334x334", "B7", 1210, 2048},
  { "334x600", "B7", 2176, 2048},
  { "334x334", "w288h432", 1380, 2048},
  { "334x600", "w288h432", 2480, 2048},
  { "334x334", "w338h504", 2048, 2380},
  { "334x600", "w338h504", 2048, 4276},
  { "334x334", "w360h504", 2048, 2380},
  { "334x600", "w360h504", 2048, 4276},
  { "334x334", "A5", 2048, 2710},
  { "334x600", "A5", 2048, 4870},
  { "334x334", "w432h576", 2048, 3050},
  { "334x600", "w432h576", 2048, 5480},
};

LIST(dyesub_printsize_list_t, citizen_cw01_printsize_list, dyesub_printsize_t, citizen_cw01_printsize);

static void citizen_cw01_printer_start(stp_vars_t *v)
{
	int media = 0;

	if (strcmp(privdata.pagesize,"w252h338") == 0)
		media = 0x00;
	else if (strcmp(privdata.pagesize,"B7") == 0)
		media = 0x01;
	else if (strcmp(privdata.pagesize,"w288h432") == 0)
		media = 0x02;
	else if (strcmp(privdata.pagesize,"w338h504") == 0)
		media = 0x03;
	else if (strcmp(privdata.pagesize,"w360h504") == 0)
		media = 0x04;
	else if (strcmp(privdata.pagesize,"A5") == 0)
		media = 0x05;
	else if (strcmp(privdata.pagesize,"w432h576") == 0)
		media = 0x06;

	stp_putc(media, v);
	if (privdata.h_dpi == 600) {
		stp_putc(0x01, v);
	} else {
		stp_putc(0x00, v);
	}
	stp_putc(0x01, v); /* This is actually number of copies */
	stp_putc(0x00, v);

	/* Compute plane size */
	media = (privdata.w_size * privdata.h_size) + 1024 + 40;

	stp_put32_le(media, v);
	stp_put32_le(0x0, v);
}

static void citizen_cw01_plane_init(stp_vars_t *v)
{
	int i;

	stp_put32_le(0x28, v);
	stp_put32_le(0x0800, v);
	stp_put16_le(privdata.h_size, v);  /* number of rows */
	stp_put16_le(0x0, v);
	stp_put32_le(0x080001, v);
	stp_put32_le(0x00, v);
	stp_put32_le(0x00, v);
	stp_put32_le(0x335a, v);
	if (privdata.h_dpi == 600) {
		stp_put32_le(0x5c40, v);
	} else {
		stp_put32_le(0x335a, v);
	}
	stp_put32_le(0x0100, v);
	stp_put32_le(0x00, v);

	/* Write the color curve data. */
	for (i = 0xff; i >= 0 ; i--) {
		unsigned long tmp;
		tmp = i | (i << 8) | (i << 16);
		stp_put32_le(tmp, v);
	}
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
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    &p10_laminate_list, NULL,
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
    p200_adj_any, p200_adj_any, p200_adj_any,
    NULL, NULL,
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
    p300_adj_cyan, p300_adj_magenta, p300_adj_yellow,
    NULL, NULL,
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
    p400_adj_cyan, p400_adj_magenta, p400_adj_yellow,
    NULL, NULL,
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
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    &p10_laminate_list, NULL,
  },
  { /* Olympus P-S100 */
    20,
    &bgr_ink_list,
    &res_306dpi_list,
    &ps100_page_list,
    &ps100_printsize_list,
    1808,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &ps100_printer_init_func, &ps100_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL, NULL,
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
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &cp10_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL, NULL,
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
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL, NULL,
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
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL, NULL,
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
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &es1_printer_init_func, NULL,
    &es1_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL, NULL,
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
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &es2_printer_init_func, NULL,
    &es2_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL, NULL,
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
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &es3_printer_init_func, &es3_printer_end_func,
    &es2_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL, NULL,
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
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &es40_printer_init_func, &es3_printer_end_func,
    &es2_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL, NULL,
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
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &cp790_printer_init_func, &es3_printer_end_func,
    &es2_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL, NULL,
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
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &cpx00_printer_init_func, NULL,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL, NULL,
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
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &cpx00_printer_init_func, &cp900_printer_end_func,
    &cpx00_plane_init_func, NULL,
    NULL, NULL,
    cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
    NULL, NULL,
  },
  { /* Canon CP820, CP910 */
    1011,
    &rgb_ink_list,
    &res_300dpi_list,
    &cp910_page_list,
    &cp910_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS | DYESUB_FEATURE_WHITE_BORDER
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_RGBtoYCBCR,
    &cp910_printer_init_func, NULL,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL, /* Printer handles color correction! */
    NULL, NULL,
  },
  { /* Sony UP-DP10  */
    2000,
    &cmy_ink_list,
    &res_300dpi_list,
    &updp10_page_list,
    &updp10_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_BORDERLESS,
    &updp10_printer_init_func, &updp10_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    updp10_adj_cyan, updp10_adj_magenta, updp10_adj_yellow,
    &updp10_laminate_list, NULL,
  },
  { /* Sony UP-DR150 */
    2001,
    &rgb_ink_list,
    &res_334dpi_list,
    &updr150_page_list,
    &updr150_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &updr150_printer_init_func, &updr150_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL, 
    &updp10_laminate_list, NULL,
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
    NULL, NULL, NULL,
    &dppex5_laminate_list, NULL,
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
    NULL, NULL, NULL, 
    &updr100_laminate_list, NULL,
  },
  { /* Sony UP-DR200 */
    2004,
    &rgb_ink_list,
    &res_334dpi_list,
    &updr150_page_list,
    &updr150_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &updr200_printer_init_func, &updr150_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL, 
    &updr200_laminate_list, NULL,
  },
  { /* Sony UP-CR10L / DNP SL10 */
    2005,
    &rgb_ink_list,
    &res_300dpi_list,
    &upcr10_page_list,
    &upcr10_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &upcr10_printer_init_func, &upcr10_printer_end_func,
    NULL, NULL,
    NULL, NULL,
    NULL, NULL, NULL, 
    NULL, NULL,
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
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL, NULL,
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
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL, NULL,
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
    NULL, NULL, NULL,	/* color profile/adjustment is built into printer */
    NULL, NULL,
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
    NULL, NULL, NULL,
    NULL, NULL,
  },
  { /* Kodak Photo Printer 6800 */
    4001,
    &rgb_ink_list,
    &res_300dpi_list,
    &kodak_6800_page_list,
    &kodak_6800_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &kodak_6800_printer_init, NULL,
    NULL, NULL, /* No plane funcs */
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &kodak_6800_laminate_list, NULL,
  },
  { /* Kodak Photo Printer 6850 */
    4002,
    &rgb_ink_list,
    &res_300dpi_list,
    &kodak_6850_page_list,
    &kodak_6850_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &kodak_6850_printer_init, NULL,
    NULL, NULL, /* No plane funcs */
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &kodak_6800_laminate_list, NULL,
  },
  { /* Kodak Photo Printer 605 */
    4003,
    &rgb_ink_list,
    &res_300dpi_list,
    &kodak_605_page_list,
    &kodak_605_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &kodak_605_printer_init, NULL,
    NULL, NULL, /* No plane funcs */
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &kodak_605_laminate_list, NULL,
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
      | DYESUB_FEATURE_ROW_INTERLACE,
    &kodak_1400_printer_init, NULL,
    NULL, NULL, 
    NULL, NULL, 
    NULL, NULL, NULL, 
    &kodak_6800_laminate_list, 
    &kodak_1400_media_list,
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
      | DYESUB_FEATURE_ROW_INTERLACE,
    &kodak_805_printer_init, NULL,
    NULL, NULL, /* No plane funcs */
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &kodak_6800_laminate_list, NULL,
  },
  { /* Kodak Professional 9810 */
    4006,
    &ymc_ink_list,
    &res_300dpi_list,
    &kodak_9810_page_list,
    &kodak_9810_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &kodak_9810_printer_init, &kodak_9810_printer_end,
    &kodak_9810_plane_init, NULL, 
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &kodak_9810_laminate_list, NULL,
  },
  { /* Kodak 8810 */
    4007,
    &bgr_ink_list,
    &res_300dpi_list,
    &kodak_8810_page_list,
    &kodak_8810_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &kodak_8810_printer_init, NULL,
    NULL, NULL, 
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &kodak_8810_laminate_list, NULL,
  },

  { /* Kodak Professional 8500 */
    4100,
    &bgr_ink_list,
    &res_314dpi_list,
    &kodak_8500_page_list,
    &kodak_8500_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &kodak_8500_printer_init, &kodak_8500_printer_end,
    NULL, NULL, /* No plane funcs */ 
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &kodak_8500_laminate_list, 
    &kodak_8500_media_list,
  },
  { /* Mitsubishi CP3020D/DU/DE */
    4101,
    &ymc_ink_list,
    &res_314dpi_list,
    &mitsu_cp3020d_page_list,
    &mitsu_cp3020d_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &mitsu_cp3020d_printer_init, &mitsu_cp3020d_printer_end,
    &mitsu_cp3020d_plane_init, &mitsu_cp3020d_plane_end, 
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    NULL, NULL,
  },
  { /* Mitsubishi CP3020DA/DAE */
    4102,
    &bgr_ink_list,
    &res_314dpi_list,
    &mitsu_cp3020d_page_list,
    &mitsu_cp3020d_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &mitsu_cp3020da_printer_init, &mitsu_cp3020da_printer_end,
    &mitsu_cp3020da_plane_init, NULL,
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    NULL, NULL,
  },
  { /* Mitsubishi CP9550D */
    4103,
    &rgb_ink_list,
    &res_346dpi_list,
    &mitsu_cp9550_page_list,
    &mitsu_cp9550_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE,
    &mitsu_cp9550_printer_init, &mitsu_cp9550_printer_end,
    &mitsu_cp3020da_plane_init, NULL,
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    NULL, NULL,
  },
  { /* Mitsubishi CP9810D */
    4104,
    &bgr_ink_list,
    &res_300dpi_list,
    &mitsu_cp9810_page_list,
    &mitsu_cp9810_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_12BPP
      | DYESUB_FEATURE_BIGENDIAN,
    &mitsu_cp9810_printer_init, &mitsu_cp9810_printer_end,
    &mitsu_cp3020da_plane_init, NULL,
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &mitsu_cp9810_laminate_list, NULL,
  },
  { /* Mitsubishi CPD70D/CPD707D */
    4105,
    &ymc_ink_list,
    &res_300dpi_list,
    &mitsu_cpd70x_page_list,
    &mitsu_cpd70x_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_16BPP
      | DYESUB_FEATURE_BIGENDIAN,
    &mitsu_cpd70x_printer_init, &mitsu_cpd70x_printer_end,
    NULL, &mitsu_cpd70x_plane_end,
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &mitsu_cpd70x_laminate_list, NULL,
  },
  { /* Mitsubishi CPK60D */
    4106,
    &ymc_ink_list,
    &res_300dpi_list,
    &mitsu_cpk60_page_list,
    &mitsu_cpk60_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_16BPP
      | DYESUB_FEATURE_BIGENDIAN,
    &mitsu_cpk60_printer_init, &mitsu_cpk60_printer_end,
    NULL, &mitsu_cpd70x_plane_end,
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &mitsu_cpd70x_laminate_list, NULL,
  },
  { /* Mitsubishi CPD80D */
    4107,
    &ymc_ink_list,
    &res_300dpi_list,
    &mitsu_cpd80_page_list,
    &mitsu_cpd80_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_16BPP
      | DYESUB_FEATURE_BIGENDIAN,
    &mitsu_cpd70x_printer_init, &mitsu_cpd70x_printer_end,
    NULL, &mitsu_cpd70x_plane_end,
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &mitsu_cpd70x_laminate_list, NULL,
  },
  { /* Kodak 305 */
    4108,
    &ymc_ink_list,
    &res_300dpi_list,
    &kodak305_page_list,
    &kodak305_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_16BPP
      | DYESUB_FEATURE_BIGENDIAN,
    &kodak305_printer_init, &mitsu_cpk60_printer_end,
    NULL, &mitsu_cpd70x_plane_end,
    NULL, NULL, /* No block funcs */
    NULL, NULL, NULL, /* color profile/adjustment is built into printer */
    &mitsu_cpd70x_laminate_list, NULL,
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
    NULL, NULL, NULL,
    NULL, NULL,
  },
  { /* Shinko/Sinfonia CHC-S2145 */
    5001,
    &rgb_ink_list,
    &res_300dpi_list,
    &shinko_chcs2145_page_list,
    &shinko_chcs2145_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &shinko_chcs2145_printer_init, &shinko_chcs2145_printer_end,
    NULL, NULL,  /* No planes */
    NULL, NULL,  /* No blocks */
    NULL, NULL, NULL, /* Color correction in printer */
    &shinko_chcs2145_laminate_list, NULL,
  },
  { /* Shinko/Sinfonia CHC-S1245 */
    5002,
    &rgb_ink_list,
    &res_300dpi_list,
    &shinko_chcs1245_page_list,
    &shinko_chcs1245_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &shinko_chcs1245_printer_init, &shinko_chcs2145_printer_end,
    NULL, NULL,  /* No planes */
    NULL, NULL,  /* No blocks */
    NULL, NULL, NULL, /* Color correction in printer */
    NULL, NULL,
  },
  { /* Shinko/Sinfonia CHC-S6245 */
    5003,
    &rgb_ink_list,
    &res_300dpi_list,
    &shinko_chcs6245_page_list,
    &shinko_chcs6245_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &shinko_chcs6245_printer_init, &shinko_chcs2145_printer_end,
    NULL, NULL,  /* No planes */
    NULL, NULL,  /* No blocks */
    NULL, NULL, NULL, /* Color correction in printer */
    &shinko_chcs6245_laminate_list, NULL,
  },
  { /* Shinko/Sinfonia CHC-S6145 */
    5004,
    &rgb_ink_list,
    &res_300dpi_list,
    &shinko_chcs6145_page_list,
    &shinko_chcs6145_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT,
    &shinko_chcs6145_printer_init, &shinko_chcs2145_printer_end,
    NULL, NULL,  /* No planes */
    NULL, NULL,  /* No blocks */
    NULL, NULL, NULL, /* Color correction in printer */
    &shinko_chcs6145_laminate_list, NULL,
  },
  { /* Dai Nippon Printing DS40 */
    6000,
    &bgr_ink_list,
    &res_dnpds40_dpi_list,
    &dnpds40_dock_page_list,
    &dnpds40_dock_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER 
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT,
    &dnpds40_printer_start, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL, NULL, NULL,
    &dnpds40_laminate_list, NULL,
  },
  { /* Dai Nippon Printing DS80 */
    6001,
    &bgr_ink_list,
    &res_dnpds40_dpi_list,
    &dnpds80_dock_page_list,
    &dnpds80_dock_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER 
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT,
    &dnpds80_printer_start, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL, NULL, NULL,
    &dnpds40_laminate_list, NULL,
  },
  { /* Dai Nippon Printing DSRX1 */
    6002,
    &bgr_ink_list,
    &res_dnpds40_dpi_list,
    &dnpsrx1_dock_page_list,
    &dnpsrx1_dock_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER 
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT,
    &dnpdsrx1_printer_start, &dnpds40_printer_end,
    &dnpds40_plane_init, NULL,
    NULL, NULL,
    NULL, NULL, NULL,
    &dnpds40_laminate_list, NULL,
  },
  { /* Citizen CW-01 */
    6005,
    &bgr_ink_list,
    &res_citizen_cw01_dpi_list,
    &citizen_cw01_page_list,
    &citizen_cw01_printsize_list,
    SHRT_MAX,
    DYESUB_FEATURE_FULL_WIDTH | DYESUB_FEATURE_FULL_HEIGHT | DYESUB_FEATURE_WHITE_BORDER 
      | DYESUB_FEATURE_PLANE_INTERLACE | DYESUB_FEATURE_PLANE_LEFTTORIGHT,
    &citizen_cw01_printer_start, NULL,
    &citizen_cw01_plane_init, NULL,
    NULL, NULL,
    NULL, NULL, NULL,
    NULL, NULL,
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
  {
    /* TRANSLATORS: Some dye sublimation printers are able to achieve */
    /* better durability of output by covering it with transparent */
    /* laminate surface. This surface can be of different patterns: */
    /* common are matte, glossy or texture. */
    "Laminate", N_("Laminate Pattern"), "Color=Yes,Category=Advanced Printer Setup",
    N_("Laminate Pattern"),
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

static const dyesub_cap_t* dyesub_get_model_capabilities(int model)
{
  int i;
  int models = sizeof(dyesub_model_capabilities) / sizeof(dyesub_cap_t);

  for (i=0; i<models; i++)
    {
      if (dyesub_model_capabilities[i].model == model)
        return &(dyesub_model_capabilities[i]);
    }
  stp_deprintf(STP_DBG_DYESUB,
  	"dyesub: model %d not found in capabilities list.\n", model);
  return &(dyesub_model_capabilities[0]);
}

static const laminate_t* dyesub_get_laminate_pattern(stp_vars_t *v)
{
  const char *lpar = stp_get_string_parameter(v, "Laminate");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
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

static const dyesub_media_t* dyesub_get_mediatype(stp_vars_t *v)
{
  const char *mpar = stp_get_string_parameter(v, "MediaType");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
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
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
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
  stp_erprintf("dyesub_printsize: printsize not found (%s, %s)\n",
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
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;

  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  return ret;
}

static void
dyesub_parameters(const stp_vars_t *v, const char *name,
	       stp_parameter_t *description)
{
  int	i;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
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
      const dyesub_pagesize_list_t *p = caps->pages;
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
  else if (strcmp(name, "InputSlot") == 0)
    {
      description->bounds.str = stp_string_list_create();
      description->is_active = 0;
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
      if (dyesub_feature(caps, DYESUB_FEATURE_BORDERLESS)) 
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


static const dyesub_pagesize_t*
dyesub_current_pagesize(const stp_vars_t *v)
{
  const char *page = stp_get_string_parameter(v, "PageSize");
  const stp_papersize_t *pt = stp_get_papersize_by_name(page);
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
		  				stp_get_model_id(v));
  const dyesub_pagesize_list_t *p = caps->pages;
  int i;

  for (i = 0; i < p->n_items; i++)
    {
      if (strcmp(p->item[i].name,pt->name) == 0)
          return &(p->item[i]);
    }
  return NULL;
}

static void
dyesub_media_size(const stp_vars_t *v,
		int *width,
		int *height)
{
  const dyesub_pagesize_t *p = dyesub_current_pagesize(v);
  stp_default_media_size(v, width, height);

  if (p && p->width_pt > 0)
    *width = p->width_pt;
  if (p && p->height_pt > 0)
    *height = p->height_pt;
}

static void
dyesub_imageable_area_internal(const stp_vars_t *v,
				int  use_maximum_area,
				int  *left,
				int  *right,
				int  *bottom,
				int  *top,
				int  *print_mode)
{
  int width, height;
  const dyesub_pagesize_t *p = dyesub_current_pagesize(v);
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
		  				stp_get_model_id(v));

  dyesub_media_size(v, &width, &height);
  if (use_maximum_area
      || (dyesub_feature(caps, DYESUB_FEATURE_BORDERLESS) &&
          stp_get_boolean_parameter(v, "Borderless"))
      || !p)
    {
      *left = 0;
      *top  = 0;
      *right  = width;
      *bottom = height;
    }
  else
    {
      *left = p->border_pt_left;
      *top  = p->border_pt_top;
      *right  = width  - p->border_pt_right;
      *bottom = height - p->border_pt_bottom;
    }
  if (p)
    *print_mode = p->print_mode;
  else
    *print_mode = DYESUB_PORTRAIT;
}

static void
dyesub_imageable_area(const stp_vars_t *v,
		       int  *left,
		       int  *right,
		       int  *bottom,
		       int  *top)
{
  int not_used;
  dyesub_imageable_area_internal(v, 0, left, right, bottom, top, &not_used);
}

static void
dyesub_maximum_imageable_area(const stp_vars_t *v,
			       int  *left,
			       int  *right,
			       int  *bottom,
			       int  *top)
{
  int not_used;
  const int model = stp_get_model_id(v);
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(model);

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
	    int *width, int *height,
	    int *min_width, int *min_height)
{
  *width  = SHRT_MAX;
  *height = SHRT_MAX;
  *min_width  = 1;
  *min_height =	1;
}

static void
dyesub_describe_resolution(const stp_vars_t *v, int *x, int *y)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
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
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(
  							stp_get_model_id(v));
  const char *output_type;
  int i;

  pv->ink_channels = 1;
  pv->ink_order = NULL;
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

static void
dyesub_nputc(stp_vars_t *v, char byte, int count)
{
  if (count == 1)
    stp_putc(byte, v);
  else
    {
      int i;
      char *buf = privdata.nputc_buf;
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
      stp_deprintf(STP_DBG_DYESUB, "dyesub: %s\n", debug_string);
      (*func)(v);
    }
}

static int
dyesub_interpolate(int oldval, int oldsize, int newsize)
{
  /* 
   * This is simple linear interpolation algorithm.
   * When imagesize <> printsize I need rescale image somehow... :-/ 
   */
  return (int)(oldval * newsize / oldsize);
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
	  stp_deprintf(STP_DBG_DYESUB,
	  	"dyesub_read_image: "
		"stp_color_get_row(..., %d, ...) == 0\n", i);
	  dyesub_free_image(pv, image);
	  return NULL;
	}	
      image_data[i] = stp_malloc(row_size);
      pv->image_rows = i+1;
      if (!image_data[i])
        {
	  stp_deprintf(STP_DBG_DYESUB,
	  	"dyesub_read_image: "
		"(image_data[%d] = stp_malloc()) == NULL\n", i);
	  dyesub_free_image(pv, image);
	  return NULL;
	}	
      memcpy(image_data[i], stp_channel_get_output(v), row_size);
    }
  return image_data;
}

static int
dyesub_print_pixel(stp_vars_t *v,
		dyesub_print_vars_t *pv,
		const dyesub_cap_t *caps,
		int row,
		int col,
		int plane)
{
  unsigned short ink[MAX_INK_CHANNELS], *out;
  int i, j, b;

  if (pv->print_mode == DYESUB_LANDSCAPE)
    { /* "rotate" image */
      dyesub_swap_ints(&col, &row);
      row = (pv->imgw_px - 1) - row;
    }

  out = &(pv->image_data[row][col * pv->out_channels]);

  for (i = 0; i < pv->ink_channels; i++)
    {
      if (pv->out_channels == pv->ink_channels)
        { /* copy out_channel (image) to equiv ink_channel (printer) */
		if (dyesub_feature(caps, DYESUB_FEATURE_RGBtoYCBCR)) {
			/* Convert RGB -> YCbCr (JPEG YCbCr444 coefficients) */
			double R, G, B;
			double Y, Cr, Cb;
			R = out[0];
			G = out[1];
			B = out[2];
			
			Y  = R *  0.29900 + G *  0.58700 + B *  0.11400;
			Cb = R * -0.16874 + G * -0.33126 + B *  0.50000 + 32768;
			Cr = R *  0.50000 + G * -0.41869 + B * -0.08131 + 32768;
			
			ink[0] = Y;
			ink[1] = Cb;
			ink[2] = Cr;

			// XXX this is sub-optimal; we compute the full YCbCr
			// values and throw away 2/3 for each pixel printed
			// if we are plane or row interleaved
		} else {
			ink[i] = out[i];
		}
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
   
  /* Downscale 16bpp to output bpp */
  /* FIXME:  Do we want to round? */
  if (pv->bytes_per_ink_channel == 1) 
    {
      unsigned char *ink_u8 = (unsigned char *) ink;
      for (i = 0; i < pv->ink_channels; i++) {
#if 0
             if (dyesub_feature(caps, DYESUB_FEATURE_RGBtoYCBCR))
                     ink_u8[i] = ink[i] >> 8;
             else
#endif
                     ink_u8[i] = ink[i] / 257;
      }
    } 
  else if (pv->bits_per_ink_channel != 16)
    {
      for (i = 0; i < pv->ink_channels; i++)
	ink[i] = ink[i] >> (16 - pv->bits_per_ink_channel);
    }

  /* Byteswap as needed */
  if (pv->bytes_per_ink_channel == 2 && pv->byteswap)
    for (i = 0; i < pv->ink_channels; i++)
      ink[i] = ((ink[i] >> 8) & 0xff) | ((ink[i] & 0xff) << 8);

  if (pv->plane_interlacing || pv->row_interlacing)
    stp_zfwrite((char *) ink + (plane * pv->bytes_per_ink_channel), 
		pv->bytes_per_ink_channel, 1, v);
  else
      /* print inks in correct order, eg. RGB  BGR */
      for (b = 0; b < pv->ink_channels; b++)
	stp_zfwrite((char *) ink + (pv->bytes_per_ink_channel * (pv->ink_order[b]-1)), 
		    pv->bytes_per_ink_channel, 1, v);

  return 1;
}

static int
dyesub_print_row(stp_vars_t *v,
		dyesub_print_vars_t *pv,
		const dyesub_cap_t *caps,
		int row,
		int plane)
{
  int ret = 0;
  int w, col;
  
  for (w = 0; w < pv->outw_px; w++)
    {
      col = dyesub_interpolate(w, pv->outw_px, pv->imgw_px);
      if (pv->plane_lefttoright)
	      ret = dyesub_print_pixel(v, pv, caps, row, pv->imgw_px - col - 1, plane);
      else
	      ret = dyesub_print_pixel(v, pv, caps, row, col, plane);
      if (ret > 1)
      	break;
    }
  return ret;
}

static int
dyesub_print_plane(stp_vars_t *v,
		dyesub_print_vars_t *pv,
		const dyesub_cap_t *caps,
		int plane)
{
  int ret = 0;
  int h, row, p;
  int out_bytes = ((pv->plane_interlacing || pv->row_interlacing) ? 1 : pv->ink_channels)
  					* pv->bytes_per_ink_channel;

  for (h = 0; h <= pv->prnb_px - pv->prnt_px; h++)
    {
      p = pv->row_interlacing ? 0 : plane;

      do {

      if (h % caps->block_size == 0)
        { /* block init */
	  privdata.block_min_h = h + pv->prnt_px;
	  privdata.block_min_w = pv->prnl_px;
	  privdata.block_max_h = MIN(h + pv->prnt_px + caps->block_size - 1,
	  					pv->prnb_px);
	  privdata.block_max_w = pv->prnr_px;

	  dyesub_exec(v, caps->block_init_func, "caps->block_init");
	}

      if (h + pv->prnt_px < pv->outt_px || h + pv->prnt_px >= pv->outb_px)
        { /* empty part above or below image area */
          dyesub_nputc(v, pv->empty_byte[plane], out_bytes * pv->prnw_px);
	}
      else
        {
	  if (dyesub_feature(caps, DYESUB_FEATURE_FULL_WIDTH)
	  	&& pv->outl_px > 0)
	    { /* empty part left of image area */
              dyesub_nputc(v, pv->empty_byte[plane], out_bytes * pv->outl_px);
	    }

	  row = dyesub_interpolate(h + pv->prnt_px - pv->outt_px,
	  					pv->outh_px, pv->imgh_px);
	  stp_deprintf(STP_DBG_DYESUB,
	  	"dyesub_print_plane: h = %d, row = %d\n", h, row);
	  ret = dyesub_print_row(v, pv, caps, row, p);

	  if (dyesub_feature(caps, DYESUB_FEATURE_FULL_WIDTH)
	  	&& pv->outr_px < pv->prnw_px)
	    { /* empty part right of image area */
	      dyesub_nputc(v, pv->empty_byte[plane], out_bytes
	      				* (pv->prnw_px - pv->outr_px));
	    }
	}

      if (h + pv->prnt_px == privdata.block_max_h)
        { /* block end */
	  dyesub_exec(v, caps->block_end_func, "caps->block_end");
	}

      } while (pv->row_interlacing && ++p < pv->ink_channels);
    }
  return ret;
}

/*
 * dyesub_print()
 */
static int
dyesub_do_print(stp_vars_t *v, stp_image_t *image)
{
  int i;
  dyesub_print_vars_t pv;
  int status = 1;

  const int model           = stp_get_model_id(v); 
  const char *ink_type;
  const dyesub_cap_t *caps = dyesub_get_model_capabilities(model);
  int max_print_px_width = 0;
  int max_print_px_height = 0;
  int w_dpi, h_dpi;	/* Resolution */

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
  (void) memset(&pv, 0, sizeof(pv));

  stp_image_init(image);
  pv.imgw_px = stp_image_width(image);
  pv.imgh_px = stp_image_height(image);

  stp_describe_resolution(v, &w_dpi, &h_dpi);
  dyesub_printsize(v, &max_print_px_width, &max_print_px_height);

  privdata.pagesize = stp_get_string_parameter(v, "PageSize");
  if (caps->laminate)
	  privdata.laminate = dyesub_get_laminate_pattern(v);
  if (caps->media)
	  privdata.media = dyesub_get_mediatype(v);

  dyesub_imageable_area_internal(v, 
  	(dyesub_feature(caps, DYESUB_FEATURE_WHITE_BORDER) ? 1 : 0),
	&page_pt_left, &page_pt_right, &page_pt_bottom, &page_pt_top,
	&page_mode);
  
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
  pv.outb_px = pv.outt_px  + pv.outh_px;
  

  stp_deprintf(STP_DBG_DYESUB,
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


  /* FIXME: move this into print_init_drv */
  ink_type = dyesub_describe_output_internal(v, &pv);
  stp_set_string_parameter(v, "STPIOutputType", ink_type);
  stp_channel_reset(v);
  for (i = 0; i < pv.ink_channels; i++)
    stp_channel_add(v, i, 0, 1.0);
  pv.out_channels = stp_color_init(v, image, 65536);

  if (dyesub_feature(caps, DYESUB_FEATURE_12BPP)) {
    pv.bytes_per_ink_channel = 2;
    pv.bits_per_ink_channel = 12;
  } else if (dyesub_feature(caps, DYESUB_FEATURE_16BPP)) {
    pv.bytes_per_ink_channel = 2;
    pv.bits_per_ink_channel = 16;
  } else {
    pv.bytes_per_ink_channel = 1;
    pv.bits_per_ink_channel = 8;
  }

  if (pv.bytes_per_ink_channel > 1) {
#if defined(__LITTLE_ENDIAN) || defined(__LITTLE_ENDIAN__)
    pv.byteswap = dyesub_feature(caps, DYESUB_FEATURE_BIGENDIAN);
#elif defined (__BIG_ENDIAN) || defined(__BIG_ENDIAN__)
    pv.byteswap = !dyesub_feature(caps, DYESUB_FEATURE_BIGENDIAN);
#else
#error "Unable to determine endianness, aborting compilation!"
#endif    
  }

  pv.image_data = dyesub_read_image(v, &pv, image);
  if (ink_type) {
	  if (dyesub_feature(caps, DYESUB_FEATURE_RGBtoYCBCR)) {
		  pv.empty_byte[0] = 0xff; /* Y */
		  pv.empty_byte[1] = 0x80; /* Cb */
		  pv.empty_byte[2] = 0x80; /* Cr */
	  } else if (strcmp(ink_type, "RGB") == 0 || strcmp(ink_type, "BGR") == 0) {
		  pv.empty_byte[0] = 0xff;
		  pv.empty_byte[1] = 0xff;
		  pv.empty_byte[2] = 0xff;
	  } else {
		  pv.empty_byte[0] = 0x0;
		  pv.empty_byte[1] = 0x0;
		  pv.empty_byte[2] = 0x0;
	  }
  } else {
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
      return 2;
    }
  /* /FIXME */

  /* FIXME:  Provide a way of disabling/altering these curves */
  dyesub_adjust_curve(v, caps->adj_cyan, "CyanCurve");
  dyesub_adjust_curve(v, caps->adj_magenta, "MagentaCurve");
  dyesub_adjust_curve(v, caps->adj_yellow, "YellowCurve");
  stp_set_float_parameter(v, "Density", 1.0);

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

  /* assign private data *after* swaping image dimensions */
  privdata.w_dpi = w_dpi;
  privdata.h_dpi = h_dpi;
  privdata.w_size = pv.prnw_px;
  privdata.h_size = pv.prnh_px;
  privdata.print_mode = pv.print_mode;
  privdata.bpp = pv.bits_per_ink_channel;

  /* printer init */
  dyesub_exec(v, caps->printer_init_func, "caps->printer_init");

  for (pl = 0; pl < (pv.plane_interlacing ? pv.ink_channels : 1); pl++)
    {
      privdata.plane = pv.ink_order[pl];
      stp_deprintf(STP_DBG_DYESUB, "dyesub: plane %d\n", privdata.plane);

      /* plane init */
      dyesub_exec(v, caps->plane_init_func, "caps->plane_init");
  
      dyesub_print_plane(v, &pv, caps, (int) pv.ink_order[pl] - 1);

      /* plane end */
      dyesub_exec(v, caps->plane_end_func, "caps->plane_end");
    }

  /* printer end */
  dyesub_exec(v, caps->printer_end_func, "caps->printer_end");

  dyesub_free_image(&pv, image);
  stp_image_conclude(image);
  return status;
}

static int
dyesub_print(const stp_vars_t *v, stp_image_t *image)
{
  int status;
  stp_vars_t *nv = stp_vars_create_copy(v);
  stp_prune_inactive_options(nv);
  status = dyesub_do_print(nv, image);
  stp_vars_destroy(nv);
  return status;
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
  stp_verify_printer_params,
  NULL,
  NULL,
  NULL
};




static stp_family_t print_dyesub_module_data =
  {
    &print_dyesub_printfuncs,
    NULL
  };


static int
print_dyesub_module_init(void)
{
  return stp_family_register(print_dyesub_module_data.printer_list);
}


static int
print_dyesub_module_exit(void)
{
  return stp_family_unregister(print_dyesub_module_data.printer_list);
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

