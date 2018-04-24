/*
 *   Mitsubishi CP-9xxx Photo Printer Family CUPS backend
 *
 *   (c) 2014-2018 Solomon Peachy <pizza@shaftnet.org>
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

/* For Integration into gutenprint */
#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#define BACKEND mitsu9550_backend

#include "backend_common.h"

#define USB_VID_MITSU        0x06D3
#define USB_PID_MITSU_9500D  0x0393
#define USB_PID_MITSU_9000D  0x0394
#define USB_PID_MITSU_9000AM 0x0395
#define USB_PID_MITSU_9550D  0x03A1
#define USB_PID_MITSU_9550DS 0x03A5  // or DZ/DZS/DZU
#define USB_PID_MITSU_9600D  0x03A9
//#define USB_PID_MITSU_9600DS  XXXXXX
#define USB_PID_MITSU_9800D  0x03AD
#define USB_PID_MITSU_9800DS 0x03AE
#define USB_PID_MITSU_98__D  0x3B21
//#define USB_PID_MITSU_9810D   XXXXXX
//#define USB_PID_MITSU_9820DS  XXXXXX

#ifndef CORRTABLE_PATH
#ifdef PACKAGE_DATA_DIR
#define CORRTABLE_PATH PACKAGE_DATA_DIR "/backend_data"
#else
#error "Must define CORRTABLE_PATH or PACKAGE_DATA_DIR!"
#endif
#endif

#define MITSU_M98xx_LAMINATE_FILE CORRTABLE_PATH "M98MATTE.raw"
#define MITSU_M98xx_DATATABLE_FILE CORRTABLE_PATH "M98TABLE.dat"
#define MITSU_M98xx_LUT_FILE       CORRTABLE_PATH "M98XXL01.lut"
#define LAMINATE_STRIDE 1868
#define DATATABLE_SIZE  42204

/* Spool file structures */

/* Print parameters1 */
struct mitsu9550_hdr1 {
	uint8_t  cmd[4]; /* 1b 57 20 2e */
	uint8_t  unk[10]; /* 00 0a 10 00 [...] */
	uint16_t cols; /* BE */
	uint16_t rows; /* BE */
	uint8_t  matte;  /* CP9810/9820 only. 01 for matte, 00 glossy */
	uint8_t  null[31];
} __attribute__((packed));

/* Print parameters2 */
struct mitsu9550_hdr2 {
	uint8_t  cmd[4]; /* 1b 57 21 2e */
	uint8_t  unk[24]; /* 00 80 00 22 08 03 [...] */
	uint16_t copies; /* BE, 1-680 */
	uint8_t  null[2];
	uint8_t  cut; /* 00 == normal, 83 == 2x6*2 */
	uint8_t  unkb[5];
	uint8_t  mode; /* 00 == fine, 80 == superfine */
	uint8_t  unkc[11]; /* 00 [...] 00 01 */
} __attribute__((packed));

/* Fine Deep selection (9550 only) */
struct mitsu9550_hdr3 {
	uint8_t  cmd[4]; /* 1b 57 22 2e */
	uint8_t  unk[7]; /* 00 40 00 [...] */
	uint8_t  mode2;  /* 00 == normal, 01 == finedeep */
	uint8_t  null[38];
} __attribute__((packed));

/* Error policy? */
struct mitsu9550_hdr4 {
	uint8_t  cmd[4]; /* 1b 57 26 2e */
	uint8_t  unk[46]; /* 00 70 00 00 00 00 00 00 01 01 00 [...] */
} __attribute__((packed));

/* Data plane header */
struct mitsu9550_plane {
	uint8_t  cmd[4]; /* 1b 5a 54 XX */  /* XX == 0x10 if 16bpp, 0x00 for 8bpp */
	uint16_t col_offset; /* BE, normally 0, where we start dumping data */
	uint16_t row_offset; /* BE, normally 0, where we start dumping data */
	uint16_t cols;       /* BE */
	uint16_t rows;       /* BE */
} __attribute__((packed));

/* CP98xx Tabular Data */
struct mitsu98xx_data {
	uint16_t GNMby[256];   // @0
	uint16_t GNMgm[256];   // @512
	uint16_t GNMrc[256];   // @1024
	double   GammaParams[3]; // @1536
	uint8_t  KH[2048];     // @1560
	uint32_t unk_b[3];     // @3608

	struct {
		double  unka[256];  // @0
		double  unkb[256];  // @2048
		uint32_t unkc[10];  // @4096
		double  unkd[256];  // @4136
		double  unke[256];  // @6184
		uint32_t unkf[10];  // @8232
		double  unkg[256];  // @10320
		                    // @12368
	} WMAM; // @3620
	uint8_t  unc_d[4];    // @13940
	uint8_t  sharp[104];  // @13944
	uint8_t  unk_e[20];   // @14048
	                      // @14068
} __attribute__((packed));

struct mitsu98xx_tables {
	struct mitsu98xx_data superfine;
	struct mitsu98xx_data fine_std;
	struct mitsu98xx_data fine_hg;
} __attribute__((packed));

/* Command header */
struct mitsu9550_cmd {
	uint8_t cmd[4];
} __attribute__((packed));

/* Private data structure */
struct mitsu9550_ctx {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	int type;
	int is_s;
	int is_98xx;

	uint8_t *databuf;
	uint32_t datalen;

	uint16_t rows;
	uint16_t cols;
	uint32_t plane_len;

	uint16_t last_donor;
	uint16_t last_remain;
	int marker_reported;

	/* Parse headers separately */
	struct mitsu9550_hdr1 hdr1;
	int hdr1_present;
	struct mitsu9550_hdr2 hdr2;
	int hdr2_present;
	struct mitsu9550_hdr3 hdr3;
	int hdr3_present;
	struct mitsu9550_hdr4 hdr4;
	int hdr4_present;

	/* CP98xx stuff */
	struct mitsu98xx_tables *m98xxdata;
};

/* Printer data structures */
struct mitsu9550_media {
	uint8_t  hdr[2];  /* 24 2e */
	uint8_t  unk[12];
	uint8_t  type;
	uint8_t  unka[13];
	uint16_t max;  /* BE, prints per media */
	uint8_t  unkb[2];
	uint16_t remain; /* BE, prints remaining */
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

struct mitsu9550_status2 {
	uint8_t  hdr[2]; /* 21 2e */
	uint8_t  unk[39];
	uint16_t remain; /* BE, media remaining */
	uint8_t  unkb[4]; /* 0a 00 00 01 */
} __attribute__((packed));

#define CMDBUF_LEN   64
#define READBACK_LEN 128

#define QUERY_STATUS()	\
	do {\
		struct mitsu9550_status *sts = (struct mitsu9550_status*) rdbuf;\
		/* struct mitsu9550_status2 *sts2 = (struct mitsu9550_status2*) rdbuf; */ \
		struct mitsu9550_media *media = (struct mitsu9550_media *) rdbuf; \
		uint16_t donor, remain;	\
		/* media */ \
		ret = mitsu9550_get_status(ctx, rdbuf, 0, 0, 1); \
		if (ret < 0) \
			return CUPS_BACKEND_FAILED; \
		\
		/* Tell CUPS about the consumables we report */ \
		if (!ctx->marker_reported) { \
			ctx->marker_reported = 1; \
			ATTR("marker-colors=#00FFFF#FF00FF#FFFF00\n");	\
			ATTR("marker-high-levels=100\n"); \
			ATTR("marker-low-levels=10\n");	\
			ATTR("marker-names='%s'\n", mitsu9550_media_types(media->type, ctx->is_s)); \
			ATTR("marker-types=ribbonWax\n"); \
		} \
		\
		/* Sanity-check media response */ \
		if (media->remain == 0 || media->max == 0) { \
			ERROR("Printer out of media!\n"); \
			ATTR("marker-levels=%d\n", 0); \
			return CUPS_BACKEND_HOLD; \
		} \
		remain = be16_to_cpu(media->remain); \
		donor = be16_to_cpu(media->max); \
		donor = remain/donor; \
		if (donor != ctx->last_donor) { \
			ctx->last_donor = donor; \
			ATTR("marker-levels=%u\n", donor); \
		} \
		if (remain != ctx->last_remain) { \
			ctx->last_remain = remain; \
			ATTR("marker-message=\"%u prints remaining on '%s' ribbon\"\n", remain, mitsu9550_media_types(media->type, ctx->is_s)); \
		} \
		if (validate_media(ctx->type, media->type, ctx->cols, ctx->rows)) { \
			ERROR("Incorrect media (%u) type for printjob (%ux%u)!\n", media->type, ctx->cols, ctx->rows); \
			return CUPS_BACKEND_HOLD; \
		} \
		/* status2 */ \
		ret = mitsu9550_get_status(ctx, rdbuf, 0, 1, 0); \
		if (ret < 0) \
			return CUPS_BACKEND_FAILED; \
		/* status */ \
		ret = mitsu9550_get_status(ctx, rdbuf, 1, 0, 0); \
		if (ret < 0) \
			return CUPS_BACKEND_FAILED; \
		\
		/* Make sure we're idle */ \
		if (sts->sts5 != 0) {  /* Printer ready for another job */ \
			sleep(1); \
			goto top; \
		} \
		/* Check for known errors */ \
		if (sts->sts2 != 0) {  \
			ERROR("Printer cover open!\n");	\
			return CUPS_BACKEND_STOP; \
		} \
	} while (0);

static void mitsu98xx_dogamma(uint8_t *src, uint16_t *dest, uint8_t plane,
			      uint16_t *table, uint32_t len)
{
	src += plane;
	while(len--) {
		*dest++ = table[*src];
		src += 3;
	}
}

static int mitsu98xx_fillmatte(struct mitsu9550_ctx *ctx)
{
	int fd, i;
	uint32_t j, remain;

	DEBUG("Reading %d bytes of matte data from disk (%d/%d)\n", ctx->cols * ctx->rows, ctx->cols, LAMINATE_STRIDE);
	fd = open(MITSU_M98xx_LAMINATE_FILE, O_RDONLY);
	if (fd < 0) {
		WARNING("Unable to open matte lamination data file '%s'\n", MITSU_M98xx_LAMINATE_FILE);
		ctx->hdr1.matte = 0;
		goto done;
	}

	/* Fill in the lamination plane header */
	struct mitsu9550_plane *matte = (struct mitsu9550_plane *)(ctx->databuf + ctx->datalen);
	matte->cmd[0] = 0x1b;
	matte->cmd[1] = 0x5a;
	matte->cmd[2] = 0x54;
	matte->cmd[3] = 0x10;
	matte->row_offset = 0;
	matte->col_offset = 0;
	matte->cols = ctx->hdr1.cols;
	matte->rows = ctx->hdr1.rows;
	ctx->datalen += sizeof(struct mitsu9550_plane);

	/* Read in the matte data plane */
	for (j = 0 ; j < ctx->rows ; j++) {
		remain = LAMINATE_STRIDE * 2;

		/* Read one row of lamination data at a time */
		while (remain) {
			i = read(fd, ctx->databuf + ctx->datalen, remain);
			if (i < 0)
				return CUPS_BACKEND_CANCEL;
			if (i == 0) {
				/* We hit EOF, restart from beginning */
				lseek(fd, 0, SEEK_SET);
				continue;
			}
			ctx->datalen += i;
			remain -= i;
		}
		/* Back off the buffer so we "wrap" on the print row. */
		ctx->datalen -= ((LAMINATE_STRIDE - ctx->cols) * 2);
	}
	/* We're done! */
	close(fd);

	/* Fill in the lamination plane footer */
	ctx->databuf[ctx->datalen++] = 0x1b;
	ctx->databuf[ctx->datalen++] = 0x50;
	ctx->databuf[ctx->datalen++] = 0x56;
	ctx->databuf[ctx->datalen++] = 0x00;

done:
	return CUPS_BACKEND_OK;
}

/*** 3D color Lookup table stuff.  Taken out of lib70x ****/
#define LUT_LEN 14739
#define COLORCONV_RGB 0
#define COLORCONV_BGR 1

struct CColorConv3D {
	uint8_t lut[17][17][17][3];
};

/* Load the Lookup table off of disk into *PRE-ALLOCATED* buffer */
int CColorConv3D_Get3DColorTable(uint8_t *buf, const char *filename)
{
	FILE *stream;

	if (!filename)
		return 1;
	if (!*filename)
		return 2;
	if (!buf)
		return 3;

	stream = fopen(filename, "rb");
	if (!stream)
		return 4;

	fseek(stream, 0, SEEK_END);
	if (ftell(stream) < LUT_LEN) {
		fclose(stream);
		return 5;
	}
	fseek(stream, 0, SEEK_SET);
	fread(buf, 1, LUT_LEN, stream);
	fclose(stream);

	return 0;
}

/* Parse the on-disk LUT data into the structure.... */
struct CColorConv3D *CColorConv3D_Load3DColorTable(const uint8_t *ptr)
{
	struct CColorConv3D *this;
	this = malloc(sizeof(*this));
	if (!this)
		return NULL;

	int i, j, k;

	for (i = 0 ; i <= 16 ; i++) {
		for (j = 0 ; j <= 16 ; j++) {
			for (k = 0; k <= 16; k++) {
				this->lut[k][j][i][2] = *ptr++;
				this->lut[k][j][i][1] = *ptr++;
				this->lut[k][j][i][0] = *ptr++;
			}
		}
	}
	return this;
}
void CColorConv3D_Destroy3DColorTable(struct CColorConv3D *this)
{
	free(this);
}

/* Transform a single pixel. */
static void CColorConv3D_DoColorConvPixel(struct CColorConv3D *this, uint8_t *redp, uint8_t *grnp, uint8_t *blup)
{
	int red_h;
	int grn_h;
	int blu_h;
	int grn_li;
	int red_li;
	int blu_li;
	int red_l;
	int grn_l;
	int blu_l;

	uint8_t *tab0;       // @ 14743
	uint8_t *tab1;       // @ 14746
	uint8_t *tab2;       // @ 14749
	uint8_t *tab3;       // @ 14752
	uint8_t *tab4;       // @ 14755
	uint8_t *tab5;       // @ 14758
	uint8_t *tab6;       // @ 14761
	uint8_t *tab7;       // @ 14764

	red_h = *redp >> 4;
	red_l = *redp & 0xF;
	red_li = 16 - red_l;

	grn_h = *grnp >> 4;
	grn_l = *grnp & 0xF;
	grn_li = 16 - grn_l;

	blu_h = *blup >> 4;
	blu_l = *blup & 0xF;
	blu_li = 16 - blu_l;

//	printf("%d %d %d =>", *redp, *grnp, *blup);

	tab0 = this->lut[red_h+0][grn_h+0][blu_h+0];
	tab1 = this->lut[red_h+1][grn_h+0][blu_h+0];
	tab2 = this->lut[red_h+0][grn_h+1][blu_h+0];
	tab3 = this->lut[red_h+1][grn_h+1][blu_h+0];
	tab4 = this->lut[red_h+0][grn_h+0][blu_h+1];
	tab5 = this->lut[red_h+1][grn_h+0][blu_h+1];
	tab6 = this->lut[red_h+0][grn_h+1][blu_h+1];
	tab7 = this->lut[red_h+1][grn_h+1][blu_h+1];

#if 0
	printf(" %d %d %d ", tab0[0], tab0[1], tab0[2]);
	printf(" %d %d %d ", tab1[0], tab1[1], tab1[2]);
	printf(" %d %d %d ", tab2[0], tab2[1], tab2[2]);
	printf(" %d %d %d ", tab3[0], tab3[1], tab3[2]);
	printf(" %d %d %d ", tab4[0], tab4[1], tab4[2]);
	printf(" %d %d %d ", tab5[0], tab5[1], tab5[2]);
	printf(" %d %d %d ", tab6[0], tab6[1], tab6[2]);
	printf(" %d %d %d ", tab7[0], tab7[1], tab7[2]);
#endif
	*redp = (blu_li
		 * (grn_li * (red_li * tab0[0] + red_l * tab1[0])
		    + grn_l * (red_li * tab2[0] + red_l * tab3[0]))
		 + blu_l
		 * (grn_li * (red_li * tab4[0] + red_l * tab5[0])
		    + grn_l * (red_li * tab6[0] + red_l * tab7[0]))
		 + 2048) >> 12;
	*grnp = (blu_li
		 * (grn_li * (red_li * tab0[1] + red_l * tab1[1])
		    + grn_l * (red_li * tab2[1] + red_l * tab3[1]))
		 + blu_l
		 * (grn_li * (red_li * tab4[1] + red_l * tab5[1])
		    + grn_l * (red_li * tab6[1] + red_l * tab7[1]))
		 + 2048) >> 12;
	*blup = (blu_li
		 * (grn_li * (red_li * tab0[2] + red_l * tab1[2])
		    + grn_l * (red_li * tab2[2] + red_l * tab3[2]))
		 + blu_l
		 * (grn_li * (red_li * tab4[2] + red_l * tab5[2])
		    + grn_l * (red_li * tab6[2] + red_l * tab7[2]))
		 + 2048) >> 12;

//	printf("=> %d %d %d\n", *redp, *grnp, *blup);
}

/* Perform a total conversion on an entire image */
void CColorConv3D_DoColorConv(struct CColorConv3D *this, uint8_t *data, uint16_t cols, uint16_t rows, uint32_t stride, int rgb_bgr)
{
	uint16_t i, j;

	uint8_t *ptr;

	for ( i = 0; i < rows ; i++ )
	{
		ptr = data;
		for ( j = 0; cols > j; j++ )
		{
			if (rgb_bgr) {
				CColorConv3D_DoColorConvPixel(this, ptr + 2, ptr + 1, ptr);
			} else {
				CColorConv3D_DoColorConvPixel(this, ptr, ptr + 1, ptr + 2);
			}
			ptr += 3;
		}
		data += stride;
	}
}
/* ---- end 3D LUT ---- */

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

static void mitsu9550_attach(void *vctx, struct libusb_device_handle *dev,
			    uint8_t endp_up, uint8_t endp_down, uint8_t jobid)
{
	struct mitsu9550_ctx *ctx = vctx;
	struct libusb_device *device;
	struct libusb_device_descriptor desc;

	UNUSED(jobid);

	ctx->dev = dev;
	ctx->endp_up = endp_up;
	ctx->endp_down = endp_down;

	device = libusb_get_device(dev);
	libusb_get_device_descriptor(device, &desc);

	ctx->type = lookup_printer_type(&mitsu9550_backend,
					desc.idVendor, desc.idProduct);

	if (ctx->type == P_MITSU_9550S ||
	    ctx->type == P_MITSU_9800S)
		ctx->is_s = 1;

	if (ctx->type == P_MITSU_9800 ||
	    ctx->type == P_MITSU_9800S ||
	    ctx->type == P_MITSU_9810)
		ctx->is_98xx = 1;

	ctx->last_donor = ctx->last_remain = 65535;
}

static void mitsu9550_teardown(void *vctx) {
	struct mitsu9550_ctx *ctx = vctx;

	if (!ctx)
		return;

	if (ctx->databuf)
		free(ctx->databuf);
	if (ctx->m98xxdata)
		free(ctx->m98xxdata);
	free(ctx);
}

static int mitsu9550_read_parse(void *vctx, int data_fd) {
	struct mitsu9550_ctx *ctx = vctx;
	uint8_t buf[sizeof(struct mitsu9550_hdr1)];
	int remain, i;
	uint32_t planelen = 0;
	int is_raw = 1;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	if (ctx->databuf) {
		free(ctx->databuf);
		ctx->databuf = NULL;
	}

	ctx->hdr1_present = 0;
	ctx->hdr2_present = 0;
	ctx->hdr3_present = 0;
	ctx->hdr4_present = 0;

top:
	/* Read in initial header */
	remain = sizeof(buf);
	while (remain > 0) {
		i = read(data_fd, buf + sizeof(buf) - remain, remain);
		if (i == 0)
			return CUPS_BACKEND_CANCEL;
		if (i < 0)
			return CUPS_BACKEND_CANCEL;
		remain -= i;
	}

	/* Sanity check */
	if (buf[0] != 0x1b || buf[1] != 0x57 || buf[3] != 0x2e) {
		if (!ctx->hdr1_present || !ctx->hdr2_present) {
			ERROR("Unrecognized data format!\n");
			return CUPS_BACKEND_CANCEL;
		} else if (buf[0] == 0x1b && buf[1] == 0x5a &&
			   buf[2] == 0x54) {

			/* We're in the data portion now */
			if (buf[3] == 0x10)
				planelen *= 2;
			else if (ctx->is_98xx && buf[3] == 0x80)
				is_raw = 0;

			goto hdr_done;
		} else {
			ERROR("Unrecognized data block!\n");
			return CUPS_BACKEND_CANCEL;
		}
	}

	switch(buf[2]) {
	case 0x20: /* header 1 */
		memcpy(&ctx->hdr1, buf, sizeof(ctx->hdr1));
		ctx->hdr1_present = 1;

		/* Work out printjob size */
		ctx->rows = be16_to_cpu(ctx->hdr1.rows);
		ctx->cols = be16_to_cpu(ctx->hdr1.cols);
		planelen = ctx->rows * ctx->cols;

		break;
	case 0x21: /* header 2 */
		memcpy(&ctx->hdr2, buf, sizeof(ctx->hdr2));
		ctx->hdr2_present = 1;
		break;
	case 0x22: /* header 3 */
		memcpy(&ctx->hdr3, buf, sizeof(ctx->hdr3));
		ctx->hdr3_present = 1;
		break;
	case 0x26: /* header 4 */
		memcpy(&ctx->hdr4, buf, sizeof(ctx->hdr4));
		ctx->hdr4_present = 1;
		break;
	default:
		ERROR("Unrecognized header format (%02x)!\n", buf[2]);
		return CUPS_BACKEND_CANCEL;
	}

	/* Read in the next chunk */
	goto top;

hdr_done:

	if (is_raw) {
		/* We have three planes and the final terminator to read */
		remain = 3 * (planelen + sizeof(struct mitsu9550_plane)) + sizeof(struct mitsu9550_cmd);
	} else {
		/* We have one planes and the final terminator to read */
		remain = planelen * 3 + sizeof(struct mitsu9550_plane) + sizeof(struct mitsu9550_cmd);
	}

	/* Mitsu9600 windows spool uses more, smaller blocks, but plane data is the same */
	if (ctx->type == P_MITSU_9600) {
		remain += 128 * sizeof(struct mitsu9550_plane); /* 39 extra seen on 4x6" */
	}

	/* 9550S/9800S doesn't typically sent over hdr4! */
	if (ctx->type == P_MITSU_9550S ||
	    ctx->type == P_MITSU_9800S) {
		/* XXX Has to do with error policy, but not sure what.
		   Mitsu9550-S/9800-S will set this based on a command,
		   but it's not part of the standard job spool */
		ctx->hdr4_present = 0;
	}

	/* Read in CP98xx data tables if necessary */
	if (ctx->is_98xx && !is_raw && !ctx->m98xxdata) {
		int fd;

		DEBUG("Reading in 98xx data from disk\n");
		fd = open(MITSU_M98xx_DATATABLE_FILE, O_RDONLY);
		if (fd < 0) {
			ERROR("Unable to open 98xx data table file '%s'\n", MITSU_M98xx_DATATABLE_FILE);
			return CUPS_BACKEND_FAILED;
		}
		ctx->m98xxdata = malloc(DATATABLE_SIZE);
		if (!ctx->m98xxdata) {
			ERROR("Memory allocation Failure!\n");
			return CUPS_BACKEND_RETRY_CURRENT;
		}
		remain = DATATABLE_SIZE;
		while (remain) {
			i = read(fd, ((uint8_t*)&ctx->m98xxdata) + (DATATABLE_SIZE - remain), remain);
			if (i < 0)
				return CUPS_BACKEND_CANCEL;
			remain -= i;
		}
		close(fd);
	}

	/* Disable matte if the printer doesn't support it */
	if (ctx->hdr1.matte) {
		if (ctx->type != P_MITSU_9810) {
			WARNING("Matte not supported on this printer, disabling\n");
			ctx->hdr1.matte = 0;
		} else if (is_raw) {
			remain += planelen + sizeof(struct mitsu9550_plane) + sizeof(struct mitsu9550_cmd);
		}
	}

	/* Allocate buffer for the payload */
	ctx->datalen = 0;
	ctx->databuf = malloc(remain);
	if (!ctx->databuf) {
		ERROR("Memory allocation failure!\n");
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
			ERROR("Unexpected data read, aborting job\n");
			return CUPS_BACKEND_CANCEL;
		}

		/* Work out the length of this block */
		planelen = be16_to_cpu(plane->rows) * be16_to_cpu(plane->cols);
		if (plane->cmd[3] == 0x10)
			planelen *= 2;

		/* Copy plane header into buffer */
		memcpy(ctx->databuf + ctx->datalen, buf, sizeof(buf));
		ctx->datalen += sizeof(buf);
		planelen -= sizeof(buf) - sizeof(struct mitsu9550_plane);

		/* Read in the spool data */
		while(planelen > 0) {
			i = read(data_fd, ctx->databuf + ctx->datalen, planelen);
			if (i == 0)
				return CUPS_BACKEND_CANCEL;
			if (i < 0)
				return CUPS_BACKEND_CANCEL;
			ctx->datalen += i;
			planelen -= i;
		}

		/* Try to read in the next chunk.  It will be one of:
		    - Additional block header (12B)
		    - Job footer (4B)
		*/
		i = read(data_fd, buf, 4);
		if (i == 0)
			return CUPS_BACKEND_CANCEL;
		if (i < 0)
			return CUPS_BACKEND_CANCEL;

		/* Is this a "job end" marker? */
		if (plane->cmd[0] != 0x1b ||
		    plane->cmd[1] != 0x5a ||
		    plane->cmd[2] != 0x54) {
			/* store it in the buffer */
			memcpy(ctx->databuf + ctx->datalen, buf, 4);
			ctx->datalen += 4;

			/* Unless we have a matte plane following, we're done */
			if (ctx->hdr1.matte != 0x01)
				break;
			planelen = sizeof(buf);
		} else {
			/* It's part of a block header, mark what we've read */
			planelen = sizeof(buf) - 4;
		}

		/* Read in the rest of the header */
		while (planelen > 0) {
			i = read(data_fd, buf + sizeof(buf) - planelen, planelen);
			if (i == 0)
				return CUPS_BACKEND_CANCEL;
			if (i < 0)
				return CUPS_BACKEND_CANCEL;
			planelen -= i;
		}
	}

	/* Do the 98xx processing here */
	if (ctx->is_98xx && !is_raw) {
		uint8_t *newbuf;
		uint32_t newlen = 0;
		struct mitsu98xx_data *table;
		struct CColorConv3D *lut;

		/* Apply LUT */
		if (ctx->hdr2.unkc[9]) {
			uint8_t *buf = malloc(LUT_LEN);
			if (!buf) {
				ERROR("Memory allocation failure!\n");
				return CUPS_BACKEND_RETRY_CURRENT;
			}
			if (CColorConv3D_Get3DColorTable(buf, MITSU_M98xx_LUT_FILE)) {
				ERROR("Unable to open LUT file '%s'\n", MITSU_M98xx_LUT_FILE);
				return CUPS_BACKEND_CANCEL;
			}
			lut = CColorConv3D_Load3DColorTable(buf);
			free(buf);
			if (!lut) {
				ERROR("Unable to parse LUT\n");
				return CUPS_BACKEND_CANCEL;
			}
			CColorConv3D_DoColorConv(lut, ctx->databuf + sizeof(struct mitsu9550_plane),
						 ctx->cols, ctx->rows, ctx->cols * 3, COLORCONV_BGR);
			CColorConv3D_Destroy3DColorTable(lut);			
			ctx->hdr2.unkc[9] = 0;
		}

		planelen *= 2;
		remain = 4 * (planelen + sizeof(struct mitsu9550_plane)) + sizeof(struct mitsu9550_cmd);
		newbuf = malloc(remain);
		if (!newbuf) {
			ERROR("Memory allocation Failure!\n");
			return CUPS_BACKEND_RETRY_CURRENT;
		}
		switch (ctx->hdr2.mode) {
		case 0x80:
			table = &ctx->m98xxdata->superfine;
			break;
		case 0x11:
			table = &ctx->m98xxdata->fine_hg;
			ctx->hdr2.mode = 0x10;
			break;
		case 0x10:
		default:
			table = &ctx->m98xxdata->fine_std;
			break;
		}

		/* For B/Y plane */
		memcpy(newbuf + newlen, ctx->databuf, sizeof(struct mitsu9550_plane));
		newbuf[newlen + 3] = 0x10;  /* ie 16bpp data */
		newlen += sizeof(struct mitsu9550_plane);
		mitsu98xx_dogamma(ctx->databuf + sizeof(struct mitsu9550_plane),
				  (uint16_t*) (newbuf + newlen),
				  0,
				  table->GNMby,
				  planelen / 2);
		newlen += planelen;

		/* For G/M plane */
		memcpy(newbuf + newlen, ctx->databuf, sizeof(struct mitsu9550_plane));
		newbuf[newlen + 3] = 0x10;  /* ie 16bpp data */
		newlen += sizeof(struct mitsu9550_plane);
		mitsu98xx_dogamma(ctx->databuf + sizeof(struct mitsu9550_plane),
				  (uint16_t*) (newbuf + newlen),
				  1,
				  table->GNMgm,
				  planelen / 2);
		newlen += planelen;

		/* For R/C plane */
		memcpy(newbuf + newlen, ctx->databuf, sizeof(struct mitsu9550_plane));
		newbuf[newlen + 3] = 0x10;  /* ie 16bpp data */
		newlen += sizeof(struct mitsu9550_plane);
		mitsu98xx_dogamma(ctx->databuf + sizeof(struct mitsu9550_plane),
				  (uint16_t*) (newbuf + newlen),
				  2,
				  table->GNMrc,
				  planelen / 2);
		newlen += planelen;

		/* And finally, the job footer. */
		memcpy(newbuf + newlen, ctx->databuf + sizeof(struct mitsu9550_plane) + planelen * 3, sizeof(struct mitsu9550_cmd));
		newlen += sizeof(struct mitsu9550_cmd);

		/* Clean up */
		free(ctx->databuf);
		ctx->databuf = newbuf;
		ctx->datalen = newlen;

		/* Now handle the matte plane generation */
		if (ctx->hdr1.matte) {
			if ((i = mitsu98xx_fillmatte(ctx)))
				return i;
		}
	}

	return CUPS_BACKEND_OK;
}

static int mitsu9550_get_status(struct mitsu9550_ctx *ctx, uint8_t *resp, int status, int status2, int media)
{
	struct mitsu9550_cmd cmd;
	int num, ret;

	/* Send Printer Query */
	cmd.cmd[0] = 0x1b;
	cmd.cmd[1] = 0x56;
	if (status)
		cmd.cmd[2] = 0x30;
	else if (status2)
		cmd.cmd[2] = 0x21;
	else if (media)
		cmd.cmd[2] = 0x24;
	cmd.cmd[3] = 0x00;
	if ((ret = send_data(ctx->dev, ctx->endp_down,
			     (uint8_t*) &cmd, sizeof(cmd))))
		return ret;
	ret = read_data(ctx->dev, ctx->endp_up,
			resp, sizeof(struct mitsu9550_status), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(struct mitsu9550_status)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(struct mitsu9550_status));
		return 4;
	}

	return 0;
}

static char *mitsu9550_media_types(uint8_t type, uint8_t is_s)
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
	case P_MITSU_9810: // XXX and don't forget the 9820S
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
	default:
		WARNING("Unknown printer type %d\n", type);
		break;
	}
	return 0;
}

static int mitsu9550_main_loop(void *vctx, int copies) {
	struct mitsu9550_ctx *ctx = vctx;
	struct mitsu9550_cmd cmd;
	uint8_t rdbuf[READBACK_LEN];
	uint8_t *ptr;

	int ret;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	/* Update printjob header to reflect number of requested copies */
	ctx->hdr2.copies = cpu_to_be16(copies);

	/* Okay, let's do this thing */
	ptr = ctx->databuf;

top:
	if (ctx->is_s) {
		int num;

		/* Send "unknown 1" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x53;
		cmd.cmd[2] = 0xc5;
		cmd.cmd[3] = 0x9d;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;

		/* Send "unknown 2" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x4b;
		cmd.cmd[2] = 0x7f;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;

		ret = read_data(ctx->dev, ctx->endp_up,
				rdbuf, READBACK_LEN, &num);
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		// seen so far: eb 4b 7f 00  02 00 5e
	}

	if (ctx->type == P_MITSU_9800S) {
		int num;

		/* Send "unknown 3" command */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x4b;
		cmd.cmd[2] = 0x01;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;

		ret = read_data(ctx->dev, ctx->endp_up,
				rdbuf, READBACK_LEN, &num);
		if (ret < 0)
			return CUPS_BACKEND_FAILED;
		// seen so far: e4 4b 01 00 02 00 78
	}

	QUERY_STATUS();

	/* Now it's time for the actual print job! */

#if 0
	if (ctx->is_s) {
		/* This is a job cancel..? */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x44;
		cmd.cmd[2] = 0;
		cmd.cmd[3] = 0;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, 4)))
			return CUPS_BACKEND_FAILED;
	}
#endif

	QUERY_STATUS();

	/* Send printjob headers from spool data */
	if (ctx->hdr1_present)
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &ctx->hdr1, sizeof(ctx->hdr1))))
			return CUPS_BACKEND_FAILED;
	if (ctx->hdr2_present)
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &ctx->hdr2, sizeof(ctx->hdr2))))
			return CUPS_BACKEND_FAILED;
	if (ctx->hdr3_present)
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &ctx->hdr3, sizeof(ctx->hdr3))))
			return CUPS_BACKEND_FAILED;
	if (ctx->hdr4_present)
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &ctx->hdr4, sizeof(struct mitsu9550_hdr4))))
			return CUPS_BACKEND_FAILED;

	if (ctx->is_s) {
		/* I think this a "clear memory' command...? */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x5a;
		cmd.cmd[2] = 0x43;
		cmd.cmd[3] = 0x00;

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	}

	/* Send over plane data */
	while(1) {
		struct mitsu9550_plane *plane = (struct mitsu9550_plane *)ptr;
		uint32_t planelen = be16_to_cpu(plane->rows) * be16_to_cpu(plane->cols);
		if (plane->cmd[0] != 0x1b ||
		    plane->cmd[1] != 0x5a ||
		    plane->cmd[2] != 0x54)
			break;
		if (plane->cmd[3] == 0x10)
			planelen *= 2;

		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) ptr, sizeof(struct mitsu9550_plane))))
			return CUPS_BACKEND_FAILED;
		ptr += sizeof(struct mitsu9550_plane);
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) ptr, planelen)))
			return CUPS_BACKEND_FAILED;
		ptr += planelen;
	}

	/* Query statuses */
	{
		struct mitsu9550_status *sts = (struct mitsu9550_status*) rdbuf;
//		struct mitsu9550_status2 *sts2 = (struct mitsu9550_status2*) rdbuf;
		struct mitsu9550_media *media = (struct mitsu9550_media *) rdbuf;
		uint16_t donor, remain;

		ret = mitsu9550_get_status(ctx, rdbuf, 0, 0, 1); // media
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Sanity-check media response */
		if (media->remain == 0 || media->max == 0) {
			ERROR("Printer out of media!\n");
			ATTR("marker-levels=%d\n", 0);
			return CUPS_BACKEND_HOLD;
		}
		remain = be16_to_cpu(media->remain);
		donor = be16_to_cpu(media->max);
		donor = remain/donor;
		if (donor != ctx->last_donor) {
			ctx->last_donor = donor;
			ATTR("marker-levels=%u\n", donor);
		}
		if (remain != ctx->last_remain) {
			ctx->last_remain = remain;
			ATTR("marker-message=\"%u prints remaining on '%s' ribbon\"\n", remain, mitsu9550_media_types(media->type, ctx->is_s));
		}
		ret = mitsu9550_get_status(ctx, rdbuf, 0, 1, 0); // status2
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		ret = mitsu9550_get_status(ctx, rdbuf, 1, 0, 0); // status
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Make sure we're ready to proceed */
		if (sts->sts5 != 0) {
			ERROR("Unexpected response (sts5 %02x)\n", sts->sts5);
			return CUPS_BACKEND_FAILED;
		}
		if (!(sts->sts3 & 0xc0)) {
			ERROR("Unexpected response (sts3 %02x)\n", sts->sts3);
			return CUPS_BACKEND_FAILED;
		}
		/* Check for known errors */
		if (sts->sts2 != 0) {
			ERROR("Printer cover open!\n");
			return CUPS_BACKEND_STOP;
		}
	}

	/* Send "end data" command */
	if (ctx->type == P_MITSU_9550S) {
		/* Override spool, which may be wrong */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x50;
		cmd.cmd[2] = 0x47;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	} else if (ctx->type == P_MITSU_9800S) {
		/* Override spool, which may be wrong */
		cmd.cmd[0] = 0x1b;
		cmd.cmd[1] = 0x50;
		cmd.cmd[2] = 0x4e;
		cmd.cmd[3] = 0x00;
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) &cmd, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
	} else {
		/* Send from spool file */
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ptr, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
		ptr += sizeof(cmd);
	}

	/* Don't forget the 9810's matte plane */
	if (ctx->hdr1.matte) {
		struct mitsu9550_plane *plane = (struct mitsu9550_plane *)ptr;
		uint32_t planelen = be16_to_cpu(plane->rows) * be16_to_cpu(plane->cols);

		if (plane->cmd[3] == 0x10)
			planelen *= 2;

		// XXX include a status loop here too?
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) ptr, sizeof(struct mitsu9550_plane))))
			return CUPS_BACKEND_FAILED;
		ptr += sizeof(struct mitsu9550_plane);
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     (uint8_t*) ptr, planelen)))
			return CUPS_BACKEND_FAILED;
		ptr += planelen;

		/* Send "lamination end data" command from spool file */
		if ((ret = send_data(ctx->dev, ctx->endp_down,
				     ptr, sizeof(cmd))))
			return CUPS_BACKEND_FAILED;
		ptr += sizeof(cmd);
	}

	/* Status loop, run until printer reports completion */
	while(1) {
		struct mitsu9550_status *sts = (struct mitsu9550_status*) rdbuf;
//		struct mitsu9550_status2 *sts2 = (struct mitsu9550_status2*) rdbuf;
		struct mitsu9550_media *media = (struct mitsu9550_media *) rdbuf;
		uint16_t donor, remain;

		ret = mitsu9550_get_status(ctx, rdbuf, 0, 0, 1); // media
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		/* Sanity-check media response */
		if (media->remain == 0 || media->max == 0) {
			ERROR("Printer out of media!\n");
			ATTR("marker-levels=%d\n", 0);
			return CUPS_BACKEND_HOLD;
		}
		remain = be16_to_cpu(media->remain);
		donor = be16_to_cpu(media->max);
		donor = remain/donor;
		if (donor != ctx->last_donor) {
			ctx->last_donor = donor;
			ATTR("marker-levels=%u\n", donor);
		}
		if (remain != ctx->last_remain) {
			ctx->last_remain = remain;
			ATTR("marker-message=\"%u prints remaining on '%s' ribbon\"\n", remain, mitsu9550_media_types(media->type, ctx->is_s));
		}
		ret = mitsu9550_get_status(ctx, rdbuf, 0, 1, 0); // status2
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		ret = mitsu9550_get_status(ctx, rdbuf, 1, 0, 0); // status
		if (ret < 0)
			return CUPS_BACKEND_FAILED;

		INFO("%03d copies remaining\n", be16_to_cpu(sts->copies));

		if (!sts->sts1) /* If printer transitions to idle */
			break;

		if (fast_return && !be16_to_cpu(sts->copies)) { /* No remaining prints */
                        INFO("Fast return mode enabled.\n");
			break;
                }

		if (fast_return && !sts->sts5) { /* Ready for another job */
			INFO("Fast return mode enabled.\n");
			break;
		}
		/* Check for known errors */
		if (sts->sts2 != 0) {
			ERROR("Printer cover open!\n");
			return CUPS_BACKEND_STOP;
		}
		sleep(1);
	}

	INFO("Print complete\n");

	return CUPS_BACKEND_OK;
}

static void mitsu9550_dump_media(struct mitsu9550_media *resp, int is_s)
{
	INFO("Media type       : %02x (%s)\n",
	     resp->type, mitsu9550_media_types(resp->type, is_s));
	INFO("Media remaining  : %03d/%03d\n",
	     be16_to_cpu(resp->remain), be16_to_cpu(resp->max));
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

static void mitsu9550_dump_status2(struct mitsu9550_status2 *resp)
{
	INFO("Prints remaining on media : %03d\n",
	     be16_to_cpu(resp->remain));
}

static int mitsu9550_query_media(struct mitsu9550_ctx *ctx)
{
	struct mitsu9550_media resp;
	int ret;

	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, 0, 0, 1);

	if (!ret)
		mitsu9550_dump_media(&resp, ctx->is_s);

	return ret;
}

static int mitsu9550_query_status(struct mitsu9550_ctx *ctx)
{
	struct mitsu9550_status resp;
	int ret;

	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, 1, 0, 0);

	if (!ret)
		mitsu9550_dump_status(&resp);

	return ret;
}

static int mitsu9550_query_status2(struct mitsu9550_ctx *ctx)
{
	struct mitsu9550_status2 resp;
	int ret;

	ret = mitsu9550_get_status(ctx, (uint8_t*) &resp, 0, 1, 0);

	if (!ret)
		mitsu9550_dump_status2(&resp);

	return ret;
}

static int mitsu9550_query_serno(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len)
{
	struct mitsu9550_cmd cmd;
	uint8_t rdbuf[READBACK_LEN];
	uint8_t *ptr;
	int ret, num, i;

	cmd.cmd[0] = 0x1b;
	cmd.cmd[1] = 0x72;
	cmd.cmd[2] = 0x6e;
	cmd.cmd[3] = 0x00;

	if ((ret = send_data(dev, endp_down,
                             (uint8_t*) &cmd, sizeof(cmd))))
                return (ret < 0) ? ret : CUPS_BACKEND_FAILED;

	ret = read_data(dev, endp_up,
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
	ret = send_data(ctx->dev, ctx->endp_down, buf, sizeof(buf));

	return ret;
}

static void mitsu9550_cmdline(void)
{
	DEBUG("\t\t[ -m ]           # Query media\n");
	DEBUG("\t\t[ -s ]           # Query status\n");
	DEBUG("\t\t[ -X ]           # Cancel current job\n");
}

static int mitsu9550_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct mitsu9550_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "msX")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'm':
			j = mitsu9550_query_media(ctx);
			break;
		case 's':
			j = mitsu9550_query_status(ctx);
			if (!j)
				j = mitsu9550_query_status2(ctx);
			break;
		case 'X':
			j = mitsu9550_cancel_job(ctx);
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return 0;
}

static const char *mitsu9550_prefixes[] = {
	"mitsu9xxx",
	"mitsu9000", "mitsu9500", "mitsu9550", "mitsi9600", "mitsu9800", "mitsu9810",
	NULL
};

/* Exported */
struct dyesub_backend mitsu9550_backend = {
	.name = "Mitsubishi CP9xxx family",
	.version = "0.33",
	.uri_prefixes = mitsu9550_prefixes,
	.cmdline_usage = mitsu9550_cmdline,
	.cmdline_arg = mitsu9550_cmdline_arg,
	.init = mitsu9550_init,
	.attach = mitsu9550_attach,
	.teardown = mitsu9550_teardown,
	.read_parse = mitsu9550_read_parse,
	.main_loop = mitsu9550_main_loop,
	.query_serno = mitsu9550_query_serno,
	.devices = {
		{ USB_VID_MITSU, USB_PID_MITSU_9000AM, P_MITSU_9550, NULL, "mitsu9000"},
		{ USB_VID_MITSU, USB_PID_MITSU_9000D, P_MITSU_9550, NULL, "mitsu9000"},
		{ USB_VID_MITSU, USB_PID_MITSU_9500D, P_MITSU_9550, NULL, "mitsu9500"},
		{ USB_VID_MITSU, USB_PID_MITSU_9550D, P_MITSU_9550, NULL, "mitsu9550"},
		{ USB_VID_MITSU, USB_PID_MITSU_9550DS, P_MITSU_9550S, NULL, "mitsu9550"},
		{ USB_VID_MITSU, USB_PID_MITSU_9600D, P_MITSU_9600, NULL, "mitsu9600"},
//	{ USB_VID_MITSU, USB_PID_MITSU_9600D, P_MITSU_9600S, NULL, "mitsu9600"},
		{ USB_VID_MITSU, USB_PID_MITSU_9800D, P_MITSU_9800, NULL, "mitsu9800"},
		{ USB_VID_MITSU, USB_PID_MITSU_9800DS, P_MITSU_9800S, NULL, "mitsu9800"},
		{ USB_VID_MITSU, USB_PID_MITSU_98__D, P_MITSU_9810, NULL, "mitsu9810"},
//	{ USB_VID_MITSU, USB_PID_MITSU_9810D, P_MITSU_9810, NULL, "mitsu9810"},
//	{ USB_VID_MITSU, USB_PID_MITSU_9820DS, P_MITSU_9820S, NULL, "mitsu9820"},  // XXX add "mitsu9820"
		{ 0, 0, 0, NULL, NULL}
	}
};

/* Mitsubish CP-9500/9550/9600/9800/9810/9820 spool format:

   Spool file consists of 3 (or 4) 50-byte headers, followed by three
   image planes, each with a 12-byte header, then a 4-byte footer.

   All multi-byte numbers are big endian.

   ~~~ Header 1

   1b 57 20 2e 00 QQ QQ 00  00 00 00 00 00 00 XX XX :: XX XX == columns
   YY YY 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: YY YY == rows
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: QQ == 0x0a90 on 9810, 0x0a10 on all others.
   00 00

   ~~~ Header 2

   1b 57 21 2e 00 80 00 22  QQ QQ 00 00 00 00 00 00 :: ZZ ZZ = num copies (>= 0x01)
   00 00 00 00 00 00 00 00  00 00 00 00 ZZ ZZ 00 00 :: YY = 00/80 Fine/SuperFine (9550), 10/80 Fine/Superfine (98x0), 00 (9600)
   XX 00 00 00 00 00 YY 00  00 00 00 00 00 00 00 00 :: XX = 00 normal, 83 Cut 2x6 (9550 only!)
   RR 01                                            :: QQ QQ = 0x0803 on 9550, 0x0801 on 98x0, 0x0003 on 9600, 0xa803 on 9500
                                                    :: RR = 01 for "use LUT" on 98xx, 0x00 otherwise.  Extension to stock.

   ~~~ Header 3 (9550 and 9800-S only..)

   1b 57 22 2e 00 QQ 00 00  00 00 00 XX 00 00 00 00 :: XX = 00 normal, 01 FineDeep
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: QQ = 0xf0 on 9500, 0x40 on the rest
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
   00 00

   ~~~ Header 4 (all but 9550-S and 9800-S, involves error policy?)

   1b 57 26 2e 00 QQ 00 00  00 00 00 SS RR 01 00 00 :: QQ = 0x70 on 9550/98x0, 0x60 on 9600 or 9800S
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: RR = 0x01 on 9550/98x0, 0x00 on 9600
   00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: SS = 0x01 on 9800S, 0x00 otherwise.
   00 00

  ~~~~ Data follows:

   Format is:  planar YMC16 for 98x0 (but only 12 bits used, BIG endian)
               planar RGB for all others

   1b 5a 54 ?? RR RR  CC CC 07 14 04 d8  :: 0714 == columns, 04d8 == rows
                                         :: RRRR == row offset for data, CCCC == col offset for data
		                         :: ?? == 0x00 for 8bpp, 0x10 for 16/12bpp.
					 ::    0x80 for PACKED BGR!

   Data follows immediately, no padding.

   1b 5a 54 00 00 00 00 00  07 14 04 d8  :: Another plane.

   Data follows immediately, no padding.

   1b 5a 54 00 00 00 00 00  07 14 04 d8  :: Another plane.

   Data follows immediately, no padding.

  ~~~~ Footer:

   1b 50 57 00  (9500)
   1b 50 46 00  (9550)
   1b 50 47 00  (9550-S)
   1b 50 48 00  (9600)
   1b 50 4c 00  (9800/9810)
   1b 50 4e 00  (9800-S)

   Unknown: 9600-S, 9820-S

  ~~~~ Lamination data follows (on 9810 only, if matte selected)

   1b 5a 54 10 00 00  00 00 06 24 04 34

   Data follows immediately, no padding.

   1b 50 56 00  (Lamination footer)

  ~~~~ QUESTIONS:

   * Lamination control?
   * Other 9550 multi-cut modes (on 6x9 media: 4x6*2, 4.4x6*2, 3x6*3, 2x6*4)
   * 9600/98x0 multi-cut modes?

 ***********************************************************************

 * Mitsubishi ** CP-9550DW-S/9800DW-S ** Communications Protocol:

  [[ Unknown ]]

 -> 1b 53 c5 9d

  [[ Unknown, query some parameter? ]]

 -> 1b 4b 7f 00
 <- eb 4b 8f 00 02 00 5e  [[ '02' seems to be a length ]]

  [[ Unknown ]]

 -> 1b 53 00 00

  Query Model & Serial number

 -> 1b 72 6e 00
 <- e4 82 6e 00 LL 39 00 35  00 35 00 30 00 5a 00 20
    00 41 00 32 00 30 00 30  00 36 00 37 00

     'LL' is length.  Data is returned in 16-bit unicode, LE.
     Contents are model ('9550Z'), then space, then serialnum ('A20067')

  Media Query

 -> 1b 56 24 00
 <- 24 2e 00 00 00 00 00 00  00 00 00 00 00 00 TT 00 :: TT = Type
    00 00 00 00 00 00 00 00  00 00 00 00 MM MM 00 00 :: MM MM = Max prints
    NN NN 00 00 00 00 00 00  00 00 00 00 00 00 00 00 :: NN NN = Remaining

  [[ unknown query, 9800-only ]]

 -> 1b 4b 01 00
 <- e4 4b 01 00 02 00 78

  Status Query

 -> 1b 56 30 00
 -> 30 2e 00 00 00 00 MM 00  NN NN ZZ 00 00 00 00 00 :: MM, NN, ZZ
    QQ RR SS 00 00 00 00 00  00 00 00 00 00 00 00 00 :: QQ, RR, SS
    00 00 00 00 00 00 00 00  00 00 00 00 TT UU 00 00 :: TT, UU

  Status Query B (not sure what to call this)

 -> 1b 56 21 00
 <- 21 2e 00 80 00 22 a8 0b  00 00 00 00 00 00 00 00
    00 00 00 00 00 00 00 00  00 00 00 QQ 00 00 00 00 :: QQ == Prints in job?
    00 00 00 00 00 00 00 00  00 00 NN NN 0A 00 00 01 :: NN NN = Remaining media

  [[ Job Cancel ]]

 -> 1b 44

  [[ Header 1 -- See above ]]

 -> 1b 57 20 2e ....

  [[ Header 2 -- See above ]]

 -> 1b 57 21 2e ....

  [[ Header 3 -- See above ]]

 -> 1b 57 22 2e ....

  [[ Unknown -- Start Data ? ]]

 -> 1b 5a 43 00

  [[ Plane header #1 (Blue) ]]

 -> 1b 5a 54 00 00 00 00 00  XX XX YY YY :: XX XX == Columns, YY YY == Rows

    Followed by image plane #1 (Blue), XXXX * YYYY bytes

  [[ Plane header #2 (Green) ]]

 -> 1b 5a 54 00 00 00 00 00  XX XX YY YY :: XX XX == Columns, YY YY == Rows

    Followed by image plane #2 (Green), XXXX * YYYY bytes

  [[ Plane header #3 (Red) ]]

 -> 1b 5a 54 00 00 00 00 00  XX XX YY YY :: XX XX == Columns, YY YY == Rows

    Followed by image plane #3 (Red), XXXX * YYYY bytes

  [[ Footer -- End Data aka START print?  See above for other models ]]

 -> 1b 50 47 00  [9550S]
 -> 1b 50 4e 00  [9800S]

  [[ At this point, loop status/status b/media queries until printer idle ]]

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

  Seen on 9600DW

    ZZ == 08  Door open

 Working theory of interpreting the status flags:

  MM :: 00 is idle, else mechanical printer state.
  NN :: Remaining prints in job, or 0x00 0x00 when idle.
  QQ :: ?? 0x3e + 0x40 or 0x80 (see below)
  RR :: ?? 0x00 is idle, 0x40 or 0x80 is "printing"?
  SS :: ?? 0x00 means "ready for another print" but 0x01 is "busy"
  TT :: ?? seen values between 0x7c through 0x96)
  UU :: ?? seen values between 0x43 and 0x4c -- temperature?

  ***

   Other printer commands seen:

  [[ Set error policy ?? aka "header 4" ]]

 -> 1b 57 26 2e 00 QQ 00 00  00 00 00 00 RR SS 00 00 :: QQ/RR/SS 00 00 00 [9550S]
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ::          20 01 00 [9550S w/ ignore failures on]
    00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 ::          70 01 01 [9550]
    00 00

 */
