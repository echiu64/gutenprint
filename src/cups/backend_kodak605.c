/*
 *   Kodak 605 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2018 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND kodak605_backend

#include "backend_common.h"

#define USB_VID_KODAK       0x040A
#define USB_PID_KODAK_605   0x402E

/* Command Header */
struct kodak605_cmd {
	uint16_t cmd; /* LE */
	uint16_t len; /* LE, not counting this header */
} __attribute__((packed));

struct kodak605_sts_hdr {
	uint8_t  result;    /* RESULT_* */
        uint8_t  unk_1[5];  /* 00 00 00 00 00 */
        uint8_t  sts_1;     /* 01/02 */
        uint8_t  sts_2;     /* 00/61->6b ?? temperature? */
        uint16_t length;    /* LE, not counting this header */
} __attribute__((packed));

#define RESULT_SUCCESS 0x01
#define RESULT_FAIL    0x02

/* Media structure */
struct kodak605_medium {
	uint8_t  index;
	uint16_t cols;   /* LE */
	uint16_t rows;   /* LE */
	uint8_t  type;   /* MEDIA_TYPE_* */
	uint8_t  unk[4]; /* 00 00 00 00 */
}  __attribute__((packed));

#define MEDIA_TYPE_UNKNOWN 0x00
#define MEDIA_TYPE_PAPER   0x01

struct kodak605_media_list {
	struct kodak605_sts_hdr hdr;
	uint8_t  unk;  /* always seen 02 */
	uint8_t  type; /* KODAK68x0_MEDIA_* */
	uint8_t  count;
	struct kodak605_medium entries[];
} __attribute__((packed));

#define KODAK68x0_MEDIA_6R   0x0b // 197-4096
#define KODAK68x0_MEDIA_UNK  0x03
#define KODAK68x0_MEDIA_6TR2 0x2c // 396-2941
#define KODAK68x0_MEDIA_NONE 0x00
/* 6R: Also seen: 101-0867, 141-9597, 659-9054, 169-6418, DNP 900-060 */

#define MAX_MEDIA_LEN 128

/* Status response */
struct kodak605_status {
	struct kodak605_sts_hdr hdr;
/*@10*/	uint32_t ctr_life;  /* Lifetime Prints */
	uint32_t ctr_maint; /* Prints since last maintenance */
	uint32_t ctr_media; /* Prints on current media */
	uint32_t ctr_cut;   /* Cutter Actuations */
	uint32_t ctr_head;  /* Prints on current head */
/*@30*/	uint8_t  donor;     /* Donor Percentage remaining */
/*@31*/	uint8_t  null_1[7]; /* 00 00 00 00 00 00 00 */
/*@38*/	uint8_t  b1_id;     /* 00/01/02 */
	uint16_t b1_remain;
	uint16_t b1_complete;
	uint16_t b1_total;
/*@45*/	uint8_t  b1_sts;    /* See BANK_STATUS_* */
	uint8_t  b2_id;     /* 00/01/02 */
	uint16_t b2_remain;
	uint16_t b2_complete;
	uint16_t b2_total;
/*@53*/	uint8_t  b2_sts;    /* see BANK_STATUS_* */
/*@54*/	uint8_t  id;        /* current job id ( 00/01/02 seen ) */
/*@55*/ uint16_t remain;    /* in current job */
/*@57*/	uint16_t complete;  /* in current job */
/*@59*/	uint16_t total;     /* in current job */
/*@61*/	uint8_t  null_2[9]; /* 00 00 00 00 00 00 00 00 00 */
/*@70*/	uint8_t  unk_12[6]; /* 01 00 00 00 00 00 */
} __attribute__((packed));

/* File header */
struct kodak605_hdr {
	uint8_t  hdr[4];   /* 01 40 0a 00 */
	uint8_t  jobid;
	uint16_t copies;   /* LE, 0x0001 or more */
	uint16_t columns;  /* LE, always 0x0734 */
	uint16_t rows;     /* LE */
	uint8_t  media;    /* 0x03 for 6x8, 0x01 for 6x4 */
	uint8_t  laminate; /* 0x02 to laminate, 0x01 for not */
	uint8_t  mode;     /* Print mode -- 0x00, 0x01 seen */
} __attribute__((packed));

#define BANK_STATUS_FREE      0x00
#define BANK_STATUS_XFER      0x01
#define BANK_STATUS_FULL      0x02
#define BANK_STATUS_PRINTING  0x12

static char *bank_statuses(uint8_t v)
{
        switch (v) {
        case BANK_STATUS_FREE:
                return "Free";
        case BANK_STATUS_XFER:
                return "Xfer";
        case BANK_STATUS_FULL:
                return "Full";
        case BANK_STATUS_PRINTING:
                return "Printing";
        default:
                return "Unknown";
        }
}

static const char *kodak68xx_mediatypes(int type)
{
	switch(type) {
	case KODAK68x0_MEDIA_NONE:
		return "No media";
	case KODAK68x0_MEDIA_6R:
	case KODAK68x0_MEDIA_6TR2:
		return "Kodak 6R";
	default:
		return "Unknown";
	}
	return "Unknown";
}

#define CMDBUF_LEN 4

/* Private data structure */
struct kodak605_printjob {
	struct kodak605_hdr hdr;
	uint8_t *databuf;
	int datalen;
};

struct kodak605_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;
	uint8_t jobid;

	struct kodak605_media_list *media;

	struct marker marker;

};

static int kodak605_get_media(struct kodak605_ctx *ctx, struct kodak605_media_list *media)
{
	uint8_t cmdbuf[4];

	int ret, num = 0;

	/* Send Media Query */
	cmdbuf[0] = 0x02;
	cmdbuf[1] = 0x00;
	cmdbuf[2] = 0x00;
	cmdbuf[3] = 0x00;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;

	/* Read in the printer status */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) media, MAX_MEDIA_LEN, &num);
	if (ret < 0)
		return ret;

	if (num < (int)sizeof(*media)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*media));
		return CUPS_BACKEND_FAILED;
	}

        if (media->hdr.result != RESULT_SUCCESS) {
                ERROR("Unexpected response from media query (%x)!\n", media->hdr.result);
                return CUPS_BACKEND_FAILED;
        }

	return 0;
}

static int kodak605_get_status(struct kodak605_ctx *ctx, struct kodak605_status *sts)
{
	uint8_t cmdbuf[4];

	int ret, num = 0;

	/* Send Status Query */
	cmdbuf[0] = 0x01;
	cmdbuf[1] = 0x00;
	cmdbuf[2] = 0x00;
	cmdbuf[3] = 0x00;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;

	/* Read in the printer status */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) sts, sizeof(*sts), &num);
	if (ret < 0)
		return ret;

	if (num < (int)sizeof(*sts)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*sts));
		return CUPS_BACKEND_FAILED;
	}

	if (sts->hdr.result != RESULT_SUCCESS) {
		ERROR("Unexpected response from status query (%x)!\n", sts->hdr.result);
		return CUPS_BACKEND_FAILED;
	}

	return 0;
}

static void *kodak605_init(void)
{
	struct kodak605_ctx *ctx = malloc(sizeof(struct kodak605_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct kodak605_ctx));

	ctx->media = malloc(MAX_MEDIA_LEN);

	return ctx;
}

static int kodak605_attach(void *vctx, struct libusb_device_handle *dev, int type,
			   uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct kodak605_ctx *ctx = vctx;

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;
	ctx->type = type;

	/* Make sure jobid is sane */
	ctx->jobid = jobid & 0x7f;
	if (!ctx->jobid)
		ctx->jobid++;

	if (test_mode < TEST_MODE_NOATTACH) {
		/* Query media info */
		if (kodak605_get_media(ctx, ctx->media)) {
			ERROR("Can't query media\n");
			return CUPS_BACKEND_FAILED;
		}
	} else {
		ctx->media->type = KODAK68x0_MEDIA_6R;
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = kodak68xx_mediatypes(ctx->media->type);
	ctx->marker.levelmax = 100; /* Ie percentage */
	ctx->marker.levelnow = -2;

	return CUPS_BACKEND_OK;
}

static void kodak605_cleanup_job(const void *vjob)
{
	const struct kodak605_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);

	free((void*)job);
}

static void kodak605_teardown(void *vctx) {
	struct kodak605_ctx *ctx = vctx;

	if (!ctx)
		return;

	free(ctx);
}

static int kodak605_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct kodak605_ctx *ctx = vctx;
	int ret;

	struct kodak605_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_CANCEL;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));

	/* Read in then validate header */
	ret = read(data_fd, &job->hdr, sizeof(job->hdr));
	if (ret < 0 || ret != sizeof(job->hdr)) {
		if (ret == 0)
			return CUPS_BACKEND_CANCEL;
		ERROR("Read failed (%d/%d/%d)\n",
		      ret, 0, (int)sizeof(job->hdr));
		perror("ERROR: Read failed");
		return CUPS_BACKEND_CANCEL;
	}

	if (job->hdr.hdr[0] != 0x01 ||
	    job->hdr.hdr[1] != 0x40 ||
	    job->hdr.hdr[2] != 0x0a ||
	    job->hdr.hdr[3] != 0x00) {
		ERROR("Unrecognized data format!\n");
		return CUPS_BACKEND_CANCEL;
	}

	job->datalen = le16_to_cpu(job->hdr.rows) * le16_to_cpu(job->hdr.columns) * 3;
	job->databuf = malloc(job->datalen);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	{
		int remain = job->datalen;
		uint8_t *ptr = job->databuf;
		do {
			ret = read(data_fd, ptr, remain);
			if (ret < 0) {
				ERROR("Read failed (%d/%d/%d)\n",
				      ret, remain, job->datalen);
				perror("ERROR: Read failed");
				return CUPS_BACKEND_CANCEL;
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}

	/* Printer handles generating copies.. */
	if (le16_to_cpu(job->hdr.copies) < copies)
		job->hdr.copies = cpu_to_le16(copies);

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int kodak605_main_loop(void *vctx, const void *vjob) {
	struct kodak605_ctx *ctx = vctx;

	struct kodak605_status sts;

	int num, ret;

	const struct kodak605_printjob *job = vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	struct kodak605_hdr hdr;
	memcpy(&hdr, &job->hdr, sizeof(hdr));

	/* Validate against supported media list */
	for (num = 0 ; num < ctx->media->count; num++) {
		if (ctx->media->entries[num].rows == hdr.rows &&
		    ctx->media->entries[num].cols == hdr.columns)
			break;
	}
	if (num == ctx->media->count) {
		ERROR("Print size unsupported by media!\n");
		return CUPS_BACKEND_HOLD;
	}

	INFO("Waiting for printer idle\n");

	while(1) {
		if ((ret = kodak605_get_status(ctx, &sts)))
			return CUPS_BACKEND_FAILED;

		if (ctx->marker.levelnow != sts.donor) {
			ctx->marker.levelnow = sts.donor;
			dump_markers(&ctx->marker, 1, 0);
		}

		// XXX check for errors

		/* Make sure we're not colliding with an existing
		   jobid */
		while (ctx->jobid == sts.b1_id ||
		       ctx->jobid == sts.b2_id) {
			ctx->jobid++;
			ctx->jobid &= 0x7f;
			if (!ctx->jobid)
				ctx->jobid++;
		}

		/* Wait for a free buffer */
		if (sts.b1_sts == BANK_STATUS_FREE ||
		    sts.b2_sts == BANK_STATUS_FREE) {
			break;
		}

		sleep(1);
	}

	/* Use specified jobid */
	hdr.jobid = ctx->jobid;

	{
		INFO("Sending image header (internal id %u)\n", ctx->jobid);
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*)&hdr, sizeof(hdr))))
			return CUPS_BACKEND_FAILED;

		struct kodak605_sts_hdr resp;
		if ((ret = read_data(ctx->dev, ctx->endp_up,
				     (uint8_t*) &resp, sizeof(resp), &num)))
			return CUPS_BACKEND_FAILED;

		if (resp.result != RESULT_SUCCESS) {
			ERROR("Unexpected response from print command (%x)!\n", resp.result);
			return CUPS_BACKEND_FAILED;
		}
		// XXX what about resp.sts1 or resp.sts2?
	}
	sleep(1);

	INFO("Sending image data\n");
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     job->databuf, job->datalen)))
		return CUPS_BACKEND_FAILED;

	INFO("Image data sent\n");

	INFO("Waiting for printer to acknowledge completion\n");
	do {
		sleep(1);
		if ((kodak605_get_status(ctx, &sts)) != 0)
			return CUPS_BACKEND_FAILED;

		if (ctx->marker.levelnow != sts.donor) {
			ctx->marker.levelnow = sts.donor;
			dump_markers(&ctx->marker, 1, 0);
		}
		// XXX check for errors

		/* Wait for completion */
		if (sts.b1_id == ctx->jobid && sts.b1_complete == sts.b1_total)
			break;
		if (sts.b2_id == ctx->jobid && sts.b2_complete == sts.b2_total)
			break;

		if (fast_return) {
			INFO("Fast return mode enabled.\n");
			break;
		}
	} while(1);

	INFO("Print complete\n");

	return CUPS_BACKEND_OK;
}

static void kodak605_dump_status(struct kodak605_ctx *ctx, struct kodak605_status *sts)
{
	INFO("Bank 1: %s Job %03u @ %03u/%03u\n",
	     bank_statuses(sts->b1_sts), sts->b1_id,
	     le16_to_cpu(sts->b1_complete), le16_to_cpu(sts->b1_total));
	INFO("Bank 2: %s Job %03u @ %03u/%03u\n",
	     bank_statuses(sts->b2_sts), sts->b2_id,
	     le16_to_cpu(sts->b2_complete), le16_to_cpu(sts->b2_total));

	INFO("Lifetime prints   : %u\n", be32_to_cpu(sts->ctr_life));
	INFO("Cutter actuations : %u\n", be32_to_cpu(sts->ctr_cut));
	INFO("Head prints       : %u\n", be32_to_cpu(sts->ctr_head));
	INFO("Media prints      : %u\n", be32_to_cpu(sts->ctr_media));
	{
		int max;

		switch(ctx->media->type) {
		case KODAK68x0_MEDIA_6R:
 		case KODAK68x0_MEDIA_6TR2:
			max = 375;
			break;
		default:
			max = 0;
			break;
		}

		if (max) {
			INFO("\t  Remaining   : %u\n", max - be32_to_cpu(sts->ctr_media));
		} else {
			INFO("\t  Remaining   : Unknown\n");
		}
	}

	INFO("Donor             : %u%%\n", sts->donor);
}

static void kodak605_dump_mediainfo(struct kodak605_media_list *media)
{
	int i;

        if (media->type == KODAK68x0_MEDIA_NONE) {
                DEBUG("No Media Loaded\n");
                return;
        }

	switch (media->type) {
	case KODAK68x0_MEDIA_6R:
		INFO("Media type: 6R (Kodak 197-4096 or equivalent)\n");
		break;
	case KODAK68x0_MEDIA_6TR2:
		INFO("Media type: 6R (Kodak 396-2941 or equivalent)\n");
		break;
	default:
		INFO("Media type %02x (unknown, please report!)\n", media->type);
		break;
	}

	DEBUG("Legal print sizes:\n");
	for (i = 0 ; i < media->count ; i++) {
		DEBUG("\t%d: %ux%u\n", i,
		      le16_to_cpu(media->entries[i].cols),
		      le16_to_cpu(media->entries[i].rows));
	}
	DEBUG("\n");
}

#define UPDATE_SIZE 1536
static int kodak605_set_tonecurve(struct kodak605_ctx *ctx, char *fname)
{
	libusb_device_handle *dev = ctx->dev;
	uint8_t endp_down = ctx->endp_down;
	uint8_t endp_up = ctx->endp_up;

	uint8_t cmdbuf[16];
	uint8_t respbuf[16];
	int ret, num = 0;

	uint16_t *data = malloc(UPDATE_SIZE);

	INFO("Set Tone Curve from '%s'\n", fname);

	/* Read in file */
	int tc_fd = open(fname, O_RDONLY);
	if (tc_fd < 0) {
		ret = -1;
		goto done;
	}
	if (read(tc_fd, data, UPDATE_SIZE) != UPDATE_SIZE) {
		ret = 4;
		goto done;
	}
	close(tc_fd);

	/* Byteswap data to printer's format */
	for (ret = 0; ret < (UPDATE_SIZE/2) ; ret++) {
		data[ret] = cpu_to_le16(be16_to_cpu(data[ret]));
	}

	/* Initial Request */
	cmdbuf[0] = 0x04;
	cmdbuf[1] = 0xc0;
	cmdbuf[2] = 0x0a;
	cmdbuf[3] = 0x00;
	cmdbuf[4] = 0x03;
	cmdbuf[5] = 0x01;
	cmdbuf[6] = 0x00;
	cmdbuf[7] = 0x00;
	cmdbuf[8] = 0x00;
	cmdbuf[9] = 0x00;
	cmdbuf[10] = 0x00; /* 00 06 in LE means 1536 bytes */
	cmdbuf[11] = 0x06;
	cmdbuf[12] = 0x00;
	cmdbuf[13] = 0x00;

	if ((ret = send_data(dev, endp_down,
			     cmdbuf, 14)))
		goto done;

	/* Get response back */
	ret = read_data(dev, endp_up,
			respbuf, sizeof(respbuf), &num);
	if (ret < 0)
		goto done;

	if (num != 10) {
		ERROR("Short Read! (%d/%d)\n", num, 10);
		ret = 4;
		goto done;
	}

	/* Send the data over! */
	ret = send_data(dev, endp_up,
			(uint8_t*)data, sizeof(data));

 done:
	/* We're done */
	free(data);
	return ret;
}


static void kodak605_cmdline(void)
{
	DEBUG("\t\t[ -C filename ]  # Set tone curve\n");
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
}

static int kodak605_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct kodak605_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "C:ms")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'C':
			j = kodak605_set_tonecurve(ctx, optarg);
			break;
		case 'm':
			kodak605_dump_mediainfo(ctx->media);
			break;
		case 's': {
			struct kodak605_status sts;

			j = kodak605_get_status(ctx, &sts);
			if (!j)
				kodak605_dump_status(ctx, &sts);
			break;
		}
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
}

static int kodak605_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct kodak605_ctx *ctx = vctx;
	struct kodak605_status sts;

	/* Query printer status */
	if (kodak605_get_status(ctx, &sts))
		return CUPS_BACKEND_FAILED;

	ctx->marker.levelnow = sts.donor;

	*markers = &ctx->marker;
	*count = 1;

	return CUPS_BACKEND_OK;
}

static const char *kodak605_prefixes[] = {
	"kodak605",
	NULL,
};

/* Exported */
struct dyesub_backend kodak605_backend = {
	.name = "Kodak 605",
	.version = "0.31",
	.uri_prefixes = kodak605_prefixes,
	.cmdline_usage = kodak605_cmdline,
	.cmdline_arg = kodak605_cmdline_arg,
	.init = kodak605_init,
	.attach = kodak605_attach,
	.teardown = kodak605_teardown,
	.cleanup_job = kodak605_cleanup_job,
	.read_parse = kodak605_read_parse,
	.main_loop = kodak605_main_loop,
	.query_markers = kodak605_query_markers,
	.devices = {
		{ USB_VID_KODAK, USB_PID_KODAK_605, P_KODAK_605, "Kodak", "kodaka605"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* Kodak 605 data format

  Spool file consists of 14-byte header followed by plane-interleaved BGR data.
  Native printer resolution is 1844 pixels per row, and 1240 or 2434 rows.

  All fields are LITTLE ENDIAN unless otherwise specified

  Header:

  01 40 0a 00                    Fixed header
  XX                             Job ID
  CC CC                          Number of copies (1-???)
  WW WW                          Number of columns (Fixed at 1844)
  HH HH                          Number of rows (1240 or 2434)
  DD                             0x01 (4x6) 0x03 (8x6)
  LL                             Laminate, 0x01 (off) or 0x02 (on)
  00                             Print Mode (???)

  ************************************************************************

  Note:  Kodak 605 is actually a Shinko CHC-S1545-5A

  ************************************************************************

*/
