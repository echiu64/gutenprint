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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "../../lib/libprintut.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#if defined(HAVE_VARARGS_H) && !defined(HAVE_STDARG_H)
#include <varargs.h>
#else
#include <stdarg.h>
#endif
#ifdef HAVE_POLL
#include <sys/poll.h>
#endif
#ifdef __GNU_LIBRARY__
#include <getopt.h>
#endif
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif

void do_align(void);
void do_align_color(void);
char *do_get_input (const char *prompt);
void do_head_clean(void);
void do_help(int code);
void do_identify(void);
void do_ink_level(void);
void do_nozzle_check(void);
void do_status(void);
int do_print_cmd(void);


const char *banner = "\
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


#ifdef __GNU_LIBRARY__

struct option optlist[] =
{
  { "printer-name",	1,	NULL,	(int) 'P' },
  { "raw-device",	1,	NULL,	(int) 'r' },
  { "ink-level",	0,	NULL,	(int) 'i' },
  { "clean-head",	0,	NULL,	(int) 'c' },
  { "nozzle-check",	0,	NULL,	(int) 'n' },
  { "align-head",	0,	NULL,	(int) 'a' },
  { "align-color",	0,	NULL,	(int) 'o' },
  { "status",           0,      NULL,   (int) 's' },
  { "usb",		0,	NULL,	(int) 'u' },
  { "help",		0,	NULL,	(int) 'h' },
  { "identify",		0,	NULL,	(int) 'd' },
  { "model",		1,	NULL,	(int) 'm' },
  { "quiet",		0,	NULL,	(int) 'q' },
  { NULL,		0,	NULL,	0 	  }
};

const char *help_msg = "\
Usage: escputil [-c | -n | -a | -i | -o | -s | -d]\n\
                [-P printer | -r device] [-u] [-q] [-m model]\n\
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
    -o|--align-color   Align the color print head (Stylus Color 480 and 580\n\
                       only).  CAUTION: Misuse of this utility may result in\n\
                       poor print quality and/or damage to the printer.\n\
    -s|--status        Retrieve printer status.\n\
    -i|--ink-level     Obtain the ink level from the printer.  This requires\n\
                       read/write access to the raw printer device.\n\
    -d|--identify      Query the printer for make and model information.\n\
                       This requires read/write access to the raw printer\n\
                       device.\n\
    -u|--usb           The printer is connected via USB.\n\
    -h|--help          Print this help message.\n\
    -q|--quiet         Suppress the banner.\n\
    -m|--model         Specify the precise printer model for head alignment.\n";
#else
const char *help_msg = "\
Usage: escputil [-c | -n | -a | -i | -o | -s | -d]\n\
                [-P printer | -r device] [-u] [-q] [-m model]\n\
    -P Specify the name of the printer to operate on.\n\
          Default is the default system printer.\n\
    -r Specify the name of the device to write to directly\n\
          rather than going through a printer queue.\n\
    -c Clean the print head.\n\
    -n Print a nozzle test pattern.\n\
          Dirty or clogged nozzles will show as gaps in the\n\
          pattern.  If you see any gaps, you should run a\n\
          head cleaning pass.\n\
    -a Align the print head.  CAUTION: Misuse of this\n\
          utility may result in poor print quality and/or\n\
          damage to the printer.\n\
    -o Align the color print head (Stylus Color 480 and 580\n\
          only).  CAUTION: Misuse of this utility may result in\n\
          poor print quality and/or damage to the printer.\n\
    -s Retrieve printer status.\n\
    -i Obtain the ink level from the printer.  This requires\n\
          read/write access to the raw printer device.\n\
    -d Query the printer for make and model information.  This\n\
          requires read/write access to the raw printer device.\n\
    -u The printer is connected via USB.\n\
    -h Print this help message.\n\
    -q Suppress the banner.\n\
    -m Specify the precise printer model for head alignment.\n";
#endif

typedef struct
{
  const char *short_name;
  const char *long_name;
  int passes;
  int choices;
  int ink_change;
  int color_passes;
  int color_choices;
} stp_printer_t;

stp_printer_t printer_list[] =
{
  { "C20sx",	"Stylus C20sx",		3,	15,	0,	2,	9 },
  { "C20ux",	"Stylus C20ux",		3,	15,	0,	2,	9 },
  { "C40sx",	"Stylus C40sx",		3,	15,	0,	2,	9 },
  { "C40ux",	"Stylus C40ux",		3,	15,	0,	2,	9 },
  { "color",	"Stylus Color",		1,	7,	0,	0,	0 },
  { "pro",	"Stylus Color Pro",	1,	7,	0,	0,	0 },
  { "pro-xl",	"Stylus Color Pro XL",	1,	7,	0,	0,	0 },
  { "400",	"Stylus Color 400",	1,	7,	0,	0,	0 },
  { "440",	"Stylus Color 440",	1,	15,	0,	0,	0 },
  { "460",	"Stylus Color 460",	1,	15,	0,	0,	0 },
  { "480",	"Stylus Color 480",	3,	15,	1,	2,	9 },
  { "500",	"Stylus Color 500",	1,	7,	0,	0,	0 },
  { "580",	"Stylus Color 580",	3,	15,	1,	2,	9 },
  { "600",	"Stylus Color 600",	1,	7,	0,	0,	0 },
  { "640",	"Stylus Color 640",	1,	15,	0,	0,	0 },
  { "660",	"Stylus Color 660",	1,	15,	0,	0,	0 },
  { "670",	"Stylus Color 670",	3,	15,	0,	0,	0 },
  { "680",	"Stylus Color 680",	3,	15,	0,	0,	0 },
  { "740",	"Stylus Color 740",	3,	15,	0,	0,	0 },
  { "760",	"Stylus Color 760",	3,	15,	0,	0,	0 },
  { "777",	"Stylus Color 777",	3,	15,	0,	0,	0 },
  { "800",	"Stylus Color 800",	1,	7,	0,	0,	0 },
  { "850",	"Stylus Color 850",	1,	7,	0,	0,	0 },
  { "860",	"Stylus Color 860",	3,	15,	0,	0,	0 },
  { "880",	"Stylus Color 880",	3,	15,	0,	0,	0 },
  { "83",	"Stylus Color 83",	3,	15,	0,	0,	0 },
  { "900",	"Stylus Color 900",	3,	15,	0,	0,	0 },
  { "980",	"Stylus Color 980",	3,	15,	0,	0,	0 },
  { "1160",	"Stylus Color 1160",	3,	15,	0,	0,	0 },
  { "1500",	"Stylus Color 1500",	1,	7,	0,	0,	0 },
  { "1520",	"Stylus Color 1520",	1,	7,	0,	0,	0 },
  { "3000",	"Stylus Color 3000",	1,	7,	0,	0,	0 },
  { "photo",	"Stylus Photo",		1,	7,	0,	0,	0 },
  { "700",	"Stylus Photo 700",	1,	7,	0,	0,	0 },
  { "ex",	"Stylus Photo EX",	1,	7,	0,	0,	0 },
  { "720",	"Stylus Photo 720",	3,	15,	0,	0,	0 },
  { "750",	"Stylus Photo 750",	3,	15,	0,	0,	0 },
  { "780",	"Stylus Photo 780",	3,	15,	0,	0,	0 },
  { "785",	"Stylus Photo 785",	3,	15,	0,	0,	0 },
  { "790",	"Stylus Photo 790",	3,	15,	0,	0,	0 },
  { "870",	"Stylus Photo 870",	3,	15,	0,	0,	0 },
  { "875",	"Stylus Photo 875",	3,	15,	0,	0,	0 },
  { "890",	"Stylus Photo 890",	3,	15,	0,	0,	0 },
  { "895",	"Stylus Photo 895",	3,	15,	0,	0,	0 },
  { "1200",	"Stylus Photo 1200",	3,	15,	0,	0,	0 },
  { "1270",	"Stylus Photo 1270",	3,	15,	0,	0,	0 },
  { "1280",	"Stylus Photo 1280",	3,	15,	0,	0,	0 },
  { "1290",	"Stylus Photo 1290",	3,	15,	0,	0,	0 },
  { "2000",	"Stylus Photo 2000P",	2,	15,	0,	0,	0 },
  { "5000",	"Stylus Pro 5000",	1,	7,	0,	0,	0 },
  { "5500",	"Stylus Pro 5500",	1,	7,	0,	0,	0 },
  { "7000",	"Stylus Pro 7000",	1,	7,	0,	0,	0 },
  { "7500",	"Stylus Pro 7500",	1,	7,	0,	0,	0 },
  { "9000",	"Stylus Pro 9000",	1,	7,	0,	0,	0 },
  { "9500",	"Stylus Pro 9500",	1,	7,	0,	0,	0 },
  { "10000",	"Stylus Pro 10000",	3,	15,	0,	0,	0 },
  { NULL,	NULL,			0,	0,	0,	0,	0 },
};

char *printer = NULL;
char *raw_device = NULL;
char *printer_model = NULL;
char printer_cmd[1025];
int bufpos = 0;
int isUSB = 0;

void
do_help(int code)
{
  stp_printer_t *printer = &printer_list[0];
  printf("%s", help_msg);
  printf("Available models are:\n");
  while (printer->short_name)
    {
      printf("%10s      %s\n", printer->short_name, printer->long_name);
      printer++;
    }
  exit(code);
}

static void
exit_packet_mode(void)
{
  static char hdr[] = "\000\000\000\033\001@EJL 1284.4\n@EJL     \n\033@";
  memcpy(printer_cmd + bufpos, hdr, sizeof(hdr) - 1); /* DON'T include null! */
  bufpos += sizeof(hdr) - 1;
}

static void
initialize_print_cmd(void)
{
  bufpos = 0;
  if (isUSB)
    exit_packet_mode();
}

int
main(int argc, char **argv)
{
  int quiet = 0;
  int operation = 0;
  int c;
  while (1)
    {
#ifdef __GNU_LIBRARY__
      int option_index = 0;
      c = getopt_long(argc, argv, "P:r:icnaosduqm:", optlist, &option_index);
#else
      c = getopt(argc, argv, "P:r:icnaosduqm:");
#endif
      if (c == -1)
	break;
      switch (c)
	{
	case 'q':
	  quiet = 1;
	  break;
	case 'c':
	case 'i':
	case 'n':
	case 'a':
	case 'd':
	case 's':
	case 'o':
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
	  printer = xmalloc(strlen(optarg) + 1);
	  strcpy(printer, optarg);
	  break;
	case 'r':
	  if (printer || raw_device)
	    {
	      printf("You may only specify one printer or raw device.\n");
	      do_help(1);
	    }
	  raw_device = xmalloc(strlen(optarg) + 1);
	  strcpy(raw_device, optarg);
	  break;
	case 'm':
	  if (printer_model)
	    {
	      printf("You may only specify one printer model.\n");
	      do_help(1);
	    }
	  printer_model = xmalloc(strlen(optarg) + 1);
	  strcpy(printer_model, optarg);
	  break;
	case 'u':
	  isUSB = 1;
	  break;
	case 'h':
	  do_help(0);
	  break;
	default:
	  printf("%s\n", banner);
	  fprintf(stderr, "Unknown option %c\n", c);
	  do_help(1);
	}
    }
  if (!quiet)
    printf("%s\n", banner);
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
    case 'o':
      do_align_color();
      break;
    case 'd':
      do_identify();
      break;
    case 's':
      do_status();
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
  char command[1024];
  memcpy(printer_cmd + bufpos, "\f\033\000\033\000", 5);
  bufpos += 5;
  if (raw_device)
    {
      pfile = fopen(raw_device, "wb");
      if (!pfile)
	{
	  fprintf(stderr, "Cannot open device %s: %s\n", raw_device,
		  strerror(errno));
	  return 1;
	}
    }
  else
    {
      if (!access("/bin/lpr", X_OK) ||
          !access("/usr/bin/lpr", X_OK) ||
          !access("/usr/bsd/lpr", X_OK))
        {
        if (printer == NULL)
          strcpy(command, "lpr -l");
	else
          sprintf(command, "lpr -P%s -l", printer);
        }
      else if (printer == NULL)
	strcpy(command, "lp -s -oraw");
      else
	sprintf(command, "lp -s -oraw -d%s", printer);

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

static int
read_from_printer(int fd, char *buf, int bufsize)
{
#ifdef HAVE_POLL
  struct pollfd ufds;
#endif
  int status;
  int retry = 5;
  memset(buf, 0, bufsize);
  do
    {
#ifdef HAVE_POLL
      ufds.fd = fd;
      ufds.events = POLLIN;
      ufds.revents = 0;
      (void) poll(&ufds, 1, 1000);
#endif
      status = read(fd, buf, bufsize - 1);
      if (status <= 0)
	sleep(1);
    }
  while ((status == 0) && (--retry != 0));
  return status;
}

static void
do_remote_cmd(const char *cmd, int nargs, ...)
{
  static char remote_hdr[] = "\033@\033(R\010\000\000REMOTE1";
  static char remote_trailer[] = "\033\000\000\000\033\000";
  int i;
  va_list args;
  va_start(args, nargs);

  memcpy(printer_cmd + bufpos, remote_hdr, sizeof(remote_hdr) - 1);
  bufpos += sizeof(remote_hdr) - 1;
  memcpy(printer_cmd + bufpos, cmd, 2);
  bufpos += 2;
  printer_cmd[bufpos] = nargs % 256;
  printer_cmd[bufpos + 1] = (nargs >> 8) % 256;
  if (nargs > 0)
    for (i = 0; i < nargs; i++)
      printer_cmd[bufpos + 2 + i] = va_arg(args, int);
  bufpos += 2 + nargs;
  memcpy(printer_cmd + bufpos, remote_trailer, sizeof(remote_trailer) - 1);
  bufpos += sizeof(remote_trailer) - 1;
}

static void
add_newlines(int count)
{
  int i;
  for (i = 0; i < count; i++)
    {
      printer_cmd[bufpos++] = '\r';
      printer_cmd[bufpos++] = '\n';
    }
}

static void
add_resets(int count)
{
  int i;
  for (i = 0; i < count; i++)
    {
      printer_cmd[bufpos++] = '\033';
      printer_cmd[bufpos++] = '\000';
    }
}

const char *colors[] = 
{
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
  initialize_print_cmd();
  do_remote_cmd("ST", 2, 0, 1);
  add_resets(2);
  if (write(fd, printer_cmd, bufpos) < bufpos)
    {
      fprintf(stderr, "Cannot write to %s: %s\n", raw_device, strerror(errno));
      exit(1);
    }
  status = read_from_printer(fd, buf, 1024);
  if (status < 0)
    {
      fprintf(stderr, "Cannot read from %s: %s\n", raw_device,strerror(errno));
      exit(1);
    }
  ind = buf;
  do
    ind = strchr(ind, 'I');
  while (ind && ind[1] != 'Q' && (ind[1] != '\0' && ind[2] != ':'));
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
  initialize_print_cmd();
  do_remote_cmd("ST", 2, 0, 0);
  add_resets(2);
  (void) write(fd, printer_cmd, bufpos);
  (void) read(fd, buf, 1024);
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
  initialize_print_cmd();
  add_resets(2);
  (void) write(fd, printer_cmd, bufpos);
  bufpos = 0;
  sprintf(printer_cmd, "\033\001@EJL ID\r\n");
  if (write(fd, printer_cmd, strlen(printer_cmd)) < strlen(printer_cmd))
    {
      fprintf(stderr, "Cannot write to %s: %s\n", raw_device, strerror(errno));
      exit(1);
    }
  status = read_from_printer(fd, buf, 1024);
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
do_status(void)
{
  int fd;
  int status;
  char buf[1024];
  char *where;
  memset(buf, 0, 1024);
  if (!raw_device)
    {
      fprintf(stderr, "Printer status requires using a raw device.\n");
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
  initialize_print_cmd();
  do_remote_cmd("ST", 2, 0, 1);
  if (write(fd, printer_cmd, bufpos) < bufpos)
    {
      fprintf(stderr, "Cannot write to %s: %s\n", raw_device, strerror(errno));
      exit(1);
    }
  status = read_from_printer(fd, buf, 1024);
  if (status < 0)
    {
      fprintf(stderr, "Cannot read from %s: %s\n", raw_device,strerror(errno));
      exit(1);
    }
  initialize_print_cmd();
  do_remote_cmd("ST", 2, 0, 0);
  add_resets(2);
  (void) write(fd, printer_cmd, bufpos);
  (void) read(fd, buf, 1024);
  while ((where = strchr(buf, ';')) != NULL)
    *where = '\n';
  printf("%s\n", buf);
  (void) close(fd);
  exit(0);
}


void
do_head_clean(void)
{
  do_remote_cmd("CH", 2, 0, 0);
  printf("Cleaning heads...\n");
  exit(do_print_cmd());
}

void
do_nozzle_check(void)
{
  do_remote_cmd("VI", 2, 0, 0);
  do_remote_cmd("NC", 2, 0, 0);
  printf("Running nozzle check, please ensure paper is in the printer.\n");
  exit(do_print_cmd());
}

const char new_align_help[] = "\
Please read these instructions very carefully before proceeding.\n\
\n\
This utility lets you align the print head of your Epson Stylus inkjet\n\
printer.  Misuse of this utility may cause your print quality to degrade\n\
and possibly damage your printer.  This utility has not been reviewed by\n\
Seiko Epson for correctness, and is offered with no warranty at all.  The\n\
entire risk of using this utility lies with you.\n\
\n\
This utility prints %d test patterns.  Each pattern looks very similar.\n\
The patterns consist of a series of pairs of vertical lines that overlap.\n\
Below each pair of lines is a number between %d and %d.\n\
\n\
When you inspect the pairs of lines, you should find the pair of lines that\n\
is best in alignment, that is, that best forms a single vertical line.\n\
Inspect the pairs very carefully to find the best match.  Using a loupe\n\
or magnifying glass is recommended for the most critical inspection.\n\
It is also suggested that you use a good quality paper for the test,\n\
so that the lines are well-formed and do not spread through the paper.\n\
After picking the number matching the best pair, place the paper back in\n\
the paper input tray before typing it in.\n\
\n\
Each pattern is similar, but later patterns use finer dots for more\n\
critical alignment.  You must run all of the passes to correctly align your\n\
printer.  After running all the alignment passes, the alignment\n\
patterns will be printed once more.  You should find that the middle-most\n\
pair (#%d out of the %d) is the best for all patterns.\n\
\n\
After the passes are printed once more, you will be offered the\n\
choices of (s)aving the result in the printer, (r)epeating the process,\n\
or (q)uitting without saving.  Quitting will not restore the previous\n\
settings, but powering the printer off and back on will.  If you quit,\n\
you must repeat the entire process if you wish to later save the results.\n\
It is essential that you not turn your printer off during this procedure.\n\n";

const char old_align_help[] = "\
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
%d and %d.\n\
\n\
When you inspect the pairs of lines, you should find the pair of lines that\n\
is best in alignment, that is, that best forms a single vertical align.\n\
Inspect the pairs very carefully to find the best match.  Using a loupe\n\
or magnifying glass is recommended for the most critical inspection.\n\
It is also suggested that you use a good quality paper for the test,\n\
so that the lines are well-formed and do not spread through the paper.\n\
After picking the number matching the best pair, place the paper back in\n\
the paper input tray before typing it in.\n\
\n\
After running the alignment pattern, it will be printed once more.  You\n\
should find that the middle-most pair (#%d out of the %d) is the best.\n\
You will then be offered the choices of (s)aving the result in the printer,\n\
(r)epeating the process, or (q)uitting without saving.  Quitting will not\n\
restore the previous settings, but powering the printer off and back on will.\n\
If you quit, you must repeat the entire process if you wish to later save\n\
the results.  It is essential that you not turn off your printer during\n\
this procedure.\n\n";

static void
do_align_help(int passes, int choices)
{
  if (passes > 1)
    printf(new_align_help, passes, 1, choices, (choices + 1) / 2, choices);
  else
    printf(old_align_help, 1, choices, (choices + 1) / 2, choices);
  fflush(stdout);
}

static void
printer_error(void)
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
  char *inbuf;
  long answer;
  char *endptr;
  int passes = 0;
  int choices = 0;
  int curpass;
  int notfound = 1;
  stp_printer_t *printer = &printer_list[0];
  const char *printer_name = NULL;
  if (!printer_model)
    {
      char buf[1024];
      int fd;
      int status;
      char *pos = NULL;
      char *spos = NULL;
      if (!raw_device)
	{
	  printf("Printer alignment must be done with a raw device or else\n");
	  printf("the -m option must be used to specify a printer.\n");
	  do_help(1);
	}
      printf("Attempting to detect printer model...");
      fflush(stdout);
      fd = open(raw_device, O_RDWR, 0666);
      if (fd == -1)
	{
	  fprintf(stderr, "\nCannot open %s read/write: %s\n", raw_device,
		  strerror(errno));
	  exit(1);
	}
      bufpos = 0;
      sprintf(printer_cmd, "\033\001@EJL ID\r\n");
      if (write(fd, printer_cmd, strlen(printer_cmd)) < strlen(printer_cmd))
	{
	  fprintf(stderr, "\nCannot write to %s: %s\n", raw_device,
		  strerror(errno));
	  exit(1);
	}
      status = read_from_printer(fd, buf, 1024);
      if (status < 0)
	{
	  fprintf(stderr, "\nCannot read from %s: %s\n", raw_device,
		  strerror(errno));
	  exit(1);
	}
      (void) close(fd);
      pos = strchr(buf, (int) ';');
      if (pos)
	pos = strchr(pos + 1, (int) ';');
      if (pos)
	pos = strchr(pos, (int) ':');
      if (pos)
	spos = strchr(pos, (int) ';');
      if (!pos)
	{
	  fprintf(stderr, "\nCannot detect printer type.  Please use -m to specify your printer model.\n");
	  do_help(1);
	}
      if (spos)
	*spos = '\000';
      printer_model = pos + 1;
      printf("%s\n\n", printer_model);
    }
  while (printer->short_name && notfound)
    {
      if (!strcasecmp(printer_model, printer->short_name) ||
	  !strcasecmp(printer_model, printer->long_name))
	{
	  passes = printer->passes;
	  choices = printer->choices;
	  printer_name = printer->long_name;
	  notfound = 0;
	}
      else
	printer++;
    }
  if (notfound)
    {
      printf("Printer model %s is not known.\n", printer_model);
      do_help(1);
    }

 start:
  do_align_help(passes, choices);
  printf("This procedure assumes that your printer is an Epson %s.\n",
	 printer_name);
  printf("If this is not your printer model, please type control-C now and\n");
  printf("choose your actual printer model.\n");
  printf("\n");
  printf("Please place a sheet of paper in your printer to begin the head\n");
  printf("alignment procedure.\n");
  inbuf = do_get_input("Press enter to continue > ");
  initialize_print_cmd();
  for (curpass = 1; curpass <= passes; curpass ++)
    {
    top:
      add_newlines(7 * (curpass - 1));
      do_remote_cmd("DT", 3, 0, curpass - 1, 0);
      if (do_print_cmd())
	printer_error();
    reread:
      printf("Please inspect the print, and choose the best pair of lines\n");
      if (curpass == passes)
	printf("in pattern #%d, and then insert a fresh page in the input tray.\n",
	       curpass);
      else
	printf("in pattern #%d, and then reinsert the page in the input tray.\n",
	       curpass);
      printf("Type a pair number, '?' for help, or 'r' to retry this pattern.\n");
      fflush(stdout);
      inbuf = do_get_input("> ");
      switch (inbuf[0])
	{
	case 'r':
	case 'R':
	  printf("Please insert a fresh sheet of paper.\n");
	  fflush(stdout);
	  initialize_print_cmd();
	  (void) do_get_input("Press enter to continue > ");
	  /* Ick. Surely there's a cleaner way? */
	  goto top;
	case 'h':
	case '?':
	  do_align_help(passes, choices);
	  fflush(stdout);
	case '\n':
	case '\000':
	  goto reread;
	default:
	  break;
	}
      answer = strtol(inbuf, &endptr, 10);
      if (errno == ERANGE)
      {
	printf("Number out of range!\n");
	goto reread;
      }
      if (endptr == inbuf)
	{
	  printf("I cannot understand what you typed!\n");
	  fflush(stdout);
	  goto reread;
	}
      if (answer < 1 || answer > choices)
	{
	  printf("The best pair of lines should be numbered between 1 and %d.\n",
		 choices);
	  fflush(stdout);
	  goto reread;
	}
      if (curpass == passes)
	{
	  printf("Aligning phase %d, and performing final test.\n", curpass);
	  printf("Please insert a fresh sheet of paper.\n");
	  (void) do_get_input("Press enter to continue > ");
	}
      else
	printf("Aligning phase %d, and starting phase %d.\n", curpass,
	       curpass + 1);
      fflush(stdout);
      initialize_print_cmd();
      do_remote_cmd("DA", 4, 0, curpass - 1, 0, answer);
    }
  for (curpass = 0; curpass < passes; curpass++)
    do_remote_cmd("DT", 3, 0, curpass, 0);
  if (do_print_cmd())
    printer_error();
 read_final:
  printf("Please inspect the final output very carefully to ensure that your\n");
  printf("printer is in proper alignment. You may now:\n");
  printf("  (s)ave the results in the printer,\n");
  printf("  (q)uit without saving the results, or\n");
  printf("  (r)epeat the entire process from the beginning.\n");
  printf("You will then be asked to confirm your choice.\n");
  printf("What do you want to do (s, q, r)?\n");
  fflush(stdout);
  inbuf = do_get_input("> ");
  switch (inbuf[0])
    {
    case 'q':
    case 'Q':
      printf("Please confirm by typing 'q' again that you wish to quit without saving:\n");
      fflush(stdout);
      inbuf = do_get_input ("> ");
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
      printf("alignment process:\n");
      fflush(stdout);
      inbuf = do_get_input("> ");
      if (inbuf[0] == 'r' || inbuf[0] == 'R')
	{
	  printf("Repeating the alignment process.\n");
	  goto start;
	}
      break;
    case 's':
    case 'S':
      printf("This will permanently alter the configuration of your printer.\n");
      printf("WARNING: this procedure has not been approved by Seiko Epson, and\n");
      printf("it may damage your printer. Proceed?\n");
      printf("Please confirm by typing 's' again that you wish to save the settings\n");
      printf("to your printer:\n");

      fflush(stdout);
      inbuf = do_get_input("> ");
      if (inbuf[0] == 's' || inbuf[0] == 'S')
	{
	  printf("Please insert your alignment test page in the printer once more\n");
	  printf("for the final save of your alignment settings.  When the printer\n");
	  printf("feeds the page through, your settings have been saved.\n");
	  fflush(stdout);
	  initialize_print_cmd();
	  add_newlines(2);
	  do_remote_cmd("SV", 0);
	  add_newlines(2);
	  if (do_print_cmd())
	    printer_error();
	  exit(0);
	}
      break;
    default:
      printf("Unrecognized command.\n");
      goto read_final;
    }
  printf("Final command was not confirmed.\n");
  goto read_final;
}

const char color_align_help[] = "\
Please read these instructions very carefully before proceeding.\n\
\n\
This utility lets you align the color print head of your Epson Stylus inkjet\n\
printer.  Misuse of this utility may cause your print quality to degrade\n\
and possibly damage your printer.  This utility has not been reviewed by\n\
Seiko Epson for correctness, and is offered with no warranty at all.  The\n\
entire risk of using this utility lies with you.\n\
\n\
This utility prints %d overprinting test patterns on one piece of paper.\n\
That is, it prints one pattern and ejects the page.  You must then reinsert\n\
the same page, and it will print another pattern.  Each pattern consists of\n\
a set of choices numbered between %d and %d.\n\
\n\
When you inspect the patterns, you should find one patch to have the\n\
smoothest texture (least ``grain'').  You should inspect the patches very\n\
carefully to choose the best one.  We suggest using Photo Quality Inkjet\n\
Paper or a similar high quality paper for this test.  If you do not find\n\
a smooth pattern, you should repeat the test.\n\
\n\
After you inspect the choices and select a patch, you will be offered the\n\
choices of (s)aving the result in the printer, (r)epeating the process,\n\
or (q)uitting without saving.  Quitting will not restore the previous\n\
settings, but powering the printer off and back on will.  If you quit,\n\
you must repeat the entire process if you wish to later save the results.\n\
It is essential that you not turn your printer off during this procedure.\n\
\n\
WARNING: THIS FUNCTION IS NOT YET TESTED!  It may not work, and it may\n\
damage your printer!\n";

static void
do_align_color_help(int passes, int choices)
{
  printf(color_align_help, 1, choices);
  fflush(stdout);
}

void
do_align_color(void)
{
  char *inbuf;
  long answer;
  char *endptr;
  int passes = 0;
  int choices = 0;
  int curpass;
  int notfound = 1;
  stp_printer_t *printer = &printer_list[0];
  const char *printer_name = NULL;
  if (!printer_model)
    {
      char buf[1024];
      int fd;
      int status;
      char *pos = NULL;
      char *spos = NULL;
      if (!raw_device)
	{
	  printf("Printer alignment must be done with a raw device or else\n");
	  printf("the -m option must be used to specify a printer.\n");
	  do_help(1);
	}
      printf("Attempting to detect printer model...");
      fflush(stdout);
      fd = open(raw_device, O_RDWR, 0666);
      if (fd == -1)
	{
	  fprintf(stderr, "\nCannot open %s read/write: %s\n", raw_device,
		  strerror(errno));
	  exit(1);
	}
      bufpos = 0;
      sprintf(printer_cmd, "\033\001@EJL ID\r\n");
      if (write(fd, printer_cmd, strlen(printer_cmd)) < strlen(printer_cmd))
	{
	  fprintf(stderr, "\nCannot write to %s: %s\n", raw_device,
		  strerror(errno));
	  exit(1);
	}
      status = read_from_printer(fd, buf, 1024);
      if (status < 0)
	{
	  fprintf(stderr, "\nCannot read from %s: %s\n", raw_device,
		  strerror(errno));
	  exit(1);
	}
      (void) close(fd);
      pos = strchr(buf, (int) ';');
      if (pos)
	pos = strchr(pos + 1, (int) ';');
      if (pos)
	pos = strchr(pos, (int) ':');
      if (pos)
	spos = strchr(pos, (int) ';');
      if (!pos)
	{
	  fprintf(stderr, "\nCannot detect printer type.  Please use -m to specify your printer model.\n");
	  do_help(1);
	}
      if (spos)
	*spos = '\000';
      printer_model = pos + 1;
      printf("%s\n\n", printer_model);
    }
  while (printer->short_name && notfound)
    {
      if (!strcasecmp(printer_model, printer->short_name) ||
	  !strcasecmp(printer_model, printer->long_name))
	{
	  passes = printer->color_passes;
	  choices = printer->color_choices;
	  printer_name = printer->long_name;
	  notfound = 0;
	}
      else
	printer++;
    }
  if (notfound)
    {
      printf("Printer model %s is not known.\n", printer_model);
      do_help(1);
    }
  if (passes == 0)
    {
      printf("Printer %s does not require color head alignment.\n",
	     printer_model);
      exit(0);
    }

 start:
  do_align_color_help(passes, choices);
  printf("This procedure assumes that your printer is an Epson %s.\n",
	 printer_name);
  printf("If this is not your printer model, please type control-C now and\n");
  printf("choose your actual printer model.\n");
  printf("\n");
  printf("Please place a fresh sheet of paper in your printer to begin the head\n");
  printf("alignment procedure.\n");
  inbuf = do_get_input("Press enter to continue > ");
  for (curpass = 1; curpass <= passes; curpass ++)
    {
      initialize_print_cmd();
      do_remote_cmd("DU", 6, 0, curpass, 0, 9, 0, curpass - 1);
      if (do_print_cmd())
	printer_error();
      if (curpass < passes)
	{
	  printf("Please re-insert the same alignment sheet in the printer when it is\n");
	  printf("finished printing.\n");
	  (void) do_get_input("Press enter to continue > ");
	}
    }
 reread:
  printf("Inspect the alignment sheet, and determine which pattern is the smoothest.\n");
  printf("This pattern will appear to have the least ``grain''.\n");
  printf("If you cannot find a smooth pattern, please select the number for the\n");
  printf("best pattern, and repeat the procedure.\n");
  printf("Type a pattern number, or '?' for help.\n");
  fflush(stdout);
  inbuf = do_get_input("> ");
  if (!inbuf)
    exit(1);
  switch (inbuf[0])
    {
    case 'h':
    case '?':
      do_align_color_help(passes, choices);
      fflush(stdout);
      /* FALLTHROUGH */
    case '\n':
    case '\000':
      goto reread;
    default:
      break;
    }
  answer = strtol(inbuf, &endptr, 10);
  if (errno == ERANGE)
    {
      printf("Number out of range!\n");
      goto reread;
    }
  if (endptr == inbuf)
    {
      printf("I cannot understand what you typed!\n");
      fflush(stdout);
      goto reread;
    }
  if (answer < 1 || answer > choices)
    {
      printf("The best pattern should be numbered between 1 and %d.\n",
	     choices);
      fflush(stdout);
      goto reread;
    }
  initialize_print_cmd();
  do_remote_cmd("DA", 6, 0, 0, 0, answer, 9, 0);
  if (do_print_cmd())
    printer_error();
 read_final:
  printf("Please inspect the final output very carefully to ensure that your\n");
  printf("printer is in proper alignment. You may now:\n");
  printf("  (s)ave the results in the printer,\n");
  printf("  (q)uit without saving the results, or\n");
  printf("  (r)epeat the entire process from the beginning.\n");
  printf("You will then be asked to confirm your choice.\n");
  printf("What do you want to do (s, q, r)?\n");
  fflush(stdout);
  inbuf = do_get_input("> ");
  switch (inbuf[0])
    {
    case 'q':
    case 'Q':
      printf("Please confirm by typing 'q' again that you wish to quit without saving:\n");
      fflush(stdout);
      inbuf = do_get_input ("> ");
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
      printf("alignment process:\n");
      fflush(stdout);
      inbuf = do_get_input("> ");
      if (inbuf[0] == 'r' || inbuf[0] == 'R')
	{
	  printf("Repeating the alignment process.\n");
	  goto start;
	}
      break;
    case 's':
    case 'S':
      printf("This will permanently alter the configuration of your printer.\n");
      printf("WARNING: this procedure has not been approved by Seiko Epson, and\n");
      printf("it may damage your printer. Proceed?\n");
      printf("Please confirm by typing 's' again that you wish to save the settings\n");
      printf("to your printer:\n");

      fflush(stdout);
      inbuf = do_get_input("> ");
      if (inbuf[0] == 's' || inbuf[0] == 'S')
	{
	  printf("Please insert your alignment test page in the printer once more\n");
	  printf("for the final save of your alignment settings.  When the printer\n");
	  printf("feeds the page through, your settings have been saved.\n");
	  fflush(stdout);
	  initialize_print_cmd();
	  add_newlines(2);
	  do_remote_cmd("SV", 0);
	  add_newlines(2);
	  if (do_print_cmd())
	    printer_error();
	  exit(0);
	}
      break;
    default:
      printf("Unrecognized command.\n");
      goto read_final;
    }
  printf("Final command was not confirmed.\n");
  goto read_final;
}

char *
do_get_input (const char *prompt)
{
	static char *input = NULL;
#if (HAVE_LIBREADLINE == 0 || !defined HAVE_READLINE_READLINE_H)
	char *fgets_status;
#endif
	/* free only if previously allocated */
	if (input)
	{
		free (input);
		input = NULL;
	}
#if (HAVE_LIBREADLINE > 0 && defined HAVE_READLINE_READLINE_H)
	/* get input with libreadline, if present */
	input = readline ((char *) prompt);
	/* if input, add to history list */
#ifdef HAVE_READLINE_HISTORY_H
	if (input && *input)
	{
		add_history (input);
	}
#endif
#else
	/* no libreadline; use fgets instead */
	input = xmalloc (sizeof (char) * BUFSIZ);
	memset(input, 0, BUFSIZ);
	printf ("%s", prompt);
	fgets_status = fgets (input, BUFSIZ, stdin);
	if (fgets_status == NULL)
	{
		fprintf (stderr, "Error in input\n");
		return (NULL);
	}
	else if (strlen (input) == 1 && input[0] == '\n')
	{
		/* user just hit enter: empty input buffer */
		/* remove line feed */
		input[0] = '\0';
	}
	else
	{
		/* remove line feed */
		input[strlen (input) - 1] = '\0';
	}
#endif
	return (input);
}
