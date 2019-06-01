/*
 *   CUPS Backend common code
 *
 *   Copyright (c) 2007-2019 Solomon Peachy <pizza@shaftnet.org>
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

#include "backend_common.h"
#include <errno.h>

#define BACKEND_VERSION "0.94G"
#ifndef URI_PREFIX
#error "Must Define URI_PREFIX"
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
int old_uri = 0;
int quiet = 0;

static int max_xfer_size = URB_XFER_SIZE;
static int xfer_timeout = XFER_TIMEOUT;

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
static char *get_device_id(struct libusb_device_handle *dev, int iface)
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
struct deviceid_dict {
	char *key;
	char *val;
};

#define MAX_DICT 32

static int parse1284_data(const char *device_id, struct deviceid_dict* dict)
{
	char *ptr;
	char key[256];
	char val[256];
	int num = 0;

	if (!device_id)
		return 0;

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
		if (!*device_id)
			break;
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

static char *dict_find(const char *key, int dlen, struct deviceid_dict* dict)
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
int read_data(struct libusb_device_handle *dev, uint8_t endp,
	      uint8_t *buf, int buflen, int *readlen)
{
	int ret;

	/* Clear buffer */
	memset(buf, 0, buflen);

	ret = libusb_bulk_transfer(dev, endp,
				   buf,
				   buflen,
				   readlen,
				   xfer_timeout);

	if (ret < 0) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, *readlen, buflen, endp);
		goto done;
	}

	if (dyesub_debug) {
		DEBUG("Received %d bytes from printer\n", *readlen);
	}

	if ((dyesub_debug > 1 && buflen < 4096) ||
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

int send_data(struct libusb_device_handle *dev, uint8_t endp,
	      const uint8_t *buf, int len)
{
	int num = 0;

	if (dyesub_debug) {
		DEBUG("Sending %d bytes to printer\n", len);
	}

	while (len) {
		int len2 = (len > max_xfer_size) ? max_xfer_size: len;
		int ret = libusb_bulk_transfer(dev, endp,
					       (uint8_t*) buf, len2,
					       &num, xfer_timeout);

		if ((dyesub_debug > 1 && len < 4096) ||
		    dyesub_debug > 2) {
			int i = num;

			DEBUG("-> ");
			while(i > 0) {
				if ((num-i) != 0 &&
				    (num-i) % 16 == 0) {
					DEBUG2("\n");
					DEBUG("   ");
				}
				DEBUG2("%02x ", buf[num-i]);
				i--;
			}
			DEBUG2("\n");
		}

		if (ret < 0) {
			ERROR("Failure to send data to printer (libusb error %d: (%d/%d to 0x%02x))\n", ret, num, len2, endp);
			return ret;
		}
		len -= num;
		buf += num;
	}

	return 0;
}

/* More stuff */
static void sigterm_handler(int signum) {
	UNUSED(signum);

	terminate = 1;
	INFO("Job Cancelled");
}

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

   http://www.geekhideout.com/urlcode.shtml

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
static char *url_encode(char *str) {
	char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;

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
			const char *uri_prefix,
			const char *prefix, char *manuf_override,
			int found, int num_claim_attempts,
			int scan_only, char *match_serno,
			uint8_t *r_iface, uint8_t *r_altset,
			uint8_t *r_endp_up, uint8_t *r_endp_down,
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
	STATE("+connecting-to-device\n");

	if (libusb_open(device, &dev)) {
		ERROR("Could not open device %04x:%04x (need to be root?)\n", desc->idVendor, desc->idProduct);
		found = -1;
		goto abort;
	}

	/* XXX FIXME:  Iterate through possible configurations? */
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

#if 1
			/* Explicitly exclude IPP-over-USB interfaces */
			if (desc->bDeviceClass == LIBUSB_CLASS_PER_INTERFACE &&
			    config->interface[iface].altsetting[altset].bInterfaceClass == LIBUSB_CLASS_PRINTER &&
			    config->interface[iface].altsetting[altset].bInterfaceSubClass == USB_SUBCLASS_PRINTER &&
			    config->interface[iface].altsetting[altset].bInterfaceProtocol == USB_INTERFACE_PROTOCOL_IPP) {
				continue;
			}
#else
			// Make sure it's a printer class device that supports bidir comms  (XXX Is this necessarily true?)
			if (desc->bDeviceClass == LIBUSB_CLASS_PRINTER ||
			    (desc->bDeviceClass == LIBUSB_CLASS_PER_INTERFACE &&
			     config->interface[iface].altsetting[altset].bInterfaceClass == LIBUSB_CLASS_PRINTER &&
			     config->interface[iface].altsetting[altset].bInterfaceSubClass == USB_SUBCLASS_PRINTER &&
			     config->interface[iface].altsetting[altset].bInterfaceProtocol != USB_INTERFACE_PROTOCOL_BIDIR)) {
				continue;
			}
#endif

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

	/* Look up mfg string. */
	if (manuf_override && strlen(manuf_override)) {
		manuf = url_encode(manuf_override);  /* Backend supplied */
	} else if ((manuf = dict_find("MANUFACTURER", dlen, dict))) {
		manuf = url_encode(manuf);
	} else if ((manuf = dict_find("MFG", dlen, dict))) {
		manuf = url_encode(manuf);
	} else if ((manuf = dict_find("MFR", dlen, dict))) {
		manuf = url_encode(manuf);
	} else if (desc->iManufacturer) { /* Get from USB descriptor */
		buf[0] = 0;
		libusb_get_string_descriptor_ascii(dev, desc->iManufacturer, (unsigned char*)buf, STR_LEN_MAX);
		sanitize_string(buf);
		manuf = url_encode(buf);
	}
	if (!manuf || !strlen(manuf)) {  /* Last-ditch */
		if (manuf) free(manuf);
		manuf = url_encode("Unknown"); // XXX use USB VID?
	}

	/* Look up model number */
	if ((product = dict_find("MODEL", dlen, dict))) {
		product = url_encode(product);
	} else if ((product = dict_find("MDL", dlen, dict))) {
		product = url_encode(product);
	} else if (desc->iProduct) {  /* Get from USB descriptor */
		buf[0] = 0;
		libusb_get_string_descriptor_ascii(dev, desc->iProduct, (unsigned char*)buf, STR_LEN_MAX);
		sanitize_string(buf);
		product = url_encode(buf);
	}

	if (!product || !strlen(product)) { /* Last-ditch */
		if (!product) free(product);
		product = url_encode("Unknown"); // XXX Use USB PID?
	}

	/* Look up description */
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

	/* Look up serial number */
	if ((serial = dict_find("SERIALNUMBER", dlen, dict))) {
		serial = url_encode(serial);
	} else if ((serial = dict_find("SN", dlen, dict))) {
		serial = url_encode(serial);
	} else if ((serial = dict_find("SER", dlen, dict))) {
		serial = url_encode(serial);
	} else if ((serial = dict_find("SERN", dlen, dict))) {
		serial = url_encode(serial);
	} else if (desc->iSerialNumber) {  /* Get from USB descriptor */
		libusb_get_string_descriptor_ascii(dev, desc->iSerialNumber, (unsigned char*)buf, STR_LEN_MAX);
		sanitize_string(buf);
		serial = url_encode(buf);
	} else if (backend->query_serno) { /* Get from backend hook */
		buf[0] = 0;
		/* Ignore result since a failure isn't critical here */
		backend->query_serno(dev, endp_up, endp_down, buf, STR_LEN_MAX);
		serial = url_encode(buf);
	}

	if (!serial || !strlen(serial)) {  /* Last-ditch */
		if (serial) free(serial);
		WARNING("**** THIS PRINTER DOES NOT REPORT A SERIAL NUMBER!\n");
		WARNING("**** If you intend to use multiple printers of this type, you\n");
		WARNING("**** must only plug one in at a time or unexpected behavior will occur!\n");
		serial = strdup("NONE_UNKNOWN");
	}

	if (scan_only) {
		if (!old_uri) {
			fprintf(stdout, "direct %s://%s/%s \"%s\" \"%s\" \"%s\" \"\"\n",
				prefix, uri_prefix, serial,
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
				prefix, buf, serial, uri_prefix,
				descr, descr,
				ieee_id? ieee_id : "");
		}
	}

	/* If a serial number was passed down, use it. */
	if (match_serno && strcmp(match_serno, (char*)serial)) {
		found = -1;
	}

	if (dyesub_debug)
		DEBUG("VID: %04X PID: %04X Manuf: '%s' Product: '%s' Serial: '%s' found: %d\n",
		      desc->idVendor, desc->idProduct, manuf, product, serial, found);

	if (found != -1) {
		if (r_iface) *r_iface = iface;
		if (r_altset) *r_altset = altset;
		if (r_endp_up) *r_endp_up = endp_up;
		if (r_endp_up) *r_endp_down = endp_down;
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

	STATE("-connecting-to-device\n");

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

static struct dyesub_backend *backends[] = {
	&canonselphy_backend,
	&canonselphyneo_backend,
	&kodak6800_backend,
	&kodak605_backend,
	&kodak1400_backend,
	&shinkos1245_backend,
	&shinkos2145_backend,
	&shinkos6145_backend,
	&shinkos6245_backend,
	&sonyupd_backend,
	&mitsu70x_backend,
	&mitsud90_backend,
	&mitsu9550_backend,
	&mitsup95d_backend,
	&dnpds40_backend,
	&magicard_backend,
	NULL,
};

static int find_and_enumerate(struct libusb_context *ctx,
			      struct libusb_device ***list,
			      struct dyesub_backend *backend,
			      char *match_serno,
			      int scan_only, int num_claim_attempts,
			      uint8_t *r_iface, uint8_t *r_altset,
			      uint8_t *r_endp_up, uint8_t *r_endp_down)
{
	int num;
	int i, j = 0, k;
	int found = -1;
	const char *prefix = NULL;

	if (test_mode >= TEST_MODE_NOATTACH) {
		found = 1;
		*r_endp_up = 0x82;
		*r_endp_down = 0x01;
		*r_iface = 0;
		*r_altset = 0;
		return found;
	}

	STATE("+org.gutenprint-searching-for-device\n");

	/* Enumerate and find suitable device */
	num = libusb_get_device_list(ctx, list);

	for (i = 0 ; i < num ; i++) {
		struct libusb_device_descriptor desc;
		libusb_get_device_descriptor((*list)[i], &desc);

		for (k = 0 ; backends[k] ; k++) {
			if (backend && backend != backends[k])
				continue;
			for (j = 0 ; backends[k]->devices[j].vid ; j++) {
				if (extra_pid != -1 &&
				    extra_vid != -1 &&
				    extra_type != -1) {
					if (backends[k]->devices[j].type == extra_type &&
					    extra_vid == desc.idVendor &&
					    extra_pid == desc.idProduct) {
						found = i;
						prefix = backends[k]->uri_prefixes[0];
						goto match;
					}
				}
				if (desc.idVendor == backends[k]->devices[j].vid &&
				    (desc.idProduct == backends[k]->devices[j].pid ||
				     desc.idProduct == 0xffff)) {
					prefix = backends[k]->devices[j].prefix;
					found = i;
					goto match;
				}
			}
		}

		continue;

	match:
		found = probe_device((*list)[i], &desc, prefix,
				     URI_PREFIX, backends[k]->devices[j].manuf_str,
				     found, num_claim_attempts,
				     scan_only, match_serno,
				     r_iface, r_altset,
				     r_endp_up, r_endp_down,
				     backends[k]);

		if (found != -1 && !scan_only)
			break;
	}

	STATE("-org.gutenprint-searching-for-device\n");
	return found;
}

static struct dyesub_backend *find_backend(char *uri_prefix)
{
	int i;

	if (!uri_prefix)
		return NULL;

	for (i = 0; ; i++) {
		struct dyesub_backend *backend = backends[i];
		const char **alias;
		if (!backend)
			return NULL;
		for (alias = backend->uri_prefixes ; alias && *alias ; alias++) {
			if (!strcmp(uri_prefix, *alias))
				return backend;
		}
	}
	return NULL;
}

static int query_markers(struct dyesub_backend *backend, void *ctx, int full)
{
	struct marker *markers = NULL;
	int marker_count = 0;
	int ret;

	if (!backend->query_markers)
		return CUPS_BACKEND_OK;

	if (test_mode >= TEST_MODE_NOPRINT)
		return CUPS_BACKEND_OK;

	ret = backend->query_markers(ctx, &markers, &marker_count);
	if (ret)
		return ret;

	dump_markers(markers, marker_count, full);

	return CUPS_BACKEND_OK;
}

void print_license_blurb(void)
{
	const char *license = "\n\
Copyright 2007-2019 Solomon Peachy <pizza AT shaftnet DOT org>\n\
\n\
This program is free software; you can redistribute it and/or modify it\n\
under the terms of the GNU General Public License as published by the Free\n\
Software Foundation; either version 3 of the License, or (at your option)\n\
any later version.\n\
\n\
This program is distributed in the hope that it will be useful, but\n\
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY\n\
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License\n\
for more details.\n\
\n\
You should have received a copy of the GNU General Public License\n\
along with this program.  If not, see https://www.gnu.org/licenses/.\n\
\n          [http://www.gnu.org/licenses/gpl-2.0.html]\n\n";

	fprintf(stderr, "%s", license);
}

void print_help(char *argv0, struct dyesub_backend *backend)
{
	struct libusb_context *ctx = NULL;
	struct libusb_device **list = NULL;

	char *ptr = strrchr(argv0, '/');
	if (ptr)
		ptr++;
	else
		ptr = argv0;

	if (!backend)
		backend = find_backend(ptr);

	if (!backend) {
		int i;
		DEBUG("Environment variables:\n");
		DEBUG(" DYESUB_DEBUG EXTRA_PID EXTRA_VID EXTRA_TYPE BACKEND SERIAL OLD_URI_SCHEME BACKEND_QUIET\n");
		DEBUG("CUPS Usage:\n");
		DEBUG("\tDEVICE_URI=someuri %s job user title num-copies options [ filename ]\n", URI_PREFIX);
		DEBUG("\n");
		DEBUG("Standalone Usage:\n");
		DEBUG("\t%s\n", URI_PREFIX);
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
		DEBUG("Standalone %s backend version %s\n",
		      backend->name, backend->version);
		DEBUG("\t supporting: ");
		for (alias = backend->uri_prefixes ; alias && *alias ; alias++)
			DEBUG2("%s ", *alias);
		DEBUG2("\n");

		DEBUG("\t[ -D ] [ -G ] [ -f ]\n");
		if (backend->cmdline_usage)
			backend->cmdline_usage();
		DEBUG("\t[ -d copies ] [ infile | - ]\n");
	}

	/* Probe for printers */
	find_and_enumerate(ctx, &list, backend, NULL, 1, 1, NULL, NULL, NULL, NULL);
	libusb_free_device_list(list, 1);
}

int parse_cmdstream(struct dyesub_backend *backend, void *backend_ctx, int fd)
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
};

int main (int argc, char **argv)
{
	struct libusb_context *ctx = NULL;
	struct libusb_device **list = NULL;
	struct libusb_device_handle *dev;

	struct dyesub_backend *backend = NULL;
	void * backend_ctx = NULL;

	uint8_t endp_up, endp_down;
	uint8_t iface, altset;

	int data_fd = fileno(stdin);

	const void *job = NULL;

	int i;

	int ret = CUPS_BACKEND_OK;

	int found = -1;
	int jobid = 0;
	int current_page = 0;

	char *uri;
	char *type;
	char *fname = NULL;
	char *use_serno = NULL;
	int  printer_type;

	/* Handle environment variables  */
	if (getenv("BACKEND_QUIET"))
		quiet = atoi(getenv("BACKEND_QUIET"));
	if (getenv("DYESUB_DEBUG"))
		dyesub_debug = atoi(getenv("DYESUB_DEBUG"));
	if (getenv("EXTRA_PID"))
		extra_pid = strtol(getenv("EXTRA_PID"), NULL, 16);
	if (getenv("EXTRA_VID"))
		extra_vid = strtol(getenv("EXTRA_VID"), NULL, 16);
	if (getenv("EXTRA_TYPE"))
		extra_type = atoi(getenv("EXTRA_TYPE"));
	if (getenv("BACKEND"))
		backend = find_backend(getenv("BACKEND"));
	if (getenv("FAST_RETURN"))
		fast_return++;
	if (getenv("MAX_XFER_SIZE"))
		max_xfer_size = atoi(getenv("MAX_XFER_SIZE"));
	if (getenv("XFER_TIMEOUT"))
		xfer_timeout = atoi(getenv("XFER_TIMEOUT"));
	if (getenv("TEST_MODE"))
		test_mode = atoi(getenv("TEST_MODE"));
	if (getenv("OLD_URI_SCHEME"))
		old_uri = atoi(getenv("OLD_URI_SCHEME"));

	if (test_mode >= TEST_MODE_NOATTACH && (extra_vid == -1 || extra_pid == -1)) {
		ERROR("Must specify EXTRA_VID, EXTRA_PID in test mode > 1!\n");
		exit(1);
	}

	DEBUG("Multi-Call Dye-sublimation CUPS Backend version %s\n",
	      BACKEND_VERSION);
	DEBUG("Copyright 2007-2019 Solomon Peachy\n");
	DEBUG("This free software comes with ABSOLUTELY NO WARRANTY! \n");
	DEBUG("Licensed under the GNU GPL.  Run with '-G' for more details.\n");
	DEBUG("\n");

	use_serno = getenv("SERIAL");
	uri = getenv("DEVICE_URI");  /* CUPS backend mode! */
	type = getenv("FINAL_CONTENT_TYPE"); /* CUPS content type -- ie raster or command */

	if (uri) {
		/* CUPS backend mode */
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
			char *ptr = strstr(uri, "backend="), *ptr2;
			if (ptr) { /* Original format */
				ptr += 8;
				ptr2 = strchr(ptr, '&');
				if (ptr2)
					*ptr2 = 0;

				backend = find_backend(ptr);
				if (!backend) {
					ERROR("Invalid backend (%s)\n", ptr);
					exit(1);
				}
				if (ptr2)
					*ptr2 = '&';

				use_serno = strchr(uri, '=');
				if (!use_serno || !*(use_serno+1)) {
					ERROR("Invalid URI (%s)\n", uri);
					exit(1);
				}
				use_serno++;
				ptr = strchr(use_serno, '&');
				if (ptr)
					*ptr = 0;
			} else { /* New format */
				// prefix://backend/serno
				ptr = strchr(uri, '/');
				ptr += 2;
				use_serno = strchr(ptr, '/');
				if (!use_serno || !*(use_serno+1)) {
					ERROR("Invalid URI (%s)\n", uri);
					exit(1);
				}
				*use_serno = 0;
				use_serno++;

				backend = find_backend(ptr);
				if (!backend) {
					ERROR("Invalid backend (%s)\n", ptr);
					exit(1);
				}

				ptr = strchr(ptr, '?');
				if (ptr)
					*ptr = 0;
			}
		}

		/* Always enable fast return in CUPS mode */
		fast_return++;
	} else {
		/* Standalone mode */

		/* Try to guess backend from executable name */
		if (!backend) {
			char *ptr = strrchr(argv[0], '/');
			if (ptr)
				ptr++;
			else
				ptr = argv[0];
			backend = find_backend(ptr);
		}

		srand(getpid());
		jobid = rand();
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
	ret = libusb_init(&ctx);
	if (ret) {
		ERROR("Failed to initialize libusb (%d)\n", ret);
		ret = CUPS_BACKEND_RETRY_CURRENT;
		goto done;
	}

	/* If we don't have a valid backend, print help and terminate */
	if (!backend) {
		print_help(argv[0], NULL); // probes all devices
		ret = CUPS_BACKEND_OK;
		goto done;
	}

	/* If we're in standalone mode, print help only if no args */
	if (!uri) {
		if (argc < 2) {
			print_help(argv[0], backend); // probes all devices
			ret = CUPS_BACKEND_OK;
			goto done;
		}
	}

	/* Enumerate devices */
	found = find_and_enumerate(ctx, &list, backend, use_serno, 0, NUM_CLAIM_ATTEMPTS, &iface, &altset, &endp_up, &endp_down);

	if (found == -1) {
		ERROR("Printer open failure (No matching printers found!)\n");
		ret = CUPS_BACKEND_RETRY;
		goto done;
	}

	if (test_mode) {
		WARNING("**** TEST MODE %d!\n", test_mode);
		if (test_mode >= TEST_MODE_NOATTACH)
			goto bypass;
	}

	/* Open an appropriate device */
	ret = libusb_open(list[found], &dev);
	if (ret) {
		ERROR("Printer open failure (Need to be root?) (%d)\n", ret);
		ret = CUPS_BACKEND_RETRY_CURRENT;
		goto done;
	}

	/* Detach the kernel driver */
	if (libusb_kernel_driver_active(dev, iface)) {
		ret = libusb_detach_kernel_driver(dev, iface);
		if (ret) {
			ERROR("Printer open failure (Could not detach printer from kernel) (%d)\n", ret);
			ret = CUPS_BACKEND_RETRY_CURRENT;
			goto done_close;
		}
	}

	/* Claim the interface so we can start using this! */
	ret = backend_claim_interface(dev, iface, NUM_CLAIM_ATTEMPTS);
	if (ret) {
		ERROR("Printer open failure (Unable to claim interface) (%d)\n", ret);
		ret = CUPS_BACKEND_RETRY;
		goto done_close;
	}

	/* Use the appropriate altesetting! */
	if (altset != 0) {
		ret = libusb_set_interface_alt_setting(dev, iface, altset);
		if (ret) {
			ERROR("Printer open failure (Unable to issue altsettinginterface) (%d)\n", ret);
			ret = CUPS_BACKEND_RETRY;
			goto done_close;
		}
	}

bypass:
	/* Initialize backend */
	DEBUG("Initializing '%s' backend (version %s)\n",
	      backend->name, backend->version);
	backend_ctx = backend->init();

	if (test_mode < TEST_MODE_NOATTACH) {
		struct libusb_device *device;
		struct libusb_device_descriptor desc;

		device = libusb_get_device(dev);
		libusb_get_device_descriptor(device, &desc);

		printer_type = lookup_printer_type(backend,
						   desc.idVendor, desc.idProduct);
	} else {
		printer_type = lookup_printer_type(backend,
						   extra_vid, extra_pid);
	}

	if (printer_type <= P_UNKNOWN) {
		ERROR("Unable to lookup printer type\n");
		ret = CUPS_BACKEND_FAILED;
		goto done_close;
	}

	/* Attach backend to device */
	if (backend->attach(backend_ctx, dev, printer_type, endp_up, endp_down, jobid)) {
		ERROR("Unable to attach to printer!");
		ret = CUPS_BACKEND_FAILED;
		goto done_close;
	}

//	STATE("+org.gutenprint-attached-to-device\n");

	if (!uri) {
		if (backend->cmdline_arg(backend_ctx, argc, argv) < 0)
			goto done_claimed;

		/* Grab the filename */
		fname = argv[optind]; // XXX do this a smarter way?
	}

	if (!fname) {
		if (uri)
			ERROR("ERROR: No input file specified\n");
		goto done_claimed;
	}

	if (ncopies < 1) {
		ERROR("ERROR: need to have at least 1 copy!\n");
		ret = CUPS_BACKEND_FAILED;
		goto done_claimed;
	}

	/* Open file if not STDIN */
	if (strcmp("-", fname)) {
		data_fd = open(fname, O_RDONLY);
		if (data_fd < 0) {
			perror("ERROR:Can't open input file");
			ret = CUPS_BACKEND_FAILED;
			goto done_claimed;
		}
	}

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
		goto done_claimed;
	}

	/* Ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, sigterm_handler);

	/* Time for the main processing loop */
	INFO("Printing started (%d copies)\n", ncopies);

	/* See if it's a CUPS command stream, and if yes, handle it! */
	if (type && !strcmp("application/vnd.cups-command", type))
	{
		ret = parse_cmdstream(backend, backend_ctx, data_fd);
		goto done_claimed;
	}

newpage:

	/* Read in data */
	if ((ret = backend->read_parse(backend_ctx, &job, data_fd, ncopies))) {
		if (current_page)
			goto done_multiple;
		else
			goto done_claimed;
	}

	/* The backend parser might not return a job due to job dependencies.
	   Try and read another page. */
	if (!job)
		goto newpage;

	/* Create our own joblist if necessary */
	if (!(backend->flags & BACKEND_FLAG_JOBLIST)) {
		struct dyesub_joblist *jlist = dyesub_joblist_create(backend, backend_ctx);
		if (!list)
			goto done_claimed;
		dyesub_joblist_addjob(jlist, job);
		job = jlist;
	}

	/* Dump the full marker dump */
	ret = query_markers(backend, backend_ctx, !current_page);
	if (ret)
		goto done_claimed;

	INFO("Printing page %d\n", ++current_page);

	if (test_mode >= TEST_MODE_NOPRINT ) {
		WARNING("**** TEST MODE, bypassing printing!\n");
	} else {
		ret = dyesub_joblist_print(job);
	}

	dyesub_joblist_cleanup(job);

	if (ret)
		goto done_claimed;

	/* Log the completed page */
	if (!uri)
		PAGE("%d %d\n", current_page, ncopies);

	/* Dump a marker status update */
	ret = query_markers(backend, backend_ctx, !current_page);
	if (ret)
		goto done_claimed;

	/* Since we have no way of telling if there's more data remaining
	   to be read (without actually trying to read it), always assume
	   multiple print jobs. */
	goto newpage;

done_multiple:
	close(data_fd);

	/* Done printing, log the total number of pages */
	if (!uri)
		PAGE("total %d\n", current_page * ncopies);
	ret = CUPS_BACKEND_OK;

done_claimed:
	if (test_mode < TEST_MODE_NOATTACH)
		libusb_release_interface(dev, iface);

done_close:
	if (test_mode < TEST_MODE_NOATTACH)
		libusb_close(dev);
done:

	if (backend && backend_ctx) {
		if (backend->teardown)
			backend->teardown(backend_ctx);
		else
			generic_teardown(backend_ctx);
//		STATE("-org.gutenprint-attached-to-device");
	}

	if (list)
		libusb_free_device_list(list, 1);

	libusb_exit(ctx);

	return ret;
}

void dump_markers(struct marker *markers, int marker_count, int full)
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
			val = (markers[i].levelnow <= 0) ? markers[i].levelnow : -1;
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
			case -1:
				DEBUG2("'\"Unable to query remaining prints on %s media\"'", markers[i].name);
				break;
			case -2:
				DEBUG2("'\"Unknown remaining prints on %s media\"'", markers[i].name);
				break;
			case -3:
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
}

int dyesub_read_file(char *filename, void *databuf, int datalen,
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

uint32_t packed_bcd_to_uint32(char *in, int len)
{
	uint32_t out = 0;

	while (len--) {
		out *= 10;
		out += (*in >> 4);
		out *= 10;
		out += (*in & 0xf);
		in++;
	}
        return out;
}

/* Job list manipulation */
struct dyesub_joblist *dyesub_joblist_create(struct dyesub_backend *backend, void *ctx)
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

int dyesub_joblist_addjob(struct dyesub_joblist *list, const void *job)
{
	if (list->num_entries >= DYESUB_MAX_JOB_ENTRIES)
		return 1;

	list->entries[list->num_entries++] = job;

	return 0;
}

int dyesub_joblist_print(const struct dyesub_joblist *list)
{
	int i, j;
	int ret;
	for (i = 0 ; i < list->copies ; i++) {
		for (j = 0 ; j < list->num_entries ; j++) {
			if (list->entries[j]) {
				ret = list->backend->main_loop(list->ctx, list->entries[j]);
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
	return CUPS_BACKEND_OK;
}
