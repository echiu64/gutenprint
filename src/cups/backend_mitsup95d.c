/*
 *   Mitsubishi P93D/P95D Monochrome Thermal Photo Printer CUPS backend
 *
 *   (c) 2016-2021 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Development of this backend was sponsored by:
 *
 *     A benefactor who wishes to remain anonymous
 *
 *   The latest version of this program can be found at:
 *
 *     https://git.shaftnet.org/cgit/selphy_print.git
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 *   SPDX-License-Identifier: GPL-2.0+
 *
 */

#define BACKEND mitsup95d_backend

#include "backend_common.h"

/* Private data structure */
struct mitsup95d_printjob {
	struct dyesub_job_common common;

	uint8_t *databuf;
	uint32_t datalen;

	uint8_t hdr[2];  // 1b 51
	uint8_t hdr1[50]; // 1b 57 20 2e ...
	uint8_t hdr2[50]; // 1b 57 21 2e ...
	uint8_t hdr3[50]; // 1b 57 22 2e ...
	uint8_t hdr4[42];  // 1b 58 ...
	int hdr4_len;      // 36 (P95) or 42 (P93)

	uint8_t plane[12]; // 1b 5a 74 00 ...

	uint8_t mem_clr[4]; // 1b 5a 43 00
	int mem_clr_present;

	uint8_t ftr[2];
};

struct mitsup95d_ctx {
	struct dyesub_connection *conn;

	char serno[STR_LEN_MAX + 1];

	struct marker marker;
};

#define QUERYRESP_SIZE_MAX 9

static const char *mitsup93d_errors(uint8_t code)
{
	switch (code) {
	case 0x6f: return "Door Open";
	case 0x50: return "No Paper";
	default:   return "Unknown Error";
	}
}

static const char *mitsup95d_errors(uint8_t code)
{
	switch (code & 0xf) {
	case 3: return "Door Open";
	case 4: return "No Paper";
	default: return "Unknown Error";
	}
}

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

static int mitsup95d_get_status(struct mitsup95d_ctx *ctx, uint8_t *resp)
{
	uint8_t querycmd[4] = { 0x1b, 0x72, 0x00, 0x00 };
	int ret;
	int num;

	/* P93D is ... special.  Windows switches to this halfway through
	   but it seems be okay to use it everywhere */
	if (ctx->conn->type == P_MITSU_P93D) {
		querycmd[2] = 0x03;
	}

	/* Query Status to sanity-check job */
	if ((ret = send_data(ctx->conn,
			     querycmd, sizeof(querycmd))))
		return CUPS_BACKEND_FAILED;
	ret = read_data(ctx->conn,
			resp, QUERYRESP_SIZE_MAX, &num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;
	if (ctx->conn->type == P_MITSU_P95D && num != 9) {
		return CUPS_BACKEND_FAILED;
	} else if (ctx->conn->type == P_MITSU_P93D && num != 8) {
		return CUPS_BACKEND_FAILED;
	}
	return CUPS_BACKEND_OK;
}

static int mitsup95d_attach(void *vctx, struct dyesub_connection *conn, uint8_t jobid)
{
	struct mitsup95d_ctx *ctx = vctx;

	UNUSED(jobid);

	ctx->conn = conn;

	ctx->marker.color = "#000000";  /* Ie black! */
	ctx->marker.name = "Unknown";
	ctx->marker.numtype = -1;
	ctx->marker.levelmax = CUPS_MARKER_UNAVAILABLE;
	ctx->marker.levelnow = CUPS_MARKER_UNKNOWN;

	if (test_mode >= TEST_MODE_NOATTACH)
		goto done;

	/* Query serial number */
	{
		struct libusb_device_descriptor desc;
		struct libusb_device *udev;

		udev = libusb_get_device(ctx->conn->dev);
		libusb_get_device_descriptor(udev, &desc);

		if (!desc.iSerialNumber) {
			WARNING("Printer configured for iSerial mode U0, so no serial number is reported.\n");
		} else {
			libusb_get_string_descriptor_ascii(ctx->conn->dev, desc.iSerialNumber, (uint8_t*)ctx->serno, STR_LEN_MAX);

			if (strstr(ctx->serno, "000000")) {
				WARNING("Printer configured for iSerial mode U2, reporting a fixed serial number of 000000\n");
			}
		}
	}

done:
	return CUPS_BACKEND_OK;
}

static void mitsup95d_cleanup_job(const void *vjob)
{
	const struct mitsup95d_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);

	free((void*)job);
}

static int mitsup95d_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct mitsup95d_ctx *ctx = vctx;
	uint8_t buf[2];  /* Enough to read in any header */
	uint8_t tmphdr[50];
	uint8_t *ptr;
	int i;
	int remain;
	int ptr_offset;

	struct mitsup95d_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));
	job->common.jobsize = sizeof(*job);

	job->mem_clr_present = 0;

top:
	i = read(data_fd, buf, sizeof(buf));

	if (i == 0) {
		mitsup95d_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}
	if (i < 0) {
		mitsup95d_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}
	if (buf[0] != 0x1b) {
		ERROR("malformed data stream\n");
		mitsup95d_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	switch (buf[1]) {
	case 0x50: /* Footer */
		remain = 2;
		ptr = job->ftr;
		break;
	case 0x51: /* Job Header */
		remain = 2;
		ptr = job->hdr;
		break;
	case 0x57: /* Geeneral headers */
		remain = sizeof(tmphdr);
		ptr = tmphdr;
		break;
	case 0x58: /* User Comment */
		if (ctx->conn->type == P_MITSU_P93D)
			job->hdr4_len = 42;
		else
			job->hdr4_len = 36;
		remain = job->hdr4_len;
		ptr = job->hdr4;
		break;
	case 0x5a: /* Plane header OR printer reset */
		// reset memory: 1b 5a 43 ...  [len 04]
		// plane header: 1b 5a 74 ...  [len 12]
		// Read in the minimum length, and clean it up later */
		ptr = tmphdr;
		remain = 4;
		break;
	default:
		ERROR("Unrecognized command! (%02x %02x)\n", buf[0], buf[1]);
		mitsup95d_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	memcpy(ptr, buf, sizeof(buf));
	remain -= sizeof(buf);
	ptr_offset = sizeof(buf);

	while (remain) {
		i = read(data_fd, ptr + ptr_offset, remain);
		if (i == 0) {
			mitsup95d_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		if (i < 0) {
			mitsup95d_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		remain -= i;
		ptr_offset += i;

		/* Handle the ambiguous 0x5a block */
		if (buf[1] == 0x5a && remain == 0) {
			if (tmphdr[2] == 0x74) { /* plane header */
				ptr = job->plane;
				remain = 12 - ptr_offset; /* Finish reading */
			} else if (tmphdr[2] == 0x43) { /* reset memory */
				ptr = job->mem_clr;
				job->mem_clr_present = 1;
				remain = 4 - ptr_offset;
			}
			memcpy(ptr, tmphdr, ptr_offset);
			buf[1] = 0xff;
		}
	}

	if (ptr == tmphdr) {
		if (tmphdr[3] != 46) {
			ERROR("Unexpected header chunk: %02x %02x %02x %02x\n",
			      tmphdr[0], tmphdr[1], tmphdr[2], tmphdr[3]);
			mitsup95d_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		switch (tmphdr[2]) {
		case 0x20:
			ptr = job->hdr1;
			break;
		case 0x21:
			ptr = job->hdr2;
			break;
		case 0x22:
			ptr = job->hdr3;
			break;
		default:
			WARNING("Unexpected header chunk: %02x %02x %02x %02x\n",
			      tmphdr[0], tmphdr[1], tmphdr[2], tmphdr[3]);
		}
		memcpy(ptr, tmphdr, sizeof(tmphdr));
	} else if (ptr == job->plane) {
		uint16_t rows = job->plane[10] << 8 | job->plane[11];
		uint16_t cols = job->plane[8] << 8 | job->plane[9];

		remain = rows * cols;

		/* Allocate buffer for the payload */
		job->datalen = 0;
		job->databuf = malloc(remain);
		if (!job->databuf) {
			ERROR("Memory allocation failure!\n");
			mitsup95d_cleanup_job(job);
			return CUPS_BACKEND_RETRY_CURRENT;
		}

		/* Read it in */
		while (remain) {
			i = read(data_fd, job->databuf + job->datalen, remain);
			if (i == 0) {
				mitsup95d_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			if (i < 0) {
				mitsup95d_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			remain -= i;
			job->datalen += i;
		}
	} else if (ptr == job->ftr) {

		/* Update unknown header field to match sniffs */
		if (ctx->conn->type == P_MITSU_P95D) {
			if (job->hdr1[18] == 0x00)
				job->hdr1[18] = 0x01;
		}

		/* Update printjob header to reflect number of requested copies */
		if (job->hdr2[13] != 0xff)
			if (copies > job->hdr2[13])
				job->hdr2[13] = copies;

		job->common.copies = copies; // XXX use larger?

		*vjob = job;
		return CUPS_BACKEND_OK;
	}

	goto top;
}

static int mitsup95d_main_loop(void *vctx, const void *vjob, int wait_for_return) {
	struct mitsup95d_ctx *ctx = vctx;
	uint8_t queryresp[QUERYRESP_SIZE_MAX];
	int ret;

	const struct mitsup95d_printjob *job = vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;


	INFO("Waiting for printer idle\n");

        /* Query Status to make sure printer is idle */
	do {
		ret = mitsup95d_get_status(ctx, queryresp);
		if (ret)
			return ret;

		if (ctx->conn->type == P_MITSU_P95D) {
			if (queryresp[6] & 0x40) {
				INFO("Printer Status: %s (%02x)\n", mitsup95d_errors(queryresp[6]), queryresp[6]);
				return CUPS_BACKEND_STOP;
			}
			if (queryresp[5] == 0x00)
				break;
		} else {
			if (queryresp[6] == 0x45) {
				ERROR("Printer error %02x\n", queryresp[7]);
				return CUPS_BACKEND_STOP;
			}
			if (queryresp[6] == 0x30)
				break;
		}

		sleep(1);
	} while (1);

	INFO("Sending print job\n");

	/* Send over Memory Clear, if present */
	if (job->mem_clr_present) {
		if ((ret = send_data(ctx->conn,
				     job->mem_clr, sizeof(job->mem_clr))))
			return CUPS_BACKEND_FAILED;
	}

	/* Send Job Start */
	if ((ret = send_data(ctx->conn,
			     job->hdr, sizeof(job->hdr))))
		return CUPS_BACKEND_FAILED;

	/* Send over headers */
	if ((ret = send_data(ctx->conn,
			     job->hdr1, sizeof(job->hdr1))))
		return CUPS_BACKEND_FAILED;
	if ((ret = send_data(ctx->conn,
			     job->hdr2, sizeof(job->hdr2))))
		return CUPS_BACKEND_FAILED;
	if ((ret = send_data(ctx->conn,
			     job->hdr3, sizeof(job->hdr3))))
		return CUPS_BACKEND_FAILED;
	if ((ret = send_data(ctx->conn,
			     job->hdr4, job->hdr4_len)))
		return CUPS_BACKEND_FAILED;

	/* Send plane header and image data */
	if ((ret = send_data(ctx->conn,
			     job->plane, sizeof(job->plane))))
		return CUPS_BACKEND_FAILED;
	if ((ret = send_data(ctx->conn,
			     job->databuf, job->datalen)))
		return CUPS_BACKEND_FAILED;

	/* Query Status to sanity-check job */
	ret = mitsup95d_get_status(ctx, queryresp);
	if (ret)
		return ret;

	if (ctx->conn->type == P_MITSU_P95D) {
		if (queryresp[6] & 0x40) {
			INFO("Printer Status: %s (%02x)\n", mitsup95d_errors(queryresp[6]), queryresp[6]);
			return CUPS_BACKEND_STOP;
		}
		if (queryresp[5] != 0x00) {
			ERROR("Printer not ready (%02x)!\n", queryresp[5]);
			return CUPS_BACKEND_CANCEL;
		}
	} else {
		if (queryresp[6] == 0x45) {
			INFO("Printer Status: %s (%02x)\n", mitsup93d_errors(queryresp[7]), queryresp[7]);
			return CUPS_BACKEND_STOP;
		}
		if (queryresp[6] != 0x30) {
			ERROR("Printer not ready (%02x)!\n", queryresp[6]);
			return CUPS_BACKEND_CANCEL;
		}
	}

	/* Send over Footer */
	if ((ret = send_data(ctx->conn,
			     job->ftr, sizeof(job->ftr))))
		return CUPS_BACKEND_FAILED;

	INFO("Waiting for completion\n");

	/* Query status until we're done.. */
	do {
		sleep(1);

		/* Query Status */
		ret = mitsup95d_get_status(ctx, queryresp);
		if (ret)
			return ret;

		if (ctx->conn->type == P_MITSU_P95D) {
			if (queryresp[6] & 0x40) {
				INFO("Printer Status: %s (%02x)\n", mitsup95d_errors(queryresp[6]), queryresp[6]);
				return CUPS_BACKEND_STOP;
			}
			if (queryresp[5] == 0x00)
				break;

			if (queryresp[7] > 0) {
				if (!wait_for_return) {
					INFO("Fast return mode enabled.\n");
					break;
				}
			}
		} else {
			if (queryresp[6] == 0x45) {
				INFO("Printer Status: %s (%02x)\n", mitsup93d_errors(queryresp[7]), queryresp[7]);
				return CUPS_BACKEND_STOP;
			}
			if (queryresp[6] == 0x30)
				break;
			if (queryresp[6] == 0x43 && queryresp[7] > 0) {
				if (!wait_for_return) {
					INFO("Fast return mode enabled.\n");
					break;
				}
			}
		}
	} while(1);

	INFO("Print complete\n");
	return CUPS_BACKEND_OK;
}

static int mitsup95d_dump_status(struct mitsup95d_ctx *ctx)
{
	uint8_t queryresp[QUERYRESP_SIZE_MAX];
	int ret;

	ret = mitsup95d_get_status(ctx, queryresp);
	if (ret)
		return ret;
	if (!ctx->serno[0])
		INFO("iSerial mode: Disasbled (U0)\n");
	else if (strstr(ctx->serno, "000000"))
		INFO("iSerial Mode: Force 000000 (U2)\n");
	else {
		INFO("iSerial Mode: Enabled (U1)\n");
		INFO("Serial Number: %s\n", ctx->serno);
	}

	if (ctx->conn->type == P_MITSU_P95D) {
		if (queryresp[6] & 0x40) {
			INFO("Printer Status: %s (%02x)\n", mitsup95d_errors(queryresp[6]), queryresp[6]);
		} else if (queryresp[5] == 0x00) {
			INFO("Printer Status: Idle\n");
		} else if (queryresp[5] == 0x02 && queryresp[7] > 0) {
			INFO("Printer Status: Printing (%d) copies remaining\n", queryresp[7]);
		}
	} else {
		if (queryresp[6] == 0x45) {
			INFO("Printer Status: %s (%02x)\n", mitsup93d_errors(queryresp[7]), queryresp[7]);
		} else if (queryresp[6] == 0x30) {
			INFO("Printer Status: Idle\n");
		} else if (queryresp[6] == 0x43 && queryresp[7] > 0) {
			INFO("Printer Status: Printing (%d) copies remaining\n", queryresp[7]);
		}
	}

	return CUPS_BACKEND_OK;
}

static void mitsup95d_cmdline(void)
{
	DEBUG("\t\t[ -s ]           # Query status\n");
}

static int mitsup95d_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct mitsup95d_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "s")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 's':
			j = mitsup95d_dump_status(ctx);
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return CUPS_BACKEND_OK;
}

static int mitsup95d_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct mitsup95d_ctx *ctx = vctx;
	uint8_t queryresp[QUERYRESP_SIZE_MAX];

	if (mitsup95d_get_status(ctx, queryresp))
		return CUPS_BACKEND_FAILED;

	ctx->marker.levelnow = CUPS_MARKER_UNKNOWN_OK;

	if (ctx->conn->type == P_MITSU_P95D) {
		if (queryresp[6] & 0x40) {
			ctx->marker.levelnow = 0;
		}
	} else {
		if (queryresp[6] == 0x45) {
			ctx->marker.levelnow = 0;
		}
	}

	*markers = &ctx->marker;
	*count = 1;

	return CUPS_BACKEND_OK;
}

static const char *mitsup95d_prefixes[] = {
	"mitsup9x", // Family driver name
	// backwards compatibility
	"mitsup95d", "mitsup93d",
	NULL
};

/* Exported */
const struct dyesub_backend mitsup95d_backend = {
	.name = "Mitsubishi P93D/P95D",
	.version = "0.16",
	.uri_prefixes = mitsup95d_prefixes,
	.cmdline_arg = mitsup95d_cmdline_arg,
	.cmdline_usage = mitsup95d_cmdline,
	.init = mitsup95d_init,
	.attach = mitsup95d_attach,
	.cleanup_job = mitsup95d_cleanup_job,
	.read_parse = mitsup95d_read_parse,
	.main_loop = mitsup95d_main_loop,
	.query_markers = mitsup95d_query_markers,
	.devices = {
		{ 0x06d3, 0x0398, P_MITSU_P93D, NULL, "mitsubishi-p93d"},
		{ 0x06d3, 0x3b10, P_MITSU_P95D, NULL, "mitsubishi-p95d"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/*****************************************************

 Mitsubishi P93D/P95D Spool Format

   ...All fields are BIG ENDIAN.

 MEMORY_CLEAR  (optional)

  1b 5a 43 00

 JOB_HDR

  1b 51

 PRINT_SETUP

  1b 57 20 2e  00 0a 00 ZZ   00 00 00 00  00 00 CC CC
  RR RR XX 00  00 00 00 00   00 00 00 00  00 00 00 00
  00 00 00 00  00 00 00 00   00 00 00 00  00 00 00 00
  00 00

   XX == 01 seen in sniffs, 00 seen in dumps.  Unknown purpose.
   ZZ == 00 on P93D, 02 on P95D

   CC CC = columns, RR RR = rows (print dimensions)

 PRINT_OPTIONS

   P95:

  1b 57 21 2e  00 4a aa 00   20 TT 00 00  64 NN 00 MM
  [[ 00 00 00  00 00 00 00   00 00 00 00  00 00 00 00
  00 ]] 00 00  00 02 00 00   00 00 00 00  00 00 00 00
  00 XY

   P93:

  1b 57 21 2e  00 4a aa 00   00 TT 00 00  00 NN 00 MM
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

         P95D:

        00 = Standard
        01 = High Density
        02 = High Glossy
        03 = High Glossy (K95HG)

         P93D:

        00 = High Density
        01 = High Glossy
        02 = Standard

   X = media cut length   (P95D ONLY.  P93 is 0)
        4..8  (mm)
   Y = flags
        0x04 = Paper save
        0x03 = Buzzer (3 = high, 2 = low, 0 = off)

 GAMMA  (P95)

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

 GAMMA  (P93)

  1b 57 22 2e  00 d5 00 00   00 00 00 00  SS 00 LL 00
  BB 00 CC 00  00 00 00 00   00 00 00 00  00 00 00 00
  00 00 00 00  00 00 00 00   00 00 00 00  00 00 00 00
  00 00

   SS = Sharpening (0 = low, 1 = normal, 2 = high)
   LL = Gamma table
        00..04  Gamma table 1..5
   BB = Brightness (signed 8-bit)
   CC = Contrast (signed 8-bit)

 USER_COMMENT  (P95)

  1b 58 [[ 20  20 20 20 20  20 20 20 20  20 20 20 20
  20 20 20 20  20 20 20 20  20 20 20 20  20 20 20 20
  20 20 20 ]]

   [[ .. ]] = Actual comment.  34 bytes payload, 0x20 -> 0x7e
               (Null terminated?)

 USER_COMMENT  (P93)

  1b 58 [[ 20  20 20 20 20  20 20 20 20  20 20 20 20
  20 20 20 20  20 20 20 20  20 20 20 20  20 20 20 20
  20 20 20 20  20 20 20 20  20 20 ]]

   [[ .. ]] = Actual comment.  40 bytes payload, 0x20 -> 0x7e
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

 P95D Printer Comms:

 STATUS query

 -> 1b 72 00 00
 <- e4 72 00 00  04 XX ZZ YY  00

  YY == remaining copies
  XX == Status?
    00 == Idle
    02 == Printing
  ZZ == Error!
    00 == None
    43 == Door open
    44 == No Paper
    4? == "Button"
    4? == "Gear Lock"
    4? == Head Up
    ^
    \--- 0x40 appears to be a flag that indicates error.

 P93D Printer Comms:

 STATUS query

 -> 1b 72 0? 00
 <- e4 72 0? 00  03 XX YY ZZ

  ? could be 0x00 or 0x03.  Seen both.

Seen:   30 30 30
        30 43 01   <- 1 copies remaining
        30 43 00   <- 0 copies remaining
           ^^
            \-- 30 == idle, 43 == printing

        30 45 6f  <- door open
        30 45 50  <- no paper

           45 == error?
 ****************************

UNKNOWNS:

 * How multiple images are stacked for printing on a single page
   (col offset too?  write four, then tell PRINT?)
 * How to adjust P95D printer sharpness?
 * What "custom gamma" table does to spool file?

*/
