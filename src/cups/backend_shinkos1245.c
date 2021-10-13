/*
 *   Shinko/Sinfonia CHC-S1245 CUPS backend -- libusb-1.0 version
 *
 *   (c) 2015-2021 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Low-level documentation was provided by Sinfonia, Inc.  Thank you!
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

#define BACKEND shinkos1245_backend

#include "backend_common.h"
#include "backend_sinfonia.h"

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
	} __attribute__((packed)) state;
	struct {
		uint32_t lifetime;  /* BE */
		uint32_t maint;     /* BE */
		uint32_t media;     /* BE */
		uint32_t cutter;    /* BE */
		uint8_t  reserved;
		uint8_t  ver_boot;
		uint8_t  ver_ctrl;
		uint8_t  control_flag; // 0x00 == epson, 0x01 == cypress
	} __attribute__((packed)) counters;
	struct {
		uint16_t main_boot;
		uint16_t main_control;
		uint16_t dsp_boot;
		uint16_t dsp_control;
	} __attribute__((packed)) versions;
	struct {
		uint8_t  bank1_id;
		uint8_t  bank2_id;
		uint16_t bank1_remain;   /* BE */
		uint16_t bank1_complete; /* BE */
		uint16_t bank1_spec;     /* BE */
		uint16_t bank2_remain;   /* BE */
		uint16_t bank2_complete; /* BE */
		uint16_t bank2_spec;     /* BE */
	} __attribute__((packed)) counters2;
	uint8_t curve_status;
} __attribute__((packed));

/* Query media info */
struct shinkos1245_cmd_getmedia {
	struct shinkos1245_cmd_hdr hdr;
	uint8_t cmd[1];   /* 0x1a/0x2a/0x3a for A/B/C */
	uint8_t pad[10];
} __attribute__((packed));

#define NUM_MEDIAS 5 /* Maximum per message */

struct shinkos1245_resp_media {
	uint8_t  code;
	uint8_t  reserved[6];
	uint8_t  count;  /* 1-5? */
	struct sinfonia_mediainfo_item data[NUM_MEDIAS];
} __attribute__((packed));


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
	uint8_t reserved[4];
} __attribute__((packed));

#define MATTE_MODE_MATTE 0x00
#define MAX_MEDIA_ITEMS 15

/* Private data structure */
struct shinkos1245_ctx {
	struct dyesub_connection *conn;

	uint8_t jobid;

	struct sinfonia_mediainfo_item medias[MAX_MEDIA_ITEMS];
	int num_medias;
	int media_8x12;

	char serial[32];
	char fwver[32];

	struct marker marker;

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
	if ((ret = send_data(ctx->conn,
			     cmd, cmd_len)))
		return (ret < 0) ? ret : -99;

	/* Read response */
	ret = read_data(ctx->conn,
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

	/* Byteswap important stuff */
        resp->state.status2 = be32_to_cpu(resp->state.status2);

	return CUPS_BACKEND_OK;
}

static int shinkos1245_get_media(struct shinkos1245_ctx *ctx)
{
	struct shinkos1245_cmd_getmedia cmd;
	struct shinkos1245_resp_media resp;
	int i, j;
	int ret = 0, num;

	shinkos1245_fill_hdr(&cmd.hdr);
	memset(cmd.pad, 0, sizeof(cmd.pad));
	ctx->media_8x12 = 0;
	for (i = 1 ; i <= 3 ; i++) {
		cmd.cmd[0] = 0x0a | (i << 4);

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

		/* Store media info */
		for (j = 0; j < NUM_MEDIAS && ctx->num_medias < resp.count ; j++) {
			ctx->medias[ctx->num_medias].code = resp.data[j].code;
			ctx->medias[ctx->num_medias].columns = be16_to_cpu(resp.data[j].columns);
			ctx->medias[ctx->num_medias].rows = be16_to_cpu(resp.data[j].rows);
			ctx->medias[ctx->num_medias].type = resp.data[j].type;
			ctx->medias[ctx->num_medias].method = resp.data[j].method;
			ctx->num_medias++;

			if (ctx->medias[i].rows >= 3636)
				ctx->media_8x12 = 1;
		}

		/* Once we've parsed them all.. we're done */
		if (ctx->num_medias == resp.count)
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

	return CUPS_BACKEND_OK;
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
			cmd.data[i] = (uint8_t) *id++;
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
	return CUPS_BACKEND_OK;
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
	return CUPS_BACKEND_OK;
}

static int shinkos1245_reset(struct shinkos1245_ctx *ctx)
{
	struct shinkos1245_cmd_reset cmd;
	struct shinkos1245_resp_status sts;

	int ret, num;

	shinkos1245_fill_hdr(&cmd.hdr);
	cmd.cmd[0] = 0xc0;

	ret = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				 &sts, sizeof(sts), &num);
	if (ret < 0) {
		ERROR("Failed to execute RESET command\n");
		return ret;
	}
	if (sts.code != CMD_CODE_OK) {
		ERROR("Bad return code on RESET command\n");
		return -99;
	}
	return CUPS_BACKEND_OK;
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
		return CUPS_BACKEND_OK;
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

	return CUPS_BACKEND_OK;
}

static const char* shinkos1245_tonecurves(int type, int table)
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

static void shinkos1245_dump_status(struct shinkos1245_ctx *ctx,
				    struct shinkos1245_resp_status *sts)
{
	const char *detail;
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
	INFO("Printer State: %s # %02x %08x %02x\n",
	     sinfonia_1x45_status_str(sts->state.status1, sts->state.status2, sts->state.error),
	     sts->state.status1, sts->state.status2, sts->state.error);
	INFO("Counters:\n");
	INFO("\tLifetime     :  %u\n", be32_to_cpu(sts->counters.lifetime));
	INFO("\tThermal Head :  %u\n", be32_to_cpu(sts->counters.maint));
	INFO("\tMedia        :  %u\n", be32_to_cpu(sts->counters.media));
	INFO("\tRemaining    :  %u\n", ctx->marker.levelmax - be32_to_cpu(sts->counters.media));
	INFO("\tCutter       :  %u\n", be32_to_cpu(sts->counters.cutter));
	INFO("Versions:\n");
	INFO("\tUSB Boot    : %u\n", sts->counters.ver_boot);
	INFO("\tUSB Control : %u\n", sts->counters.ver_ctrl);
	INFO("\tMain Boot   : %u\n", be16_to_cpu(sts->versions.main_boot));
	INFO("\tMain Control: %u\n", be16_to_cpu(sts->versions.main_control));
	INFO("\tDSP Boot    : %u\n", be16_to_cpu(sts->versions.dsp_boot));
	INFO("\tDSP Control : %u\n", be16_to_cpu(sts->versions.dsp_control));

//	INFO("USB TypeFlag: %02x\n", sts->counters.control_flag);

	INFO("Bank 1 ID: %u\n", sts->counters2.bank1_id);
	INFO("\tPrints:  %d/%d complete\n",
	     be16_to_cpu(sts->counters2.bank1_complete),
	     be16_to_cpu(sts->counters2.bank1_spec));
	INFO("Bank 2 ID: %u\n", sts->counters2.bank2_id);
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

static void shinkos1245_dump_media(struct sinfonia_mediainfo_item *medias,
				   int media_8x12,
				   int count)
{
	int i;

	INFO("Loaded media type: %s\n", media_8x12 ? "8x12" : "8x10");
	INFO("Supported print sizes: %d\n", count);

	for (i = 0 ; i < count ; i++) {
		INFO("\t %02d: %04u*%04u (%02x/%02u)\n",
		     i,
		     medias[i].columns,
		     medias[i].rows,
		     medias[i].type,
		     medias[i].method);
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
		ret = read_data(ctx->conn,
				ptr, TONE_CURVE_DATA_BLOCK_SIZE, &num);
		if (num != TONE_CURVE_DATA_BLOCK_SIZE) {
			ret = -99;
			goto done;
		}
		if (ret < 0)
			goto done;
		ptr += num;
		remaining -= num;
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

	INFO("Read %d/%d Tone Curve from '%s'\n", type, table, fname);

	/* Allocate space */
	remaining = TONE_CURVE_SIZE;
	data = malloc(remaining);
	if (!data) {
		ERROR("Memory Allocation Failure!\n");
		return -11;
	}
	ptr = data;


	/* Read in file */
	if ((ret = dyesub_read_file(fname, data, TONE_CURVE_SIZE, NULL))) {
		ERROR("Failed to read Tone Curve file\n");
		goto done;
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
		ret = send_data(ctx->conn,
				ptr, TONE_CURVE_DATA_BLOCK_SIZE);
		if (ret < 0)
			goto done;
		ptr += num;
		remaining -= num;
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
	DEBUG("\t\t[ -R ]           # Reset printer\n");
	DEBUG("\t\t[ -X jobid ]     # Abort a printjob\n");
	DEBUG("\t\t[ -F ]           # Tone curve refers to FINE mode\n");
	DEBUG("\t\t[ -c filename ]  # Get user/NV tone curve\n");
	DEBUG("\t\t[ -C filename ]  # Set user/NV tone curve\n");
	DEBUG("\t\t[ -l filename ]  # Get current tone curve\n");
	DEBUG("\t\t[ -L filename ]  # Set current tone curve\n");
}

static int shinkos1245_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct shinkos1245_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "c:C:l:L:FmRsuU:X:")) >= 0) {
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
				shinkos1245_dump_media(ctx->medias, ctx->media_8x12, ctx->num_medias);
			break;
		case 'R':
			j = shinkos1245_reset(ctx);
			break;
		case 's': {
			struct shinkos1245_resp_status sts;
			j = shinkos1245_get_status(ctx, &sts);
			if (!j)
				shinkos1245_dump_status(ctx, &sts);
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

	return CUPS_BACKEND_OK;
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

static int shinkos1245_attach(void *vctx, struct dyesub_connection *conn, uint8_t jobid)
{
	struct shinkos1245_ctx *ctx = vctx;

	ctx->conn = conn;

	/* Ensure jobid is sane */
	ctx->jobid = jobid & 0x7f;
	if (!ctx->jobid)
		ctx->jobid++;

	if (test_mode < TEST_MODE_NOATTACH) {
		/* Query Media */
		if (shinkos1245_get_media(ctx))
			return CUPS_BACKEND_FAILED;
		if (!ctx->num_medias) {
			ERROR("Media Query Error\n");
			return CUPS_BACKEND_FAILED;
		}
	} else {
		int media_code = 1;
		if (getenv("MEDIA_CODE"))
			media_code = atoi(getenv("MEDIA_CODE"));

		ctx->media_8x12 = media_code;
		ctx->num_medias = 0;
	}
	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = ctx->media_8x12 ? "8x12" : "8x10";
	ctx->marker.numtype = ctx->media_8x12;
	ctx->marker.levelmax = ctx->media_8x12 ? 230 : 280;
	ctx->marker.levelnow = CUPS_MARKER_UNKNOWN;

	return CUPS_BACKEND_OK;
}

static int shinkos1245_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct shinkos1245_ctx *ctx = vctx;
	int ret;

	struct sinfonia_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));
	job->common.jobsize = sizeof(*job);

	/* Common read/parse code */
	ret = sinfonia_read_parse(data_fd, 1245, job);
	if (ret) {
		free(job);
		return ret;
	}

	/* Use larger of our copy counts */
	if (job->common.copies < copies)
		job->common.copies = copies;

	*vjob = job;
	return CUPS_BACKEND_OK;
}

static int shinkos1245_main_loop(void *vctx, const void *vjob, int wait_for_return) {
	struct shinkos1245_ctx *ctx = vctx;
	int i, num, last_state = -1, state = S_IDLE;
	struct shinkos1245_resp_status status1, status2;
	int copies;

	const struct sinfonia_printjob *job = vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	copies = job->common.copies;

	/* Make sure print size is supported */
	for (i = 0 ; i < ctx->num_medias ; i++) {
		if (job->jp.media == ctx->medias[i].code &&
		    job->jp.method == ctx->medias[i].method &&
		    job->jp.rows == ctx->medias[i].rows &&
		    job->jp.columns == ctx->medias[i].columns)
			break;
	}
	if (i == ctx->num_medias) {
		ERROR("Unsupported print type\n");
		return CUPS_BACKEND_HOLD;
	}

	/* Fix max print count. */
	if (copies > 9999) // XXX test against remaining media?
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
		// status changed.
	} else if (state == last_state) {
		sleep(1);
		goto top;
	}

	/* Make sure we're not in an error state */
	if (status1.state.status1 == STATE_STATUS1_ERROR)
		goto printer_error;

	last_state = state;

	fflush(logger);

	switch (state) {
	case S_IDLE:
		if (status1.state.status1 == STATE_STATUS1_STANDBY) {
			state = S_PRINTER_READY_CMD;
			break;
		}

#if 0 // XXX is this necessary
		if (status1.state.status1 == STATE_STATUS1_WAIT) {
			INFO("Printer busy: %s\n",
			     sinfonia_1x45_status_str(status1.state.status1, status1.state.status2, status1.state.error));
			break;
		}
#endif
		/* If the printer is "busy" check to see if there's any
		   open memory banks so we can queue the next print */

		/* make sure we're not colliding with an existing
		   jobid */
		while (ctx->jobid == status1.counters2.bank1_id ||
		       ctx->jobid == status1.counters2.bank2_id) {
			ctx->jobid++;
			ctx->jobid &= 0x7f;
			if (!ctx->jobid)
				ctx->jobid++;
		}

		if (!status1.counters2.bank1_remain ||
		    !status1.counters2.bank2_remain) {
			state = S_PRINTER_READY_CMD;
			break;
		}
		break;
	case S_PRINTER_READY_CMD: {
		struct shinkos1245_cmd_print cmd;

		/* Set matte intensity */
		if (job->jp.mattedepth != 0x7fffffff) {
			int current = -1;
			i = shinkos1245_get_matte(ctx, &current);
			if (i < 0)
				goto printer_error2;
			if (current != job->jp.mattedepth) {
				i = shinkos1245_set_matte(ctx, job->jp.mattedepth);
				if (i < 0)
					goto printer_error2;
				if (i > 0) {
					INFO("Can't set matte intensity when printing in progress...\n");
					state = S_IDLE;
					sleep(1);
					break;
				}
			}
		}

		INFO("Sending print job (internal id %u)\n", ctx->jobid);

		shinkos1245_fill_hdr(&cmd.hdr);
		cmd.cmd[0] = 0x0a;
		cmd.cmd[1] = 0x00;

		cmd.id = ctx->jobid;
		cmd.count = uint16_to_packed_bcd(copies);
		cmd.columns = cpu_to_be16(job->jp.columns);
		cmd.rows = cpu_to_be16(job->jp.rows);
		cmd.media = job->jp.media;
		cmd.mode = (job->jp.oc_mode & 0x3f) || ((job->jp.dust & 0x3) << 6);
		cmd.combo = job->jp.method;

		/* Issue print command */
		i = shinkos1245_do_cmd(ctx, &cmd, sizeof(cmd),
				       &status1, sizeof(status1),
				       &num);
		status1.state.status2 = be32_to_cpu(status1.state.status2);
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
		if ((i = send_data(ctx->conn,
				   job->databuf, job->datalen)))
			return CUPS_BACKEND_FAILED;

		INFO("Waiting for printer to acknowledge completion\n");
		sleep(1);
		state = S_PRINTER_SENT_DATA;
		break;
	}
	case S_PRINTER_SENT_DATA:
		if (!wait_for_return) {
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

	return CUPS_BACKEND_OK;

printer_error:
	ERROR("Printer Error: %s # %02x %08x %02x\n",
	      sinfonia_1x45_status_str(status1.state.status1, status1.state.status2, status1.state.error),
	      status1.state.status1, status1.state.status2, status1.state.error);
printer_error2:
	return CUPS_BACKEND_FAILED;
}

static int shinkos1245_query_serno(struct dyesub_connection *conn, char *buf, int buf_len)
{
	struct shinkos1245_resp_getid resp;
	int i;

	struct shinkos1245_ctx ctx = {
		.conn = conn,
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

static int shinkos1245_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct shinkos1245_ctx *ctx = vctx;
	struct shinkos1245_resp_status status;

	/* Query status */
	if (shinkos1245_get_status(ctx, &status))
		return CUPS_BACKEND_FAILED;

	ctx->marker.levelnow = ctx->marker.levelmax - be32_to_cpu(status.counters.media);

	if (markers) *markers = &ctx->marker;
	if (markers) *count = 1;

	return CUPS_BACKEND_OK;
}

static int shinkos1245_query_stats(void *vctx,  struct printerstats *stats)
{
	struct shinkos1245_ctx *ctx = vctx;
	struct shinkos1245_resp_status status;

	if (shinkos1245_query_markers(ctx, NULL, NULL))
		return CUPS_BACKEND_FAILED;

	if (shinkos1245_get_status(ctx, &status))
		return CUPS_BACKEND_FAILED;

	stats->mfg = "Sinfonia";
	stats->model = "E1 / S1245";

	if (shinkos1245_query_serno(ctx->conn,
				    ctx->serial, sizeof(ctx->serial)))
		return CUPS_BACKEND_FAILED;

	stats->serial = ctx->serial;

	snprintf(ctx->fwver, sizeof(ctx->fwver)-1,
		 "%d / %d", be16_to_cpu(status.versions.main_control),
		 be16_to_cpu(status.versions.dsp_control));
	stats->fwver = ctx->fwver;

	stats->decks = 1;
	stats->mediatype[0] = ctx->marker.name;
	stats->levelmax[0] = ctx->marker.levelmax;
	stats->levelnow[0] = ctx->marker.levelnow;
	stats->name[0] = "Roll";
	stats->status[0] = strdup(sinfonia_1x45_status_str(status.state.status1, status.state.status2, status.state.error));
	stats->cnt_life[0] = be32_to_cpu(status.counters.lifetime);

	return CUPS_BACKEND_OK;
}

static const char *shinkos1245_prefixes[] = {
	"shinkos1245", /* Family Name */
	NULL
};

const struct dyesub_backend shinkos1245_backend = {
	.name = "Shinko/Sinfonia CHC-S1245/E1",
	.version = "0.35" " (lib " LIBSINFONIA_VER ")",
	.uri_prefixes = shinkos1245_prefixes,
	.cmdline_usage = shinkos1245_cmdline,
	.cmdline_arg = shinkos1245_cmdline_arg,
	.init = shinkos1245_init,
	.attach = shinkos1245_attach,
	.cleanup_job = sinfonia_cleanup_job,
	.read_parse = shinkos1245_read_parse,
	.main_loop = shinkos1245_main_loop,
	.query_serno = shinkos1245_query_serno,
	.query_markers = shinkos1245_query_markers,
	.query_stats = shinkos1245_query_stats,
	.devices = {
		{ 0x10ce, 0x0007, P_SHINKO_S1245, NULL, "shinko-chcs1245"},
		{ 0x10ce, 0x0007, P_SHINKO_S1245, NULL, "sinfonia-chcs1245"}, /* Duplicate */
		{ 0, 0, 0, NULL, NULL}
	}
};

/* CHC-S1245 data format

  Spool file consists of an 116-byte header, followed by RGB-packed data,
  followed by a 4-byte footer.  Header appears to consist of a series of
  4-byte Little Endian words.

   10 00 00 00 MM MM 00 00  00 00 00 00 01 00 00 00  MM == Model (ie 1245d)
   64 00 00 00 00 00 00 00  TT 00 00 00 00 00 00 00  TT == Media Size (0x10 fixed)
   MM 00 00 00 PP 00 00 00  00 00 00 00 ZZ ZZ ZZ ZZ  MM = Print Method (aka cut control), PP = Default/Glossy/Matte (0x01/0x03/0x05), ZZ == matte intensity (0x7fffffff for glossy, else 0x00000000 +- 25 for matte)
   VV 00 00 00 WW WW 00 00  HH HH 00 00 XX XX 00 00  VV == dust; 0x00 default, 0x01 off, 0x02 on, XX == Copies
   00 00 00 00 00 00 00 00  00 00 00 00 ce ff ff ff
   00 00 00 00 ce ff ff ff  QQ QQ 00 00 ce ff ff ff  QQ == DPI, ie 300.
   00 00 00 00 ce ff ff ff  00 00 00 00 00 00 00 00
   00 00 00 00

   [[Packed RGB payload of WW*HH*3 bytes]]

   04 03 02 01  [[ footer ]]


*/
