/*
 *
 *   Print plug-in Datamax-O'Neil DPL driver for Gutenprint.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com),
 *	Robert Krawitz (rlk@alum.mit.edu) and
 *      Dave Hill (dave@minnie.demon.co.uk)
 *
 *   Copyright 2016 Steve Letter (sletter1@yahoo.com)
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * This file must include only standard C header files.  The core code must
 * compile on generic platforms that don't support glib, gimp, gtk, etc.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <gutenprint/gutenprint.h>
#include <gutenprint/gutenprint-intl-internal.h>
#include "gutenprint-internal.h"
#include "dither-impl.h"
#include <stdio.h>
#include <string.h>

#define DEBUG
#define DPL_DEBUG_DISABLE_BLANKLINE_REMOVAL

/*
 * Local functions...
 */
static void dpl_pcx (stp_vars_t *, unsigned char *, int, int);
static int dpl_get_multiplier (const stp_vars_t * v);

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif /* !MAX */

typedef struct
{
  int height;
  int orientation;
  int label_separator;
  unsigned int h_offset;
  unsigned int v_offset;
  int darkness;
  int speed;
  int present;
} dpl_privdata_t;

/*
 * Generic define for a name/value set
 */

typedef struct
{
  const char *dpl_name;
  const char *dpl_text;
  int dpl_code;
  int p0;
  int p1;
} dpl_t;


#define DPL_RES_150_150		1
#define DPL_RES_203_203		2
#define DPL_RES_300_300		4
#define DPL_RES_400_400		8
#define DPL_RES_600_600		16

static const dpl_t dpl_resolutions[] = {
  {"600dpi", N_("600x600 DPI"), DPL_RES_600_600, 600, 600},
  {"400dpi", N_("400x400 DPI"), DPL_RES_400_400, 400, 400},
  {"300dpi", N_("300x300 DPI"), DPL_RES_300_300, 300, 300},
  {"203dpi", N_("203x203 DPI"), DPL_RES_203_203, 203, 203},
  {"150dpi", N_("150x150 DPI"), DPL_RES_150_150, 150, 150},
};

#define NUM_RESOLUTIONS		(sizeof(dpl_resolutions) / sizeof (dpl_t))

static const dpl_t dpl_speeds[] = {
  {"A", N_("1.0 IPS"), 'A'},
  {"B", N_("1.5 IPS"), 'B'},
  {"C", N_("2.0 IPS"), 'C'},
  {"D", N_("2.5 IPS"), 'D'},
  {"E", N_("3.0 IPS"), 'E'},
  {"F", N_("3.5 IPS"), 'F'},
  {"G", N_("4.0 IPS"), 'G'},
  {"H", N_("4.5 IPS"), 'H'},
  {"I", N_("5.0 IPS"), 'I'},
  {"J", N_("5.5 IPS"), 'J'},
  {"K", N_("6.0 IPS"), 'K'},
  {"L", N_("6.5 IPS"), 'L'},
  {"M", N_("7.0 IPS"), 'M'},
  {"N", N_("7.5 IPS"), 'N'},
  {"O", N_("8.0 IPS"), 'O'},
  {"P", N_("8.5 IPS"), 'P'},
  {"Q", N_("9.0 IPS"), 'Q'},
  {"R", N_("9.5 IPS"), 'R'},
  {"S", N_("10.0 IPS"), 'S'},
  {"T", N_("10.5 IPS"), 'T'},
  {"U", N_("11.0 IPS"), 'U'},
  {"V", N_("11.5 IPS"), 'V'},
  {"W", N_("12.0 IPS"), 'W'},
};

#define NUM_SPEEDS		(sizeof(dpl_speeds) / sizeof (dpl_t))

/*
 * Printer capability data
 */

typedef struct
{
  int model;
  int custom_max_width;
  int custom_max_height;
  int custom_min_width;
  int custom_min_height;
  int resolutions;
  int max_resolution;
  int resolution_adjust;
  char max_speed;
  char min_speed;
  char default_speed;
} dpl_cap_t;

static const dpl_cap_t dpl_model_capabilities[] = {
/* Datamax-O'Neil Thermal DPL printers */
  {10017,			/* I Class Mark II 203 DPI */
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   DPL_RES_203_203,
   DPL_RES_203_203,
   DPL_RES_203_203,
   'W',
   'C',
   'O',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10018,			/* I Class Mark II 300 DPI */
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   DPL_RES_150_150 | DPL_RES_300_300,	/* Resolutions */
   DPL_RES_300_300,
   DPL_RES_203_203,
   'S',
   'C',
   'O',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10020,			/* I Class Mark II 600 DPI */
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   /* for future use
      DPL_RES_150_150 | DPL_RES_203_203 | DPL_RES_300_300 | DPL_RES_600_600, */
   DPL_RES_300_300 | DPL_RES_600_600,	/* Resolutions */
   DPL_RES_600_600,
   DPL_RES_300_300,
   'K',
   'C',
   'G',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10021,			/* E Class Mark III Basic 203 DPI*/
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   DPL_RES_203_203,	/* Resolutions */
   DPL_RES_203_203,
   DPL_RES_203_203,
   'G',
   'C',
   'E',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10022,			/* E Class Mark III Basic 300 DPI*/
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   DPL_RES_300_300,	/* Resolutions */
   DPL_RES_300_300,
   DPL_RES_300_300,
   'G',
   'C',
   'E',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10023,			/* E Class Mark III Advanced 203 DPI*/
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   DPL_RES_203_203,	/* Resolutions */
   DPL_RES_203_203,
   DPL_RES_203_203,
   'I',
   'C',
   'E',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10024,			/* E Class Mark III Advanced 300 DPI*/
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   DPL_RES_300_300,	/* Resolutions */
   DPL_RES_300_300,
   DPL_RES_300_300,
   'I',
   'C',
   'G',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10025,			/* E Class Mark III Pro 203 DPI*/
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   DPL_RES_203_203,	/* Resolutions */
   DPL_RES_203_203,
   DPL_RES_203_203,
   'K',
   'C',
   'G',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10026,			/* E Class Mark III Pro 300 DPI*/
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   DPL_RES_300_300,	/* Resolutions */
   DPL_RES_300_300,
   DPL_RES_300_300,
   'I',
   'C',
   'G',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10027,			/* E Class Mark III ProPlus 203 DPI*/
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   DPL_RES_203_203,	/* Resolutions */
   DPL_RES_203_203,
   DPL_RES_203_203,
   'K',
   'C',
   'G',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10028,			/* E Class Mark III ProPlus 300 DPI*/
   4 * 72, 99 * 72,		/* Max paper size */
   1, 1,			/* Min paper size */
   DPL_RES_300_300,	/* Resolutions */
   DPL_RES_300_300,
   DPL_RES_300_300,
   'I',
   'C',
   'G',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10029,                       /* RL3e */
   3 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_203_203,     /* Resolutions */
   DPL_RES_203_203,
   DPL_RES_203_203,
   'G',
   'A',
   'E',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10030,                       /* RL4e */
   4 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_203_203,     /* Resolutions */
   DPL_RES_203_203,
   DPL_RES_203_203,
   'G',
   'A',
   'E',
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10031,                       /* H4212 */
   4 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_203_203,             /* Resolutions */
   DPL_RES_203_203,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'W',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'O',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10032,                       /* H4212X */
   4 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_203_203,             /* Resolutions */
   DPL_RES_203_203,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'W',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'O',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10033,                       /* H4310 */
   4 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_150_150 | DPL_RES_300_300,   /* Resolutions */
   DPL_RES_300_300,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'S',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'O',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10034,                       /* H4310X */
   4 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_150_150 | DPL_RES_300_300,   /* Resolutions */
   DPL_RES_300_300,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'S',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'O',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10035,                       /* H4408 */
   4 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_400_400 | DPL_RES_203_203,   /* Resolutions */
   DPL_RES_203_203,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'O',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'G',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10036,                       /* H4606 */
   4 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_300_300 | DPL_RES_600_600,   /* Resolutions */
   DPL_RES_600_600,             /* Max Resolution */
   DPL_RES_300_300,             /* Resolution Adjust */
   'K',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'G',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10037,                       /* H4606X */
   4 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_300_300 | DPL_RES_600_600,   /* Resolutions */
   DPL_RES_600_600,             /* Max Resolution */
   DPL_RES_300_300,             /* Resolution Adjust */
   'K',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'G',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10038,                       /* H6210 */
   6 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_203_203,             /* Resolutions */
   DPL_RES_203_203,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'S',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'O',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10039,                       /* H6210X */
   6 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_203_203,             /* Resolutions */
   DPL_RES_203_203,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'S',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'O',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10040,                       /* H6212 */
   6 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_203_203,             /* Resolutions */
   DPL_RES_203_203,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'W',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'O',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10041,                       /* H6212X */
   6 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_203_203,             /* Resolutions */
   DPL_RES_203_203,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'W',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'O',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10042,                       /* H6308 */
   6 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_150_150 | DPL_RES_300_300,   /* Resolutions */
   DPL_RES_300_300,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'O',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'G',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10043,                       /* H6310X */
   6 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_150_150 | DPL_RES_300_300,   /* Resolutions */
   DPL_RES_300_300,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'S',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'O',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10044,                       /* H8308 */
   8.5 * 72, 99 * 72,           /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_150_150 | DPL_RES_300_300,   /* Resolutions */
   DPL_RES_300_300,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'O',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'G',                         /* Default IPS */
   },
/* Datamax-O'Neil Thermal DPL printers */
  {10045,                       /* H8308X */
   8.5 * 72, 99 * 72,           /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_150_150 | DPL_RES_300_300,   /* Resolutions */
   DPL_RES_300_300,             /* Max Resolution */
   DPL_RES_203_203,             /* Resolution Adjust */
   'O',                         /* Maximum IPS */
   'C',                         /* Minimum IPS */
   'G',                         /* Default IPS */
   },
/* Honeywell Thermal DPL printers */
  {10046,                       /* RP2 */
   2 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_203_203,     /* Resolutions */
   DPL_RES_203_203,
   DPL_RES_203_203,
   'I',
   'A',
   'E',
   },
/* Honeywell Thermal DPL printers */
  {10047,                       /* RP4 */
   4 * 72, 99 * 72,             /* Max paper size */
   1, 1,                        /* Min paper size */
   DPL_RES_203_203,     /* Resolutions */
   DPL_RES_203_203,
   DPL_RES_203_203,
   'I',
   'A',
   'E',
   },
};

static const stp_parameter_t the_parameters[] = {
  {
   "PageSize", N_("Page Size"), "Color=No,Category=Basic Printer Setup",
   N_("Size of the paper being printed to"),
   STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
   STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0},
  {
   "Resolution", N_("Resolution"), "Color=No,Category=Basic Printer Setup",
   N_("Resolution of the print"),
   STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
   STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0},
  {
    "PrintingMode", N_("Printing Mode"), "Color=Yes,Category=Core Parameter",
    N_("Printing Output Mode"),
    STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_CORE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
  },
#ifdef FIXME  /* Orientation not available for graphics, need
               * rotation routine in this driver */
  {
   "Orientation", N_("Orientation"), "Color=No,Category=Basic Printer Setup",
   N_("Orientation, Portrait, Landscape, Upside Down, Seascape"),
   STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
   STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0
   },
#endif
  {
   "LabelSeparator", N_("Media Index Type"),
   "Color=No,Category=Basic Printer Setup",
   N_("Gap, Notch, Hole, Black Mark, Continuous"),
   STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
   STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0,
   },
  {
   "Darkness", N_("Darkness"), "Color=No,Category=Basic Printer Setup",
   N_("Darkness Adjust, from 0 to 30"),
   STP_PARAMETER_TYPE_INT, STP_PARAMETER_CLASS_FEATURE,
   STP_PARAMETER_LEVEL_BASIC, 0, 1, STP_CHANNEL_NONE, 1, 0},
  {
   "Speed", N_("Print Speed"), "Color=No,Category=Basic Printer Setup",
   N_("Speed Adjust"),
   STP_PARAMETER_TYPE_STRING_LIST, STP_PARAMETER_CLASS_FEATURE,
   STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 0, 0},
};

static const int the_parameter_count =
  sizeof (the_parameters) / sizeof (const stp_parameter_t);

typedef struct
{
  const stp_parameter_t param;
  double min;
  double max;
  double defval;
  int color_only;
} float_param_t;

static const float_param_t float_parameters[] = {
  {
   {
    "HorizOffset", N_("Horizontal Offset"),
    "Color=No,Category=Basic Output Adjustment",
    N_("Adjust horizontal position"),
    STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED3, 1, 1, STP_CHANNEL_NONE, 1, 0,
    }, 0.0, 4.0, 0.0, 0},
  {
   {
    "VertOffset", N_("Vertical Offset"),
    "Color=No,Category=Basic Output Adjustment",
    N_("Adjust vertical position"),
    STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_OUTPUT,
    STP_PARAMETER_LEVEL_ADVANCED3, 1, 1, STP_CHANNEL_NONE, 1, 0,
    }, 0.0, 10.0, 0.0, 0},
  {
   {
    "Present", N_("Present Distance"),
    "Color=No,Category=Basic Output Adjustment",
    N_("Presnt Distance, 0.0 advances the default."),
    STP_PARAMETER_TYPE_DOUBLE, STP_PARAMETER_CLASS_FEATURE,
    STP_PARAMETER_LEVEL_BASIC, 1, 1, STP_CHANNEL_NONE, 1, 0,
    }, 0.0, 10.0, 0.0, 0},
};

static const int float_parameter_count =
  sizeof (float_parameters) / sizeof (const float_param_t);

/*
 * Convert a value into it's option name
 */

static const char *
dpl_val_to_string (const stp_vars_t *v,
		   int code,	/* I: Code */
		   const dpl_t * options,	/* I: Options */
		   int num_options)	/* I: Num options */
{

  int i;
  const char *string = NULL;

  /*
   * Look up the code in the table and convert to the string.
   */

  for (i = 0; i < num_options; i++)
    {
      if (code == options[i].dpl_code)
	{
	  string = options[i].dpl_name;
	  break;
	}
    }

  stp_dprintf (STP_DBG_DPL, v, "Code: %d, String: %s\n", code, string);

  return (string);
}

static const char *
dpl_val_to_text (const stp_vars_t *v,
		 int code,	/* I: Code */
		 const dpl_t * options,	/* I: Options */
		 int num_options)	/* I: Num options */
{

  int i;
  const char *string = NULL;

  /*
   * Look up the code in the table and convert to the string.
   */

  for (i = 0; i < num_options; i++)
    {
      if (code == options[i].dpl_code)
	{
	  string = gettext (options[i].dpl_text);
	  break;
	}
    }

  stp_dprintf (STP_DBG_DPL, v, "Code: %d, String: %s\n", code, string);

  return (string);
}

/*
 * dpl_get_model_capabilities() - Return struct of model capabilities
 */

static const dpl_cap_t *	/* O: Capabilities */
dpl_get_model_capabilities (const stp_vars_t *v)	/* I: Model */
{
  int i;
  int model = stp_get_model_id(v);
  int models = sizeof (dpl_model_capabilities) / sizeof (dpl_cap_t);
  for (i = 0; i < models; i++)
    {
      if (dpl_model_capabilities[i].model == model)
	{
	  return &(dpl_model_capabilities[i]);
	}
    }
  stp_eprintf (v, "dpl: model %d not found in capabilities list.\n", model);
  return &(dpl_model_capabilities[0]);
}

/*
 * Determine the current resolution
 */

static void
dpl_describe_resolution (const stp_vars_t * v, stp_resolution_t *x, stp_resolution_t *y)
{
  int i;
  const char *resolution = stp_get_string_parameter (v, "Resolution");

  *x = -1;
  *y = -1;
  if (resolution)
    {
      for (i = 0; i < NUM_RESOLUTIONS; i++)
	{
	  if (!strcmp (resolution, dpl_resolutions[i].dpl_name))
	    {
	      *x = dpl_resolutions[i].p0;
	      *y = dpl_resolutions[i].p1;
	    }
	}
    }
  if (*x != *y) {
    if (*x > *y) {
      *y = *x;
    } else {
      *x = *y;
    }
  }
}

int
dpl_get_multiplier (const stp_vars_t * v)
{
  stp_resolution_t x, y;
  int multiplier;
  int i;
  int max_dpi;
  const dpl_cap_t *caps = dpl_get_model_capabilities (v);

  for (i = 0; i < NUM_RESOLUTIONS; i++)
    {
      if (caps->max_resolution == dpl_resolutions[i].dpl_code)
	{
	  max_dpi = dpl_resolutions[i].p0;
	}
    }

  dpl_describe_resolution (v, &x, &y);

  if (x == max_dpi)
    multiplier = 1;
  else
    multiplier = 2;

  return multiplier;
}

#ifdef FIXME
/*
 * Orientation support - modes available
 * Note that the internal names MUST match those in cups/genppd.c else the
 * PPD files will not be generated correctly
 */

static const stp_param_string_t orientation_types[] = {
  {"Portrait", N_("Portrait")},
  {"Landscape", N_("Landscape")},
  {"UpsideDown", N_("Reverse Portrait")},
  {"Seascape", N_("Reverse Landscape")},
};

#define NUM_ORIENTATION (sizeof (orientation_types) / sizeof (stp_param_string_t))
#endif

/*
 * Label Separator Support for D-O printers, modes available
 */

static const stp_param_string_t label_separator_types[] = {
  {"IGNORE", N_("Printer Setting")},
  {"GAP", N_("Gap")},
  {"NOTCH", N_("Notch")},
  {"HOLE", N_("Hole")},
  {"MARK", N_("Black Mark")},
  {"CONTINUOUS", N_("Continuous")},
};

#define NUM_LABEL_SEPARATOR (sizeof (label_separator_types) / sizeof (stp_param_string_t))

/*
 * 'dpl_papersize_valid()' - Is the paper size valid for this printer.
 */

static int
dpl_papersize_valid (const stp_vars_t *v, const stp_papersize_t * pt)
{
  const dpl_cap_t *caps = dpl_get_model_capabilities (v);
  unsigned int pwidth = pt->width;
  unsigned int pheight = pt->height;

/*
 * Is it a valid name?
 */

  if (strlen (pt->name) <= 0)
    return (0);

/*
 * We are allowed custom paper sizes. Check that the size is within
 * limits.  Check that the name contains d-o if this is the
 * Datamax O'Neil label printer and not custom paper
 */

  if (pwidth <= caps->custom_max_width &&
      pheight <= caps->custom_max_height &&
      (pheight >= caps->custom_min_height || pheight == 0) &&
      (pwidth >= caps->custom_min_width || pwidth == 0))
    {
      if (strcmp (pt->name, "Custom"))
	{
	  if (NULL != strstr (pt->name, "d-o"))
	    {
	      return (1);
	    }
	  else
	    {
	      return (0);
	    }
	}
    }

  return (0);
}

/*
 * 'dpl_parameters()' - Return the parameter values for the given parameter.
 */

static stp_parameter_list_t
dpl_list_parameters (const stp_vars_t * v)
{
  stp_parameter_list_t *ret;
  int i;

  ret = stp_parameter_list_create ();
  for (i = 0; i < the_parameter_count; i++)
    stp_parameter_list_add_param (ret, &(the_parameters[i]));
  for (i = 0; i < float_parameter_count; i++)
    stp_parameter_list_add_param (ret, &(float_parameters[i].param));
  return ret;
}

static void
dpl_parameters (const stp_vars_t * v, const char *name,
		stp_parameter_t * description)
{
  int model = stp_get_model_id (v);
  int i;
  const dpl_cap_t *caps;
  description->p_type = STP_PARAMETER_TYPE_INVALID;

  if (name == NULL)
    return;

  stp_dprintf (STP_DBG_DPL, v, "dpl_parameters(): Name = %s\n", name);

  caps = dpl_get_model_capabilities (v);

  stp_dprintf (STP_DBG_DPL, v, "Printer model = %d\n", model);
  stp_dprintf (STP_DBG_DPL, v, "PageWidth = %d, PageHeight = %d\n",
		caps->custom_max_width, caps->custom_max_height);
  stp_dprintf (STP_DBG_DPL, v, "MinPageWidth = %d, MinPageHeight = %d\n",
		caps->custom_min_width, caps->custom_min_height);
  stp_dprintf (STP_DBG_DPL, v, "Resolutions: %d\n", caps->resolutions);

  for (i = 0; i < the_parameter_count; i++)
    if (strcmp (name, the_parameters[i].name) == 0)
      {
	stp_fill_parameter_settings (description, &(the_parameters[i]));
	break;
      }
  description->deflt.str = NULL;

  for (i = 0; i < float_parameter_count; i++)
    if (strcmp (name, float_parameters[i].param.name) == 0)
      {
	stp_fill_parameter_settings (description,
				     &(float_parameters[i].param));
	description->deflt.dbl = float_parameters[i].defval;
	description->bounds.dbl.upper = float_parameters[i].max;
	description->bounds.dbl.lower = float_parameters[i].min;
	break;
      }

  if (strcmp (name, "PageSize") == 0)
    {
      const stp_papersize_list_t *paper_sizes =
	stpi_get_papersize_list_named("labels", "");
      const stp_papersize_list_item_t *ptli =
	stpi_papersize_list_get_start(paper_sizes);
      description->bounds.str = stp_string_list_create ();
      while (ptli)
	{
	  const stp_papersize_t *pt = stpi_paperlist_item_get_data(ptli);
	  if (strlen (pt->name) > 0 && dpl_papersize_valid (v, pt))
	    stp_string_list_add_string(description->bounds.str,
				       pt->name, gettext(pt->text));
	  ptli = stpi_paperlist_item_next(ptli);
	}
      description->deflt.str =
	stp_string_list_param (description->bounds.str, 0)->name;
    }
  else if (strcmp (name, "Resolution") == 0)
    {
      description->bounds.str = stp_string_list_create ();
      description->deflt.str =
	dpl_val_to_string (v, caps->max_resolution, dpl_resolutions,
			   NUM_RESOLUTIONS);
      for (i = 0; i < NUM_RESOLUTIONS; i++)
	if (caps->resolutions & dpl_resolutions[i].dpl_code)
	  {
	    stp_string_list_add_string
	      (description->bounds.str,
	       dpl_val_to_string (v, dpl_resolutions[i].dpl_code,
				  dpl_resolutions, NUM_RESOLUTIONS),
	       dpl_val_to_text (v, dpl_resolutions[i].dpl_code,
				dpl_resolutions, NUM_RESOLUTIONS));
	  }
    }
  else if (strcmp(name, "PrintingMode") == 0)
    {
      description->bounds.str = stp_string_list_create();
      stp_string_list_add_string
        (description->bounds.str, "BW", _("Black and White"));
      description->deflt.str =
        stp_string_list_param(description->bounds.str, 0)->name;
    }
#ifdef FIXME
  else if (strcmp (name, "Orientation") == 0)
    {
      description->bounds.str = stp_string_list_create ();
      description->deflt.str = orientation_types[0].name;
      for (i = 0; i < NUM_ORIENTATION; i++)
	{
	  stp_string_list_add_string (description->bounds.str,
				      orientation_types[i].name,
				      gettext (orientation_types[i].text));
	}
    }
#endif
  else if (strcmp (name, "LabelSeparator") == 0)
    {
      description->bounds.str = stp_string_list_create ();
      description->deflt.str = label_separator_types[0].name;
      for (i = 0; i < NUM_LABEL_SEPARATOR; i++)
	{
	  stp_string_list_add_string (description->bounds.str,
				      label_separator_types[i].name,
				      gettext (label_separator_types[i].
					       text));
	}
    }
  else if (strcmp (name, "Darkness") == 0)
    {
      description->deflt.integer = -1;
      description->bounds.integer.lower = 0;
      description->bounds.integer.upper = 30;
    }
  else if (strcmp (name, "Speed") == 0)
    {
      description->bounds.str = stp_string_list_create ();
      stp_string_list_add_string (description->bounds.str, "None",
                                  _("Use Current Setting"));
      stp_string_list_add_string (description->bounds.str, "Default",
                                  _("Use Default Setting"));
      description->deflt.str = "None";
      for (i = 0; i < NUM_SPEEDS; i++)
	{
	  stp_string_list_add_string (description->bounds.str,
				      dpl_speeds[i].dpl_name,
				      gettext (dpl_speeds[i].
					       dpl_text));
	}
    }
  else if (strcmp (name, "HorizOffset") == 0 ||
	   strcmp (name, "VertOffset") == 0 || strcmp (name, "Present") == 0)
    {
      description->is_active = 1;
    }
}


/*
 * 'dpl_imageable_area()' - Return the imageable area of the page.
 */
static void
internal_imageable_area (const stp_vars_t * v,	/* I */
			 stp_dimension_t *left,	/* O - Left position in points */
			 stp_dimension_t *right,	/* O - Right position in points */
			 stp_dimension_t *bottom,	/* O - Bottom position in points */
			 stp_dimension_t *top)	/* O - Top position in points */
{
  stp_dimension_t width, height;		/* Size of page */

  stp_default_media_size (v, &width, &height);

  *left = 0;
  *right = width;
  *top = 0;
  *bottom = height;
}

static void
dpl_imageable_area (const stp_vars_t * v,	/* I */
		    stp_dimension_t *left,	/* O - Left position in points */
		    stp_dimension_t *right,	/* O - Right position in points */
		    stp_dimension_t *bottom,	/* O - Bottom position in points */
		    stp_dimension_t *top)	/* O - Top position in points */
{
  internal_imageable_area (v, left, right, bottom, top);
}

static void
dpl_limit (const stp_vars_t * v,	/* I */
	   stp_dimension_t *width, stp_dimension_t *height, stp_dimension_t *min_width, stp_dimension_t *min_height)
{
  const dpl_cap_t *caps = dpl_get_model_capabilities (v);
  *width = caps->custom_max_width;
  *height = caps->custom_max_height;
  *min_width = caps->custom_min_width;
  *min_height = caps->custom_min_height;
}

static const char *
dpl_describe_output (const stp_vars_t * v)
{
  return "Grayscale";
}

static const stp_papersize_t *
dpl_describe_papersize(const stp_vars_t *v, const char *name)
{
  return stpi_get_listed_papersize(name, "labels");
}

static void
pcx_header (stp_vars_t * v, stp_image_t * image)
{
  unsigned short height;
  unsigned short right;
  unsigned short top;		/* y = 0 is at bottom */
  unsigned short bytes;
  short xdpi;
  stp_resolution_t r_xdpi;
  stp_resolution_t *xdpi_p = (&r_xdpi);
  short ydpi;
  stp_resolution_t r_ydpi;
  stp_resolution_t *ydpi_p = (&r_ydpi);
  int n;
  const short zero = 0;

  stp_putc (10, v);		/* Signature */
  stp_putc (5, v);		/* Version */
  stp_putc (1, v);		/* RLE encoding */
  stp_putc (1, v);		/* bits per pixel */

  /* Get resolutions */
  dpl_describe_resolution (v, xdpi_p, ydpi_p);

  xdpi = (short) r_xdpi;
  ydpi = (short) r_ydpi;

  bytes = (xdpi * 4 + 7 ) / 8;	/* must be an even number */
  if (bytes != (bytes & 0xfffe))
    bytes++;

  height = stp_image_height (image);

  /*
   * Convert image size to printer resolution and setup the page for printing...
   */

  right = 4 * xdpi - 1;
  top = height - 1;

  /* send image start and end positions */
  stp_zfwrite ((const char *) &zero, 2, 1, v);
  stp_zfwrite ((const char *) &zero, 2, 1, v);
  stp_zfwrite ((const char *) &right, 2, 1, v);
  stp_zfwrite ((const char *) &top, 2, 1, v);

  /* send resolutions */
  stp_zfwrite ((const char *) &xdpi, 2, 1, v);
  stp_zfwrite ((const char *) &ydpi, 2, 1, v);

  /* send palette and reserved byte */
  for (n = 0; n < 3; n++)
    stp_putc (0, v);
  for (n = 0; n < 45; n++)
    stp_putc (0xff, v);
  stp_putc (0, v);

  stp_putc (1, v);		/* number of planes, monochrome */

  stp_zfwrite ((const char *) &bytes, 2, 1, v);

  stp_putc (1, v);		/* monochrome */
  stp_putc (0, v);

  stp_putc (0, v);		/* imagee size */
  stp_putc (0, v);
  stp_putc (0, v);
  stp_putc (0, v);

  for (n = 0; n < 54; n++)
    stp_putc (0, v);		/* padding */
}

/*
 * 'dpl_print()' - Print an image to an HP printer.
 */

static void
dpl_printfunc (stp_vars_t * v, int height)
{
  unsigned char *black = stp_dither_get_channel (v, STP_ECOLOR_K, 0);

  dpl_pcx (v, black, (height + 7) / 8, 1);
}

static double
get_double_param (stp_vars_t * v, const char *param)
{
  if (param && stp_check_float_parameter (v, param, STP_PARAMETER_ACTIVE))
    return stp_get_float_parameter (v, param);
  else
    return 1.0;
}

static int
dpl_do_print (stp_vars_t * v, stp_image_t * image)
{
  dpl_privdata_t privdata;
  int status = 1;
#ifdef FIXME
  const char *orientation_mode = stp_get_string_parameter (v, "Orientation");
#endif
  const char *label_separator_mode =
    stp_get_string_parameter (v, "LabelSeparator");
  double h_offset = get_double_param (v, "HorizOffset");
  double v_offset = get_double_param (v, "VertOffset");
  double present = get_double_param (v, "Present");
  int y;			/* Looping vars */
  stp_resolution_t xdpi, ydpi;		/* Resolution */
  int multiplier;
  unsigned char *black;		/* Black bitmap data */
  unsigned zero_mask;
  int image_height;
  int image_width;
  const dpl_cap_t *caps = dpl_get_model_capabilities (v);
  const char *speed = stp_get_string_parameter(v, "Speed");

  if (!stp_verify (v))
    {
      stp_eprintf (v, "Print options not verified; cannot print.\n");
      return 0;
    }

  /*
   * Setup a read-only pixel region for the entire image...
   */

  stp_image_init (image);

  stp_set_string_parameter (v, "ColorCorrection", "None");
#ifdef TESTING
  stp_set_float_parameter (v, "Brightness", 1.0);
  stp_set_float_parameter (v, "Contrast", 1.0);
  stp_set_float_parameter (v, "Gamma", 1.0);
#endif
  /*
   * Figure out the output resolution...
   */

  dpl_describe_resolution (v, &xdpi, &ydpi);

  stp_dprintf (STP_DBG_DPL, v, "dpl: resolution=%dx%d\n", (int) xdpi, (int) ydpi);
  if (xdpi <= 0 || ydpi <= 0)
    {
      stp_eprintf (v, "No resolution found; cannot print.\n");
      return 0;
    }

  image_height = stp_image_height (image);
  image_width = stp_image_width (image);

#ifdef FIXME
  privdata.orientation = 0;
  if ((strncmp (orientation_mode, "Landscape", 9) == 0))
    privdata.orientation = 1;
  else if ((strncmp (orientation_mode, "UpsideDown", 10) == 0))
    privdata.orientation = 2;
  else if ((strncmp (orientation_mode, "Seascape", 8) == 0))
    privdata.orientation = 3;
#endif

  /*
   * Label Separator mode
   */
  privdata.label_separator = 0;
  if ((strncmp (label_separator_mode, "GAP", 3) == 0))
    privdata.label_separator = 1;
  else if ((strncmp (label_separator_mode, "NOTCH", 5) == 0))
    privdata.label_separator = 1;
  else if ((strncmp (label_separator_mode, "HOLE", 4) == 0))
    privdata.label_separator = 1;
  else if ((strncmp (label_separator_mode, "MARK", 4) == 0))
    privdata.label_separator = 2;
  else if ((strncmp (label_separator_mode, "CONTINUOUS", 10) == 0))
    privdata.label_separator = 3;

  /*
   * Print Offsets
   */
  privdata.h_offset = (int) (h_offset * 100);	/* in 0.01 of an inch */
  privdata.v_offset = (int) (v_offset * 100);   /* in 0.01 of an inch */

  privdata.present = (int) (present * 100.0);   /* in 0.01 of an inch */

  /*
   * Darkness Mode
   */
  if (-1 != (privdata.darkness = stp_get_int_parameter (v, "Darkness")))
    {
      if (0 == privdata.darkness)
	{
	  privdata.darkness = 10;	/* default */
	}
    }

  /*
   * Speed Mode
   */
  privdata.speed = 0;
  if (0 != strcmp("None", speed))
    {
      if (0 == strcmp("Default", speed))
	{
          privdata.speed = (int) caps->default_speed;
        }
      else
        {
          int i;
          for (i = 0; i < NUM_SPEEDS; i++)
            {
              if (0 == strcmp(dpl_speeds[i].dpl_name, speed))
                {
                  privdata.speed = dpl_speeds[i].dpl_code;
                  break;
                }
            }
	  if (caps->min_speed > (char) (privdata.speed))
	    {
	      privdata.speed = caps->min_speed;
	    }
	  else
	    {
	      if (caps->max_speed < (char) (privdata.speed))
		{
		  privdata.speed = caps->max_speed;
		}
	    }
	}
    }

  /* workaround for printer bug */
  for (y=0; y<64; y++)
    stp_putc (0, v);

  /*
   * Send DPL initialization commands...
   */
  stp_puts ("\002n\r", v);	/* set Imperial units */

  /* Max page length */
  if (image_height / ydpi > 4)
    {
      stp_zprintf (v, "\002M%04i\r",
		   300 * image_height / ((int) ydpi) + (3 * privdata.v_offset));
    }
  else
    {
      stp_zprintf (v, "\002M%04i\r", 1200 + (3 * privdata.v_offset));
    }
  /* set Label Width */
  stp_zprintf (v, "\002KcLW%04i\r",
	       100 * image_width / ((int) xdpi) + privdata.h_offset);
  if (0 != privdata.label_separator)
    {
      if (1 == privdata.label_separator)
	{
	  stp_puts ("\002e\r", v);	/* edge mode */
	}
      else if (2 == privdata.label_separator)
	{
	  stp_puts ("\002r\r", v);	/* Mark mode */
	}
      else
	{
	  stp_zprintf (v, "\002c%04i\r", 100 *	/* Continuous mode */
		       image_height / ((int) ydpi) + privdata.v_offset);
	}
    }
  if (privdata.darkness > -1)
    {
      stp_zprintf (v, "\002KZH%02i\r", privdata.darkness);
    }
  if (privdata.speed > 0)
    {
      stp_zprintf (v, "\002KZP%c\r", privdata.speed);
    }
  stp_zprintf (v, "\002Kf%04i\r", privdata.present);
  stp_puts ("\002IDPcups0\r", v);	/* Save PCX file */
  pcx_header (v, image);


  stp_dprintf (STP_DBG_DPL, v, "Normal init\n");

  /*
   * Allocate memory for the raster data...
   */

  black = stp_malloc ((image_width + 7) / 8);

  stp_set_string_parameter (v, "STPIOutputType", "Grayscale");

  /* set up for very fast dithering as default */
  stp_set_string_parameter (v, "DitherAlgorithm", "VeryFast");

  stp_dither_init (v, image, image_width, xdpi, ydpi);

  stp_dither_add_channel (v, black, STP_ECOLOR_K, 0);
  stp_channel_set_black_channel (v, STP_ECOLOR_K);

  stp_channel_set_density_adjustment (v, STP_ECOLOR_K, 0,
				      get_double_param (v, "BlackDensity") *
				      1);

  (void) stp_color_init (v, image, 65536);

  stp_allocate_component_data (v, "Driver", NULL, NULL, &privdata);

  for (y = 0; y < image_height; y++)
    {
      if (stp_color_get_row (v, image, y, &zero_mask))
	{
	  status = 2;
	  break;
	}
      stp_dither (v, y, 0, 0, NULL);
      dpl_printfunc (v, image_width);
    }

  stp_puts ("\r\002L\r", v);	/* enter Label Formatting mode */
  multiplier = dpl_get_multiplier (v);	/* dot multiplier */
  stp_zprintf (v, "D%1i%1i\r", multiplier, multiplier);
  stp_puts ("R0000\r", v);	/* 0 offset, offset handled below */
  stp_puts ("A2\r", v);		/* transparent mode */
  /* load graphic */
  stp_zprintf (v, "1Y11000%04i%04icups0\r", privdata.v_offset,
	       privdata.h_offset);
  stp_puts ("Q0001\r", v);	/* one label */
  stp_puts ("E\r", v);		/* print now */
  stp_puts ("\002xDGcups0\r", v);	/* delete graphic */
#if 0
  stp_puts ("\002zD\r", v);	/* reclaim space */
#endif

  stp_image_conclude (image);

  /*
   * Cleanup...
   */

  if (black != NULL)
    stp_free (black);

  return status;
}

static int
dpl_print (const stp_vars_t * v, stp_image_t * image)
{
  int status;
  stp_vars_t *nv = stp_vars_create_copy (v);
  stp_prune_inactive_options (nv);
  status = dpl_do_print (nv, image);
  stp_vars_destroy (nv);
  return status;
}

static const stp_printfuncs_t print_dpl_printfuncs = {
  dpl_list_parameters,
  dpl_parameters,
  stp_default_media_size,
  dpl_imageable_area,
  dpl_imageable_area,
  dpl_limit,
  dpl_print,
  dpl_describe_resolution,
  dpl_describe_output,
  stp_verify_printer_params,
  NULL,
  NULL,
  NULL,
  dpl_describe_papersize
};


static void
dpl_pcx (stp_vars_t * v,	/* I - Print file or command */
	 unsigned char *short_line,	/* I - Output bitmap data */
	 int height,		/* I - Height of bitmap data */
	 int last_plane)	/* I - True if this is the last plane */
{
  unsigned char *line;
  unsigned char *data;
  unsigned char stored;
  int add_bytes = 0;
  int count = 0;
  int in = 0;
  int out = 0;
  stp_resolution_t xdpi, ydpi;
  const dpl_cap_t *caps = dpl_get_model_capabilities (v);
  int i;
  int max_dpi;
  int dpi_adjust;

  /* Each line has to be 4 inches long */
  dpl_describe_resolution (v, &xdpi, &ydpi);

  for (i = 0; i < NUM_RESOLUTIONS; i++)
    {
      if (caps->max_resolution == dpl_resolutions[i].dpl_code)
	{
	  max_dpi = dpl_resolutions[i].p0;
	}
      if (caps->resolution_adjust == dpl_resolutions[i].dpl_code)
	{
	  dpi_adjust = dpl_resolutions[i].p0;
	}
    }

  if (xdpi == max_dpi)
    {
      add_bytes = ((xdpi * 4) + 7) / 8 - height;
    }
  else
    {
      add_bytes = ((dpi_adjust * 4) + 7) / 8 - height;
    }

  /* allocate 4 inch input buffer */
  line = (unsigned char *) stp_malloc (height + add_bytes);
  /* allocate output buffer, worst case */
  data = (unsigned char *) stp_malloc ((height + add_bytes) * 2);

  /* invert data, cups makes white 1 and black 0, printer wants the opposite */
  for (in = 0; in < height; in++)
    {
      line[in] = 0xff ^ short_line[in];
    }

  /* pad to 4 inches */
  for (in = height; in < (height + add_bytes); in++)
    {
      line[in] = 0xff;
    }

  in = 0;
  while (in < (height + add_bytes))
    {
      stored = line[in];	/* save the value */
      for (count = 1; in + count < (height + add_bytes) && line[in + count] == stored && count < 63; count++);	/* count the run */

      /* test to see if we need to make a run of one because the data value
         has the two top bits set and see if we actually have a run */
      if (stored > 191 || count > 1)
	{
	  data[out++] = count | 0xc0;	/* mask to indicate a run */
	  data[out++] = stored;	/* output the value */
	}
      else			/* not a run */
	{
	  data[out++] = stored;	/* output the value */
	}
      in += count;
    }
  stp_zfwrite ((const char *) data, out, 1, v);
  stp_free (line);
  stp_free (data);
}


static stp_family_t print_dpl_module_data = {
  &print_dpl_printfuncs,
  NULL
};


static int
print_dpl_module_init (void)
{
  return stpi_family_register (print_dpl_module_data.printer_list);
}


static int
print_dpl_module_exit (void)
{
  return stpi_family_unregister (print_dpl_module_data.printer_list);
}


/* Module header */
#define stp_module_version print_dpl_LTX_stp_module_version
#define stp_module_data print_dpl_LTX_stp_module_data

stp_module_version_t stp_module_version = { 0, 0 };

stp_module_t stp_module_data = {
  "dpl",
  VERSION,
  "DPL family driver",
  STP_MODULE_CLASS_FAMILY,
  NULL,
  print_dpl_module_init,
  print_dpl_module_exit,
  (void *) &print_dpl_module_data
};
