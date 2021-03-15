/*
 *   Shinko/Sinfonia CHC-S6245 CUPS backend -- libusb-1.0 version
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *          [http://www.gnu.org/licenses/gpl-2.0.html]
 *
 *   SPDX-License-Identifier: GPL-2.0+
 *
 */

#define BACKEND shinkos6245_backend

#include "backend_common.h"
#include "backend_sinfonia.h"

#include <time.h>

enum {
	S_IDLE = 0,
	S_PRINTER_READY_CMD,
	S_PRINTER_SENT_DATA,
	S_FINISHED,
};

/* Structs for printer */
struct s6245_print_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  id;
	uint16_t count;
	uint16_t columns;
	uint16_t rows;
	uint16_t columns2; /* These are necessary for EK8810 */
	uint16_t rows2;    /*                                */
	uint8_t  reserved[4];
	uint8_t  mode;
	uint8_t  method;
	uint8_t  reserved2;
} __attribute__((packed));

static const struct sinfonia_param ek8810_params[] =
{
	{ 0x01, "Unknown_01" }, // 00000001
	{ 0x11, "Unknown_11" }, // 00000001
	{ 0x12, "? Matte Gloss" }, // 00000069
	{ 0x13, "? Matte Degloss Black" }, // 000000c3
	{ 0x14, "? Matte Degloss White" }, // 000000cd
	{ 0x21, "Unknown_21" }, // 000003e8
	{ 0x22, "Unknown_22" }, // 0000041a
	{ 0x23, "Unknown_23" }, // 00000152
	{ 0x24, "Unknown_24" }, // 0000044c
	{ 0x25, "Unknown_25" }, // 0000044c

	{ 0x2f, "Unknown_2f" }, // 00000320
	{ 0x41, "Unknown_41" }, // 0000005d
	{ 0x42, "Unknown_42" }, // 00000048
	{ 0x43, "Unknown_43" }, // 0000007c
	{ 0x44, "Unknown_44" }, // 00000088
	{ 0x45, "Unknown_45" }, // 00000000
	{ 0x46, "Unknown_46" }, // 00000002
	{ 0x47, "Unknown_47" }, // 00000063
	{ 0x48, "Unknown_48" }, // 00000008
	{ 0x61, "Unknown_61" }, // 00000050

	{ 0x62, "Unknown_62" }, // 00000031
	{ 0x63, "Unknown_63" }, // 00000030
	{ 0x64, "Unknown_64" }, // 00000030
	{ 0x81, "Unknown_81" }, // ffffffff
	{ 0x82, "Unknown_82" }, // fffffff9
	{ 0x83, "Unknown_83" }, // fffffffc
	{ 0x84, "Unknown_84" }, // 00000002
	{ 0x8a, "Unknown_8a" }, // 00000005
	{ 0x8b, "Unknown_8b" }, // 00000005
	{ 0x8c, "Unknown_8c" }, // 00000000

	{ 0x8d, "Unknown_8d" }, // 00000000
	{ 0x91, "Unknown_91" }, // 0000007e
	{ 0x92, "Unknown_92" }, // 0000007d
	{ 0x93, "Unknown_93" }, // 00000077
	{ 0xa0, "Unknown_a0" }, // 00000005
	{ 0xa1, "Unknown_a1" }, // 00000000
	{ 0xa2, "Unknown_a2" }, // 00000008
	{ 0xa3, "Unknown_a3" }, // 00000030
	{ 0xa4, "Unknown_a4" }, // 00000030
	{ 0xa5, "? Thermal Protect Lamination" }, // 00000046

	{ 0xa6, "Unknown_a6" }, // 00000001
	{ 0xa7, "Unknown_a7" }, // 00000014
	{ 0xa8, "Unknown_a8" }, // 00000001
	{ 0xa9, "Unknown_a9" }, // ffffffff
	{ 0xc1, "Unknown_c1" }, // 00000002
	{ 0xc2, "Unknown_c2" }, // 000000c8
	{ 0xc3, "Unknown_c3" }, // 000000c8
	{ 0xc4, "Unknown_c4" }, // 000004d0
	{ 0xf1, "Unknown_f1" }, // 00000022
	{ 0xf2, "Unknown_f2" }, // 00000022

	{ 0xf3, "Unknown_f3" }, // 00000047
	{ 0xf4, "Unknown_f4" }, // 00000022
};
#define ek8810_params_num (sizeof(ek8810_params) / sizeof(struct sinfonia_param))

#define PARAM_DRIVER_MODE  0x3e
#define PARAM_PAPER_MODE   0x3f
#define PARAM_SLEEP_TIME   0x54

static const struct sinfonia_param s6245_params[] =
{
	{ PARAM_DRIVER_MODE, "Driver Mode/Wizard" },
	{ PARAM_PAPER_MODE,  "Paper Load Mode" },
	{ PARAM_SLEEP_TIME,  "Sleep Time" },
};
#define s6245_params_num (sizeof(s6245_params) / sizeof(struct sinfonia_param))

#define PARAM_DRIVER_WIZOFF 0x00000000
#define PARAM_DRIVER_WIZON  0x00000001

#define PARAM_PAPER_NOCUT   0x00000000
#define PARAM_PAPER_CUTLOAD 0x00000001

#define PARAM_SLEEP_5MIN    0x00000000
#define PARAM_SLEEP_15MIN   0x00000001
#define PARAM_SLEEP_30MIN   0x00000002
#define PARAM_SLEEP_60MIN   0x00000003
#define PARAM_SLEEP_120MIN  0x00000004
#define PARAM_SLEEP_240MIN  0x00000005

static const char *s6245_error_codes(uint8_t major, uint8_t minor)
{
	switch(major) {
	case 0x01: /* "Controller Error" */
		switch(minor) {
		case 0x01:
			return "Controller: EEPROM Write Timeout";
		case 0x09:
			return "Controller: DSP FW Boot";
		case 0x0A:
			return "Controller: Invalid Print Parameter Table";
		case 0x0B:
			return "Controller: DSP FW Mismatch";
		case 0x0C:
			return "Controller: Print Parameter Table Mismatch";
		case 0x0D:
			return "Controller: FPGA Configuration Failed";
		case 0x0F:
			return "Controller: Main FW Checksum";
		case 0x10:
			return "Controller: Flash Write Failed";
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
			return "Controller: MSP Communication";
		case 0x17:
			return "Controller: THV Autotuning";
		case 0x18:
			return "Controller: THV Value Out of Range";
		case 0x19:
			return "Controller: Thermal Head";
		case 0x1B:
			return "Controller: DSP Communication";
		case 0x1C:
			return "Controller: DSP DMA Failed";
		default:
			return "Controller: Unknown";
		}
	case 0x02: /* "Mechanical Error" */
		switch (minor) {
		case 0x01:
			return "Mechanical: Pinch Head Home";
		case 0x02:
			return "Mechanical: Pinch Head (position 1)";
		case 0x03:
			return "Mechanical: Pinch Head (position 2)";
		case 0x04:
			return "Mechanical: Pinch Head (position 3)";
		case 0x0B:
			return "Mechanical: Cutter (Right)";
		case 0x0C:
			return "Mechanical: Cutter (Left)";
		default:
			return "Mechanical: Unknown";
		}
	case 0x03: /* "Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Sensor: Head Up";
		case 0x02:
			return "Sensor: Head Down";
		case 0x0B:
			return "Sensor: Cutter Left";
		case 0x0C:
			return "Sensor: Cutter Right";
		case 0x0D:
			return "Sensor: Cutter Left+Right";
		case 0x15:
			return "Sensor: Head Up Unstable";
		case 0x16:
			return "Sensor: Head Down Unstable";
		case 0x17:
			return "Sensor: Cutter Left Unstable";
		case 0x18:
			return "Sensor: Cutter Right Unstable";
		case 0x19:
			return "Sensor: Cover Open Unstable";
		case 0x1E:
			return "Sensor: Ribbon Mark (Cyan)";
		case 0x1F:
			return "Sensor: Ribbon Mark (OC)";
		default:
			return "Sensor: Unknown";
		}
	case 0x04: /* "Temperature Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Temp Sensor: Thermal Head Low";
		case 0x02:
			return "Temp Sensor: Thermal Head High";
		case 0x05:
			return "Temp Sensor: Environment Low";
		case 0x06:
			return "Temp Sensor: Environment High";
		case 0x07:
			return "Temp Sensor: Preheat";
		case 0x08:
			return "Temp Sensor: Thermal Protect";
		default:
			return "Temp Sensor: Unknown";
		}
	case 0x5: /* "Paper Jam" */
		switch (minor) {
		case 0x01:
			return "Paper Jam: Loading Paper Top On";
		case 0x02:
			return "Paper Jam: Loading Print Position On";
		case 0x03:
			return "Paper Jam: Loading Print Position Off";
		case 0x04:
			return "Paper Jam: Loading Paper Top Off";
		case 0x05:
			return "Paper Jam: Loading Cut Print Position Off";
		case 0x0C:
			return "Paper Jam: Initializing Print Position Off";
		case 0x0D:
			return "Paper Jam: Initializing Print Position On";
		case 0x15:
			return "Paper Jam: Printing Print Position Off";
		case 0x16:
			return "Paper Jam: Printing Paper Top On";
		case 0x17:
			return "Paper Jam: Printing Paper Top Off";
		case 0x1F:
			return "Paper Jam: Precut Print Position Off";
		case 0x20:
			return "Paper Jam: Precut Print Position On";

		case 0x29:
			return "Paper Jam: Printing Paper Top On";
		case 0x2A:
			return "Paper Jam: Printing Pre-Yellow Print Position Off";
		case 0x2B:
			return "Paper Jam: Printing Yellow Print Position Off";
		case 0x2C:
			return "Paper Jam: Printing Yellow Print Position On";
		case 0x2D:
			return "Paper Jam: Printing Pre-Magenta Print Position Off";
		case 0x2E:
			return "Paper Jam: Printing Magenta Print Position On";
		case 0x2F:
			return "Paper Jam: Printing Magenta Print Position Off";
		case 0x30:
			return "Paper Jam: Printing Pre-Cyan Print Position Off";
		case 0x31:
			return "Paper Jam: Printing Cyan Print Position On";
		case 0x32:
			return "Paper Jam: Printing Cyan Print Position Off";
		case 0x33:
			return "Paper Jam: Printing Pre-OC Print Position Off";
		case 0x34:
			return "Paper Jam: Printing OC Print Position On";
		case 0x35:
			return "Paper Jam: Printing OC Print Position Off";
		case 0x36:
			return "Paper Jam: Cut Print Position Off";
		case 0x37:
			return "Paper Jam: Home Position Off";
		case 0x38:
			return "Paper Jam: Paper Top Off";
		case 0x39:
			return "Paper Jam: Print Position On";

		case 0x51:
			return "Paper Jam: Paper Empty On, Top On, Position On";
		case 0x52:
			return "Paper Jam: Paper Empty On, Top On, Position Off";
		case 0x53:
			return "Paper Jam: Paper Empty On, Top Off, Print Position On";
		case 0x54:
			return "Paper Jam: Paper Empty On, Top Of, Position Off";
		case 0x55:
			return "Paper Jam: Paper Empty Off, Top On, Position On";
		case 0x56:
			return "Paper Jam: Paper Empty Off, Top On, Position Off";
		case 0x57:
			return "Paper Jam: Paper Empty Off, Top Off, Position On";
		case 0x60:
			return "Paper Jam: Cutter Right";
		case 0x61:
			return "Paper Jam: Cutter Left";

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
			return "Unknown";
		}
	default:
		return "Unknown";
	}
}

/* XXX these are generally assumed to be the same as S6245, but
   at least the "user error" category is known to be different. */
static const char *ek8810_error_codes(uint8_t major, uint8_t minor)
{
	switch(major) {
	case 0x01: /* "Controller Error" */
		switch(minor) {
		case 0x01:
			return "Controller: EEPROM Write Timeout";
		case 0x09:
			return "Controller: DSP FW Boot";
		case 0x0A:
			return "Controller: Invalid Print Parameter Table";
		case 0x0B:
			return "Controller: DSP FW Mismatch";
		case 0x0C:
			return "Controller: Print Parameter Table Mismatch";
		case 0x0D:
			return "Controller: FPGA Configuration Failed";
		case 0x0F:
			return "Controller: Main FW Checksum";
		case 0x10:
			return "Controller: Flash Write Failed";
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
			return "Controller: MSP Communication";
		case 0x17:
			return "Controller: THV Autotuning";
		case 0x18:
			return "Controller: THV Value Out of Range";
		case 0x19:
			return "Controller: Thermal Head";
		case 0x1B:
			return "Controller: DSP Communication";
		case 0x1C:
			return "Controller: DSP DMA Failed";
		default:
			return "Controller: Unknown";
		}
	case 0x02: /* "Mechanical Error" */
		switch (minor) {
		case 0x01:
			return "Mechanical: Pinch Head Home";
		case 0x02:
			return "Mechanical: Pinch Head (position 1)";
		case 0x03:
			return "Mechanical: Pinch Head (position 2)";
		case 0x04:
			return "Mechanical: Pinch Head (position 3)";
		case 0x0B:
			return "Mechanical: Cutter (Right)";
		case 0x0C:
			return "Mechanical: Cutter (Left)";
		default:
			return "Mechanical: Unknown";
		}
	case 0x03: /* "Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Sensor: Head Up";
		case 0x02:
			return "Sensor: Head Down";
		case 0x0B:
			return "Sensor: Cutter Left";
		case 0x0C:
			return "Sensor: Cutter Right";
		case 0x0D:
			return "Sensor: Cutter Left+Right";
		case 0x15:
			return "Sensor: Head Up Unstable";
		case 0x16:
			return "Sensor: Head Down Unstable";
		case 0x17:
			return "Sensor: Cutter Left Unstable";
		case 0x18:
			return "Sensor: Cutter Right Unstable";
		case 0x19:
			return "Sensor: Cover Open Unstable";
		case 0x1E:
			return "Sensor: Ribbon Mark (Cyan)";
		case 0x1F:
			return "Sensor: Ribbon Mark (OC)";
		default:
			return "Sensor: Unknown";
		}
	case 0x04: /* "Temperature Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Temp Sensor: Thermal Head Low";
		case 0x02:
			return "Temp Sensor: Thermal Head High";
		case 0x05:
			return "Temp Sensor: Environment Low";
		case 0x06:
			return "Temp Sensor: Environment High";
		case 0x07:
			return "Temp Sensor: Preheat";
		case 0x08:
			return "Temp Sensor: Thermal Protect";
		default:
			return "Temp Sensor: Unknown";
		}
	case 0x5: /* "Paper Jam" */
		switch (minor) {
		case 0x01:
			return "Paper Jam: Loading Paper Top On";
		case 0x02:
			return "Paper Jam: Loading Print Position On";
		case 0x03:
			return "Paper Jam: Loading Print Position Off";
		case 0x04:
			return "Paper Jam: Loading Paper Top Off";
		case 0x05:
			return "Paper Jam: Loading Cut Print Position Off";
		case 0x0C:
			return "Paper Jam: Initializing Print Position Off";
		case 0x0D:
			return "Paper Jam: Initializing Print Position On";
		case 0x15:
			return "Paper Jam: Printing Print Position Off";
		case 0x16:
			return "Paper Jam: Printing Paper Top On";
		case 0x17:
			return "Paper Jam: Printing Paper Top Off";
		case 0x1F:
			return "Paper Jam: Precut Print Position Off";
		case 0x20:
			return "Paper Jam: Precut Print Position On";

		case 0x29:
			return "Paper Jam: Printing Paper Top On";
		case 0x2A:
			return "Paper Jam: Printing Pre-Yellow Print Position Off";
		case 0x2B:
			return "Paper Jam: Printing Yellow Print Position Off";
		case 0x2C:
			return "Paper Jam: Printing Yellow Print Position On";
		case 0x2D:
			return "Paper Jam: Printing Pre-Magenta Print Position Off";
		case 0x2E:
			return "Paper Jam: Printing Magenta Print Position On";
		case 0x2F:
			return "Paper Jam: Printing Magenta Print Position Off";
		case 0x30:
			return "Paper Jam: Printing Pre-Cyan Print Position Off";
		case 0x31:
			return "Paper Jam: Printing Cyan Print Position On";
		case 0x32:
			return "Paper Jam: Printing Cyan Print Position Off";
		case 0x33:
			return "Paper Jam: Printing Pre-OC Print Position Off";
		case 0x34:
			return "Paper Jam: Printing OC Print Position On";
		case 0x35:
			return "Paper Jam: Printing OC Print Position Off";
		case 0x36:
			return "Paper Jam: Cut Print Position Off";
		case 0x37:
			return "Paper Jam: Home Position Off";
		case 0x38:
			return "Paper Jam: Paper Top Off";
		case 0x39:
			return "Paper Jam: Print Position On";

		case 0x51:
			return "Paper Jam: Paper Empty On, Top On, Position On";
		case 0x52:
			return "Paper Jam: Paper Empty On, Top On, Position Off";
		case 0x53:
			return "Paper Jam: Paper Empty On, Top Off, Print Position On";
		case 0x54:
			return "Paper Jam: Paper Empty On, Top Of, Position Off";
		case 0x55:
			return "Paper Jam: Paper Empty Off, Top On, Position On";
		case 0x56:
			return "Paper Jam: Paper Empty Off, Top On, Position Off";
		case 0x57:
			return "Paper Jam: Paper Empty Off, Top Off, Position On";
		case 0x60:
			return "Paper Jam: Cutter Right";
		case 0x61:
			return "Paper Jam: Cutter Left";

		default:
			return "Paper Jam: Unknown";
		}
	case 0x06: /* User/Error */
		switch (minor) {
		case 0x01:
			return "Front Cover Open";
		case 0x02:
			return "Paper Cover Open";
		case 0x03:
			return "Incorrect Ribbon";
		case 0x05:
			return "No Ribbon";
		case 0x06:
			return "Paper Empty";
		case 0x08: // guess?
			return "No Paper";
		case 0x0C: // guess?
			return "Paper End";
		default:
			return "Unknown";
		}
	default:
		return "Unknown";
	}
}

#define RIBBON_NONE 0x00
#define RIBBON_8x10 0x11
#define RIBBON_8x12 0x12

#define RIBBON_8x10K 0x03  /* XXX GUESS - EK8810 (129-4966/109-9787) */
#define RIBBON_8x12K 0x04  /* EK8810 (127-7268/115-6413 or equiv) */

static const char *ribbon_sizes (uint8_t v) {
	switch (v) {
	case RIBBON_NONE:
		return "None";
	case RIBBON_8x10:
	case RIBBON_8x10K:
		return "8x10";
	case RIBBON_8x12:
	case RIBBON_8x12K:
		return "8x12";
	default:
		return "Unknown";
	}
}

static int ribbon_counts (uint8_t v) {
	switch (v) {
	case RIBBON_8x10:
		return 120;
	case RIBBON_8x12:
		return 100;
	case RIBBON_8x10K:
		return 300;
	case RIBBON_8x12K:
		return 250;
	default:
		return 120;
	}
}

struct s6245_errorlog_resp {
	struct sinfonia_status_hdr hdr;
	uint16_t error_count;
	uint8_t  error_major;
	uint8_t  error_minor;
	uint16_t reserved;
	uint32_t print_counter;
	uint16_t ribbon_remain;
	uint8_t  ribbon_takeup_diameter;
	uint8_t  ribbon_supply_diameter;
	uint16_t main_fw_ver;
	uint16_t dsp_fw_ver;
	uint16_t print_param_ver;
	uint16_t boot_fw_ver;
	uint8_t  time_sec;
	uint8_t  time_min;
	uint8_t  time_hour;
	uint8_t  time_day;
	uint8_t  time_month;
	uint8_t  time_year;
	uint16_t reserved2;
	uint8_t  printer_thermistor;
	uint8_t  head_thermistor;
	uint8_t  printer_humidity;
	uint8_t  reserved3[13];
	uint8_t  status;
	uint8_t  reserved4[3];
	uint16_t image_cols;
	uint16_t image_rows;
	uint8_t  reserved5[8];
} __attribute__((packed));

/* Private data structure */
struct shinkos6245_ctx {
	struct sinfonia_usbdev dev;

	uint8_t jobid;

	struct marker marker;

	char serial[32];
	char fwver[32];

	struct sinfonia_6x45_mediainfo_resp media;
};

#define CMDBUF_LEN sizeof(struct s6245_print_cmd)

static int get_status(struct shinkos6245_ctx *ctx)
{
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_status_resp resp;
	struct sinfonia_getextcounter_resp resp2;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSTATUS);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*) &resp, sizeof(resp),
				  &num))) {
		return ret;
	}

	INFO("Printer Status:  0x%02x (%s)\n", resp.hdr.status,
	     sinfonia_status_str(resp.hdr.status));
	if (resp.hdr.status == ERROR_PRINTER) {
		if(resp.hdr.error == ERROR_NONE)
			resp.hdr.error = resp.hdr.status;
		INFO(" Error 0x%02x (%s) 0x%02x/0x%02x (%s)\n",
		     resp.hdr.error,
		     sinfonia_error_str(resp.hdr.error),
		     resp.hdr.printer_major,
		     resp.hdr.printer_minor, ctx->dev.error_codes(resp.hdr.printer_major, resp.hdr.printer_minor));
	}

	/* Sanity checking */
	if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct sinfonia_status_resp) - sizeof(struct sinfonia_status_hdr)))
		return CUPS_BACKEND_OK;

	INFO(" Print Counts:\n");
	INFO("\tSince Paper Changed:\t%08u\n", le32_to_cpu(resp.count_paper));
	INFO("\tLifetime:\t\t%08u\n", le32_to_cpu(resp.count_lifetime));
	INFO("\tMaintenance:\t\t%08u\n", le32_to_cpu(resp.count_maint));
	INFO("\tPrint Head:\t\t%08u\n", le32_to_cpu(resp.count_head));
	INFO(" Cutter Actuations:\t%08u\n", le32_to_cpu(resp.count_cutter));
	INFO(" Ribbon Remaining:\t%8u%%\n", le32_to_cpu(resp.count_ribbon_left));
	INFO(" Prints Remaining:\t%8u\n", ribbon_counts(ctx->media.ribbon_code) - le32_to_cpu(resp.count_paper));
	INFO("Bank 1: 0x%02x (%s) Job %03u @ %03u/%03u (%03u remaining)\n",
	     resp.bank1_status, sinfonia_bank_statuses(resp.bank1_status),
	     resp.bank1_printid,
	     le16_to_cpu(resp.bank1_finished),
	     le16_to_cpu(resp.bank1_specified),
	     le16_to_cpu(resp.bank1_remaining));

	INFO("Bank 2: 0x%02x (%s) Job %03u @ %03u/%03u (%03u remaining)\n",
	     resp.bank2_status, sinfonia_bank_statuses(resp.bank1_status),
	     resp.bank2_printid,
	     le16_to_cpu(resp.bank2_finished),
	     le16_to_cpu(resp.bank2_specified),
	     le16_to_cpu(resp.bank2_remaining));

	INFO("Tonecurve Status: 0x%02x (%s)\n", resp.tonecurve_status, sinfonia_tonecurve_statuses(resp.tonecurve_status));


	/* Query Extended counters */
	if (ctx->dev.conn->type == P_KODAK_8810)
		return CUPS_BACKEND_OK; /* Kodak 8810 returns 12 bytes of garbage. */

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_EXTCOUNTER);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp2, sizeof(resp2),
				  &num))) {
		return ret;
	}

	if (le16_to_cpu(resp2.hdr.payload_len) < 12)
		return CUPS_BACKEND_OK;

	INFO("Lifetime Distance: %08u inches\n", le32_to_cpu(resp2.lifetime_distance));
	INFO("Maintenance Distance: %08u inches\n", le32_to_cpu(resp2.maint_distance));
	INFO("Head Distance: %08u inches\n", le32_to_cpu(resp2.head_distance));

	return CUPS_BACKEND_OK;
}

static int get_errorlog(struct shinkos6245_ctx *ctx)
{
	struct sinfonia_errorlog2_cmd cmd;
	struct s6245_errorlog_resp resp;
	int num = 0;
	int i = 0;

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_ERRORLOG);
	cmd.hdr.len = cpu_to_le16(2);

	do {
		int ret;
		cmd.index = i;

		if ((ret = sinfonia_docmd(&ctx->dev,
					  (uint8_t*)&cmd, sizeof(cmd),
					  (uint8_t*)&resp, sizeof(resp),
					  &num))) {
			return ret;
		}

		if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct s6245_errorlog_resp) - sizeof(struct sinfonia_status_hdr)))
			return -2;

		INFO("Stored Error ID %d:\n", i);
		INFO(" %04d-%02u-%02u %02u:%02u:%02u @ %08u prints : 0x%02x/0x%02x (%s)\n",
		     resp.time_year + 2000, resp.time_month, resp.time_day,
		     resp.time_hour, resp.time_min, resp.time_sec,
		     le32_to_cpu(resp.print_counter),
		     resp.error_major, resp.error_minor,
		     ctx->dev.error_codes(resp.error_major, resp.error_minor));
		INFO("  Temp: %02u/%02u Hum: %02u\n",
		     resp.printer_thermistor, resp.head_thermistor, resp.printer_humidity);
	} while (++i < le16_to_cpu(resp.error_count));

	return CUPS_BACKEND_OK;
}

static void dump_mediainfo(struct sinfonia_6x45_mediainfo_resp *resp)
{
	int i;

        INFO("Loaded Media Type:  %s\n", ribbon_sizes(resp->ribbon_code));
	INFO("Supported Media Information: %u entries:\n", resp->count);
	for (i = 0 ; i < resp->count ; i++) {
		INFO(" %02d: C 0x%02x (%s), %04ux%04u, P 0x%02x (%s)\n", i,
		     resp->items[i].code,
		     sinfonia_print_codes(resp->items[i].code, 1),
		     resp->items[i].columns,
		     resp->items[i].rows,
		     resp->items[i].method,
		     sinfonia_print_methods(resp->items[i].method));
	}
}

static void shinkos6245_cmdline(void)
{
	DEBUG("\t\t[ -b 0|1 ]       # Disable/Enable control panel\n");
	DEBUG("\t\t[ -c filename ]  # Get user/NV tone curve\n");
	DEBUG("\t\t[ -C filename ]  # Set user/NV tone curve\n");
	DEBUG("\t\t[ -e ]           # Query error log\n");
	DEBUG("\t\t[ -F ]           # Flash Printer LED\n");
	DEBUG("\t\t[ -i ]           # Query printer info\n");
	DEBUG("\t\t[ -k num ]       # Set sleep time (5-240 minutes)\n");
	DEBUG("\t\t[ -l filename ]  # Get current tone curve\n");
	DEBUG("\t\t[ -L filename ]  # Set current tone curve\n");
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -r ]           # Reset user/NV tone curve\n");
	DEBUG("\t\t[ -R ]           # Reset printer to factory defaults\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -X jobid ]     # Abort a printjob\n");
//	DEBUG("\t\t[ -Z ]           # Dump all parameters\n");
}

static int shinkos6245_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct shinkos6245_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "b:c:C:eFik:l:L:mrR:sX:Z:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'b':
			if (ctx->dev.conn->type != P_KODAK_8810)
				return -1;
			else if (optarg[0] == '1')
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
			if (ctx->dev.conn->type == P_KODAK_8810) {
				j = sinfonia_geterrorlog(&ctx->dev);
			} else {
				j = get_errorlog(ctx);
			}
			break;
		case 'F':
			j = sinfonia_flashled(&ctx->dev);
			break;
		case 'i':
			j = sinfonia_getfwinfo(&ctx->dev);
			break;
		case 'k': {
			i = atoi(optarg);
			if (i < 5)
				i = 0;
			else if (i < 15)
				i = 1;
			else if (i < 30)
				i = 2;
			else if (i < 60)
				i = 3;
			else if (i < 120)
				i = 4;
			else if (i < 240)
				i = 5;
			else
				i = 5;

			j = sinfonia_setparam(&ctx->dev, PARAM_SLEEP_TIME, i);
			break;
		}
		case 'l':
			j = sinfonia_gettonecurve(&ctx->dev, TONECURVE_CURRENT, optarg);
			break;
		case 'L':
			j = sinfonia_settonecurve(&ctx->dev, UPDATE_TARGET_TONE_CURRENT, optarg);
			break;
		case 'm':
			dump_mediainfo(&ctx->media);
			break;
		case 'r':
			j = sinfonia_resetcurve(&ctx->dev, RESET_TONE_CURVE, TONE_CURVE_ID);
			break;
		case 'R':
			j = sinfonia_resetcurve(&ctx->dev, RESET_PRINTER, 0);
			break;
		case 's':
			j = get_status(ctx);
			break;
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

static void *shinkos6245_init(void)
{
	struct shinkos6245_ctx *ctx = malloc(sizeof(struct shinkos6245_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct shinkos6245_ctx));

	return ctx;
}

static int shinkos6245_attach(void *vctx, struct dyesub_connection *conn, uint8_t jobid)
{
	struct shinkos6245_ctx *ctx = vctx;

	ctx->dev.conn = conn;

	if (conn->type == P_KODAK_8810) {
		ctx->dev.error_codes = &ek8810_error_codes;
		ctx->dev.params = ek8810_params;
		ctx->dev.params_count = ek8810_params_num;
	} else {
		ctx->dev.error_codes = &s6245_error_codes;
		ctx->dev.params = s6245_params;
		ctx->dev.params_count = s6245_params_num;
	}

	/* Ensure jobid is sane */
	ctx->jobid = jobid & 0x7f;
	if (!ctx->jobid)
		ctx->jobid++;

	/* Query Media */
	if (test_mode < TEST_MODE_NOATTACH) {
		int ret = sinfonia_query_media(&ctx->dev,
					       &ctx->media);
		if (ret)
			return ret;
	} else {
		int media_code = RIBBON_8x12;
		if (getenv("MEDIA_CODE"))
			media_code = atoi(getenv("MEDIA_CODE"));

		ctx->media.ribbon_code = media_code;
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = ribbon_sizes(ctx->media.ribbon_code);
	ctx->marker.numtype = ctx->media.ribbon_code;
	ctx->marker.levelmax = 100;
	ctx->marker.levelnow = CUPS_MARKER_UNKNOWN;

	return CUPS_BACKEND_OK;
}

static int shinkos6245_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct shinkos6245_ctx *ctx = vctx;
	struct sinfonia_printjob *job = NULL;
	int ret;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));

	/* Common read/parse code */
	if (ctx->dev.conn->type == P_KODAK_8810) {
		ret = sinfonia_raw18_read_parse(data_fd, job);
	} else {
		ret = sinfonia_read_parse(data_fd, 6245, job);
	}
	if (ret) {
		free(job);
		return ret;
	}

	/* Use whicever copy count is larger */
	if ((int)job->jp.copies > copies)
		job->copies = job->jp.copies;
	else
		job->copies = copies;

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static struct kodak8810_cutlist cutlist_8x4x2 = {
	.entries = 3,
	.cut[0] = cpu_to_le32(12),
	.cut[1] = cpu_to_le32(2408/2),
	.cut[2] = cpu_to_le32(2408-12),
};
static struct kodak8810_cutlist cutlist_8x4x2_d = {
	.entries = 4,
	.cut[0] = cpu_to_le32(12),
	.cut[1] = cpu_to_le32(2408/2 - 38/2),
	.cut[2] = cpu_to_le32(2408/2 + 38/2),
	.cut[3] = cpu_to_le32(2408-12),
};
static struct kodak8810_cutlist cutlist_8x5x2 = {
	.entries = 3,
	.cut[0] = cpu_to_le32(12),
	.cut[1] = cpu_to_le32(3024/2),
	.cut[2] = cpu_to_le32(3024-12),
};
static struct kodak8810_cutlist cutlist_8x5x2_d = {
	.entries = 4,
	.cut[0] = cpu_to_le32(12),
	.cut[1] = cpu_to_le32(3024/2 - 38/2),
	.cut[2] = cpu_to_le32(3024/2 + 38/2),
	.cut[3] = cpu_to_le32(3024-12),
};
static struct kodak8810_cutlist cutlist_8x6x2 = {
	.entries = 3,
	.cut[0] = cpu_to_le32(12),
	.cut[1] = cpu_to_le32(3624/2),
	.cut[2] = cpu_to_le32(3624-12),
};
static struct kodak8810_cutlist cutlist_8x6x2_d = {
	.entries = 4,
	.cut[0] = cpu_to_le32(12),
	.cut[1] = cpu_to_le32(3624/2 - 38/2),
	.cut[2] = cpu_to_le32(3624/2 + 38/2),
	.cut[3] = cpu_to_le32(3624-12),
};
static struct kodak8810_cutlist cutlist_8x4x3 = {
	.entries = 4,
	.cut[0] = cpu_to_le32(12),
	.cut[1] = cpu_to_le32(3624/3),
	.cut[2] = cpu_to_le32(3624/3 + 3624/3),
	.cut[3] = 3624,
};
static struct kodak8810_cutlist cutlist_8x4x3_d = {
	.entries = 6,
	.cut[0] = cpu_to_le32(12),
	.cut[1] = cpu_to_le32(3624/3 - 38/2),
	.cut[2] = cpu_to_le32(3624/3 + 38/2),
	.cut[3] = cpu_to_le32(3624/3 + 3624/3 - 38/2),
	.cut[4] = cpu_to_le32(3624/3 + 3624/3 + 38/2),
	.cut[5] = 3624,
};

static int shinkos6245_main_loop(void *vctx, const void *vjob) {
	struct shinkos6245_ctx *ctx = vctx;

	int ret, num;
	uint8_t cmdbuf[CMDBUF_LEN];

	int last_state = -1, state = S_IDLE;
	int copies;

	struct sinfonia_cmd_hdr *cmd = (struct sinfonia_cmd_hdr *) cmdbuf;;
	struct s6245_print_cmd *print = (struct s6245_print_cmd *) cmdbuf;
	struct sinfonia_status_resp sts, sts2;
	struct sinfonia_status_hdr resp;

	struct sinfonia_printjob *job = (struct sinfonia_printjob*) vjob;
	struct kodak8810_cutlist *cutlist = NULL;

	copies = job->copies;

	/* Cap copies */
	// XXX 120 for 8x10 media, 100 for 8x12 media (S6245 / P910L)
	// 250 for 8x12, 300 for 8x10 (Kodak 8810)
	if (copies > 120)
		copies = 120;

	/* Set up mcut */
	switch (job->jp.media) {
	case CODE_8x4_2:
	case CODE_8x5_2:
	case CODE_8x6_2:
		job->jp.method = PRINT_METHOD_COMBO_2;
		break;
	case CODE_8x4_3:
		job->jp.method = PRINT_METHOD_COMBO_3;
		break;
	default:
		job->jp.method = PRINT_METHOD_STD;
		break;
	}
	// XXX what about mcut |= PRINT_METHOD_DISABLE_ERR;

	if (ctx->dev.conn->type == P_KODAK_8810) {
		/* EK8810 uses special "cutlist" */
		switch (job->jp.media) {
		case CODE_8x4_2:
			if (job->jp.ext_flags & EXT_FLAG_DOUBLESLUG)
				cutlist = &cutlist_8x4x2_d;
			else
				cutlist = &cutlist_8x4x2;
			break;
		case CODE_8x5_2:
			if (job->jp.ext_flags & EXT_FLAG_DOUBLESLUG)
				cutlist = &cutlist_8x5x2_d;
			else
				cutlist = &cutlist_8x5x2;
			break;
		case CODE_8x6_2:
			if (job->jp.ext_flags & EXT_FLAG_DOUBLESLUG)
				cutlist = &cutlist_8x6x2_d;
			else
				cutlist = &cutlist_8x6x2;
			break;
		case CODE_8x4_3:
			if (job->jp.ext_flags & EXT_FLAG_DOUBLESLUG)
				cutlist = &cutlist_8x4x3_d;
			else
				cutlist = &cutlist_8x4x3;
			break;
		default:
			break;
		}

		/* EK8810 supports multi-panel panorama! */
		if (job->jp.rows > 3624) {
			if (copies > 1) {
				WARNING("Multiple copies of panorama prints is not supported!\n");
				copies = 1;
			}
			if (job->jp.media) {
				ERROR("Don't support multi-cut with panorama!\n");
				return CUPS_BACKEND_CANCEL;
			}
			if (job->jp.rows > 9624 &&
			    ctx->media.ribbon_code != RIBBON_8x12 &&
			    ctx->media.ribbon_code != RIBBON_8x12K) {
				/* Sizes over 8x24 require 8x12 media */
				ERROR("Incorrect media loaded for print!\n");
				return CUPS_BACKEND_HOLD;
			}
		}
	}

#if 0  /* Doesn't work on EK8810.  Not sure about S6245 */
	int i;
	/* Validate print sizes */
	for (i = 0; i < ctx->media.count ; i++) {
		/* Look for matching media */
		if (ctx->media.items[i].columns == job->jp.columns &&
		    ctx->media.items[i].rows == job->jp.rows)
			break;
	}
	if (i == ctx->media.count) {
		ERROR("Incorrect media loaded for print!\n");
		return CUPS_BACKEND_HOLD;
	}
#else
	if (ctx->media.ribbon_code != RIBBON_8x12 &&
	    ctx->media.ribbon_code != RIBBON_8x12K &&
	    job->jp.rows > 3024) {
		ERROR("Incorrect media loaded for print!\n");
		return CUPS_BACKEND_HOLD;
	}

#endif

	/* Send Set Time */
	if (ctx->dev.conn->type != P_KODAK_8810) {
		struct sinfonia_settime_cmd *settime = (struct sinfonia_settime_cmd *)cmdbuf;
		time_t now = time(NULL);
		struct tm *cur = localtime(&now);

		memset(cmdbuf, 0, CMDBUF_LEN);
		cmd->cmd = cpu_to_le16(SINFONIA_CMD_SETTIME);
		cmd->len = cpu_to_le16(sizeof(*settime)-sizeof(settime->hdr));
		settime->enable = 1;
		settime->second = cur->tm_sec;
		settime->minute = cur->tm_min;
		settime->hour = cur->tm_hour;
		settime->day = cur->tm_mday;
		settime->month = cur->tm_mon;
		settime->year = cur->tm_year + 1900 - 2000;

		if ((ret = sinfonia_docmd(&ctx->dev,
					  cmdbuf, sizeof(*settime),
					  (uint8_t*)&resp, sizeof(resp),
					  &num))) {
			return CUPS_BACKEND_FAILED;
		}
		if (resp.result != RESULT_SUCCESS) {
			ERROR("Bad result %02x\n", resp.result);
			goto fail;
		}
	}

	// XXX check copies against remaining media!

top:
	if (state != last_state) {
		if (dyesub_debug)
			DEBUG("last_state %d new %d\n", last_state, state);
	}

	/* Send Status Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmd->cmd = cpu_to_le16(SINFONIA_CMD_GETSTATUS);
	cmd->len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				  cmdbuf, sizeof(*cmd),
				  (uint8_t*)&sts, sizeof(sts),
				  &num))) {
		return CUPS_BACKEND_FAILED;
	}

	if (memcmp(&sts2, &sts, sizeof(sts))) {
		memcpy(&sts2, &sts, sizeof(sts));

		INFO("Printer Status: 0x%02x (%s)\n",
		     sts.hdr.status, sinfonia_status_str(sts.hdr.status));

		if (ctx->marker.levelnow != (int)sts.count_ribbon_left) {
			ctx->marker.levelnow = sts.count_ribbon_left;
			dump_markers(&ctx->marker, 1, 0);
		}

		if (sts.hdr.result != RESULT_SUCCESS)
			goto printer_error;
		if (sts.hdr.error == ERROR_PRINTER)
			goto printer_error;
	} else if (state == last_state) {
		sleep(1);
		goto top;
	}
	last_state = state;

	fflush(logger);

	switch (state) {
	case S_IDLE:
		INFO("Waiting for printer idle\n");

		/* make sure we're not colliding with an existing
		   jobid */
		while (ctx->jobid == sts.bank1_printid ||
		       ctx->jobid == sts.bank2_printid) {
			ctx->jobid++;
			ctx->jobid &= 0x7f;
			if (!ctx->jobid)
				ctx->jobid++;
		}

		/* If either bank is free, continue */
		if (sts.bank1_status == BANK_STATUS_FREE ||
		    sts.bank2_status == BANK_STATUS_FREE)
			state = S_PRINTER_READY_CMD;

		break;
	case S_PRINTER_READY_CMD:
		// XXX send "get eeprom backup command"

		if (ctx->dev.conn->type == P_KODAK_8810 && cutlist) {
			cutlist->hdr.cmd = cpu_to_le16(SINFONIA_CMD_SETCUTLIST);
			cutlist->hdr.len = cpu_to_le16(sizeof(*cutlist) - sizeof(cutlist->hdr));

			if ((ret = sinfonia_docmd(&ctx->dev,
						  (uint8_t*)cutlist, sizeof(*cutlist),
						  (uint8_t*)&resp, sizeof(resp),
						  &num))) {
				return CUPS_BACKEND_FAILED;
			}
			if (resp.result != RESULT_SUCCESS) {
				goto printer_error;
			}
		}

		INFO("Sending print job (internal id %u)\n", ctx->jobid);

		memset(cmdbuf, 0, CMDBUF_LEN);
		print->hdr.cmd = cpu_to_le16(SINFONIA_CMD_PRINTJOB);
		print->hdr.len = cpu_to_le16(sizeof (*print) - sizeof(*cmd));
		print->id = ctx->jobid;
		print->count = cpu_to_le16(copies);
		print->columns = print->columns2 = cpu_to_le16(job->jp.columns);
		print->rows = print->rows2 = cpu_to_le16(job->jp.rows);
		print->mode = job->jp.oc_mode;
		print->method = job->jp.method;
//		print->reserved2 = job->jp.media;  /* Ignored */

		if ((ret = sinfonia_docmd(&ctx->dev,
					  cmdbuf, sizeof(*print),
					  (uint8_t*)&resp, sizeof(resp),
					  &num))) {
			return ret;
		}

		if (resp.result != RESULT_SUCCESS) {
			if (resp.error == ERROR_BUFFER_FULL) {
				INFO("Printer Buffers full, retrying\n");
				break;
			} else if ((resp.status & 0xf0) == 0x30 || sts.hdr.status == ERROR_BUFFER_FULL) {
				INFO("Printer busy (%s), retrying\n", sinfonia_status_str(sts.hdr.status));
				break;
			} else if (resp.status != ERROR_NONE || resp.error == ERROR_INVALID_PARAM)
				goto printer_error;
		}

		INFO("Sending image data to printer\n");
		if ((ret = send_data(ctx->dev.conn,
				     job->databuf, job->datalen)))
			return CUPS_BACKEND_FAILED;

		INFO("Waiting for printer to acknowledge completion\n");
		sleep(1);
		state = S_PRINTER_SENT_DATA;
		break;
	case S_PRINTER_SENT_DATA:
		if (fast_return) {
			INFO("Fast return mode enabled.\n");
			state = S_FINISHED;
		} else if (sts.hdr.status == STATUS_READY) {
			state = S_FINISHED;
		}
		break;
	default:
		break;
	};

	if (state != S_FINISHED)
		goto top;

	INFO("Print complete\n");

	return CUPS_BACKEND_OK;

printer_error:
	ERROR("Printer reported error: %#x (%s) status: %#x (%s) -> %#x.%#x (%s)\n",
	      sts.hdr.error,
	      sinfonia_error_str(sts.hdr.error),
	      sts.hdr.status,
	      sinfonia_status_str(sts.hdr.status),
	      sts.hdr.printer_major, sts.hdr.printer_minor,
	      ctx->dev.error_codes(sts.hdr.printer_major, sts.hdr.printer_minor));
fail:
	return CUPS_BACKEND_FAILED;
}

static int shinkos6245_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct shinkos6245_ctx *ctx = vctx;
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_status_resp status;
	int num;

	/* Query Status */
	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSTATUS);
	cmd.len = cpu_to_le16(0);

	if (sinfonia_docmd(&ctx->dev,
			   (uint8_t*)&cmd, sizeof(cmd),
			   (uint8_t*)&status, sizeof(status),
			   &num)) {
		return CUPS_BACKEND_FAILED;
	}

	ctx->marker.levelnow = le32_to_cpu(status.count_ribbon_left);

	if (markers) *markers = &ctx->marker;
	if (count) *count = 1;

	return CUPS_BACKEND_OK;
}

static int shinkos6245_query_stats(void *vctx,  struct printerstats *stats)
{
	struct shinkos6245_ctx *ctx = vctx;
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_status_resp status;
	int num;

	if (shinkos6245_query_markers(ctx, NULL, NULL))
		return CUPS_BACKEND_FAILED;

	/* Query Status */
	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSTATUS);
	cmd.len = cpu_to_le16(0);

	if (sinfonia_docmd(&ctx->dev,
			   (uint8_t*)&cmd, sizeof(cmd),
			   (uint8_t*)&status, sizeof(status),
			   &num)) {
		return CUPS_BACKEND_FAILED;
	}

	switch (ctx->dev.conn->type) {
	case P_SHINKO_S6245:
		stats->mfg = "Sinfonia";
		stats->model = "CE1 / S6245";
		break;
	case P_HITI_910:
		stats->mfg = "HiTi";
		stats->model = "P910L";
		break;
	case P_KODAK_8810:
		stats->mfg = "Kodak";
		stats->model = "8810";
		break;
	default:
		stats->mfg = "Unknown";
		stats->model = "Unknown";
		break;
	}

	if (sinfonia_query_serno(ctx->dev.conn,
				 ctx->serial, sizeof(ctx->serial)))
		return CUPS_BACKEND_FAILED;

	stats->serial = ctx->serial;

	{
		struct sinfonia_fwinfo_cmd  fcmd;
		struct sinfonia_fwinfo_resp resp;

		num = 0;
		fcmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_FWINFO);
		fcmd.hdr.len = cpu_to_le16(1);
		fcmd.target = FWINFO_TARGET_MAIN_APP;

		if (sinfonia_docmd(&ctx->dev,
				   (uint8_t*)&fcmd, sizeof(fcmd),
				   (uint8_t*)&resp, sizeof(resp),
				   &num))
			return CUPS_BACKEND_FAILED;
		snprintf(ctx->fwver, sizeof(ctx->fwver)-1,
			 "%d.%d", resp.major, resp.minor);
		stats->fwver = ctx->fwver;
	}

	stats->decks = 1;
	stats->mediatype[0] = ctx->marker.name;
	stats->levelmax[0] = ctx->marker.levelmax;
	stats->levelnow[0] = ctx->marker.levelnow;
	stats->name[0] = "Roll";
	if (status.hdr.status == ERROR_PRINTER) {
		if(status.hdr.error == ERROR_NONE)
			status.hdr.error = status.hdr.status;
		stats->status[0] = strdup(sinfonia_error_str(status.hdr.error));
	} else {
		stats->status[0] = strdup(sinfonia_status_str(status.hdr.status));
	}
	stats->cnt_life[0] = le32_to_cpu(status.count_lifetime);

	return CUPS_BACKEND_OK;
}

/* Exported */
#define USB_VID_SHINKO       0x10CE
#define USB_PID_SHINKO_S6245 0x001D
#define USB_VID_HITI         0x0D16
#define USB_PID_HITI_P910L   0x000E
#define USB_VID_KODAK        0x040A
#define USB_PID_KODAK_8810   0x404D

static const char *shinkos6245_prefixes[] = {
	"shinkos6245", /* Family Name */
	// backwards compatibility
	"hitip910",
	NULL
};

const struct dyesub_backend shinkos6245_backend = {
	.name = "Sinfonia CHC-S6245 / Kodak 8810",
	.version = "0.37" " (lib " LIBSINFONIA_VER ")",
	.uri_prefixes = shinkos6245_prefixes,
	.cmdline_usage = shinkos6245_cmdline,
	.cmdline_arg = shinkos6245_cmdline_arg,
	.init = shinkos6245_init,
	.attach = shinkos6245_attach,
	.cleanup_job = sinfonia_cleanup_job,
	.read_parse = shinkos6245_read_parse,
	.main_loop = shinkos6245_main_loop,
	.query_serno = sinfonia_query_serno,
	.query_markers = shinkos6245_query_markers,
	.query_stats = shinkos6245_query_stats,
	.devices = {
		{ USB_VID_SHINKO, USB_PID_SHINKO_S6245, P_SHINKO_S6245, NULL, "sinfonia-chcs6245"},
		{ USB_VID_SHINKO, USB_PID_SHINKO_S6245, P_SHINKO_S6245, NULL, "shinko-chcs6245"}, /* Duplicate */
		{ USB_VID_HITI, USB_PID_HITI_P910L, P_HITI_910, NULL, "hiti-p910l"},
		{ USB_VID_KODAK, USB_PID_KODAK_8810, P_KODAK_8810, NULL, "kodak-8810"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* CHC-S6245 data format

  Spool file consists of an 116-byte header, followed by RGB-packed data,
  followed by a 4-byte footer.  Header appears to consist of a series of
  4-byte Little Endian words.

   10 00 00 00 MM MM 00 00  01 00 00 00 01 00 00 00  MM == Model (ie 6245d)
   64 00 00 00 00 00 00 00  TT 00 00 00 00 00 00 00  TT == 0x20 8x4, 0x21 8x5, 0x22 8x6, 0x23 8x8, 0x10 8x10, 0x11 8x12
   00 00 00 00 00 00 00 00  XX 00 00 00 00 00 00 00  XX == 0x03 matte, 0x02 glossy, 0x01 no coat
   00 00 00 00 WW WW 00 00  HH HH 00 00 NN 00 00 00  WW/HH Width, Height (LE), NN == Copies
   00 00 00 00 00 00 00 00  00 00 00 00 ce ff ff ff
   00 00 00 00 ce ff ff ff  QQ QQ 00 00 ce ff ff ff  QQ == DPI (300)
   00 00 00 00 ce ff ff ff  00 00 00 00 00 00 00 00
   00 00 00 00

   [[Packed RGB payload of WW*HH*3 bytes]]

   04 03 02 01  [[ footer ]]

   Kodak 8810 data format:  (Note: EK8810 is actually a Sinfonia CHC-S1845-5A)

  Spool file is the print_cmd_hdr (22 bytes) followed by RGB-packed data.

  01 40 0a 00                    Fixed header
  XX                             Job ID
  CC CC                          Number of copies (1-???)
  WW WW                          Number of columns
  HH HH                          Number of rows
  WW WW                          Number of columns
  HH HH                          Number of rows
  00 00 00 00                    Reserved/Unknown
  LL                             Laminate, 0x02/0x03 (on/satin)
  MM                             Print Method
  00                             Reserved/Unknown

*/
