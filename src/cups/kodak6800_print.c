/*
 *   Kodak 6800 Photo Printer print assister
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

#define VERSION "0.11"
#define URI_PREFIX "kodak6800://"
#define STR_LEN_MAX 64

#include "backend_common.c"

/* USB Identifiers */
#define USB_VID_KODAK      0x040A
#define USB_PID_KODAK_6800 0x4021

/* Program states */
enum {
	S_IDLE = 0,
	S_PRINTER_READY_HDR,
	S_PRINTER_SENT_HDR,
	S_PRINTER_SENT_HDR2,
	S_PRINTER_SENT_DATA,
	S_FINISHED,
};

struct kodak6800_hdr {
	uint8_t  hdr[9];
	uint8_t  copies;
	uint16_t columns;  /* BE */
	uint16_t rows;     /* BE */
	uint8_t  media;    /* 0x06 for 6x8, 0x00 for 6x4, 0x07 for 5x7 */ 
	uint8_t  laminate; /* 0x01 to laminate, 0x00 for not */
	uint8_t  unk1; /* 0x00, 0x01 [may be print mode] */
} __attribute__((packed));

#define CMDBUF_LEN 17
#define READBACK_LEN 58

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

		if (desc.idVendor != USB_VID_KODAK)
			continue;

		switch(desc.idProduct) {
		case USB_PID_KODAK_6800:
			found = i;
			break;
		default:
			continue;
		}

		found = print_scan_output((*list)[i], &desc,
					  URI_PREFIX, "Kodak", 
					  found, (found == i), 1, 
					  scan_only, match_serno);
	}

	return found;
}

#define UPDATE_SIZE 1536
static int get_tonecurve(char *fname, libusb_device_handle *dev, 
			 uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t cmdbuf[16];
	uint8_t respbuf[64];
	int ret, num = 0;
	int i;

	uint16_t *data = malloc(UPDATE_SIZE);

	INFO("Dump Tone Curve to '%s'\n", fname);

	/* Initial Request */
	cmdbuf[0] = 0x03;
	cmdbuf[1] = 0x1b;
	cmdbuf[2] = 0x43;
	cmdbuf[3] = 0x48;
	cmdbuf[4] = 0x43;
	cmdbuf[5] = 0x0c;
	cmdbuf[6] = 0x54;
	cmdbuf[7] = 0x4f;
	cmdbuf[8] = 0x4e;
	cmdbuf[9] = 0x45;
	cmdbuf[10] = 0x72;
	cmdbuf[11] = 0x01;
	cmdbuf[12] = 0x00;
	cmdbuf[13] = 0x00;
	cmdbuf[14] = 0x00;
	cmdbuf[15] = 0x00;

	if ((ret = send_data(dev, endp_down,
			     cmdbuf, 16)))
		return -1;
	
	ret = libusb_bulk_transfer(dev, endp_up,
				   respbuf,
				   sizeof(respbuf),
				   &num,
				   5000);
	if (ret < 0 || (num != 51)) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(respbuf), endp_up);
		return ret;
	}

	/* Then we can poll the data */
	cmdbuf[0] = 0x03;
	cmdbuf[1] = 0x1b;
	cmdbuf[2] = 0x43;
	cmdbuf[3] = 0x48;
	cmdbuf[4] = 0x43;
	cmdbuf[5] = 0x0c;
	cmdbuf[6] = 0x54;
	cmdbuf[7] = 0x4f;
	cmdbuf[8] = 0x4e;
	cmdbuf[9] = 0x45;
	cmdbuf[10] = 0x20;
	for (i = 0 ; i < 24 ; i++) {
		if ((ret = send_data(dev, endp_down,
				     cmdbuf, 11)))
			return -1;


		ret = libusb_bulk_transfer(dev, endp_up,
					   respbuf,
					   sizeof(respbuf),
					   &num,
					   5000);
		if (ret < 0 || (num != 64)) {
			ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(respbuf), endp_up);
			return ret;
		}

		/* Copy into buffer */
		memcpy(((uint8_t*)data)+i*64, respbuf, 64);
	}

	/* Open file and write it out */
	{
		int tc_fd = open(fname, O_WRONLY|O_CREAT);
		if (tc_fd < 0)
			return -1;
		
		for (i = 0 ; i < 768; i++) {
			/* Byteswap appropriately */
			data[i] = cpu_to_be16(le16_to_cpu(data[i]));
			write(tc_fd, &data[i], sizeof(uint16_t));
		}
		close(tc_fd);
	}


	/* We're done */
	free(data);

	return 0;
}

static int set_tonecurve(char *fname, libusb_device_handle *dev, 
			 uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t cmdbuf[64];
	uint8_t respbuf[64];
	int ret, num = 0;
	int remain;

	uint16_t *data = malloc(UPDATE_SIZE);
	uint8_t *ptr;

	INFO("Set Tone Curve from '%s'\n", fname);

	/* Read in file */
	int tc_fd = open(fname, O_RDONLY);
	if (tc_fd < 0)
		return -1;
	if (read(tc_fd, data, UPDATE_SIZE) != UPDATE_SIZE)
		return -2;
	close(tc_fd);

	/* Byteswap data to printer's format */
	for (ret = 0; ret < (UPDATE_SIZE-16)/2 ; ret++) {
		data[ret] = cpu_to_le16(be16_to_cpu(data[ret]));
	}

	/* Initial Request */
	cmdbuf[0] = 0x03;
	cmdbuf[1] = 0x1b;
	cmdbuf[2] = 0x43;
	cmdbuf[3] = 0x48;
	cmdbuf[4] = 0x43;
	cmdbuf[5] = 0x0c;
	cmdbuf[6] = 0x54;
	cmdbuf[7] = 0x4f;
	cmdbuf[8] = 0x4e;
	cmdbuf[9] = 0x45;
	cmdbuf[10] = 0x77;
	cmdbuf[11] = 0x01;
	cmdbuf[12] = 0x00;
	cmdbuf[13] = 0x00;
	cmdbuf[14] = 0x00;
	cmdbuf[15] = 0x00;

	if ((ret = send_data(dev, endp_down,
			     cmdbuf, 16)))
		return -1;
	
	ret = libusb_bulk_transfer(dev, endp_up,
				   respbuf,
				   sizeof(respbuf),
				   &num,
				   5000);
	if (ret < 0 || (num != 51)) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(respbuf), endp_up);
		return ret;
	}

	ptr = (uint8_t*) data;
	remain = UPDATE_SIZE;
	while (remain > 0) {
		int count = remain > 63 ? 63 : remain;

		cmdbuf[0] = 0x03;
		memcpy(cmdbuf+1, ptr, count);

		remain -= count;
		ptr += count;

		/* Send next block over */
		if ((ret = send_data(dev, endp_down,
				     cmdbuf, count+1)))
			return -1;

		ret = libusb_bulk_transfer(dev, endp_up,
					   respbuf,
					   sizeof(respbuf),
					   &num,
					   5000);
		if (ret < 0 || (num != 51)) {
			ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(respbuf), endp_up);
			return ret;
		}

	};
        
	/* We're done */
	free(data);
	return 0;
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

	int i, num;
	int claimed;

	int query_only = 0;
	int ret = 0;
	int iface = 0;
	int found = -1;
	int copies = 1;
	char *uri = getenv("DEVICE_URI");;
	char *use_serno = NULL;

	struct kodak6800_hdr hdr;
	uint8_t *planedata, *cmdbuf;
	uint32_t datasize;

	uint8_t rdbuf[READBACK_LEN];
	uint8_t rdbuf2[READBACK_LEN];
	int last_state = -1, state = S_IDLE;

	DEBUG("Kodak 6800 CUPS backend version " VERSION "/" BACKEND_VERSION " \n");

	/* Cmdline help */
	if (argc < 2) {
		DEBUG("Usage:\n\t%s [ infile | - ]\n\t%s job user title num-copies options [ filename ]\n\t%s [ -qtc filename | -stc filename ] \n\n",
		      argv[0], argv[0], argv[0]);
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
		if (!strcmp("-qtc", argv[1]) ||
		    !strcmp("-stc", argv[1])) {
			query_only = 1;
			goto skip_read;
		}

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

	/* Read in then validate header */
	read(data_fd, &hdr, sizeof(hdr));
	if (hdr.hdr[0] != 0x03 ||
	    hdr.hdr[1] != 0x1b ||
	    hdr.hdr[2] != 0x43 ||
	    hdr.hdr[3] != 0x48 ||
	    hdr.hdr[4] != 0x43) {
		ERROR("Unrecognized data format!\n");
		exit(1);
	}

	/* Read in image data */
	cmdbuf = malloc(CMDBUF_LEN);
	datasize = be16_to_cpu(hdr.rows) * be16_to_cpu(hdr.columns) * 3;
	planedata = malloc(datasize);
	if (!cmdbuf || !planedata) {
		ERROR("Memory allocation failure!\n");
		exit(1);
	}

	{
		int remain;
		uint8_t *ptr = planedata;
		remain = datasize;
		do {
			ret = read(data_fd, ptr, remain);
			if (ret < 0) {
				ERROR("Read failed (%d/%d/%d)\n", 
				      ret, remain, datasize);
				perror("ERROR: Read failed");
				exit(1);
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}
	close(data_fd); /* We're done reading! */

 skip_read:	
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

	if (query_only) {
		if (!strcmp("-qtc", argv[1]))
			get_tonecurve(argv[2], dev, endp_down, endp_up);
		if (!strcmp("-stc", argv[1]))
			set_tonecurve(argv[2], dev, endp_down, endp_up);
		goto done_claimed;
	}

	/* Time for the main processing loop */

top:
	/* Send State Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x03;
	cmdbuf[1] = 0x1b;
	cmdbuf[2] = 0x43;
	cmdbuf[3] = 0x48;
	cmdbuf[4] = 0x43;
	cmdbuf[5] = 0x03;

	if ((ret = send_data(dev, endp_down,
			    cmdbuf, CMDBUF_LEN - 1)))
		goto done_claimed;

	/* Read in the printer status */
	memset(rdbuf, 0, READBACK_LEN);
	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || ((num != 51) && (num != 58))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, READBACK_LEN, endp_up);
		ret = 4;
		goto done_claimed;
	}

	if (memcmp(rdbuf, rdbuf2, READBACK_LEN)) {
		DEBUG("readback: ");
		for (i = 0 ; i < num ; i++) {
			DEBUG2("%02x ", rdbuf[i]);
		}
		DEBUG2("\n");
	} else {
		sleep(1);
	}
	if (state != last_state) {
		DEBUG("last_state %d new %d\n", last_state, state);
		last_state = state;
	}
	fflush(stderr);       

	switch (state) {
	case S_IDLE:
		INFO("Waiting for printer idle\n");
		if (rdbuf[0] != 0x01 ||
		    rdbuf[1] != 0x02 ||
		    rdbuf[2] != 0x01) {
			break;
		}

		state = S_PRINTER_READY_HDR;
		break;
	case S_PRINTER_READY_HDR:
		INFO("Printing started; Sending init sequence\n");
		/* Send reset/attention */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x03;
		cmdbuf[1] = 0x1b;
		cmdbuf[2] = 0x43;
		cmdbuf[3] = 0x48;
		cmdbuf[4] = 0x43;
		cmdbuf[5] = 0x1a;
		if ((ret = send_data(dev, endp_down,
				     cmdbuf, CMDBUF_LEN -1)))
			goto done_claimed;
		state = S_PRINTER_SENT_HDR;
		break;
	case S_PRINTER_SENT_HDR:
		INFO("Waiting for printer to acknowledge start\n");
		if (rdbuf[0] != 0x01 ||
		    rdbuf[1] != 0x03 ||
		    rdbuf[2] != 0x00) {
			break;
		}

		memcpy(cmdbuf, &hdr, CMDBUF_LEN);

		/* If we're printing a 4x6 on 8x6 media... */
		if (hdr.media == 0x00 &&
		    rdbuf[11] == 0x09 &&
		    rdbuf[12] == 0x82) {
			cmdbuf[14] = 0x06;
			cmdbuf[16] = 0x01;
		}

		INFO("Sending image header\n");
		if ((ret = send_data(dev, endp_down,
				     cmdbuf, CMDBUF_LEN)))
			goto done_claimed;

		state = S_PRINTER_SENT_HDR2;
		break;
	case S_PRINTER_SENT_HDR2:
		INFO("Waiting for printer to accept data\n");
		if (rdbuf[0] != 0x01 ||
		    rdbuf[1] != 0x02 ||
		    rdbuf[2] != 0x01) {
			break;
		}

		INFO("Sending image data\n");
		if ((ret = send_data(dev, endp_down, planedata, datasize)))
			goto done_claimed;

		INFO("Image data sent\n");
		state = S_PRINTER_SENT_DATA;
		break;
	case S_PRINTER_SENT_DATA:
		INFO("Waiting for printer to acknowledge completion\n");
		if (rdbuf[0] != 0x01 ||
		    rdbuf[1] != 0x02 ||
		    rdbuf[2] != 0x01) {
			break;
		}

		state = S_FINISHED;
		break;
	default:
		break;
	};

	if (state != S_FINISHED)
		goto top;

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d remaining)\n", copies - 1);

	if (copies && --copies) {
		state = S_IDLE;
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
	if (planedata)
		free(planedata);
	if (cmdbuf)
		free(cmdbuf);

	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	return ret;
}

/* Kodak 6800/6850 data format

  Spool file consists of 17-byte header followed by plane-interleaved BGR data.
  Native printer resolution is 1844 pixels per row, and 1240 or 2434 rows.

  6850 Adds support for 5x7, with 1548 pixels per row and 2140 columns.

  Header:

  03 1b 43 48 43 0a 00 01 00     Fixed header
  CC                             Number of copies
  WW WW                          Number of columns, big endian. (Fixed at 1844 on 6800)
  HH HH                          Number of rows, big endian.
  DD                             0x00 (4x6) 0x06 (8x6) 0x07 (5x7 on 6850)
  LL                             Laminate, 0x00 (off) or 0x01 (on)
  00

  ************************************************************************

  The data format actually sent to the Kodak 6800 is subtly different.

[file header] 03 1b 43 48 43 0a 00 01  00 CC WW WW HH HH MT LL 00

->  03 1b 43 48 43 03 00 00  00 00 00 00 00 00 00 00  [status query]
<-  [51 octets]

    01 02 01 00 00 00 00 00  00 00 a2 7b 00 00 a2 7b  [ a2 7b may be print counters, increments after each print ]
    00 00 02 f4 00 00 e6 b1  00 00 00 1a 00 03 00 e8  [ e6 b1 may be a print counter, increments by 2 after each print ]
    00 01 00 83 00 00 00 00  00 00 00 00 00 00 00 00  [ "00" after "83" seems to be a per-powerup print counter, increments by 1 after each "get ready" command ]
    00 00 00

->  03 1b 43 48 43 03 00 00  00 00 00 00 00 00 00 00  [status query]
<-  [51 octets -- same as above]

->  03 1b 43 48 43 1a 00 00  00 00 00 00 00 00 00 00  [get ready]
<-  [58 octets]

    01 03 00 00 00 00 00 04  06 WW WW MM MM 01 00 00  [MM MM == max printable size of media, 09 82 == 2434 for 6x8!]
    00 00 06 WW WW 09 ba 01  02 00 00 00 06 WW WW HH  [09 ba == 2940 == cut area?]
    HH 01 01 00 00 00 06 WW  WW MM MM 01 03 00 00 00
    00 00 00 00 00 00 00 00  00 00

->  03 1b 43 48 43 0a 00 01  00 01 07 34 04 d8 06 01 01 [ image header, modified -- last octet is always 0x01.  '06' may be the expected media size in the printer? ]
<-  [51 octets]

    01 02 01 00 00 00 00 00  00 00 a2 7b 00 00 a2 7b
    00 00 02 f4 00 00 e6 b1  00 00 00 1a 00 03 00 e8
    00 01 00 83 01 00 00 01  00 00 00 01 00 00 00 00 [ note the "01" after "83", and the extra two "01"s ]
    00 00 00

->  [4K of plane data]
->  ...
->  [4K of plane data]
->  [remainder of plane data + 17 bytes of 0xff]

->  03 1b 43 48 43 03 00 00  00 00 00 00 00 00 00 00 [status query]
<-  [51 octets]

    01 02 01 00 00 00 00 00  00 00 a2 7b 00 00 a2 7b
    00 00 02 f4 00 00 e6 b1  00 00 00 1a 00 03 00 e8
    00 01 00 83 01 00 00 01  00 00 00 01 00 00 00 00
    00 00 00

->  03 1b 43 48 43 03 00 00  00 00 00 00 00 00 00 00
<-  [51 octets, repeats]

  Other stuff seen:

->  03 1b 43 48 43 12 00 00  00 00 00 00 00 00 00 00
<-  [32 octets]

    00 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  [[ Pascal string? ]]
    20 20 20 20 20 20 20 20  36 30 34 33 4d 32 38 31  [[ ..."  6043M281" ]]

->  03 1b 43 48 43 0c 54 4f  4e 45 65 00 00 00 00 00
<-  [51 octets]

    [[ typical status response ]]
    [[ Followed by reset. ]]s

  Read tone curve data:  

->  03 1b 43 48 43 0c 54 4f  4e 45 72 01 00 00 00 00
<-  [51 octets]

    [[ typical status response ]]

->  03 1b 43 48 43 0c 54 4f  4e 45 20
<-  [64 octets]

    81 01 07 07 27 07 72 07  c8 07 f8 07 22 07 48 08
    68 08 88 08 b3 08 db 08  f7 08 09 09 2e 09 49 09
    65 09 80 09 aa 09 ca 09  e2 09 fa 09 12 0a 32 0a
    42 0a 66 0a 81 0a 9a 0a  c3 0a d9 0a ee 0a 04 0b

->  03 1b 43 48 43 0c 54 4f  4e 45 20
<-  [64 octets]

  [[ repeats for total of 24 packets.  total of 1.5KiB. ]]

  Write tone curve data:

->  03 1b 43 48 43 0c 54 4f  4e 45 77 01 00 00 00 00
<-  [51 octets]

    [[ typical status response ]]

->  03 00 00 46 06 53 06 c0  06 07 07 37 07 5d 07 87 
    07 a1 07 c8 07 08 08 08  08 08 08 48 08 68 08 88
    08 a9 08 b9 08 d9 08 f9  08 12 09 2e 09 49 09 70
    09 89 08 99 09 ba 09 ca  08 da 09 0a 0a 24 0a 38
<-  [51 octets]

    [[ typical status response ]]

->  03 0a 53 0a 66 0a 81 0a ...
   ....
->  03 cf 38 0a 39 3d 39 79  39 96 39 b6 39 fb 39 01
    34 0a 34 08 3a 0c 1a 10  3a
<-  [51 octets]

    [[ typical status response ]]

  [[ total of 24 packets * 64, and then one final packet of 25: 1562 total. ]]
  [[ It apepars the extra 25 bytes are to compensate for the leading '03' on 
     each of the 25 URBs. ]]

*/
