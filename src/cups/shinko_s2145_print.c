/*
 *   Shinko/Sinfonia CHC-S2145 CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013 Solomon Peachy <pizza@shaftnet.org>
 *
 *   Development of this backend was sponsored by:
 * 
 *     LiveLink Technology [ www.livelinktechnology.net ]
 * 
 *   The latest version of this program can be found at:
 *
 *     http://git.shaftnet.org/git/gitweb.cgi?p=selphy_print.git
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
	uint32_t model;  /* Fixed at '2145' (decimal) */
	uint32_t unk2;
	uint32_t unk3;

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


/*

   I have a list of 72 different errors that are displayed on the printer,
   but it appears the list is incomplete, and there's no mapping between
   category and major code numbers.  Also, not all of the individual errors
   have minor codes listed (particularly the "consumables")

   These are the observed error codes to date (via stored error log dumps):

   01/16 [ controller/parameter? ]
   05/15 [ jam/reloading? ]
   05/4e [ jam/unknown    ]
   05/4f [ jam/unknown?   ]
   05/61 [ jam/cantload?  ]
   05/62 [ jam/cantload?  ]
   05/64 [ jam/unknown?   ]
   06/01 [ "cover open"   ]
   06/0a [ consumables ?  ]
   06/0b [ consumables ?  ]

   Errors logged on printer A:

   0x01/0x16 @ 77845
   0x06/0x0b @ 77822, 70053
   0x05/0x64 @ 76034
   0x05/0x61 @ 76034, 75420
   0x05/0x62 @ 76034
   0x05/0x4e @ 69824, 69820, 69781

   Errors logged on printer B:

   0x06/0x0b @ 33270
   0x05/0x4e @ 32952, 27672
   0x05/0x4f @ 32935, 31834
   0x05/0x61 @ 30856, 27982
   0x01/0x16 @ 29132
   0x05/0x64 @ 27982
   0x05/0x62 @ 27982

   Errors logged on printer C:

   0x06/0x0a @ 78014, 77948, 77943, 77938 x2, 77937, 77936, 77933, 77919
   0x05/0x15 @ 77938


 */
static char *error_codes(uint8_t major, uint8_t minor)
{
	switch(major) {
	case 0x06:
		switch (minor) {
		case 0x01:
			return "Front Cover Open";
		default:
			return "Unknown";
		}
#if 0
	case 9: /* "Controller Error" */
		switch(minor) {
		case 0x01:
			return "Controller: 01 EEPROM";
		case 0x02:
			return "Controller: 02 EEPROM";
		case 0x04:
			return "Controller: 04 DSP";
		case 0x05:
			return "Controller: 05 DSP";
		case 0x06:
			return "Controller: 06 Main FW";
		case 0x07:
			return "Controller: 07 Main FW";
		case 0x08:
			return "Controller: 08 DSP FW";
		case 0x09:
			return "Controller: 09 DSP FW";
		case 0x0A:
			return "Controller: 0A ASIC";
		case 0x0B:
			return "Controller: 0B FPGA";
		case 0x0D:
			return "Controller: 0D Tone Curve";
		case 0x16:
			return "Controller: 16 Parameter Table";
		case 0x17:
			return "Controller: 17 Parameter Table";
		case 0x18:
			return "Controller: 18 Parameter Table";
		case 0x29:
			return "Controller: 29 DSP Comms";
		case 0x2A:
			return "Controller: 2A DSP Comms";
		default:
			return "Controller: Unknown";
		}
	case 8: /* XXXX "Mechanical Error" */
		switch (minor) {
		case 0x01:
			return "Mechanical: 01 Thermal Head";
		case 0x02:
			return "Mechanical: 02 Thermal Head";
		case 0x03:
			return "Mechanical: 03 Thermal Head";
		case 0x04:
			return "Mechanical: 04 Pinch Roller";
		case 0x05:
			return "Mechanical: 05 Pinch Roller";
		case 0x06:
			return "Mechanical: 06 Pinch Roller";
		case 0x07:
			return "Mechanical: 07 Pinch Roller";
		case 0x08:
			return "Mechanical: 08 Pinch Roller";
		case 0x09:
			return "Mechanical: 09 Cutter";
		case 0x0A:
			return "Mechanical: 0A Cutter";
		default:
			return "Mechanical: Unknown";
		}
	case 2: /* XXXX "Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Sensor: 01 Thermal Head";
		case 0x02:
			return "Sensor: 02 Pinch Roller";
		case 0x03:
			return "Sensor: 03 Cutter L";
		case 0x04:
			return "Sensor: 04 Cutter R";
		case 0x05:
			return "Sensor: 05 Cutter M";
		default:
			return "Sensor: Unknown";
		}
	case 3: /* XXXX "Temperature Sensor Error" */
		switch (minor) {
		case 0x01:
			return "Temp Sensor: 01 Thermal Head High";
		case 0x02:
			return "Temp Sensor: 02 Thermal Head Low";
		case 0x03:
			return "Temp Sensor: 03 Environment High";
		case 0x04:
			return "Temp Sensor: 04 Environment Low";
		case 0x05:
			return "Temp Sensor: 05 Warmup Timed Out";
		default:
			return "Temp Sensor: Unknown";
		}

	case 4: /* XXXX "Front Cover Open" */
		switch (minor) {
//		case 0x01:
//			return "Front Cover: 01 Cover Open";
		case 0x02:
			return "Front Cover: 02 Cover Open Error";
		default:
			return "Front Cover: Unknown";
		}
	case 5: /* XXX "Paper Jam" */
		switch (minor) {
		case 0x01:
			return "Paper Jam: 01 Loading";
		case 0x02:
			return "Paper Jam: 02 Loading";
		case 0x03:
			return "Paper Jam: 03 Loading";
		case 0x04:
			return "Paper Jam: 04 Loading";
		case 0x05:
			return "Paper Jam: 05 Loading";
		case 0x11:
			return "Paper Jam: 11 Reloading";
		case 0x12:
			return "Paper Jam: 12 Reloading";
		case 0x13:
			return "Paper Jam: 13 Reloading";
		case 0x14:
			return "Paper Jam: 14 Reloading";
		case 0x15:
			return "Paper Jam: 15 Reloading";
		case 0x16:
			return "Paper Jam: 16 Reloading";
		case 0x21:
			return "Paper Jam: 21 Takeup";
		case 0x22:
			return "Paper Jam: 22 Takeup";
		case 0x41:
			return "Paper Jam: 41 Printing";
		case 0x42:
			return "Paper Jam: 42 Printing";
		case 0x43:
			return "Paper Jam: 43 Printing";
		case 0x44:
			return "Paper Jam: 44 Printing";
		case 0x45:
			return "Paper Jam: 45 Printing";
		case 0x46:
			return "Paper Jam: 46 Printing";
		case 0x47:
			return "Paper Jam: 47 Printing";
		case 0x49:
			return "Paper Jam: 49 Printing";
		case 0x4A:
			return "Paper Jam: 4A Ribbon Cut";
		case 0x61:
			return "Paper Jam: 61 Can't Load";
		case 0x62:
			return "Paper Jam: 62 Can't Load";
		default:
			return "Paper Jam: Unknown";
		}
	case 6: /* XXXX "Consumables" */
		switch (minor) {
		case 0x01:  // XXX
			return "Consumables: XX No Ribbon+Paper";
		case 0x02:  // XXX
			return "Consumables: XX No Ribbon";
		case 0x03:  // XXX
			return "Consumables: XX Ribbon Empty";
		case 0x04:  // XXX
			return "Consumables: XX Ribbon Mismatch";
		case 0x05:  // XXX
			return "Consumables: XX 01 Ribbon Incorrect";
		case 0x06:  // XXX
			return "Consumables: XX 02 Ribbon Incorrect";
		case 0x07:  // XXX
			return "Consumables: XX 03 Ribbon Incorrect";
		case 0x08:  // XXX
			return "Consumables: XX No Paper";
		case 0x09:  // XXX
			return "Consumables: XX Paper Empty";
		case 0x0A:  // XXX
			return "Consumables: XX Paper Mismatch";
		default:
			return "Consumables: Unknown";
		}
#endif
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
	case 0:
		return "Free";
	case 1:
		return "Xfer";
	case 2:
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
	uint8_t  print_type;
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

static int s2145_do_cmd(libusb_device_handle *dev, 
			uint8_t endp_up, uint8_t endp_down,
			uint8_t *cmd, int cmdlen, int minlen, int *num)
{
	int ret;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;

	if ((ret = send_data(dev, endp_down,
			     cmd, cmdlen)))
		return -99;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   num,
				   5000);

	if (ret < 0 || (*num < minlen)) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, *num, minlen, endp_up);
		return ret;
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

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
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
	INFO("\tSince Paper Changed:\t%08d\n", le32_to_cpu(resp->count_paper));
	INFO("\tLifetime:\t\t%08d\n", le32_to_cpu(resp->count_lifetime));
	INFO("\tMaintainence:\t\t%08d\n", le32_to_cpu(resp->count_maint));
	INFO("\tPrint Head:\t\t%08d\n", le32_to_cpu(resp->count_head));
	INFO(" Cutter Actuations:\t%08d\n", le32_to_cpu(resp->count_cutter));
	INFO(" Ribbon Remaining:\t%08d\n", le32_to_cpu(resp->count_ribbon_left));
	INFO("Bank 1: 0x%02x (%s) Job %03d @ %03d/%03d (%03d remaining)\n",
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
	int ret, num = 0;
	int i;

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_FWINFO);
	cmd.hdr.len = cpu_to_le16(1);

	INFO("FW Information:\n");

	for (i = FWINFO_TARGET_MAIN_BOOT ; i <= FWINFO_TARGET_TABLES ; i++) {
		cmd.target = i;

		if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
					(uint8_t*)&cmd, sizeof(cmd),
					sizeof(*resp),
					&num)) < 0) {
			ERROR("Failed to execute %s command\n", cmd_names(cmd.hdr.cmd));
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

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
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
		INFO(" %02d: @ %08d prints : 0x%02x/0x%02x (%s)\n", i,
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

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
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

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
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

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
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

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
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

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
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

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
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

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
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
	uint16_t curves[768];

	int i,j;

	cmd.curveid = type;

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_READTONE);
	cmd.hdr.len = cpu_to_le16(1);

	INFO("Dump %s Tone Curve to '%s'\n", tonecurve_statuses(type), fname);

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.hdr.cmd));
		return ret;
	}

	resp->total_size = le16_to_cpu(resp->total_size);

	data = malloc(resp->total_size * 2);

	i = 0;
	while (i < resp->total_size) {
		ret = libusb_bulk_transfer(ctx->dev, ctx->endp_up,
					   data + i,
					   resp->total_size * 2 - i,
					   &num,
					   5000);

		if (ret < 0) {
			ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num + i, (int)resp->total_size, ctx->endp_up);
			return ret;
		}
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
		int tc_fd = open(fname, O_WRONLY|O_CREAT);
		if (tc_fd < 0)
			return -1;
		
		for (i = 0 ; i < 768; i++) {
			/* Byteswap appropriately */
			curves[i] = cpu_to_be16(le16_to_cpu(curves[i]));
			write(tc_fd, &curves[i], sizeof(uint16_t));
		}
		close(tc_fd);
	}

	free(data);
	return 0;
}

static int set_tonecurve(struct shinkos2145_ctx *ctx, int target, char *fname) 
{
	struct s2145_update_cmd cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	INFO("Set %s Tone Curve from '%s'\n", update_targets(target), fname);

	uint16_t *data = malloc(UPDATE_SIZE);

	/* Read in file */
	int tc_fd = open(fname, O_RDONLY);
	if (tc_fd < 0)
		return -1;
	if (read(tc_fd, data, UPDATE_SIZE) != UPDATE_SIZE)
		return -2;
	close(tc_fd);
	/* Byteswap data to local CPU.. */
	for (ret = 0; ret < UPDATE_SIZE ; ret+=2) {
		data[ret] = be16_to_cpu(data[ret]);
	}

	/* Set up command */
	cmd.target = target;
	cmd.reserved = 0;
	cmd.size = cpu_to_le32(UPDATE_SIZE);

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_UPDATE);
	cmd.hdr.len = cpu_to_le16(sizeof(struct s2145_update_cmd)-sizeof(cmd.hdr));

	/* Byteswap data to format printer is expecting.. */
	for (ret = 0; ret < UPDATE_SIZE ; ret+=2) {
		data[ret] = cpu_to_le16(data[ret]);
	}

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
				(uint8_t*)&cmd, sizeof(cmd),
				sizeof(*resp),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd.hdr.cmd));
		return ret;
	}

	/* Sent transfer */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t *) data, UPDATE_SIZE))) {
		return ret;
	}

	free(data);

	return 0;
}

static void shinkos2145_cmdline(char *caller)
{
	DEBUG("\t\t%s [ -qs | -qm | -qf | -qe | -qu ]\n", caller);
	DEBUG("\t\t%s [ -qtu filename | -qtc filename ]\n", caller);
	DEBUG("\t\t%s [ -su somestring | -stu filename | -stc filename ]\n", caller);
	DEBUG("\t\t%s [ -pc id | -fl | -ru | -rp | -b1 | -b0 ]\n", caller);
}

int shinkos2145_cmdline_arg(void *vctx, int run, char *arg1, char *arg2)
{
	struct shinkos2145_ctx *ctx = vctx;

	if (!run || !ctx)
		return (!strcmp("-qs", arg1) ||
			!strcmp("-qf", arg1) ||
			!strcmp("-qe", arg1) ||
			!strcmp("-qm", arg1) ||
			!strcmp("-qu", arg1) ||
			!strcmp("-qtc", arg1) ||
			!strcmp("-qtu", arg1) ||
			!strcmp("-pc", arg1) ||
			!strcmp("-fl", arg1) ||
			!strcmp("-ru", arg1) ||
			!strcmp("-rp", arg1) ||
			!strcmp("-b1", arg1) ||
			!strcmp("-b0", arg1) ||
			!strcmp("-stc", arg1) ||
			!strcmp("-stu", arg1) ||
			!strcmp("-su", arg1));

	if (!strcmp("-qs", arg1))
		get_status(ctx);
	else if (!strcmp("-qf", arg1))
		get_fwinfo(ctx);
	else if (!strcmp("-qe", arg1))
		get_errorlog(ctx);
	else if (!strcmp("-qm", arg1))
		get_mediainfo(ctx);
	else if (!strcmp("-qu", arg1))
		get_user_string(ctx);
	else if (!strcmp("-qtu", arg1))
		get_tonecurve(ctx, TONECURVE_USER, arg2);
	else if (!strcmp("-qtc", arg1))
		get_tonecurve(ctx, TONECURVE_CURRENT, arg2);
	else if (!strcmp("-su", arg1))
		set_user_string(ctx, arg2);
	else if (!strcmp("-stu", arg1))
		set_tonecurve(ctx, UPDATE_TARGET_USER, arg2);
	else if (!strcmp("-stc", arg1))
		set_tonecurve(ctx, UPDATE_TARGET_CURRENT, arg2);
	else if (!strcmp("-pc", arg1))
		cancel_job(ctx, arg2);
	else if (!strcmp("-fl", arg1))
		flash_led(ctx);
	else if (!strcmp("-ru", arg1))
		reset_curve(ctx, RESET_USER_CURVE);
	else if (!strcmp("-rp", arg1))
		reset_curve(ctx, RESET_PRINTER);
	else if (!strcmp("-b1", arg1))
		button_set(ctx, BUTTON_ENABLED);
	else if (!strcmp("-b0", arg1))
		button_set(ctx, BUTTON_DISABLED);

	return -1;
}

static void *shinkos2145_init(void)
{
	struct shinkos2145_ctx *ctx = malloc(sizeof(struct shinkos2145_ctx));
	if (!ctx)
		return NULL;
	memset(ctx, 0, sizeof(struct shinkos2145_ctx));

	return ctx;
}

static void shinkos2145_attach(void *vctx, struct libusb_device_handle *dev, 
			       uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct shinkos2145_ctx *ctx = vctx;


	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

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
		return 1;

	/* Read in then validate header */
	read(data_fd, &ctx->hdr, sizeof(ctx->hdr));
	if (le32_to_cpu(ctx->hdr.len1) != 0x10 ||
	    le32_to_cpu(ctx->hdr.model) != 2145 ||
	    le32_to_cpu(ctx->hdr.len2) != 0x64 ||
	    le32_to_cpu(ctx->hdr.dpi) != 300) {
		ERROR("Unrecognized header data format!\n");
		return 1;
	}

	ctx->datalen = le32_to_cpu(ctx->hdr.rows) * le32_to_cpu(ctx->hdr.columns) * 3;
	ctx->databuf = malloc(ctx->datalen);
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return 1;
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
	if (ret < 0 || ret != 4) {
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
		return 1;
	}

	return 0;
}

static int shinkos2145_main_loop(void *vctx, int copies) {
	struct shinkos2145_ctx *ctx = vctx;

	int i, ret, num;
	uint8_t cmdbuf[CMDBUF_LEN];
	uint8_t rdbuf2[READBACK_LEN];

	int last_state = -1, state = S_IDLE;

	struct s2145_cmd_hdr *cmd = (struct s2145_cmd_hdr *) cmdbuf;;
	struct s2145_print_cmd *print = (struct s2145_print_cmd *) cmdbuf;
	struct s2145_status_resp *sts = (struct s2145_status_resp *) rdbuf; 

top:
	if (state != last_state) {
		DEBUG("last_state %d new %d\n", last_state, state);
	}

	/* Send Status Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmd->cmd = cpu_to_le16(S2145_CMD_STATUS);
	cmd->len = cpu_to_le16(0);

	if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
				cmdbuf, sizeof(*cmd),
				sizeof(struct s2145_status_hdr),
				&num)) < 0) {
		ERROR("Failed to execute %s command\n", cmd_names(cmd->cmd));
		return ret;
	}

	if (memcmp(rdbuf, rdbuf2, READBACK_LEN)) {
		DEBUG("readback: ");
		for (i = 0 ; i < num ; i++) {
			DEBUG2("%02x ", rdbuf[i]);
		}
		DEBUG2("\n");
		memcpy(rdbuf2, rdbuf, READBACK_LEN);

		INFO("Printer Status: 0x%02x (%s)\n", 
		     sts->hdr.status, status_str(sts->hdr.status));
		if (sts->hdr.error == ERROR_PRINTER) {
			ERROR("Printer Reported Error: 0x%02x.0x%02x (%s)\n",
			      sts->hdr.printer_major, sts->hdr.printer_minor,
			      error_codes(sts->hdr.printer_major, sts->hdr.printer_minor));
		}
	} else if (state == last_state) {
		sleep(1);
	}
	last_state = state;

	fflush(stderr);       

	switch (state) {
	case S_IDLE:
		INFO("Waiting for printer idle\n");
		/* Basic error handling */
		if (sts->hdr.result != RESULT_SUCCESS)
			goto printer_error;
		if (sts->hdr.error != ERROR_NONE)
			goto printer_error;

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

		if ((ret = s2145_do_cmd(ctx->dev, ctx->endp_up, ctx->endp_down, 
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
			} else if (sts->hdr.status != ERROR_NONE)
				goto printer_error;
		}

		INFO("Sending image data to printer\n");
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ctx->databuf, ctx->datalen)))
			return ret;

		INFO("Waiting for printer to acknowledge completion\n");
		sleep(1);
		state = S_PRINTER_SENT_DATA;
		break;
	case S_PRINTER_SENT_DATA:
		if (sts->hdr.result != RESULT_SUCCESS)
			goto printer_error;
		if (sts->hdr.status == STATUS_READY ||
		    sts->hdr.status == STATUS_FINISHED)
			state = S_FINISHED;
		break;
	default:
		break;
	};

	if (state != S_FINISHED)
		goto top;
	
	/* This printer handles copies internally */
	copies = 1;

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d remaining)\n", copies - 1);

	if (copies && --copies) {
		state = S_IDLE;
		goto top;
	}

	return 0;

printer_error:
	ERROR("Printer reported error: %#x (%s) status: %#x (%s) -> %#x.%#x (%s)\n",
	      sts->hdr.error, 
	      error_str(sts->hdr.error),
	      sts->hdr.status, 
	      status_str(sts->hdr.status),
	      sts->hdr.printer_major, sts->hdr.printer_minor,
	      error_codes(sts->hdr.printer_major, sts->hdr.printer_minor));
	return 1;
}

static int shinkos2145_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct s2145_cmd_hdr cmd;
	struct s2145_getunique_resp *resp = (struct s2145_getunique_resp*) rdbuf;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(S2145_CMD_GETUNIQUE);
	cmd.len = cpu_to_le16(0);

	if ((ret = s2145_do_cmd(dev, endp_up, endp_down,
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

	return 0;
}

/* Exported */
#define USB_VID_SHINKO       0x10CE
#define USB_PID_SHINKO_S2145 0x000E

struct dyesub_backend shinkos2145_backend = {
	.name = "Shinko/Sinfonia CHC-S2145",
	.version = "0.20",
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
   64 00 00 00 00 00 00 00  TT 00 00 00 00 00 00 00  TT == Media Type
   MM 00 00 00 PP 00 00 00  00 00 00 00 00 00 00 00  PP = Print Mode, MM = Print Method
   00 00 00 00 WW WW 00 00  HH HH 00 00 XX 00 00 00  XX == Copies
   00 00 00 00 00 00 00 00  00 00 00 00 ce ff ff ff
   00 00 00 00 ce ff ff ff  QQ QQ 00 00 ce ff ff ff  QQ == DPI, ie 300.
   00 00 00 00 ce ff ff ff  00 00 00 00 00 00 00 00
   00 00 00 00 

   [[Packed RGB payload of WW*HH*3 bytes]]

   04 03 02 01  [[ footer ]]

  ************************************************************************

  The data format actually sent to the CHC-S2145 is different, but not
  radically so:

  

*/
