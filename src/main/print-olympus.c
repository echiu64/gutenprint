/*
 * "$Id$"
 *
 *   Print plug-in Olympus driver for the GIMP.
 *
 *   Copyright 2003 Michael Mraka (Michael.Mraka@linux.cz)
 *
 *   The plug-in is based on the code of the RAW plugin for the GIMP of
 *   Michael Sweet (mike@easysw.com) and Robert Krawitz (rlk@alum.mit.edu)
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
#include "module.h"

#ifdef __GNUC__
#define inline __inline__
#endif

typedef struct
{
  int color_model;
  int output_channels;
  const char *name;
} ink_t;

typedef struct olympus_printer /* printer specific parameters */
{
  int model;		/* printer model */
  const char *papersize;
  int border_left;	/* unit 1/72"  */
  int border_right;
  int border_top;
  int border_bottom;
} olympus_printer_t;

static const olympus_printer_t olympus_model_capabilities[] =
{
	{ 0, "A6", 28, 28, 49, 48},	/* model P300 */
};

static const ink_t inks[] =
{
  { COLOR_MODEL_CMY, 3, "CMY" },
};

static const int ink_count = sizeof(inks) / sizeof(ink_t);

static const stp_parameter_t the_parameters[] =
{
  {
    "PageSize", N_("Page Size"),
    N_("Size of the paper being printed to"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_PAGE_SIZE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
  {
    "MediaType", N_("Media Type"),
    N_("Type of media (plain paper, photo paper, etc.)"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
  {
    "InputSlot", N_("Media Source"),
    N_("Source (input slot) of the media"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
  {
    "InkType", N_("Ink Type"),
    N_("Type of ink in the printer"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
  {
    "Resolution", N_("Resolution"),
    N_("Resolution and quality of the print"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, -1, 1
  },
};

static int the_parameter_count =
sizeof(the_parameters) / sizeof(const stp_parameter_t);

static stp_parameter_list_t
olympus_list_parameters(stp_const_vars_t v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  return ret;
}

static void
olympus_parameters(stp_const_vars_t v, const char *name,
	       stp_parameter_t *description)
{
  int		i;
  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  description->deflt.str = NULL;
  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stpi_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }
  if (strcmp(name, "PageSize") == 0)
    {
      int papersizes = stp_known_papersizes();
      int model = stpi_get_model_id(v);
      description->bounds.str = stp_string_list_create();
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t *pt = stp_get_papersize_by_index(i);
	  if (strcmp(pt->name,
	        olympus_model_capabilities[model].papersize) == 0)
	    {
	      stp_string_list_add_string(description->bounds.str,
					pt->name, pt->text);
	      break;
	    }
	}
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "MediaType") == 0)
  {
    description->bounds.str = stp_string_list_create();
    description->is_active = 0;
  }
  else if (strcmp(name, "InputSlot") == 0)
  {
    description->bounds.str = stp_string_list_create();
    description->is_active = 0;
  }
  else if (strcmp(name, "Resolution") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string(description->bounds.str, "306x306",
        "306x306 DPI"); 
      stp_string_list_add_string(description->bounds.str, "153x153",
        "153x153 DPI"); 
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else if (strcmp(name, "InkType") == 0)
    {
      description->bounds.str = stp_string_list_create();
      for (i = 0; i < ink_count; i++)
	stp_string_list_add_string(description->bounds.str,
				  inks[i].name, inks[i].name);
      description->deflt.str =
	stp_string_list_param(description->bounds.str, 0)->name;
    }
  else
    description->is_active = 0;
}


static void
olympus_imageable_area(stp_const_vars_t v,
		   int  *left,
		   int  *right,
		   int  *bottom,
		   int  *top)
{
  int width, height;
  int model = stpi_get_model_id(v);
  stpi_default_media_size(v, &width, &height);
  
  *left = olympus_model_capabilities[model].border_left;
  *top = olympus_model_capabilities[model].border_top;
  *right = width - olympus_model_capabilities[model].border_right;
  *bottom = height - olympus_model_capabilities[model].border_bottom;
}

static void
olympus_limit(stp_const_vars_t v,			/* I */
	    int *width, int *height,
	    int *min_width, int *min_height)
{
  *width = 65535;
  *height = 65535;
  *min_width = 1;
  *min_height =	1;
}

static void
olympus_describe_resolution(stp_const_vars_t v, int *x, int *y)
{
  const char *resolution = stp_get_string_parameter(v, "Resolution");
  *x = -1;
  *y = -1;
  if (resolution)
    sscanf(resolution, "%dx%d", x, y);
  return;
}

/*
 * olympus_print()
 */
static int
olympus_print(stp_const_vars_t v, stp_image_t *image)
{
  int i, j;
  int y;		/* Looping vars */
  stp_vars_t	nv = stp_vars_create_copy(v);
  int out_channels;
  unsigned short *final_out = NULL;
  unsigned char  *char_out;
  unsigned short *real_out;
  int status = 1;
  int ink_channels = 1;
  const char *ink_type = stp_get_string_parameter(nv, "InkType");
  unsigned zero_mask;

  int xdpi, ydpi;	/* Resolution */

  /* image in pixels */
  unsigned short image_width  = stpi_image_width(image);
  unsigned short image_height = stpi_image_height(image);
  unsigned short image_left, image_right, image_top, image_bottom;

  /* output in 1/72" */
  int width  = stp_get_width(v);
  int height = stp_get_height(v);
  int left   = stp_get_left(v);
  int top    = stp_get_top(v);

  /* page in 1/72" */
  int page_width = stp_get_page_width(v);
  int page_height = stp_get_page_height(v);
  int page_left, page_right, page_top, page_bottom;

  /* page w/out borders in pixels (according to selected dpi) */
  int print_width, print_height;
  
  unsigned char  copies = 1;
  unsigned short hi_speed = 1;
  char *l, layers[] = "YMC";
  unsigned char *zeros;

  if (!stp_verify(nv))
    {
      stpi_eprintf(nv, _("Print options not verified; cannot print.\n"));
      stp_vars_free(nv);
      return 0;
    }

  stp_describe_resolution(nv, &xdpi, &ydpi);
  olympus_imageable_area(v, &page_left, &page_right,
    &page_bottom, &page_top);

  print_width  = (page_right - page_left) * xdpi/72;
  print_height = (((page_bottom - page_top) * ydpi/72) | 0x000f) + 1;

  image_left   = (left - page_left) * xdpi / 72;
  image_left   = (image_left + image_width > print_width ?
    print_width - image_width : image_left); /* we correct bad rounding */
  image_top    = (top  - page_top)  * ydpi / 72;
  image_top    = (image_top + image_height > print_height ?
    print_height - image_height: image_top);
  image_right  = image_left + image_width;
  image_bottom = image_top  + image_height;
  
/*
  stpi_eprintf(v, "paper        %d x %d\n"
		"image        %d x %d\n"
		"image(72dpi) %d x %d\n"
		"* out        %d x %d\n"
		"* left x top %d x %d\n"
		"image left x top %d x %d\n"
		"border (%d - %d) = %d x (%d - %d) = %d\n"
		"printable pixels   %d x %d\n"
		"res                %d x %d\n",
		page_width, page_height,
		image_width, image_height,
		image_width * 72 / xdpi, image_height * 72 / ydpi,
		width, height,
		left, top,
		image_left, image_top,
		page_right, page_left, page_right - page_left,
		  page_bottom, page_top, page_bottom - page_top,
		print_width, print_height,
		xdpi, ydpi
		);	
*/

  /* skip the job if image is not approximately the same size as output */
  if (width - image_width * 72 / xdpi < -2
      || width - image_width * 72 / xdpi > 2
      || height - image_height * 72 / ydpi < -2
      || height - image_height * 72 / ydpi > 2)
    {
      stpi_eprintf(nv, _("This driver is under development.\n\n"
        "It can't rescale your image at present. \n"
	"Your print job is canceled now(!).\n"
	"Please scale your image suitably and try it again.\n"
	"Thank you."));
      stp_vars_free(nv);
      return 0;
    }

  
  stpi_set_output_color_model(nv, COLOR_MODEL_CMY);
  
  if (ink_type)
    {
      for (i = 0; i < ink_count; i++)
	if (strcmp(ink_type, inks[i].name) == 0)
	  {
	    stpi_set_output_color_model(nv, inks[i].color_model);
	    ink_channels = inks[i].output_channels;
	    break;
	  }
    }

  stpi_channel_reset(nv);
  for (i = 0; i < ink_channels; i++)
    stpi_channel_add(nv, i, 0, 1.0);

  out_channels = stpi_color_init(nv, image, 256);

  if (out_channels != ink_channels && out_channels != 1 && ink_channels != 1)
    {
      stpi_eprintf(nv, _("Internal error!  Output channels or input channels must be 1\n"));
      stp_vars_free(nv);
      return 0;
    }

  if (out_channels != ink_channels)
    final_out = stpi_malloc(print_width * ink_channels * 2);

  stp_set_float_parameter(nv, "Density", 1.0);

  stpi_image_progress_init(image);

  zeros = stpi_zalloc(print_width+1);
  
  /* printer init */
  stpi_zfwrite("\033\033\033C\033N", 1, 6, nv);
  stpi_putc(copies, nv);
  stpi_zfwrite("\033F", 1, 2, nv);
  stpi_putc(hi_speed >> 8, nv);
  stpi_putc(hi_speed & 0xff, nv);
  stpi_zfwrite("\033MS\xff\xff\xff\033Z", 1, 8, nv);
  stpi_putc(xdpi >> 8, nv);
  stpi_putc(xdpi & 0xff, nv);
  stpi_putc(ydpi >> 8, nv);
  stpi_putc(ydpi & 0xff, nv);
  
  
  l = layers;
  while (*l)
    {
#if 1
    /* old style - full 4MB prn file */
#define MAX_PROGRESS (print_height)
    for (y = 0; y < print_height; y++)
		  
#else
    /*
     * new style - only used lines (mod 16)
     * the same idea for columns does not work - we must print
     * all columns (include empty ones) :(
     */
#define MAX_PROGRESS (((image_bottom - 1) | 0x000f) - (image_top & 0xfff0))
    for (y = (image_top & 0xfff0); y <= ((image_bottom - 1) | 0x000f); y++)
#endif
      {
      unsigned short *out;
      if ((y % 16) == 0)
        {
        /* block init */
	stpi_zfwrite("\033\033\033W", 1, 4, nv);
	stpi_putc(*l, nv);
	stpi_putc(y >> 8, nv);
	stpi_putc(y & 0xff, nv);
	stpi_putc(0 >> 8, nv);
	stpi_putc(0 & 0xff, nv);
	stpi_putc((y + 15) >> 8, nv);
	stpi_putc((y + 15) & 0xff, nv);
	stpi_putc((print_width - 1) >> 8, nv);
	stpi_putc((print_width - 1) & 0xff, nv);
	}
      

      if (y < image_top || y >= image_bottom)
        {
	stpi_zfwrite((char *) zeros, 1, print_width, nv);
	}
      else
        {
        if (image_left > 0)
	  {
          stpi_zfwrite((char *) zeros, 1, image_left, nv);
/* stpi_erprintf("left %d ", image_left); */
	  }
        if ((y & 63) == 0)
          {
    	  stpi_image_note_progress(image, MAX_PROGRESS/3 * (l - layers) + y / 3,
            MAX_PROGRESS);
          }

        if (stpi_color_get_row(nv, image, y - image_top, &zero_mask))
          {
  	  status = 2;
  	  break;
          }
	out = stpi_channel_get_input(nv);
        real_out = out;
        if (out_channels != ink_channels)
  	{
  	  real_out = final_out;
  	  if (out_channels < ink_channels)
  	    {
  	      for (i = 0; i < image_width; i++)
  		{
  		  for (j = 0; j < ink_channels; j++)
  		    final_out[i * ink_channels + j] = out[i];
  		}
  	    }
  	  else
  	    {
  	      for (i = 0; i < image_width; i++)
  		{
  		  int avg = 0;
  		  for (j = 0; j < out_channels; j++)
  		    avg += out[i * out_channels + j];
  		  final_out[i] = avg / out_channels;
  		}
  	    }
  	}
  	char_out = (unsigned char *) real_out;
  	for (i = 0; i < image_width; i++)
  	  char_out[i] = real_out[i * ink_channels + (layers - l + 2)] / 257;
  
	stpi_zfwrite((char *) real_out, 1, image_width, nv);
/* stpi_erprintf("data %d ", image_width); */
        if (image_right < print_width)
	  {
          stpi_zfwrite((char *) zeros, 1, print_width - image_right, nv);
/* stpi_erprintf("right %d ", print_width - image_right); */
	  }
/* stpi_erprintf("\n"); */
	}
      }
      /* layer end */
      stpi_zfwrite("\033\033\033P", 1, 4, nv);
      stpi_putc(*l, nv);
      stpi_zfwrite("S", 1, 1, nv);

      l++;
    }
  stpi_image_progress_conclude(image);
  if (final_out)
    stpi_free(final_out);
  stp_vars_free(nv);
  return status;
}

static const stpi_printfuncs_t print_olympus_printfuncs =
{
  olympus_list_parameters,
  olympus_parameters,
  stpi_default_media_size,
  olympus_imageable_area,
  olympus_limit,
  olympus_print,
  olympus_describe_resolution,
  stpi_verify_printer_params,
  NULL,
  NULL
};




static stpi_internal_family_t print_olympus_module_data =
  {
    &print_olympus_printfuncs,
    NULL
  };


static int
print_olympus_module_init(void)
{
  return stpi_family_register(print_olympus_module_data.printer_list);
}


static int
print_olympus_module_exit(void)
{
  return stpi_family_unregister(print_olympus_module_data.printer_list);
}


/* Module header */
#define stpi_module_version print_olympus_LTX_stpi_module_version
#define stpi_module_data print_olympus_LTX_stpi_module_data

stpi_module_version_t stpi_module_version = {0, 0};

stpi_module_t stpi_module_data =
  {
    "olympus",
    VERSION,
    "Olympus family driver",
    STPI_MODULE_CLASS_FAMILY,
    NULL,
    print_olympus_module_init,
    print_olympus_module_exit,
    (void *) &print_olympus_module_data
  };
