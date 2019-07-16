/*
 *   Kodak 6800/6850 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2019 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Development of this backend was sponsored by:
 *
 *     LiveLink Technology [ www.livelinktechnology.net ]
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

#define BACKEND kodak6800_backend

#include "backend_common.h"
#include "backend_sinfonia.h"

#define USB_VID_KODAK       0x040A
#define USB_PID_KODAK_6800  0x4021
#define USB_PID_KODAK_6850  0x402B

/* File header */
struct kodak6800_hdr {
	uint8_t  hdr[7];   /* Always 03 1b 43 48 43 0a 00 */
	uint8_t  jobid;    /* Non-zero */
	uint16_t copies;   /* BE, in BCD format (1-9999) */
	uint16_t columns;  /* BE */
	uint16_t rows;     /* BE */
	uint8_t  size;     /* media size; 0x06 for 6x8, 0x00 for 6x4, 0x07 for 5x7 */
	uint8_t  laminate; /* 0x01 to laminate, 0x00 for not */
	uint8_t  method;     /* 0x00 or 0x01 (for 4x6 on 6x8 media), 0x21 for 2x6, 0x23 for 3x6 */
} __attribute__((packed));

struct kodak68x0_status_readback {
	uint8_t  hdr;      /* Always 01 */
	uint8_t  status;   /* STATUS_* */
	uint8_t  status1;  /* STATUS1_* */
	uint32_t status2;  /* STATUS2_* */
	uint8_t  errcode;  /* Error ## */
	uint32_t lifetime; /* Lifetime Prints (BE) */
	uint32_t maint;    /* Maint Prints (BE) */
	uint32_t media;    /* Media Prints (6850), Unknown (6800) (BE) */
	uint32_t cutter;   /* Cutter Actuations (BE) */
	uint8_t  nullB[2];
	uint8_t  errtype;   /* seen 0x00 or 0xd0 */
	uint8_t  donor;     /* Percentage, 0-100 */
	uint16_t main_boot; /* Always 003 */
	uint16_t main_fw;   /* seen 6xx/8xx (6850) and 2xx/3xx/4xx (6800) */
	uint16_t dsp_boot;  /* Always 001 */
	uint16_t dsp_fw;    /* Seen 5xx (6850) and 1xx (6800) */
	uint8_t  b1_jobid;
	uint8_t  b2_jobid;
	uint16_t b1_remain;   /* Remaining prints in job */
	uint16_t b1_complete; /* Completed prints in job */
	uint16_t b1_total;    /* Total prints in job */
	uint16_t b2_remain;   /* Remaining prints in job */
	uint16_t b2_complete; /* Completed prints in job */
	uint16_t b2_total;    /* Total prints in job */
	uint8_t  curve_status; /* Always seems to be 0x00 */
} __attribute__((packed));

#define MAX_MEDIAS 16

struct kodak68x0_media_readback {
	uint8_t  hdr;      /* Always 0x01 */
	uint8_t  type;     /* Media code, KODAK68x0_MEDIA_xxx */
	uint8_t  null[5];
	uint8_t  count;    /* Always 0x04 (6800) or 0x06 (6850)? */
	struct sinfonia_mediainfo_item sizes[];
} __attribute__((packed));

#define CMDBUF_LEN 17

/* Private data structure */
struct kodak6800_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;

	int type;
	int supports_sub4x6;

	uint8_t jobid;

	struct sinfonia_mediainfo_item sizes[MAX_MEDIAS];
	uint8_t media_count;
	uint8_t media_type;

	struct kodak68x0_status_readback sts;

	struct marker marker;
};

/* Baseline commands */
static int kodak6800_do_cmd(struct kodak6800_ctx *ctx,
			    void *cmd, int cmd_len,
			    void *resp, int resp_len,
			    int *actual_len)
{
        int ret;

        /* Write command */
        if ((ret = send_data(ctx->dev, ctx->endp_down,
                             cmd, cmd_len)))
                return (ret < 0) ? ret : -99;

        /* Read response */
        ret = read_data(ctx->dev, ctx->endp_up,
                        resp, resp_len, actual_len);
        if (ret < 0)
                return ret;

        return 0;
}

static void kodak68x0_dump_mediainfo(struct sinfonia_mediainfo_item *sizes,
				     uint8_t media_count, uint8_t media_type)
{
	int i;

	if (media_type == KODAK6_MEDIA_NONE) {
		INFO("No Media Loaded\n");
		return;
	}
	kodak6_dumpmediacommon(media_type);

	INFO("Legal print sizes:\n");
	for (i = 0 ; i < media_count ; i++) {
		INFO("\t%d: %dx%d (%02x)\n", i,
		     sizes[i].columns,
		     sizes[i].rows,
		     sizes[i].method);
	}
	INFO("\n");
}

#define MAX_MEDIA_LEN (sizeof(struct kodak68x0_media_readback) + MAX_MEDIAS * sizeof(struct sinfonia_mediainfo_item))

static int kodak6800_get_mediainfo(struct kodak6800_ctx *ctx)
{
	struct kodak68x0_media_readback *media;
	uint8_t req[16];
	int ret, num, i, j;

	memset(req, 0, sizeof(req));
	media = malloc(MAX_MEDIA_LEN);
	if (!media) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	for (j = 0 ; j < 2 ; j ++) {
		memset(media, 0, sizeof(*media));

		req[0] = 0x03;
		req[1] = 0x1b;
		req[2] = 0x43;
		req[3] = 0x48;
		req[4] = 0x43;
		req[5] = 0x1a;
		req[6] = j;

		/* Issue command and get response */
		if ((ret = kodak6800_do_cmd(ctx, req, sizeof(req),
					    media, MAX_MEDIA_LEN,
					    &num))) {
			free(media);
			return ret;
		}

		/* Validate proper response */
		if (media->hdr != CMD_CODE_OK ||
		    media->null[0] != 0x00) {
			ERROR("Unexpected response from media query!\n");
			free(media);
			return CUPS_BACKEND_STOP;
		}
		ctx->media_type = media->type;

		for (i = 0; i < media->count ; i++) {
			memcpy(&ctx->sizes[ctx->media_count], &media->sizes[i], sizeof(struct sinfonia_mediainfo_item));
			ctx->sizes[ctx->media_count].rows = be16_to_cpu(ctx->sizes[ctx->media_count].rows);
			ctx->sizes[ctx->media_count].columns = be16_to_cpu(ctx->sizes[ctx->media_count].columns);
			ctx->media_count++;
		}
		if (i < 6)
			break;
	}

	free(media);
	return CUPS_BACKEND_OK;
}

static int kodak68x0_canceljob(struct kodak6800_ctx *ctx,
			       int id)
{
	uint8_t req[16];
	int ret, num;

	memset(req, 0, sizeof(req));

	req[0] = 0x03;
	req[1] = 0x1b;
	req[2] = 0x43;
	req[3] = 0x48;
	req[4] = 0x43;
	req[5] = 0x13;
	req[6] = id;

	/* Issue command and get response */
	if ((ret = kodak6800_do_cmd(ctx, req, sizeof(req),
				    &ctx->sts, sizeof(ctx->sts),
				    &num)))
		return ret;

	/* Validate proper response */
	if (ctx->sts.hdr != CMD_CODE_OK) {
		ERROR("Unexpected response from job cancel!\n");
		return -99;
	}

	return 0;
}

static int kodak68x0_reset(struct kodak6800_ctx *ctx)
{
	uint8_t req[16];
	int ret, num;

	memset(req, 0, sizeof(req));

	req[0] = 0x03;
	req[1] = 0x1b;
	req[2] = 0x43;
	req[3] = 0x48;
	req[4] = 0xc0;

	/* Issue command and get response */
	if ((ret = kodak6800_do_cmd(ctx, req, sizeof(req),
				    &ctx->sts, sizeof(ctx->sts),
				    &num)))
		return ret;

	/* Validate proper response */
	if (ctx->sts.hdr != CMD_CODE_OK) {
		ERROR("Unexpected response from job cancel!\n");
		return -99;
	}

	return 0;
}

static void kodak68x0_dump_status(struct kodak6800_ctx *ctx, struct kodak68x0_status_readback *status)
{
	char *detail;

	switch (status->status) {
        case STATUS_PRINTING:
                detail = "Printing";
                break;
        case STATUS_IDLE:
                detail = "Idle";
                break;
        default:
                detail = "Unknown";
                break;
        }
        INFO("Printer Status :  %s\n", detail);

        INFO("Printer State  : %s # %02x %08x %02x\n",
	     sinfonia_1x45_status_str(status->status1, status->status2, status->errcode),
             status->status1, status->status2, status->errcode);

	INFO("Bank 1 ID: %u\n", status->b1_jobid);
	INFO("\tPrints:  %d/%d complete\n",
	     be16_to_cpu(status->b1_complete), be16_to_cpu(status->b1_total));
	INFO("Bank 2 ID: %u\n", status->b2_jobid);
	INFO("\tPrints:  %d/%d complete\n",
	     be16_to_cpu(status->b2_complete), be16_to_cpu(status->b2_total));

	switch (status->curve_status) {
	case CURVE_TABLE_STATUS_INITIAL:
		detail = "Initial/Default";
		break;
	case CURVE_TABLE_STATUS_USERSET:
		detail = "User Stored";
		break;
	case CURVE_TABLE_STATUS_CURRENT:
		detail = "Current";
		break;
	default:
		detail = "Unknown";
		break;
	}
	INFO("Tone Curve Status: %s\n", detail);

	INFO("Counters:\n");
	INFO("\tLifetime      : %u\n", be32_to_cpu(status->lifetime));
	INFO("\tThermal Head  : %u\n", be32_to_cpu(status->maint));
	INFO("\tCutter        : %u\n", be32_to_cpu(status->cutter));

	if (ctx->type == P_KODAK_6850) {
		int max;

		INFO("\tMedia         : %u\n", be32_to_cpu(status->media));

		switch(ctx->media_type) {
		case KODAK6_MEDIA_6R:
		case KODAK6_MEDIA_6TR2:
			max = 375;
			break;
		default:
			max = 0;
			break;
		}

		if (max) {
			INFO("\t  Remaining   : %u\n", max - be32_to_cpu(status->media));
		} else {
			INFO("\t  Remaining   : Unknown\n");
		}
	}
	INFO("Main FW version : %d\n", be16_to_cpu(status->main_fw));
	INFO("DSP FW version  : %d\n", be16_to_cpu(status->dsp_fw));
	INFO("Donor           : %u%%\n", status->donor);
	INFO("\n");
}

static int kodak6800_get_status(struct kodak6800_ctx *ctx,
				struct kodak68x0_status_readback *status)
{
	uint8_t req[16];
	int ret, num;

	memset(req, 0, sizeof(req));
	memset(status, 0, sizeof(*status));

	req[0] = 0x03;
	req[1] = 0x1b;
	req[2] = 0x43;
	req[3] = 0x48;
	req[4] = 0x43;
	req[5] = 0x03;

	/* Issue command and get response */
	if ((ret = kodak6800_do_cmd(ctx, req, sizeof(req),
				    status, sizeof(*status),
				    &num)))
		return ret;

	/* Validate proper response */
	if (status->hdr != CMD_CODE_OK) {
		ERROR("Unexpected response from status query!\n");
		return -99;
	}

	/* Byteswap important stuff */
	status->status2 = be32_to_cpu(status->status2);

	return 0;
}

static int kodak6800_get_tonecurve(struct kodak6800_ctx *ctx, char *fname)
{
	uint8_t cmdbuf[16];
	uint8_t respbuf[64];
	int ret, num = 0;
	int i;

	uint16_t *data = malloc(TONE_CURVE_SIZE);
	if (!data) {
		ERROR("Memory Allocation Failure\n");
		return -1;
	}

	INFO("Dump Tone Curve to '%s'\n", fname);

	/* Initial Request */
	cmdbuf[0] = 0x03;
	cmdbuf[1] = 0x1b;
	cmdbuf[2] = 0x43;
	cmdbuf[3] = 0x48;
	cmdbuf[4] = 0x43;
	cmdbuf[5] = 0x0c;
	cmdbuf[6] = 0x54;
	cmdbuf[7] = 0x4f;
	cmdbuf[8] = 0x4e;
	cmdbuf[9] = 0x45;
	cmdbuf[10] = 0x72;
	cmdbuf[11] = 0x01; /* 01 for user tonecurve, can be 00 or 02 */
	cmdbuf[12] = 0x00; /* param table? */
	cmdbuf[13] = 0x00;
	cmdbuf[14] = 0x00;
	cmdbuf[15] = 0x00;

	respbuf[0] = 0xff;
	/* Issue command and get response */
	if ((ret = kodak6800_do_cmd(ctx, cmdbuf, sizeof(cmdbuf),
				    respbuf, sizeof(respbuf),
				    &num)))

	/* Validate proper response */
	if (respbuf[0] != CMD_CODE_OK) {
		ERROR("Unexpected response from tonecurve query!\n");
		ret = -99;
		goto done;
	}

	/* Then we can poll the data */
	cmdbuf[0] = 0x03;
	cmdbuf[1] = 0x1b;
	cmdbuf[2] = 0x43;
	cmdbuf[3] = 0x48;
	cmdbuf[4] = 0x43;
	cmdbuf[5] = 0x0c;
	cmdbuf[6] = 0x54;
	cmdbuf[7] = 0x4f;
	cmdbuf[8] = 0x4e;
	cmdbuf[9] = 0x45;
	cmdbuf[10] = 0x20;
	for (i = 0 ; i < 24 ; i++) {
		/* Issue command and get response */
		if ((ret = kodak6800_do_cmd(ctx, cmdbuf, sizeof(cmdbuf),
					    respbuf, sizeof(respbuf),
					    &num)))
			goto done;

		if (num != 64) {
			ERROR("Short read! (%d/%d)\n", num, 51);
			ret = 4;
			goto done;
		}

		/* Copy into buffer */
		memcpy(((uint8_t*)data)+i*64, respbuf, 64);
	}

	/* Open file and write it out */
	{
		int tc_fd = open(fname, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
		if (tc_fd < 0) {
			ret = 4;
			goto done;
		}

		for (i = 0 ; i < 768; i++) {
			/* Byteswap appropriately */
			data[i] = cpu_to_be16(le16_to_cpu(data[i]));
			ret = write(tc_fd, &data[i], sizeof(uint16_t));
		}
		close(tc_fd);
	}

 done:
	/* We're done */
	free(data);

	return ret;
}

static int kodak6800_set_tonecurve(struct kodak6800_ctx *ctx, char *fname)
{
	uint8_t cmdbuf[64];
	uint8_t respbuf[64];
	int ret, num = 0;
	int remain;

	uint16_t *data = malloc(TONE_CURVE_SIZE);
	uint8_t *ptr;

	if (!data) {
		ERROR("Memory Allocation Failure\n");
		return -1;
	}

	INFO("Set Tone Curve from '%s'\n", fname);

	/* Read in file */
	if ((ret = dyesub_read_file(fname, data, TONE_CURVE_SIZE, NULL))) {
		ERROR("Failed to read Tone Curve file\n");
		goto done;
	}

	/* Byteswap data to printer's format */
	for (ret = 0; ret < (TONE_CURVE_SIZE)/2 ; ret++) {
		data[ret] = cpu_to_le16(be16_to_cpu(data[ret]));
	}

	/* Initial Request */
	cmdbuf[0] = 0x03;
	cmdbuf[1] = 0x1b;
	cmdbuf[2] = 0x43;
	cmdbuf[3] = 0x48;
	cmdbuf[4] = 0x43;
	cmdbuf[5] = 0x0c;
	cmdbuf[6] = 0x54;
	cmdbuf[7] = 0x4f;
	cmdbuf[8] = 0x4e;
	cmdbuf[9] = 0x45;
	cmdbuf[10] = 0x77;
	cmdbuf[11] = 0x01; /* User TC.  Can be 00 or 02 */
	cmdbuf[12] = 0x00; /* param table? */
	cmdbuf[13] = 0x00;
	cmdbuf[14] = 0x00;
	cmdbuf[15] = 0x00;

	/* Issue command and get response */
	if ((ret = kodak6800_do_cmd(ctx, cmdbuf, sizeof(cmdbuf),
				    respbuf, sizeof(respbuf),
				    &num)))

	/* Validate proper response */
	if (num != 51) {
		ERROR("Short read! (%d/%d)\n", num, 51);
		ret = 4;
		goto done;
	}

	if (respbuf[0] != CMD_CODE_OK) {
		ERROR("Unexpected response from tonecurve set!\n");
		ret = -99;
		goto done;
	}

	ptr = (uint8_t*) data;
	remain = TONE_CURVE_SIZE;
	while (remain > 0) {
		int count = remain > 63 ? 63 : remain;

		cmdbuf[0] = 0x03;
		memcpy(cmdbuf+1, ptr, count);

		remain -= count;
		ptr += count;

		/* Issue command and get response */
		if ((ret = kodak6800_do_cmd(ctx, cmdbuf, count + 1,
					    respbuf, sizeof(respbuf),
					    &num)))

		if (num != 51) {
			ERROR("Short read! (%d/%d)\n", num, 51);
			ret = 4;
			goto done;
		}
		if (respbuf[0] != CMD_CODE_OK) {
			ERROR("Unexpected response from tonecurve set!\n");
			ret = -99;
			goto done;
		}
	};

done:
	/* We're done */
	free(data);
	return ret;
}

static int kodak6800_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct kodak6800_ctx ctx = {
		.dev = dev,
		.endp_up = endp_up,
		.endp_down = endp_down,
	};

	int ret;
	int num;

	uint8_t resp[33];
	uint8_t req[16];

	memset(req, 0, sizeof(req));
	memset(resp, 0, sizeof(resp));

	req[0] = 0x03;
	req[1] = 0x1b;
	req[2] = 0x43;
	req[3] = 0x48;
	req[4] = 0x43;
	req[5] = 0x12;

	/* Issue command and get response */
	if ((ret = kodak6800_do_cmd(&ctx, req, sizeof(req),
				    resp, sizeof(resp),
				    &num)))
		return ret;

	if (num != 32) {
		ERROR("Short read! (%d/%d)\n", num, 32);
		return -2;
	}

	strncpy(buf, (char*)resp+24, buf_len);
	buf[buf_len-1] = 0;

	return 0;
}

static int kodak6850_send_unk(struct kodak6800_ctx *ctx)
{
	uint8_t cmdbuf[16];
	uint8_t rdbuf[64];
	int ret = 0, num = 0;

	memset(cmdbuf, 0, sizeof(cmdbuf));
	cmdbuf[0] = 0x03;
	cmdbuf[1] = 0x1b;
	cmdbuf[2] = 0x43;
	cmdbuf[3] = 0x48;
	cmdbuf[4] = 0x43;
	cmdbuf[5] = 0x4c;

	/* Issue command and get response */
	if ((ret = kodak6800_do_cmd(ctx, cmdbuf, sizeof(cmdbuf),
				    rdbuf, sizeof(rdbuf),
				    &num)))
		return -1;

	if (num != 51) {
		ERROR("Short read! (%d/%d)\n", num, 51);
		return CUPS_BACKEND_FAILED;
	}

	if (rdbuf[0] != CMD_CODE_OK ||
	    rdbuf[2] != 0x43) {
		ERROR("Unexpected response from printer init!\n");
		return CUPS_BACKEND_FAILED;
	}

#if 0
	// XXX No particular idea what this actually is
	if (rdbuf[1] != 0x01 && rdbuf[1] != 0x00) {
		ERROR("Unexpected status code (0x%02x)!\n", rdbuf[1]);
		return CUPS_BACKEND_FAILED;
	}
#endif
	return ret;
}

static void kodak6800_cmdline(void)
{
	DEBUG("\t\t[ -c filename ]  # Get tone curve\n");
	DEBUG("\t\t[ -C filename ]  # Set tone curve\n");
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -R ]           # Reset printer\n");
	DEBUG("\t\t[ -X jobid ]     # Cancel Job\n");
}

static int kodak6800_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct kodak6800_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "C:c:mRsX:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'c':
			j = kodak6800_get_tonecurve(ctx, optarg);
			break;
		case 'C':
			j = kodak6800_set_tonecurve(ctx, optarg);
			break;
		case 'm':
			kodak68x0_dump_mediainfo(ctx->sizes, ctx->media_count, ctx->media_type);
			break;
		case 'R':
			kodak68x0_reset(ctx);
			break;
		case 's': {
			j = kodak6800_get_status(ctx, &ctx->sts);
			if (!j)
				kodak68x0_dump_status(ctx, &ctx->sts);
			break;
		}
		case 'X':
			j = kodak68x0_canceljob(ctx, atoi(optarg));
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
}

static void *kodak6800_init(void)
{
	struct kodak6800_ctx *ctx = malloc(sizeof(struct kodak6800_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct kodak6800_ctx));

	return ctx;
}

static int kodak6800_attach(void *vctx, struct libusb_device_handle *dev, int type,
			    uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct kodak6800_ctx *ctx = vctx;

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;
	ctx->type = type;

        /* Ensure jobid is sane */
        ctx->jobid = jobid & 0x7f;
	if (!ctx->jobid)
		ctx->jobid++;

	if (test_mode < TEST_MODE_NOATTACH) {
		/* Query printer status */
		if (kodak6800_get_status(ctx, &ctx->sts)) {
			ERROR("Can't query status\n");
			return CUPS_BACKEND_FAILED;
		}
		uint16_t fw = be16_to_cpu(ctx->sts.main_fw);
		if (ctx->type == P_KODAK_6850) {
			if ((fw >= 878) ||
			    (fw < 800 && fw >= 678)) {
				ctx->supports_sub4x6 = 1;
			} else {
				WARNING("Printer FW out of date, recommend updating for current media and features\n");
			}
		} else {
			if ((fw >= 459) ||
			    (fw < 400 && fw >= 359) ||
			    (fw < 300 && fw >= 259)) {
				ctx->supports_sub4x6 = 1;
			} else {
				WARNING("Printer FW out of date, recommend updating for current media and features\n");
			}
		}

		/* Query media info */
		if (kodak6800_get_mediainfo(ctx)) {
			ERROR("Can't query media\n");
			return CUPS_BACKEND_FAILED;
		}
	} else {
		int media_code = KODAK6_MEDIA_6TR2;
		if (getenv("MEDIA_CODE"))
			media_code = atoi(getenv("MEDIA_CODE"));

		ctx->media_type = media_code;
		ctx->supports_sub4x6 = 1;
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = kodak6_mediatypes(ctx->media_type);
	ctx->marker.levelmax = 100; /* Ie percentage */
	ctx->marker.levelnow = -2;

	return CUPS_BACKEND_OK;
}

static int kodak6800_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct kodak6800_ctx *ctx = vctx;
	int ret;

	struct kodak6800_hdr hdr;
	struct sinfonia_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));

	/* Read in then validate header */
	ret = read(data_fd, &hdr, sizeof(hdr));
	if (ret < 0 || ret != sizeof(hdr)) {
		if (ret == 0) {
			sinfonia_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		ERROR("Read failed (%d/%d/%d)\n",
		      ret, 0, (int)sizeof(hdr));
		perror("ERROR: Read failed");
		sinfonia_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}
	if (hdr.hdr[0] != 0x03 ||
	    hdr.hdr[1] != 0x1b ||
	    hdr.hdr[2] != 0x43 ||
	    hdr.hdr[3] != 0x48 ||
	    hdr.hdr[4] != 0x43) {
		ERROR("Unrecognized data format!\n");
		sinfonia_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	uint16_t rows = be16_to_cpu(hdr.rows);
	uint16_t cols = be16_to_cpu(hdr.columns);
	if (rows != 1240 && rows != 2434 && rows != 2140 && !ctx->supports_sub4x6) {
		ERROR("Printer Firmware does not support non-4x6/8x6/5x7 prints, please upgrade!\n");
		sinfonia_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	job->datalen = rows * cols * 3;
	job->databuf = malloc(job->datalen);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		sinfonia_cleanup_job(job);
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	/* Windows driver only sends 634 rows of data, work around */
	if (rows == 636 && hdr.size == 6 && hdr.method == 0) {
		rows = 634;
		job->datalen -= 1844*2*3;
	}

	/* Read in the spool data */
	{
		int remain = job->datalen;
		uint8_t *ptr = job->databuf;
		do {
			ret = read(data_fd, ptr, remain);
			if (ret < 0) {
				ERROR("Read failed (%d/%d/%d)\n",
				      ret, remain, job->datalen);
				perror("ERROR: Read failed");
				sinfonia_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}

	/* Undo the Windows workaround... */
	if (rows == 634) {
		rows = 636;
		job->datalen += 1844*2*3;
	}

	/* Perform some header re-jiggery */
	if (hdr.size == 0) {
		if (cols == 1844)
			hdr.size = 6;
		else if (cols == 1548)
			hdr.size = 7;
	}
	if (hdr.method == 0) {
		if (rows == 636) {
			hdr.method = 0x21;
		} else if (rows == 936) {
			hdr.method = 0x23;
		} else if (rows == 1240) {
			hdr.method = 0x01;
		} else if (rows == 1282) {
			hdr.method = 0x20;
		} else if (rows == 1882) {
			hdr.method = 0x22;
		} else if (rows == 2490) {
			hdr.method = 0x2;
		}
	}

	hdr.copies = be16_to_cpu(hdr.copies);
	hdr.copies = packed_bcd_to_uint32((char*)&hdr.copies, 2);
	if (hdr.copies > 1)
		copies = hdr.copies;

	/* Fill out job structure */
	job->jp.copies = copies;
	job->jp.rows = rows;
	job->jp.columns = cols;
	job->jp.media = hdr.size;
	job->jp.oc_mode = hdr.laminate;
	job->jp.method = hdr.method;

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int kodak6800_main_loop(void *vctx, const void *vjob) {
	struct kodak6800_ctx *ctx = vctx;

	int num, ret;
	int copies;

	const struct sinfonia_printjob *job = vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	copies = job->jp.copies;

	/* Validate against supported media list */
	for (num = 0 ; num < ctx->media_count; num++) {
		if (ctx->sizes[num].rows == job->jp.rows &&
		    ctx->sizes[num].columns == job->jp.columns &&
		    ctx->sizes[num].method == job->jp.method)
			break;
	}
	if (num == ctx->media_count) {
		ERROR("Print size unsupported by media!\n");
		return CUPS_BACKEND_HOLD;
	}

	INFO("Waiting for printer idle\n");

	while(1) {
		if (kodak6800_get_status(ctx, &ctx->sts))
			return CUPS_BACKEND_FAILED;

		if (ctx->marker.levelnow != ctx->sts.donor) {
			ctx->marker.levelnow = ctx->sts.donor;
			dump_markers(&ctx->marker, 1, 0);
		}

		if (ctx->sts.status1 == STATE_STATUS1_ERROR) {
			INFO("Printer State: %s # %02x %08x %02x\n",
			     sinfonia_1x45_status_str(ctx->sts.status1, ctx->sts.status2, ctx->sts.errcode),
			     ctx->sts.status1, ctx->sts.status2, ctx->sts.errcode);
			return CUPS_BACKEND_FAILED;
		}

		/* make sure we're not colliding with an existing
		   jobid */
		while (ctx->jobid == ctx->sts.b1_jobid ||
		       ctx->jobid == ctx->sts.b2_jobid) {
			ctx->jobid++;
			ctx->jobid &= 0x7f;
			if (!ctx->jobid)
				ctx->jobid++;
		}

		/* See if we have an open bank */
                if (!ctx->sts.b1_remain ||
                    !ctx->sts.b2_remain)
                        break;

		sleep(1);
	}

	/* This command is unknown, sort of a secondary status query */
	if (ctx->type == P_KODAK_6850) {
		ret = kodak6850_send_unk(ctx);
		if (ret)
			return ret;
	}

        /* Fix max print count. */
        if (copies > 9999)
                copies = 9999;

	/* Fill out printjob header */
	struct kodak6800_hdr hdr;
	hdr.hdr[0] = 0x03;
	hdr.hdr[1] = 0x1b;
	hdr.hdr[2] = 0x43;
	hdr.hdr[3] = 0x48;
	hdr.hdr[4] = 0x43;
	hdr.hdr[5] = 0x0a;
	hdr.hdr[6] = 0x00;
	hdr.jobid = ctx->jobid;
	hdr.copies = uint16_to_packed_bcd(copies);
	hdr.columns = cpu_to_be16(job->jp.columns);
	hdr.rows = cpu_to_be16(job->jp.rows);
	hdr.size = job->jp.media;
	hdr.laminate = job->jp.oc_mode;
	hdr.method = job->jp.method;

	INFO("Sending Print Job (internal id %u)\n", ctx->jobid);
	if ((ret = kodak6800_do_cmd(ctx, (uint8_t*) &hdr, sizeof(hdr),
				    &ctx->sts, sizeof(ctx->sts),
				    &num)))
		return ret;

	if (ctx->sts.hdr != CMD_CODE_OK) {
		ERROR("Unexpected response from print command!\n");
		return CUPS_BACKEND_FAILED;
	}

//	sleep(1); // Appears to be necessary for reliability
	INFO("Sending image data\n");
	if ((send_data(ctx->dev, ctx->endp_down,
			     job->databuf, job->datalen)) != 0)
		return CUPS_BACKEND_FAILED;

	INFO("Waiting for printer to acknowledge completion\n");
	do {
		sleep(1);
		if (kodak6800_get_status(ctx, &ctx->sts))
			return CUPS_BACKEND_FAILED;

		if (ctx->marker.levelnow != ctx->sts.donor) {
			ctx->marker.levelnow = ctx->sts.donor;
			dump_markers(&ctx->marker, 1, 0);
		}

		if (ctx->sts.status1 == STATE_STATUS1_ERROR) {
			INFO("Printer State: %s # %02x %08x %02x\n",
			     sinfonia_1x45_status_str(ctx->sts.status1, ctx->sts.status2, ctx->sts.errcode),
			     ctx->sts.status1, ctx->sts.status2, ctx->sts.errcode);
			return CUPS_BACKEND_FAILED;
		}

		/* If all prints are complete, we're done! */
		if (ctx->sts.b1_jobid == hdr.jobid && ctx->sts.b1_complete == ctx->sts.b1_total)
			break;
		if (ctx->sts.b2_jobid == hdr.jobid && ctx->sts.b2_complete == ctx->sts.b2_total)
			break;

		if (fast_return) {
			INFO("Fast return mode enabled.\n");
			break;
		}

	} while (1);

	INFO("Print complete\n");

	return CUPS_BACKEND_OK;
}

static int kodak6800_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct kodak6800_ctx *ctx = vctx;

	/* Query printer status */
	if (kodak6800_get_status(ctx, &ctx->sts))
		return CUPS_BACKEND_FAILED;

	ctx->marker.levelnow = ctx->sts.donor;

	*markers = &ctx->marker;
	*count = 1;

	return CUPS_BACKEND_OK;
}

static const char *kodak6800_prefixes[] = {
	"kodak68x0", // Family driver, do not nuke.
	"kodak-6800", "kodak-6850",
	// Backwards-compatibility
	"kodak6800", "kodak6850",
	NULL
};

/* Exported */
struct dyesub_backend kodak6800_backend = {
	.name = "Kodak 6800/6850",
	.version = "0.73" " (lib " LIBSINFONIA_VER ")",
	.uri_prefixes = kodak6800_prefixes,
	.cmdline_usage = kodak6800_cmdline,
	.cmdline_arg = kodak6800_cmdline_arg,
	.init = kodak6800_init,
	.attach = kodak6800_attach,
	.cleanup_job = sinfonia_cleanup_job,
	.read_parse = kodak6800_read_parse,
	.main_loop = kodak6800_main_loop,
	.query_serno = kodak6800_query_serno,
	.query_markers = kodak6800_query_markers,
	.devices = {
		{ USB_VID_KODAK, USB_PID_KODAK_6800, P_KODAK_6800, "Kodak", "kodak-6800"},
		{ USB_VID_KODAK, USB_PID_KODAK_6850, P_KODAK_6850, "Kodak", "kodak-6850"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* Kodak 6800/6850 data format

  Spool file consists of 17-byte header followed by plane-interleaved BGR data.
  Native printer resolution is 1844 pixels per row, and 1240 or 2434 rows.

  6850 Adds support for 5x7, with 1548 pixels per row and 2140 columns.

  All fields are BIG ENDIAN unless otherwise specified.

  Header:

  03 1b 43 48 43 0a 00           Fixed header
  II                             Job ID (1-255)
  NN NN                          Number of copies in BCD form (0001->9999)
  WW WW                          Number of columns (Fixed at 1844 on 6800)
  HH HH                          Number of rows.
  SS                             Print size -- 0x00 (4x6) 0x06 (8x6) 0x07 (5x7 on 6850)
  LL                             Laminate mode -- 0x00 (off) or 0x01 (on)
  UU                             Print mode -- 0x00 (normal) or 0x01 (4x6 on 8x6) 0x21 (2x6) 0x23 (3x6)

  ************************************************************************

  Note:  6800 is Shinko CHC-S1145-5A, 6850 is Shinko CHC-S1145-5B

  Both are very similar to Shinko S1245!

  ************************************************************************

  This command is unique to the 6850:

->  03 1b 43 48 43 4c 00 00  00 00 00 00 00 00 00 00  [???]
<-  [51 octets]

    01 01 43 48 43 4c 00 00  00 00 00 00 00 00 00 00 <-- Everything after this
    00 00 01 29 00 00 3b 0a  00 00 00 0e 00 03 02 90     line is the same as
    00 01 02 1d 03 00 00 00  00 01 00 01 00 00 00 00     the "status" resp.
    00 00 00

    01 00 43 48 43 4c 00 00  00 00 00 00 00 00 00 00
    00 00 00 01 00 00 b7 d3  00 00 00 5c 00 03 02 8c
    00 01 02 1c 00 00 00 00  00 01 00 01 00 00 00 00
    00 00 00

  An additional command that's also unknown

->  03 1b 43 48 43 4d 01 00  00 00 00 00 00 00 00 00
<-  01 02 01 00 00 00 00 00  00 00 5d ca 00 00 5d ca
    00 00 00 15 00 00 b8 f8  00 00 00 40 00 03 02 a6
    00 01 02 31 1e 00 00 00  00 01 00 01 00 00 00 00
    00 00 00

  One more note for the 6850.  These sizes have been seen:

     1844x2434, method 0x03
     1844x2490, method 0x05
     1844x2222, method 0x00

*/
