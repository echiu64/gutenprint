/*
 *   PPD file generation program for the CUPS drivers.
 *
 *   Copyright 1993-2008 by Mike Sweet and Robert Krawitz.
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Contents:
 *
 *   main()              - Process files on the command-line...
 *   cat_ppd()           - Copy the named PPD to stdout.
 *   generate_ppd()      - Generate a PPD file.
 *   getlangs()          - Get a list of available translations.
 *   help()              - Show detailed help.
 *   is_special_option() - Determine if an option should be grouped.
 *   list_ppds()         - List the available drivers.
 *   print_group_close() - Close a UI group.
 *   print_group_open()  - Open a new UI group.
 *   printlangs()        - Print list of available translations.
 *   printmodels()       - Print a list of available models.
 *   usage()             - Show program usage.
 *   write_ppd()         - Write a PPD file.
 */

#include "genppd.h"

static int	generate_ppd(const char *prefix, int verbose,
		             const stp_printer_t *p, const char *language,
			     ppd_type_t ppd_type, int use_compression);
static int	generate_model_ppds(const char *prefix, int verbose,
				    const stp_printer_t *printer,
				    const char *language, int which_ppds,
				    int use_compression);
static void	help(void);
static void	printlangs(char** langs);
static void	printmodels(int verbose);
static void	usage(void);
static gpFile	gpopen(const char *path, const char *mode, int use_compression);
static int	gpclose(gpFile f, int use_compression);

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
  const stp_printer_t *printer;	    /* Pointer to printer driver */
  int           verbose = 0;        /* Verbose messages */
  char          **langs = NULL;     /* Available translations */
  char          **models = NULL;    /* Models to output, all if NULL */
  int           opt_printlangs = 0; /* Print available translations */
  int           opt_printmodels = 0;/* Print available models */
  int           which_ppds = 2;	    /* Simplified PPD's = 1, full = 2,
				       no color opts = 4 */
  unsigned      parallel = 1;	    /* Generate PPD files in parallel */
  unsigned      rotor = 0;	    /* Rotor for generating PPD files in parallel */
  unsigned      test_rotor = 0;	    /* Testing (serialized) rotor */
  unsigned      test_rotor_circumference = 1;    /* Testing (serialized) rotor size */
  pid_t         *subprocesses = NULL;
  int		parent = 1;
#ifdef HAVE_LIBZ
  int		use_compression = 1;
#else
  int		use_compression = 0;
#endif
  int		skip_duplicate_ppds = 0;


 /*
  * Parse command-line args...
  */

  prefix   = CUPS_MODELDIR;

  for (;;)
  {
    if ((i = getopt(argc, argv, "23hvqc:p:l:LMVd:saNCbZzSr:R:")) == -1)
      break;

    switch (i)
    {
    case '2':
      cups_ppd_ps_level = 2;
      break;
    case '3':
      cups_ppd_ps_level = 3;
      break;
    case 'h':
      help();
      exit(EXIT_SUCCESS);
      break;
    case 'v':
      verbose = 1;
      break;
    case 'q':
      verbose = 0;
      break;
    case 'c':
      fputs("ERROR: -c option no longer supported!\n", stderr);
      break;
    case 'p':
      prefix = optarg;
#  ifdef DEBUG
      fprintf(stderr, "DEBUG: prefix: %s\n", prefix);
#  endif
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
    case 'd':
      cups_modeldir = optarg;
      break;
    case 's':
      which_ppds = 1;
      break;
    case 'a':
      which_ppds = 3;
      break;
    case 'C':
      which_ppds |= 4;
      break;
    case 'N':
      localize_numbers = !localize_numbers;
      break;
    case 'V':
      printf("cups-genppd version %s, "
	     "Copyright 1993-2008 by Michael R Sweet and Robert Krawitz.\n\n",
	     VERSION);
      printf("Default CUPS PPD PostScript Level: %d\n", cups_ppd_ps_level);
      printf("Default PPD location (prefix):     %s\n", CUPS_MODELDIR);
      printf("Default base locale directory:     %s\n\n", PACKAGE_LOCALE_DIR);
      puts("This program is free software; you can redistribute it and/or\n"
	   "modify it under the terms of the GNU General Public License,\n"
	   "version 2, as published by the Free Software Foundation.\n"
	   "\n"
	   "This program is distributed in the hope that it will be useful,\n"
	   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	   "GNU General Public License for more details.\n");
      exit(EXIT_SUCCESS);
      break;
    case 'b':
      use_base_version = 1;
      break;
    case 'z':
#ifdef HAVE_LIBZ
      use_compression = 1;
#endif
      break;
    case 'Z':
#ifdef HAVE_LIBZ
      use_compression = 0;
#endif
      break;
    case 'S':
      skip_duplicate_ppds = 1;
      break;
    case 'r':
      test_rotor = atoi(optarg);
      break;
    case 'R':
      test_rotor_circumference = atoi(optarg);
      break;
    default:
      usage();
      exit(EXIT_FAILURE);
      break;
    }
  }
  if (test_rotor_circumference < 1 || test_rotor >= test_rotor_circumference)
    {
      test_rotor = 0;
      test_rotor_circumference = 1;
    }
#ifdef HAVE_LIBZ
  if (use_compression)
    gpext = ".gz";
  else
#endif
    gpext = "";
  if (optind < argc) {
    int n, numargs;
    numargs = argc-optind;
    models = stp_malloc((numargs+1) * sizeof(char*));
    for (n=0; n<numargs; n++)
      {
	models[n] = argv[optind+n];
      }
    models[numargs] = (char*)NULL;
  }

/*
 * Initialise libgutenprint
 */

  stp_init();

  langs = getlangs();

 /*
  * Print lists
  */

  if (opt_printlangs)
    {
      printlangs(langs);
      exit(EXIT_SUCCESS);
    }

  if (opt_printmodels)
    {
      printmodels(verbose);
      exit(EXIT_SUCCESS);
    }

 /*
  * Write PPD files...
  */

  if (getenv("STP_PARALLEL"))
    {
      parallel = atoi(getenv("STP_PARALLEL"));
      if (parallel < 1 || parallel > 256)
	parallel = 1;
    }
  if (parallel > 1)
    {
      subprocesses = stp_malloc(sizeof(pid_t) * parallel);
      for (rotor = 0; rotor < parallel; rotor++)
	{
	  pid_t pid = fork();
	  if (pid == 0)		/* Child */
	    {
	      parent = 0;
	      break;
	    }
	  else if (pid > 0)
	    subprocesses[rotor] = pid;
	  else
	    {
	      fprintf(stderr, "Cannot fork: %s\n", strerror(errno));
	      return 1;
	    }
	}
    }
  if (models)
    {
      int n;
      for (n=0; models[n]; n++)
	{
	  printer = stp_get_printer_by_driver(models[n]);
	  if (!printer)
	    printer = stp_get_printer_by_long_name(models[n]);

	  if (n % parallel == rotor && printer)
	    {
	      if (printer)
		{
		  if (generate_model_ppds(prefix, verbose, printer, language,
					  which_ppds, use_compression))
		    return 1;
		}
	      else
		{
		  printf("Driver not found: %s\n", models[n]);
		  return (1);
		}
	    }
	}
      stp_free(models);
    }
  else
    {
      int test_rotor_current = -1;
      stp_string_list_t *seen_models = NULL;
      if (skip_duplicate_ppds)
	seen_models = stp_string_list_create();

      for (i = 0; i < stp_printer_model_count(); i++)
	{
	  printer = stp_get_printer_by_index(i);
	  test_rotor_current++;
	  if (skip_duplicate_ppds)
	    {
	      char model_family[128];
	      (void) snprintf(model_family, 127, "%d_%s",
			      stp_printer_get_model(printer),
			      stp_printer_get_family(printer));
	      if (stp_string_list_is_present(seen_models, model_family))
		continue;
	      else
		stp_string_list_add_string_unsafe(seen_models, model_family,
						  model_family);
	    }
	  if (test_rotor_current % test_rotor_circumference != test_rotor)
	    continue;
	  if (i % parallel == rotor && printer)
	    {
	      if (! verbose && (i % 100) == 0)
		fputc('.',stderr);
	      if (generate_model_ppds(prefix, verbose, printer, language,
				      which_ppds, use_compression))
		return 1;
	    }
	}
      if (seen_models)
	stp_string_list_destroy(seen_models);
    }
  if (subprocesses)
    {
      pid_t pid;
      do
	{
	  int status;
	  pid = waitpid(-1, &status, 0);
	  if (pid > 0 && (!WIFEXITED(status) || WEXITSTATUS(status) != 0))
	    {
	      fprintf(stderr, "failed!\n");
	      return 1;
	    }
	} while (pid > 0);
      stp_free(subprocesses);
    }
  if (parent && !verbose)
    fprintf(stderr, " done.\n");

  return (0);
}

static int
generate_model_ppds(const char *prefix, int verbose,
		    const stp_printer_t *printer, const char *language,
		    int which_ppds, int use_compression)
{
  if ((which_ppds & 1) &&
      generate_ppd(prefix, verbose, printer, language, PPD_SIMPLIFIED,
		   use_compression))
    return (1);
  if ((which_ppds & 2) &&
      generate_ppd(prefix, verbose, printer, language, PPD_STANDARD,
		   use_compression))
    return (1);
  if ((which_ppds & 4) &&
      generate_ppd(prefix, verbose, printer, language, PPD_NO_COLOR_OPTS,
		   use_compression))
    return (1);
  return 0;
}

/*
 * 'generate_ppd()' - Generate a PPD file.
 */

static int				/* O - Exit status */
generate_ppd(
    const char          *prefix,	/* I - PPD directory prefix */
    int                 verbose,	/* I - Verbosity level */
    const stp_printer_t *p,		/* I - Driver */
    const char          *language,	/* I - Primary language */
    ppd_type_t          ppd_type,	/* I - full, simplified, no color */
    int			use_compression) /* I - compress output */
{
  int		status;			/* Exit status */
  gpFile	fp;			/* File to write to */
  char		filename[1024],		/* Filename */
		ppd_location[1024];	/* Installed location */
  struct stat   dir;                    /* Prefix dir status */
  const char    *ppd_infix;

 /*
  * Skip the PostScript drivers...
  */

  if (!strcmp(stp_printer_get_family(p), "ps") ||
      !strcmp(stp_printer_get_family(p), "raw"))
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
      exit(EXIT_FAILURE);
    }
  }

 /*
  * The files will be named stp-<driver>.<major>.<minor>.ppd, for
  * example:
  *
  * stp-escp2-ex.5.0.ppd
  *
  * or
  *
  * stp-escp2-ex.5.0.ppd.gz
  */

  switch (ppd_type)
    {
    case PPD_SIMPLIFIED:
      ppd_infix = ".sim";
      break;
    case PPD_NO_COLOR_OPTS:
      ppd_infix = ".nc";
      break;
    default:
      ppd_infix = "";
    }

  snprintf(filename, sizeof(filename) - 1, "%s/stp-%s.%s%s%s%s",
	   prefix, stp_printer_get_driver(p), GUTENPRINT_RELEASE_VERSION,
	   ppd_infix, ppdext, gpext);

 /*
  * Open the PPD file...
  */

  if ((fp = gpopen(filename, "wb", use_compression)) == NULL)
  {
    fprintf(stderr, "cups-genppd: Unable to create file \"%s\" - %s.\n",
            filename, strerror(errno));
    return (2);
  }

  if (verbose)
    fprintf(stderr, "Writing %s...\n", filename);

  snprintf(ppd_location, sizeof(ppd_location), "%s%s%s/%s",
	   cups_modeldir,
	   cups_modeldir[strlen(cups_modeldir) - 1] == '/' ? "" : "/",
	   language ? language : "C",
	   basename(filename));

  snprintf(filename, sizeof(filename) - 1, "stp-%s.%s%s%s",
	   stp_printer_get_driver(p), GUTENPRINT_RELEASE_VERSION,
	   ppd_infix, ppdext);

  status = write_ppd(fp, p, language, ppd_location, ppd_type,
		     basename(filename), use_compression);

  gpclose(fp, use_compression);

  return (status);
}


/*
 * 'help()' - Show detailed help.
 */

void
help(void)
{
  puts("Generate Gutenprint PPD files for use with CUPS\n\n");
  usage();
  puts("\nExamples: LANG=de_DE cups-genppd -p ppd -c /usr/share/locale\n"
       "          cups-genppd -L -c /usr/share/locale\n"
       "          cups-genppd -M -v\n\n"
       "Commands:\n"
       "  -h            Show this help message.\n"
       "  -L            List available translations (message catalogs).\n"
       "  -M            List available printer models.\n"
       "  -V            Show version information and defaults.\n"
       "  The default is to output PPDs.\n");
  puts("Options:\n"
       "  -N            Localize numbers.\n"
       "  -l locale     Output PPDs translated with messages for locale.\n"
       "  -p prefix     Output PPDs in directory prefix.\n"
       "  -d prefix     Embed directory prefix in PPD file.\n"
       "  -s            Generate simplified PPD files.\n"
       "  -a            Generate all (simplified and full) PPD files.\n"
       "  -q            Quiet mode.\n"
       "  -v            Verbose mode.\n");
  puts(
#ifdef HAVE_LIBZ
       "  -z            Compress PPD files.\n"
       "  -Z            Don't compress PPD files.\n"
#endif
       "  -S            Skip PPD files with duplicate model identifiers.\n"
       "  -R size       Generate every size'th PPD file.\n"
       "  -r divisor    Generate the PPD files (N % size == divisor).\n"
       "\n"
       "models:\n"
       "  A list of printer models, either the driver or quoted full name.\n");
}

/*
 * 'usage()' - Show program usage.
 */

void
usage(void)
{
  puts("Usage: cups-genppd "
        "[-l locale] [-p prefix] [-s | -a] [-q] [-v] models...\n"
        "       cups-genppd -L\n"
	"       cups-genppd -M [-v]\n"
	"       cups-genppd -h\n"
	"       cups-genppd -V\n");
}

/*
 * 'printlangs()' - Print list of available translations.
 */

void
printlangs(char **langs)		/* I - Languages */
{
  if (langs)
    {
      int n = 0;
      while (langs && langs[n])
	{
	  puts(langs[n]);
	  n++;
	}
    }
  exit(EXIT_SUCCESS);
}


/*
 * 'printmodels()' - Print a list of available models.
 */

void
printmodels(int verbose)		/* I - Verbosity level */
{
  const stp_printer_t *p;
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
  exit(EXIT_SUCCESS);
}

static gpFile
gpopen(const char *path, const char *mode, int use_compression)
{
#ifdef HAVE_LIBZ
  gpFile f = stp_malloc(sizeof(gpfile));
  if (use_compression)
    {
      f->gzf = gzopen(path, mode);
      if (!f->gzf)
	{
	  stp_free(f);
	  return NULL;
	}
      return f;
    }
  else
#endif
    {
      FILE *fl = fopen(path, mode);
#ifdef HAVE_LIBZ
      if (fl)
	{
	  f->f = fl;
	  return f;
	}
      else
	{
	  stp_free(f);
	  return NULL;
	}
#else
      return fl;
#endif
    }
}

static int
gpclose(gpFile f, int use_compression)
{
  int status;
#ifdef HAVE_LIBZ
  if (use_compression)
    status = gzclose(f->gzf);
  else
    status = fclose(f->f);
  stp_free(f);
#else
  status = fclose(f);
#endif
  return status;
}
