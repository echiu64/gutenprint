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

#include "backend_common.h"

#define BACKEND_VERSION "0.37G"
#ifndef URI_PREFIX
#error "Must Define URI_PREFIX"
#endif

/* Support Functions */

#define ID_BUF_SIZE 2048
static char *get_device_id(struct libusb_device_handle *dev)
{
	int length;
	int iface = 0;
	char *buf = malloc(ID_BUF_SIZE + 1);

	if (libusb_kernel_driver_active(dev, iface))
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

	return buf;
}

int read_data(struct libusb_device_handle *dev, uint8_t endp,
	      uint8_t *buf, int buflen, int *readlen)
{
	int ret;

	/* Clear buffer */
	memset(buf, buflen, 0);

	ret = libusb_bulk_transfer(dev, endp,
				   buf,
				   buflen,
				   readlen,
				   5000);

	if (ret < 0) {
		ERROR("Failure to receive data from printer (libusb error %d: (%d/%d from 0x%02x))\n", ret, *readlen, buflen, endp);
		goto done;
	}

	if (dyesub_debug) {
		int i;
		DEBUG("<- ");
		for (i = 0 ; i < *readlen; i++) {
			DEBUG2("%02x ", *(buf+i));
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

		if (dyesub_debug) {
			int i;
			DEBUG("-> ");
			for (i = 0 ; i < len2; i++) {
				DEBUG2("%02x ", *(buf+i));
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

int terminate = 0;

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

static int print_scan_output(struct libusb_device *device,
			     struct libusb_device_descriptor *desc,
			     char *prefix, char *manuf2,
			     int found, int match,
			     int scan_only, char *match_serno,
			     struct dyesub_backend *backend)
{
	struct libusb_device_handle *dev;

	unsigned char product[STR_LEN_MAX] = "";
	unsigned char serial[STR_LEN_MAX] = "";
	unsigned char manuf[STR_LEN_MAX] = "";
	
	if (libusb_open(device, &dev)) {
		ERROR("Could not open device %04x:%04x (need to be root?)\n", desc->idVendor, desc->idProduct);
		found = -1;
		goto abort;
	}
	
	/* Query detailed info */
	if (desc->iManufacturer) {
		libusb_get_string_descriptor_ascii(dev, desc->iManufacturer, manuf, STR_LEN_MAX);
		sanitize_string((char*)manuf);
	}
	if (desc->iProduct) {
		libusb_get_string_descriptor_ascii(dev, desc->iProduct, product, STR_LEN_MAX);
		sanitize_string((char*)product);
	}
	if (desc->iSerialNumber) {
		libusb_get_string_descriptor_ascii(dev, desc->iSerialNumber, serial, STR_LEN_MAX);
		sanitize_string((char*)serial);
	} else if (backend->query_serno) {
		/* XXX this is ... a cut-n-paste hack */

		uint8_t endp_up, endp_down;
		int i, iface = 0;
		struct libusb_config_descriptor *config;

		if (libusb_kernel_driver_active(dev, iface))
			libusb_detach_kernel_driver(dev, iface);

		/* If we fail to claim the printer, it's already in use
		   so we should just skip over it... */
		if (!libusb_claim_interface(dev, iface)) {
			libusb_get_active_config_descriptor(device, &config);
			for (i = 0 ; i < config->interface[0].altsetting[0].bNumEndpoints ; i++) {
				if ((config->interface[0].altsetting[0].endpoint[i].bmAttributes & 3) == LIBUSB_TRANSFER_TYPE_BULK) {
					if (config->interface[0].altsetting[0].endpoint[i].bEndpointAddress & LIBUSB_ENDPOINT_IN)
						endp_up = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;
					else
						endp_down = config->interface[0].altsetting[0].endpoint[i].bEndpointAddress;				
				}
			}

			/* Ignore result since a failure isn't critical here */
			backend->query_serno(dev, endp_up, endp_down, (char*)serial, STR_LEN_MAX);
			libusb_release_interface(dev, iface);
		}
	}
	
	if (!strlen((char*)serial)) {
		WARNING("**** THIS PRINTER DOES NOT REPORT A SERIAL NUMBER!\n");
		WARNING("**** If you intend to use multiple printers of this typpe, you\n");
		WARNING("**** must only plug one in at a time or unexpected behaivor will occur!\n");
		sprintf((char*)serial, "NONE_UNKNOWN");
	}
	
	if (dyesub_debug)
		DEBUG("%sVID: %04X PID: %04X Manuf: '%s' Product: '%s' Serial: '%s'\n",
		      match ? "MATCH: " : "",
		      desc->idVendor, desc->idProduct, manuf, product, serial);
	
	if (scan_only) {

		char buf[256]; // XXX ugly..
		int j = 0, k = 0;
		char *ieee_id = get_device_id(dev);

		/* URLify the manuf and model strings */
		if (strlen(manuf2))
			strncpy((char*)manuf, manuf2, sizeof(manuf));
		while(*(manuf + j)) {
			buf[k] = *(manuf+j);
			if(buf[k] == ' ') {
				buf[k++] = '%';
				buf[k++] = '2';
				buf[k] = '0';
			}
			k++;
			j++;
		}
		buf[k++] = '/';
		j = 0;
		while (*(product + j + strlen(manuf2))) {
			buf[k] = *(product + j + (strlen(manuf2) ? (strlen(manuf2) + 1) : 0));
			if(buf[k] == ' ') {
				buf[k++] = '%';
				buf[k++] = '2';
				buf[k] = '0';
			}
			k++;
			j++;
		}
		buf[k] = 0;
		
		fprintf(stdout, "direct %s://%s?serial=%s&backend=%s \"%s\" \"%s\" \"%s\" \"\"\n",
			prefix, buf, serial, backend->uri_prefix, 
			product, product,
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

static struct dyesub_backend *backends[] = {
	&canonselphy_backend,
	&kodak6800_backend,
	&kodak605_backend,
	&kodak1400_backend,
	&shinkos2145_backend,
	&updr150_backend,
	&mitsu70x_backend,
	&dnpds40_backend,
	NULL,
};

static int find_and_enumerate(struct libusb_context *ctx,
			      struct libusb_device ***list,
			      struct dyesub_backend *backend,
			      char *match_serno,
			      int printer_type,
			      int scan_only)
{
	int num;
	int i, j, k;
	int found = -1;

	/* Enumerate and find suitable device */
	num = libusb_get_device_list(ctx, list);

	for (i = 0 ; i < num ; i++) {
		struct libusb_device_descriptor desc;
		int match = 0;
		libusb_get_device_descriptor((*list)[i], &desc);
		
		for (k = 0 ; backends[k] ; k++) {
			if (backend && backend != backends[k])
				continue;
			for (j = 0 ; backends[k]->devices[j].vid ; j++) {
				if (desc.idVendor == backends[k]->devices[j].vid &&
				    desc.idProduct == backends[k]->devices[j].pid) {
					match = 1;
					if (printer_type == P_ANY ||
					    printer_type == backends[k]->devices[j].type)
						found = i;
					goto match;
				}
			}
		}

	match:
		if (!match) {
			if (getenv("EXTRA_PID") && getenv("EXTRA_TYPE") && getenv("EXTRA_VID")) {
				int pid = strtol(getenv("EXTRA_PID"), NULL, 16);
				int vid = strtol(getenv("EXTRA_VID"), NULL, 16);
				int type = atoi(getenv("EXTRA_TYPE"));
				if (vid == desc.idVendor &&
				    pid == desc.idProduct) {
					match = 1;
					if (printer_type == P_ANY ||
					    printer_type == type)
						found = i;
				}
			}
		}

		if (!match)
			continue;

		found = print_scan_output((*list)[i], &desc,
					  URI_PREFIX, backends[k]->devices[j].manuf_str,
					  found, (found == i),
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
	struct dyesub_backend *backend;

	if (!uri_prefix)
		return NULL;

	for (i = 0; ; i++) {
		backend = backends[i];
		if (!backend)
			return NULL;
		if (!strcmp(uri_prefix, backend->uri_prefix))
			return backend;
	}
	return NULL;
}

/* Debug flag */
int dyesub_debug = 0;

/* MAIN */

int main (int argc, char **argv) 
{
	struct libusb_context *ctx = NULL;
	struct libusb_device **list = NULL;
	struct libusb_device_handle *dev;
	struct libusb_config_descriptor *config;

	struct dyesub_backend *backend;
	void * backend_ctx = NULL;

	uint8_t endp_up = 0;
	uint8_t endp_down = 0;

	int data_fd = fileno(stdin);

	int i;
	int claimed;

	int ret = 0;
	int iface = 0;
	int found = -1;
	int copies = 1;
	int jobid = 0;
	int pages = 0;

	char *uri = getenv("DEVICE_URI");
	char *use_serno = NULL;
	int query_only = 0;
	int printer_type = P_ANY;

	if (getenv("DYESUB_DEBUG"))
		dyesub_debug = 1;

	DEBUG("Multi-Call Gutenprint DyeSub CUPS Backend version %s\n",
	      BACKEND_VERSION);
	DEBUG("Copyright 2007-2014 Solomon Peachy\n");

	/* Cmdline help */
	if (argc < 2) {
		char *ptr = strrchr(argv[0], '/');
		if (ptr)
			ptr++;
		else
			ptr = argv[0];
		backend = find_backend(getenv("BACKEND"));
		if (!backend)
			backend = find_backend(ptr);

		if (!backend) {
			DEBUG("CUPS Usage:\n\tDEVICE_URI=someuri %s job user title num-copies options [ filename ]\n\n",
			      URI_PREFIX);
			DEBUG("Internal Backends: (prefix with DEVICE=serno for specific device)\n");
			for (i = 0; ; i++) {
				backend = backends[i];
				if (!backend)
					break;
				DEBUG(" %s backend version %s (BACKEND=%s)\n",
				      backend->name, backend->version, backend->uri_prefix);
				DEBUG("\t\t%s [ infile | - ]\n",
				      backend->uri_prefix);
				
				if (backend->cmdline_usage) {
					backend->cmdline_usage(backend->uri_prefix);
				}
			}
		} else {
			DEBUG(" %s backend version %s (BACKEND=%s)\n",
			      backend->name, backend->version, backend->uri_prefix);
			DEBUG("  Standalone Usage: (prefix with DEVICE=serno for specific device)\n");
			DEBUG("\t\t%s [ infile | - ]\n",
			      backend->uri_prefix);

			if (backend->cmdline_usage) {
				backend->cmdline_usage(backend->uri_prefix);
			}
		}
		libusb_init(&ctx);
		find_and_enumerate(ctx, &list, backend, NULL, P_ANY, 1);
		libusb_free_device_list(list, 1);
		libusb_exit(ctx);
		exit(0);
	}

	/* Are we running as a CUPS backend? */
	if (uri) {
		if (argv[1])
			jobid = atoi(argv[1]);
		if (argv[4])
			copies = atoi(argv[4]);
		if (argv[6]) {  /* IOW, is it specified? */
			data_fd = open(argv[6], O_RDONLY);
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
		i = fcntl(data_fd, F_SETFL, 0);
		if (i < 0) {
			perror("ERROR:Can't open input");
			exit(1);
		}

		/* Figure out backend */
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
	} else {
		use_serno = getenv("DEVICE");

		/* find backend */
		backend = find_backend(getenv("BACKEND"));
		if (!backend) {
			char *ptr = strrchr(argv[0], '/');
			if (ptr)
				ptr++;
			else
				ptr = argv[0];
			backend = find_backend(ptr);
		}
		if (!backend) {
			ERROR("Invalid backend (%s)\n", uri);
			exit(1);
		}

		srand(getpid());
		jobid = rand();

		if (backend->cmdline_arg && backend->cmdline_arg(NULL, 0, argv[1], argv[2])) {
			query_only = 1;
		} else {
			/* Open Input File */
			if (strcmp("-", argv[1])) {
				data_fd = open(argv[1], O_RDONLY);
				if (data_fd < 0) {
					perror("ERROR:Can't open input file");
					exit(1);
				}
			}
		}
	}

	/* Ignore SIGPIPE */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, sigterm_handler);

	/* Initialize backend */
	DEBUG("Initializing '%s' backend (version %s)\n",
	      backend->name, backend->version);
	backend_ctx = backend->init();

	/* Parse printjob if necessary */
	if (!query_only && backend->early_parse) {
		printer_type = backend->early_parse(backend_ctx, data_fd);
		if (printer_type < 0) {
			ret = 5; /* CUPS_BACKEND_CANCEL */
			goto done;
		}
	}

	/* Libusb setup */
	libusb_init(&ctx);
	/* Enumerate devices */
	found = find_and_enumerate(ctx, &list, backend, use_serno, printer_type, 0);

	if (found == -1) {
		ERROR("Printer open failure (No suitable printers found!)\n");
		ret = 4; /* CUPS_BACKEND_STOP */
		goto done;
	}

	ret = libusb_open(list[found], &dev);
	if (ret) {
		ERROR("Printer open failure (Need to be root?) (%d)\n", ret);
		goto done;
	}
	
	claimed = libusb_kernel_driver_active(dev, iface);
	if (claimed) {
		ret = libusb_detach_kernel_driver(dev, iface);
		if (ret) {
			ERROR("Printer open failure (Could not detach printer from kernel)\n");
			goto done_close;
		}
	}

	ret = libusb_claim_interface(dev, iface);
	if (ret) {
		ERROR("Printer open failure (Could not claim printer interface)\n");
		goto done_close;
	}

	ret = libusb_get_active_config_descriptor(list[found], &config);
	if (ret) {
		ERROR("Printer open failure (Could not fetch config descriptor)\n");
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

	/* Attach backend to device */
	backend->attach(backend_ctx, dev, endp_up, endp_down, jobid);
	
	if (query_only) {
		backend->cmdline_arg(backend_ctx, 1, argv[1], argv[2]);
		goto done_claimed;
	} 

	/* Time for the main processing loop */
	INFO("Printing started (%d copies)\n", copies);

newpage:
	/* Do early parsing if needed for subsequent pages */
	if (pages && backend->early_parse) {
		ret = backend->early_parse(backend_ctx, data_fd);
		if (ret < 0)
			goto done_multiple;
	}

	/* Read in data */
	if (backend->read_parse(backend_ctx, data_fd)) {
		if (pages)
			goto done_multiple;
		else
			goto done_claimed;
	}

	INFO("Printing page %d\n", ++pages);

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
	INFO("All printing done (%d pages * %d copies)\n", pages, copies);
	ret = 0;

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

