/*
 *   Mitsubishi CP-9550DW[-S] Photo Printer CUPS backend
 *
 *   (c) 2014-2015 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND mitsu9550_backend

#include "backend_common.h"

#define USB_VID_MITSU       0x06D3
#define USB_PID_MITSU_9550D  0x03A1
#define USB_PID_MITSU_9550DS 0x03A5  // or DZ/DZS/DZU

/* Private data stucture */
struct mitsu9550_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	uint8_t *databuf;
	int datalen;

	int is_s_variant;

	uint16_t rows;
	uint16_t cols;
};

/* Spool file structures */
struct mitsu9550_hdr1 {
	uint8_t  cmd[4]; /* 1b 57 20 2e */
	uint8_t  unk[10]; 
	uint16_t cols; /* BE */
	uint16_t rows; /* BE */
	uint8_t  null[32];
} __attribute__((packed));

struct mitsu9550_hdr2 {
	uint8_t  cmd[4]; /* 1b 57 21 2e */
	uint8_t  unk[24];
	uint16_t copies; /* BE, 1-580 */
	uint8_t  null[2];
	uint8_t  cut; /* 00 == normal, 83 == 2x6*2 */
	uint8_t  unkb[5];
	uint8_t  mode; /* 00 == normal, 80 == fine */
	uint8_t  unkc[11];
} __attribute__((packed));

struct mitsu9550_hdr3 {
	uint8_t  cmd[4]; /* 1b 57 22 2e */
	uint8_t  unk[7];
	uint8_t  mode2;  /* 00 == normal, 01 == finedeep */
	uint8_t  unkb[38];
} __attribute__((packed));

struct mitsu9550_hdr4 {
	uint8_t  cmd[4]; /* 1b 57 26 2e */
	uint8_t  unk[46];
} __attribute__((packed));

struct mitsu9550_plane {
	uint8_t  cmd[4]; /* 1b 5a 54 00 */
	uint8_t  null[2];
	uint16_t rem_rows;  /* BE, normally 0 */
	uint16_t columns;   /* BE */
	uint16_t rows;      /* BE */
} __attribute__((packed));

struct mitsu9550_cmd {
	uint8_t cmd[4];
} __attribute__((packed));

/* Printer data structures */
struct mitsu9550_media {
	uint8_t  hdr[2];  /* 24 2e */
	uint8_t  unk[12];
	uint8_t  type;
	uint8_t  unka[13];
	uint16_t max;  /* BE, prints per media */
	uint8_t  unkb[2];
	uint16_t remain; /* BE, prints remaining */
	uint8_t  unkc[14];
} __attribute__((packed));

struct mitsu9550_status {
	uint8_t  hdr[2]; /* 30 2e */
	uint8_t  null[4];
	uint8_t  sts1; // MM
	uint8_t  nullb[1];
	uint16_t copies; // NN
	uint8_t  nullc[6];
	uint8_t  sts3; // QQ
	uint8_t  sts4; // RR
	uint8_t  sts5; // SS
	uint8_t  nulld[25];
	uint8_t  sts6; // TT
	uint8_t  sts7; // UU
	uint8_t  nulle[2];
} __attribute__((packed));

struct mitsu9550_status2 {
	uint8_t  hdr[2]; /* 21 2e */
	uint8_t  unk[40];
	uint8_t  remain; /* BE, media remaining */
	uint8_t  unkb[4];
} __attribute__((packed));

#define CMDBUF_LEN   64
#define READBACK_LEN 128

static void *mitsu9550_init(void)
{
	struct mitsu9550_ctx *ctx = malloc(sizeof(struct mitsu9550_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct mitsu9550_ctx));

	return ctx;
}

static void mitsu9550_attach(void *vctx, struct libusb_device_handle *dev,
			    uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct mitsu9550_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);

	ctx->type = lookup_printer_type(&mitsu9550_backend,
					desc.idVendor, desc.idProduct);	
}


static void mitsu9550_teardown(void *vctx) {
	struct mitsu9550_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	free(ctx);
}

static int mitsu9550_read_parse(void *vctx, int data_fd) {
	struct mitsu9550_ctx *ctx = vctx;
	struct mitsu9550_hdr1 hdr;

	int remain, i;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	/* Read in initial header */
	remain = sizeof(hdr);
	while (remain > 0) {
		i = read(data_fd, ((uint8_t*)&hdr) + sizeof(hdr) - remain, remain);
		if (i == 0)
			return CUPS_BACKEND_CANCEL;
		if (i < 0)
			return CUPS_BACKEND_CANCEL;
		remain -= i;
	}

	/* Sanity check */
	if (hdr.cmd[0] != 0x1b ||
	    hdr.cmd[1] != 0x57 ||
	    hdr.cmd[2] != 0x20 ||
	    hdr.cmd[3] != 0x2e) {
		ERROR("Unrecognized data format!\n");
		return CUPS_BACKEND_CANCEL;
	}

	/* Work out printjob size */
	ctx->rows = be16_to_cpu(hdr.rows);
	ctx->cols = be16_to_cpu(hdr.cols);

	remain = ctx->rows * ctx->cols + sizeof(struct mitsu9550_plane);
	remain *= 3;
	remain += sizeof(struct mitsu9550_hdr2) + sizeof(struct mitsu9550_hdr3)+ sizeof(struct mitsu9550_hdr4) + sizeof(struct mitsu9550_cmd);

	/* Allocate buffer */
	ctx->databuf = malloc(remain + sizeof(struct mitsu9550_hdr1));
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_FAILED;
	}

	memcpy(ctx->databuf, &hdr, sizeof(struct mitsu9550_hdr1));
	ctx->datalen = sizeof(struct mitsu9550_hdr1);

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

static int mitsu9550_get_status(struct mitsu9550_ctx *ctx, uint8_t *resp, int status, int status2, int media)
{
	struct mitsu9550_cmd cmd;
	int num, ret;

	/* Send Printer Query */
	cmd.cmd[0] = 0x1b;
	cmd.cmd[1] = 0x56;
	if (status)
		cmd.cmd[2] = 0x30;
	else if (status2)
		cmd.cmd[2] = 0x21;
	else if (media)
		cmd.cmd[2] = 0x24;
	cmd.cmd[3] = 0x00;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) &cmd, sizeof(cmd))))
		return ret;
	ret = read_data(ctx->dev, ctx->endp_up,
			resp, sizeof(struct mitsu9550_status), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(struct mitsu9550_status)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(struct mitsu9550_status));
		return 4;
	}

	return 0;
}

static int validate_media(int type, int cols, int rows) {
	switch(type) {
	case 0x01: /* 3.5x5 */
		if (cols != 1812 && rows != 1240)
			return 1;
		break;
	case 0x02: /* 4x6 */
	case 0x03: /* PC ??? */ 
		if (cols != 2152)
			return 1;
		if (rows != 1416 && rows != 1184 &&
		    rows != 1240)
			return 1;
		break;
	case 0x04: /* 5x7 */
		if (cols != 1812)
			return 1;
		if (rows != 1240 && rows != 2452)
			return 1;
		break;
	case 0x05: /* 6x9 */
		if (cols != 2152)
			return 1;
		if (rows != 1416 && rows != 2972 &&
		    rows != 2956 && rows != 3146)
			return 1;
		break;
	case 0x06: /* V */
		break;
	default: /* Unknown */
		break;
	}
	return 0;
}

static int mitsu9550_main_loop(void *vctx, int copies) {
	struct mitsu9550_ctx *ctx = vctx;
	struct mitsu9550_hdr2 *hdr2;
	struct mitsu9550_cmd cmd;
	uint8_t rdbuf[READBACK_LEN];

	uint8_t *ptr;
	
	int ret;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Update printjob header to reflect number of requested copies */
	hdr2 = (struct mitsu9550_hdr2 *) (ctx->databuf + sizeof(struct mitsu9550_hdr1));
	hdr2->copies = cpu_to_be16(copies);

	ptr = ctx->databuf;
	
top:
	if (ctx->type == P_MITSU_9550S) {
		int num;
		
		/* Send "unknown 1" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x53;
		cmd.cmd[2] = 0xc5;
		cmd.cmd[3] = 0x9d;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
		
		/* Send "unknown 2" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x4b;
		cmd.cmd[2] = 0x7f;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
		
		ret = read_data(ctx->dev, ctx->endp_up,
				rdbuf, READBACK_LEN, &num);
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		// seen so far: eb 4b 7f 00  02 00 5e
	}

	/* Query statuses */
	{
		struct mitsu9550_status *sts = (struct mitsu9550_status*) rdbuf;
		//struct mitsu9550_status2 *sts2 = (struct mitsu9550_status2*) rdbuf;
		struct mitsu9550_media *media = (struct mitsu9550_media *) rdbuf;

		ret = mitsu9550_get_status(ctx, rdbuf, 0, 0, 1); // media
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Sanity-check media response */
		if (media->remain == 0 || media->max == 0) {
			ERROR("Printer out of media!\n");
			return CUPS_BACKEND_HOLD;
		}
		if (validate_media(media->type, ctx->cols, ctx->rows)) {
			ERROR("Incorrect media (%d) type for printjob (%dx%d)!\n", media->type, ctx->cols, ctx->rows);
			return CUPS_BACKEND_HOLD;
		}

		ret = mitsu9550_get_status(ctx, rdbuf, 0, 1, 0); // status2
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		
		ret = mitsu9550_get_status(ctx, rdbuf, 1, 0, 0); // status
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		
		/* Make sure we're idle */
		if (sts->sts5 != 0) {  /* Printer ready for another job */
			sleep(1);
			goto top;
		}
	}

	/* Now it's time for the actual print job! */
	
	if (ctx->type == P_MITSU_9550S) {
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x44;
		cmd.cmd[2] = 0;
		cmd.cmd[3] = 0;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, 4)))
			return CUPS_BACKEND_FAILED;
	}
	
	/* Query statuses */
	{
		struct mitsu9550_status *sts = (struct mitsu9550_status*) rdbuf;
//		struct mitsu9550_status2 *sts2 = (struct mitsu9550_status2*) rdbuf;
		struct mitsu9550_media *media = (struct mitsu9550_media *) rdbuf;

		ret = mitsu9550_get_status(ctx, rdbuf, 0, 0, 1); // media
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Sanity-check media response */
		if (media->remain == 0 || media->max == 0) {
			ERROR("Printer out of media!\n");
			return CUPS_BACKEND_HOLD;
		}
		if (validate_media(media->type, ctx->cols, ctx->rows)) {
			ERROR("Incorrect media (%d) type for printjob (%dx%d)!\n", media->type, ctx->cols, ctx->rows);
			return CUPS_BACKEND_HOLD;
		}

		ret = mitsu9550_get_status(ctx, rdbuf, 0, 1, 0); // status2
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		
		ret = mitsu9550_get_status(ctx, rdbuf, 1, 0, 0); // status
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Make sure we're idle */
		if (sts->sts5 != 0) {  /* Printer ready for another job */
			sleep(1);
			goto top;
		}
	}

	/* Send printjob headers from spool data */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) ptr, sizeof(struct mitsu9550_hdr1))))
		return CUPS_BACKEND_FAILED;
	ptr += sizeof(struct mitsu9550_hdr1);
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) ptr, sizeof(struct mitsu9550_hdr2))))
		return CUPS_BACKEND_FAILED;
	ptr += sizeof(struct mitsu9550_hdr2);
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) ptr, sizeof(struct mitsu9550_hdr3))))
		return CUPS_BACKEND_FAILED;
	ptr += sizeof(struct mitsu9550_hdr3);
	if (ctx->type != P_MITSU_9550S) {
		// XXX need to investigate what hdr4 is about
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) ptr, sizeof(struct mitsu9550_hdr4))))
			return CUPS_BACKEND_FAILED;		
	}
	ptr += sizeof(struct mitsu9550_hdr4);
	
	if (ctx->type == P_MITSU_9550S) {
		/* Send "start data" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x5a;
		cmd.cmd[2] = 0x43;
		cmd.cmd[3] = 0x00;
		
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	}
	/* Send plane data */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) ptr, sizeof(struct mitsu9550_plane))))
		return CUPS_BACKEND_FAILED;
	ptr += sizeof(struct mitsu9550_plane);
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) ptr, ctx->rows * ctx->cols)))
		return CUPS_BACKEND_FAILED;
	ptr += ctx->rows * ctx->cols;
	
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) ptr, sizeof(struct mitsu9550_plane))))
		return CUPS_BACKEND_FAILED;
	ptr += sizeof(struct mitsu9550_plane);
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) ptr, ctx->rows * ctx->cols)))
		return CUPS_BACKEND_FAILED;
	ptr += ctx->rows * ctx->cols;
	
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) ptr, sizeof(struct mitsu9550_plane))))
		return CUPS_BACKEND_FAILED;
	ptr += sizeof(struct mitsu9550_plane);
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) ptr, ctx->rows * ctx->cols)))
		return CUPS_BACKEND_FAILED;
	ptr += ctx->rows * ctx->cols;


	/* Query statuses */
	{
		struct mitsu9550_status *sts = (struct mitsu9550_status*) rdbuf;
//		struct mitsu9550_status2 *sts2 = (struct mitsu9550_status2*) rdbuf;
		struct mitsu9550_media *media = (struct mitsu9550_media *) rdbuf;
		
		ret = mitsu9550_get_status(ctx, rdbuf, 0, 0, 1); // media
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Sanity-check media response */
		if (media->remain == 0 || media->max == 0) {
			ERROR("Printer out of media!\n");
			return CUPS_BACKEND_HOLD;
		}

		ret = mitsu9550_get_status(ctx, rdbuf, 0, 1, 0); // status2
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		
		ret = mitsu9550_get_status(ctx, rdbuf, 1, 0, 0); // status
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		
		/* Make sure we're ready to proceed */
		if (sts->sts5 != 0) {
			ERROR("Unexpected response (sts5 %02x)\n", sts->sts5);
			return CUPS_BACKEND_FAILED;
		}
		if (!(sts->sts3 & 0xc0)) {
			ERROR("Unexpected response (sts3 %02x)\n", sts->sts3);
			return CUPS_BACKEND_FAILED;
		}
	}
	
	if (ctx->type == P_MITSU_9550S) {
		/* Send "end data" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x50;
		cmd.cmd[2] = 0x47;
		cmd.cmd[3] = 0x00;			
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	} else {
		/* Send "end data" command from spool file */
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ptr, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
		ptr += sizeof(cmd);
	}

	/* Status loop, run until printer reports completion */
	while(1) {
		struct mitsu9550_status *sts = (struct mitsu9550_status*) rdbuf;
//		struct mitsu9550_status2 *sts2 = (struct mitsu9550_status2*) rdbuf;
		struct mitsu9550_media *media = (struct mitsu9550_media *) rdbuf;
		
		ret = mitsu9550_get_status(ctx, rdbuf, 0, 0, 1); // media
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Sanity-check media response */
		if (media->remain == 0 || media->max == 0) {
			ERROR("Printer out of media!\n");
			return CUPS_BACKEND_HOLD;
		}

		ret = mitsu9550_get_status(ctx, rdbuf, 0, 1, 0); // status2
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		
		ret = mitsu9550_get_status(ctx, rdbuf, 1, 0, 0); // status
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		INFO("%03d copies remaining\n", be16_to_cpu(sts->copies));

		if (!sts->sts1) /* If printer transitions to idle */
			break;

		if (fast_return && !be16_to_cpu(sts->copies)) { /* No remaining prints */
                        INFO("Fast return mode enabled.\n");
			break;
                }

		if (fast_return && !sts->sts5) { /* Ready for another job */
			INFO("Fast return mode enabled.\n");
			break;
		}

		sleep(1);
	}
	
	INFO("Print complete\n");

	return CUPS_BACKEND_OK;
}

static char *mitsu9550_media_types(uint8_t type)
{
	switch (type) {
	case 0x01:
		return "3.5x5";
	case 0x02:
		return "4x6";
	case 0x03:
		return "PC";
	case 0x04:
		return "5x7";
	case 0x05:
		return "6x9";
	case 0x06:
		return "V";
	default:
		return "Unknown";
	}
	return NULL;
}

static void mitsu9550_dump_media(struct mitsu9550_media *resp)
{
	INFO("Media type       : %02x (%s)\n",
	     resp->type, mitsu9550_media_types(resp->type));
	INFO("Media remaining  : %03d/%03d\n",
	     be16_to_cpu(resp->remain), be16_to_cpu(resp->max));
}

static void mitsu9550_dump_status(struct mitsu9550_status *resp)
{
	INFO("Printer status    : %02x (%s)\n",
	     resp->sts1, resp->sts1 ? "Printing": "Idle");
	INFO("Pages remaining   : %03d\n",
	     be16_to_cpu(resp->copies));
	INFO("Other status      : %02x %02x %02x  %02x %02x\n",
	     resp->sts3, resp->sts4, resp->sts5, resp->sts6, resp->sts7);
	     
}

static int mitsu9550_query_media(struct mitsu9550_ctx *ctx)
{
	struct mitsu9550_media resp;
	int ret;

	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, 0, 0, 1);

	if (!ret)
		mitsu9550_dump_media(&resp);

	return ret;
}

static int mitsu9550_query_status(struct mitsu9550_ctx *ctx)
{
	struct mitsu9550_status resp;
	int ret;

	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, 1, 0, 0);

	if (!ret)
		mitsu9550_dump_status(&resp);

	return ret;
}

static int mitsu9550_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct mitsu9550_cmd cmd;
	uint8_t rdbuf[READBACK_LEN];
	uint8_t *ptr;
	int ret, num, i;

	cmd.cmd[0] = 0x1b;
	cmd.cmd[1] = 0x72;
	cmd.cmd[2] = 0x6e;
	cmd.cmd[3] = 0x00;

	if ((ret = send_data(dev, endp_down,
                             (uint8_t*) &cmd, sizeof(cmd))))
                return (ret < 0) ? ret : CUPS_BACKEND_FAILED;

	ret = read_data(dev, endp_up,
			rdbuf, READBACK_LEN, &num);
	
	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	if ((unsigned int)num < sizeof(cmd) + 1) /* Short read */
		return CUPS_BACKEND_FAILED;
	
	if (rdbuf[0] != 0xe4 ||
	    rdbuf[1] != 0x72 ||
	    rdbuf[2] != 0x6e ||
	    rdbuf[3] != 0x00) /* Bad response */
		return CUPS_BACKEND_FAILED;

	/* If response is truncated, handle it */
	num -= (sizeof(cmd) + 1);
	if ((unsigned int) num != rdbuf[4])
		WARNING("Short serno read! (%d vs %d)\r\n",
			num, rdbuf[4]);

	/* model and serial number are encoded as 16-bit unicode, 
	   little endian, separated by spaces. */
	i = num;
	ptr = rdbuf + 5;
	while (i > 0 && buf_len > 1) {
		if (*ptr != 0x20)
			*buf++ = *ptr;
		buf_len--;
		ptr += 2;
		i -= 2;
	}
	*buf = 0; /* Null-terminate the returned string */
	
	return ret;
}

static void mitsu9550_cmdline(void)
{
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
}

static int mitsu9550_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct mitsu9550_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "ms")) >= 0) {
		switch(i) {
 		GETOPT_PROCESS_GLOBAL			
		case 'm':
			j = mitsu9550_query_media(ctx);
			break;
		case 's':
			j = mitsu9550_query_status(ctx);
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
}

/* Exported */
struct dyesub_backend mitsu9550_backend = {
	.name = "Mitsubishi CP-9550DW-S",
	.version = "0.15",
	.uri_prefix = "mitsu9550",
	.cmdline_usage = mitsu9550_cmdline,
	.cmdline_arg = mitsu9550_cmdline_arg,
	.init = mitsu9550_init,
	.attach = mitsu9550_attach,
	.teardown = mitsu9550_teardown,
	.read_parse = mitsu9550_read_parse,
	.main_loop = mitsu9550_main_loop,
	.query_serno = mitsu9550_query_serno,
	.devices = {
	{ USB_VID_MITSU, USB_PID_MITSU_9550D, P_MITSU_9550, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_9550DS, P_MITSU_9550S, ""},
	{ 0, 0, 0, ""}
	}
};

/* Mitsubish CP-9550D/DW spool data format 

   Spool file consists of four 50-byte headers, followed by three image
   planes (BGR, each with a 12-byte header), and a 4-byte footer.

   All multi-byte numbers are big endian.

   ~~~ Printer Init: 4x 50-byte blocks:

   1b 57 20 2e 00 0a 10 00  00 00 00 00 00 00 07 14 :: 0714 = 1812 = X res
   04 d8 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: 04d8 = 1240 = Y res
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   00 00 

   1b 57 21 2e 00 80 00 22  08 03 00 00 00 00 00 00 :: ZZ = num copies (>= 0x01)
   00 00 00 00 00 00 00 00  00 00 00 00 ZZ ZZ 00 00 :: YY 00 = normal, 80 = Fine
   XX 00 00 00 00 00 YY 00  00 00 00 00 00 00 00 00 :: XX 00 = normal, 83 = Cut 2x6
   00 01 

   1b 57 22 2e 00 40 00 00  00 00 00 XX 00 00 00 00 :: 00 = normal, 01 = FineDeep
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 
   00 00 

   1b 57 26 2e 00 70 00 00  00 00 00 00 01 01 00 00 
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 
   00 00  

  ~~~~ Data follows:   Data is 8-bit BGR.

   1b 5a 54 00 00 00 00 00  07 14 04 d8  :: 0714 == row len, 04d8 == rows
                     ^^ ^^               :: 0000 == remaining rows

   Data follows immediately, no padding.

   1b 5a 54 00 00 00 00 00  07 14 04 d8  :: Another plane.

   Data follows immediately, no padding.

   1b 5a 54 00 00 00 00 00  07 14 04 d8  :: Another plane.

   Data follows immediately, no padding.

  ~~~~ Footer:

   1b 50 46 00

  ~~~~ QUESTIONS:

   * Lamination control?
   * Other multi-cut modes (on 6x9 media: 4x6*2, 4.4x6*2, 3x6*3, 2x6*4)

 ***********************************************************************

 * Mitsubishi ** CP-9550DW-S ** Communications Protocol:

  [[ Unknown ]]

 -> 1b 53 c5 9d

  [[ Unknown ]]

 -> 1b 4b 7f 00
 <- eb 4b 8f 00 02 00 5e  [[ '02' seems to be a length ]] 

  [[ Unknown ]]

 -> 1b 53 00 00

  Query Model & Serial number

 -> 1b 72 6e 00
 <- e4 82 6e 00 LL 39 00 35  00 35 00 30 00 5a 00 20
    00 41 00 32 00 30 00 30  00 36 00 37 00

     'LL' is length.  Data is returned in 16-bit unicode, LE.
     Contents are model ('9550Z'), then space, then serialnum ('A20067')

  Media Query

 -> 1b 56 24 00
 <- 24 2e 00 00 00 00 00 00  00 00 00 00 00 00 TT 00 :: TT = Type
    00 00 00 00 00 00 00 00  00 00 00 00 MM MM 00 00 :: MM MM = Max prints
    NN NN 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: NN NN = Remaining

  Status Query
 
 -> 1b 56 30 00
 -> 30 2e 00 00 00 00 MM 00  NN NN 00 00 00 00 00 00 :: MM, NN
    QQ RR SS 00 00 00 00 00  00 00 00 00 00 00 00 00 :: QQ, RR, SS
    00 00 00 00 00 00 00 00  00 00 00 00 TT UU 00 00 :: TT, UU 

  Status Query B (not sure what to call this)

 -> 1b 56 21 00
 <- 21 2e 00 80 00 22 a8 0b  00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00  00 00 00 QQ 00 00 00 00 :: QQ == Prints in job?
    00 00 00 00 00 00 00 00  00 00 NN NN 0A 00 00 01 :: NN NN = Remaining media

  [[ Unknown ]]

 -> 1b 44

  [[ Header 1 -- See above ]]

 -> 1b 57 20 2e ....

  [[ Header 2 -- See above ]]

 -> 1b 57 21 2e ....

  [[ Header 3 -- See above ]]

 -> 1b 57 22 2e ....

  [[ Unknown -- Start Data ? ]]

 -> 1b 5a 43 00

  [[ Plane header #1 (Blue) ]]

 -> 1b 5a 54 00 00 00 00 00  XX XX YY YY :: XX XX == Columns, YY YY == Rows

    Followed by image plane #1 (Blue), XXXX * YYYY bytes

  [[ Plane header #2 (Green) ]]

 -> 1b 5a 54 00 00 00 00 00  XX XX YY YY :: XX XX == Columns, YY YY == Rows

    Followed by image plane #2 (Green), XXXX * YYYY bytes

  [[ Plane header #3 (Red) ]]

 -> 1b 5a 54 00 00 00 00 00  XX XX YY YY :: XX XX == Columns, YY YY == Rows

    Followed by image plane #3 (Red), XXXX * YYYY bytes

  [[ Unknown -- End Data aka START print? ]]

 -> 1b 50 47 00

  [[ At this point, loop status/status b/media queries until printer idle ]]

    MM, NN, QQ RR SS, TT UU

 <- 00  00  3e 00 00  8a 44  :: Idle.
    00  00  7e 00 00  8a 44  :: Plane data submitted, pre "end data" cmd
    00  00  7e 40 01  8a 44  :: "end data" sent
    30  01  7e 40 01  8a 44
    38  01  7e 40 01  8a 44
    59  01  7e 40 01  8a 44
    59  01  7e 40 00  8a 44
    4d  01  7e 40 00  8a 44
     [...]
    43  01  7e 40 00  82 44
     [...]
    50  01  7e 40 00  80 44
     [...]
    31  01  7e 40 00  7d 44
     [...]
    00  00  3e 00 00  80 44  :: Idle.

  Also seen: 

    00  00  3e 00 00  96 4b  :: Idle
    00  00  be 00 00  96 4b  :: Data submitted, pre "start"
    00  00  be 80 01  96 4b  :: print start sent
    30  00  be 80 01  96 4c
     [...]
    30  03  be 80 01  89 4b
    38  03  be 80 01  8a 4b
    59  03  be 80 01  8b 4b
     [...]
    4d  03  be 80 01  89 4b
     [...]
    43  03  be 80 01  89 4b
     [...]
    50  03  be 80 01  82 4b
     [...]
    31  03  be 80 01  80 4b
     [...]

 Working theory of interpreting the status flags:

  MM :: 00 is idle, else mechanical printer state.
  NN :: Remaining prints in job, or 0x00 0x00 when idle.
  QQ :: ?? 0x3e + 0x40 or 0x80 (see below)
  RR :: ?? 0x00 is idle, 0x40 or 0x80 is "printing"?
  SS :: ?? 0x00 means "ready for another print" but 0x01 is "busy"
  TT :: ?? seen values between 0x7c through 0x96)
  UU :: ?? seen values between 0x44 and 0x4c
 
  *** 

   Other printer commands seen:

  [[ Set error policy ?? aka "header 4" ]]

 -> 1b 57 26 2e 00 QQ 00 00  00 00 00 00 RR SS 00 00 :: QQ/RR 00 00 00 [9550S]
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ::       20 01 00 [9550S w/ ignore failures on]
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ::       70 01 01 [9550]
    00 00

 */
