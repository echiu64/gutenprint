/*
 *   Shinko/Sinfonia CHC-S2145 CUPS backend -- libusb-1.0 version
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

#define BACKEND shinkos2145_backend

#include "backend_common.h"

enum {
	S_IDLE = 0,
	S_PRINTER_READY_CMD,
	S_PRINTER_SENT_DATA,
	S_FINISHED,
};

/* Structure of printjob header.  All fields are LITTLE ENDIAN */
struct s2145_printjob_hdr {
	uint32_t len1;   /* Fixed at 0x10 */
	uint32_t model;  /* Equal to the printer model (eg '2145' or '1245' decimal) */
	uint32_t unk2;
	uint32_t unk3;   /* Fixed at 0x01 */

	uint32_t len2;   /* Fixed at 0x64 */
	uint32_t unk5;
	uint32_t media;
	uint32_t unk6;

	uint32_t method;
	uint32_t mode;
	uint32_t unk7;
	uint32_t unk8;

	uint32_t unk9;
	uint32_t columns;
	uint32_t rows;
	uint32_t copies;

	uint32_t unk10;
	uint32_t unk11;
	uint32_t unk12;
	uint32_t unk13;

	uint32_t unk14;
	uint32_t unk15;
	uint32_t dpi; /* Fixed at '300' (decimal) */
	uint32_t unk16;

	uint32_t unk17;
	uint32_t unk18;
	uint32_t unk19;
	uint32_t unk20;

	uint32_t unk21;
} __attribute__((packed));

/* Private data stucture */
struct shinkos2145_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	uint8_t jobid;

	struct s2145_printjob_hdr hdr;

	uint8_t *databuf;
	int datalen;
};

/* Structs for printer */
struct s2145_cmd_hdr {
	uint16_t cmd;
	uint16_t len;  /* Not including this header */
} __attribute__((packed));

#define S2145_CMD_STATUS    0x0001
#define S2145_CMD_MEDIAINFO 0x0002
#define S2145_CMD_MODELNAME 0x0003
#define S2145_CMD_ERRORLOG  0x0004
#define S2145_CMD_PRINTJOB  0x4001
#define S2145_CMD_CANCELJOB 0x4002
#define S2145_CMD_FLASHLED  0x4003
#define S2145_CMD_RESET     0x4004
#define S2145_CMD_READTONE  0x4005
#define S2145_CMD_BUTTON    0x4006
#define S2145_CMD_GETUNIQUE 0x8003
#define S2145_CMD_FWINFO    0xC003
#define S2145_CMD_UPDATE    0xC004
#define S2145_CMD_SETUNIQUE 0xC007

static char *cmd_names(uint16_t v) {
	switch (le16_to_cpu(v)) {
	case S2145_CMD_STATUS:
		return "Get Status";
	case S2145_CMD_MEDIAINFO:
		return "Get Media Info";
	case S2145_CMD_MODELNAME:
		return "Get Model Name";
	case S2145_CMD_ERRORLOG:
		return "Get Error Log";
	case S2145_CMD_PRINTJOB:
		return "Print";
	case S2145_CMD_CANCELJOB:
		return "Cancel Print";
	case S2145_CMD_FLASHLED:
		return "Flash LEDs";
	case S2145_CMD_RESET:
		return "Reset";
	case S2145_CMD_READTONE:
		return "Read Tone Curve";
	case S2145_CMD_BUTTON:
		return "Button Enable";
	case S2145_CMD_GETUNIQUE:
		return "Get Unique String";
	case S2145_CMD_FWINFO:
		return "Get Firmware Info";
	case S2145_CMD_UPDATE:
		return "Update";
	case S2145_CMD_SETUNIQUE:
		return "Set Unique String";
	default:
		return "Unknown Command";
	}
};

struct s2145_print_cmd {
	struct s2145_cmd_hdr hdr;
	uint8_t  id;
	uint16_t count;
	uint16_t columns;
	uint16_t rows;
	uint8_t  media;
	uint8_t  mode;
	uint8_t  method;
} __attribute__((packed));

#define PRINT_MEDIA_4x6    0x00
#define PRINT_MEDIA_5x3_5  0x01
#define PRINT_MEDIA_5x7    0x03
#define PRINT_MEDIA_6x9    0x05
#define PRINT_MEDIA_6x8    0x06
#define PRINT_MEDIA_2x6    0x07

static char *print_medias (uint8_t v) {
	switch (v) {
	case PRINT_MEDIA_4x6:
		return "4x6";
	case PRINT_MEDIA_5x3_5:
		return "5x3.5";
	case PRINT_MEDIA_5x7:
		return "5x7";
	case PRINT_MEDIA_6x9:
		return "6x9";
	case PRINT_MEDIA_6x8:
		return "6x8";
	case PRINT_MEDIA_2x6:
		return "2x6";
	default:
		return "Unknown";
	}
}

#define PRINT_MODE_DEFAULT      0x01
#define PRINT_MODE_STD_GLOSSY   0x02
#define PRINT_MODE_FINE_GLOSSY  0x03
#define PRINT_MODE_STD_MATTE    0x04
#define PRINT_MODE_FINE_MATTE   0x05
#define PRINT_MODE_STD_EGLOSSY  0x06
#define PRINT_MODE_FINE_EGLOSSY 0x07

#if 0
static char *print_modes(uint8_t v) {
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

#define PRINT_METHOD_STD     0x00
#define PRINT_METHOD_4x6_2UP 0x02
#define PRINT_METHOD_2x6_2UP 0x04

static char *print_methods (uint8_t v) { 
	switch (v) {
	case PRINT_METHOD_STD:
		return "Standard";
	case PRINT_METHOD_4x6_2UP:
		return "4x6 2up";
	case PRINT_METHOD_2x6_2UP:
		return "2x6 2up";
	default:
		return "Unknown";
	}
}

struct s2145_cancel_cmd {
	struct s2145_cmd_hdr hdr;
	uint8_t  id;
} __attribute__((packed));

struct s2145_reset_cmd {
	struct s2145_cmd_hdr hdr;
	uint8_t  target;
} __attribute__((packed));

#define RESET_PRINTER       0x03
#define RESET_USER_CURVE    0x04

struct s2145_readtone_cmd {
	struct s2145_cmd_hdr hdr;
	uint8_t  curveid;
} __attribute__((packed));

struct s2145_button_cmd {
	struct s2145_cmd_hdr hdr;
	uint8_t  enabled;
} __attribute__((packed));

#define BUTTON_ENABLED  0x01
#define BUTTON_DISABLED 0x00

struct s2145_fwinfo_cmd {
	struct s2145_cmd_hdr hdr;
	uint8_t  target;
} __attribute__((packed));

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
	struct s2145_cmd_hdr hdr;
	uint8_t  target;
	uint32_t reserved;
	uint32_t size;
} __attribute__((packed));

#define UPDATE_TARGET_USER    0x03
#define UPDATE_TARGET_CURRENT 0x04

static char *update_targets (uint8_t v) {
	switch (v) {
	case UPDATE_TARGET_USER:
		return "User";
	case UPDATE_TARGET_CURRENT:
		return "Current";
	default:
		return "Unknown";
	}
}

#define UPDATE_SIZE 0x600
/* Update is three channels, Y, M, C;
   each is 256 entries of 11-bit data padded to 16-bits.
   Printer expects LE data.  We use BE data on disk.
*/

struct s2145_setunique_cmd {
	struct s2145_cmd_hdr hdr;
	uint8_t  len;
	uint8_t  data[23];  /* Not necessarily all used. */
} __attribute__((packed));

struct s2145_status_hdr {
	uint8_t  result;
	uint8_t  error;
	uint8_t  printer_major;
	uint8_t  printer_minor;
	uint8_t  reserved[3];
	uint8_t  status;
	uint16_t payload_len;
} __attribute__((packed));

#define RESULT_SUCCESS 0x01
#define RESULT_FAIL    0x02

#define ERROR_NONE              0x00
#define ERROR_INVALID_PARAM     0x01
#define ERROR_MAIN_APP_INACTIVE 0x02
#define ERROR_COMMS_TIMEOUT     0x03
#define ERROR_MAINT_NEEDED      0x04
#define ERROR_BAD_COMMAND       0x05
#define ERROR_PRINTER           0x11
#define ERROR_BUFFER_FULL       0x21

static char *error_codes(uint8_t major, uint8_t minor)
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

static char *error_str(uint8_t v) {
	switch (v) {
	case ERROR_NONE:
		return "None";
	case ERROR_INVALID_PARAM:
		return "Invalid Command Parameter";
	case ERROR_MAIN_APP_INACTIVE:
		return "Main App Inactive";
	case ERROR_COMMS_TIMEOUT:
		return "Main Communication Timeout";
	case ERROR_MAINT_NEEDED:
		return "Maintainence Needed";
	case ERROR_BAD_COMMAND:
		return "Inappropriate Command";
	case ERROR_PRINTER:
		return "Printer Error";
	case ERROR_BUFFER_FULL:
		return "Buffer Full";
	default:
		return "Unknown";
	}
}

#define STATUS_READY            0x00
#define STATUS_INIT_CPU         0x31
#define STATUS_INIT_RIBBON      0x32
#define STATUS_INIT_PAPER       0x33
#define STATUS_THERMAL_PROTECT  0x34
#define STATUS_USING_PANEL      0x35
#define STATUS_SELF_DIAG        0x36
#define STATUS_DOWNLOADING      0x37

#define STATUS_FEEDING_PAPER    0x61
#define STATUS_PRE_HEAT         0x62
#define STATUS_PRINT_Y          0x63
#define STATUS_BACK_FEED_Y      0x64
#define STATUS_PRINT_M          0x65
#define STATUS_BACK_FEED_M      0x66
#define STATUS_PRINT_C          0x67
#define STATUS_BACK_FEED_C      0x68
#define STATUS_PRINT_OP         0x69
#define STATUS_PAPER_CUT        0x6A
#define STATUS_PAPER_EJECT      0x6B
#define STATUS_BACK_FEED_E      0x6C
#define STATUS_FINISHED         0x6D

static char *status_str(uint8_t v) {
	switch (v) {
	case STATUS_READY:
		return "Ready";
	case STATUS_INIT_CPU:
		return "Initializing CPU";
	case STATUS_INIT_RIBBON:
		return "Initializing Ribbon";
	case STATUS_INIT_PAPER:
		return "Loading Paper";
	case STATUS_THERMAL_PROTECT:
		return "Thermal Protection";
	case STATUS_USING_PANEL:
		return "Using Operation Panel";
	case STATUS_SELF_DIAG:
		return "Processing Self Diagnosis";
	case STATUS_DOWNLOADING:
		return "Processing Download";
	case STATUS_FEEDING_PAPER:
		return "Feeding Paper";
	case STATUS_PRE_HEAT:
		return "Pre-Heating";
	case STATUS_PRINT_Y:
		return "Printing Yellow";
	case STATUS_BACK_FEED_Y:
		return "Back-Feeding - Yellow Complete";
	case STATUS_PRINT_M:
		return "Printing Magenta";
	case STATUS_BACK_FEED_M:
		return "Back-Feeding - Magenta Complete";
	case STATUS_PRINT_C:
		return "Printing Cyan";
	case STATUS_BACK_FEED_C:
		return "Back-Feeding - Cyan Complete";
	case STATUS_PRINT_OP:
		return "Laminating";
	case STATUS_PAPER_CUT:
		return "Cutting Paper";
	case STATUS_PAPER_EJECT:
		return "Ejecting Paper";
	case STATUS_BACK_FEED_E:
		return "Back-Feeding - Ejected";
	case STATUS_FINISHED:
		return "Print Finished";
	case ERROR_PRINTER:
		return "Printer Error";
	default:
		return "Unknown";
	}
}

struct s2145_status_resp {
	struct s2145_status_hdr hdr;
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

#define BANK_STATUS_FREE  0x00
#define BANK_STATUS_XFER  0x01
#define BANK_STATUS_FULL  0x02

static char *bank_statuses(uint8_t v)
{
	switch (v) {
	case BANK_STATUS_FREE:
		return "Free";
	case BANK_STATUS_XFER:
		return "Xfer";
	case BANK_STATUS_FULL:
		return "Full";
	default:
		return "Unknown";
	}
}

#define TONECURVE_INIT    0x00
#define TONECURVE_USER    0x01
#define TONECURVE_CURRENT 0x02

static char *tonecurve_statuses (uint8_t v)
{
	switch(v) {
	case 0:
		return "Initial";
	case 1:
		return "UserSet";
	case 2:
		return "Current";
	default:
		return "Unknown";
	}
}

struct s2145_readtone_resp {
	struct s2145_status_hdr hdr;
	uint16_t total_size;
} __attribute__((packed));

struct s2145_mediainfo_item {
	uint8_t  code;
	uint16_t columns;
	uint16_t rows;
	uint8_t  media_type;
	uint8_t  print_type; /* The same as the "print method" */
	uint8_t  reserved[3];
} __attribute__((packed));

#define MEDIA_TYPE_UNKNOWN 0x00
#define MEDIA_TYPE_PAPER   0x01

static char *media_types(uint8_t v) {
	switch (v) {
	case MEDIA_TYPE_UNKNOWN:
		return "Unknown";
	case MEDIA_TYPE_PAPER:
		return "Paper";
	default:
		return "Unknown";
	}
}

struct s2145_mediainfo_resp {
	struct s2145_status_hdr hdr;
	uint8_t  count;
	struct s2145_mediainfo_item items[10];  /* Not all necessarily used */
} __attribute__((packed));

struct s2145_modelname_resp {
	struct s2145_status_hdr hdr;
	uint8_t vendor[4];
	uint8_t product[4];
	uint8_t modelname[40];
} __attribute__((packed));

struct s2145_error_item {
	uint8_t  major;
	uint8_t  minor;
	uint32_t print_counter;
} __attribute__((packed));

struct s2145_errorlog_resp {
	struct s2145_status_hdr hdr;
	uint8_t  count;
	struct s2145_error_item items[10];  /* Not all necessarily used */
} __attribute__((packed));

struct s2145_fwinfo_resp {
	struct s2145_status_hdr hdr;
	uint8_t  name[8];
	uint8_t  type[16];
	uint8_t  date[10];
	uint8_t  major;
	uint8_t  minor;
	uint16_t checksum;
} __attribute__((packed));

struct s2145_getunique_resp {
	struct s2145_status_hdr hdr;
	uint8_t  data[24];  /* Not necessarily all used. */
} __attribute__((packed));

#define READBACK_LEN 128    /* Needs to be larger than largest response hdr */
#define CMDBUF_LEN sizeof(struct s2145_print_cmd)

uint8_t rdbuf[READBACK_LEN];

static int s2145_do_cmd(struct shinkos2145_ctx *ctx,
			uint8_t *cmd, int cmdlen,
			int minlen, int *num)
{
	int ret;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;

	libusb_device_handle *dev = ctx->dev;
	uint8_t endp_up = ctx->endp_up;
	uint8_t endp_down = ctx->endp_down;

	if ((ret = send_data(dev, endp_down,
			     cmd, cmdlen)))
		return (ret < 0) ? ret : -99;

	ret = read_data(dev, endp_up,
			rdbuf, READBACK_LEN, num);

	if (ret < 0)
		return ret;
	if (*num < minlen) {
		ERROR("Short read! (%d/%d))\n", *num, minlen);
		return -99;
	}

	if (resp->result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x (%s)\n", resp->status, 
		     status_str(resp->status));
		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x = %s)\n",
		     resp->result, resp->error, resp->printer_major,
		     resp->printer_minor, error_codes(resp->printer_major, resp->printer_minor));
		return -99;
	}

	return ret;
}

static int get_status(struct shinkos2145_ctx *ctx)
{
	struct s2145_cmd_hdr cmd;
	struct s2145_status_resp *resp = (struct s2145_status_resp *) rdbuf;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(S2145_CMD_STATUS);
	cmd.len = cpu_to_le16(0);

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.cmd));
		return ret;
	}

	INFO("Printer Status:  0x%02x (%s)\n", resp->hdr.status,
	     status_str(resp->hdr.status));
	if (resp->hdr.status == ERROR_PRINTER) {
		if(resp->hdr.error == ERROR_NONE)
			resp->hdr.error = resp->hdr.status;
		INFO(" Error 0x%02x (%s) 0x%02x/0x%02x (%s)\n",
		     resp->hdr.error,
		     error_str(resp->hdr.error),
		     resp->hdr.printer_major,
		     resp->hdr.printer_minor, error_codes(resp->hdr.printer_major, resp->hdr.printer_minor));
	}
	if (le16_to_cpu(resp->hdr.payload_len) != (sizeof(struct s2145_status_resp) - sizeof(struct s2145_status_hdr)))
		return 0;

	INFO(" Print Counts:\n");
	INFO("\tSince Paper Changed:\t%08u\n", le32_to_cpu(resp->count_paper));
	INFO("\tLifetime:\t\t%08u\n", le32_to_cpu(resp->count_lifetime));
	INFO("\tMaintainence:\t\t%08u\n", le32_to_cpu(resp->count_maint));
	INFO("\tPrint Head:\t\t%08u\n", le32_to_cpu(resp->count_head));
	INFO(" Cutter Actuations:\t%08u\n", le32_to_cpu(resp->count_cutter));
	INFO(" Ribbon Remaining:\t%08u\n", le32_to_cpu(resp->count_ribbon_left));
	INFO("Bank 1: 0x%02x (%s) Job %03u @ %03u/%03u (%03u remaining)\n",
	     resp->bank1_status, bank_statuses(resp->bank1_status),
	     resp->bank1_printid,
	     le16_to_cpu(resp->bank1_finished),
	     le16_to_cpu(resp->bank1_specified),
	     le16_to_cpu(resp->bank1_remaining));

	INFO("Bank 2: 0x%02x (%s) Job %03d @ %03d/%03d (%03d remaining)\n",
	     resp->bank2_status, bank_statuses(resp->bank1_status),
	     resp->bank2_printid,
	     le16_to_cpu(resp->bank2_finished),
	     le16_to_cpu(resp->bank2_specified),
	     le16_to_cpu(resp->bank2_remaining));

	INFO("Tonecurve Status: 0x%02x (%s)\n", resp->tonecurve_status, tonecurve_statuses(resp->tonecurve_status));

	return 0;
}

static int get_fwinfo(struct shinkos2145_ctx *ctx)
{
	struct s2145_fwinfo_cmd  cmd;
	struct s2145_fwinfo_resp *resp = (struct s2145_fwinfo_resp *)rdbuf;
	int num = 0;
	int i;

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_FWINFO);
	cmd.hdr.len = cpu_to_le16(1);

	INFO("FW Information:\n");

	for (i = FWINFO_TARGET_MAIN_BOOT ; i <= FWINFO_TARGET_TABLES ; i++) {
		int ret;
		cmd.target = i;

		if ((ret = s2145_do_cmd(ctx,
					(uint8_t*)&cmd, sizeof(cmd),
					sizeof(*resp),
					&num)) < 0) {
			ERROR("Failed to execute %s command (%d)\n", cmd_names(cmd.hdr.cmd), ret);
			continue;
		}

		if (le16_to_cpu(resp->hdr.payload_len) != (sizeof(struct s2145_fwinfo_resp) - sizeof(struct s2145_status_hdr)))
			continue;
		
		INFO(" %s\t ver %02x.%02x\n", fwinfo_targets(i),
		     resp->major, resp->minor);
#if 0
		INFO("  name:    '%s'\n", resp->name);
		INFO("  type:    '%s'\n", resp->type);
		INFO("  date:    '%s'\n", resp->date);
		INFO("  version: %02x.%02x (CRC %04x)\n", resp->major, resp->minor,
		     le16_to_cpu(resp->checksum));
#endif
	}
	return 0;
}

static int get_errorlog(struct shinkos2145_ctx *ctx)
{
	struct s2145_cmd_hdr cmd;
	struct s2145_errorlog_resp *resp = (struct s2145_errorlog_resp *) rdbuf;
	int ret, num = 0;
	int i;

	cmd.cmd = cpu_to_le16(S2145_CMD_ERRORLOG);
	cmd.len = cpu_to_le16(0);

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.cmd));
		return ret;
	}
	
	if (le16_to_cpu(resp->hdr.payload_len) != (sizeof(struct s2145_errorlog_resp) - sizeof(struct s2145_status_hdr)))
		return -2;

	INFO("Stored Error Events: %d entries:\n", resp->count);
	for (i = 0 ; i < resp->count ; i++) {
		INFO(" %02d: @ %08u prints : 0x%02x/0x%02x (%s)\n", i,
		     le32_to_cpu(resp->items[i].print_counter),
		     resp->items[i].major, resp->items[i].minor, 
		     error_codes(resp->items[i].major, resp->items[i].minor));
	}
	return 0;
}

static int get_mediainfo(struct shinkos2145_ctx *ctx) 
{
	struct s2145_cmd_hdr cmd;
	struct s2145_mediainfo_resp *resp = (struct s2145_mediainfo_resp *) rdbuf;
	int ret, num = 0;
	int i;

	cmd.cmd = cpu_to_le16(S2145_CMD_MEDIAINFO);
	cmd.len = cpu_to_le16(0);

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.cmd));
		return ret;
	}
	
	if (le16_to_cpu(resp->hdr.payload_len) != (sizeof(struct s2145_mediainfo_resp) - sizeof(struct s2145_status_hdr)))
		return -2;

	INFO("Supported Media Information: %d entries:\n", resp->count);
	for (i = 0 ; i < resp->count ; i++) {
		INFO(" %02d: C 0x%02x (%s), %04dx%04d, M 0x%02x (%s), P 0x%02x (%s)\n", i,
		     resp->items[i].code, print_medias(resp->items[i].code),
		     le16_to_cpu(resp->items[i].columns),
		     le16_to_cpu(resp->items[i].rows), 
		     resp->items[i].media_type, media_types(resp->items[i].media_type),
		     resp->items[i].print_type, print_methods(resp->items[i].print_type));
	}
	return 0;
}

static int get_user_string(struct shinkos2145_ctx *ctx) 
{
	struct s2145_cmd_hdr cmd;
	struct s2145_getunique_resp *resp = (struct s2145_getunique_resp*) rdbuf;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(S2145_CMD_GETUNIQUE);
	cmd.len = cpu_to_le16(0);

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp) - 1,
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.cmd));
		return ret;
	}

	/* Null-terminate */
	resp->hdr.payload_len = le16_to_cpu(resp->hdr.payload_len);
	if (resp->hdr.payload_len > 23)
		resp->hdr.payload_len = 23;
	resp->data[resp->hdr.payload_len] = 0;
	INFO("Unique String: '%s'\n", resp->data);
	return 0;
}

static int set_user_string(struct shinkos2145_ctx *ctx, char *str)
{
	struct s2145_setunique_cmd cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
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

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_SETUNIQUE);
	cmd.hdr.len = cpu_to_le16(cmd.len + 1);

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, cmd.len + 1 + sizeof(cmd.hdr),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.hdr.cmd));
		return ret;
	}

	return 0;
}

static int cancel_job(struct shinkos2145_ctx *ctx, char *str)
{
	struct s2145_cancel_cmd cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	if (!str)
		return -1;

	cmd.id = atoi(str);

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_CANCELJOB);
	cmd.hdr.len = cpu_to_le16(1);

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.hdr.cmd));
		return ret;
	}

	return 0;
}

static int flash_led(struct shinkos2145_ctx *ctx) 
{
	struct s2145_cmd_hdr cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(S2145_CMD_FLASHLED);
	cmd.len = cpu_to_le16(0);

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.cmd));
		return ret;
	}

	return 0;
}

static int reset_curve(struct shinkos2145_ctx *ctx, int target)
{
	struct s2145_reset_cmd cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	cmd.target = target;

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_RESET);
	cmd.hdr.len = cpu_to_le16(1);

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.hdr.cmd));
		return ret;
	}

	return 0;
}

static int button_set(struct shinkos2145_ctx *ctx, int enable)
{
	struct s2145_button_cmd cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_BUTTON);
	cmd.hdr.len = cpu_to_le16(1);

	cmd.enabled = enable;

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.hdr.cmd));
		return ret;
	}

	return 0;
}

static int get_tonecurve(struct shinkos2145_ctx *ctx, int type, char *fname) 
{
	struct s2145_readtone_cmd  cmd;
	struct s2145_readtone_resp *resp = (struct s2145_readtone_resp *) rdbuf;
	int ret, num = 0;

	uint8_t *data;
	uint16_t curves[UPDATE_SIZE]  = { 0 } ;

	int i,j;

	cmd.curveid = type;

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_READTONE);
	cmd.hdr.len = cpu_to_le16(1);

	INFO("Dump %s Tone Curve to '%s'\n", tonecurve_statuses(type), fname);

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.hdr.cmd));
		return ret;
	}

	resp->total_size = le16_to_cpu(resp->total_size);

	data = malloc(resp->total_size * 2);
	if (!data) {
		ERROR("Memory allocation failure! (%d bytes)\n",
		      resp->total_size * 2);
		return -1;
	}

	i = 0;
	while (i < resp->total_size) {
		ret = read_data(ctx->dev, ctx->endp_up,
				data + i,
				resp->total_size * 2 - i,
				&num);
		if (ret < 0)
			goto done;
		i += num;
	}

	i = j = 0;
	while (i < resp->total_size) {
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

		for (i = 0 ; i < UPDATE_SIZE; i++) {
			/* Byteswap appropriately */
			curves[i] = cpu_to_be16(le16_to_cpu(curves[i]));
		}
		write(tc_fd, curves, UPDATE_SIZE * sizeof(uint16_t));
		close(tc_fd);
	}

done:
	free(data);
	return ret;
}

static int set_tonecurve(struct shinkos2145_ctx *ctx, int target, char *fname) 
{
	struct s2145_update_cmd cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	INFO("Set %s Tone Curve from '%s'\n", update_targets(target), fname);

	uint16_t *data = malloc(UPDATE_SIZE * sizeof(uint16_t));

	if (!data) {
		ERROR("Memory allocation failure! (%d bytes)\n",
		      UPDATE_SIZE);
		return -1;
	}

	/* Read in file */
	int tc_fd = open(fname, O_RDONLY);
	if (tc_fd < 0) {
		ret = -1;
		goto done;
	}
	if (read(tc_fd, data, UPDATE_SIZE * sizeof(uint16_t)) != (UPDATE_SIZE * sizeof(uint16_t))) {
		ret = -2;
		goto done;
	}
	close(tc_fd);
	/* Byteswap data to local CPU.. */
	for (ret = 0; ret < UPDATE_SIZE ; ret++) {
		data[ret] = be16_to_cpu(data[ret]);
	}

	/* Set up command */
	cmd.target = target;
	cmd.reserved = 0;
	cmd.size = cpu_to_le32(UPDATE_SIZE * sizeof(uint16_t));

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_UPDATE);
	cmd.hdr.len = cpu_to_le16(sizeof(struct s2145_update_cmd)-sizeof(cmd.hdr));

	/* Byteswap data to format printer is expecting.. */
	for (ret = 0; ret < UPDATE_SIZE ; ret++) {
		data[ret] = cpu_to_le16(data[ret]);
	}

	if ((ret = s2145_do_cmd(ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.hdr.cmd));
		goto done;
	}

	/* Sent transfer */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t *) data, UPDATE_SIZE * sizeof(uint16_t)))) {
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
	DEBUG("\t\t[ -f ]           # Use fast return mode\n");
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

	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
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
			j = get_errorlog(ctx);
			break;
		case 'F':
			j = flash_led(ctx);
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
			j = get_mediainfo(ctx);
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
			j = cancel_job(ctx, optarg);
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

static void shinkos2145_attach(void *vctx, struct libusb_device_handle *dev, 
			       uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct shinkos2145_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);
	
	ctx->type = lookup_printer_type(&shinkos2145_backend,
					desc.idVendor, desc.idProduct);	

	/* Ensure jobid is sane */
	ctx->jobid = (jobid & 0x7f) + 1;
}

static void shinkos2145_teardown(void *vctx) {
	struct shinkos2145_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);

	free(ctx);
}

static int shinkos2145_read_parse(void *vctx, int data_fd) {
	struct shinkos2145_ctx *ctx = vctx;
	int ret;
	uint8_t tmpbuf[4];

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Read in then validate header */
	ret = read(data_fd, &ctx->hdr, sizeof(ctx->hdr));
	if (ret < 0 || ret != sizeof(ctx->hdr)) {
		if (ret == 0)
			return CUPS_BACKEND_CANCEL;
		ERROR("Read failed (%d/%d/%d)\n", 
		      ret, 0, (int)sizeof(ctx->hdr));
		perror("ERROR: Read failed");
		return ret;
	}

	if (le32_to_cpu(ctx->hdr.len1) != 0x10 ||
	    le32_to_cpu(ctx->hdr.len2) != 0x64 ||
	    le32_to_cpu(ctx->hdr.dpi) != 300) {
		ERROR("Unrecognized header data format!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if (le32_to_cpu(ctx->hdr.model) != 2145) {
		ERROR("Unrecognized printer (%d)!\n", le32_to_cpu(ctx->hdr.model));

		return CUPS_BACKEND_CANCEL;
	}

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	ctx->datalen = le32_to_cpu(ctx->hdr.rows) * le32_to_cpu(ctx->hdr.columns) * 3;
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

static int shinkos2145_main_loop(void *vctx, int copies) {
	struct shinkos2145_ctx *ctx = vctx;

	int ret, num;
	uint8_t cmdbuf[CMDBUF_LEN];
	uint8_t rdbuf2[READBACK_LEN];

	int i, last_state = -1, state = S_IDLE;

	struct s2145_cmd_hdr *cmd = (struct s2145_cmd_hdr *) cmdbuf;;
	struct s2145_print_cmd *print = (struct s2145_print_cmd *) cmdbuf;
	struct s2145_status_resp *sts = (struct s2145_status_resp *) rdbuf; 
	struct s2145_mediainfo_resp *media = (struct s2145_mediainfo_resp *) rdbuf;

	/* Send Media Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmd->cmd = cpu_to_le16(S2145_CMD_MEDIAINFO);
	cmd->len = cpu_to_le16(0);

	if ((ret = s2145_do_cmd(ctx,
				cmdbuf, sizeof(*cmd),
				sizeof(*media),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd->cmd));
		return CUPS_BACKEND_FAILED;
	}
	
	if (le16_to_cpu(media->hdr.payload_len) != (sizeof(struct s2145_mediainfo_resp) - sizeof(struct s2145_status_hdr)))
		return CUPS_BACKEND_FAILED;

	/* Validate print sizes */
	for (i = 0; i < media->count ; i++) {
		/* Look for matching media */
		if (le16_to_cpu(media->items[i].columns) == cpu_to_le16(le32_to_cpu(ctx->hdr.columns)) &&
		    le16_to_cpu(media->items[i].rows) == cpu_to_le16(le32_to_cpu(ctx->hdr.rows)) &&
		    media->items[i].print_type == le32_to_cpu(ctx->hdr.method))
			break;
	}
	if (i == media->count) {
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
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmd->cmd = cpu_to_le16(S2145_CMD_STATUS);
	cmd->len = cpu_to_le16(0);

	if ((ret = s2145_do_cmd(ctx,
				cmdbuf, sizeof(*cmd),
				sizeof(struct s2145_status_hdr),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd->cmd));
		return CUPS_BACKEND_FAILED;
	}

	if (memcmp(rdbuf, rdbuf2, READBACK_LEN)) {
		memcpy(rdbuf2, rdbuf, READBACK_LEN);

		INFO("Printer Status: 0x%02x (%s)\n", 
		     sts->hdr.status, status_str(sts->hdr.status));
		if (sts->hdr.result != RESULT_SUCCESS)
			goto printer_error;		
		if (sts->hdr.error == ERROR_PRINTER)
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
		/* If either bank is free, continue */
		if (sts->bank1_status == BANK_STATUS_FREE || 
		    sts->bank2_status == BANK_STATUS_FREE) 
			state = S_PRINTER_READY_CMD;

		break;
	case S_PRINTER_READY_CMD:
		INFO("Initiating print job (internal id %d)\n", ctx->jobid);

		memset(cmdbuf, 0, CMDBUF_LEN);
		print->hdr.cmd = cpu_to_le16(S2145_CMD_PRINTJOB);
		print->hdr.len = cpu_to_le16(sizeof (*print) - sizeof(*cmd));

		print->id = ctx->jobid;
		print->count = cpu_to_le16(copies);
		print->columns = cpu_to_le16(le32_to_cpu(ctx->hdr.columns));
		print->rows = cpu_to_le16(le32_to_cpu(ctx->hdr.rows));
		print->media = le32_to_cpu(ctx->hdr.media);
		print->mode = le32_to_cpu(ctx->hdr.mode);
		print->method = le32_to_cpu(ctx->hdr.method);

		if ((ret = s2145_do_cmd(ctx,
					cmdbuf, sizeof(*print),
					sizeof(struct s2145_status_hdr),
					&num)) < 0) {
			ERROR("Failed to execute %s command\n", cmd_names(print->hdr.cmd));
			return ret;
		}

		if (sts->hdr.result != RESULT_SUCCESS) {
			if (sts->hdr.error == ERROR_BUFFER_FULL) {
				INFO("Printer Buffers full, retrying\n");
				break;
			} else if ((sts->hdr.status & 0xf0) == 0x30 || sts->hdr.status == 0x21) {
				INFO("Printer busy (%s), retrying\n", status_str(sts->hdr.status));
				break;
			} else if (sts->hdr.status != ERROR_NONE)
				goto printer_error;
		}

		INFO("Sending image data to printer\n");
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf, ctx->datalen)))
			return CUPS_BACKEND_FAILED;

		INFO("Waiting for printer to acknowledge completion\n");
		sleep(1);
		state = S_PRINTER_SENT_DATA;
		break;
	case S_PRINTER_SENT_DATA:
		if (fast_return) {
			INFO("Fast return mode enabled.\n");
			state = S_FINISHED;
		} else if (sts->hdr.status == STATUS_READY ||
			   sts->hdr.status == STATUS_FINISHED) {
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
	      sts->hdr.error, 
	      error_str(sts->hdr.error),
	      sts->hdr.status, 
	      status_str(sts->hdr.status),
	      sts->hdr.printer_major, sts->hdr.printer_minor,
	      error_codes(sts->hdr.printer_major, sts->hdr.printer_minor));
	return CUPS_BACKEND_FAILED;
}

static int shinkos2145_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct s2145_cmd_hdr cmd;
	struct s2145_getunique_resp *resp = (struct s2145_getunique_resp*) rdbuf;
	int ret, num = 0;

	struct shinkos2145_ctx ctx = {
		.dev = dev,
		.endp_up = endp_up,
		.endp_down = endp_down,
	};

	cmd.cmd = cpu_to_le16(S2145_CMD_GETUNIQUE);
	cmd.len = cpu_to_le16(0);

	if ((ret = s2145_do_cmd(&ctx,
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp) - 1,
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.cmd));
		return ret;
	}

	/* Null-terminate */
	resp->hdr.payload_len = le16_to_cpu(resp->hdr.payload_len);
	if (resp->hdr.payload_len > 23)
		resp->hdr.payload_len = 23;
	resp->data[resp->hdr.payload_len] = 0;
	strncpy(buf, (char*)resp->data, buf_len);
	buf[buf_len-1] = 0; /* ensure it's null terminated */

	return CUPS_BACKEND_OK;
}

/* Exported */
#define USB_VID_SHINKO       0x10CE
#define USB_PID_SHINKO_S2145 0x000E

struct dyesub_backend shinkos2145_backend = {
	.name = "Shinko/Sinfonia CHC-S2145",
	.version = "0.46",
	.uri_prefix = "shinkos2145",
	.cmdline_usage = shinkos2145_cmdline,
	.cmdline_arg = shinkos2145_cmdline_arg,
	.init = shinkos2145_init,
	.attach = shinkos2145_attach,
	.teardown = shinkos2145_teardown,
	.read_parse = shinkos2145_read_parse,
	.main_loop = shinkos2145_main_loop,
	.query_serno = shinkos2145_query_serno,
	.devices = {
	{ USB_VID_SHINKO, USB_PID_SHINKO_S2145, P_SHINKO_S2145, ""},
	{ 0, 0, 0, ""}
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
