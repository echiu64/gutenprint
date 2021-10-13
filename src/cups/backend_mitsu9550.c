/*
 *   Mitsubishi CP-9xxx Photo Printer Family CUPS backend
 *
 *   (c) 2014-2021 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND mitsu9550_backend

#include "backend_common.h"
#include "backend_mitsu.h"

#define MITSU_M98xx_LAMINATE_FILE  "M98MATTE.raw"
#define MITSU_M98xx_DATATABLE_FILE "M98TABLE.dat"
#define MITSU_M98xx_LUT_FILE       "M98XXL01.lut"
#define MITSU_CP30D_LUT_FILE       "CP30LT_1.lut"
#define LAMINATE_STRIDE 1868

/* Spool file structures */

/* Print parameters1 */
struct mitsu9550_hdr1 {
	uint8_t  cmd[4];  /* 1b 57 20 2e */
	uint8_t  unk[10]; /* 00 0a 10 00 [...] */
	uint16_t cols;    /* BE */
	uint16_t rows;    /* BE */
	uint8_t  matte;   /* CP9810/9820 only. 01 for matte, 00 glossy */
	uint8_t  null[31];
} __attribute__((packed));

/* Print parameters2 */
struct mitsu9550_hdr2 {
	uint8_t  cmd[4];   /* 1b 57 21 2e */
	uint8_t  unk[24];  /* 00 80 00 22 08 03 [...] */
	uint16_t copies;   /* BE, 1-680 */
	uint8_t  null[2];
	uint8_t  cut;      /* 00 == normal, 83 == 2x6*2 */
	uint8_t  unkb[5];
	uint8_t  mode;     /* 00 == fine, 80 == superfine */
	uint8_t  unkc[11]; /* 00 [...] 00 01 ; note [7][8][9] are cp98xx extensions for sharpness/reversed/lut */
} __attribute__((packed));

/* Fine Deep selection (9550 only) */
struct mitsu9550_hdr3 {
	uint8_t  cmd[4];   /* 1b 57 22 2e */
	uint8_t  unk[7];   /* 00 40 00 [...] */
	uint8_t  mode2;    /* 00 == normal, 01 == finedeep */
	uint8_t  null[38];
} __attribute__((packed));

/* Error policy? */
struct mitsu9550_hdr4 {
	uint8_t  cmd[4];   /* 1b 57 26 2e */
	uint8_t  unk[46];  /* 00 70 00 00 00 00 00 00 01 01 00 [...] */
} __attribute__((packed));

/* Data plane header */
struct mitsu9550_plane {
	uint8_t  cmd[4];     /* 1b 5a 54 XX */  /* XX == 0x10 if 16bpp, 0x00 for 8bpp */
	uint16_t col_offset; /* BE, normally 0, where we start dumping data */
	uint16_t row_offset; /* BE, normally 0, where we start dumping data */
	uint16_t cols;       /* BE */
	uint16_t rows;       /* BE */
} __attribute__((packed));

/* Command header */
struct mitsu9550_cmd {
	uint8_t cmd[4];
} __attribute__((packed));

/* Private data structure */
struct mitsu9550_printjob {
	struct dyesub_job_common common;

	uint8_t *databuf;
	uint32_t datalen;

	uint16_t rows;
	uint16_t cols;
	uint32_t plane_len;
	int is_raw;

	/* Parse headers separately */
	struct mitsu9550_hdr1 hdr1;
	int hdr1_present;
	struct mitsu9550_hdr2 hdr2;
	int hdr2_present;
	struct mitsu9550_hdr3 hdr3;
	int hdr3_present;
	struct mitsu9550_hdr4 hdr4;
	int hdr4_present;
};

struct mitsu9550_ctx {
	struct dyesub_connection *conn;

	int is_s;
	int is_98xx;
	int need_lib;
	int footer_len;
	const char *lut_fname;

	char fwver[7];  /* 6 + null */

	struct marker marker;

	/* CP98xx stuff */
	struct mitsu_lib lib;
	const struct mitsu98xx_data *m98xxdata;
};

/* Printer data structures */
#define CP9XXX_STS_x20     0x20
#define CP9XXX_STS_x21     0x21  /* struct mitsu9550_status2 */
#define CP9XXX_STS_x22     0x22
#define CP9XXX_STS_FWVER   0x23
#define CP9XXX_STS_MEDIA   0x24  /* struct mitsu9550_media */
#define CP9XXX_STS_x26     0x26
#define CP9XXX_STS_x30     0x30  /* struct mitsu9550_status */
#define CP9XXX_STS_x32     0x32  /* struct mitsucp30_status */

struct mitsu9550_media {
	uint8_t  hdr[2];  /* 24 2e */
	uint8_t  unk[12];
	uint8_t  type;
	uint8_t  unka[11];
	uint16_t unkb;    /* 0d 00 (CP9810), 00 00 (others?) */
	uint16_t max;     /* BE, prints per media */
	uint16_t remain2; /* BE, prints remaining  (CP30)*/
	uint16_t remain;  /* BE, prints remaining (Everything else) */
	uint8_t  unkc[14];
} __attribute__((packed));

struct mitsu9550_status {
	uint8_t  hdr[2]; /* 30 2e */
	uint8_t  null[4];
	uint8_t  sts1; // MM
	uint8_t  nullb[1];
	uint16_t copies; // BE, NN
	uint8_t  sts2; // ZZ  (9600 only?)
	uint8_t  nullc[5];
	uint8_t  sts3; // QQ
	uint8_t  sts4; // RR
	uint8_t  sts5; // SS
	uint8_t  nulld[25];
	uint8_t  sts6; // TT
	uint8_t  sts7; // UU
	uint8_t  nulle[2];
} __attribute__((packed));

struct mitsucp30_status {
	uint8_t  hdr[2]; /* 32 2e */
	uint8_t  zero[2];
	uint16_t serno[13]; /* UTF16, BE */
	uint8_t  sts;     /* CP30_STS_* */
	uint8_t  sts2;
	uint8_t  zerob;
	uint16_t err;     /* CP30_ERR_* */
	uint8_t  zeroc[2];
	uint8_t  remain;  /* on media */
	uint8_t  zerod[9];
} __attribute__((packed));

#define CP30_STS_IDLE     0x00
#define CP30_STS_PRINT    0x20

#define CP30_STS_PRINT_A  0x10  //Load
#define CP30_STS_PRINT_B  0x20  //Y
#define CP30_STS_PRINT_C  0x30  //M
#define CP30_STS_PRINT_D  0x40  //C
#define CP30_STS_PRINT_E  0x60  //Eject?

#define CP30_ERR_OK       0x0000
#define CP30_ERR_NOPC     0x0303
#define CP30_ERR_NORIBBON 0x0101
#define CP30_ERR_TRAYFULL 0x1404

struct mitsu9550_status2 {
	uint8_t  hdr[2]; /* 21 2e */
	uint8_t  unk[40];
	uint16_t remain; /* BE, media remaining */
	uint8_t  unkb[4]; /* 0a 00 00 01 */
} __attribute__((packed));

static int mitsu9550_main_loop(void *vctx, const void *vjob, int wait_for_return);

static const char *cp30_errors(uint16_t err)
{
	switch(err){
	case CP30_ERR_OK: return "None";
	case CP30_ERR_NOPC: return "No Paper Cassette";
	case CP30_ERR_NORIBBON: return "No Ribbon Loaded";
	case CP30_ERR_TRAYFULL: return "Output Tray Full";
	default: return "Unknown";
	}
}

static const char *cp30_media_types(uint16_t remain)
{
	switch(remain) {
	case 80: return "CK30S";
	case 50: return "CK30L";
	default: return "Unknown";
	}
}

#define CMDBUF_LEN   64
#define READBACK_LEN 128

#define QUERY_STATUS_I							\
	struct mitsu9550_status *sts = (struct mitsu9550_status*) rdbuf; \
	struct mitsucp30_status *sts30 = (struct mitsucp30_status*) rdbuf; \
	struct mitsu9550_media *media = (struct mitsu9550_media *) rdbuf; \
	uint16_t donor;							\
	/* media */							\
	ret = mitsu9550_get_status(ctx, rdbuf, CP9XXX_STS_MEDIA);	\
	if (ret < 0)							\
		return CUPS_BACKEND_FAILED;				\
									\
	if (ctx->conn->type == P_MITSU_CP30D) {				\
		donor = be16_to_cpu(media->remain2);			\
	} else {							\
		donor = be16_to_cpu(media->remain);			\
	}								\
	if (donor != ctx->marker.levelnow) {				\
		ctx->marker.levelnow = donor;				\
		dump_markers(&ctx->marker, 1, 0);			\
	}								\
	/* Sanity-check media response */				\
	if ((media->remain == 0 && media->remain2 == 0) || media->max == 0) { \
		ERROR("Printer out of media!\n");			\
		return CUPS_BACKEND_STOP;				\
	}								\

#define QUERY_STATUS_II				\
	if (ctx->conn->type != P_MITSU_CP30D) {				\
		/* struct mitsu9550_status2 *sts2 = (struct mitsu9550_status2*) rdbuf; */ \
		ret = mitsu9550_get_status(ctx, rdbuf, CP9XXX_STS_x21);	\
		if (ret < 0)						\
			return CUPS_BACKEND_FAILED;			\
		/* XXX validate status2 ? */				\
	}

#define QUERY_STATUS_III			\
		/* Check for known errors */				\
		if (sts->sts2 != 0) {					\
			ERROR("Printer cover open!\n");			\
			return CUPS_BACKEND_STOP;			\
		}

#define QUERY_STATUS_IIIB						\
		/* Check for known errors */				\
		if (sts30->err != CP30_ERR_OK) {			\
			ERROR("%s (%04x)!\n", cp30_errors(be16_to_cpu(sts30->err)), be16_to_cpu(sts30->err)); \
			return CUPS_BACKEND_STOP;			\
		}

#define QUERY_STATUS_IV				\
	if (ctx->conn->type == P_MITSU_CP30D) {				\
		ret = mitsu9550_get_status(ctx, rdbuf, CP9XXX_STS_x32); \
		if (ret < 0)						\
			return CUPS_BACKEND_FAILED;			\
									\
		if (sts30->sts != CP30_STS_IDLE) {			\
			sleep(1);					\
			goto top;					\
		}							\
		QUERY_STATUS_IIIB;					\
	} else {							\
		ret = mitsu9550_get_status(ctx, rdbuf, CP9XXX_STS_x30); \
		if (ret < 0)						\
			return CUPS_BACKEND_FAILED;			\
									\
		/* Make sure we're idle */				\
		if (sts->sts5 != 0) {					\
			sleep(1);					\
			goto top;					\
		}							\
		QUERY_STATUS_III;					\
	}								\

static int mitsu98xx_fillmatte(struct mitsu9550_printjob *job)
{
	int ret;

	/* Fill in the lamination plane header */
	struct mitsu9550_plane *matte = (struct mitsu9550_plane *)(job->databuf + job->datalen);
	matte->cmd[0] = 0x1b;
	matte->cmd[1] = 0x5a;
	matte->cmd[2] = 0x54;
	matte->cmd[3] = 0x10;
	matte->row_offset = 0;
	matte->col_offset = 0;
	matte->cols = cpu_to_be16(job->hdr1.cols);
	matte->rows = cpu_to_be16(job->hdr1.rows);
	job->datalen += sizeof(struct mitsu9550_plane);

	ret = mitsu_readlamdata(MITSU_M98xx_LAMINATE_FILE, LAMINATE_STRIDE,
				job->databuf, &job->datalen,
				job->rows, job->cols, 2);
	if (ret)
		return ret;

	/* Fill in the lamination plane footer */
	job->databuf[job->datalen++] = 0x1b;
	job->databuf[job->datalen++] = 0x50;
	job->databuf[job->datalen++] = 0x56;
	job->databuf[job->datalen++] = 0x00;

	return CUPS_BACKEND_OK;
}

static int mitsu9550_get_status(struct mitsu9550_ctx *ctx, uint8_t *resp, int type);
static const char *mitsu9550_media_types(uint8_t type, uint8_t is_s);

static void *mitsu9550_init(void)
{
	struct mitsu9550_ctx *ctx = malloc(sizeof(struct mitsu9550_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct mitsu9550_ctx));

	return ctx;
}

static int mitsu9550_attach(void *vctx, struct dyesub_connection *conn, uint8_t jobid)
{
	struct mitsu9550_ctx *ctx = vctx;
	struct mitsu9550_media media;

	UNUSED(jobid);

	ctx->conn = conn;

	if (ctx->conn->type == P_MITSU_9550S ||
	    ctx->conn->type == P_MITSU_9800S)
		ctx->is_s = 1;

	if (ctx->conn->type == P_MITSU_9800 ||
	    ctx->conn->type == P_MITSU_9800S ||
	    ctx->conn->type == P_MITSU_9810) {
		ctx->is_98xx = 1;
		ctx->need_lib = 1;
		ctx->lut_fname = MITSU_M98xx_LUT_FILE;
	}

	if (ctx->conn->type == P_MITSU_CP30D) {
		ctx->need_lib = 1;
	}
	if (ctx->need_lib) {
#if defined(WITH_DYNAMIC)
		/* Attempt to open the library */
		if (mitsu_loadlib(&ctx->lib, ctx->conn->type))
#endif
			WARNING("Dynamic library support not loaded, will be unable to print.");
	}

	if (ctx->conn->type == P_MITSU_CP30D) {
		ctx->footer_len = 6;
		ctx->lut_fname = MITSU_CP30D_LUT_FILE;
	} else {
		ctx->footer_len = 4;
	}

	if (test_mode < TEST_MODE_NOATTACH) {
		uint8_t buf[48];
		if (mitsu9550_get_status(ctx, (uint8_t*) &media, CP9XXX_STS_MEDIA))
			return CUPS_BACKEND_FAILED;

		/* Get FW Version */
		if (mitsu9550_get_status(ctx, buf, CP9XXX_STS_FWVER))
			return CUPS_BACKEND_FAILED;
		memcpy(ctx->fwver, &buf[6], 6);
		ctx->fwver[6] = 0;
		// XXX get serial number too?
	} else {
		int media_code = 0x2;
		if (getenv("MEDIA_CODE"))
			media_code = atoi(getenv("MEDIA_CODE")) & 0xf;

		media.max = cpu_to_be16(400);
		media.remain = cpu_to_be16(330);
		media.remain2 = cpu_to_be16(330);
		media.type = media_code;
		ctx->fwver[0] = 0;
	}

	ctx->marker.color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker.numtype = media.type;
	ctx->marker.levelmax = be16_to_cpu(media.max);

	if (ctx->conn->type == P_MITSU_CP30D) {
		ctx->marker.name = cp30_media_types(be16_to_cpu(media.max));
		ctx->marker.levelnow = be16_to_cpu(media.remain2);
	} else {
		ctx->marker.name = mitsu9550_media_types(media.type, ctx->is_s);
		ctx->marker.levelnow = be16_to_cpu(media.remain);
	}

	return CUPS_BACKEND_OK;
}

static void mitsu9550_cleanup_job(const void *vjob)
{
	const struct mitsu9550_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);

	free((void*)job);
}

static void mitsu9550_teardown(void *vctx) {
	struct mitsu9550_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->m98xxdata)
		ctx->lib.CP98xx_DestroyData(ctx->m98xxdata);

	mitsu_destroylib(&ctx->lib);

	free(ctx);
}

static int mitsu9550_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct mitsu9550_ctx *ctx = vctx;
	uint8_t buf[sizeof(struct mitsu9550_hdr1)];
	int remain, i;
	uint32_t planelen = 0;

	struct mitsu9550_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));
	job->is_raw = 1;
	job->common.jobsize = sizeof(*job);
	job->common.copies = copies;

top:
	/* Read in initial header */
	remain = sizeof(buf);
	while (remain > 0) {
		i = read(data_fd, buf + sizeof(buf) - remain, remain);
		if (i == 0) {
			mitsu9550_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		if (i < 0) {
			mitsu9550_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		remain -= i;
	}

	/* Sanity check */
	if (buf[0] != 0x1b || buf[1] != 0x57 || buf[3] != 0x2e) {
		if (!job->hdr1_present || !job->hdr2_present) {
			ERROR("Unrecognized data format (%02x%02x%02x%02x)!\n",
			      buf[0], buf[1], buf[2], buf[3]);
			mitsu9550_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		} else if (buf[0] == 0x1b &&
			   buf[1] == 0x5a &&
			   buf[2] == 0x54) {

			/* We're in the data portion now */
			if (buf[3] == 0x10)
				planelen *= 2;
			else if (ctx->is_98xx && buf[3] == 0x80)
				job->is_raw = 0;

			goto hdr_done;
		} else {
			ERROR("Unrecognized data block (%02x%02x%02x%02x)!\n",
			      buf[0], buf[1], buf[2], buf[3]);
			mitsu9550_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
	}

	switch(buf[2]) {
	case 0x20: /* header 1 */
		memcpy(&job->hdr1, buf, sizeof(job->hdr1));
		job->hdr1_present = 1;

		/* Work out printjob size */
		job->rows = be16_to_cpu(job->hdr1.rows);
		job->cols = be16_to_cpu(job->hdr1.cols);
		planelen = job->rows * job->cols;

		break;
	case 0x21: /* header 2 */
		memcpy(&job->hdr2, buf, sizeof(job->hdr2));
		job->hdr2_present = 1;
		break;
	case 0x22: /* header 3 */
		memcpy(&job->hdr3, buf, sizeof(job->hdr3));
		job->hdr3_present = 1;
		break;
	case 0x26: /* header 4 */
		memcpy(&job->hdr4, buf, sizeof(job->hdr4));
		job->hdr4_present = 1;
		break;
	default:
		ERROR("Unrecognized header format (%02x)!\n", buf[2]);
		mitsu9550_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Read in the next chunk */
	goto top;

hdr_done:

	/* Read in CP98xx data tables if necessary */
	if (ctx->is_98xx && !job->is_raw && !ctx->m98xxdata) {
		char full[2048];

		if (!ctx->lib.dl_handle) {
			ERROR("!!! Image Processing Library not found, aborting!\n");
			mitsu9550_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}

		snprintf(full, sizeof(full), "%s/%s", corrtable_path, MITSU_M98xx_DATATABLE_FILE);

		DEBUG("Reading in 98xx data from disk\n");
		ctx->m98xxdata = ctx->lib.CP98xx_GetData(full);
		if (!ctx->m98xxdata) {
			ERROR("Unable to read 98xx data table file '%s'\n", full);
		}
	}

	if (job->is_raw) {
		/* We have three planes + headers and the final terminator to read */
		remain = 3 * (planelen + sizeof(struct mitsu9550_plane)) + ctx->footer_len;
	} else {
		/* We have one plane + header and the final terminator to read */
		remain = planelen * 3 + sizeof(struct mitsu9550_plane) +  ctx->footer_len;
	}

	/* Mitsu9600 windows spool uses more, smaller blocks, but plane data is the same */
	if (ctx->conn->type == P_MITSU_9600) {
		remain += 128 * sizeof(struct mitsu9550_plane); /* 39 extra seen on 4x6" */
	}

	/* 9550S/9800S doesn't typically sent over hdr4! */
	if (ctx->conn->type == P_MITSU_9550S ||
	    ctx->conn->type == P_MITSU_9800S) {
		/* XXX Has to do with error policy, but not sure what.
		   Mitsu9550-S/9800-S will set this based on a command,
		   but it's not part of the standard job spool */
		job->hdr4_present = 0;
	}

	/* Disable matte if the printer doesn't support it */
	if (job->hdr1.matte) {
		if (ctx->conn->type != P_MITSU_9810) {
			WARNING("Matte not supported on this printer, disabling\n");
			job->hdr1.matte = 0;
		} else if (job->is_raw) {
			remain += planelen + sizeof(struct mitsu9550_plane) + sizeof(struct mitsu9550_cmd);
		}
	}

	/* Allocate buffer for the payload */
	job->datalen = 0;
	job->databuf = malloc(remain);
	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		mitsu9550_cleanup_job(job);
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	/* Load up the data blocks.*/
	while(1) {
		/* Note that 'buf' needs to be already filled here! */
		struct mitsu9550_plane *plane = (struct mitsu9550_plane *)buf;

		/* Sanity check header... */
		if (plane->cmd[0] != 0x1b ||
		    plane->cmd[1] != 0x5a ||
		    plane->cmd[2] != 0x54) {
			ERROR("Unrecognized data read @%d (%02x%02x%02x%02x)!\n",
			      job->datalen,
			      plane->cmd[0], plane->cmd[1], plane->cmd[2], plane->cmd[3]);
			mitsu9550_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}

		/* Work out the length of this block */
		planelen = be16_to_cpu(plane->rows) * be16_to_cpu(plane->cols);
		if (plane->cmd[3] == 0x10)
			planelen *= 2;
		if (plane->cmd[3] == 0x80)
			planelen *= 3;

		/* Copy plane header into buffer */
		memcpy(job->databuf + job->datalen, buf, sizeof(buf));
		job->datalen += sizeof(buf);
		planelen -= sizeof(buf) - sizeof(struct mitsu9550_plane);

		/* Read in the spool data */
		while(planelen > 0) {
			i = read(data_fd, job->databuf + job->datalen, planelen);
			if (i == 0) {
				mitsu9550_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			if (i < 0) {
				mitsu9550_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			job->datalen += i;
			planelen -= i;
		}

		/* Try to read in the next chunk.  It will be one of:
		    - Additional block header (12B)
		    - Job footer (4B)
		*/
		i = read(data_fd, buf, ctx->footer_len);
		if (i == 0) {
			mitsu9550_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		if (i < 0) {
			mitsu9550_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}

		/* Is this a "job end" marker? */
		if (plane->cmd[0] == 0x1b &&
		    plane->cmd[1] == 0x50 &&
		    plane->cmd[3] == 0x00) {
			/* store it in the buffer */
			memcpy(job->databuf + job->datalen, buf, ctx->footer_len);
			job->datalen += ctx->footer_len;

			/* Unless we have a raw matte plane following,
			   we're done */
			if (job->hdr1.matte != 0x01 ||
			    !job->is_raw)
				break;
			remain = sizeof(buf);
		} else {
			/* It's part of a block header, mark what we've read */
			remain = sizeof(buf) - ctx->footer_len;
		}

		/* Read in the rest of the header */
		while (remain > 0) {
			i = read(data_fd, buf + sizeof(buf) - remain, remain);
			if (i == 0) {
				mitsu9550_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			if (i < 0) {
				mitsu9550_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			remain -= i;
		}
	}

	/* One bit of fixup */
	if (ctx->conn->type == P_MITSU_CP30D)
		job->is_raw = 0;

	/* Apply LUT, if job calls for it.. */
	if (ctx->lut_fname && !job->is_raw && job->hdr2.unkc[9]) {
		int ret;
		if (ctx->conn->type == P_MITSU_CP30D) {
			uint32_t planelen = job->rows * job->cols + sizeof(struct mitsu9550_plane);
			ret = mitsu_apply3dlut_plane(&ctx->lib, ctx->lut_fname,
						     job->databuf + sizeof(struct mitsu9550_plane),
						     job->databuf + (planelen + sizeof(struct mitsu9550_plane)),
						     job->databuf + (planelen * 2 + sizeof(struct mitsu9550_plane)),
						     job->cols, job->rows);
		} else {
			ret = mitsu_apply3dlut_packed(&ctx->lib, ctx->lut_fname,
						      job->databuf + sizeof(struct mitsu9550_plane),
						      job->cols, job->rows,
						      job->cols * 3, COLORCONV_BGR);
		}
		if (ret) {
			mitsu9550_cleanup_job(job);
			return ret;
		}

		job->hdr2.unkc[9] = 0;
	}

	/* Update printjob header to reflect number of requested copies */
	// XXX use larger?
	if (job->hdr2_present) {
		if (be16_to_cpu(job->hdr2.copies) < copies)
			job->hdr2.copies = cpu_to_be16(copies);
		copies = 1;
	}
	job->common.copies = copies;

	/* All further work is in main loop */
	if (test_mode >= TEST_MODE_NOPRINT)
		mitsu9550_main_loop(ctx, job, 1);

	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int mitsu9550_get_status(struct mitsu9550_ctx *ctx, uint8_t *resp, int type)
{
	struct mitsu9550_cmd cmd;
	int num, ret;

	/* Send Printer Query */
	cmd.cmd[0] = 0x1b;
	cmd.cmd[1] = 0x56; // XXX need version for 0x72
	cmd.cmd[2] = type;
	cmd.cmd[3] = 0x00;
	if ((ret = send_data(ctx->conn,
			     (uint8_t*) &cmd, sizeof(cmd))))
		return ret;
	ret = read_data(ctx->conn,
			resp, sizeof(struct mitsu9550_status), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(struct mitsu9550_status)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(struct mitsu9550_status));
		return CUPS_BACKEND_FAILED;
	}

	return CUPS_BACKEND_OK;
}

static const char *mitsu9550_media_types(uint8_t type, uint8_t is_s)
{
	if (is_s) {
		switch (type & 0xf) { /* values can be 0x0? or 0x4? */
		case 0x02:
			return "CK9015 (4x6)";
		case 0x04:
			return "CK9318 (5x7)";
		case 0x05:
			return "CK9523 (6x9)";
		default:
			return "Unknown";
		}
		return NULL;
	}

	switch (type & 0xf) { /* values can be 0x0? or 0x4? */
	case 0x01:
		return "CK9035 (3.5x5)";
	case 0x02:
		return "CK9046 (4x6)";
	case 0x03:
		return "CK9046PST (4x6)";
	case 0x04:
		return "CK9057 (5x7)";
	case 0x05:
		return "CK9069 (6x9)";
	case 0x06:
		return "CK9068 (6x8)";
	default:
		return "Unknown";
	}
	return NULL;
}

static int validate_media(int type, int media, int cols, int rows)
{
	switch(type) {
	case P_MITSU_9550:
		switch(media & 0xf) {
		case 0x01: /* 3.5x5 */
			if (cols != 1812 && rows != 1240)
				return 1;
			break;
		case 0x02: /* 4x6 */
		case 0x03: /* 4x6 postcard */
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 1184 && rows != 1240)
				return 1;
			break;
		case 0x04: /* 5x7 */
			if (cols != 1812)
				return 1;
			if (rows != 1240 && rows != 2452)
				return 1;
			break;
		case 0x05: /* 6x9 */
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 2792 &&
			    rows != 2956 && rows != 3146)
				return 1;
			break;
		case 0x06: /* V (6x8??) */
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 2792)
				return 1;
			break;
		default: /* Unknown */
			WARNING("Unknown media type %02x\n", media);
			break;
		}
		break;
	case P_MITSU_9550S:
		switch(media & 0xf) {
		case 0x02: /* 4x6 */
		case 0x03: /* 4x6 postcard */
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 1184 && rows != 1240)
				return 1;
			break;
		case 0x04: /* 5x7 */
			if (cols != 1812 && rows != 2452)
				return 1;
			break;
		case 0x05: /* 6x9 */
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 2792 &&
			    rows != 2956 && rows != 3146)
				return 1;
			break;
		case 0x06: /* V (6x8??) */
			if (cols != 2152)
				return 1;
			if (rows != 1416 && rows != 2792)
				return 1;
			break;
		default: /* Unknown */
			WARNING("Unknown media type %02x\n", media);
			break;
		}
		break;
	case P_MITSU_9600: // XXX 9600S doesn't support 5" media at all!
		switch(media & 0xf) {
		case 0x01: /* 3.5x5 */
			if (cols == 1572) {
				if (rows == 1076)
					break;
			} else if (cols == 3144) {
				if (rows == 2152)
					break;
			}
			return 1;
		case 0x02: /* 4x6 */
		case 0x03: /* 4x6 postcard */
			if (cols == 1868) {
				if (rows == 1228)
					break;
			} else if (cols == 3736) {
				if (rows == 2458)
					break;
			}
			return 1;
		case 0x04: /* 5x7 */
			if (cols == 1572) {
				if (rows == 1076 || rows == 2128)
					break;
			} else if (cols == 3144) {
				if (rows == 2152 || rows == 4256)
					break;
			}
			return 1;
		case 0x05: /* 6x9 */
			if (cols == 1868) {
				if (rows == 1228 || rows == 2442 || rows == 2564 || rows == 2730)
					break;
			} else if (cols == 3736) {
				if (rows == 2458 || rows == 4846 || rows == 5130 || rows == 5462)
					break;
			}
			return 1;
		case 0x06: /* V (6x8??) */
			if (cols == 1868) {
				if (rows == 1228 || rows == 2442)
					break;
			} else if (cols == 3736) {
				if (rows == 2458 || rows == 4846)
					break;
			}
			return 1;
		default: /* Unknown */
			WARNING("Unknown media type %02x\n", media);
			break;
		}
		break;
	case P_MITSU_9800:
	case P_MITSU_9810:
//	case P_MITSU_9820S:
		switch(media & 0xf) {
		case 0x01: /* 3.5x5 */
			if (cols != 1572 && rows != 1076)
				return 1;
			break;
		case 0x02: /* 4x6 */
		case 0x03: /* 4x6 postcard */
			if (cols != 1868 && rows != 1228)
				return 1;
			break;
		case 0x04: /* 5x7 */
			if (cols != 1572 && rows != 2128)
				return 1;
			break;
		case 0x05: /* 6x9 */
			if (cols != 1868)
				return 1;
			if (rows != 1228 && rows != 2442 &&
			    rows != 2564 && rows != 2730)
				return 1;
			break;
		case 0x06: /* V (6x8??) */
			if (cols != 1868)
				return 1;
			if (rows != 1228 && rows != 2442)
				return 1;
			break;
		default: /* Unknown */
			WARNING("Unknown media type %02x\n", media);
			break;
		}
		break;
	case P_MITSU_9800S:
		switch(media & 0xf) {
		case 0x02: /* 4x6 */
		case 0x03: /* 4x6 postcard */
			if (cols != 1868 && rows != 1228)
				return 1;
			break;
		case 0x04: /* 5x7 */
			if (cols != 1572 && rows != 2128)
				return 1;
			break;
		case 0x05: /* 6x9 */
			if (cols != 1868)
				return 1;
			if (rows != 1228 && rows != 2442 &&
			    rows != 2564 && rows != 2730)
				return 1;
			break;
		case 0x06: /* V (6x8??) */
			if (cols != 1868)
				return 1;
			if (rows != 1228 && rows != 2442)
				return 1;
			break;
		default: /* Unknown */
			WARNING("Unknown media type %02x\n", media);
			break;
		}
		break;
	case P_MITSU_CP30D:
		if (cols != 1600)
			return 1;
		if (rows != 1200 && rows != 2100)
			return 1;
		// XXX validate media type vs job size.  Don't know readback codes.
		// S == 1600x1200
		// L == 1600x2100 ( type 00 ?)
		break;
	default:
		WARNING("Unknown printer type %d\n", type);
		break;
	}
	return CUPS_BACKEND_OK;
}

static int mitsu9550_main_loop(void *vctx, const void *vjob, int wait_for_return) {
	struct mitsu9550_ctx *ctx = vctx;
	struct mitsu9550_cmd cmd;
	uint8_t rdbuf[READBACK_LEN];
	uint8_t *ptr;

	int ret;
#if 0
	int copies = 1;
#endif

	struct mitsu9550_printjob *job = (struct mitsu9550_printjob*) vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	/* Okay, let's do this thing */
	ptr = job->databuf;

	int sharpness = job->hdr2.unkc[7];
	job->hdr2.unkc[7] = 0;  /* Clear "sharpness" parameter */

	/* Do the 98xx processing here */
	if (!ctx->is_98xx || job->is_raw)
		goto non_98xx;

	/* Special CP98xx handling code */
	uint8_t *newbuf;
	uint32_t newlen = 0;
	int i, remain, planelen;

	planelen = job->rows * job->cols * 2;
	remain = (job->hdr1.matte ? 4 : 3) * (planelen + sizeof(struct mitsu9550_plane)) + sizeof(struct mitsu9550_cmd) * (job->hdr1.matte? 2 : 1) + LAMINATE_STRIDE * 2;
	newbuf = malloc(remain);
	if (!newbuf) {
		ERROR("Memory allocation Failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	DEBUG("Running print data through processing library\n");

	/* Create band images for input and output */
	struct BandImage input;
	struct BandImage output;

	uint8_t *convbuf = malloc(planelen * 3);
	if (!convbuf) {
		free(newbuf);
		ERROR("Memory allocation Failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	input.origin_rows = input.origin_cols = 0;
	input.rows = job->rows;
	input.cols = job->cols;
	input.imgbuf = job->databuf + sizeof(struct mitsu9550_plane);
	input.bytes_per_row = job->cols * 3;

	output.origin_rows = output.origin_cols = 0;
	output.rows = job->rows;
	output.cols = job->cols;
	output.imgbuf = convbuf;
	output.bytes_per_row = job->cols * 3 * sizeof(uint16_t);

	if (!ctx->lib.CP98xx_DoConvert(ctx->m98xxdata, &input, &output, job->hdr2.mode, sharpness, job->hdr2.unkc[8])) {
		free(convbuf);
		free(newbuf);
		ERROR("CP98xx_DoConvert() failed!\n");
		return CUPS_BACKEND_FAILED;
	}

	/* Clear special extension flags used by our backend */
	if (job->hdr2.mode == 0x11)
		job->hdr2.mode = 0x10;
	job->hdr2.unkc[8] = 0;  /* Clear "already reversed" flag */

	/* Library is done, but its output is packed YMC16.
	   We need to convert this to planar YMC16, with a header for
	   each plane. */
	uint8_t *yPtr, *mPtr, *cPtr;
	int j, offset;

	yPtr = newbuf + newlen;
	memcpy(yPtr, job->databuf, sizeof(struct mitsu9550_plane));
	yPtr[3] = 0x10;  /* ie 16bpp data */
	yPtr += sizeof(struct mitsu9550_plane);
	newlen += sizeof(struct mitsu9550_plane) + planelen;

	mPtr = newbuf + newlen;
	memcpy(mPtr, job->databuf, sizeof(struct mitsu9550_plane));
	mPtr[3] = 0x10;  /* ie 16bpp data */
	mPtr += sizeof(struct mitsu9550_plane);
	newlen += sizeof(struct mitsu9550_plane) + planelen;

	cPtr = newbuf + newlen;
	memcpy(cPtr, job->databuf, sizeof(struct mitsu9550_plane));
	cPtr[3] = 0x10;  /* ie 16bpp data */
	cPtr += sizeof(struct mitsu9550_plane);
	newlen += sizeof(struct mitsu9550_plane) + planelen;

	for (offset = 0, i = 0; i < output.rows ; i++) {
		for (j = 0 ; j < output.cols ; j ++, offset += 3) {
			yPtr[i*output.cols + j] = convbuf[0 + offset];
			mPtr[i*output.cols + j] = convbuf[1 + offset];
			cPtr[i*output.cols + j] = convbuf[2 + offset];
		}
	}

	/* All done with conversion buffer, nuke it */
	free(convbuf);

	/* And finally, append the job footer. */
	memcpy(newbuf + newlen, job->databuf + sizeof(struct mitsu9550_plane) + planelen/2 * 3, ctx->footer_len);
	newlen += sizeof(struct mitsu9550_cmd);

	/* Clean up, and move pointer to new buffer; */
	free(job->databuf);
	job->databuf = newbuf;
	job->datalen = newlen;
	ptr = job->databuf;

	/* Now handle the matte plane generation */
	if (job->hdr1.matte) {
		if ((i = mitsu98xx_fillmatte(job))) {
			return i;
		}
	}

non_98xx:
	/* Bypass */
	if (test_mode >= TEST_MODE_NOPRINT)
		return CUPS_BACKEND_OK;

top:
	if (ctx->is_s) {
		int num;

		/* Send "unknown 1" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x53;
		cmd.cmd[2] = 0xc5;
		cmd.cmd[3] = 0x9d;
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;

		/* Send "unknown query 1" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x4b;
		cmd.cmd[2] = 0x7f;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;

		ret = read_data(ctx->conn,
				rdbuf, READBACK_LEN, &num);
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		// XXX no idea how to interpret this.
	}

	if (ctx->conn->type == P_MITSU_9800S) {
		int num;

		/* Send "unknown query 2" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x4b;
		cmd.cmd[2] = 0x01;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;

		ret = read_data(ctx->conn,
				rdbuf, READBACK_LEN, &num);
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		// XXX no idea how to interpret this.
	}

	/* Sanity-check the printer state. */
	{
		QUERY_STATUS_I;						\
		if (validate_media(ctx->conn->type, media->type, job->cols, job->rows)) { \
			ERROR("Incorrect media (%u) type for printjob (%ux%u)!\n", media->type, job->cols, job->rows); \
			return CUPS_BACKEND_HOLD;			\
		}							\
		QUERY_STATUS_II;					\
		QUERY_STATUS_IV;					\
	}

	/*** Now it's time for the actual print job! ***/

	/* Send printjob headers from spool data */
	if (job->hdr1_present)
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &job->hdr1, sizeof(job->hdr1))))
			return CUPS_BACKEND_FAILED;
	if (job->hdr2_present)
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &job->hdr2, sizeof(job->hdr2))))
			return CUPS_BACKEND_FAILED;
	if (job->hdr3_present)
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &job->hdr3, sizeof(job->hdr3))))
			return CUPS_BACKEND_FAILED;
	if (job->hdr4_present)
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &job->hdr4, sizeof(struct mitsu9550_hdr4))))
			return CUPS_BACKEND_FAILED;

	if (ctx->is_s) {
		/* I think this a "clear memory' command...? */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x5a;
		cmd.cmd[2] = 0x43;
		cmd.cmd[3] = 0x00;

		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	}

	/* Send over plane data */
	while(ptr < (job->databuf + job->datalen)) {
		struct mitsu9550_plane *plane = (struct mitsu9550_plane *)ptr;
		if (plane->cmd[0] != 0x1b ||
		    plane->cmd[1] != 0x5a ||
		    plane->cmd[2] != 0x54)
			break;

		planelen = be16_to_cpu(plane->rows) * be16_to_cpu(plane->cols);
		if (plane->cmd[3] == 0x10)
			planelen *= 2;

		if ((ret = send_data(ctx->conn,
				     (uint8_t*) ptr, sizeof(struct mitsu9550_plane))))
			return CUPS_BACKEND_FAILED;
		ptr += sizeof(struct mitsu9550_plane);
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) ptr, planelen)))
			return CUPS_BACKEND_FAILED;
		ptr += planelen;
	}

	/* Query statuses after sending data */
	{
		QUERY_STATUS_I;
		QUERY_STATUS_II;
		QUERY_STATUS_IV;
	}

	/* Send "end data" command */
	if (ctx->conn->type == P_MITSU_9550S) {
		/* Override spool, which may be wrong */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x50;
		cmd.cmd[2] = 0x47;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	} else if (ctx->conn->type == P_MITSU_9800S) {
		/* Override spool, which may be wrong */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x50;
		cmd.cmd[2] = 0x4e;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	} else {
		/* Send from spool file */
		if ((ret = send_data(ctx->conn,
				     ptr, ctx->footer_len)))
			return CUPS_BACKEND_FAILED;
	}
	ptr += ctx->footer_len;

	/* Don't forget the 9810's matte plane */
	if (job->hdr1.matte) {
		struct mitsu9550_plane *plane = (struct mitsu9550_plane *)ptr;
		planelen = be16_to_cpu(plane->rows) * be16_to_cpu(plane->cols);

		if (plane->cmd[3] == 0x10)
			planelen *= 2;

		// XXX include a status loop here too?
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) ptr, sizeof(struct mitsu9550_plane))))
			return CUPS_BACKEND_FAILED;
		ptr += sizeof(struct mitsu9550_plane);
		if ((ret = send_data(ctx->conn,
				     (uint8_t*) ptr, planelen)))
			return CUPS_BACKEND_FAILED;
		ptr += planelen;

		/* Send "lamination end data" command from spool file */
		if ((ret = send_data(ctx->conn,
				     ptr, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
//		ptr += sizeof(cmd);   /* Unnecessary */
	}

	/* Status loop, run until printer reports completion */
	while(1) {
		sleep(1);

		QUERY_STATUS_I;
		QUERY_STATUS_II;

		if (ctx->conn->type == P_MITSU_CP30D) {
			ret = mitsu9550_get_status(ctx, rdbuf, CP9XXX_STS_x32);
			if (ret < 0)
				return CUPS_BACKEND_FAILED;

			if (sts30->err == CP30_ERR_TRAYFULL) {
				ERROR("Output Tray Full!\n");
				return CUPS_BACKEND_STOP;
			}
			// XXX figure out remaining copy count?
			// print copy remaining

			if (sts30->sts == CP30_STS_IDLE)  /* If printer transitions to idle */
				break;

			// XXX if (!wait_for_return && copies_remaining == 0) break...

			if (!wait_for_return && sts30->sts != CP30_STS_IDLE) {
				INFO("Fast return mode enabled.\n");
				break;
			}

			QUERY_STATUS_IIIB;
		} else {
			ret = mitsu9550_get_status(ctx, rdbuf, CP9XXX_STS_x30);
			if (ret < 0)
				return CUPS_BACKEND_FAILED;

			INFO("%03d copies remaining\n", be16_to_cpu(sts->copies));

			if (!sts->sts1) /* If printer transitions to idle */
				break;

			if (!wait_for_return && !be16_to_cpu(sts->copies)) { /* No remaining prints */
				INFO("Fast return mode enabled.\n");
				break;
			}

			if (!wait_for_return && !sts->sts5) {
				INFO("Fast return mode enabled.\n");
				break;
			}
			QUERY_STATUS_III;
		}
	}

	INFO("Print complete\n");

	return CUPS_BACKEND_OK;
}

static void mitsu9550_dump_media(struct mitsu9550_ctx *ctx, struct mitsu9550_media *resp)
{
	INFO("Media type       : %02x (%s)\n",
	     resp->type, (ctx->conn->type == P_MITSU_CP30D ? cp30_media_types(be16_to_cpu(resp->max)):
			  mitsu9550_media_types(resp->type, ctx->is_s)));
	INFO("Media remaining  : %03d/%03d\n",
	     (ctx->conn->type == P_MITSU_CP30D) ? be16_to_cpu(resp->remain2) : be16_to_cpu(resp->remain), be16_to_cpu(resp->max));
}

static void mitsu9550_dump_status(struct mitsu9550_status *resp)
{
	INFO("Printer status    : %02x (%s)\n",
	     resp->sts1, resp->sts1 ? "Printing": "Idle");
	INFO("Pages remaining   : %03d\n",
	     be16_to_cpu(resp->copies));
	INFO("Other status      : %02x %02x %02x %02x  %02x %02x\n",
	     resp->sts2, resp->sts3, resp->sts4,
	     resp->sts5, resp->sts6, resp->sts7);
}

static void mitsucp30_dump_status(struct mitsucp30_status *resp)
{
	INFO("Printer status   : %02x %02x\n",
	     resp->sts, resp->sts2);
	INFO("Printer error    : %s (%04x)\n",
	     cp30_errors(be16_to_cpu(resp->err)),
	     be16_to_cpu(resp->err));
}

static void mitsu9550_dump_status2(struct mitsu9550_status2 *resp)
{
	INFO("Prints remaining on media : %03d\n",
	     be16_to_cpu(resp->remain));
}

static int mitsu9550_query_media(struct mitsu9550_ctx *ctx)
{
	struct mitsu9550_media resp;
	int ret;

	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, CP9XXX_STS_MEDIA);

	if (!ret)
		mitsu9550_dump_media(ctx, &resp);

	return ret;
}

static int mitsu9550_query_status2(struct mitsu9550_ctx *ctx)
{
	struct mitsu9550_status2 resp;
	int ret;

	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, CP9XXX_STS_x21);

	if (!ret && ctx->conn->type != P_MITSU_CP30D)
		mitsu9550_dump_status2(&resp);

	return ret;
}

static int mitsu9550_query_status(struct mitsu9550_ctx *ctx)
{
	int ret;

	if (ctx->conn->type == P_MITSU_CP30D) {
		struct mitsucp30_status resp;
		ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, CP9XXX_STS_x32);
		if (!ret) {
			mitsucp30_dump_status(&resp);
			ret = mitsu9550_query_status2(ctx);
		}
	} else {
		struct mitsu9550_status resp;
		ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, CP9XXX_STS_x30);
		if (!ret) {
			mitsu9550_dump_status(&resp);
			ret = mitsu9550_query_status2(ctx);
		}
	}

	return ret;
}

static int mitsu9550_query_statusX(struct mitsu9550_ctx *ctx)
{
	struct mitsu9550_status2 resp;
	int ret;

#if 0
	int i;
	for (i = 0 ; i < 256 ; i++) {
		ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, i);
		if (!ret) {
			DEBUG("Query %02x OK\n", i);
		}
	}
#else
	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, CP9XXX_STS_x20);
	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, CP9XXX_STS_FWVER);
	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, CP9XXX_STS_x22);
	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, CP9XXX_STS_x26);
	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, CP9XXX_STS_x32);
#endif
	return ret;
}

static int mitsu9550_query_serno(struct dyesub_connection *conn, char *buf, int buf_len)
{
	struct mitsu9550_cmd cmd;
	uint8_t rdbuf[READBACK_LEN];
	uint8_t *ptr;
	int ret, num, i;

	cmd.cmd[0] = 0x1b;
	cmd.cmd[1] = 0x72;
	cmd.cmd[2] = 0x6e;
	cmd.cmd[3] = 0x00;

	if ((ret = send_data(conn,
                             (uint8_t*) &cmd, sizeof(cmd))))
                return (ret < 0) ? ret : CUPS_BACKEND_FAILED;

	ret = read_data(conn,
			rdbuf, READBACK_LEN, &num);

	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	if ((unsigned int)num < sizeof(cmd) + 1) /* Short read */
		return CUPS_BACKEND_FAILED;

	if (rdbuf[0] != 0xe4 ||
	    rdbuf[1] != 0x72 ||
	    rdbuf[2] != 0x6e ||
	    rdbuf[3] != 0x00) /* Bad response */
		return CUPS_BACKEND_FAILED;

	/* If response is truncated, handle it */
	num -= (sizeof(cmd) + 1);
	if ((unsigned int) num != rdbuf[4])
		WARNING("Short serno read! (%d vs %u)\r\n",
			num, rdbuf[4]);

	/* model and serial number are encoded as 16-bit unicode,
	   little endian, separated by spaces. */
	i = num;
	ptr = rdbuf + 5;
	while (i > 0 && buf_len > 1) {
		if (*ptr != 0x20)
			*buf++ = *ptr;
		buf_len--;
		ptr += 2;
		i -= 2;
	}
	*buf = 0; /* Null-terminate the returned string */

	return ret;
}

static int mitsu9550_cancel_job(struct mitsu9550_ctx *ctx)
{
	int ret;

	uint8_t buf[2] = { 0x1b, 0x44 };
	ret = send_data(ctx->conn, buf, sizeof(buf));

	return ret;
}

static void mitsu9550_cmdline(void)
{
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -X ]           # Cancel current job\n");
//	DEBUG("\t\t[ -Z ]           # Dump all parameters\n");
}

static int mitsu9550_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct mitsu9550_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "msXZ")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'm':
			j = mitsu9550_query_media(ctx);
			break;
		case 's':
			j = mitsu9550_query_status(ctx);
			INFO("Firmware Version: %s\n", ctx->fwver);
			break;
		case 'X':
			j = mitsu9550_cancel_job(ctx);
			break;
		case 'Z':
			j = mitsu9550_query_statusX(ctx);
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return CUPS_BACKEND_OK;
}

static int mitsu9550_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct mitsu9550_ctx *ctx = vctx;
	struct mitsu9550_media media;

	/* Query printer status */
	if (mitsu9550_get_status(ctx, (uint8_t*) &media, CP9XXX_STS_MEDIA))
		return CUPS_BACKEND_FAILED;

	if (ctx->conn->type == P_MITSU_CP30D) {
		ctx->marker.levelnow = be16_to_cpu(media.remain2);
	} else {
		ctx->marker.levelnow = be16_to_cpu(media.remain);
	}

	*markers = &ctx->marker;
	*count = 1;

	return CUPS_BACKEND_OK;
}

static const char *mitsu9550_prefixes[] = {
	"mitsu9xxx", // Family driver, do not nuke.
	// Backwards compatibility
	"mitsu9000", "mitsu9500", "mitsu9550", "mitsu9600", "mitsu9800", "mitsu9810",
	NULL
};

/* Exported */
const struct dyesub_backend mitsu9550_backend = {
	.name = "Mitsubishi CP9xxx family",
	.version = "0.63" " (lib " LIBMITSU_VER ")",
	.uri_prefixes = mitsu9550_prefixes,
	.cmdline_usage = mitsu9550_cmdline,
	.cmdline_arg = mitsu9550_cmdline_arg,
	.init = mitsu9550_init,
	.attach = mitsu9550_attach,
	.teardown = mitsu9550_teardown,
	.cleanup_job = mitsu9550_cleanup_job,
	.read_parse = mitsu9550_read_parse,
	.main_loop = mitsu9550_main_loop,
	.query_serno = mitsu9550_query_serno,
	.query_markers = mitsu9550_query_markers,
	.devices = {
		{ 0x06d3, 0x0395, P_MITSU_9550, NULL, "mitsubishi-9000dw"}, // XXX -am instead?
		{ 0x06d3, 0x0394, P_MITSU_9550, NULL, "mitsubishi-9000dw"},
		{ 0x06d3, 0x0393, P_MITSU_9550, NULL, "mitsubishi-9500dw"},
		{ 0x06d3, 0x03a1, P_MITSU_9550, NULL, "mitsubishi-9550dw"},
		{ 0x06d3, 0x03a1, P_MITSU_9550, NULL, "mitsubishi-9550d"}, /* Duplicate */
		{ 0x06d3, 0x03a5, P_MITSU_9550S, NULL, "mitsubishi-9550dw-s"}, // or DZ/DZS/DZU
		{ 0x06d3, 0x03a5, P_MITSU_9550S, NULL, "mitsubishi-9550dz"}, /* Duplicate */
		{ 0x06d3, 0x03a9, P_MITSU_9600, NULL, "mitsubishi-9600dw"},
//	{ 0x06d3, USB_PID_MITSU_9600D, P_MITSU_9600S, NULL, "mitsubishi-9600dw-s"},
		{ 0x06d3, 0x03ab, P_MITSU_CP30D, NULL, "mitsubishi-cp30dw"},
		{ 0x06d3, 0x03ad, P_MITSU_9800, NULL, "mitsubishi-9800dw"},
		{ 0x06d3, 0x03ad, P_MITSU_9800, NULL, "mitsubishi-9800d"}, /* Duplicate */
		{ 0x06d3, 0x03ae, P_MITSU_9800S, NULL, "mitsubishi-9800dw-s"},
		{ 0x06d3, 0x03ae, P_MITSU_9800S, NULL, "mitsubishi-9800dz"}, /* Duplicate */
		{ 0x06d3, 0x3b21, P_MITSU_9810, NULL, "mitsubishi-9810dw"},
		{ 0x06d3, 0x3b21, P_MITSU_9810, NULL, "mitsubishi-9810d"}, /* Duplicate */
//	{ 0x06d3, USB_PID_MITSU_9820DS, P_MITSU_9820S, NULL, "mitsubishi-9820dw-s"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* Mitsubish CP-30/9500/9550/9600/9800/9810/9820 spool format:

   Spool file consists of 3 or 4 50-byte headers, followed by three
   image planes, each with a 12-byte header, then a 4 or 6-byte footer.

   All multi-byte numbers are big endian.

   ~~~ Header 1

   1b 57 20 2e 00 QQ QQ 00  00 00 00 00 00 00 XX XX :: XX XX == columns
   YY YY 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: YY YY == rows
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: QQ == 0x0a90 on 9810, 0x0a10 on all others.
   00 00

   ~~~ Header 2

   1b 57 21 2e 00 GG 00 HH  QQ QQ 00 00 00 00 00 00 :: ZZ ZZ = num copies (>= 0x01)
   00 00 00 00 00 00 00 00  00 00 00 00 ZZ ZZ 00 00 :: YY = 00/80 Fine/SuperFine (9550), 10/80 Fine/Superfine (98x0), 00 (9600), 0x80/0x00 Powersave/Normal (CP30)
   XX 00 00 00 00 00 YY 00  00 00 00 00 00 00 SS TT :: XX = 00 normal, 83 Cut 2x6 (9550 only!)
   RR II                                            :: QQ QQ = 0x0803 on 9550, 0x0801 on 98x0, 0x0003 on 9600, 0xa803 on 9500, 0x0802 on CP30
                                                    :: RR = 01 for "use LUT" on 98xx, 0x00 otherwise.  Extension to stock.
						    :: TT = 01 for "already reversed". Extension to stock.
						    :: SS == sharpening level, 0 for off, 1-10 otherwise. Extesion to stock.
                                                    :: GG == 0x00 on CP30, 0x80 on others
						    :: HH == 0x20 on CP30, 0x22 on others
						    :: II == 0x00 on CP30, 0x01 on others.

   ~~~ Header 3 (9550, 9800-S, and CP30 only..)

   1b 57 22 2e 00 QQ 00 00  00 00 00 XX 00 00 00 00 :: XX = 00 normal, 01 FineDeep
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: QQ = 0xf0 on 9500, 0x40 on the rest
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   00 00

   ~~~ Header 4 (all but 9550-S and 9800-S, involves error policy?)

   1b 57 26 2e 00 QQ TT 00  00 00 00 SS RR ZZ VV WW :: QQ = 0x70 on 9550/98x0, 0x60 on 9600 or 9800S, 0x3f on CP30 [also seen 0x20 & 0x00 on 9550S]
   WW 00 WW 00 00 00 00 00  00 00 00 00 00 00 00 00 :: RR = 0x01 on 9550/98x0/CP30, 0x00 on 9600  [ "ignore errors? ]
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: SS = 0x01 on 9800S, 0x00 otherwise.
   00 00
                                                    :: ZZ = Unknown; 0x01 [9550S/CP30] & 0x00 [9500].
                                                    :: TT = 0x80 on CP30, 0x00 otherwise
                                                    :: VV = 0x80 on CP30, 0x00 otherwise
                                                    :: WW = 0x10 on CP30, 0x00 otherwise

  ~~~~ Data follows:

   Format is:  planar YMC16 for 98x0 (but only 12 bits used, BIG endian)
               planar RGB for all others

   1b 5a 54 ?? RR RR CC CC  07 14 04 d8  :: 0714 == columns, 04d8 == rows
                                         :: RRRR == row offset for data, CCCC == col offset for data
		                         :: ?? == 0x00 for 8bpp, 0x10 for 16/12bpp.
					 ::    0x80 for PACKED BGR!

   Data follows immediately, no padding.

   1b 5a 54 00 00 00 00 00  07 14 04 d8  :: Another plane.

   Data follows immediately, no padding.

   1b 5a 54 00 00 00 00 00  07 14 04 d8  :: Another plane.

   Data follows immediately, no padding.

  ~~~~ Footer:

   1b 50 41 00  (9500AM)
   1b 50 46 00  (9550)
   1b 50 47 00  (9550-S)
   1b 50 48 00  (9600)
   1b 50 4c 00  (9800/9810)
   1b 50 4d 00  (9000)
   1b 50 4e 00  (9800-S)
   1b 50 51 00  (CP3020DA)
   1b 50 57 00  (9500)
   1b 50 52 00 00 00 (CP30)

   Unknown: 9600-S, 9820-S, 1 other..

  ~~~~ Lamination data follows (on 9810 only, if matte selected)

   1b 5a 54 10 00 00  00 00 06 24 04 34

   Data follows immediately, no padding.

   1b 50 56 00  (Lamination footer)

  ~~~~ QUESTIONS:

   * Lamination control?
   * Other 9550 multi-cut modes (on 6x9 media: 4x6*2, 4.4x6*2, 3x6*3, 2x6*4)
   * 9600/98x0 multi-cut modes?

 ***********************************************************************

 * Mitsubishi CP-9xxx Communications Protocol:

  JOB CANCEL

 -> 1b 44

  [[ Unknown query 1 ]]

 -> 1b 4b 7f 00
 <- eb 4b 8f  00 02  00 5e  [[ '02' seems to be a length ]]

  [[ unknown query 2, 9800-only ]]

 -> 1b 4b 01 00
 <- e4 4b 01  00 02  00 78

  PRINT START

 -> 1b 50 47 00  [9550S]
 -> 1b 50 4e 00  [9800S]
 -> 1b 50 56 00  [9810 Lamination]
    [[ see "footer" above for other models ]]

  [[ Unknown 1 ]]

 -> 1b 51 c5 9d

  [[ Unknown ]]

 -> 1b 53 00 00

  [[ Unknown ]]

 -> 1b 53 c5 9d

  [[ Status Query B ]]

 -> 1b 56 20 00                                        [ CP30 ]
 <- 20 2e 00 0a 10 00 00 00 00 00 00 00 CC CC RR RR :: CC == cols
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 :: RR == rows (04b0 for L??)
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

  [[ Status Query C ]]

 -> 1b 56 21 00                                       [ Most models ]
 <- 21 2e 00 80 00 22 XX 0b  00 00 00 00 00 00 00 00 :: XX == a8 (most) 08 (9810)
    00 00 00 00 00 00 00 00  00 00 00 QQ 00 00 00 00 :: QQ == Prints in job?
    00 00 00 00 00 00 00 00  00 00 NN NN 0a 00 00 01 :: NN NN = Remaining media

    21 2e 00 00 00 20 08 02  00 00 00 00 00 00 00 00   [ CP30 ]
    00 00 00 00 00 00 00 00  00 00 00 XX 00 00 00 00  :: XX == seen 01 and 02.  Unknown.
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00

  [[ Status Query D (unknown, possibly lifetime print count?) ]]

 -> 1b 56 22 00                                       [ CP30 ]
 <- 22 2e 00 40 00 00 00 00  00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00

  FIRWARE VERSIONS

 -> 1b 56 23 00                                       [ CP30 ]
 <- 23 2e 00 00 00 00 32 32  33 46 31 30 32 32 34 42   223F10 224B10 ... 222A10
    31 30 00 00 00 00 00 00  32 32 32 41 31 30 00 00
    00 00 00 05 d9 3f 79 20  00 00 13 97 00 00 00 00

  MEDIA INFO

 -> 1b 56 24 00
 <- 24 2e 00 00 00 00 00 00  00 00 00 00 XX 00 TT 00 :: TT = Type (!CP30) ; XX = 0x02 on CP30, 0x00 otherwise.
    00 00 00 00 00 00 00 00  00 00 00 00 MM MM N1 N1 :: MM MM = Max prints
    NN NN 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: NN NN = Remaining (!CP30) ; N1 N1 = Remaining (CP30)

  [[ Status Query E ]]

 -> 1b 56 26 00                                        [ CP30 ]
 <- 26 2e 00 3f 80 00 00 00 00 00 01 01 80 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

  Status Query  [All models except CP30]

 -> 1b 56 30 00
 -> 30 2e 00 00 00 00 MM 00  NN NN ZZ 00 00 00 00 00 :: MM, NN, ZZ
    QQ RR SS 00 00 00 00 00  00 00 00 00 00 00 00 00 :: QQ, RR, SS
    00 00 00 00 00 00 00 00  00 00 00 00 TT UU 00 00 :: TT, UU

  Status Query [CP30]

 -> 1b 56 32 00
 <- 32 2e 00 00 00 43 00 50 00 33 00 30 00 44 00 20  :: Unicode, "CP30D 204578"
    00 32 00 30 00 34 00 35 00 37 00 38 00 00 SS SS  :: SS SS = status code (see CP30_STS_*)
    00 EE EE 00 00 00 NN 00 00 00 00 00 00 00 00 00  :: NN = remaining prints on paper
                                                     :: EE EE = error code  (see CP30_ERR_*)

  Status Query X (unknown)

 -> 1b 56 33 00
 <- ??? 48 bytes?

  Status Query Y (unknown)

 -> 1b 56 36 00
 <- ??? 48 bytes?

  [[ Header 1 -- See above ]]

 -> 1b 57 20 2e ....

  [[ Header 2 -- See above ]]

 -> 1b 57 21 2e ....

  [[ Header 3 -- See above ]]

 -> 1b 57 22 2e ....

  [[ Header 4 -- See above ]]

 -> 1b 57 26 2e ....

  DATA Start  [[ Or maybe it's "DATA Clear" ?  Only seen on -S models ]]

 -> 1b 5a 43 00

  PLANE Data

 -> 1b 5a 54 ?? 00 00 00 00  XX XX YY YY :: XX XX == Columns, YY YY == Rows
                                         :: ?? == x00 8bpp, x10 16bpp

    Followed by image plane data, XXXX * YYYY [ * 2 ] bytes

    [ Three planes are needed! ]

  Query Model & FW Version (XXX Confirm this!)

 -> 1b 72 01 00
 <- e4 82 01 00 LL 39 00 35  00 35 00 30 00 5a 00 20
    00 41 00 32 00 30 00 30  00 36 00 37 00

     'LL' is length.  Data is returned in 16-bit unicode, LE.
     Contents are model ('9550Z'), then space, then serialnum ('A20067')

  [[ Unknown query.. "Printer number" related?  Seen in driver dump ]]

 -> 1b 72 10 00
 <- e4 82 10 00 LL [ 10 unknown bytes.  guess. ]

  Query Model & Serial number

 -> 1b 72 6e 00
 <- e4 82 6e 00 LL 39 00 35  00 35 00 30 00 5a 00 20
    00 41 00 32 00 30 00 30  00 36 00 37 00

     'LL' is length.  Data is returned in 16-bit unicode, LE.
     Contents are model ('9550Z'), then space, then serialnum ('A20067')


  **** After print starts, loop status/status b/media queries until printer idle

    MM, QQ RR SS, TT UU

 <- 00  3e 00 00  8a 44  :: Idle.
    00  7e 00 00  8a 44  :: Plane data submitted, pre "end data" cmd
    00  7e 40 01  8a 44  :: "end data" sent
    30  7e 40 01  8a 44
    38  7e 40 01  8a 44
    59  7e 40 01  8a 44
    59  7e 40 00  8a 44
    4d  7e 40 00  8a 44
     [...]
    43  7e 40 00  82 44
     [...]
    50  7e 40 00  80 44
     [...]
    31  7e 40 00  7d 44
     [...]
    00  3e 00 00  80 44  :: Idle.

  Also seen:

    00  3e 00 00  96 4b  :: Idle
    00  be 00 00  96 4b  :: Data submitted, pre "start"
    00  be 80 01  96 4b  :: print start sent
    30  be 80 01  96 4c
     [...]
    30  be 80 01  89 4b
    38  be 80 01  8a 4b
    59  be 80 01  8b 4b
     [...]
    4d  be 80 01  89 4b
     [...]
    43  be 80 01  89 4b
     [...]
    50  be 80 01  82 4b
     [...]
    31  be 80 01  80 4b
     [...]

    00  be 80 01  76 37  :: printed 4x6 on 6x9?

 Working theory of interpreting the status flags:

  MM :: 00 is idle, else mechanical printer state.
  NN :: Remaining prints in job, or 0x00 0x00 when idle.
  QQ :: ?? 0x3e + 0x40 or 0x80 (see below)
  RR :: ?? 0x00 is idle, 0x40 or 0x80 is "printing"?
  SS :: ?? 0x00 means "ready for another print" but 0x01 is "busy"
  TT :: ?? seen values between 0x76 through 0x96)
  UU :: ?? seen values between 0x37 and 0x4c -- temperature?
  ZZ :: ?? Error code  (08 = Door open on 9600)

  ***

   Other printer commands seen:

  [[ Set error policy ?? aka "header 4" ]]

 -> 1b 57 26 2e 00 QQ 00 00  00 00 00 00 RR ZZ 00 00 :: QQ/RR/ZZ 00 00 00 [9550S]
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ::          20 01 00 [9550S w/ ignore failures on]
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ::          70 01 01 [9550]
    00 00

 */
