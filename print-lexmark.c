/*
 * "$Id$"
 *
 *   Print plug-in Lexmark driver for the GIMP.
 *
 *   Copyright 2000 Richard wisenoecker (richard.wisenoecker@gmx.at) 
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



#include <stdarg.h>
#include "print.h"

#define false 0
#define true  1

int maxclen = 0;
/*
 * Local functions...
 */

#define max(a, b) ((a > b) ? a : b)

typedef enum Lex_model { m_lex7500,   m_z52=10052 } Lex_model;

typedef struct {
  Lex_model model;    /* printer model */
  int max_width;      /* maximum printable paper size */
  int max_height;
  int max_xdpi;
  int max_ydpi;
  int max_quality;
  int border_left;
  int border_right;
  int border_top;
  int border_bottom;
  int inks;           /* installable cartridges (LEXMARK_INK_*) */
  int slots;          /* available paperslots */
  int features;       /* special bjl settings */
} lexmark_cap_t;




const int IDX_Z52ID=2;
const int IDX_SEQLEN=3;


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

#define DPI300   0
#define DPI600   1
#define DPI1200  2
#define DPI2400  3
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

#define LX_PSHIFT 0x13
#define LX_Z52_COLOR_MODE_POS 0x9
#define LX_Z52_RESOLUTION_POS 0x7
#define LX_Z52_PRINT_DIRECTION_POS 0x8

#define LXM_Z52_HEADERSIZE 34
char outbufHeader_z52[LXM_Z52_HEADERSIZE]={
  0x1B,0x2A,0x24,0x00,0x00,0xFF,0xFF,         /* number of packets ----     vvvvvvvvv */ 
  0x01,0x01,0x01,0x1a,0x03,0x01,              /* 0x7-0xc: resolution, direction, head */
  0x03,0x60,                                  /* 0xd-0xe HE */
  0x04,0xe0,                                  /* 0xf-0x10  HS vertical pos */
  0x19,0x5c,                                  /* 0x11-0x12 */
  0x0,0x0,                                    /* 0x13-0x14  VO between packges*/
  0x0,0x80,                                   /* 0x15-0x16 */
  0x0,0x0,0x0,0x0,0x1,0x2,0x0,0x0,0x0,0x0,0x0 /* 0x17-0x21 */
};

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




int lr_shift[10] = { 9, 18, 2*18 }; /* vertical distance between ever 2nd  inkjet (related to resolution) */


static void lexmark_write_line(FILE *,
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
			     int, int, int, int);

#define MAX_OVERSAMPLED 4
#define MAX_BPP 2
#define BITS_PER_BYTE 8
#define COMPBUFWIDTH (PHYSICAL_BPI * MAX_OVERSAMPLED * MAX_BPP * \
	MAX_CARRIAGE_WIDTH / BITS_PER_BYTE)


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
    618, 936,      /* 8.58" x 13 " */
    2400, 1200, 2,
    11, 9, 10, 18,
    LEXMARK_INK_CMY | LEXMARK_INK_CMYK | LEXMARK_INK_CcMmYK,
    LEXMARK_SLOT_ASF1 | LEXMARK_SLOT_MAN1,
    LEXMARK_CAP_DMT
  },
  { /*  */
    m_lex7500,
    618, 936,      /* 8.58" x 13 " */
    2400, 1200, 2,
    11, 9, 10, 18,
    LEXMARK_INK_CMY | LEXMARK_INK_CMYK | LEXMARK_INK_CcMmYK,
    LEXMARK_SLOT_ASF1 | LEXMARK_SLOT_MAN1,
    LEXMARK_CAP_DMT
  },
};




static lexmark_cap_t lexmark_get_model_capabilities(int model)
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

static int
lexmark_media_type(const char *name, lexmark_cap_t caps)
{
  if (!strcmp(name,"Plain Paper"))           return  1;
  if (!strcmp(name,"Transparencies"))        return  2;
  if (!strcmp(name,"Back Print Film"))       return  3;
  if (!strcmp(name,"Fabric Sheets"))         return  4;
  if (!strcmp(name,"Envelope"))              return  5;
  if (!strcmp(name,"High Resolution Paper")) return  6;
  if (!strcmp(name,"T-Shirt Transfers"))     return  7;
  if (!strcmp(name,"High Gloss Film"))       return  8;
  if (!strcmp(name,"Glossy Photo Paper"))    return  9;
  if (!strcmp(name,"Glossy Photo Cards"))    return 10;
  if (!strcmp(name,"Photo Paper Pro"))       return 11;

#ifdef DEBUG
  fprintf(stderr,"lexmark: Unknown media type '%s' - reverting to plain\n",name);
#endif
  return 1;
}

static int
lexmark_source_type(const char *name, lexmark_cap_t caps)
{
  if (!strcmp(name,"Auto Sheet Feeder"))    return 4;
  if (!strcmp(name,"Manual with Pause"))    return 0;
  if (!strcmp(name,"Manual without Pause")) return 1;

#ifdef DEBUG
  fprintf(stderr,"lexmark: Unknown source type '%s' - reverting to auto\n",name);
#endif
  return 4;
}

static int
lexmark_printhead_type(const char *name, lexmark_cap_t caps)
{
  if (!strcmp(name,"Black"))       return 0;
  if (!strcmp(name,"Color"))       return 1;
  if (!strcmp(name,"Black/Color")) return 2;
  if (!strcmp(name,"Photo/Color")) return 3;
  if (!strcmp(name,"Photo"))       return 4;

#ifdef DEBUG
  fprintf(stderr,"lexmark: Unknown head combo '%s' - reverting to black\n",name);
#endif
  return 0;
}


/*******************************
lexmark_size_type
*******************************/
/* This method is actually not used.
   Is there a possibility to set such value ???????????? */
static unsigned char
lexmark_size_type(const vars_t *v, lexmark_cap_t caps)
{
  const papersize_t *pp = get_papersize_by_size(v->page_height, v->page_width);
  if (pp)
    {
      const char *name = pp->name;
      /* built ins: */
      if (!strcmp(name,"A5"))          return 0x01;
      if (!strcmp(name,"A4"))          return 0x03;
      if (!strcmp(name,"B5"))          return 0x08;
      if (!strcmp(name,"Letter"))      return 0x0d;
      if (!strcmp(name,"Legal"))       return 0x0f;
      if (!strcmp(name,"Envelope 10")) return 0x16;
      if (!strcmp(name,"Envelope DL")) return 0x17;
      if (!strcmp(name,"Letter+"))     return 0x2a;
      if (!strcmp(name,"A4+"))         return 0x2b;
      if (!strcmp(name,"Lexmark 4x2"))   return 0x2d;
      /* custom */

#ifdef DEBUG
      fprintf(stderr,"lexmark: Unknown paper size '%s' - using custom\n",name);
    } else {
      fprintf(stderr,"lexmark: Couldn't look up paper size %dx%d - "
	      "using custom\n",v->page_height, v->page_width);
#endif
    }
  return 0;
}

static char *
c_strdup(const char *s)
{
  char *ret = malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

const char *
lexmark_default_resolution(const printer_t *printer)
{
  lexmark_cap_t caps= lexmark_get_model_capabilities(printer->model);
  if (!(caps.max_xdpi%300))
    return "300x300 DPI";
  else
    return "180x180 DPI";
}

/*
 * 'lexmark_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
lexmark_parameters(const printer_t *printer,	/* I - Printer model */
                 char *ppd_file,	/* I - PPD file (not used) */
                 char *name,		/* I - Name of parameter */
                 int  *count)		/* O - Number of values */
{
  int		i;
  char		**p= 0,
                **valptrs= 0;

  static char   *media_types[] =
                {
                  ("Plain Paper"),
                  ("Transparencies"),
                  ("Back Print Film"),
                  ("Fabric Sheets"),
                  ("Envelope"),
                  ("High Resolution Paper"),
                  ("T-Shirt Transfers"),
                  ("High Gloss Film"),
                  ("Glossy Photo Paper"),
                  ("Glossy Photo Cards"),
                  ("Photo Paper Pro")
                };
  static char   *media_sources[] =
                {
                  ("Auto Sheet Feeder"),
                  ("Manual with Pause"),
                  ("Manual without Pause"),
                };

  lexmark_cap_t caps= lexmark_get_model_capabilities(printer->model);

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0) {
    int length_limit, width_limit;
    const papersize_t *papersizes = get_papersizes();
    valptrs = malloc(sizeof(char *) * known_papersizes());
    *count = 0;

    width_limit = caps.max_width;
    length_limit = caps.max_height;

    for (i = 0; i < known_papersizes(); i++) {
      if (strlen(papersizes[i].name) > 0 &&
	  papersizes[i].width <= width_limit &&
	  papersizes[i].length <= length_limit) {
	valptrs[*count] = malloc(strlen(papersizes[i].name) + 1);
	strcpy(valptrs[*count], papersizes[i].name);
	(*count)++;
      }
    }
    return (valptrs);
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    int x= caps.max_xdpi, y= caps.max_ydpi;
    int c= 0;
    valptrs = malloc(sizeof(char *) * 10);
    if (!(caps.max_xdpi%300)) {

      if ( 300<=x && 300<=y)
	valptrs[c++]= c_strdup("300x300 DPI");
      if ( 600<=x && 600<=y)
	valptrs[c++]= c_strdup("600x600 DPI");
      if (1200==x && 600==y)
	valptrs[c++]= c_strdup("1200x600 DPI");
      if (1200<=x && 1200<=y)
	valptrs[c++]= c_strdup("1200x1200 DPI");
      if (2400<=x && 1200<=y)
	valptrs[c++]= c_strdup("2400x1200 DPI");

    } else if (!(caps.max_xdpi%180)) {

      if ( 180<=x && 180<=y)
	valptrs[c++]= c_strdup("180x180 DPI");

    } else {
#ifdef DEBUG
      fprintf(stderr,"lexmark: unknown resolution multiplier for model %d\n",
	      caps.model);
#endif
      return 0;
    }
    *count= c;
    p= valptrs;
  }
  else if (strcmp(name, "InkType") == 0)
  {
    int c= 0;
    valptrs = malloc(sizeof(char *) * 5);
    if ((caps.inks & LEXMARK_INK_K))
      valptrs[c++]= c_strdup("Black");
    if ((caps.inks & LEXMARK_INK_CMY))
      valptrs[c++]= c_strdup("Color");
    if ((caps.inks & LEXMARK_INK_CMYK))
      valptrs[c++]= c_strdup("Black/Color");
    if ((caps.inks & LEXMARK_INK_CcMmYK))
      valptrs[c++]= c_strdup("Photo/Color");
    if ((caps.inks & LEXMARK_INK_CcMmYy))
      valptrs[c++]= c_strdup("Photo/Color");
    *count = c;
    p = valptrs;
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

  valptrs = malloc(*count * sizeof(char *));
  for (i = 0; i < *count; i ++)
    valptrs[i] = c_strdup(p[i]);

  return (valptrs);
}


/*
 * 'lexmark_imageable_area()' - Return the imageable area of the page.
 */

void
lexmark_imageable_area(const printer_t *printer,	/* I - Printer model */
		     const vars_t *v,   /* I */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */

  lexmark_cap_t caps= lexmark_get_model_capabilities(printer->model);

  default_media_size(printer, v, &width, &length);

  *left   = caps.border_left;
  *right  = width - caps.border_right;
  *top    = length - caps.border_top;
  *bottom = caps.border_bottom;
}

void
lexmark_limit(const printer_t *printer,	/* I - Printer model */
	    const vars_t *v,  		/* I */
	    int  *width,		/* O - Left position in points */
	    int  *length)		/* O - Top position in points */
{
  lexmark_cap_t caps= lexmark_get_model_capabilities(printer->model);
  *width =	caps.max_width;
  *length =	caps.max_height;
}



static void
lexmark_init_printer(FILE *prn, lexmark_cap_t caps,
		   int output_type, const char *media_str,
		   const vars_t *v, int print_head,
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


  /* write init sequence */
  switch (caps.model) {
  case m_z52:
    fwrite(startHeader_z52,LXM_Z52_STARTSIZE,1,prn);
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
	  page_width,page_height,printable_width,printable_height,
	  arg_70_1,arg_70_2,arg_70_3,arg_70_4);
#endif
  */

}

void lexmark_deinit_printer(FILE *prn, lexmark_cap_t caps)
{
  char buffer[4];

  memcpy(buffer,ESC2a,2);
  buffer[2] = 0x7;
  buffer[3] = 0x65;
  /* eject page */
  fwrite(buffer,4,1,prn);

  fflush(prn);
}


/* paper_shift() -- shift paper in printer -- units are unknown :-)
 */
void paper_shift(FILE *prn, int offset)
{
   unsigned char buf[5]={0x1b,0x2a,0x3,0x0,0x0};

  if (offset == 0) return;

   buf[3]=(unsigned char)(offset >> 8);
   buf[4]=(unsigned char)(offset & 0xFF);
   if (fwrite(buf,1,5,prn)!=5)
     fprintf(stderr,"paper_shift: short write\n");
   
}




/*
 *  'alloc_buffer()' allocates buffer and fills it with 0
 */
static unsigned char *lexmark_alloc_buffer(int size)
{
  unsigned char *buf= malloc(size);
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


int clean_color(unsigned char *line, int len) {
  return 0;
}


/**********************************************************
 * lexmark_print() - Print an image to a LEXMARK printer.
 **********************************************************/
/* This method should not be printer dependent (mybe it is because of nozzle count and other things) */
/* The method will set the printing method depending on the selected printer. 
   It will define the colors to be used and the resolution.
   Additionally the "interlace" and pass_height will be defined. 
   Interlace defines how many lines are prepaird related to one printer pass. 
   A value of 2 tells that we prepair the double number of lines which could be 
   printed with a single printer pass. This will be done because in this method we 
   only prepair continouse lines (it will be going to be complicated if we don't work like this).
   The methods lexmark_getNextMode() & lexmark_write() are responsible to handle the received lines 
   in a correct way. (Maybe, this is not very well from the design point of view, but 
   lexmark_getNextMode() & lexmark_write() are doing some other "special" things like 
   printing every second pixle, which is not so simpe to handle in this method).
*/
void
lexmark_print(const printer_t *printer,		/* I - Model */
            int       copies,		/* I - Number of copies */
            FILE      *prn,		/* I - File to print to */
	    Image     image,		/* I - Image to print */
	    const vars_t    *v)
{
  /*const int VERTSIZE=192;*/
  unsigned char *cmap = v->cmap;
  int		model = printer->model;
  const char	*resolution = v->resolution;
  const char	*media_type = v->media_type;
  const char	*media_source = v->media_source;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  const char	*ink_type = v->ink_type;
  double 	scaling = v->scaling;
  int		top = v->top;
  int		left = v->left;
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
		page_height,	/* Height of page */
		page_length,	/* True length of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_bpp,	/* Output bytes per pixel */
		length,		/* Length of raster data */
                buf_length,     /* Length of raster data buffer (dmt) */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  convert_t	colorfunc = 0;	/* Color conversion function... */
  int           image_height,
                image_width,
                image_bpp;
  int           use_dmt = 0;
  void *	dither;
  double        the_levels[] = { 0.5, 0.75, 1.0 };
  vars_t	nv;
  int           actPassHeight=0;  /* dots which have actually to be printed */
  unsigned char *outbuf;    /* mem block to buffer output */
  int yi, yl;
  int pass_height=0;  /* count of inkjets for one pass */
  int elinescount=0; /* line pos to move paper */
  lexmark_cap_t caps= lexmark_get_model_capabilities(model);
  int printhead= lexmark_printhead_type(ink_type,caps);
  int pass_shift; /* shift after one pass in pixle */
  int interlace=0; /* is one if not interlace. At 2, first every even line is written second every odd is written */
  int d_interlace=0; /* is one if not interlace. At 2, first every even line is written second every odd is written */
  int printMode = 0;
  double myNv;
  int direction = 0;
  int media, source;



  memcpy(&nv, v, sizeof(vars_t));

  /*
  * Setup a read-only pixel region for the entire image...
  */

  Image_init(image);
  image_height = Image_height(image);
  image_width = Image_width(image);
  image_bpp = Image_bpp(image);


  media= lexmark_media_type(media_type,caps);
  source= lexmark_source_type(media_source,caps);

  /* force grayscale if image is grayscale
   *                 or single black cartridge installed
   */

  if (nv.image_type == IMAGE_MONOCHROME)
    {
      output_type = OUTPUT_GRAY;
    }

  if (printhead == 0 || caps.inks == LEXMARK_INK_K) {
    output_type = OUTPUT_GRAY;
  }

  /*
   * Choose the correct color conversion function...
   */

  colorfunc = choose_colorfunc(output_type, image_bpp, cmap, &out_bpp, &nv);


  if (output_type == OUTPUT_GRAY) {
    printMode |= COLOR_MODE_K;
    pass_height=208;
  } else {
    /* color mode */
    printMode |= COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M;
    pass_height=192/3;
    
    if (printhead==2 && (caps.inks & LEXMARK_INK_BLACK_MASK)) {
      printMode |= COLOR_MODE_K;
    }
    if (printhead==3 && (caps.inks & (LEXMARK_INK_PHOTO_MASK))) {
      printMode |= COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M | COLOR_MODE_LC | COLOR_MODE_LM | COLOR_MODE_K;
#ifdef DEBUG
  fprintf(stderr,"lexmark: print in photo mode !!.\n");
#endif

    }
  }



 /*
  * Figure out the output resolution...
  */

  sscanf(resolution,"%dx%d",&xdpi,&ydpi);
#ifdef DEBUG
  fprintf(stderr,"lexmark: resolution=%dx%d\n",xdpi,ydpi);
#endif

  switch (xdpi) {
    case 300:
      myNv = 1.0;
      xresolution = DPI300;
      printMode |= PRINT_MODE_300;
      break;
    case 600:
      myNv = 0.5;
      xresolution = DPI600;
      printMode |= PRINT_MODE_600;
      break;
    case 1200:
      myNv = 0.3;
      xresolution = DPI1200;
      printMode |= PRINT_MODE_1200;
      break;
    case 2400:
      myNv = 0.3;
      xresolution = DPI2400;
      printMode |= PRINT_MODE_2400;
      break;
  default:
    return;
    break;
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
      pass_shift = pass_height;
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
      nv.image_type != IMAGE_MONOCHROME) {
    use_dmt= 1;
#ifdef DEBUG
    fprintf(stderr,"lexmark: using drop modulation technology\n");
#endif
  }

 /*
  * Compute the output size...
  */

  lexmark_imageable_area(printer, &nv, &page_left, &page_right,
                       &page_bottom, &page_top);

  compute_page_parameters(page_right, page_left, page_top, page_bottom,
			  scaling, image_width, image_height, image,
			  &orientation, &page_width, &page_height,
			  &out_width, &out_height, &left, &top);

#ifdef DEBUG
  printf("page_right %d, page_left %d, page_top %d, page_bottom %d, left %d, top %d\n",page_right, page_left, page_top, page_bottom,left, top); 
#endif

  /*
   * Recompute the image height and width.  If the image has been
   * rotated, these will change from previously.
   */
  image_height = Image_height(image);
  image_width = Image_width(image);

  default_media_size(printer, &nv, &n, &page_length);


  Image_progress_init(image);


  lexmark_init_printer(prn, caps, output_type, media_type,
		     &nv, printhead, media_source,
		     xdpi, ydpi, page_width, page_height,
		     top,left,use_dmt);

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;


  left = (300 * left / 72) + 60;
  
  delay_max = (92*d_interlace);
  delay_k=(delay_max-((23+19)*d_interlace)); ;/*22; */
  delay_c=((64+12+12+4)*d_interlace);/*0;*/
  delay_m=(delay_max-((44+2)*d_interlace)); /*12+64;*/
  delay_y=(delay_max-((92)*d_interlace)); /*24+(2*64);152*/
  delay_lc=((64+12+12+4)*d_interlace);/*0;*/
  delay_lm=(delay_max-((44+2)*d_interlace)); /*12+64;*/
  delay_ly=(delay_max-((92)*d_interlace)); /*24+(2*64);152*/
  delay_max += pass_shift*interlace;
  

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  if (use_dmt) {
    buf_length= length*2;
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
    cyan    = lexmark_alloc_buffer(buf_length*interlace*(delay_c+1+pass_height+pass_shift));
  }
  if ((printMode & COLOR_MODE_Y) == COLOR_MODE_Y) {
    yellow  = lexmark_alloc_buffer(buf_length*interlace*(delay_y+1+pass_height+pass_shift));
  }
  if ((printMode & COLOR_MODE_M) == COLOR_MODE_M) {
    magenta = lexmark_alloc_buffer(buf_length*interlace*(delay_m+1+pass_height+pass_shift));
    }
  if ((printMode & COLOR_MODE_K) == COLOR_MODE_K) {
    black   = lexmark_alloc_buffer(buf_length*interlace*(delay_k+1+pass_height+pass_shift));
  }
  if ((printMode & COLOR_MODE_LC) == COLOR_MODE_LC) {
    lcyan = lexmark_alloc_buffer(buf_length*interlace*(delay_lc+1+pass_height+pass_shift));
  }
  if ((printMode & COLOR_MODE_LY) == COLOR_MODE_LY) {
    lyellow = lexmark_alloc_buffer(buf_length*interlace*(delay_lc+1+pass_height+pass_shift));
  }
  if ((printMode & COLOR_MODE_LM) == COLOR_MODE_LM) {
    lmagenta = lexmark_alloc_buffer(buf_length*interlace*(delay_lm+1+pass_height+pass_shift));
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
  fprintf(stderr,"density is %f\n",nv.density);
#endif

  nv.density = (nv.density * ydpi) / (xdpi * 1.0);
  if (nv.density > 1.0)
    nv.density = 1.0;

  nv.density = myNv; /* !!!!!!!!!!! wisi */

  compute_lut(256, &nv);

#ifdef DEBUG
  fprintf(stderr,"density is %f\n",nv.density);
#endif

  if (xdpi > ydpi)
    dither = init_dither(image_width, out_width, 1, xdpi / ydpi, &nv);
  else
    dither = init_dither(image_width, out_width, ydpi / xdpi, 1, &nv);

  dither_set_black_levels(dither, 1.0, 1.0, 1.0);
  dither_set_black_lower(dither, .8 / ((1 << (use_dmt+1)) - 1));
  /*
  if (use_glossy_film)
  */
  dither_set_black_upper(dither, .999);
  /*
  else
    dither_set_black_upper(dither, .999);
  */

  if (!use_dmt) {
    dither_set_light_inks(dither,
			  (lcyan)   ? (0.3333) : (0.0),
			  (lmagenta)? (0.3333) : (0.0),
			  (lyellow) ? (0.3333) : (0.0), nv.density);
  }

  switch (nv.image_type)
    {
    case IMAGE_LINE_ART:
      dither_set_ink_spread(dither, 19);
      break;
    case IMAGE_SOLID_TONE:
      dither_set_ink_spread(dither, 15);
      break;
    case IMAGE_CONTINUOUS:
      dither_set_ink_spread(dither, 14);
      break;
    }
  dither_set_density(dither, nv.density);

  if (use_dmt)
    {
      dither_set_c_ranges_simple(dither, 3, the_levels, nv.density);
      dither_set_m_ranges_simple(dither, 3, the_levels, nv.density);
      dither_set_y_ranges_simple(dither, 3, the_levels, nv.density);
      dither_set_k_ranges_simple(dither, 3, the_levels, nv.density);
    }
 /*
  * Output the page...
  */

  elinescount = (top*20)+200;
  paper_shift(prn, elinescount);
  elinescount=0;


  in  = malloc(image_width * image_bpp);
  out = malloc(image_width * out_bpp * 2);

  /* calculate the memory we need for one line of the printer image (hopefully we are right) */
#ifdef DEBUG
  fprintf(stderr,"---------- buffer mem size = %d\n", (((((pass_height/8)*11)/10)+40) * out_width)+200);
#endif
  outbuf = malloc((((((pass_height/8)*11)/10)+40) * out_width)+200);

  errdiv  = image_height / out_height;
  errmod  = image_height % out_height;
  errval  = 0;
  errlast = -1;
  errline  = 0;


  for (yl = 0; yl <= (out_height/(interlace*pass_height)); yl ++)
  {
     int duplicate_line = 1;

#ifdef DEBUG
       fprintf(stderr,"print yl %i of %i\n", yl, out_height/pass_height); 
#endif

     if (((yl+1) * interlace*pass_height) < out_height) {
      actPassHeight = interlace*pass_height;
    } else {
      actPassHeight = (out_height-((yl) * interlace*pass_height));
    }
#ifdef DEBUG
     printf(">>> yl %d, actPassHeight %d, out_height %d\n", yl, actPassHeight, out_height);
#endif
          
     for (yi = 0; yi < actPassHeight; yi ++)  {
	 y = (yl*interlace*pass_height) + yi;
	   lexmark_advance_buffer(black,    buf_length,(delay_k+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(cyan,     buf_length,(delay_c+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(magenta,  buf_length,(delay_m+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(yellow,   buf_length,(delay_y+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(lcyan,    buf_length,(delay_lc+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(lmagenta, buf_length,(delay_lm+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(lyellow,  buf_length,(delay_ly+pass_height+pass_shift)*interlace);

	   if ((y & 63) == 0)
      Image_note_progress(image, y, out_height);

      if (errline != errlast)
	{
	  errlast = errline;
	  duplicate_line = 0;
	  Image_get_row(image, in, errline);
	  /*	  printf("errline %d ,   image height %d\n", errline, image_height);*/
	  (*colorfunc)(in, out, image_width, image_bpp, cmap, &nv, NULL);
	}
      /*      printf("Let's dither   %d    %d  %d\n", ((y%interlace)), buf_length, length);*/
      if (nv.image_type == IMAGE_MONOCHROME)
	dither_monochrome(out, y, dither, black, duplicate_line);
      else if (output_type == OUTPUT_GRAY)
	dither_black(out, y, dither, black, duplicate_line);
      else
	dither_cmyk(out, y, dither, 
		    cyan, 
		    lcyan, 
		    magenta, 
		    lmagenta,
		    yellow, 
		    lyellow,
		    black, 
		    duplicate_line);

      clean_color(cyan, length);
      clean_color(magenta, length);
      clean_color(yellow, length);

      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
	{
	  errval -= out_height;
	  errline ++;
	}
     } /* for yi */
#ifdef DEBUG
     /*          printf("\n"); */
#endif

     for (;actPassHeight < (interlace*pass_height);actPassHeight++) {
	   lexmark_advance_buffer(black,    buf_length,(delay_k+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(cyan,     buf_length,(delay_c+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(magenta,  buf_length,(delay_m+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(yellow,   buf_length,(delay_y+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(lcyan,    buf_length,(delay_lc+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(lmagenta, buf_length,(delay_lm+pass_height+pass_shift)*interlace);
	   lexmark_advance_buffer(lyellow,  buf_length,(delay_ly+pass_height+pass_shift)*interlace);	 
     }

       lexmark_write_line(prn, outbuf, printMode, &direction, &elinescount, xresolution,  yresolution, interlace, pass_height, pass_shift, caps, ydpi,
			  black,    delay_k,
			  cyan,     delay_c,
			  magenta,  delay_m,
			  yellow,   delay_y,
			  lcyan,    delay_lc,
			  lmagenta, delay_lm,
			  lyellow,  delay_ly,
			  length, out_width, left, use_dmt);
     

    /* we have to collect the lines for the inkjets */

#ifdef DEBUG
    /* fprintf(stderr,"!"); */
#endif


       /*    errval += errmod;
    errline += errdiv;
    if (errval >= out_height)
    {
      errval -= out_height;
      errline ++;
      }*/
  } 
  Image_progress_conclude(image);

  free_dither(dither);

  /*
   * Flush delayed buffers...
   */

  if (delay_max) {
#ifdef DEBUG
    printf("\nlexmark: flushing %d possibly delayed buffers\n",
	    delay_max);
#endif


    for (yl = 0; yl <= (delay_max/(interlace*pass_height)); yl ++)
  {
    for (y=0;y < (pass_height*interlace); y++) {
      lexmark_advance_buffer(black,    buf_length,(delay_k+pass_height+pass_shift)*interlace);
      lexmark_advance_buffer(cyan,     buf_length,(delay_c+pass_height+pass_shift)*interlace);
      lexmark_advance_buffer(magenta,  buf_length,(delay_m+pass_height+pass_shift)*interlace);
      lexmark_advance_buffer(yellow,   buf_length,(delay_y+pass_height+pass_shift)*interlace);
      lexmark_advance_buffer(lcyan,    buf_length,(delay_lc+pass_height+pass_shift)*interlace);
      lexmark_advance_buffer(lmagenta, buf_length,(delay_lm+pass_height+pass_shift)*interlace);
      lexmark_advance_buffer(lyellow,  buf_length,(delay_ly+pass_height+pass_shift)*interlace);
    } /* for y */

    lexmark_write_line(prn, outbuf, printMode, &direction, &elinescount, xresolution, yresolution, interlace, pass_height, pass_shift, caps, ydpi,
		       black,    delay_k,
		       cyan,     delay_c,
		       magenta,  delay_m,
		       yellow,   delay_y,
		       lcyan,    delay_lc,
		       lmagenta, delay_lm,
		       lyellow,  delay_ly,
		       length, out_width, left, use_dmt);
     
  } /* for yl */
  } /* if delay_max */

 /*
  * Cleanup...
  */

  free_lut(&nv);
  free(in);
  free(out);

  if (black != NULL)    free(black);
  if (cyan != NULL)     free(cyan);
  if (magenta != NULL)  free(magenta);
  if (yellow != NULL)   free(yellow);
  if (lcyan != NULL)    free(lcyan);
  if (lmagenta != NULL) free(lmagenta);
  if (lyellow != NULL)  free(lyellow);

  lexmark_deinit_printer(prn, caps);
}



/* lexmark_init_line 
   This method is printer type dependent code.

   This method initializes the line to be printed. It will set 
   the printer specific initialization which has to be done bofor 
   the pixles of the image could be printed. 
*/
unsigned char *
lexmark_init_line(int mode, unsigned char *prnBuf, int offset, int width, int direction,
	      lexmark_cap_t   caps	        /* I - Printer model */
) {
  int  left_margin_multipl;  /* multiplyer for left margin calculation */


  /*  printf("#### width %d, length %d, pass_height %d\n", width, length, pass_height);*/
  /* first, we wirte the line header */
  switch(caps.model)  {
  case m_z52:
    memcpy(prnBuf, outbufHeader_z52, LXM_Z52_HEADERSIZE);

    /* K could only be present if black is printed only. */
    if ((mode & COLOR_MODE_K) || (mode & (COLOR_MODE_K | COLOR_MODE_LC | COLOR_MODE_LM))) {
      prnBuf[LX_Z52_COLOR_MODE_POS] = LX_Z52_BLACK_PRINT;
    } else {
      prnBuf[LX_Z52_COLOR_MODE_POS] = LX_Z52_COLOR_PRINT;
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
    

    left_margin_multipl = 8; /* we need to multiply ! */


    offset *= left_margin_multipl;
    if (direction) {
      prnBuf[LX_Z52_PRINT_DIRECTION_POS] = 1;
    } else {
      offset += 64;
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
   This method writes a single line of the print. The line consits of "pass_height" 
   pixle lines (pixles, which could be printed with one pass by the printer. 
*/
static int
lexmark_write(FILE *prn,		/* I - Print file or command */
	      unsigned char *prnBuf,      /* mem block to buffer output */
	      int *paperShift,
	      int direction,
	      int pass_height,       /* num of inks to print */
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
  int x_max; /* maximum what x became */
  int y;  /* actual horizontal position */
  int dy; /* horiz. inkjet posintion */
  int x1;
  unsigned short pixleline;  /* byte to be written */
  unsigned int valid_bytes; /* bit list which tells the present bytes */
  int xStart=0; /* count start for horizontal line */
  int xEnd=0;
  int xIter=0;  /* count direction for horizontal line */
  int anyCol=0;
  int i, d;
  int calc_height;
#ifdef DEBUG
  /* fprintf(stderr,"<%d%c>",*empty,("CMYKcmy"[coloridx])); */
#endif
  
  
  /* first we have to write the initial sequence for a line */
  p = lexmark_init_line(mode, prnBuf, offset, width,
			direction,  /* direction */
			caps);
  
#ifdef DEBUG
  fprintf(stderr,"lexmark: printer line initialized.\n");
#endif

  if (direction) {
    /* left to right */
    xStart = -lr_shift[xresolution];
    xEnd = width-1;
    xIter = 1;
  } else {
    /* right to left ! */
    xStart = width-1;
    xEnd = -lr_shift[xresolution];
    xIter = -1;
  }
  
#ifdef DEBUG
  fprintf(stderr,"lexmark: xStart %d, xEnd %d, xIter %d.\n", xStart, xEnd, xIter);
#endif

  /* now we can start to write the pixles */
     if (yCount == 1) {
       calc_height = (pass_height >> 1) -1; /* we have to adapt the pass height according the yCount */
     } else {
       calc_height = (pass_height << (yCount >> 2)) -(1 << (yCount >> 2)); /* we have to adapt the pass height according the yCount */
     }
#ifdef DEBUG
  fprintf(stderr,"lexmark: calc_height %d.\n",calc_height);
#endif

  x_max = max(xStart, xEnd);
 
  for (x=xStart; x != xEnd; x+=xIter) {
    int  anyDots=0; /* tells us if there was any dot to print */

    tbits=p;
    *(p++)=0x3F;
    tbits[1]=0; /* here will be nice bitmap */
    p++;
    

    pixleline =0;     /* here we store 16 pixles */
    valid_bytes = 0;  /* for every valid word (16 bits) a corresponding bit will be set to 1. */

    anyDots =0;

    for (i=0; i < 3; i++) {
      d = 0;/*2*i; !!!!!!!!!!!! wisi*/
      for (dy=head_colors[i].head_nozzle_start,y=head_colors[i].v_start*yCount; 
	   (dy < head_colors[i].head_nozzle_end); 
	   y+=yCount, dy++) { /* we start counting with 1 !!!! */
	x1 = x+lr_shift[xresolution];
	if ((head_colors[i].line != NULL) &&
	    ((((x%2)==0) && (mode & EVEN_NOZZLES_H)) || 
	     (((x%2)==1) && (mode & ODD_NOZZLES_H)))) {
	  pixleline = pixleline << 1;
	  if (x >= 0)
	    if (mode & ODD_NOZZLES_V)
	      pixleline = pixleline | ((head_colors[i].line[(((calc_height)-(y))*length)+((x+d)/8)] >> (7-((x+d)%8))) & 0x1);
	  pixleline = pixleline <<1;
	  if (x1 < x_max)
	    if (mode & EVEN_NOZZLES_V)
	      pixleline = pixleline | ((head_colors[i].line[(((calc_height-(yCount>>1))-(y))*length)+ ((x1+d)/8)] >> (7-((x1+d)%8))) & 0x1);
	  
	} else {
	  pixleline = pixleline << 2;
	}
	if ((dy%8) == 7) {
	  anyDots |= pixleline;
	  if (pixleline) {
	    /* we have some dots */
	    valid_bytes = valid_bytes >> 1;
	    *((p++)) = (unsigned char)(pixleline >> 8);
	    *((p++)) = (unsigned char)(pixleline & 0xff);
	  } else {
	    /* there are no dots ! */
	    valid_bytes = valid_bytes >> 1;
	    valid_bytes |= 0x1000;
	  }
	  pixleline =0;
	}
      }
    }

    if (pass_height != 208) {
      valid_bytes = valid_bytes >> 1;
      valid_bytes |= 0x1000;
    }
    tbits[0] = 0x20 | ((unsigned char)((valid_bytes >> 8) & 0x1f));
    tbits[1] = (unsigned char)(valid_bytes & 0xff);
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
   prnBuf[IDX_SEQLEN]  =(unsigned char)(clen >> 24);
   prnBuf[IDX_SEQLEN+1]  =(unsigned char)(clen >> 16);
   prnBuf[IDX_SEQLEN+2]  =(unsigned char)(clen >> 8);
   prnBuf[IDX_SEQLEN+3]=(unsigned char)(clen & 0xFF);
   if (anyCol) {
     /* fist, move the paper */
     paper_shift(prn, (*paperShift));   
     *paperShift=0;

     /* now we write the image line */
     if ( fwrite(prnBuf,1,clen,prn)!=clen)
       fprintf(stderr,"print_cols: short write\n");
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


int lexmark_getNextMode(int *mode, int *direction, int pass_height, int *lineStep, int *pass_shift, int *interlace) {
  /* This method should be printer independent */
  /* The method calculates, dependent from the resolution, the line step size. Additionally it sets the mode
     which should be used for printing (defines which pixles/nozzles have to be used for a pass).
     Following is supported:
     300DPI:
             The step size is the full pass_height.
             Vertically, only every second nozzle is use for printing.
     600dpi:
             The step size is the full pass_height.
	     Vertically, all nozzles are used.
     1200dpi:
             The step siue is 1/4 of the full pass_height.
	     Horizontally, only every second pixle will be printed.
	     Vertically, only ever second nozzle is used for printing.
	     Every line it will be alternated between odd/even lines and odd/even nozzles.
  */

  int full_pass_step = pass_height *2;
  int overprint_step1, overprint_step2; 


  switch (*mode & PRINT_MODE_MASK) {
  case PRINT_MODE_300:
    if (*pass_shift ==0) {
      if (*mode & NOZZLE_MASK) {
	/* we are through. Stop now */
	/* that's it, reset and the exit */
	*mode = (*mode & (~NOZZLE_MASK));
	/*	*mode = (*mode & (~COLOR_MODE_MASK));*/
	return false;
      }
      /* this is the start */
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | EVEN_NOZZLES_H | ODD_NOZZLES_H);
      *lineStep += full_pass_step;
      *pass_shift = pass_height / 2;
      *interlace = 1;
      return true;
    } else {
      /* this is the start */
      *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | EVEN_NOZZLES_H | ODD_NOZZLES_H);
      *lineStep += full_pass_step;
      *pass_shift = 0;
      *interlace = 1;
      return true;
    }
    break;
  case PRINT_MODE_600:
    if (*mode & NOZZLE_MASK)
      return false;
    *mode = (*mode & (~NOZZLE_MASK)) | (EVEN_NOZZLES_V | ODD_NOZZLES_V | EVEN_NOZZLES_H | ODD_NOZZLES_H);
    *lineStep += full_pass_step;
    *pass_shift = 0;
    *interlace = 2;
    return true;
    break;
  case PRINT_MODE_2400:
    overprint_step1 = (pass_height/2)+3; /* will be 35 in color mode */
    overprint_step2 = (pass_height/2)-3; /* will be 29 in color mode */
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
    overprint_step1 = (pass_height/4)+3; /* will be 19 in color mode */
    overprint_step2 = (pass_height/4)-3; /* will be 13 in color mode */
    *interlace = 4;
    if (0 == *pass_shift) { /* odd lines */
      if (*mode & NOZZLE_MASK) {
	/* we are through. Stop now */
	/* that's it, reset and the exit */
	*mode = (*mode & (~NOZZLE_MASK));
	/*	*mode = (*mode & (~COLOR_MODE_MASK));*/
	return false;
      }
      /* this is the start */
      *lineStep += overprint_step2;
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
lexmark_write_line(FILE *prn,	/* I - Print file or command */
		   unsigned char *prnBuf,/* mem block to buffer output */
		   int printMode,
		   int *direction,
		   int *elinescount,
		   int xresolution,
		   int yresolution,
		   int interlace,
		   int pass_height,       /* num of inks to print */
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
  int           dmt)
{
  static int empty= 0;
  int written= 0;
  int i=1;

  Lexmark_head_colors head_colors[3]={{0, NULL,     0,  64/2},
				      {0, NULL,  64/2, 128/2},
				      {0, NULL, 128/2, 192/2}};

  
  /*  paper_shift(prn, *elinescount);   
   *elinescount=0;*/
  pass_shift = 0;
  
  while (lexmark_getNextMode(&printMode, direction, pass_height, elinescount, &pass_shift, &interlace)) {
    if ((printMode & (COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M)) == (COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M)) {
#ifdef DEBUG
     fprintf(stderr,"print with color catridge (%x %x %x\n", c, m, y); 
#endif
      head_colors[0].line = c+(l*dc)+(l*i*pass_shift);
      head_colors[1].line = m+(l*dm)+(l*i*pass_shift);
      head_colors[2].line = y+(l*dy)+(l*i*pass_shift);
      if (lexmark_write(prn, prnBuf, elinescount, *direction, pass_height, caps, xresolution, interlace,
		    head_colors,   
		    l, printMode & ~(COLOR_MODE_K), /* we print colors only */
		    ydpi, &empty, width, offset, dmt))
	*direction = (*direction +1) & 1;
    }
    if ((printMode & COLOR_MODE_MASK) == (COLOR_MODE_C | COLOR_MODE_Y | COLOR_MODE_M | COLOR_MODE_K)) {
#ifdef DEBUG
      fprintf(stderr,"print with black catridge (%x %x %x\n", k);
#endif
      head_colors[1].line = k+(l*dk)+(l*i*pass_shift);
      head_colors[0].line = 0;
      head_colors[2].line = 0;
      if (lexmark_write(prn, prnBuf, elinescount, *direction, pass_height, caps, xresolution,  interlace,
		    head_colors, 
		    l, printMode & ~(COLOR_MODE_C | COLOR_MODE_M | COLOR_MODE_Y), /* we print black only */
			ydpi, &empty, width, offset, dmt))
	  *direction = (*direction +1) & 1;
    }
    if ((printMode & (COLOR_MODE_MASK)) == COLOR_MODE_K) {
#ifdef DEBUG
     fprintf(stderr,"print with black catridge (%x\n",k); 
#endif
      head_colors[0].head_nozzle_start = 0;
      head_colors[0].head_nozzle_end = pass_height/2;
      head_colors[0].line = k+(l*dk)+(l*i*pass_shift);

      head_colors[1].head_nozzle_start = 0;
      head_colors[1].head_nozzle_end = 0;
      head_colors[1].line = NULL;

      head_colors[2].head_nozzle_start = 0;
      head_colors[2].head_nozzle_end = 0;
      head_colors[2].line = NULL;

      if (lexmark_write(prn, prnBuf, elinescount, *direction, pass_height, caps, xresolution,  interlace,
		    head_colors, 
		    l, printMode, /* we print black only */
			ydpi, &empty, width, offset, dmt))
	  *direction = (*direction +1) & 1;
    }
    if ((printMode & (COLOR_MODE_LC | COLOR_MODE_K | COLOR_MODE_LM)) == (COLOR_MODE_LC | COLOR_MODE_K | COLOR_MODE_LM)) {
#ifdef DEBUG
     fprintf(stderr,"print with photo catridge (%x %x %x\n", lc, lm, k); 
#endif
      head_colors[0].line = lc+(l*dlc)+(l*i*pass_shift);
      head_colors[1].line = lm+(l*dlm)+(l*i*pass_shift);
      head_colors[2].line = k +(l*dk) +(l*i*pass_shift);
      if (lexmark_write(prn, prnBuf, elinescount, *direction, pass_height, caps, xresolution, interlace,
		    head_colors,   
			l, printMode & ~(COLOR_MODE_C | COLOR_MODE_M | COLOR_MODE_Y), 
			ydpi, &empty, width, offset, dmt))
	*direction = (*direction +1) & 1;
    }
  }
  


  if (written)
    fwrite("\x1b\x28\x65\x02\x00\x00\x01", 7, 1, prn);
  else
    empty++;
}
