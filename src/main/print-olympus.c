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

#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))

typedef struct
{
  int color_model;
  int output_channels;
  const char *name;
} ink_t;

typedef struct {
  const char *name;
  const char *text;
  int xdpi;
  int ydpi;
  int x_max_res;	/* maximum width in pixels */
  int y_max_res;	/* maximum height in pixels */
} olympus_res_t;

#define OLYMPUS_RES_COUNT	5
typedef olympus_res_t olympus_res_t_array[OLYMPUS_RES_COUNT];

typedef struct /* printer specific parameters */
{
  int model;		/* printer model number from printers.xml*/
  int max_paper_width;  /* maximum printable paper size in 1/72 inch */
  int max_paper_height;
  int min_paper_width;	/* minimum printable paper size in 1/72 inch */
  int min_paper_height;
  int border_left;	/* unprintable borders - unit 1/72"  */
  int border_right;
  int border_top;
  int border_bottom;
  const olympus_res_t_array *res;	/* list of possible resolutions */
  int block_size;
  int need_empty_cols;	/* must we print empty columns? */
  int need_empty_rows;	/* must we print empty rows? */
} olympus_cap_t;

static const olympus_res_t_array resolution_p300 = 
{
	{ "306x306", N_ ("306x306 DPI"), 306, 306, 1024, 1376 },
	{ "153x153", N_ ("153x153 DPI"), 153, 153, 512, 688 },
	{ "", "", 0, 0, 0, 0 }
};

static const olympus_cap_t olympus_model_capabilities[] =
{
	{ 0, 		/* model P300 */
		297, 432,	/* "A6", "4x6" */
		288, 420,	
		28, 28, 49, 48,
		&resolution_p300,
		16,
		1, 0,
	},
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

typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
  int color_only;
} float_param_t;

static const float_param_t float_parameters[] =
{
  {
    {
      "CyanDensity", N_("Cyan Balance"),
      N_("Adjust the cyan balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 1, 1
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "MagentaDensity", N_("Magenta Balance"),
      N_("Adjust the magenta balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 2, 1
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "YellowDensity", N_("Yellow Balance"),
      N_("Adjust the yellow balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 3, 1
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "BlackDensity", N_("Black Balance"),
      N_("Adjust the black balance"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED, 0, 1, 0, 1
    }, 0.0, 2.0, 1.0, 1
  },
  {
    {
      "LightCyanTransition", N_("Light Cyan Transition"),
      N_("Light Cyan Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1
    }, 0.0, 5.0, 1.0, 1
  },
  {
    {
      "LightMagentaTransition", N_("Light Magenta Transition"),
      N_("Light Magenta Transition"),
      STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
      STP_PARAMETER_LEVEL_ADVANCED4, 0, 1, -1, 1
    }, 0.0, 5.0, 1.0, 1
  },
};    

static const int float_parameter_count =
sizeof(float_parameters) / sizeof(const float_param_t);

static const olympus_cap_t* olympus_get_model_capabilities(int model)
{
  int i;
  int models = sizeof(olympus_model_capabilities) / sizeof(olympus_cap_t);

  for (i=0; i<models; i++) {
    if (olympus_model_capabilities[i].model == model) {
      return &(olympus_model_capabilities[i]);
    }
  }
  stpi_deprintf(STPI_DBG_OLYMPUS,
  	"olympus: model %d not found in capabilities list.\n", model);
  return &(olympus_model_capabilities[0]);
}

static const olympus_res_t*
olympus_get_res_params(int model, const char *resolution)
{
  const olympus_cap_t *caps = olympus_get_model_capabilities(model);
  const olympus_res_t *res  = *(caps->res);

  if (resolution) {
    while (res->xdpi) {
      if (strcmp(resolution, res->name) == 0) {
	return res;
      }
      res++;
    }
  }
  stpi_erprintf("olympus_get_res_params: resolution not found (%s)\n", resolution);
  return NULL;
}

static stp_parameter_list_t
olympus_list_parameters(stp_const_vars_t v)
{
  stp_parameter_list_t *ret = stp_parameter_list_create();
  int i;
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(the_parameters[i]));
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param(ret, &(float_parameters[i].param));
  return ret;
}

static void
olympus_parameters(stp_const_vars_t v, const char *name,
	       stp_parameter_t *description)
{
  int		i;
  const olympus_cap_t *caps = olympus_get_model_capabilities(
		  				stpi_get_model_id(v));

  description->p_type = STP_PARAMETER_TYPE_INVALID;
  if (name == NULL)
    return;

  description->deflt.str = NULL;
  for (i = 0; i < float_parameter_count; i++)
    if (strcmp(name, float_parameters[i].param.name) == 0)
      {
	stpi_fill_parameter_settings(description,
				     &(float_parameters[i].param));
	description->deflt.dbl = float_parameters[i].defval;
	description->bounds.dbl.upper = float_parameters[i].max;
	description->bounds.dbl.lower = float_parameters[i].min;
      }

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp(name, the_parameters[i].name) == 0)
      {
	stpi_fill_parameter_settings(description, &(the_parameters[i]));
	break;
      }
  if (strcmp(name, "PageSize") == 0)
    {
      int papersizes = stp_known_papersizes();

      description->bounds.str = stp_string_list_create();
      for (i = 0; i < papersizes; i++)
	{
	  const stp_papersize_t *pt = stp_get_papersize_by_index(i);
	  if (strlen(pt->name) > 0 &&
	      pt->width <= caps->max_paper_width &&
	      pt->height <= caps->max_paper_height &&
	      (pt->width >= caps->min_paper_width || pt->width == 0) &&
	      (pt->height >= caps->min_paper_height || pt->height == 0))
	    {
/* stpi_erprintf("olympus: pagesize %s, %s\n", pt->name, pt->text); */
	      stp_string_list_add_string(description->bounds.str,
					pt->name, pt->text);
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
      const olympus_res_t *res;
      description->bounds.str = stp_string_list_create();

      res =  *(caps->res); /* get resolution specific parameters of printer */
      while (res->xdpi)
        {
/* stpi_erprintf("olympus: resolution %s, %s\n", res->name, res->text); */
          stp_string_list_add_string(description->bounds.str,
	  	res->name, _(res->text)); 
	  res++;
	}
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
  const olympus_cap_t *caps = olympus_get_model_capabilities(
  						stpi_get_model_id(v));
  
  stpi_default_media_size(v, &width, &height);
  *left = caps->border_left;
  *top = caps->border_top;
  *right = width - caps->border_right;
  *bottom = height - caps->border_bottom;
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
  if (resolution) {
    sscanf(resolution, "%dx%d", x, y);
  }
  return;
}

/*
 * olympus_print()
 */
static int
olympus_do_print(stp_vars_t v, stp_image_t *image)
{
  int i, j;
  int y, min_y, max_y, max_progress;		/* Looping vars */
  int min_x, max_x;
  int out_channels;
  unsigned short *final_out = NULL;
  unsigned char  *char_out;
  unsigned short *real_out;
  unsigned short *err_out;
  int status = 1;
  int ink_channels = 1;

  int r_errdiv, r_errmod;
  int r_errval  = 0;
  int r_errlast = -1;
  int r_errline = 0;
  int c_errdiv, c_errmod;
  int c_errval  = 0;
  int c_errlast = -1;
  int c_errcol = 0;

  const int model           = stpi_get_model_id(v); 
  const char *ink_type      = stp_get_string_parameter(v, "InkType");
  const olympus_cap_t *caps = olympus_get_model_capabilities(model);
  const char *res_param     = stp_get_string_parameter(v, "Resolution");
  const olympus_res_t *res  = olympus_get_res_params(model, res_param);

  int xdpi, ydpi;	/* Resolution */

  /* image in pixels */
  unsigned short image_px_width  = stpi_image_width(image);
  unsigned short image_px_height = stpi_image_height(image);

  /* output in 1/72" */
  int out_pt_width  = stp_get_width(v);
  int out_pt_height = stp_get_height(v);
  int out_pt_left   = stp_get_left(v);
  int out_pt_top    = stp_get_top(v);

  /* output in pixels */
  int out_px_width, out_px_height;
  int out_px_left, out_px_right, out_px_top, out_px_bottom;

  /* page in 1/72" */
  int page_pt_width  = stp_get_page_width(v);
  int page_pt_height = stp_get_page_height(v);
  int page_pt_left, page_pt_right, page_pt_top, page_pt_bottom;

  /* page w/out borders in pixels (according to selected dpi) */
  int print_px_width  = res->x_max_res;
  int print_px_height = res->y_max_res;
  
  unsigned char  copies = 1;
  unsigned short hi_speed = 1;
  char *l, layers[] = "YMC";
  unsigned char *zeros;

  if (!stp_verify(v))
    {
      stpi_eprintf(v, _("Print options not verified; cannot print.\n"));
      return 0;
    }

  stp_describe_resolution(v, &xdpi, &ydpi);
  olympus_imageable_area(v, &page_pt_left, &page_pt_right,
	&page_pt_bottom, &page_pt_top);

  out_px_width  = out_pt_width  * xdpi / 72;
  out_px_height = out_pt_height * ydpi / 72;

#if 0
  /* skip the job if image is not approximately the same size as output */
  if (out_px_width - image_px_width < -2
      || out_px_width - image_px_width > 2
      || out_px_height - image_px_height < -2
      || out_px_height - image_px_height > 2)
    {
      stpi_eprintf(v, _("This driver is under development.\n\n"
        "It can't rescale your image at present. \n"
	"Your print job is canceled now(!).\n"
	"Please scale your image suitably and try it again.\n"
	"Thank you."));
      return 0;
    }
#endif

  /* if image size is close enough to output size send out original size */
  if (out_px_width - image_px_width > -5
      && out_px_width - image_px_width < 5
      && out_px_height - image_px_height > -5
      && out_px_height - image_px_height < 5)
    {
    out_px_width  = image_px_width;
    out_px_height = image_px_height;
    }

  out_px_width  = MIN(out_px_width, print_px_width);
  out_px_height = MIN(out_px_height, print_px_height);
  out_px_left   = MIN(((out_pt_left - page_pt_left) * xdpi / 72),
		  	print_px_width - out_px_width);
  out_px_top    = MIN(((out_pt_top  - page_pt_top)  * ydpi / 72),
		  	print_px_height - out_px_height);
  out_px_right  = out_px_left + out_px_width;
  out_px_bottom = out_px_top  + out_px_height;
  
#if 0
  stpi_eprintf(v, "paper (pt)   %d x %d\n"
		"image (px)   %d x %d\n"
		"image (pt)   %d x %d\n"
		"* out (pt)   %d x %d\n"
		"* out (px)   %d x %d\n"
		"* left x top (pt) %d x %d\n"
		"* left x top (px) %d x %d\n"
		"border (pt) (%d - %d) = %d x (%d - %d) = %d\n"
		"printable pixels (px)   %d x %d\n"
		"res (dpi)               %d x %d\n",
		page_pt_width, page_pt_height,
		image_px_width, image_px_height,
		image_px_width * 72 / xdpi, image_px_height * 72 / ydpi,
		out_pt_width, out_pt_height,
		out_px_width, out_px_height,
		out_pt_left, out_pt_top,
		out_px_left, out_px_top,
		page_pt_right, page_pt_left, page_pt_right - page_pt_left,
		  page_pt_bottom, page_pt_top, page_pt_bottom - page_pt_top,
		print_px_width, print_px_height,
		xdpi, ydpi
		);	
#endif

  
  stpi_set_output_color_model(v, COLOR_MODEL_CMY);
  
  if (ink_type)
    {
      for (i = 0; i < ink_count; i++)
	if (strcmp(ink_type, inks[i].name) == 0)
	  {
	    stpi_set_output_color_model(v, inks[i].color_model);
	    ink_channels = inks[i].output_channels;
	    break;
	  }
    }

  stpi_channel_reset(v);
  for (i = 0; i < ink_channels; i++)
    stpi_channel_add(v, i, 0, 1.0);

  out_channels = stpi_color_init(v, image, 65536);

#if 0
  if (out_channels != ink_channels && out_channels != 1 && ink_channels != 1)
    {
      stpi_eprintf(v, _("Internal error!  Output channels or input channels must be 1\n"));
      return 0;
    }
#endif

  err_out = stpi_malloc(print_px_width * ink_channels * 2);
  if (out_channels != ink_channels)
    final_out = stpi_malloc(print_px_width * ink_channels * 2);

  stp_set_float_parameter(v, "Density", 1.0);

  stpi_image_progress_init(image);

  zeros = stpi_zalloc(print_px_width+1);
  
  /* printer init */
  stpi_zfwrite("\033\033\033C\033N", 1, 6, v);
  stpi_putc(copies, v);
  stpi_zfwrite("\033F", 1, 2, v);
  stpi_put16_be(hi_speed, v);
  stpi_zfwrite("\033MS\xff\xff\xff\033Z", 1, 8, v);
  stpi_put16_be(xdpi, v);
  stpi_put16_be(ydpi, v);
  
  
  min_y = (caps->need_empty_rows ? 0 : out_px_top 
       - (out_px_top % caps->block_size)); /* floor to multiple of block_size */
  max_y = (caps->need_empty_rows ? print_px_height - 1 : (out_px_bottom - 1)
       + (caps->block_size - 1) - ((out_px_bottom - 1) % caps->block_size));
                                           /* ceil to multiple of block_size */
  min_x = (caps->need_empty_cols ? 0 : out_px_left);
  max_x = (caps->need_empty_cols ? print_px_width - 1 : out_px_right);

  max_progress = max_y - min_y;

  r_errdiv  = image_px_height / out_px_height;
  r_errmod  = image_px_height % out_px_height; 
  c_errdiv = image_px_width / out_px_width;
  c_errmod = image_px_width % out_px_width;

  l = layers;
  while (*l)
    {
    r_errval  = 0;
    r_errlast = -1;
    r_errline = 0;

    for (y = min_y; y <= max_y; y++)
      {
      unsigned short *out;
      int duplicate_line = 1;
      unsigned zero_mask;

      if (((y - min_y) % caps->block_size) == 0)
        {
        /* block init */
	stpi_zfwrite("\033\033\033W", 1, 4, v);
	stpi_putc(*l, v);
	stpi_put16_be(y, v);
	stpi_put16_be(min_x, v);
	stpi_put16_be(MIN(y + caps->block_size - 1, print_px_height), v);
	stpi_put16_be(max_x, v);
	}
      

      if ((y & 63) == 0)
        {
        stpi_image_note_progress(image, max_progress/3 * (l - layers) + y / 3,
            max_progress);
        }
      if (y < out_px_top || y >= out_px_bottom)
        {
	stpi_zfwrite((char *) zeros, 1, print_px_width, v);
	}
      else
        {
        if (caps->need_empty_cols && out_px_left > 0)
	  {
          stpi_zfwrite((char *) zeros, 1, out_px_left, v);
/* stpi_erprintf("left %d ", out_px_left); */
	  }

	if (r_errline != r_errlast)
	  {
	  r_errlast = r_errline;
	  duplicate_line = 0;

/* stpi_erprintf("r_errline %d, ", r_errline); */
          if (stpi_color_get_row(v, image, r_errline, &zero_mask))
            {
  	    status = 2;
  	    break;
            }
  	  }
	
	out = stpi_channel_get_output(v);

	c_errval  = 0;
	c_errlast = -1;
	c_errcol  = 0;
	for (i = 0; i < out_px_width; i++) {
	  if (c_errcol != c_errlast) {
	    c_errlast = c_errcol;
	  }
	  for (j = 0; j < ink_channels; j++) {
  	    err_out[i * ink_channels + j] = out[c_errcol * ink_channels + j];
	  }

	  c_errval += c_errmod;
	  c_errcol += c_errdiv;
	  if (c_errval >= out_px_width) {
	    c_errval -= out_px_width;
	    c_errcol ++;
	  }
	}

	real_out = err_out;
        if (out_channels != ink_channels)
  	{
  	  real_out = final_out;
  	  if (out_channels < ink_channels)
  	    {
  	      for (i = 0; i < out_px_width; i++)
  		{
  		  for (j = 0; j < ink_channels; j++)
  		    final_out[i * ink_channels + j] = err_out[i];
  		}
  	    }
  	  else
  	    {
  	      for (i = 0; i < out_px_width; i++)
  		{
  		  int avg = 0;
  		  for (j = 0; j < out_channels; j++)
  		    avg += err_out[i * out_channels + j];
  		  final_out[i] = avg / out_channels;
  		}
  	    }
  	}
  	char_out = (unsigned char *) real_out;
  	for (i = 0; i < out_px_width; i++)
  	  char_out[i] = real_out[i * ink_channels + (layers - l + 2)] / 257;
  
	stpi_zfwrite((char *) real_out, 1, out_px_width, v);
/* stpi_erprintf("data %d ", out_px_width); */
        if (caps->need_empty_cols && out_px_right < print_px_width)
	  {
          stpi_zfwrite((char *) zeros, 1, print_px_width - out_px_right, v);
/* stpi_erprintf("right %d ", print_px_width - out_px_right); */
	  }
/* stpi_erprintf("\n"); */

	r_errval += r_errmod;
	r_errline += r_errdiv;
	if (r_errval >= out_px_height)
	  {
	    r_errval -= out_px_height;
	    r_errline ++;
	  }
	}
      }
      /* layer end */
      stpi_zfwrite("\033\033\033P", 1, 4, v);
      stpi_putc(*l, v);
      stpi_zfwrite("S", 1, 1, v);

      l++;
    }
  stpi_image_progress_conclude(image);
  if (final_out)
    stpi_free(final_out);
  return status;
}

static int
olympus_print(stp_const_vars_t v, stp_image_t *image)
{
  int status;
  stp_vars_t nv = stp_vars_create_copy(v);
  stpi_prune_inactive_options(nv);
  status = olympus_do_print(nv, image);
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
