/*
 * "$Id$"
 *
 *   PPD file generation program for the CUPS drivers.
 *
 *   Copyright 1993-2003 by Easy Software Products.
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
 *   main()                   - Process files on the command-line...
 *   initialize_stp_options() - Initialize the min/max values for
 *                              each STP numeric option.
 *   usage()                  - Show program usage.
 *   help()                   - Show detailed program usage.
 *   getlangs()               - Get available translations.
 *   printlangs()             - Show available translations.
 *   printmodels()            - Show available printer models.
 *   checkcat()               - Check message catalogue exists.
 *   xmalloc()                - Die gracefully if malloc fails.
 *   write_ppd()              - Write a PPD file.
 */

/*
 * Include necessary headers...
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include <cups/cups.h>
#include <cups/raster.h>

#ifdef INCLUDE_GIMP_PRINT_H
#include INCLUDE_GIMP_PRINT_H
#else
#include <gimp-print/gimp-print.h>
#endif
#include <gimp-print/gimp-print-intl.h>
#include "../../lib/libprintut.h"

/*
 * Note:
 *
 * The current release of ESP Ghostscript is fully Level 3 compliant,
 * so we can report Level 3 support by default...
 */

#ifndef CUPS_PPD_PS_LEVEL
#  define CUPS_PPD_PS_LEVEL 3
#endif
#include "gimp-print-cups.h"


/*
 * File handling stuff...
 */

#ifdef HAVE_LIBZ
static const char *ppdext = ".ppd.gz";
#else
static const char *ppdext = ".ppd";
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
#define CATALOG       "LC_MESSAGES/gimp-print.mo"

typedef struct				/**** Media size values ****/
{
  const char	*name,			/* Media size name */
		*text;			/* Media size text */
  int		width,			/* Media width */
		height,			/* Media height */
		left,			/* Media left margin */
		right,			/* Media right margin */
		bottom,			/* Media bottom margin */
		top;			/* Media top margin */
} paper_t;

const char *special_options[] =
{
  "PageSize",
  "MediaType",
  "InputSlot",
  "Resolution",
  NULL
};

const char *parameter_class_names[] =
{
  N_("Printer_Features"),
  N_("Output_Control")
};

const char *parameter_level_names[] =
{
  N_("Common"),
  N_("Extra_1"),
  N_("Extra_2"),
  N_("Extra_3"),
  N_("Extra_4"),
  N_("Extra_5")
};


/*
 * Local functions...
 */

void	usage(void);
void    help(void);
char ** getlangs(void);
int     checkcat (const struct dirent *localedir);
void    printlangs(char** langs);
void    printmodels(int verbose);
void *  xmalloc (size_t size);
int	write_ppd(const stp_printer_t p, const char *prefix,
	          int verbose);


/*
 * Global variables...
 */
const char *baselocaledir = PACKAGE_LOCALE_DIR;

static int
is_special_option(const char *name)
{
  int i = 0;
  while (special_options[i])
    {
      if (strcmp(name, special_options[i]) == 0)
	return 1;
      i++;
    }
  return 0;
}

static void
print_group_open(FILE *fp, stp_parameter_class_t p_class,
		 stp_parameter_level_t p_level)
{
  gzprintf(fp, "*OpenGroup: %s_%s_%s\n\n", _("Gimp-Print"),
	   _(parameter_level_names[p_level]),
	   _(parameter_class_names[p_class]));
}

static void
print_group_close(FILE *fp, stp_parameter_class_t p_class,
		 stp_parameter_level_t p_level)
{
  gzprintf(fp, "*CloseGroup: %s_%s_%s\n\n", _("Gimp-Print"),
	   _(parameter_level_names[p_level]),
	   _(parameter_class_names[p_class]));
}

/*
 * 'main()' - Process files on the command-line...
 */

int				    /* O - Exit status */
main(int  argc,			    /* I - Number of command-line arguments */
     char *argv[])		    /* I - Command-line arguments */
{
  int		i;		    /* Looping var */
  const char	*prefix;	    /* Directory prefix for output */
  const char	*language = NULL;   /* Language */
  stp_printer_t	printer;	    /* Pointer to printer driver */
  int           verbose = 0;        /* Verbose messages */
  char          **langs = NULL;     /* Available translations */
  char          **models = NULL;    /* Models to output, all if NULL */
  int           opt_printlangs = 0; /* Print available translations */
  int           opt_printmodels = 0;/* Print available models */

 /*
  * Parse command-line args...
  */

  prefix   = GENPPD_PPD_PREFIX;

  for (;;)
  {
    if ((i = getopt(argc, argv, "hvqc:p:l:LMV")) == -1)
      break;

    switch (i)
    {
    case 'h':
      help();
      break;
    case 'v':
      verbose = 1;
      break;
    case 'q':
      verbose = 0;
      break;
    case 'c':
      baselocaledir = optarg;
#ifdef DEBUG
      fprintf (stderr, "DEBUG: baselocaledir: %s\n", baselocaledir);
#endif
      break;
    case 'p':
      prefix = optarg;
#ifdef DEBUG
      fprintf (stderr, "DEBUG: prefix: %s\n", prefix);
#endif
      break;
    case 'l':
      language = optarg;
      break;
    case 'L':
      opt_printlangs = 1;
      break;
    case 'M':
      opt_printmodels = 1;
      break;
    case 'V':
      printf("cups-genppd version %s, "
	     "Copyright (c) 1993-2001 by Easy Software Products.\n\n",
	     VERSION);
      printf("CUPS PPD PostScript Level:     %d\n", CUPS_PPD_PS_LEVEL);
      printf("Default PPD location (prefix): %s\n", GENPPD_PPD_PREFIX);
      printf("Default base locale directory: %s\n\n", PACKAGE_LOCALE_DIR);
      puts("This program is free software; you can redistribute it and/or\n"
	   "modify it under the terms of the GNU General Public License,\n"
	   "version 2, as published by the Free Software Foundation.\n"
	   "\n"
	   "This program is distributed in the hope that it will be useful,\n"
	   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	   "GNU General Public License for more details.\n"
	   "\n"
	   "You should have received a copy of the GNU General Public License\n"
	   "along with this program; if not, please contact Easy Software\n"
	   "Products at:\n"
	   "\n"
	   "    Attn: CUPS Licensing Information\n"
	   "    Easy Software Products\n"
	   "    44141 Airport View Drive, Suite 204\n"
	   "    Hollywood, Maryland 20636-3111 USA\n"
	   "\n"
	   "    Voice: (301) 373-9603\n"
	   "    EMail: cups-info@cups.org\n"
	   "      WWW: http://www.cups.org\n");
      exit (EXIT_SUCCESS);
      break;
    default:
      usage();
      exit (EXIT_FAILURE);
      break;
    }
  }
  if (optind < argc) {
    int n, numargs;
    numargs = argc-optind;
    models = xmalloc((numargs+1) * sizeof(char*));
    for (n=0; n<numargs; n++)
      {
	models[n] = argv[optind+n];
      }
    models[numargs] = (char*) NULL;

    n=0;
  }

#ifdef ENABLE_NLS
  langs = getlangs();
#endif

/*
 * Initialise libgimpprint
 */

  stp_init();
  initialize_stp_options();

 /*
  * Set the language...
  */

  if (language)
    {
      unsetenv("LC_CTYPE");
      unsetenv("LC_COLLATE");
      unsetenv("LC_TIME");
      unsetenv("LC_NUMERIC");
      unsetenv("LC_MONETARY");
      unsetenv("LC_MESSAGES");
      unsetenv("LC_ALL");
      unsetenv("LANG");
      setenv("LC_ALL", language, 1);
      setenv("LANG", language, 1);
    }
  setlocale(LC_ALL, "");

#ifdef LC_NUMERIC
  setlocale(LC_NUMERIC, "C");
#endif /* LC_NUMERIC */

 /*
  * Set up the catalog
  */

#ifdef ENABLE_NLS
  if (baselocaledir)
  {
    if ((bindtextdomain(PACKAGE, baselocaledir)) == NULL)
    {
      fprintf(stderr, "cups-genppd: cannot load message catalog %s under %s: %s\n",
	      PACKAGE, baselocaledir, strerror(errno));
      exit(EXIT_FAILURE);
    }
#ifdef DEBUG
    fprintf (stderr, "DEBUG: bound textdomain: %s under %s\n",
	     PACKAGE, baselocaledir);
#endif
    if ((textdomain(PACKAGE)) == NULL)
    {
      fprintf(stderr, "cups-genppd: cannot select message catalog %s under %s: %s\n",
              PACKAGE, baselocaledir, strerror(errno));
      exit(EXIT_FAILURE);
    }
#ifdef DEBUG
    fprintf (stderr, "DEBUG: textdomain set: %s\n", PACKAGE);
#endif
  }
#endif

  /*
   * Print lists
   */

  if (opt_printlangs)
    {
      printlangs(langs);
      exit (EXIT_SUCCESS);
    }

  if (opt_printmodels)
    {
      printmodels(verbose);
      exit (EXIT_SUCCESS);
    }

 /*
  * Write PPD files...
  */

  if (models)
    {
      int n;
      for (n=0; models[n]; n++)
	{
	  printer = stp_get_printer_by_driver(models[n]);
	  if (!printer)
	    printer = stp_get_printer_by_long_name(models[n]);

	  if (printer)
	    {
	      if (write_ppd(printer, prefix, verbose))
		return 1;
	    }
	  else
	    {
	      printf("Driver not found: %s\n", models[n]);
	      return (1);
	    }
	}
      free(models);
    }
  else
    {
      for (i = 0; i < stp_printer_model_count(); i++)
	{
	  printer = stp_get_printer_by_index(i);

	  if (printer && write_ppd(printer, prefix, verbose))
	    return (1);
	}
    }
  if (!verbose)
    fprintf(stderr, " done.\n");
  if (langs)
    {
      char **langs_tmp = langs;
      while (*langs_tmp)
	{
	  free(*langs_tmp);
	  langs_tmp++;
	}
      free(langs);
    }

  return (0);
}

/*
 * 'usage()' - Show program usage...
 */

void
usage(void)
{
  puts("Usage: cups-genppd [-c localedir] "
        "[-l locale] [-p prefix] [-q] [-v] models...\n"
        "       cups-genppd -L [-c localedir]\n"
	"       cups-genppd -M [-v]\n"
	"       cups-genppd -h\n"
	"       cups-genppd -V\n");
}

void
help(void)
{
  puts("Generate gimp-print PPD files for use with CUPS\n\n");
  usage();
  puts("\nExamples: LANG=de_DE cups-genppd -p ppd -c /usr/share/locale\n"
       "          cups-genppd -L -c /usr/share/locale\n"
       "          cups-genppd -M -v\n\n"
       "Commands:\n"
       "  -h            Show this help message.\n"
       "  -L            List available translations (message catalogs).\n"
       "  -M            List available printer models.\n"
       "  -V            Show version information and defaults.\n"
       "  The default is to output PPDs.\n"
       "Options:\n"
       "  -c localedir  Use localedir as the base directory for locale data.\n"
       "  -l locale     Output PPDs translated with messages for locale.\n"
       "  -p prefix     Output PPDs in directory prefix.\n"
       "  -q            Quiet mode.\n"
       "  -v            Verbose mode.\n"
       "models:\n"
       "  A list of printer models, either the driver or quoted full name.\n");
}


/*
 * 'getlangs()' - Get a list of available translations
 */

char **
getlangs(void)
{
  struct dirent** langdirs;
  int n;
  char **langs;

  n = scandir (baselocaledir, &langdirs, checkcat, alphasort);
  if (n >= 0)
    {
      int idx;
      langs = xmalloc((n+1) * sizeof(char*));
      for (idx = 0; idx < n; ++idx)
	{
	  langs[idx] = (char*) xmalloc((strlen(langdirs[idx]->d_name)+1) * sizeof(char));
	  strcpy(langs[idx], langdirs[idx]->d_name);
	  free (langdirs[idx]);
	}
      langs[n] = NULL;
      free (langdirs);
    }
  else
    return NULL;

  return langs;
}


/*
 * 'printlangs()' - Print list of available translations
 */

void printlangs(char **langs)
{
  if (langs)
    {
      int n = 0;
      while (langs && langs[n])
	{
	  printf("%s\n", langs[n]);
	  n++;
	}
    }
  exit (EXIT_SUCCESS);
}


/*
 * 'printmodels' - Print a list of available models
 */

void printmodels(int verbose)
{
  stp_printer_t p;
  int i;

  for (i = 0; i < stp_printer_model_count(); i++)
    {
      p = stp_get_printer_by_index(i);
      if (p &&
	  strcmp(stp_printer_get_family(p), "ps") != 0 &&
	  strcmp(stp_printer_get_family(p), "raw") != 0)
	{
	  if(verbose)
	    printf("%-20s%s\n", stp_printer_get_driver(p),
		   stp_printer_get_long_name(p));
	  else
	    printf("%s\n", stp_printer_get_driver(p));
	}
    }
  exit (EXIT_SUCCESS);
}


/*
 * 'checkcat()' - A callback for scandir() to check
 *                if a message catalogue exists
 */

int
checkcat (const struct dirent *localedir)
{
  char* catpath;
  int catlen, status = 0, savederr;
  struct stat catstat;

  savederr = errno; /* since we are a callback, preserve scandir() state */

  /* LOCALEDIR / LANG / LC_MESSAGES/CATALOG */
  /* Add 3, for two '/' separators and '\0'   */
  catlen = strlen(baselocaledir) + strlen(localedir->d_name) + strlen(CATALOG) + 3;
  catpath = (char*) xmalloc(catlen * sizeof(char));

  strncpy (catpath, baselocaledir, strlen(baselocaledir));
  catlen = strlen(baselocaledir);
  *(catpath+catlen) = '/';
  catlen++;
  strncpy (catpath+catlen, localedir->d_name, strlen(localedir->d_name));
  catlen += strlen(localedir->d_name);
  *(catpath+catlen) = '/';
  catlen++;
  strncpy (catpath+catlen, CATALOG, strlen(CATALOG));
  catlen += strlen(CATALOG);
  *(catpath+catlen) = '\0';

  if (!stat (catpath, &catstat))
    {
      if (S_ISREG(catstat.st_mode))
	{
	  status = 1;
	}
     }

  free (catpath);

  errno = savederr;
  return status;
}


/*
 * 'xmalloc() - die gracefully if malloc() fails
 */

void *
xmalloc (size_t size)
{
  register void *p = NULL;

  if ((p = malloc (size)) == NULL)
    {
      fprintf (stderr, "cups-genppd: Memory allocation failed: %s.\n",
	       strerror(errno));
      exit (EXIT_FAILURE);
    }
  return (p);
}


/*
 * 'write_ppd()' - Write a PPD file.
 */

int					/* O - Exit status */
write_ppd(const stp_printer_t p,	/* I - Printer driver */
	  const char          *prefix,	/* I - Prefix (directory) for PPD files */
	  int                 verbose)
{
  int		i, j, k, l;		/* Looping vars */
  gzFile	fp;			/* File to write to */
  char		filename[1024];		/* Filename */
  char		pcfilename[13],		/* PCFileName attribute */
		*pcptr;			/* Pointer into PCFileName */
  const char	*driverptr;		/* Pointer into driver name */
  char		manufacturer[64];	/* Manufacturer name */
  int		num_opts;		/* Number of printer options */
  int		xdpi, ydpi;		/* Resolution info */
  stp_vars_t	v;			/* Variable info */
  int		width, height,		/* Page information */
		bottom, left,
		top, right;
  const char	*driver;		/* Driver name */
  const char	*long_name;		/* Driver long name */
  stp_vars_t	printvars;		/* Printer option names */
  paper_t	*the_papers;		/* Media sizes */
  int		cur_opt;		/* Current option */
  struct stat   dir;                    /* prefix dir status */
  int		variable_sizes;		/* Does the driver support variable sizes? */
  int		min_width,		/* Min/max custom size */
		min_height,
		max_width,
		max_height;
  stp_parameter_t desc;
  stp_parameter_list_t param_list;
  const stp_param_string_t *opt;

 /*
  * Initialize driver-specific variables...
  */

  driver     = stp_printer_get_driver(p);
  long_name  = stp_printer_get_long_name(p);
  printvars  = stp_printer_get_defaults(p);
  the_papers = NULL;
  cur_opt    = 0;

 /*
  * Skip the PostScript drivers...
  */

  if (strcmp(stp_printer_get_family(p), "ps") == 0 ||
      strcmp(stp_printer_get_family(p), "raw") == 0)
    return (0);

 /*
  * Make sure the destination directory exists...
  */


  if (stat(prefix, &dir) && !S_ISDIR(dir.st_mode))
    {
      if (mkdir(prefix, 0777))
	{
	  printf("cups-genppd: Cannot create directory %s: %s\n",
		 prefix, strerror(errno));
	  exit (EXIT_FAILURE);
	}
    }
  snprintf(filename, sizeof(filename) - 1, "%s/%s%s", prefix, driver, ppdext);

 /*
  * Open the PPD file...
  */

  if ((fp = gzopen(filename, "wb")) == NULL)
  {
    fprintf(stderr, "cups-genppd: Unable to create file \"%s\" - %s.\n",
            filename, strerror(errno));
    return (2);
  }

 /*
  * Write a standard header...
  */

  sscanf(long_name, "%63s", manufacturer);

  if (verbose)
    fprintf(stderr, "Writing %s...\n", filename);
  else
    fprintf(stderr, ".");

  gzputs(fp, "*PPD-Adobe: \"4.3\"\n");
  gzputs(fp, "*%PPD file for CUPS/GIMP-print.\n");
  gzputs(fp, "*%Copyright 1993-2003 by Easy Software Products, All Rights Reserved.\n");
  gzputs(fp, "*%This PPD file may be freely used and distributed under the terms of\n");
  gzputs(fp, "*%the GNU GPL.\n");
  gzputs(fp, "*FormatVersion:	\"4.3\"\n");
  gzputs(fp, "*FileVersion:	\"" VERSION "\"\n");
  /* Specify language of PPD translation */
  /* Translators: Specify the language of the PPD translation.
   * Use the English name of your language here, e.g. "Swedish" instead of
   * "Svenska".
   */
  gzprintf(fp, "*LanguageVersion: %s\n", _("English"));
  /* Specify PPD translation encoding e.g. ISOLatin1 */
  gzprintf(fp, "*LanguageEncoding: %s\n", _("ISOLatin1"));

 /*
  * Strictly speaking, the PCFileName attribute should be a 12 character
  * max (12345678.ppd) filename, as a requirement of the old PPD spec.
  * The following code generates a (hopefully unique) 8.3 filename from
  * the driver name, and makes the filename all UPPERCASE as well...
  */

  for (driverptr = driver, pcptr = pcfilename;
       pcptr < (pcfilename + 8) && *driverptr;)
  {
    *pcptr++ = toupper(*driverptr);

    if (strchr(driverptr, '-') != NULL)
      driverptr = strchr(driverptr, '-') + 1;
    else
      driverptr ++;
  }

  strcpy(pcptr, ".PPD");
    
  gzprintf(fp, "*PCFileName:	\"%s\"\n", pcfilename);

 /*
  * The Manufacturer, for now, is the first word of the long driver
  * name.
  */

  gzprintf(fp, "*Manufacturer:	\"%s\"\n", manufacturer);

 /*
  * The Product attribute specifies the string returned by the PostScript
  * interpreter.  The last one will appear in the CUPS "product" field,
  * while all instances are available as attributes.
  */

  gzputs(fp, "*Product:	\"(AFPL Ghostscript)\"\n");
  gzputs(fp, "*Product:	\"(GNU Ghostscript)\"\n");
  gzputs(fp, "*Product:	\"(ESP Ghostscript)\"\n");

 /*
  * The ModelName attribute now provides the long name rather than the
  * short driver name...  The rastertoprinter driver looks up both...
  */

  gzprintf(fp, "*ModelName:     \"%s\"\n", long_name);
  gzprintf(fp, "*ShortNickName: \"%s\"\n", long_name);

 /*
  * The Windows driver download stuff has problems with NickName fields
  * with commas.  Now use a dash instead...
  */

  gzprintf(fp, "*NickName:      \"%s - CUPS+Gimp-Print v" VERSION "\"\n",
           long_name);
#if CUPS_PPD_PS_LEVEL == 2
  gzputs(fp, "*PSVersion:	\"(2017.000) 550\"\n");
#else
  gzputs(fp, "*PSVersion:	\"(3010.000) 705\"\n");
#endif /* CUPS_PPD_PS_LEVEL == 2 */
  gzprintf(fp, "*LanguageLevel:	\"%d\"\n", CUPS_PPD_PS_LEVEL);
  gzprintf(fp, "*ColorDevice:	%s\n",
           stp_get_output_type(printvars) == OUTPUT_COLOR ? "True" : "False");
  gzprintf(fp, "*DefaultColorSpace: %s\n",
           stp_get_output_type(printvars) == OUTPUT_COLOR ? "RGB" : "Gray");
  gzputs(fp, "*FileSystem:	False\n");
  gzputs(fp, "*LandscapeOrientation: Plus90\n");
  gzputs(fp, "*TTRasterizer:	Type42\n");

  gzputs(fp, "*cupsVersion:	1.1\n");
  gzprintf(fp, "*cupsModelNumber: \"0\"\n");
  gzputs(fp, "*cupsManualCopies: True\n");
  gzputs(fp, "*cupsFilter:	\"application/vnd.cups-raster 100 rastertoprinter\"\n");
  if (strcasecmp(manufacturer, "EPSON") == 0)
    gzputs(fp, "*cupsFilter:	\"application/vnd.cups-command 33 commandtoepson\"\n");
  gzputs(fp, "\n");

 /*
  * Get the page sizes from the driver...
  */

  v = stp_vars_create_copy(printvars);
  variable_sizes = 0;
  stp_describe_parameter(v, "PageSize", &desc);
  num_opts = stp_string_list_count(desc.bounds.str);
  the_papers = xmalloc(sizeof(paper_t) * num_opts);

  for (i = 0; i < num_opts; i++)
  {
    const stp_papersize_t *papersize;
    opt = stp_string_list_param(desc.bounds.str, i);
    papersize = stp_get_papersize_by_name(opt->name);

    if (!papersize)
    {
      printf("Unable to lookup size %s!\n", opt->name);
      continue;
    }

    if (strcmp(opt->name, "Custom") == 0)
    {
      variable_sizes = 1;
      continue;
    }

    width  = papersize->width;
    height = papersize->height;

    if (width <= 0 || height <= 0)
      continue;

    stp_set_string_parameter(v, "PageSize", opt->name);

    stp_get_media_size(v, &width, &height);
    stp_get_imageable_area(v, &left, &right, &bottom, &top);

    the_papers[cur_opt].name   = opt->name;
    the_papers[cur_opt].text   = opt->text;
    the_papers[cur_opt].width  = width;
    the_papers[cur_opt].height = height;
    the_papers[cur_opt].left   = left;
    the_papers[cur_opt].right  = right;
    the_papers[cur_opt].bottom = height - bottom;
    the_papers[cur_opt].top    = height - top;

    cur_opt++;
  }

 /*
  * The VariablePaperSize attribute is obsolete, however some popular
  * applications still look for it to provide custom page size support.
  */

  gzprintf(fp, "*VariablePaperSize: %s\n\n", variable_sizes ? "true" : "false");

  gzputs(fp, "*OpenUI *PageSize: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *PageSize\n");
  gzprintf(fp, "*DefaultPageSize: %s\n", desc.deflt.str);
  for (i = 0; i < cur_opt; i ++)
  {
    gzprintf(fp,  "*PageSize %s", the_papers[i].name);
    gzprintf(fp, "/%s:\t\"<</PageSize[%d %d]/ImagingBBox null>>setpagedevice\"\n",
             the_papers[i].text, the_papers[i].width, the_papers[i].height);
  }
  gzputs(fp, "*CloseUI: *PageSize\n\n");

  gzputs(fp, "*OpenUI *PageRegion: PickOne\n");
  gzputs(fp, "*OrderDependency: 10 AnySetup *PageRegion\n");
  gzprintf(fp, "*DefaultPageRegion: %s\n", desc.deflt.str);
  for (i = 0; i < cur_opt; i ++)
  {
    gzprintf(fp,  "*PageRegion %s", the_papers[i].name);
    gzprintf(fp, "/%s:\t\"<</PageSize[%d %d]/ImagingBBox null>>setpagedevice\"\n",
	     the_papers[i].text, the_papers[i].width, the_papers[i].height);
  }
  gzputs(fp, "*CloseUI: *PageRegion\n\n");

  gzprintf(fp, "*DefaultImageableArea: %s\n", desc.deflt.str);
  for (i = 0; i < cur_opt; i ++)
  {
    gzprintf(fp,  "*ImageableArea %s", the_papers[i].name);
    gzprintf(fp, "/%s:\t\"%d %d %d %d\"\n", the_papers[i].text,
             the_papers[i].left, the_papers[i].bottom,
	     the_papers[i].right, the_papers[i].top);
  }
  gzputs(fp, "\n");

  gzprintf(fp, "*DefaultPaperDimension: %s\n", desc.deflt.str);

  for (i = 0; i < cur_opt; i ++)
  {
    gzprintf(fp, "*PaperDimension %s", the_papers[i].name);
    gzprintf(fp, "/%s:\t\"%d %d\"\n",
	     the_papers[i].text, the_papers[i].width, the_papers[i].height);
  }
  gzputs(fp, "\n");

  if (variable_sizes)
  {
    stp_get_size_limit(v, &max_width, &max_height,
			       &min_width, &min_height);
    stp_set_string_parameter(v, "PageSize", "Custom");
    stp_get_media_size(v, &width, &height);
    stp_get_imageable_area(v, &left, &right, &bottom, &top);

    gzprintf(fp, "*MaxMediaWidth:  \"%d\"\n", max_width);
    gzprintf(fp, "*MaxMediaHeight: \"%d\"\n", max_height);
    gzprintf(fp, "*HWMargins:      %d %d %d %d\n",
	     left, height - bottom, width - right, top);
    gzputs(fp, "*CustomPageSize True: \"pop pop pop <</PageSize[5 -2 roll]/ImagingBBox null>>setpagedevice\"\n");
    gzprintf(fp, "*ParamCustomPageSize Width:        1 points %d %d\n",
             min_width, max_width);
    gzprintf(fp, "*ParamCustomPageSize Height:       2 points %d %d\n",
             min_height, max_height);
    gzputs(fp, "*ParamCustomPageSize WidthOffset:  3 points 0 0\n");
    gzputs(fp, "*ParamCustomPageSize HeightOffset: 4 points 0 0\n");
    gzputs(fp, "*ParamCustomPageSize Orientation:  5 int 0 0\n\n");
  }

  stp_parameter_description_free(&desc);
  if (the_papers)
    free(the_papers);

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
  gzprintf(fp, "*ColorModel Black/Black & White:\t\"<<"
               "/cupsColorSpace %d"
	       "/cupsColorOrder %d"
	       "/cupsBitsPerColor 8>>setpagedevice\"\n",
           CUPS_CSPACE_K, CUPS_ORDER_CHUNKED);

  if (stp_get_output_type(printvars) == OUTPUT_COLOR)
  {
    gzprintf(fp, "*ColorModel RGB/Color:\t\"<<"
                 "/cupsColorSpace %d"
		 "/cupsColorOrder %d"
		 "/cupsBitsPerColor 8>>setpagedevice\"\n",
             CUPS_CSPACE_RGB, CUPS_ORDER_CHUNKED);
    gzprintf(fp, "*ColorModel CMYK/Raw CMYK:\t\"<<"
                 "/cupsColorSpace %d"
		 "/cupsColorOrder %d"
		 "/cupsBitsPerColor 8>>setpagedevice\"\n",
             CUPS_CSPACE_CMYK, CUPS_ORDER_CHUNKED);
  }

  gzputs(fp, "*CloseUI: *ColorModel\n\n");

 /*
  * Media types...
  */

  stp_describe_parameter(v, "MediaType", &desc);
  num_opts = stp_string_list_count(desc.bounds.str);

  if (num_opts > 0)
  {
    gzprintf(fp, "*OpenUI *MediaType/%s: PickOne\n", _("Media Type"));
    gzputs(fp, "*OrderDependency: 10 AnySetup *MediaType\n");
    gzprintf(fp, "*DefaultMediaType: %s\n", desc.deflt.str);

    for (i = 0; i < num_opts; i ++)
    {
      opt = stp_string_list_param(desc.bounds.str, i);
      gzprintf(fp, "*MediaType %s/%s:\t\"<</MediaType(%s)>>setpagedevice\"\n",
               opt->name, opt->text, opt->name);
    }

    gzputs(fp, "*CloseUI: *MediaType\n\n");
  }
  stp_parameter_description_free(&desc);

 /*
  * Input slots...
  */

  stp_describe_parameter(v, "InputSlot", &desc);
  num_opts = stp_string_list_count(desc.bounds.str);

  if (num_opts > 0)
  {
    gzprintf(fp, "*OpenUI *InputSlot/%s: PickOne\n", _("Media Source"));
    gzputs(fp, "*OrderDependency: 10 AnySetup *InputSlot\n");
    gzprintf(fp, "*DefaultInputSlot: %s\n", desc.deflt.str);

    for (i = 0; i < num_opts; i ++)
    {
      opt = stp_string_list_param(desc.bounds.str, i);
      gzprintf(fp, "*InputSlot %s/%s:\t\"<</MediaClass(%s)>>setpagedevice\"\n",
               opt->name, opt->text, opt->name);
    }

    gzputs(fp, "*CloseUI: *InputSlot\n\n");
  }
  stp_parameter_description_free(&desc);

 /*
  * Resolutions...
  */

  stp_describe_parameter(v, "Resolution", &desc);
  num_opts = stp_string_list_count(desc.bounds.str);

  gzprintf(fp, "*OpenUI *Resolution/%s: PickOne\n", _("Resolution"));
  gzputs(fp, "*OrderDependency: 20 AnySetup *Resolution\n");
  gzprintf(fp, "*DefaultResolution: %s\n", desc.deflt.str);

  for (i = 0; i < num_opts; i ++)
  {
   /*
    * Strip resolution name to its essentials...
    */
    opt = stp_string_list_param(desc.bounds.str, i);
    stp_set_string_parameter(v, "Resolution", opt->name);
    stp_describe_resolution(v, &xdpi, &ydpi);

    /* This should not happen! */
    if (xdpi == -1 || ydpi == -1)
      continue;

   /*
    * Write the resolution option...
    */

    gzprintf(fp, "*Resolution %s/%s:\t\"<</HWResolution[%d %d]/cupsCompression %d>>setpagedevice\"\n",
             opt->name, opt->text, xdpi, ydpi, i);
  }

  stp_parameter_description_free(&desc);

  gzputs(fp, "*CloseUI: *Resolution\n\n");

  param_list = stp_get_parameter_list(v);

  for (j = 0; j <= STP_PARAMETER_CLASS_OUTPUT; j++)
    {
      for (k = 0; k <= STP_PARAMETER_LEVEL_ADVANCED4; k++)
	{
	  int printed_open_group = 0;
	  size_t param_count = stp_parameter_list_count(param_list);
	  for (l = 0; l < param_count; l++)
	    {
	      const stp_parameter_t *lparam =
		stp_parameter_list_param(param_list, l);
	      if (lparam->p_class != j || lparam->p_level != k ||
		  is_special_option(lparam->name) ||
		  (lparam->p_type != STP_PARAMETER_TYPE_STRING_LIST &&
		   lparam->p_type != STP_PARAMETER_TYPE_BOOLEAN &&
		   lparam->p_type != STP_PARAMETER_TYPE_DOUBLE))
		  continue;
	      if (!printed_open_group)
		{
		  print_group_open(fp, j, k);
		  printed_open_group = 1;
		}
	      stp_describe_parameter(v, lparam->name, &desc);
	      gzprintf(fp, "*OpenUI *Stp%s/%s: PickOne\n",
		       desc.name, _(desc.text));
#if 0
	      gzprintf(fp, "*OrderDependency: %d AnySetup *Stp%s\n",
		       (100 + l + (j * param_count) +
			(k * STP_PARAMETER_LEVEL_ADVANCED5 * param_count)),
		       desc.name);
#endif
	      if (!desc.is_mandatory)
		{
		  gzprintf(fp, "*DefaultStp%s: DEFAULT\n", desc.name);
		  gzprintf(fp, "*Stp%s %s/%s: \"\"\n", desc.name, "DEFAULT",
			   _("Default"));
		}
	      switch (desc.p_type)
		{
		case STP_PARAMETER_TYPE_STRING_LIST:
		  if (desc.is_mandatory)
		    gzprintf(fp, "*DefaultStp%s: %s\n",
			     desc.name, desc.deflt.str);
		  num_opts = stp_string_list_count(desc.bounds.str);
		  for (i = 0; i < num_opts; i++)
		    {
		      opt = stp_string_list_param(desc.bounds.str, i);
		      gzprintf(fp, "*Stp%s %s/%s: \"\"\n",
			       desc.name, opt->name, _(opt->text));
		    }
		  break;
		case STP_PARAMETER_TYPE_BOOLEAN:
		  if (desc.is_mandatory)
		    gzprintf(fp, "*DefaultStp%s: %s\n",
			     desc.name, desc.deflt.boolean ? "True" : "False");
		  gzprintf(fp, "*Stp%s %s/%s: \"\"\n",
			   desc.name, "False", _("No"));
		  gzprintf(fp, "*Stp%s %s/%s: \"\"\n",
			   desc.name, "True", _("Yes"));
		  break;
		case STP_PARAMETER_TYPE_DOUBLE:
		  if (desc.is_mandatory)
		    gzprintf(fp, "*DefaultStp%s: %d\n", desc.name,
			     (int) (desc.deflt.dbl * 1000));
		  for (i = desc.bounds.dbl.lower * 1000;
		       i <= desc.bounds.dbl.upper * 1000 ; i += 50)
		    gzprintf(fp, "*Stp%s %d/%.3f: \"\"\n",
			     desc.name, i, ((double) i) * .001);
		  break;
		default:
		  break;
		}
	      gzprintf(fp, "*CloseUI: *Stp%s\n\n", desc.name);
	      stp_parameter_description_free(&desc);
	    }
	  if (printed_open_group)
	    print_group_close(fp, j, k);
	}
    }

 /*
  * Fonts...
  */

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

  gzprintf(fp, "\n*%%End of %s.ppd\n", driver);

  gzclose(fp);

  stp_vars_free(v);
  return (0);
}


/*
 * End of "$Id$".
 */
