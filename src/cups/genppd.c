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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef INCLUDE_GIMP_PRINT_H
#include INCLUDE_GIMP_PRINT_H
#else
#include <gimp-print.h>
#endif
#include <gimp-print-intl.h>
#include "../../lib/libprintut.h"

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
	  { N_ ("A0"),			2384, 3370 },
	  { N_ ("A0.Transverse"),	3370, 2384 },
	  { N_ ("A1"),			1684, 2384 },
	  { N_ ("A1.Transverse"),	2384, 1684 },
	  { N_ ("A2"),			1191, 1684 },
	  { N_ ("A2.Transverse"),	1684, 1191 },
	  { N_ ("A3"),			842,  1191 },
	  { N_ ("A3.Transverse"),	1191, 842 },
	  { N_ ("A4"),			595,  842 },
	  { N_ ("A4.Transverse"),	842,  595 },
	  { N_ ("A5"),			420,  595 },
	  { N_ ("A5.Transverse"),	595,  420 },
	  { N_ ("A6"),			297,  420 },
	  { N_ ("AnsiC"),		1224, 1584 },
	  { N_ ("AnsiD"),		1584, 2448 },
	  { N_ ("AnsiE"),		2448, 3168 },
	  { N_ ("ARCHA"),		648,  864 },
	  { N_ ("ARCHA.Transverse"),	864,  648 },
	  { N_ ("ARCHB"),		864,  1296 },
	  { N_ ("ARCHB.Transverse"),	1296, 864 },
	  { N_ ("ARCHC"),		1296, 1728 },
	  { N_ ("ARCHC.Transverse"),	1728, 1296 },
	  { N_ ("ARCHD"),		1728, 2592 },
	  { N_ ("ARCHD.Transverse"),	2592, 1728 },
	  { N_ ("ARCHE"),		2592, 3456 },
	  { N_ ("ARCHE.Transverse"),	3456, 2592 },
	  { N_ ("B0"),			2918, 4128 },
	  { N_ ("B1"),			2064, 2918 },
	  { N_ ("B2"),			1458, 2064 },
	  { N_ ("B3"),			1032, 1458 },
	  { N_ ("B4"),			729,  1032 },
	  { N_ ("B5"),			516,  729 },
	  { N_ ("Env10"),		297,  684 },
	  { N_ ("EnvC5"),		459,  649 },
	  { N_ ("EnvDL"),		312,  624 },
	  { N_ ("EnvISOB5"),		499,  709 },
	  { N_ ("EnvMonarch"),		279,  540 },
	  { N_ ("Executive"),		522,  756 },
	  { N_ ("FanFoldUS"),		1071, 792 },
	  { N_ ("Legal"),		612,  1008 },
	  { N_ ("Letter"),		612,  792 },
	  { N_ ("Letter.Transverse"),	792,  612 },
	  { N_ ("Tabloid"),		792,  1224 },
	  { N_ ("TabloidExtra"),	864,  1296 }
	};


/*
 * Local functions...
 */

void	usage(void);
int	write_ppd(const stp_printer_t p, const char *prefix);


/*
 * 'main()' - Process files on the command-line...
 */

int				/* O - Exit status */
main(int  argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
  int		i;		/* Looping var */
  char		*prefix;	/* Directory prefix for output */

  prefix = "ppd";

 /*
  * Initialise libgimpprint
  */

  stp_init();

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

  for (i = 0; i < stp_known_printers(); i++)
    {
      const stp_printer_t printer = stp_get_printer_by_index(i);
      if (printer && write_ppd(printer, prefix))
	return (1);
    }

  return (0);
}


/*
 * 'usage()' - Show program usage...
 */

void
usage(void)
{
  puts(_("Usage: genppd [--help] [--prefix dir]"));
  exit(1);
}


typedef struct
{
  char *name;
  char *canonical_name;
  int width;
  int height;
  int left;
  int right;
  int bottom;
  int top;
} paper_t;

/*
 * 'write_ppd()' - Write a PPD file.
 */

int					/* O - Exit status */
write_ppd(const stp_printer_t p,		/* I - Printer driver */
	  const char      *prefix)	/* I - Prefix (directory) for PPD files */
{
  int		i, j;			/* Looping vars */
  gzFile	fp;			/* File to write to */
  char		filename[1024];		/* Filename */
  char		manufacturer[64];	/* Manufacturer name */
  msize_t	*size;			/* Page size */
  int		num_opts;		/* Number of printer options */
  char		**opts;			/* Printer options */
  const char	*opt;			/* Pointer into option string */
  const char	*defopt;
  int		xdpi, ydpi;		/* Resolution info */
  stp_vars_t	v;			/* Variable info */
  int		width, height,		/* Page information */
		bottom, left,
		top, right;
  const char *driver = stp_printer_get_driver(p);
  const char *long_name = stp_printer_get_long_name(p);
  const stp_vars_t printvars = stp_printer_get_printvars(p);
  int model = stp_printer_get_model(p);
  const stp_printfuncs_t *printfuncs = stp_printer_get_printfuncs(p);
  paper_t *the_papers = NULL;
  int cur_opt = 0;

 /*
  * Skip the PostScript drivers...
  */

  if (strcmp(driver, "ps") == 0 ||
      strcmp(driver, "ps2") == 0)
    return (0);

 /* 
  * Make sure the destination directory exists...
  */

  mkdir(prefix, 0777);
  sprintf(filename, "%s/%s" PPDEXT, prefix, driver);

 /*
  * Open the PPD file...
  */

  if ((fp = gzopen(filename, "wb")) == NULL)
  {
    fprintf(stderr, _("genppd: Unable to create file \"%s\" - %s.\n"),
            filename, strerror(errno));
    return (2);
  }

 /*
  * Write a standard header...
  */

  sscanf(long_name, "%63s", manufacturer);

  fprintf(stderr, _("Writing %s...\n"), filename);

  gzputs(fp, "*PPD-Adobe: \"4.3\"\n");
  gzputs(fp, "*%PPD file for CUPS/GIMP-print.\n");
  gzputs(fp, "*%Copyright 1993-2000 by Easy Software Products, All Rights Reserved.\n");
  gzputs(fp, "*%This PPD file may be freely used and distributed under the terms of\n");
  gzputs(fp, "*%the GNU GPL.\n");
  gzputs(fp, "*FormatVersion:	\"4.3\"\n");
  gzputs(fp, "*FileVersion:	\"" VERSION "\"\n");
  gzputs(fp, "*LanguageVersion: English\n");
  gzputs(fp, "*LanguageEncoding: ISOLatin1\n");
  gzprintf(fp, "*PCFileName:	\"%s.ppd\"\n", driver);
  gzprintf(fp, "*Manufacturer:	\"%s\"\n", _(manufacturer));
  gzputs(fp, "*Product:	\"(GIMP-print v" VERSION ")\"\n");
  gzprintf(fp, "*ModelName:     \"%s\"\n", _(driver));
  gzprintf(fp, "*ShortNickName: \"%s\"\n", _(long_name));
  gzprintf(fp, "*NickName:      \"%s, CUPS+GIMP-print v" VERSION "\"\n", _(long_name));
  gzputs(fp, "*PSVersion:	\"(3010.000) 550\"\n");
  gzputs(fp, "*LanguageLevel:	\"3\"\n");
  gzprintf(fp, "*ColorDevice:	%s\n",
           stp_get_output_type(printvars) == OUTPUT_COLOR ? "True" : "False");
  gzprintf(fp, "*DefaultColorSpace: %s\n", 
           stp_get_output_type(printvars) == OUTPUT_COLOR ? "RGB" : "Gray");
  gzputs(fp, "*FileSystem:	False\n");
  gzputs(fp, "*LandscapeOrientation: Plus90\n");
  gzputs(fp, "*TTRasterizer:	Type42\n");

  gzputs(fp, "*cupsVersion:	1.1\n");
  gzprintf(fp, "*cupsModelNumber: \"%d\"\n", model);
  gzputs(fp, "*cupsManualCopies: True\n");
  gzputs(fp, "*cupsFilter:	\"application/vnd.cups-raster 100 rastertoprinter\"\n");
  if (strcasecmp(manufacturer, "EPSON") == 0)
    gzputs(fp, "*cupsFilter:	\"application/vnd.cups-command 100 commandtoepson\"\n");

 /*
  * Get the page sizes from the driver...
  */

  v = stp_allocate_copy(printvars);

  opts = (*(printfuncs->parameters))(p, NULL, "PageSize", &num_opts);
  defopt = (*(printfuncs->default_parameters))(p, NULL, "PageSize");
  the_papers = malloc(sizeof(paper_t) * num_opts);
  for (i = 0; i < num_opts; i++)
    {
      const stp_papersize_t papersize = stp_get_papersize_by_name(opts[i]);
      width = stp_papersize_get_width(papersize);
      height = stp_papersize_get_height(papersize);
      if (stp_papersize_get_width(papersize) <= 0 ||
	  stp_papersize_get_height(papersize) <= 0)
	continue;
      stp_set_media_size(v, opts[i]);
      (*(printfuncs->media_size))(p, v, &width, &height);
      (*(printfuncs->imageable_area))(p, v, &left, &right, &bottom, &top);
      the_papers[cur_opt].name = opts[i];
      the_papers[cur_opt].width = width;
      the_papers[cur_opt].height = height;
      the_papers[cur_opt].left = left;
      the_papers[cur_opt].right = right;
      the_papers[cur_opt].bottom = bottom;
      the_papers[cur_opt].top = top;
      for (j = sizeof(sizes) / sizeof(sizes[0]), size = sizes;
	   j > 0;
	   j --, size ++)
	if (size->width == width && size->height == height)
	  break;
      if (j)
	{
	  the_papers[cur_opt].canonical_name = malloc(strlen(size->name) + 1);
	  strcpy(the_papers[cur_opt].canonical_name, size->name);
	}
      else
	{
	  the_papers[cur_opt].canonical_name = malloc(32);
	  sprintf(the_papers[cur_opt].canonical_name,
		  "w%dh%d", width, height);
	}
      cur_opt++;
    }

  gzputs(fp, "*OpenUI *PageSize: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *PageSize\n");
  gzputs(fp, "*DefaultPageSize: ");
  gzputs(fp, defopt);
  gzputs(fp, "\n");
  for (i = 0; i < cur_opt; i ++)
  {
    gzprintf(fp,  "*PageSize %s", the_papers[i].canonical_name);
    gzprintf(fp, "/%s:\t\"<</PageSize[%d %d]/ImagingBBox null>>setpagedevice\"\n",
             the_papers[i].name, the_papers[i].width, the_papers[i].height);
  }
  gzputs(fp, "*CloseUI: *PageSize\n");

  gzputs(fp, "*OpenUI *PageRegion: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *PageRegion\n");
  gzputs(fp, "*DefaultPageRegion: ");
  gzputs(fp, defopt);
  gzputs(fp, "\n");
  for (i = 0; i < cur_opt; i ++)
  {
    gzprintf(fp,  "*PageRegion %s", the_papers[i].canonical_name);
    gzprintf(fp, "/%s:\t\"<</PageRegion[%d %d]/ImagingBBox null>>setpagedevice\"\n",
	     the_papers[i].name, the_papers[i].width, the_papers[i].height);
  }
  gzputs(fp, "*CloseUI: *PageRegion\n");

  gzputs(fp, "*DefaultImageableArea: ");
  gzputs(fp, defopt);
  gzputs(fp, "\n");
  for (i = 0; i < cur_opt; i ++)
  {
    gzprintf(fp,  "*ImageableArea %s", the_papers[i].canonical_name);
    gzprintf(fp, "/%s:\t\"%d %d %d %d\"\n", the_papers[i].name,
             the_papers[i].left, the_papers[i].bottom,
	     the_papers[i].right, the_papers[i].top);
  }

  gzputs(fp, "*DefaultPaperDimension: ");
  gzputs(fp, defopt);
  gzputs(fp, "\n");
  for (i = 0; i < cur_opt; i ++)
  {
    gzprintf(fp, "*PaperDimension %s", the_papers[i].canonical_name);
    gzprintf(fp, "/%s:\t\"%d %d\"\n",
	     the_papers[i].name, the_papers[i].width, the_papers[i].height);
  }

  if (opts)
    {
      for (i = 0; i < num_opts; i++)
	free(opts[i]);
      free(opts);
    }
  if (the_papers)
    {
      for (i = 0; i < cur_opt; i++)
	free(the_papers[i].canonical_name);
      free(the_papers);
    }
    

 /*
  * Do we support color?
  */

  gzputs(fp, "*OpenUI *ColorModel: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *ColorModel\n");

  if (stp_get_output_type(printvars) == OUTPUT_COLOR)
    gzputs(fp, "*DefaultColorModel: RGB\n");
  else
    gzputs(fp, "*DefaultColorModel: Gray\n");

  gzprintf(fp, "*ColorModel Gray/Grayscale:\t\"<<"
               "/cupsColorSpace %d"
	       "/cupsColorOrder %d"
	       "/cupsBitsPerColor 8>>setpagedevice\"\n",
           CUPS_CSPACE_W, CUPS_ORDER_CHUNKED);

  if (stp_get_output_type(printvars) == OUTPUT_COLOR)
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

  gzputs(fp, "*CloseUI: *ImageType\n");

 /*
  * Media types...
  */

  opts = (*(printfuncs->parameters))(p, NULL, "MediaType", &num_opts);
  defopt = (*(printfuncs->default_parameters))(p, NULL, "MediaType");

  if (num_opts > 0)
  {
    gzputs(fp, "*OpenUI *MediaType: PickOne\n");
    gzputs(fp, "*OrderDependency: 10 AnySetup *MediaType\n");
    gzputs(fp, "*DefaultMediaType: ");
    for (opt = defopt; *opt; opt ++)
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

  opts = (*(printfuncs->parameters))(p, NULL, "InputSlot", &num_opts);
  defopt = (*(printfuncs->default_parameters))(p, NULL, "InputSlot");

  if (num_opts > 0)
  {
    gzputs(fp, "*OpenUI *InputSlot: PickOne\n");
    gzputs(fp, "*OrderDependency: 10 AnySetup *InputSlot\n");
    gzputs(fp, "*DefaultInputSlot: ");
    for (opt = defopt; *opt; opt ++)
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

  for (i = 0; i < stp_dither_algorithm_count(); i ++)
  {
    const char *s;
    char *copy = xmalloc(strlen(stp_dither_algorithm_name(i)) + 1);
    char *d = copy;
    s = stp_dither_algorithm_name(i);
    do
    {
      if (*s != ' ' && *s != '\t' && *s != '-')
	*d++ = *s;
    } while (*s++);

    if (i == 0)
      gzprintf(fp, "*DefaultDither: %s\n", copy);
    gzprintf(fp, "*Dither %s/%s: \"<</OutputType(%s)>>setpagedevice\"\n",
             copy, stp_dither_algorithm_name(i), stp_dither_algorithm_name(i));
    free(copy);
  }

  gzputs(fp, "*CloseUI: *Dither\n");

 /*
  * Resolutions...
  */

  opts = (*(printfuncs->parameters))(p, NULL, "Resolution", &num_opts);
  defopt = (*(printfuncs->default_parameters))(p, NULL, "Resolution");
  gzputs(fp, "*OpenUI *Resolution: PickOne\n");
  gzputs(fp, "*OrderDependency: 20 AnySetup *Resolution\n");

  if (defopt)
    {
      const char *s = defopt;
      char *copy = xmalloc(strlen(defopt) + 1);
      char *d = copy;
      do
	{
	  if (*s != ' ' && *s != '\t' && *s != '-')
	    *d++ = *s;
	} while (*s++);
      gzprintf(fp, "*DefaultResolution: %s\n", copy);
    }

  for (i = 0; i < num_opts; i ++)
  {
    char *s;
    char *copy = xmalloc(strlen(opts[i]) + 1);
    char *d = copy;
   /* 
    * Strip resolution name to its essentials...
    */
    (printfuncs->describe_resolution)(p, opts[i], &xdpi, &ydpi);

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

  gzprintf(fp, "*%%End of %s.ppd\n", driver);

  gzclose(fp);

  stp_free_vars(v);
  return (0);
}

/*
 * End of "$Id$".
 */
