/*
 *   Kodak 605 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013 Solomon Peachy <pizza@shaftnet.org>
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
#define USB_PID_KODAK_605   0x402E

/* File header */
struct kodak605_hdr {
	uint8_t  hdr[4];   /* 01 40 0a 00 */
	uint8_t  unk1;     /* 01 or 02 */
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
	uint8_t *databuf;
	int datalen;
};

/* Program states */
enum {
	S_IDLE = 0,
	S_READY,
	S_STARTED,
	S_SENT_HDR,
	S_SENT_DATA,
	S_FINISHED,
};

#define READBACK_LEN 120

static void *kodak605_init(void)
{
	struct kodak605_ctx *ctx = malloc(sizeof(struct kodak605_ctx));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(struct kodak605_ctx));

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

	ctx->type = P_KODAK_605;
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

	if (!ctx)
		return 1;

	/* Read in then validate header */
	read(data_fd, &ctx->hdr, sizeof(ctx->hdr));
	if (ctx->hdr.hdr[0] != 0x01 ||
	    ctx->hdr.hdr[1] != 0x40 ||
	    ctx->hdr.hdr[2] != 0x0a ||
	    ctx->hdr.hdr[3] != 0x00) {
		ERROR("Unrecognized data format!\n");
		return(1);
	}

	ctx->datalen = le16_to_cpu(ctx->hdr.rows) * le16_to_cpu(ctx->hdr.columns) * 3;
	ctx->databuf = malloc(ctx->datalen);
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return 2;
	}

	{
		int remain = ctx->datalen;
		uint8_t *ptr = ctx->databuf;
		int ret;
		do {
			ret = read(data_fd, ptr, remain);
			if (ret < 0) {
				ERROR("Read failed (%d/%d/%d)\n", 
				      ret, remain, ctx->datalen);
				perror("ERROR: Read failed");
				return ret;
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}

	return 0;
}

static int kodak605_main_loop(void *vctx, int copies) {
	struct kodak605_ctx *ctx = vctx;

	uint8_t rdbuf[READBACK_LEN];
	uint8_t rdbuf2[READBACK_LEN];
	uint8_t cmdbuf[CMDBUF_LEN];

	int last_state = -1, state = S_IDLE;
	int i, num, ret;
	int pending = 0;

	if (!ctx)
		return 1;

	/* Printer handles generating copies.. */
#if 1
	ctx->hdr.copies = copies;
	copies = 1;
#else
	ctx->hdr.copies = 1;
#endif

top:
	if (state != last_state) {
		DEBUG("last_state %d new %d\n", last_state, state);
	}

	if (pending)
		goto skip_query;

	/* Send Status Query */
	cmdbuf[0] = 0x01;
	cmdbuf[1] = 0x00;
	cmdbuf[2] = 0x00;
	cmdbuf[3] = 0x00;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, CMDBUF_LEN)))
		return ret;

skip_query:
	/* Read in the printer status */
	memset(rdbuf, 0, READBACK_LEN);
	ret = libusb_bulk_transfer(ctx->dev, ctx->endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || num < 10) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, READBACK_LEN, ctx->endp_up);
		if (ret < 0)
			return ret;
		return 4;
	}

	if (num != 10 && num != 76 && num != 113) {
		ERROR("Unexpected readback from printer (%d/%d from 0x%02x))\n",
		      num, READBACK_LEN, ctx->endp_up);
		return ret;
	}

	if (memcmp(rdbuf, rdbuf2, READBACK_LEN)) {
		DEBUG("readback: ");
		for (i = 0 ; i < num ; i++) {
			DEBUG2("%02x ", rdbuf[i]);
		}
		DEBUG2("\n");
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
#if 0
		if (rdbuf[0] != 0x01 ||
		    rdbuf[1] != 0x02 ||
		    rdbuf[2] != 0x01) {
			break;
		}
#endif
		// XXX detect media type based on readback!

		INFO("Printing started; Sending init sequence\n");
		state = S_STARTED;

		break;
	case S_STARTED:
#if 0
		if (rdbuf[0] != 0x01 ||
		    rdbuf[2] != 0x00)
			break;

		/* Aappears to depend on media */
		if (rdbuf[1] != 0x0b &&
		    rdbuf[1] != 0x03)
			break;
#endif

		INFO("Sending image header\n");
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*)&ctx->hdr, sizeof(ctx->hdr))))
			return ret;
		pending = 1;
		state = S_SENT_HDR;
		break;
	case S_SENT_HDR:
		INFO("Waiting for printer to accept data\n");
		if (rdbuf[0] != 0x01 ||
		    rdbuf[6] == 0x00 ||
		    num != 10) {
			break;
		}
		INFO("Sending image data\n");
		if ((ret = send_data(ctx->dev, ctx->endp_down, 
				     ctx->databuf, ctx->datalen)))
			return ret;

		INFO("Image data sent\n");
		sleep(1);  /* An experiment */
		state = S_SENT_DATA;
		break;
	case S_SENT_DATA:
		INFO("Waiting for printer to acknowledge completion\n");
#if 0
		if (rdbuf[0] != 0x01 ||
		    rdbuf[1] != 0x02 ||
		    rdbuf[2] != 0x01) {
			break;
		}
#endif
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

	INFO("Print complete (%d remaining)\n", copies - 1);

	if (copies && --copies) {
		state = S_IDLE;
		goto top;
	}

	return 0;
}

static int kodak605_get_status(struct kodak605_ctx *ctx)
{
	uint8_t cmdbuf[4];
	uint8_t rdbuf[76];

	int ret, i, num = 0;

	/* Send Status Query */
	cmdbuf[0] = 0x01;
	cmdbuf[1] = 0x00;
	cmdbuf[2] = 0x00;
	cmdbuf[3] = 0x00;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;

	/* Read in the printer status */
	memset(rdbuf, 0, sizeof(rdbuf));
	ret = libusb_bulk_transfer(ctx->dev, ctx->endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);
	if (ret < 0 || num < (int)sizeof(rdbuf)) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(rdbuf), ctx->endp_up);
		if (ret < 0)
			return ret;
		return 4;
	}

	DEBUG("status: ");
	for (i = 0 ; i < num ; i++) {
		DEBUG2("%02x ", rdbuf[i]);
	}

	return 0;
}

static int kodak605_get_media(struct kodak605_ctx *ctx)
{
	uint8_t cmdbuf[4];
	uint8_t rdbuf[113];

	int ret, i, num = 0;

	/* Send Status Query */
	cmdbuf[0] = 0x02;
	cmdbuf[1] = 0x00;
	cmdbuf[2] = 0x00;
	cmdbuf[3] = 0x00;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;

	/* Read in the printer status */
	memset(rdbuf, 0, sizeof(rdbuf));
	ret = libusb_bulk_transfer(ctx->dev, ctx->endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);
	if (ret < 0 || num < (int)sizeof(rdbuf)) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(rdbuf), ctx->endp_up);
		if (ret < 0)
			return ret;
		return 4;
	}

	DEBUG("media: ");
	for (i = 0 ; i < num ; i++) {
		DEBUG2("%02x ", rdbuf[i]);
	}
	DEBUG("\n");

	return 0;
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
	if (tc_fd < 0)
		return -1;
	if (read(tc_fd, data, UPDATE_SIZE) != UPDATE_SIZE)
		return -2;
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
		return -1;

	/* Get response back */
	ret = libusb_bulk_transfer(dev, endp_up,
				   respbuf,
				   sizeof(respbuf),
				   &num,
				   5000);

	if (ret < 0 || (num != 10)) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(respbuf), endp_up);
		return ret;
	}

	// XXX parse the response?

	ret = libusb_bulk_transfer(dev, endp_up,
				   (uint8_t*) data,
				   sizeof(respbuf),
				   &num,
				   5000);
	if (ret < 0 || (num != sizeof(data))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(respbuf), endp_up);
		return ret;
	}
        
	/* We're done */
	free(data);
	return 0;
}


static void kodak605_cmdline(char *caller)
{
	DEBUG("\t\t%s [ -qs | -qm ]\n", caller);
	DEBUG("\t\t%s [ -stc filename ]\n", caller);
}

static int kodak605_cmdline_arg(void *vctx, int run, char *arg1, char *arg2)
{
	struct kodak605_ctx *ctx = vctx;

	if (!run || !ctx)
		return (!strcmp("-qs", arg1) ||
			!strcmp("-qm", arg1) ||
			!strcmp("-stc", arg1) );

	if (!strcmp("-qs", arg1))
		return kodak605_get_status(ctx);
	if (!strcmp("-qm", arg1))
		return kodak605_get_media(ctx);
	if (!strcmp("-stc", arg1))
		return kodak605_set_tonecurve(ctx, arg2);

	return -1;
}

/* Exported */
struct dyesub_backend kodak605_backend = {
	.name = "Kodak 605",
	.version = "0.08",
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
  XX                             Unknown, always 01 in file, but 02 seen in sniffs sometimes
  CC                             Number of copies (1-255)
  00                             Always 0x00
  WW WW                          Number of columns, little endian. (Fixed at 1844)
  HH HH                          Number of rows, little endian. (1240 or 2434)
  DD                             0x01 (4x6) 0x03 (8x6) 
  LL                             Laminate, 0x01 (off) or 0x02 (on)
  00

  ************************************************************************

   Kodak 605 Printer Comms:

   [[file header]] 01 40 0a 00 01 CC 00 WW WW HH HH MT LL 00

-> 01 00 00 00
<- [76 bytes -- status ]
-> 01 00 00 00
<- [76 bytes -- status ]
   01 00 00 00  00 00 02 00  42 00 30 00  00 00 30 00
   00 00 13 00  00 00 75 00  00 00 30 00  00 00 5d 00
   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00 00 00 00  00 00 01 00  00 00 00 00

   01 00 00 00  00 00 02 00  42 00 30 00  00 00 30 00
   00 00 13 00  00 00 75 00  00 00 30 00  00 00 5d 00
   00 00 00 00  00 00 01 01  00 00 00 01  00 20 00 00
   00 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00
   00 00 00 00  00 00 01 00  00 00 00 00

-> 02 00 00 00
<- [113 bytes -- supported media/sizes? Always seems to be identical ]

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

   01 00 00 00  00 00 01 00  42 00 31 00  00 00 31 00
   00 00 14 00  00 00 77 00  00 00 31 00  00 00 5d 00
   00 00 00 00  00 00 01 00  00 01 00 01  00 00 02 01
   00 00 00 01  00 02 02 01  00 00 00 01  00 02 00 00
   00 00 00 00  00 00 01 00  00 00 00 00


  Write tone curve data:

->  04 c0 0a 00  03 01 00 00  00 00 LL LL  00 00  [[ LL LL == 0x0600 in LE ]]
<-  01 00 00 00  00 00 XX 00  00 00  [[ Seen 0x01 and 0x02 ]

->  [[ 1536 bytes of LE tone curve data ]]

*/
