/*
 *   Mitsubishi CP-D70/D707 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2014 Solomon Peachy <pizza@shaftnet.org>
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

#include "backend_common.h"

#define USB_VID_MITSU       0x06D3
#define USB_PID_MITSU_D70X  0x3B30
#define USB_PID_MITSU_K60   0x3B31

/* Private data stucture */
struct mitsu70x_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;

	uint8_t *databuf;
	int datalen;
};

/* Program states */
enum {
	S_IDLE = 0,
	S_SENT_ATTN,
	S_SENT_HDR,
	S_SENT_DATA,
	S_FINISHED,
};

#define READBACK_LEN 256

static void *mitsu70x_init(void)
{
	struct mitsu70x_ctx *ctx = malloc(sizeof(struct mitsu70x_ctx));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(struct mitsu70x_ctx));

	return ctx;
}

static void mitsu70x_attach(void *vctx, struct libusb_device_handle *dev,
			    uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct mitsu70x_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);
}


static void mitsu70x_teardown(void *vctx) {
	struct mitsu70x_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	free(ctx);
}

/* Max job size is 6x9+lamination, equalling ~38MB */
#define MAX_PRINTJOB_LEN (1024*1024*40)

struct mitsu70x_hdr {
	uint32_t cmd;
	uint8_t  zero0[12];
	uint16_t cols;
	uint16_t rows;
	uint16_t lamcols;
	uint16_t lamrows;
	uint8_t  superfine;
	uint8_t  zero1[7];
	uint8_t  deck;
	uint8_t  zero2[7];
	uint8_t  zero3;
	uint8_t  laminate;
	uint8_t  zero4[6];
	uint8_t  zero5[512-48];
};

static int mitsu70x_read_parse(void *vctx, int data_fd) {
	struct mitsu70x_ctx *ctx = vctx;
	uint8_t hdr[1024];
	int i, remain;
	struct mitsu70x_hdr *mhdr = (struct mitsu70x_hdr*)(hdr + 512);

	if (!ctx)
		return 1;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	/* Read in initial header */
	remain = sizeof(hdr);
	while (remain > 0) {
		i = read(data_fd, hdr + sizeof(hdr) - remain, remain);
		if (i == 0)
			return 1;
		if (i < 0)
			return i;
		remain -= i;
	}

	/* Sanity check */
	if (hdr[0] != 0x1b ||
	    hdr[1] != 0x45 ||
	    hdr[2] != 0x57 ||
	    hdr[3] != 0x55) {
		ERROR("Unrecognized data format!\n");
		return(1);
	}

	/* Work out printjob size */
	remain = be16_to_cpu(mhdr->rows) * be16_to_cpu(mhdr->cols) * 2;
	remain = (remain + 511) / 512 * 512; /* Round to nearest 512 bytes. */
	remain *= 3;  /* One for each plane */
	if (mhdr->laminate) {
		i = be16_to_cpu(mhdr->lamrows) * be16_to_cpu(mhdr->lamcols) * 2;
		i = (i + 511) / 512 * 512; /* Round to nearest 512 bytes. */
		remain += i;
	}

	ctx->databuf = malloc(remain + sizeof(hdr));
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return 2;
	}

	memcpy(ctx->databuf, &hdr, sizeof(hdr));
	ctx->datalen += sizeof(hdr);

	/* Read in the spool data */
	while(remain) {
		i = read(data_fd, ctx->databuf + ctx->datalen - remain, remain);
		if (i < 0)
			return i;
		remain -= i;
	}

	return 0;
}

#define CMDBUF_LEN 512
#define READBACK_LEN 256

static int mitsu70x_main_loop(void *vctx, int copies) {
	struct mitsu70x_ctx *ctx = vctx;

	uint8_t rdbuf[READBACK_LEN];
	uint8_t rdbuf2[READBACK_LEN];
	uint8_t cmdbuf[CMDBUF_LEN];

	int last_state = -1, state = S_IDLE;
	int num, ret;
	int pending = 0;

	if (!ctx)
		return 1;

top:
	if (state != last_state) {
		if (dyesub_debug)
			DEBUG("last_state %d new %d\n", last_state, state);
	}

	if (pending)
		goto skip_query;

	/* Send Status Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x56;
	cmdbuf[2] = 0x31;
	cmdbuf[3] = 0x30;
	cmdbuf[4] = 0x00;
	cmdbuf[5] = 0x00;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, 6)))
		return ret;
	/* Send Status Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x56;
	cmdbuf[2] = 0x31;
	cmdbuf[3] = 0x30;

skip_query:
	/* Read in the printer status */
	ret = read_data(ctx->dev, ctx->endp_up,
			rdbuf, READBACK_LEN, &num);
	if (ret < 0)
		return ret;

	if (num != 26) {
		ERROR("Short Read! (%d/%d)\n", num, 26);
		return 4;
	}

	if (memcmp(rdbuf, rdbuf2, READBACK_LEN)) {
		memcpy(rdbuf2, rdbuf, READBACK_LEN);
	} else if (state == last_state) {
		sleep(1);
	}
	last_state = state;

	fflush(stderr);

	pending = 0;

	switch (state) {
	case S_IDLE:
		INFO("Waiting for printer idle\n");
		if (rdbuf[7] != 0x00 ||
		    rdbuf[8] != 0x00 ||
		    rdbuf[9] != 0x00) {
			break;
		}

		INFO("Sending attention sequence\n");
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf, 512)))
			return ret;
		
		state = S_SENT_ATTN;
	case S_SENT_ATTN:
		INFO("Sending header sequence\n");

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf + 512, 512)))
			return ret;

		state = S_SENT_HDR;
		break;
	case S_SENT_HDR:
		INFO("Sending data\n");

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf + 1024, ctx->datalen - 1024)))
			return ret;
				
		state = S_SENT_DATA;
		break;
	case S_SENT_DATA:
		INFO("Waiting for printer to acknowledge completion\n");

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

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		state = S_IDLE;
		goto top;
	}

	return 0;
}

struct mitsu70x_status_deck {
	uint8_t unk[64];
};

struct mitsu70x_status_resp {
	uint8_t unk[128];
	struct mitsu70x_status_deck lower;
	struct mitsu70x_status_deck upper;
};

static int mitsu70x_get_status(struct mitsu70x_ctx *ctx)
{
	uint8_t cmdbuf[CMDBUF_LEN];
	struct mitsu70x_status_resp resp;
	int num, ret;

	/* Send Printer Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x56;
	cmdbuf[2] = 0x32;
	cmdbuf[3] = 0x30;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, 4)))
		return ret;
	memset(&resp, 0, sizeof(resp));
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) &resp, sizeof(resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(resp));
		return 4;
	}

	if (dyesub_debug) {
		unsigned int i;

		DEBUG("Status Dump:\n");
		for (i = 0 ; i < sizeof(resp.unk) ; i++) {
			DEBUG2("%02x ", resp.unk[i]);
		}
		DEBUG2("\n");
		DEBUG("Lower Deck:\n");
		for (i = 0 ; i < sizeof(resp.lower.unk) ; i++) {
			DEBUG2("%02x ", resp.lower.unk[i]);
		}
		DEBUG2("\n");
		DEBUG("Upper Deck:\n");
		for (i = 0 ; i < sizeof(resp.upper.unk) ; i++) {
			DEBUG2("%02x ", resp.upper.unk[i]);
		}
		DEBUG2("\n");
	}
	INFO("Prints remaining:  Lower: %d Upper: %d\n",
	     resp.lower.unk[23], resp.upper.unk[23]);

	return 0;
}

static void mitsu70x_cmdline(void)
{
	DEBUG("\t\t[ -s ]           # Query status\n");
}

static int mitsu70x_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct mitsu70x_ctx *ctx = vctx;
	int i, j = 0;

	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
	while ((i = getopt(argc, argv, "s")) >= 0) {
		switch(i) {
		case 's':
			if (ctx) {
				j = mitsu70x_get_status(ctx);
				break;
			}
			return 1;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
}


/* Exported */
struct dyesub_backend mitsu70x_backend = {
	.name = "Mitsubishi CP-D70/D707/K60",
	.version = "0.13",
	.uri_prefix = "mitsu70x",
	.cmdline_usage = mitsu70x_cmdline,
	.cmdline_arg = mitsu70x_cmdline_arg,
	.init = mitsu70x_init,
	.attach = mitsu70x_attach,
	.teardown = mitsu70x_teardown,
	.read_parse = mitsu70x_read_parse,
	.main_loop = mitsu70x_main_loop,
	.devices = {
	{ USB_VID_MITSU, USB_PID_MITSU_D70X, P_MITSU_D70X, "Mitsubishi"},
	{ USB_VID_MITSU, USB_PID_MITSU_K60, P_MITSU_D70X, "Mitsubishi"},
	{ 0, 0, 0, ""}
	}
};

/* Mitsubish CP-D70x/CP-K60 data format 

   Spool file consists of two headers followed by three image planes
   and an optional lamination data plane.  All blocks are rounded up to
   a 512-byte boundary.

   All multi-byte numbers are big endian, ie MSB first.

   Header 1:  (Init)

   1b 45 57 55 00 00 00 00  00 00 00 00 00 00 00 00
   (padded by NULLs to a 512-byte boundary)

   [[ D70x ]] Header 2:  (Header)  

   1b 5a 54 01 00 00 00 00  00 00 00 00 00 00 00 00
   XX XX YY YY QQ QQ ZZ ZZ  SS 00 00 00 00 00 00 00
   UU 00 00 00 00 00 00 00  00 TT 00 00 00 00 00 00
   (padded by NULLs to a 512-byte boundary)

   XX XX == columns
   YY YY == rows
   QQ QQ == lamination columns (equal to XX XX)
   ZZ ZZ == lamination rows (YY YY + 12)
   SS    == SuperFine mode (00 == off, 03 == on
            Lamination always turns this on!
   UU    == 00 == Auto, 01 == Lower Deck, 02 == Upper Deck
   TT    == 00 with no lamination, 02 with.
   
   [[ K60 ]] Header 2:  (Header) 

   1b 5a 54 00 00 00 00 00  00 00 00 00 00 00 00 00
   XX XX YY YY QQ QQ ZZ ZZ  SS 00 00 00 00 00 00 00
   UU 00 00 00 00 00 00 00  00 TT 00 00 00 00 00 00
   RR 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00

   (padded by NULLs to a 512-byte boundary)

   XX XX == columns
   YY YY == rows
   QQ QQ == lamination columns (equal to XX XX)
   ZZ ZZ == lamination rows (usually YY YY + 12)
   SS    == UltraFine mode (00 == off, 04 == on.. forces 8x6 print?)
   UU    == 01 (Lower Deck)
   TT    == 00 with no lamination, 02 with.
   RR    == 0x05 for double-cut 2x6, 0x00 for double-cut 4x6, otherwise 0x01

   Data planes:
   16-bit data, rounded up to 512-byte block (XX * YY * 2 bytes)
   
   Lamination plane: (only present if QQ + ZZ are nonzero)
   16-byte data, rounded up to 512-byte block (QQ * ZZ * 2 bytes)

   Lamination appears to be these bytes, repeated:  28 6a  ab 58  6c 22

   ********************************************************************

   Command format: (D70/D707)

   -> 1b 56 32 30
   <- [256 byte payload] 

   e4 56 32 30 00 00 00 00 00 00 00 00 00 00 00 00   .V20............
   00 00 00 00 00 00 00 00 00 00 00 80 00 00 00 00   ................
   44 80 00 00 5f 00 00 3d 43 00 50 00 44 00 37 00   D..._..=C.P.D.7.
   30 00 44 00 30 00 30 00 31 00 31 00 31 00 37 00   0.D.0.0.1.1.1.7.
   33 31 36 54 31 33 21 a3 33 31 35 42 31 32 f5 e5   316T13!.315B12..
   33 31 39 42 31 31 a3 fb 33 31 38 45 31 32 50 0d   319B11..318E12P.
   33 31 37 41 32 32 a3 82 44 55 4d 4d 59 40 00 00   317A22..DUMMY@..
   44 55 4d 4d 59 40 00 00 00 00 00 00 00 00 00 00   DUMMY@..........

   LOWER DECK

   00 00 00 00 00 00 02 04  3f 00 00 04 96 00 00 00   ........?.......
   ff 0f 01 00 00 c8 NN NN  00 00 00 00 05 28 75 80   .......R.....(u.
   80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00   ................
   80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00   ................
   
   UPPER DECK
   
   00 00 00 00 00 00 01 ee  3d 00 00 06 39 00 00 00   ........=...9...
   ff 02 00 00 01 90 NN NN  00 00 00 00 06 67 78 00   .............gx.
   80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00   ................
   80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00   ................

   NN NN == Number of prints remaining on that deck.  
   (None of the other fields are decoded yet)

   -> 1b 56 31 30  00 00
   <- [26 byte payload]

   e4 56 31 30  00 00 00 XX  YY ZZ 00 00 00 00 00 00
   00 00 00 00  00 00 00 00  00 00
   
   XX/YY/ZZ are unkown.  Observed values:

   40 80 a0
   00 00 00
   80 80 a0

   ** ** ** ** ** **

   The windows drivers seem to send the id and status queries before 
   and in between each of the chunks sent to the printer.  There doesn't
   appear to be any particular intelligence in the protocol, but it didn't
   work when the raw dump was submitted as-is.

 */
