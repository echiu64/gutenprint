/*
 *   CUPS Backend common code
 *
 *   (c) 2013-2014 Solomon Peachy <pizza@shaftnet.org>
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

#include <libusb-1.0/libusb.h>
#include <arpa/inet.h>

#ifndef __BACKEND_COMMON_H
#define __BACKEND_COMMON_H

#define STR_LEN_MAX 64
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
			(((uint16_t)(__x) & (uint16_t)0xff00) >>  8) | \
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
	P_ES40_CP790,
	P_ES40,
	P_CP790,
	P_CP_XXX,
	P_CP10,
	P_KODAK_6800,
	P_KODAK_6850,
	P_KODAK_1400_805,
	P_KODAK_605,
	P_SHINKO_S2145,
	P_SONY_UPDR150,
	P_MITSU_D70X,
	P_DNP_DS40,
	P_DNP_DS80,
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
	int  (*early_parse)(void *ctx, int data_fd);
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

/* Exported data */
extern int terminate;
extern int dyesub_debug;

/* External data */
extern struct dyesub_backend updr150_backend;
extern struct dyesub_backend kodak6800_backend;
extern struct dyesub_backend kodak605_backend;
extern struct dyesub_backend kodak1400_backend;
extern struct dyesub_backend shinkos2145_backend;
extern struct dyesub_backend canonselphy_backend;
extern struct dyesub_backend mitsu70x_backend;
extern struct dyesub_backend dnpds40_backend;

#endif /* __BACKEND_COMMON_H */
