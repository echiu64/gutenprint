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
 *   canon_parameters()     - Return the parameter values for the given
 *                            parameter.
 *   canon_imageable_area() - Return the imageable area of the page.
 *   canon_print()          - Print an image to an EPSON printer.
 *   canon_write()          - Send 6-color graphics using tiff compression.
 *
 * Revision History:
 *
 *   $Log$
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

static int  canon_write(FILE *, unsigned char *, int, int, int, int, int,
			int*, int, int);

static void canon_write_line(FILE *,
			     unsigned char *,
			     int           ,
			     unsigned char *,
			     int           ,
			     unsigned char *,
			     int           ,
			     unsigned char *,
			     int           ,
			     unsigned char *,
			     int           ,
			     unsigned char *,
			     int           ,
			     int           ,
			     int           ,
			     int           ,
			     int           ,
			     int           );



/*
 * Printer capabilities.
 *
 * Various classes of printer capabilities are represented by bitmasks.
 */

typedef unsigned long long model_cap_t;
typedef model_cap_t model_featureset_t;
typedef model_cap_t model_class_t;

#define MODEL_PAPER_SIZE_MASK	0x3
#define MODEL_PAPER_SMALL 	0x0
#define MODEL_PAPER_LARGE 	0x1
#define MODEL_PAPER_1200	0x2

#define MODEL_IMAGEABLE_MASK	0xc
#define MODEL_IMAGEABLE_DEFAULT	0x0
#define MODEL_IMAGEABLE_PHOTO	0x4
#define MODEL_IMAGEABLE_600	0x8

#define MODEL_INIT_MASK		0xf0
#define MODEL_INIT_COLOR	0x00
#define MODEL_INIT_PRO		0x10
#define MODEL_INIT_1500		0x20
#define MODEL_INIT_600		0x30
#define MODEL_INIT_PHOTO	0x40
#define MODEL_INIT_440		0x50

#define MODEL_HASBLACK_MASK	0x100
#define MODEL_HASBLACK_YES	0x000
#define MODEL_HASBLACK_NO	0x100

#define MODEL_6COLOR_MASK	0x200
#define MODEL_6COLOR_NO		0x000
#define MODEL_6COLOR_YES	0x200

#define MODEL_720DPI_MODE_MASK	0xc00
#define MODEL_720DPI_DEFAULT	0x000
#define MODEL_720DPI_600	0x400
#define MODEL_720DPI_PHOTO	0x400 /* 0x800 for experimental stuff */

#define MODEL_1440DPI_MASK	0x1000
#define MODEL_1440DPI_NO	0x0000
#define MODEL_1440DPI_YES	0x1000

#define MODEL_VARIABLE_DOT_MASK	0x6000
#define MODEL_VARIABLE_NORMAL	0x0000
#define MODEL_VARIABLE_4	0x2000

#define MODEL_NOZZLES_MASK	0xff000000
#define MODEL_MAKE_NOZZLES(x) 	((long long) ((x)) << 24)
#define MODEL_GET_NOZZLES(x) 	(((x) & MODEL_NOZZLES_MASK) >> 24)
#define MODEL_SEPARATION_MASK	0xf00000000l
#define MODEL_MAKE_SEPARATION(x) 	(((long long) (x)) << 32)
#define MODEL_GET_SEPARATION(x)	(((x) & MODEL_SEPARATION_MASK) >> 32)


#define PHYSICAL_BPI 720
#define MAX_OVERSAMPLED 4
#define MAX_BPP 2
#define BITS_PER_BYTE 8
#define COMPBUFWIDTH (PHYSICAL_BPI * MAX_OVERSAMPLED * MAX_BPP * \
	MAX_CARRIAGE_WIDTH / BITS_PER_BYTE)

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


/*
 * A lot of these are guesses
 */

model_cap_t canon_model_capabilities[] =
{
  /* BJC-6000 */
  (MODEL_PAPER_SMALL | MODEL_IMAGEABLE_DEFAULT | MODEL_INIT_COLOR
   | MODEL_HASBLACK_YES | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT
   | MODEL_VARIABLE_NORMAL
   | MODEL_1440DPI_YES | MODEL_MAKE_NOZZLES(1) | MODEL_MAKE_SEPARATION(1)),
};

typedef struct {
  const char name[65];
  int hres;
  int vres;
  int softweave;
  int horizontal_passes;
  int vertical_passes;
} canon_res_t;

static int
canon_has_cap(int model, model_featureset_t featureset, model_class_t class)
{
  return ((canon_model_capabilities[model] & featureset) == class);
}

static model_class_t
canon_cap(int model, model_featureset_t featureset)
{
  return (canon_model_capabilities[model] & featureset);
}


static int
canon_media_type(const char *name, int model) 
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
  return 1;
}

static int 
canon_source_type(const char *name, int model)
{
  if (!strcmp(name,"Auto Sheet Feeder"))    return 4;
  if (!strcmp(name,"Manual with Pause"))    return 0;
  if (!strcmp(name,"Manual without Pause")) return 1;
  return 4;
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
  char		**p,
                **valptrs;
  static char	*media_sizes[] =
		{
		  ("A5"),
		  ("A4"),
		  ("B5"),
		  ("Letter"),
		  ("Legal"),
		  ("Envelope 10"),
		  ("Envelope DL"),
		  ("Letter+"),
		  ("A4+"),
		  ("Canon 4x2")
		};
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
  static char   *resolutions[] =
                {
                  ("90x90 DPI"),
                  ("180x180 DPI"),
                  ("360x360 DPI"),
                  ("720x720 DPI"),
                  ("1440x720 DPI")
                };

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0)
  {
    *count = 10;
    p = media_sizes;
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
  else if (strcmp(name, "Resolution") == 0)
  {
    *count = 5;
    p = resolutions;
  }
  else
    return (NULL);

  valptrs = malloc(*count * sizeof(char *));
  for (i = 0; i < *count; i ++)
    {
      /* strdup doesn't appear to be POSIX... */
      valptrs[i] = malloc(strlen(p[i]) + 1);
      strcpy(valptrs[i], p[i]);
    }

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

  *left   = 9;
  *right  = width - 9;
  *top    = length;
  *bottom = 80;
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
  #define CANON_SEND_BUFF_SIZE 10000
  static unsigned char buffer[CANON_SEND_BUFF_SIZE];
  int i;
  va_list ap;
  
  if (num >= CANON_SEND_BUFF_SIZE) {
    fprintf(stderr,"\ncanon: command too arge for send buffer!\n");
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

static void
canon_init_printer(FILE *prn, int model, 
		   int output_type, char *media_str, 
		   char *format_str, int print_head, 
		   char *source_str, int xdpi, 
		   int ydpi, int page_length, int page_top, int page_bottom, 
		   int top)
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
    arg_70_1 = 0x02, 
    arg_70_2 = 0xa6, 
    arg_70_3 = 0x01, 
    arg_70_4 = 0xe0, 
    arg_74_1 = 0x01, /* 1 bit per pixel */
    arg_74_2 = 0x00, /*  */
    arg_74_3 = 0x01; /* 01 <= 360 dpi    09 >= 720 dpi */

  int media= canon_media_type(media_str,model);
  int source= canon_source_type(source_str,model);


  fprintf(stderr,"canon: output_type=%d\n",output_type);
  fprintf(stderr,"canon: res: %dx%d = 0x%02x 0x%02x  0x%02x 0x%02x\n",
	  ydpi,xdpi,(ydpi >> 8 ),(ydpi & 255),(xdpi >> 8 ),(xdpi & 255));
  fprintf(stderr,"canon: page-top is %04x = %f\" = %f mm\n",
	  page_top,page_top/(1.*ydpi),page_top/(ydpi/25.4));



  if (model<3000) arg_63_1= arg_6c_1= 0x10; 
             else arg_63_1= arg_6c_1= 0x30;
  if (output_type==OUTPUT_GRAY) arg_63_1|= 0x01;
  arg_6c_1|= (source & 0x0f);

  fprintf(stderr,"canon: 63_1=0x%02x\n",arg_63_1);

  if (print_head==0) arg_6d_1= 0x02;
  else if (print_head<=2) arg_6d_1= 0x03;
  else if (print_head<=4) arg_6d_1= 0x04;
  if (output_type==OUTPUT_GRAY) arg_6d_2= 0x02;

  if (model==3000||model==6100||model==8200) arg_70_1= 0xab;
  if (model==6000 && media==10) { 
    arg_70_1= 0x3a; arg_70_2= 0xd9;
  }

  if (xdpi==1440) arg_74_2= 0x04;
  if (ydpi>=720)  arg_74_3= 0x09;

  if (media<MEDIACODES) {
    arg_63_2= mediacode_63[media];
    arg_6c_2= mediacode_6c[media];
  }

  /* init printer */
  canon_cmd(prn,ESC5b,0x4b, 2, 0x00,0x0f);
  canon_cmd(prn,ESC28,0x62, 1, 0x01);
  canon_cmd(prn,ESC28,0x71, 1, 0x01);

  canon_cmd(prn,ESC28,0x6d,12, arg_6d_1,0xff,0xff,0x00,0x00,0x07,
	                       0x00,0x03,0x00,arg_6d_2,0x00,arg_6d_3);

  /* set resolution */
  canon_cmd(prn,ESC28,0x64, 4, (ydpi >> 8 ), (ydpi & 255), 
	                        (xdpi >> 8 ), (xdpi & 255));

  canon_cmd(prn,ESC28,0x74, 3, arg_74_1, arg_74_2, arg_74_3);

  canon_cmd(prn,ESC28,0x63, 3, arg_63_1, arg_63_2, arg_63_3);
  canon_cmd(prn,ESC28,0x70, 8, arg_70_1, arg_70_2, 0x00, 0x00, 
                               arg_70_3, arg_70_4, 0x00, 0x00);
  canon_cmd(prn,ESC28,0x6c, 2, arg_6c_1, arg_6c_2);

  /* some linefeeds */
  canon_cmd(prn,ESC28,0x65, 2, (top >> 8 ),(top & 255));

}

/*
 *  'alloc_buffer()' allocates buffer and fills it with 0
 */ 
unsigned char *canon_alloc_buffer(int size)
{
  unsigned char *buf= malloc(size);
  if (buf) memset(buf,0,size);
  return buf;
}

/*
 * 'advance_buffer()' - Move (num) lines of length (len) down one line
 *                      accepts NULL pointers as buf 
 *                      !!! buf must contain > num lines !!!
 *                      only clears line if (num <= 1)
 */
void
canon_advance_buffer(unsigned char *buf, int len, int num)
{
  if (!buf || !len) return;
  if (num>0) memmove(buf+len,buf,len*num);
  memset(buf,0,len);
}

/*
 * 'canon_print()' - Print an image to an EPSON printer.
 */
void
canon_print(int       model,		/* I - Model */
            int       copies,		/* I - Number of copies */
            FILE      *prn,		/* I - File to print to */
	    Image     image,		/* I - Image to print */
            unsigned char    *cmap,	/* I - Colormap (for indexed images) */
	    lut_t     *lut,		/* I - Brightness lookup table */
	    vars_t    *v)
{
  char 		*ppd_file = v->ppd_file;
  char 		*resolution = v->resolution;
  char 		*media_size = v->media_size;
  char          *media_type = v->media_type;
  char          *media_source = v->media_source;
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
		*lcyan,		/* Light cyan bitmap data */
		*lmagenta,	/* Light magenta bitmap data */
		*yellow;	/* Yellow bitmap data */
  int           delay_k,
                delay_c,
                delay_m,
                delay_y,
                delay_lc,
                delay_lm,
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
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  convert_t	colorfunc = 0;	/* Color conversion function... */
  int           image_height,
                image_width,
                image_bpp;

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

  if (image_bpp < 3 && cmap == NULL)
    output_type = OUTPUT_GRAY;		/* Force grayscale output */

  if (output_type == OUTPUT_COLOR)
  {
    out_bpp = 3;

    if (image_bpp >= 3)
      colorfunc = rgb_to_rgb;
    else
      colorfunc = indexed_to_rgb;
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
  sscanf(resolution,"%dx%d",&xdpi,&ydpi);

  fprintf(stderr,"canon resolution=%dx%d\n",xdpi,ydpi);

 /*
  * Compute the output size...
  */

  landscape   = 0;
  canon_imageable_area(model, ppd_file, media_size, &page_left, &page_right,
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
    left = x;
  }

  if (left < 0)
    left = (page_width - out_width) / 2 + page_left;

  if (top < 0)
    top  = (page_height + out_height) / 2 + page_bottom;
  else
    top = page_height - top + page_bottom;

 /*
  * Let the user know what we're doing...
  */

  Image_progress_init(image);




 /*
  * Send CANON initialization commands...
  */

  canon_init_printer(prn, model, output_type, media_type, 
		     media_size, 2, media_source, 
		     xdpi, ydpi, page_length,
		     page_top, page_bottom, top);

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  left = ydpi * (left - page_left) / 72;

  fprintf(stderr,"image-top  is %04x = %f\" = %f mm\n",
          top,top/(1.*ydpi),top/(ydpi/25.4));
  fprintf(stderr,"image-left is %04x = %f\" = %f mm\n",
          left,left/(1.*ydpi),left/(ydpi/25.4));

  if(xdpi==1440){
    delay_k= 0;
    delay_c= 110;
    delay_m= 220;
    delay_y= 330;
    delay_lc= 0;
    delay_lm= 0;
    delay_max= 330;
    fprintf(stderr,"delay on!\n");
  } else {
    delay_k= delay_c= delay_m= delay_y= delay_lc= delay_lm=0;
    delay_max=0;
    fprintf(stderr,"delay off!\n");
  }

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  if (output_type == OUTPUT_GRAY)
  {
    black   = canon_alloc_buffer(length*(delay_k+1));
    cyan    = NULL;
    magenta = NULL;
    lcyan   = NULL;
    lmagenta= NULL;
    yellow  = NULL;
  }
  else
  {
    cyan    = canon_alloc_buffer(length*(delay_c+1));
    magenta = canon_alloc_buffer(length*(delay_m+1));
    yellow  = canon_alloc_buffer(length*(delay_y+1));
  
    if (canon_has_cap(model, MODEL_HASBLACK_MASK, MODEL_HASBLACK_YES))
      black = canon_alloc_buffer(length*(delay_k+1));
    else
      black = NULL;
    if (canon_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES)) {
      lcyan = canon_alloc_buffer(length*(delay_lc+1));
      lmagenta = canon_alloc_buffer(length*(delay_lm+1));
    } else {
      lcyan = NULL;
      lmagenta = NULL;
    }
  }
    
 /*
  * Output the page, rotating as necessary...
  */

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

      (*colorfunc)(in, out, image_height, image_bpp, lut, cmap, v);

      if (output_type == OUTPUT_GRAY)
	dither_black(out, x, image_height, out_width, black,1);
      else
	dither_cmyk(out, x, image_height, out_width, cyan, lcyan,
		    magenta, lmagenta, yellow, 0, black,1);

      fprintf(stderr,".");
      canon_write_line(prn,
		       black,    delay_k,
		       cyan,     delay_c, 
		       magenta,  delay_m, 
		       yellow,   delay_y, 
		       lcyan,    delay_lc, 
		       lmagenta, delay_lm,
		       length, ydpi, model, out_width, left);
      fprintf(stderr,"!");

      canon_advance_buffer(black,   length,delay_k);
      canon_advance_buffer(cyan,    length,delay_c);
      canon_advance_buffer(magenta, length,delay_m);
      canon_advance_buffer(yellow,  length,delay_y);
      canon_advance_buffer(lcyan,   length,delay_lc);
      canon_advance_buffer(lmagenta,length,delay_lm);

      errval += errmod;
      errline -= errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline --;
      }
    }
  }
  else /* ! landscape */
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

      (*colorfunc)(in, out, image_width, image_bpp, lut, cmap, v);

      if (output_type == OUTPUT_GRAY)
	dither_black(out, y, image_width, out_width, black,1);
      else
	dither_cmyk(out, y, image_width, out_width, cyan, lcyan,
		    magenta, lmagenta, yellow, 0, black,1);

      fprintf(stderr,",");

      canon_write_line(prn,
		       black,    delay_k,
		       cyan,     delay_c, 
		       magenta,  delay_m, 
		       yellow,   delay_y, 
		       lcyan,    delay_lc, 
		       lmagenta, delay_lm,
		       length, ydpi, model, out_width, left);
      fprintf(stderr,"!");

      canon_advance_buffer(black,   length,delay_k);
      canon_advance_buffer(cyan,    length,delay_c);
      canon_advance_buffer(magenta, length,delay_m);
      canon_advance_buffer(yellow,  length,delay_y);
      canon_advance_buffer(lcyan,   length,delay_lc);
      canon_advance_buffer(lmagenta,length,delay_lm);

      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline ++;
      }
    }
  }
  
  /*
   * Flush delayed buffers...
   */

  if (delay_max) {
    fprintf(stderr,"\nFlushing %d possibly delayed buffers!\n",delay_max);
    for (y= 0; y<delay_max; y++) {
      canon_write_line(prn,
		       black,    delay_k,
		       cyan,     delay_c, 
		       magenta,  delay_m, 
		       yellow,   delay_y, 
		       lcyan,    delay_lc, 
		       lmagenta, delay_lm,
		       length, ydpi, model, out_width, left);
      fprintf(stderr,"-");

      canon_advance_buffer(black,   length,delay_k);
      canon_advance_buffer(cyan,    length,delay_c);
      canon_advance_buffer(magenta, length,delay_m);
      canon_advance_buffer(yellow,  length,delay_y);
      canon_advance_buffer(lcyan,   length,delay_lc);
      canon_advance_buffer(lmagenta,length,delay_lm);
    }
  }

  fprintf(stderr,"\nDONE!\n");

 /*
  * Cleanup...
  */

  free(in);
  free(out);

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

  /* eject page */
  fputc(0x0c,prn); 

  /* say goodbye */
  canon_cmd(prn,ESC28,0x62,1,0);
  canon_cmd(prn,ESC40,0,0);
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


static void
canon_write_line(FILE          *prn,	/* I - Print file or command */
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
		int           l,	/* I - Length of bitmap data */
		int           ydpi,	/* I - Vertical resolution */
		int           model,	/* I - Printer model */
		int           width,	/* I - Printed width */
		int           offset)
{
  static int empty= 0;
  int written= 0;

  if (c) written+= 
    canon_write(prn,c+ dc*l, l, 0, 0, ydpi, model, &empty, width, offset);
  if (m) written+= 
    canon_write(prn,m+ dm*l, l, 0, 1, ydpi, model, &empty, width, offset);
  if (y) written+= 
    canon_write(prn,y+ dy*l, l, 0, 2, ydpi, model, &empty, width, offset);
  if (k) written+= 
    canon_write(prn,k+ dk*l, l, 0, 3, ydpi, model, &empty, width, offset);
  /*
  if (lc) written+= 
    canon_write(prn,lc+ dlc*l, l, 1, 4, ydpi, model, &empty, width, offset);
  if (lm) written+= 
    canon_write(prn,lm+ dlm*l, l, 1, 5, ydpi, model, &empty, width, offset);
  */

  if (written)
    fwrite("\x1b\x28\x65\x02\x00\x00\x01", 7, 1, prn);
  else 
    empty++;
}
	   
/*
 * 'canon_write()' - Send graphics using TIFF packbits compression.
 */

static int
canon_write(FILE          *prn,		/* I - Print file or command */
	    unsigned char *line,	/* I - Output bitmap data */
	    int           length,	/* I - Length of bitmap data */
	    int	   	  density,      /* I - 0 for dark, 1 for light */
	    int           coloridx,	/* I - Which color */
	    int           ydpi,		/* I - Vertical resolution */
	    int           model,	/* I - Printer model */
	    int           *empty,       /* IO- Preceeding empty lines */
	    int           width,	/* I - Printed width */
	    int           offset) 	/* I - Offset from left side */
{
  unsigned char	comp_buf[COMPBUFWIDTH],		/* Compression buffer */
    *comp_ptr;
  int newlength;

 /*
  * Don't send blank lines...
  */

  if (line[0] == 0 && memcmp(line, line + 1, length - 1) == 0)
    return 0;

  canon_pack(line, length, comp_buf, &comp_ptr);
  newlength= comp_ptr - comp_buf;

  /* send packed empty lines if any */
  if (*empty) {
    fprintf(stderr,"<%d%c>",
	    *empty,("CMYKcm"[coloridx]));
    fwrite("\x1b\x28\x65\x02\x00", 5, 1, prn);
    fputc((*empty) >> 8 , prn);
    fputc((*empty) & 255, prn);
    *empty= 0;
  }

 /*
  * Send a line of raster graphics...
  */

  fwrite("\x1b\x28\x41", 3, 1, prn);
  putc((newlength+1) & 255, prn);
  putc((newlength+1) >> 8, prn);
  putc("CMYKcm"[coloridx],prn);
  fwrite(comp_buf, newlength, 1, prn);
  putc('\x0d', prn);
  return 1;
}


/*
 * End of "$Id$".
 */
