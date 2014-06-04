/*
 *   Kodak 6800/6850 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2014 Solomon Peachy <pizza@shaftnet.org>
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

#define USB_VID_KODAK       0x040A
#define USB_PID_KODAK_6800  0x4021
#define USB_PID_KODAK_6850  0x402B

/* File header */
struct kodak6800_hdr {
	uint8_t  hdr[9];   /* Always 03 1b 43 48 43 0a 00 04 00 [6850]
	                             03 1b 43 48 43 0a 00 01 00 [6800] */
	uint8_t  copies;
	uint16_t columns;  /* BE */
	uint16_t rows;     /* BE */
	uint8_t  media;    /* 0x06 for 6x8, 0x00 for 6x4, 0x07 for 5x7 */
	uint8_t  laminate; /* 0x01 to laminate, 0x00 for not */
	uint8_t  unk1;     /* 0x00 or 0x01 (for 4x6 on 6x8 media) */
} __attribute__((packed));

struct kodak68x0_status_readback {
	uint8_t  hdr[2];   /* Always 01 02 */
	uint8_t  sts;      /* 0x01 == ready, 0x02 == no media, 0x03 == not ready */
	uint8_t  null0[3];
	uint8_t  unkA;     /* 0x00 or 0x01 */
	uint8_t  nullA;
	uint32_t ctr0;     /* Total Prints (BE) */
	uint32_t ctr1;     /* Total Prints (BE) */
	uint32_t ctr2;     /* Increments by 1 for each print (6850), unk (6800). BE */
	uint32_t ctr3;     /* Increments by 2 for each print. BE */
	uint8_t  nullB[3];
	uint8_t  donor;    /* Percentage, 0-100 */
	uint8_t  unkC[2];  /* Always 00 03 */
	uint16_t main_fw;  /* seen 652 and 656 (6850) */
	uint8_t  unkD[2];  /* Always 00 01 */
	uint16_t dsp_fw;   /* Seen 540 and 541 (6850) and 131 (6800) */
	uint8_t  unk1;     /* Seen 0x00, 0x01, 0x03, 0x04 */
	uint8_t  null1[2];
	uint8_t  unk2;     /* Seen 0x01, 0x00 */
	uint8_t  null2;
	uint8_t  unk3;     /* Seen 0x01, 0x00 */
	uint8_t  null4;
	uint8_t  unk4;     /* Seen 0x01, 0x00 */
	uint8_t  null5[7];
} __attribute__((packed));

struct kodak6800_printsize {
	uint8_t  hdr;    /* Always 0x06 */
	uint16_t width;  /* BE */
	uint16_t height; /* BE */
	uint8_t  hdr2;   /* Always 0x01 */
	uint8_t  code;   /* 00, 01, 02, 03, 04, 05 seen. index? */
	uint8_t  code2;  /* 00, 01 seen.  ?Multicut? */
	uint8_t  null[2];
} __attribute__((packed));

#define MAX_MEDIA_LEN 128

struct kodak68x0_media_readback {
	uint8_t  hdr;      /* Always 0x01 */
	uint8_t  media;    /* Always 0x0b or 0x03 */
	uint8_t  null[5];
	uint8_t  count;    /* Always 0x04 (6800) or 0x06 (6850)? */
	struct kodak6800_printsize sizes[];
} __attribute__((packed));

#define CMDBUF_LEN 17

/* Private data stucture */
struct kodak6800_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;

	int type;
	struct kodak6800_hdr hdr;
	uint8_t *databuf;
	int datalen;
};

/* Program states */
enum {
	S_IDLE = 0,
	S_6850_READY,
	S_6850_READY_WAIT,
	S_READY,
	S_SENT_HDR,
	S_SENT_DATA,
	S_FINISHED,
};

static void kodak68x0_dump_mediainfo(struct kodak68x0_media_readback *media)
{
	int i;
	DEBUG("Media type %02x\n", media->media);
	DEBUG("Legal print sizes:\n");
	for (i = 0 ; i < media->count ; i++) {
		DEBUG("\t%d: %dx%d (%02x/%02x)\n", i, 
		      be16_to_cpu(media->sizes[i].width),
		      be16_to_cpu(media->sizes[i].height),
		      media->sizes[i].code,
		      media->sizes[i].code2);
	}
	DEBUG("\n");
}

static int kodak6800_get_mediainfo(struct kodak6800_ctx *ctx, struct kodak68x0_media_readback *media)
{
	uint8_t req[16];
	int ret, num;

	memset(req, 0, sizeof(req));
	memset(media, 0, sizeof(*media));

	req[0] = 0x03;
	req[1] = 0x1b;
	req[2] = 0x43;
	req[3] = 0x48;
	req[4] = 0x43;
	req[5] = 0x1a;

	/* Send request */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     req, sizeof(req))))
		return ret;

	/* Get response */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*)media, MAX_MEDIA_LEN, &num);

	if (ret < 0)
		return ret;
	if (num < (int)sizeof(*media)) {
		ERROR("Short read! (%d/%d)\n", num, (int) sizeof(*media));
		return 4;
	}

	return 0;
}

static void kodak68x0_dump_status(struct kodak68x0_status_readback *status)
{
	DEBUG("Total prints    : %d\n", be32_to_cpu(status->ctr0));
	DEBUG("Media prints    : %d\n", be32_to_cpu(status->ctr2));
	DEBUG("Remaining prints: %d\n", 375 - be32_to_cpu(status->ctr2));
	DEBUG("Main FW version : %d\n", be16_to_cpu(status->main_fw));
	DEBUG("DSP FW version  : %d\n", be16_to_cpu(status->dsp_fw));
	DEBUG("Donor           : %d%%\n", status->donor);
	DEBUG("\n");
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

	/* Send request */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     req, sizeof(req))))
		return ret;

	/* Get response */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*)status, sizeof(*status), &num);

	if (ret < 0)
		return ret;
	if (num < (int)sizeof(*status)) {
		ERROR("Short read! (%d/%d)\n", num, (int) sizeof(*status));
		return 4;
	}

	return 0;
}


#define UPDATE_SIZE 1536
static int kodak6800_get_tonecurve(struct kodak6800_ctx *ctx, char *fname)
{
	libusb_device_handle *dev = ctx->dev;
	uint8_t endp_down = ctx->endp_down;
	uint8_t endp_up = ctx->endp_up;

	uint8_t cmdbuf[16];
	uint8_t respbuf[64];
	int ret, num = 0;
	int i;

	uint16_t *data = malloc(UPDATE_SIZE);

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
	cmdbuf[11] = 0x01;
	cmdbuf[12] = 0x00;
	cmdbuf[13] = 0x00;
	cmdbuf[14] = 0x00;
	cmdbuf[15] = 0x00;

	if ((ret = send_data(dev, endp_down,
			     cmdbuf, 16)))
		goto done;
	
	ret = read_data(dev, endp_up,
			respbuf, sizeof(respbuf), &num);
	if (ret < 0)
		goto done;
	
	if (num != 51) {
		ERROR("Short read! (%d/%d)\n", num, 51);
		ret = 4;
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
		if ((ret = send_data(dev, endp_down,
				     cmdbuf, 11)))
			goto done;

		ret = read_data(dev, endp_up,
				respbuf, sizeof(respbuf), &num);
		if (ret < 0)
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
			write(tc_fd, &data[i], sizeof(uint16_t));
		}
		close(tc_fd);
	}

 done:
	/* We're done */
	free(data);

	return 0;
}

static int kodak6800_set_tonecurve(struct kodak6800_ctx *ctx, char *fname)
{
	libusb_device_handle *dev = ctx->dev;
	uint8_t endp_down = ctx->endp_down;
	uint8_t endp_up = ctx->endp_up;

	uint8_t cmdbuf[64];
	uint8_t respbuf[64];
	int ret, num = 0;
	int remain;

	uint16_t *data = malloc(UPDATE_SIZE);
	uint8_t *ptr;

	INFO("Set Tone Curve from '%s'\n", fname);

	/* Read in file */
	int tc_fd = open(fname, O_RDONLY);
	if (tc_fd < 0) {
		ret = -1;
		goto done;
	}
	if (read(tc_fd, data, UPDATE_SIZE) != UPDATE_SIZE) {
	        ret = -2;
		goto done;
	}
	close(tc_fd);

	/* Byteswap data to printer's format */
	for (ret = 0; ret < (UPDATE_SIZE)/2 ; ret++) {
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
	cmdbuf[11] = 0x01;
	cmdbuf[12] = 0x00;
	cmdbuf[13] = 0x00;
	cmdbuf[14] = 0x00;
	cmdbuf[15] = 0x00;

	if ((ret = send_data(dev, endp_down,
			     cmdbuf, 16)))
		goto done;
	
	ret = read_data(dev, endp_up,
			respbuf, sizeof(respbuf), &num);
	if (ret < 0)
		goto done;
	
	if (num != 51) {
		ERROR("Short read! (%d/%d)\n", num, 51);
		ret = 4;
		goto done;
	}

	ptr = (uint8_t*) data;
	remain = UPDATE_SIZE;
	while (remain > 0) {
		int count = remain > 63 ? 63 : remain;

		cmdbuf[0] = 0x03;
		memcpy(cmdbuf+1, ptr, count);

		remain -= count;
		ptr += count;

		/* Send next block over */
		if ((ret = send_data(dev, endp_down,
				     cmdbuf, count+1)))
			goto done;


		ret = read_data(dev, endp_up,
				respbuf, sizeof(respbuf), &num);
		if (ret < 0)
			goto done;
		
		if (num != 51) {
			ERROR("Short read! (%d/%d)\n", num, 51);
			ret = 4;
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
	req[5] = 0x03;

	/* Send request */
	if ((ret = send_data(dev, endp_down,
			     req, sizeof(req))))
		return ret;

	/* Get response */
	ret = read_data(dev, endp_up,
			resp, sizeof(resp) - 1, &num);

	if (ret < 0)
		return ret;
	if (num != 32) {
		ERROR("Short read! (%d/%d)\n", num, 32);
		return 4;
	}
	strncpy(buf, (char*)resp+24, buf_len);
	buf[buf_len-1] = 0;

	return 0;
}

static void kodak6800_cmdline(void)
{
	DEBUG("\t\t[ -c filename ]  # Get tone curve\n");
	DEBUG("\t\t[ -C filename ]  # Set tone curve\n");
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
}

static int kodak6800_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct kodak6800_ctx *ctx = vctx;
	int i, j = 0;

	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
	while ((i = getopt(argc, argv, "C:c:ms")) >= 0) {
		switch(i) {
		case 'c':
			if (ctx) {
				j = kodak6800_get_tonecurve(ctx, optarg);
				break;
			} 
			return 1;
		case 'C':
			if (ctx) {
				j = kodak6800_set_tonecurve(ctx, optarg);
				break;
			}
			return 1;
		case 'm':
			if (ctx) {
				uint8_t buf[MAX_MEDIA_LEN];
				struct kodak68x0_media_readback *media = (struct kodak68x0_media_readback *)buf;
				j = kodak6800_get_mediainfo(ctx, media);
				if (!j)
					kodak68x0_dump_mediainfo(media);
				break;
			}
			return 1;
		case 's':
			if (ctx) {
				struct kodak68x0_status_readback status;
				j = kodak6800_get_status(ctx, &status);
				if (!j)
					kodak68x0_dump_status(&status);

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


static void *kodak6800_init(void)
{
	struct kodak6800_ctx *ctx = malloc(sizeof(struct kodak6800_ctx));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(struct kodak6800_ctx));

	ctx->type = P_ANY;

	return ctx;
}

static void kodak6800_attach(void *vctx, struct libusb_device_handle *dev, 
			      uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct kodak6800_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;	
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);

	/* Map out device type */
	if (desc.idProduct == USB_PID_KODAK_6850)
		ctx->type = P_KODAK_6850;
	else
		ctx->type = P_KODAK_6800;
}


static void kodak6800_teardown(void *vctx) {
	struct kodak6800_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	free(ctx);
}

static int kodak6800_read_parse(void *vctx, int data_fd) {
	struct kodak6800_ctx *ctx = vctx;
	int ret;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	/* Read in then validate header */
	ret = read(data_fd, &ctx->hdr, sizeof(ctx->hdr));
	if (ret < 0 || ret != sizeof(ctx->hdr)) {
		if (ret == 0)
			return CUPS_BACKEND_CANCEL;
		ERROR("Read failed (%d/%d/%d)\n",
		      ret, 0, (int)sizeof(ctx->hdr));
		perror("ERROR: Read failed");
		return CUPS_BACKEND_CANCEL;
	}
	if (ctx->hdr.hdr[0] != 0x03 ||
	    ctx->hdr.hdr[1] != 0x1b ||
	    ctx->hdr.hdr[2] != 0x43 ||
	    ctx->hdr.hdr[3] != 0x48 ||
	    ctx->hdr.hdr[4] != 0x43) {
		ERROR("Unrecognized data format!\n");
		return CUPS_BACKEND_CANCEL;
	}

	ctx->datalen = be16_to_cpu(ctx->hdr.rows) * be16_to_cpu(ctx->hdr.columns) * 3;
	ctx->databuf = malloc(ctx->datalen);
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_FAILED;
	}

	{
		int remain = ctx->datalen;
		uint8_t *ptr = ctx->databuf;
		do {
			ret = read(data_fd, ptr, remain);
			if (ret < 0) {
				ERROR("Read failed (%d/%d/%d)\n", 
				      ret, remain, ctx->datalen);
				perror("ERROR: Read failed");
				return CUPS_BACKEND_CANCEL;
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}

	return CUPS_BACKEND_OK;
}

#define READBACK_LEN 68

static int kodak6800_main_loop(void *vctx, int copies) {
	struct kodak6800_ctx *ctx = vctx;

	uint8_t rdbuf[READBACK_LEN];
	uint8_t rdbuf2[READBACK_LEN];
	uint8_t cmdbuf[CMDBUF_LEN];

	uint8_t mediabuf[MAX_MEDIA_LEN];
	struct kodak68x0_media_readback *media = (struct kodak68x0_media_readback*)mediabuf;

	int last_state = -1, state = S_IDLE;
	int num, ret;
	int pending = 0;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Printer handles generating copies.. */
	if (ctx->hdr.copies < copies)
		ctx->hdr.copies = copies;
	copies = 1;

	/* Query loaded media */
	INFO("Querying loaded media\n");
	ret = kodak6800_get_mediainfo(ctx, media);
	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	/* Validate proper response */
	if (media->hdr != 0x01 ||
	    media->null[0] != 0x00) {
		ERROR("Unexpected response from media query!\n");
		return CUPS_BACKEND_STOP;
	}
	
	/* Appears to depend on media */
	if (media->media != 0x0b &&
	    media->media != 0x03) {
		ERROR("Unrecognized media type %02x\n", media->media);
		return CUPS_BACKEND_STOP;
	}

	/* Validate against supported media list */
	for (num = 0 ; num < media->count; num++) {
		if (media->sizes[num].height == ctx->hdr.rows &&
		    media->sizes[num].width == ctx->hdr.columns)
			break;
	}
	if (num == media->count) {
		ERROR("Print size unsupported by media!\n");
		return CUPS_BACKEND_HOLD;
	}

	INFO("Waiting for printer idle\n");
top:
	if (state != last_state) {
		if (dyesub_debug)
			DEBUG("last_state %d new %d\n", last_state, state);
	}

	if (pending)
		goto skip_query;

	/* Send Status Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x03;
	cmdbuf[1] = 0x1b;
	cmdbuf[2] = 0x43;
	cmdbuf[3] = 0x48;
	cmdbuf[4] = 0x43;
	cmdbuf[5] = 0x03;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, CMDBUF_LEN - 1)))
		return CUPS_BACKEND_FAILED;

skip_query:
	/* Read in the printer status */
	ret = read_data(ctx->dev, ctx->endp_up,
			rdbuf, READBACK_LEN, &num);
	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	if (num < 51) {
		ERROR("Short read! (%d/%d)\n", num, 51);
		return CUPS_BACKEND_FAILED;
	}

	if (num != 51) {
		ERROR("Unexpected readback from printer (%d/%d from 0x%02x))\n",
		      num, READBACK_LEN, ctx->endp_up);
		return CUPS_BACKEND_FAILED;
	}

	if (!pending) {
		if (rdbuf[0] != 0x01 ||
		    rdbuf[1] != 0x02) {
			ERROR("Unexpected response from status query!\n");
			return CUPS_BACKEND_FAILED;
		}
		if (rdbuf[2] == 0x02) {
			ERROR("Printer is out of media!\n");
			return CUPS_BACKEND_STOP;
		} else if (rdbuf[2] == 0x03) {
			ERROR("Printer is offline!\n");
			return CUPS_BACKEND_STOP;
		}
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
		if (ctx->type == P_KODAK_6850)
			state = S_6850_READY;
		else
			state = S_READY;
		break;
	case S_6850_READY:
		INFO("Sending 6850 init sequence\n");
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x03;
		cmdbuf[1] = 0x1b;
		cmdbuf[2] = 0x43;
		cmdbuf[3] = 0x48;
		cmdbuf[4] = 0x43;
		cmdbuf[5] = 0x4c;

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     cmdbuf, CMDBUF_LEN -1)))
			return CUPS_BACKEND_FAILED;
		pending = 1;
		state = S_6850_READY_WAIT;
		break;
	case S_6850_READY_WAIT: /* status response, with different header */
		if (rdbuf[0] != 0x01 ||
		    rdbuf[2] != 0x43) {
			ERROR("Unexpected response from printer init!\n");
			return CUPS_BACKEND_FAILED;
		}
		// XXX is this the media position, saying when we have a 4x6 left on an 8x6 blank?
		if (rdbuf[1] != 0x01 || rdbuf[1] != 0x00) {
			ERROR("Unexpected status code!\n");
			return CUPS_BACKEND_FAILED;
		}
		state = S_READY;
		break;
	case S_READY:
		/* Set up print job header */
		memcpy(cmdbuf, &ctx->hdr, CMDBUF_LEN);

		/* 6850 uses same spool format but different header gets sent */
		if (ctx->type == P_KODAK_6850) {
			if (ctx->hdr.media == 0x00)
				cmdbuf[7] = 0x04;
			else if (ctx->hdr.media == 0x06)
				cmdbuf[7] = 0x05; /* XXX audit this! */
		}

		/* If we're printing a 4x6 on 8x6 media... */
		if (ctx->hdr.media == 0x00 &&
		    be16_to_cpu(media->sizes[0].width) == 0x0982) {
			cmdbuf[14] = 0x06;
			cmdbuf[16] = 0x01;
		}

		INFO("Sending image header\n");
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     cmdbuf, CMDBUF_LEN)))
			return ret;
		pending = 1;
		state = S_SENT_HDR;
		break;
	case S_SENT_HDR:
		INFO("Sending image data\n");
		if ((ret = send_data(ctx->dev, ctx->endp_down, 
				     ctx->databuf, ctx->datalen)))
			return CUPS_BACKEND_FAILED;

		state = S_SENT_DATA;
		break;
	case S_SENT_DATA:
		INFO("Waiting for printer to acknowledge completion\n");
		if (rdbuf[0] != 0x01 ||
		    rdbuf[1] != 0x02 ||
		    rdbuf[2] != 0x01) {
			break;
		}

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

/* Exported */
struct dyesub_backend kodak6800_backend = {
	.name = "Kodak 6800/6850",
	.version = "0.38",
	.uri_prefix = "kodak6800",
	.cmdline_usage = kodak6800_cmdline,
	.cmdline_arg = kodak6800_cmdline_arg,
	.init = kodak6800_init,
	.attach = kodak6800_attach,
	.teardown = kodak6800_teardown,
	.read_parse = kodak6800_read_parse,
	.main_loop = kodak6800_main_loop,
	.query_serno = kodak6800_query_serno,
	.devices = { 
	{ USB_VID_KODAK, USB_PID_KODAK_6800, P_KODAK_6800, "Kodak"},
	{ USB_VID_KODAK, USB_PID_KODAK_6850, P_KODAK_6850, "Kodak"},
	{ 0, 0, 0, ""}
	}
};

/* Kodak 6800/6850 data format

  Spool file consists of 17-byte header followed by plane-interleaved BGR data.
  Native printer resolution is 1844 pixels per row, and 1240 or 2434 rows.

  6850 Adds support for 5x7, with 1548 pixels per row and 2140 columns.

  Header:

  03 1b 43 48 43 0a 00 01 00     Fixed header
  CC                             Number of copies (01-255)
  WW WW                          Number of columns, big endian. (Fixed at 1844 on 6800)
  HH HH                          Number of rows, big endian.
  DD                             0x00 (4x6) 0x06 (8x6) 0x07 (5x7 on 6850)
  LL                             Laminate, 0x00 (off) or 0x01 (on)
  00

  Note:  For 4x6 prints on 6x8 media, media (DD) is set to 0x06 and the final octet
         is set to 0x01, before sending this header to the printer.

         Additionally, the header sent to the 6850 uses 0x04 for 4x6, and 0x05 for 8x6. (?)

  ************************************************************************

   Kodak 6800 Printer Comms:

   [[file header]] 03 1b 43 48 43 0a 00 01  00 CC WW WW HH HH MT LL 00
    
   Note that the 'CC' paper code is modified for 6850, 0x04 for 4x6, 0x06 for 6x8.

->  03 1b 43 48 43 03 00 00  00 00 00 00 00 00 00 00  [status query]
<-  [51 octets]

    01 02 01 00 00 00 00 00  00 00 a2 7b 00 00 a2 7b
    00 00 02 f4 00 00 e6 b1  00 00 00 1a 00 03 00 e8
    00 01 00 83 00 00 00 00  00 00 00 00 00 00 00 00
    00 00 00

->  03 1b 43 48 43 1a 00 00  00 00 00 00 00 00 00 00  [media query]
<-  [58 octets]

    01 XX 00 00 00 00 00 04  06 WW WW MM MM 01 00 00  [MM MM == max printable size of media, 09 82 == 2434 for 6x8!]
    00 00 06 WW WW 09 ba 01  02 00 00 00 06 WW WW HH  [09 ba == 2940 == cut area?]
    HH 01 01 00 00 00 06 WW  WW MM MM 01 03 00 00 00  [XX == 0b or 03 == media type?]
    00 00 00 00 00 00 00 00  00 00


->  03 1b 43 48 43 0a 00 01  00 01 WW WW HH HH 06 01  [ image header, modified, see above ]
    01 

<-  [51 octets]

    01 02 01 00 00 00 00 00  00 00 a2 7b 00 00 a2 7b
    00 00 02 f4 00 00 e6 b1  00 00 00 1a 00 03 00 e8
    00 01 00 83 01 00 00 01  00 00 00 01 00 00 00 00 [ note the "01" after "83", and the extra two "01"s ]
    00 00 00

->  [4K of plane data]
->  ...
->  [4K of plane data]
->  [remainder of plane data + 17 bytes of 0xff]

->  03 1b 43 48 43 03 00 00  00 00 00 00 00 00 00 00 [status query]
<-  [51 octets]

    01 02 01 00 00 00 00 00  00 00 a2 7c 00 00 a2 7c [ note a2 7c vs a2 7b ]
    00 00 01 7a 00 00 e6 b3  00 00 00 1a 00 03 00 e8 [ note 01 7a vs 02 f4, e6 b3 vs e6 b1 ]
    00 01 00 83 01 00 00 00  00 01 00 01 00 00 00 00 [ note the moved '01' in the middle ]
    00 00 00

->  03 1b 43 48 43 03 00 00  00 00 00 00 00 00 00 00 [ status query ]
<-  [51 octets, repeats]

  Possible Serial number query:

->  03 1b 43 48 43 12 00 00  00 00 00 00 00 00 00 00
<-  [32 octets]

    00 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  [[ Pascal string? ]]
    20 20 20 20 20 20 20 20  36 30 34 33 4d 32 38 31  [[ ..."  6043M281" ]]

->  03 1b 43 48 43 0c 54 4f  4e 45 65 00 00 00 00 00
<-  [51 octets]

    [[ typical status response ]]
    [[ Followed by reset. ]]

  Read tone curve data:  

->  03 1b 43 48 43 0c 54 4f  4e 45 72 01 00 00 00 00
<-  [51 octets]

    [[ typical status response ]]

->  03 1b 43 48 43 0c 54 4f  4e 45 20
<-  [64 octets]

    81 01 07 07 27 07 72 07  c8 07 f8 07 22 07 48 08
    68 08 88 08 b3 08 db 08  f7 08 09 09 2e 09 49 09
    65 09 80 09 aa 09 ca 09  e2 09 fa 09 12 0a 32 0a
    42 0a 66 0a 81 0a 9a 0a  c3 0a d9 0a ee 0a 04 0b

->  03 1b 43 48 43 0c 54 4f  4e 45 20
<-  [64 octets]

  [[ repeats for total of 24 packets.  total of 1.5KiB. ]]

  Write tone curve data:

->  03 1b 43 48 43 0c 54 4f  4e 45 77 01 00 00 00 00
<-  [51 octets]

    [[ typical status response ]]

->  03 00 00 46 06 53 06 c0  06 07 07 37 07 5d 07 87 
    07 a1 07 c8 07 08 08 08  08 08 08 48 08 68 08 88
    08 a9 08 b9 08 d9 08 f9  08 12 09 2e 09 49 09 70
    09 89 08 99 09 ba 09 ca  08 da 09 0a 0a 24 0a 38
<-  [51 octets]

    [[ typical status response ]]

->  03 0a 53 0a 66 0a 81 0a ...
   ....
->  03 cf 38 0a 39 3d 39 79  39 96 39 b6 39 fb 39 01
    34 0a 34 08 3a 0c 1a 10  3a
<-  [51 octets]

    [[ typical status response ]]

  [[ total of 24 packets * 64, and then one final packet of 25: 1562 total. ]]
  [[ It apepars the extra 25 bytes are to compensate for the leading '03' on 
     each of the 25 URBs. ]]

   ***********************************************************************

   Kodak 6850 Printer Comms:

   [[file header]] 03 1b 43 48 43 0a 00 01  00 CC WW WW HH HH MT LL 00

   (See above for details on these fields)

->  03 1b 43 48 43 03 00 00  00 00 00 00 00 00 00 00  [status query]
<-  [51 octets]

    01 02 01 00 00 00 00 00  00 00 21 75 00 00 08 52
    00 00 01 29 00 00 3b 0a  00 00 00 0e 00 03 02 90
    00 01 02 1d 03 00 00 00  00 01 00 01 00 00 00 00
    00 00 00

->  03 1b 43 48 43 4c 00 00  00 00 00 00 00 00 00 00  [???]
<-  [51 octets]

    01 01 43 48 43 4c 00 00  00 00 00 00 00 00 00 00
    00 00 01 29 00 00 3b 0a  00 00 00 0e 00 03 02 90
    00 01 02 1d 03 00 00 00  00 01 00 01 00 00 00 00
    00 00 00

    01 00 43 48 43 4c 00 00  00 00 00 00 00 00 00 00
    00 00 00 01 00 00 b7 d3  00 00 00 5c 00 03 02 8c
    00 01 02 1c 00 00 00 00  00 01 00 01 00 00 00 00
    00 00 00

->  03 1b 43 48 43 03 00 00  00 00 00 00 00 00 00 00  [status query]
<-  [51 octets -- same as status query before ]

->  03 1b 43 48 43 1a 00 00  00 00 00 00 00 00 00 00  [media query]
<-  [68 octets]

    01 XX 00 00 00 00 00 06  06 WW WW MM MM 01 00 00  [MM MM == max printable size of media, 09 82 == 2434 for 6x8!]
    00 00 06 WW WW 09 ba 01  02 01 00 00 06 WW WW HH  [09 ba == 2940 == cut area?]
    HH 01 01 00 00 00 06 WW  WW MM MM 01 03 00 00 00  [XX == 0b or 03 == media type?]
    06 WW WW 09 ba 01 05 01  00 00 06 WW WW HH HH 01
    04 00 00 00

->  03 1b 43 48 43 0a 00 04  00 01 07 34 04 d8 06 01  [ image header, modified, see above ] 
    01

<-  [51 octets]

    01 02 01 00 00 00 00 00  00 00 21 75 00 00 08 52
    00 00 01 29 00 00 3b 0a  00 00 00 0e 00 03 02 90
    00 01 02 1d 04 00 00 01  00 00 00 01 00 00 00 00 [ note the "04" after "1d", and the moved '01' ]
    00 00 00

->  [4K of plane data]
->  ...
->  [4K of plane data]
->  [remainder of plane data]

->  03 1b 43 48 43 03 00 00  00 00 00 00 00 00 00 00 [status query]
<-  [51 octets]

    01 02 01 00 00 00 00 00  00 00 21 76 00 00 08 53 [ note 21 76, 08 53, 01 2a incremented by 1 ]
    00 00 01 2a 00 00 3b 0c  00 00 00 0e 00 03 02 90 [ note 3b 0c incremeted by 2 ]
    00 01 02 1d 04 00 00 01  00 00 00 01 00 00 00 00
    00 00 00

  Possible Serial number query:

->  03 1b 43 48 43 12 00 00  00 00 00 00 00 00 00 00  
    00 
<-  [32 octets]

    00 20 20 20 20 20 20 20  20 20 20 20 20 20 20 20  [[ Pascal string? ]]
    20 20 20 20 20 20 20 20  36 30 39 37 4b 53 34 39  [[ ..."  6097KS49" ]]

  Read tone curve data:  

->  03 1b 43 48 43 0c 54 4f  4e 45 72 01 00 00 00 00
<-  [51 octets]

    [[ typical status response ]]

->  03 1b 43 48 43 0c 54 4f  4e 45 20
<-  [64 octets]

    81 01 07 07 27 07 72 07  c8 07 f8 07 22 07 48 08
    68 08 88 08 b3 08 db 08  f7 08 09 09 2e 09 49 09
    65 09 80 09 aa 09 ca 09  e2 09 fa 09 12 0a 32 0a
    42 0a 66 0a 81 0a 9a 0a  c3 0a d9 0a ee 0a 04 0b

->  03 1b 43 48 43 0c 54 4f  4e 45 20
<-  [64 octets]

  [[ repeats for total of 24 packets.  total of 1.5KiB. ]]

->  03 1b 43 48 43 0c 54 4f  4e 45 65 00 00 00 00 00
<-  [51 octets]

    [[ typical status response ]]

  Maybe this resets the calibration table:

->  03 1b 43 48 43 05 00 00  00 00 00 00 00 00 00 00 [???]
<-  [34 octets]

    01 00 04 00 00 00 01 00  01 00 02 00 00 00 01 00
    01 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
    00 00 

  Write tone curve data:

->  03 1b 43 48 43 0c 54 4f  4e 45 77 01 00 00 00 00
<-  [51 octets]

    [[ typical status response ]]

->  03 00 00 46 06 53 06 c0  06 07 07 37 07 5d 07 87 
    07 a1 07 c8 07 08 08 08  08 08 08 48 08 68 08 88
    08 a9 08 b9 08 d9 08 f9  08 12 09 2e 09 49 09 70
    09 89 08 99 09 ba 09 ca  08 da 09 0a 0a 24 0a 38
<-  [51 octets]

    [[ typical status response ]]

->  03 0a 53 0a 66 0a 81 0a ...
   ....
->  03 cf 38 0a 39 3d 39 79  39 96 39 b6 39 fb 39 01
    34 0a 34 08 3a 0c 1a 10  3a
<-  [51 octets]

    [[ typical status response ]]

  [[ total of 24 packets * 64, and then one final packet of 25: 1562 total. ]]
  [[ It apepars the extra 25 bytes are to compensate for the leading '03' on 
     each of the 25 URBs. ]]

  Also seen on the 6850:

DEBUG: readback: 

01 02 03 00 00 00 01 00  00 01 5f 6f 00 01 5f 6f 
00 00 00 09 00 02 90 44  00 00 00 55 00 03 02 90 
00 01 02 1d 00 00 00 00  00 00 00 00 00 00 00 00 
00 00 00 

INIT/???
DEBUG: readback: 

01 02 03 00 00 00 00 00  00 01 5f 6f 00 01 5f 6f
00 00 00 09 00 02 90 44  00 00 00 55 00 03 02 90
00 01 02 1d 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00

??? 6x8c 

DEBUG: readback: 

01 02 01 00 00 00 00 00  00 01 5f 6f 00 01 5f 6f
00 00 00 09 00 02 90 44  00 00 00 55 00 03 02 90
00 01 02 1d 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 

Seen on the 6850 with no media loaded:

01 02 02 00 00 00 10 00  00 00 5d 1d 00 00 5d 1d 
00 00 00 00 00 00 b7 cc  00 00 00 00 00 03 02 8c
00 01 02 1c 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00

Seen on 6850 with 6R media (6x8) while offline:

01 02 03 00 00 00 03 00  00 00 5d 1f 00 00 5d 1f
00 00 00 01 00 00 b7 d3  00 00 00 5c 00 03 02 8c
00 01 02 1c 00 00 00 00  00 01 00 01 00 00 00 00
00 00 00

*/
