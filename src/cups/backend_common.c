/*
 *   CUPS Backend common code
 *
 *   Copyright (c) 2007-2021 Solomon Peachy <pizza@shaftnet.org>
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

#include "backend_common.h"
#include <errno.h>
#include <signal.h>
#include <strings.h>  /* For strncasecmp */

#define BACKEND_VERSION "0.123G"

#ifndef CORRTABLE_PATH
#ifdef PACKAGE_DATA_DIR
#define CORRTABLE_PATH PACKAGE_DATA_DIR "/backend_data"
#else
#error "Must define CORRTABLE_PATH or PACKAGE_DATA_DIR!"
#endif
#endif

#define URB_XFER_SIZE  (64*1024)
#define XFER_TIMEOUT    15000

#define USB_SUBCLASS_PRINTER 0x1
#define USB_INTERFACE_PROTOCOL_BIDIR 0x2
#define USB_INTERFACE_PROTOCOL_IPP   0x4

/* Global Variables */
int dyesub_debug = 0;
int terminate = 0;
int fast_return = 0;
int extra_vid = -1;
int extra_pid = -1;
int extra_type = -1;
int ncopies = 1;
int collate = 0;
int test_mode = 0;
int quiet = 0;
int stats_only = 0;
FILE *logger;

const char *corrtable_path = CORRTABLE_PATH;
static int max_xfer_size = URB_XFER_SIZE;
static int xfer_timeout = XFER_TIMEOUT;

#ifdef OLD_URI
static int old_uri = 1;
#else
static int old_uri = 0;
#endif

/* Support Functions */
int backend_claim_interface(struct libusb_device_handle *dev, int iface,
			    int num_claim_attempts)
{
	int ret;
	do {
		ret = libusb_claim_interface(dev, iface);
		if (!ret)
			break;
		if (ret != LIBUSB_ERROR_BUSY)
			break;
		if (--num_claim_attempts == 0)
			break;
		sleep(1);
	} while (1);

	if (ret)
		ERROR("Failed to claim interface %d (%d)\n", iface, ret);

	return ret;
}

static int lookup_printer_type(struct dyesub_backend *backend, uint16_t idVendor, uint16_t idProduct)
{
	int i;
	int type = P_UNKNOWN;

	for (i = 0 ; backend->devices[i].vid ; i++) {
		if (extra_pid != -1 &&
		    extra_vid != -1 &&
		    extra_type != -1) {
			if (backend->devices[i].type == extra_type &&
			    extra_vid == idVendor &&
			    extra_pid == idProduct) {
				return extra_type;
			}
		}
		if (idVendor == backend->devices[i].vid &&
		    idProduct == backend->devices[i].pid) {
			return backend->devices[i].type;
		}
	}

	return type;
}

/* Interface **MUST** already be claimed! */
#define ID_BUF_SIZE 2048
char *get_device_id(struct libusb_device_handle *dev, int iface)
{
	int length;
	char *buf = malloc(ID_BUF_SIZE + 1);

	if (!buf) {
		ERROR("Memory allocation failure (%d bytes)\n", ID_BUF_SIZE+1);
		return NULL;
	}

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

	/* IEEE1284 length field includs the header! */
	length -= 2;

	/* Move, and terminate */
	memmove(buf, buf + 2, length);
	buf[length] = '\0';

done:
	return buf;
}

/* Used with the IEEE1284 deviceid string parsing */
int parse1284_data(const char *device_id, struct deviceid_dict* dict)
{
	char *ptr;
	char key[256];
	char val[256];
	int num = 0;

	if (!device_id)
		return CUPS_BACKEND_OK;

	//[whitespace]key[whitespace]:[whitespace]value[whitespace];
	while (*device_id && num < MAX_DICT) {
		/* Skip leading spaces */
		if (*device_id == ' ')
			device_id++;
		if (!*device_id)
			break;

		/* Work out key */
		for (ptr = key; *device_id && *device_id != ':'; device_id++)
			*ptr++ = *device_id;
		if (!*device_id)
			break;
		while (ptr > key && *(ptr-1) == ' ')
			ptr--;
		*ptr = 0;
		device_id++;
		if (!*device_id)
			break;

		/* Next up, value */
		for (ptr = val; *device_id && *device_id != ';'; device_id++)
			*ptr++ = *device_id;
		while (ptr > val && *(ptr-1) == ' ')
			ptr--;
		*ptr = 0;
		device_id++;

		/* Add it to the dictionary */
		dict[num].key = strdup(key);
		dict[num].val = strdup(val);
		num++;

		if (!*device_id)
			break;
	}
	return num;
}

char *dict_find(const char *key, int dlen, struct deviceid_dict* dict)
{
	while(dlen) {
		if (!strcmp(key, dict->key))
			return dict->val;
		dlen--;
		dict++;
	}
	return NULL;
}

/* I/O functions */

int read_data(struct dyesub_connection *conn, uint8_t *buf, int buflen, int *readlen)
{
	int ret;

	/* Clear buffer */
	memset(buf, 0, buflen);

	ret = libusb_bulk_transfer(conn->dev, conn->endp_up,
				   buf,
				   buflen,
				   readlen,
				   xfer_timeout);

	if (ret < 0) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, *readlen, buflen, conn->endp_up);
		goto done;
	}

	if (dyesub_debug) {
		DEBUG("Received %d bytes from printer\n", *readlen);
	}

	if ((dyesub_debug > 1 && *readlen < 4096) ||
	    dyesub_debug > 2) {
		int i = *readlen;

		DEBUG("<- ");
		while(i > 0) {
			if ((*readlen-i) != 0 &&
			    (*readlen-i) % 16 == 0) {
				DEBUG2("\n");
				DEBUG("   ");
			}
			DEBUG2("%02x ", buf[*readlen-i]);
			i--;
		}
		DEBUG2("\n");
	}

done:
	return ret;
}

int send_data(struct dyesub_connection *conn, const uint8_t *buf, int len)
{
	int num = 0;

	if (dyesub_debug) {
		DEBUG("Sending %d bytes to printer\n", len);
	}

	while (len) {
		int len2 = (len > max_xfer_size) ? max_xfer_size: len;

		if ((dyesub_debug > 1 && len2 < 4096) ||
		    dyesub_debug > 2) {
			int i = len2;

			DEBUG("-> ");
			while(i > 0) {
				if ((len2-i) != 0 &&
				    (len2-i) % 16 == 0) {
					DEBUG2("\n");
					DEBUG("   ");
				}
				DEBUG2("%02x ", buf[len2-i]);
				i--;
			}
			DEBUG2("\n");
		}

		int ret = libusb_bulk_transfer(conn->dev, conn->endp_down,
					       (uint8_t*) buf, len2,
					       &num, xfer_timeout);

		if (ret < 0) {
			ERROR("Failure to send data to printer (libusb error %d: (%d/%d to 0x%02x))\n", ret, num, len2, conn->endp_down);
			return ret;
		}
		len -= num;
		buf += num;
	}

	return CUPS_BACKEND_OK;
}

/* More stuff */
#ifndef _WIN32
static void sigterm_handler(int signum) {
	UNUSED(signum);

	terminate = 1;
	INFO("Job Cancelled");
}
#endif

static char *sanitize_string(char *str) {
	int len = strlen(str);

	while(len && (str[len-1] <= 0x20)) {
		str[len-1] = 0;
		len--;
	}
	return str;
}

/*

   These functions are Public Domain code obtained from:

   https://www.geekhideout.com/urlcode.shtml

*/
#include <ctype.h>  /* for isalnum() */
static char to_hex(char code) {
	static const char hex[] = "0123456789abcdef";
	return hex[code & 15];
}
static char from_hex(char ch) {
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}
/* Note -- caller must free returned pointer! */
static char *url_encode(const char *str) {
	const char *pstr = str;
	char *buf = malloc(strlen(str) * 3 + 1);
	char *pbuf = buf;

	if (!buf) {
		ERROR("Memory allocation failure (%d bytes)\n", (int) strlen(str)*3 + 1);
		return NULL;
	}

	while (*pstr) {
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
			*pbuf++ = *pstr;
		else if (*pstr == ' ')
			*pbuf++ = '+';
		else
			*pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}
static char *url_decode(char *str) {
	char *pstr = str, *buf = malloc(strlen(str) + 1), *pbuf = buf;

	if (!buf) {
		ERROR("Memory allocation failure (%d bytes)\n", (int) strlen(str) + 1);
		return NULL;
	}

	while (*pstr) {
		if (*pstr == '%') {
			if (pstr[1] && pstr[2]) {
				*pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
				pstr += 2;
			}
		} else if (*pstr == '+') {
			*pbuf++ = ' ';
		} else {
			*pbuf++ = *pstr;
		}
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}

/* And now back to our regularly-scheduled programming */

static int probe_device(struct libusb_device *device,
			struct libusb_device_descriptor *desc,
			const char *make,
			const char *uri_prefix, const char *manuf_override,
			int found, int num_claim_attempts,
			int scan_only, const char *match_serno,
			struct dyesub_connection *conn,
			struct dyesub_backend *backend)
{
	struct libusb_device_handle *dev;
	char buf[256];
	char *product = NULL, *serial = NULL, *manuf = NULL, *descr = NULL;
	uint8_t iface, altset;
	struct libusb_config_descriptor *config = NULL;
	int dlen = 0;
	struct deviceid_dict dict[MAX_DICT];
	char *ieee_id = NULL;
	int i;
	uint8_t endp_up, endp_down;

	DEBUG("Probing VID: %04X PID: %04x\n", desc->idVendor, desc->idProduct);

	if ((i = libusb_open(device, &dev))) {
#ifdef _WIN32
		if (i == LIBUSB_ERROR_NOT_SUPPORTED)
			ERROR("Could not open device %04x:%04x! (Genric USB driver missing, See README)\n", desc->idVendor, desc->idProduct);
		else
#endif
			ERROR("Could not open device %04x:%04x - %d (need to be root?)\n", desc->idVendor, desc->idProduct, i);
		found = -1;
		goto abort;
	}

#if 0
	/* XXX FIXME: Iterate through bNumConfigurations */

	/* Force reset of device configuration */
	{
		int cfgnum = -1;
		if (libusb_get_configuration(dev, &cfgnum)) {
			ERROR("Can't get config!");
		}
		INFO("Config %d\n", config);
		if (config < 1)
			cfgnum = 1;
		if (libusb_set_configuration(dev, cfgnum)) {
			ERROR("Can't set config\n");
		}
	}
#endif

	/* Get descriptor for active configuration */
	if (libusb_get_active_config_descriptor(device, &config)) {
		found  = -1;
		goto abort_close;
	}

	/* Loop through all interfaces and altsettings to find candidates */
	for (iface = 0 ; iface < config->bNumInterfaces ; iface ++) {
		for (altset = 0 ; altset < config->interface[iface].num_altsetting ; altset++) {
			/* Skip interfaces that don't have enough endpoints */
			if (config->interface[iface].altsetting[altset].bNumEndpoints < 2) {
				continue;
			}

			/* Note we actually only match on explicit VID+PIDs so there's no need to filter based
			   on specific class/type */

			/* Explicitly exclude IPP-over-USB interfaces */
			if (desc->bDeviceClass == LIBUSB_CLASS_PER_INTERFACE &&
			    config->interface[iface].altsetting[altset].bInterfaceClass == LIBUSB_CLASS_PRINTER &&
			    config->interface[iface].altsetting[altset].bInterfaceSubClass == USB_SUBCLASS_PRINTER &&
			    config->interface[iface].altsetting[altset].bInterfaceProtocol == USB_INTERFACE_PROTOCOL_IPP) {
				continue;
			}

			/* Find the first set of endpoints! */
			endp_up = endp_down = 0;
			for (i = 0 ; i < config->interface[iface].altsetting[altset].bNumEndpoints ; i++) {
				if ((config->interface[iface].altsetting[altset].endpoint[i].bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) == LIBUSB_TRANSFER_TYPE_BULK) {
					if (config->interface[iface].altsetting[altset].endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_IN)
						endp_up = config->interface[iface].altsetting[altset].endpoint[i].bEndpointAddress;
					else
						endp_down = config->interface[iface].altsetting[altset].endpoint[i].bEndpointAddress;
				}
				if (endp_up && endp_down)
					goto candidate;
			}
		}
	}

	/* If we got here, we didn't find a match. */
	found = -1;
	goto abort_close;

candidate:

	/* We've now found an interface/altset we need to query in more detail */
	/* Detach the kernel driver */
	if (libusb_kernel_driver_active(dev, iface))
		libusb_detach_kernel_driver(dev, iface);

	/* Claim the interface so we can start querying things! */
	if (backend_claim_interface(dev, iface, num_claim_attempts)) {
		found = -1;
		goto abort_release;
	}

	/* Use the appropriate altesetting, but only if the
	   printer supports more than one.  Some printers don't like
	   us unconditionally setting this. */
	if (config->interface[iface].num_altsetting > 1) {
		if (libusb_set_interface_alt_setting(dev, iface, altset)) {
			ERROR("Failed to set alternative interface %d/%d\n", iface, altset);
			found = -1;
			goto abort_release;
		}
	}

	/* Query IEEE1284 info only if it's a PRINTER class */
	if (desc->bDeviceClass == LIBUSB_CLASS_PRINTER ||
	    (desc->bDeviceClass == LIBUSB_CLASS_PER_INTERFACE &&
	     config->interface[iface].altsetting[altset].bInterfaceClass == LIBUSB_CLASS_PRINTER &&
	     config->interface[iface].altsetting[altset].bInterfaceSubClass == USB_SUBCLASS_PRINTER)) {
		ieee_id = get_device_id(dev, iface);
		dlen = parse1284_data(ieee_id, dict);
	}

	if (!old_uri && !scan_only && !dyesub_debug) goto skip_manuf_model;

	/* Look up mfg string in IEEE1284 data */
	if (manuf_override && strlen(manuf_override)) {
		manuf = url_encode(manuf_override);
	} else if ((manuf = dict_find("MANUFACTURER", dlen, dict))) {
		manuf = url_encode(manuf);
	} else if ((manuf = dict_find("MFG", dlen, dict))) {
		manuf = url_encode(manuf);
	} else if ((manuf = dict_find("MFR", dlen, dict))) {
		manuf = url_encode(manuf);
	}

	/* If no manufacturer string, fall back to USB iManufacturer */
	if ((!manuf || !strlen(manuf)) &&
	    desc->iManufacturer) {
		buf[0] = 0;
		libusb_get_string_descriptor_ascii(dev, desc->iManufacturer, (unsigned char*)buf, STR_LEN_MAX);
		sanitize_string(buf);
		manuf = url_encode(buf);
	}

	if (!manuf || !strlen(manuf)) {  /* Last-ditch */
		if (manuf) free(manuf);
		WARNING("**** THIS PRINTER DOES NOT REPORT A VALID MANUFACTURER STRING!\n");
		manuf = url_encode("Unknown"); // XXX use USB VID?
	}

	/* Look up model string in IEEE1284 data */
	if ((product = dict_find("MODEL", dlen, dict))) {
		product = url_encode(product);
	} else if ((product = dict_find("MDL", dlen, dict))) {
		product = url_encode(product);
	}

	/* If no manufacturer string, fall back to USB iProduct */
	if ((!product || !strlen(product)) &&
	    desc->iProduct) {
		buf[0] = 0;
		libusb_get_string_descriptor_ascii(dev, desc->iProduct, (unsigned char*)buf, STR_LEN_MAX);
		sanitize_string(buf);
		product = url_encode(buf);
	}

	if (!product || !strlen(product)) { /* Last-ditch */
		if (!product) free(product);
		WARNING("**** THIS PRINTER DOES NOT REPORT A VALID MODEL STRING!\n");
		product = url_encode("Unknown"); // XXX Use USB PID?
	}

	/* Look up decription string in IEEE1284 data */
	if ((descr = dict_find("DESCRIPTION", dlen, dict))) {
		descr = strdup(descr);
	} else if ((descr = dict_find("DES", dlen, dict))) {
		descr = strdup(descr);
	}

	if (!descr || !strlen(descr)) { /* Last-ditch, generate */
		char *product2 = url_decode(product);
		char *manuf3 = url_decode(manuf);
		descr = malloc(514); /* 256 + 256 + 1 + 1 */
		if (!descr) {
			ERROR("Memory allocation failure (%d bytes)\n", 514);
			if (manuf3)
				free(manuf3);
			if (product2)
				free(product2);
			return -1;
		}

		snprintf(descr, 514, "%s %s", manuf3, product2);
		free(product2);
		free(manuf3);
	}

skip_manuf_model:

	/* Prefer IEEE1284-reported serial number */
	if ((serial = dict_find("SERIALNUMBER", dlen, dict))) {
		serial = url_encode(serial);
	} else if ((serial = dict_find("SN", dlen, dict))) {
		serial = url_encode(serial);
	} else if ((serial = dict_find("SER", dlen, dict))) {
		serial = url_encode(serial);
	} else if ((serial = dict_find("SERN", dlen, dict))) {
		serial = url_encode(serial);
	}

	/* If it's not valid, fall back to USB iSerial */
	if ((!serial || !strlen(serial)) &&
	    !(backend->flags & BACKEND_FLAG_BADISERIAL) &&
	    desc->iSerialNumber) {
		libusb_get_string_descriptor_ascii(dev, desc->iSerialNumber, (unsigned char*)buf, STR_LEN_MAX);
		sanitize_string(buf);
		serial = url_encode(buf);
	}

	/* TODO: What about situations where iSerial does not match IEEE1284?
	         Or if the '1284 data is bogus? */

	/* If still no serial, fall back to backend hook */
	if ((!serial || !strlen(serial)) &&
	    backend->query_serno) { /* Get from backend hook */
		struct dyesub_connection c2;
		c2.dev = dev;
		c2.iface = iface;
		c2.altset = altset;
		c2.endp_up = endp_up;
		c2.endp_down = endp_down;
		backend->query_serno(&c2, buf, STR_LEN_MAX);
		serial = url_encode(buf);
	}

	/* Last-ditch serial number fallback */
	if (!serial || !strlen(serial)) {
		if (serial) free(serial);
		WARNING("**** THIS PRINTER DOES NOT REPORT A SERIAL NUMBER!\n");
		WARNING("**** If you intend to use multiple printers of this type, you\n");
		WARNING("**** must only plug one in at a time or unexpected behavior will occur!\n");
		serial = strdup("NONE_UNKNOWN");
	}

	if (scan_only) {
		if (!old_uri) {
			fprintf(stdout, "direct %s://%s/%s \"%s\" \"%s\" \"%s\" \"\"\n",
				uri_prefix, make, serial,
				descr, descr,
				ieee_id ? ieee_id : "");
		} else {
			int k = 0;

			/* URLify the manuf and model strings */
			strncpy(buf, manuf, sizeof(buf) - 2);
			k = strlen(buf);
			buf[k++] = '/';
			buf[k] = 0;

			strncpy(buf + k, product, sizeof(buf)-k);

			fprintf(stdout, "direct %s://%s?serial=%s&backend=%s \"%s\" \"%s\" \"%s\" \"\"\n",
				uri_prefix, buf, serial, make,
				descr, descr,
				ieee_id? ieee_id : "");
		}
	}

	/* If a serial number was passed down, use it. */
	if (match_serno && strcmp(match_serno, (char*)serial)) {
		found = -1;
	}

	uint8_t bus_num = libusb_get_bus_number(device);
	uint8_t port_num = libusb_get_port_number(device);

	if (dyesub_debug)
		DEBUG("VID/PID %04X/%04X @ bus/port %03d/%03d Manuf: '%s' Product: '%s' Serial: '%s' found: %d\n",
		      desc->idVendor, desc->idProduct, bus_num, port_num, manuf, product, serial, found);

	if (found != -1 && conn) {
		conn->iface = iface;
		conn->altset = altset;
		conn->endp_up = endp_up;
		conn->endp_down = endp_down;
		conn->bus_num = bus_num;
		conn->port_num = port_num;
	}

	/* Free things up */
	if(serial) free(serial);
	if(manuf) free(manuf);
	if(product) free(product);
	if(descr) free(descr);
	if(ieee_id) free(ieee_id);

abort_release:

	libusb_release_interface(dev, iface);

abort_close:

	libusb_close(dev);

abort:
	if (config) libusb_free_config_descriptor(config);

	/* Clean up the dictionary */
	while (dlen--) {
		free (dict[dlen].key);
		free (dict[dlen].val);
	}

	return found;
}

void generic_teardown(void *vctx)
{
	if (!vctx)
		return;

	free(vctx);
}

extern struct dyesub_backend sonyupd_backend;
extern struct dyesub_backend sonyupdneo_backend;
extern struct dyesub_backend kodak6800_backend;
extern struct dyesub_backend kodak605_backend;
extern struct dyesub_backend kodak1400_backend;
extern struct dyesub_backend kodak8800_backend;
extern struct dyesub_backend shinkos1245_backend;
extern struct dyesub_backend shinkos2145_backend;
extern struct dyesub_backend shinkos6145_backend;
extern struct dyesub_backend shinkos6245_backend;
extern struct dyesub_backend canonselphy_backend;
extern struct dyesub_backend canonselphyneo_backend;
extern struct dyesub_backend mitsu70x_backend;
extern struct dyesub_backend mitsu9550_backend;
extern struct dyesub_backend mitsup95d_backend;
extern struct dyesub_backend dnpds40_backend;
extern struct dyesub_backend magicard_backend;
extern struct dyesub_backend mitsud90_backend;
extern struct dyesub_backend hiti_backend;

static struct dyesub_backend *backends[] = {
	&canonselphy_backend,
	&canonselphyneo_backend,
	&kodak6800_backend,
	&kodak605_backend,
	&kodak1400_backend,
	&kodak8800_backend,
	&shinkos1245_backend,
	&shinkos2145_backend,
	&shinkos6145_backend,
	&shinkos6245_backend,
	&sonyupd_backend,
	&sonyupdneo_backend,
	&mitsu70x_backend,
	&mitsud90_backend,
	&mitsu9550_backend,
	&mitsup95d_backend,
	&dnpds40_backend,
	&magicard_backend,
	&hiti_backend,
	NULL,
};

static int find_and_enumerate(const char *argv0,
			      struct libusb_device ***list,
			      const struct dyesub_backend *backend,
			      const char *match_serno,
			      const char *make,
			      int scan_only, int num_claim_attempts,
			      struct dyesub_connection *conn)
{
	int num;
	int i, j = 0, k;
	int found = -1;

	if (test_mode >= TEST_MODE_NOATTACH && conn) {
		found = 1;
		conn->endp_up = 0x82;
		conn->endp_down = 0x01;
		conn->iface = 0;
		conn->altset = 0;
		return found;
	}

	STATE("+org.gutenprint.searching-for-device\n");

	/* Enumerate and find suitable device */
	num = libusb_get_device_list(NULL, list);

	/* See if we can actually match on the supplied make! */
	if (backend && make) {
		int match = 0;
		for (j = 0 ; backend->devices[j].vid ; j++) {
			if (!strcmp(make,backend->devices[j].make)) {
				match = 1;
				break;
			}
		}
		/* If not, clear it */
		if (!match)
			make = NULL;
	} else {
		make = NULL; /* Explicitly clear it */
	}

	for (i = 0 ; i < num ; i++) {
		const char *foundmake = NULL;

		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor((*list)[i], &desc);

		for (k = 0 ; backends[k] ; k++) {
			if (backend && backend != backends[k])
				continue;

			for (j = 0 ; backends[k]->devices[j].vid ; j++) {
				/* Try for extra pid/vid/type */
				// XXX nuke entire extra_??? concept?
				if (extra_pid != -1 &&
				    extra_vid != -1 &&
				    extra_type != -1) {
					if (backends[k]->devices[j].type == extra_type &&
					    extra_vid == desc.idVendor &&
					    extra_pid == desc.idProduct) {
						found = i;
						make = backends[k]->uri_prefixes[0];
						goto match;
					}
				}

				/* Match based on VID/PID (and prefix, if specified) */
				if (desc.idVendor == backends[k]->devices[j].vid &&
				    (desc.idProduct == backends[k]->devices[j].pid ||
				     desc.idProduct == 0xffff) &&
				    (!make || !strcmp(make,backends[k]->devices[j].make))) {
					    found = i;
					    foundmake = backends[k]->devices[j].make;
					    goto match;
				}
			}
		}

		continue;

	match:
		found = probe_device((*list)[i], &desc, (foundmake ? foundmake : make),
				     argv0, backends[k]->devices[j].manuf_str,
				     found, num_claim_attempts,
				     scan_only, match_serno,
				     conn,
				     backends[k]);
		foundmake = NULL;
		if (found != -1 && !scan_only)
			break;
	}

	STATE("-org.gutenprint.searching-for-device\n");
	return found;
}

static struct dyesub_backend *find_backend(const char *uri_prefix)
{
	int i;

	if (!uri_prefix)
		return NULL;

	for (i = 0; ; i++) {
		struct dyesub_backend *backend = backends[i];
		const char **alias;
		int j;
		if (!backend)
			return NULL;
		for (alias = backend->uri_prefixes ; alias && *alias ; alias++) {
			if (!strcmp(uri_prefix, *alias))
				return backend;
		}
		for (j = 0 ; backend->devices[j].vid ; j++) {
			if (!strcmp(uri_prefix,backend->devices[j].make)) {
				return backend;
			}
		}
	}
	return NULL;
}

static int query_markers(const struct dyesub_backend *backend, void *ctx, int full)
{
	struct marker *markers = NULL;
	int marker_count = 0;
	int ret;

	if (!backend->query_markers)
		return CUPS_BACKEND_OK;

	if (test_mode >= TEST_MODE_NOATTACH)
		return CUPS_BACKEND_OK;

	ret = backend->query_markers(ctx, &markers, &marker_count);
	if (ret)
		return ret;

	dump_markers(markers, marker_count, full);

	return CUPS_BACKEND_OK;
}

static void dump_stats(struct dyesub_backend *backend, struct printerstats *stats, int json)
{
	int i;
	struct tm *tm;
	char tmbuf[64];
	tm = localtime(&stats->timestamp);

	strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", tm);

	if (json) {
		fprintf(stdout, "{\n");
		fprintf(stdout, "\t\"backend\": \"%s\",\n", backend->name);
		fprintf(stdout, "\t\"version\": \"%s / %s\",\n", BACKEND_VERSION, backend->version);
		fprintf(stdout, "\t\"timestamp\": \"%s\",\n", tmbuf);
		if (stats->mfg)
			fprintf(stdout, "\t\"manufacturer\": \"%s\",\n", stats->mfg);
		if (stats->model)
			fprintf(stdout, "\t\"model\": \"%s\",\n", stats->model);
		if (stats->serial)
			fprintf(stdout, "\t\"serial\": \"%s\",\n", stats->serial);
		if (stats->fwver)
			fprintf(stdout, "\t\"firmware\": \"%s\",\n", stats->fwver);

		fprintf(stdout, "\t\"decks\": {\n");
		for (i = 0 ; i < stats->decks ; i++) {
			fprintf(stdout, "\t\t\"%s\": {\n", stats->name[i]);
			if (stats->status[i])
				fprintf(stdout, "\t\t\t\"status\": \"%s\",\n", stats->status[i]);
			fprintf(stdout, "\t\t\t\"mediatype\": \"%s\",\n", stats->mediatype[i]);
			switch (stats->levelnow[i]) {
			case CUPS_MARKER_UNKNOWN:
				fprintf(stdout, "\t\t\t\"medialevel\": \"Unknown\",\n");
				break;
			case CUPS_MARKER_UNAVAILABLE:
				fprintf(stdout, "\t\t\t\"medialevel\": \"Unavailable\",\n");
				break;
			case CUPS_MARKER_UNKNOWN_OK:
				fprintf(stdout, "\t\t\t\"medialevel\": \"OK\",\n");
				break;
			default:
				if (stats->levelnow[i] >= 0 && stats->levelmax[i] > 0) {
					fprintf(stdout, "\t\t\t\"medialevel\": \"OK\",\n");
					fprintf(stdout, "\t\t\t\"medialevelnow\": %d,\n", stats->levelnow[i]);
					fprintf(stdout, "\t\t\t\"medialevelmax\": %d,\n", stats->levelmax[i]);
				} else {
				fprintf(stdout, "\t\t\t\"medialevel\": \"Illegal value\",\n");
				}
				break;
			}
			fprintf(stdout, "\t\t\t\"counters\": {\n");
			if (stats->cnt_life[i] >= 0)
				fprintf(stdout, "\t\t\t\t\"lifetime\": %d\n", stats->cnt_life[i]);

			fprintf(stdout, "\t\t\t}\n");

			fprintf(stdout, "\t\t}%c\n", (i < (stats->decks -1) ? ',': ' '));
		}
		fprintf(stdout, "\t}\n");
		fprintf(stdout, "}\n");
	} else {
		fprintf(stdout, "Backend: %s\n", backend->name);
		fprintf(stdout, "Version: %s / %s\n", BACKEND_VERSION, backend->version);
		fprintf(stdout, "Timestamp: %s\n", tmbuf);
		if (stats->mfg)
			fprintf(stdout, "Manufacturer: %s\n", stats->mfg);
		if (stats->model)
			fprintf(stdout, "Model: %s\n", stats->model);
		if (stats->serial)
			fprintf(stdout, "Serial Number: %s\n", stats->serial);
		if (stats->fwver)
			fprintf(stdout, "Firmware Version: %s\n", stats->fwver);

		for (i = 0 ; i < stats->decks ; i++) {
			if (stats->status[i])
				fprintf(stdout, "%s Status: %s\n", stats->name[i], stats->status[i]);
			if (stats->cnt_life[i] >= 0)
				fprintf(stdout, "%s Lifetime Prints: %d\n", stats->name[i], stats->cnt_life[i]);
			fprintf(stdout, "%s Media Type: %s\n", stats->name[i], stats->mediatype[i]);
			if (stats->levelnow[i] == CUPS_MARKER_UNKNOWN_OK)
				fprintf(stdout, "%s Media Level: OK\n", stats->name[i]);
			else if (stats->levelnow[i] == CUPS_MARKER_UNKNOWN)
				fprintf(stdout, "%s Media Level: Unknown\n", stats->name[i]);
			else if (stats->levelnow[i] == CUPS_MARKER_UNAVAILABLE)
				fprintf(stdout, "%s Media Level: Unavailable\n", stats->name[i]);
			else if (stats->levelnow[i] >= 0 && stats->levelmax[i] > 0)
				fprintf(stdout, "%s Media Level: %d / %d\n", stats->name[i], stats->levelnow[i], stats->levelmax[i]);
		}
	}
}

void print_license_blurb(void)
{
	const char *license = "\n\
Copyright 2007-2021 Solomon Peachy <pizza AT shaftnet DOT org>\n\
\n\
This program is free software; you can redistribute it and/or modify it\n\
under the terms of the GNU General Public License as published by the Free\n\
Software Foundation; either version 2 of the License, or (at your option)\n\
any later version.\n\
\n\
This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\n\
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License\n\
for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program; if not, see <https://www.gnu.org/licenses/>.\n\n";

	fprintf(logger, "%s", license);
}

void print_help(const char *argv0, const struct dyesub_backend *backend)
{
	struct libusb_device **list = NULL;

	const char *ptr = getenv("BACKEND");
	if (!ptr)
		ptr = getenv("DYESUB_BACKEND");
	if (!ptr)
		ptr = argv0;

	if (!backend)
		backend = find_backend(ptr);

	if (!backend) {
		int i;
		DEBUG("Environment variables:\n");
		DEBUG(" DYESUB_DEBUG EXTRA_PID EXTRA_VID EXTRA_TYPE BACKEND SERIAL OLD_URI_SCHEME BACKEND_QUIET\n");
		DEBUG("CUPS Usage:\n");
		DEBUG("\tDEVICE_URI=someuri %s job user title num-copies options [ filename ]\n", ptr);
		DEBUG("\n");
		DEBUG("Standalone Usage:\n");
		DEBUG("\t%s\n", ptr);
		DEBUG("  [ -D ] [ -G ] [ -f ] [ -v ]\n");
		DEBUG("  [ backend_specific_args ] \n");
		DEBUG("  [ -d copies ] \n");
		DEBUG("  [ - | infile ] \n");
		for (i = 0; ; i++) {
			const char **alias;

			backend = backends[i];
			if (!backend)
				break;
			DEBUG("\t# %s version %s\n",
			      backend->name, backend->version);
			DEBUG("  BACKEND=");
			for (alias = backend->uri_prefixes ; alias && *alias ; alias++)
				DEBUG2("%s ", *alias);
			DEBUG2("\n");

			if (backend->cmdline_usage)
				backend->cmdline_usage();
		}
	} else {
		const char **alias;
		int j;
		DEBUG("Standalone %s backend version %s\n",
		      backend->name, backend->version);
		DEBUG("\t supporting: ");
		for (alias = backend->uri_prefixes ; alias && *alias ; alias++)
			DEBUG2("%s ", *alias);
		for (j = 0 ; backend->devices[j].vid ; j++)
			DEBUG2("%s ", backend->devices[j].make);
		DEBUG2("\n");

		DEBUG("\t[ -D ] [ -G ] [ -f ]\n");
		if (backend->cmdline_usage)
			backend->cmdline_usage();
		DEBUG("\t[ -d copies ] [ infile | - ]\n");
	}

	/* Scan for all printers for the specified backend */
	find_and_enumerate(argv0, &list, backend, NULL, ptr, 1, 1, NULL);
	libusb_free_device_list(list, 1);
}

static int parse_cmdstream(struct dyesub_backend *backend, void *backend_ctx, int fd)
{
	FILE *fp = stdin;
	char line[128];
	char *lp;

	if (fd != fileno(stdin)) {
		fp = fdopen(fd, "r");
		if (!fp) {
			ERROR("Can't open data stream!\n");
			return CUPS_BACKEND_FAILED;
		}
	}
	while (fgets(line, sizeof(line), fp) != NULL) {
		/* Strip trailing newline */
		lp = line + strlen(line) - 1;
		if (*lp == '\n')
			*lp = '\0';
		/* And leading spaces */
		for (lp = line; isspace(*lp); lp++);
		/* And comments and blank lines */
		if (*lp == '#' || !*lp)
			continue;

		/* Parse command! */
		if (strncasecmp(lp, "ReportLevels", 12) == 0) {
			query_markers(backend, backend_ctx, 1);
/* XXX TODO: ReportStatus, AutoConfigure, PrintSelfTestPage?  What about others, eg reset or cancel job? */
		} else {
			WARNING("Invalid printer command \"%s\"!\n", lp);
		}
	}

	/* Clean up */
	if (fp != stdin)
		fclose(fp);

	return CUPS_BACKEND_OK;
}

static int handle_input(struct dyesub_backend *backend, void *backend_ctx,
			const char *fname, char *uri, char *type)
{
	int ret = CUPS_BACKEND_OK;
	int i;
	const void *jobs[MAX_JOBS_FROM_READ_PARSE];
	int data_fd = fileno(stdin);
	int read_page = 0, print_page = 0;
	struct dyesub_joblist *jlist = NULL;

	if (!fname) {
		if (uri && strlen(uri))
			ERROR("ERROR: No input file specified\n");
		ret = CUPS_BACKEND_FAILED;
		goto done;
	}

	if (ncopies < 1) {
		ERROR("ERROR: need to have at least 1 copy!\n");
		ret = CUPS_BACKEND_FAILED;
		goto done;
	}

	/* Open file if not STDIN */
	if (strcmp("-", fname)) {
		data_fd = open(fname, O_RDONLY);
		if (data_fd < 0) {
			perror("ERROR:Can't open input file");
			ret = CUPS_BACKEND_FAILED;
			goto done;
		}
	}

#ifndef _WIN32
	/* Ensure we're using BLOCKING I/O */
	i = fcntl(data_fd, F_GETFL, 0);
	if (i < 0) {
		perror("ERROR:Can't open input");
		ret = CUPS_BACKEND_FAILED;
		goto done;
	}
	i &= ~O_NONBLOCK;
	i = fcntl(data_fd, F_SETFL, i);
	if (i < 0) {
		perror("ERROR:Can't open input");
		ret = CUPS_BACKEND_FAILED;
		goto done;
	}

	/* Ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, sigterm_handler);
#endif

	/* See if it's a CUPS command stream, and if yes, handle it! */
	if (type && !strcmp("application/vnd.cups-command", type))
	{
		INFO("CUPS Command mode\n");
		ret = parse_cmdstream(backend, backend_ctx, data_fd);
		goto done;
	}

	/* Time for the main processing loop */
	INFO("Printing started (%d copies)\n", ncopies);

	/* Emit a verbose marker dump */
	ret = query_markers(backend, backend_ctx, 1);
	if (ret)
		goto done;

newpage:
	/* Read in data */
	for (i = 0 ; i < MAX_JOBS_FROM_READ_PARSE ; i++)
		jobs[i] = NULL;

	if ((ret = backend->read_parse(backend_ctx, jobs, data_fd, ncopies))) {
		if (read_page)
			goto done_multiple;
		else
			goto done;
	}

	if (!jobs[0]) {
		WARNING("No job returned by backend read_parse?\n");
		goto newpage;
	}

	/* Create a joblist if needed */
	if (!jlist) {
		jlist = dyesub_joblist_create(backend, backend_ctx);
	}
	if (!jlist) {
		for (i = 0 ; i < MAX_JOBS_FROM_READ_PARSE ; i++)
			backend->cleanup_job(jobs[i]);
		goto done;
	}

	/* Stick jobs onto the end of the list */
	for (i = 0 ; i < MAX_JOBS_FROM_READ_PARSE ; i++) {
		if (jobs[i])
			dyesub_joblist_appendjob(jlist, jobs[i]);
	}
	read_page++;

	INFO("Parsed page %d (%d copies)\n", read_page, ncopies);

	/* If we get here, we can wait for another combined job, do so */
	if (dyesub_joblist_canwait(jlist))
		goto newpage;

print_list:
	/* Print the pagelist */
	ret = dyesub_joblist_print(jlist, &print_page);
	if (ret)
		goto done;

	dyesub_joblist_cleanup(jlist);
	jlist = NULL;

	/* Since we have no way of telling if there's more data remaining
	   to be read (without actually trying to read it), always assume
	   multiple print jobs. */
	goto newpage;

done_multiple:
	if (jlist)
		goto print_list;

	close(data_fd);

	ret = CUPS_BACKEND_OK;

done:
	if (jlist) dyesub_joblist_cleanup(jlist);

	return ret;
}

int main (int argc, char **argv)
{
	struct libusb_device **list = NULL;

	struct dyesub_backend *backend = NULL;
	void * backend_ctx = NULL;

	struct dyesub_connection conn;

	int ret = CUPS_BACKEND_OK;

	int found = -1;
	int jobid = 0;

	char *uri;
	char *type;
	const char *fname = NULL;
	char *use_serno = NULL;
	const char *backend_str = NULL;
	const char *argv0;

	/* Work out path-less executable name */
	argv0 = strrchr(argv[0], '/');
	if (argv0)
		argv0++;
	else
		argv0 = argv[0];

	logger = stderr;

	/* Handle environment variables  */
	if (getenv("BACKEND_QUIET"))
		quiet = atoi(getenv("BACKEND_QUIET"));
	if (getenv("BACKEND_STATS_ONLY"))
		stats_only = atoi(getenv("BACKEND_STATS_ONLY"));
	if (getenv("DYESUB_DEBUG"))
		dyesub_debug = atoi(getenv("DYESUB_DEBUG"));
	if (getenv("EXTRA_PID"))
		extra_pid = strtol(getenv("EXTRA_PID"), NULL, 16);
	if (getenv("EXTRA_VID"))
		extra_vid = strtol(getenv("EXTRA_VID"), NULL, 16);
	if (getenv("EXTRA_TYPE"))
		extra_type = atoi(getenv("EXTRA_TYPE"));
	if (getenv("BACKEND"))
		backend_str = getenv("BACKEND");
	else if (getenv("DYESUB_BACKEND"))
		backend_str = getenv("DYESUB_BACKEND");
	if (getenv("FAST_RETURN"))
		fast_return = atoi(getenv("FAST_RETURN"));
	if (getenv("MAX_XFER_SIZE"))
		max_xfer_size = atoi(getenv("MAX_XFER_SIZE"));
	if (getenv("XFER_TIMEOUT"))
		xfer_timeout = atoi(getenv("XFER_TIMEOUT"));
	if (getenv("TEST_MODE"))
		test_mode = atoi(getenv("TEST_MODE"));
	if (getenv("OLD_URI_SCHEME"))
		old_uri = atoi(getenv("OLD_URI_SCHEME"));
	if (getenv("CORRTABLE_PATH"))
		corrtable_path = getenv("CORRTABLE_PATH");

	if (test_mode >= TEST_MODE_NOATTACH && (extra_vid == -1 || extra_pid == -1)) {
		ERROR("Must specify EXTRA_VID, EXTRA_PID in test mode > 1!\n");
		exit(1);
	}

	if (stats_only && !dyesub_debug)
		quiet = 1;

	DEBUG("Multi-Call Dye-sublimation CUPS Backend version %s\n",
	      BACKEND_VERSION);
	DEBUG("Copyright 2007-2021 Solomon Peachy\n");
	DEBUG("This free software comes with ABSOLUTELY NO WARRANTY! \n");
	DEBUG("Licensed under the GNU GPL.  Run with '-G' for more details.\n");
	DEBUG("\n");

	use_serno = getenv("SERIAL");
	uri = getenv("DEVICE_URI");  /* CUPS backend mode! */
	type = getenv("CONTENT_TYPE"); /* CUPS content type -- ie raster or command */

	if (uri && strlen(uri)) {  /* CUPS backend mode */
		int base = optind; /* ie 1 */
		if (argc < 6) {
			ERROR("Insufficient arguments\n");
			exit(1);
		}
		if (argv[base])
			jobid = atoi(argv[base]);
		if (argv[base + 3])
			ncopies = atoi(argv[base + 3]);
		if (argc > 6)
			fname = argv[base + 5];
		else
			fname = "-";

		/* Figure out backend based on URI */
		{
			char *ptr2;
			backend_str = strstr(uri, "backend=");
			if (backend_str) { /* Original format */
				backend_str += 8;
				ptr2 = strchr(backend_str, '&');
				if (ptr2) {
					use_serno = strstr(ptr2, "serial=");
					*ptr2 = 0;
				}

				if (!use_serno)
					use_serno = strstr(uri, "serial=");

				if (!use_serno || !*(use_serno+7)) {
					ERROR("Invalid URI (%s)\n", uri);
					exit(1);
				}
				use_serno += 7;

			} else { /* New format */
				// prefix://backend/serno
				backend_str = strchr(uri, '/');
				backend_str += 2;
				use_serno = strchr(backend_str, '/');
				if (!use_serno || !*(use_serno+1)) {
					ERROR("Invalid URI (%s)\n", uri);
					exit(1);
				}
				*use_serno = 0;
				use_serno++;
			}

			if (use_serno) {
				ptr2 = strchr(use_serno, '&');
				if (ptr2)
					*ptr2 = 0;
			}
		}

		/* Enable fast return in CUPS if it's not supplied */
		if (!getenv("FAST_RETURN"))
			fast_return++;

	} else {  /* Standalone mode */

		/* Try to guess backend from executable name */
		if (!backend_str)
			backend_str = argv0;

		srand(getpid());
		jobid = rand();
	}

	/* Finally, look up the backend */
	backend = find_backend(backend_str);
	if (!backend) {
		if (uri && strlen(uri)) {
			ERROR("Invalid backend requested (%s)\n", backend_str);
			exit(1);
		}
		backend_str = NULL;
	}

#ifndef LIBUSB_PRE_1_0_10
	if (dyesub_debug) {
		const struct libusb_version *ver;
		ver = libusb_get_version();
		DEBUG(" ** running with libusb %d.%d.%d%s (%d)\n",
		      ver->major, ver->minor, ver->micro, (ver->rc? ver->rc : ""), ver->nano );
	}
#endif

	/* Libusb setup */
	ret = libusb_init(NULL);
	if (ret) {
		ERROR("Failed to initialize libusb (%d)\n", ret);
		ret = CUPS_BACKEND_RETRY_CURRENT;
		goto done;
	}

	/* If we don't have a valid backend, print help and terminate */
	if (!backend && !stats_only) {
		print_help(argv0, NULL); // probes all devices
		ret = CUPS_BACKEND_OK;
		goto done;
	}

	/* If we're in standalone mode, print help only if no args */
	if ((!uri || !strlen(uri)) && !stats_only) {
		if (argc < 2) {
			print_help(argv0, backend); // probes all devices
			ret = CUPS_BACKEND_OK;
			goto done;
		}
	}

	/* Enumerate devices */
	STATE("+connecting-to-device\n");

	found = find_and_enumerate(argv0, &list, backend, use_serno, backend_str, 0, NUM_CLAIM_ATTEMPTS, &conn);

	if (found == -1) {
		ERROR("Printer open failure (No matching printers found!)\n");
		STATE("+offline-report\n");
		ret = CUPS_BACKEND_RETRY;
		goto done;
	}
	STATE("-offline-report\n");

	if (test_mode) {
		WARNING("**** TEST MODE %d!\n", test_mode);
		if (test_mode >= TEST_MODE_NOATTACH)
			goto bypass;
	}

	/* Open an appropriate device */
	ret = libusb_open(list[found], &conn.dev);
	if (ret) {
		ERROR("Printer open failure (Need to be root?) (%d)\n", ret);
		ret = CUPS_BACKEND_RETRY_CURRENT;
		goto done;
	}

	/* Detach the kernel driver */
	if (libusb_kernel_driver_active(conn.dev, conn.iface)) {
		ret = libusb_detach_kernel_driver(conn.dev, conn.iface);
		if (ret && (ret != LIBUSB_ERROR_NOT_SUPPORTED)) {
			ERROR("Printer open failure (Could not detach printer from kernel) (%d)\n", ret);
			ret = CUPS_BACKEND_RETRY_CURRENT;
			goto done_close;
		}
	}

	/* Claim the interface so we can start using this! */
	ret = backend_claim_interface(conn.dev, conn.iface, NUM_CLAIM_ATTEMPTS);
	if (ret) {
		ERROR("Printer open failure (Unable to claim interface) (%d)\n", ret);
		ret = CUPS_BACKEND_RETRY;
		goto done_close;
	}

	/* Use the appropriate altesetting! */
	if (conn.altset != 0) {
		ret = libusb_set_interface_alt_setting(conn.dev, conn.iface, conn.altset);
		if (ret) {
			ERROR("Printer open failure (Unable to issue altsettinginterface) (%d)\n", ret);
			ret = CUPS_BACKEND_RETRY;
			goto done_claimed;
		}
	}

bypass:
	STATE("-connecting-to-device\n");

	/* Initialize backend */
	DEBUG("Initializing '%s' backend (version %s)\n",
	      backend->name, backend->version);
	backend_ctx = backend->init();

	if (test_mode < TEST_MODE_NOATTACH) {
		struct libusb_device *device;
		struct libusb_device_descriptor desc;

		device = libusb_get_device(conn.dev);
		libusb_get_device_descriptor(device, &desc);

		conn.type = lookup_printer_type(backend,
						desc.idVendor, desc.idProduct);
		conn.usb_vid = desc.idVendor;
		conn.usb_pid = desc.idProduct;
	} else {
		conn.type = lookup_printer_type(backend,
						extra_vid, extra_pid);
		conn.usb_vid = extra_vid;
		conn.usb_pid = extra_pid;
	}

	if (conn.type <= P_UNKNOWN) {
		ERROR("Unable to lookup printer type\n");
		ret = CUPS_BACKEND_FAILED;
		goto done_claimed;
	}

	/* Attach backend to device */ // XXX pass backend_str?
	ret = backend->attach(backend_ctx, &conn, jobid);
	if (ret) {
		ERROR("Unable to attach to printer!\n");
		ret = CUPS_BACKEND_FAILED;
		goto done_claimed;
	}

	/* Dump stats only */
	if (stats_only && backend->query_stats) {
		struct printerstats stats;
		memset(&stats, 0, sizeof(stats));

		stats.timestamp = time(NULL);
		ret = backend->query_stats(backend_ctx, &stats);
		if (ret)
			goto done_claimed;
		dump_stats(backend, &stats, stats_only -1);
		for (int i = 0 ; i < stats.decks ; i++) {
			if (stats.status[i])
				free(stats.status[i]); // the only dynamic member..
		}
		goto done_claimed;
	}

	PPD("StpUsbBackend=\"%s\"\n", backend_str ? backend_str : backend->name);
	PPD("StpUsbVid=%04x\n", conn.usb_vid);
	PPD("StpUsbPid=%04x\n", conn.usb_pid);
	PPD("StpUsbBus=%03d\n", conn.bus_num);
	PPD("StpUsbPort=%03d\n", conn.port_num);

	if (!uri || !strlen(uri)) {
		if (backend->cmdline_arg(backend_ctx, argc, argv))
			goto done_claimed;

		/* Grab the filename */
		fname = argv[optind]; // XXX do this a smarter way?
	}

	/* Parse the file passed in */
	ret = handle_input(backend, backend_ctx, fname, uri, type);

done_claimed:
	if (test_mode < TEST_MODE_NOATTACH)
		libusb_release_interface(conn.dev, conn.iface);

done_close:
	if (test_mode < TEST_MODE_NOATTACH)
		libusb_close(conn.dev);
done:

	STATE("-connecting-to-device\n");

	if (backend && backend_ctx) {
		if (backend->teardown)
			backend->teardown(backend_ctx);
		else
			generic_teardown(backend_ctx);
	}

	if (list)
		libusb_free_device_list(list, 1);

	libusb_exit(NULL);

	return ret;
}

void dump_markers(const struct marker *markers, int marker_count, int full)
{
	int i;

	if (!full)
		goto minimal;

	ATTR("marker-colors=");
	for (i = 0 ; i < marker_count; i++) {
		DEBUG2("%s", markers[i].color);
		if ((i+1) < marker_count)
			DEBUG2(",");
	}
	DEBUG2("\n");

	ATTR("marker-high-levels=");
	for (i = 0 ; i < marker_count; i++) {
		DEBUG2("%d", 100);
		if ((i+1) < marker_count)
			DEBUG2(",");
	}
	DEBUG2("\n");

	ATTR("marker-low-levels=");
	for (i = 0 ; i < marker_count; i++) {
		DEBUG2("%d", 10);
		if ((i+1) < marker_count)
			DEBUG2(",");
	}
	DEBUG2("\n");

	ATTR("marker-names=");
	for (i = 0 ; i < marker_count; i++) {
		DEBUG2("'\"%s\"'", markers[i].name);
		if ((i+1) < marker_count)
			DEBUG2(",");
	}
	DEBUG2("\n");

	ATTR("marker-types=");
	for (i = 0 ; i < marker_count; i++) {
		DEBUG2("ribbonWax");
		if ((i+1) < marker_count)
			DEBUG2(",");
	}
	DEBUG2("\n");

minimal:
	ATTR("marker-levels=");
	for (i = 0 ; i < marker_count; i++) {
		int val;
		if (markers[i].levelmax <= 0 || markers[i].levelnow < 0)
			val = (markers[i].levelnow <= 0) ? markers[i].levelnow : CUPS_MARKER_UNAVAILABLE;
		else if (markers[i].levelmax == 100)
			val = markers[i].levelnow;
		else
			val = markers[i].levelnow * 100 / markers[i].levelmax;
		DEBUG2("%d", val);
		if ((i+1) < marker_count)
			DEBUG2(",");
	}
	DEBUG2("\n");

	/* Only dump a message if the marker is not a percentage */
	if (markers[0].levelmax != 100) {
		ATTR("marker-message=");
		for (i = 0 ; i < marker_count; i++) {
			switch (markers[i].levelnow) {
			case CUPS_MARKER_UNAVAILABLE:
				DEBUG2("'\"Unable to query remaining prints on %s media\"'", markers[i].name);
				break;
			case CUPS_MARKER_UNKNOWN:
				DEBUG2("'\"Unknown remaining prints on %s media\"'", markers[i].name);
				break;
			case CUPS_MARKER_UNKNOWN_OK:
				DEBUG2("'\"One or more remaining prints on %s media\"'", markers[i].name);
				break;
			default:
				DEBUG2("'\"%d native prints remaining on %s media\"'", markers[i].levelnow, markers[i].name);
				break;
			}
			if ((i+1) < marker_count)
				DEBUG2(",");
		}
		DEBUG2("\n");
	}

	if (markers[0].levelnow == 0)
		STATE("+media-empty\n");
	else if (markers[0].levelnow > 0 || markers[0].levelnow == CUPS_MARKER_UNKNOWN_OK)
		STATE("-media-empty\n");

	/* If we're running as a CUPS backend, report the media type */
	if (full && getenv("DEVICE_URI")) {
		for (i = 0 ; i < marker_count ; i++) {
			PPD("StpMediaID%d=%d\n", i, markers[i].numtype);
			PPD("StpMediaName%d=\"%s\"\n", i, markers[i].name);
		}
	}
}

int dyesub_read_file(const char *filename, void *databuf, int datalen,
		     int *actual_len)
{
	int len;
	int fd = open(filename, O_RDONLY);

	if (fd < 0) {
		ERROR("Unable to open '%s'\n", filename);
		return CUPS_BACKEND_FAILED;
	}
	len = read(fd, databuf, datalen);
	if (len < 0) {
		ERROR("Bad Read! (%d/%d)\n", len, errno);
		close(fd);
		return CUPS_BACKEND_FAILED;
	}
	if (!actual_len && (datalen != len)) {
		ERROR("Read mismatch (%d vs %d)\n", len, datalen);
		close(fd);
		return CUPS_BACKEND_FAILED;
	}
	close(fd);

	if (actual_len)
		*actual_len = len;

	return CUPS_BACKEND_OK;
}

uint16_t uint16_to_packed_bcd(uint16_t val)
{
        uint16_t bcd;
        uint16_t i;

        /* Handle from 0-9999 */
        i = val % 10;
        bcd = i;
        val /= 10;
        i = val % 10;
        bcd |= (i << 4);
        val /= 10;
        i = val % 10;
        bcd |= (i << 8);
        val /= 10;
        i = val % 10;
        bcd |= (i << 12);

        return bcd;
}

uint32_t packed_bcd_to_uint32(const char *in, int len)
{
	uint32_t out = 0;

	while (len--) {
		out *= 10;
		out += ((*in >> 4) & 0xf);
		out *= 10;
		out += (*in & 0xf);
		in++;
	}
        return out;
}

/* Job list manipulation */
struct dyesub_joblist *dyesub_joblist_create(const struct dyesub_backend *backend, void *ctx)
{
	struct dyesub_joblist *list;

	list = malloc(sizeof(struct dyesub_joblist));
	if (!list) {
		ERROR("Memory allocation failure\n");
		return NULL;
	}
	list->backend = backend;
	list->ctx = ctx;
	list->num_entries = 0;

	if (collate)
		list->copies = ncopies;
	else
		list->copies = 1;

	return list;
}

void dyesub_joblist_cleanup(const struct dyesub_joblist *list)
{
	int i;
	for (i = 0; i < list->num_entries ; i++) {
		if (list->entries[i])
			list->backend->cleanup_job(list->entries[i]);
	}
	free((void*)list);
}

static int __dyesub_joblist_addjob(struct dyesub_joblist *list, const void *job)
{
	if (list->num_entries >= DYESUB_MAX_JOB_ENTRIES)
		return 1;

	list->entries[list->num_entries++] = job;

	return CUPS_BACKEND_OK;
}

static int __dyesub_append_job(struct dyesub_joblist *list, const void **vjob, int polarity)
{
	struct dyesub_job_common *job, *combined;
	const struct dyesub_job_common *oldjob;

	/* Create writable copy of the new job */
	job = malloc(((const struct dyesub_job_common *)*vjob)->jobsize);
	if (!job) {
		ERROR("Memory allocation failure!\n");
		return CUPS_BACKEND_RETRY_CURRENT;
	}
	memcpy(job, *vjob, ((const struct dyesub_job_common *)*vjob)->jobsize);

	/* If we can't combine the new job, don't bother doing anything else */
	if (!job->can_combine) {
		free(job);
		return CUPS_BACKEND_OK;
	}

	/* Get the old job */
	oldjob = dyesub_joblist_popjob(list);

	/* If we can't combine with it, carry on as before */
	if (oldjob && (oldjob->copies > 1 || !oldjob->can_combine)) {
		__dyesub_joblist_addjob(list, oldjob);
		oldjob = NULL;
	}

	if (oldjob) {
		/* Try to combine first copy with old job */
		combined = list->backend->combine_jobs(oldjob, job);
		if (combined) {
			INFO("Successfully combined two jobs\n");
			/* Success, add it to the list */
	                __dyesub_joblist_addjob(list, combined);
			combined = NULL;

			/* Clean up the old job */
			list->backend->cleanup_job(oldjob);

			/* Anything left in the new job? */
			job->copies--;
			if (job->copies == 0) {
				/* Nope, we're done */
				list->backend->cleanup_job(job);
			} else if (job->copies == 1) {
				/* Just one, shove it on the list */
				__dyesub_joblist_addjob(list, job);
			}

			goto done;
		} else {
			/* Failed to combine, restore old job and continue */
			__dyesub_joblist_addjob(list, oldjob);
		}

		polarity = 0;
	}

	/* If we have no work to do, just return */
	if (job->copies == 1) {
		free(job);
		return CUPS_BACKEND_OK;
	}

	/* Attempt to combine multiple copies! */
	combined = list->backend->combine_jobs(job, job);

	if (!combined) {
		/* Failed, so return */
		free(job);
		return CUPS_BACKEND_OK;
	}

	INFO("Successfully combined multiple copies\n");
	combined->copies = job->copies / 2;
	job->copies = job->copies % 2;

	/* Add the combined job at start if we can */
	if (!polarity) {
		__dyesub_joblist_addjob(list, combined);
	}

	/* Add the remainder, if any */
	if (job->copies) {
		__dyesub_joblist_addjob(list, job);
	} else {
		list->backend->cleanup_job(job);
	}

	/* Add combined job at end of we need to */
	if (polarity) {
		__dyesub_joblist_addjob(list, combined);
	}

done:
	/* Clean up */
	free((void*)*vjob);
	*vjob = NULL;

	return CUPS_BACKEND_OK;
}

int dyesub_joblist_appendjob(struct dyesub_joblist *list, const void *job)
{
	if (list->backend->combine_jobs) {
		int polarity = 0;
		if (list->backend->job_polarity)
			polarity = list->backend->job_polarity(list->ctx);
		__dyesub_append_job(list, &job, polarity);
	}

	if (job)
		__dyesub_joblist_addjob(list, job);

	return CUPS_BACKEND_OK;
}

const void *dyesub_joblist_popjob(struct dyesub_joblist *list)
{
	if (list->num_entries) {
		return list->entries[--list->num_entries];
	}

	return NULL;
}

int dyesub_pano_split_rgb8(const uint8_t *src, uint16_t cols,
			   uint16_t src_rows, uint8_t numpanels,
			   uint16_t overlap_rows, uint16_t max_rows,
			   uint8_t *panels[3],
			   uint16_t panel_rows[3])
{
	/* Do nothing if there's no point */
	if (numpanels < 2 || src_rows <= max_rows)
		return CUPS_BACKEND_OK;

	/* Work out panel sizes if not specified */
	if (panel_rows[0] == 0) {
		panel_rows[0] = max_rows;
		panel_rows[1] = src_rows - panel_rows[0] + overlap_rows;
		if (numpanels > 2)
			panel_rows[2] = src_rows - panel_rows[0] - panel_rows[1] + overlap_rows*2;
	}

	/* Copy panel data */
	memcpy(panels[0], src, cols * panel_rows[0] * 3);
	memcpy(panels[1], src + (panel_rows[0] - overlap_rows) * 3, cols * panel_rows[1] * 3);
	if (numpanels > 2)
		memcpy(panels[2], src + (panel_rows[0] - overlap_rows + panel_rows[1] - overlap_rows) * 3, cols * panel_rows[2] * 3);

	return CUPS_BACKEND_OK;
}

int dyesub_joblist_canwait(struct dyesub_joblist *list)
{
	if (list->num_entries == DYESUB_MAX_JOB_ENTRIES)
		return 0;
	if (!list->num_entries)
		return 1;

	return ((const struct dyesub_job_common *)(list->entries[list->num_entries - 1]))->can_combine;
}

int dyesub_joblist_print(const struct dyesub_joblist *list, int *pagenum)
{
	int i, j;
	int ret;
//	int pages = 0;

	for (i = 0 ; i < list->copies ; i++) {
		for (j = 0 ; j < list->num_entries ; j++) {
			int wait_on_return = 0;
			if (i == (list->copies - 1) && j == (list->num_entries -1))
				wait_on_return = !fast_return; /* only wait on the final copy/page. */

			if (list->entries[j]) {
				int copies = ((const struct dyesub_job_common *)(list->entries[j]))->copies;

				INFO("Printing page %d (%d copies)\n", ++(*pagenum), copies);
				if (test_mode >= TEST_MODE_NOPRINT )
					WARNING("**** TEST MODE, bypassing printing!\n");

				/* Print this page */
				if (test_mode < TEST_MODE_NOPRINT ||
				    list->backend->flags & BACKEND_FLAG_DUMMYPRINT) {
					ret = list->backend->main_loop(list->ctx, list->entries[j], wait_on_return);
					if (ret)
						return ret;
				}

//				pages += copies;

				/* Dump a marker status update */
				ret = query_markers(list->backend, list->ctx, 0);
				if (ret)
					return ret;
#if 0
				/* Free up the job as we go along
				   if we're on the final copy */
				if (i + 1 == list->copies) {
					list->backend->cleanup_job(list->entries[j]);
					list->entries[j] = NULL;
				}
#endif
			}
		}
	}

//	INFO("Printed %d total pages/copies\n", pages);

	return CUPS_BACKEND_OK;
}
