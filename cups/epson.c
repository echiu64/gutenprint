/*
 * "$Id$"
 *
 *   EPSON backend for the CUPS driver development kit.
 *
 *   Copyright 1997-2000 by Easy Software Products, all rights reserved.
 *
 *   These coded instructions, statements, and computer programs are the
 *   property of Easy Software Products and are protected by Federal
 *   copyright law.  Distribution and use rights are outlined in the file
 *   "LICENSE" which should have been included with this file.  If this
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
 *   main()         - Send a file to the specified parallel port.
 *   list_devices() - List all parallel devices.
 */

/*
 * Include necessary headers.
 */

#include <cups/cups.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#if defined(WIN32) || defined(__EMX__)
#  include <io.h>
#else
#  include <unistd.h>
#  include <fcntl.h>
#  include <termios.h>
#endif /* WIN32 || __EMX__ */

#if defined(WIN32) || defined(__EMX__)
#  include <winsock.h>
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#endif /* WIN32 || __EMX__ */

#ifdef __sgi
#  include <invent.h>
#  ifndef INV_EPP_ECP_PLP
#    define INV_EPP_ECP_PLP	6	/* From 6.3/6.4/6.5 sys/invent.h */
#    define INV_ASO_SERIAL	14	/* serial portion of SGI ASO board */
#    define INV_IOC3_DMA	16	/* DMA mode IOC3 serial */
#    define INV_IOC3_PIO	17	/* PIO mode IOC3 serial */
#    define INV_ISA_DMA		19	/* DMA mode ISA serial -- O2 */
#  endif /* !INV_EPP_ECP_PLP */
#endif /* __sgi */


/*
 * Local functions...
 */

void	list_devices(void);


/*
 * 'main()' - Send a file to the specified parallel port.
 *
 * Usage:
 *
 *    printer-uri job-id user title copies options [file]
 */

int			/* O - Exit status */
main(int  argc,		/* I - Number of command-line arguments (6 or 7) */
     char *argv[])	/* I - Command-line arguments */
{
  char		method[1024],	/* Method in URI */
		hostname[1024],	/* Hostname */
		username[1024],	/* Username info (not used) */
		resource[1024],	/* Resource info (device and options) */
		*options;	/* Pointer to options */
  int		port;		/* Port number (not used) */
  FILE		*fp;		/* Print file */
  int		copies;		/* Number of copies to print */
  int		fd,		/* Parallel/USB device or socket */
		error;		/* Last error */
  struct sockaddr_in addr;	/* Socket address */
  struct hostent *hostaddr;	/* Host address */
  int		wbytes;		/* Number of bytes written */
  size_t	nbytes,		/* Number of bytes read */
		tbytes;		/* Total number of bytes written */
  char		buffer[8192],	/* Output buffer */
		*bufptr;	/* Pointer into buffer */
  struct timeval timeout;	/* Timeout for select() */
  fd_set	input;		/* Input set for select() */
  struct termios opts;		/* Parallel port options */
#if defined(HAVE_SIGACTION) && !defined(HAVE_SIGSET)
  struct sigaction action;	/* Actions for POSIX signals */
#endif /* HAVE_SIGACTION && !HAVE_SIGSET */


  if (argc == 1)
  {
    list_devices();
    return (0);
  }
  else if (argc < 6 || argc > 7)
  {
    fputs("Usage: epson job-id user title copies options [file]\n", stderr);
    return (1);
  }

 /*
  * If we have 7 arguments, print the file named on the command-line.
  * Otherwise, send stdin instead...
  */

  if (argc == 6)
  {
    fp     = stdin;
    copies = 1;
  }
  else
  {
   /*
    * Try to open the print file...
    */

    if ((fp = fopen(argv[6], "rb")) == NULL)
    {
      perror("ERROR: unable to open print file");
      return (1);
    }

    copies = atoi(argv[4]);
  }

 /*
  * Extract the device name and options from the URI...
  */

  httpSeparate(argv[0], method, username, hostname, &port, resource);

 /*
  * See if there are any options...
  */

  if ((options = strchr(resource, '?')) != NULL)
  {
   /*
    * Yup, terminate the device name string and move to the first
    * character of the options...
    */

    *options++ = '\0';
  }

  if (hostname[0])
  {
   /*
    * Lookup the IP address...
    */

    if ((hostaddr = gethostbyname(hostname)) == NULL)
    {
      fprintf(stderr, "ERROR: Unable to locate printer \'%s\' - %s",
              hostname, strerror(errno));
      return (1);
    }

    if (port == 0)
      port = 9100;	/* Default for EPSON NIC */

    fprintf(stderr, "INFO: Attempting to connect to printer %s on port %d\n",
            hostname, port);

    memset(&addr, 0, sizeof(addr));
    memcpy(&(addr.sin_addr), hostaddr->h_addr, hostaddr->h_length);
    addr.sin_family = hostaddr->h_addrtype;
    addr.sin_port   = htons(port);

   /*
    * Try to connect...
    */

    for (;;)
    {
      if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      {
	perror("ERROR: Unable to create socket");
	return (1);
      }

      if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
      {
        error = errno;
	close(fd);
	fd = -1;

	if (error == ECONNREFUSED)
	{
	  fprintf(stderr, "INFO: Network printer \'%s\' is busy; will retry in 30 seconds...\n",
                  hostname);
	  sleep(30);
	}
	else
	{
	  perror("ERROR: Unable to connect to printer");
	  sleep(30);
	}
      }
      else
	break;
    }

    fputs("INFO: Connected to printer, sending print job...\n", stderr);
  }
  else
  {
   /*
    * Open the parallel or USB port device...
    */

    do
    {
      if ((fd = open(resource, O_RDWR | O_EXCL)) == -1)
      {
	if (errno == EBUSY)
	{
          fputs("INFO: Parallel port busy; will retry in 30 seconds...\n", stderr);
	  sleep(30);
	}
	else
	{
	  perror("ERROR: Unable to open parallel port device file");
	  return (1);
	}
      }
    }
    while (fd < 0);

   /*
    * Set any options provided...
    */

    tcgetattr(fd, &opts);

    opts.c_cflag |= CREAD;			/* Enable reading */
    opts.c_lflag &= ~(ICANON | ECHO | ISIG);	/* Raw mode */

    /**** No options supported yet ****/

    tcsetattr(fd, TCSANOW, &opts);
  }

 /*
  * Now that we are "connected" to the port, ignore SIGTERM so that we
  * can finish out any page data the driver sends (e.g. to eject the
  * current page...
  */

#ifdef HAVE_SIGSET /* Use System V signals over POSIX to avoid bugs */
  sigset(SIGTERM, SIG_IGN);
#elif defined(HAVE_SIGACTION)
  memset(&action, 0, sizeof(action));

  sigemptyset(&action.sa_mask);
  action.sa_handler = SIG_IGN;
  sigaction(SIGTERM, &action, NULL);
#else
  signal(SIGTERM, SIG_IGN);
#endif /* HAVE_SIGSET */

 /*
  * Finally, send the print file...
  */

  while (copies > 0)
  {
    copies --;

    if (fp != stdin)
    {
      fputs("PAGE: 1 1\n", stderr);
      rewind(fp);
    }

    tbytes = 0;
    while ((nbytes = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
     /*
      * Write the print data to the printer...
      */

      tbytes += nbytes;
      bufptr = buffer;

      while (nbytes > 0)
      {
	if ((wbytes = write(fd, bufptr, nbytes)) < 0)
	{
	  perror("ERROR: Unable to send print file to printer");
	  break;
	}

	nbytes -= wbytes;
	bufptr += wbytes;
      }

      if (nbytes > 0)
	break;

     /*
      * Check for possible data coming back from the printer...
      */

      timeout.tv_sec = 0;
      timeout.tv_usec = 0;
      FD_ZERO(&input);
      FD_SET(fd, &input);
      if (select(fd + 1, &input, NULL, NULL, &timeout) > 0)
      {
       /*
	* Grab the data coming back and spit it out to stderr...
	*/

	if ((nbytes = read(fd, buffer, sizeof(buffer) - 1)) < 0)
	{
	  fprintf(stderr, "ERROR: Back-channel read error - %s!\n",
	          strerror(errno));
	  break;
	}

        buffer[nbytes] = '\0';
	if (strncmp(buffer, "@BDC ", 5) != 0)
	  fprintf(stderr, "WARNING: Received %d bytes of unknown back-channel data!\n",
	          nbytes);
	else
	{
	 /*
	  * Skip initial report line...
	  */

	  for (bufptr = buffer; *bufptr && *bufptr != '\n'; bufptr ++);

	  if (*bufptr == '\n')
	    bufptr ++;

         /*
	  * Get status data...
	  */

          strcpy(buffer, bufptr);
	  for (bufptr = buffer; *bufptr && *bufptr != ';'; bufptr ++);
	  *bufptr = '\0';

	  if (strncmp(buffer, "IQ:", 3) == 0)
	  {
	   /*
	    * Report ink level...
	    */

            int i;
            int levels[6];

            buffer[12] = '\0'; /* Limit to 6 inks */
	    for (i = 0, bufptr = buffer; i < 6; i ++, bufptr += 2)
	    {
	      if (isalpha(bufptr[0]))
	        levels[i] = (tolower(bufptr[0]) - 'a' + 10) << 16;
	      else
	        levels[i] = (bufptr[0] - '0') << 16;

	      if (isalpha(bufptr[1]))
	        levels[i] |= tolower(bufptr[1]) - 'a' + 10;
	      else
	        levels[i] |= bufptr[1] - '0';
            }

            switch (i)
	    {
	      case 1 :
	      case 2 :
	          fprintf(stderr, "K=%d\n", levels[0]);
		  break;
	      case 3 :
	          fprintf(stderr, "C=%d M=%d Y=%d\n", levels[0], levels[1],
		          levels[2]);
		  break;
	      case 4 :
	      case 5 :
	          fprintf(stderr, "K=%d C=%d M=%d Y=%d\n", levels[0],
		          levels[1], levels[2], levels[3]);
		  break;
	      case 6 :
	          fprintf(stderr, "K=%d C=%d M=%d Y=%d LC=%d LM=%d\n",
		          levels[0], levels[1], levels[2], levels[3],
			  levels[4], levels[5]);
		  break;
            }
	  }
	  else
	    fprintf(stderr, "INFO: %s\n", buffer);
        }
      }
      else if (argc > 6)
	fprintf(stderr, "INFO: Sending print file, %u bytes...\n", tbytes);
    }
  }

 /*
  * Close the socket connection or parallel/USB device and input file and
  * return...
  */

  close(fd);
  if (fp != stdin)
    fclose(fp);

  return (0);
}


/*
 * 'list_devices()' - List all parallel devices.
 */

void
list_devices(void)
{
  static char	*funky_hex = "0123456789abcdefghijklmnopqrstuvwxyz";
				/* Funky hex numbering used for some devices */

#ifdef __linux
  int	i;			/* Looping var */
  int	fd;			/* File descriptor */
  char	device[255];		/* Device filename */
  FILE	*probe;			/* /proc/parport/n/autoprobe file */
  char	line[1024],		/* Line from file */
	*delim,			/* Delimiter in file */
	make[IPP_MAX_NAME],	/* Make from file */
	model[IPP_MAX_NAME];	/* Model from file */


 /*
  * Probe for parallel devices...
  */

  for (i = 0; i < 4; i ++)
  {
    sprintf(device, "/proc/parport/%d/autoprobe", i);
    if ((probe = fopen(device, "r")) != NULL)
    {
      memset(make, 0, sizeof(make));
      memset(model, 0, sizeof(model));
      strcpy(model, "EPSON");

      while (fgets(line, sizeof(line), probe) != NULL)
      {
       /*
        * Strip trailing ; and/or newline.
	*/

        if ((delim = strrchr(line, ';')) != NULL)
	  *delim = '\0';
	else if ((delim = strrchr(line, '\n')) != NULL)
	  *delim = '\0';

       /*
        * Look for MODEL and MANUFACTURER lines...
	*/

        if (strncmp(line, "MODEL:", 6) == 0 &&
	    strncmp(line, "MODEL:EPSON", 13) != 0)
	  strncpy(model, line + 6, sizeof(model) - 1);
	else if (strncmp(line, "MANUFACTURER:", 13) == 0 &&
	         strncmp(line, "MANUFACTURER:EPSON", 20) != 0)
	  strncpy(make, line + 13, sizeof(make) - 1);
      }

      fclose(probe);

      if (strcmp(make, "EPSON") == 0)
	printf("direct parallel:/dev/lp%d \"%s %s\" \"Parallel Port #%d\"\n",
	       i, make, model, i + 1);
    }
    else
    {
      sprintf(device, "/dev/lp%d", i);
      if ((fd = open(device, O_WRONLY)) >= 0)
      {
	close(fd);
	printf("direct parallel:%s \"EPSON\" \"Parallel Port #%d\"\n", device, i + 1);
      }
      else
      {
	sprintf(device, "/dev/par%d", i);
	if ((fd = open(device, O_WRONLY)) >= 0)
	{
	  close(fd);
	  printf("direct parallel:%s \"EPSON\" \"Parallel Port #%d\"\n", device, i + 1);
	}
      }
    }
  }

 /*
  * Probe for USB devices...
  */

  if ((probe = fopen("/proc/bus/usb/devices", "r")) != NULL)
  {
    i = 0;

    memset(make, 0, sizeof(make));
    memset(model, 0, sizeof(model));

    while (fgets(line, sizeof(line), probe) != NULL)
    {
     /*
      * Strip trailing newline.
      */

      if ((delim = strrchr(line, '\n')) != NULL)
	*delim = '\0';

     /*
      * See if it is a printer device ("P: ...")
      */

      if (strncmp(line, "S:", 2) == 0)
      {
       /*
        * String attribute...
	*/

        if (strncmp(line, "S:  Manufacturer=", 17) == 0)
	{
	  strncpy(make, line + 17, sizeof(make) - 1);
	  if (strcmp(make, "Hewlett-Packard") == 0)
	    strcpy(make, "HP");
	}
        else if (strncmp(line, "S:  Product=", 12) == 0)
	  strncpy(model, line + 12, sizeof(model) - 1);
      }
      else if (strncmp(line, "I:", 2) == 0 &&
               strstr(line, "Driver=printer") != NULL &&
	       make[0] && model[0])
      {
       /*
        * We were processing a printer device; send the info out...
	*/

        if (strcmp(make, "EPSON") == 0)
	  printf("direct usb:/dev/usb/lp%d \"%s %s\" \"USB Printer #%d\"\n",
		 i, make, model, i + 1);

	i ++;

	memset(make, 0, sizeof(make));
	memset(model, 0, sizeof(model));
      }
    }

    fclose(probe);
  }
  else
  {
    for (i = 0; i < 8; i ++)
    {
      sprintf(device, "/dev/usb/lp%d", i);
      if ((fd = open(device, O_WRONLY)) >= 0)
      {
	close(fd);
	printf("direct usb:%s \"EPSON\" \"USB Printer #%d\"\n", device, i + 1);
      }

      sprintf(device, "/dev/usblp%d", i);
      if ((fd = open(device, O_WRONLY)) >= 0)
      {
	close(fd);
	printf("direct usb:%s \"EPSON\" \"USB Printer #%d\"\n", device, i + 1);
      }
    }
  }
#elif defined(__sgi)
  int		i, j, n;	/* Looping vars */
  char		device[255];	/* Device filename */
  inventory_t	*inv;		/* Hardware inventory info */


 /*
  * IRIX maintains a hardware inventory of most devices...
  */

  setinvent();

  while ((inv = getinvent()) != NULL)
  {
    if (inv->inv_class == INV_PARALLEL &&
        (inv->inv_type == INV_ONBOARD_PLP ||
         inv->inv_type == INV_EPP_ECP_PLP))
    {
     /*
      * Standard parallel port...
      */

      puts("direct parallel:/dev/plp \"EPSON\" \"Onboard Parallel Port\"");
    }
    else if (inv->inv_class == INV_PARALLEL &&
             inv->inv_type == INV_EPC_PLP)
    {
     /*
      * EPC parallel port...
      */

      printf("direct parallel:/dev/plp%d \"EPSON\" \"Integral EPC parallel port, Ebus slot %d\"\n",
             inv->inv_controller, inv->inv_controller);
    }
  }

  endinvent();

 /*
  * Central Data makes serial and parallel "servers" that can be
  * connected in a number of ways.  Look for ports...
  */

  for (i = 0; i < 10; i ++)
    for (j = 0; j < 8; j ++)
      for (n = 0; n < 32; n ++)
      {
        if (i == 8)		/* EtherLite */
          sprintf(device, "/dev/lpn%d%c", j, funky_hex[n]);
        else if (i == 9)	/* PCI */
          sprintf(device, "/dev/lpp%d%c", j, funky_hex[n]);
        else			/* SCSI */
          sprintf(device, "/dev/lp%d%d%c", i, j, funky_hex[n]);

	if (access(device, 0) == 0)
	{
	  if (i == 8)
	    printf("direct parallel:%s \"EPSON\" \"Central Data EtherLite Parallel Port, ID %d, port %d\"\n",
	           device, j, n);
	  else if (i == 9)
	    printf("direct parallel:%s \"EPSON\" \"Central Data PCI Parallel Port, ID %d, port %d\"\n",
	           device, j, n);
  	  else
	    printf("direct parallel:%s \"EPSON\" \"Central Data SCSI Parallel Port, logical bus %d, ID %d, port %d\"\n",
	           device, i, j, n);
	}
      }
#elif defined(__sun)
  int		i, j, n;	/* Looping vars */
  char		device[255];	/* Device filename */


 /*
  * Standard parallel ports...
  */

  for (i = 0; i < 10; i ++)
  {
    sprintf(device, "/dev/ecpp%d", i);
    if (access(device, 0) == 0)
      printf("direct parallel:%s \"EPSON\" \"Sun IEEE-1284 Parallel Port #%d\"\n",
             device, i + 1);
  }

  for (i = 0; i < 10; i ++)
  {
    sprintf(device, "/dev/bpp%d", i);
    if (access(device, 0) == 0)
      printf("direct parallel:%s \"EPSON\" \"Sun Standard Parallel Port #%d\"\n",
             device, i + 1);
  }

  for (i = 0; i < 3; i ++)
  {
    sprintf(device, "/dev/lp%d", i);

    if (access(device, 0) == 0)
      printf("direct parallel:%s \"EPSON\" \"PC Parallel Port #%d\"\n",
             device, i + 1);
  }

 /*
  * MAGMA parallel ports...
  */

  for (i = 0; i < 40; i ++)
  {
    sprintf(device, "/dev/pm%02d", i);
    if (access(device, 0) == 0)
      printf("direct parallel:%s \"EPSON\" \"MAGMA Parallel Board #%d Port #%d\"\n",
             device, (i / 10) + 1, (i % 10) + 1);
  }

 /*
  * Central Data parallel ports...
  */

  for (i = 0; i < 9; i ++)
    for (j = 0; j < 8; j ++)
      for (n = 0; n < 32; n ++)
      {
        if (i == 8)	/* EtherLite */
          sprintf(device, "/dev/sts/lpN%d%c", j, funky_hex[n]);
        else
          sprintf(device, "/dev/sts/lp%c%d%c", i + 'C', j,
                  funky_hex[n]);

	if (access(device, 0) == 0)
	{
	  if (i == 8)
	    printf("direct parallel:%s \"EPSON\" \"Central Data EtherLite Parallel Port, ID %d, port %d\"\n",
	           device, j, n);
  	  else
	    printf("direct parallel:%s \"EPSON\" \"Central Data SCSI Parallel Port, logical bus %d, ID %d, port %d\"\n",
	           device, i, j, n);
	}
      }
#elif defined(__hpux)
  int		i, j, n;	/* Looping vars */
  char		device[255];	/* Device filename */


 /*
  * Standard parallel ports...
  */

  if (access("/dev/rlp", 0) == 0)
    puts("direct parallel:/dev/rlp \"EPSON\" \"Standard Parallel Port (/dev/rlp)\"");

  for (i = 0; i < 7; i ++)
    for (j = 0; j < 7; j ++)
    {
      sprintf(device, "/dev/c%dt%dd0_lp", i, j);
      if (access(device, 0) == 0)
	printf("direct parallel:%s \"EPSON\" \"Parallel Port #%d,%d\"\n", i, j);
    }

 /*
  * Central Data parallel ports...
  */

  for (i = 0; i < 9; i ++)
    for (j = 0; j < 8; j ++)
      for (n = 0; n < 32; n ++)
      {
        if (i == 8)	/* EtherLite */
          sprintf(device, "/dev/lpN%d%c", j, funky_hex[n]);
        else
          sprintf(device, "/dev/lp%c%d%c", i + 'C', j,
                  funky_hex[n]);

	if (access(device, 0) == 0)
	{
	  if (i == 8)
	    printf("direct parallel:%s \"EPSON\" \"Central Data EtherLite Parallel Port, ID %d, port %d\"\n",
	           device, j, n);
  	  else
	    printf("direct parallel:%s \"EPSON\" \"Central Data SCSI Parallel Port, logical bus %d, ID %d, port %d\"\n",
	           device, i, j, n);
	}
      }
#elif defined(__osf__)
  int	i;			/* Looping var */
  int	fd;			/* File descriptor */
  char	device[255];		/* Device filename */


  for (i = 0; i < 3; i ++)
  {
    sprintf(device, "/dev/lp%d", i);
    if ((fd = open(device, O_WRONLY)) >= 0)
    {
      close(fd);
      printf("direct parallel:%s \"EPSON\" \"Parallel Port #%d\"\n", device, i + 1);
    }
  }
#elif defined(FreeBSD) || defined(OpenBSD) || defined(NetBSD)
  int	i;			/* Looping var */
  int	fd;			/* File descriptor */
  char	device[255];		/* Device filename */


 /*
  * Probe for parallel devices...
  */

  for (i = 0; i < 3; i ++)
  {
    sprintf(device, "/dev/lpt%d", i);
    if ((fd = open(device, O_WRONLY)) >= 0)
    {
      close(fd);
      printf("direct parallel:%s \"EPSON\" \"Parallel Port #%d\"\n", device, i + 1);
    }
  }

 /*
  * Probe for USB devices...
  */

  for (i = 0; i < 3; i ++)
  {
    sprintf(device, "/dev/ulpt%d", i);
    if ((fd = open(device, O_WRONLY)) >= 0)
    {
      close(fd);
      printf("direct usb:%s \"EPSON\" \"USB Port #%d\"\n", device, i + 1);
    }
  }
#endif
}


/*
 * End of "$Id$".
 */
