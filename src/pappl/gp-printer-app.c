//
// Gytenprint Printer app for the Printer Application Framework
//
// Copyright © 2020 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

//
// Include necessary headers...
//

#include <pappl/pappl.h>
#include <gutenprint/vars.h>
#include <gutenprint/image.h>

//
// Local globals
//

#define CIRCULAR_BUFFER_CAPACITY 16
// Solaris with gcc has problems because gcc's limits.h doesn't #define this
#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

static volatile stp_image_status_t Image_status = STP_IMAGE_STATUS_OK;

//
// Circular Buffer structure and functions
//

typedef struct node_s {						// Circular buffer node
  int           row;							// Row
  unsigned char *data;						// Raster line
  struct node_s *next;						// Next pointer
} node_t;

typedef struct circular_buf_s {		// Circular buffer
  pthread_mutex_t   readwrite_lock_mutex;		// Read/Write mutex
  pthread_cond_t    readwrite_lock_cond;		// Read/Write cond
  node_t      *reader;						// Reader pointer
  node_t      *writer;						// Writer pointer
  size_t      count;							// Buffer Count
  size_t      capacity;						// Buffer Capacity
} circular_buf_t;

circular_buf_t 		*cbuf;    // Global circular buffer

static void circular_buf_free();
static void circular_buf_init();
static unsigned char *circular_buf_read(circular_buf_t *cbuf);
static void circular_buf_write(circular_buf_t *cbuf, const unsigned char *data, int row);

typedef struct
{
  int			row;
  int			adjusted_width;
  int			adjusted_height;
  cups_page_header2_t	header;		/* Page header from file */
} cups_image_t;

static stp_image_t theImage =
{
  Image_init,
  NULL,				/* reset */
  Image_width,
  Image_height,
  Image_get_row,
  Image_get_appname,
  Image_conclude,
  NULL
};

static const char *Image_get_appname(stp_image_t *image);
static stp_image_status_t Image_get_row(stp_image_t *image, unsigned char *data, size_t byte_limit, int row);
static int	Image_height(stp_image_t *image);
static int	Image_width(stp_image_t *image);
static void	Image_conclude(stp_image_t *image);
static void	Image_init(stp_image_t *image);

static void	pappl_writefunc(void *file, const char *buf, size_t bytes);

//
// Local Functions...
//

static bool   gp_callback(pappl_system_t *system, const char *driver_name, const char *device_uri, pappl_pdriver_data_t *driver_data, ipp_t **driver_attrs, void *data);
static void   gp_identify(pappl_printer_t *printer, pappl_identify_actions_t actions, const char *message);
static bool   gp_print(pappl_job_t *job, pappl_poptions_t *options, pappl_device_t *device);
static bool   gp_rendjob(pappl_job_t *job, pappl_poptions_t *options, pappl_device_t *device);
static bool   gp_rendpage(pappl_job_t *job, pappl_poptions_t *options, pappl_device_t *device, unsigned page);
static bool   gp_rstartjob(pappl_job_t *job, pappl_poptions_t *options, pappl_device_t *device);
static bool   gp_rstartpage(pappl_job_t *job, pappl_poptions_t *options, pappl_device_t *device, unsigned page);
static bool   gp_rwrite(pappl_job_t *job, pappl_poptions_t *options, pappl_device_t *device, unsigned y, const unsigned char *pixels);
static void   gp_setup(pappl_system_t *system);
static bool   gp_status(pappl_printer_t *printer);
static pappl_system_t   *system_cb(int num_options, cups_option_t *options, void *data);


//
// 'main()' - Main entry for the gutenprint-printer-app.
//

int
main(int  argc,				// I - Number of command-line arguments
     char *argv[])			// I - Command-line arguments
{
  circular_buf_init();
  papplMainloop(argc, argv, "1.0", NULL, NULL, NULL, system_cb, "gutenprint_printer_app");
  circular_buf_free();
  return (0);
}


//
// 'circular_buf_init()' - Initialize the circular buffer
//

static void
circular_buf_init()
{
  cbuf = (circular_buf_t *)malloc(sizeof(circular_buf_t));
  pthread_mutex_init(&cbuf->readwrite_lock_mutex, NULL);
  pthread_cond_init(&cbuf->readwrite_lock_cond, NULL);
  cbuf->reader = NULL;
  cbuf->writer = NULL;
  cbuf->count = 0;
  cbuf->capacity = CIRCULAR_BUFFER_CAPACITY;
}


//
// 'circular_buf_free()' - Free the buffer
//

static void
circular_buf_free()
{
  free(&cbuf->readwrite_lock_cond);
  free(&cbuf->readwrite_lock_mutex);
  free(cbuf);
}


//
// 'circular_buf_read()' - Read from circular buffer
//

static unsigned char *
circular_buf_read(
    circular_buf_t    *cbuf)					// I - Circular buffer
{
  unsigned char *buffer = strdup(cbuf->reader->data);

  if (cbuf->reader == cbuf->writer)
  {
    free(cbuf->reader->data);
    free(cbuf->reader);
    cbuf->reader = NULL;
    cbuf->writer = NULL;
  }
  else
  {
    cbuf->reader = cbuf->reader->next;
    cbuf->writer->next = cbuf->reader;
  }

  cbuf->count--;

  return buffer;
}


//
// 'circular_buf_write()' - Write to circular buffer
//

static void
circular_buf_write(
    circular_buf_t        *cbuf,		// I - Circular Buffer
    const unsigned char   *data,		// I - Write Data
    int                   row)			// I - Row
{
  node_t  *node = (node_t *)malloc(sizeof(node_t));

  node->data = strdup(data);
  node->row = row;

  pthread_mutex_lock(&cbuf->readwrite_lock_mutex);

  if (cbuf->reader == NULL)
    cbuf->reader = node;
  else
    cbuf->writer->next = node;

  cbuf->writer = node;
  cbuf->writer->next = cbuf->reader;

  cbuf->count++;

  if (cbuf->reader)
    pthread_cond_signal(&cbuf->readwrite_lock_cond);

  pthread_mutex_unlock(&cbuf->readwrite_lock_cond);
}


//
// 'gp_callback()' - GP callback.
//

static bool				   // O - `true` on success, `false` on failure
gp_callback(
    pappl_system_t       *system,	   // I - System
    const char           *driver_name,   // I - Driver name
    const char           *device_uri,	   // I - Device URI
    pappl_pdriver_data_t *driver_data,   // O - Driver data
    ipp_t                **driver_attrs, // O - Driver attributes
    void                 *data)	   // I - Callback data
{
  int   i;                               // Looping variable


  if (!driver_name || !device_uri || !driver_data || !driver_attrs)
  {
    papplLog(system, PAPPL_LOGLEVEL_ERROR, "Driver callback called without required information.");
    return (false);
  }

  if (!data || strcmp((const char *)data, "hp_printer_app"))
  {
    papplLog(system, PAPPL_LOGLEVEL_ERROR, "Driver callback called with bad data pointer.");
    return (false);
  }

  driver_data->identify           = gp_identify;
  driver_data->identify_default   = PAPPL_IDENTIFY_ACTIONS_SOUND;
  driver_data->identify_supported = PAPPL_IDENTIFY_ACTIONS_DISPLAY | PAPPL_IDENTIFY_ACTIONS_SOUND;
  driver_data->print              = gp_print;
  driver_data->rendjob            = gp_rendjob;
  driver_data->rendpage           = gp_rendpage;
  driver_data->rstartjob          = gp_rstartjob;
  driver_data->rstartpage         = gp_rstartpage;
  driver_data->rwrite             = gp_rwrite;
  driver_data->status             = gp_status;
  driver_data->format             = "image/raster";
  driver_data->orient_default     = IPP_ORIENT_NONE;
  driver_data->quality_default    = IPP_QUALITY_NORMAL;

}


//
// 'gp_identify()' - Identify the printer.
//

static void
gp_identify(
    pappl_printer_t          *printer,	// I - Printer
    pappl_identify_actions_t actions, 	// I - Actions to take
    const char               *message)	// I - Message, if any
{
  (void)printer;
  (void)actions;

  // Identify a printer using display, flash, sound or speech.
}


//
// 'gp_print()' - Print file
//

static bool                           // O - `true` on success, `false` on failure
gp_print(
    pappl_job_t      *job,            // I - Job
    pappl_poptions_t *options,        // I - Options
    pappl_device_t   *device)         // I - Device
{
  int		       infd;	        // Input file
  ssize_t	       bytes;	        // Bytes read/written
  char	       buffer[65536];	// Read/write buffer


  papplJobSetImpressions(job, 1);

  infd  = open(papplJobGetFilename(job), O_RDONLY);

  while ((bytes = read(infd, buffer, sizeof(buffer))) > 0)
  {
    if (papplDeviceWrite(device, buffer, (size_t)bytes) < 0)
    {
      papplLogJob(job, PAPPL_LOGLEVEL_ERROR, "Unable to send %d bytes to printer.", (int)bytes);
      close(infd);
      return (false);
    }
  }

  close(infd);

  papplJobSetImpressionsCompleted(job, 1);

  return (true);
}


//
// 'gp_rendjob()' - End a job.
//

static bool                     // O - `true` on success, `false` on failure
gp_rendjob(
    pappl_job_t      *job,      // I - Job
    pappl_poptions_t *options,  // I - Options
    pappl_device_t   *device)   // I - Device
{
  stp_vars_t	   *gp_data = (stp_vars_t *)papplJobGetData(job);
				  // Job data


  (void)options;

  stp_vars_destroy(gp_data);
  free(gp_data);
  papplJobSetData(job, NULL);

  return (true);
}


//
// 'gp_rendpage()' - End a page.
//

static bool                     // O - `true` on success, `false` on failure
gp_rendpage(
    pappl_job_t      *job,      // I - Job
    pappl_poptions_t *options,  // I - Job options
    pappl_device_t   *device,   // I - Device
    unsigned         page)      // I - Page number
{
  // gp_image_t    *gp = (gp_image_t *)papplJobGetDa  ta(job);
                                // Job data

}


//
// 'gp_rstartjob()' - Start a job.
//

static bool                     // O - `true` on success, `false` on failure
gp_rstartjob(
    pappl_job_t      *job,      // I - Job
    pappl_poptions_t *options,  // I - Job options
    pappl_device_t   *device)   // I - Device
{
  stp_vars_t  *default_settings;
  cups_image_t    cups;
  char		outname[1024];		// Output filename

  (void)options;

  theImage.rep = &cups;

  stp_init();
  default_settings = stp_vars_create();
  stp_set_outfunc(default_settings, pappl_writefunc);

  papplJobSetData(job, default_settings);

  return (true);
}


//
// 'gp_rstartpage()' - Start a page.
//

static bool                      // O - `true` on success, `false` on failure
gp_rstartpage(
    pappl_job_t       *job,       // I - Job
    pappl_poptions_t  *options,   // I - Job options
    pappl_device_t    *device,    // I - Device
    unsigned          page)       // I - Page number
{
  stp_dimension_t tmp_left, tmp_right, tmp_top, tmp_bottom;
                                //
  cups_page_header2_t header = options->header;
                                // Page header
  cups_image_t	*cups;					// CUPS image
  stp_vars_t        *gp_data = (stp_vars_t *)papplJobGetData(job);
                                // Job data

  // Check if job is canceled or not...
  if ((cups = (cups_image_t *)(theImage.rep)) == NULL || papplJobIsCanceled(job))
  {
    Image_status = STP_IMAGE_STATUS_ABORT;
    return false;
  }

  cups->header = options->header;

  if (header.cupsBitsPerColor == 16)
    stp_set_string_parameter(gp_data, "ChannelBitDepth", "16");
  else
    stp_set_string_parameter(gp_data, "ChannelBitDepth", "8");

  switch (header.cupsColorSpace)
  {
    case CUPS_CSPACE_W :
      if (options->print_color_mode | PAPPL_COLOR_MODE_MONOCHROME)
        stp_set_string_parameter(gp_data, "PrintingMode", "BW");
      stp_set_string_parameter(gp_data, "InputImageType", "Whitescale");
      break;
    case CUPS_CSPACE_K :
      // DyeSub photo printers don't support black & white ink!
      if (options->print_color_mode | PAPPL_COLOR_MODE_MONOCHROME)
        stp_set_string_parameter(gp_data, "PrintingMode", "BW");
      set_string_parameter(gp_data, "InputImageType", "Grayscale");
      break;
    case CUPS_CSPACE_RGB :
      set_string_parameter(gp_data, "PrintingMode", "Color");
      set_string_parameter(gp_data, "InputImageType", "RGB");
      break;
    case CUPS_CSPACE_CMY :
      set_string_parameter(gp_data, "PrintingMode", "Color");
      set_string_parameter(gp_data, "InputImageType", "CMY");
      break;
    case CUPS_CSPACE_CMYK :
      set_string_parameter(gp_data, "PrintingMode", "Color");
      set_string_parameter(gp_data, "InputImageType", "CMYK");
      break;
    case CUPS_CSPACE_KCMY :
      set_string_parameter(gp_data, "PrintingMode", "Color");
      set_string_parameter(gp_data, "InputImageType", "KCMY");
      break;
    default :
      break;
  }

  set_special_parameter(gp_data, "Resolution", header.cupsCompression - 1);

  set_special_parameter(gp_data, "Quality", header.cupsRowFeed - 1);

  if (strlen(header.MediaClass) > 0)
    stp_set_string_parameter(gp_data, "InputSlot", header.MediaClass);

  if (strlen(header.MediaType) > 0)
    stp_set_string_parameter(gp_data, "MediaType", header.MediaType);

  stp_set_page_width(gp_data, header.PageSize[0]);
  stp_set_page_height(gp_data, header.PageSize[1]);

  // Duplex
  // Note that the names MUST match those in the printer driver(s)

  if (header.Duplex != 0)
  {
    if (header.Tumble != 0)
      set_string_parameter(gp_data, "Duplex", "DuplexTumble");
    else
      set_string_parameter(gp_data, "Duplex", "DuplexNoTumble");
  }

  set_string_parameter(gp_data, "JobMode", "Job");

  // Pass along Collation settings...
  stp_set_boolean_parameter(gp_data, "Collate", header.Collate);
  stp_set_boolean_parameter_active(gp_data, "Collate", STP_PARAMETER_ACTIVE);

  // Pass along Copy settings...
  stp_set_int_parameter(gp_data, "NumCopies", header.NumCopies);
  stp_set_int_parameter_active(gp_data, "NumCopies", STP_PARAMETER_ACTIVE);

  // Pass along the page number...
  stp_set_int_parameter(gp_data, "PageNumber", page);

  // Set height width left and right dimensions...
  stp_set_width(gp_data, options->media.size_width);
  stp_set_height(gp_data, options->media.size_length);
  stp_set_left(gp_data, options->media.left_margin);
  stp_set_top(gp_data, options->media.top_margin);

  cups->adjusted_width = header.cupsWidth;    // Set Width

  cups->adjusted_height = header.cupsHeight;  // Set height

  cups->row = 0;

  return (true);
}


//
// 'gp_rwrite()' - Write a line
//

static bool				// O - `true` on success, `false` on failure
gp_rwrite(
    pappl_job_t         *job,			// I - Job
    pappl_poptions_t    *options,	// I - Job options
    pappl_device_t      *device,	// I - Device
    unsigned            y,				// I - Row number
    const unsigned char *line)		// I - Row
{
  // Push the data to the buffer...
  circular_buf_write(cbuf, line, y);

  return (true);
}


//
//  'Image_get_appname()' - Get the application we are running.
//

static const char *				// O - Application name
Image_get_appname(stp_image_t *image)		// I - Image
{
  (void)image;

  return ("Printer Application based on PAPPL & Gutenprint");
}


//
// 'Image_get_row()' - Get a row of the image
//

static stp_image_status_t
Image_get_row(stp_image_t   *image,		// I - Image
	      unsigned char *data,					// O - Row
	      size_t	    byte_limit,				// I - how many bytes in data
	      int           row)						// I - Row number (unused)
{
  cups_image_t	*cups;					// CUPS image
  size_t bytes_per_line;				// Bytes per line

  if ((cups = (cups_image_t *)(image->rep)) == NULL || Image_status == STP_IMAGE_STATUS_ABORT)
    return STP_IMAGE_STATUS_ABORT;

  bytes_per_line =
    ((cups->adjusted_width * cups->header.cupsBitsPerPixel) + CHAR_BIT - 1) /
    CHAR_BIT;

  if (cups->row < cups->header.cupsHeight)
  {
    while (cups->row <= row && cups->row < cups->header.cupsHeight)
    {
      pthread_mutex_lock(&cbuf->readwrite_lock_mutex);

      if (cbuf->reader == NULL)
      {
        // Handle empty buffer...
        pthread_cond_wait(&cbuf->readwrite_lock_cond, &cbuf->readwrite_lock_mutex);
      }

      // Get the row and data from the reader's end...
      cups->row = cbuf->reader->row;
      memcpy(data, circular_buf_read(cbuf), bytes_per_line);

      pthread_mutex_unlock(&cbuf->readwrite_lock_mutex);
    }
  }
  else
  {
    switch (cups->header.cupsColorSpace)
    {
      case CUPS_CSPACE_K:
      case CUPS_CSPACE_CMYK:
      case CUPS_CSPACE_KCMY:
      case CUPS_CSPACE_CMY:
        memset(data, 0, cups->header.cupsBytesPerLine);
        break;
      case CUPS_CSPACE_RGB:
      case CUPS_CSPACE_W:
        memset(data, ((1 << CHAR_BIT) - 1), cups->header.cupsBytesPerLine);
        break;
      default:
        break;
        return STP_IMAGE_STATUS_ABORT;
    }
  }

  return STP_IMAGE_STATUS_OK;
}


//
// 'Image_height()' - Return the height of an image.
//

static int				// O - Height in pixel
Image_height(stp_image_t *image)	// I - Image
{
  cups_image_t	*cups;		// CUPS image


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return (0);

  return (cups->adjusted_height);
}

//
// 'Image_Init' - Initializes an image
//

static void
Image_init(stp_image_t *image)
{
  /* Nothing to do. */
}


//
// 'Image_width()' - Return the width of an image.
//

static int				// O - Width in pixels
Image_width(stp_image_t *image)	// I - Image
{
  cups_image_t	*cups;		// CUPS image


  if ((cups = (cups_image_t *)(image->rep)) == NULL)
    return (0);

  return (cups->adjusted_width);
}


//
// 'pappl_writefunc()' - Write data to a file...
//

static void
pappl_writefunc(void *file, const char *buf, size_t bytes)
{
  FILE *prn = (FILE *)file;
  fwrite(buf, 1, bytes, prn);
}


//
// 'system_cb()' - System callback.
//

pappl_system_t *			// O - New system object
system_cb(int           num_options,	// I - Number of options
	  cups_option_t *options,	// I - Options
	  void          *data)		// I - Callback data
{
  pappl_system_t	*system;	// System object
  const char		*val,		// Current option value
			*hostname,	// Hostname, if any
			*logfile,	// Log file, if any
			*system_name;	// System name, if any
  pappl_loglevel_t	loglevel;	// Log level
  int			port = 0;	// Port number, if any
  pappl_soptions_t	soptions = PAPPL_SOPTIONS_MULTI_QUEUE | PAPPL_SOPTIONS_STANDARD | PAPPL_SOPTIONS_LOG | PAPPL_SOPTIONS_NETWORK | PAPPL_SOPTIONS_SECURITY | PAPPL_SOPTIONS_TLS;
					// System options
  static pappl_version_t versions[1] =	// Software versions
  {
    { "Gutenprint Printer App", "", "1.0", { 1, 0, 0, 0 } }
  };


  // Parse options...
  if ((val = cupsGetOption("log-level", num_options, options)) != NULL)
  {
    if (!strcmp(val, "fatal"))
      loglevel = PAPPL_LOGLEVEL_FATAL;
    else if (!strcmp(val, "error"))
      loglevel = PAPPL_LOGLEVEL_ERROR;
    else if (!strcmp(val, "warn"))
      loglevel = PAPPL_LOGLEVEL_WARN;
    else if (!strcmp(val, "info"))
      loglevel = PAPPL_LOGLEVEL_INFO;
    else if (!strcmp(val, "debug"))
      loglevel = PAPPL_LOGLEVEL_DEBUG;
    else
    {
      fprintf(stderr, "gutenprint_printer_app: Bad log-level value '%s'.\n", val);
      return (NULL);
    }
  }
  else
    loglevel = PAPPL_LOGLEVEL_UNSPEC;

  logfile     = cupsGetOption("log-file", num_options, options);
  hostname    = cupsGetOption("server-hostname", num_options, options);
  system_name = cupsGetOption("system-name", num_options, options);

  if ((val = cupsGetOption("server-port", num_options, options)) != NULL)
  {
    if (!isdigit(*val & 255))
    {
      fprintf(stderr, "gutenprint_printer_app: Bad server-port value '%s'.\n", val);
      return (NULL);
    }
    else
      port = atoi(val);
  }

  // Create the system object...
  if ((system = papplSystemCreate(soptions, system_name ? system_name : "Gutenprint Printer app", port, "_print,_universal", cupsGetOption("spool-directory", num_options, options), logfile ? logfile : "-", loglevel, cupsGetOption("auth-service", num_options, options), /* tls_only */false)) == NULL)
    return (NULL);

  papplSystemAddListeners(system, NULL);
  papplSystemSetHostname(system, hostname);
  gp_setup(system);

  papplSystemSetFooterHTML(system,
                           "Copyright &copy; 2020 by Michael R Sweet. "
                           "Provided under the terms of the <a href=\"https://www.apache.org/licenses/LICENSE-2.0\">Apache License 2.0</a>.");
  papplSystemSetSaveCallback(system, (pappl_save_cb_t)papplSystemSaveState, (void *)"/tmp/hp_printer_app.state");
  papplSystemSetVersions(system, (int)(sizeof(versions) / sizeof(versions[0])), versions);

  if (!papplSystemLoadState(system, "/tmp/hp_printer_app.state"))
    papplSystemSetDNSSDName(system, system_name ? system_name : "Gutenprint Printer app");

  return (system);
}

