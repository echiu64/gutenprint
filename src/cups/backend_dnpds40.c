/*
 *   DNP DS40/DS80 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2015 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Development of this backend was sponsored by:
 *
 *     Marco Di Antonio and [ ilgruppodigitale.com ]
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

//#define MATTE_STATE
//#define DNP_ONLY
#define MATTE_GLOSSY_2BUF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define BACKEND dnpds40_backend

#include "backend_common.h"

#define USB_VID_CITIZEN   0x1343
#define USB_PID_DNP_DS40  0x0003 // Also Citizen CX
#define USB_PID_DNP_DS80  0x0004 // Also Citizen CX-W, and Mitsubishi CP-3800DW
#define USB_PID_DNP_DSRX1 0x0005 // Also Citizen CY

#define USB_VID_DNP       0x1452
#define USB_PID_DNP_DS620 0x8b01

//#define USB_PID_DNP_DS80D XXXX

//#define USB_PID_CITIZEN_CW-02 XXXXX
//#define USB_PID_CITIZEN_OP900II XXXXX

/* Private data stucture */
struct dnpds40_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;

	int type;

	char *serno;
	char *version;

	int buf_needed;
	int last_matte;

	int ver_major;
	int ver_minor;
	int media;

	uint32_t multicut;
	int matte;
	int cutter;
	int can_rewind;

	int manual_copies;
	int supports_6x9;
	int supports_2x6;
	int supports_3x5x2;
	int supports_matte;
	int supports_fullcut;
	int supports_rewind;
	int supports_standby;
	int supports_6x4_5;
	int supports_mqty_default;
	int supports_iserial;

	uint8_t *qty_offset;
	uint8_t *buffctrl_offset;
	uint8_t *multicut_offset;

	uint8_t *databuf;
	int datalen;
};

struct dnpds40_cmd {
	uint8_t esc; /* Fixed at ascii ESC, aka 0x1B */
	uint8_t p;   /* Fixed at ascii 'P' aka 0x50 */
	uint8_t arg1[6];
	uint8_t arg2[16];
	uint8_t arg3[8]; /* Decimal value of arg4's length, or empty */
	uint8_t arg4[0]; /* Extra payload if arg3 is non-empty
			    Doesn't have to be sent in the same URB */

	/* All unused elements are set to 0x20 (ie ascii space) */
};

#define min(__x, __y) ((__x) < (__y)) ? __x : __y

static void dnpds40_build_cmd(struct dnpds40_cmd *cmd, char *arg1, char *arg2, uint32_t arg3_len)
{
	memset(cmd, 0x20, sizeof(*cmd));
	cmd->esc = 0x1b;
	cmd->p = 0x50;
	memcpy(cmd->arg1, arg1, min(strlen(arg1), sizeof(cmd->arg1)));
	memcpy(cmd->arg2, arg2, min(strlen(arg2), sizeof(cmd->arg2)));
	if (arg3_len) {
		char buf[9];
		snprintf(buf, sizeof(buf), "%08u", arg3_len);
		memcpy(cmd->arg3, buf, 8);
	}

}

static void dnpds40_cleanup_string(char *start, int len)
{
	char *ptr = strchr(start, 0x0d);

	if (ptr && (ptr - start < len)) {
		*ptr = 0x00; /* If there is a <CR>, terminate there */
		len = ptr - start;
	} else {
		start[--len] = 0x00;  /* force null-termination */
	}

	/* Trim trailing spaces */
	while (len && start[len-1] == ' ') {
		start[--len] = 0;
	}
}

static char *dnpds40_media_types(int media)
{
	switch (media) {
	case 200: return "5x3.5 (L)";
	case 210: return "5x7 (2L)";
	case 300: return "6x4 (PC)";
	case 310: return "6x8 (A5)";
	case 400: return "6x9 (A5W)";
	case 500: return "8x10";
	case 510: return "8x12";
	default:
		break;
	}

	return "Unknown type";
}

static char *dnpds40_statuses(int status)
{
	switch (status) {
	case 0:	return "Idle";
	case 1:	return "Printing";
	case 500: return "Cooling Print Head";
	case 510: return "Cooling Paper Motor";
	case 900: return "Standby Mode";
	case 1000: return "Cover Open";
	case 1010: return "No Scrap Box";
	case 1100: return "Paper End";
	case 1200: return "Ribbon End";
	case 1300: return "Paper Jam";
	case 1400: return "Ribbon Error";
	case 1500: return "Paper Definition Error";
	case 1600: return "Data Error";
	case 2000: return "Head Voltage Error";
	case 2100: return "Head Position Error";
	case 2200: return "Power Supply Fan Error";
	case 2300: return "Cutter Error";
	case 2400: return "Pinch Roller Error";
	case 2500: return "Abnormal Head Temperature";
	case 2600: return "Abnormal Media Temperature";
	case 2610: return "Abnormal Paper Motor Temperature";
	case 2700: return "Ribbon Tension Error";
	case 2800: return "RF-ID Module Error";
	case 3000: return "System Error";
	default:
		break;
	}

	return "Unknown Error";
}

static int dnpds40_do_cmd(struct dnpds40_ctx *ctx,
			  struct dnpds40_cmd *cmd,
			  uint8_t *data, int len)
{
	int ret;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*)cmd, sizeof(*cmd))))
		return ret;

	if (data && len) 
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     data, len)))
			return ret;

	return CUPS_BACKEND_OK;
}

static uint8_t * dnpds40_resp_cmd(struct dnpds40_ctx *ctx,
				  struct dnpds40_cmd *cmd,
				  int *len)
{
	char tmp[9];
	uint8_t *respbuf;

	int ret, i, num = 0;

	memset(tmp, 0, sizeof(tmp));

	if ((ret = dnpds40_do_cmd(ctx, cmd, NULL, 0)))
		return NULL;

	/* Read in the response header */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*)tmp, 8, &num);
	if (ret < 0)
		return NULL;

	if (num != 8) {
		ERROR("Short read! (%d/%d)\n", num, 8);
		return NULL;
	}

	i = atoi(tmp);  /* Length of payload in bytes, possibly padded */
	respbuf = malloc(i);
	if (!respbuf) {
		ERROR("Memory allocation failure (%d bytes)!\n", i);
		return NULL;
	}

	/* Read in the actual response */
	ret = read_data(ctx->dev, ctx->endp_up,
			respbuf, i, &num);
	if (ret < 0) {
		free(respbuf);
		return NULL;
	}

	if (num != i) {
		ERROR("Short read! (%d/%d)\n", num, i);
		free(respbuf);
		return NULL;
	}

	*len = num;
	return respbuf;
}

static int dnpds40_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;

	struct dnpds40_ctx ctx = {
		.dev = dev,
		.endp_up = endp_up,
		.endp_down = endp_down,
	};

	/* Get Serial Number */
	dnpds40_build_cmd(&cmd, "INFO", "SERIAL_NUMBER", 0);

	resp = dnpds40_resp_cmd(&ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	strncpy(buf, (char*)resp, buf_len);
	buf[buf_len-1] = 0;

	free(resp);

	return CUPS_BACKEND_OK;
}

static void *dnpds40_init(void)
{
	struct dnpds40_ctx *ctx = malloc(sizeof(struct dnpds40_ctx));
	if (!ctx) {
		ERROR("Memory allocation failure (%d bytes)!\n", (int)sizeof(struct dnpds40_ctx));
		return NULL;
	}
	memset(ctx, 0, sizeof(struct dnpds40_ctx));

	ctx->type = P_ANY;
	ctx->last_matte = -1;

	return ctx;
}

#define FW_VER_CHECK(__major, __minor) \
	((ctx->ver_major > (__major)) || \
	 (ctx->ver_major == (__major) && ctx->ver_minor >= (__minor)))

static void dnpds40_attach(void *vctx, struct libusb_device_handle *dev,
			   uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct dnpds40_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);

	ctx->type = lookup_printer_type(&dnpds40_backend,
					desc.idVendor, desc.idProduct);

	{
		/* Get Firmware Version */
		struct dnpds40_cmd cmd;
		uint8_t *resp;
		int len = 0;

		dnpds40_build_cmd(&cmd, "INFO", "FVER", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (resp) {
			char *ptr;
			dnpds40_cleanup_string((char*)resp, len);
			ctx->version = strdup((char*) resp);

			/* Parse version */
			/* ptr = */ strtok((char*)resp, " .");
			ptr = strtok(NULL, ".");
			ctx->ver_major = atoi(ptr);
			ptr = strtok(NULL, ".");
			ctx->ver_minor = atoi(ptr);
			free(resp);
		}

		/* Get Serial Number */
		dnpds40_build_cmd(&cmd, "INFO", "SERIAL_NUMBER", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (resp) {
			dnpds40_cleanup_string((char*)resp, len);
			ctx->serno = (char*) resp;
			/* Do NOT free resp! */
		}

		/* Query Media Info */
		dnpds40_build_cmd(&cmd, "INFO", "MEDIA", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (resp) {
			char tmp[4];

			dnpds40_cleanup_string((char*)resp, len);

			memcpy(tmp, resp + 4, 3);
			tmp[3] = 0;

			ctx->media = atoi(tmp);

			/* Subtract out the "mark" type */
			if (ctx->media & 1)
				ctx->media--;

			free(resp);
		}
	}

#ifdef DNP_ONLY
	/* Only allow DNP printers to work. Rebadged versions should not. */

	{ /* Validate USB Vendor String is "Dai Nippon Printing" */
		char buf[256];
		buf[0] = 0;
		libusb_get_string_descriptor_ascii(dev, desc->iManufacturer, (unsigned char*)buf, STR_LEN_MAX);
		sanitize_string(buf);
		if (strncmp(buf, "Dai", 3))
			return 0;
	}
#endif

	/* Per-printer options */
	switch (ctx->type) {
	case P_DNP_DS40:
		ctx->supports_6x9 = 1;
		if (FW_VER_CHECK(1,30))
			ctx->supports_matte = 1;
		if (FW_VER_CHECK(1,40))
			ctx->supports_2x6 = 1;
		if (FW_VER_CHECK(1,50))
			ctx->supports_3x5x2 = 1;
		if (FW_VER_CHECK(1,51))
			ctx->supports_fullcut = 1;
		break;
	case P_DNP_DS80:
		if (FW_VER_CHECK(1,30))
			ctx->supports_matte = 1;
		break;
	case P_DNP_DSRX1:
		ctx->supports_matte = 1;
		ctx->supports_mqty_default = 1; // 1.10 does. Maybe older too?
		if (FW_VER_CHECK(1,10))
			ctx->supports_2x6 = 1;
		break;
	case P_DNP_DS620:
		ctx->supports_matte = 1;
		ctx->supports_2x6 = 1;
		ctx->supports_fullcut = 1;
		ctx->supports_mqty_default = 1;
		ctx->supports_rewind = 1;
		ctx->supports_standby = 1;
		ctx->supports_iserial = 1;
		if (FW_VER_CHECK(0,30))
			ctx->supports_3x5x2 = 1;
		if (FW_VER_CHECK(1,10))
			ctx->supports_6x9 = ctx->supports_6x4_5 = 1;
		break;
	default:
		ERROR("Unknown vid/pid %04x/%04x (%d)\n", desc.idVendor, desc.idProduct, ctx->type);
		return;
	}
}

static void dnpds40_teardown(void *vctx) {
	struct dnpds40_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	if (ctx->serno)
		free(ctx->serno);
	if (ctx->version)
		free(ctx->version);
	free(ctx);
}

#define MAX_PRINTJOB_LEN (((2560*7536+1024+54))*3+1024) /* Worst-case */

static int dnpds40_read_parse(void *vctx, int data_fd) {
	struct dnpds40_ctx *ctx = vctx;
	int run = 1;
	char buf[9] = { 0 };

	uint32_t matte, dpi, cutter;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	/* There's no way to figure out the total job length in advance, we
	   have to parse the stream until we get to the image plane data, 
	   and even then the stream can contain arbitrary commands later.

	   So instead, we allocate a buffer of the maximum possible length, 
	   then parse the incoming stream until we hit the START command at
	   the end of the job.
	*/

	ctx->datalen = 0;
	ctx->databuf = malloc(MAX_PRINTJOB_LEN);
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_CANCEL;
	}

	/* Clear everything out */
	matte = 0;
	dpi = 0;
	cutter = 0;
	ctx->manual_copies = 0;
	ctx->multicut = 0;
	ctx->buffctrl_offset = ctx->qty_offset = ctx->multicut_offset = 0;

	while (run) {
		int remain, i, j;
		/* Read in command header */
		i = read(data_fd, ctx->databuf + ctx->datalen, 
			 sizeof(struct dnpds40_cmd));
		if (i < 0)
			return i;
		if (i == 0)
			break;
		if (i < (int) sizeof(struct dnpds40_cmd))
			return CUPS_BACKEND_CANCEL;

		if (ctx->databuf[ctx->datalen + 0] != 0x1b ||
		    ctx->databuf[ctx->datalen + 1] != 0x50) {
			ERROR("Unrecognized header data format @%d!\n", ctx->datalen);
			return CUPS_BACKEND_CANCEL;
		}

		/* Parse out length of data chunk, if any */
		memcpy(buf, ctx->databuf + ctx->datalen + 24, 8);
		j = atoi(buf);

		/* Read in data chunk as quickly as possible */
		remain = j;
		while (remain > 0) {
			i = read(data_fd, ctx->databuf + ctx->datalen + sizeof(struct dnpds40_cmd), 
				 remain);
			if (i < 0) {
				ERROR("Data Read Error: %d (%d/%d @%d)\n", i, remain, j, ctx->datalen);
				return i;
			}
			if (i == 0)
				return 1;
			ctx->datalen += i;
			remain -= i;
		}
		ctx->datalen -= j; /* Back it off */

		/* Check for some offsets */
		if(!memcmp("CNTRL QTY", ctx->databuf + ctx->datalen+2, 9)) {
			ctx->qty_offset = ctx->databuf + ctx->datalen + 32;
		}
		if(!memcmp("CNTRL CUTTER", ctx->databuf + ctx->datalen+2, 12)) {
			memcpy(buf, ctx->databuf + ctx->datalen + 32, 8);
			cutter = atoi(buf);
		}
		if(!memcmp("CNTRL BUFFCNTRL", ctx->databuf + ctx->datalen+2, 15)) {
			/* If the printer doesn't support matte, it doesn't
			   support buffcntrl.  strip it from the stream */
			if (ctx->supports_matte) {
				ctx->buffctrl_offset = ctx->databuf + ctx->datalen + 32;
			} else {
				WARNING("Printer FW does not support BUFFCNTRL, please update\n");
				continue;
			}
		}
		if(!memcmp("CNTRL OVERCOAT", ctx->databuf + ctx->datalen+2, 14)) {
			/* If the printer doesn't support matte, it doesn't
			   support buffcntrl.  strip it from the stream */
			if (ctx->supports_matte) {
				memcpy(buf, ctx->databuf + ctx->datalen + 32, 8);
				matte = atoi(buf);
			} else {
				WARNING("Printer FW does not support matte prints, please update\n");
				continue;
			}
		}
		if(!memcmp("IMAGE MULTICUT", ctx->databuf + ctx->datalen+2, 14)) {
			ctx->multicut_offset = ctx->databuf + ctx->datalen + 32;
			memcpy(buf, ctx->databuf + ctx->datalen + 32, 8);
			ctx->multicut = atoi(buf);
		}
		if(!memcmp("CNTRL FULL_CUTTER_SET", ctx->databuf + ctx->datalen+2, 21)) {
			if (!ctx->supports_fullcut) {
				WARNING("Printer FW does not support cutter control, please update!\n");
				continue;
			}
		}
		if(!memcmp("IMAGE YPLANE", ctx->databuf + ctx->datalen + 2, 12)) {
			uint32_t y_ppm; /* Pixels Per Meter */

			/* Validate vertical resolution */
			memcpy(&y_ppm, ctx->databuf + ctx->datalen + 32 + 42, sizeof(y_ppm));
			y_ppm = le32_to_cpu(y_ppm);

			switch (y_ppm) {
			case 11808:
				dpi = 300;
				break;
			case 23615:
				dpi = 600;
				break;
			default:
				ERROR("Unrecognized printjob resolution (%d ppm)\n", y_ppm);
				return CUPS_BACKEND_CANCEL;
			}

			/* Validate horizontal size */
			memcpy(&y_ppm, ctx->databuf + ctx->datalen + 32 + 18, sizeof(y_ppm));
			y_ppm = le32_to_cpu(y_ppm);
			if (ctx->type == P_DNP_DS80) {
				if (y_ppm != 2560) {
					ERROR("Incorrect horizontal resolution (%d), aborting!\n", y_ppm);
					return CUPS_BACKEND_CANCEL;
				}
			} else {
				if (y_ppm != 1920) {
					ERROR("Incorrect horizontal resolution (%d), aborting!\n", y_ppm);
					return CUPS_BACKEND_CANCEL;
				}
			}
		}

		/* This is the last block.. */
	        if(!memcmp("CNTRL START", ctx->databuf + ctx->datalen + 2, 11))
			run = 0;

		/* Add in the size of this chunk */
		ctx->datalen += sizeof(struct dnpds40_cmd) + j;
	}

	if (!ctx->datalen)
		return CUPS_BACKEND_CANCEL;

	/* Figure out the number of buffers we need. Most only need one. */
	if (ctx->multicut) {
		ctx->buf_needed = 1;

		if (dpi == 600) {
			if (ctx->type == P_DNP_DS620) {
				if (ctx->multicut == 5 || // 6x9
				    ctx->multicut == 31)  // 6x4.5*2
					ctx->buf_needed = 2;
			} else if (ctx->type == P_DNP_DS80) { /* DS80/CX-W */
				if (matte && (ctx->multicut == 21 || // A4 length
					      ctx->multicut == 20 || // 8x4*3
					      ctx->multicut == 19 || // 8x8+8x4
					      ctx->multicut == 15 || // 8x6*2
					      ctx->multicut == 7)) // 8x12
					ctx->buf_needed = 2;
			} else { /* DS40/CX/RX1/CY/etc */
				if (ctx->multicut == 4 ||  // 6x8
				    ctx->multicut == 5 ||  // 6x9
				    ctx->multicut == 12)   // 6x4*2
					ctx->buf_needed = 2;
				else if (matte && ctx->multicut == 3) // 5x7
					ctx->buf_needed = 2;
			}
		}
	} else {
		WARNING("Missing or illegal MULTICUT command, can't validate print job against loaded media!\n");
		if (dpi == 300)
			ctx->buf_needed = 1;
		else
			ctx->buf_needed = 2;
	}

	ctx->matte = (int)matte;
	ctx->cutter = cutter;
	ctx->can_rewind = 0;

	DEBUG("dpi %u matte %u mcut %u cutter %d, bufs %d\n",
	      dpi, matte, ctx->multicut, cutter, ctx->buf_needed);

	/* Sanity-check printjob type vs loaded media */
	if (ctx->multicut) {
		switch(ctx->media) {
		case 200: //"5x3.5 (L)"
			if (ctx->multicut != 1) {
				ERROR("Incorrect media for job loaded (%d vs %d)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 210: //"5x7 (2L)"
			if (ctx->multicut != 1 && ctx->multicut != 3 &&
			    ctx->multicut != 22 && ctx->multicut != 29) {
				ERROR("Incorrect media for job loaded (%d vs %d)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			/* Only 3.5x5 on 7x5 media can be rewound */
			if (ctx->multicut == 1)
				ctx->can_rewind = 1;
			break;
		case 300: //"6x4 (PC)"
			if (ctx->multicut != 2) {
				ERROR("Incorrect media for job loaded (%d vs %d)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 310: //"6x8 (A5)"
			if (ctx->multicut != 2 && ctx->multicut != 4 &&
			    ctx->multicut != 12 &&
			    ctx->multicut != 27 && ctx->multicut != 30) {
				ERROR("Incorrect media for job loaded (%d vs %d)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			/* Only 6x4 on 6x8 media can be rewound */
			if (ctx->multicut == 2)
				ctx->can_rewind = 1;
			break;
		case 400: //"6x9 (A5W)"
			if (ctx->multicut != 2 && ctx->multicut != 4 &&
			    ctx->multicut != 5 &&  ctx->multicut != 12 &&
			    ctx->multicut != 27 &&
			    ctx->multicut != 30 && ctx->multicut != 31) {
				ERROR("Incorrect media for job loaded (%d vs %d)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			/* Only 6x4 or 6x4.5 on 6x9 media can be rewound */
			if (ctx->multicut == 2 || ctx->multicut == 30)
				ctx->can_rewind = 1;
			break;
		case 500: //"8x10"
			if (ctx->multicut < 6 || ctx->multicut == 7 ||
			    ctx->multicut == 15 || ctx->multicut >= 18 ) {
				ERROR("Incorrect media for job loaded (%d vs %d)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 510: //"8x12"
			if (ctx->multicut < 6 || ctx->multicut > 21) {
				ERROR("Incorrect media for job loaded (%d vs %d)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		default:
			ERROR("Unknown media (%d vs %d)!\n", ctx->media, ctx->multicut);
			return CUPS_BACKEND_CANCEL;
		}
	}

	/* Additional santity checks */
	if ((ctx->multicut == 27 || ctx->multicut == 29) &&
	    ctx->type != P_DNP_DS620) {
		ERROR("Printer does not support 6x6 or 5x5 prints, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if ((ctx->multicut == 30 || ctx->multicut == 31) &&
	    !ctx->supports_6x4_5) {
		ERROR("Printer does not support 6x4.5 prints, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if (ctx->multicut == 5 && !ctx->supports_6x9) {
		ERROR("Printer does not support 6x9 prints, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if (ctx->multicut == 22 && !ctx->supports_3x5x2) {
		ERROR("Printer does not support 3.5x5*2 prints, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if (ctx->cutter == 120) {
		if (ctx->multicut == 2 || ctx->multicut == 4) {
			if (!ctx->supports_2x6) {
				ERROR("Printer does not support 2x6 prints, aborting!\n");
				return CUPS_BACKEND_CANCEL;
			}
		} else {
			ERROR("Printer only supports 2-inch cuts on 4x6 or 8x6 jobs!");
			return CUPS_BACKEND_CANCEL;
		}

		/* Work around firmware bug on DS40 where if we run out
		   of media, we can't resume the job without losing the
		   cutter setting. XXX add version test? */
		ctx->manual_copies = 1;
	}

	if (ctx->matte && !ctx->supports_matte) {
		ERROR("Printer FW does not support matte operation, please update!\n");
		return CUPS_BACKEND_CANCEL;
	}

	return CUPS_BACKEND_OK;
}

static int dnpds40_main_loop(void *vctx, int copies) {
	struct dnpds40_ctx *ctx = vctx;
	int ret;
	struct dnpds40_cmd cmd;
	uint8_t *resp = NULL;
	int len = 0;
	uint8_t *ptr;
	char buf[9];
	int status;
	int buf_needed;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Update quantity offset with count */
	// XXX this breaks if ctx->manual_copies is set, but the job
	// has a CNTRL QTY != 1
	if (!ctx->manual_copies && copies > 1) {
		snprintf(buf, sizeof(buf), "%07d\r", copies);
		if (ctx->qty_offset) {
			memcpy(ctx->qty_offset, buf, 8);
		} else {
			dnpds40_build_cmd(&cmd, "CNTRL", "QTY", 8);
			if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
				return CUPS_BACKEND_FAILED;
		}

		copies = 1;
	}

	/* Enable job resumption on correctable errors */
	if (ctx->supports_matte) {
		snprintf(buf, sizeof(buf), "%08d", 1);
		if (ctx->buffctrl_offset) {
			memcpy(ctx->buffctrl_offset, buf, 8);
		} else {
			dnpds40_build_cmd(&cmd, "CNTRL", "BUFFCNTRL", 8);
			if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
				return CUPS_BACKEND_FAILED;
		}
	}

#ifdef MATTE_STATE
	/* Check our current job's lamination vs previous job. */
	{
		/* Load last matte status from file */
		char buf[64];
		FILE *f;
		snprintf(buf, sizeof(buf), "/tmp/%s-last", ctx->serno);
		f = fopen(buf, "r");
		if (f) {
			fscanf(f, "%d", &ctx->last_matte);
			fclose(f);
		}
	}
#endif

	buf_needed = ctx->buf_needed;

#ifdef MATTE_GLOSSY_2BUF
	if (ctx->matte != ctx->last_matte)
		buf_needed = 2; /* Switching needs both buffers */
#endif

	ctx->last_matte = ctx->matte;
#ifdef MATTE_STATE
	{
		/* Store last matte status into file */
		char buf[64];
		FILE *f;
		snprintf(buf, sizeof(buf), "/tmp/%s-last", ctx->serno);
		f = fopen(buf, "w");
		if (f) {
			fprintf(f, "%08d", ctx->last_matte);
			fclose(f);
		}
	}
#endif

top:

	/* Query status */
	dnpds40_build_cmd(&cmd, "STATUS", "", 0);
	if (resp) free(resp);
	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;
	dnpds40_cleanup_string((char*)resp, len);
	status = atoi((char*)resp);

	/* Figure out what's going on */
	switch(status) {
	case 0:	/* Idle; we can continue! */
	case 1: /* Printing */
	{
		int bufs;

		if (resp) free(resp);
		/* Query buffer state */
		dnpds40_build_cmd(&cmd, "INFO", "FREE_PBUFFER", 0);
		resp = dnpds40_resp_cmd(ctx, &cmd, &len);

		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);
		/* Check to see if we have sufficient buffers */
		bufs = atoi(((char*)resp)+3);
		if (bufs < buf_needed) {
			INFO("Insufficient printer buffers (%d vs %d), retrying...\n", bufs, buf_needed);
			sleep(1);
			goto top;
		}
		break;
	}
	case 500: /* Cooling print head */
	case 510: /* Cooling paper motor */
		INFO("Printer cooling down...\n");
		sleep(1);
		goto top;
	case 900:
		INFO("Waking printer up from standby...\n");
		// XXX do someting here?
		break;
	case 1000: /* Cover open */
	case 1010: /* No Scrap Box */
	case 1100: /* Paper End */
	case 1200: /* Ribbon End */
	case 1300: /* Paper Jam */
	case 1400: /* Ribbon Error */
		WARNING("Printer not ready: %s, please correct...\n", dnpds40_statuses(status));
		sleep(1);
		goto top;
	case 1500: /* Paper definition error */
		ERROR("Paper definition error, aborting job\n");
		return CUPS_BACKEND_CANCEL;
	case 1600: /* Data error */
		ERROR("Data error, aborting job\n");
		return CUPS_BACKEND_CANCEL;
	default:
		ERROR("Fatal Printer Error: %d => %s, halting queue!\n", status, dnpds40_statuses(status));
		return CUPS_BACKEND_HOLD;
	}

	/* Verify we have sufficient media for prints */
	{
		int i = 0;

		/* See if we can rewind to save media */
		if (ctx->can_rewind && ctx->supports_rewind) {
			/* Tell the printer we want to rewind, if possible. */
			snprintf(buf, sizeof(buf), "%08d", ctx->multicut + 400);
			memcpy(ctx->multicut_offset, buf, 8);

			/* Get Media remaining */
			dnpds40_build_cmd(&cmd, "INFO", "RQTY", 0);

			if (resp) free(resp);
			resp = dnpds40_resp_cmd(ctx, &cmd, &len);
			if (!resp)
				return CUPS_BACKEND_FAILED;

			dnpds40_cleanup_string((char*)resp, len);
			i = atoi((char*)resp+4);
		}

		/* If we didn't succeed with RQTY, try MQTY */
		if (i == 0) {
			dnpds40_build_cmd(&cmd, "INFO", "MQTY", 0);

			if (resp) free(resp);
			resp = dnpds40_resp_cmd(ctx, &cmd, &len);
			if (!resp)
				return CUPS_BACKEND_FAILED;

			dnpds40_cleanup_string((char*)resp, len);

			i = atoi((char*)resp+4);

			/* For some reason all but the DS620 report 50 too high */
			if (ctx->type != P_DNP_DS620 && i > 0)
				i -= 50;
		}
#if 0
		if (i < 1) {
			ERROR("Printer out of media, please correct!\n");
			return CUPS_BACKEND_STOP;
		}
#endif
		if (i < copies) {
			WARNING("Printer does not have sufficient remaining media to complete job..\n");
		}
	}

	/* Send the stream over as individual data chunks */
	ptr = ctx->databuf;

	while(ptr && ptr < (ctx->databuf + ctx->datalen)) {
		int i;
		buf[8] = 0;
		memcpy(buf, ptr + 24, 8);
		i = atoi(buf) + 32;

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ptr, i)))
			return CUPS_BACKEND_FAILED;

		ptr += i;
	}

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
#ifdef MATTE_GLOSSY_2BUF
		/* No need to wait on buffers due to matte switching */
		buf_needed = ctx->buf_needed;
#endif
		goto top;
	}

	return CUPS_BACKEND_OK;
}

static int dnpds40_get_sensors(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;
	char *tok;

	/* Get Sensor Info */
	dnpds40_build_cmd(&cmd, "INFO", "SENSOR", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	tok = strtok((char*)resp, "; -");
	do {
		char *val = strtok(NULL, "; -");

		if (!strcmp("HDT", tok)) {
			INFO("Head Temperature   : %s\n", val);
		} else if (!strcmp("MDT", tok)) {
			INFO("Media Temperature  : %s\n", val);
		} else if (!strcmp("PMK", tok)) {
			INFO("Paper Mark         : %s\n", val);
		} else if (!strcmp("RML", tok)) {
			INFO("Ribbon Mark Left   : %s\n", val);
		} else if (!strcmp("RMC", tok)) {
			INFO("Ribbon Mark Right  : %s\n", val);
		} else if (!strcmp("RMR", tok)) {
			INFO("Ribbon Mark Center : %s\n", val);
		} else if (!strcmp("PSZ", tok)) {
			INFO("Paper Size         : %s\n", val);
		} else if (!strcmp("PNT", tok)) {
			INFO("Paper Notch        : %s\n", val);
		} else if (!strcmp("PJM", tok)) {
			INFO("Paper Jam          : %s\n", val);
		} else if (!strcmp("PED", tok)) {
			INFO("Paper End          : %s\n", val);
		} else if (!strcmp("PET", tok)) {
			INFO("Paper Empty        : %s\n", val);
		} else if (!strcmp("HDV", tok)) {
			INFO("Head Voltage       : %s\n", val);
		} else if (!strcmp("HMD", tok)) {
			INFO("Humidity           : %s\n", val);
		} else if (!strcmp("RP1", tok)) {
			INFO("Roll Paper End 1   : %s\n", val);
		} else if (!strcmp("RP2", tok)) {
			INFO("Roll Paper End 2   : %s\n", val);
		} else if (!strcmp("CSR", tok)) {
			INFO("Color Sensor Red   : %s\n", val);
		} else if (!strcmp("CSG", tok)) {
			INFO("Color Sensor Green : %s\n", val);
		} else if (!strcmp("CSB", tok)) {
			INFO("Color Sensor Blue  : %s\n", val);
		} else {
			INFO("Unknown Sensor: '%s' '%s'\n",
			     tok, val);
		}
	} while ((tok = strtok(NULL, "; -")) != NULL);

	free(resp);

	return CUPS_BACKEND_OK;
}

static int dnpds40_get_info(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;

	/* Serial number already queried */
	INFO("Serial Number: '%s'\n", ctx->serno);

	/* Firmware version already queried */
	INFO("Firmware Version: '%s'\n", ctx->version);

	/* Get Media Color offset */
	dnpds40_build_cmd(&cmd, "INFO", "MCOLOR", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media Color Offset: '%02x%02x%02x%02x'\n", *(resp+2), *(resp+3),
	     *(resp+4), *(resp+5));

	free(resp);

	/* Get Media Class */
	dnpds40_build_cmd(&cmd, "INFO", "MEDIA_CLASS", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media Class: '%s'\n", (char*)resp);

	free(resp);

	/* Get Media Lot */
	dnpds40_build_cmd(&cmd, "INFO", "MLOT", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media Lot Code: '");
	/* 16-byte data in a 20-byte response */
	for (len = 0 ; len < 16 ; len++) {
		DEBUG2("%c", *(resp+len+2));
	}
	DEBUG2("'\n");
	free(resp);

	/* Get Media ID Set (?) */
	dnpds40_build_cmd(&cmd, "MNT_RD", "MEDIA_ID_SET", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media ID(?): '%s'\n", (char*)resp+4);

	free(resp);

	/* Get Ribbon ID code (?) */
	dnpds40_build_cmd(&cmd, "MNT_RD", "RIBBON_ID_CODE", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Ribbon ID(?): '%s'\n", (char*)resp+4);

	free(resp);

	/* Figure out control data and checksums */

	/* 300 DPI */
	dnpds40_build_cmd(&cmd, "TBL_RD", "CWD300_Version", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("300 DPI Color Data Version: '%s' ", (char*)resp);

	free(resp);

	dnpds40_build_cmd(&cmd, "TBL_RD", "CWD300_Checksum", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	DEBUG2("Checksum: '%s'\n", (char*)resp);

	free(resp);

	/* 600 DPI */
	dnpds40_build_cmd(&cmd, "TBL_RD", "CWD600_Version", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("600 DPI Color Data Version: '%s' ", (char*)resp);

	free(resp);

	dnpds40_build_cmd(&cmd, "TBL_RD", "CWD600_Checksum", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	DEBUG2("Checksum: '%s'\n", (char*)resp);

	free(resp);

	if (ctx->type == P_DNP_DS620) {
		/* "Low Speed" */
		dnpds40_build_cmd(&cmd, "TBL_RD", "CWD610_Version", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Low Speed Color Data Version: '%s' ", (char*)resp);

		free(resp);

		dnpds40_build_cmd(&cmd, "TBL_RD", "CWD610_Checksum", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		DEBUG2("Checksum: '%s'\n", (char*)resp);

		free(resp);
	}

	if (ctx->type == P_DNP_DS620) {
		/* Get Standby stuff */
		dnpds40_build_cmd(&cmd, "MNT_RD", "STANDBY_TIME", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Standby Transition time: '%s' minutes\n", (char*)resp);

		free(resp);

		/* Get Media End Keep */
		dnpds40_build_cmd(&cmd, "MNT_RD", "END_KEEP_MODE", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Media End kept across power cycles: '%s'\n", (char*)resp);

		free(resp);
	}

	if (ctx->supports_iserial) {
		/* Get USB serial descriptor status */
		dnpds40_build_cmd(&cmd, "MNT_RD", "USB_ISERI_SET", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Report Serial Number in USB descriptor: '%s'\n", (char*)resp);

		free(resp);
	}

	return CUPS_BACKEND_OK;
}

static int dnpds40_get_status(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "STATUS", "", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);
	len = atoi((char*)resp);

	INFO("Printer Status: %d => %s\n", len, dnpds40_statuses(len));

	free(resp);

	/* Get remaining print quantity */
	dnpds40_build_cmd(&cmd, "INFO", "PQTY", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Prints remaining in job: '%s'\n", (char*)resp + 4);

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "INFO", "FREE_PBUFFER", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Free Buffers: '%s'\n", (char*)resp + 3);

	free(resp);

	/* Report media */
	INFO("Media Type: '%s'\n", dnpds40_media_types(ctx->media));

	if (ctx->supports_mqty_default) {
		/* Get Media remaining */
		dnpds40_build_cmd(&cmd, "INFO", "MQTY_DEFAULT", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		len = atoi((char*)resp+4);

		INFO("Prints Available on New Media: '%d'\n", len);

		free(resp);
	}

	/* Get Media remaining */
	dnpds40_build_cmd(&cmd, "INFO", "MQTY", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	len = atoi((char*)resp+4);
	if (ctx->type != P_DNP_DS620 && len > 0)
		len -= 50;

	INFO("Prints Remaining on Media: '%d'\n", len);

	free(resp);

	if (ctx->supports_rewind) {
		/* Get Media remaining */
		dnpds40_build_cmd(&cmd, "INFO", "RQTY", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("L/PC Prints Remaining on Media: '%s'\n", (char*)resp + 4);

		free(resp);
	}

	return 0;
}

static int dnpds40_get_counters(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_LIFE", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Lifetime Counter: '%s'\n", (char*)resp+2);

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_A", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("A Counter: '%s'\n", (char*)resp+2);

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_B", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("B Counter: '%s'\n", (char*)resp+2);

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_P", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("P Counter: '%s'\n", (char*)resp+2);

	free(resp);

	if (ctx->supports_matte) {
		/* Generate command */
		dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_M", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("M Counter: '%s'\n", (char*)resp+2);

		free(resp);

		/* Generate command */
		dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_MATTE", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Matte Counter: '%s'\n", (char*)resp+4);

		free(resp);
	}

	return CUPS_BACKEND_OK;
}

static int dnpds40_clear_counter(struct dnpds40_ctx *ctx, char counter)
{
	struct dnpds40_cmd cmd;
	char msg[4];
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_WT", "COUNTER_CLEAR", 4);
	msg[0] = 'C';
	msg[1] = counter;
	msg[2] = 0x0d; /* ie carriage return, ASCII '\r' */
	msg[3] = 0x00;

	if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)msg, 4)))
		return ret;

	return 0;
}

static int dnpds620_standby_mode(struct dnpds40_ctx *ctx, int delay)
{
	struct dnpds40_cmd cmd;
	char msg[9];
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_WT", "STANDBY_TIME", 8);
	snprintf(msg, sizeof(msg), "%08d", delay);

	if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)msg, 8)))
		return ret;

	return 0;
}

static int dnpds620_media_keep_mode(struct dnpds40_ctx *ctx, int delay)
{
	struct dnpds40_cmd cmd;
	char msg[9];
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_WT", "END_KEEP_MODE", 4);
	snprintf(msg, sizeof(msg), "%02d\r", delay);

	if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)msg, 4)))
		return ret;

	return 0;
}

static int dnpds620_iserial_mode(struct dnpds40_ctx *ctx, int enable)
{
	struct dnpds40_cmd cmd;
	char msg[9];
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_WT", "USB_ISERI_SET", 4);
	snprintf(msg, sizeof(msg), "%02d\r", enable);

	if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)msg, 8)))
		return ret;

	return 0;
}

static int dnpds40_set_counter_p(struct dnpds40_ctx *ctx, char *arg)
{
	struct dnpds40_cmd cmd;
	char msg[9];
	int i = atoi(arg);
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_WT", "COUNTERP_SET", 8);
	snprintf(msg, sizeof(msg), "%08d", i);

	if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)msg, 8)))
		return ret;

	return 0;
}

static void dnpds40_cmdline(void)
{
	DEBUG("\t\t[ -i ]           # Query printer info\n");
	DEBUG("\t\t[ -I ]           # Query sensor  info\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -n ]           # Query counters\n");
	DEBUG("\t\t[ -N A|B|M ]     # Clear counter A/B/M\n");
	DEBUG("\t\t[ -p num ]       # Set counter P\n");
	DEBUG("\t\t[ -k num ]       # Set standby time (1-99 minutes, 0 disables)\n");
	DEBUG("\t\t[ -K num ]       # Keep Media Status Across Power Cycles (1 on, 0 off)\n");
	DEBUG("\t\t[ -x num ]       # Set USB iSerialNumber Reporting (1 on, 0 off)\n");
}

static int dnpds40_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct dnpds40_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "iInN:p:sK:k:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL			
		case 'i':
			j = dnpds40_get_info(ctx);
			break;
		case 'I':
			j = dnpds40_get_sensors(ctx);
			break;
		case 'n':
			j = dnpds40_get_counters(ctx);
			break;
		case 'N':
			if (optarg[0] != 'A' &&
			    optarg[0] != 'B' &&
			    optarg[0] != 'M')
				return CUPS_BACKEND_FAILED;
			if (!ctx->supports_matte) {
				ERROR("Printer FW does not support matte functions, please update!\n");
				return CUPS_BACKEND_FAILED;
			}
			j = dnpds40_clear_counter(ctx, optarg[0]);
			break;
		case 'p':
			j = dnpds40_set_counter_p(ctx, optarg);
			break;
		case 's':
			j = dnpds40_get_status(ctx);
			break;
		case 'k': {
			int time = atoi(optarg);
			if (!ctx->supports_standby) {
				ERROR("Printer does not support standby\n");
				j = -1;
				break;
			}
			if (time < 0 || time > 99) {
				ERROR("Value out of range (0-99)");
				j = -1;
				break;
			}
			j = dnpds620_standby_mode(ctx, time);
			break;
		}
		case 'K': {
			int keep = atoi(optarg);
			if (!ctx->supports_standby) {
				ERROR("Printer does not support media keep mode\n");
				j = -1;
				break;
			}
			if (keep < 0 || keep > 1) {
				ERROR("Value out of range (0-1)");
				j = -1;
				break;
			}
			j = dnpds620_media_keep_mode(ctx, keep);
			break;
		}
		case 'x': {
			int enable = atoi(optarg);
			if (!ctx->supports_iserial) {
				ERROR("Printer does not support USB iSerialNumber reporting\n");
				j = -1;
				break;
			}
			if (enable < 0 || enable > 1) {
				ERROR("Value out of range (0-1)");
				j = -1;
				break;
			}
			j = dnpds620_iserial_mode(ctx, enable);
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
struct dyesub_backend dnpds40_backend = {
	.name = "DNP DS40/DS80/DSRX1/DS620",
	.version = "0.61.2",
	.uri_prefix = "dnpds40",
	.cmdline_usage = dnpds40_cmdline,
	.cmdline_arg = dnpds40_cmdline_arg,
	.init = dnpds40_init,
	.attach = dnpds40_attach,
	.teardown = dnpds40_teardown,
	.read_parse = dnpds40_read_parse,
	.main_loop = dnpds40_main_loop,
	.query_serno = dnpds40_query_serno,
	.devices = {
	{ USB_VID_CITIZEN, USB_PID_DNP_DS40, P_DNP_DS40, ""},
	{ USB_VID_CITIZEN, USB_PID_DNP_DS80, P_DNP_DS80, ""},
	{ USB_VID_CITIZEN, USB_PID_DNP_DSRX1, P_DNP_DSRX1, ""},
	{ USB_VID_DNP, USB_PID_DNP_DS620, P_DNP_DS620, ""},
//	{ USB_VID_DNP, USB_PID_DNP_DS80D, P_DNP_DS80D, ""},
//	{ USB_VID_CITIZEN, USB_PID_CITIZEN_CW-02, P_DNP_DS40, ""},
//	{ USB_VID_CITIZEN, USB_PID_CITIZEN_OP900II, P_DNP_DS40, ""},
	{ 0, 0, 0, ""}
	}
};
