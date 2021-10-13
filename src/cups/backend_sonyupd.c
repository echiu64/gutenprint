/*
 *   Sony UP-D series Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2021 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND sonyupd_backend

#include "backend_common.h"

/* Printer status
  --> 1b e0 00 00 00 00 XX 00   [[ XX is 0xe on UPD895, 0xf on others ]]
  <-- this struct
*/
struct sony_updsts {
	uint8_t  len;      /* 0x0d/0x0e (ie number of bytes AFTER this one) */
	uint8_t  zero1;    /* 0x00 */
	uint8_t  printing; /* UPD_PRINTING_* */
	uint8_t  remain;   /* Number of remaining pages */
	uint8_t  sts0;     /* UPD_STS0_* */
	uint8_t  sts1;     /* UPD_STS1_* */
	uint8_t  sts2;     /* UPD_STS2_* */
	uint8_t  sts3;     /* UPD_STS3_* */
	uint8_t  ribbon;   /* 0x04 = R206/6x8 or C48/4x8 */
	uint8_t  paper;    /* 0x38 = EMPTY, 0xa8/0x90 = loaded */
	uint16_t max_cols; /* BE */
	uint16_t max_rows; /* BE */
	uint8_t  percent;  /* 0-99, if job is printing (UP-D89x) */
} __attribute__((packed));

struct sony_prints {
	uint8_t zero[4];
	uint16_t remain;  /* BE, remaining prints on media */
} __attribute((packed));

#define UPD_PRINTING_BW    0xe0  /* UPD-895/897 only */
#define UPD_PRINTING_Y     0x40
#define UPD_PRINTING_M     0x80
#define UPD_PRINTING_C     0xc0
#define UPD_PRINTING_O     0x20
#define UPD_PRINTING_IDLE  0x00

/* Confirmed on UP-DR200 */
#define UPD_STS0_OK        0x00
#define UPD_STS0_NORIBBON  0x10
#define UPD_STS0_NOPAPER   0x20
#define UPD_STS0_DOOROPEN  0x40

#define UPD_STS1_IDLE      0x00
#define UPD_STS1_DOOROPEN  0x08
#define UPD_STS1_NOPAPER   0x40
#define UPD_STS1_PRINTING  0x80
#define UPD_STS1_PRINTING2 0xC0

#define UPD_RIBBON_R206    0x04
#define UPD_RIBBON_C48     0x04

/* Private data structures */
struct upd_printjob {
	struct dyesub_job_common common;

	uint8_t *databuf;
	int datalen;

	uint16_t rows;
	uint16_t cols;
	uint32_t imglen;
};

struct upd_ctx {
	struct dyesub_connection *conn;

	int native_bpp;

	struct sony_updsts    stsbuf;
	struct sony_prints    printbuf;

	struct marker marker;
};

static const char *upd_ribbons(int type, uint8_t code)
{
	if (type == P_SONY_UPD895 || type == P_SONY_UPD897) {
		return "UP-110 Roll";
	} else if (type == P_SONY_UPCR10) {
		if (code == UPD_RIBBON_C48)
			return "2UPC-C48 (4x8)";
	} else if (type == P_SONY_UPDR150) {
		/* DR200/DR150 */
		if (code == UPD_RIBBON_R206)
			return "R206 (8x6)";
	}

	return "Unknown";
}

static int sonyupd_media_maxes(uint8_t type, uint8_t media)
{
	if (type == P_SONY_UPDR150) {
		if (media == UPD_RIBBON_R206)
			return 350;
		else
			return 700; // XXX guess until we have more codes?

		// XXX also differs for DR200 vs DR150?
	} else if (type == P_SONY_UPCR10) {
		if (media == UPD_RIBBON_C48)
			return 150;
		return 200; // XXX guess until we have more codes.
	}
	return CUPS_MARKER_UNAVAILABLE;
}

// UP-DR200
// 2UPC-R203  3.5x5  (770)
// 2UPC-R204  4x6    (700)
// 2UPC-R205  5x7    (400)
// 2UPC-R206  6x8    (350)

// UP-DR150
// 2UPC-R153         (610)
// 2UPC-R154         (550)
// 2UPC-R155         (335)
// 2UPC-R156         (295)

// UP-CR10L & UP-CX1

// 2UPC-C13          (344)
// 2UPC-C14          (200)
// 2UPC-C15          (172)
// 2UPC-C48          (150)

// print order:  ->YMCO->
// current prints (power on)
// total prints (lifetime)
// f/w version

static const char* upd895_statuses(uint8_t code)
{
	switch (code) {
	case UPD_STS1_IDLE:
		return "Idle";
	case UPD_STS1_DOOROPEN:
		return "Door open";
	case UPD_STS1_NOPAPER:
		return "No paper";
	case UPD_STS1_PRINTING:
	case UPD_STS1_PRINTING2:
		return "Printing";
	default:
		return "Unknown";
	}
}

static const char* updr200_statuses(uint8_t code)
{
	switch (code) {
	case UPD_STS0_OK:
		return "OK";
	case UPD_STS0_DOOROPEN:
		return "Door open";
	case UPD_STS0_NOPAPER:
		return "No paper";
	case UPD_STS0_NORIBBON:
		return "No ribbon";
	default:
		return "Unknown";
	}
}

/* Now for the code */

static int sony_get_status(struct upd_ctx *ctx, struct sony_updsts *buf)
{
	int ret, num = 0;
	uint8_t query[7] = { 0x1b, 0xe0, 0, 0, 0, 0x0f, 0 };

	if (ctx->conn->type == P_SONY_UPD895)
		query[5] = 0x0e;

	if ((ret = send_data(ctx->conn,
			     query, sizeof(query))))
		return CUPS_BACKEND_FAILED;

	ret = read_data(ctx->conn, (uint8_t*) buf, sizeof(*buf),
			&num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;
#if 0
	if (ctx->conn->type == P_SONY_UPD895 && ret != 14)
		return CUPS_BACKEND_FAILED;
	else if (ret != 15)
		return CUPS_BACKEND_FAILED;
#endif

	buf->max_cols = be16_to_cpu(buf->max_cols);
	buf->max_rows = be16_to_cpu(buf->max_rows);

	return CUPS_BACKEND_OK;
}

static int sony_get_prints(struct upd_ctx *ctx, struct sony_prints *buf)
{
	int ret, num = 0;
	uint8_t query[7] = { 0x1b, 0xef, 0, 0, 0, 0x06, 0 };

	if ((ret = send_data(ctx->conn,
			     query, sizeof(query))))
		return CUPS_BACKEND_FAILED;

	ret = read_data(ctx->conn, (uint8_t*) buf, sizeof(*buf),
			&num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	buf->remain = be16_to_cpu(buf->remain);

	return CUPS_BACKEND_OK;
}

static void* upd_init(void)
{
	struct upd_ctx *ctx = malloc(sizeof(struct upd_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct upd_ctx));
	return ctx;
}

static int upd_attach(void *vctx, struct dyesub_connection *conn, uint8_t jobid)
{
	struct upd_ctx *ctx = vctx;

	UNUSED(jobid);

	ctx->conn = conn;

	if (ctx->conn->type == P_SONY_UPD895 || ctx->conn->type == P_SONY_UPD897) {
		ctx->marker.color = "#000000";  /* Ie black! */
		ctx->native_bpp = 1;
	} else {
		ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
		ctx->native_bpp = 3;
	}

	if (test_mode < TEST_MODE_NOATTACH) {
		int ret;
		if ((ret = sony_get_status(ctx, &ctx->stsbuf))) {
			return ret;
		}
		if ((ctx->conn->type != P_SONY_UPD895 && ctx->conn->type != P_SONY_UPD897) && (ret = sony_get_prints(ctx, &ctx->printbuf))) {
			return ret;
		}
	}

	if (test_mode >= TEST_MODE_NOATTACH && getenv("MEDIA_CODE")) {
		ctx->marker.numtype = atoi(getenv("MEDIA_CODE"));
	} else {
		ctx->marker.numtype = ctx->stsbuf.ribbon;
	}

	ctx->marker.name = upd_ribbons(ctx->conn->type, ctx->stsbuf.ribbon);
	if (test_mode >= TEST_MODE_NOATTACH || ctx->conn->type == P_SONY_UPD895 || ctx->conn->type == P_SONY_UPD897) {
		ctx->marker.levelmax = CUPS_MARKER_UNAVAILABLE;
		ctx->marker.levelnow = CUPS_MARKER_UNKNOWN;
	} else {
		ctx->marker.levelmax = sonyupd_media_maxes(ctx->conn->type, ctx->stsbuf.ribbon);
		ctx->marker.levelnow = ctx->printbuf.remain;
	}


	return CUPS_BACKEND_OK;
}

static void upd_cleanup_job(const void *vjob)
{
	const struct upd_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);

	free((void*)job);
}

#define MAX_PRINTJOB_LEN (2048*2764*3 + 2048)

static int upd_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct upd_ctx *ctx = vctx;
	int len, run = 1;
	uint32_t copies_offset = 0;
	uint32_t param_offset = 0;
	uint32_t data_offset = 0;

	struct upd_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));
	job->common.jobsize = sizeof(*job);
	job->common.copies = copies;

	job->datalen = 0;
	job->databuf = malloc(MAX_PRINTJOB_LEN);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		upd_cleanup_job(job);
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	while(run) {
		int i;
		int keep = 0;
		i = read(data_fd, job->databuf + job->datalen, 4);
		if (i < 0) {
			upd_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		if (i == 0)
			break;

		memcpy(&len, job->databuf + job->datalen, sizeof(len));
		len = le32_to_cpu(len);

		/* Filter out chunks we don't send to the printer */
		if (len & 0xf0000000) {
			switch (len) {
			case 0xfffffff3:
				if(dyesub_debug)
					DEBUG("Block ID '%08x' (len %d)\n", len, 0);
				len = 0;
				if (ctx->conn->type == P_SONY_UPDR150)
					run = 0;
				break;
			case 0xfffffff7:
				if(dyesub_debug)
					DEBUG("Block ID '%08x' (len %d)\n", len, 0);
				len = 0;
				if (ctx->conn->type == P_SONY_UPCR10)
					run = 0;
				break;
			case 0xfffffff8: // 895
			case 0xfffffff4: // 897
				if(dyesub_debug)
					DEBUG("Block ID '%08x' (len %d)\n", len, 0);
				len = 0;
				if (ctx->conn->type == P_SONY_UPD895 || ctx->conn->type == P_SONY_UPD897)
					run = 0;
				break;
			case 0xffffff97:
				if(dyesub_debug)
					DEBUG("Block ID '%08x' (len %d)\n", len, 12);
				len = 12;
				break;
			case 0xffffffef:
				if (ctx->conn->type == P_SONY_UPD895 || ctx->conn->type == P_SONY_UPD897) {
					if(dyesub_debug)
						DEBUG("Block ID '%08x' (len %d)\n", len, 0);
					len = 0;
					break;
				}
				/* Intentional Fallthrough */
			case 0xffffffeb:
			case 0xffffffee:
			case 0xfffffff5:
				if(dyesub_debug)
					DEBUG("Block ID '%08x' (len %d)\n", len, 4);
				len = 4;
				break;
			case 0xffffffec:
				if (ctx->conn->type == P_SONY_UPD897) {
					if(dyesub_debug)
						DEBUG("Block ID '%08x' (len %d)\n", len, 4);
					len = 4;
					break;
				}
				/* Intentional Fallthrough */
			default:
				if(dyesub_debug)
					DEBUG("Block ID '%08x' (len %d)\n", len, 0);
				len = 0;
				break;
			}
		} else {
			/* Only keep these chunks */
			if(dyesub_debug)
				DEBUG("Data block (len %d)\n", len);
			if (len > 0)
				keep = 1;
		}
		if (keep)
			job->datalen += sizeof(uint32_t);

		/* Make sure we're not too large */
		if (job->datalen + len > MAX_PRINTJOB_LEN) {
			ERROR("Buffer overflow when parsing printjob! (%d+%d)\n",
			      job->datalen, len);
			upd_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}

		/* Read in the data chunk */
		while(len > 0) {
			i = read(data_fd, job->databuf + job->datalen, len);
			if (i < 0) {
				upd_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			if (i == 0)
				break;

			/* Work out offset of copies command */
			if (job->databuf[job->datalen] == 0x1b) {
				int offset = 0;
				if (i == 7)
					offset = 4;

				switch (job->databuf[job->datalen + 1]) {
				case 0x15: /* Print dimensions */
					param_offset = job->datalen + 16 + offset;
					break;
				// XXX case 0xc0:
				// for param 03, take the value at offset 4 -- for (eg) 4x6 on 8x6 media, needs to be set to 0x02
				case 0xee:
					copies_offset = job->datalen + 7 + offset;
					break;
				case 0xe1: /* Image dimensions */
					param_offset = job->datalen + 14 + offset;
					break;
				case 0xea:
					data_offset = job->datalen + 6 + offset;
					break;
				default:
					break;
				}
			}

			if (keep)
				job->datalen += i;
			len -= i;
		}
	}
	if (!job->datalen) {
		upd_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Some models specify copies in the print job */
	if (copies_offset) {
		uint16_t tmp;
		memcpy(&tmp, job->databuf + copies_offset, sizeof(tmp));
		tmp = be16_to_cpu(tmp);
		if (tmp < copies) {  /* Use whichever one is larger */
			tmp = cpu_to_be16(copies);
			memcpy(job->databuf + copies_offset, &tmp, sizeof(tmp));
		}
		job->common.copies = 1;
	}

	/* Parse some other stuff */
	if (param_offset) {
		memcpy(&job->cols, job->databuf + param_offset, sizeof(uint16_t));
		memcpy(&job->rows, job->databuf + param_offset + 2, sizeof(uint16_t));
		job->cols = be16_to_cpu(job->cols);
		job->rows = be16_to_cpu(job->rows);
	}
	if (data_offset) {
		memcpy(&job->imglen, job->databuf + data_offset, sizeof(uint32_t));
		job->imglen = be32_to_cpu(job->imglen);
	}

	/* Sanity check job parameters */
	if (job->imglen != (uint32_t)(job->rows * job->cols * ctx->native_bpp))
	{
		ERROR("Job data length mismatch (%u vs %d)!\n",
		      job->imglen, job->rows * job->cols * ctx->native_bpp);
		upd_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int upd_main_loop(void *vctx, const void *vjob, int wait_for_return) {
	struct upd_ctx *ctx = vctx;
	int i, ret;
	int copies;

	const struct upd_printjob *job = vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	copies = job->common.copies;

top:
	/* Send Unknown CMD.  Resets? */
	if (ctx->conn->type == P_SONY_UPD897) {
		const uint8_t cmdbuf[7] = { 0x1b, 0x1f, 0, 0, 0, 0, 0 };
		ret = send_data(ctx->conn,
				cmdbuf, sizeof(cmdbuf));
		if (ret)
			return CUPS_BACKEND_FAILED;
	}

	/* Query printer status */
	ret = sony_get_status(ctx, &ctx->stsbuf);

	if (ret)
		return CUPS_BACKEND_FAILED;

	/* Sanity check job parameters */
	if (job->rows > ctx->stsbuf.max_rows ||
	    job->cols > ctx->stsbuf.max_cols) {
		ERROR("Job dimensions (%u/%u) exceed printer max (%u/%u)\n",
		      job->cols, job->rows,
		      ctx->stsbuf.max_cols,
		      ctx->stsbuf.max_rows);
		return CUPS_BACKEND_CANCEL;
	}

	/* Check for idle */
	if (ctx->stsbuf.sts1 != UPD_STS1_IDLE) {
		if (ctx->stsbuf.sts1 == UPD_STS1_PRINTING) {
			INFO("Waiting for printer idle...\n");
			sleep(1);
			goto top;
		} else {
			// XXX some sort of error?
		}
	}

	/* Send RESET */
	if (ctx->conn->type != P_SONY_UPD895) {
		const uint8_t rstbuf[7] = { 0x1b, 0x16, 0, 0, 0, 0, 0 };
		ret = send_data(ctx->conn,
				rstbuf, sizeof(rstbuf));
		if (ret)
			return CUPS_BACKEND_FAILED;
	}

#if 0 /* Unknown query */
	if (ctx->conn->type == P_SONY_UPD897) {
		// -> 1b e6 00 00 00 08 00
		// <- ???
	}
#endif

	/* Send over job */
	i = 0;
	while (i < job->datalen) {
		uint32_t len;
		memcpy(&len, job->databuf + i, sizeof(len));
		len = le32_to_cpu(len);

		i += sizeof(uint32_t);

		if ((ret = send_data(ctx->conn,
				     job->databuf + i, len)))
			return CUPS_BACKEND_FAILED;

		i += len;
	}

	// XXX generate and send copy cmd instead of using the offset.
	// 1b ee 00 00 00 02 00  NN NN  (BE)

	/* Wait for completion! */
retry:
	sleep(1);

	/* Check for idle */
	ret = sony_get_status(ctx, &ctx->stsbuf);
	if (ret)
		return ret;

	switch (ctx->stsbuf.sts1) {
	case UPD_STS1_IDLE:
		goto done;
	case UPD_STS1_PRINTING:
	case UPD_STS1_PRINTING2:
		break;
	default:
		if (ctx->conn->type == P_SONY_UPD895 || ctx->conn->type == P_SONY_UPD897) {
			ERROR("Printer error: %s (%02x)\n", upd895_statuses(ctx->stsbuf.sts1), ctx->stsbuf.sts1);
		} else {
			ERROR("Printer error: %s (%02x)\n", updr200_statuses(ctx->stsbuf.sts0), ctx->stsbuf.sts0);
		}
		return CUPS_BACKEND_STOP;
	}

	if (!wait_for_return && ctx->stsbuf.printing != UPD_PRINTING_IDLE) {
		INFO("Fast return mode enabled.\n");
	} else {
		goto retry;
	}

	/* Clean up */
	if (terminate)
		copies = 1;

done:
	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		goto top;
	}

	return CUPS_BACKEND_OK;
}

static int upd895_dump_status(struct upd_ctx *ctx)
{
	int ret = sony_get_status(ctx, &ctx->stsbuf);
	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	if (ctx->conn->type != P_SONY_UPD895 && ctx->conn->type != P_SONY_UPD897 && (ret = sony_get_prints(ctx, &ctx->printbuf))) {
		return CUPS_BACKEND_FAILED;
	}

	if (ctx->conn->type == P_SONY_UPD895 || ctx->conn->type == P_SONY_UPD897) {
		INFO("Printer status: %s (%02x)\n", upd895_statuses(ctx->stsbuf.sts1), ctx->stsbuf.sts1);
	} else {
		INFO("Printer status: %s (%02x)\n", updr200_statuses(ctx->stsbuf.sts0), ctx->stsbuf.sts0);
	}

	if (ctx->stsbuf.printing != UPD_PRINTING_IDLE &&
	    ctx->stsbuf.sts1 == UPD_STS1_PRINTING)
		INFO("Remaining copies to print: %d\n", ctx->stsbuf.remain);

	INFO("Media: %s (%02x)\n", upd_ribbons(ctx->conn->type, ctx->stsbuf.ribbon), ctx->stsbuf.ribbon);

	if (ctx->conn->type != P_SONY_UPD895 && ctx->conn->type != P_SONY_UPD897) {
		INFO("Media remaining: %d/%d\n", ctx->printbuf.remain, sonyupd_media_maxes(ctx->conn->type, ctx->stsbuf.ribbon));
	}
	return CUPS_BACKEND_OK;
}


static void upd_cmdline(void)
{
	DEBUG("\t\t[ -s ]           # Query printer status\n");
}

static int upd_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct upd_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "s")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 's':
			j = upd895_dump_status(ctx);
			break;
		}

		if (j) return j;
	}

	return CUPS_BACKEND_OK;
}

static int upd_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct upd_ctx *ctx = vctx;
	int ret = sony_get_status(ctx, &ctx->stsbuf);

	*markers = &ctx->marker;
	*count = 1;

	if (ret)
		return CUPS_BACKEND_FAILED;

	if (ctx->conn->type != P_SONY_UPD895 && ctx->conn->type != P_SONY_UPD897 && (ret = sony_get_prints(ctx, &ctx->printbuf))) {
		return CUPS_BACKEND_FAILED;
	}

	if (ctx->stsbuf.sts1 == UPD_STS1_NOPAPER ||
	    ctx->stsbuf.sts1 == UPD_STS1_DOOROPEN) {
		if (ctx->conn->type == P_SONY_UPD895 || ctx->conn->type == P_SONY_UPD897) {
			ctx->marker.levelnow = 0;
		} else {
			ctx->marker.levelnow = ctx->printbuf.remain;
		}
	} else {
		if (ctx->conn->type == P_SONY_UPD895 || ctx->conn->type == P_SONY_UPD897) {
			ctx->marker.levelnow = CUPS_MARKER_UNKNOWN_OK;
		} else {
			ctx->marker.levelnow = ctx->printbuf.remain;
		}
	}

	return CUPS_BACKEND_OK;
}

static const char *sonyupd_prefixes[] = {
	"sonyupd", /* Family Name */
	"dnp-sl10", // Unknown if shared with CR10L
	// Backwards compatibility
	"sonyupdr150", "sonyupdr200", "sonyupcr10",
	NULL
};

const struct dyesub_backend sonyupd_backend = {
	.name = "Sony UP-D",
	.version = "0.46",
	.uri_prefixes = sonyupd_prefixes,
	.cmdline_arg = upd_cmdline_arg,
	.cmdline_usage = upd_cmdline,
	.init = upd_init,
	.attach = upd_attach,
	.cleanup_job = upd_cleanup_job,
	.read_parse = upd_read_parse,
	.main_loop = upd_main_loop,
	.query_markers = upd_query_markers,
	.devices = {
		{ 0x054c, 0x01e8, P_SONY_UPDR150, NULL, "sony-updr150"},
		{ 0x054c, 0x035f, P_SONY_UPDR150, NULL, "sony-updr200"},
		{ 0x054c, 0x0226, P_SONY_UPCR10, NULL, "sony-upcr10l"},
		{ 0x054c, 0x02d4, P_SONY_UPCR10, NULL, "sony-upcx1"},
		{ 0x054c, 0x0049, P_SONY_UPD895, NULL, "sony-upd895"},
		{ 0x054c, 0x01e7, P_SONY_UPD897, NULL, "sony-upd897"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* Sony spool file format

   The spool file is a series of 4-byte commands, followed by optional
   arguments.  The purpose of the commands is unknown, but they presumably
   instruct the driver to perform certain things.

   If you treat these 4 bytes as a 32-bit little-endian number, if any of the
   least significant 4 bits are non-zero, the value is is to
   be interpreted as a driver command.  If the most significant bits are
   zero, the value signifies that the following N bytes of data should be
   sent to the printer as-is.

   Known driver "commands":

    97 ff ff ff
    eb ff ff ff  ?? 00 00 00
    ec ff ff ff  ?? 00 00 00
    ed ff ff ff  ?? 00 00 00
    ee ff ff ff  ?? 00 00 00
    ef ff ff ff  XX 00 00 00   # XX == print size (0x01/0x02/0x03/0x04)
    ef ff ff ff                # On UP-D895/897
    f3 ff ff ff
    f4 ff ff ff                # End of job on UP-D897
    f5 ff ff ff  YY 00 00 00   # YY == ??? (seen 0x01)
    f7 ff ff ff                # End of job on UP-D895

   All printer commands start with 0x1b, and are at least 7 bytes long.
   General Command format:

    1b XX ?? ?? ?? LL 00       # XX is cmd, LL is data or response length.

   UNKNOWN QUERY  [possibly media?]

 <- 1b 03 00 00 00  13 00
 -> 70 00 00 00 00 00 00 0b  00 00 00 00 00 00 00 00
    00 00 00

   UNKNOWN CMD (UP-DR & SL10 & UP-D895 & UP-DP10)  [ possibly START ]

 <- 1b 0a 00 00 00 00 00

   PRINT DIMENSIONS

 <- 1b 15 00 00 00  0d 00
 <- 00 00 00 00 ZZ QQ QQ WW  WW YY YY XX XX

    QQ/WW/YY/XX are (origin_cols/origin_rows/cols/rows) in BE.
    ZZ is 0x07 on UP-DR series, 0x01 on UP-D89x series. plane mask maybe?

   RESET

 <- 1b 16 00 00 00 00 00

   UNKNOWN CMD (UP-DR & SL & UP-D897, may be PRINT START?)

 <- 1b 17 00 00 00 00 00

   UNKNOWN CMD

 <- 1b 1f 00 00 00 00 00

   SET PARAM

 <- 1b c0 00 NN 00 LL 00    # LL is response length, NN is number.
 <- [ LL bytes]

   QUERY PARAM

 <- 1b c1 00 NN 00 LL 00    # LL is response length, NN is number.
 -> [ LL bytes ]

      PARAMS SEEN:
    03, len 5    [ 02 03 00 01 XX ]                   (UPDR200, 00 = normal, 02 is multicut/div2 print, 01 seen at end of stream too..
    02, len 06   [ 02 02 00 03 00 00 ]
    01, len 10   [ 02 01 00 06 00 02 00 00 00 00 ]    (UP-D897)
    00, len 5    [ 02 01 00 01 XX ]                   XX == Gamma table

   STATUS QUERY

 <- 1b e0 00 00 00  XX 00       # XX = 0xe (UP-D895), 0xf (All others)
 -> [14 or 15 bytes, see 'struct sony_updsts' ]

   IMAGE DIMENSIONS & OVERCOAT

 <- 1b e1 00 00 00  0b 00
 <- 00 ZZ QQ 00 00 00 00 XX XX YY YY  # XX = cols, YY == rows, ZZ == 0x04 on UP-DP10, otherwise 0x80. QQ == 00 glossy, 08 texture (UP-DP10 + UP-DR150), 0c matte, +0x10 for "nocorrection" on UP-DR200..

   UNKNOWN

 <- 1b e5 00 00 00  08 00
 <- 00 00 00 00 00 00 00 XX  00  # Seen 01, 12, 0d, etc.

   UNKNOWN  (UP-D897)

 <- 1b e6 00 00 00  08 00
 <- 07 00 00 00 00 00 00 00

   DATA TRANSFER

 <- 1b ea 00 00 00 00 ZZ ZZ ZZ ZZ 00  # ZZ is BIG ENDIAN
 <- [ ZZ ZZ ZZ ZZ bytes of data ]

   UNKNOWN CMD (UP-DR and UP-D)

 <- 1b ed 00 00 00  00 00

   QUERY REMAINING PRINTS (UPDR series)

 <- 1b ef 00 00 00  06 00
 -> 05 00 00 00 NN NN      # NN NN  print count on media remaining

   COPIES

 <- 1b ee 00 00 00  02 00
 <- NN NN                        # Number of copies (BE, 1-???)

   UNKNOWN (UPDR series)

 <- 1b f5 00 00 00  02 00
 <- ?? ??

  ************************************************************************

  The data stream sent to the printer consists of all the commands in the
  spool file, plus a couple other ones that generate responses.  It is
  unknown if those additional commands are necessary.  This is a typical
  sequence:

[[ Sniff start of a UP-DR150 ]]

<- 1b e0 00 00 00 0f 00   [ STATUS QUERY ]
-> 0e 00 00 00 00 00 00 00  04 a8 08 00 0a a4 00
                                  ----- -----
                                  MAX_C MAX_R
<- 1b 16 00 00 00 00 00
-> "reset" ??

[[ begin job ]]

<- 1b ef 00 00 00 06 00
-> 05 00 00 00 00 22

<- 1b e5 00 00 00 08 00       ** In spool file
<- 00 00 00 00 00 00 01 00

<- 1b c1 00 02 06 00 00       [ Query Param 2, length 6 ]
-> 02 02 00 03 00 00

<- 1b ee 00 00 00 02 00       ** In spool file
<- 00 01

<- 1b 15 00 00 00 0d 00       ** In spool file
<- 00 00 00 00 07 00 00 00  00 08 00 0a a4

<- 1b 03 00 00 00 13 00
-> 70 00 00 00 00 00 00 0b  00 00 00 00 00 00 00 00
   00 00 00

<- 1b e1 00 00 00 0b 00       ** In spool file
<- 00 80 00 00 00 00 00 08  00 0a a4

<- 1b 03 00 00 00 13 00
-> 70 00 00 00 00 00 00 0b  00 00 00 00 00 00 00 00
   00 00 00

<- 1b ea 00 00 00 00 00 ff  60 00 00   ** In spool file
<- [[ 0x0060ff00 bytes of data ]]

<- 1b e0 00 00 00 0f 00
-> 0e 00 00 00 00 00 00 00  04 a8 08 00 0a a4 00

<- 1b 0a 00 00 00 00 00   ** In spool file
<- 1b 17 00 00 00 00 00   ** In spool file

[[fin]]

 **************

  Sony UP-CL10 / DNP SL-10 / UP-CX1 spool format:

60 ff ff ff
f8 ff ff ff
fd ff ff ff        (or: f0 ff ff ff)
14 00 00 00
1b 15 00 00 00 0d 00 00  00 00 00 07 00 00 00 00  WW WW HH HH
fb ff ff ff
f4 ff ff ff
0b 00 00 00
1b ea 00 00 00 00 SH SH  SH SH 00
SL SL SL SL

 [[ Data, rows * cols * 3 bytes ]]

f3 ff ff ff
0f 00 00 00
1b e5 00 00 00 08 00 00  00 00 00 00 00 00 00
12 00 00 00
1b e1 00 00 00 0b 00 00  80 GG 00 00 00 00 WW WW  HH HH
fa ff ff ff
09 00 00 00
1b ee 00 00 00 02 00 00  NN
07 00 00 00
1b 0a 00 00 00 00 00
f9 ff ff ff
fc ff ff ff
07 00 00 00
1b 17 00 00 00 00 00
f7 ff ff ff

 WW WW == Columns, Big Endian
 HH HH == Rows, Big Endian
 SL SL SL SL == Plane size, Little Endian (Rows * Cols * 3)
 SH SH SH SH == Plane size, Big Endian (Rows * Cols * 3)
 NN == Copies
 GG == Overcoat, 0x00 glossy, 0x0c matte (UP-CX1 only)

 **************

  Sony UP-D895 spool format:

 XX XX == cols, BE (fixed at 1280/0x500)
 YY YY == rows, BE (798/0x031e,1038/0x040e,1475/0x05c3, 2484/09b4) @ 960/1280/1920/3840+4096
 SS SS SS SS == data len (rows * cols, LE)
 S' S' S' S' == data len (rows * cols, BE)
 NN  == copies (1 -> ??)
 GG GG == ???  0000/0050/011b/04aa/05aa at each resolution.
 G' == Gamma  01 (soft), 03 (hard), 02 (normal)

 9c ff ff ff 97 ff ff ff  00 00 00 00 00 00 00 00  00 00 00 00 ff ff ff ff

 14 00 00 00
 1b 15 00 00 00  0d 00  00 00 00 00 01 GG GG 00 00 YY YY XX XX
 0b 00 00 00
 1b ea 00 00 00  00 S' S' S' S' 00
 SS SS SS SS
 ...DATA... (rows * cols)
 ff ff ff ff
 09 00 00 00
 1b ee 00 00 00  02 00  00 NN
 0f 00 00 00
 1b e5 00 00 00  08 00  00 00 00 00 00 00 00 00
 0c 00 00 00
 1b c0 00 00 00  05 00  02 00  00 01  G'
 11 00 00 00
 1b c0 00 01 00  0a 00  02 01  00 06  00 00 00 00 00 00
 12 00 00 00
 1b e1 00 00 00  0b 00  00 08 00 GG GG 00 00 YY YY XX XX
 07 00 00 00
 1b 0a 00 00 00  00 00
 fd ff ff ff f7 ff ff ff  f8 ff ff ff

 **************

  Sony UP-D897 spool format:

 NN NN == copies  (00 for printer selected)
 XX XX == cols (fixed @ 1280)
 YY YY == rows
 GG    == gamma -- Table 2 == 2, Table 1 == 3, Table 3 == 1, Table 4 == 4
 DD    == "dark"  +- 64.
 LL    == "light" +- 64.
 AA    == "advanced" +- 32.
 SS    == Sharpness 0-14
 ZZ ZZ ZZ ZZ == Data length (BE)
 Z` Z` Z` Z` == Data length (LE)


 83 ff ff ff fc ff ff ff
 fb ff ff ff f5 ff ff ff
 f1 ff ff ff f0 ff ff ff
 ef ff ff ff

 14 00 00 00
 1b 15 00 00 00  0d 00  00 00 00 00 01 00 a2 00 00 YY YY XX XX

 0b 00 00 00
 1b ea 00 00 00  00 ZZ ZZ  ZZ ZZ 00

 Z` Z` Z` Z`
 ...DATA...

 ea ff ff ff

 09 00 00 00
 1b ee 00 00 00  02 00  NN NN

 ee ff ff ff 01 00 00 00

 0e 00 00 00
 1b e5 00 00 00  08 00  00 00 00 00 DD LL SS AA

 eb ff ff ff ?? 00 00 00   <--- 02/05   5 at #3, 2 otherwise.  Sharpness?

 0c 00 00 00
 1b c0 00 00 00  05 00  02 00  00 01  GG

 ec ff ff ff ?? 00 00 00   <--- 01/00/02/01/01  Seen.  Unknown.

 11 00 00 00
 1b c0 00 01 00  0a 00  02 01  00 06  00 00 00 00 00 00

 ed ff ff ff 00 00 00 00

 12 00 00 00
 1b e1 00 00 00  0b 00  00 08 00 00 a2 00 00 YY YY XX XX

 fa ff ff ff

 07 00 00 00
 1b 0a 00 00 00 00 00

 fc ff ff ff fd ff ff ff ff ff ff ff

 07 00 00 00
 1b 17 00 00 00 00 00

 f4 ff ff ff

 ****************

 UP-D895 comms protocol:

 <-- 1b e0 00 00 00 0e 00
 --> 0d 00 XX YY 00 SS 00 ZZ  00 00 10 00 05 00

  XX : 0xe0 when printing, 0x00 otherwise.
  YY : Number of remaining copies
  SS : Status
       0x00 = Idle
       0x08 = Door open
       0x40 = Paper empty
       0x80 = Printing
       ??   = Cooling down
       ??   = Busy / Waiting
  ZZ : Status2
       0x01 = Print complete
       0x02 = no prints yet

 UP-D897 comms protocol:

 <-- 1b e0 00 00 00 0f 00
 --> 0e 00 XX YY 00 SS RR 01  02 02 10 00 05 00 PP

  XX : 0xe0 when printing, 0x00 otherwise.
  YY : Number of remaining copies
  SS : Status
       0x00 = Idle
??       0x08 = Door open
       0x40 = Paper empty
       0x80 = Printing
       ??   = Cooling down
       ??   = Busy / Waiting
  RR : Status 2
       0x00 = Okay
       0x08 = ?? Error state?
       0x80 = Printing
  PP : Percentage complete (0-99)

Other commands seen:

 <-- 1b 16 00 00 00 00 00   -- Reset

 <-- 1b c1 00 01 00 0a 00   -- Query ID 1, legth 10
 --> 02 01 00 06 00 02 00 00  00 00

 <-- 1b c1 00 00 00 05 00   -- Query id 0, length 5
 --> 02 01 00 01 03

 <-- 1b e6 00 00 00 08 00
 --> 07 00 00 00 00 00 00 00

 <-- 1b 17 00 00 00 00 00   -- Unknown?

 CR10L: status progression when printing:

 0e 00 00 00 00 00 00 00  04 a0 04 e0 07 38 00
 0e 00 00 01 00 80 00 01  04 a0 04 e0 07 38 64
 0e 00 40 01 00 80 00 01  04 a0 04 e0 07 38 64  <-- Y
 0e 00 80 01 00 80 00 01  04 a0 04 e0 07 38 64  <-- M
 0e 00 c0 01 00 80 00 01  04 a0 04 e0 07 38 64  <-- C
 0e 00 20 01 00 80 00 01  04 a0 04 e0 07 38 64  <-- O
 0e 00 20 01 00 80 00 01  04 a0 04 e0 07 38 00
 0e 00 00 01 00 80 00 01  04 a0 04 e0 07 38 00
 0e 00 00 00 00 00 00 00  04 a0 04 e0 07 38 00

 UP-DR200 comms protocol:

 <-- 1b e0 00 00 00 0f 00
 --> 0e 00 XX NN E0 E1 00 E3  RT PT CC CC RR RR %%

  XX : Print state (eg Y/M/C/O -- see UPD_PRINTING_* )
  E0 : Error code0 (see UPD_STS0_*)
  E1 : Error code1 (see UPD_STS1_*)
  E3 : Unknown (00 or 01)
  NN : Number of remaining copies  (00 when complete/idle)
  CC : Max Cols (0x0800 on 6" ribbon)
  RR : Max Rows (0x0aa4 on 6x8)
  RT : Ribbon type (04 = R206/8x6)
  PT : Paper type  (38 = empty, a8 = 6")
  %% : Percentage complete (0-99)

 Seen on UP-DR200:

 0e 00 00 00 00 00 00 00  04 a8 08 00 0a a4 00   READY   (R206 6x8 ribbon)
 0e 00 00 00 40 00 00 00  04 a8 08 00 0a a4 00   DOOR OPEN
 0e 00 00 00 20 00 00 00  04 38 08 00 0a a4 00   LOAD PAPER
 0e 00 00 00 10 08 00 00  00 a8 08 00 0a a4 00   LOAD RIBBON

 0e 00 00 01 00 c0 00 01  04 a8 08 00 0a a4 00   PRINT START
 0e 00 40 01 00 c0 00 01  04 a8 08 00 0a a4 %%   PRINT Y
 0e 00 80 01 00 c0 00 01  04 a8 08 00 0a a4 %%   PRINT M
 0e 00 c0 01 00 c0 00 01  04 a8 08 00 0a a4 %%   PRINT C
 0e 00 20 01 00 c0 00 01  04 a8 08 00 0a a4 %%   PRINT OC
 0e 00 00 00 00 00 00 00  04 a8 08 00 0a a4 00   PRINT DONE / READY

*/
