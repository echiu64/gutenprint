/*
 *   Kodak Professional 1400/805 CUPS backend -- libusb-1.0 version
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

#define BACKEND kodak1400_backend

#include "backend_common.h"

/* Program states */
enum {
	S_IDLE = 0,
	S_PRINTER_READY_Y,
	S_PRINTER_SENT_Y,
	S_PRINTER_READY_M,
	S_PRINTER_SENT_M,
	S_PRINTER_READY_C,
	S_PRINTER_SENT_C,
	S_PRINTER_READY_L,
	S_PRINTER_SENT_L,
	S_PRINTER_DONE,
	S_FINISHED,
};

#define CMDBUF_LEN 96
#define READBACK_LEN 8

/* File header */
struct kodak1400_hdr {
	uint8_t  hdr[4];
	uint16_t columns;
	uint16_t null1;
	uint16_t rows;
	uint16_t null2;
	uint32_t planesize;
	uint32_t null3;
	uint8_t  matte;
	uint8_t  laminate;
	uint8_t  unk1;  /* Always 0x01 */
	uint8_t  lam_strength;
	uint8_t  null4[12];
} __attribute__((packed));


/* Private data stucture */
struct kodak1400_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	struct kodak1400_hdr hdr;
	uint8_t *plane_r;
	uint8_t *plane_g;
	uint8_t *plane_b;
};

static int send_plane(struct kodak1400_ctx *ctx,
		      uint8_t planeno, uint8_t *planedata,
		      uint8_t *cmdbuf)
{
	uint16_t temp16;
	int ret;

	if (planeno != 1) {
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x74;
		cmdbuf[2] = 0x00;
		cmdbuf[3] = 0x50;
	
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     cmdbuf, CMDBUF_LEN)))
			return ret;
	}

	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x5a;
	cmdbuf[2] = 0x54;
	cmdbuf[3] = planeno;

	if (planedata) {
		temp16 = htons(ctx->hdr.columns);
		memcpy(cmdbuf+7, &temp16, 2);
		temp16 = htons(ctx->hdr.rows);
		memcpy(cmdbuf+9, &temp16, 2);
	}

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, CMDBUF_LEN)))
		return ret;

	if (planedata) {
		int i;
		for (i = 0 ; i < ctx->hdr.rows ; i++) {
			if ((ret = send_data(ctx->dev, ctx->endp_down,
					     planedata + i * ctx->hdr.columns, 
					     ctx->hdr.columns)))
				return ret;
		}
	}

	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x74;
	cmdbuf[2] = 0x01;
	cmdbuf[3] = 0x50;
	
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, CMDBUF_LEN)))
		return ret;

	return 0;
}

#define UPDATE_SIZE 1552
static int kodak1400_set_tonecurve(struct kodak1400_ctx *ctx, char *fname)
{
	libusb_device_handle *dev = ctx->dev;
	uint8_t endp_down = ctx->endp_down;
	uint8_t endp_up = ctx->endp_up;

	uint8_t cmdbuf[8];
	uint8_t respbuf[64];
	int ret = 0, num = 0;

	INFO("Set Tone Curve from '%s'\n", fname);

	uint16_t *data = malloc(UPDATE_SIZE);

	if (!data) {
		ERROR("Memory Allocation Failure!\n");
		return -1;
	}

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
	for (ret = 0; ret < (UPDATE_SIZE-16)/2 ; ret++) {
		data[ret] = cpu_to_le16(be16_to_cpu(data[ret]));
	}
	/* Null-terminate */
	memset(data + (UPDATE_SIZE-16)/2, 0, 16);

	/* Clear tables */
	memset(cmdbuf, 0, sizeof(cmdbuf));
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0xa2;
	if ((ret = send_data(dev, endp_down,
			     cmdbuf, 2))) {
		ret = -3;
		goto done;
	}
	
	ret = read_data(dev, endp_up,
			respbuf, sizeof(respbuf), &num);
	
	if (ret < 0)
		goto done;
	if (num != 8) {
		ERROR("Short Read! (%d/%d)\n", num, 8);
		ret = -4;
		goto done;
	}
	if (respbuf[1] != 0x01) {
		ERROR("Received unexpected response\n");
		ret = -5;
		goto done;
	}

	/* Set up the update command */
	memset(cmdbuf, 0, sizeof(cmdbuf));
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0xa0;
	cmdbuf[2] = 0x02;
	cmdbuf[3] = 0x03;
	cmdbuf[4] = 0x06;
	cmdbuf[5] = 0x10;   /* 06 10 == UPDATE_SIZE */
	if ((ret = send_data(dev, endp_down,
			     cmdbuf, 6)))
		goto done;

	/* Send the payload over */
	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) data, UPDATE_SIZE)))
		goto done;

	/* get the response */
	ret = read_data(dev, endp_up,
			respbuf, sizeof(respbuf), &num);
	
	if (ret < 0)
		goto done;
	if (num != 8) {
		ERROR("Short Read! (%d/%d)\n", num, 8);
		ret = -6;
		goto done;
	}
	if (respbuf[1] != 0x00) {
		ERROR("Received unexpected response!\n");
		ret = -7;
		goto done;
	}

done:
	free(data);

	return ret;
}

static void kodak1400_cmdline(void)
{
	DEBUG("\t\t[ -C filename ]  # Set tone curve\n");
}

int kodak1400_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct kodak1400_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "C:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'C':
			j = kodak1400_set_tonecurve(ctx, optarg);
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
}

static void *kodak1400_init(void)
{
	struct kodak1400_ctx *ctx = malloc(sizeof(struct kodak1400_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct kodak1400_ctx));
	
	return ctx;
}

static void kodak1400_attach(void *vctx, struct libusb_device_handle *dev, 
			     uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct kodak1400_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);

	ctx->type = lookup_printer_type(&kodak1400_backend,
					desc.idVendor, desc.idProduct);
}

static void kodak1400_teardown(void *vctx) {
	struct kodak1400_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->plane_r)
		free(ctx->plane_r);
	if (ctx->plane_g)
		free(ctx->plane_g);
	if (ctx->plane_b)
		free(ctx->plane_b);
	free(ctx);
}

static int kodak1400_read_parse(void *vctx, int data_fd) {
	struct kodak1400_ctx *ctx = vctx;
	int i, ret;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->plane_r) {
		free(ctx->plane_r);
		ctx->plane_r = NULL;
	}
	if (ctx->plane_g) {
		free(ctx->plane_g);
		ctx->plane_g = NULL;
	}
	if (ctx->plane_b) {
		free(ctx->plane_b);
		ctx->plane_b = NULL;
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
	if (ctx->hdr.hdr[0] != 'P' ||
	    ctx->hdr.hdr[1] != 'G' ||
	    ctx->hdr.hdr[2] != 'H' ||
	    ctx->hdr.hdr[3] != 'D') {
		ERROR("Unrecognized data format!\n");
		return CUPS_BACKEND_CANCEL;
	}
	ctx->hdr.planesize = le32_to_cpu(ctx->hdr.planesize);
	ctx->hdr.rows = le16_to_cpu(ctx->hdr.rows);
	ctx->hdr.columns = le16_to_cpu(ctx->hdr.columns);
	
	/* Set up plane data */
	ctx->plane_r = malloc(ctx->hdr.planesize);
	ctx->plane_g = malloc(ctx->hdr.planesize);
	ctx->plane_b = malloc(ctx->hdr.planesize);
	if (!ctx->plane_r || !ctx->plane_g || !ctx->plane_b) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_FAILED;
	}
	for (i = 0 ; i < ctx->hdr.rows ; i++) {
		int j;
		uint8_t *ptr;
		for (j = 0 ; j < 3 ; j++) {
			int remain;
			if (j == 0)
				ptr = ctx->plane_r + i * ctx->hdr.columns;
			else if (j == 1)
				ptr = ctx->plane_g + i * ctx->hdr.columns;
			else if (j == 2)
				ptr = ctx->plane_b + i * ctx->hdr.columns;

			remain = ctx->hdr.columns;
			do {
				ret = read(data_fd, ptr, remain);
				if (ret < 0) {
					ERROR("Read failed (%d/%d/%d) (%d/%d @ %d)\n", 
					      ret, remain, ctx->hdr.columns,
					      i, ctx->hdr.rows, j);
					perror("ERROR: Read failed");
					return CUPS_BACKEND_CANCEL;
				}
				ptr += ret;
				remain -= ret;
			} while (remain);
		}
	}

	return CUPS_BACKEND_OK;
}

static uint8_t idle_data[READBACK_LEN] = { 0xe4, 0x72, 0x00, 0x00,
					   0x00, 0x00, 0x00, 0x00 };

static int kodak1400_main_loop(void *vctx, int copies) {
	struct kodak1400_ctx *ctx = vctx;

	uint8_t rdbuf[READBACK_LEN], rdbuf2[READBACK_LEN];
	uint8_t cmdbuf[CMDBUF_LEN];
	int last_state = -1, state = S_IDLE;
	int num, ret;
	uint16_t temp16;

top:
	if (state != last_state) {
		if (dyesub_debug)
			DEBUG("last_state %d new %d\n", last_state, state);
	}

	/* Send Status Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x72;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			    cmdbuf, CMDBUF_LEN)))
		return CUPS_BACKEND_FAILED;

	/* Read in the printer status */
	ret = read_data(ctx->dev, ctx->endp_up,
			rdbuf, READBACK_LEN, &num);
	
	if (ret < 0)
		return CUPS_BACKEND_FAILED;
	if (memcmp(rdbuf, rdbuf2, READBACK_LEN)) {
		memcpy(rdbuf2, rdbuf, READBACK_LEN);
	} else if (state == last_state) {
		sleep(1);
	}
	last_state = state;

	/* Error handling */
	if (rdbuf[4] || rdbuf[5]) {
		ERROR("Error code reported by printer (%02x/%02x), terminating print\n",
		      rdbuf[4], rdbuf[5]);
		return CUPS_BACKEND_STOP;  // HOLD/CANCEL/FAILED?  XXXX parse error!
	}

	fflush(stderr);       

	switch (state) {
	case S_IDLE:
		INFO("Printing started\n");

		/* Send reset/attention */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     cmdbuf, CMDBUF_LEN)))
			return CUPS_BACKEND_FAILED;

		/* Send page setup */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x5a;
		cmdbuf[2] = 0x53;
		temp16 = be16_to_cpu(ctx->hdr.columns);
		memcpy(cmdbuf+3, &temp16, 2);
		temp16 = be16_to_cpu(ctx->hdr.rows);
		memcpy(cmdbuf+5, &temp16, 2);

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				    cmdbuf, CMDBUF_LEN)))
			return CUPS_BACKEND_FAILED;

		/* Send lamination toggle? */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x59;
		cmdbuf[2] = ctx->hdr.matte; // ???

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				    cmdbuf, CMDBUF_LEN)))
			return CUPS_BACKEND_FAILED;

		/* Send matte toggle */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x60;
		cmdbuf[2] = ctx->hdr.laminate;

		if (send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, CMDBUF_LEN))
			return CUPS_BACKEND_FAILED;

		/* Send lamination strength */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x62;
		cmdbuf[2] = ctx->hdr.lam_strength;

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				    cmdbuf, CMDBUF_LEN)))
			return CUPS_BACKEND_FAILED;

		/* Send unknown */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x61;
		cmdbuf[2] = ctx->hdr.unk1; // ???

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				    cmdbuf, CMDBUF_LEN)))
			return CUPS_BACKEND_FAILED;

		state = S_PRINTER_READY_Y;
		break;
	case S_PRINTER_READY_Y:
		INFO("Sending YELLOW plane\n");
		if ((ret = send_plane(ctx, 1, ctx->plane_b, cmdbuf)))
			return CUPS_BACKEND_FAILED;
		state = S_PRINTER_SENT_Y;
		break;
	case S_PRINTER_SENT_Y:
		if (!memcmp(rdbuf, idle_data, READBACK_LEN))
			state = S_PRINTER_READY_M;
		break;
	case S_PRINTER_READY_M:
		INFO("Sending MAGENTA plane\n");
		if ((ret = send_plane(ctx, 2, ctx->plane_g, cmdbuf)))
			return CUPS_BACKEND_FAILED;
		state = S_PRINTER_SENT_M;
		break;
	case S_PRINTER_SENT_M:
		if (!memcmp(rdbuf, idle_data, READBACK_LEN))
			state = S_PRINTER_READY_C;
		break;
	case S_PRINTER_READY_C:
		INFO("Sending CYAN plane\n");
		if ((ret = send_plane(ctx, 3, ctx->plane_r, cmdbuf)))
			return CUPS_BACKEND_FAILED;
		state = S_PRINTER_SENT_C;
		break;
	case S_PRINTER_SENT_C:
		if (!memcmp(rdbuf, idle_data, READBACK_LEN)) {
			if (ctx->hdr.laminate)
				state = S_PRINTER_READY_L;
			else
				state = S_PRINTER_DONE;
		}
		break;
	case S_PRINTER_READY_L:
		INFO("Laminating page\n");
		if ((ret = send_plane(ctx, 4, NULL, cmdbuf)))
			return CUPS_BACKEND_FAILED;
		state = S_PRINTER_SENT_L;
		break;
	case S_PRINTER_SENT_L:
		if (!memcmp(rdbuf, idle_data, READBACK_LEN))
			state = S_PRINTER_DONE;
		break;
	case S_PRINTER_DONE:
		INFO("Cleaning up\n");
		/* Cleanup */
		memset(cmdbuf, 0, CMDBUF_LEN);
		cmdbuf[0] = 0x1b;
		cmdbuf[1] = 0x74;
		cmdbuf[2] = 0x00;
		cmdbuf[3] = 0x50;

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				    cmdbuf, CMDBUF_LEN)))
			return CUPS_BACKEND_FAILED;

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
#define USB_VID_KODAK       0x040A
#define USB_PID_KODAK_1400  0x4022
#define USB_PID_KODAK_805   0x4034

struct dyesub_backend kodak1400_backend = {
	.name = "Kodak 1400/805",
	.version = "0.34",
	.uri_prefix = "kodak1400",
	.cmdline_usage = kodak1400_cmdline,
	.cmdline_arg = kodak1400_cmdline_arg,
	.init = kodak1400_init,
	.attach = kodak1400_attach,
	.teardown = kodak1400_teardown,
	.read_parse = kodak1400_read_parse,
	.main_loop = kodak1400_main_loop,
	.devices = {
	{ USB_VID_KODAK, USB_PID_KODAK_1400, P_KODAK_1400_805, "Kodak"},
	{ USB_VID_KODAK, USB_PID_KODAK_805, P_KODAK_1400_805, "Kodak"},
	{ 0, 0, 0, ""}
	}
};

/* Kodak 1400/805 data format

  Spool file consists of 36-byte header followed by row-interleaved BGR data.
  Native printer resolution is 2560 pixels per row, and 3010 or 3612 rows.

  Header:

  50 47 48 44     "PGHD"
  XX XX           Number of columns, Little endian.  Fixed at 2560.
  00 00           NULL
  XX XX           Number of rows, Little Endian
  00 00           NULL
  XX XX XX XX     Number of bytes per plane, Little Endian
  00 00 00 00     NULL
  XX              00 Glossy, 01 Matte   (Note: Kodak805 only supports Glossy)
  XX              01 to laminate, 00 to not.
  01              Unkown, always set to 01
  XX              Lamination Strength:
 
                  3c  Glossy
                  28  Matte +5
                  2e  Matte +4
                  34  Matte +3
                  3a  Matte +2
                  40  Matte +1
                  46  Matte 
                  52  Matte -1
                  5e  Matte -2
                  6a  Matte -3
                  76  Matte -4
                  82  Matte -5

  00 00 00 00 00 00 00 00 00 00 00 00       NULL

  ************************************************************************

  The data format actually sent to the Kodak 1400 is rather different.

    All commands are null-padded to 96 bytes.
    All readback values are 8 bytes long.

    Multi-byte numbers are sent BIG ENDIAN.
  
    Image data is sent via planes, one scanline per URB.

 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 00 00        # Idle response

 <-- 1b 00                           # Reset/attention?
 <-- 1b 5a 53  0a 00  0b c2          # Setup (ie hdr.columns and hdr.rows)
 <-- 1b 59 01                        # ?? hdr.matte ? 
 <-- 1b 60 XX                        # hdr.lamination
 <-- 1b 62 XX                        # hdr.lam_strength
 <-- 1b 61 01                        # ?? hdr.unk1 ?

 <-- 1b 5a 54 01  00 00 00  0a 00  0b c2  # start of plane 1 data
 <-- row 1
 <-- row 2
 <-- row last

 <-- 1b 74 01 50                     # ??
 
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 50 59        # Printing plane 1
  [ repeats until...]
 <-- 1b 72                           # Status query
 --> e4 72 00 00  40 00 50 59        # Paper loaded?
  [ repeats until...]
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 50 59        # Printing plane 1
  [ repeats until...]
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 00 00        # Idle response

 <-- 1b 74 00 50                     # ??
 <-- 1b 5a 54 02  00 00 00  0a 00  0b c2  # start of plane 2 data
 <-- row 1
 <-- row 2
 <-- row last
 <-- 1b 74 01 50                     # ??

 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 50 4d        # Printing plane 2
  [ repeats until...]
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 00 00        # Idle response

 <-- 1b 74 00 50                     # ??
 <-- 1b 5a 54 03  00 00 00  0a 00  0b c2  # start of plane 3 data
 <-- row 1
 <-- row 2
 <-- row last
 <-- 1b 74 01 50                     # ??

 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 50 43        # Printing plane 3
  [ repeats until...]
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 00 00        # Idle response

 ## this block is only present if lamination is used

 <-- 1b 74 00 50                     # ??
 <-- 1b 5a 54 04                     # start of lamination
 <-- 1b 74 01 50                     # ??

 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 50 50        # Laminating
  [ repeats until...]
 <-- 1b 72                           # Status query
 --> e4 72 00 00  00 00 00 00        # Idle response

 ## end lamination block

 <-- 1b 74 00 50                     # ??

 [[ DONE ]]

 Other readback codes seen:

 e4 72 00 00  40 00 50 59  -- ?? paper jam?
 e4 72 00 00  10 00 50 59  -- media red blink, error red blink, [media mismatch]]
 e4 72 00 00  10 01 50 59  -- ???
 e4 72 00 00  00 04 50 59  -- media red blink, error red  [media too small for image ?]
 e4 72 00 00  02 00 50 59  -- media off, error red. [out of paper]
 e4 72 00 00  02 01 00 00  -- media off, error red. [out of paper]
 e4 72 00 00  02 00 00 00  -- media off, error red. [out of paper]
 e4 72 00 00  02 00 50 50  -- media on, error red. [paper jam while laminating]

 *********************************************
  Calibration data:

 <-- 1b a2                           # ?? Reset cal tables?
 --> 00 01 00 00  00 00 00 00        

 <-- 1b a0 02 03 06 10               # 06 10 == 1552 bytes aka the CAL data.
 <-- cal data

  [[ Data is organized as three blocks of 512 bytes followed by 
     16 NULL bytes. 

     Each block appears to be 256 entries of 16-bit LE data, 
     so each input value is translated into a 16-bit number in the printer.

     Assuming blocks are ordered BGR.

  ]] 

 --> 00 00 00 00  00 00 00 00        

*/
