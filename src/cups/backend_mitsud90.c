/*
 *   Mitsubishi CP-D90DW Photo Printer CUPS backend
 *
 *   (c) 2019-2021 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND mitsud90_backend

#include "backend_common.h"
#include "backend_mitsu.h"

/* CPM1 stuff */
#define CPM1_LAMINATE_STRIDE 1852

#define CPM1_LAMINATE_FILE "M1_MAT02.raw"
#define CPM1_CPC_FNAME "CPM1_N1.csv"
#define CPM1_CPC_G1_FNAME "CPM1_G1.csv"
#define CPM1_CPC_G5_FNAME "CPM1_G5.csv"
#define CPM1_CPC_G5_VIVID_FNAME "CPM1_G5_vivid.csv"
#define CPM1_LUT_FNAME "CPM1_NL.lut"

/* ASK500 stuff -- Note the lack of LUT or G5_vivid! */
#define ASK5_LAMINATE_FILE "ASK5_MAT.raw"
#define ASK5_CPC_FNAME "ASK5_N1.csv"
#define ASK5_CPC_G1_FNAME "ASK5_G1.csv"
#define ASK5_CPC_G5_FNAME "ASK5_G5.csv"

/* Printer data structures */
#define COM_STATUS_TYPE_MODEL   0x01 // 10, null-terminated ASCII. 'CPD90D'
#define COM_STATUS_TYPE_x02     0x02 // 1, 0x5f ?
#define CM1_STATUS_TYPE_ISERIAL 0x03 // 24, full iSerial string in UTF16(LE)
#define CM1_STATUS_TYPE_SERIAL  0x04 // 6, serial number only (ascii)
#define CM1_STATUS_TYPE_FW_0a   0x0a // 8, 34 34 38 41 31 32 29 f4 (448A12)
#define COM_STATUS_TYPE_FW_0b   0x0b // 8, 34 31 34 42 31 31 a7 de (414D11)
#define COM_STATUS_TYPE_FW_MA   0x0c // 8, 34 31 35 41 38 31 86 bf (415A81)  // MAIN FW
#define COM_STATUS_TYPE_FW_F    0x0d // 8, 34 31 36 41 35 31 dc 8a (416A51)  // FPGA FW
#define COM_STATUS_TYPE_FW_T    0x0e // 8, 34 31 37 45 31 31 e7 e6 (417E11)  // TABLE FW
#define COM_STATUS_TYPE_FW_0f   0x0f // 8, 34 31 38 41 31 32 6c 64 (418A12)
#define COM_STATUS_TYPE_FW_11   0x11 // 8, 34 32 31 51 31 31 74 f2 (421Q11)
#define COM_STATUS_TYPE_FW_ME   0x13 // 8, 34 31 39 45 31 31 15 bf (419E11)  // MECHA FW

#define COM_STATUS_TYPE_ERROR   0x16 // 11 (see below)
#define COM_STATUS_TYPE_MECHA   0x17 // 2  (see below)
#define COM_STATUS_TYPE_x1e     0x1e // 1, power state or time?  (x00)
#define COM_STATUS_TYPE_TEMP    0x1f // 1  (see below)
#define COM_STATUS_TYPE_x22     0x22 // 2,  all 0  (counter?)
#define COM_STATUS_TYPE_x28     0x28 // 2, next jobid? (starts 00 01 at power cycle, increments by 1 for each print)
#define COM_STATUS_TYPE_x29     0x29 // 8,  e0 07 00 00 21 e6 b3 22 or e0 07 80 96 3f 28 12 2d
#define COM_STATUS_TYPE_MEDIA   0x2a // 10 (see below)
#define COM_STATUS_TYPE_x2b     0x2b // 2,  all 0 (counter?)
#define COM_STATUS_TYPE_x2c     0x2c // 2,  00 56 (counter?) or 00 28
#define COM_STATUS_TYPE_x65     0x65 // 50, see below
#define D90_STATUS_TYPE_ISEREN  0x82 // 1,  80 (iserial disabled)
#define COM_STATUS_TYPE_x83     0x83 // 1,  00
#define D90_STATUS_TYPE_x84     0x84 // 1,  00

//#define D90_STATUS_TYPE_x85    0x85 // 2, 00 ?? BE, wait time?
                                    // combined total of 5.

struct mitsud90_fw_resp_single {
	uint8_t  version[6];
	uint16_t csum;
} __attribute__((packed));

struct mitsud90_media_resp {
	uint8_t  hdr[4];  /* e4 47 44 30 */
	struct {
		uint8_t  brand;
		uint8_t  type;
		uint8_t  unk_a[2];
		uint16_t capacity; /* BE */
		uint16_t remain;  /* BE */
		uint8_t  unk_b[2];
	} __attribute__((packed)) media; /* COM_STATUS_TYPE_MEDIA */
} __attribute__((packed));

struct mitsud90_status_resp {
	uint8_t  hdr[4];  /* e4 47 44 30 */
	/* COM_STATUS_TYPE_ERROR */
	uint8_t  code[2]; /* 00 is ok, nonzero is error */
	uint8_t  unk[9];
	/* COM_STATUS_TYPE_MECHA */
	uint8_t  mecha[2];
	/* COM_STATUS_TYPE_TEMP */
	uint8_t  temp;
} __attribute__((packed));

struct mitsud90_info_resp {
	uint8_t  hdr[4];  /* e4 47 44 30 */
	uint8_t  model[10];
	uint8_t  x02;
	struct mitsud90_fw_resp_single fw_vers[7];
	uint8_t  x1e;
	uint8_t  x22[2];
	uint16_t x28;
	uint8_t  x29[8];
	uint8_t  x2b[2];
	uint8_t  x2c[2];
	uint8_t  x65[50];
	uint8_t  iserial;
	uint8_t  x83;
	uint8_t  x84;
} __attribute__((packed));

struct mitsud90_fwver_resp {
	uint8_t  hdr[4];  /* e4 47 44 30 */
	struct mitsud90_fw_resp_single fw_ver;
} __attribute((packed));

struct mitsum1_info_resp {
	uint8_t  hdr[4];  /* e4 47 44 30 */
	uint8_t  model[10];
	uint8_t  x02;
	struct mitsud90_fw_resp_single fw_vers[8];
	uint8_t  x1e;
	uint8_t  x22[2];
	uint16_t x28;
	uint8_t  x29[8];
	uint8_t  x2b[2];
	uint8_t  x2c[2];
	uint8_t  x65[50];
	uint8_t  x83;
} __attribute__((packed));


#define D90_MECHA_STATUS_IDLE         0x00
#define D90_MECHA_STATUS_PRINTING     0x50
#define D90_MECHA_STATUS_INIT         0x80
#define D90_MECHA_STATUS_INIT_FEEDCUT 0x10

#define D90_MECHA_STATUS_PRINT_FEEDING 0x10  // feeding ?
#define D90_MECHA_STATUS_PRINT_PRE_Y   0x21  // pre Y ?
#define D90_MECHA_STATUS_PRINT_Y       0x22  // Y ?
#define D90_MECHA_STATUS_PRINT_PRE_M   0x23  // pre M ?
#define D90_MECHA_STATUS_PRINT_M       0x24  // M ?
#define D90_MECHA_STATUS_PRINT_PRE_C   0x25  // pre C ? guess!
#define D90_MECHA_STATUS_PRINT_C       0x26  // C ?
#define D90_MECHA_STATUS_PRINT_PRE_OC  0x27  // pre OC ? guess!
#define D90_MECHA_STATUS_PRINT_OC      0x28  // O C?
#define D90_MECHA_STATUS_PRINTING_x2f  0x2f  // ??
#define D90_MECHA_STATUS_PRINTING_x38  0x38  // eject ?

#define D90_ERROR_STATUS_OK         0x00
#define D90_ERROR_STATUS_OK_WARMING 0x40
#define D90_ERROR_STATUS_OK_COOLING 0x80
#define D90_ERROR_STATUS_RIBBON     0x21
#define D90_ERROR_STATUS_PAPER      0x22
#define D90_ERROR_STATUS_PAP_RIB    0x23
#define D90_ERROR_STATUS_OPEN       0x29

struct mitsud90_job_query {
	uint8_t  hdr[4];  /* 1b 47 44 31 */
	uint16_t jobid;   /* BE */
} __attribute__((packed));

struct mitsud90_job_resp {
	uint8_t  hdr[4];  /* e4 47 44 31 */
	uint8_t  unk1;
	uint8_t  unk2;
	uint16_t unk3;
} __attribute__((packed));

struct mitsud90_job_hdr {
	uint8_t  hdr[6]; /* 1b 53 50 30 00 33 */
	uint16_t cols;   /* BE */
	uint16_t rows;   /* BE */
	uint8_t  waittime; /* 0-100 */
	uint8_t  unk[3]; /* 00 00 01 */ // XXX 00 01 might be the jobid?
	uint8_t  margincut; /* 1 for enabled, 0 for disabled */
	uint8_t  numcuts; /* # of cuts (0-3) but 0-8 legal */
/*@0x10*/
	struct {
		uint16_t position;  // @ center?
		uint8_t  margincut; /* 0 for double cut, 1 for single */
		uint8_t  zeropad;
	} cutlist[8] __attribute__((packed));  /* 3 is current legal max */
/*@x30*/uint8_t  overcoat;  /* 0 glossy, matte is 2 (D90) or 3 (M1) */
	uint8_t  quality;   /* 0 is automatic, 5 is "fast" on M1 */
	uint8_t  colorcorr; /* Always 1 on M1 */
	uint8_t  sharp_h;   /* Always 0 on M1 */
	uint8_t  sharp_v;   /* Always 0 on M1 */
	uint8_t  zero_b[5]; /* 0 on D90, on M1, zero_b[3] is the not-raw flag */
	struct {
/* @x3a */	uint8_t  on;      /* 0x01 when pano is on / always 0x02 on M1 / 0x03 on D90 panorama that needs backend processing */
		uint8_t  zero_a;
		uint8_t  total;   /* 2 or 3 */
		uint8_t  page;    /* 1, 2, 3 */
		uint16_t rows;    /* always 0x097c (BE), ie 2428 ie 8" print */
/* @x40 */	uint16_t rows2;   /* Always 0x30 less than pano_rows */
		uint16_t zero_b;  /* 0x0000 */
		uint16_t overlap; /* always 0x0258, ie 600 or 2 inches */
		uint8_t  unk[4];  /* 00 0c 00 06 */
	} pano __attribute__((packed));
	uint8_t zero_c[6];
/*@x50*/uint8_t unk_m1;   /* 00 on d90 & m1 Linux, 01 on m1 (windows) */
	uint8_t rgbrate;  /* M1 only, see below */
	uint8_t oprate;   /* M1 only, see below */
	uint8_t zero_fill[429];
} __attribute__((packed));

struct mitsud90_plane_hdr {
	uint8_t  hdr[6]; /* 1b 5a 54 01 00 09 */
	uint16_t origin_cols;  /* Leave at 0 */
	uint16_t origin_rows;  /* Leave at 0 */
	uint16_t cols;  /* BE */
	uint16_t rows;  /* BE */
	uint8_t  zero_a[6];
	uint16_t lamcols; /* BE (M1 only, OC=3) should be cols+origin_cols */
	uint16_t lamrows; /* BE (M1 only, OC=3) should be rows+origin_rows+12 */
	uint8_t  zero_b[8];
	uint8_t  unk_m1[8]; /* 07 e4 02 19 xx xx xx 00 always incrementing. timestamp? Only seen from win-generated jobs? */
	uint8_t  zero_fill[472];
} __attribute__((packed));

struct mitsud90_job_footer {
	uint8_t hdr[4]; /* 1b 42 51 31 */
	uint16_t seconds; /* BE, 0x0005 by default (windows), 0x00ff means don't wait */
} __attribute__((packed));

struct mitsud90_memcheck {
	uint8_t  hdr[4]; /* 1b 47 44 33 */
	uint8_t  unk[2]; /* 00 33 */
	uint16_t cols;   /* BE */
	uint16_t rows;   /* BE */
	uint8_t  unk_b[4]; /* 64 00 00 01  */
	uint8_t  zero_fill[498];
} __attribute__((packed));

struct mitsud90_memcheck_resp {
	uint8_t  hdr[4];   /* e4 47 44 43 */
	uint8_t  size_bad; /* 0x00 is ok */
	uint8_t  mem_bad;  /* 0x00 is ok */
} __attribute__((packed));

static const char *mitsud90_mecha_statuses(const uint8_t *code)
{
	switch (code[0]) {
	case D90_MECHA_STATUS_IDLE:
		return "Idle";
	case D90_MECHA_STATUS_PRINTING:
		switch (code[1]) {
		case D90_MECHA_STATUS_PRINT_FEEDING:
			return "Feeding Media";
		case D90_MECHA_STATUS_PRINT_PRE_Y:
		case D90_MECHA_STATUS_PRINT_Y:
			return "Printing Yellow";
		case D90_MECHA_STATUS_PRINT_PRE_M:
		case D90_MECHA_STATUS_PRINT_M:
			return "Printing Magenta";
		case D90_MECHA_STATUS_PRINT_PRE_C:
		case D90_MECHA_STATUS_PRINT_C:
			return "Printing Cyan";
		case D90_MECHA_STATUS_PRINT_PRE_OC:
		case D90_MECHA_STATUS_PRINT_OC:
			return "Applying Overcoat";
		case D90_MECHA_STATUS_PRINTING_x2f:
		case D90_MECHA_STATUS_PRINTING_x38:
			return "Ejecting Media?";
		default:
			return "Printing (Unknown)";
		}
	case D90_MECHA_STATUS_INIT:
		if (code[1] == D90_MECHA_STATUS_INIT_FEEDCUT)
			return "Feed & Cut paper";
		else
			return "Initializing";
	default:
		return "Unknown";
	}
}

static const char *mitsud90_error_codes(const uint8_t *code)
{
	switch(code[0]) {
	case D90_ERROR_STATUS_OK:
		if (code[1] & D90_ERROR_STATUS_OK_WARMING)
			return "Heating";
		else if (code[1] & D90_ERROR_STATUS_OK_COOLING)
			return "Cooling Down";
		else
			return "Idle";
	case D90_ERROR_STATUS_RIBBON:
		switch (code[1]) {
		case 0x00:
			return "Ribbon exhausted";
		case 0x10:
			return "Insufficient remaining ribbon";
		case 0x20:
			return "Ribbon Cue Timeout";
		case 0x30:
			return "Cannot Cue Ribbon";
		case 0x90:
			return "No ribbon";
		default:
			return "Unknown Ribbon Error";
		}
	case D90_ERROR_STATUS_PAPER:
		switch (code[1]) {
		case 0x00:
			return "No paper";
		case 0x02:
			return "Paper exhausted";
		default:
			return "Unknown Paper Error";
		}
	case D90_ERROR_STATUS_PAP_RIB:
		switch (code[1]) {
		case 0x00:
			return "Ribbon/Paper mismatch";
		case 0x90:
			return "Ribbon/Job mismatch";
		default:
			return "Unknown ribbon match error";
		}
	case 0x26:
		return "Illegal Ribbon";
	case 0x28:
		return "Cut Bin Missing";
	case D90_ERROR_STATUS_OPEN:
		switch (code[1]) {
		case 0x00:
			return "Printer Open during Stop";
		case 0x10:
			return "Printer Open during Initialization";
		case 0x90:
			return "Printer Open during Printing";
		default:
			return "Unknown Door error";
		}
	case 0x2f:
		return "Printer turned off during printing";
	case 0x31:
		return "Ink feed stop";
	case 0x32:
		return "Ink Skip 1 timeout";
	case 0x33:
		return "Ink Skip 2 timeout";
	case 0x34:
		return "Ink Sticking";
	case 0x35:
		return "Ink return stop";
	case 0x36:
		return "Ink Rewind timeout";
	case 0x37:
		return "Winding sensing error";
	case 0x40:
	case 0x41:
	case 0x42:
	case 0x43:
	case 0x44:
		return "Paper Jam";
	case 0x60:
		if (code[1] == 0x20)
			return "Preheat error";
		else if (code[1] == 0x04)
			return "Humidity sensor error";
		else if (code[1] & 0x1f)
			return "Thermistor error";
		else
			return "Unknown error";
	case 0x61:
		if (code[1] == 0x00)
			return "Color Sensor Error";
		else if (code[1] & 0x10)
			return "Matte OP Error";
		else
			return "Unknown error";
	case 0x62:
		return "Data Transfer error";
	case 0x63:
		return "EEPROM error";
	case 0x64:
		return "Flash access error";
	case 0x65:
		return "FPGA configuration error";
	case 0x66:
		return "Power voltage Error";
	case 0x67:
		return "RFID access error";
	case 0x68:
		if (code[1] == 0x00)
			return "Fan Lock Error";
		else if (code[1] == 0x90)
			return "MDA Error";
		else
			return "Unknown error";
	case 0x69:
		if (code[1] == 0x10)
			return "DDR Error";
		else if (code[1] == 0x00)
			return "Firmware Error";
		else
			return "Unknown error";
	case 0x70:
	case 0x71:
	case 0x73:
	case 0x74:
	case 0x75:
		return "Mechanical Error (check ribbon and power cycle)";
	case 0x82:
		return "USB Timeout";
	case 0x83:
		return "Illegal paper size";
	case 0x84:
		return "Illegal parameter";
	case 0x85:
		return "Job Cancel";
	case 0x89:
		return "Last Job Error";
	default:
		return "Unknown";
	}
}

static void mitsud90_dump_status(struct mitsud90_status_resp *resp)
{
	INFO("Error Status: %s (%02x %02x) -- %02x %02x %02x %02x  %02x %02x %02x %02x  %02x\n",
	     mitsud90_error_codes(resp->code),
	     resp->code[0], resp->code[1],
	     resp->unk[0], resp->unk[1], resp->unk[2], resp->unk[3],
	     resp->unk[4], resp->unk[5], resp->unk[6], resp->unk[7],
	     resp->unk[8]);
	INFO("Printer Status: %s (%02x %02x)\n",
	     mitsud90_mecha_statuses(resp->mecha),
	     resp->mecha[0], resp->mecha[1]);
	INFO("Temperature Status: %s\n",
	     mitsu_temperatures(resp->temp));
}

/* Private data structure */
struct mitsud90_printjob {
	struct dyesub_job_common common;

	uint8_t *databuf;
	uint32_t datalen;

	int is_raw;
	int is_pano;

	int m1_colormode;

	struct mitsud90_job_hdr hdr;

	int has_footer;
	struct mitsud90_job_footer footer;
};

struct mitsud90_ctx {
	struct dyesub_connection *conn;

	char serno[7]; /* 6+null */
	char fwver[7]; /* 6+null */

	/* Used in parsing.. */
	struct mitsud90_job_footer holdover;
	int holdover_on;

	int pano_page;

	/* For the CP-M1 family */
	struct mitsu_lib lib;

	struct marker marker;
};

static int mitsud90_query_media(struct mitsud90_ctx *ctx, struct mitsud90_media_resp *resp)
{
	uint8_t cmdbuf[8];
	int ret, num;

	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x47;
	cmdbuf[2] = 0x44;
	cmdbuf[3] = 0x30;
	cmdbuf[4] = 0;
	cmdbuf[5] = 0;
	cmdbuf[6] = 0x01;  /* Number of commands */
	cmdbuf[7] = COM_STATUS_TYPE_MEDIA;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;
	memset(resp, 0, sizeof(*resp));

	ret = read_data(ctx->conn,
			(uint8_t*) resp, sizeof(*resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(*resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*resp));
		return 4;
	}

	return CUPS_BACKEND_OK;
}

static int mitsud90_query_status(struct mitsud90_ctx *ctx, struct mitsud90_status_resp *resp)
{
	uint8_t cmdbuf[10];
	int ret, num;

	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x47;
	cmdbuf[2] = 0x44;
	cmdbuf[3] = 0x30;
	cmdbuf[4] = 0;
	cmdbuf[5] = 0;
	cmdbuf[6] = 0x03;  /* Number of commands */
	cmdbuf[7] = COM_STATUS_TYPE_ERROR;
	cmdbuf[8] = COM_STATUS_TYPE_MECHA;
	cmdbuf[9] = COM_STATUS_TYPE_TEMP;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;
	memset(resp, 0, sizeof(*resp));

	ret = read_data(ctx->conn,
			(uint8_t*) resp, sizeof(*resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(*resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*resp));
		return 4;
	}

	return CUPS_BACKEND_OK;
}

static int mitsud90_query_fwver(struct mitsud90_ctx *ctx)
{
	uint8_t cmdbuf[8];
	int ret, num;
	struct mitsud90_fwver_resp resp;

	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x47;
	cmdbuf[2] = 0x44;
	cmdbuf[3] = 0x30;
	cmdbuf[4] = 0;
	cmdbuf[5] = 0;
	cmdbuf[6] = 1;  /* Number of commands */
	cmdbuf[7] = COM_STATUS_TYPE_FW_MA;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;
	memset(&resp, 0, sizeof(resp));

	ret = read_data(ctx->conn,
			(uint8_t*) &resp, sizeof(resp), &num);

	memcpy(ctx->fwver, resp.fw_ver.version, 6);
	ctx->fwver[6] = 0;


	return CUPS_BACKEND_OK;
}

static int mitsud90_get_serno(struct mitsud90_ctx *ctx)
{
	uint8_t cmdbuf[32];
	int ret, num;

	/* Send Request */
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x61;
	cmdbuf[2] = 0x36;
	cmdbuf[3] = 0x36;
	cmdbuf[4] = 0x41;
	cmdbuf[5] = 0xbe;
	cmdbuf[6] = 0x00;
	cmdbuf[7] = 0x00;

	cmdbuf[8] = 0x00;
	cmdbuf[9] = 0x06;
	cmdbuf[10] = 0x00;
	cmdbuf[11] = 0x00;
	cmdbuf[12] = 0x00;
	cmdbuf[13] = 0x30;
	cmdbuf[14] = 0xff;
	cmdbuf[15] = 0xff;

	cmdbuf[16] = 0xff;
	cmdbuf[17] = 0xf9;
	cmdbuf[18] = 0xff;
	cmdbuf[19] = 0xff;
	cmdbuf[20] = 0xff;
	cmdbuf[21] = 0xcf;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, 22)))
		return ret;

	ret = read_data(ctx->conn,
			cmdbuf, sizeof(cmdbuf), &num);

	/* Store it */
	memcpy(ctx->serno, &cmdbuf[22], 6);
	ctx->serno[6] = 0;

	return ret;
}

/* Generic functions */

static void *mitsud90_init(void)
{
	struct mitsud90_ctx *ctx = malloc(sizeof(struct mitsud90_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct mitsud90_ctx));

	return ctx;
}

static int mitsud90_attach(void *vctx, struct dyesub_connection *conn, uint8_t jobid)
{
	struct mitsud90_ctx *ctx = vctx;
	struct mitsud90_media_resp mresp;

	UNUSED(jobid);

	ctx->conn = conn;

	if (test_mode < TEST_MODE_NOATTACH) {
		if (mitsud90_query_media(ctx, &mresp))
			return CUPS_BACKEND_FAILED;
		if (mitsud90_get_serno(ctx))
			return CUPS_BACKEND_FAILED;
		if (mitsud90_query_fwver(ctx))
			return CUPS_BACKEND_FAILED;
	} else {
		mresp.media.brand = 0xff;
		mresp.media.type = 0x0f;
		mresp.media.capacity = cpu_to_be16(230);
		mresp.media.remain = cpu_to_be16(200);
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.numtype = mresp.media.type;
	ctx->marker.name = mitsu_media_types(ctx->conn->type, mresp.media.brand, mresp.media.type);
	ctx->marker.levelmax = be16_to_cpu(mresp.media.capacity);
	ctx->marker.levelnow = be16_to_cpu(mresp.media.remain);

	if (ctx->conn->type == P_MITSU_M1 ||
	    ctx->conn->type == P_FUJI_ASK500) {
#if defined(WITH_DYNAMIC)
		/* Attempt to open the library */
		if (mitsu_loadlib(&ctx->lib, ctx->conn->type))
#endif
			WARNING("Dynamic library support not loaded, will be unable to print.\n");
	}

	// XXX do some runtime checks for FW versions.
	// do a MA FW comparison; CP-M1 v1.00 is 450B11.  ME is 454C11
	// CP-D90-P v2.10 is 415A81 or 415G11 (revA vs revB)  ME is 419E11
	// CP-D90DW v2.10 is 415B94 or 415E54 (??)    ME is 419E42
	// if nothing else, D90 v2.10 is needed for panorama.
	// D90DW-P and D90DW share same USB VID/PID.  Not sure how to tell
	// them apart other than FW.  No idea what functional differences are.

	return CUPS_BACKEND_OK;
}

static void mitsud90_teardown(void *vctx) {
	struct mitsud90_ctx *ctx = vctx;

	if (ctx->pano_page) {
		WARNING("Panorama state left dangling!\n");
	}
	if (!ctx)
		return;

	if (ctx->conn->type == P_MITSU_M1 ||
	    ctx->conn->type == P_FUJI_ASK500) {
		mitsu_destroylib(&ctx->lib);
	}

	free(ctx);
}

static void mitsud90_cleanup_job(const void *vjob)
{
	const struct mitsud90_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);

	free((void*)job);
}

/* Sanity check some stuff */
STATIC_ASSERT(sizeof(struct mitsud90_job_hdr) == 512);
STATIC_ASSERT(sizeof(struct mitsud90_plane_hdr) == 512);

static int mitsud90_main_loop(void *vctx, const void *vjob, int wait_for_return);

static int mitsud90_panorama_splitjob(struct mitsud90_printjob *injob, struct mitsud90_printjob **newjobs)
{
	uint8_t *panels[3] = { NULL, NULL, NULL };
	uint16_t panel_rows[3] = { 0, 0, 0 };
	uint16_t overlap_rows;
	uint8_t numpanels;
	uint16_t cols;
	uint16_t inrows;
	uint16_t max_rows;
	int i;

	cols = be16_to_cpu(injob->hdr.cols);
	inrows = be16_to_cpu(injob->hdr.rows);

	/* Work out parameters */
	if (inrows == 6028) {
		numpanels = 3;
		overlap_rows = 600 + 28;
		max_rows = 2428;
	} else if (inrows == 4228) {
		numpanels = 2;
		max_rows = 2428;
		overlap_rows = 600 + 28;
	} else {
		ERROR("Invalid panorama row count (%d)\n", inrows);
		return CUPS_BACKEND_CANCEL;
	}

	/* Work out which number of rows per panel */
	if (!panel_rows[0]) {
		panel_rows[0] = max_rows;
		panel_rows[1] = inrows - panel_rows[0] + overlap_rows;
		if (numpanels > 2)
			panel_rows[2] = inrows - panel_rows[0] - panel_rows[1] + overlap_rows + overlap_rows;
	}

	/* Allocate and set up new jobs and buffers */
	for (i = 0 ; i < numpanels ; i++) {
		newjobs[i] = malloc(sizeof(struct mitsud90_printjob));
		if (!newjobs[i]) {
			ERROR("Memory allocation failure");
			return CUPS_BACKEND_RETRY_CURRENT;
		}
		panels[i] = malloc(cols * panel_rows[i] * 3) + sizeof(struct mitsud90_plane_hdr);
		if (!panels[i]) {
			ERROR("Memory allocation failure");
			return CUPS_BACKEND_RETRY_CURRENT;
		}
		/* Fill in job header differences */
		memcpy(newjobs[i], injob, sizeof(struct mitsud90_printjob));
		newjobs[i]->databuf = panels[i];
		newjobs[i]->hdr.rows = cpu_to_be16(panel_rows[i]);
		newjobs[i]->hdr.pano.on = 1;
		newjobs[i]->hdr.pano.total = numpanels;
		newjobs[i]->hdr.pano.page = i;
		newjobs[i]->hdr.pano.rows = cpu_to_be16(panel_rows[i]);
		newjobs[i]->hdr.pano.rows2 = cpu_to_be16(panel_rows[i] - 0x30);
		newjobs[i]->hdr.pano.overlap = cpu_to_be16(overlap_rows);
		newjobs[i]->hdr.pano.unk[1] = 0x0c;
		newjobs[i]->hdr.pano.unk[3] = 0x06;
		newjobs[i]->has_footer = 0;

		/* Fill in plane header differences */
		memcpy(newjobs[i]->databuf, injob->databuf, sizeof(struct mitsud90_plane_hdr));
		struct mitsud90_plane_hdr *phdr = (struct mitsud90_plane_hdr*)panels[i];
		phdr->rows = cpu_to_be16(panel_rows[i]);
		if (phdr->lamrows)
			phdr->lamrows = cpu_to_be16(panel_rows[i] + 12);
		panels[i] += sizeof(struct mitsud90_plane_hdr);
	}
	/* Last panel gets the footer, if any */
	newjobs[numpanels - 1]->has_footer = injob->has_footer;

	dyesub_pano_split_rgb8(injob->databuf, cols, inrows,
			       numpanels, overlap_rows, max_rows,
			       panels, panel_rows);

	// XXX process buffers!
	// pano_process_rgb8(numpanels, cols, overlap_rows, panels, panel_rows);

	return CUPS_BACKEND_OK;
}

static int mitsud90_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct mitsud90_ctx *ctx = vctx;
	int i, remain;

	struct mitsud90_printjob *job;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));
	job->common.jobsize = sizeof(*job);
	job->common.copies = copies;

	/* Read in header */
	uint8_t *hptr = (uint8_t*) &job->hdr;
	uint16_t hremain = sizeof(struct mitsud90_job_hdr);

	/* Make sure there's no holdover */
	if (ctx->holdover_on) {
		memcpy(hptr, &ctx->holdover, sizeof(ctx->holdover));
		hremain -= sizeof(ctx->holdover);
		ctx->holdover_on = 0;
	}
	/* Read the rest */
	while (hremain) {
		i = read(data_fd, hptr, hremain);
		if (i == 0) {
			mitsud90_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		if (i < 0) {
			mitsud90_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		hremain -= i;
		hptr += i;
	}

	/* Sanity check header */
	if (job->hdr.hdr[0] != 0x1b ||
	    job->hdr.hdr[1] != 0x53 ||
	    job->hdr.hdr[2] != 0x50 ||
	    job->hdr.hdr[3] != 0x30 ) {
		ERROR("Unrecognized data format (%02x%02x%02x%02x)!\n",
		      job->hdr.hdr[0], job->hdr.hdr[1],
		      job->hdr.hdr[2], job->hdr.hdr[3]);
		mitsud90_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Initial parsing */
	if (ctx->conn->type == P_MITSU_M1 ||
	    ctx->conn->type == P_FUJI_ASK500) {
		/* See if it's a special gutenprint "not-raw" job */
		job->is_raw = !job->hdr.zero_b[3];
		job->hdr.zero_b[3] = 0;
	} else {
		if (job->hdr.zero_b[3] && job->hdr.pano.on == 0x03) {
			job->is_pano = 1;
			job->hdr.zero_b[3] = 0;
			job->hdr.pano.on = 0x01;
		}
	}

	/* Sanity check panorama parameters */
	if (job->hdr.pano.on &&
	    ctx->conn->type == P_MITSU_D90) {
		if ((be16_to_cpu(job->hdr.pano.total) < 2 &&
		     be16_to_cpu(job->hdr.pano.total) > 3) ||
		    (be16_to_cpu(job->hdr.pano.page) < 1 &&
		     be16_to_cpu(job->hdr.pano.page) > 3) ||
		    be16_to_cpu(job->hdr.pano.page) != (ctx->pano_page + 1) ||
		    be16_to_cpu(job->hdr.pano.rows != 2428) ||
		    be16_to_cpu(job->hdr.pano.rows2 != (2428-0x30)) ||
		    be16_to_cpu(job->hdr.pano.overlap != 600)
			) {
			ERROR("Invalid panorama parameters");
			mitsud90_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
	}

	/* Sanity check cutlist */
	if (job->hdr.numcuts > 3) {
		ERROR("Cut list too long!\n");
		mitsud90_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}
	if (job->hdr.numcuts >= 1) {
		int rows = be16_to_cpu(job->hdr.rows);
		for (i = 0 ; i < job->hdr.numcuts ; i++) {
			int min_size;
			int position = be16_to_cpu(job->hdr.cutlist[i].position);
			int last_position = (i == 0) ? 0 : be16_to_cpu(job->hdr.cutlist[i-1].position);

			if (i == 0)
				min_size = 613;
			else
				min_size = (job->hdr.cutlist[i-1].margincut) ? 601 : 660; // XXX inverted?

			if ((position - last_position) < min_size) {
				ERROR("Minumum cut#%d length is %d rows\n", i, min_size);
				mitsud90_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			if ((rows - position) < min_size) {
				ERROR("Cut#%d is too close to end\n", i);
				mitsud90_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
		}
	}

	remain = be16_to_cpu(job->hdr.cols) * be16_to_cpu(job->hdr.rows) * 3;
	if (job->is_raw)
		remain *= 2;
	/* Add in the plane header */
	remain += sizeof(struct mitsud90_plane_hdr);

	/* Allocate ourselves a payload buffer */
	job->databuf = malloc(remain + 1024);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		mitsud90_cleanup_job(job);
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	job->datalen = 0;

	/* Now read in the rest */
	while(remain) {
		i = read(data_fd, job->databuf + job->datalen, remain);
		if (i == 0) {
			mitsud90_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		if (i < 0) {
			mitsud90_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		job->datalen += i;
		remain -= i;
	}

	/* Read in the footer.  Hopefully... */
	i = read(data_fd, (uint8_t*)&job->footer, sizeof(job->footer));
	if (i == 0) {
		mitsud90_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}
	if (i < 0) {
		mitsud90_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* See if this is a job footer.  If it is, keep, else holdover. */
	if (job->footer.hdr[0] != 0x1b ||
	    job->footer.hdr[1] != 0x42 ||
	    job->footer.hdr[2] != 0x51 ||
	    job->footer.hdr[3] != 0x31) {
		memcpy(&ctx->holdover, &job->footer, sizeof(job->footer));
	        ctx->holdover_on = 1;
		// XXX generate a footer!
	} else {
		job->has_footer = 1;
		ctx->holdover_on = 0;
	}

	/* CP-M1 has... other considerations */
	if ((ctx->conn->type == P_MITSU_M1 ||
	     ctx->conn->type == P_FUJI_ASK500) && !job->is_raw) {
		if (!ctx->lib.dl_handle) {
			ERROR("!!! Image Processing Library not found, aborting!\n");
			mitsud90_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}

		job->m1_colormode = job->hdr.colorcorr;

		if (job->m1_colormode == 0) {
			const char *lutfname = NULL;

			if (ctx->conn->type == P_MITSU_M1) {
				lutfname = CPM1_LUT_FNAME;
			}

			/* NOTE: No LUT for ASK-500 yet */
			if (lutfname) {
				int ret = mitsu_apply3dlut_packed(&ctx->lib, lutfname,
								  job->databuf + sizeof(struct mitsud90_plane_hdr),
								  be16_to_cpu(job->hdr.cols),
								  be16_to_cpu(job->hdr.rows),
								  be16_to_cpu(job->hdr.cols) * 3, COLORCONV_RGB);
				if (ret) {
					mitsud90_cleanup_job(job);
					return ret;
				}
			}
		}
		job->hdr.colorcorr = 1; // XXX not sure if right for ASK500?
	}

	if (job->is_pano) {
		int rval;

		rval = mitsud90_panorama_splitjob(job, (struct mitsud90_printjob**)vjob);
		/* Clean up original parsed job regardless */
		mitsud90_cleanup_job(job);

		return rval;
	} else {
		*vjob = job;
	}

	/* All further work is in main loop */
	if (test_mode >= TEST_MODE_NOPRINT)
		mitsud90_main_loop(ctx, job, 1);

	return CUPS_BACKEND_OK;
}

static int cpm1_fillmatte(struct mitsud90_printjob *job)
{
	int ret;
	int rows, cols;

	struct mitsud90_plane_hdr *phdr = (struct mitsud90_plane_hdr *) job->databuf;

	rows = be16_to_cpu(job->hdr.rows) + 12;
	cols = be16_to_cpu(job->hdr.cols);

	/* Fill in matte data */
	ret = mitsu_readlamdata(CPM1_LAMINATE_FILE, CPM1_LAMINATE_STRIDE,
				job->databuf, &job->datalen,
				rows, cols, 1);

	if (ret)
		return ret;

	/* Update plane header and overall length */
	phdr->lamcols = cpu_to_be16(cols);
	phdr->lamrows = cpu_to_be16(rows);

	return CUPS_BACKEND_OK;
}

static int mitsud90_main_loop(void *vctx, const void *vjob, int wait_for_return) {
	struct mitsud90_ctx *ctx = vctx;
	struct mitsud90_status_resp resp;
	uint8_t last_status[2] = {0xff, 0xff};

	int sent;
	int ret;
	int copies;

	struct mitsud90_printjob *job = (struct mitsud90_printjob *)vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;
	copies = job->common.copies;

	/* Handle panorama state */
	if (ctx->conn->type == P_MITSU_D90) {
		if (job->hdr.pano.on) {
			ctx->pano_page++;
			if (be16_to_cpu(job->hdr.pano.page) != ctx->pano_page) {
				ERROR("Invalid panorama state (page %d of %d)\n",
				      ctx->pano_page, be16_to_cpu(job->hdr.pano.page));
				return CUPS_BACKEND_FAILED;
			}
			if (copies > 1) {
				WARNING("Cannot print non-collated copies of a panorama job\n");
				copies = 1;
			}
		} else if (ctx->pano_page) {
			/* Clean up panorama state */
			WARNING("Dangling panorama state!\n");
			ctx->pano_page = 0;
		}
	} else {
		ctx->pano_page = 0;
	}

	if ((ctx->conn->type == P_MITSU_M1 ||
	     ctx->conn->type == P_FUJI_ASK500) && !job->is_raw) {
		struct BandImage input;
		struct BandImage output;
		struct M1CPCData *cpc;

		input.origin_rows = input.origin_cols = 0;
		input.rows = be16_to_cpu(job->hdr.rows);
		input.cols = be16_to_cpu(job->hdr.cols);
		input.imgbuf = job->databuf + sizeof(struct mitsud90_plane_hdr);
		input.bytes_per_row = input.cols * 3;

		/* Allocate new buffer, with extra room for header */
		uint8_t *convbuf = malloc(input.rows * input.cols * sizeof(uint16_t) * 3 + (job->hdr.overcoat? (input.rows + 12) * input.cols + CPM1_LAMINATE_STRIDE / 2 : 0) + sizeof(struct mitsud90_plane_hdr));
		if (!convbuf) {
			ERROR("Memory allocation Failure!\n");
			return CUPS_BACKEND_RETRY_CURRENT;
		}

		output.origin_rows = output.origin_cols = 0;
		output.rows = input.rows;
		output.cols = input.cols;
		output.imgbuf = convbuf + sizeof(struct mitsud90_plane_hdr);
		output.bytes_per_row = output.cols * 3 * sizeof(uint16_t);

		/* Copy over the plane header */
		memcpy(convbuf, job->databuf, sizeof(struct mitsud90_plane_hdr));

		// Do CContrastConv prior to RGBRate
		job->hdr.rgbrate = ctx->lib.M1_CalcRGBRate(input.rows,
							   input.cols,
							   input.imgbuf);

		/* Color modes: 0 LUT, NOMATCH
		                1 NOLUT, MATCH  <-- ie use with external ICC profile!
                                2 NOLUT, NOMATCH */

		const char *gammatab;

		if (ctx->conn->type == P_FUJI_ASK500) {
			if (job->m1_colormode == 1) {
				gammatab = ASK5_CPC_G5_FNAME;
			} else { /* Mode 0 or 2 */
				gammatab = ASK5_CPC_G1_FNAME;
			}
			cpc = ctx->lib.M1_GetCPCData(corrtable_path, ASK5_CPC_FNAME, gammatab);
		} else {
			if (job->m1_colormode == 1) {
				gammatab = CPM1_CPC_G5_FNAME;
			} else if (job->m1_colormode == 3) {
				gammatab = CPM1_CPC_G5_VIVID_FNAME;
			} else { /* Mode 0 or 2 */
				gammatab = CPM1_CPC_G1_FNAME;
			}
			cpc = ctx->lib.M1_GetCPCData(corrtable_path, CPM1_CPC_FNAME, gammatab);
		}


		if (!cpc) {
			ERROR("Cannot read data tables\n");
			free(convbuf);
			return CUPS_BACKEND_FAILED;
		}

		/* Do gamma conversion */
		ctx->lib.M1_Gamma8to14(cpc, &input, &output);

		if (job->hdr.sharp_h || job->hdr.sharp_v) {
			/* 0 is off, 1-7 corresponds to level 0-6 */
			int sharp = ((job->hdr.sharp_h > job->hdr.sharp_v) ? job->hdr.sharp_h : job->hdr.sharp_v) - 1;
			job->hdr.sharp_h = 0;
			job->hdr.sharp_v = 0;

			/* And do the sharpening */
			if (ctx->lib.M1_CLocalEnhancer(cpc, sharp, &output)) {
				ERROR("CLocalEnhancer failed (out of memory?)\n");
				free(convbuf);
				ctx->lib.M1_DestroyCPCData(cpc);
				return CUPS_BACKEND_RETRY_CURRENT;
			}
		}

		/* We're done with the CPC data */
		ctx->lib.M1_DestroyCPCData(cpc);

#if (__BYTE_ORDER == __BIG_ENDIAN)
		/* Convert data to LITTLE ENDIAN if needed */
		int i;
		uint16_t *ptr = output.imgbuf;
		for (i = 0; i < output.rows * output.cols ; i ++) {
			ptr[i] = cpu_to_le16(i);
		}
#endif

		free(job->databuf);
		job->databuf = convbuf;
		job->datalen = sizeof(struct mitsud90_plane_hdr) + input.rows * input.cols * sizeof(uint16_t) * 3;

		/* Deal with lamination settings */
		if (job->hdr.overcoat == 3) {
			int pre_matte_len = job->datalen;
			ret = cpm1_fillmatte(job);
			if (ret) {
				mitsud90_cleanup_job(job);
				return ret;
			}
			job->hdr.oprate = ctx->lib.M1_CalcOpRateMatte(output.rows,
								      output.cols,
								      job->databuf + pre_matte_len);
		} else {
			job->hdr.oprate = ctx->lib.M1_CalcOpRateGloss(output.rows,
								      output.cols);
		}
	}

	/* Bypass */
	if (test_mode >= TEST_MODE_NOPRINT)
		return CUPS_BACKEND_OK;

	INFO("Waiting for printer idle...\n");

top:
	sent = 0;

	// XXX Figure out if printer is asleep, and wake it up if necessary.

	/* Query status, wait for idle or error out */
	do {
		if (mitsud90_query_status(ctx, &resp))
			return CUPS_BACKEND_FAILED;

		if (resp.code[0] != D90_ERROR_STATUS_OK) {
			ERROR("Printer reported error condition: %s (%02x %02x)\n",
			      mitsud90_error_codes(resp.code), resp.code[0], resp.code[1]);
			return CUPS_BACKEND_STOP;
		}

		if (resp.code[1] & D90_ERROR_STATUS_OK_WARMING ||
		    resp.temp & D90_ERROR_STATUS_OK_WARMING ) {
			INFO("Printer warming up\n");
			sleep(1);
			continue;
		}
		if (resp.code[1] & D90_ERROR_STATUS_OK_COOLING ||
			   resp.temp & D90_ERROR_STATUS_OK_COOLING) {
			INFO("Printer cooling down\n");
			sleep(1);
			continue;
		}

		if (resp.mecha[0] != last_status[0] ||
		    resp.mecha[1] != last_status[1]) {
			INFO("Printer status: %s\n",
			     mitsud90_mecha_statuses(resp.mecha));
			last_status[0] = resp.mecha[0];
			last_status[1] = resp.mecha[1];
		}

		if (resp.mecha[0] == D90_MECHA_STATUS_IDLE) {
			break;
			// we don't have to wait until idle, just
			// until we have free buffers.  Don't know how
			// to check this though.. XXXX
		}
	} while(1);

	/* Send memory check */
	{
		struct mitsud90_memcheck mem;
		struct mitsud90_memcheck_resp mem_resp;
		int num;

		memcpy(&mem, &job->hdr, sizeof(mem));
		mem.hdr[0] = 0x1b;
		mem.hdr[1] = 0x47;
		mem.hdr[2] = 0x44;
		mem.hdr[3] = 0x33;

		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &mem, sizeof(mem))))
			return CUPS_BACKEND_FAILED;

		ret = read_data(ctx->conn,
				(uint8_t*)&mem_resp, sizeof(mem_resp), &num);

		if (ret < 0)
			return ret;
		if (num != sizeof(mem_resp)) {
			ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(mem_resp));
			return 4;
		}
		if (mem_resp.size_bad || mem_resp.mem_bad == 0xff) {
			ERROR("Printer reported bad print params (%02x)\n", mem_resp.size_bad);
			return CUPS_BACKEND_CANCEL;
		}
		if (mem_resp.mem_bad) {
			ERROR("Printer buffers full, retrying!\n");
			sleep(1);
			goto top;
		}
	}

	/* Send job header */
	if ((ret = send_data(ctx->conn,
			     (uint8_t*) &job->hdr, sizeof(job->hdr))))
		return CUPS_BACKEND_FAILED;

	/* Send Plane header */
	if ((ret = send_data(ctx->conn,
			     job->databuf + sent, sizeof(job->hdr))))
		return CUPS_BACKEND_FAILED;
	sent += sizeof(job->hdr);

	/* Send payload */
	if ((ret = send_data(ctx->conn,
			     job->databuf + sent, job->datalen - sent)))
		return CUPS_BACKEND_FAILED;
//	sent += (job->datalen - sent);

	/* Send job footer */
	if (job->has_footer) {
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &job->footer, sizeof(job->footer))))
			return CUPS_BACKEND_FAILED;

		/* Initiating printing means we're done parsing panorama */
		if (ctx->pano_page)
			ctx->pano_page = 0;
	}

	/* Wait for completion */
	do {
		sleep(1);

		if (mitsud90_query_status(ctx, &resp))
			return CUPS_BACKEND_FAILED;

		if (resp.code[0] != D90_ERROR_STATUS_OK) {
			ERROR("Printer reported error condition: %s (%02x %02x)\n",
			      mitsud90_error_codes(resp.code), resp.code[0], resp.code[1]);
			return CUPS_BACKEND_STOP;
		}

		if (resp.mecha[0] != last_status[0] ||
		    resp.mecha[1] != last_status[1]) {
			INFO("Printer status: %s\n",
			     mitsud90_mecha_statuses(resp.mecha));
			last_status[0] = resp.mecha[0];
			last_status[1] = resp.mecha[1];
		}

		/* Terminate when printing complete */
		if (resp.mecha[0] == D90_MECHA_STATUS_IDLE) {
			break;
		}

		if (!wait_for_return && copies <= 1) { /* Copies generated by backend? */
			INFO("Fast return mode enabled.\n");
			break;
		}
	} while(1);

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		goto top;
	}

	return CUPS_BACKEND_OK;
}

static int mitsud90_query_job(struct mitsud90_ctx *ctx, uint16_t jobid,
	struct mitsud90_job_resp *resp)
{
	struct mitsud90_job_query req;
	int ret, num;

	req.hdr[0] = 0x1b;
	req.hdr[1] = 0x47;
	req.hdr[2] = 0x44;
	req.hdr[3] = 0x31;
	req.jobid = cpu_to_be16(jobid);

	if ((ret = send_data(ctx->conn,
			     (uint8_t*) &req, sizeof(req))))
		return ret;
	memset(resp, 0, sizeof(*resp));
	ret = read_data(ctx->conn,
			(uint8_t*) resp, sizeof(*resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(*resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*resp));
		return 4;
	}

	return CUPS_BACKEND_OK;
}

static int mitsud90_get_jobstatus(struct mitsud90_ctx *ctx, uint16_t jobid)
{
	struct mitsud90_job_resp resp;

	if (mitsud90_query_job(ctx, jobid, &resp))
		return CUPS_BACKEND_FAILED;

	INFO("Job Status:  %04x = %02x/%02x/%04x\n",
	     jobid, resp.unk1, resp.unk2, be16_to_cpu(resp.unk3));

	return CUPS_BACKEND_OK;
}

static int mitsud90_get_media(struct mitsud90_ctx *ctx)
{
	struct mitsud90_media_resp resp;

	if (mitsud90_query_media(ctx, &resp))
		return CUPS_BACKEND_FAILED;

	INFO("Media Type:  %s (%02x/%02x)\n",
	     mitsu_media_types(ctx->conn->type, resp.media.brand, resp.media.type),
	     resp.media.brand,
	     resp.media.type);
	INFO("Prints Remaining:  %03d/%03d\n",
	     be16_to_cpu(resp.media.remain),
	     be16_to_cpu(resp.media.capacity));

	return CUPS_BACKEND_OK;
}

static int mitsud90_get_status(struct mitsud90_ctx *ctx)
{
	struct mitsud90_status_resp resp;

	if (mitsud90_query_status(ctx, &resp))
		return CUPS_BACKEND_FAILED;

	mitsud90_dump_status(&resp);

	return CUPS_BACKEND_OK;
}

static int mitsud90_get_info(struct mitsud90_ctx *ctx)
{
	uint8_t cmdbuf[26];
	int ret, num;
	struct mitsud90_info_resp resp;

	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x47;
	cmdbuf[2] = 0x44;
	cmdbuf[3] = 0x30;
	cmdbuf[4] = 0;
	cmdbuf[5] = 0;
	cmdbuf[6] = 19;  /* Number of commands */

	cmdbuf[7] = COM_STATUS_TYPE_MODEL;
	cmdbuf[8] = COM_STATUS_TYPE_x02;
	cmdbuf[9] = COM_STATUS_TYPE_FW_0b;
	cmdbuf[10] = COM_STATUS_TYPE_FW_MA;

	cmdbuf[11] = COM_STATUS_TYPE_FW_F;
	cmdbuf[12] = COM_STATUS_TYPE_FW_T;
	cmdbuf[13] = COM_STATUS_TYPE_FW_0f;
	cmdbuf[14] = COM_STATUS_TYPE_FW_11;

	cmdbuf[15] = COM_STATUS_TYPE_FW_ME;
	cmdbuf[16] = COM_STATUS_TYPE_x1e;
	cmdbuf[17] = COM_STATUS_TYPE_x22;
	cmdbuf[18] = COM_STATUS_TYPE_x28;

	cmdbuf[19] = COM_STATUS_TYPE_x29;
	cmdbuf[20] = COM_STATUS_TYPE_x2b;
	cmdbuf[21] = COM_STATUS_TYPE_x2c;
	cmdbuf[22] = COM_STATUS_TYPE_x65;

	cmdbuf[23] = D90_STATUS_TYPE_ISEREN;
	cmdbuf[24] = COM_STATUS_TYPE_x83;
	cmdbuf[25] = D90_STATUS_TYPE_x84;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;
	memset(&resp, 0, sizeof(resp));

	ret = read_data(ctx->conn,
			(uint8_t*) &resp, sizeof(resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(resp));
		return 4;
	}

	/* start dumping output */
	memset(cmdbuf, 0, sizeof(cmdbuf));
	memcpy(cmdbuf, resp.model, sizeof(resp.model));
	INFO("Model: %s\n", (char*)cmdbuf);
	INFO("Serial: %s\n", ctx->serno);
	for (num = 0; num < 7 ; num++) {
		memset(cmdbuf, 0, sizeof(cmdbuf));
		memcpy(cmdbuf, resp.fw_vers[num].version, sizeof(resp.fw_vers[num].version));
		INFO("FW Component %02d: %s (%04x)\n",
		     num, cmdbuf, be16_to_cpu(resp.fw_vers[num].csum));
	}
	INFO("TYPE_02: %02x\n", resp.x02);
	INFO("TYPE_1e: %02x\n", resp.x1e);
	INFO("TYPE_22: %02x %02x\n", resp.x22[0], resp.x22[1]);
	INFO("TYPE_28: %04x\n", be16_to_cpu(resp.x28));
	INFO("TYPE_29: %02x %02x %02x %02x %02x %02x %02x %02x\n",
	     resp.x29[0], resp.x29[1], resp.x29[2], resp.x29[3],
	     resp.x29[4], resp.x29[5], resp.x29[6], resp.x29[7]);
	INFO("TYPE_2b: %02x %02x\n", resp.x2b[0], resp.x2b[1]);
	INFO("TYPE_2c: %02x %02x\n", resp.x2c[0], resp.x2c[1]);

	INFO("TYPE_65:");
	for (num = 0; num < 50 ; num++) {
		DEBUG2(" %02x", resp.x65[num]);
	}
	DEBUG2("\n");
	INFO("iSerial: %s\n", resp.iserial ? "Disabled" : "Enabled");
	INFO("TYPE_83: %02x\n", resp.x83);
	INFO("TYPE_84: %02x\n", resp.x84);

	// XXX what about resume, wait time, "cut limit", sleep time ?

	return CUPS_BACKEND_OK;
}

static int mitsum1_get_info(struct mitsud90_ctx *ctx)
{
	uint8_t cmdbuf[25];
	int ret, num;
	struct mitsum1_info_resp resp;

	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x47;
	cmdbuf[2] = 0x44;
	cmdbuf[3] = 0x30;
	cmdbuf[4] = 0;
	cmdbuf[5] = 0;
	cmdbuf[6] = 18;  /* Number of commands */

	cmdbuf[7] = COM_STATUS_TYPE_MODEL;
	cmdbuf[8] = COM_STATUS_TYPE_x02;
	cmdbuf[9] = CM1_STATUS_TYPE_FW_0a;
	cmdbuf[10] = COM_STATUS_TYPE_FW_0b;
	cmdbuf[11] = COM_STATUS_TYPE_FW_MA;

	cmdbuf[12] = COM_STATUS_TYPE_FW_F;
	cmdbuf[13] = COM_STATUS_TYPE_FW_T;
	cmdbuf[14] = COM_STATUS_TYPE_FW_0f;
	cmdbuf[15] = COM_STATUS_TYPE_FW_11;

	cmdbuf[16] = COM_STATUS_TYPE_FW_ME;
	cmdbuf[17] = COM_STATUS_TYPE_x1e;
	cmdbuf[18] = COM_STATUS_TYPE_x22;
	cmdbuf[19] = COM_STATUS_TYPE_x28;

	cmdbuf[20] = COM_STATUS_TYPE_x29;
	cmdbuf[21] = COM_STATUS_TYPE_x2b;
	cmdbuf[22] = COM_STATUS_TYPE_x2c;
	cmdbuf[23] = COM_STATUS_TYPE_x65;

	cmdbuf[24] = COM_STATUS_TYPE_x83;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;
	memset(&resp, 0, sizeof(resp));

	ret = read_data(ctx->conn,
			(uint8_t*) &resp, sizeof(resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(resp));
		return 4;
	}

	/* start dumping output */
	memset(cmdbuf, 0, sizeof(cmdbuf));
	memcpy(cmdbuf, resp.model, sizeof(resp.model));
	INFO("Model: %s\n", (char*)cmdbuf);
	INFO("Serial: %s\n", ctx->serno);
	for (num = 0; num < 8 ; num++) {
		memset(cmdbuf, 0, sizeof(cmdbuf));
		memcpy(cmdbuf, resp.fw_vers[num].version, sizeof(resp.fw_vers[num].version));
		INFO("FW Component %02d: %s (%04x)\n",
		     num, cmdbuf, be16_to_cpu(resp.fw_vers[num].csum));
	}
	INFO("TYPE_02: %02x\n", resp.x02);
	INFO("TYPE_1e: %02x\n", resp.x1e);
	INFO("TYPE_22: %02x %02x\n", resp.x22[0], resp.x22[1]);
	INFO("TYPE_28: %04x\n", be16_to_cpu(resp.x28));
	INFO("TYPE_29: %02x %02x %02x %02x %02x %02x %02x %02x\n",
	     resp.x29[0], resp.x29[1], resp.x29[2], resp.x29[3],
	     resp.x29[4], resp.x29[5], resp.x29[6], resp.x29[7]);
	INFO("TYPE_2b: %02x %02x\n", resp.x2b[0], resp.x2b[1]);
	INFO("TYPE_2c: %02x %02x\n", resp.x2c[0], resp.x2c[1]);

	INFO("TYPE_65:");
	for (num = 0; num < 50 ; num++) {
		DEBUG2(" %02x", resp.x65[num]);
	}
	DEBUG2("\n");
	INFO("TYPE_83: %02x\n", resp.x83);

	// XXX what about resume, wait time, "cut limit", sleep time ?

	return CUPS_BACKEND_OK;
}

static int mitsud90_dumpall(struct mitsud90_ctx *ctx)
{
	int i;
	uint8_t cmdbuf[8];
	uint8_t buf[256];

	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x47;
	cmdbuf[2] = 0x44;
	cmdbuf[3] = 0x30;
	cmdbuf[4] = 0;
	cmdbuf[5] = 0;
	cmdbuf[6] = 0x01;  /* Number of commands */

	for (i = 0 ; i < 256 ; i++) {
		int num, ret;

		cmdbuf[7] = i;

		if ((ret = send_data(ctx->conn,
				     cmdbuf, sizeof(cmdbuf))))
			return ret;
		memset(buf, 0, sizeof(buf));

		ret = read_data(ctx->conn,
				buf, sizeof(buf), &num);

		if (ret < 0)
			continue;

		if (num > 4) {
			DEBUG("TYPE %02x LEN: %d\n", i, num - 4);
			DEBUG("<--");
			for (ret = 4; ret < num ; ret ++) {
				DEBUG2(" %02x", buf[ret]);
			}
			DEBUG2("\n");
		}
	}

	return CUPS_BACKEND_OK;
}

static int mitsud90_test_print(struct mitsud90_ctx *ctx, int type)
{
	uint8_t cmdbuf[16];
	int ret, num = 0;
	uint8_t resp[256];

	/* Send Test ON */
	memset(cmdbuf, 0, 8);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x76;
	cmdbuf[2] = 0x54;
	cmdbuf[3] = 0x45;
	cmdbuf[4] = 0x53;
	cmdbuf[5] = 0x54;
	cmdbuf[6] = 0x4f;
	cmdbuf[7] = 0x4e;
	if ((ret = send_data(ctx->conn,
			     cmdbuf, 8)))
		return ret;

	memset(resp, 0, sizeof(resp));

	ret = read_data(ctx->conn,
			resp, sizeof(resp), &num);  // always e4 44 4f 4e 45

	if (ret) return ret;

	/* Send Test print. */
	memset(cmdbuf, 0x00, 16);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x61;
	cmdbuf[2] = 0x36;
	cmdbuf[3] = 0x31;
	cmdbuf[7] = 0x02;

	switch(type) {
	default:
	case 0: /* Test Print */
		cmdbuf[15] = 0x01;
		break;
	case 1: /* Solid Black */
		cmdbuf[4] = 0x02;
		cmdbuf[9] = 0xFF;
		cmdbuf[15] = 0x01;
		break;
	case 2: /* Solid Gray */
		cmdbuf[4] = 0x02;
		cmdbuf[9] = 0x80;
		cmdbuf[15] = 0x01;
		break;
	case 3: /* Head Pattern */
		cmdbuf[4] = 0x01;
		cmdbuf[10] = 0x01;
		cmdbuf[15] = 0x01;
		break;
	case 4: /* Color Bar */
		cmdbuf[4] = 0x04;
		cmdbuf[15] = 0x01;
		break;
	case 5: /* Vertical Alignment */
		cmdbuf[4] = 0x02;
		cmdbuf[9] = 0x80;
		cmdbuf[15] = 0x02;
		break;
	case 6: /* Horizontal Alignment; Grey Cross */
		cmdbuf[4] = 0x02;
		cmdbuf[9] = 0x80;
		cmdbuf[15] = 0x01;
		break;
	case 7: /* Solid Gray 1 */
		cmdbuf[4] = 0x02;
		cmdbuf[9] = 0x80;
		cmdbuf[10] = 0x01;
		cmdbuf[15] = 0x01;
		break;
	case 8: /* Solid Gray 2 */
		cmdbuf[4] = 0x02;
		cmdbuf[9] = 0x80;
		cmdbuf[10] = 0x02;
		cmdbuf[15] = 0x01;
		break;
	case 9: /* Solid Gray 3 */
		cmdbuf[4] = 0x02;
		cmdbuf[9] = 0x80;
		cmdbuf[10] = 0x04;
		cmdbuf[15] = 0x01;
		break;
	}
	if ((ret = send_data(ctx->conn,
			     cmdbuf, 16)))
		return ret;

	ret = read_data(ctx->conn,
			resp, sizeof(resp), &num); /* Get 5 back */

	return ret;
}

static int mitsud90_query_serno(struct dyesub_connection *conn, char *buf, int buf_len)
{
	struct mitsud90_ctx ctx = {
		.conn = conn,
	};

	int ret;

	UNUSED(buf_len);

	ret = mitsud90_get_serno(&ctx);

	/* Copy it */
	memcpy(buf, ctx.serno, sizeof(ctx.serno));

	return ret;
}
static int mitsud90_set_iserial(struct mitsud90_ctx *ctx, uint8_t enabled)
{
	uint8_t cmdbuf[23];
	int ret, num;

	enabled = (enabled) ? 0: 0x80;

	/* Send Parameter.. */
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x31;
	cmdbuf[2] = 0x36;
	cmdbuf[3] = 0x30;
	cmdbuf[4] = 0x41;
	cmdbuf[5] = 0xbe;
	cmdbuf[6] = 0x00;
	cmdbuf[7] = 0x00;

	cmdbuf[8] = 0x00;
	cmdbuf[9] = 0x01;
	cmdbuf[10] = 0x00;
	cmdbuf[11] = 0x00;
	cmdbuf[12] = 0x00;
	cmdbuf[13] = 0x11;
	cmdbuf[14] = 0xff;
	cmdbuf[15] = 0xff;

	cmdbuf[16] = 0xff;
	cmdbuf[17] = 0xfe;
	cmdbuf[18] = 0xff;
	cmdbuf[19] = 0xff;
	cmdbuf[20] = 0xff;
	cmdbuf[21] = 0xfe;
	cmdbuf[22] = enabled;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;

	ret = read_data(ctx->conn,
			cmdbuf, sizeof(cmdbuf), &num);

	return ret;
}

static int mitsud90_set_sleeptime(struct mitsud90_ctx *ctx, uint16_t time)
{
	uint8_t cmdbuf[24];
	int ret;

	/* 255 minutes max, according to RE work */
	if (time > 255)
		time = 255;

	/* Send Parameter.. */
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x31;
	cmdbuf[2] = 0x36;
	cmdbuf[3] = 0x30;
	cmdbuf[4] = 0x41;
	cmdbuf[5] = 0xbe;
	cmdbuf[6] = 0x00;
	cmdbuf[7] = 0x00;

	cmdbuf[8] = 0x00;
	cmdbuf[9] = 0x02;
	cmdbuf[10] = 0x00;
	cmdbuf[11] = 0x00;
	cmdbuf[12] = 0x05;
	cmdbuf[13] = 0x02;
	cmdbuf[14] = 0xff;
	cmdbuf[15] = 0xff;

	cmdbuf[16] = 0xff;
	cmdbuf[17] = 0xfd;
	cmdbuf[18] = 0xff;
	cmdbuf[19] = 0xff;
	cmdbuf[20] = 0xfa;
	cmdbuf[21] = 0xff;
	cmdbuf[22] = (time >> 8) & 0xff;
	cmdbuf[23] = time & 0xff;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, 4)))
		return ret;

	/* No response */

	return CUPS_BACKEND_OK;
}

static void mitsud90_cmdline(void)
{
	DEBUG("\t\t[ -i ]           # Query printer info\n");
	DEBUG("\t\t[ -j jobid ]     # Query job status\n");
	DEBUG("\t\t[ -k time ]      # Set sleep time in minutes\n");
	DEBUG("\t\t[ -m ]           # Query printer media\n");
	DEBUG("\t\t[ -s ]           # Query printer status\n");
	DEBUG("\t\t[ -x 0|1 ]       # Enable/disable iSerial reporting\n");
//	DEBUG("\t\t[ -T 0-9 ]       # Test print\n");
//	DEBUG("\t\t[ -Z ]           # Dump all parameters\n");
}

static int mitsud90_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct mitsud90_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "ij:k:msT:x:Z")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'i':
			if (ctx->conn->type == P_MITSU_D90)
				j = mitsud90_get_info(ctx);
			else
				j = mitsum1_get_info(ctx);
			break;
		case 'j':
			j = mitsud90_get_jobstatus(ctx, atoi(optarg));
			break;
		case 'k':
			j = mitsud90_set_sleeptime(ctx, atoi(optarg));
			break;
		case 'm':
			j = mitsud90_get_media(ctx);
			break;
		case 's':
			j = mitsud90_get_status(ctx);
			break;
		case 'T':
			j = mitsud90_test_print(ctx, atoi(optarg));
			break;
		case 'x':
			if (ctx->conn->type == P_MITSU_D90)
				j = mitsud90_set_iserial(ctx, atoi(optarg));
			break;
		case 'Z':
			j = mitsud90_dumpall(ctx);
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return CUPS_BACKEND_OK;
}

static int mitsud90_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct mitsud90_ctx *ctx = vctx;
	struct mitsud90_media_resp resp;

	if (markers) *markers = &ctx->marker;
	if (count) *count = 1;

	if (mitsud90_query_media(ctx, &resp))
		return CUPS_BACKEND_FAILED;

	ctx->marker.levelnow = be16_to_cpu(resp.media.remain);

	return CUPS_BACKEND_OK;
}

static int mitsud90_query_stats(void *vctx, struct printerstats *stats)
{
	struct mitsud90_ctx *ctx = vctx;
	struct mitsud90_status_resp resp;

	if (mitsud90_query_markers(ctx, NULL, NULL))
		return CUPS_BACKEND_FAILED;
	if (mitsud90_query_status(ctx, &resp))
		return CUPS_BACKEND_FAILED;

	switch (ctx->conn->type) {
	case P_MITSU_D90:
		stats->mfg = "Mitsubishi";
		stats->model = "CP-D90 family";
		break;
	case P_MITSU_M1:
		stats->mfg = "Mitsubishi";
		stats->model = "CP-M1 family";
		break;
	case P_FUJI_ASK500:
		stats->mfg = "Fujifilm";
		stats->model = "AK500";
		break;
	default:
		stats->model = "Unknown!";
		break;
	}

	stats->serial = ctx->serno;
	stats->fwver = ctx->fwver;

	stats->decks = 1;

	stats->name[0] = "Roll";
	if (resp.code[0] != D90_ERROR_STATUS_OK)
		stats->status[0] = strdup(mitsud90_error_codes(resp.code));
	else if (resp.code[1] & D90_ERROR_STATUS_OK_WARMING ||
		 resp.temp & D90_ERROR_STATUS_OK_WARMING)
		stats->status[0] = strdup("Warming up");
	else if (resp.code[1] & D90_ERROR_STATUS_OK_COOLING ||
		 resp.temp & D90_ERROR_STATUS_OK_COOLING)
		stats->status[0] = strdup("Cooling down");
	else
		stats->status[0] = strdup(mitsud90_mecha_statuses(resp.mecha));

	stats->mediatype[0] = ctx->marker.name;
	stats->levelmax[0] = ctx->marker.levelmax;
	stats->levelnow[0] = ctx->marker.levelnow;
	// stats->cnt_life[0] = ??? // XXX Don't know about any counters yet.

	return CUPS_BACKEND_OK;
}

static const char *mitsud90_prefixes[] = {
	"mitsud90", /* Family Name */
	NULL
};

/* Exported */
const struct dyesub_backend mitsud90_backend = {
	.name = "Mitsubishi CP-D90/CP-M1",
	.version = "0.37"  " (lib " LIBMITSU_VER ")",
	.uri_prefixes = mitsud90_prefixes,
	.cmdline_arg = mitsud90_cmdline_arg,
	.cmdline_usage = mitsud90_cmdline,
	.init = mitsud90_init,
	.attach = mitsud90_attach,
	.teardown = mitsud90_teardown,
	.cleanup_job = mitsud90_cleanup_job,
	.read_parse = mitsud90_read_parse,
	.main_loop = mitsud90_main_loop,
	.query_serno = mitsud90_query_serno,
	.query_markers = mitsud90_query_markers,
	.query_stats = mitsud90_query_stats,
	.devices = {
		{ 0x06d3, 0x3b60, P_MITSU_D90, NULL, "mitsubishi-d90dw"},
		{ 0x06d3, 0x3b80, P_MITSU_M1, NULL, "mitsubishi-cpm1"},
		{ 0x06d3, 0x3b80, P_MITSU_M1, NULL, "mitsubishi-cpm15"}, // Duplicate for the M15
//		{ 0x04cb, 0x1234, P_FUJI_ASK500, NULL, "fujifilm-ask500"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* ToDo:

     * consolidate M1 vs D90 info query/dump more efficiently
     * job control (job id, active job, buffer status, etc)
     * any sort of counters
     * sleep and waking up
     * cut limit?
     * Validate Fujifilm ASK500 support
     * Confirm ASK500 spool format
     * Validate Panorama mode

 */

/*
   Mitsubishi CP-D90DW data format

   All multi-byte values are BIG endian

 [[HEADER 1]]

   1b 53 50 30 00 33 XX XX  YY YY TT 00 00 01 MM NN  XX XX == COLS, YY XX ROWS (BE)
   ?? ?? ?? ?? ?? ?? ?? ??  ?? ?? ?? ?? 00 00 00 00  NN == num of cuts, ?? see below
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  MM == 0 for no margin cut, 1 for margin cut
   QQ RR SS HH VV 00 00 00  00 00 ZZ 00 JJ II 09 7c  QQ == 02 matte (D90) or 03 (M1), 00 glossy,
   09 4c 00 00 02 58 00 0c  00 06 00 00 00 00 00 00  RR == 00 auto, (D90: 03 == fine, 02 == superfine), (M1: 05 == Fast)
   Z0 Z1 Z2 00 00 00 00 00  00 00 00 00 00 00 00 00  SS == 00 colorcorr, 01 == none (always 01 on M1)
                                                     HH/VV sharpening for Horiz/Vert, 0-8, 0 is off, 4 is normal (always 00 on M1)
                                                     TT is waittime (100 max, always 100 on D90)
						     ZZ is 0x02 on M1, D90 see below
						     Z0 is 0x01 (M1 windows) (00 Linux and d90 UNK!)
						     Z1 is RGB Rate (M1)
						     Z2 is OP Rate (M1)
  [pad to 512b]

                normal  == rows  00  00 00 00 00  00 00 00 00
                4x6div2 == 1226  01  02 65 01 00  00 00 00 00
                8x6div2 == 2488  01  04 be 00 00  00 00 00 00

		    guesses based on SDK docs:

		9x6div2 == 2728  01  05 36 01 00  00 00 00 00  00 00 00 00
		9x6div3 == 2724  02  03 90 01 00  07 14 00 00  00 00 00 00
		9x6div4 == 2628  03  02 97 01 00  05 22 00 00  07 ad 00 00

    from [ZZ 00 03 03] onwards, only shows in 8x20" PANORAMA prints.  Assume 2" overlap.
    ZZ == 00 (normal) or 01 (panorama)
    JJ == 02 03 (num of panorama panels)
    II == 01 02 03 (which panel # in panorama!)
    [02 58] == 600, aka 2" * 300dpi?
    [09 4c] == 2380  (48 less than 8 size? (trim length on ends?)
    [09 7c] == 2428  (ie 8" print)

     (6x20 == 1852x6036)
     (6x14 == 1852x4232)

     3*8" panels == 2428*3=7284.  -6036 = 1248.  /2 = 624 (0x270)

 [[DATA PLANE HEADER]]

   1b 5a 54 01 00 09 00 00  00 00 CC CC RR RR 00 00
   00 00 00 00 LC LC LR LR
   ...
   [pad to 512b]

   CC CC cols (BE)
   RR RR rows (BE)
   LC LC lamination columns (BE, M1 only, same as cols)
   LR LR lamination rows (BE, M1 only, rows + 12d )

   D90 family:
    data is *RGB* packed, @ 8bpp.  No padding to 512b!
   M1 family:
    data is *RGB* packed, @16bpp, LITTLE ENDIAN.  No padding to 512b!
    optional matte data is 8bpp, follows immediately.

 [[FOOTER]]

   1b 42 51 31 00 TT                  ## TT == secs to wait for second print, 0xff also valid for something?


 ****************************************************

Comms Protocol for D90 & CP-M1

 [[ ERROR STATUS ]]

-> 1b 47 44 30 00 00 01 16
<- e4 47 44 30 00 00 00 00  00 00 00 00 00 00 00   [Normal/OK]
<- e4 47 44 30 XX 00 00 00  00 00 00 00 00 3f 37   [Error condition]
                                                   XX == 29 (printer open)
                                                         28 (cut bin missing)
<- e4 47 44 30 21 90 00 00  01 00 00 00 00 3f 37   No ribbon

 [[ MEDIA STATUS ]]

-> 1b 47 44 30 00 00 01 2a
<- e4 47 44 30 ff 0f 50 00  01 ae 01 9b 01 00      [Normal/OK]
<- e4 47 44 30 ff ff ff ff  ff ff ff ff ff ff      [Error]

 [[ MECHA STATUS ]]

-> 1b 47 44 30 00 00 01 17
<- e4 47 44 30 SS SS

 [[ TEMPERATURE QUERY ]]

-> 1b 47 44 30 00 00 01 1f
<- e4 47 44 30 HH

 [[ UNKNOWN QUERY ]]

-> 1b 47 44 30 00 00 01 28
<- e4 47 44 30 XX XX        Unknown, seems to increment.  Lifetime counter?

 [[ JOB STATUS QUERY ?? ]]

-> 1b 47 44 31 00 00 JJ JJ  Jobid?
<- e4 47 44 31 XX YY ZZ ZZ  No idea... maybe remaining prints?

 [[ COMBINED STATUS QUERIES ]]

-> 1b 47 44 30 00 00 04 16  17 1f 2a
<- e4 47 44 30

   MM NN 00 00 ZZ 00 00 00  00 QQ QQ   [id 16, total 11]
   SS SS                               [id 17, total 2]
   HH                                  [id 1f, total 1]
   VV TT WW 00 XX XX YY YY  01 00      [id 2a, total 10]

   WW    == 0x50 or 0x00 (seen, no idea what it means)
   VV    == Media vendor (0xff etc)
   TT    == Media type, 0x02/0x0f etc (see mitsu_media_types!)
   XX XX == Media capacity, BE
   YY YY == Media remain,   BE
   QQ QQ == 00 00 normal, 3f 37 error
   MM NN == MM major err (00 if no error) NN minor error.
   ZZ    == 01 seen for _some_ errors.
   SS SS == Mecha Status  (00 == ready, 50 == printing, 80+10 == feedandcut, 80 == initializing?
   HH    == Temperature state.  00 is OK, 0x40 is low, 0x80 is hot.
   II II == ??
   JJ JJ == ??

 [[ WAKE UP PRINTER ]]
-> 1b 45 57 55

 [[ GET iSERIAL Setting ]]

-> 1b 61 36 36 41 be 00 00
   00 01 00 00 00 11 ff ff
   ff fe ff ff ff ee
<- e4 61 36 36 41 be 00 00
   00 01 00 00 00 11 ff ff
   ff fe ff ff ff ee XX      <- XX is 0x80 or 0x00.  (0x80)  ISERIAL OFF

 [[ GET SERIAL NUMBER ]]

-> 1b 61 36 36 41 be 00 00
   00 06 00 00 00 30 ff ff
   ff f9 ff ff ff cf
<- e4 61 36 36 41 00 00 00
   00 06 00 00 00 30 ff ff
   ff f9 ff ff ff cf XX XX
   XX XX XX XX               <- XX is 6-char ASCII serial number!

 [[ GET CUT? ]]

-> 1b 61 36 36 45 ba 00 00
   00 01 00 00 05 07 ff ff
   ff fe ff ff fa f8
-> e4 61 36 36 45 ba 00 00
   00 01 00 00 05 07 ff ff
   ff fe ff ff fa f8 XX      <- XX is 0x80 or 0x00    (0x00)  CUT ON?

 [[ GET WAIT TIME ]]

-> 1b 61 36 36 45 00 00 00
   00 01 00 00 05 05 ff ff
   ff fe ff ff fa fb
-> 1b 61 36 36 45 00 00 00
   00 01 00 00 05 05 ff ff
   ff fe ff ff fa fb XX      <- XX is time in seconds.

 [[ GET RESUME? ]]

-> 1b 61 36 36 45 ba 00 00
   00 01 00 00 05 06 ff ff
   ff fe ff ff fa f9
-> e4 61 36 36 45 ba 00 00
   00 01 00 00 05 06 ff ff
   ff fe ff ff fa f9 XX      <- XX is 0x80 or 0x00    (0x80)  (OFF)

 [[ GET SLEEP TIME! ]]

-> 1b 61 36 36 45 ba 00 00
   00 02 00 00 05 02 ff ff
   ff fd ff ff fa fd
<- e4 61 36 36 45 00 00 00
   00 02 00 00 05 02 ff ff
   ff fd ff ff fa fd XX 00     <- XX, sleep time in minutes.

 [[ SET SLEEP TIME! ]]

-> 1b 61 36 30 45 ba 00 00
   00 02 00 00 05 02 ff ff
   ff fd ff ff fa fd XX 00     <- XX, sleep time in minutes.

 [[ SET iSERIAL ]]

-> 1b 61 36 30 41 be 00 00
   00 01 00 00 00 11 ff ff
   ff fe ff ff ff ee XX        <- XX 0x80 OFF, 0x00 ON.

 [[ SANITY CHECK PRINT ARGUMENTS / MEM CHECK ]]

-> 1b 47 44 33 00 33 07 3c  04 ca 64 00 00 01 00 00
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   00 00 00 04 04 00 00 00  00 00 00 00 00 00 00 00
   [[ pad to 512 ]]

   ... 07 3c onwards is the same as main payload header.

<- e4 47 44 43 XX YY

   ... possibly the same as the D70's "memorystatus"
       XX == 00 size ok, 01 bad size, ff out of range
       YY == 00 memory ok, 01 memory full, 02 driver setting, ff out of range

 [[ SEND OVER HDRs and DATA ]]

   ... Print arguments:

-> 1b 53 50 30 00 33 07 3c  04 ca 64 00 00 01 00 00
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   00 00 00 04 04 00 00 00  00 00 00 00 00 00 00 00
   [[ pad to 512 ]]

   ... Data transfer.  Plane header:

-> 1b 5a 54 01 00 09 00 00  00 00 07 3c 04 ca 00 00
   [[ pad to 512 ]]

-> [[print data]] [[ padded? ]]
-> [[print data]]

-> 1b 42 51 31 00 ZZ

   ... Footer.
   ZZ == Seconds to wait for follow-up print (0x05)

   ALSO SEEN (in SDK)

   1b 42 61 32 00 00

 [[ UNKNOWN (seen in SDK) ]]

   1b 44 43 41  4e 43 45 4c  00 00 00 00      : \ESC D CANCEL

 [[ UNKNOWON (seen in SDK) ]]

   1b 42 51 32 00 00       [ Footer of some sort ? ]

 [[ GENERIC GET/SET ]]

-> 1b 61 36 QQ T1 T2 LL LL   QQ == 0x30 (set) 0x36 (get)
   LL LL 00 00 VV VV ff ff   LL == length (32-bit BE)
   ff fd ff ff fa fd         T1 = type1 (41, 45, others?)
<- e4 61 36 QQ TT TT 00 00   T2 = type2 (be, ba, 00, others?)
   LL LL 00 00 VV VV ff ff   VV VV = index/variable
   ff fd ff ff fa fd ?? ??   ?? == data (length LL)

    The 'ff ff ff fd  ff ff fa fd' varies; Maybe a mask?

  -----------------------------------------------
   T1 T2  LL LL  VV VV  M1 M1  M2 M2   Meaning

   41 be  00 01  00 10  ff fe  ff ef   34v Adjustment (0x00->0xff)
   41 be  00 01  00 11  ff fe  ff ee   iSerial setting
   41 be  00 06  00 30  ff f9  ff cf   Ascii serial number
   45 ba  00 02  05 02  ff fd  fa fd   Sleep Time
   45 00  00 01  05 05  ff fe  fa f8   Wait time (seconds)
   45 ba  00 01  05 06  ff fe  fa fb   Resume on/off
   45 ba  00 01  05 07  ff fe  fa f8   Cutter on/off
   45 ba  00 02  02 40  ff fd  fd bf   Density (6800d -> 9000d)
   45 ba  00 06  03 00  ff f9  fc ff   M1 Adj (F/SF/UF, two bytes each?)
   45 ba  00 10  04 10  ff ef  fb ef   M3 Adj (unknown value)
   45 ba  00 06  03 10  ff f9  fc ef   Vertical Position (A/B/C combined)
   45 ba  00 02  03 16  ff fd  fc e9   Feed (default 43402d)
   45 ba  00 01  02 47  ff fe  fd 87   Horizontal Position (0x00->0xff)
   45 ba  19 c0  02 48  e6 3f  f9 bf   "Read Info" (BIG payload!)
   45 ba  00 04  06 40  ff fe  f9 bf   Head Count
   45 ba  00 04  06 44  ff fb  f9 bb   Cutter Count
   45 ba  14 00  0c 00  eb ff  f3 ff   Error History (BIG payload)

 ALSO SEEN:

   1b 61 36 39 43 00   "AdjustColSCmd"
   1b 61 36 37 39 43   "GetColSCmd"  (12 len payload)
   1b 6a 30 71 31 31 42 38 "SetM1AdjCmd"
   1b 6a 36 34 31 00   "GetM1AdjCmd"
   1b 6a 31 32 51 30 38 30 30 30 30 31 "M1AdjSolidGreyCmd"  ???
   1b 61 36 34 50 00   "PaperSensorAdjCmd"
   1b 61 36 37 34 50   "PaperSensorGetCmd" (24 len payload)
   1b 61 36 36 45 ba   Read EEProm  (16 byte cmd payload, 0x8000 max len)
   1b 61 36 30 45 ba   Write EEProm
   1b 47 44 30 00 00 01 65  Read Sensors (streams?)

 request x65 examples:

   ac 80 00 01 bb b8 fe 48 05 13 5d 9c 00 33 00 00  00 00 00 00 00 00 00 00 00 00 02 39 00 00 00 00  03 13 00 02 10 40 00 00 00 00 00 00 05 80 00 3a  00 00
   aa 79 00 01 bb b7 fe 47 05 13 5d 9c 01 2f 00 68  00 00 00 00 00 00 00 00 00 00 02 08 00 00 00 00  03 14 00 02 10 40 00 00 00 00 00 00 05 80 00 3a  00 00

 [ power cycle, new capture ]
   a3 5d 00 01 ba ba fe 43 04 13 5d 9c 00 00 00 00  00 00 00 00 00 00 00 00 00 00 02 0c 00 00 00 00  03 0f 00 03 10 40 00 00 00 00 00 00 05 80 00 3a  00 00
   a3 5d 00 01 ba ba fe 42 04 13 5d 9c 01 08 00 87  00 00 00 00 00 00 00 00 00 00 01 e5 00 00 00 00  03 0f 00 03 10 40 00 00 00 00 00 00 05 80 00 3a  00 00
   a2 5d 00 01 ba ba fe 42 06 13 5d 9c 01 08 00 87  00 00 00 00 00 00 00 00 00 00 01 d1 00 00 00 00  03 0f 00 03 10 40 00 00 00 00 00 00 05 80 00 3a  00 00
 [ power cycle ]
   a2 5c 00 01 ba ba fe 42 06 13 5d 9c 00 00 00 00  00 00 00 00 00 00 00 00 00 00 01 e0 00 00 00 00  03 0f 00 03 10 40 00 00 00 00 00 00 05 80 00 3a  00 00
   a2 5d 00 01 ba ba fe 41 04 13 5d 9c 01 08 00 89  00 00 00 00 00 00 00 00 00 00 01 c9 00 00 00 00  03 0f 00 03 10 40 00 00 00 00 00 00 05 80 00 3a  00 00
 [ cp-m1 ]
   00 00 01 f2 00 07 00 00 00 0f 00 a7 02 9f 03 91  00 00 00 00 00 00 02 36 00 07 03 ff 02 07 03 ff  03 4c 00 01 10 00 00 00 00 00 00 00 05 80 00 24  04 00

  D90 Panorama data table files ("CP90PAN??.dat")

  struct win_pano {   // All files are LE
    uint32_t header;          // @0     0x00000007 (ie number of ymc tuples)
    uint32_t [3][16] table1;  // @4     YMC values, only first 7 used
    uint32_t pad;             // @192   0x00000000
    uint32_t header2;         // @196   0x00000011  (ie number of bgr tuples?)
    uint32_t [3][17] table2;  // @200   BGR values
    double   table3[600][184] // @408    TBD (or maybe 600*23*8 ??)
    double   unk[]            // @110808 TBD
    uint8_t  footer[8]        // @71208408 "PA17424a"
  };

    -- Table 3 seems to be a set of 600 row blocks  (1 per overlap row?)

 */
