/*
 *   Mitsubishi CP-D70/D707 Photo Printer CUPS backend -- libusb-1.0 version
 *
 *   (c) 2013-2021 Solomon Peachy <pizza@shaftnet.org>
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

#define BACKEND mitsu70x_backend

#include "backend_common.h"
#include "backend_mitsu.h"

/* Width of the laminate data file */
#define LAMINATE_STRIDE 1864

/* Max size of data chunk sent over */
#define CHUNK_LEN (256*1024)

/* Private data structure */
struct mitsu70x_printjob {
	struct dyesub_job_common common;

	uint8_t *databuf;
	uint32_t datalen;

	uint8_t *spoolbuf;
	int spoolbuflen;

	uint16_t rows;
	uint16_t cols;
	uint32_t planelen;
	uint32_t matte;
	int raw_format;

	int decks_exact[2];	 /* Media is exact match */
	int decks_ok[2];         /* Media can be used */

	/* These are used only for the image processing */
	int sharpen; /* ie mhdr.sharpen - 1 */
	int reverse;

	const char *laminatefname;
	const char *lutfname;
	const char *cpcfname;
	const char *ecpcfname;
};

struct mitsu70x_ctx {
	struct dyesub_connection *conn;

	int is_s;

	uint16_t jobid;

	struct marker marker[2];
	uint8_t medias[2];
	uint8_t media_subtypes[2];

	uint16_t last_l;
	uint16_t last_u;
	int num_decks;

	char serno[7]; /* 6+null */
	char fwver[7]; /* 6+null */

	struct mitsu_lib lib;

	const char *last_cpcfname;
	const char *last_ecpcfname;

	struct BandImage output;
};

/* Printer data structures */
struct mitsu70x_jobstatus {
	uint8_t  hdr[4]; /* E4 56 31 30 */
	uint16_t jobid;  /* BE */
	uint16_t mecha_no; /* BE */
	uint8_t  job_status[4];
	uint8_t  memory;
	uint8_t  power;
	uint8_t  mecha_status[2];
	uint8_t  temperature;
	uint8_t  error_status[3];
	uint8_t  mecha_status_up[2];
	uint8_t  temperature_up;
	uint8_t  error_status_up[3];
} __attribute__((packed));

struct mitsu70x_job {
	uint16_t id; /* BE */
	uint8_t status[4];
} __attribute__((packed));

#define NUM_JOBS 170

struct mitsu70x_jobs {
	uint8_t  hdr[4]; /* E4 56 31 31 */
	struct mitsu70x_job jobs[NUM_JOBS];
} __attribute__((packed));

#define MECHA_STATUS_INIT   0x80
#define MECHA_STATUS_FEED   0x50
#define MECHA_STATUS_LOAD   0x40
#define MECHA_STATUS_LOAD2  0x30
#define MECHA_STATUS_PRINT  0x20
#define MECHA_STATUS_IDLE   0x00

#define JOB_STATUS0_NONE    0x00
#define JOB_STATUS0_DATA    0x10
#define JOB_STATUS0_QUEUE   0x20
#define JOB_STATUS0_PRINT   0x50
#define JOB_STATUS0_ASSIGN  0x70 // XXX undefined.
#define JOB_STATUS0_END     0x80

#define JOB_STATUS1_PRINT_MEDIALOAD  0x10
#define JOB_STATUS1_PRINT_PRE_Y      0x20
#define JOB_STATUS1_PRINT_Y          0x30
#define JOB_STATUS1_PRINT_PRE_M      0x40
#define JOB_STATUS1_PRINT_M          0x50
#define JOB_STATUS1_PRINT_PRE_C      0x60
#define JOB_STATUS1_PRINT_C          0x70
#define JOB_STATUS1_PRINT_PRE_OC     0x80
#define JOB_STATUS1_PRINT_OC         0x90
#define JOB_STATUS1_PRINT_EJECT      0xA0

#define JOB_STATUS1_END_OK           0x00
#define JOB_STATUS1_END_MECHA        0x10 // 0x10...0x7f
#define JOB_STATUS1_END_HEADER       0x80
#define JOB_STATUS1_END_PRINT        0x90
#define JOB_STATUS1_END_INTERRUPT    0xA0

#define JOB_STATUS2_END_HEADER_ERROR 0x00
#define JOB_STATUS2_END_HEADER_MEMORY 0x10
#define JOB_STATUS2_END_PRINT_MEDIA   0x00
#define JOB_STATUS2_END_PRINT_PREVERR 0x10
#define JOB_STATUS2_END_INT_TIMEOUT  0x00
#define JOB_STATUS2_END_INT_CANCEL   0x10
#define JOB_STATUS2_END_INT_DISCON   0x20

/* Error codes */
#define ERROR_STATUS0_NOSTRIPBIN     0x01
#define ERROR_STATUS0_NORIBBON       0x02
#define ERROR_STATUS0_NOPAPER        0x03
#define ERROR_STATUS0_MEDIAMISMATCH  0x04
#define ERROR_STATUS0_RIBBONCNTEND   0x05
#define ERROR_STATUS0_BADRIBBON      0x06
#define ERROR_STATUS0_BADJOBPARAM    0x07
#define ERROR_STATUS0_PAPEREND       0x08
#define ERROR_STATUS0_RIBBONEND      0x09
#define ERROR_STATUS0_DOOROPEN_IDLE  0x0A
#define ERROR_STATUS0_DOOROPEN_PRNT  0x0B
#define ERROR_STATUS0_POWEROFF       0x0C // Powered off during printing..?
#define ERROR_STATUS0_NOMCOP         0x0D
#define ERROR_STATUS0_RIBBONSKIP1    0x0E
#define ERROR_STATUS0_RIBBONSKIP2    0x0F
#define ERROR_STATUS0_RIBBONJAM      0x10
#define ERROR_STATUS0_RIBBON_OTHER   0x11 // 0x11->0x1F
#define ERROR_STATUS0_PAPER_JAM      0x20 // 0x20->0x2F
#define ERROR_STATUS0_MECHANICAL     0x30 // 0x30->0x39
#define ERROR_STATUS0_RFID           0x3A
#define ERROR_STATUS0_FLASH          0x3B
#define ERROR_STATUS0_EEPROM         0x3C
#define ERROR_STATUS0_PREHEAT        0x3D
#define ERROR_STATUS0_MDASTATE       0x3E
#define ERROR_STATUS0_PSUFANLOCKED   0x3F
#define ERROR_STATUS0_OTHERS         0x40 // 0x40..?

/* Error classifications */
#define ERROR_STATUS1_PAPER          0x01
#define ERROR_STATUS1_RIBBON         0x02
#define ERROR_STATUS1_SETTING        0x03
#define ERROR_STATUS1_OPEN           0x05
#define ERROR_STATUS1_NOSTRIPBIN     0x06
#define ERROR_STATUS1_PAPERJAM       0x07
#define ERROR_STATUS1_RIBBONSYS      0x08
#define ERROR_STATUS1_MECHANICAL     0x09
#define ERROR_STATUS1_ELECTRICAL     0x0A
#define ERROR_STATUS1_FIRMWARE       0x0E
#define ERROR_STATUS1_OTHER          0x0F

/* Error recovery conditions */
#define ERROR_STATUS2_AUTO           0x00
#define ERROR_STATUS2_RELOAD_PAPER   0x01
#define ERROR_STATUS2_RELOAD_RIBBON  0x02
#define ERROR_STATUS2_CHANGE_BOTH    0x03
#define ERROR_STATUS2_CHANGE_ONE     0x04
#define ERROR_STATUS2_CLOSEUNIT      0x05
#define ERROR_STATUS2_ATTACHSTRIPBIN 0x06
#define ERROR_STATUS2_CLEARJAM       0x07
#define ERROR_STATUS2_CHECKRIBBON    0x08
#define ERROR_STATUS2_OPENCLOSEUNIT  0x0A
#define ERROR_STATUS2_POWEROFF       0x0F

struct mitsu70x_status_deck {
	uint8_t  mecha_status[2];
	uint8_t  temperature;   /* D70/D80 family only, K60 no? */
	uint8_t  error_status[3];
	uint8_t  rsvd_a[3]; /* K60 [1] == temperature? */
	uint8_t  lifetime_prints[4];
	uint8_t  rsvd_b[3]; /* K60 [3] == ?? */
	uint8_t  media_brand;
	uint8_t  media_type;
	uint8_t  media_subtype;	 /* K60 only? */
	uint8_t  rsvd_c[1];
	int16_t  capacity; /* media capacity */
	int16_t  remain;   /* media remaining */
	uint8_t  rsvd_d[2];
	uint8_t  unknown_ctr[4]; /* lifetime + 10 (EK305), lifetime+41 (D80), in BCD! */
	uint8_t  rsvd_e[2]; // Unknown
	uint16_t rsvd_f[16]; /* all 80 00 */
} __attribute__((packed));

struct mitsu70x_status_ver {
	char     ver[6];
	uint16_t checksum; /* Presumably BE */
} __attribute__((packed));

struct mitsu70x_printerstatus_resp {
	uint8_t  hdr[4];  /* E4 56 32 30 */
	uint8_t  memory;
	uint8_t  power;
	uint8_t  unk[20];
	uint8_t  sleeptime; /* In minutes, 0-60 */
	uint8_t  iserial;   /* 0x00 for Enabled, 0x80 for Disabled */
	uint8_t  unk_b[5];  // [4] == 0x44 on D70x, 0x13 on D80, 0x02 on EK305.
	uint8_t  dual_deck; /* 0x80 for dual-deck D707, 0x00 otherwise */
	uint8_t  unk_c[2];  // always 00 00 ??
	uint8_t  subtype;   /* 0x5f on D70x/K60/D80, 0x5e on D70xS/K60-S, 0x01 on EK305 */
	uint8_t  unk_d[3];  // [1:2] == 0x??bd on D70x, 0x0487 on EK305, 0x04d7 on D80
	int16_t  model[6];  /* LE, UTF-16 */
	int16_t  serno[6];  /* LE, UTF-16 */
	struct mitsu70x_status_ver vers[7]; // components are 'MLRTF'
	uint8_t  null[2];
	uint8_t  user_serno[6];  /* XXX Supposedly. Don't know how to set it! */
	struct mitsu70x_status_deck lower;
	struct mitsu70x_status_deck upper;
} __attribute__((packed));

struct mitsu70x_memorystatus_resp {
	uint8_t  hdr[3]; /* E4 56 33 */
	uint8_t  memory;
	uint8_t  size;
	uint8_t  rsvd;
} __attribute__((packed));

struct mitsu70x_calinfo_resp {  /* Interpretations valid for ASK300 */
	uint8_t hdr[6]; /* e4 6a 36 34 31 00 */

	/* Note!  All values below are ASCII hex! ie 0x23 -> 0x32 0x33 */

	uint8_t adj_horiz[2];  /* +- 128, units of 0.085 mm */
	uint8_t adj_vertA[2];  /* +- 128 */
	uint8_t adj_vertB[2];  /*  values are in units of 0.085 mm */
	uint8_t adj_vertC[2];  /*  A is -1->9, B is -4->6, C is -1->9 */
	uint8_t adj_fine[4]; /* 00DC */
	uint8_t adj_m3[2]; /* -100 -> 100 (converted to hex) */
	uint8_t unk_c[28];
	//             30 30 30 30  46 46 36 34 35 35 30 30
	// 46 46 36 34 35 35 30 30  44 43 30 30 30 30 30 30

	uint8_t adj_density[4]; /* 6800 -> 9000, def 8000 */
	uint8_t adj_24v[4];     /* 0000 -> 00FF */
} __attribute__((packed));

/*
  NOTES:  Other stuff seen:

  1b 45 48 [30 31 32] <-- No resp [30 any deck, 31 is lower, 32 is upper?]
  1b 45 4a [30 31 32] <-- No resp [30 any deck, 31 is lower, 32 is upper?]
  1b 45 53 00 10 [ ...? ] XX XX . "set printer number"..
  1b 45 53 90 00 0a [ ... 9 bytes of something ] 10
  1b 52 XX 00    <-- XX = something + 0x51
  1b 54 00 [00 31 32]  <-- No resp [00 any, 31 lower, 32 upper???]
  1b 54 31 00   "feed and cut"
  1b 54 53 90 00 0a 00 00  00 00 00 00 00 00 00 00
  1b 56 34 [31 32] <-- 6 byte response, last two bytes are value.
  1b 5a 43 00 <-- No resp
  1b 67 18 ...   (??)
  1b 6a ...      Various test commands
  1b 6e ...      (??)
  1b 72 45 [31 32]
  1b 72 67 00 00 00

*/

struct mitsu70x_hdr {
	uint8_t  hdr[4]; /* 1b 5a 54 XX */  // XXX also, seen 1b 5a 43!
	uint16_t jobid;
	uint8_t  rewind[2];  /* K60/EK305/D80 only */
	uint8_t  zero0[8];

	uint16_t cols;
	uint16_t rows;
	uint16_t lamcols;
	uint16_t lamrows;
	uint8_t  speed;
	uint8_t  zero1[7];

	uint8_t  deck; /* 0 = default, 1 = lower, 2 = upper -- Non-D70/D707 is always '1' */
	uint8_t  zero2[7];
	uint8_t  laminate; /* 00 == on, 01 == off */
	uint8_t  laminate_mode; /* 00 == glossy, 02 == matte */
	uint8_t  zero3[6];

	uint8_t  multicut;
	uint8_t  zero4[12]; /* NOTE:  everything past this point is an extension */
	uint8_t  sharpen;  /* 0-9.  5 is "normal", 0 is "off" */
	uint8_t  mode;     /* 0 for cooked YMC planar, 1 for packed BGR */
	uint8_t  use_lut;  /* in BGR mode, 0 disables, 1 enables */
	uint8_t  reversed; /* 1 tells the backend the row data is correct */
	uint8_t  pad[447];
} __attribute__((packed));

STATIC_ASSERT(sizeof(struct mitsu70x_hdr) == 512);
STATIC_ASSERT(sizeof(struct mitsu70x_calinfo_resp) == 56);

static int mitsu70x_get_printerstatus(struct mitsu70x_ctx *ctx, struct mitsu70x_printerstatus_resp *resp);
static int mitsu70x_main_loop(void *vctx, const void *vjob, int wait_for_return);

/* Error dumps, etc */

static const char *mitsu70x_mechastatus(uint8_t *sts)
{
	switch(sts[0]) {
	case MECHA_STATUS_INIT:
		return "Initializing";
	case MECHA_STATUS_FEED:
		return "Paper Feeding/Cutting";
	case MECHA_STATUS_LOAD:
	case MECHA_STATUS_LOAD2:
		return "Media Loading";
	case MECHA_STATUS_PRINT:
		return "Printing";
	case MECHA_STATUS_IDLE:
		return "Idle";
	default:
		break;
	}
	return "Unknown Mechanical Status";
}

static const char *mitsu70x_jobstatuses(uint8_t *sts)
{
	switch(sts[0]) {
	case JOB_STATUS0_NONE:
		return "No Job";
	case JOB_STATUS0_DATA:
		return "Data transfer";
	case JOB_STATUS0_QUEUE:
		return "Queued for printing";
	case JOB_STATUS0_PRINT:
		switch(sts[1]) {
		case JOB_STATUS1_PRINT_MEDIALOAD:
			return "Media loading";
		case JOB_STATUS1_PRINT_PRE_Y:
			return "Waiting to print yellow plane";
		case JOB_STATUS1_PRINT_Y:
			return "Printing yellow plane";
		case JOB_STATUS1_PRINT_PRE_M:
			return "Waiting to print magenta plane";
		case JOB_STATUS1_PRINT_M:
			return "Printing magenta plane";
		case JOB_STATUS1_PRINT_PRE_C:
			return "Waiting to print cyan plane";
		case JOB_STATUS1_PRINT_C:
			return "Printing cyan plane";
		case JOB_STATUS1_PRINT_PRE_OC:
			return "Waiting to laminate page";
		case JOB_STATUS1_PRINT_OC:
			return "Laminating page";
		case JOB_STATUS1_PRINT_EJECT:
			return "Ejecting page";
		default:
			return "Unknown 'Print' status1";
		}
		break;
	case JOB_STATUS0_ASSIGN:
		return "Unknown 'Assignment' status1";
	case JOB_STATUS0_END:
		switch(sts[1]) {
		case JOB_STATUS1_END_OK:
			return "Normal End";
		case JOB_STATUS1_END_HEADER:
			switch(sts[2]) {
			case JOB_STATUS2_END_HEADER_ERROR:
				return "Incorrect Header data (bad print size?)";
			case JOB_STATUS2_END_HEADER_MEMORY:
				return "Insufficient printer memory";
			default:
				return "Unknown 'End Header' status2";
			}
			break;
		case JOB_STATUS1_END_PRINT:
			switch(sts[2]) {
			case JOB_STATUS2_END_PRINT_MEDIA:
				return "Incorrect mediasize";
			case JOB_STATUS2_END_PRINT_PREVERR:
				return "Previous job terminated abnormally";
			default:
				return "Unknown 'End Print' status2";
			}
			break;
		case JOB_STATUS1_END_INTERRUPT:
			switch(sts[2]) {
			case JOB_STATUS2_END_INT_TIMEOUT:
				return "Timeout";
			case JOB_STATUS2_END_INT_CANCEL:
				return "Job cancelled";
			case JOB_STATUS2_END_INT_DISCON:
				return "Printer disconnected";
			default:
				return "Unknown 'End Print' status2";
			}
			break;
		default:
			if (sts[1] >= 0x10 && sts[1] <= 0x7f)
				return "Mechanical Error";
			else
				return "Unknown 'End' status1";
		}
		break;
	default:
		break;
	}

	return "Unknown status0";
}

static const char *mitsu70x_errorclass(uint8_t *err)
{
	switch(err[1]) {
	case ERROR_STATUS1_PAPER:
		return "Paper";
	case ERROR_STATUS1_RIBBON:
		return "Ribbon";
	case ERROR_STATUS1_SETTING:
		return "Job settings";
	case ERROR_STATUS1_OPEN:
		return "Cover open";
	case ERROR_STATUS1_NOSTRIPBIN:
		return "No cut bin";
	case ERROR_STATUS1_PAPERJAM:
		return "Paper jam";
	case ERROR_STATUS1_RIBBONSYS:
		return "Ribbon system";
	case ERROR_STATUS1_MECHANICAL:
		return "Mechanical";
	case ERROR_STATUS1_ELECTRICAL:
		return "Electrical";
	case ERROR_STATUS1_FIRMWARE:
		return "Firmware";
	case ERROR_STATUS1_OTHER:
		return "Other";
	default:
		break;
	}
	return "Unknown error class";
}

static const char *mitsu70x_errorrecovery(uint8_t *err)
{
	switch(err[1]) {
	case ERROR_STATUS2_AUTO:
		return "Automatic recovery";
	case ERROR_STATUS2_RELOAD_PAPER:
		return "Reload or change paper";
	case ERROR_STATUS2_RELOAD_RIBBON:
		return "Reload or change ribbon";
	case ERROR_STATUS2_CHANGE_BOTH:
		return "Change paper and ribbon";
	case ERROR_STATUS2_CHANGE_ONE:
		return "Change paper or ribbon";
	case ERROR_STATUS2_CLOSEUNIT:
		return "Close printer";
	case ERROR_STATUS2_ATTACHSTRIPBIN:
		return "Attach Strip Bin";
	case ERROR_STATUS2_CLEARJAM:
		return "Remove and reload paper";
	case ERROR_STATUS2_CHECKRIBBON:
		return "Check ribbon and reload paper";
	case ERROR_STATUS2_OPENCLOSEUNIT:
		return "Open then close printer";
	case ERROR_STATUS2_POWEROFF:
		return "Power-cycle printer";
	default:
		break;
	}
	return "Unknown recovery";
}

static const char *mitsu70x_errors(uint8_t *err)
{
	switch(err[0]) {
	case ERROR_STATUS0_NOSTRIPBIN:
		return "Strip bin not attached";
	case ERROR_STATUS0_NORIBBON:
		return "No ribbon detected";
	case ERROR_STATUS0_NOPAPER:
		return "No paper loaded";
	case ERROR_STATUS0_MEDIAMISMATCH:
		return "Ribbon/Paper mismatch";
	case ERROR_STATUS0_RIBBONCNTEND:
		return "Ribbon count end";
	case ERROR_STATUS0_BADRIBBON:
		return "Illegal Ribbon";
	case ERROR_STATUS0_BADJOBPARAM:
		return "Job does not match loaded media";
	case ERROR_STATUS0_PAPEREND:
		return "End of paper detected";
	case ERROR_STATUS0_RIBBONEND:
		return "End of ribbon detected";
	case ERROR_STATUS0_DOOROPEN_IDLE:
	case ERROR_STATUS0_DOOROPEN_PRNT:
		return "Printer door open";
	case ERROR_STATUS0_POWEROFF:
		return "Printer powered off"; // nonsense..
	case ERROR_STATUS0_RIBBONSKIP1:
	case ERROR_STATUS0_RIBBONSKIP2:
		return "Ribbon skipped";
	case ERROR_STATUS0_RIBBONJAM:
		return "Ribbon stuck to paper";
	case ERROR_STATUS0_RFID:
		return "RFID read error";
	case ERROR_STATUS0_FLASH:
		return "FLASH read error";
	case ERROR_STATUS0_EEPROM:
		return "EEPROM read error";
	case ERROR_STATUS0_PREHEAT:
		return "Preheating unit time out";
	case ERROR_STATUS0_MDASTATE:
		return "Unknown MDA state";
	case ERROR_STATUS0_PSUFANLOCKED:
		return "Power supply fan locked up";
	default:
		break;
	}

	if (err[0] >= ERROR_STATUS0_RIBBON_OTHER &&
	    err[0] < ERROR_STATUS0_PAPER_JAM) {
		return "Unknown ribbon error";
		// XXX use err[1]/err[2] codes?
	}
	if (err[0] >= ERROR_STATUS0_PAPER_JAM &&
	    err[0] < ERROR_STATUS0_MECHANICAL) {
		return "Paper jam";
		// XXX use err[1]/err[2] codes?
	}
	if (err[0] >= ERROR_STATUS0_MECHANICAL &&
	    err[0] < ERROR_STATUS0_RFID) {
		return "Unknown mechanical error";
		// XXX use err[1]/err[2] codes?
	}

	return "Unknown error";
}


#define CMDBUF_LEN 512
#define READBACK_LEN 256

static void *mitsu70x_init(void)
{
	struct mitsu70x_ctx *ctx = malloc(sizeof(struct mitsu70x_ctx));
	if (!ctx) {
		ERROR("Memory Allocation Failure!\n");
		return NULL;
	}
	memset(ctx, 0, sizeof(struct mitsu70x_ctx));

	return ctx;
}

static int mitsu70x_attach(void *vctx, struct dyesub_connection *conn, uint8_t jobid)
{
	struct mitsu70x_ctx *ctx = vctx;

	ctx->jobid = jobid;
	if (!ctx->jobid)
		jobid++;

	ctx->conn = conn;

	ctx->last_l = ctx->last_u = 65535;

#if defined(WITH_DYNAMIC)
	/* Attempt to open the library */
	if (mitsu_loadlib(&ctx->lib, ctx->conn->type))
#endif
		WARNING("Dynamic library support not loaded, will be unable to print!\n");

	struct mitsu70x_printerstatus_resp resp;
	int ret;

	if (test_mode < TEST_MODE_NOATTACH) {
		ret = mitsu70x_get_printerstatus(ctx, &resp);
		if (ret) {
			ERROR("Unable to get printer status! (%d)\n", ret);
			return CUPS_BACKEND_FAILED;
		}
	} else {
		int media_code = 0xf;
		if (getenv("MEDIA_CODE"))
			media_code = atoi(getenv("MEDIA_CODE")) & 0xf;

		resp.upper.mecha_status[0] = MECHA_STATUS_INIT;
		resp.lower.mecha_status[0] = MECHA_STATUS_INIT;
		resp.upper.capacity = cpu_to_be16(230);
		resp.lower.capacity = cpu_to_be16(230);
		resp.upper.remain = cpu_to_be16(200);
		resp.lower.remain = cpu_to_be16(200);
		resp.upper.media_brand = 0xff;
		resp.lower.media_brand = 0xff;
		resp.upper.media_type = media_code;
		resp.lower.media_type = media_code;
		resp.dual_deck = 0x80;  /* Make it a dual deck */
		resp.vers[0].ver[0] = 0;
		resp.subtype = 0x5e;
	}

	/* Figure out if we're a D707 with two decks */
	if (ctx->conn->type == P_MITSU_D70X &&
	    resp.dual_deck == 0x80)
		ctx->num_decks = 2;
	else
		ctx->num_decks = 1;

	/* Set up markers */
	ctx->marker[0].color = "#00FFFF#FF00FF#FFFF00";
	ctx->marker[0].name = mitsu_media_types(ctx->conn->type, resp.lower.media_brand, resp.lower.media_type);
	ctx->marker[0].numtype = resp.lower.media_type;
	ctx->marker[0].levelmax = be16_to_cpu(resp.lower.capacity);
	ctx->marker[0].levelnow = be16_to_cpu(resp.lower.remain);
	ctx->medias[0] = resp.lower.media_type & 0xf;
	ctx->media_subtypes[0] = resp.lower.media_subtype;

	if (ctx->num_decks == 2) {
		ctx->marker[1].color = "#00FFFF#FF00FF#FFFF00";
		ctx->marker[1].name = mitsu_media_types(ctx->conn->type, resp.upper.media_brand, resp.upper.media_type);
		ctx->marker[1].numtype = resp.upper.media_type;
		ctx->marker[1].levelmax = be16_to_cpu(resp.upper.capacity);
		ctx->marker[1].levelnow = be16_to_cpu(resp.upper.remain);
		ctx->medias[1] = resp.upper.media_type & 0xf;
		ctx->media_subtypes[1] = resp.upper.media_subtype;
	}

	/* Store the FW version */
	memcpy(ctx->fwver, resp.vers[0].ver, 6);
	ctx->fwver[6] = 0;
	/* Store the serial number */
	for (int i = 0 ; i < 6 ; i++) {
		ctx->serno[i] = le16_to_cpu(resp.serno[i]) & 0x7f;
	}
	ctx->serno[6] = 0;

	/* Check for the -S variants */
	if (resp.subtype == 0x5e)
		ctx->is_s = 1;

	/* FW sanity checking */
	if (ctx->conn->type == P_KODAK_305) {
		/* Known versions:
		   v1.02: M 316E81 1433   (Add Ultrafine and matte support)
		   v1.04: M 316F83 2878   (Add 2x6 strip and support new "Triton" media)
		   v3.01: M 443A12 8908   (add 5" media support)
		*/
		if (strncmp(resp.vers[0].ver, "443A12", 6) < 0)
			WARNING("Printer FW out of date. Highly recommend upgrading EK305 to v3.01 or newer!\n");
	} else if (ctx->conn->type == P_MITSU_K60) {
		/* Known versions:
		   v1.05: M 316M31 148C   (Add HG media support)
		*/
		if (strncmp(resp.vers[0].ver, "316M31", 6) < 0)
			WARNING("Printer FW out of date. Highly recommend upgrading K60 to v1.05 or newer!\n");
	} else if (ctx->conn->type == P_MITSU_D70X) {
		/* Known versions for D70/D707:
		   v1.10: M 316V11 064D   (Add ultrafine mode, 6x6 support, 2x6 strip, and more?)
		   v1.12: M 316W11 9FC3   (??)
		   v1.13:                 (??)

		   Known versions for D70-S/D707-S
		   v???   M 316K11 E08A   (??)
		*/
		if (strncmp(resp.vers[0].ver, "316W11", 6) < 0)
			WARNING("Printer FW out of date. Highly recommend upgrading D70/D707 to v1.12 or newer!\n");
	} else if (ctx->conn->type == P_FUJI_ASK300) {
		/* Known versions:
		   v?.??: M 316A21 7998   (ancient. no matte or ultrafine)
		   v?.??: M 316H21 F8EB
		   v4.20a: M 316J21 4431  (Add 2x6 strip support)
		*/
		if (strncmp(resp.vers[0].ver, "316J21", 6) < 0)
			WARNING("Printer FW out of date. Highly recommend upgrading ASK300 to v4.20a or newer!\n");
	}

	return CUPS_BACKEND_OK;
}

static void mitsu70x_cleanup_job(const void *vjob) {
	const struct mitsu70x_printjob *job = vjob;

	if (job->databuf)
		free(job->databuf);
	if (job->spoolbuf)
		free(job->spoolbuf);

	free((void*)job);
}

static void mitsu70x_teardown(void *vctx) {
	struct mitsu70x_ctx *ctx = vctx;

	if (!ctx)
		return;

	mitsu_destroylib(&ctx->lib);

	free(ctx);
}

#define JOB_EQUIV(__x)  if (job1->__x != job2->__x) goto done

static void *mitsu70x_combine_jobs(const void *vjob1,
				   const void *vjob2)
{
	const struct mitsu70x_printjob *job1 = vjob1;
	const struct mitsu70x_printjob *job2 = vjob2;
	struct mitsu70x_printjob *newjob = NULL;
	uint16_t newrows;
	uint16_t newcols;
	uint32_t newpad, finalpad;
	uint16_t lamoffset;

	const struct mitsu70x_hdr *hdr1, *hdr2;
	struct mitsu70x_hdr *newhdr;

        /* Sanity check */
        if (!job1 || !job2)
                goto done;

	hdr1 = (struct mitsu70x_hdr *) job1->databuf;
	hdr2 = (struct mitsu70x_hdr *) job2->databuf;

	JOB_EQUIV(rows);
	JOB_EQUIV(cols);
	JOB_EQUIV(matte);
	JOB_EQUIV(sharpen);

	if (hdr1->multicut || hdr2->multicut) // XXX type 5 (2x6*2) -> type4 (2x6*4), 6x9 needed, 2628 rows. use '4' in multicut field.
		goto done;
	if (job1->raw_format || job2->raw_format)
		goto done;
	if (hdr1->speed != hdr2->speed)
		goto done;

	switch (job1->rows) {
	case 1218:  /* K60, EK305 */
		newrows = 2454;
		newpad = 16;
		finalpad = 0;
		lamoffset = 0;
		break;
	case 1228:  /* D70, ASK300, D80 */
		newrows = 2730;
		newpad = 38;
		finalpad = 236;
		lamoffset = 12;
		break;
	case 1076: /* EK305, K60 3.5x5" prints */
		newrows = 2190;
		newpad = 49;
		finalpad = 0;
		lamoffset = 0;
		break;
	default:
		goto done;
	}
	newcols = job1->cols;
	newpad *= newcols;
	finalpad *= newcols;

	/* Okay, it's kosher to proceed */

	DEBUG("Combining jobs to save media\n");

        newjob = malloc(sizeof(*newjob));
        if (!newjob) {
                ERROR("Memory allocation failure!\n");
                goto done;
        }
        memcpy(newjob, job1, sizeof(*newjob));

	newjob->spoolbuf = NULL;
	newjob->rows = newrows;
	newjob->cols = newcols;
	newjob->planelen = (((newrows * newcols * 2) + 511) /512) * 512;
	if (newjob->matte) {
		newjob->matte = ((((newrows + lamoffset) * newcols * 2) + 511) / 512) * 512;
	}
        newjob->databuf = malloc(sizeof(*newhdr) + newjob->planelen * 3 + newjob->matte);
        newjob->datalen = 0;
        if (!newjob->databuf) {
		mitsu70x_cleanup_job(newjob);
		newjob = NULL;
                ERROR("Memory allocation failure!\n");
                goto done;
        }
	newhdr = (struct mitsu70x_hdr *) newjob->databuf;

	/* Copy over header */
	memcpy(newhdr, hdr1, sizeof(*newhdr));
	newjob->datalen += sizeof(*newhdr);

	newhdr->rows = cpu_to_be16(newrows);
	newhdr->cols = cpu_to_be16(newcols);

	if (newjob->matte) {
		newhdr->lamrows = cpu_to_be16(newrows + lamoffset);
		newhdr->lamcols = cpu_to_be16(newcols);
	}
	newhdr->multicut = 1;
	newhdr->deck = 0;  /* Let printer decide */

	newjob->spoolbuf = malloc(newrows * newcols * 3);
	newjob->spoolbuflen = 0;
	if (!newjob->spoolbuf) {
		mitsu70x_cleanup_job(newjob);
		newjob = NULL;
                ERROR("Memory allocation failure!\n");
                goto done;
	}

	/* Fill in padding */
	memset(newjob->spoolbuf + newjob->spoolbuflen, 0xff, finalpad * 3);
	newjob->spoolbuflen += finalpad * 3;

	/* Copy image payload */
	memcpy(newjob->spoolbuf + newjob->spoolbuflen, job1->spoolbuf,
	       job1->spoolbuflen);
	newjob->spoolbuflen += job1->spoolbuflen;

	/* Fill in padding */
	memset(newjob->spoolbuf + newjob->spoolbuflen, 0xff, newpad * 3);
	newjob->spoolbuflen += newpad * 3;

	/* Copy image payload */
	memcpy(newjob->spoolbuf + newjob->spoolbuflen, job2->spoolbuf,
	       job2->spoolbuflen);
	newjob->spoolbuflen += job2->spoolbuflen;

	/* Okay, we're done. */

done:
	return newjob;
}
#undef JOB_EQUIV

static int mitsu70x_read_parse(void *vctx, const void **vjob, int data_fd, int copies) {
	struct mitsu70x_ctx *ctx = vctx;
	int i, remain;
	struct mitsu70x_hdr mhdr;

	struct mitsu70x_printjob *job = NULL;

	if (!ctx)
		return CUPS_BACKEND_FAILED;

	job = malloc(sizeof(*job));
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memset(job, 0, sizeof(*job));
	job->common.jobsize = sizeof(*job);
	job->common.copies = copies;

repeat:
	/* Read in initial header */
	remain = sizeof(mhdr);
	while (remain > 0) {
		i = read(data_fd, ((uint8_t*)&mhdr) + sizeof(mhdr) - remain, remain);
		if (i == 0) {
			mitsu70x_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		if (i < 0) {
			mitsu70x_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		remain -= i;
	}

	/* Skip over wakeup header if it's present. */
	if (mhdr.hdr[0] == 0x1b &&
	    mhdr.hdr[1] == 0x45 &&
	    mhdr.hdr[2] == 0x57 &&
	    mhdr.hdr[3] == 0x55) {
		goto repeat;
	}

	/* Sanity check header */
	if (mhdr.hdr[0] != 0x1b ||
	    mhdr.hdr[1] != 0x5a ||
	    mhdr.hdr[2] != 0x54) {
		ERROR("Unrecognized data format!\n");
		mitsu70x_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	job->raw_format = !mhdr.mode;

	/* Sanity check Matte mode */
	if (!mhdr.laminate && mhdr.laminate_mode) {
		if (ctx->conn->type != P_MITSU_D70X) {
			if (mhdr.speed != 0x03 && mhdr.speed != 0x04) {
				WARNING("Forcing Ultrafine mode for matte printing!\n");
				mhdr.speed = 0x04; /* Force UltraFine */
			}
		} else {
			if (mhdr.speed != 0x03) {
				mhdr.speed = 0x03; /* Force SuperFine */
				WARNING("Forcing SuperFine mode for matte printing!\n");
			}
		}
	}

	/* Figure out the correction data table to use */
	if (ctx->conn->type == P_MITSU_D70X) {
		job->laminatefname = "D70MAT01.raw";
		job->lutfname = "CPD70L01.lut";

		if (mhdr.speed == 3) {
			job->cpcfname = "CPD70S01.cpc";
		} else if (mhdr.speed == 4) {
			job->cpcfname = "CPD70U01.cpc";
		} else {
			job->cpcfname = "CPD70N01.cpc";
		}
		if (ctx->is_s && mhdr.hdr[3] != 0x00) {
			WARNING("Print job has wrong submodel specifier (%x)\n", mhdr.hdr[3]);
			mhdr.hdr[3] = 0x00;
		} else if (!ctx->is_s && mhdr.hdr[3] != 0x01) {
			WARNING("Print job has wrong submodel specifier (%x)\n", mhdr.hdr[3]);
			mhdr.hdr[3] = 0x01;
		}
	} else if (ctx->conn->type == P_MITSU_D80) {
		job->laminatefname = "D80MAT01.raw";
		job->lutfname = "CPD80L01.lut";

		if (mhdr.speed == 3) {
			job->cpcfname = "CPD80S01.cpc";
			job->ecpcfname = "CPD80E01.cpc"; /* For SuperFine in rewind mode, depending on image.. */
		} else if (mhdr.speed == 4) {
			job->cpcfname = "CPD80U01.cpc";
			job->ecpcfname = NULL;
		} else {
			job->cpcfname = "CPD80N01.cpc";
			job->ecpcfname = NULL;
		}
		// XXX Does is_s matter?
		if (mhdr.hdr[3] != 0x01) {
			WARNING("Print job has wrong submodel specifier (%x)\n", mhdr.hdr[3]);
			mhdr.hdr[3] = 0x01;
		}
	} else if (ctx->conn->type == P_MITSU_K60) {
		job->laminatefname = "S60MAT02.raw";
		job->lutfname = "CPS60L01.lut";

		if (mhdr.speed == 3 || mhdr.speed == 4) {
			mhdr.speed = 4; /* Ultra Fine */
			if (ctx->media_subtypes[0] == 0x10) /* HG media */
				job->cpcfname = "CPS60H03.cpc";
			else
				job->cpcfname = "CPS60T03.cpc";
		} else {
			if (ctx->media_subtypes[0] == 0x10) /* HG media */
				job->cpcfname = "CPS60H01.cpc";
			else
				job->cpcfname = "CPS60T01.cpc";
		}
		// XXX Does is_s matter?
		if (mhdr.hdr[3] != 0x00) {
			WARNING("Print job has wrong submodel specifier (%x)\n", mhdr.hdr[3]);
			mhdr.hdr[3] = 0x00;
		}
	} else if (ctx->conn->type == P_KODAK_305) {
		job->laminatefname = "EK305MAT.raw"; // Same as K60
		job->lutfname = "EK305L01.lut";

		if (mhdr.speed == 3 || mhdr.speed == 4) {
			mhdr.speed = 4; /* Ultra Fine */
			job->cpcfname = "EK305T03.cpc";
		} else {
			job->cpcfname = "EK305T01.cpc";
		}
		// XXX what about using K60 media if we read back the proper code?
		if (mhdr.hdr[3] != 0x90) {
			WARNING("Print job has wrong submodel specifier (%x)\n", mhdr.hdr[3]);
			mhdr.hdr[3] = 0x90;
		}
	} else if (ctx->conn->type == P_FUJI_ASK300) {
		job->laminatefname = "ASK300M2.raw"; /* Same as D70 */
		job->lutfname = NULL; /* Printer does not come with external LUT */
		if (mhdr.speed == 3 || mhdr.speed == 4) {
			mhdr.speed = 3; /* Super Fine */
			job->cpcfname = "ASK300T3.cpc";
		} else {
			job->cpcfname = "ASK300T1.cpc";
		}
		if (mhdr.hdr[3] != 0x80) {
			WARNING("Print job has wrong submodel specifier (%x)\n", mhdr.hdr[3]);
			mhdr.hdr[3] = 0x80;
		}
	}
	if (!mhdr.use_lut)
		job->lutfname = NULL;

	job->sharpen = mhdr.sharpen - 1;
	job->reverse = !mhdr.reversed;

	/* Clean up header back to pristine. */
	mhdr.use_lut = 0;
	mhdr.mode = 0;
	mhdr.sharpen = 0;
	mhdr.reversed = 0;

	/* Work out total printjob size */
	job->cols = be16_to_cpu(mhdr.cols);
	job->rows = be16_to_cpu(mhdr.rows);

	job->planelen = job->rows * job->cols * 2;
	job->planelen = (job->planelen + 511) / 512 * 512; /* Round to nearest 512 bytes. */

	if (!mhdr.laminate && mhdr.laminate_mode) {
		i = be16_to_cpu(mhdr.lamcols) * be16_to_cpu(mhdr.lamrows) * 2;
		i = (i + 511) / 512 * 512; /* Round to nearest 512 bytes. */
		job->matte = i;
	}

	remain = 3 * job->planelen + job->matte;

	job->datalen = 0;
	job->databuf = malloc(sizeof(mhdr) + remain + LAMINATE_STRIDE*2);  /* Give us a bit extra */

	if (!job->databuf) {
		ERROR("Memory allocation failure!\n");
		mitsu70x_cleanup_job(job);
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	memcpy(job->databuf + job->datalen, &mhdr, sizeof(mhdr));
	job->datalen += sizeof(mhdr);

	if (job->raw_format) { /* RAW MODE */
		DEBUG("Reading in %d bytes of 16bpp YMC%sdata\n", remain,
		      job->matte ? "L " : " ");

		/* Read in the spool data */
		while(remain) {
			i = read(data_fd, job->databuf + job->datalen, remain);
			if (i == 0) {
				mitsu70x_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			if (i < 0) {
				mitsu70x_cleanup_job(job);
				return CUPS_BACKEND_CANCEL;
			}
			job->datalen += i;
			remain -= i;
		}
		goto bypass_raw;
	}

	/* Non-RAW mode! */
	remain = job->rows * job->cols * 3;
	DEBUG("Reading in %d bytes of 8bpp BGR data\n", remain);

	job->spoolbuflen = 0;
	job->spoolbuf = malloc(remain);
	if (!job->spoolbuf) {
		ERROR("Memory allocation failure!\n");
		mitsu70x_cleanup_job(job);
		return CUPS_BACKEND_RETRY_CURRENT;
	}

	/* Read in the BGR data */
	while (remain) {
		i = read(data_fd, job->spoolbuf + job->spoolbuflen, remain);
		if (i == 0) {
			mitsu70x_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		if (i < 0) {
			mitsu70x_cleanup_job(job);
			return CUPS_BACKEND_CANCEL;
		}
		job->spoolbuflen += i;
		remain -= i;
	}

	if (!ctx->lib.dl_handle) {
		ERROR("!!! Image Processing Library not found, aborting!\n");
		mitsu70x_cleanup_job(job);
		return CUPS_BACKEND_CANCEL;
	}

	/* Run through basic LUT, if present and enabled */
	if (job->lutfname) {
		int ret = mitsu_apply3dlut_packed(&ctx->lib, job->lutfname,
						  job->spoolbuf, job->cols,
						  job->rows, job->cols * 3,
						  COLORCONV_BGR);
		if (ret) {
			mitsu70x_cleanup_job(job);
			return ret;
		}
	}

bypass_raw:
	for (i = 0 ; i < ctx->num_decks ; i++) {
		switch (ctx->medias[i]) {
		case 0x1: // 5x3.5
			if (job->rows == 1076)
				job->decks_ok[i] = 1;
			if (job->rows == 1076)
				job->decks_exact[i] = 1;
			break;
		case 0x2: // 4x6
			if (job->rows == 1218 ||
			    job->rows == 1228)
				job->decks_ok[i] = 1;
			if (job->rows == 1218 ||
			    job->rows == 1228)
				job->decks_exact[i] = 1;
			break;
		case 0x4: // 5x7
			if (job->rows == 1076 ||
			    job->rows == 1524 ||
			    job->rows == 2128)
				job->decks_ok[i] = 1;
			if (job->rows == 1524 ||
			    job->rows == 2128)
				job->decks_exact[i] = 1;
			break;
		case 0x5: // 6x9
		case 0xf: // 6x8
			/* This is made more complicated:
			   some 6x8" jobs are 6x9" sized.  Let printer
			   sort these out.  It's unlikely we'll have
			   6x8" in one deck and 6x9" in the other!
			*/
			if (job->rows == 1218 ||
			    job->rows == 1228 ||
			    job->rows == 1820 ||
			    job->rows == 2422 ||
			    job->rows == 2564 ||
			    job->rows == 2730)
				job->decks_ok[i] = 1;
			if (job->rows == 2422 ||
			    job->rows == 2564 ||
			    job->rows == 2730)
				job->decks_exact[i] = 1;
			break;
		default:
			job->decks_ok[i] = 0;
			job->decks_exact[i] = 0;
			break;
		}
	}

	/* 6x4 can be combined, only on 6x8/6x9" media. */
	job->common.can_combine = 0;
	if (job->decks_exact[0] ||
	    job->decks_exact[1]) {
		/* Exact media match, don't combine. */
	} else if (job->rows == 1218 ||
		   job->rows == 1228) {
		if (ctx->medias[0] == 0xf ||
		    ctx->medias[0] == 0x5 ||
		    ctx->medias[1] == 0xf || /* Two decks possible */
		    ctx->medias[1] == 0x5)
			job->common.can_combine = !job->raw_format;
	} else if (job->rows == 1076) {
		if (ctx->conn->type == P_KODAK_305 ||
		    ctx->conn->type == P_MITSU_K60) {
			if (ctx->medias[0] == 0x4)  /* Only one deck */
				job->common.can_combine = !job->raw_format;
		}
	}

	/* Return what we found */
	*vjob = job;

	return CUPS_BACKEND_OK;
}

static int mitsu70x_get_jobstatus(struct mitsu70x_ctx *ctx, struct mitsu70x_jobstatus *resp, uint16_t jobid)
{
	uint8_t cmdbuf[CMDBUF_LEN];
	int num, ret;

	/* Send Printer Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x56;
	cmdbuf[2] = 0x31;
	cmdbuf[3] = 0x30;  // XXX 30 == specific, 31 = "all"

	cmdbuf[4] = (jobid >> 8) & 0xff;
	cmdbuf[5] = jobid & 0xff;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, 6)))
		return ret;

	memset(resp, 0, sizeof(*resp));

	ret = read_data(ctx->conn,
			(uint8_t*) resp, sizeof(*resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(*resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*resp));
		return 4;
	}

	return CUPS_BACKEND_OK;
}

#if 0
static int mitsu70x_get_jobs(struct mitsu70x_ctx *ctx, struct mitsu70x_jobs *resp)
{
	uint8_t cmdbuf[CMDBUF_LEN];
	int num, ret;

	/* Send Printer Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x56;
	cmdbuf[2] = 0x31;
	cmdbuf[3] = 0x31;
	cmdbuf[4] = 0x00;
	cmdbuf[5] = 0x00;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, 6)))
		return ret;

	memset(resp, 0, sizeof(*resp));

	ret = read_data(ctx->conn,
			(uint8_t*) resp, sizeof(*resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(*resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*resp));
		return 4;
	}

	return CUPS_BACKEND_OK;
}
#endif

static int mitsu70x_get_memorystatus(struct mitsu70x_ctx *ctx, const struct mitsu70x_printjob *job, uint8_t mcut, struct mitsu70x_memorystatus_resp *resp)
{
	uint8_t cmdbuf[CMDBUF_LEN];

	uint16_t tmp;

	int num;
	int ret;

	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x56;
	cmdbuf[2] = 0x33;
	cmdbuf[3] = 0x00;
	tmp = cpu_to_be16(job->cols);
	memcpy(cmdbuf + 4, &tmp, 2);

	/* We have to lie about print sizes in 4x6*2 multicut modes */
	// XXX what about type4 (2x6*4) and type3 (3x6*3)
	tmp = job->rows;
	if (tmp == 2730 && mcut == 1) {
		if (ctx->conn->type == P_MITSU_D70X ||
		    ctx->conn->type == P_FUJI_ASK300) {
			tmp = 2422;
		}
	}

	tmp = cpu_to_be16(tmp);
	memcpy(cmdbuf + 6, &tmp, 2);
	cmdbuf[8] = job->matte ? 0x80 : 0x00;
	cmdbuf[9] = 0x00;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, 10)))
		return CUPS_BACKEND_FAILED;

	/* Read in the printer status */
	ret = read_data(ctx->conn,
			(uint8_t*) resp, sizeof(*resp), &num);
	if (ret < 0)
		return CUPS_BACKEND_FAILED;

	if (num != sizeof(*resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*resp));
		return CUPS_BACKEND_FAILED;
	}

	/* Make sure response is sane */
	if (resp->hdr[0] != 0xe4 ||
	    resp->hdr[1] != 0x56 ||
	    resp->hdr[2] != 0x33) {
		ERROR("Unknown response from printer\n");
		return CUPS_BACKEND_FAILED;
	}

	return CUPS_BACKEND_OK;
}

static int mitsu70x_get_printerstatus(struct mitsu70x_ctx *ctx, struct mitsu70x_printerstatus_resp *resp)
{
	uint8_t cmdbuf[CMDBUF_LEN];
	int num, ret;

	/* Send Printer Query */
	memset(cmdbuf, 0, CMDBUF_LEN);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x56;
	cmdbuf[2] = 0x32;
	cmdbuf[3] = 0x30; /* or x31 or x32, for SINGLE DECK lower/upper query!
			     Results will only have one deck. */
	if ((ret = send_data(ctx->conn,
			     cmdbuf, 4)))
		return ret;
	memset(resp, 0, sizeof(*resp));
	ret = read_data(ctx->conn,
			(uint8_t*) resp, sizeof(*resp), &num);

	if (ret < 0)
		return ret;
	if (num != sizeof(*resp)) {
		ERROR("Short Read! (%d/%d)\n", num, (int)sizeof(*resp));
		return 4;
	}

	return CUPS_BACKEND_OK;
}

static int mitsu70x_cancel_job(struct mitsu70x_ctx *ctx, uint16_t jobid)
{
	uint8_t cmdbuf[4];
	int ret;

	/* Send Job cancel.  No response. */
	memset(cmdbuf, 0, 4);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x44;
	cmdbuf[2] = (jobid >> 8) & 0xff;
	cmdbuf[3] = jobid & 0xff;
	if ((ret = send_data(ctx->conn,
			     cmdbuf, 4)))
		return ret;

	return CUPS_BACKEND_OK;
}

static int mitsu70x_test_print(struct mitsu70x_ctx *ctx, int type)
{
	uint8_t cmdbuf[14];
	int ret, num = 0;
	uint8_t resp[256];

	/* Send Test ON */
	memset(cmdbuf, 0, 8);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x76;
	cmdbuf[2] = 0x54;
	cmdbuf[3] = 0x45;
	cmdbuf[4] = 0x53;
	cmdbuf[5] = 0x54;
	cmdbuf[6] = 0x4f;
	cmdbuf[7] = 0x4e;
	if ((ret = send_data(ctx->conn,
			     cmdbuf, 8)))
		return ret;

	memset(resp, 0, sizeof(resp));

	ret = read_data(ctx->conn,
			resp, sizeof(resp), &num);  // always e4 44 4f 4e 45

	if (ret) return ret;

	/* Send Test print. */
	memset(cmdbuf, 0x30, 12);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x6a;
	cmdbuf[2] = 0x31;

	switch(type) {
	default:
	case 0: /* Test Print */
		cmdbuf[4] = 0x31;
		cmdbuf[11] = 0x31;
		break;
	case 1: /* Solid Black */
		cmdbuf[3] = 0x32;
		cmdbuf[4] = 0x31;
		cmdbuf[6] = 0x46;
		cmdbuf[7] = 0x46;
		cmdbuf[11] = 0x31;
		break;
	case 2: /* Solid Gray */
		cmdbuf[3] = 0x32;
		cmdbuf[4] = 0x31;
		cmdbuf[6] = 0x38;
		cmdbuf[11] = 0x31;
		break;
	case 3: /* Head Pattern */
		cmdbuf[3] = 0x31;
		cmdbuf[4] = 0x31;
		cmdbuf[11] = 0x31;
		break;
	case 4: /* Color Bar */
		cmdbuf[3] = 0x34;
		cmdbuf[4] = 0x31;
		cmdbuf[11] = 0x31;
		break;
	case 5: /* Vertical Alignment */
		cmdbuf[3] = 0x32;
		cmdbuf[4] = 0x31;
		cmdbuf[6] = 0x38;
		cmdbuf[11] = 0x32;
		break;
	case 6: /* Horizontal Alignment; Grey Cross */
		cmdbuf[3] = 0x32;
		cmdbuf[4] = 0x31;
		cmdbuf[6] = 0x38;
		cmdbuf[11] = 0x31;
		break;
	}
	if ((ret = send_data(ctx->conn,
			     cmdbuf, 12)))
		return ret;

	ret = read_data(ctx->conn,
			resp, sizeof(resp), &num); /* Get 5 back */

	return ret;
}

static int mitsu70x_test_dump(struct mitsu70x_ctx *ctx)
{
	uint8_t cmdbuf[14];
	int ret, num = 0;
	uint8_t resp[8192];

	/* Send Test ON */
	memset(cmdbuf, 0, 8);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x76;
	cmdbuf[2] = 0x54;
	cmdbuf[3] = 0x45;
	cmdbuf[4] = 0x53;
	cmdbuf[5] = 0x54;
	cmdbuf[6] = 0x4f;
	cmdbuf[7] = 0x4e;
	if ((ret = send_data(ctx->conn,
			     cmdbuf, 8)))
		return ret;

	memset(resp, 0, sizeof(resp));

	ret = read_data(ctx->conn,
			resp, sizeof(resp), &num);  // always e4 44 4f 4e 45

	if (ret) return ret;

	/* Get calibration parameters */
	memset(cmdbuf, 0, 6);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x6a;
	cmdbuf[2] = 0x36;
	cmdbuf[3] = 0x34;
	cmdbuf[4] = 0x31;
	cmdbuf[5] = 0x00;
	if ((ret = send_data(ctx->conn,
			     cmdbuf, 6)))
		return ret;
	ret = read_data(ctx->conn,
			resp, sizeof(resp), &num); // 56 back!

	if (ret) return ret;

	/* response is struct mitsu70x_calinfo_resp */
	{
		struct mitsu70x_calinfo_resp *calinfo = (struct mitsu70x_calinfo_resp*) resp;
		char buf[5];
		float f;

		memset(buf, 0x0, sizeof(buf));
		memcpy(buf, calinfo->adj_horiz, 2);
		f = strtol(buf, NULL, 16);
		if (f > 127) f -= 256;
		f *= 0.085; /* 300dpi = 0.085mm/pixel */
		INFO("Horizontal Calibration: %2.3f mm\n", f);
		memcpy(buf, calinfo->adj_vertA, 2);
		f = strtol(buf, NULL, 16);
		if (f > 127) f -= 256;
		f *= 0.085;
		INFO("Vertical Calibration A: %2.3f mm\n", f);
		memcpy(buf, calinfo->adj_vertB, 2);
		f = strtol(buf, NULL, 16);
		if (f > 127) f -= 256;
		f *= 0.085;
		INFO("Vertical Calibration B: %2.3f mm\n", f);
		memcpy(buf, calinfo->adj_vertC, 2);
		f = strtol(buf, NULL, 16);
		if (f > 127) f -= 256;
		f *= 0.085;
		INFO("Vertical Calibration C: %2.2f mm\n", f);
	}

	/* Get eeprom dump.. */
	memset(cmdbuf, 0, 14);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x6a;
	cmdbuf[2] = 0x36;
	cmdbuf[3] = 0x36;
	cmdbuf[4] = 0x31;
	cmdbuf[5] = 0x00;
	cmdbuf[6] = 0x31;  //
	cmdbuf[7] = 0x30;  //
	cmdbuf[8] = 0x30;  //
	cmdbuf[9] = 0x30;  // x1000 = LENGTH (4096, 1 -> 4096)
	cmdbuf[10] = 0x30; //
	cmdbuf[11] = 0x30; //
	cmdbuf[12] = 0x30; //
	cmdbuf[13] = 0x30; // x0000 = ADDRESS (0, x0-x7fff)
	if ((ret = send_data(ctx->conn,
			     cmdbuf, 14)))
		return ret;
	ret = read_data(ctx->conn,
			resp, sizeof(resp), &num); // 4110 back!

	/* To set calibration: 1b 6a 30 XX XX XX ?? ??

	   where ?? ?? is ASCII representation of hex value

	   Horiz = x70 31 31, range 0x00->0xff (unit is pixels or 0.085 mm, def 0)
	   VertA = x70 31 32, range -1 -> 9 (unit is pixels or 0.085mm, def 4)
	   VertB = x70 31 33, range -4 -> 6 (def 1)
	   VertC = x70 31 34, range -1 -> 9 (def 4)
           M1    = x71 31 31, range -128 -> +127 step 0.05v (NOT on ASK300 / D70, one value ?)
           M1v2  = x71 31 35, range -128 -> +127 step 0.05v (for on ASK300 / D70, two values? Fine / UFine )
           M3    = x71 31 32, range -100 -> +100 (unknown unit, value is unique to each M3 motor)
           UFine = x71 31 35, (legal values are enum)
	   Density = x73 31 31, 6800d -> 9000d (steps of 80d)
	   24v   = x61 30 00  (range 0x00 -> 0xff)

	*/

	return ret;
}

static int mitsu70x_set_sleeptime(struct mitsu70x_ctx *ctx, uint8_t time)
{
	uint8_t cmdbuf[4];
	int ret;

	/* 60 minutes max, according to all docs. */
	if (time > 60)
		time = 60;

	/* Send Parameter.. */
	memset(cmdbuf, 0, 4);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x53;
	cmdbuf[2] = 0x53;
	cmdbuf[3] = time;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, 4)))
		return ret;

	return CUPS_BACKEND_OK;
}

static int mitsu70x_set_iserial(struct mitsu70x_ctx *ctx, uint8_t enabled)
{
	uint8_t cmdbuf[4];
	int ret;

	if (enabled)
		enabled = 0;
	else
		enabled = 0x80;

	/* Send Parameter.. */
	memset(cmdbuf, 0, 4);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x53;
	cmdbuf[2] = 0x4e;
	cmdbuf[3] = enabled;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, 4)))
		return ret;

	return CUPS_BACKEND_OK;
}

#if 0
/* Switches between "Driver" and "SDK" modes.
   Single-endpoint vs Multi-Endpoint, essentially.
   Not sure about the polarity.
 */
static int mitsu70x_set_printermode(struct mitsu70x_ctx *ctx, uint8_t enabled)
{
	uint8_t cmdbuf[4];
	int ret;

	if (enabled)
		enabled = 0;
	else
		enabled = 0x80;

	/* Send Parameter.. */
	memset(cmdbuf, 0, 4);
	cmdbuf[0] = 0x1b;
	cmdbuf[1] = 0x53;
	cmdbuf[2] = 0x50;
	cmdbuf[3] = enabled;

	if ((ret = send_data(ctx->conn,
			     cmdbuf, 4)))
		return ret;

	return CUPS_BACKEND_OK;
}
#endif

static int mitsu70x_wakeup(struct mitsu70x_ctx *ctx, int wait)
{
	int ret;
	uint8_t buf[512];
	struct mitsu70x_jobstatus jobstatus;

top:
	/* Query job status for jobid 0 (global) */
	ret = mitsu70x_get_jobstatus(ctx, &jobstatus, 0x0000);
	if (ret)
		return CUPS_BACKEND_FAILED;

	/* Trigger a wakeup if necessary */
	if (jobstatus.power) {
		INFO("Waking up printer...\n");

		memset(buf, 0, sizeof(buf));
		buf[0] = 0x1b;
		buf[1] = 0x45;
		buf[2] = 0x57; // XXX also, 0x53, 0x54 seen.
		buf[3] = 0x55;

		if ((ret = send_data(ctx->conn,
				     buf, sizeof(buf))))
			return CUPS_BACKEND_FAILED;

		if (wait) {
			sleep(1);
			goto top;
		}
	}


	return CUPS_BACKEND_OK;
}

static int d70_library_callback(void *context, void *buffer, uint32_t len)
{
	uint32_t chunk = len;
	uint32_t offset = 0;
	int ret = 0;

	struct mitsu70x_ctx *ctx = context;

	while (chunk > 0) {
		if (chunk > CHUNK_LEN)
			chunk = CHUNK_LEN;

		ret = send_data(ctx->conn, (uint8_t*)buffer + offset, chunk);
		if (ret < 0)
			break;

		offset += chunk;
		chunk = len - offset;
	}

	return ret;
}

static int mitsu70x_main_loop(void *vctx, const void *vjob, int wait_for_return)
{
	struct mitsu70x_ctx *ctx = vctx;
	struct mitsu70x_jobstatus jobstatus;
	struct mitsu70x_printerstatus_resp resp;
	struct mitsu70x_hdr *hdr;
	uint8_t last_status[4] = {0xff, 0xff, 0xff, 0xff};

	int ret;
	int copies;
	int deck, legal, reqdeck;

	struct mitsu70x_printjob *job = (struct mitsu70x_printjob *) vjob;

	if (!ctx)
		return CUPS_BACKEND_FAILED;
	if (!job)
		return CUPS_BACKEND_FAILED;

	copies = job->common.copies;
	hdr = (struct mitsu70x_hdr*) job->databuf;

	/* Keep track of deck requested */
	reqdeck = hdr->deck;

	if (job->raw_format)
		goto bypass;

	struct BandImage input;
	uint8_t rew[2] = { 1, 1 }; /* 1 for rewind ok (default!) */

	/* Load in the CPC file, if needed */
	if (job->cpcfname && job->cpcfname != ctx->last_cpcfname) {
		char full[2048];
		ctx->last_cpcfname = job->cpcfname;
		if (ctx->lib.cpcdata)
			ctx->lib.DestroyCPCData(ctx->lib.cpcdata);

		snprintf(full, sizeof(full), "%s/%s", corrtable_path, job->cpcfname);

		ctx->lib.cpcdata = ctx->lib.GetCPCData(full);
		if (!ctx->lib.cpcdata) {
			ERROR("Unable to load CPC file '%s'\n", full);
			return CUPS_BACKEND_CANCEL;
		}
	}

	/* Load in the secondary CPC, if needed */
	if (job->ecpcfname != ctx->last_ecpcfname) {
		char full[2048];
		ctx->last_ecpcfname = job->ecpcfname;
		if (ctx->lib.ecpcdata)
			ctx->lib.DestroyCPCData(ctx->lib.ecpcdata);

		snprintf(full, sizeof(full), "%s/%s", corrtable_path, job->ecpcfname);

		if (job->ecpcfname) {
			ctx->lib.ecpcdata = ctx->lib.GetCPCData(full);
			if (!ctx->lib.ecpcdata) {
				ERROR("Unable to load CPC file '%s'\n", full);
				return CUPS_BACKEND_CANCEL;
			}
		} else {
			ctx->lib.ecpcdata = NULL;
		}
	}

	/* Convert using image processing library */
	input.origin_rows = input.origin_cols = 0;
	input.rows = job->rows;
	input.cols = job->cols;
	input.imgbuf = job->spoolbuf;
	input.bytes_per_row = job->cols * 3;

	ctx->output.origin_rows = ctx->output.origin_cols = 0;
	ctx->output.rows = job->rows;
	ctx->output.cols = job->cols;
	ctx->output.imgbuf = job->databuf + job->datalen;
	ctx->output.bytes_per_row = job->cols * 3 * 2;

	DEBUG("Running print data through processing library\n");
	if (ctx->lib.DoImageEffect(ctx->lib.cpcdata, ctx->lib.ecpcdata,
				   &input, &ctx->output, job->sharpen, job->reverse, rew)) {
		ERROR("Image Processing failed, aborting!\n");
		return CUPS_BACKEND_CANCEL;
	}

	/* Twiddle rewind stuff if needed */
	if (ctx->conn->type != P_MITSU_D70X) {
		hdr->rewind[0] = !rew[0];
		hdr->rewind[1] = !rew[1];
		DEBUG("Rewind Inhibit? %02x %02x\n", hdr->rewind[0], hdr->rewind[1]);
	}

	/* Move up the pointer to after the image data */
	job->datalen += 3*job->planelen;

	/* Clean up */
	free(job->spoolbuf);
	job->spoolbuf = NULL;
	job->spoolbuflen = 0;

	/* Now that we've filled everything in, read matte from file */
	if (job->matte) {
		ret = mitsu_readlamdata(job->laminatefname, LAMINATE_STRIDE,
					job->databuf, &job->datalen,
					be16_to_cpu(hdr->lamrows), be16_to_cpu(hdr->lamcols), 2);
		if (ret)
			return ret;

		/* Zero out the tail end of the buffer. */
		ret = be16_to_cpu(hdr->lamcols) * be16_to_cpu(hdr->lamrows) * 2;
		memset(job->databuf + job->datalen, 0, job->matte - ret);
	}

bypass:
	/* Bypass */
	if (test_mode >= TEST_MODE_NOPRINT)
		return CUPS_BACKEND_OK;

	INFO("Waiting for printer idle...\n");

	/* Ensure printer is awake */
	ret = mitsu70x_wakeup(ctx, 1);
	if (ret)
		return CUPS_BACKEND_FAILED;

top:
	/* Query job status for jobid 0 (global) */
	ret = mitsu70x_get_jobstatus(ctx, &jobstatus, 0x0000);
	if (ret)
		return CUPS_BACKEND_FAILED;

	/* Figure out which deck(s) can be used.
	   This should be in the main loop due to copy retries */

	/* First, try to respect requested deck */
	if (ctx->conn->type == P_MITSU_D70X) {
		deck = reqdeck; /* Respect D70 deck choice, 0 is automatic. */
	} else {
		deck = 1; /* All others have one deck only */
	}

	/* If user requested a specific deck, go with it, if it's legal */
	if (deck == 1 && job->decks_ok[0]) {
		deck = 1;
	} else if (deck == 2 && job->decks_ok[1]) {
		deck = 2;
	/* If we have an exact match for media, use it exclusively */
	} else if (job->decks_exact[0] && job->decks_exact[1]) {
		deck = 1 | 2;
	} else if (job->decks_exact[0]) {
		deck = 1;
	} else if (job->decks_exact[1]) {
		deck = 2;
	/* Use a non-exact match only if we don't have an exact match */
	} else if (job->decks_ok[0] && job->decks_ok[1]) {
		deck = 1 | 2;
	} else if (job->decks_ok[0]) {
		deck = 1;
	} else if (job->decks_ok[1]) {
		deck = 2;
	} else {
		ERROR("Loaded media does not match job!\n");
		return CUPS_BACKEND_CANCEL;
	}

	if (ctx->num_decks > 1)
		DEBUG("Deck selection mask: %d (%d %d %d/%d %d/%d) \n",
		      deck, hdr->deck, job->rows,
		      job->decks_exact[0], job->decks_exact[1],
		      job->decks_ok[0], job->decks_ok[1]);

	/* Okay, we know which decks are _legal_, pick one to use */
	legal = deck;
	if (deck & 1) {
		if (jobstatus.temperature == TEMPERATURE_COOLING) {
			if (ctx->num_decks == 2)
				INFO("Lower deck cooling down...\n");
			else
				INFO("Printer cooling down...\n");
			deck &= ~1;
		} else if (jobstatus.error_status[0]) {
			ERROR("%s %s/%s -> %s:  %02x/%02x/%02x\n",
			      ctx->num_decks == 2 ? "LOWER:": "",
			      mitsu70x_errorclass(jobstatus.error_status),
			      mitsu70x_errors(jobstatus.error_status),
			      mitsu70x_errorrecovery(jobstatus.error_status),
			      jobstatus.error_status[0],
			      jobstatus.error_status[1],
			      jobstatus.error_status[2]);
			deck &= ~1;
			legal &= ~1;  /* Deck is offline! */
		} else if (jobstatus.mecha_status[0] != MECHA_STATUS_IDLE) {
			deck &= ~1;
		}
	}
	if (deck & 2) {
		if (jobstatus.temperature_up == TEMPERATURE_COOLING) {
			INFO("Upper deck cooling down...\n");
			deck &= ~2;
		} else if (jobstatus.error_status_up[0]) {
			ERROR("UPPER: %s/%s -> %s:  %02x/%02x/%02x\n",
			      mitsu70x_errorclass(jobstatus.error_status_up),
			      mitsu70x_errors(jobstatus.error_status_up),
			      mitsu70x_errorrecovery(jobstatus.error_status_up),
			      jobstatus.error_status_up[0],
			      jobstatus.error_status_up[1],
			      jobstatus.error_status_up[2]);
			deck &= ~2;
			legal &= ~2;  /* Deck is offline! */
		} else if (jobstatus.mecha_status_up[0] != MECHA_STATUS_IDLE) {
			deck &= ~2;
		}
	}

	if (deck == 3) {
		/* Both decks OK to use, pick one at random */
		if (rand() & 1)
			deck = 1;
		else
			deck = 2;
	}

	if (ctx->num_decks > 1)
		DEBUG("Deck selected: %d\n", deck);

	/* Great, we have no decks we can currently print this job on.. */
	if (deck == 0) {
		/* Halt queue if printer is entirely offline */
		if (ctx->num_decks == 2) {
			if (jobstatus.error_status[0] && jobstatus.error_status_up[0]) {
				ERROR("Both decks offline due to errors\n");
				return CUPS_BACKEND_STOP;
			}
		} else {
			if (jobstatus.error_status[0]) {
				ERROR("Printer offline due to errors\n");
				return CUPS_BACKEND_STOP;
			}
		}

		/* Hold job if we have no legal decks for it, but printer is online. */
		if (!legal) {
			ERROR("Legal deck for printjob has errors, aborting job\n");
			return CUPS_BACKEND_HOLD;
		}

		/* Legal decks are busy, retry */
		sleep(1);
		goto top;
	}

	/* Perform memory status query */
	{
		struct mitsu70x_memorystatus_resp memory;
		INFO("Checking Memory availability\n");

		ret = mitsu70x_get_memorystatus(ctx, job, hdr->multicut, &memory);
		if (ret)
			return CUPS_BACKEND_FAILED;

		/* Check size is sane */
		if (memory.size || memory.memory == 0xff) {
			ERROR("Unsupported print size!\n");
			return CUPS_BACKEND_CANCEL;
		}
		if (memory.memory) {
			INFO("Printer buffers full, retrying!\n");
			sleep(1);
			goto top;
		}
	}

#if 0
	/* Make sure we don't have any jobid collisions */
	{
		int i;
		struct mitsu70x_jobs jobs;

		ret = mitsu70x_get_jobs(ctx, &jobs);
		if (ret)
			return CUPS_BACKEND_FAILED;
		for (i = 0 ; i < NUM_JOBS ; i++) {
			if (jobs.jobs[0].id == 0)
				break;
			if (ctx->jobid == be16_to_cpu(jobs.jobs[0].id)) {
				ctx->jobid++;
				if (!ctx->jobid)
					ctx->jobid++;
				i = -1;
			}
		}
	}
#endif
	while(!ctx->jobid || ctx->jobid == be16_to_cpu(jobstatus.jobid))
		ctx->jobid++;

	/* Set jobid */
	hdr->jobid = cpu_to_be16(ctx->jobid);

	/* Set deck */
	hdr->deck = deck;

	/* K60 and EK305 need the mcut type 1 specified for 4x6 prints! */
	if ((ctx->conn->type == P_MITSU_K60 || ctx->conn->type == P_KODAK_305) &&
	    job->cols == 0x0748 &&
	    job->rows == 0x04c2 && !hdr->multicut) {
		hdr->multicut = 1;
	}

	/* We're clear to send data over! */
	INFO("Sending Print Job (internal id %u)\n", ctx->jobid);

	if ((ret = send_data(ctx->conn,
			     job->databuf,
			     sizeof(struct mitsu70x_hdr))))
		return CUPS_BACKEND_FAILED;

	if (ctx->lib.dl_handle && !job->raw_format) {
		if (ctx->lib.SendImageData(&ctx->output, ctx, d70_library_callback))
			return CUPS_BACKEND_FAILED;

		if (job->matte)
			if (d70_library_callback(ctx, job->databuf + job->datalen - job->matte, job->matte))
			    return CUPS_BACKEND_FAILED;
	} else { // Fallback code..
               /* K60 and 305 need data sent in 256K chunks, but the first
                  chunk needs to subtract the length of the 512-byte header */

		int chunk = CHUNK_LEN - sizeof(struct mitsu70x_hdr);
		int sent = 512;
		while (chunk > 0) {
			if ((ret = send_data(ctx->conn,
					     job->databuf + sent, chunk)))
				return CUPS_BACKEND_FAILED;
			sent += chunk;
			chunk = job->datalen - sent;
			if (chunk > CHUNK_LEN)
				chunk = CHUNK_LEN;
		}
       }

	/* Then wait for completion, if so desired.. */
	INFO("Waiting for printer to acknowledge completion\n");

	do {
		sleep(1);

		ret = mitsu70x_get_printerstatus(ctx, &resp);
		if (ret)
			return CUPS_BACKEND_FAILED;

		ctx->marker[0].levelmax = be16_to_cpu(resp.lower.capacity);
		ctx->marker[0].levelnow = be16_to_cpu(resp.lower.remain);
		if (ctx->num_decks == 2) {
			ctx->marker[1].levelmax = be16_to_cpu(resp.upper.capacity);
			ctx->marker[1].levelnow = be16_to_cpu(resp.upper.remain);
		}
		if (ctx->marker[0].levelnow != ctx->last_l ||
		    ctx->marker[1].levelnow != ctx->last_u) {
			dump_markers(ctx->marker, ctx->num_decks, 0);
			ctx->last_l = ctx->marker[0].levelnow;
			ctx->last_u = ctx->marker[1].levelnow;
		}

		/* Query job status for our used jobid */
		ret = mitsu70x_get_jobstatus(ctx, &jobstatus, ctx->jobid);
		if (ret)
			return CUPS_BACKEND_FAILED;

		/* See if we hit a printer error. */
		if (deck == 1) {
			if (jobstatus.error_status[0]) {
				ERROR("%s/%s -> %s:  %02x/%02x/%02x\n",
				      mitsu70x_errorclass(jobstatus.error_status),
				      mitsu70x_errors(jobstatus.error_status),
				      mitsu70x_errorrecovery(jobstatus.error_status),
				      jobstatus.error_status[0],
				      jobstatus.error_status[1],
				      jobstatus.error_status[2]);

				/* Retry job on the other deck.. */
				if (ctx->num_decks == 2)
					goto top;

				return CUPS_BACKEND_STOP;
			}
		} else if (deck == 2) {
			if (jobstatus.error_status_up[0]) {
				ERROR("UPPER: %s/%s -> %s:  %02x/%02x/%02x\n",
				      mitsu70x_errorclass(jobstatus.error_status_up),
				      mitsu70x_errors(jobstatus.error_status_up),
				      mitsu70x_errorrecovery(jobstatus.error_status_up),
				      jobstatus.error_status_up[0],
				      jobstatus.error_status_up[1],
				      jobstatus.error_status_up[2]);

				/* Retry job on the other deck.. */
				if (ctx->num_decks == 2)
					goto top;

				return CUPS_BACKEND_STOP;
			}
		}

		/* Only print if job status is changed */
		if (jobstatus.job_status[0] != last_status[0] ||
		    jobstatus.job_status[1] != last_status[1] ||
		    jobstatus.job_status[2] != last_status[2] ||
		    jobstatus.job_status[3] != last_status[3])
			INFO("%s: %02x/%02x/%02x/%02x\n",
			     mitsu70x_jobstatuses(jobstatus.job_status),
			     jobstatus.job_status[0],
			     jobstatus.job_status[1],
			     jobstatus.job_status[2],
			     jobstatus.job_status[3]);

		/* Check for job completion */
		if (jobstatus.job_status[0] == JOB_STATUS0_END) {
			if (jobstatus.job_status[1] ||
			    jobstatus.job_status[2] ||
			    jobstatus.job_status[3]) {
				ERROR("Abnormal exit: %02x/%02x/%02x\n",
				      jobstatus.job_status[1],
				      jobstatus.job_status[2],
				      jobstatus.job_status[3]);
				return CUPS_BACKEND_STOP;
			}
			/* Job complete */
			break;
		}

		/* See if we can return early, but wait until printing has started! */
		if (!wait_for_return && copies <= 1 && /* Copies generated by backend! */
		    jobstatus.job_status[0] == JOB_STATUS0_PRINT &&
		    jobstatus.job_status[1] > JOB_STATUS1_PRINT_MEDIALOAD)
		{
			INFO("Fast return mode enabled.\n");
			break;
		}

		/* On a two deck system, try to use the second deck
		   for additional copies. If we can't use it, we'll block. */
		if (ctx->num_decks > 1 && copies > 1)
			break;

		/* Update cache for the next round */
		memcpy(last_status, jobstatus.job_status, 4);
	} while(1);

	/* Clean up */
	if (terminate)
		copies = 1;

	INFO("Print complete (%d copies remaining)\n", copies - 1);

	if (copies && --copies) {
		goto top;
	}

	return CUPS_BACKEND_OK;
}

static void mitsu70x_dump_printerstatus(struct mitsu70x_ctx *ctx,
					struct mitsu70x_printerstatus_resp *resp)
{
	uint32_t i;
	uint8_t memory = ~resp->memory;

	INFO("Model         : ");
	for (i = 0 ; i < 6 ; i++) {
		DEBUG2("%c", le16_to_cpu(resp->model[i]) & 0x7f);
	}
	DEBUG2("\n");
	INFO("Serial Number : ");
	for (i = 0 ; i < 6 ; i++) {
		DEBUG2("%c", le16_to_cpu(resp->serno[i]) & 0x7f);
	}
	DEBUG2("\n");
	for (i = 0 ; i < 7 ; i++) {
		char buf[7];
		char type;
		if (resp->vers[i].ver[5] == '@')  /* "DUMMY@" */
			continue;
		memcpy(buf, resp->vers[i].ver, 6);
		buf[6] = 0;
		if (i == 0) type = 'M';  /* Main */
		else if (i == 1) type = 'L'; /* Loader */
		else if (i == 2) type = 'R'; /* Tag */
		else if (i == 3) type = 'T'; /* Copy */
		else if (i == 4) type = 'F'; /* FPGA */
		else type = i + 0x30;

		INFO("FW Component: %c %s (%04x)\n",
		     type, buf, be16_to_cpu(resp->vers[i].checksum));
	}
	INFO("Standby Timeout: %d minutes\n", resp->sleeptime);
	INFO("iSerial Reporting: %s\n", resp->iserial ? "No" : "Yes" );
	INFO("Power Status: %s\n", resp->power ? "Sleeping" : "Awake");
	INFO("Available Memory Banks: %s%s%s%s%s%s%s%s\n",
	     (memory & 0x01) ? "mem8 " : "",
	     (memory & 0x02) ? "mem7 " : "",
	     (memory & 0x04) ? "mem6 " : "",
	     (memory & 0x08) ? "mem5 " : "",
	     (memory & 0x10) ? "mem4 " : "",
	     (memory & 0x20) ? "mem3 " : "",
	     (memory & 0x40) ? "mem2 " : "",
	     (memory & 0x80) ? "mem1 " : "");

	if (resp->lower.error_status[0]) {
		INFO("Lower Error Status: %s/%s -> %s\n",
		     mitsu70x_errorclass(resp->lower.error_status),
		     mitsu70x_errors(resp->lower.error_status),
		     mitsu70x_errorrecovery(resp->lower.error_status));
	}
	INFO("Lower Temperature: %s\n", mitsu_temperatures(resp->lower.temperature));
	INFO("Lower Mechanical Status: %s\n",
	     mitsu70x_mechastatus(resp->lower.mecha_status));
	INFO("Lower Media Type:  %s (%02x/%02x/%02x)\n",
	     mitsu_media_types(ctx->conn->type, resp->lower.media_brand, resp->lower.media_type),
	     resp->lower.media_brand,
	     resp->lower.media_type,
	     resp->lower.media_subtype);
	INFO("Lower Prints Remaining:  %03d/%03d\n",
	     be16_to_cpu(resp->lower.remain),
	     be16_to_cpu(resp->lower.capacity));
	i = packed_bcd_to_uint32((char*)resp->lower.lifetime_prints, 4);
	if (i)
		i-= 10;
	INFO("Lower Lifetime Prints:  %u\n", i);

	if (ctx->num_decks == 2) {
		if (resp->upper.error_status[0]) {
			INFO("Upper Error Status: %s/%s -> %s\n",
			     mitsu70x_errorclass(resp->upper.error_status),
			     mitsu70x_errors(resp->upper.error_status),
			     mitsu70x_errorrecovery(resp->upper.error_status));
		}
		INFO("Upper Temperature: %s\n", mitsu_temperatures(resp->upper.temperature));
		INFO("Upper Mechanical Status: %s\n",
		     mitsu70x_mechastatus(resp->upper.mecha_status));
		INFO("Upper Media Type:  %s (%02x/%02x/%02x)\n",
		     mitsu_media_types(ctx->conn->type, resp->upper.media_brand, resp->upper.media_type),
		     resp->upper.media_brand,
		     resp->upper.media_type,
		     resp->upper.media_subtype);
		INFO("Upper Prints Remaining:  %03d/%03d\n",
		     be16_to_cpu(resp->upper.remain),
		     be16_to_cpu(resp->upper.capacity));

		i = packed_bcd_to_uint32((char*)resp->upper.lifetime_prints, 4);
		if (i)
			i-= 10;
		INFO("Upper Lifetime Prints:  %u\n", i);
	}
}

static int mitsu70x_query_jobs(struct mitsu70x_ctx *ctx)
{
#if 0
	struct mitsu70x_jobs jobs;
#endif
	struct mitsu70x_jobstatus jobstatus;
	int ret;

	ret = mitsu70x_get_jobstatus(ctx, &jobstatus, 0x0000);
	if (ret)
		return CUPS_BACKEND_FAILED;

	INFO("JOB00 ID     : %06u\n", jobstatus.jobid);
	INFO("JOB00 status : %s\n", mitsu70x_jobstatuses(jobstatus.job_status));
	INFO("Power Status: %s\n", jobstatus.power ? "Sleeping" : "Awake");

	if (ctx->num_decks == 2) {
		INFO("Lower Deck Mechanical Status: %s\n",
		     mitsu70x_mechastatus(jobstatus.mecha_status));
		if (jobstatus.error_status[0]) {
			INFO("%s/%s -> %s\n",
			     mitsu70x_errorclass(jobstatus.error_status),
			     mitsu70x_errors(jobstatus.error_status),
			     mitsu70x_errorrecovery(jobstatus.error_status));
		}
		INFO("Lower Deck Temperature: %s\n", mitsu_temperatures(jobstatus.temperature));

		INFO("Upper Deck Mechanical Status: %s\n",
		     mitsu70x_mechastatus(jobstatus.mecha_status_up));
		if (jobstatus.error_status_up[0]) {
			INFO("%s/%s -> %s\n",
			     mitsu70x_errorclass(jobstatus.error_status_up),
			     mitsu70x_errors(jobstatus.error_status_up),
			     mitsu70x_errorrecovery(jobstatus.error_status_up));
		}
		INFO("Upper Deck Temperature: %s\n", mitsu_temperatures(jobstatus.temperature_up));
	} else {
		INFO("Mechanical Status: %s\n",
		     mitsu70x_mechastatus(jobstatus.mecha_status));
		if (jobstatus.error_status[0]) {
			INFO("%s/%s -> %s\n",
			     mitsu70x_errorclass(jobstatus.error_status),
			     mitsu70x_errors(jobstatus.error_status),
			     mitsu70x_errorrecovery(jobstatus.error_status));
		}
		INFO("Temperature: %s\n", mitsu_temperatures(jobstatus.temperature));
	}

	// memory status?

#if 0
	ret = mitsu70x_get_jobs(ctx, &jobs);
	if (!ret) {
		int i;
		for (i = 0 ; i < NUM_JOBS ; i++) {
			if (jobs.jobs[i].id == 0)
				break;
			INFO("JOB%02d ID     : %06u\n", i, jobs.jobs[i].id);
			INFO("JOB%02d status : %s\n", i, mitsu70x_jobstatuses(jobs.jobs[i].status));
		}
	}

done:
#endif
	return CUPS_BACKEND_OK;
}

static int mitsu70x_query_status(struct mitsu70x_ctx *ctx)
{
	struct mitsu70x_printerstatus_resp resp;
	int ret;

	ret = mitsu70x_get_printerstatus(ctx, &resp);
	if (!ret)
		mitsu70x_dump_printerstatus(ctx, &resp);

	return ret;
}

static int mitsu70x_query_serno(struct dyesub_connection *conn, char *buf, int buf_len)
{
	int ret, i;
	struct mitsu70x_printerstatus_resp resp = { .hdr = { 0 } };

	struct mitsu70x_ctx ctx = {
		.conn = conn,
	};

	ret = mitsu70x_get_printerstatus(&ctx, &resp);

	if (buf_len > 6)  /* Will we ever have a buffer under 6 bytes? */
		buf_len = 6;

	for (i = 0 ; i < buf_len ; i++) {
		*buf++ = le16_to_cpu(resp.serno[i]) & 0x7f;
	}
	*buf = 0; /* Null-terminate the returned string */

	return ret;
}


static void mitsu70x_cmdline(void)
{
	DEBUG("\t\t[ -s ]           # Query printer status\n");
	DEBUG("\t\t[ -j ]           # Query job status\n");
	DEBUG("\t\t[ -w ]           # Wake up printer\n");
	DEBUG("\t\t[ -W ]           # Wake up printer and wait\n");
	DEBUG("\t\t[ -k num ]       # Set standby time (1-60 minutes, 0 disables)\n");
	DEBUG("\t\t[ -x num ]       # Set USB iSerialNumber Reporting (1 on, 0 off)\n");
	DEBUG("\t\t[ -X jobid ]     # Abort a printjob\n");
//	DEBUG("\t\t[ -t ]           # Dump calibration info (use with -DDD)\n");
//	DEBUG("\t\t[ -T 0-6 ]       # Test print\n");
}

static int mitsu70x_cmdline_arg(void *vctx, int argc, char **argv)
{
	struct mitsu70x_ctx *ctx = vctx;
	int i, j = 0;

	if (!ctx)
		return -1;

	while ((i = getopt(argc, argv, GETOPT_LIST_GLOBAL "jk:tT:swWX:x:")) >= 0) {
		switch(i) {
		GETOPT_PROCESS_GLOBAL
		case 'j':
			j = mitsu70x_query_jobs(ctx);
			break;
		case 'k':
			j = mitsu70x_set_sleeptime(ctx, atoi(optarg));
			break;
		case 's':
			j = mitsu70x_query_status(ctx);
			break;
		case 't':
			j = mitsu70x_test_dump(ctx);
			break;
		case 'T':
			j = mitsu70x_test_print(ctx, atoi(optarg));
			break;
		case 'w':
			j = mitsu70x_wakeup(ctx, 0);
			break;
		case 'W':
			j = mitsu70x_wakeup(ctx, 1);
			break;
		case 'x':
			j = mitsu70x_set_iserial(ctx, atoi(optarg));
			break;
		case 'X':
			j = mitsu70x_cancel_job(ctx, atoi(optarg));
			break;
		default:
			break;  /* Ignore completely */
		}

		if (j) return j;
	}

	return CUPS_BACKEND_OK;
}

static int mitsu70x_query_markers(void *vctx, struct marker **markers, int *count)
{
	struct mitsu70x_ctx *ctx = vctx;
	struct mitsu70x_printerstatus_resp resp;
	int ret;

	if (markers)
		*markers = ctx->marker;
	if (count)
		*count = ctx->num_decks;

	/* Tell CUPS about the consumables we report */
	ret = mitsu70x_get_printerstatus(ctx, &resp);
	if (ret)
		return CUPS_BACKEND_FAILED;

	if (resp.power) {
		ret = mitsu70x_wakeup(ctx, 1);
		if (ret)
			return CUPS_BACKEND_FAILED;

		ret = mitsu70x_get_printerstatus(ctx, &resp);
		if (ret)
			return CUPS_BACKEND_FAILED;
	}

	ctx->marker[0].levelmax = be16_to_cpu(resp.lower.capacity);
	ctx->marker[0].levelnow = be16_to_cpu(resp.lower.remain);
	if (ctx->num_decks == 2) {
		ctx->marker[1].levelmax = be16_to_cpu(resp.upper.capacity);
		ctx->marker[1].levelnow = be16_to_cpu(resp.upper.remain);
	}

	return CUPS_BACKEND_OK;
}

static int mitsu70x_job_polarity(void *vctx)
{
	struct mitsu70x_ctx *ctx = vctx;

	if (test_mode >= TEST_MODE_NOATTACH)
		return 0;

        if (mitsu70x_query_markers(ctx, NULL, NULL))
                return 0;

	/* D70x and ASK300 don't support rewinding */
	if (ctx->conn->type == P_MITSU_D70X ||
	    ctx->conn->type == P_FUJI_ASK300)
		return 0;

	/* All others do, and only have one deck */
	return (ctx->marker[0].levelnow & 1);
}

static int mitsu70x_query_stats(void *vctx, struct printerstats *stats)
{
	struct mitsu70x_ctx *ctx = vctx;
	struct mitsu70x_printerstatus_resp resp;

	if (mitsu70x_query_markers(ctx, NULL, NULL))
		return CUPS_BACKEND_FAILED;

	if (mitsu70x_get_printerstatus(ctx, &resp))
		return CUPS_BACKEND_FAILED;

	switch (ctx->conn->type) {
	case P_MITSU_D70X:
		stats->mfg = "Mitsubishi";
		if (ctx->num_decks == 2) {
			if (ctx->is_s)
				stats->model = "CP-D707DW-S";
			else
				stats->model = "CP-D707DW";
		} else {
			if (ctx->is_s)
				stats->model = "CP-D70DW-S";
			else
				stats->model = "CP-D70DW";
		}
		break;
	case P_MITSU_K60:
		stats->mfg = "Mitsubishi";
		stats->model = "CP-K60DW-S";
		// XXX does is_s matter?  Mitsu SDK cares, but all models sold are labeled as -S..
		break;
	case P_MITSU_D80:
		stats->mfg = "Mitsubishi";
		if (ctx->is_s)
			stats->model = "CP-D80DW-S";
		else
			stats->model = "CP-D80DW";
		break;
	case P_KODAK_305:
		stats->mfg = "Kodak";
		stats->model = "305";
		break;
	case P_FUJI_ASK300:
		stats->mfg = "Fujifilm";
		stats->model = "ASK-300";
		break;
	default:
		stats->mfg = "Unknown";
		stats->model = "Unknown";
		break;
	}

	stats->serial = ctx->serno;
	stats->fwver = ctx->fwver;
	stats->decks = ctx->num_decks;

	stats->name[0] = "Lower";
	stats->status[0] = strdup(mitsu70x_mechastatus(resp.lower.mecha_status));
	stats->mediatype[0] = ctx->marker[0].name;
	stats->levelmax[0] = ctx->marker[0].levelmax;
	stats->levelnow[0] = ctx->marker[0].levelnow;
	stats->cnt_life[0] = packed_bcd_to_uint32((char*)resp.lower.lifetime_prints, 4);

	if (stats->decks == 2) {
		stats->name[1] = "Upper";
		stats->status[1] = strdup(mitsu70x_mechastatus(resp.upper.mecha_status));
		stats->mediatype[1] = ctx->marker[1].name;
		stats->levelmax[1] = ctx->marker[1].levelmax;
		stats->levelnow[1] = ctx->marker[1].levelnow;
		stats->cnt_life[1] = packed_bcd_to_uint32((char*)resp.upper.lifetime_prints, 4);
	}
	return CUPS_BACKEND_OK;
}

static const char *mitsu70x_prefixes[] = {
	"mitsu70x", // Family entry, do not nuke.
	// backwards compatibility
	"mitsud80", "mitsuk60", "kodak305", "fujiask300",
	NULL,
};

/* Exported */
const struct dyesub_backend mitsu70x_backend = {
	.name = "Mitsubishi CP-D70 family",
	.version = "0.106" " (lib " LIBMITSU_VER ")",
	.flags = BACKEND_FLAG_DUMMYPRINT,
	.uri_prefixes = mitsu70x_prefixes,
	.cmdline_usage = mitsu70x_cmdline,
	.cmdline_arg = mitsu70x_cmdline_arg,
	.init = mitsu70x_init,
	.attach = mitsu70x_attach,
	.teardown = mitsu70x_teardown,
	.cleanup_job = mitsu70x_cleanup_job,
	.read_parse = mitsu70x_read_parse,
	.main_loop = mitsu70x_main_loop,
	.query_serno = mitsu70x_query_serno,
	.query_markers = mitsu70x_query_markers,
	.query_stats = mitsu70x_query_stats,
	.combine_jobs = mitsu70x_combine_jobs,
	.job_polarity = mitsu70x_job_polarity,
	.devices = {
		{ 0x06d3, 0x3b30, P_MITSU_D70X, NULL, "mitsubishi-d70dw"},
		{ 0x06d3, 0x3b30, P_MITSU_D70X, NULL, "mitsubishi-d707dw"}, /* Duplicate */
		{ 0x06d3, 0x3b31, P_MITSU_K60, NULL, "mitsubishi-k60dw"}, // variation type?
		{ 0x06d3, 0x3b36, P_MITSU_D80, NULL, "mitsubishi-d80dw"},
		{ 0x040a, 0x404f, P_KODAK_305, NULL, "kodak-305"},
		{ 0x04cb, 0x5006, P_FUJI_ASK300, NULL, "fujifilm-ask-300"},
		{ 0, 0, 0, NULL, NULL}
	}
};

/* Mitsubish CP-D70DW/D707DW/K60DW-S/D80DW, Kodak 305, Fujifilm ASK-300
   data format:

   Spool file consists of two headers followed by three image planes
   and an optional lamination data plane.  All blocks are rounded up to
   a 512-byte boundary.

   All multi-byte numbers are big endian, ie MSB first.

   Header 1:  (AKA Wake Up)

   1b 45 57 55 00 00 00 00  00 00 00 00 00 00 00 00
   (padded by NULLs to a 512-byte boundary)

   Header 2:  (Print Header)

   1b 5a 54 PP JJ JJ RR RR  00 00 00 00 00 00 00 00
   XX XX YY YY QQ QQ ZZ ZZ  SS 00 00 00 00 00 00 00
   UU 00 00 00 00 00 00 00  LL TT 00 00 00 00 00 00
   MM 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00

   (padded by NULLs to a 512-byte boundary)

   PP    == 0x01 on D70x/D80, 0x00 on K60, 0x90 on K305, 0x80 on ASK300
   JJ JJ == Job ID, can leave at 00 00
   XX XX == columns
   YY YY == rows
   QQ QQ == lamination columns (equal to XX XX)
   ZZ ZZ == lamination rows (YY YY + 12 on D70x/D80/ASK300, YY YY on others)
   RR RR == "rewind inhibit", 01 01 enabled, normally 00 00 (All but D70x/A300)
   SS    == Print mode: 00 = Fine, 03 = SuperFine (D70x/D80 only), 04 = UltraFine
            (Matte requires Superfine or Ultrafine)
   UU    == 00 = Auto, 01 = Lower Deck (required for !D70x), 02 = Upper Deck
   LL    == lamination enable, 00 == on, 01 == off
   TT    == lamination mode: 00 glossy, 02 matte
   MM    == 00 (normal), 01 = (Double-cut 4x6), 05 = (double-cut 2x6)

   Data planes:
   16-bit data, rounded up to 512-byte block (XX * YY * 2 bytes)

   Lamination plane: (only present if QQ and ZZ are nonzero)
   16-byte data, rounded up to 512-byte block (QQ * ZZ * 2 bytes)

 */
