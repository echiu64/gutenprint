/*
 * "$Id$"
 *
 *   Print plug-in Lexmark driver for the GIMP.
 *
 *   Copyright 2000 Richard Wisenoecker (richard.wisenoecker@gmx.at)
 *
 *   The plug-in is based on the code of the CANON BJL plugin for the GIMP
 *   of Michael Sweet (mike@easysw.com) and Robert Krawitz (rlk@alum.mit.edu).
 *
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

/*
 * !!! IMPORTANT !!!  Some short information: Border and page offsets
 * are defined in 1/72 DPI. This mean, that the parameter defined at
 * lexmark_cap_t which defines positions are in 1/72 DPI. At
 * lexmark_print the unit will be changed dependent on the printer,
 * according to the value defined at lexmark_cap_t.x_raster_res and
 * lexmark_cap_t.y_raster_res. These two parameters are specifing the
 * resolution used for positioning the printer head (it is not the
 * resolution used for printing!).
 */

/* TODO-LIST
 *
 *   * implement the left border
 *
 */

/*#define DEBUG 1*/
#define USEEPSEWAVE 1

#ifdef __GNUC__
#define inline __inline__
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print.h>
#include <gimp-print-internal.h>
#include <gimp-print-intl-internal.h>
#include <string.h>

#define false 0
#define true  1

typedef struct lexm_privdata_weave {
  int           bidirectional;
  unsigned char *outbuf;
} lexm_privdata_weave;



static int maxclen = 0;

static int lxm3200_headpos = 0;
static int lxm3200_linetoeject = 0;
static int lxm_nozzles_used = 192;

#define LXM3200_LEFTOFFS 6254
#define LXM3200_RIGHTOFFS (LXM3200_LEFTOFFS-2120)


/*
 * Local functions...
 */

#define max(a, b) ((a > b) ? a : b)

typedef enum Lex_model { m_lex7500,   m_z52=10052, m_3200=3200 } Lex_model;


#ifdef DEBUG
typedef struct testdata {
  FILE *ifile;
  int x, y, cols, deep;
  char colchar[16];
  char *input_line;
} testdata;

const stp_vars_t  *dbgfile;
int  lex_show_lcount, lex_show_length;

void lex_show_dither(const stp_vars_t file, unsigned char *y, unsigned char *c, unsigned char *m, unsigned char *ly, unsigned char *lc, unsigned char *lm, unsigned char *k, int length);
const stp_vars_t lex_show_init(int x, int y);
void lex_show_deinit(const stp_vars_t file);
#endif

static void
flush_pass(stp_softweave_t *sw, int passno, int model, int width,
	   int hoffset, int ydpi, int xdpi, int physical_xdpi,
	   int vertical_subpass);

/*** resolution specific parameters */
#define DPI300   0
#define DPI600   1
#define DPI1200  2
#define DPI2400  3
#define DPItest  4

#define RESOLUTION_COUNT 4

typedef struct { /* resolution specific parameters */
  int bidirectional_printing;
} lexmark_sub_cap_t;

typedef struct {
  Lex_model model;    /* printer model */
  int max_width;      /* maximum printable paper size in 1/72 inch */
  int max_length;
  unsigned int supp_res; /* list of allowed resolution right bit represents index 0 */
  int max_xdpi;
  int max_ydpi;
  int max_quality;
  int border_left;    /* unit is 72 DPI */
  int border_right;
  int border_top;
  int border_bottom;
  int inks;           /* installable cartridges (LEXMARK_INK_*) */
  int slots;          /* available paperslots */
  int features;       /* special bjl settings */
  lexmark_sub_cap_t res_specific[RESOLUTION_COUNT]; /* resolution specific parameters */
  const int *head_offset;  /* ????????????? */
  /*** printer internal parameters ***/
  /* the unit of the following parameters is identical with max phys unit of the printer */
  int offset_left_border;      /* Offset to the left paper border (== border_left=0) */
  int offset_top_border;       /* Offset to the top paper border (== border_top=0) */
  int h_offset_black_color;    /* Offset beetween first black and first color jet horizontally */
  int v_offset_black_color;    /* Offset beetween first black and first color jet vertically */
  int direction_offset_black;  /* Offset when printing in the other direction for black */
  int direction_offset_color;  /* Offset when printing in the other direction for color */
  int x_raster_res;            /* horizontal resolution for positioning of the printer head in DPI */
  int y_raster_res;            /* vertical   resolution for positioning of the printer head in DPI */
} lexmark_cap_t;




static const int IDX_Z52ID =2;
static const int IDX_SEQLEN=3;



#define V_NOZZLE_MASK 0x3
#define H_NOZZLE_MASK 0xc
#define NOZZLE_MASK   0xf

#define PRINT_MODE_300   0x100
#define PRINT_MODE_600   0x200
#define PRINT_MODE_1200  0x300
#define PRINT_MODE_2400  0x400

#define COLOR_MODE_K      0x1000
#define COLOR_MODE_C      0x2000
#define COLOR_MODE_Y      0x4000
#define COLOR_MODE_M      0x8000
#define COLOR_MODE_LC    0x10000
#define COLOR_MODE_LY    0x20000
#define COLOR_MODE_LM    0x40000

#define COLOR_MODE_MASK  0x7f000
#define PRINT_MODE_MASK    0xf00
#define COLOR_MODE_PHOTO (COLOR_MODE_LC | COLOR_MODE_LM)

#define BWR      0
#define BWL      1
#define CR       2
#define CL       3


#define LX_Z52_300_DPI  1
#define LX_Z52_600_DPI  3
#define LX_Z52_1200_DPI 4
#define LX_Z52_2400_DPI 5

#define LX_Z52_COLOR_PRINT 0
#define LX_Z52_BLACK_PRINT 1

#define LX_PSHIFT                   0x13
#define LX_Z52_COLOR_MODE_POS       0x9
#define LX_Z52_RESOLUTION_POS       0x7
#define LX_Z52_PRINT_DIRECTION_POS  0x8

#define LXM_Z52_HEADERSIZE 34
static const unsigned char outbufHeader_z52[LXM_Z52_HEADERSIZE]=
{
  0x1B,0x2A,0x24,0x00,0x00,0xFF,0xFF,         /* number of packets ----     vvvvvvvvv */
  0x01,0x01,0x01,0x1a,0x03,0x01,              /* 0x7-0xc: resolution, direction, head */
  0x03,0x60,                                  /* 0xd-0xe HE */
  0x04,0xe0,                                  /* 0xf-0x10  HS vertical pos */
  0x19,0x5c,                                  /* 0x11-0x12 */
  0x0,0x0,                                    /* 0x13-0x14  VO between packges*/
  0x0,0x80,                                   /* 0x15-0x16 */
  0x0,0x0,0x0,0x0,0x1,0x2,0x0,0x0,0x0,0x0,0x0 /* 0x17-0x21 */
};

#define LXM_3200_HEADERSIZE 24

static const char outbufHeader_3200[LXM_3200_HEADERSIZE] =
{
  0x1b, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x1b, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x1b, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static inline int
lexmark_calc_3200_checksum(unsigned char *data)
{
  int ck, i;

  ck = 0;
  for(i=1; i<7; i++)ck += data[i];

  return(ck & 255);
}

/*
   head:
     1 .. black,
     0 .. color

   resolution:
     1 .. 300 dpi (for black ?)
     2 .. like 1
     3 .. 600 dpi (color&black)
     4 .. 1200 dpi
     5 .. ? like 1
*/


static const double sat_adjustment[49] =
{
  1.0,				/* C */
  1.1,
  1.2,
  1.3,
  1.4,
  1.5,
  1.6,
  1.7,
  1.8,				/* B */
  1.9,
  1.9,
  1.9,
  1.7,
  1.5,
  1.3,
  1.1,
  1.0,				/* M */
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,				/* R */
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,				/* Y */
  1.0,
  1.0,
  1.1,
  1.2,
  1.3,
  1.4,
  1.5,
  1.5,				/* G */
  1.4,
  1.3,
  1.2,
  1.1,
  1.0,
  1.0,
  1.0,
  1.0				/* C */
};

static const double lum_adjustment[49] =
{
  0.50,				/* C */
  0.6,
  0.7,
  0.8,
  0.9,
  0.86,
  0.82,
  0.79,
  0.78,				/* B */
  0.8,
  0.83,
  0.87,
  0.9,
  0.95,
  1.05,
  1.15,
  1.3,				/* M */
  1.25,
  1.2,
  1.15,
  1.12,
  1.09,
  1.06,
  1.03,
  1.0,				/* R */
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,
  1.0,				/* Y */
  0.9,
  0.8,
  0.7,
  0.65,
  0.6,
  0.55,
  0.52,
  0.48,				/* G */
  0.47,
  0.47,
  0.49,
  0.49,
  0.49,
  0.52,
  0.51,
  0.50				/* C */
};

static const double hue_adjustment[49] =
{
  0.00,				/* C */
  0.05,
  0.04,
  0.01,
  -0.03,
  -0.10,
  -0.18,
  -0.26,
  -0.35,			/* B */
  -0.43,
  -0.40,
  -0.32,
  -0.25,
  -0.18,
  -0.10,
  -0.07,
  0.00,				/* M */
  -0.04,
  -0.09,
  -0.13,
  -0.18,
  -0.23,
  -0.27,
  -0.31,
  -0.35,			/* R */
  -0.38,
  -0.30,
  -0.23,
  -0.15,
  -0.08,
  0.00,
  -0.02,
  0.00,				/* Y */
  0.08,
  0.10,
  0.08,
  0.05,
  0.03,
  -0.03,
  -0.12,
  -0.20,			/* G */
  -0.17,
  -0.20,
  -0.17,
  -0.15,
  -0.12,
  -0.10,
  -0.08,
  0.00,				/* C */
};


static const int lr_shift_color[10] = { 9, 18, 2*18 }; /* vertical distance between ever 2nd  inkjet (related to resolution) */
static const int lr_shift_black[10] = { 9, 18, 2*18 }; /* vertical distance between ever 2nd  inkjet (related to resolution) */

/* returns the offset of the first jet when printing in the other direction */
static int get_lr_shift(int mode)
{

  const int *ptr_lr_shift;

      /* K could only be present if black is printed only. */
  if((mode & COLOR_MODE_K) == (mode & COLOR_MODE_MASK)) {
    ptr_lr_shift = lr_shift_black;
  } else {
    ptr_lr_shift = lr_shift_color;
  }

      switch(mode & PRINT_MODE_MASK) 	{
	case PRINT_MODE_300:
	  return ptr_lr_shift[0];
	  break;
	case PRINT_MODE_600:
	  return ptr_lr_shift[1];
	  break;
	case PRINT_MODE_1200:
	  return ptr_lr_shift[2];
	  break;
	case PRINT_MODE_2400:
	  return ptr_lr_shift[2];
	  break;
      }
      return 0;
}

/* Codes for possible ink-tank combinations.
 * Each combo is represented by the colors that can be used with
 * the installed ink-tank(s)
 * Combinations of the codes represent the combinations allowed for a model
 */
#define LEXMARK_INK_K           1
#define LEXMARK_INK_CMY         2
#define LEXMARK_INK_CMYK        4
#define LEXMARK_INK_CcMmYK      8
#define LEXMARK_INK_CcMmYy     16
#define LEXMARK_INK_CcMmYyK    32

#define LEXMARK_INK_BLACK_MASK (LEXMARK_INK_K|LEXMARK_INK_CMYK|\
                              LEXMARK_INK_CcMmYK|LEXMARK_INK_CcMmYyK)

#define LEXMARK_INK_PHOTO_MASK (LEXMARK_INK_CcMmYy|LEXMARK_INK_CcMmYK|\
                              LEXMARK_INK_CcMmYyK)

/* document feeding */
#define LEXMARK_SLOT_ASF1    1
#define LEXMARK_SLOT_ASF2    2
#define LEXMARK_SLOT_MAN1    4
#define LEXMARK_SLOT_MAN2    8

/* model peculiarities */
#define LEXMARK_CAP_DMT       1<<0    /* Drop Modulation Technology */
#define LEXMARK_CAP_MSB_FIRST 1<<1    /* how to send data           */
#define LEXMARK_CAP_CMD61     1<<2    /* uses command #0x61         */
#define LEXMARK_CAP_CMD6d     1<<3    /* uses command #0x6d         */
#define LEXMARK_CAP_CMD70     1<<4    /* uses command #0x70         */
#define LEXMARK_CAP_CMD72     1<<5    /* uses command #0x72         */


static const int xxdefault_head_offset[] =
{40, 92, 184, 0, 0, 0, 0, 0};  /* k, c, m, y */ /* for 600 DPI & 2400x1200 */
static const int last_default_head_offset[] =
{70, 184, 368, 0, 0, 0, 0, 0};  /* k, c, m, y */  /* for 1200 DPI */
static const int default_head_offset[] =
{70, 184, 368, 0, 184, 368, 0, 0};  /* k, c, m, y, C, M, Y */  /* for 1200 DPI */


static const lexmark_cap_t lexmark_model_capabilities[] =
{
  /* default settings for unkown models */

  {   (Lex_model)-1, 8*72,11*72,180,180,20,20,20,20, LEXMARK_INK_K, LEXMARK_SLOT_ASF1, 0 },

  /* tested models */

  { /* Lexmark */
    m_z52,
    618, 936,      /* max paper size *//* 8.58" x 13 " */
    0xffff,        /* supp_res */
    2400, 1200, 2, /* max resolution */
    0, 0, 5, 36, /* border l,r,t,b    unit is 1/72 DPI */
    LEXMARK_INK_CMY | LEXMARK_INK_CMYK | LEXMARK_INK_CcMmYK,
    LEXMARK_SLOT_ASF1 | LEXMARK_SLOT_MAN1,
    LEXMARK_CAP_DMT,
    /* resolution specific */
    {{true}, {true}, {true}, {true}},
    default_head_offset,  /* head_offset */
    /*** printer internal parameters ***/
    20,         /* real left paper border */
    123,       /* real top paper border */
    0,       /* black/color offset h */
    324,         /* black/color offset vertically */
    30,        /* direction offset black */
    10,         /* direction offset color */
    2400,   /* horizontal resolutio of 2400 dpi for positioning */
    1200    /* use a vertical resolution of 1200 dpi for positioning */
  },
  { /* Lexmark 3200 */
    m_3200,
    618, 936,      /* 8.58" x 13 " */
    0xffff,        /* supp_res */
    1200, 1200, 2,
    11, 9, 10, 18,
    LEXMARK_INK_CMYK | LEXMARK_INK_CcMmYK,
    LEXMARK_SLOT_ASF1 | LEXMARK_SLOT_MAN1,
    LEXMARK_CAP_DMT,
    /* resolution specific */
    {{false}, {false}, {false}, {false}},
    default_head_offset,  /* head_offset */
    /*** printer internal parameters ***/
    0,         /* real left paper border */
    300,       /* real top paper border */
    0,         /* black/color offset */
    20,         /* black/color offset vertically */
    40,        /* direction offset black */
    12,         /* direction offset color */
    1200,   /* horizontal resolutio of ?? dpi for positioning */
    1200    /* use a vertical resolution of 1200 dpi for positioning */
  },
  { /*  */
    m_lex7500,
    618, 936,      /* 8.58" x 13 " */
    0xffff,        /* supp_res */
    2400, 1200, 2,
    11, 9, 10, 18,
    LEXMARK_INK_CMY | LEXMARK_INK_CMYK | LEXMARK_INK_CcMmYK,
    LEXMARK_SLOT_ASF1 | LEXMARK_SLOT_MAN1,
    LEXMARK_CAP_DMT,
    /* resolution specific */
    {{true}, {true}, {true}, {true}},
    default_head_offset,  /* head_offset */
    /*** printer internal parameters ***/
    0,         /* real left paper border */
    300,       /* real top paper border */
    0,         /* black/color offset */
    20,        /* black/color offset vertically */
    25,        /* direction offset black */
    6,         /* direction offset color */
    1200,   /* horizontal resolutio of ??? dpi for positioning */
    1200    /* use a vertical resolution of 1200 dpi for positioning */
  },
};


static int model_to_index(int model)
{
  int i;
  int models= sizeof(lexmark_model_capabilities) / sizeof(lexmark_cap_t);
  for (i=0; i<models; i++) {
    if (lexmark_model_capabilities[i].model == model) {
      return i;
    }
  }
  return -1;
}


static const lexmark_cap_t *
lexmark_get_model_capabilities(int model)
{
  int i = model_to_index(model);

  if (i != -1) {
    return &(lexmark_model_capabilities[i]);
  }
#ifdef DEBUG
  stp_erprintf("lexmark: model %d not found in capabilities list.\n",model);
#endif
  return &(lexmark_model_capabilities[0]);
}

/* base density, k_lower_scale, k_upper */
static const double media_parameters[][3] =
{
  { 0.90, 0.25, 0.5 },
  { 1.80, 1.00, 0.9 },
  { 1.80, 1.00, 0.9 },
  { 0.90, 0.25, 0.5 },
  { 0.90, 0.25, 0.5 },
  { 1.40, 0.25, 0.5 },
  { 0.90, 0.25, 0.5 },
  { 1.80, 1.00, 0.9 },
  { 1.80, 1.00, 0.9 },
  { 1.80, 1.00, 0.9 },
  { 1.80, 1.00, 0.9 },
};


static int
lexmark_media_type(const char *name, const lexmark_cap_t * caps)
{
  if (!strcmp(name,_("Plain Paper")))           return  1;
  if (!strcmp(name,_("Transparencies")))        return  2;
  if (!strcmp(name,_("Back Print Film")))       return  3;
  if (!strcmp(name,_("Fabric Sheets")))         return  4;
  if (!strcmp(name,_("Envelope")))              return  5;
  if (!strcmp(name,_("High Resolution Paper"))) return  6;
  if (!strcmp(name,_("T-Shirt Transfers")))     return  7;
  if (!strcmp(name,_("High Gloss Film")))       return  8;
  if (!strcmp(name,_("Glossy Photo Paper")))    return  9;
  if (!strcmp(name,_("Glossy Photo Cards")))    return 10;
  if (!strcmp(name,_("Photo Paper Pro")))       return 11;

#ifdef DEBUG
  stp_erprintf("lexmark: Unknown media type '%s' - reverting to plain\n",name);
#endif
  return 1;
}

static int
lexmark_source_type(const char *name, const lexmark_cap_t * caps)
{
  if (!strcmp(name,_("Auto Sheet Feeder")))    return 4;
  if (!strcmp(name,_("Manual with Pause")))    return 0;
  if (!strcmp(name,_("Manual without Pause"))) return 1;

#ifdef DEBUG
  stp_erprintf("lexmark: Unknown source type '%s' - reverting to auto\n",name);
#endif
  return 4;
}

static int
lexmark_printhead_type(const char *name, const lexmark_cap_t * caps)
{


  if (!strcmp(name,_("Black")))       return 0;
  if (!strcmp(name,_("Color")))       return 1;
  if (!strcmp(name,_("Black/Color"))) return 2;
  if (!strcmp(name,_("Photo/Color"))) return 3;
  if (!strcmp(name,_("Photo")))       return 4;
  if (!strcmp(name,_("Check/Clean nozzles")))       return 5;


#ifdef DEBUG
  stp_erprintf("lexmark: Unknown head combo '%s' - reverting to black\n",name);
#endif
  return 2;
}


/*******************************
lexmark_head_offset
*******************************/
static const stp_lineoff_t *
lexmark_head_offset(int model,                      /* i */
		    int ydpi,                       /* i */
		    int photo,                      /* i do we have photo cartridge ? */
		    stp_lineoff_t *lineoff_buffer)  /* o */
{
  int im = model_to_index(model);
  int i;
  
  if (im != -1) {
#ifdef DEBUG
    stp_erprintf("lexmark_head_offset (%i) (%x)\n",model,lexmark_model_capabilities[im].head_offset);
    stp_erprintf("  sizie %d,  size_v %d, size_v[0] %d\n", sizeof(*lineoff_buffer), sizeof(lineoff_buffer->v), sizeof(lineoff_buffer->v[0]));
#endif
    
    memcpy(lineoff_buffer, lexmark_model_capabilities[im].head_offset, sizeof(*lineoff_buffer));
    for (i=0; i < (sizeof(lineoff_buffer->v) / sizeof(lineoff_buffer->v[0])); i++) {
      lineoff_buffer->v[i] /= (lexmark_model_capabilities[im].y_raster_res / ydpi);
    }

    if (photo) {
      lineoff_buffer->v[0]  = 0;  /* correct the black at photo cartridge */
    }
    return (lineoff_buffer);
  } else {
#ifdef DEBUG
    stp_erprintf("lexmark_head_offset: can't find printer (%i) !!!\n",model);
#endif
    return NULL;
  }
}


#if 0
/*******************************
lexmark_size_type
*******************************/
/* This method is actually not used.
   Is there a possibility to set such value ???????????? */
static unsigned char
lexmark_size_type(const stp_vars_t v, const lexmark_cap_t * caps)
{
  const stp_papersize_t pp = stp_get_papersize_by_size(stp_get_page_height(v),
						       stp_get_page_width(v));
  if (pp)
    {
      const char *name = stp_papersize_get_name(pp);
      /* built ins: */
      if (!strcmp(name,_("A5")))          return 0x01;
      if (!strcmp(name,_("A4")))          return 0x03;
      if (!strcmp(name,_("B5")))          return 0x08;
      if (!strcmp(name,_("Letter")))      return 0x0d;
      if (!strcmp(name,_("Legal")))       return 0x0f;
      if (!strcmp(name,_("Envelope 10"))) return 0x16;
      if (!strcmp(name,_("Envelope DL"))) return 0x17;
      if (!strcmp(name,_("Letter+")))     return 0x2a;
      if (!strcmp(name,_("A4+")))         return 0x2b;
      if (!strcmp(name,_("Lexmark 4x2"))) return 0x2d;
      /* custom */

#ifdef DEBUG
      stp_erprintf("lexmark: Unknown paper size '%s' - using custom\n",name);
    } else {
      stp_erprintf("lexmark: Couldn't look up paper size %dx%d - "
	      "using custom\n",stp_get_page_height(v), stp_get_page_width(v));
#endif
    }
  return 0;
}
#endif

static int lexmark_get_color_nozzles(const stp_printer_t printer)
{
  return 192/3;
}

static int lexmark_get_black_nozzles(const stp_printer_t printer)
{
  return 208;
}


static int lexmark_get_phys_resolution_vertical(const stp_printer_t printer)
{
  return 600;
}

#if 0
static int lexmark_get_phys_resolution_horizontal(const stp_printer_t printer)
{
  return 1200;
}
#endif

static char *
c_strdup(const char *s)
{
  char *ret = stp_malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

typedef struct {
  const char *name;
  int hres;
  int vres;
  int softweave;
  int vertical_passes;
  int vertical_oversample;
  int unidirectional;
  int resid;
} lexmark_res_t;

#define LEXM_RES_COUNT 30
static const lexmark_res_t lexmark_reslist[LEXM_RES_COUNT] =
{
  /*     name                                  hres vres softw v_pass overs unidir resid     */
  { N_ ("300 DPI x 600 DPI"),	       	       300,  600,  0,    1,    1,    0,    DPI300 },
  { N_ ("600 DPI"),		      	       600,  600,  0,    1,    1,    0,    DPI600 },
  { N_ ("600 DPI high quality"),	       600,  600,  1,    4,    1,    0,    DPI600 },
  { N_ ("600 DPI Unidirectional"),	       600,  600,  0,    1,    1,    1,    DPI600 },
  { N_ ("1200 DPI"),		      	      1200, 1200,  1,    1,    1,    0,    DPI1200},
  { N_ ("1200 DPI high quality"),             1200, 1200,  1,    1,    1,    0,    DPI300 },
  { N_ ("1200 DPI highest quality"),          1200, 1200,  1,    1,    1,    0,    DPI600 },
  { N_ ("1200 DPI  Unidirectional"),          1200, 1200,  0,    1,    1,    1,    DPI1200},
  { N_ ("2400 DPI x 1200 DPI"),	      	      2400, 1200,  1,    1,    1,    0,    DPI1200},
  { N_ ("2400 DPI x 1200 DPI high quality"),  2400, 1200,  1,    1,    1,    0,    DPI600 },
  { N_ ("2400 DPI x 1200 DPI highest quality"),2400, 1200,  1,    1,    1,    0,    DPI300},
#ifdef DEBUG
  { N_ ("test print"),                        1200, 1200,  1,    1,    1,    0,    DPItest},
#endif
  { "", 0, 0, 0, 0, 0, -1 }
};


static const lexmark_res_t
*lexmark_get_resolution_para(const stp_printer_t printer,
			    const char *resolution)
{
  const lexmark_cap_t * caps= lexmark_get_model_capabilities(stp_printer_get_model(printer));

  const lexmark_res_t *res = &(lexmark_reslist[0]);
  while (res->hres)
    {
      if (res->vres <= caps->max_ydpi != -1 &&
	  res->hres <= caps->max_xdpi != -1 &&
	  !strcmp(resolution, _(res->name)))
	{
	  return res;
	}
      res++;
    }
  stp_erprintf("lexmark_get_resolution_para: resolution not found (%s)\n", resolution);
  return NULL;
}


static int
lexmark_print_bidirectional(const stp_printer_t printer,
			    const char *resolution)
{
  const lexmark_res_t *res_para = lexmark_get_resolution_para(printer, resolution);
  return !res_para->unidirectional;
}


static void
lexmark_describe_resolution(const stp_printer_t printer,
			    const char *resolution, int *x, int *y)
{
  const lexmark_res_t *res = lexmark_get_resolution_para(printer, resolution);

  if (res)
    {
      *x = res->hres;
      *y = res->vres;
      return;
    }
  *x = -1;
  *y = -1;
}


/*
 * 'lexmark_parameters()' - Return the parameter values for the given parameter.
 */

static char **				/* O - Parameter values */
lexmark_parameters(const stp_printer_t printer,	/* I - Printer model */
		   const char *ppd_file,	/* I - PPD file (not used) */
		   const char *name,		/* I - Name of parameter */
		   int  *count)		/* O - Number of values */
{
  int		i;
  const char **p= 0;
  char **valptrs= 0;

  static const char   *media_types[] =
  {
    (N_ ("Plain Paper")),
    (N_ ("Transparencies")),
    (N_ ("Back Print Film")),
    (N_ ("Fabric Sheets")),
    (N_ ("Envelope")),
    (N_ ("High Resolution Paper")),
    (N_ ("T-Shirt Transfers")),
    (N_ ("High Gloss Film")),
    (N_ ("Glossy Photo Paper")),
    (N_ ("Glossy Photo Cards")),
    (N_ ("Photo Paper Pro"))
  };
  static const char   *media_sources[] =
  {
    (N_ ("Auto Sheet Feeder")),
    (N_ ("Manual with Pause")),
    (N_ ("Manual without Pause")),
  };

  const lexmark_cap_t * caps= lexmark_get_model_capabilities(stp_printer_get_model(printer));

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0) {
    int height_limit, width_limit;
    int papersizes = stp_known_papersizes();
    valptrs = stp_malloc(sizeof(char *) * papersizes);
    *count = 0;

    width_limit = caps->max_width;
    height_limit = caps->max_length;

    for (i = 0; i < papersizes; i++)
      {
	const stp_papersize_t pt = stp_get_papersize_by_index(i);
	if (strlen(stp_papersize_get_name(pt)) > 0 &&
	    stp_papersize_get_width(pt) <= width_limit &&
	    stp_papersize_get_height(pt) <= height_limit)
	  {
	    valptrs[*count] = stp_malloc(strlen(stp_papersize_get_name(pt)) +1);
	    strcpy(valptrs[*count], stp_papersize_get_name(pt));
	    (*count)++;
	  }
      }
    return (valptrs);
  }
  else if (strcmp(name, "Resolution") == 0)
    {
      unsigned int supported_resolutions = caps->supp_res;
      int c= 0;
      const lexmark_res_t *res;
      valptrs = stp_malloc(sizeof(char *) * LEXM_RES_COUNT);

      res = &(lexmark_reslist[0]);
      /* check for allowed resolutions */
      while (res->hres)
	{
	  if ((supported_resolutions & 1) == 1) {
	    valptrs[c++]= c_strdup(_(res->name));
	  }
	  res++;
	  supported_resolutions = supported_resolutions >> 1;
	}
      *count= c;
      return (valptrs);
    }
  else if (strcmp(name, "InkType") == 0)
    {
      int c= 0;
      valptrs = stp_malloc(sizeof(char *) * 5);
      if ((caps->inks & LEXMARK_INK_K))
	valptrs[c++]= c_strdup(_("Black"));
      if ((caps->inks & LEXMARK_INK_CMY))
	valptrs[c++]= c_strdup(_("Color"));
      if ((caps->inks & LEXMARK_INK_CMYK))
	valptrs[c++]= c_strdup(_("Black/Color"));
      if ((caps->inks & LEXMARK_INK_CcMmYK))
	valptrs[c++]= c_strdup(_("Photo/Color"));
      if ((caps->inks & LEXMARK_INK_CcMmYy))
	valptrs[c++]= c_strdup(_("Photo/Color"));
      valptrs[c++]= c_strdup(_("Check/Clean nozzles"));
      *count = c;
      return (valptrs);
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      *count = 11;
      p = media_types;
    }
  else if (strcmp(name, "InputSlot") == 0)
    {
      *count = 3;
      p = media_sources;
    }
  else
    return (NULL);

  valptrs = stp_malloc(*count * sizeof(char *));
  for (i = 0; i < *count; i ++)
    /* translate media_types and media_sources */
    valptrs[i] = c_strdup(_(p[i]));

  return ((char **) valptrs);
}

static const char *
lexmark_default_parameters(const stp_printer_t printer,
			   const char *ppd_file,
			   const char *name)
{
  int		i;

  static const char   *media_types[] =
  {
    (N_ ("Plain Paper")),
    (N_ ("Transparencies")),
    (N_ ("Back Print Film")),
    (N_ ("Fabric Sheets")),
    (N_ ("Envelope")),
    (N_ ("High Resolution Paper")),
    (N_ ("T-Shirt Transfers")),
    (N_ ("High Gloss Film")),
    (N_ ("Glossy Photo Paper")),
    (N_ ("Glossy Photo Cards")),
    (N_ ("Photo Paper Pro"))
  };
  static const char   *media_sources[] =
  {
    (N_ ("Auto Sheet Feeder")),
    (N_ ("Manual with Pause")),
    (N_ ("Manual without Pause")),
  };

  const lexmark_cap_t * caps= lexmark_get_model_capabilities(stp_printer_get_model(printer));

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0)
    {
      int height_limit, width_limit;
      int papersizes = stp_known_papersizes();

      width_limit = caps->max_width;
      height_limit = caps->max_length;

      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t pt = stp_get_papersize_by_index(i);
	  if (strlen(stp_papersize_get_name(pt)) > 0 &&
	      stp_papersize_get_width(pt) <= width_limit &&
	      stp_papersize_get_height(pt) <= height_limit)
	    {
	      return _(stp_papersize_get_name(pt));
	    }
	}
      return NULL;
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      unsigned int supported_resolutions = caps->supp_res;
      const lexmark_res_t *res;

      res = &(lexmark_reslist[0]);
      /* check for allowed resolutions */
      while (res->hres)
	{
	  if ((supported_resolutions & 1) == 1)
	    {
	      return (_(res->name));
	    }
	}
      return NULL;
    }
  else if (strcmp(name, "InkType") == 0)
    {
      if ((caps->inks & LEXMARK_INK_K))
	return (_("Black"));
      if ((caps->inks & LEXMARK_INK_CMY))
	return (_("Color"));
      if ((caps->inks & LEXMARK_INK_CMYK))
	return (_("Black/Color"));
      if ((caps->inks & LEXMARK_INK_CcMmYK))
	return (_("Photo/Color"));
      if ((caps->inks & LEXMARK_INK_CcMmYy))
	return (_("Photo/Color"));
      return NULL;
    }
  else if (strcmp(name, "MediaType") == 0)
    {
      return _(media_types[0]);
    }
  else if (strcmp(name, "InputSlot") == 0)
    {
      return _(media_sources[0]);
    }
  else
    return (NULL);
}


/*
 * 'lexmark_imageable_area()' - Return the imageable area of the page.
 */

static void
lexmark_imageable_area(const stp_printer_t printer,	/* I - Printer model */
		       const stp_vars_t v,   /* I */
		       int  *left,	/* O - Left position in points */
		       int  *right,	/* O - Right position in points */
		       int  *bottom,	/* O - Bottom position in points */
		       int  *top)	/* O - Top position in points */
{
  int	width, length;			/* Size of page */

  const lexmark_cap_t * caps= lexmark_get_model_capabilities(stp_printer_get_model(printer));

  stp_default_media_size(printer, v, &width, &length);

  *left   = caps->border_left;
  *right  = width - caps->border_right;
  *top    = length - caps->border_top;
  *bottom = caps->border_bottom;

  lxm3200_linetoeject = (length * 1200) / 72;
}

static void
lexmark_limit(const stp_printer_t printer,	/* I - Printer model */
	    const stp_vars_t v,  		/* I */
	    int  *width,		/* O - Left position in points */
	    int  *length)		/* O - Top position in points */
{
  const lexmark_cap_t * caps= lexmark_get_model_capabilities(stp_printer_get_model(printer));
  *width =	caps->max_width;
  *length =	caps->max_length;
}



static int
lexmark_init_printer(const stp_vars_t v, const lexmark_cap_t * caps,
		     int output_type, const char *media_str,
		     int print_head,
		     const char *source_str,
		     int xdpi, int ydpi,
		     int page_width, int page_height,
		     int top, int left,
		     int use_dmt)
{

  /* because the details of the header sequence are not known, we simply write it as one image. */
  /* #define LXM_Z52_STARTSIZE 0x30
  / * 600 dpi * /
  unsigned char startHeader_z52[LXM_Z52_STARTSIZE]={0x1B,0x2a,0x81,0x00,0x1c,0x56,0x49,0x00,
					   0x01,0x00,0x58,0x02,0x00,0x00,0xc0,0x12,
					   0xc8,0x19,0x02,0x00,0x68,0x00,0x09,0x00,
					   0x08,0x00,0x08,0x00,0x1b,0x2a,0x07,0x73,
					   0x30,0x1b,0x2a,0x6d,0x00,0x14,0x01,0xf4,
					   0x02,0x00,0x01,0xf0,0x1b,0x2a,0x07,0x63 };

  / * 1200 dpi * /
  unsigned char startHeader_z52[LXM_Z52_STARTSIZE]={0x1b,0x2a,0x81,0x00,0x1c,0x56,0x49,0x00,
					   0x01,0x00,0xb0,0x04,0x00,0x00,0x80,0x25,
					   0x90,0x33,0x01,0x00,0xd0,0x00,0x00,0x00,
					   0x08,0x00,0x08,0x00,0x1b,0x2a,0x07,0x73,
					   0x30,0x1b,0x2a,0x6d,0x00,0x14,0x01,0xf4,
					   0x02,0x00,0x01,0xf0,0x1b,0x2a,0x07,0x63 };*/

#define LXM_Z52_STARTSIZE 0x35
  /* 300 dpi */
  unsigned char startHeader_z52[LXM_Z52_STARTSIZE]={0x1b,0x2a,0x81,0x00,0x1c,0x56,0x49,0x00,
					   0x01,0x00,0x2c,0x01,0x00,0x00,0x60,0x09,
					   0xe4,0x0c,0x01,0x00,0x34,0x00,0x00,0x00,
					   0x08,0x00,0x08,0x00,0x1b,0x2a,0x07,0x76,
					   0x01,0x1b,0x2a,0x07,0x73,0x30,0x1b,0x2a,
					   0x6d,0x00,0x14,0x01,0xf4,0x02,0x00,0x01,
					   0xf0,0x1b,0x2a,0x07,0x63};
  #define ESC2a "\033\052"



#define LXM_3200_STARTSIZE 32

  unsigned char startHeader_3200[LXM_3200_STARTSIZE] =
  {
    0x1b, 0x2a, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1b, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33,
    0x1b, 0x30, 0x80, 0x0C, 0x02, 0x00, 0x00, 0xbe,
    0x1b, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21
  };

  /* write init sequence */
  switch(caps->model)
	{
		case m_z52:
			stp_zfwrite((const char *) startHeader_z52,
				    LXM_Z52_STARTSIZE,1,v);
			break;

		case m_3200:
			stp_zfwrite((const char *) startHeader_3200,
				    LXM_3200_STARTSIZE, 1, v);
			break;

		default:
			stp_erprintf("Unknown printer !! %i\n", caps->model);
			return 0;
  }

  if (output_type==OUTPUT_GRAY || output_type == OUTPUT_MONOCHROME) {
  }

  if (print_head==0) {
  }




  /*
#ifdef DEBUG
  stp_erprintf("lexmark: printable size = %dx%d (%dx%d) %02x%02x %02x%02x\n",
	  page_width,page_height,printable_width,printable_length,
	  arg_70_1,arg_70_2,arg_70_3,arg_70_4);
#endif
  */
  return 1;
}

static void lexmark_deinit_printer(const stp_vars_t v, const lexmark_cap_t * caps)
{

	switch(caps->model)	{
		case m_z52:
		{
			char buffer[4];

			memcpy(buffer, ESC2a, 2);
			buffer[2] = 0x7;
			buffer[3] = 0x65;

			/* eject page */
			stp_zfwrite(buffer, 4, 1, v);
		}
		break;

		case m_3200:
		{
		  unsigned char buffer[24] =
		  {
		    0x1b, 0x22, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x1b, 0x31, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
		    0x1b, 0x33, 0x10, 0x00, 0x00, 0x00, 0x00, 0x33
		  };

#ifdef DEBUG
			printf("Headpos: %d\n", lxm3200_headpos);
#endif

			lxm3200_linetoeject += 2400;
			buffer[3] = lxm3200_linetoeject >> 8;
			buffer[4] = lxm3200_linetoeject & 0xff;
			buffer[7] = lexmark_calc_3200_checksum(&buffer[0]);
			buffer[11] = lxm3200_headpos >> 8;
			buffer[12] = lxm3200_headpos & 0xff;
			buffer[15] = lexmark_calc_3200_checksum(&buffer[8]);

			stp_zfwrite((const char *)buffer, 24, 1, v);
		}
		break;

		case m_lex7500:
			break;
	}

}


/* paper_shift() -- shift paper in printer -- units are unknown :-)
 */
static void paper_shift(const stp_vars_t v, int offset, const lexmark_cap_t * caps)
{
	switch(caps->model)	{
		case m_z52:
		{
			unsigned char buf[5] = {0x1b, 0x2a, 0x3, 0x0, 0x0};
			if(offset == 0)return;
			buf[3] = (unsigned char)(offset >> 8);
			buf[4] = (unsigned char)(offset & 0xFF);
			stp_zfwrite((const char *)buf, 1, 5, v);
		}
		break;

		case m_3200:
		{
			unsigned char buf[8] = {0x1b, 0x23, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00};
			if(offset == 0)return;
			lxm3200_linetoeject -= offset;
			buf[3] = (unsigned char)(offset >> 8);
			buf[4] = (unsigned char)(offset & 0xff);
			buf[7] = lexmark_calc_3200_checksum(buf);
			stp_zfwrite((const char *)buf, 1, 8, v);
		}
		break;

		case m_lex7500:
			break;
	}

#ifdef DEBUG
	printf("Lines to eject: %d\n", lxm3200_linetoeject);
#endif
}


/*
 *  'alloc_buffer()' allocates buffer and fills it with 0
 */
static unsigned char *
lexmark_alloc_buffer(int size)
{
  unsigned char *buf= stp_malloc(size);
  if (buf) memset(buf,0,size);
  return buf;
}

/*
 * 'advance_buffer()' - Move (num) lines of length (len) down one line
 *                      and sets first line to 0s
 *                      accepts NULL pointers as buf
 *                  !!! buf must contain more than (num) lines !!!
 *                      also sets first line to 0s if num<1
 */
#if 0
static void
lexmark_advance_buffer(unsigned char *buf, int len, int num)
{
  if (!buf || !len) return;
  if (num>0) memmove(buf+len,buf,len*num);
  memset(buf,0,len);
}
#endif


static int
clean_color(unsigned char *line, int len)
{
  return 0;
}





/**********************************************************
 * lexmark_print() - Print an image to a LEXMARK printer.
 **********************************************************/
/* This method should not be printer dependent (mybe it is because of nozzle count and other things) */
/* The method will set the printing method depending on the selected printer.
   It will define the colors to be used and the resolution.
   Additionally the pass_length will be defined.
   The method lexmark_write() is responsible to handle the received lines
   in a correct way. 
*/
static void
lexmark_print(const stp_printer_t printer,		/* I - Model */
	      stp_image_t *image,		/* I - Image to print */
	      const stp_vars_t    v)
{
  /*const int VERTSIZE=192;*/
  int i;
  const unsigned char *cmap = stp_get_cmap(v);
  int		model = stp_printer_get_model(printer);
  const char	*resolution = stp_get_resolution(v);
  const char	*media_type = stp_get_media_type(v);
  const char	*media_source = stp_get_media_source(v);
  int 		output_type = stp_get_output_type(v);
  int		orientation = stp_get_orientation(v);
  const char	*ink_type = stp_get_ink_type(v);
  double 	scaling = stp_get_scaling(v);
  int		top = stp_get_top(v);
  int		left = stp_get_left(v);
  int		y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  int		n;		/* Output number */
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned char	*in;		/* Input pixels */
  int		page_left,	/* Left margin of page */
    page_right,	/* Right margin of page */
    page_top,	/* Top of page */
    page_bottom,	/* Bottom of page */
    page_width,	/* Width of page */
    page_height,	/* Length of page */
    page_true_height,	/* True length of page */
    out_width,	/* Width of image on page in pixles */
    out_height,	/* Length of image on page */
    out_bpp,	/* Output bytes per pixel */
    length,		/* Length of raster data in bytes*/
    buf_length,     /* Length of raster data buffer (dmt) */
    errdiv,		/* Error dividend */
    errmod,		/* Error modulus */
    errval,		/* Current error value */
    errline,	/* Current raster line */
    errlast;	/* Last raster line loaded */
  stp_convert_t	colorfunc = 0;	  /* Color conversion function... */
  int           zero_mask;
  int           image_height,
                image_width,
                image_bpp;
  int           use_dmt = 0;
  void *	dither;
  stp_vars_t	nv = stp_allocate_copy(v);
  int pass_length=0;              /* count of inkjets for one pass */
  int add_top_offset=0;              /* additional top offset */
  const lexmark_cap_t * caps= lexmark_get_model_capabilities(model);
  int printhead= lexmark_printhead_type(ink_type,caps);
  int printMode = 0;
  int media, source;
  /* Lexmark do not have differnet pixel sizes. We have to correct the density according the print resolution. */
  double  densityDivisor;            /* This parameter is will adapt the density according the resolution */
  double k_lower, k_upper;
  int  physical_xdpi = 0;
  int  physical_ydpi = 0;

  /* weave parameters */
  stp_linebufs_t cols;
  int  nozzle_separation;
  int  horizontal_passes;
  int  ncolors;
  lexm_privdata_weave privdata;
  void *	weave = NULL;

  stp_lineoff_t lineoff_buffer;  /* holds the line offsets of each color */
  int doTestPrint = 0;
#ifdef DEBUG
  testdata td;
#endif

  const lexmark_res_t *res_para_ptr = lexmark_get_resolution_para(printer, resolution);

  if (!stp_get_verified(nv))
    {
      stp_eprintf(nv, "Print options not verified; cannot print.\n");
      return;
    }

  /*
  * Setup a read-only pixel region for the entire image...
  */

  image->init(image);
  image_height = image->height(image);
  image_width = image->width(image);
  image_bpp = image->bpp(image);


  media= lexmark_media_type(media_type,caps);
  source= lexmark_source_type(media_source,caps);

  /* force grayscale if image is grayscale
   *                 or single black cartridge installed
   */

  if ((printhead == 0 || caps->inks == LEXMARK_INK_K) &&
      output_type != OUTPUT_MONOCHROME)
    {
      output_type = OUTPUT_GRAY;
    }
  stp_set_output_color_model(nv, COLOR_MODEL_CMY);

  /*
   * Choose the correct color conversion function...
   */

  colorfunc = stp_choose_colorfunc(output_type, image_bpp, cmap, &out_bpp, nv);


  if (output_type == OUTPUT_GRAY || output_type == OUTPUT_MONOCHROME) {
    printMode |= COLOR_MODE_K;
    ncolors = 1;
    pass_length=208;
    add_top_offset =  caps->v_offset_black_color; /* add offset to the first black jet from color jet */
    lxm_nozzles_used = lexmark_get_black_nozzles(printer);
  } else {
    add_top_offset = 0; /* we have color where first jet is on position 0 */

    lxm_nozzles_used = lexmark_get_color_nozzles(printer);

    /* color mode */
    printMode |= COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M;
    pass_length=192/3;
    ncolors = 3;

    if (printhead==2 && (caps->inks & LEXMARK_INK_BLACK_MASK)) {
      printMode |= COLOR_MODE_K;
      ncolors += 1;
    }
    if ((printhead==3 || printhead==5) && (caps->inks & (LEXMARK_INK_PHOTO_MASK))) {
      printMode |= COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M | COLOR_MODE_LC | COLOR_MODE_LM | COLOR_MODE_K;
      ncolors += 3;
#ifdef DEBUG
      stp_erprintf("lexmark: print in photo mode !!.\n");
#endif

    }
  }


  /*
  * Figure out the output resolution...
  */


  lexmark_describe_resolution(printer,
			      resolution, &xdpi,&ydpi);
#ifdef DEBUG
  stp_erprintf("lexmark: resolution=%dx%d\n",xdpi,ydpi);
#endif

  switch (res_para_ptr->resid) {
  case DPI300:
    physical_xdpi = 300;
    physical_ydpi = lexmark_get_phys_resolution_vertical(printer);
    break;
  case DPI600:
    physical_xdpi = 600;
    physical_ydpi = lexmark_get_phys_resolution_vertical(printer);
    break;
  case DPI1200:
  case DPItest:
    physical_xdpi = 1200;
    physical_ydpi = lexmark_get_phys_resolution_vertical(printer);
    break;
  default:
    return;
    break;
  }
  /* adapt the density */
  densityDivisor = ((xdpi / 300)*(ydpi/ 600)); 

#ifdef DEBUG
  if (res_para_ptr->resid == DPItest) {
    stp_erprintf("Start test print1\n");
    doTestPrint = 1;
  }
#endif

  if ((printMode & COLOR_MODE_PHOTO) == COLOR_MODE_PHOTO) {
    /* in case of photo mode we have to go a bit ligther */
densityDivisor /= 1.2;
  }

  nozzle_separation = ydpi / physical_ydpi;

  horizontal_passes = xdpi / physical_xdpi;
#ifdef DEBUG
  stp_erprintf("lexmark: horizontal_passes %i, xdpi %i, physical_xdpi %i\n",
	       horizontal_passes, xdpi, physical_xdpi);
#endif




  if (!strcmp(resolution+(strlen(resolution)-3),"DMT") &&
      (caps->features & LEXMARK_CAP_DMT) &&
      stp_get_output_type(nv) != OUTPUT_MONOCHROME &&
      output_type != OUTPUT_MONOCHROME) {
    use_dmt= 1;
#ifdef DEBUG
    stp_erprintf("lexmark: using drop modulation technology\n");
#endif
  }

  /*
  * Compute the output size...
  */

  lexmark_imageable_area(printer, nv, &page_left, &page_right,
			 &page_bottom, &page_top);

  stp_compute_page_parameters(page_right, page_left, page_top, page_bottom,
			  scaling, image_width, image_height, image,
			  &orientation, &page_width, &page_height,
			  &out_width, &out_height, &left, &top);

#ifdef DEBUG
  printf("page_right %d, page_left %d, page_top %d, page_bottom %d, left %d, top %d\n",page_right, page_left, page_top, page_bottom,left, top);
#endif

  /*
   * Recompute the image length and width.  If the image has been
   * rotated, these will change from previously.
   */
  image_height = image->height(image);
  image_width = image->width(image);

  stp_default_media_size(printer, nv, &n, &page_true_height);


  image->progress_init(image);


  if (!lexmark_init_printer(nv, caps, output_type, media_type,
			    printhead, media_source,
			    xdpi, ydpi, page_width, page_height,
			    top,left,use_dmt))
    return;

  /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

#ifdef DEBUG
  /*  dbgfile = lex_show_init(out_width, out_height);*/
  dbgfile = lex_show_init(out_width, out_height-100);

  stp_erprintf("border: left %ld, x_raster_res %d, offser_left %ld\n", left, caps->x_raster_res, caps->offset_left_border);
#endif

  left = ((caps->x_raster_res * left) / 72) + caps->offset_left_border;

#ifdef DEBUG
  stp_erprintf("border: left %d\n", left);
#endif



#ifdef DEBUG
  if (doTestPrint == 1) {
    stp_erprintf("Start test print\n");
    testprint(&td);
    out_width = td.x;
    out_height = td.y;
    if (td.cols != 7) { 
    printMode = COLOR_MODE_K | COLOR_MODE_M | COLOR_MODE_C | COLOR_MODE_Y;
    } else {
    printMode = COLOR_MODE_K | COLOR_MODE_M | COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_LM | COLOR_MODE_LC;
    }
  }
#endif

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;



  if (use_dmt) {
    /*    buf_length= length*2; */
    buf_length= length;
  } else {
    buf_length= length;
  }

#ifdef DEBUG
  stp_erprintf("lexmark: buflength is %d!\n",buf_length);
#endif


  /* Now we know the color which are used, let's get the memory for every color image */
  cols.p.k = NULL;
  cols.p.c = NULL;
  cols.p.y = NULL;
  cols.p.m = NULL;
  cols.p.C = NULL;
  cols.p.M = NULL;
  cols.p.Y = NULL;


  if ((printMode & COLOR_MODE_C) == COLOR_MODE_C) {
    cols.p.c = lexmark_alloc_buffer(buf_length+10);
  }
  if ((printMode & COLOR_MODE_Y) == COLOR_MODE_Y) {
    cols.p.y = lexmark_alloc_buffer(buf_length+10);
  }
  if ((printMode & COLOR_MODE_M) == COLOR_MODE_M) {
    cols.p.m = lexmark_alloc_buffer(buf_length+10);
  }
  if ((printMode & COLOR_MODE_K) == COLOR_MODE_K) {
    cols.p.k = lexmark_alloc_buffer(buf_length+10);
  }
  if ((printMode & COLOR_MODE_LC) == COLOR_MODE_LC) {
    cols.p.C = lexmark_alloc_buffer(buf_length+10);
  }
  if ((printMode & COLOR_MODE_LY) == COLOR_MODE_LY) {
    cols.p.Y = lexmark_alloc_buffer(buf_length+10);
  }
  if ((printMode & COLOR_MODE_LM) == COLOR_MODE_LM) {
    cols.p.M = lexmark_alloc_buffer(buf_length+10);
  }


#ifdef DEBUG
  stp_erprintf("lexmark: driver will use colors ");
  if (cols.p.c)     stp_erputc('c');
  if (cols.p.C)     stp_erputc('C');
  if (cols.p.m)     stp_erputc('m');
  if (cols.p.M)     stp_erputc('M');
  if (cols.p.y)     stp_erputc('y');
  if (cols.p.Y)     stp_erputc('Y');
  if (cols.p.k)     stp_erputc('k');
  stp_erprintf("\n");
#endif


  /* initialize soft weaveing */

  privdata.bidirectional = lexmark_print_bidirectional(printer, resolution);
  privdata.outbuf = stp_malloc((((((pass_length/8)*11))+40) * out_width)+2000);
  stp_set_driver_data(nv, &privdata);
  /*  lxm_nozzles_used = 1;*/
  weave = stp_initialize_weave(lxm_nozzles_used,                   /* jets */
			       nozzle_separation,                  /* separation */
			       horizontal_passes,                  /* h overample */
			       res_para_ptr->vertical_passes,      /* v passes */
			       res_para_ptr->vertical_oversample,  /* v oversample */
			       ncolors,                            /* colors */
			       1,                                  /* bits/pixel */
			       out_width,                          /* line width */
			       out_height, 0,
			       ((top * ydpi) / 72)+(((caps->offset_top_border+add_top_offset)*ydpi)
						     /caps->y_raster_res),
			       (page_height * ydpi) / 72,
			       1, /* weave_strategy */
			       (int *)lexmark_head_offset(model, ydpi, (ncolors==6), &lineoff_buffer),
			       nv, flush_pass,
			       stp_fill_uncompressed,  /* fill_start */
			       stp_pack_uncompressed,  /* pack */
			       stp_compute_uncompressed_linewidth);  /* compute_linewidth */
  




#ifdef DEBUG
  stp_erprintf("density is %f\n",stp_get_density(nv));
#endif

#ifdef DEBUG
  stp_erprintf("density is %f and will be changed to %f  (%f)\n",stp_get_density(nv), stp_get_density(nv)/densityDivisor, densityDivisor);
#endif
  /* Lexmark do not have differnet pixel sizes. We have to correct the density according the print resolution. */
  stp_set_density(nv, stp_get_density(nv) / densityDivisor);

  if(media >= 1 && media <= 11)
    stp_set_density(nv, stp_get_density(nv) * media_parameters[media-1][0]);
  else				/* Can't find paper type? Assume plain */
    stp_set_density(nv, stp_get_density(nv) * .5);
  if (stp_get_density(nv) > 1.0)
    stp_set_density(nv, 1.0);

  stp_compute_lut(nv, 256);

#ifdef DEBUG
  stp_erprintf("density is %f\n",stp_get_density(nv));
#endif

  if (xdpi > ydpi)
    dither = stp_init_dither(image_width, out_width, 1, xdpi / ydpi, nv);
  else
    dither = stp_init_dither(image_width, out_width, ydpi / xdpi, 1, nv);

  for (i = 0; i <= NCOLORS; i++)
    stp_dither_set_black_level(dither, i, 1.0);



  /*
  if(printMode & (COLOR_MODE_LM | COLOR_MODE_LC | COLOR_MODE_LY))
    k_lower = .4 / bits + .1;
  else
    k_lower = .25 / bits;
	*/

  k_lower = .8 / ((1 << (use_dmt+1)) - 1);
  if(media >= 1 && media <= 11)
    {
      k_lower *= media_parameters[media-1][1];
      k_upper = media_parameters[media-1][2];
    }
  else
    {
      k_lower *= .5;
      k_upper = .5;
    }
  stp_dither_set_black_lower(dither, k_lower);
  stp_dither_set_black_upper(dither, k_upper);

	/*
	  stp_dither_set_black_lower(dither, .8 / ((1 << (use_dmt+1)) - 1));*/
  /*stp_dither_set_black_levels(dither, 0.5, 0.5, 0.5);
    stp_dither_set_black_lower(dither, 0.4);*/
  /*
    if (use_glossy_film)
  */
  stp_dither_set_black_upper(dither, .8);

  if (!use_dmt) {
    if (cols.p.C)
      stp_dither_set_light_ink(dither, ECOLOR_C, .3333, stp_get_density(nv));
    if (cols.p.M)
      stp_dither_set_light_ink(dither, ECOLOR_M, .3333, stp_get_density(nv));
    if (cols.p.Y)
      stp_dither_set_light_ink(dither, ECOLOR_Y, .3333, stp_get_density(nv));
  }

  switch (stp_get_image_type(nv))
    {
    case IMAGE_LINE_ART:
      stp_dither_set_ink_spread(dither, 19);
      break;
    case IMAGE_SOLID_TONE:
      stp_dither_set_ink_spread(dither, 15);
      break;
    case IMAGE_CONTINUOUS:
      stp_dither_set_ink_spread(dither, 14);
      break;
    }
  stp_dither_set_density(dither, stp_get_density(nv));

  /*
   * Output the page...
  */


  in  = stp_malloc(image_width * image_bpp);
  out = stp_malloc(image_width * out_bpp * 2);

  /* calculate the memory we need for one line of the printer image (hopefully we are right) */
#ifdef DEBUG
  stp_erprintf("---------- buffer mem size = %d\n", (((((pass_length/8)*11)/10)+40) * out_width)+200);
#endif

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;




  
  for (y = 0; y < out_height; y ++)   /* go through every pixle line of image */
    {
      int duplicate_line = 1;
      
#ifdef DEBUGyy
      stp_erprintf("print y %i\n", y);
#endif
      
      if ((y & 63) == 0)
	image->note_progress(image, y, out_height);
      
      if (errline != errlast)
	{
	  errlast = errline;
	  duplicate_line = 0;
	  if (image->get_row(image, in, errline) != STP_IMAGE_OK)
	    break;
	  /*	  printf("errline %d ,   image height %d\n", errline, image_height);*/
#if 1
	  (*colorfunc)(nv, in, out, &zero_mask, image_width, image_bpp, cmap,
		       hue_adjustment, lum_adjustment, NULL);
#else
	  (*colorfunc)(nv, in, out, &zero_mask, image_width, image_bpp, cmap,
		       NULL, NULL, NULL);
#endif
	}
      /*      printf("Let's dither   %d    %d  %d\n", ((y)), buf_length, length);*/
      if (doTestPrint == 0) {
	stp_dither(out, y, dither, cols.p.c, cols.p.C, cols.p.m, cols.p.M,
		   cols.p.y, cols.p.Y, cols.p.k, duplicate_line, zero_mask);
      } else {
#ifdef DEBUG
	readtestprintline(&td, &cols);
#endif
      }
      
      clean_color(cols.p.c, length);
      clean_color(cols.p.m, length);
      clean_color(cols.p.y, length);
      
#ifdef DEBUGxx
      printf("Let's go lex_show_dither\n");
      lex_show_dither(dbgfile, cols.p.y, cols.p.c, cols.p.m, cols.p.Y, cols.p.C, cols.p.M, , out_width);
#endif
      
#ifdef DEBUGyy
            stp_erprintf("Let's go stp_write_weave\n");
	      stp_erprintf("length %d\n", length);
#endif

      stp_write_weave(weave, length, ydpi, model, out_width, left,
		      xdpi, physical_xdpi, (const unsigned char **)cols.v);
      
      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
	{
	  errval -= out_height;
	  errline ++;
	}
      
    }
  image->progress_conclude(image);

  stp_flush_all(weave, model, out_width, left,
		      ydpi, xdpi, physical_xdpi);

  if (doTestPrint == 0) {
    stp_free_dither(dither);
  }
  


  /*
  * Cleanup...
  */

  stp_free_lut(nv);
  stp_free(in);
  stp_free(out);
  if (privdata.outbuf != NULL) {
    stp_free(privdata.outbuf);/* !!!!!!!!!!!!!! */
  }

  if (cols.p.k != NULL) stp_free(cols.p.k);
  if (cols.p.c != NULL) stp_free(cols.p.c);
  if (cols.p.m != NULL) stp_free(cols.p.m);
  if (cols.p.y != NULL) stp_free(cols.p.y);
  if (cols.p.C != NULL) stp_free(cols.p.C);
  if (cols.p.M != NULL) stp_free(cols.p.M);
  if (cols.p.Y != NULL) stp_free(cols.p.Y);

#ifdef DEBUG
  lex_show_deinit(dbgfile);
#endif

  lexmark_deinit_printer(v, caps);
  stp_free_vars(nv); 
}

const stp_printfuncs_t stp_lexmark_printfuncs =
{
  lexmark_parameters,
  stp_default_media_size,
  lexmark_imageable_area,
  lexmark_limit,
  lexmark_print,
  lexmark_default_parameters,
  lexmark_describe_resolution,
  stp_verify_printer_params
};



/* lexmark_init_line
   This method is printer type dependent code.

   This method initializes the line to be printed. It will set
   the printer specific initialization which has to be done bofor
   the pixels of the image could be printed.
*/
static unsigned char *
lexmark_init_line(int mode, unsigned char *prnBuf, 
		  int offset,    /* offset from left in 1/"x_raster_res" DIP (printer resolution)*/
		  int width, int direction,
		  const lexmark_cap_t *   caps	        /* I - Printer model */
		  )
{
  int pos1 = 0;
  int pos2 = 0;
  int abspos, disp;


  /*  printf("#### width %d, length %d, pass_length %d\n", width, length, pass_length);*/
  /* first, we wirte the line header */
  switch(caps->model)  {
  case m_z52:
    memcpy(prnBuf, outbufHeader_z52, LXM_Z52_HEADERSIZE);

    /* K could only be present if black is printed only. */
    if ((mode & COLOR_MODE_K) || (mode & (COLOR_MODE_K | COLOR_MODE_LC | COLOR_MODE_LM))) {
#ifdef DEBUG
      stp_erprintf("set  photo/black catridge \n");
#endif
      prnBuf[LX_Z52_COLOR_MODE_POS] = LX_Z52_BLACK_PRINT;

      if (direction) {
      } else {
	offset += caps->direction_offset_color;
      }
    } else {
#ifdef DEBUG
      stp_erprintf("set color catridge \n");
#endif
      prnBuf[LX_Z52_COLOR_MODE_POS] = LX_Z52_COLOR_PRINT;

      if (direction) {
	offset += caps->h_offset_black_color;
      } else {
	offset += caps->h_offset_black_color;
	offset += caps->direction_offset_color;
      }
    }

    switch (mode & PRINT_MODE_MASK) {
    case PRINT_MODE_300:
      prnBuf[LX_Z52_RESOLUTION_POS] = LX_Z52_300_DPI;
      break;
    case PRINT_MODE_600:
      prnBuf[LX_Z52_RESOLUTION_POS] = LX_Z52_600_DPI;
      break;
    case PRINT_MODE_1200:
      prnBuf[LX_Z52_RESOLUTION_POS] = LX_Z52_1200_DPI;
      break;
    case PRINT_MODE_2400:
      prnBuf[LX_Z52_RESOLUTION_POS] = LX_Z52_2400_DPI;
      break;
    }


    if (direction) {
      prnBuf[LX_Z52_PRINT_DIRECTION_POS] = 1;
    } else {
      prnBuf[LX_Z52_PRINT_DIRECTION_POS] = 2;
    }


    /* set package count */
    prnBuf[13] =(unsigned char)((width) >> 8);
    prnBuf[14] =(unsigned char)((width) & 0xFF);
    /* set horizotal offset */
    prnBuf[15] =(unsigned char)(offset >> 8);
    prnBuf[16] =(unsigned char)(offset & 0xFF);



    return prnBuf+LXM_Z52_HEADERSIZE;  /* return the position where the pixels have to be written */
    break;
    case m_3200:
      memcpy(prnBuf, outbufHeader_3200, LXM_3200_HEADERSIZE);

      offset = (offset - 60) * 4;

      /* K could only be present if black is printed only. */
      if((mode & COLOR_MODE_K) ||
	 (mode & (COLOR_MODE_K | COLOR_MODE_LC | COLOR_MODE_LM)))
	{
	  disp = LXM3200_LEFTOFFS;
	  prnBuf[2] = 0x00;
	}
      else
	{
	  disp = LXM3200_RIGHTOFFS;
	  prnBuf[2] = 0x80;
	}

      if(lxm_nozzles_used == 208)
	{
	  prnBuf[2] |= 0x10;
	}

      switch(mode & PRINT_MODE_MASK) 	{
	case PRINT_MODE_300:
	  prnBuf[2] |= 0x20;
	  pos1 = offset + disp;
	  pos2 = offset + (width * 4) + disp;
	  break;

	case PRINT_MODE_600:
	  prnBuf[2] |= 0x00;
	  pos1 = offset + disp;
	  pos2 = offset + (width * 2) + disp;
	  break;

	case PRINT_MODE_1200:
	  prnBuf[2] |= 0x40;
	  pos1 = offset + disp;
	  pos2 = (offset + width) + disp;
	  break;
	}

      if(direction)
	prnBuf[2] |= 0x01;
      else
	prnBuf[2] |= 0x00;

      /* set package count */
      prnBuf[3] = (unsigned char)((width) >> 8);
      prnBuf[4] = (unsigned char)((width) & 0xff);

      /* set horizontal offset */
      prnBuf[21] = (unsigned char)(pos1 >> 8);
      prnBuf[22] = (unsigned char)(pos1 & 0xFF);

      abspos = ((((pos2 - 3600) >> 3) & 0xfff0) + 9);
      prnBuf[5] = (abspos-lxm3200_headpos) >> 8;
      prnBuf[6] = (abspos-lxm3200_headpos) & 0xff;

      lxm3200_headpos = abspos;

      if(LXM3200_RIGHTOFFS > 4816)
	abspos = (((LXM3200_RIGHTOFFS - 4800) >> 3) & 0xfff0);
      else
	abspos = (((LXM3200_RIGHTOFFS - 3600) >> 3) & 0xfff0);

      prnBuf[11] = (lxm3200_headpos-abspos) >> 8;
      prnBuf[12] = (lxm3200_headpos-abspos) & 0xff;

      lxm3200_headpos = abspos;

      prnBuf[7] = (unsigned char)lexmark_calc_3200_checksum(&prnBuf[0]);
      prnBuf[15] = (unsigned char)lexmark_calc_3200_checksum(&prnBuf[8]);
      prnBuf[23] = (unsigned char)lexmark_calc_3200_checksum(&prnBuf[16]);

      /* return the position where the pixels have to be written */
      return prnBuf + LXM_3200_HEADERSIZE;
      break;

  case m_lex7500:
    stp_erprintf("Lexmark 7500 not supported !\n");
    return NULL;
    break;
  }
  return NULL;
}


typedef struct Lexmark_head_colors {
  int v_start;
  unsigned char *line;
  int head_nozzle_start;
  int head_nozzle_end;
  int used_jets;
} Lexmark_head_colors;

/* lexmark_write
   This method is has NO printer type dependent code.
   This method writes a single line of the print. The line consits of "pass_length"
   pixel lines (pixels, which could be printed with one pass by the printer.
*/


static int
lexmark_write(const stp_vars_t v,		/* I - Print file or command */
	      unsigned char *prnBuf,      /* mem block to buffer output */
	      int *paperShift,
	      int direction,
	      int pass_length,       /* num of inks to print */
	      const lexmark_cap_t *   caps,	        /* I - Printer model */
	      int xdpi,
	      int yCount,
	      Lexmark_head_colors *head_colors,
	      int           length,	/* I - Length of bitmap data in bytes */
	      int           mode,	/* I - Which color */
	      int           ydpi,		/* I - Vertical resolution */
	      int           width,	/* I - Printed width in pixles */
	      int           offset, 	/* I - Offset from left side in lexmark_cap_t.x_raster_res DPI */
	      int           dmt)
{
  unsigned char *tbits=NULL, *p=NULL;
  int clen;
  int x;  /* actual vertical position */
  int y;  /* actual horizontal position */
  int dy; /* horiz. inkjet posintion */
  int x1;
  unsigned short pixelline;  /* byte to be written */
  unsigned int valid_bytes; /* bit list which tells the present bytes */
  int xStart=0; /* count start for horizontal line */
  int xEnd=0;
  int xIter=0;  /* count direction for horizontal line */
  int anyCol=0;
  int colIndex;
  int rwidth; /* real with used at printing (includes shift between even & odd nozzles) */
#ifdef DEBUG
  /* stp_erprintf("<%c>",("CMYKcmy"[coloridx])); */
  stp_erprintf("pass length %d\n", pass_length);
#endif


  /* first, we check the length of the line an cut it if necessary. */
  if ((((width*caps->x_raster_res)/xdpi)+offset) > ((caps->max_width*caps->x_raster_res)/72)) {
    /* line too long !! Cut the line */
#ifdef DEBUG
   stp_erprintf("!! Line too long !! reduce it from %d", width);
#endif
    width = ((((caps->max_width*caps->x_raster_res)/72) - offset)*xdpi)/caps->x_raster_res;
#ifdef DEBUG
   stp_erprintf(" down to %d\n", width);
#endif
  }


  /* we have to write the initial sequence for a line */

#ifdef DEBUG
  stp_erprintf("lexmark: printer line initialized.\n");
#endif

  if (direction) {
    /* left to right */
    xStart = -get_lr_shift(mode);
    xEnd = width-1;
    xIter = 1;
    rwidth = xEnd - xStart;
  } else {
    /* right to left ! */
    xStart = width-1;
    xEnd = -get_lr_shift(mode);
    rwidth = xStart - xEnd;
    xIter = -1;
  }

  p = lexmark_init_line(mode, prnBuf, offset, rwidth,
			direction,  /* direction */
			caps);


#ifdef DEBUG
  stp_erprintf("lexmark: xStart %d, xEnd %d, xIter %d.\n", xStart, xEnd, xIter);
#endif

  /* now we can start to write the pixels */
  yCount = 2;


  for (x=xStart; x != xEnd; x+=xIter) {
    int  anyDots=0; /* tells us if there was any dot to print */

       switch(caps->model)	{
	case m_z52:
	  tbits = p;
	  *(p++) = 0x3F;
	  tbits[1] = 0; /* here will be nice bitmap */
	  p++;
	  break;

	case m_3200:
	  tbits = p;
	  p += 4;
	  break;

	case m_lex7500:
	  break;
	}


    pixelline =0;     /* here we store 16 pixels */
    valid_bytes = 0;  /* for every valid word (16 bits) a corresponding bit will be set to 1. */

    anyDots =0;
    x1 = x+get_lr_shift(mode);

    for (colIndex=0; colIndex < 3; colIndex++) {
      for (dy=head_colors[colIndex].head_nozzle_start,y=head_colors[colIndex].v_start*yCount;
	   (dy < head_colors[colIndex].head_nozzle_end);
	   y+=yCount, dy++) { /* we start counting with 1 !!!! */
	if (head_colors[colIndex].line != NULL) {
	  pixelline = pixelline << 1;
	  if ((x >= 0) && 
	      ((dy - head_colors[colIndex].head_nozzle_start) < (head_colors[colIndex].used_jets/2)))
	    pixelline = pixelline | ((head_colors[colIndex].line[(y*length)+(x/8)] >> (7-(x%8))) & 0x1);
	  pixelline = pixelline << 1;
	  if ((x1 < width) &&
	      (((dy - head_colors[colIndex].head_nozzle_start)+1) < (head_colors[colIndex].used_jets/2)))
	    pixelline = pixelline | ((head_colors[colIndex].line[(((yCount>>1)+y)*length)+ (x1/8)] >> (7-(x1%8))) & 0x1);
	  
	} else {
	  pixelline = pixelline << 2;
	}
	switch(caps->model)		{
	case m_z52:
	  if ((dy%8) == 7) {
	    /* we have two bytes, write them */
	    anyDots |= pixelline;
	    if (pixelline) {
	      /* we have some dots */
	      valid_bytes = valid_bytes >> 1;
	      *((p++)) = (unsigned char)(pixelline >> 8);
	      *((p++)) = (unsigned char)(pixelline & 0xff);
	    } else {
	      /* there are no dots ! */
	      valid_bytes = valid_bytes >> 1;
	      valid_bytes |= 0x1000;
	    }
	    pixelline =0;
	  }
	  break;

	case m_3200:
	  if((dy % 4) == 3)
	    {
	      anyDots |= pixelline;
	      valid_bytes <<= 1;

	      if(pixelline)
		*(p++) = (unsigned char)(pixelline & 0xff);
	      else
		valid_bytes |= 0x01;

	      pixelline = 0;
	    }
	  break;

	case m_lex7500:
	  break;
	}
      }
    }

    switch(caps->model)	{
    case m_z52:
      if (pass_length != 208) {
	valid_bytes = valid_bytes >> 1;
	valid_bytes |= 0x1000;
      }
      tbits[0] = 0x20 | ((unsigned char)((valid_bytes >> 8) & 0x1f));
      tbits[1] = (unsigned char)(valid_bytes & 0xff);
      break;

    case m_3200:
      tbits[0] = 0x80 | ((unsigned char)((valid_bytes >> 24) & 0x1f));
      tbits[1] = (unsigned char)((valid_bytes >> 16) & 0xff);
      tbits[2] = (unsigned char)((valid_bytes >> 8) & 0xff);
      tbits[3] = (unsigned char)(valid_bytes & 0xff);
      break;

    case m_lex7500:
      break;
    }


    if (anyDots) {
      anyCol = 1;
    } else {
      /* there are no dots, make empy package */
#ifdef DEBUG
      /*     stp_erprintf("-- empty col %i\n", x); */
#endif
    }
  }

#ifdef DEBUG
  stp_erprintf("lexmark: 4\n");
#endif

  clen=((unsigned char *)p)-prnBuf;
  if (maxclen < clen) {
    maxclen = clen;
  }

  switch(caps->model)    {
    case m_z52:
  prnBuf[IDX_SEQLEN]  =(unsigned char)(clen >> 24);
  prnBuf[IDX_SEQLEN+1]  =(unsigned char)(clen >> 16);
  prnBuf[IDX_SEQLEN+2]  =(unsigned char)(clen >> 8);
  prnBuf[IDX_SEQLEN+3]=(unsigned char)(clen & 0xFF);
  break;

  case m_3200:
    prnBuf[18] = (unsigned char)((clen - LXM_3200_HEADERSIZE) >> 16);
    prnBuf[19] = (unsigned char)((clen - LXM_3200_HEADERSIZE) >> 8);
    prnBuf[20] = (unsigned char)((clen - LXM_3200_HEADERSIZE) & 0xff);
    prnBuf[23] = (unsigned char)lexmark_calc_3200_checksum(&prnBuf[16]);
    break;

  default:
    break;
  }

  if (anyCol) {
    /* fist, move the paper */
    paper_shift(v, (*paperShift), caps);
    *paperShift=0;

    /* now we write the image line */
    stp_zfwrite((const char *)prnBuf,1,clen,v);
#ifdef DEBUG
    stp_erprintf("lexmark: line written.\n");
#endif
    return 1;
  } else {
#ifdef DEBUG
    stp_erprintf("-- empty line\n");
#endif
    return 0;
  }

  /* Send a line of raster graphics... */

  return 0;
}



#ifdef DEBUG
const stp_vars_t lex_show_init(int x, int y)
{
  const stp_vars_t ofile;

  ofile = fopen("/tmp/xx.ppm", "wb");
  if (ofile == NULL)
	{
    printf("Can't create file !\n");
    exit(2);
  }

  printf("x %d, y %d\n", x, y);

  fprintf(ofile, "P6\n");
  fprintf(ofile, "# CREATOR: \n");
  fprintf(ofile, "# Created with\n");
  fprintf(ofile, "%d %d\n255\n", x, y);

  return ofile;
}

void lex_show_dither(const stp_vars_t file, unsigned char *y,
		     unsigned char *c, unsigned char *m, unsigned char *ly,
		     unsigned char *lc, unsigned char *lm, unsigned char *k,
		     int length)
{
  int i;
  unsigned char col[3];
  unsigned char col1[3];
  unsigned char col2[3];
#define DCOL  155
#define LCOL  100
  /*
    y=NULL;
    c=NULL;
    m=NULL;
  */
  /* we have ly only !! specific for lexmark !! */
  /*  if (lm != NULL) {
      ly = y;
      y = NULL;
      }*/

  lex_show_lcount++;
  lex_show_length = length;

  for (i=0; i < length; i++) {
    col[0] = 255;
    col[1] = 255;
    col[2] = 255;
    if ((k==NULL) || (((k[i/8] >> (7-(i%8))) & 0x1) == 0)) {
      if ((c!=NULL) && (((c[i/8] >> (7-(i%8))) & 0x1) == 1)) {
	col[0] -= DCOL;
      }
      if ((m!=NULL) && (((m[i/8] >> (7-(i%8))) & 0x1) == 1)) {
	col[1] -= DCOL;
      }
      if ((y!=NULL) && (((y[i/8] >> (7-(i%8))) & 0x1) == 1)) {
	col[2] -= DCOL;
      }
      if ((lc!=NULL) && (((lc[i/8] >> (7-(i%8))) & 0x1) == 1)) {
	col[0] -= LCOL;
      }
      if ((lm!=NULL) && (((lm[i/8] >> (7-(i%8))) & 0x1) == 1)) {
	col[1] -= LCOL;
      }
      if ((ly!=NULL) && (((ly[i/8] >> (7-(i%8))) & 0x1) == 1)) {
	col[2] -= LCOL;
      }
    } else {
      col[0] = 0;
      col[1] = 0;
      col[2] = 0;
    }
    col1[0] = col[0];
    col1[1] = col[1];
    col1[2] = col[2];

#if 1
#if 0
    if (col[0] > col2[0])
      col[0] -= col2[0];
    if (col[1] > col2[1])
      col[1] -= col2[1];
    if (col[2] > col2[2])
      col[2] -= col2[2];
#else
    if (col[0] > col2[0])
      col[0] = col2[0];
    if (col[1] > col2[1])
      col[1] = col2[1];
    if (col[2] > col2[2])
      col[2] = col2[2];
#endif

    col2[0] = col1[0];
    col2[1] = col1[1];
    col2[2] = col1[2];
#endif

    fwrite(&col, 1, 3, file);
  }
}

void lex_show_deinit(const stp_vars_t file) {
  stp_erprintf("lex_show_lcount %d,   lex_show_length %d\n", lex_show_lcount, lex_show_length);
}


#endif



static void
flush_pass(stp_softweave_t *sw, int passno, int model, int width,
	   int hoffset, int ydpi, int xdpi, int physical_xdpi,
	   int vertical_subpass)
{
  const stp_vars_t nv = (sw->v);
  stp_lineoff_t *lineoffs = stp_get_lineoffsets_by_pass(sw, passno);
  const stp_linebufs_t *bufs = stp_get_linebases_by_pass(sw, passno);
  stp_pass_t *pass = stp_get_pass_by_pass(sw, passno);
  stp_linecount_t *linecount = stp_get_linecount_by_pass(sw, passno);
  int lwidth = (width + (sw->horizontal_weave - 1)) / sw->horizontal_weave;
  int microoffset = vertical_subpass & (sw->horizontal_weave - 1);

  int prn_mode;
  int direction = 0;
  const lexm_privdata_weave *privdata_weave = stp_get_driver_data(nv);
  const lexmark_cap_t * caps= lexmark_get_model_capabilities(model);
  int paperShift;
  Lexmark_head_colors head_colors[3]={{0, NULL,     0,  64/2, 64},
				      {0, NULL,  64/2, 128/2, 64},
				      {0, NULL, 128/2, 192/2, 64}};



 
#ifdef DEBUG
  stp_erprintf("Lexmark: flush_pass, here we are !\n");
  stp_erprintf("  passno %i, sw->ncolors %i, width %d, lwidth %d, linecount k %d, linecount m %d, bitwidth %d, separation_rows  %d\n", passno, sw->ncolors, width, lwidth, linecount[0].p.k, linecount[0].p.m, sw->bitwidth, sw->separation_rows);
  stp_erprintf("microoffset %d, vertical_subpass %d, sw->horizontal_weave %d\n", microoffset,vertical_subpass, sw->horizontal_weave);

  stp_erprintf("Lexmark: last_pass_offset %d, last_pass %d, logicalpassstart %d\n", sw->last_pass_offset, sw->last_pass, pass->logicalpassstart);
  stp_erprintf("Lexmark: vertical adapt: caps->y_raster_res %d, ydpi %d,  \n", caps->y_raster_res, ydpi);

#endif


  if (1) { /* wisi */

#ifdef DEBUG
  stp_erprintf("1\n");
  stp_erprintf("\n");
  stp_erprintf("lineoffs[0].v[j]  %d\n", lineoffs[0].v[0]);
  stp_erprintf("lineoffs[0].v[j]  %d\n", lineoffs[0].v[1]);

#endif

  switch (physical_xdpi) {
  case 300:
    prn_mode = PRINT_MODE_300;
    break;
  case 600:
    prn_mode = PRINT_MODE_600;
    break;
  case 1200:
    prn_mode = PRINT_MODE_1200;
    break;
  default:
#ifdef DEBUG
    stp_erprintf("Eror: Unsupported phys resolution (%d)\n", physical_xdpi);
#endif
    return;
    break;
  }
  /* calculate paper shift and adapt actual resoution to physical positioning resolution */
  paperShift = (pass->logicalpassstart - sw->last_pass_offset) * (caps->y_raster_res/ydpi); 


  if ((bufs[0].p.c != NULL) || (bufs[0].p.m != NULL) || (bufs[0].p.y != NULL)) {

    head_colors[0].line = bufs[0].p.c;
    head_colors[0].used_jets = linecount[0].p.c;
  
    head_colors[1].line = bufs[0].p.m;
    head_colors[1].used_jets = linecount[0].p.m;
  
    head_colors[2].line = bufs[0].p.y;
    head_colors[2].used_jets = linecount[0].p.y;


#ifdef DEBUGxx
#define lineoffcalc(n) ((((lwidth+7)/8)*i))
    /*#define lineoffcalc(n) (lineoffs[0].v[n]*i)*/
    {
      int i;
      printf("Let's go lex_show_dither (sw->jets %d,  paperShift %d)\n", sw->jets, paperShift);
      for (i=0; i < sw->jets; i++) {
	int mywidth=lwidth;
	lex_show_dither(dbgfile, 
			bufs[0].p.y+lineoffcalc(1), /* yellow, */
			bufs[0].p.c+lineoffcalc(2), /* cyan,  */
			bufs[0].p.m+lineoffcalc(3), /* magenta,  */
			NULL, /* lyellow,   */
			NULL, /* lcyan,   */
			NULL, /* lmagenta,   */
			bufs[0].p.k+lineoffcalc(0), /* black,   */
			mywidth); /*out_width*/
      }
    }
#endif

#ifdef DEBUG
    stp_erprintf("lexmark_write: lwidth %d\n", lwidth);
#endif
    lexmark_write(nv,		/* I - Print file or command */
		  privdata_weave->outbuf,/*unsigned char *prnBuf,   mem block to buffer output */
		  &paperShift,           /* int *paperShift, */
		  direction,                     /* int direction, */
		  sw->jets,       /* num of inks to print */
		  caps,                  /* const lexmark_cap_t *   caps,	    I - Printer model */
		  xdpi,                  /* int xresolution, */
		  2,                     /* yCount,*/
		  head_colors,           /* Lexmark_head_colors *head_colors, */
		  (lwidth+7)/8, /* length,	 I - Length of bitmap data of one line in bytes */
		  prn_mode | COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M,       /* mode,	 I - Which color */
		  ydpi,                  /* ydpi,	 I - Vertical resolution */
		  lwidth,      /* width, 	 I - Printed width in pixles*/
		  hoffset+microoffset,   /* offset  I - Offset from left side in x_raster_res DPI */
		  0                      /* dmt */);
    if (privdata_weave->bidirectional)
      direction = (direction +1) & 1;
  }


  if ((bufs[0].p.C != NULL) || (bufs[0].p.M != NULL) || (bufs[0].p.k != NULL)) {
    /* we print with the photo or black cartidge */
    
    if (sw->jets != 208) {
      /* we have photo or black cartridge */      
      head_colors[0].line = bufs[0].p.C;
      head_colors[0].used_jets = linecount[0].p.C;
      
      head_colors[1].line = bufs[0].p.M;
      head_colors[1].used_jets = linecount[0].p.M;

      head_colors[2].line = bufs[0].p.k;
      head_colors[2].used_jets = linecount[0].p.k;
    } else {
      /* we have black cartridge; we have to print with all jets at once */
      head_colors[0].line = bufs[0].p.k;
      head_colors[0].used_jets = linecount[0].p.k;
      head_colors[0].head_nozzle_start = 0;
      head_colors[0].head_nozzle_end = sw->jets/2;
      
      head_colors[2].line = NULL;
      head_colors[2].used_jets = 0;
      head_colors[2].head_nozzle_start = 0;
      head_colors[2].head_nozzle_end = 0;
      head_colors[1].line = NULL;
      head_colors[1].used_jets = 0;
      head_colors[1].head_nozzle_start = 0;
      head_colors[1].head_nozzle_end = 0;
    }



    lexmark_write(nv,		/* I - Print file or command */
		  privdata_weave->outbuf,/*unsigned char *prnBuf,   mem block to buffer output */
		  &paperShift,           /* int *paperShift, */
		  direction,             /* int direction, */
		  sw->jets,              /* num of inks to print */
		  caps,                  /* const lexmark_cap_t *   caps,     I - Printer model */
		  xdpi,                  /* int xresolution, */
		  2,                     /* yCount,*/
		  head_colors,           /* Lexmark_head_colors *head_colors, */
		  (lwidth+7)/8, /* length,	 I - Length of bitmap data of one line in bytes */
		  prn_mode | COLOR_MODE_LC | COLOR_MODE_LM | COLOR_MODE_K,       /* mode,	 I - Which color */
		  ydpi,                  /* ydpi,	 I - Vertical resolution */
		  lwidth,      /* width, 	 I - Printed width in pixles*/
		  hoffset+microoffset,   /* offset  I - Offset from left side in x_raster_res DPI */
		  0                      /* dmt */);
    if (privdata_weave->bidirectional) {
      direction = (direction +1) & 1;
    }
  } 
  /* store paper position in respect if there was a paper shift */
  sw->last_pass_offset = pass->logicalpassstart - (paperShift / (caps->y_raster_res/ydpi));
}

  lineoffs[0].v[0]  = 0;
  linecount[0].v[0] = 0;
  lineoffs[0].v[1]  = 0;
  linecount[0].v[1] = 0;
  lineoffs[0].v[2]  = 0;
  linecount[0].v[2] = 0;
  lineoffs[0].v[3]  = 0;
  linecount[0].v[3] = 0;
  lineoffs[0].v[4]  = 0;
  linecount[0].v[4] = 0;
  lineoffs[0].v[5]  = 0;
  linecount[0].v[5] = 0;
  lineoffs[0].v[6]  = 0;
  linecount[0].v[6] = 0;
 
#ifdef DEBUG
  stp_erprintf("lexmark_write finished\n");
#endif 

  sw->last_pass = pass->pass;
  pass->pass = -1;
}



#ifdef DEBUG

static void testprint(testdata *td)
{
  int icol, i;
  char dummy1[256], dummy2[256];
  stp_linebufs_t linebufs;

  /* init */
  for (i=0; i < (sizeof(linebufs.v)/sizeof(linebufs.v[0])); i++) {
    linebufs.v[i] = NULL;
  }
  
  /*let's go */
  td->ifile = fopen("/t1.ppm", "rb");
  if (td->ifile != NULL) { 
    /* find "{" */
    fscanf(td->ifile, "%[^{]{%[^\"]\"%d %d %d %d\",", dummy1, dummy2, &(td->x), &(td->y), &(td->cols), &(td->deep));
    td->cols -= 1; /* we reduce it by one because fist color will be ignored */
    td->input_line = (char *)malloc(td->x+10);
    printf("<%s> <%s>\n", dummy1, dummy2);
    printf("%d %d %d %d\n", td->x, td->y, td->cols, td->deep);
    if (td->cols > 16) {
      printf("too many colors !!\n");
      return;
    }

    /* read the colors */
    fscanf(td->ifile, "%[^\"]\"%c	c %[^\"]\",", dummy1, dummy2, dummy2); /* jump over first color */
    for (icol=0; icol < td->cols; icol++) {  /* we ignor the first color. It is "no dot". */
      fscanf(td->ifile, "%[^\"]\"%c	c %[^\"]\",", dummy1, &(td->colchar[icol]), dummy2);
      printf("colchar %d <%c>\n", i, td->colchar[icol]);
    }


    if (td->cols > 5) {
      td->cols = 7;
      for (icol=0; icol < td->cols; icol++) {  /* we ignor the first color. It is "no dot". */
	linebufs.v[icol] = (char *)malloc((td->x+7)/8); /* allocate the color */
      }
    } else if (td->cols > 4) {
      td->cols = 5;
      for (icol=0; icol < td->cols; icol++) {  /* we ignor the first color. It is "no dot". */
	linebufs.v[icol] = (char *)malloc((td->x+7)/8); /* allocate the color */
      }
    } else {
      td->cols = 1;
      linebufs.v[0] = (char *)malloc((td->x+7)/8); /* allocate the color */
    } 
  } else {
    printf("can't open file !\n");
  }
}


static void readtestprintline(testdata *td, stp_linebufs_t *linebufs)
{
  char dummy1[256];
  int icol, ix;

  stp_erprintf("start readtestprintline\n");
  for (icol=0; icol < 7; icol++) {
    if (linebufs->v[icol] != NULL) {
      memset(linebufs->v[icol], 0, (td->x+7)/8);  /* clean line */
    }
  }
  stp_erprintf("1 readtestprintline cols %d\n", td->cols);


  fscanf(td->ifile, "%[^\"]\"%[^\"]\",", dummy1, td->input_line);
  for (icol=0; icol < td->cols; icol++) {
   for (ix=0; ix < td->x; ix++) {
      if (td->input_line[ix] == td->colchar[icol]) {
	/* set the dot */
	if (icol != 0) {
	  linebufs->v[icol-1][ix/8] |= 1 << (ix%8);
	} else {
	  /* this is specific, we set ymc */
	  linebufs->p.y[ix/8] |= 1 << (ix%8);
	  linebufs->p.m[ix/8] |= 1 << (ix%8);
	  linebufs->p.c[ix/8] |= 1 << (ix%8);
	}
      }
    }
  }
  /* printf("pixchar  <%s><%s>\n",dummy1, td->input_line);*/
}

#endif
