/*
 *   Dyesub command filter for Gutenprint
 *
 *
 * Contents:
 *
 *   main() - Main entry and command processing.
 */

/*
 * Include necessary headers...
 */

#include <cups/cups.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>


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
  int	feedpage;	/* Feed the page */


 /*
  * Check for valid arguments...
  */

  if (argc < 6 || argc > 7)
  {
   /*
    * We don't have the correct number of arguments; write an error message
    * and return.
    */

    fputs("ERROR: commandtocanon job-id user title copies options [file]\n", stderr);
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

  /* Just pass everything through as is */

  while (fgets(line, sizeof(line), fp) != NULL)
  {
    pwrite(line, strnlen(line, sizeof(line)));
  }

 /*
  * Close the command file and return...
  */

  if (fp != stdin)
    fclose(fp);

  return (0);
}


/*
 */
