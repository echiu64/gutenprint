/*
 * "$Id$"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
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
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gimp-print/gimp-print.h>
#include "gimp-print-internal.h"
#include <gimp-print/gimp-print-intl-internal.h>
#include <string.h>
#include <stdio.h>

#ifdef __GNUC__
#define inline __inline__
#endif

typedef struct
{
  int color_model;
  const char *name;
} ink_t;

static const ink_t inks[] =
{
  { COLOR_MODEL_CMY, "RGB" },
  { COLOR_MODEL_CMY, "CMY" },
};

static const int ink_count = sizeof(inks) / sizeof(ink_t);

static void
raw_parameters(const stp_vars_t v, const char *name,
	       stp_parameter_t *description)
{
  int		i;
  description->type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  stp_fill_parameter_settings(description, name);
  description->deflt.str = NULL;
  if (strcmp(name, "PageSize") == 0)
    {
      int papersizes = stp_known_papersizes();
      description->bounds.str = stp_string_list_allocate();
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t pt = stp_get_papersize_by_index(i);
	  if (stp_papersize_get_width(pt) == 0 &&
	      stp_papersize_get_height(pt) == 0)
	    {
	      stp_string_list_add_param(description->bounds.str,
					stp_papersize_get_name(pt),
					stp_papersize_get_text(pt));
	      break;
	    }
	}
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "InkType") == 0)
    {
      description->bounds.str = stp_string_list_allocate();
      for (i = 0; i < ink_count; i++)
	stp_string_list_add_param(description->bounds.str,
				  inks[i].name, inks[i].name);
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }      
  else if ((strcmp(name, "Resolution") == 0) ||
	   (strcmp(name, "MediaType") == 0) ||
	   (strcmp(name, "InputSlot") == 0))
    {
      description->bounds.str = stp_string_list_allocate();
      stp_string_list_add_param(description->bounds.str,
				"Standard", "Standard");
    }
  else
    stp_describe_internal_parameter(v, name, description);
}

/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

static void
raw_imageable_area(const stp_vars_t v,
		   int  *left,
		   int  *right,
		   int  *bottom,
		   int  *top)
{
  *left = 0;
  *top = 0;
  *bottom = stp_get_page_width(v);
  *top = stp_get_page_height(v);
}

static void
raw_limit(const stp_vars_t v,			/* I */
	    int *width, int *height,
	    int *min_width, int *min_height)
{
  *width = 65535;
  *height = 65535;
  *min_width = 1;
  *min_height =	1;
}

static void
raw_describe_resolution(const stp_vars_t v, int *x, int *y)
{
  *x = 72;
  *y = 72;
}

/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
static int
raw_print(const stp_vars_t v, stp_image_t *image)
{
  int width = stp_get_page_width(v);
  int height = stp_get_page_height(v);
  int		i;
  int		y;		/* Looping vars */
  stp_convert_t	colorfunc;	/* Color conversion function... */
  stp_vars_t	nv = stp_allocate_copy(v);
  int out_channels;
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned short *final_out = NULL;
  unsigned char	*in;		/* Input pixels */
  int		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  int		status = 1;

  if (!stp_verify(nv))
    {
      stp_eprintf(nv, _("Print options not verified; cannot print.\n"));
      return 0;
    }

  for (i = 0; i < ink_count; i++)
    if (strcmp(stp_get_string_parameter(nv, "InkType"), inks[i].name) == 0)
      stp_set_output_color_model(nv, inks[i].color_model);
  colorfunc = stp_choose_colorfunc(nv, image->bpp(image), &out_channels);

  in  = stp_malloc(image->width(image) * image->bpp(image));
  out = stp_malloc(image->width(image) * out_channels * 2);
  if (width != image->width(image))
    final_out = stp_malloc(width * out_channels * 2);

  stp_set_float_parameter(nv, "Density", 1.0);
  stp_compute_lut(nv, 256, NULL, NULL, NULL);

  image->progress_init(image);
  errdiv  = image->height(image) / height;
  errmod  = image->height(image) % height;
  errval  = 0;
  errlast = -1;
  errline  = 0;

  for (y = 0; y < height; y++)
    {
      int duplicate_line = 1;
      int zero_mask;
      if ((y & 63) == 0)
	image->note_progress(image, y, height);

      if (errline != errlast)
	{
	  errlast = errline;
	  duplicate_line = 0;
	  if (image->get_row(image, in, errline) != STP_IMAGE_OK)
	    {
	      status = 2;
	      break;
	    }
	  (*colorfunc)(nv, in, out, &zero_mask, image->width(image),
		       image->bpp(image));
	}
      if (width != image->width(image))
	{
	  int xerrdiv = image->width(image) / width;
	  int xerrmod = image->width(image) % width;
	  int xerrval = 0;
	  int xerrline = 0;
	  int x;
	  for (x = 0; x < width; x++)
	    {
	      memcpy(final_out + out_channels * 2 * x,
		     out + out_channels * 2 * xerrline,
		     out_channels * 2);
	      xerrval += xerrmod;
	      xerrline += xerrdiv;
	      if (xerrval >= width)
		{
		  xerrval -= width;
		  xerrline ++;
		}
	    }
	  stp_zfwrite((char *) final_out, width * out_channels * 2, 1, nv);
	}
      else
	stp_zfwrite((char *) out, width * out_channels * 2, 1, nv);
      errval += errmod;
      errline += errdiv;
      if (errval >= height)
	{
	  errval -= height;
	  errline ++;
	}
    }
  image->progress_conclude(image);
  if (final_out)
    stp_free(final_out);
  stp_free(out);
  stp_free(in);
  stp_free_vars(nv);
  return status;
}

const stp_printfuncs_t stp_raw_printfuncs =
{
  raw_parameters,
  stp_default_media_size,
  raw_imageable_area,
  raw_limit,
  raw_print,
  raw_describe_resolution,
  stp_verify_printer_params,
  NULL,
  NULL
};
