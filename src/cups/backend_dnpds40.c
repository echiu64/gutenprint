/*
 *   Citizen / DNP Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2021 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Development of this backend was sponsored by:
 *
 *     Marco Di Antonio and [ ilgruppodigitale.com ]
 *     LiveLink Technology [ www.livelinktechnology.net ]
 *     A generous benefactor who wishes to remain anonymous
 *
 *   The latest version of this program can be found at:
 *
 *     https://git.shaftnet.org/cgit/selphy_print.git
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 *   SPDX-License-Identifier: GPL-2.0+
 *
 */

//#define DNP_ONLY
//#define CITIZEN_ONLY

/* Enables caching of last print type to speed up
   job pipelining.  Without this we always have to
   assume the worst */
//#define STATE_DIR "/tmp"

#define BACKEND dnpds40_backend

#include "backend_common.h"

#include <time.h>

/* Private data structure */
struct dnpds40_printjob {
	struct dyesub_job_common common;

	uint8_t *databuf;
	int datalen;

	uint32_t dpi;
	int matte;
	int cutter;
	uint32_t multicut;
	int fullcut;
	int printspeed;
	int can_rewind;

	int buf_needed;
	int cut_paper;
};

#define MFG_DNP 0
#define MFG_CITIZEN 1
#define MFG_MITSUBISHI 2
#define MFG_OTHER 3

struct dnpds40_ctx {
	struct dyesub_connection *conn;

	int mfg; /* see MFG_* */

	/* Version and whatnot */
	char *serno;
	char *version;
	int ver_major;
	int ver_minor;

	/* State */
	uint32_t media;
	uint32_t media_subtype;

	char     media_text[32];
	uint32_t duplex_media;
	int      duplex_media_status;
	uint16_t media_count_new;

	uint32_t last_multicut;
	int last_matte;
	int partialmatte;

	int media_sticker;
	int mediaoffset;
	int correct_count;
	int needs_mlot;

	struct marker marker[2];
	int marker_count;

	/* Printer capabilities */
	uint32_t native_width;
	uint32_t max_height;
	int supports_600dpi;
	int supports_6x9;
	int supports_2x6;
	int supports_3x5x2;
	int supports_a4x6;
	int supports_matte;
	int supports_finematte;
	int supports_luster;
	int supports_advmatte;
	int supports_fullcut;
	int supports_rewind;
	int supports_standby;
	int supports_keepmode;
	int supports_6x4_5;
	int supports_mqty_default;
	int supports_iserial;
	int supports_6x6;
	int supports_5x5;
	int supports_counterp;
	int supports_adv_fullcut;
	int supports_mediaoffset;
	int supports_ctrld_ext;
	int supports_media_ext;
	int supports_printspeed;
	int supports_lowspeed;
	int supports_highdensity;
	int supports_systime;
	int supports_mediaclassrfid;
	int supports_gamma;
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

#define MULTICUT_4x4      47
#define MULTICUT_4x6      48
#define MULTICUT_4x8      49
#define MULTICUT_4_5x4_5  50
#define MULTICUT_4_5x6    51
#define MULTICUT_4_5x8    52

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

#ifndef min
#define min(__x, __y) ((__x) < (__y)) ? __x : __y
#endif

/* Legacy spool file support */
static int legacy_cw01_read_parse(struct dnpds40_printjob *job, int data_fd, int read_data);
static int legacy_dnp_read_parse(struct dnpds40_printjob *job, int data_fd, int read_data);
static int legacy_dnp620_read_parse(struct dnpds40_printjob *job, int data_fd, int read_data);
static int legacy_dnp820_read_parse(struct dnpds40_printjob *job, int data_fd, int read_data);
static int legacy_qw410_read_parse(struct dnpds40_printjob *job, int data_fd, int read_data);

static void dnpds40_cleanup_job(const void *vjob);
static int dnpds40_query_markers(void *vctx, struct marker **markers, int *count);

#define JOB_EQUIV(__x)  if (job1->__x != job2->__x) goto done

/* NOTE:  Does _not_ free the input jobs */
static void *dnp_combine_jobs(const void *vjob1,
			      const void *vjob2)
{
	const struct dnpds40_printjob *job1 = vjob1;
	const struct dnpds40_printjob *job2 = vjob2;
	struct dnpds40_printjob *newjob = NULL;
	uint32_t new_multicut;
	uint16_t new_w, new_h;
	int32_t gap_bytes;

	/* Sanity check */
	if (!job1 || !job2)
		goto done;

	/* Make sure pertinent paremeters are the same */
	JOB_EQUIV(dpi);
	JOB_EQUIV(matte);
	JOB_EQUIV(cutter);
	JOB_EQUIV(fullcut);
	JOB_EQUIV(multicut);  // TODO:  Support fancier modes for 8" models (eg 8x4+8x6, etc)
	JOB_EQUIV(datalen); // <-- cheating a little?
	// JOV_EQUIV(printspeed); <-- does it matter?

	/* Any fancy cutter action means we pass */
	if (job1->fullcut || job1->cutter > 120)
		goto done;
	/* Partial matte is no bueno */
	if (job1->matte > 100 ||
	    job2->matte > 100)
		goto done;

	/* Make sure we can combine these two prints */
	switch (job1->multicut) {
	case MULTICUT_5x3_5:
		new_multicut = MULTICUT_5x3_5X2;
		new_w = 1920;
		new_h = 2176;
		gap_bytes = 0;
		break;
	case MULTICUT_6x4:
		if (job1->cutter == 120) {
			new_multicut = MULTICUT_6x8;
			new_h = 2436;
			gap_bytes = -44;  /* Chop out the middle 44 rows */
		} else {
			new_multicut = MULTICUT_6x4X2;
			new_h = 2498;
			gap_bytes = 18;
		}
		new_w = 1920;
		break;
	case MULTICUT_6x4_5:
		new_multicut = MULTICUT_6x4_5X2;
		new_w = 1920;
		new_h = 2802;
		gap_bytes = 30;
		break;
	case MULTICUT_8x4:
		new_multicut = MULTICUT_8x4X2;
		new_w = 2560;
		new_h = 2502;
		gap_bytes = 30;
		break;
	case MULTICUT_8x5:
		new_multicut = MULTICUT_8x5X2;
		new_w = 2560;
		new_h = 3102;
		gap_bytes = 30;
		break;
	case MULTICUT_A4x5:
		new_multicut = MULTICUT_A4x5X2;
		new_w = 2560;
		new_h = 3102;
		gap_bytes = 30;
		break;
	case MULTICUT_A5:
		new_multicut = MULTICUT_A5X2;
		new_w = 2560;
		new_h = 3598;
		gap_bytes = 30;
		break;
	case MULTICUT_8x6:
		new_multicut = MULTICUT_8x6X2;
		new_w = 2560;
		new_h = 3702;
		gap_bytes = 30;
		break;
	default:
		/* Everything else is NOT handled */
		goto done;
	}
	gap_bytes *= new_w;
	if (job1->dpi == 600) {
		gap_bytes *= 2;
		new_h *= 2;
	}

	DEBUG("Combining jobs to save media\n");

	/* Okay, it's kosher to proceed */

	newjob = malloc(sizeof(*newjob));
	if (!newjob) {
		ERROR("Memory allocation failure!\n");
		goto done;
	}
	memcpy(newjob, job1, sizeof(*newjob));

	newjob->databuf = malloc(((new_w*new_h+1024+54+10))*3+1024 + abs(gap_bytes));
	newjob->datalen = 0;
	newjob->multicut = new_multicut;
	newjob->can_rewind = 0;
	newjob->common.can_combine = 0;
	if (!newjob->databuf) {
		dnpds40_cleanup_job(newjob);
		newjob = NULL;
		ERROR("Memory allocation failure!\n");
		goto done;
	}

	/* Copy data blocks from job1 */
	uint8_t *ptr, *ptr2;
	char buf[9];
	ptr = job1->databuf;
	while(ptr && ptr < (job1->databuf + job1->datalen)) {
		int i;
		buf[8] = 0;
		memcpy(buf, ptr + 24, 8);
		i = atoi(buf) + 32;
		memcpy(newjob->databuf + newjob->datalen, ptr, i);

		/* If we're on a plane data block... */
		if (!memcmp("PLANE", newjob->databuf + newjob->datalen + 9, 5)) {
			long planelen = (new_w * new_h) + 1088;
			uint32_t newlen;

			/* Fix up length in command */
			snprintf(buf, sizeof(buf), "%08ld", planelen);
			memcpy(newjob->databuf + newjob->datalen + 24, buf, 8);

			/* Alter BMP header */
			newlen = cpu_to_le32(planelen);
			memcpy(newjob->databuf + newjob->datalen + 32 + 2, &newlen, 4);

			/* alter DIB header */
			newlen = cpu_to_le32(new_h);
			memcpy(newjob->databuf + newjob->datalen + 32 + 22, &newlen, 4);

			if (gap_bytes > 0) {
				/* Insert gap/padding after first image */
				memset(newjob->databuf + newjob->datalen + i, 0xff, gap_bytes);
				newjob->datalen += gap_bytes;
			} else {
//				uint8_t *ptrA = newjob->databuf + newjob->datalen + 1088;
//				/* Back off by 1/2 the gap */
//				memmove(ptrA, ptrA - (gap_bytes / 2), (i - 1088) + gap_bytes/2);
				/* And chop the end off by half the gap */
				newjob->datalen += gap_bytes / 2;
			}

			/* Locate job2's PLANE -- Assume it's in the same place! */
			ptr2 = job2->databuf + (ptr - job1->databuf);

			/* Copy over job2's image data */
			memcpy(newjob->databuf + newjob->datalen + i,
			        ptr2 + 32 + 1088, i - 32 - 1088);

			if (gap_bytes < 0) {
				uint8_t *ptrA = newjob->databuf + newjob->datalen + i;
				/* Back off by 1/2 the gap */
				memmove(ptrA, ptrA - (gap_bytes / 2), (i - 1088) + gap_bytes/2);
				/* And chop the end off by half the gap */
				newjob->datalen += gap_bytes / 2;
			}
			newjob->datalen += i - 32 - 1088;  /* add in job2 length */
		}

		newjob->datalen += i;
		ptr += i;
	}

done:
	return newjob;
}

#undef JOB_EQUIV

static void dnpds40_build_cmd(struct dnpds40_cmd *cmd, const char *arg1, const char *arg2, uint32_t arg3_len)
{
	memset(cmd, 0x20, sizeof(*cmd));
	cmd->esc = 0x1b;
	cmd->p = 0x50;
	memcpy(cmd->arg1, arg1, min(strlen(arg1), sizeof(cmd->arg1)));
	memcpy(cmd->arg2, arg2, min(strlen(arg2), sizeof(cmd->arg2)));
	if (arg3_len) {
		char buf[11];  /* Extra padding to shut up GCC 10 */
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

static const char *dnpds40_printer_type(int type, int mfg)
{
	switch(type) {
	case P_DNP_DS40: return mfg == MFG_CITIZEN? "CX" : "DS40";
	case P_DNP_DS80: return mfg == MFG_CITIZEN? "CW" : (mfg == MFG_MITSUBISHI ? "CP3800" : "DS80");
	case P_DNP_DS80D: return "DS80DX";
	case P_DNP_DSRX1: return mfg == MFG_CITIZEN ? "CY" : "DSRX1";
	case P_DNP_DS620: return mfg == MFG_CITIZEN ? "CX-02" : "DS620";
	case P_DNP_DS820: return mfg == MFG_CITIZEN ? "CX-02W" : "DS820";
	case P_DNP_QW410: return mfg == MFG_CITIZEN ? "CZ-01" : "QW410";
	case P_CITIZEN_CW01: return "CW01";
	case P_CITIZEN_OP900II: return "CW-02 / OP900ii";
	default: break;
	}
	return "Unknown";
}

static const char *dnpds40_media_types(int media, int sticker)
{
	switch (media) {
	case 100: return "UNKNOWN100"; // seen in driver dumps
	case 110: return "UNKNOWN110"; // seen in driver dumps
	case 150: return "4x6 (PC)";
	case 151: return "4x8";
	case 160: return "4.5x6";
	case 161: return "4.5x8";
	case 200: return sticker ? "5x3.5 (L): Sticker" : "5x3.5 (L)";
	case 210: return sticker ? "5x7 (2L) Sticker" : "5x7 (2L)";
	case 300: return sticker ? "6x4 (PC) Sticker" : "6x4 (PC)";
	case 310: return sticker ? "6x8 (A5) Sticker" : "6x8 (A5)";
	case 400: return sticker ? "6x9 (A5W) Sticker" : "6x9 (A5W)";
	case 500: return "8x10";
	case 510: return "8x12";
	case 600: return "A4";
	default:
		break;
	}

	return "Unknown";
}

static const char *dnpds620_media_extension_code(int media)
{
	switch (media) {
	case  0: return "Normal Paper";
	case  1: return "Sticker Paper";
	case 99: return "Unknown Paper";
	default:
		break;
	}

	return "Unknown";
}

static const char *rfid_media_subtypes(int media)
{
	switch (media) {
	case 1: return "SD";
	case 2: return "PD";
	case 3: return "PP";
	default:
		break;
	}

	return "Unknown";
}

static const char *dnpds80_duplex_media_types(int media)
{
	switch (media) {
	case 100: return "8x10.75";
	case 200: return "8x12";
	default:
		break;
	}

	return "Unknown";
}

#define DUPLEX_UNIT_PAPER_NONE 0
#define DUPLEX_UNIT_PAPER_PROTECTIVE 1
#define DUPLEX_UNIT_PAPER_PRESENT 2

static const char *dnpds80_duplex_paper_status(int media)
{
	switch (media) {
	case DUPLEX_UNIT_PAPER_NONE: return "No Paper";
	case DUPLEX_UNIT_PAPER_PROTECTIVE: return "Protective Sheet";
	case DUPLEX_UNIT_PAPER_PRESENT: return "Cut Paper Present";
	default:
		return "Unknown";
	}
}

static const char *dnpds80_duplex_statuses(int status)
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

	return "Unknown Duplexer Error";
}

static const char *dnpds40_statuses(int status)
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
	case 2010: return "USB Power Supply Error";
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
	case 9999: return "Communication Failure"; /* Special */
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

	if ((ret = send_data(ctx->conn,
			     (uint8_t*)cmd, sizeof(*cmd))))
		return ret;

	if (data && len)
		if ((ret = send_data(ctx->conn,
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
	ret = read_data(ctx->conn,
			(uint8_t*)tmp, 8, &num);
	if (ret < 0)
		return NULL;

	if (num != 8) {
		ERROR("Short read! (%d/%d)\n", num, 8);
		return NULL;
	}

	i = atoi(tmp);  /* Length of payload in bytes, possibly padded */
	respbuf = malloc(i + 1);
	if (!respbuf) {
		ERROR("Memory allocation failure (%d bytes)!\n", i);
		return NULL;
	}
	respbuf[i] = 0; /* Explicitly null-pad */

	/* Read in the actual response */
	ret = read_data(ctx->conn,
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

static int dnpds40_query_serno(struct dyesub_connection *conn, char *buf, int buf_len)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;

	struct dnpds40_ctx ctx = {
		.conn = conn,
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

	return ctx;
}

#define FW_VER_CHECK(__major, __minor) \
	((ctx->ver_major > (__major)) || \
	 (ctx->ver_major == (__major) && ctx->ver_minor >= (__minor)))

static int dnpds40_query_mqty(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0, count;

	/* Get Media remaining */
	dnpds40_build_cmd(&cmd, "INFO", "MQTY", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return -1;

	dnpds40_cleanup_string((char*)resp, len);

	count = atoi((char*)resp+4);
	free(resp);

	if (count) {
		/* Old-sk00l models report one less than they should */
		if (!ctx->correct_count)
			count++;

		count -= ctx->mediaoffset;
		if (count < 0) /* Just in case */
			count = 0;
	}

	return count;
}

static int dnpds80dx_query_paper(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;

	/* Query Duplex Media Info */
	dnpds40_build_cmd(&cmd, "INFO", "UNIT_CUT_PAPER", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (resp) {
		char tmp[5];
		char status;

		dnpds40_cleanup_string((char*)resp, len);

		memcpy(tmp, resp + 4, 4);
		status = tmp[3];
		tmp[3] = '0';
		tmp[4] = 0;

		ctx->duplex_media = atoi(tmp);

		tmp[0] = tmp[1] = tmp[2] = '0';
		tmp[3] = status;
		ctx->duplex_media_status = atoi(tmp);

		free(resp);
	} else {
		return CUPS_BACKEND_FAILED;
	}

	return CUPS_BACKEND_OK;
}

static int dnpds40_attach(void *vctx, struct dyesub_connection *conn, uint8_t jobid)
{
	struct dnpds40_ctx *ctx = vctx;

	UNUSED(jobid);

	ctx->conn = conn;

	/* Nearly all models support 600dpi */
	ctx->supports_600dpi = 1;

	if (test_mode < TEST_MODE_NOATTACH) {
		struct dnpds40_cmd cmd;
		uint8_t *resp;
		int len = 0;

		/* Get Firmware Version */
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
		} else {
			return CUPS_BACKEND_FAILED;
		}

		/* Figure out actual Manufacturer */
		{
			struct libusb_device_descriptor desc;
			struct libusb_device *udev;

			udev = libusb_get_device(ctx->conn->dev);
			libusb_get_device_descriptor(udev, &desc);

			char buf[STR_LEN_MAX + 1];
			buf[0] = 0;
			buf[STR_LEN_MAX] = 0;
			libusb_get_string_descriptor_ascii(ctx->conn->dev, desc.iManufacturer, (unsigned char*)buf, STR_LEN_MAX);

			if (!strncmp(buf, "Dai", 3)) /* "Dai Nippon Printing" */
				ctx->mfg = MFG_DNP;
			else if (!strncmp(buf, "CIT", 3)) /* "CITIZEN SYSTEMS" */
				ctx->mfg = MFG_CITIZEN;
			else if (!strncmp(buf, "M", 1)) /* "Mitsubishi" */
				ctx->mfg = MFG_MITSUBISHI;
			else
				ctx->mfg = MFG_OTHER;

#ifdef DNP_ONLY  /* Only allow DNP printers to work. */
			if (ctx->mfg != MFG_DNP)
				return CUPS_BACKEND_FAILED;
#endif
#ifdef CITIZEN_ONLY   /* Only allow CITIZEN printers to work. */
			if (ctx->mfg != MFG_CITIZEN)
				return CUPS_BACKEND_FAILED
#endif
		}
	} else {
		ctx->ver_major = 3;
		ctx->ver_minor = 0;
		ctx->version = strdup("UNKNOWN");
	}

	/* Per-printer options */
	switch (ctx->conn->type) {
	case P_DNP_DS40:
		ctx->native_width = 1920;
		ctx->max_height = 5480;
		ctx->supports_6x9 = 1;
		if (FW_VER_CHECK(1,04))
			ctx->supports_counterp = 1;
		if (FW_VER_CHECK(1,30))
			ctx->supports_matte = 1;
		if (FW_VER_CHECK(1,40))
			ctx->supports_2x6 = 1;
		if (FW_VER_CHECK(1,50))
			ctx->supports_3x5x2 = 1;
		if (FW_VER_CHECK(1,60))
			ctx->supports_fullcut = ctx->supports_6x6 = 1; // No 5x5!
		break;
	case P_DNP_DS80:
	case P_DNP_DS80D:
		ctx->native_width = 2560;
		ctx->max_height = 7536;
		if (FW_VER_CHECK(1,02))
			ctx->supports_counterp = 1;
		if (FW_VER_CHECK(1,30))
			ctx->supports_matte = 1;
		break;
	case P_DNP_DSRX1:
		ctx->native_width = 1920;
		ctx->max_height = 5480;
		ctx->supports_counterp = 1;
		ctx->supports_matte = 1;
		if (FW_VER_CHECK(1,10))
			ctx->supports_2x6 = ctx->supports_mqty_default = 1;
                if (FW_VER_CHECK(1,20))
			ctx->supports_3x5x2 = 1;
		if (FW_VER_CHECK(2,00)) { /* AKA RX1HS */
			ctx->needs_mlot = 1;
			ctx->supports_mediaoffset = 1;
			ctx->supports_iserial = 1;
		}
		if (FW_VER_CHECK(2,06)) {
			ctx->supports_5x5 = ctx->supports_6x6 = 1;
		}
		break;
	case P_CITIZEN_OP900II:
		ctx->native_width = 1920;
		ctx->max_height = 5480;
		ctx->supports_counterp = 1;
		ctx->supports_matte = 1;
		ctx->supports_mqty_default = 1;
		ctx->supports_6x9 = 1;
		if (FW_VER_CHECK(1,11))
			ctx->supports_2x6 = 1;
		break;
	case P_CITIZEN_CW01:
		ctx->native_width = 2048;
		ctx->max_height = 5480;
		ctx->supports_6x9 = 1;
		break;
	case P_DNP_DS620:
		ctx->native_width = 1920;
		ctx->max_height = 5604;
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
		ctx->supports_keepmode = 1;
		ctx->supports_iserial = 1;
		ctx->supports_6x6 = 1;
		ctx->supports_5x5 = 1;
		ctx->supports_lowspeed = 1;
		ctx->supports_3x5x2 = 1;

		if (ctx->mfg == MFG_CITIZEN) { /* Citizen and DNP firmware diverge */
			ctx->supports_adv_fullcut = 1;
			ctx->supports_advmatte = 1;
			ctx->supports_luster = 1;

			if (FW_VER_CHECK(1,01))
				ctx->supports_finematte = 1;
			if (FW_VER_CHECK(1,10))
				ctx->supports_6x9 = ctx->supports_6x4_5 = 1;
		} else {
			if (FW_VER_CHECK(1,10))
				ctx->supports_6x9 = ctx->supports_6x4_5 = 1;
			if (FW_VER_CHECK(1,20))
				ctx->supports_adv_fullcut = ctx->supports_advmatte = 1;
			if (FW_VER_CHECK(1,30))
				ctx->supports_luster = 1;
			if (FW_VER_CHECK(1,33))
				ctx->supports_media_ext = 1;
			if (FW_VER_CHECK(1,52))
				ctx->supports_finematte = 1;
		}
		break;
	case P_DNP_DS820:
		ctx->native_width = 2560;
		ctx->max_height = 7536;
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
		ctx->supports_keepmode = 1;
		ctx->supports_iserial = 1;
		ctx->supports_adv_fullcut = 1;
		ctx->supports_advmatte = 1;
		ctx->supports_luster = 1;
		ctx->supports_finematte = 1;
		ctx->supports_printspeed = 1;
		ctx->supports_lowspeed = 1;
		ctx->supports_highdensity = 1;
		ctx->supports_ctrld_ext = 1;
		ctx->supports_mediaoffset = 1;
		ctx->supports_mediaclassrfid = 1;
		ctx->supports_gamma = 1;

		if (ctx->mfg == MFG_CITIZEN) { /* Citizen and DNP firmware diverge */
			ctx->supports_a4x6 = 1; // XXX need to confirm this!
		} else {
			if (FW_VER_CHECK(1,06))
				ctx->supports_a4x6 = 1;
		}
		break;
	case P_DNP_QW410:
		ctx->native_width = 1408;
		ctx->max_height = 2436;
		ctx->correct_count = 1;
		ctx->supports_600dpi = 0;
		ctx->supports_counterp = 1;
		ctx->supports_matte = 1;
		ctx->supports_fullcut = 1;
		ctx->supports_mqty_default = 1;
		ctx->supports_keepmode = 1;
		ctx->supports_iserial = 1;
		ctx->supports_adv_fullcut = 1;
		ctx->supports_advmatte = 1;
		ctx->supports_printspeed = 1;
		ctx->supports_lowspeed = 1;
		ctx->supports_ctrld_ext = 1;
		ctx->supports_mediaoffset = 1;
		ctx->supports_mediaclassrfid = 1;
		ctx->supports_systime = 1;
		break;
	default:
		ERROR("Unknown printer type %d\n", ctx->conn->type);
		return CUPS_BACKEND_FAILED;
	}

	if (test_mode < TEST_MODE_NOATTACH) {
		struct dnpds40_cmd cmd;
		uint8_t *resp;
		int len = 0;

		/* Get Serial Number */
		dnpds40_build_cmd(&cmd, "INFO", "SERIAL_NUMBER", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (resp) {
			dnpds40_cleanup_string((char*)resp, len);
			ctx->serno = (char*) resp;
			/* Do NOT free resp! */
		} else {
			return CUPS_BACKEND_FAILED;
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

			if (ctx->conn->type != P_DNP_QW410) {
				/* Subtract out the "mark" type */
				if (ctx->media & 1)
					ctx->media--;
			}
			free(resp);
		} else {
			return CUPS_BACKEND_FAILED;
		}

		/* Try to figure out media subtype */
		if (ctx->supports_mediaclassrfid) {
			dnpds40_build_cmd(&cmd, "INFO", "MEDIA_CLASS_RFID", 0);
			resp = dnpds40_resp_cmd(ctx, &cmd, &len);
			if (!resp)
				return CUPS_BACKEND_FAILED;

			dnpds40_cleanup_string((char*)resp, len);
			ctx->media_subtype = atoi((char*)resp);
			free(resp);
		}

		/* And sticker */
		if (ctx->supports_media_ext) {
			int type;
			dnpds40_build_cmd(&cmd, "INFO", "MEDIA_EXT_CODE", 0);
			resp = dnpds40_resp_cmd(ctx, &cmd, &len);
			if (!resp)
				return CUPS_BACKEND_FAILED;

			dnpds40_cleanup_string((char*)resp, len);
			type = atoi((char*)resp+7);
			ctx->media_sticker = (type == 1);
			free(resp);
		}

		if (ctx->conn->type == P_DNP_DS80D) {
			if (dnpds80dx_query_paper(ctx))
				return CUPS_BACKEND_FAILED;
		}
	} else {
		switch(ctx->conn->type) {
		case P_DNP_DS80D:
			ctx->duplex_media = 200;
			/* Intentional fallthrough */
		case P_DNP_DS80:
		case P_DNP_DS820:
			ctx->media = 510; /* 8x12 */
			break;
		case P_DNP_DSRX1:
			ctx->media = 310; /* 6x8 */
			break;
		default:
			ctx->media = 400; /* 6x9 */
			break;
		}

		if (getenv("MEDIA_CODE"))
			ctx->media = atoi(getenv("MEDIA_CODE"));
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
	if (test_mode < TEST_MODE_NOATTACH && ctx->supports_mediaoffset) {
		/* Get Media Offset */
		struct dnpds40_cmd cmd;
		uint8_t *resp;
		int len = 0;

		dnpds40_build_cmd(&cmd, "INFO", "MEDIA_OFFSET", 0);
		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (resp) {
			ctx->mediaoffset = atoi((char*)resp+4);
			free(resp);
		} else {
			return CUPS_BACKEND_FAILED;
		}
	} else if (!ctx->correct_count) {
		ctx->mediaoffset = 50;
	}

	if (test_mode < TEST_MODE_NOATTACH && ctx->supports_mqty_default) {
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
		} else {
			return CUPS_BACKEND_FAILED;
		}
	} else {
		/* Look it up for legacy models & FW */
		switch (ctx->conn->type) {
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
				ctx->media_count_new = 0;
				break;
			}
			break;
		case P_DNP_DSRX1:
			switch (ctx->media) {
			case 210: // 2L
				ctx->media_count_new = 350;
				break;
			case 300: // PC
				ctx->media_count_new = 700;
				break;
			case 310: // A5
				ctx->media_count_new = 350;
				break;
			default:
				ctx->media_count_new = 0;
				break;
			}
			break;
		case P_CITIZEN_OP900II:
			switch (ctx->media) {
			case 210: // 2L
				ctx->media_count_new = 350;
				break;
			case 300: // PC
				ctx->media_count_new = 600;
				break;
			case 310: // A5
				ctx->media_count_new = 300;
				break;
			case 400: // A5W
				ctx->media_count_new = 280;
				break;
			default:
				ctx->media_count_new = 0;
				break;
			}
			break;
		case P_CITIZEN_CW01:
			switch (ctx->media) {
			case 300: // PC
				ctx->media_count_new = 600;
				break;
			case 350: // 2L
				ctx->media_count_new = 230;
				break;
			case 400: // A5W
				ctx->media_count_new = 280;
				break;
			default:
				ctx->media_count_new = 0;
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
				ctx->media_count_new = 0;
				break;
			}
			break;
		case P_DNP_DS620:
			switch (ctx->media) {
			case 200: // L
				ctx->media_count_new = 420;
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
				ctx->media_count_new = 0;
				break;
			}
			break;
		case P_DNP_DS820:
			ctx->media_subtype = 1;
			switch (ctx->media) {
			case 500: // 8x10
				ctx->media_count_new = 260; // ???
				break;
			case 510: // 8x12
				ctx->media_count_new = 220; // ???
				break;
			case 600: // A4
				ctx->media_count_new = 220; // ???
				break;
			default:
				ctx->media_count_new = 0;
				break;
			}
			break;
		case P_DNP_QW410:
			switch (ctx->media) {
			case 150: // 4x6
			case 160: // 4.5x6
				ctx->media_count_new = 150;
				break;
			case 151: // 4x8
			case 161: // 4.5x8
				ctx->media_count_new = 110;
				break;
			default:
				ctx->media_count_new = 0;
				break;
			}
			break;
		default:
			ctx->media_count_new = 0;
			break;
		}
	}

	if (test_mode < TEST_MODE_NOATTACH && ctx->supports_systime) {
		/* Set Printer Time */
		struct dnpds40_cmd cmd;
		char buf[16];
		struct tm *tm;

                time_t now = time(NULL);
		tm = localtime(&now);
		strftime(buf, sizeof(buf), "%Y%m%d%H%M%S\r", tm); /* YYYYMMDDHHMMSS\n\0 */
		dnpds40_build_cmd(&cmd, "CNTRL", "SET_SYS_TIME", 0);
		if ((dnpds40_do_cmd(ctx, &cmd, (uint8_t*) buf, sizeof(buf))) != 0)
			return CUPS_BACKEND_FAILED;
	}

	/* Fill out marker message */
	if (ctx->supports_mediaclassrfid) {
		snprintf(ctx->media_text, sizeof(ctx->media_text),
			 "%s %s", dnpds40_media_types(ctx->media, ctx->media_sticker),
			 rfid_media_subtypes(ctx->media_subtype));
	} else {
		snprintf(ctx->media_text, sizeof(ctx->media_text),
			 "%s", dnpds40_media_types(ctx->media, ctx->media_sticker));
	}
	/* Fill out marker structure */
	ctx->marker[0].color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker[0].name = ctx->media_text;
	ctx->marker[0].numtype = ctx->media;
	ctx->marker[0].levelmax = ctx->media_count_new;
	ctx->marker[0].levelnow = CUPS_MARKER_UNKNOWN;
	ctx->marker_count = 1;

	if (ctx->conn->type == P_DNP_DS80D) {
		ctx->marker[1].color = "#00FFFF#FF00FF#FFFF00";
		ctx->marker[1].name = dnpds80_duplex_media_types(ctx->duplex_media);
		ctx->marker[1].numtype = ctx->duplex_media;
		ctx->marker[1].levelmax = ctx->marker[0].levelmax/2;
		ctx->marker[1].levelnow = CUPS_MARKER_UNKNOWN;
		ctx->marker_count++;
	}

	return CUPS_BACKEND_OK;
}

static void dnpds40_cleanup_job(const void *vjob) {
	const struct dnpds40_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);

	free((void*)job);
}

static void dnpds40_teardown(void *vctx) {
	struct dnpds40_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (test_mode < TEST_MODE_NOATTACH && ctx->conn->type == P_DNP_DS80D) {
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

	if (ctx->serno)
		free(ctx->serno);
	if (ctx->version)
		free(ctx->version);
	free(ctx);
}

static int dnpds40_query_status(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int count, len;

	if (test_mode >= TEST_MODE_NOATTACH)
		return 9999;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "STATUS", "", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return 9999;

	dnpds40_cleanup_string((char*)resp, len);
	count = atoi((char*)resp);
	free(resp);

	return count;
}

#define MAX_PRINTJOB_LEN (((ctx->native_width*ctx->max_height+1024+54+10))*3+1024) /* Worst-case, YMC */

static int dnpds40_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct dnpds40_ctx *ctx = vctx;
	int run = 1;
	char buf[9] = { 0 };

	struct dnpds40_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));
	job->common.jobsize = sizeof(*job);
	job->printspeed = -1;

	/* There's no way to figure out the total job length in advance, we
	   have to parse the stream until we get to the image plane data,
	   and even then the stream can contain arbitrary commands later.

	   So instead, we allocate a buffer of the maximum possible length,
	   then parse the incoming stream until we hit the START command at
	   the end of the job.
	*/

	job->databuf = malloc(MAX_PRINTJOB_LEN);
	if (!job->databuf) {
		dnpds40_cleanup_job(job);
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	while (run) {
		int remain, i, j;
		/* Read in command header */
		i = read(data_fd, job->databuf + job->datalen,
			 sizeof(struct dnpds40_cmd));
		if (i < 0) {
			dnpds40_cleanup_job(job);
			return i;
		} else if (i == 0) {
			break;
		} else if (i < (int) sizeof(struct dnpds40_cmd)) {
			int r = i;
			i = read(data_fd, job->databuf + job->datalen + r, sizeof(struct dnpds40_cmd) - r);
			if (i < 0) {
				dnpds40_cleanup_job(job);
				return i;
			} else if (i == 0) {
				break;
			} else if (i < (int)(sizeof(struct dnpds40_cmd) - r)) {
				ERROR("Double short read (%d + %d vs %d)\n", r, i, (int)sizeof(struct dnpds40_cmd));
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
		}

		/* Special case handling for beginning of job */
		if (job->datalen == 0) {
			/* See if job lacks the standard ESC-P start sequence */
			if (job->databuf[job->datalen + 0] != 0x1b ||
			    job->databuf[job->datalen + 1] != 0x50) {
				switch(ctx->conn->type) {
				case P_DNP_QW410:
					i = legacy_qw410_read_parse(job, data_fd, i);
					break;
				case P_CITIZEN_CW01:
					i = legacy_cw01_read_parse(job, data_fd, i);
					break;
				case P_DNP_DS620:
					i = legacy_dnp620_read_parse(job, data_fd, i);
					break;
				case P_DNP_DS820:
					i = legacy_dnp820_read_parse(job, data_fd, i);
					break;
				case P_DNP_DSRX1:
				case P_DNP_DS40:
				case P_DNP_DS80:
				case P_DNP_DS80D:
				default:
					i = legacy_dnp_read_parse(job, data_fd, i);
					break;
				}

				if (i == CUPS_BACKEND_OK) {
					goto parsed;
				}
				dnpds40_cleanup_job(job);
				return i;
			}
		}

		/* Parse out length of data chunk, if any */
		memcpy(buf, job->databuf + job->datalen + 24, 8);
		j = atoi(buf);

		/* Read in data chunk as quickly as possible */
		remain = j;
		while (remain > 0) {
			i = read(data_fd, job->databuf + job->datalen + sizeof(struct dnpds40_cmd),
				 remain);
			if (i < 0) {
				ERROR("Data Read Error: %d (%d/%d @%d/%d)\n", i, remain, j, job->datalen,MAX_PRINTJOB_LEN);
				dnpds40_cleanup_job(job);
				return i;
			}
			if (i == 0) {
				dnpds40_cleanup_job(job);
				return 1;
			}
			job->datalen += i;
			remain -= i;
		}
		job->datalen -= j; /* Back it off */

		/* Check for some offsets */
		if(!memcmp("CNTRL QTY", job->databuf + job->datalen+2, 9)) {
			memcpy(buf, job->databuf + job->datalen + 32, 8);
			job->common.copies = atoi(buf);
			continue;
		}
		if(!memcmp("CNTRL CUTTER", job->databuf + job->datalen+2, 12)) {
			memcpy(buf, job->databuf + job->datalen + 32, 8);
			job->cutter = atoi(buf);
			/* We'll insert it ourselves later */
			continue;
		}
		if(!memcmp("CNTRL BUFFCNTRL", job->databuf + job->datalen+2, 15)) {
			/* Ignore this.  We will insert our own later on
			   if the printer and job support it. */
			continue;
		}
		if(!memcmp("CNTRL OVERCOAT", job->databuf + job->datalen+2, 14)) {
			if (ctx->supports_matte) {
				memcpy(buf, job->databuf + job->datalen + 32, 8);
				job->matte = atoi(buf);
			} else {
				WARNING("Printer FW does not support matte prints, using glossy mode\n");
			}
			/* We'll insert our own later, if appropriate */
			continue;
		}
		if(!memcmp("IMAGE MULTICUT", job->databuf + job->datalen+2, 14)) {
			memcpy(buf, job->databuf + job->datalen + 32, 8);
			job->multicut = atoi(buf);
			/* Backend automatically handles rewind support, so
			   ignore application requests to use it. */
			if (job->multicut > 400)
				job->multicut -= 400;

			/* We'll insert this ourselves later. */
			continue;
		}
		if(!memcmp("CNTRL FULL_CUTTER_SET", job->databuf + job->datalen+2, 21)) {
			if (!ctx->supports_fullcut) {
				WARNING("Printer FW does not support full cutter control!\n");
				continue;
			}

			if (ctx->conn->type == P_DNP_DS820) {
				if (j != 24) {
					WARNING("Full cutter argument length incorrect, ignoring!\n");
					continue;
				}
			} else if (j != 16) {
				WARNING("Full cutter argument length incorrect, ignoring!\n");
				continue;
			} else if (!ctx->supports_adv_fullcut) {
				if (job->databuf[job->datalen + 32 + 12] != '0' ||
				    job->databuf[job->datalen + 32 + 13] != '0' ||
				    job->databuf[job->datalen + 32 + 14] != '0') {
					WARNING("Full cutter scrap setting not supported on this firmware, ignoring!\n");
					continue;
				}
			}
			// XXX enforce cut counts/sizes?

			job->fullcut = 1;
		}
		if(!memcmp("IMAGE YPLANE", job->databuf + job->datalen + 2, 12)) {
			uint32_t y_ppm; /* Pixels Per Meter */

			/* Validate vertical resolution */
			memcpy(&y_ppm, job->databuf + job->datalen + 32 + 42, sizeof(y_ppm));
			y_ppm = le32_to_cpu(y_ppm);

			switch (y_ppm) {
			case 11808:
				job->dpi = 300;
				break;
			case 13146:
				job->dpi = 334;
				break;
			case 23615:
			case 23616:
				job->dpi = 600;
				break;
			default:
				ERROR("Unrecognized printjob resolution (%u ppm)\n", y_ppm);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}

			/* Validate horizontal size */
			memcpy(&y_ppm, job->databuf + job->datalen + 32 + 18, sizeof(y_ppm));
			y_ppm = le32_to_cpu(y_ppm);
			if (y_ppm != ctx->native_width) {
				ERROR("Incorrect horizontal resolution (%u), aborting!\n", y_ppm);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
		}
		if(!memcmp("CNTRL PRINTSPEED", job->databuf + job->datalen + 2, 16)) {
			if (!ctx->supports_printspeed) {
				WARNING("Printer does not support PRINTSPEED\n");
				continue;
			}
			memcpy(buf, job->databuf + job->datalen + 32, 8);
			job->printspeed = atoi(buf) / 10;

			/* We'll insert this ourselves later. */
			continue;
		}

		/* This is the last block.. */
	        if(!memcmp("CNTRL START", job->databuf + job->datalen + 2, 11))
			run = 0;

		/* Add in the size of this chunk */
		job->datalen += sizeof(struct dnpds40_cmd) + j;
	}
parsed:
	/* If we have no data.. don't bother */
	if (!job->datalen) {
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Use the larger of the copy arguments */
	if (job->common.copies < copies)
		job->common.copies = copies;

	/* Make sure advanced matte modes are supported */
	if (job->matte > 100) {
		if (!ctx->supports_advmatte) {
			ERROR("Printer does not support advanced matte modes, aborting!\n");
			return CUPS_BACKEND_CANCEL;
		}
		if (ctx->partialmatte == 1) {
			ctx->partialmatte = 2;
		} else {
			INFO("Partial matte lamination layer!\n");
			ctx->partialmatte = 1;
		}
	} else {
		if (ctx->partialmatte == 1)
			ctx->partialmatte = 2;
	}

	/* Sanity check matte mode */
	if ((job->matte == 21 || job->matte == 121) && !ctx->supports_finematte) {
		WARNING("Printer FW does not support Fine Matte mode, downgrading to normal matte\n");
		job->matte = 1;
	} else if ((job->matte == 22 || job->matte == 122) && !ctx->supports_luster) {
		WARNING("Printer FW does not support Luster mode, downgrading to normal matte\n");
		job->matte = 1;
	} else if (job->matte > 1 && !ctx->supports_advmatte) {
		WARNING("Printer FW does not support advanced matte modes, downgrading to normal matte\n");
		job->matte = 1;
	}

	/* QW410 only supports 0 and 3, supposedly. */
	if (ctx->conn->type == P_DNP_QW410 && job->printspeed > 1) {
		job->printspeed = 3;
	}

	/* Pick a sane default value for printspeed if not specified */
	if (job->printspeed == -1 || job->printspeed > 3)
	{
		if (job->dpi == 600)
			job->printspeed = 1;
		else
			job->printspeed = 0;
	}

	if (job->dpi == 600 && !ctx->supports_600dpi) {
		ERROR("Printer does not support 600dpi!\n");
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* And sanity-check whatever value is there */
	if (job->printspeed == 0 && job->dpi == 600) {
		job->printspeed = 1;
	} else if (job->printspeed >= 1 && job->dpi == 300) {
		job->printspeed = 0;
	}

	/* Make sure MULTICUT is sane, most validation needs this */
	if (!job->multicut && ctx->conn->type != P_CITIZEN_CW01) {
		WARNING("Missing or illegal MULTICUT command!\n");
		if (job->dpi == 300)
			job->buf_needed = 1;
		else
			job->buf_needed = 2;

		goto skip_checks;
	}

	/* Only DS80D supports Cut Paper types */
	if (job->multicut > 100) {
		if ( ctx->conn->type == P_DNP_DS80D) {
			job->cut_paper = 1;
		} else {
			ERROR("Only DS80D supports cut-paper sizes!\n");
			dnpds40_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
	}

	/* Figure out the number of buffers we need. */
	job->buf_needed = 1;

	if (job->dpi == 600) {
		switch(ctx->conn->type) {
		case P_DNP_DS620:
			if (job->multicut == MULTICUT_6x9 ||
			    job->multicut == MULTICUT_6x4_5X2)
				job->buf_needed = 2;
			break;
		case P_DNP_DS80:  /* DS80/CX-W */
			if (job->matte && (job->multicut == MULTICUT_8xA4LEN ||
					   job->multicut == MULTICUT_8x4X3 ||
					   job->multicut == MULTICUT_8x8_8x4 ||
					   job->multicut == MULTICUT_8x6X2 ||
					   job->multicut == MULTICUT_8x12))
				job->buf_needed = 2;
			break;
		case P_DNP_DS80D:
			if (job->matte) {
				int mcut = job->multicut;

				if (mcut > MULTICUT_S_BACK)
					mcut -= MULTICUT_S_BACK;
				else if (mcut > MULTICUT_S_FRONT)
					mcut -= MULTICUT_S_FRONT;

				if (mcut == MULTICUT_8xA4LEN ||
				    mcut == MULTICUT_8x4X3 ||
				    mcut == MULTICUT_8x8_8x4 ||
				    mcut == MULTICUT_8x6X2 ||
				    mcut == MULTICUT_8x12)
					job->buf_needed = 2;

				if (mcut == MULTICUT_S_8x12 ||
				    mcut == MULTICUT_S_8x6X2 ||
				    mcut == MULTICUT_S_8x4X3)
					job->buf_needed = 2;
			}
			break;
		case P_DNP_DS820:
			// Nothing; all sizes only need 1 buffer
			break;
		case P_CITIZEN_CW01:
			job->buf_needed = 2;
			break;
		default: /* DS40/CX/RX1/CY/everything else */
			if (job->matte) {
				if (job->multicut == MULTICUT_6x8 ||
				    job->multicut == MULTICUT_6x9 ||
				    job->multicut == MULTICUT_6x4X2 ||
				    job->multicut == MULTICUT_5x7 ||
				    job->multicut == MULTICUT_5x3_5X2)
					job->buf_needed = 2;

			} else {
				if (job->multicut == MULTICUT_6x8 ||
				    job->multicut == MULTICUT_6x9 ||
				    job->multicut == MULTICUT_6x4X2)
					job->buf_needed = 1;
			}
			break;
		}
	}
	if (job->dpi == 334 && ctx->conn->type != P_CITIZEN_CW01)
	{
		ERROR("Illegal resolution (%u) for printer!\n", job->dpi);
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Sanity-check type vs loaded media */
	if (job->multicut == 0)
		goto skip_multicut;

	/* Extra sanity checks */
	if (ctx->media == 0) {
		int status = dnpds40_query_status(ctx);
		if (status > 1000) {
			ERROR("User-Recoverable Printer Error: %d => %s, halting queue!\n", status, dnpds40_statuses(status));
			return CUPS_BACKEND_STOP;
		}
		if (status > 2000) {
			ERROR("Fatal Printer Hardware Error: %d => %s, halting queue!\n", status, dnpds40_statuses(status));
			return CUPS_BACKEND_STOP;
		}
	}

	if (job->multicut < 100) {
		switch(ctx->media) {
		case 150: // 4x6, QW410
			if (job->multicut != MULTICUT_4x4 &&
			    job->multicut != MULTICUT_4x6) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 151: // 4x8, QW410
			if (job->multicut != MULTICUT_4x4 &&
			    job->multicut != MULTICUT_4x6 &&
			    job->multicut != MULTICUT_4x8) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 160: // 4.5x6, QW410
			if (job->multicut != MULTICUT_4_5x4_5 &&
			    job->multicut != MULTICUT_4_5x6) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 161: // 4.5x8, QW410
			if (job->multicut != MULTICUT_4_5x4_5 &&
			    job->multicut != MULTICUT_4_5x6 &&
			    job->multicut != MULTICUT_4_5x8) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 200: //"5x3.5 (L)"
			if (job->multicut != MULTICUT_5x3_5) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 210: //"5x7 (2L)"
			if (job->multicut != MULTICUT_5x3_5 && job->multicut != MULTICUT_5x7 &&
			    job->multicut != MULTICUT_5x3_5X2 && job->multicut != MULTICUT_5x5) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			/* Only 3.5x5 on 7x5 media can be rewound */
			if (job->multicut == MULTICUT_5x3_5)
				job->can_rewind = 1;
			break;
		case 300: //"6x4 (PC)"
			if (job->multicut != MULTICUT_6x4) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 310: //"6x8 (A5)"
			if (job->multicut != MULTICUT_6x4 && job->multicut != MULTICUT_6x8 &&
			    job->multicut != MULTICUT_6x4X2 &&
			    job->multicut != MULTICUT_6x6 && job->multicut != 30) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			/* Only 6x4 on 6x8 media can be rewound */
			if (job->multicut == MULTICUT_6x4)
				job->can_rewind = 1;
			break;
		case 400: //"6x9 (A5W)"
			if (job->multicut != MULTICUT_6x4 && job->multicut != MULTICUT_6x8 &&
			    job->multicut != MULTICUT_6x9 && job->multicut != MULTICUT_6x4X2 &&
			    job->multicut != MULTICUT_6x6 &&
			    job->multicut != MULTICUT_6x4_5 && job->multicut != MULTICUT_6x4_5X2) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			/* Only 6x4 or 6x4.5 on 6x9 media can be rewound */
			if (job->multicut == MULTICUT_6x4 || job->multicut == MULTICUT_6x4_5)
				job->can_rewind = 1;
			break;
		case 500: //"8x10"
			if (ctx->conn->type == P_DNP_DS820 &&
			    (job->multicut == MULTICUT_8x7 || job->multicut == MULTICUT_8x9)) {
				/* These are okay */
			} else if (job->multicut < MULTICUT_8x10 || job->multicut == MULTICUT_8x12 ||
			    job->multicut == MULTICUT_8x6X2 || job->multicut >= MULTICUT_8x6_8x5 ) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}

			/* 8x4, 8x5 can be rewound */
			if (job->multicut == MULTICUT_8x4 ||
			    job->multicut == MULTICUT_8x5)
				job->can_rewind = 1;
			break;
		case 510: //"8x12"
			if (job->multicut < MULTICUT_8x10 || (job->multicut > MULTICUT_8xA4LEN && !(job->multicut == MULTICUT_8x7 || job->multicut == MULTICUT_8x9))) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}

			/* 8x4, 8x5, 8x6 can be rewound */
			if (job->multicut == MULTICUT_8x4 ||
			    job->multicut == MULTICUT_8x5 ||
			    job->multicut == MULTICUT_8x6)
				job->can_rewind = 1;
			break;
		case 600: //"A4"
			if (job->multicut < MULTICUT_A5 || job->multicut > MULTICUT_A4x5X2) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			/* Only A4x4 and A5 can be rewound */
			// XXX we can fake more with fancy multicuts...
			if (job->multicut == MULTICUT_A4x5 ||
			    job->multicut == MULTICUT_A5)
				job->can_rewind = 1;
			break;
		default:
			ERROR("Unknown media (%u vs %u)!\n", ctx->media, job->multicut);
			dnpds40_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
	} else if (job->multicut < 400) {
		int mcut = job->multicut;

		switch(ctx->duplex_media) {
		case 100: //"8x10.75"
			if (mcut > MULTICUT_S_BACK)
				mcut -= MULTICUT_S_BACK;
			else if (mcut > MULTICUT_S_FRONT)
				mcut -= MULTICUT_S_FRONT;

			if (mcut == MULTICUT_S_8x12 ||
			    mcut == MULTICUT_S_8x6X2 ||
			    mcut == MULTICUT_S_8x4X3) {
				ERROR("Incorrect media for job loaded (%u vs %u)\n", ctx->duplex_media, job->multicut);
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			break;
		case 200: //"8x12"
			/* Everything is legal */
			break;
		default:
			ERROR("Unknown duplexer media (%u vs %u)!\n", ctx->duplex_media, job->multicut);
			dnpds40_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
	} else {
		ERROR("Multicut value out of range! (%u)\n", job->multicut);
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Additional santity checks, make sure printer support exists */
	if (!ctx->supports_6x6 && job->multicut == MULTICUT_6x6) {
		ERROR("Printer does not support 6x6 prints, aborting!\n");
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	if (!ctx->supports_5x5 && job->multicut == MULTICUT_5x5) {
		ERROR("Printer does not support 5x5 prints, aborting!\n");
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	if ((job->multicut == MULTICUT_6x4_5 || job->multicut == MULTICUT_6x4_5X2) &&
	    !ctx->supports_6x4_5) {
		ERROR("Printer does not support 6x4.5 prints, aborting!\n");
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	if (job->multicut == MULTICUT_6x9 && !ctx->supports_6x9) {
		ERROR("Printer does not support 6x9 prints, aborting!\n");
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	if (job->multicut == MULTICUT_5x3_5X2 && !ctx->supports_3x5x2) {
		ERROR("Printer does not support 3.5x5*2 prints, aborting!\n");
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	if (!ctx->supports_a4x6 && job->multicut == MULTICUT_A4x6) {
		ERROR("Printer does not support A4x6 prints, aborting!\n");
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

skip_multicut:

	if (job->fullcut && !ctx->supports_adv_fullcut &&
	    job->multicut != MULTICUT_6x8) {
		ERROR("Printer does not support full control on sizes other than 6x8, aborting!\n");
		dnpds40_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	if (job->fullcut && job->cutter) {
		WARNING("Cannot simultaneously use FULL_CUTTER and CUTTER, using the former\n");
		job->cutter = 0;
	}

	if (job->cutter == 120) {
		if (job->multicut == MULTICUT_6x4 || job->multicut == MULTICUT_6x8) {
			if (!ctx->supports_2x6) {
				ERROR("Printer does not support 2x6 prints, aborting!\n");
				dnpds40_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
		} else {
			ERROR("Printer only supports legacy 2-inch cuts on 4x6 or 8x6 jobs!\n");
			dnpds40_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
	}

skip_checks:
	DEBUG("job->dpi %u matte %d mcut %u cutter %d/%d, bufs %d spd %d\n",
	      job->dpi, job->matte, job->multicut, job->cutter, job->fullcut, job->buf_needed, job->printspeed);

	job->common.can_combine = job->can_rewind; /* Any rewindable size can be stacked */

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int dnpds40_main_loop(void *vctx, const void *vjob, int wait_on_return) {
	struct dnpds40_ctx *ctx = vctx;
	int ret;
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;
	uint8_t *ptr;
	char buf[9];
	int status;
	int buf_needed;
	int multicut;
	int count = 0;
	int manual_copies = 0;
	int copies;

	const struct dnpds40_printjob *job = vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	buf_needed = job->buf_needed;
	multicut = job->multicut;
	copies = job->common.copies;

	/* If we switch major overcoat modes, we need both buffers */
	if (!!job->matte != ctx->last_matte)
		buf_needed = 2;

	if (job->cutter == 120) {
		/* Work around firmware bug on DS40 where if we run out
		   of media, we can't resume the job without losing the
		   cutter setting. */
		// XXX add version test? what about other printers?
		manual_copies = 1;
	}

	/* Partial matte operation:
	   - Two buffers to start
	   - One buffer to complete
	*/
	if (ctx->partialmatte == 1) {
		copies = 1;  /* Only send the partial matte layer over once */
		buf_needed = 2;
	} else if (ctx->partialmatte == 2) {
		buf_needed = 1;
	}

	/* RX1HS requires HS media, but the only way to tell is that the
	   HS media reports a lot code, while the non-HS media does not. */
	if (ctx->needs_mlot) {
		/* Get Media Lot */
		dnpds40_build_cmd(&cmd, "INFO", "MLOT", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		len = strlen((char*)resp);
		free(resp);
		if (!len) {
			ERROR("Media does not report a valid lot number (non-HS media in RX1HS?)\n");
			return CUPS_BACKEND_STOP;
		}
	}

top:

	/* Query status */
	status = dnpds40_query_status(ctx);

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
		break;
	case 1000: /* Cover open */
	case 1010: /* No Scrap Box */
	case 1100: /* Paper End */
	case 1200: /* Ribbon End */
	case 1300: /* Paper Jam */
	case 1400: /* Ribbon Error */
		ERROR("Printer not ready: %s, please correct...\n", dnpds40_statuses(status));
		return CUPS_BACKEND_STOP;
	case 1500: /* Paper definition error */
		ERROR("Paper definition error, aborting job\n");
		return CUPS_BACKEND_CANCEL;
	case 1600: /* Data error */
		ERROR("Data error, aborting job\n");
		return CUPS_BACKEND_CANCEL;
	default:
		ERROR("Fatal Printer Error: %d => %s, halting queue!\n", status, dnpds40_statuses(status));
		return CUPS_BACKEND_STOP;
	}

	{
		/* Figure out remaining native prints */
		if (dnpds40_query_markers(ctx, NULL, NULL))
			return CUPS_BACKEND_FAILED;
		if (ctx->marker[0].levelnow < 0)
			return CUPS_BACKEND_FAILED;
		dump_markers(ctx->marker, ctx->marker_count, 0);

		// For logic below.
		count = ctx->marker[0].levelnow;
		if (job->cut_paper && count > ctx->marker[1].levelnow)
			count = ctx->marker[1].levelnow;

		/* See if we can rewind to save media */
		if (job->can_rewind && ctx->supports_rewind) {
			/* Tell printer to use rewind */
			multicut += 400;

			/* Get Media remaining */
			dnpds40_build_cmd(&cmd, "INFO", "RQTY", 0);

			resp = dnpds40_resp_cmd(ctx, &cmd, &len);
			if (!resp)
				return CUPS_BACKEND_FAILED;

			dnpds40_cleanup_string((char*)resp, len);
			count = atoi((char*)resp+4);
			free(resp);
		}

		if (ctx->conn->type == P_CITIZEN_CW01) {
			/* Get Vertical resolution */
			dnpds40_build_cmd(&cmd, "INFO", "RESOLUTION_V", 0);

			resp = dnpds40_resp_cmd(ctx, &cmd, &len);
			if (!resp)
				return CUPS_BACKEND_FAILED;

			dnpds40_cleanup_string((char*)resp, len);

#if 0  // TODO Fix 600dpi support on CW01
			// have to read the last DPI, and send the correct CWD over?
			if (ctx->dpi == 600 && strcmp("RV0334", *char*)resp) {
				ERROR("600DPI prints not yet supported, need 600DPI CWD load\n");
				return CUPS_BACKEND_CANCEL;
			}
#endif
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

	/* Work around a bug in older gutenprint releases. */
	if (ctx->last_multicut >= 200 && ctx->last_multicut < 300 &&
	    multicut >= 200 && multicut < 300) {
		WARNING("Bogus multicut value for duplex page, correcting\n");
		multicut += 100;
	}

	/* Store our last multicut state */
	ctx->last_multicut = multicut;

	/* Tell printer how many copies to make */
	snprintf(buf, sizeof(buf), "%07d\r", manual_copies ? 1 : copies);
	dnpds40_build_cmd(&cmd, "CNTRL", "QTY", 8);
	if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
		return CUPS_BACKEND_FAILED;

	if (!manual_copies)
		copies = 1;

	/* Enable job resumption on correctable errors */
	if (ctx->supports_matte) {
		snprintf(buf, sizeof(buf), "%08d", 1);
		/* DS80D does not support BUFFCNTRL when using
		   cut media; all others support this */
		if (ctx->conn->type != P_DNP_DS80D ||
		    multicut < 100) {
			dnpds40_build_cmd(&cmd, "CNTRL", "BUFFCNTRL", 8);
			if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
				return CUPS_BACKEND_FAILED;
		}
	}

	/* Set overcoat parameters if appropriate */
	if (ctx->supports_matte && ctx->partialmatte != 2) {
		snprintf(buf, sizeof(buf), "%08d", job->matte);
		dnpds40_build_cmd(&cmd, "CNTRL", "OVERCOAT", 8);
		if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
			return CUPS_BACKEND_FAILED;
	}

	/* Program in the cutter setting */
	if (job->cutter) {
		snprintf(buf, sizeof(buf), "%08d", job->cutter);
		dnpds40_build_cmd(&cmd, "CNTRL", "CUTTER", 8);
		if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
			return CUPS_BACKEND_FAILED;
	}

	/* Send over the printspeed if appropriate */
	if (ctx->supports_printspeed) {
		snprintf(buf, sizeof(buf), "%08d", job->printspeed * 10);
		dnpds40_build_cmd(&cmd, "CNTRL", "PRINTSPEED", 8);
		if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
			return CUPS_BACKEND_FAILED;
	}

	/* Program in the multicut setting, if one exists */
	if (multicut) {
		snprintf(buf, sizeof(buf), "%08d", multicut);
		dnpds40_build_cmd(&cmd, "IMAGE", "MULTICUT", 8);
		if ((ret = dnpds40_do_cmd(ctx, &cmd, (uint8_t*)buf, 8)))
			return CUPS_BACKEND_FAILED;
	}

	/* Finally, send the stream over as individual data chunks */
	ptr = job->databuf;
	while(ptr && ptr < (job->databuf + job->datalen)) {
		int i;
		buf[8] = 0;
		memcpy(buf, ptr + 24, 8);
		i = atoi(buf) + 32;

		if ((ret = send_data(ctx->conn,
				     ptr, i)))
			return CUPS_BACKEND_FAILED;

		ptr += i;
	}
	sleep(1);  /* Give things a moment */

	/* Reset partial matte state if necessary */
	if (ctx->partialmatte == 2)
		ctx->partialmatte = 0;

	if (!wait_on_return && !manual_copies) {
		INFO("Fast return mode enabled.\n");
	} else if (!ctx->partialmatte) {
		INFO("Waiting for job to complete...\n");
		int started = 0;

		while (1) {
			/* Query status */
			status = dnpds40_query_status(ctx);

			/* If we're idle or there's an error..*/
			if (status == 0 && started)
				break;
			if (status)
				started = 1;
			if (status >= 1000) {
				ERROR("Printer encountered error: %d -> %s\n", status, dnpds40_statuses(status));
				/* Note: We are *not* returning a backend error code here as we don't want to
				   stop the queue unnecessarily.  If another job is submitted and the error is
				   still present, the queue will halt at that time */
				break;
			}
			sleep(1);
		}

		/* Figure out remaining native prints */
		if (dnpds40_query_markers(ctx, NULL, NULL))
			return CUPS_BACKEND_FAILED;
		dump_markers(ctx->marker, ctx->marker_count, 0);
	}

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		/* No need to wait on buffers due to matte switching */
		buf_needed = job->buf_needed;
		goto top;
	}

	/* Finally, account for overcoat mode of last print */
	ctx->last_matte = !!job->matte;
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
		} else if (!strcmp("DC5", tok)) {
			INFO("USB Supply Voltage : %s\n", val);
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
	int cwd_extra = 0;
	int cwd_index = 1;
	uint8_t cwd_buf[5];

	INFO("Model: %s\n", dnpds40_printer_type(ctx->conn->type, ctx->mfg));

	/* Serial number already queried */
	INFO("Serial Number: %s\n", ctx->serno);

	/* Firmware version already queried */
	INFO("Firmware Version: %s\n", ctx->version);

	/* Figure out Duplexer */
	if (ctx->conn->type == P_DNP_DS80D) {
		dnpds40_build_cmd(&cmd, "INFO", "UNIT_FVER", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Duplexer Version: %s\n", resp);

		free(resp);
	}

	if (ctx->conn->type == P_CITIZEN_CW01) {
		/* Get Horizonal resolution */
		dnpds40_build_cmd(&cmd, "INFO", "RESOLUTION_H", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Horizontal Resolution: %s dpi\n", (char*)resp + 3);

		free(resp);

		/* Get Vertical resolution */
		dnpds40_build_cmd(&cmd, "INFO", "RESOLUTION_V", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Vertical Resolution: %s dpi\n", (char*)resp + 3);

		free(resp);

		/* Get Color Control Data Version */
		dnpds40_build_cmd(&cmd, "TBL_RD", "Version", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Color Data Version: %s ", (char*)resp);

		free(resp);

		/* Get Color Control Data Checksum */
		dnpds40_build_cmd(&cmd, "MNT_RD", "CTRLD_CHKSUM", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		DEBUG2("(%s)\n", (char*)resp);

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

	if (ctx->conn->type == P_CITIZEN_CW01)
		goto skip;

	/* Get Ribbon ID code (?) */
	dnpds40_build_cmd(&cmd, "MNT_RD", "RIBBON_ID_CODE", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("Ribbon ID: %s\n", (char*)resp);

	free(resp);

CWD_TOP:
	/* Figure out color data and checksums */
	if (ctx->supports_ctrld_ext) {
		cwd_extra = 4;
		snprintf((char*)cwd_buf, sizeof(cwd_buf), "%04d", cwd_index);
	} else {
		cwd_extra = 0;
		cwd_buf[0] = 0;
	}

	/* 300 DPI */
	dnpds40_build_cmd(&cmd, "TBL_RD", "CWD300_Version", cwd_extra);

	resp = dnpds40_resp_cmd2(ctx, &cmd, &len, cwd_buf, cwd_extra);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	INFO("300 DPI Color Data (%s): %s ", rfid_media_subtypes(cwd_index), (char*)resp);

	free(resp);

	dnpds40_build_cmd(&cmd, "TBL_RD", "CWD300_Checksum", cwd_extra);

	resp = dnpds40_resp_cmd2(ctx, &cmd, &len, cwd_buf, cwd_extra);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	DEBUG2("(%s)\n", (char*)resp);

	free(resp);

	if (ctx->supports_600dpi) {
		/* 600 DPI */
		dnpds40_build_cmd(&cmd, "TBL_RD", "CWD600_Version", cwd_extra);

		resp = dnpds40_resp_cmd2(ctx, &cmd, &len, cwd_buf, cwd_extra);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("600 DPI Color Data (%s): %s ", rfid_media_subtypes(cwd_index), (char*)resp);

		free(resp);

		dnpds40_build_cmd(&cmd, "TBL_RD", "CWD600_Checksum", cwd_extra);

		resp = dnpds40_resp_cmd2(ctx, &cmd, &len, cwd_buf, cwd_extra);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		DEBUG2("(%s)\n", (char*)resp);

		free(resp);
	}

	if (ctx->supports_lowspeed) {
		/* "Low Speed" */
		if (ctx->supports_600dpi) {
			dnpds40_build_cmd(&cmd, "TBL_RD", "CWD610_Version", cwd_extra);
		} else {
			dnpds40_build_cmd(&cmd, "TBL_RD", "CWD310_Version", cwd_extra);
		}

		resp = dnpds40_resp_cmd2(ctx, &cmd, &len, cwd_buf, cwd_extra);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("Low Speed Color Data (%s): %s ", rfid_media_subtypes(cwd_index), (char*)resp);

		free(resp);

		if (ctx->supports_600dpi) {
			dnpds40_build_cmd(&cmd, "TBL_RD", "CWD610_Checksum", cwd_extra);
		} else {
			dnpds40_build_cmd(&cmd, "TBL_RD", "CWD310_Checksum", cwd_extra);
		}

		resp = dnpds40_resp_cmd2(ctx, &cmd, &len, cwd_buf, cwd_extra);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		DEBUG2("(%s)\n", (char*)resp);

		free(resp);
	}
	if (ctx->supports_highdensity) {
		/* "High Density" */
		if (ctx->supports_600dpi) {
			dnpds40_build_cmd(&cmd, "TBL_RD", "CWD620_Version", cwd_extra);
		} else {
			dnpds40_build_cmd(&cmd, "TBL_RD", "CWD320_Version", cwd_extra);
		}

		resp = dnpds40_resp_cmd2(ctx, &cmd, &len, cwd_buf, cwd_extra);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		INFO("High Density Color Data (%s): %s ", rfid_media_subtypes(cwd_index), (char*)resp);

		free(resp);

		if (ctx->supports_600dpi) {
			dnpds40_build_cmd(&cmd, "TBL_RD", "CWD620_Checksum", cwd_extra);
		} else {
			dnpds40_build_cmd(&cmd, "TBL_RD", "CWD320_Checksum", cwd_extra);
		}

		resp = dnpds40_resp_cmd2(ctx, &cmd, &len, cwd_buf, cwd_extra);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		DEBUG2("(%s)\n", (char*)resp);

		free(resp);
	}

	if (cwd_extra && cwd_index == 1) {
		if (ctx->conn->type == P_DNP_QW410)
			cwd_index = 2;
		else
			cwd_index = 3;
		goto CWD_TOP;
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
	}

	if (ctx->supports_keepmode) {
		/* Get Media End Keep */
		int i;

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

skip:
	return CUPS_BACKEND_OK;
}

static int dnpds40_get_status(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int len = 0;
	int count;

	/* Generate command */
	count = dnpds40_query_status(ctx);

	INFO("Printer Status: %s (%d)\n", dnpds40_statuses(count), count);

	/* Figure out Duplexer */
	if (ctx->conn->type == P_DNP_DS80D) {
		dnpds40_build_cmd(&cmd, "INFO", "UNIT_STATUS", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);
		count = atoi((char*)resp);

		INFO("Duplexer Status: %s\n", dnpds80_duplex_statuses(count));
		INFO("Duplexer Media Status: %s\n", dnpds80_duplex_paper_status(ctx->duplex_media_status));

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
	INFO("Media Type: %s\n", dnpds40_media_types(ctx->media, ctx->media_sticker));

	if (ctx->supports_media_ext) {
		int type;
		dnpds40_build_cmd(&cmd, "INFO", "MEDIA_EXT_CODE", 0);
		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);
		type = atoi((char*)resp+7);
		ctx->media_sticker = (type == 1);
		INFO("Media Code: %s\n", dnpds620_media_extension_code(type));
		free(resp);
	}

	/* Try to figure out media subtype */
	if (ctx->supports_mediaclassrfid) {
		INFO("Media Subtype: %s\n", rfid_media_subtypes(ctx->media_subtype));
	}

	/* Report Cut Media */
	if (ctx->conn->type == P_DNP_DS80D) {
		INFO("Duplex Media Type: %s\n", dnpds80_duplex_media_types(ctx->duplex_media));
		INFO("Duplexer Media Status: %s\n", dnpds80_duplex_paper_status(ctx->duplex_media_status));
	}

	if (ctx->media_count_new)
		INFO("Native Prints Available on New Media: %u\n", ctx->media_count_new);

	/* Get Media remaining */
	count = dnpds40_query_mqty(ctx);
	if (count < 0)
		return CUPS_BACKEND_FAILED;

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

	return CUPS_BACKEND_OK;
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

	if (ctx->conn->type == P_DNP_DS620 ||
	    ctx->conn->type == P_DNP_DS820) {
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

	if (ctx->conn->type == P_DNP_DS80D) {
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

	return CUPS_BACKEND_OK;
}

static int dnpds40_cancel_job(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "CNTRL", "CANCEL", 0);

	if ((ret = dnpds40_do_cmd(ctx, &cmd, NULL, 0)))
		return ret;

	return CUPS_BACKEND_OK;
}

static int dnpds40_reset_printer(struct dnpds40_ctx *ctx)
{
	struct dnpds40_cmd cmd;
	int ret;

	/* Generate command */
	dnpds40_build_cmd(&cmd, "CNTRL", "PRINTER_RESET", 0);

	if ((ret = dnpds40_do_cmd(ctx, &cmd, NULL, 0)))
		return ret;

	return CUPS_BACKEND_OK;
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

	return CUPS_BACKEND_OK;
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

	return CUPS_BACKEND_OK;
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

	return CUPS_BACKEND_OK;
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

	return CUPS_BACKEND_OK;
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
			int sleeptime = atoi(optarg);
			if (!ctx->supports_standby) {
				ERROR("Printer does not support standby\n");
				j = -1;
				break;
			}
			if (sleeptime < 0 || sleeptime > 99) {
				ERROR("Value out of range (0-99)\n");
				j = -1;
				break;
			}
			j = dnpds620_standby_mode(ctx, sleeptime);
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
				ERROR("Value out of range (0-1)\n");
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
			if (optarg[0] == 'M' && !ctx->supports_matte) {
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
				ERROR("Value out of range (0-1)\n");
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

	return CUPS_BACKEND_OK;
}

static int dnpds40_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct dnpds40_ctx *ctx = vctx;

	if (markers)
		*markers = ctx->marker;
	if (count)
		*count = ctx->marker_count;

	ctx->marker[0].levelnow = dnpds40_query_mqty(ctx);

	if (ctx->marker[0].levelnow < 0)
		return CUPS_BACKEND_FAILED;

        if (ctx->conn->type == P_DNP_DS80D) {
		if (dnpds80dx_query_paper(ctx))
			return CUPS_BACKEND_FAILED;
		switch (ctx->duplex_media_status) {
		case DUPLEX_UNIT_PAPER_NONE:
			ctx->marker[1].levelnow = 0;
			break;
		case DUPLEX_UNIT_PAPER_PROTECTIVE:
			ctx->marker[1].levelnow = CUPS_MARKER_UNAVAILABLE;
			break;
		case DUPLEX_UNIT_PAPER_PRESENT:
			ctx->marker[1].levelnow = CUPS_MARKER_UNKNOWN_OK;
			break;
		}
	}

	return CUPS_BACKEND_OK;
}

static int dnp_query_stats(void *vctx, struct printerstats *stats)
{
	struct dnpds40_cmd cmd;
	struct dnpds40_ctx *ctx = vctx;
	uint8_t *resp;
	int len = 0;

	/* Update marker info */
	if (dnpds40_query_markers(ctx, NULL, NULL))
		return CUPS_BACKEND_FAILED;

	switch (ctx->mfg)
	{
	case MFG_DNP: stats->mfg = "Dai Nippon Printing"; break;
	case MFG_CITIZEN: stats->mfg = "Citizen Systems"; break;
	case MFG_MITSUBISHI: stats->mfg = "Mitsubishi" ; break;
	default: stats->mfg = "Unknown" ; break;
	}

	stats->model = dnpds40_printer_type(ctx->conn->type, ctx->mfg);
	stats->serial = ctx->serno;
	stats->fwver = ctx->version; // XXX duplexer version?

	stats->decks = ctx->conn->type == P_DNP_DS80D ? 2: 1;
	stats->mediatype[0] = ctx->marker[0].name;
	stats->levelmax[0] = ctx->marker[0].levelmax;
	stats->levelnow[0] = ctx->marker[0].levelnow;
	stats->name[0] = "Roll";

	/* Query status */
	stats->status[0] = strdup(dnpds40_statuses(dnpds40_query_status(ctx)));

	/* Query lifetime counter */
	dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_LIFE", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	dnpds40_cleanup_string((char*)resp, len);

	stats->cnt_life[0] = atoi((char*)resp+2);
	free(resp);

	if (ctx->conn->type == P_DNP_DS80D) {
		stats->name[0] = "Sheet";
		stats->mediatype[1] = ctx->marker[1].name;
		stats->levelmax[1] = ctx->marker[1].levelmax;
		stats->levelnow[1] = ctx->marker[1].levelnow;
		stats->status[1] = strdup(dnpds80_duplex_paper_status(ctx->duplex_media_status));

		dnpds40_build_cmd(&cmd, "MNT_RD", "COUNTER_DUPLEX", 0);

		resp = dnpds40_resp_cmd(ctx, &cmd, &len);
		if (!resp)
			return CUPS_BACKEND_FAILED;

		dnpds40_cleanup_string((char*)resp, len);

		stats->cnt_life[1] = atoi((char*)resp);

		free(resp);
	}

	return CUPS_BACKEND_OK;
}

static int dnp_job_polarity(void *vctx)
{
	struct dnpds40_ctx *ctx = vctx;
	struct dnpds40_cmd cmd;
	uint8_t *resp;
	int count;
	int len = 0;

	if (test_mode >= TEST_MODE_NOATTACH)
		return 0;

	if (!ctx->supports_rewind)
		return 0;

	/* Get Media remaining */
	dnpds40_build_cmd(&cmd, "INFO", "RQTY", 0);

	resp = dnpds40_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return 0;

	dnpds40_cleanup_string((char*)resp, len);
	count = atoi((char*)resp+4);
	free(resp);

	return (count & 1);
}

static const char *dnpds40_prefixes[] = {
	"dnp_citizen", "dnpds40",  // Family names, do *not* nuke.
	// backwards compatibility
	"dnpds80", "dnpds80dx", "dnpds620", "dnpds820", "dnprx1",
	"citizencw01", "citizencw02", "citizencx02",
	NULL
};

const struct dyesub_backend dnpds40_backend = {
	.name = "DNP DS-series / Citizen C-series",
	.version = "0.145",
	.uri_prefixes = dnpds40_prefixes,
	.cmdline_usage = dnpds40_cmdline,
	.cmdline_arg = dnpds40_cmdline_arg,
	.init = dnpds40_init,
	.attach = dnpds40_attach,
	.teardown = dnpds40_teardown,
	.read_parse = dnpds40_read_parse,
	.cleanup_job = dnpds40_cleanup_job,
	.main_loop = dnpds40_main_loop,
	.query_serno = dnpds40_query_serno,
	.query_markers = dnpds40_query_markers,
	.query_stats = dnp_query_stats,
	.combine_jobs = dnp_combine_jobs,
	.job_polarity = dnp_job_polarity,
	.devices = {
		{ 0x1343, 0x0003, P_DNP_DS40, NULL, "dnp-ds40"},
		{ 0x1343, 0x0003, P_DNP_DS40, NULL, "citizen-cx"}, /* Duplicate */
		{ 0x1343, 0x0004, P_DNP_DS80, NULL, "dnp-ds80"},
		{ 0x1343, 0x0004, P_DNP_DS80, NULL, "citizen-cx-w"}, /* Duplicate */
		{ 0x1343, 0x0004, P_DNP_DS80, NULL, "mitsubishi-cp3800dw"}, /* Duplicate */
		{ 0x1343, 0x0008, P_DNP_DS80D, NULL, "dnp-ds80dx"},
		{ 0x1343, 0x0005, P_DNP_DSRX1, NULL, "dnp-dsrx1"},
		{ 0x1343, 0x0005, P_DNP_DSRX1, NULL, "citizen-cy"}, /* Duplicate */
		{ 0x1343, 0x0005, P_DNP_DSRX1, NULL, "citizen-cy-02"}, /* Duplicate */
		{ 0x1452, 0x8b01, P_DNP_DS620, NULL, "dnp-ds620"},
		{ 0x1452, 0x9001, P_DNP_DS820, NULL, "dnp-ds820"},
		{ 0x1452, 0x9201, P_DNP_QW410, NULL, "dnp-qw410"},
		{ 0x1343, 0x0002, P_CITIZEN_CW01, NULL, "citizen-cw-01"},
		{ 0x1343, 0x0002, P_CITIZEN_CW01, NULL, "citizen-op900"}, /* Duplicate */
		{ 0x1343, 0x0006, P_CITIZEN_OP900II, NULL, "citizen-cw-02"},
		{ 0x1343, 0x0006, P_CITIZEN_OP900II, NULL, "citizen-op900ii"}, /* Duplicate */
		{ 0x1343, 0x000a, P_DNP_DS620, NULL, "citizen-cx-02"},
		{ 0x1343, 0x000b, P_DNP_DS820, NULL, "citizen-cx-02w"},
		{ 0x1343, 0x000c, P_DNP_QW410, NULL, "citizen-cz-01"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* Windows spool file support */
static int legacy_spool_helper(struct dnpds40_printjob *job, int data_fd,
			       int read_data, int hdrlen, uint32_t plane_len,
			       int parse_dpi)
{
	uint8_t *buf;
	uint8_t bmp_hdr[14];
	int i, remain;
	uint32_t j;

	/* Allocate a temp processing buffer */
	remain = plane_len * 3;
	buf = malloc(remain);
	if (!buf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	/* Copy over the post-jobhdr crap into our processing buffer */
	j = read_data - hdrlen;
	memcpy(buf, job->databuf + hdrlen, j);
	remain -= j;

	/* Read in the remaining spool data */
	while (remain) {
		i = read(data_fd, buf + j, remain);

		if (i < 0) {
			free(buf);
			return i;
		}

		remain -= i;
		j += i;
	}

	if (parse_dpi) {
		/* Parse out Y DPI */
		memcpy(&j, buf + 28, sizeof(j));
		j = le32_to_cpu(j);
		switch(j) {
		case 11808:
			job->dpi = 300;
			break;
		case 23615:
		case 23616:
			job->dpi = 600;
			break;
		default:
			ERROR("Unrecognized printjob resolution (%u ppm)\n", j);
			free(buf);
			return CUPS_BACKEND_CANCEL;
		}
	}

	/* Generate bitmap file header (same for all planes) */
	j = cpu_to_le32(plane_len + 24);
	memset(bmp_hdr, 0, sizeof(bmp_hdr));
	bmp_hdr[0] = 0x42;
	bmp_hdr[1] = 0x4d;
	memcpy(bmp_hdr + 2, &j, sizeof(j));
	bmp_hdr[10] = 0x40;
	bmp_hdr[11] = 0x04;

	/* Set up planes */
	j = 0;

	/* Y plane */
	job->datalen += sprintf((char*)job->databuf + job->datalen,
				"\033PIMAGE YPLANE          %08u", plane_len + 24);
	memcpy(job->databuf + job->datalen, bmp_hdr, sizeof(bmp_hdr));
	job->datalen += sizeof(bmp_hdr);
	memcpy(job->databuf + job->datalen, buf + j, plane_len);
	job->datalen += plane_len;
	j += plane_len;
	memset(job->databuf + job->datalen, 0, 10);
	job->datalen += 10;

	/* M plane */
	job->datalen += sprintf((char*)job->databuf + job->datalen,
				"\033PIMAGE MPLANE          %08u", plane_len + 24);
	memcpy(job->databuf + job->datalen, bmp_hdr, sizeof(bmp_hdr));
	job->datalen += sizeof(bmp_hdr);
	memcpy(job->databuf + job->datalen, buf + j, plane_len);
	job->datalen += plane_len;
	j += plane_len;
	memset(job->databuf + job->datalen, 0, 10);
	job->datalen += 10;

	/* C plane */
	job->datalen += sprintf((char*)job->databuf + job->datalen,
				"\033PIMAGE CPLANE          %08u", plane_len + 24);
	memcpy(job->databuf + job->datalen, bmp_hdr, sizeof(bmp_hdr));
	job->datalen += sizeof(bmp_hdr);
	memcpy(job->databuf + job->datalen, buf + j, plane_len);
	job->datalen += plane_len;
	j += plane_len;
	memset(job->databuf + job->datalen, 0, 10);
	job->datalen += 10;

	/* Start */
	job->datalen += sprintf((char*)job->databuf + job->datalen,
				"\033PCNTRL START                   ");

	/* We're done */
	free(buf);

	return CUPS_BACKEND_OK;
}

struct cw01_spool_hdr {
	uint8_t  type; /* TYPE_??? */
	uint8_t  res; /* DPI_??? */
	uint8_t  copies; /* number of prints */
	uint8_t  null0;
	uint32_t plane_len; /* LE */
	uint8_t  null1[4];
} __attribute__((packed));

#define DPI_334 0
#define DPI_600 1

#define TYPE_DSC  0
#define TYPE_L    1
#define TYPE_PC   2
#define TYPE_2DSC 3
#define TYPE_3L   4
#define TYPE_A5   5
#define TYPE_A6   6

static int legacy_cw01_read_parse(struct dnpds40_printjob *job, int data_fd, int read_data)
{
	struct cw01_spool_hdr hdr;
	uint32_t plane_len;

	/* get original header out of structure */
	memcpy(&hdr, job->databuf + job->datalen, sizeof(hdr));

	/* Early parsing and sanity checking */
	plane_len = le32_to_cpu(hdr.plane_len);

	if (hdr.type > TYPE_A6 ||
	    hdr.res > DPI_600 ||
	    hdr.null1[0] || hdr.null1[1] || hdr.null1[2] || hdr.null1[3]) {
		ERROR("Unrecognized header data format @%d!\n", job->datalen);
		return CUPS_BACKEND_CANCEL;
	}
	job->dpi = (hdr.res == DPI_600) ? 600 : 334;
	job->cutter = 0;

	/* Use job's copies */
	job->common.copies = hdr.copies;

	return legacy_spool_helper(job, data_fd, read_data,
				   sizeof(hdr), plane_len, 0);
}

struct rx1_spool_hdr {
	uint8_t  type; /* equals MULTICUT_?? - 1 */
	uint8_t  null0;
	uint8_t  copies; /* number of copies, always fixed at 01.. */
	uint8_t  null1;
	uint32_t plane_len; /* LE */
        uint8_t  flags; /* combination of FLAG_?? */
	uint8_t  null2[3];
} __attribute__((packed));

#define FLAG_MATTE     0x02
#define FLAG_NORETRY   0x08
#define FLAG_2INCH     0x10

static int legacy_dnp_read_parse(struct dnpds40_printjob *job, int data_fd, int read_data)
{
	struct rx1_spool_hdr hdr;
	uint32_t plane_len;

	/* get original header out of structure */
	memcpy(&hdr, job->databuf + job->datalen, sizeof(hdr));

	/* Early parsing and sanity checking */
	plane_len = le32_to_cpu(hdr.plane_len);

	if (hdr.type >= MULTICUT_A4x5X2 ||
	    hdr.null2[0] || hdr.null2[1] || hdr.null2[2]) {
		ERROR("Unrecognized header data format @%d!\n", job->datalen);
		return CUPS_BACKEND_CANCEL;
	}

	/* Use job's copies */
	job->common.copies = hdr.copies;

	/* Don't bother with FW version checks for legacy stuff */
	job->multicut = hdr.type + 1;
	job->matte = (hdr.flags & FLAG_MATTE) ? 1 : 0;
	job->cutter = (hdr.flags & FLAG_2INCH) ? 120 : 0;

	return legacy_spool_helper(job, data_fd, read_data,
				   sizeof(hdr), plane_len, 1);
}

struct ds620_spool_hdr {
	uint8_t  type; /* MULTICUT_?? -1, but >0x90 is a flag for rewind */
	uint8_t  copies; /* Always fixed at 01..*/
	uint8_t  null0[2];
	uint8_t  quality;  /* 0x02 is HQ, 0x00 is HS.  Equivalent to DPI. */
	uint8_t  unk[3];   /* Always 00 01 00 */
	uint32_t plane_len; /* LE */
	uint8_t  flags; /* FLAG_?? */
	uint8_t  null1[3];
} __attribute__((packed));

#define FLAG_LUSTER    0x04
#define FLAG_FINEMATTE 0x06

static int legacy_dnp620_read_parse(struct dnpds40_printjob *job, int data_fd, int read_data)
{
	struct ds620_spool_hdr hdr;
	uint32_t plane_len;

	/* get original header out of structure */
	memcpy(&hdr, job->databuf + job->datalen, sizeof(hdr));

	/* Early parsing and sanity checking */
	plane_len = le32_to_cpu(hdr.plane_len);

	/* Used to signify rewind request.  Backend handles automatically. */
	if (hdr.type > 0x90)
		hdr.type -= 0x90;

	if (hdr.type > MULTICUT_A4x5X2 ||
	    hdr.null1[0] || hdr.null1[1] || hdr.null1[2]) {
		ERROR("Unrecognized header data format @%d!\n", job->datalen);
		return CUPS_BACKEND_CANCEL;
	}

	/* Use job's copies */
	job->common.copies = hdr.copies;

	/* Don't bother with FW version checks for legacy stuff */
	job->multicut = hdr.type + 1;
	if ((hdr.flags & FLAG_FINEMATTE) == FLAG_FINEMATTE)
		job->matte = 21;
	else if (hdr.flags & FLAG_LUSTER)
		job->matte = 22;
	else if (hdr.flags & FLAG_MATTE)
		job->matte = 1;

	job->cutter = (hdr.flags & FLAG_2INCH) ? 120 : 0;

	return legacy_spool_helper(job, data_fd, read_data,
				   sizeof(hdr), plane_len, 1);
}

#define FLAG_820_HD     0x80
#define FLAG_820_RETRY  0x20
#define FLAG_820_LUSTER 0x06
#define FLAG_820_FMATTE 0x04
#define FLAG_820_MATTE  0x02

static int legacy_dnp820_read_parse(struct dnpds40_printjob *job, int data_fd, int read_data)
{
	struct ds620_spool_hdr hdr;
	uint32_t plane_len;

	/* get original header out of structure */
	memcpy(&hdr, job->databuf + job->datalen, sizeof(hdr));

	/* Early parsing and sanity checking */
	plane_len = le32_to_cpu(hdr.plane_len);

	/* Used to signify rewind request.  Backend handles automatically. */
	if (hdr.type > 0x90)
		hdr.type -= 0x90;

	if (hdr.type > MULTICUT_A4x5X2 ||
	    hdr.null1[0] || hdr.null1[1] || hdr.null1[2]) {
		ERROR("Unrecognized header data format @%d!\n", job->datalen);
		return CUPS_BACKEND_CANCEL;
	}

	/* Use job's copies */
	job->common.copies = hdr.copies;

	/* Don't bother with FW version checks for legacy stuff */
	job->multicut = hdr.type + 1;
	if ((hdr.flags & FLAG_820_FMATTE) == FLAG_820_FMATTE)
		job->matte = 21;
	else if ((hdr.flags & FLAG_820_LUSTER) == FLAG_820_LUSTER)
		job->matte = 22;
	else if ((hdr.flags & FLAG_820_MATTE) == FLAG_820_MATTE)
		job->matte = 1;

	if (hdr.flags & FLAG_820_HD)
		job->printspeed = 3;

	return legacy_spool_helper(job, data_fd, read_data,
				   sizeof(hdr), plane_len, 1);
}

struct qw410_spool_hdr {
	uint8_t  type; /* MULTICUT_?? -1 */
	uint8_t  null0[5];
	uint16_t copies; /* LE, 01 or bigger */

	uint32_t plane_len; /* LE */
	uint8_t  matte;
	uint8_t  null1[3];

	uint8_t  unk;  /* always 01 */
	uint8_t  null2[3];
	uint8_t  cut2;
	uint8_t  null3[3];

	uint8_t  hd;
	uint8_t  null4[7];
} __attribute__((packed));

static int legacy_qw410_read_parse(struct dnpds40_printjob *job, int data_fd, int read_data)
{
	struct qw410_spool_hdr hdr;
	uint32_t plane_len;

	/* get original header out of structure */
	memcpy(&hdr, job->databuf + job->datalen, sizeof(hdr));

	/* Early parsing and sanity checking */
	plane_len = le32_to_cpu(hdr.plane_len);

	if (hdr.type < MULTICUT_4x4 || hdr.type > MULTICUT_4_5x8 ||
	    hdr.null0[0] || hdr.null0[1] || hdr.null0[2]) {
		ERROR("Unrecognized header data format @%d!\n", job->datalen);
		return CUPS_BACKEND_CANCEL;
	}

	/* Don't bother with FW version checks for legacy stuff */
	job->multicut = hdr.type + 1;
	job->matte = hdr.matte;
	job->cutter = (hdr.cut2) ? 120 : 0;
	job->printspeed = (hdr.hd) ? 3 : 0;

	return legacy_spool_helper(job, data_fd, read_data,
				   sizeof(hdr), plane_len, 1);
}

/*

Basic spool file format for CW01

TT RR NN 00 XX XX XX XX  00 00 00 00              <- FILE header.

  NN          : copies (0x01 or more)
  RR          : resolution; 0 == 334 dpi, 1 == 600dpi
  TT          : type 0x02 == 4x6, 0x01 == 5x3.5, 0x06 = 6x9
  XX XX XX XX : plane length (LE)
                plane length * 3 + 12 == file length.

Followed by three planes, each with this 40b header:

28 00 00 00 00 08 00 00  RR RR 00 00 01 00 08 00
00 00 00 00 00 00 00 00  5a 33 00 00 YY YY 00 00
00 01 00 00 00 00 00 00

  RR RR       : rows in LE format
  YY YY       : 0x335a (334dpi) or 0x5c40 (600dpi)

Followed by 1024 bytes of color tables:

 ff ff ff 00 ... 00 00 00 00

1024+40 = 1064 bytes of header per plane.

Always have 2048 columns of data.

followed by (2048 * rows) bytes of data.

*/
