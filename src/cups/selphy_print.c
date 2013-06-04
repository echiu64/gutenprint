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

#include <libusb-1.0/libusb.h>

#define STR_LEN_MAX 64
#define URI_PREFIX "selphy://"

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

#define VERSION "0.46"

#define DEBUG( ... ) fprintf(stderr, "DEBUG: " __VA_ARGS__ )
#define INFO( ... )  fprintf(stderr, "INFO: " __VA_ARGS__ )
#define ERROR( ... ) do { fprintf(stderr, "ERROR: " __VA_ARGS__ ); sleep(1); } while (0)

#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define le32_to_cpu(__x) __x
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
#endif

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

#define ID_BUF_SIZE 2048
static char *get_device_id(struct libusb_device_handle *dev)
{
	int   length;
	int claimed = 0;
	int iface = 0;
	char *buf = malloc(ID_BUF_SIZE + 1);

	claimed = libusb_kernel_driver_active(dev, iface);
	if (claimed)
		libusb_detach_kernel_driver(dev, iface);

	libusb_claim_interface(dev, iface);

	if (libusb_control_transfer(dev,
				    LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_ENDPOINT_IN |
				    LIBUSB_RECIPIENT_INTERFACE,
				    0, 0,
				    (iface << 8),
				    (unsigned char *)buf, ID_BUF_SIZE, 5000) < 0)
	{
		*buf = '\0';
		goto done;
	}

	/* length is the first two bytes, MSB first */
	length = (((unsigned)buf[0] & 255) << 8) |
		((unsigned)buf[1] & 255);

	/* Sanity checks */
	if (length > ID_BUF_SIZE || length < 14)
		length = (((unsigned)buf[1] & 255) << 8) |
			((unsigned)buf[0] & 255);
	
	if (length > ID_BUF_SIZE)
		length = ID_BUF_SIZE;
	
	if (length < 14) {
		*buf = '\0';
		goto done;
	}

	/* Move, and terminate */
	memmove(buf, buf + 2, length);
	buf[length] = '\0';

done:
	libusb_release_interface(dev, iface);
	if (claimed)
		libusb_attach_kernel_driver(dev, iface);

	return buf;
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

		if (!strlen((char*)serial))
			strcpy((char*)serial, "NONE");

		DEBUG("%s%sPID: %04X Product: '%s' Serial: '%s'\n",
		      (!valid) ? "UNRECOGNIZED: " : "",
		      (found == i) ? "MATCH: " : "",
		      desc.idProduct, product, serial);

		if (valid && scan_only) {
			/* URL-ify model. */
			char buf[128]; // XXX ugly..
			int j = 0;
			char *ieee_id;
			while (*(product + j + strlen("Canon"))) {
				buf[j] = *(product + j + strlen("Canon "));
				if(buf[j] == ' ') {
					buf[j++] = '%';
					buf[j++] = '2';
					buf[j] = '0';
				}
				j++;
			}
			ieee_id = get_device_id(dev);

			fprintf(stdout, "direct %sCanon/%s?serial=%s \"%s\" \"%s\" \"%s\" \"\"\n", URI_PREFIX,
			        buf, serial, product, product,
				ieee_id);

			if (ieee_id)
				free(ieee_id);
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

static int terminate = 0;

void sigterm_handler(int signum) {
	terminate = 1;
	INFO("Job Cancelled");
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

	/* Cmdline help */
	if (argc < 2) {
		DEBUG("SELPHY ES/CP Print Assist version %s\nUsage:\n\t%s [ infile | - ]\n\t%s job user title num-copies options [ filename ] \n\n",
		      VERSION,
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
	plane_len += 12; /* Plane header length */
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
		if (!fancy_memcmp(rdbuf, printers[printer_type].init_readback, READBACK_LEN, paper_code_offset, paper_code)) {
			state = S_PRINTER_READY;
		}
		break;
	case S_PRINTER_READY:
		DEBUG("Sending init sequence (%d bytes)\n", header_len);

		INFO("Printing started\n");

		/* Send printer init */
		ret = libusb_bulk_transfer(dev, endp_down,
					   header,
					   header_len,
					   &num,
					   2000);
		if (ret < 0) {
			ERROR("Failure to send data to printer (libusb error %d: (%d/%d to 0x%02x))\n", ret, num, header_len, endp_down);
			ret = 4;
			goto done_claimed;
		}

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
		ret = libusb_bulk_transfer(dev, endp_down,
					   plane_y,
					   plane_len,
					   &num,
					   10000);
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
		ret = libusb_bulk_transfer(dev, endp_down,
					   plane_m,
					   plane_len,
					   &num,
					   10000);
		if (ret < 0) {
			ERROR("Failure to send data to printer (libusb error %d: (%d/%d to 0x%02x))\n", ret, num, footer_len, endp_down);
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
		ret = libusb_bulk_transfer(dev, endp_down,
					   plane_c,
					   plane_len,
					   &num,
					   10000);
		if (ret < 0) {
			ERROR("Failure to send data to printer (libusb error %d: (%d/%d to 0x%02x))\n", ret, num, footer_len, endp_down);
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
		if (footer_len) {
			DEBUG("Sending cleanup sequence\n");

			ret = libusb_bulk_transfer(dev, endp_down,
						   footer,
						   footer_len,
						   &num,
						   2000);
			if (ret < 0) {
				ERROR("Failure to send data to printer (libusb error %d: (%d/%d to 0x%02x))\n", ret, num, footer_len, endp_down);
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
