/*
 *   Sony UP-DR150 Photo Printer print assister
 *
 *   (c) 2013 Solomon Peachy <pizza@shaftnet.org>
 *
 *   The latest version of this program can be found at:
 *  
 *     http://git.shaftnet.org/git/gitweb.cgi?p=selphy_print.git
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
 *
 *          [http://www.gnu.org/licenses/gpl-2.0.html]
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define VERSION "0.02"
#define URI_PREFIX "sonyupdr150://"
#define STR_LEN_MAX 64

#include "backend_common.c"

/* USB Identifiers */
#define USB_VID_SONY      0x054C
#define USB_PID_UP_DR150  0x01E8

#define MAX_PRINTJOB_LEN 16736455

static int find_and_enumerate(struct libusb_context *ctx,
			      struct libusb_device ***list,
			      char *match_serno,
			      int scan_only)
{
	int num;
	int i;
	int found = -1;

	/* Enumerate and find suitable device */
	num = libusb_get_device_list(ctx, list);

	for (i = 0 ; i < num ; i++) {
		struct libusb_device_descriptor desc;

		libusb_get_device_descriptor((*list)[i], &desc);

		if (desc.idVendor != USB_VID_SONY)
			continue;

		switch(desc.idProduct) {
		case USB_PID_UP_DR150:
			found = i;
			break;
		default:
			continue;
		}

		found = print_scan_output((*list)[i], &desc,
					  URI_PREFIX, "", 
					  found, (found == i), 1, 
					  scan_only, match_serno);
	}

	return found;
}

int main (int argc, char **argv) 
{
	struct libusb_context *ctx;
	struct libusb_device **list;
	struct libusb_device_handle *dev;
	struct libusb_config_descriptor *config;

	uint8_t endp_up = 0;
	uint8_t endp_down = 0;

	int data_fd = fileno(stdin);

	int i;
	int claimed;

	int ret = 0;
	int iface = 0;
	int found = -1;
	int copies = 1;
	char *uri = getenv("DEVICE_URI");;
	char *use_serno = NULL;
	uint8_t *databuf = NULL;
	int datalen = 0;

	DEBUG("Sony UP-DR150 CUPS backend version " VERSION "/" BACKEND_VERSION " \n");

	/* Cmdline help */
	if (argc < 2) {
		DEBUG("Usage:\n\t%s [ infile | - ]\n\t%s job user title num-copies options [ filename ]\n\n",
		      argv[0], argv[0]);
		libusb_init(&ctx);
		find_and_enumerate(ctx, &list, NULL, 1);
		libusb_free_device_list(list, 1);
		libusb_exit(ctx);
		exit(1);
	}

	/* Are we running as a CUPS backend? */
	if (uri) {
		if (argv[4])
			copies = atoi(argv[4]);
		if (argv[6]) {  /* IOW, is it specified? */
			data_fd = open(argv[6], O_RDONLY);
			if (data_fd < 0) {
				perror("ERROR:Can't open input file");
				exit(1);
			}
		}

		/* Ensure we're using BLOCKING I/O */
		i = fcntl(data_fd, F_GETFL, 0);
		if (i < 0) {
			perror("ERROR:Can't open input");
			exit(1);
		}
		i &= ~O_NONBLOCK;
		i = fcntl(data_fd, F_SETFL, 0);
		if (i < 0) {
			perror("ERROR:Can't open input");
			exit(1);
		}
		/* Start parsing URI 'selphy://PID/SERIAL' */
		if (strncmp(URI_PREFIX, uri, strlen(URI_PREFIX))) {
			ERROR("Invalid URI prefix (%s)\n", uri);
			exit(1);
		}
		use_serno = strchr(uri, '=');
		if (!use_serno || !*(use_serno+1)) {
			ERROR("Invalid URI (%s)\n", uri);
			exit(1);
		}
		use_serno++;
	} else {
		/* Open Input File */
		if (strcmp("-", argv[1])) {
			data_fd = open(argv[1], O_RDONLY);
			if (data_fd < 0) {
				perror("ERROR:Can't open input file");
				exit(1);
			}
		}
	}

	/* Ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, sigterm_handler);

	/* Read in data */
	databuf = malloc(MAX_PRINTJOB_LEN);
	if (!databuf) {
		ERROR("Memory allocation failure!\n");
		exit(1);
	}

	while((i = read(data_fd, databuf+datalen, 4096)) > 0) {
		datalen += i;
	}

	close(data_fd); /* We're done reading! */

	/* Libusb setup */
	libusb_init(&ctx);
	found = find_and_enumerate(ctx, &list, use_serno, 0);

	if (found == -1) {
		ERROR("Printer open failure (No suitable printers found!)\n");
		ret = 3;
		goto done;
	}

	ret = libusb_open(list[found], &dev);
	if (ret) {
		ERROR("Printer open failure (Need to be root?) (%d)\n", ret);
		ret = 4;
		goto done;
	}
	
	claimed = libusb_kernel_driver_active(dev, iface);
	if (claimed) {
		ret = libusb_detach_kernel_driver(dev, iface);
		if (ret) {
			ERROR("Printer open failure (Could not detach printer from kernel)\n");
			ret = 4;
			goto done_close;
		}
	}

	ret = libusb_claim_interface(dev, iface);
	if (ret) {
		ERROR("Printer open failure (Could not claim printer interface)\n");
		ret = 4;
		goto done_close;
	}

	ret = libusb_get_active_config_descriptor(list[found], &config);
	if (ret) {
		ERROR("Printer open failure (Could not fetch config descriptor)\n");
		ret = 4;
		goto done_close;
	}

	for (i = 0 ; i < config->interface[0].altsetting[0].bNumEndpoints ; i++) {
		if ((config->interface[0].altsetting[0].endpoint[i].bmAttributes & 3) == LIBUSB_TRANSFER_TYPE_BULK) {
			if (config->interface[0].altsetting[0].endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_IN)
				endp_up = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;
			else
				endp_down = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;				
		}
	}

	/* Time for the main processing loop */

	INFO("Printing started (%d copies)\n", copies);

top:
	for (i = 0 ; i < datalen ; ) {
		uint8_t *ptr = databuf + i;
		i += 4;
		if (*ptr & 0xf0) {
			switch (*ptr) {
			case 0x6a:
			case 0xfc:
			case 0xfb:
			case 0xf4:
			case 0xed:
			case 0xf9:
			case 0xf8:
			case 0xec:
			case 0xeb:
			case 0xfa:
			case 0xf3:
				DEBUG("Block ID '%x' (len %d)\n", *ptr, 0);
				break;
			case 0xef:
			case 0xf5:
				DEBUG("Block ID '%x' (len %d)\n", *ptr, 4);
				i += 4;
				break;
			default:
				DEBUG("Unknown block ID '%x'\n", *ptr);
				break;
			}
		} else {
			uint32_t len = le32_to_cpu(*((uint32_t*)ptr));

			DEBUG("Sending %d bytes to printer\n", len);
			if ((ret = send_data(dev, endp_down,
					     databuf + i, len)))
				goto done_claimed;
			i += len;
		}
	}

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d remaining)\n", copies - 1);

	if (copies && --copies) {
		goto top;
	}

	/* Done printing */
	INFO("All printing done\n");
	ret = 0;

done_claimed:
	libusb_release_interface(dev, iface);

done_close:
	if (claimed)
		libusb_attach_kernel_driver(dev, iface);

	libusb_close(dev);
done:

	if (databuf)
		free(databuf);

	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	return ret;
}

/* Sony UP-DR150 Spool file format

   The spool file is a series of 4-byte commands, followed by optional
   arguments.  The purpose of the commands is unknown, but they presumably
   instruct the driver to perform certain things.

   Known commands:

   6a ff ff ff
   fc ff ff ff
   fb ff ff ff
   f4 ff ff ff
   ed ff ff ff
   f9 ff ff ff
   f8 ff ff ff
   ec ff ff ff
   eb ff ff ff
   fa ff ff ff
   f3 ff ff ff
   ef ff ff ff  XX 00 00 00   # XX == print size (0x01/0x02/0x03/0x04)
   f5 ff ff ff  YY 00 00 00   # YY == ??? (seen 0x01)

   Additionally, if the leading 4 bits of the command are 0, it signifies
   that what follows are data bytes to be sent to the printer.

   All printer commands start with 0x1b, and are at least 7 bytes long.

  ************************************************************************

  The data stream sent to the printer consists of all the commands in the
  spool file, plus a couple other ones that generate responses.  It is
  unknown if those additional commands are necessary.  This is a typical
  sequence:

[[ Sniff start ]]

<- 1b e0 00 00 00 0f 00
-> 0e 00 00 00 00 00 00 00  00 04 a8 08 0a a4 00

<- 1b 16 00 00 00 00 00
-> "reset" ??

[[ begin job ]]

<- 1b ef 00 00 00 06 00
-> 05 00 00 00 00 22

<- 1b e5 00 00 00 08 00       ** In spool file
<- 00 00 00 00 00 00 01 00

<- 1b c1 00 02 06 00 00
-> 02 02 00 03 00 00

<- 1b ee 00 00 00 02 00       ** In spool file
<- 00 01

<- 1b 15 00 00 00 0d 00       ** In spool file
<- 00 00 00 00 07 00 00 00  00 08 00 0a a4

<- 1b 03 00 00 00 13 00
-> 70 00 00 00 00 00 00 0b  00 00 00 00 00 00 00 00
   00 00 00

<- 1b e1 00 00 00 0b 00       ** In spool file
<- 00 80 00 00 00 00 00 08  00 0a a4

<- 1b 03 00 00 00 13 00
-> 70 00 00 00 00 00 00 0b  00 00 00 00 00 00 00 00
   00 00 00

<- 1b ea 00 00 00 00 00 ff  60 00 00   ** In spool file
<- [[ 0x0060ff00 bytes of data ]]

<- 1b e0 00 00 00 0f 00
-> 0e 00 00 00 00 00 00 00  04 a8 08 00 0a a4 00

<- 1b 0a 00 00 00 00 00   ** In spool file
<- 1b 17 00 00 00 00 00   ** In spool file

[[fin]]

*/
