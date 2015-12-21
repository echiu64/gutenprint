/*
 *   CUPS Backend common code
 *
 *   (c) 2013-2015 Solomon Peachy <pizza@shaftnet.org>
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
	P_ANY = 0,
	P_ES1,
	P_ES2_20,
	P_ES3_30,
	P_ES40,
	P_CP790,
	P_CP_XXX,
	P_CP10,
	P_KODAK_6800,
	P_KODAK_6850,
	P_KODAK_1400_805,
	P_KODAK_605,
	P_SHINKO_S2145,
	P_SHINKO_S1245,
	P_SHINKO_S6245,
	P_SHINKO_S6145,	
        P_SHINKO_S6145D,
	P_SONY_UPDR150,
	P_SONY_UPCR10,
	P_MITSU_D70X,
	P_MITSU_K60,	
	P_MITSU_9550,
	P_MITSU_9550S,	
	P_DNP_DS40,
	P_DNP_DS80,
	P_DNP_DS80D,
	P_CITIZEN_CW01,
	P_DNP_DSRX1,
	P_DNP_DS620,
	P_END,
};

struct device_id {
	uint16_t vid;
	uint16_t pid;
	int type;  /* P_** */
	char *manuf_str;
};

/* Backend Functions */
struct dyesub_backend {
	char *name;
	char *version;
	char *uri_prefix;
	void (*cmdline_usage)(void);
	void *(*init)(void);
	void (*attach)(void *ctx, struct libusb_device_handle *dev,
		       uint8_t endp_up, uint8_t endp_down, uint8_t jobid);
	void (*teardown)(void *ctx);
	int  (*cmdline_arg)(void *ctx, int argc, char **argv);
	int  (*read_parse)(void *ctx, int data_fd);
	int  (*main_loop)(void *ctx, int copies);
	int  (*query_serno)(struct libusb_device_handle *dev, uint8_t endp_up, uint8_t endp_down, char *buf, int buf_len);
	struct device_id devices[];
};

/* Exported functions */
int send_data(struct libusb_device_handle *dev, uint8_t endp,
	      uint8_t *buf, int len);
int read_data(struct libusb_device_handle *dev, uint8_t endp,
	      uint8_t *buf, int buflen, int *readlen);
int lookup_printer_type(struct dyesub_backend *backend, uint16_t idVendor, uint16_t idProduct);

void print_license_blurb(void);
void print_help(char *argv0, struct dyesub_backend *backend);

uint16_t uint16_to_packed_bcd(uint16_t val);

/* Global data */
extern int terminate;
extern int dyesub_debug;
extern int fast_return;
extern int extra_vid;
extern int extra_pid;
extern int extra_type;
extern int copies;
extern char *use_serno;
extern int current_page;

#if defined(BACKEND)
extern struct dyesub_backend BACKEND;
#endif

/* CUPS compatibility */
#define CUPS_BACKEND_OK            0 /* Sucess */
#define CUPS_BACKEND_FAILED        1 /* Failed to print use CUPS policy */
#define CUPS_BACKEND_AUTH_REQUIRED 2 /* Auth required */
#define CUPS_BACKEND_HOLD          3 /* Hold this job only */
#define CUPS_BACKEND_STOP          4 /* Stop the entire queue */
#define CUPS_BACKEND_CANCEL        5 /* Cancel print job */
#define CUPS_BACKEND_RETRY         6 /* Retry later */
#define CUPS_BACKEND_RETRY_CURRENT 7 /* Retry immediately */

/* Argument processing */
#define GETOPT_LIST_GLOBAL "d:DfGhP:S:T:V:"
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
				exit(0); \
			case 'P': \
				extra_pid = strtol(optarg, NULL, 16); \
				break; \
			case 'S': \
				use_serno = optarg; \
				break; \
			case 'T': \
				extra_type = atoi(optarg); \
				break; \
			case 'V': \
				extra_pid = strtol(optarg, NULL, 16); \
				break;

#endif /* __BACKEND_COMMON_H */
