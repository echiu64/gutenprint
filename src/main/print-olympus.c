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

#define OLYMPUS_INTERLACE_NONE	0
#define OLYMPUS_INTERLACE_LINE	1
#define OLYMPUS_INTERLACE_PLANE	2

#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))

static const char *zero = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

typedef struct
{
  int color_model;
  int output_channels;
  const char *name;
} ink_t;

typedef struct
{
	int xdpi, ydpi;
	int xsize, ysize;
	char plane;
	int block_min_x, block_min_y;
	int block_max_x, block_max_y;
} olympus_privdata_t;

static olympus_privdata_t privdata;

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
  int interlacing;	/* color interlacing scheme */
  const char *planes;	/* name and order of ribbons */
  int block_size;
  int need_empty_cols;	/* must we print empty columns? */
  int need_empty_rows;	/* must we print empty rows? */
  void (*printer_init_func)(stp_vars_t);
  void (*printer_end_func)(stp_vars_t);
  void (*plane_init_func)(stp_vars_t);
  void (*plane_end_func)(stp_vars_t);
  void (*block_init_func)(stp_vars_t);
  void (*block_end_func)(stp_vars_t);
  const char *adj_cyan;		/* default color adjustment */
  const char *adj_magenta;
  const char *adj_yellow;
} olympus_cap_t;


static const olympus_res_t_array p300_resolution = 
{
	{ "306x306", N_ ("306x306 DPI"), 306, 306, 1024, 1376 },
	{ "153x153", N_ ("153x153 DPI"), 153, 153, 512, 688 },
	{ "", "", 0, 0, 0, 0 }
}; 

static void p300_printer_init_func(stp_vars_t v)
{
	stpi_zfwrite("\033\033\033C\033N\1\033F\0\1\033MS\xff\xff\xff"
			"\033Z", 1, 19, v);
	stpi_put16_be(privdata.xdpi, v);
	stpi_put16_be(privdata.ydpi, v);
}

static void p300_plane_end_func(stp_vars_t v)
{
	stpi_zprintf(v, "\033\033\033P%cS", privdata.plane);
}

static void p300_block_init_func(stp_vars_t v)
{
	stpi_zprintf(v, "\033\033\033W%c", privdata.plane);
	stpi_put16_be(privdata.block_min_y, v);
	stpi_put16_be(privdata.block_min_x, v);
	stpi_put16_be(privdata.block_max_y, v);
	stpi_put16_be(privdata.block_max_x, v);
}

static const char p300_adj_cyan[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0.078431 0.211765 0.250980 0.282353 0.309804 0.333333 0.352941 0.368627\n"
"0.388235 0.403922 0.427451 0.443137 0.458824 0.478431 0.498039 0.513725\n"
"0.529412 0.545098 0.556863 0.576471 0.592157 0.611765 0.627451 0.647059\n"
"0.666667 0.682353 0.701961 0.713725 0.725490 0.729412 0.733333 0.737255\n"
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char p300_adj_magenta[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0.047059 0.211765 0.250980 0.278431 0.305882 0.333333 0.349020 0.364706\n"
"0.380392 0.396078 0.415686 0.435294 0.450980 0.466667 0.482353 0.498039\n"
"0.513725 0.525490 0.541176 0.556863 0.572549 0.592157 0.611765 0.631373\n"
"0.650980 0.670588 0.694118 0.705882 0.721569 0.741176 0.745098 0.756863\n"
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char p300_adj_yellow[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0.047059 0.117647 0.203922 0.250980 0.274510 0.301961 0.321569 0.337255\n"
"0.352941 0.364706 0.380392 0.396078 0.407843 0.423529 0.439216 0.450980\n"
"0.466667 0.482353 0.498039 0.513725 0.533333 0.552941 0.572549 0.596078\n"
"0.615686 0.635294 0.650980 0.666667 0.682353 0.690196 0.701961 0.713725\n"
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";


static const olympus_res_t_array p400_resolution =
{
	{"314x314", N_ ("314x314 DPI"), 314, 314, 2400, 3200},
	{"", "", 0, 0, 0, 0}
};

static void p400_printer_init_func(stp_vars_t v)
{
	stpi_zprintf(v, "\033ZQ"); stpi_zfwrite(zero, 1, 61, v);
	stpi_zprintf(v, "\033FP"); stpi_zfwrite(zero, 1, 61, v);
	stpi_zprintf(v, "\033ZF"); stpi_zfwrite(zero, 1, 61, v);
	stpi_zprintf(v, "\033ZS");
	stpi_put16_be(privdata.xsize, v);
	stpi_put16_be(privdata.ysize, v);
	stpi_zfwrite(zero, 1, 57, v);
	stpi_zprintf(v, "\033ZP"); stpi_zfwrite(zero, 1, 61, v);
}

static void p400_plane_init_func(stp_vars_t v)
{
	stpi_zprintf(v, "\033ZC"); stpi_zfwrite(zero, 1, 61, v);
}

static void p400_plane_end_func(stp_vars_t v)
{
	stpi_zprintf(v, "\033P"); stpi_zfwrite(zero, 1, 62, v);
}

static void p400_block_init_func(stp_vars_t v)
{
	stpi_zprintf(v, "\033Z%c", privdata.plane);
	stpi_put16_be(privdata.block_min_x, v);
	stpi_put16_be(privdata.block_min_y, v);
	stpi_put16_be(privdata.block_max_x - privdata.block_min_x + 1, v);
	stpi_put16_be(privdata.block_max_y - privdata.block_min_y + 1, v);
	stpi_zfwrite(zero, 1, 53, v);
}

static const char p400_adj_cyan[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0.003922 0.031373 0.058824 0.090196 0.125490 0.156863 0.184314 0.219608\n"
"0.250980 0.278431 0.309804 0.341176 0.376471 0.403922 0.439216 0.470588\n"
"0.498039 0.517647 0.533333 0.545098 0.564706 0.576471 0.596078 0.615686\n"
"0.627451 0.647059 0.658824 0.678431 0.690196 0.705882 0.721569 0.737255\n"
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char p400_adj_magenta[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0.003922 0.031373 0.062745 0.098039 0.125490 0.156863 0.188235 0.215686\n"
"0.250980 0.282353 0.309804 0.345098 0.376471 0.407843 0.439216 0.470588\n"
"0.501961 0.521569 0.549020 0.572549 0.592157 0.619608 0.643137 0.662745\n"
"0.682353 0.713725 0.737255 0.756863 0.784314 0.807843 0.827451 0.850980\n"
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char p400_adj_yellow[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0.003922 0.027451 0.054902 0.090196 0.121569 0.156863 0.184314 0.215686\n"
"0.250980 0.282353 0.309804 0.345098 0.372549 0.400000 0.435294 0.466667\n"
"0.498039 0.525490 0.552941 0.580392 0.607843 0.631373 0.658824 0.678431\n"
"0.698039 0.725490 0.760784 0.784314 0.811765 0.839216 0.866667 0.890196\n"
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";


static const olympus_res_t_array cpx00_resolution =
{
	{"314x314", N_ ("300x300 DPI"), 314, 314, 1232, 1808},
	{"", "", 0, 0, 0, 0}
};

static void cpx00_printer_init_func(stp_vars_t v)
{
	stpi_put16_be(0x4000, v);
	stpi_put16_be(0x0001, v);
	stpi_zfwrite(zero, 1, 8, v);
}

static void cpx00_plane_init_func(stp_vars_t v)
{
	stpi_put16_be(0x4001, v);
	stpi_put16_le(privdata.plane - '1', v);
	stpi_put32_le(privdata.xsize * privdata.ysize, v);
	stpi_zfwrite(zero, 1, 4, v);
}

static const char cpx00_adj_cyan[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0.000000 0.035294 0.070588 0.101961 0.117647 0.168627 0.180392 0.227451\n"
"0.258824 0.286275 0.317647 0.341176 0.376471 0.411765 0.427451 0.478431\n"
"0.505882 0.541176 0.576471 0.611765 0.654902 0.678431 0.705882 0.737255\n"
"0.764706 0.792157 0.811765 0.839216 0.862745 0.894118 0.909804 0.925490\n"
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char cpx00_adj_magenta[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0.011765 0.019608 0.035294 0.047059 0.054902 0.101961 0.133333 0.156863\n"
"0.192157 0.235294 0.274510 0.321569 0.360784 0.403922 0.443137 0.482353\n"
"0.521569 0.549020 0.584314 0.619608 0.658824 0.705882 0.749020 0.792157\n"
"0.831373 0.890196 0.933333 0.964706 0.988235 0.992157 0.992157 0.996078\n"
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";

static const char cpx00_adj_yellow[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gimp-print>\n"
"<curve wrap=\"nowrap\" type=\"spline\" gamma=\"0\">\n"
"<sequence count=\"32\" lower-bound=\"0\" upper-bound=\"1\">\n"
"0.003922 0.015686 0.015686 0.023529 0.027451 0.054902 0.094118 0.129412\n"
"0.180392 0.219608 0.250980 0.286275 0.317647 0.341176 0.388235 0.427451\n"
"0.470588 0.509804 0.552941 0.596078 0.627451 0.682353 0.768627 0.796078\n"
"0.890196 0.921569 0.949020 0.968627 0.984314 0.992157 0.992157 1.000000\n"
"</sequence>\n"
"</curve>\n"
"</gimp-print>\n";


static const olympus_res_t_array updp10_resolution =
{
	{"306x312", N_ ("300x300 DPI"), 306, 312, 1200, 1800},
	{"", "", 0, 0, 0, 0}
};

static void updp10_printer_init_func(stp_vars_t v)
{
	stpi_zfwrite("\x98\xff\xff\xff\xff\xff\xff\xff"
			"\x14\x00\x00\x00\x1b\x15\x00\x00"
			"\x00\x0d\x00\x00\x00\x00\x00\xc7"
			"\x00\x00\x00\x00", 1, 28, v);
	stpi_put16_be(privdata.xsize, v);
	stpi_put16_be(privdata.ysize, v);
	stpi_zfwrite("\x8b\xe0\x62\x00\x1b\xea\x00\x00"
			"\x00\x00\x00\x62\xe0\x80\x00", 1, 15, v);
}

static void updp10_printer_end_func(stp_vars_t v)
{
	stpi_zfwrite("\x12\x00\x00\x00\x1b\xe1\x00\x00"
			"\x00\xb0\x00\x00\04", 1, 13, v);
	stpi_putc('\x00', v); /* <- glossy lamination */
	stpi_zfwrite("\x00\x00\x00\x00\x07\x08\x04\xb0"
			"\xff\xff\xff\xff\x09\x00\x00\x00"
			"\x1b\xee\x00\x00\x00\x02\x00\x00"
			"\x01\x07\x00\x00\x00\x1b\x0a\x00"
			"\x00\x00\x00\x00\xfd\xff\xff\xff"
			"\xff\xff\xff\xff\xf8\xff\xff\xff"
			, 1, 48, v);
}


static const olympus_cap_t olympus_model_capabilities[] =
{
	{ 0, 		/* model P300 */
		297, 420,	/* A6 */
		283, 416,	/* Postcard */
		28, 28, 48, 48,
		&p300_resolution,
		OLYMPUS_INTERLACE_PLANE, "YMC",
		16,
		1, 0,
		&p300_printer_init_func, NULL,
		NULL, &p300_plane_end_func,
		&p300_block_init_func, NULL,
		p300_adj_cyan, p300_adj_magenta, p300_adj_yellow,
	},
	{ 1, 		/* model P400 */
		595, 842,	/* A4 */
		283, 416,	/* Postcard */
		22, 22, 54, 54,
		&p400_resolution,
		OLYMPUS_INTERLACE_PLANE, "123",
		180,
		1, 1,
		&p400_printer_init_func, NULL,
		&p400_plane_init_func, &p400_plane_end_func,
		&p400_block_init_func, NULL,
		p400_adj_cyan, p400_adj_magenta, p400_adj_yellow,
	},
	{ 1000, 	/* canon CP100 */
		283, 416, 	/* Postcard */
		283, 416,	/* Postcard */
		0, 0, 0, 0,
		&cpx00_resolution,
		OLYMPUS_INTERLACE_PLANE, "123",
		1808,
		1, 1,
		&cpx00_printer_init_func, NULL,
		&cpx00_plane_init_func, NULL,
		NULL, NULL,
		cpx00_adj_cyan, cpx00_adj_magenta, cpx00_adj_yellow,
	},
	{ 2000, 	/* sony UP-DP10  */
		283, 416, 	/* Postcard */
		283, 416,	/* Postcard */
		0, 0, 0, 0,
		&updp10_resolution,
		OLYMPUS_INTERLACE_NONE, "123",
		1800,
		1, 1,
		&updp10_printer_init_func, &updp10_printer_end_func,
		NULL, NULL,
		NULL, NULL,
		NULL, NULL, NULL,
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
  int y, min_y, max_y;			/* Looping vars */
  int max_progress, curr_progress = 0;	/* Progress info */
  int min_x, max_x;
  int out_channels, out_bytes;
  unsigned short *final_out = NULL;
  unsigned char  *char_out;
  unsigned short *real_out;
  unsigned short *err_out;
  int char_out_width;
  int status = 1;
  int ink_channels = 1;
  stp_curve_t   adjustment = NULL;

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
  int print_px_width;
  int print_px_height;
  
  const char *l;
  unsigned char *zeros;

  if (!stp_verify(v))
    {
      stpi_eprintf(v, _("Print options not verified; cannot print.\n"));
      return 0;
    }

  stp_describe_resolution(v, &xdpi, &ydpi);
  olympus_imageable_area(v, &page_pt_left, &page_pt_right,
	&page_pt_bottom, &page_pt_top);

  print_px_width  = MIN(res->x_max_res,
		  (page_pt_right - page_pt_left) * xdpi / 72);
  print_px_height = MIN(res->y_max_res,
		  (page_pt_bottom - page_pt_top) * ydpi / 72);
  out_px_width  = out_pt_width  * xdpi / 72;
  out_px_height = out_pt_height * ydpi / 72;


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
  privdata.xdpi = xdpi;
  privdata.ydpi = ydpi;
  privdata.xsize = print_px_width;
  privdata.ysize = print_px_height;

  stpi_set_output_color_model(v, COLOR_MODEL_CMY);
  
  if (caps->adj_cyan &&
        !stp_check_curve_parameter(v, "CyanCurve", STP_PARAMETER_ACTIVE)) {
    adjustment = stp_curve_create_from_string(caps->adj_cyan);
    stp_set_curve_parameter(v, "CyanCurve", adjustment);
    stp_set_curve_parameter_active(v, "CyanCurve", STP_PARAMETER_ACTIVE);
    stp_curve_free(adjustment);
  }
  if (caps->adj_magenta &&
        !stp_check_curve_parameter(v, "MagentaCurve", STP_PARAMETER_ACTIVE)) {
    adjustment = stp_curve_create_from_string(caps->adj_magenta);
    stp_set_curve_parameter(v, "MagentaCurve", adjustment);
    stp_set_curve_parameter_active(v, "MagentaCurve", STP_PARAMETER_ACTIVE);
    stp_curve_free(adjustment);
  }
  if (caps->adj_yellow &&
        !stp_check_curve_parameter(v, "YellowCurve", STP_PARAMETER_ACTIVE)) {
    adjustment = stp_curve_create_from_string(caps->adj_yellow);
    stp_set_curve_parameter(v, "YellowCurve", adjustment);
    stp_set_curve_parameter_active(v, "YellowCurve", STP_PARAMETER_ACTIVE);
    stp_curve_free(adjustment);
  }

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

  zeros = stpi_zalloc(ink_channels * print_px_width + 1);
  
  out_bytes = (caps->interlacing == OLYMPUS_INTERLACE_PLANE ? 1 : ink_channels);

  /* printer init */
  if (caps->printer_init_func) {
    stpi_deprintf(STPI_DBG_OLYMPUS, "olympus: caps->printer_init\n");
    (*(caps->printer_init_func))(v);
  }

  min_y = (caps->need_empty_rows ? 0 : out_px_top 
       - (out_px_top % caps->block_size)); /* floor to multiple of block_size */
  max_y = (caps->need_empty_rows ? print_px_height - 1 : (out_px_bottom - 1)
       + (caps->block_size - 1) - ((out_px_bottom - 1) % caps->block_size));
                                           /* ceil to multiple of block_size */
  min_x = (caps->need_empty_cols ? 0 : out_px_left);
  max_x = (caps->need_empty_cols ? print_px_width - 1 : out_px_right);

  max_progress = (caps->interlacing == OLYMPUS_INTERLACE_PLANE ?
		  	(max_y - min_y) * ink_channels : max_y - min_y) / 63;

  r_errdiv  = image_px_height / out_px_height;
  r_errmod  = image_px_height % out_px_height; 
  c_errdiv = image_px_width / out_px_width;
  c_errmod = image_px_width % out_px_width;

  l = caps->planes;
  while (*l)
    {
    r_errval  = 0;
    r_errlast = -1;
    r_errline = 0;

    privdata.plane = *l;

    /* plane init */
    if (caps->plane_init_func) {
      stpi_deprintf(STPI_DBG_OLYMPUS, "olympus: caps->plane_init\n");
      (*(caps->plane_init_func))(v);
    }

    for (y = min_y; y <= max_y; y++)
      {
      unsigned short *out;
      int duplicate_line = 1;
      unsigned zero_mask;

      if (((y - min_y) % caps->block_size) == 0) {
        /* block init */
        privdata.block_min_y = y;
        privdata.block_min_x = min_x;
        privdata.block_max_y = MIN(y + caps->block_size, print_px_height) - 1;
        privdata.block_max_x = max_x;

	if (caps->block_init_func) {
          stpi_deprintf(STPI_DBG_OLYMPUS, "olympus: caps->block_init\n");
          (*(caps->block_init_func))(v);
	}
      }
      

      if ((y & 63) == 0)
        {
        stpi_image_note_progress(image, curr_progress++, max_progress);
        }

      if (y < out_px_top || y >= out_px_bottom)
        {
	stpi_zfwrite((char *) zeros, out_bytes, print_px_width, v);
	}
      else
        {
        if (caps->need_empty_cols && out_px_left > 0)
	  {
          stpi_zfwrite((char *) zeros, out_bytes, out_px_left, v);
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
	char_out_width = (caps->interlacing == OLYMPUS_INTERLACE_PLANE ?
				out_px_width : out_px_width * out_channels);
  	for (i = 0; i < char_out_width; i++) {
          if (caps->interlacing == OLYMPUS_INTERLACE_PLANE) {
	    j = i * ink_channels + (caps->planes - l + 2);
	  } else if (caps->interlacing == OLYMPUS_INTERLACE_LINE) {
	    j = (i % out_px_width) + (i / out_px_width);
	  } else { /* OLYMPUS_INTERLACE_NONE */
	    j = i;
	  }
  	  
	  char_out[i] = real_out[j] / 257;
        }
	stpi_zfwrite((char *) real_out, 1, char_out_width, v);
/* stpi_erprintf("data %d ", out_px_width); */
        if (caps->need_empty_cols && out_px_right < print_px_width)
	  {
          stpi_zfwrite((char *) zeros, out_bytes, print_px_width - out_px_right, v);
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
      
      if (y == privdata.block_max_y) {
        /* block end */
        if (caps->block_end_func) {
          stpi_deprintf(STPI_DBG_OLYMPUS, "olympus: caps->block_end\n");
          (*(caps->block_end_func))(v);
	}
      }
      
      }
      /* plane end */
      if (caps->plane_end_func) {
        stpi_deprintf(STPI_DBG_OLYMPUS, "olympus: caps->plane_end\n");
        (*(caps->plane_end_func))(v);
      }

      if (caps->interlacing != OLYMPUS_INTERLACE_PLANE) {
	break;
      }
      l++;
    }
  /* printer end */
  if (caps->printer_end_func) {
    stpi_deprintf(STPI_DBG_OLYMPUS, "olympus: caps->printer_end\n");
    (*(caps->printer_end_func))(v);
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

