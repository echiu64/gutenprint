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


/* TODO-LIST
 *
 *   * implement the left border
 *
 */

/* #define DEBUG 1 */
#define USEEPSEWAVE 1


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdarg.h>
#include <gimp-print.h>
#include <gimp-print-internal.h>
#include <gimp-print-intl-internal.h>

#define false 0
#define true  1

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
const stp_vars_t  *dbgfile;

void lex_show_dither(const stp_vars_t file, unsigned char *y, unsigned char *c, unsigned char *m, unsigned char *ly, unsigned char *lc, unsigned char *lm, unsigned char *k, int length);
const stp_vars_t lex_show_init(int x, int y);
void lex_show_deinit(const stp_vars_t file);
#endif

/*** resolution specific parameters */
#define DPI300   0
#define DPI600   1
#define DPI1200  2
#define DPI2400  3

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
  /*** printer internal parameters ***/
  int offset_left_border;    /* Offset to the left paper border (== border_left=0) */
  int offset_top_border;     /* Offset to the top paper border (== border_top=0) */
  int h_offset_black_color;    /* Offset beetween first black and first color jet vertically */
  int v_offset_balck_color;    /* Offset beetween first black and first color jet horizontally */
  int direction_offset_black;      /* Offset when printing in the other direction for black */
  int direction_offset_color;      /* Offset when printing in the other direction for color */
  double x_multiplicator; /* multiplicator we have to use to get unit used by the printer */
  double y_multiplicator; /* multiplicator we have to use to get unit used by the printer */
} lexmark_cap_t;




static const int IDX_Z52ID =2;
static const int IDX_SEQLEN=3;


#define ODD_NOZZLES_V  1
#define EVEN_NOZZLES_V 2
#define ODD_NOZZLES_H  4
#define EVEN_NOZZLES_H 8

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
static char outbufHeader_z52[LXM_Z52_HEADERSIZE]={
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

static char outbufHeader_3200[LXM_3200_HEADERSIZE] =
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

static double lum_adjustment[49] =
{
  0.57,				/* C */
  0.67,
  0.77,
  0.85,
  0.85,
  0.8,
  0.75,
  0.667,
  0.65,				/* B */
  0.65,
  0.714,
  0.769,
  0.833,
  0.909,
  1.0,
  1.15,
  1.3,				/* M */
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,
  1.25,				/* R */
  1.2,
  1.15,
  1.1,
  1.05,
  1.0,
  1.0,
  1.0,
  1.0,				/* Y */
  0.9,
  0.8,
  0.7,
  0.6,
  0.55,
  0.5,
  0.45,
  0.36,				/* G */
  0.4,
  0.45,
  0.48,
  0.48,
  0.48,
  0.51,
  0.54,
  0.57				/* C */
};

static double hue_adjustment[49] =
{
  0,				/* C */
  0.17,
  0.29,
  0.38,
  0.47,
  0.52,
  0.57,
  0.62,
  0.65,				/* B */
  0.7,
  0.85,
  1.05,
  1.25,
  1.45,
  1.65,
  1.8,
  2.00,				/* M */
  2.18,
  2.29,
  2.38,
  2.47,
  2.56,
  2.65,
  2.74,
  2.83,				/* R */
  3.0,
  3.15,
  3.3,
  3.45,
  3.6,
  3.75,
  3.85,
  4.0,				/* Y */
  4.2,
  4.37,
  4.55,
  4.65,
  4.78,
  4.85,
  4.9,
  4.95,				/* G */
  5.05,
  5.15,
  5.25,
  5.35,
  5.5,
  5.65,
  5.8,
  6.0				/* C */
};


static int lr_shift_color[10] = { 9, 18, 2*18 }; /* vertical distance between ever 2nd  inkjet (related to resolution) */
static int lr_shift_black[10] = { 9, 18, 2*18 }; /* vertical distance between ever 2nd  inkjet (related to resolution) */

/* returns the offset of the first jet when printing in the other direction */
static int get_lr_shift(int mode) {

  int *ptr_lr_shift;

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








static void lexmark_write_line(const stp_vars_t ,
			       unsigned char *prnBuf,/* mem block to buffer output */
			       int printMode,
			       int *direction,
			       int *elinescount,
			       int xresolution,
			       int yresolution,
			       int interlace,
			       int inkjetLine,       /* num of inks to print */
			       int pass_shift,
			       lexmark_cap_t, int,
			       unsigned char *, int,
			       unsigned char *, int,
			       unsigned char *, int,
			       unsigned char *, int,
			       unsigned char *, int,
			       unsigned char *, int,
			       unsigned char *, int,
			       int, int, int, int, int);


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


static lexmark_cap_t lexmark_model_capabilities[] =
{
  /* default settings for unkown models */

  {   -1, 8*72,11*72,180,180,20,20,20,20, LEXMARK_INK_K, LEXMARK_SLOT_ASF1, 0 },

  /* tested models */

  { /* Lexmark */
    m_z52,
    618, 936,      /* max paper size *//* 8.58" x 13 " */
    0xffff,        /* supp_res */
    2400, 1200, 2, /* max resolution */
    0, 0, 0, 10, /* border l,r,t,b*/
    LEXMARK_INK_CMY | LEXMARK_INK_CMYK | LEXMARK_INK_CcMmYK,
    LEXMARK_SLOT_ASF1 | LEXMARK_SLOT_MAN1,
    LEXMARK_CAP_DMT,
    /* resolution specific */
    {{true}, {true}, {true}, {true}},
    /*** printer internal parameters ***/
    20,         /* real left paper border */
    123,       /* real top paper border */
    124,       /* black/color offset h */
    0,         /* black/color offset vertically */
    30,        /* direction offset black */
    10,         /* direction offset color */
    1,
    1200/72    /* use a vertical resolution of 1200 dpi */
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
    /*** printer internal parameters ***/
    0,         /* real left paper border */
    300,       /* real top paper border */
    20,       /* black/color offset */
    0,         /* black/color offset vertically */
    40,        /* direction offset black */
    12,         /* direction offset color */
    1,
    1200/72    /*  use a vertical resolution of 1200 dpi */
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
    /*** printer internal parameters ***/
    0,         /* real left paper border */
    300,       /* real top paper border */
    20,       /* black/color offset */
    0,         /* black/color offset vertically */
    25,        /* direction offset black */
    6,         /* direction offset color */
    1,
    1200/72    /*  */
  },
};




static lexmark_cap_t
lexmark_get_model_capabilities(int model)
{
  int i;
  int models= sizeof(lexmark_model_capabilities) / sizeof(lexmark_cap_t);
  for (i=0; i<models; i++) {
    if (lexmark_model_capabilities[i].model == model) {
      return lexmark_model_capabilities[i];
    }
  }
#ifdef DEBUG
  fprintf(stderr,"lexmark: model %d not found in capabilities list.\n",model);
#endif
  return lexmark_model_capabilities[0];
}

/* base density, k_lower_scale, k_upper */
static double media_parameters[][3] =
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
lexmark_media_type(const char *name, lexmark_cap_t caps)
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
  fprintf(stderr,"lexmark: Unknown media type '%s' - reverting to plain\n",name);
#endif
  return 1;
}

static int
lexmark_source_type(const char *name, lexmark_cap_t caps)
{
  if (!strcmp(name,_("Auto Sheet Feeder")))    return 4;
  if (!strcmp(name,_("Manual with Pause")))    return 0;
  if (!strcmp(name,_("Manual without Pause"))) return 1;

#ifdef DEBUG
  fprintf(stderr,"lexmark: Unknown source type '%s' - reverting to auto\n",name);
#endif
  return 4;
}

static int
lexmark_printhead_type(const char *name, lexmark_cap_t caps)
{


  if (!strcmp(name,_("Black")))       return 0;
  if (!strcmp(name,_("Color")))       return 1;
  if (!strcmp(name,_("Black/Color"))) return 2;
  if (!strcmp(name,_("Photo/Color"))) return 3;
  if (!strcmp(name,_("Photo")))       return 4;
  if (!strcmp(name,_("Photo Test Mode")))       return 5;


#ifdef DEBUG
  fprintf(stderr,"lexmark: Unknown head combo '%s' - reverting to black\n",name);
#endif
  return 2;
}


/*******************************
lexmark_size_type
*******************************/
/* This method is actually not used.
   Is there a possibility to set such value ???????????? */
static unsigned char
lexmark_size_type(const stp_vars_t v, lexmark_cap_t caps)
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
      fprintf(stderr,"lexmark: Unknown paper size '%s' - using custom\n",name);
    } else {
      fprintf(stderr,"lexmark: Couldn't look up paper size %dx%d - "
	      "using custom\n",stp_get_page_height(v), stp_get_page_width(v));
#endif
    }
  return 0;
}


static int lexmark_get_color_nozzles(const stp_printer_t printer)
{
  return 192;
}

static int lexmark_get_black_nozzles(const stp_printer_t printer)
{
  return 208;
}

/*
static int lexmark_get_nozzle_resolution(const stp_printer_t printer)
{
  return 1200;
}
*/

static char *
c_strdup(const char *s)
{
  char *ret = xmalloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

static const char *
lexmark_default_resolution(const stp_printer_t printer)
{
  lexmark_cap_t caps= lexmark_get_model_capabilities(stp_printer_get_model(printer));
  if (!(caps.max_xdpi%300))
    return _("300x300 DPI");
  else
    return _("180x180 DPI");
}



typedef struct {
  const char name[65];
  int hres;
  int vres;
  int softweave;
  int vertical_passes;
  int vertical_oversample;
  int unidirectional;
  int resid;
} lexmark_res_t;

static const lexmark_res_t lexmark_reslist[] = {
  { N_ ("300 DPI"),			 300,  300,  0, 1, 1, 0, 0 },
  { N_ ("300 DPI Unidirectional"),	 300,  300,  0, 1, 1, 1, 0 },
  { N_ ("600 DPI"),			 600,  600,  0, 1, 1, 0, 1 },
  { N_ ("600 DPI Unidirectional"),	 600,  600,  0, 1, 1, 1, 1 },
  { N_ ("1200 DPI"),			1200, 1200,  1, 2, 1, 0, 2 },
  { N_ ("1200 DPI  Unidirectional"),	1200, 1200,  0, 1, 1, 1, 2 },
  { "", 0, 0, 0, 0, 0, -1 }
};


static const lexmark_res_t
*lexmark_get_resolution_para(const stp_printer_t printer,
			    const char *resolution)
{
  lexmark_cap_t caps= lexmark_get_model_capabilities(stp_printer_get_model(printer));

  const lexmark_res_t *res = &(lexmark_reslist[0]);
  while (res->hres)
    {
      if (res->vres <= caps.max_ydpi != -1 &&
	  res->hres <= caps.max_xdpi != -1 &&
	  !strcmp(resolution, _(res->name)))
	{
	  return res;
	}
      res++;
    }
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

  lexmark_cap_t caps= lexmark_get_model_capabilities(stp_printer_get_model(printer));

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0) {
    int height_limit, width_limit;
    int papersizes = stp_known_papersizes();
    valptrs = xmalloc(sizeof(char *) * papersizes);
    *count = 0;

    width_limit = caps.max_width;
    height_limit = caps.max_length;

    for (i = 0; i < papersizes; i++)
      {
	const stp_papersize_t pt = stp_get_papersize_by_index(i);
	if (strlen(stp_papersize_get_name(pt)) > 0 &&
	    stp_papersize_get_width(pt) <= width_limit &&
	    stp_papersize_get_height(pt) <= height_limit)
	  {
	    valptrs[*count] = xmalloc(strlen(stp_papersize_get_name(pt)) +1);
	    strcpy(valptrs[*count], stp_papersize_get_name(pt));
	    (*count)++;
	  }
      }
    return (valptrs);
  }
  else if (strcmp(name, "Resolution") == 0)
    {
      unsigned int supported_resolutions = caps.supp_res;
      int c= 0;
      const lexmark_res_t *res;
      valptrs = xmalloc(sizeof(char *) * 10);

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
      valptrs = xmalloc(sizeof(char *) * 5);
      if ((caps.inks & LEXMARK_INK_K))
	valptrs[c++]= c_strdup(_("Black"));
      if ((caps.inks & LEXMARK_INK_CMY))
	valptrs[c++]= c_strdup(_("Color"));
      if ((caps.inks & LEXMARK_INK_CMYK))
	valptrs[c++]= c_strdup(_("Black/Color"));
      if ((caps.inks & LEXMARK_INK_CcMmYK))
	valptrs[c++]= c_strdup(_("Photo/Color"));
      if ((caps.inks & LEXMARK_INK_CcMmYy))
	valptrs[c++]= c_strdup(_("Photo/Color"));
      valptrs[c++]= c_strdup(_("Photo Test Mode"));
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

  valptrs = xmalloc(*count * sizeof(char *));
  for (i = 0; i < *count; i ++)
    /* translate media_types and media_sources */
    valptrs[i] = c_strdup(_(p[i]));

  return ((char **) valptrs);
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

  lexmark_cap_t caps= lexmark_get_model_capabilities(stp_printer_get_model(printer));

  stp_default_media_size(printer, v, &width, &length);

  *left   = caps.border_left;
  *right  = width - caps.border_right;
  *top    = length - caps.border_top;
  *bottom = caps.border_bottom;

  lxm3200_linetoeject = (length * 1200) / 72;
}

static void
lexmark_limit(const stp_printer_t printer,	/* I - Printer model */
	    const stp_vars_t v,  		/* I */
	    int  *width,		/* O - Left position in points */
	    int  *length)		/* O - Top position in points */
{
  lexmark_cap_t caps= lexmark_get_model_capabilities(stp_printer_get_model(printer));
  *width =	caps.max_width;
  *length =	caps.max_length;
}



static void
lexmark_init_printer(const stp_vars_t v, lexmark_cap_t caps,
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
   char startHeader_z52[LXM_Z52_STARTSIZE]={0x1B,0x2a,0x81,0x00,0x1c,0x56,0x49,0x00,
					   0x01,0x00,0x58,0x02,0x00,0x00,0xc0,0x12,
					   0xc8,0x19,0x02,0x00,0x68,0x00,0x09,0x00,
					   0x08,0x00,0x08,0x00,0x1b,0x2a,0x07,0x73,
					   0x30,0x1b,0x2a,0x6d,0x00,0x14,0x01,0xf4,
					   0x02,0x00,0x01,0xf0,0x1b,0x2a,0x07,0x63 };

  / * 1200 dpi * /
  char startHeader_z52[LXM_Z52_STARTSIZE]={0x1b,0x2a,0x81,0x00,0x1c,0x56,0x49,0x00,
					   0x01,0x00,0xb0,0x04,0x00,0x00,0x80,0x25,
					   0x90,0x33,0x01,0x00,0xd0,0x00,0x00,0x00,
					   0x08,0x00,0x08,0x00,0x1b,0x2a,0x07,0x73,
					   0x30,0x1b,0x2a,0x6d,0x00,0x14,0x01,0xf4,
					   0x02,0x00,0x01,0xf0,0x1b,0x2a,0x07,0x63 };*/

#define LXM_Z52_STARTSIZE 0x35
  /* 300 dpi */
  char startHeader_z52[LXM_Z52_STARTSIZE]={0x1b,0x2a,0x81,0x00,0x1c,0x56,0x49,0x00,
					   0x01,0x00,0x2c,0x01,0x00,0x00,0x60,0x09,
					   0xe4,0x0c,0x01,0x00,0x34,0x00,0x00,0x00,
					   0x08,0x00,0x08,0x00,0x1b,0x2a,0x07,0x76,
					   0x01,0x1b,0x2a,0x07,0x73,0x30,0x1b,0x2a,
					   0x6d,0x00,0x14,0x01,0xf4,0x02,0x00,0x01,
					   0xf0,0x1b,0x2a,0x07,0x63};
  #define ESC2a "\x1b\x2a"



#define LXM_3200_STARTSIZE 32

	char startHeader_3200[LXM_3200_STARTSIZE] =
	{
		0x1b, 0x2a, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x1b, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33,
		0x1b, 0x30, 0x80, 0x0C, 0x02, 0x00, 0x00, 0xbe,
		0x1b, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21
	};

  /* write init sequence */
  switch(caps.model)
	{
		case m_z52:
			stp_zfwrite(startHeader_z52,LXM_Z52_STARTSIZE,1,v);
			break;

		case m_3200:
			stp_zfwrite(startHeader_3200, LXM_3200_STARTSIZE, 1, v);
			break;

		default:
			fprintf(stderr, "Unknown printer !! %i\n", caps.model);
			exit(2);
  }

  if (output_type==OUTPUT_GRAY) {
  }

  if (print_head==0) {
  }




  /*
#ifdef DEBUG
  fprintf(stderr,"lexmark: printable size = %dx%d (%dx%d) %02x%02x %02x%02x\n",
	  page_width,page_height,printable_width,printable_length,
	  arg_70_1,arg_70_2,arg_70_3,arg_70_4);
#endif
  */

}

static void lexmark_deinit_printer(const stp_vars_t v, lexmark_cap_t caps)
{

	switch(caps.model)	{
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
			char buffer[24] =
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

			stp_zfwrite(buffer, 24, 1, v);
		}
		break;

		case m_lex7500:
			break;
	}

}


/* paper_shift() -- shift paper in printer -- units are unknown :-)
 */
static void paper_shift(const stp_vars_t v, int offset, lexmark_cap_t caps)
{
	switch(caps.model)	{
		case m_z52:
		{
			unsigned char buf[5] = {0x1b, 0x2a, 0x3, 0x0, 0x0};
			if(offset == 0)return;
			buf[3] = (unsigned char)(offset >> 8);
			buf[4] = (unsigned char)(offset & 0xFF);
			stp_zfwrite(buf, 1, 5, v);
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
			stp_zfwrite(buf, 1, 8, v);
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
  unsigned char *buf= xmalloc(size);
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
static void
lexmark_advance_buffer(unsigned char *buf, int len, int num)
{
  if (!buf || !len) return;
  if (num>0) memmove(buf+len,buf,len*num);
  memset(buf,0,len);
}


static int
clean_color(unsigned char *line, int len)
{
  return 0;
}



static void setcol0(char *a, int al)
{
  int i;

  for (i=0; i < al; i++) {
    a[i] = 0;
  }
}

static void setcol1(char *a, int al)
{
  int i;

  for (i=0; i < al; i++) {
    a[i] = 0x89;
    a[i] = 0xFF;
  }
}

static void setcol2(char *a, int al)
{
  int i;

  for (i=0; i < al; i++) {
    a[i] = 0x89;
    a[i] = 0xFF;
  }
}


/**********************************************************
 * lexmark_print() - Print an image to a LEXMARK printer.
 **********************************************************/
/* This method should not be printer dependent (mybe it is because of nozzle count and other things) */
/* The method will set the printing method depending on the selected printer.
   It will define the colors to be used and the resolution.
   Additionally the "interlace" and pass_length will be defined.
   Interlace defines how many lines are prepaird related to one printer pass.
   A value of 2 tells that we prepair the double number of lines which could be
   printed with a single printer pass. This will be done because in this method we
   only prepair continouse lines (it will be going to be complicated if we don't work like this).
   The methods lexmark_getNextMode() & lexmark_write() are responsible to handle the received lines
   in a correct way. (Maybe, this is not very well from the design point of view, but
   lexmark_getNextMode() & lexmark_write() are doing some other "special" things like
   printing every second pixel, which is not so simpe to handle in this method).
*/
static void
lexmark_print(const stp_printer_t printer,		/* I - Model */
	      stp_image_t *image,		/* I - Image to print */
	      const stp_vars_t    v)
{
  /*const int VERTSIZE=192;*/
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
  int		xdpi, ydpi, xresolution, yresolution;	/* Resolution */
  int		n;		/* Output number */
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned char	*in,		/* Input pixels */
    *black,		/* Black bitmap data */
    *cyan,		/* Cyan bitmap data */
    *magenta,	/* Magenta bitmap data */
    *yellow,	/* Yellow bitmap data */
    *lcyan,		/* Light cyan bitmap data */
    *lmagenta,	/* Light magenta bitmap data */
    *lyellow;	/* Light yellow bitmap data */
  int           delay_k,
    delay_c,
    delay_m,
    delay_y,
    delay_lc,
    delay_lm,
    delay_ly,
    delay_max;
  int		page_left,	/* Left margin of page */
    page_right,	/* Right margin of page */
    page_top,	/* Top of page */
    page_bottom,	/* Bottom of page */
    page_width,	/* Width of page */
    page_height,	/* Length of page */
    page_true_height,	/* True length of page */
    out_width,	/* Width of image on page */
    out_length,	/* Length of image on page */
    out_bpp,	/* Output bytes per pixel */
    length,		/* Length of raster data */
    buf_length,     /* Length of raster data buffer (dmt) */
    errdiv,		/* Error dividend */
    errmod,		/* Error modulus */
    errval,		/* Current error value */
    errline,	/* Current raster line */
    errlast;	/* Last raster line loaded */
  stp_convert_t	colorfunc = 0;	/* Color conversion function... */
  int           image_height,
    image_width,
    image_bpp;
  int           use_dmt = 0;
  void *	dither;
  stp_vars_t	nv = stp_allocate_copy(v);
  int           actPassHeight=0;  /* dots which have actually to be printed */
  unsigned char *outbuf;    /* mem block to buffer output */
  int yi, yl;
  int pass_length=0;  /* count of inkjets for one pass */
  int elinescount=0; /* line pos to move paper */
  lexmark_cap_t caps= lexmark_get_model_capabilities(model);
  int printhead= lexmark_printhead_type(ink_type,caps);
  int pass_shift; /* shift after one pass in pixel */
  int interlace=0; /* is one if not interlace. At 2, first every even line is written second every odd is written */
  int d_interlace=0; /* is one if not interlace. At 2, first every even line is written second every odd is written */
  int printMode = 0;
  int direction = 1; /* Daniel Gordini - this was initially set to zero */
  int media, source;
  /* Lexmark do not have differnet pixel sizes. We have to correct the density according the print resolution. */
  int  densityDivisor;  /* This parameter is will adapt the density according the resolution */
  double k_lower, k_upper;
  int  physical_xdpi = 0;
  int  physical_ydpi = 0;

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

  if (stp_get_image_type(nv) == IMAGE_MONOCHROME)
    {
      output_type = OUTPUT_GRAY;
    }

  if (printhead == 0 || caps.inks == LEXMARK_INK_K) {
    output_type = OUTPUT_GRAY;
  }

  /*
   * Choose the correct color conversion function...
   */

  colorfunc = stp_choose_colorfunc(output_type, image_bpp, cmap, &out_bpp, nv);


  if (output_type == OUTPUT_GRAY) {
    printMode |= COLOR_MODE_K;
    pass_length=208;
    elinescount =  caps.h_offset_black_color; /* add offset to the first black jet from color jet */
    lxm_nozzles_used = lexmark_get_black_nozzles(printer);
  } else {
    elinescount = 0; /* we have color where first jet is on position 0 */

    lxm_nozzles_used = lexmark_get_color_nozzles(printer);

    /* color mode */
    printMode |= COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M;
    pass_length=192/3;

    if (printhead==2 && (caps.inks & LEXMARK_INK_BLACK_MASK)) {
      printMode |= COLOR_MODE_K;
    }
    if ((printhead==3 || printhead==5) && (caps.inks & (LEXMARK_INK_PHOTO_MASK))) {
      printMode |= COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M | COLOR_MODE_LC | COLOR_MODE_LM | COLOR_MODE_K;
#ifdef DEBUG
      fprintf(stderr,"lexmark: print in photo mode !!.\n");
#endif

    }
  }



  /*
  * Figure out the output resolution...
  */

lexmark_describe_resolution(printer,
			    resolution, &xdpi,&ydpi);
#ifdef DEBUG
  fprintf(stderr,"lexmark: resolution=%dx%d\n",xdpi,ydpi);
#endif

  switch (xdpi) {
  case 300:
    densityDivisor = 1;
    xresolution = DPI300;
    printMode |= PRINT_MODE_300;
    physical_xdpi = 300;
    physical_ydpi = 1200;
    break;
  case 600:
    densityDivisor = 2;
    xresolution = DPI600;
    printMode |= PRINT_MODE_600;
    physical_xdpi = 600;
    physical_ydpi = 1200;
    break;
  case 1200:
    densityDivisor = 4;
    xresolution = DPI1200;
    printMode |= PRINT_MODE_1200;
    physical_xdpi = 1200;
    physical_ydpi = 1200;
    break;
  case 2400:
    densityDivisor = 16;
    xresolution = DPI2400;
    printMode |= PRINT_MODE_2400;
    physical_xdpi = 1200;
    physical_ydpi = 1200;
    break;
  default:
    return;
    break;
  }

  if ((printMode & COLOR_MODE_PHOTO) == COLOR_MODE_PHOTO) {
    /* in case of photo mode we have to go a bit ligther */
    densityDivisor *= 2;
  }

  switch (ydpi) {
  case 300:
    pass_shift = 0;
    interlace = 1;
    d_interlace = 1;
    yresolution = DPI300;
    break;
  case 600:
    pass_shift = 0;
    interlace = 1;
    d_interlace = 2;
    yresolution = DPI600;
    break;
  case 1200:
    pass_shift = pass_length;
    interlace = 2;
    d_interlace = 4;
    yresolution = DPI1200;
    break;
  default:
    return;
    break;
  }





  if (!strcmp(resolution+(strlen(resolution)-3),"DMT") &&
      (caps.features & LEXMARK_CAP_DMT) &&
      stp_get_image_type(nv) != IMAGE_MONOCHROME) {
    use_dmt= 1;
#ifdef DEBUG
    fprintf(stderr,"lexmark: using drop modulation technology\n");
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
			  &out_width, &out_length, &left, &top);

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


  lexmark_init_printer(nv, caps, output_type, media_type,
		       printhead, media_source,
		       xdpi, ydpi, page_width, page_height,
		       top,left,use_dmt);

  /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_length = ydpi * out_length / 72;

#ifdef DEBUG
  dbgfile = lex_show_init(out_width, out_length);
#endif


  left = ((300 * left) / 72) + caps.offset_left_border;


  delay_max = (92*d_interlace);
  delay_k=(delay_max-((23+19)*d_interlace)); ;/*22; */
  delay_c=((64+12+12+4)*d_interlace);/*0;*/
  delay_m=(delay_max-((44+2)*d_interlace)); /*12+64;*/
  delay_y=(delay_max-((92)*d_interlace)); /*24+(2*64);152*/
  delay_lc=((64+12+12+4)*d_interlace);/*0;*/
  delay_lm=(delay_max-((44+2)*d_interlace)); /*12+64;*/
  delay_ly=(delay_max-((92)*d_interlace)); /*24+(2*64);152*/
  if ((printMode & COLOR_MODE_PHOTO) == COLOR_MODE_PHOTO) {
    delay_k=(delay_max-((92)*d_interlace));
  }
  delay_max += pass_shift*interlace;


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
  fprintf(stderr,"lexmark: buflength is %d!\n",buf_length);
#endif


  /* Now we know the color which are used, let's get the memory for every color image */
  black   = NULL;
  cyan    = NULL;
  yellow  = NULL;
  magenta = NULL;
  lcyan   = NULL;
  lmagenta= NULL;
  lyellow = NULL;

  if ((printMode & COLOR_MODE_C) == COLOR_MODE_C) {
    cyan    = lexmark_alloc_buffer(buf_length*interlace*(delay_c+1+pass_length+pass_shift));
  }
  if ((printMode & COLOR_MODE_Y) == COLOR_MODE_Y) {
    yellow  = lexmark_alloc_buffer(buf_length*interlace*(delay_y+1+pass_length+pass_shift));
  }
  if ((printMode & COLOR_MODE_M) == COLOR_MODE_M) {
    magenta = lexmark_alloc_buffer(buf_length*interlace*(delay_m+1+pass_length+pass_shift));
  }
  if ((printMode & COLOR_MODE_K) == COLOR_MODE_K) {
    black   = lexmark_alloc_buffer(buf_length*interlace*(delay_k+1+pass_length+pass_shift));
  }
  if ((printMode & COLOR_MODE_LC) == COLOR_MODE_LC) {
    lcyan = lexmark_alloc_buffer(buf_length*interlace*(delay_lc+1+pass_length+pass_shift));
  }
  if ((printMode & COLOR_MODE_LY) == COLOR_MODE_LY) {
    lyellow = lexmark_alloc_buffer(buf_length*interlace*(delay_ly+1+pass_length+pass_shift));
  }
  if ((printMode & COLOR_MODE_LM) == COLOR_MODE_LM) {
    lmagenta = lexmark_alloc_buffer(buf_length*interlace*(delay_lm+1+pass_length+pass_shift));
  }


#ifdef DEBUG
  fprintf(stderr,"lexmark: driver will use colors ");
  if (cyan)     fputc('C',stderr);
  if (lcyan)    fputc('c',stderr);
  if (magenta)  fputc('M',stderr);
  if (lmagenta) fputc('m',stderr);
  if (yellow)   fputc('Y',stderr);
  if (lyellow)  fputc('y',stderr);
  if (black)    fputc('K',stderr);
  fprintf(stderr,"\n");
#endif





#ifdef DEBUG
  fprintf(stderr,"density is %f\n",stp_get_density(nv));
#endif

#ifdef DEBUG
  fprintf(stderr,"density is %f and will be changed to %f\n",stp_get_density(nv), stp_get_density(nv)/densityDivisor);
#endif
  /* Lexmark do not have differnet pixel sizes. We have to correct the density according the print resolution. */
  stp_set_density(nv, stp_get_density(nv) / densityDivisor);

  if(media >= 1 && media <= 11)
    stp_set_density(nv, stp_get_density(nv) * media_parameters[media-1][0]);
  else				/* Can't find paper type? Assume plain */
    stp_set_density(nv, stp_get_density(nv) * .5);
  if (stp_get_density(nv) > 1.0)
    stp_set_density(nv, 1.0);

  stp_compute_lut(256, nv);

#ifdef DEBUG
  fprintf(stderr,"density is %f\n",stp_get_density(nv));
#endif

  if (xdpi > ydpi)
    dither = stp_init_dither(image_width, out_width, 1, xdpi / ydpi, nv);
  else
    dither = stp_init_dither(image_width, out_width, ydpi / xdpi, 1, nv);

  stp_dither_set_black_levels(dither, 1.0, 1.0, 1.0);



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
  if(bits == 2)
    {
      if(use_6color)
        stp_dither_set_adaptive_divisor(dither, 8);
      else
        stp_dither_set_adaptive_divisor(dither, 4);
    }
  else
    stp_dither_set_adaptive_divisor(dither, 4);
	*/

	/*
	  stp_dither_set_black_lower(dither, .8 / ((1 << (use_dmt+1)) - 1));*/
  /*stp_dither_set_black_levels(dither, 0.5, 0.5, 0.5);
    stp_dither_set_black_lower(dither, 0.4);*/
  /*
    if (use_glossy_film)
  */
  stp_dither_set_black_upper(dither, .8);

  if (!use_dmt) {
    stp_dither_set_light_inks(dither,
			  (lcyan)   ? (0.3333) : (0.0),
			  (lmagenta)? (0.3333) : (0.0),
			  (lyellow) ? (0.3333) : (0.0), stp_get_density(nv));
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


  elinescount += (top*caps.y_multiplicator)+caps.offset_top_border;
  paper_shift(v, elinescount, caps);
  elinescount=0;


  in  = xmalloc(image_width * image_bpp);
  out = xmalloc(image_width * out_bpp * 2);

  /* calculate the memory we need for one line of the printer image (hopefully we are right) */
#ifdef DEBUG
  fprintf(stderr,"---------- buffer mem size = %d\n", (((((pass_length/8)*11)/10)+40) * out_width)+200);
#endif
  outbuf = xmalloc((((((pass_length/8)*11)/10)+40) * out_width)+200);

  errdiv  = image_height / out_length;
  errmod  = image_height % out_length;
  errval  = 0;
  errlast = -1;
  errline  = 0;





  /********* TEST Mode *************/

#define WLINE     lexmark_write_line(v, outbuf, printMode, &direction, &elinescount, xresolution,  yresolution, interlace, pass_length, pass_shift, caps, ydpi, \
		       black,    delay_k, \
		       cyan,     delay_c, \
		       magenta,  delay_m, \
		       yellow,   delay_y, \
		       lcyan,    delay_lc, \
		       lmagenta, delay_lm, \
		       lyellow,  delay_ly, \
		       length, out_width, left, use_dmt, \
		       1);


  if (printhead == 5) {
    /* we are in test mode !!! */

    elinescount=100;

    setcol0(cyan,     buf_length*((delay_c+ pass_length+pass_shift)*interlace));
    setcol0(black,    buf_length*((delay_k+ pass_length+pass_shift)*interlace));
    setcol0(magenta,  buf_length*((delay_m+ pass_length+pass_shift)*interlace));
    setcol0(yellow,   buf_length*((delay_y+ pass_length+pass_shift)*interlace));
    setcol0(lcyan,    buf_length*((delay_lc+pass_length+pass_shift)*interlace));
    setcol0(lmagenta, buf_length*((delay_lm+pass_length+pass_shift)*interlace));

    setcol1(black,    buf_length*((delay_k+ pass_length+pass_shift)*interlace));

    WLINE;

    setcol0(black,    buf_length*((delay_k+ pass_length+pass_shift)*interlace));

    WLINE;
    WLINE;
    WLINE;

    setcol1(lmagenta, buf_length*((delay_lm+pass_length+pass_shift)*interlace));
    WLINE;
    setcol0(lmagenta,  buf_length*((delay_lm+ pass_length+pass_shift)*interlace));
    WLINE;
    WLINE;
    WLINE;

    setcol1(lcyan,    buf_length*((delay_lc+pass_length+pass_shift)*interlace));
    WLINE;
    setcol0(lcyan,  buf_length*((delay_lc+ pass_length+pass_shift)*interlace));
    WLINE;
    WLINE;
    WLINE;

    setcol1(cyan,    buf_length*((delay_c+pass_length+pass_shift)*interlace));
    WLINE;
    setcol0(cyan,  buf_length*((delay_c+ pass_length+pass_shift)*interlace));
    WLINE;
    WLINE;
    WLINE;

    setcol1(magenta,    buf_length*((delay_m+pass_length+pass_shift)*interlace));
    WLINE;
    setcol0(magenta,  buf_length*((delay_m+ pass_length+pass_shift)*interlace));
    WLINE;
    WLINE;
    WLINE;

    setcol1(yellow,    buf_length*((delay_y+pass_length+pass_shift)*interlace));
    WLINE;
    setcol0(yellow,  buf_length*((delay_y+ pass_length+pass_shift)*interlace));
    WLINE;
    WLINE;
    WLINE;
    WLINE;
    WLINE;
    WLINE;


    out_length = 500;

    for (yl = 0; yl <= (out_length/(interlace*pass_length)); yl ++)
      {
	int duplicate_line = 1;

	if (((yl+1) * interlace*pass_length) < out_length) {
	  actPassHeight = interlace*pass_length;
	} else {
	  actPassHeight = (out_length-((yl) * interlace*pass_length));
	}

	for (yi = 0; yi < actPassHeight; yi ++)  {
	  y = (yl*interlace*pass_length) + yi;
	  lexmark_advance_buffer(black,    buf_length,(delay_k+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(cyan,     buf_length,(delay_c+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(magenta,  buf_length,(delay_m+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(yellow,   buf_length,(delay_y+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(lcyan,    buf_length,(delay_lc+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(lmagenta, buf_length,(delay_lm+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(lyellow,  buf_length,(delay_ly+pass_length+pass_shift)*interlace);

	  if ((y & 63) == 0)
	    image->note_progress(image, y, out_length);

	  if (errline != errlast)
	    {
	      errlast = errline;
	      duplicate_line = 0;
	    }
	  /*      printf("Let's dither   %d    %d  %d\n", ((y%interlace)), buf_length, length);*/

	  /*	 if (y == 25) {
		 printf("\n\nPrint colo line black !!!!!!!!!!!\n\n");
		 setcol2(yellow,    buf_length);
		 setcol2(cyan,    buf_length);
		 setcol2(magenta,    buf_length);
		 }
	  */
	  if ((y > 98) && (y < 102)) {
	    printf("\n\nPrint photo line black !!!!!!!!!!!\n\n");
	    if (lyellow)   setcol2(lyellow,     buf_length);
	    if (lmagenta)  setcol2(lmagenta,    buf_length);
	    if (lcyan)     setcol2(lcyan,       buf_length);

	    if (black)	 setcol2(black,    buf_length);
	  }

	  if ((y > 148) && (y < 152)) {
	    printf("\n\nPrint both line black !!!!!!!!!!!\n\n");
	    setcol2(yellow,    10);
	    setcol2(cyan,      10);
	    setcol2(magenta,   10);
	    if (lyellow)   setcol2(lyellow,   10);
	    if (lmagenta)  setcol2(lmagenta,  10);
	    if (lcyan)     setcol2(lcyan,     10);
	    if (black)	 setcol2(black,    10);
	  }
	  if ((y > 149) && (y < 151))  {
	    printf("\n\nPrint both line black !!!!!!!!!!!\n\n");
	    setcol2(yellow,    buf_length/2);
	    setcol2(cyan,    buf_length/2);
	    setcol2(magenta,    buf_length/2);
	    if (lyellow)   setcol2(lyellow +(buf_length/2),    buf_length/2);
	    if (lmagenta)  setcol2(lmagenta+(buf_length/2),    buf_length/2);
	    if (lcyan)     setcol2(lcyan   +(buf_length/2),    buf_length/2);
	    if (black)	 setcol2(black    +(buf_length/2),    buf_length/2);
	  }


	} /* for yi */
	for (;actPassHeight < (interlace*pass_length);actPassHeight++) {
	  lexmark_advance_buffer(black,    buf_length,(delay_k+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(cyan,     buf_length,(delay_c+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(magenta,  buf_length,(delay_m+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(yellow,   buf_length,(delay_y+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(lcyan,    buf_length,(delay_lc+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(lmagenta, buf_length,(delay_lm+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(lyellow,  buf_length,(delay_ly+pass_length+pass_shift)*interlace);
	}

	lexmark_write_line(v, outbuf, printMode, &direction, &elinescount, xresolution,  yresolution, interlace, pass_length, pass_shift, caps, ydpi,
			   black,    delay_k,
			   cyan,     delay_c,
			   magenta,  delay_m,
			   yellow,   delay_y,
			   lcyan,    delay_lc,
			   lmagenta, delay_lm,
			   lyellow,  delay_ly,
			   length, out_width, left, use_dmt,
			   lexmark_print_bidirectional(printer, resolution));


      }





    image->progress_conclude(image);


    stp_free_dither(dither);
    stp_free_lut(nv);
    free(in);
    free(out);

    if (black != NULL)    free(black);
    if (cyan != NULL)     free(cyan);
    if (magenta != NULL)  free(magenta);
    if (yellow != NULL)   free(yellow);
    if (lcyan != NULL)    free(lcyan);
    if (lmagenta != NULL) free(lmagenta);
    if (lyellow != NULL)  free(lyellow);


    lexmark_deinit_printer(v, caps);

    return;
  }
  /*********************************/




  for (yl = 0; yl <= (out_length/(interlace*pass_length)); yl ++)
    {
      int duplicate_line = 1;

#ifdef DEBUG
      fprintf(stderr,"print yl %i of %i\n", yl, out_length/pass_length);
#endif

      if (((yl+1) * interlace*pass_length) < out_length) {
	actPassHeight = interlace*pass_length;
      } else {
	actPassHeight = (out_length-((yl) * interlace*pass_length));
      }
#ifdef DEBUG
      printf(">>> yl %d, actPassHeight %d, out_length %d\n", yl, actPassHeight, out_length);
#endif

      for (yi = 0; yi < actPassHeight; yi ++)  {
	y = (yl*interlace*pass_length) + yi;
	lexmark_advance_buffer(black,    buf_length,(delay_k+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(cyan,     buf_length,(delay_c+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(magenta,  buf_length,(delay_m+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(yellow,   buf_length,(delay_y+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(lcyan,    buf_length,(delay_lc+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(lmagenta, buf_length,(delay_lm+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(lyellow,  buf_length,(delay_ly+pass_length+pass_shift)*interlace);

	if ((y & 63) == 0)
	  image->note_progress(image, y, out_length);

	if (errline != errlast)
	  {
	    errlast = errline;
	    duplicate_line = 0;
	    image->get_row(image, in, errline);
	    /*	  printf("errline %d ,   image height %d\n", errline, image_height);*/
#if 1
	    (*colorfunc)(in, out, image_width, image_bpp, cmap, nv,
			 hue_adjustment, lum_adjustment, NULL);
#else
	    (*colorfunc)(in, out, image_width, image_bpp, cmap, nv,
			 NULL, NULL, NULL);
#endif
	  }
	/*      printf("Let's dither   %d    %d  %d\n", ((y%interlace)), buf_length, length);*/
	stp_dither(out, y, dither, cyan, lcyan, magenta, lmagenta,
		   yellow, lyellow, black, duplicate_line);

	clean_color(cyan, length);
	clean_color(magenta, length);
	clean_color(yellow, length);

#ifdef DEBUG
	lex_show_dither(dbgfile, yellow, cyan, magenta, lyellow, lcyan, lmagenta, black, out_width);
#endif

	errval += errmod;
	errline += errdiv;
	if (errval >= out_length)
	  {
	    errval -= out_length;
	    errline ++;
	  }
      } /* for yi */
#ifdef DEBUG
      /*          printf("\n"); */
#endif

      for (;actPassHeight < (interlace*pass_length);actPassHeight++) {
	lexmark_advance_buffer(black,    buf_length,(delay_k+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(cyan,     buf_length,(delay_c+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(magenta,  buf_length,(delay_m+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(yellow,   buf_length,(delay_y+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(lcyan,    buf_length,(delay_lc+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(lmagenta, buf_length,(delay_lm+pass_length+pass_shift)*interlace);
	lexmark_advance_buffer(lyellow,  buf_length,(delay_ly+pass_length+pass_shift)*interlace);
      }

      lexmark_write_line(v, outbuf, printMode, &direction, &elinescount, xresolution,  yresolution, interlace, pass_length, pass_shift, caps, ydpi,
			 black,    delay_k,
			 cyan,     delay_c,
			 magenta,  delay_m,
			 yellow,   delay_y,
			 lcyan,    delay_lc,
			 lmagenta, delay_lm,
			 lyellow,  delay_ly,
			 length, out_width, left, use_dmt,
			 lexmark_print_bidirectional(printer, resolution));


      /* we have to collect the lines for the inkjets */

#ifdef DEBUG
      /* fprintf(stderr,"!"); */
#endif


      /*    errval += errmod;
	    errline += errdiv;
	    if (errval >= out_length)
	    {
	    errval -= out_length;
	    errline ++;
	    }*/
    }
  image->progress_conclude(image);

  stp_free_dither(dither);

  /*
   * Flush delayed buffers...
   */

  if (delay_max) {
#ifdef DEBUG
    printf("\nlexmark: flushing %d possibly delayed buffers\n",
	   delay_max);
#endif


    for (yl = 0; yl <= (delay_max/(interlace*pass_length)); yl ++)
      {
	for (y=0;y < (pass_length*interlace); y++) {
	  lexmark_advance_buffer(black,    buf_length,(delay_k+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(cyan,     buf_length,(delay_c+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(magenta,  buf_length,(delay_m+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(yellow,   buf_length,(delay_y+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(lcyan,    buf_length,(delay_lc+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(lmagenta, buf_length,(delay_lm+pass_length+pass_shift)*interlace);
	  lexmark_advance_buffer(lyellow,  buf_length,(delay_ly+pass_length+pass_shift)*interlace);
	} /* for y */

	lexmark_write_line(v, outbuf, printMode, &direction, &elinescount, xresolution, yresolution, interlace, pass_length, pass_shift, caps, ydpi,
			   black,    delay_k,
			   cyan,     delay_c,
			   magenta,  delay_m,
			   yellow,   delay_y,
			   lcyan,    delay_lc,
			   lmagenta, delay_lm,
			   lyellow,  delay_ly,
			   length, out_width, left, use_dmt,
			   lexmark_print_bidirectional(printer, resolution));

      } /* for yl */
  } /* if delay_max */

  /*
  * Cleanup...
  */

  stp_free_lut(nv);
  free(in);
  free(out);

  if (black != NULL)    free(black);
  if (cyan != NULL)     free(cyan);
  if (magenta != NULL)  free(magenta);
  if (yellow != NULL)   free(yellow);
  if (lcyan != NULL)    free(lcyan);
  if (lmagenta != NULL) free(lmagenta);
  if (lyellow != NULL)  free(lyellow);

#ifdef DEBUG
  lex_show_deinit(dbgfile);
#endif

  lexmark_deinit_printer(v, caps);
  stp_free_vars(nv);
}

stp_printfuncs_t stp_lexmark_printfuncs =
{
  lexmark_parameters,
  stp_default_media_size,
  lexmark_imageable_area,
  lexmark_limit,
  lexmark_print,
  lexmark_default_resolution,
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
lexmark_init_line(int mode, unsigned char *prnBuf, int offset, int width, int direction,
		  lexmark_cap_t   caps	        /* I - Printer model */
		  )
{
  int  left_margin_multipl;  /* multiplyer for left margin calculation */
  int pos1 = 0;
  int pos2 = 0;
  int abspos, disp;


  /*  printf("#### width %d, length %d, pass_length %d\n", width, length, pass_length);*/
  /* first, we wirte the line header */
  switch(caps.model)  {
  case m_z52:
    memcpy(prnBuf, outbufHeader_z52, LXM_Z52_HEADERSIZE);

    left_margin_multipl = 8; /* we need to multiply ! */
    offset *= left_margin_multipl;

    /* K could only be present if black is printed only. */
    if ((mode & COLOR_MODE_K) || (mode & (COLOR_MODE_K | COLOR_MODE_LC | COLOR_MODE_LM))) {
#ifdef DEBUG
      fprintf(stderr,"set  photo/black catridge \n");
#endif
      prnBuf[LX_Z52_COLOR_MODE_POS] = LX_Z52_BLACK_PRINT;

      if (direction) {
      } else {
	offset += caps.direction_offset_color;
      }
    } else {
#ifdef DEBUG
      fprintf(stderr,"set color catridge \n");
#endif
      prnBuf[LX_Z52_COLOR_MODE_POS] = LX_Z52_COLOR_PRINT;

      if (direction) {
	offset += caps.v_offset_balck_color;
      } else {
	offset += caps.v_offset_balck_color;
	offset += caps.direction_offset_color;
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

    /*    prnBuf[17] =(unsigned char)((((offset)+width)*2) >> 8);
	  prnBuf[18] =(unsigned char)((((offset)+width)*2) & 0xFF);
    */
    /*    prnBuf[30] =(unsigned char)((offset*left_margin_multipl) >> 8);
	  prnBuf[31] =(unsigned char)((offset*left_margin_multipl) & 0xFF);

	  prnBuf[32] =(unsigned char)((((offset*left_margin_multipl)+width)*2) >> 8);
	  prnBuf[33] =(unsigned char)((((offset*left_margin_multipl)+width)*2) & 0xFF);
    */
    /*   prnBuf[LX_PSHIFT] = (paperShift) >> 8;
	 prnBuf[LX_PSHIFT+1] = (paperShift) & 0xff;*/


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
    fprintf(stderr, "Lexmark 7500 not supported !\n");
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
	      lexmark_cap_t   caps,	        /* I - Printer model */
	      int xresolution,
	      int yCount,
	      Lexmark_head_colors *head_colors,
	      int           length,	/* I - Length of bitmap data */
	      int           mode,	/* I - Which color */
	      int           ydpi,		/* I - Vertical resolution */
	      int           *empty,       /* IO- Preceeding empty lines */
	      int           width,	/* I - Printed width */
	      int           offset, 	/* I - Offset from left side */
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
  int calc_length;
  int rwidth; /* real with used at printing (includes shift between even & odd nozzles) */
#ifdef DEBUG
  /* fprintf(stderr,"<%d%c>",*empty,("CMYKcmy"[coloridx])); */
#endif


  /* first we have to write the initial sequence for a line */

#ifdef DEBUG
  fprintf(stderr,"lexmark: printer line initialized.\n");
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
  fprintf(stderr,"lexmark: xStart %d, xEnd %d, xIter %d.\n", xStart, xEnd, xIter);
#endif

  /* now we can start to write the pixels */
  if (yCount == 1) {
    calc_length = (pass_length >> 1) -1; /* we have to adapt the pass height according the yCount */
  } else {
    calc_length = (pass_length << (yCount >> 2)) -(1 << (yCount >> 2)); /* we have to adapt the pass height according the yCount */
  }
#ifdef DEBUG
  fprintf(stderr,"lexmark: calc_length %d.\n",calc_length);
#endif


  for (x=xStart; x != xEnd; x+=xIter) {
    int  anyDots=0; /* tells us if there was any dot to print */

       switch(caps.model)	{
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
	if ((head_colors[colIndex].line != NULL) &&
	    ((((x%2)==0) && (mode & EVEN_NOZZLES_H)) ||
	     (((x%2)==1) && (mode & ODD_NOZZLES_H)))) {
	  pixelline = pixelline << 1;
	  if (x >= 0)
	    if (mode & ODD_NOZZLES_V)
	      pixelline = pixelline | ((head_colors[colIndex].line[(((calc_length)-(y))*length)+(x/8)] >> (7-(x%8))) & 0x1);
	  pixelline = pixelline <<1;
	  if (x1 < width)
	    if (mode & EVEN_NOZZLES_V)
	      pixelline = pixelline | ((head_colors[colIndex].line[(((calc_length-(yCount>>1))-(y))*length)+ (x1/8)] >> (7-(x1%8))) & 0x1);

	} else {
	  pixelline = pixelline << 2;
	}
	switch(caps.model)		{
	case m_z52:
	  if ((dy%8) == 7) {
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

    switch(caps.model)	{
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
      /*     fprintf(stderr,"-- empty col %i\n", x); */
#endif
    }
  }


  clen=((unsigned char *)p)-prnBuf;
  if (maxclen < clen) {
    maxclen = clen;
  }

  switch(caps.model)    {
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
    stp_zfwrite(prnBuf,1,clen,v);
#ifdef DEBUG
    fprintf(stderr,"lexmark: line written.\n");
#endif
    return 1;
  } else {
#ifdef DEBUG
    fprintf(stderr,"-- empty line\n");
#endif
    return 0;
  }

  /* Send a line of raster graphics... */

  return 0;
}


static int
lexmark_getNextMode(int *mode, int *direction, int pass_length, int *lineStep, int *pass_shift, int *interlace)
{
  /* This method should be printer independent */
  /* The method calculates, dependent from the resolution, the line step size. Additionally it sets the mode
     which should be used for printing (defines which pixels/nozzles have to be used for a pass).
     Following is supported:
     300DPI:
             The step size is the full pass_length.
             Vertically, only every second nozzle is use for printing.
     600dpi:
             The step size is the full pass_length.
	     Vertically, all nozzles are used.
     1200dpi:
             The step size is 1/4 of the full pass_length.
	     Horizontally, only every second pixel will be printed.
	     Vertically, only ever second nozzle is used for printing.
	     Every line it will be alternated between odd/even lines and odd/even nozzles.
  */

  int full_pass_step = pass_length *2;
  int overprint_step1, overprint_step2;


  switch (*mode & PRINT_MODE_MASK) {
  case PRINT_MODE_300:
    if (*pass_shift ==0) {
      if (*mode & NOZZLE_MASK) {
	/*** we are through. Stop now ***/
	/* make the last paper shift */
	*lineStep += full_pass_step;
	/* that's it, reset and the exit */
	*mode = (*mode & (~NOZZLE_MASK));
	/*	*mode = (*mode & (~COLOR_MODE_MASK));*/
	return false;
      }
      /* this is the start */
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | ODD_NOZZLES_V | EVEN_NOZZLES_H | ODD_NOZZLES_H);
      *pass_shift = pass_length / 2;
      *interlace = 1;
      return true;
    } else {
      /* this is the start */
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | ODD_NOZZLES_V | EVEN_NOZZLES_H | ODD_NOZZLES_H);
      *lineStep += full_pass_step;
      *pass_shift = 0;
      *interlace = 1;
      return true;
    }
    break;
  case PRINT_MODE_600:
    if (*mode & NOZZLE_MASK) {
      /*** that's it !! */
      *lineStep += full_pass_step;
      return false;
    }
    *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | ODD_NOZZLES_V | EVEN_NOZZLES_H | ODD_NOZZLES_H);
    *pass_shift = 0;
    *interlace = 2;
    return true;
    break;
  case PRINT_MODE_2400:
    overprint_step1 = (pass_length/2)+3; /* will be 35 in color mode */
    overprint_step2 = (pass_length/2)-3; /* will be 29 in color mode */
    *interlace = 4;
    if (0 == *pass_shift) {
      if (*mode & NOZZLE_MASK) {
	/* we are through. Stop now */
	/* that's it, reset and the exit */
	*mode = (*mode & (~NOZZLE_MASK));
	return false;
      }
      /* this is the start */
      *pass_shift = full_pass_step - overprint_step1;
      *lineStep += overprint_step1;
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | ODD_NOZZLES_V | EVEN_NOZZLES_H);
      return true;
    } else if ((full_pass_step - overprint_step1) == *pass_shift) {
      *lineStep += overprint_step2;
      *pass_shift -= overprint_step2;
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | ODD_NOZZLES_V | ODD_NOZZLES_H);
      /*      *mode = (*mode & (~COLOR_MODE_MASK)) | 0x30;*/
      return true;
    } else if ((full_pass_step - overprint_step1 - overprint_step2) == *pass_shift) {
      *lineStep += overprint_step1;
      *pass_shift -= overprint_step1;
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | ODD_NOZZLES_V | ODD_NOZZLES_H);
      /*      *mode = (*mode & (~COLOR_MODE_MASK)) | 0x30;*/
      return true;
    } else if ((full_pass_step - overprint_step1-overprint_step2-overprint_step1) == *pass_shift) {
      *lineStep += overprint_step2;
      *pass_shift -= overprint_step2;
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | ODD_NOZZLES_V | EVEN_NOZZLES_H);
      return true;
    }
    break;
  case PRINT_MODE_1200:
    overprint_step1 = (pass_length/4)+3; /* will be 19 in color mode */
    overprint_step2 = (pass_length/4)-3; /* will be 13 in color mode */
    *interlace = 4;
    if (0 == *pass_shift) { /* odd lines */
      if (*mode & NOZZLE_MASK) {
	/*** we are through. Stop now ***/
	/* make last line step */
	*lineStep += overprint_step2;
	/* that's it, reset and the exit */
	*mode = (*mode & (~NOZZLE_MASK));
	/*	*mode = (*mode & (~COLOR_MODE_MASK));*/
	return false;
      }
      /* this is the start */
      *pass_shift = full_pass_step - overprint_step2;
      *mode = (*mode & (~NOZZLE_MASK)) | (ODD_NOZZLES_V | ODD_NOZZLES_H);
      return true;
    } else if ((full_pass_step - overprint_step2) == *pass_shift) { /* even lines */
      *lineStep += overprint_step1;
      *pass_shift -= overprint_step1;
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | ODD_NOZZLES_H);
      return true;
    } else if ((full_pass_step -overprint_step2-overprint_step1) == *pass_shift) { /* odd lines */
      *lineStep += overprint_step2;
      *pass_shift -= overprint_step2;
      *mode = (*mode & (~NOZZLE_MASK)) | (ODD_NOZZLES_V | EVEN_NOZZLES_H);
      return true;
    } else if ((full_pass_step -(overprint_step2*2)-overprint_step1) == *pass_shift) { /* even lines */
      *lineStep += overprint_step1;
      *pass_shift -= overprint_step1;
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | EVEN_NOZZLES_H);
      return true;
    } else if ((full_pass_step -(overprint_step2*2)-(overprint_step1*2)) == *pass_shift) { /* odd lines */
      /* this is the start */
      *lineStep += overprint_step2;
      *pass_shift -= overprint_step2;
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | EVEN_NOZZLES_H);
      return true;
    } else if ((full_pass_step -(overprint_step2*3)-(overprint_step1*2)) == *pass_shift) { /* even lines */
      *lineStep += overprint_step1;
      *pass_shift -= overprint_step1;
      *mode = (*mode & (~NOZZLE_MASK)) | (ODD_NOZZLES_V | EVEN_NOZZLES_H);
      return true;
      break;
    } else if ((full_pass_step - (overprint_step2*3)-(overprint_step1*3)) == *pass_shift) { /* odd lines */
      *lineStep += overprint_step2;
      *pass_shift -= overprint_step2;
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | ODD_NOZZLES_H);
      return true;
      break;
    } else if ((full_pass_step -(overprint_step2*4)-(overprint_step1*3)) == *pass_shift) { /* even lines */
      *lineStep += overprint_step1;
      *pass_shift -= overprint_step1;
      *mode = (*mode & (~NOZZLE_MASK)) | (ODD_NOZZLES_V | ODD_NOZZLES_H);
      return true;
    }
    break;
  }
  return false;
}


static void
lexmark_write_line(const stp_vars_t v,	/* I - Print file or command */
		   unsigned char *prnBuf,/* mem block to buffer output */
		   int printMode,
		   int *direction,
		   int *elinescount,
		   int xresolution,
		   int yresolution,
		   int interlace,
		   int pass_length,       /* num of inks to print */
		   int pass_shift,       /* if we have to write interlace, this is the sift of lines */
		   lexmark_cap_t   caps,	/* I - Printer model */
		   int           ydpi,	/* I - Vertical resolution */
		   unsigned char *k,	/* I - Output bitmap data */
		   int           dk,	/* I -  */
		   unsigned char *c,	/* I - Output bitmap data */
		   int           dc,	/* I -  */
		   unsigned char *m,	/* I - Output bitmap data */
		   int           dm,	/* I -  */
		   unsigned char *y,	/* I - Output bitmap data */
		   int           dy,	/* I -  */
		   unsigned char *lc,	/* I - Output bitmap data */
		   int           dlc,	/* I -  */
		   unsigned char *lm,	/* I - Output bitmap data */
		   int           dlm,	/* I -  */
		   unsigned char *ly,	/* I - Output bitmap data */
		   int           dly,	/* I -  */
		   int           l,	/* I - Length of bitmap data */
		   int           width,	/* I - Printed width */
		   int           offset,  /* I - horizontal offset */
		   int           dmt,
		   int           bidirectional_printing)
{
  static int empty= 0;
  int written= 0;
  int i=1;

  Lexmark_head_colors head_colors[3]={{0, NULL,     0,  64/2},
				      {0, NULL,  64/2, 128/2},
				      {0, NULL, 128/2, 192/2}};


  /*  paper_shift(v, *elinescount, caps);
   *elinescount=0;*/
  pass_shift = 0;

  while (lexmark_getNextMode(&printMode, direction, pass_length, elinescount, &pass_shift, &interlace)) {
    if ((printMode & (COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M)) == (COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M)) {
#ifdef DEBUG
      fprintf(stderr,"print with color catridge (%x %x %x\n", (int)c, (int)m, (int)y);
#endif
      /*      if ((printMode & (COLOR_MODE_LC | COLOR_MODE_K | COLOR_MODE_LM)) != (COLOR_MODE_LC | COLOR_MODE_K | COLOR_MODE_LM))*/ if (1) {
	/* print CMY at CMYK */
	head_colors[0].line = c+(l*dc)+(l*i*pass_shift);
	head_colors[1].line = m+(l*dm)+(l*i*pass_shift);
	head_colors[2].line = y+(l*dy)+(l*i*pass_shift);
      } else {
	/* !!! Lexmark do not have light colors, the photo cartridge has dark colors !! */
	/* echange light with dark */
	head_colors[0].line = lc+(l*dlc)+(l*i*pass_shift);
	head_colors[1].line = lm+(l*dlm)+(l*i*pass_shift);
	head_colors[2].line = y+(l*dy)+(l*i*pass_shift);
      }

      if (lexmark_write(v, prnBuf, elinescount, *direction, pass_length, caps, xresolution, interlace,
			head_colors,
			l, printMode & ~(COLOR_MODE_LC | COLOR_MODE_K | COLOR_MODE_LM), /* we print colors only */
			ydpi, &empty, width, offset, dmt))
	if (bidirectional_printing)
	  *direction = (*direction +1) & 1;
    }
    if ((printMode & COLOR_MODE_MASK) == (COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M | COLOR_MODE_K)) {
      /* print K at CMYK */
#ifdef DEBUG
      fprintf(stderr,"print with black catridge (%x \n", (int)k);
#endif
      head_colors[1].line = k+(l*dk)+(l*i*pass_shift);
      head_colors[0].line = 0;
      head_colors[2].line = 0;
      if (lexmark_write(v, prnBuf, elinescount, *direction, pass_length, caps, xresolution,  interlace,
			head_colors,
			l, printMode & ~(COLOR_MODE_C | COLOR_MODE_M | COLOR_MODE_Y), /* we print black only */
			ydpi, &empty, width, offset, dmt))
	if (bidirectional_printing)
	  *direction = (*direction +1) & 1;
    }
    if ((printMode & (COLOR_MODE_MASK)) == COLOR_MODE_K) {
      /* print K at K mode */
#ifdef DEBUG
      fprintf(stderr,"print with black catridge (%x\n",(int)k);
#endif
      head_colors[0].head_nozzle_start = 0;
      head_colors[0].head_nozzle_end = pass_length/2;
      head_colors[0].line = k+(l*dk)+(l*i*pass_shift);

      head_colors[1].head_nozzle_start = 0;
      head_colors[1].head_nozzle_end = 0;
      head_colors[1].line = NULL;

      head_colors[2].head_nozzle_start = 0;
      head_colors[2].head_nozzle_end = 0;
      head_colors[2].line = NULL;

      if (lexmark_write(v, prnBuf, elinescount, *direction, pass_length, caps, xresolution,  interlace,
			head_colors,
			l, printMode, /* we print black only */
			ydpi, &empty, width, offset, dmt))
	if (bidirectional_printing)
	  *direction = (*direction +1) & 1;
    }
    if ((printMode & (COLOR_MODE_LC | COLOR_MODE_K | COLOR_MODE_LM)) == (COLOR_MODE_LC | COLOR_MODE_K | COLOR_MODE_LM)) {
      /* print lClMK at CMYlClMK mode */
      /* photo cartridge has dark colors ! */
#ifdef DEBUG
      fprintf(stderr,"print with photo catridge (%x %x %x\n", (int)lc, (int)lm, (int)k);
#endif
      head_colors[0].line = lc+(l*dlc)+(l*i*pass_shift);
      head_colors[1].line = lm+(l*dlm)+(l*i*pass_shift);
      head_colors[2].line = k +(l*dk) +(l*i*pass_shift);
      if (lexmark_write(v, prnBuf, elinescount, *direction, pass_length, caps, xresolution, interlace,
			head_colors,
			l, printMode & ~(COLOR_MODE_C | COLOR_MODE_M | COLOR_MODE_Y),
			ydpi, &empty, width, offset, dmt))
	if (bidirectional_printing)
	  *direction = (*direction +1) & 1;
    }
  }



  if (written)
    stp_zfwrite("\x1b\x28\x65\x02\x00\x00\x01", 7, 1, v);
  else
    empty++;
}


#ifdef DEBUG
const stp_vars_t lex_show_init(int x, int y) {
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

void lex_show_dither(const stp_vars_t file, unsigned char *y, unsigned char *c, unsigned char *m, unsigned char *ly, unsigned char *lc, unsigned char *lm, unsigned char *k, int length) {
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

    stp_zfwrite(&col, 1, 3, file);
  }
}

void lex_show_deinit(const stp_vars_t file) {
}


#endif
