/*

  Stylus Photo Printer driver for ghostscript

  -uses escp2-driver from gimp print plugin V3.0.3

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
/* epson stylus photo  output driver */
#include "gdevprn.h"
#include "gdevpccm.h"
#include "gdevstp.h"
#include "gsparam.h"

#include "gdevstp-print.h"

/* internal debugging output ? */
/* #define DRV_DEBUG */
#undef DRV_DEBUG

/* ------ The device descriptors ------ */

private dev_proc_print_page(stp_print_page);

private dev_proc_get_params(stp_get_params);
private dev_proc_put_params(stp_put_params);

private int stp_put_param_int(P6(gs_param_list *, gs_param_name, int *, int, int, int));
private int stp_put_param_float(P6(gs_param_list *, gs_param_name, float *, float, float, int));
private dev_proc_open_device(stp_open);

/* 24-bit color. ghostscript driver */
private const gx_device_procs stpm_procs =
prn_color_params_procs(
                 stp_open, /*gdev_prn_open,*/ /* open file, delegated to gs */
                 gdev_prn_output_page,     /* output page, delegated to gs */
                 gdev_prn_close,           /* close file, delegated to gs */
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


/* grab resolution list from escp2 driver */
#ifndef res_t
typedef struct {
  const char name[65];
  int hres;
  int vres;
  int softweave;
  int horizontal_passes;
  int vertical_passes;
  int vertical_subsample;
} res_t;
#endif

/* private data structure */
typedef struct {
  int resnr;
  int topoffset;   /* top offset in pixels */
  int bottom;
  int model;
  int algnr;			/* Algorithm */
  vars_t v;
} privdata_t;

/* in gdevstp-escp2.c */
extern const char *escp2_resname(int);

/* global variables, RO for subfunctions */
private uint stp_raster;
private byte *stp_row;
private gx_device_printer *stp_pdev;
static privdata_t stp_data =
{ 0, 0, 0, 0, 0,
  {
    "",				/* output_to */
    "",				/* driver */
    "",				/* PPD file */
    OUTPUT_COLOR,		/* output_type */
    "360 DPI",			/* resolution */
    "Letter",			/* Media size */
    "",				/* Media type */
    "",				/* Media source */
    "",				/* Ink type */
    "",				/* Dither algorithm */
    100,			/* bright     */
    100.0,			/* Scaling */
    ORIENT_PORTRAIT,		/* Orientation */
    0,				/* Left */
    0,				/* Top */
    1,				/* gamma      */
    100,			/* cont       */
    100,			/* r          */
    100,			/* g          */
    100,			/* b          */
    0,				/* lin        */
    1.0,			/* saturation */
    1.0,			/* density */
    0,				/* image type */
    0,				/* unit */
    1.0,			/* application gamma */
    NULL,			/* lookup table */
    NULL			/* Color map */
  }
};

/* ------ Private definitions ------ */

/***********************************************************************
* ghostscript driver function calls                                    *
***********************************************************************/

static void
stp_print_dbg(const char *msg, gx_device_printer *pdev,
	      const privdata_t *stp_data)
{
#ifdef DRV_DEBUG
  if (pdev)
    fprintf(stderr,"%s Image: %d x %d pixels, %f x %f dpi\n",
	    msg, pdev->width, pdev->height, pdev->x_pixels_per_inch,
	    pdev->y_pixels_per_inch);

  fprintf(stderr,"%s Settings: r: %d  g: %d  b: %d\n",
	  msg, stp_data->v.red, stp_data->v.green, stp_data->v.blue);
  fprintf(stderr, "Media size %s\n", stp_data->v.media_size);

  fprintf(stderr,"Settings: model: %d  bright: %d  contrast: %d\n",
	  stp_data->model, stp_data->v.brightness, stp_data->v.contrast);

  fprintf(stderr,"Settings: Gamma: %f  Saturation: %f  Density: %f\n",
	  stp_data->v.gamma, stp_data->v.saturation, stp_data->v.density);
#endif
}

static void
stp_print_debug(const char *msg, gx_device *pdev,
		const privdata_t *stp_data)
{
#ifdef DRV_DEBUG
  if (pdev)
    fprintf(stderr,"%s Image: %d x %d pixels, %f x %f dpi\n",
	    msg, pdev->width, pdev->height, pdev->x_pixels_per_inch,
	    pdev->y_pixels_per_inch);

  fprintf(stderr,"%s Settings: r: %d  g: %d  b: %d\n",
	  msg, stp_data->v.red, stp_data->v.green, stp_data->v.blue);
  fprintf(stderr, "Media size %s\n", stp_data->v.media_size);

  fprintf(stderr,"Settings: model: %d  bright: %d  contrast: %d\n",
	  stp_data->model, stp_data->v.brightness, stp_data->v.contrast);

  fprintf(stderr,"Settings: Gamma: %f  Saturation: %f  Density: %f\n",
	  stp_data->v.gamma, stp_data->v.saturation, stp_data->v.density);
#endif
}


/*----------- Write out a in escp2 format. ---------------*/
private int stp_print_page(gx_device_printer * pdev, FILE * file)
{
  int code;			/* return code */
  int model;
  const printer_t *printer = NULL;
  int i;

  stp_print_dbg("stp_print_page", pdev, &stp_data);
  code = 0;
  stp_pdev = pdev;
  stp_raster = gdev_prn_raster(pdev);
  stp_row = gs_alloc_bytes(pdev->memory, stp_raster, "stp file buffer");

  if (stp_row == 0)		/* can't allocate row buffer */
    return_error(gs_error_VMerror);

#ifdef DRV_DEBUG
  fprintf(stderr,"1 step done!");
#endif

  model = stp_data.model;                 /* 6 = Stylus Photo  */

  /* Ugh */
  for (i = 0; i < known_printers(); i++)
    {
      printer = get_printer_by_index(i);
      if (printer->model == model &&
	  printer->print == escp2_print)
	break;
    }
  if (printer->model != model &&
      printer->print != escp2_print)
    {
      fprintf(stderr, "Unknown printer model\n");
      code = 1;
      return code;
    }

  strcpy(stp_data.v.resolution, escp2_resname(stp_data.resnr));
  strcpy(stp_data.v.dither_algorithm, dither_algo_names[stp_data.algnr]);

  stp_data.v.scaling = -pdev->x_pixels_per_inch; /* resolution of image */

  /* compute lookup table: lut_t*,float dest_gamma,float app_gamma,vars_t* */
  stp_data.v.app_gamma = 1.0;

  stp_data.topoffset = 0;
  stp_data.v.cmap = NULL;
  escp2_print(printer,	/* I - Model */
	      1,		/* I - Number of copies */
	      file,		/* I - File to print to */
	      NULL,		/* I - Image to print (dummy) */
	      &stp_data.v);	/* vars_t * */

  gs_free_object(pdev->memory, stp_row, "stp row buffer");
  stp_row = NULL;

  return code;
}

/* 24-bit color mappers (taken from gdevmem2.c). */

/* Map a r-g-b color to a color index. */
gx_color_index
stp_map_16m_rgb_color(gx_device * dev, gx_color_value r, gx_color_value g,
		  gx_color_value b)
{
  return gx_color_value_to_byte(b) +
    ((uint) gx_color_value_to_byte(g) << 8) +
    ((ulong) gx_color_value_to_byte(r) << 16);
}

/* Map a color index to a r-g-b color. */
int
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
private int stp_get_params(gx_device *pdev, gs_param_list *plist)
{
  int code;
  gs_param_string pmedia;

  stp_print_debug("stp_get_params(0)", pdev, &stp_data);
  code = gdev_prn_get_params(pdev, plist);
  stp_print_debug("stp_get_params(1)", pdev, &stp_data);
  param_string_from_string(pmedia, stp_data.v.media_size);

  if ( code < 0 ||
       (code = param_write_int(plist, "Red", &stp_data.v.red)) < 0 ||
       (code = param_write_int(plist, "Green", &stp_data.v.green)) < 0 ||
       (code = param_write_int(plist, "Blue", &stp_data.v.blue)) < 0 ||
       (code = param_write_int(plist, "Brightness", &stp_data.v.brightness)) < 0 ||
       (code = param_write_int(plist, "Contrast", &stp_data.v.contrast)) < 0 ||
       (code = param_write_int(plist, "Color", &stp_data.v.output_type)) < 0 ||
       (code = param_write_int(plist, "Model", &stp_data.model)) < 0 ||
       (code = param_write_int(plist, "Quality", &stp_data.resnr)) < 0 ||
       (code = param_write_int(plist, "Dither", &stp_data.algnr)) < 0 ||
       (code = param_write_int(plist, "ImageType", &stp_data.v.image_type)) < 0 ||
       (code = param_write_string(plist, "PAPERSIZE", &pmedia)) < 0 ||
       (code = param_write_float(plist, "Gamma", &stp_data.v.gamma)) < 0 ||
       (code = param_write_float(plist, "Saturation", &stp_data.v.saturation)) < 0 ||
       (code = param_write_float(plist, "Density", &stp_data.v.density)) < 0
       )
    return code;

  return 0;
}

/* Put parameters. */
/* Yeah, I could have used a list for the options but... */
private int stp_put_params(gx_device *pdev, gs_param_list *plist)
{
  gs_param_string pmedia;
  int red    = stp_data.v.red;
  int green  = stp_data.v.green;
  int blue   = stp_data.v.blue;
  int bright = stp_data.v.brightness;
  int cont   = stp_data.v.contrast;
  int model  = stp_data.model;
  int color  = stp_data.v.output_type;
  int qual   = stp_data.resnr;
  int algo   = stp_data.algnr;
  int itype  = stp_data.v.image_type;
  float gamma = stp_data.v.gamma;
  float sat = stp_data.v.saturation;
  float den = stp_data.v.density;
  int code   = 0;


  stp_print_debug("stp_put_params", pdev, &stp_data);

  param_string_from_string(pmedia, stp_data.v.media_size);

  code = stp_put_param_int(plist, "Red", &red, 0, 200, code);
  code = stp_put_param_int(plist, "Green", &green, 0, 200, code);
  code = stp_put_param_int(plist, "Blue", &blue, 0, 200, code);
  code = stp_put_param_int(plist, "Brightness", &bright, 0, 400, code);
  code = stp_put_param_int(plist, "Contrast", &cont, 25, 400, code);
  code = stp_put_param_int(plist, "Color", &color, 0, 1, code);
  code = stp_put_param_int(plist, "Model", &model, 0, 24, code);
  code = stp_put_param_int(plist, "Quality", &qual, 0, 9, code);
  code = stp_put_param_int(plist, "ImageType", &itype, 0, 3, code);
  code = stp_put_param_int(plist, "Dither", &algo, 0, num_dither_algos, code);
  code = stp_put_param_float(plist, "Gamma", &gamma, 0.1, 3., code);
  code = stp_put_param_float(plist, "Saturation", &sat, 0.0, 9., code);
  code = stp_put_param_float(plist, "Density", &den, 0.1, 2., code);

  if( param_read_string(plist, "PAPERSIZE", &pmedia) == 0)
    {
      /*
	fprintf(stderr,"Media defined: %s\n",pmedia.data);
      */
    }

  if ( code < 0 )
    return code;

  stp_data.v.red = red;
  stp_data.v.green = green;
  stp_data.v.blue = blue;
  stp_data.v.brightness = bright;
  stp_data.v.contrast = cont;
  stp_data.model = model;
  stp_data.v.output_type = color;
  stp_data.resnr = qual;
  stp_data.algnr = algo;
  stp_data.v.image_type = itype;
  strcpy(stp_data.v.media_size,pmedia.data);
  stp_data.v.gamma = gamma;
  stp_data.v.saturation = sat;
  stp_data.v.density = den;

  {
    byte a;

    a = *stp_data.v.media_size;
    a &= 255-32;      /* quick`n`dirty lcase->ucase for first letter ;-) */
    *stp_data.v.media_size = a;
#if 0
    fprintf(stderr,"Media defined: %s\n",stp_data.v.media_size);
#endif
  }

  code = gdev_prn_put_params(pdev, plist);
  return code;
}

private int stp_put_param_int(gs_param_list *plist,
                                 gs_param_name pname,
                                 int *pvalue,
                                 int minval,
                                 int maxval,
                                 int ecode)
{
  int code, value;

  stp_print_debug("stp_put_param_int", NULL, &stp_data);
  switch ( code = param_read_int(plist, pname, &value) )
    {
    default:
      return code;
    case 1:
      return ecode;
    case 0:
      if ( value < minval || value > maxval )
	{
	  param_signal_error(plist, pname, gs_error_rangecheck);
	  ecode = -100;
        }
      else
	*pvalue = value;

      return (ecode < 0 ? ecode : 1);
    }
}

private int stp_put_param_float(gs_param_list *plist,
                                   gs_param_name pname,
                                   float *pvalue,
                                   float minval,
                                   float maxval,
                                   int ecode)
{
  int code;
  float value;

  stp_print_debug("stp_put_param_float", NULL, &stp_data);
  switch ( code = param_read_float(plist, pname, &value) )
    {
    default:
      return code;
    case 1:
      return ecode;
    case 0:
      if ( value < minval || value > maxval )
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
  char none[5];
  printer_t *printer = get_printer_by_index(stp_data.model);
  if (!printer)
    return (-1);

  stp_print_debug("stp_open", pdev, &stp_data);
  strcpy(none,"");

  default_media_size(printer,
                     none,
                     stp_data.v.media_size,
                     &width,
                     &length);

  escp2_imageable_area(printer,		/* I - Printer model */
                       none,		/* I - PPD file (not used) */
                       stp_data.v.media_size,	/* I - Media size */
                       &left,		/* O - Left position in points */
                       &right,		/* O - Right position in points */
                       &bottom,		/* O - Bottom position in points */
                       &top);		/* O - Top position in points */

  st[1] = (float)bottom / 72;        /* bottom margin */
  st[3] = (float)(length-top) / 72;  /* top margin    */
  st[0] = (float)left / 72;          /* left margin   */
  st[2] = (float)(width-right) / 72; /* right margin  */

  stp_data.v.top    = length-top;
  stp_data.bottom = bottom;

#ifdef DRV_DEBUG
  fprintf(stderr,"margins: %f %f %f %f\n",st[0],st[1],st[2],st[3]);
#endif

  gx_device_set_margins(pdev, st, true);
  return gdev_prn_open(pdev);
}

/***********************************************************************
* escp2 driver function callback routines                              *
***********************************************************************/

/* get one row of the image */
void Image_get_row(Image image, unsigned char *data, int row)
{
  if (stp_pdev->x_pixels_per_inch == stp_pdev->y_pixels_per_inch)
    gdev_prn_copy_scan_lines(stp_pdev, stp_data.topoffset+row,
			     data, stp_raster);
  else if (stp_pdev->x_pixels_per_inch > stp_pdev->y_pixels_per_inch)
    {
      /*
       * If xres > yres, duplicate rows
       */
      int ratio = (stp_pdev->x_pixels_per_inch / stp_pdev->y_pixels_per_inch);
      gdev_prn_copy_scan_lines(stp_pdev, (stp_data.topoffset + row) / ratio,
			       data, stp_raster);
    }
  else
    {
      /*
       * If xres < yres, skip rows
       */
      int ratio = (stp_pdev->y_pixels_per_inch / stp_pdev->x_pixels_per_inch);
      gdev_prn_copy_scan_lines(stp_pdev, (stp_data.topoffset + row) * ratio,
			       data, stp_raster);
    }
}

/* return bpp of picture (24 here) */
int Image_bpp(Image image)
{
  return 3;
}

/* return width of picture */
int Image_width(Image image)
{
  return stp_pdev->width;
}

/*
  return height of picture and
  subtract margins from image size so that the
  driver only reads the correct number of lines from the
  input

*/
int Image_height(Image image)
{
  float tmp,tmp2;

  tmp =   stp_data.v.top + stp_data.bottom; /* top margin + bottom margin */

  /* calculate height in 1/72 inches */
  tmp2 = (float)stp_pdev->height / (float)stp_pdev->x_pixels_per_inch * 72.;

  tmp2 -= tmp;			/* subtract margins from sizes */

  /* calculate new image height */
  tmp2 *= (float)stp_pdev->x_pixels_per_inch / 72.;

#ifdef DRV_DEBUG
  fprintf(stderr,"corrected page length %f\n",tmp2);
#endif

  return (int)tmp2;
}

void Image_rotate_ccw(Image img)
{
 /* dummy function, Landscape printing unsupported atm */
}

void Image_init(Image image)
{
 /* dummy function */
}

void Image_progress_init(Image image)
{
 /* dummy function */
}

/* progress display */
void Image_note_progress(Image image, double current, double total)
{
 /* dummy function */
}
