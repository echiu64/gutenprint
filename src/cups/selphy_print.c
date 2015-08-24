/*
 *   Canon SELPHY ES/CP series CUPS backend -- libusb-1.0 version
 *
 *   (c) 2007-2015 Solomon Peachy <pizza@shaftnet.org>
 *
 *   The latest version of this program can be found at:
 *
 *     http://git.shaftnet.org/cgit/selphy_print.git
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

#define BACKEND canonselphy_backend

#include "backend_common.h"

#define P_ES40_CP790 (P_END + 1) // used for detection only

/* Exported */
#define USB_VID_CANON       0x04a9
#define USB_PID_CANON_CP10  0x304A
#define USB_PID_CANON_CP100 0x3063
#define USB_PID_CANON_CP200 0x307C
#define USB_PID_CANON_CP220 0x30BD
#define USB_PID_CANON_CP300 0x307D
#define USB_PID_CANON_CP330 0x30BE
#define USB_PID_CANON_CP400 0x30F6
#define USB_PID_CANON_CP500 0x30F5
#define USB_PID_CANON_CP510 0x3128
#define USB_PID_CANON_CP520 0x3172
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
#define USB_PID_CANON_CP790 0x31E7
#define USB_PID_CANON_CP800 0x3214
#define USB_PID_CANON_CP810 0x3256
#define USB_PID_CANON_CP900 0x3255
#define USB_PID_CANON_ES1   0x3141
#define USB_PID_CANON_ES2   0x3185
#define USB_PID_CANON_ES20  0x3186
#define USB_PID_CANON_ES3   0x31AF
#define USB_PID_CANON_ES30  0x31B0
#define USB_PID_CANON_ES40  0x31EE

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
	uint8_t clear_error[READBACK_LEN];
	int     clear_error_len;
	int16_t paper_codes[256];
	int16_t pgcode_offset;  /* Offset into printjob for paper type */
	int16_t paper_code_offset; /* Offset in readback for paper type */
	int  (*error_detect)(uint8_t *rdbuf);
};

static int es1_error_detect(uint8_t *rdbuf)
{
	if (rdbuf[1] == 0x01) {
		if (rdbuf[9] == 0x00) {
			ERROR("Cover open!\n");
		} else {
			ERROR("Unknown error %02x\n", rdbuf[9]);
		}
		return 1;
	} else if (rdbuf[4] == 0x01 && rdbuf[5] == 0xff &&
		   rdbuf[6] == 0xff && rdbuf[7] == 0xff) {
		ERROR("No media loaded!\n");
		return 1;
	} else if (rdbuf[0] == 0x0f) {
		ERROR("Out of media!\n");
		return 1;
	}

	return 0;
}

static int es2_error_detect(uint8_t *rdbuf)
{
	if (rdbuf[0] == 0x16 &&
	    rdbuf[1] == 0x01) {
		ERROR("Printer cover open!\n");
		return 1;
	}
		
	if (rdbuf[0] == 0x02 &&
	    rdbuf[4] == 0x05 &&
	    rdbuf[5] == 0x05 &&
	    rdbuf[6] == 0x02) {
		ERROR("No media loaded!\n");
		return 1;
	}

	if (rdbuf[0] == 0x14) {
		ERROR("Out of media!\n");
		return 1;
	}

	return 0;
}

static int es3_error_detect(uint8_t *rdbuf)
{
	if (rdbuf[8] == 0x01) {
		if (rdbuf[10] == 0x0f) {
			ERROR("Communications Error\n");
		} else if (rdbuf[10] == 0x01) {
			ERROR("No media loaded!\n");
		} else {
			ERROR("Unknown error - %02x + %02x\n", 
			      rdbuf[8], rdbuf[10]);
		}
		return 1;
	} else if (rdbuf[8] == 0x03 &&
		   rdbuf[10] == 0x02) {
		ERROR("No media loaded!\n");
		return 1;
	} else if (rdbuf[8] == 0x08 &&
		   rdbuf[10] == 0x04) {
		ERROR("Printer cover open!\n");
		return 1;
	} else if (rdbuf[8] == 0x05 &&
		   rdbuf[10] == 0x01) {
		ERROR("Incorrect media loaded!\n");
		return 1;
	}

	if (rdbuf[8] || rdbuf[10]) {
		ERROR("Unknown error - %02x + %02x\n", 
		      rdbuf[8], rdbuf[10]);
		return 1;
	}
	
	return 0;
}

static int es40_error_detect(uint8_t *rdbuf)
{
	/* ES40 */
	if (!rdbuf[3])
		return 0;
	
	if (rdbuf[3] == 0x01)
		ERROR("Generic communication error\n");
	else if (rdbuf[3] == 0x32)
		ERROR("Cover open or media empty!\n");
	else
		ERROR("Unknown error - %02x\n", rdbuf[3]);

	return 1;
}

static int cp790_error_detect(uint8_t *rdbuf)
{
	/* CP790 */
	if (rdbuf[5] == 0xff) {
		ERROR("No ribbon loaded!\n");
		return 1;
	} else if (rdbuf[4] == 0xff) {
		ERROR("No paper tray loaded!\n");
		return 1;
	} else if (rdbuf[3]) {
		if ((rdbuf[3] & 0xf) == 0x02)  // 0x12 0x22
			ERROR("No paper tray loaded!\n");
		else if ((rdbuf[3] & 0xf) == 0x03)  // 0x13 0x23
			ERROR("Empty paper tray or feed error!\n");
		else if (rdbuf[3] == 0x11)
			ERROR("Paper feed error!\n");
		else if (rdbuf[3] == 0x21)
			ERROR("Ribbon depleted!\n");
		else
			ERROR("Unknown error - %02x\n", rdbuf[3]);
		return 1;
	}

	return 0;
}


static int cp10_error_detect(uint8_t *rdbuf)
{
	if (!rdbuf[2])
		return 0;

	if (rdbuf[2] == 0x80)
		ERROR("No ribbon loaded\n");
	else if (rdbuf[2] == 0x08)
		ERROR("Ribbon depleted!\n");
	else if (rdbuf[2] == 0x01)
		ERROR("No paper loaded!\n");
	else
		ERROR("Unknown error - %02x\n", rdbuf[2]);
	return 1;
}

static int cpxxx_error_detect(uint8_t *rdbuf)
{
	if (!rdbuf[2])
		return 0;

	if (rdbuf[2] == 0x01)
		ERROR("Paper feed problem!\n");
	else if (rdbuf[2] == 0x04)
		ERROR("Ribbon problem!\n");
	else if (rdbuf[2] == 0x08)
		ERROR("Ribbon depleted!\n");
	else
		ERROR("Unknown error - %02x\n", rdbuf[2]);
	return 1;
}

static struct printer_data selphy_printers[] = {
	{ .type = P_ES1,
	  .model = "SELPHY ES1",
	  .init_length = 12,
	  .foot_length = 0,
	  .init_readback = { 0x02, 0x00, 0x00, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .ready_y_readback = { 0x04, 0x00, 0x01, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .ready_m_readback = { 0x04, 0x00, 0x03, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .ready_c_readback = { 0x04, 0x00, 0x07, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .done_c_readback = { 0x04, 0x00, 0x00, 0x00, 0x02, 0x01, -1, 0x01, 0x00, 0x00, 0x00, 0x00 },
	  .clear_error = { 0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .clear_error_len = 12,
	  .pgcode_offset = 3,
	  .paper_code_offset = 6,
	  .error_detect = es1_error_detect,
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
	  .clear_error = { 0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .clear_error_len = 12,
	  .pgcode_offset = 2,
	  .paper_code_offset = 4,
	  .error_detect = es2_error_detect,
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
	  .clear_error = { 0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .clear_error_len = 12,
	  .pgcode_offset = 2,
	  .paper_code_offset = -1,
	  .error_detect = es3_error_detect,
	},
	{ .type = P_ES40,
	  .model = "SELPHY ES40",
	  .init_length = 16,
	  .foot_length = 12,
	  .init_readback = { 0x00, 0x00, -1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_y_readback = { 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_m_readback = { 0x00, 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .ready_c_readback = { 0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .done_c_readback = { 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, -1 },
	  .clear_error = { 0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .clear_error_len = 12,
	  .pgcode_offset = 2,
	  .paper_code_offset = 11,
	  .error_detect = es40_error_detect,
	},
	{ .type = P_CP790,
	  .model = "SELPHY CP790",
	  .init_length = 16,
	  .foot_length = 12,
	  .init_readback = { 0x00, 0x00, -1, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 },
	  .ready_y_readback = { 0x00, 0x01, 0x01, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 },
	  .ready_m_readback = { 0x00, 0x03, 0x02, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 },
	  .ready_c_readback = { 0x00, 0x05, 0x03, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 },
	  .done_c_readback = { 0x00, 0x00, 0x10, 0x00, -1, -1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 },
	  .clear_error = { 0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .clear_error_len = 12,
	  // .paper_codes
	  .pgcode_offset = 2,
	  .paper_code_offset = -1, /* Uses a different technique */
	  .error_detect = cp790_error_detect,
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
	  .clear_error_len = 12,
	  // .paper_codes
	  .pgcode_offset = 3,
	  .paper_code_offset = 6,
	  .error_detect = cpxxx_error_detect,
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
	  .clear_error = { 0x40, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	  .clear_error_len = 12,
	  // .paper_codes
	  .pgcode_offset = -1,
	  .paper_code_offset = -1,
	  .error_detect = cp10_error_detect,
	},
	{ .type = -1 },
};

#define MAX_HEADER 28

static const uint32_t es40_cp790_plane_lengths[4] = { 2227456, 1601600, 698880, 2976512 };

static void setup_paper_codes(void)
{
	int i, j;
	for (i = 0 ; ; i++) {
		if (selphy_printers[i].type == -1)
			break;
		/* Default all to IGNORE */
		for (j = 0 ; j < 256 ; j++) 
			selphy_printers[i].paper_codes[j] = -1;

		/* Set up specifics */
		switch (selphy_printers[i].type) {
		case P_ES1:
			selphy_printers[i].paper_codes[0x11] = 0x01;
			selphy_printers[i].paper_codes[0x12] = 0x02;
			selphy_printers[i].paper_codes[0x13] = 0x03;
			break;
		case P_ES2_20:
			selphy_printers[i].paper_codes[0x01] = 0x01;
			selphy_printers[i].paper_codes[0x02] = 0x02;
			selphy_printers[i].paper_codes[0x03] = 0x03;
			break;
		case P_ES40:
			selphy_printers[i].paper_codes[0x00] = 0x11;
			selphy_printers[i].paper_codes[0x01] = 0x22;
			selphy_printers[i].paper_codes[0x02] = 0x33;
			selphy_printers[i].paper_codes[0x03] = 0x44;
			break;
		case P_CP_XXX:
			selphy_printers[i].paper_codes[0x01] = 0x11;
			selphy_printers[i].paper_codes[0x02] = 0x22;
			selphy_printers[i].paper_codes[0x03] = 0x33;
			selphy_printers[i].paper_codes[0x04] = 0x44;
			break;
		case P_ES3_30:
			/* N/A, printer does not report types */
		case P_CP790:
			/* N/A, printer uses different technique */
		case P_CP10:
			/* N/A, printer supports one type only */
			break;
		}
	}
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
	S_PRINTER_CP900_FOOTER,
	S_PRINTER_DONE,
	S_FINISHED,
};

static int fancy_memcmp(const uint8_t *buf_a, const int16_t *buf_b, uint len) 
{
	uint i;
  
	for (i = 0 ; i < len ; i++) {
		if (buf_b[i] == -1)
			continue;
		else if (buf_a[i] > buf_b[i])
			return 1;
		else if (buf_a[i] < buf_b[i])
			return -1;
	}
	return 0;
}

static int parse_printjob(uint8_t *buffer, uint8_t *bw_mode, uint32_t *plane_len) 
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

done:
	return printer_type;
}

/* Private data stucture */
struct canonselphy_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	struct printer_data *printer;

	uint8_t bw_mode;

	int16_t paper_code;

	uint32_t plane_len;

	uint8_t *header;
	uint8_t *plane_y;
	uint8_t *plane_m;
	uint8_t *plane_c;
	uint8_t *footer;

	uint8_t *buffer;

	uint8_t cp900;
};

static void *canonselphy_init(void)
{
	struct canonselphy_ctx *ctx = malloc(sizeof(struct canonselphy_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct canonselphy_ctx));

	/* Static initialization */
	setup_paper_codes();

	ctx->buffer = malloc(MAX_HEADER);
	if (!ctx->buffer) {
		ERROR("Memory Allocation Failure!\n");
		free(ctx);
		ctx = NULL;
	}

	return ctx;
}

extern struct dyesub_backend canonselphy_backend;

static void canonselphy_attach(void *vctx, struct libusb_device_handle *dev, 
			       uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct canonselphy_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);

	ctx->type = lookup_printer_type(&canonselphy_backend,
					desc.idVendor, desc.idProduct);

	if (desc.idProduct == USB_PID_CANON_CP900)
		ctx->cp900 = 1;
}

static void canonselphy_teardown(void *vctx) {
	struct canonselphy_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->header)
		free(ctx->header);
	if (ctx->plane_y)
		free(ctx->plane_y);
	if (ctx->plane_m)
		free(ctx->plane_m);
	if (ctx->plane_c)
		free(ctx->plane_c);
	if (ctx->footer)
		free(ctx->footer);

	if (ctx->buffer)
		free(ctx->buffer);

	free(ctx);
}

static int canonselphy_early_parse(void *vctx, int data_fd)
{
	struct canonselphy_ctx *ctx = vctx;
	int printer_type, i;

	if (!ctx)
		return -1;

	/* Figure out printer this file is intended for */
	i = read(data_fd, ctx->buffer, MAX_HEADER);
	if (i != MAX_HEADER) {
		if (i == 0)
			return -1;
		ERROR("Read failed (%d/%d/%d)\n", 
		      i, 0, MAX_HEADER);
		perror("ERROR: Read failed");
		return -1;
	}

	printer_type = parse_printjob(ctx->buffer, &ctx->bw_mode, &ctx->plane_len);
	/* Special cases for some models */
	if (printer_type == P_ES40_CP790) {
		if (ctx->type == P_CP790)
			printer_type = P_CP790;
		else
			printer_type = P_ES40;			
	}
	
	/* Look up the printer entry */
	for (i = 0; selphy_printers[i].type != -1; i++) {
		if (selphy_printers[i].type == printer_type) {
			ctx->printer = &selphy_printers[i];
			break;
		}
	}
	if (!ctx->printer) {
		ERROR("Unrecognized printjob file format!\n");
		return -1;
	}

	INFO("%sFile intended for a '%s' printer\n",  ctx->bw_mode? "B/W " : "", ctx->printer->model);

	if (ctx->printer->type != ctx->type) {
		ERROR("Printer/Job mismatch (%d/%d)\n", ctx->type, ctx->printer->type);
		return -1;
	}

	ctx->plane_len += 12; /* Add in plane header length! */
	if (ctx->printer->pgcode_offset != -1)
		ctx->paper_code = ctx->printer->paper_codes[ctx->buffer[ctx->printer->pgcode_offset]];
	else
		ctx->paper_code = -1;

	return printer_type;
}

static int canonselphy_read_parse(void *vctx, int data_fd)
{
	struct canonselphy_ctx *ctx = vctx;
	int i, remain;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Perform early parsing */
	i = canonselphy_early_parse(ctx, data_fd);
	if (i < 0)
		return CUPS_BACKEND_FAILED;

	if (ctx->header) {
		free(ctx->header);
		ctx->header = NULL;
	}
	if (ctx->plane_y) {
		free(ctx->plane_y);
		ctx->plane_y = NULL;
	}
	if (ctx->plane_m) {
		free(ctx->plane_m);
		ctx->plane_m = NULL;
	}
	if (ctx->plane_c) {
		free(ctx->plane_c);
		ctx->plane_c = NULL;
	}
	if (ctx->footer) {
		free(ctx->footer);
		ctx->footer = NULL;
	}

	/* Set up buffers */
	ctx->plane_y = malloc(ctx->plane_len);
	ctx->plane_m = malloc(ctx->plane_len);
	ctx->plane_c = malloc(ctx->plane_len);
	ctx->header = malloc(ctx->printer->init_length);
	ctx->footer = malloc(ctx->printer->foot_length);
	if (!ctx->plane_y || !ctx->plane_m || !ctx->plane_c || !ctx->header ||
	    (ctx->printer->foot_length && !ctx->footer)) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_FAILED;
	}

	/* Move over chunks already read in */
	memcpy(ctx->header, ctx->buffer, ctx->printer->init_length);
	memcpy(ctx->plane_y, ctx->buffer+ctx->printer->init_length, 
	       MAX_HEADER-ctx->printer->init_length);

	/* Read in YELLOW plane */
	remain = ctx->plane_len - (MAX_HEADER-ctx->printer->init_length);
	while (remain > 0) {
		i = read(data_fd, ctx->plane_y + (ctx->plane_len - remain), remain);
		if (i < 0)
			return CUPS_BACKEND_CANCEL;
		remain -= i;
	}

	/* Read in MAGENTA plane */
	remain = ctx->plane_len;
	while (remain > 0) {
		i = read(data_fd, ctx->plane_m + (ctx->plane_len - remain), remain);
		if (i < 0)
			return CUPS_BACKEND_CANCEL;
		remain -= i;
	}

	/* Read in CYAN plane */
	remain = ctx->plane_len;
	while (remain > 0) {
		i = read(data_fd, ctx->plane_c + (ctx->plane_len - remain), remain);
		if (i < 0)
			return CUPS_BACKEND_CANCEL;
		remain -= i;
	}

	/* Read in footer */
	if (ctx->printer->foot_length) {
		remain = ctx->printer->foot_length;
		while (remain > 0) {
			i = read(data_fd, ctx->footer + (ctx->printer->foot_length - remain), remain);
			if (i < 0)
				return CUPS_BACKEND_CANCEL;
			remain -= i;
		}
	}

	return CUPS_BACKEND_OK;
}

static int canonselphy_main_loop(void *vctx, int copies) {
	struct canonselphy_ctx *ctx = vctx;

	uint8_t rdbuf[READBACK_LEN], rdbuf2[READBACK_LEN];
	int last_state = -1, state = S_IDLE;
	int ret, num;

	/* Read in the printer status to clear last state */
	ret = read_data(ctx->dev, ctx->endp_up,
			rdbuf, READBACK_LEN, &num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;

top:

	if (state != last_state) {
		if (dyesub_debug)
			DEBUG("last_state %d new %d\n", last_state, state);
	}

	/* Read in the printer status */
	ret = read_data(ctx->dev, ctx->endp_up,
			rdbuf, READBACK_LEN, &num);
	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	if (num != READBACK_LEN) {
		ERROR("Short read! (%d/%d)\n", num, READBACK_LEN);
		return CUPS_BACKEND_FAILED;
	}

	/* Error detection */
	if (ctx->printer->error_detect(rdbuf)) {
		if (ctx->printer->clear_error_len)
			/* Try to clear error state */
			if ((ret = send_data(ctx->dev, ctx->endp_down, ctx->printer->clear_error, ctx->printer->clear_error_len)))
				return CUPS_BACKEND_FAILED;
		return CUPS_BACKEND_HOLD;
	}

	if (memcmp(rdbuf, rdbuf2, READBACK_LEN)) {
		memcpy(rdbuf2, rdbuf, READBACK_LEN);
	} else if (state == last_state) {
		sleep(1);
	}
	last_state = state;

	fflush(stderr);

	switch(state) {
	case S_IDLE:
		INFO("Waiting for printer idle\n");
		if (fancy_memcmp(rdbuf, ctx->printer->init_readback, READBACK_LEN))
			break;
		
		/* Make sure paper/ribbon is correct */
		if (ctx->paper_code != -1) {
			if (ctx->type == P_CP_XXX) {
				uint8_t pc = rdbuf[ctx->printer->paper_code_offset];
				if (((pc >> 4) & 0xf) != (ctx->paper_code & 0x0f)) {

					if (pc & 0xf0) {
						ERROR("Incorrect paper tray loaded, aborting job!\n");
						return CUPS_BACKEND_HOLD;
					} else {
						ERROR("No paper tray loaded, aborting!\n");
						return CUPS_BACKEND_STOP;
					}
				}
				if ((pc & 0xf) != (ctx->paper_code & 0xf)) {
					if (pc & 0x0f) {
						ERROR("Incorrect ribbon loaded, aborting job!\n");
						return CUPS_BACKEND_HOLD;
					} else {

						ERROR("No ribbon loaded, aborting job!\n");
						return CUPS_BACKEND_STOP;
					}
				}
			} else {
				if (rdbuf[ctx->printer->paper_code_offset] !=
				    ctx->paper_code) {
					ERROR("Incorrect media/ribbon loaded (%02x vs %02x), aborting job!\n", 
					      ctx->paper_code,
					      rdbuf[ctx->printer->paper_code_offset]);
					return CUPS_BACKEND_HOLD;  /* Hold this job, don't stop queue */
				}
			}
		} else if (ctx->type == P_CP790) {
			uint8_t ribbon = rdbuf[4] >> 4;
			uint8_t paper = rdbuf[5];

			if (ribbon == 0xf) {
				ERROR("No ribbon loaded, aborting!\n");
				return CUPS_BACKEND_STOP;	
			} else if (ribbon != ctx->paper_code) {
				ERROR("Incorrect ribbon loaded, aborting job!\n");
				return CUPS_BACKEND_HOLD;
			}
			if (paper == 0xf) {
				ERROR("No paper tray loaded, aborting!\n");
				return CUPS_BACKEND_STOP;
			} else if (paper != ctx->paper_code) {
				ERROR("Incorrect paper loaded, aborting job!\n");
				return CUPS_BACKEND_HOLD;
			}
		}

		state = S_PRINTER_READY;
		break;
	case S_PRINTER_READY:
		INFO("Printing started; Sending init sequence\n");
		/* Send printer init */
		if ((ret = send_data(ctx->dev, ctx->endp_down, ctx->header, ctx->printer->init_length)))
			return CUPS_BACKEND_FAILED;

		state = S_PRINTER_INIT_SENT;
		break;
	case S_PRINTER_INIT_SENT:
		if (!fancy_memcmp(rdbuf, ctx->printer->ready_y_readback, READBACK_LEN)) {
			state = S_PRINTER_READY_Y;
		}
		break;
	case S_PRINTER_READY_Y:
		if (ctx->bw_mode)
			INFO("Sending BLACK plane\n");
		else
			INFO("Sending YELLOW plane\n");

		if ((ret = send_data(ctx->dev, ctx->endp_down, ctx->plane_y, ctx->plane_len)))
			return CUPS_BACKEND_FAILED;

		state = S_PRINTER_Y_SENT;
		break;
	case S_PRINTER_Y_SENT:
		if (!fancy_memcmp(rdbuf, ctx->printer->ready_m_readback, READBACK_LEN)) {
			if (ctx->bw_mode)
				state = S_PRINTER_DONE;
			else
				state = S_PRINTER_READY_M;
		}
		break;
	case S_PRINTER_READY_M:
		INFO("Sending MAGENTA plane\n");

		if ((ret = send_data(ctx->dev, ctx->endp_down, ctx->plane_m, ctx->plane_len)))
			return CUPS_BACKEND_FAILED;

		state = S_PRINTER_M_SENT;
		break;
	case S_PRINTER_M_SENT:
		if (!fancy_memcmp(rdbuf, ctx->printer->ready_c_readback, READBACK_LEN)) {
			state = S_PRINTER_READY_C;
		}
		break;
	case S_PRINTER_READY_C:
		INFO("Sending CYAN plane\n");

		if ((ret = send_data(ctx->dev, ctx->endp_down, ctx->plane_c, ctx->plane_len)))
			return CUPS_BACKEND_FAILED;

		state = S_PRINTER_C_SENT;
		break;
	case S_PRINTER_C_SENT:
		if (!fancy_memcmp(rdbuf, ctx->printer->done_c_readback, READBACK_LEN)) {
			if (ctx->cp900)
				state = S_PRINTER_CP900_FOOTER;
			else 
				state = S_PRINTER_DONE;
		}
		break;
	case S_PRINTER_CP900_FOOTER: {
		uint32_t empty = 0;

		INFO("Sending CP900 Footer\n");
		if ((ret = send_data(ctx->dev, ctx->endp_down, 
				     (uint8_t*)&empty, sizeof(empty))))
			return CUPS_BACKEND_FAILED;

		state = S_PRINTER_DONE;
		break;
	}
	case S_PRINTER_DONE:
		if (ctx->printer->foot_length) {
			INFO("Cleaning up\n");

			if ((ret = send_data(ctx->dev, ctx->endp_down, ctx->footer, ctx->printer->foot_length)))
				return CUPS_BACKEND_FAILED;
		}
		state = S_FINISHED;
		/* Intentional Fallthrough */
	case S_FINISHED:
		INFO("All data sent to printer!\n");	
		break;
	}
	if (state != S_FINISHED)
		goto top;

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		state = S_IDLE;
		goto top;
	}

	return CUPS_BACKEND_OK;
}

static int canonselphy_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct canonselphy_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;
	
	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL)) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		}

		if (j) return j;
	}

	return 0;
}

struct dyesub_backend canonselphy_backend = {
	.name = "Canon SELPHY CP/ES",
	.version = "0.89",
	.uri_prefix = "canonselphy",
	.cmdline_arg = canonselphy_cmdline_arg,
	.init = canonselphy_init,
	.attach = canonselphy_attach,
	.teardown = canonselphy_teardown,
	.read_parse = canonselphy_read_parse,
	.main_loop = canonselphy_main_loop,
	.devices = {
	{ USB_VID_CANON, USB_PID_CANON_CP10, P_CP10, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP100, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP200, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP220, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP300, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP330, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP400, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP500, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP510, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP520, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP530, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP600, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP710, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP720, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP730, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP740, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP750, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP760, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP770, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP780, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP790, P_CP790, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP800, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP810, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_CP900, P_CP_XXX, ""},
	{ USB_VID_CANON, USB_PID_CANON_ES1, P_ES1, ""},
	{ USB_VID_CANON, USB_PID_CANON_ES2, P_ES2_20, ""},
	{ USB_VID_CANON, USB_PID_CANON_ES20, P_ES2_20, ""},
	{ USB_VID_CANON, USB_PID_CANON_ES3, P_ES3_30, ""},
	{ USB_VID_CANON, USB_PID_CANON_ES30, P_ES3_30, ""},
	{ USB_VID_CANON, USB_PID_CANON_ES40, P_ES40, ""},
	{ 0, 0, 0, ""}
	}
};
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
   02 00 00 00  01 ff ff ff  00 00 00 00     [error, cover open]
   0f 00 00 00  02 01 01 01  00 00 00 00     [error, out of media]

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

   01 ff 10 00  ff ff ff ff  01 00 0f 00   [communication error]
   00 ff 01 00  ff ff ff ff  01 00 01 00   [error, no media/ink]
   00 ff 01 00  ff ff ff ff  05 00 01 00   [error, incorrect media]
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

   Reset func:  40 10 00 00  00 00 00 00  00 00 00 00

   Plane codes are 0x01, 0x02, 0x03 for Y, M, and C, respectively.

   'P' papers pgcode of 0x00 and a plane length of 2227456 bytes.
   'L' 	      pgcode of 0x01 and a plane length of 1601600 bytes.
   'C'	      pgcode of 0x02 and a plane length of  698880 bytes.
   'W' 	      pgcode of 0x03 and a plane length of 2976512 bytes.

   Readback values seen on an CP790:

   00 00 ff 00  [pg1] [pg2] 00 00  00 00 00 02
   00 00 00 00  [pg1] [pg2] 00 00  00 00 00 02   [idle, ready for header]
   00 00 01 00  [pg1] [pg2] 00 00  00 00 00 02   
   00 01 01 00  [pg1] [pg2] 00 00  00 00 00 02   [ready for Y data]
   00 03 01 00  [pg1] [pg2] 00 00  00 00 00 02   [transitions to this]
   00 03 02 00  [pg1] [pg2] 00 00  00 00 00 02   [ready for M data]
   00 05 02 00  [pg1] [pg2] 00 00  00 00 00 02   [transitions to this]
   00 05 03 00  [pg1] [pg2] 00 00  00 00 00 02   [ready for C data]
   00 0b ff 00  [pg1] [pg2] 00 00  00 00 00 02   [transitions to this]
   00 0e ff 00  [pg1] [pg2] 00 00  00 00 00 02   [transitions to this]
   00 00 10 00  [pg1] [pg2] 00 00  00 00 00 02   [ready for footer]

   [pg1] is:                  [pg2] is:

      0x00  'P' ribbon         0x00 'P' paper
      0x10  'L' ribbon         0x01 'L' paper
      0x20  'C' ribbon         0x02 'C' paper
      0x30  'W' ribbon         0x03 'W' paper
      0xff  NO RIBBON          0xff  NO PAPER TRAY

   Other readbacks seen:

   00 00 01 11  [pg1] [pg2] 00 00  00 00 00 02   [emptytray, ink match job ]
   00 00 01 12  [pg1] [pg2] 00 00  00 00 00 02   [ notray, ink match job ]
   00 00 01 13  [pg1] [pg2] 00 00  00 00 00 02   [ empty tray + mismatch ink ]
   00 00 01 21  [pg1] [pg2] 00 00  00 00 00 02   [ depleted ribbon, match ink ]
   00 00 01 22  [pg1] [pg2] 00 00  00 00 00 02   [ no paper tray ]
   00 00 01 23  [pg1] [pg2] 00 00  00 00 00 02   [ empty tray, ink mismatch ]

    Note : These error conditions are confusing.

 ***************************************************************************
 Selphy CP-10:

   Init func:   40 00 00 00  00 00 00 00  00 00 00 00
   Plane func:  40 01 00 [plane]  [length, 32-bit LE]  00 00 00 00 

   plane codes are 0x00, 0x01, 0x02 for Y, M, and C, respectively.

   length is always '00 60 81 0a' which is 688480 bytes.

   Error clear: 40 10 00 00  00 00 00 00  00 00 00 00

   Known readback values:

   01 00 00 00  00 00 00 00  00 00 00 00   [idle, waiting for init]
   02 00 00 00  00 00 00 00  00 00 00 00   [init sent, paper feeding]
   02 00 00 00  00 00 00 00  00 00 00 00   [init sent, paper feeding] 
   02 00 00 00  00 00 00 00  00 00 00 00   [waiting for Y data]
   04 00 00 00  00 00 00 00  00 00 00 00   [waiting for M data]
   08 00 00 00  00 00 00 00  00 00 00 00   [waiting for C data]
   10 00 00 00  00 00 00 00  00 00 00 00   [C done, waiting]
   20 00 00 00  00 00 00 00  00 00 00 00   [All done]

   02 00 80 00  00 00 00 00  00 00 00 00   [No ribbon]
   02 00 80 00  00 00 00 00  00 00 00 00   [Ribbon depleted]
   02 00 01 00  00 00 00 00  00 00 00 00   [No paper]

  There are no media type codes; the printer only supports one type.

 ***************************************************************************
 Selphy CP-series (except for CP790 & CP-10):
 
    This is known to apply to:
	CP-100, CP-200, CP-300, CP-330, CP400, CP500, CP510, CP710,
	CP720, CP730, CP740, CP750, CP760, CP770, CP780, CP800, CP900

   Init func:   40 00 00 [pgcode]  00 00 00 00  00 00 00 00
   Plane func:  40 01 00 [plane]  [length, 32-bit LE]  00 00 00 00 
   End func:    00 00 00 00      # NOTE:  CP900 only, and not necessary!

   Error clear: 40 10 00 00  00 00 00 00  00 00 00 00

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

 ***************************************************************************
 Selphy CP820/CP910:

  Radically different spool file format!  300dpi, same print sizes, but also
  adding a 50x50mm sticker and 22x17.3mm ministickers, though I think the
  driver treats all of those as 'C' sizes for printing purposes.

  Printer does *not* apparently require use of a spooler!

  32-byte header:

  0f 00 00 40 00 00 00 00  00 00 00 00 00 00 01 00
  01 00 ?? 00 00 00 00 00  XX 04 00 00 WW ZZ 00 00

  ?? == 50  (P)
     == 4c  (L)
     == 43  (C)

  XX == e0  (P)
        80  (L)
        40  (C)

  WW == 50  (P)
        c0  (L)
        9c  (C)

  ZZ == 07  (P)
        05  (L)
        02  (C)

  P == 7008800  == 2336256 * 3 + 32 (1872*1248)
  L == 5087264  == 1695744 * 3 + 32 (1536*1104)
  C == 2180384  == 726784 * 3 + 32  (1088*668)

*/
