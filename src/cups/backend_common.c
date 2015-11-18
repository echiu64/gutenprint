/*
 *   CUPS Backend common code
 *
 *   Copyright (c) 2007-2015 Solomon Peachy <pizza@shaftnet.org>
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

#include "backend_common.h"

#define BACKEND_VERSION "0.63G"
#ifndef URI_PREFIX
#error "Must Define URI_PREFIX"
#endif

#define NUM_CLAIM_ATTEMPTS 10

/* Global Variables */
int dyesub_debug = 0;
int terminate = 0;
int fast_return = 0;
int extra_vid = -1;
int extra_pid = -1;
int extra_type = -1;
int copies = 1;
char *use_serno = NULL;
int current_page = 0;

/* Support Functions */
static int backend_claim_interface(struct libusb_device_handle *dev, int iface)
{
	int attempts = NUM_CLAIM_ATTEMPTS;
	int ret;
	do {
		ret = libusb_claim_interface(dev, iface);
		if (!ret)
			break;
		sleep(1);
	} while (--attempts > 0);

	if (ret)
		ERROR("Printer open failure (Could not claim printer interface after %d attempts) (%d)\n", NUM_CLAIM_ATTEMPTS, ret);

	return ret;
}

#define ID_BUF_SIZE 2048
static char *get_device_id(struct libusb_device_handle *dev)
{
	int length;
	int iface = 0;
	char *buf = malloc(ID_BUF_SIZE + 1);

	if (!buf) {
		ERROR("Memory allocation failure (%d bytes)\n", ID_BUF_SIZE+1);
		return NULL;
	}
	
	if (libusb_kernel_driver_active(dev, iface))
		libusb_detach_kernel_driver(dev, iface);

	if (backend_claim_interface(dev, iface))
		return NULL;

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
};

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
				   10000);

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
	      uint8_t *buf, int len)
{
	int num = 0;

	if (dyesub_debug) {
		DEBUG("Sending %d bytes to printer\n", len);
	}

	while (len) {
		int len2 = (len > 65536) ? 65536: len;
		int ret = libusb_bulk_transfer(dev, endp,
					   buf, len2,
					   &num, 15000);

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
			ERROR("Failure to send data to printer (libusb error %d: (%d/%d to 0x%02x))\n", ret, num, len, endp);
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

static int print_scan_output(struct libusb_device *device,
			     struct libusb_device_descriptor *desc,
			     char *prefix, char *manuf2,
			     int found,
			     int scan_only, char *match_serno,
			     struct dyesub_backend *backend)
{
	struct libusb_device_handle *dev;
	char buf[256];
	char *product = NULL, *serial = NULL, *manuf = NULL, *descr = NULL;

	int dlen = 0;
	struct deviceid_dict dict[MAX_DICT];
	char *ieee_id = NULL;

	if (libusb_open(device, &dev)) {
		ERROR("Could not open device %04x:%04x (need to be root?)\n", desc->idVendor, desc->idProduct);
		found = -1;
		goto abort;
	}

	ieee_id = get_device_id(dev);

	/* Get IEEE1284 info */
	dlen = parse1284_data(ieee_id, dict);

	/* Look up mfg string. */
	if (manuf2 && strlen(manuf2)) {
		manuf = url_encode(manuf2);  /* Backend supplied */
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
		manuf = url_encode("Unknown");
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
		product = url_encode("Unknown");
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
		descr = malloc(256);
		if (!descr) {
			ERROR("Memory allocation failure (%d bytes)\n", 256);
			if (manuf3)
				free(manuf3);
			if (product2)
				free(product2);
			return -1;
		}
		
		sprintf(descr, "%s %s", manuf3, product2);
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
		int iface = 0;

		struct libusb_config_descriptor *config;

		if (libusb_kernel_driver_active(dev, iface))
			libusb_detach_kernel_driver(dev, iface);

		/* Try to claim the printer, and handle transient failures */
		if (!backend_claim_interface(dev, iface)) {
			int i;
			uint8_t endp_up = 0, endp_down = 0;
			libusb_get_active_config_descriptor(device, &config);
			for (i = 0 ; i < config->interface[0].altsetting[0].bNumEndpoints ; i++) {
				if ((config->interface[0].altsetting[0].endpoint[i].bmAttributes & 3) == LIBUSB_TRANSFER_TYPE_BULK) {
					if (config->interface[0].altsetting[0].endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_IN)
						endp_up = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;
					else
						endp_down = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;				
				}
			}

			buf[0] = 0;
			/* Ignore result since a failure isn't critical here */
			backend->query_serno(dev, endp_up, endp_down, buf, STR_LEN_MAX);
			libusb_release_interface(dev, iface);
		}
		serial = url_encode(buf);
	}

	if (!serial || !strlen(serial)) {  /* Last-ditch */
		if (serial) free(serial);
		WARNING("**** THIS PRINTER DOES NOT REPORT A SERIAL NUMBER!\n");
		WARNING("**** If you intend to use multiple printers of this type, you\n");
		WARNING("**** must only plug one in at a time or unexpected behaivor will occur!\n");
		serial = strdup("NONE_UNKNOWN");
	}

	if (dyesub_debug)
		DEBUG("VID: %04X PID: %04X Manuf: '%s' Product: '%s' Serial: '%s'\n",
		      desc->idVendor, desc->idProduct, manuf, product, serial);
	
	if (scan_only) {
		int k = 0;

		/* URLify the manuf and model strings */
		strncpy(buf, manuf, sizeof(buf) - 2);
		k = strlen(buf);
		buf[k++] = '/';
		buf[k] = 0;

		strncpy(buf + k, product, sizeof(buf)-k);

		fprintf(stdout, "direct %s://%s?serial=%s&backend=%s \"%s\" \"%s\" \"%s\" \"\"\n",
			prefix, buf, serial, backend->uri_prefix, 
			descr, descr,
			ieee_id? ieee_id : "");
		
	}
	
	/* If a serial number was passed down, use it. */
	if (match_serno && strcmp(match_serno, (char*)serial)) {
		found = -1;
	}

	/* Free things up */
	if(serial) free(serial);
	if(manuf) free(manuf);
	if(product) free(product);
	if(descr) free(descr);
	if(ieee_id) free(ieee_id);

	libusb_close(dev);
abort:

	/* Clean up the dictionary */
	while (dlen--) {
		free (dict[dlen].key);
		free (dict[dlen].val);
	}

	return found;
}

extern struct dyesub_backend updr150_backend;
extern struct dyesub_backend kodak6800_backend;
extern struct dyesub_backend kodak605_backend;
extern struct dyesub_backend kodak1400_backend;
extern struct dyesub_backend shinkos1245_backend;
extern struct dyesub_backend shinkos2145_backend;
extern struct dyesub_backend shinkos6145_backend;
extern struct dyesub_backend shinkos6245_backend;
extern struct dyesub_backend canonselphy_backend;
extern struct dyesub_backend mitsu70x_backend;
extern struct dyesub_backend mitsu9550_backend;
extern struct dyesub_backend dnpds40_backend;
extern struct dyesub_backend cw01_backend;

static struct dyesub_backend *backends[] = {
	&canonselphy_backend,
	&kodak6800_backend,
	&kodak605_backend,
	&kodak1400_backend,
	&shinkos1245_backend,
	&shinkos2145_backend,
	&shinkos6145_backend,
	&shinkos6245_backend,
	&updr150_backend,
	&mitsu70x_backend,
	&mitsu9550_backend,
	&dnpds40_backend,
	&cw01_backend,
	NULL,
};

static int find_and_enumerate(struct libusb_context *ctx,
			      struct libusb_device ***list,
			      struct dyesub_backend *backend,
			      char *match_serno,
			      int scan_only)
{
	int num;
	int i, j = 0, k;
	int found = -1;

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
						goto match;
					}
				}
				if (desc.idVendor == backends[k]->devices[j].vid &&
				    desc.idProduct == backends[k]->devices[j].pid) {
					found = i;
					goto match;
				}
			}
		}

		continue;

	match:
		found = print_scan_output((*list)[i], &desc,
					  URI_PREFIX, backends[k]->devices[j].manuf_str,
					  found,
					  scan_only, match_serno,
					  backends[k]);

		if (found != -1 && !scan_only)
			break;
	}

	return found;
}

static struct dyesub_backend *find_backend(char *uri_prefix)
{
	int i;
	

	if (!uri_prefix)
		return NULL;

	for (i = 0; ; i++) {
		struct dyesub_backend *backend = backends[i];
		if (!backend)
			return NULL;
		if (!strcmp(uri_prefix, backend->uri_prefix))
			return backend;
	}
	return NULL;
}

void print_license_blurb(void)
{
	const char *license = "\n\
Copyright 2007-2015 Solomon Peachy <pizza AT shaftnet DOT org>\n\
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
along with this program; if not, write to the Free Software\n\
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n\
\n          [http://www.gnu.org/licenses/gpl-2.0.html]\n\n";

	fprintf(stderr, "%s", license);
}

void print_help(char *argv0, struct dyesub_backend *backend)
{
	struct libusb_context *ctx = NULL;
	struct libusb_device **list = NULL;
	int i;

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
		DEBUG(" DYESUB_DEBUG EXTRA_PID EXTRA_VID EXTRA_TYPE BACKEND SERIAL\n");
		DEBUG("CUPS Usage:\n");
		DEBUG("\tDEVICE_URI=someuri %s job user title num-copies options [ filename ]\n", URI_PREFIX);
		DEBUG("\n");
		DEBUG("Standalone Usage:\n");
		DEBUG("\t%s\n", URI_PREFIX);
		DEBUG("  [ -D ] [ -G ] [ -f ]\n");
		DEBUG("  [ -S serialnum ] \n");
		DEBUG("  [ -V extra_vid ] [ -P extra_pid ] [ -T extra_type ] \n");
		DEBUG("  [ backend_specific_args ] \n");
		DEBUG("  [ -d copies ] \n");
		DEBUG("  [ - | infile ] \n");
		for (i = 0; ; i++) {
			backend = backends[i];
			if (!backend)
				break;
			DEBUG("  BACKEND=%s\t# %s version %s\n",
			      backend->uri_prefix, backend->name, backend->version);
			if (backend->cmdline_usage)
				backend->cmdline_usage();
		}
	} else {
		DEBUG("Standalone %s backend version %s\n",
		      backend->name, backend->version);
		DEBUG("\t%s\n", backend->uri_prefix);
		DEBUG("\t[ -D ] [ -G ] [ -S serialnum ] \n");
		DEBUG("\t[ -V extra_vid ] [ -P extra_pid ] [ -T extra_type ] \n");
		if (backend->cmdline_usage)
			backend->cmdline_usage();
		DEBUG("\t[ -d copies ] [ infile | - ]\n");
	}

	/* Probe for printers */
	i = libusb_init(&ctx);
	if (i) {
		ERROR("Failed to initialize libusb (%d)\n", i);
		exit(CUPS_BACKEND_STOP);
	}
	find_and_enumerate(ctx, &list, backend, NULL, 1);
	libusb_free_device_list(list, 1);
	libusb_exit(ctx);
}

int main (int argc, char **argv) 
{
	struct libusb_context *ctx = NULL;
	struct libusb_device **list = NULL;
	struct libusb_device_handle *dev;
	struct libusb_config_descriptor *config;

	struct dyesub_backend *backend = NULL;
	void * backend_ctx = NULL;

	uint8_t endp_up = 0;
	uint8_t endp_down = 0;

	int data_fd = fileno(stdin);

	int i;
	int claimed;

	int ret = CUPS_BACKEND_OK;
	int iface = 0;
	int found = -1;
	int jobid = 0;

	char *uri;
	char *fname = NULL;

	DEBUG("Multi-Call Dye-sublimation CUPS Backend version %s\n",
	      BACKEND_VERSION);
	DEBUG("Copyright 2007-2015 Solomon Peachy\n");
	DEBUG("This free software comes with ABSOLUTELY NO WARRANTY! \n");
	DEBUG("Licensed under the GNU GPL.  Run with '-G' for more details.\n");
	DEBUG("\n");

	/* First pass at cmdline parsing */
	if (getenv("DYESUB_DEBUG"))
		dyesub_debug = atoi(getenv("DYESUB_DEBUG"));
	if (getenv("EXTRA_PID"))
		extra_pid = strtol(getenv("EXTRA_PID"), NULL, 16);
	if (getenv("EXTRA_VID"))
		extra_pid = strtol(getenv("EXTRA_VID"), NULL, 16);
	if (getenv("EXTRA_PID"))
		extra_type = atoi(getenv("EXTRA_TYPE"));
	if (getenv("BACKEND"))
		backend = find_backend(getenv("BACKEND"));
	if (getenv("FAST_RETURN"))
		fast_return++;
	use_serno = getenv("SERIAL");
	uri = getenv("DEVICE_URI");  /* CUPS backend mode? */

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
			copies = atoi(argv[base + 3]);
		if (argc > 6)
			fname = argv[base + 5];
		else
			fname = "-";

		/* Figure out backend based on URI */
		{
			char *ptr = strstr (uri, "backend="), *ptr2;
			if (!ptr) {
				ERROR("Invalid URI prefix (%s)\n", uri);
				exit(1);
			}
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
		}

		use_serno = strchr(uri, '=');
		if (!use_serno || !*(use_serno+1)) {
			ERROR("Invalid URI (%s)\n", uri);
			exit(1);
		}
		use_serno++;
		{
			char *ptr = strchr(use_serno, '&');
			if (ptr)
				*ptr = 0;
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
		ret = CUPS_BACKEND_STOP;
		goto done;
	}

	/* If we don't have a valid backend, print help and terminate */
	if (!backend) {
		print_help(argv[0], NULL); // probes all devices
		exit(1);
	}

	/* If we're in standalone mode, print help only if no args */
	if (!uri) {
		if (argc < 2) {
			print_help(argv[0], backend); // probes all devices
			exit(1);
		}
	}

	/* Enumerate devices */
	found = find_and_enumerate(ctx, &list, backend, use_serno, 0);

	if (found == -1) {
		ERROR("Printer open failure (No matching printers found!)\n");
		ret = CUPS_BACKEND_HOLD;
		goto done;
	}

	/* Open an appropriate device */
	ret = libusb_open(list[found], &dev);
	if (ret) {
		ERROR("Printer open failure (Need to be root?) (%d)\n", ret);
		ret = CUPS_BACKEND_STOP;
		goto done;
	}

	claimed = libusb_kernel_driver_active(dev, iface);
	if (claimed) {
		ret = libusb_detach_kernel_driver(dev, iface);
		if (ret) {
			ERROR("Printer open failure (Could not detach printer from kernel) (%d)\n", ret);
			ret = CUPS_BACKEND_STOP;
			goto done_close;
		}
	}

	ret = backend_claim_interface(dev, iface);
	if (ret) {
		ret = CUPS_BACKEND_STOP;
		goto done_close;
	}

	ret = libusb_get_active_config_descriptor(list[found], &config);
	if (ret) {
		ERROR("Printer open failure (Could not fetch config descriptor) (%d)\n", ret);
		ret = CUPS_BACKEND_STOP;
		goto done_close;
	}

	for (i = 0 ; i < config->interface[0].altsetting[0].bNumEndpoints ; i++) {
		if ((config->interface[0].altsetting[0].endpoint[i].bmAttributes & 3) == LIBUSB_TRANSFER_TYPE_BULK) {
			if (config->interface[0].altsetting[0].endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_IN)
				endp_up = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;
			else
				endp_down = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;				
		}
	}

	/* Initialize backend */
	DEBUG("Initializing '%s' backend (version %s)\n",
	      backend->name, backend->version);
	backend_ctx = backend->init();

	/* Attach backend to device */
	backend->attach(backend_ctx, dev, endp_up, endp_down, jobid);

	if (!uri) {
		if (backend->cmdline_arg(backend_ctx, argc, argv) < 0)
			goto done_claimed;

		/* Grab the filename */
		fname = argv[optind]; // XXX do this a smarter way?
	}

	if (!fname) {
		if (uri)
			fprintf(stderr, "ERROR: No input file specified\n");
		goto done_claimed;
	}

	/* Open file if not STDIN */
	if (strcmp("-", fname)) {
		data_fd = open(fname, O_RDONLY);
		if (data_fd < 0) {
			perror("ERROR:Can't open input file");
			exit(1);
		}
	}

	/* Ensure we're using BLOCKING I/O */
	i = fcntl(data_fd, F_GETFL, 0);
	if (i < 0) {
		perror("ERROR:Can't open input");
		exit(1);
	}
	i &= ~O_NONBLOCK;
	i = fcntl(data_fd, F_SETFL, i);
	if (i < 0) {
		perror("ERROR:Can't open input");
		exit(1);
	}

	/* Ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, sigterm_handler);

	/* Time for the main processing loop */
	INFO("Printing started (%d copies)\n", copies);

newpage:

	/* Read in data */
	if ((ret = backend->read_parse(backend_ctx, data_fd))) {
		if (current_page)
			goto done_multiple;
		else
			goto done_claimed;
	}

	INFO("Printing page %d\n", ++current_page);

	ret = backend->main_loop(backend_ctx, copies);
	if (ret)
		goto done_claimed;

	/* Since we have no way of telling if there's more data remaining
	   to be read (without actually trying to read it), always assume
	   multiple print jobs. */
	goto newpage;

done_multiple:
	close(data_fd);

	/* Done printing */
	INFO("All printing done (%d pages * %d copies)\n", current_page, copies);
	ret = CUPS_BACKEND_OK;

done_claimed:
	libusb_release_interface(dev, iface);

done_close:
#if 0
	if (claimed)
		libusb_attach_kernel_driver(dev, iface);
#endif
	libusb_close(dev);
done:

	if (backend && backend_ctx)
		backend->teardown(backend_ctx);

	if (list)
		libusb_free_device_list(list, 1);
	if (ctx)
		libusb_exit(ctx);

	return ret;
}

int lookup_printer_type(struct dyesub_backend *backend, uint16_t idVendor, uint16_t idProduct)
{
	int i;
	int type = -1;

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
