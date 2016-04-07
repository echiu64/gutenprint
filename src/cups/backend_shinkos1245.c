/*
 *   Shinko/Sinfonia CHC-S1245 CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2015 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Low-level documentation was provided by Sinfonia, Inc.  Thank you!
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

#define BACKEND shinkos1245_backend

#include "backend_common.h"

/* Structure of printjob header.  All fields are LITTLE ENDIAN */
struct s1245_printjob_hdr {
	uint32_t len1;   /* Fixed at 0x10 */
	uint32_t model;  /* Equal to the printer model (eg '1245' or '2145' decimal) */
	uint32_t unk2;   /* Null */
	uint32_t unk3;   /* Fixed at 0x01 */

	uint32_t len2;   /* Fixed at 0x64 */
	uint32_t unk5;   /* Null */
	uint32_t media;  /* Fixed at 0x10 */
	uint32_t unk6;   /* Null */

	uint32_t method; /* Print Method */
	uint32_t mode;   /* Print Mode */
	uint32_t unk7;   /* Null */
	 int32_t mattedepth; /* 0x7fffffff for glossy, 0x00 +- 25 for matte */

	uint32_t dust;   /* Dust control */
	uint32_t columns;
	uint32_t rows;
	uint32_t copies;

	uint32_t unk10;  /* Null */
	uint32_t unk11;  /* Null */
	uint32_t unk12;  /* Null */
	uint32_t unk13;  /* 0xceffffff */

	uint32_t unk14;  /* Null */
	uint32_t unk15;  /* 0xceffffff */
	uint32_t dpi; /* Fixed at '300' (decimal) */
	uint32_t unk16;  /* 0xceffffff */

	uint32_t unk17;  /* Null */
	uint32_t unk18;  /* 0xceffffff */
	uint32_t unk19;  /* Null */
	uint32_t unk20;  /* Null */

	uint32_t unk21;  /* Null */
} __attribute__((packed));

/* Printer data structures */
struct shinkos1245_cmd_hdr {
	uint8_t prefix; /* 0x03 */
	uint8_t hdr[4]; /* 0x1b 0x43 0x48 0x43 */
} __attribute__((packed));

/* Get Printer ID */
struct shinkos1245_cmd_getid {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[1]; /* 0x12 */
	uint8_t pad[11];
} __attribute__((packed));

struct shinkos1245_resp_getid {
	uint8_t id;       /* 0x00 */
	uint8_t data[23]; /* padded with 0x20 (space) */
	uint8_t reserved[8]; // XXX actual serial number?
} __attribute__((packed));

/* Set Printer ID -- Returns Status */
struct shinkos1245_cmd_setid {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[2];   /* 0x0a 0x22 */
	uint8_t id;       /* 0x00 */
	uint8_t data[23]; /* pad with 0x20 (space) */
} __attribute__((packed));

/* Print -- Returns Status */
struct shinkos1245_cmd_print {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t  cmd[2];   /* 0x0a 0x00 */
	uint8_t  id;       /* 1-255 */
	uint16_t count;    /* # Copies in BCD, 1-9999 */
	uint16_t columns;  /* Fixed at 2446 */
	uint16_t rows;
	uint8_t  media;    /* Fixed at 0x10 */
	uint8_t  mode;     /* dust removal and lamination mode */
	uint8_t  combo;    /* aka "print method" in the spool file */
} __attribute__((packed));

/* Get Status */
struct shinkos1245_cmd_getstatus {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[1];   /* 0x03 */
	uint8_t pad[10]; 
} __attribute__((packed));

struct shinkos1245_resp_status {
	uint8_t  code;
	uint8_t  print_status;
	struct {
		uint8_t  status1;
		uint32_t status2; /* BE */
		uint8_t  error;
	} state;
	struct {
		uint32_t lifetime;  /* BE */
		uint32_t maint;     /* BE */
		uint32_t media;     /* BE */
		uint32_t cutter;    /* BE */
		uint8_t  reserved;
		uint8_t  ver_boot;
		uint8_t  ver_ctrl;
		uint8_t  control_flag; // 0x00 == epson, 0x01 == cypress
	} counters;
	struct {
		uint16_t main_boot;
		uint16_t main_control;
		uint16_t dsp_boot;
		uint16_t dsp_control;
	} versions;
	struct {
		uint8_t  bank1_id;
		uint8_t  bank2_id;
		uint16_t bank1_remain;   /* BE */
		uint16_t bank1_complete; /* BE */
		uint16_t bank1_spec;     /* BE */
		uint16_t bank2_remain;   /* BE */
		uint16_t bank2_complete; /* BE */
		uint16_t bank2_spec;     /* BE */
	} counters2;
	uint8_t curve_status;
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

// XXX Paper jam has 0x01 -> 0xff as error codes

/* Query media info */
struct shinkos1245_cmd_getmedia {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[1];   /* 0x1a/0x2a/0x3a for A/B/C */
	uint8_t pad[10];
} __attribute__((packed));

struct shinkos1245_mediadesc {
	uint8_t  code;  /* Fixed at 0x10 */
	uint16_t columns; /* BE */
	uint16_t rows;    /* BE */
	uint8_t  type;       /* MEDIA_TYPE_* */
	uint8_t  print_type; /* aka "print method" in the spool file */
	uint8_t  reserved[3];
} __attribute__((packed));

#define NUM_MEDIAS 5 /* Maximum per message */

struct shinkos1245_resp_media {
	uint8_t  code;
	uint8_t  reserved[5];
	uint8_t  count;  /* 1-5? */
	struct shinkos1245_mediadesc data[NUM_MEDIAS];
} __attribute__((packed));

enum {
	MEDIA_TYPE_UNKNOWN = 0x00,
	MEDIA_TYPE_PAPER = 0x01,
};

enum {
	PRINT_TYPE_STANDARD = 0x00,
	PRINT_TYPE_8x5_2up  = 0x01,
	PRINT_TYPE_8x4_2up  = 0x02,
	PRINT_TYPE_8x6_8x4  = 0x03,
	PRINT_TYPE_8x5      = 0x04,
	PRINT_TYPE_8x4      = 0x05,
	PRINT_TYPE_8x6      = 0x06,
	PRINT_TYPE_8x6_2up  = 0x07,
	PRINT_TYPE_8x4_3up  = 0x08,
	PRINT_TYPE_8x8      = 0x09,
};

/* Cancel Job -- returns Status */
struct shinkos1245_cmd_canceljob {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[1];   /* 0x13 */
	uint8_t id;       /* 1-255 */
	uint8_t pad[9];
} __attribute__((packed));

/* Reset printer -- returns Status */
struct shinkos1245_cmd_reset {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[1];   /* 0xc0 */
	uint8_t pad[10];
} __attribute__((packed));

/* Tone curve manipulation -- returns Status */
struct shinkos1245_cmd_tone {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[1];   /* 0xc0 */
	uint8_t tone[4];  /* 0x54 0x4f 0x4e 0x45 */
	uint8_t cmd2[1];  /* 0x72/0x77/0x65/0x20 for read/write/end/data */
	union {
		struct {
			uint8_t tone_table;
			uint8_t param_table;
			uint8_t pad[3];
		} read_write;
		struct {
			uint8_t pad[5];
		} end_data;
	};
} __attribute__((packed));

enum {
	TONE_TABLE_STANDARD = 0,
	TONE_TABLE_USER = 1,
	TONE_TABLE_CURRENT = 2,
};
enum {
	PARAM_TABLE_STANDARD = 1,
	PARAM_TABLE_FINE = 2,
};

#define TONE_CURVE_SIZE 1536
#define TONE_CURVE_DATA_BLOCK_SIZE 64
   
/* Query Model information */
struct shinkos1245_cmd_getmodel {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[1];   /* 0x02 */
	uint8_t pad[10];
} __attribute__((packed));

struct shinkos1245_resp_getmodel {
	uint8_t vendor_id[4];
	uint8_t product_id[4];
	uint8_t strings[40];
} __attribute__((packed));


/* Query and Set Matte info, returns a Matte response */
struct shinkos1245_cmd_getmatte {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[1]; /* 0x20 */
	uint8_t mode;   /* Fixed at 0x00 */
	uint8_t pad[9];
} __attribute__((packed));

struct shinkos1245_cmd_setmatte {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[1]; /* 0x21 */
	uint8_t mode;   /* Fixed at 0x00 */
	 int8_t level;  /* -25->+25 */
	uint8_t pad[8];
} __attribute__((packed));

struct shinkos1245_resp_matte {
	uint8_t code;
	uint8_t mode;
	 int8_t level;
	uint8_t reserved[3];
} __attribute__((packed));

#define MATTE_MODE_MATTE 0x00

/* Private data stucture */
struct shinkos1245_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	uint8_t jobid;

	struct s1245_printjob_hdr hdr;

	struct shinkos1245_mediadesc medias[15];
	int num_medias;

	uint8_t *databuf;
	int datalen;
	int tonecurve;
};

enum {
	S_IDLE = 0,
	S_PRINTER_READY_CMD,
	S_PRINTER_SENT_DATA,
	S_FINISHED,
};


/* Basic printer I/O stuffs */
static void shinkos1245_fill_hdr(struct shinkos1245_cmd_hdr *hdr)
{
	hdr->prefix = 0x03;
	hdr->hdr[0] = 0x1b;
	hdr->hdr[1] = 0x43;
	hdr->hdr[2] = 0x48;
	hdr->hdr[3] = 0x43;
}

static int shinkos1245_do_cmd(struct shinkos1245_ctx *ctx,
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
	if (*actual_len < resp_len) {
		ERROR("Short read! (%d/%d))\n", *actual_len, resp_len);
		return -99;
	}

	return ret;
}

static int shinkos1245_get_status(struct shinkos1245_ctx *ctx,
				  struct shinkos1245_resp_status *resp)
{
	struct shinkos1245_cmd_getstatus cmd;
	int ret, num;

	shinkos1245_fill_hdr(&cmd.hdr);
	cmd.cmd[0] = 0x03;
	memset(cmd.pad, 0, sizeof(cmd.pad));

	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				resp, sizeof(*resp), &num);
	if (ret < 0) {
		ERROR("Failed to execute GET_STATUS command\n");
		return ret;
	}
	if (resp->code != CMD_CODE_OK) {
		ERROR("Bad return code on GET_STATUS (%02x)\n",
		      resp->code);
		return -99;
	}

	return 0;
}

static int shinkos1245_get_media(struct shinkos1245_ctx *ctx)
{
	struct shinkos1245_cmd_getmedia cmd;
	struct shinkos1245_resp_media resp;
	int i, j;
	int ret = 0, num;

	shinkos1245_fill_hdr(&cmd.hdr);
	memset(cmd.pad, 0, sizeof(cmd.pad));
	for (i = 1 ; i <= 3 ; i++) {
		cmd.cmd[0] = 0x0a || (i << 4);

		ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
					 &resp, sizeof(resp), &num);
		if (ret < 0) {
			ERROR("Failed to execute GET_MEDIA command\n");
			return ret;
		}
		if (resp.code != CMD_CODE_OK) {
			ERROR("Bad return code on GET_MEDIA (%02x)\n",
			      resp.code);
			return -99;
		}

		if (resp.count > NUM_MEDIAS)
			resp.count = NUM_MEDIAS;

		/* Store media info */
		for (j = 0; j < resp.count ; j++) {
			ctx->medias[ctx->num_medias].code = resp.data[j].code;
			ctx->medias[ctx->num_medias].columns = be16_to_cpu(resp.data[j].columns);
			ctx->medias[ctx->num_medias].rows = be16_to_cpu(resp.data[j].rows);
			ctx->medias[ctx->num_medias].type = resp.data[j].type;
			ctx->medias[ctx->num_medias].print_type = resp.data[j].print_type;
			ctx->num_medias++;
		}

		if (resp.count < 5)
			break;
	}
	return ret;
}

static int shinkos1245_get_printerid(struct shinkos1245_ctx *ctx,
				     struct shinkos1245_resp_getid *resp)
{
	struct shinkos1245_cmd_getid cmd;
	int ret, num;

	shinkos1245_fill_hdr(&cmd.hdr);
	cmd.cmd[0] = 0x12;
	memset(cmd.pad, 0, sizeof(cmd.pad));

	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				resp, sizeof(*resp), &num);
	if (ret < 0) {
		ERROR("Failed to execute GET_PRINTERID command\n");
		return ret;
	}

	return 0;
}

static int shinkos1245_set_printerid(struct shinkos1245_ctx *ctx,
				     char *id)
{
	struct shinkos1245_cmd_setid cmd;
	struct shinkos1245_resp_status sts;

	int ret, num;
	int i;

	shinkos1245_fill_hdr(&cmd.hdr);
	cmd.cmd[0] = 0x0a;
	cmd.cmd[1] = 0x22;

	for (i = 0 ; i < (int)sizeof(cmd.data) ; i++) {
		if (*id)
			cmd.data[i] = (uint8_t) *id;
		else
			cmd.data[i] = ' ';
	}
	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				 &sts, sizeof(sts), &num);
	if (ret < 0) {
		ERROR("Failed to execute SET_PRINTERID command\n");
		return ret;
	}
	if (sts.code != CMD_CODE_OK) {
		ERROR("Bad return code on SET_PRINTERID command\n");
		return -99;
	}
	return 0;
}

static int shinkos1245_canceljob(struct shinkos1245_ctx *ctx,
				 int id)
{
	struct shinkos1245_cmd_canceljob cmd;
	struct shinkos1245_resp_status sts;

	int ret, num;

	shinkos1245_fill_hdr(&cmd.hdr);
	cmd.cmd[0] = 0x13;
	cmd.id = id;

	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				 &sts, sizeof(sts), &num);
	if (ret < 0) {
		ERROR("Failed to execute CANCELJOB command\n");
		return ret;
	}
	if (sts.code != CMD_CODE_OK) {
		ERROR("Bad return code on CANCELJOB command\n");
		return -99;
	}
	return 0;
}

static int shinkos1245_set_matte(struct shinkos1245_ctx *ctx,
				 int intensity)
{
	struct shinkos1245_cmd_setmatte cmd;
	struct shinkos1245_resp_matte sts;

	int ret, num;

	shinkos1245_fill_hdr(&cmd.hdr);
	cmd.cmd[0] = 0x21;
	cmd.mode = MATTE_MODE_MATTE;
	cmd.level = intensity;

	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				 &sts, sizeof(sts), &num);
	if (ret < 0) {
		ERROR("Failed to execute SET_MATTE command\n");
		return ret;
	}
	if (sts.code == CMD_CODE_OK)
		return 0;
	if (sts.code == CMD_CODE_BAD)
		return 1;

	ERROR("Bad return code (%02x) on SET_MATTE command\n", sts.code);	
	return -99;
}

static int shinkos1245_get_matte(struct shinkos1245_ctx *ctx,
				 int *intensity)
{
	struct shinkos1245_cmd_getmatte cmd;
	struct shinkos1245_resp_matte sts;

	int ret, num;

	shinkos1245_fill_hdr(&cmd.hdr);
	cmd.cmd[0] = 0x20;
	cmd.mode = MATTE_MODE_MATTE;

	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				 &sts, sizeof(sts), &num);
	if (ret < 0) {
		ERROR("Failed to execute GET_MATTE command\n");
		return ret;
	}
	if (sts.code != CMD_CODE_OK) {
		ERROR("Bad return code (%02x) on GET_MATTE command\n", sts.code);
		return -99;
	}
	*intensity = sts.level;

	return 0;
}


/* Structure dumps */
static char *shinkos1245_status_str(struct shinkos1245_resp_status *resp)
{
	switch(resp->state.status1) {
	case STATE_STATUS1_STANDBY:
		return "Standby (Ready)";
	case STATE_STATUS1_WAIT:
		switch (resp->state.status2) {
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
		switch (resp->state.status2) {
		case ERROR_STATUS2_CTRL_CIRCUIT:
			switch (resp->state.error) {
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
			switch (resp->state.error) {
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
			switch (resp->state.error) {
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
			switch (resp->state.error) {
			case COVER_OPEN_ERROR_UPPER:
				return "Error (Upper Cover Open)";
			case COVER_OPEN_ERROR_LOWER:
				return "Error (Lower Cover Open)";
			default:
				return "Error (Unknown Cover Open)";
			}
		case ERROR_STATUS2_TEMP_SENSOR:
			switch (resp->state.error) {
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

static char* shinkos1245_tonecurves(int type, int table)
{
	switch (type) {
	case TONE_TABLE_STANDARD:
		switch (table) {
		case PARAM_TABLE_STANDARD:
			return "Standard/Standard";
		case PARAM_TABLE_FINE:
			return "Standard/Fine";
		default:
			return "Standard/Unknown";
		}
	case TONE_TABLE_USER:
		switch (table) {
		case PARAM_TABLE_STANDARD:
			return "User/Standard";
		case PARAM_TABLE_FINE:
			return "User/Fine";
		default:
			return "User/Unknown";
		}
	case TONE_TABLE_CURRENT:
		switch (table) {
		case PARAM_TABLE_STANDARD:
			return "Current/Standard";
		case PARAM_TABLE_FINE:
			return "Current/Fine";
		default:
			return "Current/Unknown";
		}
	default:
		return "Unknown";
	}
}

static void shinkos1245_dump_status(struct shinkos1245_resp_status *sts)
{
	char *detail;
	switch (sts->print_status) {
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
	INFO("Printer Status:  %s\n", detail);

	/* Byteswap */
	sts->state.status2 = be32_to_cpu(sts->state.status2);

	INFO("Printer State: %s # %02x %08x %02x\n",
	     shinkos1245_status_str(sts),
	     sts->state.status1, sts->state.status2, sts->state.error);
	INFO("Counters:\n");
	INFO("\tLifetime     :  %d\n", be32_to_cpu(sts->counters.lifetime));
	INFO("\tThermal Head :  %d\n", be32_to_cpu(sts->counters.maint));
	INFO("\tMedia        :  %d\n", be32_to_cpu(sts->counters.media));
	INFO("\tCutter       :  %d\n", be32_to_cpu(sts->counters.cutter));

	INFO("Versions:\n");
	INFO("\tUSB Boot    : %d\n", sts->counters.ver_boot);
	INFO("\tUSB Control : %d\n", sts->counters.ver_ctrl);
	INFO("\tMain Boot   : %d\n", be16_to_cpu(sts->versions.main_boot));
	INFO("\tMain Control: %d\n", be16_to_cpu(sts->versions.main_control));
	INFO("\tDSP Boot    : %d\n", be16_to_cpu(sts->versions.dsp_boot));
	INFO("\tDSP Control : %d\n", be16_to_cpu(sts->versions.dsp_control));

//	INFO("USB TypeFlag: %02x\n", sts->counters.control_flag);

	INFO("Bank 1 ID: %d\n", sts->counters2.bank1_id);
	INFO("\tPrints:  %d/%d complete\n",
	     be16_to_cpu(sts->counters2.bank1_complete),
	     be16_to_cpu(sts->counters2.bank1_spec));
	INFO("Bank 2 ID: %d\n", sts->counters2.bank2_id);
	INFO("\tPrints:  %d/%d complete\n",
	     be16_to_cpu(sts->counters2.bank2_complete),
	     be16_to_cpu(sts->counters2.bank2_spec));

	switch (sts->curve_status) {
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
}

static void shinkos1245_dump_media(struct shinkos1245_mediadesc *medias,
				   int count)
{
	int i;

	INFO("Supported print sizes: %d\n", count);

	for (i = 0 ; i < count ; i++) {
		INFO("\t %02x: %04d*%04d (%02x/%02d)\n",
		     medias[i].print_type,
		     medias[i].columns,
		     medias[i].rows,
		     medias[i].code, medias[i].type);
	}
}

static int get_tonecurve(struct shinkos1245_ctx *ctx, int type, int table, char *fname)
{
	int ret = 0, num, remaining;
	uint8_t *data, *ptr;

	struct shinkos1245_cmd_tone cmd;
	struct shinkos1245_resp_status resp;

	INFO("Dump %s Tone Curve to '%s'\n", shinkos1245_tonecurves(type, table), fname);

	/* Issue a tone_read_start */
	shinkos1245_fill_hdr(&cmd.hdr);
	cmd.cmd[0] = 0x0c;
	cmd.tone[0] = 0x54;
	cmd.tone[1] = 0x4f;
	cmd.tone[2] = 0x4e;
	cmd.tone[3] = 0x45;
	cmd.cmd2[0] = 0x72;
	cmd.read_write.tone_table = type;
	cmd.read_write.param_table = table;

	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				&resp, sizeof(resp), &num);

	if (ret < 0) {
		ERROR("Failed to execute TONE_READ command\n");
		return ret;
	}
	if (resp.code != CMD_CODE_OK) {
		ERROR("Bad return code on TONE_READ (%02x)\n",
		      resp.code);
		return -99;
	}

	/* Get the data out */
	remaining = TONE_CURVE_SIZE;
	data = malloc(remaining);
	if (!data) {
		ERROR("Memory Allocation Failure!\n");
		return -11;
	}
	ptr = data;

	while(remaining) {
		/* Issue a tone_data message */
		cmd.cmd2[0] = 0x20;

		ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
					 &resp, sizeof(resp), &num);

		if (ret < 0) {
			ERROR("Failed to execute TONE_DATA command\n");
			goto done;
		}
		if (resp.code != CMD_CODE_OK) {
			ERROR("Bad return code on TONE_DATA (%02x)\n",
			      resp.code);
			ret = -99;
			goto done;
		}

		/* And read back 64-bytes of data */
		ret = read_data(ctx->dev, ctx->endp_up,
				ptr, TONE_CURVE_DATA_BLOCK_SIZE, &num);
		if (num != TONE_CURVE_DATA_BLOCK_SIZE) {
			ret = -99;
			goto done;
		}
		if (ret < 0)
			goto done;
		ptr += num;
	}

	/* Issue a tone_end */
	cmd.cmd2[0] = 0x65;
	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				&resp, sizeof(resp), &num);

	if (ret < 0) {
		ERROR("Failed to execute TONE_END command\n");
		goto done;
	}
	if (resp.code != CMD_CODE_OK) {
		ERROR("Bad return code on TONE_END (%02x)\n",
		      resp.code);
		ret = -99;
		goto done;
	}

	/* Open file and write it out */
	{
		int tc_fd = open(fname, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
		if (tc_fd < 0) {
			ret = tc_fd;
			goto done;
		}

		ret = write(tc_fd, data, TONE_CURVE_SIZE);
		if (ret < 0)
			goto done;
		close(tc_fd);
	}

done:
	free(data);

	return ret;
}

static int set_tonecurve(struct shinkos1245_ctx *ctx, int type, int table, char *fname)
{
	int ret = 0, num, remaining;
	uint8_t *data, *ptr;

	struct shinkos1245_cmd_tone cmd;
	struct shinkos1245_resp_status resp;

	INFO("Read %d/%d Tone Curve from '%s'\n", type, table, fname); // XXX

	/* Allocate space */
	remaining = TONE_CURVE_SIZE;
	data = malloc(remaining);
	if (!data) {
		ERROR("Memory Allocation Failure!\n");
		return -11;
	}
	ptr = data;

	/* Open file and read it in */
	{
		int tc_fd = open(fname, O_RDONLY);
		if (tc_fd < 0) {
			ret = tc_fd;
			goto done;
		}

		ret = read(tc_fd, data, TONE_CURVE_SIZE);
		if (ret < 0) {
			close(tc_fd);
			goto done;
		}

		close(tc_fd);
	}

	/* Issue a tone_write_start */
	shinkos1245_fill_hdr(&cmd.hdr);
	cmd.cmd[0] = 0x0c;
	cmd.tone[0] = 0x54;
	cmd.tone[1] = 0x4f;
	cmd.tone[2] = 0x4e;
	cmd.tone[3] = 0x45;
	cmd.cmd2[0] = 0x77;
	cmd.read_write.tone_table = type;
	cmd.read_write.param_table = table;

	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				&resp, sizeof(resp), &num);

	if (ret < 0) {
		ERROR("Failed to execute TONE_WRITE command\n");
		goto done;
	}
	if (resp.code != CMD_CODE_OK) {
		ERROR("Bad return code on TONE_WRITE (%02x)\n",
		      resp.code);
		ret = -99;
		goto done;
	}

	while(remaining) {
		/* Issue a tone_data message */
		cmd.cmd2[0] = 0x20;

		ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
					 &resp, sizeof(resp), &num);

		if (ret < 0) {
			ERROR("Failed to execute TONE_DATA command\n");
			goto done;
		}
		if (resp.code != CMD_CODE_OK) {
			ERROR("Bad return code on TONE_DATA (%02x)\n",
			      resp.code);
			ret = -99;
			goto done;
		}

		/* Write 64-bytes of data */
		ret = send_data(ctx->dev, ctx->endp_up,
				ptr, TONE_CURVE_DATA_BLOCK_SIZE);
		if (ret < 0)
			goto done;
		ptr += num;
	}

	/* Issue a tone_end */
	cmd.cmd2[0] = 0x65;
	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				&resp, sizeof(resp), &num);

	if (ret < 0) {
		ERROR("Failed to execute TONE_END command\n");
		goto done;
	}
	if (resp.code != CMD_CODE_OK) {
		ERROR("Bad return code on TONE_END (%02x)\n",
		      resp.code);
		ret = -99;
		goto done;
	}

done:
	free(data);

	return ret;
}


/* Driver API */

static void shinkos1245_cmdline(void)
{
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -u ]           # Query user string\n");
	DEBUG("\t\t[ -U sometext ]  # Set user string\n");
	DEBUG("\t\t[ -X jobid ]     # Abort a printjob\n");
	DEBUG("\t\t[ -F ]           # Tone curve refers to FINE mode\n");
	DEBUG("\t\t[ -c filename ]  # Get user/NV tone curve\n");
	DEBUG("\t\t[ -C filename ]  # Set user/NV tone curve\n");
	DEBUG("\t\t[ -l filename ]  # Get current tone curve\n");
	DEBUG("\t\t[ -L filename ]  # Set current tone curve\n");
}

int shinkos1245_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct shinkos1245_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "c:C:l:L:FmsuU:X:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'F':
			ctx->tonecurve = PARAM_TABLE_FINE;
			break;
		case 'c':
			j = get_tonecurve(ctx, TONE_TABLE_USER, ctx->tonecurve, optarg);
			break;
		case 'C':
			j = set_tonecurve(ctx, TONE_TABLE_USER, ctx->tonecurve, optarg);
			break;
		case 'l':
			j = get_tonecurve(ctx, TONE_TABLE_CURRENT, ctx->tonecurve, optarg);
			break;
		case 'L':
			j = set_tonecurve(ctx, TONE_TABLE_CURRENT, ctx->tonecurve, optarg);
			break;
		case 'm':
			j = shinkos1245_get_media(ctx);
			if (!j)
				shinkos1245_dump_media(ctx->medias, ctx->num_medias);
			break;
		case 's': {
			struct shinkos1245_resp_status sts;
			j = shinkos1245_get_status(ctx, &sts);
			if (!j)
				shinkos1245_dump_status(&sts);
			break;
		}
		case 'u': {
			struct shinkos1245_resp_getid resp;
			j = shinkos1245_get_printerid(ctx, &resp);
			if (!j) {
				char buffer[sizeof(resp.data)+1];
				memcpy(buffer, resp.data, sizeof(resp.data));
				buffer[sizeof(resp.data)] = 0;
				INFO("Printer ID: %02x '%s'\n", resp.id, buffer);
			}
			break;
		}
		case 'U':
			j = shinkos1245_set_printerid(ctx, optarg);
			break;
		case 'X':
			j = shinkos1245_canceljob(ctx, atoi(optarg));
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
}

static void *shinkos1245_init(void)
{
	struct shinkos1245_ctx *ctx = malloc(sizeof(struct shinkos1245_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct shinkos1245_ctx));

	ctx->tonecurve = PARAM_TABLE_STANDARD;

	return ctx;
}

static void shinkos1245_attach(void *vctx, struct libusb_device_handle *dev, 
			       uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct shinkos1245_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);
	
	ctx->type = lookup_printer_type(&shinkos1245_backend,
					desc.idVendor, desc.idProduct);	

	/* Ensure jobid is sane */
	ctx->jobid = (jobid & 0x7f) + 1;
}


static void shinkos1245_teardown(void *vctx) {
	struct shinkos1245_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);

	free(ctx);
}

static int shinkos1245_read_parse(void *vctx, int data_fd) {
	struct shinkos1245_ctx *ctx = vctx;
	int ret;
	uint8_t tmpbuf[4];

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Read in then validate header */
	ret = read(data_fd, &ctx->hdr, sizeof(ctx->hdr));
	if (ret < 0)
		return ret;
	if (ret < 0 || ret != sizeof(ctx->hdr))
		return CUPS_BACKEND_CANCEL;

	if (le32_to_cpu(ctx->hdr.len1) != 0x10 ||
	    le32_to_cpu(ctx->hdr.len2) != 0x64 ||
	    le32_to_cpu(ctx->hdr.dpi) != 300) {
		ERROR("Unrecognized header data format!\n");
		return CUPS_BACKEND_CANCEL;
	}

	ctx->hdr.model = le32_to_cpu(ctx->hdr.model);

	if(ctx->hdr.model != 1245) {
		ERROR("Unrecognized printer (%d)!\n", ctx->hdr.model);
		return CUPS_BACKEND_CANCEL;
	}

	/* Finish byteswapping */
	ctx->hdr.media = le32_to_cpu(ctx->hdr.media);
	ctx->hdr.method = le32_to_cpu(ctx->hdr.method);
	ctx->hdr.mode = le32_to_cpu(ctx->hdr.mode);
	ctx->hdr.mattedepth = le32_to_cpu(ctx->hdr.mattedepth);
	ctx->hdr.dust = le32_to_cpu(ctx->hdr.dust);
	ctx->hdr.columns = le32_to_cpu(ctx->hdr.columns);
	ctx->hdr.rows = le32_to_cpu(ctx->hdr.rows);
	ctx->hdr.copies = le32_to_cpu(ctx->hdr.copies);

	/* Allocate space */
	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	ctx->datalen = ctx->hdr.rows * ctx->hdr.columns * 3;
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
				return ret;
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}

	/* Make sure footer is sane too */
	ret = read(data_fd, tmpbuf, 4);
	if (ret != 4) {
		ERROR("Read failed (%d/%d/%d)\n",
		      ret, 4, 4);
		perror("ERROR: Read failed");
		return ret;
	}
	if (tmpbuf[0] != 0x04 ||
	    tmpbuf[1] != 0x03 ||
	    tmpbuf[2] != 0x02 ||
	    tmpbuf[3] != 0x01) {
		ERROR("Unrecognized footer data format!\n");
		return CUPS_BACKEND_FAILED;
	}

	return CUPS_BACKEND_OK;
}

static int shinkos1245_main_loop(void *vctx, int copies) {
	struct shinkos1245_ctx *ctx = vctx;
	int i, num, last_state = -1, state = S_IDLE;
	struct shinkos1245_resp_status status1, status2;

	// XXX query printer info

	/* Query Media information if necessary */
	if (!ctx->num_medias)
		shinkos1245_get_media(ctx);
	if (!ctx->num_medias) {
		ERROR("Media Query Error\n");
		return CUPS_BACKEND_FAILED;
	}
	/* Make sure print size is supported */
	for (i = 0 ; i < ctx->num_medias ; i++) {
		if (ctx->hdr.media == ctx->medias[i].code &&
		    ctx->hdr.method == ctx->medias[i].print_type &&
		    ctx->hdr.rows == ctx->medias[i].rows &&
		    ctx->hdr.columns == ctx->medias[i].columns)
			break;
	}
	if (i == ctx->num_medias) {
		ERROR("Unsupported print type\n");
		return CUPS_BACKEND_HOLD;
	}

	/* Fix max print count. */
	if (copies > 9999) // XXX test against remaining media
		copies = 9999;

top:
	if (state != last_state) {
		if (dyesub_debug)
			DEBUG("last_state %d new %d\n", last_state, state);
	}

	/* Send status query */
	i = shinkos1245_get_status(ctx, &status1);
	if (i < 0)
		return CUPS_BACKEND_FAILED;

	if (memcmp(&status1, &status2, sizeof(status1))) {
		memcpy(&status2, &status1, sizeof(status1));
		// status changed, check for errors and whatnot
	} else if (state == last_state) {
		sleep(1);
		goto top;
	}

	/* Make sure we're not in an error state */
	if (status1.state.status1 == STATE_STATUS1_ERROR)
		goto printer_error;

	last_state = state;

	fflush(stderr);

	switch (state) {
	case S_IDLE:
		INFO("Waiting for printer idle\n");

		if (status1.state.status1 == STATE_STATUS1_STANDBY) {
			state = S_PRINTER_READY_CMD;
			break;
		}

		if (status1.print_status == STATUS_IDLE) {
			state = S_PRINTER_READY_CMD;
			break;
		}

		// XXX what about STATUS_WAIT ?
		// XXX see if printer has an empty bank?

		/* If the printer is "busy" check to see if there's any
		   open memory banks so we can queue the next print */
		if (!status1.counters2.bank1_remain ||
		    !status1.counters2.bank2_remain) {
			state = S_PRINTER_READY_CMD;
			break;
		}
		break;
	case S_PRINTER_READY_CMD: {
		struct shinkos1245_cmd_print cmd;

		/* Set matte intensity */
		if (ctx->hdr.mattedepth != 0x7fffffff) {
			int current = -1;
			i = shinkos1245_get_matte(ctx, &current);
			if (i < 0)
				goto printer_error;
			if (current != ctx->hdr.mattedepth) {
				i = shinkos1245_set_matte(ctx, ctx->hdr.mattedepth);
				if (i < 0)
					goto printer_error;
				if (i > 0) {
					INFO("Can't set matte intensity when printing in progres...\n");
					state = S_IDLE;
					sleep(1);
					break;
				}
			}
		}

		INFO("Initiating print job (internal id %d)\n", ctx->jobid);

		shinkos1245_fill_hdr(&cmd.hdr);
		cmd.cmd[0] = 0x0a;
		cmd.cmd[1] = 0x00;

		cmd.id = ctx->jobid;
		cmd.count = cpu_to_be16(uint16_to_packed_bcd(copies));
		cmd.columns = cpu_to_be16(ctx->hdr.columns);
		cmd.rows = cpu_to_be16(ctx->hdr.rows);
		cmd.media = ctx->hdr.media;
		cmd.mode = (ctx->hdr.mode & 0x3f) || ((ctx->hdr.dust & 0x3) << 6);
		cmd.combo = ctx->hdr.method;

		/* Issue print commmand */
		i = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				       &status1, sizeof(status1),
				       &num);
		if (i < 0)
			goto printer_error;

		/* Check for buffer full state, and wait if we're full */
		if (status1.code != CMD_CODE_OK) {
			if (status1.print_status == STATUS_PRINTING) {
				sleep(1);
				break;
			} else {
				goto printer_error;
			}
		}

		/* Check for error states */
		if (status1.state.status1 == STATE_STATUS1_ERROR)
			goto printer_error;

		/* Send over data */
		INFO("Sending image data to printer\n");
		if ((i = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf, ctx->datalen)))
			return CUPS_BACKEND_FAILED;

		INFO("Waiting for printer to acknowledge completion\n");
		sleep(1);
		state = S_PRINTER_SENT_DATA;
		break;
	}
	case S_PRINTER_SENT_DATA:
		if (fast_return) {
			INFO("Fast return mode enabled.\n");
			state = S_FINISHED;
		}
		/* Check for completion */
		if (status1.print_status == STATUS_IDLE)
			state = S_FINISHED;

		break;
	default:
		break;
	}

	if (state != S_FINISHED)
		goto top;

	INFO("Print complete\n");

	if (copies && --copies) {
		state = S_IDLE;
		goto top;
	}

	return CUPS_BACKEND_OK;

printer_error:
	/* Byteswap */
	status1.state.status2 = be32_to_cpu(status1.state.status2);

	ERROR("Printer Error: %s # %02x %08x %02x\n",
	      shinkos1245_status_str(&status1),
	      status1.state.status1, status1.state.status2, status1.state.error);

	return CUPS_BACKEND_FAILED;
}

static int shinkos1245_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct shinkos1245_resp_getid resp;
	int i;

	struct shinkos1245_ctx ctx = {
		.dev = dev,
		.endp_up = endp_up,
		.endp_down = endp_down,
	};

	i = shinkos1245_get_printerid(&ctx, &resp);
	if (i < 0)
		return CUPS_BACKEND_FAILED;

	for (i = 0 ; i < (int) sizeof(resp.data) && i < buf_len; i++) {
		buf[i] = resp.data[i];
	}

	/* Ensure null-termination */
	if (i < buf_len)
		buf[i] = 0;
	else
		buf[buf_len-1] = 0;

	return CUPS_BACKEND_OK;
}

/* Exported */
#define USB_VID_SHINKO       0x10CE
#define USB_PID_SHINKO_S1245 0x0007

struct dyesub_backend shinkos1245_backend = {
	.name = "Shinko/Sinfonia CHC-S1245",
	.version = "0.07WIP",
	.uri_prefix = "shinkos1245",
	.cmdline_usage = shinkos1245_cmdline,
	.cmdline_arg = shinkos1245_cmdline_arg,
	.init = shinkos1245_init,
	.attach = shinkos1245_attach,
	.teardown = shinkos1245_teardown,
	.read_parse = shinkos1245_read_parse,
	.main_loop = shinkos1245_main_loop,
	.query_serno = shinkos1245_query_serno,
	.devices = {
	{ USB_VID_SHINKO, USB_PID_SHINKO_S1245, P_SHINKO_S1245, ""},
	{ 0, 0, 0, ""}
	}
};

/* CHC-S1245 data format

  Spool file consists of an 116-byte header, followed by RGB-packed data,
  followed by a 4-byte footer.  Header appears to consist of a series of
  4-byte Little Endian words.

   10 00 00 00 MM MM 00 00  00 00 00 00 01 00 00 00  MM == Model (ie 1245d)
   64 00 00 00 00 00 00 00  TT 00 00 00 00 00 00 00  TT == Media Size (0x10 fixed)
   MM 00 00 00 PP 00 00 00  00 00 00 00 ZZ ZZ ZZ ZZ  MM = Print Method (aka cut control), PP = Default/Glossy/Matte (0x01/0x03/0x05), ZZ == matte intensity (0x7fffffff for glossy, else 0x00000000 +- 25 for matte)
   VV 00 00 00 WW WW 00 00  HH HH 00 00 XX 00 00 00  VV == dust; 0x00 default, 0x01 off, 0x02 on, XX == Copies
   00 00 00 00 00 00 00 00  00 00 00 00 ce ff ff ff
   00 00 00 00 ce ff ff ff  QQ QQ 00 00 ce ff ff ff  QQ == DPI, ie 300.
   00 00 00 00 ce ff ff ff  00 00 00 00 00 00 00 00
   00 00 00 00

   [[Packed RGB payload of WW*HH*3 bytes]]

   04 03 02 01  [[ footer ]]


*/
