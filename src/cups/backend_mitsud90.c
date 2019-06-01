/*
 *   Mitsubishi CP-D90DW Photo Printer CUPS backend
 *
 *   (c) 2018 Solomon Peachy <pizza@shaftnet.org>
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *          [http://www.gnu.org/licenses/gpl-2.0.html]
 *
 *   SPDX-License-Identifier: GPL-2.0+
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

#define BACKEND mitsud90_backend

#include "backend_common.h"

#define USB_VID_MITSU       0x06D3
#define USB_PID_MITSU_D90   0x3B60

const char *mitsu70x_media_types(uint8_t brand, uint8_t type);
const char *mitsu70x_temperatures(uint8_t temp);

/* Printer data structures */
#define D90_STATUS_TYPE_MODEL  0x01 // 10, null-terminated ASCII. 'CPD90D'
#define D90_STATUS_TYPE_x02    0x02 // 1, 0x5f ?
#define D90_STATUS_TYPE_FW_0b  0x0b // 8, 34 31 34 42 31 31 a7 de (414D11)
#define D90_STATUS_TYPE_FW_MA  0x0c // 8, 34 31 35 41 38 31 86 bf (415A81)  // MAIN FW
#define D90_STATUS_TYPE_FW_F   0x0d // 8, 34 31 36 41 35 31 dc 8a (416A51)  // FPGA FW
#define D90_STATUS_TYPE_FW_T   0x0e // 8, 34 31 37 45 31 31 e7 e6 (417E11)  // TABLE FW
#define D90_STATUS_TYPE_FW_0f  0x0f // 8, 34 31 38 41 31 32 6c 64 (418A12)
#define D90_STATUS_TYPE_FW_11  0x11 // 8, 34 32 31 51 31 31 74 f2 (421Q11)
#define D90_STATUS_TYPE_FW_ME  0x13 // 8, 34 31 39 45 31 31 15 bf (419E11)  // MECHA FW

#define D90_STATUS_TYPE_ERROR  0x16 // 11 (see below)
#define D90_STATUS_TYPE_MECHA  0x17 // 2  (see below)
#define D90_STATUS_TYPE_x1e    0x1e // 1, power state or time?  (x00)
#define D90_STATUS_TYPE_TEMP   0x1f // 1  (see below)
#define D90_STATUS_TYPE_x22    0x22 // 2,  all 0
#define D90_STATUS_TYPE_x28    0x28 // 2,  all 0, seen some sort of counter?
#define D90_STATUS_TYPE_x29    0x29 // 8,  e0 07 00 00 21 e6 b3 22
#define D90_STATUS_TYPE_MEDIA  0x2a // 10 (see below)
#define D90_STATUS_TYPE_x2b    0x2b // 2,  all 0
#define D90_STATUS_TYPE_x2c    0x2c // 2,  00 56
#define D90_STATUS_TYPE_x65    0x65 // 50, ac 80 00 01 bb b8 fe 48 05 13 5d 9c 00 33 00 00  00 00 00 00 00 00 00 00 00 00 02 39 00 00 00 00  03 13 00 02 10 40 00 00 00 00 00 00 05 80 00 3a  00 00
#define D90_STATUS_TYPE_x82    0x82 // 1,  80 (iserial disabled?)
#define D90_STATUS_TYPE_x83    0x83 // 1,  00
#define D90_STATUS_TYPE_x84    0x84 // 1,  00

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
	} __attribute__((packed)) media; /* D90_STATUS_TYPE_MEDIA */
} __attribute__((packed));

struct mitsud90_status_resp {
	uint8_t  hdr[4];  /* e4 47 44 30 */
	/* D90_STATUS_TYPE_ERROR */
	uint8_t  code[2]; /* 00 is ok, nonzero is error */
	uint8_t  unk[9];
	/* D90_STATUS_TYPE_MECHA */
	uint8_t  mecha[2];
	/* D90_STATUS_TYPE_TEMP */
	uint8_t  temp;
} __attribute__((packed));

struct mitsud90_info_resp {
	uint8_t  hdr[4];  /* e4 47 44 30 */
	uint8_t  model[10];
	uint8_t  x02;
	struct mitsud90_fw_resp_single fw_vers[7];
	uint8_t  x1e;
	uint8_t  x22[2];
	uint8_t  x28[2];
	uint8_t  x29[8];
	uint8_t  x2b[2];
	uint8_t  x2c[2];
	uint8_t  x65[50];
	uint8_t  x82;
	uint8_t  x83;
	uint8_t  x84;
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
};

struct mitsud90_job_resp {
	uint8_t  hdr[4];  /* e4 47 44 31 */
	uint8_t  unk1;
	uint8_t  unk2;
	uint16_t unk3;
};

struct mitsud90_job_hdr {
	uint8_t  hdr[6]; /* 1b 53 50 30 00 33 */
	uint16_t cols;   /* BE */
	uint16_t rows;   /* BE */
	uint8_t  unk[5]; /* 64 00 00 01 00 */
	union {
#if 0
		struct {
			uint8_t  margin;
			uint16_t position;
		} cuts[3] __attribute__((packed));
#endif
		uint8_t cutzero[9];
	} __attribute__((packed));
	uint8_t  zero[24];

	uint8_t  overcoat;
	uint8_t  quality;
	uint8_t  colorcorr;
	uint8_t  sharp_h;
	uint8_t  sharp_v;
	uint8_t  zero_b[5];
	union {
		struct {
			uint16_t pano_on;   /* 0x0001 when pano is on,  */
			uint8_t  pano_tot;  /* 2 or 3 */
			uint8_t  pano_pg;   /* 1, 2, 3 */
			uint16_t pano_rows; /* always 0x097c (BE), ie 2428 ie 8" print */
			uint16_t pano_rows2; /* Always 0x30 less than pano_rows */
			uint16_t pano_zero; /* 0x0000 */
			uint8_t  pano_unk[6];  /* 02 58 00 0c 00 06 */
		} pano __attribute__((packed));
		uint8_t zero_c[16];
	};
	uint8_t zero_d[6];
	uint8_t zero_fill[432];
} __attribute__((packed));

struct mitsud90_plane_hdr {
	uint8_t  hdr[10]; /* 1b 5a 54 01 00 09 00 00 00 00 */
	uint16_t cols;  /* BE */
	uint16_t rows;  /* BE */
	uint8_t  zero_fill[498];
};

struct mitsud90_job_footer {
	uint8_t hdr[4]; /* 1b 42 51 31 */
	uint8_t pad;
	uint8_t seconds; /* 0x05 by default (windows) */
};

struct mitsud90_memcheck {
	uint8_t  hdr[4]; /* 1b 47 44 33 */
	uint8_t  unk[2]; /* 00 33 */
	uint16_t cols;   /* BE */
	uint16_t rows;   /* BE */
	uint8_t  unk_b[4]; /* 64 00 00 01  */
	uint8_t  zero_fill[498];
};

struct mitsud90_memcheck_resp {
	uint8_t  hdr[4];   /* e4 47 44 43 */
	uint8_t  size_bad; /* 0x00 is ok */
	uint8_t  mem_bad;  /* 0x00 is ok */
};

const char *mitsud90_mecha_statuses(const uint8_t *code)
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

const char *mitsud90_error_codes(const uint8_t *code)
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
	     mitsu70x_temperatures(resp->temp));
}

/* Private data structure */
struct mitsud90_printjob {
	uint8_t *databuf;
	int datalen;
	int copies;
};

struct mitsud90_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;

	int type;

	/* Used in parsing.. */
	struct mitsud90_job_footer holdover;
	int holdover_on;

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
	cmdbuf[7] = D90_STATUS_TYPE_MEDIA;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;
	memset(resp, 0, sizeof(*resp));

	ret = read_data(ctx->dev, ctx->endp_up,
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
	cmdbuf[7] = D90_STATUS_TYPE_ERROR;
	cmdbuf[8] = D90_STATUS_TYPE_MECHA;
	cmdbuf[9] = D90_STATUS_TYPE_TEMP;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;
	memset(resp, 0, sizeof(*resp));

	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*) resp, sizeof(*resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(*resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*resp));
		return 4;
	}

	return CUPS_BACKEND_OK;
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

static int mitsud90_attach(void *vctx, struct libusb_device_handle *dev, int type,
			    uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct mitsud90_ctx *ctx = vctx;
	struct mitsud90_media_resp resp;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;
	ctx->type = type;

	if (test_mode < TEST_MODE_NOATTACH) {
		if (mitsud90_query_media(ctx, &resp))
			return CUPS_BACKEND_FAILED;
	} else {
		resp.media.brand = 0xff;
		resp.media.type = 0x0f;
		resp.media.capacity = cpu_to_be16(230);
		resp.media.remain = cpu_to_be16(200);
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = mitsu70x_media_types(resp.media.brand, resp.media.type);
	ctx->marker.levelmax = be16_to_cpu(resp.media.capacity);
	ctx->marker.levelnow = be16_to_cpu(resp.media.remain);

	return CUPS_BACKEND_OK;
}

static void mitsud90_cleanup_job(const void *vjob)
{
	const struct mitsud90_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);

	free((void*)job);
}

static int mitsud90_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct mitsud90_ctx *ctx = vctx;
	int i, remain;
	struct mitsud90_job_hdr *hdr;

	struct mitsud90_printjob *job;;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));
	job->copies = copies;

	/* Just allocate a worst-case buffer */
	job->datalen = 0;
	job->databuf = malloc(sizeof(struct mitsud90_job_hdr) +
			      sizeof(struct mitsud90_plane_hdr) +
			      sizeof(struct mitsud90_job_footer) +
			      1852*2729*3);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		mitsud90_cleanup_job(job);
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	/* Make sure there's no holdover */
	if (ctx->holdover_on) {
		memcpy(job->databuf, &ctx->holdover, sizeof(ctx->holdover));
		job->datalen += sizeof(ctx->holdover);
		ctx->holdover_on = 0;
	}

	/* Read in first header. */
	remain = sizeof(struct mitsud90_job_hdr) - job->datalen;
	while (remain) {
		i = read(data_fd, (job->databuf + job->datalen), remain);
		if (i == 0) {
			mitsud90_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		if (i < 0) {
			mitsud90_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		remain -= i;
		job->datalen += i;
	}

	/* Sanity check header */
	hdr = (struct mitsud90_job_hdr *) job->databuf;
	if (hdr->hdr[0] != 0x1b ||
	    hdr->hdr[1] != 0x53 ||
	    hdr->hdr[2] != 0x50 ||
	    hdr->hdr[3] != 0x30 ) {
		ERROR("Unrecognized data format (%02x%02x%02x%02x)!\n",
		      hdr->hdr[0], hdr->hdr[1], hdr->hdr[2], hdr->hdr[3]);
		mitsud90_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Now read in the rest */
	remain = sizeof(struct mitsud90_plane_hdr) + be16_to_cpu(hdr->cols) * be16_to_cpu(hdr->rows) * 3;
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

	/* Read in the footer.  Hopefully. */
	remain = sizeof(struct mitsud90_job_footer);
	i = read(data_fd, job->databuf + job->datalen, remain);
	if (i == 0) {
		mitsud90_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}
	if (i < 0) {
		mitsud90_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* See if this is a job footer.  If it is, keep, else holdover. */
	if (job->databuf[job->datalen + 0] != 0x1b ||
	    job->databuf[job->datalen + 1] != 0x42 ||
	    job->databuf[job->datalen + 2] != 0x51 ||
	    job->databuf[job->datalen + 3] != 0x31) {
		memcpy(&ctx->holdover, job->databuf + job->datalen, sizeof(struct mitsud90_job_footer));
	        ctx->holdover_on = 1;
	} else {
		job->datalen += i;
		ctx->holdover_on = 0;
	}

	/* Sanity check */
	if (hdr->pano.pano_on) {
		ERROR("Unable to handle panorama jobs yet\n");
		mitsud90_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int mitsud90_main_loop(void *vctx, const void *vjob) {
	struct mitsud90_ctx *ctx = vctx;
	struct mitsud90_job_hdr *hdr;
	struct mitsud90_status_resp resp;
	uint8_t last_status[2] = {0xff, 0xff};

	int sent;
	int ret;
	int copies;

	const struct mitsud90_printjob *job = vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;
	copies = job->copies;

	hdr = (struct mitsud90_job_hdr*) job->databuf;

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

		memcpy(&mem, hdr, sizeof(mem));
		mem.hdr[0] = 0x1b;
		mem.hdr[1] = 0x47;
		mem.hdr[2] = 0x44;
		mem.hdr[3] = 0x33;

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &mem, sizeof(mem))))
			return CUPS_BACKEND_FAILED;

		ret = read_data(ctx->dev, ctx->endp_up,
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

	/* Send header */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     job->databuf + sent, sizeof(*hdr))))
		return CUPS_BACKEND_FAILED;
	sent += sizeof(*hdr);

	/* Send Plane header */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     job->databuf + sent, sizeof(*hdr))))
		return CUPS_BACKEND_FAILED;
	sent += sizeof(*hdr);

	/* Send payload + footer */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     job->databuf + sent, job->datalen - sent)))
		return CUPS_BACKEND_FAILED;
//	sent += (job->datalen - sent);

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

		if (fast_return && copies <= 1) { /* Copies generated by backend? */
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

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) &req, sizeof(req))))
		return ret;
	memset(resp, 0, sizeof(*resp));
	ret = read_data(ctx->dev, ctx->endp_up,
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
	     mitsu70x_media_types(resp.media.brand, resp.media.type),
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

int mitsud90_get_info(struct mitsud90_ctx *ctx)
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

	cmdbuf[7] = D90_STATUS_TYPE_MODEL;
	cmdbuf[8] = 0x02;
	cmdbuf[9] = 0x0b;
	cmdbuf[10] = 0x0c;

	cmdbuf[11] = 0x0d;
	cmdbuf[12] = 0x0e;
	cmdbuf[13] = 0x0f;
	cmdbuf[14] = 0x11;

	cmdbuf[15] = 0x13;
	cmdbuf[16] = 0x1e;
	cmdbuf[17] = 0x22;
	cmdbuf[18] = 0x28;

	cmdbuf[19] = 0x29;
	cmdbuf[20] = 0x2b;
	cmdbuf[21] = 0x2c;
	cmdbuf[22] = 0x65;

	cmdbuf[23] = 0x82;
	cmdbuf[24] = 0x83;
	cmdbuf[25] = 0x84;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;
	memset(&resp, 0, sizeof(resp));

	ret = read_data(ctx->dev, ctx->endp_up,
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
	for (num = 0; num < 7 ; num++) {
		memset(cmdbuf, 0, sizeof(cmdbuf));
		memcpy(cmdbuf, resp.fw_vers[num].version, sizeof(resp.fw_vers[num].version));
		INFO("FW Component %02d: %s (%04x)\n",
		     num, cmdbuf, be16_to_cpu(resp.fw_vers[num].csum));
	}
	INFO("TYPE_02: %02x\n", resp.x02);
	INFO("TYPE_1e: %02x\n", resp.x1e);
	INFO("TYPE_22: %02x %02x\n", resp.x22[0], resp.x22[1]);
	INFO("TYPE_28: %02x %02x\n", resp.x28[0], resp.x28[1]);
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
	INFO("TYPE_1e: %82x\n", resp.x82);
	INFO("TYPE_1e: %83x\n", resp.x83);

	/* XXX Dump iSerial, sleep time settings */
	// XXX what about resume, wait time, "cut limit" ?

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

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     cmdbuf, sizeof(cmdbuf))))
			return ret;
		memset(buf, 0, sizeof(buf));

		ret = read_data(ctx->dev, ctx->endp_up,
				buf, sizeof(buf), &num);

		if (ret <= 0)
			continue;

		if (num > 4) {
			DEBUG("TYPE %02x LEN: %d (%d)\n", i, num, num - 4);
			DEBUG("<--");
			for (ret = 0; ret < num ; ret ++) {
				DEBUG2(" %x", buf[ret]);
			}
			DEBUG2("\n");
		}
	}

	return CUPS_BACKEND_OK;
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

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, sizeof(cmdbuf))))
		return ret;

	ret = read_data(ctx->dev, ctx->endp_up,
			cmdbuf, sizeof(cmdbuf), &num);

	/* No response */

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

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     cmdbuf, 4)))
		return ret;

	/* No response */

	return 0;
}

static void mitsud90_cmdline(void)
{
	DEBUG("\t\t[ -i ]           # Query printer info\n");
	DEBUG("\t\t[ -j jobid ]     # Query job status\n");
	DEBUG("\t\t[ -k time ]      # Set sleep time in minutes\n");
	DEBUG("\t\t[ -m ]           # Query printer media\n");
	DEBUG("\t\t[ -s ]           # Query printer status\n");
	DEBUG("\t\t[ -x 0|1 ]       # Enable/disable iSerial reporting\n");
//	DEBUG("\t\t[ -Z ]           # Dump all parameters\n");
}

static int mitsud90_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct mitsud90_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "ij:k:msx:Z")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'i':
			j = mitsud90_get_info(ctx);
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
		case 'x':
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

	return 0;
}

static int mitsud90_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct mitsud90_ctx *ctx = vctx;
	struct mitsud90_media_resp resp;

	*markers = &ctx->marker;
	*count = 1;

	if (mitsud90_query_media(ctx, &resp))
		return CUPS_BACKEND_FAILED;

	ctx->marker.levelnow = be16_to_cpu(resp.media.remain);

	return CUPS_BACKEND_OK;
}

static const char *mitsud90_prefixes[] = {
	"mitsubishi-d90dw",
	// backwards compatibility
	"mitsud90",
	NULL
};

/* Exported */
struct dyesub_backend mitsud90_backend = {
	.name = "Mitsubishi CP-D90DW",
	.version = "0.13",
	.uri_prefixes = mitsud90_prefixes,
	.cmdline_arg = mitsud90_cmdline_arg,
	.cmdline_usage = mitsud90_cmdline,
	.init = mitsud90_init,
	.attach = mitsud90_attach,
	.cleanup_job = mitsud90_cleanup_job,
	.read_parse = mitsud90_read_parse,
	.main_loop = mitsud90_main_loop,
	.query_markers = mitsud90_query_markers,
	.devices = {
		{ USB_VID_MITSU, USB_PID_MITSU_D90, P_MITSU_D90, NULL, "mitsubishi-d90dw"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/*
   Mitsubishi CP-D90DW data format

   All multi-byte values are BIG endian

 [[HEADER 1]]

   1b 53 50 30 00 33 XX XX  YY YY 64 00 00 01 00 ??  XX XX == COLS, YY XX ROWS (BE)
   ?? ?? ?? ?? ?? ?? ?? ??  00 00 00 00 00 00 00 00  <-- cut position, see below
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   QQ RR SS HH VV 00 00 00  00 00 01 00 03 II 09 7c  QQ == 02 matte, 00 glossy,
   09 4c 00 00 02 58 00 0c  00 06                    RR == 00 auto, 03 == fine, 02 == superfine.
                                                     SS == 00 colorcorr, 01 == none
                                                     HH/VV sharpening for Horiz/Vert, 0-8, 0 is off, 4 is normal
  [pad to 512b]

                normal  == rows  00  00 00  00  00 00  00  00 00
                4x6div2 == 1226  00  02 65  01  00 00  01  00 00
                8x6div2 == 2488  01  04 be  00  00 00  00  00 00

		    guesses based on SDK docs:

		9x6div2 == 2728  01  05 36  00  00 00  00  00 00
		9x6div3 == 2724  00  03 90  00  07 14  00  00 00
		9x6div4 == 2628  00  02 97  00  05 22  00  07 ad

    from [01 00 03 03] onwards, only shows in 8x20" PANORAMA prints.  Assume 2" overlap.
    II == 01 02 03 (which panel # in panorama!)
    [02 58] == 600, aka 2" * 300dpi?
    [09 4c] == 2380  (48 less than 8 size? (trim length on ends?)
    [09 7c] == 2428  (ie 8" print)

     (6x20 == 1852x6036)
     (6x14 == 1852x4232)

     3*8" panels == 2428*3=7284.  -6036 = 1248.  /2 = 624 (0x270)

 [[DATA PLANE HEADER]]

   1b 5a 54 01 00 09 00 00  00 00 XX XX YY YY 00 00
   ...
   [pad to 512b]

    data, BGR packed, 8bpp.  No padding to 512b!

 [[FOOTER]]

   1b 42 51 31 00 TT                  ## TT == secs to wait for second print


 ****************************************************

Comms Protocol for D90:

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
<- e4 47 44 30 XX XX        Unknown, seems to increment.

 [[ JOB STATUS QUERY ?? ]]

-> 1b 47 44 31 00 00 JJ JJ  Jobid?
<- e4 47 44 31 XX YY ZZ ZZ  No idea.. sure.

 [[ COMBINED STATUS QUERIES ]]

-> 1b 47 44 30 00 00 04 16  17 1f 2a
<- e4 47 44 30

   MM NN 00 00 ZZ 00 00 00  00 QQ QQ   [id 16, total 11]
   SS SS                               [id 17, total 2]
   HH                                  [id 1f, total 1]
   VV TT WW 00 XX XX YY YY  01 00      [id 2a, total 10]

   WW    == 0x50 or 0x00 (seen, no idea what it means)
   VV    == Media vendor (0xff etc)
   TT    == Media type, 0x02/0x0f etc (see mitsu70x_media_types!)
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

 [[ GET iSERIAL ]]

-> 1b 61 36 36 41 be 00 00
   00 01 00 00 00 11 ff ff
   ff fe ff ff ff ee
<- e4 61 36 36 41 be 00 00
   00 01 00 00 00 11 ff ff
   ff fe ff ff ff ee XX      <- XX is 0x80 or 0x00.  (0x80)  ISERIAL OFF

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

 [[ SANITY CHECK PRINT ARGUMENTS / MEMTEST ]]

-> 1b 47 44 33 00 33 07 3c  04 ca 64 00 00 01 00 00
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   00 00 00 04 04 00 00 00  00 00 00 00 00 00 00 00
   [[ pad to 512 ]]

   ... 07 3c onwards is the same as main payload header.

<- e4 47 44 43 XX YY

   ... possibly the same as the D70's "memorystatus"
       XX == size ok (non-zero if bad size)
       YY == memory ok (non-zero or 0xff if full?)

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


 */
