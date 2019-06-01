/*
 *   Magicard card printer family CUPS backend -- libusb-1.0 version
 *
 *   (c) 2017-2018 Solomon Peachy <pizza@shaftnet.org>
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *          [http://www.gnu.org/licenses/gpl-2.0.html]
 *
 *   SPDX-License-Identifier: GPL-2.0+
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#define BACKEND magicard_backend

#include "backend_common.h"

/* Exported */
#define USB_VID_MAGICARD     0x0C1F
#define USB_PID_MAGICARD_TANGO2E 0x1800
#define USB_PID_MAGICARD_ENDURO  0x4800   // ??
#define USB_PID_MAGICARD_ENDUROPLUS 0x880A // ??

/* Gamma tables computed with this perl program:

  my $input_bpp = 8;
  my $output_bpp = 6;
  my $gamma = 1/1.8;  # or 1/2.2 or whatever.

  my $i;

  for (my $i = 0 ; $i < (2 ** $input_bpp) ; $i++) {
    my $linear = $i / (2 ** $input_bpp);
    my $gc = ($linear ** $gamma) * (2 ** $output_bpp);
    $gc = int($gc);
    print "$gc, ";
  }

*/

static uint8_t gammas[2][256] = {
	/* Gamma = 2.2 */
	{
		 0,  5,  7,  8,  9, 10, 11, 12, 13, 13, 14, 15, 15, 16, 17,
		17, 18, 18, 19, 19, 20, 20, 20, 21, 21, 22, 22, 23, 23, 23,
		24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28,
		29, 29, 29, 29, 30, 30, 30, 31, 31, 31, 31, 32, 32, 32, 32,
		33, 33, 33, 33, 34, 34, 34, 34, 35, 35, 35, 35, 35, 36, 36,
		36, 36, 37, 37, 37, 37, 37, 38, 38, 38, 38, 38, 39, 39, 39,
		39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41, 42, 42, 42,
		42, 42, 43, 43, 43, 43, 43, 43, 44, 44, 44, 44, 44, 45, 45,
		45, 45, 45, 45, 46, 46, 46, 46, 46, 46, 47, 47, 47, 47, 47,
		47, 48, 48, 48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 49, 50,
		50, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51, 51, 51, 52, 52,
		52, 52, 52, 52, 52, 53, 53, 53, 53, 53, 53, 53, 54, 54, 54,
		54, 54, 54, 54, 55, 55, 55, 55, 55, 55, 55, 56, 56, 56, 56,
		56, 56, 56, 56, 57, 57, 57, 57, 57, 57, 57, 57, 58, 58, 58,
		58, 58, 58, 58, 58, 59, 59, 59, 59, 59, 59, 59, 59, 60, 60,
		60, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61, 61, 61, 61, 62,
		62, 62, 62, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 63, 63, 63,
	},
	/* Gamma = 1.8 */
	{
		  0,  2,  4,  5,  6,  7,  7,  8,  9,  9, 10, 11, 11, 12, 12,
		 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 17, 18, 18, 19,
		 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23, 24,
		 24, 24, 24, 25, 25, 25, 26, 26, 26, 26, 27, 27, 27, 28, 28,
		 28, 28, 29, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 31, 32,
		 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 34, 34, 35, 35, 35,
		 35, 36, 36, 36, 36, 36, 37, 37, 37, 37, 37, 38, 38, 38, 38,
		 39, 39, 39, 39, 39, 40, 40, 40, 40, 40, 41, 41, 41, 41, 41,
		 42, 42, 42, 42, 42, 42, 43, 43, 43, 43, 43, 44, 44, 44, 44,
		 44, 45, 45, 45, 45, 45, 45, 46, 46, 46, 46, 46, 47, 47, 47,
		 47, 47, 47, 48, 48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 49,
		 50, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51, 51, 52, 52, 52,
		 52, 52, 52, 53, 53, 53, 53, 53, 53, 54, 54, 54, 54, 54, 54,
		 55, 55, 55, 55, 55, 55, 55, 56, 56, 56, 56, 56, 56, 57, 57,
		 57, 57, 57, 57, 57, 58, 58, 58, 58, 58, 58, 58, 59, 59, 59,
		 59, 59, 59, 60, 60, 60, 60, 60, 60, 60, 61, 61, 61, 61, 61,
		 61, 61, 62, 62, 62, 62, 62, 62, 62, 63, 63, 63, 63, 63, 63, 63,
	}
};

struct magicard_printjob {
	uint8_t *databuf;
	int datalen;

	int hdr_len;
	int copies;
};

/* Private data structure */
struct magicard_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	struct marker marker;
};

struct magicard_cmd_header {
	uint8_t guard[9];  /* 0x05 */
	uint8_t guard2[1]; /* 0x01 */
	uint8_t cmd[4]; /* 'REQ,' */
	uint8_t subcmd[4]; /* '???,' */
	uint8_t arg[4]; /* '???,' */
	uint8_t footer[2]; /* 0x1c 0x03 */
};

struct magicard_cmd_simple_header {
	uint8_t guard[9];  /* 0x05 */
	uint8_t guard2[1]; /* 0x01 */
	uint8_t cmd[]; /* '???' */
//	uint8_t footer[2]; /* 0x1c 0x03 */
};

struct magicard_resp_header {
	uint8_t guard[1]; /* 0x01 */
	uint8_t subcmd_arg[7]; /* '???,???' */
	uint8_t data[0]; /* freeform resp */
//	uint8_t term[2]; /* 0x2c 0x03 terminates! */
};

struct magicard_requests {
	char *key;
	char *desc;
	uint8_t type;
};

enum {
	TYPE_UNKNOWN = 0,
	TYPE_STRING,
	TYPE_STRINGINT,
	TYPE_STRINGINT_HEX,
	TYPE_IPADDR,
	TYPE_YESNO,
	TYPE_MODEL,
};

/* Data definitions */
static struct magicard_requests magicard_sta_requests[] = {
	{ "MSR", "Printer Serial Number", TYPE_STRING },
	{ "PSR", "Print Head Serial Number", TYPE_STRING },
	{ "BSR", "PCB Serial Number", TYPE_STRING },
	{ "VRS", "Firmware Version", TYPE_STRING },
	{ "FDC", "Head Density", TYPE_STRINGINT },  /* 25 per step */
	{ "FSP", "Image Start", TYPE_STRINGINT }, /* 8 steps per pixel */
	{ "FEP", "Image End", TYPE_STRINGINT },   /* 8 steps per pixel */
	{ "FSS", "Ramp Adjust", TYPE_STRINGINT },
	{ "FPP", "Head Position", TYPE_STRINGINT }, /* L-R alignment */
	{ "MDL", "Model", TYPE_MODEL },  /* 0 == Standard.  Others? */
	{ "PID", "USB PID", TYPE_STRINGINT_HEX }, /* ASCII integer, but needs to be shown as hex */
	{ "VID", "USB VID", TYPE_STRINGINT_HEX }, /* ASCII integer, but needs to be shown as hex */
	{ "USN", "USB Serial Number", TYPE_STRING },
	{ "UPN", "USB Manufacturer", TYPE_STRING },
	{ "MAC", "Ethernet MAC Address", TYPE_STRING },
	{ "DYN", "Dynamic Address", TYPE_YESNO }, /* 1 == yes, 0 == no */
	{ "IPA", "IP Address", TYPE_IPADDR },  /* ASCII signed integer */
	{ "SNM", "IP Netmask", TYPE_IPADDR },  /* ASCII signed integer */
	{ "GWY", "IP Gateway", TYPE_IPADDR },  /* ASCII signed integer */

	{ "TCQ", "Total Cards Printed", TYPE_STRINGINT },
	{ "TCP", "Prints on Head", TYPE_STRINGINT },
	{ "TCN", "Cleaning Cycles", TYPE_STRINGINT },
	{ "CCQ", "Cards Since Last Cleaning", TYPE_STRINGINT },
	{ "TPQ", "Total Panels Printed", TYPE_STRINGINT },
	{ "CCP", "Cards between Cleaning Prompts", TYPE_STRINGINT },
	{ "CPQ", "Panels Since Last Cleaning", TYPE_STRINGINT },
	{ "DFR", "Panels Remaining", TYPE_STRINGINT },  // cook somehow?
	{ "CLP", "Cleaning Prompt", TYPE_STRING },

	// CRQ:  OFF  ??  Cleaning overdue?
	// CHK:  checksum of fw?  (8 chars, hex?)
	// TES:  ??? signed int?  IP addr?
	// RAMP:  ??? hangs.

	{ NULL, NULL, 0 }
};

// Sensors: CAM1 CAM2 TACHO FLIP DYE BARCODE LID FRONT REAR BUTTON TEMP ON OFF
// Languages: ENG ITA POR FRA DEU ESP SCH

/* Helper functions */
static int magicard_build_cmd(uint8_t *buf,
			       char *cmd, char *subcmd, char *arg)
{
	struct magicard_cmd_header *hdr = (struct magicard_cmd_header *) buf;

	memset(hdr->guard, 0x05, sizeof(hdr->guard));
	hdr->guard2[0] = 0x01;
	memcpy(hdr->cmd, cmd, 3);
	hdr->cmd[3] = ',';
	memcpy(hdr->subcmd, subcmd, 3);
	hdr->subcmd[3] = ',';
	memcpy(hdr->arg, arg, 3);
	hdr->arg[3] = ',';
	hdr->footer[0] = 0x1c;
	hdr->footer[1] = 0x03;

	return sizeof(*hdr);
}

static int magicard_build_cmd_simple(uint8_t *buf,
				     char *cmd)
{
	struct magicard_cmd_simple_header *hdr = (struct magicard_cmd_simple_header *) buf;
	int len = strlen(cmd);

	memset(hdr->guard, 0x05, sizeof(hdr->guard));
	hdr->guard2[0] = 0x01;
	strncpy((char*)hdr->cmd, cmd, len);
	hdr->cmd[len] = 0x1c;
	hdr->cmd[len+1] = 0x03;

	return (sizeof(*hdr) + len + 2);
}


static uint8_t * magicard_parse_resp(uint8_t *buf, uint16_t len, uint16_t *resplen)
{
	struct magicard_resp_header *hdr = (struct magicard_resp_header *) buf;

	*resplen = len - sizeof(hdr->guard) - sizeof(hdr->subcmd_arg) - 2;

	return hdr->data;
}

static int magicard_query_sensors(struct magicard_ctx *ctx)
{
	int ret = 0;
	int i;
	uint8_t buf[256];
	char buf2[24];

	for (i = 1 ; ; i++) {
		int num = 0;

		snprintf(buf2, sizeof(buf2), "SNR%d", i);
		ret = magicard_build_cmd_simple(buf, buf2);

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     buf, ret)))
			return ret;

		memset(buf, 0, sizeof(buf));

		ret = read_data(ctx->dev, ctx->endp_up,
				buf, sizeof(buf), &num);

		if (ret < 0)
			return ret;

		if (!memcmp(buf, "END", 3))
			break;

		buf[num] = 0;
		INFO("%s\n", buf);
	}
	return 0;
}

static int magicard_selftest_card(struct magicard_ctx *ctx)
{
	int ret = 0;
	uint8_t buf[256];

	ret = magicard_build_cmd_simple(buf, "TST,");

	ret = send_data(ctx->dev, ctx->endp_down,
			buf, ret);
	return ret;
}

static int magicard_reset(struct magicard_ctx *ctx)
{
	int ret = 0;
	uint8_t buf[256];

	ret = magicard_build_cmd_simple(buf, "RST,");

	ret = send_data(ctx->dev, ctx->endp_down,
			buf, ret);
	return ret;
}

static int magicard_eject(struct magicard_ctx *ctx)
{
	int ret = 0;
	uint8_t buf[256];

	ret = magicard_build_cmd_simple(buf, "EJT,");

	ret = send_data(ctx->dev, ctx->endp_down,
			buf, ret);
	return ret;
}

static int magicard_query_printer(struct magicard_ctx *ctx)
{
	int ret = 0;
	int i;
	uint8_t buf[256];
	char buf2[24];

	for (i = 1 ; ; i++) {
		int num = 0;

		snprintf(buf2, sizeof(buf2), "QPR%d", i);
		ret = magicard_build_cmd_simple(buf, buf2);

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     buf, ret)))
			return ret;

		memset(buf, 0, sizeof(buf));

		ret = read_data(ctx->dev, ctx->endp_up,
				buf, sizeof(buf), &num);

		if (ret < 0)
			return ret;

		if (!memcmp(buf, "END", 3))
			break;

		buf[num] = 0;
		INFO("%s\n", buf);
	}
	return 0;
}

static int magicard_query_status(struct magicard_ctx *ctx)
{
	int ret = 0;
	int i;
	uint8_t buf[256];

	for (i = 0 ; ; i++) {
		uint16_t resplen = 0;
		uint8_t *resp;
		int num = 0;

		if (magicard_sta_requests[i].key == NULL)
			break;

		ret = magicard_build_cmd(buf, "REQ", "STA",
				   magicard_sta_requests[i].key);

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     buf, ret)))
			return ret;

		memset(buf, 0, sizeof(buf));

		ret = read_data(ctx->dev, ctx->endp_up,
				buf, sizeof(buf), &num);

		if (ret < 0)
			return ret;

		resp = magicard_parse_resp(buf, num, &resplen);
		resp[resplen] = 0;
		switch(magicard_sta_requests[i].type) {
		case TYPE_IPADDR: {
			int32_t ipaddr;
			uint8_t *addr = (uint8_t *) &ipaddr;
			ipaddr = atoi((char*)resp);
			INFO("%s:\t%d.%d.%d.%d\n",
			     magicard_sta_requests[i].desc,
			     addr[3], addr[2], addr[1], addr[0]);
			     break;
		}
		case TYPE_YESNO: {
			int val = atoi((char*)resp);
			INFO("%s:\t%s\n",
			     magicard_sta_requests[i].desc,
			     val? "Yes" : "No");
			break;
		}
		case TYPE_MODEL: {
			int val = atoi((char*)resp);
			INFO("%s:\t%s\n",
			     magicard_sta_requests[i].desc,
			     val == 0? "Standard" : "Unknown");
			break;
		}
		case TYPE_STRINGINT_HEX: {
			int val = atoi((char*)resp);
			INFO("%s:\t%X\n",
			     magicard_sta_requests[i].desc,
			     val);
			break;
		}
		case TYPE_STRINGINT:
			// treat differently?
		case TYPE_STRING:
		case TYPE_UNKNOWN:
		default:
			INFO("%s:\t%s\n",
			     magicard_sta_requests[i].desc,
			     resp);
		}
	}

	return ret;
}

/* Main driver */
static void* magicard_init(void)
{
	struct magicard_ctx *ctx = malloc(sizeof(struct magicard_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct magicard_ctx));
	return ctx;
}

static int magicard_attach(void *vctx, struct libusb_device_handle *dev, int type,
			   uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct magicard_ctx *ctx = vctx;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;
	ctx->type = type;

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";  // XXX YMCK too!
	ctx->marker.name = "Unknown"; // LC1/LC3/LC6/LC8
	ctx->marker.levelmax = -1;
	ctx->marker.levelnow = -2;

	return CUPS_BACKEND_OK;
}

static void magicard_cleanup_job(const void *vjob)
{
	const struct magicard_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);

	free((void*)job);
}

static void downscale_and_extract(int gamma, uint32_t pixels,
				  uint8_t *y_i, uint8_t *m_i, uint8_t *c_i,
				  uint8_t *y_o, uint8_t *m_o, uint8_t *c_o, uint8_t *k_o)
{
	uint32_t i;

	for (i = 0 ; i < pixels; i++)
	{
		uint8_t y, m, c;
		uint8_t k = 0;
		uint32_t j;
		uint32_t row;
		uint32_t col;
		uint32_t b_offset;
		uint8_t b_shift;

		/* Downscale color planes from 8bpp -> 6bpp; */
		if (gamma) {
			if (gamma > 2)
				gamma = 2;
			gamma--;
			y = gammas[gamma][*y_i++];
			m = gammas[gamma][*m_i++];
			c = gammas[gamma][*c_i++];
		} else {
			y = *y_i++ >> 2;
			m = *m_i++ >> 2;
			c = *c_i++ >> 2;
		}

		/* Extract "true black" from ymc data, if enabled */
		if (k_o && y == 0x3f && m == 0x3f && c == 0x3f) {
			k = 1;
			y = m = c = 0;
		}

		/* Compute row number and offsets */
		row = i / 672;
		col = i - (row * 672);
		b_offset = col / 8;
		b_shift = 7 - (col - (b_offset * 8));

		/* Now, for each row, break it down into sub-chunks */
		for (j = 0 ; j < 6 ; j++) {
			if (b_shift == 7) {
				y_o[row * 504 + j * 84 + b_offset] = 0;
				m_o[row * 504 + j * 84 + b_offset] = 0;
				c_o[row * 504 + j * 84 + b_offset] = 0;
			}
			if (y & (1 << j))
				y_o[row * 504 + j * 84 + b_offset] |= (1 << b_shift);
			if (m & (1 << j))
				m_o[row * 504 + j * 84 + b_offset] |= (1 << b_shift);
			if (c & (1 << j))
				c_o[row * 504 + j * 84 + b_offset] |= (1 << b_shift);
		}

		/* And resin black, if enabled */
		if (k_o) {
			if (b_shift == 7) {
				k_o[row * 84 + b_offset] = 0;
			}
			if (k)
				k_o[row * 84 + b_offset] |= (1 << b_shift);
		}
	}
}

#define MAX_HEADERS_LEN 2048
#define MAX_PRINTJOB_LEN (1016*672*4) + MAX_HEADERS_LEN  /* 1016*672 * 4color */
#define INITIAL_BUF_LEN 1024
static int magicard_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct magicard_ctx *ctx = vctx;
	uint8_t initial_buf[INITIAL_BUF_LEN + 1];
	uint32_t buf_offset = 0;
	int i;

	uint8_t *in_y, *in_m, *in_c;
	uint8_t *out_y, *out_m, *out_c, *out_k;
	uint32_t len_y = 0, len_m = 0, len_c = 0, len_k = 0;
	int gamma = 0;

	uint8_t x_gp_8bpp;
	uint8_t x_gp_rk;
	uint8_t k_only;

	struct magicard_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));
	job->copies = copies;

	/* Read in the first chunk */
	i = read(data_fd, initial_buf, INITIAL_BUF_LEN);
	if (i < 0) {
		magicard_cleanup_job(job);
		return i;
	} else if (i == 0) {
		magicard_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;  /* Ie no data, we're done */
	} else if (i < INITIAL_BUF_LEN) {
		magicard_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Basic Sanity Check */
	if (initial_buf[0] != 0x05 ||
	    initial_buf[64] != 0x01 ||
	    initial_buf[65] != 0x2c) {
		ERROR("Unrecognized header data format @%d!\n", job->datalen);
		magicard_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	initial_buf[INITIAL_BUF_LEN] = 0;

	/* We can start allocating! */
	if (job->databuf) {
		free(job->databuf);
		job->databuf = NULL;
	}
	job->datalen = 0;
	job->databuf = malloc(MAX_PRINTJOB_LEN);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		magicard_cleanup_job(job);
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	/* Copy over initial header */
	memcpy(job->databuf + job->datalen, initial_buf + buf_offset, 65);
	job->datalen += 65;
	buf_offset += 65;

	/* Start parsing headers */
	x_gp_8bpp = x_gp_rk = k_only = job->hdr_len = 0;

	char *ptr;
	ptr = strtok((char*)initial_buf + ++buf_offset, ",\x1c");
	while (ptr
	       && ((ptr - (char*)initial_buf) < INITIAL_BUF_LEN)
	       && ((ptr - (char*)initial_buf) + strnlen(ptr, INITIAL_BUF_LEN) < INITIAL_BUF_LEN)
	       && *ptr != 0x1c) {
		if (!strcmp("X-GP-8", ptr)) {
			x_gp_8bpp = 1;
		} else if (!strncmp("TDT", ptr, 3)) {
			/* Strip out the timestamp, replace it with one from the backend */
		} else if (!strncmp("IMF", ptr,3)) {
			/* Strip out the image format, replace it with backend */
//		} else if (!strncmp("ESS", ptr, 3)) {
//			/* Strip out copies */
		} else if (!strcmp("X-GP-RK", ptr)) {
			x_gp_rk = 1;
		} else if (!strncmp("ICC", ptr,3)) {
			/* Gamma curve is not handled by printer,
			   strip it out and use it! */
			gamma = atoi(ptr + 3);
		} else if (!strncmp("SZ", ptr, 2)) {
			if (ptr[2] == 'B') {
				len_y = atoi(ptr + 3);
			} else if (ptr[2] == 'G') {
				len_m = atoi(ptr + 3);
			} else if (ptr[2] == 'R') {
				len_c = atoi(ptr + 3);
			} else if (ptr[2] == 'K') {
				len_k = atoi(ptr + 3);
			}
		} else {
			/* Safety valve */
			if (strlen(ptr) + job->datalen > MAX_HEADERS_LEN) {
				ERROR("headers too long, bogus job!\n");
				magicard_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}

			/* Everything else goes in */
			job->datalen += sprintf((char*)job->databuf + job->datalen, ",%s", ptr);
		}

		/* Keep going */
		buf_offset += strlen(ptr) + 1;
		/* Peek ahead to see if this is it */
		if (initial_buf[buf_offset + 1] == 0x1c)
			break;
		/* Otherwise continue to the next token */
		ptr = strtok(NULL, ",\x1c");
	}

	/* Sanity checks */
	if (!len_y || !len_m || !len_c) {
		ERROR("Plane lengths missing? %u/%u/%u!\n", len_y, len_m, len_c);
		magicard_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}
	if (len_y != len_m || len_y != len_c) {
		ERROR("Inconsistent data plane lengths! %u/%u/%u!\n", len_y, len_m, len_c);
		magicard_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}
	if (x_gp_rk && len_k) {
		ERROR("Data stream already has a K layer!\n");
		magicard_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Generate a timestamp */
	job->datalen += sprintf((char*)job->databuf + job->datalen, ",TDT%08X", (uint32_t) time(NULL));

	/* Generate image format tag */
	if (k_only == 1) {
		job->datalen += sprintf((char*)job->databuf + job->datalen, ",IMFK");
	} else if (x_gp_rk || len_k) {
		/* We're adding K, so make this BGRK */
		job->datalen += sprintf((char*)job->databuf + job->datalen, ",IMFBGRK");
	} else {
		/* Just BGR */
		job->datalen += sprintf((char*)job->databuf + job->datalen, ",IMFBGR");
	}

	/* Insert SZB/G/R/K length descriptors */
	if (x_gp_8bpp) {
		if (k_only == 1) {
			job->datalen += sprintf((char*)job->databuf + job->datalen, ",SZK%u", len_c / 8);
		} else {
			job->datalen += sprintf((char*)job->databuf + job->datalen, ",SZB%u", len_y * 6 / 8);
			job->datalen += sprintf((char*)job->databuf + job->datalen, ",SZG%u", len_m * 6 / 8);
			job->datalen += sprintf((char*)job->databuf + job->datalen, ",SZR%u", len_c * 6 / 8);
			/* Add in a SZK length indication if requested */
			if (x_gp_rk == 1) {
				job->datalen += sprintf((char*)job->databuf + job->datalen, ",SZK%u", len_c / 8);
			}
		}
	} else {
		job->datalen += sprintf((char*)job->databuf + job->datalen, ",SZB%u", len_y);
		job->datalen += sprintf((char*)job->databuf + job->datalen, ",SZG%u", len_m);
		job->datalen += sprintf((char*)job->databuf + job->datalen, ",SZR%u", len_c);
		/* Add in a SZK length indication if requested */
		if (len_k) {
			job->datalen += sprintf((char*)job->databuf + job->datalen, ",SZK%u", len_k);
		}
	}

	/* Terminate command stream */
	job->databuf[job->datalen++] = 0x1c;

	/* Let's figure out how long the image data stream is supposed to be. */
	uint32_t remain;
	if (k_only) {
		remain = len_k + 3;
	} else {
		remain = len_y + len_m + len_c + 3 * 3;
		if (len_k)
			remain += len_k + 3;
	}
	/* Offset the stuff we already read in. */
	remain -= INITIAL_BUF_LEN - buf_offset;
	remain++;  /* Add in a byte for the end of job marker. This is our final value. */

	/* This is how much of the initial buffer is the header length. */
	job->hdr_len = job->datalen;

	if (x_gp_8bpp) {
		uint32_t srcbuf_offset = INITIAL_BUF_LEN - buf_offset;
		uint8_t *srcbuf = malloc(MAX_PRINTJOB_LEN);
		if (!srcbuf) {
			magicard_cleanup_job(job);
			ERROR("Memory allocation failure!\n");
			return CUPS_BACKEND_RETRY_CURRENT;
		}

		memcpy(srcbuf, initial_buf + buf_offset, srcbuf_offset);

		/* Finish loading the data */
		while (remain > 0) {
			i = read(data_fd, srcbuf + srcbuf_offset, remain);
			if (i < 0) {
				ERROR("Data Read Error: %d (%u) @%u)\n", i, remain, srcbuf_offset);
				magicard_cleanup_job(job);
				free(srcbuf);
				return i;
			}
			if (i == 0) {
				ERROR("Short read! (%d/%u)\n", i, remain);
				magicard_cleanup_job(job);
				free(srcbuf);
				return CUPS_BACKEND_CANCEL;
			}
			srcbuf_offset += i;
			remain -= i;
		}

		// XXX handle conversion of K-only jobs.  if needed.

		/* set up source pointers */
		in_y = srcbuf;
		in_m = in_y + len_y + 3;
		in_c = in_m + len_m + 3;

		/* Set up destination pointers */
		out_y = job->databuf + job->datalen;
		out_m = out_y + (len_y * 6 / 8) + 3;
		out_c = out_m + (len_m * 6 / 8) + 3;
		out_k = out_c + (len_c * 6 / 8) + 3;

		/* Termination of each plane */
		memcpy(out_m - 3, in_y + len_y, 3);
		memcpy(out_c - 3, in_m + len_m, 3);
		memcpy(out_k - 3, in_c + len_c, 3);

		if (!x_gp_rk)
			out_k = NULL;

		INFO("Converting image data to printer's native format %s\n", x_gp_rk ? "and extracting K channel" : "");

		downscale_and_extract(gamma, len_y, in_y, in_m, in_c,
				      out_y, out_m, out_c, out_k);

		/* Pad out the length appropriately. */
		job->datalen += ((len_c * 6 / 8) + 3) * 3;

		/* If there's a K plane, compute length.. */
		if (out_k) {
			job->datalen += (len_c / 8);
			job->databuf[job->datalen++] = 0x1c;
			job->databuf[job->datalen++] = 0x4b;
			job->databuf[job->datalen++] = 0x3a;
		}

		/* Terminate the entire stream */
		job->databuf[job->datalen++] = 0x03;

		free(srcbuf);
	} else {
		uint32_t srcbuf_offset = INITIAL_BUF_LEN - buf_offset;
		memcpy(job->databuf + job->datalen, initial_buf + buf_offset, srcbuf_offset);
		job->datalen += srcbuf_offset;

		/* Finish loading the data */
		while (remain > 0) {
			i = read(data_fd, job->databuf + job->datalen, remain);
			if (i < 0) {
				ERROR("Data Read Error: %d (%u) @%d)\n", i, remain, job->datalen);
				magicard_cleanup_job(job);
				return i;
			}
			if (i == 0) {
				magicard_cleanup_job(job);
				ERROR("Short read! (%d/%u)\n", i, remain);
				return CUPS_BACKEND_CANCEL;
			}
			job->datalen += i;
			remain -= i;
		}
	}

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int magicard_main_loop(void *vctx, const void *vjob) {
	struct magicard_ctx *ctx = vctx;
	int ret;
	int copies;

	const struct magicard_printjob *job = vjob;

	// XXX printer handles copy generation..
	// but it's a numeric parameter.  Bleh.
	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	copies = job->copies;
top:
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     job->databuf, job->hdr_len)))
		return CUPS_BACKEND_FAILED;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     job->databuf + job->hdr_len, job->datalen - job->hdr_len)))
		return CUPS_BACKEND_FAILED;

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		goto top;
	}

	return CUPS_BACKEND_OK;
}

static void magicard_cmdline(void)
{
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -q ]           # Query information\n");
	DEBUG("\t\t[ -I ]           # Query printer sensors\n");
	DEBUG("\t\t[ -E ]           # Eject card\n");
	DEBUG("\t\t[ -T ]           # Print self-test card\n");
	DEBUG("\t\t[ -R ]           # Reset printer\n");
}

static int magicard_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct magicard_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "sqEIRT")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 's':
			j = magicard_query_status(ctx);
			break;
		case 'q':
			j = magicard_query_printer(ctx);
			break;
		case 'E':
			j = magicard_eject(ctx);
			break;
		case 'I':
			j = magicard_query_sensors(ctx);
			break;
		case 'R':
			j = magicard_reset(ctx);
			break;
		case 'T':
			j = magicard_selftest_card(ctx);
			break;
		}

		if (j) return j;
	}

	return 0;
}

static int magicard_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct magicard_ctx *ctx = vctx;

	*markers = &ctx->marker;
	*count = 1;

	return CUPS_BACKEND_OK;
}

static const char *magicard_prefixes[] = {
	"magicard", // Family name
	"magicard-tango-2e", "magicard-enduro", "magicard-enduroplus",
	// extras
	"magicard-rio-2e",
	// backwards compatibility
	"tango2e", "enduro", "enduroplus",
	NULL
};

struct dyesub_backend magicard_backend = {
	.name = "Magicard family",
	.version = "0.16",
	.uri_prefixes = magicard_prefixes,
	.cmdline_arg = magicard_cmdline_arg,
	.cmdline_usage = magicard_cmdline,
	.init = magicard_init,
	.attach = magicard_attach,
	.cleanup_job = magicard_cleanup_job,
	.read_parse = magicard_read_parse,
	.main_loop = magicard_main_loop,
	.query_markers = magicard_query_markers,
	.devices = {
		{ USB_VID_MAGICARD, USB_PID_MAGICARD_TANGO2E, P_MAGICARD, NULL, "magicard-tango2e"},
		{ USB_VID_MAGICARD, USB_PID_MAGICARD_ENDURO, P_MAGICARD, NULL, "magicard-enduro"},
		{ USB_VID_MAGICARD, USB_PID_MAGICARD_ENDUROPLUS, P_MAGICARD, NULL, "magicard-enduroplus"},
		{ USB_VID_MAGICARD, 0xFFFF, P_MAGICARD, NULL, "magicard"},
		{ 0, 0, 0, NULL, "magicard"}
	}
};

/* Magicard family Spool file format (Tango2e/Rio2e/AvalonE family)

  This one was rather fun to figure out.

  * Job starts with a sequence of 64 '0x05'
  * Command sequence starts with 0x01
  * Commands are textual and comma-separated.
    * Most are passed through ignored, except for:
      * SZB, SZG, SZR, SZK  -- indicate length of respective data plane
      * IMF -- Image format (BGR/BGRK/K)
      * X-GP-8 -- Tells backend to convert from Gutenprint's 8bpp data
      * X-GP-RK -- Tells backend to extract K channel from color data
  * Command sequence ends with 0x1c
  * Image plane data follows, in the order of the SZ# entries
    * Plane lengths are specified by the SZ# entry.
    * Color planes are actually Y/M/C rather than B/G/R!
    * Each plane terminates with 0x1c __ 0x3a, where __ is 0x42, 0x47, 0x52,
      and 0x4b for B/G/R/K respectively.  Terminator is _not_ part of length.
    * Image data is 6bpp for B/G/R and 1bpp for K, 672*1016 pixels
      * Organized in a series of 84-byte rows.
      * Byte data is LSB first.
      * Each row is a single stripe of a single bit of a pixel, so
        color data is b0b0b0b0.. b1b1b1b1.. .. b5b5b5b5.
  * Job ends with 0x03

  ** ** ** ** ** **

  Firmware updates:

  0x05 (x9) 0x01 REQ,FRM###### 0x1c

  Where ###### is the length of the firmware image.

  Then send over 64 bytes at a time until it's done.

  Then send 0x03 to mark end of job.

  Follow it with:

  0x01 STA,CHK########, 0x03   (8-digit checksum?)

  0x05 (x9) 0x01 REQ,UPG, 0x1c 0x03

  ** ** ** ** ** **

  Known commands seen in print jobs:

  BAC%s    Backside format (CKO, KO, C, CO, K) -- Only used with Duplex.
  CKI%s    Custom Holokote (ON or OFF)
  CPW%s    Color power level (0-100, default 50)
  DPX%s    Duplex (ON or OFF)
  EOI%d    Card alignment end (0-100, default 50)
  ESS%d    Number of copies (1-?)
  HGT%d    Image Height (always seems to be 1016)
  HKM%06X  Holokote hole.  bitwise number, each bit corresponds to an area.
  HKT%d    Holokote type (1 is "ultra secure, 2 is "interlocking rings", etc)
  HPH%s    Holopatch (ON or OFF)
  IMF%s    Image Data Format (BGR, BGRK, K)
  KPW%s    Black power level (0-100, default 50)
  LAN%s    Printer display lanaguage (ENG, ITA, POR, FRA, DEU, ESP, SCH)
  LC%d     Force media type (LC1, LC3, LC6, LC8 for YMCKO/MONO/KO/YMCKOK)
  NCT%d,%d,%d,%d  Overcoat hole
  OPW%s    Overcoat power level (0-100, default 50)
  OVR%s    Overcoat (ON or OFF)
  PAG%d    Page number (always 1, except 2 if printing duplex backside)
  PAT%d    Holopatch area  (0-24)
  REJ%s    Reject faulty cards (ON or OFF)
  SOI%d    Card alignment start (0-100, default 50)
  SLW%s    Colorsure (ON or OFF)
  SZB%d    Blue data length
  SZG%d    Green data length
  SZK%d    Black data length
  SZR%d    Red data length
  TDT%08X  Driver-supplied timestamp of print job.
  USF%s    Holokote (ON or OFF)
  VER%s    Inform the printer of the driver version  (seems to be ignored)
  WID%d    Image Width (always seems to be 642)

    Mag-stripe encoding:

  MAG%d   Magstripe position (1, 2, or 3)
  BPI%d   Bits per Inch (75 or 210)
  MPC%d   Character encoding (5 or 7)
  COE%s   'H'igh or 'L'ow coercivity

    Unknown commands seen in print jobs:

  DDD%s    ? (only seen '50')  -- Could it be K alignment?
  KEE      ?
  NNN%s    ?  (Seen 'OFF')
  NOC%d    ?  (Seen '1')  (Seems to start a job)
  PCT%d,%d,%d,%d  ?  Print area, seems fixed @ 0,0, 1025, 641)
  RT2      ?
  TRO%d    ?  (Seen '0', appears with Holokote)
  XCO%d    ? X start offset (always seems to be 0)
  YCO%d    ? Y start offset (always seems to be 0)

    Unknown commands:  (Seen in firmware guts)

  AAA
  AMS
  BBB%d   Numeric parameter
  CLR
  FBF
  FTC
  HFD%s   String parameter
  IPM
  KKK
  LBL
  LLL
  LRC
  MGV%s  "ON" or "OFF"  but no idea
  MMM
  PAR
  RDM
  SNR
  SSP

   Unknown commands unique to Tango +L (ie w/ Laminator support)

  FRN
  LAM
  LAM_DLY
  LAM_SPD
  LAM_LEN
  LAM_END
  LAM_STA
  LAM_DEG
  LAM_FLM
  LAM_KBD
  LAM_MOD

    Commands consumed by backend:

  ICC%d    Gamma curve (0, 1, 2) -- off, 2.2, or 1.8 respectively.
  X-GP-8   Raw data is 8bpp. needs to be converted.
  X-GP-RK  Extract K channel from color data.

   Open questions:

   * How to query/read magstripe
   * How to set IP address (etc)
   * How to set other parameters

   "Simple Commands"  (REQ,....,)

  RST    Reset printer
  TST    Generate self-test page
  EJT     Eject card

   Other "Simple commands" referenced in Rio Pro/Enduro+ docs

  DEALERSERVICE%s ON/OFF (enter/exit dealer service mode)
  CAM     Reset print head cam position
  CHP%s   UP/DOWN  Feed  card into smart encoder
  CLN     Cleaning cycle
  DYE     Re-init dye film
  ENC     Test encoding cycle
  FEED%d  0/1,+  0/1, load card into standby, >1 feed N cards.
  FLIP    Flip card in printer
  FRN%s   ON/OFF -- Film saving
  HEAD%s  UP/DOWN -- Raise or lower print head.
  RAMP%d  0-100 Density ramp, 50 default
  SET     Saves settings into NVDATA
  STN     Re-init Holokote
  SNS     Soak cycle, test all sensors
  SHW%s CAM, TACHO, FLIP, DYE, LID, FRONT, MID, READ, BUTTON1, BUTTON2,
        SMART, TEMP, ON, OFF
  LNG%d 0/1/2/3/4/5 == ENG/POR/FRE/GER/SPA/ITA
  RUN%s CAM, FEED, DYE, MAIN, FLIPPER, FLIPROLL, FAN, PANEL, POUT, CAL, LCD,
        OFF
  FLM%s Y/M/C/K/O  Align ribbon at corresponding panel
  FCL     Init dye calibration routine
  FCL######  Set dye color to ###### (RGB hex)

*/
