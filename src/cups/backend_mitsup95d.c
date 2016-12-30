/*
 *   Mitsubishi P95D Monochrome Thermal Photo Printer CUPS backend
 *
 *   (c) 2016 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND mitsup95d_backend

#include "backend_common.h"

#define USB_VID_MITSU       0x06D3
#define USB_PID_MITSU_P95D  0x3b10

/* Private data stucture */
struct mitsup95d_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;

	uint8_t mem_clr[4]; // 1b 5a 43 00
	int mem_clr_present;

	uint8_t hdr[2];  // 1b 51
	
	uint8_t hdr1[50]; // 1b 57 20 2e ...
	uint8_t hdr2[50]; // 1b 57 21 2e ...
	uint8_t hdr3[50]; // 1b 57 22 2e ...

	uint8_t hdr4[36];  // 1b 58 ...
	uint8_t plane[12]; // 1b 5a 74 00 ...

	uint8_t *databuf;
	uint32_t datalen;

	uint8_t ftr[2];
};


static void *mitsup95d_init(void)
{
	struct mitsup95d_ctx *ctx = malloc(sizeof(struct mitsup95d_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct mitsup95d_ctx));

	return ctx;
}

static void mitsup95d_attach(void *vctx, struct libusb_device_handle *dev,
			    uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct mitsup95d_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);

}

static void mitsup95d_teardown(void *vctx) {
	struct mitsup95d_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	free(ctx);
}

static int mitsup95d_read_parse(void *vctx, int data_fd) {
	struct mitsup95d_ctx *ctx = vctx;
	uint8_t buf[2];  /* Enough to read in any header */
	uint8_t tmphdr[50];
	uint8_t *ptr;
	int i;
	int remain;
	int ptr_offset;
	
	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}
	ctx->mem_clr_present = 0;
		
top:
	i = read(data_fd, buf, sizeof(buf));
	if (i == 0)
		return CUPS_BACKEND_CANCEL;
	if (i < 0)
		return CUPS_BACKEND_CANCEL;

	if (buf[0] != 0x1b) {
		ERROR("malformed data stream\n");
		return CUPS_BACKEND_CANCEL;
	}

	switch (buf[1]) {
	case 0x43: /* Memory Clear */
		remain = 4;
		ptr = ctx->mem_clr;
		ctx->mem_clr_present = 1;
		break;
	case 0x50: /* Footer */
		remain = 2;
		ptr = ctx->ftr;
		break;
	case 0x51: /* Job Header */
		remain = 2;
		ptr = ctx->hdr;
		break;
	case 0x57: /* Geeneral headers */
		remain = sizeof(tmphdr);
		ptr = tmphdr;
		break;
	case 0x58: /* User Comment */
		remain = 36;
		ptr = ctx->hdr4;
		break;
	case 0x5a: /* Plane header */
		remain = 12;
		ptr = ctx->plane;
		break;
	default:
		ERROR("Unrecognized command! (%02x %02x)\n", buf[0], buf[1]);
		return CUPS_BACKEND_CANCEL;
	}

	memcpy(ptr, buf, sizeof(buf));
	remain -= sizeof(buf);
	ptr_offset = sizeof(buf);
	
	while (remain) {
		i = read(data_fd, ptr + ptr_offset, remain);
		if (i == 0)
			return CUPS_BACKEND_CANCEL;
		if (i < 0)
			return CUPS_BACKEND_CANCEL;
		remain -= i;
		ptr_offset += i;
	}
	
	if (ptr == tmphdr) {
		if (tmphdr[3] != 46) {
			ERROR("Unexpected header chunk: %02x %02x %02x %02x\n",
			      tmphdr[0], tmphdr[1], tmphdr[2], tmphdr[3]);
			return CUPS_BACKEND_CANCEL;
		}
		switch (tmphdr[2]) {
		case 0x20:
			ptr = ctx->hdr1;
			break;
		case 0x21:
			ptr = ctx->hdr2;
			break;
		case 0x22:
			ptr = ctx->hdr3;
			break;
		default:
			ERROR("Unexpected header chunk: %02x %02x %02x %02x\n",
			      tmphdr[0], tmphdr[1], tmphdr[2], tmphdr[3]);
		}
		memcpy(ptr, tmphdr, sizeof(tmphdr));
	} else if (ptr == ctx->plane) {
		uint16_t rows = ctx->plane[10] << 8 | ctx->plane[11];
		uint16_t cols = ctx->plane[8] << 8 | ctx->plane[9];

		remain = rows * cols;

		/* Allocate buffer for the payload */
		ctx->datalen = 0;
		ctx->databuf = malloc(remain);
		if (!ctx->databuf) {
			ERROR("Memory allocation failure!\n");
			return CUPS_BACKEND_FAILED;
		}

		/* Read it in */
		while (remain) {
			i = read(data_fd, ctx->databuf + ctx->datalen, remain);
			if (i == 0)
				return CUPS_BACKEND_CANCEL;
			if (i < 0)
				return CUPS_BACKEND_CANCEL;
			remain -= i;
			ctx->datalen += i;
		}
	} else if (ptr == ctx->ftr) {
		return CUPS_BACKEND_OK;
	}

	goto top;
}

static int mitsup95d_main_loop(void *vctx, int copies) {
	struct mitsup95d_ctx *ctx = vctx;
	uint8_t querycmd[4] = { 0x1b, 0x72, 0x00, 0x00 };
	uint8_t queryresp[9];
	
	int ret;
	int num;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Update printjob header to reflect number of requested copies */
	if (ctx->hdr2[13] != 0xff)
		ctx->hdr2[13] = copies;

	/* XXX Update unknown header field to match sniffs */
	if (ctx->hdr1[18] == 0x00)
		ctx->hdr1[18] = 0x01;
	
	INFO("Waiting for printer idle\n");

        /* Query Status to make sure printer is idle */
	do {
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     querycmd, sizeof(querycmd))))
			return CUPS_BACKEND_FAILED;
		ret = read_data(ctx->dev, ctx->endp_up,
				queryresp, sizeof(queryresp), &num);
		if (num != sizeof(queryresp) || ret < 0) {
			return CUPS_BACKEND_FAILED;
		}

		if (queryresp[5] & 0x40) {
			ERROR("Printer error %02x\n", queryresp[5]); // XXX decode
			return CUPS_BACKEND_STOP;
		}
		if (queryresp[5] == 0x00)
			break;
		
		sleep(1);
	} while (1);

	INFO("Sending print job\n");
	
	/* Send over Memory Clear, if present */
	if (ctx->mem_clr_present) {
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->mem_clr, sizeof(ctx->mem_clr))))
			return CUPS_BACKEND_FAILED;
	}

	/* Send Job Start */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     ctx->hdr, sizeof(ctx->hdr))))
		return CUPS_BACKEND_FAILED;

	/* Send over headers */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     ctx->hdr1, sizeof(ctx->hdr1))))
		return CUPS_BACKEND_FAILED;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     ctx->hdr2, sizeof(ctx->hdr2))))
		return CUPS_BACKEND_FAILED;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     ctx->hdr3, sizeof(ctx->hdr3))))
		return CUPS_BACKEND_FAILED;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     ctx->hdr4, sizeof(ctx->hdr4))))
		return CUPS_BACKEND_FAILED;

	/* Send plane header and image data */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     ctx->plane, sizeof(ctx->plane))))
		return CUPS_BACKEND_FAILED;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     ctx->databuf, ctx->datalen)))
		return CUPS_BACKEND_FAILED;

	/* Query Status to sanity-check job */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     querycmd, sizeof(querycmd))))
		return CUPS_BACKEND_FAILED;
	ret = read_data(ctx->dev, ctx->endp_up,
			queryresp, sizeof(queryresp), &num);
	if (num != sizeof(queryresp) || ret < 0) {
		return CUPS_BACKEND_FAILED;
	}
	if (queryresp[5] & 0x40) {
		ERROR("Printer error %02x\n", queryresp[5]); // XXX decode
		return CUPS_BACKEND_STOP;
	}
	if (queryresp[5] != 0x00) {
		ERROR("Printer not ready (%02x)!\n", queryresp[5]);
		return CUPS_BACKEND_CANCEL;
	}

	/* Send over Footer */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     ctx->ftr, sizeof(ctx->ftr))))
		return CUPS_BACKEND_FAILED;

	INFO("Waiting for completion\n");

	/* Query status until we're done.. */
	do {
		sleep(1);
		
		/* Query Status */
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     querycmd, sizeof(querycmd))))
			return CUPS_BACKEND_FAILED;
		ret = read_data(ctx->dev, ctx->endp_up,
				queryresp, sizeof(queryresp), &num);
		if (num != sizeof(queryresp) || ret < 0) {
			return CUPS_BACKEND_FAILED;
		}

		if (queryresp[5] & 0x40) {
			ERROR("Printer error %02x\n", queryresp[5]); // XXX decode
			return CUPS_BACKEND_STOP;
		}
		if (queryresp[5] == 0 && queryresp[7] == 0)
			break;

		if (queryresp[7] > 0) {
			if (fast_return) {
				INFO("Fast return mode enabled.\n");
				break;
			}
		}
	} while(1);
	
	INFO("Print complete\n");	
	return CUPS_BACKEND_OK;
}

static int mitsup95d_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct canonselphy_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL)) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		}

		if (j) return j;
	}

	return 0;
}

/* Exported */
struct dyesub_backend mitsup95d_backend = {
	.name = "Mitsubishi P95D",
	.version = "0.02",
	.uri_prefix = "mitsup95d",
	.cmdline_arg = mitsup95d_cmdline_arg,
	.init = mitsup95d_init,
	.attach = mitsup95d_attach,
	.teardown = mitsup95d_teardown,
	.read_parse = mitsup95d_read_parse,
	.main_loop = mitsup95d_main_loop,
	.devices = {
	{ USB_VID_MITSU, USB_PID_MITSU_P95D, P_MITSU_P95D, ""},
	{ 0, 0, 0, ""}
	}
};

/*****************************************************

 Mitsubishi P95D Spool Format

   ...All fields are BIG ENDIAN.

 MEMORY_CLEAR  (optional)

  1b 5a 43 00

 JOB_HDR

  1b 51

 PRINT_SETUP

  1b 57 20 2e  00 0a 00 02   00 00 00 00  00 00 CC CC
  RR RR XX 00  00 00 00 00   00 00 00 00  00 00 00 00
  00 00 00 00  00 00 00 00   00 00 00 00  00 00 00 00
  00 00

   XX == 01 seen in sniffs, 00 seen in dumps.  Unknown!

   CC CC = columns, RR RR = rows (print dimensions)

 PRINT_OPTIONS


  1b 57 21 2e  00 4a aa 00   20 TT 00 00  64 NN 00 MM
  [[ 00 00 00  00 00 00 00   00 00 00 00  00 00 00 00
  00 ]] 00 00  00 02 00 00   00 00 00 00  00 00 00 00
  00 XY

   NN = copies
        1..200
        0xff (continuous print)
   MM = comment type
        00 = None
        01 = Printer Setting
        02 = Date
        03 = DateTime
   [[ .. ]] = actual comment (18 bytes), see below.
   TT = media type
        00 = Standard
        01 = High Density
        02 = High Glossy
        03 = High Glossy (K95HG)
   X = media cut length
        4..8  (mm)
   Y = flags
        0x04 = Paper save
        0x03 = Buzzer (3 = high, 2 = low, 0 = off)


 GAMMA ????

  1b 57 22 2e  00 15 TT 00   00 00 00 00  LL BB CC 00
  [[ 00 00 00  00 00 00 00   00 00 00 00  00 00 00 00
  00 00 00 00  00 00 00 00   00 00 00 00  00 00 00 00
  00 ]]

   LL = Gamma table
        00 = Printer Setting
        01..05  Gamma table 1..5
        10 = Use LUT
   BB = Brightness (signed 8-bit)
   CC = Contrast (signed 8-bit)
   TT = Table present
        00 = No
        01 = Yes
   [[ .. ]] = Gamma table, loaded from LUT on disk.  (skip first 16 bytes)

 USER_COMMENT

  1b 58 [[ 20  20 20 20 20  20 20 20 20  20 20 20 20
  20 20 20 20  20 20 20 20  20 20 20 20  20 20 20 20
  20 20 20 ]]

   [[ .. ]] = Actual comment.  34 bytes payload, 0x20 -> 0x7e
               (Null terminated?)

 IMAGE_DATA

  1b 5a 74 00  00 00 YY YY  CC CC RR RR
  [[ .. data ... ]]

    CC CC = columns
    RR CC = rows
    YY YY = row offset

  Followed by C*R bytes of monochrome data, 0xff = white, 0x00 = black

 PRINT_START

  1b 50

 *********************************

 Printer Comms:

 STATUS query

 -> 1b 72 00 00
 <- e4 72 00 00  04 XX 00 YY  00

  YY == remaining copies
  XX == Status?
    00 == Idle
    02 == Printing
    43 == Door open
    44 == No Paper
    4? == "Button"
    4? == "Gear Lock"
    4? == Head Up
    ^
    \--- 0x40 appears to be a flag that indicates error.

 ****************************

UNKNOWNS:

 * How multiple images are stacked for printing on a single page
   (col offset too?  write four, then tell PRINT?)  Is this the mystery 0x01?
 * How to adjust printer sharpness?
 * Serial number query (iSerial appears bogus)
 * What "custom gamma" table does to spool file?

*/


