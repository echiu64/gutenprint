/*
 * "$Id$"
 *
 *   PPD file generation program for the CUPS drivers.
 *
 *   Copyright 1993-2000 by Easy Software Products.
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License,
 *   version 2, as published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, please contact Easy Software
 *   Products at:
 *
 *       Attn: CUPS Licensing Information
 *       Easy Software Products
 *       44141 Airport View Drive, Suite 204
 *       Hollywood, Maryland 20636-3111 USA
 *
 *       Voice: (301) 373-9603
 *       EMail: cups-info@cups.org
 *         WWW: http://www.cups.org
 *
 * Contents:
 *
 *   main()      - Process files on the command-line...
 *   usage()     - Show program usage...
 *   write_ppd() - Write a PPD file.
 */

/*
 * Include necessary headers...
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <zlib.h>

#include <cups/cups.h>
#include <cups/raster.h>

#include "cups-print.h"


/*
 * Printer definition data from cups-printers.c...
 */

extern const printer_t	printers[];
extern const int	printer_count;


/*
 * File handling stuff...
 */

#ifdef HAVE_LIBZ
#  define PPDEXT ".ppd.gz"
#else
#  define PPDEXT ".ppd"
#  define gzFile FILE *
#  define gzopen fopen
#  define gzclose fclose
#  define gzprintf fprintf
#  define gzputs(f,s) fputs((s),(f))
#  define gzputc(f,c) putc((c),(f))
#endif /* HAVE_LIBZ */


/*
 * Size data...
 */

#define DEFAULT_SIZE	"Letter"
/*#define DEFAULT_SIZE	"A4"*/

typedef struct
{
  char		name[32];		/* Name of size */
  int		width,			/* Width in points */
		height;			/* Height in points */
} msize_t;

msize_t	sizes[] =
	{
	  { "A0",		2384, 3370 },
	  { "A0.Transverse",	3370, 2384 },
	  { "A1",		1684, 2384 },
	  { "A1.Transverse",	2384, 1684 },
	  { "A2",		1191, 1684 },
	  { "A2.Transverse",	1684, 1191 },
	  { "A3",		842,  1191 },
	  { "A3.Transverse",	1191, 842 },
	  { "A4",		595,  842 },
	  { "A4.Transverse",	842,  595 },
	  { "A5",		420,  595 },
	  { "A5.Transverse",	595,  420 },
	  { "A6",		297,  420 },
	  { "AnsiC",		1224, 1584 },
	  { "AnsiD",		1584, 2448 },
	  { "AnsiE",		2448, 3168 },
	  { "ARCHA",		648,  864 },
	  { "ARCHA.Transverse",	864,  648 },
	  { "ARCHB",		864,  1296 },
	  { "ARCHB.Transverse",	1296, 864 },
	  { "ARCHC",		1296, 1728 },
	  { "ARCHC.Transverse",	1728, 1296 },
	  { "ARCHD",		1728, 2592 },
	  { "ARCHD.Transverse",	2592, 1728 },
	  { "ARCHE",		2592, 3456 },
	  { "ARCHE.Transverse",	3456, 2592 },
	  { "B0",		2918, 4128 },
	  { "B1",		2064, 2918 },
	  { "B2",		1458, 2064 },
	  { "B3",		1032, 1458 },
	  { "B4",		729,  1032 },
	  { "B5",		516,  729 },
	  { "Env10",		297,  684 },
	  { "EnvC5",		459,  649 },
	  { "EnvDL",		312,  624 },
	  { "EnvISOB5",		499,  709 },
	  { "EnvMonarch",	279,  540 },
	  { "Executive",	522,  756 },
	  { "FanFoldUS",	1071, 792 },
	  { "Legal",		612,  1008 },
	  { "Letter",		612,  792 },
	  { "Letter.Transverse",792,  612 },
	  { "Tabloid",		792,  1224 },
	  { "TabloidExtra",	864,  1296 }
	};


/*
 * Local functions...
 */

void	usage(void);
int	write_ppd(const printer_t *p, const char *prefix);


/*
 * 'main()' - Process files on the command-line...
 */

int				/* O - Exit status */
main(int  argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
  int		i;		/* Looping var */
  char		*prefix;	/* Directory prefix for output */
  const printer_t *p;		/* Current printer */


  prefix = "ppd";

  for (i = 1; i < argc; i ++)
    if (strcmp(argv[i], "--help") == 0)
    {
     /*
      * Show help...
      */

      usage();
    }
    else if (strcmp(argv[i], "--prefix") == 0)
    {
     /*
      * Set "installation prefix"...
      */

      i ++;
      if (i < argc)
        prefix = argv[i];
      else
        usage();
    }
    else
      usage();

  for (i = printer_count, p = printers; i > 0; i --, p ++)
    if (write_ppd(p, prefix))
      return (1);

  return (0);
}


/*
 * 'usage()' - Show program usage...
 */

void
usage(void)
{
  puts("Usage: genppd [--help] [--prefix dir]");
  exit(1);
}


/*
 * 'write_ppd()' - Write a PPD file.
 */

int					/* O - Exit status */
write_ppd(const printer_t *p,		/* I - Printer driver */
	  const char      *prefix)	/* I - Prefix (directory) for PPD files */
{
  int		i, j;			/* Looping vars */
  gzFile	fp;			/* File to write to */
  char		filename[1024];		/* Filename */
  char		manufacturer[64];	/* Manufacturer name */
  msize_t	*size;			/* Page size */
  int		num_opts;		/* Number of printer options */
  char		**opts;			/* Printer options */
  char		*opt;			/* Pointer into option string */
  int		xdpi, ydpi;		/* Resolution info */
  vars_t	v;			/* Variable info */
  int		width, height,		/* Page information */
		bottom, left,
		top, right;
  int		printed_default_resolution = 0;


 /*
  * Skip the PostScript drivers...
  */

  if (strcmp(p->driver, "ps") == 0 ||
      strcmp(p->driver, "ps2") == 0)
    return (0);

 /* 
  * Make sure the destination directory exists...
  */

  mkdir(prefix, 0777);
  sprintf(filename, "%s/%s" PPDEXT, prefix, p->driver);

 /*
  * Open the PPD file...
  */

  if ((fp = gzopen(filename, "wb")) == NULL)
  {
    fprintf(stderr, "genppd: Unable to create file \"%s\" - %s.\n",
            filename, strerror(errno));
    return (2);
  }

 /*
  * Write a standard header...
  */

  sscanf(p->long_name, "%63s", manufacturer);

  fprintf(stderr, "Writing %s...\n", filename);

  gzputs(fp, "*PPD-Adobe: \"4.3\"\n");
  gzputs(fp, "*%PPD file for CUPS/GIMP-print.\n");
  gzputs(fp, "*%Copyright 1993-2000 by Easy Software Products, All Rights Reserved.\n");
  gzputs(fp, "*%This PPD file may be freely used and distributed under the terms of\n");
  gzputs(fp, "*%the GNU GPL.\n");
  gzputs(fp, "*FormatVersion:	\"4.3\"\n");
  gzputs(fp, "*FileVersion:	\"4.0\"\n");
  gzputs(fp, "*LanguageVersion: English\n");
  gzputs(fp, "*LanguageEncoding: ISOLatin1\n");
  gzprintf(fp, "*PCFileName:	\"%s.ppd\"\n", p->driver);
  gzprintf(fp, "*Manufacturer:	\"%s\"\n", manufacturer);
  gzputs(fp, "*Product:	\"(GIMP-print v4.0)\"\n");
  gzprintf(fp, "*ModelName:     \"%s\"\n", p->driver);
  gzprintf(fp, "*ShortNickName: \"%s\"\n", p->long_name);
  gzprintf(fp, "*NickName:      \"%s, CUPS+GIMP-print v4.0\"\n", p->long_name);
  gzputs(fp, "*PSVersion:	\"(3010.000) 550\"\n");
  gzputs(fp, "*LanguageLevel:	\"3\"\n");
  gzprintf(fp, "*ColorDevice:	%s\n",
           p->printvars.output_type == OUTPUT_COLOR ? "True" : "False");
  gzprintf(fp, "*DefaultColorSpace: %s\n", 
           p->printvars.output_type == OUTPUT_COLOR ? "RGB" : "Gray");
  gzputs(fp, "*FileSystem:	False\n");
  gzputs(fp, "*LandscapeOrientation: Plus90\n");
  gzputs(fp, "*TTRasterizer:	Type42\n");

  gzputs(fp, "*cupsVersion:	1.1\n");
  gzprintf(fp, "*cupsModelNumber: \"%d\"\n", p->model);
  gzputs(fp, "*cupsManualCopies: True\n");
  gzputs(fp, "*cupsFilter:	\"application/vnd.cups-raster 100 rastertoprinter\"\n");
  if (strcasecmp(manufacturer, "EPSON") == 0)
    gzputs(fp, "*cupsFilter:	\"application/vnd.cups-command 100 commandtoepson\"\n");

 /*
  * Get the page sizes from the driver...
  */

  opts = (*(p->parameters))(p, NULL, "PageSize", &num_opts);

  gzputs(fp, "*OpenUI *PageSize: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *PageSize\n");
  gzputs(fp, "*DefaultPageSize: " DEFAULT_SIZE "\n");

  memcpy(&v, &(p->printvars), sizeof(v));

  for (i = 0; i < num_opts; i ++)
  {
   /*
    * Get the media size...
    */

    strcpy(v.media_size, opts[i]);

    (*(p->media_size))(p, &v, &width, &height);

    for (j = sizeof(sizes) / sizeof(sizes[0]), size = sizes; j > 0; j --, size ++)
      if (size->width == width && size->height == height)
        break;

    if (j)
      gzprintf(fp, "*PageSize %s", size->name);
    else
      gzprintf(fp, "*PageSize w%dh%d", width, height);

    gzprintf(fp, "/%s:\t\"<</PageSize[%d %d]/ImagingBBox null>>setpagedevice\"\n",
             opts[i], width, height);
  }
  gzputs(fp, "*CloseUI: *PageSize\n");

  gzputs(fp, "*OpenUI *PageRegion: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *PageRegion\n");
  gzputs(fp, "*DefaultPageRegion: " DEFAULT_SIZE "\n");

  for (i = 0; i < num_opts; i ++)
  {
   /*
    * Get the media size...
    */

    strcpy(v.media_size, opts[i]);

    (*(p->media_size))(p, &v, &width, &height);

    for (j = sizeof(sizes) / sizeof(sizes[0]), size = sizes; j > 0; j --, size ++)
      if (size->width == width && size->height == height)
        break;

    if (j)
      gzprintf(fp, "*PageRegion %s", size->name);
    else
      gzprintf(fp, "*PageRegion w%dh%d", width, height);

    gzprintf(fp, "/%s:\t\"<</PageRegion[%d %d]/ImagingBBox null>>setpagedevice\"\n",
             opts[i], width, height);
  }
  gzputs(fp, "*CloseUI: *PageRegion\n");

  gzputs(fp, "*DefaultImageableArea: " DEFAULT_SIZE "\n");
  for (i = 0; i < num_opts; i ++)
  {
   /*
    * Get the media size and margins...
    */

    strcpy(v.media_size, opts[i]);

    (*(p->media_size))(p, &v, &width, &height);
    (*(p->imageable_area))(p, &v, &left, &right, &bottom, &top);

    for (j = sizeof(sizes) / sizeof(sizes[0]), size = sizes; j > 0; j --, size ++)
      if (size->width == width && size->height == height)
        break;

    if (j)
      gzprintf(fp, "*ImageableArea %s", size->name);
    else
      gzprintf(fp, "*ImageableArea w%dh%d", width, height);

    gzprintf(fp, "/%s:\t\"%d %d %d %d\"\n", opts[i],
             left, bottom, right, top);
  }

  gzputs(fp, "*DefaultPaperDimension: " DEFAULT_SIZE "\n");

  for (i = 0; i < num_opts; i ++)
  {
   /*
    * Get the media size...
    */

    strcpy(v.media_size, opts[i]);

    (*(p->media_size))(p, &v, &width, &height);

    for (j = sizeof(sizes) / sizeof(sizes[0]), size = sizes; j > 0; j --, size ++)
      if (size->width == width && size->height == height)
        break;

    if (j)
      gzprintf(fp, "*PaperDimension %s", size->name);
    else
      gzprintf(fp, "*PaperDimension w%dh%d", width, height);

    gzprintf(fp, "/%s:\t\"%d %d\"\n", opts[i], width, height);
    free(opts[i]);
  }
  if (opts)
    free(opts);

 /*
  * Do we support color?
  */

  gzputs(fp, "*OpenUI *ColorModel: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *ColorModel\n");

  if (p->printvars.output_type == OUTPUT_COLOR)
    gzputs(fp, "*DefaultColorModel: RGB\n");
  else
    gzputs(fp, "*DefaultColorModel: Gray\n");

  gzprintf(fp, "*ColorModel Gray/Grayscale:\t\"<<"
               "/cupsColorSpace %d"
	       "/cupsColorOrder %d"
	       "/cupsBitsPerColor 8>>setpagedevice\"\n",
           CUPS_CSPACE_W, CUPS_ORDER_CHUNKED);

  if (p->printvars.output_type == OUTPUT_COLOR)
    gzprintf(fp, "*ColorModel RGB/Color:\t\"<<"
                 "/cupsColorSpace %d"
		 "/cupsColorOrder %d"
		 "/cupsBitsPerColor 8>>setpagedevice\"\n",
             CUPS_CSPACE_RGB, CUPS_ORDER_CHUNKED);

  gzputs(fp, "*CloseUI: *ColorModel\n");

 /*
  * Image types...
  */

  gzputs(fp, "*OpenUI *ImageType/Image Type: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *ImageType\n");
  gzputs(fp, "*DefaultImageType: Continuous\n");

  gzprintf(fp, "*ImageType LineArt/Line Art:\t\"<</cupsRowCount 0>>setpagedevice\"\n");
  gzprintf(fp, "*ImageType SolidTone/Solid Tone:\t\"<</cupsRowCount 1>>setpagedevice\"\n");
  gzprintf(fp, "*ImageType Continuous/Photograph:\t\"<</cupsRowCount 2>>setpagedevice\"\n");
  gzprintf(fp, "*ImageType Monochrome/Monochrome:\t\"<</cupsRowCount 3>>setpagedevice\"\n");

  gzputs(fp, "*CloseUI: *ImageType\n");

 /*
  * Media types...
  */

  opts = (*(p->parameters))(p, NULL, "MediaType", &num_opts);

  if (num_opts > 0)
  {
    gzputs(fp, "*OpenUI *MediaType: PickOne\n");
    gzputs(fp, "*OrderDependency: 10 AnySetup *MediaType\n");
    gzputs(fp, "*DefaultMediaType: ");
    for (opt = opts[0]; *opt; opt ++)
      if (*opt != ' ' && *opt != '/')
	gzputc(fp, *opt);
    gzputc(fp, '\n');

    for (i = 0; i < num_opts; i ++)
    {
      gzputs(fp, "*MediaType ");

      for (opt = opts[i]; *opt; opt ++)
        if (*opt != ' ' && *opt != '/')
	  gzputc(fp, *opt);

      gzprintf(fp, "/%s:\t\"<</MediaType(%s)>>setpagedevice\"\n", opts[i], opts[i]);
      free(opts[i]);
    }
    if (opts)
      free(opts);

    gzputs(fp, "*CloseUI: *MediaType\n");
  }

 /*
  * Input slots...
  */

  opts = (*(p->parameters))(p, NULL, "InputSlot", &num_opts);

  if (num_opts > 0)
  {
    gzputs(fp, "*OpenUI *InputSlot: PickOne\n");
    gzputs(fp, "*OrderDependency: 10 AnySetup *InputSlot\n");
    gzputs(fp, "*DefaultInputSlot: ");
    for (opt = opts[0]; *opt; opt ++)
      if (*opt != ' ' && *opt != '/')
	gzputc(fp, *opt);
    gzputc(fp, '\n');

    for (i = 0; i < num_opts; i ++)
    {
      gzputs(fp, "*InputSlot ");

      for (opt = opts[i]; *opt; opt ++)
        if (*opt != ' ' && *opt != '/')
	  gzputc(fp, *opt);

      gzprintf(fp, "/%s:\t\"<</MediaClass(%s)>>setpagedevice\"\n", opts[i], opts[i]);
      free(opts[i]);
    }

    if (opts)
      free(opts);
    gzputs(fp, "*CloseUI: *InputSlot\n");
  }

 /*
  * Dithering algorithms...
  */

  gzputs(fp, "*OpenUI *Dither: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *Dither\n");

  for (i = 0; i < num_dither_algos; i ++)
  {
    char *s;
    char *copy = malloc(strlen(dither_algo_names[i]) + 1);
    char *d = copy;
    s = dither_algo_names[i];
    do
    {
      if (*s != ' ' && *s != '\t' && *s != '-')
	*d++ = *s;
    } while (*s++);

    if (i == 0)
      gzprintf(fp, "*DefaultDither: %s\n", copy);
    gzprintf(fp, "*Dither %s/%s: \"<</OutputType(%s)>>setpagedevice\"\n",
             copy, dither_algo_names[i], dither_algo_names[i]);
    free(copy);
  }

  gzputs(fp, "*CloseUI: *Dither\n");

 /*
  * Resolutions...
  */

  opts = (*(p->parameters))(p, NULL, "Resolution", &num_opts);

  gzputs(fp, "*OpenUI *Resolution: PickOne\n");
  gzputs(fp, "*OrderDependency: 20 AnySetup *Resolution\n");

  for (i = 0; i < num_opts; i ++)
  {
    char *s;
    char *copy = malloc(strlen(opts[i]) + 1);
    char *d = copy;
   /* 
    * Strip resolution name to its essentials...
    */
    (p->describe_resolution)(p, opts[i], &xdpi, &ydpi);

    /* This should not happen! */
    if (xdpi == -1 || ydpi == -1)
      continue;
    s = opts[i];
    do
    {
      if (*s != ' ' && *s != '\t' && *s != '-')
	*d++ = *s;
    } while (*s++);

   /*
    * Write the resolution option...
    */

    if (printed_default_resolution == 0)
    {
      gzprintf(fp, "*DefaultResolution: %s\n", copy);
      printed_default_resolution = 1;
    }

    gzprintf(fp, "*Resolution %s/%s:\t\"<</HWResolution[%d %d]/cupsCompression %d>>setpagedevice\"\n",
             copy, opts[i], xdpi, ydpi, i);
    free(copy);
    free(opts[i]);
  }
  if (opts)
    free(opts);
  gzputs(fp, "*CloseUI: *Resolution\n");

  gzputs(fp, "*DefaultFont: Courier\n");
  gzputs(fp, "*Font AvantGarde-Book: Standard \"(001.006S)\" Standard ROM\n");
  gzputs(fp, "*Font AvantGarde-BookOblique: Standard \"(001.006S)\" Standard ROM\n");
  gzputs(fp, "*Font AvantGarde-Demi: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font AvantGarde-DemiOblique: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font Bookman-Demi: Standard \"(001.004S)\" Standard ROM\n");
  gzputs(fp, "*Font Bookman-DemiItalic: Standard \"(001.004S)\" Standard ROM\n");
  gzputs(fp, "*Font Bookman-Light: Standard \"(001.004S)\" Standard ROM\n");
  gzputs(fp, "*Font Bookman-LightItalic: Standard \"(001.004S)\" Standard ROM\n");
  gzputs(fp, "*Font Courier: Standard \"(002.004S)\" Standard ROM\n");
  gzputs(fp, "*Font Courier-Bold: Standard \"(002.004S)\" Standard ROM\n");
  gzputs(fp, "*Font Courier-BoldOblique: Standard \"(002.004S)\" Standard ROM\n");
  gzputs(fp, "*Font Courier-Oblique: Standard \"(002.004S)\" Standard ROM\n");
  gzputs(fp, "*Font Helvetica: Standard \"(001.006S)\" Standard ROM\n");
  gzputs(fp, "*Font Helvetica-Bold: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font Helvetica-BoldOblique: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font Helvetica-Narrow: Standard \"(001.006S)\" Standard ROM\n");
  gzputs(fp, "*Font Helvetica-Narrow-Bold: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font Helvetica-Narrow-BoldOblique: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font Helvetica-Narrow-Oblique: Standard \"(001.006S)\" Standard ROM\n");
  gzputs(fp, "*Font Helvetica-Oblique: Standard \"(001.006S)\" Standard ROM\n");
  gzputs(fp, "*Font NewCenturySchlbk-Bold: Standard \"(001.009S)\" Standard ROM\n");
  gzputs(fp, "*Font NewCenturySchlbk-BoldItalic: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font NewCenturySchlbk-Italic: Standard \"(001.006S)\" Standard ROM\n");
  gzputs(fp, "*Font NewCenturySchlbk-Roman: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font Palatino-Bold: Standard \"(001.005S)\" Standard ROM\n");
  gzputs(fp, "*Font Palatino-BoldItalic: Standard \"(001.005S)\" Standard ROM\n");
  gzputs(fp, "*Font Palatino-Italic: Standard \"(001.005S)\" Standard ROM\n");
  gzputs(fp, "*Font Palatino-Roman: Standard \"(001.005S)\" Standard ROM\n");
  gzputs(fp, "*Font Symbol: Special \"(001.007S)\" Special ROM\n");
  gzputs(fp, "*Font Times-Bold: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font Times-BoldItalic: Standard \"(001.009S)\" Standard ROM\n");
  gzputs(fp, "*Font Times-Italic: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font Times-Roman: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font ZapfChancery-MediumItalic: Standard \"(001.007S)\" Standard ROM\n");
  gzputs(fp, "*Font ZapfDingbats: Special \"(001.004S)\" Standard ROM\n");

  gzprintf(fp, "*%%End of %s.ppd\n", p->driver);

  gzclose(fp);

  return (0);
}


void Image_init(Image image) {}
void Image_reset(Image image) {}
void Image_transpose(Image image) {}
void Image_hflip(Image image) {}
void Image_vflip(Image image) {}
void Image_crop(Image image, int left, int top, int right, int bottom) {}
void Image_rotate_ccw(Image image) {}
void Image_rotate_cw(Image image) {}
void Image_rotate_180(Image image) {}
int  Image_bpp(Image image) { return (0); }
int  Image_width(Image image) { return (0); }
int  Image_height(Image image) { return (0); }
void Image_get_row(Image image, unsigned char *data, int row) {}

const char *Image_get_appname(Image image) { return (NULL); }
void Image_progress_init(Image image) {}
void Image_note_progress(Image image, double current, double total) {}
void Image_progress_conclude(Image image) {}

/*
 * End of "$Id$".
 */
