/*
 *   Shinko/Sinfonia CHC-S6145 CUPS backend -- libusb-1.0 version
 *
 *   (c) 2015-2019 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Low-level documentation was provided by Sinfonia.  Thank you!
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
 *   An additional permission is granted, under the GPLv2 section 10, to combine
 *   and/or redistribute this program with the proprietary libS6145ImageProcess
 *   library, providing you have *written permission* from Sinfonia Technology
 *   Co. LTD to use and/or redistribute that library.
 *
 *   You must still adhere to all other terms of the license to this program
 *   (ie GPLv2) and the license of the libS6145ImageProcess library.
 *
 *   SPDX-License-Identifier: GPL-2.0+ with special exception
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

/* For Integration into gutenprint */
#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#if defined(USE_DLOPEN)
#define WITH_DYNAMIC
#include <dlfcn.h>
#define DL_INIT() do {} while(0)
#define DL_OPEN(__x) dlopen(__x, RTLD_NOW)
#define DL_SYM(__x, __y) dlsym(__x, __y)
#define DL_CLOSE(__x) dlclose(__x)
#define DL_EXIT() do {} while(0)
#elif defined(USE_LTDL)
#define WITH_DYNAMIC
#include <ltdl.h>
#define DL_INIT() lt_dlinit()
#define DL_OPEN(__x) lt_dlopen(__x)
#define DL_SYM(__x, __y) lt_dlsym(__x, __y)
#define DL_CLOSE(__x) do {} while(0)
#define DL_EXIT() lt_dlexit()
#else
#define DL_INIT()     do {} while(0)
#define DL_CLOSE(__x) do {} while(0)
#define DL_EXIT()     do {} while(0)
#warning "No dynamic loading support!"
#endif

#define BACKEND shinkos6145_backend

#include "backend_common.h"
#include "backend_sinfonia.h"

/* Image processing library function prototypes */
typedef int (*ImageProcessingFN)(unsigned char *, unsigned short *, void *);
typedef int (*ImageAvrCalcFN)(unsigned char *, unsigned short, unsigned short, unsigned char *);

#define LIB_NAME    "libS6145ImageProcess.so"    // Official library
#define LIB_NAME_RE "libS6145ImageReProcess.so" // Reimplemented library

enum {
	S_IDLE = 0,
	S_PRINTER_READY_CMD,
	S_PRINTER_SENT_DATA,
	S_FINISHED,
};

/* "Image Correction Parameter" File */
// 128 bytes total, apparently an array of 32-bit values
struct tankParamTable {
	uint32_t trdTankSize;
	uint32_t sndTankSize;
	uint32_t fstTankSize;
	uint32_t trdTankIniEnergy;
	uint32_t sndTankIniEnergy;
	uint32_t fstTankIniEnergy;
	uint32_t trdTrdConductivity;
	uint32_t sndSndConductivity;
	uint32_t fstFstConductivity;
	uint32_t outTrdConductivity;
	uint32_t trdSndConductivity;
	uint32_t sndFstConductivity;
	uint32_t fstOutConductivity;
	uint32_t plusMaxEnergy;
	uint32_t minusMaxEnergy;
	uint32_t plusMaxEnergyPreRead;
	uint32_t minusMaxEnergyPreRead;
	uint32_t preReadLevelDiff;
	uint32_t rsvd[14]; // null?
} __attribute__((packed));

struct shinkos6145_correctionparam {
	uint16_t pulseTransTable_Y[256];   // @0
	uint16_t pulseTransTable_M[256];   // @512
	uint16_t pulseTransTable_C[256];   // @1024
	uint16_t pulseTransTable_O[256];   // @1536

	uint16_t lineHistCoefTable_Y[256]; // @2048
	uint16_t lineHistCoefTable_M[256]; // @2560
	uint16_t lineHistCoefTable_C[256]; // @3072
	uint16_t lineHistCoefTable_O[256]; // @3584

	uint16_t lineCorrectEnvA_Y;        // @4096
	uint16_t lineCorrectEnvA_M;        // @4098
	uint16_t lineCorrectEnvA_C;        // @4100
	uint16_t lineCorrectEnvA_O;        // @4102

	uint16_t lineCorrectEnvB_Y;        // @4104
	uint16_t lineCorrectEnvB_M;        // @4106
	uint16_t lineCorrectEnvB_C;        // @4108
	uint16_t lineCorrectEnvB_O;        // @4110

	uint16_t lineCorrectEnvC_Y;        // @4112
	uint16_t lineCorrectEnvC_M;        // @4114
	uint16_t lineCorrectEnvC_C;        // @4116
	uint16_t lineCorrectEnvC_O;        // @4118

	uint32_t lineCorrectSlice_Y;       // @4120
	uint32_t lineCorrectSlice_M;       // @4124
	uint32_t lineCorrectSlice_C;       // @4128
	uint32_t lineCorrectSlice_O;       // @4132

	uint32_t lineCorrectSlice1Line_Y;  // @4136
	uint32_t lineCorrectSlice1Line_M;  // @4140
	uint32_t lineCorrectSlice1Line_C;  // @4144
	uint32_t lineCorrectSlice1Line_O;  // @4148

	uint32_t lineCorrectPulseMax_Y;    // @4152 [array]
	uint32_t lineCorrectPulseMax_M;    // @4156 [array]
	uint32_t lineCorrectPulseMax_C;    // @4160 [array]
	uint32_t lineCorrectPulseMax_O;    // @4164 [array]

	struct tankParamTable tableTankParam_Y; // @4168
	struct tankParamTable tableTankParam_M; // @4296
	struct tankParamTable tableTankParam_C; // @4424
	struct tankParamTable tableTankParam_O; // @4552

	uint16_t tankPlusMaxEnergyTable_Y[256]; // @4680
	uint16_t tankPlusMaxEnergyTable_M[256]; // @5192
	uint16_t tankPlusMaxEnergyTable_C[256]; // @5704
	uint16_t tankPlusMaxEnergyTable_O[256]; // @6216

	uint16_t tankMinusMaxEnergy_Y[256];     // @6728
	uint16_t tankMinusMaxEnergy_M[256];     // @7240
	uint16_t tankMinusMaxEnergy_C[256];     // @7752
	uint16_t tankMinusMaxEnergy_O[256];     // @8264

	uint16_t printMaxPulse_Y; // @8776
	uint16_t printMaxPulse_M; // @8778
	uint16_t printMaxPulse_C; // @8780
	uint16_t printMaxPulse_O; // @8782

	uint16_t mtfWeightH_Y;    // @8784
	uint16_t mtfWeightH_M;    // @8786
	uint16_t mtfWeightH_C;    // @8788
	uint16_t mtfWeightH_O;    // @8790

	uint16_t mtfWeightV_Y;    // @8792
	uint16_t mtfWeightV_M;    // @8794
	uint16_t mtfWeightV_C;    // @8796
	uint16_t mtfWeightV_O;    // @8798

	uint16_t mtfSlice_Y;      // @8800
	uint16_t mtfSlice_M;      // @8802
	uint16_t mtfSlice_C;      // @8804
	uint16_t mtfSlice_O;      // @8806

	uint16_t val_1;           // @8808 // 1 enables linepreprintprocess
	uint16_t val_2;		  // @8810 // 1 enables ctankprocess
	uint16_t printOpLevel;    // @8812
	uint16_t matteMode;	  // @8814 // 1 for matte

	uint16_t randomBase[4];   // @8816 [use lower byte of each]

	uint16_t matteSize;       // @8824
	uint16_t matteGloss;      // @8826
	uint16_t matteDeglossBlk; // @8828
	uint16_t matteDeglossWht; // @8830

	uint16_t printSideOffset; // @8832
	uint16_t headDots;        // @8834 [always 0x0780, ie 1920. print width

	uint16_t SideEdgeCoefTable[128];   // @8836
	uint8_t  rsvd_2[256];              // @9092, null?
	uint16_t SideEdgeLvCoefTable[256]; // @9348
	uint8_t  rsvd_3[2572];             // @9860, null?

	/* User-supplied data */
	uint16_t width;           // @12432
	uint16_t height;          // @12434
	uint8_t  pad[3948];       // @12436, null.
} __attribute__((packed)); /* 16384 bytes */

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

#define PARAM_OC_PRINT     0x20
#define PARAM_PAPER_PRESV  0x3d
#define PARAM_DRIVER_MODE  0x3e
#define PARAM_PAPER_MODE   0x3f
#define PARAM_REGION_CODE  0x53 // Brava 21 only?
#define PARAM_SLEEP_TIME   0x54


#define PARAM_OC_PRINT_OFF   0x00000001
#define PARAM_OC_PRINT_GLOSS 0x00000002
#define PARAM_OC_PRINT_MATTE 0x00000003

#define PARAM_PAPER_PRESV_OFF 0x00000000
#define PARAM_PAPER_PRESV_ON  0x00000001

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

static const char *error_codes(uint8_t major, uint8_t minor)
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

struct s6145_status_resp {
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

struct s6145_geteeprom_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t data[256];
} __attribute__((packed));

#define RIBBON_NONE   0x00
#define RIBBON_4x6    0x01
#define RIBBON_3_5x5  0x02
#define RIBBON_5x7    0x03
#define RIBBON_6x8    0x04
#define RIBBON_6x9    0x05
// XXX what about 89xXXXmm ribbons?

static int ribbon_sizes (uint8_t v) {
	switch (v) {
	case RIBBON_4x6:
		return 300;
	case RIBBON_3_5x5:
		return 340;
	case RIBBON_5x7:
		return 170;
	case RIBBON_6x8:
		return 150;
	case RIBBON_6x9:
		return 130; // XXX guessed
	// XXX 89x??? rubbons.
	default:
		return 300; // don't want 0.
	}
}

static const char *print_ribbons (uint8_t v) {
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
	// XXX 89x??? ribbons.
	default:
		return "Unknown";
	}
}

struct s6145_mediainfo_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t  ribbon;
	uint8_t  reserved;
	uint8_t  count;
	struct sinfonia_mediainfo_item items[10];  /* Not all necessarily used */
} __attribute__((packed));

struct s6145_imagecorr_resp {
	struct sinfonia_status_hdr hdr;
	uint16_t total_size;
} __attribute__((packed));

struct s6145_imagecorr_data {
	uint8_t  remain_pkt;
	uint8_t  return_size;
	uint8_t  data[16];
} __attribute__((packed));

/* Private data structure */
struct shinkos6145_ctx {
	struct sinfonia_usbdev dev;

	uint8_t jobid;

	uint8_t image_avg[3]; /* CMY */

	struct marker marker;

	struct s6145_mediainfo_resp media;

	uint8_t *eeprom;
	size_t eepromlen;

	void *dl_handle;
	ImageProcessingFN ImageProcessing;
	ImageAvrCalcFN ImageAvrCalc;

	struct shinkos6145_correctionparam *corrdata;
	size_t corrdatalen;
};

static int shinkos6145_get_imagecorr(struct shinkos6145_ctx *ctx);
static int shinkos6145_get_eeprom(struct shinkos6145_ctx *ctx);

static int get_status(struct shinkos6145_ctx *ctx)
{
	struct sinfonia_cmd_hdr cmd;
	struct s6145_status_resp resp;
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
		     resp.hdr.printer_minor, error_codes(resp.hdr.printer_major, resp.hdr.printer_minor));
	}
	if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct s6145_status_resp) - sizeof(struct sinfonia_status_hdr)))
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
				  &num)) < 0) {
		return ret;
	}
	if (le16_to_cpu(resp2.hdr.payload_len) != (sizeof(struct sinfonia_getextcounter_resp) - sizeof(struct sinfonia_status_hdr)))
		return -1;

	INFO("Lifetime Distance:     %08u inches\n", le32_to_cpu(resp2.lifetime_distance));
	INFO("Maintenance Distance:  %08u inches\n", le32_to_cpu(resp2.maint_distance));
	INFO("Head Distance:         %08u inches\n", le32_to_cpu(resp2.head_distance));

	/* Query various params */
	if (ctx->dev.type == P_SHINKO_S6145D) {
		if ((ret = sinfonia_getparam(&ctx->dev, PARAM_REGION_CODE, &val))) {
			ERROR("Failed to execute command\n");
			return ret;
		}
		INFO("Region Code: %#x\n", val);

	}
	if ((ret = sinfonia_getparam(&ctx->dev, PARAM_PAPER_PRESV, &val))) {
		ERROR("Failed to execute command\n");
		return ret;
	}
	INFO("Paper Preserve mode: %s\n", (val ? "On" : "Off"));

	if ((ret = sinfonia_getparam(&ctx->dev, PARAM_DRIVER_MODE, &val))) {
		ERROR("Failed to execute command\n");
		return ret;
	}
	INFO("Driver mode:         %s\n", (val ? "On" : "Off"));

	if ((ret = sinfonia_getparam(&ctx->dev, PARAM_PAPER_MODE, &val))) {
		ERROR("Failed to execute command\n");
		return ret;
	}
	INFO("Paper load mode:     %s\n", (val ? "Cut" : "No Cut"));

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

	return 0;
}

static void dump_mediainfo(struct s6145_mediainfo_resp *resp)
{
	int i;

	INFO("Loaded Media Type:  %s\n", print_ribbons(resp->ribbon));
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

	ret = shinkos6145_get_imagecorr(ctx);
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

		ret = write(fd, ctx->corrdata, sizeof(struct shinkos6145_correctionparam));
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

	size_t total = 0;
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
				  &num)) < 0) {
		goto done;
	}

	ctx->corrdatalen = le16_to_cpu(resp.total_size);
	INFO("Fetching %zu bytes of image correction data\n", ctx->corrdatalen);

	ctx->corrdata = malloc(sizeof(struct shinkos6145_correctionparam));
	if (!ctx->corrdata) {
		ERROR("Memory allocation failure\n");
		ret = -ENOMEM;
		goto done;
	}
	memset(ctx->corrdata, 0, sizeof(struct shinkos6145_correctionparam));
	total = 0;

	while (total < ctx->corrdatalen) {
		struct s6145_imagecorr_data data;

		ret = read_data(ctx->dev.dev, ctx->dev.endp_up, (uint8_t *) &data,
				sizeof(data),
				&num);
		if (ret < 0)
			goto done;

		memcpy(((uint8_t*)ctx->corrdata) + total, data.data, sizeof(data.data));
		total += sizeof(data.data);

		if (data.remain_pkt == 0)
			DEBUG("correction block transferred (%zu/%zu total)\n", total, ctx->corrdatalen);

	}


done:
	return ret;
}

static int shinkos6145_get_eeprom(struct shinkos6145_ctx *ctx)
{
	struct sinfonia_cmd_hdr cmd;
	struct s6145_geteeprom_resp resp;

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
				  &num)) < 0) {
		goto done;
	}

	ctx->eepromlen = le16_to_cpu(resp.hdr.payload_len);
	ctx->eeprom = malloc(ctx->eepromlen);
	if (!ctx->eeprom) {
		ERROR("Memory allocation failure\n");
		ret = -ENOMEM;
		goto done;
	}
	memcpy(ctx->eeprom, resp.data, ctx->eepromlen);

done:
	return ret;
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
}

int shinkos6145_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct shinkos6145_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "c:C:eFik:l:L:mr:Q:q:rR:sX:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'c':
			j = sinfonia_gettonecurve(&ctx->dev, TONECURVE_USER, optarg);
			break;
		case 'C':
			j = sinfonia_settonecurve(&ctx->dev, TONECURVE_USER, optarg);
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
			j = sinfonia_settonecurve(&ctx->dev, TONECURVE_CURRENT, optarg);
			break;
		case 'm':
			dump_mediainfo(&ctx->media);
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
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
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

static int shinkos6145_attach(void *vctx, struct libusb_device_handle *dev, int type,
			      uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct shinkos6145_ctx *ctx = vctx;

	ctx->dev.dev = dev;
	ctx->dev.endp_up = endp_up;
	ctx->dev.endp_down = endp_down;
	ctx->dev.type = type;
	ctx->dev.error_codes = &error_codes;

	/* Attempt to open the library */
#if defined(WITH_DYNAMIC)
	INFO("Attempting to load image processing library\n");
	ctx->dl_handle = DL_OPEN(LIB_NAME); /* Try the Sinfonia one first */
	if (!ctx->dl_handle)
		ctx->dl_handle = DL_OPEN(LIB_NAME_RE); /* Then the RE one */
	if (!ctx->dl_handle)
		WARNING("Image processing library not found, using internal fallback code\n");
	if (ctx->dl_handle) {
		ctx->ImageProcessing = DL_SYM(ctx->dl_handle, "ImageProcessing");
		ctx->ImageAvrCalc = DL_SYM(ctx->dl_handle, "ImageAvrCalc");
		if (!ctx->ImageProcessing || !ctx->ImageAvrCalc) {
			WARNING("Problem resolving symbols in imaging processing library\n");
			DL_CLOSE(ctx->dl_handle);
			ctx->dl_handle = NULL;
		} else {
			INFO("Image processing library successfully loaded\n");
		}
	}
#else
	WARNING("Dynamic library support not enabled, using internal fallback code\n");
#endif

	/* Ensure jobid is sane */
	ctx->jobid = (jobid & 0x7f);
	if (!ctx->jobid)
		ctx->jobid++;

	if (test_mode < TEST_MODE_NOATTACH) {
		/* Query Media */
		struct sinfonia_cmd_hdr cmd;
		int num;

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
		int media_code = RIBBON_6x8;
		if (getenv("MEDIA_CODE"))
			media_code = atoi(getenv("MEDIA_CODE"));

		ctx->media.ribbon = media_code;
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.name = print_ribbons(ctx->media.ribbon);
	ctx->marker.levelmax = ribbon_sizes(ctx->media.ribbon);
	ctx->marker.levelnow = -2;

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

static void lib6145_calc_avg(struct shinkos6145_ctx *ctx,
			     const struct sinfonia_printjob *job,
			     uint16_t rows, uint16_t cols)
{
	uint32_t plane, i, planelen;
	planelen = rows * cols;

	for (plane = 0 ; plane < 3 ; plane++) {
		uint64_t sum = 0;

		for (i = 0 ; i < planelen ; i++) {
			sum += job->databuf[(planelen * plane) + i];
		}
		ctx->image_avg[plane] = (sum / planelen);
	}
}

static void lib6145_process_image(uint8_t *src, uint16_t *dest,
				  struct shinkos6145_correctionparam *corrdata,
				  uint8_t oc_mode)
{
	uint32_t in, out;

	uint16_t pad_l, pad_r, row_lim;
	uint16_t row, col;

	row_lim = le16_to_cpu(corrdata->headDots);
	pad_l = (row_lim - le16_to_cpu(corrdata->width)) / 2;
	pad_r = pad_l + le16_to_cpu(corrdata->width);
	out = 0;
	in = 0;

	/* Convert YMC 8-bit to 16-bit, and pad appropriately to full stripe */
	for (row = 0 ; row < le16_to_cpu(corrdata->height) ; row++) {
		for (col = 0 ; col < row_lim; col++) {
			uint16_t val;
			if (col < pad_l) {
				val = 0;
			} else if (col < pad_r) {
				val = corrdata->pulseTransTable_Y[src[in++]];
			} else {
				val = 0;
			}
			dest[out++] = val;
		}
	}
	for (row = 0 ; row < le16_to_cpu(corrdata->height) ; row++) {
		for (col = 0 ; col < row_lim; col++) {
			uint16_t val;
			if (col < pad_l) {
				val = 0;
			} else if (col < pad_r) {
				val = corrdata->pulseTransTable_M[src[in++]];
			} else {
				val = 0;
			}
			dest[out++] = val;
		}
	}
	for (row = 0 ; row < le16_to_cpu(corrdata->height) ; row++) {
		for (col = 0 ; col < row_lim; col++) {
			uint16_t val;
			if (col < pad_l) {
				val = 0;
			} else if (col < pad_r) {
				val = corrdata->pulseTransTable_C[src[in++]];
			} else {
				val = 0;
			}
			dest[out++] = val;
		}
	}

	/* Generate lamination plane, if desired */
	if (oc_mode > PRINT_MODE_NO_OC) {
		// XXX matters if we're using glossy/matte...
		for (row = 0 ; row < le16_to_cpu(corrdata->height) ; row++) {
			for (col = 0 ; col < row_lim; col++) {
				uint16_t val;
				if (col < pad_l) {
					val = 0;
				} else if (col < pad_r) {
					val = corrdata->pulseTransTable_O[corrdata->printOpLevel];
				} else {
					val = 0;
				}
				dest[out++] = val;
			}
		}
	}
}

static int shinkos6145_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct shinkos6145_ctx *ctx = vctx;
	struct sinfonia_printjob *job = NULL;
	int ret;
	int model;
	uint8_t input_ymc;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->dev.type == P_SHINKO_S6145 ||
	    ctx->dev.type == P_SHINKO_S6145D)
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
	if (ctx->dev.type == P_KODAK_6900) {
		ret = sinfonia_raw28_read_parse(data_fd, job);
	} else {
		ret = sinfonia_read_parse(data_fd, model, job);
	}
	if (ret) {
		free(job);
		return ret;
	}

	if (job->jp.copies > 1)
		job->copies = job->jp.copies;
	else
		job->copies = copies;

	/* Extended spool format to re-purpose an unused header field.
	   When bit 0 is set, this tells the backend that the data is
	   already in planar YMC format (vs packed RGB) so we don't need
	   to do the conversion ourselves.  Saves some processing overhead */
	input_ymc = job->jp.ext_flags & 0x01;

	/* Convert packed RGB to planar YMC if necessary */
	if (!input_ymc) {
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

	// if (job->copies > 1 && hdr->media == 0 && hdr->method == 0)
	// and if printer_media == 6x8 or 6x9
	// combine 4x6 + 4x6 -> 8x6
	// 1844x2492 = 1844x1240.. delta = 12.

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int shinkos6145_main_loop(void *vctx, const void *vjob) {
	struct shinkos6145_ctx *ctx = vctx;

	int ret, num;

	int i, last_state = -1, state = S_IDLE;

	struct sinfonia_cmd_hdr cmd;
	struct s6145_status_resp sts, sts2;

	uint32_t cur_mode;

	struct sinfonia_printjob *job = (struct sinfonia_printjob*) vjob; /* XXX stupid, we can't do this. */

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

	/* Query printer mode */
	ret = sinfonia_getparam(&ctx->dev, PARAM_OC_PRINT, &cur_mode);
	if (ret) {
		ERROR("Failed to execute command\n");
		return ret;
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
		if (sts.hdr.status == ERROR_PRINTER)
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
		if (sts.bank1_status == BANK_STATUS_FREE ||
		    sts.bank2_status == BANK_STATUS_FREE)
			state = S_PRINTER_READY_CMD;

		break;
	case S_PRINTER_READY_CMD: {
		/* Set matte/etc */

		uint32_t oc_mode = job->jp.oc_mode;
		uint32_t updated = 0;

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

		ret = shinkos6145_get_eeprom(ctx);
		if (ret) {
			ERROR("Failed to execute command\n");
			return ret;
		}

		/* Get image correction parameters if necessary */
		if (updated || !ctx->corrdata || !ctx->corrdatalen) {
			ret = shinkos6145_get_imagecorr(ctx);
			if (ret) {
				ERROR("Failed to execute command\n");
				return ret;
			}
		}

		/* Set up library transform... */
		uint32_t newlen = le16_to_cpu(ctx->corrdata->headDots) *
			job->jp.rows * sizeof(uint16_t) * 4;
		uint16_t *databuf2 = malloc(newlen);

		/* Set the size in the correctiondata */
		ctx->corrdata->width = cpu_to_le16(job->jp.columns);
		ctx->corrdata->height = cpu_to_le16(job->jp.rows);


		/* Perform the actual library transform */
		if (ctx->dl_handle) {
			INFO("Calling image processing library...\n");

			if (ctx->ImageAvrCalc(job->databuf, job->jp.columns, job->jp.rows, ctx->image_avg)) {
				free(databuf2);
				ERROR("Library returned error!\n");
				return CUPS_BACKEND_FAILED;
			}
			ctx->ImageProcessing(job->databuf, databuf2, ctx->corrdata);
		} else {
			WARNING("Utilizing fallback internal image processing code\n");
			WARNING(" *** Output quality will be poor! *** \n");

			lib6145_calc_avg(ctx, job, job->jp.columns, job->jp.rows);
			lib6145_process_image(job->databuf, databuf2, ctx->corrdata, oc_mode);
		}

		free(job->databuf);
		job->databuf = (uint8_t*) databuf2;
		job->datalen = newlen;

		struct s6145_print_cmd print;

		INFO("Sending print job (internal id %u)\n", ctx->jobid);

		memset(&print, 0, sizeof(print));
		print.hdr.cmd = cpu_to_le16(SINFONIA_CMD_PRINTJOB);
		print.hdr.len = cpu_to_le16(sizeof (print) - sizeof(cmd));

		print.id = ctx->jobid;
		print.count = cpu_to_le16(job->copies);
		print.columns = cpu_to_le16(job->jp.columns);
		print.rows = cpu_to_le16(job->jp.rows);
		print.image_avg = ctx->image_avg[2]; /* Cyan level */
		print.method = cpu_to_le32(job->jp.method);
		print.combo_wait = 0;

		/* Brava21 header has a few quirks */
		if(ctx->dev.type == P_SHINKO_S6145D) {
			print.media = job->jp.media;
			print.unk_1 = 0x01;
		}

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
		// XXX we shouldn't send the lamination layer over if
		// it's not needed.  hdr->oc_mode == PRINT_MODE_NO_OC
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
	return CUPS_BACKEND_FAILED;
}

static int shinkos6145_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
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

static int shinkos6145_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct shinkos6145_ctx *ctx = vctx;
	struct sinfonia_cmd_hdr cmd;
	struct s6145_status_resp sts;
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

	*markers = &ctx->marker;
	*count = 1;

	return CUPS_BACKEND_OK;
}

/* Exported */
#define USB_VID_SHINKO        0x10CE
#define USB_PID_SHINKO_S6145  0x0019
#define USB_PID_SHINKO_S6145D 0x001E /* Aka CIAAT Brava 21 */
#define USB_PID_SHINKO_S2245  0x0039
#define USB_VID_KODAK         0x040a
//#define USB_PID_KODAK_6900    0xXXXX /* Aka S2245-6A */
#define USB_VID_HITI          0x0D16
#define USB_PID_HITI_M610     0x0010

static const char *shinkos6145_prefixes[] = {
	"sinfonia-chcs6145", "ciaat-brava-21",
	"sinfonia-chcs2245", "hiti-m610", // "kodak-6900",
	// extras
	"shinko-chcs6145",
	// backwards-compatiblity
	"shinkos6145", "brava21",
	NULL
};

struct dyesub_backend shinkos6145_backend = {
	.name = "Shinko/Sinfonia CHC-S6145/CS2/S2245/S3",
	.version = "0.40" " (lib " LIBSINFONIA_VER ")",
	.uri_prefixes = shinkos6145_prefixes,
	.cmdline_usage = shinkos6145_cmdline,
	.cmdline_arg = shinkos6145_cmdline_arg,
	.init = shinkos6145_init,
	.attach = shinkos6145_attach,
	.teardown = shinkos6145_teardown,
	.cleanup_job = sinfonia_cleanup_job,
	.read_parse = shinkos6145_read_parse,
	.main_loop = shinkos6145_main_loop,
	.query_serno = shinkos6145_query_serno,
	.query_markers = shinkos6145_query_markers,
	.devices = {
		{ USB_VID_SHINKO, USB_PID_SHINKO_S6145, P_SHINKO_S6145, NULL, "sinfonia-chcs6145"},
		{ USB_VID_SHINKO, USB_PID_SHINKO_S6145D, P_SHINKO_S6145D, NULL, "ciaat-brava-21"},
		{ USB_VID_SHINKO, USB_PID_SHINKO_S2245, P_SHINKO_S2245, NULL, "sinfonia-chcs2245"},
//		{ USB_VID_KODAK, USB_PID_KODAK_6900, P_SHINKO_S2245, NULL, "sinfonia-chcs6145"},
		{ USB_VID_HITI, USB_PID_HITI_M610, P_SHINKO_S2245, NULL, "hiti-m610"},
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

*/
