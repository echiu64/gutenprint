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
 *   escp2_parameters()     - Return the parameter values for the given
 *                            parameter.
 *   escp2_imageable_area() - Return the imageable area of the page.
 *   escp2_print()          - Print an image to an EPSON printer.
 *   escp2_write()          - Send 6-color ESC/P2 graphics using TIFF packbits compression.
 *
 * Revision History:
 *
 * See bottom of file
 */

/*
 * Stylus Photo EX added by Robert Krawitz <rlk@alum.mit.edu> August 30, 1999
 */

#ifndef WEAVETEST
#include "print.h"
#endif

#ifdef DEBUG_SIGNAL
#include <signal.h>
#endif

typedef enum {
  COLOR_MONOCHROME,
  COLOR_CMYK,
  COLOR_CCMMYK
} colormode_t;

#ifndef WEAVETEST

/*
 * Local functions...
 */

static void escp2_write(FILE *, const unsigned char *, int, int, int, int, int,
			int, int, int);
static void escp2_write_all(FILE *, const unsigned char *,
			    const unsigned char *, const unsigned char *,
			    const unsigned char *, const unsigned char *,
			    const unsigned char *, int, int, int, int, int,
			    int);
static void *initialize_weave(int jets, int separation, int oversample,
			      int horizontal, int vertical,
			      colormode_t colormode, int width, int linewidth);
static void escp2_flush(void *, int model, int width, int hoffset, int ydpi,
			int xdpi, FILE *prn);
static void
escp2_write_weave(void *, FILE *, int, int, int, int, int, int,
		  const unsigned char *c, const unsigned char *m,
		  const unsigned char *y, const unsigned char *k,
		  const unsigned char *C, const unsigned char *M);

static void destroy_weave(void *);

#define PHYSICAL_BPI 720
#define MAX_OVERSAMPLED 4
#define MAX_BPP 2
#define BITS_PER_BYTE 8
#define COMPBUFWIDTH (PHYSICAL_BPI * MAX_OVERSAMPLED * MAX_BPP * \
	MAX_CARRIAGE_WIDTH / BITS_PER_BYTE)

/*
 * Printer capabilities.
 *
 * Various classes of printer capabilities are represented by bitmasks.
 */

typedef unsigned long long model_cap_t;
typedef unsigned long long model_featureset_t;

typedef struct escp2_printer
{
  model_cap_t	flags;
  int 		nozzles;
  int		nozzle_separation;
  int		black_nozzles;
  int		xres;
  int		softweave_dot_size;
  int		microweave_dot_size;
  int		max_paper_width;
  int		max_paper_height;
  int		left_margin;
  int		right_margin;
  int		top_margin;
  int		bottom_margin;
} escp2_printer_t;

#define MODEL_INIT_MASK		0xf
#define MODEL_INIT_STANDARD	0x0
#define MODEL_INIT_900		0x1

#define MODEL_HASBLACK_MASK	0x10
#define MODEL_HASBLACK_YES	0x00
#define MODEL_HASBLACK_NO	0x10

#define MODEL_6COLOR_MASK	0x20
#define MODEL_6COLOR_NO		0x00
#define MODEL_6COLOR_YES	0x20

#define MODEL_1440DPI_MASK	0x40
#define MODEL_1440DPI_NO	0x00
#define MODEL_1440DPI_YES	0x40

#define MODEL_GRAYMODE_MASK	0x80
#define MODEL_GRAYMODE_NO	0x00
#define MODEL_GRAYMODE_YES	0x80

#define MODEL_720DPI_MODE_MASK	0x300
#define MODEL_720DPI_DEFAULT	0x000
#define MODEL_720DPI_600	0x100
#define MODEL_720DPI_PHOTO	0x100 /* 0x200 for experimental stuff */

#define MODEL_VARIABLE_DOT_MASK	0xc00
#define MODEL_VARIABLE_NORMAL	0x000
#define MODEL_VARIABLE_4	0x400

#define MODEL_COMMAND_MASK	0xf000
#define MODEL_COMMAND_GENERIC	0x0000
#define MODEL_COMMAND_1999	0x1000 /* The 1999 series printers */

#define INCH_8_5	(17 * 72 / 2)
#define INCH_11		(11 * 72)
#define INCH_13		(13 * 72)
#define INCH_14		(14 * 72)
#define INCH_16_5	(33 * 72 / 2)
#define INCH_17		(17 * 72)
#define INCH_19		(19 * 72)
#define INCH_24		(24 * 72)

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

static escp2_printer_t model_capabilities[] =
{
  /* FIRST GENERATION PRINTERS */
  /* 0: Stylus Color */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_NO),
    1, 1, 1, 720, -1, -1, INCH_8_5, INCH_14, 14, 14, 9, 49
  },
  /* 1: Stylus Color Pro/Pro XL/400/500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO | MODEL_1440DPI_NO),
    48, 8, 48, 720, -1, 1, INCH_8_5, INCH_14, 14, 14, 9, 49
  },
  /* 2: Stylus Color 1500 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_NO
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO | MODEL_1440DPI_NO),
    1, 1, 1, 720, -1, 1, INCH_11, INCH_17, 14, 14, 9, 49
  },
  /* 3: Stylus Color 600 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    32, 8, 32, 720, 0, 2, INCH_8_5, INCH_14, 8, 9, 9, 49
  },
  /* 4: Stylus Color 800 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    64, 8, 64, 720, 0, 2, INCH_8_5, INCH_14, 8, 9, 9, 49
  },
  /* 5: Stylus Color 850 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    64, 8, 128, 720, 0, 2, INCH_8_5, INCH_14, 8, 9, 9, 49
  },
  /* 6: Stylus Color 1520/3000 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    64, 8, 128, 720, 0, 2, INCH_16_5, INCH_24, 8, 9, 9, 49
  },

  /* SECOND GENERATION PRINTERS */
  /* 7: Stylus Photo 700 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO | MODEL_1440DPI_YES),
    32, 8, 32, 720, 0, 3, INCH_8_5, INCH_14, 9, 9, 9, 49
  },
  /* 8: Stylus Photo EX */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO | MODEL_1440DPI_YES),
    32, 8, 32, 720, 0, 3, INCH_11, INCH_17, 9, 9, 9, 49
  },
  /* 9: Stylus Photo */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_GENERIC | MODEL_GRAYMODE_NO | MODEL_1440DPI_NO),
    32, 8, 32, 720, 0, 3, INCH_8_5, INCH_14, 9, 9, 9, 49
  },

  /* THIRD GENERATION PRINTERS */
  /* 10: Stylus Color 440/460 */
  /* No, I don't believe the nozzle separation is 7.  Probably it's 8. */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_600 | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_NO),
    21, 7, 64, 720, 2, 3, INCH_8_5, INCH_14, 9, 9, 9, 18
  },
  /* 11: Stylus Color 640 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_NO),
    32, 8, 64, 720, 0, 3, INCH_8_5, INCH_14, 9, 9, 9, 18
  },
  /* 12: Stylus Color 740 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    48, 6, 144, 360, 0, 3, INCH_11, INCH_17, 9, 9, 9, 18
  },
  /* 13: Stylus Color 900 */
  /* Dale Pontius thinks the spacing is 3 jets??? */
  /* No, Eric Sharkey verified that it's 2! */
  {
    (MODEL_INIT_900 | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    96, 2, 192, 360, 0, 1, INCH_11, INCH_17, 9, 9, 9, 18
  },
  /* 14: Stylus Photo 750, 870 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_YES | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO | MODEL_1440DPI_YES),
    48, 6, 48, 360, 0, 4, INCH_8_5, INCH_14, 9, 9, 9, 18
  },
  /* 15: Stylus Photo 1200, 1270 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO | MODEL_VARIABLE_4
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_NO | MODEL_1440DPI_YES),
    48, 6, 48, 360, 0, 4, INCH_13, INCH_19, 9, 9, 9, 18
  },
  /* 16: Stylus Color 860 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    48, 6, 144, 360, 0, 2, INCH_8_5, INCH_14, 9, 9, 9, 18
  },
  /* 17: Stylus Color 1160 */
  {
    (MODEL_INIT_STANDARD | MODEL_HASBLACK_YES
     | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT | MODEL_VARIABLE_NORMAL
     | MODEL_COMMAND_1999 | MODEL_GRAYMODE_YES | MODEL_1440DPI_YES),
    48, 6, 144, 360, 0, 2, INCH_13, INCH_19, 9, 9, 9, 18
  }
};

typedef struct {
  const char name[65];
  int hres;
  int vres;
  int softweave;
  int horizontal_passes;
  int vertical_passes;
  int vertical_subsample;
} res_t;

#ifndef ESCP2_GHOST
static
#endif
const 
res_t escp2_reslist[] = {
  { "360 DPI", 360, 360, 0, 1, 1, 1 },
  { "720 DPI Microweave", 720, 720, 0, 1, 1, 1 },
  { "720 DPI Softweave", 720, 720, 1, 1, 1, 1 },
  { "720 DPI High Quality", 720, 720, 1, 1, 2, 1 },
  { "720 DPI Highest Quality", 720, 720, 1, 1, 4, 1 },
  /* { "1440 x 720 DPI Microweave", 1440, 720, 0, 1, 1, 1 }, */
  { "1440 x 720 DPI Softweave", 1440, 720, 1, 2, 1, 1 },
  { "1440 x 720 DPI Highest Quality", 1440, 720, 1, 2, 2, 1 },
  { "1440 x 1440 DPI Two-pass", 1440, 1440, 1, 2, 1, 2 },
  { "2880 x 720 DPI Two-pass", 2880, 720, 1, 4, 1, 1 },
  /* { "1440 x 720 DPI Two-pass Microweave", 2880, 720, 0, 1, 1 }, */
  { "", 0, 0, 0, 0, 0 }
};

static int
escp2_has_cap(int model, model_featureset_t featureset,
	      model_featureset_t class)
{
  return ((model_capabilities[model].flags & featureset) == class);
}

static model_featureset_t
escp2_cap(int model, model_featureset_t featureset)
{
  return (model_capabilities[model].flags & featureset);
}

static unsigned
escp2_nozzles(int model)
{
  return (model_capabilities[model].nozzles);
}

static unsigned
escp2_black_nozzles(int model)
{
  return (model_capabilities[model].black_nozzles);
}

static unsigned
escp2_nozzle_separation(int model)
{
  return (model_capabilities[model].nozzle_separation);
}

static unsigned
escp2_xres(int model)
{
  return (model_capabilities[model].xres);
}

static int
escp2_soft_ink(int model)
{
  return (model_capabilities[model].softweave_dot_size);
}

static int
escp2_micro_ink(int model)
{
  return (model_capabilities[model].microweave_dot_size);
}

static unsigned
escp2_max_paper_width(int model)
{
  return (model_capabilities[model].max_paper_width);
}

static unsigned
escp2_max_paper_height(int model)
{
  return (model_capabilities[model].max_paper_height);
}

static unsigned
escp2_left_margin(int model)
{
  return (model_capabilities[model].left_margin);
}

static unsigned
escp2_right_margin(int model)
{
  return (model_capabilities[model].right_margin);
}

static unsigned
escp2_top_margin(int model)
{
  return (model_capabilities[model].top_margin);
}

static unsigned
escp2_bottom_margin(int model)
{
  return (model_capabilities[model].bottom_margin);
}

/*
 * 'escp2_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
escp2_parameters(int  model,		/* I - Printer model */
                 char *ppd_file,	/* I - PPD file (not used) */
                 char *name,		/* I - Name of parameter */
                 int  *count)		/* O - Number of values */
{
  int		i;
  char		**valptrs;

  static char *ink_types[] =
  {
    "Variable Dot Size",
    "Single Dot Size"
  };

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0)
    {
      unsigned int length_limit, width_limit;
      const papersize_t *papersizes = get_papersizes();
      valptrs = malloc(sizeof(char *) * known_papersizes());
      *count = 0;
      width_limit = escp2_max_paper_width(model);
      length_limit = escp2_max_paper_height(model);
      for (i = 0; i < known_papersizes(); i++)
	{
	  if (strlen(papersizes[i].name) > 0 &&
	      papersizes[i].width <= width_limit &&
	      papersizes[i].length <= length_limit)
	    {
	      valptrs[*count] = malloc(strlen(papersizes[i].name) + 1);
	      strcpy(valptrs[*count], papersizes[i].name);
	      (*count)++;
	    }
	}
      return (valptrs);
    }
  else if (strcmp(name, "Resolution") == 0)
    {
      const res_t *res = &(escp2_reslist[0]);
      valptrs = malloc(sizeof(char *) * sizeof(escp2_reslist) / sizeof(res_t));
      *count = 0;
      while(res->hres)
	{
	  if (escp2_has_cap(model, MODEL_1440DPI_MASK, MODEL_1440DPI_YES) ||
	      (res->hres <= 720 && res->vres <= 720))
	    {
	      int nozzles = escp2_nozzles(model);
	      int separation = escp2_nozzle_separation(model);
	      int max_weave = nozzles / separation;
	      if ((((720 / escp2_xres(model)) * res->horizontal_passes *
		    res->vertical_passes) <= 4) &&
		  (! res->softweave ||
		   (nozzles > 1 && res->vertical_passes <= max_weave)))
		{
		  valptrs[*count] = malloc(strlen(res->name) + 1);
		  strcpy(valptrs[*count], res->name);
		  (*count)++;
		}
	    }
	  res++;
	}
      return (valptrs);
    }
  else if (strcmp(name, "InkType") == 0)
    {
      if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_NORMAL))
	return NULL;
      else
	{
	  valptrs = malloc(sizeof(char *) * 2);
	  valptrs[0] = malloc(strlen(ink_types[0]) + 1);
	  strcpy(valptrs[0], ink_types[0]);
	  valptrs[1] = malloc(strlen(ink_types[1]) + 1);
	  strcpy(valptrs[1], ink_types[1]);
	  *count = 2;
	  return valptrs;
	}
    }
  else
    return (NULL);

}

/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

void
escp2_imageable_area(int  model,	/* I - Printer model */
                     char *ppd_file,	/* I - PPD file (not used) */
                     char *media_size,	/* I - Media size */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */
  /* Assuming 720 dpi ydpi hardware! */
  /* int softweave_margin = (9 + (escp2_nozzles(model)) *
			  escp2_nozzle_separation(model)) / 10; */

  default_media_size(model, ppd_file, media_size, &width, &length);
  *left =	escp2_left_margin(model);
  *right =	width - escp2_right_margin(model);
  *top =	length - escp2_top_margin(model);
  *bottom =	escp2_bottom_margin(model);
}

static void
escp2_init_printer(FILE *prn,int model, int output_type, int ydpi,
		   int use_softweave, int page_length, int page_width,
		   int page_top, int page_bottom, int top, int nozzles,
		   int nozzle_separation, int horizontal_passes,
		   int vertical_passes, int vertical_subsample, int bits)
{
  int n;
  if (ydpi > 720)
    ydpi = 720;
  /*
   * Hack that seems to be necessary for these silly things to print.
   * No, I don't know what it means. -- rlk
   */
  if (escp2_has_cap(model, MODEL_INIT_MASK, MODEL_INIT_900))
    fprintf(prn, "%c%c%c\033\001@EJL 1284.4\n@EJL     \n\033@", 0, 0, 0);

  fputs("\033@", prn); 				/* ESC/P2 reset */

  if (escp2_has_cap(model, MODEL_COMMAND_MASK, MODEL_COMMAND_1999))
    fprintf(prn,
	    "\033(R\010%c%cREMOTE1PM\002%c%c%cSN\003%c%c%c\001\033%c%c%c",
	    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

  fwrite("\033(G\001\000\001", 6, 1, prn);	/* Enter graphics mode */

  /* Set up print resolution */
  if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4) &&
      use_softweave)
    fprintf(prn, "\033(U\005%c%c%c%c\240\005", 0, 1440 / ydpi, 1440 / ydpi,
	    1440 / (ydpi * (horizontal_passes > 2 ? 2 : 1)));
  else
    fprintf(prn, "\033(U\001%c%c", 0, 3600 / ydpi);	    

  /* Printer capabilities */
  if (escp2_has_cap(model, MODEL_GRAYMODE_MASK, MODEL_GRAYMODE_YES))
    fprintf(prn, "\033(K\002%c%c%c", 0, 0,
	    (output_type == OUTPUT_GRAY ? 1 : 2));

  fprintf(prn, "\033(i\001%c%c", 0, use_softweave ? 0 : 1);

  fprintf(prn, "\033U%c", (horizontal_passes * vertical_passes > 2) ? 1 : 0);

  if (!use_softweave)
    {
      if (escp2_micro_ink(model) > 0)
	fprintf(prn, "\033(e\002%c%c%c", 0, 0, escp2_micro_ink(model));
    }
  else if (bits > 1)
    fwrite("\033(e\002\000\000\020", 7, 1, prn); /* Variable dots */
  else if (escp2_soft_ink(model) > 0)
    fprintf(prn, "\033(e\002%c%c%c", 0, 0, escp2_soft_ink(model));

  /* Set up page */
  if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4) &&
      use_softweave)
    {
      fwrite("\033(C\004\000", 5, 1, prn);	/* Page length */
      n = ydpi * page_length / 72;
      putc(n & 255, prn);
      putc(n >> 8, prn);
      putc(0, prn);
      putc(0, prn);

      if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
	{
	  /* This seems to confuse some printers... */
	  fwrite("\033(c\010\000", 5, 1, prn);	/* Top/bottom margins */
	  n = ydpi * (page_top) / 72;
	  putc(n & 255, prn);
	  putc(n >> 8, prn);
	  putc(0, prn);
	  putc(0, prn);
	  n = ydpi * (page_length) / 72;
	  putc(n & 255, prn);
	  putc(n >> 8, prn);
	  putc(0, prn);
	  putc(0, prn);
	}
      else
	{
	  fwrite("\033(c\004\000", 5, 1, prn);	/* Top/bottom margins */
	  n = ydpi * (page_top) / 72;
	  putc(n & 255, prn);
	  putc(n >> 8, prn);
	  n = ydpi * (page_length) / 72;
	  putc(n & 255, prn);
	  putc(n >> 8, prn);
	}

      fwrite("\033(S\010\000", 5, 1, prn);
      fprintf(prn, "%c%c%c%c%c%c%c%c",
	      (((page_width * 720 / 72) >> 0) & 0xff),
	      (((page_width * 720 / 72) >> 8) & 0xff),
	      (((page_width * 720 / 72) >> 16) & 0xff),
	      (((page_width * 720 / 72) >> 24) & 0xff),
	      (((page_length * 720 / 72) >> 0) & 0xff),
	      (((page_length * 720 / 72) >> 8) & 0xff),
	      (((page_length * 720 / 72) >> 16) & 0xff),
	      (((page_length * 720 / 72) >> 24) & 0xff));

      fprintf(prn, "\033(D%c%c%c%c%c%c", 4, 0, 14400 % 256, 14400 / 256,
	      escp2_nozzle_separation(model) * 14400 / 720,
	      14400 / escp2_xres(model));

      fwrite("\033(v\004\000", 5, 1, prn);     /* Absolute vertical position */
      n = ydpi * top / 72;
      putc(n & 255, prn);
      putc(n >> 8, prn);
      putc(0, prn);
      putc(0, prn);
    }
  else
    {
      fwrite("\033(C\002\000", 5, 1, prn);	/* Page length */
      n = ydpi * page_length / 72;
      putc(n & 255, prn);
      putc(n >> 8, prn);

      fwrite("\033(c\004\000", 5, 1, prn);	/* Top/bottom margins */
      n = ydpi * (page_top) / 72;
      putc(n & 255, prn);
      putc(n >> 8, prn);
      n = ydpi * (page_length) / 72;
      putc(n & 255, prn);
      putc(n >> 8, prn);

      if (escp2_has_cap(model, MODEL_COMMAND_MASK, MODEL_COMMAND_1999))
	{
	  fwrite("\033(S\010\000", 5, 1, prn);
	  fprintf(prn, "%c%c%c%c%c%c%c%c",
		  (((page_width * 720 / 72) >> 0) & 0xff),
		  (((page_width * 720 / 72) >> 8) & 0xff),
		  (((page_width * 720 / 72) >> 16) & 0xff),
		  (((page_width * 720 / 72) >> 24) & 0xff),
		  (((page_length * 720 / 72) >> 0) & 0xff),
		  (((page_length * 720 / 72) >> 8) & 0xff),
		  (((page_length * 720 / 72) >> 16) & 0xff),
		  (((page_length * 720 / 72) >> 24) & 0xff));
	}

      fwrite("\033(V\002\000", 5, 1, prn);    /* Absolute vertical position */
      n = ydpi * top / 72;
      putc(n & 255, prn);
      putc(n >> 8, prn);
    }
}

/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
void
escp2_print(const printer_t *printer,		/* I - Model */
            int       copies,		/* I - Number of copies */
            FILE      *prn,		/* I - File to print to */
	    Image     image,		/* I - Image to print */
            unsigned char    *cmap,	/* I - Colormap (for indexed images) */
	    vars_t    *v)
{
  int		model = printer->model;
  char 		*ppd_file = v->ppd_file;
  char 		*resolution = v->resolution;
  char 		*media_size = v->media_size;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  char          *ink_type = v->ink_type;
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
  int		use_softweave = 0;
  int		nozzles = 1;
  int		nozzle_separation = 1;
  int		horizontal_passes = 1;
  int		real_horizontal_passes = 1;
  int		vertical_passes = 1;
  int		vertical_subsample = 1;
  const res_t 	*res;
  int		bits;
  void *	weave;
  void *	dither;
  colormode_t colormode = COLOR_CCMMYK;
  if (v->image_type == IMAGE_MONOCHROME)
    {
      colormode = COLOR_MONOCHROME;
      output_type = OUTPUT_GRAY;
      bits = 1;
    }
  else if (output_type == OUTPUT_GRAY)
    colormode = COLOR_MONOCHROME;
  else if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
    colormode = COLOR_CCMMYK;
  else
    colormode = COLOR_CMYK;


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

  if (image_bpp < 3 && cmap == NULL && output_type == OUTPUT_COLOR)
    output_type = OUTPUT_GRAY_COLOR;	/* Force grayscale output */

  if (output_type == OUTPUT_COLOR)
  {
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
  for (res = &escp2_reslist[0];;res++)
    {
      if (!strcmp(resolution, res->name))
	{
	  use_softweave = res->softweave;
	  horizontal_passes = res->horizontal_passes;
	  real_horizontal_passes = horizontal_passes;
	  vertical_passes = res->vertical_passes;
	  vertical_subsample = res->vertical_subsample;
	  if (use_softweave && escp2_xres(model) < 720)
	    horizontal_passes *= 720 / escp2_xres(model);
	  xdpi = res->hres;
	  ydpi = res->vres;
	  if (output_type == OUTPUT_GRAY)
	    {
	      nozzles = escp2_black_nozzles(model);
	      if (nozzles == 0)
		nozzles = escp2_nozzles(model);
	    }
	  else
	    nozzles = escp2_nozzles(model);
	  nozzle_separation = escp2_nozzle_separation(model);
	  break;
	}
      else if (!strcmp(resolution, ""))
	{
	  return;	  
	}
    }
  if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4) &&
      use_softweave && strcmp(ink_type, "Single Dot Size") != 0 &&
      v->image_type != IMAGE_MONOCHROME)
    bits = 2;
  else
    bits = 1;
  if (!use_softweave)
    {
      /*
       * In microweave mode, correct for the loss of page height that
       * would happen in softweave mode.  The divide by 10 is to convert
       * lines into points (Epson printers all have 720 ydpi);
       */
      int extra_points = (escp2_nozzles(model) *
			  escp2_nozzle_separation(model) + 5) / 10;
      top += extra_points;
    }

 /*
  * Compute the output size...
  */

  landscape   = 0;
  escp2_imageable_area(model, ppd_file, media_size, &page_left, &page_right,
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
    left = page_width - x - out_width;
  }

  if (left < 0)
    left = (page_width - out_width) / 2;

  if (top < 0)
    top  = (page_height - out_height) / 2;

 /*
  * Let the user know what we're doing...
  */

  Image_progress_init(image);

 /*
  * Send ESC/P2 initialization commands...
  */
  page_length += (39 + (escp2_nozzles(model) * 2) *
		  escp2_nozzle_separation(model)) / 10; /* Top and bottom */
  page_top = 0;

  escp2_init_printer(prn, model, output_type, ydpi, use_softweave,
		     page_length, page_width, page_top, page_bottom,
		     top, nozzles, nozzle_separation,
		     horizontal_passes, vertical_passes, vertical_subsample,
		     bits);

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  left = ydpi * left / 72;
  if (ydpi > 720)
    left = left * 720 / ydpi;

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  if (output_type == OUTPUT_GRAY)
  {
    black   = malloc(length * bits);
    cyan    = NULL;
    magenta = NULL;
    lcyan    = NULL;
    lmagenta = NULL;
    yellow  = NULL;
  }
  else
  {
    cyan    = malloc(length * bits);
    magenta = malloc(length * bits);
    yellow  = malloc(length * bits);
  
    if (escp2_has_cap(model, MODEL_HASBLACK_MASK, MODEL_HASBLACK_YES))
      black = malloc(length * bits);
    else
      black = NULL;
    if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES)) {
      lcyan = malloc(length * bits);
      lmagenta = malloc(length * bits);
    } else {
      lcyan = NULL;
      lmagenta = NULL;
    }
  }
  if (use_softweave)
    weave = initialize_weave(nozzles, nozzle_separation, horizontal_passes,
			     vertical_passes, vertical_subsample, colormode,
			     bits, out_width);
  else
    weave = NULL;

 /*
  * Output the page, rotating as necessary...
  */

  v->density = v->density * printer->printvars.density /
    (real_horizontal_passes * vertical_subsample);
  v->saturation *= printer->printvars.saturation;

  if (landscape)
    dither = init_dither(image_height, out_width, 1);
  else
    dither = init_dither(image_width, out_width, 1);
  if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
    {
      dither_set_black_levels(dither, 1.5, 1.7, 1.7);
      dither_set_black_lower(dither, .01);
      dither_set_black_upper(dither, .4);
      dither_set_light_inks(dither, .5, .5, .5);
    }
  switch (v->image_type)
    {
    case IMAGE_LINE_ART:
      dither_set_ink_spread(dither, 19);
      dither_set_black_lower(dither, .00001);
      dither_set_randomizers(dither, 10, 10, 10, 10);
      dither_set_black_upper(dither, .0005);
      break;
    case IMAGE_SOLID_TONE:
      dither_set_ink_spread(dither, 15);
      break;
    case IMAGE_CONTINUOUS:
      dither_set_ink_spread(dither, 13);
      break;
    }	    
  scale_dither(dither, real_horizontal_passes);

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

      (*colorfunc)(in, out, image_height, image_bpp, cmap, v);

      if (bits == 1)
	{
	  if (v->image_type == IMAGE_MONOCHROME)
	    dither_fastblack(out, x, dither, black);
	  else if (output_type == OUTPUT_GRAY)
	    dither_black(out, x, dither, black);
	  else
	    dither_cmyk(out, x, dither, cyan, lcyan, magenta, lmagenta,
			yellow, 0, black);
	}
      else
	{
	  if (output_type == OUTPUT_GRAY)
	    dither_black_n(out, x, dither, black, 1);
	  else
	    dither_cmyk_n(out, x, dither, cyan, lcyan, magenta, lmagenta,
			 yellow, 0, black, 1);
	}

      if (use_softweave)
	escp2_write_weave(weave, prn, length, ydpi, model, out_width, left,
			  xdpi, cyan, magenta, yellow, black, lcyan, lmagenta);
      else
	{
	  escp2_write_all(prn, black, cyan, magenta, yellow, lcyan, lmagenta,
			  length, ydpi, model, out_width, left, bits);
	  fwrite("\033(v\002\000\001\000", 7, 1, prn);	/* Feed one line */
	}

      errval += errmod;
      errline -= errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline --;
      }
    }
    if (use_softweave)
      escp2_flush(weave, model, out_width, left, ydpi, xdpi, prn);
  }
  else
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

      if (bits == 1)
	{
	  if (v->image_type == IMAGE_MONOCHROME)
	    dither_fastblack(out, y, dither, black);
	  else if (output_type == OUTPUT_GRAY)
	    dither_black(out, y, dither, black);
	  else
	    dither_cmyk(out, y, dither, cyan, lcyan, magenta, lmagenta,
			yellow, 0, black);
	}
      else
	{
	  if (output_type == OUTPUT_GRAY)
	    dither_black_n(out, y, dither, black, 1);
	  else
	    dither_cmyk_n(out, y, dither, cyan, lcyan, magenta, lmagenta,
			 yellow, 0, black, 1);
	}

      if (use_softweave)
	escp2_write_weave(weave, prn, length, ydpi, model, out_width, left,
			  xdpi, cyan, magenta, yellow, black, lcyan, lmagenta);
      else
	{
	  escp2_write_all(prn, black, cyan, magenta, yellow, lcyan, lmagenta,
			  length, ydpi, model, out_width, left, bits);
	  fwrite("\033(v\002\000\001\000", 7, 1, prn);	/* Feed one line */
	}
      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline ++;
      }
    }
    if (use_softweave)
      escp2_flush(weave, model, out_width, left, ydpi, xdpi, prn);
  }
  free_dither(dither);

 /*
  * Cleanup...
  */

  free(in);
  free(out);
  if (use_softweave)
    destroy_weave(weave);

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

  putc('\014', prn);			/* Eject page */
  fputs("\033@", prn);			/* ESC/P2 reset */
}

static void
escp2_fold(const unsigned char *line,
	   int single_length,
	   unsigned char *outbuf)
{
  int i;
  for (i = 0; i < single_length; i++)
    {
      outbuf[0] =
	((line[0] & (1 << 7)) >> 0) +
	((line[0] & (1 << 6)) >> 1) +
	((line[0] & (1 << 5)) >> 2) +
	((line[0] & (1 << 4)) >> 3) +
	((line[single_length] & (1 << 7)) >> 1) +
	((line[single_length] & (1 << 6)) >> 2) +
	((line[single_length] & (1 << 5)) >> 3) +
	((line[single_length] & (1 << 4)) >> 4);
      outbuf[1] =
	((line[0] & (1 << 3)) << 4) +
	((line[0] & (1 << 2)) << 3) +
	((line[0] & (1 << 1)) << 2) +
	((line[0] & (1 << 0)) << 1) +
	((line[single_length] & (1 << 3)) << 3) +
	((line[single_length] & (1 << 2)) << 2) +
	((line[single_length] & (1 << 1)) << 1) +
	((line[single_length] & (1 << 0)) << 0);
      line++;
      outbuf += 2;
    }
}

static void
escp2_split_2(int length,
	      const unsigned char *in,
	      unsigned char *outhi,
	      unsigned char *outlo)
{
  int i;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = in[i];
      outlo[i] = inbyte & 0x55;
      outhi[i] = inbyte & 0xaa;
    }
}

static void
escp2_split_2_2(int length,
		const unsigned char *in,
		unsigned char *outhi,
		unsigned char *outlo)
{
  int i;
  for (i = 0; i < length * 2; i++)
    {
      unsigned char inbyte = in[i];
      outlo[i] = inbyte & 0x33;
      outhi[i] = inbyte & 0xcc;
    }
}

static void
escp2_split_4(int length,
	      const unsigned char *in,
	      unsigned char *out0,
	      unsigned char *out1,
	      unsigned char *out2,
	      unsigned char *out3)
{
  int i;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = in[i];
      out0[i] = inbyte & 0x88;
      out1[i] = inbyte & 0x44;
      out2[i] = inbyte & 0x22;
      out3[i] = inbyte & 0x11;
    }
}

static void
escp2_split_4_2(int length,
		const unsigned char *in,
		unsigned char *out0,
		unsigned char *out1,
		unsigned char *out2,
		unsigned char *out3)
{
  int i;
  for (i = 0; i < length * 2; i++)
    {
      unsigned char inbyte = in[i];
      out0[i] = inbyte & 0xc0;
      out1[i] = inbyte & 0x30;
      out2[i] = inbyte & 0x0c;
      out3[i] = inbyte & 0x03;
    }
}


static void
escp2_unpack_2(int length,
	       const unsigned char *in,
	       unsigned char *outlo,
	       unsigned char *outhi)
{
  int i;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = *in;
      if (!(i & 1))
	{
	  *outlo =
	    ((inbyte & (1 << 7)) << 0) +
	    ((inbyte & (1 << 5)) << 1) +
	    ((inbyte & (1 << 3)) << 2) +
	    ((inbyte & (1 << 1)) << 3);
	  *outhi =
	    ((inbyte & (1 << 6)) << 1) +
	    ((inbyte & (1 << 4)) << 2) +
	    ((inbyte & (1 << 2)) << 3) +
	    ((inbyte & (1 << 0)) << 4);
	}
      else
	{
	  *outlo +=
	    ((inbyte & (1 << 1)) >> 1) +
	    ((inbyte & (1 << 3)) >> 2) +
	    ((inbyte & (1 << 5)) >> 3) +
	    ((inbyte & (1 << 7)) >> 4);
	  *outhi +=
	    ((inbyte & (1 << 0)) >> 0) +
	    ((inbyte & (1 << 2)) >> 1) +
	    ((inbyte & (1 << 4)) >> 2) +
	    ((inbyte & (1 << 6)) >> 3);
	  outlo++;
	  outhi++;
	}
      in++;
    }
}

static void
escp2_unpack_2_2(int length,
		 const unsigned char *in,
		 unsigned char *outlo,
		 unsigned char *outhi)
{
  int i;
  for (i = 0; i < length * 2; i++)
    {
      unsigned char inbyte = *in;
      if (!(i & 1))
	{
	  *outlo =
	    ((inbyte & (3 << 6)) << 0) +
	    ((inbyte & (3 << 2)) << 2);
	  *outhi =
	    ((inbyte & (3 << 4)) << 2) +
	    ((inbyte & (3 << 0)) << 4);
	}
      else
	{
	  *outlo +=
	    ((inbyte & (3 << 6)) >> 4) +
	    ((inbyte & (3 << 2)) >> 2);
	  *outhi +=
	    ((inbyte & (3 << 4)) >> 2) +
	    ((inbyte & (3 << 0)) >> 0);
	  outlo++;
	  outhi++;
	}
      in++;
    }
}

static void
escp2_unpack_4(int length,
	       const unsigned char *in,
	       unsigned char *out0,
	       unsigned char *out1,
	       unsigned char *out2,
	       unsigned char *out3)
{
  int i;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = *in;
      switch (i & 3)
	{
	case 0:
	  *out0 =
	    ((inbyte & (1 << 7)) << 0) +
	    ((inbyte & (1 << 3)) << 3);
	  *out1 =
	    ((inbyte & (1 << 6)) << 1) +
	    ((inbyte & (1 << 2)) << 4);
	  *out2 =
	    ((inbyte & (1 << 5)) << 2) +
	    ((inbyte & (1 << 1)) << 5);
	  *out3 =
	    ((inbyte & (1 << 4)) << 3) +
	    ((inbyte & (1 << 0)) << 6);
	  break;
	case 1:
	  *out0 +=
	    ((inbyte & (1 << 7)) >> 2) +
	    ((inbyte & (1 << 3)) << 1);
	  *out1 +=
	    ((inbyte & (1 << 6)) >> 1) +
	    ((inbyte & (1 << 2)) << 2);
	  *out2 +=
	    ((inbyte & (1 << 5)) >> 0) +
	    ((inbyte & (1 << 1)) << 3);
	  *out3 +=
	    ((inbyte & (1 << 4)) << 1) +
	    ((inbyte & (1 << 0)) << 4);
	  break;
	case 2:
	  *out0 +=
	    ((inbyte & (1 << 7)) >> 4) +
	    ((inbyte & (1 << 3)) >> 1);
	  *out1 +=
	    ((inbyte & (1 << 6)) >> 3) +
	    ((inbyte & (1 << 2)) << 0);
	  *out2 +=
	    ((inbyte & (1 << 5)) >> 2) +
	    ((inbyte & (1 << 1)) << 1);
	  *out3 +=
	    ((inbyte & (1 << 4)) >> 1) +
	    ((inbyte & (1 << 0)) << 2);
	  break;
	case 3:
	  *out0 +=
	    ((inbyte & (1 << 7)) >> 6) +
	    ((inbyte & (1 << 3)) >> 3);
	  *out1 +=
	    ((inbyte & (1 << 6)) >> 5) +
	    ((inbyte & (1 << 2)) >> 2);
	  *out2 +=
	    ((inbyte & (1 << 5)) >> 4) +
	    ((inbyte & (1 << 1)) >> 1);
	  *out3 +=
	    ((inbyte & (1 << 4)) >> 3) +
	    ((inbyte & (1 << 0)) >> 0);
	  out0++;
	  out1++;
	  out2++;
	  out3++;
	  break;
	}
      in++;
    }
}

static void
escp2_unpack_4_2(int length,
		 const unsigned char *in,
		 unsigned char *out0,
		 unsigned char *out1,
		 unsigned char *out2,
		 unsigned char *out3)
{
  int i;
  for (i = 0; i < length * 2; i++)
    {
      unsigned char inbyte = *in;
      switch (i & 3)
	{
	case 0:
	  *out0 = ((inbyte & (3 << 6)) << 0);
	  *out1 = ((inbyte & (3 << 4)) << 2);
	  *out2 = ((inbyte & (3 << 2)) << 4);
	  *out3 = ((inbyte & (3 << 0)) << 6);
	  break;
	case 1:
	  *out0 += ((inbyte & (3 << 6)) >> 2);
	  *out1 += ((inbyte & (3 << 4)) << 0);
	  *out2 += ((inbyte & (3 << 2)) << 2);
	  *out3 += ((inbyte & (3 << 0)) << 4);
	  break;
	case 2:
	  *out0 += ((inbyte & (3 << 6)) >> 4);
	  *out1 += ((inbyte & (3 << 4)) >> 2);
	  *out2 += ((inbyte & (3 << 2)) << 0);
	  *out3 += ((inbyte & (3 << 0)) << 2);
	  break;
	case 3:
	  *out0 += ((inbyte & (3 << 6)) >> 6);
	  *out1 += ((inbyte & (3 << 4)) >> 4);
	  *out2 += ((inbyte & (3 << 2)) >> 2);
	  *out3 += ((inbyte & (3 << 0)) >> 0);
	  out0++;
	  out1++;
	  out2++;
	  out3++;
	  break;
	}
      in++;
    }
}

static int
escp2_pack(const unsigned char *line,
	   int length,
	   unsigned char *comp_buf,
	   unsigned char **comp_ptr)
{
  const unsigned char *start;		/* Start of compressed data */
  unsigned char repeat;			/* Repeating char */
  int count;			/* Count of compressed bytes */
  int tcount;			/* Temporary count < 128 */
  int active = 0;		/* Have we found data? */

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
	  if (! active && (line[-2] || line[-1] || line[0]))
	    active = 1;
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
      if (repeat)
	active = 1;

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
  return active;
}


static void
escp2_write_all(FILE          *prn,	/* I - Print file or command */
		const unsigned char *k,	/* I - Output bitmap data */
		const unsigned char *c,	/* I - Output bitmap data */
		const unsigned char *m,	/* I - Output bitmap data */
		const unsigned char *y,	/* I - Output bitmap data */
		const unsigned char *lc,	/* I - Output bitmap data */
		const unsigned char *lm,	/* I - Output bitmap data */
		int           length,	/* I - Length of bitmap data */
		int           ydpi,	/* I - Vertical resolution */
		int           model,	/* I - Printer model */
		int           width,	/* I - Printed width */
		int           offset,	/* I - Offset from left side */
		int	      bits)
{
  if (k)
    escp2_write(prn, k, length, 0, 0, ydpi, model, width, offset, bits);
  if (c)
    escp2_write(prn, c, length, 0, 2, ydpi, model, width, offset, bits);
  if (m)
    escp2_write(prn, m, length, 0, 1, ydpi, model, width, offset, bits);
  if (y)
    escp2_write(prn, y, length, 0, 4, ydpi, model, width, offset, bits);
  if (lc)
    escp2_write(prn, lc, length, 1, 2, ydpi, model, width, offset, bits);
  if (lm)
    escp2_write(prn, lm, length, 1, 1, ydpi, model, width, offset, bits);
}
	   
/*
 * 'escp2_write()' - Send ESC/P2 graphics using TIFF packbits compression.
 */

static void
escp2_write(FILE          *prn,		/* I - Print file or command */
	    const unsigned char *line,	/* I - Output bitmap data */
	    int           length,	/* I - Length of bitmap data */
	    int	   	  density,      /* I - 0 for dark, 1 for light */
	    int           plane,	/* I - Which color */
	    int           ydpi,		/* I - Vertical resolution */
	    int           model,	/* I - Printer model */
	    int           width,	/* I - Printed width */
	    int           offset,	/* I - Offset from left side */
	    int	   	  bits)		/* I - bits/pixel */
{
  unsigned char pack_buf[COMPBUFWIDTH];
  unsigned char	comp_buf[COMPBUFWIDTH],		/* Compression buffer */
    *comp_ptr;
  int setactive = 1;

  /*
   * Don't send blank lines...
   */

  if (line[0] == 0 && memcmp(line, line + 1, (bits * length) - 1) == 0)
    return;

  if (bits == 1)
    setactive = escp2_pack(line, length, comp_buf, &comp_ptr);
  else
    {
      escp2_fold(line, length, pack_buf);
      setactive = escp2_pack(pack_buf, length * bits, comp_buf, &comp_ptr);
    }
  if (!setactive)		/* Short circuit */
    return;

  /*
   * Set the print head position.
   */

  /*
   * Set the color if necessary...
   */

  if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
    fprintf(prn, "\033(r\002%c%c%c", 0, density, plane);
  else
    fprintf(prn, "\033r%c", plane);

  if (escp2_has_cap(model, MODEL_1440DPI_MASK, MODEL_1440DPI_YES) &&
      ydpi > 720)
    {
      if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK,
			MODEL_VARIABLE_4))
	fprintf(prn, "\033($%c%c%c%c%c%c", 4, 0,
		(offset * 1440 / ydpi) & 255,
		((offset * 1440 / ydpi) >> 8) & 255,
		((offset * 1440 / ydpi) >> 16) & 255,
		((offset * 1440 / ydpi) >> 24) & 255);
      else
	fprintf(prn, "\033(\\%c%c%c%c%c%c", 4, 0, 160, 5,
		(offset * 1440 / ydpi) & 255, (offset * 1440 / ydpi) >> 8);
    }
  else
    fprintf(prn, "\033\\%c%c", offset & 255, offset >> 8);

  /*
   * Send a line of raster graphics...
   */

  switch (ydpi)				/* Raster graphics header */
    {
    case 180 :
      fwrite("\033.\001\024\024\001", 6, 1, prn);
      break;
    case 360 :
      fwrite("\033.\001\012\012\001", 6, 1, prn);
      break;
    case 720 :
      if (escp2_has_cap(model, MODEL_720DPI_MODE_MASK, MODEL_720DPI_600))
	fwrite("\033.\001\050\005\001", 6, 1, prn);
      else
	fwrite("\033.\001\005\005\001", 6, 1, prn);
      break;
    }
  putc(width & 255, prn);		/* Width of raster line in pixels */
  putc(width >> 8, prn);

  fwrite(comp_buf, comp_ptr - comp_buf, 1, prn);
  putc('\r', prn);
}


/*
 * "Soft" weave
 *
 * The Epson Stylus Color/Photo printers don't have memory to print
 * using all of the nozzles in the print head.  For example, the Stylus Photo
 * 700/EX has 32 nozzles.  At 720 dpi, with an 8" wide image, a single line
 * requires (8 * 720 * 6 / 8) bytes, or 4320 bytes (because the Stylus Photo
 * printers have 6 ink colors).  To use 32 nozzles would require 138240 bytes.
 * It's actually worse than that, though, because the nozzles are spaced 8
 * rows apart.  Therefore, in order to store enough data to permit sending the
 * page as a simple raster, the printer would require enough memory to store
 * 256 rows, or 1105920 bytes.  Considering that the Photo EX can print
 * 11" wide, we're looking at more like 1.5 MB.  In fact, these printers are
 * capable of 1440 dpi horizontal resolution.  This would require 3 MB.  The
 * printers actually have 64K.
 *
 * With the newer (750 and 1200) printers it's even worse, since these printers
 * support multiple dot sizes.  But that's neither here nor there.
 *
 * The printer is capable of printing an image fed to it as single raster
 * lines.  This is called MicroWeave (tm).  It actually produces extremely
 * high quality output, but it only uses one nozzle per color per pass.
 * This means that it has to make a lot of passes to print a page, so it's
 * extremely slow (a full 8.5x11" page takes over 30 minutes!).  It's also
 * not possible to print very close to the bottom of the page with MicroWeave
 * since only the first nozzle is used, and the head cannot get closer than
 * some distance from the edge of the page.
 *
 * The solution is to have the host rearrange the output so that a single
 * pass is fed to the print head.  This means that we have to feed the printer
 * every 8th line as a single pass, and we then have to interleave ("weave")
 * the other raster lines as separate passes.  This allows us to use all 32
 * nozzles, and achieve much higher printing speed.
 *
 * What makes this interesting is that there are many different ways of
 * of accomplishing this goal.  The naive way would be to divide the image
 * up into groups of 256 rows, and print all the mod8=0 rows in the first pass,
 * mod8=1 rows in the second, and so forth.  The problem with this approach
 * is that the individual ink jets are not perfectly uniform; some emit
 * slightly bigger or smaller drops than others.  Since each group of 8
 * adjacent rows is printed with the same nozzle, that means that there will
 * be distinct streaks of lighter and darker bands within the image (8 rows
 * is 1/90", which is visible; 1/720" is not).  Possibly worse is that these
 * patterns will repeat every 256 rows.  This creates banding patterns that
 * are about 1/3" wide.
 *
 * So we have to do something to break up this patterning.
 *
 * Epson does not publish the weaving algorithms that they use in their
 * bundled drivers.  Indeed, their developer web site
 * (http://www.ercipd.com/isv/edr_docs.htm) does not even describe how to
 * do this weaving at all; it says that the only way to achieve 720 dpi
 * is to use MicroWeave.  It does note (correctly) that 1440 dpi horizontal
 * can only be achieved by the driver (i. e. in software).  The manual
 * actually makes it fairly clear how to do this (it requires two passes
 * with horizontal head movement between passes), and it is presumably
 * possible to do this with MicroWeave.
 *
 * The information about how to do this is apparently available under NDA.
 * It's actually easy enough to reverse engineer what's inside a print file
 * with a simple Perl script.  There are presumably other printer commands
 * that are not documented and may not be as easy to reverse engineer.
 *
 * I considered a few algorithms to perform the weave.  The first one I
 * devised let me use only (jets - distance_between_jets + 1) nozzles, or
 * 25.  This is OK in principle, but it's slower than using all nozzles.
 * By playing around with it some more, I came up with an algorithm that
 * lets me use all of the nozzles, except near the top and bottom of the
 * page.
 *
 * This still produces some banding, though.  Even better quality can be
 * achieved by using multiple nozzles on the same line.  How do we do this?
 * In 1440x720 mode, we're printing two output lines at the same vertical
 * position.  However, if we want four passes, we have to effectively print
 * each line twice.  Actually doing this would increase the density, so
 * what we do is print half the dots on each pass.  This produces near-perfect
 * output, and it's far faster than using "MicroWeave".
 *
 * The current algorithm is not completely general.  The number of passes
 * is limited to (nozzles / gap).  On the Photo EX class printers, that limits
 * it to 4 -- 32 nozzles, an inter-nozzle gap of 8 lines.  Furthermore, there
 * are a number of routines that are only coded up to 4 passes.
 *
 * The routine initialize_weave calculates the basic parameters, given
 * the number of jets and separation between jets, in rows.
 *
 * -- Robert Krawitz <rlk@alum.mit.edu) November 3, 1999
 */

#endif /* !WEAVETEST */

typedef struct			/* Weave parameters for a specific row */
{
  int row;			/* Absolute row # */
  int pass;			/* Computed pass # */
  int jet;			/* Which physical nozzle we're using */
  int missingstartrows;		/* Phantom rows (nonexistent rows that */
				/* would be printed by nozzles lower than */
				/* the first nozzle we're using this pass; */
				/* with the current algorithm, always zero */
  int logicalpassstart;		/* Offset in rows (from start of image) */
				/* that the printer must be for this row */
				/* to print correctly with the specified jet */
  int physpassstart;		/* Offset in rows to the first row printed */
				/* in this pass.  Currently always equal to */
				/* logicalpassstart */
  int physpassend;		/* Offset in rows (from start of image) to */
				/* the last row that will be printed this */
				/* pass (assuming that we're printing a full */
				/* pass). */
} weave_t;

typedef struct			/* Weave parameters for a specific pass */
{
  int pass;			/* Absolute pass number */
  int missingstartrows;		/* All other values the same as weave_t */
  int logicalpassstart;
  int physpassstart;
  int physpassend;
  int subpass;
} pass_t;

typedef union {			/* Offsets from the start of each line */
  off_t v[6];			/* (really pass) */
  struct {
    off_t k;
    off_t m;
    off_t c;
    off_t y;
    off_t M;
    off_t C;
  } p;
} lineoff_t;

typedef union {			/* Is this line active? */
  char v[6];			/* (really pass) */
  struct {
    char k;
    char m;
    char c;
    char y;
    char M;
    char C;
  } p;
} lineactive_t;

typedef union {			/* Base pointers for each pass */
  unsigned char *v[6];
  struct {
    unsigned char *k;
    unsigned char *m;
    unsigned char *c;
    unsigned char *y;
    unsigned char *M;
    unsigned char *C;
  } p;
} linebufs_t;

typedef struct {
  linebufs_t *linebases;	/* Base address of each row buffer */
  lineoff_t *lineoffsets;	/* Offsets within each row buffer */
  lineactive_t *lineactive;	/* Does this line have anything printed? */
  int *linecounts;		/* How many rows we've printed this pass */
  pass_t *passes;		/* Circular list of pass numbers */
  int last_pass_offset;		/* Starting row (offset from the start of */
				/* the image) of the most recently printed */
				/* pass (so we can determine how far to */
				/* advance the paper) */
  int last_pass;		/* Number of the most recently printed pass */

  int njets;			/* Number of jets in use */
  int separation;		/* Separation between jets */

  int weavefactor;		/* Interleave factor (jets / separation) */
  int jetsused;			/* How many jets we can actually use */
  int initialoffset;		/* Distance between the first row we're */
				/* printing and the logical first row */
				/* (first nozzle of the first pass). */
				/* Currently this is zero. */
  int jetsleftover;		/* How many jets we're *not* using. */
				/* This can be used to rotate exactly */
				/* what jets we're using.  Currently this */
				/* is not used. */
  int weavespan;		/* How many rows total are bracketed by */
				/* one pass (separation * (jets - 1) */
  int horizontal_weave;		/* Number of horizontal passes required */
				/* This is > 1 for some of the ultra-high */
				/* resolution modes */
  int vertical_subpasses;	/* Number of passes per line (for better */
				/* quality) */
  int vmod;			/* Number of banks of passes */
  int oversample;		/* Excess precision per row */
  int realjets;			/* Actual number of jets */
  int pass_adjustment;		/* Magical */
  int ncolors;			/* How many colors (1, 4, or 6) */
  int horizontal_width;		/* Line width in output pixels */

  int bitwidth;		/* Bits per pixel */
  int lineno;
  int vertical_oversample;	/* Vertical oversampling */
  int current_vertical_subpass;
} escp2_softweave_t;

/*
 * Mapping between color and linear index.  The colors are
 * black, magenta, cyan, yellow, light magenta, light cyan
 */

static const int color_indices[16] = { 0, 1, 2, -1,
				       3, -1, -1, -1,
				       -1, 4, 5, -1,
				       -1, -1, -1, -1 };
static const int colors[6] = { 0, 1, 2, 4, 1, 2 };
static const int densities[6] = { 0, 0, 0, 0, 1, 1 };

#ifndef WEAVETEST
static int
get_color_by_params(int plane, int density)
{
  if (plane > 4 || plane < 0 || density > 1 || density < 0)
    return -1;
  return color_indices[density * 8 + plane];
}
#endif

/*
 * Initialize the weave parameters
 */
static void *
initialize_weave(int jets, int sep, int osample, int v_subpasses,
		 int v_subsample, colormode_t colormode, int width,
		 int linewidth)
{
  int i;
  int k;
  escp2_softweave_t *sw = malloc(sizeof (escp2_softweave_t));
  if (jets <= 1)
    sw->separation = 1;
  if (sep <= 0)
    sw->separation = 1;
  else
    sw->separation = sep;
  sw->njets = jets;
  sw->realjets = jets;
  if (v_subpasses <= 0)
    v_subpasses = 1;
  sw->oversample = osample * v_subpasses * v_subsample;
  sw->vertical_subpasses = v_subpasses;
  sw->njets /= sw->oversample;
  sw->vmod = sw->separation * sw->oversample;
  sw->horizontal_weave = osample;
  sw->pass_adjustment = (osample * sep + jets - 1) / jets;

  sw->weavefactor = (sw->njets + sw->separation - 1) / sw->separation;
  sw->jetsused = MIN(((sw->weavefactor) * sw->separation), sw->njets);
  sw->initialoffset = (sw->jetsused - sw->weavefactor - 1) * sw->separation;
  if (sw->initialoffset < 0)
    sw->initialoffset = 0;
  sw->jetsleftover = sw->njets - sw->jetsused;
  sw->weavespan = (sw->jetsused - 1) * sw->separation;
  sw->vertical_oversample = v_subsample;

  sw->bitwidth = width;

  sw->last_pass_offset = 0;
  sw->last_pass = -1;
  sw->current_vertical_subpass = 0;

  switch (colormode)
    {
    case COLOR_MONOCHROME:
      sw->ncolors = 1;
      break;
    case COLOR_CMYK:
      sw->ncolors = 4;
      break;
    case COLOR_CCMMYK:
    default:
      sw->ncolors = 6;
      break;
    }
  
  /*
   * It's possible for the "compression" to actually expand the line by
   * one part in 128.
   */

  sw->horizontal_width = (linewidth + 128 + 7) * 129 / 128;
  sw->lineoffsets = malloc(sw->vmod * sizeof(lineoff_t) * sw->oversample);
  sw->lineactive = malloc(sw->vmod * sizeof(lineactive_t) * sw->oversample);
  sw->linebases = malloc(sw->vmod * sizeof(linebufs_t) * sw->oversample);
  sw->passes = malloc(sw->vmod * sizeof(pass_t));
  sw->linecounts = malloc(sw->vmod * sizeof(int));
  sw->lineno = 0;

  for (i = 0; i < sw->vmod; i++)
    {
      int j;
      sw->passes[i].pass = -1;
      for (k = 0; k < sw->oversample; k++)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      sw->linebases[k * sw->vmod + i].v[j] =
		malloc(jets * sw->bitwidth * sw->horizontal_width / 8);
	    }
	}
    }
  return (void *) sw;
}

static void
destroy_weave(void *vsw)
{
  int i, j, k;
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  free(sw->linecounts);
  free(sw->passes);
  free(sw->lineactive);
  free(sw->lineoffsets);
  for (i = 0; i < sw->vmod; i++)
    {
      for (k = 0; k < sw->oversample; k++)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      free(sw->linebases[k * sw->vmod + i].v[j]);
	    }
	}
    }
  free(sw->linebases);
  free(vsw);
}

/*
 * Compute the weave parameters for the given row.  This computation is
 * rather complex, and I need to go back and write down very carefully
 * what's going on here.
 */

#ifdef DEBUG_SIGNAL
static int 
divv(int x, int y)
{
  if (x < 0 || y < 0)
    {
      kill(getpid(), SIGFPE);
      return 0;
    }
  else
    return x / y;
}

static int 
modd(int x, int y)
{
  if (x < 0 || y < 0)
    {
      kill(getpid(), SIGFPE);
      return 0;
    }
  else
    return x % y;
}
#else

#define divv(x, y) ((x) / (y))
#define modd(x, y) ((x) % (y))

#endif

/*
 * Compute the weave parameters for the given row.  This computation is
 * rather complex, and I need to go back and write down very carefully
 * what's going on here.  That is, if I can figure it out myself :-)
 */

static void
weave_parameters_by_row(const escp2_softweave_t *sw, int row,
			int vertical_subpass, weave_t *w)
{
  int passblockstart = divv((row + sw->initialoffset), sw->jetsused);
  int internaljetsused = sw->jetsused * sw->oversample;
  int subpass_adjustment;

  w->row = row;
  w->pass = sw->pass_adjustment + (passblockstart - (sw->separation - 1)) +
    modd((sw->separation + row - passblockstart), sw->separation);
  subpass_adjustment = modd(divv((sw->separation + w->pass + 1),
				 sw->separation), sw->oversample);
  subpass_adjustment = sw->oversample - subpass_adjustment - 1;
  vertical_subpass = modd((sw->oversample + vertical_subpass +
			   subpass_adjustment), sw->oversample);
  w->pass += sw->separation * vertical_subpass;
  w->logicalpassstart = (w->pass * sw->jetsused) - sw->initialoffset -
    (sw->weavefactor * sw->separation) +
    modd((w->pass + sw->separation - 2), sw->separation);
  w->jet = divv((row + (sw->realjets * sw->separation) - w->logicalpassstart),
		sw->separation) - sw->realjets;
  w->jet += sw->jetsused * (sw->oversample - 1);
  if (w->jet >= sw->realjets)
    {
      w->jet -= sw->realjets;
      w->pass += sw->vmod;
    }
  w->logicalpassstart = w->row - (w->jet * sw->separation);
  if (w->logicalpassstart >= 0)
    w->physpassstart = w->logicalpassstart;
  else
    w->physpassstart = w->logicalpassstart +
      (sw->separation * divv((sw->separation - 1 - w->logicalpassstart),
			     sw->separation));
  w->physpassend = (internaljetsused - 1) * sw->separation +
    w->logicalpassstart;
  w->missingstartrows = divv((w->physpassstart - w->logicalpassstart),
			    sw->separation);
}

#ifndef WEAVETEST

static lineoff_t *
get_lineoffsets(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->lineoffsets[w.pass % sw->vmod]);
}

static lineactive_t *
get_lineactive(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->lineactive[w.pass % sw->vmod]);
}

static int *
get_linecount(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->linecounts[w.pass % sw->vmod]);
}

static const linebufs_t *
get_linebases(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->linebases[w.pass % sw->vmod]);
}

static pass_t *
get_pass_by_row(const escp2_softweave_t *sw, int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(sw, row, subpass, &w);
  return &(sw->passes[w.pass % sw->vmod]);
}

static lineoff_t *
get_lineoffsets_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->lineoffsets[pass % sw->vmod]);
}

static lineactive_t *
get_lineactive_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->lineactive[pass % sw->vmod]);
}

static int *
get_linecount_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->linecounts[pass % sw->vmod]);
}

static const linebufs_t *
get_linebases_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->linebases[pass % sw->vmod]);
}

static pass_t *
get_pass_by_pass(const escp2_softweave_t *sw, int pass)
{
  return &(sw->passes[pass % sw->vmod]);
}

/*
 * If there are phantom rows at the beginning of a pass, fill them in so
 * that the printer knows exactly what it doesn't have to print.  We're
 * using RLE compression here.  Each line must be specified independently,
 * so we have to compute how many full blocks (groups of 128 bytes, or 1024
 * "off" pixels) and how much leftover is needed.  Note that we can only
 * RLE-encode groups of 2 or more bytes; single bytes must be specified
 * with a count of 1.
 */

static void
fillin_start_rows(const escp2_softweave_t *sw, int row, int subpass,
		  int width, int missingstartrows)
{
  lineoff_t *offsets = get_lineoffsets(sw, row, subpass);
  const linebufs_t *bufs = get_linebases(sw, row, subpass);
  int i = 0;
  int k = 0;
  int j;
  width = sw->bitwidth * width * 8;
  for (k = 0; k < missingstartrows; k++)
    {
      int bytes_to_fill = width;
      int full_blocks = bytes_to_fill / (128 * 8);
      int leftover = (7 + (bytes_to_fill % (128 * 8))) / 8;
      int l = 0;
  
      while (l < full_blocks)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      (bufs[0].v[j][2 * i]) = 129;
	      (bufs[0].v[j][2 * i + 1]) = 0;
	    }
	  i++;
	  l++;
	}
      if (leftover == 1)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      (bufs[0].v[j][2 * i]) = 1;
	      (bufs[0].v[j][2 * i + 1]) = 0;
	    }
	  i++;
	}
      else if (leftover > 0)
	{
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      (bufs[0].v[j][2 * i]) = 257 - leftover;
	      (bufs[0].v[j][2 * i + 1]) = 0;
	    }
	  i++;
	}
    }
  for (j = 0; j < sw->ncolors; j++)
    offsets[0].v[j] = 2 * i;
}

static void
initialize_row(const escp2_softweave_t *sw, int row, int width)
{
  weave_t w;
  int i;
  for (i = 0; i < sw->oversample; i++)
    {
      weave_parameters_by_row(sw, row, i, &w);
      if (w.physpassstart == row)
	{
	  lineoff_t *lineoffs = get_lineoffsets(sw, row, i);
	  lineactive_t *lineactive = get_lineactive(sw, row, i);
	  int *linecount = get_linecount(sw, row, i);
	  int j;
	  pass_t *pass = get_pass_by_row(sw, row, i);
	  pass->pass = w.pass;
	  pass->missingstartrows = w.missingstartrows;
	  pass->logicalpassstart = w.logicalpassstart;
	  pass->physpassstart = w.physpassstart;
	  pass->physpassend = w.physpassend;
	  pass->subpass = i;
	  for (j = 0; j < sw->ncolors; j++)
	    {
	      lineoffs[0].v[j] = 0;
	      lineactive[0].v[j] = 0;
	    }
	  *linecount = 0;
	  if (w.missingstartrows > 0)
	    fillin_start_rows(sw, row, i, width, w.missingstartrows);
	}
    }
}

/*
 * A fair bit of this code is duplicated from escp2_write.  That's rather
 * a pity.  It's also not correct for any but the 6-color printers.  One of
 * these days I'll unify it.
 */
static void
flush_pass(escp2_softweave_t *sw, int passno, int model, int width,
	   int hoffset, int ydpi, int xdpi, FILE *prn, int vertical_subpass)
{
  int j;
  lineoff_t *lineoffs = get_lineoffsets_by_pass(sw, passno);
  lineactive_t *lineactive = get_lineactive_by_pass(sw, passno);
  const linebufs_t *bufs = get_linebases_by_pass(sw, passno);
  pass_t *pass = get_pass_by_pass(sw, passno);
  int *linecount = get_linecount_by_pass(sw, passno);
  int lwidth = (width + (sw->horizontal_weave - 1)) / sw->horizontal_weave;
  int microoffset = vertical_subpass & (sw->horizontal_weave - 1);
  if (ydpi > 720)
    ydpi = 720;
  if (passno == 0)
    sw->last_pass_offset = pass->logicalpassstart;
  else if (pass->logicalpassstart > sw->last_pass_offset)
    {
      int advance = pass->logicalpassstart - sw->last_pass_offset;
      int alo = advance % 256;
      int ahi = advance / 256;
      if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4))
	fprintf(prn, "\033(v\004%c%c%c%c%c", 0, alo, ahi, 0, 0);
      else
	fprintf(prn, "\033(v\002%c%c%c", 0, alo, ahi);
      sw->last_pass_offset = pass->logicalpassstart;
    }
  for (j = 0; j < sw->ncolors; j++)
    {
      if (lineactive[0].v[j] == 0)
	continue;
      if (ydpi >= 720 &&
	  escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4))
	;
      else if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
	fprintf(prn, "\033(r\002%c%c%c", 0, densities[j], colors[j]);
      else
	fprintf(prn, "\033r%c", colors[j]);
      if (escp2_has_cap(model, MODEL_1440DPI_MASK, MODEL_1440DPI_YES))
	{
	  /* FIXME need a more general way of specifying column */
	  /* separation */
	  if (escp2_has_cap(model, MODEL_COMMAND_MASK, MODEL_COMMAND_1999))
	    fprintf(prn, "\033($%c%c%c%c%c%c", 4, 0,
		    ((hoffset * xdpi / 720) + microoffset) & 255,
		    (((hoffset * xdpi / 720) + microoffset) >> 8) & 255,
		    (((hoffset * xdpi / 720) + microoffset) >> 16) & 255,
		    (((hoffset * xdpi / 720) + microoffset) >> 24) & 255);
	  else
	    fprintf(prn, "\033(\\%c%c%c%c%c%c", 4, 0, 160, 5,
		    ((hoffset * 1440 / ydpi) + microoffset) & 255,
		    ((hoffset * 1440 / ydpi) + microoffset) >> 8);
	}
      else
	{
	  fprintf(prn, "\033\\%c%c",
		  (hoffset + microoffset) & 255,
		  (hoffset + microoffset) >> 8);
	}
      if (escp2_has_cap(model, MODEL_VARIABLE_DOT_MASK, MODEL_VARIABLE_4))
	{
	  int ncolor = (densities[j] << 4) | colors[j];
	  int nlines = *linecount + pass->missingstartrows;
	  int nwidth = sw->bitwidth * ((lwidth + 7) / 8);
	  fprintf(prn, "\033i%c%c%c%c%c%c%c", ncolor, 1, sw->bitwidth,
		  nwidth & 255, nwidth >> 8, nlines & 255,
		  nlines >> 8);
	}
      else
	{
	  switch (ydpi)			/* Raster graphics header */
	    {
	    case 180 :
	      fwrite("\033.\001\024\024\001", 6, 1, prn);
	      break;
	    case 360 :
	      fwrite("\033.\001\012\012\001", 6, 1, prn);
	      break;
	    case 720 :
	      if (escp2_has_cap(model, MODEL_720DPI_MODE_MASK,
				MODEL_720DPI_600))
		fprintf(prn, "\033.%c%c%c%c", 1, 8 * 5, 5,
			*linecount + pass->missingstartrows);
	      else
		fprintf(prn, "\033.%c%c%c%c", 1, 5, 5,
			*linecount + pass->missingstartrows);
	      break;
	    }
	  putc(lwidth & 255, prn);	/* Width of raster line in pixels */
	  putc(lwidth >> 8, prn);
	}
	  
      fwrite(bufs[0].v[j], lineoffs[0].v[j], 1, prn);
      putc('\r', prn);
    }
  sw->last_pass = pass->pass;
  pass->pass = -1;
}

static void
add_to_row(escp2_softweave_t *sw, int row, unsigned char *buf, size_t nbytes,
	   int plane, int density, int subpass, int setactive)
{
  weave_t w;
  int color = get_color_by_params(plane, density);
  lineoff_t *lineoffs = get_lineoffsets(sw, row, subpass);
  lineactive_t *lineactive = get_lineactive(sw, row, subpass);
  const linebufs_t *bufs = get_linebases(sw, row, subpass);
  weave_parameters_by_row(sw, row, subpass, &w);
  memcpy(bufs[0].v[color] + lineoffs[0].v[color], buf, nbytes);
  lineoffs[0].v[color] += nbytes;
  if (setactive)
    lineactive[0].v[color] = 1;
}

static void
finalize_row(escp2_softweave_t *sw, int row, int model, int width,
	     int hoffset, int ydpi, int xdpi, FILE *prn)
{
  int i;
  for (i = 0; i < sw->oversample; i++)
    {
      weave_t w;
      int *lines = get_linecount(sw, row, i);
      weave_parameters_by_row(sw, row, i, &w);
      (*lines)++;
      if (w.physpassend == row)
	{
	  pass_t *pass = get_pass_by_row(sw, row, i);
	  flush_pass(sw, pass->pass, model, width, hoffset, ydpi, xdpi, prn,
		     i);
	}
    }
}

static void
escp2_flush(void *vsw, int model, int width, int hoffset,
	    int ydpi, int xdpi, FILE *prn)
{
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  while (1)
    {
      pass_t *pass = get_pass_by_pass(sw, sw->last_pass + 1);
      if (pass->pass < 0)
	return;
      flush_pass(sw, pass->pass, model, width, hoffset, ydpi, xdpi, prn,
		 pass->subpass);
    }
}

static void
escp2_write_weave(void *        vsw,
		  FILE          *prn,	/* I - Print file or command */
		  int           length,	/* I - Length of bitmap data */
		  int           ydpi,	/* I - Vertical resolution */
		  int           model,	/* I - Printer model */
		  int           width,	/* I - Printed width */
		  int           offset,
		  int		xdpi,
		  const unsigned char *c,
		  const unsigned char *m,
		  const unsigned char *y,
		  const unsigned char *k,
		  const unsigned char *C,
		  const unsigned char *M)
{
  escp2_softweave_t *sw = (escp2_softweave_t *) vsw;
  static unsigned char s[4][COMPBUFWIDTH];
  static unsigned char fold_buf[COMPBUFWIDTH];
  static unsigned char comp_buf[COMPBUFWIDTH];
  int xlength = (length + sw->horizontal_weave - 1) / sw->horizontal_weave;
  unsigned char *comp_ptr;
  int i, j;
  int setactive;
  int h_passes = sw->horizontal_weave * sw->vertical_subpasses;
  const unsigned char *cols[6];
  cols[0] = k;
  cols[1] = m;
  cols[2] = c;
  cols[3] = y;
  cols[4] = M;
  cols[5] = C;

  if (sw->current_vertical_subpass == 0)
    initialize_row(sw, sw->lineno, xlength);
  
  for (j = 0; j < sw->ncolors; j++)
    {
      if (cols[j])
	{
	  const unsigned char *in;
	  if (sw->bitwidth == 2)
	    {
	      escp2_fold(cols[j], length, fold_buf);
	      in = fold_buf;
	    }
	  else
	    in = cols[j];
	  if (h_passes > 1)
	    {
	      switch (sw->horizontal_weave)
		{
		case 2:
		  if (sw->bitwidth == 1)
		    escp2_unpack_2(length, in, s[0], s[1]);
		  else
		    escp2_unpack_2_2(length, in, s[0], s[1]);
		  break;
		case 4:
		  if (sw->bitwidth == 1)
		    escp2_unpack_4(length, in, s[0], s[1], s[2], s[3]);
		  else
		    escp2_unpack_4_2(length, in, s[0], s[1], s[2], s[3]);
		  break;
		}
	      switch (sw->vertical_subpasses)
		{
		case 4:
		  if (sw->bitwidth == 1)
		    escp2_split_4(length, in, s[0], s[1], s[2], s[3]);
		  else
		    escp2_split_4_2(length, in, s[0], s[1], s[2], s[3]);
		  break;
		case 2:
		  if (sw->horizontal_weave == 1)
		    {
		      if (sw->bitwidth == 1)
			escp2_split_2(xlength, in, s[0], s[1]);
		      else
			escp2_split_2_2(xlength, in, s[0], s[1]);
		    }
		  else
		    {		    
		      if (sw->bitwidth == 1)
			{
			  escp2_split_2(xlength, s[1], s[1], s[3]);
			  escp2_split_2(xlength, s[0], s[0], s[2]);
			}
		      else
			{
			  escp2_split_2_2(xlength, s[1], s[1], s[3]);
			  escp2_split_2_2(xlength, s[0], s[0], s[2]);
			}
		    }
		  break;
		  /* case 1 is taken care of because the various unpack */
		  /* functions will do the trick themselves */
		}
	      for (i = 0; i < h_passes; i++)
		{
		  int k = sw->current_vertical_subpass * sw->horizontal_weave;
		  setactive = escp2_pack(s[i], sw->bitwidth * xlength,
					 comp_buf, &comp_ptr);
		  add_to_row(sw, sw->lineno, comp_buf, comp_ptr - comp_buf,
			     colors[j], densities[j], k + i, setactive);
		}
	    }
	  else
	    {
	      int k = sw->current_vertical_subpass * sw->horizontal_weave;
	      if (sw->bitwidth == 1)
		setactive = escp2_pack(cols[j], length, comp_buf, &comp_ptr);
	      else
		setactive = escp2_pack(fold_buf, length * 2, comp_buf,
				       &comp_ptr);
	      add_to_row(sw, sw->lineno, comp_buf, comp_ptr - comp_buf,
			 colors[j], densities[j], k, setactive);
	    }
	}
    }
  sw->current_vertical_subpass++;
  if (sw->current_vertical_subpass >= sw->vertical_oversample)
    {
      finalize_row(sw, sw->lineno, model, width, offset, ydpi, xdpi, prn);
      sw->lineno++;
      sw->current_vertical_subpass = 0;
    }
}

#endif

/*
 *   $Log$
 *   Revision 1.117  2000/03/20 12:20:12  rlk
 *   Try to maintain dependency order of lex on yacc stuff
 *
 *   Revision 1.116  2000/03/14 02:24:31  rlk
 *   Added 1440x1440 mode
 *
 *   Revision 1.115  2000/03/13 13:31:26  rlk
 *   Add monochrome mode
 *
 *   Revision 1.114  2000/03/11 17:30:15  rlk
 *   Significant dither changes; addition of line art/solid color/continuous tone modes
 *
 *   Revision 1.113  2000/03/07 02:54:05  rlk
 *   Move CVS history logs to the end of the file
 *
 *   Revision 1.112  2000/03/07 00:23:19  rlk
 *   A bit more stuff
 *
 *   Revision 1.111  2000/03/06 03:10:50  rlk
 *   More cleanup stuff
 *
 *   Revision 1.110  2000/03/06 01:32:05  rlk
 *   more rearrangement
 *
 *   Revision 1.109  2000/03/05 19:51:24  rlk
 *   Create list of printers externally
 *
 *   Revision 1.108  2000/03/03 01:04:06  rlk
 *   correct parameters for ESC(D
 *
 *   Revision 1.107  2000/03/03 00:11:20  rlk
 *   Silly bug in rgb_to_gray
 *
 *   Revision 1.106  2000/03/02 03:12:20  khk
 *   Cleaned up some compiler warnings in print-escp2.c
 *
 *   Revision 1.105  2000/03/02 03:07:27  rlk
 *   Clean up options a bit
 *
 *   Revision 1.104  2000/03/01 01:38:08  rlk
 *   Some ditsy little things
 *
 *   Revision 1.103  2000/03/01 01:12:46  rlk
 *   Handle variable dot size correctly
 *
 *   Revision 1.102  2000/02/29 02:59:14  rlk
 *   1) Should be able to speed up black printing quite a bit for some models.
 *
 *   2) Add support for 1160 (tentative!)
 *
 *   Revision 1.101  2000/02/29 02:33:45  rlk
 *   Rationalize the printer options a bit
 *
 *   Revision 1.100  2000/02/29 00:39:25  rlk
 *   Improve printing to the bottom, and do memory allocation the way intended
 *
 *   Revision 1.99  2000/02/28 01:26:11  rlk
 *   Try to improve high resolution quality
 *
 *   Revision 1.98  2000/02/26 16:09:35  rlk
 *   1) Clean up (and maybe fix???) positioning.
 *
 *   2) Fix packing in multi-level code.
 *
 *   Revision 1.97  2000/02/26 00:14:44  rlk
 *   Rename dither_{black,cmyk}4 to dither_{black,cmyk}_n, and add argument to specify how levels are to be encoded
 *
 *   Revision 1.96  2000/02/25 02:22:37  rlk
 *   1) Stylus Color 460 (really a variant 440, at least until I learn otherwise).
 *
 *   2) Major optimization for gs driver in particular: elide rows that are blank.
 *
 *   3) make variable dot size the default for those printers.
 *
 *   Revision 1.95  2000/02/23 13:19:52  rlk
 *   Some minor fixes
 *
 *   Revision 1.94  2000/02/21 23:37:29  rlk
 *   Try again
 *
 *   Revision 1.93  2000/02/21 23:33:52  rlk
 *   Try for the 640
 *
 *   Revision 1.92  2000/02/21 20:32:37  rlk
 *   Important dithering bug fixes:
 *
 *   1) Avoid runaway black buildup.
 *
 *   2) Some conversion functions weren't doing density
 *
 *   Revision 1.91  2000/02/21 15:12:57  rlk
 *   Minor release prep
 *
 *   Revision 1.90  2000/02/21 01:08:02  rlk
 *   Weird init sequence for newer printers
 *
 *   Revision 1.89  2000/02/21 00:40:25  rlk
 *   change softweave sequence for older printers
 *
 *   Revision 1.88  2000/02/19 20:58:19  rlk
 *   Try one more time to fix 740
 *
 *   Revision 1.87  2000/02/19 16:01:36  rlk
 *   A bit more cleanup to non-softweave
 *
 *   Revision 1.86  2000/02/18 01:58:42  rlk
 *   Try to fix microweave and 360 dpi
 *
 *   Revision 1.85  2000/02/18 00:31:28  rlk
 *   Remove debug printf
 *
 *   Revision 1.84  2000/02/18 00:17:44  rlk
 *   Update line spacing for ESC900
 *
 *   Revision 1.83  2000/02/17 03:17:31  rlk
 *   Yet another try
 *
 *   Revision 1.82  2000/02/17 01:41:26  rlk
 *   Another try at the variable dot printers
 *
 *   Revision 1.81  2000/02/16 12:11:18  rlk
 *   narrow-carriage printers can print 14" long, not just 11"
 *
 *   Revision 1.80  2000/02/16 00:59:19  rlk
 *   1) Use correct convert functions (canon, escp2, pcl, ps).
 *
 *   2) Fix gray_to_rgb increment (print-util)
 *
 *   3) Fix dither update (print-dither)
 *
 *   Revision 1.79  2000/02/16 00:00:43  rlk
 *   Put pclunprint in build, and use actual weave code in weavetest
 *
 *   Revision 1.78  2000/02/15 12:49:10  rlk
 *   Back out clearly nonfunctional change for 740 family
 *
 *   Revision 1.77  2000/02/15 03:51:40  rlk
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
 *   Revision 1.76  2000/02/15 00:45:15  rlk
 *   Use older-style print command for single bit depth printing
 *
 *   Revision 1.75  2000/02/13 17:24:24  rlk
 *   More modes in run-weavetest, and attempt to handle column spacing in escp2
 *
 *   Revision 1.74  2000/02/13 03:14:26  rlk
 *   Bit of an oops here about printer models; also start on print-gray-using-color mode for better quality
 *
 *   Revision 1.73  2000/02/13 02:01:38  rlk
 *   Build a Ghostscript driver!  No idea if it works yet...
 *
 *   Revision 1.72  2000/02/12 23:02:00  rlk
 *   Change spacing for newer printers
 *
 *   Revision 1.71  2000/02/12 15:21:55  rlk
 *   Use Epson sequences more exactly
 *
 *   Revision 1.70  2000/02/12 03:37:04  rlk
 *   One more try
 *
 *   Revision 1.69  2000/02/11 02:19:13  rlk
 *   Remove apparently spurious flush command
 *
 *   Revision 1.68  2000/02/11 01:03:36  rlk
 *   Fix Epson left margin
 *
 *   Revision 1.67  2000/02/10 00:28:32  rlk
 *   Fix landscape vs. portrait problem
 *
 *   Revision 1.66  2000/02/09 02:56:27  rlk
 *   Put lut inside vars
 *
 *   Revision 1.65  2000/02/08 13:00:07  rlk
 *   Correct dot size for variable bits
 *
 *   Revision 1.64  2000/02/08 11:54:39  rlk
 *   Remove spurious init string
 *
 *   Revision 1.63  2000/02/08 00:29:18  rlk
 *   Is the separation really 6 lines for the 740?  That's very unusual.
 *   Usually it's 8 lines.  But we shall see...
 *
 *   Revision 1.62  2000/02/08 00:26:18  rlk
 *   Some kind of silly magic init string that it appears these printers want.
 *
 *   Revision 1.61  2000/02/07 01:35:05  rlk
 *   Try to improve variable dot stuff
 *
 *   Revision 1.60  2000/02/06 22:31:04  rlk
 *   1) Use old methods only for microweave printing.
 *
 *   2) remove MAX_DPI from print.h since it's no longer necessary.
 *
 *   3) Remove spurious CVS logs that were just clutter.
 *
 *   Revision 1.59  2000/02/06 21:58:06  rlk
 *   Choice of variable vs. single drop size for new printers
 *
 *   Revision 1.58  2000/02/06 21:33:33  rlk
 *   Try to fix softweave mode on new printers
 *
 *   Revision 1.57  2000/02/06 21:18:12  rlk
 *   Try to fix microweave on newer printers...?
 *
 *   Revision 1.56  2000/02/06 11:44:12  sharkey
 *   Don't cut corners by padding the 32 bit horizontal shifts with 0's in the
 *   upper 16 bits.  Do the full shifting and masking.  This is important when
 *   the relative offset is negative.
 *
 *   Revision 1.55  2000/02/06 03:59:09  rlk
 *   More work on the generalized dithering parameters stuff.  At this point
 *   it really looks like a proper object.  Also dynamically allocate the error
 *   buffers.  This segv'd a lot, which forced me to efence it, which was just
 *   as well because I found a few problems as a result...
 *
 *   Revision 1.54  2000/02/05 23:54:58  rlk
 *   Do horizontal positioning correctly in microweave
 *
 *   Revision 1.53  2000/02/05 14:56:41  rlk
 *   1) print-util.c: decrement rather than increment counter!
 *
 *   2) print-escp2.c: don't advance the paper a negative (or, with some printers,
 *   a very large positive) amount.
 *
 *   Revision 1.52  2000/02/04 02:07:52  rlk
 *   1440 dpi stupidity
 *
 *   Revision 1.51  2000/02/04 01:02:15  rlk
 *   Prelim support for 850/860/870/1200; fix stupid bug in ESC(S
 *
 *   Revision 1.50  2000/02/03 00:16:47  rlk
 *   Don't get too fancy with the new, undocumented ESC(c command
 *
 *   Revision 1.49  2000/02/02 03:03:55  rlk
 *   Move all the constants into members of a struct.  This will eventually permit
 *   us to use different dithering constants for each printer, or even vary them
 *   on the fly.  Currently there's a static dither_t that contains constants,
 *   but that's the easy part to fix...
 *
 *   Revision 1.48  2000/01/29 02:34:30  rlk
 *   1) Remove globals from everything except print.c.
 *
 *   2) Remove broken 1440x720 and 2880x720 microweave modes.
 *
 *   Revision 1.47  2000/01/28 03:59:53  rlk
 *   Move printers to print-util; also add top/left/bottom/right boxes to the UI
 *
 *   Revision 1.46  2000/01/25 19:51:27  rlk
 *   1) Better attempt at supporting newer Epson printers.
 *
 *   2) Generalized paper size support.
 *
 *   Revision 1.45  2000/01/25 18:59:24  rlk
 *   Try to make 440/640/740/900/750/1200 work
 *
 *   Revision 1.44  2000/01/25 03:19:47  rlk
 *   1) Weaving code for Stylus Photo 1200 and friends (the multi-bit printers).
 *   I don't expect printing to actually work, although it's not impossible that
 *   it will.
 *
 *   2) Fixed up the save code to be a bit more predictable.
 *
 *   3) Bug fixes
 *
 *   Revision 1.43  2000/01/21 00:53:39  rlk
 *   1) Add a few more paper sizes.
 *
 *   2) Clean up Makefile.standalone.
 *
 *   3) Nominal support for Stylus Color 850.
 *
 *   Revision 1.42  2000/01/17 22:23:31  rlk
 *   Print 3.1.0
 *
 *   Revision 1.41  2000/01/17 02:05:47  rlk
 *   Much stuff:
 *
 *   1) Fixes from 3.0.5
 *
 *   2) First cut at enhancing monochrome and four-level printing with stuff from
 *   the color print function.
 *
 *   3) Preliminary support (pre-support) for 440/640/740/900/750/1200.
 *
 *   Revision 1.40  2000/01/15 00:57:53  rlk
 *   Intermediate version
 *
 *   Revision 1.39  2000/01/13 03:25:31  rlk
 *   bug fix from mainline
 *
 *   Revision 1.38  2000/01/08 23:27:54  rlk
 *   Rearrange setup code; more printers to support softweave
 *
 *   Revision 1.37  1999/12/19 14:36:18  rlk
 *   Make 'em big enough
 *
 *   Revision 1.36  1999/12/18 23:08:28  rlk
 *   comments, mostly
 *
 *   Revision 1.35  1999/12/11 15:26:27  rlk
 *   hopefully get borders right
 *
 *   Revision 1.34  1999/12/11 04:52:35  rlk
 *   bug fixes
 *
 *   Revision 1.33  1999/12/11 04:25:23  rlk
 *   various other print modes
 *
 *   Revision 1.32  1999/12/11 01:46:13  rlk
 *   Better weaving code -- not absolutely complete yet
 *
 *   Revision 1.31  1999/12/05 22:10:53  rlk
 *   minor, prep for release
 *
 *   Revision 1.30  1999/12/05 04:33:43  rlk
 *   fencepost
 *
 *   Revision 1.29  1999/11/23 02:11:37  rlk
 *   Rationalize variables, pass 3
 *
 *   Revision 1.28  1999/11/23 01:45:00  rlk
 *   Rationalize variables -- pass 2
 *
 *   Revision 1.27  1999/11/16 01:04:06  rlk
 *   Documentation
 *
 *   Revision 1.26  1999/11/14 18:59:22  rlk
 *   Final preparations for release to Olof
 *
 *   Revision 1.25  1999/11/14 03:13:36  rlk
 *   Pseudo-hi-res microweave options
 *
 *   Revision 1.24  1999/11/13 02:32:58  rlk
 *   Comments on some good settings!
 *
 *   Revision 1.23  1999/11/10 01:13:27  rlk
 *   1440x720 two-pass
 *
 *   Revision 1.22  1999/11/08 13:10:21  rlk
 *   Bug fix
 *
 *   Revision 1.21  1999/11/07 22:18:51  rlk
 *   Support Stylus Photo
 *
 *   Attempt at 1440 dpi
 *
 *   Revision 1.20  1999/11/04 03:08:52  rlk
 *   Comments!  Comments!  Comments!
 *
 *   Revision 1.19  1999/11/02 23:11:16  rlk
 *   Good weave code
 *
 *   Revision 1.18  1999/11/02 03:11:17  rlk
 *   Remove dead code
 *
 *   Revision 1.17  1999/11/02 03:01:29  rlk
 *   Support both softweave and microweave
 *
 *   Revision 1.16  1999/11/02 02:04:18  rlk
 *   Much better weaving code!
 *
 *   Revision 1.15  1999/11/01 03:38:53  rlk
 *   First cut at weaving
 *
 *   Revision 1.14  1999/10/26 23:58:31  rlk
 *   indentation
 *
 *   Revision 1.13  1999/10/26 23:36:51  rlk
 *   Comment out all remaining 16-bit code, and rename 16-bit functions to "standard" names
 *
 *   Revision 1.12  1999/10/26 02:10:30  rlk
 *   Mostly fix save/load
 *
 *   Move all gimp, glib, gtk stuff into print.c (take it out of everything else).
 *   This should help port it to more general purposes later.
 *
 *   Revision 1.11  1999/10/25 23:31:59  rlk
 *   16-bit clean
 *
 *   Revision 1.10  1999/10/25 00:16:12  rlk
 *   Comment
 *
 *   Revision 1.9  1999/10/21 01:27:37  rlk
 *   More progress toward full 16-bit rendering
 *
 *   Revision 1.8  1999/10/19 02:04:59  rlk
 *   Merge all of the single-level print_cmyk functions
 *
 *   Revision 1.7  1999/10/18 01:37:19  rlk
 *   Add Stylus Photo 700 and switch to printer capabilities
 *
 *   Revision 1.6  1999/10/17 23:44:07  rlk
 *   16-bit everything (untested)
 *
 *   Revision 1.5  1999/10/14 01:59:59  rlk
 *   Saturation
 *
 *   Revision 1.4  1999/10/03 23:57:20  rlk
 *   Various improvements
 *
 *   Revision 1.3  1999/09/15 02:53:58  rlk
 *   Remove some stuff that seems to have no effect
 *
 *   Revision 1.2  1999/09/12 00:12:24  rlk
 *   Current best stuff
 *
 *   Revision 1.11  1998/05/15  21:01:51  mike
 *   Updated image positioning code (invert top and center left/top independently)
 *
 *   Revision 1.10  1998/05/08  21:18:34  mike
 *   Now enable microweaving in 720 DPI mode.
 *
 *   Revision 1.9  1998/05/08  20:49:43  mike
 *   Updated to support media size, imageable area, and parameter functions.
 *   Added support for scaling modes - scale by percent or scale by PPI.
 *
 *   Revision 1.8  1998/01/21  21:33:47  mike
 *   Updated copyright.
 *
 *   Revision 1.7  1997/11/12  15:57:48  mike
 *   Minor changes for clean compiles under Digital UNIX.
 *
 *   Revision 1.7  1997/11/12  15:57:48  mike
 *   Minor changes for clean compiles under Digital UNIX.
 *
 *   Revision 1.6  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.6  1997/07/30  20:33:05  mike
 *   Final changes for 1.1 release.
 *
 *   Revision 1.5  1997/07/30  18:47:39  mike
 *   Added scaling, orientation, and offset options.
 *
 *   Revision 1.4  1997/07/15  20:57:11  mike
 *   Updated ESC 800/1520/3000 output code to use vertical spacing of 5 instead of 40.
 *
 *   Revision 1.3  1997/07/03  13:21:15  mike
 *   Updated documentation for 1.0 release.
 *
 *   Revision 1.2  1997/07/03  13:03:57  mike
 *   Added horizontal offset to try to center image.
 *   Got rid of initial vertical positioning since the top margin is
 *   now set properly.
 *
 *   Revision 1.2  1997/07/03  13:03:57  mike
 *   Added horizontal offset to try to center image.
 *   Got rid of initial vertical positioning since the top margin is
 *   now set properly.
 *
 *   Revision 1.1  1997/07/02  13:51:53  mike
 *   Initial revision
 *
 * End of "$Id$".
 */
