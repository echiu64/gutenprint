/*
 *  $Id$
 *
 *   ijs server for gimp-print.
 *
 *   Copyright 2001 Robert Krawitz (rlk@alum.mit.edu)
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
 * Revision History:
 *
 *   See ChangeLog
 */

/* ijs server for gimp-print */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ijs.h>
#include <ijs_server.h>


/* WARNING:
 * gimp-print wants to pull the raster data.  
 * We can't push the raster data to gimp-print.  
 * All gimp-print drivers as of 2001-11-02 read rows on increasing 
 * order, sometimes with skipped rows.  They never ask for an earlier 
 * row.  To avoid keeping the entire image in memory we only buffer 
 * one row at a time and hope that the gimp-print drivers don't change 
 * their behaviour.
 * An interface like gdk_draw_rgb_image() would have been easier.
 */

#define BUF_SIZE 4096

typedef struct _GimpParamList GimpParamList;

struct _GimpParamList {
  GimpParamList *next;
  char *key;
  char *value;
  int value_size;
};

typedef struct _IMAGE
{
  IjsServerCtx *ctx;
  stp_vars_t v;
  char filename[256];	/* OutputFile */
  int width;		/* pixels */
  int height;		/* pixels */
  int bps;		/* bytes per sample */
  int n_chan;		/* number of channels */
  int xres;		/* dpi */
  int yres;
  int output_type;
  int row;		/* row number in buffer */
  int row_width;	/* length of a row */
  char *row_buf;	/* buffer for raster */
  int total_bytes;	/* total size of raster */
  int bytes_left;	/* bytes remaining to be read */
  GimpParamList *params;
} IMAGE;

static const char DeviceGray[] = "DeviceGray";
static const char DeviceRGB[] = "DeviceRGB";
static const char DeviceCMYK[] = "DeviceCMYK";

static int
image_init(IMAGE *img, IjsPageHeader *ph)
{
  img->width = ph->width;
  img->height = ph->height;
  img->bps = ph->bps;
  img->n_chan = ph->n_chan;
  img->xres = ph->xres;
  img->yres = ph->yres;

  img->row = 0;
  img->row_width = (ph->n_chan * ph->bps * ph->width + 7) >> 3;
  img->row_buf = (char *)malloc(img->row_width);

  if ((img->bps == 1) && (img->n_chan == 1) &&
      (strncmp(ph->cs, DeviceGray, strlen(DeviceGray)) == 0))
    {
      img->output_type = OUTPUT_MONOCHROME;
      /* 8-bit greyscale */
    }
  else if ((img->bps == 8) && (img->n_chan == 1) &&
      (strncmp(ph->cs, DeviceGray, strlen(DeviceGray)) == 0))
    {
      img->output_type = OUTPUT_GRAY;
      /* 8-bit greyscale */
    }
  else if ((img->bps == 8) && (img->n_chan == 3) &&
	   (strncmp(ph->cs, DeviceRGB, strlen(DeviceRGB)) == 0))
    {
      img->output_type = OUTPUT_COLOR;
      /* 24-bit colour */
    }
  else if ((img->bps == 8) && (img->n_chan == 4) && 
	   (strncmp(ph->cs, DeviceCMYK, strlen(DeviceCMYK)) == 0))
    {
      img->output_type = OUTPUT_RAW_CMYK;
      /* 32-bit CMYK colour */
    }
  else
    {
      fprintf(stderr, "Bad cs, bps %d chan %d space %s\n",
	      img->bps, img->n_chan, ph->cs);
      /* unsupported */
      return -1;
    }

  if (img->row_buf == NULL)
    {
      fprintf(stderr, "No row buffer\n");
      return -1;
    }

  return 0;
}

static void
image_finish(IMAGE *img)
{
  if (img->row_buf);
  free(img->row_buf);
  img->row_buf = NULL;
}

static int
get_float(const char *str, float *pval, float min_value, float max_value)
{
  float new_value;
  if (sscanf(str, "%f", &new_value) == 1)
    {
      if ((new_value >= min_value) && (new_value <= max_value))
	{
	  *pval = new_value;
	  return 0;
	}
    }
  return -1;
}

static int
get_int(const char *str, int *pval, int min_value, int max_value)
{
  int new_value;
  if (sscanf(str, "%d", &new_value) == 1)
    {
      if ((new_value >= min_value) && (new_value <= max_value))
	{
	  *pval = new_value;
	  return 0;
	}
    }
  return -1;
}

/* A C implementation of /^(\d\.+\-eE)+x(\d\.+\-eE)+$/ */
static int
gimp_parse_wxh (const char *val, int size,
		   double *pw, double *ph)
{
  char buf[256];
  char *tail;
  int i;

  for (i = 0; i < size; i++)
    if (val[i] == 'x')
      break;

  if (i + 1 >= size)
    return IJS_ESYNTAX;

  if (i >= sizeof(buf))
    return IJS_EBUF;

  memcpy (buf, val, i);
  buf[i] = 0;
  *pw = strtod (buf, &tail);
  if (tail == buf)
    return IJS_ESYNTAX;

  if (size - i > sizeof(buf))
    return IJS_EBUF;

  memcpy (buf, val + i + 1, size - i - 1);
  buf[size - i - 1] = 0;
  *ph = strtod (buf, &tail);
  if (tail == buf)
    return IJS_ESYNTAX;

  return 0;
}

/**
 * gimp_find_key: Search parameter list for key.
 *
 * @key: key to look up
 *
 * Return value: GimpParamList entry matching @key, or NULL.
 **/
static GimpParamList *
gimp_find_key (GimpParamList *pl, const char *key)
{
  GimpParamList *curs;

  for (curs = pl; curs != NULL; curs = curs->next)
    {
      if (!strcmp (curs->key, key))
	return curs;
    }
  return NULL;
}

static int
gimp_status_cb (void *status_cb_data,
		IjsServerCtx *ctx,
		IjsJobId job_id)
{
  return 0;
}

static int
gimp_list_cb (void *list_cb_data,
	      IjsServerCtx *ctx,
	      IjsJobId job_id,
	      char *val_buf,
	      int val_size)
{
  const char *param_list = "OutputFile,DeviceManufacturer,DeviceModel,Quality,MediaName,MediaType,MediaSource,InkType,Dither,ImageType,Brightness,Gamma,Contrast,Cyan,Magenta,Yellow,Saturation,Density,PrintableArea,PrintableTopLeft,TopLeft,Dpi";
  int size = strlen (param_list);

  fprintf (stderr, "gimp_list_cb\n");

  if (size > val_size)
    return IJS_EBUF;

  memcpy (val_buf, param_list, size);
  return size;
}

static int
gimp_enum_cb (void *enum_cb_data,
	      IjsServerCtx *ctx,
	      IjsJobId job_id,
	      const char *key,
	      char *val_buf,
	      int val_size)
{
  const char *val = NULL;
  if (!strcmp (key, "ColorSpace"))
    val = "DeviceRGB,DeviceGray,DeviceCMYK";
  else if (!strcmp (key, "DeviceManufacturer"))
    val = "Gimp-Print";
  else if (!strcmp (key, "DeviceModel"))
    val = "gimp-print";
  else if (!strcmp (key, "PageImageFormat"))
    val = "Raster";

  if (val == NULL)
    return IJS_EUNKPARAM;
  else
    {
      int size = strlen (val);

      if (size > val_size)
	return IJS_EBUF;
      memcpy (val_buf, val, size);
      return size;
    }
}

static int
gimp_get_cb (void *get_cb_data,
	     IjsServerCtx *ctx,
	     IjsJobId job_id,
	     const char *key,
	     char *val_buf,
	     int val_size)
{
  IMAGE *img = (IMAGE *)get_cb_data;
  stp_vars_t v = img->v;
  stp_printer_t printer = stp_get_printer_by_driver(stp_get_driver(v));
  GimpParamList *pl = img->params;
  GimpParamList *curs;
  const char *val;
  char buf[256];

  fprintf (stderr, "gimp_get_cb: %s\n", key);
  if (!printer)
    {
      if (strlen(stp_get_driver(v)) == 0)
	fprintf(stderr, "Printer must be specified with -sModel\n");
      else
	fprintf(stderr, "Printer %s is not a known model\n",
		stp_get_driver(v));
      return IJS_EUNKPARAM;
    }
  curs = gimp_find_key (pl, key);
  if (curs != NULL)
    {
      if (curs->value_size > val_size)
	return IJS_EBUF;
      memcpy (val_buf, curs->value, curs->value_size);
      return curs->value_size;
    }

  if (!strcmp(key, "PrintableArea"))
    {
      int l, r, b, t;
      int h, w;
      (*stp_printer_get_printfuncs(printer)->imageable_area)
	(printer, v, &l, &r, &b, &t);
      h = t - b;
      w = r - l;
      sprintf(buf, "%gx%g", (double) w / 72.0, (double) h / 72.0);
      fprintf(stderr, "PrintableArea %d %d %s\n", h, w, buf);
      val = buf;
    }
  if (!strcmp(key, "Dpi"))
    {
      int x, y;
      (*stp_printer_get_printfuncs(printer)->describe_resolution)
	(printer, stp_get_resolution(v), &x, &y);
      sprintf(buf, "%d", x);
      fprintf(stderr, "Dpi %d %d (%d) %s\n", x, y, x, buf);
      stp_set_scaling(v, -x);
      val = buf;
    }

  if (!strcmp(key, "PrintableTopLeft"))
    {
      int l, r, b, t;
      int h, w;
      (*stp_printer_get_printfuncs(printer)->media_size)
	(printer, v, &w, &h);
      (*stp_printer_get_printfuncs(printer)->imageable_area)
	(printer, v, &l, &r, &b, &t);
      t = h - t;
      sprintf(buf, "%gx%g", (double) l / 72.0, (double) t / 72.0);
      fprintf(stderr, "PrintableTopLeft %d %d %s\n", t, l, buf);
      val = buf;
    }

  if (!strcmp (key, "DeviceManufacturer"))
    val = "Gimp-Print";
  else if (!strcmp (key, "DeviceModel"))
    val = stp_get_driver(img->v);
  else if (!strcmp (key, "PageImageFormat"))
    val = "Raster";

  if (val == NULL)
    return IJS_EUNKPARAM;
  else
    {
      int size = strlen (val);

      if (size > val_size)
	return IJS_EBUF;
      memcpy (val_buf, val, size);
      return size;
    }
}

static int
gimp_set_cb (void *set_cb_data, IjsServerCtx *ctx, IjsJobId jobid,
	     const char *key, const char *value, int value_size)
{
  int code = 0;
  char vbuf[256];
  const stp_vars_t lower = stp_minimum_settings();
  const stp_vars_t upper = stp_maximum_settings();
  int i;
  float z;
  IMAGE *img = (IMAGE *)set_cb_data;
  fprintf (stderr, "gimp_set_cb: %s=", key);
  fwrite (value, 1, value_size, stderr);
  fputs ("\n", stderr);
  if (value_size > sizeof(vbuf)-1)
    return -1;
  memset(vbuf, 0, sizeof(vbuf));
  memcpy(vbuf, value, value_size);

  if (strcmp(key, "OutputFile") == 0)
    strncpy(img->filename, vbuf, sizeof(img->filename)-1);
  else if (strcmp(key, "DeviceManufacturer") == 0)
    ;				/* We don't care who makes it */
  else if (strcmp(key, "DeviceModel") == 0)
    {
      stp_printer_t printer = stp_get_printer_by_driver(vbuf);
      stp_set_driver(img->v, vbuf);
      if (printer)
	{
	  stp_set_printer_defaults(img->v, printer, NULL);
	  if (strlen(stp_get_resolution(img->v)) == 0)
	    stp_set_resolution(img->v,
			       ((*stp_printer_get_printfuncs(printer)->default_parameters)
				(printer, NULL, "Resolution")));
	  if (strlen(stp_get_dither_algorithm(img->v)) == 0)
	    stp_set_dither_algorithm(img->v, stp_default_dither_algorithm());
	}
      else
	code = IJS_ERANGE;
    }
  else if (strcmp(key, "PPDFile") == 0)
    stp_set_ppd_file(img->v, vbuf);
  else if (strcmp(key, "Quality") == 0)
    stp_set_resolution(img->v, vbuf);
  else if (strcmp(key, "MediaName") == 0)
    stp_set_media_size(img->v, vbuf);
  else if (strcmp(key, "TopLeft") == 0)
    {
      double w, h;
      code = gimp_parse_wxh(vbuf, strlen(vbuf), &w, &h);
      if (code == 0)
	{
	  fprintf(stderr, "left top %f %f %s\n", w * 72, h * 72, vbuf);
	  stp_set_left(img->v, w * 72);
	  stp_set_top(img->v, h * 72);
	}
    }      
  else if (strcmp(key, "PaperSize") == 0)
    {
      double w, h;
      code = gimp_parse_wxh(vbuf, strlen(vbuf), &w, &h);
      if (code == 0)
	{
	  fprintf(stderr, "paper size %f %f %s\n", w * 72, h * 72, vbuf);
	  stp_set_page_width(img->v, w * 72);
	  stp_set_page_height(img->v, h * 72);
	}
    }
  else if (strcmp(key, "MediaType") == 0)
    stp_set_media_type(img->v, vbuf);
  else if (strcmp(key, "MediaSource") == 0)
    stp_set_media_source(img->v, vbuf);
  else if (strcmp(key, "InkType") == 0)
    stp_set_ink_type(img->v, vbuf);
  else if (strcmp(key, "Dither") == 0)
    stp_set_dither_algorithm(img->v, vbuf);
  else if (strcmp(key, "ImageType") == 0)
    {
      code = get_int(vbuf, &i,
		     stp_get_image_type(lower), stp_get_image_type(upper));
      if (code == 0)
	stp_set_image_type(img->v, i);
    }
  else if (strcmp(key, "Brightness") == 0)
    {
      code = get_float(vbuf, &z,
		     stp_get_brightness(lower), stp_get_brightness(upper));
      if (code == 0)
	stp_set_brightness(img->v, z);
    }
  else if (strcmp(key, "Gamma") == 0)
    {
      code = get_float(vbuf, &z, 
		       stp_get_gamma(lower), stp_get_gamma(upper));
      if (code == 0)
	stp_set_gamma(img->v, z);
    }
  else if (strcmp(key, "Contrast") == 0)
    {
      code = get_float(vbuf, &z, 
		       stp_get_contrast(lower), stp_get_contrast(upper));
      if (code == 0)
	stp_set_contrast(img->v, z);
    }
  else if (strcmp(key, "Cyan") == 0)
    {
      code = get_float(vbuf, &z, 
		       stp_get_cyan(lower), stp_get_cyan(upper));
      if (code == 0)
	stp_set_cyan(img->v, z);
    }
  else if (strcmp(key, "Magenta") == 0)
    {
      code = get_float(vbuf, &z, 
		       stp_get_magenta(lower), stp_get_magenta(upper));
      if (code == 0)
	stp_set_magenta(img->v, z);
    }
  else if (strcmp(key, "Yellow") == 0)
    {
      code = get_float(vbuf, &z, 
		       stp_get_yellow(lower), stp_get_yellow(upper));
      if (code == 0)
	stp_set_yellow(img->v, z);
    }
  else if (strcmp(key, "Saturation") == 0)
    {
      code = get_float(vbuf, &z, 
		       stp_get_saturation(lower), stp_get_saturation(upper));
      if (code == 0)
	stp_set_saturation(img->v, z);
    }
  else if (strcmp(key, "Density") == 0)
    {
      code = get_float(vbuf, &z, 
		       stp_get_density(lower), stp_get_density(upper));
      if (code == 0)
	stp_set_density(img->v, z);
    }
  else
    {
      fprintf(stderr, "Unknown key %s\n", key);
      code = -1;
    }

  if (code == 0)
    {
      GimpParamList *pl = gimp_find_key (img->params, key);

      if (pl == NULL)
	{
	  pl = (GimpParamList *)malloc (sizeof (GimpParamList));
	  pl->next = img->params;
	  pl->key = malloc (strlen(key) + 1);
	  memcpy (pl->key, key, strlen(key) + 1);
	  img->params = pl;
	}
      else
	{
	  free (pl->value);
	}
      pl->value = malloc (value_size);
      memcpy (pl->value, value, value_size);
      pl->value_size = value_size;
    }

  return code;
}

/**********************************************************/

static void
gimp_outfunc(void *data, const char *buffer, size_t bytes)
{
  if ((data != NULL) && (buffer != NULL) && (bytes != 0))
    fwrite(buffer, 1, bytes, (FILE *)data);
}

/**********************************************************/
/* stp_image_t functions */

static void
gimp_image_init(stp_image_t *image)
{
}

static void
gimp_image_reset(stp_image_t *image)
{
}

/* bytes per pixel (NOT bits per pixel) */
static int
gimp_image_bpp(stp_image_t *image)
{
  IMAGE *img = (IMAGE *)(image->rep);
  fprintf(stderr, "gimp_image_bpp: bps=%d n_chan=%d returning %d\n", 
	  img->bps, img->n_chan, img->bps * img->n_chan / 8);
  return img->bps * img->n_chan / 8;
}

static int
gimp_image_width(stp_image_t *image)
{
  IMAGE *img = (IMAGE *)(image->rep);
  return img->width;
}

static int
gimp_image_height(stp_image_t *image)
{
  IMAGE *img = (IMAGE *)(image->rep);
  return img->height;
}

static int
image_next_row(IMAGE *img)
{
  int status = 0;
  int n_bytes = img->bytes_left;
  if (img->bytes_left)
    {

      if (n_bytes > img->row_width)
	n_bytes = img->row_width;
#ifdef VERBOSE
      fprintf(stderr, "%d bytes left, reading %d, on row %d\n", 
	      bytes_left, n_bytes, img->row);
#endif
      status = ijs_server_get_data(img->ctx, img->row_buf, n_bytes);
      if (status)
	{
	  fprintf(stderr, "page aborted!\n");
	}
      else
	{
	  img->row++;
	  img->bytes_left -= n_bytes;
	}
    }
  else
    return 1;	/* Done */
  return status;
}

static stp_image_status_t 
gimp_image_get_row(stp_image_t *image, unsigned char *data, int row)
{
  IMAGE *img = (IMAGE *)(image->rep);

  if ((row < 0) || (row >= img->height))
    return STP_IMAGE_ABORT;
  if (row < img->row)
    {
      fprintf(stderr, "ijsgimp: You must ask for lines in order\n");
      return STP_IMAGE_ABORT;
    }

  /* Read until we reach the requested row. */
  while (row > img->row)
    {
      if (image_next_row(img))
	return STP_IMAGE_ABORT;
    }

  if (row == img->row)
    memcpy(data, img->row_buf, img->row_width);
  else
    return STP_IMAGE_ABORT;
  return STP_IMAGE_OK;
}


static const char *
gimp_image_get_appname(stp_image_t *image)
{
  return "ijsgimp";
}

static void
gimp_image_progress_init(stp_image_t *image)
{
}

static void
gimp_image_note_progress(stp_image_t *image, double current, double total)
{
  char buf[256];
  sprintf(buf, "%.1f of %.1f\n", current, total);
  gimp_outfunc(stderr, buf, strlen(buf));
}

static void
gimp_image_progress_conclude(stp_image_t *image)
{
}

/**********************************************************/


int
main (int argc, char **argv)
{
  IjsPageHeader ph;
  int status;
  int page = 0;
  IMAGE img;
  stp_image_t si;
  stp_printer_t printer = NULL;
  FILE *f;

  memset(&img, 0, sizeof(img));

  img.ctx = ijs_server_init();
  if (img.ctx == NULL)
    return 1;

  stp_init();
  img.v = stp_allocate_vars();
  if (img.v == NULL)
    {
      ijs_server_done(img.ctx);
      return 1;
    }

  /* Error messages to stderr. */
  stp_set_errfunc(img.v, gimp_outfunc);
  stp_set_errdata(img.v, stderr);

  /* Printer data goes to file f, but we haven't opened it yet. */
  stp_set_outfunc(img.v, gimp_outfunc);
  stp_set_outdata(img.v, NULL);

  memset(&si, 0, sizeof(si));
  si.init = gimp_image_init;
  si.reset = gimp_image_reset;
  si.transpose = NULL;
  si.hflip = NULL;
  si.vflip = NULL;
  si.crop = NULL;
  si.rotate_ccw = NULL;
  si.rotate_cw = NULL;
  si.rotate_180 = NULL;
  si.bpp = gimp_image_bpp;
  si.width = gimp_image_width;
  si.height = gimp_image_height;
  si.get_row = gimp_image_get_row;
  si.get_appname = gimp_image_get_appname;
  si.progress_init = gimp_image_progress_init;
  si.note_progress = gimp_image_note_progress;
  si.progress_conclude = gimp_image_progress_conclude;
  si.rep = &img;

  ijs_server_install_status_cb (img.ctx, gimp_status_cb, &img);
  ijs_server_install_list_cb (img.ctx, gimp_list_cb, &img);
  ijs_server_install_enum_cb (img.ctx, gimp_enum_cb, &img);
  ijs_server_install_get_cb (img.ctx, gimp_get_cb, &img);
  ijs_server_install_set_cb(img.ctx, gimp_set_cb, &img);

  do
    {
      status = ijs_server_get_page_header(img.ctx, &ph);
      if (status) break;
      fprintf (stderr, "got page header, %d x %d\n",
	       ph.width, ph.height);

      status = image_init(&img, &ph);
      if (status)
	{
	  fprintf(stderr, "image_init failed %d\n", status);
	  break;
	}

      if (page == 0)
	{
	  if (strlen(img.filename) == 0)
	    status = -1;
	  if (status) 
	    {
	      fprintf(stderr, "img.filename failed %d\n", status);
	      break;
	    }

	  /* FIX - also support popen */
	  if ((f = fopen(img.filename, "wb")) == (FILE *)NULL)
	    status = -1;
	  if (status) 
	    {
	      fprintf(stderr, "fopen img.filename failed %d\n", status);
	      break;
	    }

	  /* Printer data to file */
	  stp_set_outdata(img.v, f);

	  printer = stp_get_printer_by_driver(stp_get_driver(img.v));
	  if (printer == NULL)
	    {
	      fprintf(stderr, "Unknown gimp-print driver\n");
	      status = -1;
	      break;
	    }
	  stp_merge_printvars(img.v, stp_printer_get_printvars(printer));
	  if (strlen(stp_get_resolution(img.v)) == 0)
	    stp_set_resolution(img.v, 
			       stp_printer_get_printfuncs(printer)->
			       default_parameters(printer, NULL, "Resolution"));
	  if (strlen(stp_get_dither_algorithm(img.v)) == 0)
	    stp_set_dither_algorithm(img.v, stp_default_dither_algorithm());
	}

      page++;

      img.total_bytes = ((ph.n_chan * ph.bps * ph.width + 7) >> 3) 
	* ph.height;
      img.bytes_left = img.total_bytes;

      stp_set_app_gamma(img.v, (float)1.7);
      stp_set_cmap(img.v, NULL);
      stp_set_scaling(img.v, (float)-img.xres); /* resolution of image */
      stp_set_output_type(img.v, img.output_type); 
      if (stp_printer_get_printfuncs(printer)->verify(printer, img.v))
	{
	  stp_printer_get_printfuncs(printer)->print(printer, &si, img.v);
	}
      else
	{
	  fprintf(stderr, "Couldn't verify gimp-print printer\n");
	  status = -1;
	  break;
	}

      while (img.bytes_left)
	{
	  status = image_next_row(&img);
	  if (status)
	    {
	      fprintf(stderr, "next row failed at %d\n", img.bytes_left);
	      break;
	    }
	}

      image_finish(&img);
    }
  while (status == 0);

  if (status > 0)
    status = 0; /* normal exit */

  ijs_server_done(img.ctx);

#ifdef VERBOSE
  fprintf (stderr, "server exiting with status %d\n", status);
#endif
  return status;
}
