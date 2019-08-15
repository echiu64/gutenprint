/*
 *   Shinko/Sinfonia CHC-S6245 CUPS backend -- libusb-1.0 version
 *
 *   (c) 2015-2019 Solomon Peachy <pizza@shaftnet.org>
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
#include <time.h>

#define BACKEND shinkos6245_backend

#include "backend_common.h"
#include "backend_sinfonia.h"

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

#define PARAM_DRIVER_MODE  0x3e
#define PARAM_PAPER_MODE   0x3f
#define PARAM_SLEEP_TIME   0x54

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

struct s6245_settime_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t enable;  /* 0 or 1 */
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint8_t year;
} __attribute__((packed));

struct s6245_errorlog_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint16_t index;  /* 0 is latest */
} __attribute__((packed));

static const char *error_codes(uint8_t major, uint8_t minor)
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

struct s6245_status_resp {
	struct sinfonia_status_hdr hdr;
	uint32_t count_lifetime;
	uint32_t count_maint;
	uint32_t count_paper;
	uint32_t count_cutter;
	uint32_t count_head;
	uint32_t count_ribbon_left;
	uint32_t reserved;

	uint8_t  bank1_printid;
	uint16_t bank1_remaining;
	uint16_t bank1_finished;
	uint16_t bank1_specified;
	uint8_t  bank1_status;

	uint8_t  bank2_printid;
	uint16_t bank2_remaining;
	uint16_t bank2_finished;
	uint16_t bank2_specified;
	uint8_t  bank2_status;

	uint8_t  reserved2[16];
	uint8_t  tonecurve_status;
	uint8_t  reserved3[6];
} __attribute__((packed));

struct s6245_geteeprom_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t data[256];
} __attribute__((packed));

struct s6245_mediainfo_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t ribbon_code;
	uint8_t reserved;
	uint8_t count;
	struct sinfonia_mediainfo_item items[10];  /* Not all necessarily used */
} __attribute__((packed));

#define RIBBON_NONE 0x00
#define RIBBON_8x10 0x11
#define RIBBON_8x12 0x12

#define RIBBON_8x10K 0x03  /* XXX GUESS - EK8810 */
#define RIBBON_8x12K 0x04  /* EK8810 */

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
		return 250;
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

	struct s6245_mediainfo_resp media;
};

#define CMDBUF_LEN sizeof(struct s6245_print_cmd)

static int get_status(struct shinkos6245_ctx *ctx)
{
	struct sinfonia_cmd_hdr cmd;
	struct s6245_status_resp resp;
	struct sinfonia_getextcounter_resp resp2;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSTATUS);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*) &resp, sizeof(resp),
				  &num)) < 0) {
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
		     resp.hdr.printer_minor, error_codes(resp.hdr.printer_major, resp.hdr.printer_minor));
	}
	if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct s6245_status_resp) - sizeof(struct sinfonia_status_hdr)))
		return 0;

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
	if (ctx->dev.type == P_KODAK_8810)
		return 0; /* Kodak 8810 returns 12 bytes of garbage. */

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_EXTCOUNTER);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp2, sizeof(resp2),
				  &num)) < 0) {
		return ret;
	}

	if (le16_to_cpu(resp2.hdr.payload_len) < 12)
		return 0;

	INFO("Lifetime Distance: %08u inches\n", le32_to_cpu(resp2.lifetime_distance));
	INFO("Maintenance Distance: %08u inches\n", le32_to_cpu(resp2.maint_distance));
	INFO("Head Distance: %08u inches\n", le32_to_cpu(resp2.head_distance));

	return 0;
}

static int get_errorlog(struct shinkos6245_ctx *ctx)
{
	struct s6245_errorlog_cmd cmd;
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
					  &num)) < 0) {
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
		     error_codes(resp.error_major, resp.error_minor));
		INFO("  Temp: %02u/%02u Hum: %02u\n",
		     resp.printer_thermistor, resp.head_thermistor, resp.printer_humidity);
	} while (++i < le16_to_cpu(resp.error_count));

	return 0;
}

static void dump_mediainfo(struct s6245_mediainfo_resp *resp)
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
}

int shinkos6245_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct shinkos6245_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "c:C:eFik:l:L:mrR:sX:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'c':
			j = sinfonia_gettonecurve(&ctx->dev, TONECURVE_USER, optarg);
			break;
		case 'C':
			j = sinfonia_settonecurve(&ctx->dev, TONECURVE_USER, optarg);
			break;
		case 'e':
			if (ctx->dev.type == P_KODAK_8810) {
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
			j = sinfonia_settonecurve(&ctx->dev, TONECURVE_CURRENT, optarg);
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
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
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

static int shinkos6245_attach(void *vctx, struct libusb_device_handle *dev, int type,
			      uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct shinkos6245_ctx *ctx = vctx;

	int num;

	ctx->dev.dev = dev;
	ctx->dev.endp_up = endp_up;
	ctx->dev.endp_down = endp_down;
	ctx->dev.type = type;
	ctx->dev.error_codes = &error_codes;

	/* Ensure jobid is sane */
	ctx->jobid = jobid & 0x7f;
	if (!ctx->jobid)
		ctx->jobid++;

	/* Query Media */
	if (test_mode < TEST_MODE_NOATTACH) {
		struct sinfonia_cmd_hdr cmd;
		cmd.cmd = cpu_to_le16(SINFONIA_CMD_MEDIAINFO);
		cmd.len = cpu_to_le16(0);

		if (sinfonia_docmd(&ctx->dev,
				   (uint8_t*)&cmd, sizeof(cmd),
				   (uint8_t*)&ctx->media, sizeof(ctx->media),
				   &num)) {
			return CUPS_BACKEND_FAILED;
		}

		/* Byteswap media descriptor.. */
		int i;
		for (i = 0 ; i < ctx->media.count ; i++) {
			ctx->media.items[i].columns = le16_to_cpu(ctx->media.items[i].columns);
			ctx->media.items[i].rows = le16_to_cpu(ctx->media.items[i].rows);
		}

	} else {
		int media_code = RIBBON_8x12;
		if (getenv("MEDIA_CODE"))
			media_code = atoi(getenv("MEDIA_CODE"));

		ctx->media.ribbon_code = media_code;
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = ribbon_sizes(ctx->media.ribbon_code);
	ctx->marker.levelmax = 100;
	ctx->marker.levelnow = -2;

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
	if (ctx->dev.type == P_KODAK_8810) {
		ret = sinfonia_raw18_read_parse(data_fd, job);
	} else {
		ret = sinfonia_read_parse(data_fd, 6245, job);
	}
	if (ret) {
		free(job);
		return ret;
	}

	if (job->jp.copies > 1)
		job->copies = job->jp.copies;
	else
		job->copies = copies;

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int shinkos6245_main_loop(void *vctx, const void *vjob) {
	struct shinkos6245_ctx *ctx = vctx;

	int ret, num;
	uint8_t cmdbuf[CMDBUF_LEN];

	int last_state = -1, state = S_IDLE;
	uint8_t mcut;
	int copies;

	struct sinfonia_cmd_hdr *cmd = (struct sinfonia_cmd_hdr *) cmdbuf;;
	struct s6245_print_cmd *print = (struct s6245_print_cmd *) cmdbuf;
	struct s6245_status_resp sts, sts2;
	struct sinfonia_status_hdr resp;

	struct sinfonia_printjob *job = (struct sinfonia_printjob*) vjob;

	copies = job->copies;

	/* Cap copies */
	// XXX 120 for 8x10 media, 100 for 8x12 media (S6245)
	// 250 for 8x12, 300 for 8x10 (Kodak 8810)
	if (copies > 120)
		copies = 120;

	/* Set up mcut */
	switch (job->jp.media) {
	case CODE_8x4_2:
	case CODE_8x5_2:
	case CODE_8x6_2:
		mcut = PRINT_METHOD_COMBO_2;
		break;
	case CODE_8x4_3:
		mcut = PRINT_METHOD_COMBO_3;
		break;
	default:
		mcut = PRINT_METHOD_STD;
	}
	// XXX what about mcut |= PRINT_METHOD_DISABLE_ERR;

#if 0
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
	if (ctx->dev.type != P_KODAK_8810) {
		struct s6245_settime_cmd *settime = (struct s6245_settime_cmd *)cmdbuf;
		time_t now = time(NULL);
		struct tm *cur = localtime(&now);

		memset(cmdbuf, 0, CMDBUF_LEN);
		cmd->cmd = cpu_to_le16(SINFONIA_CMD_SETTIME);
		cmd->len = cpu_to_le16(0);
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
					  &num)) < 0) {
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
				  &num)) < 0) {
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

	fflush(stderr);

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

		INFO("Sending print job (internal id %u)\n", ctx->jobid);

		memset(cmdbuf, 0, CMDBUF_LEN);
		print->hdr.cmd = cpu_to_le16(SINFONIA_CMD_PRINTJOB);
		print->hdr.len = cpu_to_le16(sizeof (*print) - sizeof(*cmd));
		print->id = ctx->jobid;
		print->count = cpu_to_le16(copies);
		print->columns = print->columns2 = cpu_to_le16(job->jp.columns);
		print->rows = print->rows2 = cpu_to_le16(job->jp.rows);
		print->mode = job->jp.oc_mode;
		print->method = mcut;

		if ((ret = sinfonia_docmd(&ctx->dev,
					  cmdbuf, sizeof(*print),
					  (uint8_t*)&resp, sizeof(resp),
					  &num)) < 0) {
			return ret;
		}

		if (resp.result != RESULT_SUCCESS) {
			if (resp.error == ERROR_BUFFER_FULL) {
				INFO("Printer Buffers full, retrying\n");
				break;
			} else if ((resp.status & 0xf0) == 0x30 || sts.hdr.status == ERROR_BUFFER_FULL) {
				INFO("Printer busy (%s), retrying\n", sinfonia_status_str(sts.hdr.status));
				break;
			} else if (resp.status != ERROR_NONE)
				goto printer_error;
		}

		INFO("Sending image data to printer\n");
		if ((ret = send_data(ctx->dev.dev, ctx->dev.endp_down,
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
	      error_codes(sts.hdr.printer_major, sts.hdr.printer_minor));
fail:
	return CUPS_BACKEND_FAILED;
}

static int shinkos6245_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_getserial_resp resp;
	int ret, num = 0;

	struct sinfonia_usbdev sdev = {
		.dev = dev,
		.endp_up = endp_up,
		.endp_down = endp_down,
	};

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSERIAL);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&sdev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num)) < 0) {
		return ret;
	}

	/* Copy and Null-terminate */
	num = (buf_len > (int)sizeof(resp.data)) ? (int)sizeof(resp.data) : (buf_len - 1);
	memcpy(buf, resp.data, num);
	buf[num] = 0;

	return CUPS_BACKEND_OK;
}

static int shinkos6245_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct shinkos6245_ctx *ctx = vctx;
	struct sinfonia_cmd_hdr cmd;
	struct s6245_status_resp status;
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

	*markers = &ctx->marker;
	*count = 1;

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
	"sinfonia-chcs6245", "hiti-p910l", "kodak-8810",
	// extras
	"shinko-chcs6245",
	// backwards compatibility
	"shinkos6245", "hitip910",
	NULL
};

struct dyesub_backend shinkos6245_backend = {
	.name = "Sinfonia CHC-S6245 / Kodak 8810",
	.version = "0.22" " (lib " LIBSINFONIA_VER ")",
	.uri_prefixes = shinkos6245_prefixes,
	.cmdline_usage = shinkos6245_cmdline,
	.cmdline_arg = shinkos6245_cmdline_arg,
	.init = shinkos6245_init,
	.attach = shinkos6245_attach,
	.cleanup_job = sinfonia_cleanup_job,
	.read_parse = shinkos6245_read_parse,
	.main_loop = shinkos6245_main_loop,
	.query_serno = shinkos6245_query_serno,
	.query_markers = shinkos6245_query_markers,
	.devices = {
		{ USB_VID_SHINKO, USB_PID_SHINKO_S6245, P_SHINKO_S6245, NULL, "shinfonia-chcs6245"},
		{ USB_VID_HITI, USB_PID_HITI_P910L, P_SHINKO_S6245, NULL, "hiti-p910l"},
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
