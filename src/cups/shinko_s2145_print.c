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

#define VERSION "0.11"
#define URI_PREFIX "shinkos2145://"

#include "backend_common.c"

/* USB Identifiers */
#define USB_VID_SHINKO       0x10CE
#define USB_PID_SHINKO_S2145 0x000E

enum {
	S_IDLE = 0,
	S_PRINTER_READY_CMD,
	S_PRINTER_SENT_PRINT_CMD,
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

static char *print_medias[] = {
	"4x6",
	"5x3.5",
	"Unused",
	"5x7",
	"Unused",
	"6x9",
	"6x8",
	"2x6"
};

#define PRINT_MODE_DEFAULT      0x01
#define PRINT_MODE_STD_GLOSSY   0x02
#define PRINT_MODE_FINE_GLOSSY  0x03
#define PRINT_MODE_STD_MATTE    0x04
#define PRINT_MODE_FINE_MATTE   0x05
#define PRINT_MODE_STD_EGLOSSY  0x06
#define PRINT_MODE_FINE_EGLOSSY 0x07

static char *print_modes[] = {
	"Unused",
	"Default",
	"Std Glossy",
	"Fine Glossy",
	"Std Matte",
	"Fine Matte",
	"Std ExGlossy",
	"Fine ExGlossy"
};

#define PRINT_METHOD_STD     0x00
#define PRINT_METHOD_4x6_2UP 0x02
#define PRINT_METHOD_2x6_2UP 0x04

static char *print_methods[] = { 
	"Standard",
	"Unused",
	"4x6 2up",
	"Unused",
	"2x6 2up",
};

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

static char *fwinfo_targets[] = {
	"Unused",
	"Main Boot",
	"Main App",
	"DSP Boot",
	"DSP App",
	"USB Boot",
	"USB App",
	"Tables"
};

struct s2145_update_cmd {
	struct s2145_cmd_hdr hdr;
	uint8_t  target;
	uint32_t reserved;
	uint32_t size;
} __attribute__((packed));

#define UPDATE_TARGET_USER    0x03
#define UPDATE_TARGET_CURRENT 0x04

static char *update_targets[] = {
	"Unused",
	"Unused",
	"Unused",
	"User",
	"Current",
};

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

/* XXX observed major/minor error codes:

   0x01/0x16 @ 77845  [maybe paper out?]
   0x06/0x0b @ 77822, 70053
   0x05/0x64 @ 76034
   0x05/0x61 @ 76034, 75420
   0x05/0x62 @ 76034
   0x05/0x4e @ 69824, 69820, 69781

   Unfortunately I have no idea what these refer to.

   major (0x01/5/6) may refer to above error codes or media/print modes.
   minor (0x61/62/64) may refer to equivalent status codes (see below)

 */

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
#define STATUS_PRINT_QP         0x69
#define STATUS_PAPER_CUT        0x6A
#define STATUS_PAPER_EJECT      0x6B
#define STATUS_BACK_FEED_E      0x6C
#define STATUS_FINISHED         0x6D

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

static char *bank_statuses[] = {
	"Free",
	"Xfer",
	"Full",
};

#define TONECURVE_INIT    0x00
#define TONECURVE_USER    0x01
#define TONECURVE_CURRENT 0x02

static char *tonecurve_statuses[] = {
	"Initial",
	"UserSet",
	"Current",
};

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

static char *media_types[] = {
	"Unknown",
	"Paper"
};

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

static int find_and_enumerate(struct libusb_context *ctx,
			      struct libusb_device ***list,
			      char *match_serno,
			      int scan_only)
{
	int num;
	int i;
	int found = -1;

	/* Enumerate and find suitable device */
	num = libusb_get_device_list(ctx, list);

	for (i = 0 ; i < num ; i++) {
		struct libusb_device_descriptor desc;

		libusb_get_device_descriptor((*list)[i], &desc);

		if (desc.idVendor != USB_VID_SHINKO)
			continue;

		switch(desc.idProduct) {
		case USB_PID_SHINKO_S2145:
			found = i;
			break;
		default:
			continue;
		}

		found = print_scan_output((*list)[i], &desc,
					  URI_PREFIX, "", 
					  found, (found == i), 1, 
					  scan_only, match_serno);
	}

	return found;
}

static int get_status(libusb_device_handle *dev, 
		      uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_cmd_hdr cmd;
	struct s2145_status_resp *resp = (struct s2145_status_resp *) rdbuf;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(S2145_CMD_STATUS);
	cmd.len = cpu_to_le16(0);

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, sizeof(cmd))))
		return -1;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}

	INFO("Printer Status:  0x%02x\n", resp->hdr.status);

	if (resp->hdr.result != RESULT_SUCCESS) {
		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->hdr.result, resp->hdr.error, resp->hdr.printer_major,
		     resp->hdr.printer_minor);
	}

	if (le16_to_cpu(resp->hdr.payload_len) != (sizeof(struct s2145_status_resp) - sizeof(struct s2145_status_hdr)))
		return 0;

	INFO(" Prints:\n");
        INFO("\tLifetime:\t\t%08d\n", le32_to_cpu(resp->count_lifetime));
	INFO("\tMaintainence:\t\t%08d\n", le32_to_cpu(resp->count_maint));
	INFO("\tSince Paper Changed:\t%08d\n", le32_to_cpu(resp->count_paper));
	INFO("\tCutter:\t\t\t%08d\n", le32_to_cpu(resp->count_cutter));
	INFO("\tPrint Head:\t\t%08d\n", le32_to_cpu(resp->count_head));
        INFO("\tRibbon Remaining:\t%08d\n", le32_to_cpu(resp->count_ribbon_left));
	INFO("Bank 1: 0x%02x (%s) Job %03d @ %03d/%03d (%03d remaining)\n",
	     resp->bank1_status, bank_statuses[resp->bank1_status],
	     resp->bank1_printid,
	     le16_to_cpu(resp->bank1_remaining),
	     le16_to_cpu(resp->bank1_finished),
	     le16_to_cpu(resp->bank1_specified));

	INFO("Bank 2: 0x%02x (%s) Job %03d @ %03d/%03d (%03d remaining)\n",
	     resp->bank2_status, bank_statuses[resp->bank1_status],
	     resp->bank2_printid,
	     le16_to_cpu(resp->bank2_remaining),
	     le16_to_cpu(resp->bank2_finished),
	     le16_to_cpu(resp->bank2_specified));

	INFO("Tonecurve Status: 0x%02x (%s)\n", resp->tonecurve_status, tonecurve_statuses[resp->tonecurve_status]);

	return 0;
}

static int get_fwinfo(libusb_device_handle *dev, 
		      uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_fwinfo_cmd  cmd;
	struct s2145_fwinfo_resp *resp = (struct s2145_fwinfo_resp *)rdbuf;
	int ret, num = 0;
	int i;

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_FWINFO);
	cmd.hdr.len = cpu_to_le16(1);

	INFO("FW Information:\n");

	for (i = 1 ; i <= 7 ; i++) {
		cmd.target = i;
		if ((ret = send_data(dev, endp_down,
				     (uint8_t *) &cmd, sizeof(cmd))))
			return -1;
		
		ret = libusb_bulk_transfer(dev, endp_up,
					   rdbuf,
					   READBACK_LEN,
					   &num,
					   5000);
		
		if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
			ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
			return ret;
		}
		
		if (resp->hdr.result != RESULT_SUCCESS) {
			INFO("Printer Status:  %02x\n", resp->hdr.status);
			
			INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
			     resp->hdr.result, resp->hdr.error, resp->hdr.printer_major,
			     resp->hdr.printer_minor);
			continue;
		}

		if (le16_to_cpu(resp->hdr.payload_len) != (sizeof(struct s2145_fwinfo_resp) - sizeof(struct s2145_status_hdr)))
			continue;
		
		INFO(" '%s'\t ver %02x.%02x\n", fwinfo_targets[i],
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

static int get_errorlog(libusb_device_handle *dev, 
			uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_cmd_hdr cmd;
	struct s2145_errorlog_resp *resp = (struct s2145_errorlog_resp *) rdbuf;
	int ret, num = 0;
	int i;

	cmd.cmd = cpu_to_le16(S2145_CMD_ERRORLOG);
	cmd.len = cpu_to_le16(0);

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, sizeof(cmd))))
		return -1;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}

	if (resp->hdr.result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x\n", resp->hdr.status);
		
		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->hdr.result, resp->hdr.error, resp->hdr.printer_major,
		     resp->hdr.printer_minor);
		return -1;
	}
	
	if (le16_to_cpu(resp->hdr.payload_len) != (sizeof(struct s2145_errorlog_resp) - sizeof(struct s2145_status_hdr)))
		return -2;

	INFO("Stored Error Events: %d entries:\n", resp->count);
	for (i = 0 ; i < resp->count ; i++) {
		INFO(" %02d: 0x%02x/0x%02x @ %08d prints\n", i,
		     resp->items[i].major, resp->items[i].minor, 
		     le32_to_cpu(resp->items[i].print_counter));
	}
	return 0;
}

static int get_mediainfo(libusb_device_handle *dev, 
			 uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_cmd_hdr cmd;
	struct s2145_mediainfo_resp *resp = (struct s2145_mediainfo_resp *) rdbuf;
	int ret, num = 0;
	int i;

	cmd.cmd = cpu_to_le16(S2145_CMD_MEDIAINFO);
	cmd.len = cpu_to_le16(0);

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, sizeof(cmd))))
		return -1;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}

	if (resp->hdr.result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x\n", resp->hdr.status);
		
		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->hdr.result, resp->hdr.error, resp->hdr.printer_major,
		     resp->hdr.printer_minor);
		return -1;
	}
	
	if (le16_to_cpu(resp->hdr.payload_len) != (sizeof(struct s2145_mediainfo_resp) - sizeof(struct s2145_status_hdr)))
		return -2;

	INFO("Supported Media Information: %d entries:\n", resp->count);
	for (i = 0 ; i < resp->count ; i++) {
		INFO(" %02d: C 0x%02x (%s), %04dx%04d, M 0x%02x (%s), P 0x%02x (%s)\n", i,
		     resp->items[i].code, print_medias[resp->items[i].code],
		     le16_to_cpu(resp->items[i].columns),
		     le16_to_cpu(resp->items[i].rows), 
		     resp->items[i].media_type, media_types[resp->items[i].media_type],
		     resp->items[i].print_type, print_methods[resp->items[i].print_type]);
	}
	return 0;
}

static int get_user_string(libusb_device_handle *dev, 
			   uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_cmd_hdr cmd;
	struct s2145_getunique_resp *resp = (struct s2145_getunique_resp*) rdbuf;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(S2145_CMD_GETUNIQUE);
	cmd.len = cpu_to_le16(0);

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, sizeof(cmd))))
		return -1;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}

	if (resp->hdr.result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x\n", resp->hdr.status);

		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->hdr.result, resp->hdr.error, resp->hdr.printer_major,
		     resp->hdr.printer_minor);
		return -1;
	}

	/* Null-terminate */
	resp->hdr.payload_len = le16_to_cpu(resp->hdr.payload_len);
	if (resp->hdr.payload_len > 23)
		resp->hdr.payload_len = 23;
	resp->data[resp->hdr.payload_len] = 0;
	INFO("Unique String: '%s'\n", resp->data);
	return 0;
}

static int set_user_string(char *str, libusb_device_handle *dev, 
			   uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
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

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, cmd.len + 1 + sizeof(cmd.hdr))))
		return -1;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}

	if (resp->result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x\n", resp->status);

		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->result, resp->error, resp->printer_major,
		     resp->printer_minor);
		return -1;
	}

	return 0;
}

static int cancel_job(char *str, libusb_device_handle *dev, 
		      uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_cancel_cmd cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	if (!str)
		return -1;

	cmd.id = atoi(str);

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_CANCELJOB);
	cmd.hdr.len = cpu_to_le16(1);

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, sizeof(cmd))))
		return -1;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}

	if (resp->result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x\n", resp->status);

		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->result, resp->error, resp->printer_major,
		     resp->printer_minor);
		return -1;
	}

	return 0;
}

static int flash_led(libusb_device_handle *dev, 
		     uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_cmd_hdr cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(S2145_CMD_FLASHLED);
	cmd.len = cpu_to_le16(0);

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, sizeof(cmd))))
		return -1;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}

	if (resp->result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x\n", resp->status);

		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->result, resp->error, resp->printer_major,
		     resp->printer_minor);
		return -1;
	}

	return 0;
}

static int reset_curve(int target, libusb_device_handle *dev, 
		       uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_reset_cmd cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	cmd.target = target;

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_RESET);
	cmd.hdr.len = cpu_to_le16(1);

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, sizeof(cmd))))
		return -1;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}

	if (resp->result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x\n", resp->status);

		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->result, resp->error, resp->printer_major,
		     resp->printer_minor);
		return -1;
	}

	return 0;
}

static int button_set(int enable, libusb_device_handle *dev, 
		      uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_button_cmd cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	cmd.enabled = enable;

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_BUTTON);
	cmd.hdr.len = cpu_to_le16(1);

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, sizeof(cmd))))
		return -1;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}

	if (resp->result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x\n", resp->status);

		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->result, resp->error, resp->printer_major,
		     resp->printer_minor);
		return -1;
	}

	return 0;
}

static int get_tonecurve(int type, char *fname, libusb_device_handle *dev, 
			 uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_readtone_cmd  cmd;
	struct s2145_readtone_resp *resp = (struct s2145_readtone_resp *) rdbuf;
	int ret, num = 0;

	uint8_t *data;
	uint16_t curves[768];

	int i,j;

	cmd.curveid = type;

	cmd.hdr.cmd = cpu_to_le16(S2145_CMD_READTONE);
	cmd.hdr.len = cpu_to_le16(1);

	INFO("Dump %s Tone Curve to '%s'\n", tonecurve_statuses[type], fname);

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, sizeof(cmd))))
		return -1;
	
	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);
	
	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}
		
	if (resp->hdr.result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x\n", resp->hdr.status);
		
		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->hdr.result, resp->hdr.error, resp->hdr.printer_major,
		     resp->hdr.printer_minor);
		return -2;
	}

	resp->total_size = le16_to_cpu(resp->total_size);

	data = malloc(resp->total_size * 2);

	i = 0;
	while (i < resp->total_size) {
		ret = libusb_bulk_transfer(dev, endp_up,
					   data + i,
					   resp->total_size * 2 - i,
					   &num,
					   5000);

		if (ret < 0) {
			ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num + i, (int)resp->total_size, endp_up);
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

static int set_tonecurve(int target, char *fname, libusb_device_handle *dev, 
			 uint8_t endp_down, uint8_t endp_up) 
{
	uint8_t rdbuf[READBACK_LEN];
	struct s2145_update_cmd cmd;
	struct s2145_status_hdr *resp = (struct s2145_status_hdr *) rdbuf;
	int ret, num = 0;

	INFO("Set %s Tone Curve from '%s'\n", update_targets[target], fname);

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

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) &cmd, sizeof(cmd))))
		return -1;

	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)sizeof(resp), endp_up);
		return ret;
	}

	if (resp->result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x\n", resp->status);

		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x)\n",
		     resp->result, resp->error, resp->printer_major,
		     resp->printer_minor);
		return -1;
	}

	/* Sent transfer */
	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) data, UPDATE_SIZE))) {
		return ret;
	}

	free(data);

	return 0;
}


int main (int argc, char **argv) 
{
	struct libusb_context *ctx;
	struct libusb_device **list;
	struct libusb_device_handle *dev;
	struct libusb_config_descriptor *config;

	uint8_t endp_up = 0;
	uint8_t endp_down = 0;

	int data_fd = fileno(stdin);

	int i, num;
	int claimed;

	int query_only = 0;
	int ret = 0;
	int iface = 0;
	int found = -1;
	int copies = 1;
	int jobid = 0;
	char *uri = getenv("DEVICE_URI");;
	char *use_serno = NULL;

	struct s2145_printjob_hdr hdr;
	struct s2145_cmd_hdr *cmd;
	struct s2145_print_cmd *print;
	struct s2145_status_resp *sts; 

	uint8_t *planedata = NULL, *cmdbuf = NULL;
	uint32_t datasize;

	uint8_t rdbuf[READBACK_LEN];
	uint8_t rdbuf2[READBACK_LEN];
	int last_state = -1, state = S_IDLE;

	DEBUG("Shinko/Sinfonia CHC-S2145 CUPS backend version " VERSION "/" BACKEND_VERSION " \n");

	/* Cmdline help */
	if (argc < 2) {
		DEBUG("Usage:\n\t%s [ infile | - ]\n\t%s job user title num-copies options [ filename ]\n\t%s [ -qs | -qm | -qf | -qe | -qu | -qtu filename | -qtc filename ]\n\t%s [ -su somestring | -stu filename | -stc filename ]\n\t%s [ -pc id | -fl | -ru | -rp | -b1 | -b0 ]\n\n",
		      argv[0], argv[0], argv[0], argv[0], argv[0]);
		libusb_init(&ctx);
		find_and_enumerate(ctx, &list, NULL, 1);
		libusb_free_device_list(list, 1);
		libusb_exit(ctx);
		exit(1);
	}

	/* Are we running as a CUPS backend? */
	if (uri) {
		if (argv[1])
			jobid = atoi(argv[1]);
		if (argv[4])
			copies = atoi(argv[4]);
		if (argv[6]) {  /* IOW, is it specified? */
			data_fd = open(argv[6], O_RDONLY);
			if (data_fd < 0) {
				perror("ERROR:Can't open input file");
				exit(1);
			}
		}

		/* Ensure we're using BLOCKING I/O */
		i = fcntl(data_fd, F_GETFL, 0);
		if (i < 0) {
			perror("ERROR:Can't open input");
			exit(1);
		}
		i &= ~O_NONBLOCK;
		i = fcntl(data_fd, F_SETFL, 0);
		if (i < 0) {
			perror("ERROR:Can't open input");
			exit(1);
		}
		/* Start parsing URI 'selphy://PID/SERIAL' */
		if (strncmp(URI_PREFIX, uri, strlen(URI_PREFIX))) {
			ERROR("Invalid URI prefix (%s)\n", uri);
			exit(1);
		}
		use_serno = strchr(uri, '=');
		if (!use_serno || !*(use_serno+1)) {
			ERROR("Invalid URI (%s)\n", uri);
			exit(1);
		}
		use_serno++;
	} else {
		if (!strcmp("-qs", argv[1]) ||
		    !strcmp("-qf", argv[1]) ||
		    !strcmp("-qe", argv[1]) ||
		    !strcmp("-qm", argv[1]) ||
		    !strcmp("-qu", argv[1]) ||
		    !strcmp("-qtc", argv[1]) ||
		    !strcmp("-qtu", argv[1]) ||
		    !strcmp("-pc", argv[1]) ||
		    !strcmp("-fl", argv[1]) ||
		    !strcmp("-ru", argv[1]) ||
		    !strcmp("-rp", argv[1]) ||
		    !strcmp("-b1", argv[1]) ||
		    !strcmp("-b0", argv[1]) ||
		    !strcmp("-stc", argv[1]) ||
		    !strcmp("-stu", argv[1]) ||
		    !strcmp("-su", argv[1])) {
			query_only = 1;
			goto skip_read;
		}

		srand(getpid());
		jobid = rand();

		/* Open Input File */
		if (strcmp("-", argv[1])) {
			data_fd = open(argv[1], O_RDONLY);
			if (data_fd < 0) {
				perror("ERROR:Can't open input file");
				exit(1);
			}
		}
	}

	/* Ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, sigterm_handler);

	/* Ensure jobid is sane */
	jobid = (jobid & 0x7f) + 1;

	/* Read in then validate header */
	read(data_fd, &hdr, sizeof(hdr));
	if (le32_to_cpu(hdr.len1) != 0x10 ||
	    le32_to_cpu(hdr.model) != 2145 ||
	    le32_to_cpu(hdr.len2) != 0x64 ||
	    le32_to_cpu(hdr.dpi) != 300) {
		ERROR("Unrecognized header data format!\n");
		exit(1);
	}

	/* Read in image data */
	cmdbuf = malloc(CMDBUF_LEN);
	cmd = (struct s2145_cmd_hdr *) cmdbuf;
	print = (struct s2145_print_cmd *) cmdbuf;

	sts = (struct s2145_status_resp *) rdbuf;

	datasize = le32_to_cpu(hdr.rows) * le32_to_cpu(hdr.columns) * 3;
	planedata = malloc(datasize);
	if (!cmdbuf || !planedata) {
		ERROR("Memory allocation failure!\n");
		exit(1);
	}

	{
		int remain;
		uint8_t *ptr = planedata;
		remain = datasize;
		do {
			ret = read(data_fd, ptr, remain);
			if (ret < 0) {
				ERROR("Read failed (%d/%d/%d)\n", 
				      ret, remain, datasize);
				perror("ERROR: Read failed");
				exit(1);
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}

	/* Make sure footer is sane too */
	ret = read(data_fd, cmdbuf, 4);
	if (ret < 0 || ret != 4) {
		ERROR("Read failed (%d/%d/%d)\n", 
		      ret, 4, 4);
		perror("ERROR: Read failed");
		exit(1);
	}
	if (cmdbuf[0] != 0x04 ||
	    cmdbuf[1] != 0x03 ||
	    cmdbuf[2] != 0x02 ||
	    cmdbuf[3] != 0x01) {
		ERROR("Unrecognized footer data format!\n");
		exit(1);
	}

	close(data_fd); /* We're done reading! */

skip_read:	
	/* Libusb setup */
	libusb_init(&ctx);
	found = find_and_enumerate(ctx, &list, use_serno, 0);

	if (found == -1) {
		ERROR("Printer open failure (No suitable printers found!)\n");
		ret = 3;
		goto done;
	}

	ret = libusb_open(list[found], &dev);
	if (ret) {
		ERROR("Printer open failure (Need to be root?) (%d)\n", ret);
		ret = 4;
		goto done;
	}
	
	claimed = libusb_kernel_driver_active(dev, iface);
	if (claimed) {
		ret = libusb_detach_kernel_driver(dev, iface);
		if (ret) {
			ERROR("Printer open failure (Could not detach printer from kernel)\n");
			ret = 4;
			goto done_close;
		}
	}

	ret = libusb_claim_interface(dev, iface);
	if (ret) {
		ERROR("Printer open failure (Could not claim printer interface)\n");
		ret = 4;
		goto done_close;
	}

	ret = libusb_get_active_config_descriptor(list[found], &config);
	if (ret) {
		ERROR("Printer open failure (Could not fetch config descriptor)\n");
		ret = 4;
		goto done_close;
	}

	for (i = 0 ; i < config->interface[0].altsetting[0].bNumEndpoints ; i++) {
		if ((config->interface[0].altsetting[0].endpoint[i].bmAttributes & 3) == LIBUSB_TRANSFER_TYPE_BULK) {
			if (config->interface[0].altsetting[0].endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_IN)
				endp_up = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;
			else
				endp_down = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;				
		}
	}

	if (query_only) {
		if (!strcmp("-qs", argv[1]))
			get_status(dev, endp_down, endp_up);
		else if (!strcmp("-qf", argv[1]))
			get_fwinfo(dev, endp_down, endp_up);
		else if (!strcmp("-qe", argv[1]))
			get_errorlog(dev, endp_down, endp_up);
		else if (!strcmp("-qm", argv[1]))
			get_mediainfo(dev, endp_down, endp_up);
		else if (!strcmp("-qu", argv[1]))
			get_user_string(dev, endp_down, endp_up);
		else if (!strcmp("-qtu", argv[1]))
			get_tonecurve(TONECURVE_USER, argv[2], dev, endp_down, endp_up);
		else if (!strcmp("-qtc", argv[1]))
			get_tonecurve(TONECURVE_CURRENT, argv[2], dev, endp_down, endp_up);
		else if (!strcmp("-su", argv[1]))
			set_user_string(argv[2], dev, endp_down, endp_up);
		else if (!strcmp("-stu", argv[1]))
			set_tonecurve(UPDATE_TARGET_USER, argv[2], dev, endp_down, endp_up);
		else if (!strcmp("-stc", argv[1]))
			set_tonecurve(UPDATE_TARGET_CURRENT, argv[2], dev, endp_down, endp_up);
		else if (!strcmp("-pc", argv[1]))
			cancel_job(argv[2], dev, endp_down, endp_up);
		else if (!strcmp("-fl", argv[1]))
			flash_led(dev, endp_down, endp_up);
		else if (!strcmp("-ru", argv[1]))
			reset_curve(RESET_USER_CURVE, dev, endp_down, endp_up);
		else if (!strcmp("-rp", argv[1]))
			reset_curve(RESET_PRINTER, dev, endp_down, endp_up);
		else if (!strcmp("-b1", argv[1]))
			button_set(BUTTON_ENABLED, dev, endp_down, endp_up);
		else if (!strcmp("-b0", argv[1]))
			button_set(BUTTON_DISABLED, dev, endp_down, endp_up);

		goto done_claimed;
	}

	/* Time for the main processing loop */
top:

	/* Send Status Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmd->cmd = cpu_to_le16(S2145_CMD_STATUS);
	cmd->len = cpu_to_le16(0);

	if ((ret = send_data(dev, endp_down,
			     (uint8_t *) cmd, sizeof(*cmd))))
		goto done_claimed;

	/* Read in the printer status */
	memset(rdbuf, 0, READBACK_LEN);
	ret = libusb_bulk_transfer(dev, endp_up,
				   rdbuf,
				   READBACK_LEN,
				   &num,
				   5000);

	if (ret < 0 || (num < sizeof(struct s2145_status_hdr))) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, num, (int)READBACK_LEN, endp_up);
		ret = 4;
		goto done_claimed;
	}

	if (memcmp(rdbuf, rdbuf2, READBACK_LEN)) {
		DEBUG("readback: ");
		for (i = 0 ; i < num ; i++) {
			DEBUG2("%02x ", rdbuf[i]);
		}
		DEBUG2("\n");
	} else {
		sleep(1);
	}
	if (state != last_state) {
		DEBUG("last_state %d new %d\n", last_state, state);
		last_state = state;
	}
	fflush(stderr);       

	switch (state) {
	case S_IDLE:
		INFO("Waiting for printer idle\n");
		/* Basic error handling */
		if (sts->hdr.result != RESULT_SUCCESS)
			goto printer_error;
		if (sts->hdr.status != ERROR_NONE)
			goto printer_error;

		/* If either bank is free, continue */
		if (sts->bank1_status == BANK_STATUS_FREE || 
		    sts->bank2_status == BANK_STATUS_FREE) 
			state = S_PRINTER_READY_CMD;

		break;
	case S_PRINTER_READY_CMD:
		INFO("Initiating print job (internal id %d)\n", jobid);

		memset(cmdbuf, 0, CMDBUF_LEN);
		print->hdr.cmd = cpu_to_le16(S2145_CMD_PRINTJOB);
		print->hdr.len = cpu_to_le16(sizeof (*print) - sizeof(*cmd));
		print->id = jobid;
		print->count = cpu_to_le16(copies);
		print->columns = cpu_to_le16(le32_to_cpu(hdr.columns));
		print->rows = cpu_to_le16(le32_to_cpu(hdr.rows));
		print->media = le32_to_cpu(hdr.media);
		print->mode = le32_to_cpu(hdr.mode);
		print->method = le32_to_cpu(hdr.method);

		if ((ret = send_data(dev, endp_down,
				     (uint8_t *) print, sizeof(*print))))
			goto done_claimed;
		state = S_PRINTER_SENT_PRINT_CMD;

		break;
	case S_PRINTER_SENT_PRINT_CMD:
		if (sts->hdr.result != RESULT_SUCCESS) {
			if (sts->hdr.status == ERROR_BUFFER_FULL) {
				INFO("Buffers full, retrying\n");
				state = S_PRINTER_READY_CMD;
				break;
			} else if (sts->hdr.status != ERROR_NONE)
				goto printer_error;
		}

		INFO("Sending data to printer\n");
		if ((ret = send_data(dev, endp_down, planedata, datasize)))
			goto done_claimed;

		state = S_PRINTER_SENT_DATA;
		break;
	case S_PRINTER_SENT_DATA:
		INFO("Waiting for printer to acknowledge completion\n");
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

	/* Done printing */
	INFO("All printing done\n");
	ret = 0;
	goto done_claimed;

printer_error:
	ERROR("Printer reported error: %#x (%#x) -> %#x.%#x\n", sts->hdr.error, sts->hdr.status, sts->hdr.printer_major, sts->hdr.printer_minor);
	// XXX write this.

done_claimed:
	libusb_release_interface(dev, iface);

done_close:
	if (claimed)
		libusb_attach_kernel_driver(dev, iface);

	libusb_close(dev);
done:
	if (planedata)
		free(planedata);
	if (cmdbuf)
		free(cmdbuf);

	libusb_free_device_list(list, 1);
	libusb_exit(ctx);

	return ret;
}



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
