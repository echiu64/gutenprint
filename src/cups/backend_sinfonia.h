 /*
 *   Shinko/Sinfonia Common Code
 *
 *   (c) 2019 Solomon Peachy <pizza@shaftnet.org>
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
 *   SPDX-License-Identifier: GPL-2.0+
 *
 */

#define LIBSINFONIA_VER "0.08"

#define SINFONIA_HDR1_LEN 0x10
#define SINFONIA_HDR2_LEN 0x64
#define SINFONIA_HDR_LEN (SINFONIA_HDR1_LEN + SINFONIA_HDR2_LEN)
#define SINFONIA_DPI 300

struct sinfonia_job_param {
	uint32_t columns;
	uint32_t rows;
	uint32_t copies;

	uint32_t method;
	uint32_t media;
	uint32_t oc_mode;

	uint32_t quality;

	int      mattedepth;
	uint32_t dust;

	uint32_t ext_flags;
};

struct sinfonia_printjob {
	struct sinfonia_job_param jp;

	uint8_t *databuf;
	int datalen;
	int copies;
};

int sinfonia_read_parse(int data_fd, uint32_t model,
			struct sinfonia_printjob *job);

int sinfonia_raw10_read_parse(int data_fd, struct sinfonia_printjob *job);
int sinfonia_raw18_read_parse(int data_fd, struct sinfonia_printjob *job);
int sinfonia_raw28_read_parse(int data_fd, struct sinfonia_printjob *job);
void sinfonia_cleanup_job(const void *vjob);

/* Common usb functions */
struct sinfonia_usbdev {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	char const *(*error_codes)(uint8_t major, uint8_t minor);
};
int sinfonia_docmd(struct sinfonia_usbdev *usbh,
		   uint8_t *cmd, int cmdlen,
		   uint8_t *resp, int resplen,
		   int *num);
int sinfonia_flashled(struct sinfonia_usbdev *usbh);
int sinfonia_canceljob(struct sinfonia_usbdev *usbh, int id);
int sinfonia_getparam(struct sinfonia_usbdev *usbh, int target, uint32_t *param);
int sinfonia_setparam(struct sinfonia_usbdev *usbh, int target, uint32_t param);
int sinfonia_getfwinfo(struct sinfonia_usbdev *usbh);
int sinfonia_geterrorlog(struct sinfonia_usbdev *usbh);
int sinfonia_resetcurve(struct sinfonia_usbdev *usbh, int target, int id);
int sinfonia_gettonecurve(struct sinfonia_usbdev *usbh, int type, char *fname);
int sinfonia_settonecurve(struct sinfonia_usbdev *usbh, int target, char *fname);

#define BANK_STATUS_FREE  0x00
#define BANK_STATUS_XFER  0x01
#define BANK_STATUS_FULL  0x02
#define BANK_STATUS_PRINTING  0x12  /* Not on S2145 */

const char *sinfonia_bank_statuses(uint8_t v);

#define UPDATE_TARGET_USER    0x03
#define UPDATE_TARGET_CURRENT 0x04

/* Update is three channels, Y, M, C;
   each is 256 entries of 11-bit data padded to 16-bits.
   Printer expects LE data.  We use BE data on disk.
*/
#define TONE_CURVE_SIZE 0x600
const char *sinfonia_update_targets (uint8_t v);

#define TONECURVE_INIT    0x00
#define TONECURVE_USER    0x01
#define TONECURVE_CURRENT 0x02

const char *sinfonia_tonecurve_statuses (uint8_t v);

struct sinfonia_error_item {
	uint8_t  major;
	uint8_t  minor;
	uint32_t print_counter;
} __attribute__((packed));

#define ERROR_NONE              0x00
#define ERROR_INVALID_PARAM     0x01
#define ERROR_MAIN_APP_INACTIVE 0x02
#define ERROR_COMMS_TIMEOUT     0x03
#define ERROR_MAINT_NEEDED      0x04
#define ERROR_BAD_COMMAND       0x05
#define ERROR_PRINTER           0x11
#define ERROR_BUFFER_FULL       0x21

const char *sinfonia_error_str(uint8_t v);

enum {
	MEDIA_TYPE_UNKNOWN = 0x00,
	MEDIA_TYPE_PAPER = 0x01,
};

const char *sinfonia_media_types(uint8_t v);

#define PRINT_MODE_NO_OC        0x01
#define PRINT_MODE_GLOSSY       0x02
#define PRINT_MODE_MATTE        0x03
const char *sinfonia_print_modes(uint8_t v);

#define PRINT_METHOD_STD     0x00
#define PRINT_METHOD_COMBO_2 0x02
#define PRINT_METHOD_COMBO_3 0x03 // S6245 only
#define PRINT_METHOD_SPLIT   0x04
#define PRINT_METHOD_DOUBLE  0x08 // S6145 only
#define PRINT_METHOD_DISABLE_ERR 0x10 // S6245 only
#define PRINT_METHOD_NOTRIM  0x80 // S6145 only

const char *sinfonia_print_methods (uint8_t v);

#define FWINFO_TARGET_MAIN_BOOT    0x01
#define FWINFO_TARGET_MAIN_APP     0x02
#define FWINFO_TARGET_PRINT_TABLES 0x03
#define FWINFO_TARGET_DSP          0x04
#define FWINFO_TARGET_USB          0x06
#define FWINFO_TARGET_PRINT_TABLES2 0x07

const char *sinfonia_fwinfo_targets (uint8_t v);

/* Common command structs */
struct sinfonia_cmd_hdr {
	uint16_t cmd;
	uint16_t len;  /* Not including this header */
} __attribute__((packed));

struct sinfonia_status_hdr {
	uint8_t  result;
	uint8_t  error;
	uint8_t  printer_major;
	uint8_t  printer_minor;
	uint8_t  reserved[2];
	uint8_t  mode;  /* S6245 and EK605 only, so far */
	uint8_t  status;
	uint16_t payload_len;
} __attribute__((packed));

struct sinfonia_cancel_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  id;
} __attribute__((packed));

struct sinfonia_fwinfo_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  target;
} __attribute__((packed));

struct sinfonia_fwinfo_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t  name[8];
	uint8_t  type[16];
	uint8_t  date[10];
	uint8_t  major;
	uint8_t  minor;
	uint16_t checksum;
} __attribute__((packed));

struct sinfonia_errorlog_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t  count;
	struct sinfonia_error_item items[10];  /* Not all necessarily used */
} __attribute__((packed));

struct sinfonia_mediainfo_item {
	uint8_t  code;
	uint16_t columns;
	uint16_t rows;
	uint8_t  type; /* S2145, EK68xx, EK605 only -- MEDIA_TYPE_* */
	uint8_t  method; /* PRINT_METHOD_* */
	uint8_t  flag;   /* EK68xx only */
	uint8_t  reserved[2];
} __attribute__((packed));

struct sinfonia_setparam_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t target;
	uint32_t param;
} __attribute__((packed));

struct sinfonia_diagnostic_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t arg3;
} __attribute__((packed));

struct sinfonia_getparam_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t target;
} __attribute__((packed));

struct sinfonia_getparam_resp {
	struct sinfonia_status_hdr hdr;
	uint32_t param;
} __attribute__((packed));

struct sinfonia_getprintidstatus_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t id;
} __attribute__((packed));

struct sinfonia_getprintidstatus_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t  id;
	uint16_t remaining;
	uint16_t finished;
	uint16_t specified;
	uint16_t status;
} __attribute__((packed));

#define IDSTATUS_WAITING   0x0000
#define IDSTATUS_PRINTING  0x0100
#define IDSTATUS_COMPLETED 0x0200
#define IDSTATUS_ERROR     0xFFFF

struct sinfonia_reset_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  target;
	uint8_t  curveid;
} __attribute__((packed));

#define RESET_PRINTER       0x03
#define RESET_TONE_CURVE    0x04

#define TONE_CURVE_ID       0x01

struct sinfonia_readtone_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  target;
	uint8_t  curveid;
} __attribute__((packed));

#define READ_TONE_CURVE_USER 0x01
#define READ_TONE_CURVE_CURR 0x02

struct sinfonia_readtone_resp {
	struct sinfonia_status_hdr hdr;
	uint16_t total_size;
} __attribute__((packed));

struct sinfonia_update_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  target;
	uint8_t  curve_id;
	uint8_t  reset; // ??
	uint8_t  reserved[3];
	uint32_t size;
} __attribute__((packed));

struct sinfonia_getserial_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t  data[8];
} __attribute__((packed));

struct sinfonia_getextcounter_resp {
	struct sinfonia_status_hdr hdr;
	uint32_t lifetime_distance;  /* Inches */
	uint32_t maint_distance;
	uint32_t head_distance;
	uint8_t  reserved[32];
} __attribute__((packed));

struct sinfonia_seteeprom_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t data[256]; /* Maxlen */
} __attribute__((packed));

struct sinfonia_printcmd10_hdr {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  jobid;
	uint16_t copies;
	uint16_t columns;
	uint16_t rows;
	uint8_t  media;
	uint8_t  oc_mode;
	uint8_t  method;
} __attribute__((packed));

struct sinfonia_printcmd18_hdr {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  jobid;
	uint16_t copies;
	uint16_t columns;
	uint16_t rows;
	uint8_t  reserved[8]; // columns and rows repeated, then nulls
	uint8_t  oc_mode;
	uint8_t  method;
	uint8_t  media; // reserved?
} __attribute__((packed));

struct sinfonia_printcmd28_hdr {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  jobid;
	uint16_t copies;
	uint16_t columns;
	uint16_t rows;
	uint8_t  media;
	uint8_t  reserved[7];
	uint8_t  options;
	uint8_t  method;
	uint8_t  reserved2[11];
} __attribute__((packed));

#define CODE_4x6     0x00
#define CODE_3_5x5   0x01
#define CODE_5x7     0x03
#define CODE_6x9     0x05
#define CODE_6x8     0x06
#define CODE_2x6     0x07
#define CODE_6x6     0x08

#define CODE_8x10    0x10
#define CODE_8x12    0x11
#define CODE_8x4     0x20
#define CODE_8x5     0x21
#define CODE_8x6     0x22
#define CODE_8x8     0x23
#define CODE_8x4_2   0x30
#define CODE_8x5_2   0x31
#define CODE_8x6_2   0x32
#define CODE_8x4_3   0x40

#define CODE_8x12K   0x02  /* Kodak 8810 */


#define CODE_89x60mm 0x10
#define CODE_89x59mm 0x11
#define CODE_89x58mm 0x12
#define CODE_89x57mm 0x13
#define CODE_89x56mm 0x14
#define CODE_89x55mm 0x15

const char *sinfonia_print_codes (uint8_t v, int eightinch);

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

const char *sinfonia_status_str(uint8_t v);

#define SINFONIA_CMD_GETSTATUS  0x0001
#define SINFONIA_CMD_MEDIAINFO  0x0002
#define SINFONIA_CMD_MODELNAME  0x0003 // 2145 only
#define SINFONIA_CMD_ERRORLOG   0x0004
#define SINFONIA_CMD_GETPARAM   0x0005 // !2145
#define SINFONIA_CMD_GETSERIAL  0x0006 // !2145
#define SINFONIA_CMD_PRINTSTAT  0x0007 // !2145
#define SINFONIA_CMD_EXTCOUNTER 0x0008 // !2145

#define SINFONIA_CMD_MEMORYBANK 0x000A // Brava 21 only?

#define SINFONIA_CMD_PRINTJOB   0x4001
#define SINFONIA_CMD_CANCELJOB  0x4002
#define SINFONIA_CMD_FLASHLED   0x4003
#define SINFONIA_CMD_RESET      0x4004
#define SINFONIA_CMD_READTONE   0x4005
#define SINFONIA_CMD_BUTTON     0x4006 // 2145 only
#define SINFONIA_CMD_SETPARAM   0x4007

#define SINFONIA_CMD_GETUNIQUE  0x8003 // 2145 only

#define SINFONIA_CMD_GETCORR    0x400D
#define SINFONIA_CMD_GETEEPROM  0x400E
#define SINFONIA_CMD_SETEEPROM  0x400F
#define SINFONIA_CMD_SETTIME    0x4011 // 6245 only

#define SINFONIA_CMD_DIAGNOSTIC 0xC001 // ??
#define SINFONIA_CMD_FWINFO     0xC003
#define SINFONIA_CMD_UPDATE     0xC004
#define SINFONIA_CMD_SETUNIQUE  0xC007 // 2145 only

const char *sinfonia_cmd_names(uint16_t v);

//#define KODAK6_MEDIA_5R      // 189-9160
#define KODAK6_MEDIA_6R   0x0b // 197-4096  [ Also: 101-0867, 141-9597, 659-9054, 169-6418, DNP-900-060 ]
#define KODAK6_MEDIA_UNK  0x03 // ??? reported but unknown
#define KODAK6_MEDIA_6TR2 0x2c // 396-2941
//#define KODAK6_MEDIA_5FR2    // 6900-compatible
//#define KODAK6_MEDIA_6FR2    // 6900-compatible, 102-5925
#define KODAK6_MEDIA_NONE 0x00
//#define KODAK7_MEDIA_5R      // 164-9011 137-0600
#define KODAK7_MEDIA_6R   0x29 // 659-9047 166-1925 396-2966 846-2004 103-7688 DNP-900-070 -- ALSO FUJI R68-D2P570 16578944
//#define KODAK7_MEDIA_6TA2
//#define KODAK7_MEDIA_5TA2

const char *kodak6_mediatypes(int type);
void kodak6_dumpmediacommon(int type);

#define RESULT_SUCCESS 0x01
#define RESULT_FAIL    0x02

/* ********** Below are for the old S1145 (EK68xx) and S1245 only! */

enum {
	CMD_CODE_OK = 1,
	CMD_CODE_BAD = 2,
};

enum {
        STATUS_PRINTING = 1,
        STATUS_IDLE = 2,
};

enum {
        STATE_STATUS1_STANDBY = 1,
        STATE_STATUS1_ERROR = 2,
        STATE_STATUS1_WAIT = 3,
};

#define STATE_STANDBY_STATUS2 0x0

enum {
        WAIT_STATUS2_INIT = 0,
        WAIT_STATUS2_RIBBON = 1,
        WAIT_STATUS2_THERMAL = 2,
        WAIT_STATUS2_OPERATING = 3,
        WAIT_STATUS2_BUSY = 4,
};

#define ERROR_STATUS2_CTRL_CIRCUIT   (0x80000000)
#define ERROR_STATUS2_MECHANISM_CTRL (0x40000000)
#define ERROR_STATUS2_SENSOR         (0x00002000)
#define ERROR_STATUS2_COVER_OPEN     (0x00001000)
#define ERROR_STATUS2_TEMP_SENSOR    (0x00000200)
#define ERROR_STATUS2_PAPER_JAM      (0x00000100)
#define ERROR_STATUS2_PAPER_EMPTY    (0x00000040)
#define ERROR_STATUS2_RIBBON_ERR     (0x00000010)

enum {
        CTRL_CIR_ERROR_EEPROM1  = 0x01,
        CTRL_CIR_ERROR_EEPROM2  = 0x02,
        CTRL_CIR_ERROR_DSP      = 0x04,
        CTRL_CIR_ERROR_CRC_MAIN = 0x06,
        CTRL_CIR_ERROR_DL_MAIN  = 0x07,
        CTRL_CIR_ERROR_CRC_DSP  = 0x08,
        CTRL_CIR_ERROR_DL_DSP   = 0x09,
        CTRL_CIR_ERROR_ASIC     = 0x0a,
        CTRL_CIR_ERROR_DRAM     = 0x0b,
        CTRL_CIR_ERROR_DSPCOMM  = 0x29,
};

enum {
        MECH_ERROR_HEAD_UP            = 0x01,
        MECH_ERROR_HEAD_DOWN          = 0x02,
        MECH_ERROR_MAIN_PINCH_UP      = 0x03,
        MECH_ERROR_MAIN_PINCH_DOWN    = 0x04,
        MECH_ERROR_SUB_PINCH_UP       = 0x05,
        MECH_ERROR_SUB_PINCH_DOWN     = 0x06,
        MECH_ERROR_FEEDIN_PINCH_UP    = 0x07,
        MECH_ERROR_FEEDIN_PINCH_DOWN  = 0x08,
        MECH_ERROR_FEEDOUT_PINCH_UP   = 0x09,
        MECH_ERROR_FEEDOUT_PINCH_DOWN = 0x0a,
        MECH_ERROR_CUTTER_LR          = 0x0b,
        MECH_ERROR_CUTTER_RL          = 0x0c,
};

enum {
        SENSOR_ERROR_CUTTER           = 0x05,
        SENSOR_ERROR_HEAD_DOWN        = 0x09,
        SENSOR_ERROR_HEAD_UP          = 0x0a,
        SENSOR_ERROR_MAIN_PINCH_DOWN  = 0x0b,
        SENSOR_ERROR_MAIN_PINCH_UP    = 0x0c,
        SENSOR_ERROR_FEED_PINCH_DOWN  = 0x0d,
        SENSOR_ERROR_FEED_PINCH_UP    = 0x0e,
        SENSOR_ERROR_EXIT_PINCH_DOWN  = 0x0f,
        SENSOR_ERROR_EXIT_PINCH_UP    = 0x10,
        SENSOR_ERROR_LEFT_CUTTER      = 0x11,
        SENSOR_ERROR_RIGHT_CUTTER     = 0x12,
        SENSOR_ERROR_CENTER_CUTTER    = 0x13,
        SENSOR_ERROR_UPPER_CUTTER     = 0x14,
        SENSOR_ERROR_PAPER_FEED_COVER = 0x15,
};

enum {
        TEMP_SENSOR_ERROR_HEAD_HIGH = 0x01,
        TEMP_SENSOR_ERROR_HEAD_LOW  = 0x02,
        TEMP_SENSOR_ERROR_ENV_HIGH  = 0x03,
        TEMP_SENSOR_ERROR_ENV_LOW   = 0x04,
};

enum {
        COVER_OPEN_ERROR_UPPER = 0x01,
        COVER_OPEN_ERROR_LOWER = 0x02,
};

enum {
        PAPER_EMPTY_ERROR = 0x00,
};

enum {
        RIBBON_ERROR = 0x00,
};

enum {
        CURVE_TABLE_STATUS_INITIAL = 0x00,
        CURVE_TABLE_STATUS_USERSET = 0x01,
        CURVE_TABLE_STATUS_CURRENT = 0x02,
};

const char *sinfonia_1x45_status_str(uint8_t status1, uint32_t status2, uint8_t error);
