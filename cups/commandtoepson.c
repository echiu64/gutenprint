/*
 * "$Id$"
 *
 *   EPSON ESC/P2 command filter for the CUPS driver development kit.
 *
 *   Copyright 1993-2000 by Easy Software Products.
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
 *   main() - Main entry and command processing.
 */

/*
 * Include necessary headers...
 */

#include <cups/cups.h>
#include <cups/ppd.h>


/*
 * Macros...
 */

#define pwrite(s,n) fwrite((s), 1, (n), stdout)


/*
 * 'main()' - Main entry and processing of driver.
 */

int			/* O - Exit status */
main(int  argc,		/* I - Number of command-line arguments */
     char *argv[])	/* I - Command-line arguments */
{
  FILE	*fp;		/* Command file */
  char	line[1024],	/* Line from file */
	*lineptr;	/* Pointer into line */


 /*
  * Check for valid arguments...
  */

  if (argc < 6 || argc > 7)
  {
   /*
    * We don't have the correct number of arguments; write an error message
    * and return.
    */

    fputs("ERROR: commandtoepson job-id user title copies options [file]\n", stderr);
    return (1);
  }

 /*
  * Open the command file as needed...
  */

  if (argc == 7)
  {
    if ((fp = fopen(argv[6], "r")) == NULL)
    {
      perror("ERROR: Unable to open command file - ");
      return (1);
    }
  }
  else
    fp = stdin;

 /*
  * Enter remote mode...
  */

  pwrite("\033@\033(R\010\000\000REMOTE1", 15);

 /*
  * Read the commands from the file and send the appropriate commands...
  */

  while (fgets(line, sizeof(line), fp) != NULL)
  {
   /*
    * Drop trailing newline...
    */

    lineptr = line + strlen(line) - 1;
    if (*lineptr == '\n')
      *lineptr = '\0';

   /*
    * Skip leading whitespace...
    */

    for (lineptr = line; isspace(*lineptr); lineptr ++);

   /*
    * Skip comments and blank lines...
    */

    if (*lineptr == '#' || !*lineptr)
      continue;

   /*
    * Parse the command...
    */

    if (strncasecmp(lineptr, "Clean", 5) == 0)
    {
     /*
      * Clean heads...
      */

      pwrite("CH\002\000\000\000", 6);
    }
    else if (strncasecmp(lineptr, "PrintAlignmentPage", 18) == 0)
    {
     /*
      * Print alignment page...
      */

      int phase;

      phase = atoi(lineptr + 18);

      pwrite("DT\003\000\000", 5);
      putchar(phase & 255);
      purchar(phase >> 8);
    }
    else if (strncasecmp(lineptr, "PrintSelfTestPrint", 18) == 0)
    {
     /*
      * Print version info and nozzle check...
      */

      pwrite("VI\002\000\000\000", 6);
      pwrite("NC\002\000\000\000", 6);
    }
    else if (strncasecmp(lineptr, "ReportLevels", 12) == 0)
    {
     /*
      * Report ink levels...
      */

      pwrite("IQ\001\000\001", 5);
    }
    else if (strncasecmp(lineptr, "SetAlignment", 12) == 0)
    {
     /*
      * Set head alignment...
      */

      int phase, x;

      if (sscanf(lineptr + 12, "%d%d", &phase, &x) != 2)
      {
        fprintf(stderr, "ERROR: Invalid printer command \"%s\"!\n", lineptr);
        continue;
      }

      pwrite("DA\004\000", 4);
      putchar(0);
      putchar(phase);
      putchar(0);
      putchar(x);
      pwrite("SV\000\000", 4);
    }
    else
      fprintf(stderr, "ERROR: Invalid printer command \"%s\"!\n", lineptr);
  }

 /*
  * Exit remote mode...
  */

  pwrite("\033\000\000\000", 4);

 /*
  * Close the command file and return...
  */

  if (fp != stdin)
    fclose(fp);

  return (0);
}


/*
 * End of "$Id$".
 */
