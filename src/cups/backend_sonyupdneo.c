/*
 *   Sony UP-D series (new) Photo Printer CUPS backend -- libusb-1.0 version
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

#define BACKEND sonyupdneo_backend

#include "backend_common.h"

/* Private data structures */
struct updneo_printjob {
	struct dyesub_job_common common;

	uint8_t *databuf;
	int datalen;
	uint8_t *hdrbuf;
	int hdrlen;
	uint8_t *ftrbuf;
	int ftrlen;

	uint16_t rows;
	uint16_t cols;
};

struct updneo_sts {
	uint16_t scdiv;
	uint32_t scsyv;
	char     scsno[17]; /* 16 char string, leading 0s, possibly trailing -- */
	char     scsys[23]; /* 22 char string, mostly unknown */
	uint16_t scmds[5];
	uint16_t scprs;
	uint16_t scses;
	uint16_t scwts;
	uint16_t scjbs;
	uint8_t  scsye;
	uint16_t scmde;
	uint8_t  scmce;
	char     scjbi[17]; /* 16 char string, unknown */
	char     scsyi[31]; /* 30 char string, max resolutions * 3, one more 6-char field */
	uint32_t scsvi[2];  /* 2* 6char numbers */
	uint32_t scmni[2];  /* 2* 6char numbers */
	char     sccai[15]; /* 14 char string, unknown */
	uint16_t scgai;
	uint8_t  scgsi;
	uint32_t scmdi; /* xxxyyy : xxx is ribbon type, yyy is paper type?  */
	uint32_t scqti;
	uint32_t spuqi;
};

struct updneo_ctx {
	struct dyesub_connection *conn;

	int native_bpp;

	struct updneo_sts sts;

	struct marker marker;
};

/* Forward declaration */
static int updneo_get_status(struct updneo_ctx *ctx);

/* Now for the code */
static const char* updneo_decode_errors(uint16_t mde, uint8_t mce, uint8_t sye)
{
	if (!mde && !mce && sye)
		return "None";
	if (mde == 0x0800 || mce == 0x1)
		return "Cover open";
	if (mde == 0x0a00)
		return "No paper loaded";
	if (mde == 0x0002)
		return "No ribbon loaded";
	if (mde == 0x0300)
		return "No media loaded";
	if (mde == 0x2000)
		return "Job does not match installed media";

	return "Unknown";
}

static void* updneo_init(void)
{
	struct updneo_ctx *ctx = malloc(sizeof(struct updneo_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct updneo_ctx));
	return ctx;
}

static const char *updneo_medias(uint32_t mdi)
{
	uint32_t mdi2 = mdi >> 12;
	mdi2 &= 0xfff;

	switch(mdi2) {
	case 0x110: return "UPC-R81MD (Letter)";
		// UPC-R80MD (A4)
	case 0x200:
		if ((mdi & 0xfff) == 0x404) {
			return "UPP-110 Roll"; // UPP-110HD, UPP-110HG, UPP-110S
		} else if ((mdi & 0xfff) == 0x406) {
			return "UPP-210 Roll";  // UPP-210HD, UPP-210SE, UPT-210BL
		} else {
			return "Unknown thermal roll";
		}
		break;
	default: return "Unknown";
	}
}

static int updneo_attach(void *vctx, struct dyesub_connection *conn, uint8_t jobid)
{
	struct updneo_ctx *ctx = vctx;
	int ret;

	UNUSED(jobid);

	ctx->conn = conn;

	if (test_mode < TEST_MODE_NOATTACH) {
		if ((ret = updneo_get_status(ctx))) {
			return ret;
		}

		/* Needed by the UP-D898!  But should be safe for
		   all models */
		libusb_reset_device(ctx->conn->dev);
	} else {
		if (ctx->conn->type == P_SONY_UPD898) {
			strcpy(ctx->sts.scsyi, "100005001000050000000000014500");
		} else if (ctx->conn->type == P_SONY_UP9x1) {
			strcpy(ctx->sts.scsyi, "1E000A001E000A0000000000014500");
		} else if (ctx->conn->type == P_SONY_UPDR80) {
			strcpy(ctx->sts.scsyi, "0A300E5609A00C7809A00C78012D00");
		}
		// XXX don't forget cr20l here!
	}

	if (test_mode >= TEST_MODE_NOATTACH && getenv("MEDIA_CODE"))
		ctx->marker.numtype = atoi(getenv("MEDIA_CODE"));
	else
		ctx->marker.numtype = (ctx->sts.scmdi >> 12) & 0xfff;

	ctx->marker.name = updneo_medias(ctx->sts.scmdi);

	if (ctx->conn->type == P_SONY_UPD898 || ctx->conn->type == P_SONY_UP9x1) {
		ctx->marker.color = "#000000";  /* Ie black! */
		ctx->native_bpp = 1;
		ctx->marker.levelmax = CUPS_MARKER_UNAVAILABLE;
		ctx->marker.levelnow = CUPS_MARKER_UNKNOWN;
	} else {
		ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
		ctx->native_bpp = 3;
		ctx->marker.levelmax = 50;
		ctx->marker.numtype = (ctx->sts.scmdi >> 12) & 0xfff;
		ctx->marker.levelnow = ctx->sts.scmds[4];
	}

	return CUPS_BACKEND_OK;
}

static void updneo_cleanup_job(const void *vjob)
{
	const struct updneo_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);
	if (job->hdrbuf)
		free(job->hdrbuf);
	if (job->ftrbuf)
		free(job->ftrbuf);

	free((void*)job);
}

#define MAX_PRINTJOB_LEN (3400*2392*3 + 2048)

static int updneo_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct updneo_ctx *ctx = vctx;
	int run = 1;

	uint8_t tmpbuf[257];

	struct updneo_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Allocate job */
	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));

	/* Read in data chunks. */
	while(run) {
		uint8_t *ptr = NULL;
		int i, len, *lenptr;

		/* Read in data block header (256 bytes) */
		i = read(data_fd, tmpbuf, 256);
		if (i < 0) {
			ERROR("Read failed (%d)\n", i);
			updneo_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		if (i == 0)
			break;

		/* Explicitly null terminate just in case */
		tmpbuf[256] = 0;

		/* Parse header.  Format:

		   JOBSIZE=pdlname,blocklen,printsize,arg1,..,argN<NULL>

		*/

		if (strncmp("JOBSIZE=", (char*) tmpbuf, 8)) {
			updneo_cleanup_job(job);
			ERROR("Invalid spool format!\n");
			return CUPS_BACKEND_CANCEL;
		}

		/* PDL type */
		char *tok = strtok((char*)&tmpbuf[8], "\r\n,");
		if (!tok) {
			updneo_cleanup_job(job);
			ERROR("Invalid spool format (PDL)!\n");
			return CUPS_BACKEND_CANCEL;
		}

		/* Payload length */
		char *tokl = strtok(NULL, "\r\n,");
		if (!tokl) {
			updneo_cleanup_job(job);
			ERROR("Invalid spool format (block length missing)!\n");
			return CUPS_BACKEND_CANCEL;
		}
		len = atoi(tokl);
		if (len == 0 || len > MAX_PRINTJOB_LEN) {
			updneo_cleanup_job(job);
			ERROR("Invalid spool format (block length %d)!\n", len);
			return CUPS_BACKEND_CANCEL;
		}

		/* Behavior based on the various PDL blocks */
		if (!strncmp("PJL-H", tok, 5)) {
			job->hdrbuf = malloc(len);
			if (!job->hdrbuf) {
				ERROR("Memory allocation failure!\n");
				updneo_cleanup_job(job);
				return CUPS_BACKEND_RETRY_CURRENT;
			}
			ptr = job->hdrbuf;
			lenptr = &job->hdrlen;
		} else if (!strncmp("PJL-T", tok, 5)) {
			job->ftrbuf = malloc(len);
			if (!job->ftrbuf) {
				ERROR("Memory allocation failure!\n");
				updneo_cleanup_job(job);
				return CUPS_BACKEND_RETRY_CURRENT;
			}
			ptr = job->ftrbuf;
			lenptr = &job->ftrlen;
			run = 0;
		} else if (!strncmp("PDL", tok, 3)) {
			job->databuf = malloc(len);
			if (!job->databuf) {
				ERROR("Memory allocation failure!\n");
				updneo_cleanup_job(job);
				return CUPS_BACKEND_RETRY_CURRENT;
			}
			ptr = job->databuf;
			lenptr = &job->datalen;
		} else {
			ERROR("Unrecognized PDL type '%s'\n", tok);
			updneo_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}

//		DEBUG("Read block '%s' @ %d ...\n", tok, job->datalen);

//		DEBUG("...len '%d'\n", len);

		// parse the rest?
		// 898MD: 6,0,0,0
		// D80MD: 4
		// CR20L: 64,0,0,0

		/* Read in the data chunk */
		while(len > 0) {
			i = read(data_fd, ptr + *lenptr, len);
			if (i < 0) {
				updneo_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			if (i == 0)
				break;

			*lenptr += i;
			len -= i;
		}
	}

	if (!job->datalen || !job->hdrlen || !job->ftrlen) {
		if (job->datalen + job->hdrlen + job->ftrlen) {
			ERROR("Necessary block missing!\n");
		}
		updneo_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Sanity check job parameters */

	/* Validate against max print size in SCSYI */
	{
		char w[5], h[5];
		uint16_t mw, mh;
		uint16_t jw, jh;
		memcpy(w, ctx->sts.scsyi, 4);
		h[4] = 0;
		memcpy(h, ctx->sts.scsyi + 4, 4);
		w[4] = 0;

		if (ctx->conn->type == P_SONY_UPD898 || ctx->conn->type == P_SONY_UP9x1) {
			mw = strtol(h, NULL, 16);
			mh = strtol(w, NULL, 16);
		} else {
			mw = strtol(w, NULL, 16);
			mh = strtol(h, NULL, 16);
		}

		if (ctx->conn->type == P_SONY_UPDR80) {
			memcpy(&jw, job->databuf + 84, 2);
			memcpy(&jh, job->databuf + 84 + 2, 2);
		} else {
			memcpy(&jw, job->databuf + 40, 2);
			memcpy(&jh, job->databuf + 40 + 2, 2);
		}

		jw = be16_to_cpu(jw);
		jh = be16_to_cpu(jh);

		if (mw && mh && (jw > mw || jh > mh)) {
			ERROR("Job (%d/%d) exceeds max dimensions(%d/%d)\n",
			      jw,jh,mw,mh);
			updneo_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
	}

	// XXX Check vs loaded media type (ctx->marker.numtype?)

	// XXX set job copies to max(job, parameter)?
	/* Find copy offset */
	for (int i = 0 ; i < 312 ; i++ ) {
		if (job->databuf[i] == 0x02 &&
		    job->databuf[i+1] == 0x00 &&
		    job->databuf[i+2] == 0x09) {
			job->databuf[i+4] = copies;
			break;
		}
	}
	job->common.copies = 1;  /* Printer makes copies */

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int dlen;
static struct deviceid_dict dict[MAX_DICT];

static int updneo_get_status(struct updneo_ctx *ctx)
{
	char *ieee_id = get_device_id(ctx->conn->dev, ctx->conn->iface);
	int i;

	if (!ieee_id)
		return CUPS_BACKEND_FAILED;

	/* Don't forget to log! */
	if (dyesub_debug >= 1) {
		DEBUG("IEEE1284: %s\n", ieee_id);
	}

	dlen = parse1284_data(ieee_id, dict);

	/* Parse out data */
	for (i = 0; i < dlen ; i++) {
		if (!strcmp("SCDIV", dict[i].key)) {
			ctx->sts.scdiv = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCSYV", dict[i].key)) {
			ctx->sts.scsyv = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCSNO", dict[i].key)) {
			strncpy(ctx->sts.scsno, dict[i].val, sizeof(ctx->sts.scsno) - 1);

			/* Trim trailing '-'s off of serial number (UP-D898, UP-9x1)*/
			for (int i = 0; i < (int) sizeof(ctx->sts.scsno); i++) {
				if (ctx->sts.scsno[i] == '-') {
					ctx->sts.scsno[i] = 0;
					break;
				}
			}
		} else if (!strcmp("SCSYS", dict[i].key)) {
			strncpy(ctx->sts.scsys, dict[i].val, sizeof(ctx->sts.scsys) - 1);
		} else if (!strcmp("SCMDS", dict[i].key)) {
			int j;
			char buf[5];
			buf[4] = 0;
			for (j = 0 ; j < 5 ; j++) {
				memcpy(buf, dict[i].val + (4*j), 4);
				ctx->sts.scmds[j] = strtol(buf, NULL, 16);
			}
		} else if (!strcmp("SCPRS", dict[i].key)) {
			ctx->sts.scprs = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCSES", dict[i].key)) {
			ctx->sts.scses = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCWTS", dict[i].key)) {
			ctx->sts.scwts = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCJBS", dict[i].key)) {
			ctx->sts.scjbs = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCSYE", dict[i].key)) {
			ctx->sts.scsye = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCMDE", dict[i].key)) {
			ctx->sts.scmde = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCMCE", dict[i].key)) {
			ctx->sts.scmce = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCJBI", dict[i].key)) {
			strncpy(ctx->sts.scjbi, dict[i].val, sizeof(ctx->sts.scjbi) - 1);
		} else if (!strcmp("SCSYI", dict[i].key)) {
			strncpy(ctx->sts.scsyi, dict[i].val, sizeof(ctx->sts.scsyi) - 1);
		} else if (!strcmp("SCSVI", dict[i].key)) {
			int j;
			char buf[7];
			buf[6] = 0;
			for (j = 0 ; j < 2 ; j++) {
				memcpy(buf, dict[i].val + (6*j), 6);
				ctx->sts.scsvi[j] = strtol(buf, NULL, 16);
			}
		} else if (!strcmp("SCMNI", dict[i].key)) {
			int j;
			char buf[7];
			buf[6] = 0;
			for (j = 0 ; j < 2 ; j++) {
				memcpy(buf, dict[i].val + (6*j), 6);
				ctx->sts.scmni[j] = strtol(buf, NULL, 16);
			}
		} else if (!strcmp("SCCAI", dict[i].key)) {
			strncpy(ctx->sts.sccai, dict[i].val, sizeof(ctx->sts.sccai) - 1);
		} else if (!strcmp("SCGAI", dict[i].key)) {
			ctx->sts.scgai = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCGSI", dict[i].key)) {
			ctx->sts.scgsi = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCMDI", dict[i].key)) {
			ctx->sts.scmdi = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SCQTI", dict[i].key)) {
			ctx->sts.scqti = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("SPUQI", dict[i].key)) {
			ctx->sts.spuqi = strtol(dict[i].val, NULL, 16);
		} else if (!strcmp("MFG", dict[i].key) ||
			   !strcmp("MDL", dict[i].key) ||
			   !strcmp("DES", dict[i].key) ||
			   !strcmp("CMD", dict[i].key) ||
			   !strcmp("CLS", dict[i].key)) {
			/* Ignore standard IEEE1284 attributes! */
		} else {
			if (!strncmp("SC", dict[i].key, 2) || !strncmp("SP", dict[i].key, 2))
				DEBUG("Extra/Unknown IEEE1284 field '%s' = '%s'\n",
				      dict[i].key, dict[i].val);
		}

	};

	/* Clean up */
	if (ieee_id) free(ieee_id);
	while (dlen--) {
		free (dict[dlen].key);
		free (dict[dlen].val);
	}

	return CUPS_BACKEND_OK;
}

static void updneo_dump_status(struct updneo_ctx *ctx, struct updneo_sts *sts)
{
	/* Dump status */
	INFO("Serial Number: %s\n", sts->scsno);
	INFO("Firmware Version: %02x.%02x.%02x.%02x\n",
	     (sts->scsyv >> 24) & 0xff,
	     (sts->scsyv >> 16) & 0xff,
	     (sts->scsyv >> 8) & 0xff,
	     (sts->scsyv >> 0) & 0xff);
	INFO("Media type: %s\n", updneo_medias(sts->scmdi));

	if (ctx->conn->type == P_SONY_UPDR80)
		INFO("Remaining prints: %u/50\n", sts->scmds[4]);

	INFO("Print count: %u\n", sts->scsvi[0]);

	/* If the printer reports an error, pass it on */
	if (sts->scmde || sts->scmce || sts->scsye) {
		ERROR("Printer error: %s (MD=%04x, MC=%02x, SY=%02x)\n",
		      updneo_decode_errors(sts->scmde, sts->scmce, sts->scsye),
		      sts->scmde, sts->scmce, sts->scsye);
	}
}

static int updneo_main_loop(void *vctx, const void *vjob, int wait_for_return) {
	struct updneo_ctx *ctx = vctx;
	int ret;
	int copies;

	const struct updneo_printjob *job = vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	copies = job->common.copies;

top:

	/* Query printer status */
	if ((ret = updneo_get_status(ctx))) {
		return ret;
	}

	/* If the printer reports an error, bail */
	if (ctx->sts.scmde || ctx->sts.scmce || ctx->sts.scsye) {
		ERROR("Printer error: %s (MD=%04x, MC=%02x, SY=%02x)\n",
		      updneo_decode_errors(ctx->sts.scmde, ctx->sts.scmce, ctx->sts.scsye),
		      ctx->sts.scmde, ctx->sts.scmce, ctx->sts.scsye);
		return CUPS_BACKEND_STOP;
	}
	/* Wait for the printer to become idle */
	if (ctx->sts.scprs) {
		sleep(1);
		goto top;
	}

	/* Send over header */
	if ((ret = send_data(ctx->conn,
			     job->hdrbuf, job->hdrlen)))
		return CUPS_BACKEND_FAILED;

	/* Send over data */
	if ((ret = send_data(ctx->conn,
			     job->databuf, job->datalen)))
		return CUPS_BACKEND_FAILED;

	/* Send over footer */
	if ((ret = send_data(ctx->conn,
			     job->ftrbuf, job->ftrlen)))
		return CUPS_BACKEND_FAILED;

	/* Wait for completion! */
retry:
	sleep(1);

	if ((ret = updneo_get_status(ctx))) {
		return ret;
	}

	/* If the printer reports an error, bail */
	if (ctx->sts.scmde || ctx->sts.scmce || ctx->sts.scsye) {
		ERROR("Printer error: %s (MD=%04x, MC=%02x, SY=%02x)\n",
		      updneo_decode_errors(ctx->sts.scmde, ctx->sts.scmce, ctx->sts.scsye),
		      ctx->sts.scmde, ctx->sts.scmce, ctx->sts.scsye);
		return CUPS_BACKEND_STOP;
	}

	/* See if we're busy... */
	if (ctx->sts.scprs != 0) {
		if (!wait_for_return) {
			INFO("Fast return mode enabled.\n");
		} else {
			goto retry;
		}
	}

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		goto top;
	}

	/* Needed by the UP-D898!  But should be safe for
	   all models */
	libusb_reset_device(ctx->conn->dev);

	return CUPS_BACKEND_OK;
}

static void updneo_cmdline(void)
{
	DEBUG("\t\t[ -s ]           # Query status\n");
}

static int updneo_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct updneo_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "s")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 's':
			j = updneo_get_status(ctx);
			if (!j)
				updneo_dump_status(ctx, &ctx->sts);
			break;
		}

		if (j) return j;
	}

	return CUPS_BACKEND_OK;
}

static int updneo_query_serno(struct dyesub_connection *conn, char *buf, int buf_len)
{
	int ret;
	char *ptr;
	struct updneo_ctx ctx = {
		.conn = conn,
	};
	if ((ret = updneo_get_status(&ctx))) {
		return ret;
	}
	ptr = ctx.sts.scsno;
	while (*ptr == 0x30) ptr++;
	strncpy(buf, ptr, buf_len);
	buf[buf_len-1] = 0;
	return CUPS_BACKEND_OK;
}

static int updneo_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct updneo_ctx *ctx = vctx;
	int ret;

	*markers = &ctx->marker;
	*count = 1;

	/* Query printer status */
	if ((ret = updneo_get_status(ctx))) {
		return ret;
	}

	if (ctx->conn->type != P_SONY_UPD898 && ctx->conn->type != P_SONY_UP9x1) {
		ctx->marker.levelnow = ctx->sts.scmds[4];
	}

	return CUPS_BACKEND_OK;
}

static const char *sonyupdneo_prefixes[] = {
	"sonyupdneo", /* Family Name */
	"dnp-sl20", // extra, unknown if shared with CR20L
	NULL
};

const struct dyesub_backend sonyupdneo_backend = {
	.name = "Sony UP-D Neo",
	.version = "0.18",
	.flags = BACKEND_FLAG_BADISERIAL, /* UP-D898MD at least */
	.uri_prefixes = sonyupdneo_prefixes,
	.cmdline_arg = updneo_cmdline_arg,
	.cmdline_usage = updneo_cmdline,
	.init = updneo_init,
	.attach = updneo_attach,
	.cleanup_job = updneo_cleanup_job,
	.read_parse = updneo_read_parse,
	.main_loop = updneo_main_loop,
	.query_markers = updneo_query_markers,
	.query_serno = updneo_query_serno,
	.devices = {
		{ 0x054c, 0x0877, P_SONY_UPD898, NULL, "sony-upd898"},
//		{ 0x054c, 0x589a, P_SONY_UPD898, NULL, "sony-upd898"}, // ???
		{ 0x054c, 0xbcde, P_SONY_UPCR20L, NULL, "sony-upcr20l"}, // XXXX
		{ 0x054c, 0x03c5, P_SONY_UPDR80, NULL, "sony-updr80"},
		{ 0x054c, 0x03c3, P_SONY_UPDR80, NULL, "sony-updr80md"},
		{ 0x054c, 0x03c4, P_SONY_UPDR80, NULL, "stryker-sdp1000"},
		{ 0x054c, 0x0873, P_SONY_UP9x1, NULL, "sony-up971ad"},
//		{ 0x054c, 0x0873, P_SONY_UP9x1, NULL, "sony-up991ad"},	// Dupe ?
		{ 0, 0, 0, NULL, NULL}
	}
};

/* Sony UP-D (new) printer spool format

   Covers UP-CR20L, UP-DR80/DR80MD, UP-D898/UP-X898

  HP-PJL wrapper around custom Sony PDL:

    JOBSIZE=PJL-H,size,arg1,arg2,etc   [null terminated, padded to 256 bytes]
    [ size bytes of PJL header! ]
    JOBSIZE=PDL,size,args [null terminated, padded to 256 bytes]
    [ size bytes of PDL data! ]
    JOBSIZE=PJL-T,size,args [null terminated, padded to 256 bytes]
    [ size bytes of PJL trailer! ]

  PJL header:

   <ESC>%-12345X<CR><LF>
   @PJL COMMENT free form text here <CR><LF>
   @PJL JOB NAME="name me" ID="someid"<CR><LF>
   @PJL .... <CR><LF>
   @PJL ENTER LANGUAGE=SONY-PDL-DS2<CR><LF>

  PJL footer:

   @PJL EOJ<CR><LF>
   <ESC>%-12345X<CR><LF>

  PDL notes:

   size is the length mentioned in the payload (ie rows * cols * planes)
   plus the PDL header (varies) and PDL footer (7 bytes)

Note:  All multi-byte values are BIG ENDIAN

 UP-D898MD:  18*16+2 == 290 byte header

  00000250                                             00 00   YY YY = rows
  00000260  01 00 00 10 0f 00 1c 00  00 00 00 00 00 00 00 00   XX XX = columns (fixed at 05 00)
  00000270  00 00 00 00 00 01 02 00  09 00 NN 01 00 11 01 08   NN = Copies (01..?)  <-- GUESS
  00000280  00 1a 00 00 00 00 XX XX  YY YY 09 00 28 01 00 d4
  00000290  00 00 03 58 YY YY 00 00  13 01 00 04 00 80 00 23
  000002a0  00 0c 01 09 XX XX YY YY  00 00 00 00 08 ff 08 00
  000002b0  19 00 00 00 00 XX XX YY  YY 00 00 81 80 00 8f 00
  000002c0  b8 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  000002d0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  000002f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000300  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000310  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000320  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000330  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000340  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000350  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000360  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000370  00 00 00 00 00 00 00 00  00 c0 00 82 LL LL LL LL   LL == payload bytes, BE (== XX * YY * 1)

   [payload of LL bytes follows]

 UP-971/991AD  18*16+2 == 290 byte header

  00000250                                             00 00  XX XX = columns (fixed at 0a 00)
  00000260  01 00 00 10 0f 00 1c 00  00 00 00 00 00 00 00 00  YY YY = rows (varies)
  00000270  00 00 SS TT TT GG 02 00  09 00 NN 01 00 11 01 08  SS == sharpening (00-0e)
  00000280  00 1a 00 00 00 00 XX XX  YY YY 09 00 28 01 01 26  GG == gamma (00/01/02 == 1/2/3)
  00000290  00 00 07 b3 YY YY 00 00  13 01 00 04 00 80 00 23  TT TT == tone, +-32 (signed, BE)
  000002a0  00 0c 01 09 XX XX YY YY  00 00 00 00 08 ff 08 00  NN == copies (00 for printer, 01-???)
  000002b0  19 00 00 00 00 XX XX YY  YY 00 00 81 80 00 8f 00
  000002c0  b8 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  000002d0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  000002e0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  000002f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000300  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000310  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000320  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000330  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000340  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000350  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000360  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000370  00 00 00 00 00 00 00 00  00 c0 00 82 LL LL LL LL   LL == payload bytes, BE (== XX * YY * 1)

 UP-CR20L:   290 byte header, 330 dpi, 1210x1728/1382x2048/1728*2380/2724*2048 (L/PC/2L/2PC)

  00000250                                             00 00
  00000260  01 00 00 10 0f 00 1c 00  00 00 00 00 00 00 00 00
  00000270  00 00 01 00 00 00 02 00  16 00 00 02 00 09 00 NN   NN = Copies (01..??)
  00000280  02 00 06 01 01 03 00 1d  00 00 00 01 00 20 01 01
  00000290  00 27 40 01 00 11 01 08  00 1a 00 00 00 00 RR RR
  000002a0  CC CC 00 00 13 01 00 04  00 80 00 23 00 10 03 00   CC CC == Columns  (BE)
  000002b0  RR RR CC CC 00 00 00 00  08 08 08 ff ff ff 01 00   RR RR == Rows     (BE)
  000002c0  17 00 08 00 19 00 00 00  00 RR RR CC CC 00 00 81
  000002d0  80 00 8f 00 a4 00 00 00  00 00 00 00 00 00 00 00
  000002e0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  000002f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000300  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000310  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000320  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000330  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000340  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000350  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000360  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000370  00 00 00 00 00 00 00 00  00 c0 00 82 LL LL LL LL   LL == payload bytes, BE (== RR * CC * 3)

   [payload of LL bytes follows]

 UP-DR80MD:   (19*16+8) = 312 byte header  (A4 = 3400x2392, Letter = 3192*2464)

  00000240                           00 00 01 00 00 10 0f 00
  00000250  1c 00 00 00 00 00 00 00  00 00 00 00 00 00 00 ZZ   ZZ = 0x00 (Letter) 0x56 (A4)
  00000260  02 00 16 00 01 80 00 15  00 12 55 50 44 52 38 30   SS = 0x00 (LUT0) 0xff (No LUT)
  00000270  00 00 4c 55 54 QQ 00 00  00 00 00 SS 02 00 09 00   QQ = 0x30 (LUT0) 0x2f (No LUT)
  00000280  NN 02 00 06 01 03 04 00  1d 01 00 00 05 01 00 20   NN = Copies (01...??)
  00000290  00 01 00 11 01 08 00 1a  00 00 00 00 CC CC RR RR   RR RR = Rows (BE)
  000002a0  00 00 13 01 00 04 00 80  00 23 00 10 03 00 CC CC   CC CC = Cols (BE)
  000002b0  RR RR 00 00 00 00 08 08  08 ff ff ff 01 00 17 00
  000002c0  08 00 19 00 00 00 00 CC  CC RR RR 00 00 81 80 00
  000002d0  8f 00 a6 00 00 00 00 00  00 00 00 00 00 00 00 00
  000002e0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  000002f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000300  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000310  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000320  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000330  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000340  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000350  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000360  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
  00000370  00 00 00 00 00 00 00 00  00 c0 00 82 LL LL LL LL  LL == payload, BE (== RR * CC * 3)

   [payload of LL bytes follows]

 *********

  898 Format:

  00  00 01
  00  00 10
  0f  00 1c  00 00 00 00  00 00 00 00  00 00 00 00  00 00 01
  02  00 09  00 NN
  01  00 11  01
  08  00 1a  00 00 00 00  XX XX YY YY
  09  00 28  01 00 d4 00  00 03 58 YY  YY
  00  00 13
  01  00 04  00
  80  00 23  00 0c 01 09  XX XX YY YY  00 00 00 00  08 ff
  08  00 19  00 00 00 00  XX XX YY YY
  00  00 81
  80  00 8f  ## ##  [ follwed by ## zeros ]  [ b8 on 898 ]
  c0  00 82  LL LL LL LL

  DR80MD format:

  00  00 01
  00  00 10
  0f  00 1c  00 00 00 00  00 00 00 00  00 00 00 00  00 00 ZZ
  02  00 16  00 01
  80  00 15  00 12 55 50  44 52 38 30  00 00 4c 55  54 QQ 00 00
             00 00 00 SS
  02  00 09  00 NN
  02  00 06  01 03
  04  00 1d  01 00 00 05
  01  00 20  00
  01  00 11  01
  08  00 1a  00 00 00 00  CC CC RR RR
  00  00 13
  01  00 04  00
  80  00 23  00 10 03 00  CC CC RR RR  00 00 00 00  08 08 08 ff
             ff ff 01 00  17 00
  08  00 19  00 00 00 00  CC CC RR RR
  00  00 81
  80  00 8f  ## ## [ followed by ## zeros ] [ a6 on dr80md ]
  c0  00 82  LL LL LL LL

  CR20L format:

  00  00 01
  00  00 10
  0f  00 1c  00 00 00 00  00 00 00 00  00 00 00 01  00 00 00
  02  00 16  00 00
  02  00 09  00 NN
  02  00 06  01 01
  03  00 1d  00 00 00
  03  00 13  00 01 02   [ only when multicut is on ]
  01  00 20  01
  01  00 27  40
  01  00 11  01
  08  00 1a  00 00 00 00  RR RR CC CC
  00  00 13
  01  00 04  00
  80  00 23  00 10 03 00  RR RR CC CC  00 00 00 00  08 08 08 ff
             ff ff 01 00  17 00
  08  00 19  00 00 00 00  RR RR CC CC
  00  00 81
  80  00 8f  ## ## [ followed by ## zeros ]   [ a4/9d if mcut is off/on ]
  c0  00 82  LL LL LL LL

  ***  COMBINED / COMMON :

  XX ZZ ZZ [ XX = length, ZZ = code, followed by XX bytes ]

  00  00 01
  00  00 10
  0f  00 1c  00 00 00 00  00 00 00 00  00 00 00 00  00 00 ZZ  [ ZZ = pagecode; 01 for 898, 00 for CR20L, 00/56 for Letter/A4 on DR80MD ]
  02  00 09  00 NN  [ NN = copies ]
  01  00 11  01
  08  00 1a  00 00 00 00  CC CC RR RR   [ CC = cols, RR = rows ]
  00  00 13
  01  00 04  00
  [ more/unique stuff in here ]
  [ always ends with these ]
  08  00 19  00 00 00 00  RR RR CC CC
  00  00 81
  80  00 8f  ## ##  [ follwed by ## zeros, pad to fill out header ]
  c0  00 82  LL LL LL LL

  *** 898 unique [ 290 byte header ]

  09  00 28  01 00 d4 00  00 03 58 CC  CC
  80  00 23  00 0c  01 09 RR RR  CC CC 00 00  00 00 08 ff
             ^^ ^^
             length of data to follow

  ** CR20L unique [ 290 byte header ]

  03  00 1d  00 00 00
  03  00 13  00 01 02   [ only when multicut is on ]
  01  00 27  40

  02  00 16  00 00
  02  00 06  01 01
  01  00 20  01

  80  00 23  00 10  03 00 RR RR  CC CC 00 00  00 00 08 08  08 ff ff ff
  01  00 17  00

  ** DR80MD unique [ 312 byte header ]

  80  00 15  00 12 55 50  44 52 38 30  00 00 4c 55  54 QQ 00 00
             00 00 00 SS  [ SS/QQ = LUT (See above) ]
  04  00 1d  01 00 00 05

  02  00 16  00 01
  02  00 06  01 03
  01  00 20  00

  80  00 23  00 10  03 00 RR RR  CC CC 00 00  00 00 08 08  08 ff ff ff
  01  00 17  00

 *****************

  PRINTER COMMS:

   * Strip out "JOBSIZE=" headers
   * Send PJL header
   * Send PDL payload  (every 9*256KB, do a status query..)
   * Send PJL footer

  PJL header and footer need to be sent separately.
  the PJL wrapper around the PDL block needs to be
  stripped.

  ***********

  It appears that the printer status is tacked onto the IEEE1284 string:  Examples:

  IDLE
    MFG:SONY;MDL:UP-DR80MD;DES:Sony UP-DR80MD;CMD:SPJL-DS,SPDL-DS;CLS:PRINTER;SCDIV:0100;SCSYV:01060000;SCSNO:0000000000089864;SCSYS:0000001000010001000100;SCMDS:00000000002C002C002C;SCPRS:0000;SCSES:0000;SCWTS:0000;SCJBS:0000;SCSYE:00;SCMDE:0000;SCMCE:00;SCJBI:0000000000000000;SCSYI:0A300E5609A00C7809A00C78012D00;SCSVI:000342000342;SCMNI:000342000342;SCCAI:00000000000000;SCGAI:0000;SCGSI:00;SCMDI:110154

  DATA XFER:
    MFG:SONY;MDL:UP-DR80MD;DES:Sony UP-DR80MD;CMD:SPJL-DS,SPDL-DS;CLS:PRINTER;SCDIV:0100;SCSYV:01060000;SCSNO:0000000000089864;SCSYS:0000011000010001000000;SCMDS:00000000002C002C002C;SCPRS:0005;SCSES:0000;SCWTS:0000;SCJBS:0000;SCSYE:00;SCMDE:0000;SCMCE:00;SCJBI:0000000000000000;SCSYI:0A300E5609A00C7809A00C78012D00;SCSVI:000342000342;SCMNI:000342000342;SCCAI:00000000000000;SCGAI:0000;SCGSI:00;SCMDI:110154

  NO PAPER:
    MFG:SONY;MDL:UP-DR80MD;DES:Sony UP-DR80MD;CMD:SPJL-DS,SPDL-DS;CLS:PRINTER;SCDIV:0100;SCSYV:01060000;SCSNO:0000000000089864;SCSYS:0000003800010000000100;SCMDS:00000000000000000000;SCPRS:0000;SCSES:0000;SCWTS:0000;SCJBS:0000;SCSYE:00;SCMDE:0A00;SCMCE:00;SCJBI:0000000000000000;SCSYI:0A300E560000000000000000012D00;SCSVI:000345000345;SCMNI:000345000345;SCCAI:00000000000000;SCGAI:0000;SCGSI:00;SCMDI:1100FF

  NO RIBBON:
    MFG:SONY;MDL:UP-DR80MD;DES:Sony UP-DR80MD;CMD:SPJL-DS,SPDL-DS;CLS:PRINTER;SCDIV:0100;SCSYV:01060000;SCSNO:0000000000089864;SCSYS:0000003800010000000100;SCMDS:00000000000000000000;SCPRS:0000;SCSES:0000;SCWTS:0000;SCJBS:0000;SCSYE:00;SCMDE:0002;SCMCE:00;SCJBI:0000000000000000;SCSYI:0A300E5609A00C7809A00C78012D00;SCSVI:000345000345;SCMNI:000345000345;SCCAI:00000000000000;SCGAI:0000;SCGSI:00;SCMDI:000154

  COVER OPEN:
    MFG:SONY;MDL:UP-DR80MD;DES:Sony UP-DR80MD;CMD:SPJL-DS,SPDL-DS;CLS:PRINTER;SCDIV:0100;SCSYV:01060000;SCSNO:0000000000089864;SCSYS:0000001800010000000100;SCMDS:00000000000000000000;SCPRS:0000;SCSES:0000;SCWTS:0000;SCJBS:0000;SCSYE:00;SCMDE:0800;SCMCE:01;SCJBI:0000000000000000;SCSYI:0A300E560000000000000000012D00;SCSVI:000345000345;SCMNI:000345000345;SCCAI:00000000000000;SCGAI:0000;SCGSI:00;SCMDI:1100FF

  Stryker SDP1000

    MFG:Stryker;MDL:SDP1000;DES:Stryker SDP1000;CMD:SPJL-DS,SPDL-DS2;CLS:PRINTER;SCDIV:0100;SCSYV:01010000;SCSNO:0000000000083389;SCSYS:0000001000010000000100;SCMDS:00000000002D002D002D;SCPRS:0000;SCSES:0000;SCWTS:0000;SCJBS:0000;SCSYE:00;SCMDE:0000;SCMCE:00;SCJBI:0000000000000000;SCSYI:0A300E5609A00C7809A00C78012D00;SCSVI:000047000047;SCMNI:000047000047;SCCAI:00000000000000;SCGAI:0000;SCGSI:00;SCMDI:110154;

  [ Notable difference is SCSYI ]

  UP-DR898MD

    MFG:Sony;MDL:UP-D898MD_X898MD;DES:Sony UP-D898MD_X898MD;CMD:SPJL-DS,SPDL-DS2;CLS:PRINTER;SCDIV:0100;SCSYV:01010000;SCSYS:0000001000010000000000;SCMDS:00000500000100000000;SCSYE:00;SCMDE:0000;SCMCE:00;SCSYI:100005001000050000000000014500;SCSVI:000204000204;SCMDI:200406;SCSNO:7100886---------;SCJBS:0000;SCCAI:00000000000000;SCGSI:01;SCQTI:0001;SPUQI:0000

  [ This adds SCQTI; SCSNO is formatted differently, no SCPRS/SCJBI ]

  UP-971AD / UP-991AD

    MFG:Sony;MDL:UP-991AD_971AD;DES:Sony UP-991AD_971AD;CMD:SPJL-DS,SPDL-DS2;CLS:PRINTER;SCDIV:0100;SCSYV:01050000;SCSYS:0000001000010000000000;SCMDS:00000500000100000000;SCSYE:00;SCMDE:0000;SCMCE:00;SCSYI:1E000A001E000A0000000000014500;SCSVI:000002000002;SCMDI:200404;SCSNO:0739166---------;SCJBS:0000;SCCAI:00000000000000;SCGSI:00;SCQTI:0001;SPUQI:0000

  [ Appears to be largely similar to 898 series ]

Breakdown:

  (+) means referenced by their Windows driver

  SCDIV  # Data Info Version (?) Always seems to be 0100
 +SCSYV  # SYstemVersion (?) (01.06.00.00) ??
  SCSNO  # SerialNO
 +SCSYS  # SystemStatus (?) some sort of state array? 22 fields.  b19 is 1 when data can be sent?, b5 is 1 when printer busy?, b20:21 is 64 sometimes, maybe paper or ribbon feed.  b6:7 is 38 with no paper&|ribbon, or 18 with cover open
 +SCMDS  # MeDiaStatus: five 4-value hex numbers, last three decrease in unison (remaining prints). second one is 0000/0100/0200/0300/0600, maybe Y/M/C/O?
  SCPRS  # PRinterStatus: (0000 = idle, 0002 = printing, 0005 = data xfer?)
 +SCSES  # "SE" Status
 +SCWTS  # "WT" Status
 +SCJBS  # JoBStatus (?)
  SCSYE  # SYstemError (?)
 +SCMDE  # MeDiaError: 2000 media mismatch, 0A00 no paper, 0800 cover open, 0002 no ribbon
 +SCMCE  # MediaCoverError: 01 cover open
  SCJBI  # JoBInfo (?)
  SCSYI  # SYstemInfo (?) Includes legal job max dimensions/parameters; up to three dimension sets and a fourth unknown field
 +SCSVI  # print counter(s)?  (XXXXXXYYYYYY, and X = Y so far.  SCSVI and SCMNI are identical so far)
  SCMNI  # print counter(s)?  (see SCSVI)
  SCCAI  # "CA" Info (?)
  SCGAI  # "GA" Info (?)
  SCGSI  # "GS" Info (?)
 +SCMDI  # MeDiaInfo: 110154 OK w/UPD-R81MD(Letter), 1100FF with no paper, 000154 with no ribbon, 200404 on A4 thermal printers, 2000406 on A6 thermal printers?
  SCQTI  # "QT" Info (?) (898MD, UP9x1)
  SPUQI  # "UQ" Info (?) (898MD, UP9x1)

Guess:

  SCxxy  SC = Sony Corp
         xx = class/field (eg MD = media)
          y = var type (S = status, I = info, E = error V = version, O

 */
