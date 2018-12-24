/*
 *
 *   Print plug-in for the GIMP.
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
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gutenprintui2/gutenprintui.h>
#include "print_gimp.h"

#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "print-intl.h"

/*
 * Local functions...
 */

static void    query           (void);
static void    run             (const char        *name,
                                gint               nparams,
                                const GimpParam   *param,
                                gint              *nreturn_vals,
                                GimpParam        **return_vals);
static int     do_print_dialog (const gchar *proc_name,
                                gint32       image_ID);

/*
 * Globals...
 */

GimpPlugInInfo	PLUG_IN_INFO =		/* Plug-in information */
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

static stpui_plist_t gimp_vars;

/*
 * 'main()' - Main entry - just call gimp_main()...
 */

MAIN()

/*
 * 'query()' - Respond to a plug-in query...
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
static void
query (void)
{
  static GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,	  (BAD_CONST_CHAR) "run_mode",(BAD_CONST_CHAR) "Interactive, non-interactive" },
    { GIMP_PDB_IMAGE,	  (BAD_CONST_CHAR) "image",   (BAD_CONST_CHAR) "Input image" },
    { GIMP_PDB_DRAWABLE,(BAD_CONST_CHAR) "drawable",(BAD_CONST_CHAR) "Input drawable" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "driver",	(BAD_CONST_CHAR) "Printer driver short name" },
    { GIMP_PDB_STRING,	(BAD_CONST_CHAR) "printer_queue",(BAD_CONST_CHAR) "CUPS Printer Queue" },
    { GIMP_PDB_FLOAT,   (BAD_CONST_CHAR) "left",    (BAD_CONST_CHAR) "Left offset (points, -1 = centered)" },
    { GIMP_PDB_FLOAT,   (BAD_CONST_CHAR) "top",     (BAD_CONST_CHAR) "Top offset (points, -1 = centered)" },
    { GIMP_PDB_INT32,   (BAD_CONST_CHAR) "length_key_value_array", (BAD_CONST_CHAR) "Length of the key-value array" },
    { GIMP_PDB_STRINGARRAY, (BAD_CONST_CHAR) "keys", (BAD_CONST_CHAR) "Key-value pairs for Gutenprint Settings" },
  };

  static const gchar *blurb = "This plug-in prints images from The GIMP using Gutenprint directly.";
  static const gchar *help  = "Prints images to many printers.";
  static const gchar *auth  = "Michael Sweet <mike@easysw.com> and Robert Krawitz <rlk@alum.mit.edu>";
  static const gchar *copy  = "Copyright 1997-2006 by Michael Sweet and Robert Krawitz";
  static const gchar *types = "RGB*,GRAY*,INDEXED*";

  gimp_plugin_domain_register (cast_safe(PACKAGE), cast_safe(PACKAGE_LOCALE_DIR));

  do_gimp_install_procedure(blurb, help, auth, copy, types, G_N_ELEMENTS(args),
			    args);
}
#pragma GCC diagnostic pop

static guchar *gimp_thumbnail_data = NULL;

static guchar *
stpui_get_thumbnail_data_function(void *image_ID, gint *width, gint *height,
				  gint *bpp, gint page)
{
  if (gimp_thumbnail_data)
    g_free(gimp_thumbnail_data);
  gint x = gimp_image_width(image_ID);
  gint y = gimp_image_height(image_ID);
  if (*width > x)
    *width = x;
  if (*height > y)
    *height = y;
  gimp_thumbnail_data =
    gimp_image_get_thumbnail_data(p2gint(image_ID), width, height, bpp);
  return gimp_thumbnail_data;
}

/*
 * 'run()' - Run the plug-in...
 */

volatile int SDEBUG = 1;

static void
run (const char        *name,		/* I - Name of print program. */
     gint               nparams,	/* I - Number of parameters passed in */
     const GimpParam   *param,		/* I - Parameter values */
     gint              *nreturn_vals,	/* O - Number of return values */
     GimpParam        **return_vals)	/* O - Return values */
{
  GimpDrawable	*drawable;	/* Drawable for image */
  GimpRunMode	 run_mode;	/* Current run mode */
  GimpParam	*values;	/* Return values */
  gint32         drawable_ID;   /* drawable ID */
  GimpExportReturn export = GIMP_EXPORT_CANCEL;    /* return value of gimp_export_image() */
  gdouble xres, yres;
  char *image_filename;
  stpui_image_t *image;
  gint32 image_ID;
  gint32 base_type;
  stp_parameter_t desc;
  if (getenv("STP_DEBUG_STARTUP"))
    {
      fprintf(stderr, "pid is %d\n", getpid());
      while (SDEBUG)
	;
    }

 /*
  * Initialise libgutenprint
  */

  stp_init();

  stp_set_output_codeset("UTF-8");

#ifdef INIT_I18N_UI
  INIT_I18N_UI();
#else
  /*
   * With GCC and glib 1.2, there will be a warning here about braces in
   * expressions.  Getting rid of it causes more problems than it solves.
   * In particular, turning on -ansi on the command line causes a number of
   * other useful things, such as strcasecmp, popen, and snprintf to go away
   */
  INIT_LOCALE (PACKAGE);
#endif

  stpui_printer_initialize(&gimp_vars);
  /*
   * Initialize parameter data...
   */

  run_mode = (GimpRunMode)param[0].data.d_int32;

  values = g_new (GimpParam, 1);

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_SUCCESS;

  *nreturn_vals = 1;
  *return_vals  = values;

  image_ID = param[1].data.d_int32;
  drawable_ID = param[2].data.d_int32;

  image_filename = gimp_image_get_filename (image_ID);
  if (image_filename)
    {
      if (strchr(image_filename, '/'))
	image_filename = strrchr(image_filename, '/') + 1;
      stpui_set_image_filename(image_filename);
      /* g_free(image_filename); */
    }
  else
    stpui_set_image_filename("Untitled");

  /*  eventually export the image */
  switch (run_mode)
  {
  case GIMP_RUN_INTERACTIVE:
    case GIMP_RUN_WITH_LAST_VALS:
      gimp_ui_init ("print", TRUE);
      export = gimp_export_image (&image_ID, &drawable_ID, "Print",
                                  (GIMP_EXPORT_CAN_HANDLE_RGB |
                                   GIMP_EXPORT_CAN_HANDLE_GRAY |
                                   GIMP_EXPORT_CAN_HANDLE_INDEXED |
                                   GIMP_EXPORT_CAN_HANDLE_ALPHA));
      if (export == GIMP_EXPORT_CANCEL)
      {
        values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
        fprintf(stderr,"Cannot handle image type\n");
        return;
      }
      break;
  default:
    break;
  }

  /*
   * Get drawable...
   */

  drawable = gimp_drawable_get (drawable_ID);
  stpui_set_image_dimensions(drawable->width, drawable->height);
  gimp_image_get_resolution (image_ID, &xres, &yres);
  stpui_set_image_resolution(xres, yres);
  stpui_set_image_channel_depth(8);
  base_type = gimp_image_base_type(image_ID);
  switch (base_type)
  {
  case GIMP_INDEXED:
  case GIMP_RGB:
    stpui_set_image_type("RGB");
    break;
  case GIMP_GRAY:
    stpui_set_image_type("Whitescale");
    break;
  default:
    break;
  }

  image = Image_GimpDrawable_new(drawable, image_ID);
  stp_set_float_parameter(gimp_vars.v, "AppGamma", gimp_gamma());

  /*
   * See how we will run
   */

  switch (run_mode)
  {
  case GIMP_RUN_INTERACTIVE:
    /*
     * Get information from the dialog...
     */

    if (!do_print_dialog (name, image_ID))
    {
      values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
    }
    else
    {
      stpui_plist_copy(&gimp_vars, stpui_get_current_printer());
    }
    break;

  case GIMP_RUN_NONINTERACTIVE:
    /*
     * Make sure all the arguments are present...
     */
    if (nparams < 9)
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
    else
    {
      gimp_image_get_resolution(image_ID, &xres, &yres);
      gdouble pixwidth = gimp_drawable_width(drawable_ID);
      gdouble pixheight = gimp_drawable_height(drawable_ID);
      gdouble pointwidth = gimp_pixels_to_units(pixwidth, GIMP_UNIT_POINT, xres);
      gdouble pointheight = gimp_pixels_to_units(pixheight, GIMP_UNIT_POINT, yres);

      stp_set_height(gimp_vars.v, pointheight);
      stp_set_width(gimp_vars.v, pointwidth);

      /*
       * Avoid calling stpui_print with an invalid driver (SEGFAULT)
       */

      if (! stp_get_printer_by_driver(param[3].data.d_string) )
      {
        values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        fprintf(stderr, "Unknown driver %s\n", stp_get_driver(gimp_vars.v));
        break;
      }

      stp_set_driver(gimp_vars.v, param[3].data.d_string);

      stpui_plist_set_queue_name(&gimp_vars, param[4].data.d_string);

      /*
       * Left offset (points, -1 = centered)
       */

      stp_set_left(gimp_vars.v, param[6].data.d_float);

      /*
       * Top offset (points, -1 = centered)
       */

      stp_set_top(gimp_vars.v, param[5].data.d_float);

      /*
       * Parse remaining parameters from key-value string array
       */

      int kv_arr_len = param[7].data.d_int32;

      if (kv_arr_len % 2 != 0)
      {
        /*
         * Key with no Value
         */

        values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
        fprintf(stderr,"Key with no value provided\n");
      } else {

	int k = 0;
        for( k=0; k<kv_arr_len && values[0].data.d_status == GIMP_PDB_SUCCESS; k+=2 )
        {
          char *key = param[8].data.d_stringarray[k];
          char *value = param[8].data.d_stringarray[k + 1];
          char *endptr = NULL;
          float float_value = 0;

          stp_describe_parameter(gimp_vars.v, key, &desc);

          switch(desc.p_type)
          {
          case STP_PARAMETER_TYPE_STRING_LIST:

            /*
             * Some useful string stp parameters
             *
             * "PrintingMode": BW, Color
             * "Resolution": "300", "720", etc.
             * "PageSize": "Letter", "A4", etc. TODO: Support Custom
             * "MediaType": "Plain", "Glossy", etc.
             * "InputSlot": "Tray1", "Manual", etc.
             * "ColorCorrection": Color Correction model
             * "InkType": Type of ink or cartridge
             * "InkSet": Set of inks to use
             * "DitherAlgorithm": Dither algorithm
             * "Weave": Weave method
             * "PrintingDirection": "Bidirectional", "Unidirectional"
             */

            stp_set_string_parameter(gimp_vars.v, key, value);
            break;

          case STP_PARAMETER_TYPE_INT:
            stp_set_int_parameter(gimp_vars.v, key, atoi(value));
            break;

          case STP_PARAMETER_TYPE_BOOLEAN:
            stp_set_boolean_parameter(gimp_vars.v, key, atoi(value));
            break;

          case STP_PARAMETER_TYPE_DOUBLE:
          case STP_PARAMETER_TYPE_DIMENSION:

            /*
             * Some useful floating point stp parameters
             *
             * "Brightness"  0-400%
             * "Gamma"  Output gamma 0.1 - 3.0
             * "Contrast" 0.1 - 3.0
             * "Saturation"  0-1000%
             * "Density"  0-200%
             * "DropSize1"  0.0-1.0
             * "DropSize2"  0.0-1.0
             * "DropSize3"  0.0-1.0
             */

            float_value = strtof(value, &endptr);
            if (float_value == 0 && endptr == value)
            {
              /*
               * No conversion was performed -- invalid floating point number
               */
              *nreturn_vals = 1;
              values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
              fprintf(stderr,"Invalid floating point value provided for key: %s\n", key);
            }
            else
            {
	      if (desc.p_type == STP_PARAMETER_TYPE_DOUBLE)
		stp_set_float_parameter(gimp_vars.v, key, float_value);
	      else
		stp_set_dimension_parameter(gimp_vars.v, key, float_value);
            }
            break;

          case STP_PARAMETER_TYPE_CURVE:
          case STP_PARAMETER_TYPE_FILE:
          case STP_PARAMETER_TYPE_RAW:
          case STP_PARAMETER_TYPE_ARRAY:
            values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
            fprintf(stderr,"Parameter type unsupported in gimp2 plugin for parameter %s\n", key);
            break;

          case STP_PARAMETER_TYPE_INVALID:

            /*
             * Output scaling (0-100%, -PPI)
             */

            if (strncmp("Scaling", key, 7) == 0)
            {
              float_value = strtof(value, &endptr);
              if (float_value == 0 && endptr == value)
              {
                /*
                 * No conversion was performed -- invalid floating point number
                 */

                values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
                fprintf(stderr,"Invalid floating point value provided for key: Scaling\n");
              }
              else
              {
                gimp_vars.scaling = float_value;

                if (gimp_vars.scaling == 0) {
                  values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
                  fprintf(stderr,"Scaling cannot be 0\n");
                }
                else if (gimp_vars.scaling > 0)
                {
                  /*
                   * Scaling > 0 in %
                   */

                  stp_set_width(gimp_vars.v, pointwidth * gimp_vars.scaling / 100.);
                  stp_set_height(gimp_vars.v, pointheight * gimp_vars.scaling / 100.);
                }
                else /* gimp_vars < 0 */
                {
                  /*
                   * Scaling < 0 in DPI
                   */

                  pointwidth = gimp_pixels_to_units(pixwidth, GIMP_UNIT_POINT, -gimp_vars.scaling);
                  pointheight = gimp_pixels_to_units(pixheight, GIMP_UNIT_POINT, -gimp_vars.scaling);
                  stp_set_width(gimp_vars.v, pointwidth);
                  stp_set_height(gimp_vars.v, pointheight);
                }
              }
              break;
            }

            /*
             * Output orientation (-1 = auto, 0 = portrait, 1 = landscape)
             */

            else if (strncmp("Orientation", key, 11) == 0)
            {
              gimp_vars.orientation = atoi(value);
              break;
            }

          default:
            values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
            fprintf(stderr,"Parameter unsupported in gimp2 plugin for parameter %s\n", key);
            break;
          }
	  stp_parameter_description_destroy(&desc);
        }
      }
    }
    break;

  case GIMP_RUN_WITH_LAST_VALS:
    values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
    break;

  default:
    values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
    break;
  }

  if (gimp_thumbnail_data)
    g_free(gimp_thumbnail_data);

  /*
   * Print the image...
   */
  if (values[0].data.d_status == GIMP_PDB_SUCCESS)
  {
    /*
     * Set the tile cache size...
     */

    if (drawable->height > drawable->width)
      gimp_tile_cache_ntiles ((drawable->height + gimp_tile_width () - 1) /
                              gimp_tile_width () + 1);
    else
      gimp_tile_cache_ntiles ((drawable->width + gimp_tile_width () - 1) /
                              gimp_tile_width () + 1);

    if (! stpui_print(&gimp_vars, image))
    {
      values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
    }

    /*
     * Store data...
     * FIXME! This is broken!
     */

#if 0
    if (run_mode == GIMP_RUN_INTERACTIVE)
      gimp_set_data (PLUG_IN_NAME, vars, sizeof (vars));
#endif
  }

  /*
   * Detach from the drawable...
   */
  gimp_drawable_detach (drawable);

  if (export == GIMP_EXPORT_EXPORT)
    gimp_image_delete (image_ID);
  stp_vars_destroy(gimp_vars.v);
}

/*
 * 'do_print_dialog()' - Pop up the print dialog...
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
static void
gimp_errfunc(void *file, const char *buf, size_t bytes)
{
  char formatbuf[32];
  snprintf(formatbuf, 31, "%%%lus", (unsigned long) bytes);
  g_message(formatbuf, buf);
}
#pragma GCC diagnostic pop

static gint
do_print_dialog (const gchar *proc_name,
		 gint32       image_ID)
{
 /*
  * Generate the filename for the current user...
  */
  char *filename = gimp_personal_rc_file (cast_safe("printrc"));
  stpui_set_printrc_file(filename);
  g_free(filename);
  if (! getenv("STP_PRINT_MESSAGES_TO_STDERR"))
    stpui_set_errfunc(gimp_errfunc);
  stpui_set_thumbnail_func(stpui_get_thumbnail_data_function);
  stpui_set_thumbnail_data(gint2p(image_ID));
  return stpui_do_print_dialog();
}
