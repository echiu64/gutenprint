/*
 * "$Id$"
 *
 *   Printer maintenance utility for Epson Stylus printers
 *
 *   Copyright 2000 Robert Krawitz (rlk@alum.mit.edu)
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
 */

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>

struct option optlist[] =
{
  { "printer-name",	1,	NULL,	(int) 'P' },
  { "raw-device",	1,	NULL,	(int) 'r' },
  { "ink-level",	0,	NULL,	(int) 'i' },
  { "clean-head",	0,	NULL,	(int) 'c' },
  { "nozzle-check",	0,	NULL,	(int) 'n' },
  { "align-head",	0,	NULL,	(int) 'a' },
  { "usb",		0,	NULL,	(int) 'u' },
  { "help",		0,	NULL,	(int) 'h' },
  { "new-series",	0,	NULL,	(int) 'l' },
  { "old-series",	0,	NULL,	(int) 'o' },
  { "identify",		0,	NULL,	(int) 'd' },
  { NULL,		0,	NULL,	0 	  }
};

char *printer = NULL;
char *raw_device = NULL;
char printer_cmd[1025];
int bufpos = 0;
int isUSB = 0;
int isNew = 0;

char *banner = "\
Copyright 2000 Robert Krawitz (rlk@alum.mit.edu)\n\
\n\
This program is free software; you can redistribute it and/or modify it\n\
under the terms of the GNU General Public License as published by the Free\n\
Software Foundation; either version 2 of the License, or (at your option)\n\
any later version.\n\
\n\
This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\n\
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License\n\
for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n";


char *help_msg = "\
Usage: escputil [-P printer | -r device] [-u] [-c | -n | -a | -i]\n\
    -P|--printer-name  Specify the name of the printer to operate on.\n\
                       Default is the default system printer.\n\
    -r|--raw-device    Specify the name of the device to write to directly\n\
                       rather than going through a printer queue.\n\
    -c|--clean-head    Clean the print head.\n\
    -n|--nozzle-check  Print a nozzle test pattern.\n\
                       Dirty or clogged nozzles will show as gaps in the\n\
                       pattern.  If you see any gaps, you should run a\n\
                       head cleaning pass.\n\
    -a|--align-head    Align the print head.  CAUTION: Misuse of this\n\
                       utility may result in poor print quality and/or\n\
                       damage to the printer.\n\
    -i|--ink-level     Obtain the ink level from the printer.  This requires\n\
                       read/write access to the raw printer device.\n\
    -l|--new-series    For newer ESCP/2 printers (Epson Stylus Color 440\n\
                       and newer; Epson Stylus Photo 750 and newer).\n\
    -o|--old-series    For older ESCP/2 printers.\n\
    -u|--usb           The printer is connected via USB.\n\
    -h|--help          Print this help message.\n";

void initialize_print_cmd(void);
void do_head_clean(void);
void do_nozzle_check(void);
void do_align(void);
void do_ink_level(void);
void do_identify(void);

void
do_help(int code)
{
  printf("%s", help_msg);
  exit(code);
}

int
main(int argc, char **argv)
{
  int operation = 0;
  int c;
  printf("%s\n", banner);
  while (1)
    {
      int option_index = 0;
      c = getopt_long(argc, argv, "P:r:icnaduol", optlist, &option_index);
      if (c == -1)
	break;
      switch (c)
	{
	case 'l':
	  isNew = 1;
	  break;
	case 'o':
	  isNew = 0;
	  break;
	case 'c':
	case 'i':
	case 'n':
	case 'a':
	case 'd':
	  if (operation)
	    do_help(1);
	  operation = c;
	  break;
	case 'P':
	  if (printer || raw_device)
	    {
	      printf("You may only specify one printer or raw device.\n");
	      do_help(1);
	    }
	  printer = malloc(strlen(optarg) + 1);
	  strcpy(printer, optarg);
	  break;
	case 'r':
	  if (printer || raw_device)
	    {
	      printf("You may only specify one printer or raw device.\n");
	      do_help(1);
	    }
	  raw_device = malloc(strlen(optarg) + 1);
	  strcpy(raw_device, optarg);
	  break;
	case 'u':
	  isUSB = 1;
	  break;
	case 'h':
	  do_help(0);
	  break;
	default:
	  fprintf(stderr, "Unknown option %c\n", c);
	  do_help(1);
	}
    }
  if (operation == 0)
    do_help(1);
  initialize_print_cmd();
  switch(operation)
    {
    case 'c':
      do_head_clean();
      break;
    case 'n':
      do_nozzle_check();
      break;
    case 'i':
      do_ink_level();
      break;
    case 'a':
      do_align();
      break;
    case 'd':
      do_identify();
      break;
    default:
      do_help(1);
    }
  exit(0);
}

int
do_print_cmd(void)
{
  FILE *pfile;
  int bytes = 0;
  int retries = 0;
  char *command;
  memcpy(printer_cmd + bufpos, "\f\033\000\033\000", 5);
  bufpos += 5;
  if (raw_device)
    {
      pfile = fopen(raw_device, "rb");
      if (!pfile)
	{
	  fprintf(stderr, "Cannot open device %s: %s\n", raw_device,
		  strerror(errno));
	  return 1;
	}
    }
  else
    {
#if defined(LPR_COMMAND)
      if (printer == NULL)
	{
	  command = malloc(strlen(LPR_COMMAND) + 32);
	  sprintf(command, "%s -l", LPR_COMMAND);
	}
      else
	{
	  command = malloc(strlen(LPR_COMMAND) + strlen(printer) + 32);
	  sprintf(command, "%s -P%s -l", LPR_COMMAND, printer);
	}
#elif defined(LP_COMMAND)
      if (printer == NULL)
	{
	  command = malloc(strlen(LP_COMMAND) + 32);
	  sprintf(command, "%s -s", LPR_COMMAND);
	}
      else
	{
	  command = malloc(strlen(LP_COMMAND) + 32);
	  sprintf(command, "%s -s -d%s", LPR_COMMAND, printer);
	}
#endif
      if ((pfile = popen(command, "w")) == NULL)
	{
	  fprintf(stderr, "Cannot print to printer %s with %s\n", printer,
		  command);
	  return 1;
	}
    }
  while (bytes < bufpos)
    {
      int status = fwrite(printer_cmd + bytes, 1, bufpos - bytes, pfile);
      if (status == 0)
	{
	  retries++;
	  if (retries > 2)
	    {
	      fprintf(stderr, "Unable to send command to printer\n");
	      if (raw_device)
		fclose(pfile);
	      else
		pclose(pfile);
	      return 1;
	    }
	}
      else if (status == -1)
	{
	  fprintf(stderr, "Unable to send command to printer\n");
	  if (raw_device)
	    fclose(pfile);
	  else
	    pclose(pfile);
	  return 1;
	}
      else
	{
	  bytes += status;
	  retries = 0;
	}
    }
  if (raw_device)
    fclose(pfile);
  else
    pclose(pfile);
  return 0;
}

void
initialize_print_cmd(void)
{
  bufpos = 0;
  if (isUSB)
    {
      static char hdr[] = "\000\000\000\033\001@EJL 1284.4\n@EJL     \n\033@";
      memcpy(printer_cmd, hdr, sizeof(hdr) - 1); /* Do NOT include the null! */
      bufpos = sizeof(hdr) - 1;
    }
}

void
do_remote_cmd(char *cmd, int nargs, int a0, int a1, int a2, int a3)
{
  static char remote_hdr[] = "\033@\033(R\010\000\000REMOTE1";
  static char remote_trailer[] = "\033\000\000\000\033\000";
  memcpy(printer_cmd + bufpos, remote_hdr, sizeof(remote_hdr) - 1);
  bufpos += sizeof(remote_hdr) - 1;
  memcpy(printer_cmd + bufpos, cmd, 2);
  bufpos += 2;
  printer_cmd[bufpos] = nargs % 256;
  printer_cmd[bufpos + 1] = (nargs >> 8) % 256;
  if (nargs > 0)
    printer_cmd[bufpos + 2] = a0;
  if (nargs > 1)
    printer_cmd[bufpos + 3] = a1;
  if (nargs > 2)
    printer_cmd[bufpos + 4] = a2;
  if (nargs > 3)
    printer_cmd[bufpos + 5] = a3;
  bufpos += 2 + nargs;
  memcpy(printer_cmd + bufpos, remote_trailer, sizeof(remote_trailer) - 1);
  bufpos += sizeof(remote_trailer) - 1;
}

void
add_newlines(int count)
{
  int i;
  for (i = 0; i < count; i++)
    {
      printer_cmd[bufpos++] = '\r';
      printer_cmd[bufpos++] = '\n';
    }
}

void
add_resets(int count)
{
  int i;
  for (i = 0; i < count; i++)
    {
      printer_cmd[bufpos++] = '\033';
      printer_cmd[bufpos++] = '\000';
    }
}

char *colors[] = {
  "Black", "Cyan", "Magenta", "Yellow", "Light Cyan", "Light Magenta", 0
};

void
do_ink_level(void)
{
  int fd;
  int status;
  char buf[1024];
  char *ind;
  int i;
  if (!raw_device)
    {
      fprintf(stderr, "Obtaining ink levels requires using a raw device.\n");
      exit(1);
    }
  fd = open(raw_device, O_RDWR, 0666);
  if (fd == -1)
    {
      fprintf(stderr, "Cannot open %s read/write: %s\n", raw_device,
	      strerror(errno));
      exit(1);
    }
  do_remote_cmd("IQ", 1, 1, 0, 0, 0);
  add_resets(2);
  if (write(fd, printer_cmd, bufpos) < bufpos)
    {
      fprintf(stderr, "Cannot write to %s: %s\n", raw_device, strerror(errno));
      exit(1);
    }
  sleep(1);
  memset(buf, 0, 1024);
  status = read(fd, buf, 1023);
  if (status < 0)
    {
      fprintf(stderr, "Cannot read from %s: %s\n", raw_device,strerror(errno));
      exit(1);
    }
  ind = index(buf, 'I');
  if (!ind || ind[1] != 'Q' || ind[2] != ':')
    {
      fprintf(stderr, "Cannot parse output from printer\n");
      exit(1);
    }
  ind += 3;
  printf("%20s    %s\n", "Ink color", "Percent remaining");
  for (i = 0; i < 6; i++)
    {
      int val, j;
      if (!ind[0] || ind[0] == ';')
	exit(0);
      for (j = 0; j < 2; j++)
	{
	  if (ind[j] >= '0' && ind[j] <= '9')
	    ind[j] -= '0';
	  else if (ind[j] >= 'A' && ind[j] <= 'F')
	    ind[j] = ind[j] - 'A' + 10;
	  else if (ind[j] >= 'a' && ind[j] <= 'f')
	    ind[j] = ind[j] - 'a' + 10;
	  else
	    exit(1);
	}
      val = (ind[0] << 4) + ind[1];
      printf("%20s    %3d\n", colors[i], val);
      ind += 2;
    }
  (void) close(fd);
  exit(0);
}

void
do_identify(void)
{
  int fd;
  int status;
  char buf[1024];
  if (!raw_device)
    {
      fprintf(stderr, "Printer identification requires using a raw device.\n");
      exit(1);
    }
  fd = open(raw_device, O_RDWR, 0666);
  if (fd == -1)
    {
      fprintf(stderr, "Cannot open %s read/write: %s\n", raw_device,
	      strerror(errno));
      exit(1);
    }
  bufpos = 0;
  sprintf(printer_cmd, "\033\001@EJL ID\r\n");
  if (write(fd, printer_cmd, strlen(printer_cmd)) < strlen(printer_cmd))
    {
      fprintf(stderr, "Cannot write to %s: %s\n", raw_device, strerror(errno));
      exit(1);
    }
  sleep(1);
  memset(buf, 0, 1024);
  status = read(fd, buf, 1023);
  if (status < 0)
    {
      fprintf(stderr, "Cannot read from %s: %s\n", raw_device,strerror(errno));
      exit(1);
    }
  printf("%s\n", buf);
  (void) close(fd);
  exit(0);
}


void
do_head_clean(void)
{
  do_remote_cmd("CH", 2, 0, 0, 0, 0);
  printf("Cleaning heads...\n");
  exit(do_print_cmd());
}

void
do_nozzle_check(void)
{
  do_remote_cmd("VI", 2, 0, 0, 0, 0);
  do_remote_cmd("NC", 2, 0, 0, 0, 0);
  printf("Running nozzle check, please ensure paper is in the printer.\n");
  exit(do_print_cmd());
}

char new_align_help[] = "\
Please read these instructions very carefully before proceeding.\n\
\n\
This utility lets you align the print head of your Epson Stylus inkjet\n\
printer.  Misuse of this utility may cause your print quality to degrade\n\
and possibly damage your printer.  This utility has not been reviewed by\n\
Seiko Epson for correctness, and is offered with no warranty at all.  The\n\
entire risk of using this utility lies with you.\n\
\n\
This utility prints three test patterns.  Each pattern looks very similar.\n\
The patterns consist of a series of pairs of vertical lines that overlap.\n\
Below each pair of lines is a number between 1 and 15.\n\
\n\
When you inspect the pairs of lines, you should find the pair of lines that\n\
is best in alignment, that is, that best forms a single vertical align.\n\
Inspect the pairs very carefully to find the best match.  Using a loupe\n\
or magnifying glass is recommended for the most critical inspection.\n\
After picking the number matching the best pair, place the paper back in\n\
the paper input tray before typing it in.\n\
\n\
The second and third patterns are similar, but use finer dots for more\n\
critical alignment.  You must run all three passes to correctly align your\n\
printer.  After running all three alignment passes, all three alignment\n\
patterns will be printed once more.  You should find that the middle-most\n\
pair (#8 out of the 15) is the best for all three patterns.\n\
\n\
After the three passes are printed once more, you will be offered the\n\
choices of (s)aving the result in the printer, (r)epeating the process,\n\
or (q)uitting without saving.  Quitting will not restore the previous\n\
settings, but powering the printer off and back on will.  If you quit,\n\
you must repeat the entire process if you wish to later save the results.\n\
It is essential that you not turn your printer off during this procedure.";

char old_align_help[] = "\
Please read these instructions very carefully before proceeding.\n\
\n\
This utility lets you align the print head of your Epson Stylus inkjet\n\
printer.  Misuse of this utility may cause your print quality to degrade\n\
and possibly damage your printer.  This utility has not been reviewed by\n\
Seiko Epson for correctness, and is offered with no warranty at all.  The\n\
entire risk of using this utility lies with you.\n\
\n\
This utility prints a test pattern that consist of a series of pairs of\n\
vertical lines that overlap.  Below each pair of lines is a number between\n\
1 and 7.\n\
\n\
When you inspect the pairs of lines, you should find the pair of lines that\n\
is best in alignment, that is, that best forms a single vertical align.\n\
Inspect the pairs very carefully to find the best match.  Using a loupe\n\
or magnifying glass is recommended for the most critical inspection.\n\
After picking the number matching the best pair, place the paper back in\n\
the paper input tray before typing it in.\n\
\n\
After running the alignment pattern, it will be printed once more.  You\n\
should find that the middle-most pair (#4 out of the 7) is the best.\n\
You will then be offered the choices of (s)aving the result in the printer,\n\
(r)epeating the process, or (q)uitting without saving.  Quitting will not\n\
restore the previous settings, but powering the printer off and back on will.\n\
If you quit, you must repeat the entire process if you wish to later save\n\
the results.  It is essential that you not turn off your printer during\n\
this procedure.";

void
do_align_help(void)
{
  if (isNew)
    printf("%s\n", new_align_help);
  else
    printf("%s\n", old_align_help);
  fflush(stdout);
}

void
align_error(void)
{
  printf("Unable to send command to the printer, exiting.\n");
  exit(1);
}

/*
 * This is the thorny one.
 */
void
do_align(void)
{
  char inbuf[64];
  long answer;
  char *endptr;
 start:
  do_align_help();
  if (isNew)
    {
      printf("This procedure requires that your printer be an Epson Stylus Color 440\n");
      printf("or newer.  If you have an older printer, please type control-C now\n");
      printf("and run 'escputil -a -o'\n");
    }
  else
    {
      printf("This procedure requires that your printer be older than the\n");
      printf("Epson Stylus Color 440.  If you have a newer printer, please\n");
      printf("type control-C now and run 'escputil -a -o'\n");
    }
  printf("Please place a sheet of paper in your printer to begin the head\n");
  printf("alignment procedure.\n");
  memset(inbuf, 0, 64);
  fflush(stdin);
  fgets(inbuf, 63, stdin);
  putc('\n', stdout);
 one:
  {
    printf("Starting alignment phase 1.  Please insert a fresh sheet of paper.\n");
    fflush(stdout);
    initialize_print_cmd();
    do_remote_cmd("DT", 3, 0, 0, 0, 0);
    if (do_print_cmd())
      align_error();
  reread1:
    printf("Please inspect the print, and choose the best pair of lines\n");
    printf("in pattern #1, and then reinsert the page in the input tray.\n");
    printf("Type a pair number, '?' for help, or 'r' to retry this pattern. ==> ");
    fflush(stdout);
    memset(inbuf, 0, 64);
    fflush(stdin);
    fgets(inbuf, 63, stdin);
    putc('\n', stdout);
    switch (inbuf[0])
      {
      case 'r':
	goto one;
	break;
      case 'h':
      case '?':
	do_align_help();
	fflush(stdout);
      case '\n':
      case '\000':
	goto reread1;
	break;
      default:
	break;
      }
    answer = strtol(inbuf, &endptr, 10);
    if (endptr == inbuf)
      {
	printf("I cannot understand what you typed!\n");
	fflush(stdout);
	goto reread1;
      }
    if (answer < 1 || answer > 15 || (answer > 7 && isNew == 0))
      {
	printf("The best pair of lines should be numbered between 1 and %d.\n",
	       isNew ? 15 : 7);
	fflush(stdout);
	goto reread1;
      }
    if (isNew)
      printf("Aligning phase 1, and starting phase 2.\n");
    else
      {
	printf("Aligning phase 1, and performing final test.\n");
	printf("Please insert a fresh sheet of paper.\n");
      }
    fflush(stdout);
    initialize_print_cmd();
    do_remote_cmd("DA", 4, 0, 0, 0, answer);
    if (!isNew)
      goto final;
  }
 two:
  {
    add_newlines(7);
    do_remote_cmd("DT", 3, 0, 1, 0, 0);
    if (do_print_cmd())
      align_error();
  reread2:
    printf("Please inspect the print, and choose the best pair of lines\n");
    printf("in pattern #2, and then reinsert the page in the input tray.\n");
    printf("Type a pair number, '?' for help, or 'r' to retry this pattern. ==> ");
    fflush(stdout);
    memset(inbuf, 0, 64);
    fflush(stdin);
    fgets(inbuf, 63, stdin);
    putc('\n', stdout);
    switch (inbuf[0])
      {
      case 'r':
      case 'R':
	printf("Please insert a fresh sheet of paper, and then type the enter key.\n");
	initialize_print_cmd();
	fflush(stdin);
	fgets(inbuf, 15, stdin);
	putc('\n', stdout);
	goto two;
	break;
      case 'h':
      case 'H':
      case '?':
	do_align_help();
	fflush(stdout);
      case '\n':
      case '\000':
	goto reread2;
	break;
      default:
	break;
      }
    answer = strtol(inbuf, &endptr, 10);
    if (endptr == inbuf)
      {
	printf("I cannot understand what you typed!\n");
	fflush(stdout);
	goto reread2;
      }
    if (answer < 1 || answer > 15)
      {
	printf("The best pair of lines should be numbered between 1 and 15.\n");
	fflush(stdout);
	goto reread2;
      }
    printf("Aligning phase 2, and starting phase 3.\n");
    fflush(stdout);
    initialize_print_cmd();
    do_remote_cmd("DA", 4, 0, 1, 0, answer);
  }
 three:
  {
    add_newlines(14);
    do_remote_cmd("DT", 3, 0, 2, 0, 0);
    if (do_print_cmd())
      align_error();
  reread3:
    printf("Please inspect the print, and choose the best pair of lines\n");
    printf("in pattern #3, and then insert a fresh page in the input tray\n");
    printf("for the final alignment test.\n");
    printf("Type a pair number, '?' for help, or 'r' to retry this pattern. ==> ");
    fflush(stdout);
    memset(inbuf, 0, 64);
    fflush(stdin);
    fgets(inbuf, 63, stdin);
    putc('\n', stdout);
    switch (inbuf[0])
      {
      case 'r':
      case 'R':
	printf("Please insert a fresh sheet of paper, and then type the enter key.\n");
	fflush(stdout);
	initialize_print_cmd();
	fflush(stdin);
	fgets(inbuf, 15, stdin);
	putc('\n', stdout);
	goto three;
	break;
      case 'h':
      case 'H':
      case '?':
	do_align_help();
	fflush(stdout);
      case '\n':
      case '\000':
	goto reread3;
	break;
      default:
	break;
      }
    answer = strtol(inbuf, &endptr, 10);
    if (endptr == inbuf)
      {
	printf("I cannot understand what you typed!\n");
	fflush(stdout);
	goto reread3;
      }
    if (answer < 1 || answer > 15)
      {
	printf("The best pair of lines should be numbered between 1 and 15.\n");
	fflush(stdout);
	goto reread3;
      }
    printf("Aligning phase 3, and performing final test.\n");
    printf("Please insert a fresh sheet of paper, and then type the enter key.\n");
    fflush(stdout);
    initialize_print_cmd();
    do_remote_cmd("DA", 4, 0, 1, 0, answer);
  }
 final:
  do_remote_cmd("DT", 3, 0, 0, 0, 0);
  if (isNew)
    {
      do_remote_cmd("DT", 3, 0, 1, 0, 0);
      do_remote_cmd("DT", 3, 0, 2, 0, 0);
    }
  if (do_print_cmd())
    align_error();
 read_final:
  printf("Please inspect the final output very carefully to ensure that your\n");
  printf("printer is in proper alignment.  You may now (s)ave the results in\n");
  printf("the printer, (q)uit without saving the results, or (r)epeat the entire\n");
  printf("process from the beginning.  You will then be asked to confirm your choice\n");
  printf("What do you want to do (s, q, r)? ");
  fflush(stdout);
  memset(inbuf, 0, 64);
  fflush(stdin);
  fgets(inbuf, 15, stdin);
  putc('\n', stdout);
  switch (inbuf[0])
    {
    case 'q':
    case 'Q':
      printf("Please confirm by typing 'q' again that you wish to quit without saving: ");
      fflush(stdout);
      memset(inbuf, 0, 64);
      fflush(stdin);
      putc('\n', stdout);
      fgets(inbuf, 15, stdin);
      if (inbuf[0] == 'q' || inbuf[0] == 'Q')
	{
	  printf("OK, your printer is aligned, but the alignment has not been saved.\n");
	  printf("If you wish to save the alignment, you must repeat this process.\n");
	  exit(0);
	}
      break;
    case 'r':
    case 'R':
      printf("Please confirm by typing 'r' again that you wish to repeat the\n");
      printf("alignment process: ");
      fflush(stdout);
      memset(inbuf, 0, 64);
      fflush(stdin);
      putc('\n', stdout);
      fgets(inbuf, 15, stdin);
      if (inbuf[0] == 'r' || inbuf[0] == 'R')
	{
	  printf("Repeating the alignment process.\n");
	  goto start;
	}
      break;
    case 's':
    case 'S':
      printf("Please confirm by typing 's' again that you wish to save the settings\n");
      printf("to your printer.  This will permanently alter the configuration of\n");
      printf("your printer.  WARNING: this procedure has not been approved by\n");
      printf("Seiko Epson, and it may damage your printer.  Proceed? ");
	     
      fflush(stdout);
      memset(inbuf, 0, 64);
      fflush(stdin);
      putc('\n', stdout);
      fgets(inbuf, 15, stdin);
      if (inbuf[0] == 's' || inbuf[0] == 'S')
	{
	  printf("Please insert your alignment test page in the printer once more\n");
	  printf("for the final save of your alignment settings.  When the printer\n");
	  printf("feeds the page through, your settings have been saved.\n");
	  fflush(stdout);
	  initialize_print_cmd();
	  add_newlines(2);
	  do_remote_cmd("SV", 0, 0, 0, 0, 0);
	  add_newlines(2);
	  if (do_print_cmd())
	    align_error();
	  exit(0);
	}
      break;
    default:
      printf("Unrecognized command.\n");
      goto read_final;
      break;
    }
  printf("Final command was not confirmed.\n");
  goto read_final;
}
