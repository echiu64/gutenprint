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

	uint16_t rows;
	uint16_t cols;

	int k60;
};

/* Program states */
enum {
	S_IDLE = 0,
	S_SENT_ATTN,
	S_SENT_HDR,
	S_SENT_DATA,
	S_FINISHED,
};

/* Printer data structures */
struct mitsu70x_state {
	uint32_t hdr;
	uint8_t  data[22];
} __attribute__((packed));
struct mitsu70x_status_deck {
	uint8_t present; /* 0x80 for NOT present */
	uint8_t unk[21];
	uint16_t remain; /* BIG ENDIAN */
	uint8_t unkb[40];
} __attribute__((packed));
struct mitsu70x_status_resp {
	uint8_t unk[128];
	struct mitsu70x_status_deck lower;
	struct mitsu70x_status_deck upper;
} __attribute__((packed));

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

	uint8_t  multicut;
	uint8_t  zero5[15];

	uint8_t  zero6[448];
} __attribute__((packed));

#define CMDBUF_LEN 512
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

	if (desc.idProduct == USB_PID_MITSU_K60)
		ctx->k60 = 1;
}


static void mitsu70x_teardown(void *vctx) {
	struct mitsu70x_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	free(ctx);
}

static int mitsu70x_read_parse(void *vctx, int data_fd) {
	struct mitsu70x_ctx *ctx = vctx;
	uint8_t hdr[1024];
	int i, remain;
	struct mitsu70x_hdr *mhdr = (struct mitsu70x_hdr*)(hdr + 512);

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	/* Read in initial header */
	remain = sizeof(hdr);
	while (remain > 0) {
		i = read(data_fd, hdr + sizeof(hdr) - remain, remain);
		if (i == 0)
			return CUPS_BACKEND_CANCEL;
		if (i < 0)
			return CUPS_BACKEND_CANCEL;
		remain -= i;
	}

	/* Sanity check */
	if (hdr[0] != 0x1b ||
	    hdr[1] != 0x45 ||
	    hdr[2] != 0x57 ||
	    hdr[3] != 0x55) {
		ERROR("Unrecognized data format!\n");
		return CUPS_BACKEND_CANCEL;
	}

	/* Work out printjob size */
	ctx->rows = mhdr->rows;
	ctx->cols = mhdr->cols;

	remain = be16_to_cpu(ctx->rows) * be16_to_cpu(ctx->cols) * 2;
	remain = (remain + 511) / 512 * 512; /* Round to nearest 512 bytes. */
	remain *= 3;  /* One for each plane */

	if (mhdr->laminate) {
		i = be16_to_cpu(mhdr->lamrows) * be16_to_cpu(mhdr->lamcols) * 2;
		i = (i + 511) / 512 * 512; /* Round to nearest 512 bytes. */
		remain += i;
	}

	ctx->databuf = malloc(sizeof(hdr) + remain);
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_FAILED;
	}

	memcpy(ctx->databuf, &hdr, sizeof(hdr));
	ctx->datalen += sizeof(hdr);

	/* Read in the spool data */
	while(remain) {
		i = read(data_fd, ctx->databuf + ctx->datalen, remain);
		if (i == 0)
			return CUPS_BACKEND_CANCEL;
		if (i < 0)
			return CUPS_BACKEND_CANCEL;
		ctx->datalen += i;
		remain -= i;
	}

	return CUPS_BACKEND_OK;
}

static int mitsu70x_do_pagesetup(struct mitsu70x_ctx *ctx)
{
	uint8_t cmdbuf[CMDBUF_LEN];
	uint8_t rdbuf[READBACK_LEN];

	int num, ret;

	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x56;
	cmdbuf[2] = 0x33;
	cmdbuf[3] = 0x00;
	memcpy(cmdbuf + 4, &ctx->rows, 2);
	memcpy(cmdbuf + 6, &ctx->cols, 2);
	cmdbuf[8] = 0x00; // or 0x80??
	cmdbuf[9] = 0x00;
	
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, 10)))
		return CUPS_BACKEND_FAILED;
	
	/* Read in the printer status */
	ret = read_data(ctx->dev, ctx->endp_up,
			rdbuf, READBACK_LEN, &num);
	if (ret < 0)
		return CUPS_BACKEND_FAILED;
	
	if (num != 6) {
		ERROR("Short Read! (%d/%d)\n", num, 26);
		return CUPS_BACKEND_FAILED;
	}
	
	/* Make sure response is sane */
	if (rdbuf[0] != 0xe4 ||
	    rdbuf[1] != 0x56 ||
	    rdbuf[2] != 0x33) {
		ERROR("Unknown response from printer\n");
		return CUPS_BACKEND_FAILED;
	}
	
	return 0;
}

static int mitsu70x_get_state(struct mitsu70x_ctx *ctx, struct mitsu70x_state *resp)
{
	uint8_t cmdbuf[CMDBUF_LEN];
	int num, ret;

	/* Send Printer Query */
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

	memset(resp, 0, sizeof(*resp));

	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) resp, sizeof(*resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(*resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*resp));
		return 4;
	}

	return 0;
}

static int mitsu70x_get_status(struct mitsu70x_ctx *ctx, struct mitsu70x_status_resp *resp)
{
	uint8_t cmdbuf[CMDBUF_LEN];
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
	memset(resp, 0, sizeof(*resp));
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) resp, sizeof(*resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(*resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*resp));
		return 4;
	}

	return 0;
}

static int mitsu70x_main_loop(void *vctx, int copies) {
	struct mitsu70x_ctx *ctx = vctx;

	struct mitsu70x_state rdbuf, rdbuf2;

	int last_state = -1, state = S_IDLE;
	int ret;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

top:
	if (state != last_state) {
		if (dyesub_debug)
			DEBUG("last_state %d new %d\n", last_state, state);
	}

	ret = mitsu70x_get_state(ctx, &rdbuf);
	if (ret)
		return CUPS_BACKEND_FAILED;

	if (memcmp(&rdbuf, &rdbuf2, sizeof(rdbuf))) {
		memcpy(&rdbuf2, &rdbuf, sizeof(rdbuf));
	} else if (state == last_state) {
		sleep(1);
	}
	last_state = state;

	fflush(stderr);

	switch (state) {
	case S_IDLE:
		INFO("Waiting for printer idle\n");
#if 0 // XXX no idea if this works..
		if (rdbuf.data[9] != 0x00) {
			break;
		}
#endif
		INFO("Sending attention sequence\n");
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf, 512)))
			return CUPS_BACKEND_FAILED;

		state = S_SENT_ATTN;
		break;
	case S_SENT_ATTN: {
		struct mitsu70x_status_resp resp;
		ret = mitsu70x_get_status(ctx, &resp);
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Yes, do it twice.. */

		ret = mitsu70x_get_status(ctx, &resp);
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		// XXX check resp for sanity?

		state = S_SENT_HDR;
		break;
	}
	case S_SENT_HDR:
		INFO("Sending Page setup sequence\n");
		if ((ret = mitsu70x_do_pagesetup(ctx)))
			return ret;

		INFO("Sending header sequence\n");

		/* K60 may require fixups */
		if (ctx->k60) {
			/* K60 only has a lower deck */
			ctx->databuf[512+32] = 1;

			/* 4x6 prints on 6x8 media need multicut mode */
			if (ctx->databuf[512+16] == 0x07 &&
			    ctx->databuf[512+16+1] == 0x48 &&
			    ctx->databuf[512+16+2] == 0x04 &&
			    ctx->databuf[512+16+3] == 0xc2) {
				ctx->databuf[512+48] = 1;
			}
		}

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf + 512, 512)))
			return CUPS_BACKEND_FAILED;

		INFO("Sending data\n");

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf + 1024, ctx->datalen - 1024)))
			return CUPS_BACKEND_FAILED;

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

	return CUPS_BACKEND_OK;
}

static void mitsu70x_dump_status(struct mitsu70x_status_resp *resp)
{
	if (dyesub_debug) {
		uint8_t *ptr;
		unsigned int i;

		DEBUG("Status Dump:\n");
		for (i = 0 ; i < sizeof(resp->unk) ; i++) {
			DEBUG2("%02x ", resp->unk[i]);
		}
		DEBUG2("\n");
		DEBUG("Lower Deck:\n");
		ptr = (uint8_t*) &resp->lower;
		for (i = 0 ; i < sizeof(resp->lower) ; i++) {
			DEBUG2("%02x ", *ptr++);
		}
		DEBUG2("\n");
		ptr = (uint8_t*) &resp->upper;
		DEBUG("Upper Deck:\n");
		for (i = 0 ; i < sizeof(resp->upper) ; i++) {
			DEBUG2("%02x ", *ptr++);
		}
		DEBUG2("\n");
	}
	if (resp->upper.present & 0x80) {  /* Not present */
		INFO("Prints remaining:  %d\n",
		     be16_to_cpu(resp->lower.remain));
	} else {
		INFO("Prints remaining:  Lower: %d Upper: %d\n",
		     be16_to_cpu(resp->lower.remain),
		     be16_to_cpu(resp->upper.remain));
	}
}

static int mitsu70x_query_status(struct mitsu70x_ctx *ctx)
{
	struct mitsu70x_status_resp resp;
	int ret;

	ret = mitsu70x_get_status(ctx, &resp);

	if (!ret)
		mitsu70x_dump_status(&resp);

	return ret;
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
				j = mitsu70x_query_status(ctx);
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
	.version = "0.24",
	.uri_prefix = "mitsu70x",
	.cmdline_usage = mitsu70x_cmdline,
	.cmdline_arg = mitsu70x_cmdline_arg,
	.init = mitsu70x_init,
	.attach = mitsu70x_attach,
	.teardown = mitsu70x_teardown,
	.read_parse = mitsu70x_read_parse,
	.main_loop = mitsu70x_main_loop,
	.devices = {
	{ USB_VID_MITSU, USB_PID_MITSU_D70X, P_MITSU_D70X, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_K60, P_MITSU_D70X, ""},
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
   RR 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00

   (padded by NULLs to a 512-byte boundary)

   XX XX == columns
   YY YY == rows
   QQ QQ == lamination columns (equal to XX XX)
   ZZ ZZ == lamination rows (YY YY + 12)
   SS    == Print mode: 00 = Fine, 03 = SuperFine, 04 = UltraFine
            (Matte requires Superfine or Ultrafine)
   UU    == 00 == Auto, 01 == Lower Deck, 02 == Upper Deck
   TT    == 00 with no lamination, 02 with.
   RR    == 00 (normal), 01 == (Double-cut 4x6), 05 == (double-cut 2x6)
   
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
   SS    == Print mode: 00 = Fine, 04 = UltraFine
            (Matte requires Ultrafine)
   UU    == 01 (Lower Deck)
   TT    == 00 with no lamination, 02 with.
   RR    == 00 (normal), 01 == (Double-cut 4x6), 05 == (double-cut 2x6)

   Data planes:
   16-bit data, rounded up to 512-byte block (XX * YY * 2 bytes)
   
   Lamination plane: (only present if QQ + ZZ are nonzero)
   16-byte data, rounded up to 512-byte block (QQ * ZZ * 2 bytes)

   Lamination appears to be these bytes, repeated:  28 6a  ab 58  6c 22

   ********************************************************************

   Command format: (D707)

   -> 1b 56 32 30
   <- [256 byte payload]

   CP-D707DW:

    e4 56 32 30 00 00 00 00  00 00 00 00 00 00 00 00   .V20............
    00 00 00 00 00 00 00 00  00 00 00 80 00 00 00 00   ................
    44 80 00 00 5f 00 00 3d  43 00 50 00 44 00 37 00   D..._..=C.P.D.7.
    30 00 44 00 30 00 30 00  31 00 31 00 31 00 37 00   0.D.0.0.1.1.1.7.
    33 31 36 54 31 33 21 a3  33 31 35 42 31 32 f5 e5   316T13!.315B12..
    33 31 39 42 31 31 a3 fb  33 31 38 45 31 32 50 0d   319B11..318E12P.
    33 31 37 41 32 32 a3 82  44 55 4d 4d 59 40 00 00   317A22..DUMMY@..
    44 55 4d 4d 59 40 00 00  00 00 00 00 00 00 00 00   DUMMY@..........

     alt:

    e4 56 32 30 0f 00 00 00  00 00 00 00 00 00 00 00 
    00 00 00 00 00 00 00 00  00 00 0a 00 80 00 00 00
    44 00 00 00 5f 00 00 bd  43 00 50 00 44 00 37 00
    30 00 44 00 30 00 37 00  38 00 33 00 39 00 38 00
    33 31 36 56 31 31 06 4d  33 31 35 42 31 32 f5 e5
    33 31 39 42 31 31 a3 fb  33 31 38 46 31 31 cc 65
    33 31 37 42 32 31 f4 19  44 55 4d 4d 59 40 00 00
    44 55 4d 4d 59 40 00 00  00 00 00 00 00 00 00 00 

    LOWER DECK

    00 00 00 00 00 00 02 04  3f 00 00 04 96 00 00 00
    ff 0f 01 00 00 c8 NN NN  00 00 00 00 05 28 75 80  NN NN: prints remaining
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00

      alt:

    00 00 00 0a 05 05 01 d5  38 00 00 00 14 00 00 00 
    ff ff ff ff ff ff ff ff  ff ff 00 00 00 27 72 80
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00


    UPPER DECK

    00 00 00 00 00 00 01 ee  3d 00 00 06 39 00 00 00
    ff 02 00 00 01 90 NN NN  00 00 00 00 06 67 78 00  NN NN: prints remaining
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00

   CP-K60DW-S:

    e4 56 32 30 0f 00 00 00  00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00  00 00 0a 80 00 00 00 00
    02 00 00 00 5e 00 04 87  43 00 50 00 4b 00 36 00
    30 00 44 00 30 00+32 00 +33 00 32 00+30 00 36 00
    33 31 36+4b 33 31 d6 7a  33 31 35 41 33 31 ae 37
    33 31 39 41 37 31 6a 36  33 31 38 44 33 31 1e 4a
    33 31 37 42 32 31 f4 19  44 55 4d 4d 59 40 00 00
    44 55 4d 4d 59 40 00 00  00 00 00 00 00 00 00 00

     alt:

    e4 56 32 30 0f 00 00 00  00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00  00 00 0a 80 00 00 00 00
    02 00 00 00 5e 00 04 87  43 00 50 00 4b 00 36 00
    30 00 44 00 30 00+37 00 +39 00 32 00+31 00 30 00
    33 31 36+4c 33 31+a4+0b  33 31 35 41 33 31 ae 37 
    33 31 39 41 37 31 6a 36  33 31 38 44 33 31 1e 4a 
    33 31 37 42 32 31 f4 19  44 55 4d 4d 59 40 00 00 
    44 55 4d 4d 59 40 00 00  00 00 00 00 00 00 00 00

    LOWER DECK (K60)

    00 00 00 00 00 00?02 09  3f 00 00 00?05 00 00 01
    61 8f 00 00 01 40 NN NN  00 00 00 00 00?16 81 80  NN NN: prints remaining
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00

     alt:

    00 00 00 00 00 00?01 d2  39 00 00 00?07 00 00 00 
    61 8f 00 00 01 40 NN MM  00 00 00 00 00?17 79 80
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00

    UPPER DECK (K60 -- No upper deck present)

    80 00 00 00 00 00 00 ff  ff 00 00 00 00 00 00 00
    ff ff ff ff ff ff ff ff  ff ff 00 00 00 00 80 00
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00

   -> 1b 56 31 30  00 00
   <- [26 byte payload]

   CP-D707DW:

    e4 56 31 30 00 00 00 XX  YY ZZ 00 00 TT 00 00 00
    00 00 00 00 WW 00 00 00  00 00

    XX/YY/ZZ and WW/TT are unknown.  Observed values:

    00 00 00   00/00
    40 80 a0   80/0f
    80 80 a0
    40 80 90
    40 80 00

     also seen:

    e4 56 31 30 00 00 00 00  00 00 00 00 0f 00 00 00
    00 0a 05 05 80 00 00 00  00 00

    e4 56 31 30 00 00 00 40  80 90 10 00 0f 00 00 00 
    00 0a 05 05 80 00 00 00  00 00

    e4 56 31 30 00 00 00 00  40 80 00 00 00 ff 40 00 
    00 00 00 00 80 00 00 00  00 00

     print just submitted:

    e4 56 31 30 00 00 00 00  40 20 00 00 00 8c 00 00 
    00 00 00 00 80 00 00 00  00 00

     prints running...

    e4 56 31 30 00 00 00 00  40 20 00 00 00 cf 00 20 
    00 00 00 00 80 00 00 00  00 00



   CP-K60DW-S:

    e4 56 31 30 00 00 00 XX  YY 00 00 00 0f 00 00 00
    00 00 00 00 80 00 00 00  00 00

    XX/YY are unknown, observed values:

    40/80
    00/00

   Sent to start a print

   -> 1b 56 33 00 XX XX YY YY UU 00

    XX XX == columns
    YY YY == rows
    UU    == Unknown, seen 0x00 and 0x80

   <- [ 6 byte payload ]

    e4 56 33 00 00 00
    e4 56 33 00 00 01
    e5 56 33 ff 01 01  (which appeared to work)

   ** ** ** ** ** **

   The windows drivers seem to send the id and status queries before
   and in between each of the chunks sent to the printer.  There doesn't
   appear to be any particular intelligence in the protocol, but it didn't
   work when the raw dump was submitted as-is.

 */
