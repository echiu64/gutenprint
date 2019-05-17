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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "backend_common.h"
#include "backend_sinfonia.h"

int sinfonia_read_parse(int data_fd, uint32_t model,
			struct sinfonia_printjob *job)
{
	uint32_t hdr[29];
	int ret, i;
	uint8_t tmpbuf[4];

	/* Read in header */
	ret = read(data_fd, hdr, SINFONIA_HDR_LEN);
	if (ret < 0 || ret != SINFONIA_HDR_LEN) {
		if (ret == 0)
			return CUPS_BACKEND_CANCEL;
		ERROR("Read failed (%d/%d)\n",
		      ret, SINFONIA_HDR_LEN);
		perror("ERROR: Read failed");
		return ret;
	}

	/* Byteswap everything */
	for (i = 0 ; i < (SINFONIA_HDR_LEN / 4) ; i++) {
		hdr[i] = le32_to_cpu(hdr[i]);
	}

	/* Sanity-check headers */
	if (hdr[0] != SINFONIA_HDR1_LEN ||
	    hdr[4] != SINFONIA_HDR2_LEN ||
	    hdr[22] != SINFONIA_DPI) {
		ERROR("Unrecognized header data format!\n");
		return CUPS_BACKEND_CANCEL;
	}
	if (hdr[1] != model) {
		ERROR("job/printer mismatch (%u/%u)!\n", hdr[1], model);
		return CUPS_BACKEND_CANCEL;
	}

	if (!hdr[13] || !hdr[14]) {
		ERROR("Bad job cols/rows!\n");
		return CUPS_BACKEND_CANCEL;
	}

	/* Work out data length */
	job->datalen = hdr[13] * hdr[14] * 3;
	job->databuf = malloc(job->datalen);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	/* Read in payload data */
	{
		uint32_t remain = job->datalen;
		uint8_t *ptr = job->databuf;
		do {
			ret = read(data_fd, ptr, remain);
			if (ret < 0) {
				ERROR("Read failed (%d/%u/%d)\n",
				      ret, remain, job->datalen);
				perror("ERROR: Read failed");
				free(job->databuf);
				job->databuf = NULL;
				return ret;
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}

	/* Make sure footer is sane too */
	ret = read(data_fd, tmpbuf, 4);
	if (ret != 4) {
		ERROR("Read failed (%d/%d)\n", ret, 4);
		perror("ERROR: Read failed");
		free(job->databuf);
		job->databuf = NULL;
		return ret;
	}
	if (tmpbuf[0] != 0x04 ||
	    tmpbuf[1] != 0x03 ||
	    tmpbuf[2] != 0x02 ||
	    tmpbuf[3] != 0x01) {
		ERROR("Unrecognized footer data format!\n");
		free (job->databuf);
		job->databuf = NULL;
		return CUPS_BACKEND_CANCEL;
	}

	/* Fill out job params */
	job->jp.media = hdr[6];
	if (hdr[1] != 6245)
		job->jp.method = hdr[8];
	if (hdr[1] == 2245 || hdr[1] == 6245)
		job->jp.quality = hdr[9];
	if (hdr[1] == 1245 || hdr[1] == 2145)
		job->jp.oc_mode = hdr[9];
	else
		job->jp.oc_mode = hdr[10];
	if (hdr[1] == 1245)
		job->jp.mattedepth = hdr[11];
	if (hdr[1] == 1245)
		job->jp.dust = hdr[12];
	job->jp.columns = hdr[13];
	job->jp.rows = hdr[14];
	job->jp.copies = hdr[15];

	if (hdr[1] == 2245 || hdr[1] == 6145)
		job->jp.ext_flags = hdr[28];

	return CUPS_BACKEND_OK;
}

const char *sinfonia_update_targets (uint8_t v) {
	switch (v) {
	case UPDATE_TARGET_USER:
		return "User";
	case UPDATE_TARGET_CURRENT:
		return "Current";
	default:
		return "Unknown";
	}
}

const char *sinfonia_tonecurve_statuses (uint8_t v)
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

const char *sinfonia_bank_statuses(uint8_t v)
{
	switch (v) {
	case BANK_STATUS_FREE:
		return "Free";
	case BANK_STATUS_XFER:
		return "Xfer";
	case BANK_STATUS_FULL:
		return "Full";
	case BANK_STATUS_PRINTING:
		return "Printing";
	default:
		return "Unknown";
	}
}

const char *sinfonia_error_str(uint8_t v) {
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
		return "Maintenance Needed";
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

const char *sinfonia_media_types(uint8_t v) {
	switch (v) {
	case MEDIA_TYPE_UNKNOWN:
		return "Unknown";
	case MEDIA_TYPE_PAPER:
		return "Paper";
	default:
		return "Unknown";
	}
}

const char *sinfonia_print_methods (uint8_t v) {
	switch (v & 0xf) {
	case PRINT_METHOD_STD:
		return "Standard";
	case PRINT_METHOD_COMBO_2:
		return "2up";
	case PRINT_METHOD_COMBO_3:
		return "3up";
	case PRINT_METHOD_SPLIT:
		return "Split";
	case PRINT_METHOD_DOUBLE:
		return "Double";
	default:
		return "Unknown";
	}
}

const char *sinfonia_print_codes (uint8_t v, int eightinch) {
	if (eightinch) {
		switch (v) {
		case CODE_8x10:
			return "8x10";
		case CODE_8x12:
			return "8x12";
		case CODE_8x4:
			return "8x4";
		case CODE_8x5:
			return "8x5";
		case CODE_8x6:
			return "8x6";
		case CODE_8x8:
			return "8x8";
		case CODE_8x4_2:
			return "8x4*2";
		case CODE_8x5_2:
			return "8x5*2";
		case CODE_8x6_2:
			return "8x6*2";
		case CODE_8x4_3:
			return "8x4*3";
		default:
			return "Unknown";
		}
	}

	switch (v) {
	case CODE_4x6:
		return "4x6";
	case CODE_3_5x5:
		return "3.5x5";
	case CODE_5x7:
		return "5x7";
	case CODE_6x9:
		return "6x9";
	case CODE_6x8:
		return "6x8";
	case CODE_2x6:
		return "2x6";
	case CODE_6x6:
		return "6x6";
	case CODE_89x60mm:
		return "89x60mm";
	case CODE_89x59mm:
		return "89x59mm";
	case CODE_89x58mm:
		return "89x58mm";
	case CODE_89x57mm:
		return "89x57mm";
	case CODE_89x56mm:
		return "89x56mm";
	case CODE_89x55mm:
		return "89x55mm";
	default:
		return "Unknown";
	}
}

const char *sinfonia_status_str(uint8_t v) {
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

const char *sinfonia_cmd_names(uint16_t v) {
	switch (le16_to_cpu(v)) {
	case SINFONIA_CMD_GETSTATUS:
		return "Get Status";
	case SINFONIA_CMD_MEDIAINFO:
		return "Get Media Info";
	case SINFONIA_CMD_MODELNAME:
		return "Get Model Name";
	case SINFONIA_CMD_ERRORLOG:
		return "Get Error Log";
	case SINFONIA_CMD_GETPARAM:
		return "Get Parameter";
	case SINFONIA_CMD_GETSERIAL:
		return "Get Serial Number";
	case SINFONIA_CMD_PRINTSTAT:
		return "Get Print ID Status";
	case SINFONIA_CMD_EXTCOUNTER:
		return "Get Extended Counters";
	case SINFONIA_CMD_PRINTJOB:
		return "Print";
	case SINFONIA_CMD_CANCELJOB:
		return "Cancel Print";
	case SINFONIA_CMD_FLASHLED:
		return "Flash LEDs";
	case SINFONIA_CMD_RESET:
		return "Reset";
	case SINFONIA_CMD_READTONE:
		return "Read Tone Curve";
	case SINFONIA_CMD_BUTTON:
		return "Button Enable";
	case SINFONIA_CMD_SETPARAM:
		return "Set Parameter";
	case SINFONIA_CMD_GETUNIQUE:
		return "Get Unique String";
	case SINFONIA_CMD_GETCORR:
		return "Get Image Correction Parameter";
	case SINFONIA_CMD_GETEEPROM:
		return "Get EEPROM Backup Parameter";
	case SINFONIA_CMD_SETEEPROM:
		return "Set EEPROM Backup Parameter";
	case SINFONIA_CMD_SETTIME:
		return "Time Setting";
	case SINFONIA_CMD_FWINFO:
		return "Get Firmware Info";
	case SINFONIA_CMD_UPDATE:
		return "Update";
	case SINFONIA_CMD_SETUNIQUE:
		return "Set Unique String";
	default:
		return "Unknown Command";
	}
}

const char *kodak6_mediatypes(int type)
{
	switch(type) {
	case KODAK6_MEDIA_NONE:
		return "No media";
	case KODAK6_MEDIA_6R:
	case KODAK6_MEDIA_6TR2:
		return "Kodak 6R";
	default:
		return "Unknown";
	}
	return "Unknown";
}

void kodak6_dumpmediacommon(int type)
{
	switch (type) {
	case KODAK6_MEDIA_6R:
		INFO("Media type: 6R (Kodak 197-4096 or equivalent)\n");
		break;
	case KODAK6_MEDIA_6TR2:
		INFO("Media type: 6R (Kodak 396-2941 or equivalent)\n");
		break;
	default:
		INFO("Media type %02x (unknown, please report!)\n", type);
		break;
	}
}

/* Below are for S1145 (EK68xx) and S1245 only! */
const char *sinfonia_1x45_status_str(uint8_t status1, uint32_t status2, uint8_t error)
{
	switch(status1) {
	case STATE_STATUS1_STANDBY:
		return "Standby (Ready)";
	case STATE_STATUS1_WAIT:
		switch (status2) {
		case WAIT_STATUS2_INIT:
			return "Wait (Initializing)";
		case WAIT_STATUS2_RIBBON:
			return "Wait (Ribbon Winding)";
		case WAIT_STATUS2_THERMAL:
			return "Wait (Thermal Protection)";
		case WAIT_STATUS2_OPERATING:
			return "Wait (Operating)";
		case WAIT_STATUS2_BUSY:
			return "Wait (Busy)";
		default:
			return "Wait (Unknown)";
		}
	case STATE_STATUS1_ERROR:
		switch (status2) {
		case ERROR_STATUS2_CTRL_CIRCUIT:
			switch (error) {
			case CTRL_CIR_ERROR_EEPROM1:
				return "Error (EEPROM1)";
			case CTRL_CIR_ERROR_EEPROM2:
				return "Error (EEPROM2)";
			case CTRL_CIR_ERROR_DSP:
				return "Error (DSP)";
			case CTRL_CIR_ERROR_CRC_MAIN:
				return "Error (Main CRC)";
			case CTRL_CIR_ERROR_DL_MAIN:
				return "Error (Main Download)";
			case CTRL_CIR_ERROR_CRC_DSP:
				return "Error (DSP CRC)";
			case CTRL_CIR_ERROR_DL_DSP:
				return "Error (DSP Download)";
			case CTRL_CIR_ERROR_ASIC:
				return "Error (ASIC)";
			case CTRL_CIR_ERROR_DRAM:
				return "Error (DRAM)";
			case CTRL_CIR_ERROR_DSPCOMM:
				return "Error (DSP Communincation)";
			default:
				return "Error (Unknown Circuit)";
			}
		case ERROR_STATUS2_MECHANISM_CTRL:
			switch (error) {
			case MECH_ERROR_HEAD_UP:
				return "Error (Head Up Mechanism)";
			case MECH_ERROR_HEAD_DOWN:
				return "Error (Head Down Mechanism)";
			case MECH_ERROR_MAIN_PINCH_UP:
				return "Error (Main Pinch Up Mechanism)";
			case MECH_ERROR_MAIN_PINCH_DOWN:
				return "Error (Main Pinch Down Mechanism)";
			case MECH_ERROR_SUB_PINCH_UP:
				return "Error (Sub Pinch Up Mechanism)";
			case MECH_ERROR_SUB_PINCH_DOWN:
				return "Error (Sub Pinch Down Mechanism)";
			case MECH_ERROR_FEEDIN_PINCH_UP:
				return "Error (Feed-in Pinch Up Mechanism)";
			case MECH_ERROR_FEEDIN_PINCH_DOWN:
				return "Error (Feed-in Pinch Down Mechanism)";
			case MECH_ERROR_FEEDOUT_PINCH_UP:
				return "Error (Feed-out Pinch Up Mechanism)";
			case MECH_ERROR_FEEDOUT_PINCH_DOWN:
				return "Error (Feed-out Pinch Down Mechanism)";
			case MECH_ERROR_CUTTER_LR:
				return "Error (Left->Right Cutter)";
			case MECH_ERROR_CUTTER_RL:
				return "Error (Right->Left Cutter)";
			default:
				return "Error (Unknown Mechanism)";
			}
		case ERROR_STATUS2_SENSOR:
			switch (error) {
			case SENSOR_ERROR_CUTTER:
				return "Error (Cutter Sensor)";
			case SENSOR_ERROR_HEAD_DOWN:
				return "Error (Head Down Sensor)";
			case SENSOR_ERROR_HEAD_UP:
				return "Error (Head Up Sensor)";
			case SENSOR_ERROR_MAIN_PINCH_DOWN:
				return "Error (Main Pinch Down Sensor)";
			case SENSOR_ERROR_MAIN_PINCH_UP:
				return "Error (Main Pinch Up Sensor)";
			case SENSOR_ERROR_FEED_PINCH_DOWN:
				return "Error (Feed Pinch Down Sensor)";
			case SENSOR_ERROR_FEED_PINCH_UP:
				return "Error (Feed Pinch Up Sensor)";
			case SENSOR_ERROR_EXIT_PINCH_DOWN:
				return "Error (Exit Pinch Up Sensor)";
			case SENSOR_ERROR_EXIT_PINCH_UP:
				return "Error (Exit Pinch Up Sensor)";
			case SENSOR_ERROR_LEFT_CUTTER:
				return "Error (Left Cutter Sensor)";
			case SENSOR_ERROR_RIGHT_CUTTER:
				return "Error (Right Cutter Sensor)";
			case SENSOR_ERROR_CENTER_CUTTER:
				return "Error (Center Cutter Sensor)";
			case SENSOR_ERROR_UPPER_CUTTER:
				return "Error (Upper Cutter Sensor)";
			case SENSOR_ERROR_PAPER_FEED_COVER:
				return "Error (Paper Feed Cover)";
			default:
				return "Error (Unknown Sensor)";
			}
		case ERROR_STATUS2_COVER_OPEN:
			switch (error) {
			case COVER_OPEN_ERROR_UPPER:
				return "Error (Upper Cover Open)";
			case COVER_OPEN_ERROR_LOWER:
				return "Error (Lower Cover Open)";
			default:
				return "Error (Unknown Cover Open)";
			}
		case ERROR_STATUS2_TEMP_SENSOR:
			switch (error) {
			case TEMP_SENSOR_ERROR_HEAD_HIGH:
				return "Error (Head Temperature High)";
			case TEMP_SENSOR_ERROR_HEAD_LOW:
				return "Error (Head Temperature Low)";
			case TEMP_SENSOR_ERROR_ENV_HIGH:
				return "Error (Environmental Temperature High)";
			case TEMP_SENSOR_ERROR_ENV_LOW:
				return "Error (Environmental Temperature Low)";
			default:
				return "Error (Unknown Temperature)";
			}
		case ERROR_STATUS2_PAPER_JAM:
			return "Error (Paper Jam)";
		case ERROR_STATUS2_PAPER_EMPTY:
			return "Error (Paper Empty)";
		case ERROR_STATUS2_RIBBON_ERR:
			return "Error (Ribbon)";
		default:
			return "Error (Unknown)";
		}
	default:
		return "Unknown!";
	}
}
