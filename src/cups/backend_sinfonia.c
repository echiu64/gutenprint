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

#include "backend_common.h"
#include "backend_sinfonia.h"

int sinfonia_read_parse(int data_fd, uint32_t model,
			struct sinfonia_printjob *job)
{
	uint32_t hdr[29];
	int ret, i;
	uint8_t tmpbuf[4];

	job->common.jobsize = sizeof(*job);

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
	if (hdr[1] == 1245) {
		job->jp.mattedepth = hdr[11];
		job->jp.dust = hdr[12];
	}
	job->jp.columns = hdr[13];
	job->jp.rows = hdr[14];
	job->common.copies = hdr[15];

	if (hdr[1] == 2245 || hdr[1] == 6145)
		job->jp.ext_flags = hdr[28];

	return CUPS_BACKEND_OK;
}

int sinfonia_raw10_read_parse(int data_fd, struct sinfonia_printjob *job)
{
	struct sinfonia_printcmd10_hdr hdr;
	int ret;

	job->common.jobsize = sizeof(*job);

	/* Read in header */
	ret = read(data_fd, &hdr, sizeof(hdr));
	if (ret < 0 || ret != sizeof(hdr)) {
		if (ret == 0)
			return CUPS_BACKEND_CANCEL;
		ERROR("Read failed (%d/%d/%d)\n",
		      ret, 0, (int)sizeof(hdr));
		perror("ERROR: Read failed");
		return CUPS_BACKEND_CANCEL;
	}
	/* Validate header */
	if (le16_to_cpu(hdr.hdr.cmd) != SINFONIA_CMD_PRINTJOB ||
	    le16_to_cpu(hdr.hdr.len) != 10) {
		ERROR("Unrecognized data format!\n");
		return CUPS_BACKEND_CANCEL;
	}
	job->common.copies = le16_to_cpu(hdr.copies);
	job->jp.rows = le16_to_cpu(hdr.rows);
	job->jp.columns = le16_to_cpu(hdr.columns);
	job->jp.media = hdr.media;
	job->jp.oc_mode = hdr.oc_mode;
	job->jp.method = hdr.method;

	/* Allocate buffer */
	job->datalen = job->jp.rows * job->jp.columns * 3;

	/* Hack in backprinting */
	if (job->common.copies & 0x8000) {
		job->common.copies &= ~0x8000;
		job->datalen += (44 * 2);
		job->jp.ext_flags = EXT_FLAG_BACKPRINT;
	}

	job->databuf = malloc(job->datalen);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	{
		int remain = job->datalen;
		uint8_t *ptr = job->databuf;
		do {
			ret = read(data_fd, ptr, remain);
			if (ret < 0) {
				ERROR("Read failed (%d/%d/%d)\n",
				      ret, remain, job->datalen);
				perror("ERROR: Read failed");
				return CUPS_BACKEND_CANCEL;
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}

	return CUPS_BACKEND_OK;
}

int sinfonia_panorama_splitjob(struct sinfonia_printjob *injob,
			       uint16_t max_rows,
			       struct sinfonia_printjob **newjobs)
{
	uint8_t *panels[3] = { NULL, NULL, NULL };
	uint16_t panel_rows[3] = { 0, 0, 0 };
	uint8_t numpanels;
	uint16_t overlap_rows;
	uint16_t inrows;
	uint16_t cols;

	int i;

	inrows = injob->jp.rows;
	cols = injob->jp.columns;

	switch (cols) {
	case 1548: /* EK6900/6950 */
		if (max_rows != 2136) {
			ERROR("Bad pano input\n");
			return CUPS_BACKEND_CANCEL;
		}
		if (inrows > 3036) // 5x10
			numpanels = 3;
		else
			numpanels = 2;

		overlap_rows = 600 + 36;

		if (numpanels == 3) {
			if (inrows > 4536) // 5x15
				overlap_rows = 100 + 36;
			else
				overlap_rows = 600 + 36;
		}
		break;
	case 1844: /* EK6900/6950 */
		if (max_rows != 2436) {
			ERROR("Bad pano input\n");
			return CUPS_BACKEND_CANCEL;
		}

		overlap_rows = 600 + 36;

		if (inrows > 4236) // ie 6x14
			numpanels = 3;
		else
			numpanels = 2;

		break;
	case 2464: /* EK8810 */
		if (max_rows != 3624 && max_rows != 3024) {
			ERROR("Bad pano input\n");
			return CUPS_BACKEND_CANCEL;
		}
		overlap_rows = 600 + 24;

		if (max_rows == 3024) { /* 8x10 media */
			if (inrows > 5424) // 8x18
				numpanels = 3;
			else
				numpanels = 2;
		} else { /* 8x12 media */
			if (inrows > 6624) // 8x22
				numpanels = 3;
			else
				numpanels = 2;
		}

		if (numpanels == 3) {
			if (max_rows == 3024) {
				if (inrows == 6024) // 8x20
					overlap_rows = 24;
			} else {
				if (inrows == 10824) // 8x36
					overlap_rows = 24;
			}
		} else {
			if (max_rows == 3024) {
				if (inrows == 9024) // 8x30
					overlap_rows = 24;
			} else {
				if (inrows == 7224) // 8x24
					overlap_rows = 24;
			}
		}

		break;
	default:
		ERROR("Unknown pano input cols: %d\n", cols);
		return CUPS_BACKEND_CANCEL;
	}

	/* Work out which number of rows per panel */
	if (!panel_rows[0]) {
		panel_rows[0] = max_rows;
		panel_rows[1] = inrows - panel_rows[0] + overlap_rows;
		if (numpanels > 2)
			panel_rows[2] = inrows - panel_rows[0] - panel_rows[1] + overlap_rows + overlap_rows;
	}

	/* Allocate and set up new jobs and buffers */
	for (i = 0 ; i < numpanels ; i++) {
		newjobs[i] = malloc(sizeof(struct sinfonia_printjob));
		if (!newjobs[i]) {
			ERROR("Memory allocation failure");
			return CUPS_BACKEND_RETRY_CURRENT;
		}
		panels[i] = malloc(cols * panel_rows[i] * 3);
		if (!panels[i]) {
			ERROR("Memory allocation failure");
			return CUPS_BACKEND_RETRY_CURRENT;
		}
		/* Fill in header differences */
		memcpy(newjobs[i], injob, sizeof(struct sinfonia_printjob));
		newjobs[i]->databuf = panels[i];
		newjobs[i]->jp.rows = panel_rows[i];
		// XXX what else?
	}

	dyesub_pano_split_rgb8(injob->databuf, cols, inrows,
			       numpanels, overlap_rows, max_rows,
			       panels, panel_rows);

	// XXX postprocess buffers!
	// pano_process_rgb8(numpanels, cols, overlap_rows, panels, panel_rows);

	return CUPS_BACKEND_OK;
}

int sinfonia_raw18_read_parse(int data_fd, struct sinfonia_printjob *job)
{
	struct sinfonia_printcmd18_hdr hdr;
	int ret;

	job->common.jobsize = sizeof(*job);

	/* Read in header */
	ret = read(data_fd, &hdr, sizeof(hdr));
	if (ret < 0 || ret != sizeof(hdr)) {
		if (ret == 0)
			return CUPS_BACKEND_CANCEL;
		ERROR("Read failed (%d/%d/%d)\n",
		      ret, 0, (int)sizeof(hdr));
		perror("ERROR: Read failed");
		return CUPS_BACKEND_CANCEL;
	}
	/* Validate header */
	if (le16_to_cpu(hdr.hdr.cmd) != SINFONIA_CMD_PRINTJOB ||
	    le16_to_cpu(hdr.hdr.len) != 18) {
		ERROR("Unrecognized data format!\n");
		return CUPS_BACKEND_CANCEL;
	}
	job->common.copies = le16_to_cpu(hdr.copies);
	job->jp.rows = le16_to_cpu(hdr.rows);
	job->jp.columns = le16_to_cpu(hdr.columns);
	job->jp.media = hdr.media;
	job->jp.oc_mode = hdr.oc_mode;
	job->jp.method = hdr.method;

	/* Allocate buffer */
	job->datalen = job->jp.rows * job->jp.columns * 3;
	job->databuf = malloc(job->datalen);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	{
		int remain = job->datalen;
		uint8_t *ptr = job->databuf;
		do {
			ret = read(data_fd, ptr, remain);
			if (ret < 0) {
				ERROR("Read failed (%d/%d/%d)\n",
				      ret, remain, job->datalen);
				perror("ERROR: Read failed");
				return CUPS_BACKEND_CANCEL;
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}

	return CUPS_BACKEND_OK;
}

int sinfonia_raw28_read_parse(int data_fd, struct sinfonia_printjob *job)
{
	struct sinfonia_printcmd28_hdr hdr;
	int ret;

	job->common.jobsize = sizeof(*job);

	/* Read in header */
	ret = read(data_fd, &hdr, sizeof(hdr));
	if (ret < 0 || ret != sizeof(hdr)) {
		if (ret == 0)
			return CUPS_BACKEND_CANCEL;
		ERROR("Read failed (%d/%d/%d)\n",
		      ret, 0, (int)sizeof(hdr));
		perror("ERROR: Read failed");
		return CUPS_BACKEND_CANCEL;
	}
	/* Validate header */
	if (le16_to_cpu(hdr.hdr.cmd) != SINFONIA_CMD_PRINTJOB ||
	    le16_to_cpu(hdr.hdr.len) != 28) {
		ERROR("Unrecognized data format!\n");
		return CUPS_BACKEND_CANCEL;
	}
	job->common.copies = le16_to_cpu(hdr.copies);
	job->jp.rows = le16_to_cpu(hdr.rows);
	job->jp.columns = le16_to_cpu(hdr.columns);
	job->jp.media = hdr.media;
	job->jp.oc_mode = hdr.options & SINFONIA_PRINT28_OC_MASK;
	job->jp.quality = hdr.options & SINFONIA_PRINT28_OPTIONS_HQ;
	job->jp.method = hdr.method;

	/* Allocate buffer */
	job->datalen = job->jp.rows * job->jp.columns * 3;
	job->databuf = malloc(job->datalen);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	{
		int remain = job->datalen;
		uint8_t *ptr = job->databuf;
		do {
			ret = read(data_fd, ptr, remain);
			if (ret < 0) {
				ERROR("Read failed (%d/%d/%d)\n",
				      ret, remain, job->datalen);
				perror("ERROR: Read failed");
				return CUPS_BACKEND_CANCEL;
			}
			ptr += ret;
			remain -= ret;
		} while (remain);
	}

	return CUPS_BACKEND_OK;
}

void sinfonia_cleanup_job(const void *vjob)
{
	const struct sinfonia_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);

	free((void*)job);
}

int sinfonia_docmd(struct sinfonia_usbdev *usbh,
		   uint8_t *cmd, int cmdlen,
		   uint8_t *resp, int resplen,
		   int *num)
{
	int ret;

	struct sinfonia_cmd_hdr *cmdhdr =  (struct sinfonia_cmd_hdr *) cmd;
	struct sinfonia_status_hdr *resphdr = (struct sinfonia_status_hdr *)resp;

	if ((ret = send_data(usbh->conn,
			      cmd, cmdlen))) {
		goto fail;
	}

	ret = read_data(usbh->conn,
			 (uint8_t *)resp, resplen, num);

	if (ret < 0)
		goto fail;

	if (resphdr->result != RESULT_SUCCESS) {
		INFO("Printer Status:  %02x (%s)\n", resphdr->status,
		     sinfonia_status_str(resphdr->status));
		INFO(" Result: 0x%02x  Error: 0x%02x (0x%02x/0x%02x = %s)\n",
		     resphdr->result, resphdr->error, resphdr->printer_major,
		     resphdr->printer_minor, usbh->error_codes(resphdr->printer_major, resphdr->printer_minor));
		ret = CUPS_BACKEND_FAILED;
		goto fail;
	}

	return CUPS_BACKEND_OK;
fail:
	ERROR("Failed to execute %s command\n", sinfonia_cmd_names(cmdhdr->cmd));
	return ret;
}

int sinfonia_flashled(struct sinfonia_usbdev *usbh)
{
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_status_hdr resp;
	int ret, num = 0;

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_FLASHLED);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(usbh,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
		return ret;
	}

	return CUPS_BACKEND_OK;
}

int sinfonia_canceljob(struct sinfonia_usbdev *usbh, int id)
{
	struct sinfonia_cancel_cmd cmd;
	struct sinfonia_status_hdr resp;
	int ret, num = 0;

	cmd.id = id;

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_CANCELJOB);
	cmd.hdr.len = cpu_to_le16(1);

	if ((ret = sinfonia_docmd(usbh,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
		return ret;
	}

	return CUPS_BACKEND_OK;
}

int sinfonia_getparam(struct sinfonia_usbdev *usbh, int target, uint32_t *param)
{
	struct sinfonia_getparam_cmd cmd;
	struct sinfonia_getparam_resp resp;
	int ret, num = 0;

	/* Set up command */
	cmd.target = target;

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_GETPARAM);
	cmd.hdr.len = cpu_to_le16(sizeof(struct sinfonia_getparam_cmd)-sizeof(cmd.hdr));

	if ((ret = sinfonia_docmd(usbh,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
//		ERROR("Unable to query param id %02x: %s\n",
//		      target, sinfonia_paramname(usbh, target));
		return ret;
	}
	*param = le32_to_cpu(resp.param);

	return CUPS_BACKEND_OK;
}

const char *sinfonia_paramname(struct sinfonia_usbdev *usbh,
			       int id)
{
	int i;
	for (i = 0 ; i < usbh->params_count ; i++) {
		if (usbh->params[i].id == id)
			return usbh->params[i].descr;

	}
	return "Unknown/Not Found!";
}

int sinfonia_dumpallparams(struct sinfonia_usbdev *usbh, int known)
{
	int i, ret;
	uint32_t param = 0;

	if (known) {
		for (i = 0 ; i < usbh->params_count ; i++) {
			ret = sinfonia_getparam(usbh, usbh->params[i].id, &param);
			if (ret)
				continue;
			DEBUG("%02x (%s): %08x\n", usbh->params[i].id,
			      usbh->params[i].descr, param);
		}
	} else {
		for (i = 0 ; i < 256 ; i++) {
			ret = sinfonia_getparam(usbh, i, &param);
			if (ret)
				continue;
			DEBUG("%02x (%s): %08x\n", i, sinfonia_paramname(usbh, i), param);
		}
	}
	return CUPS_BACKEND_OK;
}

int sinfonia_setparam(struct sinfonia_usbdev *usbh, int target, uint32_t param)
{
	struct sinfonia_setparam_cmd cmd;
	struct sinfonia_status_hdr resp;
	int ret, num = 0;

	/* Set up command */
	cmd.target = target;
	cmd.param = cpu_to_le32(param);

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_SETPARAM);
	cmd.hdr.len = cpu_to_le16(sizeof(struct sinfonia_setparam_cmd)-sizeof(cmd.hdr));

	if ((ret = sinfonia_docmd(usbh,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
//		ERROR("Unable to query param id %02x: %s\n",
//		      target, sinfonia_paramname(usbh, target));
	}

	return CUPS_BACKEND_OK;
}

int sinfonia_getfwinfo(struct sinfonia_usbdev *usbh)
{
	struct sinfonia_fwinfo_cmd  cmd;
	struct sinfonia_fwinfo_resp resp;
	int num = 0;
	int i;
	int last = FWINFO_TARGET_PRINT_TABLES2;

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_FWINFO);
	cmd.hdr.len = cpu_to_le16(1);

	resp.hdr.payload_len = 0;

	INFO("FW Information:\n");

	if (usbh->conn->type == P_SHINKO_S6145) last = FWINFO_TARGET_PRINT_TABLES;
	if (usbh->conn->type == P_SHINKO_S2245) last = FWINFO_TARGET_DSP;

	for (i = FWINFO_TARGET_MAIN_BOOT ; i <= last ; i++) {
		int ret;
		cmd.target = i;
		resp.major = 0;

		if ((ret = sinfonia_docmd(usbh,
					  (uint8_t*)&cmd, sizeof(cmd),
					  (uint8_t*)&resp, sizeof(resp),
					  &num))) {
			continue;
		}

		if (resp.major == 0)
			continue;

		if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct sinfonia_fwinfo_resp) - sizeof(struct sinfonia_status_hdr)))
			continue;

		INFO(" %s\t ver %02x.%02x\n", sinfonia_fwinfo_targets(i),
		     resp.major, resp.minor);
#if 0
		INFO("  name:    '%s'\n", resp.name);
		INFO("  type:    '%s'\n", resp.type);
		INFO("  date:    '%s'\n", resp.date);
		INFO("  version: %02x.%02x (CRC %04x)\n", resp.major, resp.minor,
		     le16_to_cpu(resp.checksum));
#endif
	}
	return CUPS_BACKEND_OK;
}

int sinfonia_geterrorlog(struct sinfonia_usbdev *usbh)
{
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_errorlog_resp resp;
	int ret, num = 0;
	int i;

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_ERRORLOG);
	cmd.len = cpu_to_le16(0);

	resp.hdr.payload_len = 0;

	if ((ret = sinfonia_docmd(usbh,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
		return ret;
	}

	if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct sinfonia_errorlog_resp) - sizeof(struct sinfonia_status_hdr)))
		return -2;

	INFO("Stored Error Events: %u entries:\n", resp.count);
	for (i = 0 ; i < resp.count ; i++) {
		INFO(" %02d: @ %08u prints : 0x%02x/0x%02x (%s)\n", i,
		     le32_to_cpu(resp.items[i].print_counter),
		     resp.items[i].major, resp.items[i].minor,
		     usbh->error_codes(resp.items[i].major, resp.items[i].minor));
	}

	return CUPS_BACKEND_OK;
}

int sinfonia_resetcurve(struct sinfonia_usbdev *usbh, int target, int id)
{
	struct sinfonia_reset_cmd cmd;
	struct sinfonia_status_hdr resp;
	int ret, num = 0;

	cmd.target = target;
	cmd.curveid = id;
	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_RESET);
	cmd.hdr.len = cpu_to_le16(2);

	if ((ret = sinfonia_docmd(usbh,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
		return ret;
	}

	return CUPS_BACKEND_OK;
}

int sinfonia_gettonecurve(struct sinfonia_usbdev *usbh, int type, char *fname)
{
	struct sinfonia_readtone_cmd cmd;
	struct sinfonia_readtone_resp resp;
	int ret, num = 0;

	uint8_t *data;
	uint16_t curves[TONE_CURVE_SIZE] = { 0 };

	int i,j;

	cmd.target = type;
	cmd.curveid = TONE_CURVE_ID;

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_READTONE);
	cmd.hdr.len = cpu_to_le16(2);

	resp.hdr.payload_len = 0;

	INFO("Dump %s Tone Curve to '%s'\n", sinfonia_tonecurve_statuses(type), fname);

	if ((ret = sinfonia_docmd(usbh,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
		return ret;
	}

	if (le16_to_cpu(resp.hdr.payload_len) != (sizeof(struct sinfonia_readtone_resp) - sizeof(struct sinfonia_status_hdr)))
		return -2;

	resp.total_size = le16_to_cpu(resp.total_size);

	data = malloc(resp.total_size * 2);
	if (!data) {
		ERROR("Memory Allocation Failure!\n");
		return -1;
	}

	i = 0;
	while (i < resp.total_size) {
		ret = read_data(usbh->conn,
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
		close(tc_fd);
	}

done:
	free(data);
	return ret;
}

int sinfonia_button_set(struct sinfonia_usbdev *dev, int enable)
{
	struct sinfonia_button_cmd cmd;
	struct sinfonia_status_hdr resp;
	int ret, num = 0;

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_BUTTON);
	cmd.hdr.len = cpu_to_le16(1);

	cmd.enabled = enable;

	if ((ret = sinfonia_docmd(dev,
				(uint8_t*)&cmd, sizeof(cmd),
				(uint8_t*)&cmd, sizeof(resp),
				&num))) {
		return ret;
	}

	return CUPS_BACKEND_OK;
}

int sinfonia_settonecurve(struct sinfonia_usbdev *usbh, int target, char *fname)
{
	struct sinfonia_update_cmd cmd;
	struct sinfonia_status_hdr resp;
	int ret, num = 0;

	INFO("Set %s from '%s'\n", sinfonia_update_targets(target), fname);

	uint16_t *data = malloc(TONE_CURVE_SIZE * sizeof(uint16_t));
	if (!data) {
		ERROR("Memory Allocation Failure!\n");
		return -1;
	}

	/* Read in file */
	if ((ret = dyesub_read_file(fname, data, TONE_CURVE_SIZE * sizeof(uint16_t), NULL))) {
		ERROR("Failed to read Tone Curve file\n");
		goto done;
	}

	/* Byteswap data to local CPU.. */
	for (ret = 0; ret < TONE_CURVE_SIZE ; ret++) {
		data[ret] = be16_to_cpu(data[ret]);
	}

	/* Set up command */
	cmd.target = target;
	cmd.curve_id = TONE_CURVE_ID;
	cmd.reserved[0] = cmd.reserved[1] = cmd.reserved[2] = 0;
	cmd.reset = 0;
	cmd.size = cpu_to_le32(TONE_CURVE_SIZE * sizeof(uint16_t));

	cmd.hdr.cmd = cpu_to_le16(SINFONIA_CMD_UPDATE);
	cmd.hdr.len = cpu_to_le16(sizeof(struct sinfonia_update_cmd)-sizeof(cmd.hdr));

	/* Byteswap data to format printer is expecting.. */
	for (ret = 0; ret < TONE_CURVE_SIZE ; ret++) {
		data[ret] = cpu_to_le16(data[ret]);
	}

	if ((ret = sinfonia_docmd(usbh,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
		return ret;
	}

	/* Sent transfer */
	if ((ret = send_data(usbh->conn,
			     (uint8_t *) data, TONE_CURVE_SIZE * sizeof(uint16_t)))) {
		goto done;
	}

done:
	free(data);

	return ret;
}

int sinfonia_query_media(struct sinfonia_usbdev *dev,
			 void *resp)
{
	struct sinfonia_cmd_hdr cmd;
	int i, num;

	struct sinfonia_6x45_mediainfo_resp *media = resp;

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_MEDIAINFO);
	cmd.len = cpu_to_le16(0);

	if (sinfonia_docmd(dev,
			   (uint8_t*)&cmd, sizeof(cmd),
			   (uint8_t*)media, sizeof(*media),
			   &num)) {
		return CUPS_BACKEND_FAILED;
	}

	/* Byteswap media descriptor.. */
	for (i = 0 ; i < media->count ; i++) {
		media->items[i].columns = le16_to_cpu(media->items[i].columns);
		media->items[i].rows = le16_to_cpu(media->items[i].rows);
	}

	return CUPS_BACKEND_OK;
}

static const char *dummy_error_codes(uint8_t major, uint8_t minor)
{
	UNUSED(major);
	UNUSED(minor);
	return "Unknown";
}

int sinfonia_query_serno(struct dyesub_connection *conn, char *buf, int buf_len)
{
	struct sinfonia_cmd_hdr cmd;
	struct sinfonia_getserial_resp resp;
	int ret, num = 0;

	struct sinfonia_usbdev sdev = {
		.error_codes = dummy_error_codes,
		.conn = conn,
	};

	cmd.cmd = cpu_to_le16(SINFONIA_CMD_GETSERIAL);
	cmd.len = cpu_to_le16(0);

	if ((ret = sinfonia_docmd(&sdev,
				  (uint8_t*)&cmd, sizeof(cmd),
				  (uint8_t*)&resp, sizeof(resp),
				  &num))) {
		return ret;
	}

	/* Copy and Null-terminate */
	num = (buf_len > (int)sizeof(resp.data)) ? (int)sizeof(resp.data) : (buf_len - 1);
	memcpy(buf, resp.data, num);
	buf[num] = 0;

	return CUPS_BACKEND_OK;
}

const char *sinfonia_update_targets (uint8_t v) {
	switch (v) {
	case UPDATE_TARGET_TONE_USER:
		return "User Tone Curve";
	case UPDATE_TARGET_TONE_CURRENT:
		return "Current Tone Curve";
	case UPDATE_TARGET_LAM_USER:
		return "User Lamination Data";
	case UPDATE_TARGET_LAM_CUR:
		return "Current Lamination Data";
	case UPDATE_TARGET_LAM_DEF:
		return "Default Lamination Data";
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
	case ERROR_INAPP_COMMAND:
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
	case PRINT_METHOD_COMBO_4:
		return "4up";
	case PRINT_METHOD_SPLIT:
		return "Split";
	case PRINT_METHOD_DOUBLE:
		return "Double";
	default:
		return "Unknown";
	}
}

const char *sinfonia_print_modes(uint8_t v) {
	switch (v) {
	case PRINT_MODE_NO_OC:
		return "No Overcoat";
	case PRINT_MODE_GLOSSY:
		return "Glossy";
	case PRINT_MODE_MATTE:
		return "Matte";
	default:
		return "Unknown";
	}
}

const char *sinfonia_fwinfo_targets (uint8_t v) {
	switch (v) {
	case FWINFO_TARGET_MAIN_BOOT:
		return "Main Boot   ";
	case FWINFO_TARGET_MAIN_APP:
		return "Main App    ";
	case FWINFO_TARGET_PRINT_TABLES:
	case FWINFO_TARGET_PRINT_TABLES2:  // Seen on EK70xx
		return "Print Tables";
	case FWINFO_TARGET_DSP:
		return "DSP         ";
	case FWINFO_TARGET_USB:
		return "USB         ";
	default:
		return "Unknown     ";
	}
}

const char *sinfonia_print_codes (uint8_t v, int eightinch) {
	if (eightinch) {
		switch (v) {
		case CODE_8x10:
			return "8x10";
		case CODE_8x12:
		case CODE_8x12K:
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
	case SINFONIA_CMD_GETSERIAL2:
		return "Get Serial Number";
	case SINFONIA_CMD_PRINTSTAT:
		return "Get Print ID Status";
	case SINFONIA_CMD_MEMORYBANK:
		return "Get Memory Bank Info";

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
	case SINFONIA_CMD_SETLAMSTR:
		return "Set Lamination String";
	case SINFONIA_CMD_COMMPPA:
//	case SINFONIA_CMD_SETCUTLIST:
		return "Communication PPA / Set Cut List";
	case SINFONIA_CMD_SETPPAPARM:
//	case SINFONIA_CMD_WAKEUPSTBY::
		return "Set PPA Parameter / Set Wakeup Standby";
	case SINFONIA_CMD_BACKPRINT:
		return "Set Backprint String";
	case SINFONIA_CMD_UNKNOWN4C:
		return "UKNOWN 400C"; // XXX
	case SINFONIA_CMD_GETCORR:
		return "Get Image Correction Parameter";
	case SINFONIA_CMD_GETEEPROM:
	case SINFONIA_CMD_GETEEPROM2:
		return "Get EEPROM Backup Parameter";
	case SINFONIA_CMD_SETEEPROM:
	case SINFONIA_CMD_SETEEPROM2:
		return "Set EEPROM Backup Parameter";
	case SINFONIA_CMD_SETTIME:
		return "Time Setting";
	case SINFONIA_CMD_UNIVERSAL:
		return "Unicersal Command";

	case SINFONIA_CMD_USBFWDL:
		return "USB Firmware Download";
	case SINFONIA_CMD_MAINTPERM:
		return "Maintenance Permission";
	case SINFONIA_CMD_GETUNIQUE:
		return "Get Unique String";

	case SINFONIA_CMD_SELFDIAG:
		return "Execute Self-Diagnostic";
	case SINFONIA_CMD_FWINFO:
		return "Get Firmware Info";
	case SINFONIA_CMD_UPDATE:
		return "Update";
	case SINFONIA_CMD_SETUNIQUE:
		return "Set Unique String";
	case SINFONIA_CMD_RESETERR:
		return "Reset Error Log";
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
	case KODAK7_MEDIA_6R:
		return "Kodak 6R";

	default:
		return "Unknown";
	}
	return "Unknown";
}

int kodak6_mediamax(int type)
{
	switch(type) {
	case KODAK6_MEDIA_5R:
	case KODAK6_MEDIA_6R:
	case KODAK6_MEDIA_6TR2:
		return 375;
	case KODAK7_MEDIA_5R:
	case KODAK7_MEDIA_6R:
		return 570;
	default:
		return CUPS_BACKEND_OK;
	}
}

void kodak6_dumpmediacommon(int type)
{
	switch (type) {
	case KODAK6_MEDIA_5R:
		INFO("Media type: 5R (Kodak 189-9160 or equivalent)\n");
		break;
	case KODAK6_MEDIA_6R:
		INFO("Media type: 6R (Kodak 197-4096 or equivalent)\n");
		break;
	case KODAK6_MEDIA_6TR2:
		INFO("Media type: 6R (Kodak 396-2941 or equivalent)\n");
		break;
	case KODAK7_MEDIA_5R:
		INFO("Media type: 5R (Kodak 164-9011 or equivalent)\n");
		break;
	case KODAK7_MEDIA_6R:
		INFO("Media type: 6R (Kodak 659-9047 or equivalent)\n");
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
