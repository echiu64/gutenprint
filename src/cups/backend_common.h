/*
 *   CUPS Backend common code
 *
 *   (c) 2013-2018 Solomon Peachy <pizza@shaftnet.org>
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

#include <libusb.h>
#include <arpa/inet.h>

#ifndef __BACKEND_COMMON_H
#define __BACKEND_COMMON_H

#define STR_LEN_MAX 64
#define STATE( ... ) fprintf(stderr, "STATE: " __VA_ARGS__ )
#define ATTR( ... ) fprintf(stderr, "ATTR: " __VA_ARGS__ )
#define PAGE( ... ) fprintf(stderr, "PAGE: " __VA_ARGS__ )
#define DEBUG( ... ) fprintf(stderr, "DEBUG: " __VA_ARGS__ )
#define DEBUG2( ... ) fprintf(stderr, __VA_ARGS__ )
#define INFO( ... )  fprintf(stderr, "INFO: " __VA_ARGS__ )
#define WARNING( ... )  fprintf(stderr, "WARNING: " __VA_ARGS__ )
#define ERROR( ... ) do { fprintf(stderr, "ERROR: " __VA_ARGS__ ); sleep(1); } while (0)

#if (__BYTE_ORDER == __LITTLE_ENDIAN)
#define le32_to_cpu(__x) __x
#define le16_to_cpu(__x) __x
#define be16_to_cpu(__x) ntohs(__x)
#define be32_to_cpu(__x) ntohl(__x)
#else
#define le32_to_cpu(x)							\
	({								\
		uint32_t __x = (x);					\
		((uint32_t)(						\
			(((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
			(((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
			(((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
			(((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) )); \
	})
#define le16_to_cpu(x)							\
	({								\
		uint16_t __x = (x);					\
		((uint16_t)(						\
			(((uint16_t)(__x) & (uint16_t)0x00ff) <<  8) | \
			(((uint16_t)(__x) & (uint16_t)0xff00) >>  8))); \
	})
#define be32_to_cpu(__x) __x
#define be16_to_cpu(__x) __x
#endif

#define cpu_to_le16 le16_to_cpu
#define cpu_to_le32 le32_to_cpu
#define cpu_to_be16 be16_to_cpu
#define cpu_to_be32 be32_to_cpu

/* To cheat the compiler */
#define UNUSED(expr) do { (void)(expr); } while (0)

/* To enumerate supported devices */
enum {
	P_UNKNOWN = 0,
	P_CP_XXX = 1,
	P_CP10 = 2,
	P_CP790 = 3,
	P_CP900 = 4,
	P_CP910 = 5,
	P_ES1 = 6,
	P_ES2_20 = 7,
	P_ES3_30 = 8,
	P_ES40 = 9,
	P_KODAK_1400_805 = 10,
	P_KODAK_6800 = 11,
	P_KODAK_6850 = 12,
	P_KODAK_305 = 13,
	P_KODAK_605 = 14,
	P_SHINKO_S1245 = 15,
	P_SHINKO_S2145 = 16,
	P_SHINKO_S6145 = 17,
	P_SHINKO_S6145D = 18,
	P_SHINKO_S6245 = 19,
	P_SONY_UPCR10 = 20,
	P_SONY_UPDR150 = 21,
	P_MITSU_9550 = 22,
	P_MITSU_9550S = 23,
	P_MITSU_9600 = 24,
	P_MITSU_9800 = 25,
	P_MITSU_9800S = 26,
	P_MITSU_9810 = 27,
	P_MITSU_D70X = 28,
	P_MITSU_D80 = 29,
	P_MITSU_D90 = 30,
	P_MITSU_K60 = 31,
	P_MITSU_P93D = 32,
	P_MITSU_P95D = 33,
	P_CITIZEN_CW01 = 34,
	P_CITIZEN_OP900II = 35,
	P_DNP_DS40 = 36,
	P_DNP_DS620 = 37,
	P_DNP_DS80 = 38,
	P_DNP_DS80D = 39,
	P_DNP_DS820 = 40,
	P_DNP_DSRX1 = 41,
	P_FUJI_ASK300 = 42,
	P_MAGICARD = 43,
	P_END,
};

struct device_id {
	uint16_t vid;
	uint16_t pid;
	int type;  /* P_** */
	char *manuf_str;
	char *prefix;
};

struct marker {
	const char *color;  /* Eg "#00FFFF" */
	const char *name;   /* Eg "CK9015 (4x6)" */
	int levelmax; /* Max media count, eg '600', or '-1' */
	int levelnow; /* Remaining media, -3, -2, -1, 0..N.  See CUPS. */
};

/* Backend Functions */
struct dyesub_backend {
	const char *name;
	const char *version;
	const char **uri_prefixes;
	void (*cmdline_usage)(void);  /* Optional */
	void *(*init)(void);
	int  (*attach)(void *ctx, struct libusb_device_handle *dev, int type,
		       uint8_t endp_up, uint8_t endp_down, uint8_t jobid);
	void (*teardown)(void *ctx);
	int  (*cmdline_arg)(void *ctx, int argc, char **argv);
	int  (*read_parse)(void *ctx, int data_fd);
	int  (*main_loop)(void *ctx, int copies);
	int  (*query_serno)(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len); /* Optional */
	int  (*query_markers)(void *ctx, struct marker **markers, int *count);
	const struct device_id devices[];
};

/* Exported functions */
int send_data(struct libusb_device_handle *dev, uint8_t endp,
	      uint8_t *buf, int len);
int read_data(struct libusb_device_handle *dev, uint8_t endp,
	      uint8_t *buf, int buflen, int *readlen);

void dump_markers(struct marker *markers, int marker_count, int full);

void print_license_blurb(void);
void print_help(char *argv0, struct dyesub_backend *backend);

uint16_t uint16_to_packed_bcd(uint16_t val);
uint32_t packed_bcd_to_uint32(char *in, int len);

/* Global data */
extern int terminate;
extern int dyesub_debug;
extern int fast_return;
extern int extra_vid;
extern int extra_pid;
extern int extra_type;
extern int copies;
extern int test_mode;

enum {
	TEST_MODE_NONE = 0,
	TEST_MODE_NOPRINT,
	TEST_MODE_NOATTACH,
	TEST_MODE_MAX,
};

#if defined(BACKEND)
extern struct dyesub_backend BACKEND;
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

/* Argument processing */
#define GETOPT_LIST_GLOBAL "d:DfGh"
#define GETOPT_PROCESS_GLOBAL \
			case 'd': \
				copies = atoi(optarg); \
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
				exit(0);

#endif /* __BACKEND_COMMON_H */
