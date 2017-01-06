/*
 *   DNP DS40/DS80 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2016 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Development of this backend was sponsored by:
 *
 *     Marco Di Antonio and [ ilgruppodigitale.com ]
 *     LiveLink Technology [ www.livelinktechnology.net ]
 *     A generous benefactor who wishes to remain anonymous
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

//#define DNP_ONLY

/* Enables caching of last print type to speed up 
   job pipelining.  Without this we always have to
   assume the worst */
//#define STATE_DIR "/tmp"

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
#define USB_PID_CITIZEN_CW02 0x0006
#define USB_PID_DNP_DS80D 0x0007
#define USB_PID_DNP_DS620_OLD 0x0008

#define USB_VID_DNP       0x1452
#define USB_PID_DNP_DS620 0x8b01
#define USB_PID_DNP_DS820 0x9001

/* Private data stucture */
struct dnpds40_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;

	int type;

	char *serno;
	char *version;

	int buf_needed;

	int ver_major;
	int ver_minor;

	uint32_t media;
	uint32_t duplex_media;
	uint16_t media_count_new;
	uint32_t multicut;

	uint32_t last_multicut;
	int last_matte;

	int fullcut;	
	int matte;
	int cutter;
	int can_rewind;

	int printspeed;

	int mediaoffset;
	int manual_copies;
	int correct_count;

	uint32_t native_width;
	int supports_6x9;
	int supports_2x6;
	int supports_3x5x2;
	int supports_matte;
	int supports_finematte;
	int supports_luster;	
	int supports_advmatte;
	int supports_fullcut;
	int supports_rewind;
	int supports_standby;
	int supports_6x4_5;
	int supports_mqty_default;
	int supports_iserial;
	int supports_6x6;
	int supports_5x5;
	int supports_counterp;
	int supports_adv_fullcut;
	int supports_mediaoffset;
	int supports_media_ext;
	int supports_printspeed;
	int supports_lowspeed;
	int supports_highdensity;
	int supports_gamma;
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

#define MULTICUT_5x3_5     1
#define MULTICUT_6x4       2
#define MULTICUT_5x7       3
#define MULTICUT_6x8       4
#define MULTICUT_6x9       5
#define MULTICUT_8x10      6
#define MULTICUT_8x12      7
#define MULTICUT_8x4       8
#define MULTICUT_8x5       9
#define MULTICUT_8x6      10
#define MULTICUT_8x8      11
#define MULTICUT_6x4X2    12
#define MULTICUT_8x4X2    13
#define MULTICUT_8x5X2    14
#define MULTICUT_8x6X2    15
#define MULTICUT_8x5_8x4  16
#define MULTICUT_8x6_8x4  17
#define MULTICUT_8x6_8x5  18
#define MULTICUT_8x8_8x4  19
#define MULTICUT_8x4X3    20
#define MULTICUT_8xA4LEN  21
#define MULTICUT_5x3_5X2  22
#define MULTICUT_6x6      27
#define MULTICUT_5x5      29
#define MULTICUT_6x4_5    30
#define MULTICUT_6x4_5X2  31
#define MULTICUT_8x7      32
#define MULTICUT_8x9      33
#define MULTICUT_A5       34
#define MULTICUT_A5X2     35
#define MULTICUT_A4x4     36
#define MULTICUT_A4x5     37
#define MULTICUT_A4x6     38
#define MULTICUT_A4x8     39
#define MULTICUT_A4x10    40
#define MULTICUT_A4       41
#define MULTICUT_A4x5X2   43


#define MULTICUT_S_SIMPLEX  100
#define MULTICUT_S_FRONT    200
#define MULTICUT_S_BACK     300

#define MULTICUT_S_8x10     6
#define MULTICUT_S_8x12     7
#define MULTICUT_S_8x4      8
#define MULTICUT_S_8x5      9
#define MULTICUT_S_8x6     10
#define MULTICUT_S_8x8     11
#define MULTICUT_S_8x4X2   13
#define MULTICUT_S_8x5X2   14
#define MULTICUT_S_8x6X2   15
#define MULTICUT_S_8x10_5  25
#define MULTICUT_S_8x10_75 26
#define MULTICUT_S_8x4X3   28  // different than roll type.

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

static char *dnpds40_printer_type(int type)
{
	switch(type) {
	case P_DNP_DS40: return "DS40";
	case P_DNP_DS80: return "DS80";
	case P_DNP_DS80D: return "DS80DX";
	case P_DNP_DSRX1: return "DSRX1";
	case P_DNP_DS620: return "DS620";
	case P_DNP_DS820: return "DS820";
	default: break;
	}
	return "Unknown";
}

static char *dnpds40_media_types(int media)
{
	switch (media) {
	case 100: return "UNKNOWN100"; // seen in driver dumps
	case 110: return "UNKNOWN110"; // seen in driver dumps
	case 200: return "5x3.5 (L)";
	case 210: return "5x7 (2L)";
	case 300: return "6x4 (PC)";
	case 310: return "6x8 (A5)";
	case 400: return "6x9 (A5W)";
	case 500: return "8x10";
	case 510: return "8x12";
	case 600: return "A4";
	default:
		break;
	}

	return "Unknown type";
}

static char *dnpds620_media_extension_code(int media)
{
	switch (media) {
	case 00: return "Normal Paper";
	case 01: return "Sticky Paper";
	case 99: return "Unknown Paper";
	default:
		break;
	}

	return "Unknown type";
}

static char *dnpds820_media_subtypes(int media)
{
	switch (media) {
	case 0001: return "SD";
	case 0003: return "PP";
	default:
		break;
	}

	return "Unknown type";
}

static char *dnpds80_duplex_media_types(int media)
{
	switch (media) {
	case 100: return "8x10.75";
	case 200: return "8x12";
	default:
		break;
	}

	return "Unknown type";
}

static char *dnpds80_duplex_statuses(int status)
{
	switch (status) {
	case 5000: return "No Error";

	case 5500: return "Duplex Unit Not Connected";

	case 5017: return "Paper Jam: Supply Sensor On";
	case 5018: return "Paper Jam: Supply Sensor Off";
	case 5019: return "Paper Jam: Slot Sensor On";
	case 5020: return "Paper Jam: Slot Sensor Off";
	case 5021: return "Paper Jam: Pass Sensor On";
	case 5022: return "Paper Jam: Pass Sensor Off";
	case 5023: return "Paper Jam: Shell Sensor 1 On";
	case 5024: return "Paper Jam: Shell Sensor 1 Off";
	case 5025: return "Paper Jam: Shell Sensor 2 On";
	case 5026: return "Paper Jam: Shell Sensor 2 Off";
	case 5027: return "Paper Jam: Eject Sensor On";
	case 5028: return "Paper Jam: Eject Sensor Off";
	case 5029: return "Paper Jam: Slot FG Sensor";
	case 5030: return "Paper Jam: Shell FG Sensor";

	case 5033: return "Paper Supply Sensor Off";
	case 5034: return "Printer Feed Slot Sensor Off";
	case 5035: return "Pinch Pass Sensor Off";
	case 5036: return "Shell Pass Sensor 1 Off";
	case 5037: return "Shell Pass Sensor 2 Off";
	case 5038: return "Eject Sensor Off";

	case 5049: return "Capstan Drive Control Error";
	case 5065: return "Shell Roller Error";

	case 5081: return "Pinch Open Error";
	case 5082: return "Pinch Close Error";
	case 5083: return "Pinch Init Error";
	case 5084: return "Pinch Position Error";

	case 5097: return "Pass Guide Supply Error";
	case 5098: return "Pass Guide Shell Error";
	case 5099: return "Pass Guide Eject Error";
	case 5100: return "Pass Guide Init Error";
	case 5101: return "Pass Guide Position Error";

	case 5113: return "Side Guide Home Error";
	case 5114: return "Side Guide Position Error";
	case 5115: return "Side Guide Init Error";

	case 5129: return "Act Guide Home Error";

	case 5145: return "Shell Rotate Home Error";
	case 5146: return "Shell Rotate Rev Error";

	case 5161: return "Paper Feed Lever Down Error";
	case 5162: return "Paper Feed Lever Lock Error";
	case 5163: return "Paper Feed Lever Up Error";

	case 5177: return "Cutter Home Error";
	case 5178: return "Cutter Away Error";
	case 5179: return "Cutter Init Error";
	case 5180: return "Cutter Position Error";

	case 5193: return "Paper Tray Removed";
	case 5209: return "Cover Opened";
	case 5241: return "System Error";

	default:
		break;
	}

	return "Unkown Duplexer Error";
}

static char *dnpds40_statuses(int status)
{
	if (status >= 5000 && status <= 5999)
		return dnpds80_duplex_statuses(status);

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

static uint8_t *dnpds40_resp_cmd2(struct dnpds40_ctx *ctx,
				  struct dnpds40_cmd *cmd,
				  int *len,
				  uint8_t *buf, uint32_t buf_len)
{
	char tmp[9];
	uint8_t *respbuf;

	int ret, i, num = 0;

	memset(tmp, 0, sizeof(tmp));

	if ((ret = dnpds40_do_cmd(ctx, cmd, buf, buf_len)))
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

#define dnpds40_resp_cmd(__ctx, __cmd, __len) dnpds40_resp_cmd2(__ctx, __cmd, __len, NULL, 0)

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

	if (ctx->type == P_DNP_DS80D) {
		struct dnpds40_cmd cmd;
		uint8_t *resp;
		int len = 0;

		/* Query Duplex Media Info */
		dnpds40_build_cmd(&cmd, "INFO", "CUT_PAPER", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (resp) {
			char tmp[5];

			dnpds40_cleanup_string((char*)resp, len);

			memcpy(tmp, resp + 4, 4);
			tmp[4] = 0;

			ctx->duplex_media = atoi(tmp);

			/* Subtract out the paper status */
			if (ctx->duplex_media & 3)
				ctx->duplex_media -= (ctx->duplex_media & 3);

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
		ctx->native_width = 1920;
		ctx->supports_6x9 = 1;
		if (FW_VER_CHECK(1,04))
			ctx->supports_counterp = 1;
		if (FW_VER_CHECK(1,30))
			ctx->supports_matte = 1;
		if (FW_VER_CHECK(1,40))
			ctx->supports_2x6 = 1;
		break;
	case P_DNP_DS80:
	case P_DNP_DS80D:
		ctx->native_width = 2560;
		if (FW_VER_CHECK(1,02))
			ctx->supports_counterp = 1;
		if (FW_VER_CHECK(1,30))
			ctx->supports_matte = 1;
		break;
	case P_DNP_DSRX1:
		ctx->native_width = 1920;
		ctx->supports_counterp = 1;
		ctx->supports_matte = 1;
		if (FW_VER_CHECK(1,10))
			ctx->supports_2x6 = ctx->supports_mqty_default = 1;
                if (FW_VER_CHECK(1,20))
			ctx->supports_3x5x2 = 1;
		if (FW_VER_CHECK(2,00)) { /* AKA RX1HS */
			ctx->supports_mediaoffset = 1;
			ctx->supports_iserial = 1;
		}
		break;
	case P_DNP_DS620:
		ctx->native_width = 1920;
		ctx->correct_count = 1;
		ctx->supports_counterp = 1;
		ctx->supports_matte = 1;
		ctx->supports_2x6 = 1;
		ctx->supports_fullcut = 1;
		ctx->supports_mqty_default = 1;
		if (strchr(ctx->version, 'A'))
			ctx->supports_rewind = 0;
		else
			ctx->supports_rewind = 1;
		ctx->supports_standby = 1;
		ctx->supports_iserial = 1;
		ctx->supports_6x6 = 1;
		ctx->supports_5x5 = 1;
		ctx->supports_lowspeed = 1;
		if (FW_VER_CHECK(0,30))
			ctx->supports_3x5x2 = 1;
		if (FW_VER_CHECK(1,10))
			ctx->supports_6x9 = ctx->supports_6x4_5 = 1;
		if (FW_VER_CHECK(1,20))
			ctx->supports_adv_fullcut = ctx->supports_advmatte = 1;
		if (FW_VER_CHECK(1,30))
			ctx->supports_luster = ctx->supports_finematte = 1;
		if (FW_VER_CHECK(1,33))
			ctx->supports_media_ext = 1;
		break;
	case P_DNP_DS820:
		ctx->native_width = 2560;
		ctx->correct_count = 1;
		ctx->supports_counterp = 1;
		ctx->supports_matte = 1;
		ctx->supports_fullcut = 1;
		ctx->supports_mqty_default = 1;
		if (strchr(ctx->version, 'A'))
			ctx->supports_rewind = 0;
		else
			ctx->supports_rewind = 1;
		ctx->supports_standby = 1;
		ctx->supports_iserial = 1;
		ctx->supports_adv_fullcut = 1;
		ctx->supports_advmatte = 1;
		ctx->supports_luster = 1;
		ctx->supports_finematte = 1;
		ctx->supports_printspeed = 1;
		ctx->supports_lowspeed = 1;
		ctx->supports_highdensity = 1;
		if (FW_VER_CHECK(0,50))
			ctx->supports_gamma = 1;
		break;
	default:
		ERROR("Unknown vid/pid %04x/%04x (%d)\n", desc.idVendor, desc.idProduct, ctx->type);
		return;
	}

	ctx->last_matte = -1;
#ifdef STATE_DIR
	/* Check our current job's lamination vs previous job. */
	{
		/* Load last matte status from file */
		char buf[64];
		FILE *f;
		snprintf(buf, sizeof(buf), STATE_DIR "/%s-last", ctx->serno);
		f = fopen(buf, "r");
		if (f) {
			fscanf(f, "%d", &ctx->last_matte);
			fclose(f);
		}
	}
#endif

	if (ctx->supports_mediaoffset) {
		/* Get Media Offset */
		struct dnpds40_cmd cmd;
		uint8_t *resp;
		int len = 0;

		dnpds40_build_cmd(&cmd, "INFO", "MEDIA_OFFSET", 0);
		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (resp) {
			ctx->mediaoffset = atoi((char*)resp+4);
			free(resp);
		}
	} else if (!ctx->correct_count) {
		ctx->mediaoffset = 50;
	}

	if (ctx->supports_mqty_default) {
		struct dnpds40_cmd cmd;
		uint8_t *resp;
		int len = 0;

		dnpds40_build_cmd(&cmd, "INFO", "MQTY_DEFAULT", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (resp) {
			dnpds40_cleanup_string((char*)resp, len);
			ctx->media_count_new = atoi((char*)resp+4);
			free(resp);
			ctx->media_count_new -= ctx->mediaoffset;
		}
	} else {
		/* Look it up for legacy models & FW */
		switch (ctx->type) {
		case P_DNP_DS40:
			switch (ctx->media) {
			case 200: // L
				ctx->media_count_new = 460;
				break;
			case 210: // 2L
				ctx->media_count_new = 230;
				break;
			case 300: // PC
				ctx->media_count_new = 400;
				break;
			case 310: // A5
				ctx->media_count_new = 200;
				break;
			case 400: // A5W
				ctx->media_count_new = 180;
				break;
			default:
				ctx->media_count_new = 999; // non-zero
				break;
			}
			break;
		case P_DNP_DSRX1:
			switch (ctx->media) {
			case 300: // PC
				ctx->media_count_new = 700;
				break;
			case 310: // A5
				ctx->media_count_new = 350;
				break;
			default:
				ctx->media_count_new = 999; // non-zero
				break;
			}
			break;
		case P_DNP_DS80:
		case P_DNP_DS80D:
			switch (ctx->media) {
			case 500: // 8x10
				ctx->media_count_new = 130;
				break;
			case 510: // 8x12
				ctx->media_count_new = 110;
				break;
			default:
				ctx->media_count_new = 999; // non-zero
				break;
			}
			break;
		default:
			ctx->media_count_new = 999; // non-zero
			break;
		}
	}
}

static void dnpds40_teardown(void *vctx) {
	struct dnpds40_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->type == P_DNP_DS80D) {
		struct dnpds40_cmd cmd;

		/* Check to see if last print was the front side
		   of a duplex job, and if so, cancel things so we're done */
		if (ctx->last_multicut >= 200 &&
		    ctx->last_multicut < 300) {
			dnpds40_build_cmd(&cmd, "CNTRL", "DUPLEX_CANCEL", 0);
			if ((dnpds40_do_cmd(ctx, &cmd, NULL, 0)) != 0)
				return;
		}
	}

	if (ctx->databuf)
		free(ctx->databuf);
	if (ctx->serno)
		free(ctx->serno);
	if (ctx->version)
		free(ctx->version);
	free(ctx);
}

#define MAX_PRINTJOB_LEN (((2560*7536+1024+54))*3+1024) /* Worst-case, YMC */

static int dnpds40_read_parse(void *vctx, int data_fd) {
	struct dnpds40_ctx *ctx = vctx;
	int run = 1;
	char buf[9] = { 0 };

	uint32_t dpi;

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
	dpi = 0;
	ctx->matte = 0;
	ctx->cutter = 0;
	ctx->manual_copies = 0;
	ctx->multicut = 0;
	ctx->fullcut = 0;
	ctx->printspeed = -1;
	ctx->can_rewind = 0;
	ctx->buf_needed = 0;

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
			/* Ignore this.  We will insert our own later on */
			continue;			
		}
		if(!memcmp("CNTRL CUTTER", ctx->databuf + ctx->datalen+2, 12)) {
			memcpy(buf, ctx->databuf + ctx->datalen + 32, 8);
			ctx->cutter = atoi(buf);
			/* We'll insert it ourselves later */
			continue;
		}
		if(!memcmp("CNTRL BUFFCNTRL", ctx->databuf + ctx->datalen+2, 15)) {
			/* Ignore this.  We will insert our own later on
			   if the printer and job support it. */
			continue;
		}
		if(!memcmp("CNTRL OVERCOAT", ctx->databuf + ctx->datalen+2, 14)) {
			if (ctx->supports_matte) {
				memcpy(buf, ctx->databuf + ctx->datalen + 32, 8);
				ctx->matte = atoi(buf);
			} else {
				WARNING("Printer FW does not support matte prints, using glossy mode\n");
			}
			/* We'll insert our own later, if appropriate */
			continue;
		}
		if(!memcmp("IMAGE MULTICUT", ctx->databuf + ctx->datalen+2, 14)) {
			memcpy(buf, ctx->databuf + ctx->datalen + 32, 8);
			ctx->multicut = atoi(buf);
			/* Backend automatically handles rewind support, so
			   ignore application requests to use it. */
			if (ctx->multicut > 400)
				ctx->multicut -= 400;

			/* We'll insert this ourselves later. */
			continue;
		}
		if(!memcmp("CNTRL FULL_CUTTER_SET", ctx->databuf + ctx->datalen+2, 21)) {
			if (!ctx->supports_fullcut) {
				WARNING("Printer FW does not support full cutter control!\n");
				continue;
			}

			if (ctx->type == P_DNP_DS820) {
				if (j != 24) {
					WARNING("Full cutter argument length incorrect, ignoring!\n");
					continue;
				}
			} else if (j != 16) {
				WARNING("Full cutter argument length incorrect, ignoring!\n");
				continue;
			} else if (!ctx->supports_adv_fullcut) {
				if (ctx->databuf[ctx->datalen + 32 + 12] != '0' ||
				    ctx->databuf[ctx->datalen + 32 + 13] != '0' ||
				    ctx->databuf[ctx->datalen + 32 + 14] != '0') {
					WARNING("Full cutter scrap setting not supported on this firmware, ignoring!\n");
					continue;
				}
			}
			// XXX enforce cut counts/sizes?

			ctx->fullcut = 1;
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
				ERROR("Unrecognized printjob resolution (%u ppm)\n", y_ppm);
				return CUPS_BACKEND_CANCEL;
			}

			/* Validate horizontal size */
			memcpy(&y_ppm, ctx->databuf + ctx->datalen + 32 + 18, sizeof(y_ppm));
			y_ppm = le32_to_cpu(y_ppm);
			if (y_ppm != ctx->native_width) {
				ERROR("Incorrect horizontal resolution (%u), aborting!\n", y_ppm);
				return CUPS_BACKEND_CANCEL;
			}
		}
		if(!memcmp("CNTRL PRINTSPEED", ctx->databuf + ctx->datalen + 2, 16)) {
			if (!ctx->supports_printspeed) {
				WARNING("Printer does not support PRINTSPEED\n");
				continue;
			}
			memcpy(buf, ctx->databuf + ctx->datalen + 32, 8);
			ctx->printspeed = atoi(buf) / 10;

			/* We'll insert this ourselves later. */
			continue;
		}

		/* This is the last block.. */
	        if(!memcmp("CNTRL START", ctx->databuf + ctx->datalen + 2, 11))
			run = 0;

		/* Add in the size of this chunk */
		ctx->datalen += sizeof(struct dnpds40_cmd) + j;
	}

	/* If we have no data.. don't bother */
	if (!ctx->datalen)
		return CUPS_BACKEND_CANCEL;

	/* Sanity check matte mode */
	if (ctx->matte == 21 && !ctx->supports_finematte) {
		WARNING("Printer FW does not support Fine Matte mode, downgrading to normal matte\n");
		ctx->matte = 1;
	} else if (ctx->matte == 22 && !ctx->supports_luster) {
		WARNING("Printer FW does not support Luster mode, downgrading to normal matte\n");
		ctx->matte = 1;
	} else if (ctx->matte > 1 && !ctx->supports_advmatte) {
		WARNING("Printer FW does not support advanced matte modes, downgrading to normal matte\n");
		ctx->matte = 1;
	}

	/* Pick a sane default value for printspeed if not specified */
	if (ctx->printspeed == -1 || ctx->printspeed > 3)
	{
		if (dpi == 600)
			ctx->printspeed = 1;
		else
			ctx->printspeed = 0;
	}
	/* And sanity-check whatever value is there */
	if (ctx->printspeed == 0 && dpi == 600) {
		ctx->printspeed = 1;
	} else if (ctx->printspeed == 1 && dpi == 300) {
		ctx->printspeed = 0;
	}

	/* Make sure MULTICUT is sane, most validation needs this */
	if (!ctx->multicut) {
		WARNING("Missing or illegal MULTICUT command!\n");
		if (dpi == 300)
			ctx->buf_needed = 1;
		else
			ctx->buf_needed = 2;

		goto skip_checks;
	}

	/* Only DS80D supports Cut Paper types */
	if (ctx->multicut > 100 &&
	    ctx->type != P_DNP_DS80D) {
		ERROR("Only DS80D supports cut-paper sizes!\n");
		return CUPS_BACKEND_CANCEL;
	}

	/* Figure out the number of buffers we need. */
	ctx->buf_needed = 1;

	if (dpi == 600) {
		switch(ctx->type) {
		case P_DNP_DS620:
			if (ctx->multicut == MULTICUT_6x9 ||
			    ctx->multicut == MULTICUT_6x4_5X2)
				ctx->buf_needed = 2;
			break;
		case P_DNP_DS80:  /* DS80/CX-W */
			if (ctx->matte && (ctx->multicut == MULTICUT_8xA4LEN ||
					   ctx->multicut == MULTICUT_8x4X3 ||
					   ctx->multicut == MULTICUT_8x8_8x4 ||
					   ctx->multicut == MULTICUT_8x6X2 ||
					   ctx->multicut == MULTICUT_8x12))
				ctx->buf_needed = 2;
			break;
		case P_DNP_DS80D:
			if (ctx->matte) {
				int mcut = ctx->multicut;
				
				if (mcut > MULTICUT_S_BACK)
					mcut -= MULTICUT_S_BACK;
				else if (mcut > MULTICUT_S_FRONT)
					mcut -= MULTICUT_S_FRONT;

				if (mcut == MULTICUT_8xA4LEN ||
				    mcut == MULTICUT_8x4X3 ||
				    mcut == MULTICUT_8x8_8x4 ||
				    mcut == MULTICUT_8x6X2 ||
				    mcut == MULTICUT_8x12)
					ctx->buf_needed = 2;

				if (mcut == MULTICUT_S_8x12 ||
				    mcut == MULTICUT_S_8x6X2 ||
				    mcut == MULTICUT_S_8x4X3)
					ctx->buf_needed = 2;
			}
			break;
		case P_DNP_DS820:
			// Nothing; all sizes only need 1 buffer
			break;
		default: /* DS40/CX/RX1/CY/everything else */
			if (ctx->matte) {
				if (ctx->multicut == MULTICUT_6x8 ||
				    ctx->multicut == MULTICUT_6x9 ||
				    ctx->multicut == MULTICUT_6x4X2 ||
				    ctx->multicut == MULTICUT_5x7 ||
				    ctx->multicut == MULTICUT_5x3_5X2)
					ctx->buf_needed = 2;

			} else {
				if (ctx->multicut == MULTICUT_6x8 ||
				    ctx->multicut == MULTICUT_6x9 ||
				    ctx->multicut == MULTICUT_6x4X2)
					ctx->buf_needed = 1;
			}
			break;
		}
	}

	/* Sanity-check type vs loaded media */
	if (ctx->multicut < 100) {
		switch(ctx->media) {
		case 200: //"5x3.5 (L)"
			if (ctx->multicut != MULTICUT_5x3_5) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 210: //"5x7 (2L)"
			if (ctx->multicut != MULTICUT_5x3_5 && ctx->multicut != MULTICUT_5x7 &&
			    ctx->multicut != MULTICUT_5x3_5X2 && ctx->multicut != MULTICUT_5x5) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			/* Only 3.5x5 on 7x5 media can be rewound */
			if (ctx->multicut == MULTICUT_5x3_5)
				ctx->can_rewind = 1;
			break;
		case 300: //"6x4 (PC)"
			if (ctx->multicut != MULTICUT_6x4) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 310: //"6x8 (A5)"
			if (ctx->multicut != MULTICUT_6x4 && ctx->multicut != MULTICUT_6x8 &&
			    ctx->multicut != MULTICUT_6x4X2 &&
			    ctx->multicut != MULTICUT_6x6 && ctx->multicut != 30) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			/* Only 6x4 on 6x8 media can be rewound */
			if (ctx->multicut == MULTICUT_6x4)
				ctx->can_rewind = 1;
			break;
		case 400: //"6x9 (A5W)"
			if (ctx->multicut != MULTICUT_6x4 && ctx->multicut != MULTICUT_6x8 &&
			    ctx->multicut != MULTICUT_6x9 && ctx->multicut != MULTICUT_6x4X2 &&
			    ctx->multicut != MULTICUT_6x6 &&
			    ctx->multicut != MULTICUT_6x4_5 && ctx->multicut != MULTICUT_6x4_5X2) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			/* Only 6x4 or 6x4.5 on 6x9 media can be rewound */
			if (ctx->multicut == MULTICUT_6x4 || ctx->multicut == MULTICUT_6x4_5)
				ctx->can_rewind = 1;
			break;
		case 500: //"8x10"
			if (ctx->type == P_DNP_DS820 &&
			    (ctx->multicut == MULTICUT_8x7 || ctx->multicut == MULTICUT_8x9)) {
				/* These are okay */
			} else if (ctx->multicut < MULTICUT_8x10 || ctx->multicut == MULTICUT_8x12 ||
			    ctx->multicut == MULTICUT_8x6X2 || ctx->multicut >= MULTICUT_8x6_8x5 ) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}

			/* 8x4, 8x5 can be rewound */
			if (ctx->multicut == MULTICUT_8x4 ||
			    ctx->multicut == MULTICUT_8x5)
				ctx->can_rewind = 1;
			break;
		case 510: //"8x12"
			if (ctx->multicut < MULTICUT_8x10 || (ctx->multicut > MULTICUT_8xA4LEN && !(ctx->multicut == MULTICUT_8x7 || ctx->multicut == MULTICUT_8x9))) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}

			/* 8x4, 8x5, 8x6 can be rewound */
			if (ctx->multicut == MULTICUT_8x4 ||
			    ctx->multicut == MULTICUT_8x5 ||
			    ctx->multicut == MULTICUT_8x6)
				ctx->can_rewind = 1;
			break;
		case 600: //"A4"
			if (ctx->multicut < MULTICUT_A5 || ctx->multicut > MULTICUT_A4x5X2) {
				ERROR("Incorrect media for job loaded (%d vs %d)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			/* A4xn and A5 can be rewound */
			if (ctx->multicut == MULTICUT_A4x4 ||
			    ctx->multicut == MULTICUT_A4x5 ||
			    ctx->multicut == MULTICUT_A4x6 ||
			    ctx->multicut == MULTICUT_A5)
				ctx->can_rewind = 1;
			break;
		default:
			ERROR("Unknown media (%u vs %u)!\n", ctx->media, ctx->multicut);
			return CUPS_BACKEND_CANCEL;
		}
	} else if (ctx->multicut < 400) {
		int mcut = ctx->multicut;

		switch(ctx->duplex_media) {
		case 100: //"8x10.75"
			if (mcut > MULTICUT_S_BACK)
				mcut -= MULTICUT_S_BACK;
			else if (mcut > MULTICUT_S_FRONT)
				mcut -= MULTICUT_S_FRONT;

			if (mcut == MULTICUT_S_8x12 ||
			    mcut == MULTICUT_S_8x6X2 ||
			    mcut == MULTICUT_S_8x4X3) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, ctx->multicut);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 200: //"8x12"
			/* Everything is legal */
			break;
		default:
			ERROR("Unknown duplexer media (%u vs %u)!\n", ctx->duplex_media, ctx->multicut);
			return CUPS_BACKEND_CANCEL;
		}
	} else {
		ERROR("Multicut value out of range! (%u)\n", ctx->multicut);
		return CUPS_BACKEND_CANCEL;
	}

	/* Additional santity checks, make sure printer support exists */
	if (!ctx->supports_6x6 && ctx->multicut == MULTICUT_6x6) {
		ERROR("Printer does not support 6x6 prints, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if (!ctx->supports_5x5 && ctx->multicut == MULTICUT_5x5) {
		ERROR("Printer does not support 5x5 prints, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if ((ctx->multicut == MULTICUT_6x4_5 || ctx->multicut == MULTICUT_6x4_5X2) &&
	    !ctx->supports_6x4_5) {
		ERROR("Printer does not support 6x4.5 prints, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if (ctx->multicut == MULTICUT_6x9 && !ctx->supports_6x9) {
		ERROR("Printer does not support 6x9 prints, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if (ctx->multicut == MULTICUT_5x3_5X2 && !ctx->supports_3x5x2) {
		ERROR("Printer does not support 3.5x5*2 prints, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if (ctx->fullcut && !ctx->supports_adv_fullcut &&
	    ctx->multicut != MULTICUT_6x8) {
		ERROR("Printer does not support full control on sizes other than 6x8, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if (ctx->cutter == 120) {
		if (ctx->multicut == MULTICUT_6x4 || ctx->multicut == MULTICUT_6x8) {
			if (!ctx->supports_2x6) {
				ERROR("Printer does not support 2x6 prints, aborting!\n");
				return CUPS_BACKEND_CANCEL;
			}
		} else {
			ERROR("Printer only supports legacy 2-inch cuts on 4x6 or 8x6 jobs!");
			return CUPS_BACKEND_CANCEL;
		}

		/* Work around firmware bug on DS40 where if we run out
		   of media, we can't resume the job without losing the
		   cutter setting. */
		// XXX add version test? what about other printers?
		ctx->manual_copies = 1;
	}

skip_checks:
	DEBUG("dpi %u matte %d mcut %u cutter %d, bufs %d spd %d\n",
	      dpi, ctx->matte, ctx->multicut, ctx->cutter, ctx->buf_needed, ctx->printspeed);

	return CUPS_BACKEND_OK;
}

static int dnpds40_main_loop(void *vctx, int copies) {
	struct dnpds40_ctx *ctx = vctx;
	int ret;
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;
	uint8_t *ptr;
	char buf[9];
	int status;
	int buf_needed;
	int count = 0;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	buf_needed = ctx->buf_needed;

	/* If we switch major overcoat modes, we need both buffers */
	if (!!ctx->matte != ctx->last_matte)
		buf_needed = 2;

	if (ctx->media_count_new) {
		ATTR("marker-colors=#00FFFF#FF00FF#FFFF00\n");
		ATTR("marker-high-levels=100\n");
		ATTR("marker-low-levels=10\n");
		ATTR("marker-names='%s'\n", dnpds40_media_types(ctx->media));
		ATTR("marker-types=ribbonWax\n");
	}

top:

	/* Query status */
	dnpds40_build_cmd(&cmd, "STATUS", "", 0);
	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;
	dnpds40_cleanup_string((char*)resp, len);
	status = atoi((char*)resp);
	free(resp);

	/* Figure out what's going on */
	switch(status) {
	case 0:	/* Idle; we can continue! */
	case 1: /* Printing */
	{
		int bufs;

		/* Query buffer state */
		dnpds40_build_cmd(&cmd, "INFO", "FREE_PBUFFER", 0);
		resp = dnpds40_resp_cmd(ctx, &cmd, &len);

		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);
		/* Check to see if we have sufficient buffers */
		bufs = atoi(((char*)resp)+3);
		free(resp);
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

	{
		/* Figure out remaining native prints */
		dnpds40_build_cmd(&cmd, "INFO", "MQTY", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		count = atoi((char*)resp+4);
		free(resp);

		if (count) {
			/* Old-sk00l models report one less than they should */
			if (!ctx->correct_count)
				count++;

			count -= ctx->mediaoffset;
		}

		if (ctx->media_count_new) {
			ATTR("marker-levels=%d\n", count * 100 / ctx->media_count_new);
			ATTR("marker-message=\"%d native prints remaining on '%s' ribbon\"\n", count, dnpds40_media_types(ctx->media));
		}

		/* See if we can rewind to save media */
		if (ctx->can_rewind && ctx->supports_rewind) {
			/* Tell printer to use rewind */
			ctx->multicut += 400;

			/* Get Media remaining */
			dnpds40_build_cmd(&cmd, "INFO", "RQTY", 0);

			resp = dnpds40_resp_cmd(ctx, &cmd, &len);
			if (!resp)
				return CUPS_BACKEND_FAILED;

			dnpds40_cleanup_string((char*)resp, len);
			count = atoi((char*)resp+4);
			free(resp);
		}

		/* Verify we have sufficient media for prints */

#if 0 // disabled this to allow error to be reported on the printer panel
		if (count < 1) {
			ERROR("Printer out of media, please correct!\n");
			return CUPS_BACKEND_STOP;
		}
#endif

		if (count < copies) {
			WARNING("Printer does not have sufficient remaining media (%d) to complete job (%d)\n", copies, count);
		}
	}

	/* Store our last multicut state */
	ctx->last_multicut = ctx->multicut;

	/* Tell printer how many copies to make */
	snprintf(buf, sizeof(buf), "%07d\r", ctx->manual_copies ? 1 : copies);
	dnpds40_build_cmd(&cmd, "CNTRL", "QTY", 8);
	if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
		return CUPS_BACKEND_FAILED;

	if (!ctx->manual_copies)
		copies = 1;

	/* Enable job resumption on correctable errors */
	if (ctx->supports_matte) {
		snprintf(buf, sizeof(buf), "%08d", 1);
		/* DS80D does not support BUFFCNTRL when using
		   cut media; all others support this */
		if (ctx->type != P_DNP_DS80D ||
		    ctx->multicut < 100) {
			dnpds40_build_cmd(&cmd, "CNTRL", "BUFFCNTRL", 8);
			if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
				return CUPS_BACKEND_FAILED;
		}
	}

	/* Set overcoat parameters if appropriate */
	if (ctx->supports_matte) {
		snprintf(buf, sizeof(buf), "%08d", ctx->matte);
		dnpds40_build_cmd(&cmd, "CNTRL", "OVERCOAT", 8);
		if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
			return CUPS_BACKEND_FAILED;
	}

	/* Program in the cutter setting */
	if (ctx->cutter) {
		snprintf(buf, sizeof(buf), "%08d", ctx->cutter);
		dnpds40_build_cmd(&cmd, "CNTRL", "CUTTER", 8);
		if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
			return CUPS_BACKEND_FAILED;
	}

	/* Send over the printspeed if appropriate */
	if (ctx->supports_printspeed) {
		snprintf(buf, sizeof(buf), "%08d", ctx->printspeed * 10);
		dnpds40_build_cmd(&cmd, "CNTRL", "PRINTSPEED", 8);
		if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
			return CUPS_BACKEND_FAILED;
	}

	/* Program in the multicut setting */
	snprintf(buf, sizeof(buf), "%08u", ctx->multicut);
	dnpds40_build_cmd(&cmd, "IMAGE", "MULTICUT", 8);
	if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
		return CUPS_BACKEND_FAILED;

	/* Finally, send the stream over as individual data chunks */
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
	sleep(1);  /* Give things a moment */

	if (fast_return) {
		INFO("Fast return mode enabled.\n");
	} else {
		INFO("Waiting for job to complete...\n");

		while (1) {
			/* Query status */
			dnpds40_build_cmd(&cmd, "STATUS", "", 0);
			resp = dnpds40_resp_cmd(ctx, &cmd, &len);
			if (!resp)
				return CUPS_BACKEND_FAILED;
			dnpds40_cleanup_string((char*)resp, len);
			status = atoi((char*)resp);
			free(resp);

			/* If we're idle or there's an error..*/
			if (status == 0)
				break;
			if (status >= 1000) {
				ERROR("Printer encountered error: %s\n", dnpds40_statuses(status));
				break;
			}
			sleep(1);
		}

		/* Figure out remaining native prints */
		dnpds40_build_cmd(&cmd, "INFO", "MQTY", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		count = atoi((char*)resp+4);
		free(resp);

		if (count) {
			/* Old-sk00l models report one less than they should */
			if (!ctx->correct_count)
				count++;

			count -= ctx->mediaoffset;
		}

		if (ctx->media_count_new) {
			ATTR("marker-levels=%d\n", count * 100 / ctx->media_count_new);
			ATTR("marker-message=\"%d native prints remaining on '%s' ribbon\"\n", count, dnpds40_media_types(ctx->media));
		}
	}

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		/* No need to wait on buffers due to matte switching */
		buf_needed = ctx->buf_needed;
		goto top;
	}

	/* Finally, account for overcoat mode of last print */
	ctx->last_matte = !!ctx->matte;
#ifdef STATE_DIR
	{
		/* Store last matte status into file */
		char buf[64];
		FILE *f;
		snprintf(buf, sizeof(buf), STATE_DIR "/%s-last", ctx->serno);
		f = fopen(buf, "w");
		if (f) {
			fprintf(f, "%08d", ctx->last_matte);
			fclose(f);
		}
	}
#endif

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

	INFO("Model: %s\n", dnpds40_printer_type(ctx->type));

	/* Serial number already queried */
	INFO("Serial Number: %s\n", ctx->serno);

	/* Firmware version already queried */
	INFO("Firmware Version: %s\n", ctx->version);

	/* Figure out Duplexer */
	if (ctx->type == P_DNP_DS80D) {
		dnpds40_build_cmd(&cmd, "INFO", "UNIT_FVER", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Duplexer Version: %s\n", resp);

		free(resp);
	}

	/* Get Media Color offset */
	dnpds40_build_cmd(&cmd, "INFO", "MCOLOR", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media Color Offset: Y %u M %u C %u L %u\n", *(resp+2), *(resp+3),
	     *(resp+4), *(resp+5));

	free(resp);

	/* Get Media Class */
	dnpds40_build_cmd(&cmd, "INFO", "MEDIA_CLASS", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media Class: %d\n", atoi((char*)resp + 4));

	free(resp);

	/* Get Media Lot */
	dnpds40_build_cmd(&cmd, "INFO", "MLOT", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media Lot Code: %s\n", (char*)resp+2);
	free(resp);

	/* Get Media ID Set (?) */
	dnpds40_build_cmd(&cmd, "MNT_RD", "MEDIA_ID_SET", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Media ID: %d\n", atoi((char*)resp+4));

	free(resp);

	/* Get Ribbon ID code (?) */
	dnpds40_build_cmd(&cmd, "MNT_RD", "RIBBON_ID_CODE", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Ribbon ID: %s\n", (char*)resp);

	free(resp);

	/* Figure out control data and checksums */

	/* 300 DPI */
	dnpds40_build_cmd(&cmd, "TBL_RD", "CWD300_Version", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("300 DPI Color Data: %s ", (char*)resp);

	free(resp);

	dnpds40_build_cmd(&cmd, "TBL_RD", "CWD300_Checksum", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	DEBUG2("(%s)\n", (char*)resp);

	free(resp);

	/* 600 DPI */
	dnpds40_build_cmd(&cmd, "TBL_RD", "CWD600_Version", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("600 DPI Color Data: %s ", (char*)resp);

	free(resp);

	dnpds40_build_cmd(&cmd, "TBL_RD", "CWD600_Checksum", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	DEBUG2("(%s)\n", (char*)resp);

	free(resp);

	if (ctx->supports_lowspeed) {
		/* "Low Speed" */
		dnpds40_build_cmd(&cmd, "TBL_RD", "CWD610_Version", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Low Speed Color Data: %s ", (char*)resp);

		free(resp);

		dnpds40_build_cmd(&cmd, "TBL_RD", "CWD610_Checksum", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		DEBUG2("(%s)\n", (char*)resp);

		free(resp);
	}
	if (ctx->supports_highdensity) {
		uint8_t buf[5];
		int i = 0;

		snprintf((char*)buf, sizeof(buf), "%04d", i);

		/* "High Density" */
		dnpds40_build_cmd(&cmd, "TBL_RD", "CWD620_Version", 4);

		resp = dnpds40_resp_cmd2(ctx, &cmd, &len, buf, 4);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("High Density Color Data: %s ", (char*)resp);

		free(resp);

		dnpds40_build_cmd(&cmd, "TBL_RD", "CWD620_Checksum", 4);

		resp = dnpds40_resp_cmd2(ctx, &cmd, &len, buf, 4);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		DEBUG2("(%s)\n", (char*)resp);

		free(resp);
	}
	if (ctx->supports_gamma) {
		/* "Low Speed" */
		dnpds40_build_cmd(&cmd, "TBL_RD", "CTRLD_GAMMA16", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Gamma Correction Data Checksum: %s\n", (char*)resp);

		free(resp);
	}

	if (ctx->supports_standby) {
		/* Get Standby stuff */
		int i;

		dnpds40_build_cmd(&cmd, "MNT_RD", "STANDBY_TIME", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);
		i = atoi((char*)resp);
			
		INFO("Standby Transition time: %d minutes\n", i);

		free(resp);

		/* Get Media End Keep */
		dnpds40_build_cmd(&cmd, "MNT_RD", "END_KEEP_MODE", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);
		i = atoi((char*)resp);
		INFO("Media End kept across power cycles: %s\n",
		     i ? "Yes" : "No");		     

		free(resp);
	}

	if (ctx->supports_iserial) {
		int i;
		/* Get USB serial descriptor status */
		dnpds40_build_cmd(&cmd, "MNT_RD", "USB_ISERI_SET", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);
		i = atoi((char*)resp);

		INFO("Report Serial Number in USB descriptor: %s\n",
		     i ? "Yes" : "No");

		free(resp);
	}

	return CUPS_BACKEND_OK;
}

static int dnpds40_get_status(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;
	int count;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "STATUS", "", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);
	count = atoi((char*)resp);

	INFO("Printer Status: %s (%d)\n", dnpds40_statuses(count), count);

	free(resp);

	/* Figure out Duplexer */
	if (ctx->type == P_DNP_DS80D) {
		dnpds40_build_cmd(&cmd, "INFO", "UNIT_STATUS", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);
		count = atoi((char*)resp);

		INFO("Duplexer Status: %s\n", dnpds80_duplex_statuses(count));

		free(resp);
	}

	/* Get remaining print quantity */
	dnpds40_build_cmd(&cmd, "INFO", "PQTY", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Prints remaining in job: %d\n", atoi((char*)resp + 4));

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "INFO", "FREE_PBUFFER", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Free Buffers: %d\n", atoi((char*)resp + 3));

	free(resp);

	/* Report media */
	INFO("Media Type: %s\n", dnpds40_media_types(ctx->media));

	if (ctx->supports_media_ext) {
		int type;
		dnpds40_build_cmd(&cmd, "INFO", "MEDIA_EXT_CODE", 0);
		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);
		*(resp+2) = 0;  // Only the first two chars are used.
		type = atoi((char*)resp);
		INFO("Media Code: %s\n", dnpds620_media_extension_code(type));
		free(resp);
	}

	/* Try to figure out media subtype */
	if (ctx->type == P_DNP_DS820) {
		int type;
		dnpds40_build_cmd(&cmd, "INFO", "MEDIA_CLASS_RFID", 0);
		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);
		type = atoi((char*)resp);
		INFO("Media Subtype: %s\n", dnpds820_media_subtypes(type));
		free(resp);
	}

	/* Report Cut Media */
	if (ctx->type == P_DNP_DS80D)
		INFO("Duplex Media Type: %s\n", dnpds80_duplex_media_types(ctx->media));

	if (ctx->media_count_new)
		INFO("Native Prints Available on New Media: %u\n", ctx->media_count_new);

	/* Get Media remaining */
	dnpds40_build_cmd(&cmd, "INFO", "MQTY", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	count = atoi((char*)resp+4);
	free(resp);

	if (count) {
		/* Old-sk00l models report one less than they should */
		if (!ctx->correct_count)
			count++;

		count -= ctx->mediaoffset;
	}
	INFO("Native Prints Remaining on Media: %d\n", count);

	if (ctx->supports_rewind) {
		/* Get Media remaining */
		dnpds40_build_cmd(&cmd, "INFO", "RQTY", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		count = atoi((char*)resp+4);
		free(resp);
	} else {
		// Do nothing, re-use native print count.
	}
	INFO("Half-Size Prints Remaining on Media: %d\n", count);

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

	INFO("Lifetime Counter: %d\n", atoi((char*)resp+2));

	free(resp);

	if (ctx->type == P_DNP_DS620 ||
	    ctx->type == P_DNP_DS820) {
		/* Generate command */
		dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_HEAD", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Head Counter: %d\n", atoi((char*)resp+2));

		free(resp);
	}

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_A", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("A Counter: %d\n", atoi((char*)resp+2));

	free(resp);

	/* Generate command */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_B", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("B Counter: %d\n", atoi((char*)resp+2));

	free(resp);

	if (ctx->supports_counterp) {
		/* Generate command */
		dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_P", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("P Counter: %d\n", atoi((char*)resp+2));

		free(resp);
	}

	if (ctx->supports_matte) {
		/* Generate command */
		dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_M", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("M Counter: %d\n", atoi((char*)resp+2));

		free(resp);

		/* Generate command */
		dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_MATTE", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Matte Counter: %d\n", atoi((char*)resp+4));

		free(resp);
	}

	if (ctx->type == P_DNP_DS80D) {
		dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_DUPLEX", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Duplexer Counter: %d\n", atoi((char*)resp));

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

static int dnpds40_cancel_job(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "CNTRL", "CANCEL", 0);

	if ((ret = dnpds40_do_cmd(ctx, &cmd, NULL, 0)))
		return ret;

	return 0;
}

static int dnpds40_reset_printer(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "CNTRL", "PRINTER_RESET", 0);

	if ((ret = dnpds40_do_cmd(ctx, &cmd, NULL, 0)))
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
	DEBUG("\t\t[ -k num ]       # Set standby time (1-99 minutes, 0 disables)\n");
	DEBUG("\t\t[ -K num ]       # Keep Media Status Across Power Cycles (1 on, 0 off)\n");
	DEBUG("\t\t[ -n ]           # Query counters\n");
	DEBUG("\t\t[ -N A|B|M ]     # Clear counter A/B/M\n");
	DEBUG("\t\t[ -p num ]       # Set counter P\n");
	DEBUG("\t\t[ -R ]           # Reset printer\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -x num ]       # Set USB iSerialNumber Reporting (1 on, 0 off)\n");
	DEBUG("\t\t[ -X ]           # Cancel current print job\n");
}

static int dnpds40_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct dnpds40_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "iIk:K:nN:p:Rsx:X")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'i':
			j = dnpds40_get_info(ctx);
			break;
		case 'I':
			j = dnpds40_get_sensors(ctx);
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
			if (!ctx->supports_counterp) {
				ERROR("Printer FW dows not support P counter!\n");
				return CUPS_BACKEND_FAILED;
			}
			j = dnpds40_set_counter_p(ctx, optarg);
			break;
		case 'R': {
			j = dnpds40_reset_printer(ctx);
			break;
		}
		case 's': {
			j = dnpds40_get_status(ctx);
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
		case 'X': {
			j = dnpds40_cancel_job(ctx);
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
	.version = "0.91",
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
	{ USB_VID_CITIZEN, USB_PID_DNP_DS620_OLD, P_DNP_DS620, ""},
	{ USB_VID_DNP, USB_PID_DNP_DS620, P_DNP_DS620, ""},
	{ USB_VID_DNP, USB_PID_DNP_DS80D, P_DNP_DS80D, ""},
	{ USB_VID_CITIZEN, USB_PID_CITIZEN_CW02, P_DNP_DS40, ""},
	{ USB_VID_DNP, USB_PID_DNP_DS820, P_DNP_DS820, ""},
	{ 0, 0, 0, ""}
	}
};
