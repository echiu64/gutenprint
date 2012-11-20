/*
 *   Canon SELPHY ES/CP series print assister -- libusb-1.0 version
 *
 *   (c) 2007-2012 Solomon Peachy <pizza@shaftnet.org>
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

#include <libusb-1.0/libusb.h>

#include "selphy_print_common.h"

#define STR_LEN_MAX 64
#define URI_PREFIX "selphy://"

/* USB Identifiers */
#define USB_VID_CANON       0x04a9
#define USB_PID_CANON_ES1   0x3141
#define USB_PID_CANON_ES2   0x3185
#define USB_PID_CANON_ES20  0x3186
#define USB_PID_CANON_ES3   3   // XXX 31af? 31b1??
#define USB_PID_CANON_ES30  0x31B0
#define USB_PID_CANON_ES40  0x31EE
#define USB_PID_CANON_CP10  0x304A
#define USB_PID_CANON_CP100 0x3063 // - incoming G
#define USB_PID_CANON_CP200 0x307C
#define USB_PID_CANON_CP220 0x30BD
#define USB_PID_CANON_CP300 0x307D
#define USB_PID_CANON_CP330 0x30BE
#define USB_PID_CANON_CP400 0x30F6
#define USB_PID_CANON_CP500 500 // XXX 30f5? 30f7? (related to cp400?) - incoming G
#define USB_PID_CANON_CP510 0x3128
#define USB_PID_CANON_CP520 520 // XXX 316f? 3172? (related to cp740/cp750)
#define USB_PID_CANON_CP530 530 // XXX
#define USB_PID_CANON_CP600 0x310B
#define USB_PID_CANON_CP710 0x3127
#define USB_PID_CANON_CP720 0x3143
#define USB_PID_CANON_CP730 0x3142
#define USB_PID_CANON_CP740 0x3171
#define USB_PID_CANON_CP750 0x3170
#define USB_PID_CANON_CP760 0x31AB
#define USB_PID_CANON_CP770 0x31AA
#define USB_PID_CANON_CP780 0x31DD
#define USB_PID_CANON_CP790 790 // XXX
#define USB_PID_CANON_CP800 0x3214
#define USB_PID_CANON_CP810 0x3256
#define USB_PID_CANON_CP900 0x3255

static int dump_data_libusb(int remaining, int present, int data_fd, 
			    struct libusb_device_handle *dev, 
			    uint8_t endpoint,
			    uint8_t *buf, uint16_t buflen) {
	int cnt;
	int i;
	int wrote;
	int num;

	while (remaining > 0) {
		cnt = read(data_fd, buf + present, (remaining < (buflen-present)) ? remaining : (buflen-present));
		
		if (cnt < 0)
			return -1;

		if (present) {
			cnt += present;
			present = 0;
		}

		i = libusb_bulk_transfer(dev, endpoint,
					 buf,
					 cnt,
					 &num,
					 2000);
		if (i < 0) {
			ERROR("libusb error %d: (%d/%d to 0x%02x)\n", i, num, cnt, endpoint);
			return -1;
		}

		if (num != cnt) {
			/* Realign buffer.. */
			present = cnt - num;
			memmove(buf, buf + num, present);
		}
		wrote += num;
		remaining -= num;
	}
	
	DEBUG("Wrote %d bytes\n", wrote);
	
	return wrote;
}

static int find_and_enumerate(struct libusb_context *ctx,
			      struct libusb_device ***list,
			      int printer_type,
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
		int valid = 0;
		libusb_get_device_descriptor((*list)[i], &desc);

		if (desc.idVendor != USB_VID_CANON)
			continue;

		switch(desc.idProduct) {
		case USB_PID_CANON_ES1: // "Canon SELPHY ES1"
			if (printer_type == P_ES1)
				found = i;
			valid = 1;
			break;
		case USB_PID_CANON_ES2: // "Canon SELPHY ES2"
		case USB_PID_CANON_ES20: // "Canon SELPHY ES20"
			if (printer_type == P_ES2_20)
				found = i;
			valid = 1;
			break;
		case USB_PID_CANON_ES3: // "Canon SELPHY ES3"
		case USB_PID_CANON_ES30: // "Canon SELPHY ES30"
			if (printer_type == P_ES3_30)
				found = i;
			valid = 1;
			break;
		case USB_PID_CANON_ES40: // "Canon SELPHY ES40"
		case USB_PID_CANON_CP790:
			if (printer_type == P_ES40_CP790)
				found = i;
			valid = 1;
			break;
		case USB_PID_CANON_CP10: // "Canon CP-10"
		case USB_PID_CANON_CP100: // "Canon CP-100"
		case USB_PID_CANON_CP200: // "Canon CP-200"
		case USB_PID_CANON_CP220: // "Canon CP-220"
		case USB_PID_CANON_CP300: // "Canon CP-300"
		case USB_PID_CANON_CP330: // "Canon CP-330"
		case USB_PID_CANON_CP400: // "Canon SELPHY CP400"
		case USB_PID_CANON_CP500: // "Canon SELPHY CP500"
		case USB_PID_CANON_CP510: // "Canon SELPHY CP510"
		case USB_PID_CANON_CP520: // "Canon SELPHY CP520"
		case USB_PID_CANON_CP530: // "Canon SELPHY CP530"
		case USB_PID_CANON_CP600: // "Canon SELPHY CP600"
		case USB_PID_CANON_CP710: // "Canon SELPHY CP710"
		case USB_PID_CANON_CP720: // "Canon SELPHY CP720"
		case USB_PID_CANON_CP730: // "Canon SELPHY CP730"
		case USB_PID_CANON_CP740: // "Canon SELPHY CP740"
		case USB_PID_CANON_CP750: // "Canon SELPHY CP750"
		case USB_PID_CANON_CP760: // "Canon SELPHY CP760"
		case USB_PID_CANON_CP770: // "Canon SELPHY CP770"
		case USB_PID_CANON_CP780: // "Canon SELPHY CP780"
		case USB_PID_CANON_CP800: // "Canon SELPHY CP800"
		case USB_PID_CANON_CP810: // "Canon SELPHY CP810"
		case USB_PID_CANON_CP900: // "Canon SELPHY CP900"
			if (printer_type == P_CP_XXX)
				found = i;
			valid = 1;
			break;
		default:
			/* Hook for testing unknown PIDs */
			if (getenv("SELPHY_PID") && getenv("SELPHY_TYPE")) {
				int pid = strtol(getenv("SELPHY_PID"), NULL, 16);
				int type = atoi(getenv("SELPHY_TYPE"));
				if (pid == desc.idProduct) {
					valid = 1;
					if (printer_type == type) {
						found = i;
					}
				}
			}
			break;
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

		DEBUG("%s%sPID: %04X Product: '%s' Serial: '%s'\n",
		      (!valid) ? "UNRECOGNIZED: " : "",
		      (found == i) ? "MATCH: " : "",
		      desc.idProduct, product, serial);

		// XXX MATCH based on passed-in serial number?

		if (valid && scan_only) {
			/* URL-ify model. */
			char buf[128]; // XXX ugly..
			i = 0;
			while (*(product + i + strlen("Canon"))) {
			  buf[i] = *(product + i + strlen("Canon "));
			  if(buf[i] == ' ') {
			    buf[i++] = '%';
			    buf[i++] = '2';
			    buf[i] = '0';
			  }
			  i++;
			}
			fprintf(stdout, "direct %sCanon/%s?serial=%s \"%s\" \"%s\" \"MFG:Canon;CMD:SelphyRaster;CLS:PRINTER;MDL:%s;DES:%s;SN:%s\" \"\"\n", URI_PREFIX,
			        buf, serial, product, product,
				product + strlen("Canon "), product, serial);
		}

		libusb_close(dev);
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

	int printer_type = P_END;

	int iface = 0;

	int num, i;
	int ret = 0;
	int claimed;
	int found = -1;

	uint8_t rdbuf[READBACK_LEN], rdbuf2[READBACK_LEN];
	int last_state = -1, state = S_IDLE;

	int plane_len = 0;

	int bw_mode = 0;
	int16_t paper_code_offset = -1;
	int16_t paper_code = -1;

	uint8_t buffer[BUF_LEN];

	int data_fd = fileno(stdin);

	char *uri = getenv("DEVICE_URI");;
	char *use_serno = NULL;

	/* Static initialization */
	setup_paper_codes();

	/* Cmdline help */
	if (argc < 2) {
		DEBUG("SELPHY ES/CP Print Assist version %s\nUsage:\n\t%s [ infile | - ]\n\t%s job user title num-copies options [ filename ] \n\n",
		      VERSION,
		      argv[0], argv[0]);
		libusb_init(&ctx);
		find_and_enumerate(ctx, &list, printer_type, 1);
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

	/* Figure out printer this file is intended for */
	read(data_fd, buffer, MAX_HEADER);

	printer_type = parse_printjob(buffer, &bw_mode, &plane_len);
	if (printer_type < 0) {
		ERROR("Unrecognized printjob file format!\n");
		exit(1);
	}

	DEBUG("%sFile intended for a '%s' printer\n",  bw_mode? "B/W " : "", printers[printer_type].model);

	plane_len += 12; /* Add in plane header */
	paper_code_offset = printers[printer_type].paper_code_offset;
	if (printers[printer_type].pgcode_offset != -1)
		paper_code = printers[printer_type].paper_codes[buffer[printers[printer_type].pgcode_offset]];

	/* Libusb setup */
	libusb_init(&ctx);
	found = find_and_enumerate(ctx, &list, printer_type, 0);

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

top:
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
		DEBUG("readback:  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
			rdbuf[0], rdbuf[1], rdbuf[2], rdbuf[3],
			rdbuf[4], rdbuf[5], rdbuf[6], rdbuf[7],
			rdbuf[8], rdbuf[9], rdbuf[10], rdbuf[11]);
		memcpy(rdbuf2, rdbuf, READBACK_LEN);
	} else {
		sleep(1);
	}
	if (state != last_state) {
		DEBUG("last_state %d new %d\n", last_state, state);
		last_state = state;
	}
	fflush(stderr);       

	if (!memcmp(rdbuf, printers[printer_type].error_readback, READBACK_LEN)) {
		DEBUG("error condition; aborting.  (Out of ribbon/paper?)\n");
		ret = 4;
		goto done_claimed;
	}

	switch(state) {
	case S_IDLE:
		if (!fancy_memcmp(rdbuf, printers[printer_type].init_readback, READBACK_LEN, paper_code_offset, paper_code)) {
			state = S_PRINTER_READY;
		}
		break;
	case S_PRINTER_READY:
		DEBUG("Sending init sequence (%d bytes)\n", printers[printer_type].init_length);

		/* Send printer init */
		ret = libusb_bulk_transfer(dev, endp_down,
					   buffer,
					   printers[printer_type].init_length,
					   &num,
					   2000);
		if (ret < 0) {
			ERROR("libusb error %d: (%d/%d to 0x%02x)\n", ret, num, printers[printer_type].init_length, endp_down);
			ret = 4;
			goto done_claimed;
		}

		/* Realign plane data to start of buffer.. */
		memmove(buffer, buffer+printers[printer_type].init_length,
			MAX_HEADER-printers[printer_type].init_length);

		state = S_PRINTER_INIT_SENT;
		break;
	case S_PRINTER_INIT_SENT:
		if (!fancy_memcmp(rdbuf, printers[printer_type].ready_y_readback, READBACK_LEN, paper_code_offset, paper_code)) {
			state = S_PRINTER_READY_Y;
		}
		break;
	case S_PRINTER_READY_Y:
		if (bw_mode)
			DEBUG("Sending BLACK plane\n");
		else
			DEBUG("Sending YELLOW plane\n");
		ret = dump_data_libusb(plane_len, MAX_HEADER-printers[printer_type].init_length, data_fd, dev, endp_down, buffer, BUF_LEN);
		if (ret < 0) {
			ret = 4;
			goto done_claimed;
		}
		state = S_PRINTER_Y_SENT;
		break;
	case S_PRINTER_Y_SENT:
		if (!fancy_memcmp(rdbuf, printers[printer_type].ready_m_readback, READBACK_LEN, paper_code_offset, paper_code)) {
			if (bw_mode)
				state = S_PRINTER_DONE;
			else
				state = S_PRINTER_READY_M;
		}
		break;
	case S_PRINTER_READY_M:
		DEBUG("Sending MAGENTA plane\n");
		ret = dump_data_libusb(plane_len, 0, data_fd, dev, endp_down, buffer, BUF_LEN);
		if (ret < 0) {
			ret = 4;
			goto done_claimed;
		}
		state = S_PRINTER_M_SENT;
		break;
	case S_PRINTER_M_SENT:
		if (!fancy_memcmp(rdbuf, printers[printer_type].ready_c_readback, READBACK_LEN, paper_code_offset, paper_code)) {
			state = S_PRINTER_READY_C;
		}
		break;
	case S_PRINTER_READY_C:
		DEBUG("Sending CYAN plane\n");
		ret = dump_data_libusb(plane_len, 0, data_fd, dev, endp_down, buffer, BUF_LEN);
		if (ret < 0) {
			ret = 4;
			goto done_claimed;
		}
		state = S_PRINTER_C_SENT;
		break;
	case S_PRINTER_C_SENT:
		if (!fancy_memcmp(rdbuf, printers[printer_type].done_c_readback, READBACK_LEN, paper_code_offset, paper_code)) {
			state = S_PRINTER_DONE;
		}
		break;
	case S_PRINTER_DONE:
		if (printers[printer_type].foot_length) {
			DEBUG("Sending cleanup sequence\n");
			ret = dump_data_libusb(printers[printer_type].foot_length, 0, data_fd, dev, endp_down, buffer, BUF_LEN);
			if (ret < 0) {
				ret = 4;
				goto done_claimed;
			}
		}
		state = S_FINISHED;
		/* Intentional Fallthrough */
	case S_FINISHED:
		DEBUG("All data sent to printer!\n");	
		break;
	}
	if (state != S_FINISHED)
		goto top;

	/* Done printing */

done_claimed:
	libusb_release_interface(dev, iface);

done_close:
	if (claimed)
		libusb_attach_kernel_driver(dev, iface);

	libusb_close(dev);

done:
	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	close(data_fd);

	return ret;
}
