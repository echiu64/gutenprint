/*
 *   Kodak 605 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2019 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND kodak605_backend

#include "backend_common.h"
#include "backend_sinfonia.h"

#define USB_VID_KODAK       0x040A
#define USB_PID_KODAK_605   0x402E
#define USB_PID_KODAK_7000  0x4035
#define USB_PID_KODAK_7010  0x4037
#define USB_PID_KODAK_7015  0x4038

/* Media structure */
struct kodak605_media_list {
	struct sinfonia_status_hdr hdr;
	uint8_t  unk;  /* always seen 02 */
	uint8_t  type; /* KODAK68x0_MEDIA_* */
	uint8_t  count;
	struct sinfonia_mediainfo_item entries[];
} __attribute__((packed));

#define MAX_MEDIA_LEN (sizeof(struct kodak605_media_list) + sizeof(struct sinfonia_mediainfo_item) * 10)

/* Status response */
struct kodak605_status {
	struct sinfonia_status_hdr hdr;
/*@10*/	uint32_t ctr_life;  /* Lifetime Prints */
	uint32_t ctr_maint; /* Prints since last maintenance */
	uint32_t ctr_media; /* Prints on current media */
	uint32_t ctr_cut;   /* Cutter Actuations */
	uint32_t ctr_head;  /* Prints on current head */
/*@30*/	uint8_t  donor;     /* Donor Percentage remaining */
/*@31*/	uint8_t  null_1[7]; /* 00 00 00 00 00 00 00 */
/*@38*/	uint8_t  b1_id;     /* jobid */
	uint16_t b1_remain;
	uint16_t b1_complete;
	uint16_t b1_total;
	uint8_t  b1_sts;    /* See BANK_STATUS_* */
/*@46*/	uint8_t  b2_id;     /* jobid */
	uint16_t b2_remain;
	uint16_t b2_complete;
	uint16_t b2_total;
	uint8_t  b2_sts;    /* see BANK_STATUS_* */
/*@54*/	uint8_t  b3_id;     /* jobid */
	uint16_t b3_remain;
	uint16_t b3_complete;
	uint16_t b3_total;
	uint8_t  b3_sts;    /* see BANK_STATUS_* */
/*@62*/	uint8_t  b4_id;     /* jobid */
	uint16_t b4_remain;
	uint16_t b4_complete;
	uint16_t b4_total;
	uint8_t  b4_sts;    /* see BANK_STATUS_* */
/*@70*/	uint8_t  unk[4];  /* EK605 has 01/00/00/00, EK7000 has 01/01/01/01 */
/*@74*/	uint8_t  null_2[2]; /* 00 00 */
/*@76*/	uint8_t  null_3[1]; /* EK7000 only */
} __attribute__((packed));

static const struct sinfonia_param ek7000_params[] =
{
	{ 0x01, "Unknown_01" }, // 00000001
	{ 0x11, "Unknown_11" }, // 00000001
	{ 0x12, "Matte Gloss" }, // 00000069
	{ 0x13, "Matte Degloss Black" }, // 000000c3
	{ 0x14, "Matte Degloss White" }, // 000000cd
	{ 0x21, "Exit Speed With Sorter 4x6" }, // 000003e8
	{ 0x22, "Exit Speed With Sorter 8x6" }, // 0000041a
	{ 0x23, "Exit Speed With Backprinting" }, // 00000152
	{ 0x24, "Exit Speed Without PPAC 4x6" }, // 0000044c
	{ 0x25, "Exit Speed Without PPAC 8x6" }, // 0000044c

	{ 0x2f, "Unknown_2f" }, // 00000320
	{ 0x41, "Unknown_41" }, // 0000006d
	{ 0x42, "Unknown_42" }, // 00000051
	{ 0x43, "Unknown_43" }, // 0000003b
	{ 0x44, "Unknown_44" }, // 00000082
	{ 0x45, "Unknown_45" }, // 00000000
	{ 0x46, "Unknown_46" }, // 00000000
	{ 0x47, "Unknown_47" }, // 00000028
	{ 0x48, "Unknown_48" }, // 00000002
	{ 0x81, "Unknown_81" }, // ffffffff

	{ 0x82, "Unknown_82" }, // fffffffe
	{ 0x83, "Unknown_83" }, // ffffffee
	{ 0x84, "Unknown_84" }, // 00000001
	{ 0x91, "Unknown_91" }, // 0000006c
	{ 0x92, "Unknown_92" }, // 00000077
	{ 0x93, "Unknown_93" }, // 00000067
	{ 0x94, "Unknown_94" }, // 00000076
	{ 0xa0, "Unknown_a0" }, // 00000005
	{ 0xa1, "Unknown_a1" }, // 00000000
	{ 0xa2, "Unknown_a2" }, // 00000010

	{ 0xa3, "Unknown_a3" }, // 0000003b
	{ 0xa4, "Unknown_a4" }, // 0000003b
	{ 0xa5, "Thermal Protect Lamination" }, // 0000003e
	{ 0xa6, "Unknown_a6" }, // 00000001
	{ 0xa7, "Unknown_a7" }, // 00000014
	{ 0xa8, "Unknown_a8" }, // 00000001
	{ 0xa9, "Unknown_a9" }, // ffffffff
	{ 0xc1, "Unknown_c1" }, // 00000002
	{ 0xc2, "Unknown_c2" }, // 000000c8
	{ 0xc3, "Unknown_c3" }, // 000000c8

	{ 0xc4, "Unknown_c4" }, // 00000200
	{ 0xf1, "Unknown_f1" }, // 00000068
	{ 0xf2, "Unknown_f2" }, // 00000068
	{ 0xf3, "Unknown_f3" }, // 00000094
	{ 0xf4, "Unknown_f4" }, // 00000068
};
#define ek7000_params_num (sizeof(ek7000_params) / sizeof(struct sinfonia_param))

/* Private data structure */
struct kodak605_ctx {
	struct sinfonia_usbdev dev;

	uint8_t jobid;

	struct kodak605_media_list *media;

	struct marker marker;
};

/* Note this is for Kodak 7000-series only! */
static const char *error_codes(uint8_t major, uint8_t minor)
{
	switch(major) {
	case 0x01: /* "Controller Error" */
		switch(minor) {
		case 0x01:
			return "Controller: EEPROM Write Timeout";
		case 0x02:
			return "Controller: EEPROM Verify";
		case 0x09:
			return "Controller: DSP FW Boot";
		case 0x0A:
			return "Controller: Invalid Print Parameter Table";
		case 0x0B:
			return "Controller: DSP FW Mismatch";
		case 0x0C:
			return "Controller: Print Parameter Table Mismatch";
		case 0x0D:
			return "Controller: ASIC Error";
		case 0x0E:
			return "Controller: FPGA Error";
		case 0x0F:
			return "Controller: Main FW Checksum";
		case 0x10:
			return "Controller: Main FW Write Failed";
		case 0x11:
			return "Controller: DSP Checksum";
		case 0x12:
			return "Controller: DSP FW Write Failed";
		case 0x13:
			return "Controller: Print Parameter Table Checksum";
		case 0x14:
			return "Controller: Print Parameter Table Write Failed";
		case 0x15:
			return "Controller: User Tone Curve Write Failed";
		case 0x16:
			return "Controller: Main-DSP Communication";
		case 0x17:
			return "Controller: DSP DMA Failed";
		case 0x18:
			return "Controller: Matte Pattern Write Failed";
		case 0x19:
			return "Controller: Matte not Initialized";
		case 0x20:
			return "Controller: Serial Number Error";
		default:
			return "Controller: Unknown";
		}
	case 0x02: /* "Mechanical Error" */
		switch (minor) {
		case 0x01:
			return "Mechanical: Pinch Head Up";
		case 0x02:
			return "Mechanical: Pinch Head Down";
		case 0x03:
			return "Mechanical: Pinch Roll Main Up Feed Up";
		case 0x04:
			return "Mechanical: Pinch Roll Main Dn Feed Up";
		case 0x05:
			return "Mechanical: Pinch Roll Main Dn Feed Dn";
		case 0x06:
			return "Mechanical: Pinch Roll Eject Up";
		case 0x07:
			return "Mechanical: Pinch Roll Eject Dn";
		case 0x0B:
			return "Mechanical: Cutter (Left->Right)";
		case 0x0C:
			return "Mechanical: Cutter (Right->Left)";
		default:
			return "Mechanical: Unknown";
		}
	case 0x03: /* "Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Sensor: Head Up/Dn All On";
		case 0x02:
			return "Sensor: Feed Pitch Roll Up/Dn All On";
		case 0x05:
			return "Sensor: Cutter L/R All On";
		case 0x06:
			return "Sensor: Cutter L Stuck On";
		case 0x07:
			return "Sensor: Cutter Move not Detected";
		case 0x08:
			return "Sensor: Cutter R Stuck On";
		case 0x09:
			return "Sensor: Head Up Unstable";
		case 0x0A:
			return "Sensor: Head Dn Unstable";
		case 0x0B:
			return "Sensor: Main/Feed Pinch Up Unstabe";
		case 0x0C:
			return "Sensor: Main/Feed Pinch Dn Unstable";
		case 0x0D:
			return "Sensor: Eject Up Unstable";
		case 0x0E:
			return "Sensor: Eject Dn Unstable";
		case 0x0F:
			return "Sensor: Left Cutter Unstable";
		case 0x10:
			return "Sensor: Right Cutter Unstable";
		case 0x11:
			return "Sensor: Center Cutter Unstable";
		case 0x12:
			return "Sensor: Upper Cover Unstable";
		case 0x13:
			return "Sensor: Paper Cover Unstable";
		case 0x14:
			return "Sensor: Ribbon Takeup Unstable";
		case 0x15:
			return "Sensor: Ribbon Supply Unstable";
		default:
			return "Sensor: Unknown";
		}
	case 0x04: /* "Temperature Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Temp Sensor: Thermal Head High";
		case 0x02:
			return "Temp Sensor: Thermal Head Low";
		case 0x05:
			return "Temp Sensor: Environment High";
		case 0x09:
			return "Temp Sensor: Environment Low";
		case 0x0A:
			return "Temp Sensor: Preheat";
		default:
			return "Temp Sensor: Unknown";
		}
	case 0x5: /* "Paper Jam" */
		switch (minor) {
		// XXX these have been seen on EK7000:
		// case 0x0c:
		// case 0x36:
		case 0x3D:
			return "Paper Jam: Feed Cut->Home";
		case 0x3E:
			return "Paper Jam: Feed Cut->Exit Stuck Off";
		case 0x3F:
			return "Paper Jam: Feed Cut->Exit Stuck On";
		case 0x4A:
			return "Paper Jam: Idle / Paper Set";
		case 0x51:
			return "Paper Jam: Paper Exit On";
		case 0x52:
			return "Paper Jam: Print Position On";
		case 0x53:
			return "Paper Jam: Paper Empty On";
		case 0x54:
			return "Paper Jam: Idle / Paper Not Set";
		default:
			return "Paper Jam: Unknown";
		}
	case 0x06: /* User Error */
		switch (minor) {
		case 0x01:
			return "Drawer Unit Open";
		case 0x02:
			return "Incorrect Ribbon";
		case 0x03:
			return "No/Empty Ribbon";
		case 0x04:
			return "Mismatched Ribbon";
		case 0x08:
			return "No Paper";
		case 0x0C:
			return "Paper End";
		default:
			return "User: Unknown";
		}
	default:
		return "Unknown";
	}
}

static int kodak605_get_status(struct kodak605_ctx *ctx, struct kodak605_status *sts)
{
	struct sinfonia_cmd_hdr cmd;

	int ret, num = 0;

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSTATUS);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)sts, sizeof(*sts), &num))) {
		return ret;
	}

	return CUPS_BACKEND_OK;
}

static void *kodak605_init(void)
{
	struct kodak605_ctx *ctx = malloc(sizeof(struct kodak605_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct kodak605_ctx));

	ctx->media = malloc(MAX_MEDIA_LEN);

	return ctx;
}

static int kodak605_attach(void *vctx, struct libusb_device_handle *dev, int type,
			   uint8_t endp_up, uint8_t endp_down, int iface, uint8_t jobid)
{
	struct kodak605_ctx *ctx = vctx;

	ctx->dev.dev = dev;
	ctx->dev.endp_up = endp_up;
	ctx->dev.endp_down = endp_down;
	ctx->dev.type = type;
	ctx->dev.iface = iface;
	ctx->dev.error_codes = &error_codes;

	if (ctx->dev.type != P_KODAK_605) {
		ctx->dev.params = ek7000_params;
		ctx->dev.params_count = ek7000_params_num;
	}

	/* Make sure jobid is sane */
	ctx->jobid = jobid & 0x7f;
	if (!ctx->jobid)
		ctx->jobid++;

	if (test_mode < TEST_MODE_NOATTACH) {
		/* Query media info */
		int ret = sinfonia_query_media(&ctx->dev,
					       ctx->media);
		if (ret)
			return ret;
	} else {
		int media_code = KODAK6_MEDIA_6TR2;
		if (getenv("MEDIA_CODE"))
			media_code = atoi(getenv("MEDIA_CODE"));

		ctx->media->type = media_code;
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = kodak6_mediatypes(ctx->media->type);
	ctx->marker.numtype = ctx->media->type;
	ctx->marker.levelmax = 100; /* Ie percentage */
	ctx->marker.levelnow = -2;

	return CUPS_BACKEND_OK;
}

static int kodak605_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct kodak605_ctx *ctx = vctx;
	int ret;

	struct sinfonia_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_CANCEL;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));

	/* Read in header */
	ret = sinfonia_raw10_read_parse(data_fd, job);
	if (ret) {
		free(job);
		return ret;
	}

	/* Printer handles generating copies.  Use larger of our options */
	if (le16_to_cpu(job->jp.copies) < (uint16_t)copies)
		job->jp.copies = cpu_to_le16(copies);

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int kodak605_main_loop(void *vctx, const void *vjob) {
	struct kodak605_ctx *ctx = vctx;

	struct kodak605_status sts;

	int num, ret;
	int offset = 0;

	const struct sinfonia_printjob *job = vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	/* Validate against supported media list */
	for (num = 0 ; num < ctx->media->count; num++) {
		if (ctx->media->entries[num].rows == job->jp.rows &&
		    ctx->media->entries[num].columns == job->jp.columns)
			break;
	}
	if (num == ctx->media->count) {
		ERROR("Print size unsupported by media!\n");
		return CUPS_BACKEND_HOLD;
	}

	/* > 4x6 jobs need two banks */
	int banks_needed;
	if (job->jp.rows > 1240)
		banks_needed = 2;
	else
		banks_needed = 1;

	INFO("Waiting for printer idle (%d banks needed)\n", banks_needed);

	while(1) {
		if ((ret = kodak605_get_status(ctx, &sts)))
			return CUPS_BACKEND_FAILED;

		if (ctx->marker.levelnow != sts.donor) {
			ctx->marker.levelnow = sts.donor;
			dump_markers(&ctx->marker, 1, 0);
		}

		if (sts.hdr.result != RESULT_SUCCESS) {
			ERROR("Printer Status:  %02x (%s)\n", sts.hdr.status,
			      sinfonia_status_str(sts.hdr.status));
			ERROR("Result: %02x Error: %02x (%s) %02x/%02x = %s\n",
			      sts.hdr.result, sts.hdr.error,
			      sinfonia_error_str(sts.hdr.error),
			      sts.hdr.printer_major, sts.hdr.printer_minor,
			      error_codes(sts.hdr.printer_major, sts.hdr.printer_minor));
			return CUPS_BACKEND_FAILED;
		}

		/* Make sure we're not colliding with an existing
		   jobid */
		while (ctx->jobid == sts.b1_id ||
		       ctx->jobid == sts.b2_id ||
		       ctx->jobid == sts.b3_id ||
		       ctx->jobid == sts.b4_id) {
			ctx->jobid++;
			ctx->jobid &= 0x7f;
			if (!ctx->jobid)
				ctx->jobid++;
		}

		int banks_free = 0;
		if (sts.b1_sts == BANK_STATUS_FREE)
			banks_free++;
		if (sts.b2_sts == BANK_STATUS_FREE)
			banks_free++;
		if (sts.b3_sts == BANK_STATUS_FREE)
			banks_free++;
		if (sts.b4_sts == BANK_STATUS_FREE)
			banks_free++;

		/* Do we have enough free buffers? */
		if (banks_free >= banks_needed) {
			break;
		}

		sleep(1);
	}

	/* Send backprint */
	if (job->jp.ext_flags & EXT_FLAG_BACKPRINT && offset == 0) {
		struct kodak701x_backprint bp;
		INFO("Sending backprint text..\n");
		bp.hdr.cmd = cpu_to_le16(SINFONIA_CMD_BACKPRINT);
		bp.hdr.len = cpu_to_le16(sizeof(bp) - sizeof(bp.hdr));

		/* Line 1 */
		bp.unk_0 = job->databuf[offset + 0];
		memset(bp.null, 0, sizeof(bp.null));
		bp.unk_1 = job->databuf[offset + 1];
		memcpy(bp.text, &job->databuf[offset + 2], sizeof(bp.text));

		if ((ret = sinfonia_docmd(&ctx->dev,
					  (uint8_t*)&bp, sizeof(bp),
					  (uint8_t*)&sts.hdr, sizeof(sts.hdr),
					  &num))) {
			return ret;
		}
		offset += 44;

		/* Line 2 */
		bp.unk_0 = job->databuf[offset + 0];
		memset(bp.null, 0, sizeof(bp.null));
		bp.unk_1 = job->databuf[offset + 1];
		memcpy(bp.text, &job->databuf[offset + 2], sizeof(bp.text));

		if ((ret = sinfonia_docmd(&ctx->dev,
					  (uint8_t*)&bp, sizeof(bp),
					  (uint8_t*)&sts.hdr, sizeof(sts.hdr),
					  &num))) {
			return ret;
		}
		offset += 44;
		// XXX sanity check backprint parameters..
	}

	/* Send print job */
	struct sinfonia_printcmd10_hdr hdr;

	INFO("Sending print job (internal id %u)\n", ctx->jobid);

	/* Set up header */
	hdr.hdr.cmd = cpu_to_le16(SINFONIA_CMD_PRINTJOB);
	hdr.hdr.len = cpu_to_le16(10);
	hdr.jobid = ctx->jobid;
	hdr.rows = cpu_to_le16(job->jp.rows);
	hdr.columns = cpu_to_le16(job->jp.columns);
	hdr.copies = cpu_to_le16(job->jp.copies);
	hdr.media = job->jp.media;
	hdr.oc_mode = job->jp.oc_mode;
	hdr.method = job->jp.method;

retry_print:
	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&hdr, sizeof(hdr),
				  (uint8_t*)&sts.hdr, sizeof(sts.hdr),
				  &num))) {
		return ret;
	}

	if (sts.hdr.result != RESULT_SUCCESS) {
		if (sts.hdr.error == ERROR_BUFFER_FULL) {
			INFO("Printer Buffers full, retrying\n");
			sleep(1);
			goto retry_print;
		} else if ((sts.hdr.status & 0xf0) == 0x30 || sts.hdr.status == ERROR_BUFFER_FULL) {
			INFO("Printer busy (%02x : %s), retrying\n", sts.hdr.status, sinfonia_status_str(sts.hdr.status));

		} else {
			ERROR("Unexpected response from print command!\n");
			ERROR("Printer Status:  %02x: %s\n", sts.hdr.status, sinfonia_status_str(sts.hdr.status));
			ERROR("Result: %02x Error: %02x (%02x %02x = %s)\n",
			      sts.hdr.result, sts.hdr.error,
			      sts.hdr.printer_major, sts.hdr.printer_minor,
			      error_codes(sts.hdr.printer_major, sts.hdr.printer_minor));

			return CUPS_BACKEND_FAILED;
		}
	}

	INFO("Sending image data\n");
	if ((ret = send_data(ctx->dev.dev, ctx->dev.endp_down,
			     job->databuf + offset, job->datalen - offset)))
		return CUPS_BACKEND_FAILED;

	INFO("Waiting for printer to acknowledge completion\n");
	do {
		sleep(1);
		if ((kodak605_get_status(ctx, &sts)) != 0)
			return CUPS_BACKEND_FAILED;

		if (ctx->marker.levelnow != sts.donor) {
			ctx->marker.levelnow = sts.donor;
			dump_markers(&ctx->marker, 1, 0);
		}

		INFO("Printer Status:  %02x (%s)\n", sts.hdr.status,
		     sinfonia_status_str(sts.hdr.status));

		if (sts.hdr.result != RESULT_SUCCESS ||
		    sts.hdr.error == ERROR_PRINTER) {
			INFO("Result: %02x Error: %02x (%s) %02x/%02x = %s\n",
			     sts.hdr.result, sts.hdr.error,
			     sinfonia_error_str(sts.hdr.error),
			     sts.hdr.printer_major, sts.hdr.printer_minor,
			     error_codes(sts.hdr.printer_major, sts.hdr.printer_minor));
			return CUPS_BACKEND_STOP;
		}

		/* Wait for completion */
		if (sts.b1_id == ctx->jobid && sts.b1_complete == sts.b1_total)
			break;
		if (sts.b2_id == ctx->jobid && sts.b2_complete == sts.b2_total)
			break;
		if (sts.b3_id == ctx->jobid && sts.b3_complete == sts.b3_total)
			break;
		if (sts.b4_id == ctx->jobid && sts.b4_complete == sts.b4_total)
			break;

		if (sts.hdr.status == STATUS_READY)
			break;

		if (fast_return) {
			INFO("Fast return mode enabled.\n");
			break;
		}
	} while(1);

	INFO("Print complete\n");
	return CUPS_BACKEND_OK;
}

static void kodak605_dump_status(struct kodak605_ctx *ctx, struct kodak605_status *sts)
{
	INFO("Status: %02x (%s)\n",
	     sts->hdr.status, sinfonia_status_str(sts->hdr.status));
	INFO("Error: %02x (%s) %02x/%02x = %s\n",
	     sts->hdr.error, sinfonia_error_str(sts->hdr.error),
	     sts->hdr.printer_major, sts->hdr.printer_minor,
	     error_codes(sts->hdr.printer_major, sts->hdr.printer_minor));

	INFO("Bank 1: %s - Job %03u @ %03u/%03u\n",
	     sinfonia_bank_statuses(sts->b1_sts), sts->b1_id,
	     le16_to_cpu(sts->b1_complete), le16_to_cpu(sts->b1_total));
	INFO("Bank 2: %s - Job %03u @ %03u/%03u\n",
	     sinfonia_bank_statuses(sts->b2_sts), sts->b2_id,
	     le16_to_cpu(sts->b2_complete), le16_to_cpu(sts->b2_total));
	INFO("Bank 3: %s - Job %03u @ %03u/%03u\n",
	     sinfonia_bank_statuses(sts->b3_sts), sts->b3_id,
	     le16_to_cpu(sts->b3_complete), le16_to_cpu(sts->b3_total));
	INFO("Bank 4: %s - Job %03u @ %03u/%03u\n",
	     sinfonia_bank_statuses(sts->b4_sts), sts->b4_id,
	     le16_to_cpu(sts->b4_complete), le16_to_cpu(sts->b4_total));

	INFO("Lifetime prints   : %u\n", le32_to_cpu(sts->ctr_life));
	INFO("Cutter actuations : %u\n", le32_to_cpu(sts->ctr_cut));
	INFO("Head prints       : %u\n", le32_to_cpu(sts->ctr_head));
	INFO("Media prints      : %u\n", le32_to_cpu(sts->ctr_media));
	{
		int max = kodak6_mediamax(ctx->media->type);

		if (max) {
			INFO("\t  Remaining     : %u\n", max - le32_to_cpu(sts->ctr_media));
		} else {
			INFO("\t  Remaining     : Unknown\n");
		}
	}

	INFO("Donor             : %u%%\n", sts->donor);
}

static void kodak605_dump_mediainfo(struct kodak605_media_list *media)
{
	int i;

        if (media->type == KODAK6_MEDIA_NONE) {
                DEBUG("No Media Loaded\n");
                return;
        }
	kodak6_dumpmediacommon(media->type);

	DEBUG("Legal print sizes:\n");
	for (i = 0 ; i < media->count ; i++) {
		DEBUG("\t%d: %ux%u (%x)\n", i,
		      media->entries[i].columns,
		      media->entries[i].rows,
		      media->entries[i].code);
	}
	DEBUG("\n");
}

static void kodak605_cmdline(void)
{
	DEBUG("\t\t[ -b 0|1 ]       # Disable/Enable control panel\n");
	DEBUG("\t\t[ -c filename ]  # Get user/NV tone curve\n");
	DEBUG("\t\t[ -C filename ]  # Set tone curve\n");
	DEBUG("\t\t[ -e ]           # Query error log\n");
	DEBUG("\t\t[ -F ]           # Flash Printer LED\n");
	DEBUG("\t\t[ -i ]           # Query printer info\n");
	DEBUG("\t\t[ -l filename ]  # Get current tone curve\n");
	DEBUG("\t\t[ -L filename ]  # Set current tone curve\n");
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -r ]           # Reset user/NV tone curve\n");
	DEBUG("\t\t[ -R ]           # Reset printer to factory defaults\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -X jobid ]     # Cancel job\n");
//	DEBUG("\t\t[ -Z 0|1 ]       # Dump all parameters\n");
}

static int kodak605_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct kodak605_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "b:c:C:eFil:L:mrRsX:Z:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'b':
			if (optarg[0] == '1')
				j = sinfonia_button_set(&ctx->dev, BUTTON_ENABLED);
			else if (optarg[0] == '0')
				j = sinfonia_button_set(&ctx->dev, BUTTON_DISABLED);
			else
				return -1;
			break;
		case 'c':
			j = sinfonia_gettonecurve(&ctx->dev, TONECURVE_USER, optarg);
			break;
		case 'C':
			j = sinfonia_settonecurve(&ctx->dev, UPDATE_TARGET_TONE_USER, optarg);
			break;
		case 'e':
			j = sinfonia_geterrorlog(&ctx->dev);
			break;
		case 'F':
			j = sinfonia_flashled(&ctx->dev);
			break;
		case 'i':
			j = sinfonia_getfwinfo(&ctx->dev);
			break;
		case 'l':
			j = sinfonia_gettonecurve(&ctx->dev, TONECURVE_CURRENT, optarg);
			break;
		case 'L':
			j = sinfonia_settonecurve(&ctx->dev, UPDATE_TARGET_TONE_CURRENT, optarg);
			break;
		case 'm':
			kodak605_dump_mediainfo(ctx->media);
			break;
		case 'r':
			j = sinfonia_resetcurve(&ctx->dev, RESET_TONE_CURVE, TONE_CURVE_ID);
			break;
		case 'R':
			j = sinfonia_resetcurve(&ctx->dev, RESET_PRINTER, 0);
			break;
		case 's': {
			struct kodak605_status sts;

			j = kodak605_get_status(ctx, &sts);
			if (!j)
				kodak605_dump_status(ctx, &sts);
			break;
		}
		case 'X':
			j = sinfonia_canceljob(&ctx->dev, atoi(optarg));
			break;
		case 'Z':
			j = sinfonia_dumpallparams(&ctx->dev, atoi(optarg));
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return CUPS_BACKEND_OK;
}

static int kodak605_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct kodak605_ctx *ctx = vctx;
	struct kodak605_status sts;

	/* Query printer status */
	if (kodak605_get_status(ctx, &sts))
		return CUPS_BACKEND_FAILED;

	ctx->marker.levelnow = sts.donor;

	*markers = &ctx->marker;
	*count = 1;

	return CUPS_BACKEND_OK;
}

static const char *kodak605_prefixes[] = {
	"kodak605",  // Family driver, do NOT nuke.
	"kodak-605", "kodak-7000", "kodak-7010", "kodak-7015",
	NULL,
};

/* Exported */
struct dyesub_backend kodak605_backend = {
	.name = "Kodak 605/70xx",
	.version = "0.52" " (lib " LIBSINFONIA_VER ")",
	.uri_prefixes = kodak605_prefixes,
	.cmdline_usage = kodak605_cmdline,
	.cmdline_arg = kodak605_cmdline_arg,
	.init = kodak605_init,
	.attach = kodak605_attach,
	.cleanup_job = sinfonia_cleanup_job,
	.read_parse = kodak605_read_parse,
	.main_loop = kodak605_main_loop,
	.query_markers = kodak605_query_markers,
	.devices = {
		{ USB_VID_KODAK, USB_PID_KODAK_605, P_KODAK_605, "Kodak", "kodak-605"},
		{ USB_VID_KODAK, USB_PID_KODAK_7000, P_KODAK_7000, "Kodak", "kodak-7000"},
		{ USB_VID_KODAK, USB_PID_KODAK_7010, P_KODAK_701X, "Kodak", "kodak-7010"},
		{ USB_VID_KODAK, USB_PID_KODAK_7015, P_KODAK_701X, "Kodak", "kodak-7015"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* Kodak 605/70xx data format

  Spool file consists of 14-byte header followed by plane-interleaved BGR data.
  Native printer resolution is 1844 pixels per row on all models but 7015,
  which is 1548 pixels per row.

  All fields are LITTLE ENDIAN unless otherwise specified

  Header:

  01 40 0a 00                    Fixed header
  XX                             Job ID
  CC CC                          Number of copies (1-???)
  WW WW                          Number of columns (Fixed at 1844 or 1548)
  HH HH                          Number of rows
  DD                             0x01 (4x6) 0x03 (8x6)
  LL                             Laminate, 0x01/0x02/0x03 (off/on/satin[70xx only])
  00                             Print Mode (???)

  ************************************************************************

  Note:  Kodak 605  is actually a Shinko CHC-S1545-5A
  Note:  Kodak 7000 is actually a Shinko CHC-S1645-5A
  Note:  Kodak 7010 is actually a Shinko CHC-S1645-5B
  Note:  Kodak 7015 is actually a Shinko CHC-S1645-5C

  ************************************************************************

*/
