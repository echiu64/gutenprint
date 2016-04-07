/*
 *   Citizen CW-01 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2014-2015 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND cw01_backend

#include "backend_common.h"

#define USB_VID_CITIZEN      0x1343
#define USB_PID_CITIZEN_CW01 0x0002 // Maybe others?
//#define USB_PID_OLMEC_OP900 XXXX

/* Private data stucture */
struct cw01_spool_hdr {
	uint8_t  type; /* 0x00 -> 0x06 */
	uint8_t  res; /* vertical resolution; 0x00 == 334dpi, 0x01 == 600dpi */
	uint8_t  copies; /* number of prints */
	uint8_t  null0;
	uint32_t plane_len; /* LE */
	uint8_t  null1[4];
};
#define DPI_334 0
#define DPI_600 1

#define TYPE_DSC  0
#define TYPE_L    1
#define TYPE_PC   2
#define TYPE_2DSC 3
#define TYPE_3L   4
#define TYPE_A5   5
#define TYPE_A6   6

#define SPOOL_PLANE_HDR_LEN 1064
#define PRINTER_PLANE_HDR_LEN 1088

struct cw01_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;

	uint8_t *databuf;
	struct cw01_spool_hdr hdr;
};

struct cw01_cmd {
	uint8_t esc; /* Fixed at ascii ESC, aka 0x1B */
	uint8_t p;   /* Fixed at ascii 'P' aka 0x50 */
	uint8_t arg1[6];
	uint8_t arg2[16];
	uint8_t arg3[8]; /* Decimal value of arg4's length, or empty */
	uint8_t arg4[0]; /* Extra payload if arg3 is non-empty
			    Doesn't have to be sent in the same URB */

	/* All unused elements are set to 0x20 (ie ascii space) */
};

#define min(__x, __y) ((__x) < (__y)) ? __x : __y

static void cw01_build_cmd(struct cw01_cmd *cmd, char *arg1, char *arg2, uint32_t arg3_len)
{
	memset(cmd, 0x20, sizeof(*cmd));
	cmd->esc = 0x1b;
	cmd->p = 0x50;
	memcpy(cmd->arg1, arg1, min(strlen(arg1), sizeof(cmd->arg1)));
	memcpy(cmd->arg2, arg2, min(strlen(arg2), sizeof(cmd->arg2)));
	if (arg3_len) {
		char buf[9];
		snprintf(buf, sizeof(buf), "%08u", arg3_len);
		memcpy(cmd->arg3, buf, 8);
	}

}

static void cw01_cleanup_string(char *start, int len)
{
	char *ptr = strchr(start, 0x0d);

	if (ptr && (ptr - start < len)) {
		*ptr = 0x00; /* If there is a <CR>, terminate there */
		len = ptr - start;
	} else {
		start[--len] = 0x00;  /* force null-termination */
	}

	/* Trim trailing spaces */
	while (len && start[len-1] == ' ') {
		start[--len] = 0;
	}
}

static char *cw01_media_types(char *str)
{
	char tmp[4];
	int i;

	memcpy(tmp, str + 4, 3);
	tmp[3] = 0;

	i = atoi(tmp);

	switch (i) {
	case 100: return "UNK 100";
	case 110: return "UNK 110";
	case 200: return "?? 5x3.5 (L)";
	case 210: return "?? 5x7 (2L)";
	case 300: return "?? 6x4 (PC)";
	case 400: return "?? 6x9 (A5W)";
	default:
		break;
	}

	return "Unknown type";
}

static char *cw01_statuses(char *str)
{
	char tmp[6];
	int i;
	memcpy(tmp, str, 5);
	tmp[5] = 0;

	i = atoi(tmp);

	switch (i) {
	case 0:	return "Idle";
	case 1:	return "Printing";
	case 500: return "Cooling Print Head";
	case 510: return "Cooling Paper Motor";
	case 1000: return "Cover Open";
	case 1010: return "No Scrap Box";
	case 1100: return "Paper End";
	case 1200: return "Ribbon End";
	case 1300: return "Paper Jam";
	case 1400: return "Ribbon Error";
	case 1500: return "Paper Definition Error";
	case 1600: return "Data Error";
	case 2000: return "Head Voltage Error";
	case 2100: return "Head Position Error";
	case 2200: return "Power Supply Fan Error";
	case 2300: return "Cutter Error";
	case 2400: return "Pinch Roller Error";
	case 2500: return "Abnormal Head Temperature";
	case 2600: return "Abnormal Media Temperature";
	case 2610: return "Abnormal Paper Motor Temperature";
	case 2700: return "Ribbon Tension Error";
	case 2800: return "RF-ID Module Error";
	case 3000: return "System Error";
	default:
		break;
	}

	return "Unkown Error";
}

static int cw01_do_cmd(struct cw01_ctx *ctx,
			  struct cw01_cmd *cmd,
			  uint8_t *data, int len)
{
	int ret;

	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*)cmd, sizeof(*cmd))))
		return ret;

	if (data && len)
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     data, len)))
			return ret;

	return CUPS_BACKEND_OK;
}

static uint8_t *cw01_resp_cmd(struct cw01_ctx *ctx,
			      struct cw01_cmd *cmd,
			      int *len)
{
	char tmp[9];
	uint8_t *respbuf;

	int ret, i, num = 0;

	memset(tmp, 0, sizeof(tmp));

	if ((ret = cw01_do_cmd(ctx, cmd, NULL, 0)))
		return NULL;

	/* Read in the response header */
	ret = read_data(ctx->dev, ctx->endp_up,
			(uint8_t*)tmp, 8, &num);
	if (ret < 0)
		return NULL;

	if (num != 8) {
		ERROR("Short read! (%d/%d)\n", num, 8);
		return NULL;
	}

	i = atoi(tmp);  /* Length of payload in bytes, possibly padded */
	respbuf = malloc(i);
	if (!respbuf) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}

	/* Read in the actual response */
	ret = read_data(ctx->dev, ctx->endp_up,
			respbuf, i, &num);
	if (ret < 0) {
		free(respbuf);
		return NULL;
	}

	if (num != i) {
		ERROR("Short read! (%d/%d)\n", num, i);
		free(respbuf);
		return NULL;
	}

	*len = num;
	return respbuf;
}

static int cw01_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct cw01_cmd cmd;
	uint8_t *resp;
	int len = 0;

	struct cw01_ctx ctx = {
		.dev = dev,
		.endp_up = endp_up,
		.endp_down = endp_down,
	};

	/* Get Serial Number */
	cw01_build_cmd(&cmd, "INFO", "SERIAL_NUMBER", 0);

	resp = cw01_resp_cmd(&ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	strncpy(buf, (char*)resp, buf_len);
	buf[buf_len-1] = 0;

	free(resp);

	return CUPS_BACKEND_OK;
}

static void *cw01_init(void)
{
	struct cw01_ctx *ctx = malloc(sizeof(struct cw01_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct cw01_ctx));

	return ctx;
}

static void cw01_attach(void *vctx, struct libusb_device_handle *dev,
			      uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct cw01_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);
	
	ctx->type = lookup_printer_type(&cw01_backend,
					desc.idVendor, desc.idProduct);
}

static void cw01_teardown(void *vctx) {
	struct cw01_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	free(ctx);
}

static int cw01_read_parse(void *vctx, int data_fd) {
	struct cw01_ctx *ctx = vctx;
	int i, j, remain;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	i = read(data_fd, (uint8_t*) &ctx->hdr, sizeof(struct cw01_spool_hdr));
	
	if (i < 0)
		return i;
	if (i == 0)
		return CUPS_BACKEND_CANCEL;

	if (i < (int)sizeof(struct cw01_spool_hdr))
		return CUPS_BACKEND_CANCEL;
	
	if (ctx->hdr.type > 0x06 || ctx->hdr.res > 0x01) {
		ERROR("Unrecognized header data format!\n");
		return CUPS_BACKEND_CANCEL;
	}
	ctx->hdr.plane_len = le32_to_cpu(ctx->hdr.plane_len);
	remain = ctx->hdr.plane_len * 3;
	ctx->databuf = malloc(remain);
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_CANCEL;
	}

	j = 0;
	while (remain) {
		i = read(data_fd, ctx->databuf + j, remain);

		if (i < 0)
			return i;

		remain -= i;
		j += i;
	}

	return CUPS_BACKEND_OK;
}

static int cw01_main_loop(void *vctx, int copies) {
	struct cw01_ctx *ctx = vctx;
	int ret;
	struct cw01_cmd cmd;
	uint8_t *resp = NULL;
	int len = 0;
	uint32_t tmp;
	uint8_t *ptr;
	char buf[9];
	uint8_t plane_hdr[PRINTER_PLANE_HDR_LEN];

	if (!ctx)
		return CUPS_BACKEND_FAILED;

top:

	if (resp) free(resp);

	/* Query status */
	cw01_build_cmd(&cmd, "STATUS", "", 0);
	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;
	cw01_cleanup_string((char*)resp, len);

	/* If we're not idle */
	if (strcmp("00000", (char*)resp)) {
		if (!strcmp("00001", (char*)resp)) {
			free(resp);
			/* Query buffer state */
			cw01_build_cmd(&cmd, "INFO", "FREE_PBUFFER", 0);
			resp = cw01_resp_cmd(ctx, &cmd, &len);
			if (!resp)
				return CUPS_BACKEND_FAILED;
			cw01_cleanup_string((char*)resp, len);
	
			/* Check to see if we have sufficient buffers */
			// XXX audit these rules...?
			if (!strcmp("FBP00", (char*)resp) ||
			    (ctx->hdr.res == DPI_600 && !strcmp("FBP01", (char*)resp))) {
				INFO("Insufficient printer buffers, retrying...\n");
				sleep(1);
				goto top;
			}
		} else {
			ERROR("Printer Status: %s\n", cw01_statuses((char*)resp));
			free(resp);
			return CUPS_BACKEND_RETRY_CURRENT;
		}
	}

	free(resp);
	/* Get Vertical resolution */
	cw01_build_cmd(&cmd, "INFO", "RESOLUTION_V", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

#if 0
	if (ctx->hdr.res == DPI_600 && strcmp("RV0334", *char*)resp) {
		ERROR("600DPI prints not yet supported, need 600DPI CWD load");
		return CUPS_BACKEND_CANCEL;
	}
#endif

	free(resp);
	resp = NULL;

	/* Set print quantity */ // XXX check against remaining print count

	cw01_build_cmd(&cmd, "CNTRL", "QTY", 8);
	snprintf(buf, sizeof(buf), "%07d\r", copies);
	ret = cw01_do_cmd(ctx, &cmd, (uint8_t*) buf, 8);
	if (ret)
		return CUPS_BACKEND_FAILED;

	/* Cutter control.  ??? */
	// cw01_build_cmd(&cmd, "CNTRL", "CUTTER", 8);
	//snprintf(buf, sizeof(buf), "%08d", ???);
	//ret = cw01_do_cmd(ctx, &cmd, (uint8_t*) buf, 8);
	//if (ret)
	//	return CUPS_BACKEND_FAILED;

	/* Start sending image data */
	ptr = ctx->databuf;

	/* Generate plane header (same for all planes) */
	tmp = cpu_to_le32(ctx->hdr.plane_len) + 24;
	memset(plane_hdr, 0, PRINTER_PLANE_HDR_LEN);
	plane_hdr[0] = 0x42;
	plane_hdr[1] = 0x4d;
	memcpy(plane_hdr + 2, &tmp, sizeof(tmp));
	plane_hdr[10] = 0x40;
	plane_hdr[11] = 0x04;
	memcpy(plane_hdr + 14, ptr, SPOOL_PLANE_HDR_LEN);

	/******** Plane 1 */
	cw01_build_cmd(&cmd, "IMAGE", "YPLANE", ctx->hdr.plane_len - SPOOL_PLANE_HDR_LEN + PRINTER_PLANE_HDR_LEN);
	ret = cw01_do_cmd(ctx, &cmd, plane_hdr, PRINTER_PLANE_HDR_LEN);
	if (ret)
		return CUPS_BACKEND_FAILED;

	/* Send plane data */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ptr + SPOOL_PLANE_HDR_LEN, ctx->hdr.plane_len - SPOOL_PLANE_HDR_LEN)))
			return CUPS_BACKEND_FAILED;

	ptr += ctx->hdr.plane_len;

	/******** Plane 2 */
	cw01_build_cmd(&cmd, "IMAGE", "MPLANE", ctx->hdr.plane_len - SPOOL_PLANE_HDR_LEN + PRINTER_PLANE_HDR_LEN);
	ret = cw01_do_cmd(ctx, &cmd, plane_hdr, PRINTER_PLANE_HDR_LEN);
	if (ret)
		return CUPS_BACKEND_FAILED;

	/* Send plane data */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ptr + SPOOL_PLANE_HDR_LEN, ctx->hdr.plane_len - SPOOL_PLANE_HDR_LEN)))
			return CUPS_BACKEND_FAILED;

	ptr += ctx->hdr.plane_len;

	/******** Plane 3 */
	cw01_build_cmd(&cmd, "IMAGE", "CPLANE", ctx->hdr.plane_len - SPOOL_PLANE_HDR_LEN + PRINTER_PLANE_HDR_LEN);
	ret = cw01_do_cmd(ctx, &cmd, plane_hdr, PRINTER_PLANE_HDR_LEN);
	if (ret)
		return CUPS_BACKEND_FAILED;

	/* Send plane data */
	if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ptr + SPOOL_PLANE_HDR_LEN, ctx->hdr.plane_len - SPOOL_PLANE_HDR_LEN)))
			return CUPS_BACKEND_FAILED;

	/* ptr += ctx->hdr.plane_len; */

	/* Start print */
	cw01_build_cmd(&cmd, "CNTRL", "START", 0);
	ret = cw01_do_cmd(ctx, &cmd, NULL, 0);
	if (ret)
		return CUPS_BACKEND_FAILED;

	INFO("Print complete\n");

	if (resp) free(resp);

	return CUPS_BACKEND_OK;
}

static int cw01_get_info(struct cw01_ctx *ctx)
{
	struct cw01_cmd cmd;
	uint8_t *resp;
	int len = 0;

	/* Get Serial Number */
	cw01_build_cmd(&cmd, "INFO", "SERIAL_NUMBER", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Serial Number: '%s'\n", (char*)resp);

	free(resp);

	/* Get Firmware Version */
	cw01_build_cmd(&cmd, "INFO", "FVER", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Firmware Version: '%s'\n", (char*)resp);

	free(resp);

	/* Get Sensor Info */
	cw01_build_cmd(&cmd, "INFO", "SENSOR", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Sensor Info: '%s'\n", (char*)resp);
	// XXX parse this out. Each token is 'XXX-###' delimited by '; '

	free(resp);

	/* Get Horizonal resolution */
	cw01_build_cmd(&cmd, "INFO", "RESOLUTION_H", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Horizontal Resolution: '%s' dpi\n", (char*)resp + 3);

	free(resp);

	/* Get Vertical resolution */
	cw01_build_cmd(&cmd, "INFO", "RESOLUTION_V", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Vertical Resolution: '%s' dpi\n", (char*)resp + 3);

	free(resp);

	/* Get Media Color offset */
	cw01_build_cmd(&cmd, "INFO", "MCOLOR", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Media Color Offset: '%02x%02x%02x%02x'\n", *(resp+2), *(resp+3),
	     *(resp+4), *(resp+5));

	free(resp);

	/* Get Media Lot */
	cw01_build_cmd(&cmd, "INFO", "MLOT", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Media Lot Code: '%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x'\n", 
	     *(resp+2), *(resp+3), *(resp+4), *(resp+5), *(resp+6), *(resp+7),
	     *(resp+8), *(resp+9), *(resp+10), *(resp+11), *(resp+12), *(resp+13));

	free(resp);

	/* Get Media ID Set (?) */
	cw01_build_cmd(&cmd, "MNT_RD", "MEDIA_ID_SET", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Media ID(?): '%s'\n", (char*)resp+4);

	free(resp);

	/* Get Color Control Data Version */
	cw01_build_cmd(&cmd, "TBL_RD", "Version", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Color Data Version: '%s'\n", (char*)resp);

	free(resp);

	/* Get Color Control Data Checksum */
	cw01_build_cmd(&cmd, "MNT_RD", "CTRLD_CHKSUM", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Color Data Checksum: '%s'\n", (char*)resp);

	free(resp);

	return CUPS_BACKEND_OK;
}

static int cw01_get_status(struct cw01_ctx *ctx)
{
	struct cw01_cmd cmd;
	uint8_t *resp;
	int len = 0;

	/* Generate command */
	cw01_build_cmd(&cmd, "STATUS", "", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Printer Status: %s => %s\n", (char*)resp, cw01_statuses((char*)resp));

	free(resp);

	/* Get remaining prints in this job */
	cw01_build_cmd(&cmd, "INFO", "PQTY", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Prints Remaining in job: '%s'\n", (char*)resp + 4);

	free(resp);

	/* Generate command */
	cw01_build_cmd(&cmd, "INFO", "FREE_PBUFFER", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Free Buffers: '%s'\n", (char*)resp + 3);

	free(resp);

	/* Get Media Info */
	cw01_build_cmd(&cmd, "INFO", "MEDIA", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Media Type: '%s'\n", cw01_media_types((char*)resp));

	free(resp);

	/* Get Media remaining */
	cw01_build_cmd(&cmd, "INFO", "MQTY", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Prints Remaining: '%s'\n", (char*)resp + 4);

	free(resp);

	return 0;
}

static int cw01_get_counters(struct cw01_ctx *ctx)
{
	struct cw01_cmd cmd;
	uint8_t *resp;
	int len = 0;

	/* Generate command */
	cw01_build_cmd(&cmd, "MNT_RD", "COUNTER_LIFE", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("Lifetime Counter: '%s'\n", (char*)resp+2);

	free(resp);

	/* Generate command */
	cw01_build_cmd(&cmd, "MNT_RD", "COUNTER_A", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("A Counter: '%s'\n", (char*)resp+2);

	free(resp);

	/* Generate command */
	cw01_build_cmd(&cmd, "MNT_RD", "COUNTER_B", 0);

	resp = cw01_resp_cmd(ctx, &cmd, &len);
	if (!resp)
		return CUPS_BACKEND_FAILED;

	cw01_cleanup_string((char*)resp, len);

	INFO("B Counter: '%s'\n", (char*)resp+2);

	free(resp);

	return CUPS_BACKEND_OK;
}

static int cw01_clear_counter(struct cw01_ctx *ctx, char counter)
{
	struct cw01_cmd cmd;
	char msg[4];
	int ret;

	/* Generate command */
	cw01_build_cmd(&cmd, "MNT_WT", "COUNTER_CLEAR", 4);
	msg[0] = 'C';
	msg[1] = counter;
	msg[2] = 0x0d; /* ie carriage return, ASCII '\r' */
	msg[3] = 0x00;

	if ((ret = cw01_do_cmd(ctx, &cmd, (uint8_t*)msg, 4)))
		return ret;

	return 0;
}


static void cw01_cmdline(void)
{
	DEBUG("\t\t[ -i ]           # Query printer info\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -n ]           # Query counters\n");
	DEBUG("\t\t[ -N A|B|M ]     # Clear counter A/B/M\n");
}

static int cw01_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct cw01_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	/* Reset arg parsing */
	optind = 1;
	opterr = 0;
	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "inN:s")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'i':
			j = cw01_get_info(ctx);
			break;
		case 'n':
			j = cw01_get_counters(ctx);
			break;
		case 'N':
			if (optarg[0] != 'A' &&
			    optarg[0] != 'B')
				return CUPS_BACKEND_FAILED;
			j = cw01_clear_counter(ctx, optarg[0]);
			break;
		case 's':
			j = cw01_get_status(ctx);
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
}

/* Exported */
struct dyesub_backend cw01_backend = {
	.name = "Citizen CW-01",
	.version = "0.12",
	.uri_prefix = "citizencw01",
	.cmdline_usage = cw01_cmdline,
	.cmdline_arg = cw01_cmdline_arg,
	.init = cw01_init,
	.attach = cw01_attach,
	.teardown = cw01_teardown,
	.read_parse = cw01_read_parse,
	.main_loop = cw01_main_loop,
	.query_serno = cw01_query_serno,
	.devices = {
	{ USB_VID_CITIZEN, USB_PID_CITIZEN_CW01, P_CITIZEN_CW01, ""},
//	{ USB_VID_CITIZEN, USB_PID_OLMEC_OP900, P_CITIZEN_CW01, ""},
	{ 0, 0, 0, ""}
	}
};

/* 

Basic spool file format:

TT RR NN 00 XX XX XX XX  00 00 00 00              <- FILE header.

  NN          : copies (0x01 or more)
  RR          : resolution; 0 == 334 dpi, 1 == 600dpi
  TT          : type 0x02 == 4x6, 0x01 == 5x3.5
  XX XX XX XX : plane length (LE)
                plane length * 3 + 12 == file length.

Followed by three planes, each with this header:

28 00 00 00 00 08 00 00  RR RR 00 00 01 00 08 00 
00 00 00 00 00 00 00 00  5a 33 00 00 YY YY 00 00
00 01 00 00 00 00 00 00

  RR RR       : rows in LE format
  YY YY       : 0x335a (334dpi) or 0x5c40 (600dpi)

Followed by 1024 bytes of color tables:

 ff ff ff 00 ... 00 00 00 00 

1024+40 = 1064 bytes of header per plane.

Always have 2048 columns of data.

followed by (2048 * rows) bytes of data.

*/
