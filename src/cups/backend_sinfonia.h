 /*
 *   Shinko/Sinfonia Common Code
 *
 *   (c) 2019-2021 Solomon Peachy <pizza@shaftnet.org>
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
 *   SPDX-License-Identifier: GPL-2.0+
 *
 */

#define LIBSINFONIA_VER "0.19"

#define SINFONIA_HDR1_LEN 0x10
#define SINFONIA_HDR2_LEN 0x64
#define SINFONIA_HDR_LEN (SINFONIA_HDR1_LEN + SINFONIA_HDR2_LEN)
#define SINFONIA_DPI 300

struct sinfonia_job_param {
	uint32_t columns;
	uint32_t rows;

	uint32_t method;
	uint32_t media;
	uint32_t oc_mode;

	uint32_t quality;

	int      mattedepth;
	uint32_t dust;

	uint32_t ext_flags;
};
#define EXT_FLAG_PLANARYMC  0x01
#define EXT_FLAG_BACKPRINT  0x02
#define EXT_FLAG_DOUBLESLUG 0x04

struct sinfonia_printjob {
	struct dyesub_job_common common;
	struct sinfonia_job_param jp;

	uint8_t *databuf;
	int datalen;
};

int sinfonia_read_parse(int data_fd, uint32_t model,
			struct sinfonia_printjob *job);

int sinfonia_raw10_read_parse(int data_fd, struct sinfonia_printjob *job);
int sinfonia_raw18_read_parse(int data_fd, struct sinfonia_printjob *job);
int sinfonia_raw28_read_parse(int data_fd, struct sinfonia_printjob *job);
void sinfonia_cleanup_job(const void *vjob);

int sinfonia_panorama_splitjob(struct sinfonia_printjob *injob,
                               uint16_t max_rows,
                               struct sinfonia_printjob **newjobs);

/* mapping param IDs to names */
struct sinfonia_param {
	const uint8_t id;
	const char *descr;
};

/* Known param IDs */
enum {
	PARAM_UNK_01      = 0x01, // 6145 = 0x32, 8810 = 0x01, 7000 = 0x01
	PARAM_UNK_02      = 0x02, // 6145 = 0xffffffff
	PARAM_UNK_03      = 0x02, // 6145 = 0xffffffff
	PARAM_UNK_04      = 0x04, // 6145 = 0xffffffff
	PARAM_UNK_05      = 0x05, // 6145 = 0xffffffff

	PARAM_UNK_10      = 0x10, // 2245 = 0x7b
	PARAM_UNK_11      = 0x11, // 2245 = 0x72, 8810 = 0x01, 7000 = 0x01
	PARAM_UNK_12      = 0x12, // 6145 = 0xffffffc4, 8810 = 0x69, 7000 = 0x69  (Matte Gloss?)
	PARAM_UNK_13      = 0x13, // 6145 = 0xffffffe2, 8810 = 0xc3, 7000 = 0xc3  (Matte Degloss Black?)
	PARAM_UNK_14      = 0x14, // 6145 = 0xffffffc7, 8810 = 0xcd, 7000 = 0xcd  (Matte Degloss White?)
	PARAM_UNK_15      = 0x15, // 6145 = 0xffffffec
	PARAM_UNK_16      = 0x16, // 6145 = 0x00

	PARAM_OC_PRINT    = 0x20, // 6145 ONLY
	PARAM_UNK_21      = 0x21, // 6145 = 0x69, 8810 = 0x3e8, 7000 = 0xe39 (Exit Speed with Sorter 4x6?)
	PARAM_UNK_22      = 0x22, // 6145 = 0xc3, 8810 = 0x41a, 7000 = 0x41a (Exit Speed with Sorter 8x6?)
	PARAM_UNK_23      = 0x23, // 6145 = 0xc3, 8810 = 0x152, 7000 = 0x152 (Exit Speed with Backprinting?)
	PARAM_UNK_24      = 0x24, // 8810 = 0x44c, 7000 = 0x44c (Exit Speed without PPAC 4x6?)
	PARAM_UNK_25      = 0x25, // 8810 = 0x44c, 7000 = 0x44c (Exit Speed without PPAC 8x6?)
	PARAM_UNK_2F      = 0x2f, // 8810 = 0x320, 7000 = 0x320

	PARAM_UNK_31      = 0x31, // 2245 = 0x64. 6145 = 0x68
	PARAM_UNK_32      = 0x32, // 2245 = 0x64
	PARAM_UNK_33      = 0x33, // 2245 = 0x64
	PARAM_UNK_34      = 0x34, // 2245 = 0x00
	PARAM_UNK_35      = 0x35, // 2245 = 0x00
	PARAM_UNK_36      = 0x36, // 2245 = 0x00
	PARAM_PAPER_PRESV = 0x3d, // 6145 ONLY
	PARAM_DRIVER_MODE = 0x3e, // 6145, 2245, 6245 ONLY
	PARAM_PAPER_MODE  = 0x3f, // 2245, 6245 ONLY

	PARAM_UNK_40      = 0x40, // 2245 = 0xff
	PARAM_UNK_41      = 0x41, // 2245 = 0x00, 8810 = 0x5d, 7000 = 0x6d
	PARAM_UNK_42      = 0x42, // 8810 = 0x48, 7000 = 0x51
	PARAM_UNK_43      = 0x43, // 8810 = 0x7c, 7000 = 0x3b
	PARAM_UNK_44      = 0x44, // 8810 = 0x88, 7000 = 0x82
	PARAM_UNK_45      = 0x45, // 8810 = 0x00, 7000 = 0x00
	PARAM_UNK_46      = 0x46, // 8810 = 0x02, 7000 = 0x00
	PARAM_UNK_47      = 0x47, // 8810 = 0x03, 7000 = 0x28
	PARAM_UNK_48      = 0x48, // 8810 = 0x08, 7000 = 0x02

	PARAM_REGION_CODE = 0x53, // 6145 ONLY
	PARAM_SLEEP_TIME  = 0x54, // 6145, 2245, 6245 ONLY
	PARAM_UNK_55      = 0x55, // 6145 = 0xff
	PARAM_UNK_56      = 0x56, // 6145 = 0xff
	PARAM_UNK_57      = 0x57, // 6145 = 0x00
	PARAM_UNK_58      = 0x58, // 6145 = 0xff

	PARAM_UNK_60      = 0x60, // 6145 = 0xa2
	PARAM_UNK_61      = 0x61, // 6145 = 0xa7, 8810 = 0x50
	PARAM_UNK_62      = 0x62, // 8810 = 0x31
	PARAM_UNK_63      = 0x63, // 8810 = 0x30
	PARAM_UNK_64      = 0x64, // 8810 = 0x30

	PARAM_UNK_70      = 0x70, // 2245 = 0x22f8, 6145 = 0x24ba
	PARAM_UNK_71      = 0x71, // 2245 = 0x01
	PARAM_UNK_72      = 0x72, // 6145 = 0x84
	PARAM_UNK_73      = 0x73, // 6145 = 0x8b
	PARAM_UNK_74      = 0x74, // 6145 = 0x89
	PARAM_UNK_75      = 0x75, // 6145 = 0x80

	PARAM_UNK_81      = 0x81, // 8810 = 0xffffffff, 7000 = 0xffffffff
	PARAM_UNK_82      = 0x82, // 8810 = 0xfffffff9, 7000 = 0xfffffffe
	PARAM_UNK_83      = 0x83, // 8810 = 0xfffffffc, 7000 = 0xffffffee
	PARAM_UNK_84      = 0x84, // 8810 = 0x02, 7000 = 0x01
	PARAM_UNK_8A      = 0x8a, // 8810 = 0x05
	PARAM_UNK_8B      = 0x8b, // 8810 = 0x05
	PARAM_UNK_8C      = 0x8c, // 8810 = 0x00
	PARAM_UNK_8D      = 0x8d, // 8810 = 0x00

	PARAM_UNK_91      = 0x91, // 2245 = 0xfffffffc, 6145 = 0xfffffffa, 8810 = 0x7e, 7000 = 0x6c
	PARAM_UNK_92      = 0x92, // 2245 = 0x00, 6145 = 0x06, 8810 = 0x7d, 7000 = 0x87
	PARAM_UNK_93      = 0x93, // 2245 = 0x06, 6145 = 0x06, 8810 = 0x77, 7000 = 0x67
	PARAM_UNK_94      = 0x94, // 7000 = 0x76

	PARAM_UNK_A0      = 0xa0, // 2245 = 0x01, 8810 = 0x05, 7000 = 0x05
	PARAM_UNK_A1      = 0xa1, // 2245 = 0xffffffff, 8810 = 0x00, 7000 = 0x00
	PARAM_UNK_A2      = 0xa2, // 2245 = 0xffffffff, 8810 = 0x08, 7000 = 0x10
	PARAM_UNK_A3      = 0xa3, // 2245 = 0xffffffff, 8810 = 0x30, 7000 = 0x3b
	PARAM_UNK_A4      = 0xa4, // 2245 = 0xffffffff, 8810 = 0x30, 7000 = 0x3b
	PARAM_UNK_A5      = 0xa5, // 2245 = 0x42, 6145 = 0x4d, 8810 = 0x46, 7000 = 0x3e (Thermal Protect Lamaination?)
	PARAM_UNK_A6      = 0xa6, // 2245 = 0x00, 6145 = 0x01, 8810 = 0x01, 7000 = 0x01
	PARAM_UNK_A7      = 0xa7, // 2245 = 0x01, 8810 = 0x14, 7000 = 0x14
	PARAM_UNK_A8      = 0xa8, // 2245 = 0x01, 6145 = 0x01, 8810 = 0x01, 7000 = 0x01
	PARAM_UNK_A9      = 0xa9, // 6145 = 0xffffffff, 8810 = 0xffffffff, 7000 = 0xffffffff

	PARAM_UNK_B0      = 0xb0, // 2245 = 0x1a/00   (VARIES?)
	PARAM_UNK_B1      = 0xb1, // 2245 = 0x70/79   (VARIES?)

	PARAM_UNK_C1      = 0xc1, // 8810 = 0x02, 7000 = 0x02
	PARAM_UNK_C2      = 0xc2, // 8810 = 0xc8, 7000 = 0xc8
	PARAM_UNK_C3      = 0xc3, // 8810 = 0xc8, 7000 = 0xc8
	PARAM_UNK_C4      = 0xc4, // 8810 = 0x4d0, 7000 = 0x200

	PARAM_UNK_D3      = 0xd3, // 6145 = 0x00
	PARAM_UNK_D4      = 0xd4, // 6145 = 0x00
	PARAM_UNK_D5      = 0xd5, // 6145 = 0xff
	PARAM_UNK_D6      = 0xd6, // 6145 = 0xff
	PARAM_UNK_D7      = 0xd7, // 6145 = 0x00
	PARAM_UNK_D8      = 0xd8, // 6145 = 0xff
	PARAM_UNK_DC      = 0xdc, // 2245 = 0x00
	PARAM_UNK_DD      = 0xdd, // 2245 = 0x0c
	PARAM_UNK_DE      = 0xde, // 2245 = 0x32
	PARAM_UNK_DF      = 0xdf, // 2245 = 0x00

	PARAM_UNK_E1      = 0xe1, // 2245 = 0x33/49, 6145 = 0x213e  (VARIES?)
	PARAM_UNK_E2      = 0xe2, // 2245 = 0x33/49, 6145 = 0x213e  (VARIES?)
	PARAM_UNK_E3      = 0xe3, // 6145 = 0x00
	PARAM_UNK_E4      = 0xe4, // 2245 = 0x78/ad, 6145 = 0x43ab    (VARIES?)
	PARAM_UNK_E5      = 0xe5, // 2245 = 0x33/49, 6145 = 0x213e    (VARIES?)
	PARAM_UNK_E6      = 0xe6, // 2245 = 0x0194/219, 6145 = 0x00   (VARIES?)
	PARAM_UNK_E7      = 0xe7, // 2245 = 0x0194/219, 6145 = 0x84f8 (VARIES?)
	PARAM_UNK_E8      = 0xe8, // 2245 = 0x00, 6145 = 0x84f8
	PARAM_UNK_E9      = 0xe9, // 2245 = 0x00, 6145 = 0x84f8
	PARAM_UNK_EA      = 0xea, // 2245 = 0x33/49, 6145 = 0x00  (VARIES?)
	PARAM_UNK_EB      = 0xeb, // 2245 = 0x0194/219    (VARIES?)

	PARAM_UNK_F1      = 0xf1, // 8810 = 0x22, 7000 = 0x68
	PARAM_UNK_F2      = 0xf2, // 8810 = 0x22, 7000 = 0x68
	PARAM_UNK_F3      = 0xf3, // 8810 = 0x47, 7000 = 0x94
	PARAM_UNK_F4      = 0xf4, // 8810 = 0x22, 7000 = 0x68
};

// S6145, S2245, S6245
#define PARAM_DRIVER_WIZOFF 0x00000000
#define PARAM_DRIVER_WIZON  0x00000001

// S6145, S2245, S6245
#define PARAM_PAPER_NOCUT   0x00000000
#define PARAM_PAPER_CUTLOAD 0x00000001

// S6145, S6245
#define PARAM_SLEEP_5MIN    0x00000000
#define PARAM_SLEEP_15MIN   0x00000001
#define PARAM_SLEEP_30MIN   0x00000002
#define PARAM_SLEEP_60MIN   0x00000003
#define PARAM_SLEEP_120MIN  0x00000004
#define PARAM_SLEEP_240MIN  0x00000005

/* Common usb functions */
struct sinfonia_usbdev {
	struct dyesub_connection *conn;

	const struct sinfonia_param *params;
	int params_count;

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
int sinfonia_button_set(struct sinfonia_usbdev *dev, int enable);

int sinfonia_query_serno(struct dyesub_connection *conn, char *buf, int buf_len);
int sinfonia_dumpallparams(struct sinfonia_usbdev *usbh, int known);
const char *sinfonia_paramname(struct sinfonia_usbdev *usbh, int id);

#define BANK_STATUS_FREE  0x00
#define BANK_STATUS_XFER  0x01
#define BANK_STATUS_FULL  0x02
#define BANK_STATUS_PRINTING  0x12  /* Not on S2145 */

const char *sinfonia_bank_statuses(uint8_t v);

#define UPDATE_TARGET_TONE_USER     0x03
#define UPDATE_TARGET_TONE_CURRENT  0x04
#define UPDATE_TARGET_LAM_USER 0x10
#define UPDATE_TARGET_LAM_DEF  0x11
#define UPDATE_TARGET_LAM_CUR  0x12

/* Update is three channels, Y, M, C;
   each is 256 entries of 11-bit data padded to 16-bits.
   Printer expects LE data.  We use BE data on disk.
*/
#define TONE_CURVE_SIZE (256*3)
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
#define ERROR_INAPP_COMMAND     0x05
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
#define PRINT_METHOD_COMBO_4 0x05 // S2245 only
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
	uint8_t  mode;  /* S2245, S6245 and EK605 only, so far */
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


struct sinfonia_errorlog2_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint16_t index;  /* 0 is latest */
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

struct sinfonia_6x45_mediainfo_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t ribbon_code;
	uint8_t reserved;
	uint8_t count;
	struct sinfonia_mediainfo_item items[10];  /* Not all necessarily used */
} __attribute__((packed));

/* resp needs to be at least sizeof(struct sinfonia_6x45_mediainfo_resp) */

int sinfonia_query_media(struct sinfonia_usbdev *usbh,
			 void *resp);

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

struct sinfonia_status_resp {
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

struct sinfonia_geteeprom_resp {
	struct sinfonia_status_hdr hdr;
	uint8_t data[256];
} __attribute__((packed));

struct sinfonia_button_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  enabled;
} __attribute__((packed));

#define BUTTON_ENABLED  0x01
#define BUTTON_DISABLED 0x00

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
	uint8_t  target;    // UPDATE_TARGET_TONE_*
	uint8_t  curve_id;  // 00 for lamination, 01 for tone?
	uint8_t  reset;     // ??
	uint8_t  reserved[3];
	uint32_t size;  // TONE_CURVE_SIZE or lamination data that is rows*cols bytes
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
	uint16_t columns2;
	uint16_t rows2;
	uint8_t  reserved[4]; // then nulls
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
	uint8_t  media;  /* always 0 on S2245 */
	uint8_t  reserved[7];
	uint8_t  options;
	uint8_t  method;
	uint8_t  ipp;
	uint8_t  reserved2[10];
} __attribute__((packed));

#define SINFONIA_PRINT28_OPTIONS_HQ   0x08
#define SINFONIA_PRINT28_OC_MASK      0x03
#define SINFONIA_PRINT28_OC_GLOSS     0x01
#define SINFONIA_PRINT28_OC_MATTE     0x02

#define SINFONIA_PRINT28_METHOD_PRINT_MASK   0x07
#define SINFONIA_PRINT28_METHOD_PRINT_COMBO  0x02
#define SINFONIA_PRINT28_METHOD_PRINT_SPLIT  0x04
#define SINFONIA_PRINT28_METHOD_PRINT_4SPLIT 0x05

#define SINFONIA_PRINT28_METHOD_ERR_RECOVERY 0x08
#define SINFONIA_PRINT28_METHOD_PREHEAT      0x10

#define SINFONIA_PRINT28_IPP_RESP     0x01
#define SINFONIA_PRINT28_IPP_CONTOUR  0x02

struct sinfonia_settime_cmd {
	struct sinfonia_cmd_hdr hdr;
	uint8_t enable;  /* 0 or 1 */
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint8_t year;
} __attribute__((packed));

struct kodak701x_backprint {
	struct sinfonia_cmd_hdr hdr;
	uint8_t unk_0;  // unknown.  maybe the line number?
	uint8_t null[6]; // always zero.
	uint8_t unk_1;  // length of text?  (max 40)
	uint8_t text[42]; //
} __attribute__((packed));

struct kodak8810_cutlist {
	struct sinfonia_cmd_hdr hdr;
	uint8_t  entries; /* max 24 */
	uint16_t cut[36]; /* LE, row number to cut at. Deltas must be >= 12. */
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

#define CODE_6x4K    0x01  /* Kodak 605 & 70xx */
#define CODE_6x8K    0x03
#define CODE_5x7K    0x06
#define CODE_5x4K    0x07
#define CODE_5x5K    0x08
#define CODE_5x7_5K  0x09
#define CODE_5x3_5K  0x0d
#define CODE_6x6K    0x0e

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
#define SINFONIA_CMD_MODELNAME  0x0003 // 2145
#define SINFONIA_CMD_ERRORLOG   0x0004
#define SINFONIA_CMD_GETPARAM   0x0005 // !2145
#define SINFONIA_CMD_GETSERIAL  0x0006 // !2145
#define SINFONIA_CMD_PRINTSTAT  0x0007 // !2145
#define SINFONIA_CMD_EXTCOUNTER 0x0008 // !2145

#define SINFONIA_CMD_MEMORYBANK 0x000A // 6145

#define SINFONIA_CMD_PRINTJOB   0x4001
#define SINFONIA_CMD_CANCELJOB  0x4002
#define SINFONIA_CMD_FLASHLED   0x4003
#define SINFONIA_CMD_RESET      0x4004
#define SINFONIA_CMD_READTONE   0x4005
#define SINFONIA_CMD_BUTTON     0x4006 // 2145?
#define SINFONIA_CMD_SETPARAM   0x4007 // !2145

#define SINFONIA_CMD_SETLAMSTR  0x4008 // EK70xx, EK8810? (len 28)
#define SINFONIA_CMD_COMMPPA    0x4009 // EK70xx
#define SINFONIA_CMD_SETCUTLIST 0x4009 // EK8810 (len 73, count + 36 entries, 16bit LE)
#define SINFONIA_CMD_SETPPAPARM 0x400A // EK70xx
#define SINFONIA_CMD_WAKEUPSDBY 0x400A // EK8810 (len 1)
#define SINFONIA_CMD_BACKPRINT  0x400B // EK701x only! (len 50)
#define SINFONIA_CMD_UNKNOWN4C  0x400C // EK8810, panorama setup?

#define SINFONIA_CMD_GETCORR    0x400D // 6145/2245
#define SINFONIA_CMD_GETEEPROM  0x400E // 6x45
#define SINFONIA_CMD_SETEEPROM  0x400F // 6x45

#define SINFONIA_CMD_SETTIME    0x4011 // 6245, 2245

#define SINFONIA_CMD_UNIVERSAL  0x4080 // EK70xx

#define SINFONIA_CMD_USBFWDL    0x8001 // EK70xx (len 5)
#define SINFONIA_CMD_MAINTPERM  0x8002 // EK70xx
#define SINFONIA_CMD_GETUNIQUE  0x8003 // 2145

#define SINFONIA_CMD_SELFDIAG   0xC001 // (len 3)
#define SINFONIA_CMD_DIAGRES    0xC002
#define SINFONIA_CMD_FWINFO     0xC003
#define SINFONIA_CMD_UPDATE     0xC004
#define SINFONIA_CMD_GETEEPROM2 0xC005 // EK70xx
#define SINFONIA_CMD_SETEEPROM2 0xC006 // EK70xx
#define SINFONIA_CMD_SETUNIQUE  0xC007 // 2145
#define SINFONIA_CMD_RESETERR   0xC008
#define SINFONIA_CMD_GETSERIAL2 0xC009 // EK70xx (len 8)

const char *sinfonia_cmd_names(uint16_t v);

#define KODAK6_MEDIA_5R   0xff //XX 189-9160
#define KODAK6_MEDIA_6R   0x0b // 197-4096  [ Also: 101-0867, 141-9597, 659-9054, 169-6418, DNP-900-060 ]
#define KODAK6_MEDIA_UNK  0x03 // ??? reported but unknown
#define KODAK6_MEDIA_6TR2 0x2c // 396-2941
//#define KODAK6_MEDIA_5FR2    // 6900-compatible
//#define KODAK6_MEDIA_6FR2    // 6900-compatible, 102-5925
#define KODAK6_MEDIA_NONE 0x00
#define KODAK7_MEDIA_5R   0xfe //XX 164-9011 137-0600
#define KODAK7_MEDIA_6R   0x29 // 659-9047 166-1925 396-2966 846-2004 103-7688 DNP-900-070 -- ALSO FUJI R68-D2P570 16578944
//#define KODAK7_MEDIA_6TA2
//#define KODAK7_MEDIA_5TA2

int kodak6_mediamax(int type);
const char *kodak6_mediatypes(int type);
void kodak6_dumpmediacommon(int type);

#define RESULT_SUCCESS 0x01
#define RESULT_FAIL    0x02

/* ********** Below are for the old S1145 (EK68xx) and S1245 only! */

enum {
	TONE_TABLE_STANDARD = 0,
	TONE_TABLE_USER = 1,
	TONE_TABLE_CURRENT = 2,
};
enum {
	PARAM_TABLE_NONE = 0,
	PARAM_TABLE_STANDARD = 1,
	PARAM_TABLE_FINE = 2,
};

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
