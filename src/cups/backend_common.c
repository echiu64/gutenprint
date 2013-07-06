/*
 *   CUPS Backend common code
 *
 *   (c) 2013 Solomon Peachy <pizza@shaftnet.org>
 *
 *   The latest version of this program can be found at:
 *  
 *     http://git.shaftnet.org/git/gitweb.cgi?p=selphy_print.git
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

#include <libusb-1.0/libusb.h>
#include <arpa/inet.h>

#define BACKEND_VERSION "0.2"

#define STR_LEN_MAX 64
#define DEBUG( ... ) fprintf(stderr, "DEBUG: " __VA_ARGS__ )
#define DEBUG2( ... ) fprintf(stderr, __VA_ARGS__ )
#define INFO( ... )  fprintf(stderr, "INFO: " __VA_ARGS__ )
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

#define ID_BUF_SIZE 2048
static char *get_device_id(struct libusb_device_handle *dev)
{
	int   length;
	int claimed = 0;
	int iface = 0;
	char *buf = malloc(ID_BUF_SIZE + 1);

	claimed = libusb_kernel_driver_active(dev, iface);
	if (claimed)
		libusb_detach_kernel_driver(dev, iface);

	libusb_claim_interface(dev, iface);

	if (libusb_control_transfer(dev,
				    LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_ENDPOINT_IN |
				    LIBUSB_RECIPIENT_INTERFACE,
				    0, 0,
				    (iface << 8),
				    (unsigned char *)buf, ID_BUF_SIZE, 5000) < 0)
	{
		*buf = '\0';
		goto done;
	}

	/* length is the first two bytes, MSB first */
	length = (((unsigned)buf[0] & 255) << 8) |
		((unsigned)buf[1] & 255);

	/* Sanity checks */
	if (length > ID_BUF_SIZE || length < 14)
		length = (((unsigned)buf[1] & 255) << 8) |
			((unsigned)buf[0] & 255);
	
	if (length > ID_BUF_SIZE)
		length = ID_BUF_SIZE;
	
	if (length < 14) {
		*buf = '\0';
		goto done;
	}

	/* Move, and terminate */
	memmove(buf, buf + 2, length);
	buf[length] = '\0';

done:
	libusb_release_interface(dev, iface);
	if (claimed)
		libusb_attach_kernel_driver(dev, iface);

	return buf;
}

static int send_data(struct libusb_device_handle *dev, uint8_t endp, 
		    uint8_t *buf, int len)
{
	int num;

	while (len) {
		int ret = libusb_bulk_transfer(dev, endp,
					       buf, (len > 65536) ? 65536: len,
					       &num, 5000);
		if (ret < 0) {
			ERROR("Failure to send data to printer (libusb error %d: (%d/%d to 0x%02x))\n", ret, num, len, endp);
			return ret;
		}
		len -= num;
		buf += num;
//		DEBUG("Sent %d (%d remaining) to 0x%x\n", num, len, endp);
	}

	return 0;
}

static int terminate = 0;

static void sigterm_handler(int signum) {
	terminate = 1;
	INFO("Job Cancelled");
}

static int print_scan_output(struct libusb_device *device,
			     struct libusb_device_descriptor *desc,
			     char *prefix, char *manuf2,
			     int found, int match, int valid, 
			     int scan_only, char *match_serno)
{
	struct libusb_device_handle *dev;

	unsigned char product[STR_LEN_MAX] = "";
	unsigned char serial[STR_LEN_MAX] = "";
	unsigned char manuf[STR_LEN_MAX] = "";
	
	if (libusb_open(device, &dev)) {
		ERROR("Could not open device %04x:%04x\n", desc->idVendor, desc->idProduct);
		found = -1;
		goto abort;
	}
	
	/* Query detailed info */
	if (desc->iManufacturer) {
		libusb_get_string_descriptor_ascii(dev, desc->iManufacturer, manuf, STR_LEN_MAX);
	}
	if (desc->iProduct) {
		libusb_get_string_descriptor_ascii(dev, desc->iProduct, product, STR_LEN_MAX);
	}
	if (desc->iSerialNumber) {
		libusb_get_string_descriptor_ascii(dev, desc->iSerialNumber, serial, STR_LEN_MAX);
	}
	
	if (!strlen((char*)serial))
		strcpy((char*)serial, "NONE");
	
	DEBUG("%s%sPID: %04X Product: '%s' Serial: '%s'\n",
	      (!valid) ? "UNRECOGNIZED: " : "",
	      match ? "MATCH: " : "",
	      desc->idProduct, product, serial);
	
	if (valid && scan_only) {
		/* URL-ify model. */
		char buf[128]; // XXX ugly..
		int j = 0, k = 0;
		char *ieee_id;
		while (*(product + j + strlen(manuf2))) {
			buf[k] = *(product + j + strlen(manuf2) + 1);
			if(buf[k] == ' ') {
				buf[k++] = '%';
				buf[k++] = '2';
				buf[k] = '0';
			}
			k++;
			j++;
		}
		ieee_id = get_device_id(dev);
		
		fprintf(stdout, "direct %s%s/%s?serial=%s \"%s\" \"%s\" \"%s\" \"\"\n",
			prefix, manuf2,
			buf, serial, product, product,
			ieee_id);
		
		if (ieee_id)
			free(ieee_id);
	}
	
	/* If a serial number was passed down, use it. */
	if (found && match_serno &&
	    strcmp(match_serno, (char*)serial)) {
		found = -1;
	}
	
	libusb_close(dev);
abort:
	return found;
}
