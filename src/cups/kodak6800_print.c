/*
 *   Kodak 6800/6850 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2015 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND kodak6800_backend

#include "backend_common.h"

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
	uint8_t  size;     /* 0x06 for 6x8, 0x00 for 6x4, 0x07 for 5x7 */
	uint8_t  laminate; /* 0x01 to laminate, 0x00 for not */
	uint8_t  mode;     /* 0x00 or 0x01 (for 4x6 on 6x8 media) */
} __attribute__((packed));

struct kodak68x0_status_readback {
	uint8_t  hdr;      /* Always 01 */
	uint8_t  status;   /* STATUS_* */
	uint8_t  status1;  /* STATUS1_* */
	uint32_t status2;  /* STATUS2_* */
	uint8_t  errcode;  /* Error ## */
	uint32_t lifetime; /* Lifetime Prints (BE) */
	uint32_t maint;    /* Maint Prints (BE) */
	uint32_t media;     /* Media Prints (6850), Unknown (6800) (BE) */
	uint32_t cutter;     /* Cutter Actuations (BE) */
	uint8_t  nullB[2];
	uint8_t  errtype;   /* seen 0x00 or 0xd0 */
	uint8_t  donor;     /* Percentage, 0-100 */
	uint16_t main_boot; /* Always 003 */
	uint16_t main_fw;   /* seen 652, 656, 670, 671 (6850) and 232 (6800) */
	uint16_t dsp_boot;  /* Always 001 */
	uint16_t dsp_fw;    /* Seen 540, 541, 560 (6850) and 131 (6800) */
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

enum {
	CMD_CODE_OK = 1,
	CMD_CODE_BAD = 2,
};

enum {
        STATUS_PRINTING = 1,
        STATUS_IDLE = 2,
};

enum {
        STATE_STATUS1_STANDBY = 1,
        STATE_STATUS1_ERROR = 2,
        STATE_STATUS1_WAIT = 3,
};

#define STATE_STANDBY_STATUS2 0x0

enum {
        WAIT_STATUS2_INIT = 0,
        WAIT_STATUS2_RIBBON = 1,
        WAIT_STATUS2_THERMAL = 2,
        WAIT_STATUS2_OPERATING = 3,
        WAIT_STATUS2_BUSY = 4,
};

#define ERROR_STATUS2_CTRL_CIRCUIT   (1<<31)
#define ERROR_STATUS2_MECHANISM_CTRL (1<<30)
#define ERROR_STATUS2_SENSOR         (1<<13)
#define ERROR_STATUS2_COVER_OPEN     (1<<12)
#define ERROR_STATUS2_TEMP_SENSOR    (1<<9)
#define ERROR_STATUS2_PAPER_JAM      (1<<8)
#define ERROR_STATUS2_PAPER_EMPTY    (1<<6)
#define ERROR_STATUS2_RIBBON_ERR     (1<<4)

enum {
        CTRL_CIR_ERROR_EEPROM1  = 0x01,
        CTRL_CIR_ERROR_EEPROM2  = 0x02,
        CTRL_CIR_ERROR_DSP      = 0x04,
        CTRL_CIR_ERROR_CRC_MAIN = 0x06,
        CTRL_CIR_ERROR_DL_MAIN  = 0x07,
        CTRL_CIR_ERROR_CRC_DSP  = 0x08,
        CTRL_CIR_ERROR_DL_DSP   = 0x09,
        CTRL_CIR_ERROR_ASIC     = 0x0a,
        CTRL_CIR_ERROR_DRAM     = 0x0b,
        CTRL_CIR_ERROR_DSPCOMM  = 0x29,
};

enum {
        MECH_ERROR_HEAD_UP            = 0x01,
        MECH_ERROR_HEAD_DOWN          = 0x02,
        MECH_ERROR_MAIN_PINCH_UP      = 0x03,
        MECH_ERROR_MAIN_PINCH_DOWN    = 0x04,
        MECH_ERROR_SUB_PINCH_UP       = 0x05,
        MECH_ERROR_SUB_PINCH_DOWN     = 0x06,
        MECH_ERROR_FEEDIN_PINCH_UP    = 0x07,
        MECH_ERROR_FEEDIN_PINCH_DOWN  = 0x08,
        MECH_ERROR_FEEDOUT_PINCH_UP   = 0x09,
        MECH_ERROR_FEEDOUT_PINCH_DOWN = 0x0a,
        MECH_ERROR_CUTTER_LR          = 0x0b,
        MECH_ERROR_CUTTER_RL          = 0x0c,
};

enum {
        SENSOR_ERROR_CUTTER           = 0x05,
        SENSOR_ERROR_HEAD_DOWN        = 0x09,
        SENSOR_ERROR_HEAD_UP          = 0x0a,
        SENSOR_ERROR_MAIN_PINCH_DOWN  = 0x0b,
        SENSOR_ERROR_MAIN_PINCH_UP    = 0x0c,
        SENSOR_ERROR_FEED_PINCH_DOWN  = 0x0d,
        SENSOR_ERROR_FEED_PINCH_UP    = 0x0e,
        SENSOR_ERROR_EXIT_PINCH_DOWN  = 0x0f,
        SENSOR_ERROR_EXIT_PINCH_UP    = 0x10,
        SENSOR_ERROR_LEFT_CUTTER      = 0x11,
        SENSOR_ERROR_RIGHT_CUTTER     = 0x12,
        SENSOR_ERROR_CENTER_CUTTER    = 0x13,
        SENSOR_ERROR_UPPER_CUTTER     = 0x14,
        SENSOR_ERROR_PAPER_FEED_COVER = 0x15,
};

enum {
        TEMP_SENSOR_ERROR_HEAD_HIGH = 0x01,
        TEMP_SENSOR_ERROR_HEAD_LOW  = 0x02,
        TEMP_SENSOR_ERROR_ENV_HIGH  = 0x03,
        TEMP_SENSOR_ERROR_ENV_LOW   = 0x04,
};

enum {
        COVER_OPEN_ERROR_UPPER = 0x01,
        COVER_OPEN_ERROR_LOWER = 0x02,
};

enum {
        PAPER_EMPTY_ERROR = 0x00,
};

enum {
        RIBBON_ERROR = 0x00,
};

enum {
        CURVE_TABLE_STATUS_INITIAL = 0x00,
        CURVE_TABLE_STATUS_USERSET = 0x01,
        CURVE_TABLE_STATUS_CURRENT = 0x02,
};

struct kodak6800_printsize {
	uint8_t  hdr;    /* Always 0x06 */
	uint16_t width;  /* BE */
	uint16_t height; /* BE */
	uint8_t  type;   /* MEDIA_TYPE_* [ ie paper ] */
	uint8_t  code;   /* 00, 01, 02, 03, 04, 05 seen. An index? */
	uint8_t  code2;  /* 00, 01 seen. Seems to be 1 only after a 4x6 printed.  */
	uint8_t  null[2];
} __attribute__((packed));

#define MAX_MEDIA_LEN 128

struct kodak68x0_media_readback {
	uint8_t  hdr;      /* Always 0x01 */
	uint8_t  media;    /* Always 0x00 (none), 0x0b or 0x03 */
	uint8_t  null[5];
	uint8_t  count;    /* Always 0x04 (6800) or 0x06 (6850)? */
	struct kodak6800_printsize sizes[];
} __attribute__((packed));

#define KODAK68x0_MEDIA_6R   0x0b
#define KODAK68x0_MEDIA_UNK  0x03
#define KODAK68x0_MEDIA_NONE 0x00

#define CMDBUF_LEN 17

/* Private data stucture */
struct kodak6800_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;

	int type;

	uint8_t jobid;

	struct kodak68x0_media_readback *media;

	struct kodak6800_hdr hdr;
	uint8_t *databuf;
	int datalen;
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

static void kodak68x0_dump_mediainfo(struct kodak68x0_media_readback *media)
{
	int i;
	if (media->media == KODAK68x0_MEDIA_NONE) {
		INFO("No Media Loaded\n");
		return;
	}

	if (media->media == KODAK68x0_MEDIA_6R) {
		INFO("Media type: 6R (Kodak 197-4096 or equivalent)\n");
	} else {
		INFO("Media type %02x (unknown, please report!)\n", media->media);
	}
	INFO("Legal print sizes:\n");
	for (i = 0 ; i < media->count ; i++) {
		INFO("\t%d: %dx%d (%02x) %s\n", i,
		      be16_to_cpu(media->sizes[i].width),
		      be16_to_cpu(media->sizes[i].height),
		      media->sizes[i].code,
		      media->sizes[i].code2? "Disallowed" : "");
	}
	INFO("\n");
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

	/* Issue command and get response */
	if ((ret = kodak6800_do_cmd(ctx, req, sizeof(req),
				    media, MAX_MEDIA_LEN,
				    &num)))
		return ret;

	/* Validate proper response */
	if (media->hdr != CMD_CODE_OK ||
	    media->null[0] != 0x00) {
		ERROR("Unexpected response from media query!\n");
		return CUPS_BACKEND_STOP;
	}

	return 0;
}

static int kodak68x0_canceljob(struct kodak6800_ctx *ctx,
			       int id)
{
	uint8_t req[16];
	int ret, num;
	struct kodak68x0_status_readback sts;

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
				    &sts, sizeof(sts),
				    &num)))
		return ret;

	/* Validate proper response */
	if (sts.hdr != CMD_CODE_OK) {
		ERROR("Unexpected response from job cancel!\n");
		return -99;
	}

	return 0;
}

/* Structure dumps */
static char *kodak68x0_status_str(struct kodak68x0_status_readback *resp)
{
        switch(resp->status1) {
        case STATE_STATUS1_STANDBY:
                return "Standby (Ready)";
        case STATE_STATUS1_WAIT:
                switch (be32_to_cpu(resp->status2)) {
                case WAIT_STATUS2_INIT:
                        return "Wait (Initializing)";
                case WAIT_STATUS2_RIBBON:
                        return "Wait (Ribbon Winding)";
                case WAIT_STATUS2_THERMAL:
                        return "Wait (Thermal Protection)";
                case WAIT_STATUS2_OPERATING:
                        return "Wait (Operating)";
                case WAIT_STATUS2_BUSY:
                        return "Wait (Busy)";
                default:
                        return "Wait (Unknown)";
                }
        case STATE_STATUS1_ERROR:
                switch (be32_to_cpu(resp->status2)) {
                case ERROR_STATUS2_CTRL_CIRCUIT:
                        switch (resp->errcode) {
                        case CTRL_CIR_ERROR_EEPROM1:
                                return "Error (EEPROM1)";
                        case CTRL_CIR_ERROR_EEPROM2:
                                return "Error (EEPROM2)";
                        case CTRL_CIR_ERROR_DSP:
                                return "Error (DSP)";
                        case CTRL_CIR_ERROR_CRC_MAIN:
                                return "Error (Main CRC)";
                        case CTRL_CIR_ERROR_DL_MAIN:
                                return "Error (Main Download)";
                        case CTRL_CIR_ERROR_CRC_DSP:
                                return "Error (DSP CRC)";
                        case CTRL_CIR_ERROR_DL_DSP:
                                return "Error (DSP Download)";
                        case CTRL_CIR_ERROR_ASIC:
                                return "Error (ASIC)";
                        case CTRL_CIR_ERROR_DRAM:
                                return "Error (DRAM)";
                        case CTRL_CIR_ERROR_DSPCOMM:
                                return "Error (DSP Communincation)";
                        default:
                                return "Error (Unknown Circuit)";
                        }
                case ERROR_STATUS2_MECHANISM_CTRL:
                        switch (resp->errcode) {
                        case MECH_ERROR_HEAD_UP:
                                return "Error (Head Up Mechanism)";
                        case MECH_ERROR_HEAD_DOWN:
                                return "Error (Head Down Mechanism)";
                        case MECH_ERROR_MAIN_PINCH_UP:
                                return "Error (Main Pinch Up Mechanism)";
                        case MECH_ERROR_MAIN_PINCH_DOWN:
                                return "Error (Main Pinch Down Mechanism)";
                        case MECH_ERROR_SUB_PINCH_UP:
                                return "Error (Sub Pinch Up Mechanism)";
                        case MECH_ERROR_SUB_PINCH_DOWN:
                                return "Error (Sub Pinch Down Mechanism)";
                        case MECH_ERROR_FEEDIN_PINCH_UP:
                                return "Error (Feed-in Pinch Up Mechanism)";
                        case MECH_ERROR_FEEDIN_PINCH_DOWN:
                                return "Error (Feed-in Pinch Down Mechanism)";
                        case MECH_ERROR_FEEDOUT_PINCH_UP:
                                return "Error (Feed-out Pinch Up Mechanism)";
                        case MECH_ERROR_FEEDOUT_PINCH_DOWN:
                                return "Error (Feed-out Pinch Down Mechanism)";
                        case MECH_ERROR_CUTTER_LR:
                                return "Error (Left->Right Cutter)";
                        case MECH_ERROR_CUTTER_RL:
                                return "Error (Right->Left Cutter)";
                        default:
                                return "Error (Unknown Mechanism)";
                        }
                case ERROR_STATUS2_SENSOR:
                        switch (resp->errcode) {
                        case SENSOR_ERROR_CUTTER:
                                return "Error (Cutter Sensor)";
                        case SENSOR_ERROR_HEAD_DOWN:
                                return "Error (Head Down Sensor)";
                        case SENSOR_ERROR_HEAD_UP:
                                return "Error (Head Up Sensor)";
                        case SENSOR_ERROR_MAIN_PINCH_DOWN:
                                return "Error (Main Pinch Down Sensor)";
                        case SENSOR_ERROR_MAIN_PINCH_UP:
                                return "Error (Main Pinch Up Sensor)";
                        case SENSOR_ERROR_FEED_PINCH_DOWN:
                                return "Error (Feed Pinch Down Sensor)";
                        case SENSOR_ERROR_FEED_PINCH_UP:
                                return "Error (Feed Pinch Up Sensor)";
                        case SENSOR_ERROR_EXIT_PINCH_DOWN:
                                return "Error (Exit Pinch Up Sensor)";
                        case SENSOR_ERROR_EXIT_PINCH_UP:
                                return "Error (Exit Pinch Up Sensor)";
                        case SENSOR_ERROR_LEFT_CUTTER:
                                return "Error (Left Cutter Sensor)";
                        case SENSOR_ERROR_RIGHT_CUTTER:
                                return "Error (Right Cutter Sensor)";
                        case SENSOR_ERROR_CENTER_CUTTER:
                                return "Error (Center Cutter Sensor)";
                        case SENSOR_ERROR_UPPER_CUTTER:
                                return "Error (Upper Cutter Sensor)";
                        case SENSOR_ERROR_PAPER_FEED_COVER:
                                return "Error (Paper Feed Cover)";
                        default:
                                return "Error (Unknown Sensor)";
                        }
                case ERROR_STATUS2_COVER_OPEN:
                        switch (resp->errcode) {
                        case COVER_OPEN_ERROR_UPPER:
                                return "Error (Upper Cover Open)";
                        case COVER_OPEN_ERROR_LOWER:
                                return "Error (Lower Cover Open)";
                        default:
                                return "Error (Unknown Cover Open)";
                        }
                case ERROR_STATUS2_TEMP_SENSOR:
                        switch (resp->errcode) {
                        case TEMP_SENSOR_ERROR_HEAD_HIGH:
                                return "Error (Head Temperature High)";
                        case TEMP_SENSOR_ERROR_HEAD_LOW:
                                return "Error (Head Temperature Low)";
                        case TEMP_SENSOR_ERROR_ENV_HIGH:
                                return "Error (Environmental Temperature High)";
                        case TEMP_SENSOR_ERROR_ENV_LOW:
                                return "Error (Environmental Temperature Low)";
                        default:
                                return "Error (Unknown Temperature)";
                        }
                case ERROR_STATUS2_PAPER_JAM:
                        return "Error (Paper Jam)";
                case ERROR_STATUS2_PAPER_EMPTY:
                        return "Error (Paper Empty)";
                case ERROR_STATUS2_RIBBON_ERR:
                        return "Error (Ribbon)";
                default:
                        return "Error (Unknown)";
                }
        default:
                return "Unknown!";
        }
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
             kodak68x0_status_str(status),
             status->status1, be32_to_cpu(status->status2), status->errcode);

	INFO("Bank 1 ID: %d\n", status->b1_jobid);
	INFO("\tPrints:  %d/%d complete\n",
	     be16_to_cpu(status->b1_complete), be16_to_cpu(status->b1_total));
	INFO("Bank 2 ID: %d\n", status->b2_jobid);
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
	INFO("\tLifetime     :  %d\n", be32_to_cpu(status->lifetime));
	INFO("\tThermal Head :  %d\n", be32_to_cpu(status->maint));
	INFO("\tCutter       :  %d\n", be32_to_cpu(status->cutter));

	if (ctx->type == P_KODAK_6850) {
		int max;

		INFO("\tMedia        :  %d\n", be32_to_cpu(status->media));

		if (ctx->media->media == KODAK68x0_MEDIA_6R) {
			max = 375;
		} else {
			max = 0;
		}

		if (max) {
			INFO("\t  Remaining  : %d\n", max - be32_to_cpu(status->media));
		} else {
			INFO("\t  Remaining  : Unknown\n");
		}
	}
	INFO("Main FW version: %d\n", be16_to_cpu(status->main_fw));
	INFO("DSP FW version : %d\n", be16_to_cpu(status->dsp_fw));
	INFO("Donor          : %d%%\n", status->donor);
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

	return 0;
}


#define UPDATE_SIZE 1536
static int kodak6800_get_tonecurve(struct kodak6800_ctx *ctx, char *fname)
{
	uint8_t cmdbuf[16];
	uint8_t respbuf[64];
	int ret, num = 0;
	int i;

	uint16_t *data = malloc(UPDATE_SIZE);
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
	cmdbuf[11] = 0x01;
	cmdbuf[12] = 0x00;
	cmdbuf[13] = 0x00;
	cmdbuf[14] = 0x00;
	cmdbuf[15] = 0x00;

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
			write(tc_fd, &data[i], sizeof(uint16_t));
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

	uint16_t *data = malloc(UPDATE_SIZE);
	uint8_t *ptr;

	if (!data) {
		ERROR("Memory Allocation Failure\n");
		return -1;
	}

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
	remain = UPDATE_SIZE;
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

static int kodak6850_send_init(struct kodak6800_ctx *ctx)
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

	// XXX I believe this the media position
	//     saying when we have a 4x6 left on an 8x6 blank
	if (rdbuf[1] != 0x01 && rdbuf[1] != 0x00) {
		ERROR("Unexpected status code (0x%02x)!\n", rdbuf[1]);
		return CUPS_BACKEND_FAILED;
	}
	return ret;
}

static void kodak6800_cmdline(void)
{
	DEBUG("\t\t[ -c filename ]  # Get tone curve\n");
	DEBUG("\t\t[ -C filename ]  # Set tone curve\n");
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -X jobid ]     # Cancel Job\n");	
}

static int kodak6800_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct kodak6800_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "C:c:msX:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'c':
			j = kodak6800_get_tonecurve(ctx, optarg);
			break;
		case 'C':
			j = kodak6800_set_tonecurve(ctx, optarg);
			break;
		case 'm':
			kodak68x0_dump_mediainfo(ctx->media);
			break;
		case 's': {
			struct kodak68x0_status_readback status;
			j = kodak6800_get_status(ctx, &status);
			if (!j)
				kodak68x0_dump_status(ctx, &status);
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

	ctx->media = malloc(MAX_MEDIA_LEN);

	ctx->type = P_ANY;

	return ctx;
}

static void kodak6800_attach(void *vctx, struct libusb_device_handle *dev, 
			      uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct kodak6800_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);

	ctx->type = lookup_printer_type(&kodak6800_backend,
					desc.idVendor, desc.idProduct);

        /* Ensure jobid is sane */
        ctx->jobid = (jobid & 0x7f) + 1;

	/* Query media info */
	if (kodak6800_get_mediainfo(ctx, ctx->media)) {
		ERROR("Can't query media\n");
	}
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

static int kodak6800_main_loop(void *vctx, int copies) {
	struct kodak6800_ctx *ctx = vctx;
	struct kodak68x0_status_readback status;

	int num, ret;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

        /* Fix max print count. */
        if (copies > 9999) // XXX test against remaining media
                copies = 9999;

	/* Printer handles generating copies.. */
	ctx->hdr.copies = cpu_to_be16(uint16_to_packed_bcd(copies));

	/* Validate media */
	if (ctx->media->media != KODAK68x0_MEDIA_6R &&
	    ctx->media->media != KODAK68x0_MEDIA_UNK) {
		ERROR("Unrecognized media type %02x\n", ctx->media->media);
		return CUPS_BACKEND_STOP;
	}

	/* Validate against supported media list */
	for (num = 0 ; num < ctx->media->count; num++) {
		if (ctx->media->sizes[num].height == ctx->hdr.rows &&
		    ctx->media->sizes[num].width == ctx->hdr.columns &&
		    ctx->media->sizes[num].code2 == 0x00)
			break;
	}
	if (num == ctx->media->count) {
		ERROR("Print size unsupported by media!\n");
		return CUPS_BACKEND_HOLD;
	}

	INFO("Waiting for printer idle\n");

	while(1) {
		if (kodak6800_get_status(ctx, &status))
			return CUPS_BACKEND_FAILED;

		if (status.status1 == STATE_STATUS1_ERROR) {
			INFO("Printer State: %s # %02x %08x %02x\n",
				kodak68x0_status_str(&status),
				status.status1, be32_to_cpu(status.status2), status.errcode);
			return CUPS_BACKEND_FAILED;
		}

		if (status.status == STATUS_IDLE)
			break;

		/* See if we have an open bank */
                if (!status.b1_remain ||
                    !status.b2_remain)
                        break;

		sleep(1);
	}

	if (ctx->type == P_KODAK_6850) {
//		INFO("Sending 6850 init sequence\n");
		ret = kodak6850_send_init(ctx);
		if (ret)
			return ret;
	}

	ctx->hdr.jobid = ctx->jobid;

#if 0
	/* If we want to disable 4x6 rewind on 8x6 media.. */
	if (ctx->hdr.size == 0x00 &&
	    be16_to_cpu(ctx->media->sizes[0].width) == 0x0982) {
		ctx->hdr.size = 0x06;
		ctx->hdr.mode = 0x01;
	}
#endif

	INFO("Initiating Print Job\n");
	if ((ret = kodak6800_do_cmd(ctx, (uint8_t*) &ctx->hdr, sizeof(ctx->hdr),
				    &status, sizeof(status),
				    &num)))
		return ret;

	if (status.hdr != CMD_CODE_OK) {
		ERROR("Unexpected response from print command!\n");
		return CUPS_BACKEND_FAILED;
	}

//	sleep(1); // Appears to be necessary for reliability
	INFO("Sending image data\n");
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     ctx->databuf, ctx->datalen)))
		return CUPS_BACKEND_FAILED;

	INFO("Waiting for printer to acknowledge completion\n");
	do {
		sleep(1);
		if (kodak6800_get_status(ctx, &status))
			return CUPS_BACKEND_FAILED;

		if (status.status1 == STATE_STATUS1_ERROR) {
			INFO("Printer State: %s # %02x %08x %02x\n",
				kodak68x0_status_str(&status),
				status.status1, be32_to_cpu(status.status2), status.errcode);
			return CUPS_BACKEND_FAILED;
		}

		/* If all prints are complete, we're done! */
		if (status.b1_jobid == ctx->hdr.jobid && status.b1_complete == status.b1_total)
			break;
		if (status.b2_jobid == ctx->hdr.jobid && status.b2_complete == status.b2_total)
			break;

		if (fast_return) {
			INFO("Fast return mode enabled.\n");
			break;
		}

	} while (1);

	INFO("Print complete\n");

	return CUPS_BACKEND_OK;
}

/* Exported */
struct dyesub_backend kodak6800_backend = {
	.name = "Kodak 6800/6850",
	.version = "0.51",
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

  All fields are BIG ENDIAN unless otherwise specified.

  Header:

  03 1b 43 48 43 0a 00 01        Fixed header
  NN NN                          Number of copies in BCD form (0001->9999)
  WW WW                          Number of columns (Fixed at 1844 on 6800)
  HH HH                          Number of rows.
  SS                             Print size -- 0x00 (4x6) 0x06 (8x6) 0x07 (5x7 on 6850)
  LL                             Laminate mode -- 0x00 (off) or 0x01 (on)
  UU                             Print mode -- 0x00 (normal) or (0x01) 4x6 on 8x6

  ************************************************************************

  Note:  6800 is Shinko CHC-S1145-5A, 6850 is Shinko CHC-S1145-5B

  Both are very similar to Shinko S1245!

  ************************************************************************

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

*/
