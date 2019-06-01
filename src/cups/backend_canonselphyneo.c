/*
 *   Canon SELPHY CPneo series CUPS backend -- libusb-1.0 version
 *
 *   (c) 2016-2018 Solomon Peachy <pizza@shaftnet.org>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define BACKEND canonselphyneo_backend

#include "backend_common.h"

/* Exported */
#define USB_VID_CANON        0x04a9
#define USB_PID_CANON_CP820  0x327b
#define USB_PID_CANON_CP910  0x327a
#define USB_PID_CANON_CP1000 0x32ae
#define USB_PID_CANON_CP1200 0x32b1
#define USB_PID_CANON_CP1300 0x32db

/* Header data structure */
struct selphyneo_hdr {
	uint8_t data[24];
	uint32_t cols;  /* LE */
	uint32_t rows;  /* LE */
} __attribute((packed));

/* Readback data structure */
struct selphyneo_readback {
	uint8_t data[12];
} __attribute((packed));

/* Private data structure */
struct selphyneo_printjob {
	uint8_t *databuf;
	uint32_t datalen;

	int copies;
};

struct selphyneo_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	struct marker marker;
};

static char *selphyneo_statuses(uint8_t sts)
{
	switch(sts) {
	case 0x01:
		return "Idle";
	case 0x02:
		return "Feeding Paper";
	case 0x04:
		return "Printing YELLOW";
	case 0x08:
		return "Printing MAGENTA";
	case 0x10:
		return "Printing CYAN";
	case 0x20:
		return "Printing LAMINATE";
	default:
		return "Unknown state!";
	}
}

static char *selphyneo_errors(uint8_t err)
{
	switch(err) {
	case 0x00:
		return "None";
	case 0x02:
		return "Paper Feed";
	case 0x03:
		return "No Paper";
	case 0x05:
		return "Incorrect Paper loaded";
	case 0x06:
		return "Ink Cassette Empty";
	case 0x07:
		return "No Ink";
	case 0x09:
		return "No Paper and Ink";
	case 0x0A:
		return "Incorrect media for job";
	case 0x0B:
		return "Paper jam";
	default:
		return "Unknown Error";
	}
}

static char *selphynew_pgcodes(uint8_t type) {

	switch (type & 0xf) {
	case 0x01:
		return "P";
	case 0x02:
		return "L";
	case 0x03:
		return "C";
	case 0x00:
		return "None";
	default:
		return "Unknown";
	}
}

static int selphyneo_send_reset(struct selphyneo_ctx *ctx)
{
	uint8_t rstcmd[12] = { 0x40, 0x10, 0x00, 0x00,
			       0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00, 0x00, 0x00 };
	int ret;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     rstcmd, sizeof(rstcmd))))
		return CUPS_BACKEND_FAILED;

	return CUPS_BACKEND_OK;
}

static int selphyneo_get_status(struct selphyneo_ctx *ctx)
{
	struct selphyneo_readback rdback;
	int ret, num;

	/* Read in the printer status to clear last state */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) &rdback, sizeof(rdback), &num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	/* And again, for the markers */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) &rdback, sizeof(rdback), &num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	INFO("Printer state: %s\n", selphyneo_statuses(rdback.data[0]));
	INFO("Media type: %s\n", selphynew_pgcodes(rdback.data[6]));
	if (rdback.data[2]) {
		INFO("Printer error: %s\n", selphyneo_errors(rdback.data[2]));
	}

	return CUPS_BACKEND_OK;
}

static void *selphyneo_init(void)
{
	struct selphyneo_ctx *ctx = malloc(sizeof(struct selphyneo_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct selphyneo_ctx));

	return ctx;
}

extern struct dyesub_backend selphyneo_backend;

static int selphyneo_attach(void *vctx, struct libusb_device_handle *dev, int type,
			    uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct selphyneo_ctx *ctx = vctx;
	struct selphyneo_readback rdback;
	int ret, num;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;
	ctx->type = type;

	if (test_mode < TEST_MODE_NOATTACH) {
		/* Read in the printer status to clear last state */
		ret = read_data(ctx->dev, ctx->endp_up,
				(uint8_t*) &rdback, sizeof(rdback), &num);

		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* And again, for the markers */
		ret = read_data(ctx->dev, ctx->endp_up,
				(uint8_t*) &rdback, sizeof(rdback), &num);

		if (ret < 0)
			return CUPS_BACKEND_FAILED;
	} else {
		rdback.data[2] = 0;
		rdback.data[6] = 0x01;
		if (getenv("MEDIA_CODE"))
			rdback.data[6] = atoi(getenv("MEDIA_CODE"));
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = selphynew_pgcodes(rdback.data[6]);
	ctx->marker.levelmax = -1;
	if (rdback.data[2]) {
		ctx->marker.levelnow = 0;
	} else {
		ctx->marker.levelnow = -3;
	}

	return CUPS_BACKEND_OK;
}

static void selphyneo_cleanup_job(const void *vjob) {
	const struct selphyneo_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);

	free((void*)job);
}

static int selphyneo_read_parse(void *vctx, const void **vjob, int data_fd, int copies)
{
	struct selphyneo_ctx *ctx = vctx;
	struct selphyneo_hdr hdr;
	int i, remain;

	struct selphyneo_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));
	job->copies = copies;

	/* Read the header.. */
	i = read(data_fd, &hdr, sizeof(hdr));
	if (i != sizeof(hdr)) {
		if (i == 0) {
			selphyneo_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		ERROR("Read failed (%d/%d)\n",
		      i, (int)sizeof(hdr));
		perror("ERROR: Read failed");
		selphyneo_cleanup_job(job);
		return CUPS_BACKEND_FAILED;
	}

	/* Determine job length */
	switch(hdr.data[18]) {
	case 0x50:  /* P */
	case 0x4c:  /* L */
	case 0x43:  /* C */
		remain = le32_to_cpu(hdr.cols) * le32_to_cpu(hdr.rows) * 3;
		break;
	default:
		ERROR("Unknown print size! (%02x, %ux%u)\n",
		      hdr.data[10], le32_to_cpu(hdr.cols), le32_to_cpu(hdr.rows));
		selphyneo_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	// XXX Sanity check job against loaded media?

	/* Allocate a buffer */
	job->datalen = 0;
	job->databuf = malloc(remain + sizeof(hdr));
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		selphyneo_cleanup_job(job);
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	/* Store the read-in header */
	memcpy(job->databuf, &hdr, sizeof(hdr));
	job->datalen += sizeof(hdr);

	/* Read in data */
	while (remain > 0) {
		i = read(data_fd, job->databuf + job->datalen, remain);
		if (i < 0) {
			selphyneo_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		remain -= i;
		job->datalen += i;
	}

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int selphyneo_main_loop(void *vctx, const void *vjob) {
	struct selphyneo_ctx *ctx = vctx;
	struct selphyneo_readback rdback;

	int ret, num;
	int copies;

	const struct selphyneo_printjob *job = vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	copies = job->copies;

	/* Read in the printer status to clear last state */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) &rdback, sizeof(rdback), &num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;

top:
	INFO("Waiting for printer idle\n");

	do {
		ret = read_data(ctx->dev, ctx->endp_up,
				(uint8_t*) &rdback, sizeof(rdback), &num);

		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		if (rdback.data[0] == 0x01)
			break;

		INFO("Printer state: %s\n", selphyneo_statuses(rdback.data[0]));

		switch (rdback.data[2]) {
		case 0x00:
			break;
		case 0x0A:
			ERROR("Printer error: %s (%02x)\n", selphyneo_errors(rdback.data[2]), rdback.data[2]);
			ctx->marker.levelnow = 0;
			dump_markers(&ctx->marker, 1, 0);
			return CUPS_BACKEND_CANCEL;
		default:
			ERROR("Printer error: %s (%02x)\n", selphyneo_errors(rdback.data[2]), rdback.data[2]);
			ctx->marker.levelnow = 0;
			dump_markers(&ctx->marker, 1, 0);
			return CUPS_BACKEND_STOP;
		}

		sleep(1);
	} while(1);

	dump_markers(&ctx->marker, 1, 0);

	INFO("Sending spool data\n");
	/* Send the data over in 256K chunks */
	{
		int chunk = 256*1024;
		int sent = 0;
		while (chunk > 0) {
			if ((ret = send_data(ctx->dev, ctx->endp_down,
					     job->databuf + sent, chunk)))
				return CUPS_BACKEND_FAILED;
			sent += chunk;
			chunk = job->datalen - sent;
			if (chunk > 256*1024)
				chunk = 256*1024;
		}
	}

	/* Read in the printer status to clear last state */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) &rdback, sizeof(rdback), &num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	INFO("Waiting for printer acknowledgement\n");
	do {
		ret = read_data(ctx->dev, ctx->endp_up,
				(uint8_t*) &rdback, sizeof(rdback), &num);

		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		if (rdback.data[0] == 0x01)
			break;

		INFO("Printer state: %s\n", selphyneo_statuses(rdback.data[0]));

		switch (rdback.data[2]) {
		case 0x00:
			break;
		case 0x0A:
			ERROR("Printer error: %s (%02x)\n", selphyneo_errors(rdback.data[2]), rdback.data[2]);
			ctx->marker.levelnow = 0;
			dump_markers(&ctx->marker, 1, 0);
			return CUPS_BACKEND_CANCEL;
		default:
			ERROR("Printer error: %s (%02x)\n", selphyneo_errors(rdback.data[2]), rdback.data[2]);
			ctx->marker.levelnow = 0;
			dump_markers(&ctx->marker, 1, 0);
			return CUPS_BACKEND_STOP;
		}

		if (rdback.data[0] > 0x02 && fast_return && copies <= 1) {
			INFO("Fast return mode enabled.\n");
			break;
		}

		sleep(1);
	} while(1);

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		goto top;
	}

	return CUPS_BACKEND_OK;
}

static int selphyneo_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct selphyneo_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "Rs")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'R':
			selphyneo_send_reset(ctx);
			break;
		case 's':
			selphyneo_get_status(ctx);
			break;
		}

		if (j) return j;
	}

	return 0;
}

static void selphyneo_cmdline(void)
{
	DEBUG("\t\t[ -R ]           # Reset printer\n");
	DEBUG("\t\t[ -s ]           # Query printer status\n");
}

static int selphyneo_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct selphyneo_ctx *ctx = vctx;
	struct selphyneo_readback rdback;
	int ret, num;

	/* Read in the printer status to clear last state */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) &rdback, sizeof(rdback), &num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	/* And again, for the markers */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) &rdback, sizeof(rdback), &num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	if (rdback.data[2])
		ctx->marker.levelnow = 0;
	else
		ctx->marker.levelnow = -3;

	*markers = &ctx->marker;
	*count = 1;

	return CUPS_BACKEND_OK;
}

static const char *canonselphyneo_prefixes[] = {
	"canonselphyneo", // Family name
	"canon-cp820", "canon-cp910", "canon-cp1000", "canon-cp1200", "canon-cp1300",
	// backwards compatibility
	"selphycp820", "selphycp910", "selphycp1000", "selphycp1200", "selphycp1300",
	NULL
};

struct dyesub_backend canonselphyneo_backend = {
	.name = "Canon SELPHY CP (new)",
	.version = "0.20",
	.uri_prefixes = canonselphyneo_prefixes,
	.cmdline_usage = selphyneo_cmdline,
	.cmdline_arg = selphyneo_cmdline_arg,
	.init = selphyneo_init,
	.attach = selphyneo_attach,
	.cleanup_job = selphyneo_cleanup_job,
	.read_parse = selphyneo_read_parse,
	.main_loop = selphyneo_main_loop,
	.query_markers = selphyneo_query_markers,
	.devices = {
		{ USB_VID_CANON, USB_PID_CANON_CP820, P_CP910, NULL, "canon-cp820"},
		{ USB_VID_CANON, USB_PID_CANON_CP910, P_CP910, NULL, "canon-cp910"},
		{ USB_VID_CANON, USB_PID_CANON_CP1000, P_CP910, NULL, "canon-cp1000"},
		{ USB_VID_CANON, USB_PID_CANON_CP1200, P_CP910, NULL, "canon-cp1200"},
		{ USB_VID_CANON, USB_PID_CANON_CP1300, P_CP910, NULL, "canon-cp1300"},
		{ 0, 0, 0, NULL, NULL}
	}
};
/*

 ***************************************************************************

	Stream formats and readback codes for supported printers

 ***************************************************************************
 Selphy CP820/CP910/CP1000/CP1200:

  Radically different spool file format!  300dpi, same print sizes, but also
  adding a 50x50mm sticker and 22x17.3mm ministickers, though I think the
  driver treats all of those as 'C' sizes for printing purposes.

  32-byte header:

  0f 00 00 40 00 00 00 00  00 00 00 00 00 00 01 00
  01 00 TT 00 00 00 00 ZZ  XX XX XX XX YY YY YY YY

                           cols (le32) rows (le32)
        50                 e0 04       50 07          1248 * 1872  (P)
        4c                 80 04       c0 05          1152 * 1472  (L)
        43                 40 04       9c 02          1088 * 668   (C)

  TT == 50  (P)
     == 4c  (L)
     == 43  (C)

  ZZ == 00  Y'CbCr data follows
     == 01  CMY    data follows

  Followed by three planes of image data.

  P == 7008800  == 2336256 * 3 + 32 (1872*1248)
  L == 5087264  == 1695744 * 3 + 32 (1472*1152)
  C == 2180384  == 726784 * 3 + 32  (1088*668)

  It is worth mentioning that the Y'CbCr image data is surmised to use the
  JPEG coefficients, although we realistically have no way of confirming this.

  Other questions:

    * Printer supports different lamination types, how to control?

 Data Readback:

 XX 00 YY 00  00 00 ZZ 00 00 00 00 00

 XX == Status

   01  Idle
   02  Feeding Paper
   04  Printing Y
   08  Printing M
   10  Printing C
   20  Printing L

 YY == Error

   00  None
   02  No Paper (?)
   03  No Paper
   05  Wrong Paper
   07  No Ink
   09  No Paper and Ink
   0A  Media/Job mismatch
   0B  Paper Jam

 ZZ == Media?

   01
   10
   11
    ^-- Ribbon
   ^-- Paper

   1 == P
   2 == L ??
   3 == C

Also, the first time a readback happens after plugging in the printer:

34 44 35 31  01 00 01 00  01 00 45 00      "4D51" ...??


** ** ** ** This is what windows sends if you print over the network:

00 00 00 00 40 00 00 00  02 00 00 00 00 00 04 00  Header [unknown]
00 00 02 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00

00 00 01 00 HH HH HH HH  02 00 00 00 PP PP PP PP  Header2 [unknown] PP == payload len, HH == payload + header2 len [ie + 3 ]
CC CC CC CC RR RR RR RR  00 00 00 00 LL LL LL LL  CC == cols, RR == rows, LL == plane len (ie RR * CC)
L2 L2 L2 L2                                       L2 == LL * 2, apparently.

[ ..followed by three planes of LL bytes, totalling PP bytes.. ]

*/
