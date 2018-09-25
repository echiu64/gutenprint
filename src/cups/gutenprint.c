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

/*
 * 'main()' - Process files on the command-line...
 */

#include "genppd.h"

static int	cat_ppd(const char *uri);
static int	list_ppds(const char *argv0);


int				    /* O - Exit status */
main(int  argc,			    /* I - Number of command-line arguments */
     char *argv[])		    /* I - Command-line arguments */
{
 /*
  * Force POSIX locale, since stp_init incorrectly calls setlocale...
  */

  (void) setenv("LANG", "C", 1);
  (void) setenv("LC_ALL", "C", 1);
  (void) setenv("LC_NUMERIC", "C", 1);

 /*
  * Initialise libgutenprint
  */

  stp_init();

 /*
  * Process command-line...
  */

  if (argc == 2 && !strcmp(argv[1], "list"))
    return (list_ppds(argv[0]));
  else if (argc == 3 && !strcmp(argv[1], "cat"))
    return (cat_ppd(argv[2]));
  else if (argc == 2 && !strcmp(argv[1], "org.gutenprint.multicat"))
    {
      char buf[1024];
      int status = 0;
      while (fgets(buf, sizeof(buf) - 1, stdin))
	{
	  size_t len = strlen(buf);
	  if (len == 0)
	    continue;
	  if (buf[len - 1] == '\n')
	    buf[len - 1] = '\0';
	  status |= cat_ppd(buf);
	  fputs("*%*%EOFEOF\n", stdout);
	  (void) fflush(stdout);
	}
    }
  else if (argc == 2 && !strcmp(argv[1], "VERSION"))
    {
      printf("%s\n", VERSION);
      return (0);
    }
  else if (argc == 2 && !strcasecmp(argv[1], "org.gutenprint.extensions"))
    {
      printf("org.gutenprint.multicat");
      return (0);
    }
  else
    {
      fprintf(stderr, "Usage: %s list\n", argv[0]);
      fprintf(stderr, "       %s cat URI\n", argv[0]);
      return (1);
    }
  return (0);
}


/*
 * 'cat_ppd()' - Copy the named PPD to stdout.
 */

static int				/* O - Exit status */
cat_ppd(const char *uri)	/* I - Driver URI */
{
  char			scheme[64],	/* URI scheme */
			userpass[32],	/* URI user/pass (unused) */
			hostname[32],	/* URI hostname */
			resource[1024];	/* URI resource */
  int			port;		/* URI port (unused) */
  http_uri_status_t	status;		/* URI decode status */
  const stp_printer_t	*p;		/* Printer driver */
  const char		*lang = NULL;
  char			*s;
  char			filename[1024],		/* Filename */
			ppd_location[2048];	/* Installed location */
  const char 		*infix = "";
  ppd_type_t 		ppd_type = PPD_STANDARD;
  gpfile		outFD;

  if ((status = httpSeparateURI(HTTP_URI_CODING_ALL, uri,
                                scheme, sizeof(scheme),
                                userpass, sizeof(userpass),
				hostname, sizeof(hostname),
		                &port, resource, sizeof(resource)))
				    < HTTP_URI_OK)
  {
    fprintf(stderr, "ERROR: Bad ppd-name \"%s\" (%d)!\n", uri, status);
    return (1);
  }

  if (strcmp(scheme, "gutenprint." GUTENPRINT_RELEASE_VERSION) != 0)
    {
      fprintf(stderr, "ERROR: Gutenprint version mismatch!\n");
      return(1);
    }

  s = strchr(resource + 1, '/');
  if (s)
    {
      lang = s + 1;
      *s = '\0';
    }

  if ((p = stp_get_printer_by_driver(hostname)) == NULL)
  {
    fprintf(stderr, "ERROR: Unable to find driver \"%s\"!\n", hostname);
    return (1);
  }

  if (strcmp(resource + 1, "simple") == 0)
    {
      infix = ".sim";
      ppd_type = PPD_SIMPLIFIED;
    }
  else if (strcmp(resource + 1, "nocolor") == 0)
    {
      infix = ".nc";
      ppd_type = PPD_NO_COLOR_OPTS;
    }

  /*
   * This isn't really the right thing to do.  We really shouldn't
   * be embedding filenames in automatically generated PPD files, but
   * if the user ever decides to go back from generated PPD files to
   * static PPD files we'll need to have this for genppdupdate to work.
   */
  snprintf(filename, sizeof(filename) - 1, "stp-%s.%s%s%s",
	   hostname, GUTENPRINT_RELEASE_VERSION, infix, ppdext);
  snprintf(ppd_location, sizeof(ppd_location) - 1, "%s%s%s/ppd/%s%s",
	   cups_modeldir,
	   cups_modeldir[strlen(cups_modeldir) - 1] == '/' ? "" : "/",
	   lang ? lang : "C",
	   filename, gpext);

  outFD.f = stdout;
  return (write_ppd(&outFD, p, lang, ppd_location, ppd_type, filename, 0));
}

/*
 * 'list_ppds()' - List the available drivers.
 */

static int				/* O - Exit status */
list_ppds(const char *argv0)		/* I - Name of program */
{
  const char		*scheme;	/* URI scheme */
  int			i;		/* Looping var */
  const stp_printer_t	*printer;	/* Pointer to printer driver */

  if ((scheme = strrchr(argv0, '/')) != NULL)
    scheme ++;
  else
    scheme = argv0;

  for (i = 0; i < stp_printer_model_count(); i++)
    if ((printer = stp_get_printer_by_index(i)) != NULL)
    {
      const char *device_id;
      if (!strcmp(stp_printer_get_family(printer), "ps") ||
	  !strcmp(stp_printer_get_family(printer), "raw"))
        continue;

      device_id = stp_printer_get_device_id(printer);
      printf("\"%s://%s/expert\" "
             "%s "
	     "\"%s\" "
             "\"%s" CUPS_PPD_NICKNAME_STRING VERSION "\" "
	     "\"%s\"\n",
             scheme, stp_printer_get_driver(printer),
	     "en",
	     stp_printer_get_manufacturer(printer),
	     stp_printer_get_long_name(printer),
	     device_id ? device_id : "");

#ifdef GENERATE_SIMPLIFIED_PPDS
      printf("\"%s://%s/simple\" "
             "%s "
	     "\"%s\" "
             "\"%s" CUPS_PPD_NICKNAME_STRING VERSION " Simplified\" "
	     "\"%s\"\n",
             scheme, stp_printer_get_driver(printer),
	     "en",
	     stp_printer_get_manufacturer(printer),
	     stp_printer_get_long_name(printer),
	     device_id ? device_id : "");
#endif

#ifdef GENERATE_NOCOLOR_PPDS
      printf("\"%s://%s/nocolor\" "
             "%s "
	     "\"%s\" "
             "\"%s" CUPS_PPD_NICKNAME_STRING VERSION " No color options\" "
	     "\"%s\"\n",
             scheme, stp_printer_get_driver(printer),
	     "en",
	     stp_printer_get_manufacturer(printer),
	     stp_printer_get_long_name(printer),
	     device_id ? device_id : "");
#endif
    }

  return (0);
}
