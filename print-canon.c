/*
 * "$Id$"
 *
 *   Print plug-in CANON BJL driver for the GIMP.
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
 *   canon_parameters()     - Return the parameter values for the given
 *                            parameter.
 *   canon_imageable_area() - Return the imageable area of the page.
 *   canon_print()          - Print an image to a CANON printer.
 *   canon_write()          - Send 6-color graphics using compression.
 *
 * Revision History:
 *
 *   $Log$
 *   Revision 1.27  2000/02/22 08:08:39  gandy
 *   Some minor cosmethics
 *
 *   Revision 1.26  2000/02/21 15:12:57  rlk
 *   Minor release prep
 *
 *   Revision 1.25  2000/02/16 00:59:19  rlk
 *   1) Use correct convert functions (canon, escp2, pcl, ps).
 *
 *   2) Fix gray_to_rgb increment (print-util)
 *
 *   3) Fix dither update (print-dither)
 *
 *   Revision 1.24  2000/02/15 03:51:40  rlk
 *
 *   1) It wasn't possible to print to the edge of the page (as defined by
 *      the printer).
 *
 *   2) The page top/bottom/left/right (particularly bottom and right) in
 *      the size boxes wasn't displayed accurately (it *had* been coded in
 *      1/10", because that's the units used to print out the pager --
 *      really sillyl, that -- now it's all in points, which is more
 *      reasonable if still not all that precise).
 *
 *   3) The behavior of landscape mode was weird, to say the least.
 *
 *   4) Calculating the size based on scaling was also weird -- in portrait
 *      mode it just looked at the height of the page vs. the height of the
 *      image, and in landscape it just looked at width of the page and
 *      height of the image.  Now it looks at both axes and scales so that
 *      the larger of the two ratios (widths and heights) is set equal to
 *      the scale factor.  That seems more intuitive to me, at any rate.
 *      It avoids flipping between landscape and portrait mode as you
 *      rescale the image in auto mode (which seems just plain bizarre to
 *      me).
 *
 *   5) I changed the escp2 stuff so that the distance from the paper edge
 *      will be identical in softweave and in microweave mode.  Henryk,
 *      that might not quite be what you intended (it's the opposite of
 *      what you actually did), but at least microweave and softweave
 *      should generate stuff that looks consistent.
 *
 *   Revision 1.23  2000/02/13 08:47:52  gandy
 *   Fixed maximum paper size for BJC-6000
 *
 *   Revision 1.22  2000/02/13 03:14:26  rlk
 *   Bit of an oops here about printer models; also start on print-gray-using-color mode for better quality
 *
 *   Revision 1.21  2000/02/10 02:46:25  rlk
 *   initialization
 *
 *   Revision 1.20  2000/02/10 00:28:32  rlk
 *   Fix landscape vs. portrait problem
 *
 *   Revision 1.19  2000/02/09 02:56:27  rlk
 *   Put lut inside vars
 *
 *   Revision 1.18  2000/02/08 20:25:17  gandy
 *   Small fix that makes variable drop sizes work (in B/W)
 *
 *   Revision 1.17  2000/02/08 17:55:25  gandy
 *   Added call to dither_cmyk4()
 *
 *   Revision 1.16  2000/02/08 17:39:48  gandy
 *   Got support for variable drop sizes ready for testing
 *
 *   Revision 1.15  2000/02/08 14:12:17  gandy
 *   Next step in supporting variable dot sizes (still experimental)
 *
 *   Revision 1.14  2000/02/08 12:24:50  gandy
 *   Beginning support for variable drop sizes (experimental stage)
 *
 *   Revision 1.13  2000/02/07 17:03:19  gandy
 *   Major code-cleanups, prettified model capabilities
 *
 *   Revision 1.12  2000/02/06 22:08:19  rlk
 *   Remove calls to non-POSIX strdup
 *
 *   Revision 1.11  2000/02/06 03:59:09  rlk
 *   More work on the generalized dithering parameters stuff.  At this point
 *   it really looks like a proper object.  Also dynamically allocate the error
 *   buffers.  This segv'd a lot, which forced me to efence it, which was just
 *   as well because I found a few problems as a result...
 *
 *   Revision 1.10  2000/02/04 09:40:28  gandy
 *   Models BJC-1000/2000/3000/6000/6100/7000/7100 ready for testing.
 *
 *   Revision 1.9  2000/02/03 18:11:18  gandy
 *   Preparations for some more printer models (to be continued...)
 *
 *   Revision 1.8  2000/02/03 17:40:34  gandy
 *   Dirty left-border-treatment leaving an uncertainty of -4..+4 dots
 *
 *   Revision 1.7  2000/02/03 08:53:07  gandy
 *   Honours the new ink-type setting
 *
 *   Revision 1.6  2000/02/03 01:12:27  rlk
 *   Ink type
 *
 *   Revision 1.5  2000/02/02 18:36:25  gandy
 *   Minor cleanups of code and debugging messages
 *
 *   Revision 1.4  2000/02/02 18:25:27  gandy
 *   Prepared the driver for one of K/CMY/CMYK/CcMmYK/CcMmYy printing
 *
 *   Revision 1.3  2000/02/02 16:00:31  gandy
 *   Removed remnants from the original escp/2 source not needed for BJL
 *
 *   Revision 1.2  2000/02/02 15:53:09  gandy
 *   1) reworked printer capabilities handling
 *   2) initilization sends media type, paper format and printable area
 *   3) works fine with new dithering stuff
 *
 *   Revision 1.1  2000/02/01 09:01:40  gandy
 *   Add print-canon.c: Support for the BJC 6000 and possibly others
 *
 *   
 */


/* TODO-LIST 
 *
 *   * implement the left border
 *
 */

#include <stdarg.h>
#include "print.h"

/*
 * Local functions...
 */

typedef struct {
  int model;
  int max_width;      /* maximum printable paper size */
  int max_height;
  int max_xdpi;
  int max_ydpi;
  int inks;           /* installable cartridges (CANON_INK_*) */
  int slots;          /* available paperslots */
  int features;       /* special bjl settings */
} canon_cap_t;

static void canon_write_line(FILE *, canon_cap_t, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     unsigned char *, int,
			     int, int, int, int);

#define PHYSICAL_BPI 1440
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
#define CANON_INK_K           1     
#define CANON_INK_CMY         2     
#define CANON_INK_CMYK        4     
#define CANON_INK_CcMmYK      8     
#define CANON_INK_CcMmYy     16
#define CANON_INK_CcMmYyK    32

#define CANON_INK_BLACK_MASK (CANON_INK_K|CANON_INK_CMYK|\
                              CANON_INK_CcMmYK|CANON_INK_CcMmYyK)

#define CANON_INK_PHOTO_MASK (CANON_INK_CcMmYy|CANON_INK_CcMmYK|\
                              CANON_INK_CcMmYyK)

/* document feeding */
#define CANON_SLOT_ASF1    1
#define CANON_SLOT_ASF2    2
#define CANON_SLOT_MAN1    4
#define CANON_SLOT_MAN2    8

/* model peculiarities */
#define CANON_CAP_CMD61     1    /* uses command #0x61         */
#define CANON_CAP_DMT       2    /* Drop Modulation Technology */
#define CANON_CAP_MSB_FIRST 4    /* how to send data */

static canon_cap_t canon_model_capabilities[] =
{
  /* default settings for unkown models */

  {   -1, 8*72, 11*72,  180, 180, CANON_INK_K, CANON_SLOT_ASF1, 0 },

  /* tested models */

  { /* Canon BJC 6000 */
    6000,          
    618, 936,      /* 8.58" x 13 " */
    1440, 720, 
    CANON_INK_CMYK | CANON_INK_CcMmYK, 
    CANON_SLOT_ASF1 | CANON_SLOT_MAN1, 
    CANON_CAP_DMT
  },

  /* untested models */

  { /* Canon BJC 1000 */
    1000, 
    11*72, 17*72,  
    360, 360, 
    CANON_INK_K | CANON_INK_CMY, 
    CANON_SLOT_ASF1, 
    CANON_CAP_CMD61 
  },
  { /* Canon BJC 2000 */
    2000, 
    11*72, 17*72,  
    720, 360, 
    CANON_INK_CMYK, 
    CANON_SLOT_ASF1, 
    CANON_CAP_CMD61 
  },
  { /* Canon BJC 3000 */
    3000, 
    11*72, 17*72, 
    1440, 720, 
    CANON_INK_CMYK | CANON_INK_CcMmYK, 
    CANON_SLOT_ASF1, 
    CANON_CAP_CMD61 | CANON_CAP_DMT 
  },
  { /* Canon BJC 6100 */
    6100, 
    11*72, 17*72, 
    1440, 720, 
    CANON_INK_CMYK | CANON_INK_CcMmYK, 
    CANON_SLOT_ASF1, 
    CANON_CAP_CMD61 
  },
  { /* Canon BJC 7000 */
    7000, 
    11*72, 17*72, 
    1200, 600, 
    CANON_INK_CMYK | CANON_INK_CcMmYK, 
    CANON_SLOT_ASF1, 
    0 
  },
  { /* Canon BJC 7100 */
    7100, 
    11*72, 17*72, 
    1200, 600, 
    CANON_INK_CMYK | CANON_INK_CcMmYyK, 
    CANON_SLOT_ASF1, 
    0 
  },
  
  /* extremely fuzzy models */

  { /* Canon BJC 5100 */
    5100, 
    17*72, 22*72, 
    1440, 720, 
    CANON_INK_CMYK | CANON_INK_CcMmYK, 
    CANON_SLOT_ASF1, 
    CANON_CAP_DMT 
  },
  { /* Canon BJC 5500 */
    5500, 
    22*72, 34*72,  
    720, 360, 
    CANON_INK_CMYK | CANON_INK_CcMmYK, 
    CANON_SLOT_ASF1, 
    CANON_CAP_CMD61 
  },
  { /* Canon BJC 6500 */
    6500, 
    17*72, 22*72, 
    1440, 720, 
    CANON_INK_CMYK | CANON_INK_CcMmYK, 
    CANON_SLOT_ASF1, 
    CANON_CAP_CMD61 | CANON_CAP_DMT 
  },
  { /* Canon BJC 8200 */
    8200, 
    11*72, 17*72, 
    1200,1200, 
    CANON_INK_CMYK | CANON_INK_CcMmYK, 
    CANON_SLOT_ASF1, 
    0 
  },
  { /* Canon BJC 8500 */
    8500, 
    17*72, 22*72, 
    1200,1200, 
    CANON_INK_CMYK | CANON_INK_CcMmYK, 
    CANON_SLOT_ASF1, 
    0 
  },
};




static canon_cap_t canon_get_model_capabilities(int model)
{
  int i;
  int models= sizeof(canon_model_capabilities) / sizeof(canon_cap_t);
  for (i=0; i<models; i++) {
    if (canon_model_capabilities[i].model == model) {
      return canon_model_capabilities[i];
    }
  }
  fprintf(stderr,"canon: model %d not found in capabilities list.\n",model);
  return canon_model_capabilities[0];
}

static int
canon_media_type(const char *name, canon_cap_t caps) 
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

  fprintf(stderr,"canon: Unknown media type '%s' - reverting to plain\n",name);
  return 1;
}

static int 
canon_source_type(const char *name, canon_cap_t caps)
{
  if (!strcmp(name,"Auto Sheet Feeder"))    return 4;
  if (!strcmp(name,"Manual with Pause"))    return 0;
  if (!strcmp(name,"Manual without Pause")) return 1;

  fprintf(stderr,"canon: Unknown source type '%s' - reverting to auto\n",name);
  return 4;
}

static int 
canon_printhead_type(const char *name, canon_cap_t caps)
{
  if (!strcmp(name,"Black"))       return 0;
  if (!strcmp(name,"Color"))       return 1;
  if (!strcmp(name,"Black/Color")) return 2;
  if (!strcmp(name,"Photo/Color")) return 3;
  if (!strcmp(name,"Photo"))       return 4;

  fprintf(stderr,"canon: Unknown head combo '%s' - reverting to black\n",name);
  return 0;
}

static unsigned char
canon_size_type(const char *name, canon_cap_t caps)
{
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
  if (!strcmp(name,"Canon 4x2"))   return 0x2d;

  /* custom */
  
  fprintf(stderr,"canon: Unknown paper size '%s' - using custom\n",name);
  return 0; 
}

static char *
c_strdup(const char *s)
{
  char *ret = malloc(strlen(s) + 1);
  strcpy(ret, s);
  return ret;
}

/*
 * 'canon_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
canon_parameters(int  model,		/* I - Printer model */
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
                  ("Glossy Photo Cards")
                };
  static char   *media_sources[] =
                {
                  ("Auto Sheet Feeder"),
                  ("Manual with Pause"),
                  ("Manual without Pause"),
                };

  canon_cap_t caps= canon_get_model_capabilities(model);

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
      if (1200<=x && 600<=y)
	valptrs[c++]= c_strdup("1200x600 DPI");
    } else if (!(caps.max_xdpi%180)) {
      if ( 180<=x && 180<=y)
	valptrs[c++]= c_strdup("180x180 DPI");
      if ( 360<=x && 360<=y)
	valptrs[c++]= c_strdup("360x360 DPI");
      if ( 360<=x && 360<=y && (caps.features&CANON_CAP_DMT))
	valptrs[c++]= c_strdup("360x360 DPI w/ DMT");
      if ( 720<=x && 360<=y)
	valptrs[c++]= c_strdup("720x360 DPI");
      if ( 720<=x && 720<=y)
	valptrs[c++]= c_strdup("720x720 DPI");
      if (1440<=x && 720<=y)
	valptrs[c++]= c_strdup("1440x720 DPI");
    } else {
      fprintf(stderr,"canon: unknown resolution multiplier for model %d\n",
	      caps.model);
      return 0;
    }
    *count= c;
    p= valptrs;
  }
  else if (strcmp(name, "InkType") == 0)
  {
    int c= 0;
    valptrs = malloc(sizeof(char *) * 5);
    if ((caps.inks & CANON_INK_K))
      valptrs[c++]= c_strdup("Black");
    if ((caps.inks & CANON_INK_CMY))
      valptrs[c++]= c_strdup("Color");
    if ((caps.inks & CANON_INK_CMYK))
      valptrs[c++]= c_strdup("Black/Color");
    if ((caps.inks & CANON_INK_CcMmYK))
      valptrs[c++]= c_strdup("Photo/Color");
    if ((caps.inks & CANON_INK_CcMmYy))
      valptrs[c++]= c_strdup("Photo/Color");
    *count = c;
    p = valptrs;
  }
  else if (strcmp(name, "MediaType") == 0)
  {
    *count = 12;
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
 * 'canon_imageable_area()' - Return the imageable area of the page.
 */

void
canon_imageable_area(int  model,	/* I - Printer model */
                     char *ppd_file,	/* I - PPD file (not used) */
                     char *media_size,	/* I - Media size */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */

  default_media_size(model, ppd_file, media_size, &width, &length);

  /* ok for BJC 6000 */
  *left   = 9;
  *right  = width - 9;
  *top    = length- 10;
  *bottom = 18;
}

/*
 * 'canon_cmd()' - Sends a command with variable args
 */
static void
canon_cmd(FILE *prn, /* I - the printer         */
	  char *ini, /* I - 2 bytes start code  */
	  char cmd,  /* I - command code        */
	  int  num,  /* I - number of arguments */
	  ...        /* I - the args themselves */
	  )
{
  /* hopefully not too small: */
  #define CANON_SEND_BUFF_SIZE 10000
  static unsigned char buffer[CANON_SEND_BUFF_SIZE];
  int i;
  va_list ap;
  
  if (num >= CANON_SEND_BUFF_SIZE) {
    fprintf(stderr,"\ncanon: command too large for send buffer!\n");
    fprintf(stderr,"canon: command 0x%02x with %d args dropped\n\n",
	    cmd,num);
    return;
  }
  if (num) {
    va_start(ap, num);
    for (i=0; i<num; i++) 
      buffer[i]= va_arg(ap, unsigned char);
    va_end(ap);
  }

  fwrite(ini,2,1,prn);
  if (cmd) {
    fputc(cmd,prn);
    fputc((num & 255),prn);
    fputc((num >> 8 ),prn);
    fwrite(buffer,num,1,prn);
  }
}

#define PUT(WHAT,VAL,RES) fprintf(stderr,"canon: "WHAT" is %04x =% 5d = %f\" = %f mm\n",(VAL),(VAL),(VAL)/(1.*RES),(VAL)/(RES/25.4))

static void
canon_init_printer(FILE *prn, canon_cap_t caps, 
		   int output_type, char *media_str, 
		   char *size_str, int print_head, 
		   char *source_str, 
		   int xdpi, int ydpi, 
		   int page_width, int page_height,
		   int top, int left,
		   int use_dmt)
{
#define MEDIACODES 11
  static unsigned char mediacode_63[] = {
    0x00,0x00,0x02,0x03,0x04,0x08,0x07,0x03,0x06,0x05,0x05
  };
  static unsigned char mediacode_6c[] = {
    0x00,0x00,0x02,0x03,0x04,0x08,0x07,0x03,0x06,0x05,0x0a
  };
  
  #define ESC28 "\x1b\x28"
  #define ESC5b "\x1b\x5b"
  #define ESC40 "\x1b\x40"

  unsigned char 
    arg_63_1 = 0x00, 
    arg_63_2 = 0x00, /* plain paper */
    arg_63_3 = 0x02, /* seems to be ok for most stuff... */
    arg_6c_1 = 0x00, 
    arg_6c_2 = 0x01, /* plain paper */
    arg_6d_1 = 0x03, /* color printhead? */
    arg_6d_2 = 0x00, /* 00=color  02=b/w */
    arg_6d_3 = 0x00, /* only 01 for bjc8200 */
    arg_6d_a = 0x03, /* A4 paper */
    arg_6d_b = 0x00, 
    arg_70_1 = 0x02, /* A4 printable area */
    arg_70_2 = 0xa6, 
    arg_70_3 = 0x01, 
    arg_70_4 = 0xe0, 
    arg_74_1 = 0x01, /* 1 bit per pixel */
    arg_74_2 = 0x00, /*  */
    arg_74_3 = 0x01; /* 01 <= 360 dpi    09 >= 720 dpi */

  int media= canon_media_type(media_str,caps);
  int source= canon_source_type(source_str,caps);

  int printable_width=  page_width*10/12;
  int printable_height= page_height*10/12;

  arg_6d_a= canon_size_type(size_str,caps);
  if (!arg_6d_a) arg_6d_b= 1;

  if (caps.model<3000) 
    arg_63_1= arg_6c_1= 0x10;
  else 
    arg_63_1= arg_6c_1= 0x30;

  if (output_type==OUTPUT_GRAY) arg_63_1|= 0x01;
  arg_6c_1|= (source & 0x0f);

  if (print_head==0) arg_6d_1= 0x03;
  else if (print_head<=2) arg_6d_1= 0x02;
  else if (print_head<=4) arg_6d_1= 0x04;
  if (output_type==OUTPUT_GRAY) arg_6d_2= 0x02;

  arg_70_1= (printable_height >> 8) & 0xff;
  arg_70_2= (printable_height) & 0xff;
  arg_70_3= (printable_width >> 8) & 0xff;
  arg_70_4= (printable_width) & 0xff;
  
  if (xdpi==1440) arg_74_2= 0x04;
  if (ydpi>=720)  arg_74_3= 0x09;

  if (media<MEDIACODES) {
    arg_63_2= mediacode_63[media];
    arg_6c_2= mediacode_6c[media];
  }

  if (use_dmt) {
    arg_74_1= 0x02;
    arg_74_2= 0x80;
    arg_74_3= 0x09;
  }

  /*
  fprintf(stderr,"canon: printable size = %dx%d (%dx%d) %02x%02x %02x%02x\n",
	  page_width,page_height,printable_width,printable_height,
	  arg_70_1,arg_70_2,arg_70_3,arg_70_4);
  */

  /* init printer */

  canon_cmd(prn,ESC5b,0x4b, 2, 0x00,0x0f);
  if (caps.features & CANON_CAP_CMD61) 
    canon_cmd(prn,ESC5b,0x61, 1, 0x00,0x01);
  canon_cmd(prn,ESC28,0x62, 1, 0x01);
  canon_cmd(prn,ESC28,0x71, 1, 0x01);

  canon_cmd(prn,ESC28,0x6d,12, arg_6d_1,0xff,0xff,0x00,0x00,0x07,
	                       0x00,arg_6d_a,arg_6d_b,arg_6d_2,
	                       0x00,arg_6d_3);

  /* set resolution */

  canon_cmd(prn,ESC28,0x64, 4, (ydpi >> 8 ), (ydpi & 255), 
	                        (xdpi >> 8 ), (xdpi & 255));

  canon_cmd(prn,ESC28,0x74, 3, arg_74_1, arg_74_2, arg_74_3);

  canon_cmd(prn,ESC28,0x63, 3, arg_63_1, arg_63_2, arg_63_3);
  canon_cmd(prn,ESC28,0x70, 8, arg_70_1, arg_70_2, 0x00, 0x00, 
                               arg_70_3, arg_70_4, 0x00, 0x00);
  canon_cmd(prn,ESC28,0x6c, 2, arg_6c_1, arg_6c_2);

  /* some linefeeds */

  top= (top*ydpi)/72; 
  PUT("topskip ",top,ydpi);
  canon_cmd(prn,ESC28,0x65, 2, (top >> 8 ),(top & 255));
}

/*
 *  'alloc_buffer()' allocates buffer and fills it with 0
 */ 
static unsigned char *canon_alloc_buffer(int size)
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
canon_advance_buffer(unsigned char *buf, int len, int num)
{
  if (!buf || !len) return;
  if (num>0) memmove(buf+len,buf,len*num);
  memset(buf,0,len);
}

/*
 * 'canon_print()' - Print an image to an CANON printer.
 */
static void
canon_print(int       model,		/* I - Model */
            int       copies,		/* I - Number of copies */
            FILE      *prn,		/* I - File to print to */
	    Image     image,		/* I - Image to print */
            unsigned char    *cmap,	/* I - Colormap (for indexed images) */
	    vars_t    *v)
{
  char 		*ppd_file = v->ppd_file;
  char 		*resolution = v->resolution;
  char 		*media_size = v->media_size;
  char          *media_type = v->media_type;
  char          *media_source = v->media_source;
  char          *ink_type = v->ink_type;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
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
		temp_width,	/* Temporary width of image on page */
		temp_height,	/* Temporary height of image on page */
		landscape,	/* True if we rotate the output 90 degrees */
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
  /*
  double        the_levels[] = { 0.0, 0.5, 0.75, 1.0 };
  */

  canon_cap_t caps= canon_get_model_capabilities(model);
  int printhead= canon_printhead_type(ink_type,caps);

  /*
  PUT("top        ",top,72);
  PUT("left       ",left,72);
  */

  /*
  * Setup a read-only pixel region for the entire image...
  */

  Image_init(image);
  image_height = Image_height(image);
  image_width = Image_width(image);
  image_bpp = Image_bpp(image);
  
  /* force grayscale if image is grayscale 
   *                 or single black cartridge installed
   */

  if (printhead == 0 || caps.inks == CANON_INK_K)
    output_type = OUTPUT_GRAY;
  else if (image_bpp < 3 && cmap == NULL && output_type == OUTPUT_COLOR)
    output_type = OUTPUT_GRAY_COLOR;

  /*
   * Choose the correct color conversion function...
   */

  if (output_type == OUTPUT_COLOR) {
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
  } else {
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

  sscanf(resolution,"%dx%d",&xdpi,&ydpi);
  fprintf(stderr,"canon: resolution=%dx%d\n",xdpi,ydpi);

  if (!strcmp(resolution+(strlen(resolution)-3),"DMT") && 
      (caps.features & CANON_CAP_DMT)) {
    use_dmt= 1;
    fprintf(stderr,"canon: using drop modulation technology\n");
  }

 /*
  * Compute the output size...
  */

  landscape   = 0;
  canon_imageable_area(model, ppd_file, media_size, &page_left, &page_right,
                       &page_top, &page_bottom);

  page_width  = page_right - page_left;
  page_height = page_top - page_bottom;
  if (page_width<0) page_width= -page_width;
  if (page_height<0) page_height= -page_height;

  default_media_size(model, ppd_file, media_size, &n, &page_length);

  /*
  PUT("top        ",top,72);
  PUT("left       ",left,72);
  PUT("page_top   ",page_top,72);
  PUT("page_bottom",page_bottom,72);
  PUT("page_left  ",page_left,72);
  PUT("page_right ",page_right,72);
  PUT("page_width ",page_width,72);
  PUT("page_height",page_height,72);
  PUT("page_length",page_length,72);
  */

 /*
  * Portrait width/height...
  */

  if (scaling < 0.0) {
    /* Scale to pixels per inch... */
    out_width  = image_width * -72.0 / scaling;
    out_height = image_height * -72.0 / scaling;
  } else {
    /* Scale by percent... */
    out_width  = page_width * scaling / 100.0;
    out_height = out_width * image_height / image_width;
    if (out_height > page_height) {
      out_height = page_height * scaling / 100.0;
      out_width  = out_height * image_width / image_height;
    }
  }

  if (out_width == 0)  out_width = 1;
  if (out_height == 0) out_height = 1;

 /*
  * Landscape width/height...
  */

  if (scaling < 0.0) {
    /* Scale to pixels per inch... */
    temp_width  = image_height * -72.0 / scaling;
    temp_height = image_width * -72.0 / scaling;
  } else {
    /* Scale by percent... */
    temp_width  = page_width * scaling / 100.0;
    temp_height = temp_width * image_width / image_height;
    if (temp_height > page_height) {
      temp_height = page_height;
      temp_width  = temp_height * image_height / image_width;
    }
  }

 /*
  * See which orientation has the greatest area (or if we need to rotate the
  * image to fit it on the page...)
  */

  if (orientation == ORIENT_AUTO) {
    if (scaling < 0.0) {
      if ((out_width > page_width && out_height < page_width) ||
          (out_height > page_height && out_width < page_height))
	orientation = ORIENT_LANDSCAPE;
      else
	orientation = ORIENT_PORTRAIT;
    } else {
      if ((temp_width * temp_height) > (out_width * out_height))
	orientation = ORIENT_LANDSCAPE;
      else
	orientation = ORIENT_PORTRAIT;
    }
  }

  if (orientation == ORIENT_LANDSCAPE) {
    out_width  = temp_width;
    out_height = temp_height;
    landscape  = 1;

    /* Swap left/top offsets... */
    x    = top;
    top  = left;
    left = page_width - x - out_width;
  }

  if (left < 0)
    left = (page_width - out_width) / 2 + page_left;
  else
    left = left /*+ page_left*/;

  if (top < 0)
    top  = (page_height + out_height) / 2 + page_top;
  else
    top = top /*+ page_top*/;

  /*
  PUT("top        ",top,72);
  PUT("left       ",left,72);
  PUT("page_top   ",page_top,72);
  PUT("page_bottom",page_bottom,72);
  PUT("page_left  ",page_left,72);
  PUT("page_right ",page_right,72);
  PUT("page_width ",page_width,72);
  PUT("page_height",page_height,72);
  PUT("page_length",page_length,72);
  PUT("out_width ", out_width,xdpi);
  PUT("out_height", out_height,ydpi);
  */

  Image_progress_init(image);

  PUT("top     ",top,72);
  PUT("left    ",left,72);

  canon_init_printer(prn, caps, output_type, media_type, 
		     media_size, printhead, media_source, 
		     xdpi, ydpi, page_width, page_height,
		     top,left,use_dmt);

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  /*
  PUT("out_width ", out_width,xdpi);
  PUT("out_height", out_height,ydpi);
  */

  left = ydpi * left / 72;

  PUT("leftskip",left,xdpi);

  if(xdpi==1440){
    delay_k= 0;
    delay_c= 110;
    delay_m= 220;
    delay_y= 330;
    delay_lc= 0;
    delay_lm= 0;
    delay_ly= 0;
    delay_max= 330;
    fprintf(stderr,"canon: delay on!\n");
  } else {
    delay_k= delay_c= delay_m= delay_y= delay_lc= delay_lm= delay_ly=0;
    delay_max=0;
    fprintf(stderr,"canon: delay off!\n");
  }

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  if (use_dmt) {
    buf_length= length*2;
  } else {
    buf_length= length;
  }

  if (output_type == OUTPUT_GRAY) {
    black   = canon_alloc_buffer(buf_length*(delay_k+1));
    cyan    = NULL;
    magenta = NULL;
    lcyan   = NULL;
    lmagenta= NULL;
    yellow  = NULL;
    lyellow = NULL;
  } else {
    cyan    = canon_alloc_buffer(buf_length*(delay_c+1));
    magenta = canon_alloc_buffer(buf_length*(delay_m+1));
    yellow  = canon_alloc_buffer(buf_length*(delay_y+1));
  
    if ((caps.inks & CANON_INK_BLACK_MASK))
      black = canon_alloc_buffer(buf_length*(delay_k+1));
    else
      black = NULL;

    if (printhead==3 && (caps.inks & (CANON_INK_PHOTO_MASK))) {
      lcyan = canon_alloc_buffer(buf_length*(delay_lc+1));
      lmagenta = canon_alloc_buffer(buf_length*(delay_lm+1));
      if ((caps.inks & CANON_INK_CcMmYy)) 
	lyellow = canon_alloc_buffer(buf_length*(delay_lc+1));
      else 
	lyellow = NULL;
    } else {
      lcyan = NULL;
      lmagenta = NULL;
      lyellow = NULL;
    }
  }
  fprintf(stderr,"canon: driver will use colors ");
  if (cyan)     fputc('C',stderr);
  if (lcyan)    fputc('c',stderr);
  if (magenta)  fputc('M',stderr);
  if (lmagenta) fputc('m',stderr);
  if (yellow)   fputc('Y',stderr);
  if (lyellow)  fputc('y',stderr);
  if (black)    fputc('K',stderr);
  fprintf(stderr,"\n");

  if (landscape)
    dither = init_dither(image_height, out_width, 1);
  else
    dither = init_dither(image_width, out_width, 1);

  /*
  if (use_dmt) {
    if (cyan)     dither_set_c_levels(dither,4,the_levels);
    if (lcyan)    dither_set_lc_levels(dither,4,the_levels);
    if (magenta)  dither_set_m_levels(dither,4,the_levels);
    if (lmagenta) dither_set_lm_levels(dither,4,the_levels);
    if (yellow)   dither_set_y_levels(dither,4,the_levels);
    if (lyellow)  dither_set_ly_levels(dither,4,the_levels);
    if (black)    dither_set_k_levels(dither,4,the_levels);
  }
  */    

 /*
  * Output the page, rotating as necessary...
  */

  if (landscape) {
    in  = malloc(image_height * image_bpp);
    out = malloc(image_height * out_bpp * 2);

    errdiv  = image_width / out_height;
    errmod  = image_width % out_height;
    errval  = 0;
    errlast = -1;
    errline  = image_width - 1;
    
    for (x = 0; x < out_height; x ++) {
      if ((x & 255) == 0)
 	Image_note_progress(image, x, out_height);

      if (errline != errlast) {
        errlast = errline;
	Image_get_col(image, in, errline);
      }

      (*colorfunc)(in, out, image_height, image_bpp, cmap, v);
      
      if (output_type == OUTPUT_GRAY && use_dmt) {
	dither_black4(out, x, dither, black);

      } else if (output_type == OUTPUT_GRAY) {
	dither_black(out, x, dither, black);
	
      } else if (use_dmt) {
	dither_cmyk4(out, x, dither, cyan, lcyan, magenta, lmagenta,
		     yellow, lyellow, black);

      } else {
	dither_cmyk(out, x, dither, cyan, lcyan, magenta, lmagenta,
		    yellow, lyellow, black);
      }

      /* fprintf(stderr,"."); */

      canon_write_line(prn, caps, ydpi,
		       black,    delay_k,
		       cyan,     delay_c, 
		       magenta,  delay_m, 
		       yellow,   delay_y, 
		       lcyan,    delay_lc, 
		       lmagenta, delay_lm,
		       lyellow,  delay_ly, 
		       length, out_width, left, use_dmt);

      /* fprintf(stderr,"!"); */
 
      canon_advance_buffer(black,   buf_length,delay_k);
      canon_advance_buffer(cyan,    buf_length,delay_c);
      canon_advance_buffer(magenta, buf_length,delay_m);
      canon_advance_buffer(yellow,  buf_length,delay_y);
      canon_advance_buffer(lcyan,   buf_length,delay_lc);
      canon_advance_buffer(lmagenta,buf_length,delay_lm);
      canon_advance_buffer(lyellow, buf_length,delay_ly);

      errval += errmod;
      errline -= errdiv;
      if (errval >= out_height) {
        errval -= out_height;
        errline --;
      }
    }
  } 
  else /* portrait */
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

      (*colorfunc)(in, out, image_width, image_bpp, cmap, v);

      if (output_type == OUTPUT_GRAY && use_dmt) {
	dither_black4(out, y, dither, black);

      } else if (output_type == OUTPUT_GRAY) {
	dither_black(out, y, dither, black);

      } else if (use_dmt) {
	dither_cmyk4(out, y, dither, cyan, lcyan, magenta, lmagenta,
		     yellow, lyellow, black);

      } else {
	dither_cmyk(out, y, dither, cyan, lcyan, magenta, lmagenta,
		    yellow, lyellow, black);

      /* fprintf(stderr,","); */

      canon_write_line(prn, caps, ydpi,
		       black,    delay_k,
		       cyan,     delay_c, 
		       magenta,  delay_m, 
		       yellow,   delay_y, 
		       lyellow,  delay_ly, 
		       lcyan,    delay_lc, 
		       lmagenta, delay_lm,
		       length, out_width, left, use_dmt);

      /* fprintf(stderr,"!"); */

      canon_advance_buffer(black,   buf_length,delay_k);
      canon_advance_buffer(cyan,    buf_length,delay_c);
      canon_advance_buffer(magenta, buf_length,delay_m);
      canon_advance_buffer(yellow,  buf_length,delay_y);
      canon_advance_buffer(lcyan,   buf_length,delay_lc);
      canon_advance_buffer(lmagenta,buf_length,delay_lm);
      canon_advance_buffer(lyellow, buf_length,delay_ly);

      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline ++;
      }
    }
  }
  free_dither(dither);

  /*
   * Flush delayed buffers...
   */

  if (delay_max) {
    fprintf(stderr,"\ncanon: flushing %d possibly delayed buffers\n",
	    delay_max);
    for (y= 0; y<delay_max; y++) {

      canon_write_line(prn, caps, ydpi,
		       black,    delay_k,
		       cyan,     delay_c, 
		       magenta,  delay_m, 
		       yellow,   delay_y, 
		       lcyan,    delay_lc, 
		       lmagenta, delay_lm,
		       lyellow,  delay_ly, 
		       length, out_width, left, use_dmt);

      /* fprintf(stderr,"-"); */

      canon_advance_buffer(black,   buf_length,delay_k);
      canon_advance_buffer(cyan,    buf_length,delay_c);
      canon_advance_buffer(magenta, buf_length,delay_m);
      canon_advance_buffer(yellow,  buf_length,delay_y);
      canon_advance_buffer(lcyan,   buf_length,delay_lc);
      canon_advance_buffer(lmagenta,buf_length,delay_lm);
      canon_advance_buffer(lyellow, buf_length,delay_ly);
    }
  }

 /*
  * Cleanup...
  */

  free(in);
  free(out);

  if (black != NULL)    free(black);
  if (cyan != NULL)     free(cyan);
  if (magenta != NULL)  free(magenta);
  if (yellow != NULL)   free(yellow);
  if (lcyan != NULL)    free(lcyan);
  if (lmagenta != NULL) free(lmagenta);
  if (lyellow != NULL)  free(lyellow);

  /* eject page */
  fputc(0x0c,prn); 

  /* say goodbye */
  canon_cmd(prn,ESC28,0x62,1,0);
  if (caps.features & CANON_CAP_CMD61) 
    canon_cmd(prn,ESC5b,0x61, 1, 0x00,0x00);
  canon_cmd(prn,ESC40,0,0);
  fflush(prn);
}

/*
 * 'canon_fold_lsb_msb()' fold 2 lines in order lsb/msb
 */

static void
canon_fold_lsb_msb(const unsigned char *line,
		   int single_length,
		   unsigned char *outbuf)
{
  int i;
  for (i = 0; i < single_length; i++) {
    outbuf[0] =
      ((line[0] & (1 << 7)) >> 1) |
      ((line[0] & (1 << 6)) >> 2) |
      ((line[0] & (1 << 5)) >> 3) |
      ((line[0] & (1 << 4)) >> 4) |
      ((line[single_length] & (1 << 7)) >> 0) |
      ((line[single_length] & (1 << 6)) >> 1) |
      ((line[single_length] & (1 << 5)) >> 2) |
      ((line[single_length] & (1 << 4)) >> 3);
    outbuf[1] =
      ((line[0] & (1 << 3)) << 3) |
      ((line[0] & (1 << 2)) << 2) |
      ((line[0] & (1 << 1)) << 1) |
      ((line[0] & (1 << 0)) << 0) |
      ((line[single_length] & (1 << 3)) << 4) |
      ((line[single_length] & (1 << 2)) << 3) |
      ((line[single_length] & (1 << 1)) << 2) |
      ((line[single_length] & (1 << 0)) << 1);
    line++;
    outbuf += 2;
  }
}

/*
 * 'canon_fold_msb_lsb()' fold 2 lines in order msb/lsb
 */

static void
canon_fold_msb_lsb(const unsigned char *line,
		   int single_length,
		   unsigned char *outbuf)
{
  int i;
  for (i = 0; i < single_length; i++) {
    outbuf[0] =
      ((line[0] & (1 << 7)) >> 0) |
      ((line[0] & (1 << 6)) >> 1) |
      ((line[0] & (1 << 5)) >> 2) |
      ((line[0] & (1 << 4)) >> 3) |
      ((line[single_length] & (1 << 7)) >> 1) |
      ((line[single_length] & (1 << 6)) >> 2) |
      ((line[single_length] & (1 << 5)) >> 3) |
      ((line[single_length] & (1 << 4)) >> 4);
    outbuf[1] =
      ((line[0] & (1 << 3)) << 4) |
      ((line[0] & (1 << 2)) << 3) |
      ((line[0] & (1 << 1)) << 2) |
      ((line[0] & (1 << 0)) << 1) |
      ((line[single_length] & (1 << 3)) << 3) |
      ((line[single_length] & (1 << 2)) << 2) |
      ((line[single_length] & (1 << 1)) << 1) |
      ((line[single_length] & (1 << 0)) << 0);
    line++;
    outbuf += 2;
  }
}

static void
canon_pack(unsigned char *line,
	   int length,
	   unsigned char *comp_buf,
	   unsigned char **comp_ptr)
{
  unsigned char *start;			/* Start of compressed data */
  unsigned char repeat;			/* Repeating char */
  int count;			/* Count of compressed bytes */
  int tcount;			/* Temporary count < 128 */

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
}

	   
/*
 * 'canon_write()' - Send graphics using TIFF packbits compression.
 */

static int
canon_write(FILE          *prn,		/* I - Print file or command */
	    canon_cap_t   caps,	        /* I - Printer model */
	    unsigned char *line,	/* I - Output bitmap data */
	    int           length,	/* I - Length of bitmap data */
	    int           coloridx,	/* I - Which color */
	    int           ydpi,		/* I - Vertical resolution */
	    int           *empty,       /* IO- Preceeding empty lines */
	    int           width,	/* I - Printed width */
	    int           offset, 	/* I - Offset from left side */
	    int           dmt)        
{
  unsigned char	
    comp_buf[COMPBUFWIDTH],		/* Compression buffer */
    in_fold[COMPBUFWIDTH],
    *in_ptr= line,
    *comp_ptr, *comp_data;
  int newlength;

 /* Don't send blank lines... */

  if (line[0] == 0 && memcmp(line, line + 1, length - 1) == 0)
    return 0;

  /* fold lsb/msb pairs if drop modulation is active */

  if (dmt) {
    if (1) {
      if (caps.features & CANON_CAP_MSB_FIRST) 
	canon_fold_msb_lsb(line,length,in_fold);
      else
	canon_fold_lsb_msb(line,length,in_fold);
      in_ptr= in_fold;
    }
    length*= 2;
    offset*= 2;
  } 

  /* pack left border rounded to multiples of 8 dots */

  comp_data= comp_buf;
  if (offset) {
    int offset2= (offset+4)/8;
    while (offset2>0) {
      unsigned char toffset = offset2 > 128 ? 128 : offset2;
      comp_data[0] = 1 - toffset;
      comp_data[1] = 0;
      comp_data += 2;
      offset2-= toffset;
    }
  }

  canon_pack(in_ptr, length, comp_data, &comp_ptr);
  newlength= comp_ptr - comp_buf;

  /* send packed empty lines if any */

  if (*empty) {
    /* fprintf(stderr,"<%d%c>",*empty,("CMYKcmy"[coloridx])); */
    fwrite("\x1b\x28\x65\x02\x00", 5, 1, prn);
    fputc((*empty) >> 8 , prn);
    fputc((*empty) & 255, prn);
    *empty= 0;
  }

 /* Send a line of raster graphics... */

  fwrite("\x1b\x28\x41", 3, 1, prn);
  putc((newlength+1) & 255, prn);
  putc((newlength+1) >> 8, prn);
  putc("CMYKcm"[coloridx],prn);
  fwrite(comp_buf, newlength, 1, prn);
  putc('\x0d', prn);
  return 1;
}


static void
canon_write_line(FILE          *prn,	/* I - Print file or command */
		 canon_cap_t   caps,	/* I - Printer model */
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

  if (k) written+= 
    canon_write(prn, caps, k+ dk*l,  l, 3, ydpi, &empty, width, offset, dmt);
  if (y) written+= 
    canon_write(prn, caps, y+ dy*l,  l, 2, ydpi, &empty, width, offset, dmt);
  if (m) written+= 
    canon_write(prn, caps, m+ dm*l,  l, 1, ydpi, &empty, width, offset, dmt);
  if (c) written+= 
    canon_write(prn, caps, c+ dc*l,  l, 0, ydpi, &empty, width, offset, dmt);
  if (ly) written+= 
    canon_write(prn, caps, ly+dly*l, l, 6, ydpi, &empty, width, offset, dmt);
  if (lm) written+= 
    canon_write(prn, caps, lm+dlm*l, l, 5, ydpi, &empty, width, offset, dmt);
  if (lc) written+= 
    canon_write(prn, caps, lc+dlc*l, l, 4, ydpi, &empty, width, offset, dmt);

  if (written)
    fwrite("\x1b\x28\x65\x02\x00\x00\x01", 7, 1, prn);
  else 
    empty++;
}

/*
 * End of "$Id$".
 */
