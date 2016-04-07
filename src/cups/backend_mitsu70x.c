/*
 *   Mitsubishi CP-D70/D707 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2015 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND mitsu70x_backend

#include "backend_common.h"

#define USB_VID_MITSU       0x06D3
#define USB_PID_MITSU_D70X  0x3B30
#define USB_PID_MITSU_K60   0x3B31
//#define USB_PID_MITSU_D80   XXXXXX
#define USB_VID_KODAK       0x040a
#define USB_PID_KODAK305    0x404f

/* Private data stucture */
struct mitsu70x_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	uint8_t *databuf;
	int datalen;

	uint16_t rows;
	uint16_t cols;
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
	uint16_t present; /* 0x80 for NOT present, 0x00 otherwise */
	uint16_t unk[9];
	uint16_t capacity; /* media capacity */
	uint16_t remain;   /* media remaining */
	uint16_t unkb[2];
	uint16_t prints; /* lifetime prints on deck? */
	uint16_t unkc[1];
	uint16_t blank[16]; /* All fields are 0x8000 */
} __attribute__((packed));

struct mitsu70x_status_ver {
	char     ver[6];
	uint8_t  unk[2];  /* checksum? */
} __attribute__((packed));

struct mitsu70x_status_resp {
	uint8_t  hdr[4];
	uint8_t  unk[36];
	int16_t  model[6]; /* LE, UTF-16 */
	int16_t  serno[6]; /* LE, UTF-16 */
	struct mitsu70x_status_ver vers[7];
	uint8_t  null[8];
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
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
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

	ctx->type = lookup_printer_type(&mitsu70x_backend,
					desc.idVendor, desc.idProduct);
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
	struct mitsu70x_hdr *mhdr = (struct mitsu70x_hdr*)(hdr + sizeof(struct mitsu70x_hdr));

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
	ctx->cols = be16_to_cpu(mhdr->cols);
	ctx->rows = be16_to_cpu(mhdr->rows);

	remain = ctx->rows * ctx->cols * 2;
	remain = (remain + 511) / 512 * 512; /* Round to nearest 512 bytes. */
	remain *= 3;  /* One for each plane */

	if (mhdr->laminate) {
		i = be16_to_cpu(mhdr->lamcols) * be16_to_cpu(mhdr->lamrows) * 2;
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

	uint16_t tmp;
	
	int num, ret;

	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x56;
	cmdbuf[2] = 0x33;
	cmdbuf[3] = 0x00;
	tmp = cpu_to_be16(ctx->cols);
	memcpy(cmdbuf + 4, &tmp, 2);
	tmp = cpu_to_be16(ctx->rows);
	memcpy(cmdbuf + 6, &tmp, 2);
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

	struct mitsu70x_state rdbuf = { .hdr = 0 }, rdbuf2 = { .hdr = 0 };

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
				     ctx->databuf, sizeof(struct mitsu70x_hdr))))
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
		if (ctx->type == P_MITSU_K60) {
			struct mitsu70x_hdr *hdr = (struct mitsu70x_hdr*) (ctx->databuf + sizeof(struct mitsu70x_hdr));
			/* K60 only has a lower deck */
			hdr->deck = 1;

			/* 4x6 prints on 6x8 media need multicut mode */
			if (ctx->cols == 0x0748 &&
			    ctx->rows == 0x04c2)
				hdr->multicut = 1;
		}

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf + sizeof(struct mitsu70x_hdr),
				     sizeof(struct mitsu70x_hdr))))
			return CUPS_BACKEND_FAILED;

		INFO("Sending data\n");

		{
			/* K60 and 305 need data sent in 256K chunks, but the first
			   chunk needs to subtract the length of the 512-byte header */
			int chunk = 256*1024 - sizeof(struct mitsu70x_hdr);
			int sent = 1024;
			while (ctx->datalen > 0) {
				if ((ret = send_data(ctx->dev, ctx->endp_down,
						     ctx->databuf + sent, chunk)))
					return CUPS_BACKEND_FAILED;
				sent += chunk;
				chunk = ctx->datalen - sent;
				if (chunk > 256*1024)
					chunk = 256*1024;
			}
		}

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
	unsigned int i;
	
	INFO("Model         : ");
	for (i = 0 ; i < 6 ; i++) {
		DEBUG2("%c", le16_to_cpu(resp->model[i]) & 0x7f);
	}
	DEBUG2("\n");
	INFO("Serial Number : ");
	for (i = 0 ; i < 6 ; i++) {
		DEBUG2("%c", le16_to_cpu(resp->serno[i]) & 0x7f);
	}
	DEBUG2("\n");
	for (i = 0 ; i < 7 ; i++) {
		char buf[7];
		if (resp->vers[i].ver[5] == '@')  /* "DUMMY@" */
			continue;
		memcpy(buf, resp->vers[i].ver, 6);
		buf[6] = 0;
		INFO("Component #%d ID: %s (%02x%02x)\n",
		     i, buf, resp->vers[i].unk[0], resp->vers[i].unk[1]);
	}	
	if (resp->upper.present) {  /* IOW, Not present */
		INFO("Prints remaining:  %03d/%03d\n",
		     be16_to_cpu(resp->lower.remain),
		     be16_to_cpu(resp->lower.capacity));
	} else {
		INFO("Prints remaining:  Lower: %03d/%03d\n"
		     "                   Upper: %03d/%03d\n",
		     be16_to_cpu(resp->lower.remain),
		     be16_to_cpu(resp->lower.capacity),
		     be16_to_cpu(resp->upper.remain),
		     be16_to_cpu(resp->upper.capacity));
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

static int mitsu70x_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	int ret, i;
	struct mitsu70x_status_resp resp = { .hdr = { 0 } };

	struct mitsu70x_ctx ctx = {
		.dev = dev,
		.endp_up = endp_up,
		.endp_down = endp_down,
	};

	ret = mitsu70x_get_status(&ctx, &resp);

	if (buf_len > 6)  /* Will we ever have a buffer under 6 bytes? */
		buf_len = 6;
		
	for (i = 0 ; i < buf_len ; i++) {
		*buf++ = le16_to_cpu(resp.serno[i]) & 0x7f;
	}
	*buf = 0; /* Null-terminate the returned string */
	
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

	if (!ctx)
		return -1;

	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "s")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL			
		case 's':
			j = mitsu70x_query_status(ctx);
			break;
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
	.version = "0.32WIP",
	.uri_prefix = "mitsu70x",
	.cmdline_usage = mitsu70x_cmdline,
	.cmdline_arg = mitsu70x_cmdline_arg,
	.init = mitsu70x_init,
	.attach = mitsu70x_attach,
	.teardown = mitsu70x_teardown,
	.read_parse = mitsu70x_read_parse,
	.main_loop = mitsu70x_main_loop,
	.query_serno = mitsu70x_query_serno,
	.devices = {
	{ USB_VID_MITSU, USB_PID_MITSU_D70X, P_MITSU_D70X, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_K60, P_MITSU_K60, ""},
//	{ USB_VID_MITSU, USB_PID_MITSU_D80, P_MITSU_D70X, ""},
	{ USB_VID_KODAK, USB_PID_KODAK305, P_MITSU_K60, ""},
	{ 0, 0, 0, ""}
	}
};

/* Mitsubish CP-D70DW/CP-D707DW/CP-K60DW-S/CP-D80DW/Kodak 305 data format 

   Spool file consists of two headers followed by three image planes
   and an optional lamination data plane.  All blocks are rounded up to
   a 512-byte boundary.

   All multi-byte numbers are big endian, ie MSB first.

   Header 1:  (Init)

   1b 45 57 55 00 00 00 00  00 00 00 00 00 00 00 00
   (padded by NULLs to a 512-byte boundary)

   Header 2:  (Header)

   1b 5a 54 PP 00 00 00 00  00 00 00 00 00 00 00 00
   XX XX YY YY QQ QQ ZZ ZZ  SS 00 00 00 00 00 00 00
   UU 00 00 00 00 00 00 00  00 TT 00 00 00 00 00 00
   RR 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00

   (padded by NULLs to a 512-byte boundary)

   PP    == 0x01 on D70x/D80, 0x02 on K60/305
   XX XX == columns
   YY YY == rows
   QQ QQ == lamination columns (equal to XX XX)
   ZZ ZZ == lamination rows (YY YY + 12)
   SS    == Print mode: 00 = Fine, 03 = SuperFine (D70x/D80 only), 04 = UltraFine
            (Matte requires Superfine or Ultrafine)
   UU    == 00 = Auto, 01 = Lower Deck (required for K60/305), 02 = Upper Deck
   TT    == lamination: 00 glossy, 02 matte.
   RR    == 00 (normal), 01 = (Double-cut 4x6), 05 = (double-cut 2x6)
   
   Data planes:
   16-bit data, rounded up to 512-byte block (XX * YY * 2 bytes)
   
   Lamination plane: (only present if QQ and ZZ are nonzero)
   16-byte data, rounded up to 512-byte block (QQ * ZZ * 2 bytes)

   ********************************************************************

   Command format:

   -> 1b 56 32 30
   <- [256 byte payload]

    PRINTER STATUS

    e4 56 32 30 00 00 00 00  00 00 00 00 00 00 00 00   .V20............
    00 00 00 00 00 00 00 00  00 00 00 80 00 00 00 00   ................
    44 80 00 00 5f 00 00 3d  43 00 50 00 44 00 37 00   D..._..=C.P.D.7.
    30 00 44 00 30 00 30 00  31 00 31 00 31 00 37 00   0.D.0.0.1.1.1.7.
    33 31 36 54 31 33 21 a3  33 31 35 42 31 32 f5 e5   316T13!.315B12..
    33 31 39 42 31 31 a3 fb  33 31 38 45 31 32 50 0d   319B11..318E12P.
    33 31 37 41 32 32 a3 82  44 55 4d 4d 59 40 00 00   317A22..DUMMY@..
    44 55 4d 4d 59 40 00 00  00 00 00 00 00 00 00 00   DUMMY@..........

    LOWER DECK STATUS

    00 00 00 00 00 00 02 04  3f 00 00 04 96 00 00 00  MM MM: media capacity
    ff 0f 01 00 MM MM NN NN  00 00 00 00 05 28 75 80  NN NN: prints remaining
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00

      alt (some sort of error state)

    00 00 00 0a 05 05 01 d5  38 00 00 00 14 00 00 00 
    ff ff ff ff ff ff ff ff  ff ff 00 00 00 27 72 80
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00

    UPPER DECK STATUS (if present)  

    XX XX 00 00 00 00 01 ee  3d 00 00 06 39 00 00 00  MM MM: media capacity
    ff 02 00 00 MM MM NN NN  00 00 00 00 06 67 78 00  NN NN: prints remaining
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00  XX XX: 0x80 00 if no deck
    80 00 80 00 80 00 80 00  80 00 80 00 80 00 80 00

     alt (no deck present)

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

   ** ** ** ** ** **

Various deck status dumps:

0080   00 00 00 00 00 00 01 d2  39 00 00 00 07 00 00 00  ........9.......
0090   61 8f 00 00 01 40 01 36  00 00 00 00 00 17 79 80  a....@.6......y.

0080   00 00 00 00 00 00 01 c6  39 00 00 00 08 00 00 00  ........9.......
0090   61 8f 00 00 01 40 01 35  00 00 00 00 00 18 79 80  a....@.5......y.

0080   00 00 00 00 00 00 02 19  50 00 00 00 19 00 00 01  ........P.......
0090   6c 8f 00 00 01 40 01 22  00 00 00 00 00 27 83 80  l....@.".....'..

0080   00 00 00 00 00 00 02 00  3e 00 00 04 96 00 00 00  ........>.......
0090   ff 0f 01 00 00 c8 00 52  00 00 00 00 05 28 75 80  .......R.....(u.

00c0   00 00 00 00 00 00 01 f3  3d 00 00 06 39 00 00 00  ........=...9...
00d0   ff 02 00 00 01 90 00 c3  00 00 00 00 06 67 78 00  .............gx.

0080   00 00 00 00 00 00 01 d0  38 00 00 03 70 00 00 00  ........8...p...
0090   ff 02 00 00 01 90 00 1e  01 00 00 00 03 83 72 80  ..............r.

0080   00 00 00 00 00 00 01 d6  39 00 00 00 20 00 00 00  ........9... ...
0090   ff 02 00 00 01 90 01 7c  01 00 00 00 00 33 72 80  .......|.....3r.

       00 00 00 0a 05 05 01 d5  38 00 00 00 14 00 00 00 
       ff ff ff ff ff ff ff ff  ff ff 00 00 00 27 72 80   ?? Error ??

       80 00 00 00 00 00 00 ff  ff 00 00 00 00 00 00 00
       ff ff ff ff ff ff ff ff  ff ff 00 00 00 00 80 00   NO DECK PRESENT
 */
