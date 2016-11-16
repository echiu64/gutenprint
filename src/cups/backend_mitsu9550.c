/*
 *   Mitsubishi CP-9xxx Photo Printer Family CUPS backend
 *
 *   (c) 2014-2016 Solomon Peachy <pizza@shaftnet.org>
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

#define USB_VID_MITSU        0x06D3
#define USB_PID_MITSU_9500D  0x0393
#define USB_PID_MITSU_9000D  0x0394
#define USB_PID_MITSU_9000AM 0x0395
#define USB_PID_MITSU_9550D  0x03A1
#define USB_PID_MITSU_9550DS 0x03A5  // or DZ/DZS/DZU
#define USB_PID_MITSU_9600D  0x03A9
//#define USB_PID_MITSU_9600DS  XXXXXX
#define USB_PID_MITSU_9800D  0x03AD
#define USB_PID_MITSU_9800DS 0x03AE
#define USB_PID_MITSU_98__D  0x3B21
//#define USB_PID_MITSU_9810D   XXXXXX
//#define USB_PID_MITSU_9820DS  XXXXXX

/* Spool file structures */

/* Print parameters1 */
struct mitsu9550_hdr1 {
	uint8_t  cmd[4]; /* 1b 57 20 2e */
	uint8_t  unk[10]; /* 00 0a 10 00 [...] */
	uint16_t cols; /* BE */
	uint16_t rows; /* BE */
	uint8_t  matte;  /* CP9810 only. 01 for matte, 00 glossy */
	uint8_t  null[31];
} __attribute__((packed));

/* Print parameters2 */
struct mitsu9550_hdr2 {
	uint8_t  cmd[4]; /* 1b 57 21 2e */
	uint8_t  unk[24]; /* 00 80 00 22 08 03 00 [...] */
	uint16_t copies; /* BE, 1-680 */
	uint8_t  null[2];
	uint8_t  cut; /* 00 == normal, 83 == 2x6*2 */
	uint8_t  unkb[5];
	uint8_t  mode; /* 00 == fine, 80 == superfine */
	uint8_t  unkc[11]; /* 00 [...] 00 01 */
} __attribute__((packed));

/* Fine Deep selection (9550 only) */
struct mitsu9550_hdr3 {
	uint8_t  cmd[4]; /* 1b 57 22 2e */
	uint8_t  unk[7]; /* 00 40 00 [...] */
	uint8_t  mode2;  /* 00 == normal, 01 == finedeep */
	uint8_t  null[38];
} __attribute__((packed));

/* Error policy? */
struct mitsu9550_hdr4 {
	uint8_t  cmd[4]; /* 1b 57 26 2e */
	uint8_t  unk[46]; /* 00 70 00 00 00 00 00 00 01 01 00 [...] */
} __attribute__((packed));

/* Data plane header */
struct mitsu9550_plane {
	uint8_t  cmd[4]; /* 1b 5a 54 XX */  /* XX == 0x10 if 16bpp, 0x00 for 8bpp */
	uint16_t row_offset; /* BE, normally 0, where we start dumping data */
	uint16_t null;       /* ??? */
	uint16_t cols;       /* BE */
	uint16_t rows;       /* BE */
} __attribute__((packed));

struct mitsu9550_cmd {
	uint8_t cmd[4];
} __attribute__((packed));

/* Private data stucture */
struct mitsu9550_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;
	int is_s;

	uint8_t *databuf;
	uint32_t datalen;

	uint16_t rows;
	uint16_t cols;
	uint32_t plane_len;

	uint16_t last_donor;
	uint16_t last_remain;
	int marker_reported;

	/* Parse headers separately */
	struct mitsu9550_hdr1 hdr1;
	int hdr1_present;
	struct mitsu9550_hdr2 hdr2;
	int hdr2_present;
	struct mitsu9550_hdr3 hdr3;
	int hdr3_present;
	struct mitsu9550_hdr4 hdr4;
	int hdr4_present;
};

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
	uint16_t copies; // BE, NN
	uint8_t  sts2; // ZZ  (9600 only?)
	uint8_t  nullc[5];
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
	uint8_t  unk[39];
	uint16_t remain; /* BE, media remaining */
	uint8_t  unkb[4]; /* 0a 00 00 01 */
} __attribute__((packed));

#define CMDBUF_LEN   64
#define READBACK_LEN 128

#define QUERY_STATUS()	\
	do {\
		struct mitsu9550_status *sts = (struct mitsu9550_status*) rdbuf;\
		/* struct mitsu9550_status2 *sts2 = (struct mitsu9550_status2*) rdbuf; */ \
		struct mitsu9550_media *media = (struct mitsu9550_media *) rdbuf; \
		uint16_t donor, remain;	\
		/* media */ \
		ret = mitsu9550_get_status(ctx, rdbuf, 0, 0, 1); \
		if (ret < 0) \
			return CUPS_BACKEND_FAILED; \
		\
		/* Tell CUPS about the consumables we report */ \
		if (!ctx->marker_reported) { \
			ctx->marker_reported = 1; \
			ATTR("marker-colors=#00FFFF#FF00FF#FFFF00\n");	\
			ATTR("marker-high-levels=100\n"); \
			ATTR("marker-low-levels=10\n");	\
			ATTR("marker-names='%s'\n", mitsu9550_media_types(media->type, ctx->is_s)); \
			ATTR("marker-types=ribbonWax\n"); \
		} \
		\
		/* Sanity-check media response */ \
		if (media->remain == 0 || media->max == 0) { \
			ERROR("Printer out of media!\n"); \
			ATTR("marker-levels=%d\n", 0); \
			return CUPS_BACKEND_HOLD; \
		} \
		remain = be16_to_cpu(media->remain); \
		donor = be16_to_cpu(media->max); \
		donor = remain/donor; \
		if (donor != ctx->last_donor) { \
			ctx->last_donor = donor; \
			ATTR("marker-levels=%u\n", donor); \
		} \
		if (remain != ctx->last_remain) { \
			ctx->last_remain = remain; \
			ATTR("marker-message=\"%u prints remaining on '%s' ribbon\"\n", remain, mitsu9550_media_types(media->type, ctx->is_s)); \
		} \
		if (validate_media(ctx->type, media->type, ctx->cols, ctx->rows)) { \
			ERROR("Incorrect media (%u) type for printjob (%ux%u)!\n", media->type, ctx->cols, ctx->rows); \
			return CUPS_BACKEND_HOLD; \
		} \
		/* status2 */ \
		ret = mitsu9550_get_status(ctx, rdbuf, 0, 1, 0); \
		if (ret < 0) \
			return CUPS_BACKEND_FAILED; \
		/* status */ \
		ret = mitsu9550_get_status(ctx, rdbuf, 1, 0, 0); \
		if (ret < 0) \
			return CUPS_BACKEND_FAILED; \
		\
		/* Make sure we're idle */ \
		if (sts->sts5 != 0) {  /* Printer ready for another job */ \
			sleep(1); \
			goto top; \
		} \
		/* Check for known errors */ \
		if (sts->sts2 != 0) {  \
			ERROR("Printer cover open!\n");	\
			return CUPS_BACKEND_STOP; \
		} \
	} while (0);

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

	if (ctx->type == P_MITSU_9550S ||
	    ctx->type == P_MITSU_9800S)
		ctx->is_s = 1;

	ctx->last_donor = ctx->last_remain = 65535;
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
	uint8_t buf[sizeof(struct mitsu9550_hdr1)];
	int remain, i;
	uint32_t planelen = 0;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	ctx->hdr1_present = 0;
	ctx->hdr2_present = 0;
	ctx->hdr3_present = 0;
	ctx->hdr4_present = 0;

top:
	/* Read in initial header */
	remain = sizeof(buf);
	while (remain > 0) {
		i = read(data_fd, buf + sizeof(buf) - remain, remain);
		if (i == 0)
			return CUPS_BACKEND_CANCEL;
		if (i < 0)
			return CUPS_BACKEND_CANCEL;
		remain -= i;
	}

	/* Sanity check */
	if (buf[0] != 0x1b || buf[1] != 0x57 || buf[3] != 0x2e) {
		if (!ctx->hdr1_present || !ctx->hdr2_present) {
			ERROR("Unrecognized data format!\n");
			return CUPS_BACKEND_CANCEL;
		} else if (buf[0] == 0x1b && buf[1] == 0x5a &&
			   buf[2] == 0x54) {

			/* We're in the data portion now */
			if (buf[3] == 0x10)
				planelen *= 2;

			goto hdr_done;
		} else {
			ERROR("Unrecognized data block!\n");
			return CUPS_BACKEND_CANCEL;
		}
	}

	switch(buf[2]) {
	case 0x20: /* header 1 */
		memcpy(&ctx->hdr1, buf, sizeof(ctx->hdr1));
		ctx->hdr1_present = 1;

		/* Work out printjob size */
		ctx->rows = be16_to_cpu(ctx->hdr1.rows);
		ctx->cols = be16_to_cpu(ctx->hdr1.cols);
		planelen = ctx->rows * ctx->cols;

		break;
	case 0x21: /* header 2 */
		memcpy(&ctx->hdr2, buf, sizeof(ctx->hdr2));
		ctx->hdr2_present = 1;
		break;
	case 0x22: /* header 3 */
		memcpy(&ctx->hdr3, buf, sizeof(ctx->hdr3));
		ctx->hdr3_present = 1;
		break;
	case 0x26: /* header 4 */
		memcpy(&ctx->hdr4, buf, sizeof(ctx->hdr4));
		ctx->hdr4_present = 1;
		break;
	default:
		ERROR("Unrecognized header format (%02x)!\n", buf[2]);
		return CUPS_BACKEND_CANCEL;
	}

	/* Read in the next chunk */
	goto top;

hdr_done:

	/* We have three planes and the final terminator to read */
	remain = 3 * (planelen + sizeof(struct mitsu9550_plane)) + sizeof(struct mitsu9550_cmd);

	/* Mitsu9600 windows spool uses more, smaller blocks, but plane data is the same */
	if (ctx->type == P_MITSU_9600) {
		remain += 128 * sizeof(struct mitsu9550_plane); /* 39 extra seen on 4x6" */
	}

	/* Don't forget the matte plane! */
	if (ctx->hdr1.matte) {
		remain += planelen + sizeof(struct mitsu9550_plane) + sizeof(struct mitsu9550_cmd);
	}

	/* 9550S/9800S doesn't typically sent over hdr4! */
	if (ctx->type == P_MITSU_9550S ||
	    ctx->type == P_MITSU_9800S) {
		/* XXX Has to do with error policy, but not sure what.
		   Mitsu9550-S/9800-S will set this based on a command,
		   but it's not part of the standard job spool */
		ctx->hdr4_present = 0;
	}

	/* Allocate buffer for the payload */
	ctx->datalen = 0;
	ctx->databuf = malloc(remain);
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_FAILED;
	}

	/* Load up the data blocks.*/
	while(1) {
		/* Note that 'buf' needs to be already filled here! */
		struct mitsu9550_plane *plane = (struct mitsu9550_plane *)buf;

		/* Sanity check header... */
		if (plane->cmd[0] != 0x1b ||
		    plane->cmd[1] != 0x5a ||
		    plane->cmd[2] != 0x54) {
			ERROR("Unexpected data read, aborting job\n");
			return CUPS_BACKEND_CANCEL;
		}

		/* Work out the length of this block */
		planelen = be16_to_cpu(plane->rows) * be16_to_cpu(plane->cols);
		if (plane->cmd[3] == 0x10)
			planelen *= 2;

		/* Copy plane header into buffer */
		memcpy(ctx->databuf + ctx->datalen, buf, sizeof(buf));
		ctx->datalen += sizeof(buf);
		planelen -= sizeof(buf) - sizeof(struct mitsu9550_plane);

		/* Read in the spool data */
		while(planelen > 0) {
			i = read(data_fd, ctx->databuf + ctx->datalen, planelen);
			if (i == 0)
				return CUPS_BACKEND_CANCEL;
			if (i < 0)
				return CUPS_BACKEND_CANCEL;
			ctx->datalen += i;
			planelen -= i;
		}

		/* Try to read in the next chunk.  It will be one of:
		    - Additional block header (12B)
		    - Job footer (4B)
		*/
		i = read(data_fd, buf, 4);
		if (i == 0)
			return CUPS_BACKEND_CANCEL;
		if (i < 0)
			return CUPS_BACKEND_CANCEL;

		/* Is this a "job end" marker? */
		if (plane->cmd[0] != 0x1b ||
		    plane->cmd[1] != 0x5a ||
		    plane->cmd[2] != 0x54) {
			/* store it in the buffer */
			memcpy(ctx->databuf + ctx->datalen, buf, 4);
			ctx->datalen += 4;

			/* Unless we have a matte plane following, we're done */
			if (!ctx->hdr1.matte)
				break;
			planelen = sizeof(buf);
		} else {
			/* It's part of a block header, mark what we've read */
			planelen = sizeof(buf) - 4;
		}

		/* Read in the rest of the header */
		while (planelen > 0) {
			i = read(data_fd, buf + sizeof(buf) - planelen, planelen);
			if (i == 0)
				return CUPS_BACKEND_CANCEL;
			if (i < 0)
				return CUPS_BACKEND_CANCEL;
			planelen -= i;
		}
	}

	/* Disable matte if the printer doesn't support it */
	if (ctx->hdr1.matte && ctx->type != P_MITSU_9810) {
		WARNING("Matte not supported on this printer, disabling\n");
		ctx->hdr1.matte = 0;
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

static char *mitsu9550_media_types(uint8_t type, uint8_t is_s)
{
	if (is_s) {
		switch (type & 0xf) { /* values can be 0x0? or 0x4? */
		case 0x02:
			return "CK9015 (4x6)";
		case 0x04:
			return "CK9318 (5x7)";
		case 0x05:
			return "CK9523 (6x9)";
		default:
			return "Unknown";
		}
		return NULL;
	}
	
	switch (type & 0xf) { /* values can be 0x0? or 0x4? */
	case 0x01:
		return "CK9035 (3.5x5)";
	case 0x02:
		return "CK9046 (4x6)";
	case 0x03:
		return "CK9046PST (4x6)";
	case 0x04:
		return "CK9057 (5x7)";
	case 0x05:
		return "CK9069 (6x9)";
	case 0x06:
		return "CK9068 (6x8)";
	default:
		return "Unknown";
	}
	return NULL;
}

static int validate_media(int type, int media, int cols, int rows)
{
	switch(type) {
	case P_MITSU_9550:
		switch(media & 0xf) {
		case 0x01: /* 3.5x5 */
			if (cols != 1812 && rows != 1240)
				return 1;
			break;
		case 0x02: /* 4x6 */
		case 0x03: /* 4x6 postcard */ 
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 1184 && rows != 1240)
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
			if (rows != 1416 && rows != 2792 &&
			    rows != 2956 && rows != 3146)
				return 1;
			break;
		case 0x06: /* V (6x8??) */
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 2792)
				return 1;
			break;
		default: /* Unknown */
			WARNING("Unknown media type %02x\n", media);
			break;
		}
		break;
	case P_MITSU_9550S:
		switch(media & 0xf) {
		case 0x02: /* 4x6 */
		case 0x03: /* 4x6 postcard */
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 1184 && rows != 1240)
				return 1;
			break;
		case 0x04: /* 5x7 */
			if (cols != 1812 && rows != 2452)
				return 1;
			break;
		case 0x05: /* 6x9 */
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 2792 &&
			    rows != 2956 && rows != 3146)
				return 1;
			break;
		case 0x06: /* V (6x8??) */
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 2792)
				return 1;
			break;
		default: /* Unknown */
			WARNING("Unknown media type %02x\n", media);
			break;
		}
		break;
	case P_MITSU_9600: // XXX 9600S doesn't support 5" media at all!
		switch(media & 0xf) {
		case 0x01: /* 3.5x5 */
			if (cols == 1572) {
				if (rows == 1076)
					break;
			} else if (cols == 3144) {
				if (rows == 2152)
					break;
			}
			return 1;
		case 0x02: /* 4x6 */
		case 0x03: /* 4x6 postcard */
			if (cols == 1868) {
				if (rows == 1228)
					break;
			} else if (cols == 3736) {
				if (rows == 2458)
					break;
			}
			return 1;
		case 0x04: /* 5x7 */
			if (cols == 1572) {
				if (rows == 1076 || rows == 2128)
					break;
			} else if (cols == 3144) {
				if (rows == 2152 || rows == 4256)
					break;
			}
			return 1;
		case 0x05: /* 6x9 */
			if (cols == 1868) {
				if (rows == 1228 || rows == 2442 || rows == 2564 || rows == 2730)
					break;
			} else if (cols == 3736) {
				if (rows == 2458 || rows == 4846 || rows == 5130 || rows == 5462)
					break;
			}
			return 1;
		case 0x06: /* V (6x8??) */
			if (cols == 1868) {
				if (rows == 1228 || rows == 2442)
					break;
			} else if (cols == 3736) {
				if (rows == 2458 || rows == 4846)
					break;
			}
			return 1;
		default: /* Unknown */
			WARNING("Unknown media type %02x\n", media);
			break;
		}
		break;
	case P_MITSU_9800:
	case P_MITSU_9810: // XXX and don't forget the 9820S
		switch(media & 0xf) {
		case 0x01: /* 3.5x5 */
			if (cols != 1572 && rows != 1076)
				return 1;
			break;
		case 0x02: /* 4x6 */
		case 0x03: /* 4x6 postcard */
			if (cols != 1868 && rows != 1228)
				return 1;
			break;
		case 0x04: /* 5x7 */
			if (cols != 1572 && rows != 2128)
				return 1;
			break;
		case 0x05: /* 6x9 */
			if (cols != 1868)
				return 1;
			if (rows != 1228 && rows != 2442 &&
			    rows != 2564 && rows != 2730)
				return 1;
			break;
		case 0x06: /* V (6x8??) */
			if (cols != 1868)
				return 1;
			if (rows != 1228 && rows != 2442)
				return 1;
			break;
		default: /* Unknown */
			WARNING("Unknown media type %02x\n", media);
			break;
		}
		break;
	case P_MITSU_9800S:
		switch(media & 0xf) {
		case 0x02: /* 4x6 */
		case 0x03: /* 4x6 postcard */ 
			if (cols != 1868 && rows != 1228)
				return 1;
			break;
		case 0x04: /* 5x7 */
			if (cols != 1572 && rows != 2128)
				return 1;
			break;
		case 0x05: /* 6x9 */
			if (cols != 1868)
				return 1;
			if (rows != 1228 && rows != 2442 &&
			    rows != 2564 && rows != 2730)
				return 1;
			break;
		case 0x06: /* V (6x8??) */
			if (cols != 1868)
				return 1;
			if (rows != 1228 && rows != 2442)
				return 1;
			break;
		default: /* Unknown */
			WARNING("Unknown media type %02x\n", media);
			break;
		}
		break;
	default:
		WARNING("Unknown printer type %d\n", type);
		break;
	}
	return 0;
}

static int mitsu9550_main_loop(void *vctx, int copies) {
	struct mitsu9550_ctx *ctx = vctx;
	struct mitsu9550_cmd cmd;
	uint8_t rdbuf[READBACK_LEN];
	uint8_t *ptr;

	int ret;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Update printjob header to reflect number of requested copies */
	ctx->hdr2.copies = cpu_to_be16(copies);

	/* Okay, let's do this thing */
	ptr = ctx->databuf;

top:
	if (ctx->is_s) {
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

	if (ctx->type == P_MITSU_9800S) {
		int num;

		/* Send "unknown 3" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x4b;
		cmd.cmd[2] = 0x01;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;

		ret = read_data(ctx->dev, ctx->endp_up,
				rdbuf, READBACK_LEN, &num);
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		// seen so far: e4 4b 01 00 02 00 78
	}

	QUERY_STATUS();

	/* Now it's time for the actual print job! */

	if (ctx->is_s) {
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x44;
		cmd.cmd[2] = 0;
		cmd.cmd[3] = 0;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, 4)))
			return CUPS_BACKEND_FAILED;
	}

	QUERY_STATUS();

	/* Send printjob headers from spool data */
	if (ctx->hdr1_present)
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &ctx->hdr1, sizeof(ctx->hdr1))))
			return CUPS_BACKEND_FAILED;
	if (ctx->hdr2_present)
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &ctx->hdr2, sizeof(ctx->hdr2))))
			return CUPS_BACKEND_FAILED;
	if (ctx->hdr3_present)
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &ctx->hdr3, sizeof(ctx->hdr3))))
			return CUPS_BACKEND_FAILED;
	if (ctx->hdr4_present)
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &ctx->hdr4, sizeof(struct mitsu9550_hdr4))))
			return CUPS_BACKEND_FAILED;		
	
	if (ctx->is_s) {
		/* Send "start data" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x5a;
		cmd.cmd[2] = 0x43;
		cmd.cmd[3] = 0x00;
		
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	}
	
	/* Send over plane data */
	while(1) {
		struct mitsu9550_plane *plane = (struct mitsu9550_plane *)ptr;
		uint32_t planelen = be16_to_cpu(plane->rows) * be16_to_cpu(plane->cols);
		if (plane->cmd[0] != 0x1b ||
		    plane->cmd[1] != 0x5a ||
		    plane->cmd[2] != 0x54)
			break;
		if (plane->cmd[3] == 0x10)
			planelen *= 2;

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) ptr, sizeof(struct mitsu9550_plane))))
			return CUPS_BACKEND_FAILED;
		ptr += sizeof(struct mitsu9550_plane);
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) ptr, planelen)))
			return CUPS_BACKEND_FAILED;
		ptr += planelen;
	}

	/* Query statuses */
	{
		struct mitsu9550_status *sts = (struct mitsu9550_status*) rdbuf;
//		struct mitsu9550_status2 *sts2 = (struct mitsu9550_status2*) rdbuf;
		struct mitsu9550_media *media = (struct mitsu9550_media *) rdbuf;
		uint16_t donor, remain;
		
		ret = mitsu9550_get_status(ctx, rdbuf, 0, 0, 1); // media
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Sanity-check media response */
		if (media->remain == 0 || media->max == 0) {
			ERROR("Printer out of media!\n");
			ATTR("marker-levels=%d\n", 0);
			return CUPS_BACKEND_HOLD;
		}
		remain = be16_to_cpu(media->remain);
		donor = be16_to_cpu(media->max);
		donor = remain/donor;
		if (donor != ctx->last_donor) {
			ctx->last_donor = donor;
			ATTR("marker-levels=%u\n", donor);
		}
		if (remain != ctx->last_remain) {
			ctx->last_remain = remain;
			ATTR("marker-message=\"%u prints remaining on '%s' ribbon\"\n", remain, mitsu9550_media_types(media->type, ctx->is_s));
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
		/* Check for known errors */
		if (sts->sts2 != 0) {
			ERROR("Printer cover open!\n");
			return CUPS_BACKEND_STOP;
		}
	}

	/* Send "end data" command */
	if (ctx->type == P_MITSU_9550S) {
		/* Override spool, which may be wrong */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x50;
		cmd.cmd[2] = 0x47;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	} else if (ctx->type == P_MITSU_9800S) {
		/* Override spool, which may be wrong */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x50;
		cmd.cmd[2] = 0x4e;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	} else {
		/* Send from spool file */
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ptr, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
		ptr += sizeof(cmd);
	}

	/* Don't forget the 9810's matte plane */
	if (ctx->hdr1.matte) {
		struct mitsu9550_plane *plane = (struct mitsu9550_plane *)ptr;
		uint32_t planelen = be16_to_cpu(plane->rows) * be16_to_cpu(plane->cols);

		if (plane->cmd[3] == 0x10)
			planelen *= 2;

		// XXX include a status loop here too?
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) ptr, sizeof(struct mitsu9550_plane))))
			return CUPS_BACKEND_FAILED;
		ptr += sizeof(struct mitsu9550_plane);
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) ptr, planelen)))
			return CUPS_BACKEND_FAILED;
		ptr += planelen;

		/* Send "lamination end data" command from spool file */
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
		uint16_t donor, remain;

		ret = mitsu9550_get_status(ctx, rdbuf, 0, 0, 1); // media
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Sanity-check media response */
		if (media->remain == 0 || media->max == 0) {
			ERROR("Printer out of media!\n");
			ATTR("marker-levels=%d\n", 0);
			return CUPS_BACKEND_HOLD;
		}
		remain = be16_to_cpu(media->remain);
		donor = be16_to_cpu(media->max);
		donor = remain/donor;
		if (donor != ctx->last_donor) {
			ctx->last_donor = donor;
			ATTR("marker-levels=%u\n", donor);
		}
		if (remain != ctx->last_remain) {
			ctx->last_remain = remain;
			ATTR("marker-message=\"%u prints remaining on '%s' ribbon\"\n", remain, mitsu9550_media_types(media->type, ctx->is_s));
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
		/* Check for known errors */
		if (sts->sts2 != 0) {
			ERROR("Printer cover open!\n");
			return CUPS_BACKEND_STOP;
		}
		sleep(1);
	}

	INFO("Print complete\n");

	return CUPS_BACKEND_OK;
}

static void mitsu9550_dump_media(struct mitsu9550_media *resp, int is_s)
{
	INFO("Media type       : %02x (%s)\n",
	     resp->type, mitsu9550_media_types(resp->type, is_s));
	INFO("Media remaining  : %03d/%03d\n",
	     be16_to_cpu(resp->remain), be16_to_cpu(resp->max));
}

static void mitsu9550_dump_status(struct mitsu9550_status *resp)
{
	INFO("Printer status    : %02x (%s)\n",
	     resp->sts1, resp->sts1 ? "Printing": "Idle");
	INFO("Pages remaining   : %03d\n",
	     be16_to_cpu(resp->copies));
	INFO("Other status      : %02x %02x %02x %02x  %02x %02x\n",
	     resp->sts2, resp->sts3, resp->sts4,
	     resp->sts5, resp->sts6, resp->sts7);
}

static void mitsu9550_dump_status2(struct mitsu9550_status2 *resp)
{
	INFO("Prints remaining on media : %03d\n",
	     be16_to_cpu(resp->remain));
}

static int mitsu9550_query_media(struct mitsu9550_ctx *ctx)
{
	struct mitsu9550_media resp;
	int ret;

	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, 0, 0, 1);

	if (!ret)
		mitsu9550_dump_media(&resp, ctx->is_s);

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

static int mitsu9550_query_status2(struct mitsu9550_ctx *ctx)
{
	struct mitsu9550_status2 resp;
	int ret;
	
	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, 0, 1, 0);

	if (!ret)
		mitsu9550_dump_status2(&resp);

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
		WARNING("Short serno read! (%d vs %u)\r\n",
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

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "ms")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'm':
			j = mitsu9550_query_media(ctx);
			break;
		case 's':
			j = mitsu9550_query_status(ctx);
			if (!j)
				j = mitsu9550_query_status2(ctx);
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
	.name = "Mitsubishi CP-9550 family",
	.version = "0.29",
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
	{ USB_VID_MITSU, USB_PID_MITSU_9000AM, P_MITSU_9550, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_9000D, P_MITSU_9550, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_9500D, P_MITSU_9550, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_9550D, P_MITSU_9550, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_9550DS, P_MITSU_9550S, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_9600D, P_MITSU_9600, ""},
//	{ USB_VID_MITSU, USB_PID_MITSU_9600D, P_MITSU_9600S, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_9800D, P_MITSU_9800, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_9800DS, P_MITSU_9800S, ""},
	{ USB_VID_MITSU, USB_PID_MITSU_98__D, P_MITSU_9810, ""},
//	{ USB_VID_MITSU, USB_PID_MITSU_9810D, P_MITSU_9810, ""},
//	{ USB_VID_MITSU, USB_PID_MITSU_9820DS, P_MITSU_9820S, ""},
	{ 0, 0, 0, ""}
	}
};

/* Mitsubish CP-9550/9600/9800/9810 spool format:

   Spool file consists of 3 (or 4) 50-byte headers, followed by three
   image planes, each with a 12-byte header, then a 4-byte footer.

   All multi-byte numbers are big endian.

   ~~~ Header 1

   1b 57 20 2e 00 QQ QQ 00  00 00 00 00 00 00 XX XX :: XX XX == columns
   YY YY 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: YY YY == rows
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: QQ == 0x0a90 on 9810, 0x0a10 on all others.
   00 00 

   ~~~ Header 2

   1b 57 21 2e 00 80 00 22  QQ QQ 00 00 00 00 00 00 :: ZZ ZZ = num copies (>= 0x01)
   00 00 00 00 00 00 00 00  00 00 00 00 ZZ ZZ 00 00 :: YY = 00/80 Fine/SuperFine (9550), 10/80 Fine/Superfine (98x0), 00 (9600)
   XX 00 00 00 00 00 YY 00  00 00 00 00 00 00 00 00 :: XX = 00 normal, 83 Cut 2x6 (9550 only!)
   00 01                                            :: QQ QQ = 0x0803 on 9550, 0x0801 on 98x0, 0x0003 on 9600

   ~~~ Header 3 (9550 and 9800-S only..)

   1b 57 22 2e 00 40 00 00  00 00 00 XX 00 00 00 00 :: XX = 00 normal, 01 FineDeep
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 
   00 00 

   ~~~ Header 4 (all but 9550-S and 9800-S, involves error policy?)

   1b 57 26 2e 00 QQ 00 00  00 00 00 SS RR 01 00 00 :: QQ = 0x70 on 9550/98x0, 0x60 on 9600 or 9800S
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: RR = 0x01 on 9550/98x0, 0x00 on 9600
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: SS = 0x01 on 9800S
   00 00  

  ~~~~ Data follows:

   Format is:  planar YMC16 for 98x0 (only 12 bits used)
               planar BGR for 9550DW
               planar RGB for 9550DW-S and 9600DW

   1b 5a 54 ?? RR RR 00 00  07 14 04 d8  :: 0714 == columns, 04d8 == rows
                                         :: RRRR == row offset for data
		                         :: ?? == 0x00 for 8bpp, 0x10 for 16/12bpp.

   Data follows immediately, no padding.

   1b 5a 54 00 00 00 00 00  07 14 04 d8  :: Another plane.

   Data follows immediately, no padding.

   1b 5a 54 00 00 00 00 00  07 14 04 d8  :: Another plane.

   Data follows immediately, no padding.

  ~~~~ Footer:

   1b 50 46 00  (9550)
   1b 50 47 00  (9550-S)
   1b 50 48 00  (9600)
   1b 50 4c 00  (98x0)
   1b 50 4e 00  (9800-S)

  ~~~~ Lamination data follows (on 9810 only, if matte selected)

   1b 5a 54 10 00 00  00 00 06 24 04 34

   Data follows immediately, no padding.

   1b 50 56 00  (Lamination footer)

  ~~~~ QUESTIONS:

   * Lamination control?
   * Other 9550 multi-cut modes (on 6x9 media: 4x6*2, 4.4x6*2, 3x6*3, 2x6*4)
   * 9600/98x0 multi-cut modes?

 ***********************************************************************

 * Mitsubishi ** CP-9550DW-S/9800DW-S ** Communications Protocol:

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

  [[ unknown, 9800-only ]]

 -> 1b 4b 01 00
 <- e4 4b 01 00 02 00 78

  Status Query
 
 -> 1b 56 30 00
 -> 30 2e 00 00 00 00 MM 00  NN NN ZZ 00 00 00 00 00 :: MM, NN, ZZ
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

 -> 1b 50 47 00  [9550S]
 -> 1b 50 4e 00  [9800S]

  [[ At this point, loop status/status b/media queries until printer idle ]]

    MM, QQ RR SS, TT UU

 <- 00  3e 00 00  8a 44  :: Idle.
    00  7e 00 00  8a 44  :: Plane data submitted, pre "end data" cmd
    00  7e 40 01  8a 44  :: "end data" sent
    30  7e 40 01  8a 44
    38  7e 40 01  8a 44
    59  7e 40 01  8a 44
    59  7e 40 00  8a 44
    4d  7e 40 00  8a 44
     [...]
    43  7e 40 00  82 44
     [...]
    50  7e 40 00  80 44
     [...]
    31  7e 40 00  7d 44
     [...]
    00  3e 00 00  80 44  :: Idle.

  Also seen: 

    00  3e 00 00  96 4b  :: Idle
    00  be 00 00  96 4b  :: Data submitted, pre "start"
    00  be 80 01  96 4b  :: print start sent
    30  be 80 01  96 4c
     [...]
    30  be 80 01  89 4b
    38  be 80 01  8a 4b
    59  be 80 01  8b 4b
     [...]
    4d  be 80 01  89 4b
     [...]
    43  be 80 01  89 4b
     [...]
    50  be 80 01  82 4b
     [...]
    31  be 80 01  80 4b
     [...]

  Seen on 9600DW

    ZZ == 08  Door open

 Working theory of interpreting the status flags:

  MM :: 00 is idle, else mechanical printer state.
  NN :: Remaining prints in job, or 0x00 0x00 when idle.
  QQ :: ?? 0x3e + 0x40 or 0x80 (see below)
  RR :: ?? 0x00 is idle, 0x40 or 0x80 is "printing"?
  SS :: ?? 0x00 means "ready for another print" but 0x01 is "busy"
  TT :: ?? seen values between 0x7c through 0x96)
  UU :: ?? seen values between 0x43 and 0x4c -- temperature?
 
  *** 

   Other printer commands seen:

  [[ Set error policy ?? aka "header 4" ]]

 -> 1b 57 26 2e 00 QQ 00 00  00 00 00 00 RR SS 00 00 :: QQ/RR/SS 00 00 00 [9550S]
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ::          20 01 00 [9550S w/ ignore failures on]
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ::          70 01 01 [9550]
    00 00

 */
