/*
 * "$Id$"
 *
 *   PPD file generation program for the CUPS driver development kit.
 *
 *   Copyright 1993-2000 by Easy Software Products, All Rights Reserved.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Easy Software Products and are protected by Federal
 *   copyright law.  Distribution and use rights are outlined in the file
 *   "LICENSE.txt" which should have been included with this file.  If this
 *   file is missing or damaged please contact Easy Software Products
 *   at:
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
 *   main()            - Process files on the command-line...
 *   compare_sizes()   - Compare two page sizes...
 *   get_colororder()  - Get a color order value from a name...
 *   get_colorspace()  - Get a colorspace value...
 *   get_measurement() - Get a measurement value...
 *   get_param()       - Get option names for multiple languages...
 *   get_size()        - Get a page size...
 *   get_token()       - Get a token from a file...
 *   scan_file()       - Scan a file for driver definitions.
 *   usage()           - Show program usage...
 *   write_ppd()       - Write a PPD file.
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
#endif /* HAVE_LIBZ */


/*
 * Printer description data...
 */

typedef struct
{
  char		name[32],		/* Name of size */
		text[NUM_LANG][64];	/* Human-readable text */
  int		value;			/* Parameter value, if any */
} param_t;

typedef struct
{
  char		name[32],		/* Name of size */
		text[NUM_LANG][64];	/* Human-readable text */
  int		width,			/* Width in points */
		length;			/* Length in points */
} msize_t;

typedef struct
{
  char		name[32],		/* Name of size */
		text[NUM_LANG][64];	/* Human-readable text */
  int		xdpi,			/* X resolution per inch */
		ydpi,			/* Y resolution per inch */
		colorspace,		/* Colorspace */
		colororder,		/* Color order */
		depth,			/* Depth */
		row_count,		/* Row count */
		row_feed,		/* Row feed */
		row_step;		/* Row step */
} resolution_t;

typedef struct
{
  char		name[32],		/* Name of size */
		text[NUM_LANG][64];	/* Human-readable text */
  int		colorspace,		/* Colorspace */
 		colororder,		/* Color order */
		compression;		/* Compresion mode */
} color_t;

typedef struct
{
  char		resolution[32],		/* Resolution name */
		media_type[32];		/* Media type name */
  float		density,		/* Color profile density */
		gamma,			/* Color profile gamma */
		profile[9];		/* Color profile matrix */
} profile_t;

typedef struct
{
  char		model_name[128];	/* Name of printer model */
  char		pc_file_name[16];	/* 8 character PC filename for PPD */
  int		model_number,		/* Model number for driver */
		manual_copies,		/* Do manual copies? */
		duplexing,		/* Support duplexing? */
		flipduplex,		/* Do flipped duplexing? */
		cutting,		/* Support cutting of media? */
		color_device,		/* Support color? */
		throughput;		/* Throughput in pages per minute */
  float		hw_margins[4];		/* Margins for device */
  int		variable_paper_size,	/* Support variable sizes? */
		max_width,		/* Maximum width (points) */
		max_length,		/* Maximum length (points) */
		min_width,		/* Minimum width (points) */
		min_length,		/* Minimum length (points) */
		num_sizes,		/* Number of fixed sizes */
		sizes[128],		/* Fixed sizes */
		default_size,		/* Default size option */
		num_profiles;		/* Number of color profiles */
  profile_t	profiles[16];		/* Color profiles */
  int		num_filters;		/* Number of filters */
  char		filters[8][128];	/* Filters */
  int		num_resolutions,	/* Number of resolutions */
		default_resolution;	/* Default resolution */
  resolution_t	resolutions[8];		/* Resolutions */
  int		num_colors,		/* Number of color modes */
		default_color;		/* Default color mode */
  color_t	colors[8];		/* Color modes */
  int		num_types,		/* Number of media types */
		default_type;		/* Default media type */
  param_t	types[16];		/* Media types */
  int		num_slots,		/* Number of input slots */
		default_slot;		/* Default input slot */
  param_t	slots[16];		/* Input slots */
  int		num_finishings,		/* Number of finishing options */
		default_finishing;	/* Default finishing option */
  param_t	finishings[16];		/* Finishing options */
  int		num_constraints;	/* Number of constraints */
  char		constraints[100][128];	/* Constraints */
  int		num_installable;	/* Number of installable options */
  param_t	installable[16];	/* Installable options */
} driver_t;


/*
 * Local globals...
 */

msize_t	sizes[] =
	{
	  { "A0",		{ "A0 - 841x1189mm",
				  "A0 - 841x1189mm",
				  "A0 - 841x1189mm",
				  "A0 - 841x1189mm",
				  "A0 - 841x1189mm" },		2380, 3368 },
	  { "A0.Transverse",	{ "A0 - 1189x841mm",
				  "A0 - 1189x841mm",
				  "A0 - 1189x841mm",
				  "A0 - 1189x841mm",
				  "A0 - 1189x841mm" }, 		3368, 2380 },
	  { "A1",		{ "A1 - 594x841mm",
	                          "A1 - 594x841mm",
	                          "A1 - 594x841mm",
	                          "A1 - 594x841mm",
	                          "A1 - 594x841mm" },		1684, 2380 },
	  { "A1.Transverse",	{ "A1 - 841x594mm",
	  			  "A1 - 841x594mm",
	  			  "A1 - 841x594mm",
	  			  "A1 - 841x594mm",
	  			  "A1 - 841x594mm" },		2380, 1684 },
	  { "A2",		{ "A2 - 420x594mm",
				  "A2 - 420x594mm",
				  "A2 - 420x594mm",
				  "A2 - 420x594mm",
				  "A2 - 420x594mm" },		1190, 1684 },
	  { "A2.Transverse",	{ "A2 - 594x420mm",
	  			  "A2 - 594x420mm",
	  			  "A2 - 594x420mm",
	  			  "A2 - 594x420mm",
	  			  "A2 - 594x420mm" },		1684, 1190 },
	  { "A3",		{ "A3 - 297x420mm",
	                          "A3 - 297x420mm",
	                          "A3 - 297x420mm",
	                          "A3 - 297x420mm",
	                          "A3 - 297x420mm" },		842,  1190 },
	  { "A3.Transverse",	{ "A3 - 420x297mm",
	  			  "A3 - 420x297mm",
	  			  "A3 - 420x297mm",
	  			  "A3 - 420x297mm",
	  			  "A3 - 420x297mm" },		1190, 842 },
	  { "A4",		{ "A4 - 210x297mm",
	  			  "A4 - 210x297mm",
	  			  "A4 - 210x297mm",
	  			  "A4 - 210x297mm",
	  			  "A4 - 210x297mm" },		595,  842 },
	  { "A4.Transverse",	{ "A4 - 297x210mm",
	  			  "A4 - 297x210mm",
	  			  "A4 - 297x210mm",
	  			  "A4 - 297x210mm",
	  			  "A4 - 297x210mm" },		842,  595 },
	  { "A5",		{ "A5 - 149x210mm",
	  			  "A5 - 149x210mm",
	  			  "A5 - 149x210mm",
	  			  "A5 - 149x210mm",
	  			  "A5 - 149x210mm" },		421,  595 },
	  { "A5.Transverse",	{ "A5 - 210x149mm",
	  			  "A5 - 210x149mm",
	  			  "A5 - 210x149mm",
	  			  "A5 - 210x149mm",
	  			  "A5 - 210x149mm" },		595,  421 },
	  { "A6",		{ "A6 - 105x149mm",
	  			  "A6 - 105x149mm",
	  			  "A6 - 105x149mm",
	  			  "A6 - 105x149mm",
	  			  "A6 - 105x149mm" },		297,  421 },
	  { "AnsiA",		{ "ANSI A - 8.5x11in",
	  			  "ANSI A - 8.5x11in",
	  			  "ANSI A - 8.5x11in",
	  			  "ANSI A - 8.5x11in",
	  			  "ANSI A - 8.5x11in" },	612,  792 },
	  { "AnsiB",		{ "ANSI B - 11x17in",
	  			  "ANSI B - 11x17in",
	  			  "ANSI B - 11x17in",
	  			  "ANSI B - 11x17in",
	  			  "ANSI B - 11x17in" },		792,  1224 },
	  { "AnsiC",		{ "ANSI C - 17x22in",
	  			  "ANSI C - 17x22in",
	  			  "ANSI C - 17x22in",
	  			  "ANSI C - 17x22in",
	  			  "ANSI C - 17x22in" },		1224, 1584 },
	  { "AnsiD",		{ "ANSI D - 22x34in",
	  			  "ANSI D - 22x34in",
	  			  "ANSI D - 22x34in",
	  			  "ANSI D - 22x34in",
	  			  "ANSI D - 22x34in" },		1584, 2448 },
	  { "AnsiE",		{ "ANSI E - 34x44in",
	  			  "ANSI E - 34x44in",
	  			  "ANSI E - 34x44in",
	  			  "ANSI E - 34x44in",
	  			  "ANSI E - 34x44in" },		2448, 3168 },
	  { "ARCHA",		{ "Arch. A - 9x12in",
	  			  "Arch. A - 9x12in",
	  			  "Arch. A - 9x12in",
	  			  "Arch. A - 9x12in",
	  			  "Arch. A - 9x12in" },		648,  864 },
	  { "ARCHA.Transverse",	{ "Arch. A - 12x9in",
	  			  "Arch. A - 12x9in",
	  			  "Arch. A - 12x9in",
	  			  "Arch. A - 12x9in",
	  			  "Arch. A - 12x9in" },		864,  648 },
	  { "ARCHB",		{ "Arch. B - 12x18in",
	  			  "Arch. B - 12x18in",
	  			  "Arch. B - 12x18in",
	  			  "Arch. B - 12x18in",
	  			  "Arch. B - 12x18in" },	864,  1296 },
	  { "ARCHB.Transverse",	{ "Arch. B - 18x12in",
	  			  "Arch. B - 18x12in",
	  			  "Arch. B - 18x12in",
	  			  "Arch. B - 18x12in",
	  			  "Arch. B - 18x12in" },	1296, 864 },
	  { "ARCHC",		{ "Arch. C - 18x24in",
	  			  "Arch. C - 18x24in",
	  			  "Arch. C - 18x24in",
	  			  "Arch. C - 18x24in",
	  			  "Arch. C - 18x24in" },	1296, 1728 },
	  { "ARCHC.Transverse",	{ "Arch. C - 24x18in",
	  			  "Arch. C - 24x18in",
	  			  "Arch. C - 24x18in",
	  			  "Arch. C - 24x18in",
	  			  "Arch. C - 24x18in" },	1728, 1296 },
	  { "ARCHD",		{ "Arch. D - 24x36in",
	  			  "Arch. D - 24x36in",
	  			  "Arch. D - 24x36in",
	  			  "Arch. D - 24x36in",
	  			  "Arch. D - 24x36in" },	1728, 2592 },
	  { "ARCHD.Transverse",	{ "Arch. D - 36x24in",
	  			  "Arch. D - 36x24in",
	  			  "Arch. D - 36x24in",
	  			  "Arch. D - 36x24in",
	  			  "Arch. D - 36x24in" },	2592, 1728 },
	  { "ARCHE",		{ "Arch. E - 36x48in",
	  			  "Arch. E - 36x48in",
	  			  "Arch. E - 36x48in",
	  			  "Arch. E - 36x48in",
	  			  "Arch. E - 36x48in" },	2592, 3456 },
	  { "ARCHE.Transverse",	{ "Arch. E - 48x36in",
	  			  "Arch. E - 48x36in",
	  			  "Arch. E - 48x36in",
	  			  "Arch. E - 48x36in",
	  			  "Arch. E - 48x36in" },	3456, 2592 },
	  { "B0",		{ "B0 - 1028x1456mm",
	  			  "B0 - 1028x1456mm",
	  			  "B0 - 1028x1456mm",
	  			  "B0 - 1028x1456mm",
	  			  "B0 - 1028x1456mm" },		2918, 4128 },
	  { "B1",		{ "B1 - 728x1028mm",
	  			  "B1 - 728x1028mm",
	  			  "B1 - 728x1028mm",
	  			  "B1 - 728x1028mm",
	  			  "B1 - 728x1028mm" },		2064, 2918 },
	  { "B2",		{ "B2 - 514x728mm",
	  			  "B2 - 514x728mm",
	  			  "B2 - 514x728mm",
	  			  "B2 - 514x728mm",
	  			  "B2 - 514x728mm" },		1458, 2064 },
	  { "B3",		{ "B3 - 364x514mm",
	  			  "B3 - 364x514mm",
	  			  "B3 - 364x514mm",
	  			  "B3 - 364x514mm",
	  			  "B3 - 364x514mm" },		1032, 1458 },
	  { "B4",		{ "B4 - 257x364mm",
	  			  "B4 - 257x364mm",
	  			  "B4 - 257x364mm",
	  			  "B4 - 257x364mm",
	  			  "B4 - 257x364mm" },		729,  1032 },
	  { "B5",		{ "B5 - 182x257mm",
	  			  "B5 - 182x257mm",
	  			  "B5 - 182x257mm",
	  			  "B5 - 182x257mm",
	  			  "B5 - 182x257mm" },		516,  729 },
	  { "Env10",		{ "#10 Envelope - 4.13x9.5in",
	  			  "Env. US #10 - 4.13x9.5in",
	  			  "Busta n.10 - 4.13x9.5in",
	  			  "Envelope #10 - 4.13x9.5in",
	  			  "Sobre n<ba>10 - 4.13x9.5in" }, 297,  684 },
	  { "EnvC5",		{ "C5 Envelope - 162x229mm",
	  			  "Env. C5 - 162x229mm",
	  			  "Busta C5 - 162x229mm",
	  			  "Envelope C5 - 162x229mm",
	  			  "Sobre C5 - 162x229mm" },	459,  649 },
	  { "EnvDL",		{ "DL Envelope - 110x220mm",
	  			  "Env. DL - 110x220mm",
	  			  "Busta DL - 110x220mm",
	  			  "Envelope DL - 110x220mm",
	  			  "Sobre DL - 110x220mm" },	312,  624 },
	  { "EnvISOB5",		{ "ISOB5 Envelope - 176x250mm",
	  			  "Env. ISOB5 - 176x250mm",
	  			  "Busta ISOB5 - 176x250mm",
	  			  "Envelope ISOB5 - 176x250mm",
	  			  "Sobre ISOB5 - 176x250mm" }, 499,  709 },
	  { "EnvMonarch",	{ "Monarch Envelope - 3.88x7.5in",
	  			  "Env. Monarch US - 3.88x7.5in",
	  			  "Busta Monarch - 3.88x7.5in",
	  			  "Envelope Monarch - 3.88x7.5in",
	  			  "Sobre monarca - 3.88x7.5in" }, 279,  540 },
	  { "Executive",	{ "Executive - 7.25x10.5in",
	  			  "Ex<e9>cutive - 7.25x10.5in",
	  			  "Executive - 7.25x10.5in",
	  			  "Executive - 7.25x10.5in",
	  			  "Ejecutivo - 7.25x10.5in" },	522,  756 },
	  { "FanFoldUS",	{ "Fanfold - 14.875x11in",
	  			  "Fanfold - 14.875x11in",
	  			  "Fanfold - 14.875x11in",
	  			  "Fanfold - 14.875x11in",
	  			  "Fanfold - 14.875x11in" },	1071, 792 },
	  { "Legal",		{ "Legal - 8.5x14in",
	  			  "L<e9>gal - 8.5x14in",
	  			  "Legal - 8.5x14in",
	  			  "Legale - 8.5x14in",
	  			  "Oficio - 8.5x14in" },		612,  1008 },
	  { "Letter",		{ "Letter - 8.5x11in",
	  			  "Lettre US - 8.5x11in",
	  			  "Lettera - 8.5x11in",
	  			  "Letter - 8.5x11in",
	  			  "Carta - 8.5x11in" },	612,  792 },
	  { "Letter.Transverse",{ "Letter - 11x8.5in",
	  			  "Lettre US - 11x8.5in",
	  			  "Lettera - 11x8.5in",
	  			  "Letter - 11x8.5in",
	  			  "Carta - 11x8.5in" },	792,  612 },
	  { "Photo4x6",		{ "Photo - 4x6in",
	  			  "Photo - 4x6in",
	  			  "Photo - 4x6in",
	  			  "Photo - 4x6in",
	  			  "Photo - 4x6in" },		288,  432 },
	  { "PhotoLabel",	{ "Photo Labels - 4x6.5in",
	  			  "Photo Labels - 4x6.5in",
	  			  "Photo Labels - 4x6.5in",
	  			  "Photo Labels - 4x6.5in",
	  			  "Photo Labels - 4x6.5in" },	288,  468 },
	  { "Tabloid",		{ "Tabloid - 11x17in",
	  			  "Tabloid - 11x17in",
	  			  "Tabloid - 11x17in",
	  			  "Tabloid - 11x17in",
	  			  "Tabloid - 11x17in" },	792,  1224 },
	  { "TabloidExtra",	{ "Tabloid Extra - 12x18in",
	  			  "Tabloid Extra - 12x18in",
	  			  "Tabloid Extra - 12x18in",
	  			  "Tabloid Extra - 12x18in",
	  			  "Tabloid Extra - 12x18in" },	864,  1296 },
	  { "w288h418",		{ "Photo - 4x5.8in",
	  			  "Photo - 4x5.8in",
	  			  "Photo - 4x5.8in",
	  			  "Photo - 4x5.8in",
	  			  "Photo - 4x5.8in" },		288,  418 },
	  { "w936h1368",	{ "Super B/A3 - 13x19in",
	  			  "Super B/A3 - 13x19in",
	  			  "Super B/A3 - 13x19in",
	  			  "Super B/A3 - 13x19in",
	  			  "Super B/A3 - 13x19in" },	936,  1368 }
	};
static char	*locales[] =	/* Locale names */
		{
		  "en",
		  "fr",
		  "it",
		  "de",
		  "es"
		};


/*
 * Local functions...
 */

int	compare_sizes(msize_t *s0, msize_t *s1);
int	get_colororder(const char *name);
int	get_colorspace(const char *name);
float	get_measurement(const char *name);
void	get_param(char *token, param_t *param);
int	get_size(const char *name);
char	*get_token(FILE *fp, char *buffer, int buflen);
int	scan_file(FILE *fp, driver_t *drv, const char *prefix);
void	usage(void);
int	write_ppd(driver_t *drv, int lang, const char *prefix);


/*
 * 'main()' - Process files on the command-line...
 */

int				/* O - Exit status */
main(int  argc,			/* I - Number of command-line arguments */
     char *argv[])		/* I - Command-line arguments */
{
  int	i;			/* Looping var */
  int	status;			/* Exit status */
  FILE	*fp;			/* Open file */
  char	filename[255];		/* Dependency file */
  char	*prefix;		/* Directory prefix for output */


  status = 0;
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
    else if ((fp = fopen(argv[i], "r")) != NULL)
    {
     /*
      * Scan the driver info file...
      */

      status = scan_file(fp, NULL, prefix);
      fclose(fp);
      if (status)
        break;

     /*
      * Create a dependency file...
      */

      strcpy(filename, argv[i]);
      strcpy(filename + strlen(filename) - 4, ".dep");
      if ((fp = fopen(filename, "w")) != NULL)
        fclose(fp);
    }
    else
    {
      fprintf(stderr, "genppd: Unable to open \"%s\" - %s.\n", argv[i],
              strerror(errno));
      status = 1;
      break;
    }

  return (status);
}


/*
 * 'compare_sizes()' - Compare two page sizes...
 */

int				/* O - Result of comparison */
compare_sizes(msize_t *s0,	/* I - First size */
              msize_t *s1)	/* I - Second size */
{
  return (strcasecmp(s0->name, s1->name));
}


/*
 * 'get_colororder()' - Get a color order value from a name...
 */

int					/* O - Color order value */
get_colororder(const char *name)	/* I - Color order name */
{
  if (strcasecmp(name, "chunked") == 0 ||
      strcasecmp(name, "chunky") == 0)
    return (CUPS_ORDER_CHUNKED);
  else if (strcasecmp(name, "banded") == 0)
    return (CUPS_ORDER_BANDED);
  else if (strcasecmp(name, "planar") == 0)
    return (CUPS_ORDER_PLANAR);
  else
    return (-1);
}


/*
 * 'get_colorspace()' - Get a colorspace value...
 */

int					/* O - Colorspace value */
get_colorspace(const char *name)	/* I - Colorspace name */
{
  if (strcasecmp(name, "w") == 0)
    return (CUPS_CSPACE_W);
  else if (strcasecmp(name, "rgb") == 0)
    return (CUPS_CSPACE_RGB);
  else if (strcasecmp(name, "rgba") == 0)
    return (CUPS_CSPACE_RGBA);
  else if (strcasecmp(name, "k") == 0)
    return (CUPS_CSPACE_K);
  else if (strcasecmp(name, "cmy") == 0)
    return (CUPS_CSPACE_CMY);
  else if (strcasecmp(name, "ymc") == 0)
    return (CUPS_CSPACE_YMC);
  else if (strcasecmp(name, "cmyk") == 0)
    return (CUPS_CSPACE_CMYK);
  else if (strcasecmp(name, "ymck") == 0)
    return (CUPS_CSPACE_YMCK);
  else if (strcasecmp(name, "kcmy") == 0)
    return (CUPS_CSPACE_KCMY);
  else if (strcasecmp(name, "kcmycm") == 0)
    return (CUPS_CSPACE_KCMYcm);
  else if (strcasecmp(name, "gmck") == 0)
    return (CUPS_CSPACE_GMCK);
  else if (strcasecmp(name, "gmcs") == 0)
    return (CUPS_CSPACE_GMCS);
  else if (strcasecmp(name, "white") == 0)
    return (CUPS_CSPACE_WHITE);
  else if (strcasecmp(name, "gold") == 0)
    return (CUPS_CSPACE_GOLD);
  else if (strcasecmp(name, "silver") == 0)
    return (CUPS_CSPACE_SILVER);
  else
    return (-1);
}


/*
 * 'get_measurement()' - Get a measurement value...
 */

float					/* O - Measurement in points */
get_measurement(const char *name)	/* I - Measurement string */
{
  float	val;				/* Measurement value */


 /*
  * Get the floating point value of "s" and skip all digits and decimal points.
  */

  val = (float)atof(name);
  while (isdigit(*name) || *name == '.')
    name ++;

 /*
  * Check for a trailing unit specifier...
  */

  if (strcasecmp(name, "mm") == 0)
    val *= 72.0f / 25.4f;
  else if (strcasecmp(name, "cm") == 0)
    val *= 72.0f / 2.54f;
  else if (strncasecmp(name, "in", 2) == 0)
    val *= 72.0f;

  return ((int)val);
}


/*
 * 'get_param()' - Get option names for multiple languages...
 */

void
get_param(char    *token,	/* I - Token to dice up into pieces */
          param_t *param)	/* I - Parameter to fill */
{
  int	lang;			/* Language for this string */
  char	*tptr;			/* Pointer into token */


 /*
  * First get the parameter name...
  */

  if ((tptr = strchr(token, '/')) != NULL)
    *tptr++ = '\0';
  else
    tptr = token;

  strncpy(param->name, token, sizeof(param->name) - 1);
  param->name[sizeof(param->name) - 1] = '\0';

 /*
  * Loop until we have all of the language strings...
  */

  while (*tptr != '\0')
  {
   /*
    * Get the next string...
    */

    token = tptr;

    if ((tptr = strchr(token, '/')) != NULL)
      *tptr++ = '\0';
    else
      tptr = token + strlen(token);

   /*
    * See if we have a localized string or not...
    */

    if (isalpha(token[0]) && isalpha(token[1]) && token[2] == '=')
    {
     /*
      * Yes, find the language...
      */

      token[2] = '\0';
      for (lang = 0; lang < NUM_LANG; lang ++)
        if (strcmp(token, locales[lang]) == 0)
	{
          strncpy(param->text[lang], token + 3, sizeof(param->text[0]) - 1);
	  param->text[lang][sizeof(param->text[0]) - 1] = '\0';
	}
    }
    else
    {
     /*
      * No, make this the default...
      */

      for (lang = 0; lang < NUM_LANG; lang ++)
      {
        strncpy(param->text[lang], token, sizeof(param->text[0]) - 1);
	param->text[lang][sizeof(param->text[0]) - 1] = '\0';
      }
    }
  }
}


/*
 * 'get_size()' - Get a page size...
 */

int				/* O - Page size index or -1 */
get_size(const char *name)	/* I - Page size name */
{
  msize_t	key,		/* Search key */
		*match;		/* Matching size */


  strncpy(key.name, name, sizeof(key.name) - 1);
  key.name[sizeof(key.name) - 1] = '\0';
  match = bsearch(&key, sizes, sizeof(sizes) / sizeof(sizes[0]), sizeof(sizes[0]),
                  (int (*)(const void *, const void *))compare_sizes);

  if (match == NULL)
  {
    fprintf(stderr, "genppd: Unknown media size \"%s\"!\n", name);
    return (-1);
  }
  else
    return (match - sizes);
}


/*
 * 'get_token()' - Get a token from a file...
 */

char *				/* O - Token string or NULL on EOF */
get_token(FILE *fp,		/* I - File to read from */
          char *buffer,		/* IO - Buffer */
	  int  buflen)		/* I - Length of buffer */
{
  char	*bufptr,		/* Pointer into string buffer */
	*bufend;		/* Pointer to end of string buffer */
  int	ch;			/* Character from file */
  int	quote;			/* Quote character used... */


  bufptr = buffer;
  bufend = buffer + buflen - 1;

  while ((ch = getc(fp)) != EOF)
  {
    if (isspace(ch))
    {
      if (bufptr == buffer)
        continue;
      else
        break;
    }
    else if (ch == '#')
    {
      while ((ch = getc(fp)) != EOF)
        if (ch == '\n')
	  break;

      if (ch == EOF)
        break;
    }
    else if (ch == '\'' || ch == '\"')
    {
      quote = ch;

      while ((ch = getc(fp)) != EOF)
        if (ch == quote || ch == '\n')
	  break;
	else if (bufptr < bufend)
	  *bufptr++ = ch;
    }
    else if (bufptr < bufend)
      *bufptr++ = ch;
  }

  *bufptr = '\0';
  if (bufptr > buffer)
    return (buffer);
  else
    return (NULL);
}


/*
 * 'scan_file()' - Scan a file for driver definitions.
 */

int				/* O - Exit status */
scan_file(FILE       *fp,	/* I - File to scan */
          driver_t   *drv,	/* I - Driver information */
	  const char *prefix)	/* I - PPD file directory prefix */
{
  int		i,		/* Looping var */
		isdefault;	/* Non-zero if this is the default */
  driver_t	temp;		/* Driver information */
  char		token[1024],	/* Token from file */
		*tptr;		/* Pointer into token */


  if (drv)
    memcpy(&temp, drv, sizeof(driver_t));
  else
    memset(&temp, 0, sizeof(driver_t));

  while (get_token(fp, token, sizeof(token)) != NULL)
  {
    if (token[0] == '*')
    {
      isdefault = 1;
      strcpy(token, token + 1);
    }
    else
      isdefault = 0;

    if (strcmp(token, "{") == 0)
      scan_file(fp, &temp, prefix);
    else if (strcmp(token, "}") == 0)
      break;
    else if (strcasecmp(token, "ColorDevice") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing boolean value following ColorDevice attribute!\n",
	      stderr);
	return (1);
      }

      temp.color_device = strcasecmp(token, "true") == 0;
    }
    else if (strcasecmp(token, "ColorModel") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing name/text value following ColorModel attribute!\n",
	      stderr);
	return (1);
      }

      get_param(token, (param_t *)(temp.colors + temp.num_colors));

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing colorspace value following ColorModel attribute!\n",
	      stderr);
	return (1);
      }

      temp.colors[temp.num_colors].colorspace = get_colorspace(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing color order value following ColorModel attribute!\n",
	      stderr);
	return (1);
      }

      temp.colors[temp.num_colors].colororder = get_colororder(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing compression value following ColorModel attribute!\n",
	      stderr);
	return (1);
      }

      temp.colors[temp.num_colors].compression = atoi(token);

      if (isdefault)
        temp.default_color = temp.num_colors;

      temp.num_colors ++;
    }
    else if (strcasecmp(token, "ColorProfile") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing resolution/mediatype value following ColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      if ((tptr = strchr(token, '/')) != NULL)
        *tptr++ = '\0';
      else
        tptr = token;

      strcpy(temp.profiles[temp.num_profiles].resolution, token);
      strcpy(temp.profiles[temp.num_profiles].media_type, tptr);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing density value following ColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      temp.profiles[temp.num_profiles].density = atof(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing gamma value following ColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      temp.profiles[temp.num_profiles].gamma = atof(token);

      for (i = 0; i < 9; i ++)
      {
	if (get_token(fp, token, sizeof(token)) == NULL)
	{
          fputs("genppd: Missing matrix value following ColorProfile attribute!\n",
		stderr);
	  return (1);
	}

	temp.profiles[temp.num_profiles].profile[i] = atof(token);
      }

      temp.num_profiles ++;
    }
    else if (strcasecmp(token, "Cutter") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing boolean value following Cutter attribute!\n",
	      stderr);
	return (1);
      }

      temp.cutting = strcasecmp(token, "true") == 0;
    }
    else if (strcasecmp(token, "Duplex") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing boolean value following Duplex attribute!\n",
	      stderr);
	return (1);
      }

      temp.duplexing  = strcasecmp(token, "true") == 0 ||
                        strcasecmp(token, "flip") == 0;
      temp.flipduplex = strcasecmp(token, "flip") == 0;
    }
    else if (strcasecmp(token, "Filter") == 0)
    {
      if (get_token(fp, temp.filters[temp.num_filters],
                    sizeof(temp.filters[0])) == NULL)
      {
        fputs("genppd: Missing filter value following Filter attribute!\n",
	      stderr);
	return (1);
      }

      temp.num_filters ++;
    }
    else if (strcasecmp(token, "Finishing") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing name/text value following Finishing attribute!\n",
	      stderr);
	return (1);
      }

      get_param(token, temp.finishings + temp.num_finishings);

      if (isdefault)
        temp.default_finishing = temp.num_finishings;

      temp.num_finishings ++;
    }
    else if (strcasecmp(token, "HWMargins") == 0)
    {
      for (i = 0; i < 4; i ++)
      {
	if (get_token(fp, token, sizeof(token)) == NULL)
	{
          fputs("genppd: Missing margin value following HWMargins attribute!\n",
		stderr);
	  return (1);
	}

	temp.hw_margins[i] = get_measurement(token);
      }
    }
    else if (strcasecmp(token, "InputSlot") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing position value following InputSlot attribute!\n",
	      stderr);
	return (1);
      }

      temp.slots[temp.num_slots].value = atoi(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing name/text value following InputSlot attribute!\n",
	      stderr);
	return (1);
      }

      get_param(token, temp.slots + temp.num_slots);

      if (isdefault)
        temp.default_slot = temp.num_slots;

      temp.num_slots ++;
    }
    else if (strcasecmp(token, "Installable") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing name/text value following Installable attribute!\n",
	      stderr);
	return (1);
      }

      get_param(token, temp.installable + temp.num_installable);

      temp.num_installable ++;
    }
    else if (strcasecmp(token, "ManualCopies") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing boolean value following ManualCopies attribute!\n",
	      stderr);
	return (1);
      }

      temp.manual_copies = strcasecmp(token, "true") == 0;
    }
    else if (strcasecmp(token, "MaxSize") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing width value following MaxSize attribute!\n",
	      stderr);
	return (1);
      }

      temp.max_width = get_measurement(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing length value following MaxSize attribute!\n",
	      stderr);
	return (1);
      }

      temp.max_length = get_measurement(token);
    }
    else if (strcasecmp(token, "MediaSize") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing size value following MediaSize attribute!\n",
	      stderr);
	return (1);
      }

      if ((temp.sizes[temp.num_sizes] = get_size(token)) >= 0)
      {
	if (isdefault)
          temp.default_size = temp.num_sizes;

        temp.num_sizes ++;
      }
    }
    else if (strcasecmp(token, "MediaType") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing type value following MediaType attribute!\n",
	      stderr);
	return (1);
      }

      temp.types[temp.num_types].value = atoi(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing name/text value following MediaType attribute!\n",
	      stderr);
	return (1);
      }

      get_param(token, temp.types + temp.num_types);

      if (isdefault)
        temp.default_type = temp.num_types;

      temp.num_types ++;
    }
    else if (strcasecmp(token, "MinSize") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing width value following MinSize attribute!\n",
	      stderr);
	return (1);
      }

      temp.min_width = get_measurement(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing length value following MinSize attribute!\n",
	      stderr);
	return (1);
      }

      temp.min_length = get_measurement(token);
    }
    else if (strcasecmp(token, "ModelName") == 0)
    {
      if (get_token(fp, temp.model_name, sizeof(temp.model_name)) == NULL)
      {
        fputs("genppd: Missing name value following ModelName attribute!\n",
	      stderr);
	return (1);
      }
    }
    else if (strcasecmp(token, "ModelNumber") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing model value following ModelNumber attribute!\n",
	      stderr);
	return (1);
      }

      temp.model_number = atoi(token);
    }
    else if (strcasecmp(token, "PCFileName") == 0)
    {
      if (get_token(fp, temp.pc_file_name, sizeof(temp.pc_file_name)) == NULL)
      {
        fputs("genppd: Missing filename value following PCFileName attribute!\n",
	      stderr);
	return (1);
      }
    }
    else if (strcasecmp(token, "Resolution") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing colorspace value following Resolution attribute!\n",
	      stderr);
	return (1);
      }

      temp.resolutions[temp.num_resolutions].colororder = get_colororder(token);
      temp.resolutions[temp.num_resolutions].colorspace = get_colorspace(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing depth value following Resolution attribute!\n",
	      stderr);
	return (1);
      }

      temp.resolutions[temp.num_resolutions].depth = atoi(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing row count value following Resolution attribute!\n",
	      stderr);
	return (1);
      }

      temp.resolutions[temp.num_resolutions].row_count = atoi(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing row feed value following Resolution attribute!\n",
	      stderr);
	return (1);
      }

      temp.resolutions[temp.num_resolutions].row_feed = atoi(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing row step value following Resolution attribute!\n",
	      stderr);
	return (1);
      }

      temp.resolutions[temp.num_resolutions].row_step = atoi(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing name/text value following Resolution attribute!\n",
	      stderr);
	return (1);
      }

      get_param(token, (param_t *)(temp.resolutions + temp.num_resolutions));

      if (sscanf(token, "%dx%d", &(temp.resolutions[temp.num_resolutions].xdpi),
		 &(temp.resolutions[temp.num_resolutions].ydpi)) == 1)
        temp.resolutions[temp.num_resolutions].ydpi =
	    temp.resolutions[temp.num_resolutions].xdpi;

      if (isdefault)
        temp.default_resolution = temp.num_resolutions;

      temp.num_resolutions ++;
    }
    else if (strcasecmp(token, "SimpleColorProfile") == 0)
    {
      float	kd, rd, g;		/* Densities and gamma */
      float	red, green, blue;	/* RGB adjustments */
      float	yellow;			/* Yellow density */
      float	color;			/* Color density values */
      profile_t	*p;			/* Color profile */


      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing resolution/mediatype value following SimpleColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      if ((tptr = strchr(token, '/')) != NULL)
        *tptr++ = '\0';
      else
        tptr = token;

      strcpy(temp.profiles[temp.num_profiles].resolution, token);
      strcpy(temp.profiles[temp.num_profiles].media_type, tptr);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing black density value following SimpleColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      kd = atof(token) * 0.01f;

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing yellow density value following SimpleColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      yellow = atof(token) * 0.01f / kd;

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing red density value following SimpleColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      rd = atof(token) * 0.01f;

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing gamma value following SimpleColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      g = atof(token);

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing red adjustment value following SimpleColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      red = atof(token) * 0.01f;

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing green adjustment value following SimpleColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      green = atof(token) * 0.01f;

      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing blue adjustment value following SimpleColorProfile attribute!\n",
	      stderr);
	return (1);
      }

      blue = atof(token) * 0.01f;

      color = 0.5f * rd / kd - kd;

      p = temp.profiles + temp.num_profiles;

      p->density    = kd;
      p->gamma      = g;

      p->profile[0] = 1.0f;			/* C */
      p->profile[1] = color + blue;		/* C + M (blue) */
      p->profile[2] = color - green;		/* C + Y (green) */
      p->profile[3] = color - blue;		/* M + C (blue) */
      p->profile[4] = 1.0f;			/* M */
      p->profile[5] = color + red;		/* M + Y (red) */
      p->profile[6] = yellow * (color + green);	/* Y + C (green) */
      p->profile[7] = yellow * (color - red);	/* Y + M (red) */
      p->profile[8] = yellow;			/* Y */

      if (p->profile[1] > 0.0f)
      {
        p->profile[3] -= p->profile[1];
	p->profile[1] = 0.0f;
      }
      else if (p->profile[3] > 0.0f)
      {
        p->profile[1] -= p->profile[3];
	p->profile[3] = 0.0f;
      }

      if (p->profile[2] > 0.0f)
      {
        p->profile[6] -= p->profile[2];
	p->profile[2] = 0.0f;
      }
      else if (p->profile[6] > 0.0f)
      {
        p->profile[2] -= p->profile[6];
	p->profile[6] = 0.0f;
      }

      if (p->profile[5] > 0.0f)
      {
        p->profile[7] -= p->profile[5];
	p->profile[5] = 0.0f;
      }
      else if (p->profile[7] > 0.0f)
      {
        p->profile[5] -= p->profile[7];
	p->profile[7] = 0.0f;
      }

      temp.num_profiles ++;
    }
    else if (strcasecmp(token, "Throughput") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing value following Throughput attribute!\n",
	      stderr);
	return (1);
      }

      temp.throughput = atoi(token);
    }
    else if (strcasecmp(token, "UIConstraints") == 0)
    {
      if (get_token(fp, temp.constraints[temp.num_constraints],
                    sizeof(temp.constraints[0])) == NULL)
      {
        fputs("genppd: Missing string value following UIConstraints attribute!\n",
	      stderr);
	return (1);
      }

      temp.num_constraints ++;
    }
    else if (strcasecmp(token, "VariablePaperSize") == 0)
    {
      if (get_token(fp, token, sizeof(token)) == NULL)
      {
        fputs("genppd: Missing boolean value following VariablePaperSize attribute!\n",
	      stderr);
	return (1);
      }

      temp.variable_paper_size = strcasecmp(token, "true") == 0;
    }
    else
    {
      fprintf(stderr, "genppd: Unknown token \"%s\" seen!\n", token);
      return (1);
    }
  }

  if (temp.pc_file_name[0])
  {
    for (i = 0; i < NUM_LANG; i ++)
      if (write_ppd(&temp, i, prefix))
        return (2);
  }

  return (0);
}


/*
 * 'usage()' - Show program usage...
 */

void
usage(void)
{
  puts("Usage: genppd [--help] [--prefix dir] filename.drv ... filenameN.drv");
  exit(1);
}


/*
 * 'write_ppd()' - Write a PPD file.
 */

int				/* O - Exit status */
write_ppd(driver_t   *drv,	/* I - Driver information */
          int        lang,	/* I - Language */
	  const char *prefix)	/* I - Prefix (directory) for PPD files */
{
  int		i;		/* Looping var */
  gzFile	fp;		/* File to write to */
  char		filename[1024];	/* Filename */
  msize_t	*size;		/* Page size */
  static char	*langs[] =	/* Languages */
		{
		  "English",
		  "French",
		  "Italian",
		  "German",
		  "Spanish"
		};
  static char	*cutmedia[]=	/* CutMedia option */
		{
		  "Cut Media",
		  "Coupez Les Medias",
		  "Tagliare I Media",
		  "Schneiden Sie Media",
		  "Corte Los Media",
		};
  static char	*duplex[]=	/* Duplex option */
		{
		  "Duplex",
		  "Recto-verso",
		  "Duplex",
		  "Duplex",
		  "D<fa>plex",
		};
  static char	*duplexes[][3] =/* Duplex strings */
		{
		  { "Off",
		    "Long-Edge Binding",
		    "Short-Edge Binding" },
		  { "Non",
		    "Retourner sur les bords longs (standard)",
		    "Retourner sur les bords courts" },
		  { "Disattivata (facciata singola)",
		    "Ruota sul lato lungo (Standard)",
		    "Ruota sul lato corto" },
		  { "Aus (einseitig)",
		    "An langer Kante spiegeln (Standard)",
		    "An kurzer Kante spiegeln" },
		  { "Des (por un solo lado)",
		    "Dar vuelta - borde largo (est<e1>ndar)",
		    "Dar vuelta - borde corto" }
		};
  static char	*installable[][2] =/* Installable strings */
		{
		  { "Installed", "Not Installed" },
		  { "Install<e9>", "Non install<e9>" },
		  { "Installato", "Non installato" },
		  { "Installiert", "Nicht installiert" },
		  { "Instalada", "No instalada" }
		};


  mkdir(prefix, 0777);
  sprintf(filename, "%s/%s", prefix, locales[lang]);
  mkdir(filename, 0777);
  sprintf(filename, "%s/%s/%s.gz", prefix, locales[lang], drv->pc_file_name);

  if ((fp = gzopen(filename, "wb9")) == NULL)
  {
    fprintf(stderr, "genppd: Unable to create file \"%s\" - %s.\n",
            filename, strerror(errno));
    return (2);
  }

  fprintf(stderr, "Writing %s...\n", filename);

  gzputs(fp, "*PPD-Adobe: \"4.3\"\n");
  gzputs(fp, "*%PPD file for ESP Print Pro.\n");
  gzputs(fp, "*%Copyright 1993-2000 by Easy Software Products, All Rights Reserved.\n");
  gzputs(fp, "*%These coded instructions, statements, and computer programs contain\n");
  gzputs(fp, "*%unpublished proprietary information of Easy Software Products, and\n");
  gzputs(fp, "*%are protected by Federal copyright law.  They may not be disclosed\n");
  gzputs(fp, "*%to third parties or copied or duplicated in any form, in whole or\n");
  gzputs(fp, "*%in part, without the prior written consent of Easy Software Products.\n");
  gzputs(fp, "*FormatVersion:	\"4.3\"\n");
  gzprintf(fp, "*FileVersion:	\"%.4f\"\n", CUPS_VERSION);
  gzprintf(fp, "*LanguageVersion: %s\n", langs[lang]);
  gzputs(fp, "*LanguageEncoding: ISOLatin1\n");
  gzprintf(fp, "*PCFileName:	\"%s\"\n", drv->pc_file_name);
  gzputs(fp, "*Manufacturer:	\"ESP\"\n");
  gzprintf(fp, "*Product:	\"(CUPS v%.4f)\"\n", CUPS_VERSION);
  gzprintf(fp, "*ModelName:     \"%s\"\n", drv->model_name);
  gzprintf(fp, "*ShortNickName: \"%s\"\n", drv->model_name);
  gzprintf(fp, "*NickName:      \"%s CUPS v%.4f\"\n", drv->model_name,
           CUPS_VERSION);
  gzputs(fp, "*PSVersion:	\"(3010.000) 550\"\n");
  gzputs(fp, "*LanguageLevel:	\"3\"\n");
  gzprintf(fp, "*ColorDevice:	%s\n", drv->color_device ? "True" : "False");
  gzprintf(fp, "*DefaultColorSpace: %s\n", drv->color_device ? "RGB" : "Gray");
  gzputs(fp, "*FileSystem:	False\n");
  if (drv->throughput)
    gzprintf(fp, "*Throughput:	\"%d\"\n", drv->throughput);
  gzputs(fp, "*LandscapeOrientation: Plus90\n");
  gzprintf(fp, "*VariablePaperSize: %s\n",
           drv->variable_paper_size ? "True" : "False");
  gzputs(fp, "*TTRasterizer:	Type42\n");

  gzputs(fp, "*cupsVersion:	1.1\n");
  gzprintf(fp, "*cupsModelNumber: \"%d\"\n", drv->model_number);
  gzprintf(fp, "*cupsManualCopies: %s\n", drv->manual_copies ? "True" : "False");
  if (drv->duplexing && drv->flipduplex)
    gzprintf(fp, "*cupsFlipDuplex: %s\n", drv->flipduplex ? "True" : "False");
  for (i = 0; i < drv->num_filters; i ++)
    gzprintf(fp, "*cupsFilter:	\"%s\"\n", drv->filters[i]);
  for (i = 0; i < drv->num_profiles; i ++)
    gzprintf(fp, "*cupsColorProfile %s/%s: \"%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\"\n",
	     drv->profiles[i].resolution, drv->profiles[i].media_type,
	     drv->profiles[i].density, drv->profiles[i].gamma,
	     drv->profiles[i].profile[0], drv->profiles[i].profile[1],
	     drv->profiles[i].profile[2], drv->profiles[i].profile[3],
	     drv->profiles[i].profile[4], drv->profiles[i].profile[5],
	     drv->profiles[i].profile[6], drv->profiles[i].profile[7],
	     drv->profiles[i].profile[8]);

  for (i = 0; i < drv->num_constraints; i ++)
    gzprintf(fp, "*UIConstraints: %s\n", drv->constraints[i]);

  gzputs(fp, "*OpenUI *PageSize: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *PageSize\n");
  gzprintf(fp, "*DefaultPageSize: %s\n",
           sizes[drv->sizes[drv->default_size]].name);
  for (i = 0; i < drv->num_sizes; i ++)
  {
    size = sizes + drv->sizes[i];
    gzprintf(fp, "*PageSize %s/%s:\t\"<</PageSize[%d %d]/ImagingBBox null>>setpagedevice\"\n",
             size->name, size->text[lang], size->width, size->length);
  }
  gzputs(fp, "*CloseUI: *PageSize\n");

  gzputs(fp, "*OpenUI *PageRegion: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *PageRegion\n");
  gzprintf(fp, "*DefaultPageRegion: %s\n",
           sizes[drv->sizes[drv->default_size]].name);
  for (i = 0; i < drv->num_sizes; i ++)
  {
    size = sizes + drv->sizes[i];
    gzprintf(fp, "*PageRegion %s/%s:\t\"<</PageSize[%d %d]/ImagingBBox null>>setpagedevice\"\n",
             size->name, size->text[lang], size->width, size->length);
  }
  gzputs(fp, "*CloseUI: *PageRegion\n");

  gzprintf(fp, "*DefaultImageableArea: %s\n",
           sizes[drv->sizes[drv->default_size]].name);
  for (i = 0; i < drv->num_sizes; i ++)
  {
    size = sizes + drv->sizes[i];
    gzprintf(fp, "*ImageableArea %s/%s:\t\"%.2f %.2f %.2f %.2f\"\n",
             size->name, size->text[lang], drv->hw_margins[0], drv->hw_margins[1],
	     size->width - drv->hw_margins[2],
	     size->length - drv->hw_margins[3]);
  }

  gzprintf(fp, "*DefaultPaperDimension: %s\n",
           sizes[drv->sizes[drv->default_size]].name);
  for (i = 0; i < drv->num_sizes; i ++)
  {
    size = sizes + drv->sizes[i];
    gzprintf(fp, "*PaperDimension %s/%s:\t\"%d %d\"\n", size->name,
             size->text[lang], size->width, size->length);
  }

  if (drv->variable_paper_size)
  {
    gzprintf(fp, "*MaxMediaWidth:  \"%d\"\n", drv->max_width);
    gzprintf(fp, "*MaxMediaHeight: \"%d\"\n", drv->max_length);
    gzprintf(fp, "*HWMargins:      %.1f %.1f %.1f %.1f\n",
	     drv->hw_margins[0], drv->hw_margins[1], drv->hw_margins[2],
             drv->hw_margins[3]);
    gzputs(fp, "*CustomPageSize True: \"pop pop pop <</PageSize[5 -2 roll]/ImagingBBox null>>setpagedevice\"\n");
    gzprintf(fp, "*ParamCustomPageSize Width:        1 points %d %d\n",
             drv->min_width, drv->max_width);
    gzprintf(fp, "*ParamCustomPageSize Height:       2 points %d %d\n",
             drv->min_length, drv->max_length);
    gzputs(fp, "*ParamCustomPageSize WidthOffset:  3 points 0 0\n");
    gzputs(fp, "*ParamCustomPageSize HeightOffset: 4 points 0 0\n");
    gzputs(fp, "*ParamCustomPageSize Orientation:  5 int 0 0\n");
  }

  if (drv->num_types)
  {
    gzputs(fp, "*OpenUI *MediaType: PickOne\n");
    gzputs(fp, "*OrderDependency: 10 AnySetup *MediaType\n");
    gzprintf(fp, "*DefaultMediaType: %s\n",
             drv->types[drv->default_type].name);
    for (i = 0; i < drv->num_types; i ++)
      gzprintf(fp, "*MediaType %s/%s:\t\"<</MediaType(%s)/cupsMediaType %d>>setpagedevice\"\n",
               drv->types[i].name, drv->types[i].text[lang],
	       drv->types[i].name, drv->types[i].value);
    gzputs(fp, "*CloseUI: *MediaType\n");
  }

  if (drv->num_slots)
  {
    gzputs(fp, "*OpenUI *InputSlot: PickOne\n");
    gzputs(fp, "*OrderDependency: 10 AnySetup *InputSlot\n");
    gzprintf(fp, "*DefaultInputSlot: %s\n",
             drv->slots[drv->default_slot].name);
    for (i = 0; i < drv->num_slots; i ++)
      gzprintf(fp, "*InputSlot %s/%s:\t\"<</MediaPosition %d>>setpagedevice\"\n",
               drv->slots[i].name, drv->slots[i].text[lang],
	       drv->slots[i].value);
    gzputs(fp, "*CloseUI: *InputSlot\n");
  }

  gzputs(fp, "*OpenUI *Resolution: PickOne\n");
  gzputs(fp, "*OrderDependency: 20 AnySetup *Resolution\n");
  gzprintf(fp, "*DefaultResolution: %s\n",
           drv->resolutions[drv->default_resolution].name);
  for (i = 0; i < drv->num_resolutions; i ++)
  {
    gzprintf(fp, "*Resolution %s/%s:\t\"<</HWResolution[%d %d]/cupsBitsPerColor %d",
             drv->resolutions[i].name, drv->resolutions[i].text[lang],
	     drv->resolutions[i].xdpi, drv->resolutions[i].ydpi,
	     drv->resolutions[i].depth);
    if (drv->resolutions[i].colorspace >= 0)
      gzprintf(fp, "/cupsColorSpace %d", drv->resolutions[i].colorspace);
    if (drv->resolutions[i].colororder >= 0)
      gzprintf(fp, "/cupsColorOrder %d", drv->resolutions[i].colororder);
    if (drv->resolutions[i].row_count > 0)
      gzprintf(fp, "/cupsRowCount %d", drv->resolutions[i].row_count);
    if (drv->resolutions[i].row_step > 0)
      gzprintf(fp, "/cupsRowStep %d", drv->resolutions[i].row_step);
    if (drv->resolutions[i].row_feed > 0)
      gzprintf(fp, "/cupsRowFeed %d", drv->resolutions[i].row_feed);

    gzputs(fp, ">>setpagedevice\"\n");
  }
  gzputs(fp, "*CloseUI: *Resolution\n");

  if (drv->num_colors)
  {
    gzputs(fp, "*OpenUI *ColorModel: PickOne\n");
    gzputs(fp, "*OrderDependency: 10 AnySetup *ColorModel\n");
    gzprintf(fp, "*DefaultColorModel: %s\n",
             drv->colors[drv->default_color].name);
    for (i = 0; i < drv->num_colors; i ++)
    {
      gzprintf(fp, "*ColorModel %s/%s:\t\"<<",
               drv->colors[i].name, drv->colors[i].text[lang]);

      if (drv->colors[i].colororder >= 0)
        gzprintf(fp, "/cupsColorOrder %d", drv->colors[i].colororder);

      if (drv->colors[i].colorspace >= 0)
	gzprintf(fp, "/cupsColorSpace %d", drv->colors[i].colorspace);

      gzprintf(fp, "/cupsCompression %d>>setpagedevice\"\n",
	       drv->colors[i].compression);
    }

    gzputs(fp, "*CloseUI: *ColorModel\n");
  }

  if (drv->duplexing)
  {
    gzprintf(fp, "*OpenUI *Duplex/%s: PickOne\n", duplex[lang]);
    gzputs(fp, "*OrderDependency: 10 AnySetup *Duplex\n");
    gzputs(fp, "*DefaultDuplex: None\n");
    gzprintf(fp, "*Duplex None/%s: \"<</Duplex false/Tumble false>>setpagedevice\"\n",
             duplexes[lang][0]);
    gzprintf(fp, "*Duplex DuplexNoTumble/%s: \"<</Duplex true/Tumble false>>setpagedevice\"\n",
             duplexes[lang][1]);
    gzprintf(fp, "*Duplex DuplexTumble/%s: \"<</Duplex true/Tumble true>>setpagedevice\"\n",
             duplexes[lang][2]);
    gzputs(fp, "*CloseUI: *Duplex\n");
  }

  if (drv->cutting)
  {
    gzprintf(fp, "*OpenUI *CutMedia/%s: Boolean\n", cutmedia[lang]);
    gzputs(fp, "*OrderDependency: 10 AnySetup *Cut\n");
    gzputs(fp, "*DefaultCutMedia: False\n");
    gzputs(fp, "*CutMedia False: \"<</CutMedia 0>>setpagedevice\"\n");
    gzputs(fp, "*CutMedia True: \"<</CutMedia 4>>setpagedevice\"\n");
    gzputs(fp, "*CloseUI: *CutMedia\n");
  }

  if (drv->num_finishings)
  {
    gzputs(fp, "*OpenUI *ESPFinishing/Finishing: PickOne\n");
    gzputs(fp, "*OrderDependency: 10 AnySetup *ESPFinishing\n");
    gzprintf(fp, "*DefaultESPFinishing: %s\n",
             drv->finishings[drv->default_finishing].name);
    for (i = 0; i < drv->num_finishings; i ++)
      gzprintf(fp, "*ESPFinishing %s/%s:\t\"<</OutputType(%s)>>setpagedevice\"\n",
               drv->finishings[i].name, drv->finishings[i].text[lang],
	       drv->finishings[i].name);

    gzputs(fp, "*CloseUI: *ESPFinishing\n");
  }

  if (drv->num_installable)
  {
    gzputs(fp, "*OpenGroup: InstallableOptions\n");

    for (i = 0; i < drv->num_installable; i ++)
    {
      gzprintf(fp, "*OpenUI *%s/%s: Boolean\n", drv->installable[i].name,
               drv->installable[i].text[lang]);
      gzprintf(fp, "*Default%s: False\n", drv->installable[i].name);
      gzprintf(fp, "*%s True/%s: ""\n", drv->installable[i].name,
               installable[lang][0]);
      gzprintf(fp, "*%s False/%s: ""\n", drv->installable[i].name,
               installable[lang][1]);
      gzprintf(fp, "*CloseUI *%s\n", drv->installable[i].name);
    }

    gzputs(fp, "*CloseGroup: InstallableOptions\n");
  }

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

  gzprintf(fp, "*%%End of %s\n", drv->pc_file_name);

  gzclose(fp);

  return (0);
}


/*
 * End of "$Id$".
 */
