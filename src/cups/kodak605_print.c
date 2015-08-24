/*
 *   Kodak 605 Photo Printer CUPS backend -- libusb-1.0 version
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

#define BACKEND kodak605_backend

#include "backend_common.h"

#define USB_VID_KODAK       0x040A
#define USB_PID_KODAK_605   0x402E

/* Media structure */
struct kodak605_medium {
	uint8_t  index;
	uint16_t cols;   /* LE */
	uint16_t rows;   /* LE */
	uint8_t  unk[5]; /* 01 00 00 00 00 */
}  __attribute__((packed));

struct kodak605_media_list {
	uint8_t  unk[12]; /* 01 00 00 00 00 00 02 00 67 00 02 0b */
	uint8_t  count;
	struct kodak605_medium entries[];
} __attribute__((packed));

#define MAX_MEDIA_LEN 128

/* Status response */
struct kodak605_status {
	uint8_t  unk_1[6];  /* 01 00 00 00 00 00 */
	uint8_t  sts_1;     /* 01 or 02 */
	uint8_t  sts_2;     /* 00 or 62 */
	uint8_t  unk_2[2];  /* 42 00 */
	uint32_t ctr_1;     /* 30 00 00 00 -> incr by 1 ?? lifetime? */
	uint32_t ctr_2;     /* 30 00 00 00 -> incr by 1 ?? since maint? */
	uint32_t ctr_3;     /* 13 00 00 00 -> incr by 1 ?? media count? */
	uint32_t ctr_4;     /* 75 00 00 00 -> incr by 2 ?? cutter? */
	uint32_t ctr_5;     /* 30 00 00 00 -> incr by 1 ?? lifetime? */
	uint8_t  unk_3;     /* 5d -> 5c -> 5b -- Donor % ?? */
/*@31*/	uint8_t  null_1[7]; /* 00 00 00 00 00 00 00 */
/*@38*/	uint8_t  sts_3;     /* 00 or 01 or 02 */
	uint8_t  sts_4;     /* 00 or 01 */
	uint8_t  unk_4;     /* 00 */
	uint8_t  sts_5;     /* 00 or 01 */
	uint8_t  unk_5;     /* 00 */
	uint8_t  sts_6;     /* 00 or 01 */
	uint8_t  unk_6;     /* 00 */
/*@45*/	uint8_t  sts_7;     /* 00 or 02 or 12 */  // XXX theory; 00 is idle, 02 is buffer "full", 12 is "printing"
	uint8_t  sts_8;     /* 00 or 01 or 02 */
	uint8_t  sts_9;     /* 00 or 01 */
	uint8_t  unk_7[3];  /* 00 00 00 */
	uint8_t  sts_10;    /* 00 or 01 */
	uint8_t  unk_8;     /* 00 */
/*@53*/	uint8_t  sts_11;    /* 00 or 02 or 12, always matches @45 */
/*@54*/	uint8_t  sts_12;    /* 00 or 01 or 02 */
/*@55*/ uint8_t  sts_13;    /* 00 or 01 */
/*@56*/	uint8_t  unk_9;     /* 00 */
/*@57*/	uint8_t  sts_14;    /* 00 or 01 */
/*@58*/	uint8_t  unk_10;    /* 00 */
/*@59*/	uint8_t  sts_15;    /* 00 or 01 */
/*@60*/	uint8_t  null_2[10];/* 00 00 00 00 00 00 00 00 00 00*/
	uint8_t  unk_12[6]; /* 01 00 00 00 00 00 */
} __attribute__((packed));

/* File header */
struct kodak605_hdr {
	uint8_t  hdr[4];   /* 01 40 0a 00 */
	uint8_t  unk1;     /* 01 (file) or 02 (sometimes sent to printer) */
	uint8_t  copies;   /* 01 or more */
	uint8_t  unk2;     /* always 00 */
	uint16_t columns;  /* BE always 0x0734 */
	uint16_t rows;     /* BE */
	uint8_t  media;    /* 0x03 for 6x8, 0x01 for 6x4 */
	uint8_t  laminate; /* 0x02 to laminate, 0x01 for not */
	uint8_t  unk3;     /* 0x00, 0x01 [may be print mode] */
} __attribute__((packed));

#define CMDBUF_LEN 4

/* Private data stucture */
struct kodak605_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	struct kodak605_hdr hdr;

	struct kodak605_media_list *media;

	uint8_t *databuf;
	int datalen;
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
		return 4;
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

	ctx->type = P_ANY;

	return ctx;
}

static void kodak605_attach(void *vctx, struct libusb_device_handle *dev,
			      uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct kodak605_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;	
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);
	
	ctx->type = lookup_printer_type(&kodak605_backend,
					desc.idVendor, desc.idProduct);	

	/* Query media info */
	if (kodak605_get_media(ctx, ctx->media)) {
		ERROR("Can't query media\n");
	}
}

static void kodak605_teardown(void *vctx) {
	struct kodak605_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	free(ctx);
}

static int kodak605_read_parse(void *vctx, int data_fd) {
	struct kodak605_ctx *ctx = vctx;
	int ret;

	if (!ctx)
		return CUPS_BACKEND_CANCEL;

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

	if (ctx->hdr.hdr[0] != 0x01 ||
	    ctx->hdr.hdr[1] != 0x40 ||
	    ctx->hdr.hdr[2] != 0x0a ||
	    ctx->hdr.hdr[3] != 0x00) {
		ERROR("Unrecognized data format!\n");
		return CUPS_BACKEND_CANCEL;
	}

	ctx->datalen = le16_to_cpu(ctx->hdr.rows) * le16_to_cpu(ctx->hdr.columns) * 3;
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

	if (sts->unk_1[0] != 0x01) {
		ERROR("Unexpected response from status query!\n");
		return CUPS_BACKEND_FAILED;
	}
	
	return 0;
}

static int kodak605_main_loop(void *vctx, int copies) {
	struct kodak605_ctx *ctx = vctx;

	struct kodak605_status sts;

	int num, ret;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Printer handles generating copies.. */
	if (copies > 255)
		copies = 255;
	if (ctx->hdr.copies < copies)
		ctx->hdr.copies = copies;

	/* Validate against supported media list */
	for (num = 0 ; num < ctx->media->count; num++) {
		if (ctx->media->entries[num].rows == ctx->hdr.rows &&
		    ctx->media->entries[num].cols == ctx->hdr.columns)
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

		// XXX  check status.. check for idle?
		sleep(1);
		break;
	}

	if (ctx->hdr.media == 0x03)
		ctx->hdr.unk1 = 0x02;  /* XXX Seen in many sniffs... */

	{
		INFO("Sending image header\n");
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*)&ctx->hdr, sizeof(ctx->hdr))))
			return CUPS_BACKEND_FAILED;

		uint8_t rdbuf[10];
		if ((ret = read_data(ctx->dev, ctx->endp_up,
				     rdbuf, sizeof(rdbuf), &num)))
			return CUPS_BACKEND_FAILED;
		if (rdbuf[0] != 0x01 ||
		    rdbuf[6] != 0x02) { // XXX is this always the case?
			ERROR("Printer not ready..\n");
			return CUPS_BACKEND_FAILED;
		}
	}
	sleep(1);
	
	INFO("Sending image data\n");
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     ctx->databuf, ctx->datalen)))
		return CUPS_BACKEND_FAILED;

	INFO("Image data sent\n");
	sleep(1);

	INFO("Waiting for printer to acknowledge completion\n");
	while(1) {
		if ((ret = kodak605_get_status(ctx, &sts)))
			return CUPS_BACKEND_FAILED;

		// XXX  check status.. wait for idle?
		// don't forget fast_return!
		sleep(1);
		break;
	}
	
	INFO("Print complete\n");

	return CUPS_BACKEND_OK;
}

static void kodak605_dump_status(struct kodak605_status *sts)
{
	uint8_t *rdbuf = (uint8_t *) sts;
	int i;
	
	DEBUG("raw status: ");
	for (i = 0 ; i < (int)sizeof(*sts) ; i++) {
		DEBUG2("%02x ", rdbuf[i]);
	}
}

static void kodak605_dump_mediainfo(struct kodak605_media_list *media)
{
	int i;

	DEBUG("Legal print sizes:\n");
	for (i = 0 ; i < media->count ; i++) {
		DEBUG("\t%d: %dx%d\n", i, 
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

	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
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
				kodak605_dump_status(&sts);
			break;
		}
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
}

/* Exported */
struct dyesub_backend kodak605_backend = {
	.name = "Kodak 605",
	.version = "0.23",
	.uri_prefix = "kodak605",
	.cmdline_usage = kodak605_cmdline,
	.cmdline_arg = kodak605_cmdline_arg,
	.init = kodak605_init,
	.attach = kodak605_attach,
	.teardown = kodak605_teardown,
	.read_parse = kodak605_read_parse,
	.main_loop = kodak605_main_loop,
	.devices = { 
	{ USB_VID_KODAK, USB_PID_KODAK_605, P_KODAK_605, "Kodak"},
	{ 0, 0, 0, ""}
	}
};

/* Kodak 605 data format

  Spool file consists of 14-byte header followed by plane-interleaved BGR data.
  Native printer resolution is 1844 pixels per row, and 1240 or 2434 rows.

  Header:

  01 40 0a 00                    Fixed header
  XX                             Unknown, always 01 in file, but 02 seen in sniffs sometimes (6x8?)
  CC                             Number of copies (1-255)
  00                             Always 0x00
  WW WW                          Number of columns, little endian. (Fixed at 1844)
  HH HH                          Number of rows, little endian. (1240 or 2434)
  DD                             0x01 (4x6) 0x03 (8x6)
  LL                             Laminate, 0x01 (off) or 0x02 (on)
  00

  ************************************************************************

  Note:  605 is Shinko CHC-S1545-5A

  ************************************************************************

   Kodak 605 Printer Comms:

   [[file header]] 01 40 0a 00 01 CC 00 WW WW HH HH MT LL 00

-> 01 00 00 00
<- [76 bytes -- status ]

   01 00 00 00  00 00 02 00  42 00 30 00  00 00 30 00
   00 00 13 00  00 00 75 00  00 00 30 00  00 00 5d 00
   00 00 00 00  00 00 01 01  00 00 00 01  00 02 00 00
   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00 00 00 00  00 00 01 00  00 00 00 00

-> 01 00 00 00
<- [76 bytes -- status ]

   01 00 00 00  00 00 02 00  42 00 30 00  00 00 30 00
   00 00 13 00  00 00 75 00  00 00 30 00  00 00 5d 00
   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00 00 00 00  00 00 01 00  00 00 00 00


-> 02 00 00 00
<- [113 bytes -- supported media/sizes? Always seems to be identical ]

   [ 13-byte header, plus 10 slots for 10-byte media definitions, see above ]

   01 00 00 00  00 00 02 00  67 00 02 0b  04 01 34 07
   d8 04 01 00  00 00 00 02  dc 05 34 08  01 00 00 00
   00 03 34 07  82 09 01 00  00 00 00 04  34 07 ba 09
   01 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00

-> 01 40 0a 00  01 01 00 34  07 d8 04 01  02 00  [[ unmodified header ]]
<- 01 00 00 00  00 00 XX 00  00 00  [[ Seen 0x01 and 0x02 ]
-> image data!
-> image data!

-> 01 00 00 00
<- [76 bytes -- status ?? ]

   01 00 00 00  00 00 01 00  42 00 31 00  00 00 31 00
   00 00 14 00  00 00 77 00  00 00 31 00  00 00 5d 00
   00 00 00 00  00 00 01 00  00 01 00 01  00 00 00 00
   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00 00 00 00  00 00 01 00  00 00 00 00

  Write tone curve data:

->  04 c0 0a 00  03 01 00 00  00 00 LL LL  00 00  [[ LL LL == 0x0600 in LE ]]
<-  01 00 00 00  00 00 XX 00  00 00  [[ Seen 0x01 and 0x02 ]

->  [[ 1536 bytes of LE tone curve data ]]

*/
