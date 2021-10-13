/*
 *   Shinko/Sinfonia CHC-S6145 CUPS backend -- libusb-1.0 version
 *
 *   (c) 2015-2021 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Low-level documentation was provided by Sinfonia.  Thank you!
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
 *   An additional permission is granted, under the GPLv3 section 7, to combine
 *   and/or redistribute this program with the proprietary libS6145ImageProcess
 *   and S2245IP libraries, providing you have *written permission* from Sinfonia
 *   Technology Co. LTD to use and/or redistribute that library.
 *
 *   You must still adhere to all other terms of the license to this program
 *   (ie GPLv3) and the license of the libS6145ImageProcess/S2245IP libraries.
 *
 *   SPDX-License-Identifier: GPL-2.0+ with special exception
 *
 */

#define BACKEND shinkos6145_backend

#include "backend_common.h"
#include "backend_sinfonia.h"

#include <time.h>
#include <stdbool.h>

#ifndef WITH_DYNAMIC
#warning "No dynamic loading support!"
#endif

/* Image processing library function prototypes */
typedef void (*dump_announceFN)(FILE *fp);

typedef int (*ImageProcessingFN)(unsigned char *, unsigned short *, void *);
typedef int (*ImageAvrCalcFN)(unsigned char *, unsigned short, unsigned short, unsigned char *);

#define LIB6145_NAME    "libS6145ImageProcess" DLL_SUFFIX    // Official library
#define LIB6145_NAME_RE "libS6145ImageReProcess" DLL_SUFFIX // Reimplemented library

#define S6145_CORRDATA_HEADDOTS_OFFSET  8834
#define S6145_CORRDATA_WIDTH_OFFSET     12432
#define S6145_CORRDATA_HEIGHT_OFFSET    12434
#define S6145_CORRDATA_EXTRA_LEN        4

#define S2245_CORRDATA_HEADER_MODE_OFFSET (64+7)

typedef bool (*ip_imageProcFN)(uint16_t *destData, uint8_t *srcInRgb,
			       uint16_t width, uint16_t height, void *srcIpp);
typedef bool (*ip_checkIppFN)(uint16_t width, uint16_t height, void *srcIpp);
typedef bool (*ip_getMemorySizeFN)(uint32_t *szMemory,
				   uint16_t width, uint16_t height,
				   void *srcIpp);

#define LIB2245_NAME    "libS2245ImageProcess" DLL_SUFFIX    // Official library
#define LIB2245_NAME_RE "libS2245ImageReProcess" DLL_SUFFIX // Reimplemented library

enum {
	S_IDLE = 0,
	S_PRINTER_READY_CMD,
	S_PRINTER_SENT_DATA,
	S_FINISHED,
};

/* Structs for printer */
struct s6145_print_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  id;
	uint16_t count;
	uint16_t columns;
	uint16_t rows;
	uint8_t  media;      /* reserved in docs, but brava21 uses this */
	uint8_t  combo_wait;
	uint8_t  reserved[6];
	uint8_t  unk_1;      /* Brava 21 sets this to 1 */
	uint8_t  method;
	uint8_t  image_avg;
} __attribute__((packed));


struct s6245_errorlog_resp {
	struct sinfonia_status_hdr hdr;
	uint16_t error_count;   // @10
	uint8_t  reserved_12;   // @12
	uint8_t  error_major;   // @13
	uint8_t  error_minor;   // @14
	uint8_t  reserved_15;   // @15
	uint16_t ribbon_remain; // @16  /* 4x6 */
	uint8_t  reserved_18[2];// @18
	uint16_t boot_fw_ver;   // @20
	uint16_t main_fw_ver;   // @22
	uint16_t dsp_fw_ver;    // @24
	uint16_t tables_ver;    // @26
	uint32_t print_counter; // @28
	uint8_t  unkb[42];      // @32

	/* on 6245 series, these additional fields:
	uint8_t  ribbon_takeup_diameter;
	uint8_t  ribbon_supply_diameter;
	uint8_t  time_sec;
	uint8_t  time_min;
	uint8_t  time_hour;
	uint8_t  time_day;
	uint8_t  time_month;
	uint8_t  time_year;
	uint8_t  printer_thermistor;
	uint8_t  head_thermistor;
	uint8_t  printer_humidity;
	uint8_t  status;
	uint16_t image_cols;
	uint16_t image_rows;
	*/
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct s6245_errorlog_resp) == 74);

static const struct sinfonia_param s6145_params[] =
{
	{ PARAM_OC_PRINT, "Overcoat Mode" },
	{ PARAM_PAPER_PRESV, "Paper Preserve Mode" },
	{ PARAM_DRIVER_MODE, "Driver Mode/Wizard" },
	{ PARAM_PAPER_MODE, "Paper Load Mode" },
	{ PARAM_SLEEP_TIME, "Sleep Time" },
	{ PARAM_REGION_CODE, "Region Code" },
};
#define s6145_params_num (sizeof(s6145_params) / sizeof(struct sinfonia_param))

static const struct sinfonia_param s2245_params[] =
{
	{ PARAM_DRIVER_MODE, "Driver Mode/Wizard" },
	{ PARAM_PAPER_MODE, "Paper Load Mode" },
};
#define s2245_params_num (sizeof(s2245_params) / sizeof(struct sinfonia_param))

// CHC-S6145
#define PARAM_OC_PRINT_OFF   0x00000001
#define PARAM_OC_PRINT_GLOSS 0x00000002
#define PARAM_OC_PRINT_MATTE 0x00000003

// CHC-S6145-5A
#define PARAM_PRINTM_OC_OFF    0x00000001
#define PARAM_PRINTM_OC_GLOSS  0x00000002
#define PARAM_PRINTM_OC_MATTE  0x00000012
#define PARAM_PRINTM_STD       0x00000000
#define PARAM_PRINTM_FINE      0x00000004
#define PARAM_PRINTM_FAST      0x00000008

// S6145
#define PARAM_PAPER_PRESV_OFF 0x00000000
#define PARAM_PAPER_PRESV_ON  0x00000001

static const char *s6145_error_codes(uint8_t major, uint8_t minor)
{
	switch(major) {
	case 0x01: /* "Controller Error" */
		switch(minor) {
		case 0x01:
			return "Controller: EEPROM Write Timeout";
		case 0x0A:
			return "Controller: Invalid Print Parameter Table";
		case 0x0C:
			return "Controller: Print Parameter Table Mismatch";
		case 0x0F:
			return "Controller: Main FW Checksum";
		case 0x10:
			return "Controller: Flash Write Failed";
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
		case 0x1A:
			return "Controller: Wake from Power Save Failed";
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
		case 0x54:
			return "Paper Jam: Paper Empty On, Top Off, Position Off";
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
		case 0x04:
			return "Ribbon Empty";
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

static const char *s2245_error_codes(uint8_t major, uint8_t minor)
{
	switch(major) {
	case 0x01: /* "Controller Error" */
		switch(minor) {
		case 0x01:
			return "Controller: EEPROM Write Timeout";
		case 0x0A:
			return "Controller: Invalid Print Parameter Table";
		case 0x0C:
			return "Controller: Print Parameter Table Mismatch";
		case 0x0F:
			return "Controller: Main FW Data Error";
		case 0x10:
			return "Controller: Main FW Write Failed";
		case 0x13:
			return "Controller: Print Parameter Table Checksum";
		case 0x14:
			return "Controller: Print Parameter Table Write Failed";
		case 0x15:
			return "Controller: User Tone Curve Write Failed";
		case 0x16:
			return "Controller: MSP Communication";
		case 0x19:
			return "Controller: Thermal Head";
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
		case 0x05:
			return "Mechanical: Pinch Head (position 4)";
		case 0x14:
			return "Mechanical: Cutter (Left-Right)";
		case 0x15:
			return "Mechanical: Cutter (Right-Left)";
		default:
			return "Mechanical: Unknown";
		}
	case 0x03: /* "Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Sensor: Cutter Left+Right";
		case 0x02:
			return "Sensor: Cutter Left drove";
		case 0x03:
			return "Sensor: Cutter Right drove";
		case 0x1E:
			return "Sensor: Left Front Lock";
		case 0x1F:
			return "Sensor: Right Front Lock";
		case 0x20:
			return "Sensor: Cutter Left";
		case 0x21:
			return "Sensor: Cutter Right";
		case 0x22:
			return "Sensor: Left Head/Pinch";
		case 0x23:
			return "Sensor: Right Head/Pinch";
		case 0x24:
			return "Sensor: Head/Pinch Encoder";
		case 0x25:
			return "Sensor: Supply Ribbon Encoder";
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
			return "Paper Jam: Paper Not Detected";
		case 0x02:
			return "Paper Jam: Pinch Roller Fail";
		case 0x10:
			return "Paper Jam: Print Position Sensor on";
		case 0x11:
			return "Paper Jam: Print Position Sensor off";
		case 0x12:
			return "Paper Jam: Paper End Sensor on";
		case 0x13:
			return "Paper Jam: Paper Cut";
		case 0x30:
			return "Paper Jam: Print Position Sensor early";
		case 0x31:
			return "Paper Jam: Print Position Sensor on";
		case 0x32:
			return "Paper Jam: Print Position Sensor off";
		case 0x40:
			return "Paper Jam: Cutter Left-Right";
		case 0x41:
			return "Paper Jam: Cutter Right-Left";
		case 0x51:
			return "Paper Jam: Paper End On, Position Off";
		case 0x52:
			return "Paper Jam: Paper End Off, Position On";
		case 0x53:
			return "Paper Jam: Paper End On, Position On";
		default:
			return "Paper Jam: Unknown";
		}
	case 0x06: /* User Error */
		switch (minor) {
		case 0x01:
			return "Cover Open";
		case 0x02:
			return "Cover Not Closed";
		case 0x03:
			return "Incorrect Ribbon";
		case 0x04:
			return "Ribbon End";
		case 0x07:
			return "No Paper";
		case 0x08:
			return "Paper End";
		case 0x0D:
			return "No Ribbon";
		case 0x0E:
			return "Ribbon Rewind Failure";
		case 0x0F:
			return "Ribbon Sense Failure";
		case 0x20:
			return "THV Tuning";
		case 0x21:
			return "THV Not Tuned";
		default:
			return "Unknown";
		}
	default:
		return "Unknown";
	}
}

#define RIBBON_NONE   0x00
#define RIBBON_4x6    0x01
#define RIBBON_3_5x5  0x02
#define RIBBON_5x7    0x03
#define RIBBON_6x8    0x04
#define RIBBON_6x9    0x05

#define RIBBON_89x60mm 0x01

static int ribbon_sizes (uint8_t v, uint8_t is_card, uint8_t is_2245) {
	if (is_card) {
		return 450;
	}

	switch (v) {
	case RIBBON_4x6:
		if (is_2245)
			return 900;
		else
			return 300;
	case RIBBON_3_5x5:
		return 340;
	case RIBBON_5x7:
		return 170;
	case RIBBON_6x8:
		if (is_2245)
			return 450;
		else
			return 150;
	case RIBBON_6x9:
		return 130; // XXX guessed
	default:
		if (is_2245)
			return 450;
		else
			return 300;
	}
}

static const char *print_ribbons (uint8_t v, uint8_t is_card) {
	if (is_card) {
		if (v == RIBBON_89x60mm)
			return "89x60mm";
		else if (v == RIBBON_NONE)
			return "None";
		else
			return "Unknown";
	}

	switch (v) {
	case RIBBON_NONE:
		return "None";
	case RIBBON_4x6:
		return "4x6";
	case RIBBON_3_5x5:
		return "3.5x5";
	case RIBBON_5x7:
		return "5x7";
	case RIBBON_6x8:
		return "6x8";
	case RIBBON_6x9:
		return "6x9";
	default:
		return "Unknown";
	}
}

struct s6145_imagecorr_resp {
	struct sinfonia_status_hdr hdr;
	uint16_t total_size;
} __attribute__((packed));

struct s2245_imagecorr_req {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  options;
	uint8_t  flags;
	uint8_t  null[10];
} __attribute__((packed));

#define S2245_IMAGECORR_FLAG_CONTOUR_ENH 0x01

struct s2245_imagecorr_resp {
	struct sinfonia_status_hdr hdr;
	uint32_t total_size;
} __attribute__((packed));

struct s6145_imagecorr_data {
	uint8_t  remain_pkt;
	uint8_t  return_size; /* Always 0x10 */
	uint8_t  data[16];
} __attribute__((packed));

/* Private data structure */
struct shinkos6145_ctx {
	struct sinfonia_usbdev dev;

	uint8_t jobid;

	uint8_t image_avg[3]; /* YMC */

	char serial[32];
	char fwver[32];

	int is_card; /* card printer model */
	int is_2245; /* 2245 or its variants */

	struct marker marker;

	struct sinfonia_6x45_mediainfo_resp media;

	uint8_t *eeprom;
	size_t eepromlen;

	void *dl_handle;

	dump_announceFN DumpAnnounce;
	ImageProcessingFN ImageProcessing;
	ImageAvrCalcFN ImageAvrCalc;

	ip_imageProcFN ip_imageProc;
	ip_checkIppFN  ip_checkIpp;
	ip_getMemorySizeFN ip_getMemorySize;

	void *corrdata;  /* Correction table */
	uint16_t corrdatalen;
};

static const char *s2245_drivermodes(uint8_t val);
static int shinkos2245_get_imagecorr(struct shinkos6145_ctx *ctx, uint8_t options);
static int shinkos6145_get_imagecorr(struct shinkos6145_ctx *ctx);
static int shinkos6145_get_eeprom(struct shinkos6145_ctx *ctx);

static int get_status(struct shinkos6145_ctx *ctx)
{
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_status_resp resp;
	struct sinfonia_getextcounter_resp resp2;
	int ret, num = 0;
	uint32_t val;

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSTATUS);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp), &num)) < 0) {
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
	if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct sinfonia_status_resp) - sizeof(struct sinfonia_status_hdr)))
		return -1;

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

	/* Query Extended counters */
	cmd.cmd = cpu_to_le16(SINFONIA_CMD_EXTCOUNTER);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp2, sizeof(resp2),
				  &num))) {
		return ret;
	}
	if (le16_to_cpu(resp2.hdr.payload_len) != (sizeof(struct sinfonia_getextcounter_resp) - sizeof(struct sinfonia_status_hdr)))
		return -1;

	INFO("Lifetime Distance:     %08u inches\n", le32_to_cpu(resp2.lifetime_distance));
	INFO("Maintenance Distance:  %08u inches\n", le32_to_cpu(resp2.maint_distance));
	INFO("Head Distance:         %08u inches\n", le32_to_cpu(resp2.head_distance));

	/* Query various params */
	if (ctx->dev.conn->type == P_SHINKO_S6145D) {
		if ((ret = sinfonia_getparam(&ctx->dev, PARAM_REGION_CODE, &val))) {
			ERROR("Failed to execute command\n");
			return ret;
		}
		INFO("Region Code: %#x\n", val);

	}
	if (!ctx->is_2245) {
		if ((ret = sinfonia_getparam(&ctx->dev, PARAM_PAPER_PRESV, &val))) {
			ERROR("Failed to execute command\n");
			return ret;
		}
		INFO("Paper Preserve mode: %s\n", (val ? "On" : "Off"));
	}
	if ((ret = sinfonia_getparam(&ctx->dev, PARAM_DRIVER_MODE, &val))) {
		ERROR("Failed to execute command\n");
		return ret;
	}
	if (ctx->is_2245) {
		INFO("Driver mode:         %s\n", s2245_drivermodes(val));
	} else {
		INFO("Driver mode:         %s\n", (val ? "On" : "Off"));
	}

	if ((ret = sinfonia_getparam(&ctx->dev, PARAM_PAPER_MODE, &val))) {
		ERROR("Failed to execute command\n");
		return ret;
	}
	INFO("Paper load mode:     %s\n", (val ? "Cut" : "No Cut"));

	if (!ctx->is_2245) {
		if ((ret = sinfonia_getparam(&ctx->dev, PARAM_SLEEP_TIME, &val))) {
			ERROR("Failed to execute command\n");
			return ret;
		}
		if (val == 0)
			val = 5;
		else if (val == 1)
			val = 15;
		else if (val == 2)
			val = 30;
		else if (val == 3)
			val = 60;
		else if (val == 4)
			val = 120;
		else if (val >= 5)
			val = 240;
		else
			val = 240; // default?

		INFO("Sleep delay:         %u minutes\n", val);
	}

	return CUPS_BACKEND_OK;
}

static void dump_mediainfo(struct sinfonia_6x45_mediainfo_resp *resp, int is_card)
{
	int i;

	INFO("Loaded Media Type:  %s\n", print_ribbons(resp->ribbon_code, is_card));
	INFO("Supported Print Sizes: %u entries:\n", resp->count);
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

static int shinkos6145_dump_corrdata(struct shinkos6145_ctx *ctx, char *fname)
{
	int ret;

	if (ctx->is_2245) {
		ret = shinkos2245_get_imagecorr(ctx, 0x0a); // XXX have to supply something..  this is HQ matte.
	} else {
		ret = shinkos6145_get_imagecorr(ctx);
	}
	if (ret) {
		ERROR("Failed to execute command\n");
		return ret;
	}

	/* Open file and write it out */
	{
		int fd = open(fname, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
		if (fd < 0) {
			ERROR("Unable to open filename\n");
			return fd;
		}

		ret = write(fd, ctx->corrdata, ctx->corrdatalen);
		close(fd);
	}

	/* Free the buffers */
	free(ctx->corrdata);
	ctx->corrdata = NULL;
	ctx->corrdatalen = 0;

	return ret;
}

static int shinkos6145_dump_eeprom(struct shinkos6145_ctx *ctx, char *fname)
{
	int ret;

	ret = shinkos6145_get_eeprom(ctx);
	if (ret) {
		ERROR("Failed to execute command\n");
		return ret;
	}

	/* Open file and write it out */
	{
		int fd = open(fname, O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
		if (fd < 0) {
			ERROR("Unable to open filename\n");
			return fd;
		}

		ret = write(fd, ctx->eeprom, ctx->eepromlen);
		close(fd);
	}

	/* Free the buffers */
	free(ctx->eeprom);
	ctx->eeprom = NULL;
	ctx->eepromlen = 0;

	return ret;
}

static int shinkos6145_get_imagecorr(struct shinkos6145_ctx *ctx)
{
	struct sinfonia_cmd_hdr cmd;
	struct s6145_imagecorr_resp resp;

	uint16_t total = 0;
	int ret, num;
	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETCORR);
	cmd.len = 0;

	if (ctx->corrdata) {
		free(ctx->corrdata);
		ctx->corrdata = NULL;
	}

	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
		goto done;
	}

	ctx->corrdatalen = le16_to_cpu(resp.total_size);
	INFO("Fetching %u bytes of image correction data\n", ctx->corrdatalen);

	/* We need a little extra to pass arguments to the library */
	ctx->corrdata = malloc(ctx->corrdatalen + S6145_CORRDATA_EXTRA_LEN);
	if (!ctx->corrdata) {
		ERROR("Memory allocation failure\n");
		ret = CUPS_BACKEND_FAILED;
		goto done;
	}
	memset(ctx->corrdata, 0, ctx->corrdatalen);
	total = 0;

	while (total < ctx->corrdatalen) {
		struct s6145_imagecorr_data data;

		ret = read_data(ctx->dev.conn, (uint8_t *) &data,
				sizeof(data),
				&num);
		if (ret < 0)
			goto done;

		memcpy(((uint8_t*)ctx->corrdata) + total, data.data, sizeof(data.data));
		total += sizeof(data.data);

		if (data.remain_pkt == 0)
			DEBUG("correction block transferred (%u/%u total)\n", total, ctx->corrdatalen);

	}

done:
	return ret;
}

static int shinkos2245_get_imagecorr(struct shinkos6145_ctx *ctx, uint8_t options)
{
	struct s2245_imagecorr_req cmd;
	struct s2245_imagecorr_resp resp;

	uint16_t total = 0;
	int ret, num;
	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_GETCORR);
	cmd.hdr.len = sizeof(cmd) - sizeof(cmd.hdr);
	cmd.options = options;
	cmd.flags = S2245_IMAGECORR_FLAG_CONTOUR_ENH; // XXX make configurable?  or key off a flag in the job?
	memset(cmd.null, 0, sizeof(cmd.null));

	if (ctx->corrdata) {
		free(ctx->corrdata);
		ctx->corrdata = NULL;
	}

	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
		goto done;
	}

	ctx->corrdatalen = le16_to_cpu(resp.total_size);
	INFO("Fetching %u bytes of image correction data\n", ctx->corrdatalen);

	ctx->corrdata = malloc(ctx->corrdatalen);
	if (!ctx->corrdata) {
		ERROR("Memory allocation failure\n");
		ret = CUPS_BACKEND_FAILED;
		goto done;
	}
	memset(ctx->corrdata, 0, ctx->corrdatalen);
	total = 0;

	while (total < ctx->corrdatalen) {
		struct s6145_imagecorr_data data;

		ret = read_data(ctx->dev.conn, (uint8_t *) &data,
				sizeof(data),
				&num);
		if (ret < 0)
			goto done;

		memcpy(((uint8_t*)ctx->corrdata) + total, data.data, sizeof(data.data));
		total += sizeof(data.data);

		if (data.remain_pkt == 0)
			DEBUG("correction block transferred (%u/%u total)\n", total, ctx->corrdatalen);

	}

done:
	return ret;
}

static int shinkos6145_get_eeprom(struct shinkos6145_ctx *ctx)
{
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_geteeprom_resp resp;

	int ret, num;
	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETEEPROM);
	cmd.len = 0;

	if (ctx->eeprom) {
		free(ctx->eeprom);
		ctx->eeprom = NULL;
	}

	if ((ret = sinfonia_docmd(&ctx->dev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
		goto done;
	}

	ctx->eepromlen = le16_to_cpu(resp.hdr.payload_len);
	ctx->eeprom = malloc(ctx->eepromlen);
	if (!ctx->eeprom) {
		ERROR("Memory allocation failure\n");
		ret = CUPS_BACKEND_FAILED;
		goto done;
	}
	memcpy(ctx->eeprom, resp.data, ctx->eepromlen);

done:
	return ret;
}

static const char *s2245_drivermodes(uint8_t val)
{
	switch(val) {
	case 0x00:
		return "-Class/-iSerial";
	case 0x01:
		return "-Class/+iSerial";
	case 0x02:
		return "+Class/-iSerial";
	case 0x03:
		return "+Class/+iSerial";
	}
	return "Unknown";
}

static int s2245_get_errorlog(struct sinfonia_usbdev *usbh)
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

		if ((ret = sinfonia_docmd(usbh,
					  (uint8_t*)&cmd, sizeof(cmd),
					  (uint8_t*)&resp, sizeof(resp),
					  &num))) {
			return ret;
		}

		if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct s6245_errorlog_resp) - sizeof(struct sinfonia_status_hdr)))
			return -2;

		INFO("Stored Error ID %d:\n", i);
		INFO("%08u prints : 0x%02x/0x%02x (%s)\n",
		     le32_to_cpu(resp.print_counter),
		     resp.error_major, resp.error_minor,
		     usbh->error_codes(resp.error_major, resp.error_minor));
		/*
		INFO(" %04d-%02u-%02u %02u:%02u:%02u @ %08u prints : 0x%02x/0x%02x (%s)\n",
		     resp.time_year + 2000, resp.time_month, resp.time_day,
		     resp.time_hour, resp.time_min, resp.time_sec,
		     le32_to_cpu(resp.print_counter),
		     resp.error_major, resp.error_minor,
		     usbh->error_codes(resp.error_major, resp.error_minor));
		INFO("  Temp: %02u/%02u Hum: %02u\n",
		     resp.printer_thermistor, resp.head_thermistor, resp.printer_humidity);
		*/
	} while (++i < le16_to_cpu(resp.error_count));

	return CUPS_BACKEND_OK;
}

static void shinkos6145_cmdline(void)
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
	DEBUG("\t\t[ -q filename ]  # Extract eeprom data\n");
	DEBUG("\t\t[ -Q filename ]  # Extract image correction params\n");
	DEBUG("\t\t[ -r ]           # Reset user/NV tone curve\n");
	DEBUG("\t\t[ -R ]           # Reset printer to factory defaults\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -X jobid ]     # Abort a printjob\n");
//	DEBUG("\t\t[ -Z 0 | 1 ]     # Dump all parameters\n");
}

static int shinkos6145_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct shinkos6145_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "c:C:eFik:l:L:mr:Q:q:rR:sX:Z:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'c':
			j = sinfonia_gettonecurve(&ctx->dev, TONECURVE_USER, optarg);
			break;
		case 'C':
			j = sinfonia_settonecurve(&ctx->dev, UPDATE_TARGET_TONE_USER, optarg);
			break;
		case 'e':
			if (ctx->is_2245) {
				j = s2245_get_errorlog(&ctx->dev);
			} else {
				j = sinfonia_geterrorlog(&ctx->dev);
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
			if (i <= 5)
				i = 0;
			else if (i <= 15)
				i = 1;
			else if (i <= 30)
				i = 2;
			else if (i <= 60)
				i = 3;
			else if (i <= 120)
				i = 4;
			else if (i <= 240)
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
			dump_mediainfo(&ctx->media, ctx->is_card);
			break;
		case 'q':
			j = shinkos6145_dump_eeprom(ctx, optarg);
			break;
		case 'Q':
			j = shinkos6145_dump_corrdata(ctx, optarg);
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

static void *shinkos6145_init(void)
{
	struct shinkos6145_ctx *ctx = malloc(sizeof(struct shinkos6145_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct shinkos6145_ctx));

	DL_INIT();

	return ctx;
}

static int shinkos6145_attach(void *vctx, struct dyesub_connection *conn, uint8_t jobid)
{
	struct shinkos6145_ctx *ctx = vctx;

	ctx->dev.conn = conn;

	/* Mark 2245-class */
	if (conn->type == P_SHINKO_S2245 ||
	    conn->type == P_KODAK_6900) {
		ctx->is_2245 = 1;
	}

	if (ctx->is_2245) {
		ctx->dev.params = s2245_params;
		ctx->dev.params_count = s2245_params_num;
		ctx->dev.error_codes = &s2245_error_codes;

#if defined(WITH_DYNAMIC)
		DEBUG("Attempting to load S2245 image processing library\n");
		ctx->dl_handle = DL_OPEN(LIB2245_NAME); /* Try the Sinfonia one first */
		if (!ctx->dl_handle)
			ctx->dl_handle = DL_OPEN(LIB2245_NAME_RE); /* Then the RE one */
		if (ctx->dl_handle) {
			ctx->DumpAnnounce = DL_SYM(ctx->dl_handle, "dump_announce");
			ctx->ip_imageProc = DL_SYM(ctx->dl_handle, "ip_imageProc");
			ctx->ip_checkIpp = DL_SYM(ctx->dl_handle, "ip_checkIpp");
			ctx->ip_getMemorySize = DL_SYM(ctx->dl_handle, "ip_getMemorySize");
			if (!ctx->ip_imageProc || !ctx->ip_checkIpp || !ctx->ip_getMemorySize) {
				WARNING("Problem resolving symbols in imaging processing library\n");
				DL_CLOSE(ctx->dl_handle);
				ctx->dl_handle = NULL;
			} else {
				DEBUG("Image processing library successfully loaded\n");
				if (!stats_only && ctx->DumpAnnounce)
					ctx->DumpAnnounce(logger);
			}
		}
#endif
	} else if (conn->type == P_SHINKO_S6145 ||
		   conn->type == P_SHINKO_S6145D) {
		ctx->dev.params = s6145_params;
		ctx->dev.params_count = s6145_params_num;
		ctx->dev.error_codes = &s6145_error_codes;

#if defined(WITH_DYNAMIC)
		DEBUG("Attempting to load S6145 image processing library\n");
		ctx->dl_handle = DL_OPEN(LIB6145_NAME); /* Try the Sinfonia one first */
		if (!ctx->dl_handle)
			ctx->dl_handle = DL_OPEN(LIB6145_NAME_RE); /* Then the RE one */
		if (ctx->dl_handle) {
			ctx->DumpAnnounce = DL_SYM(ctx->dl_handle, "dump_announce");
			ctx->ImageProcessing = DL_SYM(ctx->dl_handle, "ImageProcessing");
			ctx->ImageAvrCalc = DL_SYM(ctx->dl_handle, "ImageAvrCalc");
			if (!ctx->ImageProcessing || !ctx->ImageAvrCalc) {
				WARNING("Problem resolving symbols in imaging processing library\n");
				DL_CLOSE(ctx->dl_handle);
				ctx->dl_handle = NULL;
			} else {
				DEBUG("Image processing library successfully loaded\n");
				if (!stats_only && ctx->DumpAnnounce)
					ctx->DumpAnnounce(logger);
			}
		}
#endif
	}

#if defined(WITH_DYNAMIC)
	if (!ctx->dl_handle)
		WARNING("Image processing library not found; will NOT be able to print!\n");
#else
	WARNING("Image processing library cannot be loaded; will NOT be able to print!e\n");
#endif

	/* Ensure jobid is sane */
	ctx->jobid = (jobid & 0x7f);
	if (!ctx->jobid)
		ctx->jobid++;

	/* Query Media */
	if (test_mode < TEST_MODE_NOATTACH) {
		int ret = sinfonia_query_media(&ctx->dev,
					       &ctx->media);
		if (ret)
			return ret;
	} else {
		int media_code = RIBBON_6x8;
		if (getenv("MEDIA_CODE"))
			media_code = atoi(getenv("MEDIA_CODE"));

		ctx->media.ribbon_code = media_code;
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = print_ribbons(ctx->media.ribbon_code, ctx->is_card);
	ctx->marker.numtype = ctx->media.ribbon_code;
	ctx->marker.levelmax = ribbon_sizes(ctx->media.ribbon_code, ctx->is_card, ctx->is_2245);
	ctx->marker.levelnow = CUPS_MARKER_UNKNOWN;

	return CUPS_BACKEND_OK;
}

static void shinkos6145_teardown(void *vctx) {
	struct shinkos6145_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->eeprom)
		free(ctx->eeprom);
	if (ctx->corrdata)
		free(ctx->corrdata);
	if (ctx->dl_handle)
		DL_CLOSE(ctx->dl_handle);

	DL_EXIT();

	free(ctx);
}

static int shinkos6145_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct shinkos6145_ctx *ctx = vctx;
	struct sinfonia_printjob *job = NULL;
	int ret;
	int model;
	uint8_t input_ymc;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->dev.conn->type == P_SHINKO_S6145 ||
	    ctx->dev.conn->type == P_SHINKO_S6145D)
		model = 6145;
	else
		model = 2245;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));

	/* Common read/parse code */
	if (ctx->dev.conn->type == P_KODAK_6900) {
		ret = sinfonia_raw28_read_parse(data_fd, job);
	} else {
		ret = sinfonia_read_parse(data_fd, model, job);
	}
	if (ret) {
		free(job);
		return ret;
	}

	/* Use whicever copy count is larger */
	if (job->common.copies < copies)
		job->common.copies = copies;

	/* S6145 can only combine 2* 4x6 -> 8x6.
	   2x6 strips and 3.5x5 -> 5x7 can't.
	   S2245 can combine 2x6 strips too!
	*/
	if (job->jp.columns == 1844 &&
	    job->jp.rows == 1240 &&
	    (ctx->media.ribbon_code == RIBBON_6x8 ||
	     ctx->media.ribbon_code == RIBBON_6x9)) {

		if (model == 6145 && job->jp.method == PRINT_METHOD_STD)
			job->common.can_combine = 1;
		else if (model == 2245)
			job->common.can_combine = 1;
	}

	/* Extended spool format to re-purpose an unused header field.
	   When bit 0 is set, this tells the backend that the data is
	   already in planar YMC format (vs packed RGB) so we don't need
	   to do the conversion ourselves.  Saves some processing overhead */
	input_ymc = job->jp.ext_flags & EXT_FLAG_PLANARYMC;

	/* Convert packed RGB to planar YMC if necessary */
	if (!ctx->is_2245 && !input_ymc) {
		INFO("Converting Packed RGB to Planar YMC\n");
		int planelen = job->jp.columns * job->jp.rows;
		uint8_t *databuf3 = malloc(job->datalen);
		int i;
		if (!databuf3) {
			ERROR("Memory allocation failure!\n");
			sinfonia_cleanup_job(job);
			return CUPS_BACKEND_RETRY_CURRENT;
		}
		for (i = 0 ; i < planelen ; i++) {
			uint8_t r, g, b;
			r = job->databuf[3*i];
			g = job->databuf[3*i+1];
			b = job->databuf[3*i+2];
			databuf3[i] = 255 - b;
			databuf3[planelen + i] = 255 - g;
			databuf3[planelen + planelen + i] = 255 - r;
		}
		free(job->databuf);
		job->databuf = databuf3;
	}

	*vjob = job;

	return CUPS_BACKEND_OK;
}

#define JOB_EQUIV(__x)  if (job1->__x != job2->__x) goto done

static void *shinkos6145_combine_jobs(const void *vjob1,
				      const void *vjob2)
{
	const struct sinfonia_printjob *job1 = vjob1;
	const struct sinfonia_printjob *job2 = vjob2;
	struct sinfonia_printjob *newjob = NULL;

	uint16_t newrows;
	uint16_t newpad;

	if (!job1 || !job2)
		goto done;

	/* Make sure we're okay to proceed */
	JOB_EQUIV(jp.columns);
	JOB_EQUIV(jp.rows);
	JOB_EQUIV(jp.oc_mode);
	JOB_EQUIV(jp.method);
	JOB_EQUIV(jp.media);
	JOB_EQUIV(jp.quality);

	switch (job1->jp.rows) {
	case 1240:  /* 4x6 */
		newrows = 2492;
		newpad = 12;
		break;
	default:
		/* All other sizes, we can't combine */
		goto done;
	}

        newjob = malloc(sizeof(*newjob));
        if (!newjob) {
                ERROR("Memory allocation failure!\n");
                goto done;
        }
        memcpy(newjob, job1, sizeof(*newjob));

	newjob->jp.rows = newrows;
	newjob->jp.media = CODE_6x8;

	if (job1->jp.method == PRINT_METHOD_SPLIT) /* 4x6-div2 -> 8x6-div4 */
		newjob->jp.method = PRINT_METHOD_COMBO_4;
	else /* 4x6 -> 8x6-div2 */
		newjob->jp.method = PRINT_METHOD_SPLIT;

	/* Allocate new buffer */
	newjob->databuf = malloc(newjob->jp.rows * newjob->jp.columns * 3);
	newjob->datalen = 0;
	if (!newjob->databuf) {
		sinfonia_cleanup_job(newjob);
		newjob = NULL;
		goto done;
	}

	/* Copy Planar YMC payload into new buffer! */
	memcpy(newjob->databuf + newjob->datalen,
	       job1->databuf,
	       job1->jp.rows * job1->jp.columns);
	newjob->datalen += job1->jp.rows * job1->jp.columns;
	memset(newjob->databuf + newjob->datalen, 0xff, newpad * newjob->jp.columns);
	memcpy(newjob->databuf + newjob->datalen,
	       job2->databuf,
	       job2->jp.rows * job2->jp.columns);
	newjob->datalen += job2->jp.rows * job2->jp.columns;

	memcpy(newjob->databuf + newjob->datalen,
	       job1->databuf + (job1->jp.rows * job1->jp.columns),
	       job1->jp.rows * job1->jp.columns);
	newjob->datalen += job1->jp.rows * job1->jp.columns;
	memset(newjob->databuf + newjob->datalen, 0xff, newpad * newjob->jp.columns);
	memcpy(newjob->databuf + newjob->datalen,
	       job2->databuf + (job2->jp.rows * job2->jp.columns),
	       job2->jp.rows * job2->jp.columns);
	newjob->datalen += job2->jp.rows * job2->jp.columns;

	memcpy(newjob->databuf + newjob->datalen,
	       job1->databuf + 2*(job1->jp.rows * job1->jp.columns),
	       job1->jp.rows * job1->jp.columns);
	newjob->datalen += job1->jp.rows * job1->jp.columns;
	memset(newjob->databuf + newjob->datalen, 0xff, newpad * newjob->jp.columns);
	memcpy(newjob->databuf + newjob->datalen,
	       job2->databuf + 2*(job2->jp.rows * job2->jp.columns),
	       job2->jp.rows * job2->jp.columns);
	newjob->datalen += job2->jp.rows * job2->jp.columns;

done:
	return newjob;
}

static int shinkos6145_main_loop(void *vctx, const void *vjob, int wait_for_return) {
	struct shinkos6145_ctx *ctx = vctx;

	int ret, num;

	int i, last_state = -1, state = S_IDLE;

	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_status_resp sts, sts2;

	uint32_t cur_mode;

	struct sinfonia_printjob *job = (struct sinfonia_printjob*) vjob;

	if (!job)
		return CUPS_BACKEND_FAILED;

	/* Validate print sizes */
	for (i = 0; i < ctx->media.count ; i++) {
		/* Look for matching media */
		if (ctx->media.items[i].columns == job->jp.columns &&
		    ctx->media.items[i].rows == job->jp.rows &&
		    ctx->media.items[i].method == job->jp.method &&
		    ctx->media.items[i].code == job->jp.media)
			break;
	}
	if (i == ctx->media.count) {
		ERROR("Incorrect media loaded for print!\n");
		return CUPS_BACKEND_HOLD;
	}

	// XXX check copies against remaining media?

	if (!ctx->is_2245) {
		/* Query printer mode */
		ret = sinfonia_getparam(&ctx->dev, PARAM_OC_PRINT, &cur_mode);
		if (ret) {
			ERROR("Failed to execute command\n");
			return ret;
		}
	}

	/* Send Set Time */
	if (ctx->is_2245) {
		struct sinfonia_settime_cmd settime;
		time_t now = time(NULL);
		struct tm *cur = localtime(&now);

		memset(&settime, 0, sizeof(settime));
		settime.hdr.cmd = cpu_to_le16(SINFONIA_CMD_SETTIME);
		settime.hdr.len = cpu_to_le16(sizeof(settime)-sizeof(settime.hdr));
		settime.enable = 1;
		settime.second = cur->tm_sec;
		settime.minute = cur->tm_min;
		settime.hour = cur->tm_hour;
		settime.day = cur->tm_mday;
		settime.month = cur->tm_mon;
		settime.year = cur->tm_year + 1900 - 2000;

		if ((ret = sinfonia_docmd(&ctx->dev,
					  (uint8_t*)&settime, sizeof(settime),
					  (uint8_t*)&sts, sizeof(sts),
					  &num))) {
			return CUPS_BACKEND_FAILED;
		}
		if (sts.hdr.result != RESULT_SUCCESS)
			return CUPS_BACKEND_FAILED;
	}


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
				  &num))) {
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
		if (sts.hdr.status == ERROR_PRINTER)
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
		/* If either bank is free, continue */
		if (sts.bank1_status == BANK_STATUS_FREE ||
		    sts.bank2_status == BANK_STATUS_FREE)
			state = S_PRINTER_READY_CMD;

		break;
	case S_PRINTER_READY_CMD: {
		/* Set matte/etc */
		uint32_t oc_mode = job->jp.oc_mode;
		uint32_t updated = 0;

		if (ctx->is_2245) {
			oc_mode = (job->jp.oc_mode & SINFONIA_PRINT28_OC_MASK) | (job->jp.quality ? SINFONIA_PRINT28_OPTIONS_HQ : 0);
			if (!ctx->corrdata ||
			    ctx->corrdatalen <= S2245_CORRDATA_HEADER_MODE_OFFSET ||
			    ((uint8_t*)ctx->corrdata)[S2245_CORRDATA_HEADER_MODE_OFFSET] != oc_mode)
				updated = 1;
		} else {
			if (!oc_mode) /* if nothing set, default to glossy */
				oc_mode = PARAM_OC_PRINT_GLOSS;

			if (cur_mode != oc_mode) {
				/* If cur_mode is not the same as desired oc_mode,
				   change it -- but we have to wait until the printer
				   is COMPLETELY idle */
				if (sts.bank1_status != BANK_STATUS_FREE ||
				    sts.bank2_status != BANK_STATUS_FREE) {
					INFO("Need to switch overcoat mode, waiting for printer idle\n");
					sleep(1);
					goto top;
				}

				ret = sinfonia_setparam(&ctx->dev, PARAM_OC_PRINT, oc_mode);
				if (ret) {
					ERROR("Failed to execute command\n");
					return ret;
				}
				updated = 1;
			}
		}

		ret = shinkos6145_get_eeprom(ctx);
		if (ret) {
			ERROR("Failed to execute command\n");
			return ret;
		}

		/* Get image correction parameters if necessary */
		if (updated || !ctx->corrdata || !ctx->corrdatalen) {
			if (ctx->is_2245) {
				ret = shinkos2245_get_imagecorr(ctx, oc_mode);
			} else {
				ret = shinkos6145_get_imagecorr(ctx);
			}
		}
		if (ret) {
			ERROR("Failed to execute command\n");
			return ret;
		}

		if (ctx->dl_handle) {
			INFO("Calling image processing library...\n");
		} else {
			ERROR("Image processing library not found!  Cannot print!\n");
			return CUPS_BACKEND_FAILED;
		}

		if (ctx->is_2245) {
			uint32_t bufSize = 0;
			uint16_t *newbuf;

			if (!ctx->ip_checkIpp(job->jp.columns, job->jp.rows, ctx->corrdata)) {
				ERROR("ip_checkIPP Failed!\n");
				return CUPS_BACKEND_FAILED;
			}
			if (!ctx->ip_getMemorySize(&bufSize, job->jp.columns, job->jp.rows, ctx->corrdata)) {
				ERROR("ip_getMemorySize Failed!\n");
				return CUPS_BACKEND_FAILED;
			}
			newbuf = malloc(bufSize);
			if (!newbuf) {
				ERROR("Memory Allocation failure!\n");
				return CUPS_BACKEND_RETRY;
			}
			if (!ctx->ip_imageProc(newbuf, job->databuf, job->jp.columns, job->jp.rows, ctx->corrdata)) {
				ERROR("ip_imageProc Failed!\n");
				free(newbuf);
				return CUPS_BACKEND_FAILED;
			}
			free(job->databuf);
			job->databuf = (uint8_t*)newbuf;
			job->datalen = bufSize;
		} else {
			uint16_t tmp;
			memcpy(&tmp, (uint8_t*)ctx->corrdata + S6145_CORRDATA_HEADDOTS_OFFSET, sizeof(tmp));
			tmp = le16_to_cpu(tmp);

			/* Set up library transform... */
			uint32_t newlen = tmp * job->jp.rows * sizeof(uint16_t) * 4;
			uint16_t *databuf2 = malloc(newlen);
			if (!databuf2) {
				ERROR("Memory Allocation failure!\n");
				return CUPS_BACKEND_RETRY;
			}
			/* Set the size in the correctiondata */
			tmp = cpu_to_le16(job->jp.columns);
			memcpy((uint8_t*)ctx->corrdata + S6145_CORRDATA_WIDTH_OFFSET, &tmp, sizeof(tmp));
			tmp = cpu_to_le16(job->jp.rows);
			memcpy((uint8_t*)ctx->corrdata + S6145_CORRDATA_HEIGHT_OFFSET, &tmp, sizeof(tmp));

			/* Perform the actual library transform */
			if (ctx->ImageAvrCalc(job->databuf, job->jp.columns, job->jp.rows, ctx->image_avg)) {
				free(databuf2);
				ERROR("Library returned error!\n");
				return CUPS_BACKEND_FAILED;
			}
			ctx->ImageProcessing(job->databuf, databuf2, ctx->corrdata);

			free(job->databuf);
			job->databuf = (uint8_t*) databuf2;
			job->datalen = newlen;
		}

		INFO("Sending print job (internal id %u)\n", ctx->jobid);

		if (!ctx->is_2245) {
			struct s6145_print_cmd print;
			memset(&print, 0, sizeof(print));
			print.hdr.cmd = cpu_to_le16(SINFONIA_CMD_PRINTJOB);
			print.hdr.len = cpu_to_le16(sizeof (print) - sizeof(cmd));

			print.id = ctx->jobid;
			print.count = cpu_to_le16(job->common.copies);
			print.columns = cpu_to_le16(job->jp.columns);
			print.rows = cpu_to_le16(job->jp.rows);
			print.image_avg = ctx->image_avg[2]; /* Cyan level */
			print.method = cpu_to_le32(job->jp.method);
			print.combo_wait = 0;

			/* Brava21 header has a few quirks */
			if(ctx->dev.conn->type == P_SHINKO_S6145D) {
				print.media = job->jp.media;
				print.unk_1 = 0x01;
			}
			if ((ret = sinfonia_docmd(&ctx->dev,
						  (uint8_t*)&print, sizeof(print),
						  (uint8_t*)&sts, sizeof(sts),
						  &num))) {
				return ret;
			}
		} else {
			struct sinfonia_printcmd28_hdr print;
			memset(&print, 0, sizeof(print));
			print.hdr.cmd = cpu_to_le16(SINFONIA_CMD_PRINTJOB);
			print.hdr.len = cpu_to_le16(sizeof (print) - sizeof(cmd));
			print.jobid = ctx->jobid;
			print.copies = cpu_to_le16(job->common.copies);
			print.columns = cpu_to_le16(job->jp.columns);
			print.rows = cpu_to_le16(job->jp.rows);
			print.options = job->jp.oc_mode & 0x3;
			if (job->jp.quality)
				print.options |= 0x08;
			print.media = 0;  /* ignore job->jp.media! */

			print.ipp = SINFONIA_PRINT28_IPP_CONTOUR; // XXX make configurable?
			print.method = cpu_to_le32(job->jp.method | SINFONIA_PRINT28_METHOD_ERR_RECOVERY | SINFONIA_PRINT28_METHOD_PREHEAT);

			if ((ret = sinfonia_docmd(&ctx->dev,
						  (uint8_t*)&print, sizeof(print),
						  (uint8_t*)&sts, sizeof(sts),
						  &num))) {
				return ret;
			}
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
		// XXX we shouldn't send the lamination layer over if
		// it's not needed.  hdr->oc_mode == PRINT_MODE_NO_OC
		if ((ret = send_data(ctx->dev.conn,
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
	return CUPS_BACKEND_FAILED;
}

static int shinkos6145_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct shinkos6145_ctx *ctx = vctx;
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_status_resp sts;
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

	ctx->marker.levelnow = le32_to_cpu(sts.count_ribbon_left);

	if (markers) *markers = &ctx->marker;
	if (count) *count = 1;

	return CUPS_BACKEND_OK;
}

static int shinkos6145_query_stats(void *vctx,  struct printerstats *stats)
{
	struct shinkos6145_ctx *ctx = vctx;
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_status_resp status;
	int num;

	if (shinkos6145_query_markers(ctx, NULL, NULL))
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
	case P_SHINKO_S6145:
		stats->mfg = "Sinfonia";
		stats->model = "CS2 / S6145";
		break;
	case P_SHINKO_S6145D:
		stats->mfg = "Ciaat";
		stats->model = "Brava 21";
		break;
	case P_KODAK_6900:
		stats->mfg = "Kodak";
		if (ctx->dev.conn->usb_pid == 0x0003) {
			stats->model = "6900";
		} else if (ctx->dev.conn->usb_pid == 0x0004) {
			stats->model = "6950";
		}
		break;
	case P_SHINKO_S2245:
		if (ctx->dev.conn->usb_pid == 0x0010) {
			stats->mfg = "HiTi";
			stats->model = "M610";
		} else if (ctx->dev.conn->usb_pid == 0x0039) {
			stats->mfg = "Sinfonia";
			stats->model = "S3 / S2245";
		} else {
			stats->mfg = "Unknown";
			stats->model = "Unknown";
		}
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

static const char *shinkos6145_prefixes[] = {
	"shinkos6145", /* Family Name */
	// backwards-compatiblity
	"brava21",
	NULL
};

const struct dyesub_backend shinkos6145_backend = {
	.name = "Shinko/Sinfonia CHC-S6145/CS2/S2245/S3",
	.version = "0.49" " (lib " LIBSINFONIA_VER ")",
	.uri_prefixes = shinkos6145_prefixes,
	.cmdline_usage = shinkos6145_cmdline,
	.cmdline_arg = shinkos6145_cmdline_arg,
	.init = shinkos6145_init,
	.attach = shinkos6145_attach,
	.teardown = shinkos6145_teardown,
	.cleanup_job = sinfonia_cleanup_job,
	.read_parse = shinkos6145_read_parse,
	.main_loop = shinkos6145_main_loop,
	.query_serno = sinfonia_query_serno,
	.query_markers = shinkos6145_query_markers,
	.query_stats = shinkos6145_query_stats,
	.combine_jobs = shinkos6145_combine_jobs,
	.devices = {
		{ 0x10ce, 0x0019, P_SHINKO_S6145, NULL, "sinfonia-chcs6145"},
		{ 0x10ce, 0x0019, P_SHINKO_S6145, NULL, "shinko-chcs6145"}, /* Duplicate */
		{ 0x10ce, 0x001e, P_SHINKO_S6145D, NULL, "ciaat-brava-21"},
		{ 0x10ce, 0x0039, P_SHINKO_S2245, NULL, "sinfonia-chcs2245"},
		{ 0x29cc, 0x0003, P_KODAK_6900, NULL, "kodak-6900"},      /* aka CHC-S2245-6A */
		{ 0x29cc, 0x0004, P_KODAK_6900, NULL, "kodak-6950"},      /* aka CHC-S2245-6C */
		{ 0x0d16, 0x0010, P_SHINKO_S2245, NULL, "hiti-m610"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* CHC-S6145 spool file format

  Spool file consists of an 116-byte header, followed by RGB-packed data,
  followed by a 4-byte footer.  Header appears to consist of a series of
  4-byte Little Endian words.

   10 00 00 00 MM MM 00 00  HH 00 00 00 01 00 00 00  MM == Model (ie 6145d), HH == 0x02 (5" media), 0x03 (6" media)
   64 00 00 00 00 00 00 00  TT 00 00 00 00 00 00 00  TT == 0x08 5x5, 0x03 5x7, 0x07 2x6, 0x00 4x6, 0x06 6x6/6x6+6x2/6x8
   UU 00 00 00 ZZ 00 00 00  XX 00 00 00 00 00 00 00  XX == 0x00 default, 0x02 glossy, 0x03 matte, ZZ == 0x00 default, 0x01 == std qual; UU == 0x00 normal, 0x04 2x6*2, 0x05 6x6+2x6
   00 00 00 00 WW WW 00 00  HH HH 00 00 NN 00 00 00  WW/HH Width, Height (LE), NN == Copies
   00 00 00 00 00 00 00 00  00 00 00 00 ce ff ff ff
   00 00 00 00 ce ff ff ff  QQ QQ 00 00 ce ff ff ff  QQ == DPI (300)
   00 00 00 00 ce ff ff ff  00 00 00 00 00 00 00 00
   00 00 00 00

   [[Packed RGB payload of WW*HH*3 bytes]]

   04 03 02 01  [[ footer ]]

  * CHC-S2245 spool file format

   10 00 00 00 c5 08 00 00  MM MM MM MM 01 00 00 00   [ hdr len 16 ]
   64 00 00 00 00 00 00 00  ZZ ZZ ZZ ZZ 00 00 00 00   [ hdr len 100 ]
   XX XX XX XX QQ QQ QQ QQ  GG GG GG GG 00 00 00 00
   00 00 00 00 CC CC CC CC  RR RR RR RR NN NN NN NN
   00 00 00 00 00 00 00 00  00 00 00 00 ce ff ff ff
   00 00 00 00 ce ff ff ff  DD DD DD DD ce ff ff ff
   00 00 00 00 ce ff ff ff  00 00 00 00 00 00 00 00
   00 00 00 00

   [[Packed RGB payload of WW*HH*3 bytes]]

   04 03 02 01  [[ footer ]]

   CC == columns (1844)
   RR == rows    (1240 = 4x6, 2434 = 8x6, 1824 = 6x6, 634 = 2x6)
   DD == dpi     (300 fixed)
   NN == copies  (1-900)
   GG == overcoat (1 = gloss, 2 = matte)
   QQ == quality (0 = standard, 1 = high)
   MM == media? (1 = 8x6, 0 = 4x6)
   XX == cut?  (04 = 4x6-div2, 05 = 8x6-div2, 00 = normal)
   ZZ == method (00 = 4x6, 08 = 6x6, 07 = 2x6, 06 = 8x6)

*/
