/*

  stp driver for ghostscript

  -gs frontend derived from gdevbmp and gdevcdj


  written in January 2000 by
  Henryk Richter <buggs@comlab.uni-rostock.de>
  for ghostscript 5.x/6.x


  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/
/*$Id$ */
/* stp output driver */
#include "gdevprn.h"
#include "gdevpccm.h"
#include "gsparam.h"

#include "gdevstp-print.h"

/* internal debugging output ? */

private int stp_debug = 0;

#define STP_DEBUG(x)				\
if (stp_debug) x

/* ------ The device descriptors ------ */

#define X_DPI 360
#define Y_DPI 360

private dev_proc_map_rgb_color(stp_map_16m_rgb_color);
private dev_proc_map_color_rgb(stp_map_16m_color_rgb);
private dev_proc_print_page(stp_print_page);
private dev_proc_get_params(stp_get_params);
private dev_proc_put_params(stp_put_params);
private int stp_put_param_int(P6(gs_param_list *, gs_param_name, int *,
				 int, int, int));
private int stp_put_param_float(P6(gs_param_list *, gs_param_name,
				   float *, float, float, int));
private dev_proc_open_device(stp_open);

/* 24-bit color. ghostscript driver */
private const gx_device_procs stpm_procs =
prn_color_params_procs(
		       stp_open, /*gdev_prn_open,*/ /* open file, delegated */
		       gdev_prn_output_page,     /* output page, delegated */
		       gdev_prn_close,           /* close file, delegated */
		       stp_map_16m_rgb_color, /* map color (own) */
		       stp_map_16m_color_rgb, /* map color (own) */
		       stp_get_params,        /* get params (own) */
		       stp_put_params         /* put params (own) */
		       );

gx_device_printer gs_stp_device =
prn_device(stpm_procs, "stp",
	   DEFAULT_WIDTH_10THS, DEFAULT_HEIGHT_10THS,
	   X_DPI, Y_DPI,
	   0, 0, 0, 0,		/* margins */
	   24, stp_print_page);

/* private data structure */
typedef struct
{
  int topoffset;   /* top offset in pixels */
  int bottom;
  vars_t v;
} privdata_t;

/* global variables, RO for subfunctions */
private privdata_t stp_data =
{ 0, 0,
  {
    "",				/* output_to */
    "",				/* driver */
    "",				/* PPD file */
    OUTPUT_COLOR,		/* output_type */
    "",				/* resolution */
    "Letter",			/* Media size */
    "",				/* Media type */
    "",				/* Media source */
    "",				/* Ink type */
    "",				/* Dither algorithm */
    1.0,			/* bright     */
    100.0,			/* Scaling */
    ORIENT_PORTRAIT,		/* Orientation */
    0,				/* Left */
    0,				/* Top */
    1,				/* gamma      */
    1.0,			/* cont       */
    1.0,			/* c          */
    1.0,			/* m          */
    1.0,			/* y          */
    0,				/* lin        */
    1.0,			/* saturation */
    1.0,			/* density */
    0,				/* image type */
    0,				/* unit */
    1.0,			/* application gamma */
    0,				/* Page width */
    0,				/* Page height */
    NULL,			/* lookup table */
    NULL			/* Color map */
  }
};

typedef struct
{
  gx_device_printer *dev;
  privdata_t *data;
  uint raster;
} stp_image_t;



/* ------ Private definitions ------ */

/***********************************************************************
* ghostscript driver function calls                                    *
***********************************************************************/

private void
stp_dbg(const char *msg, const privdata_t *stp_data)
{
  fprintf(gs_stderr,"%s Settings: r: %f  g: %f  b: %f\n",
	  msg, stp_data->v.cyan, stp_data->v.magenta, stp_data->v.yellow);
  fprintf(gs_stderr, "Ink type %s\n", stp_data->v.ink_type);

  fprintf(gs_stderr,"Settings: bright: %f  contrast: %f\n",
	  stp_data->v.brightness, stp_data->v.contrast);

  fprintf(gs_stderr,"Settings: Gamma: %f  Saturation: %f  Density: %f\n",
	  stp_data->v.gamma, stp_data->v.saturation, stp_data->v.density);
  fprintf(gs_stderr, "Settings: width %d, height %d\n",
	  stp_data->v.page_width, stp_data->v.page_height);
  fprintf(gs_stderr, "Settings: Quality %s\n", stp_data->v.resolution);
  fprintf(gs_stderr, "Settings: Dither %s\n", stp_data->v.dither_algorithm);
  fprintf(gs_stderr, "Settings: InputSlot %s\n", stp_data->v.media_source);
  fprintf(gs_stderr, "Settings: MediaType %s\n", stp_data->v.media_type);
  fprintf(gs_stderr, "Settings: MediaSize %s\n", stp_data->v.media_size);
  fprintf(gs_stderr, "Settings: Model %s\n", stp_data->v.driver);
  fprintf(gs_stderr, "Settings: InkType %s\n", stp_data->v.ink_type);
}

private void
stp_print_dbg(const char *msg, gx_device_printer *pdev,
	      const privdata_t *stp_data)
{
  STP_DEBUG(if (pdev)
	    fprintf(gs_stderr,"%s Image: %d x %d pixels, %f x %f dpi\n",
		    msg, pdev->width, pdev->height, pdev->x_pixels_per_inch,
		    pdev->y_pixels_per_inch));
  STP_DEBUG(stp_dbg(msg, stp_data));
}

private void
stp_print_debug(const char *msg, gx_device *pdev,
		const privdata_t *stp_data)
{
  STP_DEBUG(if (pdev)
	    fprintf(gs_stderr,"%s Image: %d x %d pixels, %f x %f dpi\n",
		    msg, pdev->width, pdev->height, pdev->x_pixels_per_inch,
		    pdev->y_pixels_per_inch));
  STP_DEBUG(stp_dbg(msg, stp_data));
}


private int
stp_print_page(gx_device_printer * pdev, FILE * file)
{
  stp_image_t theImage;
  int code;			/* return code */
  const printer_t *printer = NULL;
  uint stp_raster;
  byte *stp_row;
  const papersize_t *p;

  stp_print_dbg("stp_print_page", pdev, &stp_data);
  code = 0;
  stp_raster = gdev_prn_raster(pdev);
  printer = get_printer_by_driver(stp_data.v.driver);
  if (printer == NULL)
    {
      fprintf(gs_stderr, "Printer %s is not a known printer model\n",
	      stp_data.v.driver);
      return_error(gs_error_rangecheck);
    }

  stp_row = gs_alloc_bytes(pdev->memory, stp_raster, "stp file buffer");

  if (stp_row == 0)		/* can't allocate row buffer */
    return_error(gs_error_VMerror);

  if (strlen(stp_data.v.resolution) == 0)
    strcpy(stp_data.v.resolution, (*printer->default_resolution)(printer));
  if (strlen(stp_data.v.dither_algorithm) == 0)
    strcpy(stp_data.v.dither_algorithm, default_dither_algorithm());

  stp_data.v.scaling = -pdev->x_pixels_per_inch; /* resolution of image */

  /* compute lookup table: lut_t*,float dest_gamma,float app_gamma,vars_t* */
  stp_data.v.app_gamma = 1.0;

  stp_data.topoffset = 0;
  stp_data.v.cmap = NULL;

  stp_data.v.page_width = pdev->MediaSize[0];
  stp_data.v.page_height = pdev->MediaSize[1];
  if ((p =
       get_papersize_by_size(stp_data.v.page_height, stp_data.v.page_width)) !=
      NULL)
    strcpy(stp_data.v.media_size, p->name);
  stp_print_dbg("stp_print_page", pdev, &stp_data);
    
  theImage.dev = pdev;
  theImage.data = &stp_data;
  theImage.raster = stp_raster;
  if (verify_printer_params(printer, &(stp_data.v)))
    (*printer->print)(printer,		/* I - Model */
		      1,		/* I - Number of copies */
		      file,		/* I - File to print to */
		      &theImage,	/* I - Image to print (dummy) */
		      &stp_data.v);	/* vars_t * */
  else
    code = 1;

  gs_free_object(pdev->memory, stp_row, "stp row buffer");
  stp_row = NULL;

  if (code)
    return_error(gs_error_rangecheck);
  else
    return 0;
}

/* 24-bit color mappers (taken from gdevmem2.c). */

/* Map a r-g-b color to a color index. */
private gx_color_index
stp_map_16m_rgb_color(gx_device * dev, gx_color_value r, gx_color_value g,
		  gx_color_value b)
{
  return gx_color_value_to_byte(b) +
    ((uint) gx_color_value_to_byte(g) << 8) +
    ((ulong) gx_color_value_to_byte(r) << 16);
}

/* Map a color index to a r-g-b color. */
private int
stp_map_16m_color_rgb(gx_device * dev, gx_color_index color,
		  gx_color_value prgb[3])
{
  prgb[2] = gx_color_value_from_byte(color & 0xff);
  prgb[1] = gx_color_value_from_byte((color >> 8) & 0xff);
  prgb[0] = gx_color_value_from_byte(color >> 16);
  return 0;
}


/*
 * Get parameters.  In addition to the standard and printer
 * parameters, we supply a lot of options to play around with
 * for maximum quality out of the photo printer
*/
/* Yeah, I could have used a list for the options but... */
private int
stp_get_params(gx_device *pdev, gs_param_list *plist)
{
  int code;
  gs_param_string pinktype;
  gs_param_string pmodel;
  gs_param_string pmediatype;
  gs_param_string pInputSlot;
  gs_param_string palgorithm;
  gs_param_string pquality;

#if 0
  stp_print_debug("stp_get_params(0)", pdev, &stp_data);
#endif
  code = gdev_prn_get_params(pdev, plist);
#if 0
  stp_print_debug("stp_get_params(1)", pdev, &stp_data);
#endif
  param_string_from_string(pmediatype, stp_data.v.media_type);
  param_string_from_string(pInputSlot, stp_data.v.media_source);
  param_string_from_string(pinktype, stp_data.v.ink_type);
  param_string_from_string(pmodel, stp_data.v.driver);
  param_string_from_string(palgorithm, stp_data.v.dither_algorithm);
  param_string_from_string(pquality, stp_data.v.resolution);

  if (code < 0 ||
      (code = param_write_float(plist, "Cyan", &stp_data.v.cyan)) < 0 ||
      (code = param_write_float(plist, "Magenta", &stp_data.v.magenta)) < 0 ||
      (code = param_write_float(plist, "Yellow", &stp_data.v.yellow)) < 0 ||
      (code = param_write_float(plist, "Brightness", &stp_data.v.brightness)) < 0 ||
      (code = param_write_float(plist, "Contrast", &stp_data.v.contrast)) < 0 ||
      (code = param_write_int(plist, "Color", &stp_data.v.output_type)) < 0 ||
      (code = param_write_int(plist, "ImageType", &stp_data.v.image_type)) < 0 ||
      (code = param_write_float(plist, "Gamma", &stp_data.v.gamma)) < 0 ||
      (code = param_write_float(plist, "Saturation", &stp_data.v.saturation)) < 0 ||
      (code = param_write_float(plist, "Density", &stp_data.v.density)) < 0 ||
      (code = param_write_string(plist, "Model", &pinktype)) < 0 ||
      (code = param_write_string(plist, "Dither", &palgorithm)) < 0 ||
      (code = param_write_string(plist, "Quality", &pquality)) < 0 ||
      (code = param_write_string(plist, "InkType", &pinktype) < 0) ||
      (code = param_write_string(plist, "MediaType", &pmediatype)) < 0 ||
      (code = param_write_string(plist, "stpMediaType", &pmediatype)) < 0 ||
      (code = param_write_string(plist, "InputSlot", &pInputSlot)) < 0
      )
    return code;

  return 0;
}

private void
gsncpy(char *d, const gs_param_string *s, int limit)
{
  if (limit > s->size)
    limit = s->size;
  strncpy(d, s->data, limit);
  d[limit] = '\000';
}

/* Put parameters. */
/* Yeah, I could have used a list for the options but... */

private int
stp_put_params(gx_device *pdev, gs_param_list *plist)
{
  const vars_t *lower = print_minimum_settings();
  const vars_t *upper = print_maximum_settings();
  gs_param_string pmediatype;
  gs_param_string pInputSlot;
  gs_param_string pinktype;
  gs_param_string pmodel;
  gs_param_string palgorithm;
  gs_param_string pquality;
  int code   = 0;

  stp_print_debug("stp_put_params(0)", pdev, &stp_data);

  param_string_from_string(pmodel, stp_data.v.driver);
  param_string_from_string(pInputSlot, stp_data.v.media_source);
  param_string_from_string(pmediatype, stp_data.v.media_type);
  param_string_from_string(pinktype, stp_data.v.ink_type);
  param_string_from_string(palgorithm, stp_data.v.dither_algorithm);
  param_string_from_string(pquality, stp_data.v.resolution);

  code = stp_put_param_float(plist, "Cyan", &stp_data.v.cyan,
			     lower->cyan, upper->cyan, code);
  code = stp_put_param_float(plist, "Magenta", &stp_data.v.magenta,
			     lower->magenta, upper->magenta, code);
  code = stp_put_param_float(plist, "Yellow", &stp_data.v.yellow,
			     lower->yellow, upper->yellow, code);
  code = stp_put_param_float(plist, "Brightness", &stp_data.v.brightness,
			     lower->brightness, upper->brightness, code);
  code = stp_put_param_float(plist, "Contrast", &stp_data.v.contrast,
			     lower->contrast, upper->contrast, code);
  code = stp_put_param_int(plist, "Color", &stp_data.v.output_type,
			   0, 1, code);
  code = stp_put_param_int(plist, "ImageType", &stp_data.v.image_type,
			   0, NIMAGE_TYPES, code);
  code = stp_put_param_float(plist, "Gamma", &stp_data.v.gamma,
			     lower->gamma, upper->gamma, code);
  code = stp_put_param_float(plist, "Saturation", &stp_data.v.saturation,
			     lower->saturation, upper->saturation, code);
  code = stp_put_param_float(plist, "Density", &stp_data.v.density,
			     lower->density, upper->density, code);
  param_read_string(plist, "Quality", &pquality);
  param_read_string(plist, "Dither", &palgorithm);
  param_read_string(plist, "InputSlot", &pInputSlot);
  param_read_string(plist, "stpMediaType", &pmediatype);
  if (pmediatype.data && strlen(pmediatype.data) == 0)
    param_read_string(plist, "MediaType", &pmediatype);
  param_read_string(plist, "Model", &pmodel);
  param_read_string(plist, "InkType", &pinktype);

  if ( code < 0 )
    return code;

  gsncpy(stp_data.v.driver, &pmodel, sizeof(stp_data.v.driver) - 1);
  gsncpy(stp_data.v.media_type, &pmediatype,
	 sizeof(stp_data.v.media_type) - 1);
  gsncpy(stp_data.v.media_source, &pInputSlot,
	 sizeof(stp_data.v.media_source) - 1);
  gsncpy(stp_data.v.ink_type, &pinktype,
	 sizeof(stp_data.v.ink_type) - 1);
  gsncpy(stp_data.v.dither_algorithm, &palgorithm,
	 sizeof(stp_data.v.dither_algorithm) - 1);
  gsncpy(stp_data.v.resolution, &pquality,
	 sizeof(stp_data.v.resolution) - 1);
  stp_print_debug("stp_put_params(1)", pdev, &stp_data);

  code = gdev_prn_put_params(pdev, plist);
  return code;
}

private int
stp_put_param_int(gs_param_list *plist,
		  gs_param_name pname,
		  int *pvalue,
		  int minval,
		  int maxval,
		  int ecode)
{
  int code, value;

#if 0
  stp_print_debug("stp_put_param_int", NULL, &stp_data);
#endif
  code = param_read_int(plist, pname, &value);
  switch (code)
    {
    default:
      return code;
    case 1:
      return ecode;
    case 0:
      if (value < minval || value > maxval)
	{
	  param_signal_error(plist, pname, gs_error_rangecheck);
	  ecode = -100;
        }
      else
	*pvalue = value;

      return (ecode < 0 ? ecode : 1);
    }
}

private int
stp_put_param_float(gs_param_list *plist,
		    gs_param_name pname,
		    float *pvalue,
		    float minval,
		    float maxval,
		    int ecode)
{
  int code;
  float value;

#if 0
  stp_print_debug("stp_put_param_float", NULL, &stp_data);
#endif
  code = param_read_float(plist, pname, &value);
  switch (code)
    {
    default:
      return code;
    case 1:
      return ecode;
    case 0:
      if (value < minval || value > maxval)
	{
	  param_signal_error(plist, pname, gs_error_rangecheck);
	  ecode = -100;
        }
      else
	*pvalue = value;

      return (ecode < 0 ? ecode : 1);
    }
}

private int
stp_open(gx_device *pdev)
{
  /* Change the margins if necessary. */
  float st[4];
  int left,right,bottom,top,width,length;
  const printer_t *printer = get_printer_by_driver(stp_data.v.driver);
  if (!printer)
    {
      if (strlen(stp_data.v.driver) == 0)
	fprintf(gs_stderr, "Printer must be specified with -sModel\n");
      else
	fprintf(gs_stderr, "Printer %s is not a known model\n",
		stp_data.v.driver);
      return_error(gs_error_undefined);
    }

  stp_data.v.page_width = pdev->MediaSize[0];
  stp_data.v.page_height = pdev->MediaSize[1];
  stp_print_debug("stp_open", pdev, &stp_data);

  (*printer->media_size)(printer,
			 &(stp_data.v),
			 &width,
			 &length);

  (*printer->imageable_area)(printer,	/* I - Printer model */
			     &(stp_data.v),
			     &left,	/* O - Left position in points */
			     &right,	/* O - Right position in points */
			     &bottom,	/* O - Bottom position in points */
			     &top);	/* O - Top position in points */

  st[1] = (float)bottom / 72;        /* bottom margin */
  st[3] = (float)(length-top) / 72;  /* top margin    */
  st[0] = (float)left / 72;          /* left margin   */
  st[2] = (float)(width-right) / 72; /* right margin  */

  stp_data.v.top    = 0;
  stp_data.bottom = bottom + length-top;

  STP_DEBUG(fprintf(gs_stderr, "margins:  l %f  b %f  r %f  t %f\n",
		    st[0], st[1], st[2], st[3]));

  gx_device_set_margins(pdev, st, true);
  return gdev_prn_open(pdev);
}


/***********************************************************************
* driver function callback routines                                    *
***********************************************************************/

/* get one row of the image */
void
Image_get_row(Image image, unsigned char *data, int row)
{
  stp_image_t *im = (stp_image_t *) image;
  memset(data, 0, im->dev->width * 3);
  if (im->dev->x_pixels_per_inch == im->dev->y_pixels_per_inch)
    {
      gdev_prn_copy_scan_lines(im->dev, im->data->topoffset+row,
			       data, im->raster);
    }
  else if (im->dev->x_pixels_per_inch > im->dev->y_pixels_per_inch)
    {
      /*
       * If xres > yres, duplicate rows
       */
      int ratio = (im->dev->x_pixels_per_inch / im->dev->y_pixels_per_inch);
      gdev_prn_copy_scan_lines(im->dev, (im->data->topoffset + row) / ratio,
			       data, im->raster);
    }
  else
    {
      /*
       * If xres < yres, skip rows
       */
      int ratio = (im->dev->y_pixels_per_inch / im->dev->x_pixels_per_inch);
      gdev_prn_copy_scan_lines(im->dev, (im->data->topoffset + row) * ratio,
			       data, im->raster);
    }
}

/* return bpp of picture (24 here) */
int
Image_bpp(Image image)
{
  return 3;
}

/* return width of picture */
int
Image_width(Image image)
{
  stp_image_t *im = (stp_image_t *) image;
  return im->dev->width;
}

/*
  return height of picture and
  subtract margins from image size so that the
  driver only reads the correct number of lines from the
  input

*/
int
Image_height(Image image)
{
  stp_image_t *im = (stp_image_t *) image;
  float tmp,tmp2;

  tmp = im->data->v.top + im->data->bottom; /* top margin + bottom margin */

  /* calculate height in 1/72 inches */
  tmp2 = (float)(im->dev->height) / (float)(im->dev->y_pixels_per_inch) * 72.;

  tmp2 -= tmp;			/* subtract margins from sizes */

  /* calculate new image height */
  tmp2 *= (float)(im->dev->x_pixels_per_inch) / 72.;

  STP_DEBUG(fprintf(gs_stderr,"corrected page length %f\n",tmp2));

  return (int)tmp2;
}

void
Image_rotate_ccw(Image img)
{
 /* dummy function, Landscape printing unsupported atm */
}

void
Image_rotate_cw(Image img)
{
 /* dummy function, Seascape printing unsupported atm */
}

void
Image_rotate_180(Image img)
{
 /* dummy function,  upside down printing unsupported atm */
}

void
Image_init(Image image)
{
 /* dummy function */
}

void
Image_progress_init(Image image)
{
 /* dummy function */
}

/* progress display */
void
Image_note_progress(Image image, double current, double total)
{
  STP_DEBUG(fprintf(gs_stderr, "."));
}

void
Image_progress_conclude(Image image)
{
  STP_DEBUG(fprintf(gs_stderr, "\n"));
}

const char *
Image_get_appname(Image image)
{
  return "GhostScript/stp";
}
