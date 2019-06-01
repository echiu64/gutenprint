 /*
 *   Shinko/Sinfonia CHC-S2145 CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2019 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND shinkos2145_backend

#include "backend_common.h"
#include "backend_sinfonia.h"

enum {
	S_IDLE = 0,
	S_PRINTER_READY_CMD,
	S_PRINTER_SENT_DATA,
	S_FINISHED,
};

/* Structs for printer */
static int print_counts (uint8_t v) {
	switch (v) {
	case CODE_4x6:
		return 700;
	case CODE_3_5x5:
		return 800;
	case CODE_5x7:
		return 400;
	case CODE_6x9:
		return 310;
	case CODE_6x8:
		return 350;
	default:
		return 700;
	}
}

#if 0
#define PRINT_MODE_DEFAULT      0x01
#define PRINT_MODE_STD_GLOSSY   0x02
#define PRINT_MODE_FINE_GLOSSY  0x03
#define PRINT_MODE_STD_MATTE    0x04
#define PRINT_MODE_FINE_MATTE   0x05
#define PRINT_MODE_STD_EGLOSSY  0x06
#define PRINT_MODE_FINE_EGLOSSY 0x07

static char *s2145_print_modes(uint8_t v) {
	switch (v) {
	case PRINT_MODE_DEFAULT:
		return "Default";
	case PRINT_MODE_STD_GLOSSY:
		return "Std Glossy";
	case PRINT_MODE_FINE_GLOSSY:
		return "Fine Glossy";
	case PRINT_MODE_STD_MATTE:
		return "Std Matte";
	case PRINT_MODE_FINE_MATTE:
		return "Fine Matte";
	case PRINT_MODE_STD_EGLOSSY:
		return "Std ExGlossy";
	case PRINT_MODE_FINE_EGLOSSY:
		return "Fine ExGlossy";
	default:
		return "Unknown";
	}
}
#endif

struct s2145_reset_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  target;
} __attribute__((packed));

#define RESET_PRINTER       0x03
#define RESET_USER_CURVE    0x04

struct s2145_readtone_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  curveid;
} __attribute__((packed));

struct s2145_button_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  enabled;
} __attribute__((packed));

#define BUTTON_ENABLED  0x01
#define BUTTON_DISABLED 0x00

#define FWINFO_TARGET_MAIN_BOOT 0x01
#define FWINFO_TARGET_MAIN_APP  0x02
#define FWINFO_TARGET_DSP_BOOT  0x03
#define FWINFO_TARGET_DSP_APP   0x04
#define FWINFO_TARGET_USB_BOOT  0x05
#define FWINFO_TARGET_USB_APP   0x06
#define FWINFO_TARGET_TABLES    0x07

static char *fwinfo_targets (uint8_t v) {
	switch (v) {
	case FWINFO_TARGET_MAIN_BOOT:
		return "Main Boot";
	case FWINFO_TARGET_MAIN_APP:
		return "Main App ";
	case FWINFO_TARGET_DSP_BOOT:
		return "DSP Boot ";
	case FWINFO_TARGET_DSP_APP:
		return "DSP App  ";
	case FWINFO_TARGET_USB_BOOT:
		return "USB Boot ";
	case FWINFO_TARGET_USB_APP:
		return "USB App  ";
	case FWINFO_TARGET_TABLES:
		return "Tables   ";
	default:
		return "Unknown  ";
	}
}

struct s2145_update_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  target;
	uint32_t reserved;
	uint32_t size;
} __attribute__((packed));

struct s2145_setunique_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  len;
	uint8_t  data[23];  /* Not necessarily all used. */
} __attribute__((packed));

static const char *error_codes(uint8_t major, uint8_t minor)
{
	switch(major) {
	case 0x01: /* "Controller Error" */
		switch(minor) {
		case 0x01:
			return "Controller: EEPROM Write Timeout";
		case 0x02:
			return "Controller: EEPROM Verify";
		case 0x04:
			return "Controller: DSP Inactive";
		case 0x05:
			return "Controller: DSP Application Inactive";
		case 0x06:
			return "Controller: Main FW Data";
		case 0x07:
			return "Controller: Main FW Write";
		case 0x08:
			return "Controller: DSP FW Data";
		case 0x09:
			return "Controller: DSP FW Write";
		case 0x0A:
			return "Controller: 0A ASIC??";
		case 0x0B:
			return "Controller: 0B FPGA??";
		case 0x0D:
			return "Controller: Tone Curve Write";
		case 0x16:
			return "Controller: Invalid Parameter Table";
		case 0x17:
			return "Controller: Parameter Table Data";
		case 0x18:
			return "Controller: Parameter Table Write";
		case 0x29:
			return "Controller: DSP Communication";
		case 0x2A:
			return "Controller: DSP DMA Failure";
		default:
			return "Controller: Unknown";
		}
	case 0x02: /* "Mechanical Error" */
		switch (minor) {
		case 0x01:
			return "Mechanical: Thermal Head (Upper Up)";
		case 0x02:
			return "Mechanical: Thermal Head (Head Up)";
		case 0x03:
			return "Mechanical: Thermal Head (Head Down)";
		case 0x04:
			return "Mechanical: Pinch Roller (Initialize)";
		case 0x05:
			return "Mechanical: Pinch Roller (Mode1)";
		case 0x06:
			return "Mechanical: Pinch Roller (Mode2)";
		case 0x07:
			return "Mechanical: Pinch Roller (Mode3)";
		case 0x08:
			return "Mechanical: Pinch Roller (Mode4)";
		case 0x09:
			return "Mechanical: Cutter (Right)";
		case 0x0A:
			return "Mechanical: Cutter (Left)";
		case 0x0B:
			return "Mechanical: Thermal Head (Head Down Recovery)";
		default:
			return "Mechanical: Unknown";
		}
	case 0x03: /* "Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Sensor: Thermal Head";
		case 0x02:
			return "Sensor: Pinch Roller";
		case 0x03:
			return "Sensor: Cutter Left";
		case 0x04:
			return "Sensor: Cutter Right";
		case 0x05:
			return "Sensor: Cutter Unknown";
		case 0x08:
			return "Sensor: Ribbon Encoder (Supply)";
		case 0x09:
			return "Sensor: Ribbon Encoder (Takeup)";
		case 0x13:
			return "Sensor: Thermal Head";
		default:
			return "Sensor: Unknown";
		}
	case 0x04: /* "Temperature Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Temp Sensor: Thermal Head High";
		case 0x02:
			return "Temp Sensor: Thermal Head Low";
		case 0x03:
			return "Temp Sensor: Environment High";
		case 0x04:
			return "Temp Sensor: Environment Low";
		case 0x05:
			return "Temp Sensor: Warmup Timed Out";
		default:
			return "Temp Sensor: Unknown";
		}
	case 0x5: /* "Paper Jam" */
		switch (minor) {
		case 0x01:
			return "Paper Jam: Loading Leading Edge Off";
		case 0x02:
			return "Paper Jam: Loading Print Position On";
		case 0x03:
			return "Paper Jam: Loading Print Position Off";
		case 0x04:
			return "Paper Jam: Loading Print Position On";
		case 0x05:
			return "Paper Jam: Loading Leading Edge On";
		case 0x11:
			return "Paper Jam: Initializing Print Position Off";
		case 0x12:
			return "Paper Jam: Initializing Print Position On";
		case 0x13:
			return "Paper Jam: Initializing Leading Edge On";
		case 0x14:
			return "Paper Jam: Initializing Print Position On";
		case 0x15:
			return "Paper Jam: Initializing Print Position Off";
		case 0x16:
			return "Paper Jam: Initializing Print Position On";
		case 0x21:
			return "Paper Jam: Initializing Print Position On";
		case 0x22:
			return "Paper Jam: Rewinding Print Position On";
		case 0x40:
			return "Paper Jam: Pre-Printing Print Position Off";
		case 0x41:
			return "Paper Jam: Pre-Printing Print Position Off";
		case 0x42:
			return "Paper Jam: Printing Leading Edge Off";
		case 0x43:
			return "Paper Jam: After Returning Lead Edge Off";
		case 0x44:
			return "Paper Jam: After Printing Print Position Off";
		case 0x45:
			return "Paper Jam: After Printing Print Position On";
		case 0x46:
			return "Paper Jam: After Printing Print Position On";
		case 0x47:
			return "Paper Jam: After Printing Print Position Off";
		case 0x49:
			return "Paper Jam: Printing Lost Ribbon Mark";
		case 0x4A:
			return "Paper Jam: Printing Ribbon Cut";
		case 0x4D:
			return "Paper Jam: Printing Lost M Mark";
		case 0x4E:
			return "Paper Jam: Printing Lost C Mark";
		case 0x4F:
			return "Paper Jam: Printing Lost OP Mark";
		case 0x61:
			return "Paper Jam: Initializing Lead Edge On";
		case 0x62:
			return "Paper Jam: Initizlizing Print Position On";
		case 0x64:
			return "Paper Jam: Initizlizing Paper Size On";
		default:
			return "Paper Jam: Unknown";
		}
	case 0x06: /* User Error */
		switch (minor) {
		case 0x01:
			return "Front Cover Open";
		case 0x02:
			return "Incorrect Ribbon";
		case 0x03:
			return "No Ribbon";
		case 0x04:
			return "Mismatched Ribbon";
		case 0x05:
			return "Mismatched Paper";
		case 0x06:
			return "Paper Empty";
		case 0x08:
			return "No Paper";
		case 0x09:
			return "Take Out Paper";
		case 0x0A:
			return "Cover Open Error";
		case 0x0B:
			return "Thermal Head Damaged";
		case 0x0C:
			return "Thermal Head Recovery";
		default:
			return "Unknown";
		}
	default:
		return "Unknown";
	}
}

struct s2145_status_resp {
	struct sinfonia_status_hdr hdr;
	uint32_t count_lifetime;
	uint32_t count_maint;
	uint32_t count_paper;
	uint32_t count_cutter;
	uint32_t count_head;
	uint32_t count_ribbon_left;
	uint8_t  bank1_printid;
	uint8_t  bank2_printid;
	uint16_t bank1_remaining;
	uint16_t bank1_finished;
	uint16_t bank1_specified;
	uint8_t  bank1_status;
	uint16_t bank2_remaining;
	uint16_t bank2_finished;
	uint16_t bank2_specified;
	uint8_t  bank2_status;
	uint8_t  tonecurve_status;
} __attribute__((packed));

struct s2145_readtone_resp {
	struct sinfonia_status_hdr hdr;
	uint16_t total_size;
} __attribute__((packed));

struct s2145_mediainfo_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t  count;
	struct sinfonia_mediainfo_item items[10];  /* Not all necessarily used */
} __attribute__((packed));

struct s2145_modelname_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t vendor[4];
	uint8_t product[4];
	uint8_t modelname[40];
} __attribute__((packed));

struct s2145_getunique_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t  data[24];  /* Not necessarily all used. */
} __attribute__((packed));

/* Private data structure */
struct shinkos2145_ctx {
	struct sinfonia_usbdev dev;

	uint8_t jobid;

	struct s2145_mediainfo_resp media;
	struct marker marker;
	int media_code;
};

static int get_status(struct shinkos2145_ctx *ctx)
{
	struct sinfonia_cmd_hdr cmd;
	struct s2145_status_resp resp;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSTATUS);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				(uint8_t*)&cmd, sizeof(cmd),
				(uint8_t*)&resp, sizeof(resp),
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
	if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct s2145_status_resp) - sizeof(struct sinfonia_status_hdr)))
		return 0;

	INFO(" Print Counts:\n");
	INFO("\tSince Paper Changed:\t%08u\n", le32_to_cpu(resp.count_paper));
	INFO("\tLifetime:\t\t%08u\n", le32_to_cpu(resp.count_lifetime));
	INFO("\tMaintenance:\t\t%08u\n", le32_to_cpu(resp.count_maint));
	INFO("\tPrint Head:\t\t%08u\n", le32_to_cpu(resp.count_head));
	INFO(" Cutter Actuations:\t%08u\n", le32_to_cpu(resp.count_cutter));
	INFO(" Ribbon Remaining:\t%08u\n", le32_to_cpu(resp.count_ribbon_left));
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

	return 0;
}

static int get_fwinfo(struct shinkos2145_ctx *ctx)
{
	struct sinfonia_fwinfo_cmd  cmd;
	struct sinfonia_fwinfo_resp resp;
	int num = 0;
	int i;

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_FWINFO);
	cmd.hdr.len = cpu_to_le16(1);

	INFO("FW Information:\n");

	for (i = FWINFO_TARGET_MAIN_BOOT ; i <= FWINFO_TARGET_TABLES ; i++) {
		int ret;
		cmd.target = i;

		if ((ret = sinfonia_docmd(&ctx->dev,
					(uint8_t*)&cmd, sizeof(cmd),
					(uint8_t*)&resp, sizeof(resp),
					&num)) < 0) {
			continue;
		}

		if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct sinfonia_fwinfo_resp) - sizeof(struct sinfonia_status_hdr)))
			continue;

		INFO(" %s\t ver %02x.%02x\n", fwinfo_targets(i),
		     resp.major, resp.minor);
#if 0
		INFO("  name:    '%s'\n", resp.name);
		INFO("  type:    '%s'\n", resp.type);
		INFO("  date:    '%s'\n", resp.date);
		INFO("  version: %02x.%02x (CRC %04x)\n", resp.major, resp.minor,
		     le16_to_cpu(resp.checksum));
#endif
	}
	return 0;
}

static void dump_mediainfo(struct s2145_mediainfo_resp *resp)
{
	int i;

	INFO("Supported Media Information: %u entries:\n", resp->count);
	for (i = 0 ; i < resp->count ; i++) {
		INFO(" %02d: C 0x%02x (%s), %04ux%04u, P 0x%02x (%s)\n", i,
		     resp->items[i].code,
		     sinfonia_print_codes(resp->items[i].code, 0),
		     resp->items[i].columns,
		     resp->items[i].rows,
		     resp->items[i].method,
		     sinfonia_print_methods(resp->items[i].method));
	}
}

static int get_user_string(struct shinkos2145_ctx *ctx)
{
	struct sinfonia_cmd_hdr cmd;
	struct s2145_getunique_resp resp;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETUNIQUE);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				(uint8_t*)&cmd, sizeof(cmd),
				(uint8_t*)&resp, sizeof(resp),
				&num)) < 0) {
		return ret;
	}

	/* Null-terminate */
	resp.hdr.payload_len = le16_to_cpu(resp.hdr.payload_len);
	if (resp.hdr.payload_len > 23)
		resp.hdr.payload_len = 23;
	resp.data[resp.hdr.payload_len] = 0;
	INFO("Unique String: '%s'\n", resp.data);
	return 0;
}

static int set_user_string(struct shinkos2145_ctx *ctx, char *str)
{
	struct s2145_setunique_cmd cmd;
	struct sinfonia_status_hdr resp;
	int ret, num = 0;

	if (str) {
		cmd.len = strlen(str);
		if (cmd.len > 23)
			cmd.len = 23;
		memset(cmd.data, 0, sizeof(cmd.data));
		strncpy((char*)cmd.data, str, cmd.len);
	} else {
		cmd.len = 0;
	}

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_SETUNIQUE);
	cmd.hdr.len = cpu_to_le16(cmd.len + 1);

	if ((ret = sinfonia_docmd(&ctx->dev,
				(uint8_t*)&cmd, cmd.len + 1 + sizeof(cmd.hdr),
				(uint8_t*)&resp, sizeof(resp),
				&num)) < 0) {
		return ret;
	}

	return 0;
}

static int reset_curve(struct shinkos2145_ctx *ctx, int target)
{
	struct s2145_reset_cmd cmd;
	struct sinfonia_status_hdr resp;

	int ret, num = 0;

	cmd.target = target;

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_RESET);
	cmd.hdr.len = cpu_to_le16(1);

	if ((ret = sinfonia_docmd(&ctx->dev,
				(uint8_t*)&cmd, sizeof(cmd),
				(uint8_t*)&resp, sizeof(resp),
				&num)) < 0) {
		return ret;
	}

	return 0;
}

static int button_set(struct shinkos2145_ctx *ctx, int enable)
{
	struct s2145_button_cmd cmd;
	struct sinfonia_status_hdr resp;
	int ret, num = 0;

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_BUTTON);
	cmd.hdr.len = cpu_to_le16(1);

	cmd.enabled = enable;

	if ((ret = sinfonia_docmd(&ctx->dev,
				(uint8_t*)&cmd, sizeof(cmd),
				(uint8_t*)&cmd, sizeof(resp),
				&num)) < 0) {
		return ret;
	}

	return 0;
}

static int get_tonecurve(struct shinkos2145_ctx *ctx, int type, char *fname)
{
	struct s2145_readtone_cmd  cmd;
	struct s2145_readtone_resp resp;
	int ret, num = 0;

	uint8_t *data;
	uint16_t curves[TONE_CURVE_SIZE]  = { 0 } ;

	int i,j;

	cmd.curveid = type;

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_READTONE);
	cmd.hdr.len = cpu_to_le16(1);

	INFO("Dump %s Tone Curve to '%s'\n", sinfonia_tonecurve_statuses(type), fname);

	if ((ret = sinfonia_docmd(&ctx->dev,
				(uint8_t*)&cmd, sizeof(cmd),
				(uint8_t*)&resp, sizeof(resp),
				&num)) < 0) {
		return ret;
	}

	resp.total_size = le16_to_cpu(resp.total_size);

	data = malloc(resp.total_size * 2);
	if (!data) {
		ERROR("Memory allocation failure! (%d bytes)\n",
		      resp.total_size * 2);
		return -1;
	}

	i = 0;
	while (i < resp.total_size) {
		ret = read_data(ctx->dev.dev, ctx->dev.endp_up,
				data + i,
				resp.total_size * 2 - i,
				&num);
		if (ret < 0)
			goto done;
		i += num;
	}

	i = j = 0;
	while (i < resp.total_size) {
		memcpy(curves + j, data + i+2, data[i+1]);
		j += data[i+1] / 2;
		i += data[i+1] + 2;
	}

	/* Open file and write it out */
	{
		int tc_fd = open(fname, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
		if (tc_fd < 0) {
			ret = -1;
			goto done;
		}

		for (i = 0 ; i < TONE_CURVE_SIZE; i++) {
			/* Byteswap appropriately */
			curves[i] = cpu_to_be16(le16_to_cpu(curves[i]));
		}
		ret = write(tc_fd, curves, TONE_CURVE_SIZE * sizeof(uint16_t));
		if (ret < 0)
			ERROR("Can't write curve file\n");
		else
			ret = 0;

		close(tc_fd);

	}

done:
	free(data);
	return ret;
}

static int set_tonecurve(struct shinkos2145_ctx *ctx, int target, char *fname)
{
	struct s2145_update_cmd cmd;
	struct sinfonia_status_hdr resp;
	int ret, num = 0;

	INFO("Set %s Tone Curve from '%s'\n", sinfonia_update_targets(target), fname);

	uint16_t *data = malloc(TONE_CURVE_SIZE * sizeof(uint16_t));

	if (!data) {
		ERROR("Memory allocation failure! (%d bytes)\n",
		      TONE_CURVE_SIZE);
		return -1;
	}

	/* Read in file */
	if ((ret = dyesub_read_file(fname, data, TONE_CURVE_SIZE, NULL))) {
		ERROR("Failed to read Tone Curve file\n");
		goto done;
	}

	/* Byteswap data to local CPU.. */
	for (ret = 0; ret < TONE_CURVE_SIZE ; ret++) {
		data[ret] = be16_to_cpu(data[ret]);
	}

	/* Set up command */
	cmd.target = target;
	cmd.reserved = 0;
	cmd.size = cpu_to_le32(TONE_CURVE_SIZE * sizeof(uint16_t));

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_UPDATE);
	cmd.hdr.len = cpu_to_le16(sizeof(struct s2145_update_cmd)-sizeof(cmd.hdr));

	/* Byteswap data to format printer is expecting.. */
	for (ret = 0; ret < TONE_CURVE_SIZE ; ret++) {
		data[ret] = cpu_to_le16(data[ret]);
	}

	if ((ret = sinfonia_docmd(&ctx->dev,
				(uint8_t*)&cmd, sizeof(cmd),
				(uint8_t*)&resp, sizeof(resp),
				&num)) < 0) {
		goto done;
	}

	/* Sent transfer */
	if ((ret = send_data(ctx->dev.dev, ctx->dev.endp_down,
			     (uint8_t *) data, TONE_CURVE_SIZE * sizeof(uint16_t)))) {
		goto done;
	}

done:
	free(data);

	return ret;
}

static void shinkos2145_cmdline(void)
{
	DEBUG("\t\t[ -b 0|1 ]       # Disable/Enable control panel\n");
	DEBUG("\t\t[ -c filename ]  # Get user/NV tone curve\n");
	DEBUG("\t\t[ -C filename ]  # Set user/NV tone curve\n");
	DEBUG("\t\t[ -e ]           # Query error log\n");
	DEBUG("\t\t[ -F ]           # Flash Printer LED\n");
	DEBUG("\t\t[ -i ]           # Query printer info\n");
	DEBUG("\t\t[ -l filename ]  # Get current tone curve\n");
	DEBUG("\t\t[ -L filename ]  # Set current tone curve\n");
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -r ]           # Reset user/NV tone curve\n");
	DEBUG("\t\t[ -R ]           # Reset printer to factory defaults\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -u ]           # Query user string\n");
	DEBUG("\t\t[ -U sometext ]  # Set user string\n");
	DEBUG("\t\t[ -X jobid ]     # Abort a printjob\n");
}

int shinkos2145_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct shinkos2145_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "b:c:C:eFil:L:mr:R:suU:X:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'b':
			if (optarg[0] == '1')
				j = button_set(ctx, BUTTON_ENABLED);
			else if (optarg[0] == '0')
				j = button_set(ctx, BUTTON_DISABLED);
			else
				return -1;
			break;
		case 'c':
			j = get_tonecurve(ctx, TONECURVE_USER, optarg);
			break;
		case 'C':
			j = set_tonecurve(ctx, TONECURVE_USER, optarg);
			break;
		case 'e':
			j = sinfonia_geterrorlog(&ctx->dev);
			break;
		case 'F':
			j = sinfonia_flashled(&ctx->dev);
			break;
		case 'i':
			j = get_fwinfo(ctx);
			break;
		case 'l':
			j = get_tonecurve(ctx, TONECURVE_CURRENT, optarg);
			break;
		case 'L':
			j = set_tonecurve(ctx, TONECURVE_CURRENT, optarg);
			break;
		case 'm':
			dump_mediainfo(&ctx->media);
			break;
		case 'r':
			j = reset_curve(ctx, RESET_USER_CURVE);
			break;
		case 'R':
			j = reset_curve(ctx, RESET_PRINTER);
			break;
		case 's':
			j = get_status(ctx);
			break;
		case 'u':
			j = get_user_string(ctx);
			break;
		case 'U':
			j = set_user_string(ctx, optarg);
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

static void *shinkos2145_init(void)
{
	struct shinkos2145_ctx *ctx = malloc(sizeof(struct shinkos2145_ctx));
	if (!ctx) {
		ERROR("Memory allocation failure! (%d bytes)\n",
		      (int)sizeof(struct shinkos2145_ctx));

		return NULL;
	}
	memset(ctx, 0, sizeof(struct shinkos2145_ctx));

	return ctx;
}

static int shinkos2145_attach(void *vctx, struct libusb_device_handle *dev, int type,
			      uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct shinkos2145_ctx *ctx = vctx;

	ctx->dev.dev = dev;
	ctx->dev.endp_up = endp_up;
	ctx->dev.endp_down = endp_down;
	ctx->dev.type = type;
	ctx->dev.error_codes = &error_codes;

	/* Ensure jobid is sane */
	ctx->jobid = (jobid & 0x7f);
	if (!ctx->jobid)
		ctx->jobid++;

	int media_prints = 65536;
	if (test_mode < TEST_MODE_NOATTACH) {
		/* Query Media */
		struct sinfonia_cmd_hdr cmd;
		int num = 0;
		int i;

		cmd.cmd = cpu_to_le16(SINFONIA_CMD_MEDIAINFO);
		cmd.len = cpu_to_le16(0);

		if (sinfonia_docmd(&ctx->dev,
				 (uint8_t*)&cmd, sizeof(cmd),
				 (uint8_t*)&ctx->media, sizeof(ctx->media),
				 &num)) {
			return CUPS_BACKEND_FAILED;
		}

		/* Byteswap media descriptor.. */
		for (i = 0 ; i < ctx->media.count ; i++) {
			ctx->media.items[i].columns = le16_to_cpu(ctx->media.items[i].columns);
			ctx->media.items[i].rows = le16_to_cpu(ctx->media.items[i].rows);
		}

		/* Figure out the media type... */
		for (i = 0 ; i < ctx->media.count ; i++) {
			if (print_counts(ctx->media.items[i].code) < media_prints) {
				media_prints = print_counts(ctx->media.items[i].code);
				ctx->media_code = ctx->media.items[i].code;
			}
		}
	} else {
		int media_code = CODE_6x9;
		if (getenv("MEDIA_CODE"))
			media_code = atoi(getenv("MEDIA_CODE"));

		media_prints = 680;
		ctx->media_code = media_code;
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = sinfonia_print_codes(ctx->media_code, 0);
	ctx->marker.levelmax = media_prints;
	ctx->marker.levelnow = -2;

	return CUPS_BACKEND_OK;
}

static int shinkos2145_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct shinkos2145_ctx *ctx = vctx;
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
	ret = sinfonia_read_parse(data_fd, 2145, job);
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

static int shinkos2145_main_loop(void *vctx, const void *vjob) {
	struct shinkos2145_ctx *ctx = vctx;

	int ret, num;

	int i, last_state = -1, state = S_IDLE;

	struct sinfonia_printjob *job = (struct sinfonia_printjob*) vjob;
	struct sinfonia_cmd_hdr cmd;
	struct s2145_status_resp sts, sts2;

	/* Validate print sizes */
	for (i = 0; i < ctx->media.count ; i++) {
		/* Look for matching media */
		if (ctx->media.items[i].columns == job->jp.columns &&
		    ctx->media.items[i].rows == job->jp.rows &&
		    ctx->media.items[i].method == job->jp.method)
			break;
	}
	if (i == ctx->media.count) {
		ERROR("Incorrect media loaded for print!\n");
		return CUPS_BACKEND_HOLD;
	}

	// XXX check copies against remaining media!

top:
	if (state != last_state) {
		if (dyesub_debug)
			DEBUG("last_state %d new %d\n", last_state, state);
	}

	/* Send Status Query */
	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSTATUS);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				(uint8_t*)&cmd, sizeof(cmd),
				(uint8_t*)&sts, sizeof(sts),
				&num)) < 0) {
		return CUPS_BACKEND_FAILED;
	}

	if (memcmp(&sts, &sts2, sizeof(sts))) {
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
	case S_PRINTER_READY_CMD: {
		struct sinfonia_printcmd10_hdr print;

		INFO("Sending print job (internal id %u)\n", ctx->jobid);

		memset(&print, 0, sizeof(print));
		print.hdr.cmd = cpu_to_le16(SINFONIA_CMD_PRINTJOB);
		print.hdr.len = cpu_to_le16(sizeof(print) - sizeof(print.hdr));

		print.jobid = ctx->jobid;
		print.copies = cpu_to_le16(job->copies);
		print.columns = cpu_to_le16(job->jp.columns);
		print.rows = cpu_to_le16(job->jp.rows);
		print.media = job->jp.media;
		print.oc_mode = job->jp.oc_mode;
		print.method = job->jp.method;

		if ((ret = sinfonia_docmd(&ctx->dev,
					(uint8_t*)&print, sizeof(print),
					(uint8_t*)&sts, sizeof(sts),
					&num)) < 0) {
			return ret;
		}

		if (sts.hdr.result != RESULT_SUCCESS) {
			if (sts.hdr.error == ERROR_BUFFER_FULL) {
				INFO("Printer Buffers full, retrying\n");
				break;
			} else if ((sts.hdr.status & 0xf0) == 0x30 || sts.hdr.status == 0x21) {
				INFO("Printer busy (%s), retrying\n", sinfonia_status_str(sts.hdr.status));
				break;
			} else if (sts.hdr.status != ERROR_NONE)
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
	}
	case S_PRINTER_SENT_DATA:
		if (fast_return) {
			INFO("Fast return mode enabled.\n");
			state = S_FINISHED;
		} else if (sts.hdr.status == STATUS_READY ||
			   sts.hdr.status == STATUS_FINISHED ||
			   sts.hdr.status == ERROR_PRINTER) {
			state = S_FINISHED;
		}
		break;
	default:
		break;
	};

	if (state != S_FINISHED)
		goto top;

	if (sts.hdr.status == ERROR_PRINTER) {
		if(sts.hdr.error == ERROR_NONE)
			sts.hdr.error = sts.hdr.status;
		INFO(" Error 0x%02x (%s) 0x%02x/0x%02x (%s)\n",
		     sts.hdr.error,
		     sinfonia_error_str(sts.hdr.error),
		     sts.hdr.printer_major,
		     sts.hdr.printer_minor, error_codes(sts.hdr.printer_major, sts.hdr.printer_minor));
	}

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
	return CUPS_BACKEND_FAILED;
}

static int shinkos2145_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct sinfonia_cmd_hdr cmd;
	struct s2145_getunique_resp resp;
	int ret, num = 0;

	struct sinfonia_usbdev sdev = {
		.dev = dev,
		.endp_up = endp_up,
		.endp_down = endp_down,
	};

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETUNIQUE);
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

static int shinkos2145_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct shinkos2145_ctx *ctx = vctx;
	struct sinfonia_cmd_hdr cmd;
	struct s2145_status_resp sts;
	int num;

	/* Query Status */
	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSTATUS);
	cmd.len = cpu_to_le16(0);

	if (sinfonia_docmd(&ctx->dev,
			 (uint8_t*)&cmd, sizeof(cmd),
			 (uint8_t*)&sts, sizeof(sts),
			 &num)) {
		return CUPS_BACKEND_FAILED;
	}

	ctx->marker.levelnow = ctx->marker.levelmax - le32_to_cpu(sts.count_ribbon_left);

	*markers = &ctx->marker;
	*count = 1;

	return CUPS_BACKEND_OK;
}

/* Exported */
#define USB_VID_SHINKO       0x10CE
#define USB_PID_SHINKO_S2145 0x000E

static const char *shinkos2145_prefixes[] = {
	"shinko-chcs2145",
	// extras
	"sinfonia-chcs2145",
	// Backwards compatibility
	"shinkos2145",
	NULL
};

struct dyesub_backend shinkos2145_backend = {
	.name = "Shinko/Sinfonia CHC-S2145/S2",
	.version = "0.61" " (lib " LIBSINFONIA_VER ")",
	.uri_prefixes = shinkos2145_prefixes,
	.cmdline_usage = shinkos2145_cmdline,
	.cmdline_arg = shinkos2145_cmdline_arg,
	.init = shinkos2145_init,
	.attach = shinkos2145_attach,
	.cleanup_job = sinfonia_cleanup_job,
	.read_parse = shinkos2145_read_parse,
	.main_loop = shinkos2145_main_loop,
	.query_serno = shinkos2145_query_serno,
	.query_markers = shinkos2145_query_markers,
	.devices = {
		{ USB_VID_SHINKO, USB_PID_SHINKO_S2145, P_SHINKO_S2145, NULL, "shinko-chc2145"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* CHC-S2145 data format

  Spool file consists of an 116-byte header, followed by RGB-packed data,
  followed by a 4-byte footer.  Header appears to consist of a series of
  4-byte Little Endian words.

   10 00 00 00 MM MM 00 00  00 00 00 00 01 00 00 00  MM == Model (ie 2145d)
   64 00 00 00 00 00 00 00  TT 00 00 00 00 00 00 00  TT == Media/Print Size
   MM 00 00 00 PP 00 00 00  00 00 00 00 00 00 00 00  MM = Print Method (aka cut control), PP = Print Mode
   00 00 00 00 WW WW 00 00  HH HH 00 00 XX 00 00 00  XX == Copies
   00 00 00 00 00 00 00 00  00 00 00 00 ce ff ff ff
   00 00 00 00 ce ff ff ff  QQ QQ 00 00 ce ff ff ff  QQ == DPI, ie 300.
   00 00 00 00 ce ff ff ff  00 00 00 00 00 00 00 00
   00 00 00 00

   [[Packed RGB payload of WW*HH*3 bytes]]

   04 03 02 01  [[ footer ]]

*/
