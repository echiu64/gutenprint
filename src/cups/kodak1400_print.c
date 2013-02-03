/*
 *   Kodak Professional 1400 print assister
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

#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <libusb-1.0/libusb.h>

#define VERSION "0.07"
#define STR_LEN_MAX 64
#define CMDBUF_LEN 96
#define READBACK_LEN 8
#define URI_PREFIX "kodak1400://"
#define DEBUG( ... ) fprintf(stderr, "DEBUG: " __VA_ARGS__ )
#define ERROR( ... ) fprintf(stderr, "ERROR: " __VA_ARGS__ )

#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define le32_to_cpu(__x) __x
#define le16_to_cpu(__x) __x
#else
#define le32_to_cpu(x)							\
	({								\
		uint32_t __x = (x);					\
		((uint32_t)(						\
			(((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
			(((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
			(((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
			(((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) )); \
	})
#define le16_to_cpu(x)							\
	({								\
		uint16_t __x = (x);					\
		((uint16_t)(						\
			(((uint16_t)(__x) & (uint16_t)0x00ff) <<  8) | \
			(((uint16_t)(__x) & (uint16_t)0xff00) >>  8) | \
	})
#endif

/* USB Identifiers */
#define USB_VID_KODAK      0x040A
#define USB_PID_KODAK_1400 0x4022

/* Program states */
enum {
	S_IDLE = 0,
	S_PRINTER_READY_Y,
	S_PRINTER_SENT_Y,
	S_PRINTER_READY_M,
	S_PRINTER_SENT_M,
	S_PRINTER_READY_C,
	S_PRINTER_SENT_C,
	S_PRINTER_READY_L,
	S_PRINTER_SENT_L,
	S_PRINTER_DONE,
	S_FINISHED,
};

struct kodak1400_hdr {
	uint8_t  hdr[4];
	uint16_t columns;
	uint16_t null1;
	uint16_t rows;
	uint16_t null2;
	uint32_t planesize;
	uint32_t null3;
	uint8_t  matte;
	uint8_t  laminate;
	uint8_t  unk1;  /* Always 0x01 */
	uint8_t  lam_strength;
	uint8_t  null4[12];
};

static uint8_t idle_data[READBACK_LEN] = { 0xe4, 0x72, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00 };


static int find_and_enumerate(struct libusb_context *ctx,
			      struct libusb_device ***list,
			      char *match_serno,
			      int scan_only)
{
	int num;
	int i;
	int found = -1;

	struct libusb_device_handle *dev;

	/* Enumerate and find suitable device */
	num = libusb_get_device_list(ctx, list);

	for (i = 0 ; i < num ; i++) {
		struct libusb_device_descriptor desc;
		unsigned char product[STR_LEN_MAX] = "";
		unsigned char serial[STR_LEN_MAX] = "";
		unsigned char manuf[STR_LEN_MAX] = "";

		libusb_get_device_descriptor((*list)[i], &desc);

		if (desc.idVendor != USB_VID_KODAK)
			continue;

		switch(desc.idProduct) {
		case USB_PID_KODAK_1400: // "Kodak 1400"
			found = i;
			break;
		default:
			continue;
		}

		if (libusb_open(((*list)[i]), &dev)) {
			ERROR("Could not open device %04x:%04x\n", desc.idVendor, desc.idProduct);
			found = -1;
			continue;
		}

		/* Query detailed info */
		if (desc.iManufacturer) {
			libusb_get_string_descriptor_ascii(dev, desc.iManufacturer, manuf, STR_LEN_MAX);
		}
		if (desc.iProduct) {
			libusb_get_string_descriptor_ascii(dev, desc.iProduct, product, STR_LEN_MAX);
		}
		if (desc.iSerialNumber) {
			libusb_get_string_descriptor_ascii(dev, desc.iSerialNumber, serial, STR_LEN_MAX);
		}

		DEBUG("PID: %04X Product: '%s' Serial: '%s'\n",
		      desc.idProduct, product, serial);

		if (scan_only) {
			/* URL-ify model. */
			char buf[128]; // XXX ugly..
			i = 0;
			while (*(product + i + strlen("Kodak"))) {
				buf[i] = *(product + i + strlen("Kodak "));
				if(buf[i] == ' ') {
					buf[i++] = '%';
					buf[i++] = '2';
					buf[i] = '0';
				}
				i++;
			}
			fprintf(stdout, "direct %sKodak/%s?serial=%s \"%s\" \"%s\" \"MFG:Kodak;CMD:Kodak1400Raster;CLS:PRINTER;MDL:%s;DES:%s;SN:%s\" \"\"\n", URI_PREFIX,
			        buf, serial, product, product,
				product + strlen("Kodak "), product, serial);
		}

		/* If a serial number was passed down, use it. */
		if (found && match_serno &&
		    strcmp(match_serno, (char*)serial)) {
			found = -1;
		}

		libusb_close(dev);
	}

	return found;
}

static int send_data(struct libusb_device_handle *dev, uint8_t endp, 
		    uint8_t *buf, uint16_t len)
{
	int num;

	int ret = libusb_bulk_transfer(dev, endp,
				       buf, len,
				       &num, 5000);

	if (ret < 0) {
		ERROR("libusb error %d: (%d/%d to 0x%02x)\n", ret, num, len, endp);
		return ret;
	}
	return 0;
}

static int send_plane(struct libusb_device_handle *dev, uint8_t endp,
		      uint8_t planeno, uint8_t *planedata,
		      struct kodak1400_hdr *hdr, uint8_t *cmdbuf)
{
	int i;
	uint16_t temp16;
	int ret;

	if (planeno != 1) {
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x74;
		cmdbuf[2] = 0x00;
		cmdbuf[3] = 0x50;
	
		if ((ret = send_data(dev, endp,
				     cmdbuf, CMDBUF_LEN)))
			return ret;
	}

	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x5a;
	cmdbuf[2] = 0x54;
	cmdbuf[3] = planeno;

	if (planedata) {
		temp16 = htons(hdr->columns);
		memcpy(cmdbuf+7, &temp16, 2);
		temp16 = htons(hdr->rows);
		memcpy(cmdbuf+9, &temp16, 2);
	}

	if ((ret = send_data(dev, endp,
			     cmdbuf, CMDBUF_LEN)))
		return ret;

	if (planedata) {
		for (i = 0 ; i < hdr->rows ; i++) {
			if ((ret = send_data(dev, endp,
					     planedata + i * hdr->columns, 
					     hdr->columns)))
				return ret;
		}
	}

	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x74;
	cmdbuf[2] = 0x01;
	cmdbuf[3] = 0x50;
	
	if ((ret = send_data(dev, endp,
			     cmdbuf, CMDBUF_LEN)))
		return ret;

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

	uint16_t temp16;

	int data_fd = fileno(stdin);

	int i, num;
	int claimed;

	int ret = 0;
	int iface = 0;
	int found = -1;
	char *uri = getenv("DEVICE_URI");;
	char *use_serno = NULL;

	struct kodak1400_hdr hdr;
	uint8_t *plane_r, *plane_g, *plane_b, *cmdbuf;

	uint8_t rdbuf[READBACK_LEN], rdbuf2[READBACK_LEN];
	int last_state = -1, state = S_IDLE;

	/* Cmdline help */
	if (argc < 2) {
		DEBUG("Kodak 1400 Print Assist version %s\nUsage:\n\t%s [ infile | - ]\n\t%s job user title num-copies options [ filename ] \n\n",
		      VERSION,
		      argv[0], argv[0]);
		libusb_init(&ctx);
		find_and_enumerate(ctx, &list, NULL, 1);
		libusb_free_device_list(list, 1);
		libusb_exit(ctx);
		exit(1);
	}

	/* Are we running as a CUPS backend? */
	if (uri) {
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

	/* Read in then validate header */
	read(data_fd, &hdr, sizeof(hdr));
	if (hdr.hdr[0] != 'P' ||
	    hdr.hdr[1] != 'G' ||
	    hdr.hdr[2] != 'H' ||
	    hdr.hdr[3] != 'D') {
		ERROR("Unrecognized data format!\n");
		exit(1);
	}
	hdr.planesize = le32_to_cpu(hdr.planesize);
	hdr.rows = le16_to_cpu(hdr.rows);
	hdr.columns = le16_to_cpu(hdr.columns);

	/* Set up plane data */
	cmdbuf = malloc(CMDBUF_LEN);
	plane_r = malloc(hdr.planesize);
	plane_g = malloc(hdr.planesize);
	plane_b = malloc(hdr.planesize);
	if (!cmdbuf || !plane_r || !plane_g || !plane_b) {
		ERROR("Memory allocation failure!\n");
		exit(1);
	}
	for (i = 0 ; i < hdr.rows ; i++) {
		int j;
		int remain;
		uint8_t *ptr;
		for (j = 0 ; j < 3 ; j++) {
			if (j == 0)
				ptr = plane_r + i * hdr.columns;
			else if (j == 1)
				ptr = plane_g + i * hdr.columns;
			else if (j == 2)
				ptr = plane_b + i * hdr.columns;

			remain = hdr.columns;
			do {
				ret = read(data_fd, ptr, remain);
				if (ret < 0) {
					ERROR("Read failed (%d/%d/%d) (%d/%d @ %d)\n", 
					      ret, remain, hdr.columns,
					      i, hdr.rows, j);
					perror("ERROR: Read failed");
					exit(1);
				}
				ptr += ret;
				remain -= ret;
			} while (remain);
		}
	}

	/* Libusb setup */
	libusb_init(&ctx);
	found = find_and_enumerate(ctx, &list, use_serno, 0);

	if (found == -1) {
		ERROR("No suitable printers found!\n");
		ret = 3;
		goto done;
	}

	ret = libusb_open(list[found], &dev);
	if (ret) {
		ERROR("Could not open device (Need to be root?) (%d)\n", ret);
		ret = 4;
		goto done;
	}
	
	claimed = libusb_kernel_driver_active(dev, iface);
	if (claimed) {
		ret = libusb_detach_kernel_driver(dev, iface);
		if (ret) {
			ERROR("Could not detach printer from kernel (%d)\n", ret);
			ret = 4;
			goto done_close;
		}
	}

	ret = libusb_claim_interface(dev, iface);
	if (ret) {
		ERROR("Could not claim printer interface (%d)\n", ret);
		ret = 4;
		goto done_close;
	}

	ret = libusb_get_active_config_descriptor(list[found], &config);
	if (ret) {
		ERROR("Could not fetch config descriptor (%d)\n", ret);
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

top:
	/* Send State Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x72;

	if ((ret = send_data(dev, endp_down,
			    cmdbuf, CMDBUF_LEN)))
		goto done_claimed;

	/* Read in the printer status */
	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   2000);

	if (ret < 0) {
		ERROR("libusb error %d: (%d/%d from 0x%02x)\n", ret, num, READBACK_LEN, endp_up);
		ret = 4;
		goto done_claimed;
	}

	if (memcmp(rdbuf, rdbuf2, READBACK_LEN)) {
		DEBUG("readback:  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
		      rdbuf[0], rdbuf[1], rdbuf[2], rdbuf[3],
		      rdbuf[4], rdbuf[5], rdbuf[6], rdbuf[7]);
		memcpy(rdbuf2, rdbuf, READBACK_LEN);
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
		/* Send reset/attention */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;

		if ((ret = send_data(dev, endp_down,
				    cmdbuf, CMDBUF_LEN)))
			goto done_claimed;

		/* Send page setup */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x5a;
		cmdbuf[2] = 0x53;
		temp16 = ntohs(hdr.columns);
		memcpy(cmdbuf+3, &temp16, 2);
		temp16 = ntohs(hdr.rows);
		memcpy(cmdbuf+5, &temp16, 2);

		if ((ret = send_data(dev, endp_down,
				    cmdbuf, CMDBUF_LEN)))
			goto done_claimed;

		/* Send lamination toggle? */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x59;
		cmdbuf[2] = hdr.matte; // ???

		if ((ret = send_data(dev, endp_down,
				    cmdbuf, CMDBUF_LEN)))
			goto done_claimed;

		/* Send matte toggle */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x60;
		cmdbuf[2] = hdr.laminate;

		if (send_data(dev, endp_down,
			     cmdbuf, CMDBUF_LEN))
			goto done_claimed;

		/* Send lamination strength */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x62;
		cmdbuf[2] = hdr.lam_strength;

		if ((ret = send_data(dev, endp_down,
				    cmdbuf, CMDBUF_LEN)))
			goto done_claimed;

		/* Send unknown */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x61;
		cmdbuf[2] = hdr.unk1; // ???

		if ((ret = send_data(dev, endp_down,
				    cmdbuf, CMDBUF_LEN)))
			goto done_claimed;

		state = S_PRINTER_READY_Y;
		break;
	case S_PRINTER_READY_Y:
		if ((ret = send_plane(dev, endp_down,
				      1, plane_b, &hdr, cmdbuf)))
			goto done_claimed;
		state = S_PRINTER_SENT_Y;
		break;
	case S_PRINTER_SENT_Y:
		if (!memcmp(rdbuf, idle_data, READBACK_LEN))
			state = S_PRINTER_READY_M;
		break;
	case S_PRINTER_READY_M:
		if ((ret = send_plane(dev, endp_down,
				      2, plane_g, &hdr, cmdbuf)))
			goto done_claimed;		    
		state = S_PRINTER_SENT_M;
		break;
	case S_PRINTER_SENT_M:
		if (!memcmp(rdbuf, idle_data, READBACK_LEN))
			state = S_PRINTER_READY_C;
		break;
	case S_PRINTER_READY_C:
		if ((ret = send_plane(dev, endp_down,
				      3, plane_r, &hdr, cmdbuf)))
			goto done_claimed;
		state = S_PRINTER_SENT_C;
		break;
	case S_PRINTER_SENT_C:
		if (!memcmp(rdbuf, idle_data, READBACK_LEN)) {
			if (hdr.laminate)
				state = S_PRINTER_READY_L;
			else
				state = S_PRINTER_DONE;
		}
		break;
	case S_PRINTER_READY_L:
		if ((ret = send_plane(dev, endp_down,
				      4, NULL, &hdr, cmdbuf)))
			goto done_claimed;
		state = S_PRINTER_SENT_L;
		break;
	case S_PRINTER_SENT_L:
		if (!memcmp(rdbuf, idle_data, READBACK_LEN))
			state = S_PRINTER_DONE;
		break;
	case S_PRINTER_DONE:
		/* Cleanup */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x74;
		cmdbuf[2] = 0x00;
		cmdbuf[3] = 0x50;

		if ((ret = send_data(dev, endp_down,
				    cmdbuf, CMDBUF_LEN)))
			goto done_claimed;

		state = S_FINISHED;
		break;
	default:
		break;
	};

	if (state != S_FINISHED)
		goto top;
	
	/* All done, clean up */
	ret = 0;

done_claimed:
	libusb_release_interface(dev, iface);

done_close:
	if (claimed)
		libusb_attach_kernel_driver(dev, iface);

	libusb_close(dev);
done:
	if (plane_r)
		free(plane_r);
	if (plane_b)
		free(plane_b);
	if (plane_g)
		free(plane_g);
	if (cmdbuf)
		free(cmdbuf);

	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	close(data_fd);

	return ret;
}

/* Kodak 1400/805 data format

  Spool file consists of 36-byte header followed by row-interleaved BGR data.
  Native printer resolution is 2560 pixels per row, and 3010 or 3612 rows.

  Header:

  50 47 48 44     "PGHD"
  00 0a           Number of columns, Little endian.  Fixed at 2560.
  00 00           NULL
  XX XX           Number of rows, Little Endian
  00 00           NULL
  XX XX XX XX     Number of bytes per plane, Little Endian
  00 00 00 00     NULL
  XX              00 Glossy, 01 Matte   (Note: Kodak805 only supports Glossy)
  XX              01 to laminate, 00 to not.
  01              Unkown, always set to 01
  XX              Lamination Strength:
 
                  3c  Glossy
                  28  Matte +5
                  2e  Matte +4
                  34  Matte +3
                  3a  Matte +2
                  40  Matte +1
                  46  Matte 
                  52  Matte -1
                  5e  Matte -2
                  6a  Matte -3
                  76  Matte -4
                  82  Matte -5

  00 00 00 00 00 00 00 00 00 00 00 00       NULL

  ************************************************************************

  The data format actually sent to the Kodak 1400 is rather different.

    All commands are null-padded to 96 bytes.
    All readback values are 8 bytes long.

    Multi-byte numbers are sent BIG ENDIAN.
  
    Image data is sent via planes, one scanline per URB.

 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 00 00        # Idle response

 <-- 1b 00                           # Reset/attention?
 <-- 1b 5a 53  0a 00  0b c2          # Setup (ie hdr.columns and hdr.rows)
 <-- 1b 59 01                        # ?? hdr.matte ? 
 <-- 1b 60 XX                        # hdr.lamination
 <-- 1b 62 XX                        # hdr.lam_strength
 <-- 1b 61 01                        # ?? hdr.unk1 ?

 <-- 1b 5a 54 01  00 00 00  0a 00  0b c2  # start of plane 1 data
 <-- row 1
 <-- row 2
 <-- row last

 <-- 1b 74 01 50                     # ??
 
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 50 59        # Printing plane 1
  [ repeats until...]
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 00 00        # Idle response

 <-- 1b 74 00 50                     # ??
 <-- 1b 5a 54 02  00 00 00  0a 00  0b c2  # start of plane 2 data
 <-- row 1
 <-- row 2
 <-- row last
 <-- 1b 74 01 50                     # ??

 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 50 4d        # Printing plane 2
  [ repeats until...]
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 00 00        # Idle response

 <-- 1b 74 00 50                     # ??
 <-- 1b 5a 54 03  00 00 00  0a 00  0b c2  # start of plane 3 data
 <-- row 1
 <-- row 2
 <-- row last
 <-- 1b 74 01 50                     # ??

 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 50 43        # Printing plane 3
  [ repeats until...]
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 00 00        # Idle response

 ## this block is only present if lamination is used

 <-- 1b 74 00 50                     # ??
 <-- 1b 5a 54 04                     # start of lamination
 <-- 1b 74 01 50                     # ??

 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 50 50        # Laminating
  [ repeats until...]
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 00 00        # Idle response

 ## end lamination block

 <-- 1b 74 00 50                     # ??

 [[ DONE ]]

 *********************************************
  Calibration data:

 <-- 1b a2                           # ?? Reset cal tables?
 --> 00 01 00 00  00 00 00 00        

 <-- 1b a0 02 03 06 10               # 06 10 == 1552 bytes aka the CAL data.
 <-- cal data

  [[ Data is organized as three blocks of 512 bytes followed by 
     16 NULL bytes. 

     Each block appears to be 256 entries of 16-bit LE data, 
     so each input value is translated into a 16-bit number in the printer.

     Assuming blocks are ordered BGR.

  ]] 

 --> 00 00 00 00  00 00 00 00        

*/
