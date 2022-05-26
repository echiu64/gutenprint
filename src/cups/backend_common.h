/*
 *   CUPS Backend common code
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <libusb.h>

/* For Integration into gutenprint */
#if defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#ifndef __BACKEND_COMMON_H
#define __BACKEND_COMMON_H

#ifdef ERROR
#undef ERROR
#endif

#define STR_LEN_MAX 64
#define STATE( ... ) do { if (!quiet) fprintf(logger, "STATE: " __VA_ARGS__ ); } while(0)
#define ATTR( ... ) do { if (!quiet) fprintf(logger, "ATTR: " __VA_ARGS__ ); } while(0)
#define PAGE( ... ) do { if (!quiet) fprintf(logger, "PAGE: " __VA_ARGS__ ); } while(0)
#define DEBUG( ... ) do { if (!quiet) fprintf(logger, "DEBUG: " __VA_ARGS__ ); } while(0)
#define DEBUG2( ... ) do { if (!quiet) fprintf(logger, __VA_ARGS__ ); } while(0)
#define INFO( ... )  do { if (!quiet) fprintf(logger, "INFO: " __VA_ARGS__ ); } while(0)
#define WARNING( ... )  do { fprintf(logger, "WARNING: " __VA_ARGS__ ); } while(0)
#define ERROR( ... ) do { fprintf(logger, "ERROR: " __VA_ARGS__ ); sleep(1); } while (0)
#define PPD( ... ) do { fprintf(logger, "PPD: " __VA_ARGS__ ); } while (0)

#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define le16_to_cpu(__x) __x
#define le32_to_cpu(__x) __x
#define be16_to_cpu(__x) __builtin_bswap16(__x)
#define be32_to_cpu(__x) __builtin_bswap32(__x)
#else
#define le16_to_cpu(__x) __builtin_bswap16(__x)
#define le32_to_cpu(__x) __builtin_bswap32(__x)
#define be32_to_cpu(__x) __x
#define be16_to_cpu(__x) __x
#endif

#define cpu_to_le16 le16_to_cpu
#define cpu_to_le32 le32_to_cpu
#define cpu_to_be16 be16_to_cpu
#define cpu_to_be32 be32_to_cpu

/* To cheat the compiler */
#define UNUSED(expr) do { (void)(expr); } while (0)

/* Compile-time assertions */
#define STATIC_ASSERT(test_for_true) _Static_assert((test_for_true), "(" #test_for_true ") failed")

/* IEEE1284 ID processing */
struct deviceid_dict {
	char *key;
	char *val;
};
#define MAX_DICT 32
int parse1284_data(const char *device_id, struct deviceid_dict* dict);
char *dict_find(const char *key, int dlen, struct deviceid_dict* dict);
char *get_device_id(struct libusb_device_handle *dev, int iface);

/* To enumerate supported devices */
enum {
	P_UNKNOWN = 0,
	P_CITIZEN_CW01,
	P_CITIZEN_OP900II,
	P_CPGENERIC,
	P_CP10,
	P_CP790,
	P_CP900,
	P_CP910,
	P_DNP_DS40,
	P_DNP_DS80,
	P_DNP_DS80D,
	P_DNP_DS620,
	P_DNP_DS820,
	P_DNP_DSRX1,
	P_DNP_QW410,
	P_ES1,
	P_ES2_20,
	P_ES3_30,
	P_ES40,
	P_FUJI_ASK300,
	P_FUJI_ASK500,
	P_HITI_CS2XX,
	P_HITI_51X,
	P_HITI_52X,
	P_HITI_720,
	P_HITI_750,
	P_HITI_910,
	P_KODAK_1400_805,
	P_KODAK_305,
	P_KODAK_605,
	P_KODAK_6800,
	P_KODAK_6850,
	P_KODAK_6900,
	P_KODAK_7000,
	P_KODAK_701X,
	P_KODAK_8800,
	P_KODAK_8810,
	P_MAGICARD,
	P_MITSU_9550,
	P_MITSU_9550S,
	P_MITSU_9600,
	P_MITSU_9800,
	P_MITSU_9800S,
	P_MITSU_9810,
	P_MITSU_D70X,
	P_MITSU_D80,
	P_MITSU_D90,
	P_MITSU_K60,
	P_MITSU_M1,
	P_MITSU_P93D,
	P_MITSU_P95D,
	P_MITSU_CP30D,
	P_SHINKO_S1245,
	P_SHINKO_S2145,
	P_SHINKO_S2245,
	P_SHINKO_S6145,
	P_SHINKO_S6145D,
	P_SHINKO_S6245,
	P_SONY_UPCR10,
	P_SONY_UPCR20L,
	P_SONY_UPD895,
	P_SONY_UPD897,
	P_SONY_UPD898,
	P_SONY_UP9x1,
	P_SONY_UPDR150,
	P_SONY_UPDR80,
	P_END,
};

struct device_id {
	uint16_t vid;
	uint16_t pid;
	int type;  /* P_** */
	const char *manuf_str;
	const char *make;
};

struct marker {
	const char *color;  /* Eg "#00FFFF" */
	const char *name;   /* Eg "CK9015 (4x6)" */
	int levelmax; /* Max media count, eg '600', or '-1' */
	int levelnow; /* Remaining media, -3, -2, -1, 0..N.  See CUPS. */
	int numtype; /* Numerical type, (-1 for unknown) */
};

#define DECKS_MAX 2
struct printerstats {
	time_t timestamp;
	const char *mfg;      /* Manufacturer */
	const char *model;    /* Model */
	const char *serial;   /* Serial Number */
	const char *fwver;    /* Firmware Version */
	uint8_t decks;        /* Number of "decks" (1 or 2) */

	const char *name[DECKS_MAX];  /* Name */
	char *status[DECKS_MAX];      /* Status (dynamic) */
	const char *mediatype[DECKS_MAX]; /* Media Type */
	int32_t levelmax[DECKS_MAX];  /* Max media count (-1 if unknown) */
	int32_t levelnow[DECKS_MAX];  /* Remaining media count (-1 if unknown) */
	int32_t cnt_life[DECKS_MAX];  /* Lifetime prints */
};

struct dyesub_connection {
	struct libusb_device_handle *dev;
	uint8_t endp_up;
	uint8_t endp_down;
	uint8_t iface;
	uint8_t altset;

	/* to make our lives easier later */
	uint8_t bus_num;
	uint8_t port_num;
	uint16_t usb_vid;
	uint16_t usb_pid;

	// TODO:  mutex/lock

	int type; /* P_XXXX */
};

#define DYESUB_MAX_JOB_ENTRIES 3

struct dyesub_joblist {
	// TODO: mutex/lock
	const struct dyesub_backend *backend;
	void *ctx;
	int num_entries;
	int copies;
	const void *entries[DYESUB_MAX_JOB_ENTRIES];
};
#define MAX_JOBS_FROM_READ_PARSE 3

/* This MUST be the start of every per-printer job struct! */
struct dyesub_job_common {
	size_t jobsize;
	int copies;
	int can_combine;
};

/* Exported functions */
int send_data(struct dyesub_connection *conn, const uint8_t *buf, int len);
int read_data(struct dyesub_connection *conn,
	       uint8_t *buf, int buflen, int *readlen);

void dump_markers(const struct marker *markers, int marker_count, int full);

void print_license_blurb(void);
void print_help(const char *argv0, const struct dyesub_backend *backend);

int dyesub_read_file(const char *filename, void *databuf, int datalen,
		     int *actual_len);

uint16_t uint16_to_packed_bcd(uint16_t val);
uint32_t packed_bcd_to_uint32(const char *in, int len);

void generic_teardown(void *vctx);

/* USB enumeration and attachment */
#define NUM_CLAIM_ATTEMPTS 10
int backend_claim_interface(struct libusb_device_handle *dev, int iface,
			    int num_claim_attempts);

/* Job list manipulation */
struct dyesub_joblist *dyesub_joblist_create(const struct dyesub_backend *backend, void *ctx);
int dyesub_joblist_appendjob(struct dyesub_joblist *list, const void *job);
void dyesub_joblist_cleanup(const struct dyesub_joblist *list);
int dyesub_joblist_print(const struct dyesub_joblist *list, int *pagenum);
const void *dyesub_joblist_popjob(struct dyesub_joblist *list);
int dyesub_joblist_canwait(struct dyesub_joblist *list);

#define BACKEND_FLAG_BADISERIAL 0x00000001
#define BACKEND_FLAG_DUMMYPRINT 0x00000002

int dyesub_pano_split_rgb8(const uint8_t *src, uint16_t cols,
			   uint16_t src_rows, uint8_t numpanels,
			   uint16_t overlap_rows, uint16_t max_rows,
			   uint8_t *panels[3],
			   uint16_t panel_rows[3]);

/* Backend Functions */
struct dyesub_backend {
	const char *name;
	const char *version;
	const char **uri_prefixes;
	const uint32_t flags;
	void (*cmdline_usage)(void);  /* Optional */
	void *(*init)(void);
	int  (*attach)(void *ctx, struct dyesub_connection *conn, uint8_t jobid);
	void (*teardown)(void *ctx);
	int  (*cmdline_arg)(void *ctx, int argc, char **argv);
	int  (*read_parse)(void *ctx, const void **job, int data_fd, int copies);
	void (*cleanup_job)(const void *job);
	void *(*combine_jobs)(const void *job1, const void *job2);
	int  (*job_polarity)(void *ctx);
	int  (*main_loop)(void *ctx, const void *job, int wait_on_return);
	int  (*query_serno)(struct dyesub_connection *conn, char *buf, int buf_len); /* Optional */
	int  (*query_markers)(void *ctx, struct marker **markers, int *count);
	int  (*query_stats)(void *ctx, struct printerstats *stats); /* Optional */
	const struct device_id devices[];
};

/* Global data */
extern int terminate;
extern int dyesub_debug;
extern int fast_return;
extern int extra_vid;
extern int extra_pid;
extern int extra_type;
extern int ncopies;
extern int collate;
extern int test_mode;
extern int quiet;
extern const char *corrtable_path;
extern FILE *logger;
extern int stats_only;

enum {
	TEST_MODE_NONE = 0,
	TEST_MODE_NOPRINT,
	TEST_MODE_NOATTACH,
	TEST_MODE_MAX,
};

#if defined(BACKEND)
extern const struct dyesub_backend BACKEND;
#endif

/* CUPS compatibility */
#define CUPS_BACKEND_OK            0 /* Success */
#define CUPS_BACKEND_FAILED        1 /* Failed to print use CUPS policy */
#define CUPS_BACKEND_AUTH_REQUIRED 2 /* Auth required */
#define CUPS_BACKEND_HOLD          3 /* Hold this job only */
#define CUPS_BACKEND_STOP          4 /* Stop the entire queue */
#define CUPS_BACKEND_CANCEL        5 /* Cancel print job */
#define CUPS_BACKEND_RETRY         6 /* Retry later */
#define CUPS_BACKEND_RETRY_CURRENT 7 /* Retry immediately */

#define CUPS_MARKER_UNAVAILABLE   -1
#define CUPS_MARKER_UNKNOWN       -2
#define CUPS_MARKER_UNKNOWN_OK    -3

/* Argument processing */
#define GETOPT_LIST_GLOBAL "d:DfGhv"
#define GETOPT_PROCESS_GLOBAL \
			case 'd': \
				ncopies = atoi(optarg); \
				break; \
			case 'D': \
				dyesub_debug++; \
				break; \
			case 'f': \
				fast_return++; \
				break; \
			case 'G': \
				print_license_blurb(); \
				exit(0); \
			case 'h': \
				print_help(argv[0], &BACKEND); \
				exit(0); \
			case 'v': \
				quiet++; \
				break;

/* Dynamic library loading */
#if defined(USE_DLOPEN)
#define WITH_DYNAMIC
#include <dlfcn.h>
#define DL_INIT() do {} while(0)
#define DL_OPEN(__x) dlopen(__x, RTLD_NOW)
#define DL_SYM(__x, __y) dlsym(__x, __y)
#define DL_CLOSE(__x) dlclose(__x)
#define DL_EXIT() do {} while(0)
#elif defined(USE_LTDL)
#define WITH_DYNAMIC
#include <ltdl.h>
#define DL_INIT() lt_dlinit()
#define DL_OPEN(__x) lt_dlopen(__x)
#define DL_SYM(__x, __y) lt_dlsym(__x, __y)
#define DL_CLOSE(__x) do {} while(0)
#define DL_EXIT() lt_dlexit()
#else
#define DL_INIT()     do {} while(0)
#define DL_CLOSE(__x) do {} while(0)
#define DL_EXIT()     do {} while(0)
#endif
#if defined(_WIN32)
#define DLL_SUFFIX ".dll"
#elif defined(__APPLE__)
#define DLL_SUFFIX ".dylib"
#else
#define DLL_SUFFIX ".so"
#endif

#endif /* __BACKEND_COMMON_H */
