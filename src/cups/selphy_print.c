/*
 *   Canon SELPHY ES/CP series print assister -- libusb-1.0 version
 *
 *   (c) 2007-2013 Solomon Peachy <pizza@shaftnet.org>
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

#define VERSION "0.52"
#define URI_PREFIX "canonselphy://"

#include "backend_common.c"

/* USB Identifiers */
#define USB_VID_CANON       0x04a9
#define USB_PID_CANON_ES1   0x3141
#define USB_PID_CANON_ES2   0x3185
#define USB_PID_CANON_ES20  0x3186
#define USB_PID_CANON_ES3   0x31AF
#define USB_PID_CANON_ES30  0x31B0
#define USB_PID_CANON_ES40  0x31EE
#define USB_PID_CANON_CP10  0x304A
#define USB_PID_CANON_CP100 0x3063
#define USB_PID_CANON_CP200 0x307C
#define USB_PID_CANON_CP220 0x30BD
#define USB_PID_CANON_CP300 0x307D
#define USB_PID_CANON_CP330 0x30BE
#define USB_PID_CANON_CP400 0x30F6
#define USB_PID_CANON_CP500 0x30F5
#define USB_PID_CANON_CP510 0x3128
#define USB_PID_CANON_CP520 520 // XXX 316f? 3172? (related to cp740/cp750)
#define USB_PID_CANON_CP530 0x31b1
#define USB_PID_CANON_CP600 0x310B
#define USB_PID_CANON_CP710 0x3127
#define USB_PID_CANON_CP720 0x3143
#define USB_PID_CANON_CP730 0x3142
#define USB_PID_CANON_CP740 0x3171
#define USB_PID_CANON_CP750 0x3170
#define USB_PID_CANON_CP760 0x31AB
#define USB_PID_CANON_CP770 0x31AA
#define USB_PID_CANON_CP780 0x31DD
#define USB_PID_CANON_CP790 790 // XXX 31ed? 31ef? (related to es40)
#define USB_PID_CANON_CP800 0x3214
#define USB_PID_CANON_CP810 0x3256
#define USB_PID_CANON_CP900 0x3255

#define READBACK_LEN 12

struct printer_data {
	int  type;  /* P_??? */
	char *model; /* eg "SELPHY ES1" */
	int  init_length;
	int  foot_length;
	int16_t init_readback[READBACK_LEN];
	int16_t ready_y_readback[READBACK_LEN];
	int16_t ready_m_readback[READBACK_LEN];
	int16_t ready_c_readback[READBACK_LEN];
	int16_t done_c_readback[READBACK_LEN];
	int16_t clear_error[READBACK_LEN];
	int16_t paper_codes[256];
	int16_t pgcode_offset;  /* Offset into printjob for paper type */
	int16_t paper_code_offset; /* Offset in readback for paper type */
	int16_t error_offset;
};

/* printer types */
enum {
	P_ES1 = 0,
	P_ES2_20,
	P_ES3_30,
	P_ES40_CP790,
	P_CP_XXX,
	P_CP10,
	P_END
};

struct printer_data printers[P_END] = {
	{ .type = P_ES1,
	  .model = "SELPHY ES1",
	  .init_length = 12,
	  .foot_length = 0,
	  .init_readback = { 0x02, 0x00, 0x00, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .ready_y_readback = { 0x04, 0x00, 0x01, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .ready_m_readback = { 0x04, 0x00, 0x03, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .ready_c_readback = { 0x04, 0x00, 0x07, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .done_c_readback = { 0x04, 0x00, 0x00, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  // .clear_error
	  // .paper_codes
	  .pgcode_offset = 3,
	  .paper_code_offset = 6,
	  .error_offset = 1,
	},
	{ .type = P_ES2_20,
	  .model = "SELPHY ES2/ES20",
	  .init_length = 16,
	  .foot_length = 0,
	  .init_readback = { 0x02, 0x00, 0x00, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00 },
	  .ready_y_readback = { 0x03, 0x00, 0x01, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00 },
	  .ready_m_readback = { 0x06, 0x00, 0x03, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00 },
	  .ready_c_readback = { 0x09, 0x00, 0x07, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00 },
	  .done_c_readback = { 0x09, 0x00, 0x00, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00 },
	  // .clear_error
	  // .paper_codes
	  .pgcode_offset = 2,
	  .paper_code_offset = 4,
	  .error_offset = 1,  // XXX insufficient
	},
	{ .type = P_ES3_30,
	  .model = "SELPHY ES3/ES30",
	  .init_length = 16,
	  .foot_length = 12,
	  .init_readback = { 0x00, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
	  .ready_y_readback = { 0x01, 0xff, 0x01, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
	  .ready_m_readback = { 0x03, 0xff, 0x02, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
	  .ready_c_readback = { 0x05, 0xff, 0x03, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
	  .done_c_readback = { 0x00, 0xff, 0x10, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 },
	  // .clear_error
	  // .paper_codes
	  .pgcode_offset = 2,
	  .paper_code_offset = -1,
	  .error_offset = 8, // or 10
	},
	{ .type = P_ES40_CP790,
	  .model = "SELPHY ES40/CP790",
	  .init_length = 16,
	  .foot_length = 12,
	  .init_readback = { 0x00, 0x00, -1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_y_readback = { 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_m_readback = { 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_c_readback = { 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .done_c_readback = { 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  // .clear_error
	  // .paper_codes
	  .pgcode_offset = 2,
	  .paper_code_offset = 11,
	  .error_offset = 3,
	},
	{ .type = P_CP_XXX,
	  .model = "SELPHY CP Series (!CP-10/CP790)",
	  .init_length = 12,
	  .foot_length = 0,  /* CP900 has four-byte NULL footer that can be safely ignored */
	  .init_readback = { 0x01, 0x00, 0x00, 0x00, -1, 0x00, -1, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_y_readback = { 0x02, 0x00, 0x00, 0x00, 0x70, 0x00, -1, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_m_readback = { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, -1, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_c_readback = { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, -1, 0x00, 0x00, 0x00, 0x00, -1 },
	  .done_c_readback = { 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, -1, 0x00, 0x00, 0x00, 0x00, -1 },
	  .clear_error = { 0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  // .paper_codes
	  .pgcode_offset = 3,
	  .paper_code_offset = 6,
	  .error_offset = 2,
	},
	{ .type = P_CP10,
	  .model = "SELPHY CP-10",
	  .init_length = 12,
	  .foot_length = 0,
	  .init_readback = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .ready_y_readback = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .ready_m_readback = { 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .ready_c_readback = { 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .done_c_readback = { 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  // .paper_codes
	  // .clear_error
	  .pgcode_offset = -1,
	  .paper_code_offset = -1,
	  .error_offset = 2,
	},
};

#define MAX_HEADER 28
#define BUF_LEN 4096

static const int es40_cp790_plane_lengths[4] = { 2227456, 1601600, 698880, 2976512 };

static void setup_paper_codes(void)
{
	/* Default all to IGNORE */
	int i, j;
	for (i = 0; i < P_END ; i++)
		for (j = 0 ; j < 256 ; j++) 
			printers[i].paper_codes[j] = -1;
	
	/* SELPHY ES1 paper codes */
	printers[P_ES1].paper_codes[0x11] = 0x01;
	printers[P_ES1].paper_codes[0x12] = 0x02; // ? guess
	printers[P_ES1].paper_codes[0x13] = 0x03;
	
	/* SELPHY ES2/20 paper codes */
	printers[P_ES2_20].paper_codes[0x01] = 0x01;
	printers[P_ES2_20].paper_codes[0x02] = 0x02; // ? guess
	printers[P_ES2_20].paper_codes[0x03] = 0x03;
	
	/* SELPHY ES3/30 paper codes -- N/A, printer does not report paper type */	
	/* SELPHY ES40/CP790 paper codes -- ? guess */
	printers[P_ES40_CP790].paper_codes[0x00] = 0x11;
	printers[P_ES40_CP790].paper_codes[0x01] = 0x22;
	printers[P_ES40_CP790].paper_codes[0x02] = 0x33;
	printers[P_ES40_CP790].paper_codes[0x03] = 0x44;

	/* SELPHY CP-series (except CP790) paper codes */
	printers[P_CP_XXX].paper_codes[0x01] = 0x11;
	printers[P_CP_XXX].paper_codes[0x02] = 0x22;
	printers[P_CP_XXX].paper_codes[0x03] = 0x33;
	printers[P_CP_XXX].paper_codes[0x04] = 0x44;

	/* SELPHY CP-10 paper codes -- N/A, only one type */
}

#define INCORRECT_PAPER -999

/* Program states */
enum {
	S_IDLE = 0,
	S_PRINTER_READY,
	S_PRINTER_INIT_SENT,
	S_PRINTER_READY_Y,
	S_PRINTER_Y_SENT,
	S_PRINTER_READY_M,
	S_PRINTER_M_SENT,
	S_PRINTER_READY_C,
	S_PRINTER_C_SENT,
	S_PRINTER_DONE,
	S_FINISHED,
};

static int fancy_memcmp(const uint8_t *buf_a, const int16_t *buf_b, uint len, int16_t papercode_offset, int16_t papercode_val) 
{
	int i;
  
	for (i = 0 ; i < len ; i++) {
		if (papercode_offset != -1 && i == papercode_offset) {
			if (papercode_val == -1)
				continue;
			else if (buf_a[i] != papercode_val)
				return INCORRECT_PAPER;
		} else if (buf_b[i] == -1)
			continue;
		else if (buf_a[i] > buf_b[i])
			return 1;
		else if (buf_a[i] < buf_b[i])
			return -1;
	}
	return 0;
}

static int parse_printjob(uint8_t *buffer, int *bw_mode, int *plane_len) 
{
	int printer_type = -1;

	if (buffer[0] != 0x40 &&
	    buffer[1] != 0x00) {
		goto done;
	}
	
	if (buffer[12] == 0x40 &&
	    buffer[13] == 0x01) {
		*plane_len = *(uint32_t*)(&buffer[16]);
		*plane_len = le32_to_cpu(*plane_len);

		if (buffer[2] == 0x00) {
			if (*plane_len == 688480)
				printer_type = P_CP10;
			else
				printer_type = P_CP_XXX;
		} else {
			printer_type = P_ES1;
			*bw_mode = (buffer[2] == 0x20);
		}
		goto done;
	}

	*plane_len = *(uint32_t*)(&buffer[12]);
	*plane_len = le32_to_cpu(*plane_len);

	if (buffer[16] == 0x40 &&
	    buffer[17] == 0x01) {

		if (buffer[4] == 0x02) {
			printer_type = P_ES2_20;
			*bw_mode = (buffer[7] == 0x01);
			goto done;
		}
    
		if (es40_cp790_plane_lengths[buffer[2]] == *plane_len) {
			printer_type = P_ES40_CP790; 
			*bw_mode = (buffer[3] == 0x01);
			goto done;
		} else {
			printer_type = P_ES3_30; 
			*bw_mode = (buffer[3] == 0x01);
			goto done;
		}
	}

	return -1;

done:

	return printer_type;
}

static int read_data(int remaining, int present, int data_fd, uint8_t *target,
		     uint8_t *buf, uint16_t buflen) {
	int cnt;
	int wrote = 0;

	while (remaining > 0) {
		cnt = read(data_fd, buf + present, (remaining < (buflen-present)) ? remaining : (buflen-present));
		
		if (cnt < 0)
			return -1;

		if (present) {
			cnt += present;
			present = 0;
		}

		memcpy(target + wrote, buf, cnt);

		wrote += cnt;
		remaining -= cnt;
	}
	
	return wrote;
}

static int find_and_enumerate(struct libusb_context *ctx,
			      struct libusb_device ***list,
			      char *match_serno,
			      int printer_type,
			      int scan_only)
{
	int num;
	int i;
	int found = -1;

	/* Enumerate and find suitable device */
	num = libusb_get_device_list(ctx, list);

	for (i = 0 ; i < num ; i++) {
		struct libusb_device_descriptor desc;
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
			if (printer_type == P_CP10)
				found = i;
			valid = 1;
			break;
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

		found = print_scan_output((*list)[i], &desc,
					  URI_PREFIX, "Canon", 
					  found, (found == i), valid, 
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

	int printer_type = P_END;

	int iface = 0;

	int num, i;
	int ret = 0;
	int claimed;
	int found = -1;

	uint8_t rdbuf[READBACK_LEN], rdbuf2[READBACK_LEN];
	int last_state = -1, state = S_IDLE;

	int plane_len = 0, header_len = 0, footer_len = 0;

	int bw_mode = 0;
	int16_t paper_code_offset = -1;
	int16_t paper_code = -1;

	uint8_t buffer[BUF_LEN];

	int data_fd = fileno(stdin);

	int copies = 1;
	char *uri = getenv("DEVICE_URI");;
	char *use_serno = NULL;

	uint8_t *plane_y, *plane_m, *plane_c, *footer, *header;

	/* Static initialization */
	setup_paper_codes();

	DEBUG("Canon SELPHY ES/CP CUPS backend version " VERSION "/" BACKEND_VERSION " \n");

	/* Cmdline help */
	if (argc < 2) {
		DEBUG("Usage:\n\t%s [ infile | - ]\n\t%s job user title num-copies options [ filename ] \n\n",
		      argv[0], argv[0]);
		libusb_init(&ctx);
		find_and_enumerate(ctx, &list, NULL, printer_type, 1);
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

	/* Figure out printer this file is intended for */
	read(data_fd, buffer, MAX_HEADER);

	printer_type = parse_printjob(buffer, &bw_mode, &plane_len);
	plane_len += 12; /* Add in plane header length! */
	if (printer_type < 0) {
		ERROR("Unrecognized printjob file format!\n");
		exit(1);
	}
	footer_len = printers[printer_type].foot_length;
	header_len = printers[printer_type].init_length;
	DEBUG("%sFile intended for a '%s' printer\n",  bw_mode? "B/W " : "", printers[printer_type].model);

	/* Set up buffers */
	plane_y = malloc(plane_len);
	plane_m = malloc(plane_len);
	plane_c = malloc(plane_len);
	header = malloc(header_len);
	footer = malloc(footer_len);
	if (!plane_y || !plane_m || !plane_c || !header ||
	    (footer_len && !footer)) {
		ERROR("Memory allocation failure!\n");
		exit(1);
	}

	/* Ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, sigterm_handler);

	/* Read in entire print job */
	memcpy(header, buffer, header_len);
	memmove(buffer, buffer+header_len,
		MAX_HEADER-header_len);
	read_data(plane_len, MAX_HEADER-header_len, data_fd, plane_y, buffer, BUF_LEN);
	read_data(plane_len, 0, data_fd, plane_m, buffer, BUF_LEN);
	read_data(plane_len, 0, data_fd, plane_c, buffer, BUF_LEN);
	read_data(footer_len, 0, data_fd, footer, buffer, BUF_LEN);
	close(data_fd);  /* We're done */

	/* Libusb setup */
	libusb_init(&ctx);
	found = find_and_enumerate(ctx, &list, use_serno, printer_type, 0);

	/* Compute offsets and other such things */
	paper_code_offset = printers[printer_type].paper_code_offset;
	if (printers[printer_type].pgcode_offset != -1)
		paper_code = printers[printer_type].paper_codes[buffer[printers[printer_type].pgcode_offset]];

	if (found == -1) {
		ERROR("No suitable printers found!\n");
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
			ERROR("Printer open failure (Could not detach printer)\n");
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
		ERROR("Printer open failire (Could not fetch config descriptor)\n");
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

	/* Read in the printer status */
	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   2000);
top:

	/* Do it twice to clear initial state */
	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   2000);

	if (ret < 0) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, READBACK_LEN, endp_up);
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

	/* Error detection */
	if (printers[printer_type].error_offset != -1 &&
	    rdbuf[printers[printer_type].error_offset]) {
		ERROR("Printer reported error condition %02x; aborting.  (Out of ribbon/paper?)\n", rdbuf[printers[printer_type].error_offset]);
		ret = 4;
		goto done_claimed;
	}

	switch(state) {
	case S_IDLE:
		INFO("Waiting for printer idle\n");
		if (!fancy_memcmp(rdbuf, printers[printer_type].init_readback, READBACK_LEN, paper_code_offset, paper_code)) {
			state = S_PRINTER_READY;
		}
		break;
	case S_PRINTER_READY:
		INFO("Printing started; Sending init sequence\n");
		/* Send printer init */
		if ((ret = send_data(dev, endp_down, header, header_len)))
			goto done_claimed;

		state = S_PRINTER_INIT_SENT;
		break;
	case S_PRINTER_INIT_SENT:
		if (!fancy_memcmp(rdbuf, printers[printer_type].ready_y_readback, READBACK_LEN, paper_code_offset, paper_code)) {
			state = S_PRINTER_READY_Y;
		}
		break;
	case S_PRINTER_READY_Y:
		if (bw_mode)
			INFO("Sending BLACK plane\n");
		else
			INFO("Sending YELLOW plane\n");

		if ((ret = send_data(dev, endp_down, plane_y, plane_len)))
			goto done_claimed;

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
		INFO("Sending MAGENTA plane\n");

		if ((ret = send_data(dev, endp_down, plane_m, plane_len)))
			goto done_claimed;

		state = S_PRINTER_M_SENT;
		break;
	case S_PRINTER_M_SENT:
		if (!fancy_memcmp(rdbuf, printers[printer_type].ready_c_readback, READBACK_LEN, paper_code_offset, paper_code)) {
			state = S_PRINTER_READY_C;
		}
		break;
	case S_PRINTER_READY_C:
		INFO("Sending CYAN plane\n");

		if ((ret = send_data(dev, endp_down, plane_c, plane_len)))
			goto done_claimed;

		state = S_PRINTER_C_SENT;
		break;
	case S_PRINTER_C_SENT:
		if (!fancy_memcmp(rdbuf, printers[printer_type].done_c_readback, READBACK_LEN, paper_code_offset, paper_code)) {
			state = S_PRINTER_DONE;
		}
		break;
	case S_PRINTER_DONE:
		if (footer_len) {
			INFO("Cleaning up\n");

			if ((ret = send_data(dev, endp_down, footer, footer_len)))
				goto done_claimed;
		}
		state = S_FINISHED;
		/* Intentional Fallthrough */
	case S_FINISHED:
		DEBUG("All data sent to printer!\n");	
		break;
	}
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
	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	if (plane_y)
		free(plane_y);
	if (plane_m)
		free(plane_m);
	if (plane_c)
		free(plane_c);
	if (header)
		free(header);
	if (footer)
		free(footer);


	return ret;
}

/* 

 ***************************************************************************

	Stream formats and readback codes for supported printers

 ***************************************************************************
 Selphy ES1:

   Init func:   40 00 [typeA] [pgcode]  00 00 00 00  00 00 00 00
   Plane func:  40 01 [typeB] [plane]  [length, 32-bit LE]  00 00 00 00 

   TypeA codes are 0x10 for Color papers, 0x20 for B&W papers.
   TypeB codes are 0x01 for Color papers, 0x02 for B&W papers.

   Plane codes are 0x01, 0x03, 0x07 for Y, M, and C, respectively.
   B&W Jobs have a single plane code of 0x01.

   'P' papers pgcode of 0x11 and a plane length of 2227456 bytes
   'L'        pgcode of 0x12 and a plane length of 1601600 bytes.
   'C'        pgcode of 0x13 and a plane length of  698880 bytes.

   Readback values seen:

   02 00 00 00  02 01 [pg] 01  00 00 00 00   [idle, waiting for init seq]
   04 00 00 00  02 01 [pg] 01  00 00 00 00   [init received, not ready..]
   04 00 01 00  02 01 [pg] 01  00 00 00 00   [waiting for Y data]
   04 00 03 00  02 01 [pg] 01  00 00 00 00   [waiting for M data]
   04 00 07 00  02 01 [pg] 01  00 00 00 00   [waiting for C data]
   04 00 00 00  02 01 [pg] 01  00 00 00 00   [all data sent; not ready..]
   05 00 00 00  02 01 [pg] 01  00 00 00 00   [?? transitions to this]
   06 00 00 00  02 01 [pg] 01  00 00 00 00   [?? transitions to this]
   02 00 00 00  02 01 [pg] 01  00 00 00 00   [..transitions back to idle]

   02 01 00 00  01 ff ff ff  00 80 00 00     [error, no media]
   02 01 00 00  01 ff ff ff  00 00 00 00     [error, cover open]

   Known paper types for all ES printers:  P, Pbw, L, C, Cl
   Additional types for ES3/30/40:         Pg, Ps

   [pg] is:  0x01 for P-papers
   	     0x02 for L-papers
             0x03 for C-papers

 ***************************************************************************
 Selphy ES2/20:

   Init func:   40 00 [pgcode] 00  02 00 00 [type]  00 00 00 [pg2] [length, 32-bit LE]
   Plane func:  40 01 [plane] 00  00 00 00 00  00 00 00 00 

   Type codes are 0x00 for Color papers, 0x01 for B&W papers.

   Plane codes are 0x01, 0x02, 0x03 for Y, M, and C, respectively.
   B&W Jobs have a single plane code of 0x01.

   'P' papers pgcode of 0x01 and a plane length of 2227456 bytes
   'L' 	      pgcode of 0x02 and a plane length of 1601600 bytes.
   'C'	      pgcode of 0x03 and a plane length of  698880 bytes.

   pg2 is 0x00 for all media types except for 'C', which is 0x01.

   Readback values seen on an ES2:

   02 00 00 00  [pg] 00 [pg2] [xx]  00 00 00 00   [idle, waiting for init seq]
   03 00 01 00  [pg] 00 [pg2] [xx]  00 00 00 00   [init complete, ready for Y]
   04 00 01 00  [pg] 00 [pg2] [xx]  00 00 00 00   [? paper loaded]
   05 00 01 00  [pg] 00 [pg2] [xx]  00 00 00 00   [? transitions to this]
   06 00 03 00  [pg] 00 [pg2] [xx]  00 00 00 00   [ready for M]
   08 00 03 00  [pg] 00 [pg2] [xx]  00 00 00 00   [? transitions to this]
   09 00 07 00  [pg] 00 [pg2] [xx]  00 00 00 00   [ready for C]
   09 00 00 00  [pg] 00 [pg2] 00  00 00 00 00   [? transitions to this]
   0b 00 00 00  [pg] 00 [pg2] 00  00 00 00 00   [? transisions to this]
   0c 00 00 00  [pg] 00 [pg2] 00  00 00 00 00   [? transitions to this]
   0f 00 00 00  [pg] 00 [pg2] 00  00 00 00 00   [? transitions to this]
   13 00 00 00  [pg] 00 [pg2] 00  00 00 00 00   [? transitions to this]

   14 00 00 00  [pg] 00 [pg2] 00  00 00 00 00   [out of paper/ink]
   14 00 01 00  [pg] 00 [pg2] 00  01 00 00 00   [out of paper/ink]

   16 01 00 00  [pg] 00 [pg2] 00  00 00 00 00   [error, cover open]
   02 00 00 00  05 05 02 00  00 00 00 00        [error, no media]

   [xx] can be 0x00 or 0xff, depending on if a previous print job has 
	completed or not.

   [pg] is:  0x01 for P-papers
   	     0x02 for L-papers
             0x03 for C-papers

   [pg2] is: 0x00 for P & L papers
             0x01 for Cl-paper

       *** note: may refer to Label (0x01) vs non-Label (0x00) media.

 ***************************************************************************
 Selphy ES3/30:

   Init func:   40 00 [pgcode] [type]  00 00 00 00  00 00 00 00 [length, 32-bit LE]
   Plane func:  40 01 [plane] 00  00 00 00 00  00 00 00 00 

   End func:    40 20 00 00  00 00 00 00  00 00 00 00

   Type codes are 0x00 for Color papers, 0x01 for B&W papers.

   Plane codes are 0x01, 0x02, 0x03 for Y, M, and C, respectively.
   B&W Jobs have a single plane code of 0x01.

   'P' papers pgcode of 0x01 and a plane length of 2227456 bytes.
   'L' 	      pgcode of 0x02 and a plane length of 1601600 bytes.
   'C' 	      pgcode of 0x03 and a plane length of  698880 bytes.

   Readback values seen on an ES3 & ES30:

   00 ff 00 00  ff ff ff ff  00 00 00 00   [idle, waiting for init seq]
   01 ff 01 00  ff ff ff ff  00 00 00 00   [init complete, ready for Y]
   03 ff 01 00  ff ff ff ff  00 00 00 00   [?]
   03 ff 02 00  ff ff ff ff  00 00 00 00   [ready for M]
   05 ff 02 00  ff ff ff ff  00 00 00 00   [?]
   05 ff 03 00  ff ff ff ff  00 00 00 00   [ready for C]
   07 ff 03 00  ff ff ff ff  00 00 00 00   [?]
   0b ff 03 00  ff ff ff ff  00 00 00 00   [?]
   13 ff 03 00  ff ff ff ff  00 00 00 00   [?]
   00 ff 10 00  ff ff ff ff  00 00 00 00   [ready for footer]

   00 ff 00 00  ff ff ff ff  00 00 00 00   [cover open, no media]

   00 ff 01 00  ff ff ff ff  03 00 02 00   [attempt to print with no media]
   00 ff 01 00  ff ff ff ff  08 00 04 00   [attempt to print with cover open]

   There appears to be no paper code in the readback; codes were identical for
   the standard 'P-Color' and 'Cl' cartridges:

 ***************************************************************************
 Selphy ES40:

   Init func:   40 00 [pgcode] [type]  00 00 00 00  00 00 00 00 [length, 32-bit LE]
   Plane func:  40 01 [plane] 00  00 00 00 00  00 00 00 00 

   End func:    40 20 00 00  00 00 00 00  00 00 00 00

   Type codes are 0x00 for Color papers, 0x01 for B&W papers.

   Plane codes are 0x01, 0x02, 0x03 for Y, M, and C, respectively.
   B&W Jobs have a single plane code of 0x01.

   'P' papers pgcode of 0x00 and a plane length of 2227456 bytes.
   'L' 	      pgcode of 0x01 and a plane length of 1601600 bytes.
   'C'	      pgcode of 0x02 and a plane length of  698880 bytes.

   Readback values seen on an ES40:

   00 00 ff 00  00 00 00 00  00 00 00 [pg]
   00 00 00 00  00 00 00 00  00 00 00 [pg]   [idle, ready for header]
   00 01 01 00  00 00 00 00  00 00 00 [pg]   [ready for Y data]
   00 03 01 00  00 00 00 00  00 00 00 [pg]   [transitions to this]
   00 03 02 00  00 00 00 00  00 00 00 [pg]   [ready for M data]
   00 05 02 00  00 00 00 00  00 00 00 [pg]   [transitions to this]
   00 05 03 00  00 00 00 00  00 00 00 [pg]   [ready for C data]
   00 07 03 00  00 00 00 00  00 00 00 [pg]   [transitions to this]
   00 0b ff 00  00 00 00 00  00 00 00 [pg]   [transitions to this]
   00 0e ff 00  00 00 00 00  00 00 00 [pg]   [transitions to this]
   00 00 10 00  00 00 00 00  00 00 00 [pg]   [ready for footer]

   00 ** ** [xx]  00 00 00 00  00 00 00 [pg] [error]

   [xx]:
	01:  Generic communication error
	32:  Cover open / media empty

   [pg] is as follows:

      'P' paper 0x11
      'L' paper 0x22
      'C' paper 0x33
      'W' paper 0x44


 ***************************************************************************
 Selphy CP790:

   Init func:   40 00 [pgcode] 00  00 00 00 00  00 00 00 00 [length, 32-bit LE]
   Plane func:  40 01 [plane] 00  00 00 00 00  00 00 00 00 

   End func:    40 20 00 00  00 00 00 00  00 00 00 00

   Plane codes are 0x01, 0x02, 0x03 for Y, M, and C, respectively.

   'P' papers pgcode of 0x00 and a plane length of 2227456 bytes.
   'L' 	      pgcode of 0x01 and a plane length of 1601600 bytes.
   'C'	      pgcode of 0x02 and a plane length of  698880 bytes.
   'W' 	      pgcode of 0x03 and a plane length of 2976512 bytes.

   Readback codes are completely unknown, but are likely to be the same
   as the ES40.

 ***************************************************************************
 Selphy CP-10:

   Init func:   40 00 00 00  00 00 00 00  00 00 00 00
   Plane func:  40 01 00 [plane]  [length, 32-bit LE]  00 00 00 00 

   plane codes are 0x00, 0x01, 0x02 for Y, M, and C, respectively.

   length is always '00 60 81 0a' which is 688480 bytes.

   Known readback values:

   01 00 00 00  00 00 00 00  00 00 00 00   [idle, waiting for init]
   02 00 00 00  00 00 00 00  00 00 00 00   [init sent, paper feeding]
   02 00 00 00  00 00 00 00  00 00 00 00   [init sent, paper feeding] 
   02 00 00 00  00 00 00 00  00 00 00 00   [waiting for Y data]
   04 00 00 00  00 00 00 00  00 00 00 00   [waiting for M data]
   08 00 00 00  00 00 00 00  00 00 00 00   [waiting for C data]
   10 00 00 00  00 00 00 00  00 00 00 00   [C done, waiting]
   20 00 00 00  00 00 00 00  00 00 00 00   [All done]

   02 00 80 00  00 00 00 00  00 00 00 00   [No ink]
   02 00 01 00  00 00 00 00  00 00 00 00   [No media]

  There are no media type codes; the printer only supports one type.

 ***************************************************************************
 Selphy CP-series (except for CP790 & CP-10):
 
    This is known to apply to:
	CP-100, CP-200, CP-300, CP-330, CP400, CP500, CP510, CP710,
	CP720, CP730, CP740, CP750, CP760, CP770, CP780, CP800, CP900

   Init func:   40 00 00 [pgcode]  00 00 00 00  00 00 00 00
   Plane func:  40 01 00 [plane]  [length, 32-bit LE]  00 00 00 00 
   End func:    00 00 00 00      # NOTE:  CP900 only, and not necessary!

   Error clear: 40 10 00 00  00 00 00 00  00 00 00 00  # CP800.  Others?

   plane codes are 0x00, 0x01, 0x02 for Y, M, and C, respectively.

   'P' papers pgcode 0x01   plane length 2227456 bytes.
   'L' 	      pgcode 0x02   plane length 1601600 bytes.
   'C' 	      pgcode 0x03   plane length  698880 bytes.
   'W'	      pgcode 0x04   plane length 2976512 bytes.

   Known readback values:

   01 00 00 00  [ss] 00 [pg] 00  00 00 00 [xx]   [idle, waiting for init]
   02 00 [rr] 00  00 00 [pg] 00  00 00 00 [xx]   [init sent, paper feeding]
   02 00 [rr] 00  10 00 [pg] 00  00 00 00 [xx]   [init sent, paper feeding] 
   02 00 [rr] 00  70 00 [pg] 00  00 00 00 [xx]   [waiting for Y data]
   04 00 00 00  00 00 [pg] 00  00 00 00 [xx]   [waiting for M data]
   08 00 00 00  00 00 [pg] 00  00 00 00 [xx]   [waiting for C data]
   10 00 00 00  00 00 [pg] 00  00 00 00 [xx]   [C done, waiting]
   20 00 00 00  00 00 [pg] 00  00 00 00 [xx]   [All done]

   [xx] is 0x01 on the CP780/CP800/CP900, 0x00 on all others.

   [rr] is error code:
   	0x00 no error
	0x01 paper out
	0x04 ribbon problem
	0x08 ribbon depleted

   [ss] is either 0x00 or 0x70.  Unsure as to its significance; perhaps it
	means paper or ribbon is already set to go?

   [pg] is as follows:

      'P' paper 0x11
      'L' paper 0x22
      'C' paper 0x33
      'W' paper 0x44

      First four bits are paper, second four bits are the ribbon.  They aren't
      necessarily identical.  So it's possible to have a code of, say,
      0x41 if the 'Wide' paper tray is loaded with a 'P' ribbon. A '0' is used
      to signify nothing being loaded.
 

*/
